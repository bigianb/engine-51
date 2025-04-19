#include "SpatialDBase.h"

#include <cassert>
#include <cstdlib>
#include <new>
#include <cfloat>

spatial_dbase::spatial_dbase(void)
{
    m_pCell = NULL;
    Clear();
}

spatial_dbase::~spatial_dbase(void)
{
    Clear();
}

spatial_cell& spatial_dbase::GetCell(uint16_t ID) const
{
    assert(ID != SPATIAL_CELL_NULL);
    assert(ID < m_nCellsAllocated);
    return m_pCell[ID];
}

spatial_cell* spatial_dbase::GetCell(int X, int Y, int Z, int Level, bool Create)
{
    uint16_t ID = GetCellIndex(X, Y, Z, Level, Create);
    if (ID == SPATIAL_CELL_NULL) {
        return NULL;
    }
    return &GetCell(ID);
}

uint16_t spatial_dbase::GetCellIndex(int X, int Y, int Z, int Level, bool Create)
{
    // Compute hash entry
    int H = ComputeHash(X, Y, Z, Level);

    // Loop through hash entries and look for a match
    uint16_t ID = m_Hash[H].FirstCell;
    while (ID != SPATIAL_CELL_NULL) {
        const spatial_cell& Cell = GetCell(ID);

        if ((Cell.X == X) &&
            (Cell.Y == Y) &&
            (Cell.Z == Z) &&
            (Cell.Level == Level)) {
            return ID;
        }

        ID = Cell.HashNext;
    }

    // Could not find cell and caller requested to create one
    if (Create) {
        ID = AllocCell(X, Y, Z, Level);
        return ID;
    }

    // Could not find cell and caller did not request to create one
    // so just return
    return SPATIAL_CELL_NULL;
}

int spatial_dbase::ComputeHash(int X, int Y, int Z, int Level) const
{
    // Compute hash index
    int H = ((uint32_t)((((((X << 10) + Y) << 10) + Z) << 10) + Level)) % HASH_SIZE;
    assert((H >= 0) && (H < HASH_SIZE));
    return H;
}

void spatial_dbase::Clear(void)
{
    if (m_pCell) {
        free(m_pCell);
    }

    m_pCell = NULL;
    m_nCellsAllocated = 0;
    m_nCells = 0;
    m_FirstFree = SPATIAL_CELL_NULL;
    m_SearchSeq = -1;

    // Clear number of cells in each level
    for (int i = 0; i < MAX_LEVELS; i++) {
        m_nCellsInLevel[i] = 0;
    }
}

void spatial_dbase::Kill(void)
{
    Clear();
}

void spatial_dbase::Init(float MinCellWidth)
{
    // Reset the class
    Clear();

    // Set the smallest cell
    m_CellWidth[0] = MinCellWidth;

    // Pre-Compute cell sizes
    for (int i = 1; i < MAX_LEVELS; i++) {
        m_CellWidth[i] = 0;
    }

    for (int i = 1; i < MAX_LEVELS; i++) {
        m_CellWidth[i] = m_CellWidth[i - 1] * 2.0f;
    }

    // Clear hash table
    for (int i = 0; i < HASH_SIZE; i++) {
        m_Hash[i].FirstCell = SPATIAL_CELL_NULL;
        m_Hash[i].nCells = 0;
    }
}

void spatial_dbase::UpdateCell(spatial_cell& Cell)
{
    assert((&Cell >= m_pCell) && (&Cell < (&m_pCell[m_nCellsAllocated])));

    if ((Cell.OccFlags == 0) && (Cell.Child == SPATIAL_CELL_NULL)) {
        int Index = (((uint8_t*)&Cell) - (uint8_t*)m_pCell) / ((int)sizeof(spatial_cell));
        assert(Index >= 0 && Index < m_nCellsAllocated);
        FreeCell(Index);
    }
}

