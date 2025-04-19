#pragma once

#include "../VectorMath.h"

class floor_properties
{
public:

             floor_properties    ();
            ~floor_properties    ();

            void    Init            ( float Radius, float ColorFadeTime );
            void    Update          ( const Vector3& Position, float DeltaTime, bool bIgnorePosition = false );
            void    ForceUpdate     ( const Vector3& NewPosition );

            Colour  GetColor        () { return m_CurrentColor; }
            uint32_t     GetMaterial     () { return m_FloorMat; }
private:

            bool   GrabFloorProperties( const Vector3& Position, Colour& Color, uint32_t& Mat );

private:

    Vector3 m_LastPosition;
    float     m_RadiusSquared;

    Vector3 m_StartColor;
    Vector3 m_EndColor;
    Colour  m_CurrentColor;
    float     m_ColorFadeTime;
    float     m_ColorFadeT;
    
    uint32_t     m_FloorMat;
};
