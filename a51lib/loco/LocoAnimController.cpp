
#include "Loco.h"

//=========================================================================
// FUNCTIONS
//=========================================================================
loco_anim_controller::loco_anim_controller()
    : m_iAnim(-1)
    , // Index of current anim
    m_iAnimType(-1)
    , // Index that was passed into "SetAnim" call
    m_AnimFlags(0)
    , // Associated anim flags for playback
    m_nFrames(0)
    , // nFrames in animation
    m_EndFrameOffset(0)
    , // # of frames from end of anim to trigger it's finished
    m_Frame(0)
    , // Current modulated frame
    m_Cycle(0)
    , // Current Cycle, 0,1,2,3
    m_Weight(1)
    , // influence at mixing time
    m_PrevFrame(0)
    , // frame before Advance()
    m_PrevCycle(0)
    , // cycle before Advance()
    m_Rate(1)
    , // playback rate in frames per second
    m_bLooping(false)
    , // true if playing a looping anim

    m_bAccumYawMotion(false)
    , // true if delta Yaw motion should be extracted
    m_bAccumHorizMotion(false)
    , // true if delta XZ motion should be extracted
    m_bAccumVertMotion(false)
    , // true if delta Y motion should be extracted

    m_bRemoveYawMotion(false)
    , // true if yaw motion should be removed from motion bone
    m_bRemoveHorizMotion(false)
    , // true if horiz motion should be removed from motion bone
    m_bRemoveVertMotion(false)
    , // true if vert motion should be removed from motion bone

    m_bGravity(true)
    , // true if gravity should be applied
    m_bWorldCollision(true)
    , // true if world collision should happen

    m_bStartedOnMainTrack(false)
    , // true if anim has been started on the main track
    m_bIsBlendingOut(false)
    ,                      // true if anim is blending out
    m_bIsBlendingIn(false) // true if anim if blending in
{
}

//=========================================================================

loco_anim_controller::~loco_anim_controller(void)
{
}

//=========================================================================

void loco_anim_controller::SetAnimGroup(const AnimGroup::handle& hAnimGroup)
{
    // Set group
    m_hAnimGroup = hAnimGroup;

    // Clear vars
    Clear();
}

//=========================================================================

void loco_anim_controller::SetAnimGroup(const char* pAnimGroup)
{
    // Set group
    m_hAnimGroup.setName(pAnimGroup);

    // Clear vars
    Clear();
}

//=========================================================================

void loco_anim_controller::Clear(void)
{
    // Clear vars
    m_iAnim = -1;
    m_iAnimType = -1;
    m_AnimFlags = 0;
    m_nFrames = 0;
    m_EndFrameOffset = 0;
    m_Frame = 0;
    m_Cycle = 0;
    m_Weight = 0;
    m_PrevFrame = 0;
    m_PrevCycle = 0;
    m_Rate = 1.0f;
    m_bLooping = false;

    m_bAccumYawMotion = false;
    m_bAccumHorizMotion = true;
    m_bAccumVertMotion = false;

    m_bRemoveYawMotion = false;
    m_bRemoveHorizMotion = true;
    m_bRemoveVertMotion = false;

    m_bWorldCollision = true;
    m_bGravity = true;
}

//=========================================================================
// Anim group functions - returns info about animation package
//=========================================================================

const AnimGroup& loco_anim_controller::GetAnimGroup(void) const
{
    AnimGroup* pGroup = (AnimGroup*)m_hAnimGroup.getPointer();
    assert(pGroup);
    return *pGroup;
}

//=========================================================================

const AnimGroup::handle& loco_anim_controller::GetAnimGroupHandle(void) const
{
    return m_hAnimGroup;
}

//=========================================================================

const AnimInfo& loco_anim_controller::GetAnimInfo(int iAnim) const
{
    assert(iAnim != -1);
    const AnimGroup& AnimGroup = GetAnimGroup();
    return AnimGroup.GetAnimInfo(iAnim);
}

//=========================================================================

int loco_anim_controller::GetNAnims(void) const
{
    return GetAnimGroup().GetNAnims();
}
//=========================================================================

