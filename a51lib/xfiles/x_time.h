#pragma once

#include <cstdint>

typedef int64_t xtick;

class xtimer
{
protected:
    xtick m_StartTime;
    xtick m_TotalTime;
    bool  m_Running;
    int   m_NSamples;

public:
    xtimer();

    void Start();
    void Reset();

    xtick Stop();
    float StopMs();
    float StopSec();

    xtick Read() const;
    float ReadMs() const;
    float ReadSec() const;

    xtick Trip(); // Similar to Stop, Reset, Start
    float TripMs();
    float TripSec();

    int   GetNSamples() const;
    float GetAverageMs() const;
    bool  IsRunning() const { return m_Running; }
};


xtick  x_GetTime();
double x_GetTimeSec();

float  x_TicksToMs(xtick Ticks);
double x_TicksToSec(xtick Ticks);

int64_t x_GetTicksPerMs();
int64_t x_GetTicksPerSecond();


void x_TimeInit();
void x_TimeKill();
