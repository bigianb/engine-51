#include "ResourceLoaders.h"
#include "../Bitmap.h"
#include "../RigidGeom.h"
#include "../strings/StringTable.h"

class XBMPResourceLoader : public ResourceLoader
{
public:
    XBMPResourceLoader()
        : ResourceLoader("TEXTURE", ".XBMP")
    {
    }

    void* resolve(uint8_t* data, int len, std::string name) const
    {
        Bitmap* bitmap = new Bitmap();
        bitmap->readFile(data, len, false);
        return bitmap;
    }

    void unload(void* data) const
    {
        Bitmap* bitmap = (Bitmap*)data;
        delete bitmap;
    }
};

class StringResourceLoader : public ResourceLoader
{
public:
    StringResourceLoader()
        : ResourceLoader("Binary String", ".STRINGBIN")
    {
    }

    void* resolve(uint8_t* data, int len, std::string name) const
    {
        StringTable* obj = new StringTable();
        obj->read(data, len, name);
        return obj;
    }

    void unload(void* data) const
    {
        StringTable* obj = (StringTable*)data;
        delete obj;
    }
};

class RGEOMResourceLoader : public ResourceLoader
{
public:
    RGEOMResourceLoader()
        : ResourceLoader("RIGIDGEOM", ".RIGIDGEOM")
    {
    }

    void* resolve(uint8_t* data, int len, std::string name) const
    {
        RigidGeom* obj = new RigidGeom();
        obj->readFile(data, len);
        return obj;
    }

    void unload(void* data) const
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
    loaders.push_back(new StringResourceLoader());

    for (ResourceLoader* loader : loaders) {
        rm->registerLoader(loader);
    }
};