void spatial_dbase::FreeCell(uint16_t ID)
{
    //x_DebugMsg("Calling FreeCell on %3d\n", I);
    spatial_cell& Cell = GetCell(ID);

    // TODO: Confirm there are no children
    assert(Cell.Child == SPATIAL_CELL_NULL);
    assert(Cell.OccFlags == 0);

    //
    // Remove from parent
    //
    if (Cell.Level < MAX_LEVELS - 1) {
        spatial_cell& Parent = GetCell(Cell.Parent);

        if (Parent.Child == ID) {
            Parent.Child = Cell.Next;
        }

        if (Cell.Next != SPATIAL_CELL_NULL) {
            spatial_cell& Prev = GetCell(Cell.Next);
            Prev.Prev = Cell.Prev;
        }

        if (Cell.Prev != SPATIAL_CELL_NULL) {
            spatial_cell& Prev = GetCell(Cell.Prev);
            Prev.Next = Cell.Next;
        }

        // Check if we should recursively delete parent
        if ((Parent.Child == SPATIAL_CELL_NULL) && (Parent.OccFlags == 0)) {
            FreeCell(Cell.Parent);
        }

        Cell.Parent = SPATIAL_CELL_NULL;
    }

    //
    // Decrease number of Cells
    //
    m_nCells--;
    m_nCellsInLevel[Cell.Level]--;

    //
    // Remove node from the hash table
    //

    // Compute hash
    int H = ComputeHash(Cell.X, Cell.Y, Cell.Z, Cell.Level);

    // Remove from hash
    if (Cell.HashNext != SPATIAL_CELL_NULL) {
        spatial_cell& HashNext = GetCell(Cell.HashNext);
        HashNext.HashPrev = Cell.HashPrev;
    }

    if (Cell.HashPrev != SPATIAL_CELL_NULL) {
        spatial_cell& HashPrev = GetCell(Cell.HashPrev);
        HashPrev.HashNext = Cell.HashNext;
    }

    if (m_Hash[H].FirstCell == ID) {
        m_Hash[H].FirstCell = Cell.HashNext;
    }

    m_Hash[H].nCells--;
    assert(m_Hash[H].nCells >= 0);

    //
    // Add node to free list
    //
    Cell.HashNext = m_FirstFree;
    Cell.HashPrev = SPATIAL_CELL_NULL;

    if (m_FirstFree != SPATIAL_CELL_NULL) {
        spatial_cell& Frist = GetCell(m_FirstFree);
        Frist.HashPrev = ID;
    }

    m_FirstFree = ID;
}

uint16_t spatial_dbase::AllocCell(int X, int Y, int Z, int Level)
{
    // Check if we need to grow cell list
    if (m_nCells == m_nCellsAllocated) {
        // WARNING:
        // Todo this is very unsave. Since some of the functions have references to the
        // memory previously allocated. So when it reallocs and changes the memory it crashes
        // the computer.
        //
        if (m_nCellsAllocated > 0) {
            assert(false); // Must Make the spacial data base bigger
        }

        m_nCellsAllocated += (2048 * 8);

        //x_DebugMsg("WARNING: AllocCell1: %08X ", (u32)m_pCell);
        m_pCell = (spatial_cell*)realloc(m_pCell, sizeof(spatial_cell) * m_nCellsAllocated);
        assert(m_pCell);
        //x_DebugMsg("AllocCell2: %08X\n", m_pCell);

        // Add new Cells to free list
        for (int i = m_nCellsAllocated - 1; i >= m_nCells; i--) {
            spatial_cell& Cell = GetCell(i);

            // placement new
            new (&Cell) spatial_cell;

            // Add cells into the empty list
            Cell.HashNext = m_FirstFree;
            Cell.HashPrev = SPATIAL_CELL_NULL;

            if (m_FirstFree != SPATIAL_CELL_NULL) {
                spatial_cell& First = GetCell(m_FirstFree);
                First.HashPrev = i;
            }

            m_FirstFree = i;
        }
    }

    //
    // Increase number of Cells
    //
    m_nCells++;
    m_nCellsInLevel[Level]++;

    //
    // Pull Cell from free list
    //
    assert(m_FirstFree != SPATIAL_CELL_NULL);
    uint32_t      ID = m_FirstFree;
    spatial_cell& Cell = GetCell(ID);

    m_FirstFree = Cell.HashNext;
    if (m_FirstFree != SPATIAL_CELL_NULL) {
        spatial_cell& First = GetCell(m_FirstFree);
        First.HashPrev = SPATIAL_CELL_NULL;
    }

    //
    // Fill out cell
    //
    {
        Cell.X = X;
        Cell.Y = Y;
        Cell.Z = Z;
        Cell.Level = Level;
        Cell.OccFlags = 0; // TODO: Add flags?
    }

    //
    // Compute hash entry
    //
    int H = ComputeHash(X, Y, Z, Level);

    // Add to hash list
    if (m_Hash[H].FirstCell != SPATIAL_CELL_NULL) {
        spatial_cell& First = GetCell(m_Hash[H].FirstCell);
        First.HashPrev = ID;
    }

    Cell.HashNext = m_Hash[H].FirstCell;
    Cell.HashPrev = SPATIAL_CELL_NULL;
    m_Hash[H].FirstCell = ID;
    m_Hash[H].nCells++;

    // Hook up to parents and siblings
    if (Level < MAX_LEVELS - 1) {
        // Do recursive call to create parent
        int ParentID = GetCellIndex(X >> 1, Y >> 1, Z >> 1, Level + 1, true);

        // Get ptrs to parent and child
        spatial_cell& Parent = GetCell(ParentID);

        // Remember who the parent is
        Cell.Parent = ParentID;
        Cell.Child = SPATIAL_CELL_NULL;

        // Attach child to parent
        if (Parent.Child != SPATIAL_CELL_NULL) {
            spatial_cell& FirstChild = GetCell(Parent.Child);
            FirstChild.Prev = ID;
        }

        Cell.Next = Parent.Child;
        Cell.Prev = SPATIAL_CELL_NULL;
        Parent.Child = ID;
    } else {
        Cell.Parent = SPATIAL_CELL_NULL;
        Cell.Child = SPATIAL_CELL_NULL;
        Cell.Next = SPATIAL_CELL_NULL;
        Cell.Prev = SPATIAL_CELL_NULL;
    }

    return ID;
}

