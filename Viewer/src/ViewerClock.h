#ifndef NOVODEX_VIEWER_CLOCK_H
#define NOVODEX_VIEWER_CLOCK_H
#include <chrono>
#include "Smart.h"

class ViewerClock
{
    typedef std::chrono::steady_clock Clock;
    Clock::time_point previous;
public:
    ViewerClock() : previous(Clock::now()) {}
    second GetElapsedSeconds()
    {
        const Clock::time_point now = Clock::now();
        const second elapsed = std::chrono::duration<second>(now - previous).count();
        previous = now;
        return elapsed;
    }
};
#endif
