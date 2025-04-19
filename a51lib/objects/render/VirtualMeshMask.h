#pragma once

#include <cstdint>

class Geom;
class prop_enum;
class prop_query;

struct virtual_mesh_mask
{
    enum
    {
        MAX_VMESHES = 32
    };

    // init
    virtual_mesh_mask();

    // useful helper functions
    operator uint32_t() const { return VMeshMask; }
    void OnEnumProp(prop_enum& List, Geom* pGeom);
    bool OnProperty(prop_query& I, Geom* pGeom);

    void SyncVMeshes(Geom* pGeom);

    // run-time data
    uint32_t VMeshMask;
};
