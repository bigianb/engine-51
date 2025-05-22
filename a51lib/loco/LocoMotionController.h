#pragma once

#include "LocoAnimController.h"

class loco_motion_controller : public loco_anim_controller
{
public:
    loco_motion_controller(ResourceManager* rm);
    virtual ~loco_motion_controller();

    // Misc functions

    // Clears the animation to a safe unused state
    virtual void Clear();

    // Sets a new animation and initializes the blend buffer
    virtual void SetAnim(const AnimGroup::handle& hAnimGroup, int iAnim, uint32_t Flags = 0);

private:
    // Advance animation and extracts motion
    void Advance(const AnimInfo& AnimInfo,
                 float           DeltaFrame,
                 float&          Frame,
                 float&          PrevFrame,
                 int&            Cycle,
                 int&            PrevCycle,
                 Vector3&        DeltaPos,
                 Radian&         DeltaYaw);

public:
    // Advances animation and returns delta pos and delta yaw
    virtual void Advance(float DeltaTime, Vector3& DeltaPos, Radian& DeltaYaw);

    // Root bone functions
    Radian GetRootBoneFixupYaw(void);

    // Yaw functions
    Radian GetStartYaw(void);
    void   ResetYaw(void);
    void   ApplyDeltaYaw(Radian DeltaYaw);
    void   SetYaw(Radian Yaw);
    Radian GetYaw(void) const;

    // Returns info about animation playing
    float GetMovementSpeed(void) const;

    // Sets playback rate to match delta position (returns rate)
    float SetMatchingRate(const Vector3& DeltaPos, float DeltaTime, float RateMin, float RateMax);

    // Key mixing
    void         FixRootBoneKey(const AnimInfo& AnimInfo, float Frame, AnimKey& RootBoneKey);
    virtual void GetInterpKeys(const info& Info, AnimKey* pKey);

    // Motion functions
    bool IsUsingMotionProp(void) const { return m_iMotionProp != -1; }
    void GetMotionRawKey(const AnimInfo& AnimInfo, int Frame, AnimKey& Key);
    void GetMotionInterpKey(const AnimInfo& AnimInfo, float Frame, AnimKey& Key);

    // Mix animation functions
    void SetMixAnim(int iMixAnim, float Mix);

    //=====================================================================
    // DATA
    //=====================================================================

protected:
    // Motion vars
    int    m_iMotionProp; // Index to motion prop or -1 if none
    Radian m_Yaw;         // Current yaw

    // Mix animation vars
    int   m_iMixAnim;     // Index of mix animation (or -1 if none)
    float m_Mix;          // Amount of mix animation (0 = none, 1 = all)
    float m_MixAnimFrame; // Current frame of mix animation

    //=====================================================================
    // FRIENDS
    //=====================================================================
    friend class loco_char_anim_player;
};

//=========================================================================
// INLINES
//=========================================================================

inline void loco_motion_controller::ResetYaw(void)
{
    m_Yaw = GetStartYaw();
}

//=========================================================================

inline void loco_motion_controller::ApplyDeltaYaw(Radian DeltaYaw)
{
    m_Yaw += DeltaYaw;
}

//=========================================================================

inline void loco_motion_controller::SetYaw(Radian Yaw)
{
    m_Yaw = Yaw;
}

//=========================================================================

inline Radian loco_motion_controller::GetYaw(void) const
{
    return m_Yaw;
}

//=========================================================================
// Mix animation functions
//=========================================================================

inline void loco_motion_controller::SetMixAnim(int iMixAnim, float Mix)
{
    // Store info
    m_iMixAnim = iMixAnim;
    m_Mix = Mix;
}
