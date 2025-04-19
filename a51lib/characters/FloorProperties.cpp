
#include "FloorProperties.h"
#include "../objectManager/ObjectManager.h"
#include "../collisionMgr/CollisionMgr.h"

floor_properties::floor_properties()
{
    m_StartColor.Zero();
    m_EndColor.Zero();
    m_CurrentColor = Colour(64, 64, 64, 255);
    m_ColorFadeTime = 0;
    m_RadiusSquared = -1;
    m_LastPosition.Zero();
}

floor_properties::~floor_properties()
{
}

void floor_properties::Init(float Radius, float ColorFadeTime)
{
    m_RadiusSquared = Radius * Radius;
    m_ColorFadeTime = ColorFadeTime;
}

void floor_properties::Update(const Vector3& NewPosition, float DeltaTime, bool bIgnorePosition)
{
    // Check if we've been initialized
    assert(m_RadiusSquared >= 0);

    //
    // Check if object has moved beyond radius
    //
    Vector3 HorizDelta = NewPosition - m_LastPosition;
    HorizDelta.y = 0;

    if ((HorizDelta.LengthSquared() > m_RadiusSquared) && (!bIgnorePosition)) {
        // Backup the new position
        m_LastPosition = NewPosition;

        Colour   NewFloorColor;
        uint32_t NewFloorMat = 0;
        if (GrabFloorProperties(NewPosition, NewFloorColor, NewFloorMat)) {
            // Backup current color into old color
            m_StartColor.set(m_CurrentColor.r, m_CurrentColor.g, m_CurrentColor.b);

            // Store new floor material
            m_FloorMat = NewFloorMat;

            // Store new floor color
            m_EndColor.set(NewFloorColor.r, NewFloorColor.g, NewFloorColor.b);

            // Reset color fade timer
            m_ColorFadeT = 0;
        }
    }

    //
    // Has the color timer run completely out?
    //
    if (m_ColorFadeT != m_ColorFadeTime) {
        // Advance the color timer
        m_ColorFadeT += DeltaTime;
        if (m_ColorFadeT > m_ColorFadeTime) {
            m_ColorFadeT = m_ColorFadeTime;
        }

        // Compute the interpolated color
        float   T = m_ColorFadeT / m_ColorFadeTime;
        Vector3 C = m_StartColor + T * (m_EndColor - m_StartColor);

        // Build the new Colour
        m_CurrentColor.r = (uint8_t)C.x;
        m_CurrentColor.g = (uint8_t)C.y;
        m_CurrentColor.b = (uint8_t)C.z;
        m_CurrentColor.a = (uint8_t)255;
    }
}

//==============================================================================

void floor_properties::ForceUpdate(const Vector3& NewPosition)
{
    Colour   NewFloorColor;
    uint32_t NewFloorMat = 0;

    // Backup the new position
    m_LastPosition = NewPosition;

    // Try to grab new floor properties. If we can't find any we'll stick with
    // what we've got.
    if (GrabFloorProperties(NewPosition, NewFloorColor, NewFloorMat)) {
        // Backup current color into old color
        m_StartColor.x = NewFloorColor.r;
        m_StartColor.y = NewFloorColor.g;
        m_StartColor.z = NewFloorColor.b;

        // Store new floor material
        m_FloorMat = NewFloorMat;

        // Store new floor color
        m_EndColor.x = NewFloorColor.r;
        m_EndColor.y = NewFloorColor.g;
        m_EndColor.z = NewFloorColor.b;

        m_CurrentColor.r = NewFloorColor.r;
        m_CurrentColor.g = NewFloorColor.g;
        m_CurrentColor.b = NewFloorColor.b;
        m_CurrentColor.a = (uint8_t)255;
    }

    m_ColorFadeT = m_ColorFadeTime;
}

//==============================================================================

bool floor_properties::GrabFloorProperties(const Vector3& ObjectPosition, Colour& FloorColor, uint32_t& FloorMat)
{
    // Cast a ray down and collect the collisions
    Vector3 Start(ObjectPosition);
    Vector3 End(Start);
    Start.y += 10.0f; // sometimes the root can be below the ground, so nudge it up a bit
    End.y -= 100.0f;  // give it a meters to hit the ground

    g_CollisionMgr.RaySetup(0, Start, End);
    g_CollisionMgr.CheckCollisions(Object::TYPE_ALL_TYPES, Object::ATTR_COLLIDABLE, Object::ATTR_LIVING);

    // from the collision info, save out the poly color and material
    for (int iCol = 0; iCol < g_CollisionMgr.m_nCollisions; iCol++) {
        Object::detail_tri Tri;

        // if we can't get a primitive key, we can't get a color
        int PrimitiveKey = g_CollisionMgr.m_Collisions[iCol].PrimitiveKey;
        if (PrimitiveKey == -1) {
            continue;
        }

        // grab the collision info
        guid    HitGuid = g_CollisionMgr.m_Collisions[iCol].ObjectHitGuid;
        Object* pObj = nullptr; // IJB g_ObjMgr.GetObjectByGuid(HitGuid);
        if (pObj == nullptr) {
            continue;
        }

        if (pObj->GetColDetails(PrimitiveKey, Tri) == false) {
            continue;
        }

        // the color is a weighted average of the triangle colors
        // use barycentric coords to achieve that
        const Vector3& P0 = Tri.Vertex[0];
        const Vector3& P1 = Tri.Vertex[1];
        const Vector3& P2 = Tri.Vertex[2];
        const Vector3& TP = g_CollisionMgr.m_Collisions[iCol].Point;

        // Compute scaled normal
        Vector3 Normal = v3_Cross(P1 - P0, P2 - P0);
        Normal *= 1.0f / Normal.LengthSquared();

        // Compute barycentric co-ords
        Vector3 Bary(v3_Cross(P2 - P1, TP - P1).Dot(Normal),
                     v3_Cross(P0 - P2, TP - P2).Dot(Normal),
                     v3_Cross(P1 - P0, TP - P0).Dot(Normal));

        // now we can get the color
        const Colour& C0 = Tri.Color[0];
        const Colour& C1 = Tri.Color[1];
        const Colour& C2 = Tri.Color[2];
        Vector3       vFloorCol(Bary.x * C0.r + Bary.y * C1.r + Bary.z * C2.r,
                                Bary.x * C0.g + Bary.y * C1.g + Bary.z * C2.g,
                                Bary.x * C0.b + Bary.y * C1.b + Bary.z * C2.b);
        vFloorCol.Min(255.0f);
        vFloorCol.Max(0.0f);

        // woohoo...done with the hard stuff
        FloorColor.set((uint8_t)vFloorCol.x, (uint8_t)vFloorCol.y, (uint8_t)vFloorCol.z);
        FloorMat = g_CollisionMgr.m_Collisions[iCol].Flags;
        return true;
    }

    return false;
}
