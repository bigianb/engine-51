#pragma once

#include "Object.h"
#include "../view/View.h"
#include "../zoneManager/ZoneManager.h"

//=========================================================================
// THIRD_PERSON_CAMERA
//=========================================================================
class third_person_camera : public Object
{
public:
    virtual const object_desc& GetTypeDesc              ( void ) const;
    static  const object_desc& GetObjectType            ( void );
    virtual BBox            GetLocalBBox                ( void ) const { return BBox( Vector3( 0.0f, 0.0f, 0.0f ), Vector3( 1.0f, 1.0f, 1.0f ) ); }
    virtual int             GetMaterial                 ( void ) const { return 0;}
                            third_person_camera         ( void );
            void            Setup                       ( const Vector3& InitialOrbitPoint, 
                                                          const Vector3& IdealAimDirection, 
                                                          float            StartDist, 
                                                          float            EndDist,
                                                          Object*        pOrbitObject );
            void            ComputeView                 ( view& View ) const;

            void            MoveTowards                 ( const Vector3& DesiredPosition );
            void            MoveTowards                 ( Radian Pitch, Radian Yaw, float Distance );
            void            RotateYaw                   ( Radian DeltaYaw );
            void            MoveTowardsPitch            ( Radian NewPitch );
            void            SetOrbitPoint               ( const Vector3& DesiredOrbitPoint );

            const Vector3&  GetOrbitPoint               ( void ) const { return m_OrbitPoint; }
            float             GetDistance                 ( void ) const { return m_Distance; }
    virtual void            OnAdvanceLogic              ( float DeltaTime );      
            bool           HaveClearView               ( void ) const;

protected:
            bool           CheckForObstructions        ( const Vector3& Dir, float DistToCheck, float& MaxDistFound );

private:
    Vector3         m_OrbitPoint;
    Vector3         m_DesiredOrbitPoint;
    Vector3         m_OrbitPointVelocity;

    float             m_Distance;
    float             m_DesiredDistance;
    float             m_DistanceVelocity;
    float             m_DistanceAcceleration;

    Radian          m_Pitch;
    Radian          m_DesiredPitch;
    Radian          m_PitchVelocity;

    Radian          m_Yaw;
    Radian          m_DesiredYaw;
    Radian          m_YawVelocity;

    guid            m_HostPlayerGuid;

    Vector3         m_CameraRodEnds[2];
    float             m_CameraRodLength;
    int             m_iDesiredRodEnd;
    Vector3         m_CameraPos;

    zone_mgr::tracker   m_ZoneTracker;
}; 
