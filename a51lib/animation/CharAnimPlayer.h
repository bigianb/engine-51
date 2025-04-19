#pragma once

#include "AnimTrack.h"
#include "BasePlayer.h"

//=========================================================================
// ANIM EVENT TYPES
//=========================================================================

// NOTE: - These events MUST match up with the type list in
//         the max script "JV_B52_AnimPrim_event.ms"
enum anim_events
{
    // Define                               MaxPluginEventName
    ANIM_EVENT_NULL = 0, // DON'T NEED ONE!

    ANIM_EVENT_SFX_FOOT_HEEL,     // "SFX_Foot_Heel",
    ANIM_EVENT_SFX_FOOT_TOE,      // "SFX_Foot_Toe",
    ANIM_EVENT_SFX_ATTACK_SWOOSH, // "SFX_AttackSwosh",

    ANIM_EVENT_PAIN_CLAW_FORWARD,      // "PAIN_ClawForward",
    ANIM_EVENT_PAIN_CLAW_LEFT_2_RIGHT, // "PAIN_ClawLeft2Right",
    ANIM_EVENT_PAIN_CLAW_RIGHT_2_LEFT, // "PAIN_ClawRight2Left",

    ANIM_EVENT_PAIN_HOWL, // "PAIN_Howl",

    ANIM_EVENT_PAIN_FIST_FORWARD,      // "PAIN_FistForward",
    ANIM_EVENT_PAIN_FIST_LEFT_2_RIGHT, // "PAIN_FistLeft2Right",
    ANIM_EVENT_PAIN_FIST_RIGHT_2_LEFT, // "PAIN_FistRight2Left",

    ANIM_EVENT_PAIN_FOOT_FORWARD,      // "PAIN_FootForward",
    ANIM_EVENT_PAIN_FOOT_LEFT_2_RIGHT, // "PAIN_FootLeft2Right",
    ANIM_EVENT_PAIN_FOOT_RIGHT_2_LEFT, // "PAIN_FootRight2Left",

    ANIM_EVENT_PAIN_WEAPON_FORWARD,      // "PAIN_WeaponForward",
    ANIM_EVENT_PAIN_WEAPON_LEFT_2_RIGHT, // "PAIN_WeaponLeft2Right",
    ANIM_EVENT_PAIN_WEAPON_RIGHT_2_LEFT, // "PAIN_WeaponRight2Left",

    ANIM_EVENT_GRAB_BEGIN,
    ANIM_EVENT_GRAB_ATTACH,
    ANIM_EVENT_IMPALE_BEGIN,
    ANIM_EVENT_IMPALE_ATTACH,
    ANIM_EVENT_IMPALE_DISMOUNT,
    ANIM_EVENT_SFX_ATTACKGRUNTSHORT, // "SFX_AttackGruntShort"
    ANIM_EVENT_SFX_ATTACKGRUNTLONG,  // SFX_AttackGruntLong"
    ANIM_EVENT_SFX_HOWLRAGE,         // SFX_HowlRage"

    ANIM_EVENT_PRIMARY_FIRE,
    ANIM_EVENT_SECONDARY_FIRE,

    ANIM_EVENT_SFX_WEAPONLOOP,
    ANIM_EVENT_SFX_SWITCH_TO,
    ANIM_EVENT_SFX_SWITCH_FROM,
    ANIM_EVENT_SFX_RELOAD,
    ANIM_EVENT_SFX_ALT_RELOAD,

    ANIM_EVENT_GENERIC_TIMERANGE,

    ANIM_EVENT_GRAB_GRENADE,
    ANIM_EVENT_RELEASE_GRENADE,

    ANIM_EVENT_WPN_PRIMARY_FIRE_LR,
    ANIM_EVENT_WPN_PRIMARY_FIRE_LL,
    ANIM_EVENT_WPN_PRIMARY_FIRE_UR,
    ANIM_EVENT_WPN_PRIMARY_FIRE_UL,
    ANIM_EVENT_WPN_SECONDARY_FIRE_LR,
    ANIM_EVENT_WPN_SECONDARY_FIRE_LL,
    ANIM_EVENT_WPN_SECONDARY_FIRE_UR,
    ANIM_EVENT_WPN_SECONDARY_FIRE_UL,