uint16_t spatial_dbase::TraverseCells(const BBox& RegionBBox, uint32_t OccFlags)
{
    int      i, X, Y, Z;
    int      MinX, MinY, MinZ;
    int      MaxX, MaxY, MaxZ;
    int      RegionLevel;
    int      nChecked = 0;
    uint16_t Cursor[MAX_LEVELS];

    m_SearchSeq++;

    // Get information about what level and span this region covers
    RegionLevel = GetBBoxLevel(RegionBBox);
    GetCellRegion(RegionBBox, RegionLevel, MinX, MinY, MinZ, MaxX, MaxY, MaxZ);

    // Loop through cells at original level
    m_FirstSearch = SPATIAL_CELL_NULL;
    m_nSearchNodes = 0;
    i = 0;
    for (X = MinX; X <= MaxX; X++) {
        for (Y = MinY; Y <= MaxY; Y++) {
            for (Z = MinZ; Z <= MaxZ; Z++) {
                // For each cell, find lowest level that a cell exists
                int CX = X;
                int CY = Y;
                int CZ = Z;
                int CellID = -1;
                int CLevel = RegionLevel;
                while (CLevel < MAX_LEVELS) {
                    CellID = GetCellIndex(CX, CY, CZ, CLevel);
                    if (CellID != SPATIAL_CELL_NULL) {
                        break;
                    }

                    CX >>= 1;
                    CY >>= 1;
                    CZ >>= 1;
                    CLevel++;
                }

                assert(CellID != -1);

                // Get access to cell
                if (CellID != SPATIAL_CELL_NULL) {

                    spatial_cell* pCell = &GetCell(CellID);

                    if (pCell->SearchSeq != m_SearchSeq) {
                        //                pCell->SearchSeq = m_SearchSeq;

                        // Run up parents
                        //                int C=0;
                        while (pCell->Parent != SPATIAL_CELL_NULL) {
                            //assert( (C++) < 1000 );
                            uint16_t ID = pCell->Parent;

                            pCell = &GetCell(ID);

                            if (pCell->SearchSeq == m_SearchSeq) {
                                break;
                            }

                            pCell->SearchSeq = m_SearchSeq;

                            nChecked++;

                            if (pCell->OccFlags & OccFlags) {
                                m_nSearchNodes++;

                                if (m_FirstSearch == SPATIAL_CELL_NULL) {
                                    pCell->SearchNext = SPATIAL_CELL_NULL;
                                    m_FirstSearch = ID;
                                } else {
                                    pCell->SearchNext = m_FirstSearch;
                                    m_FirstSearch = ID;
                                }
                            }
                        }

                        // Process children
                        {
                            assert(CLevel < MAX_LEVELS);
                            Cursor[CLevel] = GetCellIndex(CX, CY, CZ, CLevel);
                            int Level = CLevel;

                            //                    int C=0;
                            while (Level <= CLevel) {
                                //assert( (C++) < 1000 );

                                // Pop up
                                if (Cursor[Level] == SPATIAL_CELL_NULL) {
                                    Level++;
                                    continue;
                                }

                                // Get cell
                                nChecked++;
                                uint16_t      ID = Cursor[Level];
                                spatial_cell* pCell = &GetCell(ID);
                                assert(pCell);

                                if (pCell->SearchSeq == m_SearchSeq) {
                                    //                            Level++;
                                    break;
                                }

                                // Check if current cell is visible
                                bool InRegion = GetCellBBox(*pCell).Intersect(RegionBBox);

                                if (InRegion) {
                                    pCell->SearchSeq = m_SearchSeq;

                                    // Call user function
                                    if (pCell->OccFlags & OccFlags) {
                                        m_nSearchNodes++;
                                        if (m_FirstSearch == SPATIAL_CELL_NULL) {
                                            pCell->SearchNext = SPATIAL_CELL_NULL;
                                            m_FirstSearch = ID;
                                        } else {
                                            pCell->SearchNext = m_FirstSearch;
                                            m_FirstSearch = ID;
                                        }
                                    }

                                    // Move this level to sibling
                                    assert(Level < MAX_LEVELS);
                                    Cursor[Level] = pCell->Next;

                                    // Push children
                                    if (Level > 0) {
                                        Level--;
                                        assert(Level < MAX_LEVELS);
                                        Cursor[Level] = pCell->Child;
                                    }
                                } else {
                                    // Move to next in list
                                    assert(Level < MAX_LEVELS);
                                    Cursor[Level] = pCell->Next;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return m_FirstSearch;
}

uint16_t spatial_dbase::TraverseCells(const Vector3& WorldSpaceRayStart,
                                      const Vector3& WorldSpaceRayEnd,
                                      uint32_t       OccFlags)
{
    // Get ray coordinates in spatial-dbase cells
    float   BaseCellSize = m_CellWidth[0];
    Vector3 RayStart = WorldSpaceRayStart / BaseCellSize;
    Vector3 RayEnd = WorldSpaceRayEnd / BaseCellSize;
    Vector3 RayDir = RayEnd - RayStart;
    float   RayLen = RayDir.Length();

    if (RayLen == 0.0f) {
        return SPATIAL_CELL_NULL;
    } else {
        RayDir /= RayLen;
    }

    // Initialize which cell we are starting in
    int curCell[3] = {0, 0, 0};
    curCell[0] = (((int)(RayStart[0] + 16384)) - 16384);
    curCell[1] = (((int)(RayStart[1] + 16384)) - 16384);
    curCell[2] = (((int)(RayStart[2] + 16384)) - 16384);

    // Clear search info
    m_SearchSeq++;
    m_FirstSearch = SPATIAL_CELL_NULL;
    m_nSearchNodes = 0;

    // Clear the current distance and begin loop through ray length
    float CurrentDist = 0;
    while (CurrentDist < RayLen) {
        // Process current Cell
        {
            int CX = curCell[0];
            int CY = curCell[1];
            int CZ = curCell[2];

            // Find the first cell that actually contains something
            int CellID = -1;
            int CellLevel = 0;
            while (CellLevel < MAX_LEVELS) {
                CellID = GetCellIndex(CX, CY, CZ, CellLevel);
                if (CellID != SPATIAL_CELL_NULL) {
                    break;
                }

                CX >>= 1;
                CY >>= 1;
                CZ >>= 1;
                CellLevel++;
            }

            // Process this cell and parents
            while (CellID != SPATIAL_CELL_NULL) {
                // Get access to this cell
                spatial_cell* pCell = &GetCell(CellID);

                // If we have seen this cell during this search already
                // then we know we have seen all the parents so bail.
                if (pCell->SearchSeq == m_SearchSeq) {
                    break;
                }

                // Remember we have visited this cell
                pCell->SearchSeq = m_SearchSeq;

                // Add this cell to the search results
                if (pCell->OccFlags & OccFlags) {
                    m_nSearchNodes++;

                    if (m_FirstSearch == SPATIAL_CELL_NULL) {
                        pCell->SearchNext = SPATIAL_CELL_NULL;
                        m_FirstSearch = CellID;
                    } else {
                        pCell->SearchNext = m_FirstSearch;
                        m_FirstSearch = CellID;
                    }
                }

                // Go to parent
                CellID = pCell->Parent;
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

            assert(NewDist != FLT_MAX);
            CurrentDist = NewDist;
            curCell[0] = NewCell[0];
            curCell[1] = NewCell[1];
            curCell[2] = NewCell[2];
        }
    }

    return m_FirstSearch;
}

uint16_t spatial_dbase::GetFirstInSearch()
{
    return m_FirstSearch;
}

uint16_t spatial_dbase::GetNextInSearch(uint16_t ID)
{
    assert(ID < m_nCellsAllocated);
    assert((m_pCell[ID].SearchNext == SPATIAL_CELL_NULL) ||
           (ID < m_nCellsAllocated));

    return m_pCell[ID].SearchNext;
}
