#include "Loco.h"

#include <cassert>

loco_motion_controller::loco_motion_controller(void)
    : loco_anim_controller()
    , m_iMotionProp(-1)
    , // Index to motion prop or -1 if none
    m_Yaw(0)
    , // Current yaw

    m_iMixAnim(-1)
    , // Index of mix animation (or -1 if none)
    m_Mix(0.0f)
    ,                    // Amount of mix animation (0 = none, 1 = all)
    m_MixAnimFrame(0.0f) // Current frame of mix animation
{
}

//=========================================================================

loco_motion_controller::~loco_motion_controller(void)
{
}

//=========================================================================
// Misc functions
//=========================================================================

void loco_motion_controller::Clear(void)
{
    // Call base class
    loco_anim_controller::Clear();

    // Clear
    m_iMotionProp = -1,
    m_Yaw = 0.0f;

    m_iMixAnim = -1;
    m_Mix = 0.0f;
    m_MixAnimFrame = 0.0f;
}

//=========================================================================

void loco_motion_controller::SetAnim(const AnimGroup::handle& hAnimGroup, int iAnim, uint32_t Flags)
{
    // No animation?
    if (iAnim == -1) {
        return;
    }

    // Call base class
    loco_anim_controller::SetAnim(hAnimGroup, iAnim, Flags);

    // Get anim info
    const AnimInfo& AnimInfo = GetAnimInfo();

    // Lookup motion prop
    m_iMotionProp = AnimInfo.GetPropChannel("MotionProp");
}

//=========================================================================

// Advance animation and extracts motion
void loco_motion_controller::Advance(const AnimInfo& AnimInfo,
                                     float           DeltaFrame,
                                     float&          Frame,
                                     float&          PrevFrame,
                                     int&            Cycle,
                                     int&            PrevCycle,
                                     Vector3&        DeltaPos,
                                     Radian&         DeltaYaw)
{
    // If motion prop is defined, then make sure it's in normal and mix anim!
    if (m_iMotionProp != -1) {
        assert(m_iMotionProp == AnimInfo.GetPropChannel("MotionProp"));
    }

    // Lookup info from anim
    int nFrames = AnimInfo.GetNFrames();

    // Clear deltas
    DeltaPos.Zero();
    DeltaYaw = 0;

    // Get first and last keys of motion
    AnimKey Key0;
    AnimKey Key1;
    GetMotionRawKey(AnimInfo, 0, Key0);
    GetMotionRawKey(AnimInfo, nFrames - 1, Key1);

    // Get root key from old frame
    AnimKey OldKey;
    GetMotionInterpKey(AnimInfo, Frame, OldKey);

    // Remember previous frame and cycle
    PrevFrame = Frame;
    PrevCycle = Cycle;

    // Advance frame
    Frame += DeltaFrame;

    // Update which cycle we are in and modulate the frame
    while (Frame >= (nFrames - 1)) {
        Frame -= (nFrames - 1);
        Frame += AnimInfo.GetLoopFrame();
        Cycle++;
    }

    // If the anim doesn't loop and we are past the end then peg at the end
    if ((!m_bLooping) && ((Cycle > 0) || (Frame >= (nFrames - 2)))) {
        PrevCycle = 0;
        Cycle = 0;
        Frame = (float)(nFrames - 2);
    }

    // Are we not in the same cycle?
    int OldCycle = PrevCycle;
    int NewCycle = Cycle;

    // Catch up to current cycle
    while (OldCycle < NewCycle) {
        // Advance to end of this cycle
        DeltaPos += Key1.translation - OldKey.translation;
        DeltaYaw += x_MinAngleDiff(Key1.rotation.GetRotation().yaw, OldKey.rotation.GetRotation().yaw);
        OldKey = Key0;
        OldCycle++;
    }

    // Get new key
    AnimKey NewKey;
    GetMotionInterpKey(AnimInfo, Frame, NewKey);

    // Compute deltas in animation space
    DeltaPos += NewKey.translation - OldKey.translation;
    DeltaYaw += x_MinAngleDiff(NewKey.rotation.GetRotation().yaw, OldKey.rotation.GetRotation().yaw);

    // Clear delta XZ if we are not accumulating it (it's in the anim)
    if (!m_bAccumHorizMotion) {
        DeltaPos.x = DeltaPos.z = 0;
    }

    // Clear delta Y if we are not accumulating it (it's in the anim)
    if (!m_bAccumVertMotion) {
        DeltaPos.y = 0;
    }

    // Clear delta yaw if not accumulating yaw
    if (!m_bAccumYawMotion) {
        DeltaYaw = 0;
    }

    // Start with the anim yaw
    Radian YawFix = GetRootBoneFixupYaw();

    // Since turns add to the yaw, I need to anti-rotate the translation by this amount
    // to keep it in sync!
    if (m_bRemoveYawMotion) {
        // Get yaw of current frame
        Radian CurrYaw = NewKey.rotation.GetRotation().yaw;

        // Using motion prop?
        if (m_iMotionProp != -1) {
            // Motion prop is along axis so remove it all
            YawFix -= CurrYaw + R_180;
        } else {
            // Get yaw of 1st frame
            Radian FirstYaw = Key0.rotation.GetRotation().yaw;

            // Remove what's been added so far!
            YawFix -= CurrYaw - FirstYaw;
        }
    }

    // Adjust yaw
    m_Yaw += DeltaYaw;

    // Rotate delta pos by current yaw
    DeltaPos.RotateY(YawFix);
}

