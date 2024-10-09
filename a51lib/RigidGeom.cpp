#include <cassert>

#include "RigidGeom.h"
#include "InevFile.h"
#include "PlatformDef.h"

void RigidGeom::read(InevFile& inevFile)
{
    Geom::read(inevFile);
    inevFile.read(collision);
    inevFile.align16();
    inevFile.read(numDList);

    switch( platform )
    {
        case PLATFORM_XBOX :
            inevFile.readArray( system.pXbox, numDList );
            break;

        case PLATFORM_PS2 :
            inevFile.readArray( system.pPS2, numDList );
            break;

        case PLATFORM_PC :
            inevFile.readArray( system.pPC, numDList );
            break;
        
        default :
            assert( false );
            break;
    }
}

void RigidGeom::Dlist_PS2::read(InevFile& inevFile)
{
    inevFile.read(numVerts);
    inevFile.readNativeArray(pUV, numVerts * 2);
    inevFile.readNativeArray(pNormal, numVerts * 3);
    inevFile.readNativeArray(pPosition, numVerts);
    inevFile.read(iBone);
    inevFile.read(iColor);
}

void RigidGeom::Vertex_PC::read(InevFile& inevFile)
{
    inevFile.read(pos);
    inevFile.read(normal);
    inevFile.read(colour);
    inevFile.read(uv);
}

void RigidGeom::Dlist_PC::read(InevFile& inevFile)
{
    inevFile.read(numIndices);
    inevFile.readNativeArray(indices, numIndices);
    inevFile.read(numVerts);
    inevFile.readArray(verts, numVerts);
    inevFile.read(iBone);
}

void RigidGeom::Vertex_Xbox::read(InevFile& inevFile)
{
    inevFile.read(pos);
    inevFile.read(packedNormal);
    inevFile.read(uv);
}

void RigidGeom::Dlist_Xbox::read(InevFile& inevFile)
{
    inevFile.read(numIndices);
    inevFile.readNativeArray(indices, numIndices);

    inevFile.read(pushSize);
    inevFile.readNativeArray(pushBuffer, pushSize);
    uint32_t pad;
    inevFile.read(pad);     // unused hPushBuffer

    inevFile.read(numVerts);
    inevFile.readArray(verts, numVerts);
    inevFile.read(pad);     // unused hVert

    inevFile.read(iBone);
    inevFile.read(iColour);

}