/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#define WIN32_LEAN_AND_MEAN
#define NOCRYPT
#define NOGDI
#define NOMCX
#define NOIME
#include "Nxf.h"
#ifdef WIN32
#include <windows.h>
#elif defined LINUX
#include <sys/time.h>
#endif
#include "NxTime.h"

namespace NxFoundation
	{

Time::Time()
	{
	GetElapsedSeconds();
	//::specialAssert(0);
	}

//hi precision timer:

second Time::GetElapsedSeconds()	//returns ms elapsed since last call.
	{
	second curTime, newTime;
	#ifdef WIN32
	newTime = GetTimeTicks();
	curTime = newTime-lstTimeS;
	lstTimeS = newTime;
	return (curTime / GetClockFrequency());
	#elif LINUX
	newTime = GetTimeSeconds();
	curTime = newTime-lstTimeS;
	lstTimeS = newTime;
	return curTime;
	#else
	#error implement GetElapsedSeconds for your OS!
	#endif
	}

second Time::PeekElapsedSeconds()
	{
	#ifdef WIN32
	return (GetTimeTicks()-lstTimeS)/GetClockFrequency();
	#elif LINUX
	second newTime;
	newTime = GetTimeSeconds();
	return newTime-lstTimeS;
	#else
	#error implement PeekElapsedSeconds for your OS!
	#endif
	}

#ifdef WIN32
second Time::GetTimeTicks()
	{
	LARGE_INTEGER a;
	QueryPerformanceCounter (&a);
	return double(a.QuadPart);
	}

second Time::GetClockFrequency()
	{
	static int haveIt=0;
	static second hz=0.0;
	if (!haveIt) 
		{
		LARGE_INTEGER a;
		QueryPerformanceFrequency (&a);
		hz = second(a.QuadPart);
		haveIt = 1;
		}
	return hz;
	}

	
#elif LINUX
second Time::GetTimeSeconds()
	{
	static struct timeval _tv;
	static struct timezone _tz;
	gettimeofday(&_tv, &_tz);
	return (double)_tv.tv_sec + (double)_tv.tv_usec/(1000*1000);
	}
#else
#endif
	}