int loco_anim_controller::GetAnimIndex(const char* pAnimName) const
{
    return GetAnimGroup().GetAnimIndex(pAnimName);
}

//=========================================================================

int loco_anim_controller::GetNBones(void) const
{
    return GetAnimGroup().GetNBones();
}

//=========================================================================

const AnimBone& loco_anim_controller::GetBone(int iBone) const
{
    return GetAnimGroup().GetBone(iBone);
}

//=========================================================================

int loco_anim_controller::GetBoneIndex(const char* pBoneName) const
{
    return GetAnimGroup().GetBoneIndex(pBoneName);
}

//=========================================================================
// Animation settings functions
//=========================================================================

void loco_anim_controller::SetAnim(const AnimGroup::handle& hAnimGroup, int iAnim, uint32_t Flags)
{
    // Update group?
    // NOTE: This clears all anim vars to -1, 0, or NULL
    if (m_hAnimGroup.getPointer() != hAnimGroup.getPointer()) {
        SetAnimGroup(hAnimGroup);
    }

    // Keep flags
    m_AnimFlags = Flags;

    // Lookup group
    const AnimGroup& AnimGroup = GetAnimGroup();
    assert((iAnim >= 0) && (iAnim < AnimGroup.GetNAnims()));
    if ((iAnim < 0) || (iAnim >= AnimGroup.GetNAnims())) {
        return;
    }

    // If we are already playing this anim type index?
    if ((m_Weight != 0.0f) && (iAnim == m_iAnimType) && (m_iAnim != -1) && ((Flags & loco::ANIM_FLAG_RESTART_IF_SAME_ANIM) == 0)) {
        // If the animation is non looping and we are pegged at the end then restart
        if ((!m_bLooping) && (IsAtEnd())) {
            // Reset frame
            m_Frame = 0;
            m_Cycle = 0;

            // Set previous frame/cycle also
            m_PrevFrame = 0;
            m_PrevCycle = 0;
        }

        // Do not reset!
        return;
    }

    // Clear main track flag
    m_bStartedOnMainTrack = false;

    // Store the type index that was requested
    m_iAnimType = iAnim;

    // Select a new animation, skipping the current one?
    if ((Flags & loco::ANIM_FLAG_TURN_OFF_RANDOM_SELECTION) == 0) {
        iAnim = AnimGroup.GetRandomAnimIndex(iAnim, m_iAnim);
    }

    // Lookup anim info
    const AnimInfo& AnimInfo = AnimGroup.GetAnimInfo(iAnim);

    // Keep useful info
    m_iAnim = iAnim;
    m_nFrames = AnimInfo.GetNFrames();
    m_EndFrameOffset = AnimInfo.GetEndFrameOffset();
    m_bLooping = AnimInfo.DoesLoop();

    m_bAccumHorizMotion = m_bRemoveHorizMotion = AnimInfo.AccumHorizMotion();
    m_bAccumVertMotion = m_bRemoveVertMotion = AnimInfo.AccumVertMotion();
    m_bAccumYawMotion = m_bRemoveYawMotion = AnimInfo.AccumYawMotion();

    m_bGravity = AnimInfo.Gravity();
    m_bWorldCollision = AnimInfo.WorldCollision();

    // Clear blending status
    m_bIsBlendingIn = false;
    m_bIsBlendingOut = false;

    // Reset frame and cycle info
    if (Flags & loco::ANIM_FLAG_START_ON_SAME_FRAME) {
        m_Frame = (float)((int)m_Frame % (m_nFrames - 1));
    } else {
        m_Frame = 0;
    }
    m_Cycle = 0;
    m_PrevFrame = m_Frame;
    m_PrevCycle = 0;
}

//=========================================================================

void loco_anim_controller::SetAnim(const AnimGroup::handle& hAnimGroup, const char* pAnimName, uint32_t Flags)
{
    // Update group?
    if (m_hAnimGroup.getPointer() != hAnimGroup.getPointer()) {
        SetAnimGroup(hAnimGroup);
    }

    SetAnim(hAnimGroup, GetAnimIndex(pAnimName), Flags);
}