    ANIM_EVENT_TOTAL
};

//=========================================================================
// CLASS CHAR_ANIM_PLAYER
//=========================================================================

#define MAX_CHAR_ANIM_PLAYER_TRACKS 8

class char_anim_player : public base_player
{

    //-------------------------------------------------------------------------
public:
    char_anim_player();
    virtual ~char_anim_player();

    //
    // Tells the animation system which package of animations to use.
    //
    virtual void SetAnimGroup(const AnimGroup::handle& hGroup);

    //
    // Sets the current Track0 animation.  The Manual bools tell the animation
    // system how much of the root position you are controlling.
    //
    void SetAnim(int iAnim, bool ManualVert, bool ManualHoriz, float BlendTime = DEFAULT_BLEND_TIME, bool ResetFrameCount = false);
    void SetAnimHoriz(int iAnim, float BlendTime = DEFAULT_BLEND_TIME);
    void SetAnimVert(int iAnim, float BlendTime = DEFAULT_BLEND_TIME);
    void SetAnim(const char* pAnimName, bool ManualVert, bool ManualHoriz, float BlendTime = DEFAULT_BLEND_TIME);
    void SetAnimHoriz(const char* pAnimName, float BlendTime = DEFAULT_BLEND_TIME);
    void SetAnimVert(const char* pAnimName, float BlendTime = DEFAULT_BLEND_TIME);

    void SetManualYawControl(bool bIsManual);
    void SetOverrideRootBlend(bool bOverrideRootBlend);

    void SetRemoveTurnYaw(bool bRemoveTurnYaw);
    void SetMirrorBone(int iBone) { m_iMirrorBone = iBone; }

    //
    // The render offset translates the skeleton in addition to the animation.
    // This is purely a render effect.
    //
    void    SetRenderOffset(const Vector3& RenderOffset);
    Vector3 GetRenderOffset();

    //
    // The SlideDelta is divided by the length of the animation and is combined
    // with the animation's delta and returned in the Advance( DeltaPos ).
    // The SlideDelta is always described in worldspace.
    //
    void SetSlideDelta(const Vector3& SlideDelta);

    //
    // The YawDelta is divided by the length of the animation and is combined
    // with the animation's delta and returned in the Advance( paDeltaYaw ).
    //
    void SetYawDelta(const Radian& YawDelta);

    //
    // The basis matrix rotates the universe onto a different plane.
    // The animation system is still controlled by the Yaw to steer the
    // character left and right but the notion of up and forward will have
    // been rotated.
    //
    void     SetBasisMatrix(const Matrix4& BasisM);
    Matrix4& GetBasisMatrix();
    Matrix4& GetPreviousBasisMatrix();

    //
    // This returns the total translation of the animation in world space
    // taking into account the characters scale and orientation.  This is
    // useful to predict the position of the character at the end of the
    // animation.
    //
    Vector3 GetWorldAnimTranslation();

    // Gets the bind posotion of the bone
    Vector3 GetBindPosition(int iBone);

    //
    // Returns TRUE if at last frame of animation or in a cycle > 0
    //
    bool IsAtEnd();

    //
    // Advances the animation and returns the change in position that occurred
    //
    void Advance(float nSeconds, Vector3& DeltaPos, Radian& DeltaYaw);
    void Advance(float nSeconds, Vector3& DeltaPos);

    //
    // Skips directly to a particular frame and cycle of the animation
    //
    void SetFrame(float Frame);
    void SetCycle(int Cycle);

    Vector3 GetPosition() const;
    Radian3 GetRotation() const;

    Radian GetYaw();
    Radian GetPitch();
    Radian GetRoll();
    float  GetScale();

    void SetPosition(const Vector3& Pos);
    void SetYaw(Radian Yaw);
    void SetPitch(Radian Pitch);
    void SetRoll(Radian Roll);
    void SetScale(float Scale);
    void SetRotationAndPosition(const Matrix4& L2W);

    //
    // Returns the cached L2W matrix for that bone
    //
    const Matrix4* GetBoneL2Ws();
    const Matrix4& GetBoneL2W(int iBone);

    //
    // Returns the cached world space bone position
    //
    const Vector3* GetBonePositions();
    const Vector3& GetBonePosition(int iBone);

