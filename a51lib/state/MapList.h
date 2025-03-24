
// The file format takes the following form:
/*
    [Version]
    1149

    [GameType]
    0, "TDM", "Team Deathmatch",  "Team based deathmatch game"
                                    "Kill the guys on the other team";
    [Map]
    4010, "tinymp.gz", "Tiny MP downloaded", "This is the Tiny MP Description"
                                            "downloaded from the internet";

    [Play]
    4010, 0, 1, 16;
*/

//
// [Version] -  Specifies the version of this file. This is used during a content
//              check to see if the user needs to be informed of new additional maps.
// [GameType] - This defines what a specific game type index means. We have, by default
//              used -2 to 5 for built in gametypes but we can define new ones as
//              necessary.
// [Map] -      Describes a specific map. Can have comments as to it's special features.
// [Play] -     Binds a game type, map description and minimum, maximum players for this
//              particular play mode.
//==============================================================================

#pragma once

#include <string>
#include <vector>

#define MAP_TABLE_SIZE 256
#define MAX_GAME_TYPES 16
#define MAX_MAPS 80

enum game_type
{
    GAME_CAMPAIGN = -2,
    GAME_MP = -1, // Only used on client.  Represents ALL online types.

    GAME_DM,  //   0
    GAME_TDM, //   1
    GAME_CTF, //   2
    GAME_TAG, //   3
    GAME_INF, //   4
    GAME_CNH, //   5

    GAME_RESERVED_06,
    GAME_RESERVED_07,
    GAME_RESERVED_08,
    GAME_RESERVED_09,
    GAME_RESERVED_10,
    GAME_RESERVED_11,
    GAME_RESERVED_12,
    GAME_RESERVED_13,
    GAME_RESERVED_14,
    GAME_RESERVED_15,

    GAME_LAST,
    GAME_FIRST = 0,
};

enum map_flags
{
    MF_DVD_MAP = 0x00000001,      // this map is on the DVD
    MF_DOWNLOAD_MAP = 0x00000002, // this map was downloaded
    MF_WIRE_MAP = 0x00000004,     // this is a wire map
    MF_NOT_PRESENT = 0,
};

enum level_ids
{
    LEVELID_WELCOME_TO_DREAMLAND = 1000,
    LEVELID_DEEP_UNDERGROUND = 1010,
    LEVELID_THE_HOT_ZONE = 1020,
    LEVELID_THE_SEARCH = 1030,
    LEVELID_THEY_GET_BIGGER = 1040,
    LEVELID_THE_LAST_STAND = 1050,
    LEVELID_ONE_OF_THEM = 1060,
    LEVELID_INTERNAL_CHANGES = 1070,
    LEVELID_LIFE_OR_DEATH = 1075,
    LEVELID_DR_CRAY = 1080,
    LEVELID_HATCHING_PARASITES = 1090,
    LEVELID_PROJECT_BLUE_BOOK = 1095,
    LEVELID_LIES_OF_THE_PAST = 1100,
    LEVELID_BURIED_SECRETS = 1105,
    LEVELID_NOW_BOARDING = 1110,
    LEVELID_THE_GRAYS = 1115,
    LEVELID_DESCENT = 1120,
    LEVELID_THE_ASCENSION = 1125,
    LEVELID_THE_LAST_EXIT = 1130,
};

class game_type_info
{
public:
    game_type_info();
    ~game_type_info();
    game_type   Type;          // GameType
    std::string ShortTypeName; // Pointer to short game type for display
    std::string TypeName;      // Pointer to long game type for display
    std::string Rules;         // Pointer to rules for that game type.
    bool        operator==(const game_type_info& Right) const;
};

class map_info
{
public:
    map_info();
    ~map_info();
    bool        IsAvailable();
    int         MapID;       // Unique map id (-1 for end of table)
    std::string Filename;    // Pointer to dfs file containing map
    std::string DisplayName; // Pointer to display name
    std::string Description; // Pointer to description of map
    int         Length;      // Length of the map when stored on the content server
    map_flags   Flags;       // map flags (see above)
    bool        operator==(const map_info& Right) const;
};

class map_entry
{
public:
    int         GetMapID() const;
    const char* GetFilename() const;
    const char* GetDisplayName() const;
    const char* GetDescription() const;
    map_flags   GetFlags() const;
    game_type   GetGameType() const;
    const char* GetShortGameTypeName() const;
    const char* GetGameTypeName() const;
    const char* GetGameRules() const;
    int         GetMaxPlayers() const { return m_MaxPlayers; }
    int         GetMinPlayers() const { return m_MinPlayers; }
    bool        IsAvailable() const;
    int         GetLocation() const { return m_Location; }
    void        SetLocation(int Location) { m_Location = Location; }

    bool operator==(const map_entry& MapEntry) const;
    bool operator!=(const map_entry& MapEntry) const;

private:
    game_type m_GameTypeID;
    int       m_MapID;
    int       m_MinPlayers; // min players for mode/zoning
    int       m_MaxPlayers; // max players for map in mode
    int       m_Location;

    friend class map_list;
};

class map_list
{
public:
    map_list();
    ~map_list();
    void                  Init();
    void                  LoadDefault();
    void                  Kill();
    void                  Clear();
    void                  Parse(const char* pMapFile, map_flags Flags, int Location);
    const map_entry*      Find(int MapID, int GameType = -1);
    int                   GetVersion();
    void                  SetVersion(int Version);
    const char*           GetDisplayName(int MapID);
    const char*           GetFileName(int MapID);
    map_entry*            GetByIndex(int MapIndex);
    map_entry*            GetNextMap(const map_entry* pCurrent);
    bool                  Append(const map_entry& Entry, const map_list* pSourceMapList = nullptr);
    int                   GetGameTypeCount() const { return m_GameTypes.size(); }
    const game_type_info* GetGameTypeInfo(int GameTypeID) const;
    int                   GetMapCount() const { return m_Maps.size(); }
    const map_info*       GetMapInfo(int MapID) const;
    void                  RemoveByFlags(map_flags Flags);
    void                  RemoveByMapID(int MapID);
    int                   GetCount() const { return m_MapList.size(); }
    //int                   Find(const map_entry& Entry) const { return m_MapList.Find(Entry); }
    std::string               Serialize() const;
    const map_entry&            operator[](int Index) const { return m_MapList[Index]; }

private:
    bool                        IsPresent(const char* pFilename);
    int                         m_Version;
    std::vector<map_entry>      m_MapList;
    std::vector<map_info>       m_Maps;
    std::vector<game_type_info> m_GameTypes;
    friend class map_entry;
};
