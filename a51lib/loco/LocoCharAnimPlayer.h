#pragma once

//#include "Animation\SMemMatrixCache.hpp"
#include "LocoMotionController.h"
#include "../animation/BasePlayer.h"

//=========================================================================
// DEFINES
//=========================================================================
#define LOCO_MAX_CHAR_ANIM_PLAYER_TRACKS 9

//=========================================================================
// FORWARD DELCLARATIONS
//=========================================================================
class loco;
class loco_ik_solver;

//=========================================================================
// LOCOMOTION CHARACTER ANIMATION PLAYER CLASS
//=========================================================================
class loco_char_anim_player : public base_player
{

    //=====================================================================
    // PUBLIC FUNCTIONS
    //=====================================================================
public:
    // Constructor/destructor
    loco_char_anim_player(void);
    virtual ~loco_char_anim_player(void);

    // Animation functions

    // Tells animation player about loco owner
    void SetLoco(loco* pLoco);

    // Tells the animation system which package of animations to use.
    virtual void SetAnimGroup(const AnimGroup::handle& hGroup);

    // Bone LOD control - tells the player how many bones to actually compute when mixing
    void SetNActiveBones(int nBones);

    // Sets the new animation and blends out the old animation if BlendTime is non zero
    // NOTE: If the player is currently blending and bInterruptBlend is FALSE, then
    //       the new animation will be started as soon as blending has finished
    void SetAnim(const AnimGroup::handle& hAnimGroup,
                 int                      iAnim,
                 float                    BlendTime = DEFAULT_BLEND_TIME,
                 float                    Rate = 1.0f,
                 uint32_t                 Flags = 0);

    void SetAnim(const AnimGroup::handle& hAnimGroup,
                 const char*              pAnim,
                 float                    BlendTime = DEFAULT_BLEND_TIME,
                 float                    Rate = 1.0f,
                 uint32_t                 Flags = 0);

    void SetCurrAnimFrame(float Frame); // Sets current animation frame
    void SetCurrAnimCycle(int Cycle);   // Sets current animation cycle

    // Gives access to animation tracks for reading individual yaws etc
    loco_motion_controller&       GetCurrAnim(void);
    loco_motion_controller&       GetBlendAnim(void);
    const loco_motion_controller& GetCurrAnim(void) const;
    const loco_motion_controller& GetBlendAnim(void) const;

    // Yaw functions
    Radian GetFacingYaw(void) const;                // Facing yaw (blended)
    Radian GetCurrAnimYaw(void) const;              // Current animation yaw
    void   SetCurrAnimYaw(Radian Yaw);              // Set current animation yaw
    Radian GetBlendAnimYaw(void) const;             // Blend animation yaw
    void   SetBlendAnimYaw(Radian Yaw);             // Set blend animation yaw
    void   ApplyCurrAnimDeltaYaw(Radian DeltaYaw);  // CurrAnimYaw  += DeltaYaw
    void   ApplyBlendAnimDeltaYaw(Radian DeltaYaw); // BlendAnimYaw += DeltaYaw

    // Logic functions - advances the animation and returns the change in position that occurred
    void Advance(float nSeconds, Vector3& DeltaPos, Radian& DeltaYaw);
    void Advance(float nSeconds, Vector3& DeltaPos);

    // Position/speed functions
    const Vector3& GetPosition(void) const;         // Returns world position
    void           SetPosition(const Vector3& Pos); // Sets world position
    float          GetMovementSpeed(void);

    // Prop functions

    // Returns the current L2W for a prop.  If the prop is not present
    // the return value will be FALSE and the matrix undetermined
    bool GetPropL2W(const char* pPropName, Matrix4& L2W);

    // Animation playback query functions
    bool            IsBlending(void) const;                                 // TRUE if blending between animations
    float           GetBlendAmount(void) const;                             // 0 = All blend track, 1 = All current track
    bool            IsAtEnd(void) const;                                    // TRUE if anim has played or looped
    float           GetFrame(void) const;                                   // Current frame of current track
    int             GetCycle(void) const;                                   // Cycle count of current track
    float           GetPrevFrame(void) const;                               // Previous frame of current track
    int             GetPrevCycle(void) const;                               // Previous cylce count of current track
    int             GetNFrames(void) const;                                 // # of frames in current animation
    int             GetAnimIndex(void) const;                               // Index of current animation
    int             GetAnimTypeIndex(void) const;                           // Index passed to "SetAnim" (may differ from actual playing index!)
    const AnimInfo& GetAnimInfo(void) const;                                // Info for current animation
    bool            IsPlaying(const char* pAnimName, int iTrack = 0) const; // TRUE if anim is playing on track

