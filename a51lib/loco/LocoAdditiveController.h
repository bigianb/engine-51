#pragma once

#include "LocoAnimController.h"

class loco_additive_controller : public loco_anim_controller
{
public:
    // Constructs a loco_additive_controller object.
    loco_additive_controller();

    // Destroys a loco_additive_controller object, handles cleanup and de-allocation.
    virtual ~loco_additive_controller();

    // Member functions
public:
    // Blend time control functions
    void SetBlendInTime(float Secs);
    void SetBlendOutTime(float Secs);

    // Mixes the anims keyframes into the dest keyframes
    virtual void MixKeys(const info& Info, AnimKey* pDestKey);

    // Advances animation and returns delta pos and delta yaw
    virtual void Advance(float nSeconds, Vector3& DeltaPos, Radian& DeltaYaw);

    // Member variables
protected:
    float m_BlendInTime;  // Time to blend anim in
    float m_BlendOutTime; // Time to blend anim out
};
