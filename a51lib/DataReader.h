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
        if (cursor + 4 <= len) {
            float f = *(float*)(fileData + cursor);
            cursor += 4;
            return f;
        }
        return 0.0;
    }

    int16_t readInt16()
    {
        if (cursor + 2 <= len) {
            int16_t i = *(int16_t*)(fileData + cursor);
            cursor += 2;
            return i;
        }
        return 0;
    }

    uint16_t readUInt16()
    {
        if (cursor + 2 <= len) {
            uint16_t i = *(uint16_t*)(fileData + cursor);
            cursor += 2;
            return i;
        }
        return 0;
    }

    int32_t readInt32()
    {
        if (cursor + 4 <= len) {
            int32_t i = *(int32_t*)(fileData + cursor);
            cursor += 4;
            return i;
        }
        return 0;
    }

    uint32_t readUInt32()
    {
        if (cursor + 4 <= len) {
            uint32_t i = *(uint32_t*)(fileData + cursor);
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

    void read(Vector4& o){
        o.x = readFloat();
        o.y = readFloat();
        o.z = readFloat();
        o.w = readFloat();
    }

    void read(BBox& o){
        read(o.min);
        read(o.max);
    }

    void read(Matrix4& m4){
        read(m4.vCol0);
        read(m4.vCol1);
        read(m4.vCol2);
        read(m4.vCol3);
    }

    const char* readString()
    {
        return (const char*)(fileData + cursor);
    }

    std::string consumeString()
    {
        std::string s = readString();
        while (hasData() && fileData[cursor]) { ++cursor; }
        if (hasData()){ ++cursor; }
        return s;
    }

    std::wstring consumeWString()
    {
        std::wstring s;
        uint16_t c;
        // Can't simply cast because wchar is 32 bit in linux and 16 bit on windows.
        while ((c = readUInt16())) {
            s += c;
        }
        return s;
    }

    void skip(int n)
    {
        cursor += n;
    }

    bool hasData(int n = 1){
        return (cursor + n) <= len;
    }

    uint8_t* fileData;
    int      len;
    int      cursor;
};
