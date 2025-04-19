#pragma once

#include <vector>
#include <cstdint>
#include <sstream>
#include "render/Geom.h"
#include "render/CollisionVolume.h"
#include "DataReader.h"
#include "Guid.h"

struct Surface;

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

    PlaysurfaceDB()
    {
        m_QueryNumber = 0;
        m_BuildingNewDBase = false;
        cellSize = 0;
    }

    void Reset()
    {
        m_QueryNumber = 0;
        m_BuildingNewDBase = false;
        cellSize = 0;
        cells.clear();
        surfaces.clear();
    }

    void read(DataReader&);
    void describe(std::ostringstream& ss);

    void CollectSurfaces(const BBox& BBox,
                         uint32_t    QueryNumber,
                         uint32_t    Attributes,
                         uint32_t    NotTheseAttributes);

    void CollectSurfaces(const Vector3& RayStart,
                         const Vector3& RayEnd,
                         uint32_t       QueryNumber,
                         uint32_t       Attributes,
                         uint32_t       NotTheseAttributes);

    Surface* GetNextSurface();
    Surface* GetCurrentSurface();
    guid     GetCurrentGuid() const;
    Surface* GetSurfaceByGuid(guid Guid);

private:
    Cell*     GetCell(int XCell, int YCell, int ZCell);
    void      GetCellRange(const BBox& bb,
                           int& XMin, int& XMax,
                           int& YMin, int& YMax,
                           int& ZMin, int& ZMax);
    void     GetCell(float XPos, float YPos, float ZPos,
                      int& XCell, int& YCell, int& ZCell);
    HashEntry hashTable[HASH_SIZE];

    int  m_QueryNumber;
    bool m_BuildingNewDBase;

    int cellSize;
    int numSurfaces;

    int m_CurrentSurface;
    int m_NextSurface;

    std::vector<Cell>     cells;
    std::vector<Surface*> surfaces;
};
