
#include "PhysicsInst.h"
#include "PhysicsMgr.h"

#include "../loco/LocoCharAnimPlayer.h"
//#include "../objects/BaseProjectile.h"

inline void ClampForce(Vector3& Force)
{
    assert(Force.IsValid());

    // Too big?
    float Max = g_PhysicsMgr.m_Settings.m_MaxForce;
    float ForceMagSqr = Force.LengthSquared();
    if (ForceMagSqr > x_sqr(Max)) {
        // Compute clamping scale
        float ForceMag = sqrt(ForceMagSqr);
        float Scale = Max / ForceMag;

        // Clamp
        Force *= Scale;
    }
}

//==============================================================================
// FUNCTIONS
//==============================================================================

physics_inst::physics_inst(ResourceManager* rm) : m_hAnimGroup(rm), m_SkinInst(rm)
{
    m_bInitialized = false;
    m_bInAwakeList = false;
    m_bInSleepingList = false;
    m_bInCollisionWakeupList = false;
    m_bPopFix = false;
    m_bActorCollision = false;
    m_bWorldCollision = true;
    m_bInstCollision = true;
    m_bActiveWhenVisible = false;
    m_WorldBBox.Clear();
    m_ConstraintWeight = 1.0f;
    m_ConstraintWeightDelta = 0.0f;
    m_KeepActiveTime = 0.0f;
    m_Zone = 0xFF;
}

//==============================================================================

physics_inst::~physics_inst()
{
    // Free
    Kill();
}

//==============================================================================
// Initialization functions
//==============================================================================

void ComputeSpacing(float Height, float Radius, int nSpheres, float& Offset, float& Spacing)
{
    // Simple case
    if (nSpheres == 1) {
        Offset = 0.0f;
        Spacing = 0.0f;
    }

    // Compute total height used by spheres
    float SpheresHeight = Radius * 2.0f * nSpheres;
    float Delta = Height - SpheresHeight;

    // Stretch?
    if (Delta >= 0) {
        Delta /= (float)(nSpheres + 1);
        Offset = (-Height * 0.5f) + Delta + Radius;
        Spacing = (Radius * 2.0f) + Delta;
    } else {
        // Shrink
        float Top = Radius - (Height * 0.5f);
        float Bot = -Top;
        Offset = Top;
        Spacing = (Bot - Top) / (nSpheres - 1);
    }
}

//==============================================================================

