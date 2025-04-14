#include <cassert>
#include <iostream>

#include "SkinGeom.h"
#include "InevFile.h"
#include "PlatformDef.h"

void SkinGeom::describe(std::ostringstream& ss)
{
    Geom::describe(ss);
}

void SkinGeom::read(InevFile& inevFile)
{
    Geom::read(inevFile);

    inevFile.skip(4);
    inevFile.read(numDList);

    switch (platform) {
    case PLATFORM_XBOX:
        inevFile.readArray(system.pXbox, numDList);
        break;

    case PLATFORM_PS2:
        inevFile.readArray(system.pPS2, numDList);
        break;

    case PLATFORM_PC:
        inevFile.readArray(system.pPC, numDList);
        break;

    default:
        assert(false);
        break;
    }
}

void SkinGeom::Dlist_PS2::read(InevFile& inevFile)
{
    inevFile.read(numVerts);
    inevFile.readNativeArray(pUV, numVerts * 2);
    inevFile.readNativeArray(pNormal, numVerts * 3);
    inevFile.readNativeArray(pPosition, numVerts);
    inevFile.read(iBone);
    inevFile.read(iColor);
}

void SkinGeom::Vertex_PC::read(InevFile& inevFile)
{
    inevFile.read(pos);
    inevFile.read(normal);
    inevFile.read(UVWeights);
}

void SkinGeom::command_pc::read(InevFile& inevFile)
{
    int iCmd;
    inevFile.read(iCmd);
    Cmd = (command_types_pc)iCmd;
    inevFile.read(Arg1);
    inevFile.read(Arg2);
}

void SkinGeom::Dlist_PC::read(InevFile& inevFile)
{
    inevFile.read(numIndices);
    inevFile.readNativeArray(indices, numIndices);
    inevFile.read(numVerts);
    inevFile.readArray(verts, numVerts);
    inevFile.read(nCommands);
    inevFile.readArray(pCmd, nCommands);
    inevFile.skip(4);
}

void SkinGeom::Vertex_Xbox::read(InevFile& inevFile)
{
    inevFile.read(pos);
    inevFile.read(packedNormal);
    inevFile.read(uv);
}

void SkinGeom::Dlist_Xbox::read(InevFile& inevFile)
{
    inevFile.read(numIndices);
    inevFile.readNativeArray(indices, numIndices);

    inevFile.read(pushSize);
    inevFile.readNativeArray(pushBuffer, pushSize);
    uint32_t pad;
    inevFile.read(pad); // unused hPushBuffer

    inevFile.read(numVerts);
    inevFile.readArray(verts, numVerts);
    inevFile.read(pad); // unused hVert

  //  inevFile.read(iBone);
  //  inevFile.read(iColour);
}
