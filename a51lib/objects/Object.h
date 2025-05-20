#pragma once
#include <cassert>
#include "../Property.h"
#include "../RTTI.h"
#include "../render/CollisionVolume.h"
#include "render/RenderInst.h"
#include "../animation/animData.h"

class object_desc;
class pain;
struct event;
class ObjectManager;
class simple_anim_player;

#define MAX_OBJECT_NAME_LENGTH 32

class Object : public prop_interface
{
public:
    CREATE_RTTI_BASE(Object);

    Object(ObjectManager* om) : objectManager(om){}

    enum type
    {
        TYPE_NULL = 0,
        TYPE_LIGHT,
        TYPE_CHARACTER_LIGHT,
        TYPE_TEAM_LIGHT,
        TYPE_DYNAMIC_LIGHT,
        TYPE_PROJECTOR,

        TYPE_TRIGGER,
        TYPE_TRIGGER_EX,

        // The audio driven cinema object needs to be above ALL THE OBJECTS IT CAN CONTROL
        // so that all cinema objects stay perfectly in sync and WYSIWYG from max.
        TYPE_AUDIO_DRIVEN_CINEMA,

        // Trackers are placed early so they can move objects before the object
        // runs it's logic.
        TYPE_TRACKER,
        TYPE_CONTROLLER,

        TYPE_PLAY_SURFACE,
        TYPE_ANIM_SURFACE,
        TYPE_PROP_SURFACE,
        TYPE_REACTIVE_SURFACE,

        TYPE_COUPLER, //-- Do not move the coupler has been placed here and both fixes (couple to anim ans well as elevator to player uses )

        TYPE_NPC,
        TYPE_NAVNODE,
        TYPE_NET_GHOST,

        TYPE_LENS_FILTER,
        TYPE_SND_EMITTER,
        TYPE_PICKUP,
        TYPE_DOOR,
        TYPE_NAV_NODE_PLACE_HOLDER,
        TYPE_NAV_CONNECTION_PLACE_HOLDER,
        TYPE_EDITOR_STATIC_DECAL,
        TYPE_EDITOR_BLUEPRINT_ANCHOR,
        TYPE_EDITOR_NOTEPAD_OBJECT,
        TYPE_MARKER_OBJECT,
        TYPE_LOCOMOTION_OBJECT,
        TYPE_LOCOMOTION_TESTER,
        TYPE_SPAWN_POINT,

        TYPE_CORPSE,
        TYPE_GHOST_IMAGE,
        TYPE_ZONE_PORTAL,
        TYPE_NAV_CONNECTION_EDITOR,
        TYPE_NAV_NODE_EDITOR,
        TYPE_COVER_NODE,
        TYPE_ALARM_NODE,
        TYPE_SPATIAL_TRIGGER,
        TYPE_VIEWABLE_SPATIAL_TRIGGER,
        TYPE_STATIC_DECAL_OBJECT,
        TYPE_SKIN_PROP_ANIM_SURFACE,
        TYPE_MUTAGEN_RESERVOIR,

        TYPE_BLACK_OPPS,
        TYPE_HAZMAT,
        TYPE_GRUNT,
        TYPE_MUTANT_TANK,
        TYPE_GOD,
        TYPE_STAGE5,
        TYPE_GRAY,
        TYPE_GENERIC_NPC,
        TYPE_TURRET,
        TYPE_DEBRIS,
        TYPE_DEBRIS_RIGID,
        TYPE_DEBRIS_GLASS,
        TYPE_DEBRIS_FRAG_EXPLOSION,
        TYPE_DEBRIS_ALIEN_GRENADE_EXPLOSION,
        TYPE_DEBRIS_MESON_EXPLOSION,

        TYPE_LADDER_FIELD,
        TYPE_SPAWNER_OBJECT,
        TYPE_SIMPLE_SND_EMITTER,
        TYPE_FRIENDLY_SCIENTIST,
        TYPE_FRIENDLY_SOLDIER,
        TYPE_PLAYER,

        TYPE_PARTICLE,
        TYPE_PARTICLE_EVENT_EMITTER,

