#ifndef NX_PHYSICS_NP_SCENE_GUARD
#define NX_PHYSICS_NP_SCENE_GUARD

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#if NX_PHYSICS_USE_X87
// The public Scene and actor wrappers hold a pointer to a four-byte link;
// that link points to a 0x20-byte block containing a CRITICAL_SECTION and
// the writer flag/thread ID at +0x18/+0x1c.
inline CRITICAL_SECTION* nxNpSceneGuardBlock(void* link)
	{
	return *reinterpret_cast<CRITICAL_SECTION**>(link);
	}

inline void nxNpSceneGuardEnter(void* link)
	{
	CRITICAL_SECTION* cs = nxNpSceneGuardBlock(link);
	unsigned* state = reinterpret_cast<unsigned*>(cs) + 6;
	::EnterCriticalSection(cs);
	::InterlockedCompareExchange(reinterpret_cast<volatile long*>(state), 1, 0);
	state[1] = ::GetCurrentThreadId();
	}

inline bool nxNpSceneGuardWriteTry(void* link)
	{
	CRITICAL_SECTION* cs = nxNpSceneGuardBlock(link);
	unsigned* state = reinterpret_cast<unsigned*>(cs) + 6;
	long held = ::InterlockedCompareExchange(
		reinterpret_cast<volatile long*>(state), 1, 0);
	if(held != 0 && state[1] != static_cast<unsigned>(::GetCurrentThreadId()))
		return false;
	::EnterCriticalSection(cs);
	::InterlockedCompareExchange(reinterpret_cast<volatile long*>(state), 1, 0);
	state[1] = ::GetCurrentThreadId();
	return true;
	}

inline void nxNpSceneGuardLeave(void* link)
	{
	CRITICAL_SECTION* cs = nxNpSceneGuardBlock(link);
	unsigned* state = reinterpret_cast<unsigned*>(cs) + 6;
	::InterlockedCompareExchange(reinterpret_cast<volatile long*>(state), 0, 1);
	::LeaveCriticalSection(cs);
	}
#else
// Scalar links and their state have real class lifetimes; call the actual methods.
void nxNpSceneGuardEnter(void* link);
bool nxNpSceneGuardWriteTry(void* link);
void nxNpSceneGuardLeave(void* link);
#endif

#endif
