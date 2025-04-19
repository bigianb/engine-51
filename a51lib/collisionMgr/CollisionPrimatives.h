#pragma once

#include "../VectorMath.h"

bool ComputeSphereTriCollision(const Vector3* Tri,
                               const Vector3& Start,
                               const Vector3& End,
                               float          Radius,
                               float&         FinalT,
                               Vector3&       FinalHitPoint);

bool ComputeRayTriCollision(const Vector3* Tri,
                            const Vector3& Start,
                            const Vector3& End,
                            float&         FinalT,
                            Vector3&       FinalHitPoint);

bool ComputeRaySphereCollision(const Vector3& SpherePos,
                               const float    SphereRadius,
                               const Vector3& Start,
                               const Vector3& End,
                               float&         FinalT,
                               Vector3&       FinalHitPoint);

bool ComputeSphereSphereCollision(const Vector3& TestSpherePos,
                                  const float    TestSphereRadius,
                                  const float    MovingSphereRadius,
                                  const Vector3& Start,
                                  const Vector3& End,
                                  float&         FinalT,
                                  Vector3&       FinalHitPoint);

bool ComputeSphereAABBoxCollision(const BBox&    AABBox,
                                  const Vector3& Start,
                                  const Vector3& End,
                                  float          Radius,
                                  float&         FinalT,
                                  Vector3&       FinalHitPoint,
                                  plane&         FinalHitPlane,
                                  plane&         FinalSlipPlane);

bool ComputeRayAABBoxCollision(const BBox&    AABBox,
                               const Vector3& Start,
                               const Vector3& End,
                               float&         FinalT,
                               Vector3&       FinalHitPoint,
                               plane&         FinalHitPlane,
                               plane&         FinalSlipPlane);
