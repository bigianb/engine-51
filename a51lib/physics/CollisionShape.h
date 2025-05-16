#pragma once

#include "../VectorMath.h"
#include <vector>
#include <cassert>

class shape;
class rigid_body;
struct collision_info;


class collision_shape
{
public:

    // Types
    enum type
    {
        TYPE_NULL = -1,     // Not defined
        
        TYPE_SPHERE,        // Sphere  (Pos, Radius)
        TYPE_BOX,           // 3D Box  (Width, Height, Length)
        TYPE_CAPSULE,       // Capsule (StartPos, EndPos, Radius)
        TYPE_WORLD,         // World (dummy)
        
        TYPE_COUNT          // Total 
    };

//==============================================================================
// Structures
//==============================================================================
public:
    // Internal swept sphere
    struct sphere
    {
        // Constant properties
        Vector3 m_Offset;           // Local offset 
        
        // Computed members
        Vector3 m_CollFreePos; // Last world collision free pos
        Vector3 m_PrevPos;     // World start position
        Vector3 m_CurrPos;     // World end position
            
    };

//==============================================================================
// Functions
//==============================================================================
public:
            // Constructor/destructor
             collision_shape();
            ~collision_shape();

            // Type functions
            type            GetType             ( void ) const ;
            void            SetType             ( type Type ) ;

            // Owner functions
            void            SetOwner            ( rigid_body* pOwner ) ;
            rigid_body*     GetOwner            ( void ) const ;

            // Dimension functions
            void            SetRadius           ( float Radius ) ;
            float             GetRadius           ( void ) ;

            // Query functions
            BBox            ComputeLocalBBox    ( void ) const ;
            BBox            ComputeWorldBBox    ( void ) const ;

            // Collision functions
            void            SetL2W              ( const Matrix4& L2W ) ;
            void            SetL2W              ( const Matrix4& PrevL2W, const Matrix4& NextL2W ) ;
            
            // Sphere functions
            void            SetSphereCapacity   ( int Count ) ;
            void            AddSphere           ( const Vector3& Offset ) ;
            int             GetNSpheres         ( void ) const ;
            sphere&         GetSphere           ( int Index ) ;
      const sphere&         GetSphere           ( int Index ) const ;
          
            // Capsule functions  
      const Vector3&        GetCapsulePrevStartPos  ( void ) const ;
      const Vector3&        GetCapsulePrevEndPos    ( void ) const ;
      const Vector3&        GetCapsuleCurrStartPos  ( void ) const ;
      const Vector3&        GetCapsuleCurrEndPos    ( void ) const ;
            
//==============================================================================
// Data
//==============================================================================
protected:
        type            m_Type;         // Type of collision
        rigid_body*     m_pOwner;       // Owner of collision_shape (or NULL)
        float             m_Radius;       // Radius of spheres and/or capsule
        std::vector<sphere>  m_Spheres;      // List of spheres
            
//==============================================================================
// Friends
//==============================================================================
friend class rigid_body;
friend class physics_mgr;
friend class physics_inst;
friend class collider;

};

inline
collision_shape::collision_shape() :
    m_Type      ( collision_shape::TYPE_NULL ),
    m_pOwner    ( NULL ),
    m_Radius    ( 0.0f )
{
}

//==============================================================================

inline
collision_shape::~collision_shape()
{
}


//==============================================================================
// Type functions
//==============================================================================

inline
collision_shape::type collision_shape::GetType(  ) const
{
    return m_Type;
}

//==============================================================================

inline
void collision_shape::SetType( collision_shape::type Type )
{
    m_Type = Type;
}

//==============================================================================
// Owner functions
//==============================================================================

inline
void collision_shape::SetOwner( rigid_body* pOwner )
{
    m_pOwner = pOwner;
}

//==============================================================================

inline
rigid_body* collision_shape::GetOwner( void ) const
{
    return m_pOwner;
}

//==============================================================================
// Dimension functions
//==============================================================================

inline
void collision_shape::SetRadius ( float Radius )
{
    m_Radius = Radius;
}

//==============================================================================

inline
float collision_shape::GetRadius ( void )
{
    return m_Radius;
}

//==============================================================================
// Sphere functions
//==============================================================================

inline
void collision_shape::SetSphereCapacity( int Count )
{
    m_Spheres.reserve( Count );
}

//==============================================================================

inline 
int collision_shape::GetNSpheres(  ) const
{
    return m_Spheres.size();
}

//==============================================================================

inline
collision_shape::sphere& collision_shape::GetSphere( int Index )
{
    return m_Spheres[ Index ];
}

//==============================================================================

inline
const collision_shape::sphere& collision_shape::GetSphere( int Index ) const
{
    return m_Spheres[ Index ];
}

//==============================================================================
// Capsule functions  
//==============================================================================

inline
const Vector3& collision_shape::GetCapsulePrevStartPos( void ) const
{
    assert( m_Type == TYPE_CAPSULE );
    assert( m_Spheres.size() > 1 );

    return m_Spheres[0].m_PrevPos;
}

//==============================================================================

inline
const Vector3& collision_shape::GetCapsulePrevEndPos( void ) const
{
    assert( m_Type == TYPE_CAPSULE );
    assert( m_Spheres.size() > 1 );

    return m_Spheres[ m_Spheres.size() - 1 ].m_PrevPos;
}

//==============================================================================

inline
const Vector3& collision_shape::GetCapsuleCurrStartPos( void ) const
{
    assert( m_Type == TYPE_CAPSULE );
    assert( m_Spheres.size() > 1 );

    return m_Spheres[0].m_CurrPos;
}

//==============================================================================

inline
const Vector3& collision_shape::GetCapsuleCurrEndPos( void ) const
{
    assert( m_Type == TYPE_CAPSULE );
    assert( m_Spheres.size() > 1 );

    return m_Spheres[ m_Spheres.size() - 1 ].m_CurrPos;
}
