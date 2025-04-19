
#include "PainMgr.h"
#include "../xfiles/xfs.h"
//#include "parsing\tokenizer.hpp"

#define DO_LOG_PAIN

//==============================================================================
// SUPPORT FOR GENERIC PAIN
//==============================================================================

typedef enum_pair<generic_pain_type> generic_pain_type_enum_pair;
static generic_pain_type_enum_pair   s_GenericPainTypeList[] =
    {
        generic_pain_type_enum_pair("Generic_1", TYPE_GENERIC_1),
        generic_pain_type_enum_pair("Generic_2", TYPE_GENERIC_2),
        generic_pain_type_enum_pair("Generic_3", TYPE_GENERIC_3),
        generic_pain_type_enum_pair("Generic_Lethal", TYPE_LETHAL),
        generic_pain_type_enum_pair("Generic_Explosive", TYPE_EXPLOSIVE),
        generic_pain_type_enum_pair("Generic_LASER", TYPE_LASER),
        generic_pain_type_enum_pair("ACID_WEAK", TYPE_ACID_1),
        generic_pain_type_enum_pair("ACID_STRONG", TYPE_ACID_2),
        generic_pain_type_enum_pair("ACID_LETHAL", TYPE_ACID_3),

        generic_pain_type_enum_pair("GOO_WEAK", TYPE_GOO_1),
        generic_pain_type_enum_pair("GOO_STRONG", TYPE_GOO_2),
        generic_pain_type_enum_pair("GOO_LETHAL", TYPE_GOO_3),

        generic_pain_type_enum_pair("FIRE_WEAK", TYPE_FIRE_1),
        generic_pain_type_enum_pair("FIRE_STRONG", TYPE_FIRE_2),
        generic_pain_type_enum_pair("FIRE_LETHAL", TYPE_FIRE_3),

        generic_pain_type_enum_pair("DROWNING", TYPE_DROWNING),

        generic_pain_type_enum_pair(k_EnumEndStringConst, TYPE_GENERIC_1)};
enum_table<generic_pain_type> g_GenericPainTypeList(s_GenericPainTypeList);

//==============================================================================

pain_handle GetPainHandleForGenericPain(generic_pain_type Type)
{
    assert(g_GenericPainTypeList.DoesValueExist(Type));
    pain_handle Handle(g_GenericPainTypeList.GetString(Type));
    return Handle;
}

//==============================================================================
//  PAIN_HANDLE
//==============================================================================

pain_handle::pain_handle()
{
    data_handle::SetType(DATA_TYPE_PAIN_PROFILE);
}

pain_handle::~pain_handle()
{
}

pain_handle::pain_handle(const char* pPainDescriptor)
{
    data_handle::SetType(DATA_TYPE_PAIN_PROFILE);
    data_handle::SetName(pPainDescriptor);
}

const pain_profile* pain_handle::GetPainProfile() const
{
    const pain_profile* pData = (pain_profile*)GetData();

    return pData;
}

pain_health_handle pain_handle::BuildPainHealthProfileHandle(const health_handle& HHandle) const
{
    pain_health_handle    PHHandle;
    const pain_profile*   pPP = GetPainProfile();
    const health_profile* pHP = HHandle.GetHealthProfile();
    if (pPP && pHP) {
        PHHandle.SetName(xfs("PH(%03d)(%03d)", pPP->m_iPainHealthTable, pHP->m_iPainHealthTable));
    }
    return PHHandle;
}

//==============================================================================
// HEALTH_HANDLE
//==============================================================================

health_handle::health_handle()
{
    data_handle::SetType(DATA_TYPE_HEALTH_PROFILE);
}

//==============================================================================

health_handle::~health_handle()
{
}

//==============================================================================

health_handle::health_handle(const char* pHealthDescriptor)
{
    data_handle::SetType(DATA_TYPE_HEALTH_PROFILE);
    data_handle::SetName(pHealthDescriptor);
}

const health_profile* health_handle::GetHealthProfile() const
{
    const health_profile* pData = (health_profile*)GetData();

    return pData;
}

//==============================================================================
// PAIN_HEALTH_HANDLE
//==============================================================================

pain_health_handle::pain_health_handle()
{
    data_handle::SetType(DATA_TYPE_PAIN_HEALTH_PROFILE);
}

pain_health_handle::~pain_health_handle()
{
}

pain_health_handle::pain_health_handle(const char* pPainHealthDescriptor)
{
    data_handle::SetType(DATA_TYPE_PAIN_HEALTH_PROFILE);
    data_handle::SetName(pPainHealthDescriptor);
}

const pain_health_profile* pain_health_handle::GetPainHealthProfile() const
{
    const pain_health_profile* pData = (pain_health_profile*)GetData();

    return pData;
}

