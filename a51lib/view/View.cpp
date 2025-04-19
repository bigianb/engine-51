
#include "View.h"

#include <cstdlib>

#define DIRTY_W2V (1 << 0)
#define DIRTY_V2W (1 << 1)

#define DIRTY_V2C (1 << 2)
#define DIRTY_C2S (1 << 3)
#define DIRTY_V2S (1 << 4)
#define DIRTY_W2C (1 << 5)
#define DIRTY_W2S (1 << 6)
#define DIRTY_YFOV (1 << 7)

#define DIRTY_PLANES (1 << 8)
#define DIRTY_EDGES (1 << 9)
#define DIRTY_PROJECTION (1 << 10)
#define DIRTY_SCREENDIST (1 << 11)

#define DIRTY_ALL 0xFFFFFFFF
#define DIRTY_MATRICES (DIRTY_W2V | \
                        DIRTY_V2W | \
                        DIRTY_V2C | \
                        DIRTY_C2S | \
                        DIRTY_V2S | \
                        DIRTY_W2C | \
                        DIRTY_W2S)

//==============================================================================
//  FUNCTIONS
//==============================================================================

view::view()
{
    m_WorldPos.set(0.0f, 0.0f, 0.0f);
    m_WorldOrient.Identity();
    m_ViewportX0 = 50;  // Default viewport is 640x480, but
    m_ViewportY0 = 50;  // pulled in 50 pixels on all sides.
    m_ViewportX1 = 589; //   639 - 50 = 589
    m_ViewportY1 = 429; //   479 - 50 = 249
    m_XFOV = R_60;
    m_PixelScale = DEFAULT_PIXEL_SCALE;
    m_ZNear = 0.1f;
    m_ZFar = 1000.0f;

    m_Dirty = DIRTY_ALL;

    m_nScissors = 0;
    m_iScissor = 0;
}


view::view(const view& View)
{
    m_WorldPos = View.m_WorldPos;
    m_WorldOrient = View.m_WorldOrient;
    m_ViewportX0 = View.m_ViewportX0;
    m_ViewportY0 = View.m_ViewportY0;
    m_ViewportX1 = View.m_ViewportX1;
    m_ViewportY1 = View.m_ViewportY1;
    m_XFOV = View.m_XFOV;
    m_PixelScale = View.m_PixelScale;
    m_ZNear = View.m_ZNear;
    m_ZFar = View.m_ZFar;

    m_Dirty = DIRTY_ALL;
}

//==============================================================================

view::~view()
{
}

//==============================================================================

void view::SetViewport(int X0, int Y0, int X1, int Y1)
{
    m_ViewportX0 = X0;
    m_ViewportY0 = Y0;
    m_ViewportX1 = X1;
    m_ViewportY1 = Y1;

    m_Dirty |= DIRTY_YFOV;
    m_Dirty |= DIRTY_PLANES;
    m_Dirty |= DIRTY_EDGES;
    m_Dirty |= DIRTY_PROJECTION;
    m_Dirty |= DIRTY_SCREENDIST;
}

//==============================================================================

void view::SetXFOV(Radian XFOV)
{
    m_XFOV = XFOV;

    m_Dirty |= DIRTY_MATRICES;
    m_Dirty |= DIRTY_YFOV;
    m_Dirty |= DIRTY_PLANES;
    m_Dirty |= DIRTY_EDGES;
    m_Dirty |= DIRTY_PROJECTION;
    m_Dirty |= DIRTY_SCREENDIST;
}

//==============================================================================

void view::SetYFOV(Radian YFOV)
{
    // We need a distance where the viewport pixel units are the same as our
    // world units.  Since we know the size of the viewport vertically (in
    // pixels) and we were given the vertical angle, we can find this distance.
    // Note that we want only half of the viewport height and half of the angle
    // to give us a right triangle for the trig.

    float HalfVPHeight = (m_ViewportY1 - m_ViewportY0 + 1) * 0.5f;
    float HalfVPWidth = (m_ViewportX1 - m_ViewportX0 + 1) * 0.5f / m_PixelScale;

    m_ScaledScreenDistY = HalfVPHeight / tan(YFOV * 0.5f);

    // Given this distance, and knowing the size of the viewport laterally
    // (again, in pixels), we can find the laterial angle.  Note, the trig uses
    // with half the viewport size to find half the anlge.

    m_XFOV = atan(HalfVPWidth / m_ScaledScreenDistY) * 2.0f;
    m_YFOV = YFOV;

    m_Dirty &= ~DIRTY_YFOV;
    m_Dirty |= DIRTY_SCREENDIST;
    m_Dirty |= DIRTY_PLANES;
    m_Dirty |= DIRTY_EDGES;
    m_Dirty |= DIRTY_MATRICES;
    m_Dirty |= DIRTY_PROJECTION;
}