        TYPE_WEAPON,
        TYPE_WEAPON_SMP,
        TYPE_WEAPON_DUAL_SMP,
        TYPE_WEAPON_DUAL_SHT,
        TYPE_WEAPON_SHOTGUN,
        TYPE_WEAPON_SCANNER,
        TYPE_WEAPON_SNIPER,
        TYPE_WEAPON_GAUSS,
        TYPE_WEAPON_DESERT_EAGLE,
        TYPE_WEAPON_FRAG_GRENADE,
        TYPE_WEAPON_MUTATION,
        TYPE_WEAPON_MHG,
        TYPE_WEAPON_GRAV_CHARGE,
        TYPE_WEAPON_MSN,
        TYPE_WEAPON_BBG,
        TYPE_WEAPON_TRA,

        PROJECTILE_BEGIN,
        TYPE_SPIKE_PROJECTILE,
        TYPE_BULLET_PROJECTILE,
        TYPE_GRENADE_PROJECTILE,
        TYPE_JUMPING_BEAN_PROJECTILE,
        TYPE_GRAV_CHARGE_PROJECTILE,
        TYPE_ENERGY_PROJECTILE,
        TYPE_SEEKER_PROJECTILE,
        TYPE_MESONSEEKER_PROJECTILE,
        TYPE_HOMING_PROJECTILE,
        TYPE_MUTANT_PARASITE_PROJECTILE,
        TYPE_MUTANT_CONTAGION_PROJECTILE,
        TYPE_MUTANT_TENDRIL_PROJECTILE,
        PROJECTILE_END,

        TYPE_CLOTH_OBJECT,
        TYPE_CHARACTER_TASK,

        TYPE_INVENTORY_ITEM,
        TYPE_INPUT_SETTINGS,
        TYPE_LEVEL_SETTINGS,
        TYPE_GLASS_SURFACE,

        TYPE_MANIPULATOR,
        TYPE_ANIMATION_OBJECT,
        TYPE_FOCUS_OBJECT,
        TYPE_LORE_OBJECT,
        TYPE_HUD_OBJECT,

        TYPE_PLAY_SURFACE_PROXY,

        TYPE_EVENT_SND_EMITTER,
        TYPE_DAMAGE_FIELD,

        TYPE_DESTRUCTIBLE_OBJ,

        TYPE_NAV_CONNECTION2_EDITOR,
        TYPE_NAV_CONNECTION2_ANCHOR,

        TYPE_PATH,
        TYPE_CAMERA,
        TYPE_PIP,
        TYPE_THIRD_PERSON_CAMERA,
        TYPE_NAV_POINT,
        TYPE_EXPLOSIVE_BULLET_PROJECTILE,

        TYPE_INVENTORY_HEALTH_ITEM,

        TYPE_INVENTORY_MUTAGEN_ITEM,

        TYPE_ALIEN_TURRET_PROJECTILE,
        TYPE_ALIEN_ORB,
        TYPE_ALIEN_SPOTTER,
        TYPE_ALIEN_ORB_SPAWNER,
        TYPE_ALIEN_SPAWN_TUBE,
        TYPE_ALIEN_GLOB,
        TYPE_ALIEN_SHIELD,

        TYPE_GZ_CORE_OBJ,
        TYPE_FEEDBACK_EMITTER,

        TYPE_VOLUMETRIC_LIGHT,
        TYPE_BOMB,
        TYPE_COKE_CAN,
        TYPE_SUPER_DESTRUCTIBLE_OBJ,

        TYPE_INVISIBLE_WALL_OBJ,

        TYPE_DEBRIS_GLASS_CLUSTER,

        TYPE_GROUP,

        TYPE_ALIEN_PLATFORM,
        TYPE_ALIEN_PLATFORM_DOCK,

        // BEGIN - Multiplayer specific objects.

        TYPE_GAME_PROP,
        TYPE_FLAG,
        TYPE_BLUEPRINT_BAG,
        TYPE_MP_SETTINGS,
        TYPE_JUMP_PAD,
        TYPE_TELEPORTER,
        TYPE_TEAM_PROP,
        TYPE_FORCE_FIELD,
        TYPE_CAP_POINT,
        TYPE_FLAG_BASE,
        // END   - Multiplayer specific objects.

        TYPE_END_OF_LIST,
        TYPE_ALL_TYPES = 0xFFFFFFFF
    };

#define BIT(x) (1 << (x))

