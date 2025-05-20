#include "Player.h"
#include "../inputMgr/GamePad.h"
// #include "objects\ParticleEmiter.hpp"
// #include "objects\Render\PostEffectMgr.hpp"
// #include "objects\SpawnPoint.hpp"
#include "Event.h"
#include "sound/EventSoundEmitter.h"
// #include "..\support\templatemgr\TemplateMgr.hpp"
// #include "characters\Character.hpp"
// #include "Characters\Conversation_Packet.hpp"
// #include "GameLib\StatsMgr.hpp"
// #include "GameLib\RenderContext.hpp"
// #include "Dictionary\global_dictionary.hpp"
// #include "objects\WeaponSniper.hpp"
#include "ThirdPersonCamera.h"
//#include "objects\WeaponSMP.hpp"
#include "Corpse.h"
// #include "NetworkMgr/NetObjMgr.hpp"
// #include "NetworkMgr/Voice/VoiceMgr.hpp"
// #include "Objects\Ladders\Ladder_Field.hpp"
// #include "Objects\GrenadeProjectile.hpp"
// #include "Objects\GravChargeProjectile.hpp"
// #include "Objects\JumpingBeanProjectile.hpp"
// #include "render\LightMgr.hpp"
// #include "Objects\Door.hpp"
#include "Projector.h"
// #include "objects\WeaponMutation.hpp"
// #include "StateMgr\StateMgr.hpp"
// #include "NetworkMgr\GameMgr.hpp"
#include "HudObject.h"
#include "../characters/ActorEffects.h"
// #include "Configuration/GameConfig.hpp"
// #include "objects\turret.hpp"
// #include "objects\WeaponShotgun.hpp"
// #include "Gamelib/DebugCheats.hpp"
// #include "objects\FocusObject.hpp"
#include "../PerceptionMgr.h"
#include "LoreObject.h"
//#include "Objects\Camera.hpp"
//#include "Characters\MutantTank\Mutant_Tank.hpp"
//#include "Objects\WeaponBBG.hpp"

#include "../tweakManager/TweakMgr.h"

#include "../xfiles/xfs.h"
#include "../xfiles/x_plus.h"
#include <algorithm>

static const float s_CheckForGameSpeakTime = 0.25f;

void player::GatherGameSpeakGuid()
{
    if (s_CheckForGameSpeakTime > objectManager->GetGameDeltaTime(m_GameSpeakCounter)) {
        return;
    }

    // If already speaking, nothing.
    if (m_bSpeaking) {
        return;
    }

    if (m_SpeakToGuid != 0) {
        Object* pSpeakTo = objectManager->GetObjectByGuid(m_SpeakToGuid);

        // Has the object we are talking to been destroyed?
        if (pSpeakTo == NULL) {
            m_SpeakToGuid = 0;
            return;
        }
    }

    m_GameSpeakCounter = objectManager->GetGameTime();

    // Gather all NPC's who the player can speak with in a Bbox around the player.

    // Create the BBox.
    Vector3 BoxCenter = GetPosition();
    BoxCenter.y += .5f * GetCollisionHeight();
    float BoxRadius = 200.0f;
    BBox  GameSpeakBox(BoxCenter, BoxRadius);

    // Some variables for the collision check.
    slot_id CollidedObjectSlot = SLOT_NULL;
    Object* pPotentialTarget = NULL;

    // Check the collisions.
    objectManager->SelectBBox(Object::ATTR_CHARACTER_OBJECT, GameSpeakBox);

    // Scan the box for living objects.
    for (CollidedObjectSlot = objectManager->StartLoop();
         CollidedObjectSlot != SLOT_NULL;
         CollidedObjectSlot = objectManager->GetNextResult(CollidedObjectSlot)) {
        pPotentialTarget = objectManager->GetObjectBySlot(CollidedObjectSlot);
/* IJB
        // Check the factions for the character object in the BBox.
        factions TargetObjectFaction = SMP_UTIL_GetFactionForGuid(pPotentialTarget->GetGuid());

        if (IsFriendlyFaction(TargetObjectFaction)) {
            assert(pPotentialTarget->IsKindOf(character::GetRTTI()));

            character* pCharacter = (character*)pPotentialTarget;
            if (pCharacter->SetPotentialListener(true)) {
                m_SpeakToGuid = pPotentialTarget->GetGuid();
            }
            break;
        }
            */
    }

    objectManager->EndLoop();
/* IJB
    // Clear out SpeakToGuid if there is no potential target.
    if (!pPotentialTarget && m_SpeakToGuid) {
        assert(objectManager->GetObjectByGuid(m_SpeakToGuid)->IsKindOf(character::GetRTTI()));
        Object*    pObject = objectManager->GetObjectByGuid(m_SpeakToGuid);
        character* pCharacter = (character*)pObject;
        pCharacter->SetPotentialListener(false);
        m_SpeakToGuid = 0;
    }
    */
}