//=========================================================================

void loco_motion_controller::Advance(float DeltaTime, Vector3& DeltaPos, Radian& DeltaYaw)
{
    assert(DeltaTime <= 0.1f);

    // No anim?
    if (m_iAnim == -1) {
        return;
    }

    // If no rate (cinema object can drive anim) then exit now so PrevCycle, PrevFrame are not messed with which breaks cinema events
    if (m_Rate == 0.0f) {
        return;
    }

    // Lookup anim group
    const AnimGroup& ag = GetAnimGroup();

    // Lookup main anim info
    const AnimInfo& ai = ag.GetAnimInfo(m_iAnim);
    float            nAnimFrames = (float)ai.GetNFrames() - 1;
    float            AnimFPS = (float)ai.GetFPS();
    float            AnimLength = nAnimFrames / AnimFPS;
    float            AnimDeltaFrame = DeltaTime * AnimFPS * m_Rate;

    // Mixing both anims?
    if (m_iMixAnim != -1) {
        // Lookup mix anim info
        const AnimInfo& MixAnimInfo = ag.GetAnimInfo(m_iMixAnim);
        float            nMixAnimFrames = (float)MixAnimInfo.GetNFrames() - 1;
        float            MixAnimFPS = (float)MixAnimInfo.GetFPS();
        float            MixAnimLength = nMixAnimFrames / MixAnimFPS;
        float            MixAnimDeltaFrame = DeltaTime * MixAnimFPS * m_Rate;

        // Get ratio of animations playback rate
        float Ratio = AnimLength / MixAnimLength;
        assert(Ratio > 0.0001f);

        // Compute main anim delta frame:
        //  when Mix=0, use DeltaFrame0
        //  when Mix=1, use DeltaFrame1
        float DeltaFrame0 = AnimDeltaFrame;
        float DeltaFrame1 = AnimDeltaFrame * Ratio;
        float DeltaFrame = DeltaFrame0 + (m_Mix * (DeltaFrame1 - DeltaFrame0));

        // Compute mix anim frame via matching the anims parametrically
        m_MixAnimFrame = nMixAnimFrames * (m_Frame / nAnimFrames);

        // Advance main animation and extract motion info
        Advance(ai,
                DeltaFrame,
                m_Frame,
                m_PrevFrame,
                m_Cycle,
                m_PrevCycle,
                DeltaPos,
                DeltaYaw);

        // Compute mix anim delta frame:
        //  when Mix=0, use DeltaFrame1
        //  when Mix=1, use DeltaFrame0
        DeltaFrame0 = MixAnimDeltaFrame;
        DeltaFrame1 = MixAnimDeltaFrame / Ratio;
        DeltaFrame = DeltaFrame1 + (m_Mix * (DeltaFrame0 - DeltaFrame1));

        // Extract motion info from mix animation
        float   PrevFrame = m_MixAnimFrame;
        int     Cycle = m_Cycle;
        int     PrevCycle = Cycle;
        Vector3 MixDeltaPos(0, 0, 0);
        Radian  MixDeltaYaw = 0;
        Advance(MixAnimInfo,
                DeltaFrame,
                m_MixAnimFrame,
                PrevFrame,
                Cycle,
                PrevCycle,
                MixDeltaPos,
                MixDeltaYaw);

        // Mix motion deltas into main deltas
        DeltaPos += m_Mix * (MixDeltaPos - DeltaPos);
        DeltaYaw += m_Mix * (x_MinAngleDiff(MixDeltaYaw, DeltaYaw));
    } else {
        // Compute main anim delta frame
        float DeltaFrame = DeltaTime * AnimFPS * m_Rate;

        // Advance main anim
        Advance(ai,
                DeltaFrame,
                m_Frame,
                m_PrevFrame,
                m_Cycle,
                m_PrevCycle,
                DeltaPos,
                DeltaYaw);
    }

    assert(abs(DeltaPos.GetX()) < (100.0f * 10.0f));
    assert(abs(DeltaPos.GetY()) < (100.0f * 10.0f));
    assert(abs(DeltaPos.GetZ()) < (100.0f * 10.0f));
}

