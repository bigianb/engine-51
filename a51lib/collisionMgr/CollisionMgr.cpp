
#include "CollisionMgr.h"
//#include "x_stdio.hpp"
#include "CollisionPrimatives.h"
#include "../objectManager/ObjectManager.h"
//#include "Entropy.hpp"
#include "../RigidGeomCollision.h"
//#include "GameLib\StatsMgr.hpp"
#include "../PlaysurfaceMgr.h"
#include "PolyCache.h"


collision_mgr g_CollisionMgr;

bool FORCE_LOW_POLY_LOS = true;

void collision_mgr::InitializeCollisionCheckDefaults()
{
    m_nCollisions = 0;
    m_DynamicPrimitive = PRIMITIVE_INVALID;
    m_nMaxCollisions = 1;

    m_bUseIgnoreList = false;
    m_nIgnoredObjects = 0;

    m_bCollectPermeable = false;
    m_nPermeables = 0;

    m_bUseLowPoly = false;
    m_bIgnoreGlass = false;
    m_bStopOnFirstCollisionFound = false;
    m_bIsRayCheck = false;
    m_bIsEditorSelectRay = false;

    m_ContextInfo.Context = 0;
    m_ContextInfo.Guid = 0;

    m_bApplyStarted = false;
    m_bNotifyingPermeables = false;

    m_bRemoveDuplicateGuids = true;
    m_bUsePolyCache = false;
}


int   RAYBBOX_TEST = 0;
int   RAYBBOX_CULL = 0;
bool COLL_DISPLAY_OBJECTS = false;
bool collision_mgr::fn_CheckCollisions(Object::type ThisType /*= TYPE_ALL_TYPES*/,
                                        uint32_t          TheseAttributes /*= ATTR_COLLIDABLE */,
                                        uint32_t          NotTheseAttributes /*= 0x00000000*/)
{

    m_FilterThisType = ThisType;
    m_FilterTheseAttributes = TheseAttributes;
    m_FilterNotTheseAttributes = NotTheseAttributes;

    if (FORCE_LOW_POLY_LOS && (m_DynamicPrimitive == PRIMITIVE_DYNAMIC_LOS)) {
        m_bUseLowPoly = true;
    }

    // Turn on polycache if lowpoly
    if (m_bUseLowPoly) {
        m_bUsePolyCache = true;
    }

    // Shut off polycache if looking for permeables
    if (m_FilterTheseAttributes & Object::ATTR_COLLISION_PERMEABLE) {
        m_bUsePolyCache = false;
    }

    //if( COLL_DISPLAY_OBJECTS )
    //    x_DebugMsg("---------------------------------------\n");

    //
    // If using polycache let specialized functions handle it
    //
    if (m_bUsePolyCache) {
        // Confirm Low polys
        assert(m_bUseLowPoly);

        if (m_DynamicPrimitive == PRIMITIVE_DYNAMIC_CYLINDER) {
            ApplyCylinderToPolyCache();
        }

        if (m_DynamicPrimitive == PRIMITIVE_DYNAMIC_SPHERE) {
            ApplySphereToPolyCache();
        }

        if (m_DynamicPrimitive == PRIMITIVE_DYNAMIC_LOS) {
            ApplyRayToPolyCache();
        }

        if (m_DynamicPrimitive == PRIMITIVE_DYNAMIC_RAY) {
            ApplyRayToPolyCache();
        }
    } else {
        bool bEarlyFinish = false;

        //
        // Playsurfaces are special, handle them now
        //
        if ((ThisType == Object::TYPE_ALL_TYPES) || (ThisType == Object::TYPE_PLAY_SURFACE)) {
            if (m_bIsRayCheck) {
                playsurfaceMgr->CollectSurfaces(m_RayInfo[0].Start, m_RayInfo[0].End, TheseAttributes, NotTheseAttributes);
            } else {
                playsurfaceMgr->CollectSurfaces(m_DynamicBBoxes[m_ContextInfo.Context], TheseAttributes, NotTheseAttributes);
            }

            Surface* pSurface = playsurfaceMgr->GetNextSurface();
            while (pSurface != nullptr) {
                Surface* pS = pSurface;
                guid                      SurfaceGuid = playsurfaceMgr->GetPlaySurfaceGuid();
                pSurface = playsurfaceMgr->GetNextSurface();

                //
                // Do a ray check against bbox for early culling
                //
                if (m_bIsRayCheck) {
                    float T;
                    RAYBBOX_TEST++;
                    if (pS->WorldBBox.Intersect(T, m_RayInfo[0].Start, m_RayInfo[0].End) == false) {
                        RAYBBOX_CULL++;
                        continue;
                    }
                }

                RigidGeom* pGeom = (RigidGeom*)render::GetGeom(pS->RenderInst);

                RigidGeom_ApplyCollision(SurfaceGuid,
                                         pS->WorldBBox,
                                         (uint64_t)-1,
                                         &pS->L2W,
                                         pGeom);

                // If we are doing LOS then we might be done!
                if (m_bStopOnFirstCollisionFound && (m_nCollisions > 0)) {
                    bEarlyFinish = true;
                    break;
                }
            }
        }

        //
        // Check other objects if not finished
        //
        if (!bEarlyFinish) {
            if (m_bIsRayCheck) {
                objectManager->SelectRay(TheseAttributes, m_RayInfo[0].Start, m_RayInfo[0].End, ThisType, NotTheseAttributes);
            } else {
                objectManager->SelectBBox(TheseAttributes, m_DynamicBBoxes[0], ThisType, NotTheseAttributes);
            }

            for (slot_id aID = objectManager->StartLoop(); aID != SLOT_NULL; aID = objectManager->GetNextResult(aID)) {
                if (m_MovingObjGuid != objectManager->GetObjectBySlot(aID)->GetGuid() &&
                    !IsInIgnoreList(objectManager->GetObjectBySlot(aID)->GetGuid())) {

                    Object& object = *objectManager->GetObjectBySlot(aID);

                    /// here we make sure the object is collidable or else we just move on.
                    if ((object.GetAttrBits() & Object::ATTR_COLLIDABLE) != Object::ATTR_COLLIDABLE) {
                        continue;
                    }
                    //
                    // Do a ray check against bbox for early culling (Handled by SelectRay())
                    //

                    object.OnColCheck();

                    // If we are doing LOS then we might be done!
                    if (m_bStopOnFirstCollisionFound && (m_nCollisions > 0)) {
                        bEarlyFinish = true;
                        break;
                    }
                }
            }

            objectManager->EndLoop();
        }
    }

    //
    // sort the collisions by T
    //
    SortCollisions();

    //
    // clean permeables
    //
    if (m_bCollectPermeable) {
        CleanPermeables();
    }


    return (m_nCollisions > 0);
}

//==============================================================================
extern int GetNLogicLoops(void);


