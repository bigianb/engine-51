#include "animData.h"
#include "../DataReader.h"
#include "../streamingOperators.h"

void read(DataReader& reader, Matrix4& matrix)
{
    float *pf = &matrix.cells[0][0];
    for (int i=0; i<16; ++i){
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

bool AnimData::readFile(uint8_t* fileData, int len)
{
    bool okay = true;

    DataReader reader = DataReader(fileData, len);
    reader.read(bbox);
    filename = std::string(reader.readString());
    reader.skip(60);
    reader.skip(4);

    version = reader.readInt32();
    if (version != 2014){
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

    reader.skip(16);    // uncompressed and compressed data pointers
    reader.skip(8);     // 16 byte alignment.

    bones.resize(numBones);
    for (int i=0; i < numBones; ++i){
        readBone(reader, bones.at(i));
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
    for (int i=0; i<numBones; ++i){
        auto& bone = bones.at(i);
        ss << "  name:          " << bone.name << std::endl;
        ss << "  iBone:         " << bone.iBone << std::endl;
        ss << "  iParent:       " << bone.iParent << std::endl;
        ss << "  num Children : " << bone.nChildren << std::endl;
        ss << std::endl;
    }
}
