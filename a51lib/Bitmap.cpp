#include "Bitmap.h"

#include <cstring>
#include "Colour.h"

Bitmap::Bitmap()
{
    data.pixelData = nullptr;
    clutData = nullptr;
}

Bitmap::~Bitmap()
{
    delete[] data.pixelData;
    delete[] clutData;
}

bool Bitmap::readFile(uint8_t* fileData, int len)
{
    delete[] data.pixelData; data.pixelData = nullptr;
    delete[] clutData; clutData = nullptr;
    
    int32_t* p = (int32_t*)fileData;
    dataSize = p[0];
    clutSize = p[1];
    width = p[2];
    height = p[3];
    physicalWidth = p[4];
    flags = ((uint32_t*)p)[5];
    nMips = p[6];
    format = p[7];

    data.pixelData = new uint8_t[dataSize];
    memcpy(data.pixelData, fileData + 4 * 8, dataSize);
    if (clutSize > 0) {
        clutData = new uint8_t[clutSize];
        memcpy(clutData, fileData + 4 * 8 + dataSize, clutSize);
    }

    return true;
}

// clang-format off

//                 Bits    -----Bits----    ----Shift Right---    --Shift Left-
//  BITS           Used    R  G  B  A  U     R   G   B   A   U    R  G  B  A  U
//                    |    |  |  |  |  |     |   |   |   |   |    |  |  |  |  |
#define B_RGBA_8888  32,   8, 8, 8, 8, 0,   24, 16,  8,  0,  0,   0, 0, 0, 0, 0
#define B_RGBU_8888  24,   8, 8, 8, 0, 8,   24, 16,  8,  0,  0,   0, 0, 0, 0, 0
#define B_ARGB_8888  32,   8, 8, 8, 8, 0,   16,  8,  0, 24,  0,   0, 0, 0, 0, 0
#define B_URGB_8888  24,   8, 8, 8, 0, 8,   16,  8,  0,  0, 24,   0, 0, 0, 0, 0
#define B_RGB_888    24,   8, 8, 8, 0, 0,   16,  8,  0,  0,  0,   0, 0, 0, 0, 0
#define B_RGBA_4444  16,   4, 4, 4, 4, 0,   12,  8,  4,  0,  0,   4, 4, 4, 4, 0
#define B_ARGB_4444  16,   4, 4, 4, 4, 0,    8,  4,  0, 12,  0,   4, 4, 4, 4, 0
#define B_RGBA_5551  16,   5, 5, 5, 1, 0,   11,  6,  1,  0,  0,   3, 3, 3, 7, 0
#define B_RGBU_5551  16,   5, 5, 5, 0, 1,   11,  6,  1,  0,  0,   3, 3, 3, 0, 7
#define B_ARGB_1555  16,   5, 5, 5, 1, 0,   10,  5,  0, 15,  0,   3, 3, 3, 7, 0
#define B_URGB_1555  16,   5, 5, 5, 0, 1,   10,  5,  0,  0, 15,   3, 3, 3, 0, 7
#define B_RGB_565    16,   5, 6, 5, 0, 0,   11,  5,  0,  0,  0,   3, 2, 3, 0, 0


#define B_BGRA_8888  32,   8, 8, 8, 8, 0,    8, 16, 24,  0,  0,   0, 0, 0, 0, 0
#define B_BGRU_8888  24,   8, 8, 8, 0, 8,    8, 16, 24,  0,  0,   0, 0, 0, 0, 0
#define B_ABGR_8888  32,   8, 8, 8, 8, 0,    0,  8, 16, 24,  0,   0, 0, 0, 0, 0
#define B_UBGR_8888  24,   8, 8, 8, 0, 8,    0,  8, 16,  0, 24,   0, 0, 0, 0, 0
#define B_BGR_888    24,   8, 8, 8, 0, 0,    0,  8, 16,  0,  0,   0, 0, 0, 0, 0
#define B_BGRA_4444  16,   4, 4, 4, 4, 0,    4,  8, 12,  0,  0,   4, 4, 4, 4, 0
#define B_ABGR_4444  16,   4, 4, 4, 4, 0,    0,  4,  8, 12,  0,   4, 4, 4, 4, 0
#define B_BGRA_5551  16,   5, 5, 5, 1, 0,    1,  6, 11,  0,  0,   3, 3, 3, 7, 0
#define B_BGRU_5551  16,   5, 5, 5, 0, 1,    1,  6, 11,  0,  0,   3, 3, 3, 0, 7
#define B_ABGR_1555  16,   5, 5, 5, 1, 0,    0,  5, 10, 15,  0,   3, 3, 3, 7, 0
#define B_UBGR_1555  16,   5, 5, 5, 0, 1,    0,  5, 10,  0, 15,   3, 3, 3, 0, 7
#define B_BGR_565    16,   5, 6, 5, 0, 0,    0,  5, 11,  0,  0,   3, 2, 3, 0, 0

