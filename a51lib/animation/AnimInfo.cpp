
#include "animData.h"


void AnimInfo::GetRawKey(int iFrame, int iBone, AnimKey& Key) const
{
    iFrame = iFrame % nFrames;
    animKeys.GetRawKey(*parentGroup, iFrame, iBone, Key);
}

void AnimInfo::GetInterpKey(float Frame, int iBone, AnimKey& Key) const
{
    Frame = fmod(Frame, (float)(nFrames - 1));
    animKeys.GetInterpKey(*parentGroup, Frame, iBone, Key);
}

void AnimInfo::GetRawKeys(int iFrame, AnimKey* pKey) const
{
    iFrame = iFrame % nFrames;
    animKeys.GetRawKeys(*parentGroup, iFrame, pKey);
}

void AnimInfo::GetInterpKeys(float Frame, AnimKey* pKey) const
{
    Frame = fmod(Frame, (float)(nFrames - 1));
    animKeys.GetInterpKeys(*parentGroup, Frame, pKey);
}

void AnimInfo::GetInterpKeys(float Frame, AnimKey* pKey, int nBones) const
{
    Frame = fmod(Frame, (float)(nFrames - 1));
    animKeys.GetInterpKeys(*parentGroup, Frame, pKey, nBones);
}

int AnimInfo::GetPropChannel(const char* pChannelName) const
{
    /* IJB
    for (int i = 0; i < nProps; i++) {
        if (x_stricmp(parentGroup->m_pProp[iProp + i].m_Type, pChannelName) == 0) {
            return i;
        }
    }
*/
    return -1;
}

void AnimInfo::GetPropRawKey(int iChannel, int iFrame, AnimKey& Key) const
{
    iFrame = iFrame % nFrames;
    assert(iChannel >= 0);
    assert(iChannel < nProps);
    animKeys.GetRawKey(*parentGroup, iFrame, parentGroup->numBones + iChannel, Key);
    return;
}

void AnimInfo::GetPropInterpKey(int iChannel, float Frame, AnimKey& Key) const
{
    Frame = fmod(Frame, (float)(nFrames - 1));
    assert(iChannel >= 0);
    assert(iChannel < nProps);
    animKeys.GetInterpKey(*parentGroup, Frame, parentGroup->numBones + iChannel, Key);
    return;
}

float AnimInfo::GetSpeed() const
{
    float Dist = totalTranslation.Length();
    float Time = (float)(nFrames) / (float)fps;
    return Dist / Time;
}

float AnimInfo::GetYawRate() const
{
    Radian Yaw = GetTotalYaw();
    float  Time = (float)(nFrames) / (float)fps;
    return Yaw / Time;
}

bool AnimInfo::IsEventActive(int eventIdx, float Frame) const
{
    assert((eventIdx >= 0) && (eventIdx < nEvents));
    const anim_event& E = parentGroup->events[iEvent + eventIdx];

    if ((Frame >= E.StartFrame()) && (Frame <= E.EndFrame())) {
        return true;
    }

    return false;
}

bool AnimInfo::IsEventTypeActive(int Type, float Frame) const
{
    for (int i = 0; i < nEvents; i++) {
        if (parentGroup->events[iEvent + i].GetInt(anim_event::INT_IDX_OLD_TYPE) == Type) {
            const anim_event& E = parentGroup->events[iEvent + i];

            if ((Frame >= E.StartFrame()) && (Frame <= E.EndFrame())) {
                return true;
            }
        }
    }

    return false;
}

// *INEV* *SB* - Added for A51
bool AnimInfo::IsEventActive(int eventIdx, float CurrFrame, float PrevFrame) const
{
    assert((eventIdx >= 0) && (eventIdx < nEvents));
    const anim_event& E = parentGroup->events[iEvent + eventIdx];

    // For debugging
    int EndFrame = E.EndFrame();
    int StartFrame = E.StartFrame();

    if (EndFrame == (StartFrame + 1)) {
        // This is considered a single-shot and should only be active
        // if we are crossing over the start frame
        if (PrevFrame <= CurrFrame) {
            if ((CurrFrame > StartFrame) && (PrevFrame <= StartFrame)) {
                return true;
            }
        } else if (PrevFrame > CurrFrame) {
            float P, C;

            P = PrevFrame;
            C = (float)nFrames - 1.0f;
            if ((C >= StartFrame) && (P < StartFrame)) {
                return true;
            }

            P = 0;
            C = CurrFrame;
            if ((C >= StartFrame) && (P < StartFrame)) {
                return true;
            }
        } else {
            if (CurrFrame == StartFrame) {
                return true;
            }
        }
    } else {
        // This is considered a duration event and should be active
        // if we are overlapping any part of the event time frame

        if (PrevFrame > CurrFrame) {
            float P, C;

            P = PrevFrame;
            C = (float)nFrames - 1.0f;
            if (!((C < StartFrame) || (P > EndFrame))) {
                return true;
            }

            P = 0;
            C = CurrFrame;
            if (!((C < StartFrame) || (P > EndFrame))) {
                return true;
            }
        } else {
            if (!((CurrFrame < StartFrame) || (PrevFrame > EndFrame))) {
                return true;
            }
        }
    }

    return false;
}

bool AnimInfo::IsEventTypeActive(int Type, float CurrFrame, float PrevFrame) const
{
    for (int i = 0; i < nEvents; i++) {
        if (parentGroup->events[iEvent + i].GetInt(anim_event::INT_IDX_OLD_TYPE) == Type) {
            const anim_event& E = parentGroup->events[iEvent + i];

            // For debugging
            int EndFrame = E.EndFrame() - 1;
            int StartFrame = E.StartFrame();

            // Frames overlapping event?
            if (!((PrevFrame >= EndFrame) || (CurrFrame < StartFrame))) {
                return true;
            }
        }
    }

    return false;
}

float AnimInfo::FindLipSyncEventStartFrame() const
{
    // Check all events
    for (int i = 0; i < GetNEvents(); i++) {
        // Lookup event info
        const anim_event& Event = GetEvent(i);
        const char*       pType = Event.GetType();

        // Is this a generic event?
        if (strcmp(pType, "Generic") == 0) {
            // Get generic type
            const char* pGenericType = Event.GetString(anim_event::STRING_IDX_GENERIC_TYPE);

            // Is this a lip sync start event?
            if (strcmp(pGenericType, "Lip-Sync Start") == 0) {
                // Found!
                return (float)Event.StartFrame();
            }
        }
    }

    // Not found
    return -1.0f;
}
