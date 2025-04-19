

#include "EventMgr.h"
#include "objects/Event.h"
#include "objects/sound/EventSoundEmitter.h"
#include "objects/ParticleEventEmitter.h"
#include "animation/animData.h"
#include "objects/Player.h"
//#include "Debris\Debris_mgr.hpp"
#include "xfiles/x_plus.h"
#include "xfiles/xfs.h"

event_mgr g_EventMgr;

int pain_event::CurrentEventID = 0;

event_mgr::event_mgr()
{
}

//==============================================================================

event_mgr::~event_mgr()
{
}

//==============================================================================

void event_mgr::HandleSuperEvents(loco_char_anim_player& CharAnimPlayer, loco_anim_controller& LocoAnimController, Object* pObj)
{
    for (int i = 0; i < LocoAnimController.GetNEvents(); i++) {
        // Send this event?
        if (LocoAnimController.IsEventActive(i)) {
            // Lookup event and world position
            const anim_event& Event = LocoAnimController.GetEvent(i);
            HandleAnimEvents(Event, pObj, i, CharAnimPlayer);
        }
    }
}

void event_mgr::HandleSuperEvents(loco_char_anim_player& CharAnimPlayer, Object* pObj)
{
    for (int i = 0; i < CharAnimPlayer.GetNEvents(); i++) {
        // Send this event?
        if (CharAnimPlayer.IsEventActive(i)) {
            const anim_event& Event = CharAnimPlayer.GetEvent(i);
            HandleAnimEvents(Event, pObj, i, CharAnimPlayer);
        }
    }
}

void event_mgr::HandleSuperEvents(char_anim_player& CharAnimPlayer, Object* pObj)
{
    for (int i = 0; i < CharAnimPlayer.GetNEvents(); i++) {
        // Send this event?
        if (CharAnimPlayer.IsEventActive(i)) {
            // Lookup event and world position
            const anim_event& Event = CharAnimPlayer.GetEvent(i);
            HandleAnimEvents(Event, pObj, i, CharAnimPlayer);
        }
    }
}

//==============================================================================

void event_mgr::HandleSuperEvents(simple_anim_player& SimpleAnimPlayer, Object* pObj)
{
    if (SimpleAnimPlayer.GetAnimGroup() == nullptr) {
        return;
    }

    for (int i = 0; i < SimpleAnimPlayer.GetNEvents(); i++) {
        // Send this event?
        if (SimpleAnimPlayer.IsEventActive(i)) {
            // Lookup event and world position
            const anim_event& Event = SimpleAnimPlayer.GetEvent(i);
            HandleAnimEvents(Event, pObj, i, SimpleAnimPlayer);
        }
    }
}

//==============================================================================

