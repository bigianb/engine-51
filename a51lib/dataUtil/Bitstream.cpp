#include "Bitstream.h"

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
    data = NULL;
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
