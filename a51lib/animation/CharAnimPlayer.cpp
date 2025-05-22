
#include "animData.h"
#include "CharAnimPlayer.h"
#include "../xfiles/scratchMem.h"
#include "../render/Geom.h"

char_anim_player::char_anim_player(ResourceManager* rm) : m_hAnimGroup(rm), m_AnimTrack(rm)
{
    int i;

    // Clear track controller ptrs
    for (i = 0; i < MAX_CHAR_ANIM_PLAYER_TRACKS; i++) {
        m_Track[i] = NULL;
    }

    // Setup basic controller for track0
    SetTrackController(0, &m_AnimTrack);
    m_AnimTrack.SetManualYaw(false);

    // Clear all tracks
    ClearTracks();

    // Clear out other variables
    m_WorldPos.Zero();
    m_WorldRot.Zero();
    m_WorldScale = 1.0f;
    m_RenderOffset.Zero();
    m_SlideDelta.Zero();
    m_YawDelta = 0.0f;
    m_AnimHandleYaw = 0;
    m_BasisM.Identity();
    m_PreviousBasisM.Identity();
    m_pCachedL2W = NULL;
    m_CachedL2WIsDirty = true;
    m_nCachedL2WUpdates = 0;
    m_nCachedL2WDirties = 0;

    m_pCachedBonePos = NULL;
    m_CachedBonePosIsDirty = true;
    m_nCachedBonePosUpdates = 0;
    m_nCachedBonePosDirties = 0;
    m_bManualYaw = false;
    m_bRemoveTurnYaw = false;
    m_iMirrorBone = -1;
}

char_anim_player::~char_anim_player()
{
    free(m_pCachedL2W);
    free(m_pCachedBonePos);
}

void char_anim_player::SetAnimGroup(const AnimGroup::handle& hAnimGroup)
{
    m_hAnimGroup = hAnimGroup;

    assert(GetAnimGroup().GetNBones() <= MAX_ANIM_BONES);

    // Allocate cached L2W arrays
    free(m_pCachedL2W);
    m_pCachedL2W = (Matrix4*)malloc(sizeof(Matrix4) * GetAnimGroup().GetNBones());
    assert(m_pCachedL2W);
    DirtyCachedL2W();

    // Allocate cached bone position arrays
    free(m_pCachedBonePos);
    m_pCachedBonePos = (Vector3*)malloc(sizeof(Vector3) * GetAnimGroup().GetNBones());
    assert(m_pCachedBonePos);
    DirtyCachedBonePos();

    // Notify track controllers of anim group
    for (int i = 0; i < MAX_CHAR_ANIM_PLAYER_TRACKS; i++) {
        if (m_Track[i]) {
            m_Track[i]->SetAnimGroup(hAnimGroup);
        }
    }
}

void char_anim_player::SetAnimHoriz(int iAnim, float BlendTime)
{
    SetAnim(iAnim, false, true, BlendTime);
}

void char_anim_player::SetAnimVert(int iAnim, float BlendTime)
{
    SetAnim(iAnim, true, false, BlendTime);
}

void char_anim_player::SetAnim(int iAnim, bool ManualVert, bool ManualHoriz, float BlendTime, bool ResetFrameCount)
{
    // Lookup group
    const AnimGroup* pAnimGroup = m_hAnimGroup.getPointer();
    if (!pAnimGroup) {
        return;
    }

    // Select random anim
    iAnim = pAnimGroup->GetRandomAnimIndex(iAnim, m_AnimTrack.GetAnimIndex());

    // Mark cached L2W matrices as unusable
    DirtyCachedL2W();

    m_AnimTrack.SetAnim(iAnim, BlendTime, ResetFrameCount);
    m_AnimHandleYaw = m_AnimTrack.GetAnimInfo().GetHandleAngle();

    m_bManualVert = ManualVert;
    m_bManualHoriz = ManualHoriz;

    m_RenderOffset.Zero();
    m_SlideDelta.Zero();
    m_YawDelta = 0.0f;
}

void char_anim_player::SetAnimHoriz(const char* pAnimName, float BlendTime)
{
    SetAnim(GetAnimIndex(pAnimName), false, true, BlendTime);
}

void char_anim_player::SetAnimVert(const char* pAnimName, float BlendTime)
{
    SetAnim(GetAnimIndex(pAnimName), true, false, BlendTime);
}

void char_anim_player::SetManualYawControl(bool bIsManual)
{
    m_bManualYaw = bIsManual;

    m_AnimTrack.SetManualYaw(bIsManual);
}

