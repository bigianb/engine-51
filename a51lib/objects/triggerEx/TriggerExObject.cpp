#include "TriggerExObject.h"
#include "../../math/Sphere.h"
#include "../../collisionMgr/CollisionMgr.h"
#include "../Player.h"
#include "../../xfiles/x_plus.h"
#include "../../xfiles/xfs.h"

static float  s_SphereRadius = 100.0f;
static Sphere s_EditSphere(Vector3(0, 0, 0), s_SphereRadius);

static const Colour s_TriggerColor_Sleep(150, 150, 150);
static const Colour s_TriggerColor_Checking(0, 255, 0);
static const Colour s_TriggerColor_Recovering(255, 255, 0);
static const Colour s_TriggerColor_Delaying(255, 0, 255);

static const int MAX_STRING_LEN = 255;

extern bool g_game_running;
extern bool g_first_person;

int g_TriggerAdvLogicCount = 0;

#define TRIGGER_DATA_VERSION 1001

//=========================================================================
// Spatial type table
trigger_ex_object::spatial_pair trigger_ex_object::s_SpatialPairTable[] =
    {
        spatial_pair("AXIS_CUBE", trigger_ex_object::SPATIAL_TYPE_AXIS_CUBE),
        spatial_pair("SPHERICAL", trigger_ex_object::SPATIAL_TYPE_SPHERICAL),
        spatial_pair(k_EnumEndStringConst, trigger_ex_object::SPATIAL_TYPES_INVALID), //**MUST BE LAST**//
};
trigger_ex_object::spatial_table trigger_ex_object::s_SpatialEnumTable(s_SpatialPairTable);

//=========================================================================
// Activation type table
trigger_ex_object::activation_pair trigger_ex_object::s_ActivationPairTable[] =
    {
        activation_pair("PLAYER", trigger_ex_object::SPATIAL_ACTIVATE_ON_PLAYER),
        activation_pair("NPC", trigger_ex_object::SPATIAL_ACTIVATE_ON_NPC),
        activation_pair("BULLET", trigger_ex_object::SPATIAL_ACTIVATE_ON_BULLET),
        activation_pair("NPC_OR_PLAYER", trigger_ex_object::SPATIAL_ACTIVATE_ON_NPC_OR_PLAYER),
        activation_pair(k_EnumEndStringConst, trigger_ex_object::SPATIAL_ACTIVATION_TYPES_INVALID), //**MUST BE LAST**//
};
trigger_ex_object::activation_table trigger_ex_object::s_ActivationEnumTable(s_ActivationPairTable);

//=========================================================================
// Activation type table
trigger_ex_object::activation_bullet_pair trigger_ex_object::s_ActivationBulletTypePairTable[] =
    {
        activation_bullet_pair("ALL", trigger_ex_object::SPATIAL_ACTIVATE_BULLET_TYPES_ALL),
        activation_bullet_pair("BULLET", trigger_ex_object::SPATIAL_ACTIVATE_BULLET_TYPES_BULLET),
        activation_bullet_pair("MES_ALT_FIRE", trigger_ex_object::SPATIAL_ACTIVATE_BULLET_TYPES_MES_ALT_FIRE),
        activation_bullet_pair("NPC_BULLET", trigger_ex_object::SPATIAL_ACTIVATE_BULLET_TYPES_NPC_BULLET),
        activation_bullet_pair("PLAYER_BULLET", trigger_ex_object::SPATIAL_ACTIVATE_BULLET_TYPES_PLAYER_BULLET),
        activation_bullet_pair(k_EnumEndStringConst, trigger_ex_object::SPATIAL_ACTIVATION_BULLET_TYPES_END), //**MUST BE LAST **//
};
trigger_ex_object::activation_bullet_table trigger_ex_object::s_ActivationBulletTypeEnumTable(s_ActivationBulletTypePairTable);

//=========================================================================
// Response type table
trigger_ex_object::response_pair trigger_ex_object::s_ResponsePairTable[] =
    {
        response_pair("IF_STILL_INSIDE", trigger_ex_object::SPATIAL_RESPONSE_IF_STILL_INSIDE),
        response_pair("RETURN_TO_SLEEP", trigger_ex_object::SPATIAL_RESPONSE_RETURN_TO_SLEEP),
        response_pair(k_EnumEndStringConst, trigger_ex_object::SPATIAL_RESPONSE_TYPE_INVALID), //**MUST BE LAST**//
};
trigger_ex_object::response_table trigger_ex_object::s_ResponseEnumTable(s_ResponsePairTable);

//=========================================================================
// OBJECT DESCRIPTION
//=========================================================================

struct trigger_ex_object_desc : public object_desc
{
    trigger_ex_object_desc()
        : object_desc(
              Object::TYPE_TRIGGER_EX,
              "TriggerEx",
              "SCRIPT",

              Object::ATTR_NEEDS_LOGIC_TIME |
                  Object::ATTR_SPACIAL_ENTRY |
                  Object::ATTR_COLLIDABLE |
                  Object::ATTR_COLLISION_PERMEABLE |
                  Object::ATTR_BLOCKS_ALL_ACTORS |
                  Object::ATTR_BLOCKS_ALL_PROJECTILES |
                  Object::ATTR_RENDERABLE,

              FLAGS_GENERIC_EDITOR_CREATE |
                  FLAGS_TARGETS_OBJS |
                  FLAGS_IS_DYNAMIC)
    {
        m_bRenderSpatial = true;
        m_bBreakOnError = false;
        m_bRetryOnError = true;
    }

    //-------------------------------------------------------------------------

    virtual void OnEnumProp(prop_enum& List)
    {
        object_desc::OnEnumProp(List);
        List.PropEnumBool("ObjectDesc\\Show Spatial", "Show spacial fields always.", 0);
        List.PropEnumBool("ObjectDesc\\Break on Error", "Pause editor when a trigger error occurs.", 0);
        List.PropEnumBool("ObjectDesc\\Retry on Error", "Retry the same action again.", 0);
    }

    //-------------------------------------------------------------------------

    virtual bool OnProperty(prop_query& I)
    {
        if (object_desc::OnProperty(I)) {
            return true;
        }

        if (I.VarBool("ObjectDesc\\Show Spatial", m_bRenderSpatial)) {
            return true;
        }

        if (I.VarBool("ObjectDesc\\Break on Error", m_bBreakOnError)) {
            return true;
        }

        if (I.VarBool("ObjectDesc\\Retry on Error", m_bRetryOnError)) {
            return true;
        }

        return false;
    }

    Object* Create(ObjectManager* om, collision_mgr* cm, ResourceManager* rm) { return new trigger_ex_object(om, cm); }

    bool m_bRenderSpatial;
    bool m_bBreakOnError;
    bool m_bRetryOnError;
};

static trigger_ex_object_desc s_TriggerExObjectDesc;

//=========================================================================

const object_desc& trigger_ex_object::GetTypeDesc() const
{
    return s_TriggerExObjectDesc;
}

//=========================================================================

const object_desc& trigger_ex_object::GetObjectType()
{
    return s_TriggerExObjectDesc;
}

trigger_ex_object::trigger_ex_object(ObjectManager* om, collision_mgr* cm)
    : Object(om)
    , collisionManager(cm)
    , m_DrawActivationIcon(0)
    , m_DrawError(false)
    , m_CurrentColor(s_TriggerColor_Sleep)
    ,
    // IJB m_ConditionAffecter(true, 0),
    m_TriggerType(TRIGGER_TYPE_SIMPLE)
    , m_UpdateRate(1.0f)
    , m_RecoveryRate(0.0f)
    , m_DelayRate(0.0f)
    , m_NumIfActions(0)
    , m_NumElseActions(0)
    , m_Behavior(TRIGGER_OCCURS_ONCE)
    , m_ResetType(TRIGGER_RESET_DESTROY)
    , m_OnActivate(true)
    , m_RepeatCount(-1)
    , m_ActivateCount(0)
    , m_bTriggerFired(false)
    , m_CurrentActionStatus(ACTION_ONHOLD)
    , m_iCurrentActionIndex(-1)
    , m_IsSpatialActivated(false)
    , m_ActivationType(SPATIAL_ACTIVATE_ON_PLAYER)
    , m_ActivationBulletType(SPATIAL_ACTIVATE_BULLET_TYPES_ALL)
    , m_SpatialType(SPATIAL_TYPE_AXIS_CUBE)
    , m_ResponseType(SPATIAL_RESPONSE_RETURN_TO_SLEEP)
    , m_ActorGuidVarName(-1)
    , m_ActorGuidVarHandle(HNULL)
    , m_bRequiresLineOfSight(false)
    , m_State(STATE_SLEEPING)
    , m_NextUpdateTime(0.0f)
    , m_Next(NULL_GUID)
    , m_Prev(NULL_GUID)
    , m_TriggerSlot(-1)
    , m_EnteringDelay(true)
    , m_EnteringRecovery(true)
{
    for (int i = 0; i < MAX_ACTION_ARRAY_SIZE; i++) {
        m_IfActions[i] = nullptr;
        m_ElseActions[i] = nullptr;
    }

    // spatial dimensions
    for (int i = 0; i < MAX_SPATIAL_DIMENSIONS_PARAMS; i++) {
        m_Dimensions[i] = 400.0f;
    }
    m_bLOSVerified = false;
}

