#pragma once

#include "LocoAnimController.h"

class loco_mask_controller : public loco_anim_controller
{
public:
    // Constructs a loco_mask_controller object.
    loco_mask_controller();

    // Destroys a loco_mask_controller object, handles cleanup and de-allocation.
    virtual ~loco_mask_controller();

    // Member functions
public:
    // Bone mask control functions
    float GetBoneWeight(int iBone);
    void  SetBoneMasks(const Geom::BoneMask& BoneMasks, float BlendTime = 0.5f);

    // Blend time control functions
    void SetBlendInTime(float Secs);
    void SetBlendOutTime(float Secs);

    // Mixes the anims keyframes into the dest keyframes
    virtual void MixKeys(const info& Info, AnimKey* pDestKey);

    // Advances animation and returns delta pos and delta yaw
    virtual void Advance(float DeltaTime, Vector3& DeltaPos, Radian& DeltaYaw);

    // Blends out the mask controller
    void Stop(void);

    // Member variables
protected:
    float m_BlendInTime;  // Time to blend anim in
    float m_BlendOutTime; // Time to blend anim out

    const Geom::BoneMask* m_pCurrentBoneMasks; // Current bone masks
    const Geom::BoneMask* m_pBlendBoneMasks;   // Bone masks to blend from
    float           m_BoneBlend;         // 1 = "BlendBoneMasks", 0 = "CurrentBoneMasks"
    float           m_BoneBlendDelta;    // Decrements to 0
};

//=========================================================================

inline float loco_mask_controller::GetBoneWeight(int iBone)
{
    // Compute bone weight and skip if it has no influence
    assert(m_pCurrentBoneMasks);
    float Weight = m_pCurrentBoneMasks->weights[iBone];
    if (m_pBlendBoneMasks) {
        Weight += m_BoneBlend * (m_pBlendBoneMasks->weights[iBone] - Weight);
    }
    Weight *= m_Weight;
    return Weight;
}
