#pragma once

#include "../math/Vector3.h"
#include "CharacterState.h"

// #include "navigation\ng_connection2.hpp"
// #include "objects\actor\actor.hpp"
// #include "BaseStates\Character_Trigger_State.hpp"
// #include "BaseStates\Character_Follow_State.hpp"
// #include "Objects\Event.hpp"

enum eNPCName
{
    NPC_GENERIC,
    NPC_BRIDGES,
    NPC_CARSON,
    NPC_CRISPY, 
    NPC_CRISPY_MUTATED,
    NPC_CHEW,   
    NPC_DRCRAY,
    NPC_FERRI,
    NPC_LEONARD,
    NPC_MCCANN, 
    NPC_MRWHITE,
    NPC_RAMIREZ,
    NPC_VICTOR,    
    NPC_SCIENTIST,
    NPC_TECHNICIAN,
    NPC_THETA,
    NPC_ALIENGLOB,    
    NPC_MUTANT,
    NPC_BLACKOPS,
    NPC_BLACKOP_LEADER,
    NPC_UNKNOWN,
    NPC_MAX,
};

#define MAX_ENTITIES_FOR_PROXIMITY_CHECK    6
#define MAX_NODES_IN_PATH_LIST              50
const int k_MaxSplinePoints = 6;

const int k_MaxAllies = 4;

//=========================================================================
// FORWARD DECLARATIONS
//=========================================================================
class character ;
class cover_node;
class character_hold_state ;
class action_ai_dialog_line;
class character_task_set;

//=========================================================================
// character_hold_state class
//  [12/12/2003]
//
//  This state is used when the NPC is not active. Does nothing and ignores all.
//=========================================================================

class character_hold_state : public character_state
{
public:
// Construction / destruction
    character_hold_state   ( character& Character, states State );
    
    inline virtual bool        IgnorePain      ( const pain& Pain );
    inline virtual bool        IgnoreAlerts    (); 
    inline virtual bool        IgnoreSight     ();
    inline virtual bool        IgnoreSound     ();
    inline virtual bool        IgnoreAttacks   ();

// Virtual functions    
    virtual const char*GetStateName ()            { return "CHARACTER_HOLD"; }    
    virtual const char*GetPhaseName ( int thePhase )    { (void)thePhase; return "PHASE NONE"; }
};

//=========================================================================
// CLASSES
// Base class character
class character : public actor
{
// Real time type information
public:
    CREATE_RTTI( character, actor, object )

//=========================================================================
// Friends of the family
//=========================================================================
friend class character_state ;
friend class character_death_state;
friend class god ;

//=========================================================================
// Defines
//=========================================================================
public:

    enum eCharacterSubtypes
    {
        SUBTYPE_NONE = -1
    };

    enum eDialogActors
    {
        INVALID_ACTION_TYPES = -1,        
        DIALOG_ACTOR_A,
        DIALOG_ACTOR_B,
        DIALOG_ACTOR_C,
        DIALOG_ACTOR_D,
        DIALOG_ACTOR_NONE,
    };

    enum eAwarenessLevel
    {
        AWARENESS_NONE,
        AWARENESS_COMBAT_READY,
        AWARENESS_ALERT,
        AWARENESS_SEARCHING,
        AWARENESS_ACQUIRING_TARGET,
        AWARENESS_TARGET_LOST,
        AWARENESS_TARGET_SPOTTED,
    };

    enum eLookatModes
    {
        LOOKAT_NONE,
        LOOKAT_FORWARD,
        LOOKAT_LAST_LOCATION_OF_INTEREST,
        LOOKAT_NAVIGATION,
        LOOKAT_NAV_FORWARD,
        LOOKAT_CURRENT_TARGET,
        LOOKAT_DOING_GOAL,
        LOOKAT_ENTERING_COVER,
        LOOKAT_MARKER_ALIGN,
        LOOKAT_INTEREST_OBJECT,
    };

    enum eGoalTypes
    {
        GOAL_NONE,
		GOAL_IDLE,
		GOAL_TURNTO_TARGET,
		GOAL_TURNTO_LOCATION,
		GOAL_LOOKAT_TARGET,
		GOAL_LOOKAT_LOCATION,
		GOAL_GOTO_TARGET,
		GOAL_GOTO_LOCATION,
		GOAL_RETREAT_FROM_TARGET,
		GOAL_RETREAT_FROM_LOCATION,
		GOAL_PLAY_ANIMATION,
		GOAL_PLAY_ANIMATION_SCALED_TO_TARGET,
		GOAL_SAY_DIALOG,
        GOAL_COUNT
    };

    enum eGoalFailureReasons
    {
        FAILED_GOAL_NONE,
        FAILED_GOAL_STUCK,
        FAILED_GOAL_CANT_REACH,
        FAILED_GOAL_BELOW_TARGET,
        FAILED_GOAL_ABOVE_TARGET,
        FAILED_GOAL_TIMED_OUT,
        FAILED_GOAL_PLAY_ANIM_ERROR,
        FAILED_GOAL_PLAY_DIALOG_ERROR,
        FAILED_GOAL_VOICE_TIMED_OUT,
        FAILED_GOAL_ACTOR_COLLISION,
        FAILED_GOAL_OBJECT_COLLISION,
        FAILED_GOAL_NOWHERE_TO_RETREAT,
        FAILED_GOAL_OUTSIDE_LEASH,
    };

    enum eAvoidCharacterAttempt
    {
        AVOID_NONE,
        AVOID_THROUGH,
        AVOID_WAIT,
    };

    enum eEscapeAttempt
    {
        ESCAPE_NONE = -1,
        ESCAPE_GO_LEFT,
        ESCAPE_GO_RIGHT,
        ESCAPE_GO_BACK_LEFT,
        ESCAPE_GO_BACK_RIGHT,
        ESCAPE_HEAD_EXPLODE,
        ESCAPE_SLIGHT_DELAY
    };

    enum eCoverPreference 
    {
        COVER_PREF_NONE,
        COVER_PREF_CLOSER_TO_TARGET,
        COVER_PREF_FURTHER_FROM_TARGET,
        COVER_PREF_CLOSER_TO_US,
    };

    struct dialog_info
    {        
        void            Clear();
        void            SetData( eDialogType dialogType, const char* dialogName , const char* animName , const char* animPkg, uint32_t Flags = 0, uint8_t DialogFlags = 0, float BlendOutTime = 0.25f );
        // data
        char            m_DialogName[64];
        char            m_AnimName[64];   
        char            m_AnimGroup[64];
        float             m_BlendOutTime;
        eDialogType     m_DialogType;
        uint32_t             m_AnimFlags;
        uint8_t              m_DialogFlags;
    };

    struct goal_info
    {
        Vector3             m_Offset;
        Vector3             m_Location;       
        Vector3             m_RetreatGoalDestination; 
        char                m_AnimName[64];   
        char                m_AnimGroup[64];
        char                m_DialogName[64];    
        guid                m_TargetGuid;
        radian              m_FOV;
        float                 m_AnimPlayTime ;        
        float                 m_DesiredDist;
        float                 m_DialogAnimBlendOutTime;
        eGoalTypes          m_GoalType;
        loco::move_style    m_MoveStyle;                
        loco::anim_type     m_AnimType;
        float                 m_AnimBlendTime;
        uint32_t                 m_AnimFlags ;
        uint32_t                 m_IsExact:1,
                            m_UseCurrentTarget:1,
                            m_DialogBlockOnDialog:1,
                            m_DialogKillAnim:1,
                            m_FaceTargetOnRetreat:1;
        uint8_t                  m_DialogFlags;
    };
    
//=========================================================================
// Class functions
//=========================================================================
public:
                        character       ();
    virtual             ~character() ;


//=========================================================================
// Inherited virtual functions from base class
//=========================================================================
public:
    virtual void        OnInit                  ();
    virtual void        OnKill                  ();   
    virtual void        OnAdvanceLogic          ( float DeltaTime );      
    virtual void        OnRender                ();
            
