#include <cassert>
#include <iostream>

#include "RigidGeom.h"
#include "InevFile.h"
#include "PlatformDef.h"

void RigidGeom::describe(std::ostringstream& ss)
{
    Geom::describe(ss);
}

void RigidGeom::read(InevFile& inevFile)
{
    if (inevFile.findOffsetForPtr(56) == 224) {
        readXboxDemo(inevFile);
        return;
    }
    Geom::read(inevFile);
    if (version != 41) {
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

void RigidGeom::readXboxDemo(InevFile& inevFile)
{
    // This could be better in the base class - but for now at least I think
    // it is easier to keep the demo stuff in one place.
    inevFile.read(bbox);
    inevFile.skip(4);
    platform = inevFile.readInt();
    inevFile.setPlatform(platform);

    version = inevFile.readInt();
    numFaces = inevFile.readInt();
    numVertices = inevFile.readInt();
    numMeshes = inevFile.readInt();
    meshes = new Mesh[numMeshes];
    int meshesOffset = inevFile.readAndResolvePtr();
    int saved = inevFile.getCursor();
    inevFile.setCursor(meshesOffset);
    for (int i = 0; i < numMeshes; ++i) {
        auto& mesh = meshes[i];
        inevFile.read(mesh.bbox);
        mesh.nSubMeshes = 0;
        mesh.iSubMesh = 0;
        mesh.nBones = 0;
        mesh.nFaces = 0;
        mesh.nVertices = 0;
    }
    inevFile.setCursor(saved);

    int numNames = inevFile.readInt();
    inevFile.skip(4); // pointer to a string

    numSubMeshes = inevFile.readInt();
    subMeshes = new Submesh[numSubMeshes];
    int submeshesOffset = inevFile.readAndResolvePtr();
    saved = inevFile.getCursor();
    inevFile.setCursor(submeshesOffset);
    // each submesh looks like 4 ints.
    for (int i = 0; i < numSubMeshes; ++i) {
        auto& submesh = subMeshes[i];
        submesh.iDList = inevFile.readInt();
        submesh.iMaterial = inevFile.readInt();
        submesh.worldPixelSize = 0.0;
        int extra1 = inevFile.readInt();
        int extra2 = inevFile.readInt();

        std::cout << "submesh extra1: " << extra1 << " ( as float= " << *((float*)&extra1) << ") 0x" << std::hex << extra1 << std::dec << std::endl;
        std::cout << "submesh extra2: " << extra2 << " ( as float= " << *((float*)&extra2) << ") 0x" << std::hex << extra2 << std::dec << std::endl;
        std::cout << std::endl;
    }
    // Assume there is aways 1 mesh and it has all the subMeshes.
    meshes[0].nSubMeshes = numSubMeshes;
    numMeshes = 1;

    inevFile.setCursor(saved);

    numMaterials = inevFile.readInt();
    int materialsOffset = inevFile.readAndResolvePtr();
    saved = inevFile.getCursor();
    inevFile.setCursor(materialsOffset);

    // 100 bytes per material
    // For now, just point to textures
    std::cout << "Found " << numMaterials << " materials: " << std::endl;
    materials = new Material[numMaterials];
    for (int i = 0; i < numMaterials; ++i) {
        for (int x=0; x<25; ++x){
            // flags, nTextures, iTexture...
            int v;
            inevFile.read(v);
            if (x == 1){
                materials[i].nTextures = v;
            } else if (x == 2){
                materials[i].iTexture = v;
            }
            
            if (x >= 6 && x <= 10){
                std::cout << *(float *)&v;
            } else {
                std::cout << v;
            }
            if (x != 24){
                std::cout << ", ";
            } else {
                std::cout << std::endl;
            }
        }
    }
    inevFile.setCursor(saved);

    numTextures = inevFile.readInt();
    int texturesOffset = inevFile.readAndResolvePtr();
    saved = inevFile.getCursor();
    inevFile.setCursor(texturesOffset);
    textures = new Texture[numTextures];
    for (int t = 0; t < numTextures; ++t) {
        textures[t].fileName = inevFile.getStrData();
        texturesOffset += 256;
        inevFile.setCursor(texturesOffset);
    }
    inevFile.setCursor(saved);

    inevFile.skip(8);
    int numUnknown = inevFile.readInt();
    int unknownOfffset = inevFile.readAndResolvePtr();
    std::cout << "Found " << numUnknown << " unknown elements: " << std::endl;
    saved = inevFile.getCursor();
    inevFile.setCursor(submeshesOffset);
    // each unknown looks like 4 ints.
    // Maybe materials.
    for (int i = 0; i < numUnknown; ++i) {
        int i1 = inevFile.readInt();
        int i2 = inevFile.readInt();
        int i3 = inevFile.readInt();
        int i4 = inevFile.readInt();
        std::cout << i1 << ", " << i2 << ", " << i2 << ", " << i4 << std::endl;
    }
    std::cout << std::endl;
    inevFile.setCursor(saved);

    /*
    Example from blue_col_offcol_4x8_001.RIGIDGEOM

    Pointer resolutions:
        56 -> 224, flags = 3
        64 -> 256, flags = 3
        72 -> 352, flags = 3
        80 -> 384, flags = 3
        88 -> 608, flags = 3
        104 -> 1376, flags = 3
        148 -> 1408, flags = 3
        156 -> 1504, flags = 3
        168 -> 1568, flags = 3
        172 -> 1632, flags = 3
        176 -> 1856, flags = 3
        196 -> 1920, flags = 3
        1924 -> 2016, flags = 3
        1932 -> 2112, flags = 3
        1944 -> 0, flags = 1
        1964 -> 2784, flags = 3
        1972 -> 2880, flags = 3
        1984 -> 576, flags = 1
    */
    /*
         BBox        // 0 - 31
         int unk     // 32
         int plat    // 36
         int ver     // 40
         int         // 44   32      // num faces maybe
         int         // 48   48      // num vertices

         int         // 52   // len (1)  // num meshes
         ptr         // 56    -> 224

         int         // 60   len (1)
         ptr         // 64   -> 256 points to a string

         int         // 68   len (2)     // num submeshes
         ptr         // 72  -> 352 (points to file offset 0x174)

         // File off 0x60
         int         // 76   len (2)     // num materials
         ptr         // 80   -> 384

         int         numTextures // 84   len (3)
         ptr         textures    // 88   -> 608

         int             // 92
         ptr (unused)    // 96

         int         // 100  len (2)
         ptr         // 104 -> 1376
 */

    // Looks like the rigidgeom part stays the same
    inevFile.skip(4);
    inevFile.read(collision);
    /*
            float       // 108      // pad?
    CollisionData
            // file off 0x80
            // BBox

            float       // 112      -122.65
            float       // 116      -0
            float       // 120      -122.65

            float       // 124
            float       // 128      122.65
            float       // 132      400
            float       // 136      122.65

            // file off 0xA0
            float       // 140
            int         // 144      len (2)
            ptr         // 148      -> 1408

            int         // 152      len (32)
            ptr         // 156      -> 1504

            short[4]    // 160 - 167    (1, 14, 6, 0x7FFF)
            ptr         // 168      -> 1568

            // offset 0xC0
            ptr         // 172      -> 1632     pLowCluster (0x14 of them)
            ptr         // 176      -> 1856     pLowQuad
            int         // 180      0
            int         // 184      0
    end of collision data
    */

    inevFile.align16();
    inevFile.read(numDList);
    inevFile.readArray(system.pXbox, numDList);
    calcMeshBBoxes();
    /*
            int         // 188      0
            int         // 192  len (2)         m_nDList
            ptr         // 196      -> 1920     m_system


            -------
            mesh
            // File offset 0xF4
            float            // 224     -122.75
            float            // 228     -0.1
            float            // 232     -122.75
            float                       0

            float           // 240      122.75
            float           // 244      400.1
            float           // 248      122.75
            float           // 252      0

            ---------

            // File offset 0x114
            char[]          // 256 "Blue_col_offcol_4x8_001"

            ----------

            // File offset 0x174 (16 bytes each element) - submeshes
            int[4]             // 352  0,0,0,0
            int[4]                     1, 1, 32, 0
            ------------

            // File offset 0x194        // materials (74 bytes each)

            3, 1,            // 0x194
            0, 1, 0, 0,
            2.0, 0, 1.0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            0, 0, 0, 0,
            1, 0, 0,

            3, 2,                // 1f8
            1, 1, 0, 1          // 200
            2.0, 0, 1.0, 0      // 210
            0, 0, 0, 0          // 220
            0, 0, 0, 0          // 230
            0, 0, 0, 0          // 240
            1, 1, 0,

            0
            0, 0, 0, 0
            0


            -----------

            // File offset 0x274 (608) - texture
            char[3][256]        "Blue_hosp_panel_flat_0000.xbmp"
                                "Blue_wall_groove_diff_0000.xbmp"
                                "DET_steel_0000.xbmp"

            --------------

            // File offset 0x574. 2 elements of 16 bytes each
            int[4]             // 1376  0,0,0,0
            int[4]                      0,0,0,0

            --------------

            // File offset 0x594 (1408), 2 elements
            float[4]    // 1408      -122.65, 0, -122.65, 0
            float[4]                 122.65, 400, 122.65, 0
            int         // 1440     0x10
            int         // 1444     0
            int         // 1448     0
            int         // 1452     7

            float[4]                -122.65, 0, -122.65, 0
            float[4]                122.65, 400, 122.65, 0
            int                     0x10
            int                     0x00010000
            int                     0x10
            int                     0x4

            ----------------

            File offset 0x5f4 (1504) 32 elements.
            short[32]

            -----------------

            File offset 0x634 (1568)

            -----------------

            File Offset 0x674 (1632)

            Vertices (vec3 but with vec4 size) (20 of them)
            -----------------

            File Offset 0x754 (1856)

            Arrray of bytes for vertex indices (maybe 35 or 36)

            ----------------- dlist

            File Offset 0x794 (1920)
            2 elements
            int     // 1920     48      // nIndices (16 triangles)
            short*  // 1924     -> 2016
            int     // 1928     664     // push size
            ptr     // 1932     -> 2112
            int     // 1936     0
                    // 1940     24      // nVerts
                    // 1944 -> 0 (this)     24 els of 24 bytes
                    // 1948    0
                    // 1952    0
                    // 1956    0
    file off 7bc
            int     // 1960     48
                    // 1964 (7c0)-> 2784 (0xAf4)
            int     // 1968     664
                    // 1972     -> 2880 (0xb54)
                    // 1976    0
                    // 1980    24
                    // 1984     -> 576
                    // 1988    0
                    // 1992    0
    File 7e0        // 1996    24

                   0 padding

            ----------------------
            File offset 0x7F4 (2016)

            short[]         // 48 elements long
                            // 0, 1, 2, 2, 3, 0, 4, 5, 6, 0, 6, 7...

            ----------------------
            File offset 0x854 (2112)



       */
}

int RigidGeom::getNumSubmeshVertices(int submeshNo)
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
        return system.pPS2[dlistIdx].numVerts;
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

int RigidGeom::getNumVertices(int meshNo)
{
    const auto& mesh = meshes[meshNo];
    int         numVertices = 0;

    for (int i = 0; i < mesh.nSubMeshes; ++i) {
        numVertices += getNumSubmeshVertices(i + mesh.iSubMesh);
    }
    return numVertices;
}

void RigidGeom::calcMeshBBoxes()
{
    for (int m = 0; m < numMeshes; ++m) {
        auto& mesh = meshes[m];
        mesh.bbox.reset();
        for (int i = 0; i < mesh.nSubMeshes; ++i) {
            const auto& submesh = subMeshes[i + mesh.iSubMesh];
            const int   dlistIdx = submesh.iDList;
            switch (platform) {
            case PLATFORM_XBOX:
            {
                auto& dlist = system.pXbox[dlistIdx];
                for (int j = 0; j < dlist.numIndices; ++j) {
                    auto& v = dlist.verts[dlist.indices[j]];
                    if (v.pos.x > mesh.bbox.max.x) {
                        mesh.bbox.max.x = v.pos.x;
                    }
                    if (v.pos.y > mesh.bbox.max.y) {
                        mesh.bbox.max.y = v.pos.y;
                    }
                    if (v.pos.z > mesh.bbox.max.z) {
                        mesh.bbox.max.z = v.pos.z;
                    }
                    if (v.pos.x < mesh.bbox.min.x) {
                        mesh.bbox.min.x = v.pos.x;
                    }
                    if (v.pos.y < mesh.bbox.min.y) {
                        mesh.bbox.min.y = v.pos.y;
                    }
                    if (v.pos.z < mesh.bbox.min.z) {
                        mesh.bbox.min.z = v.pos.z;
                    }
                }

            } break;

            case PLATFORM_PS2:
            {
                // Are these strips?
                auto& dlist = system.pPS2[dlistIdx];
                for (int j = 0; j < dlist.numVerts; ++j) {
                    Vector4& v = dlist.pPosition[j];
                    if (v.x > mesh.bbox.max.x) {
                        mesh.bbox.max.x = v.x;
                    }
                    if (v.y > mesh.bbox.max.y) {
                        mesh.bbox.max.y = v.y;
                    }
                    if (v.z > mesh.bbox.max.z) {
                        mesh.bbox.max.z = v.z;
                    }
                    if (v.x < mesh.bbox.min.x) {
                        mesh.bbox.min.x = v.x;
                    }
                    if (v.y < mesh.bbox.min.y) {
                        mesh.bbox.min.y = v.y;
                    }
                    if (v.z < mesh.bbox.min.z) {
                        mesh.bbox.min.z = v.z;
                    }
                }
            } break;

            case PLATFORM_PC:
            {
                auto& dlist = system.pPC[dlistIdx];
                for (int j = 0; j < dlist.numIndices; ++j) {
                    auto& v = dlist.verts[dlist.indices[j]];
                    if (v.pos.x > mesh.bbox.max.x) {
                        mesh.bbox.max.x = v.pos.x;
                    }
                    if (v.pos.y > mesh.bbox.max.y) {
                        mesh.bbox.max.y = v.pos.y;
                    }
                    if (v.pos.z > mesh.bbox.max.z) {
                        mesh.bbox.max.z = v.pos.z;
                    }
                    if (v.pos.x < mesh.bbox.min.x) {
                        mesh.bbox.min.x = v.pos.x;
                    }
                    if (v.pos.y < mesh.bbox.min.y) {
                        mesh.bbox.min.y = v.pos.y;
                    }
                    if (v.pos.z < mesh.bbox.min.z) {
                        mesh.bbox.min.z = v.pos.z;
                    }
                }

            } break;

            default:
                break;
            }
        }
    }
}

float* RigidGeom::getPUVHelper(const Submesh& submesh, float* pf, float* puv)
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
        // Are these strips?
        auto& dlist = system.pPS2[dlistIdx];
        for (int j = 0; j < dlist.numVerts; ++j) {
            Vector4& v = dlist.pPosition[j];
            *pf++ = v.x;
            *pf++ = v.y;
            *pf++ = v.z;
        }
        int16_t* puv = dlist.pUV;
        for (int j = 0; j < dlist.numVerts; ++j) {
            *puv++ = (float)*puv++; // TODO: Fixed point 4.12
            *puv++ = (float)*puv++;
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
            *puv++ = v.uv.x;
            *puv++ = v.uv.y;
        }
    } break;

    default:
        break;
    }
    return pf;
}

float* RigidGeom::getSubmeshVerticesPUV(int submeshIdx)
{
    const auto&  submesh = subMeshes[submeshIdx];
    int          num = getNumSubmeshVertices(submeshIdx);
    float* const output = new float[num * 5];
    float*       pf = output;
    float*       puv = pf + num * 3;
    getPUVHelper(submesh, pf, puv);
    return output;
}

float* RigidGeom::getVerticesPUV(int meshNo)
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
