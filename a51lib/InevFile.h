#pragma once

#include <cstdint>
#include <cstring>
#include <sstream>
#include <type_traits>

#include "VectorMath.h"

struct InevFileHeader
{
    uint32_t inev;
    int32_t  version;
    int32_t  numStaticBytes;
    int32_t  numTables;
    int32_t  numDynamicBytes;
};

class InevFile
{
public:
    InevFile()
    {
        header = nullptr;
        pStaticData = nullptr;
        pDynamicData = nullptr;
        cursor = 0;
        platform = 1;
    }

    bool isPC()
    {
        return platform == 1;
    }
    bool isPS2()
    {
        return platform == 4;
    }
    bool isXBOX()
    {
        return platform == 8;
    }

    void setPlatform(int pf)
    {
        platform = pf;
    }

    void describe(std::ostringstream& ss);

    bool init(uint8_t* fileData, int len);

    void read(Quaternion&);
    void read(Vector3&);
    void read(BBox&);
    void read(int8_t&);
    void read(uint8_t&);
    void read(int16_t&);
    void read(uint16_t&);
    void read(float&);
    void read(int&);
    void read(unsigned int&);
    void read(uint64_t&);

    template <class T>
    void read(T& obj)
    {
        obj.read(*this);
    }

    int readAndResolvePtr();

    template <typename T>
    void readArray(T& ptr, int size)
    {
        if (size == 0) {
            cursor += 4;
            return;
        }
        ptr = new std::remove_pointer_t<T>[size];
        // This is the offset in the file where the data is.
        // Need to convert it to a pointer via the table.
        int dataOffset = readAndResolvePtr();
        int startCursor = cursor;
        cursor = dataOffset;
        for (int i = 0; i < size; ++i) {
            ptr[i].read(*this);
        }
        cursor = startCursor;
    }

    template <typename T>
    void readNativeArray(T& ptr, int size)
    {
        if (size == 0) {
            cursor += 4;
            return;
        }
        ptr = new std::remove_pointer_t<T>[size];
        int dataOffset = readAndResolvePtr();
        int startCursor = cursor;
        cursor = dataOffset;
        for (int i = 0; i < size; ++i) {
            read(ptr[i]);
        }
        cursor = startCursor;
    }

    void readNativeArray(char*& ptr, int size)
    {
        if (size == 0) {
            cursor += 4;
            return;
        }
        int dataOffset = readAndResolvePtr();
        int startCursor = cursor;
        cursor = dataOffset;
        ptr = new char[size];
        for (int i = 0; i < size; ++i) {
            memcpy(ptr, pStaticData + cursor, size);
            cursor += size;
        }
        cursor = startCursor;
    }

private:
    struct Ref
    {
        int32_t  offset;     // Byte offset where the pointer lives
        int32_t  count;      // Count of entries that this pointer is pointing to
        int32_t  pointingAT; // What part of the file is this pointer pointing to
        uint32_t flags;      // Flags for the miscelaneous stuff
    };

    struct Resolve
    {
        int32_t numPointers;
        Ref*    table;
    };

    Resolve         resolve;
    InevFileHeader* header;
    uint8_t*        pStaticData;
    uint8_t*        pDynamicData;

    int cursor;

    // needed to understand alignment
    int platform;
};
