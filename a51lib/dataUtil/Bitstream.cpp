#include "Bitstream.h"
#include <cassert>

#define TO_U32(x) (*((int32_t*)(&(x))))
#define TO_F32(x) (*((float*)(&(x))))
#define LOWER_BITS(x) ((uint32_t)(~(-1 << (x))))

static int32_t s_VarLenBitOptions[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 32};

//=========================================================================

inline int GetHighestBit(uint32_t n)
{
    int b = 0;
    while (n) {
        b++;
        n >>= 1;
    }
    return b;
}

//=========================================================================

Bitstream::Bitstream(void)
{
    data = nullptr;
    dataSize = 0;
    dataSizeInBits = dataSize << 3;
    highestBitWritten = -1;
    cursor = 0;
    ownsData = true;
    maxGrowSize = 1024;
    sectionCursor = -1;
    overwrite = false;
}

//=========================================================================

Bitstream::~Bitstream(void)
{
    kill();
}

void Bitstream::kill(void)
{
    if (ownsData) {
        delete[] data;
    }
    data = nullptr;
    dataSize = 0;
    dataSizeInBits = 0;
    highestBitWritten = -1;
    cursor = 0;
    ownsData = false;
}

void Bitstream::init(int dataSizeIn)
{
    if (ownsData) {
        delete[] data;
    }
    dataSize = dataSizeIn;
    data = new uint8_t[dataSize];
    ownsData = true;
    dataSizeInBits = dataSize << 3;

    highestBitWritten = -1;
    cursor = 0;
    sectionCursor = -1;
}

void Bitstream::init(uint8_t* pData, int DataSize)
{
    ownsData = false;
    data = pData;
    dataSize = DataSize;
    dataSizeInBits = dataSize << 3;
    highestBitWritten = -1;
    cursor = 0;
}

void Bitstream::readRangedU32(uint32_t& value, int32_t min, int32_t max) const
{

    int range = max - min;
    int nBits = GetHighestBit(range);
    readU32(value, nBits);
    value += min;
}

uint32_t Bitstream::readRaw32(int nBits) const
{
    if (nBits == 0) {
        return 0;
    }

    int      leftOffset = (cursor & 0x7);
    int      rightOffset = (40 - leftOffset - nBits);
    uint8_t* src = data + (cursor >> 3);
    uint8_t* end = data + ((cursor + nBits - 1) >> 3) + 1;

    cursor += nBits;

    uint64_t readMask = (0xFFFFFFFFFFUL >> leftOffset) &
                        (0xFFFFFFFFFFUL << rightOffset);

    uint64_t dataMask = 0;

    // Read data
    int shift = 32;
    while (src != end) {
        dataMask |= ((uint64_t)(*src++)) << shift;
        shift -= 8;
    }

    return (uint32_t)((dataMask & readMask) >> rightOffset);
}

bool Bitstream::readFlag() const
{
    if (cursor >= dataSizeInBits) {
        return (false);
    }
    const bool flag = (*(data + (cursor >> 3)) & (1 << (7 - (cursor & 0x7)))) != 0;
    cursor++;
    return flag;
}

void Bitstream::readF32(float& value) const
{
    uint32_t uv = readRaw32(32);
    value = TO_F32(uv);
}

void Bitstream::readVariableLenS32(int32_t& value) const
{
    const bool neg = readFlag();

    uint32_t nBitsOption;
    readU32(nBitsOption, 4);
    uint32_t uv;
    readU32(uv, s_VarLenBitOptions[nBitsOption]);

    value = (int32_t)uv;

    if (neg) {
        value = -value;
    }
}

void Bitstream::readRangedF32(float& value, int nBits, float min, float max) const
{
    if (nBits == 32) {
        readF32(value);
        return;
    }
    assert(max > min);
    float    range = max - min;
    int      scalar = LOWER_BITS(nBits);
    uint32_t N;
    readU32(N, nBits);
    value = (((float)N) / scalar) * range + min;
    assert((value >= min) && (value <= max));
}

void Bitstream::readVector(Vector3& v) const
{
    readF32(v.x);
    readF32(v.y);
    readF32(v.z);
}

void Bitstream::readQuaternion(Quaternion& q) const
{
    readF32(q.x);
    readF32(q.y);
    readF32(q.z);
    readF32(q.w);
}
