#pragma once

#include <cstdint>
#include <iostream>
#include <sstream>

class Colour;

class Bitmap
{
public:
    Bitmap();
    ~Bitmap();

    bool readFile(uint8_t* fileData, int len);

    void describe(std::ostringstream& ss);

    enum Format
    {
        FMT_NULL = 0,

        FMT_32_RGBA_8888 = 1,
        FMT_32_BGRA_8888 = 13,
        FMT_32_RGBU_8888 = 2,
        FMT_32_BGRU_8888 = 14,
        FMT_32_ARGB_8888 = 3,
        FMT_32_ABGR_8888 = 15,
        FMT_32_URGB_8888 = 4,
        FMT_32_UBGR_8888 = 16,
        FMT_24_RGB_888 = 5,
        FMT_24_BGR_888 = 17,
        FMT_16_RGBA_4444 = 6,
        FMT_16_BGRA_4444 = 18,
        FMT_16_ARGB_4444 = 7,
        FMT_16_ABGR_4444 = 19,
        FMT_16_RGBA_5551 = 8,
        FMT_16_BGRA_5551 = 20,
        FMT_16_RGBU_5551 = 9,
        FMT_16_BGRU_5551 = 21,
        FMT_16_ARGB_1555 = 10,
        FMT_16_ABGR_1555 = 22,
        FMT_16_URGB_1555 = 11,
        FMT_16_UBGR_1555 = 23,
        FMT_16_RGB_565 = 12,
        FMT_16_BGR_565 = 24,

        FMT_P8_RGBA_8888 = 25,
        FMT_P8_BGRA_8888 = 37,
        FMT_P8_RGBU_8888 = 26,
        FMT_P8_BGRU_8888 = 38,
        FMT_P8_ARGB_8888 = 27,
        FMT_P8_ABGR_8888 = 39,
        FMT_P8_URGB_8888 = 28,
        FMT_P8_UBGR_8888 = 40,
        FMT_P8_RGB_888 = 29,
        FMT_P8_BGR_888 = 41,
        FMT_P8_RGBA_4444 = 30,
        FMT_P8_BGRA_4444 = 42,
        FMT_P8_ARGB_4444 = 31,
        FMT_P8_ABGR_4444 = 43,
        FMT_P8_RGBA_5551 = 32,
        FMT_P8_BGRA_5551 = 44,
        FMT_P8_RGBU_5551 = 33,
        FMT_P8_BGRU_5551 = 45,
        FMT_P8_ARGB_1555 = 34,
        FMT_P8_ABGR_1555 = 46,
        FMT_P8_URGB_1555 = 35,
        FMT_P8_UBGR_1555 = 47,
        FMT_P8_RGB_565 = 36,
        FMT_P8_BGR_565 = 48,

        FMT_P4_RGBA_8888 = 49,
        FMT_P4_BGRA_8888 = 61,
        FMT_P4_RGBU_8888 = 50,
        FMT_P4_BGRU_8888 = 62,
        FMT_P4_ARGB_8888 = 51,
        FMT_P4_ABGR_8888 = 63,
        FMT_P4_URGB_8888 = 52,
        FMT_P4_UBGR_8888 = 64,
        FMT_P4_RGB_888 = 53,
        FMT_P4_BGR_888 = 65,
        FMT_P4_RGBA_4444 = 54,
        FMT_P4_BGRA_4444 = 66,
        FMT_P4_ARGB_4444 = 55,
        FMT_P4_ABGR_4444 = 67,
        FMT_P4_RGBA_5551 = 56,
        FMT_P4_BGRA_5551 = 68,
        FMT_P4_RGBU_5551 = 57,
        FMT_P4_BGRU_5551 = 69,
        FMT_P4_ARGB_1555 = 58,
        FMT_P4_ABGR_1555 = 70,
        FMT_P4_URGB_1555 = 59,
        FMT_P4_UBGR_1555 = 71,
        FMT_P4_RGB_565 = 60,
        FMT_P4_BGR_565 = 72,

        // DXT Formats
        // DXT1 = 4-bit RGB plus 1 bit of alpha (punchthrough)
        // DXT2 = (NOT SUPPORTED) Like DXT3 with premultiplied alpha
        // DXT4 = (NOT SUPPORTED) Like DXT5 with premultiplied alpha
        // DXT3 = 4-bit RGB plus 4 bit alpha channel
        // DXT5 = 4-bit RGB plus 4-bit interpolated alpha
        FMT_DXT1 = 73,
        FMT_DXT2 = 74,
        FMT_DXT3 = 75,
        FMT_DXT4 = 76,
        FMT_DXT5 = 77,

        // Intensity formats

        FMT_A8 = 78, // used for pure alpha textures

        // TERMINATOR

        FMT_END_OF_LIST,
    };

    struct FormatInfo
    {
        Format      format;    // Format
        const char* string;    // String version of Format
        bool        ClutBased; // Is a clut used?
        int8_t      BPP;       // Bits Per Pixel
        int8_t      BPC;       // Bits Per Color
        int8_t      BitsUsed;  // Bits used
        int8_t      RBits;     // Bits of Red
        int8_t      GBits;     // Bits of Green
        int8_t      BBits;     // Bits of Blue
        int8_t      ABits;     // Bits of Alpha
        int8_t      UBits;     // Bits of Unused
        int8_t      RShiftR;   // Shift right for Red
        int8_t      GShiftR;   // Shift right for Green
        int8_t      BShiftR;   // Shift right for Blue
        int8_t      AShiftR;   // Shift right for Alpha
        int8_t      UShiftR;   // Shift right for Unused
        int8_t      RShiftL;   // Shift left  for Red
        int8_t      GShiftL;   // Shift left  for Green
        int8_t      BShiftL;   // Shift left  for Blue
        int8_t      AShiftL;   // Shift left  for Alpha
        int8_t      UShiftL;   // Shift left  for Unused
        uint32_t    RMask;     // Mask for Red
        uint32_t    GMask;     // Mask for Green
        uint32_t    BMask;     // Mask for Blue
        uint32_t    AMask;     // Mask for Alpha
        uint32_t    UMask;     // Mask for Unused
    };

    static const FormatInfo formatInfo[FMT_END_OF_LIST];

    void     convertFormat(Format DestinationFormat);
    uint8_t* getPixelData(int mip) const;

private:
    void    unswizzlePS2Clut();
    void    unflip4BitNibbles();
    void    decodeDXT1ToColour(const uint8_t* source, Colour* dest);
    Colour* decodeToColor(const uint8_t* source, Format sourceFormat, int count);
    Colour* decodeIndexToColor(const uint8_t* source,
                               const Colour*  palette,
                               int            sourceBitsPer,
                               int            count);
    void    encodeFromColor(uint8_t*      destination,
                            Format        destinationFormat,
                            const Colour* source,
                            int           count);
    int     getBPP()
    {
        return formatInfo[format].BPP;
    }

    int getMipDataSize(int mip = 0) const;

public:
    int          dataSize;
    int          clutSize;
    int          width;
    int          height;
    int          physicalWidth;
    unsigned int flags;
    int          nMips;
    int          format;

    struct Mip
    {
        int32_t offset; // offset into pixelData
        int16_t width;  // Width  of mip in pixels
        int16_t height; // Height of mip in pixels
    };

    union
    {
        uint8_t* pixelData;
        Mip*     mipData;
    } data;

    uint8_t* clutData;
};

static_assert(sizeof(Bitmap::Mip) == 8, "Mip must be 8 bytes long");