//==============================================================================

void view::SetPixelScale(float PixelScale)
{
    m_PixelScale = PixelScale;

    m_Dirty |= DIRTY_MATRICES;
    m_Dirty |= DIRTY_YFOV;
    m_Dirty |= DIRTY_PLANES;
    m_Dirty |= DIRTY_EDGES;
    m_Dirty |= DIRTY_PROJECTION;
    m_Dirty |= DIRTY_SCREENDIST;
}

//==============================================================================

void view::SetZLimits(float ZNear, float ZFar)
{
    m_ZNear = ZNear;
    m_ZFar = ZFar;

    m_Dirty = DIRTY_ALL;
}

//==============================================================================

void view::SetPosition(const Vector3& Position)
{
    m_WorldPos = Position;
    m_Dirty = DIRTY_ALL;
}

//==============================================================================

void view::SetRotation(const Radian3& Rotation)
{
    m_WorldOrient.Setup(Rotation);
    m_Dirty = DIRTY_ALL;
}

//==============================================================================

void view::SetRotation(const Quaternion& Quat)
{
    m_WorldOrient.Setup(Quat);
    m_Dirty = DIRTY_ALL;
}

//==============================================================================

void view::SetV2W(const Matrix4& V2W)
{
    m_WorldOrient = V2W;
    m_WorldOrient.ClearTranslation();

    m_WorldPos = V2W.GetTranslation();

    m_Dirty = DIRTY_ALL;
}

//==============================================================================

void view::Translate(const Vector3& Translation, system System)
{
    switch (System) {
    case WORLD:
        m_WorldPos += Translation;
        break;
    case VIEW:
        m_WorldPos += (m_WorldOrient * Translation);
        break;
    default:
        assert(false);
    }

    m_Dirty = DIRTY_ALL;
}

//==============================================================================

void view::RotateX(Radian Angle, system System)
{
    switch (System) {
    case WORLD:
        m_WorldOrient.RotateX(Angle);
        break;
    case VIEW:
        m_WorldOrient.PreRotateX(Angle);
        break;
    default:
        assert(false);
    }

    m_Dirty = DIRTY_ALL;
}

//==============================================================================

void view::RotateY(Radian Angle, system System)
{
    switch (System) {
    case WORLD:
        m_WorldOrient.RotateY(Angle);
        break;
    case VIEW:
        m_WorldOrient.PreRotateY(Angle);
        break;
    default:
        assert(false);
    }

    m_Dirty = DIRTY_ALL;
}

//==============================================================================

void view::RotateZ(Radian Angle, system System)
{
    switch (System) {
    case WORLD:
        m_WorldOrient.RotateZ(Angle);
        break;
    case VIEW:
        m_WorldOrient.PreRotateZ(Angle);
        break;
    default:
        assert(false);
    }

    m_Dirty = DIRTY_ALL;
}

//==============================================================================

void view::GetViewport(int& X0, int& Y0, int& X1, int& Y1) const
{
    X0 = m_ViewportX0;
    Y0 = m_ViewportY0;
    X1 = m_ViewportX1;
    Y1 = m_ViewportY1;
}

//==============================================================================

void view::GetViewport(rect& Rect) const
{
    Rect.Min.x = (float)m_ViewportX0;
    Rect.Min.y = (float)m_ViewportY0;
    Rect.Max.x = (float)m_ViewportX1;
    Rect.Max.y = (float)m_ViewportY1;
}

//==============================================================================

void view::GetPixel(float ParamX, float ParamY, int& X, int& Y) const
{
    X = (int)(m_ViewportX0 + ParamX * (m_ViewportX1 - m_ViewportX0));
    Y = (int)(m_ViewportY0 + ParamY * (m_ViewportY1 - m_ViewportY0));
}

//==============================================================================

void view::GetZLimits(float& ZNear, float& ZFar) const
{
    ZNear = m_ZNear;
    ZFar = m_ZFar;
}

//==============================================================================

void view::GetPitchYaw(Radian& Pitch, Radian& Yaw) const
{
    // We want the view's Z axis (line of sight) in World space.  We can easily
    // pull that unit vector from the internal orientation matrix which converts
    // View oriented space to World oriented space.

    Vector3 LOS(m_WorldOrient(2, 0),
                m_WorldOrient(2, 1),
                m_WorldOrient(2, 2));

    LOS.GetPitchYaw(Pitch, Yaw);
}

//==============================================================================

Radian view::GetXFOV(void) const
{
    return (m_XFOV);
}

//==============================================================================

Radian view::GetYFOV(void) const
{
    UpdateYFOV();
    return (m_YFOV);
}

//==============================================================================

