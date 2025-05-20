
#include "ActorEffects.h"
#include "../objectManager/ObjectManager.h"
#include "../objectManager/ObjectPtr.h"
#include "../collisionMgr/CollisionMgr.h"
#include "../objects/Actor.h"
#include "../objects/Corpse.h"
#include "../objects/NewWeapon.h"
//#include "GameLib\RenderContext.hpp"
#include "../objects/Player.h"
//#include "MutantTank/Mutant_Tank.hpp"

//#include "..\Support\Sound\EventSoundEmitter.hpp"

actor_effects::actor_effects()
{
    for (int i = 0; i < FX_MAX; i++) {
        m_bActive[i] = false;
        m_AudioID[i] = 0;
    }

    for (int i = 0; i < MAX_FRY_POINTS; i++) {
        m_FryBone[i].iBone = -1;
    }

    for (int i = 0; i < MAX_CLOAK_POINTS; i++) {
        m_CloakBone[i].iBone = -1;
    }

    m_DeathTimer = 0.0f;
    m_ShockTimer = 0.0f;
}

//==============================================================================

actor_effects::~actor_effects()
{
    Kill();
}

//==============================================================================

void actor_effects::Init()
{
}

//==============================================================================

void actor_effects::Kill()
{
    for (int i = 0; i < FX_MAX; i++) {
        KillEffect((effect_type)i);
    }
}

//==============================================================================

void actor_effects::InitEffect(effect_type Type, Object* pParent, int iBone)
{
    switch (Type) {
    //--------------------------------------------------------------------------
    case FX_FLASHLIGHT:
        //--------------------------------------------------------------------------
        assert(pParent);
        InitBasicEffect(Type,
                        pParent->GetPosition(),
                        pParent->GetZone1(),
                        "FlashLight_000.fxo",
                        nullptr);

        // Set correct position
        UpdateFlashLight(Type, pParent);
        break;

    //--------------------------------------------------------------------------
    case FX_FLAME:
        //--------------------------------------------------------------------------
        InitFryEffect(Type,
                      pParent,
                      "jm_fire_002.fxo",
                      "FIRE_NPC_LOOP");
        break;

    //--------------------------------------------------------------------------
    case FX_SHOCK:
        //--------------------------------------------------------------------------
        InitFryEffect(Type,
                      pParent,
                      "Actor_effect_Electric.fxo",
                      "FIRE_NPC_LOOP"); // TO DO - Fire sound?
        break;

    //--------------------------------------------------------------------------
    case FX_MUTATE:
    {
        //--------------------------------------------------------------------------
        assert(pParent);
        InitBasicEffect(Type,
                        pParent->GetPosition(),
                        pParent->GetZone1(),
                        "MP_MUTATION.fxo",
                        nullptr);
        break;
        //--------------------------------------------------------------------------
    }

    case FX_UNMUTATE:
    {
        //--------------------------------------------------------------------------
        assert(pParent);

        InitBasicEffect(Type,
                        pParent->GetPosition(),
                        pParent->GetZone1(),
                        "MP_MUTATION.fxo",
                        nullptr);
        break;

        //--------------------------------------------------------------------------
    }
    case FX_SPAWN:
        //--------------------------------------------------------------------------
        assert(pParent);
        KillEffect(Type);
        InitBasicEffect(Type,
                        pParent->GetPosition(),
                        pParent->GetZone1(),
                        "MP_Spawn_Shield.fxo",
                        "JumpPad_Launch");
        break;

    //--------------------------------------------------------------------------
    case FX_CLOAK:
        //--------------------------------------------------------------------------
        InitCloakEffect(Type,
                        pParent,
                        "blackops_cloak.fxo");
        break;

    //--------------------------------------------------------------------------
    case FX_DECLOAK:
        //--------------------------------------------------------------------------
        InitCloakEffect(Type,
                        pParent,
                        "blackops_decloak.fxo");
        break;

    //--------------------------------------------------------------------------
    case FX_CLOAK_PAIN:
        //--------------------------------------------------------------------------
        InitCloakEffect(Type,
                        pParent,
                        "blackops_impact.fxo",
                        iBone);
        break;

    //--------------------------------------------------------------------------
    case FX_CONTAIGON:
        //--------------------------------------------------------------------------
        InitFryEffect(Type,
                      pParent,
                      "Contagion_Actor.fxo",
                      ""); // TO DO - Fire sound?
        break;

    //--------------------------------------------------------------------------
    default:
        //--------------------------------------------------------------------------
        assert(false);
        break;
    }
}

