#pragma once

#include "CommonMath.h"

struct Radian3
{
    Radian3()
        : pitch(0.0f)
        , yaw(0.0f)
        , roll(0.0f)
    {
    }
    Radian3(Radian p, Radian y, Radian r)
        : pitch(p)
        , yaw(y)
        , roll(r)
    {
    }

    void Set(Radian p, Radian y, Radian r)
    {
        pitch = p;
        yaw = y;
        roll = r;
    }

    void Zero()
    {
        pitch = 0.0f;
        yaw = 0.0f;
        roll = 0.0f;
    }

    Radian pitch, yaw, roll;
};
