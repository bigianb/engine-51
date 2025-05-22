#pragma once

#include "../Property.h"
#include "../render/Geom.h"
#include "../animation/animData.h"
#include "../inventory/Inventory.h"

#include "LocoCharAnimPlayer.h"
#include "LocoAimController.h"
#include "CharacterPhysics.h"
#include "LocoAdditiveController.h"
/*
#include "LocoMaskController.hpp"

#include "LocoLipSyncController.hpp"
#include "LocoEyeController.hpp"
*/

class loco_state;
class skin_inst;
class ObjectManager;

class loco : public prop_interface
{
public:
    CREATE_RTTI_BASE( Object );

    enum state
    {
        STATE_NULL = -1, // Undefined

        STATE_IDLE,      // Idle and turning
        STATE_MOVE,      // Moving
        STATE_PLAY_ANIM, // Playing an animation

        STATE_TOTAL
    };

    // List of motions
    enum motion
    {
        // NOTE: Do not change the order of FORWARD/LEFT/BACK/RIGHT or you'll break the locomotion!
        MOTION_FRONT,
        MOTION_LEFT,
        MOTION_BACK,
        MOTION_RIGHT,

        MOTION_IDLE,
        MOTION_IDLE_TURN_LEFT,
        MOTION_IDLE_TURN_RIGHT,
        MOTION_IDLE_TURN_180,

        MOTION_TRANSITION,
        MOTION_NULL,
    };

    enum move_style
    {
        MOVE_STYLE_NULL = -1,

        MOVE_STYLE_WALK,
        MOVE_STYLE_RUN,
        MOVE_STYLE_RUNAIM,
        MOVE_STYLE_PROWL,
        MOVE_STYLE_CROUCH,
        MOVE_STYLE_CROUCHAIM,
        MOVE_STYLE_CHARGE,
        MOVE_STYLE_CHARGE_FAST,

        MOVE_STYLE_COUNT
    };

    // NOTE: You MUST add the anim set defines to the "anim_type" enum
    enum move_style_anim
    {
        MOVE_STYLE_ANIM_NULL = -1, // -1 So we can use "ANIM_TYPE_COUNT * MoveStyle"

        // NOTE: These defines should match the defines for a style!
        MOVE_STYLE_ANIM_IDLE,
        MOVE_STYLE_ANIM_IDLE_TURN_LEFT,
        MOVE_STYLE_ANIM_IDLE_TURN_RIGHT,
        MOVE_STYLE_ANIM_IDLE_TURN_180,
        MOVE_STYLE_ANIM_IDLE_TURN_LEFT_180,
        MOVE_STYLE_ANIM_IDLE_TURN_RIGHT_180,
        MOVE_STYLE_ANIM_IDLE_FIDGET,
        MOVE_STYLE_ANIM_MOVE_FRONT,
        MOVE_STYLE_ANIM_MOVE_LEFT,
        MOVE_STYLE_ANIM_MOVE_BACK,
        MOVE_STYLE_ANIM_MOVE_RIGHT,
        MOVE_STYLE_ANIM_COUNT
    };

    // List of bone mask types
    enum bone_masks_type
    {
        BONE_MASKS_TYPE_FULL_BODY,
        BONE_MASKS_TYPE_UPPER_BODY,
        BONE_MASKS_TYPE_FACE,
        BONE_MASKS_TYPE_AIM_VERT,
        BONE_MASKS_TYPE_AIM_HORIZ,
        BONE_MASKS_TYPE_NO_AIM_VERT,
        BONE_MASKS_TYPE_NO_AIM_HORIZ,
        BONE_MASKS_TYPE_RELOAD_SHOOT,

        BONE_MASKS_TYPE_COUNT
    };

    // List of animation types that map to an index
    enum anim_type
    {
        ANIM_NULL = -1,

        // NOTE: Each move style, must match the "move_style_anim" enums!
        //       See SteveB if you want to add to this list

        // "MOVE_STYLE_WALK" style anims
        ANIM_WALK_IDLE,
        ANIM_WALK_IDLE_TURN_LEFT,
        ANIM_WALK_IDLE_TURN_RIGHT,
        ANIM_WALK_IDLE_TURN_180,
        ANIM_WALK_IDLE_TURN_LEFT_180,
        ANIM_WALK_IDLE_TURN_RIGHT_180,
        ANIM_WALK_IDLE_FIDGET,
        ANIM_WALK_MOVE_FRONT,
        ANIM_WALK_MOVE_LEFT,
        ANIM_WALK_MOVE_BACK,
        ANIM_WALK_MOVE_RIGHT,

        // "MOVE_STYLE_RUN" style anims
        ANIM_RUN_IDLE,
        ANIM_RUN_IDLE_TURN_LEFT,
        ANIM_RUN_IDLE_TURN_RIGHT,
        ANIM_RUN_IDLE_TURN_180,
        ANIM_RUN_IDLE_TURN_LEFT_180,
        ANIM_RUN_IDLE_TURN_RIGHT_180,
        ANIM_RUN_IDLE_FIDGET,
        ANIM_RUN_MOVE_FRONT,
        ANIM_RUN_MOVE_LEFT,
        ANIM_RUN_MOVE_BACK,
        ANIM_RUN_MOVE_RIGHT,

        // "MOVE_STYLE_RUNAIM" style anims
        ANIM_RUNAIM_IDLE,
        ANIM_RUNAIM_IDLE_TURN_LEFT,
        ANIM_RUNAIM_IDLE_TURN_RIGHT,
        ANIM_RUNAIM_IDLE_TURN_180,
        ANIM_RUNAIM_IDLE_TURN_LEFT_180,
        ANIM_RUNAIM_IDLE_TURN_RIGHT_180,
        ANIM_RUNAIM_IDLE_FIDGET,
        ANIM_RUNAIM_MOVE_FRONT,
        ANIM_RUNAIM_MOVE_LEFT,
        ANIM_RUNAIM_MOVE_BACK,
        ANIM_RUNAIM_MOVE_RIGHT,

        // "PROWL" style anims
        ANIM_PROWL_IDLE,
        ANIM_PROWL_IDLE_TURN_LEFT,
        ANIM_PROWL_IDLE_TURN_RIGHT,
        ANIM_PROWL_IDLE_TURN_180,
        ANIM_PROWL_IDLE_TURN_LEFT_180,
        ANIM_PROWL_IDLE_TURN_RIGHT_180,
        ANIM_PROWL_IDLE_FIDGET,
        ANIM_PROWL_MOVE_FRONT,
        ANIM_PROWL_MOVE_LEFT,
        ANIM_PROWL_MOVE_BACK,
        ANIM_PROWL_MOVE_RIGHT,

