#pragma once

#include "../VectorMath.h"
#include "../Guid.h"
#include "../objects/Object.h"
#include "CollisionPrimatives.h"
#include "../objectManager/ObjectManager.h"
#include "PolyCache.h"

class poly_cache;
class PlaysurfaceMgr;

extern poly_cache g_PolyCache;

//==============================================================================

#define MAX_NUM_SPHERES 10
#define MAX_COLLISION_MGR_COLLISIONS 32
#define MAX_IGNORED_OBJECTS 32
#define MAX_PERMEABLE_OBJECTS 32

enum primitive
{
    PRIMITIVE_START = 0,
    PRIMITIVE_INVALID = PRIMITIVE_START,

    // moving primitives
    PRIMITIVE_DYNAMIC_START,
    PRIMITIVE_DYNAMIC_CYLINDER = PRIMITIVE_DYNAMIC_START,
    PRIMITIVE_DYNAMIC_SPHERE,
    PRIMITIVE_DYNAMIC_RAY,
    PRIMITIVE_DYNAMIC_LOS,
    PRIMITIVE_DYNAMIC_END = PRIMITIVE_DYNAMIC_LOS,

    // stationary primitives
    PRIMITIVE_STATIC_START,
    PRIMITIVE_STATIC_SPHERE = PRIMITIVE_STATIC_START,
    PRIMITIVE_STATIC_TRIANGLE,
    PRIMITIVE_STATIC_AA_BBOX,
    PRIMITIVE_STATIC_END = PRIMITIVE_STATIC_AA_BBOX
};

class collision_mgr
{
    //------------------------------------------------------------------------------
    //  Public constants
public:
    //------------------------------------------------------------------------------
    //  Public Types

public:
    struct collision_context_info
    {
        int     Context;
        guid    Guid;
        Matrix4 L2W;
        Matrix4 W2L;
    };

    //
    // dynamic primitive structs
    //
    struct dynamic_cylinder
    {
        Vector3 BotStart;
        Vector3 BotEnd;
        Vector3 TopStart;
        Vector3 TopEnd;
        float   Radius;
        float   Height;

        Vector3 StartSpherePositions[MAX_NUM_SPHERES];
        Vector3 EndSpherePositions[MAX_NUM_SPHERES];
        int     nStartSpheres;
        int     nEndSpheres;
    };

    struct dynamic_sphere
    {
        Vector3 Start;
        Vector3 End;
        float   Radius;
    };

    struct dynamic_ray
    {
        Vector3 Start;
        Vector3 End;
    };

    struct collision
    {
        float   T;                 // collisoin intersection 0->1
        Vector3 Point;             // Point of collision
        plane   Plane;             // Plane of collision
        plane   SlipPlane;         // Plane we slip along
        guid    ObjectHitGuid;     // Object that was hit
        int     PrimitiveKey;      // ID (can be used to identify
                                   //      sub-objects in object etc)
        primitive StaticPrimitive; // Static primitive hit
        bool      HitTriangleEdge; // Point is on the tri edge
        uint32_t  Flags;           // Flags indicating the properties of the collision object.

        collision() { Clear(); }

        void Clear()
        {
            T = 0;
            ObjectHitGuid = 0;
            PrimitiveKey = 0;
            StaticPrimitive = PRIMITIVE_INVALID;
            HitTriangleEdge = false;
            Flags = 0;

            Point.set(0, 0, 0);
            Plane.Setup(0, 1, 0, 0);
            SlipPlane.Setup(0, 1, 0, 0);
        }

        bool IsValid(void) const
        {
            return !!(ObjectHitGuid != 0);
        }

        collision(
            float          FinalT,
            const Vector3& HitPoint,
            const plane&   HitPlane,
            const plane&   SlipPlane,
            guid           HitGuid,
            int            HitKey,
            primitive      HitStatic,
            bool           HitEdge,
            float          HitObjectHeight,
            uint32_t       aFlags)
            : T(FinalT)
            , Point(HitPoint)
            , Plane(HitPlane)
            , SlipPlane(SlipPlane)
            , ObjectHitGuid(HitGuid)
            , PrimitiveKey(HitKey)
            , StaticPrimitive(HitStatic)
            , HitTriangleEdge(HitEdge)
            , Flags(aFlags)
        {
        }
    };

