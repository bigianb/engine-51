#include "Random.h"

static Random s_Random( 2 );

//==============================================================================
//  Global "standard" pseudo random number generation functions.
//==============================================================================

void x_srand( int Seed )
{
    s_Random.srand( Seed );
}

//==============================================================================

int x_rand( void )
{
    return( s_Random.rand() );
}

//==============================================================================

int x_irand( int Min, int Max )
{
    return( s_Random.irand( Min, Max ) );
}

//==============================================================================

float x_frand( float Min, float Max )
{
    return( s_Random.frand( Min, Max ) );
}

//==============================================================================
//  Functions for class Random.
//==============================================================================

Random::Random( void )
{
    m_Seed = 2;
}

//==============================================================================

Random::Random( int Seed )
{
    m_Seed = Seed;
}                

//==============================================================================

void Random::srand( int Seed )
{
    m_Seed = Seed;
}

//==============================================================================

int Random::rand( void )
{
    m_Seed = m_Seed * 214013 + 2531011;
    return( (int)((m_Seed >> 16) & X_RAND_MAX) );
}

//==============================================================================

int Random::irand( int Min, int Max )
{
    //ASSERT( Max >= Min );
    return( (rand() % (Max-Min+1)) + Min );
}

//==============================================================================

float Random::frand( float Min, float Max )
{
    //ASSERT( Max >= Min );
    return( (((float)rand() / (float)X_RAND_MAX) * (Max-Min)) + Min );
}

//==============================================================================

Vector2 Random::v2( float MinX, float MaxX, float MinY, float MaxY )
{
    return( Vector2( frand(MinX,MaxX),
                     frand(MinY,MaxY) ) );
}

//==============================================================================

Vector3 Random::v3( float MinX, float MaxX, float MinY, float MaxY, float MinZ, float MaxZ )
{
    return( Vector3( frand(MinX,MaxX),
                     frand(MinY,MaxY),
                     frand(MinZ,MaxZ) ) );
}

//==============================================================================

Colour Random::color( uint8_t A )
{
    uint8_t r = (uint8_t)irand(0,255);
    uint8_t g = (uint8_t)irand(0,255);
    uint8_t b = (uint8_t)irand(0,255);

    return( Colour( r, g, b, A ) );
}
