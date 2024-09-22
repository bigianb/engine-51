#include "RigidGeom.h"
#include "InevFile.h"



bool RigidGeom::read(uint8_t* fileData, int len)
{
    InevFile inevFile;
    bool ok = inevFile.init(fileData, len);
    if (!ok){
        return false;
    }

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


    return ok;
}

const char* describePlatform(int plat)
{
    switch(plat)
    {
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
    ss << "Number of rigid bodies: " << numRigidBodies << std::endl;
    ss << "Number of meshes: " << numMeshes << std::endl;

}
