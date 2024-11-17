#pragma once

#include "VectorMath.h"


class InevFile;

struct Bone
{
    enum HitLocation
    {
        HIT_LOCATION_START,

        HIT_LOCATION_HEAD = HIT_LOCATION_START,
        HIT_LOCATION_SHOULDER_LEFT,
        HIT_LOCATION_SHOULDER_RIGHT,
        HIT_LOCATION_TORSO,
        HIT_LOCATION_LEGS,

        HIT_LOCATION_COUNT,

        HIT_LOCATION_UNKNOWN,
        HIT_LOCATION_UNKNOWN_WRONG_GUID,
    };

    // Local space
    Quaternion bindRotation;
    Vector3    bindPosition;
    BBox       bbox;

    HitLocation hitLocation;  // serialised as a short
    short       rigidBodyIdx; // Index of rigid body to attach to (or -1 if none)

    void read(InevFile&);
};

#define MAX_ANIM_BONES 80

struct BoneMask
{
    int   nameOffset;
    int   numBones;
    float weights[MAX_ANIM_BONES];

    void read(InevFile&);
};

struct PropertySection
{
    short nameOffset;
    short propertyIdx;
    short numProperties;

    void read(InevFile&);
};

struct Property
{
    enum type
    {
        TYPE_FLOAT,
        TYPE_INTEGER,
        TYPE_ANGLE,
        TYPE_STRING,

        TYPE_TOTAL
    };

    short nameOffset;
    short type;
    union
    {
        float floatVal;
        int   intVal;
        float angle;
        int   stringOffset;
    } value;

    void read(InevFile&);
};

struct RigidBody
{
    // Invese Kinematic degress of freedom
    struct Dof
    {
        enum Axis
        {
            // translation
            DOF_TX,
            DOF_TY,
            DOF_TZ,

            // rotation
            DOF_RX,
            DOF_RY,
            DOF_RZ,
        };

        enum Flags
        {
            FLAG_ACTIVE = (1 << 0),
            FLAG_LIMITED = (1 << 1),
        };

        unsigned int flags;
        float        min;
        float        max;

        void read(InevFile&);
    };

    enum Type
    {
        TYPE_SPHERE,
        TYPE_CYLINDER,
        TYPE_BOX
    };

    enum Flags
    {
        FLAG_WORLD_COLLISION = (1 << 0),
    };

    Quaternion bodyBindRotation;  // World space body bind rotation
    Vector3    bodyBindPosition;  // World space body bind position
    Quaternion pivotBindRotation; // World space pivot bind rotation
    Vector3    pivotBindPosition; // World space pivot bind position

    int          nameOffset;    // Offset into string data for name
    float        mass;          // Mass of rigid body
    float        radius;        // Radius of rigid body
    float        width;         // Width of rigid body
    float        height;        // Height of rigid body
    float        length;        // Length of rigid body
    int16_t      type;          // Type of rigid body
    uint16_t     flags;         // Various flags
    int16_t      iParentBody;   // Index of parent rigid body (or -1)
    int16_t      iBone;         // Index of best bone to attach to
    unsigned int collisionMask; // Describes collision with other bodies
    Dof          dof[6];        // Degrees of freedom info

    void read(InevFile&);
};

struct Mesh
{
    BBox    bbox;
    int16_t nameOffset;
    int16_t nSubMeshes;
    int16_t iSubMesh;
    int16_t nBones;
    int16_t nFaces;
    int16_t nVertices;

    void read(InevFile&);
};

struct Submesh
{
    uint16_t     iDList;         // Index into list of display lists
    int16_t      iMaterial;      // Index of the Material that this SubMesh uses
    float        worldPixelSize; // Average World Pixel size for this SubMesh
    unsigned int baseSortKey;    // used internally by the rendering system, not serialised

    void read(InevFile&);
};

struct Material
{
    struct UVanim
    {
        enum Type
        {
            FIXED = 0,
            LOOPED,
            PINGPONG,
            ONESHOT,
        };

        int8_t  type;
        int8_t  startFrame;
        int8_t  fps;
        int8_t  nKeys;
        int16_t iKey;

        void read(InevFile&);
    };

    enum
    {
        MAX_PARAMS = 12,
    };

    enum
    {
        FLAG_DOUBLE_SIDED = 0x0001,
        FLAG_HAS_ENV_MAP = 0x0002,
        FLAG_HAS_DETAIL_MAP = 0x0004,
        FLAG_ENV_WORLD_SPACE = 0x0008,
        FLAG_ENV_VIEW_SPACE = 0x0010,
        FLAG_ENV_CUBE_MAP = 0x0020,
        FLAG_FORCE_ZFILL = 0x0040,
        FLAG_ILLUM_USES_DIFFUSE = 0x0080,
        FLAG_IS_PUNCH_THRU = 0x0100,
        FLAG_IS_ADDITIVE = 0x0200,
        FLAG_IS_SUBTRACTIVE = 0x0400
    };

    UVanim   uvAnim; // UV Animation data
    float    detailScale;
    float    fixedAlpha;
    uint16_t flags; // flags
    uint8_t  type;
    uint8_t  nTextures;    // Total number of textures used in the material
    uint8_t  iTexture;     // Index into global texture list for the Geom
    uint8_t  nVirtualMats; // Number of registered mats based on this material (1 unless there is a virtual texture present)
    uint8_t  iVirtualMat;  // Offset to the bitmaps

    void read(InevFile&);
};

struct Texture
{
    int16_t descOffset;
    int16_t fileNameOffset;

    void read(InevFile&);
};

struct UVkey
{
    uint8_t offsetU;
    uint8_t offsetV;

    void read(InevFile&);
};

struct VirtualMesh
{
    int16_t nameOffset;
    int16_t nLODs;
    int16_t iLOD;

    void read(InevFile&);
};

struct VirtualTexture
{
    int16_t  nameOffset;
    uint32_t materialMask;

    void read(InevFile&);
};

class Geom
{
public:
    Geom();
    ~Geom();

    bool readFile(uint8_t* fileData, int len);
    virtual void read(InevFile&);

    void describe(std::ostringstream& ss);

    const BBox& getBoundingBox() const {return bbox;}

private:
    void describeProperty(std::ostringstream& ss, const char* prefix, int propertyIndex);
    void describeProperies(std::ostringstream& ss);
    void describeTextures(std::ostringstream& ss);
    std::string lookupString(int offset);

protected:
    BBox     bbox;
    int16_t  platform;
    uint16_t unknown;
    int16_t  version;
    int16_t  numFaces;
    int16_t  numVertices;
    int16_t  numBones;
    int16_t  numBoneMasks;
    int16_t  numPropertySections;
    int16_t  numProperties;
    int16_t  numRigidBodies;
    int16_t  numMeshes;
    int16_t  numSubMeshes;
    int16_t  numMaterials;
    int16_t  numTextures;
    int16_t  numUVKeys;
    int16_t  numLODs;
    int16_t  numVirtualMeshes;
    int16_t  numVirtualMaterials;
    int16_t  numVirtualTextures;
    int16_t  stringDataSize;

    Bone*            bones;
    BoneMask*        boneMasks;
    Property*        properties;
    PropertySection* propertySections;
    RigidBody*       rigidBodies;
    Mesh*            meshes;
    Submesh*         subMeshes;

    Material*    materials;
    Texture*     textures;
    UVkey*       uvKeys;
    uint16_t*    lodSizes;
    uint64_t*    lodMasks;
    VirtualMesh* virtualMeshes;
    //VirtualMaterial* virtualMaterials;    // not serialised
    VirtualTexture* virtualTextures;
    char*           stringData;
};