        // "MOVE_STYLE_CROUCH" style anims
        ANIM_CROUCH_IDLE,
        ANIM_CROUCH_IDLE_TURN_LEFT,
        ANIM_CROUCH_IDLE_TURN_RIGHT,
        ANIM_CROUCH_IDLE_TURN_180,
        ANIM_CROUCH_IDLE_TURN_LEFT_180,
        ANIM_CROUCH_IDLE_TURN_RIGHT_180,
        ANIM_CROUCH_IDLE_FIDGET,
        ANIM_CROUCH_MOVE_FRONT,
        ANIM_CROUCH_MOVE_LEFT,
        ANIM_CROUCH_MOVE_BACK,
        ANIM_CROUCH_MOVE_RIGHT,

        // "MOVE_STYLE_CROUCH" style anims
        ANIM_CROUCHAIM_IDLE,
        ANIM_CROUCHAIM_IDLE_TURN_LEFT,
        ANIM_CROUCHAIM_IDLE_TURN_RIGHT,
        ANIM_CROUCHAIM_IDLE_TURN_180,
        ANIM_CROUCHAIM_IDLE_TURN_LEFT_180,
        ANIM_CROUCHAIM_IDLE_TURN_RIGHT_180,
        ANIM_CROUCHAIM_IDLE_FIDGET,
        ANIM_CROUCHAIM_MOVE_FRONT,
        ANIM_CROUCHAIM_MOVE_LEFT,
        ANIM_CROUCHAIM_MOVE_BACK,
        ANIM_CROUCHAIM_MOVE_RIGHT,

        // "MOVE_STYLE_CHARGE" style anims
        ANIM_CHARGE_IDLE,
        ANIM_CHARGE_IDLE_TURN_LEFT,
        ANIM_CHARGE_IDLE_TURN_RIGHT,
        ANIM_CHARGE_IDLE_TURN_180,
        ANIM_CHARGE_IDLE_TURN_LEFT_180,
        ANIM_CHARGE_IDLE_TURN_RIGHT_180,
        ANIM_CHARGE_IDLE_FIDGET,
        ANIM_CHARGE_MOVE_FRONT,
        ANIM_CHARGE_MOVE_LEFT,
        ANIM_CHARGE_MOVE_BACK,
        ANIM_CHARGE_MOVE_RIGHT,

        // "MOVE_STYLE_CHARGE_FAST" style anims
        ANIM_CHARGE_FAST_IDLE,
        ANIM_CHARGE_FAST_IDLE_TURN_LEFT,
        ANIM_CHARGE_FAST_IDLE_TURN_RIGHT,
        ANIM_CHARGE_FAST_IDLE_TURN_180,
        ANIM_CHARGE_FAST_IDLE_TURN_LEFT_180,
        ANIM_CHARGE_FAST_IDLE_TURN_RIGHT_180,
        ANIM_CHARGE_FAST_IDLE_FIDGET,
        ANIM_CHARGE_FAST_MOVE_FRONT,
        ANIM_CHARGE_FAST_MOVE_LEFT,
        ANIM_CHARGE_FAST_MOVE_BACK,
        ANIM_CHARGE_FAST_MOVE_RIGHT,

        // Cover Anims
        ANIM_COVER_IDLE,
        ANIM_COVER_SHOOT,
        ANIM_COVER_PEEK,
        ANIM_COVER_GRENADE,
        ANIM_COVER_COVERINGFIRE,

        // Evade anims
        ANIM_EVADE_LEFT,
        ANIM_EVADE_RIGHT,
        ANIM_GRENADE_EVADE_LEFT,
        ANIM_GRENADE_EVADE_RIGHT,

        // Grenade animations
        ANIM_GRENADE_THROW_LONG,
        ANIM_GRENADE_THROW_SHORT,
        ANIM_GRENADE_THROW_OVER_OBJECT,

        // Melee animations
        ANIM_MELEE_BACK_LEFT,
        ANIM_MELEE_BACK_RIGHT,
        ANIM_MELEE_SHORT,
        ANIM_MELEE_LONG,
        ANIM_MELEE_LEAP,

        // Misc animations
        ANIM_SPOT_TARGET,
        ANIM_HEAR_TARGET,
        ANIM_LOST_TARGET,
        ANIM_ADD_REACT_RAGE,
        ANIM_FACE_IDLE,
        ANIM_DRAIN_LIFE,
        ANIM_REQUEST_COVER,
        ANIM_REQUEST_ATTACK,
        ANIM_RESPONSE,

        // Death animations
        ANIM_DEATH,
        ANIM_DEATH_SIMPLE,

        ANIM_DEATH_HARD_SHOT_IN_BACK_HIGH,
        ANIM_DEATH_HARD_SHOT_IN_BACK_MED,
        ANIM_DEATH_HARD_SHOT_IN_BACK_LOW,

        ANIM_DEATH_HARD_SHOT_IN_FRONT_HIGH,
        ANIM_DEATH_HARD_SHOT_IN_FRONT_MED,
        ANIM_DEATH_HARD_SHOT_IN_FRONT_LOW,

        ANIM_DEATH_LIGHT_SHOT_IN_BACK_HIGH,
        ANIM_DEATH_LIGHT_SHOT_IN_BACK_MED,
        ANIM_DEATH_LIGHT_SHOT_IN_BACK_LOW,

        ANIM_DEATH_LIGHT_SHOT_IN_FRONT_HIGH,
        ANIM_DEATH_LIGHT_SHOT_IN_FRONT_MED,
        ANIM_DEATH_LIGHT_SHOT_IN_FRONT_LOW,

        ANIM_DEATH_CROUCH,
        ANIM_DEATH_EXPLOSION,

        // Toss weapon
        ANIM_TOSS_WEAPON,

        // Reload animations
        ANIM_RELOAD,
        ANIM_RELOAD_SMP,
        ANIM_RELOAD_DUAL_SMP,
        ANIM_RELOAD_DUAL_SHT,
        ANIM_RELOAD_SNIPER,
        ANIM_RELOAD_SHOTGUN,
        ANIM_RELOAD_GAUSS,
        ANIM_RELOAD_DESERT_EAGLE,
        ANIM_RELOAD_MSN,
        ANIM_RELOAD_BBG,
        ANIM_RELOAD_TRA,
        ANIM_RELOAD_SCN,

        // Shoot animations
        ANIM_SHOOT,
        ANIM_SHOOT_SECONDARY,
        ANIM_SHOOT_SMP,
        ANIM_SHOOT_DUAL_SMP,
        ANIM_SHOOT_DUAL_SHT,
        ANIM_SHOOT_DUAL_SHT_SECONDARY,
        ANIM_SHOOT_SNIPER,

        ANIM_SHOOT_SHOTGUN,

        ANIM_SHOOT_GAUSS,
        ANIM_SHOOT_DESERT_EAGLE,
        ANIM_SHOOT_MHG,
        ANIM_SHOOT_MSN,
        ANIM_SHOOT_BBG,
        ANIM_SHOOT_TRA,
        ANIM_SHOOT_MUTANT,
        ANIM_SHOOT_SCN,

        ANIM_SHOOT_SECONDARY_SMP,
        ANIM_SHOOT_SECONDARY_SNIPER,

