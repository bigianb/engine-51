#include <cstdint>
#include <iostream>
#include <cassert>
#include "../streamingOperators.h"
#include "../Helpers.h"

#include "Geom.h"
#include "../InevFile.h"


void Geom::Bone::read(InevFile& inevFile)
{
    inevFile.read(bindRotation);
    inevFile.read(bindPosition);
    inevFile.read(bbox);

    short s;
    inevFile.read(s);
    hitLocation = HitLocation(s);
    inevFile.read(rigidBodyIdx);
    inevFile.skip(12);
}

void Geom::BoneMask::read(InevFile& inevFile)
{
    inevFile.read(nameOffset);
    inevFile.read(numBones);
    for (int i = 0; i < MAX_ANIM_BONES; ++i) {
        inevFile.read(weights[i]);
    }
}

void Geom::Property::read(InevFile& inevFile)
{
    inevFile.read(nameOffset);
    inevFile.read(type);
    switch (type) {
    case Geom::Property::TYPE_FLOAT:
        inevFile.read(value.floatVal);
        break;
    case Geom::Property::TYPE_INTEGER:
        inevFile.read(value.intVal);
        break;
    case Geom::Property::TYPE_ANGLE:
        inevFile.read(value.angle);
        break;
    case Geom::Property::TYPE_STRING:
        inevFile.read(value.stringOffset);
        break;
    default:
        value.stringOffset = 0;
        inevFile.skip(4);
        std::cerr << "Uknown Type " << type << std::endl;
    }
}

void Geom::PropertySection::read(InevFile& inevFile)
{
    inevFile.read(nameOffset);
    inevFile.read(propertyIdx);
    inevFile.read(numProperties);
}

void Geom::RigidBody::read(InevFile& inevFile)
{
    inevFile.read(bodyBindRotation);
    inevFile.read(bodyBindPosition);
    inevFile.read(pivotBindRotation);
    inevFile.read(pivotBindPosition);
    inevFile.read(nameOffset);
    inevFile.read(mass);
    inevFile.read(radius);
    inevFile.read(width);
    inevFile.read(height);
    inevFile.read(length);
    inevFile.read(type);
    inevFile.read(flags);
    inevFile.read(iParentBody);
    inevFile.read(iBone);
    inevFile.read(collisionMask);
    for (int i = 0; i < 6; ++i) {
        inevFile.read(dof[i]);
    }
}

void Geom::RigidBody::Dof::read(InevFile& inevFile)
{
    inevFile.read(flags);
    inevFile.read(min);
    inevFile.read(max);
}

void Geom::Mesh::read(InevFile& inevFile)
{
    inevFile.read(bbox);
    inevFile.read(nameOffset);
    inevFile.read(nSubMeshes);
    inevFile.read(iSubMesh);
    inevFile.read(nBones);
    inevFile.read(nFaces);
    inevFile.read(nVertices);
    inevFile.skip(4);
}

void Geom::Submesh::read(InevFile& inevFile)
{
    inevFile.read(iDList);
    inevFile.read(iMaterial);
    inevFile.read(worldPixelSize);
    inevFile.skip(4);
}

void Geom::Material::UVanim::read(InevFile& inevFile)
{
    inevFile.read(type);
    inevFile.read(startFrame);
    inevFile.read(fps);
    inevFile.read(nKeys);
    inevFile.read(iKey);
}

void Geom::Material::read(InevFile& inevFile)
{
    inevFile.read(uvAnim);
    inevFile.skip(2);
    inevFile.read(detailScale);
    inevFile.read(fixedAlpha);
    inevFile.read(flags);
    inevFile.read(type);
    inevFile.read(nTextures);
    inevFile.read(iTexture);
    inevFile.read(nVirtualMats);
    inevFile.read(iVirtualMat);
    inevFile.skip(1);
}

void Geom::Texture::read(InevFile& inevFile)
{
    inevFile.read(descOffset);
    inevFile.read(fileNameOffset);
}

void Geom::UVkey::read(InevFile& inevFile)
{
    inevFile.read(offsetU);
    inevFile.read(offsetV);
}

void Geom::VirtualMesh::read(InevFile& inevFile)
{
    inevFile.read(nameOffset);
    inevFile.read(nLODs);
    inevFile.read(iLOD);
}

void Geom::VirtualTexture::read(InevFile& inevFile)
{
    inevFile.read(nameOffset);
    inevFile.read(materialMask);
}

