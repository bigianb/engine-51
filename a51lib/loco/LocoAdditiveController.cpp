
#include "LocoAdditiveController.h"

#include <cassert>

loco_additive_controller::loco_additive_controller(ResourceManager *rm)
    : loco_anim_controller(rm)
{
    m_BlendInTime = 0.1f;  // Time to blend anim in
    m_BlendOutTime = 0.1f; // Time to blend anim out
}

loco_additive_controller::~loco_additive_controller()
{
}

//=========================================================================
// Blend time control functions
//=========================================================================
void loco_additive_controller::SetBlendInTime(float Secs)
{
    assert(Secs >= 0);
    m_BlendInTime = Secs;
}

//=========================================================================

void loco_additive_controller::SetBlendOutTime(float Secs)
{
    assert(Secs >= 0);
    m_BlendOutTime = Secs;
}

//=========================================================================

void loco_additive_controller::MixKeys(const info& Info, AnimKey* pDestKey)
{
    // Additively mixes the anims keyframes with the dest keyframes
    AdditiveMixKeys(Info, m_iAnim, m_Frame, 0, pDestKey);
}

//=========================================================================

void loco_additive_controller::Advance(float nSeconds, Vector3& DeltaPos, Radian& DeltaYaw)
{
    // No animation?
    if (m_iAnim == -1) {
        return;
    }

    // Advance the base class
    loco_anim_controller::Advance(nSeconds, DeltaPos, DeltaYaw);

    // Skip deltas
    DeltaPos.Zero();
    DeltaYaw = 0;

    // Lookup info
    const AnimInfo& animInfo = GetAnimInfo();

    // Compute frame info
    float NFrames = std::max(1.0f, (float)m_nFrames - 2);
    float FPS = (float)animInfo.GetFPS();
    float BlendInFrames = std::max(1.0f, m_BlendInTime * FPS);
    float BlendOutFrames = std::max(1.0f, m_BlendOutTime * FPS);
    float BlendInFrame = BlendInFrames;
    float BlendOutFrame = NFrames - BlendOutFrames;

    // Only blend in on first cycle
    if ((m_Cycle == 0) && (m_Frame < BlendInFrame)) {
        // Blend in
        m_Weight = m_Frame / BlendInFrames;
    } else
        // Only blend out if not looping
        if ((!m_bLooping) && (m_Frame > BlendOutFrame)) {
            // Blend out
            m_Weight = 1.0f - ((m_Frame - BlendOutFrame) / BlendOutFrames);
        } else {
            // Full anim
            m_Weight = 1.0f;
        }

    // Range check
    if (m_Weight < 0) {
        m_Weight = 0;
    } else if (m_Weight > 1) {
        m_Weight = 1;
    }
}
