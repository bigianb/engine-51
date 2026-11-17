#include "GlobalVariablesManager.h"
#include "objectManager/ObjectManager.h"
#include "xfiles/xfs.h"

static constexpr int MAX_VARIABLE_NAME_LENGTH = 255;

var_mngr g_VarMgr;

//=========================================================================

var_mngr::var_mngr()
    : m_VarKeys(nullptr)
    , m_TimerKeys(nullptr)
    , m_GuidKeys(nullptr)
    , m_StoreVarKeys(nullptr)
    , m_StoreGuidKeys(nullptr)
    , m_RuntimeVarKeys(nullptr)
    , m_RuntimeGuidKeys(nullptr)
{
}

//=========================================================================

void var_mngr::var_key::Copy( const var_key& rSource )
{
    m_Name =rSource.m_Name;

    m_Type = rSource.m_Type;                 
    m_Data = rSource.m_Data;                 
}

//=========================================================================

void var_mngr::guid_key::Copy( const guid_key& rSource )
{
    m_Name =rSource.m_Name;
    m_Guid = rSource.m_Guid;                 
}

//=========================================================================

void var_mngr::timer_key::Copy( const timer_key& rSource )
{
    m_Name =rSource.m_Name;
    m_Timer = rSource.m_Timer;                 
}

//=========================================================================

xhandle var_mngr::RegisterFloat ( const char* pName )
{
    return RegisterVariable( TYPE_FLOAT, pName );
}

//=========================================================================

xhandle var_mngr::RegisterInt ( const char* pName )
{
    return RegisterVariable( TYPE_INT, pName );
}

//=========================================================================

xhandle var_mngr::RegisterBool ( const char* pName )
{
    return RegisterVariable( TYPE_BOOL, pName );
}

//=========================================================================

xhandle var_mngr::RegisterVariable    ( variable_types Type, const char* pName )
{
    xhandle new_handle;

    var_key &new_key = m_VarKeys.Add( new_handle );
    
    assert( new_handle.IsNonNull() );
    
    float     f_value = 0.0f;
    int     s_value = 0;
    bool   b_value = false;
    
    new_key.m_Name = pName;
    new_key.m_Type = Type;
    
    switch ( Type )
    {
    case TYPE_FLOAT:
        new_key.m_Data = *((uint32_t*)(&f_value));
        break;
    case TYPE_INT:
        new_key.m_Data = *((uint32_t*)(&s_value));
        break;
    case TYPE_BOOL:
        new_key.m_Data = *((uint32_t*)(&b_value));
        break;
    default:
        assert(0);  //Cannot be of indeterminite type..
        break;
    }
    
    return new_handle;
}

//=========================================================================

xhandle var_mngr::RegisterTimer( const char* pName )
{
    xhandle new_handle;

    timer_key &new_key = m_TimerKeys.Add( new_handle );
    
    assert( new_handle.IsNonNull() );
  
    new_key.m_Name = pName;

    return new_handle;
}

//=========================================================================

xhandle var_mngr::RegisterGuid( const char* pName )
{


    xhandle new_handle;
    


    guid_key &new_key = m_GuidKeys.Add( new_handle );
    
    assert( new_handle.IsNonNull() );
  
    new_key.m_Name = pName;

    return new_handle;
}

//=========================================================================

bool var_mngr::DestroyVariable ( xhandle& rVarHandle )
{


    assert( rVarHandle.IsNonNull() );
    
    m_VarKeys.DeleteByHandle ( rVarHandle );
    
    return true;
}

//=========================================================================

bool var_mngr::DestroyGuid ( xhandle& rGuidHandle )
{


    assert( rGuidHandle.IsNonNull() );
    
    m_GuidKeys.DeleteByHandle ( rGuidHandle );
    
    return true;
}

//=========================================================================

bool var_mngr::DestroyTimer ( xhandle& rTimerHandle )
{
  

    assert( rTimerHandle.IsNonNull() );
    
    m_TimerKeys.DeleteByHandle ( rTimerHandle );
    
    return true;
}

