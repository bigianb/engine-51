#include "Vector3.h"

void x_ClosestPtsOnLineSegs(const Vector3& StartA, const Vector3& EndA, // IN
                            const Vector3& StartB, const Vector3& EndB, // IN
                            Vector3& PtOnA,                             // OUT
                            Vector3& PtOnB,                             // OUT
                            float&   TA,                                // OUT
                            float&   TB)                                  // OUT
{
    Vector3 u = EndA - StartA;
    Vector3 v = EndB - StartB;
    Vector3 w = StartA - StartB;
    float   a = u.Dot(u); // always >= 0
    float   b = u.Dot(v);
    float   c = v.Dot(v); // always >= 0
    float   d = u.Dot(w);
    float   e = v.Dot(w);
    float   D = a * c - b * b; // always >= 0

    //
    // compute the line parameters of the two closest points
    //
    if (D < 0.000001f) {
        TA = 0.0f;
        TB = (b > c ? d / b : e / c); // use the largest denominator
    } else {
        TA = (b * e - c * d) / D;
        TB = (a * e - b * d) / D;
    }

    if (TA < 0) {
        TA = 0;
    }
    if (TA > 1) {
        TA = 1;
    }
    if (TB < 0) {
        TB = 0;
    }
    if (TB > 1) {
        TB = 1;
    }

    PtOnA = StartA + TA * u;
    PtOnB = StartB + TB * v;
}

void x_ClosestPtsOnLineSegs(const Vector3& StartA, const Vector3& EndA, // IN
                            const Vector3& StartB, const Vector3& EndB, // IN
                            Vector3& PtOnA,                             // OUT
                            Vector3& PtOnB)                             // OUT
{
    float TA, TB;

    x_ClosestPtsOnLineSegs(StartA, EndA, StartB, EndB, PtOnA, PtOnB, TA, TB);
}

float Vector3::ClosestPointToRectangle(const Vector3& P0, // Origin from the edges.
                                       const Vector3& E0,
                                       const Vector3& E1,
                                       Vector3&       OutClosestPoint) const
{
    Vector3 kDiff = P0 - *this;
    float   fA00 = E0.LengthSquared();
    float   fA11 = E1.LengthSquared();
    float   fB0 = kDiff.Dot(E0);
    float   fB1 = kDiff.Dot(E1);
    float   fS = -fB0;
    float   fT = -fB1;
    float   fSqrDist = kDiff.LengthSquared();

    if (fS < 0.0f) {
        fS = 0.0f;
    } else if (fS <= fA00) {
        fS /= fA00;
        fSqrDist += fB0 * fS;
    } else {
        fS = 1.0f;
        fSqrDist += fA00 + 2.0f * fB0;
    }

    if (fT < 0.0f) {
        fT = 0.0f;
    } else if (fT <= fA11) {
        fT /= fA11;
        fSqrDist += fB1 * fT;
    } else {
        fT = 1.0f;
        fSqrDist += fA11 + 2.0f * fB1;
    }

    // Set the closest point
    OutClosestPoint = P0 + (E0 * fS) + (E1 * fT);

    return abs(fSqrDist);
}
