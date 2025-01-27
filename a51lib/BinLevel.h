#pragma once

#include <cstdint>
#include <sstream>
#include <vector>

#include "PropertyDefs.h"
#include "VectorMath.h"

class BinLevel
{
public:
    BinLevel()
    {
        bitstreamData = nullptr;
    };
    ~BinLevel();

    bool readFile(uint8_t* fileData, int len, uint8_t* dictData, int dictDataLen);
    void describe(std::ostringstream& ss);

    int version;

    struct PropertyEntry
    {
        PropertyType type;
        int          nameIndex;
    };

    struct ObjectEntry
    {
        int typeIndex; // Dictionary Index
        int iProperty; // Index to the first property
        int nProperty; // Number of properties
        uint64_t guid;
    };

    std::vector<ObjectEntry>   objects;
    std::vector<PropertyEntry> properties;
    std::vector<std::string>   dictionary;

    int      bitstreamLen;
    uint8_t* bitstreamData;
};
