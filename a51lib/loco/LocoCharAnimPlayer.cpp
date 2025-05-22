#include "../animation/animData.h"
#include "Loco.h"
#include "LocoCharAnimPlayer.h"
//#include "LocoIKSolver.h"

// NOTE: These names only have to be a sub-string of the bone names
static const char* NECK_ATTACH_BONE_NAME = "Neck";
static const char* HEAD_ATTACH_BONE_NAME = "Head";
static const char* LEFT_EYE_BONE_NAME = "L_Eye";
static const char* RIGHT_EYE_BONE_NAME = "R_Eye";
static const char* RIGHT_WEAPON_ATTACH_BONE_NAME = "Attach_R";
static const char* LEFT_WEAPON_ATTACH_BONE_NAME = "Attach_L";
static const char* GRENADE_ATTACH_BONE_NAME = "Attach_R";
static const char* FLAG_ATTACH_BONE_NAME = "B_Flag";

//=========================================================================
// LOCOMOTION CHARACTER ANIMATION PLAYER CLASS
//=========================================================================

loco_char_anim_player::loco_char_anim_player(ResourceManager* rm)
    : m_hAnimGroup(rm)
    , m_hRequestAnimGroup(rm)
    , m_AnimCurrTrack(rm)
    , m_AnimBlendTrack(rm)
{
    int i;

    // Useful bone indices
    m_iNeckBone = -1;
    m_iHeadBone = -1;
    m_iLEyeBone = -1;
    m_iREyeBone = -1;
    m_iWeaponBone[0] = -1;
    m_iWeaponBone[1] = -1;
    m_iGrenadeBone = -1;
    m_iFlagBone = -1;

    // Useful offsets
    m_MidEyeOffset.Zero();

    // Useful matrices
    m_HeadL2W.Identity();    // Head local to world matrix
    m_MidEyePosition.Zero(); // Position between eyes
    m_AimAtOffset.Zero();    // Offset from pos for a good spot to aim at

    // Animation vars
    m_nActiveBones = 0;
    m_WorldPos.Zero();

    // Track controller vars
    m_AnimBlendFrame = 0;
    m_AnimBlendLength = 0;
    for (i = 0; i < LOCO_MAX_CHAR_ANIM_PLAYER_TRACKS; i++) {
        m_Track[i] = NULL;
    }

    // Add current and blending track to player
    SetTrack(0, &m_AnimCurrTrack);
    SetTrack(1, &m_AnimBlendTrack);

    // IK
    m_pIKSolver = NULL;

    // Animation request (used to wait for blending to finish)
    m_bRequest = false;
    m_iRequestAnim = -1;
    m_RequestBlendTime = 0.0f;
    m_RequestRate = 1.0f;
    m_RequestFlags = 0;
    m_YawDelta = 0.0f;

    // Locomotion vars
    m_pLoco = NULL;

    // Chain anim vars
    m_ChainCycles = -1.0f;
}

//=========================================================================

loco_char_anim_player::~loco_char_anim_player()
{
}

//=========================================================================
// Animation functions
//=========================================================================

void loco_char_anim_player::SetAnimGroup(const AnimGroup::handle& hAnimGroup)
{
    // Keep anim group
    m_hAnimGroup = hAnimGroup;
    assert(GetAnimGroup());
    const AnimGroup& AnimGroup = *GetAnimGroup();

    // Lookup useful bone indices
    m_iNeckBone = AnimGroup.GetBoneIndex(NECK_ATTACH_BONE_NAME, true);
    m_iHeadBone = AnimGroup.GetBoneIndex(HEAD_ATTACH_BONE_NAME, true);
    m_iLEyeBone = AnimGroup.GetBoneIndex(LEFT_EYE_BONE_NAME, true);
    m_iREyeBone = AnimGroup.GetBoneIndex(RIGHT_EYE_BONE_NAME, true);
    m_iWeaponBone[0] = AnimGroup.GetBoneIndex(RIGHT_WEAPON_ATTACH_BONE_NAME, true);
    m_iWeaponBone[1] = AnimGroup.GetBoneIndex(LEFT_WEAPON_ATTACH_BONE_NAME, true);
    m_iGrenadeBone = AnimGroup.GetBoneIndex(GRENADE_ATTACH_BONE_NAME, true);
    m_iFlagBone = AnimGroup.GetBoneIndex(FLAG_ATTACH_BONE_NAME, false);

    // Use root bone for neck and head if they are not found
    if (m_iNeckBone == -1) {
        m_iNeckBone = 0;
    }
    if (m_iHeadBone == -1) {
        m_iHeadBone = 0;
    }

    // Use head bone if eyes are not found
    if (m_iLEyeBone == -1) {
        m_iLEyeBone = m_iHeadBone;
    }
    if (m_iREyeBone == -1) {
        m_iREyeBone = m_iHeadBone;
    }

    // Use root bone if weapon, grenade, or flag bone not found
    if (m_iWeaponBone[0] == -1) {
        m_iWeaponBone[0] = 0;
    }
    if (m_iWeaponBone[1] == -1) {
        m_iWeaponBone[1] = 0;
    }
    if (m_iGrenadeBone == -1) {
        m_iGrenadeBone = 0;
    }
    if (m_iFlagBone == -1) {
        m_iFlagBone = 0;
    }

    // Setup initial eye and aim offsets
    assert(m_iHeadBone != -1);
    assert(m_iLEyeBone != -1);
    assert(m_iREyeBone != -1);

    // Lookup bind positions
    Vector3 HeadBindPos = AnimGroup.GetBone(m_iHeadBone).bindTranslation;
    Vector3 LEyeOffset = AnimGroup.GetBone(m_iLEyeBone).bindTranslation - HeadBindPos;
    Vector3 REyeOffset = AnimGroup.GetBone(m_iREyeBone).bindTranslation - HeadBindPos;

    // Setup mid eye offset (used by aimers etc)
    m_MidEyeOffset = HeadBindPos + ((LEyeOffset + REyeOffset) * 0.5f);
    m_AimAtOffset = m_MidEyeOffset;

    // Init active bones
    m_nActiveBones = AnimGroup.GetNBones();
    assert(m_nActiveBones > 0);

    assert(m_nActiveBones <= MAX_ANIM_BONES);

    // Dirty cached L2W arrays
    DirtyCachedL2Ws();

    // Notify track controllers of anim group
    SetTracksAnimGroup(hAnimGroup);

    // Animation request (used to wait for blending to finish)
    m_bRequest = false;
    m_iRequestAnim = -1;
    m_RequestBlendTime = 0.0f;
    m_RequestRate = 1.0f;
    m_RequestFlags = 0;

    // Set a default animation so that characters are not in bind pose
    int NAnims = GetAnimGroup()->GetNAnims();
    if (NAnims) {
        // Set the default anim to be animation 0 which is always the bind pose
        m_AnimCurrTrack.SetAnim(hAnimGroup, 0);
        m_AnimCurrTrack.SetWeight(1.0f);
    }
}