#define B_A_8         8,   0, 0, 0, 8, 0,    0,  0,  0,  0,  0,   0, 0, 0, 0, 0

#define B_NULL      -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1

//
//  MASKS                 RMask       GMask       BMask       AMask       UMask
//                            |           |           |           |           |
#define M_RGBA_8888  0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000
#define M_RGBU_8888  0xFF000000, 0x00FF0000, 0x0000FF00, 0x00000000, 0x000000FF
#define M_ARGB_8888  0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000, 0x00000000
#define M_URGB_8888  0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, 0xFF000000
#define M_RGB_888    0x00FF0000, 0x0000FF00, 0x000000FF, 0x00000000, 0x00000000
#define M_RGBA_4444  0x0000F000, 0x00000F00, 0x000000F0, 0x0000000F, 0x00000000
#define M_ARGB_4444  0x00000F00, 0x000000F0, 0x0000000F, 0x0000F000, 0x00000000
#define M_RGBA_5551  0x0000F800, 0x000007C0, 0x0000003E, 0x00000001, 0x00000000
#define M_RGBU_5551  0x0000F800, 0x000007C0, 0x0000003E, 0x00000000, 0x00000001
#define M_ARGB_1555  0x00007C00, 0x000003E0, 0x0000001F, 0x00008000, 0x00000000
#define M_URGB_1555  0x00007C00, 0x000003E0, 0x0000001F, 0x00000000, 0x00008000
#define M_RGB_565    0x0000F800, 0x000007E0, 0x0000001F, 0x00000000, 0x00000000

#define M_BGRA_8888  0x0000FF00, 0x00FF0000, 0xFF000000, 0x000000FF, 0x00000000
#define M_BGRU_8888  0x0000FF00, 0x00FF0000, 0xFF000000, 0x00000000, 0x000000FF
#define M_ABGR_8888  0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000, 0x00000000
#define M_UBGR_8888  0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000, 0xFF000000
#define M_BGR_888    0x000000FF, 0x0000FF00, 0x00FF0000, 0x00000000, 0x00000000
#define M_BGRA_4444  0x000000F0, 0x00000F00, 0x0000F000, 0x0000000F, 0x00000000
#define M_ABGR_4444  0x0000000F, 0x000000F0, 0x00000F00, 0x0000F000, 0x00000000
#define M_BGRA_5551  0x0000003E, 0x000007C0, 0x0000F800, 0x00000001, 0x00000000
#define M_BGRU_5551  0x0000003E, 0x000007C0, 0x0000F800, 0x00000000, 0x00000001
#define M_ABGR_1555  0x0000001F, 0x000003E0, 0x00007C00, 0x00008000, 0x00000000
#define M_UBGR_1555  0x0000001F, 0x000003E0, 0x00007C00, 0x00000000, 0x00008000
#define M_BGR_565    0x0000001F, 0x000007E0, 0x0000F800, 0x00000000, 0x00000000

#define M_A_8        0x00000000, 0x00000000, 0x00000000, 0x000000FF, 0x00000000

#define M_NULL       0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF

const Bitmap::FormatInfo Bitmap::formatInfo[Bitmap::FMT_END_OF_LIST] =
    {
        //  Format            String        CLUT? BPP BPC  BITS         MASKS 

        {FMT_NULL,          "NULL FORMAT",  false, -1, -1, B_NULL,      M_NULL},

        {FMT_32_RGBA_8888,  "32_RGBA_8888", false, 32, 32, B_RGBA_8888, M_RGBA_8888},
        {FMT_32_RGBU_8888,  "32_RGBU_8888", false, 32, 32, B_RGBU_8888, M_RGBU_8888},
        {FMT_32_ARGB_8888,  "32_ARGB_8888", false, 32, 32, B_ARGB_8888, M_ARGB_8888},
        {FMT_32_URGB_8888,  "32_URGB_8888", false, 32, 32, B_URGB_8888, M_URGB_8888},
        {FMT_24_RGB_888,    "24_RGB_888",   false, 24, 24, B_RGB_888,   M_RGB_888},
        {FMT_16_RGBA_4444,  "16_RGBA_4444", false, 16, 16, B_RGBA_4444, M_RGBA_4444},
        {FMT_16_ARGB_4444,  "16_ARGB_4444", false, 16, 16, B_ARGB_4444, M_ARGB_4444},
        {FMT_16_RGBA_5551,  "16_RGBA_5551", false, 16, 16, B_RGBA_5551, M_RGBA_5551},
        {FMT_16_RGBU_5551,  "16_RGBU_5551", false, 16, 16, B_RGBU_5551, M_RGBU_5551},
        {FMT_16_ARGB_1555,  "16_ARGB_1555", false, 16, 16, B_ARGB_1555, M_ARGB_1555},
        {FMT_16_URGB_1555,  "16_URGB_1555", false, 16, 16, B_URGB_1555, M_URGB_1555},
        {FMT_16_RGB_565,    "16_RGB_565",   false, 16, 16, B_RGB_565,   M_RGB_565},

        {FMT_32_BGRA_8888,  "32_BGRA_8888", false, 32, 32, B_BGRA_8888, M_BGRA_8888},
        {FMT_32_BGRU_8888,  "32_BGRU_8888", false, 32, 32, B_BGRU_8888, M_BGRU_8888},
        {FMT_32_ABGR_8888,  "32_ABGR_8888", false, 32, 32, B_ABGR_8888, M_ABGR_8888},
        {FMT_32_UBGR_8888,  "32_UBGR_8888", false, 32, 32, B_UBGR_8888, M_UBGR_8888},
        {FMT_24_BGR_888,    "24_BGR_888",   false, 24, 24, B_BGR_888,   M_BGR_888},
        {FMT_16_BGRA_4444,  "16_BGRA_4444", false, 16, 16, B_BGRA_4444, M_BGRA_4444},
        {FMT_16_ABGR_4444,  "16_ABGR_4444", false, 16, 16, B_ABGR_4444, M_ABGR_4444},
        {FMT_16_BGRA_5551,  "16_BGRA_5551", false, 16, 16, B_BGRA_5551, M_BGRA_5551},
        {FMT_16_BGRU_5551,  "16_BGRU_5551", false, 16, 16, B_BGRU_5551, M_BGRU_5551},
        {FMT_16_ABGR_1555,  "16_ABGR_1555", false, 16, 16, B_ABGR_1555, M_ABGR_1555},
        {FMT_16_UBGR_1555,  "16_UBGR_1555", false, 16, 16, B_UBGR_1555, M_UBGR_1555},
        {FMT_16_BGR_565,    "16_BGR_565",   false, 16, 16, B_BGR_565,   M_BGR_565},

        {FMT_P8_RGBA_8888,  "P8_RGBA_8888", true,   8, 32, B_RGBA_8888, M_RGBA_8888},
        {FMT_P8_RGBU_8888,  "P8_RGBU_8888", true,   8, 32, B_RGBU_8888, M_RGBU_8888},
        {FMT_P8_ARGB_8888,  "P8_ARGB_8888", true,   8, 32, B_ARGB_8888, M_ARGB_8888},
        {FMT_P8_URGB_8888,  "P8_URGB_8888", true,   8, 32, B_URGB_8888, M_URGB_8888},
        {FMT_P8_RGB_888,    "P8_RGB_888",   true,   8, 24, B_RGB_888,   M_RGB_888},
        {FMT_P8_RGBA_4444,  "P8_RGBA_4444", true,   8, 16, B_RGBA_4444, M_RGBA_4444},
        {FMT_P8_ARGB_4444,  "P8_ARGB_4444", true,   8, 16, B_ARGB_4444, M_ARGB_4444},
        {FMT_P8_RGBA_5551,  "P8_RGBA_5551", true,   8, 16, B_RGBA_5551, M_RGBA_5551},
        {FMT_P8_RGBU_5551,  "P8_RGBU_5551", true,   8, 16, B_RGBU_5551, M_RGBU_5551},
        {FMT_P8_ARGB_1555,  "P8_ARGB_1555", true,   8, 16, B_ARGB_1555, M_ARGB_1555},
        {FMT_P8_URGB_1555,  "P8_URGB_1555", true,   8, 16, B_URGB_1555, M_URGB_1555},
        {FMT_P8_RGB_565,    "P8_RGB_565",   true,   8, 16, B_RGB_565,   M_RGB_565},

        {FMT_P8_BGRA_8888,  "P8_BGRA_8888", true,   8, 32, B_BGRA_8888, M_BGRA_8888},
        {FMT_P8_BGRU_8888,  "P8_BGRU_8888", true,   8, 32, B_BGRU_8888, M_BGRU_8888},
        {FMT_P8_ABGR_8888,  "P8_ABGR_8888", true,   8, 32, B_ABGR_8888, M_ABGR_8888},
        {FMT_P8_UBGR_8888,  "P8_UBGR_8888", true,   8, 32, B_UBGR_8888, M_UBGR_8888},
        {FMT_P8_BGR_888,    "P8_BGR_888",   true,   8, 24, B_BGR_888,   M_BGR_888},
        {FMT_P8_BGRA_4444,  "P8_BGRA_4444", true,   8, 16, B_BGRA_4444, M_BGRA_4444},
        {FMT_P8_ABGR_4444,  "P8_ABGR_4444", true,   8, 16, B_ABGR_4444, M_ABGR_4444},
        {FMT_P8_BGRA_5551,  "P8_BGRA_5551", true,   8, 16, B_BGRA_5551, M_BGRA_5551},
        {FMT_P8_BGRU_5551,  "P8_BGRU_5551", true,   8, 16, B_BGRU_5551, M_BGRU_5551},
        {FMT_P8_ABGR_1555,  "P8_ABGR_1555", true,   8, 16, B_ABGR_1555, M_ABGR_1555},
        {FMT_P8_UBGR_1555,  "P8_UBGR_1555", true,   8, 16, B_UBGR_1555, M_UBGR_1555},
        {FMT_P8_BGR_565,    "P8_BGR_565",   true,   8, 16, B_BGR_565, M_BGR_565},

        {FMT_P4_RGBA_8888,  "P4_RGBA_8888", true,   4, 32, B_RGBA_8888, M_RGBA_8888},
        {FMT_P4_RGBU_8888,  "P4_RGBU_8888", true,   4, 32, B_RGBU_8888, M_RGBU_8888},
        {FMT_P4_ARGB_8888,  "P4_ARGB_8888", true,   4, 32, B_ARGB_8888, M_ARGB_8888},
        {FMT_P4_URGB_8888,  "P4_URGB_8888", true,   4, 32, B_URGB_8888, M_URGB_8888},
        {FMT_P4_RGB_888,    "P4_RGB_888",   true,   4, 24, B_RGB_888,   M_RGB_888},
        {FMT_P4_RGBA_4444,  "P4_RGBA_4444", true,   4, 16, B_RGBA_4444, M_RGBA_4444},
        {FMT_P4_ARGB_4444,  "P4_ARGB_4444", true,   4, 16, B_ARGB_4444, M_ARGB_4444},
        {FMT_P4_RGBA_5551,  "P4_RGBA_5551", true,   4, 16, B_RGBA_5551, M_RGBA_5551},
        {FMT_P4_RGBU_5551,  "P4_RGBU_5551", true,   4, 16, B_RGBU_5551, M_RGBU_5551},
        {FMT_P4_ARGB_1555,  "P4_ARGB_1555", true,   4, 16, B_ARGB_1555, M_ARGB_1555},
        {FMT_P4_URGB_1555,  "P4_URGB_1555", true,   4, 16, B_URGB_1555, M_URGB_1555},
        {FMT_P4_RGB_565,    "P4_RGB_565",   true,   4, 16, B_RGB_565,   M_RGB_565},

        {FMT_P4_BGRA_8888,  "P4_BGRA_8888", true,   4, 32, B_BGRA_8888, M_BGRA_8888},
        {FMT_P4_BGRU_8888,  "P4_BGRU_8888", true,   4, 32, B_BGRU_8888, M_BGRU_8888},
        {FMT_P4_ABGR_8888,  "P4_ABGR_8888", true,   4, 32, B_ABGR_8888, M_ABGR_8888},
        {FMT_P4_UBGR_8888,  "P4_UBGR_8888", true,   4, 32, B_UBGR_8888, M_UBGR_8888},
        {FMT_P4_BGR_888,    "P4_BGR_888",   true,   4, 24, B_BGR_888,   M_BGR_888},
        {FMT_P4_BGRA_4444,  "P4_BGRA_4444", true,   4, 16, B_BGRA_4444, M_BGRA_4444},
        {FMT_P4_ABGR_4444,  "P4_ABGR_4444", true,   4, 16, B_ABGR_4444, M_ABGR_4444},
        {FMT_P4_BGRA_5551,  "P4_BGRA_5551", true,   4, 16, B_BGRA_5551, M_BGRA_5551},
        {FMT_P4_BGRU_5551,  "P4_BGRU_5551", true,   4, 16, B_BGRU_5551, M_BGRU_5551},
        {FMT_P4_ABGR_1555,  "P4_ABGR_1555", true,   4, 16, B_ABGR_1555, M_ABGR_1555},
        {FMT_P4_UBGR_1555,  "P4_UBGR_1555", true,   4, 16, B_UBGR_1555, M_UBGR_1555},
        {FMT_P4_BGR_565,    "P4_BGR_565",   true,   4, 16, B_BGR_565,   M_BGR_565},

        {FMT_DXT1, "DXT1", false, 4, -1, B_NULL, M_NULL},
        {FMT_DXT2, "DXT2", false, 4, -1, B_NULL, M_NULL},
        {FMT_DXT3, "DXT3", false, 8, -1, B_NULL, M_NULL},
        {FMT_DXT4, "DXT4", false, 8, -1, B_NULL, M_NULL},
        {FMT_DXT5, "DXT5", false, 8, -1, B_NULL, M_NULL},

        {FMT_A8, "A8", false, 8, -1, B_A_8, M_A_8},
};

