#pragma once
#include "LocoMaskController.h"

class loco;

class loco_aim_controller : public loco_mask_controller
{
public:
    // Functions
    loco_aim_controller();

    void SetBlendFactor(float wSide, float wUpDown, float BlendSpeed = 0.5f);
    void ApplyDeltaHoriz(float Delta);

    virtual void Clear();
    virtual void Advance(float DeltaTime, Vector3& DeltaPos, Radian& DeltaYaw);
    virtual void MixKeys(const info& Info, AnimKey* pDestKey);

    const AnimInfo& GetAnimInfo(int iSide);

    // Manual (coded) aiming control
    void   SetHorizLimits(Radian HorizMinLimit, Radian HorizMaxLimit);
    void   SetVertLimits(Radian VertMinLimit, Radian VertMaxLimit);
    Radian GetHorizMinLimit() { return m_HorizMinLimit; }
    Radian GetHorizMaxLimit() { return m_HorizMaxLimit; }
    Radian GetVertMinLimit() { return m_VertMinLimit; }
    Radian GetVertMaxLimit() { return m_VertMaxLimit; }

    // Returns current and target aiming angles
    Radian GetHorizAim() const;
    Radian GetVertAim() const;

    Radian GetTargetHorizAim() const;
    Radian GetTargetVertAim() const;

    // Weight blending
    void SetWeight(float Weight, float BlendTime = 0.0f);
    void SetBoneMasks(const Geom::BoneMask& VertBoneMasks,
                      const Geom::BoneMask& HorizBoneMasks,
                      float           BlendTime = 0.5f);

protected:
    float m_wSide;   // Positive is right negative is left
    float m_wUpDown; // Positive is up negative is down

    float m_BlendSpeed;    // Speed to get to target
    float m_wTargetSide;   // Target side to blend to
    float m_wTargetUpDown; // Target up/down to blend to

    Radian m_HorizMinLimit, m_HorizMaxLimit; // Horizontal angle limits
    Radian m_VertMinLimit, m_VertMaxLimit;   // Vertical angle limits

    float m_WeightBlendDelta; // Blends weight in and out

    const Geom::BoneMask* m_pCurrentBoneMasksHoriz; // Current bone masks
    const Geom::BoneMask* m_pBlendBoneMasksHoriz;   // Bone masks to blend from

    friend class loco;
};
