#pragma once

#include "../RTTI.h"
#include "../VectorMath.h"

#define GUN_SHOT_EVENT 1
#define EXPLOSION_EVENT 2
#define VOICE_EVENT 3

struct event
{
    CREATE_RTTI_BASE(event);

    enum event_type
    {
        EVENT_AUDIO,
        EVENT_PARTICLE,
        EVENT_HOTPOINT,
        EVENT_VOICE,
        EVENT_GENERIC,
        EVENT_INTENSITY,
        EVENT_WORLD_COLLISION,
        EVENT_GRAVITY,
        EVENT_WEAPON,
        EVENT_PAIN,
        EVENT_DIALOG,
        EVENT_TEXTURE_SWAP,
        EVENT_CAMERA_FOV,
    };

    event_type Type;
};

//--------------------------------------------------------------------

struct audio_event : public event
{
    CREATE_RTTI(audio_event, event, event);

    char     DescriptorName[64];
    Vector3  Position;
    uint32_t SoundType;
    uint32_t Flag;
    bool     bUsePosition;
};

//--------------------------------------------------------------------

struct particle_event : public event
{
    CREATE_RTTI(particle_event, event, event);

    char     ParticleName[64];
    Vector3  Position;
    Radian3  Orientation;
    int      lParams;
    uint32_t Flags;
    int      EventIndex;
    bool     EventActive;
    bool     DoNotAppyTransform;
};

//--------------------------------------------------------------------

struct hotpoint_event : public event
{
    CREATE_RTTI(hotpoint_event, event, event);

    char    HotPointType[64];
    Vector3 Position;
    Vector3 Orientation;
};

//--------------------------------------------------------------------

struct voice_event : public event
{
    CREATE_RTTI(voice_event, event, event);

    int VoiceID;
};

//--------------------------------------------------------------------

struct dialog_event : public event
{
    CREATE_RTTI(dialog_event, event, event);

    bool HotVoice;
    char DialogName[64];
};

//--------------------------------------------------------------------
struct generic_event : public event
{
    CREATE_RTTI(generic_event, event, event);

    char GenericType[64];
    int  EventIndex;
};

//--------------------------------------------------------------------

struct intensity_event : public event
{
    CREATE_RTTI(intensity_event, event, event);

    float ControllerIntensity;
    float ControllerDuration;
    float CameraShakeTime;
    float CameraShakeAmount;
    float CameraShakeSpeed;
    float BlurIntensity;
    float BlurDuration;
};

//--------------------------------------------------------------------

struct world_collision_event : public event
{
    CREATE_RTTI(world_collision_event, event, event);

    bool bWorldCollisionOn;
};

//--------------------------------------------------------------------

struct gravity_event : public event
{
    CREATE_RTTI(gravity_event, event, event);

    bool bGravityOn;
};

//--------------------------------------------------------------------

struct weapon_event : public event
{
    CREATE_RTTI(weapon_event, event, event);

    Vector3 Pos;
    int     WeaponState;
};

//--------------------------------------------------------------------

struct pain_event : public event
{
    CREATE_RTTI(pain_event, event, event);

    enum event_pain_type
    {
        EVENT_PAIN_MELEE,
        EVENT_PAIN_LEAP_CHARGE,
        EVENT_PAIN_SPECIAL,
    };
    Vector3 Position;
    Vector3 Rotation;
    float   PainRadius;
    int     PainType;

    int PainEventID;

    static int CurrentEventID;
};

//--------------------------------------------------------------------

struct debris_event : public event
{
    CREATE_RTTI(debris_event, event, event);

    char    MeshName[64];
    Vector3 Position;
    Radian3 Orientation;
    float   MinVelocity;
    float   MaxVelocity;
    float   Life;
    bool    Bounce;
};

//--------------------------------------------------------------------

struct set_mesh_event : public event
{
    CREATE_RTTI(set_mesh_event, event, event);

    char MeshName[64];
    bool On;
};

//--------------------------------------------------------------------

struct swap_mesh_event : public event
{
    CREATE_RTTI(swap_mesh_event, event, event);

    char OnMeshName[64];
    char OffMeshName[64];
};

//--------------------------------------------------------------------

struct fade_geometry_event : public event
{
    CREATE_RTTI(fade_geometry_event, event, event);

    int   Direction;
    float FadeTime;
};

//--------------------------------------------------------------------

struct swap_virtual_texture_event : public event
{
    CREATE_RTTI(swap_virtual_texture_event, event, event);

    char SetTextureName[64];
    char UseTextureName[64];
};

struct camera_fov_event : public event
{
    CREATE_RTTI(camera_fov_event, event, event);

    float CameraFOV;
    float CameraTime;
};
