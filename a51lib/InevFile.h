#pragma once

#include <cstdint>
#include <sstream>
#include <type_traits>

struct InevFileHeader
{
    uint32_t inev;
    int32_t version;
    int32_t numStaticBytes;
    int32_t numTables;
    int32_t numDynamicBytes;
};

struct BBox;
struct Quaternion;
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

    void describe(std::ostringstream& ss);

    bool init(uint8_t* fileData, int len);

    void read(Quaternion&);
    void read(Vector3&);
    void read(BBox&);
    void read(int16_t&);
    void read(uint16_t&);
    void read(float&);

    template< class T >
    void read( T& obj ) {
        obj.read(*this);
    }

    template< typename T >
    void readArray( T& ptr, int size ) {
        ptr = new std::remove_pointer_t<T>[size];
        for (int i=0; i<size; ++i){
            ptr[i].read(*this);
        }
    }

private:
    InevFileHeader* header;
    uint8_t* pStaticData;
    uint8_t* pDynamicData;

    int cursor;
};