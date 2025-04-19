#include "BinLevel.h"
#include "DataReader.h"
#include "streamingOperators.h"
#include "dataUtil/Bitstream.h"
#include "objectManager/ObjectManager.h"
#include "objects/Object.h"

#include <cassert>
#include <cstring>
#include <inttypes.h>

BinLevel::~BinLevel()
{
    delete[] bitstreamData;
}

bool BinLevel::loadData(const uint8_t* fileData, int len, const uint8_t* dictData, int dictDataLen)
{
    DataReader dictReader = DataReader(dictData, dictDataLen);
    while (dictReader.hasData()) {
        dictionary.push_back(dictReader.consumeString());
    }

    DataReader reader = DataReader(fileData, len);
    version = reader.readUInt16();

    reader.skip(4); // dict size which we don't need
    int numObjects = reader.readInt32();
    int numProperties = reader.readInt32();
    bitstreamLen = reader.readInt32();

    for (int i = 0; i < numObjects; ++i) {
        ObjectEntry entry;
        entry.typeIndex = reader.readInt16();
        entry.nProperty = reader.readInt16();
        entry.iProperty = reader.readInt32();
        entry.guid = reader.readUInt64();
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

void BinLevel::loadLevel(ObjectManager* objectManager, const uint8_t* binLevelData, int binLevelDataLen, const uint8_t* levelDictData, int levelDictDataLen)
{
    loadData(binLevelData, binLevelDataLen, levelDictData, levelDictDataLen);
    Bitstream bs;
    bs.init(bitstreamData, bitstreamLen);
    for (auto& object : objects) {
        // The player object is exported in some levels - it shouldn't be!
        if (dictionary[object.typeIndex] == "Player") {
            if (playerGuid != 0) {
                Object* playerObject = objectManager->GetObjectByGuid(playerGuid);
                if (playerObject) {
                    playerObject->LoadStart();
                    //set all the objects properties
                    for (int k = object.iProperty; k < (object.iProperty + object.nProperty); k++) {
                        auto& property = properties[k];
                        AddPropertyToObject(bs, property, playerObject);
                    }
                    playerObject->LoadEnd();

                    // player has been successfully restored
                    continue;
                }
            }
            // store the guid of this player
            playerGuid = object.guid;
        }
        //now create the object
        guid ObjGuid = object.guid;
        {
            const std::string& pTypeName = dictionary[object.typeIndex];
            const object_desc* pDesc = objectManager->GetDescFromName(pTypeName.c_str());
            if (!pDesc) {
                std::cout << "Don't know type " << pTypeName << std::endl;
            } else {
                std::cout << "Creating object " << pTypeName << std::endl;
                objectManager->CreateObject(*pDesc, ObjGuid);
            }
        }
        Object* pObject = objectManager->GetObjectByGuid(ObjGuid);
        if (pObject) {
            pObject->LoadStart();

            //set all the objects properties
            for (int k = object.iProperty; k < (object.iProperty + object.nProperty); k++) {
                AddPropertyToObject(bs, properties[k], pObject);
            }

            pObject->LoadEnd();

            //
            // We don't really load the player, just allowing it so that the bitstream cursor
            // stays current. Now we'll drop the player.
            //
            if (pObject->GetType() == Object::TYPE_PLAYER) {
                objectManager->DestroyObject(ObjGuid);
                assert(false); // "We loaded a player, which should not have been exported"
            }

            // We don't really want the pickups on the client so nuke them now
            /*
            if ((pObject->GetType() == object::TYPE_PICKUP) && SuppressNetObjects) {
                objectManager->DestroyObject(ObjGuid);
            }*/

        } else {
            //assert(false); // Error loading couldn't create an object
        }
    }
    ClearData(true);
}

void BinLevel::ClearData(bool clearDictionary)
{
    objects.clear();
    properties.clear();

    if (clearDictionary) {
        dictionary.clear();
    }
    delete[] bitstreamData;
    bitstreamData = nullptr;
}

void BinLevel::AddPropertyToObject(Bitstream& bs, PropertyEntry& pe, Object* pObject)
{
    prop_query pq;
    char       cData[256];

    switch (pe.type) {
    case PROP_TYPE_EXTERNAL:
    {
        bs.readString(cData);
        pq.WQueryExternal(dictionary[pe.nameIndex].c_str(), cData);
        pObject->OnProperty(pq);
    } break;
    case PROP_TYPE_BUTTON:
    {
        bs.readString(cData);
        pq.WQueryButton(dictionary[pe.nameIndex].c_str(), cData);
        pObject->OnProperty(pq);
    } break;
    case PROP_TYPE_FILENAME:
    {
        bs.readString(cData);
        pq.WQueryFileName(dictionary[pe.nameIndex].c_str(), cData);
        pObject->OnProperty(pq);
    } break;
    case PROP_TYPE_ENUM:
    {
        bs.readString(cData);
        pq.WQueryEnum(dictionary[pe.nameIndex].c_str(), cData);
        pObject->OnProperty(pq);
    } break;
    case PROP_TYPE_STRING:
    {
        bs.readString(cData);
        pq.WQueryString(dictionary[pe.nameIndex].c_str(), cData);
        pObject->OnProperty(pq);
    } break;
    case PROP_TYPE_FLOAT:
    {
        float fData;
        bs.readF32(fData);
        pq.WQueryFloat(dictionary[pe.nameIndex].c_str(), fData);
        pObject->OnProperty(pq);
    } break;
    case PROP_TYPE_INT:
    {
        int32_t nData;
        bs.readS32(nData);
        pq.WQueryInt(dictionary[pe.nameIndex].c_str(), nData);
        pObject->OnProperty(pq);
    } break;
    case PROP_TYPE_BOOL:
    {
        bool bData;
        bData = bs.readFlag();
        pq.WQueryBool(dictionary[pe.nameIndex].c_str(), bData);
        pObject->OnProperty(pq);
    } break;
    case PROP_TYPE_VECTOR3:
    {
        Vector3 v3Data;
        bs.readVector(v3Data);
        pq.WQueryVector3(dictionary[pe.nameIndex].c_str(), v3Data);
        pObject->OnProperty(pq);
    } break;
    case PROP_TYPE_ANGLE:
    {
        Radian rData;
        bs.readF32(rData);
        pq.WQueryAngle(dictionary[pe.nameIndex].c_str(), rData);
        pObject->OnProperty(pq);
    } break;
    case PROP_TYPE_ROTATION:
    {
        Radian3 r3Data;
        bs.readRadian3(r3Data);
        pq.WQueryRotation(dictionary[pe.nameIndex].c_str(), r3Data);
        pObject->OnProperty(pq);
    } break;
    case PROP_TYPE_BBOX:
    {
        BBox bbData;
        bs.readVector(bbData.min);
        bs.readVector(bbData.max);
        pq.WQueryBBox(dictionary[pe.nameIndex].c_str(), bbData);
        pObject->OnProperty(pq);
    } break;
    case PROP_TYPE_COLOR:
    {
        Colour xcData;
        bs.readColor(xcData);
        pq.WQueryColor(dictionary[pe.nameIndex].c_str(), xcData);
        pObject->OnProperty(pq);
    } break;
    case PROP_TYPE_GUID:
    {
        uint64_t gData;
        bs.readU64(gData);
        pq.WQueryGUID(dictionary[pe.nameIndex].c_str(), gData);
        pObject->OnProperty(pq);
    } break;
    default:
        std::cerr << "Unknown property type: " << pe.type << std::endl;
        assert(false);
        break;
    }
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
    for (auto& object : objects) {
        ss << "    Type: " << dictionary[object.typeIndex] << std::endl;
        ss << "    Properties:" << std::endl;
        for (int ip = object.iProperty; ip < object.iProperty + object.nProperty; ++ip) {
            auto& property = properties[ip];
            ss << "          '" << dictionary[property.nameIndex] << "' = ";
            ss << getPropStringVal(property.type, bs) << std::endl;
        }
        ss << std::endl;
    }
}
