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
        uint32_t  nIndices;
        uint16_t* pIndices;

        uint32_t nPushSize;
        uint8_t* pPushBuffer;
        void*    hPushBuffer;

        uint32_t     nVerts;
        Vertex_Xbox* pVert;
        void*        hVert;

        uint32_t iBone;
        uint32_t iColor; // Index into color table

        void read(InevFile&);
    };

    struct Dlist_PS2
    {
        uint32_t nVerts;
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
        uint32_t  nIndices;
        uint16_t* pIndices;

        uint32_t   nVerts;
        Vertex_PC* pVert;

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
    int32_t       nDList;
    SystemPtr     system;
};