//=========================================================================

// Bone LOD control - tells the player how many bones to actually compute when mixing
void loco_char_anim_player::SetNActiveBones(int nBones)
{
    // Valid?
    assert(nBones > 0);
    //ASSERTS(nBones <= MAX_ANIM_BONES, "matx has too many bones!");
    //ASSERTS( nBones <= (GetAnimGroup() ? GetAnimGroup()->GetNBones() : 0), "more active bones than in anim group!") ;

    // If # has got bigger we need to invalidate the matrix cache
    if (nBones > m_nActiveBones) {
        DirtyCachedL2Ws();
    }

    // Keep
    m_nActiveBones = nBones;
}

//=========================================================================

void loco_char_anim_player::SetAnim(const AnimGroup::handle& hAnimGroup, int iAnim, float BlendTime, float Rate, uint32_t Flags)
{
    assert(Rate >= 0);

    // No animation?
    if (iAnim == -1) {
        return;
    }

    // If not interupting blend and currently blending, store as a request
    if (((Flags & loco::ANIM_FLAG_INTERRUPT_BLEND) == 0) && (IsBlending()) && (BlendTime != 0)) {
        // Setup request
        m_bRequest = true;
        m_hRequestAnimGroup = hAnimGroup;
        m_iRequestAnim = iAnim;
        m_RequestBlendTime = BlendTime;
        m_RequestRate = Rate;
        m_RequestFlags = Flags;
        return;
    }

    // Clear request
    if (!m_bRequest) {
        m_YawDelta = 0.0f;
    }
    m_bRequest = false;
    m_iRequestAnim = -1;
    m_RequestBlendTime = 0;
    m_RequestRate = 1.0f;
    m_RequestFlags = 0;

    // Mark cached L2W matrices as unusable
    DirtyCachedL2Ws();

    // Get current yaw (maybe a blend)
    Radian CurrYaw = GetFacingYaw();

    // Copy current anim into blend anim incase blending is specified in anim
    m_AnimBlendTrack = m_AnimCurrTrack;

    // Set the current animation, preserving the yaw
    m_AnimCurrTrack.SetAnim(hAnimGroup, iAnim, Flags);
    m_AnimCurrTrack.SetRate(Rate);
    m_AnimCurrTrack.SetWeight(1.0f);
    m_AnimCurrTrack.SetYaw(CurrYaw);
    m_AnimCurrTrack.SetRate(Rate);

    // Fail?
    if (m_AnimCurrTrack.GetAnimIndex() == -1) {
        return;
    }

    // Blend aimer in/out?
    if (Flags & loco::ANIM_FLAG_TURN_OFF_AIMER) {
        //  m_pLoco->GetAimController().SetWeight(0.0f, 0.2f);
    }

    // Use blend time from animation if it's specified
    const AnimInfo& AnimInfo = m_AnimCurrTrack.GetAnimInfo();
    if (AnimInfo.GetBlendTime() >= 0.0f) {
        BlendTime = AnimInfo.GetBlendTime();
    }

    // Turn on blending?
    if (BlendTime > 0.0f &&
        (Flags & loco::ANIM_FLAG_DO_NO_BLENDING) == 0) {
        // Already blending?
        //  if (IsBlending())
        //      x_DebugMsg("Blending interrupted\n") ;

        // Setup blend track
        m_AnimBlendFrame = 0;
        m_AnimBlendLength = BlendTime;

        // Turn on blend
        m_AnimBlendTrack.SetWeight(1.0f);
    } else {
        // Turn off blend
        m_AnimBlendTrack.SetWeight(0.0f);
    }

    // Owned by loco?
    if (m_pLoco) {
        // Tell loco about gravity and world collision
        //    m_pLoco->m_Physics.SetLocoGravityOn(m_AnimCurrTrack.GetGravity());
        //    m_pLoco->m_Physics.SetLocoCollisionOn(m_AnimCurrTrack.GetWorldCollision());
    }

    // Setup chain cycle count?
    /*
    if (AnimInfo.GetChainAnim() != -1) {
        m_ChainCycles = x_frand((float)AnimInfo.GetChainFramesMin(), (float)AnimInfo.GetChainFramesMax()) / ((float)AnimInfo.GetNFrames() - 1);
        if (AnimInfo.ChainCyclesInteger()) {
            m_ChainCycles = x_floor(m_ChainCycles);
        }
    } else */
    {

        m_ChainCycles = -1.0f;
    }
}