        ANIM_SHOOT_SECONDARY_SHOTGUN,

        ANIM_SHOOT_SECONDARY_GAUSS,
        ANIM_SHOOT_SECONDARY_DESERT_EAGLE,
        ANIM_SHOOT_SECONDARY_MHG,
        ANIM_SHOOT_SECONDARY_MSN,
        ANIM_SHOOT_SECONDARY_MUTANT,
        ANIM_SHOOT_SECONDARY_SCN,

        // Idle Shoot animations
        ANIM_SHOOT_IDLE_SMP,
        ANIM_SHOOT_IDLE_SNIPER,
        ANIM_SHOOT_IDLE_SHOTGUN,
        ANIM_SHOOT_IDLE_GAUSS,
        ANIM_SHOOT_IDLE_DESERT_EAGLE,
        ANIM_SHOOT_IDLE_MHG,
        ANIM_SHOOT_IDLE_MSN,
        ANIM_SHOOT_IDLE_BBG,
        ANIM_SHOOT_IDLE_SCN,
        ANIM_SHOOT_IDLE_TRA,
        ANIM_SHOOT_IDLE_MUTANT,

        // Idle Crounching Shoot animations
        ANIM_SHOOT_CROUCH_IDLE_SMP,
        ANIM_SHOOT_CROUCH_IDLE_SNIPER,
        ANIM_SHOOT_CROUCH_IDLE_SHOTGUN,
        ANIM_SHOOT_CROUCH_IDLE_GAUSS,
        ANIM_SHOOT_CROUCH_IDLE_DESERT_EAGLE,
        ANIM_SHOOT_CROUCH_IDLE_MHG,
        ANIM_SHOOT_CROUCH_IDLE_MSN,
        ANIM_SHOOT_CROUCH_IDLE_BBG,
        ANIM_SHOOT_CROUCH_IDLE_SCN,
        ANIM_SHOOT_CROUCH_IDLE_TRA,
        ANIM_SHOOT_CROUCH_IDLE_MUTANT,

        // Damage animations
        ANIM_DAMAGE_STEP_BACK,
        ANIM_DAMAGE_STEP_FORWARD,
        ANIM_DAMAGE_STEP_LEFT,
        ANIM_DAMAGE_STEP_RIGHT,

        ANIM_DAMAGE_SHOCK,
        ANIM_DAMAGE_PARASITE,

        ANIM_DAMAGE_PLAYER_MELEE_0,
        ANIM_MESON_STUN,

        // idle pain anims
        ANIM_PAIN_IDLE_FRONT,
        ANIM_PAIN_IDLE_BACK,

        ANIM_PROJECTILE_ATTACHED,

        // Additive impact anims
        ANIM_ADD_IMPACT_HEAD_FRONT,
        ANIM_ADD_IMPACT_HEAD_BACK,
        ANIM_ADD_IMPACT_TORSO_FRONT,
        ANIM_ADD_IMPACT_TORSO_BACK,
        ANIM_ADD_IMPACT_SHOULDER_LEFT_FRONT,
        ANIM_ADD_IMPACT_SHOULDER_RIGHT_FRONT,
        ANIM_ADD_IMPACT_SHOULDER_LEFT_BACK,
        ANIM_ADD_IMPACT_SHOULDER_RIGHT_BACK,

        // These are used by the frienldy scientist - they need
        // to be made into a common naming scheme like the above defines...
        ANIM_UA_HEAD_SHAKE,
        ANIM_UA_HEAD_NOD,
        ANIM_UA_CONVERSATION1,
        ANIM_UA_CONVERSATION2,
        ANIM_UA_CONVERSATION3,
        ANIM_UA_CHAIR_SEATED_LOOKAROUND,
        ANIM_UA_CHAIR_SEATED_IDLE,
        ANIM_UA_COME_HERE,
        ANIM_UA_COWER_TO_DEATH,
        ANIM_HAND_SIGNAL_ENEMY_FORWARD,

        // Mutant tank (Theta) specific anims
        ANIM_ATTACK_HOWL,
        ANIM_ATTACK_CLAW,
        ANIM_ATTACK_CHARGE_SWING,
        ANIM_ATTACK_CHARGE_MISS,
        ANIM_ATTACK_RANGED_ATTACK,
        ANIM_ATTACK_BUBBLE,
        ANIM_STAGE0_RAGE,
        ANIM_STAGE1_RAGE,
        ANIM_STAGE2_RAGE,
        ANIM_STAGE3_RAGE,
        ANIM_CANISTER_TO,
        ANIM_CANISTER_IDLE,
        ANIM_CANISTER_SMASH,
        ANIM_CANISTER_FROM,
        ANIM_SHIELD_ON,
        ANIM_SHIELD_SHOOT,
        ANIM_SHIELD_REGEN,

        ANIM_GRATE_TO,
        ANIM_GRATE_SMASH,
        ANIM_GRATE_FROM,
        ANIM_PERCH_TO,
        ANIM_PERCH_FROM,
        ANIM_THETA_CROUCH,
        ANIM_THETA_JUMP,

        // These are not used by any current animations but are here
        // so the old code compiles...
        ANIM_EMOTION1,
        ANIM_EMOTION2,
        ANIM_ALRT_SMP_FRENZY,
        ANIM_ALRT_SHT_FRENZY,
        ANIM_CONVULSE1,

        // Lip sync anims
        ANIM_LIP_SYNC_TEST,

        // Jumping anims
        ANIM_JUMP_OVER,
        ANIM_JUMP_UP,
        ANIM_JUMP_DOWN,
        ANIM_JUMP,

        // Misc multi-player anims
        ANIM_FALL,
        ANIM_GRENADE,
        ANIM_MELEE,

        ANIM_CROUCH_ENTER,
        ANIM_CROUCH_EXIT,

        ANIM_STAND_LEAN_LEFT,
        ANIM_STAND_LEAN_RIGHT,

        ANIM_CROUCH_LEAN_LEFT,
        ANIM_CROUCH_LEAN_RIGHT,

        // END OF WHOLE LIST ENUM
        ANIM_TOTAL,
    };

    // Animation flags
    enum amim_flags
    {
        //--------------------------------------------------------------------------------------------
        // Type flags used by "PlayAnim" functions
        //--------------------------------------------------------------------------------------------

        // If SET, the loco will play the animation for the specified number of cycles
        // (DEFAULT if no play type flags are set)
        ANIM_FLAG_PLAY_TYPE_CYCLIC = (1 << 0),

        // If SET, the loco will play the animation for the specified number of seconds
        ANIM_FLAG_PLAY_TYPE_TIMED = (1 << 1),

        // All the type flags combines
        // NOTE: Update me when any new type flags are added/modified
        ANIM_FLAG_PLAY_TYPE_ALL = (ANIM_FLAG_PLAY_TYPE_CYCLIC | ANIM_FLAG_PLAY_TYPE_TIMED),

