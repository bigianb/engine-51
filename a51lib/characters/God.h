#pragma once

#include "../objects/Object.h"
#include "../objectManager/ObjectManager.h"
#include "../xfiles/x_time.h"
//#include "AStar.hpp"
//#include "Characters\AlertPackage.hpp"
//#include "Navigation\ng_connection2.hpp"
//#include "TriggerEX\Actions\action_music_intensity.hpp"


const int k_NumTargettingData = 32;
const int k_MaxMeleeingPlayer = 2;

// God class - takes care of telling all characters what to do
class God : public Object
{
public:
    CREATE_RTTI( God, Object, Object )

    const object_desc&  GetTypeDesc() const override;
    static  const object_desc&  GetObjectType();

    struct TargettingData
    {
        TargettingData(guid targetter, guid targetGuid, float distanceSqr );
        TargettingData() { Clear(); }        
        void Clear();

        guid m_Targetter;
        guid m_TargetGuid;
        float  m_DistanceSqr;
    };

//=========================================================================
// Class functions
//=========================================================================
public:
            God(ObjectManager* om, ResourceManager* rm);
    virtual ~God();
    
//=========================================================================
// Inherited virtual functions from base class
//=========================================================================

    void    OnActivate              ( bool Flag ) override;            
    void    OnKill                  () override;   
    void    OnAdvanceLogic          ( float DeltaTime ) override;

    int     GetMaterial             ( ) const override;
    BBox    GetLocalBBox            ( ) const override;
/*
            bool   RequestPathWithEdges    ( Object*               pRequestObject, 
                                              const Vector3&        vDestination, 
                                              path_find_struct&     rPathStruct, 
                                              int                   nMaxEdgeListSize,
                                              const pathing_hints*  pPathingHints = nullptr,
                                              bool                 bDestInSameGrid = true );

            bool   RequestRetreatPath      ( Object*               pRequestObject, 
                                              const Vector3&        vRetreatFrom,
                                              float                   DesiredMinDistance,
                                              path_find_struct&     rPathStruct, 
                                              int                   nMaxEdgeListSize,
                                              const pathing_hints*  pPathingHints = nullptr );
 */           
            int     PlayAlertSound          ( const char* pObjectName, const char* pAction, int State, guid ObjGuid,
                                              short ZoneID, Vector3& Pos );


            bool   IsActiveZone            ( unsigned short ZoneID );
            float     GetMinDistanceToPlayersThroughZones( const Vector3& position, unsigned short ZoneID  );
            bool   GetCanMeleePlayer       ( guid requestNPC );
            
            void    AddTargettingData       ( TargettingData newTargetData );           // adds data to the list of targetting data
            int     GetNumTargettingCloser  ( TargettingData newTargetData );           // this tells us the number targeting our target unless we are the closest, then it returns 0.



//    astar_path_finder   m_AStarPathFinder;          // pathfinder
protected:    
    TargettingData      m_CurrentTargettingData[k_NumTargettingData];
    TargettingData      m_LastTickTargettingData[k_NumTargettingData];

    int                 m_ActiveThinkID ;           // Current ID of character that can think
    guid                m_MeleeingPlayerGuids[k_MaxMeleeingPlayer];       // guid of the NPC currently meleeing the player.   
    xtick               m_SoundTimer;
    xtick               m_GrenadeTimer;
    float                 m_TimeDeltaToTalk;      
    int                 m_LastTalkState;

protected:
    void OnEnumProp(prop_enum& List) override;
    bool OnProperty(prop_query& I) override;
};
