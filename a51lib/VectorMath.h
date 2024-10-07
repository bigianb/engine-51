#pragma once

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
};

struct Quaternion
{
    float x, y, z, w;
};
