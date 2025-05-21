#pragma once

#include "Vector3.h"
#include "Matrix4.h"
#include "Plane.h"

struct BBox
{
    Vector3 min, max;

    BBox() {}
    BBox(const Vector3& Center, float Radius)
    {
        min = Center - Vector3(Radius, Radius, Radius);
        max = Center + Vector3(Radius, Radius, Radius);
    }

    BBox(const Vector3& P1, const Vector3& P2)
    {
        min = max = P1;
        min.Min(P2);
        max.Max(P2);
    }

    BBox(const Vector3& P1, const Vector3& P2, const Vector3& P3)
    {
        min = max = P1;
        min.Min(P2);
        max.Max(P2);
        min.Min(P3);
        max.Max(P3);
    }

    void Set(const Vector3& Center, float Radius)
    {
        min = Center - Vector3(Radius, Radius, Radius);
        max = Center + Vector3(Radius, Radius, Radius);
    }

    void Set(const Vector3& p1, const Vector3& p2)
    {
        min = max = p1;
        min.Min(p2);
        max.Max(p2);
    }

    void Clear() { reset(); }

    void reset()
    {
        min.x = std::numeric_limits<float>::max();
        min.y = std::numeric_limits<float>::max();
        min.z = std::numeric_limits<float>::max();

        max.x = std::numeric_limits<float>::min();
        max.y = std::numeric_limits<float>::min();
        max.z = std::numeric_limits<float>::min();
    }

    Vector3 GetSize() const { return (max - min); }
    Vector3 GetCenter() const { return ((min + max) * 0.5f); };

    float GetRadius() const { return (max - min).Length() / 2.0f; }
    float GetRadiusSquared() const
    {
        const Vector3 r = (max - min) * 0.5f;
        return r.Dot(r);
    }

    void Inflate(float x, float y, float z)
    {
        min -= Vector3(x, y, z);
        max += Vector3(x, y, z);
    }

    void Translate(float x, float y, float z)
    {
        min += Vector3(x, y, z);
        max += Vector3(x, y, z);
    }

    void Translate(const Vector3& T)
    {
        min += T;
        max += T;
    }

    void Transform(const Matrix4& mtx)
    {
        Vector3 AMin, AMax;
        float   a, b;
        int     i, j;

        // Copy box A into min and max array.
        AMin = min;
        AMax = max;

        // Begin at T.
        min = max = mtx.GetTranslation();

        // Find extreme points by considering product of
        // min and max with each component of M.
        for (j = 0; j < 3; j++) {
            for (i = 0; i < 3; i++) {
                a = mtx(i, j) * AMin[i];
                b = mtx(i, j) * AMax[i];

                if (a < b) {
                    min[j] += a;
                    max[j] += b;
                } else {
                    min[j] += b;
                    max[j] += a;
                }
            }
        }
    }

    bool Intersect(const Vector3& Point) const
    {
        return ((Point.GetX() <= max.GetX()) &&
                (Point.GetY() <= max.GetY()) &&
                (Point.GetZ() <= max.GetZ()) &&
                (Point.GetX() >= min.GetX()) &&
                (Point.GetY() >= min.GetY()) &&
                (Point.GetZ() >= min.GetZ()));
    }

    bool Intersect(const BBox& bb) const
    {
        if (bb.min.x > max.x) {
            return false;
        }
        if (bb.max.x < min.x) {
            return false;
        }
        if (bb.min.z > max.z) {
            return false;
        }
        if (bb.max.z < min.z) {
            return false;
        }
        if (bb.min.y > max.y) {
            return false;
        }
        if (bb.max.y < min.y) {
            return false;
        }
        return true;
    }
    bool Intersect(const plane& Plane) const;
    bool Intersect(float& t, const Vector3& P0, const Vector3& P1) const;
    bool Intersect(const Vector3& Center, float Radius) const;
    bool Intersect(const Vector3* pVert, int nVerts) const;

    bool IntersectTriBBox(const Vector3& P0,
                          const Vector3& P1,
                          const Vector3& P2) const
    {
        // Compute triangle bbox
        BBox TriBBox(P0, P1, P2);
        return Intersect(TriBBox);
    }

    BBox& AddVerts(const Vector3* pVerts, int NVerts)
    {
        assert(pVerts);
        assert(NVerts > 0);

        while (NVerts > 0) {
            min.Min(*pVerts);
            max.Max(*pVerts);

            pVerts++;
            NVerts--;
        }

        return (*this);
    }

    BBox& operator+=(const BBox& box)
    {
        min.Min(box.min);
        max.Max(box.max);

        return (*this);
    }

    BBox& operator+=(const Vector3& Point)
    {
        min.Min(Point);
        max.Max(Point);

        return (*this);
    }
};
