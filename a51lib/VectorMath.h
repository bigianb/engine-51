#pragma once

#include "math/CommonMath.h"
#include "math/Vector3.h"
#include "math/Quaternion.h"
#include "math/Radian3.h"
#include "math/Matrix4.h"
#include "math/Plane.h"
#include "math/BoundingBox.h"

#include <limits>
#include <algorithm>
#include <cmath>
#include "Colour.h"

class IntRect
{
public:
    int left;
    int top;
    int right;
    int bottom;

    IntRect()
        : left(0)
        , right(0)
        , top(0)
        , bottom(0)
    {
    }
    IntRect(int l, int t, int r, int b)
        : left(l)
        , right(r)
        , top(t)
        , bottom(b)
    {
    }
    IntRect(const IntRect& src)
    {
        left = src.left;
        top = src.top;
        right = src.right;
        bottom = src.bottom;
    }

    int getWidth() const
    {
        return right - left;
    }

    int getHeight() const
    {
        return bottom - top;
    }

    void translate(int dx, int dy)
    {
        left += dx;
        right += dx;
        top += dy;
        bottom += dy;
    }

    void deflate(int dx, int dy)
    {
        left += dx;
        right -= dx;
        top += dy;
        bottom -= dy;
    }

    void set(int l, int t, int r, int b)
    {
        left = l;
        top = t;
        right = r;
        bottom = b;
    }

    void setHeight(int h)
    {
        bottom = top + h;
    }
};

struct rect
{
    Vector2 Min;
    Vector2 Max;

    rect(void);
    rect(const rect& Rect);
    rect(const Vector2& Min, const Vector2& Max);
    rect(float l, float t, float r, float b);

    void Clear(void);
    void Set(const Vector2& Min, const Vector2& Max);
    void Set(float l, float t, float r, float b);

    bool Intersect(const rect& Rect);
    bool Intersect(rect& R, const rect& Rect);

    void AddPoint(const Vector2& Point);
    void AddRect(const rect& Rect);

    float   GetWidth(void) const;
    float   GetHeight(void) const;
    Vector2 GetSize(void) const;
    Vector2 GetCenter(void) const;

    void SetWidth(float W);
    void SetHeight(float H);
    void SetSize(const Vector2& S);

    void Translate(const Vector2& T);
    void Inflate(const Vector2& T);
    void Deflate(const Vector2& T);

    float Difference(const rect& R) const;
    bool  InRange(float Min, float Max) const;
    bool  IsValid(void) const;
};


inline Quaternion Matrix4::GetQuaternion() const
{
    float      T;
    float      X2, Y2, Z2, W2; // squared magniudes of quaternion components
    int        i;
    Quaternion Q;

    // remove scale from matrix
    Matrix4 O = *this;
    O.ClearScale();

    // first compute squared magnitudes of quaternion components - at least one
    // will be greater than 0 since quaternion is unit magnitude
    W2 = 0.25f * (O(0, 0) + O(1, 1) + O(2, 2) + 1.0f);
    X2 = W2 - 0.5f * (O(1, 1) + O(2, 2));
    Y2 = W2 - 0.5f * (O(2, 2) + O(0, 0));
    Z2 = W2 - 0.5f * (O(0, 0) + O(1, 1));

    // find maximum magnitude component
    i = (W2 > X2) ? ((W2 > Y2) ? ((W2 > Z2) ? 0 : 3) : ((Y2 > Z2) ? 2 : 3)) : ((X2 > Y2) ? ((X2 > Z2) ? 1 : 3) : ((Y2 > Z2) ? 2 : 3));

    // compute signed quaternion components using numerically stable method
    switch (i) {
    case 0:
        Q.w = sqrt(W2);
        T = 0.25f / Q.w;
        Q.x = (O(1, 2) - O(2, 1)) * T;
        Q.y = (O(2, 0) - O(0, 2)) * T;
        Q.z = (O(0, 1) - O(1, 0)) * T;
        break;
    case 1:
        Q.x = sqrt(X2);
        T = 0.25f / Q.x;
        Q.w = (O(1, 2) - O(2, 1)) * T;
        Q.y = (O(1, 0) + O(0, 1)) * T;
        Q.z = (O(2, 0) + O(0, 2)) * T;
        break;
    case 2:
        Q.y = sqrt(Y2);
        T = 0.25f / Q.y;
        Q.w = (O(2, 0) - O(0, 2)) * T;
        Q.z = (O(2, 1) + O(1, 2)) * T;
        Q.x = (O(0, 1) + O(1, 0)) * T;
        break;
    case 3:
        Q.z = sqrt(Z2);
        T = 0.25f / Q.z;
        Q.w = (O(0, 1) - O(1, 0)) * T;
        Q.x = (O(0, 2) + O(2, 0)) * T;
        Q.y = (O(1, 2) + O(2, 1)) * T;
        break;
    }

    Q.Normalize();
    return Q;
}

