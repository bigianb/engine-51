#pragma once
#include <string>

enum inven_item
{
    INVEN_NULL = 0,

    // Weaponss
    INVEN_WEAPON_SCANNER,

    INVEN_WEAPON_DESERT_EAGLE,
    INVEN_WEAPON_DUAL_EAGLE,

    INVEN_WEAPON_SMP,
    INVEN_WEAPON_DUAL_SMP,

    INVEN_WEAPON_SHOTGUN,
    INVEN_WEAPON_DUAL_SHT,

    INVEN_WEAPON_SNIPER_RIFLE,
    INVEN_WEAPON_BBG,
    INVEN_WEAPON_MESON_CANNON,
    INVEN_WEAPON_TRA,
    INVEN_WEAPON_MUTATION,
    INVEN_WEAPON_FIRST = INVEN_WEAPON_SCANNER, // Used for range checks.
    INVEN_WEAPON_LAST = INVEN_WEAPON_MUTATION,

    // Weapon Ammo
    INVEN_AMMO_SMP,
    INVEN_AMMO_SHOTGUN,
    INVEN_AMMO_SNIPER_RIFLE,
    INVEN_AMMO_MESON,
    INVEN_AMMO_BBG,
    INVEN_AMMO_DESERT_EAGLE,
    INVEN_AMMO_FIRST = INVEN_AMMO_SMP, // Used for range checks.
    INVEN_AMMO_LAST = INVEN_AMMO_DESERT_EAGLE,

    // Grenades
    INVEN_GRENADE_FRAG,
    INVEN_GRENADE_GRAV,
    INVEN_GRENADE_JBEAN,
    INVEN_GRENADE_JBEAN_ENHANCE,
    INVEN_GRENADE_FIRST = INVEN_GRENADE_FRAG, // Used for range checks.
    INVEN_GRENADE_LAST = INVEN_GRENADE_JBEAN_ENHANCE,

    // General Pickups
    INVEN_HEALTH,
    INVEN_MUTAGEN,
    INVEN_MUTAGEN_CORPSE,

    // Gloves (not a pickup)
    INVEN_GLOVES,

    // Keycards
    INVEN_KEYCARD_RED,
    INVEN_KEYCARD_GREEN,
    INVEN_KEYCARD_BLUE,
    INVEN_KEYCARD_YELLOW,
    INVEN_KEYCARD_ORANGE,

    // User items for use as quest items in levels
    INVEN_ITEM_1,
    INVEN_ITEM_2,
    INVEN_ITEM_3,
    INVEN_ITEM_4,
    INVEN_ITEM_5,
    INVEN_ITEM_6,
    INVEN_ITEM_7,
    INVEN_ITEM_8,
    INVEN_ITEM_9,
    INVEN_ITEM_10,
    INVEN_ITEM_FIRST = INVEN_HEALTH, // Used for range checks.
    INVEN_ITEM_LAST = INVEN_ITEM_10,

    // Count of enumeration entries
    INVEN_COUNT
};

#define INVEN_NUM_WEAPONS (INVEN_WEAPON_LAST + 1) // Must include "NULL weapon".

class inventory2
{
public:
    static const char*    ItemToName(inven_item Item);
    static const wchar_t* ItemToDisplayName(inven_item Item);
    static inven_item     NameToItem(const char* pName);
    static int            GetNumItems();

    static inven_item AmmoToWeapon(inven_item AmmoItem);
    static inven_item WeaponToAmmo(inven_item WeaponItem);
    static int        ItemToWeaponIndex(inven_item Item);
    static inven_item WeaponIndexToItem(int iWeapon);
    static bool       IsAWeapon(inven_item Item);
    static int        GetNumWeapons();

    static std::string GetEnumString();
    static std::string GetEnumStringWeapons();
    static std::string GetEnumStringGrenades();

    static const char* ItemToBlueprintName(inven_item Item);
    static const char* ItemToPickupGeomName(inven_item Item, float Amount = -1.0f);

    inventory2();
    ~inventory2();

    void Init();

    void Clear();

    inventory2& operator=(const inventory2& Source);

    float GetAmount(inven_item Item);
    void  SetAmount(inven_item Item, float Amount);
    void  AddAmount(inven_item Item, float Amount);
    void  RemoveAmount(inven_item Item, float Amount);
    float GetMinAmount(inven_item Item);
    float GetMaxAmount(inven_item Item);

    bool CanHoldMore(inven_item Item);
    bool HasItem(inven_item Item);

    //void                Save            ( bitstream& Bitstream );
    //void                Load            ( bitstream& Bitstream );

protected:
    float m_Data[INVEN_COUNT];
};
