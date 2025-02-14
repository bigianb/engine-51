#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>

class ResourceManager;
class FileSystem;
class StringTable;

// Abstract base class for a loader.
// Deserialises a types based on a file extension.
class ResourceLoader
{
public:
    virtual ~ResourceLoader() {}
    virtual void* resolve(uint8_t* data, int len, std::string resourceName) const = 0;
    virtual void  unload(void* data) const = 0;

    std::string getExtension() const { return extension; }

protected:
    ResourceLoader(std::string t, std::string e) : typeName(t), extension(e) {}

private:
    std::string typeName;
    std::string extension;
};

/**
 * A handle to a resource in the ResourceManager.
 * The type of the resource is unknown. Sub-classes are templetised to concrete types.
 */
class ResourceHandleBase
{
public:
    ResourceHandleBase(ResourceManager* mgr)
    {
        data = -1;
        resourceManager = mgr;
    }

    ~ResourceHandleBase()
    {
    }

    void        setName(std::string resourceName);
    std::string getName() const;

    void* getPointer() const;
    bool  isLoaded() const;

    int getIndex() const
    {
        return data;
    };

    void destroy();
    bool isNull() const
    {
        return data == -1;
    }

    // only called by the resource manager
    void setIndex(int idx)
    {
        data = idx;
    }

    int              data;
    ResourceManager* resourceManager;
};

/**
 * A specialised ResourceHandleBase.
 */
template <class T>
struct ResourceHandle : public ResourceHandleBase
{
    ResourceHandle(ResourceManager* mgr) : ResourceHandleBase(mgr) {}

    T* getPointer() const
    {
        return (T*)ResourceHandleBase::getPointer(); 
    }
};

/**
 * The ResourceManager owns the resources. A resource is identified by a ResourceHandle.
 */
class ResourceManager
{
public:
    ResourceManager();
    void init();
    void setRootDirectory(std::string rd);
    void setFilesystem(FileSystem* fs);
    void setOnDemandLoading(bool);

    void addRHandle(ResourceHandleBase& rHandle, std::string resourceName);
    std::string getRHandleName(const ResourceHandleBase& rHandle);
    
    int  findEntry(std::string resourceName);

    void* getPointer(const ResourceHandleBase& handle);

    // does not take ownership of the object.
    void registerLoader(ResourceLoader* loader);

    void load(std::string resourceName);

    void loadStringTable(std::string tableName, std::string stringbinName);

private:

    void* getPointerSlow(const ResourceHandleBase& handle);

    enum class State
    {
        NOT_USED,
        NOT_LOADED,
        LOADED,
        FAILED_LOAD,
    };

    struct Resource
    {
        std::string     name;
        State           state;
        void*           data;
        ResourceLoader* loader;
        bool            tagged;
    };

    FileSystem* fs;

    std::map<std::string, ResourceLoader*> loaders;
    std::vector<Resource> resources;
    std::map<std::string, int> resourceIdxByName;

    std::map<std::string, StringTable*> stringTables;

    bool        onDemandLoading;
    std::string rootDir;
};

inline void* ResourceHandleBase::getPointer() const
{
    return resourceManager->getPointer(*this);
}
