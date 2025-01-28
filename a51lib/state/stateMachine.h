#pragma once

#include "../ui/UIManager.h"

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
        start_game,
        playing_game,
        exit_game
    };

    StateMachine();

    void init(ui::Manager*);

    void update(float deltaTime);

    State getState() { return state; }
    void setState(State targetState);

private:
    void enterState();
    void updateState();
    void exitState();

    State state;
    State previousState;
    ui::Manager* uiManager;
    ui::Dialog* currentDialog;

    void enterESRBNotice();
    void updateESRBNotice();
    void exitESRBNotice();
};