//=========================================================================

void loco_char_anim_player::SetCurrAnimFrame(float Frame)
{
    // Update?
    if (m_AnimCurrTrack.GetFrame() != Frame) {
        // Mark cached L2W matrices as unusable
        DirtyCachedL2Ws();

        // Set
        m_AnimCurrTrack.SetFrame(Frame);
    }
}

//=========================================================================

void loco_char_anim_player::SetCurrAnimCycle(int Cycle)
{
    // Update?
    if (m_AnimCurrTrack.GetCycle() != Cycle) {
        DirtyCachedL2Ws(); // Mark cached L2W matrices as unusable
        m_AnimCurrTrack.SetCycle(Cycle);
    }
}

//=========================================================================
// Yaw functions
//=========================================================================

Radian loco_char_anim_player::GetFacingYaw() const
{
    // Get current animation yaw and return if no blending
    Radian CurrYaw = m_AnimCurrTrack.GetYaw();
    if (IsBlending() == false) {
        return CurrYaw;
    }

    // Get blending animation yaw
    Radian BlendYaw = m_AnimBlendTrack.GetYaw();

    // Compute delta yaw, blending in the requested direction
    Radian DeltaYaw = x_MinAngleDiff(CurrYaw, BlendYaw);
    float  BlendT = GetBlendAmount();

    // Flip blending direction to the long way round?
    if (m_AnimCurrTrack.GetAnimFlags() & loco::ANIM_FLAG_REVERSE_YAW_BLEND) {
        if (DeltaYaw > 0) {
            DeltaYaw -= R_360;
        } else {
            DeltaYaw += R_360;
        }
    }

    // Blend between blend and current animation yaw
    float FacingYaw = BlendYaw + (BlendT * DeltaYaw);
    FacingYaw = x_ModAngle2(FacingYaw);
    return FacingYaw;
}

//=========================================================================

void loco_char_anim_player::SetCurrAnimYaw(Radian Yaw)
{
    // Update?
    if (m_AnimCurrTrack.GetYaw() != Yaw) {
        // Mark cached L2W matrices as unusable
        DirtyCachedL2Ws();

        // Update yaw
        m_AnimCurrTrack.SetYaw(Yaw);
    }
}

//=========================================================================

void loco_char_anim_player::SetBlendAnimYaw(Radian Yaw)
{
    // Update?
    if (m_AnimBlendTrack.GetYaw() != Yaw) {
        // Mark cached L2W matrices as unusable
        DirtyCachedL2Ws();

        // Update yaw
        m_AnimBlendTrack.SetYaw(Yaw);
    }
}

//=========================================================================

void loco_char_anim_player::ApplyCurrAnimDeltaYaw(Radian DeltaYaw)
{
    // Update?
    if (DeltaYaw != 0) {
        // Mark cached L2W matrices as unusable
        DirtyCachedL2Ws();

        // Update yaw
        m_AnimCurrTrack.ApplyDeltaYaw(DeltaYaw);
    }
}

//=========================================================================

void loco_char_anim_player::ApplyBlendAnimDeltaYaw(Radian DeltaYaw)
{
    // Update?
    if (DeltaYaw != 0) {
        // Mark cached L2W matrices as unusable
        DirtyCachedL2Ws();

        // Update yaw
        m_AnimBlendTrack.ApplyDeltaYaw(DeltaYaw);
    }
}

//=========================================================================
// Logic functions - advances the animation and returns the change in position that occurred
//=========================================================================