//=========================================================================
// Root bone functions
//=========================================================================

Radian loco_motion_controller::GetRootBoneFixupYaw(void)
{
    // R_180 is because animations are exported 180 degrees out with the engine space!
    return m_Yaw + R_180;
}

//=========================================================================
// Yaw functions
//=========================================================================

Radian loco_motion_controller::GetStartYaw(void)
{
    // No anim?
    if (m_iAnim == -1) {
        return 0;
    }

    // Lookup anim data
    const AnimInfo& AnimInfo = GetAnimInfo();

    // Get root node key of frame 0
    AnimKey RootKey;
    AnimInfo.GetRawKey(0, 0, RootKey);

    // Get yaw
    Radian3 Rot = RootKey.rotation.GetRotation();
    return Rot.yaw;
}

//=========================================================================

float loco_motion_controller::GetMovementSpeed(void) const
{
    // No animation?
    if (m_iAnim == -1) {
        return 0.0f;
    }

    // Lookup main anim move speed
    const AnimInfo& ai = GetAnimInfo();
    float            MoveSpeed = ai.GetSpeed();

    // Using mix anim also?
    if ((m_iMixAnim != -1) && (m_Mix != 0.0f)) {
        // Lookup mix anim move speed
        const AnimInfo& MixAnimInfo = GetAnimInfo(m_iMixAnim);
        float            MixMoveSpeed = MixAnimInfo.GetSpeed();

        // Blend with main anim
        // 0 = All main anim
        // 1 = All mix anim
        MoveSpeed += (MixMoveSpeed - MoveSpeed) * m_Mix;
    }

    return MoveSpeed;
}

//=========================================================================

// Sets playback rate to match delta position (returns rate)
float loco_motion_controller::SetMatchingRate(const Vector3& DeltaPos, float DeltaTime, float RateMin, float RateMax)
{
    // No animation?
    if (m_iAnim == -1) {
        return 0.0f;
    }

    // Compute speeds
    float MoveSpeedSqr = x_sqr(DeltaPos.GetX()) + x_sqr(DeltaPos.GetZ());
    float AnimSpeed = GetMovementSpeed() * DeltaTime;

    // Update rate
    float Rate = 0.0f;
    if ((MoveSpeedSqr > x_sqr(0.000001f)) && (AnimSpeed > 0.000001f)) {
        // Compute rate
        float MoveSpeed = sqrt(MoveSpeedSqr);
        Rate = MoveSpeed / AnimSpeed;

        // Set valid rate
        m_Rate = Rate;

        // Range check for anim rate, but return computed rate
        m_Rate = std::min(m_Rate, RateMax);
        m_Rate = std::max(m_Rate, RateMin);
    }

    return Rate;
}

//=========================================================================
// Key mixing
//=========================================================================