        //--------------------------------------------------------------------------------------------
        // State flags used by "PlayAnim" functions only
        //--------------------------------------------------------------------------------------------

        // If SET, the loco will resume the previous state upon completion of animation playback
        // (DEFAULT if no end state flags are set)
        ANIM_FLAG_END_STATE_RESUME = (1 << 2),

        // If SET, the loco will remain in the STATE_PLAY_ANIM upon completion of animation playback
        ANIM_FLAG_END_STATE_HOLD = (1 << 3),

        // NOTE: Update me when any new state flags are added/modified
        ANIM_FLAG_END_STATE_ALL = (ANIM_FLAG_END_STATE_RESUME | ANIM_FLAG_END_STATE_HOLD),

        //--------------------------------------------------------------------------------------------
        // Misc flags used by "PlayAnim" functions only
        //--------------------------------------------------------------------------------------------

        // If SET, the animation play will begin immediately, even if there is a blend currently happending
        // (which will cause a pop). If NOT SET, the animation will play as soon any current blend has ended -
        // if there is a blend currently happending, then the anim is queued up (NOTE: only the last request is queued)
        // The only reason I could see this being SET is for death animations from explosions etc -
        // for all other cases, the blend is so fast and smooth, that you will not notice any delay
        ANIM_FLAG_INTERRUPT_BLEND = (1 << 4),

        // If SET, the controller blend between the blend anim and the current anims goes the longest route
        ANIM_FLAG_REVERSE_YAW_BLEND = (1 << 5),

        // If SET, the aimer controller is blended out over a short time,
        // then blended back in at the very end of the animation
        ANIM_FLAG_TURN_OFF_AIMER = (1 << 6),

        // If SET, we will not blend into this anim even if
        // a blend time is set on the anim info
        ANIM_FLAG_DO_NO_BLENDING = (1 << 7),

        //--------------------------------------------------------------------------------------------
        // Misc flags used by all functions
        //--------------------------------------------------------------------------------------------

        // If SET, and same animation is playing, the animation will be restarted from the beginning
        ANIM_FLAG_RESTART_IF_SAME_ANIM = (1 << 8),

        // If SET, the new animation is started from the current frame position
        ANIM_FLAG_START_ON_SAME_FRAME = (1 << 9),

        // Applies to when there are multiple animations with the same name.
        // If SET and play anim by name is used, the first animation in the list will be played.
        // If SET and play anim by index is used, the specified index will be played
        // If NOT SET, then a random animation from all animations of the same name will be played
        ANIM_FLAG_TURN_OFF_RANDOM_SELECTION = (1 << 10),

        // These are user flags that you can use for priority testing in your AI code
        ANIM_FLAG_PRIORITY_LOW = (1 << 11),
        ANIM_FLAG_PRIORITY_MEDIUM = (1 << 12),
        ANIM_FLAG_PRIORITY_HIGH = (1 << 13),

        //--------------------------------------------------------------------------------------------
        // Mask type flags used by "PlayLipSyncAnim" functions only
        //--------------------------------------------------------------------------------------------

        // If SET, playback will be full body
        // (DEFAULT if no mask flags are set)
        ANIM_FLAG_MASK_TYPE_FULL_BODY = (1 << 14),

        // If SET, the whole upper body gets the animation
        ANIM_FLAG_MASK_TYPE_UPPER_BODY = (1 << 15),

        // If SET, just the face gets the animation
        ANIM_FLAG_MASK_TYPE_FACE = (1 << 16),

        // If SET, just the face gets the animation
        ANIM_FLAG_MASK_TYPE_DYNAMIC = (1 << 17),

        // NOTE: Update me when any new mask flags are added/modified
        ANIM_FLAG_MASK_TYPE_ALL = (ANIM_FLAG_MASK_TYPE_FULL_BODY | ANIM_FLAG_MASK_TYPE_UPPER_BODY | ANIM_FLAG_MASK_TYPE_FACE | ANIM_FLAG_MASK_TYPE_DYNAMIC),

        //--------------------------------------------------------------------------------------------
        // Controller flags used by "PlayAdditiveAnim" functions only
        //--------------------------------------------------------------------------------------------

        // These specify which controller to use (defaults to controller0 if no controller flags are set).
        // There are currently 2 controllers.
        ANIM_FLAG_CONTROLLER0 = (1 << 18),
        ANIM_FLAG_CONTROLLER1 = (1 << 19),

        //--------------------------------------------------------------------------------------------
        // Audio flags used by "PlayLipSync" functions only
        //--------------------------------------------------------------------------------------------

        // Tells lip sync controller playback is in artist viewer so that
        // audio can be triggered when event is reached (if present)
        ANIM_FLAG_ARTIST_VIEWER = (1 << 20),

        // Tells lip sync controller that this came from a cinema and not to turn the voice off
        ANIM_FLAG_CINEMA = (1 << 21),

        // Turns cinema relative mode on
        ANIM_FLAG_CINEMA_RELATIVE_MODE = (1 << 22),

        // Turns cover relative mode on
        ANIM_FLAG_COVER_RELATIVE_MODE = (1 << 23),
    };

    
    // Structures
    
public:
    // Animation indices and blend times for move style
    struct move_style_info_default
    {
        // IJB: this default param is wrong
        move_style_info_default(ResourceManager* = nullptr);

        // Anim group
        AnimGroup::handle m_hAnimGroup;

        // Animations
        int16_t m_iAnims[MOVE_STYLE_ANIM_COUNT]; // Idle and move animation indices

        // Blend related
        float m_IdleBlendTime;         // Blend times when in idle state
        float m_MoveBlendTime;         // Blend times when in move state
        float m_FromPlayAnimBlendTime; // Blend time after playing a generic animation

        // Turning info
        Radian m_MoveTurnRate; // Max delta yaw when moving and turning
    };

    struct move_style_info
    {
        // Anim group
        AnimGroup::handle m_hAnimGroup; // Animation group

        // Animations
        int16_t m_iAnims[MOVE_STYLE_ANIM_COUNT]; // Idle and move animation indices

        // Blend related
        float m_IdleBlendTime;         // Blend times when in idle state
        float m_MoveBlendTime;         // Blend times when in move state
        float m_FromPlayAnimBlendTime; // Blend time after playing a generic animation

        // Turning info
        Radian m_MoveTurnRate; // Max delta yaw when moving and turning

        // Misc
        float  m_AimerBlendSpeed;        // Controls aim blending speed (default 1.0f)
        Radian m_IdleDeltaYawMin;        // Min delta yaw threshold before playing idle anim
        Radian m_IdleDeltaYawMax;        // Max delta yaw threshold before playing idle anim
        Radian m_IdleTurnDeltaYawMin;    // Min delta yaw threshold before playing idle turn anim
        Radian m_IdleTurnDeltaYawMax;    // Max delta yaw threshold before playing idle turn anim
        Radian m_IdleTurn180DeltaYawMin; // Min delta yaw threshold before playing idle turn 180 anim
        Radian m_IdleTurn180DeltaYawMax; // Max delta yaw threshold before playing idle turn 180 anim

