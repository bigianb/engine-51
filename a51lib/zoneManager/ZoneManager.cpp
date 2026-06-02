
#include "ZoneManager.h"
//#include "AudioMgr\AudioMgr.hpp"
#include "../objects/Object.h"
#include "../DataReader.h"
#include <cassert>

zone_mgr g_ZoneMgr;

#define FLOAT1_TO_U8BIT(n) ((uint8_t)((n * 255.0f) + 0.5f))
#define U8BIT_TO_FLOAT1(n) ((float)n / 255.0f)
#define MAX_FARCLIP_SCALE 10.0f
#define MIN_ZONE_VALUE 0.01f

//==============================================================================
// DEBUG
//==============================================================================

int s_ParentZone = -1;
int s_ParentPortal = -1;
//X_FILE* pFile               = nullptr;
bool s_PrintPortalWalk = false;

void zone_mgr::Reset()
{
    delete[] m_pPortal;
    delete[] m_pZone;
    delete[] m_pZone2Portal;
    delete[] m_pVisivilityBits;

    m_nPortals = 0;
    m_pPortal = nullptr;
    m_nZones = 0;
    m_pZone = nullptr;
    m_nZone2Portal = 0;
    m_pZone2Portal = nullptr;
    m_pVisivilityBits = nullptr;
    m_bAddMode = false;
    m_MaxPortals = 0;
    m_nFrustums = 0;

    m_GuidLookup.Clear();
}

zone_mgr::zone_mgr()
{
    m_nPortals = 0;
    m_pPortal = nullptr;
    m_nZones = 0;
    m_pZone = nullptr;
    m_nZone2Portal = 0;
    m_pZone2Portal = nullptr;
    m_pVisivilityBits = nullptr;
    m_bAddMode = false;
    m_nFrustums = 0;
}

zone_mgr::~zone_mgr()
{
    delete[] m_pPortal;
    delete[] m_pZone;
    delete[] m_pZone2Portal;
    delete[] m_pVisivilityBits;
}

//=========================================================================

void zone_mgr::AddStart(int nZones, int nPortals)
{
    assert(m_bAddMode == false);
    assert(m_pPortal == nullptr);
    assert(m_pZone == nullptr);

    // Allocate zones and portals
    m_nPortals = 0;
    m_MaxPortals = nPortals;
    m_pPortal = new portal[m_MaxPortals];
    //if( m_pPortal == nullptr ) x_throw( "out of memory");

    m_nZones = 256; // +1 because zone 0 actually is an empty zone
    m_pZone = new zone[m_nZones];
    //if( m_pZone == nullptr ) x_throw( "out of memory");
    memset(m_pZone, 0, sizeof(zone) * m_nZones);

    // Initialize zone zero
    m_pZone[0].bbox.Clear();
    m_pZone[0].nPortals = 0;
    m_pZone[0].iPortal2Portal = 0;

    // indicate that we are in adding mode
    m_bAddMode = true;

    // Set the capacity for the guid lookup
    m_GuidLookup.SetCapacity(nPortals, false);
}

//=========================================================================

void zone_mgr::AddPortal(guid Guid, const BBox& bbox, Vector3* pEdges, int ZoneA, int ZoneB, float SoundOcclusion)
{
    assert(m_bAddMode);
    assert(m_nPortals < m_MaxPortals);

    int     Index = m_nPortals;
    portal& Portal = m_pPortal[m_nPortals++];

    Portal.Guid = Guid;
    Portal.bbox = bbox;
    Portal.iZone[0] = ZoneA;
    Portal.iZone[1] = ZoneB;
    Portal.Edges[0] = pEdges[0];
    Portal.Edges[1] = pEdges[1];
    Portal.Edges[2] = pEdges[2];
    Portal.Edges[3] = pEdges[3];
    Portal.Flags = 0;
    Portal.Plane.Setup(pEdges[0], pEdges[1], pEdges[3]);
    Portal.Occlusion = SoundOcclusion;
    Portal.BaseOcclusion = SoundOcclusion;

    m_GuidLookup.Add(Guid, Index);
}

//=========================================================================

void zone_mgr::AddZone(const BBox& bbox,
                       int         ZoneID,
                       float       SndWeight,
                       int         MinPlayers,
                       int         MaxPlayers,
                       const char* EnvMap,
                       const char* FogName,
                       bool        QuickFog)
{
    assert(m_bAddMode);
    assert(ZoneID < m_nZones);

    zone& Zone = m_pZone[ZoneID];
    Zone.bbox = bbox;
    Zone.nPortals = 0;
    Zone.iPortal2Portal = 0;
    Zone.bStack = false;
    Zone.SndWeight = SndWeight;

    Zone.MinPlayers = MinPlayers;
    Zone.MaxPlayers = MaxPlayers;

    strncpy(Zone.EnvMapName, EnvMap, 128);
    strncpy(Zone.FogName, FogName, 128);
    Zone.QuickFog = QuickFog;

    // IJB GameMgr.SetZoneLimits( ZoneID, MinPlayers, MaxPlayers );
}

