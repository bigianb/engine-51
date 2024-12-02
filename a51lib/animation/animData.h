#pragma once
#include <cstdint>
#include <string>
#include <iostream>

#include "../VectorMath.h"

struct AnimBone
{
    Matrix4     bindMatrixInv;
    Vector3     localTranslation;
    Vector3     bindTranslation;

    int16_t     iBone;
    int16_t     iParent;
    int16_t     nChildren;
    char        name[32+2];
};

class AnimData
{
public:
    bool readFile(uint8_t* fileData, int len);
    void describe(std::ostream& ss);

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

};

