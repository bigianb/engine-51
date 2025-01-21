#include "Playsurface.h"

void PlaysurfaceDB::read(DataReader& reader)
{
    int dbCellSize = reader.readInt32();
    int dbNumCells = reader.readInt32();
    int dbNumServices = reader.readInt32();
}

void PlaysurfaceDB::describe(std::ostringstream& ss)
{

}