//=========================================================================

trigger_ex_object::~trigger_ex_object()
{
    for (int i = 0; i < MAX_ACTION_ARRAY_SIZE; i++) {
        delete m_IfActions[i];
        m_IfActions[i] = nullptr;

        delete m_ElseActions[i];
        m_ElseActions[i] = nullptr;
    }
}

//=========================================================================

void trigger_ex_object::OnInit()
{
    Object::OnInit();

    // we must have this flag in the descriptor to ever get logic for objects of this type
    // but we need to remove it immediately, we don't want logic until this trigger fires
    SetAttrBits(GetAttrBits() & ~Object::ATTR_NEEDS_LOGIC_TIME);

    // Register myself too the global Trigger_Manager Object...
    // IJB g_TriggerExMgr.RegisterTrigger(*this);

    PreUpdateState();
}

//=========================================================================

void trigger_ex_object::PreUpdateState()
{
    // set this to its default..
    switch (m_TriggerType) {
    case TRIGGER_TYPE_SIMPLE:
        // Set the approritate state
        SetTriggerState(STATE_CHECKING);
        // Set the activation type
        m_ActivationType = SPATIAL_ACTIVATE_ON_PLAYER;
        break;
    case TRIGGER_TYPE_SPATIAL:
        // Set the approritate state
        SetTriggerState(STATE_SLEEPING);
        // Set the activation type
        m_ActivationType = SPATIAL_ACTIVATE_ON_PLAYER;
        break;
    case TRIGGER_TYPE_VIEWABLE:
        // Set the approritate state
        SetTriggerState(STATE_SLEEPING);
        // Set the activation type
        m_ActivationType = SPATIAL_ACTIVATE_ON_INTERNAL;
        break;
    }

    m_IsSpatialActivated = false;
}

//=========================================================================

void trigger_ex_object::OnKill()
{
    Object::OnKill();

    // Unregister myself too the global Trigger_Manager Object...
    // IJB g_TriggerExMgr.UnregisterTrigger(*this);
}

//=========================================================================

void trigger_ex_object::ReleaseBlocking()
{
    if (m_iCurrentActionIndex == -1) {
        return;
    }
    /* IJB
        if (m_CurrentActionStatus == ACTION_RUN_IF)
        {
            if (m_IfActions[m_iCurrentActionIndex] && m_IfActions[m_iCurrentActionIndex]->IsKindOf(action_ai_base::GetRTTI()))
            {
                ((action_ai_base *)m_IfActions[m_iCurrentActionIndex])->ReleaseBlocking();
            }
        }
        else if (m_CurrentActionStatus == ACTION_RUN_ELSE)
        {
            if (m_ElseActions[m_iCurrentActionIndex] && m_ElseActions[m_iCurrentActionIndex]->IsKindOf(action_ai_base::GetRTTI()))
            {
                ((action_ai_base *)m_ElseActions[m_iCurrentActionIndex])->ReleaseBlocking();
            }
        }
            */
}

//=========================================================================
BBox trigger_ex_object::GetLocalBBox() const
{
    if (m_TriggerType == TRIGGER_TYPE_SIMPLE) {
        return s_EditSphere.GetBBox();
    } else // spatial
    {
        BBox bbox;

        switch (m_SpatialType) {
        case SPATIAL_TYPE_AXIS_CUBE:
        {
            float   HalfWidth = m_Dimensions[0] / 2;
            float   HalfHeight = m_Dimensions[1] / 2;
            float   HalfLength = m_Dimensions[2] / 2;
            Vector3 Min(-HalfWidth, -HalfHeight, -HalfLength);
            Vector3 Max(HalfWidth, HalfHeight, HalfLength);
            bbox.Set(Min, Max);
        } break;

        case SPATIAL_TYPE_SPHERICAL:
        {
            Vector3 Min(-m_Dimensions[0], -m_Dimensions[0], -m_Dimensions[0]);
            Vector3 Max(m_Dimensions[0], m_Dimensions[0], m_Dimensions[0]);
            bbox.Set(Min, Max);
        } break;

        default:
            assert(0);
            break;
        }

        return bbox;
    }
}

//=========================================================================

void trigger_ex_object::OnRender()
{
    if (m_OnActivate && (m_TriggerType == TRIGGER_TYPE_VIEWABLE)) {
        if (g_game_running && g_first_person) {
            if (!m_IsSpatialActivated) {
                ActivateSpatialTrigger();
            }
        }
    }
}

void trigger_ex_object::OnColCheck()
{
    if (g_game_running && m_TriggerType == TRIGGER_TYPE_SPATIAL) {
        // Spatial Trigger Only
        switch (m_SpatialType) {
        case SPATIAL_TYPE_AXIS_CUBE:
        {
            collisionManager->StartApply(GetGuid());
            collisionManager->ApplyAABBox(GetBBox());
            collisionManager->EndApply();
        } break;

        case SPATIAL_TYPE_SPHERICAL:
        {
            collisionManager->StartApply(GetGuid());
            collisionManager->ApplySphere(GetPosition(), m_Dimensions[0]);
            collisionManager->EndApply();
        } break;

        default:
            assert(0);
            break;
        }
    }
}

//=============================================================================

void trigger_ex_object::OnColNotify(Object& Object)
{
    if (m_OnActivate && m_TriggerType == TRIGGER_TYPE_SPATIAL) {
        bool bCollide = false;

        // spatial trigger
        switch (m_ActivationType) {
        case SPATIAL_ACTIVATE_ON_PLAYER:
        {
            if (Object.IsKindOf(player::GetRTTI()) == true) {
                bCollide = true;
            }
        } break;
        /* IJB
        case SPATIAL_ACTIVATE_ON_NPC:
        {
            if (Object.IsKindOf(character::GetRTTI()) == true) {
                bCollide = true;
            }
        } break;
         */
/* IJB
        case SPATIAL_ACTIVATE_ON_BULLET:
        {
            if (Object.IsKindOf(base_projectile::GetRTTI()) || Object.IsKindOf(net_proj::GetRTTI())) {
                if (m_ActivationBulletType != SPATIAL_ACTIVATE_BULLET_TYPES_ALL) {
                    switch (m_ActivationBulletType) {
                    case SPATIAL_ACTIVATE_BULLET_TYPES_BULLET:
                    {
                        if (Object.GetTypeDesc().GetType() == Object::TYPE_BULLET_PROJECTILE) {
                            bCollide = true;
                        }
                    } break;

                    case SPATIAL_ACTIVATE_BULLET_TYPES_MES_ALT_FIRE:
                    {
                        if (Object.GetTypeDesc().GetType() == Object::TYPE_MESONSEEKER_PROJECTILE) {
                            bCollide = true;
                        }
                    } break;

                    case SPATIAL_ACTIVATE_BULLET_TYPES_NPC_BULLET:
                    {
                        if (Object.IsKindOf(base_projectile::GetRTTI())) {
                            base_projectile& projectile = base_projectile::GetSafeType(Object);
                            Object*          parentObject = objectManager->GetObjectByGuid(projectile.GetOwnerID());
                            if (parentObject && parentObject->IsKindOf(character::GetRTTI())) {
                                bCollide = true;
                            }
                        } else if (Object.IsKindOf(net_proj::GetRTTI())) {
                            net_proj& projectile = net_proj::GetSafeType(Object);
                            Object*   parentObject = objectManager->GetObjectByGuid(projectile.GetOriginGuid());
                            if (parentObject && parentObject->IsKindOf(character::GetRTTI())) {
                                bCollide = true;
                            }
                        }
                    } break;

                    case SPATIAL_ACTIVATE_BULLET_TYPES_PLAYER_BULLET:
                    {
                        if (Object.IsKindOf(base_projectile::GetRTTI())) {
                            base_projectile& projectile = base_projectile::GetSafeType(Object);
                            Object*          parentObject = objectManager->GetObjectByGuid(projectile.GetOwnerID());
                            if (parentObject && parentObject->IsKindOf(player::GetRTTI())) {
                                bCollide = true;
                            }
                        } else if (Object.IsKindOf(net_proj::GetRTTI())) {
                            net_proj& projectile = net_proj::GetSafeType(Object);
                            Object*   parentObject = objectManager->GetObjectByGuid(projectile.GetOriginGuid());
                            if (parentObject && parentObject->IsKindOf(player::GetRTTI())) {
                                bCollide = true;
                            }
                        }
                    } break;

                    default:
                    {
                        bCollide = false;
                    } break;
                    }
                } else {
                    // All bullet types collide with this trigger
                    bCollide = true;
                }
            }
        } break;
    */
        case SPATIAL_ACTIVATE_ON_NPC_OR_PLAYER:
        {
            if (/* IJB Object.IsKindOf(character::GetRTTI()) == true || */
                Object.IsKindOf(player::GetRTTI())) {
                bCollide = true;
            }
        } break;
        case SPATIAL_ACTIVATE_ON_INTERNAL:
        {
            // no-op, special case for viewable
        } break;
        default:
            assert(0);
            break;
            break;
        }

        if (bCollide) {
            ActivateSpatialTrigger();
            if (m_ActorGuidVarHandle.IsNonNull()) {
                // IJB g_VarMgr.SetGuid(m_ActorGuidVarHandle, Object.GetGuid());
            }
        }
    }

    Object::OnColNotify(Object);
}

