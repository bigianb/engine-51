#pragma once

#include <cstdint>
#include <sstream>
#include <vector>

#include "PropertyDefs.h"
#include "VectorMath.h"

class ObjectManager;
class Bitstream;
class Object;

class BinLevel
{
public:
    BinLevel()
    {
        bitstreamData = nullptr;
        playerGuid = 0;
    };
    ~BinLevel();

    bool loadData(const uint8_t* binLevelData, int binLevelDataLen, const uint8_t* levelDictData, int levelDictDataLen);
    void loadLevel(ObjectManager* objectManager, const uint8_t* binLevelData, int binLevelDataLen, const uint8_t* levelDictData, int levelDictDataLen);

    void describe(std::ostringstream& ss);

    int version;

    struct PropertyEntry
    {
        PropertyType type;
        int          nameIndex;
    };

    struct ObjectEntry
    {
        int      typeIndex; // Dictionary Index
        int      iProperty; // Index to the first property
        int      nProperty; // Number of properties
        uint64_t guid;
    };

    void AddPropertyToObject(Bitstream& bs, PropertyEntry& pe, Object* pObject);
    void ClearData(bool clearDictionary);

    std::vector<ObjectEntry>   objects;
    std::vector<PropertyEntry> properties;
    std::vector<std::string>   dictionary;

    int      bitstreamLen;
    uint8_t* bitstreamData;

    // Required to detect and skip player objects in the level data.
    uint64_t playerGuid;
};