void event_mgr::HandleAnimEvents(const anim_event& AnimEvent, Object* pObj, int EventIndex, base_player& BasePlayer)
{
    const char* EventType = AnimEvent.GetType();

    if (strcmp(EventType, "Old Event") == 0) {
        return;
    }

    if (strcmp(EventType, "Audio") == 0) {
        //-------------------------------------------------------
        // Audio
        //-------------------------------------------------------

        audio_event AudioEvent;
        AudioEvent.Type = event::EVENT_AUDIO;
        AudioEvent.bUsePosition = false;
        strncpy(AudioEvent.DescriptorName, AnimEvent.GetString(anim_event::STRING_IDX_AUDIO_SOUND_ID), 64);

        //-------------------------------------------------------
        // Produce the position if needed
        //-------------------------------------------------------
        Vector3     Position;
        bool       UsePosition = false;
        const char* Location = AnimEvent.GetString(anim_event::STRING_IDX_AUDIO_LOCATION);

        if ((strlen(Location) == 0) || !x_stricmp(Location, "Center of Object")) {
            Position = pObj->GetPosition(); //pObj->GetBBox().GetCenter();
            UsePosition = true;
        } else if (!x_stricmp(Location, "Center of Event")) {
            Position = BasePlayer.GetEventPosition(AnimEvent);
            UsePosition =
                AudioEvent.bUsePosition = true;
        } else if (!x_stricmp(Location, "Globally")) {
            Position.Zero();
            UsePosition = false;
        }

        AudioEvent.Position = Position;

        //-------------------------------------------------------
        // Get the type of audio event.
        //-------------------------------------------------------

        const char* Type = AnimEvent.GetString(anim_event::STRING_IDX_AUDIO_TYPE);

        if ((strlen(Type) == 0) || !x_stricmp(Type, "One Shot")) {
            AudioEvent.SoundType = event_sound_emitter::SINGLE_SHOT;
        } else if (!x_stricmp(Type, "Looped")) {
            AudioEvent.SoundType = event_sound_emitter::LOOPED;
        } else if (!x_stricmp(Type, "Contact")) {
            AudioEvent.SoundType = event_sound_emitter::CONTACT;
        } else {
            assert(false); //, xfs("Animation Audio Event is of invalid type"));
        }

        AudioEvent.Flag = AnimEvent.GetInt(anim_event::INT_IDX_AUDIO_DATA);

        HandleAudioEvent(AudioEvent, pObj, UsePosition);
    } else if (strcmp(EventType, "Particle") == 0) {
        //-------------------------------------------------------
        // Particle
        //-------------------------------------------------------

        particle_event ParticleEvent;

        ParticleEvent.Type = event::EVENT_PARTICLE;
        strncpy(ParticleEvent.ParticleName, AnimEvent.GetString(anim_event::STRING_IDX_HOTPOINT_TYPE), 64);

        ParticleEvent.EventActive = AnimEvent.GetBool(anim_event::BOOL_IDX_PARTICLE_EVENT_ACTIVE);
        ParticleEvent.DoNotAppyTransform = AnimEvent.GetBool(anim_event::BOOL_IDX_PARTICLE_DONOT_APPLY_TRANSFORM);

        //-------------------------------------------------------
        // Produce the position if needed
        //-------------------------------------------------------
        ParticleEvent.Position = BasePlayer.GetEventPosition(AnimEvent); //.Set( EventPos.X, EventPos.Y, EventPos.Z );

        //-------------------------------------------------------
        // Get the orientation if needed
        //-------------------------------------------------------

        ParticleEvent.Orientation = BasePlayer.GetEventRotation(AnimEvent); //EventRotation;

        ParticleEvent.EventIndex = EventIndex;

        HandleParticleEvent(ParticleEvent, pObj);

    } else if (strcmp(EventType, "Hot Point") == 0) {
        //-------------------------------------------------------
        // Hot Points
        //-------------------------------------------------------

        hotpoint_event HotPointEvent;

        HotPointEvent.Type = event::EVENT_HOTPOINT;
        strncpy(HotPointEvent.HotPointType, AnimEvent.GetString(anim_event::STRING_IDX_HOTPOINT_TYPE), 64);

        //-------------------------------------------------------
        // Get the position + orientation if needed
        //-------------------------------------------------------
        HotPointEvent.Position = BasePlayer.GetEventPosition(AnimEvent);    //.Set( EventPos.X, EventPos.Y, EventPos.Z );
        HotPointEvent.Orientation = BasePlayer.GetEventRotation(AnimEvent); //EventRotation;

        //HotPointEvent.Orientation.Rotate( AnimEvent.GetPoint( anim_event::POINT_IDX_ROTATION ) );

        HandleHotPointEvent(HotPointEvent, pObj);
    } else if (strcmp(EventType, "Generic") == 0) {
        //-------------------------------------------------------
        // Generic
        //-------------------------------------------------------

        generic_event GenericEvent;

        GenericEvent.Type = event::EVENT_GENERIC;
        strncpy(GenericEvent.GenericType, AnimEvent.GetString(anim_event::STRING_IDX_GENERIC_TYPE), 64);
        GenericEvent.EventIndex = EventIndex;

        HandleGenericEvent(GenericEvent, pObj);
    } else if (strcmp(EventType, "Intensity") == 0) {
        //-------------------------------------------------------
        // Intensity
        //-------------------------------------------------------

        intensity_event IntensityEvent;

        //-------------------------------------------------------
        // Intensity controller event.
        //-------------------------------------------------------

        IntensityEvent.Type = event::EVENT_INTENSITY;
        IntensityEvent.ControllerIntensity = std::min(AnimEvent.GetFloat(anim_event::FLOAT_IDX_CONTROLLER_INTENSITY), 1.0f);
        IntensityEvent.ControllerDuration = AnimEvent.GetFloat(anim_event::FLOAT_IDX_CONTROLLER_DURATION);
        IntensityEvent.CameraShakeTime = AnimEvent.GetFloat(anim_event::FLOAT_IDX_CAMERA_SHAKE_TIME);
        IntensityEvent.CameraShakeAmount = AnimEvent.GetFloat(anim_event::FLOAT_IDX_CAMERA_SHAKE_AMOUNT);
        IntensityEvent.CameraShakeSpeed = AnimEvent.GetFloat(anim_event::FLOAT_IDX_CAMERA_SHAKE_SPEED);
        IntensityEvent.BlurIntensity = AnimEvent.GetFloat(anim_event::FLOAT_IDX_BLUR_INTENSITY);
        IntensityEvent.BlurDuration = AnimEvent.GetFloat(anim_event::FLOAT_IDX_BLUR_DURATION);

        HandleIntensityEvent(IntensityEvent, pObj);
    } else if (strcmp(EventType, "World Collision") == 0) {
        //-------------------------------------------------------
        // World Collision
        //-------------------------------------------------------

        world_collision_event WorldCollisionEvent;

        WorldCollisionEvent.Type = event::EVENT_WORLD_COLLISION;
        WorldCollisionEvent.bWorldCollisionOn = AnimEvent.GetBool(anim_event::BOOL_IDX_WORLD_COLLISION);

        pObj->OnEvent(WorldCollisionEvent);
    } else if (strcmp(EventType, "Gravity") == 0) {
        //-------------------------------------------------------
        // Gravity
        //-------------------------------------------------------

        gravity_event GravityEvent;

        GravityEvent.Type = event::EVENT_GRAVITY;
        GravityEvent.bGravityOn = AnimEvent.GetBool(anim_event::BOOL_IDX_GRAVITY);

        pObj->OnEvent(GravityEvent);
    } else if (strcmp(EventType, "Weapon") == 0) {
        weapon_event WeaponEvent;

        WeaponEvent.Pos = BasePlayer.GetEventPosition(AnimEvent); //EventPos;
        WeaponEvent.Type = event::EVENT_WEAPON;
        WeaponEvent.WeaponState = AnimEvent.GetInt(anim_event::INT_IDX_WEAPON_DATA);

        pObj->OnEvent(WeaponEvent);
    } else if (strcmp(EventType, "Pain") == 0) {
        pain_event PainEvent;

        //-------------------------------------------------------
        // Pain event.
        //-------------------------------------------------------

        PainEvent.Position = BasePlayer.GetEventPosition(AnimEvent); //EventPos;
        PainEvent.Rotation = AnimEvent.GetPoint(anim_event::POINT_IDX_ROTATION);
        PainEvent.PainRadius = AnimEvent.GetFloat(anim_event::FLOAT_IDX_RADIUS);
        PainEvent.Type = event::EVENT_PAIN;
        PainEvent.PainType = AnimEvent.GetInt(anim_event::INT_IDX_PAIN_TYPE);

        HandlePainEvent(PainEvent, pObj);
    } else if (strcmp(EventType, "Debris") == 0) {
        //-------------------------------------------------------
        // Debris
        //-------------------------------------------------------

        debris_event DebrisEvent;

        DebrisEvent.Position = BasePlayer.GetEventPosition(AnimEvent);
        DebrisEvent.Orientation = BasePlayer.GetEventRotation(AnimEvent);
        DebrisEvent.MinVelocity = AnimEvent.GetFloat(anim_event::FLOAT_IDX_DEBRIS_MIN_VELOCITY);
        DebrisEvent.MaxVelocity = AnimEvent.GetFloat(anim_event::FLOAT_IDX_DEBRIS_MAX_VELOCITY);
        DebrisEvent.Life = AnimEvent.GetFloat(anim_event::FLOAT_IDX_DEBRIS_LIFE);
        DebrisEvent.Bounce = AnimEvent.GetBool(anim_event::BOOL_IDX_DEBRIS_BOUNCE);

        strncpy(DebrisEvent.MeshName, xfs("%s.rigidgeom", AnimEvent.GetString(anim_event::STRING_IDX_DEBRIS_TYPE)), 64);

        HandleDebrisEvent(DebrisEvent, pObj);
    } else if (strcmp(EventType, "Set Mesh") == 0) {
        // Turn on the mesh
        set_mesh_event SetMeshEvent;

        strncpy(SetMeshEvent.MeshName, AnimEvent.GetString(anim_event::STRING_IDX_SET_MESH), 64);
        SetMeshEvent.On = (strcmp(AnimEvent.GetString(anim_event::STRING_IDX_SET_MESH_ON_OR_OFF), "On") == 0);

        HandleSetMeshEvent(SetMeshEvent, pObj);
    } else if (strcmp(EventType, "Swap Mesh") == 0) {
        swap_mesh_event SwapMeshEvent;
        strncpy(SwapMeshEvent.OnMeshName, AnimEvent.GetString(anim_event::STRING_IDX_SWAP_MESH_ON), 64);
        strncpy(SwapMeshEvent.OffMeshName, AnimEvent.GetString(anim_event::STRING_IDX_SWAP_MESH_OFF), 64);

        HandleSwapMeshEvent(SwapMeshEvent, pObj);
    } else if (strcmp(EventType, "Swap Virtual Texture") == 0) {
        swap_virtual_texture_event SwapVTEvent;
        strncpy(SwapVTEvent.SetTextureName, AnimEvent.GetString(anim_event::STRING_IDX_SET_TEXTURE), 64);
        strncpy(SwapVTEvent.UseTextureName, AnimEvent.GetString(anim_event::STRING_IDX_USE_TEXTURE), 64);

        HandleSetVirtualTextureEvent(SwapVTEvent, pObj);
    } else if (strcmp(EventType, "Fade Geometry") == 0) {
        fade_geometry_event FadeGeomEvent;
        FadeGeomEvent.Direction = AnimEvent.GetInt(anim_event::INT_IDX_FADE_DIRECTION);
        FadeGeomEvent.FadeTime = AnimEvent.GetFloat(anim_event::FLOAT_IDX_GEOMETRY_FADE_TIME);

        HandleFadeGeometryEvent(FadeGeomEvent, pObj);
    } else if (strcmp(EventType, "Camera FOV") == 0) {
        camera_fov_event CameraFOVEvent;
        CameraFOVEvent.CameraFOV = AnimEvent.GetFloat(anim_event::FLOAT_IDX_CAMERA_FOV);
        CameraFOVEvent.CameraTime = AnimEvent.GetFloat(anim_event::FLOAT_IDX_CAMERA_FOV_TIME);

        HandleCameraFOVEvent(CameraFOVEvent, pObj);
    }
}

