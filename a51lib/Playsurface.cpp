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

    return okay;
}
void Playsurface::describe(std::ostringstream& ss)
{
    ss << "Version:" << version << std::endl;
    ss << "Num Zones:" << numZones << std::endl;
    ss << "Num Portals:" << numPortals << std::endl;
    ss << "Num Geoms:" << numGeoms << std::endl;

    spatialDBase.describe(ss);
}