pain_profile::pain_profile()
{
    SetType(DATA_TYPE_PAIN_PROFILE);
}

pain_profile::~pain_profile()
{
}

health_profile::health_profile()
{
    SetType(DATA_TYPE_HEALTH_PROFILE);
}

health_profile::~health_profile()
{
}

pain_health_profile::pain_health_profile()
{
    SetType(DATA_TYPE_PAIN_HEALTH_PROFILE);
}

pain_health_profile::~pain_health_profile()
{
}

//==============================================================================
// LOAD PAIN PROFILES
//==============================================================================

bool LoadPainProfiles(const char* pFileName)
{
    int                       i;
    std::vector<pain_profile> PainProfileArray;
    pain_profile              PainProfile;
    /*
       // Open the file for reading
       token_stream TOK;
       if( !TOK.OpenFile( pFileName ) )
           return false;

       // Loop through the header tokens
       {
           TOK.SkipToNextLine();
       }

       // Loop through all the profiles and add them to the array
       while( TOK.IsEOF() == false )
       {
           TOK.ReadString();

           // If this is a blank line then skip to next line
           if( x_stricmp(TOK.String(),"")==0 )
           {
               TOK.SkipToNextLine();
               continue;
           }

           // Load the pain_profile with its name
           PainProfile.SetName( TOK.String() );

           // Confirm there are no duplicate pain profiles
           {
               for( i=0; i<PainProfileArray.GetCount(); i++ )
               {
                   if( PainProfile.GetDataID() == PainProfileArray[i].GetDataID() )
                       break;
               }
               if( i!=PainProfileArray.GetCount() )
               {
                   ASSERTS(false,"PainProfile duplicate!\n");
                   TOK.SkipToNextLine();
                   continue;
               }
           }

           // Skip Blank column
           TOK.ReadString();

           // Fill out the pain profile data
           PainProfile.m_DamageNearDist   = TOK.ReadF32FromString();
           PainProfile.m_DamageFarDist    = TOK.ReadF32FromString();
           PainProfile.m_ForceNearDist    = TOK.ReadF32FromString();
           PainProfile.m_ForceFarDist     = TOK.ReadF32FromString();
           PainProfile.m_bCheckLOS        = TOK.ReadBoolFromString();
           PainProfile.m_bSplash          = TOK.ReadBoolFromString();
           PainProfile.m_iPainHealthTable  = PainProfileArray.GetCount();

           if( PainProfile.m_DamageFarDist == PainProfile.m_DamageNearDist )
               PainProfile.m_DamageFarDist += 1.0f;

           if( PainProfile.m_ForceFarDist == PainProfile.m_ForceNearDist )
               PainProfile.m_ForceFarDist += 1.0f;

           // Build pain bbox
           f32 MaxRadius = MAX(PainProfile.m_DamageFarDist,PainProfile.m_ForceFarDist);
               MaxRadius = MAX(MaxRadius,10);
           PainProfile.m_BBox = bbox( vector3(0,0,0), MaxRadius );

           // Add profile to the array
           PainProfileArray.Append( PainProfile );

           // Fast forward to next line
           TOK.SkipToNextLine();
       }

       TOK.CloseFile();

       // Allocate final array of pain profiles
       pain_profile* pPP = new pain_profile[ PainProfileArray.GetCount() ];
       for( i=0; i<PainProfileArray.GetCount(); i++ )
           pPP[i] = PainProfileArray[i];

       // Hand off to the data vault.  ..."Keep them out of trouble!"...
       g_DataVault.AddDataBlocks("P_PROFILE",pPP,PainProfileArray.GetCount(),sizeof(pain_profile));

       LOG_MESSAGE("LoadPainProfiles","End");
   */
    return true;
}

//==============================================================================
// LOAD HEALTH PROFILES
//==============================================================================

bool LoadHealthProfiles(const char* pFileName)
{

    int                         i;
    std::vector<health_profile> HealthProfileArray;
    health_profile              HealthProfile;
    /*
        // Open the file for reading
        token_stream TOK;
        if( !TOK.OpenFile( pFileName ) )
            return false;

        // Loop through all the profiles and add them to the array
        int LN = TOK.GetLineNumber();
        while( 1 )
        {
            TOK.ReadString();

            if( TOK.GetLineNumber() != LN )
                break;

            // If this is a blank line then skip to next line
            if( x_stricmp(TOK.String(),"")==0 )
            {
                continue;
            }

            // Load the health_profile with its name
            HealthProfile.SetName( TOK.String() );

            // Confirm there are no duplicate health profiles
            {
                for( i=0; i<HealthProfileArray.GetCount(); i++ )
                {
                    if( HealthProfile.GetDataID() == HealthProfileArray[i].GetDataID() )
                        break;
                }
                if( i!=HealthProfileArray.GetCount() )
                {
                    ASSERTS(false,"HealthProfile duplicate!\n");
                    continue;
                }
            }

            // Fill out the health profile data
            HealthProfile.m_iPainHealthTable  = HealthProfileArray.GetCount();

            // Add profile to the array
            HealthProfileArray.Append( HealthProfile );
        }

        TOK.CloseFile();

        // Allocate final array of health profiles
        health_profile* pHP = new health_profile[ HealthProfileArray.GetCount() ];
        for( i=0; i<HealthProfileArray.GetCount(); i++ )
            pHP[i] = HealthProfileArray[i];

        // Hand off to the data vault.  ..."Keep them out of trouble!"...
        g_DataVault.AddDataBlocks("H_PROFILE",pHP,HealthProfileArray.GetCount(),sizeof(health_profile));

        LOG_MESSAGE("LoadHealthProfiles","End");
    */
    return true;
}

