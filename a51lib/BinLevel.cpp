#include "BinLevel.h"
#include "DataReader.h"
#include "streamingOperators.h"
#include "dataUtil/Bitstream.h"

#include <cstring>
#include <inttypes.h>

BinLevel::~BinLevel()
{
    delete[] bitstreamData;
}

bool BinLevel::readFile(uint8_t* fileData, int len, uint8_t* dictData, int dictDataLen)
{
    DataReader dictReader = DataReader(dictData, dictDataLen);
    while(dictReader.hasData()){
        dictionary.push_back(dictReader.consumeString());
    }

    DataReader reader = DataReader(fileData, len);
    version = reader.readUInt16();

    reader.skip(4);     // dict size which we don't need
    int numObjects = reader.readInt32();
    int numProperties = reader.readInt32();
    bitstreamLen = reader.readInt32();

    for (int i = 0; i < numObjects; ++i) {
        ObjectEntry entry;
        entry.typeIndex = reader.readInt16();
        entry.nProperty = reader.readInt16();
        entry.iProperty = reader.readInt32();
        reader.skip(8);             // guid
        objects.push_back(entry);
    }

    for (int i = 0; i < numProperties; ++i) {
        PropertyEntry entry;
        entry.type = (PropertyType)reader.readUInt16();
        entry.nameIndex = reader.readInt16();
        properties.push_back(entry);
    }

    // bitstream data follows
    delete[] bitstreamData;
    bitstreamData = nullptr;

    bitstreamData = new uint8_t[bitstreamLen];
    std::memcpy(bitstreamData, fileData + reader.cursor, bitstreamLen);
    return true;
}

std::string getPropStringVal(PropertyType type, Bitstream& bs);


void BinLevel::describe(std::ostringstream& ss)
{
    Bitstream bs;
    bs.init(bitstreamData, bitstreamLen);

    ss << "Version:" << version << std::endl;
    ss << "Dict Size:" << dictionary.size() << std::endl;
    ss << "Bitstream Length:" << bitstreamLen << std::endl;

    ss << "Num Objects: " << objects.size() << std::endl;
    ss << "Num Properties: " << properties.size() << std::endl;

    ss << "Objects: " << std::endl;
    for (auto& object : objects){
        ss << "    Type: " << dictionary[object.typeIndex] << std::endl;
        ss << "    Properties:" << std::endl;
        for (int ip = object.iProperty; ip < object.iProperty + object.nProperty; ++ip){
            auto& property = properties[ip];
            ss << "          '" << dictionary[property.nameIndex] << "' = ";
            ss << getPropStringVal(property.type, bs) << std::endl;
        }
        ss << std::endl;
    }
    
}