    // Animation group query functions
    const AnimGroup*  GetAnimGroup(void) const;                  // Anim group assigned to player (this may return NULL, so check it!)
    AnimGroup::handle GetAnimGroupHandle(void);                  // Anim group handle assigned to player
    bool              HasAnimGroup(void) const;                  // TRUE if anim group has been assigned
    int               GetNAnims(void) const;                     // # of anims in assigned anim group
    int               GetAnimIndex(const char* pAnimName) const; // Index of anim or -1 if not found

    // Bone query functions
    int             GetNBones(void) const;                     // # of bones in assigned anim group
    int             GetNActiveBones(void) const;               // # of bones to compute (LOD)
    int             GetBoneIndex(const char* pBoneName) const; // Index of bone, or -1 if not found
    const AnimBone& GetBone(int iBone) const;                  // Info for bone
    const Matrix4*  GetBoneL2Ws(void);                         // All bone L2W matrices
    const Matrix4&  GetBoneL2W(int iBone);                     // Bone L2W matrix
    Vector3         GetBonePosition(int iBone);                // Bone position
    const Vector3&  GetBoneBindPosition(int iBone) const;      // Bone bind position

    // Event functions
    virtual int               GetNEvents(void);             // # of event in current animation
    virtual const anim_event& GetEvent(int iEvent);         // Animation event
    virtual bool              IsEventActive(int iEvent);    // TRUE if event should be fired
    bool                      IsEventTypeActive(int Type);  // TRUE if any event of type is active
    virtual Vector3           GetEventPosition(int iEvent); // World position of event.
    virtual Radian3           GetEventRotation(int iEvent); // World rotation of event.
    virtual Vector3           GetEventPosition(const anim_event& Event);
    virtual Radian3           GetEventRotation(const anim_event& Event);

    void GetEventPositionAndRotation(int iEvent, Vector3& Position, Radian3& Rotation);

    // Track functions
    void                  ClearTracks(void);                                            // Remove tracks
    void                  ClearTrack(int iTrack);                                       // Remove track
    loco_anim_controller* GetTrack(int iTrack);                                         // Assigned track
    void                  SetTrack(int iTrack, loco_anim_controller* pTrackController); // Assign track
    void                  SetTracksAnimGroup(const AnimGroup::handle& hAnimGroup);      // Assigns anim group

    // IK functions
    void SetIKSolver(loco_ik_solver* pIKSolver);

    //
    // The YawDelta is divided by the length of the animation and is combined
    // with the animation's delta and returned in the Advance( paDeltaYaw ).
    //
    void SetYawDelta(const Radian& YawDelta);

    // Returns the combined bbox of all the current playing animations
    BBox ComputeBBox(void);

    // Forces matrices to be re-computed the next time any bone query function is called
    void DirtyCachedL2Ws(void);

    //=====================================================================
    // PRIVATE FUNCTIONS
    //=====================================================================

private:
    // Cached matrices functions
    void           GetInterpKeys(const Matrix4& L2W, AnimKey* pKey);
    const Matrix4& GetCachedL2W(int iBone);
    const Matrix4* GetCachedL2Ws(void);
    void           UpdateCachedL2Ws(void);

    //=====================================================================
    // PUBLIC DATA
    //=====================================================================
public:
    // Useful bone indices
    int16_t m_iNeckBone;      // Neck bone index
    int16_t m_iHeadBone;      // Head bone index
    int16_t m_iLEyeBone;      // Index of left eye
    int16_t m_iREyeBone;      // Index of right eye
    int16_t m_iWeaponBone[2]; // Index of right/left hand weapon bone
    int16_t m_iGrenadeBone;   // Index of grenade bone
    int16_t m_iFlagBone;      // Index of mp avatar flag bone

