#pragma once

#include <vector>
#include <set>
#include <string>
#include <map>

class dictionary
{
public:
    std::vector<std::string> theVector;
    std::map<std::string, int> dict;

    int Add(const char* s);

    std::string GetString(int);
};

extern dictionary g_StringMgr;
// extern dictionary g_TemplateStringMgr;
