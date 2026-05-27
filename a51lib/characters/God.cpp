
#include "God.h"
#include "../math/Random.h"
//#include "Characters\Character.hpp"
#include "../objects/Corpse.h"
//#include "Gamelib\StatsMgr.hpp"
//#include "ConversationMgr\ConversationMgr.hpp"
//#include "MusicStateMgr\MusicStateMgr.hpp"
#include "../objectManager/ObjectManager.h"
#include "../objectManager/ObjectPtr.h"

static const float  s_KeepActiveAfterRendering      =    2.0f;
static const int    k_MaxNumPlayers                 =    5;
const float         k_MinActiveDistance             =    10000.0f;
float g_MinGodTimeTalk    = 10.0f;
float g_MaxGodTimeTalk    = 20.0f;


//=========================================================================
// OBJECT DESCRIPTION
//=========================================================================

static struct god_desc : public object_desc
{
        god_desc() : object_desc( 
            Object::TYPE_GOD, 
            "God", 
            "AI",

            Object::ATTR_NEEDS_LOGIC_TIME,

            FLAGS_NO_EDITOR_RENDERABLE )   { }

    Object* Create(ObjectManager* om, collision_mgr*, ResourceManager* rm) override { return new God(om, rm); }
    void OnEnumProp(prop_enum& List) override;
    bool OnProperty(prop_query& I) override;

} s_god_Desc;

const object_desc& God::GetObjectType()
{
    return s_god_Desc;
}

const object_desc& God::GetTypeDesc() const
{
    return s_god_Desc;
}

void god_desc::OnEnumProp( prop_enum&   List )
{
    // Call base class
    object_desc::OnEnumProp( List ) ;

    // Character debug
    List.PropEnumHeader  ("CharacterDebug", "Character debug settings.", 0 );
    List.PropEnumBool    ("CharacterDebug\\Loco",  "Render debug for loco lookat, moveat", 0 );
    List.PropEnumBool    ("CharacterDebug\\AI",    "Render debug for sight etc", 0 );
    List.PropEnumBool    ("CharacterDebug\\Path",  "Render debug for pathfinding", 0 );
    List.PropEnumBool    ("CharacterDebug\\Stats", "Shows # of meshes, verts, faces, bones etc", 0 );
}

bool god_desc::OnProperty( prop_query& I )
{
    // Call base class
    if ( object_desc::OnProperty( I ) )
        return true ;

#if 0
//#ifndef X_RETAIL
    // Character debug render
    if ( I.VarBool( "CharacterDebug\\Loco",   character::s_bDebugLoco ) )
        return true ;
    
    // Character debug render
    if ( I.VarBool( "CharacterDebug\\AI",     character::s_bDebugAI ) )
        return true ;
    
    // Character debug render
    if ( I.VarBool( "CharacterDebug\\Path",   character::s_bDebugPath ) )
        return true ;
    
    // Character mesh,vert,face,bone counts etc
    if ( I.VarBool( "CharacterDebug\\Stats",  character::s_bDebugStats ) )
        return true ;
#endif // X_RETAIL

    return false ;
}


//=============================================================================
//=============================================================================

God::TargettingData::TargettingData(guid targetter, guid targetGuid, float distanceSqr ):
    m_Targetter(targetter),
    m_TargetGuid(targetGuid),
    m_DistanceSqr(distanceSqr)
{
}

void God::TargettingData::Clear()
{
    m_TargetGuid = NULL_GUID;
    m_Targetter = NULL_GUID;
    m_DistanceSqr = -1.0f;
}

//=========================================================================
// FUNCTIONS
//=========================================================================

God::God(ObjectManager* om, ResourceManager* rm) : Object(om)
{
    m_ActiveThinkID = -1 ;  // Current ID of character that can think
    m_SoundTimer = 0;
    m_GrenadeTimer = 0;

    m_TimeDeltaToTalk = x_frand( g_MinGodTimeTalk, g_MaxGodTimeTalk );
}

//=========================================================================

God::~God()
{
}

//=============================================================================

