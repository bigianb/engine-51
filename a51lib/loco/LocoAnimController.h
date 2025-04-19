#pragma once

#include "../VectorMath.h"
#include "../animation/animData.h"
#include "../resourceManager/ResourceManager.h"
#include "../render/Geom.h"

#include <cassert>

#ifndef DEFAULT_BLEND_TIME
#define DEFAULT_BLEND_TIME  (0.125f)
#endif


class loco_char_anim_player ;


class loco_anim_controller
{
public:

    // Info that is available to controller when collecting/mixing keys
    struct info
    {
        int                     m_nActiveBones ;    // Number of bones to compute (for skeleton LODs)
        Matrix4                 m_Local2World ;     // Local->world of anim player
        Quaternion              m_Local2AnimSpace ; // Fixup rotation used by anim player
        Quaternion              m_Local2AimSpace ;  // Fixup rotation for correct aiming
        Matrix4                 m_HeadL2W ;         // Head local to world matrix
        Vector3                 m_MidEyePosition ;  // World position between eyes
    } ;


public:

//=========================================================================
// PUBLIC FUNCTIONS
//=========================================================================
public:
                loco_anim_controller    () ;
virtual        ~loco_anim_controller    () ;


        // Sets location of animation data package
virtual void                SetAnimGroup        ( const AnimGroup::handle& hAnimGroup ) ;
virtual void                SetAnimGroup        ( const char* pAnimGroup );

        // Clears the animation to a safe unused state
virtual void                Clear               () ;

        // Anim group functions - returns info about animation package
virtual const AnimGroup&           GetAnimGroup        () const ;
virtual const AnimGroup::handle&   GetAnimGroupHandle  () const ;
        const AnimInfo&            GetAnimInfo         ( int iAnim ) const ;
        int                         GetNAnims           () const ;
        int                         GetAnimIndex        ( const char* pAnimName ) const ;
        int                         GetNBones           () const ;
        const AnimBone&            GetBone             ( int iBone ) const ;
        int                         GetBoneIndex        ( const char* pBoneName ) const ;

        // Animation settings functions
virtual void                SetAnim             ( const AnimGroup::handle& hAnimGroup, int iAnim, uint32_t Flags = 0 ) ;
virtual void                SetAnim             ( const AnimGroup::handle& hAnimGroup, const char* pAnimName, uint32_t Flags = 0 ) ;

        // Logic functions - advances animation and returns delta pos and delta yaw
virtual void                Advance             ( float nSeconds, Vector3&  DeltaPos, Radian& DeltaYaw ) ;

        // Animation query functions - returns info on animation being played
        int                 GetAnimIndex        () const;     // Index of anim currently playing
        int                 GetAnimTypeIndex    () const;     // Index that was passed into "SetAnim" call
        uint32_t                 GetAnimFlags        () const;     // Flags for currently playing anim
        const char*         GetAnimName         () const;     // Name of anim currently playing
        int                 GetNFrames          () const;     // # of frames in currently playing anim
        int                 GetLoopFrame        () const;     // Frame to loop to for current anim
        const AnimInfo&    GetAnimInfo         () const;     // Info for currently playing anim
        bool               IsAtEnd             () const;     // TRUE if anim has fully played 
        bool               IsPlaying           ( const char* pAnimName ) const; // TRUE if this anim is playing
        float                 GetPrevFrame        () const;     // Previous frame before advance
        float                 GetFrame            () const;     // Current frame
        void                SetFrame            ( float Frame );      // Sets the current frame
        void                SetTime             ( float Time );       // Sets the current frame based on time
        float                 GetFrameParametric  () const;     // Current frame (0=start, 1=end)
        void                SetFrameParametric  ( float Frame ) ;     // Sets the current frame (0=start, 1=end)
        int                 GetPrevCycle        () const;     // Cycle before advance
        int                 GetCycle            () const;     // Current cycle
        void                SetCycle            ( int Cycle ) ;     // Sets current cycle
        int                 GetFPS              () const;     // Playback rate (in frames per second)
        void                SetRate             ( float Rate ) ;      // Adjust playback rate (1=normal)
        float                 GetRate             () const;     // Playback rate (1=normal)
        const BBox&         GetBBox             () const;     // BBox of currently playing anim
        
        void                SetLooping          ( bool bLooping ) ;    // Sets looping flags
        bool               IsUpperBody         () const ;        // Returns TRUE if animation is an upper body
        bool               IsFullBody          () const ;        // Returns TRUE if animation is full body
        bool               IsPlaying           () const;         // Returns TRUE if currently playing
        bool               IsLooping           () const { return m_bLooping;       }

        void                SetAccumYawMotion   ( bool bEnable );
        bool               GetAccumYawMotion   () const;
        void                SetAccumHorizMotion ( bool bEnable );
        bool               GetAccumHorizMotion () const;
        void                SetAccumVertMotion  ( bool bEnable );
        bool               GetAccumVertMotion  () const;

