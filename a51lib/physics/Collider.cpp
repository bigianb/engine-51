
#include "Collider.h"
#include "RigidBody.h"
#include "PhysicsMgr.h"
#include "CollisionShape.h"

// Pointer to function for collider class collision function
typedef void ( collider::* collider_coll_func )( collision_shape* pColl0, collision_shape* pColl1 );

// Collider collision function info (used to define collision function lookup table)
struct coll_func_info
{
    collision_shape::type   m_Type0;        // Type of collision shape0
    collision_shape::type   m_Type1;        // Type of collision shape1
    collider_coll_func      m_Function;     // Collider collision function to call
};

//==============================================================================
// DATA
//==============================================================================
collider g_Collider;


// Collision lookup table for "( collision_shape::type, collision_shape::type )"
//      0: TYPE_SPHERE
//      1: TYPE_BOX
//      2: TYPE_CAPSULE
//      3: TYPE_WORLD
static coll_func_info s_CollFuncInfo[collision_shape::TYPE_COUNT][collision_shape::TYPE_COUNT] =
{
    // 0: TYPE_SPHERE versus ???
    {    
        {   collision_shape::TYPE_SPHERE,   collision_shape::TYPE_SPHERE,   &collider::CheckSpheresSpheres },   // TYPE_SPHERE v TYPE_SHERE
        {   collision_shape::TYPE_SPHERE,   collision_shape::TYPE_BOX,      &collider::CheckSpheresSpheres },   // TYPE_SPHERE v TYPE_BOX
        {   collision_shape::TYPE_SPHERE,   collision_shape::TYPE_CAPSULE,  &collider::CheckSpheresCapsule },   // TYPE_SPHERE v TYPE_CAPSULE
        {   collision_shape::TYPE_SPHERE,   collision_shape::TYPE_WORLD,    &collider::CheckSpheresWorld   },   // TYPE_SPHERE v TYPE_WORLD
    },

    // 1: TYPE_BOX versus ???
    {    
        {   collision_shape::TYPE_BOX,      collision_shape::TYPE_SPHERE,   NULL                            },  // TYPE_BOX v TYPE_SPHERE
        {   collision_shape::TYPE_BOX,      collision_shape::TYPE_BOX,      &collider::CheckSpheresSpheres  },  // TYPE_BOX v TYPE_BOX
        {   collision_shape::TYPE_BOX,      collision_shape::TYPE_CAPSULE,  &collider::CheckSpheresCapsule  },  // TYPE_BOX v TYPE_CAPSULE
        {   collision_shape::TYPE_BOX,      collision_shape::TYPE_WORLD,    &collider::CheckSpheresWorld    },  // TYPE_BOX v TYPE_WORLD
    },

    // 2: TYPE_CAPSULE versus ???
    {    
        {   collision_shape::TYPE_CAPSULE,  collision_shape::TYPE_SPHERE,   NULL                            },  // TYPE_CAPSULE v TYPE_SPHERE
        {   collision_shape::TYPE_CAPSULE,  collision_shape::TYPE_BOX,      NULL                            },  // TYPE_CAPSULE v TYPE_BOX
        {   collision_shape::TYPE_CAPSULE,  collision_shape::TYPE_CAPSULE,  &collider::CheckCapsuleCapsule  },  // TYPE_CAPSULE v TYPE_CAPSULE
        {   collision_shape::TYPE_CAPSULE,  collision_shape::TYPE_WORLD,    &collider::CheckSpheresWorld    },  // TYPE_CAPSULE v TYPE_WORLD
    },

        // 3: TYPE_WORLD versus ???
    {    
        {   collision_shape::TYPE_WORLD,    collision_shape::TYPE_SPHERE,   NULL                            },  // TYPE_WORLD v TYPE_SPHERE
        {   collision_shape::TYPE_WORLD,    collision_shape::TYPE_BOX,      NULL                            },  // TYPE_WORLD v TYPE_BOX
        {   collision_shape::TYPE_WORLD,    collision_shape::TYPE_CAPSULE,  NULL                            },  // TYPE_WORLD v TYPE_CAPSULE
        {   collision_shape::TYPE_WORLD,    collision_shape::TYPE_WORLD,    NULL                            },  // TYPE_WORLD v TYPE_WORLD
    },
};


//==============================================================================
// CONSTRUCTOR/DESTRUCTOR
//==============================================================================

collider::collider()
{
    m_pMovingColl    = nullptr;
    m_nMaxCollisions = 4;
    m_nClusters      = 0;
    m_ClusterListBBox.Clear();
}

