#pragma once

#include <vector>
#include <cstdint>
#include <sstream>
#include "Geom.h"
#include "CollisionVolume.h"
#include "DataReader.h"

class PlaysurfaceDB
{
public:
    enum
    {
        HASH_SIZE = 1021
    }; 

    struct Cell
    {
        int X, Y, Z;
        int iSurfaces;
        int nSurfaces;
        int MaxSurfaces;
    };

    struct HashEntry
    {
        int iCells;
        int nCells;
    };

    void read(DataReader&);
    void describe(std::ostringstream& ss);

    std::vector<Cell> cells;
    std::vector<HashEntry> surfaces;
};
