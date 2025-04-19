#pragma once

#include "../dataVault/DataVault.h"
#include "../VectorMath.h"

//==============================================================================
//  TWEAK_HANDLE
//==============================================================================

class tweak_handle : public data_handle
{
public:
                        tweak_handle    ();
                        tweak_handle    ( const char* pDataDescriptorName );
                       ~tweak_handle    ();
                       
    bool               Exists          () const;                       
    float                 GetF32          () const;
    int                 GetS32          () const;
    Radian              GetRadian       () const;
    bool               GetBool         () const;
};

//==============================================================================
// TWEAK_BLOCK
//==============================================================================

struct tweak_data_block : public data_block
{
                        tweak_data_block     ();
                       ~tweak_data_block     ();

    float                 GetValue        () const;
    void                SetValue        ( float Value );

public:

    float             m_Value;
};

//==============================================================================
// TWEAK_MGR
//==============================================================================

bool LoadTweaks    ( const char* pDirectory );
void  UnloadTweaks  ();


//==============================================================================
// GLOBAL TWEAK UTILITY FUNCTIONS
//==============================================================================

// NOTE: For applicable functions, if tweak does not exist, then "Default" is returned.

float     GetTweakF32     ( const char* pName );
float     GetTweakF32     ( const char* pName, float Default );

int     GetTweakS32     ( const char* pName );
int     GetTweakS32     ( const char* pName, int Default );

Radian  GetTweakRadian  ( const char* pName );
Radian  GetTweakRadian  ( const char* pName, Radian Default );

bool   GetTweakBool    ( const char* pName );
bool   GetTweakBool    ( const char* pName, bool Default );