    //------------------------------------------------------------------------------
    //  Public Functions

public:
    //--------------------------------------------------
    // Initial Setup functions.  Each function sets up
    // the other options as listed below.
    //--------------------------------------------------

    //--------------------------------------------------
    // CylinderSetup
    //--------------------------------------------------
    // MaxCollisions                    = 1
    // UseLowPoly                       = TRUE
    // IgnoreGlass                      = FALSE
    // StopOnFirstCollisionFound        = FALSE
    // CollectPermeables                = FALSE
    // RemoveDuplicateGuids             = TRUE
    // Ignore list is emptied
    //--------------------------------------------------
    void CylinderSetup(guid           MovingObjGuid,
                       const Vector3& WorldStart,
                       const Vector3& WorldEnd,
                       float          Radius,
                       float          Height);

    //--------------------------------------------------
    // SphereSetup
    //--------------------------------------------------
    // MaxCollisions                    = 1
    // UseLowPoly                       = FALSE
    // IgnoreGlass                      = FALSE
    // StopOnFirstCollisionFound        = FALSE
    // CollectPermeables                = FALSE
    // RemoveDuplicateGuids             = TRUE
    // Ignore list is emptied
    //--------------------------------------------------
    void SphereSetup(guid           MovingObjGuid,
                     const Vector3& WorldStart,
                     const Vector3& WorldEnd,
                     float          Radius);

    //--------------------------------------------------
    // RaySetup and EditorSelectRaySetup
    //--------------------------------------------------
    // MaxCollisions                    = 1
    // UseLowPoly                       = FALSE
    // IgnoreGlass                      = FALSE
    // StopOnFirstCollisionFound        = FALSE
    // CollectPermeables                = FALSE
    // RemoveDuplicateGuids             = TRUE
    // Ignore list is emptied
    //--------------------------------------------------
    void RaySetup(guid           MovingObjGuid,
                  const Vector3& WorldStart,
                  const Vector3& WorldEnd);

    void EditorSelectRaySetup(const Vector3& WorldStart,
                              const Vector3& WorldEnd);

    //--------------------------------------------------
    // LineOfSightSetup
    //--------------------------------------------------
    // MaxCollisions                    = 1
    // UseLowPoly                       = FALSE
    // IgnoreGlass                      = TRUE
    // StopOnFirstCollisionFound        = TRUE
    // CollectPermeables                = FALSE
    // RemoveDuplicateGuids             = TRUE
    // Ignore list is emptied
    //--------------------------------------------------
    void LineOfSightSetup(guid           MovingObjGuid,
                          const Vector3& WorldStart,
                          const Vector3& WorldEnd);

    //--------------------------------------------------
    // Here are additional options to be applied after one of
    // the Setup functions is called
    //--------------------------------------------------

    // Set prefered maximum number of collisions to keep.
    // All defaults are 1 so only change this when you
    // intend on looping through the multiple collisions
    void SetMaxCollisions(int nMaxCollisions);

    // Add entries into the ignore list.  Remember that the
    // MovingObjGuid passed into the setup function will
    // always be ignored so you don't need to add it into
    // the ignore list.
    void AddToIgnoreList(guid Guid);
    void AddToIgnoreList(guid* pGuids, int nGuids);

    // Turn on collection of permeables
    void CollectPermeables();

    // Turn on low-poly collision.  Low poly collision or
    // the objects bbox will be used instead of the default
    // high poly collision
    void UseLowPoly();

    // Turn on ignore glass.  This will ignore rigid geometry
    // tagged as being glass.  Used by LOS.
    void IgnoreGlass();

    // Stop searching after first collision.  This is usefull
    // in cases when you only need A collision, not the closest.
    void StopOnFirstCollisionFound();

    void DoNotRemoveDuplicateGuids();

    //--------------------------------------------------
    // Collision checking
    //--------------------------------------------------

    bool CheckCollisions(Object::type ThisType = Object::TYPE_ALL_TYPES,
                         uint32_t     TheseAttributes = Object::ATTR_COLLIDABLE,
                         uint32_t     NotTheseAttributes = Object::ATTR_NULL);

    //--------------------------------------------------
    // Post collision-check queries
    //--------------------------------------------------

    // Permeable list
    int  GetNPermeables();
    guid GetPermeableGuid(int Index);
    void NotifyPermeables();

