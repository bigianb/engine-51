#pragma once

#include "Inventory.h"
#include "../Guid.h"


#define MAX_LEVELS (18)
#define MAX_CHECKPOINTS (6)

struct ammo_counts
{
    int Amount;
    int CurrentClip;
};

struct check_point
{
    bool        bIsValid;
    int         TableName;
    int         TitleName;
    guid        RespawnGUID;
    guid        AdvanceGUID;
    int         CurrWeapon;
    int         PrevWeapon;
    int         NextWeapon;
    bool        MutantMelee;
    bool        MutantPrimary;
    bool        MutantSecondary;
    float       Mutagen;
    float       Health;
    float       MaxHealth;
    float       Inventory[INVEN_COUNT];
    ammo_counts Ammo[INVEN_NUM_WEAPONS * 2];

    void Init(void)
    {
        bIsValid = false;
        TableName = -1;
        TitleName = -1;
        RespawnGUID = 0;
        AdvanceGUID = 0;
        CurrWeapon = 0;
        PrevWeapon = 0;
        NextWeapon = 0;
        MutantMelee = false;
        MutantPrimary = false;
        MutantSecondary = false;
        Mutagen = 0.0f;
        Health = 0.0f;
        MaxHealth = 0.0f;
        for (int i = 0; i < INVEN_COUNT; i++) {
            Inventory[i] = 0.0f;
        }
        for (int i = 0; i < INVEN_NUM_WEAPONS * 2; i++) {
            Ammo[i].Amount = 0;
            Ammo[i].CurrentClip = 0;
        }
    };
};

struct level_check_points
{
    int         MapID;              // Unique Map ID.
    int         nValidCheckPoints;  // Number of valid check points.
    int         iCurrentCheckPoint; // Current check point.
    check_point CheckPoints[MAX_CHECKPOINTS];

    void Init(int ID)
    {
        MapID = ID;
        nValidCheckPoints = 0;
        iCurrentCheckPoint = -1;
        for (int i = 0; i < MAX_CHECKPOINTS; i++) {
            CheckPoints[i].Init();
        }
    }
};

class check_point_mgr
{
public:
    check_point_mgr(void) {}
    ~check_point_mgr(void) {}

    void Init(int MapID) { m_Level.Init(MapID); }
    void Kill(void) {}
    int  GetCheckPointIndex(void);
    void SetCheckPointIndex(int CheckPoint);
    bool SetCheckPoint(guid RespawnwGUID,
                       guid DebugAdvanceGUID,
                       int  TableName,
                       int  TitleName);
    int  GetMapID(void);
    int  Read(Bitstream& in);
    int  Write(Bitstream& out);
    bool Restore(bool bIsDebugAdvance);
    void Reinit(int MapID);

    level_check_points m_Level;
};

#undef MAX_LEVELS