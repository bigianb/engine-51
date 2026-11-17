#pragma once

#include "xfiles/xHandleArray.h"
#include "xfiles/xHandle.h"
#include "xfiles/x_time.h"
#include "Property.h"


class var_mngr : public prop_interface
{
public:

    var_mngr();

	enum global_types
	{
        GLOBAL_NULL = 0,

        GLOBAL_FLOAT,
        GLOBAL_INT,
        GLOBAL_BOOL,
        GLOBAL_TIMER,
        GLOBAL_GUID,

        GLOBAL_END
    };

	enum variable_types
	{
        TYPE_NULL  = 0 ,

        TYPE_FLOAT  ,
        TYPE_INT    ,
        TYPE_BOOL   ,

        VARIABLE_TYPES_END
	};

    xhandle                     RegisterFloat       ( const char* pName );
    xhandle                     RegisterInt         ( const char* pName );
    xhandle                     RegisterBool        ( const char* pName );
    xhandle                     RegisterTimer       ( const char* pName );
    xhandle                     RegisterGuid        ( const char* pName );

    bool                       DestroyVariable     ( xhandle& rVarHandle );
    bool                       DestroyGuid         ( xhandle& rGuidHandle );
    bool                       DestroyTimer        ( xhandle& rTimerHandle );

    float                         GetFloat            ( xhandle& rVarHandle );
    int                         GetInt              ( xhandle& rVarHandle );
    bool                       GetBool             ( xhandle& rVarHandle );

    bool                       GetVarHandle        ( const char* pName, xhandle* rVarHandle );
    bool                       GetTimerHandle      ( const char* pName, xhandle* rTimerHandle );
    bool                       GetGuidHandle       ( const char* pName, xhandle* rGuidHandle );

    variable_types              GetType             ( xhandle& rVarHandle );
    
    void                        SetFloat            ( xhandle& rVarHandle, float   Value  );
    void                        SetInt              ( xhandle& rVarHandle, int   Value  );
    void                        SetBool             ( xhandle& rVarHandle, bool Value  );

    guid                        GetGuid             ( xhandle& rGuidHandle );
    void                        SetGuid             ( xhandle& rGuidHandle, guid  Value  );
    
    void                        ResetTimer          ( xhandle& rVarHandle );
    void                        StartTimer          ( xhandle& rVarHandle );
    void                        StopTimer           ( xhandle& rVarHandle );
    float                         ReadTimer           ( xhandle& rVarHandle );
/*
    bool                       SaveRuntimeData     ( void );
    bool                       LoadRuntimeData     ( void );
    
    void                        StoreRuntimeData    ( bitstream& BitStream );
    void                        RestoreRuntimeData  ( bitstream& BitStream );
    void                        DumpRuntimeData     ( bitstream& BitStream );
*/
    void                        ClearData           ( void );

public:

    //Editor functionality for restorting states

    void                        LoadGlobals         ( const char* pFileName );

    void                        ImportGlobals       ( const char* pFileName );
    void                        StoreState          ( void );
    void                        ResetState          ( void );
    void                        ResetAllTimers      ( void );

public:
    
    //Property system implementation for saving and loading ...
    
    virtual			void	    OnEnumProp			( prop_enum&   rList );
    virtual			bool	    OnProperty			( prop_query&  rPropQuery );
          
    virtual			void	    OnEnumPropInternal	( prop_enum&   rList );
    virtual			bool	    OnPropertyInternal  ( prop_query&  rPropQuery );

    virtual			void	    OnEnumPropVariables ( prop_enum&   rList, uint32_t Flags = 0 );
    virtual			bool	    OnPropertyVariables	( prop_query&  rPropQuery );

protected:
        
    struct var_key 
    {
        void      Copy( const var_key& rSource );
       
        std::string  m_Name;                     // Name of variable..
        uint16_t         m_Type;                     // Type of the variable used to access the correct array
        uint32_t         m_Data;                     // Data itself
    };
 
    struct timer_key 
    {
        void      Copy( const timer_key& rSource );
       
        std::string  m_Name;                     // Name of variable..
        xtimer      m_Timer;                    // The timer associated with this variable.
    };
    
    struct guid_key 
    {
        void      Copy( const guid_key& rSource );
       
        std::string  m_Name;                     // Name of variable..
        guid        m_Guid;                     // The guid associated with this variable.
    };

protected:

    xhandle                     RegisterVariable    ( variable_types Type, const char* pName );
    void                        ResetKeys           ( void );
    
protected:

    xharray<var_key>        m_VarKeys;          // Variables Key Array..
    xharray<timer_key>      m_TimerKeys;        // Timer Key Array..
    xharray<guid_key>       m_GuidKeys;         // Guid Key Array..
    xharray<var_key>        m_StoreVarKeys;     // Variables Key Array for state restore..
    xharray<guid_key>       m_StoreGuidKeys;    // Variables Key Array for state restore..
    xharray<var_key>        m_RuntimeVarKeys;   // Variables Key Array for state restore..
    xharray<guid_key>       m_RuntimeGuidKeys;  // Variables Key Array for state restore..
};

extern var_mngr g_VarMgr;
