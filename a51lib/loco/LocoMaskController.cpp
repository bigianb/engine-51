#include "LocoMaskController.h"
#include <cassert>
#include "../VectorMath.h"

loco_mask_controller::loco_mask_controller(ResourceManager* rm) :
        loco_anim_controller(rm),
        m_BlendInTime       ( 0.25f ),  // Time to blend anim in
        m_BlendOutTime      ( 0.25f ),  // Time to blend anim out
        m_pCurrentBoneMasks ( nullptr ),   // Current bone masks
        m_pBlendBoneMasks   ( nullptr ),   // Bone masks to blend from
        m_BoneBlend         ( 0.0f ),   // 1 = "BlendBoneMasks", 0 = "CurrentBoneMasks"
        m_BoneBlendDelta    ( 0.0f )    // Decrements to 0
{
}

//=========================================================================

loco_mask_controller::~loco_mask_controller()
{
}

//=========================================================================
// Bone mask control functions
//=========================================================================

void loco_mask_controller::SetBoneMasks( const Geom::BoneMask& BoneMasks, float BlendTime )
{
    // Blend?
    if( ( m_pCurrentBoneMasks ) && ( BlendTime > 0.0f ) )
    {
        // Copy current to blend and start blending
        m_pBlendBoneMasks   = m_pCurrentBoneMasks;
        m_pCurrentBoneMasks = &BoneMasks;
        m_BoneBlend         = 1.0f;
        m_BoneBlendDelta    = -1.0f / BlendTime;
    }
    else
    {
        // Non blending
        m_pBlendBoneMasks   = nullptr;
        m_pCurrentBoneMasks = &BoneMasks;
        m_BoneBlend         = 0.0f;
    }
}

//=========================================================================
// Blend time control functions
//=========================================================================

void loco_mask_controller::SetBlendInTime( float Secs )
{
    assert(Secs >= 0) ;
    m_BlendInTime = Secs ;
}

//=========================================================================

void loco_mask_controller::SetBlendOutTime( float Secs )
{
    assert(Secs >= 0) ;
    m_BlendOutTime = Secs ;
}

//=========================================================================

void loco_mask_controller::MixKeys( const info& Info, AnimKey* pDestKey )
{
    // Mixes with masks the anims keyframes with the dest keyframes
    if( ( m_pCurrentBoneMasks ) && ( m_pBlendBoneMasks ) )
        MaskedMixKeys(Info, m_iAnim, m_Frame, *m_pCurrentBoneMasks, *m_pBlendBoneMasks, m_BoneBlend, pDestKey) ;
    else
    if( m_pCurrentBoneMasks )
        MaskedMixKeys(Info, m_iAnim, m_Frame, *m_pCurrentBoneMasks, pDestKey) ;
}

//=========================================================================

void loco_mask_controller::Advance( float DeltaTime, Vector3&  DeltaPos, Radian& DeltaYaw )
{
    // No animation?
    if ( m_iAnim == -1 )
        return;

    // Advance the base class
    loco_anim_controller::Advance( DeltaTime, DeltaPos, DeltaYaw ) ;

    // Force blend out?
    if( m_bIsBlendingOut )
    {
        // Lookup blend time or use default if non specified
        float BlendOutTime = 0.25f;
        if( m_BlendOutTime > 0.0f )    
            BlendOutTime = m_BlendOutTime;

        // Blend out the weight      
        m_Weight -= DeltaTime / BlendOutTime;      
    }
    else
    // Blend animation in/out?
    if( ( m_BlendInTime != 0.0f ) || ( m_BlendOutTime != 0.0f ) )
    {
        // Lookup info
        const AnimInfo& ai = GetAnimInfo();

        // Compute frame info
        float NFrames        = (float)std::max(1, m_nFrames-2) ;
        float FPS            = (float)ai.GetFPS();
        float BlendInFrames  = std::max(1.0f, m_BlendInTime  * FPS) ;
        float BlendOutFrames = std::max(1.0f, m_BlendOutTime * FPS) ;
        float BlendInFrame   = BlendInFrames ;
        float BlendOutFrame  = NFrames - BlendOutFrames ;

        // Only blend in on first cycle
        if ( (m_Cycle == 0) && (m_BlendInTime != 0.0f) && (m_Frame < BlendInFrame) )
        {
            // Blend in
            m_Weight = m_Frame / BlendInFrames ;
        }
        else
        // Only blend out if not looping
        if ( (!m_bLooping) && (m_BlendOutTime != 0.0f) && (m_Frame > BlendOutFrame) )
        {
            // Blend out
            m_Weight = 1.0f - ((m_Frame - BlendOutFrame) / BlendOutFrames) ;
        }
        else
        {
            // Full anim
            m_Weight = 1.0f ;
        }
    }

    // Range check
    if (m_Weight < 0)
        m_Weight = 0 ;
    else
    if (m_Weight > 1)
        m_Weight = 1 ;

    // Advance bone blends
    m_BoneBlend += m_BoneBlendDelta * DeltaTime;
    if( m_BoneBlend < 0.0f )
    {
        // End blending
        m_BoneBlend       = 0.0f;
        m_BoneBlendDelta  = 0.0f;
        m_pBlendBoneMasks = nullptr;
    }
}

//=========================================================================

// Blends out the mask controller
void loco_mask_controller::Stop(  )
{
    // Blend out
    m_bIsBlendingOut = true;
}

//=========================================================================