    // Usefull offsets
    Vector3 m_MidEyeOffset; // Offset between eyes from head

    // Useful matrices/positions
    Matrix4 m_HeadL2W;        // Head local to world matrix
    Vector3 m_MidEyePosition; // Position between eyes
    Vector3 m_AimAtOffset;    // Offset from pos for a good spot to aim at

    //=====================================================================
    // PRIVATE DATA
    //=====================================================================
private:
    // Animation vars
    AnimGroup::handle m_hAnimGroup;   // Group of anims we are using
    int               m_nActiveBones; // Number of bones to compute (used for LODs)
    Vector3           m_WorldPos;     // World position

    // Track controller vars
    loco_motion_controller m_AnimCurrTrack;                           // Current animation controller track
    loco_motion_controller m_AnimBlendTrack;                          // Blend animation controller track
    float                  m_AnimBlendFrame;                          // Blend frame
    float                  m_AnimBlendLength;                         // Blend length
    loco_anim_controller*  m_Track[LOCO_MAX_CHAR_ANIM_PLAYER_TRACKS]; // List of tracks

    // IK
    loco_ik_solver* m_pIKSolver; // User IK solver (or NULL if none)

    // Animation request vars (used to wait for blending to finish)
    bool              m_bRequest;          // If TRUE, animation request is pending
    AnimGroup::handle m_hRequestAnimGroup; // Anim group of request anim
    int               m_iRequestAnim;      // Anim index of request anim
    float             m_RequestBlendTime;  // Blend time of request anim
    float             m_RequestRate;       // Playback rate scaler
    uint32_t          m_RequestFlags;      // Flags of anim request

    // Locomotion vars
    loco* m_pLoco; // Pointer to owner locomotion

    // Cached matrices vars
    //smem_matrix_cache       m_CachedL2Ws;           // Cached matrices

    // Chain anim vars
    float  m_ChainCycles; // Cycles to play before playing chain anim
    Radian m_YawDelta;
    float  m_YawStartFrame;
    float  m_YawEndFrame;
};

//=========================================================================
// INLINES
//=========================================================================

inline const Vector3& loco_char_anim_player::GetPosition(void) const
{
    return m_WorldPos;
}

//=========================================================================

// Tells animation player about loco owner
inline void loco_char_anim_player::SetLoco(loco* pLoco)
{
    m_pLoco = pLoco;
}

//=========================================================================

inline void loco_char_anim_player::SetAnim(const AnimGroup::handle& hAnimGroup, const char* pAnim, float BlendTime, float Rate, uint32_t Flags)
{
    SetAnim(hAnimGroup, GetAnimIndex(pAnim), BlendTime, Rate, Flags);
}

//=========================================================================

inline loco_motion_controller& loco_char_anim_player::GetCurrAnim(void)
{
    return m_AnimCurrTrack;
}

//=========================================================================

inline loco_motion_controller& loco_char_anim_player::GetBlendAnim(void)
{
    return m_AnimBlendTrack;
}

//=========================================================================

inline const loco_motion_controller& loco_char_anim_player::GetCurrAnim(void) const
{
    return m_AnimCurrTrack;
}

//=========================================================================

inline const loco_motion_controller& loco_char_anim_player::GetBlendAnim(void) const
{
    return m_AnimBlendTrack;
}

//=========================================================================

inline Radian loco_char_anim_player::GetCurrAnimYaw(void) const
{
    return m_AnimCurrTrack.GetYaw();
}

//=========================================================================

inline Radian loco_char_anim_player::GetBlendAnimYaw(void) const
{
    return m_AnimBlendTrack.GetYaw();
}

//=========================================================================

inline void loco_char_anim_player::Advance(float nSeconds, Vector3& DeltaPos)
{
    Radian DeltaYaw;
    Advance(nSeconds, DeltaPos, DeltaYaw);
}

//=========================================================================

// Returns TRUE if anims are being blended
inline bool loco_char_anim_player::IsBlending(void) const
{
    return (m_AnimBlendLength > 0);
}

// Returns blend amount (0 = all blend, 1=all current)
inline float loco_char_anim_player::GetBlendAmount(void) const
{
    if (m_AnimBlendLength > 0) {
        return m_AnimBlendFrame / m_AnimBlendLength;
    } else {
        return 1.0f;
    }
}

