
#include "RigidGeomCollision.h"
#include "collisionMgr/CollisionMgr.h"
#include "collisionMgr/PolyCache.h"
#include "render/RigidGeom.h"
//#include "e_Draw.h"

#include <cassert>

#define IN_RANGE(a,v,b)     ( ((a) <= (v)) && ((v) <= (b)) )

#define INDEX_MASK 0x7FFF

inline void RigidGeom_SetPrimKey(int& PrimKey, bool bUseHighPoly, int iCluster, int iTriangle)
{
    PrimKey = (iCluster << 16) | (iTriangle);
    PrimKey |= (bUseHighPoly) ? (0x80000000) : (0x00000000);
}

inline void RigidGeom_GetPrimKey(int PrimKey, bool& bUseHighPoly, int& iCluster, int& iTriangle)
{
    assert(PrimKey != -1);
    bUseHighPoly = (PrimKey & 0x80000000) ? (true) : (false);
    iTriangle = (PrimKey & 0xFFFF);
    iCluster = (PrimKey >> 16) & 0x7FFF;
}

//===========================================================================

bool RigidGeom_GetTriangle(const RigidGeom* pRigidGeom,
                           int              Key,
                           Vector3&         P0,
                           Vector3&         P1,
                           Vector3&         P2)
{
    if (pRigidGeom == NULL) {
        return false;
    }

    assert(pRigidGeom->collision.numHighClusters);

    bool bUseHighPoly;
    int  iCluster;
    int  iTriangle;

    RigidGeom_GetPrimKey(Key, bUseHighPoly, iCluster, iTriangle);

    if (!IN_RANGE(0, iCluster, pRigidGeom->collision.numHighClusters - 1)) {
        return false;
    }
    CollisionData::HighCluster& Cluster = pRigidGeom->collision.highClusters[iCluster];
    RigidGeom::Dlist_PC&        DList = pRigidGeom->system.pPC[Cluster.iDList];

    if (!IN_RANGE(0, iTriangle, Cluster.nTris - 1)) {
        return false;
    }

    int Index = pRigidGeom->collision.highIndexToVert0[Cluster.iOffset + iTriangle];

    // We have arrived.  Fill out the information.

    P0 = DList.verts[DList.indices[Index + 0]].pos;
    P1 = DList.verts[DList.indices[Index + 1]].pos;
    P2 = DList.verts[DList.indices[Index + 2]].pos;

    return (true);
}

//===========================================================================

