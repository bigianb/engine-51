#pragma once

//=========================================================================
//  Defines
//=========================================================================

#define LORE_VERSION 1000
#define NUM_VAULTS 19
#define NUM_PER_VAULT 5
#define LORE_TABLE_SIZE NUM_VAULTS* NUM_PER_VAULT

enum lore_type
{
    LORE_TYPE_VIDEO,
    LORE_TYPE_AUDIO,
    LORE_TYPE_STILL,
    LORE_TYPE_TEXT,
    LORE_TYPE_UNKNOWN,
};

//=========================================================================
//  Structs
//=========================================================================

struct lore_entry
{
    int       LoreID;        // unique ID
    int       MapID;         // the map ID that the lore is from
    int       NumItems;      // number of items in set
    lore_type LoreType;      // lore type ID
    char      FileName[32];  // physical filename
    char      Clue[32];      // string ID of lore location clue
    char      ShortDesc[32]; // string ID of lore short description
    char      FullDesc[32];  // string ID of lore long description
    char      FullName[32];  // string ID of lore name for scanner
};

struct lore_vault
{
    int MapID;
    int LoreID[NUM_PER_VAULT];
};

class lore_list
{
public:
    void              Init(void);
    void              Kill(void);
    void              Clear(void);
    void              Append(const char* pLoreFile);
    const lore_entry* Find(int LoreID);
    const lore_type   GetType(int LoreID);
    const char*       GetFileName(int LoreID);
    lore_entry*       GetByIndex(int Index);
    const wchar_t*    GetLoreName(int Index);
    lore_vault*       GetVaultByMapID(int MapID, int& Index);
    lore_vault*       GetVaultByLoreID(int LoreID, int& Index, int& LoreIndex);
    int               GetLoreIDByVault(lore_vault* pVault, int Index);

private:
    bool Append(const lore_entry& Entry);
    bool AddToVault(const lore_entry& Entry);

    lore_entry m_LoreList[LORE_TABLE_SIZE];
    lore_vault m_LoreVault[NUM_VAULTS];
};
