
#include "x_time.h"

#include <SDL3/SDL_timer.h>

#define XTICKS_PER_MS s_PCFrequency
#define XTICKS_PER_SECOND s_PCFrequency2

static double s_PCFrequency;
static double s_PCFrequency2;
static xtick  s_BaseTimeTick;

//==============================================================================

void x_TimeInit(void)
{
    s_BaseTimeTick = SDL_GetPerformanceCounter();
    uint64_t ticksPerSecond = SDL_GetPerformanceFrequency();
    s_PCFrequency = (double)ticksPerSecond / 1000.0;
    s_PCFrequency2 = (double)ticksPerSecond;
}

void x_TimeKill(void)
{
}

xtick x_GetTime(void)
{
    static xtick LastTicks = 0;
    xtick Ticks = SDL_GetPerformanceCounter() - s_BaseTimeTick;

    if (Ticks < LastTicks) {
        Ticks = LastTicks + 1;
    }

    LastTicks = Ticks;

    return Ticks;
}

#define ONE_HOUR ((int64_t)XTICKS_PER_MS * 1000 * 60 * 60)
#define ONE_DAY ((int64_t)XTICKS_PER_MS * 1000 * 60 * 60 * 24)

//
// This is so we can see these values in the debugger
//
xtick s_XTICKS_PER_MS = (xtick)XTICKS_PER_MS;
xtick s_XTICKS_PER_DAY = (xtick)ONE_DAY;
xtick s_XTICKS_PER_HOUR = (xtick)ONE_HOUR;

//==============================================================================

int64_t x_GetTicksPerMs()
{
    return ((int64_t)XTICKS_PER_MS);
}

//==============================================================================

int64_t x_GetTicksPerSecond()
{
    return ((int64_t)XTICKS_PER_SECOND);
}

//==============================================================================

float x_TicksToMs(xtick Ticks)
{
    // We do the multiple casting here to try and preserve as much accuracy as possible
    //return( (float)(     Ticks) / (float)XTICKS_PER_MS );
    return float(double(Ticks) / double(s_PCFrequency2) * 1000);
}

//==============================================================================

double x_TicksToSec(xtick Ticks)
{
    return (((double)Ticks) / ((double)(XTICKS_PER_MS * 1000)));
}

//==============================================================================

double x_GetTimeSec()
{
    return (x_TicksToSec(x_GetTime()));
}

//==============================================================================

xtimer::xtimer()
{
    m_Running = false;
    m_StartTime = 0;
    m_TotalTime = 0;
    m_NSamples = 0;
}

void xtimer::Start()
{
    if (!m_Running) {
        m_StartTime = x_GetTime();
        m_Running = true;
        m_NSamples++;
    }
}

//==============================================================================

void xtimer::Reset()
{
    m_Running = false;
    m_StartTime = 0;
    m_TotalTime = 0;
    m_NSamples = 0;
}

//==============================================================================

xtick xtimer::Stop()
{
    if (m_Running) {
        m_TotalTime += x_GetTime() - m_StartTime;
        m_Running = false;
    }
    return (m_TotalTime);
}

float xtimer::StopMs()
{
    if (m_Running) {
        m_TotalTime += x_GetTime() - m_StartTime;
        m_Running = false;
    }

    return ((float)(m_TotalTime) / (float)XTICKS_PER_MS);
}

float xtimer::StopSec()
{
    if (m_Running) {
        m_TotalTime += x_GetTime() - m_StartTime;
        m_Running = false;
    }

    return ((float)(m_TotalTime) / ((float)XTICKS_PER_MS * 1000.0f));
}

xtick xtimer::Read() const
{
    if (m_Running) {
        return (m_TotalTime + (x_GetTime() - m_StartTime));
    } else {
        return (m_TotalTime);
    }
}

float xtimer::ReadMs() const
{
    xtick Ticks;
    if (m_Running) {
        Ticks = m_TotalTime + (x_GetTime() - m_StartTime);
    } else {
        Ticks = m_TotalTime;
    }

    return ((float)(Ticks) / (float)XTICKS_PER_MS);
}

//==============================================================================

float xtimer::ReadSec() const
{
    xtick Ticks;
    if (m_Running) {
        Ticks = m_TotalTime + (x_GetTime() - m_StartTime);
    } else {
        Ticks = m_TotalTime;
    }

    return ((float)(Ticks) / ((float)XTICKS_PER_MS * 1000.0f));
}

//==============================================================================

xtick xtimer::Trip()
{
    xtick Ticks = 0;
    if (m_Running) {
        xtick Now = x_GetTime();
        Ticks = m_TotalTime + (Now - m_StartTime);
        m_TotalTime = 0;
        m_StartTime = Now;
        m_NSamples++;
    }
    return (Ticks);
}

//==============================================================================

float xtimer::TripMs()
{
    xtick Ticks = 0;
    if (m_Running) {
        xtick Now = x_GetTime();
        Ticks = m_TotalTime + (Now - m_StartTime);
        m_TotalTime = 0;
        m_StartTime = Now;
        m_NSamples++;
    }

    return ((float)(Ticks) / (float)XTICKS_PER_MS);
}

float xtimer::TripSec()
{
    xtick Ticks = 0;
    if (m_Running) {
        xtick Now = x_GetTime();
        Ticks = m_TotalTime + (Now - m_StartTime);
        m_TotalTime = 0;
        m_StartTime = Now;
        m_NSamples++;
    }

    return ((float)(Ticks) / ((float)XTICKS_PER_MS * 1000.0f));
}

int xtimer::GetNSamples() const
{
    return (m_NSamples);
}

float xtimer::GetAverageMs() const
{
    if (m_NSamples <= 0) {
        return (0);
    }

    return (ReadMs() / m_NSamples);
}
