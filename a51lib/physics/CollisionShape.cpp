
#include "CollisionShape.h"
//#include "PhysicsMgr.h"

void collision_shape::AddSphere(const Vector3& Offset)
{
    sphere Sphere;
    Sphere.m_Offset = Offset;
    Sphere.m_CollFreePos.Zero();
    Sphere.m_PrevPos.Zero();
    Sphere.m_CurrPos.Zero();

    m_Spheres.push_back(Sphere);
}

//==============================================================================
// BBox functions
//==============================================================================

BBox collision_shape::ComputeLocalBBox() const
{
    // Compute local BBox
    BBox LocalBBox;
    LocalBBox.Clear();

    // Add all spheres to bounds
    for (int i = 0; i < m_Spheres.size(); i++) {
        // Get sphere
        const sphere& Sphere = m_Spheres[i];

        // Add local sphere center to local BBox
        LocalBBox += Sphere.m_Offset;
    }

    // Take sphere radius and float error into account
    float Inflate = m_Radius + 1.0f;
    LocalBBox.Inflate(Inflate, Inflate, Inflate);

    return LocalBBox;
}

//==============================================================================

BBox collision_shape::ComputeWorldBBox() const
{
    // Clear world BBox
    BBox WorldBBox;
    WorldBBox.Clear();

    // Add all spheres to bounds
    float Radius = m_Radius + 1.0f;
    for (int i = 0; i < m_Spheres.size(); i++) {
        // Lookup sphere info
        const sphere& Sphere = m_Spheres[i];

        // Compute movement BBox
        BBox MoveBBox(Sphere.m_PrevPos, Sphere.m_CurrPos);
        MoveBBox.Inflate(Radius, Radius, Radius);

        // Compute movement sphere bounds
        BBox MoveSphereBBox(MoveBBox.GetCenter(), MoveBBox.GetRadius());

        // Compute collision BBox
        BBox CollBBox(Sphere.m_CollFreePos, Radius);

        // Update world bounds
        WorldBBox += MoveSphereBBox;
        WorldBBox += CollBBox;
    }

    // For float error
    WorldBBox.Inflate(1.0f, 1.0f, 1.0f);

    return WorldBBox;
}

//==============================================================================

void collision_shape::SetL2W(const Matrix4& L2W)
{
    // Put spheres and collision pos into world space
    for (int i = 0; i < m_Spheres.size(); i++) {
        // Look up sphere
        sphere& Sphere = m_Spheres[i];

        // Compute world position
        Vector3 WorldPos = L2W * Sphere.m_Offset;
        Sphere.m_CollFreePos = WorldPos;
        Sphere.m_PrevPos = WorldPos;
        Sphere.m_CurrPos = WorldPos;
    }
}

//==============================================================================

void collision_shape::SetL2W(const Matrix4& PrevL2W, const Matrix4& NextL2W)
{
    assert(PrevL2W.IsValid());
    assert(NextL2W.IsValid());

    // Put spheres into world space
    for (int i = 0; i < m_Spheres.size(); i++) {
        // Look up sphere
        sphere& Sphere = m_Spheres[i];

        // Compute world positions
        Sphere.m_PrevPos = PrevL2W * Sphere.m_Offset;
        Sphere.m_CurrPos = NextL2W * Sphere.m_Offset;
    }
}