//==============================================================================

void actor_effects::InitBasicEffect(effect_type    Type,
                                    const Vector3& Position,
                                    int            Zone,
                                    const char*    pEffectName,
                                    const char*    pAudioName)
{
    if (m_bActive[Type]) {
        return;
    }

    ResourceHandle<char> Resource;
    Resource.setName(pEffectName);

    char* pPtr = Resource.getPointer();
    assert(pPtr);
    if (!pPtr) {
        return;
    }

    m_FXHandle[Type].InitInstance(pPtr);
    m_FXHandle[Type].Restart();
    m_FXHandle[Type].SetTranslation(Position);

    if (pAudioName) {
        if (m_AudioID[Type] != 0) {
            // IJB: g_AudioMgr.Release(m_AudioID[Type], 0.0f);
            m_AudioID[Type] = 0;
        }
/* IJB
        m_AudioID[Type] = g_AudioMgr.Play(pAudioName,
                                          Position,
                                          Zone,
                                          true);
                */
    }

    m_bActive[Type] = true;
}

//==============================================================================

void actor_effects::InitFryEffect(effect_type Type,
                                  Object*     pParent,
                                  const char* pEffectName,
                                  const char* pAudioName)
{
    KillFryEffect();

    if (pParent && pParent->IsKindOf(actor::GetRTTI())) {
        actor* pActor = (actor*)pParent;

        ResourceHandle<char> Resource;
        Resource.setName(pEffectName);

        // Need the parent actor loco player.
        assert(pActor->GetLocoPointer());
        loco_char_anim_player& Player = pActor->GetLocoPointer()->m_Player;

        // Need a range of bones on which to add flames.
        int nBones = std::min(Player.GetNBones(), 16);
        m_nFryPoints = std::min(nBones, MAX_FRY_POINTS);

        // Fire it up!  Literally.
        for (int i = 0; i < m_nFryPoints; i++) {
            m_FryBone[i].iBone = x_irand(0, nBones - 1);
            m_FryBone[i].FXHandle.InitInstance(Resource.getPointer());

            if (Type == FX_SHOCK) {
                float Scale = x_frand(0.4f, 0.7f);
                m_FryBone[i].FXHandle.SetScale(Vector3(Scale, Scale, Scale));
            }
            if (Type == FX_FLAME) {
                m_FryBone[i].FXHandle.SetScale(Vector3(0.4f, 0.4f, 0.4f));
            }

            if (Type == FX_CONTAIGON) {
                // Randomize rotation for contaigon
                Radian P, Y;
                P = x_frand(0, R_360);
                Y = x_frand(0, R_360);
                m_FryBone[i].FXHandle.SetRotation(Radian3(P, Y, 0));

                /* IJB
                if (pParent->IsKindOf(mutant_tank::GetRTTI())) {
                    m_FryBone[i].FXHandle.SetScale(Vector3(2, 2, 2));
                }
                    */
            }
        }
    }

    if (m_AudioID[Type] != 0) {
        // IJB: g_AudioMgr.Release(m_AudioID[Type], 0.0f);
        m_AudioID[Type] = 0;
    }

    /* IJB
    if (pAudioName && pParent) {
        m_AudioID[Type] = g_AudioMgr.PlayVolumeClipped(pAudioName,
                                                       pParent->GetPosition(),
                                                       pParent->GetZone1(),
                                                       true);
    }
*/
    m_bActive[Type] = true;
}