void collision_mgr::StartApply(guid           Guid,
                               const Matrix4& L2W,
                               const Matrix4& W2L)
{
    assert(!m_bApplyStarted); // EndApply() wasn't called

    //
    // Object is colliding against self
    //
    assert((Guid == 0) || (Guid != m_MovingObjGuid));

    //
    // Set up context info
    //
    m_bApplyStarted = true;
    m_ContextInfo.Context = 1;
    m_ContextInfo.Guid = Guid;
    m_ContextInfo.L2W = L2W;
    m_ContextInfo.W2L = W2L;

    //
    // Transform the dynamic primitive from world to local space
    //
    switch (m_DynamicPrimitive) {
    case PRIMITIVE_DYNAMIC_CYLINDER:
    {
        // transform start/end bottom/top
        m_CylinderInfo[1].BotStart = W2L.Transform(m_CylinderInfo[0].BotStart);
        m_CylinderInfo[1].BotEnd = W2L.Transform(m_CylinderInfo[0].BotEnd);
        m_CylinderInfo[1].TopStart = W2L.Transform(m_CylinderInfo[0].TopStart);
        m_CylinderInfo[1].TopEnd = W2L.Transform(m_CylinderInfo[0].TopEnd);

        // transform radius
        const Vector3 Scale = W2L.GetScale();
        assert(abs(Scale.x - 1.0f) < 0.001f);

        m_CylinderInfo[1].Radius = m_CylinderInfo[0].Radius * Scale.x;
        m_CylinderInfo[1].Height = m_CylinderInfo[0].Height * Scale.x;

        // transform spheres
        int i;
        m_CylinderInfo[1].nStartSpheres = m_CylinderInfo[0].nStartSpheres;
        m_CylinderInfo[1].nEndSpheres = m_CylinderInfo[0].nEndSpheres;
        for (i = 0; i < m_CylinderInfo[1].nStartSpheres; ++i) {
            m_CylinderInfo[1].StartSpherePositions[i] = W2L.Transform(m_CylinderInfo[0].StartSpherePositions[i]);
        }

        for (i = 0; i < m_CylinderInfo[1].nEndSpheres; ++i) {
            m_CylinderInfo[1].EndSpherePositions[i] = W2L.Transform(m_CylinderInfo[0].EndSpherePositions[i]);
        }

        break;
    }

    case PRIMITIVE_DYNAMIC_SPHERE:
    {
        m_SphereInfo[1].Start = W2L.Transform(m_SphereInfo[0].Start);
        m_SphereInfo[1].End = W2L.Transform(m_SphereInfo[0].End);

        const Vector3 Scale = W2L.GetScale();
        assert((abs(Scale.x - Scale.y) < 0.001f) &&
               (abs(Scale.y - Scale.z) < 0.001f));

        m_SphereInfo[1].Radius = m_SphereInfo[0].Radius * Scale.x;
        break;
    }

    case PRIMITIVE_DYNAMIC_RAY:
    case PRIMITIVE_DYNAMIC_LOS:
        m_RayInfo[1].Start = W2L.Transform(m_RayInfo[0].Start);
        m_RayInfo[1].End = W2L.Transform(m_RayInfo[0].End);
        break;

    default:
        assert(0); // invalid dynamic primitive
    }

    //
    // Transform the dynamic bbox
    //
    m_DynamicBBoxes[1] = m_DynamicBBoxes[0];
    m_DynamicBBoxes[1].Transform(W2L);
}

//==============================================================================

void collision_mgr::CylinderSetup(
    guid           MovingObjGuid,
    const Vector3& WorldStart,
    const Vector3& WorldEnd,
    float            Radius,
    float            Height)
{
    //LOG_STAT(k_stats_Collision);

    // Initialize
    InitializeCollisionCheckDefaults();
    UseLowPoly();

    m_DynamicPrimitive = PRIMITIVE_DYNAMIC_CYLINDER;
    m_MovingObjGuid = MovingObjGuid;

    m_CylinderInfo[0].BotStart = WorldStart;
    m_CylinderInfo[0].BotEnd = WorldEnd;
    m_CylinderInfo[0].TopStart = WorldStart + Vector3(0, Height, 0);
    m_CylinderInfo[0].TopEnd = WorldEnd + Vector3(0, Height, 0);
    m_CylinderInfo[0].Radius = Radius;
    m_CylinderInfo[0].Height = Height;

    //--------------------------------------------------
    // Find the bbox around the motion
    //--------------------------------------------------

    // Set up two arrays of spheres, representing the
    // cylinder at the start, and the cylinder at the
    // end.
    m_CylinderInfo[0].nStartSpheres = GetCylinderSpherePositions(
        m_CylinderInfo[0].BotStart,
        m_CylinderInfo[0].TopStart,
        m_CylinderInfo[0].Radius,
        m_CylinderInfo[0].StartSpherePositions,
        MAX_NUM_SPHERES);

    // SB 2/21/05:
    // Compute end sphere positions from start positions + movement delta since
    // calling GetCylinderSpherePositions() with the BotEnd and TopEnd
    // positions can return a different sphere count due to float precision.
    Vector3 MoveDelta = WorldEnd - WorldStart;
    int     nSpheres = m_CylinderInfo[0].nStartSpheres;
    for (int i = 0; i < nSpheres; i++) {
        m_CylinderInfo[0].EndSpherePositions[i] = m_CylinderInfo[0].StartSpherePositions[i] + MoveDelta;
    }
    m_CylinderInfo[0].nEndSpheres = nSpheres;

    // Check
    assert(m_CylinderInfo[0].nStartSpheres    // Something must be wrong with
           == m_CylinderInfo[0].nEndSpheres); // GetCylinderSpherePositions()
                                              // or the caller has changed the
                                              // size of the cylinder during
                                              // the move

    const int StartLast = m_CylinderInfo[0].nStartSpheres - 1;
    const int EndLast = m_CylinderInfo[0].nEndSpheres - 1;

    m_DynamicBBoxes[0].min = m_CylinderInfo[0].StartSpherePositions[0];
    m_DynamicBBoxes[0].max = m_CylinderInfo[0].StartSpherePositions[0];
    m_DynamicBBoxes[0].min.Min(m_CylinderInfo[0].StartSpherePositions[StartLast]);
    m_DynamicBBoxes[0].max.Max(m_CylinderInfo[0].StartSpherePositions[StartLast]);
    m_DynamicBBoxes[0].min.Min(m_CylinderInfo[0].EndSpherePositions[0]);
    m_DynamicBBoxes[0].max.Max(m_CylinderInfo[0].EndSpherePositions[0]);
    m_DynamicBBoxes[0].min.Min(m_CylinderInfo[0].EndSpherePositions[EndLast]);
    m_DynamicBBoxes[0].max.Max(m_CylinderInfo[0].EndSpherePositions[EndLast]);

    Vector3 vRadius(Radius, Radius, Radius);
    m_DynamicBBoxes[0].min -= vRadius;
    m_DynamicBBoxes[0].max += vRadius;
}

//==============================================================================

void collision_mgr::SphereSetup(
    guid           MovingObjGuid,
    const Vector3& WorldStart,
    const Vector3& WorldEnd,
    float            Radius)
{
    //LOG_STAT(k_stats_Collision);

    // Initialize
    InitializeCollisionCheckDefaults();

    m_DynamicPrimitive = PRIMITIVE_DYNAMIC_SPHERE;
    m_MovingObjGuid = MovingObjGuid;
    m_SphereInfo[0].Start = WorldStart;
    m_SphereInfo[0].End = WorldEnd;
    m_SphereInfo[0].Radius = Radius;

    Vector3 vRadius(Radius, Radius, Radius);
    m_DynamicBBoxes[0].min = WorldStart;
    m_DynamicBBoxes[0].min.Min(WorldEnd);
    m_DynamicBBoxes[0].min -= vRadius;

    m_DynamicBBoxes[0].max = WorldStart;
    m_DynamicBBoxes[0].max.Max(WorldEnd);
    m_DynamicBBoxes[0].max += vRadius;
}

//==============================================================================

void collision_mgr::RaySetup(
    guid           MovingObjGuid,
    const Vector3& WorldStart,
    const Vector3& WorldEnd)
{
    //LOG_STAT(k_stats_Collision);

    // Initialize
    InitializeCollisionCheckDefaults();

    m_DynamicPrimitive = PRIMITIVE_DYNAMIC_RAY;
    m_MovingObjGuid = MovingObjGuid;
    m_RayInfo[0].Start = WorldStart;
    m_RayInfo[0].End = WorldEnd;

    m_DynamicBBoxes[0].min = WorldStart;
    m_DynamicBBoxes[0].min.Min(WorldEnd);
    m_DynamicBBoxes[0].max = WorldStart;
    m_DynamicBBoxes[0].max.Max(WorldEnd);

    m_bIsRayCheck = true;
}

//==============================================================================

void collision_mgr::EditorSelectRaySetup(
    const Vector3& WorldStart,
    const Vector3& WorldEnd)
{
    //LOG_STAT(k_stats_Collision);

    // Initialize
    RaySetup(0, WorldStart, WorldEnd);

    // Record that it's an editor select ray
    m_bIsEditorSelectRay = true;
}

//==============================================================================

void collision_mgr::LineOfSightSetup(
    guid           MovingObjGuid,
    const Vector3& WorldStart,
    const Vector3& WorldEnd)
{
    //LOG_STAT(k_stats_Collision);

    // Initialize
    RaySetup(MovingObjGuid, WorldStart, WorldEnd);

    m_DynamicPrimitive = PRIMITIVE_DYNAMIC_LOS;

    StopOnFirstCollisionFound();
    IgnoreGlass();
}

