#pragma once

#include "../VectorMath.h"
#include "../animation/animData.h"
#include "../resourceManager/ResourceManager.h"
#include "../render/Geom.h"

class loco_ik_solver
{
public:
    // Bone info
    struct bone_mapping
    {
        // Data
        int     m_iBone; // Index of geometry bone
        Matrix4 m_B2S;   // Bone space to solver space mapping
        Matrix4 m_S2B;   // Solver to bone space mapping
    };

    // Body to body constraint
    struct constraint
    {
        // Data
        int     m_iBone0;     // Index of bone0
        int     m_iBone1;     // Index of bone1
        Vector3 m_LocalPos0;  // Position in local space of bone0
        Vector3 m_LocalPos1;  // Position in local space of bone1
        float   m_MinDist;    // Minimum distance points should be
        float   m_MaxDist;    // Maximum distance points should be
        float   m_MassRatio0; // Mass ratio of bone0
        float   m_MassRatio1; // Mass ratio of bone 1
        float   m_Inertia0;   // Rotation inertia of bone0
        float   m_Inertia1;   // Rotation inertia of bone1

        // Functions
        void SolveBone(Matrix4&       M,
                       const Vector3& Pos,
                       const Vector3& Dir,
                       float          Amount,
                       float          MassRatio,
                       float          Inertia);

        void Solve(Matrix4* pMatrices, int nActiveBones, float Weight);
    };

    //=========================================================================
    // PUBLIC FUNCTIONS
    //=========================================================================
public:
    loco_ik_solver();
    virtual ~loco_ik_solver();

    // Initializes solver
    void Init(bone_mapping* pBoneMappings, int nBoneMappings,
              constraint* pConstraints, int nConstraints,
              int nIterations);

private:
    // Solves all constraints
    void SolveConstraints(Matrix4* pMatrices, int nActiveBones);

public:
    // Applies all constraints and solves matrices
    void Solve(Matrix4* pMatrices, int nActiveBones);

    // Sets the IK solving weight amount
    void SetWeight(float Weight);

    // Retrieves the current weight
    float GetWeight() const;

    //=========================================================================
    // PRIVATE DATA
    //=========================================================================
protected:
    bone_mapping* m_pBoneMappings; // Pointer to list of bone mappings
    int           m_nBoneMappings; // # if bones to solve

    constraint* m_pConstraints; // Pointer to list of constraints
    int         m_nConstraints; // # of constraints

    int   m_nIterations; // # of iterations to do when solving
    float m_Weight;      // Weight of IK. 0 = none, 1= full

    //=========================================================================
    // FRIENDS
    //=========================================================================
    friend class loco_char_anim_player;
    friend class loco;
};

//=========================================================================
// INLINE FUNCTIONS
//=========================================================================

// Sets the IK solving weight amount
inline void loco_ik_solver::SetWeight(float Weight)
{
    assert(Weight >= 0.0f);
    assert(Weight <= 1.0f);

    m_Weight = Weight;
}

//=========================================================================

// Retrieves the current weight
inline float loco_ik_solver::GetWeight() const
{
    return m_Weight;
}
