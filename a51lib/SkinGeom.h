#pragma once

#include <cstdint>
#include <sstream>
#include "Geom.h"
#include "CollisionVolume.h"

class SkinGeom : public Geom
{
public:
    struct Vertex_Xbox
    {
        Vector3p pos;
        uint32_t packedNormal;
        Vector2  uv;
        Vector2  weights;
        Vector2  bones;

        void read(InevFile&);
    };

    enum command_types_xbox
    {
        XBOX_CMD_NULL,
        XBOX_CMD_UPLOAD_MATRIX, // Arg1 = BoneID, Arg2 = CacheID
        XBOX_CMD_DRAW_SECTION,  // Arg1 = Start,  Arg2 = End
        XBOX_CMD_END = 0xffffffff
    };

    struct command_xbox
    {
        command_types_xbox Cmd;
        uint32_t           Arg1;
        uint32_t           Arg2;
    };

    struct Dlist_Xbox
    {
        uint32_t  numIndices;
        uint16_t* indices;

        uint32_t pushSize;
        uint8_t* pushBuffer;
        void*    hPushBuffer;

        uint32_t     numVerts;
        Vertex_Xbox* verts;
        void*        hVert;

        uint32_t      nCommands;
        command_xbox* pCmd;

        void* pOpt;

        void read(InevFile&);
    };

    struct Dlist_PS2
    {
        uint32_t numVerts;
        int16_t* pUV;       // 2*VertIndex
        int8_t*  pNormal;   // 3*VertIndex
        Vector4* pPosition; // 1:1

        uint32_t iBone;
        uint32_t iColor; // Index into color table

        void read(InevFile&);
    };

    struct Vertex_PC
    {
        Vector4 pos;
        Vector4 normal;
        Vector4 UVWeights;

        void read(InevFile&);
    };

    enum command_types_pc
    {
        PC_CMD_NULL,
        PC_CMD_UPLOAD_MATRIX, // Arg1 = BoneID, Arg2 = CacheID
        PC_CMD_DRAW_SECTION,  // Arg1 = Start,  Arg2 = End
        PC_CMD_END = 0xffffffff
    };

    struct command_pc
    {
        command_types_pc Cmd;
        int16_t          Arg1;
        int16_t          Arg2;

        void read(InevFile&);
    };

    struct Dlist_PC
    {
        uint32_t  numIndices;
        uint16_t* indices;

        uint32_t   numVerts;
        Vertex_PC* verts;

        uint32_t    nCommands;
        command_pc* pCmd;

        void read(InevFile&);
    };

    union SystemPtr
    {
        Dlist_Xbox* pXbox;
        Dlist_PS2*  pPS2;
        Dlist_PC*   pPC;
    };

    void read(InevFile&);

    void describe(std::ostringstream& ss);

    // Gets the number of vertices returned by getVerticesPUV.
    int getNumVertices(int meshNo);

    // Gets the de-indexed vertices (so can contain duplicates) in x,y,z,u,v format.
    float* getVerticesPUV(int meshNo);

    // Gets the number of vertices returned by getSubmeshVerticesPUV.
    int getNumSubmeshVertices(int submeshNo);

    // Gets the de-indexed vertices (so can contain duplicates) in x,y,z,u,v format.
    float* getSubmeshVerticesPUV(int submeshNo);

    float* getSubmeshVertexNormals(int submeshIdx);

    float* getPUVHelper(const Submesh& submesh, float* pf, float* puv);

    // Only needed when we change something. Used when reading the xbox demo meshes.
    void calcMeshBBoxes();

    CollisionData collision;
    int32_t       numDList;
    SystemPtr     system;
};