//==============================================================================

void event_mgr::HandleAudioEvent(const event& Event, Object* pParentObj, bool UsePosition)
{
    const audio_event& AudioEvent = audio_event::GetSafeType(Event);

    ObjectManager* objectManager = pParentObj->getObjectManager();

    //
    // If its a looped sounds then we want to check if it already exist, if it does then
    // update its position instead of creating a new one.
    //
    if (AudioEvent.SoundType == event_sound_emitter::LOOPED) {
        if (UsePosition) {
            slot_id SlotID = objectManager->GetFirst(Object::TYPE_EVENT_SND_EMITTER);
            Object* pObject = objectManager->GetObjectBySlot(SlotID);

            while (pObject) {
                event_sound_emitter& EventEmitter = event_sound_emitter::GetSafeType(*pObject);
                if ((EventEmitter.GetParentGuid() == pParentObj->GetGuid()) &&
                    (strcmp(EventEmitter.GetDescriptorName(), (const char*)AudioEvent.DescriptorName) == 0)) {
                    break;
                }

                SlotID = objectManager->GetNext(SlotID);
                pObject = objectManager->GetObjectBySlot(SlotID);
            }

            // Update the emitter.
            if (pObject == NULL) {
                guid Guid = objectManager->CreateObject(event_sound_emitter::GetObjectType());
                pObject = objectManager->GetObjectByGuid(Guid);
                assert(pObject);

                if (pObject) {
                    event_sound_emitter& EventEmitter = event_sound_emitter::GetSafeType(*pObject);

                    Vector3 temp = AudioEvent.Position;
                    EventEmitter.PlayEmitter(AudioEvent.DescriptorName,
                                             temp,
                                             pParentObj->GetZone1(),
                                             (event_sound_emitter::sound_type)AudioEvent.SoundType,
                                             pParentObj->GetGuid(),
                                             AudioEvent.Flag,
                                             0.0f,
                                             false,
                                             1.0f,
                                             !UsePosition);
                }
            }

            event_sound_emitter& EventEmitter = event_sound_emitter::GetSafeType(*pObject);
            // Check if we can calculate a new bone position
            if (AudioEvent.bUsePosition) {
                EventEmitter.OnMove(AudioEvent.Position);
            } else {
                EventEmitter.OnMove(pParentObj->GetSubPosition(0));
            }
            return;
        }
    } else {
        //
        // Create a new event sound emitter object.
        //
        guid                 Guid = objectManager->CreateObject(event_sound_emitter::GetObjectType());
        Object*              pSndObj = objectManager->GetObjectByGuid(Guid);
        event_sound_emitter& EventEmitter = event_sound_emitter::GetSafeType(*pSndObj);

        Vector3 temp = AudioEvent.Position;
        EventEmitter.PlayEmitter(AudioEvent.DescriptorName,
                                 temp, pParentObj->GetZone1(),
                                 (event_sound_emitter::sound_type)AudioEvent.SoundType,
                                 pParentObj->GetGuid(),
                                 AudioEvent.Flag,
                                 0.0f,
                                 false,
                                 1.0f,
                                 !UsePosition);
    }
}