bool physics_inst::Init(bool bPopFix, float ConstraintBlendTime)
{
    int i;

    // Free current memory
    Kill();

    // Lookup geometry
    const Geom* pGeom = m_SkinInst.GetGeom();
    if (!pGeom) {
        return false;
    }

    // No rigid bodies?
    if (pGeom->numRigidBodies == 0) {
        return false;
    }

    // Setup constraint blending?
    assert(ConstraintBlendTime >= 0.0f);
    if (ConstraintBlendTime > 0.0f) {
        // Blend in constraints
        m_ConstraintWeight = 0.0f;
        m_ConstraintWeightDelta = 1.0f / ConstraintBlendTime;
    } else {
        // Constraints are fully active
        m_ConstraintWeight = 1.0f;
        m_ConstraintWeightDelta = 0.0f;
    }

    // Allocate collision group
    int CollisionGroup = g_PhysicsMgr.GetNextCollisionGroup();

    // Compute pre-allocation counts
    int nRigidBodies = pGeom->numRigidBodies;
    int nCollisionShapes = pGeom->numRigidBodies;
    int nConstraints = (nRigidBodies - 1) * 4;

    // Pre-allocate physics components
    m_RigidBodies.clear();
    //m_RigidBodies.SetGrowAmount( 1 );
    m_RigidBodies.reserve(nRigidBodies);
    m_CollisionShapes.clear();
    //m_CollisionShapes.SetGrowAmount( 1 );
    m_CollisionShapes.reserve(nCollisionShapes);
    m_BodyBodyContraints.clear();
    if (nConstraints) {
        //m_BodyBodyContraints.SetGrowAmount( 4 );
        m_BodyBodyContraints.reserve(nConstraints);
    }
    m_BodyWorldContraints.clear();
    //m_BodyWorldContraints.SetGrowAmount( 4 );

    // Pre-allocate pop fix matrices?
    m_bPopFix = bPopFix;
    if (m_bPopFix) {
        // Allocate pop fix matrices
        //m_PopFixMatrices.SetGrowAmount( 1 );
        m_PopFixMatrices.resize(pGeom->numBones);

        // Clear pop bind matrices
        for (i = 0; i < pGeom->numBones; i++) {
            m_PopFixMatrices[i].Identity();
        }
    }

    // Build physics components
    for (i = 0; i < nRigidBodies; i++) {
        // Lookup geometry rigid body info
        const Geom::RigidBody& GeomRigidBody = pGeom->rigidBodies[i];

        // Compute size
        Vector3 Size(GeomRigidBody.width, GeomRigidBody.height, GeomRigidBody.length);

        // Compute radius
        float Radius = GeomRigidBody.radius;

        // Compute bind L2W
        Matrix4 BodyBindL2W;
        BodyBindL2W.Setup(Vector3(1.0f, 1.0f, 1.0f),
                          GeomRigidBody.bodyBindRotation,
                          GeomRigidBody.bodyBindPosition);

        // Compute world pivot info
        Matrix4 PivotBindL2W;
        Vector3 PivotWorldPos = GeomRigidBody.pivotBindPosition;
        PivotBindL2W.Setup(Vector3(1.0f, 1.0f, 1.0f),
                           GeomRigidBody.pivotBindRotation,
                           GeomRigidBody.pivotBindPosition);

        // Create rigid body
        rigid_body& RigidBody = m_RigidBodies.emplace_back();

        // Init rigid body
        RigidBody.SetCollisionInfo(CollisionGroup, i, GeomRigidBody.collisionMask);
        RigidBody.SetPrevL2W(BodyBindL2W);
        RigidBody.SetL2W(BodyBindL2W);
        RigidBody.ZeroLinearVelocity();
        RigidBody.ZeroAngularVelocity();
        RigidBody.SetLinearDamping(0.1f);
        RigidBody.SetAngularDamping(i == 0 ? 0.2f : 100000.0f); // Let root bone spin!
        RigidBody.SetElasticity(0.1f);
        RigidBody.SetDynamicFriction(0.8f);
        RigidBody.SetStaticFriction(0.9f);
        RigidBody.ClearForces();
        RigidBody.SetWorldCollision((GeomRigidBody.flags & Geom::RigidBody::FLAG_WORLD_COLLISION) != 0);

        // Create collision shape
        collision_shape& Collision = m_CollisionShapes.emplace_back();
        switch (GeomRigidBody.type) {
        default:
            assert(false); //, "Geometry rigid body data is corrupt!" );

        case Geom::RigidBody::TYPE_SPHERE:
        {
            // Setup simple sphere
            Collision.SetType(collision_shape::TYPE_SPHERE);
            Collision.SetSphereCapacity(1);
            Collision.AddSphere(Vector3(0, 0, 0));
        } break;

        case Geom::RigidBody::TYPE_CYLINDER:
        {
            float Height = Size.y;
            int   nSpheres = std::max(1, (int)(0.5f + (Height / (Radius * 2.0f))));

            Vector3 Offset(0, 0, 0);
            Vector3 Delta(0, 0, 0);
            ComputeSpacing(Height, Radius, nSpheres, Offset.y, Delta.y);

            // Set shape type
            if (nSpheres == 1) {
                Collision.SetType(collision_shape::TYPE_SPHERE);
            } else {
                Collision.SetType(collision_shape::TYPE_CAPSULE);
            }

            // Create spheres and flag as capsule
            Collision.SetSphereCapacity(nSpheres);
            for (int j = 0; j < nSpheres; j++) {
                Collision.AddSphere(Offset);
                Offset += Delta;
            }
        } break;

        case Geom::RigidBody::TYPE_BOX:
        {
            // Compute radius and other axis
            Radius = std::min(Size.x, std::min(Size.y, Size.z));
            int Axis0 = 0;
            int Axis1 = 0;
            if (Radius == Size.x) {
                Axis0 = 1;
                Axis1 = 2;
            } else if (Radius == Size.y) {
                Axis0 = 0;
                Axis1 = 2;
            } else {
                Axis0 = 0;
                Axis1 = 1;
            }
            Radius *= 0.5f;

            float Size0 = Size[Axis0];
            float Size1 = Size[Axis1];

            int nSpheres0 = std::max(1, (int)(0.5f + (Size0 / (Radius * 2.0f))));
            int nSpheres1 = std::max(1, (int)(0.5f + (Size1 / (Radius * 2.0f))));

            Vector3 Offset0(0, 0, 0), Delta0(0, 0, 0);
            Vector3 Offset1(0, 0, 0), Delta1(0, 0, 0);

            ComputeSpacing(Size0, Radius, nSpheres0, Offset0[Axis0], Delta0[Axis0]);
            ComputeSpacing(Size1, Radius, nSpheres1, Offset1[Axis1], Delta1[Axis1]);

            // Set shape type
            if ((nSpheres0 == 1) && (nSpheres1 == 1)) {
                Collision.SetType(collision_shape::TYPE_SPHERE);
            } else if ((nSpheres0 == 1) || (nSpheres1 == 1)) {
                Collision.SetType(collision_shape::TYPE_CAPSULE);
            } else {
                Collision.SetType(collision_shape::TYPE_BOX);
            }

            // Create spheres
            Collision.SetSphereCapacity(nSpheres0 * nSpheres1);
            for (int c0 = 0; c0 < nSpheres0; c0++) {
                Vector3 Offset = Offset0 + Offset1;
                for (int c1 = 0; c1 < nSpheres1; c1++) {
                    Collision.AddSphere(Offset);
                    Offset += Delta1;
                }

                Offset0 += Delta0;
            }
        } break;
        }

        // Set shape and mass properties (let root bone rotate easier)
        assert(Collision.GetNSpheres()); // Make sure some spheres were created!
        Collision.SetRadius(Radius);
        RigidBody.SetCollisionShape(&Collision, GeomRigidBody.mass, (i == 0) ? 50.0f : 100.0f);

        // Connect to parent?
        if (GeomRigidBody.iParentBody != -1) {
            // Clear twist angle
            float TwistAngle = 0.0f;

            // Get parent body
            assert(GeomRigidBody.iParentBody < i);
            rigid_body& ParentRigidBody = m_RigidBodies[(int)GeomRigidBody.iParentBody];

            // Create hinge?
            if (1) {
                // Create hinge?
                for (int j = 0; j < 3; j++) {
                    // Lookup rotation DOFs
                    const Geom::RigidBody::Dof& GeomDOF = GeomRigidBody.dof[j + Geom::RigidBody::Dof::DOF_RX];
                    const Geom::RigidBody::Dof& PrevDOF = GeomRigidBody.dof[((j - 1 + 3) % 3) + Geom::RigidBody::Dof::DOF_RX];
                    const Geom::RigidBody::Dof& NextDOF = GeomRigidBody.dof[((j + 1) % 3) + Geom::RigidBody::Dof::DOF_RX];

                    // Create hinge on this axis?
                    if ((GeomDOF.flags & Geom::RigidBody::Dof::FLAG_ACTIVE) && (GeomDOF.flags & Geom::RigidBody::Dof::FLAG_LIMITED)) {
                        // Setup axis, direction, and angle sign
                        int Axis = j;

                        // Setup pivot local rotation axis
                        Vector3 PivotLocalAxis(0, 0, 0);
                        PivotLocalAxis[Axis] = 1.0f;

                        // Setup pivot world rotation axis
                        Vector3 PivotWorldAxis = PivotBindL2W.RotateVector(PivotLocalAxis);
                        PivotWorldAxis.Normalize();

                        // Compute width of hinge
                        float W = Collision.ComputeLocalBBox().GetSize()[Axis];

                        // Increase width of hinge constraints to make more stable on small rigid bodies
                        W *= 4.0f;

                        // Compute hinge points in world space
                        Vector3 HingeOffset = PivotWorldAxis * 0.5f * W;
                        Vector3 Hinge0 = PivotWorldPos - HingeOffset;
                        Vector3 Hinge1 = PivotWorldPos + HingeOffset;

                        // Compute sloppy twist amount
                        float TwistMaxDist = 0.0f;
                        if ((PrevDOF.flags & Geom::RigidBody::Dof::FLAG_LIMITED) == 0) {
                            TwistAngle = std::max(TwistAngle, PrevDOF.max - PrevDOF.min);
                        }
                        if ((NextDOF.flags & Geom::RigidBody::Dof::FLAG_LIMITED) == 0) {
                            TwistAngle = std::max(TwistAngle, NextDOF.max - NextDOF.min);
                        }
                        if (TwistAngle != 0) {
                            // Convert to max distance constraint
                            Vector3 P0(0.0f, 0.0f, W * 0.5f);
                            Vector3 P1(0.0f, 0.0f, W * 0.5f);
                            P1.RotateY(DEG_TO_RAD(TwistAngle * 0.5f));
                            TwistMaxDist = (P0 - P1).Length();
                        }

                        // Add hinge0 constraint
                        constraint& Constraint0 = m_BodyBodyContraints.emplace_back();
                        Constraint0.Init(&RigidBody,                // pBody0
                                         &ParentRigidBody,          // pBody1
                                         Hinge0,                    // WorldPos0
                                         TwistMaxDist,              // MaxDist
                                         constraint::FLAG_BLEND_IN, // Flags
                                         COLOR_BLUE);               // DebugColor

                        // Add hinge1 constraint
                        constraint& Constraint1 = m_BodyBodyContraints.emplace_back();
                        Constraint1.Init(&RigidBody,                // pBody0
                                         &ParentRigidBody,          // pBody1
                                         Hinge1,                    // WorldPos0
                                         TwistMaxDist,              // MaxDist
                                         constraint::FLAG_BLEND_IN, // Flags
                                         COLOR_RED);                // DebugColor

                        // Limit angle?
                        if (GeomDOF.flags & Geom::RigidBody::Dof::FLAG_LIMITED) {
                            // Lookup limits
                            float AngleMin = GeomDOF.min;
                            float AngleMax = GeomDOF.max;

                            //TEMP!
                            //AngleMin = AngleMax;  // MAX POSE TEST
                            //AngleMax = AngleMin;  // MIN POSE TEST

                            // Compute min, max, and mid Quaternion rotations
                            Quaternion QMinRot, QMaxRot, QMidRot;
                            QMinRot.Setup(PivotWorldAxis, DEG_TO_RAD(AngleMin));
                            QMaxRot.Setup(PivotWorldAxis, DEG_TO_RAD(AngleMax));
                            QMidRot = BlendSlow(QMaxRot, QMinRot, 0.5f);

                            // Compute min, max, and mid matrix rotations
                            Matrix4 MinRot, MaxRot, MidRot;
                            MinRot.Setup(QMinRot);
                            MaxRot.Setup(QMaxRot);
                            MidRot.Setup(QMidRot);

                            // Compute min/max limit world space positions (rotating around pivot position)
                            // Reading from right->left:
                            //    WorldMinTM = PivotBindPos * MinRot * InvPivotBindPos * BodyBindL2W
                            // Optimizes to this:

                            // Put into world space, then make relative to pivot pos
                            Matrix4 InvPivotBindPos_BodyBindL2W = BodyBindL2W;
                            InvPivotBindPos_BodyBindL2W.Translate(-PivotWorldPos);

                            // Apply rotation around pivot
                            Matrix4 WorldMinTM = MinRot * InvPivotBindPos_BodyBindL2W;
                            Matrix4 WorldMaxTM = MaxRot * InvPivotBindPos_BodyBindL2W;
                            Matrix4 WorldMidTM = MidRot * InvPivotBindPos_BodyBindL2W;

                            // Put back into world space
                            WorldMinTM.Translate(PivotWorldPos);
                            WorldMaxTM.Translate(PivotWorldPos);
                            WorldMidTM.Translate(PivotWorldPos);

                            // Get points and convert to a max distance constraint
                            Vector3 WorldMin = WorldMinTM.GetTranslation();
                            Vector3 WorldMax = WorldMaxTM.GetTranslation();
                            Vector3 WorldMid = WorldMidTM.GetTranslation();
                            float   MinDist = (WorldMin - WorldMid).Length();
                            float   MaxDist = (WorldMax - WorldMid).Length();
                            float   LimitDist = (MinDist + MaxDist) * 0.5f;

                            // Compute inverse mid pos ready for setting up constraint
                            Matrix4 InvWorldMidTM = m4_InvertRT(WorldMidTM);

                            // Add limit constraint
                            constraint& ConstraintLim = m_BodyBodyContraints.emplace_back();
                            ConstraintLim.Init(&RigidBody,                          // pBody0
                                               &ParentRigidBody,                    // pBody1
                                               InvWorldMidTM * WorldMid,            // BodyPos0
                                               ParentRigidBody.GetW2L() * WorldMid, // BodyPos1
                                               LimitDist,                           // MaxDist
                                               constraint::FLAG_BLEND_IN,           // Flags
                                               COLOR_PURPLE);                       // DebugColor
                        }
                    }
                }
            }

            // Always create a pivot constraint to keep hinge extra strong
            {
                // Create constraint
                constraint& Constraint = m_BodyBodyContraints.emplace_back();

                // Init constraint
                Constraint.Init(&RigidBody,       // pBody0
                                &ParentRigidBody, // pBody1
                                PivotWorldPos,    // WorldPos
                                0.0f,             // MaxDist
                                0,                // Flags
                                COLOR_YELLOW);    // DebugColor

                // Keep ptr to pivot constraint for rendering
                RigidBody.SetPivotConstraint(&Constraint);
            }
        }
    }

    // Add to physics manager
    g_PhysicsMgr.AddInstance(this);

    // Add to physics manager active list
    g_PhysicsMgr.WakeupInstance(this);

    // Success!
    return true;
}