    //--------------------------------------------------
    // Apply functions
    //--------------------------------------------------

    void StartApply(guid           Guid,
                    const Matrix4& L2W,
                    const Matrix4& W2L);

    void StartApply(guid Guid);

    void EndApply();

    void ApplySphere(const Vector3& WorldPos,
                     float          Radius,
                     uint32_t       Flags = 0,
                     int            PrimitiveKey = -1);

    void ApplyTriangle(const Vector3& P0,
                       const Vector3& P1,
                       const Vector3& P2,
                       uint32_t       Flags = 0,
                       int            PrimitiveKey = -1);

    void ApplyQuad(const Vector3& P0,
                   const Vector3& P1,
                   const Vector3& P2,
                   const Vector3& P3,
                   uint32_t       Flags = 0,
                   int            PrimitiveKey = -1);

    void ApplyAABBox(const BBox& bb,
                     uint32_t    Flags = 0,
                     int         PrimitiveKey = -1);

    void ApplyOOBBox(const BBox&    LocalBBox,
                     const Matrix4& L2W,
                     uint32_t       Flags = 0,
                     int            PrimitiveKey = -1);

    //--------------------------------------------------

    guid GetMovingObjGuid() const;

    void EditorSelectRay(const Vector3& Start, const Vector3& End, bool bIncludeIcons);

    bool IsUsingHighPoly() { return !m_bUseLowPoly; }
    bool IsIgnoringGlass() { return m_bIgnoreGlass; }
    bool IsEditorSelectRay() { return m_bIsEditorSelectRay; }
    bool IsStopOnFirstCollision() { return m_bStopOnFirstCollisionFound; }

    //--------------------------------------------------
    // Generic functions
    //--------------------------------------------------

    // construct/destruct
    collision_mgr()
        : m_nCollisions(0)
        , m_nMaxCollisions(MAX_COLLISION_MGR_COLLISIONS)
        , m_bApplyStarted(false)
        , m_bUseIgnoreList(false)
        , m_bCollectPermeable(false)
        , m_bNotifyingPermeables(false)
        , m_bUseLowPoly(false)
        , m_bIgnoreGlass(false)
        , m_bStopOnFirstCollisionFound(false)
        , m_bIsRayCheck(false)
        , m_bIsEditorSelectRay(false)
        , m_bRemoveDuplicateGuids(true)
        , polyCache(nullptr)
        , objectManager(nullptr)
        , playsurfaceMgr(nullptr)
    {
        InitializeCollisionCheckDefaults();
    }

    ~collision_mgr() {};

    void setPolycache(poly_cache* pPolyCache)
    {
        polyCache = pPolyCache;
    }

    void setObjectManager(ObjectManager* pObjectManager)
    {
        objectManager = pObjectManager;
        g_PolyCache.setObjectManager(pObjectManager);   // IJB Hack
    }

    void setPlaysurfaceManager(PlaysurfaceMgr* psm)
    {
        playsurfaceMgr = psm;
    }

    // init
    void InitializeCollisionCheckDefaults();

    // Get moving object bounds
    primitive               GetDynamicPrimitive() const { return m_DynamicPrimitive; }
    const BBox&             GetDynamicBBox() const;
    const dynamic_cylinder& GetDynamicCylinder() const;
    const dynamic_ray&      GetDynamicRay() const;

    static int GetCylinderSpherePositions(const Vector3& Bottom,
                                          const Vector3& Top,
                                          float          Radius,
                                          Vector3*       SpherePositions,
                                          int            MaxNSpheres);

    void RecordCollision(const collision_mgr::collision& Collision);

    poly_cache* getPolyCache() { return polyCache; }

    //------------------------------------------------------------------------------
    // Private Functions
private:
    // static sphere to dynamic cylinder
    void ApplySphereToCylinder(const Vector3& WorldPos,
                               float          Radius,
                               uint32_t       Flags,
                               int            PrimitiveKey = -1);

    // static sphere to dynamic sphere
    void ApplySphereToSphere(const Vector3& WorldPos,
                             float          Radius,
                             uint32_t       Flags,
                             int            PrimitiveKey = -1);