    virtual void        InitLoco                ();
            void        InitTweaks              ();


    virtual void        OnRenderTransparent     ();
    virtual int         GetMaterial             () const ;
    virtual bbox        GetLocalBBox            () const;
    virtual void        OnEvent                 ( const event& Event );
    virtual void        OnEnumProp              ( prop_enum&    List ) ;
    virtual bool       OnProperty              ( prop_query&   I    ) ;
    virtual float         GetYaw                  ();

inline virtual bool    IsCharacter             ();
    
    virtual void        OnKilledEnemy           ( guid deadEnemy );
    virtual void        OnKilledAlly            ( guid deadAlly );
    virtual void        OnDamagedEnemy          ( guid damagedEnemy );
    virtual const char* GetLogicalName          ();

//=========================================================================
// Virtual functions
//=========================================================================
public:
    virtual void        OnHitByFriendly         ( guid ShooterID  );
    virtual bool       OnPlayFullBodyImpactAnim( loco::anim_type AnimType, float BlendTime, uint32_t Flags );
    virtual bool       GetHasDrainableCorpse   ()            { return false; }

    // Navigation helper functions
    virtual void        HandleNavMapUpdate      ( nav_map::update_info& Update );
    virtual bool       AlwaysAllowMoveBackwards()            { return false; }
            int         GetSubtype              ()            { return m_Subtype; }    
            void        SetSubtype              ( int newSubtype)   { m_Subtype = newSubtype; }    
    
    inline  nav_connection_slot_id  GetCurrentConnection();
    inline  bool       GetHasClearJumpAttack   ();
    inline  bool       GetHasClearLOSFeet      ();
    inline  bool       GetHasClearLOSEyes      ();
    inline  bool       GetCanMoveLeft          ();
    inline  bool       GetCanMoveRight         ();
    inline  bool       GetCanMoveBack          ();
    inline  bool       GetCanMoveForward       ();
    inline  bool       GetInNavMap             ();
    inline  float         GetTimeOutofNavMap      ();
    inline  float         GetLeashDistance        () const;
    inline  guid        GetLeashGuid            () const;
    inline  void        RecalcPath              ()            { m_NeedsToRecalcPath = true; }
            bool       GetClearToMoveTo        ( const Vector3& location, bool doCollisionCheck = true );
            float         GetSqrHorzDistToNextNav ();
            bool       GetIsAtDestination      ();

        // pathing
    inline  bool       GetIsFollowingPath      ();
    inline  void        SetIsFollowingPath      ( bool followingPath );
            const char* GetEscapeAttemptName    ();                        
            const char* GetAvoidAttemptName     ();                        
    inline  void        SetCollisionIgnoreGuid  ( guid ignoreGuid );
            bool       GetIsStuck              ()            { return m_IsStuck; }

    // Collision functions
    virtual void        ApplyBoneBBoxCollision  ();

    // State functions
    static  int         GetStateCount           ();            
    static  const char* GetStateName            ( int Index ); 
    static  const char* GetStatesEnum           ( void);
    static  character_state::states GetStateByName ( const char* pName );                                    
    inline  void        ClearInitialState       ();
            void        SetPostLipSyncState     ( character_state::states lipSyncState ) { m_PostLipsyncState = lipSyncState; }
    

    // Event functions
    virtual bool       OnAnimEvent( const anim_event& Event, const Vector3& WorldPos );

    // Alert functions                          
    virtual void        OnAlert                 ( alert_package& Package );

    // Pain/damage functions
    virtual void        OnPain                  ( const pain& Pain );
    virtual bool       TakeDamage              ( const pain& Pain );
    virtual void        KillMe                  ();
            void        DropWeapon              ();
            void        DropAmmoForWeapon       ( inven_item weaponItem );
            void        DropGrenade             ();
            void        DropInventory           ();
            void        SetIgnorePain           ( bool bShouldIgnorePain );

    // On event or state functions.
    virtual void        OnThink                 () ;    
    virtual void        OnBeingShotAt           ( object::type ProjectileType , guid ShooterID );
    virtual void        OnHitFriendly           ( guid FriendlyID );
    virtual void        OnGrenadeAlert          ( alert_package& Package );
    virtual void        OnActivate              ( bool Flag );
    virtual void        OnDeath                 ();
    virtual void        OnSpawn                 ();
    virtual void        OnContagionDOT          ();

    // Trigger state functions
    virtual bool       TriggerState            ( action_ai_base* pAction );
            void        SetTriggerStateData     ( action_ai_base* pContext );
            bool       TriggerState            ( character_trigger_state::TriggerData pData );
            void        SetTriggerStateData     ( character_trigger_state::TriggerData pData );
    inline  character_trigger_state::TriggerData GetTriggerStateData();
        //task list functions
    inline  void        SetPendingTaskListGuid  ( guid newGuid );
            character_task_set* GetPendingTaskList();
            void        SetPostTriggerTarget    ( guid target );

    // Dialog functions
    virtual bool       PlayDialog              ( eDialogType dialogType, const char* dialogName = NULL, bool hotVoice = false, const char* animName = NULL, const char* animPkg = NULL, uint32_t Flags = 0, uint8_t DialogFlags = 0, float BlendOutTime = 0.25f );
    virtual const char* GetDialogPrefix         ();
    virtual int         GetNumberVoiceActors    () = 0;
    inline  int         GetVoiceID              ();
    inline  void        SetVoiceID              ( int ID );

    // Sound/sight related functions
            // sight functions
    inline  float         GetTargetNotSeenTimer   ();
    inline  Vector3     GetLastKnownLocationOfTarget();
    inline  void        SetLastKnownLocationOfTarget( const Vector3& Location );
    inline  Vector3     GetLastSeenLocationOfTarget();
    inline  float         GetSightRadius          ();
            // sound functions
    inline  Vector3     GetLastSoundPosition    ();
    inline  float         GetTimeSinceLastSound   ();
    inline  void        ClearSoundHeard         ();
    inline  bool       GetSoundHeard           ();
    inline  virtual void Listen                 ( conversation_packet& rPacket );
    virtual bool       SetPotentialListener    ( bool bPotentialListener ) ;
    virtual void        SetActiveListener       ( bool bActiveListener ) ;
            void        SendAlert               ( alert_package::alert_type alertType, guid CauseGuid = 0, bool forceSend = false );
            void        SendAlert               ( alert_package::alert_type alertType, const Vector3& sourceLocation, guid CauseGuid = 0, bool forceSend = false );
            void        SetAllyAcquiredTarget   ( bool targetAcquired )    { m_bAllyAcquiredTarget = targetAcquired; }
            bool       GetAllyAcquiredTarget   ()                    { return m_bAllyAcquiredTarget; }

    // Awareness functions
    inline  Vector3     GetLastLocationOfInterest();
    inline  void        SetLastLocationOfInterest( const Vector3& Location );
    inline  eAwarenessLevel GetAwarenessLevel   ();
            void        SetAwarenessLevel       ( eAwarenessLevel newAwarenessLevel );
            const char* GetAwarenessLevelName   ();
            character_state::states GetStateFromAwareness();