void loco_char_anim_player::Advance(float nSeconds, Vector3& DeltaPos, Radian& DeltaYaw)
{
    //LOG_STAT( k_stats_Animation );
    //CONTEXT("loco_char_anim_player::Advance") ;

    // Clear blend deltas incase anim rate is set to zero!
    DeltaPos.Zero();
    DeltaYaw = 0;

    // If completely frozen just return
    if (nSeconds == 0) {
        return;
    }

    // Keep yaw incase full body lip sync controller needs to start with the current yaw
    Radian CurrYaw = m_AnimCurrTrack.GetYaw();

    // If a request is pending, try to it!
    if ((m_bRequest) && (!IsBlending())) {
        SetAnim(m_hRequestAnimGroup, m_iRequestAnim, m_RequestBlendTime, m_RequestRate, m_RequestFlags);
    }

    // Check for chaining anims?
    loco_motion_controller& CurrTrack = m_AnimCurrTrack;
    if ((m_ChainCycles != -1.0f) && (CurrTrack.GetAnimIndex() != -1) && (IsBlending() == false)) {
        // Lookup info
        const AnimInfo& AnimInfo = CurrTrack.GetAnimInfo();

        // Played enough cycles?
        int   I = (int)m_ChainCycles;
        float F = m_ChainCycles - (float)I;
        if ((CurrTrack.GetCycle() >= I) && (CurrTrack.GetFrameParametric() >= F)) {
            // Setup flags
            uint32_t Flags = CurrTrack.GetAnimFlags() | loco::ANIM_FLAG_RESTART_IF_SAME_ANIM;
            int      Frame = AnimInfo.GetChainFrame();
            if (Frame < 0) {
                Flags |= loco::ANIM_FLAG_START_ON_SAME_FRAME;
            }

            // Pick new anim
            SetAnim(CurrTrack.GetAnimGroupHandle(), // hAnimGroup
                    AnimInfo.GetChainAnim(),        // iAnim
                    0.2f,                           // BlendTime
                    CurrTrack.GetRate(),            // Rate
                    Flags);                         // Flags

            // Set start frame if it's specified
            if (Frame >= 0) {
                CurrTrack.SetFrame((float)Frame);
            }
        }
    }

    // Mark cached L2W matrices as unusable
    DirtyCachedL2Ws();

    // Advance other tracks
    for (int i = 2; i < LOCO_MAX_CHAR_ANIM_PLAYER_TRACKS; i++) {
        // Lookup controller
        loco_anim_controller* pTrack = m_Track[i];

        // Present?
        if (!pTrack) {
            continue;
        }

        // Advance
        pTrack->Advance(nSeconds, DeltaPos, DeltaYaw);

        // Sanity check
        assert(DeltaPos.IsValid());
        assert(isvalid(DeltaYaw));
        assert(abs(DeltaPos.GetX()) < (100.0f * 10.0f));
        assert(abs(DeltaPos.GetY()) < (100.0f * 10.0f));
        assert(abs(DeltaPos.GetZ()) < (100.0f * 10.0f));

        // Any influence?
        if (pTrack->GetWeight() == 0.0f) {
            continue;
        }

        // Fix full body accumulation on lip sync anims etc:
        // Is this track playing on full body?
        if (pTrack->IsFullBody()) {
            // If lip sync has not been started on main track yet, then do it!
            if (pTrack->GetStartedOnMainTrack() == false) {
                // Start the anim
                SetAnim(pTrack->m_hAnimGroup,
                        pTrack->m_iAnim,
                        DEFAULT_BLEND_TIME,
                        pTrack->m_Rate,
                        pTrack->m_AnimFlags | loco::ANIM_FLAG_INTERRUPT_BLEND);

                // Make sure yaw is maintained
                m_AnimCurrTrack.SetYaw(CurrYaw);

                // Turn off accumulation so blend anim doesn't screw up the pos/yaw of the cinema
                m_AnimBlendTrack.SetAccumHorizMotion(false);
                m_AnimBlendTrack.SetAccumVertMotion(false);
                m_AnimBlendTrack.SetAccumYawMotion(false);

                // Make sure this only happens once!
                pTrack->SetStartedOnMainTrack(true);
            }

            // Is lip sync playing on current track?
            bool bOnCurr = (m_AnimCurrTrack.m_hAnimGroup.getPointer() == pTrack->m_hAnimGroup.getPointer()) &&
                           (m_AnimCurrTrack.m_iAnim == pTrack->m_iAnim);

            // Is lip sync playing on blend track?
            // NOTE: If the same anim is playing on the current and the blend anim, then to avoid popping,
            //       do not update the blend anim (fixes DrWhite pops where the same anim, but different
            //       audio is triggered multiple times back to back)
            bool bOnBlend = (m_AnimBlendTrack.m_hAnimGroup.getPointer() == pTrack->m_hAnimGroup.getPointer()) &&
                            (m_AnimBlendTrack.m_iAnim == pTrack->m_iAnim) &&
                            (m_AnimBlendTrack.m_iAnim != m_AnimCurrTrack.m_iAnim);

            // If playing on the current track, keep it in sync so accumulation works
            if (bOnCurr) {
                assert(pTrack->GetStartedOnMainTrack());
                m_AnimCurrTrack.m_Rate = pTrack->m_Rate;
                m_AnimCurrTrack.m_Frame = pTrack->m_Frame;
                m_AnimCurrTrack.m_Cycle = pTrack->m_Cycle;
                m_AnimCurrTrack.m_PrevFrame = pTrack->m_PrevFrame;
                m_AnimCurrTrack.m_PrevCycle = pTrack->m_PrevCycle;
            }

            // If playing on the blend track, keep it in sync so accumulation works
            if (bOnBlend) {
                assert(pTrack->GetStartedOnMainTrack());
                m_AnimBlendTrack.m_Rate = pTrack->m_Rate;
                m_AnimBlendTrack.m_Frame = pTrack->m_Frame;
                m_AnimBlendTrack.m_Cycle = pTrack->m_Cycle;
                m_AnimBlendTrack.m_PrevFrame = pTrack->m_PrevFrame;
                m_AnimBlendTrack.m_PrevCycle = pTrack->m_PrevCycle;
            }
        }
    }

    // Playing base animations?
    if ((m_AnimCurrTrack.GetWeight() > 0) || (m_AnimBlendTrack.GetWeight() > 0)) {
        // Playing blend animation?
        if (m_AnimBlendLength > 0) {
            // Advance blending
            m_AnimBlendFrame += nSeconds;

            // Finished?
            if (m_AnimBlendFrame >= m_AnimBlendLength) {
                // Turn off blending
                m_AnimBlendTrack.SetWeight(0);
                m_AnimBlendFrame = 0;
                m_AnimBlendLength = 0;

                // Advance current animation
                m_AnimCurrTrack.Advance(nSeconds, DeltaPos, DeltaYaw);

                // only do the yaw thing if we are within the start/end frame
                if (m_AnimCurrTrack.GetFrame() > m_YawStartFrame &&
                    m_AnimCurrTrack.GetFrame() < m_YawEndFrame) {
                    DeltaYaw += m_YawDelta * nSeconds;

                    Radian yawHolder = m_AnimCurrTrack.GetYaw();
                    yawHolder += m_YawDelta * nSeconds;
                    m_AnimCurrTrack.SetYaw(yawHolder);
                }
            } else {
                // Clear blend deltas incase anim rate is set to zero!
                Vector3 BlendDeltaPos(0.0f, 0.0f, 0.0f);
                Radian  BlendDeltaYaw = 0.0f;

                // Compute mix weights of anims
                float CurrWeight = (m_AnimBlendFrame / m_AnimBlendLength);
                float BlendWeight = 1.0f - CurrWeight;

                // Advance current animation
                m_AnimCurrTrack.Advance(nSeconds, DeltaPos, DeltaYaw);

                // Advance blend animation
                m_AnimBlendTrack.SetWeight(BlendWeight);
                m_AnimBlendTrack.Advance(nSeconds, BlendDeltaPos, BlendDeltaYaw);

                // Blend deltas with current animation deltas
                assert(BlendDeltaPos.IsValid());
                assert(isvalid(BlendDeltaYaw));
                DeltaPos += BlendWeight * (BlendDeltaPos - DeltaPos);
                DeltaYaw += BlendWeight * (BlendDeltaYaw - DeltaYaw);
                DeltaYaw += m_YawDelta * nSeconds;

                // Jason's - what is this used for? Can you comment it please. SB.

                // This distributes a given yaw evenly across an anim from start to end frame
                // allows the character to end up facing where we want once the anim is finished.
                if (m_AnimCurrTrack.GetFrame() > m_YawStartFrame &&
                    m_AnimCurrTrack.GetFrame() < m_YawEndFrame) {
                    Radian yawHolder = m_AnimCurrTrack.GetYaw();
                    yawHolder += m_YawDelta * nSeconds;
                    m_AnimCurrTrack.SetYaw(yawHolder);

                    yawHolder = m_AnimBlendTrack.GetYaw();
                    yawHolder += m_YawDelta * nSeconds;
                    m_AnimBlendTrack.SetYaw(yawHolder);
                }
            }
        } else {
            // Advance current animation
            m_AnimCurrTrack.Advance(nSeconds, DeltaPos, DeltaYaw);

            DeltaYaw += m_YawDelta * nSeconds;
            if (m_AnimCurrTrack.GetFrame() > m_YawStartFrame &&
                m_AnimCurrTrack.GetFrame() < m_YawEndFrame) {
                Radian yawHolder = m_AnimCurrTrack.GetYaw();
                yawHolder += m_YawDelta * nSeconds;
                m_AnimCurrTrack.SetYaw(yawHolder);
            }
        }
    }
}

