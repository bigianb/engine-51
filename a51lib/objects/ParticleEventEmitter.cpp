
#include "ParticleEventEmitter.h"
#include "../animation/animData.h"
#include "Event.h"
//#include "ParticleEmiter.h"


static struct particle_event_emitter_desc : public object_desc
{
    particle_event_emitter_desc() : object_desc( 
        Object::TYPE_PARTICLE_EVENT_EMITTER, 
        "Particle Event Emitter", 
        "EFFECTS",
            Object::ATTR_NEEDS_LOGIC_TIME,
            FLAGS_IS_DYNAMIC) {}         

    //---------------------------------------------------------------------

    virtual Object* Create(ObjectManager* om, collision_mgr*, ResourceManager*)
    {
        return new particle_event_emitter(om);
    }


} s_ParticleEventEmitter_Desc;

const object_desc&  particle_event_emitter::GetTypeDesc() const
{
    return s_ParticleEventEmitter_Desc;
}

const object_desc&  particle_event_emitter::GetObjectType()
{
    return s_ParticleEventEmitter_Desc;
}

particle_event_emitter::particle_event_emitter(ObjectManager* om) : Object(om) 
{
    m_FxName[0]     = 0;
    m_EventID       = -1;
    m_ParentGuid    = 0;
    m_ParticleGuid  = 0;
    m_LogicRunning  = true;
    m_EventActive   = false;
}

void particle_event_emitter::OnAdvanceLogic ( float DeltaTime )
{    
    if( m_EventActive )
    {
        // If this object wasn't updated last cycle then that means that we are done so stop the effect and destory
        // this object.
        if( m_LogicRunning == false )
        {
            Object* pObj = objectManager->GetObjectByGuid( m_ParticleGuid );
            if( pObj )
            {
                /* IJB
                particle_emitter& Particle = particle_emitter::GetSafeType( *pObj );
                Particle.DestroyParticle();
                */
            }
            
            objectManager->DestroyObject( GetGuid() );
        }
        
        m_LogicRunning = false;
    }
    else
    {
        // If the particle is done playing, kill this object.
        if( objectManager->GetObjectByGuid( m_ParticleGuid ) == NULL )
            objectManager->DestroyObject( GetGuid() );
    }
}

//=========================================================================

void particle_event_emitter::StartEmitter( const char*      pFx,
                                           const Vector3&   Position,
                                           const Vector3&   Rotation,
                                           uint16_t              ZoneID,
                                           guid             ParentGuid,
                                           int              EventID,
                                           bool            EventActive )
{
    strncpy( m_FxName, pFx, 64 );
    m_EventID       = EventID;
    m_ParentGuid    = ParentGuid;
    m_EventActive   = EventActive;
    m_LogicRunning  = true;

    m_ParticleGuid  = 0; // IJBparticle_emitter::CreatePresetParticleAndOrient( m_FxName, Rotation, Position, ZoneID );

    OnMove( Position );
}   

//=========================================================================

void particle_event_emitter::OnMove( const Vector3& NewPos )
{
    Object::OnMove( NewPos );
    
    Object* pObj = objectManager->GetObjectByGuid( m_ParticleGuid );

    if( pObj )
        pObj->OnMove( NewPos );

    // If we are getting updated that mean this particle is still "active".
    m_LogicRunning = true;
}

//=========================================================================

void particle_event_emitter::OnTransform( const Matrix4& L2W      )
{
    Object::OnTransform( L2W );

    Object* pObj = objectManager->GetObjectByGuid( m_ParticleGuid );

    if( pObj )
        pObj->OnTransform( L2W );

    // If we are getting updated that mean this particle is still "active".
    m_LogicRunning = true;
}

//=========================================================================

BBox particle_event_emitter::GetLocalBBox() const
{
    BBox Box( Vector3(0.0f, 0.0f, 0.0f), 0.0f );
    return Box;
}