//==============================================================================

bool physics_inst::Init(const char* pGeomName, bool bPopFix, float ConstraintBlendTime)
{
    m_SkinInst.SetUpSkinGeom(pGeomName);
    Init(bPopFix, ConstraintBlendTime);
    return m_bInitialized;
}

//==============================================================================

bool physics_inst::Init(const skin_inst& SkinInst, bool bPopFix, float ConstraintBlendTime)
{
    m_SkinInst = SkinInst;
    Init(bPopFix, ConstraintBlendTime);
    return m_bInitialized;
}

//==============================================================================

void physics_inst::Kill()
{
    // Remove from physics manager list
    g_PhysicsMgr.RemoveInstance(this);

    // Free memory
    m_RigidBodies.clear();
    m_CollisionShapes.clear();
    m_BodyBodyContraints.clear();
    m_BodyWorldContraints.clear();
    m_PopFixMatrices.clear();
}

//==============================================================================
// Render functions
//==============================================================================

void physics_inst::Render(uint32_t Flags, Colour Ambient)
{
    // Get geometry
    const Geom* pGeom = m_SkinInst.GetGeom();
    if (!pGeom) {
        return;
    }

    // Get geometry bone matrices
    uint64_t       LODMask;
    int            nActiveBones;
    const Matrix4* pMatrices = GetBoneL2Ws(LODMask, nActiveBones);
    if (!pMatrices) {
        return;
    }

    // Render skin
    m_SkinInst.Render(&m_RigidBodies[0].GetL2W(),
                      pMatrices,
                      nActiveBones,
                      Flags,
                      LODMask,
                      Ambient);

    // Activate all bodies?
    if (m_bActiveWhenVisible) {
        Activate();
    }
}

