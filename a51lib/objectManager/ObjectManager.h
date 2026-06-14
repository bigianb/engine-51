#pragma once

#include "../Guid.h"
#include "../Property.h"
#include "../objects/Object.h"
#include "../xfiles/x_time.h"
#include <vector>
#include <string>

class spatial_dbase;
class view;
class ObjectManager;
class collision_mgr;
class PlaysurfaceMgr;

class object_desc : public prop_interface
{
public:
    const char*  GetTypeName() const { return m_pTypeName; }
    Object::type GetType() const { return m_Type; };

    bool UseInEditor() const
    {
        return (m_Flags & FLAGS_GENERIC_EDITOR_CREATE) == FLAGS_GENERIC_EDITOR_CREATE;
    }
    bool CanRender() const { return m_bRender &&
                                    ((m_Flags & FLAGS_NO_EDITOR_RENDERABLE) != FLAGS_NO_EDITOR_RENDERABLE); }
    bool IsDynamic() const { return (m_Flags & FLAGS_IS_DYNAMIC) == FLAGS_IS_DYNAMIC; }
    bool TargetsOtherObjects() const { return (m_Flags & FLAGS_TARGETS_OBJS) == FLAGS_TARGETS_OBJS; }
    bool NoAllowEditorCopy() const { return (m_Flags & FLAGS_NO_ALLOW_COPY) == FLAGS_NO_ALLOW_COPY; }
    bool IsGlobalObject() const { return (m_Flags & FLAGS_IS_GLOBAL) == FLAGS_IS_GLOBAL; }
    bool IsBurnVertexLighting() const
    {
        return (m_Flags & FLAGS_BURN_VERTEX_LIGHTING) == FLAGS_BURN_VERTEX_LIGHTING;
    }
    bool IsIconSelect() const { return (m_Flags & FLAGS_NO_ICON) != FLAGS_NO_ICON; }
    bool IsTempObject() const { return !!(m_Flags & FLAGS_EDITOR_TEMP); }
    bool HasLogic() const
    {
        return m_bLogic && (m_CommonAttrBits & Object::ATTR_NEEDS_LOGIC_TIME);
    }

protected:
    virtual Object* Create(ObjectManager*, collision_mgr*, ResourceManager*) = 0;
    void            AddObjectCount(int n) const { m_ObjectCount += n; }
    virtual bool    OnBeginRender() { return CanRender() && m_CommonAttrBits & Object::ATTR_RENDERABLE; }
    virtual void    OnEndRender() {}
    virtual bool    OnBeginLogic() { return m_bLogic && m_CommonAttrBits & Object::ATTR_NEEDS_LOGIC_TIME; }
    virtual void    OnEndLogic() {}

    virtual void OnEnumProp(prop_enum& List);
    virtual bool OnProperty(prop_query& I);

    object_desc(Object::type Type,
                const char*  pTypeName,
                const char*  pEditorGroupName,
                uint32_t     CommonAttrBit,
                uint32_t     TypeBits = 0);

    enum flags
    {
        FLAGS_GENERIC_EDITOR_CREATE = (1 << 0),
        FLAGS_NO_ALLOW_COPY = (1 << 1),
        FLAGS_IS_DYNAMIC = (1 << 2),
        FLAGS_IS_GLOBAL = (1 << 3),
        FLAGS_BURN_VERTEX_LIGHTING = (1 << 5),
        FLAGS_NO_ICON = (1 << 6),
        FLAGS_NO_EDITOR_RENDERABLE = (1 << 7),
        FLAGS_TARGETS_OBJS = (1 << 8),
        FLAGS_EDITOR_TEMP = (1 << 9),
    };

    const char*  m_pEditorGroupName;
    const char*  m_pTypeName;
    Object::type m_Type;
    object_desc* m_pNextGroupType;
    uint32_t     m_CommonAttrBits;
    bool         m_bRender;
    bool         m_bLogic;
    bool         m_bDisplayLocation;
    bool         m_bTrapOnCreate;
    bool         m_bTrapOnDestroy;
    bool         m_bRenderHiCollision;
    bool         m_bRenderLoCollision;
    mutable int  m_ObjectCount;
    uint32_t     m_Flags;

    friend class ObjectManager;
};

typedef uint16_t slot_id;
typedef int      link_id;

#define SLOT_NULL ((uint16_t)0xffff)
#define LINK_NULL (-1)

struct obj_cell_link
{
    uint16_t SpacialCellID;
    link_id  Next; // Next link in the list. (link_id/8) == slot_id
    link_id  Prev; // Prev link in the list. (link_id/8) == slot_id
};