void char_anim_player::SetOverrideRootBlend(bool bOverrideRootBlend)
{
    m_AnimTrack.SetOverrideRootBlend(bOverrideRootBlend);
}

void char_anim_player::SetRemoveTurnYaw(bool bRemoveTurnYaw)
{
    // Mark cached L2W matrices as unusable
    DirtyCachedL2W();

    // Keep
    m_bRemoveTurnYaw = bRemoveTurnYaw;

    // Tell all tracks
    for (int i = 0; i < MAX_CHAR_ANIM_PLAYER_TRACKS; i++) {
        if (m_Track[i]) {
            m_Track[i]->SetRemoveTurnYaw(bRemoveTurnYaw);
        }
    }
}

void char_anim_player::Advance(float nSeconds, Vector3& DeltaPos, Radian& DeltaYaw)
{
    // If completely frozen just return
    if (nSeconds == 0) {
        // but make sure the return data is valid!
        DeltaPos.Zero();
        DeltaYaw = 0;
        return;
    }

    // Mark cached L2W matrices as unusable
    DirtyCachedL2W();

    int             i;
    const AnimInfo& AnimData = m_AnimTrack.GetAnimInfo();

    //
    // Get first and last keys of Track0
    //
    AnimKey Key0;
    AnimKey Key1;
    AnimData.GetRawKey(0, 0, Key0);
    AnimData.GetRawKey(AnimData.GetNFrames() - 1, 0, Key1);

    //
    // Get root key from old frame
    //
    AnimKey OldKey;
    AnimData.GetInterpKey(m_AnimTrack.GetFrame(), 0, OldKey);

    //
    // Advance all tracks
    //
    for (i = 0; i < MAX_CHAR_ANIM_PLAYER_TRACKS; i++) {
        if (m_Track[i]) {
            m_Track[i]->Advance(nSeconds);
        }
    }

    //
    // Begin accumulating deltas
    //
    DeltaPos.Zero();
    DeltaYaw = 0;

    // Are we not in the same cycle?
    int OldCycle = m_AnimTrack.GetPrevCycle();
    int NewCycle = m_AnimTrack.GetCycle();

    // Catch up to current cycle
    while (OldCycle != NewCycle) {
        // Advance to end of this cycle
        DeltaPos += Key1.translation - OldKey.translation;
        DeltaYaw += x_MinAngleDiff(Key1.rotation.GetRotation().yaw, OldKey.rotation.GetRotation().yaw);
        OldKey = Key0;
        OldCycle++;
    }

    //
    // Get new key
    //
    AnimKey NewKey;
    AnimData.GetInterpKey(m_AnimTrack.GetFrame(), 0, NewKey);

    //
    // Compute deltas in animation space
    //
    DeltaPos += NewKey.translation - OldKey.translation;
    DeltaYaw += x_MinAngleDiff(NewKey.rotation.GetRotation().yaw, OldKey.rotation.GetRotation().yaw);

    // Scale deltapos
    DeltaPos *= m_WorldScale;

    // Rotate DeltaPos into world
    Matrix4 AnimToWorldM;
    ComputeAnimToWorldM(AnimToWorldM);
    DeltaPos = AnimToWorldM * DeltaPos;

    // Apply slide delta in world space
    DeltaPos += m_SlideDelta * nSeconds;

    Vector3 DeltaI;
    Vector3 DeltaJ;
    Vector3 DeltaK;
    CalcBasisVectors(DeltaPos, DeltaI, DeltaJ, DeltaK);
    DeltaPos.Zero();
    if (m_bManualVert) {
        DeltaPos += DeltaJ;
    }
    if (m_bManualHoriz) {
        DeltaPos += DeltaI;
        DeltaPos += DeltaK;
    }

    if (m_bManualYaw &&
        m_AnimTrack.GetNFrames() &&
        m_AnimTrack.GetFrame() > m_YawStartFrame &&
        m_AnimTrack.GetFrame() < m_YawEndFrame) {
        float fVal = (m_YawDelta)*nSeconds;

        DeltaYaw += fVal;
    }

    // There should be no delta yaw if we are removing turn yaw and it's not a turn anim playing
    if ((m_bRemoveTurnYaw) && (!m_AnimTrack.IsPlayingTurnAnim())) {
        DeltaYaw = 0;
    }
}

void char_anim_player::SetRotationAndPosition(const Matrix4& L2W)
{
    assert(L2W.IsValid());

    // Mark cached L2W matrices as unusable
    DirtyCachedL2W();

    m_WorldRot = L2W.GetRotation();
    m_WorldPos = L2W.GetTranslation();

    assert(abs(m_WorldRot.pitch) < 1000.0f);
    assert(abs(m_WorldRot.yaw) < 1000.0f);
    assert(abs(m_WorldRot.roll) < 1000.0f);
}

