#include "animData.h"
#include "../DataReader.h"
#include "../streamingOperators.h"

void read(DataReader& reader, Matrix4& matrix)
{
    float* pf = &matrix.cells[0][0];
    for (int i = 0; i < 16; ++i) {
        *pf++ = reader.readFloat();
    }
}

void read(DataReader& reader, Vector3& vec)
{
    vec.x = reader.readFloat();
    vec.y = reader.readFloat();
    vec.z = reader.readFloat();
    vec.w = reader.readFloat();
}

void read(DataReader& reader, BBox& bbox)
{
    read(reader, bbox.min);
    read(reader, bbox.max);
}

void AnimData::readBone(DataReader& reader, AnimBone& bone)
{
    read(reader, bone.bindMatrixInv);
    read(reader, bone.localTranslation);
    read(reader, bone.bindTranslation);
    bone.iBone = reader.readInt16();
    bone.iParent = reader.readInt16();
    bone.nChildren = reader.readInt16();
    bone.name = std::string(reader.readString());
    reader.skip(42);
}

void AnimData::readAnim(DataReader& reader, AnimInfo& info)
{
    read(reader, info.totalTranslation);
    read(reader, info.bbox);
    reader.skip(4);
    info.nAnims = reader.readInt32();
    info.animsWeight = reader.readFloat();

    info.name = reader.readString();
    reader.skip(32);
    info.weight = reader.readFloat();
    info.blendTime = reader.readFloat();

    info.nChainFramesMin = reader.readInt16();
    info.nChainFramesMax = reader.readInt16();
    info.iChainAnim = reader.readInt16();
    info.iChainFrame = reader.readInt16();

    info.iAnimBoneMin = reader.readInt16();
    info.iAnimBoneMax = reader.readInt16();

    info.nFrames = reader.readInt16();
    info.iLoopFrame = reader.readInt16();
    info.endFrameOffset = reader.readInt16();

    info.nEvents = reader.readInt16();
    info.iEvent = reader.readInt16();

    info.nProps = reader.readInt16();
    info.iProp = reader.readInt16();

    info.handleAngle = reader.readUInt16();
    info.totalYaw = reader.readUInt16();
    info.totalMoveDir = reader.readUInt16();

    info.flags = reader.readUInt16();
    info.fps = reader.readUInt16();

    info.animKeys.nFrames = reader.readInt16();
    info.animKeys.nBones = reader.readInt16();
    info.animKeys.nProps = reader.readInt16();
    info.animKeys.nKeyBlocks = reader.readInt16();
    info.animKeys.iKeyBlock = reader.readInt16();
    reader.skip(6);
    reader.skip(8);
}

bool AnimData::readFile(uint8_t* fileData, int len)
{
    bool okay = true;

    DataReader reader = DataReader(fileData, len);
    reader.read(bbox);
    filename = std::string(reader.readString());
    reader.skip(60);
    reader.skip(4);

    version = reader.readInt32();
    if (version != 2014) {
        return false;
    }

    totalNFrames = reader.readInt32();
    totalNKeys = reader.readInt32();
    numBones = reader.readInt32();
    reader.skip(4);

    numAnims = reader.readInt32();
    reader.skip(4);

    numProps = reader.readInt32();
    reader.skip(4);

    numEvents = reader.readInt32();
    reader.skip(8);

    numKeyBlocks = reader.readInt32();
    reader.skip(4);

    reader.skip(16); // uncompressed and compressed data pointers
    reader.skip(8);  // 16 byte alignment.

    bones.resize(numBones);
    for (int i = 0; i < numBones; ++i) {
        readBone(reader, bones.at(i));
    }
    anims.resize(numAnims);
    for (int i = 0; i < numAnims; ++i) {
        readAnim(reader, anims.at(i));
    }

    return okay;
}

void AnimData::describe(std::ostream& ss)
{
    ss << "BBox:" << bbox << std::endl;
    ss << "Filename: " << filename << std::endl;
    ss << "Total N Frames: " << totalNFrames << std::endl;
    ss << "Total N Keys: " << totalNKeys << std::endl;
    ss << "Num Bones: " << numBones << std::endl;
    ss << "Num Anims: " << numAnims << std::endl;
    ss << "Num Props: " << numProps << std::endl;
    ss << "Num Events: " << numEvents << std::endl;
    ss << "Num Key blocks: " << numKeyBlocks << std::endl;

    ss << "Skeleton: " << std::endl;
    for (int i = 0; i < numBones; ++i) {
        auto& bone = bones.at(i);
        ss << "  name:          " << bone.name << std::endl;
        ss << "  iBone:         " << bone.iBone << std::endl;
        ss << "  iParent:       " << bone.iParent << std::endl;
        ss << "  num Children : " << bone.nChildren << std::endl;
        ss << std::endl;
    }
    ss << std::endl
       << "Infos: " << std::endl;
    for (int i = 0; i < numAnims; ++i) {
        auto& info = anims.at(i);
        ss << "  translation:  " << info.totalTranslation << std::endl;
        ss << "  bounding box: " << info.bbox << std::endl;
        ss << "  name:         " << info.name << std::endl;
        ss << "  num anims:    " << info.nAnims << std::endl;
        ss << "  total weight: " << info.animsWeight << std::endl;
        ss << "  num events:   " << info.nEvents << std::endl;
        ss << std::endl;
    }
}