        // Constructor (in loco.cpp)
        move_style_info(ResourceManager* rm);

        // Initializer
        void InitDefaults(move_style_info_default& Defaults);
    };

    // Animation lookup table. Maps type to actual index
    struct anim_lookup_table
    {
        // List of animation indices
        int16_t m_Index[ANIM_TOTAL];

        // Constructor
        anim_lookup_table()
        {
            Clear();
        }

        // Clear all entries
        void Clear()
        {
            // Set all anims to -1
            memset(this, -1, sizeof(*this));
        }
    };

    // Simple structure used to setup anim indices
    struct anim_lookup
    {
        anim_type   m_AnimType; // Loco enum type
        const char* m_pName;    // Name of animation
    };

    // Simple structure used to setup bone masks
    struct bone_lookup
    {
        char* m_pName;  // Name of bone
        float m_Weight; // Weight of bone
    };

    
    // Private functions
    
private:
    // These functions is only used internally - never call this outside of loco!
    bool  HasArrivedAtPosition(const Vector3& Pos, float ArriveDistSqr);
    float ComputeExactArriveDistSqr();

    
    // Public functions
    
public:
    // Static property functions
    static const char* GetMoveStyleName(int MoveStyle);
    static const char* GetMoveStyleHeader(int MoveStyle);
    static const char* GetMoveStyleAnimName(int MoveStyleAnim);
    static const char* GetMoveStyleEnum();
    static move_style  GetMoveStyleByName(const char* pName);

    // Position related functions
    loco(ObjectManager* pObjectManager, collision_mgr* collisionManager, ResourceManager* rm);


    const Matrix4* ComputeL2W();
    const Vector3& GetPosition() const { return m_Player.GetPosition(); }
    void           SetPosition(const Vector3& Position);
    void           SetPitch(Radian Pitch);

    const Vector3& GetMoveAt() const { return m_MoveAt; }
    const Vector3& GetHeadLookAt() const { return m_HeadLookAt; }
    const Vector3& GetBodyLookAt() const { return m_BodyLookAt; }
    bool           IsExactMove() const { return m_bExactMove; }
    bool           IsExactMoveBlending() const { return m_bExactMoveBlending; }
    bool           IsExactLook() const { return m_bExactLook; }
    bool           IsMoveAtSnap() const { return m_bMoveAtSnap; }

    // Bone related function
    const Vector3& GetEyeOffset() const;
    Vector3        GetEyePosition() const;
    Radian         GetSightYaw() const;

    const Matrix4& GetWeaponBoneL2W(int iHand = 0);
    Matrix4        GetWeaponL2W(int iHand = 0);
    const Matrix4& GetGrenadeBoneL2W();
    Vector3        GetGrenadeBonePosition();

    Matrix4 GetFlagL2W();

    int GetWeaponBoneIndex(int iHand = 0) { return m_Player.m_iWeaponBone[iHand]; }
    int GetGrenadeBoneIndex() { return m_Player.m_iGrenadeBone; }
    int GetFlagBoneIndex() { return m_Player.m_iFlagBone; }

    // Destination functions
    bool IsAtDestination();
    bool IsAtPosition(const Vector3& Pos);
    bool IsAtPosition(const Vector3& Pos, float ArriveDistSqr);
    void SetArriveDist(float ArriveDist);
    void SetArriveDistSqr(float ArriveDistSqr);

    // Look at functions
    bool IsExactLookComplete();

    // Delta position functions
    const Vector3& GetDeltaPos() const { return m_DeltaPos; }
    void           SetDeltaPosScale(const Vector3& Scale);
    Radian         GetDeltaYaw() const { return m_DeltaYaw; }

    // Bone functions
    int  GetNBones();
    int  GetNActiveBones();
    bool IsAnimLoaded();

    // Actor functions
    float GetActorCollisionRadius();
    float GetActorCollisionHeight();

    // Move and look functions
    // NOTE: "SetLookAt" sets both the head and body look at to the same location
    void ResetMoveAndLookAt();
    void SetMoveAt(const Vector3& Target);
    void SetHeadLookAt(const Vector3& Target, bool bSetEyesLookAt = true);
    void SetBodyLookAt(const Vector3& Target);
    void SetLookAt(const Vector3& Target, bool bSetEyesLookAt = true);
    void SetEyesLookAt(const Vector3& Target);
    void SetExactMove(bool bExact);
    void SetExactLook(bool bExact);
    void SetMoveAtSnap(bool bSnap) { m_bMoveAtSnap = bSnap; }
    void SetAimerBlendSpeed(float AimerBlendSpeed); // Default = 1.0f
    void SetAimerWeight(float Weight, float BlendTime = 0.0f);
    void ClearMoveFlags();

    // Cinema relative mode functions
    void           SetCinemaRelativeMode(bool bEnable);
    void           SetCinemaRelativeInfo(const Vector3& Pos, Radian Yaw);
    const Vector3& GetCinemaRelativePos() const { return m_CinemaRelativePos; }
    Radian         GetCinemaRelativeYaw() const { return m_CinemaRelativeYaw; }

    // Cover relative mode functions
    void           SetCoverRelativeMode(bool bEnable);
    void           SetCoverRelativeInfo(const Vector3& Pos, Radian Yaw);
    const Vector3& GetCoverRelativePos() const { return m_CoverRelativePos; }
    Radian         GetCoverRelativeYaw() const { return m_CoverRelativeYaw; }

    // Ghost mode functions
    void SetGhostMode(bool bEnable);
    bool GetGhostMode() const { return m_bGhostMode; }
    void SetGhostIsMoving(bool bMoving);
    bool GetGhostIsMoving() const { return m_bGhostIsMoving; }

    // Initialize and main logic functions
    virtual void OnInit(const Geom* pGeom, const char* pAnimFileName, guid ObjectGuid = 0);
    virtual void OnAdvance(float nSeconds);

    // Weapon functions
    virtual bool SetWeapon(inven_item InvenWeapon)
    {
        return false;
    }

    // Initialization functions
    void               SetupAnimLookupTable(anim_lookup AnimLookups[]);
    int                GetAnimIndex(loco::anim_type AnimType);
    int                GetMoveStyleAnimIndex(loco::move_style_anim MoveStyleAnim);
    anim_lookup_table& GetAnimLookupTable() { return m_AnimLookupTable; }
    void               InitBoneMasks(const Geom* pGeom);
    const Geom::BoneMask&    GetBoneMasks(loco::bone_masks_type Type);
    void               InitProperties(const Geom* pGeom);

    // State functions
    const char* GetStateName(loco::state State) const;
    bool        SetState(loco::state State);
    loco::state GetState() const;
    const char* GetStateName() const;
    loco::state GetPrevState() const;
    const char* GetPrevStateName() const;
    void        SetStateAnimRate(loco::state State, float Rate);
    float       GetStateAnimRate(loco::state State) const;