//==============================================================================

void collision_mgr::ApplySphere(
    const Vector3& WorldPos,
    float            Radius,
    uint32_t            Flags,
    int            PrimitiveKey /* = 0 */)
{

    //LOG_STAT(k_stats_Collision);
    assert(m_bApplyStarted);

    // check to see if the bboxes intersect
    Vector3 vRadius(Radius, Radius, Radius);
    BBox    bb(WorldPos - vRadius, WorldPos + vRadius);
    if (!bb.Intersect(m_DynamicBBoxes[m_ContextInfo.Context])) {
        return;
    }

    // what's our moving primitive?
    switch (m_DynamicPrimitive) {
    case PRIMITIVE_DYNAMIC_CYLINDER:
        ApplySphereToCylinder(WorldPos, Radius, Flags, PrimitiveKey);
        break;
    case PRIMITIVE_DYNAMIC_SPHERE:
        ApplySphereToSphere(WorldPos, Radius, Flags, PrimitiveKey);
        break;
    case PRIMITIVE_DYNAMIC_RAY:
    case PRIMITIVE_DYNAMIC_LOS:
        ApplySphereToRay(WorldPos, Radius, Flags, PrimitiveKey);
        break;
    default:
        assert(0); // invalid primitive
    }
}

//==============================================================================

void collision_mgr::ApplyTriangle(
    const Vector3& P0,
    const Vector3& P1,
    const Vector3& P2,
    uint32_t            Flags, /* = 0 */
    int            PrimitiveKey /* = 0 */)

{
    assert(m_bApplyStarted);

    const BBox& DBox = m_DynamicBBoxes[m_ContextInfo.Context];

    if (P0.x < DBox.min.x && P1.x < DBox.min.x && P2.x < DBox.min.x) {
        return;
    }
    if (P0.y < DBox.min.y && P1.y < DBox.min.y && P2.y < DBox.min.y) {
        return;
    }
    if (P0.z < DBox.min.z && P1.z < DBox.min.z && P2.z < DBox.min.z) {
        return;
    }
    if (P0.x > DBox.max.x && P1.x > DBox.max.x && P2.x > DBox.max.x) {
        return;
    }
    if (P0.y > DBox.max.y && P1.y > DBox.max.y && P2.y > DBox.max.y) {
        return;
    }
    if (P0.z > DBox.max.z && P1.z > DBox.max.z && P2.z > DBox.max.z) {
        return;
    }

    // what's our moving primitive?
    switch (m_DynamicPrimitive) {
    case PRIMITIVE_DYNAMIC_CYLINDER:
        ApplyTriangleToCylinder(P0, P1, P2, Flags, PrimitiveKey);
        break;
    case PRIMITIVE_DYNAMIC_SPHERE:
        ApplyTriangleToSphere(P0, P1, P2, Flags, PrimitiveKey);
        break;
    case PRIMITIVE_DYNAMIC_RAY:
    case PRIMITIVE_DYNAMIC_LOS:
        ApplyTriangleToRay(P0, P1, P2, Flags, PrimitiveKey);
        break;
    default:
        assert(0); // invalid primitive
    }
}

//==============================================================================

void collision_mgr::ApplyQuad(const Vector3& P0,
                              const Vector3& P1,
                              const Vector3& P2,
                              const Vector3& P3,
                              uint32_t            Flags,
                              int            PrimitiveKey)
{
    //LOG_STAT(k_stats_Collision);
    //CONTEXT("collision_mgr::ApplyQuad");

    assert(m_bApplyStarted);

    const BBox& DBox = m_DynamicBBoxes[m_ContextInfo.Context];

    if ((P0.x < DBox.min.x) && (P1.x < DBox.min.x) && (P2.x < DBox.min.x) && (P3.x < DBox.min.x)) {
        return;
    }
    if ((P0.y < DBox.min.y) && (P1.y < DBox.min.y) && (P2.y < DBox.min.y) && (P3.y < DBox.min.y)) {
        return;
    }
    if ((P0.z < DBox.min.z) && (P1.z < DBox.min.z) && (P2.z < DBox.min.z) && (P3.z < DBox.min.z)) {
        return;
    }
    if ((P0.x > DBox.min.x) && (P1.x > DBox.min.x) && (P2.x > DBox.min.x) && (P3.x > DBox.min.x)) {
        return;
    }
    if ((P0.y > DBox.min.y) && (P1.y > DBox.min.y) && (P2.y > DBox.min.y) && (P3.y > DBox.min.y)) {
        return;
    }
    if ((P0.z > DBox.min.z) && (P1.z > DBox.min.z) && (P2.z > DBox.min.z) && (P3.z > DBox.min.z)) {
        return;
    }

    // what's our moving primitive?
    switch (m_DynamicPrimitive) {
    case PRIMITIVE_DYNAMIC_CYLINDER:
        ApplyTriangleToCylinder(P0, P1, P2, Flags, PrimitiveKey);
        ApplyTriangleToCylinder(P2, P3, P0, Flags, PrimitiveKey);
        break;
    case PRIMITIVE_DYNAMIC_SPHERE:
        ApplyTriangleToSphere(P0, P1, P2, Flags, PrimitiveKey);
        ApplyTriangleToSphere(P2, P3, P0, Flags, PrimitiveKey);
        break;
    case PRIMITIVE_DYNAMIC_RAY:
    case PRIMITIVE_DYNAMIC_LOS:
        ApplyTriangleToRay(P0, P1, P2, Flags, PrimitiveKey);
        ApplyTriangleToRay(P2, P3, P0, Flags, PrimitiveKey);
        break;
    default:
        assert(0); // invalid primitive
    }
}

//==============================================================================

void collision_mgr::ApplyAABBox(
    const BBox& bb,
    uint32_t         Flags,
    int         PrimitiveKey /* = 0 */)
{
    //LOG_STAT(k_stats_Collision);
    assert(m_bApplyStarted);

    // check to see if the bboxes intersect
    if (!bb.Intersect(m_DynamicBBoxes[m_ContextInfo.Context])) {
        return;
    }

    // what's our moving primitive?
    switch (m_DynamicPrimitive) {
    case PRIMITIVE_DYNAMIC_CYLINDER:
        ApplyAABBoxToCylinder(bb, Flags, PrimitiveKey);
        break;
    case PRIMITIVE_DYNAMIC_SPHERE:
        ApplyAABBoxToSphere(bb, Flags, PrimitiveKey);
        break;
    case PRIMITIVE_DYNAMIC_RAY:
    case PRIMITIVE_DYNAMIC_LOS:
        ApplyAABBoxToRay(bb, Flags);
        break;
    default:
        assert(0); // invalid primitive
    }
}

//==============================================================================

void collision_mgr::ApplyOOBBox(const BBox&    LocalBBox,
                                const Matrix4& L2W,
                                uint32_t            Flags,
                                int            PrimitiveKey)
{
    // Indices used to convert min + max of bbox into 8 corners
    static int CornerIndices[8 * 3] = {0, 1, 2,
                                       4, 1, 2,
                                       0, 5, 2,
                                       4, 5, 2,
                                       0, 1, 6,
                                       4, 1, 6,
                                       0, 5, 6,
                                       4, 5, 6};

    // Indices used to convert 8 corners into a 4 sided NGon
    static int SideIndices[6 * 4] = {0, 2, 3, 1,
                                     1, 3, 7, 5,
                                     5, 7, 6, 4,
                                     4, 6, 2, 0,
                                     2, 6, 7, 3,
                                     4, 0, 1, 5};

    // Locals
    int        i;
    Vector3    Local;
    Vector3    Corners[8];
    const int* pIndices;
    const float* pBBoxF;

    // Transform all corners of the local AA bbox into world space
    pIndices = CornerIndices;
    pBBoxF = (float*)&LocalBBox;
    for (i = 0; i < 8; i++) {
        // Setup corner in local space
        Local.set(pBBoxF[pIndices[0]], pBBoxF[pIndices[1]], pBBoxF[pIndices[2]]);

        // Transform into world space
        Corners[i] = L2W * Local;

        // Next vert
        pIndices += 3;
    }

    // Apply 6 sides of world oobbox
    pIndices = SideIndices;
    for (i = 0; i < 6; i++) {
        // Indices of side plane are pIndices[0], pIndices[1], pIndices[2], pIndices[3]

        // Apply tri0
        ApplyQuad(Corners[pIndices[0]], // P0
                  Corners[pIndices[1]], // P1
                  Corners[pIndices[2]], // P2
                  Corners[pIndices[3]], // P3
                  Flags,                // Flags
                  PrimitiveKey);        // PrimitiveKey

        // Next side
        pIndices += 4;
    }
}

