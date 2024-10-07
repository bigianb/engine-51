#include "RigidGeom.h"
#include "InevFile.h"

void RigidGeom::read(InevFile& inevFile)
{
    Geom::read(inevFile);
    inevFile.read(collision);

/*
    File.Static( m_Collision );
    File.Static( m_nDList );

    switch( m_Platform )
    {
        case PLATFORM_XBOX :
            File.Static( m_System.pXbox, m_nDList );
            break;

        case PLATFORM_PS2 :
            File.Static( m_System.pPS2, m_nDList );
            break;

        case PLATFORM_PC :
            File.Static( m_System.pPC, m_nDList );
            break;
        
        default :
            ASSERT( 0 );
            break;
    }
*/
}