void God::OnActivate( bool bActive )
{

    m_ActiveThinkID = -1 ;  // Current ID of character that can think

    // IJB:
    //m_AStarPathFinder.ResetNumNodes() ;

}

//=============================================================================

void God::OnKill()
{

}

//=============================================================================

bool IntersectBBoxes( BBox ActiveBBox[k_MaxNumPlayers], Object* pObject, int MaxIndex )
{
    if ( MaxIndex < 0 )
        return false;

    int i = 0;
    
    while( i < MaxIndex )
    {
        if ( ActiveBBox[i].Intersect( pObject->GetBBox() ) ){
            return true;
        }
        
        i++;
    }
    
    return false;
}

//=============================================================================

bool God::GetCanMeleePlayer( guid reqestNPC )
{
    int c;
    for(c=0;c<k_MaxMeleeingPlayer;c++)
    {
        if( reqestNPC == m_MeleeingPlayerGuids[c] || 
            m_MeleeingPlayerGuids[c] == NULL_GUID )
        {
            return true;
        }
    }
    return false;
}

//=============================================================================

void God::OnAdvanceLogic( float DeltaTime )
{
    // Create infinite BBox incase a player is not found
    BBox ActiveBBox[k_MaxNumPlayers];

    for (int i = 0; i < k_MaxNumPlayers; i++)
    {
        ActiveBBox[i].Clear();
    }

    int CurrentBBox = -1;

    // Grab active BBox around all players
    slot_id PlayerID = getObjectManager()->GetFirst(Object::TYPE_PLAYER) ;
        
    while ( PlayerID != SLOT_NULL )
    {  
        CurrentBBox++;

        if (CurrentBBox >= k_MaxNumPlayers)
            break;

        Object* pObject = getObjectManager()->GetObjectBySlot( PlayerID );
        
        if (pObject == NULL)
        {
            PlayerID = getObjectManager()->GetNext(PlayerID);
            continue;
        }

        ActiveBBox[CurrentBBox] = pObject->GetBBox();
        ActiveBBox[CurrentBBox].Inflate( 2000.f, 2000.f, 2000.f ) ;
      
        PlayerID = getObjectManager()->GetNext(PlayerID);
    }

    // Loop through all characters, setting up their active flag and 
    // records which character should think next (if any)
    character* pActiveThink = NULL ;
    int        ActiveCount = 0 ;

    //=============================================================================
    //
    //  re-wrote this logic Jim
    //
    //  Used to ask Object manager for a list of all objects with an attribute
    //  and Object manager would check every Object in the world and see what flags
    //  it had.  I changed it to walk the list of Object types and chack the first
    //  Object in the list and just check it.  If it has the flag then we assume
    //  all objects of that type have the flag and walk through just the objects
    //  of that type
    //
    //      -CDS
    //=============================================================================
 
    int c;
    for(c=0;c<k_MaxMeleeingPlayer;c++)
    {    
        m_MeleeingPlayerGuids[c] = NULL_GUID;
    }
#if 0
    unsigned int objectTypeCount;
    int HighestAwareness = 0;
    for(objectTypeCount = 0; objectTypeCount < TYPE_END_OF_LIST; objectTypeCount++)
    {
        slot_id SlotID = getObjectManager()->GetFirst((Object::type)objectTypeCount);

        if( SlotID != SLOT_NULL )
        {
            Object* tempObject = getObjectManager()->GetObjectBySlot(SlotID);

            if(tempObject->GetAttrBits() & Object::ATTR_CHARACTER_OBJECT )
            {
                while(SlotID != SLOT_NULL)
                {
                    // Lookup the actual character
                    character* pCharacter = (character*)getObjectManager()->GetObjectBySlot(SlotID) ;
                    assert(pCharacter) ;
                    assert(pCharacter->IsKindOf( character::GetRTTI() ) ) ;

                    if( pCharacter->IsMeleeingPlayer() )
                    {
                        for(c=0;c<k_MaxMeleeingPlayer;c++)
                        {
                            if( m_MeleeingPlayerGuids[c] == NULL_GUID )
                            {                            
                                m_MeleeingPlayerGuids[c] = pCharacter->GetGuid();
                                break;
                            }
                        }
                    }

                    bool bShouldBeActive = false;

                    // Setup active flag
                    if (pCharacter->m_pActiveState && pCharacter->m_pActiveState->m_State == character_state::STATE_HOLD)
                    {
                        // Force active if character should be killed now, otherwise force inactive
                        bShouldBeActive = pCharacter->GetAutoRagdoll();
                    }
                    /*else if (pCharacter->m_pActiveState && pCharacter->m_pActiveState->m_State == character_state::STATE_GOAL)
                    {
                        //goal state is always active
                        pCharacter->SetIsActive( true );
                    }*/
                    else if (pCharacter->m_pActiveState && pCharacter->m_pActiveState->m_State == character_state::STATE_TRIGGER)
                    {
                        //goal state is always active
                        bShouldBeActive = true;
                    }                    
                    else if ( pCharacter->IsDead() )
                    {
                        //dead, so must be active to finish death anim
                        bShouldBeActive = true;
                    }
                    else if ( pCharacter->TimeSinceLastRender() < s_KeepActiveAfterRendering )
                    {
                        //keep active for a few seconds after last rendering
                        bShouldBeActive = true;
                    }
                    else if ( IntersectBBoxes(ActiveBBox, pCharacter, CurrentBBox) )
                    {
                        //ok are we in an active zone
                        bShouldBeActive = true;
                    }
                    else if (IsActiveZone( pCharacter->GetZone1() ))
                    {
                        //in an adjacent zone
                        bShouldBeActive = true;
                    }
                    else if( GetMinDistanceToPlayersThroughZones(pCharacter->GetPosition(), pCharacter->GetZone1()) < (k_MinActiveDistance*k_MinActiveDistance) )
                    {
                        //in an adjacent zone
                        bShouldBeActive = true;
                    }

                    // Finally set the actual value
                    pCharacter->SetIsActive( bShouldBeActive );

                    // Do?
                    if( pCharacter->IsActive() &&
                        pCharacter->GetDoRunLogic() )
                    {
                        if( pCharacter->GetAwarenessLevel() > HighestAwareness )
                        {
                            HighestAwareness = pCharacter->GetAwarenessLevel();
                        }

                        // NOTE: Be careful if you tell a character to do anything
                        //       which will perform collision checks etc since
                        //       currently the Object manager loops cannot be nested!
                        //       That's why I have to keep a pointer to the active thinker
                        //       and call the OnThink functions outside this loop.

                        // Keep this character for later so we can make it think
                        if (ActiveCount == m_ActiveThinkID)
                            pActiveThink = pCharacter ;

                        // Update count
                        ActiveCount++ ;
                    }

                    // Check next
                    SlotID = getObjectManager()->GetNext(SlotID) ;
                }
            }
        }
    }
    
    // IJB: g_MusicStateMgr.SetAwarenessLevel( HighestAwareness, DeltaTime );

    // update targetting info. 
    for(c=0;c<k_NumTargettingData;c++)
    {
        m_LastTickTargettingData[c] = m_CurrentTargettingData[c];
        m_CurrentTargettingData[c].Clear();
    }

    // Let one character do some thinking this frame
    if (pActiveThink)
    {
        // Flag as thinking and then think!
        pActiveThink->m_bThinking = true ;
        pActiveThink->OnThink() ;
        pActiveThink->m_bThinking = false ;
    }
#endif // 0

    // Goto next thinking ID
    if (++m_ActiveThinkID >= ActiveCount){
        m_ActiveThinkID = 0 ;
    }
}

