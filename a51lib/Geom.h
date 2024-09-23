#pragma once

struct Vector3
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

class InevFile;

struct Bone
{
    enum HitLocation
    {
        HIT_LOCATION_START,

        HIT_LOCATION_HEAD = HIT_LOCATION_START,
        HIT_LOCATION_SHOULDER_LEFT,
        HIT_LOCATION_SHOULDER_RIGHT,
        HIT_LOCATION_TORSO,
        HIT_LOCATION_LEGS,

        HIT_LOCATION_COUNT,

        HIT_LOCATION_UNKNOWN,
        HIT_LOCATION_UNKNOWN_WRONG_GUID,
    };

    // Local space
    Quaternion bindRotation;
    Vector3 bindPosition;
    BBox bbox;

    HitLocation hitLocation; // serialised as a short
    short rigidBodyIdx;      // Index of rigid body to attach to (or -1 if none)

    void read(InevFile&);

};
