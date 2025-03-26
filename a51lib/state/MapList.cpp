#include "MapList.h"

#include "../resourceManager/ResourceManager.h"
#include "../dataUtil/TextIn.h"

#include <iostream>
#include <cassert>
#include <cctype>
#include <string.h>

map_list::map_list()
{
    Clear();
    m_Version = 0;
}

map_list::~map_list()
{
}

void map_list::Init()
{
    Clear();
}

void map_list::LoadDefault(ResourceManager* rm)
{
    Clear();
    // These are in the root directory of preload.dfs
    const char* diskMapData = (const char*)rm->getResourceData("ENG_DiskMaps.TXT");
    Parse(diskMapData, MF_NOT_PRESENT, -1);
}

void map_list::Kill()
{
    Clear();
}

void map_list::Clear()
{
    m_GameTypes.clear();
    m_Maps.clear();
    m_MapList.clear();
}

//=========================================================================
// This will append an entry from one maplist to another. If the game type info is not
// within the new maplist, then all required fields will be copied. This will make the
// current manifest totally self-contained.
bool map_list::Append(const map_entry& Entry, const map_list* pSourceMapList)
{
    assert(false);
    /*
    if( m_MapList.Find( Entry ) != -1 )
    {
        return false;
    }

    if( pSourceMapList == nullptr )
    {
        pSourceMapList = &g_MapList;
    }

    // Ok, so this entry was not in the maplist. So, we duplicate it and append it to the maplist.

    map_entry& NewEntry = m_MapList.Append( );
    NewEntry = Entry;

    // We need to search through our GameType field in the new maplist to see if it's there. If so,
    // then we set up the index within this entry or we append it to our list of gametypes.
    game_type_info TempType;

    TempType.Type = Entry.m_GameTypeID;
    if( m_GameTypes.Find( TempType ) == -1 )
    {
        int GameTypeIndex = pSourceMapList->m_GameTypes.Find( TempType );
        if( GameTypeIndex != -1 )
        {
            m_GameTypes.push_back( pSourceMapList->m_GameTypes[GameTypeIndex] );
        }
        else
        {
            // Must have found it!
            GameTypeIndex = g_MapList.m_GameTypes.Find( TempType );
            //ASSERTS( GameTypeIndex != -1, "Could not find game type definition" );
            m_GameTypes.push_back( g_MapList.m_GameTypes[GameTypeIndex] );
        }
    }

    map_info TempMap;
    TempMap.MapID = Entry.m_MapID;
    if( m_Maps.Find( TempMap ) == -1 )
    {
        int MapIndex = pSourceMapList->m_Maps.Find( TempMap );
        if( MapIndex != -1 )
        {
            m_Maps.push_back( pSourceMapList->m_Maps[MapIndex] );
        }
        else
        {
            MapIndex = g_MapList.m_Maps.Find( TempMap );
            //ASSERTS( MapIndex != -1, "Could not find map definition" );
            m_Maps.push_back( g_MapList.m_Maps[MapIndex] );
        }

    }
*/
    return true;
}

const char* map_list::GetDisplayName(int MapID)
{
    const map_entry* pEntry;

    pEntry = Find(MapID);
    if (pEntry == nullptr) {
        return nullptr;
    }
    return pEntry->GetDisplayName();
}

const char* map_list::GetFileName(int MapID)
{
    const map_entry* pEntry;

    pEntry = Find(MapID);
    if (pEntry == nullptr) {
        return nullptr;
    }
    return pEntry->GetFilename();
}

const map_entry* map_list::Find(int MapID, int GameType)
{
    int i;
    for (i = 0; i < m_MapList.size(); i++) {
        if (MapID == -1) {
            if ((m_MapList[i].GetGameType() == GameType) && m_MapList[i].IsAvailable()) {
                return &m_MapList[i];
            }
        } else if (m_MapList[i].GetMapID() == MapID) {
            if ((GameType == -1) || (m_MapList[i].GetGameType() == GameType)) {
                return &m_MapList[i];
            }
        }
    }
    return nullptr;
}

map_entry* map_list::GetByIndex(int MapIndex)
{
    return &m_MapList[MapIndex];
}

bool map_list::IsPresent(const char* pFilename)
{
    int i;

    for (i = 0; i < m_MapList.size(); i++) {
        if (strcmp(pFilename, m_MapList[i].GetFilename()) == 0) {
            return true;
        }
    }
    return false;
}

