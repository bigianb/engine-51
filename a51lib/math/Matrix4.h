#pragma once

#include "Radian3.h"
#include "Quaternion.h"
#include "Vector3.h"

#define M(column, row) cells[column][row]

struct Vector4
{
    Vector4()
        : x(0.0f)
        , y(0.0f)
        , z(0.0f)
        , w(0.0f)
    {
    }

    Vector4(const Vector3& v3)
        : x(v3.x)
        , y(v3.y)
        , z(v3.z)
        , w(0.0f)
    {
    }

    Vector4(float xx, float yy, float zz, float ww)
        : x(xx)
        , y(yy)
        , z(zz)
        , w(ww)
    {
    }

    float Dot(const Vector4& v) const
    {
        return (x * v.x + y * v.y + z * v.z + w * v.w);
    }

    float x, y, z, w;
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

    Matrix4() {}
    Matrix4(const Radian3& r) { Setup(r); }
    Matrix4(const Quaternion& q) { Setup(q); }
    Matrix4(const Vector3&    Scale,
            const Quaternion& Rotation,
            const Vector3&    Translation)
    {
        Setup(Scale, Rotation, Translation);
    }

    float  operator()(int Column, int Row) const { return cells[Column][Row]; }
    float& operator()(int Column, int Row) { return cells[Column][Row]; }

    void Zero()
    {
        // clang-format off
        M(0,0) = M(1,0) = M(2,0) = M(3,0) = 0.0f;
        M(0,1) = M(1,1) = M(2,1) = M(3,1) = 0.0f;
        M(0,2) = M(1,2) = M(2,2) = M(3,2) = 0.0f;
        M(0,3) = M(1,3) = M(2,3) = M(3,3) = 0.0f;
        // clang-format on
    }

    bool IsValid() const
    {
        for (int r = 0; r < 4; r++) {
            for (int c = 0; c < 4; ++c) {
                if (!isvalid(cells[c][r])) {
                    return false;
                }
            }
        }
        return true;
    }

    Quaternion GetQuaternion() const;

    Vector3 GetTranslation() const { return Vector3(cells[3][0], cells[3][1], cells[3][2]); }
    void    SetTranslation(const Vector3& translation)
    {

        cells[3][0] = translation.x;
        cells[3][1] = translation.y;
        cells[3][2] = translation.z;
    }
    Radian3 GetRotation() const;
    void    ClearScale()
    {
        Vector3 S = GetScale();
        PreScale(Vector3(1 / S.x, 1 / S.y, 1 / S.z));
    }

    Vector3 GetScale() const
    {
        return (Vector3(std::sqrt(M(0, 0) * M(0, 0) + M(0, 1) * M(0, 1) + M(0, 2) * M(0, 2)),
                        std::sqrt(M(1, 0) * M(1, 0) + M(1, 1) * M(1, 1) + M(1, 2) * M(1, 2)),
                        std::sqrt(M(2, 0) * M(2, 0) + M(2, 1) * M(2, 1) + M(2, 2) * M(2, 2))));
    }

    void SetScale(float Scale)
    {
        M(0, 0) = Scale;
        M(1, 1) = Scale;
        M(2, 2) = Scale;
    }

    void SetScale(const Vector3& Scale)
    {
        M(0, 0) = Scale.x;
        M(1, 1) = Scale.y;
        M(2, 2) = Scale.z;
    }

    void ClearTranslation()
    {
        M(3, 0) = M(3, 1) = M(3, 2) = 0.0f;
    }

    void Identity()
    {
        M(1, 0) = M(2, 0) = M(3, 0) = 0.0f;
        M(0, 1) = M(2, 1) = M(3, 1) = 0.0f;
        M(0, 2) = M(1, 2) = M(3, 2) = 0.0f;
        M(0, 3) = M(1, 3) = M(2, 3) = 0.0f;

        M(0, 0) = 1.0f;
        M(1, 1) = 1.0f;
        M(2, 2) = 1.0f;
        M(3, 3) = 1.0f;
    }

    void SetColumns(const Vector3& V1,
                    const Vector3& V2,
                    const Vector3& V3)
    {
        // clang-format off
        M(0,0) = V1.x; M(1,0) = V2.x; M(2,0) = V3.x;
        M(0,1) = V1.y; M(1,1) = V2.y; M(2,1) = V3.y;
        M(0,2) = V1.z; M(1,2) = V2.z; M(2,2) = V3.z;
        // clang-format on
    }