//=========================================================================
// Logic functions - advances animation and returns delta pos and delta yaw
//=========================================================================

void loco_anim_controller::Advance(float nSeconds, Vector3& DeltaPos, Radian& DeltaYaw)
{
    assert(m_Rate >= 0);

    // Clear deltas
    DeltaPos.Zero();
    DeltaYaw = 0;

    // No anim playing?
    if (m_iAnim == -1) {
        return;
    }

    // Skip if no rate (cinema audio driven) so that PrevCycle, PrevFrame are not touched (which would stop events from firing)
    if (m_Rate == 0.0f) {
        return;
    }

    // Get anim info
    const AnimInfo& AnimInfo = GetAnimInfo();

    // Remember previous frame and cycle
    m_PrevFrame = m_Frame;
    m_PrevCycle = m_Cycle;

    // Advance frame
    float nFrames = nSeconds * (float)AnimInfo.GetFPS() * m_Rate;
    m_Frame += nFrames;

    // Update which cycle we are in and modulate the frame
    while (m_Frame >= (m_nFrames - 1)) {
        m_Frame -= (m_nFrames - 1);
        m_Frame += AnimInfo.GetLoopFrame();
        m_Cycle++;
    }

    // If the anim doesn't loop and we are past the end then peg at the end
    if ((!m_bLooping) && ((m_Cycle > 0) || (m_Frame >= (m_nFrames - 2)))) {
        m_Cycle = 0;
        m_Frame = (float)(m_nFrames - 2);
    }
}

//=========================================================================
// Animation query functions - returns info on animation being played
//=========================================================================

const char* loco_anim_controller::GetAnimName(void) const
{
    // None playing?
    if (m_iAnim == -1) {
        return "NULL";
    }

    // Lookup name
    const AnimInfo& Info = GetAnimInfo();
    return Info.GetName();
}

//=========================================================================

int loco_anim_controller::GetLoopFrame(void) const
{
    // No anim playing?
    if (m_iAnim == -1) {
        return 0;
    }

    // Lookup loop to frame
    const AnimInfo& Info = GetAnimInfo();
    return Info.GetLoopFrame();
}

//=========================================================================

bool loco_anim_controller::IsAtEnd(void) const
{
    // -2 is because the last and first frames are the same so loops work.

    // Also take the end frame offset into account - this can be used to
    // flag the anim has ended early so that non-looping animations do not
    // "freeze" and look static when blending to the next animation
    int EndFrame = std::max(0, m_nFrames - 2 - m_EndFrameOffset);
    return ((m_Cycle > 0) || (m_Frame >= EndFrame));
}

//=========================================================================

bool loco_anim_controller::IsPlaying(const char* pAnimName) const
{
    if (m_iAnim == -1) {
        return false;
    }

    if (m_iAnimType == GetAnimIndex(pAnimName)) {
        return true;
    }

    return false;
}

//=========================================================================

bool loco_anim_controller::IsCinemaRelativeMode(void) const
{
    return (m_AnimFlags & loco::ANIM_FLAG_CINEMA_RELATIVE_MODE) != 0;
}

//=========================================================================

void loco_anim_controller::SetCinemaRelativeMode(bool bFlag)
{
    // Set or clear the relative flag?
    if (bFlag) {
        m_AnimFlags |= loco::ANIM_FLAG_CINEMA_RELATIVE_MODE;
    } else {
        m_AnimFlags &= ~loco::ANIM_FLAG_CINEMA_RELATIVE_MODE;
    }
}

//=========================================================================

bool loco_anim_controller::IsCoverRelativeMode(void) const
{
    return (m_AnimFlags & loco::ANIM_FLAG_COVER_RELATIVE_MODE) != 0;
}

//=========================================================================

void loco_anim_controller::SetCoverRelativeMode(bool bFlag)
{
    // Set or clear the relative flag?
    if (bFlag) {
        m_AnimFlags |= loco::ANIM_FLAG_COVER_RELATIVE_MODE;
    } else {
        m_AnimFlags &= ~loco::ANIM_FLAG_COVER_RELATIVE_MODE;
    }
}

