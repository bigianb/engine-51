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
    if (inevFile.findOffsetForPtr(56) == 224){
        readXboxDemo(inevFile);
        return;

    }
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
    inevFile.skip(4);           // skip mesh reading for now. Just contains a BBox

    int numNames = inevFile.readInt();
    inevFile.skip(4);                   // pointer to a string

    numSubMeshes = inevFile.readInt();
    inevFile.skip(4);                   // each submesh looks like 4 ints.

    numMaterials = inevFile.readInt();
    inevFile.skip(4);                   // need to figure materials out

    numTextures = inevFile.readInt();
    inevFile.skip(4);
    inevFile.skip(16);

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

        // File offset 0x174 (16 bytes each element)
        int[4]             // 352  0,0,0,0
        int[4]                     1, 1, 32, 0
        ------------

        // File offset 0x194        // materials
        int             // 384      3

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
            auto& dlist = system.pXbox[i];
            for (int j = 0; j < dlist.numIndices; ++j) {
                auto& v = dlist.verts[dlist.indices[j]];
                *pf++ = v.pos.x;
                *pf++ = v.pos.y;
                *pf++ = v.pos.z;
            }
        }
        for (int i = 0; i < numDList; ++i) {
            auto& dlist = system.pXbox[i];
            for (int j = 0; j < dlist.numIndices; ++j) {
                auto& v = dlist.verts[dlist.indices[j]];
                *pf++ = v.uv.x;
                *pf++ = v.uv.y;
            }
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