//==============================================================================
// LOAD PAIN_HEALTH PROFILES
//==============================================================================

bool BuildPainHealthProfiles(const char* pFileName)
{
    /*
        health_handle         BlankHealthHandle;
        std::vector<health_handle> HealthHandle;
        std::vector<pain_health_profile> PainHealthProfileArray;
        pain_health_profile PainHealthProfile;
        // Open the file for reading
        token_stream TOK;
        if( !TOK.OpenFile( pFileName ) )
            return false;

        xtimer t;
        xtimer Time;

        HealthHandle.SetCapacity( 100 );

        t.Start();
        Time.Start();

        // Read in the health_profile names
        int LN = TOK.GetLineNumber();

        while( 1 )
        {
            TOK.ReadString();
            if( TOK.GetLineNumber() != LN )
                break;

            // Load the health_handle with its name
            health_handle& HH = HealthHandle.Append();
            if( x_stricmp(TOK.String(),"")!=0 )
            {
                HH.SetName( TOK.String() );
            }
        }

        // This initial capacity should match the final capacity recorded in the logger.
        // If this matches or exceeds the logger value, we elimate any unnecessary
        // resizing of the std::vector.
        PainHealthProfileArray.SetCapacity( 6785 );

        // Rewind and go to second line where the pain listing starts
        TOK.Rewind();
        TOK.SkipToNextLine();

        while( TOK.IsEOF() == false )
        {
            TOK.ReadString();

            if( x_stricmp(TOK.String(),"")==0 )
            {
                TOK.SkipToNextLine();
                continue;
            }

            // Load the handle with its name
            pain_handle PainHandle( TOK.String() );

            // If the profile doesn't exist then move to next line
            const pain_profile* pPP = PainHandle.GetPainProfile();
            if( pPP == NULL )
            {
                TOK.SkipToNextLine();
                continue;
            }

            // Loop across reading in values for each health_handle
            int iHealthHandle=1;
            for( iHealthHandle=1; iHealthHandle<HealthHandle.GetCount(); iHealthHandle++ )
            {
                // If it's blank then just move on to next handle
                if( HealthHandle[iHealthHandle].GetDataID() == BlankHealthHandle.GetDataID() )
                    continue;

                // Is this a valid health_handle?
                const health_profile* pHP = HealthHandle[iHealthHandle].GetHealthProfile();
                if( pHP == NULL )
                    continue;

                // Build a pain_health_profile for this combination
                pain_health_handle PHHandle = PainHandle.BuildPainHealthProfileHandle( HealthHandle[iHealthHandle] );
                PainHealthProfile.SetDataID( PHHandle.GetDataID() );
                PainHealthProfile.m_Damage  = 0;
                PainHealthProfile.m_Force   = 0;
                PainHealthProfile.m_HitType = 0;

                // Append the new profile to the list
                PainHealthProfileArray.Append(PainHealthProfile);
            }

            // Fast forward to next line
            TOK.SkipToNextLine();
        }

        TOK.CloseFile();

        // Allocate final array
        pain_health_profile* pPHP = new pain_health_profile[ PainHealthProfileArray.GetCount() ];
        for( int i=0; i<PainHealthProfileArray.GetCount(); i++ )
            pPHP[i] = PainHealthProfileArray[i];

        // Hand off to the data vault.  ..."Keep them out of trouble!"...
        g_DataVault.AddDataBlocks("PH_PROFILE",pPHP,PainHealthProfileArray.GetCount(),sizeof(pain_health_profile));

        LOG_MESSAGE( "BuildPainHealthProfiles","PainHealthProfileArray Size : %d elements",PainHealthProfileArray.GetCount() );
        LOG_MESSAGE( "BuildPainHealthProfiles","HealthHandle Size : %d elements",HealthHandle.GetCount() );
        LOG_MESSAGE( "BuildPainHealthProfiles","End. Took %2.02f seconds.", Time.ReadSec() );

        */
    return true;
}

