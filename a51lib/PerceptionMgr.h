#pragma once

class perception_mgr
{
public:
    enum perception_state
    {
        STATE_NORMAL = 0,
        STATE_BEGIN_MUTATE,
        STATE_BEGIN_MUTATE_DELAY,
        STATE_TO_MUTATE,
        STATE_MUTATED,
        STATE_END_MUTATE,
        STATE_END_MUTATE_DELAY,
        STATE_FROM_MUTATE,
        STATE_BEGIN_SHELLSHOCK,
        STATE_TO_SHELLSHOCK,
        STATE_SHELLSHOCK,
        STATE_END_SHELLSHOCK,
        STATE_FROM_SHELLSHOCK
    };
    perception_mgr();
    ~perception_mgr();
    void  Init();
    void  Kill();
    void  Update(float DeltaTime);
    void  NewState(perception_state TargetState);
    void  BeginMutate();
    void  EndMutate();
    void  BeginShellShock(float Severity);
    void  EndShellShock();
    float GetGlobalTimeDialation() { return m_GlobalTimeDialation; }
    float GetPlayerTimeDialation() { return m_PlayerTimeDialation; }
    float GetAudioTimeDialation() { return m_AudioTimeDialation; }
    float GetForwardSpeedFactor() { return m_ForwardSpeedFactor; }
    float GetTurnRateFactor() { return m_TurnRateFactor; }
    void  SetMutantTargetGlobalTimeDialation(float Target) { m_MutantTargetGlobalTimeDialation = Target; }
    void  SetMutantTargetPlayerTimeDialation(float Target) { m_MutantTargetPlayerTimeDialation = Target; }
    void  SetMutantTargetAudioTimeDialation(float Target) { m_MutantTargetAudioTimeDialation = Target; }
    void  SetMutantTargetForwardSpeedFactor(float Target) { m_MutantTargetForwardSpeedFactor = Target; }
    void  SetMutantBeginDelay(float Target) { m_BeginMutantDelay = Target; }
    void  SetMutantBeginLength(float Target) { m_BeginMutantLength = Target; }
    void  SetMutantEndDelay(float Target) { m_EndMutantDelay = Target; }
    void  SetMutantEndLength(float Target) { m_EndMutantLength = Target; }

    void SetTriggerTargetGlobalTimeDialation(float Target) { m_TriggerTargetGlobalTimeDialation = Target; }
    void SetTriggerTargetAudioTimeDialation(float Target) { m_TriggerTargetAudioTimeDialation = Target; }
    void SetTriggerTargetForwardSpeedFactor(float Target) { m_TriggerTargetForwardSpeedFactor = Target; }
    void SetTriggerTargetTurnRateFactor(float Target) { m_TriggerTargetTurnRateFactor = Target; }
    void SetTriggerBeginLength(float Target) { m_TriggerBeginLength = Target; }
    void SetTriggerEndLength(float Target) { m_TriggerEndLength = Target; }
    void BeginTriggerPerception() {}
    void EndTriggerPerception() {}

private:
    perception_state m_State;

    float m_GlobalTimeDialation;
    float m_PlayerTimeDialation;
    float m_AudioTimeDialation;
    float m_ForwardSpeedFactor;
    float m_TurnRateFactor;

    float m_MutantTargetGlobalTimeDialation;
    float m_MutantTargetPlayerTimeDialation;
    float m_MutantTargetAudioTimeDialation;
    float m_MutantTargetForwardSpeedFactor;

    float m_TriggerTargetGlobalTimeDialation;
    float m_TriggerTargetAudioTimeDialation;
    float m_TriggerTargetForwardSpeedFactor;
    float m_TriggerTargetTurnRateFactor;
    float m_TriggerBeginLength;
    float m_TriggerEndLength;

    float m_Timer;
    float m_BeginMutantDelay;
    float m_BeginMutantLength;
    float m_EndMutantDelay;
    float m_EndMutantLength;
};

extern perception_mgr g_PerceptionMgr;
