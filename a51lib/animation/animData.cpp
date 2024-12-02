#include "animData.h"
#include "../DataReader.h"
#include "../streamingOperators.h"

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

    // AnimBone data follows (TODO)

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
}