        void                SetRemoveYawMotion  ( bool bEnable );
        bool               GetRemoveYawMotion  () const;
        void                SetRemoveHorizMotion( bool bEnable );
        bool               GetRemoveHorizMotion() const;
        void                SetRemoveVertMotion ( bool bEnable );
        bool               GetRemoveVertMotion () const;

        void                SetGravity          ( bool bEnable );
        bool               GetGravity          () const;
        void                SetWorldCollision   ( bool bEnable );
        bool               GetWorldCollision   () const;

        void                SetStartedOnMainTrack   ( bool bStarted ) { m_bStartedOnMainTrack = bStarted; }
        bool               GetStartedOnMainTrack   () const { return m_bStartedOnMainTrack; } // TRUE if anim has been started on the main track

        bool               IsBlendingOut           () const { return m_bIsBlendingOut; } // Blending out status
        bool               IsBlendingIn            () const { return m_bIsBlendingIn;  } // Blending in status

        void                SetBlendingOut          ( bool bFlag ) { m_bIsBlendingOut = bFlag; } // Trigger blending out
        void                SetBlendingIn           ( bool bFlag ) { m_bIsBlendingIn = bFlag;  } // Trigger blending in

        bool               IsCinemaRelativeMode    () const;
        void                SetCinemaRelativeMode   ( bool bFlag );

        bool               IsCoverRelativeMode     () const;
        void                SetCoverRelativeMode    ( bool bFlag );

        // Event functions
        int                 GetNEvents          () ;          // # of event in current animation
        const anim_event&   GetEvent            ( int iEvent ) ;    // Animation event
        bool               IsEventActive       ( int iEvent ) ;    // TRUE if event should be fired
        bool               IsEventTypeActive   ( int Type ) ;      // TRUE if any event of type is active

        // Weight functions - controls the influence during the mixing process
        void                SetWeight           ( float Weight ) ;    // Set weight (0=off, 1=fully)
        float                 GetWeight           () ;          // Current weight
        
        // Key mixing functions
virtual void                GetInterpKeys       ( const info& Info, AnimKey* pKey ) ;      // Grabs interpolated keys
virtual void                MixKeys             ( const info& Info, AnimKey* pDestKey ) ; 
        void                AdditiveMixKeys     ( const info& Info, int iAnim, float Frame, int iRefFrame, AnimKey* pDestKey ) ;
        
        void                MaskedMixKeys       ( const info&               Info, 
                                                        int                 iAnim, 
                                                        float                 Frame, 
                                                  const Geom::BoneMask&   BoneMasks,
                                                        AnimKey*           pDestKey );

        void                MaskedMixKeys       ( const info&               Info, 
                                                        int                 iAnim, 
                                                        float                 Frame, 
                                                  const Geom::BoneMask&   CurrentBoneMasks,
                                                  const Geom::BoneMask&   BlendBoneMasks,
                                                        float                 BoneBlend,
                                                        AnimKey*           pDestKey );


//=========================================================================
// PRIVATE DATA
//=========================================================================
protected:

        AnimGroup::handle  m_hAnimGroup;       // Group of anims we are using
        int                 m_iAnim;            // Index of current anim
        int                 m_iAnimType ;       // Index that was passed into "SetAnim" call
        uint32_t                 m_AnimFlags;        // Associated anim flags for playback
        int16_t                 m_nFrames;          // # of frames in animation
        int16_t                 m_EndFrameOffset;   // # of frames from end of anim to trigger it's finished
        float                 m_Frame;            // Current modulated frame
        int                 m_Cycle;            // Current Cycle, 0,1,2,3
        float                 m_Weight;           // Influence at mixing time
        float                 m_PrevFrame;        // Frame before Advance()
        int                 m_PrevCycle;        // Cycle before Advance()
        float                 m_Rate;             // Playback rate in frames per second
        
        uint32_t                 m_bLooping            : 1,   // TRUE if playing a looping anim
                            m_bAccumYawMotion     : 1,   // TRUE if delta Yaw motion should be extracted
                            m_bAccumHorizMotion   : 1,   // TRUE if delta XZ motion should be extracted
                            m_bAccumVertMotion    : 1,   // TRUE if delta Y motion should be extracted
                            m_bRemoveYawMotion    : 1,   // TRUE if yaw motion should be removed from motion bone
                            m_bRemoveHorizMotion  : 1,   // TRUE if horiz motion should be removed from motion bone
                            m_bRemoveVertMotion   : 1,   // TRUE if vert motion should be removed from motion bone
                            m_bGravity            : 1,   // TRUE if gravity should be applied
                            m_bWorldCollision     : 1,   // TRUE if world collision should happen
                            m_bStartedOnMainTrack : 1,   // TRUE if anim has been started on the main track
                            m_bIsBlendingOut      : 1,   // TRUE if anim is blending out
                            m_bIsBlendingIn       : 1;   // TRUE if anim if blending in

//=========================================================================
// FRIENDS
//=========================================================================
friend class loco_char_anim_player;
friend class loco;
};

//=========================================================================
// INLINE FUNCTIONS
//=========================================================================

