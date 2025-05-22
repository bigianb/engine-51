

#include "EventSoundEmitter.h"
//#include "AudioMgr\AudioMgr.hpp"
#include "../../animation/animData.h"
#include "../Event.h"
// #include "conversationMgr\ConversationMgr.hpp"
#include "../../collisionMgr/CollisionMgr.h"
// #include "gamelib\StatsMgr.hpp"
// #include "Characters\Character.hpp"
#include "../Player.h"

#define FOOTFALL_COLLISION_DEPTH 10.0f
#define MIN_INDEX 1
#define MAX_INDEX 4

bool s_EventContactDebug = false;

//=========================================================================
static struct event_sound_emitter_desc : public object_desc
{
    event_sound_emitter_desc(void)
        : object_desc(
              Object::TYPE_EVENT_SND_EMITTER,
              "Event Sound Emitter",
              "SOUND",
              Object::ATTR_NEEDS_LOGIC_TIME |
                  Object::ATTR_SOUND_SOURCE,
              FLAGS_IS_DYNAMIC)
    {
    }

    //---------------------------------------------------------------------

    virtual Object* Create(ObjectManager* om, collision_mgr*, ResourceManager*)
    {
        return new event_sound_emitter(om);
    }

} s_EventSoundEmitter_Desc;

//=========================================================================

const object_desc& event_sound_emitter::GetTypeDesc() const
{
    return s_EventSoundEmitter_Desc;
}

//=========================================================================

const object_desc& event_sound_emitter::GetObjectType()
{
    return s_EventSoundEmitter_Desc;
}

//=========================================================================
// FUNCTIONS
//=========================================================================

//=========================================================================

event_sound_emitter::event_sound_emitter(ObjectManager* om)
    : Object(om)
{
    m_DescriptorName[0] = 0;
    m_ActionName[0] = 0;
    m_ObjectName[0] = 0;
    m_VoiceID = 0;
    m_ParentGuid = 0;
    m_Active = true;
    m_Delay = 0.0f;
    m_Flags = 0;
    m_Radius = 0.0f;
    m_SoundType = SINGLE_SHOT;
    m_SphereTest = false;
    m_b2D = false;
}

//=========================================================================

void event_sound_emitter::OnAdvanceLogic(float DeltaTime)
{
    // Check if its time for the sound to start yet.
    if (m_Delay > 0.0f) {
        m_Delay -= DeltaTime;

        if (m_Delay <= 0.0f) {
            if (m_b2D) {
                StartEmitter2D();
            } else {
                StartEmitter();
            }
        } else {
            return;
        }
    }

    // If its a single_shot sound then only destroy the object when its done playing.
    // If its a looped sound then keep the object alive till it get update from OnMove.
    // If its a conversation sound then don't kill the object untill the sound becomes inactive.
    /* IJB
        else if (m_SoundType == SINGLE_SHOT || m_SoundType == CONTACT) {
            if (!g_AudioMgr.IsValidVoiceId(m_VoiceID)) {
                objectManager->DestroyObject(GetGuid());
            }
        } else if (m_SoundType == LOOPED) {
            if (m_Active == false) {
                g_AudioMgr.Release(m_VoiceID, 0.0f);
                objectManager->DestroyObject(GetGuid());
            }
        } else if ((m_SoundType == CONVERSATION) || (m_Flags == VOICE_EVENT)) {
            if (g_ConverseMgr.IsActive(m_VoiceID) == false) {
                objectManager->DestroyObject(GetGuid());
            }
        }
    */
    m_Active = false;
}

//=========================================================================