//=========================================================================
// Position/speed functions
//=========================================================================

void loco_char_anim_player::SetPosition(const Vector3& Pos)
{
    // Update?
    if (m_WorldPos != Pos) {
        // Mark cached L2W matrices as unusable
        DirtyCachedL2Ws();

        // Set
        m_WorldPos = Pos;
    }
}

//=========================================================================

// Returns movement speed
float loco_char_anim_player::GetMovementSpeed()
{
    // Blending?
    if (m_AnimBlendLength > 0) {
        // Get blend ratio
        float T = GetBlendAmount();

        // Lookup speeds
        float CurrAnimSpeed = GetCurrAnim().GetMovementSpeed();
        float BlendAnimSpeed = GetBlendAnim().GetMovementSpeed();

        // Blend between
        return BlendAnimSpeed + (T * (CurrAnimSpeed - BlendAnimSpeed));
    } else {
        return GetCurrAnim().GetMovementSpeed();
    }
}

//=========================================================================
// Prop functions
//=========================================================================

bool loco_char_anim_player::GetPropL2W(const char* pPropName, Matrix4& L2W)
{
    //LOG_STAT( k_stats_Animation );

    // Lookup info
    assert(GetAnimGroup());
    const AnimGroup& AnimGroup = *GetAnimGroup();
    const AnimInfo&  AnimInfo = m_AnimCurrTrack.GetAnimInfo();

    // Is this prop channel present?
    int PropChannel = AnimInfo.GetPropChannel(pPropName);
    if (PropChannel == -1) {
        L2W.Identity();
        return false;
    }

    // Get key from raw prop animation
    AnimKey PropKey;
    AnimInfo.GetPropInterpKey(PropChannel, m_AnimCurrTrack.GetFrame(), PropKey);

    // Build final L2W for prop
    PropKey.Setup(L2W);

    // Put into world space if attached to a parent bone
    int iParent = AnimInfo.GetPropParentBoneIndex(PropChannel);
    if (iParent != -1) {
        // If the parent bone is not computed, keep going up the tree
        assert(m_nActiveBones);
        while (iParent >= m_nActiveBones) {
            iParent = AnimGroup.GetBoneParent(iParent);
        }

        // Mult by parent bone
        const Matrix4& ParentM = GetCachedL2W(iParent);
        L2W = ParentM * L2W;
    }

    // return
    return true;
}

//=========================================================================
// Bone query functions
//=========================================================================

const Matrix4& loco_char_anim_player::GetBoneL2W(int iBone)
{
    // Get bone matrices
    const Matrix4* pL2W = GetCachedL2Ws();
    if (!pL2W) {
        static Matrix4 I;
        I.Identity();
        return I;
    }

    // Lookup animation info
    assert(GetAnimGroup());
    const AnimGroup& AnimGroup = *GetAnimGroup();
    int              nBones = GetNActiveBones();

    // Make sure bone index is valid
    assert(iBone >= 0);
    assert(iBone < AnimGroup.GetNBones());

    // If the bone is not active, keep going up the hierarchy
    assert(nBones);
    while (iBone >= nBones) {
        iBone = AnimGroup.GetBoneParent(iBone);
    }

    // Return bone
    return pL2W[iBone];
}

