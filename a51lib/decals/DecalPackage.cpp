#include "DecalPackage.h"

/*
void decal_package::group::FileIO( fileio& File )
{
    File.Static( Name, 32 );
    File.Static( Color );
    File.Static( nDecalDefs );
    File.Static( iDecalDef );
}
*/
//==============================================================================

decal_package::decal_package(  ) :
    m_Version( DECAL_PACKAGE_VERSION ),
    m_nGroups( 0 ),
    m_pGroups( nullptr ),
    m_nDecalDefs( 0 ),
    m_pDecalDefs( nullptr )
{
}

//==============================================================================
/*
decal_package::decal_package( fileio& File )
{
    (void)File;

    if ( m_Version != DECAL_PACKAGE_VERSION )
    {
        x_throw( xfs( "Decal package versions do not match. App wants %d, package is %d", DECAL_PACKAGE_VERSION, m_Version ) );
        decal_package();
        return;
    }

    ForceDecalLoaderLink();
}
*/
//==============================================================================

decal_package::~decal_package(  )
{
}

//==============================================================================
/*
void decal_package::FileIO( fileio& File )
{
    File.Static( m_Version );
    File.Static( m_nGroups );
    File.Static( m_pGroups, m_nGroups );
    File.Static( m_nDecalDefs );
    File.Static( m_pDecalDefs, m_nDecalDefs );
}
*/
