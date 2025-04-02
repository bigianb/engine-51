#include "ObjectManager.h"
#include <cassert>
#include <cstring>

guid ObjectManager::CreateObject(const char* objectTypeName)
{
    if (objectTypeName != nullptr) {
        for (object_desc* desc : objects) {
            if (strcasecmp(desc->GetTypeName(), objectTypeName) == 0) {
                return CreateObject(*desc);
            }
        }
    }
    assert(false);
    return guid(0);
}

guid ObjectManager::CreateObject(const object_desc& desc)
{
        return guid(0);
}