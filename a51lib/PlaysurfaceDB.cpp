#include "Playsurface.h"

void PlaysurfaceDB::read(DataReader& reader)
{
    cellSize = reader.readInt32();
    int dbNumCells = reader.readInt32();
    numSurfaces = reader.readInt32();

    for (int i=0; i < HASH_SIZE; ++i){
        hashTable[i].iCells = reader.readInt32();
        hashTable[i].nCells = reader.readInt32();
    }

    for (int i=0; i < dbNumCells; ++i){
        Cell c;
        c.X = reader.readInt32();
        c.Y = reader.readInt32();
        c.Z = reader.readInt32();
        c.iSurfaces = reader.readInt32();
        c.nSurfaces = reader.readInt32();
        c.MaxSurfaces = reader.readInt32();

        cells.push_back(c);
    }

}

void PlaysurfaceDB::describe(std::ostringstream& ss)
{
    ss << "DB Num cells " << cells.size() << std::endl;
    ss << "cell size: " << cellSize << std::endl;
    ss << "num surfaces: " << numSurfaces << std::endl;
}