//==============================================================================

void actor_effects::InitCloakEffect(effect_type Type,
                                    Object*     pParent,
                                    const char* pEffectName,
                                    int         iBone)
{
    // Shut down any active cloaking effect.
    if (m_bActive[FX_CLOAK]) {
        KillEffect(FX_CLOAK);
    }
    if (m_bActive[FX_DECLOAK]) {
        KillEffect(FX_DECLOAK);
    }
    if (m_bActive[FX_CLOAK_PAIN]) {
        KillEffect(FX_CLOAK_PAIN);
    }

    ResourceHandle<char> Resource;
    Resource.setName(pEffectName);

    assert(pParent && pParent->IsKindOf(actor::GetRTTI()));
    actor*           pActor = (actor*)pParent;
    const AnimGroup* pAnimGroup = pActor->GetAnimGroupPtr();

    if (iBone == -1) {
        // Apply this cloaking effect to all bones.

        for (int i = 0; i < MAX_CLOAK_POINTS; i++) {
            m_CloakBone[i].FXHandle.InitInstance(Resource.getPointer());
        }

        if (pAnimGroup) {
            m_CloakBone[0].iBone = pAnimGroup->GetBoneIndex("B_01_Arm_R_UpperArm");
            m_CloakBone[1].iBone = pAnimGroup->GetBoneIndex("B_01_Arm_R_ForeArm");
            m_CloakBone[2].iBone = pAnimGroup->GetBoneIndex("B_01_Leg_R_Thigh");
            m_CloakBone[3].iBone = pAnimGroup->GetBoneIndex("B_01_Leg_R_Calf");
            m_CloakBone[4].iBone = pAnimGroup->GetBoneIndex("B_01_Arm_L_UpperArm");
            m_CloakBone[5].iBone = pAnimGroup->GetBoneIndex("B_01_Arm_L_ForeArm");
            m_CloakBone[6].iBone = pAnimGroup->GetBoneIndex("B_01_Leg_L_Thigh");
            m_CloakBone[7].iBone = pAnimGroup->GetBoneIndex("B_01_Leg_L_Calf");
            m_CloakBone[8].iBone = pAnimGroup->GetBoneIndex("B_01_Spine01");
            m_CloakBone[9].iBone = pAnimGroup->GetBoneIndex("B_01_Spine02");
            m_CloakBone[10].iBone = pAnimGroup->GetBoneIndex("B_01_Head");
        }
    } else {
        // Apply this cloaking effect to ONE bone.

        m_CloakBone[0].FXHandle.InitInstance(Resource.getPointer());

        if (pAnimGroup) {
            assert((iBone >= 0) && (iBone < pAnimGroup->GetNBones()));
            m_CloakBone[0].iBone = iBone;
        }
    }

    m_bActive[Type] = true;
}

//==============================================================================

void actor_effects::KillEffect(effect_type Type)
{
    switch (Type) {
    //--------------------------------------------------------------------------
    case FX_FLASHLIGHT:
    case FX_MUTATE:
    case FX_UNMUTATE:
    case FX_SPAWN:
        //--------------------------------------------------------------------------
        if (m_bActive[Type]) {
            m_FXHandle[Type].KillInstance();
            m_bActive[Type] = false;
            if (m_AudioID[Type] != 0) {
                // IJB: g_AudioMgr.Release(m_AudioID[Type], 0.25f);
            }
        }
        break;

    //--------------------------------------------------------------------------
    case FX_FLAME:
    case FX_SHOCK:
    case FX_CONTAIGON:
        //--------------------------------------------------------------------------
        KillFryEffect();
        break;

    //--------------------------------------------------------------------------
    case FX_CLOAK:
    case FX_DECLOAK:
    case FX_CLOAK_PAIN:
        //--------------------------------------------------------------------------
        if (m_bActive[Type]) {
            for (int i = 0; i < MAX_CLOAK_POINTS; i++) {
                if (m_CloakBone[i].iBone != -1) {
                    m_CloakBone[i].FXHandle.KillInstance();
                    m_CloakBone[i].iBone = -1;
                }
            }
            m_bActive[Type] = false;
        }
        break;

    //--------------------------------------------------------------------------
    default:
        //--------------------------------------------------------------------------
        break;
    }
}

