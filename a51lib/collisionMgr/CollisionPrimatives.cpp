#include "CollisionPrimatives.h"

#include <cassert>

bool CollideSphereWithPoint(const Vector3& Start,
                            const Vector3& Dir,
                            float          Radius,
                            const Vector3& Point,
                            float&         hit_time,
                            Vector3&       hit_point)
{
    float length = Dir.Length();
    if (abs(length) < 0.0001f) {
        return false;
    }

    Vector3 ray_start = Point;
    Vector3 ray_dir = -Dir;
    Vector3 sphere_pos = Start;
    float   sphere_rad = Radius;
    ray_dir /= length;

    // get the offset vector
    Vector3 offset = sphere_pos - ray_start;

    // get the distance along the ray to the center point of the sphere
    float ray_dist = ray_dir.Dot(offset);
    if (ray_dist <= 0 || (ray_dist - length) > sphere_rad) {
        // moving away from object or too far away
        return false;
    }

    // get the squared distances
    float off2 = offset.Dot(offset);
    float Rad2 = sphere_rad * sphere_rad;
    if (off2 <= Rad2) {
        // we're in the sphere
        hit_point = ray_start;
        hit_time = 0;
        return true;
    }

    // find hit distance squared
    float d = Rad2 - (off2 - ray_dist * ray_dist);
    if (d < 0) {
        // ray passes by sphere without hitting
        return false;
    }

    // get the distance along the ray
    hit_time = (float)(ray_dist - sqrt(d));
    if (hit_time > length) {
        // hit point beyond length
        return false;
    }

    // sort out the details
    Vector3 sphere_point = ray_start + ray_dir * hit_time;
    hit_time /= length;
    hit_point = sphere_point + hit_time * Dir;
    return true;
}

//=========================================================================

