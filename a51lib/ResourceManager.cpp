#include "ResourceManager.h"

class ResourceManager;

/**
 * A handle to a resource in the ResourceManager.
 * The type of the resource is unknown. Sub-classes are templetised to concrete types.
 */
class ResourceHandleBase
{
public:
    ResourceHandleBase(ResourceManager& resourceManager, const char* resourceName = nullptr);

    ~ResourceHandleBase();

    void        SetName(const char* resourceName);
    const char* GetName() const;

    void* GetPointer() const;
    bool IsLoaded() const;

    int GetIndex() const;

    void  Destroy();
    bool IsNull() const
    {
        return data == -1;
    }

protected:
    void SetIndex(int I);

    int data;
    ResourceManager& resourceManager;
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
