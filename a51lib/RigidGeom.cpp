#include <cassert>

#include "RigidGeom.h"
#include "InevFile.h"
#include "PlatformDef.h"

void RigidGeom::describe(std::ostringstream& ss)
{
    Geom::describe(ss);
    
}

void RigidGeom::read(InevFile& inevFile)
{
    Geom::read(inevFile);
    if (version != 41){
        // TODO: figure out other versions
        return;
    }
    inevFile.read(collision);
    inevFile.align16();
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

int RigidGeom::getNumVertices()
{
    int numVertices = 0;
    switch (platform) {
    case PLATFORM_XBOX:
    {
        for (int i = 0; i < numDList; ++i) {
            numVertices += system.pXbox[i].numIndices;
        }
    } break;

    case PLATFORM_PS2:
    {
        for (int i = 0; i < numDList; ++i) {
            numVertices += system.pPS2[i].numVerts;
        }
    } break;

    case PLATFORM_PC:
    {
        for (int i = 0; i < numDList; ++i) {
            numVertices += system.pPC[i].numIndices;
        }
    } break;

    default:
        break;
    }
    return numVertices;
}

float* RigidGeom::getVerticesPUV()
{
    int          num = getNumVertices();
    float* const output = new float[num * 5];
    float*       pf = output;
    switch (platform) {
    case PLATFORM_XBOX:
    {
        for (int i = 0; i < numDList; ++i) {
        }
    } break;

    case PLATFORM_PS2:
    {
        // Are these strips?
        for (int i = 0; i < numDList; ++i) {
            auto& dlist = system.pPS2[i];
            for (int j = 0; j < dlist.numVerts; ++j) {
                Vector4& v = dlist.pPosition[j];
                *pf++ = v.x;
                *pf++ = v.y;
                *pf++ = v.z;
            }
        }
        for (int i = 0; i < numDList; ++i) {
            auto&    dlist = system.pPS2[i];
            int16_t* puv = dlist.pUV;
            for (int j = 0; j < dlist.numVerts; ++j) {
                *pf++ = (float)*puv++;      // TODO: Fixed point 4.12
                *pf++ = (float)*puv++;
            }
        }
    } break;

    case PLATFORM_PC:
    {
        for (int i = 0; i < numDList; ++i) {
            auto& dlist = system.pPC[i];
            for (int j = 0; j < dlist.numIndices; ++j) {
                auto& v = dlist.verts[dlist.indices[j]];
                *pf++ = v.pos.x;
                *pf++ = v.pos.y;
                *pf++ = v.pos.z;
            }
        }
        for (int i = 0; i < numDList; ++i) {
            auto& dlist = system.pPC[i];
            for (int j = 0; j < dlist.numIndices; ++j) {
                auto& v = dlist.verts[dlist.indices[j]];
                *pf++ = v.uv.x;
                *pf++ = v.uv.y;
            }
        }
    } break;

    default:
        break;
    }
    return output;
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
    inevFile.skip(4);
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
    inevFile.read(pad); // unused hPushBuffer

    inevFile.read(numVerts);
    inevFile.readArray(verts, numVerts);
    inevFile.read(pad); // unused hVert

    inevFile.read(iBone);
    inevFile.read(iColour);
}