void event_sound_emitter::PlayEmitter(const char* pDescriptor, Vector3& Position, uint16_t ZoneID, sound_type Type,
                                      guid ParentGuid, uint32_t Flags, float Delay, bool UseRadius, float Radius, bool Play2D)
{
    assert(pDescriptor != NULL);
    assert(((Type >= FIRST_TYPE) && (Type <= LAST_TYPE)));

    Object* pObject = objectManager->GetObjectByGuid(ParentGuid);

    // Set the zone and the position.
    SetZone1(ZoneID);
    Object::OnMove(Position);

    m_SphereTest = UseRadius;
    m_Radius = Radius;

    if (Type == CONTACT) {
        /* IJB
        if (pObject && pObject->IsKindOf(character::GetRTTI())) {
            character& ourCharacter = character::GetSafeType(*pObject);
            sprintf(m_DescriptorName, "%s_%s_%s", ourCharacter.GetDialogPrefix(), pDescriptor, GetMaterialTypeFromActor(ParentGuid));
        } else {
            sprintf(m_DescriptorName, "%s_%s", pDescriptor, GetMaterialType());
        }
            */
    } else if (pObject) {
        /* IJB
        if (pObject->IsKindOf(character::GetRTTI())) {
            character& ourCharacter = character::GetSafeType(*pObject);
            sprintf(m_DescriptorName, "%s_%s", ourCharacter.GetDialogPrefix(), pDescriptor);
            sprintf(m_ObjectName, "%s", ourCharacter.GetDialogPrefix());
        } else */ if (pObject->GetType() == Object::TYPE_WEAPON_SMP) {
            snprintf(m_DescriptorName, 64, "SMP_%s", pDescriptor);
            snprintf(m_ObjectName, 64, "SMP");
        } else if (pObject->GetType() == Object::TYPE_WEAPON_DUAL_SMP) {
            snprintf(m_DescriptorName, 64, "2MP_%s", pDescriptor);
            snprintf(m_ObjectName, 64, "2MP");
        }
        // KSS -- TO ADD NEW WEAPON
        else if (pObject->GetType() == Object::TYPE_WEAPON_SHOTGUN) {
            snprintf(m_DescriptorName, 64, "SHT_%s", pDescriptor);
            snprintf(m_ObjectName, 64, "SHT");
        }

        else if (pObject->GetType() == Object::TYPE_WEAPON_DUAL_SHT) {
            snprintf(m_DescriptorName, 64, "2SH_%s", pDescriptor);
            snprintf(m_ObjectName, 64, "2SH");
        }

        else if (pObject->GetType() == Object::TYPE_WEAPON_SCANNER) {
            snprintf(m_DescriptorName, 64, "SCN_%s", pDescriptor);
            snprintf(m_ObjectName, 64, "SCN");
        }

        else if (pObject->GetType() == Object::TYPE_WEAPON_SNIPER) {
            snprintf(m_DescriptorName, 64, "SNI_%s", pDescriptor);
            snprintf(m_ObjectName, 64, "SNI");
        }

        else if (pObject->GetType() == Object::TYPE_WEAPON_GAUSS) {
            snprintf(m_DescriptorName, 64, "GAS_%s", pDescriptor);
            snprintf(m_ObjectName, 64, "GAS");
        }

        else if (pObject->GetType() == Object::TYPE_WEAPON_DESERT_EAGLE) {
            snprintf(m_DescriptorName, 64, "EGL_%s", pDescriptor);
            snprintf(m_ObjectName, 64, "EGL");
        }

        else if (pObject->GetType() == Object::TYPE_WEAPON_MHG) {
            snprintf(m_DescriptorName, 64, "MHG_%s", pDescriptor);
            snprintf(m_ObjectName, 64, "MHG");
        } else if (pObject->GetType() == Object::TYPE_WEAPON_MSN) {
            snprintf(m_DescriptorName, 64, "MSN_%s", pDescriptor);
            snprintf(m_ObjectName, 64, "MSN");
        } else if (pObject->GetType() == Object::TYPE_WEAPON_BBG) {
            snprintf(m_DescriptorName, 64, "BBG_%s", pDescriptor);
            snprintf(m_ObjectName, 64, "BBG");
        } else if (pObject->GetType() == Object::TYPE_WEAPON_TRA) {
            snprintf(m_DescriptorName, 64, "TRA_%s", pDescriptor);
            snprintf(m_ObjectName, 64, "TRA");
        } else if (pObject->GetType() == Object::TYPE_WEAPON_MUTATION) {
            snprintf(m_DescriptorName, 64, "MUT_%s", pDescriptor);
            snprintf(m_ObjectName, 64, "MUT");
        } else {
            strncpy(m_DescriptorName, pDescriptor, 64);
        }
    }

    strncpy(m_ActionName, pDescriptor, 64);

    m_ParentGuid = ParentGuid;
    m_SoundType = Type;
    m_Delay = Delay;
    m_Flags = Flags;

    //
    // NOTE: This should be a separate call to the event sound emitter but since
    //       I don't have time, this will have to do.
    //

    m_b2D = Play2D;

    if (m_Delay == 0.0f) {
        if (m_b2D) {
            StartEmitter2D();
        } else {
            StartEmitter();
        }
    }
}

//=========================================================================

