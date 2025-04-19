#pragma once

#include "../objectManager/ObjectManager.h"

class ObjectRegistrar : public ObjectRegistrarInterface
{
public:
    ObjectRegistrar() {}
    ~ObjectRegistrar() {}

    void RegisterObjects(std::vector<const object_desc*>& objectDescriptors) override;
};