//==============================================================================

void event_mgr::HandleParticleEvent(const event& Event, Object* pParentObj)
{
    const particle_event& ParticleEvent = particle_event::GetSafeType(Event);

    ObjectManager* objectManager = pParentObj->getObjectManager();
    //
    // Check if a particle event for this object has already been created.
    //
    slot_id SlotID = objectManager->GetFirst(Object::TYPE_PARTICLE_EVENT_EMITTER);
    Object* pObject = objectManager->GetObjectBySlot(SlotID);

    while (pObject) {
        particle_event_emitter& ParticleEventEmitter = particle_event_emitter::GetSafeType(*pObject);

        // If the particle emitter has the same parent, name and event id then that mean we just have to update it.
        if ((ParticleEventEmitter.GetParentGuid() == pParentObj->GetGuid()) &&
            (strcmp(ParticleEventEmitter.GetFxName(), (const char*)ParticleEvent.ParticleName) == 0) &&
            (ParticleEvent.EventIndex == ParticleEventEmitter.GetEventID())) {
            break;
        }

        SlotID = objectManager->GetNext(SlotID);
        pObject = objectManager->GetObjectBySlot(SlotID);
    }

    // Update the emitter.
    if (pObject != NULL) {
        particle_event_emitter& ParticleEventEmitter = particle_event_emitter::GetSafeType(*pObject);

        if (ParticleEvent.DoNotAppyTransform == false) {
            m_Tranform.Identity();

            //radian3 Orient;
            //Orient.Roll = R_0;
            //ParticleEvent.Orientation.GetPitchYaw( Orient.Pitch, Orient.Yaw );

            m_Tranform.SetTranslation(ParticleEvent.Position);
            m_Tranform.SetRotation(ParticleEvent.Orientation);

            ParticleEventEmitter.OnTransform(m_Tranform);
        }

        ParticleEventEmitter.EnableUpdate();

        return;
    } else {
        // Create a particle event object.
        guid                    Guid = objectManager->CreateObject(particle_event_emitter::GetObjectType());
        Object*                 pPrtEvtObj = objectManager->GetObjectByGuid(Guid);
        particle_event_emitter& ParticleEventEmitter = particle_event_emitter::GetSafeType(*pPrtEvtObj);

        ParticleEventEmitter.StartEmitter(ParticleEvent.ParticleName, ParticleEvent.Position,
                                          ParticleEvent.Orientation, pParentObj->GetZone1(), pParentObj->GetGuid(),
                                          ParticleEvent.EventIndex, ParticleEvent.EventActive);
    }
}