//==============================================================================

void collision_mgr::ApplySphereToCylinder(
    const Vector3& WorldPos,
    float            Radius,
    uint32_t            Flags,
    int            PrimitiveKey /* = 0 */)
{
    // required because of data dependencies
    assert(m_DynamicPrimitive == PRIMITIVE_DYNAMIC_CYLINDER);

    //--------------------------------------------------
    // Check collisions between matching spheres in
    // the start cylinder and the end cylinder
    //--------------------------------------------------
    int     i;
    float     FinalT;
    Vector3 FinalHitPoint;

    for (i = 0; i < m_CylinderInfo[m_ContextInfo.Context].nStartSpheres; ++i) {
        if (ComputeSphereSphereCollision(
                WorldPos,
                Radius,
                m_CylinderInfo[m_ContextInfo.Context].Radius,
                m_CylinderInfo[m_ContextInfo.Context].StartSpherePositions[i],
                m_CylinderInfo[m_ContextInfo.Context].EndSpherePositions[i],
                FinalT,
                FinalHitPoint)) {
            // record the collision
            Vector3 Normal = (FinalHitPoint - WorldPos) * (1 / Radius);
            plane   HitPlane;

            // Deal with creating a plane when we are already inside the collision cylinder
            if (FinalT == 0) {
                Normal = m_CylinderInfo[m_ContextInfo.Context].StartSpherePositions[i] - WorldPos;
            }

            //#### WE SHOULDN'T HAVE TO DO THIS IF STATEMENT. FIGURE OUT WHAT IS
            //     WRONG WITH THE ABOVE LOGIC!!!!
            if (Normal.LengthSquared() < 0.01f) {
                Normal.set(0.0f, 1.0f, 0.0f);
            }
            HitPlane.Setup(FinalHitPoint, Normal);

            plane SlipPlane = HitPlane;

            //  This assert is to verify that the number is normalized BUT
            //  it was causing an assert failure due purely to rounding errors.
            //  The number was increased to avoid the assert failure but the
            //  assert was left to verify that it was still close to 1.0, just
            //  not as close.  If it fails again with a normalized value, just
            //  up it again.
            //assert(x_abs(SlipPlane.Normal.LengthSquared() - 1.0f) < 0.0005f);

            collision_mgr::collision TempCollision(
                FinalT,
                FinalHitPoint,
                HitPlane,
                SlipPlane,
                m_ContextInfo.Guid,
                PrimitiveKey,
                PRIMITIVE_STATIC_SPHERE,
                false,
                WorldPos.y + Radius,
                Flags);

            RecordCollision(TempCollision);
        }
    }
}

//==============================================================================

void collision_mgr::ApplySphereToSphere(
    const Vector3& WorldPos,
    float            Radius,
    uint32_t            Flags,
    int            PrimitiveKey /* = 0 */)
{
    assert(m_DynamicPrimitive == PRIMITIVE_DYNAMIC_SPHERE);

    float     FinalT;
    Vector3 FinalHitPoint;

    // is there a collison?
    if (ComputeSphereSphereCollision(
            WorldPos, Radius, m_SphereInfo[m_ContextInfo.Context].Radius, m_SphereInfo[m_ContextInfo.Context].Start, m_SphereInfo[m_ContextInfo.Context].End, FinalT, FinalHitPoint)) {
        // record the collision
        const Vector3 Normal = (FinalHitPoint - WorldPos) * (1 / Radius);
        plane         HitPlane;
        HitPlane.Setup(FinalHitPoint, Normal);

        plane SlipPlane = HitPlane;
       // assert(x_abs(SlipPlane.Normal.LengthSquared() - 1.0f) < 0.01f);

        collision_mgr::collision TempCollision(
            FinalT,
            FinalHitPoint,
            HitPlane,
            SlipPlane,
            m_ContextInfo.Guid,
            PrimitiveKey,
            PRIMITIVE_STATIC_SPHERE,
            false,
            WorldPos.y + Radius,
            Flags);

        RecordCollision(TempCollision);
    }
}

//==============================================================================

void collision_mgr::ApplySphereToRay(
    const Vector3& WorldPos,
    float            Radius,
    uint32_t            Flags,
    int            PrimitiveKey /* = 0 */)
{
    assert(m_bIsRayCheck);

    float     FinalT;
    Vector3 FinalHitPoint;

    // is there a collison?
    if (ComputeRaySphereCollision(
            WorldPos, Radius, m_RayInfo[m_ContextInfo.Context].Start, m_RayInfo[m_ContextInfo.Context].End, FinalT, FinalHitPoint)) {
        // Setup normal
        Vector3 Normal;
        if (FinalT == 0) { // SB - Fix the normal for when ray start + end are inside the sphere!
            Normal = m_RayInfo[m_ContextInfo.Context].End - m_RayInfo[m_ContextInfo.Context].Start;
        } else {
            Normal = (FinalHitPoint - WorldPos) * (1 / Radius);
        }

        // record the collision
        plane HitPlane;
        HitPlane.Setup(FinalHitPoint, Normal);

        plane SlipPlane = HitPlane;
        assert(abs(SlipPlane.Normal.LengthSquared() - 1.0f) < 0.01f);

        collision_mgr::collision TempCollision(
            FinalT,
            FinalHitPoint,
            HitPlane,
            SlipPlane,
            m_ContextInfo.Guid,
            PrimitiveKey,
            PRIMITIVE_STATIC_SPHERE,
            false,
            WorldPos.y + Radius,
            Flags);

        RecordCollision(TempCollision);
    }
}

//==============================================================================

void collision_mgr::ApplyTriangleToCylinder(
    const Vector3& P0,
    const Vector3& P1,
    const Vector3& P2,
    uint32_t            Flags,
    int            PrimitiveKey)
{
    assert(m_DynamicPrimitive == PRIMITIVE_DYNAMIC_CYLINDER);

    //--------------------------------------------------
    // Check collisions between matching spheres in
    // the start cylinder and the end cylinder
    //--------------------------------------------------
    int     i;
    float     FinalT;
    Vector3 FinalHitPoint;
    Vector3 Triangle[3];
    //bool       HitTriangleEdge;
    Triangle[0] = P0;
    Triangle[1] = P1;
    Triangle[2] = P2;

    for (i = 0; i < m_CylinderInfo[m_ContextInfo.Context].nStartSpheres; ++i) {
        if (ComputeSphereTriCollision(
                Triangle, m_CylinderInfo[m_ContextInfo.Context].StartSpherePositions[i], m_CylinderInfo[m_ContextInfo.Context].EndSpherePositions[i], m_CylinderInfo[m_ContextInfo.Context].Radius, FinalT, FinalHitPoint
                //, HitTriangleEdge
                )) {
            // record the collision
            plane HitPlane;
            plane SlipPlane;

            //if ( HitTriangleEdge )
            {
                // Our slide plane is defined by the impact point, and a
                // normal from that point towards the sphere's center,
                // when the sphere is at the collision T
                const Vector3 SphereImpactPosition = m_CylinderInfo[m_ContextInfo.Context]
                                                         .StartSpherePositions[i] +
                                                     ((m_CylinderInfo[m_ContextInfo.Context]
                                                           .EndSpherePositions[i] -
                                                       m_CylinderInfo[m_ContextInfo.Context]
                                                           .StartSpherePositions[i]) *
                                                      FinalT);
                Vector3 Normal = SphereImpactPosition - FinalHitPoint;
                Normal.Normalize();
                SlipPlane.Setup(FinalHitPoint, Normal);
                assert(abs(SlipPlane.Normal.LengthSquared() - 1.0f) < 0.0001f);
            }

            HitPlane.Setup(P0, P1, P2);

            collision_mgr::collision TempCollision(
                FinalT,
                FinalHitPoint,
                HitPlane,
                SlipPlane,
                m_ContextInfo.Guid,
                PrimitiveKey,
                PRIMITIVE_STATIC_TRIANGLE,
                false, //HitTriangleEdge,
                std::max(P0.y, std::max(P1.y, P2.y)),
                Flags);

            RecordCollision(TempCollision);
        }
    }
}