    // cloak functions
    virtual void        UpdateCloak             ( float DeltaTime );
    virtual void        Cloak                   ();
    virtual void        Decloak                 ();

    // Rendering functions
    inline  anim_group::handle& GetAnimGroupHandle();

    // Target functions
            void        UpdateTarget            () ;
            void        SetTargetGuid           ( guid Guid, bool shoutAboutTarget = true ); 
    inline  guid        GetTargetGuid           ();
            bool       IsTargetEnemy           ( guid Guid = 0 );
            bool       IsValidTarget           ( guid targetGuid );
            void        SetOverrideTarget       ( guid OverrideGuid );
            bool       OfferOverrideTarget     ( guid OverrideGuid );
    virtual bool       CanTargetGlobs          ()                { return false; }
            bool       GetIsKungFuTime         ();
            

            // aiming
    inline  bool       CanSeeTarget            ();
    inline  bool       CanShootAtTarget        ();
    inline  radian      GetAimToTargetYaw       ();
            bool       IsTargetInFiringCone    ( const Vector3& Target );//, Vector3& FiringPoint, Vector3& AimPoint, radian Threshold = R_20 );
            
            bool       IsFacingTarget          ( const Vector3& TargetPos, radian SightFOV ) ;
            bool       IsFacingTarget          ( radian SightFOV );
            bool       IsTargetFacingUs        ( radian SightFOV );

            bool       IsTargetInSightCone     ( float SightRadius, radian SightFOV );   // Uses m_SightDist and m_FOV
            bool       IsTargetInSightCone     ( const Vector3& TargetPos, float SightRadius, radian SightFOV ) ;
            bool       IsTargetInSightCone     ( const Vector3& TargetPos, uint8_t brightness ) ;   // Uses m_SightDist and m_FOV
            bool       IsTargetInSightCone     () ;                             // Uses m_SightDist and m_FOV and current target
        // navigation
            Vector3     GetToTargetWeighted     ( guid target = 0, eOffsetPos offset = OFFSET_NONE );     // return the distance to either the passed in target, or our target if NULL passed.
            Vector3     GetToTarget             ( guid target = 0, eOffsetPos offset = OFFSET_NONE );     // return the distance to either the passed in target, or our target if NULL passed.
            bool       IsNewTargetCloser       ( guid Guid );
            bool       UpdateCanReachTarget    ( const Vector3& targetPosition, float reachRadius );
    inline  bool       CanReachTarget          ();
    inline  bool       CanPathToTarget         ();
    inline  bool       GetCanReachGoalTarget   ();
            Vector3     GetTargetPosWithOffset  ( guid target = 0, eOffsetPos offset = OFFSET_NONE );
            void        ForceTargetSeen         ()    { m_bTargetSeen = true; }

    // info tracking
    inline  bool       IsCombatReady           ();
    inline  void        SetCombatReady          ( bool isReady );
    inline  bool       HasAllies               ();
            bool       HasAlliesWithin         ( float distSqr );
            void        UpdateHasAllies         ();
            float         GetTimeSinceLastCombat  ();

    // Weapon and inventory related functions           
            bool       SelectWeapon            ();
            void        SelectGrenade           ();
            void        CheckShooting           ();
            void        AllowShootingNow        ();
            bool       HasFullClip             ();
            void        CreateWeaponObject      ();
            void        InitInventory           ();
    inline  bool       GetFriendlyBlocksTarget ();
            bool       WeaponReady             ();
            inven_item  GetWeaponItem           ()    { return m_WeaponItem; }

        // grenade functions
    inline  xtick       GetLastGrenadeThrowTime ();
            bool       HasClearGrenadeThrow    ( bool useTestLocation = false, const Vector3& testLocation = Vector3(0.0f,0.0f,0.0f) ); //quick determination if character has correct arc to
            bool       HasClearFragThrow       ( bool useTestLocation = false, const Vector3& testLocation = Vector3(0.0f,0.0f,0.0f) ); //quick determination if character has correct arc to
            bool       HasClearGravThrow       ( bool useTestLocation = false, const Vector3& testLocation = Vector3(0.0f,0.0f,0.0f) ); //quick determination if character has correct arc to
            bool       DetermineGrenadeTrajectory( bool useTestLocation = false, const Vector3& testLocation = Vector3(0.0f,0.0f,0.0f) );    // Creates the trajectory of the grenade
            bool       DetermineGrenadeStraightLine( bool useTestLocation = false, const Vector3& testLocation = Vector3(0.0f,0.0f,0.0f) );    // Creates the trajectory of the grenade
            bool       IsTrajectoryValid       ();      // Checks the path of the grenade to make sure that there are no collisions.
            void        ThrowGrenade            ( const Vector3& grenadePosition );
            void        ThrowSecondaryGrenade   ( const Vector3& grenadePosition );

            bool       RequestAutofire         ();       // help me!
            bool       ProvideAutofire         ();
            void        AutofireRequestOver     ();
            void        CancelAutofire          ();

        // firing functions
            void        UpdateFiring            ();        
    inline  void        SetWantsToFirePrimary   ( bool wantsToFirePrimary );
    inline  void        SetWantsToFireSecondary ( bool wantsToFireSecondary );
    inline  bool       GetWantsToFirePrimary   ();
    inline  bool       GetWantsToFireSecondary ();
            void        FireWeapon              ( bool firingPrimary = true );
            void        CinemaFireWeapon        ( );
            Vector3     CalculateShotDestination( bool& isHit, int accuracyModifier = 100 );
            Vector3     GetHitDestination       ();
            Vector3     GetMissDestination      ();
            bool       IsPlayingIdleFiring     ();

            void        PlayPrimaryFiringAnimation  ();
            void        PlaySecondaryFiringAnimation();
            void        EmitMeleePain               ( const pain_event& PainEvent );
            void        GetMeleeInfo                ( Vector3& StartPos, Vector3& EndPos, float& MeleeSphereRadius );
            void        InitMeleeValues         ();

        // reload functions
            void        UpdateReloading         ();
    inline  void        ReloadWeapon            ();
    inline  bool       IsReloading             ();
            bool       IsFiring                ();
            void        PlayReloadAnimation     ();
    inline  float         GetShortMeleeRange      ();
    inline  float         GetLongMeleeRange       ();
    inline  float         GetTimeSinceLastReload  ();
            
        // aiming functions
    inline  void        SetWantsToAim           ( bool wantsToAim );
    inline  bool       GetWantsToAim           ();
            Vector3     GetShootAtPosition      ();                   // what are we shooting at?

    // Cover Functions
//            cover_node* GetAndReserveNearestCoverNode() ;
            guid        FindNearestValidCover   ( eCoverPreference coverPref, bool checkCurrent = false );
    inline  guid        GetStickyCoverNode      ();
    inline  void        SetStickCoverNode       ( guid newStickyNode );
    inline  guid        GetCurrentCover         ();
    inline  cover_node* GetCurrentCoverNodePtr  ();
    inline  bool       GetCoverChanged         ();
    inline  bool       GetIsCoverHopper        ();
            anim_group::handle GetCoverAnimGroupHandle ();
            ng_node2&   GetClosestNode          ();
            void        SetCurrentCover         ( guid CoverNode );
            void        UpdateCoverNode         ();
    inline  bool       GetCoverIsValid         ();
            bool       GetIsInCover            ();
            bool       GetIsCoverSticky        ( guid coverGuid );
    virtual bool       CoverRetreatWhenDamaged ()    { return true; }            

