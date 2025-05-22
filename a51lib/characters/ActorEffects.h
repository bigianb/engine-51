#pragma once

#include "../objects/Object.h"
#include "../fx/fx_Mgr.h"

//==============================================================================

#define MAX_FRY_POINTS      8   // Flame and shock effect.
#define MAX_CLOAK_POINTS   11   // Various clocking effects.

class actor;
class ResourceManager;
//==============================================================================

class actor_effects
{

//------------------------------------------------------------------------------
public:

        enum effect_type
        {
            FX_FLASHLIGHT = 0,  // Must be 0.
            FX_FLAME,
            FX_SHOCK,
            FX_MUTATE,
            FX_UNMUTATE,
            FX_SPAWN,
            FX_CLOAK,
            FX_DECLOAK,
            FX_CLOAK_PAIN,
            FX_CONTAIGON,

            FX_MAX,
            FX_FIRST = 0,
        };

                    actor_effects       (ResourceManager* pRM);
                   ~actor_effects       ();

        void        Init                ();
        void        Kill                ();
        void        UpdateFlashLight    ( effect_type Type, Object* pParent );
        void        Update              ( Object* pParentObj, float DeltaTime );
        void        Render              ( Object* pParentObj );   
        void        RenderTransparent   ( Object* pParentObj, float Alpha = 1.0f );

        bool       IsActive            ();

        void        InitEffect          ( effect_type Type, Object* pParent = NULL, int iBone = -1 );
        void        KillEffect          ( effect_type Type );
        bool       IsEffectOn          ( effect_type Type );

        void        SetDeathTimer       ( float DeathTimer );
        float         GetDeathTimer       () { return( m_DeathTimer ); }

        void        SetShockTimer       ( float Timer );        

//------------------------------------------------------------------------------
protected:

        void            InitBasicEffect (       effect_type   Type,
                                          const Vector3&      Position,
                                                int           Zone,
                                          const char*         pEffectName,
                                          const char*         pAudioName );

        void            InitCloakEffect (       effect_type   Type,
                                                Object*       pParent, 
                                          const char*         pEffectName,
                                                int           iBone = -1 );

        void            InitFryEffect   (       effect_type   Type,
                                                Object*       pParent, 
                                          const char*         pEffectName,
                                          const char*         pAudioName );

        void            KillFryEffect   ();

const   Vector3         GetBonePosition ( Object* pParent, int iBone );
        void            GetBoneL2W      ( Object* pParent, int iBone, Matrix4& L2W );

        struct fx_bone
        { 
            int       iBone;
            fx_handle FXHandle;
        };

        float             m_DeltaTime;    // To avoid repeated arg passing.
        float             m_DeathTimer;
        float             m_ShockTimer;

        bool           m_bActive     [ FX_MAX ];
        fx_handle       m_FXHandle    [ FX_MAX ];
        int             m_AudioID     [ FX_MAX ];

        int             m_nFryPoints;

        fx_bone         m_FryBone     [ MAX_FRY_POINTS   ];
        fx_bone         m_CloakBone   [ MAX_CLOAK_POINTS ];

        ResourceManager* resourceManager;
};