//==============================================================================

void collision_mgr::ApplyTriangleToStretchedSphere(
    const Vector3& P0,
    const Vector3& P1,
    const Vector3& P2,
    uint32_t            Flags,
    int            PrimitiveKey)
{
    assert(m_DynamicPrimitive == PRIMITIVE_DYNAMIC_CYLINDER);

    float     Radius = m_CylinderInfo[0].Radius;
    float     HalfHeight = m_CylinderInfo[0].Height * 0.5f;
    float     WorldToSphereScale = Radius / HalfHeight;
    float     SphereToWorldScale = 1.0f / WorldToSphereScale;
    Vector3 WorldSpaceCenter = m_CylinderInfo[0].BotStart + Vector3(0, HalfHeight, 0);
    WorldSpaceCenter.x = 0;
    WorldSpaceCenter.z = 0;

    //
    // Scale all important info from world into sphere space
    //
    Vector3 SphereSpaceTriangle[3];
    SphereSpaceTriangle[0] = (P0 - WorldSpaceCenter);
    SphereSpaceTriangle[1] = (P1 - WorldSpaceCenter);
    SphereSpaceTriangle[2] = (P2 - WorldSpaceCenter);
    SphereSpaceTriangle[0].y *= WorldToSphereScale;
    SphereSpaceTriangle[1].y *= WorldToSphereScale;
    SphereSpaceTriangle[2].y *= WorldToSphereScale;

    Vector3 SphereSpaceStart;
    Vector3 SphereSpaceStop;

    SphereSpaceStart = (m_CylinderInfo[0].BotStart + Vector3(0, HalfHeight, 0)) - WorldSpaceCenter;
    SphereSpaceStop = (m_CylinderInfo[0].BotEnd + Vector3(0, HalfHeight, 0)) - WorldSpaceCenter;
    SphereSpaceStart.y *= WorldToSphereScale;
    SphereSpaceStop.y *= WorldToSphereScale;

    //
    // Now compute sphere triangle collision
    //
    float     FinalT;
    Vector3 FinalHitPoint;

    if (ComputeSphereTriCollision(
            SphereSpaceTriangle, SphereSpaceStart, SphereSpaceStop, Radius, FinalT, FinalHitPoint
            //, HitTriangleEdge
            )) {
        // record the collision
        plane HitPlane;
        plane SlipPlane;

        //
        // Keep sphere space hit point
        //
        Vector3 SphereSpaceHitPoint = FinalHitPoint;

        //
        // Transform the final hit point back into world space
        //
        FinalHitPoint.y *= SphereToWorldScale;
        FinalHitPoint += WorldSpaceCenter;

        // Compute ellipsoid space hit point
        Vector3 SphereSpaceCenterAtImpact = SphereSpaceStart + FinalT * (SphereSpaceStop - SphereSpaceStart);
        Vector3 EllipsoidSpaceHitPoint = SphereSpaceHitPoint - SphereSpaceCenterAtImpact;
        EllipsoidSpaceHitPoint.y *= SphereToWorldScale;

        // The normal(Nx,Ny,Nz) of a point(x,y,z) on an ellipsoid of radii (a,b,c) is:
        //      ( Nx, Ny, Nz ) = ( 2x/a^2, 2y/b^2, 2z/c^2 )
        // See this link for details on how it is derived:
        // http://www.peroxide.dk/download/tutorials/tut10/pxdtut10.html

        // Compute slide world normal using ( Nx, Ny, Nz ) = ( x/a^2, y/b^2, z/c^2 )
        //   where:
        //      ( x, y, z ) = EllipsoidSpaceHitPoint
        //      ( a, b, c ) = ( Radius, HalfHeight, Radius )
        // NOTE: The *2 is skipped since the end result is normalized
        Vector3 WorldSpaceNormal = -EllipsoidSpaceHitPoint;
        float     OneOverRadiusSqr = 1.0f / (Radius * Radius);
        WorldSpaceNormal.x *= OneOverRadiusSqr;
        WorldSpaceNormal.y /= HalfHeight * HalfHeight;
        WorldSpaceNormal.z *= OneOverRadiusSqr;
        WorldSpaceNormal.Normalize();


        SlipPlane.Setup(FinalHitPoint, WorldSpaceNormal);

        //
        // Setup the hit plane from the original triangle
        //
        HitPlane.Setup(P0, P1, P2);

        collision_mgr::collision TempCollision(
            FinalT,
            FinalHitPoint,
            HitPlane,
            SlipPlane,
            m_ContextInfo.Guid,
            PrimitiveKey,
            PRIMITIVE_STATIC_TRIANGLE,
            false,
            std::max(P0.y, std::max(P1.y, P2.y)),
            Flags);

        RecordCollision(TempCollision);
    }
}

//==============================================================================

void collision_mgr::ApplyTriangleToSphere(
    const Vector3& P0,
    const Vector3& P1,
    const Vector3& P2,
    uint32_t            Flags, /* = 0 */
    int            PrimitiveKey /* = 0 */)
{
    assert(m_DynamicPrimitive == PRIMITIVE_DYNAMIC_SPHERE);

    float     FinalT;
    Vector3 FinalHitPoint;
    Vector3 Triangle[3];
    Triangle[0] = P0;
    Triangle[1] = P1;
    Triangle[2] = P2;

    if (ComputeSphereTriCollision(
            Triangle,
            m_SphereInfo[m_ContextInfo.Context].Start,
            m_SphereInfo[m_ContextInfo.Context].End,
            m_SphereInfo[m_ContextInfo.Context].Radius,
            FinalT,
            FinalHitPoint)) {
        // record the collision
        plane HitPlane;
        plane SlipPlane;

        {
            // Our slide plane is defined by the impact point, and a
            // normal from that point towards the sphere's center,
            // when the sphere is at the collision T
            const Vector3 SphereImpactPosition = m_SphereInfo[m_ContextInfo.Context].Start + ((m_SphereInfo[m_ContextInfo.Context].End - m_SphereInfo[m_ContextInfo.Context].Start) * FinalT);
            Vector3       Normal = SphereImpactPosition - FinalHitPoint;
            Normal.Normalize();
            SlipPlane.Setup(FinalHitPoint, Normal);
            assert(abs(SlipPlane.Normal.LengthSquared() - 1.0f) < 0.0001f);
        }

        // The triangle plane is our slide plane
        HitPlane.Setup(P0, P1, P2);

        collision_mgr::collision TempCollision(
            FinalT,
            FinalHitPoint,
            HitPlane,
            SlipPlane,
            m_ContextInfo.Guid,
            PrimitiveKey,
            PRIMITIVE_STATIC_TRIANGLE,
            false,
            std::max(P0.y, std::max(P1.y, P2.y)),
            Flags);

        RecordCollision(TempCollision);
    }
}

//==============================================================================

void collision_mgr::ApplyTriangleToRay(
    const Vector3& P0,
    const Vector3& P1,
    const Vector3& P2,
    uint32_t            Flags, /* = 0 */
    int            PrimitiveKey /* = 0 */)
{
    assert((m_DynamicPrimitive == PRIMITIVE_DYNAMIC_RAY) || (m_DynamicPrimitive == PRIMITIVE_DYNAMIC_LOS));
    float     FinalT;
    Vector3 FinalHitPoint;
    Vector3 Triangle[3];
    Triangle[0] = P0;
    Triangle[1] = P1;
    Triangle[2] = P2;

    if (ComputeRayTriCollision(
            Triangle, m_RayInfo[m_ContextInfo.Context].Start, m_RayInfo[m_ContextInfo.Context].End, FinalT, FinalHitPoint)) {
        // record the collision
        plane HitPlane(P0, P1, P2);

        plane SlipPlane = HitPlane;

        collision_mgr::collision TempCollision(
            FinalT,
            FinalHitPoint,
            HitPlane,
            SlipPlane,
            m_ContextInfo.Guid,
            PrimitiveKey,
            PRIMITIVE_STATIC_TRIANGLE,
            false,
            std::max(P0.y, std::max(P1.y, P2.y)),
            Flags);

        RecordCollision(TempCollision);
    }
}

