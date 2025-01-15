#pragma once

#include <limits>
#include "Colour.h"

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

    const Vector3& operator=(const Vector3p& v3p){
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