//=============================================================================

void trigger_ex_object::ActivateSpatialTrigger()
{
    // the m_IsSpatialActivated flag is used to prevent multiple activation events to cause the trigger
    // to udpate faster than its natural update rate and also prevent multiple triggering of actions.
    if (m_IsSpatialActivated) {
        return;
    }

    ForceNextUpdate();
    m_CurrentColor = Colour(255, 0, 0);
    SetTriggerState(STATE_CHECKING);
    m_IsSpatialActivated = true;
}

//=============================================================================

void trigger_ex_object::DeactivateSpatialTrigger()
{
    if (!m_IsSpatialActivated) {
        return;
    }

    SetTriggerState(STATE_SLEEPING);
    m_IsSpatialActivated = false;
}

//=========================================================================

void trigger_ex_object::OnEnumProp(prop_enum& rPropList)
{
    Object::OnEnumProp(rPropList);

    rPropList.PropEnumHeader("Trigger Object", "The base class for trigger objects.", PROP_TYPE_HEADER);

    EnumPropDynamic(rPropList);

    rPropList.PropEnumEnum("Trigger Object\\Type", "SIMPLE\0SPATIAL\0VIEWABLE\0", "Type of this trigger. Simple fire when active, Spatial fire when active and collision occurs, Viewable fire when active and rendered.", PROP_TYPE_MUST_ENUM);
    rPropList.PropEnumBool("Trigger Object\\Start Active", "Does this trigger start active?", 0);
    rPropList.PropEnumBool("Trigger Object\\Is Active", "Is this trigger currently active?", PROP_TYPE_EXPOSE | PROP_TYPE_READ_ONLY | PROP_TYPE_DONT_SHOW);
    rPropList.PropEnumBool("Trigger Object\\Is Spatial Active", "Is this spatial trigger currently active?", PROP_TYPE_EXPOSE | PROP_TYPE_READ_ONLY | PROP_TYPE_DONT_SHOW);

    if (m_TriggerType == TRIGGER_TYPE_SIMPLE) {
        rPropList.PropEnumFloat("Trigger Object\\Update Rate", "The rate at which this trigger check for valid conditions in seconds( if 0, then no update occurs.) Note that the first update will occur at a random time between 0 and the update rate.", 0);
    }

    rPropList.PropEnumFloat("Trigger Object\\Delay Time", "The time at which a trigger waits before it executues its actions in seconds. For repeating it will wait before each pass through the actions.", 0);

    if (m_TriggerType != TRIGGER_TYPE_VIEWABLE) {
        // these don't make sense for viewable triggers
        rPropList.PropEnumEnum("Trigger Object\\Behavior", "ONCE\0REPEATING\0", "Type defines the trigger activation behavior.( ONCE allows only 1 execution, REPEAT allows more than one.)", PROP_TYPE_MUST_ENUM);

        if (m_Behavior == TRIGGER_OCCURS_REPEATING) {
            rPropList.PropEnumInt("Trigger Object\\Repeat Count", "The number of times a repeating trigger can be activated, if -1 then this trigger will repeat indefinitely.", PROP_TYPE_MUST_ENUM);
            rPropList.PropEnumFloat("Trigger Object\\Recovery Rate", "The rate at which repeating triggers recover in seconds.", 0);
        }
    }

    if (m_TriggerType == TRIGGER_TYPE_VIEWABLE) {
        rPropList.PropEnumBool("Trigger Object\\Requires Line Of Sight", "Does this trigger require a clear line of sight to activate?", 0);
    }

    // if not an infinitely repeating trigger
    if (!((m_Behavior == TRIGGER_OCCURS_REPEATING) && (m_RepeatCount == -1))) {
        rPropList.PropEnumEnum("Trigger Object\\Reset Style", "DESTROY\0DEACTIVATE\0", "Once all actions have completed for the specific count desired, what does this trigger do, destory itself, or deactivate itself?", 0);
    }

    // data intergrity must happen prior to the actual conditions and actions
    rPropList.PropEnumInt("Trigger Object\\NumIfActions", "", PROP_TYPE_DONT_SHOW);
    rPropList.PropEnumInt("Trigger Object\\NumElseActions", "", PROP_TYPE_DONT_SHOW);
    // IJB m_ConditionAffecter.EnumPropConditionData(rPropList);

    /* IJB
    // enum actual conditions and actions
    m_ConditionAffecter.EnumPropConditions(rPropList);
    EnumPropActions(rPropList);
    m_ConditionAffecter.EnumPropElseConditions(rPropList);
    EnumPropElseActions(rPropList);
*/
    if (m_TriggerType != TRIGGER_TYPE_SIMPLE) {
        EnumPropSpatial(rPropList);
    }
}

//===========================================================================

bool trigger_ex_object::OnProperty(prop_query& rPropQuery)
{
    SetFlagBits(GetFlagBits() | FLAG_DIRTY_TRANSLATION);

    if (Object::OnProperty(rPropQuery)) {
        return true;
    }

    if (OnPropertyDynamic(rPropQuery)) {
        return true;
    }

    if (rPropQuery.IsVar("Trigger Object\\Type")) {
        if (rPropQuery.IsRead()) {
            switch (m_TriggerType) {
            case TRIGGER_TYPE_SIMPLE:
                rPropQuery.SetVarEnum("SIMPLE");
                break;
            case TRIGGER_TYPE_SPATIAL:
                rPropQuery.SetVarEnum("SPATIAL");
                break;
            case TRIGGER_TYPE_VIEWABLE:
                rPropQuery.SetVarEnum("VIEWABLE");
                break;
            default:
                assert(0);
                break;
            }
        } else {
            const char* pString = rPropQuery.GetVarEnum();
            uint32_t    Bits = GetAttrBits();

            if (x_stricmp(pString, "SIMPLE") == 0) {
                m_TriggerType = TRIGGER_TYPE_SIMPLE;

                // change bits for PS2 only
                Bits &= ~ATTR_SPACIAL_ENTRY;
                Bits &= ~ATTR_COLLIDABLE;
                Bits &= ~ATTR_COLLISION_PERMEABLE;
                Bits &= ~ATTR_BLOCKS_ALL_ACTORS;
                Bits &= ~ATTR_BLOCKS_ALL_PROJECTILES;
                Bits &= ~ATTR_RENDERABLE;

                m_ActivationType = SPATIAL_ACTIVATE_ON_PLAYER;
                m_SpatialType = SPATIAL_TYPE_AXIS_CUBE;
            } else if (x_stricmp(pString, "SPATIAL") == 0) {
                m_TriggerType = TRIGGER_TYPE_SPATIAL;

                // change bits for PS2 only
                Bits |= ATTR_SPACIAL_ENTRY;
                Bits |= ATTR_COLLIDABLE;
                Bits |= ATTR_COLLISION_PERMEABLE;
                Bits |= ATTR_BLOCKS_ALL_ACTORS;
                Bits |= ATTR_BLOCKS_ALL_PROJECTILES;
                Bits &= ~ATTR_RENDERABLE;

                m_ActivationType = SPATIAL_ACTIVATE_ON_PLAYER;
                m_SpatialType = SPATIAL_TYPE_AXIS_CUBE;
            } else if (x_stricmp(pString, "VIEWABLE") == 0) {
                m_TriggerType = TRIGGER_TYPE_VIEWABLE;

                // change bits for PS2 only
                Bits |= ATTR_SPACIAL_ENTRY;
                Bits &= ~ATTR_COLLIDABLE;
                Bits &= ~ATTR_COLLISION_PERMEABLE;
                Bits &= ~ATTR_BLOCKS_ALL_ACTORS;
                Bits &= ~ATTR_BLOCKS_ALL_PROJECTILES;
                Bits |= ATTR_RENDERABLE;

                m_ActivationType = SPATIAL_ACTIVATE_ON_INTERNAL;
                m_SpatialType = SPATIAL_TYPE_AXIS_CUBE;
            }

            // update attr bits
            SetAttrBits(Bits);
            OnSpatialUpdate();
            PreUpdateState();
        }

        return true;
    }

    if (rPropQuery.VarFloat("Trigger Object\\Update Rate", m_UpdateRate)) {
        // Ensures the triggers get updated out of sync from one another...
        //   This might cause problems with repeating runs of complex trigger systems, use the define to disable...

        m_NextUpdateTime = x_frand(0, m_UpdateRate);

        return true;
    }

    if (rPropQuery.VarFloat("Trigger Object\\Delay Time", m_DelayRate)) {
        return true;
    }

    if (rPropQuery.VarFloat("Trigger Object\\Recovery Rate", m_RecoveryRate)) {
        return true;
    }

    if (rPropQuery.VarInt("Trigger Object\\Repeat Count", m_RepeatCount)) {
        return true;
    }

    if (rPropQuery.VarInt("Trigger Object\\NumIfActions", m_NumIfActions)) {
        return true;
    }

    if (rPropQuery.VarInt("Trigger Object\\NumElseActions", m_NumElseActions)) {
        return true;
    }

    if (rPropQuery.VarBool("Trigger Object\\Start Active", m_OnActivate)) {
        if (!rPropQuery.IsRead() && g_game_running) {
            OnActivate(m_OnActivate);
        }
        return true;
    }

    if (rPropQuery.VarBool("Trigger Object\\Requires Line Of Sight", m_bRequiresLineOfSight)) {
        return true;
    }

    /* IJB
    if (m_ConditionAffecter.OnPropertyConditionData(rPropQuery)) {
        return true;
    }

    if (m_ConditionAffecter.OnPropertyConditions(rPropQuery)) {
        return true;
    }
*/
    if (OnPropertyActions(rPropQuery)) {
        return true;
    }

    if (rPropQuery.IsVar("Trigger Object\\Is Active")) {
        if (rPropQuery.IsRead()) {
            rPropQuery.SetVarBool(m_OnActivate);
        }

        return true;
    }

    if (rPropQuery.IsVar("Trigger Object\\Is Spatial Active")) {
        if (rPropQuery.IsRead()) {
            rPropQuery.SetVarBool(m_IsSpatialActivated);
        }

        return true;
    }

    if (rPropQuery.IsVar("Trigger Object\\Behavior")) {
        if (rPropQuery.IsRead()) {
            switch (m_Behavior) {
            case TRIGGER_OCCURS_ONCE:
                rPropQuery.SetVarEnum("ONCE");
                break;
            case TRIGGER_OCCURS_REPEATING:
                rPropQuery.SetVarEnum("REPEATING");
                break;
            default:
                assert(0);
                break;
            }
        } else {
            const char* pString = rPropQuery.GetVarEnum();

            if (x_stricmp(pString, "ONCE") == 0) {
                m_Behavior = TRIGGER_OCCURS_ONCE;
            }
            if (x_stricmp(pString, "REPEATING") == 0) {
                m_Behavior = TRIGGER_OCCURS_REPEATING;
            }
        }

        return true;
    }

    if (rPropQuery.IsVar("Trigger Object\\Reset Style")) {
        if (rPropQuery.IsRead()) {
            switch (m_ResetType) {
            case TRIGGER_RESET_DESTROY:
                rPropQuery.SetVarEnum("DESTROY");
                break;
            case TRIGGER_RESET_DEACTIVATE:
                rPropQuery.SetVarEnum("DEACTIVATE");
                break;
            default:
                assert(0);
                break;
            }
        } else {
            const char* pString = rPropQuery.GetVarEnum();

            if (x_stricmp(pString, "DESTROY") == 0) {
                m_ResetType = TRIGGER_RESET_DESTROY;
            }
            if (x_stricmp(pString, "DEACTIVATE") == 0) {
                m_ResetType = TRIGGER_RESET_DEACTIVATE;
            }
        }

        return true;
    }

    if (OnPropertySpatial(rPropQuery)) {
        return true;
    }

    return false;
}

