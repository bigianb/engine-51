#pragma once

#include <cstdint>
#include <string>

class Geom;
class prop_enum;
class prop_query;

class virtual_texture_mask
{
public:
    enum
    {
        MAX_VTEXTURES = 8
    };

    // init
    virtual_texture_mask();

    // useful helper functions
    operator uint32_t() const { return VTextureMask; }
    void OnEnumProp(prop_enum& List, Geom* pGeom);
    bool OnProperty(prop_query& I, Geom* pGeom);

    int         FindFirstMat(Geom* pGeom, int iVTexture);
    std::string BuildEnumList(Geom* pGeom, int iVTexture);
    void        SyncVTextures(Geom* pGeom);

    // run-time data
    uint32_t VTextureMask;
};
