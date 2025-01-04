#pragma once
#include <stdlib.h>
#include <cstdint>
#include <string>
#include <iostream>
#include <vector>

#include "../VectorMath.h"

class DataReader;

struct AnimBone
{
    Matrix4 bindMatrixInv;
    Vector3 localTranslation;
    Vector3 bindTranslation;

    int16_t     iBone;
    int16_t     iParent;
    int16_t     nChildren;
    std::string name;
};

class AnimKeys
{
public:
    short nFrames;
    short nBones;
    short nProps;
    short nKeyBlocks;
    short iKeyBlock;
};

class AnimInfo
{
public:
    Vector3     totalTranslation; // Total movement
    BBox        bbox;             // BBox of all verts pushed thu anim
    int         nAnims;
    float       animsWeight;
    std::string name;
    float       weight;
    float       blendTime;

    short nChainFramesMin;
    short nChainFramesMax;
    short iChainAnim;
    short iChainFrame;

    short iAnimBoneMin;
    short iAnimBoneMax;

    short nFrames;
    short iLoopFrame;
    short endFrameOffset;

    short nEvents;
    short iEvent;

    short nProps;
    short iProp;

    uint16_t handleAngle;
    uint16_t totalYaw;
    uint16_t totalMoveDir;

    uint16_t flags;
    uint16_t fps;

    AnimKeys animKeys;
};

class AnimKeyStream
{
public:
    void setOffset(uint32_t);
    uint32_t getOffset() const;

    //  2 bits for the scale format = 4 formats
    //  2 bits for the rot   format = 4 formats
    //  2 bits for the trans format = 4 formats
    //  8 bits for bone flag bits   = 8 custom flags per bone per anim
    // 18 bits for the offset to the scale data = 0-262143 range
    //-----
    // 32

    uint32_t Offset;
};

class AnimKeyBlock
{
public:
    AnimKeyBlock(){
        next = nullptr;
        prev = nullptr;
        stream = nullptr;
        factoredCompressedData = nullptr;
    }

    ~AnimKeyBlock(){
        if (stream != nullptr){
            free(stream);
            stream = nullptr;
        }
    }

    AnimKeyBlock*  next;
    AnimKeyBlock*  prev;
    AnimKeyStream* stream; // Points to decompressed data if available
    uint32_t       checksum;
    uint8_t*       factoredCompressedData;
    int            compressedDataOffset; // Offset into compressed data for this key set
    int            nFrames;
    int            decompressedDataSize;
};

// AKA anim_group
class AnimData
{
public:
    bool readFile(uint8_t* fileData, int len);
    void describe(std::ostream& ss);

private:
    void readBone(DataReader& reader, AnimBone& bone);
    void readAnim(DataReader& reader, AnimInfo& info);
    void readKeyBlock(DataReader& reader, AnimKeyBlock& keyBlock, int compressedDataStartOffset);

public:
    BBox        bbox;
    std::string filename;
    int         version;

    int totalNFrames;
    int totalNKeys;

    int numBones;
    int numAnims;
    int numProps;
    int numEvents;
    int numKeyBlocks;

    std::vector<AnimBone> bones;
    std::vector<AnimInfo> anims;
    std::vector<AnimKeyBlock> keyBlocks;
};