int God::GetMaterial() const
{
    // Not really sure what "God" is made of, but this will do!
    return MAT_TYPE_FLESH ;
}

//=============================================================================

BBox God::GetLocalBBox() const
{
    BBox BBox ;
    BBox.Set(Vector3(0,0,0), 0) ;
    return BBox ;
}

void God::OnEnumProp( prop_enum& List )
{
}

bool God::OnProperty( prop_query& I )
{
    return false ;
}

void God::AddTargettingData( TargettingData newTargetData )
{
    // don't add targetting null.
    if( newTargetData.m_TargetGuid == NULL_GUID )
        return;

    int c;
    for (c=0;c<k_NumTargettingData;c++)
    {
        // search for the first empty one. once found add us. 
        if( m_CurrentTargettingData[c].m_TargetGuid == NULL_GUID )
        {
            m_CurrentTargettingData[c] = newTargetData;
            return;
        }
    }
    // we are full. Oh well...
}

//=========================================================================

int God::GetNumTargettingCloser( TargettingData newTargetData )
{
    if( newTargetData.m_TargetGuid == NULL_GUID )
        return 0;

    int numCloser = 0;
    // check against last ticks data.
    int c;
    for(c=0;c<k_NumTargettingData;c++)
    {
        // look for data on this target
        if( m_LastTickTargettingData[c].m_TargetGuid == newTargetData.m_TargetGuid )
        {
            // only add if not us and it's closer.
            if( m_LastTickTargettingData[c].m_Targetter != newTargetData.m_Targetter &&
                m_LastTickTargettingData[c].m_DistanceSqr < newTargetData.m_DistanceSqr )
            {
                numCloser++;
            }
        }
        else if( m_LastTickTargettingData[c].m_TargetGuid == NULL_GUID )
        {
            // we reached the end of the list break;
            break;
        }
    }
    // we reached the end of the list and not found, so no targetters.
    return numCloser;
}


