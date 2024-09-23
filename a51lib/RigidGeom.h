#pragma once

#include <cstdint>
#include <sstream>
#include "Geom.h"

class RigidGeom
{
public:
    RigidGeom();
    ~RigidGeom();
    
    bool read(uint8_t* fileData, int len);

    void describe(std::ostringstream& ss);

private:
    BBox bbox;
    int16_t platform;
    uint16_t unknown;
    int16_t version;
    int16_t numFaces;
    int16_t numVertices;
    int16_t numBones;
    int16_t numBoneMasks;
    int16_t numPropertySections;
    int16_t numProperties;
    int16_t numRigidBodies;
    int16_t numMeshes;
    int16_t numSubMeshes;
    int16_t numMaterials;
    int16_t numTextures;
    int16_t numUVKeys;
    int16_t numLODs;
    int16_t numVirtualMeshes;
    int16_t numVirtualMaterials;
    int16_t numVirtualTextures;
    int16_t stringDataSize;

    Bone* bones;
};
