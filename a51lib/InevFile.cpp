#include "InevFile.h"
#include "Geom.h"

bool InevFile::init(uint8_t* fileData, int len)
{
    header = reinterpret_cast<InevFileHeader*>(fileData);
    if (header->inev != 0x56656e49) {
        return false;
    }
    pStaticData = fileData + sizeof(InevFileHeader);
    pDynamicData = pStaticData + sizeof(header->numStaticBytes);

    return true;
}

void InevFile::describe(std::ostringstream& ss)
{
    ss << "Inev file" << std::endl;
    ss << "Version: " << header->version << std::endl;
    ss << "Num static bytes: " << header->numStaticBytes << std::endl;
    ss << "Num tables: " << header->numTables << std::endl;
    ss << "Num dynamic bytes: " << header->numDynamicBytes << std::endl;
}

void InevFile::read(Quaternion& obj)
{
    read(obj.x);
    read(obj.y);
    read(obj.z);
    read(obj.w);
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

void InevFile::read(int8_t& obj)
{
    const uint8_t* p = pStaticData + cursor;
    obj = *(int8_t*)p;
    cursor += 1;
}

void InevFile::read(uint8_t& obj)
{
    obj = pStaticData[cursor++];
}

void InevFile::read(int16_t& obj)
{
    const uint8_t* p = pStaticData + cursor;
    obj = *(int16_t*)p;
    cursor += 2;
}

void InevFile::read(uint16_t& obj)
{
    const uint8_t* p = pStaticData + cursor;
    obj = *(uint16_t*)p;
    cursor += 2;
}

void InevFile::read(float& obj)
{
    const uint8_t* p = pStaticData + cursor;
    obj = *(float*)p;
    cursor += 4;
}

void InevFile::read(int& obj)
{
    const uint8_t* p = pStaticData + cursor;
    obj = *(int*)p;
    cursor += 4;
}

void InevFile::read(unsigned int& obj)
{
    const uint8_t* p = pStaticData + cursor;
    obj = *(unsigned int*)p;
    cursor += 4;
}

void InevFile::read(uint64_t& obj)
{
    const uint8_t* p = pStaticData + cursor;
    obj = *(uint64_t*)p;
    cursor += 8;
}
