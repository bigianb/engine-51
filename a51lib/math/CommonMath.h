#pragma once

#include <cmath>
#include <cassert>
#include <algorithm>

#ifdef PI
#undef PI
#endif
#define PI (3.14159265358979323846f)

#define RADIAN(A) ((float)((A) * 0.0174532925199432957692369055556f))
#define DEG_TO_RAD(A) ((float)((A) * 0.0174532925199432957692369055556f))
#define RAD_TO_DEG(A) ((float)((A) * 57.295779513082320876798161804285f))

#define R_0 RADIAN(0)
#define R_1 RADIAN(1)
#define R_2 RADIAN(2)
#define R_3 RADIAN(3)
#define R_4 RADIAN(4)
#define R_5 RADIAN(5)
#define R_8 RADIAN(8)
#define R_10 RADIAN(10)
#define R_20 RADIAN(20)
#define R_40 RADIAN(40)
#define R_45 RADIAN(45)
#define R_60 RADIAN(60)
#define R_80 RADIAN(80)
#define R_87 RADIAN(87)
#define R_89 RADIAN(89)
#define R_90 RADIAN(90)
#define R_100 RADIAN(100)
#define R_135 RADIAN(135)
#define R_140 RADIAN(140)
#define R_180 RADIAN(180)
#define R_240 RADIAN(240)
#define R_355 RADIAN(355)
#define R_356 RADIAN(356)
#define R_357 RADIAN(357)
#define R_358 RADIAN(358)
#define R_359 RADIAN(359)
#define R_360 RADIAN(360)

typedef float Radian;

#define PI_OVER_TWO_HI 1.5707960f
#define PI_OVER_TWO_LO 3.1391647e-7f

#define IN_RANGE(a,v,b)     ( ((a) <= (v)) && ((v) <= (b)) )

template< class T > inline T x_sign( T a )              { if (a < 0) return -1 ; else if (a > 0) return 1 ; else return 0 ; }


inline void sincos(Radian Angle, float& Sine, float& Cosine)
{
    float Result1 = Angle * (2.0f / PI);
    int   Quadrant = (int)((Result1 > 0) ? Result1 + 0.5f : Result1 - 0.5f);
    Angle = (Angle - (Quadrant * PI_OVER_TWO_HI)) - (Quadrant * PI_OVER_TWO_LO);
    float Squared = Angle * Angle;

    Result1 = (1 / 1.0f - Squared *
                              (1 / 2.0f - Squared *
                                              (1 / 24.0f - Squared *
                                                               (1 / 720.0f - Squared *
                                                                                 (1 / 40320.0f)))));

    float Result2 = Angle *
                    (1 / 1.0f + Squared *
                                    (-1 / 6.0f + Squared *
                                                     (1 / 120.0f + Squared *
                                                                       (-1 / 5040.0f + Squared *
                                                                                           (1 / 362880.0f)))));

    if (Quadrant & 0x01) {
        Sine = ((Quadrant + 0) & 0x02) ? -Result1 : Result1;
        Cosine = ((Quadrant + 1) & 0x02) ? -Result2 : Result2;
    } else {
        Sine = ((Quadrant + 1) & 0x02) ? -Result2 : Result2;
        Cosine = ((Quadrant + 0) & 0x02) ? -Result1 : Result1;
    }
}

inline bool isvalid(float a)
{
    // Check if the float is NaN (Not a Number)
    union
    {
        float    f;
        uint32_t i;
    } u;
    u.f = a;

    // Check if the exponent is not all ones (which indicates NaN)
    return (((u.i) & 0x7F800000) != 0x7F800000);
}

inline Radian x_ModAngle(Radian a)
{
    if ((a > RADIAN(1440)) ||
        (a < RADIAN(-1440))) {
        a = fmodf(a, R_360);
    }

    while (a >= R_360) {
        a -= R_360;
    }
    while (a < R_0) {
        a += R_360;
    }

    return a;
}

inline Radian x_ModAngle2(Radian a)
{
    a += R_180;
    a = x_ModAngle(a);
    a -= R_180;

    return (a);
}

inline Radian x_MinAngleDiff(Radian a, Radian b)
{
    return (x_ModAngle2(a - b));
}

inline float x_sqr ( float a )  { return( a * a ); }

inline
float x_parametric( float V, float ValueAtT0, float ValueAtT1, bool bClamp )
{
    assert( ValueAtT0 != ValueAtT1 );
    V = (V - ValueAtT0) / (ValueAtT1 - ValueAtT0);
    if( bClamp ) V = std::clamp( V, 0.0f, 1.0f );
    return V;
}
