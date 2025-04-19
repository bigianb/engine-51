#pragma once

#include "VectorMath.h"

class char_anim_player;
class loco_char_anim_player;
class loco_anim_controller;
class simple_anim_player;
class anim_event;
class base_player;
class event;
class Object;

class event_mgr
{
public:
    event_mgr();
    ~event_mgr();

    void HandleSuperEvents(char_anim_player& CharAnimPlayer, Object* pObj);
    void HandleSuperEvents(loco_char_anim_player& CharAnimPlayer, Object* pObj);
    void HandleSuperEvents(simple_anim_player& SimpleAnimPlayer, Object* pObj);
    void HandleSuperEvents(loco_char_anim_player& CharAnimPlayer, loco_anim_controller& LocoAnimController, Object* pObj);
    float  ClosestPointToAABox(const Vector3& Point, const BBox& Box, Vector3& ClosestPoint);

protected:
    void HandleAnimEvents(const anim_event& AnimEvent, Object* pObj, int EventIndex, base_player& BasePlayer);

    void HandleAudioEvent(const event& Event, Object* pParentObj, bool UsePosition);
    void HandleParticleEvent(const event& Event, Object* pParentObj);
    void HandleHotPointEvent(const event& Event, Object* pParentObj);
    void HandleGenericEvent(const event& Event, Object* pParentObj);
    void HandleIntensityEvent(const event& Event, Object* pParentObj);
    void HandleDebrisEvent(const event& Event, Object* pParentObj);
    void HandlePainEvent(event& Event, Object* pParentObj);
    void HandleSetMeshEvent(const event& Event, Object* pParentObj);
    void HandleSwapMeshEvent(const event& event, Object* pParentObj);
    void HandleFadeGeometryEvent(const event& Event, Object* pParentObj);
    void HandleSetVirtualTextureEvent(const event& event, Object* pParentObj);
    void HandleCameraFOVEvent(const event& Event, Object* pParentObj);

    Matrix4 m_Tranform;

protected:
};

extern event_mgr g_EventMgr;
