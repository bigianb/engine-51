#pragma once

#include "../Guid.h"
#include "../Property.h"

#include <vector>

class object_desc : public prop_interface
{
public:
    const char* GetTypeName() const { return m_pTypeName; }

protected:
    const char* m_pTypeName;
};

class ObjectManager
{
public:
    guid CreateObject(const char* objectTypeName);
    guid CreateObject(const object_desc& Desc);

private:
    std::vector<object_desc*> objects;
};
