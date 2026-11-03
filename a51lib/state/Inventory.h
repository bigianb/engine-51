#pragma once

#include <string>
#include "../dataUtil/Bitstream.h"
#include "../inventory/Inventory.h"

//=========================================================================
// INVENTORY CLASS
//=========================================================================

class Inventory
{
public:
    static const char*   ItemToName(inven_item Item);
    static const wchar_t* ItemToDisplayName(inven_item Item);
    static inven_item    NameToItem(const char* pName);
    static int           GetNumItems(void);

    static inven_item AmmoToWeapon(inven_item AmmoItem);
    static inven_item WeaponToAmmo(inven_item WeaponItem);
    static int        ItemToWeaponIndex(inven_item Item);
    static inven_item WeaponIndexToItem(int iWeapon);
    static bool      IsAWeapon(inven_item Item);
    static int        GetNumWeapons(void);

    static std::string GetEnumString(void);
    static std::string GetEnumStringWeapons(void);
    static std::string GetEnumStringGrenades(void);

    static const char* ItemToBlueprintName(inven_item Item);
    static const char* ItemToPickupGeomName(inven_item Item, float Amount = -1.0f);

    Inventory();
    ~Inventory();

    void Init(void);

    void Clear(void);

    Inventory& operator=(const Inventory& Source);

    float  GetAmount(inven_item Item);
    void SetAmount(inven_item Item, float Amount);
    void AddAmount(inven_item Item, float Amount);
    void RemoveAmount(inven_item Item, float Amount);
    float  GetMinAmount(inven_item Item);
    float  GetMaxAmount(inven_item Item);

    bool CanHoldMore(inven_item Item);
    bool HasItem(inven_item Item);

    void Save(Bitstream& Bitstream);
    void Load(Bitstream& Bitstream);

protected:
    float m_Data[INVEN_COUNT];
};