//=========================================================================
/*
bool   God::RequestPath( const int SourceNodeIndex, const int DestNodeIndex, const guid RequestorGuid , int* PathList , int PathCount )
{ 
    CONTEXT("God::RequestPath1") ;

    //get the references to the nodes.
    ng_node2& SourceNode = g_NavMap.GetNodeByID( SourceNodeIndex );
    ng_node2& DestNode   = g_NavMap.GetNodeByID( DestNodeIndex );

    bool RetVal;
    if ( SourceNode.GetGridID() != DestNode.GetGridID() )
    {
        RetVal = false;
    }
    else
    if ( SourceNodeIndex == DestNodeIndex )
    {
        RetVal = false;
    }
    else
    {
        //generate the path.
        RetVal = m_AStarPathFinder.GeneratePath( &SourceNode, &DestNode, RequestorGuid, PathList, PathCount );
    }

    return RetVal;
}

//=========================================================================

bool   God::RequestPath( Vector3 SourcePos ,Vector3 DestPos ,guid RequestorGuid, int* PathList , int PathCount )
{
    CONTEXT("God::RequestPath2") ;

    //get the nearest node to SourcePos and to DestPos
    int SourceID = g_NavMap.GetNearestNode( SourcePos );
    int DestID   = g_NavMap.GetNearestNode( DestPos );
    return RequestPath( SourceID , DestID , RequestorGuid, PathList, PathCount );
}

//=========================================================================

bool   God::RequestPath( int SourceID ,Vector3 DestPos ,guid RequestorGuid, int* PathList , int PathCount )
{
    CONTEXT("God::RequestPath3") ;

    //get the nearest node to SourcePos and to DestPos
    int DestID   = g_NavMap.GetNearestNode( DestPos );
    return RequestPath( SourceID , DestID , RequestorGuid, PathList, PathCount );
}
*/
//=========================================================================

int God::PlayAlertSound( const char* pObjectName, const char* pAction, int State, guid ObjGuid, short ZoneID, Vector3& Pos )
{
    int iId = 0;
    
    float TimePassed = getObjectManager()->GetGameDeltaTime( m_SoundTimer );

    if( TimePassed > m_TimeDeltaToTalk )
    {
        m_TimeDeltaToTalk = x_frand( g_MinGodTimeTalk, g_MaxGodTimeTalk );
        m_LastTalkState = 0;
    }

    if(  m_LastTalkState != State )
    {
        // IJB: 
        // iId = g_ConverseMgr.PlayStream( pObjectName, pAction, ObjGuid, ZoneID, Pos );
        m_LastTalkState = State;

        m_SoundTimer = getObjectManager()->GetGameTime();
    }

    return iId;
}