//=========================================================================

Vector3 loco_char_anim_player::GetBonePosition(int iBone)
{
    // Get bone matrices
    const Matrix4* pL2W = GetCachedL2Ws();
    if (!pL2W) {
        return Vector3(0, 0, 0);
    }

    // Lookup animation info
    assert(GetAnimGroup());
    const AnimGroup& AnimGroup = *GetAnimGroup();
    int              nBones = GetNActiveBones();

    // Make sure bone index is valid
    assert(iBone >= 0);
    assert(iBone < AnimGroup.GetNBones());

    // If the bone is not active, keep going up the hierarchy
    assert(nBones);
    while (iBone >= nBones) {
        iBone = AnimGroup.GetBoneParent(iBone);
    }

    // Return bone matrix translation, but counter-act the InvBind from the front
    return pL2W[iBone] * AnimGroup.GetBone(iBone).bindTranslation;
}

//=========================================================================

const Vector3& loco_char_anim_player::GetBoneBindPosition(int iBone) const
{
    assert(GetAnimGroup());
    const AnimBone& AnimBone = GetAnimGroup()->GetBone(iBone);
    return AnimBone.bindTranslation;
}

int loco_char_anim_player::GetNEvents()
{
    return m_AnimCurrTrack.GetNEvents();
}

//=========================================================================

const anim_event& loco_char_anim_player::GetEvent(int iEvent)
{
    return m_AnimCurrTrack.GetEvent(iEvent);
}

//=========================================================================

bool loco_char_anim_player::IsEventActive(int iEvent)
{
    return m_AnimCurrTrack.IsEventActive(iEvent);
}

//=========================================================================

bool loco_char_anim_player::IsEventTypeActive(int Type)
{
    return m_AnimCurrTrack.IsEventTypeActive(Type);
}

//=========================================================================

Vector3 loco_char_anim_player::GetEventPosition(int iEvent)
{
    const anim_event& EV = m_AnimCurrTrack.GetEvent(iEvent);

    //event_data eventData = EV.GetData();  // For debugging!

    const Matrix4& BoneM = GetBoneL2W(EV.GetInt(anim_event::INT_IDX_BONE));
    Vector3        P = BoneM * EV.GetPoint(anim_event::POINT_IDX_OFFSET);
    return P;
}

//=========================================================================

Radian3 loco_char_anim_player::GetEventRotation(int iEvent)
{
    const anim_event& EV = m_AnimCurrTrack.GetEvent(iEvent);

    //event_data eventData = EV.GetData();

    const Matrix4& BoneM = GetBoneL2W(EV.GetInt(anim_event::INT_IDX_BONE));

    Vector3 ERot(EV.GetPoint(anim_event::POINT_IDX_ROTATION));
    Radian3 Rot(ERot.GetX(), ERot.GetY(), ERot.GetZ());

    Matrix4 EventRot(Rot);
    Matrix4 WorldRot = BoneM * EventRot;
    Rot = WorldRot.GetRotation();

    return Rot;
}

//=========================================================================

Vector3 loco_char_anim_player::GetEventPosition(const anim_event& Event)
{
    // Get world position.
    const Matrix4& BoneM = GetBoneL2W(Event.GetInt(anim_event::INT_IDX_BONE));
    Vector3        P = BoneM * Event.GetPoint(anim_event::POINT_IDX_OFFSET);
    return P;
}

//=========================================================================

Radian3 loco_char_anim_player::GetEventRotation(const anim_event& Event)
{
    // Get rotation.
    const Matrix4& BoneM = GetBoneL2W(Event.GetInt(anim_event::INT_IDX_BONE));

    Vector3 ERot(Event.GetPoint(anim_event::POINT_IDX_ROTATION));
    Radian3 Rot(ERot.GetX(), ERot.GetY(), ERot.GetZ());

    Matrix4 EventRot(Rot);
    Matrix4 WorldRot = BoneM * EventRot;
    Rot = WorldRot.GetRotation();

    return Rot;
}

//=========================================================================

void loco_char_anim_player::GetEventPositionAndRotation(int iEvent, Vector3& Position, Radian3& Rotation)
{
    const anim_event& EV = m_AnimCurrTrack.GetEvent(iEvent);

    //event_data eventData = EV.GetData();

    const Matrix4& BoneM = GetBoneL2W(EV.GetInt(anim_event::INT_IDX_BONE));

    Position = BoneM * EV.GetPoint(anim_event::POINT_IDX_OFFSET);

    Vector3 ERot(EV.GetPoint(anim_event::POINT_IDX_ROTATION));
    Radian3 Rot(ERot.GetX(), ERot.GetY(), ERot.GetZ());
    Matrix4 EventRot(Rot);
    Matrix4 WorldRot = BoneM * EventRot;
    Rotation = WorldRot.GetRotation();
}

//=========================================================================
// Track functions
//=========================================================================

void loco_char_anim_player::ClearTracks()
{
    // Mark cached L2W matrices as unusable
    DirtyCachedL2Ws();

    for (int i = 1; i < LOCO_MAX_CHAR_ANIM_PLAYER_TRACKS; i++) {
        if (m_Track[i]) {
            m_Track[i]->Clear();
        }
    }
}

