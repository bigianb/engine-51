#pragma once

#include <cstdint>
#include <string>
#include <sstream>
#include <vector>

class StringTable
{
public:
    void read(uint8_t* data, int len, std::string name);

    void describe(std::wostringstream& ss);

    std::string tableName;

    struct Entry
    {
        std::wstring id;
        std::wstring val;
        std::wstring subtitle;
        std::wstring soundDesc;
    };
    std::vector<Entry> entries;
};
