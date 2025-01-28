#pragma once

#include <vector>

#include "ResourceManager.h"

class ResourceLoaders
{
public:
    ~ResourceLoaders();
    void registerLoaders(ResourceManager*);

private:
    std::vector<ResourceLoader*> loaders;
};
