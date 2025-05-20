
inline
spatial_cell& spatial_dbase::GetCell( uint16_t ID ) const
{
    assert( ID != SPATIAL_CELL_NULL );
    assert( ID < m_nCellsAllocated );
    return m_pCell[ ID ];
}

inline
uint16_t spatial_dbase::GetFirstInSearch() 
{
    return m_FirstSearch;
}

inline
uint16_t spatial_dbase::GetNextInSearch( uint16_t ID ) 
{
    assert( ID < m_nCellsAllocated );
    assert( (m_pCell[ ID ].SearchNext == SPATIAL_CELL_NULL) || 
            ( ID < m_nCellsAllocated ) );

    return m_pCell[ ID ].SearchNext;
}

inline
spatial_cell* spatial_dbase::GetCell( int X, int Y, int Z, int Level, bool Create )
{
    uint16_t ID = GetCellIndex( X, Y, Z, Level, Create );
    if( ID == SPATIAL_CELL_NULL )
        return NULL;
    return &GetCell( ID );
}

inline
int spatial_dbase::ComputeHash( int X, int Y, int Z, int Level ) const
{
    // Compute hash index
    int H = ((uint32_t)((((((X<<10)+Y)<<10)+Z)<<10)+Level)) % HASH_SIZE;
    assert( (H>=0) && (H<HASH_SIZE) );
    return H;
}

inline
BBox spatial_dbase::GetCellBBox( int X, int Y, int Z, int Level ) const
{
    const float W = m_CellWidth[ Level ];

    return BBox( Vector3(X*W,   Y*W,   Z*W),
                 Vector3(X*W+W, Y*W+W, Z*W+W) );
}

inline
BBox spatial_dbase::GetCellBBox( const spatial_cell& Cell ) const
{
    return GetCellBBox( Cell.X, Cell.Y, Cell.Z, Cell.Level );
}

inline
int spatial_dbase::GetBBoxLevel( const BBox& BBox ) const
{
    // Get largest dimension...treat it as a cube
    Vector3 Size = BBox.GetSize();
    float     S    = std::max( std::max(Size.x, Size.y), Size.z);

    // Find level such that S <= CellWidth
    for( int i=0; i<MAX_LEVELS; i++ )
    if( S <= m_CellWidth[i] )
    {
        // Push objects that are close to the size of the cell
        // to the next level
        if( abs(S-m_CellWidth[i]) < 2.0f )
        {
            i++;
            assert( i<MAX_LEVELS );
        }

        return i;
    }

    // Object too big or too few levels
    
    //x_throw( "Object too big or too few levels" );
    return 0;
}

inline
void spatial_dbase::GetCellRegion( 
    const BBox& bb, 
    int         Level, int& MinX, int& MinY, int& MinZ,
                       int& MaxX, int& MaxY, int& MaxZ ) const
{
    const float W = 1.0f / m_CellWidth[Level];

    MinX = (int)(( bb.min.x * W )+1024.0f)-1024;
    MinY = (int)(( bb.min.y * W )+1024.0f)-1024;
    MinZ = (int)(( bb.min.z * W )+1024.0f)-1024;
    MaxX = (int)(( bb.max.x * W )+1024.0f)-1024;
    MaxY = (int)(( bb.max.y * W )+1024.0f)-1024;
    MaxZ = (int)(( bb.max.z * W )+1024.0f)-1024;
}

inline
uint16_t spatial_dbase::GetCellIndex( int X, int Y, int Z, int Level, bool Create )
{
    // Compute hash entry
    int H = ComputeHash( X, Y, Z, Level );

    // Loop through hash entries and look for a match
    uint16_t ID = m_Hash[ H ].FirstCell;
    while( ID != SPATIAL_CELL_NULL )
    {
        const spatial_cell& Cell = GetCell( ID );

        if( (Cell.X     == X) && 
            (Cell.Y     == Y) && 
            (Cell.Z     == Z) && 
            (Cell.Level == Level) )
        {
            return ID;
        }

        ID = Cell.HashNext;
    }   

    // Could not find cell and caller requested to create one
    if( Create )
    {
        ID = AllocCell( X, Y, Z, Level );
        return ID;
    }

    // Could not find cell and caller did not request to create one
    // so just return
    return SPATIAL_CELL_NULL;
}

inline
void spatial_dbase::UpdateCell( spatial_cell& Cell )
{
    assert( (&Cell >= m_pCell) && (&Cell < (&m_pCell[m_nCellsAllocated])) );

    if( (Cell.OccFlags == 0) && (Cell.Child==SPATIAL_CELL_NULL) )
    {
        int Index = (((uint8_t*)&Cell) - (uint8_t*)m_pCell) / ((int)sizeof(spatial_cell));
        assert( Index >= 0 && Index < m_nCellsAllocated );
        FreeCell( Index );
    }
}
