#include "Quaternion.h"

Quaternion BlendSlow( const Quaternion& Q0, 
                  const Quaternion& Q1, float T )
{
    Quaternion Q;
    float Cs,Sn;
    float Angle,InvSn,TAngle;
    float C0,C1;
    float x0,y0,z0,w0;

    // Determine if quats are further than 90 degrees
    Cs = Q0.x*Q1.x + Q0.y*Q1.y + Q0.z*Q1.z + Q0.w*Q1.w;

    // If dot is negative flip one of the quaterions
    if( Cs < 0.0f )
    {
       x0 = -Q0.x;
       y0 = -Q0.y;
       z0 = -Q0.z;
       w0 = -Q0.w;
       Cs = -Cs;
    }
    else
    {
       x0 = +Q0.x;
       y0 = +Q0.y;
       z0 = +Q0.z;
       w0 = +Q0.w;
    }

    // Compute sine of angle between Q0,Q1
    Sn = 1.0f - Cs*Cs;
    if( Sn < 0.0f ) Sn = -Sn;
    Sn = sqrt( Sn );

    // Check if quaternions are very close together
    if( (Sn < 1e-3) && (Sn > -1e-3) )
    {
        return Q0;
    }

    Angle   = atan2( Sn, Cs );
    InvSn   = 1.0f/Sn;    
    TAngle  = T*Angle;

    C0      = sin( Angle - TAngle) * InvSn;
    C1      = sin( TAngle ) * InvSn;    

    Q.x     = C0*x0 + C1*Q1.x;
    Q.y     = C0*y0 + C1*Q1.y;
    Q.z     = C0*z0 + C1*Q1.z;
    Q.w     = C0*w0 + C1*Q1.w;

    return Q;
}

//==============================================================================

Quaternion BlendToIdentitySlow( const Quaternion& Q0, float T )
{
    Quaternion Q;
    float Cs,Sn;
    float Angle,InvSn,TAngle;
    float C0,C1;
    float x0,y0,z0,w0;

    // Determine if quats are further than 90 degrees
    Cs = Q0.w;

    // If dot is negative flip one of the quaterions
    if( Cs < 0.0f )
    {
       x0 = -Q0.x;
       y0 = -Q0.y;
       z0 = -Q0.z;
       w0 = -Q0.w;
       Cs = -Cs;
    }
    else
    {
       x0 = +Q0.x;
       y0 = +Q0.y;
       z0 = +Q0.z;
       w0 = +Q0.w;
    }

    // Compute sine of angle between Q0,Q1
    Sn = 1.0f - Cs*Cs;
    if( Sn < 0.0f ) Sn = -Sn;
    Sn = sqrt( Sn );

    // Check if quaternions are very close together
    if( (Sn < 1e-3) && (Sn > -1e-3) )
    {
        return Q0;
    }

    Angle   = atan2( Sn, Cs );
    InvSn   = 1.0f/Sn;    
    TAngle  = T*Angle;

    C0      = sin( Angle - TAngle) * InvSn;
    C1      = sin( TAngle ) * InvSn;    

    Q.x     = C0*x0;
    Q.y     = C0*y0;
    Q.z     = C0*z0;
    Q.w     = C0*w0 + C1;

    return Q;
}
