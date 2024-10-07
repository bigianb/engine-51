
#pragma once

#include <cstdint>
#include <sstream>

#include "Geom.h"

class InevFile;

struct CollisionData
{
    void read(InevFile&);

    CollisionData();
    ~CollisionData();

    struct MatInfo
    {
        enum
        {
            FLAG_DOUBLESIDED = (1 << 0), // Material is double sided
            FLAG_TRANSPARENT = (1 << 1), // Material is transparent
        };

        uint16_t soundType;
        uint16_t flags;

        void read(InevFile&);
    };

    struct HighCluster
    {
        BBox    bbox;
        int16_t nTris;
        int16_t iMesh;
        int16_t iBone;
        int16_t iDList;
        int16_t iOffset;
        MatInfo materialInfo;

        void read(InevFile&);
    };

    struct LowQuad
    {
        uint8_t iP[4];
        uint8_t iN;
        uint8_t flags;
        void    read(InevFile&);
    };

    struct LowCluster
    {
        BBox    bbox;
        int16_t iVectorOffset;
        int16_t nPoints;
        int16_t nNormals;
        int16_t iQuadOffset;
        int16_t nQuads;
        int16_t iMesh;
        int16_t iBone;
        void    read(InevFile&);
    };

    BBox         bbox; // Only valid for "zero pose".
    int32_t      numHighClusters;
    HighCluster* highClusters;
    int          numHighIndices;
    uint16_t*    highIndexToVert0;
    int16_t      numLowClusters;
    int16_t      numLowVectors;
    int16_t      numLowQuads;
    LowCluster*  lowClusters;
    Vector3*     lowVectors;
    LowQuad*     lowQuads;
};
