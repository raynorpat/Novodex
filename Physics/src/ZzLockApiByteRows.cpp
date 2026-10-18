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

// phys_fn_002379 (0x5b860): worker entrypoint invokes virtual slot 1 on self.
unsigned long __stdcall nxLockApiWorkerStart2379(void* self)
	{
	typedef void (__thiscall* WorkerCallback)(void*);
	void** vtable = *reinterpret_cast<void***>(self);
	reinterpret_cast<WorkerCallback>(vtable[1])(self);
	return 0;
	}

// phys_fn_002381 (0x5b870): create the worker only while the record is idle.
void* NxLockApiWorkerStarter2381::start()
	{
	unsigned char* owner = reinterpret_cast<unsigned char*>(this);
	unsigned char* record = 0;
	memcpy(&record, owner + 4, sizeof(record));
	unsigned state = 0;
	memcpy(&state, record + 4, sizeof(state));
	if(state == 0)
		{
		HANDLE thread = ::CreateThread(0, 0, &nxLockApiWorkerStart2379, this, 0, 0);
		memcpy(record, &thread, sizeof(thread));
		state = 1;
		memcpy(record + 4, &state, sizeof(state));
		return thread;
		}
	return record;
	}

// phys_fn_002398 (0x5b9d0, ret 4): store the stack argument at +0x10 and return it.
unsigned NxLockApiWordStore2398::store(unsigned value)
	{
	memcpy(reinterpret_cast<unsigned char*>(this) + 0x10, &value, sizeof(value));
	return value;
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

// phys_fn_002396 (0x5b9a0): construct the derived worker owner with its three fields.
void* __fastcall nxLockApiWorkerConstruct2396(
	void* self, void*, unsigned arg0, unsigned arg1, unsigned arg2)
	{
	nxLockApiWorkerBaseConstruct2377(self, nullptr);
	unsigned char* owner = static_cast<unsigned char*>(self);
	memcpy(owner + 8, &arg0, 4);
	void* vtable = reinterpret_cast<void*>(static_cast<size_t>(0x10108894u));
	memcpy(owner, &vtable, 4);
	memcpy(owner + 0xc, &arg1, 4);
	memcpy(owner + 0x14, &arg2, 4);
	return self;
	}