//==============================================================================

void collision_mgr::ApplyAABBoxToCylinder(
    const BBox& AABBox,
    uint32_t         Flags, /* = 0 */
    int         PrimitiveKey /* = 0 */)
{
    assert(m_DynamicPrimitive == PRIMITIVE_DYNAMIC_CYLINDER);

    //--------------------------------------------------
    // Check collisions between matching spheres in
    // the start cylinder and the end cylinder
    //--------------------------------------------------
    int     i;
    float     FinalT;
    Vector3 FinalHitPoint;
    plane   HitPlane;
    plane   SlipPlane;

    for (i = 0; i < m_CylinderInfo[m_ContextInfo.Context].nStartSpheres; ++i) {
        if (ComputeSphereAABBoxCollision(
                AABBox,
                m_CylinderInfo[m_ContextInfo.Context].StartSpherePositions[i],
                m_CylinderInfo[m_ContextInfo.Context].EndSpherePositions[i],
                m_CylinderInfo[m_ContextInfo.Context].Radius,
                FinalT,
                FinalHitPoint,
                HitPlane,
                SlipPlane)) {
            assert(abs(SlipPlane.Normal.LengthSquared() - 1.0f) < 0.01f);
            // record the collision

            collision_mgr::collision TempCollision(
                FinalT,
                FinalHitPoint,
                HitPlane,
                SlipPlane,
                m_ContextInfo.Guid,
                PrimitiveKey,
                PRIMITIVE_STATIC_AA_BBOX,
                false,
                AABBox.max.y,
                Flags);

            RecordCollision(TempCollision);
        }
    }
}

//==============================================================================

void collision_mgr::ApplyAABBoxToSphere(
    const BBox& AABBox,
    uint32_t         Flags, /* = 0 */
    int         PrimitiveKey /* = 0 */)
{
    assert(m_DynamicPrimitive == PRIMITIVE_DYNAMIC_SPHERE);

    float     FinalT;
    Vector3 FinalHitPoint;
    plane   HitPlane;
    plane   SlipPlane;

    if (ComputeSphereAABBoxCollision(
            AABBox,
            m_SphereInfo[m_ContextInfo.Context].Start,
            m_SphereInfo[m_ContextInfo.Context].End,
            m_SphereInfo[m_ContextInfo.Context].Radius,
            FinalT,
            FinalHitPoint,
            HitPlane,
            SlipPlane)) {
        assert(abs(SlipPlane.Normal.LengthSquared() - 1.0f) < 0.01f);
        // record the collision

        collision_mgr::collision TempCollision(
            FinalT,
            FinalHitPoint,
            HitPlane,
            SlipPlane,
            m_ContextInfo.Guid,
            PrimitiveKey,
            PRIMITIVE_STATIC_AA_BBOX,
            false,
            AABBox.max.y,
            Flags);

        RecordCollision(TempCollision);
    }
}

//==============================================================================

void collision_mgr::ApplyAABBoxToRay(
    const BBox& AABBox,
    uint32_t         Flags, /* = 0 */
    int         PrimitiveKey /* = 0 */)
{
    assert(m_DynamicPrimitive == PRIMITIVE_DYNAMIC_RAY || m_DynamicPrimitive == PRIMITIVE_DYNAMIC_LOS);

    float     FinalT;
    Vector3 FinalHitPoint;
    plane   HitPlane;
    plane   SlipPlane;

    if (ComputeRayAABBoxCollision(
            AABBox,
            m_RayInfo[m_ContextInfo.Context].Start,
            m_RayInfo[m_ContextInfo.Context].End,
            FinalT,
            FinalHitPoint,
            HitPlane,
            SlipPlane)) {
        assert(abs(SlipPlane.Normal.LengthSquared() - 1.0f) < 0.01f);
        // record the collision

        collision_mgr::collision TempCollision(
            FinalT,
            FinalHitPoint,
            HitPlane,
            SlipPlane,
            m_ContextInfo.Guid,
            PrimitiveKey,
            PRIMITIVE_STATIC_AA_BBOX,
            false,
            AABBox.max.y,
            Flags);

        RecordCollision(TempCollision);
    }
}

//==============================================================================

int collision_mgr::GetCylinderSpherePositions(
    const Vector3& Bottom,
    const Vector3& Top,
    float            Radius,
    Vector3*       SpherePositions,
    int            MaxnSpheres)
{
    assert((Top - Bottom).LengthSquared() > 0.0001f);

    Vector3 Dir = Top - Bottom;
    Dir.Normalize();

    // Position the bottom and top spheres
    const Vector3 RadiusDir = Dir * Radius;
    const Vector3 First = Bottom + RadiusDir;
    const Vector3 Last = Top - RadiusDir;

    // Determine the number of spheres
    const float Dist = (Last - First).Length();
    int       nSpheres = (int)(Dist / Radius);

    // Patch for cases where Dist ~=~ Radius
    float fSpheres = (Dist / Radius) - (float)nSpheres;
    if (fSpheres > 0.9f) {
        nSpheres++;
    }

    nSpheres += 2;
    while (nSpheres > MaxnSpheres) {
        --nSpheres;
    }

    // Determine the spacing between the spheres
    const float     Interval = Dist / (nSpheres - 1);
    const Vector3 IntervalDir = Dir * Interval;

    // Create and store the sphere positions
    int     i;
    Vector3 Cur = First;
    for (i = 0; i < nSpheres; ++i) {
        SpherePositions[i] = Cur;
        Cur += IntervalDir;
    }

    return nSpheres;
}

//==============================================================================

void collision_mgr::RecordCollision(const collision_mgr::collision& Collision)
{
    //
    // Check data is decent
    //
    assert(abs(Collision.Plane.Normal.LengthSquared() - 1.0f) < 0.001f);
    assert(abs(Collision.SlipPlane.Normal.LengthSquared() - 1.0f) < 0.001f);
    assert((Collision.T >= 0.0f) && (Collision.T <= 1.0f));

    //
    // Check if we need to watch for permeable collisions
    //
    if (m_bCollectPermeable) {
        Object* pObject = objectManager->GetObjectByGuid(Collision.ObjectHitGuid);
        if (pObject && (pObject->GetAttrBits() & Object::ATTR_COLLISION_PERMEABLE)) {
            int i;

            // Look and see if already in the list
            for (i = 0; i < m_nPermeables; i++) {
                if (m_Permeable[i] == Collision.ObjectHitGuid) {
                    // Remember closest collision with permeable
                    m_PermeableT[i] = std::min(m_PermeableT[i], Collision.T);
                    break;
                }
            }

            // Add object to permeable list
            if ((i == m_nPermeables) && (i < MAX_PERMEABLE_OBJECTS)) {
                m_Permeable[m_nPermeables] = Collision.ObjectHitGuid;
                m_PermeableT[m_nPermeables] = Collision.T;
                m_nPermeables++;
            }

            // Don't list as a normal collision
            return;
        }
    }

    //
    // This will point to the collision slot we will keep
    //
    bool                     bFoundDuplicate = false;
    collision_mgr::collision* pDestC = NULL;

    //
    // Decide where the destination collision will be
    //

    // If there is a duplicate guid possibly replace it.
    if (m_bRemoveDuplicateGuids) {
        for (int i = 0; i < m_nCollisions; i++) {
            if (m_Collisions[i].ObjectHitGuid == Collision.ObjectHitGuid) {
                bFoundDuplicate = true;

                if (m_Collisions[i].T > Collision.T) {
                    // Replace with closer collision
                    pDestC = &m_Collisions[i];
                    break;
                }
            }
        }
    }

    // If we didn't find a duplicate then we need to find a fresh
    // collision entry
    if (!bFoundDuplicate) {
        if (m_nCollisions < m_nMaxCollisions) {
            assert(m_nCollisions < MAX_COLLISION_MGR_COLLISIONS);
            pDestC = &m_Collisions[m_nCollisions];
            m_nCollisions++;
        } else {
            // We may need to displace the collision with the largest T
            int i;
            float MaxT = -1.0f;
            int MaxIndex = -1;

            for (i = 0; i < m_nCollisions; ++i) {
                if (m_Collisions[i].T > MaxT) {
                    MaxT = m_Collisions[i].T;
                    MaxIndex = i;
                }
            }

            // Did we find one with a great T than the new collision?
            if (MaxT > Collision.T) {
                pDestC = &m_Collisions[MaxIndex];
            }
        }
    }

    // If we could never find a good DestC then skip the final steps
    if (pDestC != NULL) {
        *pDestC = Collision;

        if (m_ContextInfo.Context == 1) {
            // Transform the collision back to world space
            pDestC->Point = m_ContextInfo.L2W.Transform(pDestC->Point);
            pDestC->Plane.Transform(m_ContextInfo.L2W);
            pDestC->SlipPlane.Transform(m_ContextInfo.L2W);
        }
    }
}