//=========================================================================

void loco_anim_controller::SetFrame(float Frame)
{
    // Make sure frame is valid
    if (Frame < 0) {
        Frame = 0;
    } else {
        float MaxFrame = (float)m_nFrames - 2;
        if (Frame > MaxFrame) {
            Frame = MaxFrame;
        }
    }

    // Set it
    m_Frame = Frame;
}

//=========================================================================

void loco_anim_controller::SetTime(float Time)
{
    // No anim?
    if (m_iAnim == -1) {
        return;
    }

    // Convert time into frame and set
    int   FPS = GetFPS();
    float Frame = (float)FPS * Time;

    // Past the end of the anim?
    float NFrames = (float)GetNFrames() - 2.0f;
    if (Frame > NFrames) {
        // Fixup for looping?
        if (IsLooping()) {
            float LoopFrames = NFrames - (float)GetLoopFrame();
            m_PrevCycle = m_Cycle;
            m_Cycle = 0;
            while (Frame > NFrames) {
                Frame -= LoopFrames;
                m_Cycle++;
            }
        } else {
            // Clamp at end of anim
            Frame = NFrames;
        }
    }

    // Record so events fire off correctly
    m_PrevFrame = m_Frame;

    // Finally, set the frame
    SetFrame(Frame);
}

//=========================================================================

float loco_anim_controller::GetFrameParametric(void) const
{
    // NOTE: The -2 is because the last 2 frames are the same - they are used
    //       to calculate a correct delta for the positions eg. looping run anim
    //       DO NOT CHANGE THIS. If you use -1, then an incorrect delta gets calculated
    //       upon the loop of the animation, resulting in a position pop!

    float Frame = m_Frame / std::max(1, (m_nFrames - 2));

    // Range check
    if (Frame < 0) {
        Frame = 0;
    } else if (Frame > 1) {
        Frame = 1;
    }

    return Frame;
}

//=========================================================================

void loco_anim_controller::SetFrameParametric(float Frame)
{
    // Range check
    if (Frame < 0) {
        Frame = 0;
    } else if (Frame > 1) {
        Frame = 1;
    }

    // NOTE: The -2 is because the last 2 frames are the same - they are used
    //       to calculate a correct delta for the positions eg. looping run anim
    //       DO NOT CHANGE THIS. If you use -1, then an incorrect delta gets calculated
    //       upon the loop of the animation, resulting in a position pop!

    // Set
    m_Frame = Frame * ((float)m_nFrames - 2);
}

//=========================================================================

int loco_anim_controller::GetFPS(void) const
{
    if (m_iAnim == -1) {
        return 30;
    }

    const AnimInfo& AnimInfo = GetAnimGroup().GetAnimInfo(m_iAnim);
    return AnimInfo.GetFPS();
    ;
}

//=========================================================================

// Returns true if animation is upper body
bool loco_anim_controller::IsUpperBody(void) const
{
    // No anim?
    if (m_iAnim == -1) {
        return false;
    }

    // Declare so we can debug easier
    bool bUpperBody = ((m_AnimFlags & loco::ANIM_FLAG_MASK_TYPE_UPPER_BODY) != 0);
    return bUpperBody;
}

//=========================================================================

// Returns true if animation is full body
bool loco_anim_controller::IsFullBody(void) const
{
    // No anim?
    if (m_iAnim == -1) {
        return false;
    }

    // Declare so we can debug easier
    bool bFullBody = ((m_AnimFlags & loco::ANIM_FLAG_MASK_TYPE_FULL_BODY) != 0);
    return bFullBody;
}

//=========================================================================

bool loco_anim_controller::IsPlaying(void) const
{
    // Any anim?
    if (m_iAnim == -1) {
        return false;
    }

    // Are we blending in?
    if (m_bIsBlendingIn) {
        return true;
    }

    // Any weight?
    if (m_Weight <= 0.0f) {
        return false;
    }

    // Must be playing
    return true;
}

