/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The shipped NxPhysics.dll entry point is the stock _DllMainCRTStartup at
// 0x000f74bd (phys_fn_005807), which calls the DllMain at 0x000fe26f
// (phys_fn_006070). That DllMain is six bytes -- xor eax,eax / inc eax /
// ret 0xc -- so attach and detach do nothing beyond CRT initialisation and
// termination. In particular the SDK singleton is not torn down on
// DLL_PROCESS_DETACH; a process that never calls NxPhysicsSDK::release leaks it.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID)
	{
	return TRUE;
	}