class ObjectRegistrarInterface
{
public:
    virtual ~ObjectRegistrarInterface() {}
    virtual void RegisterObjects(std::vector<const object_desc*>& objectDescriptors) = 0;
};

class ObjectManager
{
public:
    enum
    {
        MAX_OBJECTS = 60000,
        MAX_REF_NODES = MAX_OBJECTS
    };

    enum
    {
        MAX_VISIBLE_SHADOW_PROJECTORS = 256,
    };

    enum query_types
    {
        QUERY_NULL = 0,
        QUERY_VIS,
        QUERY_BBOX,
        QUERY_RAY,
        QUERY_VOLUME,
        QUERY_ATTRIBUTE
    };

    ObjectManager();
    ~ObjectManager();
    void Init(ObjectRegistrarInterface* objectRegistrar, spatial_dbase*, collision_mgr* cm, ResourceManager*, PlaysurfaceMgr*);
    void Kill();

    xtick GetGameTime() const { return m_GameTime; };
    float GetGameDeltaTime(xtick LastTime) const
    {
        int64_t Delta = (m_GameTime - LastTime) / x_GetTicksPerMs();
        float   Time = ((float)Delta) * (1 / 1000.0f);
        return Time;
    }

    void Render(bool DoPortalWalk, const view& PortalView, uint8_t StartZone, Renderer* renderer);

    void Render3dObjects(bool DoPortalWalk, const view& PortalView, uint8_t StartZone, Renderer* renderer);

protected:
    void DoVisibilityTests(const view& View);
    void CollectShadowCasters();
    void CompleteVisAndShadowTests();
    void CreateShadowMap();
    void Render3dPrep(bool DoPortalWalk, const view& PortalView, uint8_t StartZone);
    void RenderNormalObjects();
    void RenderPlaySurfaces();
    void Render2dObjects();
    void RenderSpecialObjects();

public:
    void ReserveGuid(guid Guid);
    guid CreateObject(const char* objectTypeName);
    void CreateObject(const char* objectTypeName, guid Guid);
    guid CreateObject(const object_desc& desc);
    void CreateObject(const object_desc& desc, guid Guid);

    void DestroyObject(guid ObjectGuid) { DestroyObjectEx(ObjectGuid, false); }
    void DestroyObjectEx(guid ObjectGuid, bool bRemoveNow);

    void ChangeObjectGuid(guid CurrentGuid, guid NewGuid);

    Object* GetObjectBySlot(slot_id SlotID)
    {
        if (SlotID == SLOT_NULL) {
            return nullptr;
        }

        return m_ObjectSlot[SlotID].pObject;
    }

    Object* GetObjectByGuid(guid Guid);

    //  Gets the first object of this type
    slot_id GetFirst(Object::type Type)
    {
        return m_ObjectType[Type].FirstType;
    };

    slot_id GetNext(slot_id SlotID); //  Gets the next object of the same type

    const object_desc* GetDescFromName(const char* objectTypeName) const;

    void RemoveFromSpatialDBase(slot_id SlotID);
    void AddToSpatialDBase(slot_id SlotID);
    void RemoveFromSpatialDBase(guid Guid);
    void AddToSpatialDBase(guid Guid);
    void UpdateSpatialDBase(slot_id SlotID, const BBox& OldBBox);

    void AddSlotToZone(slot_id SlotID);
    void AddSlotToRenderable(slot_id SlotID);
    void RemoveSlotFromZone(slot_id SlotID);
    void RemoveSlotFromRenderable(slot_id SlotID);

    slot_id GetSlotFromGuid(guid Guid);

    int GetNLogicLoops() { return m_nLogicLoops; }

    const object_desc* GetDescFromName(const char* pObjectTypeName);
    const object_desc* GetTypeDesc(Object::type Type) { return m_ObjectType[Type].pDesc; }
    const BBox&        GetSafeBBox(void) { return m_SafeBBox; }
    void               SetSafeBBox(const BBox& SafeBBox) { m_SafeBBox = SafeBBox; }
    void               InflateSafeBBox(float Inflate) { m_SafeBBox.Inflate(Inflate, Inflate, Inflate); }

    // Queries

    //check LOS from point to point (ignores permeable and living)
    bool HasLOS(guid Object0, const Vector3& P0, guid Object1, const Vector3& P1);

    // Selects all objects whose bbox intersects the bbox provided
    void SelectBBox(uint32_t Attribute, const BBox& bb, Object::type Type = Object::TYPE_ALL_TYPES, uint32_t NotTheseAttributes = 0x00000000);

