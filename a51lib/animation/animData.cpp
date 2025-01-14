#include <cassert>
#include "animData.h"
#include "../DataReader.h"
#include "../dataUtil/Bitstream.h"
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

void AnimGroup::readBone(DataReader& reader, AnimBone& bone)
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

void AnimationDecompress(const AnimGroup&, const uint8_t*, AnimKeyStream*, int);

void AnimGroup::readKeyBlock(DataReader& reader, AnimKeyBlock& keyBlock, int compressedDataStartOffset)
{
    keyBlock.next = nullptr;
    keyBlock.prev = nullptr;
    keyBlock.stream = nullptr;
    reader.skip(12);
    keyBlock.checksum = reader.readUInt32();
    keyBlock.factoredCompressedData = nullptr;
    reader.skip(4);
    keyBlock.compressedDataOffset = reader.readInt32();
    uint32_t temp = reader.readUInt32();
    keyBlock.nFrames = temp & 0xFF;
    keyBlock.decompressedDataSize = (temp >> 8);

    keyBlock.stream = (AnimKeyStream*)malloc(keyBlock.decompressedDataSize);
    AnimationDecompress(*this, reader.fileData + compressedDataStartOffset + keyBlock.compressedDataOffset, keyBlock.stream, keyBlock.decompressedDataSize);
}

void AnimGroup::readAnim(DataReader& reader, AnimInfo& info)
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

bool AnimGroup::readFile(uint8_t* fileData, int len)
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
    uint32_t bonesOffset = reader.readInt32();

    numAnims = reader.readInt32();
    uint32_t animsOffset = reader.readInt32();

    numProps = reader.readInt32();
    uint32_t propsOffset = reader.readInt32();

    numEvents = reader.readInt32();
    reader.skip(8);

    numKeyBlocks = reader.readInt32();
    uint32_t keyBlocksOffset = reader.readInt32();

    uint32_t uncompressedDataSize = reader.readInt32();
    reader.skip(4);
    uint32_t compressedDataSize = reader.readInt32();
    reader.skip(4);

    reader.skip(8); // 16 byte alignment.

    uint32_t uncompressedDataStartOffset = reader.cursor;
    uint32_t compressedDataStartOffset = uncompressedDataStartOffset + uncompressedDataSize;
    // This is the uncompressed data.
    bones.resize(numBones);
    for (int i = 0; i < numBones; ++i) {
        readBone(reader, bones.at(i));
    }
    anims.resize(numAnims);
    for (int i = 0; i < numAnims; ++i) {
        readAnim(reader, anims.at(i));
    }

    // TODO: read props

    keyBlocks.resize(numKeyBlocks);
    reader.cursor = uncompressedDataStartOffset + keyBlocksOffset;
    for (int i = 0; i < numKeyBlocks; ++i) {
        readKeyBlock(reader, keyBlocks.at(i), compressedDataStartOffset);
    }
    // file offset 0x690 AH_AMMOCRATE.anim
    // 0 events, 0 props, 10 key blocks

    // This is the compressed data.

    // Event data follows
    return okay;
}

void AnimGroup::describe(std::ostream& ss)
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

        ss << "  num frames:   " << info.nFrames << std::endl;
        ss << "  bone min:   " << info.iAnimBoneMin << std::endl;
        ss << "  bone max:   " << info.iAnimBoneMax << std::endl;

        ss << "   start key block " << info.animKeys.iKeyBlock << std::endl;
        ss << "   num key blocks " << info.animKeys.nKeyBlocks << std::endl;
        ss << "   num bones " << info.animKeys.nBones << std::endl;
        ss << "   num frames " << info.animKeys.nFrames << std::endl;

        ss << std::endl;
    }
    ss << "Key blocks: " << std::endl;
    for (int i = 0; i < numKeyBlocks; ++i) {
        AnimKeyBlock& kb = keyBlocks.at(i);
        ss << "    num Frames: " << kb.nFrames << std::endl;

        ss << std::endl;
    }
}

enum AnimKeyFormat
{
    CONSTANT_VALUE = 0,
    SINGLE_VALUE = 1,
    PRECISION_16 = 2,
    PRECISION_32 = 3,
};

static const int s_ScaleFormatOverhead[] = {0, 12, 24, 0};
static const int s_ScaleFormatSize[] = {0, 0, 2, 12};
static const int s_RotationFormatOverhead[] = {0, 16, 0, 0};
static const int s_RotationFormatSize[] = {0, 0, 8, 16};

void AnimKeyStream::grabKey(const uint8_t* data, int totalFrames, int frame, AnimKey& Key)
{
    const int rotationFormat = (Offset >> STREAM_ROT_SHIFT) & STREAM_ROT_MASK;
    const int scaleFormat = (Offset >> STREAM_SCL_SHIFT) & STREAM_SCL_MASK;
    const int s_SO = (Offset >> STREAM_OFT_SHIFT) & STREAM_OFT_MASK;
    const int s_RO = s_SO + s_ScaleFormatOverhead[scaleFormat] + s_ScaleFormatSize[scaleFormat] * totalFrames;
    const int s_TO = s_RO + s_RotationFormatOverhead[rotationFormat] + s_RotationFormatSize[rotationFormat] * totalFrames;

    if (rotationFormat == PRECISION_16) {
        uint16_t* pR = &((uint16_t*)(data + s_RO))[frame << 2];
        float     TempX = ((float)pR[0] * (2.0f / 65535.0f)) - 1.0f;
        float     TempY = ((float)pR[1] * (2.0f / 65535.0f)) - 1.0f;
        float     TempZ = ((float)pR[2] * (2.0f / 65535.0f)) - 1.0f;
        float     TempW = ((float)pR[3] * (2.0f / 65535.0f)) - 1.0f;
        Key.rotation.x = TempX;
        Key.rotation.y = TempY;
        Key.rotation.z = TempZ;
        Key.rotation.w = TempW;
    } else if (rotationFormat == CONSTANT_VALUE) {
        Key.rotation.identity();
    } else if (rotationFormat == SINGLE_VALUE) {
        Key.rotation = ((Quaternion*)(data + s_RO))[0];
    } else if (rotationFormat == PRECISION_32) {
        Key.rotation = ((Quaternion*)(data + s_RO))[frame];
    } else {
        assert(false);
    }
}
