#include "xfs.h"

#include <cassert>
#include <cstdio>
#include <cstdarg>

#define ALIGN_4(n)      ( (((int)(n)) +   3) & (  -4) )
#define X_GLOBAL_STRING_BUFFER_SIZE  2048

static char             s_StringBuffer[X_GLOBAL_STRING_BUFFER_SIZE];
static int NextOffset = 0;

static
char* FormatIntoStringBuffer( const char* pFormatString, va_list Args )
{
    int* pNChars  = (int*)(s_StringBuffer + NextOffset);
    char* pResult  = s_StringBuffer + NextOffset + sizeof(int);
    *pNChars = vsnprintf( pResult, X_GLOBAL_STRING_BUFFER_SIZE - NextOffset - sizeof(int), pFormatString, Args );

    NextOffset = ALIGN_4( NextOffset + *pNChars + sizeof(int) + 1 );

    // Make sure we did not overflow the string buffer.
    assert(NextOffset < X_GLOBAL_STRING_BUFFER_SIZE );

    return( pResult );
}

static
void ReleaseFromStringBuffer( const char* pString )
{
    int NewOffset = pString - s_StringBuffer - sizeof(int);

    // Make sure we are always "releasing memory" in the buffer.  That is, when
    // an xfs destructs, its pointer should be "behind" the current global 
    // offset.
    assert( NewOffset <= NextOffset );

    NextOffset = NewOffset;
    s_StringBuffer[ NewOffset+0 ] = '\0';
    s_StringBuffer[ NewOffset+1 ] = '\0';
    s_StringBuffer[ NewOffset+2 ] = '\0';
    s_StringBuffer[ NewOffset+3 ] = '\0';
    s_StringBuffer[ NewOffset+4 ] = '\0';
}

xfs::xfs( const char* pFormatString, ... )
{
    va_list   Args;
    va_start( Args, pFormatString );

    m_pString = FormatIntoStringBuffer( pFormatString, Args );
}

xfs::operator const char* (  )
{
    return( m_pString );
}

xfs::~xfs(  )
{
    ReleaseFromStringBuffer( m_pString );
}
