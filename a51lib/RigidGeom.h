#pragma once

#include <cstdint>
#include <sstream>
#include "Geom.h"
#include "CollisionVolume.h"

class RigidGeom : public Geom
{
public:
    struct Vertex_Xbox
    {
        Vector3p pos;
        uint32_t packedNormal;
        Vector2  uv;

        void read(InevFile&);
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

        uint32_t iBone;
        uint32_t iColour; // Index into color table

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
        Vector3p pos;
        Vector3p normal;
        Colour   colour;
        Vector2  uv;

        void read(InevFile&);
    };

    struct Dlist_PC
    {
        uint32_t  numIndices;
        uint16_t* indices;

        uint32_t   numVerts;
        Vertex_PC* verts;

        uint32_t iBone;

        void read(InevFile&);
    };

    union SystemPtr
    {
        Dlist_Xbox* pXbox;
        Dlist_PS2*  pPS2;
        Dlist_PC*   pPC;
    };

    void read(InevFile&);

    CollisionData collision;
    int32_t       numDList;
    SystemPtr     system;
};
