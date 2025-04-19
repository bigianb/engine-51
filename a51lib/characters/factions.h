#pragma once

#undef BIT
#define BIT(x) ( 1<<(x) )

enum factions
{
    FACTION_NONE            = 0,
    FACTION_PLAYER_NORMAL   = BIT( 0 ),     //use for matrix only, no character will belong to this faction
    FACTION_PLAYER_STRAIN1  = BIT( 1 ),     //use for matrix only, no character will belong to this faction
    FACTION_PLAYER_STRAIN2  = BIT( 2 ),     //use for matrix only, no character will belong to this faction
    FACTION_PLAYER_STRAIN3  = BIT( 3 ),     //use for matrix only, no character will belong to this faction

    FACTION_NEUTRAL         = BIT( 7 ),     // always friendly
    FACTION_BLACK_OPS       = BIT( 8 ),
    FACTION_MILITARY        = BIT( 9 ),
    FACTION_MUTANTS_LESSER  = BIT( 10 ),
    FACTION_MUTANTS_GREATER = BIT( 11 ),
    FACTION_GRAY            = BIT( 12 ),
    FACTION_THETA           = BIT( 13 ),
    FACTION_WORKERS         = BIT( 14 ),

    FACTION_TEAM_ONE        = BIT( 15 ),
    FACTION_TEAM_TWO        = BIT( 16 ),
    FACTION_DEATHMATCH      = BIT( 17 ),

    FACTION_NOT_SET = 0xFFFFFFFF,
    
    MAX_FACTION_COUNT       = 18,

    INVALID_FACTION         = -1
};

#undef BIT

/*
class factions_manager
{
public:
    factions_manager      ( );
    ~factions_manager     ( );
    
    static enum_table<factions>    s_FactionList; 
    
    static         void        OnEnumProp( prop_enum& rList );
    static         bool       OnProperty( prop_query& rPropQuery, factions& Faction, u32& Friends );

    static         void        OnEnumFaction( prop_enum& rList );
    static         void        OnEnumFriends( prop_enum& rList );

    static         bool       OnPropertyFaction( prop_query& rPropQuery, factions& rFaction );
    static         bool       OnPropertyFriends( prop_query& rPropQuery, u32& rFriends );

    static          int        GetFactionBitIndex( const factions& rFaction );
};
  
*/