map_entry* map_list::GetNextMap(const map_entry* pCurr)
{
    assert(false);

    return nullptr;
    /*
    game_type   GameType = pCurr->GetGameType();
    int         MinID    = S32_MAX;
    int         MaxID    = S32_MIN;
    int         MinIndex = -1;
    int         MaxIndex = -1;
    int         ResultIndex = -1;
    int         i;
    int         CurrentIndex = m_MapList.Find( *pCurr );

    assert( CurrentIndex != -1 );

    // Search the entire map list looking for the 'next' mapid
    for( i=0; i<m_MapList.size(); i++ )
    {
        if( (m_MapList[i].GetGameType() == GameType) && m_MapList[i].IsAvailable() )
        {
            if( m_MapList[i].GetMapID() < MinID )
            {
                MinID    = m_MapList[i].GetMapID();
                MinIndex = i;
            }

            if( m_MapList[i].GetMapID() > MaxID )
            {
                MaxID = m_MapList[i].GetMapID();
                MaxIndex = i;
            }
        }
    }

    // Wrap?
    if( CurrentIndex == MaxIndex )
    {
        ResultIndex = MinIndex;
    }
    else
    {
        ResultIndex = MaxIndex;
        for( i=0; i< m_MapList.size(); i++ )
        {
            if( (m_MapList[i].GetGameType() == GameType) && m_MapList[i].IsAvailable() )
            {
                if( (m_MapList[i].GetMapID() > m_MapList[ResultIndex].GetMapID()) && (m_MapList[i].GetMapID() < m_MapList[CurrentIndex].GetMapID()) )
                {
                    ResultIndex = i;
                }
            }
        }
    }

    if( ResultIndex==-1 )
    {
        return nullptr;
    }
    else
    {
        return &m_MapList[ResultIndex];
    }
        */
}

//=========================================================================

void map_list::RemoveByFlags(map_flags Flags)
{
    int i, j;
    /*
        //
        // Go through this maplist, remove any bindings that were added with the download flag set
        //
        for( i=0; i<m_MapList.size(); i++ )
        {
            if( (m_MapList[i].GetFlags() & Flags) == Flags )
            {
                m_MapList.Delete(i);
                i--;
            }
        }

        //
        // All flags have been removed. Now go through the gametype list and remove
        // any entries that have no references to them.
        //
        for( i=0; i<m_GameTypes.size(); i++ )
        {
            // For each game type, search map bindings for a reference to it.
            bool Found = false;
            for( j=0; j<m_MapList.size(); j++ )
            {
                if( m_MapList[j].GetGameType() == m_GameTypes[i].Type )
                {
                    Found = true;
                    break;
                }
            }
            if( !Found )
            {
                m_GameTypes.Delete(i);
                i--;
            }
        }

        //
        // Now go through the maps list and remove any no longer referenced maps
        //
        for( i=0; i<m_Maps.size(); i++ )
        {
            // For each game type, search map bindings for a reference to it.
            bool Found = false;
            for( j=0; j<m_MapList.size(); j++ )
            {
                if( m_MapList[j].GetMapID() == m_Maps[i].MapID )
                {
                    Found = true;
                    break;
                }
            }
            if( !Found )
            {
                m_Maps.Delete(i);
                i--;
            }
        }
            */
}

//=========================================================================

void map_list::RemoveByMapID(int MapID)
{
    int i, j;
    /*
        //
        // Go through this maplist, remove any bindings for this specific mapid.
        //
        for( i=0; i<m_MapList.size(); i++ )
        {
            if( m_MapList[i].GetMapID() == MapID )
            {
                m_MapList.Delete(i);
                i--;
            }
        }

        //
        // All flags have been removed. Now go through the gametype list and remove
        // any entries that have no references to them.
        //
        for( i=0; i<m_GameTypes.size(); i++ )
        {
            // For each game type, search map bindings for a reference to it.
            bool Found = false;
            for( j=0; j<m_MapList.size(); j++ )
            {
                if( m_MapList[j].GetGameType() == m_GameTypes[i].Type )
                {
                    Found = true;
                    break;
                }
            }
            if( !Found )
            {
                m_GameTypes.Delete(i);
                i--;
            }
        }

        //
        // Now go through the maps list and remove any no longer referenced maps
        //
        for( i=0; i<m_Maps.size(); i++ )
        {
            // For each game type, search map bindings for a reference to it.
            bool Found = false;
            for( j=0; j<m_MapList.size(); j++ )
            {
                if( m_MapList[j].GetMapID() == m_Maps[i].MapID )
                {
                    Found = true;
                    break;
                }
            }
            if( !Found )
            {
                m_Maps.Delete(i);
                i--;
            }
        }
            */
}

enum file_section
{
    SECTION_UNDEFINED = 0,
    SECTION_VERSION,
    SECTION_GAMETYPE,
    SECTION_MAPTYPE,
    SECTION_PLAY,
};