#define FLAG_VALID                (1 << 0)
#define FLAG_DATA_OWNED           (1 << 1)
#define FLAG_CLUT_OWNED           (1 << 2)

#define FLAG_PS2_CLUT_SWIZZLED    (1 << 8)
#define FLAG_4BIT_NIBBLES_FLIPPED (1 << 9)

#define FLAG_GCN_DATA_SWIZZLED    (1 << 11)

#define FLAG_XBOX_DATA_SWIZZLED   (1 << 12)
#define FLAG_XBOX_PRE_REGISTERED  (1 << 13)

// clang-format on

void Bitmap::unswizzlePS2Clut()
{
    if (!(flags & FLAG_PS2_CLUT_SWIZZLED)) {
        return;
    }

    if (getBPP() == 8) {
        uint32_t  unswizzled[256];
        uint32_t* swizzled = (uint32_t*)clutData;

        int idx = 0;
        for (int i = 0; i < 256; i += 32) {
            for (int j = i; j < i + 8; j++) {
                unswizzled[idx++] = swizzled[j];
            }
            for (int j = i + 16; j < i + 16 + 8; j++) {
                unswizzled[idx++] = swizzled[j];
            }
            for (int j = i + 8; j < i + 8 + 8; j++) {
                unswizzled[idx++] = swizzled[j];
            }
            for (int j = i + 24; j < i + 24 + 8; j++) {
                unswizzled[idx++] = swizzled[j];
            }
        }

        for (int i = 0; i < 256; i++) {
            swizzled[i] = unswizzled[i];
        }

        flags &= ~FLAG_PS2_CLUT_SWIZZLED;
    }
}

