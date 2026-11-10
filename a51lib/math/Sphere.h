#pragma once

#include "Vector3.h"
#include "BoundingBox.h"
#include "Plane.h"

class Sphere
{
public:
    Vector3 Pos;
    float R;

    Sphere() {}
    Sphere(const Vector3 &Pos, float R)
        : Pos(Pos), R(R)
    {
    }

    Sphere(const BBox &bbox)
    {
        Pos = bbox.GetCenter();
        R = bbox.GetRadius();
    }

    void Clear()
    {
        Pos.Zero();
        R = 0.0f;
    }

    void Set(const Vector3 &Pos, float R)
    {
        this->Pos = Pos;
        this->R = R;
    }

    BBox GetBBox() const
    {
        return BBox(Pos, R);
    }

    bool TestIntersect(const Vector3 &P0, const Vector3 &P1) const
    {
        const float SqrD = Pos.GetSqrtDistToLineSeg(P0, P1);
        return SqrD <= (R * R);
    }

    int TestIntersection(const plane &Plane) const
    {
        const float Distance = Plane.Normal.Dot(Pos) - Plane.D;

        if (Distance < -R || Distance > R)
            return 0;

        if (Distance < 0)
            return -1;
        return 1;
    }

    int Intersect(float &t0, float &t1, const Vector3 &P0, const Vector3 &P1) const
    {
        // set up quadratic Q(t) = a*t^2 + 2*b*t + c
        const Vector3 Diff = P0 - Pos;
        const Vector3 Dir = (P1 - P0);
        const float A = Dir.LengthSquared();
        const float B = Diff.Dot(Dir);
        const float C = Diff.LengthSquared() - (R * R);

        // no intersection if Q(t) has no real roots
        const float Discr = B * B - A * C;

        if (Discr < 0.0f)
        {
            // The ray is completly contain inside the sphere
            if (C < 0)
                return -0x1;

            return 0x0;
        }

        if (Discr > 0.0f)
        {
            const float Root = sqrt(Discr);
            const float InvA = 1.0f / A;
            t0 = (-B - Root) * InvA;
            t1 = (-B + Root) * InvA;

            // assert: t0 < t1 since A > 0
            assert(t0 < t1);

            if (t0 > 1.0f || t1 < 0.0f)
                return 0x0;

            if (t0 >= 0.0f)
            {
                // there is an entrance point but not an exit
                if (t1 > 1.0f)
                    return 0x1;

                return 0x3;
            }
            else // t1 >= 0
            {
                assert(t1 >= 0);

                // there is only one exit point
                return 0x2;
            }
        }
        else
        {
            t0 = -B / A;

            // there is an entrance point but not an exit
            if (t0 >= 0.0f && t0 <= 1.0f)
                return 0x1;
        }

        return 0x0;
    }

    bool Intersect(float &t0, const Vector3 &P0, const Vector3 &P1) const
    {
        float t1;
        const int C = Intersect(t0, t1, P0, P1);

        // We didn't hit
        if (C == 0)
            return 0;

        // If the ray is completly contain in the sphere
        if (C == -1)
        {
            t0 = 0;
            return true;
        }

        // If there is only an exit point then we consider that we were in.
        if (C == 2)
        {
            t0 = 0;
            return true;
        }

        // Anything with an entry hit is a good hit
        assert(C & 1);

        return true;
    }
};
