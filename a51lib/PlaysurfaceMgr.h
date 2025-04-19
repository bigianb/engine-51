#pragma once

#include <cstdint>
#include <sstream>
#include "render/Geom.h"
#include "render/CollisionVolume.h"
#include "PlaysurfaceDB.h"
#include "objects/render/RenderInst.h"

struct Surface
{
    Matrix4            L2W;
    BBox               WorldBBox;
    uint32_t           AttrBits;
    int                ColorOffset;
    render::hgeom_inst RenderInst;
    union
    {
        uint32_t* pColor32;
        uint16_t* pColor16;
        void*     pColor;
    };
    uint32_t DBaseQuery;
    int      NextSurface;

    uint16_t ZoneInfo;
    int16_t  GeomNameIndex;
    int      RenderFlags;
};

class PlaysurfaceMgr
{
public:
    bool readFile(uint8_t* fileData, int len);
    void describe(std::ostringstream& ss);

    struct ZoneInfo
    {
        int fileOffset;
        int numSurfaces;
        int numColors;

        std::vector<Surface>  surfaces;
        std::vector<uint32_t> colours;
    };

    //---------------------------------------------------------------------
    // Collection routines for looking up surfaces (most likely for
    // collision purposes)
    //---------------------------------------------------------------------
    void     CollectSurfaces(const BBox& bb,
                             uint32_t    Attributes,
                             uint32_t    NotTheseAttributes);
    void     CollectSurfaces(const Vector3& RayStart,
                             const Vector3& RayEnd,
                             uint32_t       Attributes,
                             uint32_t       NotTheseAttributes);
    Surface* GetNextSurface();

    //---------------------------------------------------------------------
    // Helpers to fake out the collision system
    //---------------------------------------------------------------------
    guid GetPlaySurfaceGuid() { return spatialDBase.GetCurrentGuid(); }

    // Header
    int version;
    int numZones;
    int numPortals;
    int numGeoms;

    PlaysurfaceDB            spatialDBase;
    std::vector<std::string> geoms; // names

    std::vector<ZoneInfo> zones;
    std::vector<ZoneInfo> portals;

    int m_QueryNumber;

    // for debugging
    int endPortalOffset;
    int fileLen;

private:
    void ClearDBaseQueries();
    void readZoneInfo(DataReader& reader, ZoneInfo& zi);
};