inline 
void loco_anim_controller::SetAccumYawMotion( bool bEnable )
{
    m_bAccumYawMotion = bEnable;
}

//=========================================================================

inline 
bool loco_anim_controller::GetAccumYawMotion() const
{
    return m_bAccumYawMotion;
}

//=========================================================================

inline 
void loco_anim_controller::SetAccumHorizMotion( bool bEnable )
{
    m_bAccumHorizMotion = bEnable;
}

//=========================================================================

inline 
bool loco_anim_controller::GetAccumHorizMotion() const
{
    return m_bAccumHorizMotion;
}

//=========================================================================

inline 
void loco_anim_controller::SetAccumVertMotion( bool bEnable )
{
    m_bAccumVertMotion = bEnable;
}

//=========================================================================

inline 
bool loco_anim_controller::GetAccumVertMotion() const
{
    return m_bAccumVertMotion;
}

//=========================================================================

inline 
void loco_anim_controller::SetRemoveYawMotion( bool bEnable )
{
    m_bRemoveYawMotion = bEnable;
}

//=========================================================================

inline 
bool loco_anim_controller::GetRemoveYawMotion() const
{
    return m_bRemoveYawMotion;
}

//=========================================================================

inline 
void loco_anim_controller::SetRemoveHorizMotion( bool bEnable )
{
    m_bRemoveHorizMotion = bEnable;
}

//=========================================================================

inline 
bool loco_anim_controller::GetRemoveHorizMotion() const
{
    return m_bRemoveHorizMotion;
}

//=========================================================================

inline 
void loco_anim_controller::SetRemoveVertMotion( bool bEnable )
{
    m_bRemoveVertMotion = bEnable;
}

//=========================================================================

inline 
bool loco_anim_controller::GetRemoveVertMotion() const
{
    return m_bRemoveVertMotion;
}

//=========================================================================

inline 
void loco_anim_controller::SetGravity( bool bEnable )
{
    m_bGravity = bEnable;
}

//=========================================================================

inline 
bool loco_anim_controller::GetGravity() const
{
    return m_bGravity;
}

//=========================================================================

inline 
void loco_anim_controller::SetWorldCollision( bool bEnable )
{
    m_bWorldCollision = bEnable;
}

//=========================================================================

inline 
bool loco_anim_controller::GetWorldCollision() const
{
    return m_bWorldCollision;
}

//=========================================================================

inline
int loco_anim_controller::GetAnimIndex() const
{
    return m_iAnim;
}

//=========================================================================

inline
int loco_anim_controller::GetAnimTypeIndex() const
{
    return m_iAnimType ;
}

//=========================================================================

inline
uint32_t loco_anim_controller::GetAnimFlags() const
{
    return m_AnimFlags;
}

//=========================================================================

inline
int loco_anim_controller::GetNFrames() const 
{
    return m_nFrames;
}

//=========================================================================

inline
const AnimInfo& loco_anim_controller::GetAnimInfo() const
{
    assert( m_iAnim != -1 );
    const AnimInfo& AnimInfo = GetAnimGroup().GetAnimInfo(m_iAnim) ;
    return AnimInfo ;
}

//=========================================================================

inline
float loco_anim_controller::GetPrevFrame() const 
{
    return m_PrevFrame;
}

//=========================================================================

inline
float loco_anim_controller::GetFrame() const 
{
    return m_Frame;
}

//=========================================================================

inline
int loco_anim_controller::GetPrevCycle() const 
{
    return m_PrevCycle;
}

//=========================================================================

inline
int loco_anim_controller::GetCycle() const 
{
    return m_Cycle;
}

//=========================================================================

inline
void loco_anim_controller::SetCycle( int Cycle )
{
    m_Cycle = Cycle;
}

//=========================================================================

inline
void loco_anim_controller::SetRate( float Rate )
{
    assert( m_Rate >= 0 );
    m_Rate = Rate;
}

//=========================================================================

inline
float loco_anim_controller::GetRate() const
{
    assert( m_Rate >= 0 );
    return m_Rate;
}

//=========================================================================

inline
const BBox& loco_anim_controller::GetBBox() const
{
    const AnimInfo& AnimInfo = GetAnimInfo();
    return AnimInfo.GetBBox();
}

//=========================================================================

inline
void loco_anim_controller::SetLooping( bool bLooping )
{
    m_bLooping = bLooping ; 
}

//=========================================================================

inline
const anim_event& loco_anim_controller::GetEvent( int iEvent ) 
{
    return GetAnimInfo().GetEvent( iEvent );
}

//=========================================================================

inline
bool loco_anim_controller::IsEventActive( int iEvent ) 
{
    return GetAnimInfo().IsEventActive( iEvent, m_Frame, m_PrevFrame);
}

//=========================================================================

inline
bool loco_anim_controller::IsEventTypeActive( int Type ) 
{
    return GetAnimInfo().IsEventTypeActive( Type, m_Frame, m_PrevFrame);
}

//=========================================================================

inline
float loco_anim_controller::GetWeight()
{
    return m_Weight;
}