uint8_t* Bitmap::getPixelData(int mip) const
{
    if (mip < 0 || nMips == 0) {
        return data.pixelData;
    }
    return data.pixelData + data.mipData[mip].offset;
}

int Bitmap::getMipDataSize(int mip) const
{
    if (nMips == 0) {
        return dataSize;
    }
    return (data.mipData[mip].width * data.mipData[mip].height * formatInfo[format].BPP) >> 3;
}

void Bitmap::unflip4BitNibbles()
{
    if (getBPP() != 4 || !(flags & FLAG_4BIT_NIBBLES_FLIPPED)) {
        return;
    }

    // Flip those nibbles
    for (int mipNo = 0; mipNo < nMips + 1; mipNo++) {

        uint8_t* mipData = getPixelData(mipNo);
        uint32_t mipDataSize = getMipDataSize(mipNo);

        for (int i = 0; i < mipDataSize; i++) {
            int n0 = (mipData[i] >> 4) & 0x0F;
            int n1 = (mipData[i] >> 0) & 0x0F;
            mipData[i] = (n1 << 4) | (n0 << 0);
        }
    }

    flags &= ~FLAG_4BIT_NIBBLES_FLIPPED;
}

void Bitmap::convertFormat(Format destinationFormat)
{
    // note, we don't support converting to a palettised format.
    // there is no need on modern systems.
    if (destinationFormat == format) {
        // nothing to do
        return;
    }

    // Unswizzle as needed.
    if (flags & FLAG_PS2_CLUT_SWIZZLED) {
        unswizzlePS2Clut();
    }

    // Unflip as needed
    if (flags & FLAG_4BIT_NIBBLES_FLIPPED) {
        unflip4BitNibbles();
    }

    const FormatInfo& oldFormat = formatInfo[format];
    const FormatInfo& newFormat = formatInfo[destinationFormat];

    int      dataPixels = physicalWidth * height;
    int      dataBytes = (dataPixels * newFormat.BPP) >> 3;
    uint8_t* newData = new uint8_t[dataBytes];
    uint8_t* oldData = data.pixelData;
    oldData += nMips ? data.mipData[0].offset : 0;

    uint8_t* oldPalette = nullptr;
    uint8_t* newPalette = nullptr;
    Colour*  colorPalette = nullptr;
    int      numPaletteEntries = 0;
    Colour*  colorData = nullptr;

    if (oldFormat.ClutBased) {
        numPaletteEntries = clutSize / (oldFormat.BPC >> 3);
        oldPalette = clutData;
        colorPalette = decodeToColor(oldPalette, (Format)format, numPaletteEntries);
    } else {
        colorData = decodeToColor(oldData, (Format)format, dataPixels);
    }

    if ((oldFormat.ClutBased) && !(newFormat.ClutBased)) {
        colorData = decodeIndexToColor(oldData, colorPalette, oldFormat.BPP, dataPixels);
        delete[] colorPalette;
        colorPalette = nullptr;
    }

    if (!(newFormat.ClutBased)) {
        encodeFromColor(newData, destinationFormat, colorData, dataPixels);
        delete[] colorData;
        colorData = nullptr;
    }

    delete[] data.pixelData;

    data.pixelData = newData;
    dataSize = dataBytes;
    nMips = 0;
    flags |= FLAG_DATA_OWNED;

    delete[] clutData;
    clutData = nullptr;
    clutSize = 0;
    flags &= ~FLAG_CLUT_OWNED;
    format = destinationFormat;

    delete[] colorData;
    delete[] colorPalette;
}