    enum object_attr
    {
        ATTR_NULL = 0,
        ATTR_NEEDS_LOGIC_TIME = BIT(0),          // This flag indicates that this object needs time for logic
        ATTR_DRAW_2D = BIT(1),                   // Objects that are rendered in 2D.
        ATTR_COLLIDABLE = BIT(2),                // This object can collide with other objects
        ATTR_RENDERABLE = BIT(3),                // This object is actually renderable in the normal game
        ATTR_TRANSPARENT = BIT(4),               // This is use with the render flag to indicate whether this objects uses a specialice pipe line.
        ATTR_PLAYER = BIT(5),                    // This is a player, either local human player, remote human player, or monkey
        ATTR_DAMAGEABLE = BIT(6),                // This object can be destroyed
        ATTR_SOUND_SOURCE = BIT(7),              // This object can create emit sounds
        ATTR_SPACIAL_ENTRY = BIT(8),             // Obj mgr knows when to add/remove from the spacial
        ATTR_DESTROY = BIT(9),                   // Flag should be set when the object is marked for death
        ATTR_NO_RUNTIME_SAVE = BIT(10),          // This dynamic object should not be saved as part of save load
        ATTR_COLLISION_PERMEABLE = BIT(11),      // This flag makes a collidable object not stop movement of other objects
        ATTR_EDITOR_SELECTED = BIT(12),          // This flag is set when the editor selects an object.
        ATTR_EDITOR_BLUE_PRINT = BIT(13),        // This flags is set if the object is part of blue print.
        ATTR_EDITOR_TEMP_OBJECT = BIT(14),       // This object is a temporary editor only object (not exported)
        ATTR_EDITOR_PLACEMENT_OBJECT = BIT(15),  // This object is a temporary editor only object used for placement
        ATTR_LIVING = BIT(16),                   // This object is a living type object.  Should inclue all things that inherit from actor
        ATTR_CHARACTER_OBJECT = BIT(17),         // This object is derived from the character class
        ATTR_DISABLE_PROJ_SHADOWS = BIT(18),     // This object cannot receive projector shadows (artist-placed)
        ATTR_CAST_SHADOWS = BIT(19),             // This object can cast dynamic shadows
        ATTR_RECEIVE_SHADOWS = BIT(20),          // This object can receive dynamic shadows
        ATTR_DESTRUCTABLE_OBJECT = BIT(21),      // This object is a destructible or superdestructable object
        ATTR_ACTOR_RIDEABLE = BIT(22),           // This object will apply collision data to bullets NOTE!!! Only use this on decendants of anim_surface for now.  CharacterPhysics does some blind casts, and we would need to implement GetBoneL2W at the object level to support this anywhere
        ATTR_BLOCKS_PLAYER = BIT(23),            // This object blocks player movement
        ATTR_BLOCKS_CHARACTER = BIT(24),         // This object blocks character movement
        ATTR_BLOCKS_RAGDOLL = BIT(25),           // This object blocks ragdoll movement
        ATTR_BLOCKS_SMALL_PROJECTILES = BIT(26), // This object blocks small projectiles
        ATTR_BLOCKS_LARGE_PROJECTILES = BIT(27), // This object blocks large projectiles
        ATTR_BLOCKS_CHARACTER_LOS = BIT(28),     // This object blocks character LOS
        ATTR_BLOCKS_PLAYER_LOS = BIT(29),        // This object blocks player LOS i.e. AimAssist
        ATTR_BLOCKS_PAIN_LOS = BIT(30),          // This object blocks pain LOS
        ATTR_BLOCKS_SMALL_DEBRIS = BIT(31),      // This object blocks small debris movement
        ATTR_ALL = 0xFFFFFFFF,                   // All attributes
        ATTR_BLOCKS_ALL_PROJECTILES = (ATTR_BLOCKS_SMALL_PROJECTILES | ATTR_BLOCKS_LARGE_PROJECTILES),
        ATTR_BLOCKS_ALL_ACTORS = (ATTR_BLOCKS_PLAYER | ATTR_BLOCKS_CHARACTER),
    };