    //
    // Returns the current L2W for a prop.  If the prop is not present
    // the return value will be false and the matrix undetermined
    //
    bool GetPropL2W(const char* pPropName, Matrix4& L2W);

    float GetFrame();
    int   GetCycle();
    float GetPrevFrame();
    int   GetPrevCycle();

    int   GetNFrames();
    int   GetNBones();
    int   GetNAnims();
    float GetSpeed();
    int   GetBoneIndex(const char* pBoneName);

    float GetFrameParametric();
    void  SetFrameParametric(float Frame);

    int  GetAnimIndex();
    int  GetAnimIndex(const char* pAnimName);
    bool IsPlaying(const char* pAnimName, int iTrack = 0);

    const AnimInfo&   GetAnimInfo();
    const AnimBone&   GetBone(int iBone);
    const AnimGroup&  GetAnimGroup();
    AnimGroup::handle GetAnimGroupHandle();
    bool              HasAnimGroup();

    //
    // Render
    //
    void RenderSkeleton(bool LabelBones = false);

    //
    // Events
    //
    virtual int               GetNEvents();
    virtual const anim_event& GetEvent(int iEvent);
    virtual bool              IsEventActive(int iEvent);
    bool                      IsEventTypeActive(int Type);
    virtual Vector3           GetEventPosition(int iEvent);
    virtual Radian3           GetEventRotation(int iEvent);
    virtual Vector3           GetEventPosition(const anim_event& Event);
    virtual Radian3           GetEventRotation(const anim_event& Event);

    //
    // Track management
    //
    void              SetTrackController(int iTrack, track_controller* pTrackController);
    track_controller* GetTrackController(int iTrack);
    void              ClearTracks();
    void              ClearTrack(int iTrack);

    //-------------------------------------------------------------------------

private:
    void ComputeAnimToWorldM(Matrix4& M);
    void PrepareRootKey(AnimKey& Key);
    void CalcBasisVectors(const Vector3& world, Vector3& i, Vector3& j, Vector3& k);

    void GetInterpKeys(AnimKey* pKey);

    void           DirtyCachedL2W();
    const Matrix4& GetCachedL2W(int iBone);
    const Matrix4* GetCachedL2Ws();
    void           ComputeBonesL2W(const Matrix4& L2W, AnimKey* pKey, Matrix4* pBoneL2W);
    void           UpdateCachedL2W();

    void           DirtyCachedBonePos();
    const Vector3* GetCachedBonePoss();
    const Vector3& GetCachedBonePos(int iBone);
    void           UpdateCachedBonePos();

    //-------------------------------------------------------------------------

private:
    AnimGroup::handle m_hAnimGroup; // Group of anims we are using

    bool m_bManualVert;    // Don't apply Y trans at render time
    bool m_bManualHoriz;   // Don't apply XZ trans at render time
    bool m_bManualYaw;     // Don't apply Yaw at render time
    bool m_bRemoveTurnYaw; // Removes yaw from root node of turn animations
    int  m_iMirrorBone;    // Index of bone to mirror

    Vector3 m_WorldPos;
    Radian3 m_WorldRot;
    float   m_WorldScale;
    Radian  m_AnimHandleYaw; // Handle of current animation

    Vector3 m_RenderOffset;
    Vector3 m_SlideDelta;
    Radian  m_YawDelta;
    float   m_YawStartFrame;
    float   m_YawEndFrame;

    Matrix4 m_BasisM;
    Matrix4 m_PreviousBasisM;

    // Track controller array
    anim_track_controller m_AnimTrack;
    track_controller*     m_Track[MAX_CHAR_ANIM_PLAYER_TRACKS];

    // Cached Current Matrices
    bool     m_CachedL2WIsDirty;
    Matrix4* m_pCachedL2W;
    int      m_nCachedL2WUpdates;
    int      m_nCachedL2WDirties;

    // Cached Current Bone Positions
    bool     m_CachedBonePosIsDirty;
    Vector3* m_pCachedBonePos;
    int      m_nCachedBonePosUpdates;
    int      m_nCachedBonePosDirties;
};

//=========================================================================
// INLINES
//=========================================================================

inline const AnimGroup& char_anim_player::GetAnimGroup()
{
    AnimGroup* pG = m_hAnimGroup.getPointer();
    assert(pG);
    return *pG;
}

