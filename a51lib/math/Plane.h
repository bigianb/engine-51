#pragma once

#include "Vector3.h"
#include "Matrix4.h"

struct plane
{

    //------------------------------------------------------------------------------
    //  Fields
    //------------------------------------------------------------------------------

    Vector3 Normal;
    float   D;

    //------------------------------------------------------------------------------
    //  Functions
    //------------------------------------------------------------------------------

    plane();
    plane(const Vector3& P1,
          const Vector3& P2,
          const Vector3& P3);
    plane(const Vector3& Normal, float Distance);
    plane(float A, float B, float C, float D);

    void Setup(const Vector3& P1,
               const Vector3& P2,
               const Vector3& P3);
    void Setup(const Vector3& Normal, float Distance);
    void Setup(const Vector3& P, const Vector3& Normal);
    void Setup(float A, float B, float C, float D);

    void Negate();
    void Transform(const Matrix4& M);

    float Dot(const Vector3& P) const;
    float Distance(const Vector3& P) const;
    bool  InFront(const Vector3& P) const;
    bool  InBack(const Vector3& P) const;
    void  ComputeD(const Vector3& P);

    float Difference(const plane& P) const;
    bool  InRange(float Min, float Max) const;
    bool  IsValid() const;

    bool Intersect(float&         t,
                   const Vector3& P0,
                   const Vector3& P1) const;

    void GetComponents(const Vector3& V,
                       Vector3&       Parallel,
                       Vector3&       Perpendicular) const;

    void GetOrthoVectors(Vector3& AxisA,
                         Vector3& AxisB) const;

    Vector3 ReflectVector(const Vector3& V) const;

    bool ClipNGon(Vector3* pDst, int& NDstVerts,
                  const Vector3* pSrc, int NSrcVerts) const;

    void GetBBoxIndices(int* pMinIndices, int* pMaxIndices) const;

    friend plane operator*(const Matrix4& M, const plane& Plane);
};

inline
void plane::Setup( float A, float B, float C, float aD )
{
    Normal.set( A, B, C );
    D          = aD;

    float Len = Normal.LengthSquared();
    if( !IN_RANGE( 0.9999f, Len, 1.0001f ) )
    {
        float Factor = 1.0 / sqrt(Len);
        Normal.Scale( Factor );
        D *= Factor;
    }
}

//==============================================================================

inline
void plane::Setup( const Vector3& P1, const Vector3& P2, const Vector3& P3 )
{
    Normal = v3_Cross( P2-P1, P3-P1 );
    Normal.SafeNormalize();
    D = -Normal.Dot( P1 );
}

//==============================================================================

inline
void plane::Setup( const Vector3& aNormal, float aDistance )
{
    Normal = aNormal;
    D      = aDistance;

    float Len = Normal.LengthSquared();
    if( !IN_RANGE( 0.9999f, Len, 1.0001f ) )
    {
        float Factor = 1.0 / sqrt(Len);
        Normal.Scale( Factor );
        D *= Factor;
    }
}

//==============================================================================

inline
void plane::Setup( const Vector3& P, const Vector3& aNormal )
{
    Normal = aNormal;

    float Len = Normal.LengthSquared();
    if( !IN_RANGE( 0.9999f, Len, 1.0001f ) )
        Normal.Scale( 1.0 / sqrt(Len) );

    D = -Normal.Dot( P );
}

//==============================================================================

inline 
plane::plane()
{
}

//==============================================================================

inline 
plane::plane( const Vector3& P1, const Vector3& P2, const Vector3& P3 )
{    
    Setup( P1, P2, P3 );
}

//==============================================================================

inline 
plane::plane( const Vector3& aNormal, float aDistance )
{
    Setup( aNormal, aDistance );
}

//==============================================================================

inline 
plane::plane( float A, float B, float C, float aD )
{
    Setup( A, B, C, aD );
}

//==============================================================================

inline
void plane::Negate()
{
    Normal.Negate();
    D      = -D;
}

//==============================================================================

inline
void plane::Transform( const Matrix4& M )
{
    (*this) = M * (*this);
}

//==============================================================================

inline
void plane::GetOrthoVectors ( Vector3& AxisA,
                              Vector3& AxisB ) const
{
    float     AbsA, AbsB, AbsC;
    Vector3 Dir;

    // Get a non-parallel axis to normal.
    AbsA = abs( Normal.GetX() );
    AbsB = abs( Normal.GetY() );
    AbsC = abs( Normal.GetZ() );
    if( (AbsA<=AbsB) && (AbsA<=AbsC) ) Dir = Vector3(1,0,0);
    else
    if( (AbsB<=AbsA) && (AbsB<=AbsC) ) Dir = Vector3(0,1,0);
    else                               Dir = Vector3(0,0,1);

    AxisA = Normal.Cross(Dir);
    AxisB = Normal.Cross(AxisA);
    AxisA.Normalize();
    AxisB.Normalize();
}

//==============================================================================

inline 
float plane::Dot( const Vector3& P ) const  
{
    return( P.Dot(Normal) );
}

//==============================================================================

inline 
float plane::Distance( const Vector3& P ) const
{
    return( P.Dot(Normal) + D );
}

//==============================================================================

inline 
bool plane::InFront( const Vector3& P ) const
{
    return( (P.Dot(Normal) + D) >= 0.0f );
}

