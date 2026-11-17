#pragma once

#include "../../Property.h"
#include "../../PropertyEnum.h"
#include "../../objects/triggerEx/TriggerExActions.h"
#include "../../objects/triggerEx/actions/action_ai_base.h"

#include "../ResponseList.h"

#define MAX_SUB_TASK_ARRAY_SIZE     48

#undef BIT
#define BIT(x) ( 1<<(x) )

class character_task : public prop_interface
{
public:

    enum handle_commands
    {
        INVALID_HANDLE_COMMAND = -1,
 
        HANDLE_COMMAND_RETRY_AT_CURRENT,            //0
        HANDLE_COMMAND_RETRY_AT_TOP,                //1
        HANDLE_COMMAND_REASSIGN_AT_CURRENT,         //2
        HANDLE_COMMAND_REASSIGN_AT_TOP,             //3
        HANDLE_COMMAND_DELETE_TASK,                 //4
        HANDLE_COMMAND_MOVE_TO_NEXT_TASK,
        HANDLE_COMMAND_DO_NOTHING,

        HANDLE_COMMAND_END
    };

    enum task_flags
    {
        TF_NULL                         =      0,
        TF_IGNORE_ATTACKS               = BIT( 1),  
        TF_IGNORE_SIGHT                 = BIT( 2),  
        TF_IGNORE_SOUND                 = BIT( 3),  
        TF_IGNORE_ALERTS                = BIT( 4),  
        TF_INVINCIBLE                   = BIT( 5)
    };
    

                                    character_task          (  );
                                    ~character_task         (  );
                            
	virtual			void	        OnEnumProp		        ( prop_enum& rList );
	virtual			bool	        OnProperty		        ( prop_query& rPropQuery );  

                    void            AddSubtask              ( actions_ex_base::action_ex_types Type );
                    void            RemoveSubTask           ( action_ai_base* pSubTask );           
                    void            MoveSubTaskUp           ( action_ai_base* pSubTask );                 
                    void            MoveSubTaskDown         ( action_ai_base* pSubTask );  
                    int             GetSubTaskIndex         ( action_ai_base* pSubTask );
                    void            SetSubTaskIndex         ( action_ai_base* pAction, int Index );
                    
                    uint32_t             GetTaskFlags            (  ) { return m_ResponseList.GetFlags(); }
                    guid            GetSetGuid              (  ) { return m_SetGuid;   }
    
	virtual		    bool           ExecuteTask		        ( float DeltaTime );
	virtual		    void            OnTaskItemCompleted     (  );
                    void            OnTaskFailure           (  );
                    bool           OnTaskInterrupt         (  ); //return true if we should continue task after interrupt
                    
    virtual         void            OnRender                ( int TaskId );
                    
public:

    static      character_task*     CreateTask              ( guid SetGuid );

    handle_commands                      m_OnInterrupt;
    handle_commands                      m_OnFailure;
    actions_ex_base::action_ex_types     m_SubTasksChooser;

    guid                                 m_ActivateOnSuccess;
    guid                                 m_ActivateOnInterrupt;
    guid                                 m_ActivateOnFailure;

    actions_ex_base*                     m_SubTask[MAX_SUB_TASK_ARRAY_SIZE];
    int                                  m_SubTaskCount;
    int                                  m_SubCurrentTaskIndex;

protected:

    guid        m_SetGuid;
    response_list m_ResponseList;
    

    static      enum_table<handle_commands>     m_HandleOnInterrupt; 
    static      enum_table<handle_commands>     m_HandleOnFailure; 
};