//=========================================================================

float var_mngr::GetFloat ( xhandle& rVarHandle )
{


    assert( rVarHandle.IsNonNull() );
    
    var_key &key = m_VarKeys( rVarHandle );
    return *((float*) &key.m_Data);   
}

//=========================================================================

int var_mngr::GetInt ( xhandle& rVarHandle )
{


    assert( rVarHandle.IsNonNull() );
    
    var_key &key = m_VarKeys( rVarHandle );
    return *((int*) &key.m_Data);   
}

//=========================================================================

bool var_mngr::GetBool ( xhandle& rVarHandle )
{


    assert( rVarHandle.IsNonNull() );
    
    var_key &key = m_VarKeys( rVarHandle );
    return *((bool*) &key.m_Data);   
}

//=========================================================================

bool var_mngr::GetVarHandle ( const char* pName, xhandle* rVarHandle )
{


    assert( pName != NULL );
    
    int count = m_VarKeys.GetCount();
    
    for (int i = 0; i < count; i++)
    {
        var_key &key = m_VarKeys[i];
        
        if ( key.m_Name.compare ( pName ) == 0 )
        {
            *rVarHandle = m_VarKeys.GetHandleByIndex(i);
            return true;
        }
    }
    
    return false;
}

//=========================================================================

bool var_mngr::GetTimerHandle ( const char* pName, xhandle* rVarHandle )
{


    assert( pName != NULL );
    
    int count = m_TimerKeys.GetCount();
    
    for (int i = 0; i < count; i++)
    {
        timer_key &key = m_TimerKeys[i];
        
        if ( key.m_Name.compare ( pName ) == 0 )
        {
            *rVarHandle = m_TimerKeys.GetHandleByIndex(i);
            return true;
        }
    }
    
    return false;
}

//=========================================================================

bool var_mngr::GetGuidHandle ( const char* pName, xhandle* rVarHandle )
{


    assert( pName != NULL );
    
    int count = m_GuidKeys.GetCount();
    
    for (int i = 0; i < count; i++)
    {
        guid_key &key = m_GuidKeys[i];
        
        if ( key.m_Name.compare ( pName ) == 0 )
        {
            *rVarHandle = m_GuidKeys.GetHandleByIndex(i);
            return true;
        }
    }
    
    return false;
}

//=========================================================================

var_mngr::variable_types  var_mngr::GetType( xhandle& rVarHandle )
{


    assert( rVarHandle.IsNonNull() );

    if ( rVarHandle.IsNull() )
        return TYPE_NULL;

    var_key &key = m_VarKeys( rVarHandle );
    
    return (variable_types) key.m_Type;
}

//=========================================================================

void var_mngr::SetFloat ( xhandle& rVarHandle, float Value )
{
 
    assert( rVarHandle.IsNonNull() );
    
    if ( rVarHandle.IsNull() )
        return;

    var_key &key = m_VarKeys( rVarHandle );
    key.m_Data   = *((uint32_t*)&Value);
}

//=========================================================================

void var_mngr::SetInt ( xhandle& rVarHandle, int Value  )
{

    assert( rVarHandle.IsNonNull() );
     
    if ( rVarHandle.IsNull() )
        return;
    
    var_key &key = m_VarKeys( rVarHandle );
    key.m_Data   = *((uint32_t*)&Value);
}

//=========================================================================

void var_mngr::SetBool ( xhandle& rVarHandle, bool Value  )
{
    
    assert( rVarHandle.IsNonNull() );
    
    if ( rVarHandle.IsNull() )
        return;

    var_key &key = m_VarKeys( rVarHandle );
    key.m_Data   = *((uint32_t*)&Value);
}  

//=========================================================================

void var_mngr::SetGuid ( xhandle& rGuidHandle, guid Value  )
{
   
    assert( rGuidHandle.IsNonNull() );
    
    if ( rGuidHandle.IsNull() )
        return;

    guid_key &key = m_GuidKeys( rGuidHandle );
    key.m_Guid    = Value;
}  