    void Orthogonalize();
    bool Invert();
    bool InvertRT();

    bool InvertSRT();

    void Transpose()
    {
        float T;

        T = M(1, 0);
        M(1, 0) = M(0, 1);
        M(0, 1) = T;

        T = M(2, 0);
        M(2, 0) = M(0, 2);
        M(0, 2) = T;

        T = M(3, 0);
        M(3, 0) = M(0, 3);
        M(0, 3) = T;

        T = M(2, 1);
        M(2, 1) = M(1, 2);
        M(1, 2) = T;

        T = M(3, 1);
        M(3, 1) = M(1, 3);
        M(1, 3) = T;

        T = M(3, 2);
        M(3, 2) = M(2, 3);
        M(2, 3) = T;
    }

    void RotateX(Radian Rx)
    {
        if (Rx == 0) {
            return;
        }

        float s, c;
        sincos(Rx, s, c);

        // this is the slow method but easy to see what's going on:
        //matrix4 RM;
        //RM.Identity();
        //RM.M(1,1) =  c;
        //RM.M(2,1) = -s;
        //RM.M(1,2) =  s;
        //RM.M(2,2) =  c;
        //*this = RM * *this;

        // this is the fast method with less multiplies and adds:
        // clang-format off
        float M01 = M(0,1);   M(0,1) = c*M01 - s*M(0,2);  M(0,2) = s*M01 + c*M(0,2);
        float M11 = M(1,1);   M(1,1) = c*M11 - s*M(1,2);  M(1,2) = s*M11 + c*M(1,2);
        float M21 = M(2,1);   M(2,1) = c*M21 - s*M(2,2);  M(2,2) = s*M21 + c*M(2,2);
        float M31 = M(3,1);   M(3,1) = c*M31 - s*M(3,2);  M(3,2) = s*M31 + c*M(3,2);
        // clang-format on
    }

    void RotateY(Radian Ry)
    {
        if (Ry == 0) {
            return;
        }

        float s, c;
        sincos(Ry, s, c);

        // clang-format off
        float M00 = M(0, 0); M(0, 0) = c * M00 + s * M(0, 2); M(0, 2) = c * M(0, 2) - s * M00;
        float M10 = M(1, 0); M(1, 0) = c * M10 + s * M(1, 2); M(1, 2) = c * M(1, 2) - s * M10;
        float M20 = M(2, 0); M(2, 0) = c * M20 + s * M(2, 2); M(2, 2) = c * M(2, 2) - s * M20;
        float M30 = M(3, 0); M(3, 0) = c * M30 + s * M(3, 2); M(3, 2) = c * M(3, 2) - s * M30;
        // clang-format on
    }

    void RotateZ(Radian Rz)
    {
        if (Rz == 0) {
            return;
        }

        float s, c;
        sincos(Rz, s, c);

        // this is the slow method but easy to see what's going on:
        //matrix4 RM;
        //RM.Identity();
        //RM.M(0,0) =  c;
        //RM.M(1,0) = -s;
        //RM.M(0,1) =  s;
        //RM.M(1,1) =  c;
        //*this = RM * *this;

        // this is the fast method with less multiplies and adds:
        // clang-format off
        float M00 = M(0,0);   M(0,0) = c*M00 - s*M(0,1);  M(0,1) = s*M00 + c*M(0,1);
        float M10 = M(1,0);   M(1,0) = c*M10 - s*M(1,1);  M(1,1) = s*M10 + c*M(1,1);
        float M20 = M(2,0);   M(2,0) = c*M20 - s*M(2,1);  M(2,1) = s*M20 + c*M(2,1);
        float M30 = M(3,0);   M(3,0) = c*M30 - s*M(3,1);  M(3,1) = s*M30 + c*M(3,1);
        // clang-format on
    }