//=========================================================================

inline bool loco_char_anim_player::IsAtEnd(void) const
{
    // Keep logic happy if waiting for a request
    if (m_bRequest) {
        return false;
    }

    return m_AnimCurrTrack.IsAtEnd();
}

//=========================================================================

inline float loco_char_anim_player::GetFrame(void) const
{
    return m_AnimCurrTrack.GetFrame();
}

//=========================================================================

inline int loco_char_anim_player::GetCycle(void) const
{
    return m_AnimCurrTrack.GetCycle();
}

//=========================================================================

inline float loco_char_anim_player::GetPrevFrame(void) const
{
    return m_AnimCurrTrack.GetPrevFrame();
}

//=========================================================================

inline int loco_char_anim_player::GetPrevCycle(void) const
{
    return m_AnimCurrTrack.GetPrevCycle();
}

//=========================================================================

inline int loco_char_anim_player::GetNFrames(void) const
{
    return m_AnimCurrTrack.GetNFrames();
}

//=========================================================================

inline int loco_char_anim_player::GetAnimIndex(void) const
{
    // Keep the logic happy if waiting for a request
    if (m_bRequest) {
        return m_iRequestAnim;
    }

    return m_AnimCurrTrack.GetAnimIndex();
}

//=========================================================================

inline int loco_char_anim_player::GetAnimTypeIndex(void) const
{
    // Keep the logic happy if waiting for a request
    if (m_bRequest) {
        return m_iRequestAnim;
    }

    return m_AnimCurrTrack.GetAnimTypeIndex();
}

//=========================================================================

inline const AnimInfo& loco_char_anim_player::GetAnimInfo(void) const
{
    return m_AnimCurrTrack.GetAnimInfo();
}

//=========================================================================

inline bool loco_char_anim_player::IsPlaying(const char* pAnimName, int iTrack) const
{
    assert((iTrack >= 0) && (iTrack < LOCO_MAX_CHAR_ANIM_PLAYER_TRACKS) && (m_Track[iTrack]));
    return ((loco_anim_controller*)m_Track[iTrack])->IsPlaying(pAnimName);
}

//=========================================================================

inline const AnimGroup* loco_char_anim_player::GetAnimGroup(void) const
{
    AnimGroup* pGroup = (AnimGroup*)m_hAnimGroup.getPointer();
    assert(pGroup);
    return pGroup;
}

//=========================================================================

inline AnimGroup::handle loco_char_anim_player::GetAnimGroupHandle(void)
{
    return m_hAnimGroup;
}

//=========================================================================

inline bool loco_char_anim_player::HasAnimGroup(void) const
{
    return (m_hAnimGroup.getPointer() != nullptr);
}

//=========================================================================

inline int loco_char_anim_player::GetNAnims(void) const
{
    const AnimGroup* pAnimGroup = GetAnimGroup();
    return pAnimGroup ? pAnimGroup->GetNAnims() : 0;
}

//=========================================================================

inline int loco_char_anim_player::GetAnimIndex(const char* pAnimName) const
{
    int Index = GetAnimGroup()->GetAnimIndex(pAnimName);

    return Index;
}

//=========================================================================

inline int loco_char_anim_player::GetNBones(void) const
{
    const AnimGroup* pAnimGroup = GetAnimGroup();
    return pAnimGroup ? pAnimGroup->GetNBones() : 0;
}

//=========================================================================

inline int loco_char_anim_player::GetNActiveBones(void) const
{
    return m_nActiveBones;
}

//=========================================================================

inline int loco_char_anim_player::GetBoneIndex(const char* pBoneName) const
{
    return GetAnimGroup()->GetBoneIndex(pBoneName);
}

//=========================================================================

inline const AnimBone& loco_char_anim_player::GetBone(int iBone) const
{
    return GetAnimGroup()->GetBone(iBone);
}

//=========================================================================

inline const Matrix4* loco_char_anim_player::GetBoneL2Ws(void)
{
    return GetCachedL2Ws();
}

//=========================================================================
// IK functions
//=========================================================================

inline void loco_char_anim_player::SetIKSolver(loco_ik_solver* pIKSolver)
{
    m_pIKSolver = pIKSolver;
}