    // Selects all objects whose bbox is intersected by the line segment provided
    void SelectRay(uint32_t Attribute, const Vector3& RayStart, const Vector3& RayEnd, Object::type Type = Object::TYPE_ALL_TYPES, uint32_t notTheseAttributes = 0x00000000);

    // Selects all objects whose bboxes are within the volume specified by the front
    // sides of the planes provided. The planes must describe a convex volume.
    // You can use up to 16 planes to describe your volume.
    // When using the fast option it's possible to get false positives.
    void SelectVolume(uint32_t Attribute, const plane* pPlane, int NPlanes, bool Fast = true, Object::type Type = Object::TYPE_NULL);

    //  Selects only objects with specific attributes but without geometry constraints
    void SelectByAttribute(uint32_t Attribute, Object::type Type = Object::TYPE_NULL);

    bool    IsLoopActive() { return m_InLoop; }
    slot_id StartLoop()
    {
        assert(!m_InLoop);
        m_InLoop = true;

        return m_FirstSearchResult;
    };

    slot_id GetNextResult(slot_id SlotID) { return m_ObjectSlot[SlotID].NextSearch; }
    void    EndLoop()
    {
        assert(m_InLoop);
        m_InLoop = false;
    }
    void ResetSearchResult();

    query_types GetLastQueryType();

    void SetNextSearchResult(slot_id SlotID);

    slot_id        GetFirstVis(Object::type Type);
    slot_id        GetNextVis(slot_id SlotID);
    obj_cell_link* GetCellLinks(void) { return m_ObjectLink; }
    int            IsBoxInView(const BBox& bb,
                               uint32_t    CheckPlaneMask) const;

protected:
    Object* CreateObject(const object_desc& Desc, slot_id Slot);
    slot_id AllocSlot();
    void    UnlinkAndFreeObject(slot_id Slot);

private:
    // For keeping stats on each object type.
    struct obj_type_node
    {
        int                InstanceCount; // How many of these do we have
        slot_id            FirstType;     // First of this type in the obj_slot list
        slot_id            FirstVis;      // First visible of this type
        int                nVis;          // Number visible of this type
        const object_desc* pDesc;         // Description of object type
    };

    //  This is used to store a list of all the objects currently in use in
    //  the game.  As objects are created an obj_slot is filled with info about
    //  the objects.
    struct obj_slot
    {
        Object* pObject;           // The only place that actually holds a pointer to the object
        slot_id Next;              // Points to index of next object of this type
        slot_id Prev;              // Points to index of previous object of this type
        slot_id NextSearch;        // Points to next object in a search result
        slot_id NextVis;           // Points to next object in a visibility result
        slot_id NextRenderable[2]; // Points to index of next renderable in the zone
        slot_id PrevRenderable[2]; // Points to index of previous renderable in the zone
        slot_id NextZone[2];       // Points to index of next object in the zone
        slot_id PrevZone[2];       // Points to index of next object in the zone
        int     Sequence;          // Sequence number
    };

    // This is used to store a list of objects connected to a zone
    struct obj_zone
    {
        slot_id FirstZone[2];
        slot_id FirstRenderable[2];
    };

private:
    std::vector<const object_desc*> objectDescriptors;

    std::vector<slot_id> m_DeleteObject;

    obj_type_node m_ObjectType[Object::TYPE_END_OF_LIST]; //  Info about each type of object      Size = 40*numOfObjects
    obj_slot      m_ObjectSlot[MAX_OBJECTS];              //  The array that holds all pointers   Size = 14*4096
    obj_cell_link m_ObjectLink[MAX_OBJECTS * 8];          //  for the spatial structure           Size = 48*4096
    obj_zone      m_ObjectZone[256];                      //  for the zone linked-list            Size = 4*255

    GuidLookup m_GuidLookup; //  Guid lookup object

    slot_id m_FirstFreeSlot; //  index to fist free slot                     2
    slot_id m_FirstSearchResult;
    bool    m_InLoop;
    int     m_Sequence;

    // Next 3 members are used for visibility checks
    plane m_Plane[6 * 2];
    int   m_PlaneMinIndex[6 * 2 * 3];
    int   m_PlaneMaxIndex[6 * 2 * 3];

    BBox m_SafeBBox;
    guid m_ReservedGuid;

    int m_nLogicLoops;

    spatial_dbase*   spatialDatabase;
    collision_mgr*   collisionMgr;
    ResourceManager* resourceManager;
    PlaysurfaceMgr*  playsurfaceManager;

    xtick m_GameTime;
};
