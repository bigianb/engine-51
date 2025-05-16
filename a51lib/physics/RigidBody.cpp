

#include "RigidBody.h"
#include "PhysicsMgr.h"
#include "CollisionShape.h"

rigid_body::rigid_body()
{
    // Constant properties
    m_Mass	 = 1.0f;
    m_InvMass = 1.0f;
    m_BodyInvInertia.set( 1.0f, 1.0f, 1.0f );
    m_Elasticity = 0.7f;
    m_StaticFriction = 0.8f;
    m_DynamicFriction = 0.6f;
    m_LinearDamping = 0.05f;
    m_AngularDamping = 0.15f;
    m_pCollisionShape = NULL;

    // Status information
    m_Flags = FLAG_WORLD_COLLISION | FLAG_IS_ACTIVE;
    m_WorldBBox.Clear();
    m_InactiveTime = 0.0f;

    // State variables
    m_L2W.Identity();
    m_WorldBBox.Clear();
    m_BackupState.m_L2W.Identity();
    m_BackupState.m_LinearVelocity.Zero();
    m_BackupState.m_AngularVelocity.Zero();

    // Derived quantities (auxilary variables)
    m_WorldInvInertia.Identity();
    m_LinearVelocity.Zero();
    m_AngularVelocity.Zero();

    // Computed quantities about center of mass
    m_Force.Zero();
    m_Torque.Zero();

    // Collision info
    m_CollisionGroup    = -1;
    m_CollisionID       = -1;
    m_CollisionBit      = 0xFFFFFFFF;
    m_CollisionMask     = 0xFFFFFFFF;
    m_CollisionSpeedSqr = 0.0f;

    // Constraint info
    m_pPivotConstraint = NULL;
}

void rigid_body::SetCollisionShape( collision_shape* pCollisionShape, float Mass, float InertiaMax )
{
    // Setup inertia
    if( Mass != 0.0f )
    {
        // Lookup BBox info
        BBox    LocalBBox = pCollisionShape->ComputeLocalBBox();
        Vector3 Size      = LocalBBox.GetSize();
        float     X         = Size.x;
        float     Y         = Size.y;
        float     Z         = Size.z;

        // Keep physics stable
        X = std::max( X, InertiaMax );
        Y = std::max( Y, InertiaMax );
        Z = std::max( Z, InertiaMax );
        
        // Setup approximate inertia based off box
        Vector3 I;
        I.x = (1.0f / 12.0f) * Mass * ( ( Y * Y ) + ( Z * Z ) );
        I.y = (1.0f / 12.0f) * Mass * ( ( X * X ) + ( Z * Z ) );
        I.z = (1.0f / 12.0f) * Mass * ( ( X * X ) + ( Y * Y ) );
        SetBodyInertia( I );
        
        // Set mass
        SetMass( Mass );
    }
    else
    {
        // Make immovable
        SetInfiniteMass();
    }

    // Remove old collision?
    if ( m_pCollisionShape )
        m_pCollisionShape->SetOwner( NULL );

    // Set new collision
    m_pCollisionShape = pCollisionShape;
    m_pCollisionShape->SetOwner( this );
    
    // Init position of collision shape
    m_pCollisionShape->SetL2W( GetL2W() );
}

//==============================================================================

inline
void ClampVelocity( Vector3& V, float Max )
{
    float SpeedSqr = V.LengthSquared();
    if( SpeedSqr > x_sqr( Max ) )
    {
        float Scale = Max / sqrt( SpeedSqr );
        V *= Scale;
    }
}

//==============================================================================

void rigid_body::ClampVelocities(  )
{
    // Clamp velocities
    ClampVelocity( m_LinearVelocity, g_PhysicsMgr.m_Settings.m_MaxLinearVel );
    ClampVelocity( m_AngularVelocity, g_PhysicsMgr.m_Settings.m_MaxAngularVel );
}

//==============================================================================

void rigid_body::SetL2W( const Matrix4& L2W )
{
    assert( L2W.IsValid() );
    
    // Keep new L2W
    m_L2W = L2W;
    
    // Compute world space inverse inertia
    ComputeWorldInvInertia();
}

//==============================================================================

void rigid_body::ComputeForces( float DeltaTime )
{
    // Add damping
    m_Force  -= m_LinearDamping  * m_LinearVelocity * m_Mass * DeltaTime;
    m_Torque -= m_AngularDamping * m_AngularVelocity * m_Mass * DeltaTime;

    // Add gravity
    if( HasCollided() ) // Reduce gravity if colliding with something to reduce jitter some!
        AddWorldForce( m_Mass * g_PhysicsMgr.m_Settings.m_Gravity * 0.5f );
    else        
        AddWorldForce( m_Mass * g_PhysicsMgr.m_Settings.m_Gravity );
}

//==============================================================================

inline
void SetupAngularVelocityMatrix( Matrix4& M, const Vector3& Vel )
{
    M( 0, 0 ) =  0.0f;
    M( 0, 1 ) =  Vel[2];
    M( 0, 2 ) = -Vel[1];
    M( 0, 3 ) =  0.0f;
         
    M( 1, 0 ) = -Vel[2];
    M( 1, 1 ) =  0.0f;
    M( 1, 2 ) =  Vel[0];
    M( 1, 3 ) =  0.0f;
         
    M( 2, 0 ) =  Vel[1];
    M( 2, 1 ) = -Vel[0];
    M( 2, 2 ) =  0.0f;
    M( 2, 3 ) =  0.0f;
    
    M( 3, 0 ) =  0.0f;
    M( 3, 1 ) =  0.0f;
    M( 3, 2 ) =  0.0f;
    M( 3, 3 ) =  0.0f;
}