//==============================================================================

inline 
bool plane::InBack( const Vector3& P ) const
{
    return( (P.Dot(Normal) + D) < 0.0f );
}

//==============================================================================

inline 
bool plane::Intersect( float& t, const Vector3& P0, const Vector3& P1 ) const
{
    t = (P1 - P0).Dot( Normal );

    if( t == 0.0f ) 
        return false;
    else    
    {
        t = -Distance( P0 ) / t;
        return true;
    }
}

//==============================================================================

inline
void plane::GetComponents( const Vector3& V,
                                 Vector3& Parallel,
                                 Vector3& Perpendicular ) const
{
    Perpendicular = Normal.Dot(V) * Normal;
    Parallel      = V - Perpendicular;
}

//==============================================================================

inline
Vector3 plane::ReflectVector( const Vector3& V ) const
{
    return( V - (Normal.Dot(V)*2*Normal) );
}

//==============================================================================

inline
bool plane::ClipNGon(       Vector3* pDst, int& NDstVerts, 
                       const Vector3* pSrc, int  NSrcVerts ) const
{
    float   D0, D1;
    int   P0, P1;
    bool Clipped = false;

    NDstVerts = 0;
    P1 = NSrcVerts-1;
    D1 = Distance( pSrc[P1] );

    for( int i=0; i<NSrcVerts; i++ ) 
    {
        P0 = P1;
        D0 = D1;
        P1 = i;
        D1 = Distance(pSrc[P1]);

        // Do we keep starting vert?
        if( D0 >= 0 )
        {
            pDst[NDstVerts++] = pSrc[P0];
        }

        // Do we need to compute intersection?
        if( ((D0>=0)&&(D1<0)) || ((D0<0)&&(D1>=0)) )
        {
            float d = (D1-D0);
            if( abs(d) < 0.00001f )  d = 0.00001f;
            float t = (0-D0) / d;
            pDst[NDstVerts++] = pSrc[P0] + t*(pSrc[P1]-pSrc[P0]);
            Clipped = true;
        }
    }

    return( Clipped );
}

//==============================================================================

inline
void plane::GetBBoxIndices  ( int* pMinIndices, int* pMaxIndices ) const
{
    if( pMinIndices )
    {
        if( Normal.GetX() >= 0 ) pMinIndices[0] = 0;
        else                     pMinIndices[0] = 4;
                                                                        
        if( Normal.GetY() >= 0 ) pMinIndices[1] = 1;
        else                     pMinIndices[1] = 5;
                                                                      
        if( Normal.GetZ() >= 0 ) pMinIndices[2] = 2;
        else                     pMinIndices[2] = 6;
    }

    if( pMaxIndices )
    {
        if( Normal.GetX() >= 0 ) pMaxIndices[0] = 4;
        else                     pMaxIndices[0] = 0;
                                               
        if( Normal.GetY() >= 0 ) pMaxIndices[1] = 5;
        else                     pMaxIndices[1] = 1;
                                               
        if( Normal.GetZ() >= 0 ) pMaxIndices[2] = 6;
        else                     pMaxIndices[2] = 2;
    }
}

//==============================================================================

inline
void plane::ComputeD( const Vector3& P )
{
    D = -Normal.Dot( P );
}

//==============================================================================

inline 
plane operator * ( const Matrix4& M, const plane& Plane )
{
    Matrix4     Adjoint;
    plane       NewPlane;
    Vector3     V;

    // Transform a point in the plane by M
    V = M * ( Plane.Normal * -Plane.D );

    // Compute the Transpouse of the Inverse of the Matrix
    Adjoint.Identity();
    Adjoint.SetColumns( Vector3( M(1,1) * M(2,2) - M(1,2) * M(2,1),
                                 M(1,2) * M(2,0) - M(1,0) * M(2,2),
                                 M(1,0) * M(2,1) - M(1,1) * M(2,0) ),
                        Vector3( M(2,1) * M(0,2) - M(2,2) * M(0,1),
                                 M(2,2) * M(0,0) - M(2,0) * M(0,2),
                                 M(2,0) * M(0,1) - M(2,1) * M(0,0) ),
                        Vector3( M(0,1) * M(1,2) - M(0,2) * M(1,1),
                                 M(0,2) * M(1,0) - M(0,0) * M(1,2),
                                 M(0,0) * M(1,1) - M(0,1) * M(1,0) ) );

    // Transform the normal
    NewPlane.Normal = Adjoint.RotateVector( Plane.Normal );

    // Renormalize
    NewPlane.Normal.Normalize();

    // Recompute D by in the transform point
    NewPlane.ComputeD( V );

    return NewPlane;
}

//==============================================================================

inline
float plane::Difference( const plane& P ) const
{
    Vector3 N = Normal - P.Normal;
    return( N.Dot(N) + (D-P.D)*(D-P.D) );
}

//==============================================================================

inline
bool plane::InRange( float Min, float Max ) const
{
    return( (Normal.GetX()>=-1.0f) && (Normal.GetX()<=1.0f) &&
            (Normal.GetY()>=-1.0f) && (Normal.GetY()<=1.0f) &&
            (Normal.GetZ()>=-1.0f) && (Normal.GetZ()<=1.0f) &&
            (D>=Min) && (D<=Max) );
}
