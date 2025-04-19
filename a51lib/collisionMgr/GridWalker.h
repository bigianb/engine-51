#pragma once

#include "../VectorMath.h"

class grid_walker
{
public:
    grid_walker();

    void Setup(const Vector3& StartPos,
               const Vector3& EndPos,
               const Vector3& GridBasePos,
               float          GridCellSize);

    bool Step(void);

    inline void GetCell(int& X, int& Y, int& Z);
    inline void GetSegment(Vector3& SegmentStart, Vector3& SegmentEnd);

private:
    struct step_info
    {
        float CellExitDeltaMinusRayStart;
        int   CellIndexDelta;
        float OneOverRayDirDelta;
        bool  bDoStep;
    };

    int       m_Cell[3];
    float     m_GridCellSize;
    Vector3   m_GridBase;
    Vector3   m_GridSpaceRayStart;
    Vector3   m_GridSpaceRayEnd;
    Vector3   m_GridSpaceRayDir;
    Vector3   m_GridSpaceSegmentStart;
    Vector3   m_GridSpaceSegmentEnd;
    float     m_GridSpaceRayLen;
    float     m_GridSpaceCurrentLen;
    step_info m_StepInfo[3];
};

//==============================================================================

void grid_walker::GetCell(int& X, int& Y, int& Z)
{
    X = m_Cell[0];
    Y = m_Cell[1];
    Z = m_Cell[2];
}

//==============================================================================

void grid_walker::GetSegment(Vector3& SegmentStart, Vector3& SegmentEnd)
{
    SegmentStart = m_GridBase + (m_GridSpaceSegmentStart * m_GridCellSize);
    SegmentEnd = m_GridBase + (m_GridSpaceSegmentEnd * m_GridCellSize);
}