//==============================================================================

bool FillPainHealthProfileData(const char* pFileName, int DataOffset, bool bFloat)
{
    health_handle              BlankHealthHandle;
    std::vector<health_handle> HealthHandle;
    /*
        // Open the file for reading
        token_stream TOK;
        if( !TOK.OpenFile( pFileName ) )
            return false;

        // Read in the health_profile names
        int LN = TOK.GetLineNumber();
        xtimer t;
        xtimer Time;
        t.Start();
        Time.Start();
        while( 1 )
        {
            TOK.ReadString();
            if( TOK.GetLineNumber() != LN )
                break;

            // Load the health_handle with its name
            health_handle& HH = HealthHandle.Append();
            if( x_stricmp(TOK.String(),"")!=0 )
            {
                HH.SetName( TOK.String() );
            }
        }

        // Rewind and go to second line where the pain listing starts
        TOK.Rewind();
        TOK.SkipToNextLine();

        while( TOK.IsEOF() == false )
        {
            TOK.ReadString();

            if( x_stricmp(TOK.String(),"")==0 )
            {
                TOK.SkipToNextLine();
                continue;
            }

            // Load the handle with its name
            pain_handle PainHandle( TOK.String() );

            // If the profile doesn't exist then move to next line
            const pain_profile* pPP = PainHandle.GetPainProfile();
            if( pPP == NULL )
            {
                TOK.SkipToNextLine();
                continue;
            }

            // Loop across reading in values for each health_handle
            int iHealthHandle=1;
            for( iHealthHandle=1; iHealthHandle<HealthHandle.GetCount(); iHealthHandle++ )
            {
                // Read value
                f32 V = TOK.ReadF32FromString();

                // If the healthhandle is blank then just move on to next handle
                if( HealthHandle[iHealthHandle].GetDataID() == BlankHealthHandle.GetDataID() )
                    continue;

                // Is this a valid health_handle?
                const health_profile* pHP = HealthHandle[iHealthHandle].GetHealthProfile();
                if( pHP == NULL )
                    continue;

                // Build a pain_health_profile for this combination
                pain_health_handle PHHandle = PainHandle.BuildPainHealthProfileHandle( HealthHandle[iHealthHandle] );
                pain_health_profile* pPHP = (pain_health_profile*)PHHandle.GetPainHealthProfile();
                if( pPHP )
                {
                    if( bFloat )    ((f32*)(((byte*)pPHP) + DataOffset))[0] = (f32)V;
                    else            ((int*)(((byte*)pPHP) + DataOffset))[0] = (int)V;
                }
            }

            // Fast forward to next line
            TOK.SkipToNextLine();
        }

        TOK.CloseFile();

        LOG_MESSAGE("FillPainHealthProfileData","Took %2.02fms to build profile data", Time.ReadSec() );
    */
    return true;
}

//==============================================================================

bool LoadPainHealthProfiles(const char* pDirectory)
{
    /*
        // Go through damage table and build pain_health_profile entries
        if (!BuildPainHealthProfiles(xfs("%s\\Tweak_DamageTable.txt", pDirectory))) {
            return false;
        }

        pain_health_profile PHP;

        if (!FillPainHealthProfileData(xfs("%s\\Tweak_DamageTable.txt", pDirectory), ((u32)&PHP.m_Damage) - ((u32)&PHP), true)) {
            return false;
        }

        if (!FillPainHealthProfileData(xfs("%s\\Tweak_ForceTable.txt", pDirectory), ((u32)&PHP.m_Force) - ((u32)&PHP), true)) {
            return false;
        }

        if (!FillPainHealthProfileData(xfs("%s\\Tweak_HitTypeTable.txt", pDirectory), ((u32)&PHP.m_HitType) - ((u32)&PHP), false)) {
            return false;
        }
    */
    return true;
}

//==============================================================================

bool LoadPain(const char* pDirectory)
{
/*
    if (!LoadPainProfiles(xfs("%s\\Tweak_PainProfile.txt", pDirectory))) {
        return false;
    }

    if (!LoadHealthProfiles(xfs("%s\\Tweak_DamageTable.txt", pDirectory))) {
        return false;
    }

    if (!LoadPainHealthProfiles(pDirectory)) {
        return false;
    }
*/
    return true;
}

void UnloadPain()
{
    // Remove data from the vault
    pain_profile*        pPP = (pain_profile*)g_DataVault.DelDataBlocks("P_PROFILE");
    health_profile*      pHP = (health_profile*)g_DataVault.DelDataBlocks("H_PROFILE");
    pain_health_profile* pPHP = (pain_health_profile*)g_DataVault.DelDataBlocks("PH_PROFILE");

    // Deallocate the arrays
    delete[] pPP;
    delete[] pHP;
    delete[] pPHP;
}