    void PreRotateX(Radian Rx)
    {
        if (Rx == 0) {
            return;
        }

        float s, c;
        sincos(Rx, s, c);

        // this is the slow method but easy to see what's going on:
        //matrix4 RM;
        //RM.Identity();
        //RM.M(1,1) =  c;
        //RM.M(2,1) = -s;
        //RM.M(1,2) =  s;
        //RM.M(2,2) =  c;
        //*this *= RM;

        // this is the fast method with less multiplies and adds:
        // clang-format off
        float M10 = M(1,0);   M(1,0) = c*M10 + s*M(2,0);  M(2,0) = c*M(2,0) - s*M10;
        float M11 = M(1,1);   M(1,1) = c*M11 + s*M(2,1);  M(2,1) = c*M(2,1) - s*M11;
        float M12 = M(1,2);   M(1,2) = c*M12 + s*M(2,2);  M(2,2) = c*M(2,2) - s*M12;
        float M13 = M(1,3);   M(1,3) = c*M13 + s*M(2,3);  M(2,3) = c*M(2,3) - s*M13;
        // clang-format on
    }

    void PreRotateY(Radian Ry)
    {
        if (Ry == 0) {
            return;
        }

        float s, c;
        sincos(Ry, s, c);

        // this is the slow method but easy to see what's going on:
        //matrix4 RM;
        //RM.Identity();
        //RM.M(0,0) =  c;
        //RM.M(2,0) =  s;
        //RM.M(0,2) = -s;
        //RM.M(2,2) =  c;
        //*this *= RM;

        // this is the fast method with less multiplies and adds:
        // clang-format off
        float M00 = M(0,0);   M(0,0) = c*M00 - s*M(2,0);  M(2,0) = s*M00 + c*M(2,0);
        float M01 = M(0,1);   M(0,1) = c*M01 - s*M(2,1);  M(2,1) = s*M01 + c*M(2,1);
        float M02 = M(0,2);   M(0,2) = c*M02 - s*M(2,2);  M(2,2) = s*M02 + c*M(2,2);
        float M03 = M(0,3);   M(0,3) = c*M03 - s*M(2,3);  M(2,3) = s*M03 + c*M(2,3);
        // clang-format on
    }

    void PreRotateZ(Radian Rz)
    {
        if (Rz == 0) {
            return;
        }

        float s, c;
        sincos(Rz, s, c);

        // this is the slow method but easy to see what's going on:
        //matrix4 RM;
        //RM.Identity();
        //RM.M(0,0) =  c;
        //RM.M(1,0) = -s;
        //RM.M(0,1) =  s;
        //RM.M(1,1) =  c;
        //*this *= RM;

        // this is the fast method with less multiplies and adds:
        // clang-format off
        float M00 = M(0,0);   M(0,0) = c*M00 + s*M(1,0);  M(1,0) = c*M(1,0) - s*M00;
        float M01 = M(0,1);   M(0,1) = c*M01 + s*M(1,1);  M(1,1) = c*M(1,1) - s*M01;
        float M02 = M(0,2);   M(0,2) = c*M02 + s*M(1,2);  M(1,2) = c*M(1,2) - s*M02;
        float M03 = M(0,3);   M(0,3) = c*M03 + s*M(1,3);  M(1,3) = c*M(1,3) - s*M03;
        // clang-format on
    }

    Vector3 RotateVector(const Vector3& V) const
    {

        return (Vector3((M(0, 0) * V.x) + (M(1, 0) * V.y) + (M(2, 0) * V.z),
                        (M(0, 1) * V.x) + (M(1, 1) * V.y) + (M(2, 1) * V.z),
                        (M(0, 2) * V.x) + (M(1, 2) * V.y) + (M(2, 2) * V.z)));
    }

    void GetColumns(Vector3& V1, Vector3& V2, Vector3& V3) const
    {
        V1.x = M(0, 0);
        V2.x = M(1, 0);
        V3.x = M(2, 0);

        V1.y = M(0, 1);
        V2.y = M(1, 1);
        V3.y = M(2, 1);

        V1.z = M(0, 2);
        V2.z = M(1, 2);
        V3.z = M(2, 2);
    }