    enum material_type
    {
        MAT_TYPE_NULL = 0,
        MAT_TYPE_EARTH,
        MAT_TYPE_ROCK,
        MAT_TYPE_CONCRETE,
        MAT_TYPE_SOLID_METAL,
        MAT_TYPE_HOLLOW_METAL,
        MAT_TYPE_METAL_GRATE,
        MAT_TYPE_PLASTIC,
        MAT_TYPE_WATER,
        MAT_TYPE_WOOD,
        MAT_TYPE_ENERGY_FIELD,
        MAT_TYPE_BULLET_PROOF_GLASS,
        MAT_TYPE_ICE,

        MAT_TYPE_LEATHER,
        MAT_TYPE_EXOSKELETON,
        MAT_TYPE_FLESH,
        MAT_TYPE_BLOB,

        MAT_TYPE_FIRE,
        MAT_TYPE_GHOST,
        MAT_TYPE_FABRIC,
        MAT_TYPE_CERAMIC,
        MAT_TYPE_WIRE_FENCE,

        MAT_TYPE_GLASS,
        MAT_TYPE_RUBBER,

        MAT_TYPE_CARPET,
        MAT_TYPE_CLOTH,
        MAT_TYPE_DRYWALL,
        MAT_TYPE_FLESHHEAD,
        MAT_TYPE_MARBLE,
        MAT_TYPE_TILE,

        MAT_TYPE_LAST,
    };

    struct detail_tri
    {
        Vector3                Vertex[3];
        Vector3                Normal[3];
        Vector2                UV[3];
        Colour                 Color[3];
        CollisionData::MatInfo MaterialInfo;
    };

    uint16_t GetZones() const { return m_ZoneInfo; }
    uint16_t GetZone1() const { return m_ZoneInfo & 0xff; }
    uint16_t GetZone2() const { return (m_ZoneInfo >> 8) & 0xff; }

    /*
    // PUBLIC MESSAGES
    //------------------------------------------------------------------------------
    // Public messages are messages that objects can send to other objects including
    // them selves. When handling the message must call the parent class version.
    //------------------------------------------------------------------------------
    //
    // OnMove        - Sets the absolute position for the object.
    //
    // OnMoveRel     - Moves the object relative to current pos.
    //
    // OnTransfrom   - Moves/Rotate/Scale Object. Note that this is the most optimal function.
    //
    // OnTriggerTransform - Some objects need to re-initialize some of their internal data when moved
    //                      through the trigger system.  This allows those objects to do so.
    //
    // OnEnumProp           - Enumerates all the properties that can be saved/loaded/edited.
    //
    // OnProperty           - Read/writes property. Can be called from UI edit, or save/load.
    //
    // OnValidateProperties - Used by editor only to make sure properties are valid.
    //
    // OnLoad        - Loads an object. Note it has to came from a text_in. Also the property has the initial onload
    //
    // OnColNotify  - Some one is colling with you. The object is past as a parameter.
    //
    // OnActivate   - Used within the Trigger system to notifiy an object to activate
    //
    */
public:
    virtual void OnMove(const Vector3& NewPos);
    virtual void OnMoveRel(const Vector3& DeltaPos);
    virtual void OnTransform(const Matrix4& L2W);
    virtual void OnTriggerTransform(const Matrix4& L2W);
    virtual void OnEnumProp(prop_enum& List);
    virtual bool OnProperty(prop_query& I);
    virtual void OnLoad(text_in& TextIn);
    virtual void OnPaste(const std::vector<prop_container>& Container);

    virtual void OnPain(const pain& Pain) {} // Tells object to recieve pain
    virtual bool OnChildPain(guid ChildGuid, const pain& Pain) { return false; }

    virtual void    OnColNotify(Object& ) { }
    virtual void    OnActivate(bool Flag);
    virtual void    OnEvent(const event& ) {}
    virtual Vector3 GetSubPosition(int ID) { return m_L2W.GetTranslation(); }

    virtual bool        GetColDetails(int Key, detail_tri& Tri) { return false; }
    virtual void        OnPolyCacheGather(void) {}
    virtual const char* GetLogicalName(void);

    virtual void OnAddedToGroup(guid gGroup) {}