void zone_mgr::AddEnd()
{
    int i, j;

    assert(m_bAddMode);
    assert(m_nPortals == m_MaxPortals);

    //
    // First lets find how many zones to portals
    //
    m_nZone2Portal = 0;
    for (i = 0; i < m_nZones; i++) {
        zone& Zone = m_pZone[i];

        for (j = 0; j < m_nPortals; j++) {
            portal& Portal = m_pPortal[j];

            if (Portal.iZone[0] == i || Portal.iZone[1] == i) {
                m_nZone2Portal++;
                Zone.nPortals++;
            }
        }
    }

    //
    // Okay we are really to create the zone 2 portal mapping
    //

    // Allocate the zone to portal buffer
    m_pZone2Portal = new int[m_nZone2Portal];
    if (m_pZone2Portal == nullptr) {
        assert(false); //x_throw( "out of memory" );
    }

    // Set the zones indices
    int iCursor = 0;
    for (i = 0; i < m_nZones; i++) {
        zone& Zone = m_pZone[i];

        Zone.iPortal2Portal = iCursor;

        for (j = 0; j < m_nPortals; j++) {
            portal& Portal = m_pPortal[j];

            if (Portal.iZone[0] == i || Portal.iZone[1] == i) {
                m_pZone2Portal[iCursor++] = j;
                assert(iCursor <= m_nZone2Portal);
            }
        }

        assert(Zone.nPortals == (iCursor - Zone.iPortal2Portal));
    }
    assert(iCursor == m_nZone2Portal);

    //
    // TODO: Compute the zone visibility
    // Given two portals try to see if you can see portal #3 This will tell whether
    // zone a can see zone b.
    //
}

//=========================================================================

bool zone_mgr::ComputeFrustum(
    const zone&    Zone,
    frustum&       NewFrustum,
    const frustum& CurrentFrustum,
    const Vector3& EyePosition,
    const portal&  Portal) const
{
    int     i;
    int     Count[2];
    Vector3 Edges[2][MAX_PLANES];

    //
    // TODO: Make sure that all the edges that make the frustrum have a larger length of 0
    // If not it will crash.
    //

    //
    // Get ready to start the process.
    // Make sure to that the portal always represets the near clip plane.
    // so if we are in the back of the plane we must reverse the edges
    //
    if (Portal.Plane.InBack(EyePosition)) {
        Count[1] = 4;
        for (i = 0; i < 4; i++) {
            Edges[1][i] = Portal.Edges[i];
        }
    } else {
        Count[1] = 4;
        for (i = 0; i < 4; i++) {
            Edges[1][i] = Portal.Edges[3 - i];
        }
    }

    //
    // Start clipping away
    //
    for (i = 0; i < CurrentFrustum.nPlanes; i++) {
        const int iNew = i & 1;
        const int iOld = 1 - (i & 1);

        CurrentFrustum.Plane[i].ClipNGon(Edges[iNew], Count[iNew], Edges[iOld], Count[iOld]);
        assert(Count[iNew] < MAX_PLANES);

        // Did we clip all away?
        if (Count[iNew] == 0) {
            return false;
        }
    }

    //
    // Copy the final edges
    //
    const int iNew = 1 - (i & 1);

    NewFrustum.nEdges = Count[iNew];
    assert(NewFrustum.nEdges >= 3);
    for (i = 0; i < NewFrustum.nEdges; i++) {
        NewFrustum.Edges[i] = Edges[iNew][i];
    }

    //
    // Compute the new planes
    //

    // Compute all the general planes
    NewFrustum.nPlanes = 0;
    for (i = 0; i < NewFrustum.nEdges; i++) {
        const Vector3& V1 = EyePosition;
        const Vector3& V2 = Edges[iNew][i];
        const Vector3& V3 = Edges[iNew][(i + 1) % NewFrustum.nEdges];
        plane&         Plane = NewFrustum.Plane[NewFrustum.nPlanes];

        Plane.Normal = v3_Cross(V2 - V1, V3 - V1);
        if (Plane.Normal.LengthSquared() < 0.00001f) {
            continue;
        }

        Plane.Normal.Normalize();
        Plane.D = -Plane.Normal.Dot(V1);

        // Okay ready for the next
        NewFrustum.nPlanes++;
    }

    if (NewFrustum.nPlanes < 3) {
        return false;
    }

    //
    // Now create the near plane
    // TODO: This could also be done using the plane of the portal.
    //       Then we will determine whether the eye is in the front or the back
    //       (It needs to be in the back) and we will flip the plane acordenly.
    //       This will be faster but I am too lazy right now.
    //
    NewFrustum.nPlanes++;
    NewFrustum.Plane[i++].Setup(NewFrustum.Edges[0], NewFrustum.Edges[1], NewFrustum.Edges[2]);

    //
    // Also Create the far plane.
    //
    //

    // Compute the maximun distance of the far plane base on the bbox of the zone
    const float* pF = (float*)&Zone.bbox;
    float        MaxDist = m_Far.Normal.x * pF[m_FarMinIndex[0]] +
                    m_Far.Normal.y * pF[m_FarMinIndex[1]] +
                    m_Far.Normal.z * pF[m_FarMinIndex[2]] +
                    m_Far.D;

    NewFrustum.nPlanes++;
    NewFrustum.Plane[i].Normal = m_Far.Normal;
    NewFrustum.Plane[i].D = m_Far.D - MaxDist;
    i++;

    // make sure that something strange didn't happen.
    assert(NewFrustum.nPlanes <= MAX_PLANES);

    return true;
}