void event_sound_emitter::StartEmitter(void)
{
    uint16_t ZoneID = GetZone1();
/* IJB
    // TODO: Need to also give it the zoneid.
    if (m_Flags == GUN_SHOT_EVENT) {
        m_VoiceID = g_AudioMgr.PlayVolumeClipped(m_DescriptorName, GetPosition(), ZoneID, true);
        g_AudioManager.NewAudioAlert(m_VoiceID, audio_manager::GUN_SHOT, GetPosition(), ZoneID, m_ParentGuid);
    } else if (m_Flags == EXPLOSION_EVENT) {
        m_VoiceID = g_AudioMgr.Play(m_DescriptorName, GetPosition(), ZoneID, true);
        g_AudioManager.NewAudioAlert(m_VoiceID, audio_manager::EXPLOSION, GetPosition(), ZoneID, m_ParentGuid);
    } else if (m_Flags == VOICE_EVENT) {
        Object* pObj = objectManager->GetObjectByGuid(m_ParentGuid);

        // actors handle it this way
        if (pObj->IsKindOf(actor::GetRTTI())) {
            dialog_event dialogEvent;
            dialogEvent.Type = event::EVENT_DIALOG;

            strcpy(dialogEvent.DialogName, m_DescriptorName); //m_ActionName );
            dialogEvent.HotVoice = m_SoundType != CONVERSATION;
            pObj->OnEvent(dialogEvent);
        }
        // all others handle it this way
        else {
            if (m_SoundType == CONVERSATION) {
                m_VoiceID = g_ConverseMgr.PlayStream(m_ObjectName, m_ActionName, GetGuid(), ZoneID, GetPosition(), 1.0f);
            } else {
                m_VoiceID = g_ConverseMgr.PlayHotVoice(m_ObjectName, m_ActionName, GetPosition(), ZoneID, 1.0f);
            }

            voice_event VoiceEvent;
            VoiceEvent.Type = event::EVENT_VOICE;
            VoiceEvent.VoiceID = m_VoiceID;

            pObj->OnEvent(VoiceEvent);
        }
    } else {
        if (m_SoundType == CONVERSATION) {
            m_VoiceID = g_ConverseMgr.PlayStream(m_DescriptorName, GetPosition(), GetGuid(), GetZone1());
        } else {
            m_VoiceID = g_AudioMgr.PlayVolumeClipped(m_DescriptorName, GetPosition(), GetZone1(), true);
        }
    }
*/
    m_Active = true;
}

//=========================================================================

void event_sound_emitter::StartEmitter2D()
{
    /* IJB
    // TODO: Need to also give it the zoneid.
    if (m_Flags == GUN_SHOT_EVENT) {
        m_VoiceID = g_AudioMgr.Play(m_DescriptorName);
        g_AudioManager.NewAudioAlert(m_VoiceID, audio_manager::GUN_SHOT, m_ParentGuid);
    } else if (m_Flags == EXPLOSION_EVENT) {
        m_VoiceID = g_AudioMgr.Play(m_DescriptorName);
        g_AudioManager.NewAudioAlert(m_VoiceID, audio_manager::EXPLOSION, m_ParentGuid);
    } else if (m_Flags == VOICE_EVENT) {
        Object* pObj = objectManager->GetObjectByGuid(m_ParentGuid);

        // actors handle it this way
        if (pObj->IsKindOf(actor::GetRTTI())) {
            dialog_event dialogEvent;
            dialogEvent.Type = event::EVENT_DIALOG;

            strcpy(dialogEvent.DialogName, m_DescriptorName); //m_ActionName );
            dialogEvent.HotVoice = m_SoundType != CONVERSATION;
            pObj->OnEvent(dialogEvent);
        }
        // all others handle it this way
        else {
            if (m_SoundType == CONVERSATION) {
                m_VoiceID = g_ConverseMgr.PlayStream(m_ObjectName, m_ActionName, GetGuid(), GetZone1(),
                                                     GetPosition(), 1.0f, true, PLAY_2D);
            } else {
                m_VoiceID = g_ConverseMgr.PlayHotVoice(m_ObjectName, m_ActionName,
                                                       GetPosition(), GetZone1(), 1.0f, true, PLAY_2D);
            }

            voice_event VoiceEvent;
            VoiceEvent.Type = event::EVENT_VOICE;
            VoiceEvent.VoiceID = m_VoiceID;

            pObj->OnEvent(VoiceEvent);
        }
    } else {
        if (m_SoundType == CONVERSATION) {
            m_VoiceID = g_ConverseMgr.PlayStream(m_DescriptorName, GetPosition(), GetGuid(), GetZone1(), IMMEDIATE_PLAY,
                                                 true, PLAY_2D);
        } else {
            m_VoiceID = g_AudioMgr.Play(m_DescriptorName);
        }
    }
*/
    m_Active = true;
}