//==============================================================================

collider::~collider()
{
}

//==============================================================================
// FUNCTIONS
//==============================================================================

// Clears internal collisions
void collider::ClearCollisions( void )
{
    // Clear collisions
    for( int i = 0; i < m_nMaxCollisions; i++ )
        m_Collisions[i].m_Depth = -FLT_MAX;
}

//==============================================================================

// Reports collisions to physics manager
int collider::ReportCollisions( collision_shape* pColl0, collision_shape* pColl1 )
{
    // Count the # of collisions to report and compute average collision
    int nCollisions = 0;
    int i;
    Vector3 Position( 0.0f, 0.0f, 0.0f );
    Vector3 Normal  ( 0.0f, 0.0f, 0.0f );
    float     Depth = 0.0f;
    for( i = 0; i < m_nMaxCollisions; i++)
    {
        // Collision setup
        collision& Collision = m_Collisions[i];
        if( Collision.m_Depth > -FLT_MAX )
        {
            // Update stats
            nCollisions++;

            // Accumulate
            Position += Collision.m_Point;
            Normal   += Collision.m_Normal;
            Depth    += Collision.m_Depth;
        }
    }
    // Nothing to do?
    if( !nCollisions )
        return 0;

    // Lookup rigid bodies
    rigid_body* pBody0 = pColl0->GetOwner();
    rigid_body* pBody1 = pColl1->GetOwner();

    // Report average?
    if( nCollisions > 1 )
    {
        // Compute average
        float Scale = 1.0f / (float)nCollisions;
        Position *= Scale;
        Normal   *= Scale;
        Depth    *= Scale;
    
        // Report the average collision
        g_PhysicsMgr.AddCollision( pBody0, 
                                   pBody1, 
                                   Position,
                                   Normal,
                                   Depth );
    }
    
    // Report other collisions
    for( i = 0; i < m_nMaxCollisions; i++)
    {
        // Collision setup
        collision& Collision = m_Collisions[i];
        if( Collision.m_Depth > -FLT_MAX )
        {
            // Add world space collision
            g_PhysicsMgr.AddCollision( pBody0, 
                                       pBody1, 
                                       Collision.m_Point,
                                       Collision.m_Normal,
                                       Collision.m_Depth );
        }
    }
    
    return nCollisions;
}

//==============================================================================

// Adds to collision list if it's one of the top deepest so far
void collider::RecordCollision( const collision& Collision )
{
    assert( Collision.m_Depth != FLT_MAX );
    assert( Collision.m_Depth != -FLT_MAX );

    // Find the least deepest current collision
    int I = 0;
    float D = m_Collisions[0].m_Depth; // Counter act expand!
    for( int i = 1; i < m_nMaxCollisions; i++ )
    {
        if( m_Collisions[i].m_Depth < D )
        {
            I = i;
            D = m_Collisions[i].m_Depth;
        }
    }

    // Record?
    if( Collision.m_Depth > D )
        m_Collisions[I] = Collision;
}

//==============================================================================

template< class T > inline void x_swap( T& a, T& b )  { T c = a; a = b; b = c; }
 
//==============================================================================

// Checks collision between 2 moving shapes
int collider::Check( collision_shape* pColl0, collision_shape* pColl1 )
{
    // Make sure they are valid
    assert( pColl0 );
    assert( pColl1 );
    assert( pColl0 != pColl1 );
    
    // Lookup owner rigid bodies
    rigid_body* pBody0 = pColl0->GetOwner();
    rigid_body* pBody1 = pColl1->GetOwner();
    assert( pBody0 );
    assert( pBody1 );
    assert( pBody0 != pBody1 );
    
    // Clear collision list
    ClearCollisions();

    // Make sure types are setup correctly
    assert( pColl0->GetType() >= 0 );
    assert( pColl1->GetType() >= 0 );
    assert( pColl0->GetType() < collision_shape::TYPE_COUNT );
    assert( pColl1->GetType() < collision_shape::TYPE_COUNT );

    // Make the higher shape be the moving one for better stacking
    if(     ( pBody1 == &g_ActorBody ) 
        ||  ( pBody1 == &g_WorldBody ) 
        ||  ( pBody0->GetPosition().y > pBody1->GetPosition().y ) )
    {            
        m_pMovingColl = pColl0;
    }
    else
    {            
        m_pMovingColl = pColl1;
    }
    
    // Fixup order ready for calling collision functions
    if( pColl1->GetType() < pColl0->GetType() )
        x_swap( pColl0, pColl1 );
    assert( pColl0->GetType() <= pColl1->GetType() );
        
    // Lookup collision function
    coll_func_info& CollFuncInfo = s_CollFuncInfo[ pColl0->GetType() ][ pColl1->GetType() ];
    
    // Validate table and collision shape types are setup correctly
    assert( CollFuncInfo.m_Type0 == pColl0->GetType() );
    assert( CollFuncInfo.m_Type1 == pColl1->GetType() );
    assert( CollFuncInfo.m_Function );
    
    // Check collision
    (this->*CollFuncInfo.m_Function)( pColl0, pColl1 );
        
    // Clear moving shape
    m_pMovingColl = NULL;
        
    // Report any collisions to physics manager
    return ReportCollisions( pColl0, pColl1 );
}

