#pragma once

#include "CommonMath.h"
#include "Radian3.h"

#include <algorithm>

struct Vector2
{
    Vector2() {}
    Vector2(float xx, float yy)
        : x(xx)
        , y(yy)
    {
    }

    void set(float xx, float yy)
    {
        x = xx;
        y = yy;
    }

    float x, y;
};

inline Vector2 operator*(float S, const Vector2& V)
{
    return (Vector2(V.x * S, V.y * S));
}

inline Vector2 operator*(const Vector2& V, float S)
{
    return (Vector2(V.x * S, V.y * S));
}

struct Vector3;

// Does not have a w component.
struct Vector3p
{
    Vector3p() {}
    Vector3p(const Vector3& v);
    float x, y, z;
};

struct Vector3
{
    union
    {
        float cells[4];
        struct
        {
            float x, y, z, w;
        };
    };

    Vector3()
        : x(0.0f)
        , y(0.0f)
        , z(0.0f)
        , w(0.0f)
    {
    }

    Vector3(float xx, float yy, float zz)
        : x(xx)
        , y(yy)
        , z(zz)
        , w(0.0f)
    {
    }

    Vector3(const Vector3p v)
    {
        x = v.x;
        y = v.y;
        z = v.z;
        w = 0.0f;
    }

    Vector3(Radian Pitch, Radian Yaw)
    {
        set(Pitch, Yaw);
    }

    Vector3(const Radian3& R)
    {
        set(R);
    }

    float  operator[](int Index) const { return cells[Index]; }
    float& operator[](int Index) { return cells[Index]; }

    void Zero()
    {
        x = 0.0f;
        y = 0.0f;
        z = 0.0f;
    }

    void Negate()
    {
        x = -x;
        y = -y;
        z = -z;
    }

    void Min(const float f)
    {
        x = std::min(GetX(), f);
        y = std::min(GetY(), f);
        z = std::min(GetZ(), f);
    }

    void Min(const Vector3& V)
    {
        x = std::min(GetX(), V.GetX());
        y = std::min(GetY(), V.GetY());
        z = std::min(GetZ(), V.GetZ());
    }

    void Max(const float f)
    {
        x = std::max(GetX(), f);
        y = std::max(GetY(), f);
        z = std::max(GetZ(), f);
    }

    void Max(const Vector3& V)
    {
        x = std::max(GetX(), V.GetX());
        y = std::max(GetY(), V.GetY());
        z = std::max(GetZ(), V.GetZ());
    }

    void set(float xx, float yy, float zz)
    {
        x = xx;
        y = yy;
        z = zz;
        w = 0.0f;
    }

    void set(Radian Pitch, Radian Yaw)
    {
        // If you take the vector (0,0,1) and pass it through a rotation matrix
        // created from pitch and yaw, this would be the resulting vector.
        // By knowing that the x and y components of the vector are 0, we have
        // removed lots of extra operations.
        float PS, PC;
        float YS, YC;

        sincos(Pitch, PS, PC);
        sincos(Yaw, YS, YC);

        set((YS * PC), -(PS), (YC * PC));
    }

    void set(const Radian3& R)
    {
        // Rotations are applied in the order roll, pitch, and yaw, and setting
        // this vector with a radian3 has the effect of rotating the vector (0,0,1)
        // by R. Which means the roll is useless.
        set(R.pitch, R.yaw);
    }

    const Vector3& operator=(const Vector3p& v3p)
    {
        x = v3p.x;
        y = v3p.y;
        z = v3p.z;
        w = 1.0;
        return *this;
    }

    float GetX() const { return x; }
    float GetY() const { return y; }
    float GetZ() const { return z; }

    void Normalize()
    {
        float LengthSquared = GetX() * GetX() + GetY() * GetY() + GetZ() * GetZ();
        if (LengthSquared > 0.0000001f) {
            float N = 1.0 / sqrt(LengthSquared);

            x *= N;
            y *= N;
            z *= N;
        }
    }

    void NormalizeAndScale(float Scalar)
    {
        float LengthSquared = GetX() * GetX() + GetY() * GetY() + GetZ() * GetZ();
        if (LengthSquared > 0.0000001f) {
            float N = Scalar / sqrt(LengthSquared);

            x *= N;
            y *= N;
            z *= N;
        }
    }

    bool SafeNormalize()
    {
        float N = 1.0 / sqrt(x * x + y * y + z * z);

        if (isvalid(N)) {
            x *= N;
            y *= N;
            z *= N;
            return true;
        } else {
            x = 0.0f;
            y = 0.0f;
            z = 0.0f;
            return false;
        }
    }

    bool IsValid() const
    {
        return (isvalid(GetX()) && isvalid(GetY()) && isvalid(GetZ()));
    }

    float LengthSquared() const
    {
        return (GetX() * GetX() + GetY() * GetY() + GetZ() * GetZ());
    }

