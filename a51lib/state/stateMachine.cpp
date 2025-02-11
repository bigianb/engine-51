#include "stateMachine.h"

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
        /*
        if( m_CurrentDialog )
        {
            m_CurrentDialog->SetState( DIALOG_STATE_ACTIVE );

            // store the current control id
            m_CurrentControl[m_State] = m_CurrentDialog->GetControl();
        }
*/
        // Do clean up for current state
        exitState();

        // reset dialog pointer
        //m_CurrentDialog = NULL;

        // Set the current state and reset the timeout
        //m_Timeout   = m_TimeoutTime;
        previousState = state;
        state = targetState;

        enterState();
    }
}

void StateMachine::enterState()
{
    switch(state){
        case State::ersb_notice:
            enterESRBNotice();
            break;
        default:
            break;
    }
}

void StateMachine::updateState()
{
    switch(state){
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
    IntRect mainarea(16, 16, 512-16, 448-16);
    currentDialog = uiManager->openDialog("ESRB", mainarea, nullptr, ui::Window::WF_VISIBLE);
    //uiManager->SetUserBackground( g_UiUserID, "" );
}

void StateMachine::updateESRBNotice()
{
    // Get the current dialog state
    /*
    if( m_CurrentDialog != NULL )
    {
        u32 DialogState = m_CurrentDialog->GetState();

        if( ( DialogState == DIALOG_STATE_SELECT ) || ( CONFIG_IS_AUTOSERVER || CONFIG_IS_AUTOCLIENT || CONFIG_IS_AUTOCAMPAIGN || CONFIG_IS_AUTOSPLITSCREEN) )
        {
            SetState( SM_INEVITABLE_INTRO );
        }
    }
    */
}

void StateMachine::exitESRBNotice()
{
    //uiManager->endDialog( g_UiUserID, TRUE );
}