void zone_mgr::Search(float* pVolumes, float Volume, int ZoneID, int Depth)
{
    zone& Zone = m_pZone[ZoneID];

    // Tell the system that we already tried this one.
    Zone.bStack = true;

    // Set the volume.
    pVolumes[ZoneID] = std::max(pVolumes[ZoneID], Volume);

    // Only if we are not too deep...
    if ((Depth < 4) && (Volume > 0.01f)) {
        // Now lets go thru all its portals and see what we get
        for (int i = 0; i < Zone.nPortals; i++) {
            const int Index = m_pZone2Portal[Zone.iPortal2Portal + i];
            portal&   Portal = m_pPortal[Index];

            // Find which is going to be our new zone.
            int iNewZone = (Portal.iZone[0] == ZoneID) ? Portal.iZone[1] : Portal.iZone[0];

            // Is the next zone already in the stack?
            if (m_pZone[iNewZone].bStack) {
                continue;
            }

            // If its really quiet, dont continue.
            if (Portal.Occlusion < 0.01f) {
                continue;
            }

            // Recurse.
            Search(pVolumes, Volume * Portal.Occlusion, iNewZone, Depth + 1);
        }
    }

    // Woot, done!
    Zone.bStack = false;
}

//=========================================================================

void zone_mgr::UpdateEar(int EarID)
{
    /* IJB
    float   ZoneVolumes[MAX_ZONES];
    Matrix4 W2V;
    Vector3 Position;
    int     ZoneID;
    float   Volume;

    g_AudioMgr.GetEar(EarID, W2V, Position, ZoneID, Volume);

    if ((ZoneID >= 0) && (ZoneID < ZONELESS)) {
        // Clear them all.
        for (int i = 0; i < MAX_ZONES; i++) {
            ZoneVolumes[i] = 0.0f;
        }

        ZoneVolumes[ZONELESS] = 1.0f;
        ZoneVolumes[ZoneID] = 1.0f;

        Search(ZoneVolumes, 1.0f, ZoneID, 0);
        g_AudioMgr.UpdateEarZoneVolumes(EarID, ZoneVolumes);
    }
        */
}

