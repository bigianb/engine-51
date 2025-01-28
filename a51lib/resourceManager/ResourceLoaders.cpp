#include "ResourceLoaders.h"
#include "../Bitmap.h"
#include "../RigidGeom.h"

class XBMPResourceLoader : public ResourceLoader
{
public:
    XBMPResourceLoader()
        : ResourceLoader("TEXTURE", ".XBMP")
    {
    }

    void* resolve(uint8_t* data, int len)
    {
        Bitmap* bitmap = new Bitmap();
        bitmap->readFile(data, len, false);
        return bitmap;
    }

    void unload(void* data)
    {
        Bitmap* bitmap = (Bitmap*)data;
        delete bitmap;
    }
};

class RGEOMResourceLoader : public ResourceLoader
{
public:
    RGEOMResourceLoader()
        : ResourceLoader("RIGIDGEOM", ".RIGIDGEOM")
    {
    }

    void* resolve(uint8_t* data, int len)
    {
        RigidGeom* obj = new RigidGeom();
        obj->readFile(data, len);
        return obj;
    }

    void unload(void* data)
    {
        RigidGeom* obj = (RigidGeom*)data;
        delete obj;
    }
};

ResourceLoaders::~ResourceLoaders()
{
    for (ResourceLoader* loader : loaders) {
        delete loader;
    }
    loaders.clear();
}

void ResourceLoaders::registerLoaders(ResourceManager* rm)
{
    if (loaders.size() > 0) {
        // Already registered.
        return;
    }

    loaders.push_back(new XBMPResourceLoader());
    loaders.push_back(new RGEOMResourceLoader());

    for (ResourceLoader* loader : loaders) {
        rm->registerLoader(loader);
    }
};
