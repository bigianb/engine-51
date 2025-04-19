#pragma once

#include "../objectManager/ObjectManager.h"
#include "render/SkinInst.h"
#include "../decals/DecalPackage.h"
#include "../zoneManager/ZoneManager.h"
#include "../characters/FloorProperties.h"
//#include "PhysicsMgr\PhysicsInst.hpp"

//=========================================================================
// DEFINITIONS
//=========================================================================
enum eCorpseName
{
    CORPSE_GENERIC,
    CORPSE_BRIDGES,
    CORPSE_CARSON,
    CORPSE_CRISPY,
    CORPSE_CRISPY_MUTATED,
    CORPSE_CHEW,
    CORPSE_DRCRAY,
    CORPSE_FERRI,
    CORPSE_LEONARD,
    CORPSE_MCCANN,
    CORPSE_MRWHITE,
    CORPSE_RAMIREZ,
    CORPSE_VICTOR,
    CORPSE_NAME_MAX,
};

//=========================================================================
// FORWARD DECLARATIONS
//=========================================================================

class actor;
class actor_effects;
class character;

class corpse : public Object
{
public:
    CREATE_RTTI(corpse, Object, Object)
    const object_desc& GetTypeDesc() const override;
    static const object_desc&  GetObjectType();

    //=========================================================================
    // Public functions
    //=========================================================================
public:
    corpse(ObjectManager* om);
    virtual ~corpse();

    static void LimitCount(ObjectManager* om);
    static bool ReachedMaxActiveLimit();

    bool Initialize(actor&         Actor,
                    bool           bDoBodyFade = true,
                    actor_effects* pActorEffects = nullptr);
    bool Initialize(const char* pGeomName,
                    const char* pAnimGroupName,
                    const char* pAnimName,
                    int         AnimFrame);
    bool InitializeEditorPlaced();

    BBox GetLocalBBox() const override;
    int  GetMaterial() const override { return m_Material; }
    void OnRender() override;

    void  OnRenderTransparent() override;
    void  OnRenderShadowCast(uint64_t ProjMask) override;
    void  OnAdvanceLogic(float DeltaTime) override;
    void  OnActivate(bool Flag) override;
    void  OnPain(const pain& Pain) override;
    void  OnColCheck() override;
    void  OnKill() override;
    void  OnMove(const Vector3& NewPos) override;
    void  OnTransform(const Matrix4& L2W) override;
    float GetHealth() override { return 0.0f; }

    // IJB physics_inst& GetPhysicsInst();
    void SetPermanent(bool Permanent);

    Colour GetFloorColor() { return m_FloorProperties.GetColor(); }

    void ChangeObjectGuid(guid NewGuid);

    /* IJB
    skin_inst&       GetSkinInst() { return m_PhysicsInst.GetSkinInst(); }
    const skin_inst& GetSkinInst() const { return m_PhysicsInst.GetSkinInst(); }
*/
    void SetDrainable(bool isDrainable) { m_bDrainable = isDrainable; }
    bool GetDrainable() { return (m_bDrainable != 0); }

    /* IJB
    virtual render_inst*       GetRenderInstPtr() { return &m_PhysicsInst.GetSkinInst(); }
    virtual AnimGroup::handle* GetAnimGroupHandlePtr() { return &m_PhysicsInst.GetAnimGroupHandle(); }
*/

    const char* GetLogicalName() override { return "DEADBODY"; }
    eCorpseName NameToEnum(const char* pName);
    std::string GetEnumStringCorpse();
    const char* EnumToName(eCorpseName theCorpse);
    const char* GetScanIdentifier();

    void StartFading();
    void StartFading(float NewFadeOutTime);

    const decal_package* GetBloodDecalPackage() const;
    int                  GetBloodDecalGroup() const;

    bool IsBloodEnabled() const;
    bool IsRagdollEnabled() const;

    void CreateSplatDecalOnGround();
    void CreateImpactEffect(const pain& Pain);

    //=========================================================================
    // Editor functions
    //=========================================================================
protected:
    void OnEnumProp(prop_enum& List) override;
    bool OnProperty(prop_query& I) override;

    //=========================================================================
    // Data
    //=========================================================================
protected:
    // Static data
    static int m_ActiveCount; // # of active (moving) corpses

    // Misc
    guid m_OriginGuid; // Guid of object that created it (if any)

    // Flags
    uint32_t m_bActive : 1;            // true if physics should be active when editor placed
    uint32_t m_bCreatedBlood : 1;      // true if blood has been created
    uint32_t m_bCanDelete : 1;         // true if corpse can be deleted
    uint32_t m_bPermanent : 1;         // true if corpse can never be deleted (editor placed)
    uint32_t m_bDestroy : 1;           // true if corpse should be destroyed
    uint32_t m_bDrainable : 1;         // true if corpse is available to be drained by a BO.
    uint32_t m_bActorCollision : 1;    // true if corpse gets pushed by actors
    uint32_t m_bWorldCollision : 1;    // true if corpse collides with the world
    uint32_t m_bActiveWhenVisible : 1; // true if bodies are always active when visible

    // Zone tracking
    zone_mgr::tracker m_ZoneTracker; // Tracks the zones.

    // Logic
    // IJB physics_inst     m_PhysicsInst;     // Physics instance
    float m_TimeAlive; // Used for time out delete
    floor_properties m_FloorProperties; // Floor tracking class
    actor_effects* m_pActorEffects; // any attached special FX (such as flame particles)
    guid           m_FlamingDamageField;
    float          m_FadeOutTime;

    // Properties (for editor created dead body)
    int   m_AnimGroupName;  // Animation group to use
    int   m_AnimName;       // Animation to use
    int   m_AnimFrame;      // Animation frame to use
    float m_SimulationTime; // Time to run ragdoll for pose

    // Blood properties
    int                           m_Material;           // Material type
    ResourceHandle<decal_package> m_hBloodDecalPackage; // Blood package to use
    int                           m_BloodDecalGroup;    // Decal group to use
    eCorpseName                   m_CorpseName;         // Name of the corpse

    // Audio
    float m_ImpactSfxTimer; // Timer count down since last impact
};

//=========================================================================
/* IJB
inline physics_inst& corpse::GetPhysicsInst()
{
    return m_PhysicsInst;
}
*/
//=========================================================================

inline void corpse::SetPermanent(bool Permanent)
{
    m_bPermanent = Permanent;
}

//=========================================================================

inline const decal_package* corpse::GetBloodDecalPackage() const
{
    return m_hBloodDecalPackage.getPointer();
}

//=========================================================================

inline int corpse::GetBloodDecalGroup() const
{
    return m_BloodDecalGroup;
}