//==============================================================================

void actor_effects::KillFryEffect()
{
    effect_type Type = FX_MAX;

    if (m_bActive[FX_FLAME]) {
        Type = FX_FLAME;
    }
    if (m_bActive[FX_SHOCK]) {
        Type = FX_SHOCK;
    }
    if (m_bActive[FX_CONTAIGON]) {
        Type = FX_CONTAIGON;
    }

    if (Type != FX_MAX) {
        for (int i = 0; i < m_nFryPoints; i++) {
            m_FryBone[i].FXHandle.KillInstance();
        }
        m_nFryPoints = 0;
        // IJB: g_AudioMgr.Release(m_AudioID[Type], 0.5f);
        m_AudioID[Type] = 0;
        m_bActive[Type] = false;
    }
}

//==============================================================================

bool actor_effects::IsEffectOn(effect_type Type)
{
    switch (Type) {
    //--------------------------------------------------------------------------
    case FX_FLASHLIGHT:
    case FX_FLAME:
    case FX_SHOCK:
    case FX_MUTATE:
    case FX_UNMUTATE:
    case FX_SPAWN:
    case FX_CLOAK:
    case FX_DECLOAK:
    case FX_CLOAK_PAIN:
    case FX_CONTAIGON:
        //--------------------------------------------------------------------------
        return (m_bActive[Type]);
        break;

    //--------------------------------------------------------------------------
    default:
        //--------------------------------------------------------------------------
        assert(false);
        break;
    }

    return (false);
}

//==============================================================================

void actor_effects::UpdateFlashLight(effect_type Type, Object* pParent)
{
    // Attached to an actor?
    if (pParent && pParent->IsKindOf(actor::GetRTTI())) {
        actor*      pActor = (actor*)pParent;
        new_weapon* pWeapon = pActor->GetCurrentWeaponPtr();
        Matrix4     L2W;
        Vector3     Offset;

        // Transform the flashlight effect based on the weapon.
        if (pWeapon) {
            new_weapon::render_state OldRenderState = pWeapon->GetRenderState();
            pWeapon->SetRenderState(new_weapon::RENDER_STATE_NPC);
            if (pWeapon->GetFlashlightTransformInfo(L2W, Offset)) {
                L2W.PreTranslate(Offset);
                m_FXHandle[Type].SetRotation(L2W.GetRotation());
                m_FXHandle[Type].SetTranslation(L2W.GetTranslation());
            }
            pWeapon->SetRenderState(OldRenderState);
        } else {
            // Kill flashlight if no weapon to follow.
            KillEffect(FX_FLASHLIGHT);
        }
    } else {
        // Kill flashlight if no actor to follow.
        KillEffect(FX_FLASHLIGHT);
    }
}

//==============================================================================