Vector3 physics_inst::GetPosition() const
{
    // Take average position of all bodies
    Vector3 Pos(0.0f, 0.0f, 0.0f);
    if (m_RigidBodies.size()) {
        // Accumulate all positions
        for (int i = 0; i < m_RigidBodies.size(); i++) {
            Pos += m_RigidBodies[i].GetPosition();
        }

        // Take average
        Pos /= (float)m_RigidBodies.size();
    }

    return Pos;
}

//==============================================================================
// Active functions
//==============================================================================

float physics_inst::GetSpeedSqr() const
{
    float SpeedSqr = 0.0f;

    // Loop through all bodies and add speed
    for (int i = 0; i < m_RigidBodies.size(); i++) {
        // Lookup body
        const rigid_body& Body = m_RigidBodies[i];

        // Take linear and angular into account
        SpeedSqr += Body.GetLinearVelocity().LengthSquared();
        SpeedSqr += Body.GetAngularVelocity().LengthSquared();
    }

    return SpeedSqr;
}

//==============================================================================

bool physics_inst::HasActiveEnergy() const
{
    // Loop through all bodies and freeze
    for (int i = 0; i < m_RigidBodies.size(); i++) {
        // Lookup body
        const rigid_body& Body = m_RigidBodies[i];

        // If body has active energy, then so does instance
        if (Body.HasActiveEnergy()) {
            return true;
        }
    }

    // No active bodies were found
    return false;
}

//==============================================================================

void physics_inst::Deactivate()
{
    assert(m_SkinInst.GetGeom());                       //, "Ragdoll geometry is missing - make sure all resources are compiled!" );
    assert(!(m_SkinInst.GetGeom() && !m_bInitialized)); //, "Geometry is present, but ragdoll not initialized?! - Grab SteveB" );

    // Loop through all bodies and deactivate
    for (int i = 0; i < m_RigidBodies.size(); i++) {
        // Lookup body
        rigid_body& Body = m_RigidBodies[i];

        // Deactivate
        Body.Deactivate();
    }

    // Remove from physics manager active list
    g_PhysicsMgr.PutToSleepInstance(this);
}

