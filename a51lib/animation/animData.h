#pragma once
#include <cstdint>
#include <string>
#include <iostream>
#include <vector>

#include "../VectorMath.h"

class DataReader;

struct AnimBone
{
    Matrix4     bindMatrixInv;
    Vector3     localTranslation;
    Vector3     bindTranslation;

    int16_t     iBone;
    int16_t     iParent;
    int16_t     nChildren;
    std::string name;
};

class AnimData
{
public:
    bool readFile(uint8_t* fileData, int len);
    void describe(std::ostream& ss);

private:
    void readBone(DataReader& reader, AnimBone& bone);

public:
    BBox bbox;
    std::string filename;
    int version;

    int totalNFrames;
    int totalNKeys;

    int numBones;
    int numAnims;
    int numProps;
    int numEvents;
    int numKeyBlocks;

    std::vector<AnimBone> bones;

};