//=========================================================================

void loco_char_anim_player::ClearTrack(int iTrack)
{
    // Mark cached L2W matrices as unusable
    DirtyCachedL2Ws();

    assert((iTrack >= 0) && (iTrack < LOCO_MAX_CHAR_ANIM_PLAYER_TRACKS));
    if (m_Track[iTrack]) {
        m_Track[iTrack]->Clear();
    }
}

//=========================================================================

loco_anim_controller* loco_char_anim_player::GetTrack(int iTrack)
{
    assert((iTrack >= 0) && (iTrack < LOCO_MAX_CHAR_ANIM_PLAYER_TRACKS));
    return m_Track[iTrack];
}

//=========================================================================

void loco_char_anim_player::SetTrack(int iTrack, loco_anim_controller* pTrackController)
{
    // Mark cached L2W matrices as unusable
    DirtyCachedL2Ws();

    assert((iTrack >= 0) && (iTrack < LOCO_MAX_CHAR_ANIM_PLAYER_TRACKS));
    m_Track[iTrack] = pTrackController;

    if (m_Track[iTrack]) {
        m_Track[iTrack]->Clear();
    }
}

//=========================================================================

void loco_char_anim_player::SetTracksAnimGroup(const AnimGroup::handle& hAnimGroup)
{
    // Notify track controllers of anim group
    for (int i = 0; i < LOCO_MAX_CHAR_ANIM_PLAYER_TRACKS; i++) {
        if (m_Track[i]) {
            m_Track[i]->SetAnimGroup(hAnimGroup);
        }
    }
}

//=====================================================================
// PRIVATE FUNCTIONS
//=====================================================================

void loco_char_anim_player::GetInterpKeys(const Matrix4& L2W, AnimKey* pKey)
{
    int                        i;
    loco_anim_controller::info Info;

    // Setup # of bones to compute
    Info.m_nActiveBones = m_nActiveBones;

    // Setup L2W
    Info.m_Local2World = L2W;

    // Compute final facing yaw ( R_180 is because anims are exported 180 degrees off in max!)
    float FacingYaw = GetFacingYaw() + R_180;
    Info.m_Local2AnimSpace.Setup(Radian3(0.0f, FacingYaw, 0.0f));

    // Set to identity since we are doing the yaw fixup last now!
    Info.m_Local2AimSpace.identity();

    // Setup head info
    Info.m_HeadL2W = m_HeadL2W;                          // Head local to world matrix
    Info.m_MidEyePosition = m_WorldPos + m_MidEyeOffset; // World position between eyes

    // Compute blended keys
    m_AnimCurrTrack.GetInterpKeys(Info, pKey);
    m_AnimBlendTrack.MixKeys(Info, pKey);

    // Loop through tracks and mix in
    for (i = 2; i < LOCO_MAX_CHAR_ANIM_PLAYER_TRACKS; i++) {
        // Lookup controller
        loco_anim_controller* pTrack = m_Track[i];

        // Present?
        if (!pTrack) {
            continue;
        }

        // Skip if full body, since it will be in the curr anim track
        if (pTrack->IsFullBody()) {
            continue;
        }

        // Mix into keys
        pTrack->MixKeys(Info, pKey);
    }

    // Apply fixup rotation AFTER the above blending has took place in animation space.
    // This fixes the case of blending between forward + backwards motions
    // when the quaternion blend would flip the otherway partway thru the anims
    // which are 180 degrees off from each other.
    pKey[0].rotation = Info.m_Local2AnimSpace * pKey[0].rotation;
    pKey[0].translation = Info.m_Local2AnimSpace * pKey[0].translation;
}

//=========================================================================

void loco_char_anim_player::DirtyCachedL2Ws()
{
    //m_CachedL2Ws.SetDirty(true);
}

//=========================================================================

const Matrix4* loco_char_anim_player::GetCachedL2Ws()
{
    // Update if needed
    UpdateCachedL2Ws();
    /*
        // If cache is not good, exit
        if (!m_CachedL2Ws.IsValid(m_nActiveBones)) {
            return NULL;
        }

        // Get cache data
        return m_CachedL2Ws.GetMatrices();
        */
    return nullptr;
}

//=========================================================================

const Matrix4& loco_char_anim_player::GetCachedL2W(int iBone)
{
    // Lookup matrices
    const Matrix4* pMatrices = GetCachedL2Ws();
    if (!pMatrices) {
        // Just return the players position
        static Matrix4 I;
        I.Identity();
        I.SetTranslation(m_WorldPos);
        return I;
    }

    // If the bone is not active, keep going up the tree
    assert(GetAnimGroup());
    const AnimGroup& AnimGroup = *GetAnimGroup();
    assert(m_nActiveBones);
    while (iBone >= m_nActiveBones) {
        iBone = AnimGroup.GetBoneParent(iBone);
    }

    // In range?
    assert((iBone >= 0) && (iBone < GetAnimGroup()->GetNBones()));

    // Return bone matrix
    return pMatrices[iBone];
}

//=========================================================================

