#include "ObjectModel.h"

#include <cstring>
#include <new>
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

// phys_fn_002392 (0x5b920): destroy a worker record. Install its final
// vptr, terminate a still-running worker and mark it stopped, close the
// thread handle, release the record through the Foundation allocator, then
// clear the owner pointer. Keep this leaf in the final-sort TU.
void __fastcall nxLockApiWorkerDestroy2392(void* self)
	{
	unsigned char* owner = static_cast<unsigned char*>(self);
	void* vtable = reinterpret_cast<void*>(static_cast<size_t>(0x1010886cu));
	memcpy(owner, &vtable, 4);
	unsigned recordWord = 0;
	memcpy(&recordWord, owner + 4, 4);
	unsigned char* record = reinterpret_cast<unsigned char*>(
		static_cast<size_t>(recordWord));
	unsigned threadWord = 0, state = 0;
	memcpy(&threadWord, record, 4);
	memcpy(&state, record + 4, 4);
	if(state == 1u)
		{
		::TerminateThread(reinterpret_cast<HANDLE>(
			static_cast<size_t>(threadWord)), 0u);
		state = 2u;
		memcpy(record + 4, &state, 4);
		}
	::CloseHandle(reinterpret_cast<HANDLE>(static_cast<size_t>(threadWord)));
	if(recordWord)
		{
		nxFoundationSDKAllocator->free(record);
		unsigned cleared = 0;
		memcpy(owner + 4, &cleared, 4);
		}
	}

// phys_fn_002394 (0x5b980): scalar deleting destructor for the worker owner.
// The low flag bit controls the CRT delete; the base destruction always runs.
void* __fastcall nxLockApiWorkerScalarDelete2394(void* self, void*, unsigned flags)
	{
	nxLockApiWorkerDestroy2392(self);
	if(flags & 1u)
		::operator delete(self);
	return self;
	}

// phys_fn_002377 (0x5b820): construct the base worker owner and zero its record.
void* __fastcall nxLockApiWorkerBaseConstruct2377(void* self, void*)
	{
	unsigned char* owner = static_cast<unsigned char*>(self);
	void* vtable = reinterpret_cast<void*>(static_cast<size_t>(0x1010886cu));
	memcpy(owner, &vtable, 4);
	unsigned char* record = static_cast<unsigned char*>(
		nxFoundationSDKAllocator->malloc(0x14, NX_MEMORY_PERSISTENT));
	memcpy(owner + 4, &record, 4);
	unsigned zero = 0;
	memcpy(record, &zero, 4);
	memcpy(record + 4, &zero, 4);
	record[8] = 0;
	return self;
	}