void trigger_ex_object::EnumPropSpatial(prop_enum& rPropList)
{
    rPropList.PropEnumHeader("Spatial Trigger", "Select the type of condition to add.", PROP_TYPE_HEADER);

    if (m_TriggerType == TRIGGER_TYPE_SPATIAL) {
        rPropList.PropEnumEnum("Spatial Trigger\\Type",
                               s_SpatialEnumTable.BuildString(),
                               "Types of spatial triggers.  AxisCube is a box or Sphere is Radius.", PROP_TYPE_MUST_ENUM);

        rPropList.PropEnumEnum("Spatial Trigger\\Activation Triggers",
                               s_ActivationEnumTable.BuildString(),
                               "Things which will activate this spatial trigger.", PROP_TYPE_MUST_ENUM);

        if (m_ActivationType == SPATIAL_ACTIVATE_ON_BULLET) {
            rPropList.PropEnumEnum("Spatial Trigger\\Activation Bullet Type", s_ActivationBulletTypeEnumTable.BuildString(), "Type of bullet activates this trigger", PROP_TYPE_MUST_ENUM);
        }

        rPropList.PropEnumExternal("Spatial Trigger\\ActivaterVar", "global\0global_guid",
                                   "(Optional) Global Variable (type guid) where we can store the activator guid; leave blank to not store this info.", PROP_TYPE_MUST_ENUM);

        rPropList.PropEnumEnum("Spatial Trigger\\Response Type",
                               s_ResponseEnumTable.BuildString(),
                               "How to deal with repeating triggers.", PROP_TYPE_MUST_ENUM);
    }

    switch (m_SpatialType) {
    case SPATIAL_TYPE_AXIS_CUBE:
        rPropList.PropEnumFloat("Spatial Trigger\\Cube Width", "The width of the cube.", 0);
        rPropList.PropEnumFloat("Spatial Trigger\\Cube Height", "The height of the cube.", 0);
        rPropList.PropEnumFloat("Spatial Trigger\\Cube Length", "The length of the cube.", 0);
        break;

    case SPATIAL_TYPE_SPHERICAL:
        rPropList.PropEnumFloat("Spatial Trigger\\Sphere Radius", "The radius of the sphere.", 0);
        break;

    default:
        assert(0);
        break;
    }
}

//=============================================================================

bool trigger_ex_object::OnPropertySpatial(prop_query& rPropQuery)
{
    SetFlagBits(GetFlagBits() | FLAG_DIRTY_TRANSLATION);
/* IJB
    // always check for a valid guid handle
    if (m_ActorGuidVarName == -1) {
        m_ActorGuidVarHandle = HNULL;
    } else if (!g_VarMgr.GetGuidHandle(g_StringMgr.GetString(m_ActorGuidVarName), &m_ActorGuidVarHandle)) {
        m_ActorGuidVarHandle = HNULL;
    }

    if (rPropQuery.IsVar("Spatial Trigger\\ActivaterVar")) {
        if (rPropQuery.IsRead()) {
            if (m_ActorGuidVarName >= 0) {
                rPropQuery.SetVarExternal(g_StringMgr.GetString(m_ActorGuidVarName), 256);
            } else {
                rPropQuery.SetVarExternal("", 256);
            }
            return true;
        } else {
            if (strlen(rPropQuery.GetVarExternal()) > 0) {
                m_ActorGuidVarName = g_StringMgr.Add(rPropQuery.GetVarExternal());
                return true;
            }
        }
    }

    if (SMP_UTIL_IsEnumVar<trigger_ex_spatial_types, trigger_ex_spatial_types>(rPropQuery, "Spatial Trigger\\Type",
                                                                               m_SpatialType, s_SpatialEnumTable)) {
        return true;
    }

    if (SMP_UTIL_IsEnumVar<trigger_ex_spatial_activation_types, trigger_ex_spatial_activation_types>(rPropQuery, "Spatial Trigger\\Activation Triggers",
                                                                                                     m_ActivationType, s_ActivationEnumTable)) {
        // Be sure and mark trigger to collide with bullets
        if (m_ActivationType == trigger_ex_object::SPATIAL_ACTIVATE_ON_BULLET) {
            SetAttrBits(GetAttrBits() | Object::ATTR_BLOCKS_ALL_PROJECTILES);
        } else {
            SetAttrBits(GetAttrBits() & (~Object::ATTR_BLOCKS_ALL_PROJECTILES));
        }

        return true;
    }

    if (m_ActivationType == SPATIAL_ACTIVATE_ON_BULLET) {
        if (SMP_UTIL_IsEnumVar<trigger_ex_spatial_activation_bullet_types, trigger_ex_spatial_activation_bullet_types>(rPropQuery, "Spatial Trigger\\Activation Bullet Type",
                                                                                                                       m_ActivationBulletType, s_ActivationBulletTypeEnumTable)) {
            return true;
        }
    }

    if (SMP_UTIL_IsEnumVar<trigger_ex_spatial_activation_response, trigger_ex_spatial_activation_response>(rPropQuery, "Spatial Trigger\\Response Type",
                                                                                                           m_ResponseType, s_ResponseEnumTable)) {
        return true;
    }
*/
    if (rPropQuery.VarFloat("Spatial Trigger\\Cube Width", m_Dimensions[0])) {
        return true;
    }

    if (rPropQuery.VarFloat("Spatial Trigger\\Cube Height", m_Dimensions[1])) {
        return true;
    }

    if (rPropQuery.VarFloat("Spatial Trigger\\Cube Length", m_Dimensions[2])) {
        return true;
    }

    if (rPropQuery.VarFloat("Spatial Trigger\\Sphere Radius", m_Dimensions[0])) {
        return true;
    }

    return false;
}

//=============================================================================

bool trigger_ex_object::IsAwake()
{
    if (m_State == STATE_SLEEPING) {
        return false;
    }

    return true;
}

//=============================================================================
// used by derived classes who want to update at the natural rate of the trigger but not execute the logic..

bool trigger_ex_object::CanUpdate(float DeltaTime)
{
    return CheckNextTime(DeltaTime);
}

//=============================================================================

bool trigger_ex_object::CheckNextTime(float DeltaTime)
{
    // if we can update make sure to not syncrhonize by carrying the difference over..
    m_NextUpdateTime -= DeltaTime;

    if (m_NextUpdateTime > 0.0f) {
        return false;
    }

    return true;
}

//=============================================================================

void trigger_ex_object::UpdateNextTime(float Time)
{
    m_NextUpdateTime += Time;
}

//=============================================================================