Colour* Bitmap::decodeIndexToColor(const uint8_t* source,
                                   const Colour*  palette,
                                   int            sourceBitsPer,
                                   int            count)
{
    Colour* output = new Colour[count];

    auto* pw = output;
    auto* pr = source;

    if (sourceBitsPer == 4) {
        while (count > 0) {
            *pw++ = palette[*pr >> 4];
            *pw++ = palette[*pr & 0x0F];
            pr += 1;
            count -= 2;
        }
    } else {
        while (count > 0) {
            *pw++ = palette[*pr++];
            count--;
        }
    }

    return output;
}

void Bitmap::encodeFromColor(uint8_t*      destination,
                             Format        destinationFormat,
                             const Colour* source,
                             int           count)
{
    const auto* pr = source;
    const auto& destFormat = formatInfo[destinationFormat];

    uint32_t R, G, B, A;

    auto U = destFormat.UMask;

    if (destFormat.BPC == 32) {
        uint32_t* pw = (uint32_t*)destination;

        while (count-- > 0) {
            R = pr->r;
            G = pr->g;
            B = pr->b;
            A = pr->a;

            R >>= destFormat.RShiftL;
            R <<= destFormat.RShiftR;

            G >>= destFormat.GShiftL;
            G <<= destFormat.GShiftR;

            B >>= destFormat.BShiftL;
            B <<= destFormat.BShiftR;

            A >>= destFormat.AShiftL;
            A <<= destFormat.AShiftR;
            A &= destFormat.AMask;

            *pw++ = R | G | B | A | U;

            pr++;
        }
    } else if (destFormat.BPC == 24) {
        auto* pw = destination;

        if (destFormat.RShiftR == 16) {
            while (count-- > 0) {
                *pw++ = pr->r;
                *pw++ = pr->g;
                *pw++ = pr->b;
                pr++;
            }
        } else if (destFormat.RShiftR == 0) {
            while (count-- > 0) {
                *pw++ = pr->b;
                *pw++ = pr->g;
                *pw++ = pr->r;
                pr++;
            }
        }
    } else if (destFormat.BPC == 16) {
        auto* pWrite = (uint16_t*)destination;

        while (count-- > 0) {
            R = pr->r;
            G = pr->g;
            B = pr->b;
            A = pr->a;

            R >>= destFormat.RShiftL;
            R <<= destFormat.RShiftR;

            G >>= destFormat.GShiftL;
            G <<= destFormat.GShiftR;

            B >>= destFormat.BShiftL;
            B <<= destFormat.BShiftR;

            A >>= destFormat.AShiftL;
            A <<= destFormat.AShiftR;
            A &= destFormat.AMask;

            *pWrite++ = (uint16_t)(R | G | B | A | U);

            pr++;
        }
    }
}