//=========================================================================
// Event functions
//=========================================================================

int loco_anim_controller::GetNEvents(void)
{
    if (m_iAnim == -1) {
        return 0;
    }

    if (m_Weight == 0) {
        return 0;
    }

    return GetAnimInfo().GetNEvents();
}

//=========================================================================
// Weight functions - controls the influence during the mixing process
//=========================================================================

void loco_anim_controller::SetWeight(float Weight)
{
    m_Weight = std::max(0.0f, std::min(1.0f, Weight));
}

//=========================================================================
// Key mixing functions
//=========================================================================

void loco_anim_controller::GetInterpKeys(const info& Info, AnimKey* pKey)
{
    int i;

    // Clear keys if no animation
    if (m_iAnim == -1) {
        for (i = 0; i < Info.m_nActiveBones; i++) {
            pKey[i].Identity();
        }

        return;
    }

    // If this assert fires off, then either the geometry and anim group have different numbers of bones,
    // ie. the geometry and anim group file are using a different bind pose matxs,
    // or this particular anim group has a different number of bones than an anim group used in
    // the same player ie. multiple anim groups used in this player have different bind pose matxs.
    // (this can happen when using "PlayAnim" with other anim group packages).
    // Either way - fix your resources!
    ////ASSERTS( (Info.m_nActiveBones <= GetAnimGroup().GetNBones()), "Incompatible anim group with bone lods!" ) ;

    // Lookup anim data
    const AnimInfo& AnimInfo = GetAnimInfo();

    // Grab the keys
    AnimInfo.GetInterpKeys(m_Frame, pKey, Info.m_nActiveBones);
}

//=========================================================================

void loco_anim_controller::MixKeys(const info& Info, AnimKey* pDestKey)
{
    //CONTEXT("loco_anim_controller::MixKeys") ;

    // If we aren't playing anything then just return
    if ((m_iAnim == -1) || (m_Weight == 0.0f)) {
        return;
    }

    // If this assert fires off, then either the geometry and anim group have different numbers of bones,
    // ie. the geometry and anim group file are using a different bind pose matxs,
    // or this particular anim group has a different number of bones than an anim group used in
    // the same player ie. multiple anim groups used in this player have different bind pose matxs.
    // (this can happen when using "PlayAnim" with other anim group packages).
    // Either way - fix your resources!
    //ASSERTS( (Info.m_nActiveBones <= GetAnimGroup().GetNBones()), "Incompatible anim group with bone lods!" ) ;

    int i;
    int nBones = Info.m_nActiveBones;

    // Allocate mix buffer
    AnimKey* MixBuffer = base_player::GetMixBuffer(base_player::MIX_BUFFER_CONTROLLER);
    assert(MixBuffer);

    // Read interpolated keys from the animation
    GetInterpKeys(Info, MixBuffer);

    // Check if this animation has bone masks
    if (GetAnimInfo().HasMasks()) {
        // Blend destination into track keys by weight amount
        for (i = 0; i < nBones; i++) {
            if (!GetAnimInfo().IsBoneMasked(i)) {
                pDestKey[i].Interpolate(pDestKey[i], MixBuffer[i], m_Weight);
            }
        }
    } else {
        // Blend destination into track keys by weight amount
        for (i = 0; i < nBones; i++) {
            pDestKey[i].Interpolate(pDestKey[i], MixBuffer[i], m_Weight);
        }
    }
}

//=========================================================================