//==============================================================================

void collision_mgr::CleanPermeables(void)
{
    float MaxT = 1.0f;
    if (m_nCollisions > 0) {
        MaxT = m_Collisions[0].T;
    }

    // Only keep permeables with T <= MaxT
    int i = 0;
    for (int j = 0; j < m_nPermeables; j++) {
        if (m_PermeableT[j] <= MaxT) {
            m_Permeable[i] = m_Permeable[j];
            m_PermeableT[i] = m_PermeableT[j];
            i++;
        }
    }

    m_nPermeables = i;
}

//==============================================================================

void collision_mgr::NotifyPermeables(void)
{
    m_bNotifyingPermeables = true;

    Object* pMovingObj = objectManager->GetObjectByGuid(m_MovingObjGuid);
    if (pMovingObj) {
        //if( m_nPermeables ) x_DebugMsg("*********************************\n");

        for (int i = 0; i < m_nPermeables; i++) {
            Object* pObj = objectManager->GetObjectByGuid(m_Permeable[i]);
            if (pObj) {
                //x_DebugMsg("Notify %2d %s\n",i,pObj->m_DebugInfo.m_pDesc->GetTypeName());
                pObj->OnColNotify(*pMovingObj);
            }
        }
    }

    m_bNotifyingPermeables = false;
}

//==============================================================================

inline int FindFirstIntersect(const BBox& bb, const BBox* pBBox, int nBBoxes, int iStart)
{
    while (iStart < nBBoxes) {
        if (bb.Intersect(pBBox[iStart])) {
            break;
        }
        iStart++;
    }
    return iStart;
}

//==============================================================================

void collision_mgr::ApplySphereToPolyCache(void)
{
    BBox DynamicBBox = m_DynamicBBoxes[0];
    DynamicBBox.Inflate(1, 1, 1);

    //
    // Gather factored out list of clusters in dynamic area
    //
    polyCache->BuildClusterList(DynamicBBox,
                                m_FilterThisType,
                                m_FilterTheseAttributes,
                                m_FilterNotTheseAttributes,
                                m_IgnoreList,
                                m_nIgnoredObjects);

    //
    // Were there no clusters?
    //
    if (polyCache->m_nClusters == 0) {
        return;
    }

    Vector3 SphereStart = m_SphereInfo[0].Start;
    Vector3 SphereEnd = m_SphereInfo[0].End;
    float     Radius = m_SphereInfo[0].Radius;

    //
    // Build culling flags
    //
    uint32_t     CullFlags = 0;
    Vector3 Dir = SphereEnd - SphereStart;
    Dir.Normalize();
    if (Dir.x > +0.001f) {
        CullFlags |= BOUNDS_X_POS;
    }
    if (Dir.x < -0.001f) {
        CullFlags |= BOUNDS_X_NEG;
    }
    if (Dir.y > +0.001f) {
        CullFlags |= BOUNDS_Y_POS;
    }
    if (Dir.y < -0.001f) {
        CullFlags |= BOUNDS_Y_NEG;
    }
    if (Dir.z > +0.001f) {
        CullFlags |= BOUNDS_Z_POS;
    }
    if (Dir.z < -0.001f) {
        CullFlags |= BOUNDS_Z_NEG;
    }

    //
    // Loop through the clusters and process the triangles
    //
    for (int iCL = 0; iCL < polyCache->m_nClusters; iCL++) {
        poly_cache::cluster& CL = *polyCache->m_ClusterList[iCL];

        // Setup context
        StartApply(CL.Guid);

        int iQ = -1;
        while (1) {
            // Do tight loop on bbox checks and cull flags
            {
                iQ++;
                while (iQ < CL.nQuads) {
                    // Do flag culling
                    if ((CL.pBounds[iQ].Flags & CullFlags) == 0) {
                        //#### This could be moved outside the loop
                        // and do a pointer incremenet after we
                        // optimized the Vector3/bbox stuff
                        BBox* pBBox = (BBox*)(&CL.pBounds[iQ]);
                        if (DynamicBBox.Intersect(*pBBox)) {
                            break;
                        }
                    }
                    iQ++;
                }
                if (iQ == CL.nQuads) {
                    break;
                }
            }

            // Get access to this quad
            poly_cache::cluster::quad& QD = CL.pQuad[iQ];

            // Skip if moving away from quad
            Vector3& N = CL.pNormal[QD.iN];
            if (N.Dot(Dir) > 0) {
                continue;
            }

            // Check if starting sphere is behind plane
            if ((N.Dot(SphereStart) + CL.pBounds[iQ].PlaneD) < -Radius) {
                continue;
            }

            // Check if ending sphere is in front of plane
            if ((N.Dot(SphereEnd) + CL.pBounds[iQ].PlaneD) > Radius) {
                continue;
            }

            // Get the verts and call the actual collision routines
            {
                Vector3* P4[4];
                P4[0] = &CL.pPoint[QD.iP[0]];
                P4[1] = &CL.pPoint[QD.iP[1]];
                P4[2] = &CL.pPoint[QD.iP[2]];
                P4[3] = &CL.pPoint[QD.iP[3]];

                ApplyTriangleToSphere(*P4[0], *P4[1], *P4[2], 0, CL.PrimKey);

                if (CL.pBounds[iQ].Flags & BOUNDS_IS_QUAD) {
                    ApplyTriangleToSphere(*P4[0], *P4[2], *P4[3], 0, CL.PrimKey);
                }
            }
        }

        EndApply();

        if ((m_nCollisions > 0) && (m_bStopOnFirstCollisionFound)) {
            return;
        }
    }
}

//==============================================================================