//==============================================================================
// Private functions
//==============================================================================

// Checks static sphere V sphere collision
void collider::SphereSphereCollision( const Vector3&     SpherePos0,
                                      const float          SphereRadius0,
                                      const Vector3&     SpherePos1,
                                      const float          SphereRadius1 )
{   
    // Compute distance to be apart
    float CheckDistSqr = x_sqr( SphereRadius0 + SphereRadius1 );
    
    // Intersecting?
    Vector3 Dir     = SpherePos0 - SpherePos1;
    float     DistSqr = Dir.LengthSquared();
    if( DistSqr < CheckDistSqr )
    {
        // Compute intersection depth
        float Depth = CheckDistSqr - DistSqr;
        if( Depth > 0.001f )
        {
            // Build collision
            collision Collision;
            Collision.m_Depth  = sqrt( Depth );
            Collision.m_Normal = Dir;
            Collision.m_Normal.Normalize();
            
            // Use average of collision pt on both spheres for final collision pt
            Collision.m_Point =  0.5f * (   ( SpherePos0 - ( Collision.m_Normal * SphereRadius0 ) )
                                          + ( SpherePos1 + ( Collision.m_Normal * SphereRadius1 ) ) );

            // Record
            RecordCollision( Collision );
        }
    }
}

//==============================================================================

// Checks sphere V sphere collision
void collider::SphereCapsuleCollision( const Vector3&    SpherePos,
                                       const float         SphereRadius,
                                       const Vector3&    CapsuleStartPos,
                                       const Vector3&    CapsuleEndPos,
                                       const float         CapsuleRadius )
{
    // Get delta and distance from sphere to closest pt on capsule
    Vector3 Delta   = SpherePos.GetClosestVToLSeg( CapsuleStartPos, CapsuleEndPos  );
    float     DistSqr = Delta.LengthSquared();
    
    // Overlapping?
    float CheckDistSqr = x_sqr( SphereRadius + CapsuleRadius );
    if( DistSqr < CheckDistSqr )
    {
        // Compute intersection depth
        float Depth = CheckDistSqr - DistSqr;
        if( Depth > 0.001f )
        {
            // Build collision
            collision Collision;
            Collision.m_Depth  = sqrt( Depth );
            Collision.m_Normal = -Delta;
            Collision.m_Normal.Normalize();
            Collision.m_Point = SpherePos - ( Collision.m_Normal * SphereRadius );
            
            // Record
            RecordCollision( Collision );
        }
    }
}

// Checks static capsule V capsule collision
void collider::CapsuleCapsuleCollision( const Vector3&    CapsuleStartPos0,
                                        const Vector3&    CapsuleEndPos0,
                                        const float         CapsuleRadius0,
                                        const Vector3&    CapsuleStartPos1,
                                        const Vector3&    CapsuleEndPos1,
                                        const float         CapsuleRadius1 )
{
    // Compute radius checking distance
    float CheckDistSqr = x_sqr( CapsuleRadius0 + CapsuleRadius1 );
    
    // Compute closest points on lines
    Vector3 P0,P1;    
    x_ClosestPtsOnLineSegs( CapsuleStartPos0, CapsuleEndPos0,
                            CapsuleStartPos1, CapsuleEndPos1,
                            P0, P1 );
                            
    // Get distance between them
    Vector3 Delta   = P1 - P0;
    float     DistSqr = Delta.LengthSquared();

    // Overlapping?    
    if( DistSqr < CheckDistSqr )
    {
        // Compute intersection depth
        float Depth = CheckDistSqr - DistSqr;
        if( Depth > 0.001f )
        {
            // Build collision
            collision Collision;
            Collision.m_Depth  = sqrt( Depth );
            Collision.m_Normal = -Delta;
            Collision.m_Normal.Normalize();
            
            // Use average of collision pt on both spheres for final collision pt
            Collision.m_Point =  0.5f * (     ( P0 - ( Collision.m_Normal * CapsuleRadius0 ) )
                                            + ( P1 + ( Collision.m_Normal * CapsuleRadius1 ) ) );

            // Record
            RecordCollision( Collision );
        }
    }
}