//=========================================================================

guid var_mngr::GetGuid ( xhandle& rGuidHandle )
{

    assert( rGuidHandle.IsNonNull() );
    
    guid_key &key = m_GuidKeys( rGuidHandle );
    return key.m_Guid;   
}

//=========================================================================

void var_mngr::ResetTimer( xhandle& rVarHandle )
{
    
    assert( rVarHandle.IsNonNull() );

    if ( rVarHandle.IsNull() )
        return;

    timer_key &key = m_TimerKeys( rVarHandle );
    key.m_Timer.Reset();
}

//=========================================================================

void var_mngr::StartTimer( xhandle& rVarHandle )
{
   
    assert( rVarHandle.IsNonNull() );
    
    if ( rVarHandle.IsNull() )
        return;

    timer_key &key = m_TimerKeys( rVarHandle );
    key.m_Timer.Start();
}

//=========================================================================

void var_mngr::StopTimer ( xhandle& rVarHandle )
{
    
    assert( rVarHandle.IsNonNull() );
    
    if ( rVarHandle.IsNull() )
        return;

    timer_key &key = m_TimerKeys( rVarHandle );
    key.m_Timer.Stop();
}

//=========================================================================

float var_mngr::ReadTimer ( xhandle& rVarHandle )
{
    
    assert( rVarHandle.IsNonNull() );
    
    if ( rVarHandle.IsNull() )
        return 0.0f;

    timer_key &key = m_TimerKeys( rVarHandle );

    return key.m_Timer.ReadSec();
}

//=========================================================================

void var_mngr::OnEnumProp ( prop_enum&   rPropList )
{
    int iHeader = rPropList.PushPath( "Global Variables\\Variables\\" );        
    
    OnEnumPropInternal( rPropList );

    rPropList.PopPath( iHeader );
}

//=========================================================================

bool var_mngr::OnProperty ( prop_query&  rPropQuery )
{
    int iHeader = rPropQuery.PushPath( "Global Variables\\Variables\\" );        
    
    if (OnPropertyInternal(rPropQuery))
    {
        rPropQuery.PopPath( iHeader );
        return true;
    }
    
    rPropQuery.PopPath( iHeader );
    return false;
}

//=========================================================================

void var_mngr::OnEnumPropVariables ( prop_enum&   rPropList, uint32_t Flags )
{
    int count = m_VarKeys.GetCount();
    int i = 0;

    for ( i = 0; i < count; i++)
    {
        var_key &key = m_VarKeys[i];
        
        switch (key.m_Type)
        {
        case TYPE_FLOAT:
            {
                rPropList.PropEnumString( xfs("Variable[%d]", i) ,       "A single entry..", Flags );        
                rPropList.PropEnumFloat ( xfs("Variable[%d]\\Float", i), "Floating point value.", Flags );
                rPropList.PropEnumButton( xfs("Variable[%d]\\Delete",i), "Delete this Global Variable.", Flags | PROP_TYPE_MUST_ENUM );
            }
            break;
            
        case TYPE_INT:
            {
                rPropList.PropEnumString( xfs("Variable[%d]", i) ,       "A single entry..", Flags );        
                rPropList.PropEnumInt   ( xfs("Variable[%d]\\Int", i) ,  "Interger point value.", Flags );
                rPropList.PropEnumButton( xfs("Variable[%d]\\Int",i) ,   "Delete this Global Variable.", Flags | PROP_TYPE_MUST_ENUM );
            }
            break;
            
        case TYPE_BOOL:
            {
                rPropList.PropEnumString( xfs("Variable[%d]", i) ,       "A single entry..", Flags );        
                rPropList.PropEnumBool  ( xfs("Variable[%d]\\Bool", i) , "Boolean point value.", Flags );
                rPropList.PropEnumButton( xfs("Variable[%d]\\Bool",i) ,  "Delete this Global Variable.", Flags | PROP_TYPE_MUST_ENUM );
            }
            break;
            
        default:
            assert(0);  //Cannot be of indeterminite type..
            break;
        }
    }

    //Enumerate the timers...
    
    count = m_TimerKeys.GetCount();
    i = 0;
    
    for ( i = 0; i < count; i++)
    {
        rPropList.PropEnumString( xfs("Timer[%d]", i) ,          "A single entry..", Flags );  
        rPropList.PropEnumFloat ( xfs("Timer[%d]\\Value", i) ,   "Current time value in secs.", Flags );
        rPropList.PropEnumButton( xfs("Timer[%d]\\Delete",i) ,   "Delete this Global Variable.", Flags | PROP_TYPE_MUST_ENUM );
    }

    //Enumerate the guids...
    
    count = m_GuidKeys.GetCount();
    i = 0;
    
    for ( i = 0; i < count; i++)
    {
        rPropList.PropEnumString( xfs("Guid[%d]", i) ,          "A single entry..", Flags );  
        rPropList.PropEnumGuid  ( xfs("Guid[%d]\\Value", i) ,   "Guid Value.", Flags );
        rPropList.PropEnumButton( xfs("Guid[%d]\\Delete",i) ,   "Delete this Global Variable.", Flags | PROP_TYPE_MUST_ENUM );
    }
}