    // Anim functions
    void SetAimerBoneMasks(bool bAiming, float BlendTime);

    // Play animation functions
    // IN:
    //      hAnimGroup   = Handle to specific anim group to use
    //      iAnim        = Index of specific animation to play
    //      pAnim        = Name of specific animation to play
    //      BlendInTime  = Time it takes to blend this animation in
    //      BlendOutTime = Time it takes to blend this animation out
    //      BlendTime    = Time it takes to blend out the old animation and blend in this new animation
    //      Flags        = Various control flags (see anim_flags anum)

    // Masked controller functions
    bool PlayMaskedAnim(const char* pAnimGroup, const char* pAnim, float BlendTime, uint32_t Flags = 0);
    bool PlayMaskedAnim(int iAnim, bone_masks_type MaskType, float BoneMaskBlendTime, uint32_t Flags = 0);
    bool PlayMaskedAnim(loco::anim_type AnimType, bone_masks_type MaskType, float BoneMaskBlendTime, uint32_t Flags = 0);

    // Additive controller functions
    bool PlayAdditiveAnim(int iAnim, float BlendInTime = 0.1f, float BlendOutTime = 0.1f, uint32_t Flags = 0);
    bool PlayAdditiveAnim(loco::anim_type AnimType, float BlendInTime = 0.1f, float BlendOutTime = 0.1f, uint32_t Flags = 0);
    bool PlayAdditiveAnim(const char* pAnim, float BlendInTime = 0.1f, float BlendOutTime = 0.1f, uint32_t Flags = 0);

    // Lip sync controller functions
    bool PlayLipSyncAnim(const AnimGroup::handle& hAnimGroup, int iAnim, uint32_t VoiceID, uint32_t Flags = ANIM_FLAG_MASK_TYPE_FACE);
    bool PlayLipSyncAnim(const AnimGroup::handle& hAnimGroup, const char* pAnimName, const char* pAudioName, uint32_t Flags = ANIM_FLAG_MASK_TYPE_FACE);
    bool PlayLipSyncAnim(const AnimGroup::handle& hAnimGroup, const char* pAnimName, uint32_t VoiceID, uint32_t Flags = ANIM_FLAG_MASK_TYPE_FACE);

    bool PlayLipSyncAnim(int iAnim, const char* pAudioName, uint32_t Flags = ANIM_FLAG_MASK_TYPE_FACE);
    bool PlayLipSyncAnim(loco::anim_type AnimType, const char* pAudioName, uint32_t Flags = ANIM_FLAG_MASK_TYPE_FACE);
    bool PlayLipSyncAnim(const char* pAnimName, const char* pAudioName, uint32_t Flags = ANIM_FLAG_MASK_TYPE_FACE);

    void UpdateFaceIdle(float DeltaTime);
    void UpdateEyeTracking(float DeltaTime);
    void UpdateDynamicLipSyncBoneMasks();

    // Main motion controller functions
    bool         PlayAnim(const AnimGroup::handle& hAnimGroup, int iAnim, float BlendTime = DEFAULT_BLEND_TIME, uint32_t Flags = 0, float PlayTime = 0.0f);
    bool         PlayAnim(const AnimGroup::handle& hAnimGroup, const char* pAnim, float BlendTime = DEFAULT_BLEND_TIME, uint32_t Flags = 0, float PlayTime = 0.0f);
    bool         PlayAnim(const char* pAnimGroup, const char* pAnim, float BlendTime = DEFAULT_BLEND_TIME, uint32_t Flags = 0, float PlayTime = 0.0f);
    bool         PlayAnim(int iAnim, float BlendTime = DEFAULT_BLEND_TIME, uint32_t Flags = 0, float PlayTime = 0.0f);
    bool         PlayAnim(const char* pAnim, float BlendTime = DEFAULT_BLEND_TIME, uint32_t Flags = 0, float PlayTime = 0.0f);
    bool         PlayAnim(loco::anim_type AnimType, float BlendTime = DEFAULT_BLEND_TIME, uint32_t Flags = 0, float PlayTime = 0.0f);
    virtual bool PlayDeathAnim(loco::anim_type AnimType, float BlendTime = DEFAULT_BLEND_TIME, uint32_t Flags = ANIM_FLAG_INTERRUPT_BLEND);
    virtual bool PlayDeathAnim(const AnimGroup::handle& hAnimGroup, const char* pAnim, float BlendTime = DEFAULT_BLEND_TIME, uint32_t Flags = ANIM_FLAG_INTERRUPT_BLEND);
    bool         IsPlayAnimComplete() const;
    uint32_t     GetPlayAnimFlags() const;

    // Controller access functions
    
    loco_motion_controller&   GetMotionController(uint32_t AnimFlags = 0);
    loco_aim_controller&      GetAimController(uint32_t AnimFlags = 0);
    
    loco_mask_controller&     GetMaskController(uint32_t AnimFlags = 0);
    /*
    loco_additive_controller& GetAdditiveController(uint32_t AnimFlags = 0);
    loco_lip_sync_controller& GetLipSyncController(uint32_t AnimFlags = 0);
    loco_eye_controller&      GetEyeController(uint32_t AnimFlags = 0);
*/
    // Misc
    virtual void SetMoveStyle(move_style Style);
    void         SetBlendMoveStyle(move_style Style);
    void         SetBlendMoveStyleAmount(float Amount);

    void SwitchMoveStyleSmoothly(move_style Style, move_style_anim Anim);
    void UpdateMoveStyle();

    move_style_anim    GetCurrentMoveStyleAnim();
    int                GetMoveStyleAnimIndex(move_style Style, move_style_anim Anim);
    virtual bool       IsValidMoveStyle(move_style Style);
    virtual move_style GetValidMoveStyle(move_style Style);
    void               SetMoveStyleDefaults(move_style Style, const move_style_info_default& Defaults);

    // Internal math functions
    void   ComputeHeadAim(Radian FacingYawBias, Radian& H, Radian& V);
    void   ComputeBodyAim(Radian FacingYawBias, Radian& H);
    void   ComputeMotion(bool          bAllowDir[4], // F L B R
                         Radian        LookYaw,
                         Radian        MoveYaw,
                         loco::motion& Motion,
                         Radian&       DeltaYaw);
    void   ComputeValidMotion(loco::motion& Motion, Radian& DeltaYaw);
    Radian ComputeMoveDir();

    // Motion functions
    motion      GetMotion() const { return m_Motion; }
    Radian      GetMotionYaw(motion Motion) const;
    const char* GetMotionName(motion Motion) const;
    Vector3     GetMotionLookPoint() const;
    void        SetAllowMotion(motion Motion, bool bEnable);
    bool        IsMotionAllowed(motion Motion);

    // Yaw functions. NOTE: All yaws are in game world space
    void   ApplyDeltaYaw(Radian DeltaYaw);
    Radian GetYaw();
    void   SetYaw(Radian Yaw);
    void   SetYawFacingTarget(Radian TargetYaw, Radian MaxDeltaYaw);
    Radian GetAimerYaw();

