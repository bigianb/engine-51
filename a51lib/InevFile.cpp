#include "InevFile.h"
#include "Geom.h"

bool InevFile::init(uint8_t* fileData, int len)
{
    header = reinterpret_cast<InevFileHeader*>(fileData);
    if (header->inev != 0x56656e49){
        return false;
    }
    pStaticData = fileData + sizeof(InevFileHeader);
    pDynamicData = pStaticData + sizeof(header->numStaticBytes);

    return true;
}

void InevFile::read(Vector3& obj)
{
    read(obj.x);
    read(obj.y);
    read(obj.z);
    read(obj.w);
}

void InevFile::read(BBox& obj)
{
    read(obj.min);
    read(obj.max);
}

void InevFile::read(int16_t& obj)
{
    const uint8_t* p = pStaticData + cursor;
    obj = *(int16_t *)p;
    cursor += 2;
} 

void InevFile::read(uint16_t& obj)
{
    const uint8_t* p = pStaticData + cursor;
    obj = *(uint16_t *)p;
    cursor += 2;
} 

void InevFile::read(float& obj)
{
    const uint8_t* p = pStaticData + cursor;
    obj = *(float *)p;
    cursor += 4;
} 
