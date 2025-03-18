#include "PlayerProfile.h"
#include "../dataUtil/Crc.h"

#include <ctime>

PlayerProfile::PlayerProfile()
{
    Reset();
}

void PlayerProfile::Reset()
{
    int i = 0;

    memset(this, 0, sizeof(PlayerProfile));

    m_Version = PROFILE_VERSION_NUMBER;
    m_HashString = 0;
    m_AvatarID = 0;
    m_Sensitivity[0] = 50;
    m_Sensitivity[1] = 50;
    m_LoreTotal = 0;
    m_bNewLoreUnlocked = false;
    m_NumSecretsUnlocked = 0;
    m_bNewSecretUnlocked = false;
    m_bInvertY = true;
    m_bVibration = true;
    m_bCrouchOn = false;
    m_bLookspringOn = false;
    m_bIsVisibleOnline = true;
    m_bAutosaveOn = true;
    m_bAlienAvatarsOn = false;
    m_UniqueIdLength = 0;
    m_CinemaMutatedMsgCount = 3;
    m_bIsMutated = false;
    m_DifficultyLevel = 1;      // medium difficulty by default
    m_bWeaponAutoSwitch = true; // if on/true, will auto-switch to a weapon with a > rating (default is true)
    m_bSecretAwarded = false;
    m_bHardUnlocked = false;
    m_bDifficultyChanged = false;
    m_bGameFinished = false;
    m_bAgeVerified = true;

    // Clear lore collected
    for (i = 0; i < NUM_VAULTS; i++) {
        m_Lore[i] = 0;
    }

    // Mark all checkpoints as having invalid level id, this will make it
    // available for use.
    for (i = 0; i < MAX_SAVED_LEVELS; i++) {
        m_Checkpoints[i].Init(-1);
    }
}

void PlayerProfile::SetHash()
{
    std::string TempString(m_pProfileName);
    uint32_t    Hash = 5381;

    time_t rawtime;
    time(&rawtime);
    TempString += ctime(&rawtime);

    const char* pString = TempString.c_str();
    while (*pString) {
        Hash = (Hash * 33) ^ *pString++;
    }

    m_HashString = Hash;
}

void PlayerProfile::RestoreControlDefaults(void)
{
    m_Sensitivity[0] = 50;
    m_Sensitivity[1] = 50;
    m_bLookspringOn = false;
    m_bCrouchOn = false;
    m_bInvertY = true;
    m_bVibration = true;
    m_bWeaponAutoSwitch = true;
}

void PlayerProfile::SetProfileName(const char* pProfileName)
{
    strcpy(m_pProfileName, pProfileName);
}

bool PlayerProfile::GetLoreAcquired(uint32_t Vault, int Index)
{
    // check if this is a general inquiry
    if (Index == -1) {
        return (m_Lore[Vault] != 0);
    }

    // return specific lore acquired
    return (m_Lore[Vault] & (1 << Index));
}

void PlayerProfile::SetLoreAcquired(uint32_t Vault, uint32_t Index)
{
    // make sure we're not already acquired
    if (!GetLoreAcquired(Vault, Index)) {
        m_Lore[Vault] += (1 << Index);
        m_LoreTotal++;
        m_bNewLoreUnlocked = true;
    }

    // check for unlocking secrets
    if ((m_LoreTotal % 5) == 0) {
        if (m_LoreTotal == 90) {
            m_NumSecretsUnlocked = 21;
        } else {
            m_NumSecretsUnlocked++;
        }
        m_bNewSecretUnlocked = true;
    }
    // TODO: Auto save
    //g_StateMgr.SilentSaveProfile();
}


void PlayerProfile::AcquireSecret()
{
    // used to award a secret after deep underground
    if (!m_bSecretAwarded) {
        m_NumSecretsUnlocked++;
        m_bSecretAwarded = true;
        m_bNewSecretUnlocked = true;
    }
}


level_check_points* PlayerProfile::GetCheckpointByMapID(int MapID)
{
    for (int i = 0; i < MAX_SAVED_LEVELS; i++) {
        level_check_points* pCheckpoint = &m_Checkpoints[i];

        // Did we find one the same as the current map id? If so, bail out.
        if (pCheckpoint->MapID == MapID) {
            // found it!
            return pCheckpoint;
        } else if (pCheckpoint->MapID == -1) {
            // no checkpoint for this MapID
            return NULL;
        }
    }
    // ran out of checkpoints!

    return NULL;
}

void PlayerProfile::Checksum(void)
{
    m_Checksum = 0;
    m_Checksum = calcCRC(this, sizeof(PlayerProfile));
}

bool PlayerProfile::Validate(void)
{
    int DesiredChecksum;
    int ActualChecksum;

    DesiredChecksum = m_Checksum;
    m_Checksum = 0;
    ActualChecksum = calcCRC(this, sizeof(PlayerProfile));
    m_Checksum = DesiredChecksum;
    return (ActualChecksum == DesiredChecksum);
}

bool PlayerProfile::HasChanged()
{
    return !Validate();
}

void PlayerProfile::MarkDirty()
{
    m_Checksum = 0;
}

void PlayerProfile::SetUniqueId(const uint8_t* pUniqueId, int Length)
{
    memset(m_UniqueId, 0, sizeof(m_UniqueId));
    if (Length >= sizeof(m_UniqueId)) {
        Length = sizeof(m_UniqueId) - 1;
    }
    memcpy(m_UniqueId, pUniqueId, Length);
    m_UniqueIdLength = Length;
}

const uint8_t* PlayerProfile::GetUniqueId(int& Length)
{
    Length = m_UniqueIdLength;
    return m_UniqueId;
}

bool PlayerProfile::DisplayCinemaMutatedMsg(void)
{
    if (m_CinemaMutatedMsgCount > 0) {
        m_CinemaMutatedMsgCount--;
        return true;
    }

    return false;
}