    // Convenient local to world matrix functions
    void    SetL2W(const Matrix4& L2W);
    Matrix4 GetL2W();

    void ChangeToPreviousState();

    // Misc query functions
    bool                     HasPassedMoveAt() const { return m_bPassedMoveAt; }
    void                     SetDead(bool bDead) { m_bDead = bDead; }
    void                     SetFaceIdleEnabled(bool bEnabled) { m_bFaceIdleEnabled = bEnabled; }
    bool                     IsFaceIdleEnabled() const { return m_bFaceIdleEnabled; }
    bool                     IsStuck() const { return m_bLocoIsStuck; }
    move_style               GetMoveStyle() const { return m_MoveStyle; }
    move_style               GetBlendMoveStyle() const { return m_BlendMoveStyle; }
    float                    GetBlendMoveStyleAmount() const { return m_BlendMoveStyleAmount; }
    const AnimGroup::handle& GetAnimGroupHandle() const { return m_hAnimGroup; }

    // use run aim?
    void SetUseAimMoveStyles(bool bUseAimStyles) { m_bUseAimMoveStyles = bUseAimStyles; }

    void SetDoAdvancePhysics(bool bDoAdvancePhysics) { m_bAdvancePhysics = bDoAdvancePhysics; }

    Vector3 GetBonePosition(int Bone);
    int     GetRandomBone();

    
    // Data
    
public:

    loco_char_anim_player m_Player;  // The big animation player handles all animation
    character_physics     m_Physics; // Handles movement

protected:
    loco_aim_controller      m_AimController;         // Controller responsible for aiming
    loco_mask_controller     m_MaskController;        // Masked controller
    loco_additive_controller m_AdditiveController1; // Additive controllers
    loco_additive_controller m_AdditiveController2; // Additive controllers
//    loco_lip_sync_controller m_LipSyncController;     // Lip sync controller
//    loco_eye_controller      m_EyeController;         // Additive eye controller
    
protected:
    ObjectManager* objectManager;
    ResourceManager* resourceManager;

    uint32_t m_bDead : 1,                // true if character is dead
        m_bLocoIsStuck : 1,              // true if character has been stuck for a while
        m_bAllowFrontMotion : 1,         // Controls if character can play MOVE_FRONT animations
        m_bAllowLeftMotion : 1,          // Controls if character can play MOVE_LEFT animations
        m_bAllowBackMotion : 1,          // Controls if character can play MOVE_BACK animations
        m_bAllowRightMotion : 1,         // Controls if character can play MOVE_RIGHT animations
        m_bPassedMoveAt : 1,             // true if NPC has gone thru the move at
        m_bExactMove : 1,                // Use for scripted stuff - pixel perfect if true!
        m_bExactLook : 1,                // Use for scripted stuff - pixel perfect if true!
        m_bExactLookComplete : 1,        // true if exact look at is complete
        m_bUseAimMoveStyles : 1,         // true if we want to use RUNAIM in place of RUN.
        m_bMoveAtSnap : 1,               // true if NPC should snap if passing move at
        m_bExactMoveBlending : 1,        // true if blending from MOVE->IDLE with exact move on
        m_bExactMoveBlendingStarted : 1, // true if blending has begun.
        m_bAdvancePhysics : 1,           // true if physics should be advanced
        m_bGhostMode : 1,                // true if loco is being used for a net ghost character
        m_bGhostIsMoving : 1,            // true if ghost should be moving ie. not idle
        m_bFrameMatchMoveAnim : 1,       // Used for smooth move style switching
        m_bDynamicLipSyncAnim : 1,       // true if our lipsync anims switches masks depending upon loco state.
        m_bStateChangedThisTick : 1,     // true if our state changed this tick.
        m_bFaceIdleEnabled : 1;          // true if blinking etc is allowed

    AnimGroup::handle m_hAnimGroup;    // Assigned animation group
    state             m_AnimState;     // Current animation state
    uint32_t          m_PlayAnimFlags; // Current play animation flags
    Vector3           m_DeltaPosScale; // Use to slide delta pos
    Vector3           m_DeltaPos;      // Current delta position
    Radian            m_DeltaYaw;      // Current delta yaw

    Radian m_GhostYaw;   // Current ghost yaw
    Radian m_GhostPitch; // Current ghost pitch

    Vector3 m_MoveAt;            // Location to move to
    Vector3 m_ExactMoveBlendPos; // Position to blend from

    float   m_ArriveDistSqr; // Min distance squared to get from move at
    Vector3 m_HeadLookAt;    // Head location to look at
    Vector3 m_BodyLookAt;    // Body location to look at

    motion m_Motion;       // Current motion
    motion m_ToTransition; // Current transition motion

    loco_state* m_pHead;   // Head of state list
    loco_state* m_pActive; // Currently active state
    loco_state* m_pPrev;   // Previously active state

    move_style              m_MoveStyle;                              // Current move style
    move_style              m_BlendMoveStyle;                         // Blend move style
    float                   m_BlendMoveStyleAmount;                   // 0 = m_MoveStyle, 1 = m_BlendMoveStyle
    move_style_info         m_MoveStyleInfo;                          // Current move style info (anims to use etc)
    move_style_info_default m_MoveStyleInfoDefault[MOVE_STYLE_COUNT]; // Default values per movestyle

    float m_StateAnimRate[STATE_TOTAL]; // Anim rate modifier

    Vector3 m_CinemaRelativePos; // Cinema relative position
    Radian  m_CinemaRelativeYaw; // Cinema relative yaw

    Vector3 m_CoverRelativePos; // Cover relative position
    Radian  m_CoverRelativeYaw; // Cover relative yaw

    anim_lookup_table m_AnimLookupTable; // Anim type -> index lookup

    const Geom*             m_pGeom;                                   // Geom that loco is controlling
    const Geom::BoneMask* m_pBoneMasks[loco::BONE_MASKS_TYPE_COUNT]; // List of bone masks

    float m_FaceIdleTimer;       // Time before next face idle
    float m_FaceIdleMinInterval; // Min time before next face idle
    float m_FaceIdleMaxInterval; // Max time before next face idle

    friend class loco_state;
    friend struct loco_play_anim;
    friend struct loco_idle;
    friend struct loco_move;

    
    // Editor
    
public:
    virtual void OnEnumProp(prop_enum& List);
    virtual bool OnProperty(prop_query& I);

    
    // Public static data
    

    static Geom::BoneMask s_ZeroBoneMasks;
    static Geom::BoneMask s_OneBoneMasks;
};


// LOCOMOTION STATES

class loco_state
{
public:
    loco_state(loco& Loco, loco::state State);
    virtual void OnAdvance(float nSeconds) { (void)nSeconds; }
    virtual void OnEnter() {}
    virtual bool OnExit() { return true; }
    virtual void OnInit() {}
    virtual bool IsComplete() { return true; }

