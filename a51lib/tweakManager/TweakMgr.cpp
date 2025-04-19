
#include "../tweakManager/TweakMgr.h"

//==============================================================================
//  DATA_HANDLE
//==============================================================================

tweak_handle::tweak_handle()
{
    data_handle::SetType(DATA_TYPE_TWEAK);
}

//==============================================================================

tweak_handle::~tweak_handle()
{
}

//==============================================================================

tweak_handle::tweak_handle(const char* pDataDescriptorName)
{
    data_handle::SetType(DATA_TYPE_TWEAK);
    data_handle::SetName(pDataDescriptorName);
}

//==============================================================================

bool tweak_handle::Exists() const
{
    tweak_data_block* pTweak = (tweak_data_block*)g_DataVault.GetData(*this);
    return (pTweak != NULL);
}

//==============================================================================

float tweak_handle::GetF32() const
{
    tweak_data_block* pTweak = (tweak_data_block*)g_DataVault.GetData(*this);
    float             V = (pTweak) ? ((float)pTweak->GetValue()) : (0);

    return V;
}

//==============================================================================

int tweak_handle::GetS32() const
{
    tweak_data_block* pTweak = (tweak_data_block*)g_DataVault.GetData(*this);
    //ASSERT( pTweak );
    int V = (pTweak) ? ((int)pTweak->GetValue()) : (0);

    return V;
}

//==============================================================================

Radian tweak_handle::GetRadian() const
{
    tweak_data_block* pTweak = (tweak_data_block*)g_DataVault.GetData(*this);
    //ASSERT( pTweak );
    Radian V = (pTweak) ? ((Radian)pTweak->GetValue()) : (0);

    // We're assuming this badboy was typed in degrees in the tables.
    V = DEG_TO_RAD(V);

    return V;
}

//==============================================================================

bool tweak_handle::GetBool() const
{
    tweak_data_block* pTweak = (tweak_data_block*)g_DataVault.GetData(*this);
    bool              V = (pTweak) ? ((bool)pTweak->GetValue()) : (0);

    return V;
}

//==============================================================================
//  DATA_HANDLE
//==============================================================================

tweak_data_block::tweak_data_block()
{
    SetType(DATA_TYPE_TWEAK);
}

//==============================================================================

tweak_data_block::~tweak_data_block()
{
}

//==============================================================================

float tweak_data_block::GetValue() const
{
    return m_Value;
}

//==============================================================================

void tweak_data_block::SetValue(float Value)
{
    m_Value = Value;
}

//=============================================================================
bool LoadTweaks(const char* pDirectory)
{
/*
    int                           i;
    std::vector<tweak_data_block> TweakDataArray;
    tweak_data_block              TweakData;

    // Open the file for reading
    token_stream TOK;

    if (!TOK.OpenFile(xfs("%s\\Tweak_General.txt", pDirectory))) {
        return false;
    }

    TweakDataArray.SetCapacity(911);

    // Loop through all the tweaks and add them to the array
    while (TOK.IsEOF() == false) {
        TOK.ReadString();

        // If this is a blank line then skip to next line
        if (x_stricmp(TOK.String(), "") == 0) {
            TOK.SkipToNextLine();
            continue;
        }

        // Load the tweak_data_block with its name
        char CopyOfName[64];
        x_strcpy(CopyOfName, TOK.String());
        TweakData.SetName(TOK.String());

        // Confirm there are no duplicate tweaks
        {
            for (i = 0; i < TweakDataArray.GetCount(); i++) {
                if (TweakData.GetDataID() == TweakDataArray[i].GetDataID()) {
                    break;
                }
            }
            if (i != TweakDataArray.GetCount()) {
                ASSERTS(false, "Tweak duplicate!\n");
                TOK.SkipToNextLine();
                continue;
            }
        }

        // Fill out the pain profile data
        TweakData.m_Value = TOK.ReadF32FromString();

        //  LOG_MESSAGE("LoadTweaks","NewTweak (%s) (%8.2f)",CopyOfName, TweakData.m_Value);

        // Add profile to the array
        TweakDataArray.Append(TweakData);

        // Fast forward to next line
        TOK.SkipToNextLine();
    }

    TOK.CloseFile();

    // Allocate final array of tweaks
    tweak_data_block* pTW = new tweak_data_block[TweakDataArray.GetCount()];
    for (i = 0; i < TweakDataArray.GetCount(); i++) {
        pTW[i] = TweakDataArray[i];
    }

    LOG_MESSAGE("LoadTweaks", "Tweak array size: %d elements", TweakDataArray.GetCount());
    LOG_MESSAGE("LoadTweaks", "End. Took %2.02f seconds.", Time.ReadSec());

    // Hand off to the data vault.  ..."Keep them out of trouble!"...
    g_DataVault.AddDataBlocks("TWDATA", pTW, TweakDataArray.GetCount(), sizeof(tweak_data_block));
*/
    return true;
}

void UnloadTweaks()
{
    tweak_data_block* pTW = (tweak_data_block*)g_DataVault.DelDataBlocks("TWDATA");
    delete[] pTW;
}

//==============================================================================
// TWEAK UTIL FUNCTIONS
//==============================================================================

float GetTweakF32(const char* pName)
{
    tweak_handle hTweak(pName);
    return hTweak.GetF32();
}

float GetTweakF32(const char* pName, float Default)
{
    tweak_handle hTweak(pName);
    if (hTweak.Exists()) {
        return hTweak.GetF32();
    } else {
        //CLOG_MESSAGE(DO_LOG_TWEAKS, "tweak_handle::GetF32", "(%8.3f) (%s) (not present so using code specified default value)", Default, pName);
        return Default;
    }
}

int GetTweakS32(const char* pName)
{
    tweak_handle hTweak(pName);
    return hTweak.GetS32();
}

int GetTweakS32(const char* pName, int Default)
{
    tweak_handle hTweak(pName);
    if (hTweak.Exists()) {
        return hTweak.GetS32();
    } else {
        //CLOG_MESSAGE(DO_LOG_TWEAKS, "tweak_handle::GetS32", "(%8d) (%s) (not present so using code specified default value)", Default, pName);
        return Default;
    }
}

Radian GetTweakRadian(const char* pName)
{
    tweak_handle hTweak(pName);
    return hTweak.GetRadian();
}

Radian GetTweakRadian(const char* pName, Radian Default)
{
    tweak_handle hTweak(pName);
    if (hTweak.Exists()) {
        return hTweak.GetRadian();
    } else {
        //CLOG_MESSAGE(DO_LOG_TWEAKS, "tweak_handle::GetRadian", "(%8.3f) (%s) (not present so using code specified default value)", RAD_TO_DEG(Default), pName);
        return Default;
    }
}

bool GetTweakBool(const char* pName)
{
    tweak_handle hTweak(pName);
    return hTweak.GetBool();
}

bool GetTweakBool(const char* pName, bool Default)
{
    tweak_handle hTweak(pName);
    if (hTweak.Exists()) {
        return hTweak.GetBool();
    } else {
        //CLOG_MESSAGE(DO_LOG_TWEAKS, "tweak_handle::GetBool", "(%8d) (%s) (not present so using code specified default value)", Default, pName);
        return Default;
    }
}