void trigger_ex_object::SetTriggerState(const trigger_ex_state State)
{
    // cannot come out of RESET state once we enter it...
    if (m_State == STATE_RESETTING) {
        return;
    }

    // set the desired state
    m_State = State;

    // no update for 0 rate, set it to sleep mode.
    if ((m_TriggerType == TRIGGER_TYPE_SIMPLE) && (m_UpdateRate == 0.0f)) {
        // set us to sleeping.
        m_State = STATE_SLEEPING;
    }

    switch (m_State) {
    case STATE_SLEEPING:
        // tell the manager that were asleep so we dont get anymore updates..
        // IJB g_TriggerExMgr.TriggerSleep(*this);
        m_CurrentColor = s_TriggerColor_Sleep;
        break;

    case STATE_CHECKING:
        // tell the manager that were awake so we get moved into the updating list.
        ForceNextUpdate();
        // IJB g_TriggerExMgr.TriggerAwake(*this);
        m_DrawActivationIcon = 0;
        m_CurrentColor = s_TriggerColor_Checking;
        break;

    case STATE_RECOVERY:
        m_DrawActivationIcon = 0;
        m_CurrentColor = s_TriggerColor_Recovering;
        break;

    case STATE_DELAYING:
        m_DrawActivationIcon = 0;
        m_CurrentColor = s_TriggerColor_Delaying;
        break;

    case STATE_RESETTING:
        m_DrawActivationIcon = 0;
        m_CurrentColor = s_TriggerColor_Checking;
        // no-op
        break;

    default:
        assert(0);
        break;
    }
}

//=============================================================================

void trigger_ex_object::OnMove(const Vector3& NewPos)
{
    LogicCheckOnActivate();
    Object::OnMove(NewPos);
}

//=============================================================================

void trigger_ex_object::ResetTrigger()
{
    SetTriggerState(STATE_RESETTING);
}

//=============================================================================

void trigger_ex_object::KillTrigger()
{
    m_ResetType = TRIGGER_RESET_DESTROY;
    m_iCurrentActionIndex = -1;
    m_CurrentActionStatus = ACTION_ONHOLD;
    SetAttrBits(GetAttrBits() & ~Object::ATTR_NEEDS_LOGIC_TIME);
    SetTriggerState(STATE_RESETTING);
}

//=============================================================================

void trigger_ex_object::ExecuteLogic(float DeltaTime)
{

    // don't do anything if we are not active
    if (!m_OnActivate) {
        return;
    }

    // if we are in logic loop, don't do anything
    if (m_iCurrentActionIndex != -1) {
        return;
    }

    if (m_TriggerType != TRIGGER_TYPE_SIMPLE && !m_IsSpatialActivated) {
        // if spacial is not activated, make sure we go to sleep
        m_State = STATE_SLEEPING;
    }

    switch (m_State) {
    case STATE_SLEEPING:
        ExecuteSleeping(DeltaTime);
        break;

    case STATE_CHECKING:
        ExecuteChecking(DeltaTime);
        break;

    case STATE_RECOVERY:
        ExecuteRecovery(DeltaTime);
        break;

    case STATE_DELAYING:
        ExecuteDelaying(DeltaTime);
        break;

    case STATE_RESETTING:
        if (m_ResetType == TRIGGER_RESET_DESTROY) {
            // destroy the trigger
            objectManager->DestroyObject(GetGuid());
        } else {
            // turn the trigger off
            m_State = STATE_SLEEPING; // first we change to another temp state
            m_NextUpdateTime = 0.0f;  // reset time
            m_OnActivate = false;     // make trigger inactivate
            m_ActivateCount = 0;      // reset repeat counter
            m_bTriggerFired = false;
            m_DrawActivationIcon = 0;
            m_iCurrentActionIndex = -1;
            m_CurrentActionStatus = ACTION_ONHOLD;
            if (m_TriggerType != TRIGGER_TYPE_SIMPLE) {
                DeactivateSpatialTrigger(); // make sure the spatial trigger is not active
                SetTriggerState(STATE_SLEEPING);
            } else {
                SetTriggerState(STATE_CHECKING); // go into checking state
            }
        }
        break;

    default:

        assert(0);
        break;
    }

    // spatial trigger specific
    if (m_IsSpatialActivated && m_TriggerType == TRIGGER_TYPE_SPATIAL) {
        if (GetTriggerState() == STATE_CHECKING) {
            switch (m_ResponseType) {
            case SPATIAL_RESPONSE_IF_STILL_INSIDE:
            {
                // do check to see if the Object were intrested in is still inside of our volume
                switch (m_ActivationType) {
                case SPATIAL_ACTIVATE_ON_PLAYER:
                {
                    if (QueryPlayerInVolume() == false) {
                        DeactivateSpatialTrigger();
                    }
                } break;
                case SPATIAL_ACTIVATE_ON_NPC:
                {
                    if (QueryNpcInVolume() == false) {
                        DeactivateSpatialTrigger();
                    }
                } break;
                case SPATIAL_ACTIVATE_ON_NPC_OR_PLAYER:
                {
                    if (QueryNpcInVolume() == false && QueryPlayerInVolume() == false) {
                        DeactivateSpatialTrigger();
                    }
                } break;
                case SPATIAL_ACTIVATE_ON_BULLET:
                {
                    if (QueryBulletInVolume() == false) {
                        DeactivateSpatialTrigger();
                    }
                } break;
                case SPATIAL_ACTIVATE_ON_INTERNAL:
                {
                    // no-op, special case for viewable
                } break;
                default:
                    assert(0);
                    break;
                    break;
                }
            } break;

            case SPATIAL_RESPONSE_RETURN_TO_SLEEP:
                // goes back to sleep after activating..
                DeactivateSpatialTrigger();
                break;

            default:
                assert(0);
                break;
            }
        }
    }
}

//=============================================================================
// This function checks the logic of spatial triggers activated by on voulme, once it
// awakes during an OnActivate call..

void trigger_ex_object::LogicCheckOnActivate()
{
    if (!m_OnActivate) {
        return;
    }

    // for spatial
    if (m_TriggerType != TRIGGER_TYPE_SIMPLE) {
        // do check to see if the Object were intrested in is still inside of our volume
        switch (m_ActivationType) {
        case SPATIAL_ACTIVATE_ON_PLAYER:
        {
            if (QueryPlayerInVolume() == true) {
                ActivateSpatialTrigger();
            }
        } break;
        case SPATIAL_ACTIVATE_ON_NPC:
        {
            if (QueryNpcInVolume() == true) {
                ActivateSpatialTrigger();
            }
        } break;
        case SPATIAL_ACTIVATE_ON_NPC_OR_PLAYER:
        {
            if (QueryNpcInVolume() == true || QueryPlayerInVolume() == true) {
                ActivateSpatialTrigger();
            }
        } break;
        case SPATIAL_ACTIVATE_ON_BULLET:
        {
            if (QueryBulletInVolume() == true) {
                ActivateSpatialTrigger();
            }
        } break;
        case SPATIAL_ACTIVATE_ON_INTERNAL:
        {
            // no-op, special case for viewable
        } break;
        default:
            assert(0);
            break;
        }
    }
}

//=============================================================================

void trigger_ex_object::ForceNextUpdate()
{
    m_NextUpdateTime = 0.0f;
}

//=============================================================================

void trigger_ex_object::ExecuteSleeping(float DeltaTime)
{
    // Sleep state puts the trigger into an inactive mode
    // IJB g_TriggerExMgr.TriggerSleep(*this);
}

//=============================================================================

void trigger_ex_object::ExecuteChecking(float DeltaTime)
{
    // Check state, checks all the conditions, if they meet the requeisite state flags then
    // run the actions.. Then using the type, determine what to do post activiation...

    if (CheckNextTime(DeltaTime) == false) {
        return;
    }
/* IJB
    // Evaluate the conditions, only update the time if we don't get a true evaluation because
    // we don't want to time penalize the next state...
    switch (m_ConditionAffecter.EvaluateConditions(GetGuid())) {
    case conditional_affecter::EVAL_COMMIT_IF:
        if (GetActionCount() > 0) {
            m_CurrentActionStatus = ACTION_RUN_IF;
            SetTriggerState(STATE_DELAYING);
            return;
        }
        break;
    case conditional_affecter::EVAL_COMMIT_ELSE:
        if (GetElseActionCount() > 0) {
            m_CurrentActionStatus = ACTION_RUN_ELSE;
            SetTriggerState(STATE_DELAYING);
        }
        break;
    case conditional_affecter::EVAL_NOT_MET:
        m_CurrentActionStatus = ACTION_ONHOLD;
        break;
    }
*/
    m_DrawActivationIcon = 1;

    if (m_TriggerType == TRIGGER_TYPE_SIMPLE) {
        UpdateNextTime(m_UpdateRate);
    } else {
        ForceNextUpdate();
    }
}

//=============================================================================

void trigger_ex_object::ExecuteRecovery(float DeltaTime)
{
    // Recovery state, either destroy the Object or sets it into checking mode with an extended
    // wait time until the next valid update within checking state.

    switch (m_Behavior) {
    case TRIGGER_OCCURS_ONCE:
        if (m_ActivateCount > 0) {
            SetTriggerState(STATE_RESETTING);
        } else {
            SetTriggerState(STATE_CHECKING);
        }
        break;
    case TRIGGER_OCCURS_REPEATING:
    {
        if (m_RepeatCount != -1 && (m_ActivateCount >= m_RepeatCount)) {
            SetTriggerState(STATE_RESETTING);
        } else {
            // Only add the recoveryrate once..
            if (m_EnteringRecovery) {
                m_EnteringRecovery = false;
                UpdateNextTime(m_RecoveryRate);
            }

            // Check if we can return to our checking state now...
            if ((!m_bQuitRecovery) && (CheckNextTime(DeltaTime) == false)) {
                return;
            }

            // Reset flag
            m_EnteringRecovery = true;

            // Set the next state
            SetTriggerState(STATE_CHECKING);
        }
    } break;
    default:
        assert(0);
        break;
        ;
    }
}