    // Alarm Functions
            guid        FindNearestAlarm        ();
    inline  guid        GetCurrentAlarm         ();
            void        UpdateAlarmNode         ();
            bool       IsAlarmNodeValid        ( guid testNodeGuid );

    // Turret Functions
    inline  guid        GetStickyTurret         ();
    
    // Goal functions
            bool       UpdateGoal              ( float DeltaTime );
            void        ChangeGoal              ();
    inline  bool       GetGoalCompleted        ();
    inline  bool       GetGoalSucceeded        ();
    inline  goal_info   GetCurrentGoalInfo      ();
            const char* GetGoalFailedReasonName () const;
    inline eGoalFailureReasons GetGoalFailedReason () const;
            bool       DoingMovementGoal       (void );

            void        ClearGoalData           ();
            void        SetIdleGoal             ();
            void        SetTurnToTargetGoal     ( guid target, const Vector3& offset = Vector3(0.0f,0.0f,0.0f), radian sightFOV = 0.0f, bool turnExact = false );
            void        SetTurnToLocationGoal   ( const Vector3& location, radian sightFOV = 0.0f, bool turnExact = false );
            void        SetLookAtTargetGoal     ( guid target, const Vector3& offset = Vector3(0.0f,0.0f,0.0f), float distance = 0.0f, radian sightFOV = 0.0f, bool lookatHead = false );
            void        SetLookAtLocationGoal   ( const Vector3& location, float distance = 0.0f, radian sightFOV = 0.0f );
            void        SetGotoTargetGoal       ( guid target, const Vector3& offset = Vector3(0.0f,0.0f,0.0f), loco::move_style moveStyle = loco::MOVE_STYLE_NULL, float arriveDist = 25.0f, bool gotoExact = false );
            void        SetGotoLocationGoal     ( const Vector3& location, loco::move_style moveStyle = loco::MOVE_STYLE_NULL, float arriveDist = 25.0f, bool gotoExact = false );
            void        SetRetreatFromTargetGoal( guid target, const Vector3& offset = Vector3(0.0f,0.0f,0.0f), loco::move_style moveStyle = loco::MOVE_STYLE_NULL, float minDistance = 100.0f, bool faceTargetOnRetreat = true );
            void        SetRetreatFromLocationGoal( const Vector3& location, loco::move_style moveStyle = loco::MOVE_STYLE_NULL, float minDistance = 100.0f, bool faceTargetOnRetreat = true );
            void        SetPlayAnimationGoal    ( loco::anim_type AnimType, float BlendTime = DEFAULT_BLEND_TIME, uint32_t AnimFlags = 0, float PlayTime = 0.0f );
            void        SetScaledPlayAnimationGoal( loco::anim_type newAnim, float BlendTime = DEFAULT_BLEND_TIME, uint32_t AnimFlags = 0, const Vector3& location = Vector3(0.0f,0.0f,0.0f) );
            void        SetPlayAnimationGoal    ( const char* animName , const char* animPkg = NULL, float BlendTime = DEFAULT_BLEND_TIME, uint32_t AnimFlags = 0, float PlayTime = 0.0f );
            void        SetScaledPlayAnimationGoal( const char* animName , const char* animPkg = NULL, float BlendTime = DEFAULT_BLEND_TIME, uint32_t AnimFlags = 0, float PlayTime = 0.0f, const Vector3& location = Vector3(0.0f,0.0f,0.0f) );
            void        SetSayDialogGoal        ( const char* dialogName, const char* animName = NULL, const char* animPkg = NULL, float BlendTime = DEFAULT_BLEND_TIME, uint32_t Flags = 0, uint8_t DialogFlags = 0, bool BlockOnDialog = true, bool KillAnim = true, float BlendOutTime = 0.25f );
            void        UpdateGoalLocation      ( const Vector3& location );
            Vector3     GetGoalTrueLocation     ();
            Vector3     GetGoalTargetsLocation  ();
inline virtual bool       GetLogStateChanges   ();
            const char* GetGoalTypeName         ();

            void        SetAnimRate             ( float animRate );
            float         GetAnimRate             ();
            void        SetAnimYawDelta         ( radian yawDelta )     { m_AnimYawDelta = yawDelta; }

    // State functions
    virtual void        AdvanceState            ( float DeltaTime );
    virtual bool       SetupState              ( character_state::states State, void* pContext = NULL );
            bool       ChangeState             ();
            void        SetMinStateTime         ( float minStateTime )            { m_MinStateTime = minStateTime; }
    virtual void        SetRecoverState         ( character_state::states NewState ) ;        
            character_state::states  GetActiveState();
            bool       HasState                ( character_state::states stateType );
            character_state *GetStateByType     ( character_state::states stateType );
    inline  character_state::states GetLastState();
            character_state::states GetCurrentStateType();
            bool       IgnoreAttacks           ();
inline virtual bool    AlertDetected           ( float ThreatLevel, const Vector3& ThreatPos );
    virtual void        SetYawFacingTarget      ( radian TargetYaw, radian MaxDeltaYaw );
    virtual radian      GetCurrentFacingYaw     ();
            bool       IsMeleeingPlayer        ();

            void        SetProjectileAttached   ()    { m_bProjectileAttached = true; }
            bool       GetProjectileAttached   ()    { return m_bProjectileAttached; }

    // Loco functions
        //animations
            float         GetLipSyncEventStartFrame( const char* pAnimGroup, const char* pName );
    virtual bool       PlayAnimation           ( const char* pAnimGroup, const char* pName, float BlendTime = DEFAULT_BLEND_TIME, uint32_t Flags = 0, float PlayTime = 0.0f ) ;
    virtual void        StopAnimation           ();
    virtual bool       IsAnimAtEnd             ();
inline virtual void     ControlUpperBody        ( bool bControl, float BlendTime = DEFAULT_BLEND_TIME );
        // lookat
    inline  eLookatModes GetLookatMode          ();
    inline  void        SetHeadLookat           ( guid lookatGuid, float lookatTime = -1.0f , float lookatDistance = 400.0f, radian lookatAngle = R_60 );
    inline  void        SetOverrideLookatMode   ( eLookatModes newMode );
    inline  void        SetOverrideLookatInterest( guid newInterest );
    inline  bool       GetRootWhenIdle         ();


    // Misc data functions
            void        TurnAgainstPlayer       ();
            const char* GetLookatStateName      ();
    inline  Vector3     GetInitialPosition      ();
    inline  void        SetInitialPosition      ( const Vector3& newPosition);
    inline  guid        GetFollowTargetGuid     ();
    virtual void        OverridePhysics         ( bool bOverrride );
    inline  bool       GetAutoRagdoll          ();
            void        SetNotifyOnDeathGuid    ( guid NotifyOnDeath );
    inline  bool       GetBodyFades            ();
    inline  void        SetCrazyFire            ( bool crazyFire );

    inline  bool       GetDoRunLogic           ();
    inline  bool       GetStartStunAnim        ();
    inline  void        StunAnimStarted         ();
    inline  float         GetStunTimer            ();
            void        EnableStun              ( float Duration );

    // scanner stuff
            eNPCName    NameToEnum              ( const char* pName );
            xstring     GetEnumStringNPCs       ();
            const char* EnumToName              ( eNPCName NPC );
            const char* GetScanIdentifier       ();
            bool       WasRecentlyInCombat     ();
            eNPCName    GetIDFromSubtype        ();

