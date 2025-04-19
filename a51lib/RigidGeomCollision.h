#pragma once

#include "Guid.h"
#include "objects/Object.h"

class RigidGeom;

void RigidGeom_ApplyCollision( guid              Guid,
                               const BBox&       BBox,
                               const uint64_t         MeshMask,      
                               const Matrix4*    pL2W, 
                               const RigidGeom* pRigidGeom );

bool RigidGeom_GetColDetails( const RigidGeom*           pRigidGeom,
                               const Matrix4*        pL2W,
                               const void*           pColor,
                               int                   Key,
                               Object::detail_tri&   Tri );

bool RigidGeom_GetTriangle( const RigidGeom*          pRigidGeom,
                             int                   Key,
                             Vector3&              P0,
                             Vector3&              P1,
                             Vector3&              P2);

int RigidGeom_GetBoneIndexFromPrimKey( const RigidGeom&    RigidGeom, 
                                       int                  PrimKey );

void RigidGeom_GatherLoTris( const uint64_t         MeshMask,
                             const Matrix4*    pL2W, 
                             const RigidGeom& RigidGeom,
                             const BBox&       GatherBBox,
                             Vector3*&         pTriVert,
                             int&              nTris);


void RigidGeom_GatherToPolyCache(       guid       Guid,
                               const BBox&       BBox,
                               const uint64_t         MeshMask,      
                               const Matrix4*    pL2W, 
                               const RigidGeom* pRigidGeom );