// -----------------------------------------------
// Quaternion inlines

inline Quaternion Blend(const Quaternion& Q0,
                        const Quaternion& Q1, float T)
{
    float Dot;
    float LenSquared;
    float OneOverL;
    float x0, y0, z0, w0;

    // Determine if quats are further than 90 degrees
    Dot = Q0.x * Q1.x + Q0.y * Q1.y + Q0.z * Q1.z + Q0.w * Q1.w;

    // If dot is negative flip one of the quaterions
    if (Dot < 0.0f) {
        x0 = -Q0.x;
        y0 = -Q0.y;
        z0 = -Q0.z;
        w0 = -Q0.w;
    } else {
        x0 = +Q0.x;
        y0 = +Q0.y;
        z0 = +Q0.z;
        w0 = +Q0.w;
    }

    // Compute interpolated values
    x0 = x0 + T * (Q1.x - x0);
    y0 = y0 + T * (Q1.y - y0);
    z0 = z0 + T * (Q1.z - z0);
    w0 = w0 + T * (Q1.w - w0);

    // Get squared length of new Quaternion
    LenSquared = x0 * x0 + y0 * y0 + z0 * z0 + w0 * w0;

    // Use home-baked polynomial to compute 1/sqrt(LenSquared)
    // Input range is 0.5 <-> 1.0
    // Ouput range is 1.414213 <-> 1.0

    if (LenSquared < 0.857f) {
        OneOverL = (((0.699368f) * LenSquared) + -1.819985f) * LenSquared + 2.126369f; //0.0000792
    } else {
        OneOverL = (((0.454012f) * LenSquared) + -1.403517f) * LenSquared + 1.949542f; //0.0000373
    }

    // Renormalize and return Quaternion
    return Quaternion(x0 * OneOverL, y0 * OneOverL, z0 * OneOverL, w0 * OneOverL);
}

inline void Quaternion::RotateX(Radian Rx)
{
    float s, c;
    sincos(Rx / 2, s, c);
    Quaternion Q(s, 0, 0, c);

    *this = Q * *this;
}

//==============================================================================

inline void Quaternion::RotateY(Radian Ry)
{
    float s, c;
    sincos(Ry / 2, s, c);
    Quaternion Q(0, s, 0, c);

    *this = Q * *this;
}

//==============================================================================

inline void Quaternion::RotateZ(Radian Rz)
{
    float s, c;
    sincos(Rz / 2, s, c);
    Quaternion Q(0, 0, s, c);

    *this = Q * *this;
}

inline void Quaternion::Setup(const Radian3& R)
{
    identity();
    RotateZ(R.roll);
    RotateX(R.pitch);
    RotateY(R.yaw);
}

inline void Quaternion::Setup(const Vector3& Axis, Radian Angle)
{
    float Sine, Cosine;

    sincos(Angle * 0.5f, Sine, Cosine);

    w = Cosine;             // cos( Angle/2 )
    x = Sine * Axis.GetX(); // sin( Angle/2 ) * X
    y = Sine * Axis.GetY(); // sin( Angle/2 ) * Y
    z = Sine * Axis.GetZ(); // sin( Angle/2 ) * Z
}