//=========================================================================

void event_sound_emitter::OnMove(const Vector3& NewPos)
{
    Object::OnMove(NewPos);

    m_Active = true;

    if (m_b2D) {
        return;
    }

    // IJB g_AudioMgr.SetPosition(m_VoiceID, NewPos, GetZone1());
}

//=========================================================================

BBox event_sound_emitter::GetLocalBBox() const
{
    BBox Box(Vector3(0.0f, 0.0f, 0.0f), 0.0f);
    return Box;
}

//=========================================================================

const char* event_sound_emitter::GetMaterialType(void)
{
    int CollisionMat = MAT_TYPE_NULL;

    // We need to see what tri the player is colliding against so we can play the correct sound.
    Vector3 Start(GetPosition());
    Vector3 End(Start);
    Start.y += FOOTFALL_COLLISION_DEPTH;
    End.y -= FOOTFALL_COLLISION_DEPTH * 2.0f;

    if (m_SphereTest) {
        g_CollisionMgr.SphereSetup(GetGuid(), GetPosition(), GetPosition(), m_Radius);
    } else {
        g_CollisionMgr.RaySetup(GetGuid(), Start, End);
    }

    g_CollisionMgr.CheckCollisions();

    if (g_CollisionMgr.m_nCollisions) {
        CollisionMat = g_CollisionMgr.m_Collisions[0].Flags;
        return GetMaterialName(CollisionMat);
    } else {
        return GetMaterialName(CollisionMat);
    }
}

//=========================================================================

const char* event_sound_emitter::GetMaterialTypeFromActor(guid Guid)
{
    Object* pObj = objectManager->GetObjectByGuid(Guid);

    assert(pObj->GetAttrBits() & Object::ATTR_LIVING);

    actor& Actor = actor::GetSafeType(*pObj);

    return nullptr; // IJB GetMaterialName(Actor.GetFloorMaterial());
}

//=========================================================================

const char* event_sound_emitter::GetMaterialName(int MatType)
{
    switch (MatType) {
    case MAT_TYPE_NULL:
        return "Null";
        break;
    case MAT_TYPE_EARTH:
        return "Earth";
        break;
    case MAT_TYPE_ROCK:
        return "Rock";
        break;
    case MAT_TYPE_CONCRETE:
        return "Concrete";
        break;
    case MAT_TYPE_SOLID_METAL:
        return "Metal";
        break;
    case MAT_TYPE_HOLLOW_METAL:
        return "HollowMetal";
        break;
    case MAT_TYPE_METAL_GRATE:
        return "MetalGrate";
        break;
    case MAT_TYPE_PLASTIC:
        return "Plastic";
        break;
    case MAT_TYPE_WATER:
        return "Water";
        break;
    case MAT_TYPE_WOOD:
        return "Wood";
        break;
    case MAT_TYPE_ENERGY_FIELD:
        return "EnergyField";
        break;
    case MAT_TYPE_BULLET_PROOF_GLASS:
        return "BulletProofGlass";
        break;
    case MAT_TYPE_ICE:
        return "Ice";
        break;

    case MAT_TYPE_LEATHER:
        return "Leather";
        break;
    case MAT_TYPE_EXOSKELETON:
        return "Exoskeleton";
        break;
    case MAT_TYPE_FLESH:
        return "Flesh";
        break;
    case MAT_TYPE_BLOB:
        return "Blob";
        break;

    case MAT_TYPE_FIRE:
        return "Fire";
        break;
    case MAT_TYPE_GHOST:
        return "Ghost";
        break;
    case MAT_TYPE_FABRIC:
        return "Fabric";
        break;
    case MAT_TYPE_CERAMIC:
        return "Ceramic";
        break;
    case MAT_TYPE_WIRE_FENCE:
        return "WireFence";
        break;

    case MAT_TYPE_GLASS:
        return "Glass";
        break;

    case MAT_TYPE_CARPET:
        return "Carpet";
        break;
    case MAT_TYPE_CLOTH:
        return "Cloth";
        break;
    case MAT_TYPE_DRYWALL:
        return "Drywall";
        break;
    case MAT_TYPE_FLESHHEAD:
        return "Flesh_head";
        break;
    case MAT_TYPE_MARBLE:
        return "Marble";
        break;
    case MAT_TYPE_TILE:
        return "Tile";
        break;

    default:
        return "Null";
        break;
    }
}