void collision_mgr::ApplyCylinderToPolyCache(void)
{
    BBox DynamicBBox = m_DynamicBBoxes[0];
    DynamicBBox.Inflate(1, 1, 1);

    //
    // Gather factored out list of clusters in dynamic area
    //
    polyCache->BuildClusterList(DynamicBBox,
                                m_FilterThisType,
                                m_FilterTheseAttributes,
                                m_FilterNotTheseAttributes,
                                m_IgnoreList,
                                m_nIgnoredObjects);

    //
    // Were there no clusters?
    //
    if (polyCache->m_nClusters == 0) {
        return;
    }

    //
    // Build culling flags
    //
    uint32_t     CullFlags = 0;
    Vector3 Dir = m_CylinderInfo[0].BotEnd - m_CylinderInfo[0].BotStart;
    Dir.Normalize();
    if (Dir.x > +0.001f) {
        CullFlags |= BOUNDS_X_POS;
    }
    if (Dir.x < -0.001f) {
        CullFlags |= BOUNDS_X_NEG;
    }
    if (Dir.y > +0.001f) {
        CullFlags |= BOUNDS_Y_POS;
    }
    if (Dir.y < -0.001f) {
        CullFlags |= BOUNDS_Y_NEG;
    }
    if (Dir.z > +0.001f) {
        CullFlags |= BOUNDS_Z_POS;
    }
    if (Dir.z < -0.001f) {
        CullFlags |= BOUNDS_Z_NEG;
    }

    //
    // Loop through the clusters and process the triangles
    //
    for (int iCL = 0; iCL < polyCache->m_nClusters; iCL++) {
        poly_cache::cluster& CL = *polyCache->m_ClusterList[iCL];

        // Setup context
        StartApply(CL.Guid);

        int iQ = -1;
        while (1) {
            // Do tight loop on bbox checks and cull flags
            {
                iQ++;
                while (iQ < CL.nQuads) {
                    // Do flag culling
                    if ((CL.pBounds[iQ].Flags & CullFlags) == 0) {
                        // Do bbox culling
                        //#### This could be moved outside the loop
                        // and do a pointer incremenet after we
                        // optimized the Vector3/bbox stuff
                        BBox* pBBox = (BBox*)(&CL.pBounds[iQ]);
                        if (DynamicBBox.Intersect(*pBBox)) {
                            break;
                        }
                    }
                    iQ++;
                }
                if (iQ == CL.nQuads) {
                    break;
                }
            }

            // Process this quad
            poly_cache::cluster::quad& QD = CL.pQuad[iQ];

            // Skip if moving away from quad
            if (CL.pNormal[QD.iN].Dot(Dir) > -0.0001f) {
                continue;
            }

            {
                Vector3* P4[4];
                P4[0] = &CL.pPoint[QD.iP[0]];
                P4[1] = &CL.pPoint[QD.iP[1]];
                P4[2] = &CL.pPoint[QD.iP[2]];
                P4[3] = &CL.pPoint[QD.iP[3]];

                ApplyTriangleToStretchedSphere(*P4[0], *P4[1], *P4[2], 0, CL.PrimKey);
                if (CL.pBounds[iQ].Flags & BOUNDS_IS_QUAD) {
                    ApplyTriangleToStretchedSphere(*P4[0], *P4[2], *P4[3], 0, CL.PrimKey);
                }
            }
        }

        EndApply();

        if ((m_nCollisions > 0) && (m_bStopOnFirstCollisionFound)) {
            return;
        }
    }
}

//==============================================================================
static bool ClipRay(const BBox&    bb,
                     const Vector3& P0,
                     const Vector3& P1,
                     float&           T0,
                     float&           T1)
{
    Vector3 Dir = P1 - P0;
    float     tx_min, tx_max;
    float     ty_min, ty_max;
    float     tz_min, tz_max;
    float     t_min, t_max;

    Vector3 MinMinusP0 = bb.min - P0;
    Vector3 MaxMinusP0 = bb.max - P0;

    if (Dir.x >= 0.0f) {
        t_min = tx_min = MinMinusP0.x / Dir.x;
        t_max = tx_max = MaxMinusP0.x / Dir.x;
    } else {
        t_min = tx_min = MaxMinusP0.x / Dir.x;
        t_max = tx_max = MinMinusP0.x / Dir.x;
    }

    if (Dir.y >= 0.0f) {
        ty_min = MinMinusP0.y / Dir.y;
        ty_max = MaxMinusP0.y / Dir.y;
    } else {
        ty_min = MaxMinusP0.y / Dir.y;
        ty_max = MinMinusP0.y / Dir.y;
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

    if (Dir.z >= 0.0f) {
        tz_min = MinMinusP0.z / Dir.z;
        tz_max = MaxMinusP0.z / Dir.z;
    } else {
        tz_min = MaxMinusP0.z / Dir.z;
        tz_max = MinMinusP0.z / Dir.z;
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

    return true;
}

//==============================================================================
void collision_mgr::ApplyRayToPolyCache(void)
{
    Vector3 RayStart = m_RayInfo[0].Start;
    Vector3 RayEnd = m_RayInfo[0].End;
    Vector3 Dir = RayEnd - RayStart;
    float     RayLen = Dir.Length();
    if (RayLen < 0.00001f) {
        return;
    }
    Vector3 NDir = Dir;
    NDir /= RayLen;
    float T1CM = 1.0f / RayLen;

    //
    // Build culling flags
    //
    uint32_t CullFlags = 0;
    if (NDir.x > +0.001f) {
        CullFlags |= BOUNDS_X_POS;
    }
    if (NDir.x < -0.001f) {
        CullFlags |= BOUNDS_X_NEG;
    }
    if (NDir.y > +0.001f) {
        CullFlags |= BOUNDS_Y_POS;
    }
    if (NDir.y < -0.001f) {
        CullFlags |= BOUNDS_Y_NEG;
    }
    if (NDir.z > +0.001f) {
        CullFlags |= BOUNDS_Z_POS;
    }
    if (NDir.z < -0.001f) {
        CullFlags |= BOUNDS_Z_NEG;
    }

    // Begin the polycache ray walk
    polyCache->BeginRayClusterWalk(RayStart,
                                   RayEnd,
                                   m_FilterThisType,
                                   m_FilterTheseAttributes,
                                   m_FilterNotTheseAttributes,
                                   m_IgnoreList,
                                   m_nIgnoredObjects);

    // Walk through polycache clusters
    while (1) {
        poly_cache::cluster* pCluster = polyCache->GetNextClusterFromRayWalk();
        if (!pCluster) {
            break;
        }

        poly_cache::cluster& CL = *pCluster;

        //
        // Try ray to cluster bbox
        //
        float T0, T1;
        if (ClipRay(CL.m_BBox, RayStart, RayEnd, T0, T1)) {
            // Back of parametric just slightly
            T0 -= T1CM;
            if (T0 < 0) {
                T0 = 0;
            }
            T1 += T1CM;
            if (T1 > 1) {
                T1 = 1;
            }

            // Compute smaller bbox points
            Vector3 HitBBoxStart = RayStart + Dir * T0;
            Vector3 HitBBoxEnd = RayStart + Dir * T1;
            BBox    RayBBox(HitBBoxStart, HitBBoxEnd);

            // Setup context
            StartApply(CL.Guid);

            int iQ = -1;
            while (1) {
                // Do tight loop on bbox checks and cull flags
                {
                    iQ++;
                    while (iQ < CL.nQuads) {
                        // Do flag culling
                        if ((CL.pBounds[iQ].Flags & CullFlags) == 0) {
                            // Do bbox culling
                            //#### This could be moved outside the loop
                            // and do a pointer incremenet after we
                            // optimized the Vector3/bbox stuff
                            BBox* pBBox = (BBox*)(&CL.pBounds[iQ]);
                            if (RayBBox.Intersect(*pBBox)) {
                                break;
                            }
                        }
                        iQ++;
                    }
                    if (iQ == CL.nQuads) {
                        break;
                    }
                }

                // Process this quad
                poly_cache::cluster::quad& QD = CL.pQuad[iQ];

                // Skip if moving away from quad
                Vector3& N = CL.pNormal[QD.iN];
                float      Dot = N.Dot(Dir);
                if ((Dot) > -0.0001f) {
                    continue;
                }

                {
                    Vector3* P4[4];
                    P4[0] = &CL.pPoint[QD.iP[0]];
                    P4[1] = &CL.pPoint[QD.iP[1]];
                    P4[2] = &CL.pPoint[QD.iP[2]];
                    P4[3] = &CL.pPoint[QD.iP[3]];

                    ApplyTriangleToRay(*P4[0], *P4[1], *P4[2], 0, CL.PrimKey);
                    if (CL.pBounds[iQ].Flags & BOUNDS_IS_QUAD) {
                        ApplyTriangleToRay(*P4[0], *P4[2], *P4[3], 0, CL.PrimKey);
                    }
                }
            }

            EndApply();

            if ((m_nCollisions > 0) && (m_bStopOnFirstCollisionFound)) {
                return;
            }
        }
    }
}

void collision_mgr::AddToIgnoreList(guid* Guid, int nGuids)
{

    assert((m_nIgnoredObjects + nGuids) <= MAX_IGNORED_OBJECTS);

    for (int i = 0; i < nGuids; i++) {
        m_IgnoreList[m_nIgnoredObjects] = Guid[i];
        m_nIgnoredObjects++;
    }
}