//=========================================================================

void var_mngr::OnEnumPropInternal	( prop_enum&   rPropList )
{
    rPropList.PropEnumInt( "Array\\Size", "", 0 );    

    int count = m_VarKeys.GetCount();
    int i = 0;

    for ( i = 0; i < count; i++ )
    {
        rPropList.PropEnumInt( xfs("Array\\Type[%d]", i) , "", 0 );    
    }

    rPropList.PropEnumInt( "Array\\Timer Size", "", 0 );

    rPropList.PropEnumInt( "Array\\Guid Size", "", 0 );

    OnEnumPropVariables(rPropList);
}

//=========================================================================

bool var_mngr::OnPropertyInternal  ( prop_query&  rPropQuery )
{
    if (rPropQuery.IsVar( "Array\\Size" ))
    {
        if (rPropQuery.IsRead())
        {
            int Size = m_VarKeys.GetCount();
            
            rPropQuery.SetVarInt(Size);
        }
        else
        {
            int Size = rPropQuery.GetVarInt();
            
            for (int i = 0; i < Size; i++)
            {
                xhandle new_handle;
                var_key &EmptyKey  = m_VarKeys.Add( new_handle );
				(void)EmptyKey;
            }
        }
        
        return true;
    }
    
    if (rPropQuery.IsVar( "Array\\Type[]" ))
    {
        int iIndex = rPropQuery.GetIndex(0);
        
        assert ( iIndex < m_VarKeys.GetCount() );
        
        var_key &key = m_VarKeys[iIndex];
        
        if (rPropQuery.IsRead())
        {
            rPropQuery.SetVarInt((int)key.m_Type);
        }
        else
        {
            key.m_Type = (variable_types)rPropQuery.GetVarInt();
        }
        
        return true;
    }
    
    if (rPropQuery.IsVar( "Array\\Timer Size" ))
    {
        if (rPropQuery.IsRead())
        {
            int Size = m_TimerKeys.GetCount();
            
            rPropQuery.SetVarInt(Size);
        }
        else
        {
            int Size = rPropQuery.GetVarInt();
            
            for (int i = 0; i < Size; i++)
            {
                xhandle new_handle;
                timer_key &EmptyKey  = m_TimerKeys.Add( new_handle );
                (void)EmptyKey;
            }
        }
        
        return true;
    }

    
    if (rPropQuery.IsVar( "Array\\Guid Size" ))
    {
        if (rPropQuery.IsRead())
        {
            int Size = m_GuidKeys.GetCount();
            
            rPropQuery.SetVarInt(Size);
        }
        else
        {
            int Size = rPropQuery.GetVarInt();
            
            for (int i = 0; i < Size; i++)
            {
                xhandle new_handle;
                guid_key &EmptyKey  = m_GuidKeys.Add( new_handle );
                (void)EmptyKey;
            }
        }
        
        return true;
    }

    if (OnPropertyVariables(rPropQuery))
    {
        return true;
    }

    return false;
}