//==============================================================================

int collider::CollectWorldPolys( const BBox& WorldBBox )
{
    // Skip if not using polycache, but return 1 so collision is called
    if( !g_PhysicsMgr.m_Settings.m_bUsePolycache )
        return 1;

    // Gather factored out list of clusters in dynamic area
    m_ClusterListBBox = WorldBBox;
    g_PolyCache.BuildClusterList( m_ClusterListBBox, 
                                  Object::TYPE_ALL_TYPES, 
                                  Object::ATTR_BLOCKS_RAGDOLL, 
                                  Object::ATTR_COLLISION_PERMEABLE | Object::ATTR_LIVING );

    // Copy clusters into local array
    m_nClusters = g_PolyCache.m_nClusters;
    assert( m_nClusters < MAX_CLUSTERS );
    if( m_nClusters > MAX_CLUSTERS )
        m_nClusters = MAX_CLUSTERS;
    memcpy( m_pClusters, g_PolyCache.m_ClusterList, sizeof(poly_cache::cluster*) * m_nClusters );
    
    // Return cluster count
    return m_nClusters;
}

//==============================================================================

// Returns true if sphere intersects NGon
bool collider::SphereNGonIntersection( const plane&   Plane,
                                        const Vector3* Verts, 
                                        const int      nVerts, 
                                        const Vector3& SpherePos, 
                                        const float      SphereRadius )
{
    int i;
    // Does sphere intersect the infinite plane?
    float Dist = Plane.Distance( SpherePos );
    if( abs( Dist ) > SphereRadius )
        return false;

    // Check to see if sphere is within all edges
    for( i = 0; i < nVerts; i++ )
    {
        // Lookup edge end pts
        const Vector3& EdgeStart = Verts[( i == 0 ) ? nVerts-1 : i-1 ];
        const Vector3& EdgeEnd   = Verts[ i ];

        // Exit loop if point is outside of edge
        Vector3 EdgeDir        = EdgeEnd - EdgeStart;
        Vector3 EdgeNormal     = EdgeDir.Cross( Plane.Normal );
        Vector3 EdgePointDelta = EdgeStart - SpherePos;
        if( EdgeNormal.Dot( EdgePointDelta ) < 0 )
            break;
    }

    // If point is in NGon then sphere intersects
    if( i == nVerts )
    {
        return true;
    }

    // Intersection pt is not in NGon, so check for sphere v edge intersections
    float SphereRadiusSqr = SphereRadius * SphereRadius;
    for( i = 0; i < nVerts; i++ )
    {
        // Lookup edge end pts
        const Vector3& EdgeStart = Verts[( i == 0 ) ? nVerts-1 : i-1 ];
        const Vector3& EdgeEnd   = Verts[ i ];

        // Does edge intersection sphere?
        float DistSqr = SpherePos.GetSqrtDistToLineSeg( EdgeStart, EdgeEnd );
        if( DistSqr < SphereRadiusSqr )
            return true;
    }

    // No intersection
    return false;
}

//==============================================================================