    loco&       m_Base;
    loco::state m_State;
    loco_state* m_pNext;
};



struct loco_play_anim : public loco_state
{
    loco_play_anim(loco& Loco);
    virtual void OnAdvance(float nSeconds);
    virtual void OnEnter();
    virtual bool OnExit();
    virtual bool IsComplete() { return m_bComplete; }
    bool         PlayAnim(const AnimGroup::handle& hAnimGroup, int iAnim, float BlendTime, uint32_t Flags, float PlayTime);

    loco::state m_PrevState; // Previous state before this one was entered
    uint32_t    m_Flags;     // Play anim flags
    float       m_PlayTime;  // How long animation should play in secs or cylces
    float       m_Timer;     // Time in state
    bool        m_bComplete; // true if play animation is complete
};



struct loco_idle : public loco_state
{
    loco_idle(loco& Loco);
    virtual void OnAdvance(float nSeconds);
    virtual void OnEnter();
    virtual bool OnExit();

    float                 m_Timer;       // General timer
    float                 m_FidgetTimer; // Fidget timer
    loco::move_style_anim m_IdleAnim;    // Idle animation to play
};



struct loco_move : public loco_state
{
    loco_move(loco& Loco);
    virtual void OnAdvance(float nSeconds);
    virtual void OnEnter();
    virtual bool OnExit();

    float m_Timer;      // General timer
    bool  m_bFirstTime; // true if first time advance has been called in state
};


// INLINE FUNCTIONS


inline Radian loco::GetYaw()
{
    return m_Player.GetFacingYaw();
}



inline void loco::ChangeToPreviousState()
{
    if (m_pPrev) {
        SetState(m_pPrev->m_State);
    } else {
        SetState(STATE_IDLE);
    }
}



inline void loco::SetDeltaPosScale(const Vector3& Scale)
{
    // Sanity check
    assert(Scale.GetX() >= -20.0f);
    assert(Scale.GetY() >= -20.0f);
    assert(Scale.GetZ() >= -20.0f);
    assert(Scale.GetX() <= 20.0f);
    assert(Scale.GetY() <= 20.0f);
    assert(Scale.GetZ() <= 20.0f);

    m_DeltaPosScale = Scale;
}

inline float loco::GetActorCollisionRadius()
{
    return m_Physics.GetActorCollisionRadius();
}



inline float loco::GetActorCollisionHeight()
{
    return m_Physics.GetColHeight();
}

inline bool loco::PlayLipSyncAnim(const char* pAnimName, const char* pAudioName, uint32_t Flags)
{
    return PlayLipSyncAnim(m_hAnimGroup, pAnimName, pAudioName, Flags);
}


inline void loco::SetAimerBoneMasks(bool bAiming, float BlendTime)
{
    // Use aiming masks?
    if (bAiming) {
        // Set aiming masks
        m_AimController.SetBoneMasks(GetBoneMasks(BONE_MASKS_TYPE_AIM_VERT),
                                     GetBoneMasks(BONE_MASKS_TYPE_AIM_HORIZ),
                                     BlendTime);
    } else {
        // Set no aiming masks
        m_AimController.SetBoneMasks(GetBoneMasks(BONE_MASKS_TYPE_NO_AIM_VERT),
                                     GetBoneMasks(BONE_MASKS_TYPE_NO_AIM_HORIZ),
                                     BlendTime);
    }
}


inline bool loco::PlayAnim(int iAnim, float BlendTime, uint32_t Flags, float PlayTime)
{
    return PlayAnim(m_hAnimGroup, iAnim, BlendTime, Flags, PlayTime);
}



inline uint32_t loco::GetPlayAnimFlags() const
{
    return m_PlayAnimFlags;
}



inline loco_aim_controller& loco::GetAimController(uint32_t AnimFlags)
{
    return m_AimController;
}



inline loco_mask_controller& loco::GetMaskController(uint32_t AnimFlags)
{
    return m_MaskController;
}

/*

inline loco_lip_sync_controller& loco::GetLipSyncController(uint32_t AnimFlags)
{
    // Not presently used
    (void)AnimFlags;

    return m_LipSyncController;
}



inline loco_eye_controller& loco::GetEyeController(uint32_t AnimFlags)
{
    // Not presently used
    (void)AnimFlags;

    return m_EyeController;
}
*/


inline void loco::SetArriveDist(float ArriveDist)
{
    assert(isvalid(ArriveDist)) ;
    m_ArriveDistSqr = sqrt(ArriveDist);
}



inline void loco::SetArriveDistSqr(float ArriveDistSqr)
{
    assert(isvalid(ArriveDistSqr)) ;
    m_ArriveDistSqr = ArriveDistSqr;
}



inline bool loco::IsAtDestination()
{
    // Arrived within distance?
    return IsAtPosition(m_MoveAt, m_ArriveDistSqr);
}



inline bool loco::IsAtPosition(const Vector3& Pos)
{
    // Use default arrive distance
    return IsAtPosition(Pos, m_ArriveDistSqr);
}



inline bool loco::IsExactLookComplete()
{
    return (m_bExactLook && m_bExactLookComplete);
}



inline const Matrix4* loco::ComputeL2W()
{
    return m_Player.GetBoneL2Ws();
}



inline const Vector3& loco::GetEyeOffset() const
{
    return m_Player.m_AimAtOffset;
}



inline Vector3 loco::GetEyePosition() const
{
    const Vector3& Pos = GetPosition();
    const Vector3& Offset = GetEyeOffset();
    return Pos + Offset;
}



inline Radian loco::GetSightYaw() const
{
    return m_Player.m_HeadL2W.GetRotation().yaw + R_180;
}



inline const Matrix4& loco::GetWeaponBoneL2W(int iHand)
{
    return m_Player.GetBoneL2W(m_Player.m_iWeaponBone[iHand]);
}



inline const Matrix4& loco::GetGrenadeBoneL2W()
{
    return m_Player.GetBoneL2W(m_Player.m_iGrenadeBone);
}



inline Vector3 loco::GetGrenadeBonePosition()
{
    return m_Player.GetBonePosition(m_Player.m_iGrenadeBone);
}



inline int loco::GetNBones()
{
    return (m_Player.GetNBones());
}



inline int loco::GetNActiveBones()
{
    return m_Player.GetNActiveBones();
}



inline bool loco::IsAnimLoaded()
{
    return (m_hAnimGroup.isLoaded());
}


/*
inline void loco::SetEyesLookAt(const Vector3& Target)
{
    //assert(Target.IsValid()) ;
    m_EyeController.SetLookAt(Target);
}
*/


inline void loco::SetExactMove(bool bExact)
{
    m_bExactMove = bExact;
}



inline void loco::SetExactLook(bool bExact)
{
    m_bExactLook = bExact;
}



inline void loco::SetGhostIsMoving(bool bMoving)
{
    m_bGhostIsMoving = bMoving;
}
