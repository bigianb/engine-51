#include "Bitmap.h"

Bitmap::Bitmap()
{
    pixelData = nullptr;
    clutData = nullptr;
}

Bitmap::~Bitmap()
{
    delete[] pixelData;
    delete[] clutData;
}

bool Bitmap::readFile(uint8_t* fileData, int len)
{
        int32_t* p = (int32_t*)fileData;
        dataSize = p[0];
        clutSize = p[1];
        width = p[2];
        height = p[3];
        physicalWidth = p[4];
        flags = ((uint32_t*)p)[5];
        nMips = p[6];
        format = p[7];

        pixelData = new uint8_t[dataSize];
        memcpy(pixelData, fileData + 4*8, dataSize);
        if (clutSize > 0){
            clutData = new uint8_t[clutSize];
            memcpy(clutData, fileData + 4*8 + dataSize, clutSize);
        }

        return true;
}

void Bitmap::describe(std::ostringstream& ss)
{
    ss << "Width x Height: " << width << " x " << height << std::endl;
    ss << "Data Size: " << dataSize << std::endl;
    ss << "CLUT Size: " << clutSize << std::endl;
    ss << "num Mips: " << nMips << std::endl;
    ss << "Physical Pixel Width: " << physicalWidth << std::endl;
    ss << "Format: " << format << std::endl;
    ss << "Flags: 0x" << std::hex << flags << std::dec << std::endl;
}