void loco_motion_controller::FixRootBoneKey(const AnimInfo& AnimInfo, float Frame, AnimKey& RootBoneKey)
{
    // Lookup current key
    AnimKey MotionKey;
    GetMotionInterpKey(AnimInfo, Frame, MotionKey);

    // Remove XZ from animation?
    if (m_bRemoveHorizMotion) {
        RootBoneKey.translation.x -= MotionKey.translation.x;
        RootBoneKey.translation.z -= MotionKey.translation.z;
    }

    // Remove Y from animation?
    if (m_bRemoveVertMotion) {
        // Remove Y translation of the root bone
        RootBoneKey.translation.y -= MotionKey.translation.y;
    }

    // Remove yaw from animation?
    if (m_bRemoveYawMotion) {
        Radian DeltaYaw;
        Radian CurrYaw = MotionKey.rotation.GetRotation().yaw;

        // Using motion prop?
        if (m_iMotionProp != -1) {
            // Motion prop is along axis so remove it all
            DeltaYaw = -CurrYaw + R_180;
        } else {
            // Get 1st key
            AnimKey FirstKey;
            GetMotionRawKey(AnimInfo, 0, FirstKey);

            // Get yaw of first frame
            Radian FirstYaw = FirstKey.rotation.GetRotation().yaw;

            // Since root bone may not be down axis, use difference
            DeltaYaw = FirstYaw - CurrYaw;
        }

        // Make yaw be that of the 1st frame
        Quaternion DeltaRot(Vector3(0, 1, 0), DeltaYaw);
        RootBoneKey.rotation = DeltaRot * RootBoneKey.rotation;
        RootBoneKey.translation.RotateY(DeltaYaw);
    }
}

//=========================================================================

void loco_motion_controller::GetInterpKeys(const info& Info, AnimKey* pKey)
{
    int i;

    // Clear keys if no animation
    if (m_iAnim == -1) {
        for (i = 0; i < Info.m_nActiveBones; i++) {
            pKey[i].Identity();
        }

        return;
    }

    // Lookup anim group
    const AnimGroup& AnimGroup = GetAnimGroup();

    // If this assert fires off, then either the geometry and anim group have different numbers of bones,
    // ie. the geometry and anim group file are using a different bind pose matxs,
    // or this particular anim group has a different number of bones than an anim group used in
    // the same player ie. multiple anim groups used in this player have different bind pose matxs.
    // (this can happen when using "PlayAnim" with other anim group packages).
    // Either way - fix your resources!
    //ASSERTS((Info.m_nActiveBones <= AnimGroup.GetNBones()), "Incompatible anim group with bone lods!");

    // Grab main animation keys and fix root bone key
    const AnimInfo& ai = AnimGroup.GetAnimInfo(m_iAnim);
    ai.GetInterpKeys(m_Frame, pKey, Info.m_nActiveBones);
    FixRootBoneKey(ai, m_Frame, pKey[0]);

    // Blend in mix animation also?
    if ((m_iMixAnim != -1) && (m_Mix != 0.0f)) {
        // Get mixing buffer
        AnimKey* pMixAnimKeys = base_player::GetMixBuffer(base_player::MIX_BUFFER_TEMP);
        assert(pMixAnimKeys);

        // Grab mix animation keys and fix root bone key
        const AnimInfo& MixAnimInfo = AnimGroup.GetAnimInfo(m_iMixAnim);
        MixAnimInfo.GetInterpKeys(m_MixAnimFrame, pMixAnimKeys, Info.m_nActiveBones);
        FixRootBoneKey(MixAnimInfo, m_MixAnimFrame, pMixAnimKeys[0]);

        // Now blend the keys with the main anim
        for (i = 0; i < Info.m_nActiveBones; i++) {
            pKey[i].Interpolate(pKey[i], pMixAnimKeys[i], m_Mix);
        }
    }
}

//=========================================================================
// Motion functions
//=========================================================================

void loco_motion_controller::GetMotionRawKey(const AnimInfo& AnimInfo, int Frame, AnimKey& Key)
{
    assert(Frame >= 0);

    // If no motion prop, use the root bone
    if ((m_iMotionProp < 0) || (m_iMotionProp >= AnimInfo.GetNProps())) {
        AnimInfo.GetRawKey(Frame, 0, Key);
    } else {
        AnimInfo.GetPropRawKey(m_iMotionProp, Frame, Key);
    }
}

//=========================================================================

void loco_motion_controller::GetMotionInterpKey(const AnimInfo& AnimInfo, float Frame, AnimKey& Key)
{
    assert(Frame >= 0);

    // If no motion prop, use the root bone
    if ((m_iMotionProp < 0) || (m_iMotionProp >= AnimInfo.GetNProps())) {
        AnimInfo.GetInterpKey(Frame, 0, Key);
    } else {
        AnimInfo.GetPropInterpKey(m_iMotionProp, Frame, Key);
    }
}