//=========================================================================

bool var_mngr::OnPropertyVariables	( prop_query&  rPropQuery )
{ 
    int iIndex = rPropQuery.GetIndex(0);
    
    if ( rPropQuery.IsSimilarPath("Variable[]") )
    {
        if ( iIndex >= m_VarKeys.GetCount() )
            return false;
        
        var_key &key = m_VarKeys[iIndex];
        
        if ( rPropQuery.IsVar( "Variable[]" ) )
        { 
            if( rPropQuery.IsRead() )
            {
                rPropQuery.SetVarString( key.m_Name.c_str(), MAX_VARIABLE_NAME_LENGTH );
            }
            else
            {
                key.m_Name = rPropQuery.GetVarString();
            }
            
            return true;
        }
        
        if ( rPropQuery.VarFloat( "Variable[]\\Float" ,     *((float*) &key.m_Data) ) )
        {
            return true;
        }
        
        if ( rPropQuery.VarInt( "Variable[]\\Int" ,         *((int*) &key.m_Data) ) )
        {
            return true;
        }
        
        if ( rPropQuery.VarBool( "Variable[]\\Bool" ,       *((bool*) &key.m_Data) ) )
        {
            return true;
        }

        if( rPropQuery.IsVar( "Variable[]\\Delete" ) )
        {
            if( rPropQuery.IsRead() )
            {
                rPropQuery.SetVarButton( "Delete" );
            }
            else
            {
                xhandle rHandle;
                if (GetVarHandle(key.m_Name.c_str(), &rHandle))
                {
                    m_VarKeys.DeleteByHandle ( rHandle );
                }
            }
            return true;
        }
    }
    
    if ( rPropQuery.IsSimilarPath("Timer[]") )
    {
        if ( iIndex >= m_TimerKeys.GetCount() )
            return false;
        
        timer_key &key = m_TimerKeys[iIndex]; 
        
        if ( rPropQuery.IsVar( "Timer[]" ) )
        { 
            if( rPropQuery.IsRead() )
            {
                rPropQuery.SetVarString( key.m_Name.c_str(), MAX_VARIABLE_NAME_LENGTH );
            }
            else
            {
                key.m_Name = rPropQuery.GetVarString();
            }
            
            return true;
        }
        
        float CurrentTimeValue = key.m_Timer.ReadSec();

        if ( rPropQuery.VarFloat( "Timer[]\\Value" , CurrentTimeValue ) )
        {
            return true;
        }

        if( rPropQuery.IsVar( "Timer[]\\Delete" ) )
        {
            if( rPropQuery.IsRead() )
            {
                rPropQuery.SetVarButton( "Delete" );
            }
            else
            {
                xhandle rHandle;
                if (GetTimerHandle(key.m_Name.c_str(), &rHandle))
                {
                    m_TimerKeys.DeleteByHandle ( rHandle );
                }
            }
            return true;
        }
    }

    if ( rPropQuery.IsSimilarPath("Guid[]") )
    {
        if ( iIndex >= m_GuidKeys.GetCount() )
            return false;
        
        guid_key &key = m_GuidKeys[iIndex]; 
        
        if ( rPropQuery.IsVar( "Guid[]" ) )
        { 
            if( rPropQuery.IsRead() )
            {
                rPropQuery.SetVarString( key.m_Name.c_str(), MAX_VARIABLE_NAME_LENGTH );
            }
            else
            {
                key.m_Name = rPropQuery.GetVarString();
            }
            
            return true;
        }
        
        if ( rPropQuery.VarGUID( "Guid[]\\Value" , key.m_Guid ) )
        {
            return true;
        }

        if( rPropQuery.IsVar( "Guid[]\\Delete" ) )
        {
            if( rPropQuery.IsRead() )
            {
                rPropQuery.SetVarButton( "Delete" );
            }
            else
            {
                xhandle rHandle;
                if (GetGuidHandle(key.m_Name.c_str(), &rHandle))
                {
                    m_GuidKeys.DeleteByHandle ( rHandle );
                }
            }
            return true;
        }
    }
    return false;
}

