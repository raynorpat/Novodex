/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "FPSCounter.h"

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX				//suppress windows' global min,max macros.
#include <windows.h>
#include <mmsystem.h>

#elif LINUX
#include <sys/time.h>
#include "SysTime.h"
#else
	#error TODO: adapt this for Linux
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FPSCounter::FPSCounter()
	{
	mLastTime	= 0.0f;
	mFrames		= 0;
	mLastTime2	= 0.0f;
	mFrames2	= 0;
	mFPS		= 0.0f;
	mInstantFPS	= 0.0f;
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

FPSCounter::~FPSCounter()
	{
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void FPSCounter::Update()
	{
	// Keep track of the time laps and frame count
#ifdef WIN32
	NxF32 Time = timeGetTime() * 0.001f; // Get current time in seconds
#elif LINUX
	static struct timeval _tv;
	static struct timezone _tz;
	gettimeofday(&_tv, &_tz);
	NxF32 Time = (double)_tv.tv_sec + (double)_tv.tv_usec/(1000000);
#endif
	mFrames++;
	mFrames2++;

	// Instant frame rate
	NxF32 Delta = Time - mLastTime2;
	if(Delta > 0.01f)
		{
		mInstantFPS	= NxF32(mFrames2) / Delta;
		mLastTime2	= Time;
		mFrames2	= 0;
		}

	// Update the frame rate once per second
	Delta = Time - mLastTime;
	if(Delta > 1.0f)
		{
		mFPS		= NxF32(mFrames) / Delta;
		mLastTime	= Time;
		mFrames		= 0;
		}
	}