Geom::Geom()
{
    m_RefCount = 0;
    m_hGeom    = HNULL;
    bones = nullptr;
    boneMasks = nullptr;
    properties = nullptr;
    propertySections = nullptr;
    rigidBodies = nullptr;
    meshes = nullptr;
    subMeshes = nullptr;
    materials = nullptr;
    textures = nullptr;
    uvKeys = nullptr;
    lodSizes = nullptr;
    lodMasks = nullptr;
    virtualMeshes = nullptr;
    virtualTextures = nullptr;
    virtualMaterials = nullptr;
    stringData = nullptr;
}

Geom::~Geom()
{
    delete[] bones;
    bones = nullptr;
    delete[] boneMasks;
    boneMasks = nullptr;
    delete[] properties;
    properties = nullptr;
    delete[] propertySections;
    propertySections = nullptr;
    delete[] rigidBodies;
    rigidBodies = nullptr;
    delete[] meshes;
    meshes = nullptr;
    delete[] subMeshes;
    subMeshes = nullptr;
    delete[] materials;
    materials = nullptr;
    delete[] textures;
    textures = nullptr;
    delete[] uvKeys;
    uvKeys = nullptr;
    delete[] lodSizes;
    lodSizes = nullptr;
    delete[] lodMasks;
    lodMasks = nullptr;
    delete[] virtualMeshes;
    virtualMeshes = nullptr;
    delete[] virtualTextures;
    virtualTextures = nullptr;
    delete[] virtualMaterials;
    virtualMaterials = nullptr;
    delete[] stringData;
    stringData = nullptr;
}

bool Geom::readFile(uint8_t* fileData, int len)
{
    InevFile inevFile;
    bool     ok = inevFile.init(fileData, len);
    if (!ok) {
        return false;
    }

    //std::ostringstream ss;
    //inevFile.describe(ss);
    //std::cout << ss.str() << std::endl;
    read(inevFile);
    return true;
}