//==============================================================================

void physics_inst::Activate()
{
    assert(m_SkinInst.GetGeom());                       //, "Ragdoll geometry is missing - make sure all resources are compiled!" );
    assert(!(m_SkinInst.GetGeom() && !m_bInitialized)); //, "Geometry is present, but ragdoll not initialized?! - Grab SteveB" );
    assert(m_bInitialized);

    // Loop through all bodies and activate
    for (int i = 0; i < m_RigidBodies.size(); i++) {
        // Lookup body
        rigid_body& Body = m_RigidBodies[i];

        // Activate
        Body.Activate();
    }

    // Add to physics manager active list
    g_PhysicsMgr.WakeupInstance(this);
}

//==============================================================================
// Matrix functions
//==============================================================================

void physics_inst::DirtyMatrices()
{
    // Flag matrices need rebuilding
    m_MatrixCache.SetDirty(true);
}

//==============================================================================

const Matrix4* physics_inst::GetBoneL2Ws(uint64_t& LODMask, int& nActiveBones)
{
    int i;

    // Must have some rigid bodies!
    int nRigidBodies = m_RigidBodies.size();
    if (!nRigidBodies) {
        return nullptr;
    }

    // Get geometry
    const Geom* pGeom = m_SkinInst.GetGeom();
    if (!pGeom) {
        return nullptr;
    }

    // Geometry rigid body count must match physics rigid body count!
    assert(nRigidBodies == pGeom->numRigidBodies);

    // Compute render info
    LODMask = m_SkinInst.GetLODMask(m_RigidBodies[0].GetL2W());
    nActiveBones = m_SkinInst.GetNActiveBones(LODMask);
    assert(nActiveBones > 0);
    assert(nActiveBones <= pGeom->numBones);

    // Allocate matrices
    Matrix4* pBoneMatrices = m_MatrixCache.GetMatrices(nActiveBones);
    if (!pBoneMatrices) {
        return nullptr;
    }

    // Already valid?
    if (m_MatrixCache.IsValid(nActiveBones)) {
        return pBoneMatrices;
    }

    // Allocate rigid body matrices

    assert(nRigidBodies <= 32);
    static Matrix4 pBodyMatrices[32];

    // Start with physics rigid body L2W's
    for (i = 0; i < nRigidBodies; i++) {
        pBodyMatrices[i] = m_RigidBodies[i].GetL2W();
    }

    // Iterate over pivot constraints and spread the error correction across connected bodies
    // (this smooths out the impulse jitter that bodies do before they go to sleep)
    for (int Iters = g_PhysicsMgr.m_Settings.m_nRenderIterations; Iters > 0; Iters--) {
        // Alternate directions to spread results more evenly
        int iStart, iEnd, iDir;
        if (Iters & 1) {
            // Forwards (skip end body)
            iStart = 0;
            iEnd = nRigidBodies - 1;
            iDir = 1;
        } else {
            // Backwards (skip start body)
            iStart = nRigidBodies - 1;
            iEnd = 0;
            iDir = -1;
        }

        // Loop through rigid bodies
        for (i = iStart; i != iEnd; i += iDir) {
            // Lookup rigid body info
            Matrix4&    BodyL2W = pBodyMatrices[i];
            rigid_body& RigidBody = m_RigidBodies[i];

            // Is this body connected to a parent body?
            constraint* pPivotConstraint = RigidBody.GetPivotConstraint();
            if (pPivotConstraint) {
                // Make sure this is a pivot constraint
                assert(pPivotConstraint->GetMaxDist() == 0.0f);

                // Lookup parent rigid body index
                int iParentBody = pGeom->rigidBodies[i].iParentBody;
                assert(iParentBody != -1);
                assert(iParentBody != i);
                Matrix4& ParentBodyL2W = pBodyMatrices[iParentBody];

                // Compute world pivot position
                assert(pPivotConstraint->GetRigidBody(0) == &RigidBody);
                Vector3 WorldPivot = BodyL2W * pPivotConstraint->GetBodyPos(0);

                // Compute parent world pivot position
                assert(pPivotConstraint->GetRigidBody(1) == &m_RigidBodies[iParentBody]);
                Vector3 ParentWorldPivot = ParentBodyL2W * pPivotConstraint->GetBodyPos(1);

                // Compute correction and apply half to each body - spread that jitter love!
                Vector3 Delta = 0.5f * (ParentWorldPivot - WorldPivot);
                BodyL2W.Translate(Delta);
                ParentBodyL2W.Translate(-Delta);
            }
        }
    }

    // Bake in inverse bind ready for skinning
    for (i = 0; i < nRigidBodies; i++) {
        // Compute inverse bind matrix for rigid body
        const Geom::RigidBody& Body = pGeom->rigidBodies[i];
        Matrix4                BodyBind(Vector3(1.0f, 1.0f, 1.0f), Body.bodyBindRotation, Body.bodyBindPosition);
        Matrix4                InvBodyBind = m4_InvertRT(BodyBind);

        // Compute final skin L2W
        pBodyMatrices[i] = pBodyMatrices[i] * InvBodyBind;
    }

    // Finally, compute the bone render matrices. Use pop fix?
    if (m_bPopFix) {
        // Loop through all bones
        for (i = 0; i < nActiveBones; i++) {
            // Lookup index of rigid body that bone is attached to
            int iRigidBody = pGeom->bones[i].rigidBodyIdx;
            assert(iRigidBody >= 0);
            assert(iRigidBody < nRigidBodies);

            // Use rigid body skin matrix with pop fix applied
            pBoneMatrices[i] = pBodyMatrices[iRigidBody] * m_PopFixMatrices[i];
        }
    } else {
        // Loop through all bones
        for (i = 0; i < nActiveBones; i++) {
            // Lookup index of rigid body that bone is attached to
            int iRigidBody = pGeom->bones[i].rigidBodyIdx;
            assert(iRigidBody >= 0);
            assert(iRigidBody < nRigidBodies);

            // Just copy rigid body skin matrix
            pBoneMatrices[i] = pBodyMatrices[iRigidBody];
        }
    }

    // Flag matrices as valid
    m_MatrixCache.SetDirty(false);

    return pBoneMatrices;
}

