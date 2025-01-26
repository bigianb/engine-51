#pragma once

#include <limits>
#include "Colour.h"

#define RADIAN(A)     ((float)((A) * 0.0174532925199432957692369055556f))
#define DEG_TO_RAD(A) ((float)((A) * 0.0174532925199432957692369055556f))
#define RAD_TO_DEG(A) ((float)((A) * 57.295779513082320876798161804285f))

#define R_0     RADIAN(   0 )
#define R_1     RADIAN(   1 )
#define R_2     RADIAN(   2 )
#define R_3     RADIAN(   3 )
#define R_4     RADIAN(   4 )
#define R_5     RADIAN(   5 )

#define R_355   RADIAN( 355 )
#define R_356   RADIAN( 356 )
#define R_357   RADIAN( 357 )
#define R_358   RADIAN( 358 )
#define R_359   RADIAN( 359 )
#define R_360   RADIAN( 360 )

struct Vector2
{
    float x, y;
};

// Does not have a w component.
struct Vector3p
{
    float x, y, z;
};

struct Vector3
{
    float x, y, z, w;

    void set(float xx, float yy, float zz)
    {
        x = xx;
        y = yy;
        z = zz;
        w = 0.0f;
    }

    const Vector3& operator=(const Vector3p& v3p)
    {
        x = v3p.x;
        y = v3p.y;
        z = v3p.z;
        w = 1.0;
        return *this;
    }
};

struct Vector4
{
    float x, y, z, w;
};

struct BBox
{
    Vector3 min, max;

    void reset()
    {
        min.x = std::numeric_limits<float>::max();
        min.y = std::numeric_limits<float>::max();
        min.z = std::numeric_limits<float>::max();

        max.x = std::numeric_limits<float>::min();
        max.y = std::numeric_limits<float>::min();
        max.z = std::numeric_limits<float>::min();
    }
};

struct Radian3
{
    float pitch, yaw, roll;
};

struct Quaternion
{
    float x, y, z, w;

    void identity()
    {
        x = 0.0;
        y = 0.0;
        z = 0.0;
        w = 1.0;
    }
};

class Matrix4
{
public:
    union
    {
        float cells[4][4];

        struct
        {
            Vector4 vCol0;
            Vector4 vCol1;
            Vector4 vCol2;
            Vector4 vCol3;
        };
    };
};