float view::GetPixelScale(void) const
{
    return m_PixelScale;
}

//==============================================================================

Vector3 view::GetPosition(void) const
{
    return (m_WorldPos);
}

//==============================================================================

const Matrix4& view::GetW2V(void) const
{
    UpdateW2V();
    return (m_W2V);
}

//==============================================================================

const Matrix4& view::GetV2W(void) const
{
    UpdateV2W();
    return (m_V2W);
}

//==============================================================================

const Matrix4& view::GetV2C(void) const
{
    UpdateV2C();
    return (m_V2C);
}

//==============================================================================

const Matrix4& view::GetC2S(void) const
{
    UpdateC2S();
    return (m_C2S);
}

//==============================================================================

const Matrix4& view::GetV2S(void) const
{
    UpdateV2S();
    return (m_V2S);
}

//==============================================================================

const Matrix4& view::GetW2C(void) const
{
    UpdateW2C();
    return (m_W2C);
}

//==============================================================================

void view::GetV2C(int ClipSize, Matrix4& V2C) const
{

    float Ratio = (float)(m_ViewportY1 - m_ViewportY0) / (float)(m_ViewportX1 - m_ViewportX0);
    float ClipX = (float)ClipSize;
    float ClipY = (float)ClipSize * Ratio;

    UpdateScreenDist();
    float DX = m_ScaledScreenDistX;
    float DY = m_ScaledScreenDistY;
    float X = atan(ClipX / DX);
    float Y = atan(ClipY / DY);
    float W = (float)(1.0f / tan(X));
    float H = (float)(1.0f / tan(Y));

    V2C.Zero();

    V2C(0, 0) = -W;
    V2C(1, 1) = H;
    V2C(2, 2) = (m_ZNear + m_ZFar) / (m_ZFar - m_ZNear);
    V2C(3, 2) = (-2.0f * m_ZNear * m_ZFar) / (m_ZFar - m_ZNear);
    V2C(2, 3) = 1.0f;
}

//==============================================================================

void view::GetW2C(int ClipSize, Matrix4& W2C) const
{
    UpdateW2C();

    Matrix4 V2C;
    GetV2C(ClipSize, V2C);

    W2C = V2C * m_W2V;
}

//==============================================================================

void view::GetC2W(int ClipSize, Matrix4& C2W) const
{
    Matrix4 V2C;

    GetV2C(ClipSize, V2C);
    V2C.Invert();

    C2W = GetV2W() * V2C;
}

//==============================================================================

void view::GetC2W(Matrix4& C2W) const
{
    Matrix4 C2V(GetV2C());
    C2V.Invert();

    C2W = GetV2W() * C2V;
}

//==============================================================================

void view::GetC2S(int ClipSize, Matrix4& C2S) const
{
    float Ratio = (float)(m_ViewportY1 - m_ViewportY0) / (float)(m_ViewportX1 - m_ViewportX0);
    float ClipX = (float)ClipSize;
    float ClipY = (float)ClipSize * Ratio;

    float W = ClipX;
    float H = ClipY;

    float SW = (m_ViewportX1 - m_ViewportX0) * 0.5f;
    float ZScale = (float)((int)1 << 19);

    C2S.Zero();
    C2S(0, 0) = W;
    C2S(1, 1) = -H;
    C2S(2, 2) = -ZScale / 2.0f;
    C2S(3, 3) = 1.0f;
    C2S(3, 0) = W + m_ViewportX0 + (2048.0f - ClipX);
    C2S(3, 1) = H + m_ViewportY0 + (2048.0f - ClipY) - ((1.0f - Ratio) * SW);
    C2S(3, 2) = ZScale / 2.0f;
}

//==============================================================================

const Matrix4& view::GetW2S(void) const
{
    UpdateW2S();
    return (m_W2S);
}

//==============================================================================

Vector3 view::GetViewX(void) const
{
    return (Vector3(m_WorldOrient(0, 0),
                    m_WorldOrient(0, 1),
                    m_WorldOrient(0, 2)));
}

//==============================================================================

Vector3 view::GetViewY(void) const
{
    return (Vector3(m_WorldOrient(1, 0),
                    m_WorldOrient(1, 1),
                    m_WorldOrient(1, 2)));
}

//==============================================================================

Vector3 view::GetViewZ(void) const
{
    return (Vector3(m_WorldOrient(2, 0),
                    m_WorldOrient(2, 1),
                    m_WorldOrient(2, 2)));
}

//==============================================================================

void view::LookAtPoint(const Vector3& FromPoint,
                       const Vector3& ToPoint,
                       system         System)
{
    Vector3 WorldFrom;

    // We need the "Target" point to be in World space.

    switch (System) {
    case WORLD:
        WorldFrom = FromPoint;
        break;
    case VIEW:
        WorldFrom = ConvertV2W(FromPoint);
        break;
    default:
        assert(false);
    }

    // Now, look at the Target.

    SetPosition(WorldFrom);
    LookAtPoint(ToPoint, System);

    m_Dirty = DIRTY_ALL;
}