    virtual void        NotifyScanBegin         ();
    virtual void        NotifyScanEnd           ();


        
protected:
    
            void        SetAnimPackage          ( const char* animPackageName );
    // naviagation functions
            bool       CanPathTo               ( const Vector3& vDestination );
            void        UpdateHasClearJumpAttack();
            void        UpdateHasClearLOS       ();
            bool       CheckClearLOS( eOffsetPos offset );
            bool       UpdateCanMoveLeft       ( float distance = 200.0f, bool doCollisionCheck = true );
            bool       UpdateCanMoveRight      ( float distance = 200.0f, bool doCollisionCheck = true );
            bool       UpdateCanMoveBack       ( float distance = 200.0f, bool doCollisionCheck = true );
            bool       UpdateCanMoveForward    ( float distance = 200.0f, bool doCollisionCheck = true );
            void        UpdateLookahead         ();
            void        SwitchLookatMode        ();
            void        UpdateInNavMap          (); 
            void        UpdatePathing           ();
            void        UpdateIsStuck           ( float DeltaTime );
            void        UpdateEscape            ();
            void        UpdateLocoMoveStyle     ();
            bool       UpdateEscapeAttempt     ( bool& success );
            void        AttemptToAvoidActorCollision( float DeltaTime );
            void        ChangeEscapeAttempt     ();
    virtual void        InitPathingHints        ();
            bool       GetNewPath              ( const Vector3& vDestination );
            // jumping
    virtual void        CalculateJumpInfo       ();
    virtual void        HandleJumpLogic         ( float DeltaTime );

            void        AdvanceAIState          ( float DeltaTime );
    virtual void        AdvanceLoco             ( float DeltaTime );
            void        UpdateAllowMotion       ();

            void        UpdateVoice             ( float DeltaTime);
            bool       ListenForSounds         ();
            void        ListenForAlerts         ();
            void        UpdateAwarenessLevel    ();                    

            bool       IsCoverNodeValid        ( guid testNodeGuid );
            // Returns true if the character can see another object
            bool       CanSeeObject            ( guid Guid, bool bIgnoreSightCone = false );
            bool       UpdateCanShootAtTarget  ();
            nav_connection_slot_id GetNewRetreatPath( const Vector3& vRetreatFrom, float DistToAchieve );
            bool       SetupRetreatInsideConnection( const Vector3& vRetreatFrom );    // Called by GetNewRetreatPath
            void        UpdateLookAt            ();
            void        UpdateMoveTo            ( float DeltaTime );
            void        UpdateCurrentConnection ();
            void        UpdateAimToTarget       ();
inline virtual bool    SetupShoulderLight      ();
inline virtual void     UpdateFellFromAltitude  ();
inline virtual void     TakeFallPain            ();
       virtual bool    IgnorePain              ( const pain& Pain );
       virtual bool    IgnoreFlinches          ();
       virtual bool    IgnoreFullBodyFlinches  ();
       virtual actor::eHitType OverrideFlinchType( actor::eHitType hitType );


//=========================================================================
// Data
//=========================================================================

protected:
    
    // update target helper functions
        void        UpdateTargetData        ();
        void        ChoseBestTarget         ();
        int         GetHitChance            ();
        void        CalculateSplinePoints   ();
        void        UpdateSpline            ();
virtual bool       UseSplines              () { return true; }

    //===========================================================================
    // Here are the states that belong to all characters
    //===========================================================================
        character_state*        m_pStateList ;          // List of connected states
        character_state*        m_pActiveState ;        // Currently active state
        character_trigger_state m_TriggerState;
        character_hold_state	m_HoldState;
        character_follow_state	m_FollowState;    
        character_trigger_state::TriggerData m_TriggerStateData;       

    // Navigation data and collision data
        pathing_hints       m_PathingHints;
        path_find_struct    m_PathFindStruct;
        eAvoidCharacterAttempt m_AvoidActorAttempt;
        Vector3             m_SplinePointList[k_MaxSplinePoints];
        Vector3             m_InitialPosition;
        Vector3             m_DesiredLocation;              // this is where we want to go (it's our loco_move to when we are moving)
        Vector3             m_NextPathPoint;
        Vector3             m_PreviousPathPoint;
        Vector3             m_JumpStartPos;
        Vector3             m_JumpLandPos;
        Vector3             m_JumpApexPos;
        Vector3             m_LastMovingPosition;
        Vector3             m_UnstuckVector;
        guid                m_CollisionIgnoreGuid;
        guid                m_LeashGuid;    
        float                 m_AvoidAttemptTime;
        float                 m_TimeSinceLastRetreatJig;                                            
        float                 m_RethinkRetreatTimer;
        float                 m_JumpVelocity;
        float                 m_JumpTimeRemaining;
        float                 m_TotalJumpTime;
        float                 m_JumpTimeToApex;
        float                 m_SinceLastGetPath;
        float                 m_LeashDistance;
        float                 m_TimeInEscapeAttempt;
        float                 m_TimeStuck;
        float                 m_TimeOutofNavMap;
        float                 m_PreviousActorCollisionRadius;
        float                 m_ModifiedActorCollisionRadius;
        float                 m_CloakDecloakTimer;
        int                 m_CurrentPathStructIndex;
        int                 m_CurrentEscapeAttempt;
        int                 m_EscapeAttempts;
        int                 m_LastEscapeAttempt;
        int                 m_PointsPerSpline;
        int                 m_CurrentSplinePoint;
        int                 m_Subtype;
        uint32_t                 m_NeedsToRecalcPath:1,            // should we recalculate our path?
                            m_CurrentConnectionSlotChanged:1,
                            m_bFollowingPath:1,
                            m_PathReset:1,
                            m_InNavMap:1,
                            m_EscapeDoneMovingBack:1,
                            m_IsStuck:1,
                            m_CanMoveForward:1,
                            m_CanMoveLeft:1,
                            m_CanMoveRight:1,
                            m_CanMoveBack:1,
                            m_bHasClearJumpAttack:1,
                            m_bHasClearLOSFeet:1,
                            m_bHasClearLOSEyes:1,
                            m_bCanReachPathDest:1,
                            m_bCrazyFire:1,
                            m_bForceWeaponFire:1,
                            m_bCanReload:1,
                            m_bNeverPlayReloadAnim:1,
                            m_bWantsToDropWeapon:1,
                            m_bCanAlwaysBackpeddle:1;

        
static  enum_table<eDialogActors>m_DialogActorsEnum;        // Enumeration of the dialog actors..
        nav_connection_slot_id  m_GoalsConnectionSlot;      // what is the nearest connection slot to our current goals location?
        nav_connection_slot_id  m_CurrentConnectionSlot;    // what is the nearest connection slot to us?
        nav_connection_slot_id  m_GoalRetreatToConnectionSlot; // what slot are we attempting to retreat to?