//==============================================================================

Vector3 physics_inst::GetBoneWorldPosition(int iBone)
{
    // Look world transform
    Matrix4 L2W = GetBoneWorldTransform(iBone);

    // Return world position
    return L2W.GetTranslation();
}

//==============================================================================

Matrix4 physics_inst::GetBoneWorldTransform(int iBone)
{
    // Default
    Matrix4 L2W;

    // Lookup geom
    Geom* pGeom = m_SkinInst.GetSkinGeom();
    if (!pGeom) {
        L2W.Identity();
        return L2W;
    }

    // Lookup render matrices
    uint64_t       LODMask;
    int            nActiveBones;
    const Matrix4* pMatrices = GetBoneL2Ws(LODMask, nActiveBones);

    // If no matrices we are screwed
    if (!pMatrices) {
        L2W.Identity();
        return L2W;
    }

    // If bone is not visible, attach to root bone
    if ((iBone >= nActiveBones) || (iBone >= pGeom->numBones)) {
        iBone = 0;
    }

    // Lookup transform with bind baked in
    L2W = pMatrices[iBone];

    // Counter act inverse bind position (rotation is always zero)
    assert(iBone < pGeom->numBones);
    L2W.PreTranslate(pGeom->bones[iBone].bindPosition);

    return L2W;
}

//==============================================================================

void physics_inst::SetMatrices(loco_char_anim_player& AnimPlayer, const Vector3& Vel)
{
    // No bodies setup?
    if (!m_RigidBodies.size()) {
        return;
    }

    // Get geometry
    const Geom* pGeom = m_SkinInst.GetGeom();
    if (!pGeom) {
        return;
    }

    // No anim playing?
    loco_motion_controller& CurrAnim = AnimPlayer.GetCurrAnim();
    if (CurrAnim.GetAnimIndex() == -1) {
        return;
    }

    // Lookup anim info
    const AnimInfo& animInfo = CurrAnim.GetAnimInfo();
    float           DeltaTime = (CurrAnim.GetRate() / 30.0f) * (float)animInfo.GetFPS();
    float           LastFrame = (float)(animInfo.GetNFrames() - 2);
    float           CurrFrame = CurrAnim.GetFrame();
    float           NextFrame = CurrFrame + DeltaTime;

    // At end of anim?
    if (CurrFrame >= LastFrame) {
        // Rewind a frame
        CurrFrame = LastFrame - DeltaTime;
        NextFrame = LastFrame;

        // Range check
        if (CurrFrame < 0) {
            CurrFrame = 0.0f;
        }
    } else {
        // Range check
        if (NextFrame > LastFrame) {
            NextFrame = LastFrame;
        }
    }

    // Compute matrices for current frame and setup position
    AnimPlayer.SetNActiveBones(pGeom->numBones);
    AnimPlayer.SetCurrAnimFrame(CurrFrame);
    SetMatrices(AnimPlayer.GetBoneL2Ws(), pGeom->numBones, false);

    // Compute matrices for next frame and inherit vels
    AnimPlayer.SetCurrAnimFrame(NextFrame);
    SetMatrices(AnimPlayer.GetBoneL2Ws(), pGeom->numBones, true);

    // Add final velocity
    for (int i = 0; i < m_RigidBodies.size(); i++) {
        // Lookup body
        rigid_body& Body = m_RigidBodies[i];

        // Add to the velocity
        Body.GetLinearVelocity() += Vel * 30.0f;
    }
}

//==============================================================================

