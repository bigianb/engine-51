#include "GlobalDictionary.h"

#include <assert.h>

dictionary g_StringMgr;
// std::vector<std::string> g_TemplateStringMgr;

int dictionary::Add(const char* s)
{
    if (dict.contains(s)){
        return dict.at(s);
    }
    int sz = dict.size();
    dict.emplace(s, sz);
    theVector.push_back(s);
    assert(sz == theVector.size() - 1);
    return sz;
}

std::string dictionary::GetString(int v)
{
    return theVector.at(v);
}