//=============================================================================

void trigger_ex_object::ExecuteDelaying(float DeltaTime)
{
    // Delay state, waits for a specificed amount of time before executing the action...
    if (m_EnteringDelay) {
        m_EnteringDelay = false;
        UpdateNextTime(m_DelayRate);
    }

    // Check if we can go onto our recovery state now...
    if (CheckNextTime(DeltaTime) == false) {
        return;
    }

    m_bQuitRecovery = false;

    // Execute all actions..
    if (m_CurrentActionStatus != ACTION_ONHOLD) {
        ExecuteAllActions();
    } else {
        // nothing to execute
        m_bQuitRecovery = true;
    }

    // Reset this flag
    m_EnteringDelay = true;

    // Set the next state
    SetTriggerState(STATE_RECOVERY);

    // Turn on the draw activation flag
    m_DrawActivationIcon = 2;

    return;
}

//=============================================================================

bool trigger_ex_object::AddAction(actions_ex_base::action_ex_types ActionType, int Number, bool bAsElse)
{
    // IJB
    return false;
    
    if (Number >= 0 && Number < MAX_ACTION_ARRAY_SIZE) {
        if (!bAsElse) {
            if (m_IfActions[Number]) {
                delete m_IfActions[Number];
                m_IfActions[Number] = nullptr;
            }

            m_IfActions[Number] = actions_ex_base::CreateAction(ActionType, GetGuid());
            assert(m_IfActions[Number]);
            m_IfActions[Number]->SetElse(false);
        } else {
            if (m_ElseActions[Number]) {
                delete m_ElseActions[Number];
                m_ElseActions[Number] = nullptr;
            }

            m_ElseActions[Number] = actions_ex_base::CreateAction(ActionType, GetGuid());
            assert(m_ElseActions[Number]);
            m_ElseActions[Number]->SetElse(true);
        }
        return true;
    } else {
        // x_try;
        // x_throw("You can not add any more actions to this trigger!");
        // x_catch_display;
        return false;
    }
}

//=============================================================================

void trigger_ex_object::EnumPropActions(prop_enum& rPropList)
{

    if (GetActionCount() > 0) {
        // we have at least 1 action
        rPropList.PropEnumHeader("Do", "All the Actions for this Trigger.", PROP_TYPE_HEADER);

        for (int i = 0; i < MAX_ACTION_ARRAY_SIZE; i++) {
            if (m_IfActions[i]) {
                assert(m_IfActions[i]->GetElse() == false);

                rPropList.PropEnumString(xfs("Do\\Action[%d]", i),
                                         m_IfActions[i]->GetTypeInfo(), PROP_TYPE_HEADER);

                int iHeader = rPropList.PushPath(xfs("Do\\Action[%d]\\", i));

                m_IfActions[i]->OnEnumProp(rPropList);

                rPropList.PopPath(iHeader);
            }
        }
    }
}

//=============================================================================

void trigger_ex_object::EnumPropElseActions(prop_enum& rPropList)
{
    /* IJB
    if (GetElseActionCount() > 0) {
        // we have at least 1 else action
        sml_string HeaderName;
        if (m_ConditionAffecter.GetElseConditionCount() > 0) {
            HeaderName.Set("Then Do");
        } else {
            HeaderName.Set("Else Do");
        }
        rPropList.PropEnumHeader(HeaderName.Get(), "All the else Actions for this Trigger.", PROP_TYPE_HEADER);

        for (int i = 0; i < MAX_ACTION_ARRAY_SIZE; i++) {
            if (m_ElseActions[i]) {
                assert(m_ElseActions[i]->GetElse() == true);

                rPropList.PropEnumString(xfs("%s\\Else Action[%d]", HeaderName.Get(), i),
                                         m_ElseActions[i]->GetTypeInfo(), PROP_TYPE_HEADER);

                int iHeader = rPropList.PushPath(xfs("%s\\Else Action[%d]\\", HeaderName.Get(), i));

                m_ElseActions[i]->OnEnumProp(rPropList);

                rPropList.PopPath(iHeader);
            }
        }
    }
        */
}

//=============================================================================

bool trigger_ex_object::OnPropertyActions(prop_query& rPropQuery)
{
    // if actions
    if (rPropQuery.IsSimilarPath("Do\\Action")) {
        int iIndex = rPropQuery.GetIndex(0);

        assert(iIndex < MAX_ACTION_ARRAY_SIZE && iIndex >= 0);

        if (rPropQuery.IsVar("Do\\Action[]")) {
            if (rPropQuery.IsRead()) {
                rPropQuery.SetVarString(m_IfActions[iIndex]->GetDescription(), MAX_STRING_LEN);
            }

            return true;
        }

        if (m_IfActions[iIndex]) {
            int iHeader = rPropQuery.PushPath("Do\\Action[]\\");

            if (m_IfActions[iIndex]->OnProperty(rPropQuery)) {
                rPropQuery.PopPath(iHeader);
                return true;
            }

            rPropQuery.PopPath(iHeader);
        }
    }

    // else actions
    if (rPropQuery.IsSimilarPath("Else Do\\Else Action")) {
        int iIndex = rPropQuery.GetIndex(0);

        assert(iIndex < MAX_ACTION_ARRAY_SIZE && iIndex >= 0);

        if (rPropQuery.IsVar("Else Do\\Else Action[]")) {
            if (rPropQuery.IsRead()) {
                rPropQuery.SetVarString(m_ElseActions[iIndex]->GetDescription(), MAX_STRING_LEN);
            }

            return true;
        }

        if (m_ElseActions[iIndex]) {
            int iHeader = rPropQuery.PushPath("Else Do\\Else Action[]\\");

            if (m_ElseActions[iIndex]->OnProperty(rPropQuery)) {
                rPropQuery.PopPath(iHeader);
                return true;
            }

            rPropQuery.PopPath(iHeader);
        }
    }

    if (rPropQuery.IsSimilarPath("Then Do\\Else Action")) {
        int iIndex = rPropQuery.GetIndex(0);

        assert(iIndex < MAX_ACTION_ARRAY_SIZE && iIndex >= 0);

        if (rPropQuery.IsVar("Then Do\\Else Action[]")) {
            if (rPropQuery.IsRead()) {
                rPropQuery.SetVarString(m_ElseActions[iIndex]->GetDescription(), MAX_STRING_LEN);
            }

            return true;
        }

        if (m_ElseActions[iIndex]) {
            int iHeader = rPropQuery.PushPath("Then Do\\Else Action[]\\");

            if (m_ElseActions[iIndex]->OnProperty(rPropQuery)) {
                rPropQuery.PopPath(iHeader);
                return true;
            }

            rPropQuery.PopPath(iHeader);
        }
    }

    return false;
}

//=============================================================================

bool trigger_ex_object::IsFirstInActionSet(actions_ex_base* pAction)
{
    if (pAction) {
        if (!pAction->GetElse()) {
            if (m_IfActions[0] == pAction) {
                return true;
            }
        } else {
            if (m_ElseActions[0] == pAction) {
                return true;
            }
        }

        return false;
    }

    assert(false);
    return false;
}

//=============================================================================

int trigger_ex_object::GetActionIndex(actions_ex_base* pAction)
{
    for (int i = 0; i < MAX_ACTION_ARRAY_SIZE; i++) {
        if (!pAction->GetElse()) {
            if (m_IfActions[i] == pAction) {
                return i;
            }
        } else {
            if (m_ElseActions[i] == pAction) {
                return i;
            }
        }
    }

    return -1;
}

//=============================================================================

bool trigger_ex_object::HasDialogLine()
{
    int i = 0;
    for (i = 0; i < MAX_ACTION_ARRAY_SIZE; i++) {
        if (!m_IfActions[i]) {
            break;
        }

        if (m_IfActions[i]->GetType() == actions_ex_base::TYPE_ACTION_AI_DIALOG_LINE) {
            return true;
        }
    }

    for (i = 0; i < MAX_ACTION_ARRAY_SIZE; i++) {
        if (!m_ElseActions[i]) {
            break;
        }

        if (m_ElseActions[i]->GetType() == actions_ex_base::TYPE_ACTION_AI_DIALOG_LINE) {
            return true;
        }
    }

    return false;
}

//=============================================================================