void physics_inst::SetMatrices(const Matrix4* pMatrices, int nBones, bool bInheritVel)
{
    int i, j, k;

    assert(nBones > 0);

    // Must have matrices!
    if (!pMatrices) {
        return;
    }

    // No bodies setup?
    if (!m_RigidBodies.size()) {
        return;
    }

    // Get geometry
    const Geom* pGeom = m_SkinInst.GetGeom();
    if (!pGeom) {
        return;
    }

    // Lookup root bone world position (middle of character)
    // NOTE: Bind pos is added because inverse bind pos is baked into matrix
    Vector3 WorldPos = pGeom->bones[0].bindPosition + pMatrices[0].GetTranslation();

    // Move in the air a bit in-case the npc is sitting on the floor
    WorldPos.y += 40.0f;

    // Setup rigid bodies from animation matrices
    assert(pGeom->numRigidBodies == m_RigidBodies.size());
    for (i = 0; i < m_RigidBodies.size(); i++) {
        // Lookup rigid bodies
        rigid_body&            Body = m_RigidBodies[i];
        const Geom::RigidBody& GeomBody = pGeom->rigidBodies[i];

        // Clear body movement
        Body.ClearForces();
        Body.ZeroLinearVelocity();
        Body.ZeroAngularVelocity();

        // Compute rigid body bind and inverse bind
        Matrix4 BodyBind;
        BodyBind.Setup(Vector3(1.0f, 1.0f, 1.0f),
                       GeomBody.bodyBindRotation,
                       GeomBody.bodyBindPosition);

        // Make sure bone is valid
        assert(GeomBody.iBone >= 0);
        assert(GeomBody.iBone < pGeom->numBones);
        assert(GeomBody.iBone < nBones);

        // Compute rigid body L2Ws
        Matrix4 CurrBodyL2W = pMatrices[GeomBody.iBone] * BodyBind;

        // Inherit motion too?
        if (bInheritVel) {
            // Compute velocity (don't need to take into account rotation - this looks good enough)
            const Matrix4& PrevBodyL2W = Body.GetL2W();
            Vector3        Motion = CurrBodyL2W.GetTranslation() - PrevBodyL2W.GetTranslation();
            Body.SetLinearVelocity(Motion * 30.0f);
        }

        // Set new rigid body L2W
        Body.SetPrevL2W(CurrBodyL2W);
        Body.SetL2W(CurrBodyL2W);

        // Setup collision shapes
        for (j = 0; j < m_CollisionShapes.size(); j++) {
            // Lookup collision spheres shape
            collision_shape& Shape = m_CollisionShapes[j];

            // Setup spheres
            for (k = 0; k < Shape.GetNSpheres(); k++) {
                // Lookup sphere
                collision_shape::sphere& Sphere = Shape.GetSphere(k);

                // Setup coll free pos (hopefully!) to be in middle of character
                Sphere.m_CollFreePos = WorldPos;

                // Setup start and end positions
                Sphere.m_PrevPos =
                    Sphere.m_CurrPos = CurrBodyL2W * Sphere.m_Offset;
            }
        }
    }

    // Compute pop fix bind matrices?
    if (m_bPopFix) {
        // Create pop fix matrix for every geometry bone...
        for (i = 0; i < nBones; i++) {
            // Lookup bone
            const Geom::Bone& Bone = pGeom->bones[i];

            // Lookup rigid body
            assert(Bone.rigidBodyIdx >= 0);
            assert(Bone.rigidBodyIdx < nBones);
            const Geom::RigidBody& Body = pGeom->rigidBodies[Bone.rigidBodyIdx];

            // Compute inverse bind matrix for rigid body
            Matrix4 BodyBind(Vector3(1.0f, 1.0f, 1.0f), Body.bodyBindRotation, Body.bodyBindPosition);
            Matrix4 InvBodyBind = m4_InvertRT(BodyBind);

            // Compute BoneL2W that will be computed from rigid body transform without pop fix up
            Matrix4 BoneL2W = m_RigidBodies[(int)Bone.rigidBodyIdx].GetL2W() * InvBodyBind;

            // Compute correction matrix that will fix the pop
            m_PopFixMatrices[i] = m4_InvertRT(BoneL2W) * pMatrices[i];
        }
    }
}

//==============================================================================
// Blast/force functions
//==============================================================================

void physics_inst::ApplyBlast(const Vector3& Pos, float Radius, float Amount)
{
    assert(Radius > 0.01f);
    assert(Pos.IsValid());

    // Compute radius squared
    float RadiusSqr = x_sqr(Radius);

    // Loop through all rigid bodies
    bool bActivate = false;
    for (int i = 0; i < m_RigidBodies.size(); i++) {
        // Lookup body
        rigid_body& Body = m_RigidBodies[i];

        // Compute distance of body from blast
        Vector3 Delta = Body.GetPosition() - Pos;
        float   DistSqr = Delta.LengthSquared();

        // Within force radius and not right on top of rigid body?
        if ((DistSqr < RadiusSqr) && (DistSqr > 0.001f)) {
            // Compute dist and inverse dist
            float Dist = 0.0f;
            float InvDist = 1.0f;
            if (DistSqr > 0.0001f) {
                Dist = sqrt(DistSqr);
                InvDist = 1.0f / Dist;
            }

            // Compute direction
            Vector3 Dir = Delta * InvDist;

            // Compute force
            Vector3 Force = Amount * Dir * ((Radius - Dist) / Radius);

            // Keep force valid
            ClampForce(Force);

            // Apply force
            Body.ApplyWorldImpulse(Force);
            bActivate = true;
        }
    }

    // Make sure instance is in physics managers active list
    if (bActivate) {
        g_PhysicsMgr.WakeupInstance(this);
    }
}

//==============================================================================