bool char_anim_player::GetPropL2W(const char* pPropName, Matrix4& L2W)
{

    const AnimInfo& AnimData = m_AnimTrack.GetAnimInfo();

    // Is this prop channel present?
    int PropChannel = AnimData.GetPropChannel(pPropName);
    if (PropChannel == -1) {
        L2W.Identity();
        return false;
    }

    // Get key from raw prop animation
    AnimKey PropKey;
    AnimData.GetPropInterpKey(PropChannel, m_AnimTrack.GetFrame(), PropKey);

    // Build final L2W for prop
    PropKey.Setup(L2W);

    // Put into world space if attached to a parent bone
    int iParent = AnimData.GetPropParentBoneIndex(PropChannel);
    if (iParent != -1) {
        const Matrix4& ParentM = GetCachedL2W(iParent);
        L2W = ParentM * L2W;
    }

    // return
    return true;
}

Vector3 char_anim_player::GetWorldAnimTranslation()
{
    Vector3 TotalTrans = m_AnimTrack.GetAnimInfo().GetTotalTranslation();
    TotalTrans *= m_WorldScale;

    // Rotate DeltaPos into world
    Matrix4 AnimToWorldM;
    ComputeAnimToWorldM(AnimToWorldM);
    TotalTrans = AnimToWorldM * TotalTrans;

    return TotalTrans;
}

void char_anim_player::PrepareRootKey(AnimKey& Key)
{
    // Get AnimToWorldM
    Matrix4 AnimToWorldM;
    ComputeAnimToWorldM(AnimToWorldM);

    // Copy root key info into temporary variables
    Vector3    RootTrans = Key.translation;
    Quaternion RootRot = Key.rotation;
    float      RootScale = m_WorldScale;

    // Scale root translation
    RootTrans *= RootScale;

    // Transform root translation and rotation into world
    RootTrans = AnimToWorldM * RootTrans;
    RootRot = (AnimToWorldM * (Matrix4)RootRot).GetQuaternion();

    Vector3 RootTransI;
    Vector3 RootTransJ;
    Vector3 RootTransK;
    CalcBasisVectors(RootTrans, RootTransI, RootTransJ, RootTransK);
    RootTrans = m_WorldPos;

    // Override root position if being manually controlled
    if (!m_bManualHoriz) {
        RootTrans += RootTransI;
        RootTrans += RootTransK;
    }

    if (!m_bManualVert) {
        RootTrans += RootTransJ;
    }

    // Transform render offset into world and add to root translation
    RootTrans += AnimToWorldM * m_RenderOffset;

    // Put data back into Key[0]
    Key.rotation = RootRot;
    Key.translation = RootTrans;
#if USE_SCALE_KEYS
    Key.Scale = Vector3(RootScale, RootScale, RootScale);
#endif
}

void char_anim_player::GetInterpKeys(AnimKey* pKey)
{
    int i;

    // Get original keys from track0
    m_AnimTrack.GetInterpKeys(pKey);

    // Modify root key before other tracks so they can respond
    PrepareRootKey(pKey[0]);

    // Loop through tracks and mix in
    for (i = 1; i < MAX_CHAR_ANIM_PLAYER_TRACKS; i++) {
        if (m_Track[i]) {
            m_Track[i]->MixKeys(pKey);
        }
    }
}

Vector3 char_anim_player::GetEventPosition(int iEvent)
{
    const anim_event& EV = m_AnimTrack.GetEvent(iEvent);

    //event_data eventData = EV.GetData();  // For debugging!

    const Matrix4& BoneM = GetCachedL2W(EV.GetInt(anim_event::INT_IDX_BONE));
    Vector3        P = BoneM * EV.GetPoint(anim_event::POINT_IDX_OFFSET);
    return P;
}

Radian3 char_anim_player::GetEventRotation(int iEvent)
{
    const anim_event& EV = m_AnimTrack.GetEvent(iEvent);

    const Matrix4& BoneM = GetCachedL2W(EV.GetInt(anim_event::INT_IDX_BONE));

    Vector3 ERot(EV.GetPoint(anim_event::POINT_IDX_ROTATION));
    Radian3 Rot(ERot.GetX(), ERot.GetY(), ERot.GetZ());

    Matrix4 EventRot(Rot);
    Matrix4 WorldRot = BoneM * EventRot;
    Rot = WorldRot.GetRotation();

    return Rot;
}