Colour* Bitmap::decodeToColor(const uint8_t* source, Format sourceFormat, int count)
{
    const FormatInfo& sourceFormatInfo = formatInfo[sourceFormat];

    Colour* output = new Colour[count];
    Colour* pw = output;
    if (sourceFormatInfo.BPC == 32) {
        uint32_t* source32 = (uint32_t*)source;

        while (count > 0) {
            uint32_t v = *source32++;
            pw->r = (uint8_t)((v & sourceFormatInfo.RMask) >> sourceFormatInfo.RShiftR);
            pw->g = (uint8_t)((v & sourceFormatInfo.GMask) >> sourceFormatInfo.GShiftR);
            pw->b = (uint8_t)((v & sourceFormatInfo.BMask) >> sourceFormatInfo.BShiftR);
            pw->a = (uint8_t)((v & sourceFormatInfo.AMask) >> sourceFormatInfo.AShiftR);
            pw->a |= (uint8_t)((v & sourceFormatInfo.UMask) >> sourceFormatInfo.UShiftR);

            pw++;
            count--;
        }
    } else if (sourceFormatInfo.BPC == 24) {
        if (sourceFormatInfo.RShiftR == 16) {
            while (count > 0) {
                pw->r = *source++;
                pw->g = *source++;
                pw->b = *source++;
                pw->a = 255;
                pw++;
                count--;
            }
        } else {
            while (count > 0) {
                pw->b = *source++;
                pw->g = *source++;
                pw->r = *source++;
                pw->a = 255;
                pw++;
                count--;
            }
        }
    } else {
        // 16 BPP
        while (count > 0) {
            uint16_t* source16 = (uint16_t*)source;

            uint32_t R = *source16;
            R &= sourceFormatInfo.RMask;
            R >>= sourceFormatInfo.RShiftR;
            R <<= sourceFormatInfo.RShiftL;
            R |= (R >> sourceFormatInfo.RBits);

            uint32_t G = *source16;
            G &= sourceFormatInfo.GMask;
            G >>= sourceFormatInfo.GShiftR;
            G <<= sourceFormatInfo.GShiftL;
            G |= (G >> sourceFormatInfo.GBits);

            uint32_t B = *source16;
            B &= sourceFormatInfo.BMask;
            B >>= sourceFormatInfo.BShiftR;
            B <<= sourceFormatInfo.BShiftL;
            B |= (B >> sourceFormatInfo.BBits);

            uint32_t A = 0xFF;
            if (sourceFormatInfo.ABits) {
                A = *source16;
                A &= sourceFormatInfo.AMask;
                A >>= sourceFormatInfo.AShiftR;
                A <<= sourceFormatInfo.AShiftL;

                int aBits = sourceFormatInfo.ABits;
                while (aBits < 8) {
                    A |= (A >> aBits);
                    aBits <<= 1;
                }
            }

            pw->set((uint8_t)R, (uint8_t)G, (uint8_t)B, (uint8_t)A);

            ++source16;
            pw++;
            count--;
        }
    }
    return output;
}

void Bitmap::describe(std::ostringstream& ss)
{
    ss << "Width x Height: " << width << " x " << height << std::endl;
    ss << "Data Size: " << dataSize << std::endl;
    ss << "CLUT Size: " << clutSize << std::endl;
    ss << "num Mips: " << nMips << std::endl;
    ss << "Physical Pixel Width: " << physicalWidth << std::endl;
    ss << "Format: " << format << ", " << formatInfo[format].string << std::endl;
    ss << "Flags: 0x" << std::hex << flags << std::dec << std::endl;
}
