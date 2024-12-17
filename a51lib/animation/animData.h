#pragma once
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

// AKA anim_group
class AnimData
{
public:
    bool readFile(uint8_t* fileData, int len);
    void describe(std::ostream& ss);

private:
    void readBone(DataReader& reader, AnimBone& bone);
    void readAnim(DataReader& reader, AnimInfo& info);

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
};
