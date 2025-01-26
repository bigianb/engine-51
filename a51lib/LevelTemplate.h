#pragma once

#include <cstdint>
#include <sstream>
#include <vector>

#include "PropertyDefs.h"
#include "VectorMath.h"

class LevelTemplate
{
public:
    LevelTemplate()
    {
        bitstreamData = nullptr;
    };
    ~LevelTemplate();

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
    };

    // structure to represent a blueprint template containg multiple objects
    struct TemplateEntry
    {
        Vector3 anchorPos; // Position for the blueprint anchor

        int iStartBitStream; // Byte/bit offset to the first property
        int nameIndex;       // Dictionary Index
        int iObject;         // Index to the first object
        int nObjects;        // Number of object for the template
    };

    std::vector<TemplateEntry> templates;
    std::vector<ObjectEntry>   objects;
    std::vector<PropertyEntry> properties;
    std::vector<std::string>   dictionary;

    int      bitstreamLen;
    uint8_t* bitstreamData;
};
