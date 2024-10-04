#include "RigidGeom.h"
#include "InevFile.h"

#include <iostream>

void Bone::read(InevFile& inevFile)
{
    inevFile.read(bindRotation);
    inevFile.read(bindPosition);
    inevFile.read(bbox);

    short s;
    inevFile.read(s);
    hitLocation = HitLocation(s);
    inevFile.read(rigidBodyIdx);
}

void BoneMask::read(InevFile& inevFile)
{
    inevFile.read(nameOffset);
    inevFile.read(numBones);
    for (int i=0; i < MAX_ANIM_BONES; ++i){
        inevFile.read(weights[i]);
    }
}

void Property::read(InevFile& inevFile)
{
    inevFile.read(nameOffset);
    inevFile.read(type);
    switch (type)
    {
        case Property::TYPE_FLOAT:
            inevFile.read(value.floatVal);
            break;
        case Property::TYPE_INTEGER:
            inevFile.read(value.intVal);
            break;
        case Property::TYPE_ANGLE:
            inevFile.read(value.angle);
            break;
        case Property::TYPE_STRING:
            inevFile.read(value.stringOffset);
            break;
        default:
            std::cerr << "Uknown Type " << type << std::endl;
    }
}

RigidGeom::RigidGeom()
{
    bones = nullptr;
}

RigidGeom::~RigidGeom()
{
    delete bones; bones = nullptr;
}

bool RigidGeom::read(uint8_t* fileData, int len)
{
    InevFile inevFile;
    bool ok = inevFile.init(fileData, len);
    if (!ok) {
        return false;
    }

    std::ostringstream ss;
    inevFile.describe(ss);  
    std::cout << ss.str() << std::endl;

    inevFile.read(bbox);
    inevFile.read(platform);
    inevFile.read(unknown);
    inevFile.read(version);
    inevFile.read(numFaces);
    inevFile.read(numVertices);
    inevFile.read(numBones);
    inevFile.read(numBoneMasks);
    inevFile.read(numPropertySections);
    inevFile.read(numProperties);
    inevFile.read(numRigidBodies);
    inevFile.read(numMeshes);
    inevFile.read(numSubMeshes);
    inevFile.read(numMaterials);
    inevFile.read(numTextures);
    inevFile.read(numUVKeys);
    inevFile.read(numLODs);
    inevFile.read(numVirtualMeshes);
    inevFile.read(numVirtualMaterials);
    inevFile.read(numVirtualTextures);
    inevFile.read(stringDataSize);

    inevFile.readArray(bones, numBones);
    inevFile.readArray(boneMasks, numBoneMasks);
    inevFile.readArray(properties, numProperties);
    inevFile.readArray(propertySections, numPropertySections);

    return ok;
}

const char* describePlatform(int plat)
{
    switch (plat) {
    case 1:
        return "PC";
    case 4:
        return "PS2";
    case 8:
        return "XBOX";
    }
    return "unknown";
}

void RigidGeom::describe(std::ostringstream& ss)
{
    ss << "Platform: " << describePlatform(platform) << std::endl;
    ss << "Unknown: 0x" << std::hex << unknown << std::dec << std::endl;
    ss << "Version: " << version << std::endl;
    ss << "Number of faces: " << numFaces << std::endl;
    ss << "Number of vertices: " << numVertices << std::endl;
    ss << "Number of bones: " << numBones << std::endl;
    ss << "Number of bone masks: " << numBoneMasks << std::endl;
    ss << "Number of property sections: " << numPropertySections << std::endl;
    ss << "Number of properties: " << numProperties << std::endl;

    ss << "Number of rigid bodies: " << numRigidBodies << std::endl;
    ss << "Number of meshes: " << numMeshes << std::endl;
    ss << "Number of sub-meshes: " << numSubMeshes << std::endl;
    ss << "Number of materials: " << numMaterials << std::endl;
    ss << "Number of textures: " << numTextures << std::endl;
    ss << "Number of UV keys: " << numUVKeys << std::endl;
    ss << "Number of LODs: " << numLODs << std::endl;

    ss << "Number of virtual meshes: " << numVirtualMeshes << std::endl;
    ss << "Number of virtual materials: " << numVirtualMaterials << std::endl;
    ss << "Number of virtual textures: " << numVirtualTextures << std::endl;

    ss << "String data size: " << stringDataSize << std::endl;
}
