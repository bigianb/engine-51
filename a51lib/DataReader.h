#pragma once
#include <cstdint>
#include "VectorMath.h"

class DataReader
{
public:
    DataReader(uint8_t* data, int l)
        : fileData(data)
        , len(l)
        , cursor(0)
    {
    }

    float readFloat()
    {
        if (cursor + 4 < len) {
            float f = *(float*)(fileData + cursor);
            cursor += 4;
            return f;
        }
        return 0.0;
    }

    int16_t readInt16()
    {
        if (cursor + 2 < len) {
            int16_t i = *(int16_t*)(fileData + cursor);
            cursor += 2;
            return i;
        }
        return 0;
    }

    uint16_t readUInt16()
    {
        if (cursor + 2 < len) {
            uint16_t i = *(uint16_t*)(fileData + cursor);
            cursor += 2;
            return i;
        }
        return 0;
    }

    int32_t readInt32()
    {
        if (cursor + 4 < len) {
            int32_t i = *(int32_t*)(fileData + cursor);
            cursor += 4;
            return i;
        }
        return 0;
    }

    void read(Vector3& o){
        o.x = readFloat();
        o.y = readFloat();
        o.z = readFloat();
        o.w = readFloat();
    }

    void read(BBox& o){
        read(o.min);
        read(o.max);
    }

    const char* readString()
    {
        return (const char*)(fileData + cursor);
    }

    void skip(int n)
    {
        cursor += n;
    }

    uint8_t* fileData;
    int      len;
    int      cursor;
};