//==============================================================================

void view::LookAtPoint(const Vector3& Point, system System)
{
    Vector3 Target;

    // We need the "Target" point to be in World space.

    switch (System) {
    case WORLD:
        Target = Point;
        break;
    case VIEW:
        Target = ConvertV2W(Point);
        break;
    default:
        assert(false);
    }

    // Now, look at the Target.

    Target -= m_WorldPos;

    m_WorldOrient.Identity();
    m_WorldOrient.RotateX(Target.GetPitch());
    m_WorldOrient.RotateY(Target.GetYaw());

    m_Dirty = DIRTY_ALL;
}

//==============================================================================

void view::OrbitPoint(const Vector3& Point,
                      float          Distance,
                      Radian         Pitch,
                      Radian         Yaw)
{
    m_WorldPos.set(0, 0, Distance);
    m_WorldPos.RotateX(Pitch);
    m_WorldPos.RotateY(Yaw);
    m_WorldPos += Point;
    LookAtPoint(Point);
}

//==============================================================================

Vector3 view::ConvertW2V(const Vector3& Point) const
{
    UpdateW2V();
    return (m_W2V * Point);
}

//==============================================================================

Vector3 view::ConvertV2W(const Vector3& Point) const
{
    UpdateV2W();
    return (m_V2W * Point);
}

//==============================================================================

const plane* view::GetViewPlanes(system System) const
{
    if (m_Dirty & DIRTY_PLANES) {
        UpdatePlanes();
    }
    return ((System == WORLD) ? (m_WorldSpacePlane) : (m_ViewSpacePlane));
}

//==============================================================================

const int* view::GetViewPlaneMinBBoxIndices(system System) const
{
    if (m_Dirty & DIRTY_PLANES) {
        UpdatePlanes();
    }
    return ((System == WORLD) ? (m_WorldPlaneMinIndex) : (m_ViewPlaneMinIndex));
}

//==============================================================================

const int* view::GetViewPlaneMaxBBoxIndices(system System) const
{
    if (m_Dirty & DIRTY_PLANES) {
        UpdatePlanes();
    }
    return ((System == WORLD) ? (m_WorldPlaneMaxIndex) : (m_ViewPlaneMaxIndex));
}

//==============================================================================

void view::GetMinMaxZ(const BBox& BBox, float& MinZ, float& MaxZ) const
{
    float* pF = ((float*)&BBox);

    // Be sure planes have been constructed.
    if (m_Dirty & DIRTY_PLANES) {
        UpdatePlanes();
    }

    // Compute min and max dist along normal
    MinZ = m_ZPlane.Normal.GetX() * pF[m_ZPlaneMinI[0]] +
           m_ZPlane.Normal.GetY() * pF[m_ZPlaneMinI[1]] +
           m_ZPlane.Normal.GetZ() * pF[m_ZPlaneMinI[2]] +
           m_ZPlane.D;

    MaxZ = m_ZPlane.Normal.GetX() * pF[m_ZPlaneMaxI[0]] +
           m_ZPlane.Normal.GetY() * pF[m_ZPlaneMaxI[1]] +
           m_ZPlane.Normal.GetZ() * pF[m_ZPlaneMaxI[2]] +
           m_ZPlane.D;
}

//==============================================================================

void view::GetViewPlanes(plane& Top,
                         plane& Bottom,
                         plane& Left,
                         plane& Right,
                         system System) const
{
    plane* pPlane;

    UpdatePlanes();

    pPlane = (System == WORLD) ? (m_WorldSpacePlane) : (m_ViewSpacePlane);

    Left = pPlane[0];
    Right = pPlane[1];
    Bottom = pPlane[2];
    Top = pPlane[3];
}

//==============================================================================

void view::GetViewPlanes(plane& Top,
                         plane& Bottom,
                         plane& Left,
                         plane& Right,
                         plane& Near,
                         plane& Far,
                         system System) const
{
    plane* pPlane;

    UpdatePlanes();

    pPlane = (System == WORLD) ? (m_WorldSpacePlane) : (m_ViewSpacePlane);

    Left = pPlane[0];
    Right = pPlane[1];
    Bottom = pPlane[2];
    Top = pPlane[3];
    Near = pPlane[4];
    Far = pPlane[5];
}

//==============================================================================

