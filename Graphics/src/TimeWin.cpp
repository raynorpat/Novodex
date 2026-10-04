// Recovered QPC semantics from TimeWin.obj; initialized safely before first sample.
#include "SysTime.h"
#ifdef WIN32
clWinTime::clWinTime():lstTimeS(GetTimeTicks()) {}
clWinTime::~clWinTime() {}
void clWinTime::Sleep(milisec milliseconds) { ::Sleep(milliseconds); }
second clWinTime::GetElapsedSeconds() {
    const double now=GetTimeTicks(), elapsed=(now-lstTimeS)/GetClockFrequency();
    lstTimeS=now; return elapsed;
}
second clWinTime::PeekElapsedSeconds() { return (GetTimeTicks()-lstTimeS)/GetClockFrequency(); }
double clWinTime::GetTimeTicks() {
    LARGE_INTEGER count; QueryPerformanceCounter(&count); return double(count.QuadPart);
}
second clWinTime::GetClockFrequency() {
    static const double frequency=[]() {
        LARGE_INTEGER value; QueryPerformanceFrequency(&value); return double(value.QuadPart);
    }();
    return frequency;
}
#endif
