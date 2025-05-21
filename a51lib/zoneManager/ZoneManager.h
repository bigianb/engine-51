#pragma once

#include "../VectorMath.h"
#include "../Guid.h"
#include "../view/View.h"
#include <cassert>

#define MAX_ZONE_DISTANCE_TRAVELED 65535.0f
#define ZONE_MANAGER_VERSION 1003

class Object;

class zone_mgr
{
public:
    //---------------------------------------------------------------------
    enum portal_flags
    {
        PFLAGS_DISABLE = (1 << 0),
    };

    //---------------------------------------------------------------------
    enum zone_flags
    {
        ZFLAGS_VISITED = (1 << 0),
    };

    //---------------------------------------------------------------------
    enum max
    {
        MAX_PLANES = 16
    };

    //---------------------------------------------------------------------
    typedef uint8_t zone_id;

    //---------------------------------------------------------------------

    struct zone
    {
        BBox     bbox; // BBox containing the hold zone
        uint32_t Flags;
        int      nPortals;       // How many portals does this zone has
        int      iPortal2Portal; // start index to the array of indices to portals
        bool     bStack;         // Flags we have push this zone already in the stack
        float    SndWeight;      // Sound multiplier per zone
        uint16_t MinPlayers;
        uint16_t MaxPlayers;
        char     EnvMapName[128];
        char     FogName[128];
        bool     QuickFog; // fog transition is immediate
    };

    //---------------------------------------------------------------------
    struct portal
    {
        BBox     bbox;     // BBox
        Vector3  Edges[4]; // A portal is always a square.
        plane    Plane;    // Plane of the actual portal.
        guid     Guid;
        zone_id  iZone[2];      // The two zones that this portal connects
        uint32_t Flags;         // Flags about the portal.
        float    Occlusion;     // The amount of sound this protal is occluding.
        float    BaseOcclusion; // The fixed amount of sound this protal is occluding, not changed in runtime.
    };

    //---------------------------------------------------------------------
    struct tracker
    {
    public:
        tracker();

        zone_id     GetMainZone() const { return iCurrentZone; }
        zone_id     GetZone2() const { return iTempZone; }
        void        SetMainZone(uint8_t Zone) { iCurrentZone = Zone; }
        void        SetZone2(uint8_t Zone) { iTempZone = Zone; }
        void        SetPosition(const Vector3& Pos) { LastPosition = Pos; }
        void        SetBBox(const BBox& aBBox) { bbox = aBBox; }
        const BBox& GetBBox() { return bbox; }

    protected:
        Vector3 LastPosition; // Last Know position.
        zone_id iCurrentZone; // Current Zone that the tracker is in
        zone_id iTempZone;    // Temporary zone which the object may also be in
        BBox    bbox;         // Local Space BBox

        friend class zone_mgr;
    };

    //---------------------------------------------------------------------
    struct node
    {
        int   iParentZone;     // Parent zone.
        int   iParentPortal;   // Parent portal.
        int   iZone;           // Current zone.
        int   iPortal;         // Current portal.
        float SndWeight;       // Sound weight accumalated so far.
        float Distance;        // Distance accumalated this far.
        int   ZonePortalIndex; // Zone to portal index.
    };

public:
    zone_mgr();
    ~zone_mgr();

    void TurnOff();
    void Reset();
    void AddStart(int nZones, int nPortals);
    void AddZone(const BBox& BBox,
                 int         ZoneID,
                 float       SndWeight,
                 int         MinPlayers,
                 int         MaxPlayers,
                 const char* EnvMap,
                 const char* FogName,
                 bool        QuickFog);
    void AddPortal(guid Guid, const BBox& BBox, Vector3* pEdges, int ZoneA, int ZoneB,
                   float SoundOcclusion);
    void AddEnd();

    void Save(const char* pFileName);
    void Load(const char* pFileName);
    void UpdateEar(int EarID);
    void Search(float* pVolumes, float Volume, int ZoneID, int Depth);
    void PortalWalk(const view& View, int iZone);
    int  GetLastPortalWalkZone() const;

    void    Render() const;
    void    RenderMPZoneStates() const;
    zone_id FindZone(const Vector3& Position) const;
    void    GetBBoxMaxNormalMasks(const plane& Plane, Vector3& Mask0, Vector3& Mask1) const;
    bool    IsBBoxVisible(const BBox& BBox, zone_id Zone1, zone_id Zone2) const;
    bool    IsZoneVisible(zone_id iZone) const;

private:
    void MoveTracker(tracker& Tracker, const Vector3& NewPosition) const;

public:
    void InitZoneTracking(Object& Object, tracker& Tracker) const;
    void UpdateZoneTracking(Object& Object, tracker& Tracker, const Vector3& NewPosition) const;

