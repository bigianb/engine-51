#include "StateMachine.h"

#include "../ui/UIManager.h"
#include "../VectorMath.h"

#define DIALOG_TOP 24
#define DIALOG_BOTTOM 448-72

StateMachine::StateMachine()
{
    uiManager = nullptr;
    state = State::idle;
}

void StateMachine::init(ui::Manager* ui)
{
    uiManager = ui;

    state = State::idle;
    previousState = state;
    setState(State::ersb_notice);

    ui->enableUser(ui->getUserId(), true);
}

void StateMachine::update(float deltaTime)
{
    updateState();
}

void StateMachine::setState(State targetState)
{
    if (targetState != state) {

        if (currentDialog) {
            currentDialog->setState(ui::Dialog::DialogState::Active);

            // store the current control id
            //m_CurrentControl[m_State] = m_CurrentDialog->GetControl();
        }

        // Do clean up for current state
        exitState();
        currentDialog = nullptr;

        // Set the current state and reset the timeout
        //m_Timeout   = m_TimeoutTime;
        previousState = state;
        state = targetState;

        enterState();
    }
}

void StateMachine::enterState()
{
    switch (state) {
    case State::ersb_notice:
        enterESRBNotice();
        break;
    case State::press_start_screen:
        enterPressStart();
        break;
    case State::main_menu:
        enterMainMenu();
        break;
    default:
        break;
    }
}

void StateMachine::updateState()
{
    switch (state) {
    case State::ersb_notice:
        updateESRBNotice();
        break;
    case State::press_start_screen:
        updatePressStart();
        break;
    case State::main_menu:
        updateMainMenu();
        break;
    default:
        break;
    }
}

void StateMachine::exitState()
{
}

void StateMachine::enterESRBNotice()
{
    IntRect mainarea(16, 16, 512 - 16, 448 - 16);
    currentDialog = uiManager->openDialog("ESRB", mainarea, nullptr, ui::Window::WF_VISIBLE);
    //uiManager->SetUserBackground( g_UiUserID, "" );
}

void StateMachine::enterPressStart()
{
    uiManager->endDialog( true );
    IntRect mainarea(16, 16, 512-16, 448-16);
    currentDialog = uiManager->openDialog("press start", mainarea, nullptr, ui::Window::WF_VISIBLE);
    //g_UiMgr->SetUserBackground( g_UiUserID, "" );
}

void StateMachine::updatePressStart()
{
    if (currentDialog != nullptr) {
        auto dialogState = currentDialog->getState();

        if (dialogState == ui::Dialog::DialogState::Select) {
            // TODO: really should go to inevitable intro which will play the midway movie
            setState(State::main_menu);
        }
    }
}

void StateMachine::updateESRBNotice()
{
    if (currentDialog != nullptr) {
        auto dialogState = currentDialog->getState();

        if (dialogState == ui::Dialog::DialogState::Select) {
            // TODO: really should go to inevitable intro which will play the midway movie
            setState(State::press_start_screen);
        }
    }
}

void StateMachine::exitESRBNotice()
{
    //uiManager->endDialog( g_UiUserID, TRUE );
}

void StateMachine::enterMainMenu()
{
    // Should play movie MenuBackground instead of the background.
    //uiManager->loadBackground ( "background1", "A51Background.XBMP" );
    IntRect mainarea(136, DIALOG_TOP, 376, DIALOG_BOTTOM );
    currentDialog = uiManager->openDialog("main menu", mainarea, nullptr, ui::Window::WF_VISIBLE | ui::Window::WF_BORDER);
    // If playing movie, don't set the background.
    //uiManager->setUserBackground( "background1" );
}

void StateMachine::updateMainMenu()
{
    if (currentDialog != nullptr) {
        auto dialogState = currentDialog->getState();
        if (dialogState == ui::Dialog::DialogState::Select) {
            // TODO: main menu should contain the update logic.
        }
    }
}