void uppercase(char* c){
    while (*c){
        *c = toupper(*c);
        ++c;
    }
}

void map_list::Parse(const char* pMapFile, map_flags Flags, int Location)
{
    assert(pMapFile != nullptr);

    token_stream Stream;
    file_section FileSection = SECTION_UNDEFINED;
    int          Cursor;

    Stream.OpenText(pMapFile);

    while (true) {
        Cursor = Stream.GetCursor();
        Stream.Read();
        if (Stream.IsEOF()) {
            break;
        }
        const char* strTok = Stream.String();
        char upperTok[32];
        strncpy(upperTok, strTok, 31);
        upperTok[31] = 0;
        uppercase(upperTok);
        if (strcmp(upperTok, "[VERSION]") == 0) {
            FileSection = SECTION_VERSION;
        } else if (strcmp(upperTok, "[GAMETYPE]") == 0) {
            FileSection = SECTION_GAMETYPE;
        } else if (strcmp(upperTok, "[MAP]") == 0) {
            FileSection = SECTION_MAPTYPE;
        } else if (strcmp(upperTok, "[PLAY]") == 0) {
            FileSection = SECTION_PLAY;
        } else if (FileSection == SECTION_VERSION) {
            Stream.SetCursor(Cursor);
            m_Version = Stream.ReadInt();
            Stream.Read();
            assert(strcmp(Stream.String(), ";") == 0);
        } else if (FileSection == SECTION_GAMETYPE) {
            //
            // Entries within this section of the file take the following format:
            // <GameTypeID>, <ShortTypeName>, <LongTypeName>, <Description>
            // e.g. -2, "SOLO", "Campaign", "This is a campaign"
            //
            // It is used to describe a game type
            //
            game_type_info TypeEntry;

            Stream.SetCursor(Cursor);
            TypeEntry.Type = (game_type)Stream.ReadInt();
            Stream.Read();
            assert(strcmp(Stream.String(), ",") == 0);
            TypeEntry.ShortTypeName = Stream.ReadString();
            Stream.Read();
            assert(strcmp(Stream.String(), ",") == 0);
            TypeEntry.TypeName = Stream.ReadString();
            Stream.Read();
            assert(strcmp(Stream.String(), ",") == 0);
            TypeEntry.Rules = Stream.ReadString();
            Stream.Read();
            while (strcmp(Stream.String(), ";") != 0) {
                TypeEntry.Rules += "\n";
                TypeEntry.Rules += Stream.String();
                Stream.Read();
            }
            m_GameTypes.push_back(TypeEntry);
        } else if (FileSection == SECTION_MAPTYPE) {
            //
            // Entries within this section of the file take the following format:
            // <mapid>, <path-to-dfs-file>, <display-name>, <description>
            // e.g. 3000, "mp_00\blaze", "Blaze", "This is the blaze map"
            //
            // It is used to describe an individual map
            //
            map_info MapInfo;

            Stream.SetCursor(Cursor);
            MapInfo.MapID = Stream.ReadInt();
            Stream.Read();
            assert(strcmp(Stream.String(), ",") == 0);
            MapInfo.Filename = Stream.ReadString();
            Stream.Read();
            assert(strcmp(Stream.String(), ",") == 0);
            Stream.Read();
            if (Stream.Type() == token_stream::TOKEN_NUMBER) {
                MapInfo.Length = Stream.Int();
                Stream.Read();
                assert(strcmp(Stream.String(), ",") == 0);
                Stream.Read();
            } else {
                MapInfo.Length = 0;
            }

            MapInfo.DisplayName = Stream.String();
            Stream.Read();
            assert(strcmp(Stream.String(), ",") == 0);
            MapInfo.Description = Stream.ReadString();
            Stream.Read();
            MapInfo.Flags = Flags;
            while (strcmp(Stream.String(), ";") != 0) {
                assert(Stream.Type() == token_stream::TOKEN_STRING); // "Expected quoted string or ;"
                MapInfo.Description += "\n";
                MapInfo.Description += Stream.String();
                Stream.Read();
            };
            m_Maps.push_back(MapInfo);
        } else if (FileSection == SECTION_PLAY) {
            //
            // Entries within this section of the file take the following format:
            // <gametypeid>, <mapid>, <minplayers>, <maxplayers>
            // e.g. -2, 3000, 1, 16
            //
            // It is used to bind a game type to a map
            //
            Stream.SetCursor(Cursor);

            token_stream::type TokenType = Stream.Read();

            if (TokenType == token_stream::TOKEN_NUMBER) {
                map_entry MapEntry;

                MapEntry.m_GameTypeID = (game_type)Stream.Int();
                Stream.Read();
                assert(strcmp(Stream.String(), ",") == 0);
                MapEntry.m_MapID = Stream.ReadInt();
                Stream.Read();
                assert(strcmp(Stream.String(), ",") == 0);
                MapEntry.m_MinPlayers = Stream.ReadInt();
                Stream.Read();
                assert(strcmp(Stream.String(), ",") == 0);
                MapEntry.m_MaxPlayers = Stream.ReadInt();
                Stream.Read();
                assert(strcmp(Stream.String(), ";") == 0);
                MapEntry.m_Location = Location;

                // Verify that we have the game type and map already defined for this play binding.
                //ASSERTS(GetMapInfo(MapEntry.m_MapID), "Map needs to be defined before bind.");
                //ASSERTS(GetGameTypeInfo(MapEntry.m_GameTypeID), "GameType needs to be defined before bind.");

                m_MapList.push_back(MapEntry);
            } else {
                Stream.SkipToNextLine();
            }
        } else {
            std::cout << "Expected [VERSION]|[GAMETYPE]|[MAP]|[PLAY]" << std::endl;
            assert(false);
        }
    }
    Stream.CloseText();
}