    void Translate(const Vector3& Translation)
    {
        // If bottom row of M is [0 0 0 1] do optimized translation.
        if ((M(0, 3) == 0.0f) &&
            (M(1, 3) == 0.0f) &&
            (M(2, 3) == 0.0f) &&
            (M(3, 3) == 1.0f)) {
            M(3, 0) += Translation.x;
            M(3, 1) += Translation.y;
            M(3, 2) += Translation.z;
        } else {
            M(0, 0) += M(0, 3) * Translation.x;
            M(1, 0) += M(1, 3) * Translation.x;
            M(2, 0) += M(2, 3) * Translation.x;
            M(3, 0) += M(3, 3) * Translation.x;

            M(0, 1) += M(0, 3) * Translation.y;
            M(1, 1) += M(1, 3) * Translation.y;
            M(2, 1) += M(2, 3) * Translation.y;
            M(3, 1) += M(3, 3) * Translation.y;

            M(0, 2) += M(0, 3) * Translation.z;
            M(1, 2) += M(1, 3) * Translation.z;
            M(2, 2) += M(2, 3) * Translation.z;
            M(3, 2) += M(3, 3) * Translation.z;
        }
    }

    Vector3 Transform(const Vector3& V) const
    {
        return ((*this) * V);
    }

    void Transform(Vector3*       pDest,
                   const Vector3* pSource,
                   int            NVerts) const
    {
        while (NVerts > 0) {
            *pDest = Transform(*pSource);

            pDest++;
            pSource++;
            NVerts--;
        }
    }

    Vector4 Transform(const Vector4& V) const
    {
        return ((*this) * V);
    }

    void Transform(Vector4*       pDest,
                   const Vector4* pSource,
                   int            NVerts) const
    {
        while (NVerts > 0) {
            *pDest = Transform(*pSource);

            pDest++;
            pSource++;
            NVerts--;
        }
    }

    void Scale(const Vector3& scale)
    {
        M(0, 0) *= scale.x;
        M(0, 1) *= scale.y;
        M(0, 2) *= scale.z;

        M(1, 0) *= scale.x;
        M(1, 1) *= scale.y;
        M(1, 2) *= scale.z;

        M(2, 0) *= scale.x;
        M(2, 1) *= scale.y;
        M(2, 2) *= scale.z;

        // Scale the translation.

        M(3, 0) *= scale.x;
        M(3, 1) *= scale.y;
        M(3, 2) *= scale.z;
    }

    void PreScale(const Vector3& Scale)
    {
        M(0, 0) *= Scale.x;
        M(0, 1) *= Scale.x;
        M(0, 2) *= Scale.x;
        M(0, 3) *= Scale.x;

        M(1, 0) *= Scale.y;
        M(1, 1) *= Scale.y;
        M(1, 2) *= Scale.y;
        M(1, 3) *= Scale.y;

        M(2, 0) *= Scale.z;
        M(2, 1) *= Scale.z;
        M(2, 2) *= Scale.z;
        M(2, 3) *= Scale.z;
    }

    void PreTranslate(const Vector3& Translation)
    {
        M(3, 0) += (M(0, 0) * Translation.x) + (M(1, 0) * Translation.y) + (M(2, 0) * Translation.z);
        M(3, 1) += (M(0, 1) * Translation.x) + (M(1, 1) * Translation.y) + (M(2, 1) * Translation.z);
        M(3, 2) += (M(0, 2) * Translation.x) + (M(1, 2) * Translation.y) + (M(2, 2) * Translation.z);
        M(3, 3) += (M(0, 3) * Translation.x) + (M(1, 3) * Translation.y) + (M(2, 3) * Translation.z);
    }

    void Setup(const Vector3& Axis, Radian Angle);

    void Setup(const Vector3& Scale,
               const Radian3& Rotation,
               const Vector3& Translation);

    void Setup(const Radian3& Rotation)
    {
        SetRotation(Rotation);

        M(0, 3) = 0.0f;
        M(1, 3) = 0.0f;
        M(2, 3) = 0.0f;
        M(3, 3) = 1.0f;
        M(3, 2) = 0.0f;
        M(3, 1) = 0.0f;
        M(3, 0) = 0.0f;
    }

    void Setup(const Quaternion& Q)
    {
        SetRotation(Q);

        M(0, 3) = 0.0f;
        M(1, 3) = 0.0f;
        M(2, 3) = 0.0f;
        M(3, 3) = 1.0f;
        M(3, 2) = 0.0f;
        M(3, 1) = 0.0f;
        M(3, 0) = 0.0f;
    }

    void Setup(const Vector3&    Scale,
               const Quaternion& Rotation,
               const Vector3&    Translation);