bool trigger_ex_object::SetTriggerActionIndexToLabel(const char* pName)
{
    assert(m_iCurrentActionIndex != -1);
/* IJB
    if (m_CurrentActionStatus == ACTION_RUN_IF) {
        for (int i = 0; i < MAX_ACTION_ARRAY_SIZE; i++) {
            if (!m_IfActions[i]) {
                break;
            }

            if (m_IfActions[i]->IsKindOf(trigger_meta_label::GetRTTI())) {
                trigger_meta_label* pLabel = (trigger_meta_label*)m_IfActions[i];
                if (x_strcmp(pLabel->GetLabel(), pName) == 0) {
                    m_iCurrentActionIndex = i;
                    return true;
                }
            }
        }
    } else if (m_CurrentActionStatus == ACTION_RUN_ELSE) {
        for (int i = 0; i < MAX_ACTION_ARRAY_SIZE; i++) {
            if (!m_ElseActions[i]) {
                break;
            }

            if (m_ElseActions[i]->IsKindOf(trigger_meta_label::GetRTTI())) {
                trigger_meta_label* pLabel = (trigger_meta_label*)m_ElseActions[i];
                if (x_strcmp(pLabel->GetLabel(), pName) == 0) {
                    m_iCurrentActionIndex = i;
                    return true;
                }
            }
        }
    } else {
        assert(false);
    }
*/
    return false;
}

//=============================================================================

void trigger_ex_object::GetValidLabels(bool bElseAction, char* pLabels)
{
    int MaxStrLen = 255;
    int CurrentIndex = 0;
/* IJB
    if (!bElseAction) {
        // if labels
        for (int i = 0; i < MAX_ACTION_ARRAY_SIZE; i++) {
            if (!m_IfActions[i]) {
                break;
            }

            if (m_IfActions[i]->IsKindOf(trigger_meta_label::GetRTTI())) {
                trigger_meta_label* pLabel = (trigger_meta_label*)m_IfActions[i];
                if (strlen(pLabel->GetLabel()) > 0) {
                    if (CurrentIndex < MaxStrLen) {
                        strcpy((char*)&pLabels[CurrentIndex], pLabel->GetLabel());
                        CurrentIndex += strlen(pLabel->GetLabel());
                        pLabels[CurrentIndex] = 0;
                        CurrentIndex++;
                    }
                }
            }
        }
    } else {
        // else labels
        for (int i = 0; i < MAX_ACTION_ARRAY_SIZE; i++) {
            if (!m_ElseActions[i]) {
                break;
            }

            if (m_ElseActions[i]->IsKindOf(trigger_meta_label::GetRTTI())) {
                trigger_meta_label* pLabel = (trigger_meta_label*)m_ElseActions[i];
                if (strlen(pLabel->GetLabel()) > 0) {
                    if (CurrentIndex < MaxStrLen) {
                        strcpy((char*)&pLabels[CurrentIndex], pLabel->GetLabel());
                        CurrentIndex += strlen(pLabel->GetLabel());
                        pLabels[CurrentIndex] = 0;
                        CurrentIndex++;
                    }
                }
            }
        }
    }
*/
    pLabels[CurrentIndex] = 0;
    pLabels[CurrentIndex + 1] = 0;
}

//=============================================================================

void trigger_ex_object::SetActionIndex(actions_ex_base* pAction, int Index)
{
    if (Index < 0) {
        return;
    }

    if (!pAction) {
        return;
    }

    if (!pAction->GetElse()) {
        // if the index is too high, set it to the highest used index
        if (Index >= m_NumIfActions) {
            Index = m_NumIfActions - 1;
        }

        for (int i = 0; i < MAX_ACTION_ARRAY_SIZE; i++) {
            if (m_IfActions[i] == pAction) {
                // found the right action, first shift the array to fill in the empty spot
                for (i++; i < MAX_ACTION_ARRAY_SIZE; i++) {
                    m_IfActions[i - 1] = m_IfActions[i];
                }
                m_IfActions[MAX_ACTION_ARRAY_SIZE - 1] = nullptr;

                // now clear out the desired index
                for (i = MAX_ACTION_ARRAY_SIZE - 1; i > Index; i--) {
                    m_IfActions[i] = m_IfActions[i - 1];
                }
                m_IfActions[Index] = pAction;

                return;
            }
        }
    } else {
        // if the index is too high, set it to the highest used index
        if (Index >= m_NumElseActions) {
            Index = m_NumElseActions - 1;
        }

        for (int i = 0; i < MAX_ACTION_ARRAY_SIZE; i++) {
            if (m_ElseActions[i] == pAction) {
                // found the right action, first shift the array to fill in the empty spot
                for (i++; i < MAX_ACTION_ARRAY_SIZE; i++) {
                    m_ElseActions[i - 1] = m_ElseActions[i];
                }
                m_ElseActions[MAX_ACTION_ARRAY_SIZE - 1] = nullptr;

                // now clear out the desired index
                for (i = MAX_ACTION_ARRAY_SIZE - 1; i > Index; i--) {
                    m_ElseActions[i] = m_ElseActions[i - 1];
                }
                m_ElseActions[Index] = pAction;

                return;
            }
        }
    }

    assert(false);
}

//=============================================================================

void trigger_ex_object::RemoveAction(actions_ex_base* pAction, bool bAndDelete)
{
    int i = 0;

    if (!pAction) {
        return;
    }

    if (!pAction->GetElse()) {
        for (i = 0; i < MAX_ACTION_ARRAY_SIZE; i++) {
            if (m_IfActions[i] == pAction) {
                if (bAndDelete) {
                    delete m_IfActions[i];
                }

                m_IfActions[i] = nullptr;
                break;
            }
        }

        if (i == MAX_ACTION_ARRAY_SIZE) {
            //x_DebugMsg("trigger_ex_object::RemoveAction, Cannot find action in table.");
            assert(0);
            return;
        }

        // Shift the array to remove the empty slot...
        for (i++; i < MAX_ACTION_ARRAY_SIZE; i++) {
            m_IfActions[i - 1] = m_IfActions[i];
        }

        m_IfActions[MAX_ACTION_ARRAY_SIZE - 1] = nullptr;
        m_NumIfActions--;
    } else {
        for (i = 0; i < MAX_ACTION_ARRAY_SIZE; i++) {
            if (m_ElseActions[i] == pAction) {
                if (bAndDelete) {
                    delete m_ElseActions[i];
                }

                m_ElseActions[i] = nullptr;
                break;
            }
        }

        if (i == MAX_ACTION_ARRAY_SIZE) {
            //x_DebugMsg("trigger_ex_object::RemoveAction, Cannot find action in table.");
            assert(0);
            return;
        }

        // Shift the array to remove the empty slot...
        for (i++; i < MAX_ACTION_ARRAY_SIZE; i++) {
            m_ElseActions[i - 1] = m_ElseActions[i];
        }

        m_ElseActions[MAX_ACTION_ARRAY_SIZE - 1] = nullptr;
        m_NumElseActions--;
    }
}

//=============================================================================

bool trigger_ex_object::CanSwitchElse(actions_ex_base* pAction)
{
    assert(pAction);
    if (pAction) {
        if (!pAction->GetElse()) {
            if (GetElseActionCount() < MAX_ACTION_ARRAY_SIZE) {
                return true;
            }
        } else {
            if (GetActionCount() < MAX_ACTION_ARRAY_SIZE) {
                return true;
            }
        }
    }
    return false;
}

//=============================================================================

void trigger_ex_object::SwitchElse(actions_ex_base* pAction)
{
    assert(pAction);
    if (pAction && CanSwitchElse(pAction)) {
        RemoveAction(pAction, false);
        pAction->SetElse(!pAction->GetElse());

        if (!pAction->GetElse()) {
            assert(!m_IfActions[m_NumIfActions]);
            m_IfActions[m_NumIfActions] = pAction;
            m_NumIfActions++;
        } else {
            assert(!m_ElseActions[m_NumElseActions]);
            m_ElseActions[m_NumElseActions] = pAction;
            m_NumElseActions++;
        }
    }
}

//=============================================================================

void trigger_ex_object::ExecuteAllActions()
{
    m_iCurrentActionIndex = 0;
    m_bTriggerFired = false;

    SetAttrBits(GetAttrBits() | Object::ATTR_NEEDS_LOGIC_TIME);
}

//=============================================================================

void trigger_ex_object::EnumPropDynamic(prop_enum& rPropList)
{
    for (int i = 0; i < MAX_ACTION_ARRAY_SIZE; i++) {
        if (m_IfActions[i] != nullptr) {
            rPropList.PropEnumInt(xfs("Trigger Object\\AIfType[%d]", i), "", PROP_TYPE_DONT_SHOW);
        }

        if (m_ElseActions[i] != nullptr) {
            rPropList.PropEnumInt(xfs("Trigger Object\\AElseType[%d]", i), "", PROP_TYPE_DONT_SHOW);
        }
    }
}

//=============================================================================

bool trigger_ex_object::OnPropertyDynamic(prop_query& rPropQuery)
{
    if (rPropQuery.IsSimilarPath("Trigger Object\\AIfType[")) {
        int iIndex = rPropQuery.GetIndex(0);
        assert(iIndex < MAX_ACTION_ARRAY_SIZE && iIndex >= 0);

        if (m_IfActions[iIndex] != nullptr && rPropQuery.IsRead()) {
            rPropQuery.SetVarInt(m_IfActions[iIndex]->GetType());
        } else {
            int ActionType = -1;
            ActionType = rPropQuery.GetVarInt();
            AddAction((actions_ex_base::action_ex_types)ActionType, iIndex, false);
        }

        return true;
    }

    if (rPropQuery.IsSimilarPath("Trigger Object\\AElseType[")) {
        int iIndex = rPropQuery.GetIndex(0);
        assert(iIndex < MAX_ACTION_ARRAY_SIZE && iIndex >= 0);

        if (m_ElseActions[iIndex] != nullptr && rPropQuery.IsRead()) {
            rPropQuery.SetVarInt(m_ElseActions[iIndex]->GetType());
        } else {
            int ActionType = -1;
            ActionType = rPropQuery.GetVarInt();
            AddAction((actions_ex_base::action_ex_types)ActionType, iIndex, true);
        }

        return true;
    }

    return false;
}