void zone_mgr::PortalWalk(frustum* pFrustum, const frustum& ParentFrustum)
{
    zone& Zone = m_pZone[ParentFrustum.iZone];

    // Tell the system that we already when throw this zone
    Zone.bStack = true;

    // Now lets go throw all its portals and see what we get
    for (int i = 0; i < Zone.nPortals; i++) {
        const int Index = m_pZone2Portal[Zone.iPortal2Portal + i];
        portal&   Portal = m_pPortal[Index];
        frustum&  NewFrustum = pFrustum[m_nFrustums];

        // Check whether this portal is disable
        if ((Portal.Flags & PFLAGS_DISABLE) == PFLAGS_DISABLE) {
            continue;
        }

        //
        // +-------------------------+ If we are trying to solve a portal
        // |           Zone1         | of a zone which we have already
        // |       +------------+    | visited we can skip it. In this example
        // |       | +--------+ |    | the view starts at zone1 then goes throw
        // |       | | Zone2  | |    | Portal1 (P1) amd that takes to an unvisited
        // | View  +-+        +-+    | Zone2 but this zone has a Portal going back
        // |  *<    | P1       | P2  | to Zone1 so this if statement tells the
        // |       +-+        +-+    | system not to go down this portal.
        // |       | |        | |    |
        // +-------+ |        | +----+
        //           +--------+
        //
        if (m_pZone[Portal.iZone[0]].bStack && m_pZone[Portal.iZone[1]].bStack) {
            continue;
        }

        //
        // TODO: If zone1 has 2 portals and you can see portal2 throw portal1
        //       portal1 should became an antiportal znd acluded portal2.
        //       this requieres may be a bit too much work.
        //

        // Find which is going to be our new zone
        int iNewZone = (Portal.iZone[0] == ParentFrustum.iZone) ? Portal.iZone[1] : Portal.iZone[0];

        // Clip and compute the frustrum for the zone
        if (ComputeFrustum(m_pZone[iNewZone], NewFrustum, ParentFrustum, m_EyePosition, Portal)) {
            // OKay here is the new frustum zone and portal
            NewFrustum.iPortal = Index;
            NewFrustum.iZone = iNewZone;

            // Compute the bbox masks for this Frustum
            for (int j = 0; j < NewFrustum.nPlanes; j++) {
                GetBBoxMaxNormalMasks(NewFrustum.Plane[j], NewFrustum.NormalMasks[j * 2 + 0], NewFrustum.NormalMasks[j * 2 + 1]);
            }

            // Okay we have sucessfully build a new Frustum
            m_nFrustums++;
            assert(m_nFrustums < MAX_FRUSTUMS);

            // Okay lets walk down this new zone and see what we see.
            PortalWalk(pFrustum, NewFrustum);
        }
    }

    // OKay we pop this zone out
    Zone.bStack = false;
}

//=========================================================================
// Note that under certain conditions one zone could have multiple frustrums
// Right now we are choose not to mergethe frustrums.
//=========================================================================
void zone_mgr::PortalWalk(const view& View, int iZone)
{
    int i;

    // Nothing to do.
    if (iZone == 0) {
        m_nFrustums = 0;
        return;
    }

    assert(iZone < m_nZones);

    //
    // Clear the zone to frustrum look up
    //
    for (i = 0; i < 256; i++) {
        m_ZoneToFrustum[i] = -1;
    }

    // Save the eye position
    m_EyePosition = View.GetPosition();

    //
    // Build the main frustum
    //
    m_nFrustums = 1;
    m_Frustum[0].nPlanes = 5;
    m_Frustum[0].iZone = iZone;
    m_Frustum[0].iPortal = -1;
    plane Near;
    View.GetViewPlanes(m_Frustum[0].Plane[0],  // Top
                       m_Frustum[0].Plane[1],  // Bottom
                       m_Frustum[0].Plane[2],  // Left
                       m_Frustum[0].Plane[3],  // Right
                       Near,                   // Near
                       m_Frustum[0].Plane[4]); // Far

    // Compute the bbox indices for this Frustum
    for (i = 0; i < m_Frustum[0].nPlanes; i++) {
        GetBBoxMaxNormalMasks(m_Frustum[0].Plane[i], m_Frustum[0].NormalMasks[i * 2 + 0], m_Frustum[0].NormalMasks[i * 2 + 1]);
    }

    // Compute the max indices for the far plane
    int NearMinIndex[3];
    m_Far = m_Frustum[0].Plane[4];
    m_Far.GetBBoxIndices(m_FarMinIndex, NearMinIndex);

    // Compute the zone far plane base on the zone max bbox
    const float* pF = (float*)&m_pZone[iZone].bbox;
    float        MaxDist = m_Far.Normal.x * pF[m_FarMinIndex[0]] +
                    m_Far.Normal.y * pF[m_FarMinIndex[1]] +
                    m_Far.Normal.z * pF[m_FarMinIndex[2]] +
                    m_Far.D;

    m_Frustum[0].Plane[4].D -= MaxDist;

    //
    // Okay lets start the walk
    //
    PortalWalk(m_Frustum, m_Frustum[0]);

    //
    // Insert all the frustum into the lookup table
    //
    for (i = 0; i < m_nFrustums; i++) {
        m_Frustum[i].iNext = m_ZoneToFrustum[m_Frustum[i].iZone];
        m_ZoneToFrustum[m_Frustum[i].iZone] = (int8_t)i;
    }

    //
    // TODO: here we may choose to merge frustrums that exits in the same zone.
    //       Some case this will work better some cases it will not.
    //       I am not sure what is the way to go here.
    //
}

int zone_mgr::GetLastPortalWalkZone() const
{
    // Last zone walked is tored here
    return m_Frustum[0].iZone;
}

void zone_mgr::RenderMPZoneStates() const
{
}

zone_mgr::zone_id zone_mgr::FindZone(const Vector3& Position) const
{
    for (int i = 1; i < m_nZones; i++) {
        if (m_pZone[i].bbox.Intersect(Position)) {
            return (zone_id)i;
        }
    }

    return (zone_id)0;
}

