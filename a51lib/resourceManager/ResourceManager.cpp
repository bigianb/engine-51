#include "ResourceManager.h"
#include <string>
#include <iostream>

#include "../io/FileSystem.h"

void ResourceHandleBase::setName(std::string resourceName)
{
    resourceManager->addRHandle(*this, resourceName);
}

std::string ResourceHandleBase::getName() const
{
    return resourceManager->getRHandleName(*this);
}

ResourceManager::ResourceManager()
{
    onDemandLoading = false;
    fs = nullptr;
}

void ResourceManager::init()
{
}

void ResourceManager::setFilesystem(FileSystem* fsIn)
{
    fs = fsIn;
}

void ResourceManager::setRootDirectory(std::string)
{
}

void ResourceManager::setOnDemandLoading(bool)
{
}

void ResourceManager::addRHandle(ResourceHandleBase& rHandle, std::string resourceName)
{
    if (!resourceIdxByName.contains(resourceName)) {
        Resource newRes;
        newRes.state = State::NOT_LOADED;
        newRes.loader = nullptr;
        newRes.data = nullptr;
        newRes.name = resourceName;
        resources.push_back(newRes);
        resourceIdxByName[resourceName] = resources.size() - 1;

        if (!onDemandLoading)
        {
            load(resourceName);
        }
    }
    rHandle.setIndex(resourceIdxByName[resourceName]);
}

std::string ResourceManager::getRHandleName(const ResourceHandleBase& rHandle)
{
    // TODO: Implement
    return "";
}
    

int ResourceManager::findEntry(std::string resourceName)
{
    return -1;
}

void* ResourceManager::getPointer(const ResourceHandleBase& handle)
{
    int i = handle.getIndex();

    if (i == -1) {
        return nullptr;
    }

    if (resources[i].data) {
        return resources[i].data;
    }

    return getPointerSlow(handle);
}

void* ResourceManager::getPointerSlow(const ResourceHandleBase& handle)
{
    int i = handle.getIndex();

    if (i == -1) {
        return nullptr;
    }

    if (!resources[i].data) {
        if (onDemandLoading) {
            load(resources[i].name);
        }
    }

    return resources[i].data;
}

// includes the dot
static std::string getFileExtension(const std::string& s)
{
    size_t i = s.rfind('.', s.length());
    if (i != std::string::npos) {
        return s.substr(i, s.length() - i);
    }

    return "";
}

void ResourceManager::load(std::string resourceName)
{
    //std::cout << "Load " << resourceName << std::endl;
    if (!resourceIdxByName.contains(resourceName)) {
        Resource newRes;
        newRes.state = State::NOT_LOADED;
        newRes.loader = nullptr;
        newRes.data = nullptr;
        newRes.name = resourceName;
        resources.push_back(newRes);
        resourceIdxByName[resourceName] = resources.size() - 1;
    }
    Resource& res = resources.at(resourceIdxByName[resourceName]);
    if (res.state == State::LOADED || res.state == State::FAILED_LOAD) {
        // Already done or failed (don't try again)
        return;
    }

    auto extension = getFileExtension(resourceName);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return std::toupper(c); });

    if (loaders.contains(extension)) {
        ResourceLoader* loader = loaders[extension];
        res.loader = loader;

        int      len = 0;
        uint8_t* fileData = fs->readFile(resourceName, len);
        if (fileData != nullptr) {
            void* resolved = loader->resolve(fileData, len);
            if (resolved != nullptr) {
                std::cout << "Successfully loaded " << resourceName << std::endl;
                res.state = State::LOADED;
            }
        }
    } else {
        std::cout << "No Loader for " << extension << std::endl;
    }
    if (res.state != State::LOADED) {
        res.state = State::FAILED_LOAD;
    }
}

void ResourceManager::registerLoader(ResourceLoader* loader)
{
    std::cout << "Registering loader for " << loader->getExtension() << " files" << std::endl;
    loaders[loader->getExtension()] = loader;
}