//=============================================================================

void trigger_ex_object::OnActivate(bool Flag)
{
    m_OnActivate = Flag;

    // give all trigger actions the chance to know that this trigger has activated
    for (int j = 0; j < MAX_ACTION_ARRAY_SIZE; j++) {
        if (m_IfActions[j]) {
            m_IfActions[j]->OnActivate(Flag);
        }

        if (m_ElseActions[j]) {
            m_ElseActions[j]->OnActivate(Flag);
        }
    }

    if (Flag) {
        // work around for volume spatial checks, because its failing for entities already inside
        // the volume when the trigger is activated
        LogicCheckOnActivate();
    } else if (GetAttrBits() & Object::ATTR_NEEDS_LOGIC_TIME) {
        // we are already active, and being deactivated
        m_bTriggerFired = false;
        m_iCurrentActionIndex = -1;
        m_CurrentActionStatus = ACTION_ONHOLD;
        SetAttrBits(GetAttrBits() & ~Object::ATTR_NEEDS_LOGIC_TIME);
        m_DrawActivationIcon = 0;
        m_bQuitRecovery = true;
    }
}

//=============================================================================
// SPACIAL QUERIES
//=============================================================================

bool trigger_ex_object::QueryPlayerInVolume()
{
    // Query for the player...
    slot_id SlotID = objectManager->GetFirst(Object::TYPE_PLAYER);

    bool bVal = false;
    while (SlotID != SLOT_NULL) {
        Object* pObject = objectManager->GetObjectBySlot(SlotID);

        if (pObject != nullptr) {
            bVal |= QueryObjectInVolume(pObject);
        }

        SlotID = objectManager->GetNext(SlotID);
    }

    return bVal;
}

//=============================================================================

bool trigger_ex_object::QueryNpcInVolume()
{
    bool bVal = false;

    for (uint32_t objectTypeCount = 0; objectTypeCount < TYPE_END_OF_LIST; objectTypeCount++) {
        slot_id SlotID = objectManager->GetFirst((Object::type)objectTypeCount);

        if (SlotID != SLOT_NULL) {
            Object* tempObject = objectManager->GetObjectBySlot(SlotID);

            if (tempObject->GetAttrBits() & Object::ATTR_CHARACTER_OBJECT) {
                while (SlotID != SLOT_NULL) {
                    Object* pObject = objectManager->GetObjectBySlot(SlotID);

                    if (pObject != nullptr) {
                        bVal |= QueryObjectInVolume(pObject);
                    }

                    SlotID = objectManager->GetNext(SlotID);
                }
            }
        }
    }

    return bVal;
}

//=============================================================================

bool trigger_ex_object::QueryBulletInVolume()
{
    // Query for bullets in the volume...
    slot_id SlotID = objectManager->GetFirst(Object::TYPE_BULLET_PROJECTILE);

    bool bVal = false;
    while (SlotID != SLOT_NULL) {
        Object* pObject = objectManager->GetObjectBySlot(SlotID);

        if (pObject != nullptr) {
            bVal |= QueryObjectInVolume(pObject);
        }

        SlotID = objectManager->GetNext(SlotID);
    }

    return bVal;
}

//=============================================================================

bool trigger_ex_object::QueryObjectInVolume(Object* pObject)
{
    switch (m_SpatialType) {
    case SPATIAL_TYPE_AXIS_CUBE:
    {
        BBox PlayerBox = pObject->GetBBox();
        BBox ObjectBox = GetBBox();

        if (ObjectBox.Intersect(PlayerBox)) {
            return true;
        } else {
            return false;
        }
    } break;

    case SPATIAL_TYPE_SPHERICAL:
    {
        BBox PlayerBox = pObject->GetBBox();

        if (PlayerBox.Intersect(GetPosition(), m_Dimensions[0])) {
            return true;
        } else {
            return false;
        }
    } break;

    default:
        assert(0);
        break;
    }

    return false;
}

//=============================================================================

void trigger_ex_object::ForceStartTrigger()
{
    m_OnActivate = true;
    m_iCurrentActionIndex = 0;
    m_CurrentActionStatus = ACTION_RUN_IF;

    TurnAttrBitsOn(Object::ATTR_NEEDS_LOGIC_TIME);
}

//=============================================================================

void trigger_ex_object::OnAdvanceLogic(float DeltaTime)
{
    g_TriggerAdvLogicCount++;

    // TODO:SH - Not sure where this needs to go.
    //   Probably inside the if (m_OnActivate) below, but I'm
    //   not sure what action status it should fall under.
    //   Nick - I'm leaving it up to you to relocate it.

    /* IJB - needs GetView
    if (m_bRequiresLineOfSight && !m_bLOSVerified) {
        bool bViewable = false;

        const view* pView = eng_GetView();

        collisionManager->LineOfSightSetup(GetGuid(), pView->GetPosition(), GetPosition());
        // Only need one collision to say that we can't see the trigger
        collisionManager->SetMaxCollisions(1);

        // Perform collision
        collisionManager->CheckCollisions(Object::TYPE_ALL_TYPES, Object::ATTR_BLOCKS_PLAYER_LOS, (Object::object_attr)(Object::ATTR_COLLISION_PERMEABLE | Object::ATTR_LIVING));
        if (collisionManager->m_nCollisions == 0) {
            bViewable = true;
            m_bLOSVerified = true;
        }

        if (!bViewable) {
            m_bTriggerFired = false;
            m_iCurrentActionIndex = -1;
            m_CurrentActionStatus = ACTION_ONHOLD;
            SetAttrBits(GetAttrBits() & ~Object::ATTR_NEEDS_LOGIC_TIME);
            m_DrawActivationIcon = 0;
            m_bLOSVerified = false;
            m_IsSpatialActivated = false;
            return;
        }
    }
*/
    if (m_OnActivate) {
        if (m_CurrentActionStatus == ACTION_ONHOLD) {
            // we need a valid index???
            assert(false);
            return;
        }

        if (m_CurrentActionStatus == ACTION_RUN_IF) {
            assert(m_iCurrentActionIndex != -1);

            // Execute all actions...
            for (int j = m_iCurrentActionIndex; j < MAX_ACTION_ARRAY_SIZE; j++) {
                if (m_IfActions[j] == nullptr) {
                    continue;
                }

                assert(m_IfActions[j]->GetElse() == false);

                if (!ExecuteIndividualAction(m_IfActions[j], j, DeltaTime)) {
                    return;
                } else {
                    m_bTriggerFired = true;
                }
            }
        } else if (m_CurrentActionStatus == ACTION_RUN_ELSE) {
            assert(m_iCurrentActionIndex != -1);

            // Execute all else actions...
            for (int j = m_iCurrentActionIndex; j < MAX_ACTION_ARRAY_SIZE; j++) {
                if (m_ElseActions[j] == nullptr) {
                    continue;
                }

                assert(m_ElseActions[j]->GetElse() == true);

                if (!ExecuteIndividualAction(m_ElseActions[j], j, DeltaTime)) {
                    return;
                } else {
                    m_bTriggerFired = true;
                }
            }
        }
    }

    if (m_bTriggerFired) {
        m_ActivateCount++;
    } else {
        // nothing fired!
        m_bQuitRecovery = true;
    }

    // all actions finished
    m_bTriggerFired = false;
    m_iCurrentActionIndex = -1;
    m_CurrentActionStatus = ACTION_ONHOLD;
    SetAttrBits(GetAttrBits() & ~Object::ATTR_NEEDS_LOGIC_TIME);
    m_DrawActivationIcon = 0;
    m_bLOSVerified = false;
}

//===========================================================================
// return true to continue, false to end advance

bool trigger_ex_object::ExecuteIndividualAction(actions_ex_base* pAction, int ActionIndex, float DeltaTime)
{
    assert(pAction);
    m_DrawError = false; // reset the error draw

    pAction->ResetErrorRegister();
    if (pAction->Execute(DeltaTime)) {
        // true, iterate the index
        m_iCurrentActionIndex = ActionIndex + 1;
        assert(m_iCurrentActionIndex != -1);
    } else {
        // false, leave index where it is
        if (pAction->WasExecuteAnError()) {
            m_DrawError = true;
        }

        return false;
    }
/* IJB
    if (pAction->IsKindOf(trigger_meta_base::GetRTTI())) {
        if (pAction->IsKindOf(trigger_meta_breakpoint::GetRTTI())) {
            // this is a breakpoint, don't stop on the PS2
            return true;
        }

        if (pAction->IsKindOf(trigger_meta_label::GetRTTI())) {
            // this is a label, this should not stop the advance
            return true;
        }

        // stop execution every time we encounter a meta not handled above
        return false;
    }
*/
    return true;
}

//===========================================================================

bool trigger_ex_object::RetryOnError()
{
    // we only want to do re-tries in the editor
    return false;
}