void physics_inst::ApplyBlast(const Vector3& Pos, const Vector3& Dir, float Radius, float Amount)
{
    assert(Pos.IsValid());
    assert(Dir.IsValid());
    assert(Dir.LengthSquared() <= x_sqr(1.1f));
    assert(isvalid(Radius));
    assert(isvalid(Amount));
    assert(Radius > 0.01f);
    assert(Amount >= 0.0f);

    // Compute radius squared
    float RadiusSqr = x_sqr(Radius);

    // Loop through all rigid bodies
    bool bActivate = false;
    for (int i = 0; i < m_RigidBodies.size(); i++) {
        // Lookup body
        rigid_body& Body = m_RigidBodies[i];

        // Compute distance of body from blast
        Vector3 Delta = Body.GetPosition() - Pos;
        float   DistSqr = Delta.LengthSquared();

        // Within radius?
        if (DistSqr < RadiusSqr) {
            // Compute dist
            float Dist = 1.0f;
            if (DistSqr > 0.0001f) {
                Dist = sqrt(DistSqr);
            }

            // Compute force
            Vector3 Force = Amount * Dir * ((Radius - Dist) / Radius);

            // Keep force valid
            ClampForce(Force);

            // Apply force
            Body.ApplyWorldImpulse(Force);
            bActivate = true;
        }
    }

    // Make sure instance is in physics managers active list
    if (bActivate) {
        g_PhysicsMgr.WakeupInstance(this);
    }
}

//==============================================================================

void physics_inst::ApplyVectorForce(const Vector3& Dir, float Amount)
{
    assert(Dir.IsValid());
    assert(isvalid(Amount));
    assert(Amount >= 0.0f);

    // Compute force
    assert(Dir.IsValid());
    Vector3 Force = Dir * Amount;

    // Keep force valid
    ClampForce(Force);

    // Loop through all rigid bodies
    for (int i = 0; i < m_RigidBodies.size(); i++) {
        // Lookup body
        rigid_body& Body = m_RigidBodies[i];

        // Apply force
        Body.ApplyWorldImpulse(Force);
    }

    // Make sure instance is in physics managers active list
    g_PhysicsMgr.WakeupInstance(this);
}

//==============================================================================
// Collision functions
//==============================================================================

void physics_inst::OnColCheck(guid OwnerObject)
{
    g_CollisionMgr.StartApply(OwnerObject);

    // Loop over all collision shapes
    for (int i = 0; i < m_CollisionShapes.size(); i++) {
        // Lookup collision
        const collision_shape& Shape = m_CollisionShapes[i];

        // Lookup rigid body ID so corpse can use collision info
        assert(Shape.GetOwner());
        int ID = Shape.GetOwner()->GetCollisionID();

        // Loop over all collision spheres
        for (int j = 0; j < Shape.GetNSpheres(); j++) {
            // Lookup sphere
            const collision_shape::sphere& Sphere = Shape.GetSphere(j);

            // Apply
            g_CollisionMgr.ApplySphere(Sphere.m_CurrPos, Shape.m_Radius, Object::MAT_TYPE_FLESH, ID);
        }
    }

    g_CollisionMgr.EndApply();
}

//==============================================================================

void physics_inst::ComputeWorldBBox(void)
{
    // Clear bbox
    m_WorldBBox.Clear();

    // Loop through all bodies and accumulate bounding boxes
    for (int i = 0; i < m_RigidBodies.size(); i++) {
        // Lookup body
        rigid_body& Body = m_RigidBodies[i];

        // Update collision shape of body
        collision_shape* pColl = Body.GetCollisionShape();
        if (pColl) {
            // Collide with world?
            if (Body.m_Flags & rigid_body::FLAG_WORLD_COLLISION) {
                // Prepare for sweep test
                pColl->SetL2W(Body.GetPrevL2W(), Body.GetL2W());
            } else {
                // Set directly
                pColl->SetL2W(Body.GetL2W());
            }
        }

        // Compute bbox of body
        Body.ComputeWorldBBox();

        // Accumulate world bbox of rigid body
        m_WorldBBox += Body.GetWorldBBox();
    }
}

//==============================================================================
// Rigid body functions
//==============================================================================

std::string physics_inst::GetRigidBodyName(int iRigidBody) const
{
    // Lookup geometry
    const SkinGeom* pGeom = m_SkinInst.GetSkinGeom();
    if (!pGeom) {
        return "nullptr";
    }

    // Invalid rigid body?
    assert(m_RigidBodies.size() == pGeom->numRigidBodies);
    if ((iRigidBody < 0) || (iRigidBody >= pGeom->numRigidBodies)) {
        return "nullptr";
    }

    // Found!
    return pGeom->GetRigidBodyName(iRigidBody);
}

//==============================================================================
// Constraint functions
//==============================================================================

int physics_inst::AddBodyWorldConstraint(int            iRigidBody,
                                         const Vector3& WorldPos,
                                         float          MaxDist)
{
    // Invalid rigid body or not loaded yet?
    if ((iRigidBody < 0) || (iRigidBody >= m_RigidBodies.size())) {
        return -1;
    }

    // Create a new constraint
    int         Index = m_BodyWorldContraints.size();
    constraint& Constraint = m_BodyWorldContraints.emplace_back();

    // Init constraint
    Constraint.Init(&m_RigidBodies[iRigidBody], // pBody0
                    &g_WorldBody,               // pBody1
                    WorldPos,                   // WorldPos
                    MaxDist,                    // MaxDist
                    0,                          // Flags
                    COLOR_PURPLE);              // DebugColor

    return Index;
}

//==============================================================================
// Logic functions
//==============================================================================

void physics_inst::Advance(float DeltaTime)
{
    // Update constraint blending
    m_ConstraintWeight += m_ConstraintWeightDelta * DeltaTime;
    m_ConstraintWeight = std::clamp(m_ConstraintWeight, 0.0f, 1.0f);

    // Keep active?
    if (m_KeepActiveTime > 0.0f) {
        // Update timer
        m_KeepActiveTime -= DeltaTime;

        // Keep all bodies active
        Activate();
    }
}
