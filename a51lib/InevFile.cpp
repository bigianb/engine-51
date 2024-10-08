#include "InevFile.h"
#include "Geom.h"

#include <iostream>

bool InevFile::init(uint8_t* fileData, int len)
{
    header = reinterpret_cast<InevFileHeader*>(fileData);
    if (header->inev != 0x56656e49) {
        return false;
    }
    pStaticData = fileData + sizeof(InevFileHeader);
    pDynamicData = pStaticData + sizeof(header->numStaticBytes);

    resolve.numPointers = header->numTables;
    static_assert(sizeof(Ref) == 16, "Ref must be 16 bytes long");
    resolve.table = (Ref* )(pStaticData + header->numStaticBytes - (header->numTables * sizeof(Ref)));
    return true;
}

int InevFile::readAndResolvePtr()
{
    for( int i=0; i<resolve.numPointers; i++ )
    {
        const Ref& ref = resolve.table[i];
        if (ref.offset == cursor){
            cursor += 4;
            if (ref.flags != 3){
                std::cerr << "ERROR: offset in dynamic data not yet supported. Flags = " << ref.flags << std::endl;
            }
            return ref.pointingAT;
        }
    }
    cursor += 4;
    std::cerr << "ERROR: No pointer resolution for offset " << cursor << std::endl;
    std::cerr << "known offsets are: " << std::endl;
    for( int i=0; i<resolve.numPointers; i++ )
    {
        const Ref& ref = resolve.table[i];
        std::cerr << ref.offset << " -> " << ref.pointingAT << ", flags = " << ref.flags << std::endl;
    }
    return 0;
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
    // aligned to 16 bytes on the PS2
    if (isPS2()){
        cursor = (cursor + 0x0F) & ~0x0F;
    }
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