//==============================================================================

void rigid_body::IntegratePosition( float DeltaTime )
{
    // Immovable?
    if ( m_Mass == 0.0f )
        return;

    // Clamp velocities to keep stable
    ClampVelocities();

    // Blend in/out when activating/deactivating
    //DeltaTime *= GetActiveBlend();
    
    // Lookup translation and remove from L2W (so it's now orientation only)
    Vector3 Position = m_L2W.GetTranslation();
    m_L2W.ClearTranslation();

    // Integrate position
    Position += DeltaTime * m_LinearVelocity;

    // Compute delta rotation
    Matrix4 DeltaRotation;
    SetupAngularVelocityMatrix( DeltaRotation, m_AngularVelocity * DeltaTime );

    // Integrate orientation
    m_L2W += DeltaRotation * m_L2W;
    m_L2W.Orthogonalize();
    
    // Compute world space inverse as follows (operations read from right->left):
    // m_WorldInvInertia = InvOrient * BodyInvInertia * Orient

    // Since the "BodyInvInertia" represents the diagonal elements of the inverse 
    // inertia matrix, the computation can be optimized to a matrix scale
    Matrix4 Orient    = m_L2W;  // NOTE: Translation is zero at this point
    Matrix4 InvOrient = m4_Transpose( Orient );
    Orient.Scale( m_BodyInvInertia );   // ie. BodyInvInertia * Orient
    m_WorldInvInertia = InvOrient * Orient;
    
    // Finally, put translation back into L2W
    m_L2W.SetTranslation( Position );
}

//==============================================================================

void rigid_body::IntegrateVelocity( float DeltaTime )
{
    // Immovable?
    if ( m_Mass == 0.0f )
        return;
    
    // Integrate
    m_LinearVelocity  += ( DeltaTime * m_InvMass ) * m_Force;
    m_AngularVelocity += m_WorldInvInertia.RotateVector( DeltaTime * m_Torque );
    
    // Clamp velocities to keep stable
    ClampVelocities();
}

//==============================================================================

void rigid_body::UpdateActiveState( float DeltaTime )
{
    // Allow active?
    if( g_PhysicsMgr.m_Settings.m_bDeactivate == false )
    {
        // Force active
        m_InactiveTime = 0.0f;
        m_Flags |= FLAG_IS_ACTIVE;
        return;    
    }

    // Active energy?
    float DeactivateEndTime = g_PhysicsMgr.m_Settings.m_DeactivateEndTime;
    if( HasActiveEnergy() )
    {
        // Decrease inactive time so integration is blended back in
        m_InactiveTime -= DeltaTime;
        if( m_InactiveTime < 0.0f )
            m_InactiveTime = 0.0f;
    }
    else
    {    
        // Increase inactive time so integration is blended out
        m_InactiveTime += DeltaTime;
        if( m_InactiveTime > DeactivateEndTime )
            m_InactiveTime = DeactivateEndTime;
    }

    // Update active flag    
    if( m_InactiveTime < DeactivateEndTime )
        m_Flags |= FLAG_IS_ACTIVE;
    else        
        m_Flags &= ~FLAG_IS_ACTIVE;
}

//==============================================================================

bool rigid_body::HasActiveEnergy( void ) const
{
    // Above linear energy?
    float LinearSpeedSqr  = m_LinearVelocity.LengthSquared();
    if( LinearSpeedSqr > x_sqr( g_PhysicsMgr.m_Settings.m_ActiveLinearSpeed ) )
        return true;
        
    // Above angular energy?        
    float AngularSpeedSqr = m_AngularVelocity.LengthSquared();
    if( AngularSpeedSqr > x_sqr( g_PhysicsMgr.m_Settings.m_ActiveAngularSpeed ) )
        return true;
        
    // Both linear and angular energy are below threshold        
    return false;
}

//==============================================================================

float rigid_body::GetActiveBlend( void ) const
{
    // Lookup blend info
    float BlendStartTime = g_PhysicsMgr.m_Settings.m_DeactivateStartTime;
    float BlendEndTime   = g_PhysicsMgr.m_Settings.m_DeactivateEndTime;
    float BlendTime      = BlendEndTime - BlendStartTime;
    assert( BlendStartTime < BlendEndTime );
    assert( BlendTime > 0.0f );
    
    // Fully active?
    if( m_InactiveTime <= BlendStartTime )
        return 1.0f;
    
    // Fully inactive?
    if( m_InactiveTime >= BlendEndTime )
        return 0.0f;
        
    // Compute blend ratio:
    //      Blend = 1.0f when m_InactiveTime = BlendStartTime
    //      Blend = 0.0f when m_InactiveTime = BlendEndTime
    float Blend = 1.0f - ( ( m_InactiveTime - BlendStartTime ) / BlendTime );
    assert( Blend >= 0.0f );
    assert( Blend <= 1.0f );
    return Blend;
}

void rigid_body::Deactivate(  )
{
    // Clear movement
    ZeroVelocity();
    ClearForces();

    // Set inactive time to high value
    m_InactiveTime = 100.0f;
    m_Flags &= ~FLAG_IS_ACTIVE;
}


void rigid_body::Activate(  )
{
    // Clear inactive time
    m_InactiveTime = 0.0f;
    m_Flags |= FLAG_IS_ACTIVE;
}