    void SetRotation(const Radian3& Rotation);

    void SetRotation(const Quaternion& Q)
    {

        float tx = 2.0f * Q.x; // 2x
        float ty = 2.0f * Q.y; // 2y
        float tz = 2.0f * Q.z; // 2z
        float txw = tx * Q.w;  // 2x * w
        float tyw = ty * Q.w;  // 2y * w
        float tzw = tz * Q.w;  // 2z * w
        float txx = tx * Q.x;  // 2x * x
        float tyx = ty * Q.x;  // 2y * x
        float tzx = tz * Q.x;  // 2z * x
        float tyy = ty * Q.y;  // 2y * y
        float tzy = tz * Q.y;  // 2z * y
        float tzz = tz * Q.z;  // 2z * z

        // Fill out 3x3 rotations.

        M(0, 0) = 1.0f - (tyy + tzz);
        M(0, 1) = tyx + tzw;
        M(0, 2) = tzx - tyw;
        M(1, 0) = tyx - tzw;
        M(1, 1) = 1.0f - (txx + tzz);
        M(1, 2) = tzy + txw;
        M(2, 0) = tzx + tyw;
        M(2, 1) = tzy - txw;
        M(2, 2) = 1.0f - (txx + tyy);
    }

    Matrix4& operator+=(const Matrix4& M);

    Vector3 operator*(const Vector3& V) const
    {
        return (Vector3((M(0, 0) * V.x) + (M(1, 0) * V.y) + (M(2, 0) * V.z) + M(3, 0),
                        (M(0, 1) * V.x) + (M(1, 1) * V.y) + (M(2, 1) * V.z) + M(3, 1),
                        (M(0, 2) * V.x) + (M(1, 2) * V.y) + (M(2, 2) * V.z) + M(3, 2)));
    }
    Vector4 operator*(const Vector4& V) const
    {
        return (Vector4((M(0, 0) * V.x) + (M(1, 0) * V.y) + (M(2, 0) * V.z) + (M(3, 0) * V.w),
                        (M(0, 1) * V.x) + (M(1, 1) * V.y) + (M(2, 1) * V.z) + (M(3, 1) * V.w),
                        (M(0, 2) * V.x) + (M(1, 2) * V.y) + (M(2, 2) * V.z) + (M(3, 2) * V.w),
                        (M(0, 3) * V.x) + (M(1, 3) * V.y) + (M(2, 3) * V.z) + (M(3, 3) * V.w)));
    }

    friend Matrix4 operator+(const Matrix4& M1, const Matrix4& M2);
    friend Matrix4 operator-(const Matrix4& M1, const Matrix4& M2);
    friend Matrix4 operator*(const Matrix4& M1, const Matrix4& M2);
};

inline Matrix4 operator+(const Matrix4& M1, const Matrix4& M2)
{
    Matrix4 Result;
    Result.M(0, 0) = M1.M(0, 0) + M2.M(0, 0);
    Result.M(0, 1) = M1.M(0, 1) + M2.M(0, 1);
    Result.M(0, 2) = M1.M(0, 2) + M2.M(0, 2);
    Result.M(0, 3) = M1.M(0, 3) + M2.M(0, 3);

    Result.M(1, 0) = M1.M(1, 0) + M2.M(1, 0);
    Result.M(1, 1) = M1.M(1, 1) + M2.M(1, 1);
    Result.M(1, 2) = M1.M(1, 2) + M2.M(1, 2);
    Result.M(1, 3) = M1.M(1, 3) + M2.M(1, 3);

    Result.M(2, 0) = M1.M(2, 0) + M2.M(2, 0);
    Result.M(2, 1) = M1.M(2, 1) + M2.M(2, 1);
    Result.M(2, 2) = M1.M(2, 2) + M2.M(2, 2);
    Result.M(2, 3) = M1.M(2, 3) + M2.M(2, 3);

    Result.M(3, 0) = M1.M(3, 0) + M2.M(3, 0);
    Result.M(3, 1) = M1.M(3, 1) + M2.M(3, 1);
    Result.M(3, 2) = M1.M(3, 2) + M2.M(3, 2);
    Result.M(3, 3) = M1.M(3, 3) + M2.M(3, 3);

    return Result;
}

//==============================================================================

