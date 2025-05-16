
#include "PhysicsMgr.h"
#include "Constraint.h"

// Initialization with world position
void constraint::Init(rigid_body*    pBody0,
                      rigid_body*    pBody1,
                      const Vector3& WorldPos,
                      float          MaxDist,
                      uint32_t       Flags,
                      Colour         DebugColor)
{
    assert(pBody0);
    assert(pBody1);

    m_pBody0 = pBody0;
    m_pBody1 = pBody1;
    m_BodyPos0 = pBody0->GetW2L() * WorldPos;
    m_BodyPos1 = pBody1->GetW2L() * WorldPos;
    m_MaxDist = MaxDist;
    m_Flags = Flags;
}

//==============================================================================

// Initialization with local position for each body
void constraint::Init(rigid_body*    pBody0,
                      rigid_body*    pBody1,
                      const Vector3& Body0Pos,
                      const Vector3& Body1Pos,
                      float          MaxDist,
                      uint32_t       Flags,
                      Colour         DebugColor)
{
    assert(pBody0);
    assert(pBody1);

    m_pBody0 = pBody0;
    m_pBody1 = pBody1;
    m_BodyPos0 = Body0Pos;
    m_BodyPos1 = Body1Pos;
    m_MaxDist = MaxDist;
    m_Flags = Flags;
}

//==============================================================================

bool constraint::PreApply(float DeltaTime, active_constraint& Active)
{
    // TO DO: Use prediction for max constraint?

    // Lookup bodies
    rigid_body* pBody0 = m_pBody0;
    rigid_body* pBody1 = m_pBody1;
    assert(pBody0);
    assert(pBody1);

    // Compute world space positions
    Vector3 WorldPos0 = pBody0->GetL2W() * m_BodyPos0;
    Vector3 WorldPos1 = pBody1->GetL2W() * m_BodyPos1;

    // Compute world delta
    Vector3 Delta = WorldPos0 - WorldPos1;
    float   DistSqr = Delta.LengthSquared();

    // Constraint already satisfied?
    // NOTE: Always keep pin (zero dist) constraints active so that limbs don't jerk
    //       (fixes the punchbag dummy from jittering)
    float MaxDist = m_MaxDist;
    float MaxDistSqr = x_sqr(MaxDist);
    if ((m_MaxDist > 0.0f) && (DistSqr <= MaxDistSqr)) {
        return false;
    }

    // Compute mid pos and relative mid positions
    Vector3 WorldMidPos = (WorldPos0 + WorldPos1) * 0.5f;
    Active.m_RelMidPos0 = WorldMidPos - pBody0->GetPosition();
    Active.m_RelMidPos1 = WorldMidPos - pBody1->GetPosition();

    // Compute relative positions
    Active.m_RelPos0 = WorldPos0 - pBody0->GetPosition();
    Active.m_RelPos1 = WorldPos1 - pBody1->GetPosition();

    // Compute deviation distance between points?
    if (DistSqr > 0.0001f) {
        // Normalize direction between constraints
        float Dist = sqrt(DistSqr);
        Delta *= 1.0f / Dist;

        // Compute deviation dist from constraint limit
        if (Dist > MaxDist) {
            Dist -= MaxDist;
        }

        // Compute correction scaler
        float Extra = Dist * g_PhysicsMgr.m_Settings.m_ConstraintFix / (DeltaTime * 60.0f);
        float MaxConstraintFix = g_PhysicsMgr.m_Settings.m_MaxConstraintFix;
        if (Extra > MaxConstraintFix) {
            Extra = MaxConstraintFix;
        } else if (Extra < -MaxConstraintFix) {
            Extra = -MaxConstraintFix;
        }

        // Compute velocity correction based on deviation distance
        Active.m_CorrectionVel = Delta * Extra;
    } else {
        // No correction needed or it can't be computed due to points being on top of each other
        Active.m_CorrectionVel.Zero();
    }

    // Needs solving
    return true;
}

//==============================================================================

bool constraint::Apply(active_constraint& Active)
{
    // Lookup bodies
    rigid_body* pBody0 = m_pBody0;
    rigid_body* pBody1 = m_pBody1;
    assert(pBody0);
    assert(pBody1);

    // Compute velocities of each point
    Vector3 Vel0 = pBody0->GetLinearVelocity() + v3_Cross(pBody0->GetAngularVelocity(), Active.m_RelPos0);
    Vector3 Vel1 = pBody1->GetLinearVelocity() + v3_Cross(pBody1->GetAngularVelocity(), Active.m_RelPos1);

    // Compute relative velocity
    Vector3 RelVel = Vel0 - Vel1 + Active.m_CorrectionVel;

    // Compute relative speed
    float RelSpeedSqr = RelVel.LengthSquared();
    if (RelSpeedSqr < 0.00001f) {
        return false;
    }
    float RelSpeed = sqrt(RelSpeedSqr);

    // Compute impulse to satisfy constraint
    Vector3 N = RelVel / RelSpeed;
    float   Numerator = -RelSpeed;
    float   Denominator = pBody0->GetInvMass() + pBody1->GetInvMass();

    // Add components
    Vector3 DENOM0 = v3_Cross(pBody0->GetWorldInvInertia().RotateVector(v3_Cross(Active.m_RelPos0, N)), Active.m_RelPos0);
    Vector3 DENOM1 = v3_Cross(pBody1->GetWorldInvInertia().RotateVector(v3_Cross(Active.m_RelPos1, N)), Active.m_RelPos1);
    Denominator += v3_Dot(N, DENOM0 + DENOM1);

    // Valid?
    if (Denominator < 0.00001f) {
        return false;
    }

    // Apply impulse to active bodies only so other body is not woken up!
    Vector3 NormalImpulse = Active.m_Weight * N * (Numerator / Denominator);

    if (pBody0->IsActive()) {
        pBody0->ApplyLocalImpulse(NormalImpulse, Active.m_RelMidPos0);
    }

    if (pBody1->IsActive()) {
        pBody1->ApplyLocalImpulse(-NormalImpulse, Active.m_RelMidPos1);
    }

    return true;
}