void view::GetViewPlanes(float X0, float Y0, float X1, float Y1,
                         plane& Top,
                         plane& Bottom,
                         plane& Left,
                         plane& Right,
                         plane& Near,
                         plane& Far,
                         system System) const
{
    // Imagine sitting at the origin and looking down Z+.  Build the points on
    // the frustum and transform into correct system.  Then build the planes
    // the points.

    // Compute distance to screen in camera space.
    UpdateScreenDist();

    // Get sub shot frustrum
    float L, R, T, B;

    {
        L = X0;
        T = Y0;
        R = X1;
        B = Y1;
    }

    // Flip signs to go from screen coordinates to view space.
    L = -L;
    R = -R;
    T = -T;
    B = -B;

    // Build camera space coordinates.
    float   YScale = m_ScaledScreenDistX / m_ScaledScreenDistY;
    Vector3 PTL = Vector3(L, T * YScale, m_ScaledScreenDistX);
    Vector3 PTR = Vector3(R, T * YScale, m_ScaledScreenDistX);
    Vector3 PBL = Vector3(L, B * YScale, m_ScaledScreenDistX);
    Vector3 PBR = Vector3(R, B * YScale, m_ScaledScreenDistX);
    Vector3 PEYE = Vector3(0, 0, 0);
    Vector3 PNR = Vector3(0, 0, m_ZNear);
    Vector3 PFR = Vector3(0, 0, m_ZFar);
    Vector3 PUP = Vector3(0, 1, 0);
    Vector3 PLFT = Vector3(1, 0, 0);

    // Transform into correct system.
    if (System == WORLD) {
        PTL = ConvertV2W(PTL);
        PTR = ConvertV2W(PTR);
        PBL = ConvertV2W(PBL);
        PBR = ConvertV2W(PBR);
        PEYE = ConvertV2W(PEYE);
        PNR = ConvertV2W(PNR);
        PFR = ConvertV2W(PFR);
        PUP = ConvertV2W(PUP);
        PLFT = ConvertV2W(PLFT);
    }

    // Construct side planes.
    Top.Setup(PEYE, PTL, PTR);
    Bottom.Setup(PEYE, PBR, PBL);
    Left.Setup(PEYE, PBL, PTL);
    Right.Setup(PEYE, PTR, PBR);

    // Construct LOS normal for near and far.
    Near.Setup(PEYE, PLFT, PUP);
    Near.D = -Near.Dot(PNR);
    Far = Near;
    Far.Negate();
    Far.D = -Far.Dot(PFR);
}

//==============================================================================

bool view::PointInView(const Vector3& Point,
                       system         System) const
{
    // Be sure planes have been constructed.
    if (m_Dirty & DIRTY_PLANES) {
        UpdatePlanes();
    }

    // Decide which planes to use.
    plane* pPlane = (System == WORLD) ? (m_WorldSpacePlane) : (m_ViewSpacePlane);

    // Loop through planes looking for a trivial reject.
    for (int i = 0; i < 6; i++) {
        float Dist = pPlane[i].Distance(Point);

        // If completely plane, we are culled.
        if (Dist < 0) {
            return (false);
        }
    }

    return (true);
}

//==============================================================================

int view::SphereInView(const Vector3& Center,
                       float          Radius,
                       system         System) const
{
    int ReturnValue = 1;

    // Be sure planes have been constructed.
    if (m_Dirty & DIRTY_PLANES) {
        UpdatePlanes();
    }

    // Decide which planes to use.
    plane* pPlane = (System == WORLD) ? (m_WorldSpacePlane) : (m_ViewSpacePlane);

    // Loop through planes looking for a trivial reject.
    for (int i = 0; i < 6; i++) {
        float Dist = pPlane[i].Distance(Center);

        // If completely plane, we are culled.
        if (Dist < -Radius) {
            return 0;
        }

        // If partially out, remember.
        if (Dist < Radius) {
            ReturnValue = 2;
        }
    }

    return (ReturnValue);
}

//==============================================================================