void actor_effects::Update(Object* pParent, float DeltaTime)
{
    for (int Type = 0; Type < FX_MAX; Type++) {
        if (!m_bActive[Type]) {
            continue;
        }

        switch (Type) {
        //----------------------------------------------------------------------
        case FX_FLASHLIGHT:
            //----------------------------------------------------------------------

            UpdateFlashLight((effect_type)Type, pParent);
            break;

        //----------------------------------------------------------------------
        case FX_MUTATE:
        case FX_UNMUTATE:
        case FX_SPAWN:
            //----------------------------------------------------------------------
            {
                assert(pParent);
                if (pParent->IsKindOf(actor::GetRTTI())) {
                    float I = ((actor*)pParent)->GetFloorIntensity();
                    I /= 255.0f;
                    I *= 0.8f; // This...
                    I += 0.2f; //   ...plus this should sum to 1.0.
                    I *= 255.0f;
                    int i = (int)I;

                    m_FXHandle[Type].SetColor(Colour(i, i, i));
                }

                m_FXHandle[Type].AdvanceLogic(DeltaTime);
                m_FXHandle[Type].SetTranslation(pParent->GetPosition());

                if (m_FXHandle[Type].IsFinished()) {
                    m_bActive[Type] = false;
                }
                if (m_AudioID[Type] != 0) {
                    /* IJB
                    g_AudioMgr.SetPosition(m_AudioID[Type],
                                           pParent->GetPosition(),
                                           pParent->GetZone1());
                                           */
                }
            }
            break;

        //----------------------------------------------------------------------
        case FX_FLAME:
        case FX_SHOCK:
        case FX_CONTAIGON:
            //----------------------------------------------------------------------
            for (int i = 0; i < m_nFryPoints; i++) {
                assert(m_FryBone[i].iBone != -1);
                {
                    Vector3 Position = GetBonePosition(pParent, m_FryBone[i].iBone);
                    m_FryBone[i].FXHandle.SetTranslation(Position);
                    m_FryBone[i].FXHandle.AdvanceLogic(DeltaTime);
                }
            }
            break;

        //----------------------------------------------------------------------
        case FX_CLOAK:
        case FX_DECLOAK:
        case FX_CLOAK_PAIN:
            //----------------------------------------------------------------------
            for (int i = 0; i < MAX_CLOAK_POINTS; i++) {
                bool Finished = true; // Assume true, disprove below.
                if (m_CloakBone[i].iBone != -1) {
                    Matrix4 L2W;
                    GetBoneL2W(pParent, m_CloakBone[i].iBone, L2W);
                    m_CloakBone[i].FXHandle.SetTranslation(L2W.GetTranslation());
                    m_CloakBone[i].FXHandle.SetRotation(L2W.GetRotation());
                    m_CloakBone[i].FXHandle.AdvanceLogic(DeltaTime);
                    if (!m_CloakBone[i].FXHandle.IsFinished()) {
                        Finished = false;
                    }
                }
                if (Finished) {
                    KillEffect((effect_type)Type);
                }
            }
            break;

        //----------------------------------------------------------------------
        default:
            //----------------------------------------------------------------------
            assert(false);
            break;
        }
    }

    if (m_DeathTimer != 0.0f) {
        m_DeathTimer -= DeltaTime;
        if (m_DeathTimer <= 0.0f) {
            if (pParent) {
                pain Pain(pParent->getObjectManager());
                Pain.Setup("GENERIC_LETHAL", 0, pParent->GetColBBox().GetCenter());
                Pain.SetDirectHitGuid(pParent->GetGuid());
                Pain.ApplyToObject(pParent);
            }
            m_DeathTimer = 0.0f;
        }
    }

    if (m_ShockTimer != 0.0f) {
        m_ShockTimer -= DeltaTime;
        if (m_ShockTimer <= 0.0f) {
            KillEffect(FX_SHOCK);
            m_ShockTimer = 0.0f;
        }
    }
}

//==============================================================================

const Vector3 actor_effects::GetBonePosition(Object* pParent, int iBone)
{
    if (pParent->IsKindOf(actor::GetRTTI())) {
        actor* pActor = (actor*)pParent;
        if (pActor->GetLocoPointer()) {
            loco_char_anim_player& Player = pActor->GetLocoPointer()->m_Player;
            return (Player.GetBonePosition(iBone));
        }
    } else if (pParent->IsKindOf(corpse::GetRTTI())) {
        corpse*       pCorpse = (corpse*)pParent;
        physics_inst& PhysicsInst = pCorpse->GetPhysicsInst();
        return (PhysicsInst.GetBoneWorldPosition(iBone));
    }

    assert(false);
    return (Vector3(0.0f, 0.0f, 0.0f));
}

