#pragma once

#include <cstdint>
#include <sstream>
#include "Geom.h"
#include "CollisionVolume.h"

#include "PlaysurfaceDB.h"

class Playsurface
{
public:
    bool readFile(uint8_t* fileData, int len);
    void describe(std::ostringstream& ss);

    // Header
    int version;
    int numZones;
    int numPortals;
    int numGeoms;

    PlaysurfaceDB            spatialDBase;
    std::vector<std::string> geoms; // names

    struct Surface
    {
        Matrix4  L2W;
        BBox     WorldBBox;
        uint32_t AttrBits;
        int      ColorOffset;

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

    struct ZoneInfo
    {
        int fileOffset;
        int numSurfaces;
        int numColors;

        std::vector<Surface> surfaces;
        std::vector<uint32_t> colours;
    };

    std::vector<ZoneInfo> zones;
    std::vector<ZoneInfo> portals;

    // for debugging
    int endPortalOffset;
    int fileLen;

private:
    void readZoneInfo(DataReader& reader, ZoneInfo& zi);
};
