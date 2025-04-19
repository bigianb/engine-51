#pragma once

#include "DecalDefinition.h"

class decal_package
{
public:
    enum
    {
        DECAL_PACKAGE_VERSION = 0x0004
    };

    //==========================================================================
    // Constructors/destructors
    //==========================================================================
            decal_package       ( void );
            //decal_package       ( fileio& File );
            ~decal_package      ( void );
    //void    FileIO              ( fileio& File );

    //==========================================================================
    // Get/Set functions
    //==========================================================================
    int                 GetNGroups      ( void                  )  const;
    const char*         GetGroupName    ( int         iGroup    )  const;
    void                SetGroupName    ( int         iGroup,
                                          const char* Name      );
    Colour              GetGroupColor   ( int         iGroup    )  const;
    void                SetGroupColor   ( int         iGroup, 
                                          Colour      Color     );
    int                 GetNDecalDefs   ( void                  )  const;
    decal_definition&   GetDecalDef     ( int         iDecalDef )  const;
    int                 GetNDecalDefs   ( int         iGroup    )  const;
    decal_definition&   GetDecalDef     ( int         iGroup,
                                          int         iDecalDef )  const;
    decal_definition*   GetDecalDef     ( const char* Name,
                                          int         iDecalDef )  const;
        
protected:
    //==========================================================================
    // Internal structures
    //==========================================================================
    struct group
    {
        //void    FileIO  ( fileio& File );

        char    Name[32];
        Colour  Color;
        int     nDecalDefs;
        int     iDecalDef;
    };

    //==========================================================================
    // Decal data
    //==========================================================================
    int                 m_Version;
    int                 m_nGroups;
    group*              m_pGroups;
    int                 m_nDecalDefs;
    decal_definition*   m_pDecalDefs;
};

//==============================================================================
// Inlines
//==============================================================================

inline int decal_package::GetNGroups( void ) const
{
    return m_nGroups;
}

//==============================================================================

inline const char* decal_package::GetGroupName( int iGroup ) const
{
    assert( (iGroup>=0) && (iGroup < m_nGroups) );
    return m_pGroups[iGroup].Name;
}

//==============================================================================

inline void decal_package::SetGroupName( int iGroup, const char* Name )
{
    assert( (iGroup>=0) && (iGroup < m_nGroups) );
    strncpy( m_pGroups[iGroup].Name, Name, 32 );
}

//==============================================================================

inline Colour decal_package::GetGroupColor( int iGroup )  const
{
    assert( (iGroup>=0) && (iGroup < m_nGroups) );
    return m_pGroups[iGroup].Color;
}

//==============================================================================

inline void decal_package::SetGroupColor( int iGroup, Colour Color )
{
    assert( (iGroup>=0) && (iGroup < m_nGroups) );
    m_pGroups[iGroup].Color = Color;
}

//==============================================================================

inline int decal_package::GetNDecalDefs( void ) const
{
    return m_nDecalDefs;
}

//==============================================================================

inline decal_definition& decal_package::GetDecalDef( int iDecalDef ) const
{
    assert( (iDecalDef>=0) && (iDecalDef < m_nDecalDefs) );
    return m_pDecalDefs[iDecalDef];
}

//==============================================================================

inline int decal_package::GetNDecalDefs( int iGroup ) const
{
    assert( (iGroup>=0) && (iGroup < m_nGroups) );
    return m_pGroups[iGroup].nDecalDefs;
}

//==============================================================================

inline decal_definition& decal_package::GetDecalDef( int iGroup, int iDecalDef ) const
{
    assert( (iGroup>=0) && (iGroup < m_nGroups) );
    assert( (iDecalDef>=0) && (iDecalDef < m_pGroups[iGroup].nDecalDefs) );
    return m_pDecalDefs[m_pGroups[iGroup].iDecalDef+iDecalDef];
}

//==============================================================================

inline decal_definition* decal_package::GetDecalDef( const char* Name,
                                                     int         iDecalDef )  const
{
    for (int i = 0; i < m_nGroups; i++ )
    {
        if ( !strcmp( GetGroupName(i), Name ) )
        {
            if ( (iDecalDef>=0) && (iDecalDef<m_pGroups[i].nDecalDefs) )
                return &m_pDecalDefs[m_pGroups[i].iDecalDef+iDecalDef];
        }
    }

    return nullptr;
}
