#include "StateMachine.h"

#include "../ui/UIManager.h"
#include "../VectorMath.h"

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