bool RigidGeom_GetColDetails(const RigidGeom*    pRigidGeom,
                             const Matrix4*      pL2W,
                             const void*         pColorIn,
                             int                 Key,
                             Object::detail_tri& Tri)
{
    const uint16_t* pColor = (uint16_t*)pColorIn;
    assert(pRigidGeom);
    assert(pRigidGeom->collision.numHighClusters);
    const CollisionData& Coll = pRigidGeom->collision;

    bool bUseHighPoly;
    int  iCluster;
    int  iTriangle;
    RigidGeom_GetPrimKey(Key, bUseHighPoly, iCluster, iTriangle);
    assert(bUseHighPoly);

    if (!IN_RANGE(0, iCluster, Coll.numHighClusters - 1)) {
        return false;
    }

    {
        CollisionData::HighCluster& Cluster = Coll.highClusters[iCluster];
        RigidGeom::Dlist_PC&        DList = pRigidGeom->system.pPC[Cluster.iDList];

        if (!IN_RANGE(0, iTriangle, Cluster.nTris - 1)) {
            return false;
        }

        int Index = Coll.highIndexToVert0[Cluster.iOffset + iTriangle] & INDEX_MASK;

        // We have arrived.  Fill out the information.
        const Matrix4& L2W = *pL2W;

        Tri.Vertex[0] = DList.verts[DList.indices[Index + 0]].pos;
        Tri.Vertex[1] = DList.verts[DList.indices[Index + 1]].pos;
        Tri.Vertex[2] = DList.verts[DList.indices[Index + 2]].pos;
        L2W.Transform(Tri.Vertex, Tri.Vertex, 3);

        if (!pColor) {
            Tri.Color[0] = 0xFFFFFFFF;
            Tri.Color[1] = 0xFFFFFFFF;
            Tri.Color[2] = 0xFFFFFFFF;
        } else {
            Tri.Color[0] = DList.verts[DList.indices[Index + 0]].colour;
            Tri.Color[1] = DList.verts[DList.indices[Index + 1]].colour;
            Tri.Color[2] = DList.verts[DList.indices[Index + 2]].colour;
        }

        Tri.Normal[0] = DList.verts[DList.indices[Index + 0]].normal;
        Tri.Normal[1] = DList.verts[DList.indices[Index + 1]].normal;
        Tri.Normal[2] = DList.verts[DList.indices[Index + 2]].normal;
        L2W.Transform(Tri.Normal, Tri.Normal, 3);
        Tri.Normal[0] -= L2W.GetTranslation();
        Tri.Normal[1] -= L2W.GetTranslation();
        Tri.Normal[2] -= L2W.GetTranslation();

        Tri.UV[0] = DList.verts[DList.indices[Index + 0]].uv;
        Tri.UV[1] = DList.verts[DList.indices[Index + 1]].uv;
        Tri.UV[2] = DList.verts[DList.indices[Index + 2]].uv;

        Tri.MaterialInfo = Cluster.materialInfo;

        return (true);
    }
}

//==============================================================================

bool ClipLineToBBoxWithSafeSpace(const BBox& bbox, const Vector3& P0, const Vector3& P1, float& T0, float& T1)
{
    Vector3 Dir = P1 - P0;
    float   tx_min, tx_max;
    float   ty_min, ty_max;
    float   tz_min, tz_max;
    float   t_min, t_max;

    Vector3 MinMinusP0 = bbox.min - P0;
    Vector3 MaxMinusP0 = bbox.max - P0;

    if (Dir.GetX() >= 0.0f) {
        t_min = tx_min = MinMinusP0.GetX() / Dir.GetX();
        t_max = tx_max = MaxMinusP0.GetX() / Dir.GetX();
    } else {
        t_min = tx_min = MaxMinusP0.GetX() / Dir.GetX();
        t_max = tx_max = MinMinusP0.GetX() / Dir.GetX();
    }

    if (Dir.GetY() >= 0.0f) {
        ty_min = MinMinusP0.GetY() / Dir.GetY();
        ty_max = MaxMinusP0.GetY() / Dir.GetY();
    } else {
        ty_min = MaxMinusP0.GetY() / Dir.GetY();
        ty_max = MinMinusP0.GetY() / Dir.GetY();
    }

    if (t_min > ty_max || ty_min > t_max) {
        return false;
    }

    if (t_min < ty_min) {
        t_min = ty_min;
    }
    if (t_max > ty_max) {
        t_max = ty_max;
    }

    if (Dir.GetZ() >= 0.0f) {
        tz_min = MinMinusP0.GetZ() / Dir.GetZ();
        tz_max = MaxMinusP0.GetZ() / Dir.GetZ();
    } else {
        tz_min = MaxMinusP0.GetZ() / Dir.GetZ();
        tz_max = MinMinusP0.GetZ() / Dir.GetZ();
    }

    if (t_min > tz_max || tz_min > t_max) {
        return false;
    }

    if (t_min < tz_min) {
        t_min = tz_min;
    }
    if (t_max > tz_max) {
        t_max = tz_max;
    }

    if (!((t_min <= 1.0f) && (t_max >= 0.0f))) {
        return false;
    }

    // Set the initial entry and exit points of the ray
    T0 = t_min;
    T1 = t_max;

    // Backup and push forward by 1cm.
    float RayLen = Dir.Length();
    float SafeT = 1.0f / RayLen;
    T0 -= SafeT;
    T1 += SafeT;
    if (T0 < 0) {
        T0 = 0;
    }
    if (T1 > 1) {
        T1 = 1;
    }

    return true;
}