    // static sphere to dynamic ray
    void ApplySphereToRay(const Vector3& WorldPos,
                          float          Radius,
                          uint32_t       Flags,
                          int            PrimitiveKey = -1);

    // static triangle to dynamic cylinder
    void ApplyTriangleToCylinder(const Vector3& P0,
                                 const Vector3& P1,
                                 const Vector3& P2,
                                 uint32_t       Flags,
                                 int            PrimitiveKey = -1);

    // static triangle to dynamic sphere
    void ApplyTriangleToSphere(const Vector3& P0,
                               const Vector3& P1,
                               const Vector3& P2,
                               uint32_t       Flags,
                               int            PrimitiveKey = -1);

    // static triangle to dynamic ray
    void ApplyTriangleToRay(const Vector3& P0,
                            const Vector3& P1,
                            const Vector3& P2,
                            uint32_t       Flags,
                            int            PrimitiveKey = -1);

    // static aabbox to dynamic cylinder
    void ApplyAABBoxToCylinder(const BBox& AABBox,
                               uint32_t    Flags,
                               int         PrimitiveKey = -1);

    // static aabbox to dynamic sphere
    void ApplyAABBoxToSphere(const BBox& AABBox,
                             uint32_t    Flags,
                             int         PrimitiveKey = -1);

    // static aabbox to dynamic ray
    void ApplyAABBoxToRay(const BBox& AABBox,
                          uint32_t    Flags,
                          int         PrimitiveKey = -1);

    void ApplyTriangleToStretchedSphere(const Vector3& P0,
                                        const Vector3& P1,
                                        const Vector3& P2,
                                        uint32_t       Flags,
                                        int            PrimitiveKey = -1);

    void ApplySphereToPolyCache();
    void ApplyCylinderToPolyCache();
    void ApplyRayToPolyCache();

    //------------------------------------------------------------------------------

    void ClearIgnoreList();
    bool IsInIgnoreList(guid Guid);

    void SortCollisions();
    void CleanPermeables();

    void UsePolyCache();

    //------------------------------------------------------------------------------
    // Hidden public functions
public:
    bool fn_CheckCollisions(Object::type ThisType = Object::TYPE_ALL_TYPES,
                            uint32_t     TheseAttributes = Object::ATTR_COLLIDABLE,
                            uint32_t     NotTheseAttributes = Object::ATTR_NULL);

    //------------------------------------------------------------------------------
    // Public Storage
public:
    // collisions
    collision m_Collisions[MAX_COLLISION_MGR_COLLISIONS];
    int       m_nCollisions;
    int       m_nMaxCollisions;

    //------------------------------------------------------------------------------
    //  Private Storage
private:
    //
    // moving primative stuff
    //
    primitive m_DynamicPrimitive;
    BBox      m_DynamicBBoxes[2];
    guid      m_MovingObjGuid;

    //
    // Other internals
    //
    bool                   m_bApplyStarted;
    dynamic_cylinder       m_CylinderInfo[2];
    dynamic_sphere         m_SphereInfo[2];
    dynamic_ray            m_RayInfo[2];
    collision_context_info m_ContextInfo;

    //
    // Ignore list
    //
    bool m_bUseIgnoreList;
    guid m_IgnoreList[MAX_IGNORED_OBJECTS];
    int  m_nIgnoredObjects;

    //
    // Permeable list
    //
    bool  m_bCollectPermeable;
    guid  m_Permeable[MAX_PERMEABLE_OBJECTS];
    float m_PermeableT[MAX_PERMEABLE_OBJECTS];
    int   m_nPermeables;
    bool  m_bNotifyingPermeables;

    //
    // Other options
    //
    bool m_bUseLowPoly;
    bool m_bIgnoreGlass;
    bool m_bStopOnFirstCollisionFound;
    bool m_bIsRayCheck;
    bool m_bIsEditorSelectRay;
    bool m_bRemoveDuplicateGuids;
    bool m_bUsePolyCache;

    //
    // Filters used during CheckCollisions
    //
    Object::type m_FilterThisType;
    uint32_t     m_FilterTheseAttributes;
    uint32_t     m_FilterNotTheseAttributes;

    poly_cache* polyCache;
    ObjectManager* objectManager;
    PlaysurfaceMgr* playsurfaceMgr;
};

extern collision_mgr g_CollisionMgr;

#include "CollisionMgr_Private.h"