//=========================================================================

void var_mngr::ResetKeys( void )
{
    m_VarKeys.Clear();
    m_TimerKeys.Clear();
    m_GuidKeys.Clear();
    m_StoreVarKeys.Clear();
    m_StoreGuidKeys.Clear();
}

//=========================================================================

void var_mngr::ClearData( void )
{
    ResetKeys();
    m_RuntimeGuidKeys.Clear();
    m_RuntimeVarKeys.Clear();
}

//=========================================================================

void var_mngr::ResetAllTimers ( void )
{
    int count = m_TimerKeys.GetCount();
    
    for (int i = 0; i < count; i++)
    { 
        timer_key &Key        = m_TimerKeys[i];
        Key.m_Timer.Reset();
    }
}

//=========================================================================
/*
void var_mngr::LoadGlobals ( const char* pFileName )
{
    ResetKeys();
    if (SMP_UTIL_FileExist( pFileName ))
    {
        text_in GlobalFile;
        
        x_try;
        
        GlobalFile.OpenFile( pFileName );

        OnLoad( GlobalFile );
        
        GlobalFile.CloseFile();

        ResetAllTimers();
        
        x_catch_begin;
        
        x_catch_end;
    }
}

//=========================================================================

void var_mngr::ImportGlobals ( const char* pFileName )
{
    //first backup the current globals
    int i = 0;

    xharray<var_key> VarKeysBackup;
    for (i = 0; i < m_VarKeys.GetCount(); i++)
    {
        xhandle new_handle;
        var_key &Key        = m_VarKeys[i];
        var_key &CopyKey    = VarKeysBackup.Add( new_handle );
        CopyKey.Copy(Key);
    }

    xharray<guid_key> GuidKeysBackup;
    for (i = 0; i < m_GuidKeys.GetCount(); i++)
    {
        xhandle new_handle;
        guid_key &Key        = m_GuidKeys[i];
        guid_key &CopyKey    = GuidKeysBackup.Add( new_handle );
        CopyKey.Copy(Key);
    }

    xharray<timer_key> TimerKeysBackup;
    for (i = 0; i < m_TimerKeys.GetCount(); i++)
    {
        xhandle new_handle;
        timer_key &Key        = m_TimerKeys[i];
        timer_key &CopyKey    = TimerKeysBackup.Add( new_handle );
        CopyKey.Copy(Key);
    }
    
    //now load new globals
    LoadGlobals(pFileName);

    //next re-append our original globals
    for (i = 0; i < VarKeysBackup.GetCount(); i++)
    {
        xhandle new_handle;
        var_key &Key        = VarKeysBackup[i];
        var_key &CopyKey    = m_VarKeys.Add( new_handle );
        CopyKey.Copy(Key);
    }

    for (i = 0; i < GuidKeysBackup.GetCount(); i++)
    {
        xhandle new_handle;
        guid_key &Key        = GuidKeysBackup[i];
        guid_key &CopyKey    = m_GuidKeys.Add( new_handle );
        CopyKey.Copy(Key);
    }

    for (i = 0; i < TimerKeysBackup.GetCount(); i++)
    {
        xhandle new_handle;
        timer_key &Key        = TimerKeysBackup[i];
        timer_key &CopyKey    = m_TimerKeys.Add( new_handle );
        CopyKey.Copy(Key);
    }
}
*/