bool zone_mgr::BBoxInView(const frustum& Frustum, const BBox& bbox) const
{
    // Collect all the data
    const plane*   pPlane = Frustum.Plane;
    const Vector3* pMask = Frustum.NormalMasks;

    // Loop through planes looking for a trivial reject.
    for (int i = 0; i < Frustum.nPlanes; i++) {
        // Compute max dist along normal
        float MaxDist = pMask[0].Dot(bbox.min) +
                        pMask[1].Dot(bbox.max) +
                        pPlane->D;

        // If outside plane, we are culled.
        if (MaxDist < 0) {
            return false;
        }

        // Move to next plane
        pMask += 2;
        pPlane++;
    }

    return true;
}

void zone_mgr::GetBBoxMaxNormalMasks(const plane& Plane, Vector3& Mask0, Vector3& Mask1) const
{
    // normally, to check if a bbox is completely behind a plane, you will find
    // the bbox's max point along the plane normal, and it's usually done like this:
    //
    // BBoxMax.X = Plane.Normal.X>=0 ? BBox.Max.X : BBox.Min.X
    // BBoxMax.Y = Plane.Normal.Y>=0 ? BBox.Max.Y : BBox.Min.Y
    // BBoxMax.Z = Plane.Normal.Z>=0 ? BBox.Max.Z : BBox.Min.Z
    // MaxDist = Plane.Normal.Dot( BBoxMax ) + Plane.D
    // if( MaxDist < 0 )
    //     BBox is behind plane
    //
    // We will try to go with an optimized route that gets rid of the comparisons
    // at run-time, and replaces them with masks, so it looks like this:
    // MaxDist = Plane.Normal DOT (BBox.Min*Mask0) + Plane.Normal DOT (BBox.Max*Mask1) + D
    //         = (Plane.Normal*Mask0) DOT BBox.Min + (Plane.Normal*Mask1) DOT BBox.Max + D
    //
    // Hopefully this will speed up the calculation time since we'll be using the vector unit
    // to be dealing with the dot products.
    //
    // So we just want to pre-calculate (Plane.Normal*Mask0) and (Plane.Normal*Mask1)
    Mask0.Zero();
    Mask1.Zero();
    if (Plane.Normal.x >= 0.0f) {
        Mask1.x = Plane.Normal.x;
    } else {
        Mask0.x = Plane.Normal.x;
    }
    if (Plane.Normal.y >= 0.0f) {
        Mask1.y = Plane.Normal.y;
    } else {
        Mask0.y = Plane.Normal.y;
    }
    if (Plane.Normal.z >= 0.0f) {
        Mask1.z = Plane.Normal.z;
    } else {
        Mask0.z = Plane.Normal.z;
    }
}

bool zone_mgr::IsBBoxVisible(const BBox& bbox, zone_id Zone1, zone_id Zone2) const
{
    // make sure that we have Frustums to check with
    if (m_nFrustums == 0) {
        return true;
    }

    // Must be in the global zone
    if (Zone1 == 0 && Zone2 == 0) {
        return true;
    }

    // Must be insize a portal
    if (Zone1 && Zone2) {
        // if neither zone is visible then you can't really see it
        if (m_ZoneToFrustum[Zone1] == -1 && m_ZoneToFrustum[Zone2] == -1) {
            return false;
        }

        // other wise you should always be able to see it
        return true;
    }

    // Check the bbox with the first zone
    if (Zone1) {
        for (int I = m_ZoneToFrustum[Zone1]; I != -1; I = m_Frustum[I].iNext) {

            if (BBoxInView(m_Frustum[I], bbox)) {
                return true;
            }
        }
    }

    // Check bbox witht he second bbox
    if (Zone2) {
        for (int I = m_ZoneToFrustum[Zone2]; I != -1; I = m_Frustum[I].iNext) {
            if (BBoxInView(m_Frustum[I], bbox)) {
                return true;
            }
        }
    }

    return false;
}

bool zone_mgr::LineCrossPortal(const portal& Portal, const Vector3& P0, const Vector3& P1) const
{
    float t;
    bool  bFront = Portal.Plane.InFront(P0);

    if (bFront == Portal.Plane.InFront(P1)) {
        return false;
    }

    if (Portal.Plane.Intersect(t, P0, P1) == false) {
        return false;
    }

    if ((t < 0) || (t > 1.0f)) {
        return false;
    }

    Vector3 Point = P0 + t * (P1 - P0);

    for (int i = 0; i < 4; i++) {
        Vector3 Normal = Portal.Plane.Normal.Cross(Portal.Edges[(i + 1) % 4] - Portal.Edges[i]);

        if (Normal.Dot(Point - Portal.Edges[i]) < -0.0001f) {
            return false;
        }
    }

    return true;
}