    int         GetPortalCount() const;
    int         GetZoneCount() const;
    portal&     GetPortal(int PortalID);
    portal&     GetPortal(guid Guid);
    portal&     GetPortal(int ZoneID, int PortalIndex);
    const zone& GetZone(int ZoneID);
    void        TurnPortalOff(guid Guid);
    void        TurnPortalOn(guid Guid);
    bool        IsPortalOn(guid Guid);
    void        SetPortalOcclusion(guid Guid, float Occlusion);
    const char* GetZoneEnvMap(int ZoneID);
    const char* GetZoneFog(int ZoneID, bool& QuickFog);

    bool IsAdjacentZone(int Zone1, int Zone2);

    void SanityCheck();

    int GetZoneCount() { return m_nZones; }
    int GetStartingZone() { return m_Frustum[0].iZone; }

protected:
    //---------------------------------------------------------------------
    enum
    {
        MAX_FRUSTUMS = 32
    };

    //---------------------------------------------------------------------
    struct frustum
    {
        plane   Plane[MAX_PLANES];           // array of planes
        Vector3 NormalMasks[MAX_PLANES * 2]; // masks for doing optimizing vis calculations
        Vector3 Edges[MAX_PLANES];           // Array of the edges
        int     nPlanes;                     // Number of planes
        int     nEdges;                      // Number of edges ones all has been cliped. This is more for debuging
        zone_id iZone;                       // Which zone does this frustum belongs
        char    iNext;                       // Link list of frustum that have the same zone
        int     iPortal;                     // The portal that caused this frustum to be clipped and created
    };

protected:
    bool ComputeFrustum(const zone&    Zone,
                        frustum&       NewFrustum,
                        const frustum& CurrentFrustum,
                        const Vector3& EyePosition,
                        const portal&  Portal) const;
    void PortalWalk(frustum* pFrustum, const frustum& ParentFrustum);
    bool BBoxInView(const frustum& Frustum, const BBox& BBox) const;
    bool LineCrossPortal(const portal& Portal, const Vector3& P0, const Vector3& P1) const;

protected:
    // Data-Base variables
    int     m_nPortals;
    portal* m_pPortal;

    int   m_nZones;
    zone* m_pZone;

    int  m_nZone2Portal;
    int* m_pZone2Portal;

    // Add mode variables
    bool m_bAddMode;
    int  m_MaxPortals;

    // Portal Walk variables
    int     m_nFrustums;
    frustum m_Frustum[MAX_FRUSTUMS];
    Vector3 m_EyePosition;
    plane   m_Far;
    int     m_FarMinIndex[3];

    // Quick Frustum look up
    char m_ZoneToFrustum[256];

    // Dome to quicly look up portals
    GuidLookup m_GuidLookup;

    //
    // This is an array of bits nZones by nZones big. It tells whether a
    // zone is visible from another zone. This could be RLE compress if need be ala Quake
    //
    uint8_t* m_pVisivilityBits;
};

//=========================================================================

inline int zone_mgr::GetPortalCount() const
{
    return m_nPortals;
}

//=========================================================================

inline zone_mgr::portal& zone_mgr::GetPortal(int PortalID)
{
    assert((PortalID >= 0) && (PortalID < m_nPortals));
    return m_pPortal[PortalID];
}

//=========================================================================

inline const char* zone_mgr::GetZoneEnvMap(int ZoneID)
{
    assert(ZoneID >= 0);
    assert(ZoneID < 256);

    if (ZoneID < m_nZones) {
        return m_pZone[ZoneID].EnvMapName;
    } else {
        return "";
    }
}

//=========================================================================

inline const char* zone_mgr::GetZoneFog(int ZoneID, bool& QuickFog)
{
    assert(ZoneID >= 0);
    assert(ZoneID < 256);

    if (ZoneID < m_nZones) {
        QuickFog = m_pZone[ZoneID].QuickFog;
        return m_pZone[ZoneID].FogName;
    } else {
        QuickFog = false;
        return "";
    }
}

//=========================================================================

inline bool zone_mgr::IsZoneVisible(zone_id iZone) const
{
    if (m_nFrustums == 0) {
        return true;
    } else {
        return (m_ZoneToFrustum[iZone] != -1);
    }
}

//=========================================================================

inline int zone_mgr::GetZoneCount() const
{
    return m_nZones;
}

//=========================================================================

extern zone_mgr g_ZoneMgr;
