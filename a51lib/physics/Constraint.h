#pragma once

#include "RigidBody.h"

class constraint;

class active_constraint
{
    //==============================================================================
    // Data
    //==============================================================================
public:
    // Constraint vars
    constraint* m_pOwner; // Owner constraint

    // Linked list nodes
    std::vector<active_constraint*> m_SolveListNode; // In solve list

    // Derived quantities
    bool    m_bSatisfied;    // Does this constraint need
    Vector3 m_RelMidPos0;    // Mid points relative to bodies space
    Vector3 m_RelMidPos1;    // Mid points relative to bodies space
    Vector3 m_RelPos0;       // Points relative to bodies space
    Vector3 m_RelPos1;       // Points relative to bodies space
    Vector3 m_CorrectionVel; // Velocity correction based on distance
    float   m_Weight;        // Weight of constraint ( 0 = no effect, 1 = full effect )
};

class constraint
{
    //==============================================================================
    // Defines
    //==============================================================================
public:
    // Logic flags
    enum flags
    {
        FLAG_BLEND_IN = (1 << 0), // Should constraint be blended in?
    };

    //==============================================================================
    // Functions
    //==============================================================================
public:
    constraint();
    ~constraint();

    // Query functions
    rigid_body*    GetRigidBody(int Index) const;
    const Vector3& GetBodyPos(int Index) const;
    Vector3&       GetBodyPos(int Index);
    void           SetBodyPos(int Index, const Vector3& BodyPos);

    Vector3 GetWorldPos(int Index) const;
    void    SetWorldPos(int Index, const Vector3& WorldPos);

    float    GetMaxDist() const;
    uint32_t GetFlags() const;

    // Initialization with world position
    void Init(rigid_body*    pBody0,
              rigid_body*    pBody1,
              const Vector3& WorldPos,
              float          MaxDist,
              uint32_t       Flags,
              Colour         DebugColor);

    // Initialization with local position for each body
    void Init(rigid_body*    pBody0,
              rigid_body*    pBody1,
              const Vector3& Body0Pos,
              const Vector3& Body1Pos,
              float          MaxDist,
              uint32_t       Flags,
              Colour         DebugColor);

    // Logic functions
    bool IsActive() const;
    bool PreApply(float DeltaTime, active_constraint& Active);
    bool Apply(active_constraint& Active);

protected:
    // Body space positions and max world space distance allowed
    Vector3  m_BodyPos0; // Points in body0 space
    Vector3  m_BodyPos1; // Points in body1 space
    uint32_t m_Flags;    // Logic flags
    float    m_MaxDist;  // Max distance allowed between points

    // Rigid bodies
    rigid_body* m_pBody0; // Body0 to constrain
    rigid_body* m_pBody1; // Body1 to constrain

    friend class physics_mgr;
};

inline constraint::constraint()
    : m_BodyPos0(0.0f, 0.0f, 0.0f)
    , m_BodyPos1(0.0f, 0.0f, 0.0f)
    , m_MaxDist(0.0f)
    , m_pBody0(NULL)
    , m_pBody1(NULL)
{
}

//==============================================================================

inline constraint::~constraint()
{
}

inline rigid_body* constraint::GetRigidBody(int Index) const
{
    assert(Index >= 0);
    assert(Index < 2);
    return (&m_pBody0)[Index];
}

//==============================================================================

inline const Vector3& constraint::GetBodyPos(int Index) const
{
    assert(Index >= 0);
    assert(Index < 2);
    return (&m_BodyPos0)[Index];
}

//==============================================================================

inline Vector3& constraint::GetBodyPos(int Index)
{
    assert(Index >= 0);
    assert(Index < 2);
    return (&m_BodyPos0)[Index];
}

//==============================================================================

inline void constraint::SetBodyPos(int Index, const Vector3& BodyPos)
{
    GetBodyPos(Index) = BodyPos;
}

//==============================================================================

inline Vector3 constraint::GetWorldPos(int Index) const
{
    // Lookup rigid body
    rigid_body* pBody = GetRigidBody(Index);
    assert(pBody);

    // Lookup body pos
    const Vector3& BodyPos = GetBodyPos(Index);

    // Convert to world space
    return pBody->GetL2W() * BodyPos;
}

//==============================================================================

inline void constraint::SetWorldPos(int Index, const Vector3& WorldPos)
{
    // Lookup rigid body
    rigid_body* pBody = GetRigidBody(Index);
    assert(pBody);

    // Convert to body space
    SetBodyPos(Index, pBody->GetW2L() * WorldPos);
}

//==============================================================================

inline float constraint::GetMaxDist() const
{
    return m_MaxDist;
}

//==============================================================================

inline uint32_t constraint::GetFlags() const
{
    return m_Flags;
}

//==============================================================================
// Logic functions
//==============================================================================

inline bool constraint::IsActive() const
{
    assert(m_pBody0);
    assert(m_pBody1);

    // Body0 active?
    if (m_pBody0->IsActive()) {
        return true;
    }

    // Body1 active?
    if (m_pBody1->IsActive()) {
        return true;
    }

    // Both inactive so constraint can be inactive
    return false;
}
