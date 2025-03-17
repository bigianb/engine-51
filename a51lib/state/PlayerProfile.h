#pragma once
#include <cstdint>
#include <string>
#include <ctime>

#include "CheckPointMgr.h"
#include "LoreList.h"

#define MAX_SAVED_LEVELS 20

#define PROFILE_VERSION_NUMBER 1027

enum profile_states
{
    PROFILE_OK,
    PROFILE_CORRUPT,
    PROFILE_EXPIRED,
};

enum difficulty_level
{
    DIFFICULTY_EASY,
    DIFFICULTY_MEDIUM,
    DIFFICULTY_HARD,
};

class PlayerProfile
{
public:
    PlayerProfile(void);
    void     Reset(void);
    uint32_t GetVersion(void) { return m_Version; }

    void RestoreControlDefaults(void);
    void RestoreAudioDefaults(void);
    void RestoreHeadsetDefaults(void);

    void        SetProfileName(const char* pProfileName);
    const char* GetProfileName(void) const { return (const char*)m_pProfileName; }

    void     SetHash(void);
    uint32_t GetHash(void) { return m_HashString; }

    uint32_t GetSensitivity(uint32_t Index) const
    {
        return m_Sensitivity[Index];
    }

    void SetSensitivity(uint32_t Index, uint32_t Sensitivity)
    {
        m_Sensitivity[Index] = Sensitivity;
    }

    uint8_t GetVolume(uint32_t Index) const
    {
        return m_Volume[Index];
    }
    void SetVolume(uint32_t Index, uint8_t Volume)
    {
        m_Volume[Index] = Volume;
    }

    int32_t GetAvatarID(void) { return m_AvatarID; }
    void    SetAvatarID(int32_t AvatarID) { m_AvatarID = AvatarID; }

    bool GetLoreAcquired(uint32_t Vault, int32_t Index);
    void SetLoreAcquired(uint32_t Vault, uint32_t Index);

    void AcquireSecret(void);

    uint32_t GetTotalLoreAcquired(void) { return m_LoreTotal; }
    bool     IsNewLoreUnlocked(void) { return m_bNewLoreUnlocked; }
    void     ClearNewLoreUnlocked(void) { m_bNewLoreUnlocked = false; }

    uint32_t GetNumSecretsUnlocked(void) { return m_NumSecretsUnlocked; }
    bool     IsNewSecretUnlocked(void) { return m_bNewSecretUnlocked; }
    void     ClearNewSecretUnlocked(void) { m_bNewSecretUnlocked = false; }

    void           SetUniqueId(const uint8_t* pUniqueId, int32_t IdLength);
    const uint8_t* GetUniqueId(int32_t& IdLength);

    bool DisplayCinemaMutatedMsg(void);

    void SetPlayerIsMutated(bool bIsMutated) { m_bIsMutated = bIsMutated; }
    bool IsPlayerMutated(void) { return m_bIsMutated; }

    void    SetDifficultyLevel(uint8_t Difficulty) { m_DifficultyLevel = Difficulty; }
    uint8_t GetDifficultyLevel(void) { return m_DifficultyLevel; }

    void SetWeaponAutoSwitch(bool bAutoSwitch) { m_bWeaponAutoSwitch = bAutoSwitch; }
    bool GetWeaponAutoSwitch(void) { return m_bWeaponAutoSwitch; }

    level_check_points& GetCheckpoint(int32_t Index) { return m_Checkpoints[Index]; }
    level_check_points* GetCheckpointByMapID(int32_t MapID);

    void Checksum(void);
    bool Validate(void);
    bool HasChanged(void);
    void MarkDirty(void);

private:
    int32_t  m_Checksum;           // CRC32 of the profile
    uint32_t m_Version;            // profile version control
    char     m_pProfileName[32];   // the nickname (MUST BE FIRST)
    uint32_t m_HashString;         // hash of the profile name and the time created
    int32_t  m_AvatarID;           // avatar
    uint32_t m_Sensitivity[2];     // controller sensitivity
    uint8_t  m_Volume[5];          // volume controls
    uint8_t  m_Lore[NUM_VAULTS];   // lore acquired flags
    uint32_t m_LoreTotal;          // total lore acquired
    bool     m_bNewLoreUnlocked;   // flag set when new piece of lore is unlocked
    uint32_t m_NumSecretsUnlocked; // number of secrets unlocked
    bool     m_bNewSecretUnlocked; // flag set when a new secret is unlocked
    uint8_t  m_UniqueId[64];
    int32_t  m_UniqueIdLength;
    bool     m_bIsMutated;            // is the player currently in mutant form
    int8_t   m_CinemaMutatedMsgCount; // counts how many times we should display a msg
    uint8_t  m_DifficultyLevel;       // campaign game difficulty level
    bool     m_bWeaponAutoSwitch;     // if on/true, will auto-switch to a weapon with a > rating

    level_check_points m_Checkpoints[MAX_SAVED_LEVELS];

public:
    // members
    uint32_t m_bLookspringOn : 1;      // toggle lookspring
    uint32_t m_bCrouchOn : 1;          // toggle crouch
    uint32_t m_bIsActive : 1;          // is currently active
    uint32_t m_bInvertY : 1;           // invert Y axis
    uint32_t m_bVibration : 1;         // Force feedback/vibration enabled
    uint32_t m_bIsVisibleOnline : 1;   // Report full status when online
    uint32_t m_bAutosaveOn : 1;        // Autosave is enabled
    uint32_t m_bAlienAvatarsOn : 1;    // Alien avatars are selectable
    uint32_t m_bSecretAwarded : 1;     // Give a secret away after deep underground
    uint32_t m_bHardUnlocked : 1;      // Difficulty level hard is available
    uint32_t m_bDifficultyChanged : 1; // Was the difficulty level changed during a campaign
    uint32_t m_bAgeVerified : 1;       // The player's age has been verified for this profile (COPA requirement)
    uint32_t m_bGameFinished : 1;      // Player has finished the game.
};

//==============================================================================
//  profile_info
//==============================================================================

struct profile_info
{
    bool         bDamaged;
    int32_t      ProfileID;
    int32_t      CardID;
    std::wstring Name;
    uint32_t     Hash;
    uint32_t     Ver;
    std::string  Dir;
    std::time_t  CreationDate;
    std::time_t  ModifiedDate;
};
