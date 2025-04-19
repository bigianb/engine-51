#include "StringTable.h"
#include "../DataReader.h"

#include <cstring>
#include <iostream>

void StringTable::read(uint8_t* data, int len, std::string name)
{
    // TODO: trim extension
    tableName = name;
    DataReader reader(data, len);

    int numStrings = reader.readInt32();

    int offset0 = 4 + numStrings * 4;

    for (int i = 0; i < numStrings; ++i) {
        int        offset = offset0 + reader.readInt32();
        DataReader stringReader(data + offset, len - offset);
        Entry      entry;
        entry.id = stringReader.consumeWString();
        entry.val = stringReader.consumeWString();
        entry.subtitle = stringReader.consumeWString();
        entry.soundDesc = stringReader.consumeWString();
        entries.push_back(entry);
    }
}

void StringTable::describe(std::wostringstream& ss)
{
    int i = 0;
    for (const auto& entry : entries) {
        ss << "Entry " << i++ << std::endl;
        ss << "     Id " << entry.id << std::endl;
        ss << "     Val " << entry.val << std::endl;
        ss << "     Subtitle " << entry.subtitle << std::endl;
        ss << "     Sound " << entry.soundDesc << std::endl
           << std::endl;
    }
}

std::wstring StringTable::lookupStringVal(int id)
{
    return entries[id].val;
}

std::wstring StringTable::lookupStringVal(const char* id)
{
    std::wstring sid(id, id + strlen(id));
    for (const auto& entry : entries){
        if (entry.id == sid){
            return entry.val;
        }
    }
    return L"unknown";
}