inline Matrix4 operator-(const Matrix4& M1, const Matrix4& M2)
{
    Matrix4 Result;

    Result.M(0, 0) = M1.M(0, 0) - M2.M(0, 0);
    Result.M(0, 1) = M1.M(0, 1) - M2.M(0, 1);
    Result.M(0, 2) = M1.M(0, 2) - M2.M(0, 2);
    Result.M(0, 3) = M1.M(0, 3) - M2.M(0, 3);

    Result.M(1, 0) = M1.M(1, 0) - M2.M(1, 0);
    Result.M(1, 1) = M1.M(1, 1) - M2.M(1, 1);
    Result.M(1, 2) = M1.M(1, 2) - M2.M(1, 2);
    Result.M(1, 3) = M1.M(1, 3) - M2.M(1, 3);

    Result.M(2, 0) = M1.M(2, 0) - M2.M(2, 0);
    Result.M(2, 1) = M1.M(2, 1) - M2.M(2, 1);
    Result.M(2, 2) = M1.M(2, 2) - M2.M(2, 2);
    Result.M(2, 3) = M1.M(2, 3) - M2.M(2, 3);

    Result.M(3, 0) = M1.M(3, 0) - M2.M(3, 0);
    Result.M(3, 1) = M1.M(3, 1) - M2.M(3, 1);
    Result.M(3, 2) = M1.M(3, 2) - M2.M(3, 2);
    Result.M(3, 3) = M1.M(3, 3) - M2.M(3, 3);

    return Result;
}

inline Matrix4 operator*(const Matrix4& L, const Matrix4& R)
{
    Matrix4 Result;
    for (int i = 0; i < 4; i++) {
        Result.M(i, 0) = (L.M(0, 0) * R.M(i, 0)) + (L.M(1, 0) * R.M(i, 1)) + (L.M(2, 0) * R.M(i, 2)) + (L.M(3, 0) * R.M(i, 3));
        Result.M(i, 1) = (L.M(0, 1) * R.M(i, 0)) + (L.M(1, 1) * R.M(i, 1)) + (L.M(2, 1) * R.M(i, 2)) + (L.M(3, 1) * R.M(i, 3));
        Result.M(i, 2) = (L.M(0, 2) * R.M(i, 0)) + (L.M(1, 2) * R.M(i, 1)) + (L.M(2, 2) * R.M(i, 2)) + (L.M(3, 2) * R.M(i, 3));
        Result.M(i, 3) = (L.M(0, 3) * R.M(i, 0)) + (L.M(1, 3) * R.M(i, 1)) + (L.M(2, 3) * R.M(i, 2)) + (L.M(3, 3) * R.M(i, 3));
    }
    return Result;
}

inline void Matrix4::Setup(const Vector3& Axis, Radian Angle)
{
    Setup(Quaternion(Axis, Angle));
}

inline void Matrix4::Setup(const Vector3& Scale,
                           const Radian3& Rotation,
                           const Vector3& Translation)
{

    float sx, cx;
    float sy, cy;
    float sz, cz;
    float sxsz;
    float sxcz;

    sincos(Rotation.pitch, sx, cx);
    sincos(Rotation.yaw, sy, cy);
    sincos(Rotation.roll, sz, cz);

    sxsz = sx * sz;
    sxcz = sx * cz;

    // Fill out 3x3 rotations.
    M(0, 0) = (cy * cz + sy * sxsz) * Scale.x;
    M(0, 1) = (cx * sz) * Scale.x;
    M(0, 2) = (cy * sxsz - sy * cz) * Scale.x;
    M(1, 0) = (sy * sxcz - sz * cy) * Scale.y;
    M(1, 1) = (cx * cz) * Scale.y;
    M(1, 2) = (sy * sz + sxcz * cy) * Scale.y;
    M(2, 0) = (cx * sy) * Scale.z;
    M(2, 1) = (-sx) * Scale.z;
    M(2, 2) = (cx * cy) * Scale.z;

    // Translate
    M(3, 0) = Translation.x;
    M(3, 1) = Translation.y;
    M(3, 2) = Translation.z;

    // Clear bottom row
    M(0, 3) = 0.0f;
    M(1, 3) = 0.0f;
    M(2, 3) = 0.0f;
    M(3, 3) = 1.0f;
}

