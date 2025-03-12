#pragma once

#include "../ui/UIManager.h"
#include "GameConfig.h"

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

    void init(ui::Manager*);

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

private:
    void enterState();
    void updateState();
    void exitState();

    GameConfig pendingConfig;
    GameConfig activeConfig;

    State        state;
    State        previousState;
    ui::Manager* uiManager;
    ui::Dialog*  currentDialog;

#define SM_PROFILE_COUNT 4

    // profiles
    //player_profile          m_Profiles[SM_PROFILE_COUNT];           // profile array - one for each player
    int      profileListIndex[SM_PROFILE_COUNT]; // index in the list of profiles read from the card
    uint32_t selectedProfile[SM_PROFILE_COUNT];  // hash of the selected profile

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
