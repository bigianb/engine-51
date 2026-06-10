#include "PlaysurfaceMgr.h"
#include "DataReader.h"
#include "streamingOperators.h"
#include "objectManager/ObjectManager.h"

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

    // Exporter code does not have a resource manager.
    if (resourceManager){
        for(Surface& surface : zi.surfaces) {
            if (surface.GeomNameIndex >= geoms.size()) {
                std::cerr << "Warning: Surface geom index " << surface.GeomNameIndex << " is out of bounds (max " << geoms.size() << ")" << std::endl;
                surface.GeomNameIndex = 0;
                assert(false);
            } else {
                ResourceHandle<RigidGeom> geomHandle(resourceManager);
                geomHandle.setName( geoms[surface.GeomNameIndex].c_str() );
                RigidGeom* pGeom = geomHandle.getPointer();
                if (pGeom) {
                    surface.RenderInst = render::RegisterRigidInstance(*pGeom, resourceManager);
                } else {
                    std::cerr << "Warning: Failed to find geom for surface: " << geoms[surface.GeomNameIndex] << std::endl;
                    surface.RenderInst.setNull();
                }
            }
        }
    }

    reader.cursor = cursorToRestore;
}

bool PlaysurfaceMgr::readFile(const uint8_t* fileData, int len)
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

void PlaysurfaceMgr::RenderPlaySurfaces(ObjectManager* objectManager)
{
    if (zones.size() == 0) {
        return;
    }

    // mark playsurfaces that need to have lighting calculations done (going
    // through the lights using the spatial dbase should be quicker than
    // doing additional checks for all playsurfaces)
    // IJB MarkLitPlaySurfaces();

    // render the zones
    RenderZone(objectManager, zones[0], 0, 0); // default zone is always visible
    for (int i = 1; i < zones.size(); i++) {
        if (g_ZoneMgr.IsZoneVisible(i)) {
            RenderZone(objectManager, zones[i], i, 0);
        }
    }

    // render the portals
    for (int i = 0; i < portals.size(); i++) {
        zone_mgr::portal& Portal = g_ZoneMgr.GetPortal(i);
        if (g_ZoneMgr.IsZoneVisible(Portal.iZone[0]) ||
            g_ZoneMgr.IsZoneVisible(Portal.iZone[1])) {
            RenderZone(objectManager, portals[i], Portal.iZone[0], Portal.iZone[1]);
        }
    }
}

void PlaysurfaceMgr::RenderZone(ObjectManager* objectManager, ZoneInfo&         zoneInfo,
                                zone_mgr::zone_id Zone1,
                                zone_mgr::zone_id Zone2)
{
    // if this zone isn't loaded fully, we can't render it
    // if (zoneInfo.Resolved == false) {
    //     return;
    // }

    if (zoneInfo.numSurfaces == 0) {
        return;
    }

    // use the starting zone to determine whether we should do a zone vis check
    int  StartingZone = g_ZoneMgr.GetStartingZone();
    bool bNotInStartingZone = !((Zone1 == StartingZone) || (Zone2 == StartingZone));

    for (auto& surface : zoneInfo.surfaces) {
        // check if this surface is in the zone
        if (bNotInStartingZone && !g_ZoneMgr.IsBBoxVisible(surface.WorldBBox, Zone1, Zone2)) {
            continue;
        }

        // check for clipping against the view frustum
        int Vis = objectManager->IsBoxInView(surface.WorldBBox, 0b0111111);
        if (Vis == -1) {
            continue;
        }

        // check if we have a valid instance (bad data could cause this to get hit)
        if (surface.RenderInst.IsNull()) {
            continue;
        }

        // accumulate any other flags (Note that we don't use pSpadSurface because
        // we have no guarantee that pSurface was flushed from the cache before
        // our dma. That's okay becauses everything we access besides the render
        // flags is constant.)
        int Flags = surface.RenderFlags;

        // to clip or not to clip?
        if (Vis) {
            Flags |= render::CLIPPED;
        }

        // render it
        render::AddRigidInstanceSimple(surface.RenderInst,
                                       nullptr, // TODO, use index. (const uint16_t*)surface.pColor,
                                       &surface.L2W,
                                       surface.WorldBBox,
                                       Flags);

        // clear any accumulated flags for the next frame
        surface.RenderFlags &= ~(render::CLIPPED | render::SHADOW_PASS | render::DO_SIMPLE_LIGHTING);
    }
}