inline void Matrix4::Setup(const Vector3&    Scale,
                           const Quaternion& Rotation,
                           const Vector3&    Translation)
{
    /*
    Identity   ( );
    SetRotation( Rotation );
    PreScale   ( Scale );
    Translate  ( Translation );
    */

    float tx = 2.0f * Rotation.x; // 2x
    float ty = 2.0f * Rotation.y; // 2y
    float tz = 2.0f * Rotation.z; // 2z
    float txw = tx * Rotation.w;  // 2x * w
    float tyw = ty * Rotation.w;  // 2y * w
    float tzw = tz * Rotation.w;  // 2z * w
    float txx = tx * Rotation.x;  // 2x * x
    float tyx = ty * Rotation.x;  // 2y * x
    float tzx = tz * Rotation.x;  // 2z * x
    float tyy = ty * Rotation.y;  // 2y * y
    float tzy = tz * Rotation.y;  // 2z * y
    float tzz = tz * Rotation.z;  // 2z * z

    // Fill out 3x3 rotations.
    M(0, 0) = (1.0f - (tyy + tzz)) * Scale.x;
    M(0, 1) = (tyx + tzw) * Scale.x;
    M(0, 2) = (tzx - tyw) * Scale.x;
    M(1, 0) = (tyx - tzw) * Scale.y;
    M(1, 1) = (1.0f - (txx + tzz)) * Scale.y;
    M(1, 2) = (tzy + txw) * Scale.y;
    M(2, 0) = (tzx + tyw) * Scale.z;
    M(2, 1) = (tzy - txw) * Scale.z;
    M(2, 2) = (1.0f - (txx + tyy)) * Scale.z;

    // Translate
    M(3, 0) = Translation.x;
    M(3, 1) = Translation.y;
    M(3, 2) = Translation.z;

    // Clear bottom row
    M(0, 3) = 0.0f;
    M(1, 3) = 0.0f;
    M(2, 3) = 0.0f;
    M(3, 3) = 1.0f;
}

inline Radian3 Matrix4::GetRotation() const
{
    float          Roll, Pitch, Yaw;
    const Matrix4& M = *this;

    // rot =  cy*cz-sx*sy*sz -cx*sz           cz*sy+cy*sx*sz
    //        cz*sx*sy+cy*sz  cx*cz          -cy*cz*sx+sy*sz
    //       -cx*sy           sx              cx*cy

    // Do we have to deal with scales?
    // We loose precision and speed if we have to.

    const float l1 = std::abs(Vector3(M(0, 0), M(0, 1), M(0, 2)).LengthSquared());
    const float l2 = std::abs(Vector3(M(1, 0), M(1, 1), M(1, 2)).LengthSquared());
    const float l3 = std::abs(Vector3(M(2, 0), M(2, 1), M(2, 2)).LengthSquared());

    if (l1 > (1 + 0.001f) || l1 < (1 - 0.001f) ||
        l2 > (1 + 0.001f) || l2 < (1 - 0.001f) ||
        l3 > (1 + 0.001f) || l3 < (1 - 0.001f)) {
        Matrix4 OM = M;
        OM.ClearScale();

        if (OM(2, 1) < 1.0f) {
            if (OM(2, 1) > -1.0f) {
                Roll = std::atan2(OM(0, 1), OM(1, 1));
                Pitch = -std::asin(OM(2, 1));
                Yaw = std::atan2(OM(2, 0), OM(2, 2));
            } else {
                // WARNING.  Not unique.  ZA - YA = atan(r02,r00)
                Roll = std::atan2(OM(0, 2), OM(0, 0));
                Pitch = (PI / 2.0f);
                Yaw = 0.0f;
            }
        } else {
            // WARNING.  Not unique.  ZA + YA = -atan2(r02,r00)
            Roll = -std::atan2(OM(0, 2), OM(0, 0));
            Pitch = -(PI / 2.0f);
            Yaw = 0.0f;
        }
    } else {
        if (M(2, 1) < 1.0f) {
            if (M(2, 1) > -1.0f) {
                Roll = std::atan2(M(0, 1), M(1, 1));
                Pitch = -std::asin(M(2, 1));
                Yaw = std::atan2(M(2, 0), M(2, 2));
            } else {
                // WARNING.  Not unique.  ZA - YA = atan(r02,r00)
                Roll = std::atan2(M(0, 2), M(0, 0));
                Pitch = (PI / 2.0f);
                Yaw = 0.0f;
            }
        } else {
            // WARNING.  Not unique.  ZA + YA = -atan2(r02,r00)
            Roll = -std::atan2(M(0, 2), M(0, 0));
            Pitch = -(PI / 2.0f);
            Yaw = 0.0f;
        }
    }

    return Radian3(Pitch, Yaw, Roll);
}

