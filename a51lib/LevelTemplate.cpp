#include "LevelTemplate.h"
#include "DataReader.h"
#include "streamingOperators.h"
#include "dataUtil/Bitstream.h"

#include <cstring>
#include <inttypes.h>

LevelTemplate::~LevelTemplate()
{
    delete[] bitstreamData;
}

bool LevelTemplate::readFile(uint8_t* fileData, int len, uint8_t* dictData, int dictDataLen)
{
    DataReader dictReader = DataReader(dictData, dictDataLen);
    while(dictReader.hasData()){
        dictionary.push_back(dictReader.consumeString());
    }

    DataReader reader = DataReader(fileData, len);
    version = reader.readUInt16();

    reader.skip(4);     // dict size which we don't need
    int numTemplates = reader.readInt32();
    int numObjects = reader.readInt32();
    int numProperties = reader.readInt32();
    bitstreamLen = reader.readInt32();

    for (int i = 0; i < numTemplates; ++i) {
        TemplateEntry entry;
        reader.read(entry.anchorPos);
        entry.iStartBitStream = reader.readInt32();
        entry.nameIndex = reader.readInt16();
        entry.iObject = reader.readInt16();
        entry.nObjects = reader.readInt16();
        reader.skip(6);
        templates.push_back(entry);
    }

    for (int i = 0; i < numObjects; ++i) {
        ObjectEntry entry;
        entry.typeIndex = reader.readInt16();
        reader.skip(2);
        entry.iProperty = reader.readInt32();
        entry.nProperty = reader.readInt16();
        reader.skip(2);
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

std::string getPropStringVal(PropertyType type, Bitstream& bs)
{
    char data[256];
    switch(type){
    case PROP_TYPE_EXTERNAL:
    case PROP_TYPE_BUTTON:
    case PROP_TYPE_FILENAME:
    case PROP_TYPE_ENUM:
    case PROP_TYPE_STRING:
        {
            bs.readString(data, 255);
        }
	    break;
    
    case PROP_TYPE_FLOAT:
    case PROP_TYPE_ANGLE:
        {
            float val;
            bs.readF32(val);
            snprintf(data, 256, "%f", val);
        }
	    break;

    case PROP_TYPE_INT:       
        {
            int32_t val;
            bs.readS32(val);
            snprintf(data, 256, "%d", val);
        }
	    break;

    case PROP_TYPE_BOOL:
        {
            bool flag = bs.readFlag();
            snprintf(data, 256, "%s", flag ? "true" : "false");
        }
	    break;
    case PROP_TYPE_VECTOR3:
        {
            Vector3 vec;
            bs.readVector(vec);
            snprintf(data, 256, "[%f, %f, %f]", vec.x, vec.y, vec.z);
        }
	    break;

    case PROP_TYPE_ROTATION:  			
        {
            Radian3 radian;
            bs.readRadian3(radian);
            snprintf(data, 256, "pitch %f, roll %f, yaw %f", radian.pitch, radian.roll, radian.yaw);
        }
	    break;
    case PROP_TYPE_BBOX:      
        {
            BBox bbox;
            bs.readVector(bbox.min);
            bs.readVector(bbox.max);
            snprintf(data, 256, "[%f, %f, %f -> %f, %f, %f]", bbox.min.x, bbox.min.y, bbox.min.z, bbox.max.x, bbox.max.y, bbox.max.z);
        }
	    break;
    case PROP_TYPE_COLOR:
        {
            Colour colour;
            bs.readColor(colour);
            snprintf(data, 256, "[%d, %d, %d, %d]", colour.r, colour.g, colour.b, colour.a);
        }
	    break;
    case PROP_TYPE_GUID:     
        {
            uint64_t i;
            bs.readU64(i);
            snprintf(data, 256, "%" PRIu64, i);
        }
	    break;
    default:
        snprintf(data, 256, "unknown type %d", type);
    }
    return data;
}

void LevelTemplate::describe(std::ostringstream& ss)
{
    Bitstream bs;
    bs.init(bitstreamData, bitstreamLen);

    ss << "Version:" << version << std::endl;
    ss << "Dict Size:" << dictionary.size() << std::endl;
    ss << "Bitstream Length:" << bitstreamLen << std::endl;

    ss << "Num Templates: " << templates.size() << std::endl;
    ss << "Num Objects: " << objects.size() << std::endl;
    ss << "Num Properties: " << properties.size() << std::endl;

    ss << "Templates:" << std::endl;
    for (auto& entry : templates)
    {
        bs.setCursor(entry.iStartBitStream);

        ss << "    Anchor Pos: " << entry.anchorPos << std::endl;
        ss << "    Name: " << dictionary[entry.nameIndex] << std::endl;
        ss << "    Objects: " << std::endl;
        for (int io = entry.iObject; io < entry.iObject + entry.nObjects; ++io){
            auto& object = objects[io];
            ss << "        Type: " << dictionary[object.typeIndex] << std::endl;
            ss << "        Properties:" << std::endl;
            for (int ip = object.iProperty; ip < object.iProperty + object.nProperty; ++ip){
                auto& property = properties[ip];
                ss << "          '" << dictionary[property.nameIndex] << "' = ";
                ss << getPropStringVal(property.type, bs) << std::endl;
            }
            ss << std::endl;
        }
        ss << std::endl;
    }
}