//=========================================================================

inline AnimGroup::handle char_anim_player::GetAnimGroupHandle()
{
    return m_hAnimGroup;
}

//=========================================================================

inline void char_anim_player::SetRenderOffset(const Vector3& RenderOffset)
{
    assert(RenderOffset.IsValid());

    // Mark cached L2W matrices as unusable
    DirtyCachedL2W();

    m_RenderOffset = RenderOffset;
}

//=========================================================================

inline Vector3 char_anim_player::GetRenderOffset()
{
    return m_RenderOffset;
}

//=========================================================================

inline bool char_anim_player::IsAtEnd()
{
    return m_AnimTrack.IsAtEnd();
}

//=========================================================================

inline void char_anim_player::SetFrame(float Frame)
{
    //ASSERT( x_isvalid( Frame ) );

    // Mark cached L2W matrices as unusable
    DirtyCachedL2W();

    m_AnimTrack.SetFrame(Frame);
}

//=========================================================================

inline void char_anim_player::SetCycle(int Cycle)
{
    DirtyCachedL2W(); // Mark cached L2W matrices as unusable
    m_AnimTrack.SetCycle(Cycle);
}

//=========================================================================

inline void char_anim_player::SetAnim(const char* pAnimName, bool ManualVert, bool ManualHoriz, float BlendTime)
{
    SetAnim(GetAnimIndex(pAnimName), ManualVert, ManualHoriz, BlendTime);
}

//=========================================================================

inline void char_anim_player::Advance(float nSeconds, Vector3& DeltaPos)
{
    Radian DummyDeltaYaw;
    Advance(nSeconds, DeltaPos, DummyDeltaYaw);
}

//=========================================================================

inline void char_anim_player::SetPosition(const Vector3& Pos)
{
    assert(Pos.IsValid());

    // Mark cached L2W matrices as unusable
    DirtyCachedL2W();

    m_WorldPos = Pos;
}

//=========================================================================

inline void char_anim_player::SetPitch(Radian Pitch)
{
    //ASSERT( x_isvalid( Pitch ) );

    // Mark cached L2W matrices as unusable
    DirtyCachedL2W();

    m_WorldRot.pitch = Pitch;
}

//=========================================================================

inline void char_anim_player::SetYaw(Radian Yaw)
{
    //ASSERT( x_isvalid( Yaw ) );

    // Mark cached L2W matrices as unusable
    DirtyCachedL2W();

    m_WorldRot.yaw = Yaw;
}

//=========================================================================

inline void char_anim_player::SetRoll(Radian Roll)
{
    //ASSERT( x_isvalid( Roll ) );

    // Mark cached L2W matrices as unusable
    DirtyCachedL2W();

    m_WorldRot.roll = Roll;
}

//=========================================================================

inline void char_anim_player::SetScale(float Scale)
{
    //ASSERT( x_isvalid( Scale ) );

    // Mark cached L2W matrices as unusable
    DirtyCachedL2W();

    m_WorldScale = Scale;
}

//=========================================================================

inline Vector3 char_anim_player::GetPosition() const
{
    return m_WorldPos;
}

//=========================================================================

inline Radian3 char_anim_player::GetRotation() const
{
    return m_WorldRot;
}

//=========================================================================

inline Radian char_anim_player::GetPitch()
{
    return m_WorldRot.pitch;
}

//=========================================================================

inline Radian char_anim_player::GetYaw()
{
    return m_WorldRot.yaw;
}

//=========================================================================

inline Radian char_anim_player::GetRoll()
{
    return m_WorldRot.roll;
}

//=========================================================================

inline float char_anim_player::GetScale()
{
    return m_WorldScale;
}

//=========================================================================

inline const Vector3* char_anim_player::GetBonePositions()
{
    return GetCachedBonePoss();
}

//=========================================================================

inline const Vector3& char_anim_player::GetBonePosition(int iBone)
{
    return GetCachedBonePos(iBone);
}

//=========================================================================

inline float char_anim_player::GetFrame()
{
    return m_AnimTrack.GetFrame();
}

//=========================================================================

inline int char_anim_player::GetCycle()
{
    return m_AnimTrack.GetCycle();
}

//=========================================================================