//==============================================================================

bool IsPointInsideTri(const Vector3& P, const Vector3& TriP0, const Vector3& TriP1, const Vector3& TriP2, const Vector3& TriNormal)
{
    Vector3 EdgeNormal;

    EdgeNormal = TriNormal.Cross(TriP1 - TriP0);
    float PDist = EdgeNormal.Dot(P - TriP0);
    if ((PDist < 0.0f)) {
        return false;
    }

    EdgeNormal = TriNormal.Cross(TriP2 - TriP1);
    if (EdgeNormal.Dot(P - TriP1) < 0.0f) {
        return false;
    }

    EdgeNormal = TriNormal.Cross(TriP0 - TriP2);
    if (EdgeNormal.Dot(P - TriP2) < 0.0f) {
        return false;
    }

    return true;
}

void RigidGeom_RayVsPCHiPoly(guid             Guid,
                             const BBox&      WorldBBox,
                             const uint64_t   MeshMask,
                             const Matrix4*   pL2W,
                             const RigidGeom& RigidGeom)
{
    int k;

    //
    // Dig out Ray and clip to world bbox
    //
    Vector3 WorldRayStart;
    Vector3 WorldRayEnd;
    float   LocalRayT0;
    float   LocalRayT1;
    {

        const collision_mgr::dynamic_ray& Ray = g_CollisionMgr.GetDynamicRay();
        if (!ClipLineToBBoxWithSafeSpace(WorldBBox, Ray.Start, Ray.End, LocalRayT0, LocalRayT1)) {
            return;
        }

        Vector3 Direction = Ray.End - Ray.Start;
        WorldRayStart = Ray.Start + LocalRayT0 * (Direction);
        WorldRayEnd = Ray.Start + LocalRayT1 * (Direction);
    }

    // Which collision resolution to use?
    assert(g_CollisionMgr.IsUsingHighPoly());
    assert(RigidGeom.collision.numHighClusters);

    const CollisionData& Coll = RigidGeom.collision;

    // For every bone...
    int            iCurrentBone = -1;
    const Matrix4* pCurrentL2W = NULL;
    Vector3        LocalRayStart;
    Vector3        LocalRayEnd;
    Vector3        LocalRayDelta;
    BBox           LocalRayBBox;
    Vector3        ClusterRayStart;
    Vector3        ClusterRayEnd;
    Vector3        ClusterRayDelta;
    BBox           ClusterRayBBox;
    float          ClusterRayT0;
    float          ClusterRayT1;

    // Loop thru the clusters and apply them if:
    //  - mesh is active
    //  - cluster uses the current bone
    for (int iC = 0; iC < Coll.numHighClusters; iC++) {
        const CollisionData::HighCluster& Cluster = Coll.highClusters[iC];
        //
        // Do quick checks to see if we should be skipping this mesh or material
        //
        {
            uint64_t Bit = 1 << Cluster.iMesh;
            if (!(MeshMask & Bit)) {
                continue;
            }

            if ((Cluster.materialInfo.soundType == Object::MAT_TYPE_GLASS) &&
                (g_CollisionMgr.IsIgnoringGlass())) {
                continue;
            }
        }
        //
        // Update local ray info
        //
        if (Cluster.iBone != iCurrentBone) {
            iCurrentBone = Cluster.iBone;

            // Get ptr to the current matrix
            pCurrentL2W = &pL2W[iCurrentBone];

            // Transform ray into space of bone.  Rather than InvertRT the matrix
            // we're just going to rework the transform.
            Vector3 Translation = pCurrentL2W->GetTranslation();
            Vector3 T, C1, C2, C3;
            pCurrentL2W->GetColumns(C1, C2, C3);

            T = WorldRayStart - Translation;
            LocalRayStart.x = T.Dot(C1);
            LocalRayStart.y = T.Dot(C2);
            LocalRayStart.z = T.Dot(C3);

            T = WorldRayEnd - Translation;
            LocalRayEnd.x = T.Dot(C1);
            LocalRayEnd.y = T.Dot(C2);
            LocalRayEnd.z = T.Dot(C3);

            LocalRayDelta = LocalRayEnd - LocalRayStart;
            LocalRayBBox.Set(LocalRayStart, LocalRayEnd);
            LocalRayBBox.Inflate(1, 1, 1);
        }

        //
        // Check if local ray bbox intersects cluster bbox and if ray intersects
        //
        {
            if (!LocalRayBBox.Intersect(Cluster.bbox)) {
                continue;
            }

            if (!ClipLineToBBoxWithSafeSpace(Cluster.bbox, LocalRayStart, LocalRayEnd, ClusterRayT0, ClusterRayT1)) {
                continue;
            }

            ClusterRayStart = LocalRayStart + ClusterRayT0 * LocalRayDelta;
            ClusterRayEnd = LocalRayStart + ClusterRayT1 * LocalRayDelta;
            ClusterRayDelta = ClusterRayEnd - ClusterRayStart;
            ClusterRayBBox.Set(ClusterRayStart, ClusterRayEnd);
        }

        // Apply the cluster to the collision manager.
        const RigidGeom::Dlist_PC& DList = RigidGeom.system.pPC[Cluster.iDList];
        //        extern bool COLL_DISPLAY_OBJECTS;
        //        if( COLL_DISPLAY_OBJECTS )
        //            x_DebugMsg("CLUSTER %4d\n",Cluster.nTris);

        // 0.133
        for (k = 0; k < Cluster.nTris; k++) {

            const uint32_t Offset = Coll.highIndexToVert0[Cluster.iOffset + k] & INDEX_MASK;
            //
            // Setup ptrs to positions
            //
            const Vector3* P0;
            const Vector3* P1;
            const Vector3* P2;
            P0 = (Vector3*)(&DList.verts[DList.indices[Offset + 0]].pos);
            P1 = (Vector3*)(&DList.verts[DList.indices[Offset + 1]].pos);
            P2 = (Vector3*)(&DList.verts[DList.indices[Offset + 2]].pos);

            if (ClusterRayBBox.IntersectTriBBox(*P0, *P1, *P2)) {
                Vector3 PlaneNormal;
                float   PlaneD;

                Vector3 P1MinusP0 = (*P1) - (*P0);
                Vector3 P2MinusP0 = (*P2) - (*P0);

                // Calculate non-normalized normal vector
                PlaneNormal = v3_Cross(P1MinusP0, P2MinusP0);

                float PlaneNormalDotClusterRayStartPlusPlaneD;
                float PlaneNormalDotClusterRayDelta;
                float PlaneIntersectT;

                bool bDoubleSided = !!(Cluster.materialInfo.flags & CollisionData::MatInfo::FLAG_DOUBLESIDED);

                if (!bDoubleSided) {
                    // Check if we are moving away from the triangle
                    PlaneNormalDotClusterRayDelta = PlaneNormal.Dot(ClusterRayDelta);
                    if (PlaneNormalDotClusterRayDelta >= 0) {
                        continue;
                    }

                    // Compute PlaneD
                    PlaneD = -PlaneNormal.Dot(*P0);

                    // Check if ray end is in front of triangle
                    if (PlaneNormal.Dot(ClusterRayEnd) + PlaneD > 0) {
                        continue;
                    }

                    // Check if ray start is behind triangle
                    PlaneNormalDotClusterRayStartPlusPlaneD = PlaneNormal.Dot(ClusterRayStart) + PlaneD;
                    if (PlaneNormalDotClusterRayStartPlusPlaneD < 0) {
                        continue;
                    }

                    PlaneIntersectT = -PlaneNormalDotClusterRayStartPlusPlaneD / PlaneNormalDotClusterRayDelta;
                } else {
                    PlaneNormalDotClusterRayDelta = PlaneNormal.Dot(ClusterRayDelta);
                    PlaneD = -PlaneNormal.Dot(*P0);
                    PlaneNormalDotClusterRayStartPlusPlaneD = PlaneNormal.Dot(ClusterRayStart) + PlaneD;
                    PlaneIntersectT = -PlaneNormalDotClusterRayStartPlusPlaneD / PlaneNormalDotClusterRayDelta;

                    // Check if intersection is not in the interval of the test
                    if ((PlaneIntersectT < 0) || (PlaneIntersectT > 1)) {
                        continue;
                    }
                }

                Vector3 HP = ClusterRayStart + PlaneIntersectT * ClusterRayDelta;

                // Confirm that intersection point is inside triangle bounds.  The order
                // of testing is a bit twisted to make use of data already available.
                // For the first two edgenormals we are also testing the intersection point
                // against the opposite point boundary.
                float   HPDist;
                Vector3 EdgeNormal;
                Vector3 HPMinusP0 = HP - (*P0);

                // Test P0->P1 edge
                EdgeNormal = PlaneNormal.Cross(P1MinusP0);
                HPDist = EdgeNormal.Dot(HPMinusP0);
                if ((HPDist < 0.0f) || (HPDist > EdgeNormal.Dot(P2MinusP0))) {
                    continue;
                }

                // Test P0->P2 edge
                EdgeNormal = PlaneNormal.Cross(P2MinusP0);
                HPDist = EdgeNormal.Dot(HPMinusP0);
                if ((HPDist > 0.0f) || (HPDist < EdgeNormal.Dot(P1MinusP0))) {
                    continue;
                }

                // Test P1->P2 edge
                EdgeNormal = PlaneNormal.Cross((*P2) - (*P1));
                if (EdgeNormal.Dot(HP - (*P1)) < 0.0f) {
                    continue;
                }

                {
                    // Compute the Primitive Key
                    int PrimKey;
                    RigidGeom_SetPrimKey(PrimKey, true, iC, k);

                    // Transform back into worldspace
                    plane TrianglePlane(*P0, *P1, *P2);
                    TrianglePlane.Transform(*pCurrentL2W);
                    HP = *pCurrentL2W * HP;

                    // Work back and solve what the T value should be for the original ray
                    float T = PlaneIntersectT;
                    T = ClusterRayT0 + T * (ClusterRayT1 - ClusterRayT0);
                    T = LocalRayT0 + T * (LocalRayT1 - LocalRayT0);

                    // Build collision entry
                    collision_mgr::collision TempCollision(
                        T,
                        HP,
                        TrianglePlane,
                        TrianglePlane,
                        Guid,
                        PrimKey,
                        PRIMITIVE_STATIC_TRIANGLE,
                        false,
                        std::max(P0->GetY(), std::max(P1->GetY(), P2->GetY())),
                        Cluster.materialInfo.soundType);

                    // Handle it off to the collision mgr
                    g_CollisionMgr.RecordCollision(TempCollision);

                    // If we are doing LOS then we are done!
                    if (g_CollisionMgr.IsStopOnFirstCollision()) {
                        return;
                    }
                }
            }
        }
    }
}