// Returns true if sphere intersects any of world
bool collider::SphereWorldIntersection( const Vector3& SpherePos, const float SphereRadius )
{
    // Use ground plane?
    if( !g_PhysicsMgr.m_Settings.m_bUsePolycache )
    {
        // Setup ground plane
        plane Plane;
        Plane.Setup( Vector3( 0.0f, 1.0f, 0.0f ), 0.0f );

        // Call NGon function
        return SphereNGonIntersection( Plane, NULL, 0, SpherePos, SphereRadius );
    }
    
    // Should only call if clusters are present
    assert( m_nClusters );

    // Create bounding box for sphere
    BBox SphereBBox( SpherePos, SphereRadius );

    // Keeping these out the loop stops debug crashes?!
    plane   Plane;
    Vector3 Verts[4];

    // Loop through the clusters and process the triangles
    for( int iCL=0; iCL < m_nClusters; iCL++ )
    {
        poly_cache::cluster& CL = *m_pClusters[iCL];

        // Skip over this cluster if sphere bbox doesn't intersect cluster bbox
        if ( !SphereBBox.Intersect(CL.m_BBox) )
            continue;

        int iQ = -1;
        while( 1 )
        {
            // Do tight loop on bbox checks and cull flags
            {
                iQ++;
                while( iQ < (int)CL.nQuads )
                {
                    // Do bbox culling
                    BBox* pBBox = (BBox*)(&CL.pBounds[iQ]);
                    if( SphereBBox.Intersect( *pBBox ) ) 
                    {
                        break;
                    }
                    iQ++;
                }
                if( iQ==(int)CL.nQuads )
                    break;
            }

            // Get access to this quad
            poly_cache::cluster::quad& QD = CL.pQuad[iQ];

            // Setup plane
            Plane.Normal = CL.pNormal[ QD.iN ];
            Plane.D      = CL.pBounds[ iQ ].PlaneD;

            // Check intersection
            if( CL.pBounds[iQ].Flags & BOUNDS_IS_QUAD )
            {
                // Get verts
                Verts[0] = CL.pPoint[ QD.iP[0] ];
                Verts[1] = CL.pPoint[ QD.iP[1] ];
                Verts[2] = CL.pPoint[ QD.iP[2] ];
                Verts[3] = CL.pPoint[ QD.iP[3] ];
            
                // Check for quad intersection
                if( SphereNGonIntersection( Plane, Verts, 4, SpherePos, SphereRadius ) )
                    return true;
            }
            else
            {
                // Get verts
                Verts[0] = CL.pPoint[ QD.iP[0] ];
                Verts[1] = CL.pPoint[ QD.iP[1] ];
                Verts[2] = CL.pPoint[ QD.iP[2] ];
            
                // Check for tri intersection
                if( SphereNGonIntersection( Plane, Verts, 3, SpherePos, SphereRadius ) )
                    return true;
            }                        
        }            
    }
    
    // No intersection
    return false;
}

//==============================================================================

// Check sweeping sphere V NGon collision
bool collider::SphereNGonCollision( const plane&   Plane,
                                     const Vector3* Verts,
                                     const int      nVerts,
                                     const Vector3& SphereStartPos,
                                     const Vector3& SphereEndPos,
                                     const float      SphereRadius,
                                           float&     CollT,
                                           plane&   CollPlane )
{
    int i;

    // Moving away from plane?
    Vector3 Dir = SphereEndPos - SphereStartPos;
    float DirDotNormal = Plane.Normal.Dot( Dir );
    if( DirDotNormal > 0.0f )
        return false;
     
    // Check if starting sphere is behind plane
    float StartDist = Plane.Distance( SphereStartPos );
    if( StartDist < -SphereRadius )
        return false;

    // Check if ending sphere is in front of plane
    float EndDist = Plane.Distance( SphereEndPos );
    if( EndDist > SphereRadius )
        return false;

    // Compute intersection ratio
    Vector3 SphereBot = SphereStartPos - ( Plane.Normal * SphereRadius );
    float     T         = -Plane.Distance( SphereBot ) / DirDotNormal;

    // No collision?
    if( ( T < 0.0f ) || ( T >= CollT ) )
        return false;

    // Compute point on plane at intersection
    Vector3 Point = SphereBot + ( T * Dir );

    // Check to see if point is within all edges
    for( i = 0; i < nVerts; i++ )
    {
        // Lookup edge end pts
        const Vector3& EdgeStart = Verts[( i == 0 ) ? nVerts-1 : i-1 ];
        const Vector3& EdgeEnd   = Verts[ i ];
    
        // Exit loop if point is outside of edge
        Vector3 EdgeDir        = EdgeEnd - EdgeStart;
        Vector3 EdgeNormal     = EdgeDir.Cross( Plane.Normal );
        Vector3 EdgePointDelta = EdgeStart - Point;
        if( EdgeNormal.Dot( EdgePointDelta ) < 0 )
            break;
    }

    // If point is in NGon then collision is valid
    if( i == nVerts )
    {
        CollT     = T;
        CollPlane = Plane;
        return true;
    }
            
    // Intersection pt is not in NGon, so check for sphere v edge collisions
    float   SphereRadiusSqr = SphereRadius * SphereRadius;
    bool bCollision = false;
    for( i = 0; i < nVerts; i++ )
    {
        // Lookup edge end pts
        const Vector3& EdgeStart = Verts[( i == 0 ) ? nVerts-1 : i-1 ];
        const Vector3& EdgeEnd   = Verts[ i ];
    
        // Get closest pts between sphere movement and edge
        Vector3 SpherePoint, EdgePoint;
        float     SphereT, EdgeT;
        x_ClosestPtsOnLineSegs( SphereStartPos, SphereEndPos, 
                                EdgeStart,   EdgeEnd, 
                                SpherePoint, EdgePoint,
                                SphereT,     EdgeT );
    
        // Skip if not closest edge so far
        if( SphereT > CollT ) 
            continue;
            
        // Intersection - is distance between closest pts on line less than sphere radius?
        Vector3 Delta   = SpherePoint - EdgePoint;
        float     DistSqr = Delta.LengthSquared();
        if( DistSqr < SphereRadiusSqr )
        {
            // Really the edge -> sphere collision normal should be computed by casting
            // a ray in the opposite direction of the sphere movement towards the sphere
            // start pos, but we don't need that much accuracy.
        
            // Almost on edge?
            if( DistSqr < 0.00001f )
            {
                // Collide with plane
                bCollision = true;
                CollT      = SphereT;
                CollPlane  = Plane;
            }
            else
            {
                // Setup plane from edge point and direction to sphere
                bCollision = true;
                CollT      = SphereT;
                CollPlane.Setup( EdgePoint, Delta );
            }
        }
    }
    
    return bCollision;
}

