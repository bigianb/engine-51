#pragma once

#include "../collisionMgr/PolyCache.h"

class rigid_body;
class collision_shape;

class collider
{
public:
    enum defines
    {
        MAX_COLLISIONS = 5,
        MAX_CLUSTERS = 512,
    };

    // Collision
    struct collision
    {
        // These must be filled out
        float   m_Depth;  // Penetration depth
        Vector3 m_Point;  // World position
        Vector3 m_Normal; // World normal
    };

    //==============================================================================
    // Public functions
    //==============================================================================
public:
    // Constructor/destructor
    collider();
    ~collider();

    // Clears internal collisions
    void ClearCollisions();

    // Reports collisions to physics manager
    int ReportCollisions(collision_shape* pColl0, collision_shape* pColl1);

    // Adds to collision list
    void RecordCollision(const collision& Collision);

    // Checks collision between 2 moving shapes
    int Check(collision_shape* pColl0, collision_shape* pColl1);

    // Collects world polys of BBox and returns # of clusters
    int CollectWorldPolys(const BBox& WorldBBox);

    //==============================================================================
    // Private functions
    //==============================================================================
private:
    // Returns TRUE if sphere intersects NGon
    bool SphereNGonIntersection(const plane&   Plane,
                                const Vector3* Verts,
                                const int      nVerts,
                                const Vector3& SpherePos,
                                const float    SphereRadius);

    // Returns TRUE if sphere intersects any of world
    bool SphereWorldIntersection(const Vector3& SpherePos, const float SphereRadius);

    // Check sweeping sphere V NGon collision
    bool SphereNGonCollision(const plane&   Plane,
                             const Vector3* Verts,
                             const int      nVerts,
                             const Vector3& SphereStart,
                             const Vector3& SphereEnd,
                             const float    SphereRadius,
                             float&         CollT,
                             plane&         CollPlane);

    // Checks sweeping sphere V world collision
    bool SphereWorldCollision(const Vector3& SphereStartPos,
                              const Vector3& SphereEndPos,
                              const float    SphereRadius,
                              float&         CollT,
                              plane&         CollPlane);

    // Checks static sphere V sphere collision
    void SphereSphereCollision(const Vector3& SpherePos0,
                               const float    SphereRadius0,
                               const Vector3& SpherePos1,
                               const float    SphereRadius1);

    // Checks static sphere V sphere collision
    void SphereCapsuleCollision(const Vector3& SpherePos,
                                const float    SphereRadius,
                                const Vector3& CapsuleStartPos,
                                const Vector3& CapsuleEndPos,
                                const float    CapsuleRadius);

    // Checks static capsule V capsule collision
    void CapsuleCapsuleCollision(const Vector3& CapsuleStartPos0,
                                 const Vector3& CapsuleEndPos0,
                                 const float    CapsuleRadius0,
                                 const Vector3& CapsuleStartPos1,
                                 const Vector3& CapsuleEndPos1,
                                 const float    CapsuleRadius1);

public:
    // Checks collision between swept spheres and world
    void CheckSpheresWorld(collision_shape* pColl0, collision_shape* pColl1);

    // Checks collision between 2 spheres lists
    void CheckSpheresSpheres(collision_shape* pColl0, collision_shape* pColl1);

    // Checks collision between spheres and capsule
    void CheckSpheresCapsule(collision_shape* pColl0, collision_shape* pColl1);

    // Checks collision between 2 capsules
    void CheckCapsuleCapsule(collision_shape* pColl0, collision_shape* pColl1);

protected:
    // Deepest collisions
    collision_shape* m_pMovingColl;
    collision        m_Collisions[MAX_COLLISIONS];
    int              m_nMaxCollisions; // Dynamic max collisions

    // Poly cache info
    poly_cache::cluster* m_pClusters[MAX_CLUSTERS];
    int                  m_nClusters;
    BBox                 m_ClusterListBBox;
};

extern collider g_Collider;