bool ComputeSphereTriCollision(const Vector3* Tri,
                               const Vector3& Start,
                               const Vector3& End,
                               float          Radius,
                               float&         FinalT,
                               Vector3&       FinalHitPoint)
{
    int     i;
    Vector3 Dir = End - Start;
    plane   Plane;
    Plane.Setup(Tri[0], Tri[1], Tri[2]);
    const float DirDotNormal = Dir.Dot(Plane.Normal);

    // Moving away from plane?
    if (DirDotNormal > 0) {
        return false;
    }

    // Are we completely in front or starting from behind?
    float StartDist = Plane.Distance(Start);
    float EndDist = Plane.Distance(End);
    if ((StartDist < -Radius) || (EndDist > Radius)) {
        return false;
    }

    //
    // Find the closest point on the sphere to the plane
    //
    {
        float   T;
        Vector3 HP;
        Vector3 SphereBot = Start - (Plane.Normal * Radius);
        T = -Plane.Distance(SphereBot) / DirDotNormal;

        if (T > 1.0f) {
            return false;
        }

        if (T >= 0) {
            HP = SphereBot + T * Dir;

            // Determine if point is inside tri.
            int i;
            for (i = 0; i < 3; i++) {
                if (v3_Cross(Plane.Normal, Tri[(i + 1) % 3] - Tri[i]).Dot(HP - Tri[i]) < 0) {
                    break;
                }
            }

            if (i == 3) {
                FinalT = T;
                FinalHitPoint = HP;
                return true;
            }
        }
    }

    FinalT = 2.0f;
    uint32_t CullVert = 0;

    int va = 0;
    int vb = 2;
    for (i = 0; i < 3; i++) {
        va = vb;
        vb = i;
        Vector3 P0 = Tri[va];
        Vector3 P1 = Tri[vb];

        Vector3 Edge = P1 - P0;
        Vector3 Delta = Start - P0;
        float   DeltaDotEdge = Delta.Dot(Edge);
        float   DeltaDotDir = Delta.Dot(Dir);
        float   EdgeDotDir = Edge.Dot(Dir);
        float   DeltaSqr = Delta.LengthSquared();
        float   EdgeSqr = Edge.LengthSquared();
        float   DirSqr = Dir.LengthSquared();

        float A = EdgeDotDir * EdgeDotDir - EdgeSqr * DirSqr;
        float B = 2 * (DeltaDotEdge * EdgeDotDir - DeltaDotDir * EdgeSqr);
        float C = DeltaDotEdge * DeltaDotEdge + Radius * Radius * EdgeSqr - DeltaSqr * EdgeSqr;
        float Disc = B * B - 4 * A * C;

        if (Disc < 0) {
            // discriminant negative, sphere passed edge too far away
            // don't check verts
            CullVert |= ((1 << va) | (1 << vb));
            continue;
        }

        if (A < -0.0001f || A > 0.0001f) {
            float root = sqrt(Disc);
            float root1 = (-B + root) / (2 * A);
            float root2 = (-B - root) / (2 * A);

            // sort root1 and root2, use the earliest intersection.  the larger root
            //  corresponds to the final contact of the sphere with the edge on its
            //  way out.
            if (root2 < root1) {
                float temp = root1;
                root1 = root2;
                root2 = temp;
            }

            // root1 is a time, check that it's in our currently valid range
            if ((root1 < 0) || (root1 > 1.0f)) {
                // did not hit line within time range
                continue;
            }

            // find sphere and edge positions
            Vector3 SphereHit = Start + Dir * root1;

            // check if hit is between P0 and P1
            float EdgeT = ((SphereHit - P0).Dot(Edge)) / EdgeSqr;
            if ((EdgeT >= 0) && (EdgeT <= 1)) {
                // bingo
                if (root1 < FinalT) {
                    FinalT = root1;
                    FinalHitPoint = P0 + Edge * EdgeT;

                    // hit within edge so don't check end verts
                    CullVert |= ((1 << va) | (1 << vb));
                    continue;
                }
            }

            // sphere hit line but not within segment.
            continue;
        }

        // Degenerate case, sphere is traveling parallel to edge.
        // It might hit the verts
    }

    // Are there any verts to check?
    if (CullVert != 0x7) {
        for (i = 0; i < 3; i++) {
            if (CullVert & (1 << i)) {
                continue;
            }

            float   T;
            Vector3 HP;
            if (CollideSphereWithPoint(Start, Dir, Radius, Tri[i], T, HP)) {
                if (T < FinalT) {
                    FinalT = T;
                    FinalHitPoint = HP;
                }
            }
        }
    }

    // Collision?
    if (FinalT < 2.0f) {
        return true;
    }

    // Intersecting plane and above the plane at the start position?
    // (stopping intersections when below the plane breaks crouching on the grated floors in excavation?!)
    // NOTE: 0.1f is used to make sure that HP != Start ( which can happen if StartDist is really small )
    if ((StartDist > 0.1f) && (StartDist < Radius)) {
        // Compute center point projected onto plane
        Vector3 HP = Start - (Plane.Normal * StartDist);

        // Make sure slip plane normal can be computed
        assert((Start - HP).SafeNormalize());

        // Determine if point is inside edges
        int i;
        for (i = 0; i < 3; i++) {
            // Inside edge?
            const Vector3& P0 = Tri[i];
            const Vector3& P1 = Tri[(i + 1) % 3];
            Vector3        EdgeDir = P1 - P0;
            Vector3        EdgeNormal = v3_Cross(Plane.Normal, EdgeDir);
            Vector3        EdgeToPoint = HP - P0;
            float          Dot = v3_Dot(EdgeNormal, EdgeToPoint);
            if (Dot < 0.0f) {
                break;
            }
        }

        // Inside all edges?
        if (i == 3) {
            FinalT = 0.0f;
            FinalHitPoint = HP;
            return true;
        }
    }

    return false;
}

//=============================================================================
bool ComputeRayTriCollision(const Vector3* Tri,
                            const Vector3& Start,
                            const Vector3& End,
                            float&         FinalT,
                            Vector3&       FinalHitPoint)
{
    int   i;
    plane Plane;
    Plane.Setup(Tri[0], Tri[1], Tri[2]);

    // Are we completely in front or starting from behind?
    if (!(Plane.InFront(Start) && Plane.InBack(End))) {
        return false;
    }

    // Find where we hit the plane
    float T;
    Plane.Intersect(T, Start, End);

    if ((T < 0.0f) || (T > 1.0f)) {
        return false;
    }

    Vector3 HitPoint = Start + ((End - Start) * T);

    // See if hit point is inside tri
    Vector3 EdgeNormal;

    for (i = 0; i < 3; ++i) {
        EdgeNormal = Plane.Normal.Cross(Tri[(i + 1) % 3] - Tri[i]);
        if (EdgeNormal.Dot(HitPoint - Tri[i]) < -0.001f) {
            return false;
        }
    }

    // Collision
    FinalT = T;
    FinalHitPoint = HitPoint;
    return true;
}

