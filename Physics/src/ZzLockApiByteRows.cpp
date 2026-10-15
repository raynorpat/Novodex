#include "ObjectModel.h"

#include <cstring>
#include <windows.h>

// Keep these recovered leaf rows in this final-sort translation unit so they
// don't move PhysicsSDK.obj's link-sensitive NxCreatePhysicsSDK RVA.
// phys_fn_002383 (0x5b8b0): store byte 1 through [this+4] at +8.
void nxLockApiSetByte2383(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* inner;
	memcpy(&inner, p + 4, 4);
	inner[8] = 1;
	}

// phys_fn_002387 (0x5b8e0): return byte +8 through [this+4].
unsigned char nxLockApiGetByte2387(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned char* inner;
	memcpy(&inner, p + 4, 4);
	return inner[8];
	}

// phys_fn_002389 (0x5b8f0): store state 2 at [[this+4]+4], then end
// the worker thread with exit code zero.
void __fastcall nxLockApiThreadExit2389(void* self)
	{
	unsigned char* p = reinterpret_cast<unsigned char*>(self);
	unsigned object;
	memcpy(&object, p + 4, 4);
	unsigned char* state = reinterpret_cast<unsigned char*>(static_cast<size_t>(object));
	unsigned value = 2u;
	memcpy(state + 4, &value, 4);
	::ExitThread(0u);
	}
