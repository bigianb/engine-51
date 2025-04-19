
#include "GridWalker.h"
#include <cassert>
#include <cfloat>

grid_walker::grid_walker()
{
    m_Cell[0] = 0;
    m_Cell[1] = 0;
    m_Cell[2] = 0;
    m_GridCellSize = 0;
    m_GridSpaceCurrentLen = 0;
}

//=========================================================================

void grid_walker::Setup( const Vector3&    StartPos, 
                         const Vector3&    EndPos, 
                         const Vector3&    GridBasePos, 
                               float         GridCellSize )
{
    assert( GridCellSize > 0.0f );

    m_GridBase              = GridBasePos;
    m_GridCellSize          = GridCellSize;
    m_GridSpaceRayStart     = (StartPos-m_GridBase) / m_GridCellSize;
    m_GridSpaceRayEnd       = (EndPos-m_GridBase) / m_GridCellSize;
    m_GridSpaceRayDir       = m_GridSpaceRayEnd - m_GridSpaceRayStart;
    m_GridSpaceRayLen       = m_GridSpaceRayDir.Length();
    m_GridSpaceCurrentLen   = 0;
    m_GridSpaceSegmentStart = m_GridSpaceRayStart;
    m_GridSpaceSegmentEnd   = m_GridSpaceRayStart;

    // Normalize ray direction
    if( m_GridSpaceRayLen == 0.0f )
        m_GridSpaceRayDir = Vector3(1,0,0);
    else
        m_GridSpaceRayDir /= m_GridSpaceRayLen;

    for( int i=0; i<3; i++ )
    {
        // Initialize which cell we are starting in
        m_Cell[i] = (((int)(m_GridSpaceRayStart[i] + 16384))-16384);

        // Setup information for fast stepping
        step_info& S = m_StepInfo[i];

        if( m_GridSpaceRayDir[i] > 0.000001f )
        {
            S.CellIndexDelta                = 1;
            S.OneOverRayDirDelta            = 1.0f / m_GridSpaceRayDir[i];
            S.CellExitDeltaMinusRayStart    = 1.0f - m_GridSpaceRayStart[i];
            S.bDoStep                       = true;
        }
        else
        if( m_GridSpaceRayDir[i] < -0.000001f )
        {
            S.CellIndexDelta                = -1;
            S.OneOverRayDirDelta            = 1.0f / m_GridSpaceRayDir[i];
            S.CellExitDeltaMinusRayStart    = 0.0f - m_GridSpaceRayStart[i];
            S.bDoStep                       = true;
        }
        else
        {
            S.bDoStep = false;
        }
    }
}

//=========================================================================

bool grid_walker::Step( void )
{
    // Step to next cell.  Note that the computations are calculating the
    // distance to the exit point for the cell from scratch.  There is no
    // delta tracking except the integral cell[3] index.  This is to avoid
    // numerical precision issues due to the accumulation of deltas.
    // If computers could just do numbers well...

    // Init shortest distance to a new cell
    float NewCurrentLen = FLT_MAX;
    int NewCell[3]={0,0,0};

    // Loop through the three dimensions and step into next cell
    for( int i=0; i<3; i++ )
    {
        if( m_StepInfo[i].bDoStep )
        {
            step_info& S = m_StepInfo[i];
            float ExitDist = (m_Cell[i]+S.CellExitDeltaMinusRayStart) * S.OneOverRayDirDelta;
            if( ExitDist < NewCurrentLen )
            {
                NewCurrentLen = ExitDist;
                NewCell[0] = m_Cell[0];
                NewCell[1] = m_Cell[1];
                NewCell[2] = m_Cell[2];
                NewCell[i] = m_Cell[i] + S.CellIndexDelta;
            }
        }
    }

    assert( NewCurrentLen != FLT_MAX );

    // Update new cell indices
    m_Cell[0] = NewCell[0];
    m_Cell[1] = NewCell[1];
    m_Cell[2] = NewCell[2];

    // Update new len along ray
    m_GridSpaceCurrentLen = NewCurrentLen;

    // Compute new segment
    m_GridSpaceSegmentStart = m_GridSpaceSegmentEnd;
    m_GridSpaceSegmentEnd   = m_GridSpaceRayStart + (m_GridSpaceRayDir * m_GridSpaceCurrentLen);

    // Return true if we didn't step past the end of the ray
    return( m_GridSpaceCurrentLen <= m_GridSpaceRayLen );
}