void player::OnGameSpeak()
{
    assert(m_ActivePlayerPad != -1);
/* IJB

    // If we are already speaking, we need to verify that the gamespeak emitter is still
    // a valid object.
    if (m_bSpeaking) {
        if (objectManager->GetObjectByGuid(m_GameSpeakEmitterGuid) == NULL) {
            m_bSpeaking = false;
        }
    }

    // If there is no one to speak to, nothing.
    if (m_SpeakToGuid == 0) {
        return;
    }

    // If already speaking, nothing.
    if (m_bSpeaking) {
        return;
    }

    Object* pSpeakTo = objectManager->GetObjectByGuid(m_SpeakToGuid);

    // Has the object we are talking to been destroyed?
    if (pSpeakTo == NULL) {
        m_SpeakToGuid = 0;
        return;
    }

    character* pCharacter = NULL;

    // If object he wants to talk to is already speaking, do nothing.
    if (m_SpeakToGuid) {
        assert(objectManager->GetObjectByGuid(m_SpeakToGuid)->IsKindOf(character::GetRTTI()));

        pCharacter = (character*)pSpeakTo;
    }

    conversation_packet Request;
    char                SoundDescriptorName[64];

    if (g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_SPEAK_FOLLOW_STAY).WasValue) {
        switch (objectManager->GetObjectByGuid(m_SpeakToGuid)->GetType()) {
        case TYPE_FRIENDLY_SCIENTIST:
            sprintf(SoundDescriptorName, "Request_Sci_Follow");
            break;
        default:
            return;
        }
        Request.m_ConvType = CONV_REQUEST_FOLLOW;
    } else if (g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_SPEAK_USE_ACTIVATE).WasValue) {
        // Just make sure we didn't get here without having someone to talk to.
        assert(pCharacter);

        switch (objectManager->GetObjectByGuid(m_SpeakToGuid)->GetType()) {
        case TYPE_FRIENDLY_SCIENTIST:
            sprintf(SoundDescriptorName, "Request_Sci_Activate_Item");
            break;
        default:
            return;
        }

        Request.m_ConvType = CONV_REQUEST_ACTIVATE_ITEM;
    } else if (g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_SPEAK_COVER_ME).WasValue) {
        // Just make sure we didn't get here without having someone to talk to.
        assert(pCharacter);

        switch (objectManager->GetObjectByGuid(m_SpeakToGuid)->GetType()) {
        case TYPE_FRIENDLY_SCIENTIST:
            sprintf(SoundDescriptorName, "Request_Stay_Here");
            break;
        default:
            return;
        }
        Request.m_ConvType = CONV_REQUEST_STAY;
    } else if (g_IngamePad[m_ActivePlayerPad].GetLogical(ingame_pad::ACTION_SPEAK_ATTACK_COVER).WasValue) {
        return;
    } else {
        return;
    }

    // We create the emitter.
    m_GameSpeakEmitterGuid = objectManager->CreateObject(event_sound_emitter::GetObjectType());
    Object*              pSndObj = objectManager->GetObjectByGuid(m_GameSpeakEmitterGuid);
    event_sound_emitter& EventEmitter = event_sound_emitter::GetSafeType(*pSndObj);
    Request.m_SpeakerGuid = GetGuid();
    Request.m_SoundEmitterGuid = m_GameSpeakEmitterGuid;
    m_bSpeaking = true;

    Vector3 vPos = GetPosition();
    EventEmitter.PlayEmitter(SoundDescriptorName, vPos, GetZone1(),
                             event_sound_emitter::SINGLE_SHOT, GetGuid());

    assert(objectManager->GetObjectByGuid(m_SpeakToGuid)->IsKindOf(character::GetRTTI()));

    // Now sent the packet to the character who needs it.
    assert(pCharacter);
    pCharacter->SetActiveListener(true);
    pCharacter->Listen(Request);
*/
}

bool JIM_BOOL_TEST = false;

void player::VoteCast(int Vote)
{
}

void player::VoteStartKick(int Kick)
{

}

void player::VoteStartMap(int Map)
{

}