int view::BBoxInView(const BBox& BBox,
                     uint32_t&   CheckPlaneMask,
                     system      System) const
{
    int ReturnValue = VISIBLE_FULL; // Means "Fully In View".
    CheckPlaneMask = 0;

    // Be sure planes have been constructed.
    if (m_Dirty & DIRTY_PLANES) {
        UpdatePlanes();
    }

    // Decide which planes to use.
    plane* pPlane = (System == WORLD) ? (m_WorldSpacePlane) : (m_ViewSpacePlane);
    int*   pMinI = (System == WORLD) ? (m_WorldPlaneMinIndex) : (m_ViewPlaneMinIndex);
    int*   pMaxI = (System == WORLD) ? (m_WorldPlaneMaxIndex) : (m_ViewPlaneMaxIndex);
    float* pF = (float*)&BBox;

    // Loop through planes looking for a trivial reject.
    for (int i = 0; i < 6; i++) {
        // Compute max dist along normal
        float MaxDist = pPlane->Normal.GetX() * pF[pMaxI[0]] +
                        pPlane->Normal.GetY() * pF[pMaxI[1]] +
                        pPlane->Normal.GetZ() * pF[pMaxI[2]] +
                        pPlane->D;

        // If outside plane, we are culled.
        if (MaxDist < 0) {
            CheckPlaneMask = 0;
            return (VISIBLE_NONE);
        }

        // Compute min dist along normal
        float MinDist = pPlane->Normal.GetX() * pF[pMinI[0]] +
                        pPlane->Normal.GetY() * pF[pMinI[1]] +
                        pPlane->Normal.GetZ() * pF[pMinI[2]] +
                        pPlane->D;

        // If partially out, remember.
        if (MinDist < 0) {
            ReturnValue = VISIBLE_PARTIAL;
            CheckPlaneMask |= (1 << i);
        }

        // Move to next plane
        pMinI += 3;
        pMaxI += 3;
        pPlane++;
    }

    return (ReturnValue);
}

//==============================================================================

int view::BBoxInView(const BBox& BBox, system System) const
{
    uint32_t CheckPlaneMask;
    return BBoxInView(BBox, CheckPlaneMask, System);
}

//==============================================================================

bool view::SphereInCone(const Vector3& Center,
                        float          Radius) const
{
    float Radius2 = Radius * Radius;

    // Be sure planes have been constructed.
    if (m_Dirty & DIRTY_PLANES) {
        UpdatePlanes();
    }

    // Get delta from eye to center of sphere
    Vector3 Delta = Center - m_WorldPos;
    float   DeltaLen2 = Delta.Dot(Delta);
    float   ZDist = m_ConeAxis.Dot(Delta);

    // Check if eye is contained in sphere
    if (DeltaLen2 < Radius2) {
        return true;
    }

    // Check if sphere is behind camera
    if ((ZDist < m_ZNear) || (ZDist > m_ZFar)) {
        return false;
    }

    // Get dist from axis to center and radius of cone
    float ConeRadius = m_ConeSlope * ZDist;
    float PerpDist2 = DeltaLen2 - (ZDist * ZDist);

    // Check if sphere is trivially rejected
    if (PerpDist2 > (ConeRadius + Radius) * (ConeRadius + Radius)) {
        return false;
    }

    return true;
}

//==============================================================================

bool view::SphereInConeAngle(const Vector3& Center,
                             float          Radius,
                             float          tanAngle) const
{
    float Radius2 = Radius * Radius;

    // Be sure planes have been constructed.
    if (m_Dirty & DIRTY_PLANES) {
        UpdatePlanes();
    }

    // Get delta from eye to center of sphere
    Vector3 Delta = Center - m_WorldPos;
    float   DeltaLen2 = Delta.Dot(Delta);
    float   ZDist = m_ConeAxis.Dot(Delta);

    // Check if eye is contained in sphere
    if (DeltaLen2 < Radius2) {
        return true;
    }

    // Check if sphere is behind camera
    if ((ZDist < m_ZNear) || (ZDist > m_ZFar)) {
        return false;
    }

    // Get dist from axis to center and radius of cone
    float ConeRadius = tanAngle * ZDist;
    float PerpDist2 = DeltaLen2 - (ZDist * ZDist);

    // Check if sphere is trivially rejected
    if (PerpDist2 > (ConeRadius + Radius) * (ConeRadius + Radius)) {
        return false;
    }

    return true;
}

//==============================================================================

void view::GetProjection(float& XP0, float& XP1, float& YP0, float& YP1) const
{
    UpdateProjection();
    XP0 = m_ProjectX[0];
    XP1 = m_ProjectX[1];
    YP0 = m_ProjectY[0];
    YP1 = m_ProjectY[1];
}

//==============================================================================

Vector3 view::PointToScreen(const Vector3& Point,
                            system         System) const
{
    // Move point to view space.
    Vector3 P = Point;
    if (System == WORLD) {
        P = ConvertW2V(P);
    }

    // Be sure projection is updated.
    UpdateProjection();

    // Handle strange/dangerous Z's.
    float ProjZ = P.GetZ();
    if (ProjZ < 0.001f) {
        if (ProjZ > -0.001f) {
            ProjZ = 0.001f;
        }
        if (ProjZ < 0.000f) {
            ProjZ = -ProjZ;
        }
    }

    // Project to screen.
    return (Vector3(m_ProjectX[0] + m_ProjectX[1] * (P.GetX() / ProjZ),
                    m_ProjectY[0] + m_ProjectY[1] * (P.GetY() / ProjZ),
                    P.GetZ()));
}

//==============================================================================

