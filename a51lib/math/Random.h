#pragma once

#include "Vector3.h"
#include "../Colour.h"

#define X_RAND_MAX 0x7FFF

//==============================================================================

void  x_srand(int Seed);
int   x_rand(void);                  // Result in [   0, X_RAND_MAX ]
int   x_irand(int Min, int Max);     // Result in [ Min, Max        ]
float x_frand(float Min, float Max); // Result in [ Min, Max        ]

//==============================================================================

class Random
{
public:
    Random();
    Random(int Seed);

    void  srand(int Seed);
    int   rand(void);                  // Result in [   0, X_RAND_MAX ]
    int   irand(int Min, int Max);     // Result in [ Min, Max        ]
    float frand(float Min, float Max); // Result in [ Min, Max        ]

    Vector2 v2(float MinX, float MaxX, float MinY, float MaxY);
    Vector3 v3(float MinX, float MaxX, float MinY, float MaxY, float MinZ, float MaxZ);
    Colour  color(uint8_t A = 255);

private:
    int m_Seed;
};