void loco_anim_controller::AdditiveMixKeys(const info& Info, int iAnim, float Frame, int iRefFrame, AnimKey* pDestKey)
{
    //CONTEXT(" loco_anim_controller::AdditiveMixKeys") ;

    // If we aren't playing anything then just return
    if ((iAnim == -1) || (m_Weight == 0.0f)) {
        return;
    }

    // Get anim group
    const AnimGroup& AnimGroup = GetAnimGroup();

    // If this assert fires off, then either the geometry and anim group have different numbers of bones,
    // ie. the geometry and anim group file are using a different bind pose matxs,
    // or this particular anim group has a different number of bones than an anim group used in
    // the same player ie. multiple anim groups used in this player have different bind pose matxs.
    // (this can happen when using "PlayAnim" with other anim group packages).
    // Either way - fix your resources!
    //ASSERTS( (Info.m_nActiveBones <= AnimGroup.GetNBones()), "Incompatible anim group with bone lods!" ) ;

    // Lookup anim info
    const AnimInfo& AnimInfo = AnimGroup.GetAnimInfo(iAnim);

    // Lookup min and max bones used
    int iAnimBoneMin = AnimInfo.GetAnimBoneMinIndex();
    int iAnimBoneMax = AnimInfo.GetAnimBoneMaxIndex();

    // No additive animation present?
    if (iAnimBoneMin == -1) {
        assert(iAnimBoneMax == -1);
        return;
    }

    // Make sure anim compiler is working correctly
    assert(iAnimBoneMin >= 0);
    assert(iAnimBoneMax >= 0);
    assert(iAnimBoneMax >= iAnimBoneMin);

    // Bones not displayed?
    if (Info.m_nActiveBones <= iAnimBoneMin) {
        return;
    }

    // Take LOD into account
    iAnimBoneMax = std::min(Info.m_nActiveBones - 1, iAnimBoneMax);

    // Loop over all animated bones and mix keys
    for (int i = iAnimBoneMin; i <= iAnimBoneMax; i++) {
        // Skip if masked
        if (AnimInfo.IsBoneMasked(i)) {
            continue;
        }

        // Read reference key
        AnimKey InvRefKey;
        AnimInfo.GetRawKey(iRefFrame, i, InvRefKey);

        // Lookup current key
        AnimKey CurrKey;
        AnimInfo.GetInterpKey(Frame, i, CurrKey);

        // Skip if keys are the same
        if (InvRefKey.translation == CurrKey.translation) {
            // Get cosine of angle between rotations
            float CosAngle = (InvRefKey.rotation.x * CurrKey.rotation.x) +
                             (InvRefKey.rotation.y * CurrKey.rotation.y) +
                             (InvRefKey.rotation.z * CurrKey.rotation.z) +
                             (InvRefKey.rotation.w * CurrKey.rotation.w);

            // Skip if rotation is almost the same
            if (CosAngle > 0.99999999f) {
                continue;
            }
        }

        // Eyely applies the difference between reference frame and the current frame as follows:
        //
        // Variables:   CurrRot,   CurrPos   = Current key frame
        //              AnimRot,   AnimPos   = Animation key frame
        //              InvRefRot, InvRefPos = Inverse key frame of reference frame
        //
        //  Apply delta to current keys
        //          CurrRot = AnimRot * InvRefRot * CurrRot ;
        //          CurrPos = AnimPos + InvRefPos + CurrPos ;
        //
        //

        // Get inverse of reference key
        InvRefKey.rotation.Invert();
        InvRefKey.translation = -InvRefKey.translation;

        // Compute eye key
        AnimKey AddKey;
        AddKey.rotation = CurrKey.rotation * InvRefKey.rotation;
        AddKey.translation = CurrKey.translation + InvRefKey.translation;

        // Weighted?
        if (m_Weight != 1.0f) {
            AddKey.rotation = BlendToIdentity(AddKey.rotation, 1.0f - m_Weight);
            AddKey.translation *= m_Weight;
        }

        // Add delta on top of current keys
        pDestKey[i].rotation = AddKey.rotation * pDestKey[i].rotation;
        pDestKey[i].translation = AddKey.translation + pDestKey[i].translation;

    }
}

//=========================================================================