void Geom::read(InevFile& inevFile)
{
    inevFile.read(bbox);
    inevFile.read(platform);
    inevFile.read(m_RefCount);
    inevFile.setPlatform(platform);

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

    inevFile.readArray(propertySections, numPropertySections);
    inevFile.readArray(properties, numProperties);
    inevFile.readArray(rigidBodies, numRigidBodies);
    inevFile.readArray(meshes, numMeshes);
    inevFile.readArray(subMeshes, numSubMeshes);
    inevFile.readArray(materials, numMaterials);
    inevFile.readArray(textures, numTextures);
    inevFile.readArray(uvKeys, numUVKeys);
    inevFile.readNativeArray(lodSizes, numLODs);
    inevFile.readNativeArray(lodMasks, numLODs);
    inevFile.readArray(virtualMeshes, numVirtualMeshes);
    uint32_t x;
    inevFile.read(x); // Read the unused virtualMaterials pointer.
    if (numVirtualMaterials > 0){
        virtualMaterials = new VirtualMaterial[numVirtualMaterials];
    }
    inevFile.readArray(virtualTextures, numVirtualTextures);
    inevFile.readNativeArray(stringData, stringDataSize);
    inevFile.read(x); // Read the unused handle

    m_RefCount = 0;
    m_hGeom    = HNULL;
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

std::string Geom::lookupString(int offset) const
{
    if (stringData == nullptr) {
        return "undefined";
    }
    return std::string(stringData + offset);
}

void Geom::describeProperty(std::ostringstream& ss, const char* prefix, int propertyIndex) const
{
    auto& property = properties[propertyIndex];
    ss << prefix << lookupString(property.nameOffset) << ": ";
    switch (property.type) {
    case Property::TYPE_FLOAT:
        ss << property.value.floatVal;
        break;
    case Property::TYPE_ANGLE:
        ss << property.value.angle;
        break;
    case Property::TYPE_INTEGER:
        ss << property.value.intVal;
        break;
    case Property::TYPE_STRING:
        ss << lookupString(property.value.stringOffset);
        break;
    default:
        ss << "unknown type: " << property.type;
    }
    ss << std::endl;
}

void Geom::describeProperies(std::ostringstream& ss) const
{
    if (numPropertySections == 0) {
        ss << "  No Properties" << std::endl;
    }

    for (int ps = 0; ps < numPropertySections; ++ps) {
        auto&       propertySection = propertySections[ps];
        std::string name = lookupString(propertySection.nameOffset);
        ss << "  Section: " << name << std::endl;
        for (int pi = propertySection.propertyIdx; pi < propertySection.propertyIdx + propertySection.numProperties; ++pi) {
            describeProperty(ss, "    ", pi);
        }
    }
}

void Geom::describeMaterials(std::ostringstream& ss) const
{
    if (numMaterials == 0) {
        ss << "  No Materials" << std::endl;
    }
    if (nullptr == materials) {
        // demo geoms are not fully parsed.
        return;
    }
    ss << numMaterials << " materials" << std::endl;
    for (int i = 0; i < numMaterials; ++i) {
        const auto& mat = materials[i];
        ss << "  detailScale: " << mat.detailScale << std::endl;
        ss << "  fixedAlpha: " << mat.fixedAlpha << std::endl;
        ss << "  iTexture: " << (int)mat.iTexture << std::endl;
        ss << "  nTextures: " << (int)mat.nTextures << std::endl;
        ss << "  iVirtualMat: " << (int)mat.iVirtualMat << std::endl;
        ss << "  nVirtualMats: " << (int)mat.nVirtualMats << std::endl;
        ss << std::endl;
    }
}

std::string Geom::getTextureFilename(int tNum)
{
    if (textures == nullptr) {
        return "unknown";
    }
    if (textures[tNum].fileName.empty()) {
        textures[tNum].fileName = lookupString(textures[tNum].fileNameOffset);
    }
    return textures[tNum].fileName;
}

std::string Geom::GetTextureDesc(int iTexture) const
{
    assert( (iTexture >= 0) && (iTexture < numTextures) );
    int StringDataOffset = textures[iTexture].descOffset;
    return lookupString(StringDataOffset);
}

int Geom::GetVTextureIndex( const char* pName ) const
{
    for( int i = 0; i < numVirtualTextures; i++ )
    {
        int StringDataOffset = virtualTextures[i].nameOffset;
        if (lookupString(StringDataOffset) == pName){
            return i;
        }
    }

    return -1;
}

void Geom::describeBones(std::ostringstream& ss) const
{
    if (numBones == 0) {
        ss << "  No Bones" << std::endl;
    }

    for (int i = 0; i < numBones; ++i) {
        const auto& bone = bones[i];
        ss << "  Rigid Body Index: " << bone.rigidBodyIdx << std::endl;
        ss << "  Hit Location:     " << bone.hitLocation << std::endl;
        ss << "  Position          " << bone.bindPosition << std::endl;
        ss << "  Rotation          " << bone.bindRotation << std::endl;
        ss << "  Bounding Box:     " << bone.bbox << std::endl;
        ss << std::endl;
    }
}

void Geom::describeTextures(std::ostringstream& ss)
{
    if (numTextures == 0) {
        ss << "  No Textures" << std::endl;
    }
    if (nullptr == textures) {
        // demo geoms are not fully parsed.
        return;
    }
    for (int i = 0; i < numTextures; ++i) {
        const auto& texture = textures[i];
        ss << "  ID: " << i << std::endl;
        ss << "  Description: " << lookupString(texture.descOffset) << std::endl;
        ss << "  Filename: " << getTextureFilename(i) << std::endl
           << std::endl;
    }
}

void Geom::describeMeshes(std::ostringstream& ss) const
{
    if (numMeshes == 0 || meshes == nullptr) {
        return;
    }
    for (int i = 0; i < numMeshes; ++i) {
        const auto& mesh = meshes[i];
        ss << "  Mesh: " << i << std::endl;
        ss << "      Name: " << lookupString(mesh.nameOffset) << std::endl;
        ss << "      BBox: " << mesh.bbox;
        ss << std::endl;
        ss << "      Num Submeshes: " << mesh.nSubMeshes << std::endl;
        ss << "      Idx Submeshes: " << mesh.iSubMesh << std::endl;
        ss << "      Num Bones: " << mesh.nBones << std::endl;
        ss << "      Num Faces: " << mesh.nFaces << std::endl;
        ss << "      Num Vertices: " << mesh.nVertices << std::endl;
        ss << "      Submeshes: " << std::endl;
        if (subMeshes != nullptr) {
            for (int j = mesh.iSubMesh; j < mesh.iSubMesh + mesh.nSubMeshes; ++j) {
                const auto& subMesh = subMeshes[j];
                ss << "          Dlist Idx: " << subMesh.iDList << std::endl;
                ss << "          Material Idx: " << subMesh.iMaterial << std::endl;
                ss << "          Pixel Size: " << subMesh.worldPixelSize << std::endl;
                ss << std::endl;
            }
        }
        ss << std::endl;
    }
}

void Geom::describe(std::ostringstream& ss)
{
    ss << "Platform: " << describePlatform(platform) << std::endl;
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

    ss << std::endl
       << "Meshes" << std::endl
       << "------" << std::endl
       << std::endl;
    describeMeshes(ss);

    ss << std::endl
       << "Properties" << std::endl
       << "----------" << std::endl
       << std::endl;
    describeProperies(ss);

    ss << std::endl
       << "Textures" << std::endl
       << "--------" << std::endl
       << std::endl;
    describeTextures(ss);

    ss << std::endl
       << "Materials" << std::endl
       << "--------" << std::endl
       << std::endl;
    describeMaterials(ss);

    ss << std::endl
       << "Bones" << std::endl
       << "-----" << std::endl
       << std::endl;
    describeBones(ss);
}

const Geom::BoneMask* Geom::FindBoneMasks(const char* pName) const
{
    // Loop through all bone masks
    for (int i = 0; i < numBoneMasks; i++) {
        // Lookup name of bone masks entry
        std::string pBoneMasksName = lookupString(boneMasks[i].nameOffset);

        if (pBoneMasksName == pName) {
            return &boneMasks[i];
        }
    }

    // Not found
    return nullptr;
}

const Geom::PropertySection* Geom::FindPropertySection(const char* pSection) const
{
    // Search all property sections
    for (int iSection = 0; iSection < numPropertySections; iSection++) {
        // Lookup section info
        const PropertySection& Section = propertySections[iSection];
        std::string            pSectionName = lookupString(Section.nameOffset);

        // Matching section?
        if (pSectionName == pSection) {
            return &Section;
        }
    }

    // Not found
    return nullptr;
}

const Geom::Property* Geom::FindProperty(const Geom::PropertySection* pSection, const char* pName, Geom::Property::Type type) const
{
    // Make sure section is valid
    assert(pSection);
    assert(pSection >= &propertySections[0]);
    assert(pSection < &propertySections[numPropertySections]);

    // Search all properties in the section
    const Property* Properties = &properties[pSection->propertyIdx];
    for (int iProperty = 0; iProperty < pSection->numProperties; iProperty++) {
        // Lookup property
        const Property& Property = Properties[iProperty];

        // Matching type?
        if (type == Property.type) {
            // Matching name?
            std::string pPropName = lookupString(Property.nameOffset);
            if (pPropName == pName) {
                return &Property;
            }
        }
    }

    // Not found
    return nullptr;
}

// Property query: Returns TRUE and sets up the value if present, else just returns FALSE and leaves value
bool Geom::GetPropertyFloat(const Geom::PropertySection* pSection, const char* pName, float* pValue) const
{
    const Geom::Property* pProp = FindProperty(pSection, pName, Geom::Property::TYPE_FLOAT);
    if (!pProp) {
        return false;
    }

    // Grab value and return success
    *pValue = pProp->value.floatVal;
    return true;
}

bool Geom::GetPropertyInteger(const Geom::PropertySection* pSection, const char* pName, int* pValue) const
{
    const Geom::Property* pProp = FindProperty(pSection, pName, Geom::Property::TYPE_INTEGER);
    if (!pProp) {
        return false;
    }

    // Grab value and return success
    *pValue = pProp->value.intVal;
    return true;
}

bool Geom::GetPropertyAngle(const Geom::PropertySection* pSection, const char* pName, Radian* pValue) const
{
    const Geom::Property* pProp = FindProperty(pSection, pName, Geom::Property::TYPE_ANGLE);
    if (!pProp) {
        return false;
    }

    // Grab value and return success
    *pValue = pProp->value.angle;
    return true;
}

bool Geom::GetPropertyString(const Geom::PropertySection* pSection, const char* pName, char* pValue, int nChars) const
{
    const Geom::Property* pProp = FindProperty(pSection, pName, Geom::Property::TYPE_STRING);
    if (!pProp) {
        return false;
    }

    // Grab value and return success
    strsavecpy( pValue, &stringData[ pProp->value.stringOffset ], nChars );
    *pValue = pProp->value.floatVal;
    return true;
}

uint64_t Geom::GetLODMask(uint32_t VMeshMask, uint16_t ScreenSize) const
{
    if( numVirtualMeshes == 0 )
    {
        return -1;
    }

    uint64_t LODMask = 0;
    for( int i = 0; i < numVirtualMeshes; i++ )
    {
        // check the vmesh mask
        if( (VMeshMask & (1<<i)) && virtualMeshes[i].nLODs )
        {
            // okay, this vmesh is on, which LOD?
            int Choice = 0;
             {
                for( int j = 1; j < virtualMeshes[i].nLODs; j++ )
                {
                    if( ScreenSize < lodSizes[j+virtualMeshes[i].iLOD] )
                        Choice = j;
                }
            }

            // or in this LOD into the total mask
            LODMask |= lodMasks[Choice+virtualMeshes[i].iLOD];
        }
    }

    return LODMask;    
}