//==============================================================================

void event_mgr::HandleHotPointEvent(const event& Event, Object* pParentObj)
{
    const hotpoint_event& HotPointEvent = hotpoint_event::GetSafeType(Event);
    (void)HotPointEvent;
    (void)pParentObj;
}

//==============================================================================

void event_mgr::HandleGenericEvent(const event& Event, Object* pParentObj)
{
    //    const generic_event& GenericEvent = generic_event::GetSafeType( Event );
    pParentObj->OnEvent(Event);
}

//==============================================================================

void event_mgr::HandleIntensityEvent(const event& Event, Object* pParentObj)
{
    const intensity_event& IntensityEvent = intensity_event::GetSafeType(Event);

    if (pParentObj && pParentObj->IsKindOf(new_weapon::GetRTTI())) {
        new_weapon* pWeapon = (new_weapon*)pParentObj;
        Object*     pObj = pParentObj->getObjectManager()->GetObjectByGuid(pWeapon->GetParentGuid());

        if (pObj && pObj->IsKindOf(player::GetRTTI())) {
            player* pPlayer = (player*)pObj;
            pPlayer->OnEvent(IntensityEvent);
            return;
        }
    } else if (pParentObj && pParentObj->IsKindOf(player::GetRTTI())) {
        player* pPlayer = (player*)pParentObj;
        pPlayer->OnEvent(IntensityEvent);
        return;
    }

    // was player* pPlayer = SMP_UTIL_GetActivePlayer();
    ObjectManager* objectManager = pParentObj->getObjectManager();
    slot_id PlayerSlot = objectManager->GetFirst( Object::TYPE_PLAYER ) ;
    player* pPlayer = (player*)objectManager->GetObjectBySlot( PlayerSlot ) ;
    if (pPlayer) {
        pPlayer->OnEvent(IntensityEvent);
    }
}