//=========================================================================

float God::GetMinDistanceToPlayersThroughZones( const Vector3& position, unsigned short zoneID )
{
    // Grab all players
    slot_id PlayerID = getObjectManager()->GetFirst(Object::TYPE_PLAYER) ;
    float minDistSquared = 100000.0f * 100000.0f;    

    while ( PlayerID != SLOT_NULL )
    {  
        Object* pObject = getObjectManager()->GetObjectBySlot( PlayerID );

        if (pObject == NULL)
        {
            PlayerID = getObjectManager()->GetNext(PlayerID);
            continue;
        }

        float zoneDistSquared = (position - pObject->GetPosition()).LengthSquared();
        if( zoneDistSquared < minDistSquared )
        {
            minDistSquared = zoneDistSquared;
        }
        PlayerID = getObjectManager()->GetNext(PlayerID);
    }
    return minDistSquared;    
}

//=========================================================================

bool God::IsActiveZone( unsigned short ZoneID )
{
    //query the zonemngr for all active zones .... (ASK TOMAS TO IMPLEMENT)

    // Grab all players
    slot_id PlayerID = getObjectManager()->GetFirst(Object::TYPE_PLAYER) ;
        
    while ( PlayerID != SLOT_NULL )
    {  
        Object* pObject = getObjectManager()->GetObjectBySlot( PlayerID );

        if (pObject == NULL)
        {
            PlayerID = getObjectManager()->GetNext(PlayerID);
            continue;
        }

        unsigned short PlayerZoneID = pObject->GetZone1();

        if ( ZoneID == PlayerZoneID )
            return true;    

        if ( g_ZoneMgr.IsAdjacentZone(PlayerZoneID, ZoneID) )
            return true;

        PlayerID = getObjectManager()->GetNext(PlayerID);
    }

    return false;
}

