#pragma once

#include <cstdint>

struct InevFileHeader
{
    uint32_t inev;
    int32_t version;
    int32_t numStaticBytes;
    int32_t numTables;
    int32_t numDynamicBytes;
};

struct BBox;
struct Vector3;

class InevFile
{
public:
    InevFile()
    {
        header = nullptr;
        pStaticData = nullptr;
        pDynamicData = nullptr;
        cursor = 0;
    }

    bool init(uint8_t* fileData, int len);

    void read(Vector3&);
    void read(BBox&);
    void read(int16_t&);
    void read(uint16_t&);
    void read(float&);

private:
    InevFileHeader* header;
    uint8_t* pStaticData;
    uint8_t* pDynamicData;

    int cursor;
};
