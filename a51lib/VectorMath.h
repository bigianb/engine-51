#pragma once

#include <limits>

struct Colour
{
    uint8_t b, g, r, a;
};

struct Vector2
{
    float x, y;
};

struct Vector3
{
    float x, y, z, w;
};

// Does not have a w component.
struct Vector3p
{
    float x, y, z;
};

struct Vector4
{
    float x, y, z, w;
};

struct BBox
{
    Vector3 min, max;

    void reset(){
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
};
