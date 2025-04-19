#pragma once

#include "Object.h"
#include "../VectorMath.h"
#include "../resourceManager/ResourceManager.h"
#include "../inventory/Inventory.h"

class actor;

class pickup : public Object
{
protected:
    enum state
    {
        STATE_IDLE,         // -> DECAYING, WAIT_RESPAWN
        STATE_DECAYING,     // -> FADE_OUT
        STATE_FADE_OUT,     // -> DESTROY
        STATE_WAIT_RESPAWN, // -> RESPAWNING
        STATE_RESPAWNING,   // -> IDLE,
        STATE_DESTROY,      // Terminal
        STATE_FIRST = STATE_IDLE,
        STATE_LAST = STATE_DESTROY,
    };

    enum
    {
        DIRTY_STATE = 0x00010000,
        DIRTY_REQUEST = 0x00020000,
        DIRTY_ASSIGN = 0x00040000,
    };

public:
    pickup();
    ~pickup();

    static pickup* CreatePickup(guid           OriginGuid,
                                int            OriginNetSlot,
                                inven_item     Item,
                                float          Amount,
                                float          DecayTime,
                                const Vector3& Position,
                                const Radian3& Orientation,
                                const Vector3& Velocity,
                                int            Zone1,
                                int            Zone2);

    virtual bool IsActive() { return m_bIsActive; }
    virtual void OnActivate(bool Flag);
    virtual BBox GetLocalBBox() const;
    virtual void OnEnumProp(prop_enum& rList);
    virtual bool OnProperty(prop_query& rPropQuery);

    virtual void OnRender();
    virtual void OnColCheck();
    //               rigid_inst&     GetRigidInst        () { return( m_Inst ); }

    virtual void OnMove(const Vector3& NewPos);
    virtual void OnTransform(const Matrix4& L2W);

    //virtual         render_inst*    GetRenderInstPtr    () { return &m_Inst; }

    virtual const object_desc& GetTypeDesc() const;
    static const object_desc&  GetObjectType();

    inven_item GetItem() const { return m_Item; }
    float      GetAmount() const { return m_Amount; }

    void PlayerRequest(int PlayerIndex);
    bool GetTakeable() { return m_bTakeable; }

protected:
    virtual void OnInit();
    virtual void OnKill();
    virtual void OnAdvanceLogic(float DeltaTime);
    virtual void OnColNotify(Object& Object);

    void ProcessTake(actor& Actor);
    void SetupRigidGeom(const char* pGeomName = NULL);
    bool ShouldHidePickup();

    const char* GetSound();

protected:
    inven_item m_Item;   // Item to pickup (defined in inventory.hpp)
    float      m_Amount; // Amount to pickup

    state m_State;     // State machine for pickup
    bool  m_IsDynamic; // Is a dynamically created pickup

    float m_RespawnTime;             // Time it takes for pickup to respawn
    float m_DecayTime;               // Time until pickup decays
    float m_Timer;                   // Timer for next respawn
                                     //               rigid_inst      m_Inst;                 // Render Instance
    ResourceHandle<char> m_hAudioPackage;   // Audio resource
    ResourceHandle<char> m_hParticleEffect; // Effect resource
    guid          m_FXGuid;          // FX

    int m_PlayerIndex; // Player touching/receiving pickup

    bool m_bIsActive;          // is this item activated?
    bool m_bHideWhileInactive; // do we hide this pickup if it's inactive?
    bool m_bHasBeenPickedup;   // has someone picked this up? (resets to false after respawn)
    bool m_bTakeable;          // Can this currently be picked up?

    int m_MinPlayers;
    int m_MaxPlayers;

    bool    m_bSpins;
    Matrix4 m_GeomL2W;
    Radian  m_Rotation;
};