//==============================================================================

void actor_effects::GetBoneL2W(Object* pParent, int iBone, Matrix4& L2W)
{
    if (pParent->IsKindOf(actor::GetRTTI())) {
        actor* pActor = (actor*)pParent;
        if (pActor->GetLocoPointer()) {
            // Get animation player from actor.
            loco_char_anim_player& Player = pActor->GetLocoPointer()->m_Player;

            // Get the bone L2W making sure to do the proper conversion to take
            // out the extra skin offset.
            L2W = Player.GetBoneL2W(iBone);
            L2W.PreTranslate(Player.GetBoneBindPosition(iBone));
        }
    } else if (pParent->IsKindOf(corpse::GetRTTI())) {
        // Snag the L2W from the ragdoll.
        corpse*       pCorpse = (corpse*)pParent;
        physics_inst& PhysicsInst = pCorpse->GetPhysicsInst();
        L2W = PhysicsInst.GetBoneWorldTransform(iBone);
    } else {
        assert(false);
    }
}

//==============================================================================

void actor_effects::Render(Object* pParent)
{
    (void)pParent;
}

//==============================================================================

void actor_effects::RenderTransparent(Object* pParent, float Alpha)
{
    for (int Type = 0; Type < FX_MAX; Type++) {
        if (!m_bActive[Type]) {
            continue;
        }

        switch (Type) {
        //----------------------------------------------------------------------
        case FX_FLASHLIGHT:
        case FX_MUTATE:
        case FX_UNMUTATE:
            //----------------------------------------------------------------------
            if (pParent->IsKindOf(actor::GetRTTI())) {
                actor* pActor = (actor*)pParent;
                /* IJB 
                if ((g_RenderContext.NetPlayerSlot != pActor->net_GetSlot())) {
                    m_FXHandle[Type].Render();
                }
                    */
            } else {
                m_FXHandle[Type].Render();
            }
            break;

        case FX_SPAWN:

            m_FXHandle[Type].Render();
            break;

        case FX_FLAME:
        case FX_SHOCK:
        case FX_CONTAIGON:
            
            {
                bool              bRender = true;
                object_ptr<actor> pActor(pParent);

                /* IJB
                if (pActor.IsValid()) {
                    if (g_RenderContext.NetPlayerSlot == pActor->net_GetSlot()) {
                        bRender = false;
                    }
                }
*/
                if (bRender) {
                    for (int i = 0; i < m_nFryPoints; i++) {
                        Colour Color(255, 255, 255, (uint8_t)(Alpha * 255));
                        m_FryBone[i].FXHandle.SetColor(Color);
                        m_FryBone[i].FXHandle.Render();
                    }
                }
            }
            break;

        //----------------------------------------------------------------------
        case FX_CLOAK:
        case FX_DECLOAK:
        case FX_CLOAK_PAIN:
            //----------------------------------------------------------------------
            for (int i = 0; i < MAX_CLOAK_POINTS; i++) {
                if (m_CloakBone[i].iBone >= 0) {
                    m_CloakBone[i].FXHandle.Render();
                }
            }
            break;
        //----------------------------------------------------------------------
        default:
            //----------------------------------------------------------------------
            assert(false);
            break;
        }
    }
}

void actor_effects::SetDeathTimer(float DeathTimer)
{
    if (DeathTimer > 10000.0f) {
        m_DeathTimer = 0.0f;
    } else {
        m_DeathTimer = DeathTimer;
    }
}

void actor_effects::SetShockTimer(float Timer)
{
    if (Timer > 10000.0f) {
        m_ShockTimer = 0.0f;
    } else {
        m_ShockTimer = Timer;
    }
}

bool actor_effects::IsActive()
{
    for (int i = 0; i < FX_MAX; i++) {
        if (m_bActive[i]) {
            return true;
        }
    }

    return false;
}