Vector3 char_anim_player::GetEventPosition(const anim_event& Event)
{
    // Get the world position.
    const Matrix4& BoneM = GetCachedL2W(Event.GetInt(anim_event::INT_IDX_BONE));
    Vector3        P = BoneM * Event.GetPoint(anim_event::POINT_IDX_OFFSET);
    return P;
}

Radian3 char_anim_player::GetEventRotation(const anim_event& Event)
{
    // Get Rotation.
    const Matrix4& BoneM = GetCachedL2W(Event.GetInt(anim_event::INT_IDX_BONE));

    Vector3 ERot(Event.GetPoint(anim_event::POINT_IDX_ROTATION));
    Radian3 Rot(ERot.GetX(), ERot.GetY(), ERot.GetZ());

    Matrix4 EventRot(Rot);
    Matrix4 WorldRot = BoneM * EventRot;
    Rot = WorldRot.GetRotation();

    return Rot;
}

void char_anim_player::ClearTracks()
{
    // Mark cached L2W matrices as unusable
    DirtyCachedL2W();

    for (int i = 1; i < MAX_CHAR_ANIM_PLAYER_TRACKS; i++) {
        if (m_Track[i]) {
            m_Track[i]->Clear();
        }
    }
}

void char_anim_player::ClearTrack(int iTrack)
{
    // Mark cached L2W matrices as unusable
    DirtyCachedL2W();

    assert((iTrack >= 0) && (iTrack < MAX_CHAR_ANIM_PLAYER_TRACKS));
    if (m_Track[iTrack]) {
        m_Track[iTrack]->Clear();
    }
}

void char_anim_player::SetTrackController(int iTrack, track_controller* pTrackController)
{
    // Mark cached L2W matrices as unusable
    DirtyCachedL2W();

    assert((iTrack >= 0) && (iTrack < MAX_CHAR_ANIM_PLAYER_TRACKS));
    m_Track[iTrack] = pTrackController;
    if (m_Track[iTrack]) {
        m_Track[iTrack]->Clear();
    }
}

void char_anim_player::SetSlideDelta(const Vector3& SlideDelta)
{
    // Mark cached L2W matrices as unusable
    DirtyCachedL2W();

    float nSeconds = (float)m_AnimTrack.GetNFrames() / (float)m_AnimTrack.GetAnimInfo().GetFPS();
    m_SlideDelta = SlideDelta / nSeconds;
}

void char_anim_player::SetYawDelta(const Radian& YawDelta)
{
    // Specify exactly how far we WANT the animation to turn over a given frame

    // Mark cached L2W matrices as unusable
    DirtyCachedL2W();

    m_YawStartFrame = 0;
    m_YawEndFrame = (float)m_AnimTrack.GetNFrames();
    // look for a start and or end event.
    int c;
    for (c = 0; c < m_AnimTrack.GetNEvents(); c++) {
        anim_event animEvent = m_AnimTrack.GetEvent(c);
        if (!strcmp(animEvent.GetType(), "Generic")) {
            if (!strcmp(animEvent.GetString(anim_event::STRING_IDX_GENERIC_TYPE), "Yaw Begin")) {
                m_YawStartFrame = (float)animEvent.StartFrame();
            } else if (!strcmp(animEvent.GetString(anim_event::STRING_IDX_GENERIC_TYPE), "Yaw End")) {
                m_YawEndFrame = (float)animEvent.EndFrame();
            }
        }
    }
    float nSeconds = (m_YawEndFrame - m_YawStartFrame) / (float)m_AnimTrack.GetAnimInfo().GetFPS();
    m_YawDelta = YawDelta / nSeconds;
}

void char_anim_player::ComputeAnimToWorldM(Matrix4& M)
{
    Radian3 Rot = m_WorldRot;
    Rot.yaw = m_WorldRot.yaw - m_AnimHandleYaw;
    if (m_bManualYaw) {
        const AnimInfo& AnimData = m_AnimTrack.GetAnimInfo();

        //
        // Get root key from old frame
        //
        AnimKey OldKey;
        AnimData.GetInterpKey(m_AnimTrack.GetFrame(), 0, OldKey);

        Rot.yaw -= OldKey.rotation.GetRotation().yaw;
    }
    M.Identity();
    M.SetRotation(Rot);
    M = m_BasisM * M;
}

void char_anim_player::CalcBasisVectors(const Vector3& world, Vector3& i, Vector3& j, Vector3& k)
{
    Vector3 I, J, K;
    Matrix4 TB;
    TB = m_BasisM;
    TB.GetColumns(I, J, K);
    TB.Transpose();

    Vector3 BasisSpace = TB * world;

    i = I * BasisSpace.GetX();
    j = J * BasisSpace.GetY();
    k = K * BasisSpace.GetZ();
}