//==============================================================================

// Checks sweeping sphere V world collision        
bool collider::SphereWorldCollision( const Vector3& SphereStartPos,
                                      const Vector3& SphereEndPos,
                                      const float      SphereRadius,
                                            float&     CollT,
                                            plane&   CollPlane )
{
    // Use ground plane?
    if( !g_PhysicsMgr.m_Settings.m_bUsePolycache )
    {
        // Setup ground plane
        plane Plane;
        Plane.Setup( Vector3( 0.0f, 1.0f, 0.0f ), 0.0f );

        // Collide with ground plane
        return SphereNGonCollision( Plane, NULL, 0, SphereStartPos, SphereEndPos, SphereRadius, CollT, CollPlane );
    }

    // Should only call if clusters are present
    assert( m_nClusters );

    // Compute dynamic bbox
    BBox DynamicBBox( SphereStartPos, SphereEndPos );
    DynamicBBox.Inflate( SphereRadius + 1.0f, SphereRadius + 1.0f, SphereRadius + 1.0f );
    
    // Keeping these out the loop stops debug crashes?!
    plane   Plane;
    Vector3 Verts[4];

    // Build culling flags
    uint32_t     CullFlags = 0;
    Vector3 Dir       = SphereEndPos - SphereStartPos;
    Dir.Normalize();
    if( Dir.x > +0.001f ) CullFlags |= BOUNDS_X_POS;
    if( Dir.x < -0.001f ) CullFlags |= BOUNDS_X_NEG;
    if( Dir.y > +0.001f ) CullFlags |= BOUNDS_Y_POS;
    if( Dir.y < -0.001f ) CullFlags |= BOUNDS_Y_NEG;
    if( Dir.z > +0.001f ) CullFlags |= BOUNDS_Z_POS;
    if( Dir.z < -0.001f ) CullFlags |= BOUNDS_Z_NEG;
    
    // Loop through the clusters and process the triangles
    bool bCollision = false;
    for( int iCL=0; iCL < m_nClusters; iCL++ )
    {
        poly_cache::cluster& CL = *m_pClusters[iCL];

        // Skip over this cluster if dynamic bbox doesn't intersect
        if ( !DynamicBBox.Intersect(CL.m_BBox) )
            continue;

        int iQ = -1;
        while( 1 )
        {
            // Do tight loop on bbox checks and cull flags
            {
                iQ++;
                while( iQ < (int)CL.nQuads )
                {
                    // Do flag culling
                    if( (CL.pBounds[iQ].Flags & CullFlags) == 0 )
                    {
                        // Do bbox culling
                        BBox* pBBox = (BBox*)(&CL.pBounds[iQ]);
                        if( DynamicBBox.Intersect( *pBBox ) ) 
                        {
                            break;
                        }
                    }
                    iQ++;
                }
                if( iQ==(int)CL.nQuads )
                    break;
            }

            // Get access to this quad
            poly_cache::cluster::quad& QD = CL.pQuad[iQ];

            // Setup plane
            Plane.Normal = CL.pNormal[QD.iN];
            Plane.D      = CL.pBounds[iQ].PlaneD;

            // Check collision
            if( CL.pBounds[iQ].Flags & BOUNDS_IS_QUAD )
            {
                // Get verts
                Verts[0] = CL.pPoint[ QD.iP[0] ];
                Verts[1] = CL.pPoint[ QD.iP[1] ];
                Verts[2] = CL.pPoint[ QD.iP[2] ];
                Verts[3] = CL.pPoint[ QD.iP[3] ];
            
                // Check for quad intersection
                bCollision |= SphereNGonCollision( Plane, Verts, 4, 
                                                   SphereStartPos, SphereEndPos, SphereRadius, 
                                                   CollT, CollPlane );
            }
            else
            {
                // Get verts
                Verts[0] = CL.pPoint[ QD.iP[0] ];
                Verts[1] = CL.pPoint[ QD.iP[1] ];
                Verts[2] = CL.pPoint[ QD.iP[2] ];
            
                // Check for tri intersection
                bCollision |= SphereNGonCollision( Plane, Verts, 3, 
                                                    SphereStartPos, SphereEndPos, SphereRadius, 
                                                    CollT, CollPlane );
            }                        
        }
    }
    return bCollision;
}