//==============================================================================
#include "objects/Player.h"

void event_mgr::HandleDebrisEvent(const event& Event, Object* pParentObj)
{
    int BounceSound = -1;
    (void)pParentObj;
    const debris_event& DebrisEvent = debris_event::GetSafeType(Event);

    // The vector needs to start facing Y up since that represents -Z in max and that the axis we are shooting down.
    Vector3 RotateVector(0.0f, 1.0f, 0.0f);
    RotateVector.Rotate(DebrisEvent.Orientation);
    //( DebrisEvent.Orientation.Pitch, DebrisEvent.Orientation.Yaw, DebrisEvent.Orientation.Roll );

    RotateVector.NormalizeAndScale(x_frand(DebrisEvent.MinVelocity, DebrisEvent.MaxVelocity));

    Vector3 InheritedVelocity(0.0f, 0.0f, 0.0f);
    if (pParentObj && pParentObj->IsKindOf(new_weapon::GetRTTI())) {
        new_weapon& Weapon = *((new_weapon*)pParentObj);
        InheritedVelocity = Weapon.GetVelocity();

        const float Radius = (DebrisEvent.Position - Weapon.GetPosition()).Length();
        const float ArcSpeed = Radius * Weapon.GetAngularSpeed(); // s = r(Theta)
        const float Yaw = Weapon.GetYaw();
        Vector3   Dir;
        if (ArcSpeed > 0) {
            Dir.set(1.0f, 0.0f, 0.0f);
        } else {
            Dir.set(-1.0f, 0.0f, 0.0f);
        }

        Dir.RotateY(Yaw);
        InheritedVelocity += (Dir * ArcSpeed);
/* IJB
        if (pParentObj->GetType() == Object::TYPE_WEAPON_SHOTGUN) {
            BounceSound = g_StringMgr.Add("SHT_ShellDropConcrete");
        } else {
            BounceSound = g_StringMgr.Add("SMP_ShellDropConcrete");
        }
            */
    }
    static const float ONE_FRAME = 0.03f;
/* IJB
    debris_mgr::GetDebrisMgr()->CreateDebris(DebrisEvent.Position + (InheritedVelocity * ONE_FRAME),
                                             pParentObj->GetZones(),
                                             InheritedVelocity + RotateVector,
                                             DebrisEvent.MeshName,
                                             DebrisEvent.Life,
                                             DebrisEvent.Bounce,
                                             U32_MAX,
                                             BounceSound);
*/
}

void event_mgr::HandlePainEvent(event& Event, Object* pParentObj)
{
    pain_event& PainSuperEvent = pain_event::GetSafeType(Event);
    pParentObj->OnEvent(PainSuperEvent);
}