//=========================================================================

void map_list::SetVersion(int Version)
{
    m_Version = Version;
}

//=========================================================================

int map_list::GetVersion()
{
    return m_Version;
}

const game_type_info* map_list::GetGameTypeInfo(int TypeID) const
{
    /*
    int i;
    game_type_info Dummy;

    Dummy.Type = (game_type)TypeID;

    i=m_GameTypes.Find( Dummy );
    if( i!= -1 )
    {
        return &m_GameTypes[i];
    }
    if( this != &g_MapList )
    {
        return g_MapList.GetGameTypeInfo( TypeID );
    }
        */
    return nullptr;
}

//=========================================================================
const map_info* map_list::GetMapInfo(int MapID) const
{
    /*
    int i;
    map_info Dummy;

    Dummy.MapID = MapID;
    i = m_Maps.Find( Dummy );
    if( i!=-1 )
    {
        return &m_Maps[i];
    }

    if( this != &g_MapList )
    {
        return g_MapList.GetMapInfo( MapID );
    }
        */
    return nullptr;
}

int map_entry::GetMapID() const
{
    return m_MapID;
}

const char* map_entry::GetFilename() const
{
    return nullptr; //g_MapList.GetMapInfo( m_MapID )->Filename;
}

const char* map_entry::GetDisplayName() const
{
    return nullptr; //g_MapList.GetMapInfo( m_MapID )->DisplayName;
}

const char* map_entry::GetDescription() const
{
    return nullptr; //g_MapList.GetMapInfo( m_MapID )->Description;
}

map_flags map_entry::GetFlags() const
{
    return map_flags::MF_DVD_MAP; //g_MapList.GetMapInfo( m_MapID )->Flags;
}

game_type map_entry::GetGameType() const
{
    return m_GameTypeID;
}

const char* map_entry::GetShortGameTypeName() const
{

    return nullptr; //g_MapList.GetGameTypeInfo( m_GameTypeID )->ShortTypeName;
}

const char* map_entry::GetGameTypeName() const
{
    return nullptr; //g_MapList.GetGameTypeInfo( m_GameTypeID )->TypeName;
}

const char* map_entry::GetGameRules() const
{
    return nullptr; //g_MapList.GetGameTypeInfo( m_GameTypeID )->Rules;
}

bool map_entry::IsAvailable() const
{
    return GetFlags() != MF_NOT_PRESENT;
}

bool map_entry::operator==(const map_entry& MapEntry) const
{
    return (m_GameTypeID == MapEntry.m_GameTypeID) && (m_MapID == MapEntry.m_MapID);
}

bool map_entry::operator!=(const map_entry& MapEntry) const
{
    return (m_GameTypeID != MapEntry.m_GameTypeID) || (m_MapID != MapEntry.m_MapID);
}

map_info::map_info()
{
    MapID = -1;
    Flags = MF_NOT_PRESENT;
}

map_info::~map_info()
{
}

bool map_info::operator==(const map_info& Right) const
{
    return (Right.MapID == MapID);
}

game_type_info::game_type_info()
{
    Type = GAME_DM;
}

game_type_info::~game_type_info()
{
}

bool game_type_info::operator==(const game_type_info& Right) const
{
    return (Right.Type == Type);
}

bool map_info::IsAvailable()
{
    return (Flags != MF_NOT_PRESENT);
}
