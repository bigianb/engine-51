#pragma once

class ResourceManager;

/**
 * A handle to a resource in the ResourceManager.
 * The type of the resource is unknown. Sub-classes are templetised to concrete types.
 * It needs to know the ResourceManager, so call setResourceManager before setName.
 */
class ResourceHandleBase
{
public:
    ResourceHandleBase() {data = -1; resourceManager = nullptr;}

    ~ResourceHandleBase() {}

    void setResourceManager(ResourceManager* mgr) {resourceManager = mgr;}

    void        setName(const char* resourceName);
    const char* getName() const;

    void* getPointer() const;
    bool isLoaded() const;

    int getIndex() const { return data; };

    void  destroy();
    bool isNull() const
    {
        return data == -1;
    }

protected:
    void setIndex(int idx) { data = idx; }

    int data;
    ResourceManager* resourceManager;
};

/**
 * A specialised ResourceHandleBase.
 */
template< class T >
struct ResourceHandle : public ResourceHandleBase
{
    T*  GetPointer  () const;
};

/**
 * The ResourceManager owns the resources. A resource is identified by a ResourceHandle.
 */
class ResourceManager
{
};