//=========================================================================

void zone_mgr::MoveTracker(tracker& Tracker, const Vector3& NewPosition) const
{
    if (m_nZones == 0) {
        return;
    }

    if (NewPosition == Vector3(0, 0, 0)) {
        return;
    }

    Vector3 Distance;

    Distance = NewPosition - Tracker.LastPosition;
    if (Distance.LengthSquared() < (0.1f * 0.1f)) {
        return;
    }

    assert(Tracker.iCurrentZone < m_nZones);
    zone& Zone = m_pZone[Tracker.iCurrentZone];

    float ClosestPortal = 100000000.0f;
    int   ClosestIndex = -1;
    int   i;

    BBox MoveBBox;
    MoveBBox.Clear();

    Vector3 Temp = NewPosition + Tracker.bbox.min;
    MoveBBox += Temp;

    Temp = NewPosition + Tracker.bbox.max;
    MoveBBox += Temp;

    Temp = Tracker.LastPosition + Tracker.bbox.min;
    MoveBBox += Temp;

    Temp = Tracker.LastPosition + Tracker.bbox.max;
    MoveBBox += Temp;

    //
    // Check if we are near a portal
    //
    for (i = 0; i < Zone.nPortals; i++) {
        int Index;

        Index = m_pZone2Portal[Zone.iPortal2Portal + i];
        portal& Portal = m_pPortal[Index];

        if (Portal.bbox.Intersect(MoveBBox)) {
            Vector3 ClosestPoint;
            float   Distance = NewPosition.ClosestPointToRectangle(
                Portal.Edges[0],
                Portal.Edges[1] - Portal.Edges[0],
                Portal.Edges[3] - Portal.Edges[0],
                ClosestPoint);

            if (Distance < ClosestPortal) {
                ClosestPortal = Distance;
                ClosestIndex = Index;
            }
        }
    }

    //
    // Okay we are close to a portal check whether we have penetrade the portal
    //
    if (ClosestIndex != -1) {
        portal& Portal = m_pPortal[ClosestIndex];

        if (LineCrossPortal(Portal, Tracker.LastPosition, NewPosition)) {
            // Okay we must be in the other zone.
            Tracker.iCurrentZone = (Portal.iZone[0] == Tracker.iCurrentZone) ? Portal.iZone[1] : Portal.iZone[0];
        }
    }

    //
    // Update tracker new position
    //
    Tracker.LastPosition = NewPosition;

    //
    // Update the temporart zone
    //
    if (ClosestIndex != -1) {
        portal& Portal = m_pPortal[ClosestIndex];
        Tracker.iTempZone = (Portal.iZone[0] == Tracker.iCurrentZone) ? Portal.iZone[1] : Portal.iZone[0];
    } else {
        Tracker.iTempZone = 0;
    }
}

void zone_mgr::InitZoneTracking(Object& Object, tracker& Tracker) const
{
    Tracker.SetPosition(Object.GetPosition());
    Tracker.SetMainZone((uint8_t)Object.GetZone1());
    Tracker.SetZone2((uint8_t)Object.GetZone2());
}

void zone_mgr::UpdateZoneTracking(Object& Object, tracker& Tracker, const Vector3& NewPosition) const
{
    // Get current zones from object
    Tracker.SetMainZone((uint8_t)Object.GetZone1());
    Tracker.SetZone2((uint8_t)Object.GetZone2());

    // Update tracker zone info
    MoveTracker(Tracker, NewPosition);

    // Update object zones
    Object.SetZone1(Tracker.GetMainZone());
    Object.SetZone2(Tracker.GetZone2());
}

//=========================================================================

void zone_mgr::TurnOff()
{
    m_nFrustums = 0;
}

//=========================================================================
// The save is kind of hack right now
void zone_mgr::Save(const char* pFileName)
{
    /*
    X_FILE* FP;
    FP = x_fopen(pFileName, "wb");
    if (FP == nullptr) {
        x_throw(xfs("Unable to save the zone file[%s]", pFileName));
    }

    int Version = ZONE_MANAGER_VERSION;
    x_fwrite(&Version, 1, sizeof(int), FP);
    x_fwrite(&m_nPortals, 1, sizeof(m_nPortals), FP);
    x_fwrite(&m_nZones, 1, sizeof(m_nZones), FP);
    x_fwrite(&m_nZone2Portal, 1, sizeof(m_nZone2Portal), FP);

    x_fwrite(m_pPortal, 1, m_nPortals * sizeof(portal), FP);
    x_fwrite(m_pZone, 1, m_nZones * sizeof(zone), FP);
    x_fwrite(m_pZone2Portal, 1, m_nZone2Portal * sizeof(int), FP);

    x_fclose(FP);
    */
}