    // Target, Awareness, and Sound/Sight related data
        guid                m_AllyGuids[k_MaxAllies];
        dialog_info         m_CurrentDialogInfo;
        object_affecter     m_OverrideTargetAffector; //affector for the override target.
        Vector3             m_LastLocationOfInterest;
        Vector3             m_LastKnownLocationOfTarget;
        Vector3             m_LastSeenLocationOfTarget;
        Vector3             m_LastSoundPos;         // posistion of last threatining sound.
        guid                m_TargetGuid;           // Object of interest
        guid                m_OverrideTargetGuid;   // We MUST target this guid as long as it exists.
        guid                m_AimAtGuid;            // if no targets and we get a fire event, we'll aim it at this.
        xtick               m_LastListenTime;       // The last time the character listened for sound.
        xtick               m_LastAlertTime;        // The last time the character listened for alerts.
        xtick               m_LastAlertSentTime;    // Last time we sent out an alert;
        radian              m_IdleSightFOV ;        // Sight field of view
        radian              m_AlertSightFOV ;       // Sight field of view
        float                 m_LightSightRadius ;    // Distance that can be seen
        float                 m_DarkSightRadius ;     // Distance that can be seen
        float                 m_TargetSightedTimer;
        float                 m_TimeAtAwarenessLevel;
        float                 m_TimeSinceLastSound;   // time since we last heard a sound.
        float                 m_SoundRange;           // How far the characters can listen;
        float                 m_TimeSinceLastDialog;  // how long since we last played a dialog?
        float                 m_TargetNotSeenTimer;   // Time target has not been seen
        float                 m_NoTargetTimer;
        float                 m_TimeSinceLastPathToTargetCheck; // how long since we tried to update to see if we can path to target?
        float                 m_TimeSinceLastTargetUpdate;
        float                 m_TimeWaitingForVoiceToStart;
        float                 m_TimeSincePlayerDamagedMeLast;
        int                 m_TimesPlayerHasDamagedMe;
        eDialogActors       m_OverrridePreferedVoiceActor;   // -1 to 3 the voice that says our dialog a,b,c,d, -1 means leave it random.
        eAwarenessLevel     m_AwarenessLevel;
        eDialogType         m_CurrentDialogType;
        uint32_t                 m_SoundHeard:1,             // did we hear a sound?
                            m_VoiceStarted:1,           // has the voice started?
                            m_AllowDialog:1,            // does this NPC allow dialog to be played?
                            m_CanSeeTarget:1,           // Object of interest
                            m_CanShootAtTarget:1,       // do we have a straight LOS to target?
                            m_FriendlyBlocksTarget:1,
                            m_CanReachTarget:1,         // can we reach our current target?
                            m_CanReachGoalTarget:1,     // can we reach the target of our goal?
                            m_CanPathToTarget:1,        // can we path to our current target?
                            m_bTargetPlayer :1,         // Is the character targeting a player.        
                            m_CombatReady:1,            // Is this NPC ready/expecting combat?
                            m_bNeverCombatReady:1,      // If true we never become combat ready.
                            m_HasAllies:1,              // do we have friends nearby?
                            m_bTargetSeen:1,            // have we ever seen our current target?
                            m_bAllyAcquiredTarget:1,
                            m_bPlayAllClearDialog:1;

    // NPC Name
        eNPCName            m_NPCName;                  // what index for the NPCs name is it?
        int                 m_ScannerVOIndex;           // keep track of the last VO said so we can walk the list.

    // Weapon and Pain data
        inven_item          m_WeaponItem;               // The weapon this character is carrying
        inven_item          m_GrenadeItem;              // The grenade this character is carrying
        Vector3             m_GrenadeDestination ;                  // Where the grenade will land
        Vector3             m_GrenadeThrowStart;
        xtick               m_LastGrenadeThrowTime; // last time we threw a grenade.
        guid                m_NotifyOnDeath;        // Guid of an object to notify when this object is killed.
        radian              m_WeaponFireAngle;
        radian3             m_GrenadeTrajectory ;                   // Trajectory of the grenade
        float                 m_SinceLastShot;        // time since last shot.
        float                 m_ShootDelay;           // min time between shots.
        float                 m_ShortMeleeRange;      // range of our short melee attack 
        float                 m_LongMeleeRange;       // range of our long melee attack
        float                 m_TimeSinceLastReload;  // how long since we reloaded last?
        float                 m_WeaponDropPercent;    // chance we will drop our weapon
        float                 m_AmmoDropPercent;      // chance we will drop an ammo clip
        float                 m_GrenadeDropPercent;   // chance we will drop a grenade
        float                 m_InventoryDropPercent; // chance we will drop other inventory
        int                 m_Accuracy;             // how accurate are we?
        int                 m_MovingTargetAccuracy; // how accurate when our target is moving?
        float                 m_AccuracyDifficultyScaler; // accuracy tweak due to difficulty
        bool               m_bPassOnGuidToCorpse;
        uint32_t                 m_Reloading:1,
                            m_WantsToAim:1,
                            m_WantsToFirePrimary:1,
                            m_WantsToFireSecondary:1,
                            m_NextShotMustHit:1,// if true then the next fired shot WILL be a hit.
                            m_bIgnorePain:1,
                            m_bIgnoreFlinches:1,
                            m_bAutoRagdoll:1,
                            m_MeleeInited:1,
                            m_bProjectileAttached:1,
                            m_bDropWeapons:1,
                            m_bDropAmmo:1,
                            m_bDropGrenades:1,
                            m_bDropInventory:1;


    // State, Goal, and loco data
        goal_info           m_GoalInfo;             // our current goal
        eGoalFailureReasons m_FailedReason;         // why did we fail? 
        char                m_WeaponlessAnimPackage[64]; // anim package to switch to when we lose our gun.
        float                 m_TimeInGoal;           // how long have we been processing the current goal...
        float                 m_StateTime;            // Time in current state
        float                 m_MinFallHeight;        // min height till we start taking damage
        float                 m_MaxFallHeight;        // min height till we start taking damage
        float                 m_MinStateTime;         // min time we must remain in our current state.
        radian              m_AimToTargetYaw;       // Difference between our aim and totarget angle
        radian              m_AnimYawDelta;         // this is feed as the yaw delta into the next animation we play.          
        character_state::states m_InitialState;     // Startup state of character
        character_state::states m_LastState;
        character_state::states m_DesiredState;     // the state we wish to change to.
        character_state::states m_PostLipsyncState; // go to this state when you finish your cinema lipsync.
        void *              m_StateContext;         // the conetext for this new state;
        uint32_t                 m_IgnoreLocoInTransform:1, // annoying bool so we ignore the loco on transform in the editor.
                            m_LocoFallingOverride:1,
                            m_NewGoal:1,            // do we have a new goal
                            m_GoalCompleted:1,      // have we completed our current goal?
                            m_GoalSucceeded:1,      // were we sucessful in completing our goal?
                            m_LogStateChanges:1,
                            m_bWasPlayingFullBodyLipSync:1;    // should we log AI state and phase changes?