//==============================================================================

// Checks collision between moving sphere list and world
void collider::CheckSpheresWorld( collision_shape* pColl0, collision_shape* pColl1 )
{
    // Collision shapes should be valid and contain data!
    assert( pColl0 );
    assert( pColl0->GetNSpheres() );
    assert( pColl1 );
    assert( pColl1 == &g_WorldColl );
    (void)pColl1;

    int     i;
    Vector3 Dir;
    float     Dist, DistSqr;          

    // If there are no cluster, then just update spheres fast and get out of here
    if( m_nClusters == 0 )
    {
        // Update all spheres
        for( i = 0; i < pColl0->GetNSpheres(); i++ )
        {
            // Lookup sphere
            collision_shape::sphere& Sphere = pColl0->GetSphere( i );

            // Update collision free pos
            Sphere.m_CollFreePos = Sphere.m_CurrPos;
        }
        
        // Nothing else to do...
        return;    
    }

    // Lookup back off dist
    float HitBackoffDist = g_PhysicsMgr.m_Settings.m_CollisionHitBackoffDist;

    // Check all spheres against world for first collision
    const float SphereRadius = pColl0->m_Radius;
    for( i = 0; i < pColl0->GetNSpheres(); i++ )
    {
        // Get sphere info
        collision_shape::sphere& Sphere = pColl0->GetSphere( i );

        // If movement is collision free, then update collision 
        // free pos to stop ragdoll getting hung up on stuff
        BBox    MoveBBox( Sphere.m_PrevPos, Sphere.m_CurrPos );
        MoveBBox.Inflate( SphereRadius, SphereRadius, SphereRadius );
        if( SphereWorldIntersection( MoveBBox.GetCenter(), MoveBBox.GetRadius() ) == false )
        {
            // No collision, so just update collision pos
            Sphere.m_CollFreePos = Sphere.m_CurrPos;
        }
        else
        {
            // Lookup movement
            const Vector3& SphereStartPos = Sphere.m_CollFreePos;
            const Vector3& SphereEndPos   = Sphere.m_CurrPos;

            // Skip if not moving very far
            Dir     = SphereEndPos - SphereStartPos;
            DistSqr = Dir.LengthSquared();
            if( DistSqr < x_sqr( 0.001f ) )
                continue;
            
            // Collision with world?
            float   CollT = FLT_MAX;
            plane CollPlane;
            if( SphereWorldCollision( SphereStartPos, SphereEndPos, SphereRadius, CollT, CollPlane ) )
            {
                // Record the collision (keep as if it happened at the end position)
                collision   Collision;
                Collision.m_Normal = CollPlane.Normal;
                Collision.m_Point  = SphereEndPos - ( Collision.m_Normal * SphereRadius );
                Collision.m_Depth  = -CollPlane.Distance( Collision.m_Point );
                RecordCollision( Collision );
                
                // Pull back from collision a bit
                Dist  = sqrt( DistSqr );
                CollT -= HitBackoffDist / Dist;
                if( CollT < 0 )
                    CollT = 0.0f;

                // Update collision free pos to be just before the collision
                Sphere.m_CollFreePos += CollT * ( SphereEndPos - SphereStartPos );
            }
            else
            {
                // No collision so update collision free pos to be end of movement
                Sphere.m_CollFreePos = Sphere.m_CurrPos;
            }
        }
    }
}

//==============================================================================

