#include "Playsurface.h"
#include "DataReader.h"

bool Playsurface::readFile(uint8_t* fileData, int len)
{
    bool okay = true;

    DataReader reader = DataReader(fileData, len);
    version = reader.readInt32();
    numZones = reader.readInt32();
    numPortals = reader.readInt32();
    numGeoms = reader.readInt32();

    // Spacial DB is next
    spatialDBase.read(reader);

    // The geoms
    for (int i = 0; i < numGeoms; ++i) {
        std::string gname = reader.readString();
        reader.skip(128);
        geoms.push_back(gname);
    }

    // zone info
    for (int i = 0; i < numZones; ++i) {
        ZoneInfo zi;
        reader.skip(4); // resolved
        zi.fileOffset = reader.readInt32();
        zi.numSurfaces = reader.readInt32();
        reader.skip(4);
        zi.numColors = reader.readInt32();
        reader.skip(8);
        zones.push_back(zi);
    }

    // portals
    for (int i = 0; i < numPortals; ++i) {
        ZoneInfo zi;
        reader.skip(4); // resolved
        zi.fileOffset = reader.readInt32();
        zi.numSurfaces = reader.readInt32();
        reader.skip(4);
        zi.numColors = reader.readInt32();
        reader.skip(8);
        portals.push_back(zi);
    }

    endPortalOffset = reader.cursor;
    fileLen = reader.len;

    return okay;
}
void Playsurface::describe(std::ostringstream& ss)
{
    ss << "Version:" << version << std::endl;
    ss << "Num Zones:" << numZones << std::endl;
    ss << "Num Geoms:" << numGeoms << std::endl;
    ss << "Num Portals:" << numPortals << std::endl;
    ss << "End portal file offset: " << endPortalOffset << std::endl;
    ss << "File length: " << fileLen;
    ss << std::endl;

    ss << "Geoms: " << std::endl;
    for (auto& gname : geoms) {
        ss << "    " << gname << std::endl;
    }
    ss << std::endl;
    ss << "Zones: " << std::endl;
    for (auto& zi : zones) {
        ss << "    File Offset: " << zi.fileOffset << std::endl;
        ss << "    Num Surfaces: " << zi.numSurfaces << std::endl;
        ss << "    Num Colours: " << zi.numColors << std::endl;
        ss << std::endl;
    }
    ss << std::endl;
    ss << "Portals: " << std::endl;
    for (auto& zi : portals) {
        ss << "    File Offset: " << zi.fileOffset << std::endl;
        ss << "    Num Surfaces: " << zi.numSurfaces << std::endl;
        ss << "    Num Colours: " << zi.numColors << std::endl;
        ss << std::endl;
    }
    spatialDBase.describe(ss);
}