//==============================================================================
bool ComputeRaySphereCollision(const Vector3& SpherePos,
                               const float    SphereRadius,
                               const Vector3& Start,
                               const Vector3& End,
                               float&         FinalT,
                               Vector3&       FinalHitPoint)
{
    //ASSERT( (Start-End).LengthSquared() > 0.000000001f );

    const Vector3 TestPoint = SpherePos.GetClosestPToLSeg(Start, End);
    const Vector3 Diff = TestPoint - SpherePos;
    const float   DistSQ = Diff.LengthSquared();
    const float   RadiusSQ = SphereRadius * SphereRadius;

    if (DistSQ < RadiusSQ) {
        // The point is inside the sphere, so find the point of contact
        float BackDist = sqrt(RadiusSQ - DistSQ);

        Vector3     Temp = TestPoint - Start;
        const float Length = Temp.Length();

        float DistToHitPoint = Length - BackDist;

        if (DistToHitPoint < 0) {
            DistToHitPoint = 0;
        }

        if (Length > 0) {
            Temp /= Length; // Normalize
        }

        Temp *= DistToHitPoint;

        FinalHitPoint = Start + Temp;
        if ((Start - End).LengthSquared() < 0.000000001f) {
            FinalT = 0.0000001f;
        } else {

            FinalT = DistToHitPoint / ((Start - End).Length());
        }
        if ((FinalT < 0.0f) || (FinalT > 1.0f)) {
            return false;
        }

        return true;
    } else {
        return false;
    }
}

//==============================================================================
bool ComputeSphereSphereCollision(const Vector3& TestSpherePos,
                                  const float    TestSphereRadius,
                                  const float    MovingSphereRadius,
                                  const Vector3& Start,
                                  const Vector3& End,
                                  float&         FinalT,
                                  Vector3&       FinalHitPoint)
{
    // Shrink the moving sphere to a ray, and grow the test sphere by the
    // moving sphere's radius. Then do a ray/sphere check
    float TempRadius = TestSphereRadius + MovingSphereRadius;

    if (!ComputeRaySphereCollision(TestSpherePos, TempRadius, Start, End, FinalT, FinalHitPoint)) {
        return false;
    }

    // they hit, so find the hit point. Final T will be correct
    Vector3 MovingSpherePos = FinalHitPoint;
    Vector3 Diff = TestSpherePos - MovingSpherePos;
    Diff.Normalize();
    Diff *= MovingSphereRadius;

    FinalHitPoint = MovingSpherePos + Diff;

    return true;
}

void BuildTrisForAABBox(const BBox& AABBox, Vector3 Tris[12][3])
{
    Vector3 Corners[8];

    Corners[0].set(AABBox.min.GetX(), AABBox.min.GetY(), AABBox.min.GetZ());
    Corners[1].set(AABBox.max.GetX(), AABBox.min.GetY(), AABBox.min.GetZ());
    Corners[2].set(AABBox.max.GetX(), AABBox.min.GetY(), AABBox.max.GetZ());
    Corners[3].set(AABBox.min.GetX(), AABBox.min.GetY(), AABBox.max.GetZ());
    Corners[4].set(AABBox.min.GetX(), AABBox.max.GetY(), AABBox.min.GetZ());
    Corners[5].set(AABBox.max.GetX(), AABBox.max.GetY(), AABBox.min.GetZ());
    Corners[6].set(AABBox.max.GetX(), AABBox.max.GetY(), AABBox.max.GetZ());
    Corners[7].set(AABBox.min.GetX(), AABBox.max.GetY(), AABBox.max.GetZ());

    Tris[0][0] = Corners[2];
    Tris[0][1] = Corners[7];
    Tris[0][2] = Corners[3];

    Tris[1][0] = Corners[2];
    Tris[1][1] = Corners[6];
    Tris[1][2] = Corners[7];

    Tris[2][0] = Corners[0];
    Tris[2][1] = Corners[3];
    Tris[2][2] = Corners[4];

    Tris[3][0] = Corners[3];
    Tris[3][1] = Corners[7];
    Tris[3][2] = Corners[4];

    Tris[4][0] = Corners[1];
    Tris[4][1] = Corners[0];
    Tris[4][2] = Corners[5];

    Tris[5][0] = Corners[5];
    Tris[5][1] = Corners[0];
    Tris[5][2] = Corners[4];

    Tris[6][0] = Corners[1];
    Tris[6][1] = Corners[6];
    Tris[6][2] = Corners[2];

    Tris[7][0] = Corners[1];
    Tris[7][1] = Corners[5];
    Tris[7][2] = Corners[6];

    Tris[8][0] = Corners[6];
    Tris[8][1] = Corners[4];
    Tris[8][2] = Corners[7];

    Tris[9][0] = Corners[6];
    Tris[9][1] = Corners[5];
    Tris[9][2] = Corners[4];

    Tris[10][0] = Corners[2];
    Tris[10][1] = Corners[3];
    Tris[10][2] = Corners[0];

    Tris[11][0] = Corners[2];
    Tris[11][1] = Corners[0];
    Tris[11][2] = Corners[1];
}