    // Misc data 
        bbox                m_BBox;                 // Rendering BBox
        object_affecter     m_FollowTargetAffector;
        guid                m_PendingTaskListGuid;         // the guid of our task list.       
//        guid                m_TriggerGuid;          // the guid of our triggerEX.       
        guid                m_FollowTargetGuid;
        guid                m_CurrentAlarm;         // The cover we are currently using.
        guid                m_StickyCoverNode;      // If this is set, then we will stay in cover state at this node.
        guid                m_CurrentCover;         // The cover we are currently using.
        guid                m_StickyTurret;         // If this is set, then we will use this turret.
        guid                m_ActivateOnDeath;
        guid                m_OverrideLookatInterest; // target we were told to lookat as an override. 
        guid                m_pNoBlockDialogTrigger;
        guid                m_HeadLookat;           // object for our head to lookat
        float                 m_HeadLookatTimer;      // we will look at the object this long, negative means never turn off.
        float                 m_OutofLookatTimer;     // this gets incremented when we are outside the lookat area... 
        float                 m_HeadLookatDistance;   // distance we must be within in order to look at the thinghy. 
        radian              m_HeadLookatAngle;      // max angle we will look around to see the object. 

//        anim_group::handle  m_CoverAnimGroupHandle; // the animgrouphandle for our current cover.
        float                 m_TimeActive;
        float                 m_TimeSinceLastCoverCheck;
        float                 m_DamageInCover;
        float                 m_TimeSinceLastLookatSwitch;
        float                 m_MaxDistToCover;
        float                 m_LipSyncStartAnimFrame;
        float                 m_StunTimer;
        int                 m_DialogPrefixString;
        int                 m_LogicalName;
        int                 m_GroundMaterial ;      // Material under feet
        eLookatModes        m_LastLookatMode;           // Our current lookat mode.
        eLookatModes        m_LookatMode;           // Our current lookat mode.
        eLookatModes        m_OverrideLookatMode;   // an override mode provided by the AI.
        uint32_t                 m_bThinking:1,          // true if character is thinking
                            m_CoverChanged:1,         // set to true for one tick when our sticky cover node changes.
                            m_CoverIsValid:1,         // true if cover is valid.            
                            m_IgnoreInterest:1,       // we will ignore interest objects.
                            m_BodiesFade:1,
                            m_bCanUseAlarms:1,
                            m_bRootToPositionWhenIdle:1,
                            m_bOnlyUsesCoverAhead:1,
                            m_bCoverHopper:1,
                            m_bDoRunLogic:1,
                            m_bDoRender:1,
                            m_bIsInHeadTracking:1;
};

//=========================================================================

inline bool character_hold_state::IgnorePain( const pain& Pain ) 
{ 
    return true; 
}

//=========================================================================

inline bool character_hold_state::IgnoreAlerts() 
{ 
    return true; 
}    

//=========================================================================

inline bool character_hold_state::IgnoreSight()
{ 
    return false; 
}

//=========================================================================

inline bool character_hold_state::IgnoreSound()
{ 
    return false; 
}    

//=========================================================================

inline bool character_hold_state::IgnoreAttacks()
{ 
    return false; 
}    

//=========================================================================

inline bool character::IsCharacter()               
{ 
    return true; 
}

//=========================================================================

inline  nav_connection_slot_id character::GetCurrentConnection()
{ 
    return m_CurrentConnectionSlot; 
}

//=========================================================================

inline  bool character::GetCanMoveLeft( )
{
    return m_CanMoveLeft;
}

//=========================================================================

inline  bool character::GetCanMoveRight( )
{
    return m_CanMoveRight;
}

//=========================================================================

inline  bool character::GetCanMoveBack( )
{
    return m_CanMoveBack;
}

//=========================================================================

inline  bool character::GetCanMoveForward( )
{
    return m_CanMoveForward;
}

//===========================================================================

inline bool character::GetIsFollowingPath()
{
    return m_bFollowingPath;
}

//===========================================================================

inline void character::SetIsFollowingPath( bool followingPath )
{
    m_bFollowingPath = followingPath;
}

//=========================================================================

inline void character::ClearInitialState()
{ 
    m_InitialState = character_state::STATE_NULL; 
}

//=========================================================================

inline character_trigger_state::TriggerData character::GetTriggerStateData()
{ 
    return m_TriggerStateData; 
}

//=============================================================================

inline void character::SetPendingTaskListGuid( guid newGuid )
{
    m_PendingTaskListGuid = newGuid;
}

//=========================================================================

inline int character::GetVoiceID()
{ 
    return m_VoiceID; 
}

//=========================================================================

inline void character::SetVoiceID( int ID )      
{ 
    m_VoiceID = ID; 
}

//=========================================================================

inline bool character::GetHasClearJumpAttack()
{
    return m_bHasClearJumpAttack;
}

//=========================================================================

inline bool character::GetHasClearLOSFeet()
{
    return m_bHasClearLOSFeet;
}

//=========================================================================

inline bool character::GetHasClearLOSEyes()
{
    return m_bHasClearLOSEyes;
}

//=========================================================================

inline float character::GetTargetNotSeenTimer()        
{ 
    return m_TargetNotSeenTimer; 
}

//=========================================================================

inline Vector3 character::GetLastKnownLocationOfTarget()   
{ 
    return m_LastKnownLocationOfTarget; 
}

//=========================================================================

inline void character::SetLastKnownLocationOfTarget( const Vector3& Location )   
{ 
    m_LastKnownLocationOfTarget = Location; 
}

//=========================================================================

inline Vector3 character::GetLastSeenLocationOfTarget()    
{ 
    return m_LastSeenLocationOfTarget;  
}

//=========================================================================

inline float character::GetSightRadius()        
{ 
    return m_LightSightRadius; 
}

//=========================================================================

inline Vector3 character::GetLastLocationOfInterest()       
{ 
    return m_LastLocationOfInterest; 
}

//=========================================================================

inline void character::SetLastLocationOfInterest( const Vector3& Location ) 
{ 
    m_LastLocationOfInterest = Location; 
}

//=========================================================================

inline character::eAwarenessLevel character::GetAwarenessLevel( ) 
{ 
    return m_AwarenessLevel;
}

//=========================================================================

inline Vector3 character::GetLastSoundPosition()        
{ 
    return m_LastSoundPos;        
}

//=========================================================================

inline float character::GetTimeSinceLastSound()        
{ 
    return m_TimeSinceLastSound;  
}

//=========================================================================

inline void character::ClearSoundHeard()            
{ 
    m_SoundHeard = false; 
}   

//=========================================================================

inline bool character::GetSoundHeard()            
{ 
    return m_SoundHeard; 
}           

//=========================================================================

inline void character::Listen( conversation_packet& rPacket )
{ 
    (void)rPacket ; 
}

//=========================================================================

inline
void character::SetNotifyOnDeathGuid( guid NotifyOnDeath )
{
    m_NotifyOnDeath = NotifyOnDeath;
}

//=========================================================================

inline guid character::GetTargetGuid()            
{ 
    return m_TargetGuid; 
}

//=========================================================================

inline bool character::CanSeeTarget()        
{ 
    return m_CanSeeTarget; 
}           

//=========================================================================

inline bool character::CanShootAtTarget()        
{ 
    return m_CanShootAtTarget; 
}           

//=========================================================================

inline radian character::GetAimToTargetYaw()        
{ 
    return m_AimToTargetYaw; 
}                            

//=========================================================================

inline bool character::CanReachTarget()
{ 
    return m_CanReachTarget; 
}            

//=========================================================================

inline bool character::GetCanReachGoalTarget()
{ 
    return m_CanReachGoalTarget; 
}            

//=========================================================================

inline bool character::CanPathToTarget()
{ 
    return m_CanPathToTarget; 
}            

//=========================================================================

inline xtick character::GetLastGrenadeThrowTime()                    
{ 
    return m_LastGrenadeThrowTime; 
}            

//=========================================================================

inline void character::SetWantsToFirePrimary( bool wantsToFirePrimary )    
{ 
    m_WantsToFirePrimary = wantsToFirePrimary;    
}

//=========================================================================

inline void character::SetWantsToFireSecondary( bool wantsToFireSecondary )  
{ 
    m_WantsToFireSecondary = wantsToFireSecondary;
}

//=========================================================================

inline bool character::GetWantsToFirePrimary()                    
{ 
    return m_WantsToFirePrimary;  
}

//=========================================================================

inline bool character::GetWantsToFireSecondary()                    
{ 
    return m_WantsToFireSecondary;
}

//=========================================================================