inline float char_anim_player::GetPrevFrame()
{
    return m_AnimTrack.GetPrevFrame();
}

//=========================================================================

inline int char_anim_player::GetPrevCycle()
{
    return m_AnimTrack.GetPrevCycle();
}

//=========================================================================

inline int char_anim_player::GetNFrames()
{
    return m_AnimTrack.GetNFrames();
}

//=========================================================================

inline int char_anim_player::GetAnimIndex()
{
    return m_AnimTrack.GetAnimIndex();
}

//=========================================================================

inline int char_anim_player::GetNAnims()
{
    return GetAnimGroup().GetNAnims();
}

//=========================================================================

inline int char_anim_player::GetNBones()
{
    return GetAnimGroup().GetNBones();
}

//=========================================================================

inline const Matrix4* char_anim_player::GetBoneL2Ws()
{
    return GetCachedL2Ws();
}

//=========================================================================

inline const Matrix4& char_anim_player::GetBoneL2W(int iBone)
{
    return GetCachedL2W(iBone);
}

//=========================================================================

inline int char_anim_player::GetAnimIndex(const char* pAnimName)
{
    return GetAnimGroup().GetAnimIndex(pAnimName);
}

//=========================================================================

inline bool char_anim_player::IsPlaying(const char* pAnimName, int iTrack)
{
    assert((iTrack >= 0) && (iTrack < MAX_CHAR_ANIM_PLAYER_TRACKS) && (m_Track[iTrack]));
    return ((anim_track_controller*)m_Track[iTrack])->IsPlaying(pAnimName);
}

//=========================================================================

inline float char_anim_player::GetSpeed()
{
    return GetAnimGroup().GetAnimInfo(m_AnimTrack.GetAnimIndex()).GetSpeed();
}

//=========================================================================

inline const AnimInfo& char_anim_player::GetAnimInfo()
{
    return m_AnimTrack.GetAnimInfo();
}

//=========================================================================

inline int char_anim_player::GetNEvents()
{
    return m_AnimTrack.GetNEvents();
}

//=========================================================================

inline const anim_event& char_anim_player::GetEvent(int iEvent)
{
    return m_AnimTrack.GetEvent(iEvent);
}

//=========================================================================

inline bool char_anim_player::IsEventActive(int iEvent)
{
    return m_AnimTrack.IsEventActive(iEvent);
}

//=========================================================================

inline bool char_anim_player::IsEventTypeActive(int Type)
{
    return m_AnimTrack.IsEventTypeActive(Type);
}

//=========================================================================

inline int char_anim_player::GetBoneIndex(const char* pBoneName)
{
    return GetAnimGroup().GetBoneIndex(pBoneName);
}

//=========================================================================

inline const AnimBone& char_anim_player::GetBone(int iBone)
{
    return GetAnimGroup().GetBone(iBone);
}

//=========================================================================

inline float char_anim_player::GetFrameParametric()
{
    return m_AnimTrack.GetFrameParametric();
}

//=========================================================================

inline void char_anim_player::SetFrameParametric(float Frame)
{
    //assert( x_isvalid( Frame ) );

    m_AnimTrack.SetFrameParametric(Frame);
}

//=========================================================================

inline track_controller* char_anim_player::GetTrackController(int iTrack)
{
    assert((iTrack >= 0) && (iTrack < MAX_CHAR_ANIM_PLAYER_TRACKS));
    return m_Track[iTrack];
}

//=========================================================================

inline bool char_anim_player::HasAnimGroup()
{
    return (!m_hAnimGroup.isNull());
}

//=========================================================================

inline void char_anim_player::SetBasisMatrix(const Matrix4& BasisM)
{
    //assert( BasisM.IsValid() );

    // Mark cached L2W matrices as unusable
    DirtyCachedL2W();

    m_BasisM = BasisM;
}

//=========================================================================

inline Matrix4& char_anim_player::GetBasisMatrix()
{
    return m_BasisM;
}

//=========================================================================

inline Matrix4& char_anim_player::GetPreviousBasisMatrix()
{
    return m_PreviousBasisM;
}

//=========================================================================

inline Vector3 char_anim_player::GetBindPosition(int iBone)
{
    const AnimBone& ab = GetAnimGroup().GetBone(iBone);
    return ab.bindTranslation;
}
