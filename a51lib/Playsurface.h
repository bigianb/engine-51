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

    PlaysurfaceDB spatialDBase;
    std::vector<std::string> geoms;     // names

    struct ZoneInfo
    {
        int fileOffset;
        int numSurfaces;
        int numColors;
    };
    
    std::vector<ZoneInfo> zones;
    std::vector<ZoneInfo> portals;

    // for debugging
    int endPortalOffset;
    int fileLen;

};