inline void character::ReloadWeapon()                    
{ 
    new_weapon* pWeapon = GetCurrentWeaponPtr();
    if( pWeapon )
    {    
        // do we play the reload anim? if not just do it now.
        if( m_bNeverPlayReloadAnim || 
            IsPlayingFullBodyCinema() )
        {        
            pWeapon->Reload( new_weapon::AMMO_PRIMARY );
            m_Reloading = false;
            m_TimeSinceLastReload = 0.0f;
            AutofireRequestOver();
        }
        else
        {        
            m_Reloading = true; 
            PlayReloadAnimation();
            if( GetAwarenessLevel() >= AWARENESS_TARGET_SPOTTED &&
                HasAllies() )
            {        
                if( RequestAutofire() )
                {            
                    PlayDialog(DIALOG_RELOAD);
                }
            }
        }
    }
}

//=========================================================================

inline float character::GetTimeSinceLastReload()                    
{ 
    return m_TimeSinceLastReload;
}

//=========================================================================

inline bool character::IsReloading()                    
{ 
    return m_Reloading;
}

//=========================================================================

inline float character::GetShortMeleeRange()
{
    return m_ShortMeleeRange;
}

//=========================================================================

inline float character::GetLongMeleeRange()
{
    return m_LongMeleeRange;
}

//=========================================================================

inline void character::SetWantsToAim( bool wantsToAim )        
{ 
    m_WantsToAim = wantsToAim;    
}

//=========================================================================

inline bool character::GetWantsToAim()                    
{ 
    return m_WantsToAim;          
}

//=========================================================================

inline  bool character::GetInNavMap()
{
    return m_InNavMap;
}

//=========================================================================

inline  float character::GetTimeOutofNavMap()
{
    return m_TimeOutofNavMap;
}

//=========================================================================

inline  void character::SetCollisionIgnoreGuid( guid ignoreGuid )
{
    m_CollisionIgnoreGuid = ignoreGuid;
}

//=========================================================================

inline bool character::GetCoverIsValid()
{ 
    return m_CoverIsValid; 
}

//=========================================================================

inline guid character::GetStickyCoverNode()
{ 
    return m_StickyCoverNode; 
}

//=========================================================================

inline bool character::GetCoverChanged()
{ 
    return m_CoverChanged; 
}

//=========================================================================

inline bool character::GetIsCoverHopper()
{ 
    return m_bCoverHopper; 
}

//=========================================================================

/*inline anim_group::handle character::GetCoverAnimGroupHandle()
{
    return m_CoverAnimGroupHandle;
}*/

//=========================================================================

inline guid character::GetCurrentCover()
{ 
    return m_CurrentCover; 
}

//=========================================================================

inline
cover_node* character::GetCurrentCoverNodePtr()
{
    cover_node* pCoverNode = (cover_node*)g_ObjMgr.GetObjectByGuid( m_CurrentCover );
    return pCoverNode;
}

//=========================================================================

inline guid character::GetCurrentAlarm()
{ 
    return m_CurrentAlarm; 
}

//=========================================================================

inline guid character::GetStickyTurret()
{ 
    return m_StickyTurret; 
}

//=========================================================================

inline void character::SetStickCoverNode( guid newStickyNode )  
{ 
    if( m_StickyCoverNode != newStickyNode )
    {    
        m_StickyCoverNode = newStickyNode; 
    }
}            

//=========================================================================

inline bool character::GetGoalCompleted()                    
{ 
    return m_GoalCompleted;   
}    

//=========================================================================

inline bool character::GetGoalSucceeded()                    
{ 
    return m_GoalSucceeded;   
}    

//=========================================================================

inline character::goal_info character::GetCurrentGoalInfo()                    
{ 
    return m_GoalInfo;   
}    

//=========================================================================

inline bool character::GetLogStateChanges()    
{ 
    return m_LogStateChanges; 
}

//=========================================================================

inline bool character::AlertDetected( float ThreatLevel, const Vector3& ThreatPos ) 
{ 
    return false; 
}

//=========================================================================

inline void character::ControlUpperBody( bool bControl, float BlendTime ) 
{ 
}

//=========================================================================

inline character::eLookatModes character::GetLookatMode() 
{ 
    return m_LookatMode; 
}

//=========================================================================

inline void character::SetOverrideLookatMode( eLookatModes newMode )    
{ 
    m_OverrideLookatMode = newMode; 
}

//=========================================================================

inline void character::SetHeadLookat( guid lookatGuid, float lookatTime, float lookatDistance , radian lookatAngle )
{
    m_HeadLookat = lookatGuid;
    m_HeadLookatTimer = lookatTime;
    m_HeadLookatDistance = lookatDistance;
    m_HeadLookatAngle = lookatAngle;
}

//=========================================================================

inline void character::SetOverrideLookatInterest( guid newInterest )        
{ 
    m_OverrideLookatInterest = newInterest; 
}            

//=========================================================================

inline bool character::GetRootWhenIdle()
{
    return m_bRootToPositionWhenIdle;
}

//=========================================================================

inline Vector3 character::GetInitialPosition()
{ 
    return m_InitialPosition; 
}

//=========================================================================

inline void character::SetInitialPosition( const Vector3& newPosition)
{ 
    m_InitialPosition = newPosition; 
}

//=========================================================================

inline guid character::GetFollowTargetGuid()    
{ 
    return m_FollowTargetGuid;
}            

//=========================================================================

inline bool character::GetAutoRagdoll()
{ 
    return m_bAutoRagdoll; 
}

//=========================================================================

inline bool character::SetupShoulderLight()    
{ 
    return false; 
}

//=========================================================================

inline void character::UpdateFellFromAltitude()    
{ ; /*Do nothing for now*/ }

//=========================================================================

inline void character::TakeFallPain()    
{ ; /*Do nothing for now*/ }

//=========================================================================

inline  
float character::GetLeashDistance() const
{
    return m_LeashDistance;
}

//=========================================================================

inline 
guid character::GetLeashGuid() const
{
    return m_LeashGuid;
}

//=========================================================================

inline 
bool character::GetFriendlyBlocksTarget()
{
    return m_FriendlyBlocksTarget;
}

//=========================================================================

inline 
void character::SetCombatReady( bool isReady ) 
{
    m_CombatReady = isReady;
    if( m_bNeverCombatReady )
    {
        m_CombatReady = false;
    }
}

//=========================================================================

inline 
bool character::IsCombatReady() 
{
    return m_CombatReady;
}

//=========================================================================

inline 
bool character::HasAllies() 
{
    return m_HasAllies;
}

//=========================================================================

inline 
character::eGoalFailureReasons character::GetGoalFailedReason () const
{
    return m_FailedReason;
}

//=========================================================================

inline bool character::GetBodyFades( ) 
{ 
    return m_BodiesFade; 
}

//=========================================================================

inline void character::SetCrazyFire( bool crazyFire )
{
    m_bCrazyFire = crazyFire;
}

inline bool character::GetDoRunLogic( )
{
    return m_bDoRunLogic;
}

inline  character_state::states character::GetLastState()
{
    return m_LastState;
}

inline void character::SetIgnorePain( bool bShouldIgnorePain )
{
    m_bIgnorePain = bShouldIgnorePain;
}

//=========================================================================

inline bool character::GetStartStunAnim()
{
    return ( m_StunTimer > 0.0f );
}

//=========================================================================

inline void character::StunAnimStarted()
{
    m_StunTimer = 0.0f;
}

//=========================================================================

inline float character::GetStunTimer()
{
    return m_StunTimer;
}

//=========================================================================

