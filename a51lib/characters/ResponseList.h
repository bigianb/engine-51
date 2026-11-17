#pragma once

#include "../Property.h"

#undef BIT
#define BIT(x) ( 1<<(x) )

struct response_list
{
    enum response_flags
    {
        RF_NULL = 0,
        RF_IGNORE_ATTACKS = BIT(1),
        RF_IGNORE_SIGHT = BIT(2),
        RF_IGNORE_SOUND = BIT(3),
        RF_IGNORE_ALERTS = BIT(4),
        RF_INVINCIBLE = BIT(5)
    };

    response_list() { m_ResponseFlags = RF_IGNORE_ATTACKS | RF_IGNORE_SIGHT | RF_IGNORE_SOUND | RF_IGNORE_ALERTS; }
    virtual void OnEnumProp(prop_enum& List);
    virtual bool OnProperty(prop_query& I);
    void         AddFlags(uint32_t newFlags);
    void         RemoveFlags(uint32_t newFlags);
    bool         HasFlags(uint32_t newFlags);
    void         SetFlags(uint32_t newFlags);
    uint32_t     GetFlags();

protected:
    uint32_t m_ResponseFlags;
};