void RigidGeom_ApplyCollision(guid             Guid,
                              const BBox&      bbox,
                              const uint64_t   MeshMask,
                              const Matrix4*   pL2W,
                              const RigidGeom* pRigidGeom)
{
    int i, j, k;

    //
    // If we made it to here we should only be checking high poly!
    //
    assert(g_CollisionMgr.IsUsingHighPoly());

    //
    // Is the geometry available?  If not, apply bbox
    //
    if (pRigidGeom == nullptr) {
        g_CollisionMgr.StartApply(Guid);
        g_CollisionMgr.ApplyAABBox(bbox);
        g_CollisionMgr.EndApply();
        return;
    }

    const RigidGeom& RigidGeom = *pRigidGeom;

    //
    // Did we compile high poly collision?
    //
    if (RigidGeom.collision.numHighClusters == 0) {
        return;
    }

    //
    // Check for Ray vs. PC High poly
    //
    if (g_CollisionMgr.IsUsingHighPoly() &&
        ((g_CollisionMgr.GetDynamicPrimitive() == PRIMITIVE_DYNAMIC_RAY) ||
         (g_CollisionMgr.GetDynamicPrimitive() == PRIMITIVE_DYNAMIC_LOS))) {
        RigidGeom_RayVsPCHiPoly(Guid, bbox, MeshMask, pL2W, *pRigidGeom);
        return;
    }

    // Which collision resolution to use?
    if (g_CollisionMgr.IsUsingHighPoly()) {
        assert(RigidGeom.collision.numHighClusters);
        const CollisionData& Coll = RigidGeom.collision;

        // For every bone...
        for (i = 0; i < RigidGeom.numBones; i++) {
            // Compute the inverse matrix.
            const Matrix4& L2W = pL2W[i];
            const Matrix4  W2L = m4_InvertRT(L2W);

            g_CollisionMgr.StartApply(Guid, L2W, W2L);

            // Loop thru the clusters and apply them if:
            //  - mesh is active
            //  - cluster uses the current bone

            for (j = 0; j < Coll.numHighClusters; j++) {
                const CollisionData::HighCluster& Cluster = Coll.highClusters[j];

                if ((Cluster.materialInfo.soundType == Object::MAT_TYPE_GLASS) &&
                    (g_CollisionMgr.IsIgnoringGlass())) {
                    continue;
                }

                bool bDoubleSided = !!(Cluster.materialInfo.flags & CollisionData::MatInfo::FLAG_DOUBLESIDED);

                if (Cluster.iBone != i) {
                    continue;
                }

                uint64_t Bit = 1 << Cluster.iMesh;
                if (!(MeshMask & Bit)) {
                    continue;
                }

                {
                    // Apply the cluster to the collision manager.
                    const RigidGeom::Dlist_PC& DList = RigidGeom.system.pPC[Cluster.iDList];

                    for (k = 0; k < Cluster.nTris; k++) {
                        const uint32_t Offset = Coll.highIndexToVert0[Cluster.iOffset + k] & INDEX_MASK;

                        int PrimKey;
                        RigidGeom_SetPrimKey(PrimKey, true, j, k);

                        g_CollisionMgr.ApplyTriangle(
                            DList.verts[DList.indices[Offset + 0]].pos,
                            DList.verts[DList.indices[Offset + 1]].pos,
                            DList.verts[DList.indices[Offset + 2]].pos,
                            Cluster.materialInfo.soundType,
                            PrimKey);

                        // Double Sided single plane so apply collision for both sides!
                        if (bDoubleSided) {
                            g_CollisionMgr.ApplyTriangle(
                                DList.verts[DList.indices[Offset + 2]].pos,
                                DList.verts[DList.indices[Offset + 1]].pos,
                                DList.verts[DList.indices[Offset + 0]].pos,
                                Cluster.materialInfo.soundType,
                                PrimKey);
                        }
                    }
                }
            }

            g_CollisionMgr.EndApply();
        }
    }
}

void RigidGeom_GatherToPolyCache(guid             Guid,
                                 const BBox&      bbox,
                                 uint64_t         MeshMask,
                                 const Matrix4*   pL2W,
                                 const RigidGeom* pRigidGeom)
{
    if ((pRigidGeom == nullptr) || (pRigidGeom->collision.numLowClusters == 0)) {
        return;
    }

    g_PolyCache.GatherCluster(pRigidGeom->collision, pL2W, MeshMask, Guid);
}