Vector3 view::RayFromScreen(float  ScreenX,
                            float  ScreenY,
                            system System) const
{
    if (m_Dirty & DIRTY_SCREENDIST) {
        UpdateScreenDist();
    }

    // Build ray in viewspace.
    Vector3 Ray(-(ScreenX - (m_ViewportX0 + m_ViewportX1) * 0.5f),
                -(ScreenY - (m_ViewportY0 + m_ViewportY1) * 0.5f) * m_PixelScale,
                m_ScaledScreenDistX);

    // Transform into correct space.
    if (System == WORLD) {
        Ray = ConvertV2W(Ray);
        Ray -= m_WorldPos;
    }

    Ray.Normalize();
    return (Ray);
}

//==============================================================================

float view::CalcScreenSize(const Vector3& Position,
                           float          WorldRadius,
                           system         System /* = WORLD */) const
{
    float ZDist;

    if (m_Dirty & DIRTY_SCREENDIST) {
        UpdateScreenDist();
    }

    // Get view distance
    if (System == WORLD) {
        Vector3 Diff = Position - m_WorldPos;
        ZDist = m_WorldOrient(2, 0) * Diff.GetX() +
                m_WorldOrient(2, 1) * Diff.GetY() +
                m_WorldOrient(2, 2) * Diff.GetZ();
    } else {
        ZDist = Position.GetZ();
    }

    // Cull everything behind camera
    if (ZDist < -WorldRadius) {
        return 0;
    }

    if (ZDist < WorldRadius) {
        ZDist = WorldRadius;
    }

    // Return projected size
    float ScreenDist = std::min(m_ScaledScreenDistX, m_ScaledScreenDistY);

    return (WorldRadius * 2.0f * ScreenDist) / ZDist;
}

//==============================================================================

void view::UpdateW2V(void) const
{
    if (m_Dirty & DIRTY_W2V) {
        m_W2V = m_WorldOrient;
        m_W2V.Transpose();
        m_W2V.PreTranslate(-m_WorldPos);
        m_Dirty &= ~DIRTY_W2V;
    }
}

//==============================================================================

void view::UpdateV2W(void) const
{
    if (m_Dirty & DIRTY_V2W) {
        m_V2W = m_WorldOrient;
        m_V2W.SetTranslation(m_WorldPos);
        m_Dirty &= ~DIRTY_V2W;
    }
}

//==============================================================================

void view::UpdateV2C(void) const
{
    if (m_Dirty & DIRTY_V2C) {
        m_Dirty &= ~DIRTY_V2C;
        memset(&m_V2C, 0, sizeof(Matrix4));

        UpdateYFOV();

        float W = (float)(1.0f / tan(m_XFOV * 0.5f));
        float H = (float)(1.0f / tan(m_YFOV * 0.5f));
        float Q = m_ZFar / (m_ZFar - m_ZNear);
        m_V2C(0, 0) = -W;
        m_V2C(1, 1) = H;
        m_V2C(2, 2) = Q;
        m_V2C(3, 2) = -Q * m_ZNear;
        m_V2C(2, 3) = 1;
    }
}

//==============================================================================

void view::UpdateC2S(void) const
{
    if (m_Dirty & DIRTY_C2S) {
        m_Dirty &= ~DIRTY_C2S;

        memset(&m_C2S, 0, sizeof(Matrix4));

        float W = (m_ViewportX1 - m_ViewportX0 + 1) * 0.5f;
        float H = (m_ViewportY1 - m_ViewportY0 + 1) * 0.5f;

        m_C2S(0, 0) = W;
        m_C2S(1, 1) = -H;
        m_C2S(2, 2) = 1;
        m_C2S(3, 3) = 1;
        m_C2S(3, 0) = W + m_ViewportX0;
        m_C2S(3, 1) = H + m_ViewportY0;
    }
}

//==============================================================================

void view::UpdateV2S() const
{
    if (m_Dirty & DIRTY_V2S) {
        m_Dirty &= ~DIRTY_V2S;

        UpdateV2C();
        UpdateC2S();

        m_V2S = m_C2S * m_V2C;
    }
}

//==============================================================================

void view::UpdateW2C() const
{
    if (m_Dirty & DIRTY_W2C) {
        m_Dirty &= ~DIRTY_W2C;

        UpdateW2V();
        UpdateV2C();

        m_W2C = m_V2C * m_W2V;
    }
}

//==============================================================================

void view::UpdateW2S() const
{
    if (m_Dirty & DIRTY_W2S) {
        m_Dirty &= ~DIRTY_W2S;

        UpdateW2C();
        UpdateC2S();

        m_W2S = m_C2S * m_W2C;
    }
}

//==============================================================================

