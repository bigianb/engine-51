#pragma once

#include "animData.h"

class base_player
{
public:
    enum mix_buffer
    {
        MIX_BUFFER_PLAYER,     // Main animation player buffer
        MIX_BUFFER_CONTROLLER, // Controller buffer
        MIX_BUFFER_TEMP,       // General mixer

        MIX_BUFFER_COUNT // Total
    };

public:
    base_player() {}
    virtual ~base_player() {}

    // Event functions
    virtual int               GetNEvents() = 0;
    virtual bool              IsEventActive(int iEvent) = 0;
    virtual const anim_event& GetEvent(int iEvent) = 0;
    virtual Vector3           GetEventPosition(int iEvent) = 0;              // World position of event.
    virtual Radian3           GetEventRotation(int iEvent) = 0;              // World rotation of event.
    virtual Vector3           GetEventPosition(const anim_event& Event) = 0; // World position of event.
    virtual Radian3           GetEventRotation(const anim_event& Event) = 0; // World rotation of event.
    virtual const char*       GetAnimName() { return "ANIM_UNKNOWN"; }

    // Mix buffer functions
    static AnimKey* GetMixBuffer(mix_buffer MixBuffer); // Returns mix buffer address
};
