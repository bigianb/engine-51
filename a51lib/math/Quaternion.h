#pragma once

#include "CommonMath.h"

#include "Vector3.h"
#include "Radian3.h"

class Matrix4;

struct Quaternion
{
public:
    float x, y, z, w;

    Quaternion() { identity(); }
    Quaternion(float xx, float yy, float zz, float ww)
        : x(xx)
        , y(yy)
        , z(zz)
        , w(ww)
    {
    }

    Quaternion(const Vector3& Axis, Radian Angle)
    {
        Setup(Axis, Angle);
    }

    Quaternion(const Radian3 rot)
    {
        Setup(rot);
    }

    void identity()
    {
        x = 0.0;
        y = 0.0;
        z = 0.0;
        w = 1.0;
    }

    void Invert()
    {
        x = -x;
        y = -y;
        z = -z;
    }

    void Normalize()
    {
        float S = 1.0 / sqrt(x * x + y * y + z * z + w * w);

        x *= S;
        y *= S;
        z *= S;
        w *= S;
    }

    void Setup(const Radian3& R);
    void Setup(const Vector3& Axis, Radian Angle);
    void Setup(const Matrix4& M);
    void Setup(const Vector3& StartDir, const Vector3& EndDir);

    Vector3 GetAxis() const;
    Radian  GetAngle() const;
    Radian3 GetRotation() const;

    void RotateX(Radian Rx);
    void RotateY(Radian Ry);
    void RotateZ(Radian Rz);

    Vector3 Rotate(const Vector3& V) const
    {
        return (*this * V);
    }

    Vector3 operator*(const Vector3& V) const;

    friend Quaternion operator*(const Quaternion& Qa,
                                const Quaternion& Qb);

    friend Quaternion BlendToIdentity(const Quaternion& Q1, float T);
};

inline Vector3 Quaternion::GetAxis() const
{
    Vector3 Axis;

    float Cosine = w;

    if (Cosine > 1.0f) {
        Cosine = 1.0f;
    }
    if (Cosine < -1.0f) {
        Cosine = -1.0f;
    }

    float Sine = sin(acos(Cosine));

    if ((Sine > -0.00001f) && (Sine < 0.00001f)) {
        Axis.x = 0.0f;
        Axis.y = 0.0f;
        Axis.z = 0.0f;
    } else {
        Sine = 1.0f / Sine;

        Axis.x = x * Sine;
        Axis.y = y * Sine;
        Axis.z = z * Sine;
    }

    return Axis;
}

inline Radian Quaternion::GetAngle() const
{
    float Cosine = w; // cos( Angle/2 )

    if (Cosine > 1.0f) {
        Cosine = 1.0f;
    }
    if (Cosine < -1.0f) {
        Cosine = -1.0f;
    }

    return (acos(Cosine) * 2.0f);
}

inline Radian3 Quaternion::GetRotation() const
{
    float tx = 2.0f * x; // 2x
    float ty = 2.0f * y; // 2y
    float tz = 2.0f * z; // 2z
    float txw = tx * w;  // 2x * w
    float tyw = ty * w;  // 2y * w
    float tzw = tz * w;  // 2z * w
    float txx = tx * x;  // 2x * x
    float tyx = ty * x;  // 2y * x
    float tzx = tz * x;  // 2z * x
    float tyy = ty * y;  // 2y * y
    float tzy = tz * y;  // 2z * y
    float tzz = tz * z;  // 2z * z

    Radian Pitch, Yaw, Roll;
    float  s;

    // Get pitch (Rx).

    s = tzy - txw;
    if (s > 1.0f) {
        s = 1.0f;
    }
    if (s < -1.0f) {
        s = -1.0f;
    }
    Pitch = asin(-s);

    // Get yaw (Ry) and roll (Rz).

    if ((Pitch > -R_89) || (Pitch < R_89)) {
        Yaw = atan2(tzx + tyw, 1.0f - (txx + tyy));
        Roll = atan2(tyx + tzw, 1.0f - (txx + tzz));
    } else {
        Yaw = 0.0f;
        Roll = atan2(tzx - tyw, 1.0f - (tyy + tzz));
    }

    return (Radian3(Pitch, Yaw, Roll));
}

Quaternion BlendSlow(const Quaternion& Q0,
                     const Quaternion& Q1, float T);

Quaternion BlendToIdentitySlow(const Quaternion& Q1, float T);