// Checks collision between 2 moving sphere lists
void collider::CheckSpheresSpheres( collision_shape* pColl0, collision_shape* pColl1 )
{
    // Validate
    assert( pColl0 );
    assert( pColl1 );
    assert( pColl0->GetNSpheres() );
    assert( pColl1->GetNSpheres() );
    assert(     ( pColl0->GetType() == collision_shape::TYPE_SPHERE  )
            ||  ( pColl0->GetType() == collision_shape::TYPE_BOX     )
            ||  ( pColl0->GetType() == collision_shape::TYPE_CAPSULE ) );
    assert(     ( pColl1->GetType() == collision_shape::TYPE_SPHERE  )
            ||  ( pColl1->GetType() == collision_shape::TYPE_BOX     )
            ||  ( pColl1->GetType() == collision_shape::TYPE_CAPSULE ) );

    // Lookup radii
    const float Radius0 = pColl0->GetRadius();
    const float Radius1 = pColl1->GetRadius();

    // Check all spheres against all other spheres    
    for( int i = 0; i < pColl0->GetNSpheres(); i++ )
    {
        // Get sphere0
        collision_shape::sphere& Sphere0 = pColl0->GetSphere( i );

        // Check against all other spheres    
        for( int j = 0; j < pColl1->GetNSpheres(); j++ )
        {
            // Get sphere1
            collision_shape::sphere& Sphere1 = pColl1->GetSphere( j );

            // Check for collision      
            if( m_pMovingColl == pColl0 )
            {              
                SphereSphereCollision( Sphere0.m_CurrPos, Radius0,
                                       Sphere1.m_PrevPos, Radius1 );
            }                                    
            else
            {              
                SphereSphereCollision( Sphere0.m_PrevPos, Radius0,
                                       Sphere1.m_CurrPos, Radius1 );
            }                                    
        }                                   
    }
}

//==============================================================================

// Checks collision between moving sphere list and a capsule
void collider::CheckSpheresCapsule( collision_shape* pColl0, collision_shape* pColl1 )
{
    // Validate
    assert( pColl0 );
    assert( pColl1 );
    assert( pColl0->GetNSpheres() );
    assert( pColl1->GetNSpheres() );
    assert(     ( pColl0->GetType() == collision_shape::TYPE_SPHERE  )
            ||  ( pColl0->GetType() == collision_shape::TYPE_BOX     )
            ||  ( pColl0->GetType() == collision_shape::TYPE_CAPSULE ) );
    assert( pColl1->GetType() == collision_shape::TYPE_CAPSULE );

    // Loop through all spheres and collide with capsule
    for( int i = 0; i < pColl0->GetNSpheres(); i++ )
    {
        // Lookup sphere
        collision_shape::sphere& Sphere0 = pColl0->GetSphere( i );
        
        // Check for collision
        if( m_pMovingColl == pColl0 )
        {        
            SphereCapsuleCollision( Sphere0.m_CurrPos,
                                    pColl0->GetRadius(),
                                    pColl1->GetCapsulePrevStartPos(),
                                    pColl1->GetCapsulePrevEndPos(),
                                    pColl1->GetRadius() );
        }
        else
        {
            SphereCapsuleCollision( Sphere0.m_PrevPos,
                                    pColl0->GetRadius(),
                                    pColl1->GetCapsuleCurrStartPos(),
                                    pColl1->GetCapsuleCurrEndPos(),
                                    pColl1->GetRadius() );
        }
    }
}

//==============================================================================

// Checks collision between 2 capsules
void collider::CheckCapsuleCapsule( collision_shape* pColl0, collision_shape* pColl1 )
{
    // Validate
    assert( pColl0 );
    assert( pColl1 );
    assert( pColl0->GetNSpheres() );
    assert( pColl1->GetNSpheres() );
    assert( pColl0->GetType() == collision_shape::TYPE_CAPSULE );
    assert( pColl1->GetType() == collision_shape::TYPE_CAPSULE );
    
    // Check for collision
    if( m_pMovingColl == pColl0 )
    {
        CapsuleCapsuleCollision( pColl0->GetCapsuleCurrStartPos(),
                                 pColl0->GetCapsuleCurrEndPos(),
                                 pColl0->GetRadius(),
                                 pColl1->GetCapsulePrevStartPos(),
                                 pColl1->GetCapsulePrevEndPos(),
                                 pColl1->GetRadius() );
    }
    else
    {
        CapsuleCapsuleCollision( pColl0->GetCapsulePrevStartPos(),
                                 pColl0->GetCapsulePrevEndPos(),
                                 pColl0->GetRadius(),
                                 pColl1->GetCapsuleCurrStartPos(),
                                 pColl1->GetCapsuleCurrEndPos(),
                                 pColl1->GetRadius() );
    }
}