bool ComputeSphereAABBoxCollision(const BBox&    AABBox,
                                  const Vector3& Start,
                                  const Vector3& End,
                                  float          Radius,
                                  float&         FinalT,
                                  Vector3&       FinalHitPoint,
                                  plane&         FinalHitPlane,
                                  plane&         FinalSlipPlane)
{
    Vector3 Tris[12][3];
    BuildTrisForAABBox(AABBox, Tris);

    const int TRIS_IN_BOX = 12;
    const int MAX_COLLISIONS = 12;

    int     i;
    float   FinalTs[MAX_COLLISIONS];
    Vector3 FinalHitPoints[MAX_COLLISIONS];
    plane   FinalHitPlanes[MAX_COLLISIONS];
    plane   FinalSlipPlanes[MAX_COLLISIONS];
    int     nCollisions = 0;

    for (i = 0; i < TRIS_IN_BOX; ++i) {
        if (ComputeSphereTriCollision(
                Tris[i],
                Start,
                End,
                Radius,
                FinalTs[nCollisions],
                FinalHitPoints[nCollisions])) {
            {
                // Our slide plane is defined by the impact point, and a
                // normal from that point towards the sphere's center,
                // when the sphere is at the collision T
                Vector3 SphereCenterAtImpact = Start + (End - Start) * FinalTs[nCollisions];
                Vector3 Normal = SphereCenterAtImpact - FinalHitPoints[nCollisions];

                Normal.Normalize();
                FinalSlipPlanes[nCollisions].Setup(
                    FinalHitPoints[nCollisions], Normal);
            }

            FinalHitPlanes[nCollisions].Setup(
                Tris[i][0],
                Tris[i][1],
                Tris[i][2]);

            ++nCollisions;
        }
    }

    if (nCollisions > 0) {
        // we have a collision, find the one with the smallest final T
        FinalT = 2.0f; // greater than is valid initially

        for (i = 0; i < nCollisions; ++i) {
            if (FinalTs[i] < FinalT) {
                FinalT = FinalTs[i];
                FinalHitPoint = FinalHitPoints[i];
                FinalHitPlane = FinalHitPlanes[i];
                FinalSlipPlane = FinalSlipPlanes[i];
            }
        }

        if ((FinalT < 0.0f) || (FinalT > 1.0f)) {
            return false;
        }

        return true;
    }

    return false;
}

bool ComputeRayAABBoxCollision(const BBox&    AABBox,
                               const Vector3& Start,
                               const Vector3& End,
                               float&         FinalT,
                               Vector3&       FinalHitPoint,
                               plane&         FinalHitPlane,
                               plane&         FinalSlipPlane)
{
    Vector3 Tris[12][3];
    BuildTrisForAABBox(AABBox, Tris);

    const int TRIS_IN_BOX = 12;
    const int MAX_COLLISIONS = 12;

    int     i;
    float   FinalTs[MAX_COLLISIONS];
    Vector3 FinalHitPoints[MAX_COLLISIONS];
    plane   FinalHitPlanes[MAX_COLLISIONS];
    int     nCollisions = 0;

    for (i = 0; i < TRIS_IN_BOX; ++i) {
        if (ComputeRayTriCollision(
                Tris[i],
                Start,
                End,
                FinalTs[nCollisions],
                FinalHitPoints[nCollisions])) {
            FinalHitPlanes[nCollisions].Setup(
                Tris[i][0],
                Tris[i][1],
                Tris[i][2]);
            ++nCollisions;
        }
    }

    if (nCollisions > 0) {
        // we have a collision, find the one with the smallest final T
        FinalT = 2.0f; // greater than is valid initially

        for (i = 0; i < nCollisions; ++i) {
            if (FinalTs[i] < FinalT) {
                FinalT = FinalTs[i];
                FinalHitPoint = FinalHitPoints[i];
                FinalHitPlane = FinalHitPlanes[i];
                FinalSlipPlane = FinalHitPlanes[i];
            }
        }

        if ((FinalT < 0.0f) || (FinalT > 1.0f)) {
            return false;
        }

        return true;
    }

    return false;
}