//===========================================================================
static int      g_pPathList[ 50 ];
static int      g_PathCount = 50;
/*
bool God::RequestPathWithEdges( Object*                pRequestObject, 
                                 const Vector3&         vDestination, 
                                 path_find_struct&      rPathStruct, 
                                 int                    nMaxEdgeListSize,
                                 const pathing_hints*   pPathingHints,
                                 bool                  bDestInSameGrid )                                 
{
    (void)nMaxEdgeListSize;

    pathing_hints   DefaultHints;
    if ( NULL == pPathingHints )
        pPathingHints = &DefaultHints;

    // Let's clear out the path list.
    memset( g_pPathList, -1, g_PathCount * sizeof( int ) );
    rPathStruct.Clear();

    // If not using nav map, go straight to destination
    if( pPathingHints->bUseNavMap == false )
    {
        rPathStruct.m_vStartPoint             = vDestination;
        rPathStruct.m_vEndPoint               = vDestination;
        rPathStruct.m_bStartPointOnConnection = true;
        rPathStruct.m_bEndPointOnConnection   = true;
        rPathStruct.m_bStraightPath           = true;
        rPathStruct.m_StartConnectionSlotID   = g_NavMap.GetNearestConnection( pRequestObject->GetPosition() );
        rPathStruct.m_EndConnectionSlotID     = g_NavMap.GetNearestConnection( vDestination );
        
        rPathStruct.m_StepData[0].m_CurrentConnection = rPathStruct.m_StartConnectionSlotID;
        rPathStruct.m_StepData[0].m_DestConnection    = rPathStruct.m_EndConnectionSlotID;
        rPathStruct.m_StepData[0].m_NodeToPassThrough = NULL_NAV_SLOT;
        rPathStruct.m_nSteps = 0;
        return true;
    }

    if (g_NavMap.GetConnectionCount() == 0)
    {
        // There are no nav connections to work with.
        return false;
    }

    Vector3 vStartPos = pRequestObject->GetPosition();

    rPathStruct.m_vStartPoint = vStartPos;
    rPathStruct.m_vEndPoint   = vDestination;

    // Find the start connection
    nav_connection_slot_id StartConnectionSlot = g_NavMap.GetNearestConnection( vStartPos );

    // We cannot build a path if the start point isn't even inside the navmap.
    // A base state should be directing the NPC to step back into the navmap before it
    // even bothers with pathfinding.
    if (NULL_NAV_SLOT == StartConnectionSlot)
        return false;

    ng_connection2&        StartConnection     = g_NavMap.GetConnectionByID( StartConnectionSlot );

    // Find the end connection.
    nav_connection_slot_id EndConnectionSlot;
    if( bDestInSameGrid )
    {    
        EndConnectionSlot = g_NavMap.GetNearestConnectionInGrid( vDestination, StartConnection.GetGridID() );
    }
    else
    {
        EndConnectionSlot = g_NavMap.GetNearestConnection( vDestination );
    }
    ng_connection2&        EndConnection     = g_NavMap.GetConnectionByID( EndConnectionSlot );
    
    // if not in same grid, can't path to.
    if( StartConnection.GetGridID() != EndConnection.GetGridID() )
        return false;

    // Get connection info for path end pts
    rPathStruct.m_bStartPointOnConnection = g_NavMap.GetConnectionContainingPoint( rPathStruct.m_StartConnectionSlotID, vStartPos );
    rPathStruct.m_bEndPointOnConnection   = g_NavMap.GetConnectionContainingPoint( rPathStruct.m_EndConnectionSlotID, vDestination );
    
    // If the connections are the same, don't worry about pathfinding because we're there
    if(         ( rPathStruct.m_StartConnectionSlotID == rPathStruct.m_EndConnectionSlotID )
            ||  ( g_NavMap.DoesStraightPathExist( rPathStruct ) ) )    
    {        
        rPathStruct.m_bStraightPath = true;
        rPathStruct.m_nSteps = 0;
        rPathStruct.m_StepData[0].m_CurrentConnection = StartConnectionSlot;
        rPathStruct.m_StepData[0].m_DestConnection    = EndConnectionSlot;
        rPathStruct.m_StepData[0].m_NodeToPassThrough = NULL_NAV_SLOT;
        
        return true;        
    }

    // Let's get a path from the pathfinder.    
    int nStepsInPath = 0;
    if ( !m_AStarPathFinder.GeneratePath( &StartConnection, 
                                          &EndConnection,
                                          vDestination,
                                          pRequestObject->GetGuid(),
                                          *pPathingHints,
                                          g_pPathList,
                                          g_PathCount,
                                          nStepsInPath ))
    {
        // No path was found
        return false;
    }

    rPathStruct.m_nSteps = nStepsInPath;

    int i;
    for (i=0;i<g_PathCount;i++)
    {
        nav_connection_slot_id iNextConnection;
        if (i < (g_PathCount-1))
            iNextConnection = g_pPathList[i+1];
        else
            iNextConnection = NULL_NAV_SLOT;

        rPathStruct.m_StepData[i].m_CurrentConnection = g_pPathList[i];
        rPathStruct.m_StepData[i].m_DestConnection    = g_pPathList[i+1];
        
        nav_node_slot_id iOverlap = NULL_NAV_SLOT;
        g_NavMap.DoOverlap( g_pPathList[i], iNextConnection, &iOverlap );

        rPathStruct.m_StepData[i].m_NodeToPassThrough = iOverlap;
    }
    
    return true;
}
*/

//===========================================================================
//
//  Retreat uses 2 planes.
//  1) Reference plane
//  2) Termination plane
//
//  The reference plane is a plane through the current position with a normal
//  pointing toward the threat.  This plane can be used to examine nodes
//  from the current connection and determine if they are toward or away from
//  the threat.                                 
//
//  The termination plane is locked down the very first time that we have
//  nodes on the back side of the reference plane.  Once the termination
//  plane is locked down, the retreat pathfinding will be forced to stop
//  if it has no option but to cross the plane.
//  
//  For each step of retreat:
//  1) Evaluate and score all nodes leading out of the connection
//     Ignoring the current node (1st cycle doesn't have a current node)
//
//  2) If there are nodes on the back side of the reference plane,
//     choose the best of those.  Otherwise choose the best front side
//     node.
//
//  3) Lather, rinse, repeat until a termination condition is met
//  
//  Score for nodes is a combination of distance away from the current
//  node, and the number of nodes in the remote connection (remote
//  connection is the connection on the other side of a node - the one
//  that the npc would be travelling into should it choose that node)
//
//  When the connection being evaluated also contains the threat,
//  scores are multiplied by a scalar.  The scalar is some relative
//  measure of the distance the threat is from the line segment
//  defined by the current node and the node being evaluated.
//  Greate distance from the threat is favoured.
//