    void Scale(float scalar)
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
    }

    float Length() const
    {
        return sqrt(x * x + y * y + z * z);
    }

    void Rotate(const Radian3& R);
    void RotateX(Radian Rx);
    void RotateY(Radian Ry);
    void RotateZ(Radian Rz);

    float Dot(const Vector3& V) const
    {
        return ((GetX() * V.GetX()) + (GetY() * V.GetY()) + (GetZ() * V.GetZ()));
    }

    Vector3 Cross(const Vector3& V) const
    {
        return (Vector3((GetY() * V.GetZ()) - (GetZ() * V.GetY()),
                        (GetZ() * V.GetX()) - (GetX() * V.GetZ()),
                        (GetX() * V.GetY()) - (GetY() * V.GetX())));
    }

    friend Vector3 v3_Cross(const Vector3& V1, const Vector3& V2);

    Radian GetPitch() const
    {
        float L = sqrt(GetX() * GetX() + GetZ() * GetZ());
        return (-atan2(GetY(), L));
    }
    Radian GetYaw() const { return (atan2(GetX(), GetZ())); }
    void   GetPitchYaw(Radian& Pitch, Radian& Yaw) const
    {
        Pitch = GetPitch();
        Yaw = GetYaw();
    }

    float   GetSqrtDistToLineSeg(const Vector3& Start, const Vector3& End) const;
    Vector3 GetClosestPToLSeg(const Vector3& Start, const Vector3& End) const;
    Vector3 GetClosestVToLSeg(const Vector3& Start, const Vector3& End) const;
    float   GetClosestPToLSegRatio(const Vector3& Start, const Vector3& End) const;
    float   ClosestPointToRectangle(const Vector3& P0, // Origin from the edges.
                                    const Vector3& E0,
                                    const Vector3& E1,
                                    Vector3&       OutClosestPoint) const;

    void GetRotationTowards(const Vector3& DestV,
                            Vector3&       RotAxis,
                            Radian&        RotAngle) const;

    Vector3  operator-(void) const { return (Vector3(-GetX(), -GetY(), -GetZ())); }
    Vector3& operator+=(const Vector3& V)
    {
        x += V.x;
        y += V.y;
        z += V.z;
        return *this;
    }
    Vector3& operator-=(const Vector3& V)
    {
        x -= V.x;
        y -= V.y;
        z -= V.z;
        return *this;
    }
    Vector3& operator*=(float scalar)
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }
    Vector3& operator/=(float scalar)
    {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }

    bool           operator==(const Vector3& V) const { return x == V.x && y == V.y && z == V.z; }
    bool           operator!=(const Vector3& V) const { return x != V.x || y != V.y || z != V.z; }
    friend Vector3 operator/(const Vector3& V, float S);
    friend Vector3 operator*(const Vector3& V1, const Vector3& V2);
};

inline Vector3p::Vector3p(const Vector3& v)
{
    x = v.x;
    y = v.y;
    z = v.z;
}

inline Vector3 operator*(const Vector3& V1, const Vector3& V2)
{
    return (Vector3(V1.GetX() * V2.GetX(),
                    V1.GetY() * V2.GetY(),
                    V1.GetZ() * V2.GetZ()));
}

inline Vector3 operator+(const Vector3& V1, const Vector3& V2)
{
    return (Vector3(V1.GetX() + V2.GetX(),
                    V1.GetY() + V2.GetY(),
                    V1.GetZ() + V2.GetZ()));
}

inline Vector3 operator-(const Vector3& V1, const Vector3& V2)
{
    return (Vector3(V1.GetX() - V2.GetX(),
                    V1.GetY() - V2.GetY(),
                    V1.GetZ() - V2.GetZ()));
}

inline Vector3 operator*(float S, const Vector3& V)
{
    return (Vector3(V.GetX() * S,
                    V.GetY() * S,
                    V.GetZ() * S));
}

inline Vector3 operator*(const Vector3& V, float S)
{
    return (Vector3(V.GetX() * S,
                    V.GetY() * S,
                    V.GetZ() * S));
}

inline Vector3 operator/(const Vector3& V, float S)
{
    return (Vector3(V.GetX() / S,
                    V.GetY() / S,
                    V.GetZ() / S));
}

inline void Vector3::RotateX(Radian Angle)
{
    Radian S, C;
    sincos(Angle, S, C);

    float y0 = y;
    float z0 = z;
    y = (C * y0) - (S * z0);
    z = (C * z0) + (S * y0);
}

inline void Vector3::RotateY(Radian Angle)
{
    Radian S, C;
    sincos(Angle, S, C);

    float x0 = x;
    float z0 = z;
    x = (C * x0) + (S * z0);
    z = (C * z0) - (S * x0);
}

inline void Vector3::RotateZ(Radian Angle)
{
    Radian S, C;
    sincos(Angle, S, C);

    float x0 = x;
    float y0 = y;
    x = (C * x0) - (S * y0);
    y = (C * y0) + (S * x0);
}

//==============================================================================