void char_anim_player::DirtyCachedL2W()
{
    m_CachedL2WIsDirty = true;
    m_nCachedL2WDirties++;
    DirtyCachedBonePos();
}

const Matrix4* char_anim_player::GetCachedL2Ws()
{
    if (m_CachedL2WIsDirty) {
        UpdateCachedL2W();
    }

    return m_pCachedL2W;
}

const Matrix4& char_anim_player::GetCachedL2W(int iBone)
{
    assert((iBone >= 0) && (iBone < GetAnimGroup().GetNBones()));

    if (m_CachedL2WIsDirty) {
        UpdateCachedL2W();
    }

    return m_pCachedL2W[iBone];
}

void char_anim_player::ComputeBonesL2W(const Matrix4& L2W, AnimKey* pKey, Matrix4* pBoneL2W)
{
    int i;

    // Lookup anim group
    const AnimGroup& AnimGroup = GetAnimGroup();

    // Convert all keys to matrices and put into world space
    int nBones = AnimGroup.GetNBones();
    for (i = 0; i < nBones; i++) {
        // Lookup bone
        const AnimBone& Bone = AnimGroup.GetBone(i);

        // Setup L2W
        pKey[i].Setup(pBoneL2W[i]);

        // Concatenate with parent or L2W
        const Matrix4* PM = (Bone.iParent == -1) ? (&L2W) : (&pBoneL2W[Bone.iParent]);
        pBoneL2W[i] = (*PM) * pBoneL2W[i];

        // Mirror bone in local space?
        if (i == m_iMirrorBone) {
            // Compute local -> world matrix
            Matrix4 L2W;
            L2W.Setup(Vector3(m_WorldScale, m_WorldScale, m_WorldScale), m_WorldRot, m_WorldPos);

            // Compute world -> local matrix
            Matrix4 W2L = L2W;
            if (m_WorldScale != 1.0f) {
                W2L.InvertSRT();
            } else {
                W2L.InvertRT();
            }

            // Compute flip matrix
            Matrix4 F;
            F.Identity();
            F.SetScale(Vector3(-1, 1, 1));

            // Mirror bone in local space, then put back into world space
            pBoneL2W[i] = L2W * F * W2L * pBoneL2W[i];
        }
    }

    // Apply bind matrices
    for (i = 0; i < nBones; i++) {
        const AnimBone& Bone = AnimGroup.GetBone(i);
        pBoneL2W[i] = pBoneL2W[i] * Bone.bindMatrixInv;
    }
}

void char_anim_player::UpdateCachedL2W()
{
    if (!m_CachedL2WIsDirty) {
        return;
    }

    m_CachedL2WIsDirty = false;
    m_nCachedL2WUpdates++;

    // Allocate mix buffer
    AnimKey* MixBuffer = base_player::GetMixBuffer(base_player::MIX_BUFFER_PLAYER);
    assert(MixBuffer);

    // Get keys
    Matrix4 L2W;
    L2W.Identity();

    // Get keys
    GetInterpKeys(MixBuffer);

    // Build matrices
    ComputeBonesL2W(L2W, MixBuffer, m_pCachedL2W);
}

void char_anim_player::DirtyCachedBonePos()
{
    m_CachedBonePosIsDirty = true;
    m_nCachedBonePosDirties++;
}

const Vector3* char_anim_player::GetCachedBonePoss()
{
    if (m_CachedBonePosIsDirty) {
        UpdateCachedBonePos();
    }

    return m_pCachedBonePos;
}

const Vector3& char_anim_player::GetCachedBonePos(int iBone)
{
    assert((iBone >= 0) && (iBone < GetAnimGroup().GetNBones()));

    if (m_CachedBonePosIsDirty) {
        UpdateCachedBonePos();
    }

    return m_pCachedBonePos[iBone];
}

//=========================================================================

void char_anim_player::UpdateCachedBonePos()
{
    if (!m_CachedBonePosIsDirty) {
        return;
    }

    m_CachedBonePosIsDirty = false;
    m_nCachedBonePosUpdates++;

    const Matrix4*   pL2W = GetCachedL2Ws();
    const AnimGroup& animGroup = GetAnimGroup();
    int              nBones = animGroup.GetNBones();

    for (int i = 0; i < nBones; i++) {
        m_pCachedBonePos[i] = pL2W[i] * animGroup.GetBone(i).bindTranslation;
    }
}