//==============================================================================

float event_mgr::ClosestPointToAABox(const Vector3& Point, const BBox& Box, Vector3& ClosestPoint)
{
    float SqrDistance = 0.0f;
    float Delta;

    if (Point.x <= Box.min.x) {
        Delta = Point.x - Box.min.x;
        SqrDistance += Delta * Delta;
        ClosestPoint.x = Box.min.x;
    } else if (Point.x > Box.min.x) {
        Delta = Point.x - Box.min.x;
        SqrDistance += Delta * Delta;
        ClosestPoint.x = Box.min.x;
    } else {
        ClosestPoint.x = Point.x;
    }

    if (Point.y <= Box.min.y) {
        Delta = Point.y - Box.min.y;
        SqrDistance += Delta * Delta;
        ClosestPoint.y = Box.min.y;
    } else if (Point.y > Box.min.y) {
        Delta = Point.y - Box.min.y;
        SqrDistance += Delta * Delta;
        ClosestPoint.y = Box.min.y;
    } else {
        ClosestPoint.y = Point.y;
    }

    if (Point.z <= Box.min.z) {
        Delta = Point.z - Box.min.z;
        SqrDistance += Delta * Delta;
        ClosestPoint.z = Box.min.z;
    } else if (Point.z > Box.min.z) {
        Delta = Point.z - Box.min.z;
        SqrDistance += Delta * Delta;
        ClosestPoint.z = Box.min.z;
    } else {
        ClosestPoint.z = Point.z;
    }

    return SqrDistance;
}

//=============================================================================

void event_mgr::HandleSetMeshEvent(const event& Event, Object* pParentObj)
{
    if (pParentObj) {
        const set_mesh_event& SetMeshEvent = set_mesh_event::GetSafeType(Event);
        Geom*                 pGeom = pParentObj->GetGeomPtr();
        render_inst*          pRenderInst = pParentObj->GetRenderInstPtr();

        if (pGeom && pRenderInst) {
            int Index = pGeom->GetVMeshIndex(SetMeshEvent.MeshName);
            pRenderInst->SetVMeshBit(Index, SetMeshEvent.On);
        }
    }
}

//=============================================================================

void event_mgr::HandleSwapMeshEvent(const event& Event, Object* pParentObj)
{
    if (pParentObj) {
        const swap_mesh_event& SwapMeshEvent = swap_mesh_event::GetSafeType(Event);
        Geom*                  pGeom = pParentObj->GetGeomPtr();
        render_inst*           pRenderInst = pParentObj->GetRenderInstPtr();

        if (pGeom && pRenderInst) {
            int Index = pGeom->GetVMeshIndex(SwapMeshEvent.OnMeshName);
            pRenderInst->SetVMeshBit(Index, true);

            Index = pGeom->GetVMeshIndex(SwapMeshEvent.OffMeshName);
            pRenderInst->SetVMeshBit(Index, false);
        }
    }
}

//=============================================================================

void event_mgr::HandleSetVirtualTextureEvent(const event& Event, Object* pParentObj)
{
    if (pParentObj) {
        const swap_virtual_texture_event& SwapVTEvent = swap_virtual_texture_event::GetSafeType(Event);
        render_inst*                      pRenderInst = pParentObj->GetRenderInstPtr();

        // this only works for actors.
        if (pRenderInst && pParentObj->IsKindOf(actor::GetRTTI())) {
            skin_inst* pSkinInst = (skin_inst*)pRenderInst;
            pSkinInst->SetVirtualTexture(SwapVTEvent.SetTextureName, SwapVTEvent.UseTextureName);
        }
    }
}

//=============================================================================

void event_mgr::HandleFadeGeometryEvent(const event& Event, Object* pParentObj)
{
    if (pParentObj) {
        const fade_geometry_event& FadeGeomEvent = fade_geometry_event::GetSafeType(Event);
        render_inst*               pRenderInst = pParentObj->GetRenderInstPtr();

        if (pRenderInst) {
            pRenderInst->StartFade((int8_t)FadeGeomEvent.Direction, FadeGeomEvent.FadeTime);
        }
    }
}

//=============================================================================

void event_mgr::HandleCameraFOVEvent(const event& Event, Object* pParentObj)
{

}
