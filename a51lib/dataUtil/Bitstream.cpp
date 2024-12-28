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
