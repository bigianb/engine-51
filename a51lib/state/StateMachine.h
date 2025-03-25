#pragma once

#include "../ui/UIManager.h"
#include "GameConfig.h"
#include "PlayerProfile.h"
#include "MapList.h"

class ResourceManager;

class StateMachine
{
public:
    enum class State
    {
        idle,
        ersb_notice,
        inevitable_intro,
        press_start_screen,
        main_menu,
        campaign_menu,
        profile_select,
        start_game,
        playing_game,
        exit_game
    };

    StateMachine();
    ~StateMachine();

    void init(ui::Manager*, ResourceManager*);

    void update(float deltaTime);

    State getState() { return state; }
    void  setState(State targetState);

    // Profile functions
    void setSelectedProfile(int playerID, uint32_t hash)
    {
        selectedProfile[playerID] = hash;
    }
    int getSelectedProfile(int playerID)
    {
        return selectedProfile[playerID];
    }
    void clearSelectedProfile(int playerID)
    {
        selectedProfile[playerID] = 0;
    }
    void setProfileNotSaved(int playerID, bool bNotSaved) { m_ProfileNotSaved[playerID] = bNotSaved; }
    bool getProfileNotSaved(int playerID) { return m_ProfileNotSaved[playerID]; }

    PlayerProfile& getActiveProfile(int playerID) { return m_Profiles[playerID]; }

    void        resetProfile(int index);
    const char* getProfileName(int index) { return m_Profiles[index].GetProfileName(); }

    // profiles are managed explicitly on the Front-end side.
    // These functions help to correctly map the indices on the client/server end.
    void resetProfileListIndex(void);
    void setupProfileListIndex(void);
    int  getProfileListIndex(int index) { return profileListIndex[index]; }
    void readProfiles();

    // pending profile functions
    void           initPendingProfile(int index);
    void           activatePendingProfile(bool MarkDirty = false);
    PlayerProfile& getPendingProfile() { return m_PendingProfile; }
    int            getPendingProfileIndex() { return m_PendingProfileIndex; }

    std::vector<profile_info>& getProfileList() { return m_ProfileNames; }

private:
    void enterState();
    void updateState();
    void exitState();

    GameConfig pendingConfig;
    GameConfig activeConfig;

    State            state;
    State            previousState;
    ui::Manager*     uiManager;
    ResourceManager* resourceManager;
    ui::Dialog*      currentDialog;

    map_list* mapList;

#define SM_PROFILE_COUNT 4

    // profiles
    PlayerProfile             m_Profiles[SM_PROFILE_COUNT];        // profile array - one for each player
    int                       profileListIndex[SM_PROFILE_COUNT];  // index in the list of profiles read from the card
    uint32_t                  selectedProfile[SM_PROFILE_COUNT];   // hash of the selected profile
    bool                      m_ProfileNotSaved[SM_PROFILE_COUNT]; // is the selected profile on card or only in memory
    std::vector<profile_info> m_ProfileNames;                      // xarray of profile names read from the memory cards
    PlayerProfile             m_PendingProfile;                    // pending profile changes
    int                       m_PendingProfileIndex;               // pending profile index
    bool                      m_bCreatingProfile;                  // we are creating a new profile
    bool                      m_bAutosaveProfile;                  // Autosave profile is enabled.
    bool                      m_bAutosaveInProgress;               // Autosave is currently in progress
    bool                      m_bDisableMemcardDialogs;            // Should we display memcard dialogs
    bool                      m_bFollowBuddy;                      // Following your buddy in to a game, SessionID in g_Pending
    //login_source            m_LoginFailureDestination;
    bool m_bSilentSigninStarted;
    int  m_iCard; // Card slot in use for current operation

    void enterESRBNotice();
    void updateESRBNotice();
    void exitESRBNotice();

    void enterPressStart();
    void updatePressStart();

    void enterMainMenu();
    void updateMainMenu();

    void enterProfileSelect();
    void updateProfileSelect();
};
