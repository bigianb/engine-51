#pragma once

#include <cstdint>
#include "../VectorMath.h"

#define SPATIAL_CELL_NULL ((uint16_t)0xFFFF)

#define NUM_SPATIAL_CHANNELS 2
#define SPATIAL_CHANNEL_GENERIC 0
#define SPATIAL_CHANNEL_LIGHTS 1

struct spatial_user
{
    spatial_user(void)
    {
        for (int j = 0; j < NUM_SPATIAL_CHANNELS; j++) {
            FirstObjectLink[j] = -1; // LINK_NULL
        }
    }

    // This is the link_id of the object
    int FirstObjectLink[NUM_SPATIAL_CHANNELS];
};

class spatial_cell : public spatial_user
{
public:
    spatial_cell()
    {
        X = Y = Z = 0;
        Level = 0;
        SearchSeq = -1;
        OccFlags = 0;
        Flags = 0;
        HashNext = SPATIAL_CELL_NULL;
        HashPrev = SPATIAL_CELL_NULL;
        Parent = SPATIAL_CELL_NULL;
        Child = SPATIAL_CELL_NULL;
        Next = SPATIAL_CELL_NULL;
        Prev = SPATIAL_CELL_NULL;
        SearchNext = SPATIAL_CELL_NULL;
    }

    int OccFlags; // Is there something in it

    enum flags
    {
        FLAG_SKIN_PLANES = 0b111111
    };
    inline uint32_t GetFlags(void) { return Flags; }

protected:
    int16_t  X, Y, Z;  // Actual position of the cell in the world
    uint8_t  Level;    // Level in the Hirarchy tree
    uint8_t  Flags;    // Flags
    uint16_t HashNext; // Next node in the hash entry
    uint16_t HashPrev; // Previous node in the hash entry
    uint16_t Parent;   // Link to the Parent in the tree
    uint16_t Child;    // Link to its first Child  in the tree
    uint16_t Next;     // Link to the next node in the tree
    uint16_t Prev;     // Link to the Previous node in the tree

    uint16_t SearchNext; // Links all visible nodes
    int      SearchSeq;  // Sequence for the search

private:
    // Not copy allow
    const spatial_cell& operator=(const spatial_cell&)
    {
        return *this;
    }

    friend class spatial_dbase;
};

class view;

class spatial_dbase
{
public:
    spatial_dbase();
    ~spatial_dbase();

    void Init(float MinCellWidth);
    void Kill();
    void SanityCheck();
    void DumpStats() const;
    void Render(int MinLevel, int MaxLevel) const;

    spatial_cell* GetCell(int X, int Y, int Z, int Level, bool Create = false);
    spatial_cell& GetCell(uint16_t ID) const;
    uint16_t      GetCellIndex(int X, int Y, int Z, int Level, bool Create = false);

    BBox GetCellBBox(int X, int Y, int Z, int Level) const;
    BBox GetCellBBox(const spatial_cell& Cell) const;
    void UpdateCell(spatial_cell& pCell);

    void GetCellRegion(const BBox& BBox, int Level,
                       int& MinX, int& MinY, int& MinZ,
                       int& MaxX, int& MaxY, int& MaxZ) const;

    int GetBBoxLevel(const BBox& BBox) const;
    int GetCellCount() { return m_nCells; }

    uint16_t BuildVisList(const view& View, bool DoCulling);
    uint16_t TraverseCells(const BBox& BBox, uint32_t OccFlags = 0xFFFFFFFF);
    uint16_t TraverseCells(const Vector3& RayStart,
                           const Vector3& RayEnd,
                           uint32_t       OccFlags = 0xFFFFFFFF);

    uint16_t GetFirstInSearch();
    uint16_t GetNextInSearch(uint16_t ID);

protected:
    enum
    {
        MAX_LEVELS = 12,
        HASH_SIZE = 16001,
    };

    struct hash
    {
        uint16_t FirstCell;
        int16_t  nCells;
    };

public:
    void Clear(void);

protected:
    int      ComputeHash(int X, int Y, int Z, int Level) const;
    uint16_t AllocCell(int X, int Y, int Z, int Level);
    void     FreeCell(uint16_t ID);

protected:
    spatial_cell* m_pCell;           // Pointer of nodes
    int           m_nCells;          // Number of nodes in the tree
    int           m_nCellsAllocated; // How many total nodes we have allocated

    int      m_SearchSeq;    // Sequence use to skip nodes in searches
    uint16_t m_FirstFree;    // Link list of free nodes
    uint16_t m_FirstSearch;  // Link list of nodes for the active search
    int      m_nSearchNodes; // Number of

    float m_CellWidth[MAX_LEVELS];     // Precomputer cell sizes
    int   m_nCellsInLevel[MAX_LEVELS]; // Counts of cells in levels
    hash  m_Hash[HASH_SIZE];           // Hash of active cells
};

#include "SpatialDBase_inline.h"