void view::UpdateYFOV() const
{
    if (m_Dirty & DIRTY_YFOV) {
        // See the comments in the function SetYFOV().

        float Distance;
        float HalfVPHeight = m_PixelScale * (m_ViewportY1 - m_ViewportY0 + 1) * 0.5f;
        float HalfVPWidth = (m_ViewportX1 - m_ViewportX0 + 1) * 0.5f;

        Distance = HalfVPWidth / tan(m_XFOV * 0.5f);
        m_YFOV = atan(HalfVPHeight / Distance) * 2.0f;
        m_Dirty &= ~DIRTY_YFOV;
        m_Dirty |= DIRTY_V2C;
    }
}

//==============================================================================

void view::UpdatePlanes() const
{
    if (m_Dirty & DIRTY_PLANES) {
        m_Dirty &= ~DIRTY_PLANES;

        // Get full frustrum
        float W = (m_ViewportX1 - m_ViewportX0 + 1) * 0.5f;
        float H = (m_ViewportY1 - m_ViewportY0 + 1) * 0.5f;

        GetViewPlanes(-W, -H, W, H,
                      m_WorldSpacePlane[3],
                      m_WorldSpacePlane[2],
                      m_WorldSpacePlane[0],
                      m_WorldSpacePlane[1],
                      m_WorldSpacePlane[4],
                      m_WorldSpacePlane[5],
                      WORLD);

        GetViewPlanes(-W, -H, W, H,
                      m_ViewSpacePlane[3],
                      m_ViewSpacePlane[2],
                      m_ViewSpacePlane[0],
                      m_ViewSpacePlane[1],
                      m_ViewSpacePlane[4],
                      m_ViewSpacePlane[5],
                      VIEW);

        for (int i = 0; i < 6; i++) {
            m_WorldSpacePlane[i].GetBBoxIndices(&m_WorldPlaneMinIndex[i * 3], &m_WorldPlaneMaxIndex[i * 3]);
            m_ViewSpacePlane[i].GetBBoxIndices(&m_ViewPlaneMinIndex[i * 3], &m_ViewPlaneMaxIndex[i * 3]);
        }

        // Build ZPlane and BBox indices
        m_ZPlane = m_WorldSpacePlane[4];
        m_ZPlane.ComputeD(m_WorldPos);
        m_ZPlane.GetBBoxIndices(m_ZPlaneMinI, m_ZPlaneMaxI);

        // Build cone values
        m_ConeAxis = GetViewZ();

        float   ScreenDist = std::min(m_ScaledScreenDistX, m_ScaledScreenDistY);
        Vector3 Ray(-(m_ViewportX0 - ((m_ViewportX0 + m_ViewportX1) * 0.5f)),
                    -(m_ViewportY0 - ((m_ViewportY0 + m_ViewportY1) * 0.5f)),
                    0.0f);
        Ray = Ray * Vector3(m_ZFar / ScreenDist, m_ZFar / ScreenDist, 1.0f);
        m_ConeSlope = Ray.Length() / m_ZFar;
    }
}

//==============================================================================

void view::UpdateEdges(void) const
{
}

//==============================================================================

float view::GetScreenDist(void) const
{
    if (m_Dirty & DIRTY_SCREENDIST) {
        UpdateScreenDist();
    }

    return m_ScaledScreenDistX; // Take shot into account for correct PS2 MipK
}

//==============================================================================

void view::UpdateScreenDist(void) const
{
    if (m_Dirty & DIRTY_SCREENDIST) {
        m_Dirty &= ~DIRTY_SCREENDIST;

        // Compute dist using the FOV's. Note that the YFOV has been adjusted to
        // take into account the pixel scale, so the scaled screen distances will
        // not be the same.
        UpdateYFOV();
        float HalfVPWidth = (m_ViewportX1 - m_ViewportX0 + 1) * 0.5f;
        m_ScaledScreenDistX = HalfVPWidth / tan(m_XFOV * 0.5f);
        float HalfVPHeight = (m_ViewportY1 - m_ViewportY0 + 1) * 0.5f;
        m_ScaledScreenDistY = HalfVPHeight / tan(m_YFOV * 0.5f);
    }
}

//==============================================================================

void view::UpdateProjection(void) const
{
    if (m_Dirty & DIRTY_PROJECTION) {
        m_Dirty &= ~DIRTY_PROJECTION;

        // Compute centers of viewport.
        m_ProjectX[0] = (m_ViewportX0 + m_ViewportX1) * 0.5f;
        m_ProjectY[0] = (m_ViewportY0 + m_ViewportY1) * 0.5f;

        // Setup 'focal length'.
        UpdateScreenDist();
        UpdateYFOV();

        m_ProjectX[1] = -m_ScaledScreenDistX;
        m_ProjectY[1] = -m_ScaledScreenDistY;
    }
}
