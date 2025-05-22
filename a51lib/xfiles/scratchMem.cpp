
#include "scratchMem.h"

#include <cstring>
#include <algorithm>
#include <cassert>
#include <cstdio>

//==============================================================================
//  
//  
//              +---BufferTop                             StackTop---+
//              |                                                    |
//              |                            StackMarker[]--+--+--+  |
//              |                                           |  |  |  |
//              V                                       ... v  v  v  V
//  Storage---> +----------------------------------------------------+
//              |                                                    |
//              |    Buffer -->                         <-- Stack    |
//              |                                                    |
//              +----------------------------------------------------+
//  
//==============================================================================


#define SMEM_ALIGNMENT  SMEM_ALIGN(1)       // Do not change this.

//==============================================================================
//  STORAGE
//==============================================================================

bool   g_SMemInitialized = false;
uint8_t*   g_pSMemBufferTop;
uint8_t*   g_pSMemStackTop;
uint8_t*   g_pSMemMarker[ MAX_MARKERS ];
int     g_SMemNextMarker;
int     g_SMemThreadId = -1;

static  int     s_ActiveID = 0 ;    
                
static  int     s_Active;
static  uint8_t*   s_pStorage[2];
static  uint8_t*   s_pAllocedStorage[2];
static  int     s_CurrentSize[2];
static  int     s_RequestedSize;
                
static  uint8_t    s_Signature[ SMEM_ALIGNMENT ];
        int     SCRATCH_MEM_MAX_USED = 0;

enum calltype
{
    BUFFER_ALLOC = 0,
    STACK_ALLOC,
    STACK_PUSH_MARKER,
    STACK_POP_MARKER,
};

int smem_GetBufferSize()
{
    return s_RequestedSize;
}

//==============================================================================

int smem_GetMaxUsed()
{
    return SCRATCH_MEM_MAX_USED;
}

//==============================================================================

void smem_SetThreadId( int ThreadId )
{
    g_SMemThreadId = ThreadId;
}

static
void smem_ActivateSection()
{
    g_pSMemBufferTop = s_pStorage[ s_Active ];
    g_pSMemStackTop  = g_pSMemBufferTop + s_CurrentSize[ s_Active ];
     
    // Place a signature below the buffer region for overrun checking.
    memcpy( g_pSMemBufferTop, s_Signature, SMEM_ALIGNMENT );
    g_pSMemBufferTop += SMEM_ALIGNMENT;

    // We also place a signature above the stack region.
    g_pSMemStackTop -= SMEM_ALIGNMENT;
    memcpy( g_pSMemStackTop, s_Signature, SMEM_ALIGNMENT );

    // The next available marker is 0.
    g_SMemNextMarker = 0;
}

//==============================================================================
//  Exposed functions
//==============================================================================

void smem_Init( int NBytes )
{
    assert( !g_SMemInitialized );
    assert( NBytes >= 0 );

    // Build the signature.
    memset( s_Signature, '~', SMEM_ALIGNMENT );
    memcpy( s_Signature + (SMEM_ALIGNMENT>>1) - 2, "SMEM", 4 );

    s_RequestedSize = SMEM_ALIGN( NBytes );

    s_pAllocedStorage[0] = (uint8_t*)malloc( s_RequestedSize + 64 );
    s_pAllocedStorage[1] = (uint8_t*)malloc( s_RequestedSize + 64 );
    s_pStorage[0] = (uint8_t*)ALIGN_64( s_pAllocedStorage[0] );
    s_pStorage[1] = (uint8_t*)ALIGN_64( s_pAllocedStorage[1] );
    s_CurrentSize[1] = s_RequestedSize;
    s_CurrentSize[0] = s_RequestedSize;

    s_Active = 0;
    smem_ActivateSection();

    g_SMemInitialized = true;
    SCRATCH_MEM_MAX_USED = 0;
}

//==============================================================================

void smem_Kill()
{
    assert( g_SMemInitialized );
    assert( g_SMemNextMarker == 0 );

    free( s_pAllocedStorage[0] );
    free( s_pAllocedStorage[1] );
    s_pAllocedStorage[0] = NULL;
    s_pAllocedStorage[1] = NULL;
    s_pStorage[0]        = NULL;
    s_pStorage[1]        = NULL;
    s_CurrentSize[0] = 0;
    s_CurrentSize[1] = 0;

    g_SMemInitialized = false;
}

//==============================================================================

extern bool g_bInsideRTF;

void smem_Toggle()
{
    assert( g_SMemInitialized );
    assert( g_SMemNextMarker == 0 );

    // Check to see if the signature under the buffer was violated.
    //
    // NOTE - There is a "signature" under the buffer area of scratch memory.
    //        This signature is no longer intact.  If you got an assert failure
    //        here, then some code is trashing memory.
    //
    assert( memcmp( s_pStorage[s_Active], 
                      s_Signature, SMEM_ALIGNMENT ) == 0 );

    // Make sure the stack wasn't blown.
    //
    // NOTE - If you got an assert failure here, then you probably overran a 
    //        scratch memory stack allocation.
    //
    assert( memcmp( s_pStorage[s_Active] + 
                        s_CurrentSize[s_Active] - 
                        SMEM_ALIGNMENT, 
                      s_Signature, SMEM_ALIGNMENT ) == 0 );

    // Remember most used
    SCRATCH_MEM_MAX_USED = std::max( (g_pSMemBufferTop-s_pStorage[ s_Active ]), (ptrdiff_t)SCRATCH_MEM_MAX_USED );

    // To make life a little easier, get the index to the non-active section.
    int Other = 1 - s_Active;

    // Right now, "s_Active" is active.  The "other" section must no longer be
    // needed, otherwise we wouldn't be here.  If the program has requested a 
    // change in scratch memory size, we can go ahead and resize the "other" 
    // section now.

    if( s_CurrentSize[Other] != s_RequestedSize )
    {
        free( s_pAllocedStorage[Other] );
        s_pAllocedStorage[Other] = (uint8_t*)malloc( s_RequestedSize + 64 );
        s_pStorage[Other] = (uint8_t*)ALIGN_64( s_pAllocedStorage[Other] );
        s_CurrentSize[Other] = s_RequestedSize;
    }

    // Toggle the sections.

    s_Active = Other;
    smem_ActivateSection();

    // Next ID
    s_ActiveID++ ;
}

void smem_ResetAfterException()
{
    g_SMemNextMarker = 0;
    smem_Toggle();
}

void smem_ChangeSize( int NBytes )
{
    assert( g_SMemInitialized );
    assert( NBytes >= 0 );
    s_RequestedSize = SMEM_ALIGN( NBytes );
}

int smem_GetActiveID()
{
    return s_ActiveID ;    
}

uint8_t* smem_GetActiveStartAddr() 
{
    // Get start of current buffer
    uint8_t* Addr = s_pStorage[ s_Active ];

    // Skip signature
    Addr += SMEM_ALIGNMENT ;

    return Addr ;
}

uint8_t* smem_GetActiveEndAddr()
{
    // Just return current active buffer position
    return g_pSMemBufferTop ;
}

void smem_Validate()
{

}
