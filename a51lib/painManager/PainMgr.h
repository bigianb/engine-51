#pragma once

#include "../dataVault/DataVault.h"
#include "../PropertyEnum.h"
#include "../VectorMath.h"

class  pain_handle;
class  health_handle;
class  pain_health_handle;
struct pain_profile;
struct health_profile;
struct pain_health_profile;

enum generic_pain_type
{
    TYPE_GENERIC_1,
    TYPE_GENERIC_2,
    TYPE_GENERIC_3,
    TYPE_LETHAL,
    TYPE_EXPLOSIVE,
    TYPE_LASER,
    TYPE_ACID_1, 
    TYPE_ACID_2,
    TYPE_ACID_3, 

    TYPE_GOO_1,  
    TYPE_GOO_2,  
    TYPE_GOO_3,  

    TYPE_FIRE_1, 
    TYPE_FIRE_2, 
    TYPE_FIRE_3, 

    TYPE_DROWNING,
};

extern enum_table<generic_pain_type>  g_GenericPainTypeList;
pain_handle GetPainHandleForGenericPain( generic_pain_type Type );

class pain_handle : public data_handle
{
public:
                         pain_handle    ( const char* pPainDescriptor );
                         pain_handle    ( void );
                        ~pain_handle    ( void );

const pain_profile*   GetPainProfile ( void )const;
pain_health_handle    BuildPainHealthProfileHandle( const health_handle& Handle ) const;  
};

class health_handle : public data_handle
{
public:
            health_handle    ( const char* pHealthDescriptor );
            health_handle    ( void );
           ~health_handle    ( void );

const health_profile*   GetHealthProfile ( void ) const;
};

class pain_health_handle : public data_handle
{
public:
            pain_health_handle    ( const char* pPainHealthDescriptor );
            pain_health_handle    ( void );
           ~pain_health_handle    ( void );

const pain_health_profile*   GetPainHealthProfile ( void ) const;
};

struct pain_profile : public data_block
{
        pain_profile     ( void );
        ~pain_profile     ( void );

public:

    float     m_DamageNearDist;
    float     m_DamageFarDist;
    float     m_ForceNearDist;
    BBox    m_BBox;
    float     m_ForceFarDist;
    int16_t     m_iPainHealthTable;
    uint16_t     m_bSplash:1,
            m_bCheckLOS:1;
};

//==============================================================================

struct health_profile : public data_block
{
        health_profile     ( void );
        ~health_profile     ( void );

public:

    int16_t     m_iPainHealthTable;
};

//==============================================================================

struct pain_health_profile : public data_block
{
        pain_health_profile     ( void );
        ~pain_health_profile     ( void );

public:

    float     m_Damage;
    float     m_Force;
    int     m_HitType;
};

bool LoadPain    ( const char* pDirectory );
void  UnloadPain  ( void );

