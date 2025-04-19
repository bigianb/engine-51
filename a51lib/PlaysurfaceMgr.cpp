#include "PlaysurfaceMgr.h"
#include "DataReader.h"
#include "streamingOperators.h"

void PlaysurfaceMgr::readZoneInfo(DataReader& reader, ZoneInfo& zi)
{
    reader.skip(4); // resolved
    zi.fileOffset = reader.readInt32();
    zi.numSurfaces = reader.readInt32();
    reader.skip(4);
    zi.numColors = reader.readInt32();
    reader.skip(8);

    const int cursorToRestore = reader.cursor;

    if (zi.numSurfaces > 0) {
        reader.cursor = zi.fileOffset;
        for (int i = 0; i < zi.numSurfaces; ++i) {
            Surface surface;
            reader.read(surface.L2W);
            reader.read(surface.WorldBBox);
            surface.AttrBits = reader.readUInt32();
            surface.ColorOffset = reader.readInt32();
            reader.skip(4); // unused handle
            reader.skip(4); // color pointer
            surface.DBaseQuery = reader.readUInt32();
            surface.NextSurface = reader.readInt32();
            surface.ZoneInfo = reader.readUInt16();
            surface.GeomNameIndex = reader.readInt16();
            surface.RenderFlags = reader.readInt32();
            zi.surfaces.push_back(surface);
        }

        for (int i = 0; i < zi.numColors; ++i) {
            // TODO: Xbox is 32 bit
            zi.colours.push_back(reader.readUInt16());
        }
    }

    reader.cursor = cursorToRestore;
}

bool PlaysurfaceMgr::readFile(uint8_t* fileData, int len)
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
        readZoneInfo(reader, zi);
        zones.push_back(zi);
    }

    // portals
    for (int i = 0; i < numPortals; ++i) {
        ZoneInfo zi;
        readZoneInfo(reader, zi);
        portals.push_back(zi);
    }

    endPortalOffset = reader.cursor;
    fileLen = reader.len;

    return okay;
}
void PlaysurfaceMgr::describe(std::ostringstream& ss)
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
        if (zi.numSurfaces == 0) {
            continue;
        }
        ss << "    File Offset: " << zi.fileOffset << std::endl;
        ss << "    Num Surfaces: " << zi.numSurfaces << std::endl;
        ss << "    Num Colours: " << zi.numColors << std::endl;
        for (auto& surface : zi.surfaces) {
            ss << "        L2W: " << surface.L2W << std::endl;
            ss << "        World BBox: " << surface.WorldBBox << std::endl;
            ss << "        Geom Name: " << geoms.at(surface.GeomNameIndex) << std::endl;
            ss << "        Zone Info: " << (surface.ZoneInfo >> 8) << ", " << (surface.ZoneInfo & 0xFF) << std::endl;
            ss << std::endl;
        }
        ss << std::endl;
    }
    ss << std::endl;
    ss << "Portals: " << std::endl;
    for (auto& zi : portals) {
        if (zi.numSurfaces == 0) {
            continue;
        }
        ss << "    File Offset: " << zi.fileOffset << std::endl;
        ss << "    Num Surfaces: " << zi.numSurfaces << std::endl;
        ss << "    Num Colours: " << zi.numColors << std::endl;
        for (auto& surface : zi.surfaces) {
            ss << "        L2W: " << surface.L2W << std::endl;
            ss << "        World BBox: " << surface.WorldBBox << std::endl;
            ss << "        Geom Name: " << geoms.at(surface.GeomNameIndex) << std::endl;
            ss << "        Zone Info: " << (surface.ZoneInfo >> 8) << ", " << (surface.ZoneInfo & 0xFF) << std::endl;
            ss << std::endl;
        }
        ss << std::endl;
    }
    spatialDBase.describe(ss);
}

void PlaysurfaceMgr::ClearDBaseQueries()
{
    int i, j;
    for (i = 0; i < zones.size(); i++) {
        ZoneInfo& zi = zones[i];
        for (j = 0; j < zi.numSurfaces; j++) {
            zi.surfaces[j].DBaseQuery = 0;
        }
    }

    for (i = 0; i < portals.size(); i++) {
        ZoneInfo& zi = portals[i];
        for (j = 0; j < zi.numSurfaces; j++) {
            zi.surfaces[j].DBaseQuery = 0;
        }
    }
}

void PlaysurfaceMgr::CollectSurfaces(const BBox& bb,
                                     uint32_t    Attributes,
                                     uint32_t    NotTheseAttributes)
{
    if (zones.size() == 0) {
        m_QueryNumber = 0;
        return;
    }

    m_QueryNumber++;
    if (m_QueryNumber == 0) {
        // zero is considered a special query, and will force us to reset
        // all of the id's so that we are 100% correct when this number
        // wraps around
        ClearDBaseQueries();
        m_QueryNumber++;
    }

    spatialDBase.CollectSurfaces(bb, m_QueryNumber, Attributes, NotTheseAttributes);
}

void PlaysurfaceMgr::CollectSurfaces(const Vector3& RayStart,
                                     const Vector3& RayEnd,
                                     uint32_t       Attributes,
                                     uint32_t       NotTheseAttributes)
{
    if (zones.size() == 0) {
        m_QueryNumber = 0;
        return;
    }

    m_QueryNumber++;
    if (m_QueryNumber == 0) {
        // zero is considered a special query, and will force us to reset
        // all of the id's so that we are 100% correct when this number
        // wraps around
        ClearDBaseQueries();
        m_QueryNumber++;
    }
    spatialDBase.CollectSurfaces(RayStart, RayEnd, m_QueryNumber, Attributes, NotTheseAttributes);
}

Surface* PlaysurfaceMgr::GetNextSurface()
{
    return spatialDBase.GetNextSurface();
}