static void readPlane(DataReader& reader, plane& Plane)
{
    reader.read(Plane.Normal);
    Plane.D = reader.readFloat();
    reader.skip(12); // padding to 16 byte alignment.
}

static void readPortal(DataReader& reader, zone_mgr::portal& Portal)
{
    int start = reader.cursor;
    reader.read(Portal.bbox);
    for (int i = 0; i < 4; i++) {
        reader.read(Portal.Edges[i]);
    }
    
    readPlane(reader, Portal.Plane);
    Portal.Guid = reader.readUInt64();
    Portal.iZone[0] = reader.readInt8();
    Portal.iZone[1] = reader.readInt8();
    reader.skip(2); // padding
    Portal.Flags = reader.readUInt32();
    Portal.Occlusion = reader.readFloat();
    Portal.BaseOcclusion = reader.readFloat();
    reader.skip(8); // padding to 16 byte alignment.
    int length = reader.cursor - start;
    assert(length == 160); // Each portal should be 160 bytes in size.
}

static void logPortal(const zone_mgr::portal& Portal)
{
    std::cout << "Portal: " << std::endl;
    std::cout << "  BBox: " << Portal.bbox.min.x << "," << Portal.bbox.min.y << "," << Portal.bbox.min.z
              << " - " << Portal.bbox.max.x << "," << Portal.bbox.max.y << "," << Portal.bbox.max.z
              << std::endl;
    for (int i = 0; i < 4; i++) {
        std::cout << "  Edge " << i << ": " << Portal.Edges[i].x << "," << Portal.Edges[i].y << "," << Portal.Edges[i].z
                  << std::endl;
    }
    std::cout << "  Plane: Normal(" << Portal.Plane.Normal.x << "," << Portal.Plane.Normal.y << "," << Portal.Plane.Normal.z
              << ") D: " << Portal.Plane.D
              << std::endl;
    std::cout << "  Zones: " << (int)Portal.iZone[0] << ", " << (int)Portal.iZone[1]
              << std::endl;
    std::cout << "  Flags: " << Portal.Flags
              << std::endl;
    std::cout << "  Occlusion: " << Portal.Occlusion
              << std::endl;
}

static void readZone(DataReader& reader, zone_mgr::zone& Zone)
{
    int start = reader.cursor;
    reader.read(Zone.bbox);
    Zone.Flags = reader.readUInt32();
    Zone.nPortals = reader.readInt32();
    Zone.iPortal2Portal = reader.readInt32();
    Zone.bStack = reader.readBool();
    Zone.SndWeight = reader.readFloat();
    Zone.MinPlayers = reader.readUInt16();
    Zone.MaxPlayers = reader.readUInt16();
    reader.read(Zone.EnvMapName, 128);
    reader.read(Zone.FogName, 128);
    Zone.QuickFog = reader.readBool();
    reader.skip(4); // padding to 16 byte alignment.
    int length = reader.cursor - start;
    assert(length == 320);
}

static void logZone(const zone_mgr::zone& Zone)
{
    std::cout << "Zone: " << std::endl;
    std::cout << "  BBox: " << Zone.bbox.min.x << "," << Zone.bbox.min.y << "," << Zone.bbox.min.z
              << " - " << Zone.bbox.max.x << "," << Zone.bbox.max.y << "," << Zone.bbox.max.z
              << std::endl;
    std::cout << "  Flags: " << Zone.Flags
              << std::endl;
    std::cout << "  nPortals: " << Zone.nPortals
              << std::endl;
    std::cout << "  iPortal2Portal: " << Zone.iPortal2Portal
              << std::endl;
    std::cout << "  SndWeight: " << Zone.SndWeight
              << std::endl;
    std::cout << "  MinPlayers: " << Zone.MinPlayers
              << std::endl;
    std::cout << "  MaxPlayers: " << Zone.MaxPlayers
              << std::endl;
    std::cout << "  EnvMapName: " << Zone.EnvMapName
              << std::endl;
    std::cout << "  FogName: " << Zone.FogName
              << std::endl;
    std::cout << "  QuickFog: " << (Zone.QuickFog ? "true" : "false")
              << std::endl;
}

