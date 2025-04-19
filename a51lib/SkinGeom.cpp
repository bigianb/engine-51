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

// TODO: This could be in base class
int SkinGeom::getNumVertices(int meshNo)
{
    const auto& mesh = meshes[meshNo];
    int         numVertices = 0;

    for (int i = 0; i < mesh.nSubMeshes; ++i) {
        numVertices += getNumSubmeshVertices(i + mesh.iSubMesh);
    }
    return numVertices;
}

int SkinGeom::getNumSubmeshVertices(int submeshNo)
{
    const auto& submesh = subMeshes[submeshNo];
    const int   dlistIdx = submesh.iDList;

    switch (platform) {
    case PLATFORM_XBOX:
    {
        return system.pXbox[dlistIdx].numIndices;
    } break;

    case PLATFORM_PS2:
    {
        // Num tri-strips
        // Need to look at ADC flags really.
        return (system.pPS2[dlistIdx].numVerts - 2) * 3;
    } break;

    case PLATFORM_PC:
    {
        return system.pPC[dlistIdx].numIndices;
    } break;

    default:
        break;
    }

    return 0;
}

float* SkinGeom::getVerticesPUV(int meshNo)
{
    int          num = getNumVertices(meshNo);
    float* const output = new float[num * 5];
    float*       pf = output;
    float*       puv = pf + num * 3;

    const auto& mesh = meshes[meshNo];
    for (int i = 0; i < mesh.nSubMeshes; ++i) {
        const auto& submesh = subMeshes[i + mesh.iSubMesh];
        float*      pf1 = getPUVHelper(submesh, pf, puv);
        puv += ((pf1 - pf) / 3) * 2;
        pf = pf1;
    }
    if (pf - output != num * 3) {
        std::cerr << "ERROR: unexpected vertex count" << std::endl;
    }
    return output;
}

float* SkinGeom::getSubmeshVerticesPUV(int submeshIdx)
{
    const auto&  submesh = subMeshes[submeshIdx];
    int          num = getNumSubmeshVertices(submeshIdx);
    float* const output = new float[num * 5];
    float*       pf = output;
    float*       puv = pf + num * 3;
    getPUVHelper(submesh, pf, puv);
    return output;
}

float* SkinGeom::getPUVHelper(const Submesh& submesh, float* pf, float* puv)
{
    const int dlistIdx = submesh.iDList;
    switch (platform) {
    case PLATFORM_XBOX:
    {
        auto& dlist = system.pXbox[dlistIdx];
        for (int j = 0; j < dlist.numIndices; ++j) {
            auto& v = dlist.verts[dlist.indices[j]];
            *pf++ = v.pos.x;
            *pf++ = v.pos.y;
            *pf++ = v.pos.z;
        }
        for (int j = 0; j < dlist.numIndices; ++j) {
            auto& v = dlist.verts[dlist.indices[j]];
            *puv++ = v.uv.x;
            *puv++ = v.uv.y;
        }
    } break;

    case PLATFORM_PS2:
    {
        // Tri strips so 0,1,2 1,2,3 2,3,4 etc
        auto& dlist = system.pPS2[dlistIdx];
        for (int j = 2; j < dlist.numVerts; ++j) {
            Vector4& v2 = dlist.pPosition[j];
            uint32_t iw = *(uint32_t *)(&v2.w);
            bool isCCW = (iw & 0x20) == 0x20;
            bool isADC = (iw & 0x8000) == 0x8000;
            if (!isADC){
                Vector4& v1 = dlist.pPosition[j-1];
                Vector4& v0 = dlist.pPosition[j-2];
                if (!isCCW){
                    v0 = dlist.pPosition[j];
                    v2 = dlist.pPosition[j-2];
                }
                *pf++ = v0.x;
                *pf++ = v0.y;
                *pf++ = v0.z;
                *pf++ = v1.x;
                *pf++ = v1.y;
                *pf++ = v1.z;
                *pf++ = v2.x;
                *pf++ = v2.y;
                *pf++ = v2.z;
            }
        }
        int16_t* puv = dlist.pUV;
        for (int j = 0; j < dlist.numVerts; ++j) {
            int16_t* v = dlist.pUV + j*2;
            *puv++ = ((float)v[0]) / 4096.0; // Fixed point 4.12
            *puv++ = ((float)v[1]) / 4096.0;
        }
    } break;

    case PLATFORM_PC:
    {
        auto& dlist = system.pPC[dlistIdx];
        for (int j = 0; j < dlist.numIndices; ++j) {
            auto& v = dlist.verts[dlist.indices[j]];
            *pf++ = v.pos.x;
            *pf++ = v.pos.y;
            *pf++ = v.pos.z;
        }
        for (int j = 0; j < dlist.numIndices; ++j) {
            auto& v = dlist.verts[dlist.indices[j]];
            *puv++ = v.UVWeights.x;
            *puv++ = v.UVWeights.y;
        }
    } break;

    default:
        break;
    }
    return pf;
}

float* SkinGeom::getSubmeshVertexNormals(int submeshIdx)
{
    const auto&  submesh = subMeshes[submeshIdx];
    int          num = getNumSubmeshVertices(submeshIdx);
    float* const output = new float[num * 3];
    float*       pf = output;
    const int    dlistIdx = submesh.iDList;
    switch (platform) {
    case PLATFORM_XBOX:
    {
        auto& dlist = system.pXbox[dlistIdx];
        for (int j = 0; j < dlist.numIndices; ++j) {
            auto&    v = dlist.verts[dlist.indices[j]];
            uint32_t iz = (v.packedNormal >> 22) & 0x3ff;
            if (iz > 0x1FF) {
                iz |= 0xFFFFFC00;
            }
            uint32_t iy = (v.packedNormal >> 11) & 0x7ff;
            if (iy > 0x3FF) {
                iy |= 0xFFFFF800;
            }
            uint32_t ix = (v.packedNormal) & 0x7ff;
            if (ix > 0x3FF) {
                ix |= 0xFFFFF800;
            }

            float x = (int32_t)ix / 1023.0;
            float y = (int32_t)iy / 1023.0;
            float z = (int32_t)iz / 511.0;

            *pf++ = x;
            *pf++ = y;
            *pf++ = z;
        }
    } break;

    case PLATFORM_PS2:
    {
        auto& dlist = system.pPS2[dlistIdx];
        for (int j = 2; j < dlist.numVerts; ++j) {
            int8_t* n = dlist.pNormal + j*3;
            *pf++ = (float)n[0] / 127.0;
            *pf++ = (float)n[1] / 127.0;
            *pf++ = (float)n[2] / 127.0;  
        }
    } break;

    case PLATFORM_PC:
    {
        auto& dlist = system.pPC[dlistIdx];
        for (int j = 0; j < dlist.numIndices; ++j) {
            auto& v = dlist.verts[dlist.indices[j]];
            *pf++ = v.normal.x;
            *pf++ = v.normal.y;
            *pf++ = v.normal.z;
        }
    } break;
    }
    return output;
}