void loco_char_anim_player::UpdateCachedL2Ws()
{
    //LOG_STAT( k_stats_Animation );
    //CONTEXT("loco_char_anim_player::UpdateCachedL2Ws") ;

    int i;

    // If cache is all good, then there is nothing to do
    //if (m_CachedL2Ws.IsValid(m_nActiveBones)) {
    //    return;
    //}

    return;
#if 0
    // Allocate new matrices
    Matrix4* pMatrices = m_CachedL2Ws.GetMatrices(m_nActiveBones);
    if (!pMatrices) {
        return;
    }

    // Setup L2W
    static Matrix4 L2W;
    L2W.Identity();
    L2W.SetTranslation(m_WorldPos);

    // Allocate mix buffer
    AnimKey* MixBuffer = base_player::GetMixBuffer(base_player::MIX_BUFFER_PLAYER);
    assert(MixBuffer);

    // Grab the keys
    GetInterpKeys(L2W, MixBuffer);

    // Convert to world space
    assert(GetAnimGroup());
    const AnimGroup& AnimGroup = *GetAnimGroup();

    // Compute relative keys into world space matrices
    for (i = 0; i < m_nActiveBones; i++) {
        // Setup matrix for local space key
        Matrix4& BoneL2W = pMatrices[i];
        MixBuffer[i].Setup(BoneL2W);

        // Concatenate with parent or L2W
        int            iParent = AnimGroup.GetBoneParent(i);
        const Matrix4& ParentL2W = (iParent == -1) ? (L2W) : (pMatrices[iParent]);
        BoneL2W = ParentL2W * BoneL2W;
    }

    // Apply inverse bind ready for skin rendering
    for (i = 0; i < m_nActiveBones; i++) {
        Matrix4& BoneL2W = pMatrices[i];
        BoneL2W = BoneL2W * AnimGroup.GetBoneBindInvMatrix(i);
    }

    // Apply IK to bone matrices?
    if (m_pIKSolver) {
        // Do the math
        m_pIKSolver->Solve(pMatrices, m_nActiveBones);
    }

    // Compute eye offsets relative to parent bone (head)
    if ((m_iHeadBone != -1) && (m_iHeadBone < m_nActiveBones)) {
        // Update mid eye offset from eye bones?
        Vector3 HeadBindPos = AnimGroup.GetBone(m_iHeadBone).bindTranslation;
        if ((m_iLEyeBone != -1) && (m_iLEyeBone < m_nActiveBones) && (m_iREyeBone != -1) && (m_iREyeBone < m_nActiveBones)) {
            Vector3 LEyeOffset = AnimGroup.GetBone(m_iLEyeBone).bindTranslation - HeadBindPos;
            Vector3 REyeOffset = AnimGroup.GetBone(m_iREyeBone).bindTranslation - HeadBindPos;
            m_MidEyeOffset = HeadBindPos + ((LEyeOffset + REyeOffset) * 0.5f);
        } else {
            // Assume mid eye above head bone
            m_MidEyeOffset = HeadBindPos + Vector3(0.0f, 10.0f, 0.0f);
        }

        // Keep eye tracking info
        m_HeadL2W = pMatrices[m_iHeadBone];

        // Calculate "Eye" position - this might not actually be the eyes,
        // if we are forced to use the root bone...
        m_MidEyePosition = pMatrices[m_iHeadBone] * m_MidEyeOffset;
        m_AimAtOffset = m_MidEyePosition - GetPosition();
    }

    // Flag cache data as finally valid!
    m_CachedL2Ws.SetDirty(false);
#endif
}

//=========================================================================

void loco_char_anim_player::SetYawDelta(const Radian& YawDelta)
{
    // Specify exactly how far we WANT the animation to turn over a given frame

    // Mark cached L2W matrices as unusable
    //m_CachedL2Ws.SetDirty(true);

    m_YawStartFrame = 0;
    m_YawEndFrame = (float)m_AnimCurrTrack.GetNFrames();
    // look for a start and or end event.
    int c;
    for (c = 0; c < m_AnimCurrTrack.GetNEvents(); c++) {
        anim_event animEvent = m_AnimCurrTrack.GetEvent(c);
        if (!strcmp(animEvent.GetType(), "Generic")) {
            if (!strcmp(animEvent.GetString(anim_event::STRING_IDX_GENERIC_TYPE), "Yaw Begin")) {
                m_YawStartFrame = (float)animEvent.StartFrame();
            } else if (!strcmp(animEvent.GetString(anim_event::STRING_IDX_GENERIC_TYPE), "Yaw End")) {
                m_YawEndFrame = (float)animEvent.EndFrame();
            }
        }
    }
    float nSeconds = (m_YawEndFrame - m_YawStartFrame) / (float)m_AnimCurrTrack.GetAnimInfo().GetFPS();
    m_YawDelta = YawDelta / nSeconds;
}

//=========================================================================

// Returns the combined bbox of all the current playing animations
BBox loco_char_anim_player::ComputeBBox()
{
    // Start with empty bbox around origin
    BBox BBox;
    BBox.min.Zero();
    BBox.max.Zero();

    // Accumulate bounding boxes of all animations currently playing
    bool bAnimPlaying = false;
    for (int i = 0; i < LOCO_MAX_CHAR_ANIM_PLAYER_TRACKS; i++) {
        loco_anim_controller* pTrack = m_Track[i];
        if ((pTrack) && (pTrack->IsPlaying())) {
            bAnimPlaying = true;
            BBox += pTrack->GetBBox();
        }
    }

    // If no animations are currently playing, use bbox of all anims
    if (bAnimPlaying == false) {
        // Anim group loaded?
        const AnimGroup* pAnimGroup = GetAnimGroup();
        if (pAnimGroup) {
            // Grab bbox of all anims
            BBox = pAnimGroup->GetBBox();
        } else {
            // No anim group assigned, so use default bbox
            BBox.Set(Vector3(0.0f, 0.0f, 0.0f), 100.0f);
        }
    }

    return BBox;
}
