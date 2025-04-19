#include "PlaysurfaceMgr.h"

void PlaysurfaceDB::read(DataReader& reader)
{
    cellSize = reader.readInt32();
    int dbNumCells = reader.readInt32();
    numSurfaces = reader.readInt32();

    for (int i = 0; i < HASH_SIZE; ++i) {
        hashTable[i].iCells = reader.readInt32();
        hashTable[i].nCells = reader.readInt32();
    }

    for (int i = 0; i < dbNumCells; ++i) {
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

Surface* PlaysurfaceDB::GetCurrentSurface()
{
    if (m_CurrentSurface == -1) {
        return nullptr;
    } else {
        return surfaces[m_CurrentSurface];
    }
}

guid PlaysurfaceDB::GetCurrentGuid(void) const
{
    if (m_CurrentSurface == -1) {
        return guid(0);
    } else {
        return guid(((uint64_t)m_CurrentSurface << 32) | 0xffffffff);
    }
}

Surface* PlaysurfaceDB::GetSurfaceByGuid(guid Guid)
{
    return surfaces[(Guid >> 32)];
}

Surface* PlaysurfaceDB::GetNextSurface()
{
    // empty search or we've reached the end?
    if (m_QueryNumber == 0) {
        m_CurrentSurface = -1;
        return nullptr;
    }

    // get the value to return
    m_CurrentSurface = m_NextSurface;
    Surface* pSurface = surfaces[m_CurrentSurface];

    // move to the next cell/surface
    m_NextSurface = pSurface->NextSurface;
    if (m_NextSurface == -1) {
        m_QueryNumber = 0; // we've reached the end
    }

    return pSurface;
}

const int HASH_SIZE = 1021; // keep this a prime number for good hashing

int DBaseHashFn(int XCell, int YCell, int ZCell)
{
    return ((((XCell << 10) + YCell) << 10) + ZCell) % HASH_SIZE;
}

PlaysurfaceDB::Cell* PlaysurfaceDB::GetCell(int XCell, int YCell, int ZCell)
{
    int HashIndex = DBaseHashFn(XCell, YCell, ZCell);

    for (int iCell = hashTable[HashIndex].iCells;
         iCell < hashTable[HashIndex].iCells + hashTable[HashIndex].nCells;
         iCell++) {
        Cell& cell = cells[iCell];
        if ((cell.X == XCell) && (cell.Y == YCell) && (cell.Z == ZCell)) {
            return &cell;
        }
    }

    return NULL;
}

void PlaysurfaceDB::GetCell(float XPos, float YPos, float ZPos,
                            int& XCell, int& YCell, int& ZCell)
{
    // The playstation and pc like to handle floats differently, and rounding
    // errors will eat you alive, so convert the positions to integers for
    // these operations. (Trust me...or test x_floor((f32)XPos/1600.0f)) The
    // integer method introduces an error of 1 centimeter, which should be
    // close enough.
    int XPosInt = (int)XPos;
    int YPosInt = (int)YPos;
    int ZPosInt = (int)ZPos;

    if (XPosInt >= 0) {
        XCell = XPosInt / cellSize;
    } else {
        XCell = XPosInt / cellSize - ((XPosInt % cellSize) ? 1 : 0);
    }
    if (YPosInt >= 0) {
        YCell = YPosInt / cellSize;
    } else {
        YCell = YPosInt / cellSize - ((YPosInt % cellSize) ? 1 : 0);
    }
    if (ZPosInt >= 0) {
        ZCell = ZPosInt / cellSize;
    } else {
        ZCell = ZPosInt / cellSize - ((ZPosInt % cellSize) ? 1 : 0);
    }
}

void PlaysurfaceDB::GetCellRange(const BBox& bb,
                                 int& XMin, int& XMax,
                                 int& YMin, int& YMax,
                                 int& ZMin, int& ZMax)
{
    GetCell(bb.min.x, bb.min.y, bb.min.z, XMin, YMin, ZMin);
    GetCell(bb.max.x, bb.max.y, bb.max.z, XMax, YMax, ZMax);
}

void PlaysurfaceDB::CollectSurfaces(const BBox& bb,
                                    uint32_t    QueryNumber,
                                    uint32_t    Attributes,
                                    uint32_t    NotTheseAttributes)
{
    m_QueryNumber = QueryNumber;
    m_CurrentSurface = -1;

    // what is the range of cells to collect?
    int XMin, XMax, YMin, YMax, ZMin, ZMax;
    GetCellRange(bb, XMin, XMax, YMin, YMax, ZMin, ZMax);

    // walk the cells, and build a linked list
    Surface* pPrevSurface = nullptr;
    for (int XCell = XMin; XCell <= XMax; XCell++) {
        for (int YCell = YMin; YCell <= YMax; YCell++) {
            for (int ZCell = ZMin; ZCell <= ZMax; ZCell++) {
                Cell* pCell = GetCell(XCell, YCell, ZCell);
                if (pCell && pCell->nSurfaces) {
                    // check each of the cell surfaces
                    for (int iSurface = pCell->iSurfaces;
                         iSurface < (pCell->iSurfaces + pCell->nSurfaces);
                         iSurface++) {
                        Surface* pSurface = surfaces[iSurface];

                        if (pSurface->DBaseQuery == QueryNumber) {
                            continue;
                        }
                        if ((pSurface->AttrBits & Attributes) == 0) {
                            continue;
                        }
                        if ((pSurface->AttrBits & NotTheseAttributes) != 0) {
                            continue;
                        }

                        if (pSurface->WorldBBox.Intersect(bb)) {
                            pSurface->DBaseQuery = QueryNumber;
                            pSurface->NextSurface = -1;
                            if (m_CurrentSurface == -1) {
                                m_CurrentSurface = iSurface;
                            } else {
                                pPrevSurface->NextSurface = iSurface;
                            }

                            pPrevSurface = pSurface;
                        }
                    }
                }
            }
        }
    }

    // disable the query if no surfaces were found
    if (m_CurrentSurface == -1) {
        m_QueryNumber = 0;
    } else {
        m_NextSurface = m_CurrentSurface;
    }
}

void PlaysurfaceDB::CollectSurfaces(const Vector3& WorldSpaceRayStart,
                                    const Vector3& WorldSpaceRayEnd,
                                    uint32_t       QueryNumber,
                                    uint32_t       Attributes,
                                    uint32_t       NotTheseAttributes)
{
    // Get ray coordinates in spatial-dbase cells
    Vector3 RayStart = WorldSpaceRayStart / (float)cellSize;
    Vector3 RayEnd = WorldSpaceRayEnd / (float)cellSize;
    Vector3 RayDir = RayEnd - RayStart;
    float   RayLen = RayDir.Length();

    if (RayLen == 0.0f) {
        return;
    } else {
        RayDir /= RayLen;
    }

    // Initialize which cell we are starting in
    int curCell[3] = {0, 0, 0};
    curCell[0] = (((int)(RayStart[0] + 16384)) - 16384);
    curCell[1] = (((int)(RayStart[1] + 16384)) - 16384);
    curCell[2] = (((int)(RayStart[2] + 16384)) - 16384);

    // Clear search info
    m_QueryNumber = QueryNumber;
    m_CurrentSurface = -1;
    Surface* pPrevSurface = nullptr;

    // Clear the current distance and begin loop through ray length
    float CurrentDist = 0;
    while (CurrentDist < RayLen) {
        // Process current Cell
        {
            Cell* pCell = GetCell(curCell[0], curCell[1], curCell[2]);
            if (pCell && pCell->nSurfaces) {
                // check each of the cell surfaces
                for (int iSurface = pCell->iSurfaces;
                     iSurface < (pCell->iSurfaces + pCell->nSurfaces);
                     iSurface++) {
                    Surface* pSurface = surfaces[iSurface];
                    if (pSurface->DBaseQuery == QueryNumber) {
                        continue;
                    }
                    if ((pSurface->AttrBits & Attributes) == 0) {
                        continue;
                    }
                    if ((pSurface->AttrBits & NotTheseAttributes) != 0) {
                        continue;
                    }

                    float T;
                    if (pSurface->WorldBBox.Intersect(T, WorldSpaceRayStart, WorldSpaceRayEnd)) {
                        pSurface->DBaseQuery = QueryNumber;
                        pSurface->NextSurface = -1;
                        if (m_CurrentSurface == -1) {
                            m_CurrentSurface = iSurface;
                        } else {
                            pPrevSurface->NextSurface = iSurface;
                        }

                        pPrevSurface = pSurface;
                    }
                }
            }
        }

        // Step to next cell.  Note that the computations are calculating the
        // distance to the exit point for the cell from scratch.  There is no
        // delta tracking except the integral cell[3] index.  This is to avoid
        // numerical precision issues due to the accumulation of small deltas.
        {
            // Init shortest distance to a new cell
            float NewDist = FLT_MAX;
            int   NewCell[3];

            // Loop through the three dimensions and figure out what the next
            // cell will be.
            for (int i = 0; i < 3; i++) {
                if (RayDir[i] > 0.000001f) {
                    float Dist = ((curCell[i] + 1.0f) - RayStart[i]) / RayDir[i];
                    if (Dist < NewDist) {
                        NewDist = Dist;
                        NewCell[0] = curCell[0];
                        NewCell[1] = curCell[1];
                        NewCell[2] = curCell[2];
                        NewCell[i] = curCell[i] + 1;
                    }
                } else if (RayDir[i] < -0.000001f) {
                    float Dist = ((curCell[i]) - RayStart[i]) / RayDir[i];
                    if (Dist < NewDist) {
                        NewDist = Dist;
                        NewCell[0] = curCell[0];
                        NewCell[1] = curCell[1];
                        NewCell[2] = curCell[2];
                        NewCell[i] = curCell[i] - 1;
                    }
                }
            }

            CurrentDist = NewDist;
            curCell[0] = NewCell[0];
            curCell[1] = NewCell[1];
            curCell[2] = NewCell[2];
        }
    }

    // disable the query if no surfaces were found
    if (m_CurrentSurface == -1) {
        m_QueryNumber = 0;
    } else {
        m_NextSurface = m_CurrentSurface;
    }
}