void var_mngr::StoreState ( void )
{
    int i = 0;
    m_StoreVarKeys.Clear();
    int count = m_VarKeys.GetCount();
    for (i = 0; i < count; i++)
    {
        xhandle new_handle;
        var_key &Key        = m_VarKeys[i];
        var_key &CopyKey    = m_StoreVarKeys.Add( new_handle );

        CopyKey.Copy(Key);
    }

    m_StoreGuidKeys.Clear();
    count = m_GuidKeys.GetCount();
    for (i = 0; i < count; i++)
    {
        xhandle new_handle;
        guid_key &Key        = m_GuidKeys[i];
        guid_key &CopyKey    = m_StoreGuidKeys.Add( new_handle );

        CopyKey.Copy(Key);
    }
}

//=========================================================================

void var_mngr::ResetState ( void )
{
    int i = 0;
    m_VarKeys.Clear();
    int count = m_StoreVarKeys.GetCount();
    for (i = 0; i < count; i++)
    { 
        xhandle new_handle;
        var_key &Key        = m_StoreVarKeys[i];
        var_key &CopyKey    = m_VarKeys.Add( new_handle );

        CopyKey.Copy(Key);
    }

    m_GuidKeys.Clear();
    count = m_StoreGuidKeys.GetCount();
    for (i = 0; i < count; i++)
    { 
        xhandle new_handle;
        guid_key &Key        = m_StoreGuidKeys[i];
        guid_key &CopyKey    = m_GuidKeys.Add( new_handle );

        CopyKey.Copy(Key);
    }
    
    ResetAllTimers();
}