void loco_anim_controller::MaskedMixKeys(const info&     Info,
                                         int             iAnim,
                                         float           Frame,
                                         const Geom::BoneMask& BoneMasks,
                                         AnimKey*        pDestKey)
{
    //CONTEXT(" loco_anim_controller::MaskedMixKeys") ;

    // If we aren't playing anything then just return
    if ((iAnim == -1) || (m_Weight == 0.0f)) {
        return;
    }

    // Get anim group
    const AnimGroup& AnimGroup = GetAnimGroup();

    // Get bone counts from masks and take into account current LOD
    int NBones = std::min(BoneMasks.numBones, Info.m_nActiveBones);

    // Exit if nothing to do
    if (NBones == 0) {
        return;
    }

    // If this fires, the above bone count logic in incorrect
    assert(NBones <= Info.m_nActiveBones);

    // If this assert fires off, then either the geometry and anim group have different numbers of bones,
    // ie. the geometry and anim group file are using a different bind pose matxs,
    // or this particular anim group has a different number of bones than an anim group used in
    // the same player ie. multiple anim groups used in this player have different bind pose matxs.
    // (this can happen when using "PlayAnim" with other anim group packages).
    // Either way - fix your resources!
    //ASSERTS( (NBones <= AnimGroup.GetNBones()), "Incompatible anim group with bone lods!" ) ;

    // Lookup anim info
    const AnimInfo& AnimInfo = AnimGroup.GetAnimInfo(iAnim);

    // Loop over all bones and mix keys
    for (int i = 0; i < NBones; i++) {
        // Skip if masked
        if (AnimInfo.IsBoneMasked(i)) {
            continue;
        }

        // Compute bone weight and skip if it has no influence
        float Weight = BoneMasks.weights[i] * m_Weight;
        if (Weight == 0) {
            continue;
        }

        // Lookup key
        AnimKey Key;
        AnimInfo.GetInterpKey(Frame, i, Key);

        // Mix key
        pDestKey[i].Interpolate(pDestKey[i], Key, Weight);
    }
}

//=========================================================================

void loco_anim_controller::MaskedMixKeys(const info&     Info,
                                         int             iAnim,
                                         float           Frame,
                                         const Geom::BoneMask& CurrentBoneMasks,
                                         const Geom::BoneMask& BlendBoneMasks,
                                         float           BoneBlend,
                                         AnimKey*        pDestKey)
{
    //CONTEXT(" loco_anim_controller::MaskedMixKeys") ;

    // If we aren't playing anything then just return
    if ((iAnim == -1) || (m_Weight == 0.0f)) {
        return;
    }

    // Get anim group
    const AnimGroup& AnimGroup = GetAnimGroup();

    // Get bone counts from masks
    int NBones = std::max(CurrentBoneMasks.numBones, BlendBoneMasks.numBones);

    // Take into account current LOD
    NBones = std::min(NBones, Info.m_nActiveBones);

    // Exit if nothing to do
    if (NBones == 0) {
        return;
    }

    // If this fires, the above bone count logic in incorrect
    assert(NBones <= Info.m_nActiveBones);

    // If this assert fires off, then either the geometry and anim group have different numbers of bones,
    // ie. the geometry and anim group file are using a different bind pose matxs,
    // or this particular anim group has a different number of bones than an anim group used in
    // the same player ie. multiple anim groups used in this player have different bind pose matxs.
    // (this can happen when using "PlayAnim" with other anim group packages).
    // Either way - fix your resources!
    //ASSERTS( (NBones <= AnimGroup.GetNBones()), "Incompatible anim group with bone lods!" ) ;

    // Lookup anim info
    const AnimInfo& AnimInfo = AnimGroup.GetAnimInfo(iAnim);

    // Loop over all bones and mix keys
    for (int i = 0; i < NBones; i++) {
        // Skip if masked
        if (AnimInfo.IsBoneMasked(i)) {
            continue;
        }

        // Compute blended bone weight and skip if it has no influence
        float Weight = CurrentBoneMasks.weights[i];
        Weight += BoneBlend * (BlendBoneMasks.weights[i] - Weight);
        if (Weight == 0.0f) {
            continue;
        }

        // Lookup key
        AnimKey Key;
        AnimInfo.GetInterpKey(Frame, i, Key);

        // Mix key
        pDestKey[i].Interpolate(pDestKey[i], Key, Weight * m_Weight);
    }
}

//=========================================================================