inline void Quaternion::Setup(const Matrix4& M)
{
    *this = M.GetQuaternion();
}

inline void Quaternion::Setup(const Vector3& StartDir, const Vector3& EndDir)
{
    Vector3 Axis;
    Radian  Angle;

    StartDir.GetRotationTowards(EndDir, Axis, Angle);
    Setup(Axis, Angle);
}

inline Quaternion operator*(const Quaternion& Qa,
                            const Quaternion& Qb);

inline void Vector3::GetRotationTowards(const Vector3& DestV,
                                        Vector3&       RotAxis,
                                        Radian&        RotAngle) const
{
    // Get lengths of vectors
    float D = this->Length() * DestV.Length();

    // vectors are zero length
    if (D == 0.0f) {
        RotAxis = Vector3(1, 0, 0);
        RotAngle = 0;
        return;
    }

    // Get unit dot product of the vectors
    float Dot = this->Dot(DestV) / D;

    if (Dot > 1.0f) {
        Dot = 1.0f;
    } else if (Dot < -1.0f) {
        Dot = -1.0f;
    }

    // Get axis to rotate about
    RotAxis = v3_Cross(*this, DestV);

    if (RotAxis.SafeNormalize()) {
        RotAngle = acos(Dot);
    } else {
        RotAxis = Vector3(1, 0, 0);

        if (Dot > 0) {
            RotAngle = 0; // Facing same direction
        } else {
            RotAngle = R_180; // Facing opposite directions
        }
    }
}

inline Vector3 Quaternion::operator*(const Vector3& V) const
{
    Vector3 Result;
    Vector3 v1;
    Vector3 v2;

    Result.set(x, y, z);
    v1 = v3_Cross(Result, V);
    v2 = v3_Cross(Result, v1);
    v1 *= 2.0f * w;
    v2 *= 2.0f;
    Result = V + v1 + v2;

    return Result;
}

inline Quaternion operator*(const Quaternion& L,
                            const Quaternion& R)
{
    Quaternion Q;

    Q.x = (L.w * R.x) + (R.w * L.x) + (L.y * R.z) - (L.z * R.y);
    Q.y = (L.w * R.y) + (R.w * L.y) + (L.z * R.x) - (L.x * R.z);
    Q.z = (L.w * R.z) + (R.w * L.z) + (L.x * R.y) - (L.y * R.x);

    Q.w = (L.w * R.w) - (R.x * L.x) - (L.y * R.y) - (L.z * R.z);

    return Q;
}

inline Quaternion BlendToIdentity(const Quaternion& Q0, float T)
{
    float LenSquared;
    float OneOverL;
    float x0, y0, z0, w0;

    // If dot is negative flip one of the quaterions
    if (Q0.w < 0.0f) {
        x0 = -Q0.x;
        y0 = -Q0.y;
        z0 = -Q0.z;
        w0 = -Q0.w;
    } else {
        x0 = +Q0.x;
        y0 = +Q0.y;
        z0 = +Q0.z;
        w0 = +Q0.w;
    }

    // Compute interpolated values
    x0 = x0 + T * (0.0f - x0);
    y0 = y0 + T * (0.0f - y0);
    z0 = z0 + T * (0.0f - z0);
    w0 = w0 + T * (1.0f - w0);

    // Get squared length of new quaternion
    LenSquared = x0 * x0 + y0 * y0 + z0 * z0 + w0 * w0;

    // Use home-baked polynomial to compute 1/sqrt(LenSquared)
    // Input range is 0.5 <-> 1.0
    // Ouput range is 1.414213 <-> 1.0

    if (LenSquared < 0.857f) {
        OneOverL = (((0.699368f) * LenSquared) + -1.819985f) * LenSquared + 2.126369f; //0.0000792
    } else {
        OneOverL = (((0.454012f) * LenSquared) + -1.403517f) * LenSquared + 1.949542f; //0.0000373
    }

    // Renormalize and return quaternion
    return Quaternion(x0 * OneOverL, y0 * OneOverL, z0 * OneOverL, w0 * OneOverL);
}