/*
bool God::RequestRetreatPath( Object*                  pRequestObject, 
                                 const Vector3&         vRetreatFrom, 
                                 float                    DesiredMinDistance,
                                 path_find_struct&      rPathStruct, 
                                 int                    nMaxEdgeListSize,
                                 const pathing_hints*   pPathingHints )
{
    (void)nMaxEdgeListSize;

    pathing_hints   DefaultHints;
    if ( NULL == pPathingHints )
        pPathingHints = &DefaultHints;

    rPathStruct.Clear();


    Vector3 vStartPos = pRequestObject->GetPosition();
    nav_connection_slot_id StartConnectionSlot      = g_NavMap.GetNearestConnection( vStartPos );
    //ng_connection2&        StartConnection          = g_NavMap.GetConnectionByID( StartConnectionSlot );

    //nav_connection_slot_id RetreatConnectionSlot    = g_NavMap.GetNearestConnection( vRetreatFrom );

    int i;

    //
    //  2 Scores are kept.
    //
    //  [0] is the back halfspace defined by the partitioning plane
    //      that lies perpendicular to the direction vector from the
    //      requesting Object -> the point to retreat from.
    //  [1] is in the front halfspace.
    //
    //  We want to use the best in the back halfspace, if there is on.
    //  If there isn't, then we use the best in the front halfspace
    //

    nav_connection_slot_id      CurrentConnectionSlot   = NULL_NAV_SLOT;
    nav_node_slot_id            CurrentNodeSlot         = NULL_NAV_SLOT;
    Vector3                     vCurrentPos             = vStartPos;
    bool                       bDonePathing            = false;
    plane                       TerminationPlane;
    plane                       ReferencePlane;
    float                         DistanceTravelled       = 0;
    
    Vector3                     vToThreat = vRetreatFrom - vStartPos;
    vToThreat.GetY() = 0;
    vToThreat.Normalize();
    
    ReferencePlane.Setup( vStartPos, vToThreat );
    TerminationPlane = ReferencePlane;

    CurrentConnectionSlot = StartConnectionSlot;

    if (NULL_NAV_SLOT == CurrentConnectionSlot)
        return false;

    //
    //  Setup the pathfindstruct
    //
    rPathStruct.m_nSteps = 0;

    //
    //  Loop until we're done
    //  or we discover a special case
    //
    bool   bNoRetreatAvailable = false;

    while (!bDonePathing)
    {
        //
        //  2 Scores are kept.
        //
        //  [0] is the back halfspace defined by the partitioning plane
        //      that lies perpendicular to the direction vector from the
        //      requesting Object -> the point to retreat from.
        //  [1] is in the front halfspace.
        //
        //  For now, we are using the NoRetreatAvailable special condition
        //  to short circuit out of the retreat pathing.
        //  
        //  With this special case in place, we are guaranteed that we will
        //  always have back halfspace nodes available, so technically
        //  we dont' need to track the front halfspace nodes.
        //
        //  I'm leaving it in for now, just in case the NoRetreatAvailable
        //  case ever goes away.
        //

        int     iBest[2];
        float     BestScore[2];   

        iBest[0] = -1;
        iBest[1] = -1;

        BestScore[0] = -1;
        BestScore[1] = -1;

        ng_connection2& CurrentConnection = g_NavMap.GetConnectionByID( CurrentConnectionSlot );

        Vector3 vToThreat = vRetreatFrom - vCurrentPos;
        vToThreat.Normalize();
        ReferencePlane.Setup( vCurrentPos, vToThreat );

        // Evaluate all nodes of this connection
        for (i=0;i<CurrentConnection.GetOverlapCount();i++)
        {
            nav_node_slot_id        iOverlap          = CurrentConnection.GetOverlapNodeID( i );
            nav_connection_slot_id  iRemoteConnection = CurrentConnection.GetOverlapRemoteConnectionID( i );

            if (iOverlap == NULL_NAV_SLOT)
                continue;
            if (iOverlap == CurrentNodeSlot)
                continue;

            ng_node2&       Node           = g_NavMap.GetNodeByID      ( iOverlap          );
            ng_connection2& RemoteConn     = g_NavMap.GetConnectionByID( iRemoteConnection );
            int             nRemoteOptions = RemoteConn.GetOverlapCount() - 1;

            Vector3 Pos = Node.GetPosition();

            int iSlot = 0;              
            if (ReferencePlane.InFront( Pos ))
            {
                iSlot = 1;
            }
                
            Vector3 Delta      = Pos - vCurrentPos;
            float     LenSquared = Delta.LengthSquared();
        
            float     Score = LenSquared * (nRemoteOptions+1);       
        
            if (Score > BestScore[ iSlot ])
            {
                BestScore[ iSlot ] = Score;
                iBest    [ iSlot ] = i;
            }           
        }

        //  We have a special case if, on the very first attempt we 
        //  discover no nodes on the back size of the ref plane.
        //  In this case, we just retreat to the edge of the connection
        if ((rPathStruct.m_nSteps == 0) && ( iBest[0] == -1))
        {
            bDonePathing        = true;
            bNoRetreatAvailable = true;
            break;
        }

        int    iRetreatTo = -1;

        if (iBest[0] != -1)
            iRetreatTo = iBest[0];
        else 
        if (iBest[1] != -1)
            iRetreatTo = iBest[1];

        // If we have nothing, we're done
        if (iRetreatTo == -1)
            bDonePathing = true;
        else if (iBest[0] == -1)
            bDonePathing = true;
        else
        {
            // Otherwise, we retreat to overlap #(iRetreat)
            nav_node_slot_id        iOverlap          = CurrentConnection.GetOverlapNodeID( iRetreatTo );
            nav_connection_slot_id  iRemoteConnection = CurrentConnection.GetOverlapRemoteConnectionID( iRetreatTo );
            ng_node2&               Node              = g_NavMap.GetNodeByID( iOverlap );
    
            int iCurStep = rPathStruct.m_nSteps++;

            rPathStruct.m_vStartPoint               = vStartPos;
            rPathStruct.m_vEndPoint                 = Node.GetPosition();
            rPathStruct.m_bStartPointOnConnection   = true;
            rPathStruct.m_bEndPointOnConnection     = true;
            rPathStruct.m_StepData[iCurStep].m_CurrentConnection   = CurrentConnectionSlot;
            rPathStruct.m_StepData[iCurStep].m_DestConnection      = iRemoteConnection;
            rPathStruct.m_StepData[iCurStep].m_NodeToPassThrough   = iOverlap;

            rPathStruct.m_StepData[iCurStep+1].m_CurrentConnection   = iRemoteConnection;
            rPathStruct.m_StepData[iCurStep+1].m_DestConnection      = NULL_NAV_SLOT;
            rPathStruct.m_StepData[iCurStep+1].m_NodeToPassThrough   = NULL_NAV_SLOT;


            DistanceTravelled += (vCurrentPos - rPathStruct.m_vEndPoint).Length();

            vCurrentPos = rPathStruct.m_vEndPoint;

            CurrentConnectionSlot = iRemoteConnection;
            //CurrentConnection = g_NavMap.GetConnectionByID( CurrentConnectionSlot );

            CurrentNodeSlot = iOverlap;
        }

        if ( DistanceTravelled >= DesiredMinDistance)
            bDonePathing = true;

    }// while done pathing
    

    if (bNoRetreatAvailable)
        return false;

    return true;
}
*/
                                 