//=========================================================================
// The load is kind of hack right now
void zone_mgr::Load(const uint8_t* pData, int dataLength)
{
    Reset();
    DataReader reader = DataReader(pData, dataLength);
    int        version = reader.readInt32();

    std::cout << "Zone manager version: " << version << std::endl;
    
    m_nPortals = reader.readInt32();
    m_nZones = reader.readInt32();
    m_nZone2Portal = reader.readInt32();

    std::cout << "Number of portals: " << m_nPortals << std::endl;
    std::cout << "Number of zones: " << m_nZones << std::endl;
    std::cout << "Number of zone to portal mappings: " << m_nZone2Portal << std::endl;

    m_pPortal = new portal[m_nPortals];
    m_pZone = new zone[m_nZones];
    m_pZone2Portal = new int[m_nZone2Portal];

    for (int i = 0; i < m_nPortals; i++) {
        readPortal(reader, m_pPortal[i]);
    }

    for (int i = 0; i < m_nZones; i++) {
        readZone(reader, m_pZone[i]);
    }

    for (int i = 0; i < m_nZone2Portal; i++) {
        m_pZone2Portal[i] = reader.readInt32();
    }

    /*

    // make sure to add the guid lookups
    m_GuidLookup.SetCapacity(m_nPortals, false);
    for (int i = 0; i < m_nPortals; i++) {
        m_GuidLookup.Add(m_pPortal[i].Guid, i);
    }

    // Set up the zone limits in GameMgr.
    GameMgr.SetZoneLimits(0, 0, 32);
    for (int i = 1; i < m_nZones; i++) {
        GameMgr.SetZoneLimits(i, m_pZone[i].MinPlayers, m_pZone[i].MaxPlayers);
    }
*/
}

zone_mgr::portal& zone_mgr::GetPortal(guid Guid)
{
    int Index;

    assert(m_pPortal);

    if (m_GuidLookup.Find(Guid, Index)) {
        assert(Index < m_nPortals);
        return m_pPortal[Index];
    }

    assert(0);
    //x_throw("Unable to get portal");
    return m_pPortal[0];
}

zone_mgr::portal& zone_mgr::GetPortal(int ZoneID, int PortalIndex)
{
    assert(ZoneID >= 0);
    assert(ZoneID < 256);
    assert(PortalIndex < m_pZone[ZoneID].nPortals);
    assert(PortalIndex >= 0);

    const zone& Zone = m_pZone[ZoneID];
    int         iPortal = m_pZone2Portal[Zone.iPortal2Portal + PortalIndex];

    return m_pPortal[iPortal];
}

const zone_mgr::zone& zone_mgr::GetZone(int ZoneID)
{
    assert(ZoneID >= 0);
    assert(ZoneID < 256);

    return m_pZone[ZoneID];
}

void zone_mgr::TurnPortalOff(guid Guid)
{
    int Index;

    if (m_nPortals == 0) {
        return;
    }

    assert(m_pPortal);

    if (m_GuidLookup.Find(Guid, Index)) {
        m_pPortal[Index].Flags |= PFLAGS_DISABLE;
    }
}

void zone_mgr::TurnPortalOn(guid Guid)
{
    int Index;

    if (m_nPortals == 0) {
        return;
    }

    assert(m_pPortal);

    if (m_GuidLookup.Find(Guid, Index)) {
        m_pPortal[Index].Flags &= ~PFLAGS_DISABLE;
    }
}

bool zone_mgr::IsPortalOn(guid Guid)
{
    int Index;

    if (m_nPortals == 0) {
        return false;
    }

    assert(m_pPortal);

    if (m_GuidLookup.Find(Guid, Index)) {
        if (!(m_pPortal[Index].Flags & PFLAGS_DISABLE)) {
            return true;
        }
    }

    return false;
}

void zone_mgr::SetPortalOcclusion(guid Guid, float Occlusion)
{
    int Index;

    if (m_nPortals == 0) {
        return;
    }

    assert(m_pPortal);

    if (m_GuidLookup.Find(Guid, Index)) {
        m_pPortal[Index].Occlusion = Occlusion;
    }
}

bool zone_mgr::IsAdjacentZone(int Zone1, int Zone2)
{
    if (m_nZones > Zone1) {
        for (int i = 0; i < m_pZone[Zone1].nPortals; i++) {
            //for each portal
            int index = m_pZone[Zone1].iPortal2Portal + i;

            if ((m_nZone2Portal > index) && (m_nPortals > m_pZone2Portal[index])) {
                if (m_pPortal[m_pZone2Portal[index]].iZone[0] == Zone2 ||
                    m_pPortal[m_pZone2Portal[index]].iZone[1] == Zone2) {
                    return true;
                }
            }
        }
    }

    return false;
}

void zone_mgr::SanityCheck()
{
}

zone_mgr::tracker::tracker()
{
    LastPosition.Zero();
    iTempZone = iCurrentZone = 0;
    bbox.Set(Vector3(0, 0, 0), 100);
}