    //------------------------------------------------------------------------------
    // PUBLIC FUNCTIONS
    //------------------------------------------------------------------------------
    //
    // GetLocalBBox    - User must provide a way to get the local bbox
    //
    // GetType         - Gets the object type. Must be provided by the user.
    //
    // GetBBox         - Gets the world bbox
    //
    // GetPosition     - Gets the position of the object
    //
    // GetVelocity     - Gets the movement velocity of the object
    //
    // ScaleVelocity   - Scales velocity along a plane
    //                         PlaneNormal = Collision plane
    //                         PerpScale   = Scales velocity along plane normal
    //                         ParaScale   = Scales velocity parallel to plane
    //
    // GetLookAtExtent - Gets the fraction of the height where the object should be looked at
    //
    // GetL2W          - Get the Local to World Matrix
    //
    // GetW2L          - Gets the World to Local Matrix
    //
    // OnDebugRender   - Renders the object in a non-standard way. Default is just a bbox.
    //
    // GetAttrBits     - Get the attribute bits
    //
    // SetAttrBits     - Overwrites the attribute bits with a new set
    //
    // GetFlagBits     - Get the flag bits
    //
    // SetFlagBits     - Overwrites the flag bits with a new set
    //
    // GetGuid         - Gets the guid of the object
    //
    // EnumAttachPoints         - Returns a string of attachpoints formatted suitably for
    //                            the AddEnum property call
    //
    // GetAttachPointIDByName   - Gets the s32 ID of an attach point
    //
    // GetAttachPointNameByID   - Gets the string name of an attach point
    //
    // GetAttachPointData       - Retrieves a matrix describing the attach point
    //
    // OnAttachedMove           - Performs an attached move operation
    //
    //------------------------------------------------------------------------------
public:
    virtual BBox               GetLocalBBox() const = 0;
    virtual int                GetMaterial() const = 0;
    virtual const object_desc& GetTypeDesc() const = 0;
    virtual float              GetLookAtExtent() const { return .95f; }
    type                       GetType() const;

    ObjectManager* getObjectManager() const { return objectManager; }

    const BBox& GetBBox()
    {
        if (m_FlagBits & FLAG_DIRTY_TRANSFORM) {
            UpdateTransform();
        }

        return m_WorldBBox;
    }
    virtual BBox GetColBBox() { return GetBBox(); }
    //virtual BBox    GetScreenBBox(const view& rView);
    Vector3         GetPosition() const { return m_L2W.GetTranslation(); }
    virtual Vector3 GetVelocity() const { return Vector3(0.0f, 0.0f, 0.0f); }
    virtual void    ScaleVelocity(const Vector3& PlaneNormal, float PerpScale, float ParaScale) {}

    const Matrix4& GetL2W() const { return m_L2W; }

    uint32_t GetRenderMode() const { return 0; }
    uint32_t GetAttrBits() const { return m_AttrBits; }
    void     SetAttrBits(uint32_t NewBits)
    {
        if ((NewBits & Object::ATTR_RENDERABLE) != (m_AttrBits & Object::ATTR_RENDERABLE)) {
            m_AttrBits = NewBits;
            UpdateRenderableLinks();
        } else {
            m_AttrBits = NewBits;
        }
    }
    void         TurnAttrBitsOn(uint32_t BitsToTurnOn) { SetAttrBits(m_AttrBits | BitsToTurnOn); }
    void         TurnAttrBitsOff(uint32_t BitsToTurnOff) { SetAttrBits(m_AttrBits & (~BitsToTurnOff)); }
    uint32_t     GetFlagBits() const { return m_FlagBits; }
    void         SetFlagBits(uint32_t NewBits) { m_FlagBits = NewBits; }
    virtual bool IsActive() { return (m_AttrBits & ATTR_NEEDS_LOGIC_TIME) != 0; }

    guid GetGuid() const { return m_Guid; }
    int  GetSlot() const { return m_SlotID; }

    void SetZones(uint16_t Zones)
    {
        SetZone1(Zones & 0xff);
        SetZone2((Zones & 0xff00) >> 8);
    }
    void SetZone1(uint16_t Zone1)
    {
        uint16_t NewZoneInfo = (m_ZoneInfo & 0xff00) | Zone1;
        if (NewZoneInfo != m_ZoneInfo) {
            SetNewZoneInfo(NewZoneInfo);
        }
    }
    void SetZone2(uint16_t Zone2)
    {
        uint16_t NewZoneInfo = (m_ZoneInfo & 0xff) | (Zone2 << 8);
        if (NewZoneInfo != m_ZoneInfo) {
            SetNewZoneInfo(NewZoneInfo);
        }
    }