//=========================================================================
/*
bool var_mngr::SaveRuntimeData( void )
{
    int i = 0;
    m_RuntimeVarKeys.Clear();
    int count = m_VarKeys.GetCount();
    for (i = 0; i < count; i++)
    {
        xhandle new_handle;
        var_key &Key        = m_VarKeys[i];
        var_key &CopyKey    = m_RuntimeVarKeys.Add( new_handle );

        CopyKey.Copy(Key);
    }

    m_RuntimeGuidKeys.Clear();
    count = m_GuidKeys.GetCount();
    for (i = 0; i < count; i++)
    {
        xhandle new_handle;
        guid_key &Key        = m_GuidKeys[i];
        guid_key &CopyKey    = m_RuntimeGuidKeys.Add( new_handle );

        CopyKey.Copy(Key);
    }

    return true;
}

//=========================================================================

bool var_mngr::LoadRuntimeData( void )
{
    int i = 0;
    m_VarKeys.Clear();
    int count = m_RuntimeVarKeys.GetCount();
    for (i = 0; i < count; i++)
    { 
        xhandle new_handle;
        var_key &Key        = m_RuntimeVarKeys[i];
        var_key &CopyKey    = m_VarKeys.Add( new_handle );

        CopyKey.Copy(Key);
    }

    m_GuidKeys.Clear();
    count = m_RuntimeGuidKeys.GetCount();
    for (i = 0; i < count; i++)
    { 
        xhandle new_handle;
        guid_key &Key        = m_RuntimeGuidKeys[i];
        guid_key &CopyKey    = m_GuidKeys.Add( new_handle );

        CopyKey.Copy(Key);
    }
    
    ResetAllTimers();

    return true;
}

//=========================================================================

void var_mngr::StoreRuntimeData( bitstream& BitStream )
{
    // write the variable count
    int Count = m_VarKeys.GetCount();
    BitStream.WriteS32( Count );

    // write the variable values
    for ( int i = 0; i < Count; i++ )
    {
        BitStream.WriteU32( m_VarKeys[i].m_Data );
    }

    // write the guid key count 
    Count = m_GuidKeys.GetCount();
    BitStream.WriteS32( Count );

    // write guid values
    for (int j = 0; j < Count; j++)
    {
        BitStream.WriteU64( m_GuidKeys[j].m_Guid );
    }
}

//=========================================================================

void var_mngr::RestoreRuntimeData( bitstream& BitStream )
{
    // read the variable count
    int Count;
    BitStream.ReadS32( Count );

    if( m_VarKeys.GetCount() != Count )
    {
        ASSERTS( false, "Global variable count does not match save data!" );
        x_DebugMsg( 0, "Global variable count does not match save data!/n" );

        // Don't restore the globals!
        BitStream.SetCursor( BitStream.GetCursor() + ( Count * 8 ) );
    }
    else
    {
        // read the variable values
        for ( int i = 0; i < Count; i++ )
        {
            BitStream.ReadU32( m_VarKeys[i].m_Data );
        }
    }

    // read the guid key count 
    BitStream.ReadS32( Count );

    if( m_GuidKeys.GetCount() != Count )
    {
        ASSERTS( false, "Guid count does not match save data!" );
        x_DebugMsg( 0, "Guid count does not match save data!/n" );

        // Don't restore the guids!
        BitStream.SetCursor( BitStream.GetCursor() + ( Count * 8 ) );
    }
    else
    {
        // read guid values
        for ( int i = 0; i < Count; i++ )
        {
            BitStream.ReadU64( m_GuidKeys[i].m_Guid.Guid );
        }
    }
}

//=========================================================================

void var_mngr::DumpRuntimeData( bitstream& BitStream )
{
    // output header info
    x_DebugMsg( 0, "------------------------\n" );
    x_DebugMsg( 0, "        Globals         \n" );
    x_DebugMsg( 0, "------------------------\n" );

    // read the variable count
    int Count;
    BitStream.ReadS32( Count );
    x_DebugMsg( 0, "Variable Count = %d\n", Count );

    if( m_VarKeys.GetCount() != Count )
    {
        x_DebugMsg( 0, "Global variable count does not match save data!/n" );

        // Don't restore the globals!
        BitStream.SetCursor( BitStream.GetCursor() + ( Count * 8 ) );
    }
    else
    {
        // read the variable values
        for ( int i = 0; i < Count; i++ )
        {           
            x_DebugMsg( 0, "------------------------\n" );
            x_DebugMsg( 0, "Variable Name : %s\n", m_VarKeys[i].m_Name );

            uint32_t Data;
            BitStream.ReadU32( Data );

            switch( m_VarKeys[i].m_Type )
            {
            case TYPE_NULL:
                x_DebugMsg( 0, "Variable Type : TYPE_NULL\n" );
                x_DebugMsg( 0, "Variable Data : %d\n", Data );
                break;
            case TYPE_FLOAT:
                x_DebugMsg( 0, "Variable Type : TYPE_FLOAT\n" );
                x_DebugMsg( 0, "Variable Data : %f\n", (float)Data );
                break;
            case TYPE_INT:
                x_DebugMsg( 0, "Variable Type : TYPE_INT\n" );
                x_DebugMsg( 0, "Variable Data : %d\n", Data );
                break;
            case TYPE_BOOL:
                x_DebugMsg( 0, "Variable Type : TYPE_BOOL\n" );
                x_DebugMsg( 0, "Variable Data : %d\n", Data );
                break;
            default:
                x_DebugMsg( 0, "Variable Type : UNDEFINED!\n" );
                x_DebugMsg( 0, "Variable Data : %d\n", Data );
                break;
            }

        }
    }

    x_DebugMsg( 0, "------------------------\n" );
    x_DebugMsg( 0, "         GUIDS          \n" );
    x_DebugMsg( 0, "------------------------\n" );

    // read the guid key count 
    BitStream.ReadS32( Count );
    x_DebugMsg( 0, "Guid Count = %d\n", Count );

    if( m_GuidKeys.GetCount() != Count )
    {
        x_DebugMsg( 0, "Guid count does not match save data!/n" );

        // Don't restore the guids!
        BitStream.SetCursor( BitStream.GetCursor() + ( Count * 8 ) );
    }
    else
    {
        // read guid values
        for ( int i = 0; i < Count; i++ )
        {
            u64 GuidSequence;
            BitStream.ReadU64( GuidSequence );
            x_DebugMsg( 0, "------------------------\n" );
            x_DebugMsg( 0, "Guid Name     : %s\n", m_GuidKeys[i].m_Name );
            x_DebugMsg( 0, "Guid Sequence : %d\n", GuidSequence );            
        }
    }
}
*/