inline void Matrix4::SetRotation(const Radian3& Rotation)
{
    float sx, cx;
    float sy, cy;
    float sz, cz;
    float sxsz;
    float sxcz;

    sincos(Rotation.pitch, sx, cx);
    sincos(Rotation.yaw, sy, cy);
    sincos(Rotation.roll, sz, cz);

    sxsz = sx * sz;
    sxcz = sx * cz;

    // Fill out 3x3 rotations.

    M(0, 0) = cy * cz + sy * sxsz;
    M(0, 1) = cx * sz;
    M(0, 2) = cy * sxsz - sy * cz;
    M(1, 0) = sy * sxcz - sz * cy;
    M(1, 1) = cx * cz;
    M(1, 2) = sy * sz + sxcz * cy;
    M(2, 0) = cx * sy;
    M(2, 1) = -sx;
    M(2, 2) = cx * cy;
}

inline Matrix4 m4_InvertRT(const Matrix4& Src)
{
    Matrix4 Dest;

    Dest(0, 0) = Src(0, 0);
    Dest(0, 1) = Src(1, 0);
    Dest(0, 2) = Src(2, 0);
    Dest(1, 0) = Src(0, 1);
    Dest(1, 1) = Src(1, 1);
    Dest(1, 2) = Src(2, 1);
    Dest(2, 0) = Src(0, 2);
    Dest(2, 1) = Src(1, 2);
    Dest(2, 2) = Src(2, 2);
    Dest(0, 3) = 0.0f;
    Dest(1, 3) = 0.0f;
    Dest(2, 3) = 0.0f;
    Dest(3, 3) = 1.0f;
    Dest(3, 0) = -(Src(3, 0) * Dest(0, 0) + Src(3, 1) * Dest(1, 0) + Src(3, 2) * Dest(2, 0));
    Dest(3, 1) = -(Src(3, 0) * Dest(0, 1) + Src(3, 1) * Dest(1, 1) + Src(3, 2) * Dest(2, 1));
    Dest(3, 2) = -(Src(3, 0) * Dest(0, 2) + Src(3, 1) * Dest(1, 2) + Src(3, 2) * Dest(2, 2));

    return Dest;
}

inline bool Matrix4::InvertRT()
{
    *this = m4_InvertRT(*this);
    return true;
}

inline void Matrix4::Orthogonalize()
{
    Vector3 VX(M(0, 0), M(0, 1), M(0, 2));
    Vector3 VY(M(1, 0), M(1, 1), M(1, 2));

    Vector3 VZ;

    VX.Normalize();
    VY.Normalize();

    VZ = v3_Cross(VX, VY);
    VY = v3_Cross(VZ, VX);

    SetColumns(VX, VY, VZ);
}

inline Matrix4 m4_Transpose(const Matrix4& M)
{
    Matrix4 Mout = M;
    Mout.Transpose();
    return Mout;
}

inline Matrix4& Matrix4::operator+=(const Matrix4& aM)
{
    M(0, 0) += aM.M(0, 0);
    M(0, 1) += aM.M(0, 1);
    M(0, 2) += aM.M(0, 2);
    M(0, 3) += aM.M(0, 3);

    M(1, 0) += aM.M(1, 0);
    M(1, 1) += aM.M(1, 1);
    M(1, 2) += aM.M(1, 2);
    M(1, 3) += aM.M(1, 3);

    M(2, 0) += aM.M(2, 0);
    M(2, 1) += aM.M(2, 1);
    M(2, 2) += aM.M(2, 2);
    M(2, 3) += aM.M(2, 3);

    M(3, 0) += aM.M(3, 0);
    M(3, 1) += aM.M(3, 1);
    M(3, 2) += aM.M(3, 2);
    M(3, 3) += aM.M(3, 3);

    return (*this);
}

#undef M