    void LoadStart();
    void LoadEnd();

    // Life functions
    virtual bool IsAlive()
    {
        float Health = GetHealth();
        return (Health > 0.0f);
    }
    virtual float GetHealth() { return 100.0f; }

    enum
    {
        ATTACH_USE_WORLDSPACE = 1, // Return the attachpoint in worldspace
    };

    virtual void        EnumAttachPoints(std::string& String) const;
    virtual int         GetAttachPointIDByName(const char* pName) const;
    virtual std::string GetAttachPointNameByID(int iAttachPt) const;
    virtual bool        GetAttachPointData(int      iAttachPt,
                                           Matrix4& L2W,
                                           uint32_t Flags = 0);
    virtual void        OnAttachedMove(int            iAttachPt,
                                       const Matrix4& L2W);

    virtual guid GetParentGuid() { return 0; }

    virtual render_inst* GetRenderInstPtr() { return nullptr; }
    virtual Geom*        GetGeomPtr();
    virtual std::string GetGeomName();

    virtual simple_anim_player* GetSimpleAnimPlayer() { return nullptr; }
    virtual AnimGroup::handle* GetAnimGroupHandlePtr() { return nullptr; }
    virtual AnimGroup*         GetAnimGroupPtr();
    virtual std::string GetAnimGroupName();

    const char* GetName();
    void        SetName(const char* pNewObjectName);

    virtual int net_GetSlot() const { return -1; }

protected:
    ObjectManager* objectManager;

    enum flags
    {
        FLAG_CHECK_PLANES = 0x3F,
        FLAG_CHECK_PLANES_SHIFT = 0,
        FLAG_LOADING = 0x10,
        FLAG_DIRTY_TRANSLATION = 0x20,
        FLAG_DIRTY_ROTATION = 0x30,
        FLAG_DIRTY_TRANSFORM = FLAG_DIRTY_TRANSLATION | FLAG_DIRTY_ROTATION,
    };
    // Protected messages are messages that only the system can send to objects.
    // All messages are hirarchical so user must notify parent class.
    virtual void OnInit() {}
    virtual void OnKill() {}
    virtual void OnAdvanceLogic(float DeltaTime) {}
    virtual void OnSpatialUpdate();
    virtual void OnColCheck();
    virtual void OnRender() { assert(false); }
    virtual void OnRenderShadowCast(uint64_t ProjMask) {}
    virtual void OnRenderShadowReceive(uint64_t ProjMask) {}

    virtual void OnRenderTransparent() {}
    bool         NeedsClipping() { return (m_FlagBits & FLAG_CHECK_PLANES) != 0; }
    virtual void OnAnimEvent() { assert(false); }

    void SetTransform(const Matrix4& L2W);
    void UpdateTransform();
    bool IsLoading() { return (m_FlagBits & FLAG_LOADING) != 0; }

    void SetNewZoneInfo(uint16_t zoneInfo);
    void UpdateRenderableLinks();

private:
    bool HandleAttribute(prop_query& I, const char* PropName, uint32_t AttrBit);

    uint32_t m_AttrBits;
    guid     m_Guid;
    Matrix4  m_L2W;
    BBox     m_WorldBBox;
    uint32_t m_FlagBits;
    uint16_t m_SlotID;

    uint16_t m_ZoneInfo; // Objects can be in 2 zones (each 8 bits)

    friend class ObjectManager;
    friend class collision_mgr;
};

inline void Object::LoadStart()
{
    assert((m_FlagBits & FLAG_LOADING) == 0);
    // Okay indicate that we are loading
    m_FlagBits |= FLAG_LOADING;
}

//==============================================================================

inline void Object::LoadEnd()
{
    m_FlagBits &= ~FLAG_LOADING;

    //
    // Now that we are done loading lets make sure that we end up in the spacial data base
    //
    OnSpatialUpdate();
}