inline void Vector3::Rotate(const Radian3& R)
{
    float sx, cx;
    float sy, cy;
    float sz, cz;

    sincos(R.pitch, sx, cx);
    sincos(R.yaw, sy, cy);
    sincos(R.roll, sz, cz);

    // fill out 3x3 rotations...this would give you the columns of a rotation
    // matrix that is roll, pitch, then yaw
    float sxsz = sx * sz;
    float sxcz = sx * cz;

    Vector3 Mat0(cy * cz + sy * sxsz, cx * sz, cy * sxsz - sy * cz);
    Vector3 Mat1(sy * sxcz - sz * cy, cx * cz, sy * sz + sxcz * cy);
    Vector3 Mat2(cx * sy, -sx, cx * cy);

    float x0 = x;
    float y0 = y;
    float z0 = z;

    x = Mat0.x * x0 + Mat1.x * y0 + Mat2.x * z0;
    y = Mat0.y * x0 + Mat1.y * y0 + Mat2.y * z0;
    z = Mat0.z * x0 + Mat1.z * y0 + Mat2.z * z0;
}

//==============================================================================
//           * Start
//           |
//           <--------------(* this )
//           | Return Vector
//           |
//           |
//           * End
//
// Such:
//
// this.GetClosestVToLSeg(a,b).LengthSquared(); // Is the length square to the line segment
// this.GetClosestVToLSeg(a,b) + this;          // Is the closest point in to the line segment
// this.GetClosestPToLSegRatio(a,b)             // Is the ratio (0=a to 1=b) of the closest
//                                                 point in line segment.
//
//==============================================================================
inline Vector3 Vector3::GetClosestVToLSeg(const Vector3& Start, const Vector3& End) const
{
    Vector3 Diff = *this - Start;
    Vector3 Dir = End - Start;
    float   T = Diff.Dot(Dir);

    if (T > 0.0f) {
        float SqrLen = Dir.Dot(Dir);

        if (T >= SqrLen) {
            Diff -= Dir;
        } else {
            T /= SqrLen;
            Diff -= T * Dir;
        }
    }

    return -Diff;
}

//==============================================================================

inline float Vector3::GetSqrtDistToLineSeg(const Vector3& Start, const Vector3& End) const
{
    return GetClosestVToLSeg(Start, End).LengthSquared();
}

//==============================================================================

inline Vector3 Vector3::GetClosestPToLSeg(const Vector3& Start, const Vector3& End) const
{
    return GetClosestVToLSeg(Start, End) + *this;
}

//==============================================================================

// Given the line defined by the points A and B, returns the ratio (0->1) of the
// closest point on the line to P.
// 0 = A
// 1 = B
// >0 and <1 means somewhere inbetween A and B
inline float Vector3::GetClosestPToLSegRatio(const Vector3& A, const Vector3& B) const
{

    // Get direction vector and length squared of line
    Vector3 AB = B - A;
    float   L = AB.LengthSquared();

    // Is line a point?
    if (L == 0.0f) {
        return 0.0f;
    }

    // Get vector from start of line to point
    Vector3 AP = (*this) - A;

    // Calculate: Cos(theta)*|AP|*|AB|
    float Dot = AP.Dot(AB);

    // Before start?
    if (Dot <= 0) {
        return 0.0f;
    }

    // After end?
    if (Dot >= L) {
        return 1.0f;
    }

    // Let distance along AB = X = (|AP| dot |AB|) / |AB|
    // Ratio T is X / |AB| = (|AP| dot |AB|) / (|AB| * |AB|)
    float T = Dot / L;

    // Slerp between A and B
    return T;
}

inline float v3_Dot(const Vector3& V1, const Vector3& V2)
{
    return (V1.x * V2.x) + (V1.y * V2.y) + (V1.z * V2.z);
}

inline Vector3 v3_Cross(const Vector3& V1, const Vector3& V2)
{
    return (Vector3((V1.GetY() * V2.GetZ()) - (V1.GetZ() * V2.GetY()),
                    (V1.GetZ() * V2.GetX()) - (V1.GetX() * V2.GetZ()),
                    (V1.GetX() * V2.GetY()) - (V1.GetY() * V2.GetX())));
}

inline Radian v3_AngleBetween(const Vector3& V1, const Vector3& V2)
{
    float D = V1.Length() * V2.Length();

    if (D == 0.0f) {
        return (R_0);
    }

    float Cos = v3_Dot(V1, V2) / D;

    if (Cos > 1.0f) {
        Cos = 1.0f;
    } else if (Cos < -1.0f) {
        Cos = -1.0f;
    }

    return (acos(Cos));
}

void    x_ClosestPtsOnLineSegs   ( const Vector3& StartA, const Vector3& EndA,    // IN
                                   const Vector3& StartB, const Vector3& EndB,    // IN
                                         Vector3& PtOnA,                          // OUT
                                         Vector3& PtOnB                       );  // OUT

void    x_ClosestPtsOnLineSegs   ( const Vector3& StartA, const Vector3& EndA,    // IN
                                   const Vector3& StartB, const Vector3& EndB,    // IN
                                         Vector3& PtOnA,                          // OUT
                                         Vector3& PtOnB,                          // OUT
                                         float&     TA,                             // OUT
                                         float&     TB                          );  // OUT
