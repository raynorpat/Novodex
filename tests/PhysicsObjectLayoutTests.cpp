// The Phase 5 object-layout gate, and why it is RED.
//
// Task 1 locks the object model BEFORE behaviour is reconstructed. This
// harness pins the structural facts the model claims, against the shipped
// DLL, so that Tasks 2 and 3 write layouts that already have something
// failing for them:
//
//   * VTABLE IDENTITY. For each table the model names -- the two actor
//     tables in full, and a twelve-slot window of every shape final plus the
//     base-shape table -- the loaded oracle's slot words are folded into one
//     digest. A slot order that moved would move the digest; nothing on the
//     candidate side can produce these words.
//   * THE COLLISION OBJECT. The oracle's own constructor for the 0x1c-byte
//     object Shape+0x9c points at (phys_fn_001193) is called on a poisoned
//     buffer with a marked argument, and the resulting bytes are folded and
//     printed field by field: three vtables, the zeroed word, and the
//     argument stored twice. That pins the borrowed layout byte for byte.
//   * THE OWNER ACCESSOR. phys_fn_001281 is four bytes -- mov eax,[ecx+4];
//     ret -- and is driven against a fake shape whose +0x04 carries a mark.
//
// The candidate side answers CANDIDATE-MISSING per family until Task 2/3
// transcribe the constructors; the compile-time half of the lock lives in
// docs/reconstruction/novodex-physics/object_model.json and in the static
// asserts Tasks 2 and 3 will carry in their headers.

#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// The reconstruction under test (Phase 5 Task 1/2 rows).
#include "ObjectModel.h"
// nxSetSdkAllocatorBridge: the candidate's growth arms must allocate from
// the same emulator arena the oracle's shim serves (see NxTestArenaAllocator).
#include "PhysicsInternal.h"

// ---------------------------------------------------------------------------
// Addresses, all censused and image-relative like every other address in this
// programme.

struct NxTableSpec
	{
	const char* name;
	unsigned rva;			// image-relative, like every other address here
	unsigned slots;
	unsigned expectSlotsPrinted;
	};

// Twelve-slot windows for the shape finals: each stays inside the extent the
// neighbouring known tables bound, so the window is slot semantics rather
// than arbitrary bytes. The two actor tables are folded in FULL -- their
// extents are exact, and the dynamic table's includes the adjacent one-slot
// member table the ctor overwrites into the +8 subobject.
static const NxTableSpec nxTables[] =
	{
	{ "actor_interface", 0x001043d0, 87, 87 },
	{ "actor_dynamic",   0x00104530, 88, 88 },
	{ "shape_base",      0x00107494, 12, 12 },
	{ "box",             0x00106ab8, 12, 12 },
	{ "capsule",         0x00106b20, 12, 12 },
	{ "plane",           0x00107430, 12, 12 },
	{ "sphere",          0x00107528, 12, 12 },
	{ "mesh",            0x00107630, 12, 12 },
	};

static const unsigned kColObjCtorRva = 0x000247c0;	// phys_fn_001193, 57 bytes
static const unsigned kOwnerAccessorRva = 0x000257a0;	// phys_fn_001281, mov eax,[ecx+4]; ret

typedef void (__thiscall* NxShapeCtorFn)(void* self, void* owner, unsigned argument);
typedef void (__thiscall* NxBoxDtorFn)(void* self, unsigned flags);

static unsigned gDtorFaultCode;
static unsigned gDtorFaultAddr;

// The Foundation lock API is reached through four globals in the oracle image
// that hold placeholder RVAs at file time. Binding them to no-op stubs lets
// the rows that bracket their bodies with the lock pair be driven; the locks
// have no effect on those rows' outputs. The call sites push arguments and
// never clean them, so the slots are __stdcall, not __cdecl.
extern "C" int __stdcall nxLockStub1(void*) { return 1; }
extern "C" int __stdcall nxLockStub3(void*, int, int) { return 1; }
extern "C" int __stdcall nxLockStubQuery() { return 0x2222; }
// The 004886 callback slot holds a non-code sentinel in the image; binding it
// to a no-op makes the 004886-calling rows drivable. The call site pushes two
// arguments and cleans them itself, so this is __cdecl.
extern "C" int __cdecl nxCallbackStub2(void*, void*) { return 0; }

// The mutex-family work arms dispatch through the object's own vtable slot
// +0x38. The fixture supplies a table whose slot points here, so the oracle
// and the candidate both reach the same body.
static unsigned gSlot38Hits;
// The call site pushes the argument and does not clean it, so the slot is a
// __stdcall one-argument function; `this` arrives in ecx and is unused here.
static void __stdcall nxSlot38Stub(void*) { ++gSlot38Hits; }

// The constant-argument thunks dispatch to vtable slot +0x4c with one value.
static unsigned gVtConstArg;
static unsigned gVtConstHits;
static void __stdcall nxVtConstStub(unsigned a) { gVtConstArg = a; ++gVtConstHits; }

// Recorders for the multi-argument dispatch thunks. The argument counts differ
// per row and so does who pops them, so each gets a stub with its own
// convention.
static unsigned gVtRec[4];
static unsigned gVtRecN;
static void __cdecl nxRec1C(unsigned a) { gVtRec[0] = a; gVtRecN = 1; }
static void __stdcall nxRec2S(unsigned a, unsigned b) { gVtRec[0] = a; gVtRec[1] = b; gVtRecN = 2; }
static void __stdcall nxRec3S(unsigned a, unsigned b, unsigned c)
	{ gVtRec[0] = a; gVtRec[1] = b; gVtRec[2] = c; gVtRecN = 3; }

// Slots reached with `this` in ecx and no stack arguments: __fastcall with one
// parameter is exactly that layout.
static unsigned __fastcall nxRecThis44(void* self)
	{ gVtRec[0] = static_cast<unsigned>(reinterpret_cast<size_t>(self)); gVtRecN = 1; return 0x5A5A0000u; }
static float __fastcall nxRecThis3c(void* self)
	{ gVtRec[0] = static_cast<unsigned>(reinterpret_cast<size_t>(self)); gVtRecN = 1; return 2.75f; }

// The same "this in ecx, nothing on the stack" layout for a second slot.
static unsigned __fastcall nxRecThis30(void* self)
	{ gVtRec[0] = static_cast<unsigned>(reinterpret_cast<size_t>(self)); gVtRecN = 1; return 0x30300000u; }
// Slot +0x24 takes one stack argument in addition to `this`.
static unsigned __stdcall nxRecArg24(void* arg)
	{ gVtRec[0] = static_cast<unsigned>(reinterpret_cast<size_t>(arg)); ++gVtRecN; return 0; }

// Report-once dispatch slots take one or two fixed arguments.
static void __stdcall nxRecOnce1(unsigned a) { gVtRec[0] = a; gVtRecN = 1; }
static void __stdcall nxRecOnce2(unsigned a, unsigned b)
	{ gVtRec[0] = a; gVtRec[1] = b; gVtRecN = 2; }

// The per-element loop passes one stack argument and leaves `this` in ecx; the
// stub records the argument and the call count.
static unsigned gLoopArg;
static unsigned gLoopHits;
static void __stdcall nxLoopStub(void* arg)
	{ gLoopArg = static_cast<unsigned>(reinterpret_cast<size_t>(arg)); ++gLoopHits; }

// The deleting destructor frees through the allocator singleton at
// [0x101041bc]: the slot is reached as [[[0x101041bc]][0]+0x14].
static unsigned gFreeHits;
static void* gFreeArg;
// the call site pushes the block and does not clean it, so the slot is
// a __stdcall one-argument function
static void __stdcall nxFreeRecorder(void* block) { gFreeArg = block; ++gFreeHits; }

// The 003938 global slot [0x10104194] takes no arguments.
static unsigned gGlobalHits;
static void __cdecl nxGlobalRecorder(void) { ++gGlobalHits; }

struct NxGlobalSaved { void* slot; void* page; DWORD prot; int ok; };

static NxGlobalSaved nxBindGlobalSlot(const void* imageBase)
	{
	NxGlobalSaved sv;
	unsigned char* img = const_cast<unsigned char*>(
		reinterpret_cast<const unsigned char*>(imageBase));
	void** slot = reinterpret_cast<void**>(img + 0x104194);
	sv.slot = *slot;
	sv.page = reinterpret_cast<void*>(
		reinterpret_cast<size_t>(img + 0x104000) & ~static_cast<size_t>(0xFFF));
	sv.prot = 0; sv.ok = 0;
	if(!VirtualProtect(sv.page, 0x2000, PAGE_READWRITE, &sv.prot))
		return sv;
	sv.ok = 1;
	*slot = reinterpret_cast<void*>(&nxGlobalRecorder);
	return sv;
	}

static void nxUnbindGlobalSlot(const void* imageBase, const NxGlobalSaved& sv)
	{
	if(!sv.ok)
		return;
	unsigned char* img = const_cast<unsigned char*>(
		reinterpret_cast<const unsigned char*>(imageBase));
	DWORD t = 0;
	VirtualProtect(sv.page, 0x2000, PAGE_READWRITE, &t);
	*reinterpret_cast<void**>(img + 0x104194) = sv.slot;
	VirtualProtect(sv.page, 0x2000, sv.prot, &t);
	}

struct NxAllocSaved { void* slot; void* page; DWORD prot; int ok; };

static NxAllocSaved nxBindAllocSlot(const void* imageBase, void* holder)
	{
	NxAllocSaved sv;
	unsigned char* img = const_cast<unsigned char*>(
		reinterpret_cast<const unsigned char*>(imageBase));
	void** slot = reinterpret_cast<void**>(img + 0x1041bc);
	sv.slot = *slot;
	sv.page = reinterpret_cast<void*>(
		reinterpret_cast<size_t>(img + 0x104000) & ~static_cast<size_t>(0xFFF));
	sv.prot = 0; sv.ok = 0;
	if(!VirtualProtect(sv.page, 0x2000, PAGE_READWRITE, &sv.prot))
		return sv;
	sv.ok = 1;
	*slot = holder;
	return sv;
	}

static void nxUnbindAllocSlot(const void* imageBase, const NxAllocSaved& sv)
	{
	if(!sv.ok)
		return;
	unsigned char* img = const_cast<unsigned char*>(
		reinterpret_cast<const unsigned char*>(imageBase));
	DWORD t = 0;
	VirtualProtect(sv.page, 0x2000, PAGE_READWRITE, &t);
	*reinterpret_cast<void**>(img + 0x1041bc) = sv.slot;
	VirtualProtect(sv.page, 0x2000, sv.prot, &t);
	}

// Bind the 004886 callback slot [0x10128478] to the stub above.
struct NxCallbackSaved { void* slot; void* page; DWORD prot; int ok; };

static NxCallbackSaved nxBindCallbackSlot(const void* imageBase)
	{
	NxCallbackSaved sv;
	unsigned char* img = const_cast<unsigned char*>(
		reinterpret_cast<const unsigned char*>(imageBase));
	void** slot = reinterpret_cast<void**>(img + 0x128478);
	sv.slot = *slot;
	sv.page = reinterpret_cast<void*>(
		reinterpret_cast<size_t>(img + 0x128000) & ~static_cast<size_t>(0xFFF));
	sv.prot = 0; sv.ok = 0;
	if(!VirtualProtect(sv.page, 0x2000, PAGE_READWRITE, &sv.prot))
		return sv;
	sv.ok = 1;
	*slot = reinterpret_cast<void*>(&nxCallbackStub2);
	return sv;
	}

// The assert report slot [0x101041b4] also holds a placeholder. The recorder
// is __cdecl -- the call sites push five arguments and do `add esp, 0x14`.
static unsigned gRepCap[5];
static unsigned gRepCount;

static void __cdecl nxReportRecorder(unsigned a1, unsigned a2, unsigned a3,
	unsigned a4, unsigned a5)
	{
	gRepCap[0] = a1; gRepCap[1] = a2; gRepCap[2] = a3; gRepCap[3] = a4; gRepCap[4] = a5;
	++gRepCount;
	}

extern "C" void __cdecl nxReportStub5(unsigned a1, unsigned a2, unsigned a3,
	unsigned a4, unsigned a5)
	{
	nxReportRecorder(a1, a2, a3, a4, a5);
	}

// The guarded rows dereference [0x101041b0] first; that slot holds an
// unrelocated RVA in the image, so it must be pointed at real memory whose
// first word is non-zero or the row's int3 path is taken.
static unsigned gAssertGuard = 1u;

struct NxReportSaved { void* slot; void* guard; void* page; DWORD prot; int ok; };

static NxReportSaved nxBindReportSlot(const void* imageBase)
	{
	NxReportSaved sv;
	unsigned char* img = const_cast<unsigned char*>(
		reinterpret_cast<const unsigned char*>(imageBase));
	void** slot = reinterpret_cast<void**>(img + 0x1041b4);
	void** guard = reinterpret_cast<void**>(img + 0x1041b0);
	sv.slot = *slot;
	sv.guard = *guard;
	sv.page = reinterpret_cast<void*>(
		reinterpret_cast<size_t>(img + 0x104000) & ~static_cast<size_t>(0xFFF));
	sv.prot = 0; sv.ok = 0;
	if(!VirtualProtect(sv.page, 0x2000, PAGE_READWRITE, &sv.prot))
		return sv;
	sv.ok = 1;
	*slot = reinterpret_cast<void*>(&nxReportStub5);
	*guard = reinterpret_cast<void*>(&gAssertGuard);
	return sv;
	}

// The 003708 gate byte lives at [0x101263ac]; binding it lets the report arm
// be driven as well as the shipped work arm.
struct NxGateSaved { void* slot; void* page; DWORD prot; int ok; };

static NxGateSaved nxBindGateSlot(const void* imageBase, unsigned char value)
	{
	NxGateSaved sv;
	unsigned char* img = const_cast<unsigned char*>(
		reinterpret_cast<const unsigned char*>(imageBase));
	unsigned char* slot = img + 0x1263ac;
	sv.slot = reinterpret_cast<void*>(*slot);
	sv.page = reinterpret_cast<void*>(
		reinterpret_cast<size_t>(img + 0x126000) & ~static_cast<size_t>(0xFFF));
	sv.prot = 0; sv.ok = 0;
	if(!VirtualProtect(sv.page, 0x2000, PAGE_READWRITE, &sv.prot))
		return sv;
	sv.ok = 1;
	*slot = value;
	return sv;
	}

static void nxUnbindGateSlot(const void* imageBase, const NxGateSaved& sv)
	{
	if(!sv.ok)
		return;
	unsigned char* img = const_cast<unsigned char*>(
		reinterpret_cast<const unsigned char*>(imageBase));
	DWORD t = 0;
	VirtualProtect(sv.page, 0x2000, PAGE_READWRITE, &t);
	*reinterpret_cast<unsigned char*>(img + 0x1263ac) = static_cast<unsigned char>(
		reinterpret_cast<size_t>(sv.slot) & 0xffu);
	VirtualProtect(sv.page, 0x2000, sv.prot, &t);
	}

static void nxUnbindReportSlot(const void* imageBase, const NxReportSaved& sv)
	{
	if(!sv.ok)
		return;
	unsigned char* img = const_cast<unsigned char*>(
		reinterpret_cast<const unsigned char*>(imageBase));
	DWORD t = 0;
	VirtualProtect(sv.page, 0x2000, PAGE_READWRITE, &t);
	*reinterpret_cast<void**>(img + 0x1041b4) = sv.slot;
	*reinterpret_cast<void**>(img + 0x1041b0) = sv.guard;
	VirtualProtect(sv.page, 0x2000, sv.prot, &t);
	}

static void nxUnbindCallbackSlot(const void* imageBase, const NxCallbackSaved& sv)
	{
	if(!sv.ok)
		return;
	unsigned char* img = const_cast<unsigned char*>(
		reinterpret_cast<const unsigned char*>(imageBase));
	DWORD t = 0;
	VirtualProtect(sv.page, 0x2000, PAGE_READWRITE, &t);
	*reinterpret_cast<void**>(img + 0x128478) = sv.slot;
	VirtualProtect(sv.page, 0x2000, sv.prot, &t);
	}

// Bind the four Foundation lock-API slots in the oracle image to the no-op
// stubs above. The slots live in a read-only page, so the stores need the
// page unlocked first. Pass the saved values back to nxUnbindLockApi.
struct NxLockApiSaved { void* s10; void* s2c; void* s44; void* s14; void* page; DWORD prot; int ok; };

static NxLockApiSaved nxBindLockApi(const void* imageBase)
	{
	NxLockApiSaved sv;
	unsigned char* img = const_cast<unsigned char*>(
		reinterpret_cast<const unsigned char*>(imageBase));
	void** g10 = reinterpret_cast<void**>(img + 0x104010);
	void** g2c = reinterpret_cast<void**>(img + 0x10402c);
	void** g44 = reinterpret_cast<void**>(img + 0x104044);
	void** g14 = reinterpret_cast<void**>(img + 0x104014);
	sv.s10 = *g10; sv.s2c = *g2c; sv.s44 = *g44; sv.s14 = *g14;
	sv.page = reinterpret_cast<void*>(
		reinterpret_cast<size_t>(img + 0x104000) & ~static_cast<size_t>(0xFFF));
	sv.prot = 0; sv.ok = 0;
	if(!VirtualProtect(sv.page, 0x2000, PAGE_READWRITE, &sv.prot))
		return sv;
	sv.ok = 1;
	*g10 = reinterpret_cast<void*>(&nxLockStub1);
	*g2c = reinterpret_cast<void*>(&nxLockStub3);
	*g44 = reinterpret_cast<void*>(&nxLockStubQuery);
	*g14 = reinterpret_cast<void*>(&nxLockStub1);
	return sv;
	}

static void nxUnbindLockApi(const void* imageBase, const NxLockApiSaved& sv)
	{
	if(!sv.ok)
		return;
	unsigned char* img = const_cast<unsigned char*>(
		reinterpret_cast<const unsigned char*>(imageBase));
	DWORD t = 0;
	VirtualProtect(sv.page, 0x2000, PAGE_READWRITE, &t);
	*reinterpret_cast<void**>(img + 0x104010) = sv.s10;
	*reinterpret_cast<void**>(img + 0x10402c) = sv.s2c;
	*reinterpret_cast<void**>(img + 0x104044) = sv.s44;
	*reinterpret_cast<void**>(img + 0x104014) = sv.s14;
	VirtualProtect(sv.page, 0x2000, sv.prot, &t);
	}

static void nxGuardedBoxDtor(NxBoxDtorFn fn, void* object, unsigned flags)
	{
	gDtorFaultCode = 0;
	gDtorFaultAddr = 0;
	__try
		{
		fn(object, flags);
		}
	__except(gDtorFaultAddr = (unsigned) GetExceptionInformation()->ExceptionRecord->ExceptionAddress,
		gDtorFaultCode = GetExceptionInformation()->ExceptionRecord->ExceptionCode,
		EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}

// Runs a two-argument __thiscall constructor under SEH so a fault inside the
// oracle code reports its address instead of killing the transcript. No
// unwindable locals here.
static unsigned gFaultCode;
static unsigned gFaultAddr;

// phys_fn_001305 capture. Copy borrowed stack payloads DURING each call;
// storing their addresses cannot preserve the data after the callback.
static unsigned gRenderLine[8][7]; // start xyz, end xyz, color
static unsigned gRenderCircle[8][16]; // count, pose[12], color, radius bits, reserved
static unsigned gRenderN20;
static unsigned gRenderN38;
// Call-order stamps: the slot-3 dispatcher must invoke 001305 BEFORE the
// +0x28 descriptor draw; sequence numbers make that order assertable.
static unsigned gRenderCallSeq;
static unsigned gSeqLast20;
static unsigned gSeqLast38;
static unsigned gSeqN28First;
static void __fastcall nxRenderOn20(void*, void*, const unsigned* start,
	const unsigned* end, unsigned color)
	{
	if(gRenderN20 < 8)
		{
		memcpy(gRenderLine[gRenderN20], start, 12);
		memcpy(gRenderLine[gRenderN20] + 3, end, 12);
		gRenderLine[gRenderN20][6] = color;
		}
	gSeqLast20 = ++gRenderCallSeq;
	++gRenderN20;
	}
// Five stack DWORDs, callee pops 20 bytes. Argument 4 is the raw radius
// from shape slot 10's fourth result word, NOT a second buffer pointer.
static void __fastcall nxRenderOn38(void*, void*, unsigned count,
	const unsigned* pose, unsigned color, unsigned radiusBits, unsigned reserved)
	{
	if(gRenderN38 < 8)
		{
		unsigned* row = gRenderCircle[gRenderN38];
		row[0] = count;
		memcpy(row + 1, pose, 48);
		row[13] = color;
		row[14] = radiusBits;
		row[15] = reserved;
		}
	gSeqLast38 = ++gRenderCallSeq;
	++gRenderN38;
	}
// BOX slot 3's renderer row +0x28 (index 10): the dispatcher calls it with
// (descriptor pointer, computed word, 0) after nxFillShapeDescriptor fills
// the local. Recorded during the call, before the callee's stack frame is
// reused; the callee pops its own stack dwords per thiscall.
static unsigned gRenderFill[16];
static unsigned gRenderArg2;
static unsigned gRenderArg3;
static unsigned gRenderN28;
static void __fastcall nxRenderOn28(void*, void*, const unsigned* fill,
	unsigned arg2, unsigned arg3)
	{
	if(gRenderN28 == 0)
		{
		memcpy(gRenderFill, fill, 15 * 4);
		gRenderArg2 = arg2;
		gRenderArg3 = arg3;
		gSeqN28First = ++gRenderCallSeq;
		}
	++gRenderN28;
	}
static void nxGuardedCtor(NxShapeCtorFn fn, void* object, void* owner,
	unsigned argument, unsigned& outCode, unsigned& outAddr)
	{
	gFaultCode = 0;
	gFaultAddr = 0;
	__try
		{
		fn(object, owner, argument);
		}
	__except(gFaultAddr = (unsigned) GetExceptionInformation()->ExceptionRecord->ExceptionAddress,
		gFaultCode = GetExceptionInformation()->ExceptionRecord->ExceptionCode,
		EXCEPTION_EXECUTE_HANDLER)
		{
		}
	outCode = gFaultCode;
	outAddr = gFaultAddr;
	}

// Guarded void(void*) call under SEH so a fault inside a driven row reports
// through gFaultCode instead of killing the transcript. Convention-generic:
// the function pointer's OWN type names the convention -- re-casting the
// candidate's cdecl thunk to __thiscall here (the first spelling) put the
// receiver in ecx and left the stack argument garbage.
template<class Fn>
static void nxGuardedVoidCall(Fn fn, void* object)
	{
	gFaultCode = 0;
	gFaultAddr = 0;
	__try
		{
		fn(object);
		}
	__except(gFaultAddr = (unsigned) GetExceptionInformation()->ExceptionRecord->ExceptionAddress,
		gFaultCode = GetExceptionInformation()->ExceptionRecord->ExceptionCode,
		EXCEPTION_EXECUTE_HANDLER)
		{
		}
	}

// Allocator shim, shared by every shape-ctor probe whose collision-object arm
// allocates through the SDK allocator singleton ([.data 0x101041bc] ->
// holder -> interface -> vtable slot +8). The shipped holder word is NULL
// until an NxPhysicsSDK exists (phase4-formats.md), and creating one is
// Task-4/SDK machinery this gate does not own -- so the probes install a
// minimal interface whose slot +8 hands back a fixed block. Every instruction
// of the rows under test still runs natively; only the 28 bytes come from the
// shim.
// ---------------------------------------------------------------------------
// Allocator emulator. The real SDK creation path needs genuine malloc/free
// semantics (arbitrary sizes, reuse after free) -- a flat block cannot
// serve it (evidence 3z2). First-fit free list over a 1 MiB arena, with an
// 8-byte header per block carrying the block size for free().
static unsigned char g_arena[1 << 20];
struct NxFreeEnt { unsigned off; unsigned sz; };
static NxFreeEnt g_free[2048];
static unsigned g_freeN = 0;

// Allocation-stream accounting. The growth-arm differentials fold the
// DELTA of these four counters across one drive on each side: equal deltas
// pin that both sides performed the same number of allocations of the same
// total size and released the same number of blocks of the same total size.
// Deltas -- not absolute values -- because every earlier family's traffic
// is still in these totals and differs between the drives' positions.
static unsigned g_heapMallocOps = 0;
static unsigned g_heapMallocBytes = 0;
static unsigned g_heapFreeOps = 0;
static unsigned g_heapFreeBytes = 0;

static void nxHeapInit()
	{
	g_free[0].off = 0;
	g_free[0].sz = sizeof(g_arena);
	g_freeN = 1;
	}

static void* nxHeapAlloc(unsigned size)
	{
	if(g_freeN == 0) nxHeapInit();
	unsigned need = ((size + 7u) & ~7u) + 4u;
	if(need < 4u) need = 4u;
	for(unsigned i = 0; i < g_freeN; ++i)
		if(g_free[i].sz >= need)
			{
			unsigned off = g_free[i].off;
			if(g_free[i].sz == need)
				{
				for(unsigned j = i; j + 1 < g_freeN; ++j) g_free[j] = g_free[j + 1];
				--g_freeN;
				}
			else
				{
				g_free[i].off += need;
				g_free[i].sz -= need;
				}
			*reinterpret_cast<unsigned*>(g_arena + off) = need;
			g_heapMallocOps += 1;
			g_heapMallocBytes += need;
			return g_arena + off + 4;
			}
	return nullptr;
	}

static void nxHeapFree(void* p)
	{
	if(p == nullptr) return;
	unsigned off = static_cast<unsigned>(
		reinterpret_cast<unsigned char*>(p) - 4 - g_arena);
	if(off >= sizeof(g_arena))
		{
		return;
		}
	unsigned sz = *reinterpret_cast<unsigned*>(g_arena + off);
	g_heapFreeOps += 1;
	g_heapFreeBytes += sz;
	unsigned i = 0;
	while(i < g_freeN && g_free[i].off < off) ++i;
	for(unsigned j = g_freeN; j > i; --j) g_free[j] = g_free[j - 1];
	g_free[i].off = off;
	g_free[i].sz = sz;
	++g_freeN;
	if(i + 1 < g_freeN && g_free[i].off + g_free[i].sz == g_free[i + 1].off)
		{
		g_free[i].sz += g_free[i + 1].sz;
		for(unsigned j = i + 1; j + 1 < g_freeN; ++j) g_free[j] = g_free[j + 1];
		--g_freeN;
		}
	if(i > 0 && g_free[i - 1].off + g_free[i - 1].sz == g_free[i].off)
		{
		g_free[i - 1].sz += g_free[i].sz;
		for(unsigned j = i; j + 1 < g_freeN; ++j) g_free[j] = g_free[j + 1];
		--g_freeN;
		}
	}

static bool nxInstallAllocatorShim(const unsigned char* base)
	{
	typedef void* (__fastcall* NxAllocFn)(void*, void*, unsigned, unsigned);
	typedef void* (__fastcall* NxAllocFn)(void*, void*, unsigned, unsigned);
	// Adapter vtable layout, from the shipped tables: +0x00/+0x0c/+0x14 are
	// free variants (one pushed pointer), +0x08 is malloc (two pushed args:
	// size, flags). The SdkContainer helper at 0x000b4000 uses the same
	// layout through the Foundation global at .data 0x1012845c.
	struct NxShim
		{
		static void* __fastcall freeA(void*, void*, unsigned p)
			{ nxHeapFree(reinterpret_cast<void*>(p)); return nullptr; }
		static void* __fastcall freeB(void*, void*, unsigned p)
			{ nxHeapFree(reinterpret_cast<void*>(p)); return nullptr; }
		static void* __fastcall alloc(void*, void*, unsigned size, unsigned)
			{ return nxHeapAlloc(size); }
		static void* __fastcall freeC(void*, void*, unsigned p)
			{ nxHeapFree(reinterpret_cast<void*>(p)); return nullptr; }
		static void* __fastcall freeD(void*, void*, unsigned p)
			{ nxHeapFree(reinterpret_cast<void*>(p)); return nullptr; }
		};
	static void* sTable[6];
	sTable[0] = reinterpret_cast<NxAllocFn>(&NxShim::freeA);
	sTable[2] = reinterpret_cast<NxAllocFn>(&NxShim::alloc);
	sTable[3] = reinterpret_cast<NxAllocFn>(&NxShim::freeB);
	sTable[5] = reinterpret_cast<NxAllocFn>(&NxShim::freeC);
	static void* sIface[1] = { sTable };
	unsigned holder = 0;
	memcpy(&holder, base + 0x001041bc, 4);
	if(!holder)
		return false;
	void* iface = sIface;
	memcpy((void*) holder, &iface, 4);
	return true;
	}

// The SdkContainer rows (phys_fn_004846 and the 002352 thunk into it) reach
// the allocator through the Foundation global at .data 0x1012845c -- the
// helper at 0x000b4000 returns it and defaults it to the static CRT adapter
// at 0x10122368. Repointing only the SDK holder left empty() freeing arena
// blocks through the CRT heap: the 0xc0000374 that killed the first three
// addthunk runs. Same adapter layout, same emulator.
static bool nxInstallFoundationShim(const unsigned char* base)
	{
	typedef void* (__fastcall* NxFreeFn)(void*, void*, unsigned);
	struct NxFShim
		{
		// The Foundation adapter's free slots take ONE pushed pointer --
		// 0x000b4060 is pop ecx; ret 4 -- so these pop 4, not 8. The
		// 4-parameter spelling made every oracle empty() drive drift the
		// stack by four bytes: the wild c0000005 at ee5710dc.
		static void* __fastcall freeA(void*, void*, unsigned p)
			{ nxHeapFree(reinterpret_cast<void*>(p)); return nullptr; }
		static void* __fastcall freeB(void*, void*, unsigned p)
			{ nxHeapFree(reinterpret_cast<void*>(p)); return nullptr; }
		static void* __fastcall alloc(void*, void*, unsigned size)
			{ return nxHeapAlloc(size); }
		static void* __fastcall freeC(void*, void*, unsigned p)
			{ nxHeapFree(reinterpret_cast<void*>(p)); return nullptr; }
		static void* __fastcall freeD(void*, void*, unsigned p)
			{ nxHeapFree(reinterpret_cast<void*>(p)); return nullptr; }
		};
	static NxFreeFn fTable[6];
	fTable[0] = &NxFShim::freeA;
	fTable[2] = &NxFShim::alloc;
	fTable[3] = &NxFShim::freeB;
	fTable[5] = &NxFShim::freeC;
	static void* fIface[1] = { fTable };
	void* iface = fIface;
	memcpy(const_cast<unsigned char*>(base) + 0x0012845c, &iface, 4);
	return true;
	}


typedef void (__thiscall* NxColObjCtorFn)(void* self, unsigned arg);
typedef const void* (__fastcall* NxOwnerAccessorFn)(const void* self, void* edxUnused);


// Shared seed for the support-map probes: a scaled+translated pose used as
// stand-in vertex data and as the pose argument alike.
static float nxSupportPose[16] =
	{ 2.0f, 0.0f, 0.0f,   0.0f, 3.0f, 0.0f,   0.0f, 0.0f, 4.0f,
	  1.5f, -2.5f, 0.25f };
static float nxSupportDirection[3] = { 0.5f, -1.25f, 2.0f };

// ---------------------------------------------------------------------------

static unsigned nxFold(unsigned digest, unsigned word)
	{
	digest ^= word & 0xffu;              digest *= 16777619u;
	digest ^= (word >> 8) & 0xffu;       digest *= 16777619u;
	digest ^= (word >> 16) & 0xffu;      digest *= 16777619u;
	digest ^= (word >> 24) & 0xffu;      digest *= 16777619u;
	return digest;
	}

static int nxFail(const char* message)
	{
	fprintf(stderr, "FAIL %s\n", message);
	return 1;
	}

// Allocation-stream snapshots for the growth-arm differentials: read one
// before a drive, fold the delta after, and both sides must agree on how
// many allocations of what total size -- and how many releases of what
// total size -- the drive performed.
struct NxHeapMark { unsigned mo; unsigned mb; unsigned fo; unsigned fb; };

static NxHeapMark nxHeapMarkNow()
	{
	NxHeapMark m;
	m.mo = g_heapMallocOps;
	m.mb = g_heapMallocBytes;
	m.fo = g_heapFreeOps;
	m.fb = g_heapFreeBytes;
	return m;
	}

static unsigned nxFoldHeapDelta(unsigned digest, const NxHeapMark& before)
	{
	digest = nxFold(digest, g_heapMallocOps - before.mo);
	digest = nxFold(digest, g_heapMallocBytes - before.mb);
	digest = nxFold(digest, g_heapFreeOps - before.fo);
	digest = nxFold(digest, g_heapFreeBytes - before.fb);
	return digest;
	}

// The candidate-side allocator. The transcriptions reach
// nxGetSdkAllocator(), which falls back to the CRT when nothing is
// registered -- CRT blocks do not answer for the oracle's arena and their
// traffic never touches the counters above. Registering this bridge at boot
// routes every candidate allocation through the same emulator the shim
// serves the oracle through, so both sides of a growth differential drive
// one heap.
class NxTestArenaAllocator : public SdkAllocator
	{
	public:
	void* malloc(size_t size, NxMemoryType)
		{ return nxHeapAlloc(static_cast<unsigned>(size)); }
	void* mallocDEBUG(size_t size, const char*, int, const char*, NxMemoryType)
		{ return nxHeapAlloc(static_cast<unsigned>(size)); }
	void* realloc(void* memory, size_t size)
		{
		void* fresh = nxHeapAlloc(static_cast<unsigned>(size));
		if(fresh != nullptr && memory != nullptr)
			{
			unsigned oldNeed = *reinterpret_cast<unsigned*>(
				static_cast<unsigned char*>(memory) - 4) - 4;
			memcpy(fresh, memory, oldNeed < size ? oldNeed : size);
			}
		nxHeapFree(memory);
		return fresh;
		}
	void free(void* memory)
		{ nxHeapFree(memory); }
	};
static NxTestArenaAllocator g_arenaAllocator;

// ---------------------------------------------------------------------------
// Error-stream capture, shared by the oracle and candidate errstream drives
// (they run sequentially and never overlap).

struct NxErrCap
	{
	int fired;
	int kind;
	int line;
	int code;
	char file[160];
	char msg[160];
	};

static NxErrCap g_errCap;

static void __cdecl g_errSink(int kind, const char* file, int line,
	int code, const char* message)
	{
	if(getenv("NXSINK_TRACE") != nullptr)
		printf("SINK tid=%u kind=%d line=%d code=%d file=%p msg=%.40s\n",
			static_cast<unsigned>(GetCurrentThreadId()), kind, line, code,
			file, message ? message : "(null)");
	g_errCap.fired += 1;
	g_errCap.kind = kind;
	g_errCap.line = line;
	g_errCap.code = code;
	if(file != nullptr)
		{
		size_t n = strlen(file);
		if(n >= sizeof(g_errCap.file))
			n = sizeof(g_errCap.file) - 1;
		memcpy(g_errCap.file, file, n);
		g_errCap.file[n] = 0;
		}
	if(message != nullptr)
		{
		size_t n = strlen(message);
		if(n >= sizeof(g_errCap.msg))
			n = sizeof(g_errCap.msg) - 1;
		memcpy(g_errCap.msg, message, n);
		g_errCap.msg[n] = 0;
		}
	}

static unsigned nxFoldErrCap(unsigned digest)
	{
	digest = nxFold(digest, static_cast<unsigned>(g_errCap.fired));
	digest = nxFold(digest, static_cast<unsigned>(g_errCap.kind));
	digest = nxFold(digest, static_cast<unsigned>(g_errCap.line));
	digest = nxFold(digest, static_cast<unsigned>(g_errCap.code));
	for(const char* q = g_errCap.file; *q; ++q)
		digest = nxFold(digest, static_cast<unsigned char>(*q));
	for(const char* q = g_errCap.msg; *q; ++q)
		digest = nxFold(digest, static_cast<unsigned char>(*q));
	return digest;
	}

// Slate-11 node-kill shim: the image deletes cached nodes via their own
// virtual slot 0 (__thiscall: ecx=this, push arg, callee pops) -- served by
// a fastcall free function (ecx=node, one stack param, callee pops 4).
static unsigned g_sl11KillCount;
static unsigned g_sl11KillLast;
static unsigned __fastcall NxS11KillThunk(void* ecxDummy, void*, unsigned arg)
	{
	g_sl11KillCount += 1;
	g_sl11KillLast = reinterpret_cast<unsigned>(ecxDummy);
	return 0;
	}

// phys_fn_002379: discriminate slot 1 from slot 0, receiver identity,
// exactly-once dispatch, and discarded callback results. The sentinels record
// but never dereference their receiver, so a wrong receiver fails safely.
static unsigned g_slot1Calls, g_slot0Calls, g_slot1Tag;
static void* g_slot1Receiver;
static unsigned __fastcall nxSlot0Sentinel(void* self, void*)
	{
	++g_slot0Calls;
	g_slot1Receiver = self;
	return 0xdeadbeefu;
	}
static unsigned __fastcall nxSlot1SentinelA(void* self, void*)
	{
	++g_slot1Calls;
	g_slot1Receiver = self;
	g_slot1Tag = 0x13579bdfu;
	return 0xffffffffu;
	}
static unsigned __fastcall nxSlot1SentinelB(void* self, void*)
	{
	++g_slot1Calls;
	g_slot1Receiver = self;
	g_slot1Tag = 0x2468ace0u;
	return 0;
	}

// Declaration is test-local until the existing transcription is exposed by
// the private header. No public SDK interface is changed.
bool nxVirtualSlot1Wrapper(void* arg);

struct NxSlot1Result
	{
	unsigned words[4][7];
	unsigned digest;
	unsigned failures;
	};

template<class Wrapper>
static NxSlot1Result nxDriveSlot1Wrapper(Wrapper wrapper)
	{
	unsigned vtA[] = { reinterpret_cast<unsigned>(&nxSlot0Sentinel),
		reinterpret_cast<unsigned>(&nxSlot1SentinelA) };
	unsigned vtB[] = { reinterpret_cast<unsigned>(&nxSlot0Sentinel),
		reinterpret_cast<unsigned>(&nxSlot1SentinelB) };
	unsigned objects[2][3] = {
		{ reinterpret_cast<unsigned>(vtA), 0x11223344u, 0xaabbccddu },
		{ reinterpret_cast<unsigned>(vtB), 0x55667788u, 0xeeff0011u } };
	unsigned before[2][3];
	unsigned vtBefore[2][2];
	memcpy(before, objects, sizeof(before));
	memcpy(vtBefore[0], vtA, sizeof(vtA));
	memcpy(vtBefore[1], vtB, sizeof(vtB));
	const unsigned order[] = { 0, 1, 1, 0 };
	const unsigned tags[] = { 0x13579bdfu, 0x2468ace0u,
		0x2468ace0u, 0x13579bdfu };
	NxSlot1Result result = {};
	result.digest = 2166136261u;
	for(unsigned i = 0; i < 4; ++i)
		{
		g_slot1Calls = g_slot0Calls = g_slot1Tag = 0;
		g_slot1Receiver = 0;
		unsigned* object = objects[order[i]];
		unsigned* words = result.words[i];
		words[0] = static_cast<unsigned>(wrapper(object));
		words[1] = g_slot1Calls;
		words[2] = g_slot0Calls;
		words[3] = g_slot1Receiver == object ? 1u : 0u;
		words[4] = g_slot1Tag;
		words[5] = memcmp(objects, before, sizeof(before)) == 0 ? 1u : 0u;
		words[6] = memcmp(vtA, vtBefore[0], sizeof(vtA)) == 0
			&& memcmp(vtB, vtBefore[1], sizeof(vtB)) == 0 ? 1u : 0u;
		// Explicit expectations, not just agreement between two wrong runs.
		const unsigned expected[] = { 0, 1, 0, 1, tags[i], 1, 1 };
		for(unsigned j = 0; j < 7; ++j)
			{
			result.digest = nxFold(result.digest, words[j]);
			if(words[j] != expected[j])
				++result.failures;
			}
		}
	return result;
	}

static unsigned nxTestSlot1Wrapper(const unsigned char* base, bool selfOnly,
	unsigned& oracleDigest)
	{
	// mov ecx,[esp+4]; mov eax,[ecx]; call [eax+4]; xor eax,eax; ret 4.
	typedef unsigned (__stdcall* OracleWrapper)(void*);
	NxSlot1Result oracle = nxDriveSlot1Wrapper(
		reinterpret_cast<OracleWrapper>(const_cast<unsigned char*>(base) + 0x5b860));
	oracleDigest = nxFold(oracleDigest, oracle.digest);
	printf("slot1wrapper row=oracle cases=4 failures=%u digest=%08x\n",
		oracle.failures, oracle.digest);
	if(selfOnly)
		return oracle.failures;
	NxSlot1Result candidate = nxDriveSlot1Wrapper(&nxVirtualSlot1Wrapper);
	unsigned mismatches = 0;
	for(unsigned i = 0; i < 4; ++i)
		for(unsigned j = 0; j < 7; ++j)
			if(oracle.words[i][j] != candidate.words[i][j])
				++mismatches;
	printf("slot1wrapper candidate cases=4 failures=%u mismatches=%u digest=%08x\n",
		candidate.failures, mismatches, candidate.digest);
	return oracle.failures + candidate.failures + mismatches;
	}

// phys_fn_002352: the +0x28 adjustor thunk into SdkContainer::empty
// (0x000b4f50). Three containers: an owned buffer (factor 2.0f) whose free
// must fire through the allocator, an external buffer (factor -1.0f, the
// not-owned marker) whose buffer must be kept, and a null-entries container
// (no free). The adjustor offset itself is pinned by field identity: the
// thunk receives container-0x28, so a wrong offset clears the wrong words.
struct NxAddThunkResult
	{
	unsigned words[3][7];
	unsigned digest;
	unsigned failures;
	};

template<class Thunk>
static NxAddThunkResult nxDriveAddThunk(Thunk thunk)
	{
	NxAddThunkResult result = {};
	result.digest = 2166136261u;

	// -- case 0: owned buffer.
	unsigned cOwned[4];
	memset(cOwned, 0xcd, sizeof(cOwned));
	cOwned[0] = 4;
	cOwned[1] = 3;
	unsigned* buf = static_cast<unsigned*>(nxHeapAlloc(4 * sizeof(unsigned)));
	buf[0] = 0x5a5a0001u;
	buf[1] = 0x5a5a0002u;
	buf[2] = 0x5a5a0003u;
	buf[3] = 0xcacacacau;
	cOwned[2] = reinterpret_cast<unsigned>(buf);
	cOwned[3] = 0x40000000u;			// 2.0f
	const unsigned factorBits = cOwned[3];
	NxHeapMark m0 = nxHeapMarkNow();
	nxGuardedVoidCall(thunk,
		reinterpret_cast<unsigned char*>(cOwned) - 0x28);
	if(gFaultCode)
		return result;
	result.words[0][0] = g_heapFreeOps == m0.fo + 1 ? 1u : 0u;
	result.words[0][1] = g_heapMallocOps == m0.mo ? 1u : 0u;
	result.words[0][2] = cOwned[0] == 0 && cOwned[1] == 0 && cOwned[2] == 0
		? 1u : 0u;
	result.words[0][3] = cOwned[3] == factorBits ? 1u : 0u;
	result.words[0][4] = buf[0];		// empty() must not write the buffer
	result.words[0][5] = g_heapFreeBytes - m0.fb;	// the planted block's size
	result.words[0][6] = 1u;

	// -- case 1: external buffer, factor -1.0f.
	unsigned cExt[4];
	memset(cExt, 0xcd, sizeof(cExt));
	static unsigned extBuf[4] = { 0x6b6b0001u, 0x6b6b0002u, 0, 0 };
	cExt[0] = 4;
	cExt[1] = 2;
	cExt[2] = reinterpret_cast<unsigned>(extBuf);
	cExt[3] = 0xbf800000u;				// -1.0f
	NxHeapMark m1 = nxHeapMarkNow();
	thunk(reinterpret_cast<unsigned char*>(cExt) - 0x28);
	result.words[1][0] = g_heapFreeOps == m1.fo && g_heapMallocOps == m1.mo
		? 1u : 0u;
	// The listing's shared tail (0x000b4f81/87) clears ONLY capacity and
	// count; the entries pointer survives the external-buffer arm -- it is
	// nulled at 0x000b4f7a only inside the owned arm, before the shared
	// clear. First decode had cExt[2]==0 here; the oracle's own drive
	// corrected it.
	result.words[1][1] = cExt[0] == 0 && cExt[1] == 0
		&& cExt[2] == reinterpret_cast<unsigned>(extBuf) ? 1u : 0u;
	result.words[1][2] = extBuf[0];
	result.words[1][3] = 1u;

	// -- case 2: null entries.
	unsigned cNull[4];
	memset(cNull, 0xcd, sizeof(cNull));
	cNull[0] = 0;
	cNull[1] = 0;
	cNull[2] = 0;
	cNull[3] = 0x40000000u;
	NxHeapMark m2 = nxHeapMarkNow();
	thunk(reinterpret_cast<unsigned char*>(cNull) - 0x28);
	result.words[2][0] = g_heapFreeOps == m2.fo && g_heapMallocOps == m2.mo
		? 1u : 0u;
	result.words[2][1] = cNull[0] == 0 && cNull[1] == 0 && cNull[2] == 0
		? 1u : 0u;
	result.words[2][2] = 1u;

	// Explicit expectations, hand-derived from the listing: free exactly the
	// owned block, keep the external one, never touch the buffer bytes, and
	// always clear capacity/count/entries.
	result.digest = nxFold(result.digest, result.words[0][0]);
	result.digest = nxFold(result.digest, result.words[0][1]);
	result.digest = nxFold(result.digest, result.words[0][2]);
	result.digest = nxFold(result.digest, result.words[0][3]);
	result.digest = nxFold(result.digest, result.words[0][4]);
	result.digest = nxFold(result.digest, result.words[0][5]);
	if(result.words[0][0] != 1u) ++result.failures;
	if(result.words[0][1] != 1u) ++result.failures;
	if(result.words[0][2] != 1u) ++result.failures;
	if(result.words[0][3] != 1u) ++result.failures;
	if(result.words[0][4] != 0x5a5a0001u) ++result.failures;
	if(result.words[0][5] != ((4 * sizeof(unsigned) + 7u) & ~7u) + 4u)
		++result.failures;
	result.digest = nxFold(result.digest, result.words[1][0]);
	result.digest = nxFold(result.digest, result.words[1][1]);
	result.digest = nxFold(result.digest, result.words[1][2]);
	if(result.words[1][0] != 1u) ++result.failures;
	if(result.words[1][1] != 1u) ++result.failures;
	if(result.words[1][2] != 0x6b6b0001u) ++result.failures;
	result.digest = nxFold(result.digest, result.words[2][0]);
	result.digest = nxFold(result.digest, result.words[2][1]);
	if(result.words[2][0] != 1u) ++result.failures;
	if(result.words[2][1] != 1u) ++result.failures;
	return result;
	}

// Declared in ObjectModel.h as void (the thunk's own return is whatever
// empty() leaves in eax -- unpinned, like phys_fn_000004's null path).
void nxContainerAddThunk(void* innerThis);

static unsigned nxTestAddThunk(const unsigned char* base, bool selfOnly,
	unsigned& oracleDigest)
	{
	typedef void (__thiscall* OracleThunk)(void*);
	NxAddThunkResult oracle = nxDriveAddThunk(
		reinterpret_cast<OracleThunk>(const_cast<unsigned char*>(base) + 0x5b610));
	oracleDigest = nxFold(oracleDigest, oracle.digest);
	printf("addthunk row=oracle failures=%u digest=%08x\n",
		oracle.failures, oracle.digest);
	if(selfOnly)
		return oracle.failures;
	// The candidate thunk is a cdecl free function (the transcript's own
	// spelling), not __thiscall: driving it through a thiscall pointer put
	// the receiver in ecx and left the stack argument garbage, and the
	// guard ate the fault into a zero-folded digest.
	NxAddThunkResult candidate = nxDriveAddThunk(
		reinterpret_cast<void (__cdecl*)(void*)>(&nxContainerAddThunk));
	unsigned mismatches = 0;
	for(unsigned i = 0; i < 3; ++i)
		for(unsigned j = 0; j < 7; ++j)
			if(oracle.words[i][j] != candidate.words[i][j])
				++mismatches;
	printf("addthunk candidate failures=%u mismatches=%u digest=%08x\n",
		candidate.failures, mismatches, candidate.digest);
	return oracle.failures + candidate.failures + mismatches;
	}

static bool nxSha256(const wchar_t* path, char* text)
	{
	HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(file == INVALID_HANDLE_VALUE)
		return false;
	LARGE_INTEGER size;
	BYTE digest[32];
	bool ok = GetFileSizeEx(file, &size) != 0 && size.QuadPart > 0 && size.QuadPart < 0x08000000;
	BYTE* bytes = ok ? static_cast<BYTE*>(malloc(static_cast<size_t>(size.QuadPart))) : 0;
	DWORD read = 0;
	ok = bytes != 0
		&& ReadFile(file, bytes, static_cast<DWORD>(size.QuadPart), &read, 0) != 0
		&& read == size.QuadPart
		&& BCryptHash(BCRYPT_SHA256_ALG_HANDLE, 0, 0, bytes, read, digest, sizeof(digest)) == 0;
	free(bytes);
	CloseHandle(file);
	if(!ok)
		return false;
	for(int i = 0; i < 32; ++i)
		sprintf_s(text + i * 2, 3, "%02x", digest[i]);
	return true;
	}

int wmain(int argc, wchar_t** argv)
	{
	bool selfOnly = false;
	bool addThunkOnly = argc == 4 && wcscmp(argv[3], L"--addthunk-only") == 0;
	bool slot1Only = argc == 4 && wcscmp(argv[3], L"--slot1-only") == 0;
	if(argc == 4 && wcscmp(argv[3], L"--self") == 0)
		selfOnly = true;
	else if(argc != 3 && !slot1Only && !addThunkOnly)
		{
		fprintf(stderr, "usage: NxPhysicsObjectLayoutTests <oracle directory> <NxPhysics.dll sha256> [--self|--slot1-only|--addthunk-only]\n");
		return 2;
		}

	wchar_t physicsPath[MAX_PATH];
	if(swprintf_s(physicsPath, L"%s\\NxPhysics.dll", argv[1]) < 0)
		return nxFail("cannot form the oracle path");

	if(!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32) || !AddDllDirectory(argv[1]))
		return nxFail("cannot restrict the DLL search path");
	HMODULE physics = LoadLibraryExW(physicsPath, 0, LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32);
	if(!physics)
		{
		fprintf(stderr, "FAIL isolated LoadLibraryEx failed: %lu\n", GetLastError());
		return 1;
		}

	wchar_t loadedPath[MAX_PATH];
	char loadedHash[65];
	if(!GetModuleFileNameW(physics, loadedPath, MAX_PATH) || !nxSha256(loadedPath, loadedHash))
		return nxFail("cannot identify the loaded oracle");
	char expected[65];
	size_t converted = 0;
	if(wcstombs_s(&converted, expected, sizeof(expected), argv[2], _TRUNCATE) != 0)
		return nxFail("cannot read the expected hash argument");

	printf("layout module path=%S sha256=%s\n", loadedPath, loadedHash);
	printf("layout base=%p mode=%s\n", (void*) physics, selfOnly ? "self" : "differential");
	if(strcmp(loadedHash, expected) != 0)
		{
		fprintf(stderr, "FAIL loaded oracle is not the pinned one: expected %s\n", expected);
		return 1;
		}
	printf("layout pin=matched\n");
	fflush(stdout);
	setvbuf(stdout, 0, _IONBF, 0);	// probe crashes must not eat the transcript

	// Candidate-side allocations join the oracle's arena before any family
	// runs, so no CRT block can ever be handed to the emulator's free.
	nxSetSdkAllocatorBridge(&g_arenaAllocator);
	printf("layout arena-bridge=installed\n");

	const unsigned char* base = (const unsigned char*) physics;

	// The oracle side must serve frees from the emulator before any family
	// drives oracle code that releases memory: the addthunk drive was the
	// first to hit this, crashing 0xc0000374 on the first run because its
	// oracle empty() freed an arena block through the oracle's own adapter
	// before the shim existed. Installing once here makes the allocator
	// state uniform from the first drive; the per-block installs below are
	// idempotent re-pins of the same holder word.
	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");
	printf("layout allocator-shim=installed\n");

	// The SdkContainer rows (phys_fn_004846/002352) reach the allocator
	// through a DIFFERENT global: the helper at 0x000b4000 reads the
	// Foundation instance pointer at .data 0x1012845c (defaulting to the
	// static CRT adapter at 0x10122368 whose slot +0xc is a plain free).
	// Repointing only the SDK holder left empty() freeing arena blocks
	// through the CRT heap -- the 0xc0000374 that killed the first three
	// addthunk runs. Same adapter layout, same emulator.
	if(!nxInstallFoundationShim(base))
		return nxFail("the Foundation allocator global is unwritable");
	printf("layout foundation-shim=installed\n");

	unsigned oracleDigest = 2166136261u;
	unsigned addThunkFailures = nxTestAddThunk(base, selfOnly, oracleDigest);
	if(addThunkOnly)
		return addThunkFailures == 0 ? 0 : 1;
	unsigned slot1Failures = nxTestSlot1Wrapper(base, selfOnly, oracleDigest);
	if(slot1Only)
		return slot1Failures == 0 ? 0 : 1;
	unsigned candidateMissing = (slot1Failures != 0 || addThunkFailures != 0)
		? 1u : 0u;
	float oMin = 0.0f, oMax = 0.0f;
	unsigned oMinBits = 0, oMaxBits = 0, cMinBits = 0, cMaxBits = 0;
	unsigned oShapeBaseDigest = 0;
	unsigned oBoxDigest = 0;
	unsigned oSphereDigest = 0;
	unsigned oCapsuleDigest = 0;
	unsigned oPlaneDigest = 0;
	unsigned oMeshDigest = 0;
	unsigned oSaveStateDigest = 0;
	unsigned oBoxRowDigest = 0;
	unsigned oBoxSlot11Digest = 0;
	unsigned oBoxSlot13Digest = 0;
	unsigned oBoxSlot8Digest = 0;
	unsigned oBoxSlot9Digest = 0;
	unsigned oBoxDtorDigest = 0;
	static unsigned char sBoxDtorReference[0x228];
	unsigned oPlaneSaveDigest = 0;
	unsigned oSphereRowsDigest = 0;
	unsigned oSphereRadBits = 0;
	unsigned oCapsuleSaveDigest = 0;
	static unsigned char sCapsuleSaveReference[0x58];
	unsigned oSphereAABBDigest = 0;
	unsigned oCapsuleCRDigest = 0;
	unsigned oMeshSaveDigest = 0;
	unsigned oMeshWordsDigest = 2166136261u;
	unsigned oSphereLocalDigest = 0;
	unsigned oSphereDtorDigest = 0;
	unsigned oMesh44Digest = 0;
	unsigned oCapsuleDtorDigest = 0;
	unsigned oCapsuleLoadRadBits = 0;
	unsigned oCapsuleAABBDigest = 0;
	unsigned oSphereLoadRadBits = 0;
	unsigned oPlaneLoadNyBits = 0;
	unsigned oSphereLoadGroup = 0;
	unsigned oPlaneDtorDigest = 0;
	unsigned oMeshDtorDigest = 0;
	static const unsigned kDtorMask[] = { 0x00, 0x9c, 0xa4, 0xa8, 0xb0, 0xb4 };
	unsigned oSphereSetDigest = 0;
	unsigned oCapsuleSetDigest = 0;
	unsigned oGroupDigest = 0;
	unsigned oPlaneExtentDigest = 0;
	unsigned oMassFrameDigest = 0;
	unsigned oBoxMassDigest = 0;
	unsigned oCapMassDigest = 0;
	unsigned oFoldDigest = 0;
	unsigned oErrDigest = 0;
	unsigned oGroupErrDigest = 0;
	unsigned oLoadErrDigest = 0;
	unsigned oMaterialTemplateDigest = 0;
	unsigned oMaterialBootedDigest = 0;
	unsigned oOwnDigest = 0;
	unsigned oOwnDtorDigest = 0;
	unsigned oVecGrowDigest = 0;
	unsigned oVecGrowReservedDigest = 0;
	unsigned oRelGrowDigest = 0;
	unsigned oPairRmDigest = 0;
	unsigned oActorsmDigest = 0;
	unsigned oActorCtorDigest = 0;
	unsigned oActorsm2Digest = 0;
	unsigned oActorsm3Digest = 0;
	unsigned oActorsm4Digest = 0;
	unsigned oActorsm5Digest = 0;
	unsigned oActorsm6Digest = 0;
	unsigned oActorsm7Digest = 0;
	unsigned oMiscsmDigest = 0;
	unsigned oMiscsm2Digest = 0;
	unsigned oSlate11Digest = 0;
	unsigned oSlate12Digest = 0;
	unsigned oZeroDigest = 0;
	unsigned oZeroCandDigest = 0;
	static unsigned char sSaveStateRecord[0x48];

	// -----------------------------------------------------------------------
	// Vtable identity.
	for(size_t t = 0; t < sizeof(nxTables) / sizeof(nxTables[0]); ++t)
		{
		const NxTableSpec* spec = &nxTables[t];
		const unsigned rva = spec->rva;
		unsigned digest = 2166136261u;
		unsigned printed = 0;
		for(unsigned i = 0; i < spec->slots; ++i)
			{
			unsigned word;
			memcpy(&word, base + rva + i * 4, 4);
			digest = nxFold(digest, word);
			++printed;
			}
		oracleDigest = nxFold(oracleDigest, digest);
		printf("vt name=%s slots=%u digest=%08x\n", spec->name, printed, digest);
		if(printed != spec->expectSlotsPrinted)
			return nxFail("slot window drifted from its registration");
		}

	// -----------------------------------------------------------------------
	// The collision object constructor, on a poisoned buffer.
	{
	NxColObjCtorFn colObjCtor = (NxColObjCtorFn) (base + kColObjCtorRva);
	unsigned char object[0x1c];
	memset(object, 0xcd, sizeof(object));
	const unsigned kArg = 0xa5a5a5a5u;
	colObjCtor(object, kArg);

	unsigned digest = 2166136261u;
	for(unsigned i = 0; i < sizeof(object); i += 4)
		{
		unsigned word;
		memcpy(&word, object + i, 4);
		digest = nxFold(digest, word);
		}
	unsigned vptrFinal, vptrMember, w04, w08, arg18;
	memcpy(&vptrFinal, object + 0x00, 4);
	memcpy(&w04, object + 0x04, 4);
	memcpy(&w08, object + 0x08, 4);
	memcpy(&vptrMember, object + 0x0c, 4);
	memcpy(&arg18, object + 0x18, 4);
	oracleDigest = nxFold(oracleDigest, digest);

	printf("colobj ctor=phys_fn_001193 size=28 digest=%08x vptr_final=%08x zero04=%08x "
		"arg_at_8=%08x vptr_member=%08x arg_at_18=%08x\n",
		digest, vptrFinal, w04, w08, vptrMember, arg18);
	}

	// -----------------------------------------------------------------------
	// The owner accessor against a fake shape.
	{
	NxOwnerAccessorFn ownerAccessor = (NxOwnerAccessorFn) (base + kOwnerAccessorRva);
	unsigned char fake[16];
	memset(fake, 0, sizeof(fake));
	const unsigned kMark = 0x13579bdfu;
	memcpy(fake + 0x04, &kMark, 4);
	const void* owner = ownerAccessor(fake, 0);
	unsigned got;
	memcpy(&got, &owner, 4);
	oracleDigest = nxFold(oracleDigest, got);
	printf("owner accessor=phys_fn_001281 mark=%08x returned=%08x\n", kMark, got);
	}

	// -----------------------------------------------------------------------
	// The box hull facade's trivial rows, driven on a poisoned buffer at the
	// recorded RVAs. Pointer-valued results are module-specific, so what is
	// printed is their OFFSET from the fake object, plus the three constant
	// rows and the static-table contents.
	{
	typedef unsigned (__thiscall* NxConstFn)(void* self);
	typedef const void* (__thiscall* NxIndexFn)(void* self, unsigned index);
	typedef const void* (__thiscall* NxThisFn)(void* self);
	unsigned char fake[sizeof(BoxHullFacade)];
	memset(fake, 0xcd, sizeof(fake));
	NxConstFn vertexCount = (NxConstFn) (base + 0x00020d20);
	NxConstFn faceCount = (NxConstFn) (base + 0x000213c0);
	NxConstFn zeroRow = (NxConstFn) (base + 0x000213e0);
	NxIndexFn faceRow = (NxIndexFn) (base + 0x000213d0);
	NxThisFn verticesRow = (NxThisFn) (base + 0x00020d30);
	unsigned vc = vertexCount(fake);
	unsigned fc = faceCount(fake);
	unsigned zr = zeroRow(fake);
	const void* f2 = faceRow(fake, 2);
	const void* vx = verticesRow(fake);
	unsigned offFace = (unsigned) ((const unsigned char*) f2 - fake);
	unsigned offVerts = (unsigned) ((const unsigned char*) vx - fake);
	oracleDigest = nxFold(oracleDigest, vc);
	oracleDigest = nxFold(oracleDigest, fc);
	oracleDigest = nxFold(oracleDigest, zr);
	oracleDigest = nxFold(oracleDigest, offFace);
	oracleDigest = nxFold(oracleDigest, offVerts);
	printf("hull row=phys_fn_000953..71 vertexCount=%u faceCount=%u zero=%u "
		"face2_offset=%#x vertices_offset=%#x\n", vc, fc, zr, offFace, offVerts);

	unsigned digestTables = 2166136261u;
	for(int t = 0; t < 3; ++t)
		{
		static const unsigned rvas[3] = { 0x00122180, 0x001221e0, 0x00122240 };
		for(unsigned i = 0; i < 12 * 4; i += 4)
			{
			unsigned word;
			memcpy(&word, base + rvas[t] + i, 4);
			digestTables = nxFold(digestTables, word);
			}
		}
	oracleDigest = nxFold(oracleDigest, digestTables);
	printf("hull static tables digest=%08x\n", digestTables);

	// The support mapping: seed twin buffers with known vertices, an
	// identity-ish pose and a direction. The row's frame decodes as
	// (this, a1 unread, a2=&minOut, a3=&maxOut, a4=direction, a5=pose,
	// a6 unread): edx=a4 multiplies, edi=[E+8]=a2 gets the +FLT_MAX
	// sentinel, ebx=[E+0xc]=a3 the -FLT_MAX one.
	typedef void (__thiscall* NxSupportFn)(void* self, const void* unread1,
		float* outMin, float* outMax, const float* direction, const float* pose,
		const void* unread6);
	NxSupportFn support = (NxSupportFn) (base + 0x000217c0);
	unsigned char fakeFacade[sizeof(BoxHullFacade)];
	memset(fakeFacade, 0xcd, sizeof(fakeFacade));
	memcpy(fakeFacade + 0x10, nxSupportPose, sizeof(nxSupportPose));	// stand-in vertices
	support(fakeFacade, 0, &oMin, &oMax, nxSupportDirection, nxSupportPose, 0);
	memcpy(&oMinBits, &oMin, 4);
	memcpy(&oMaxBits, &oMax, 4);
	oracleDigest = nxFold(oracleDigest, oMinBits);
	oracleDigest = nxFold(oracleDigest, oMaxBits);
	printf("hull support row=phys_fn_000975 min_bits=%08x max_bits=%08x\n",
		oMinBits, oMaxBits);

	// Slot 0: the lazy shared-hook accessor. Oracle side -- two calls must
	// return the same pointer and the twelve bytes behind it stay zero.
	typedef const void* (__thiscall* NxSharedHookFn)(void* self);
	NxSharedHookFn sharedHook = (NxSharedHookFn) (base + 0x00021a10);
	const void* hook1 = sharedHook(fakeFacade);
	const void* hook2 = sharedHook(fakeFacade);
	unsigned hookWords[3] = { 0, 0, 0 };
	if(hook1)
		memcpy(hookWords, hook1, 12);
	bool hookOk = hook1 != 0 && hook1 == hook2
		&& hookWords[0] == 0 && hookWords[1] == 0 && hookWords[2] == 0;
	unsigned hookStable = (hook1 != 0 && hook1 == hook2) ? 1u : 0u;
	oracleDigest = nxFold(oracleDigest, hookStable);	// ptr itself is ASLR-moved: never folded
	oracleDigest = nxFold(oracleDigest, hookWords[0]);
	oracleDigest = nxFold(oracleDigest, hookWords[2]);
	printf("hull sharedhook row=phys_fn_000985 stable=%u words=%08x.%08x.%08x\n",
		hookStable, hookWords[0], hookWords[1], hookWords[2]);
	}

	// -----------------------------------------------------------------------
	// The base-shape constructor, phys_fn_001273 (0x00025530): __thiscall,
	// `ret 8`. Driven on a poisoned 0xe0 buffer with a null owner -- the
	// registration arm through [owner+4]+0x48 needs Task 4's actor classes --
	// and a marked second argument. Words that are module-specific pointers
	// (both vtables, the shape-to-prunable owner store and the prunable's
	// back-pointer into itself) are folded on neither side.
	{
	typedef void (__thiscall* NxShapeBaseCtorFn)(void* self, void* owner, unsigned argument);
	NxShapeBaseCtorFn shapeBaseCtor = (NxShapeBaseCtorFn) (base + 0x00025530);
	static const unsigned kPointerWords[] = { 0x00, 0xa4, 0xa8, 0xb0, 0xb4 };
	unsigned char object[0xe0];
	memset(object, 0xcd, sizeof(object));
	const unsigned kArg2 = 0x5a5a5a5au;
	shapeBaseCtor(object, 0, kArg2);

	unsigned digest = 2166136261u;
	for(unsigned i = 0; i < sizeof(object); i += 4)
		{
		bool pointer = false;
		for(size_t p = 0; p < sizeof(kPointerWords) / sizeof(kPointerWords[0]); ++p)
			if(kPointerWords[p] == i)
				pointer = true;
		if(pointer)
			continue;
		unsigned word;
		memcpy(&word, object + i, 4);
		digest = nxFold(digest, word);
		}
	oShapeBaseDigest = digest;
	oracleDigest = nxFold(oracleDigest, digest);
	unsigned w08, pose00c, w9c, wa0, prun24, prun28, sentinel, argD4, hwDC, hwDE;
	w08 = pose00c = w9c = wa0 = prun24 = prun28 = sentinel = argD4 = hwDC = hwDE = 0;
	memcpy(&w08, object + 0x08, 4);
	memcpy(&pose00c, object + 0x0c, 4);
	memcpy(&w9c, object + 0x9c, 4);
	memcpy(&wa0, object + 0xa0, 4);
	memcpy(&prun24, object + 0xc8, 4);	// Prunable+0x24
	memcpy(&prun28, object + 0xcc, 4);	// Prunable+0x28 as a word: ffff then zero bytes
	memcpy(&sentinel, object + 0xd0, 4);
	memcpy(&argD4, object + 0xd4, 4);
	memcpy(&hwDC, object + 0xdc, 2);
	memcpy(&hwDE, object + 0xde, 2);
	printf("shapebase ctor=phys_fn_001273 size=%u digest=%08x zero08=%08x pose_diag=%08x "
		"zero9c=%08x zeroa0=%08x prun24=%08x prun28=%08x sentinel_d0=%08x arg_d4=%08x "
		"hw_dc=%u hw_de=%u\n",
		(unsigned) sizeof(object), digest, w08, pose00c, w9c, wa0, prun24, prun28,
		sentinel, argD4, hwDC, hwDE);
	}

	// -----------------------------------------------------------------------
	// Slate 12, two leaf rows off a marked ShapeBase record: the +0xde
	// flag-bits reader (phys_fn_001287) and the descriptor getter
	// (phys_fn_000931 -- translation, dims, rotation into a 15-word record).
	{
	// Rebuild the marked record: the getter reads dims at +0xe4..+0xef --
	// PAST the 0xe0 base extent (they are the hull facade's first words on
	// a real box) -- so the buffer is 0xf0 bytes; the first drive's 0xe0
	// sizing made both sides read unrelated stack garbage, and the
	// digests diverged nondeterministically.
	unsigned char marked[0xf0];
	memset(marked, 0xcd, sizeof(marked));
	const unsigned kMark = 0x7e7e0000u;
	for(unsigned i = 0; i < 12; ++i)
		{
		unsigned w = kMark + 0x1000u + i;
		memcpy(marked + 0x30 + i * 4, &w, 4);
		}
	for(unsigned i = 0; i < 3; ++i)
		{
		unsigned w = kMark + 0x2000u + i;
		memcpy(marked + 0xe4 + i * 4, &w, 4);
		}
	for(unsigned i = 0; i < 9; ++i)
		{
		unsigned w = kMark + 0x3000u + i;
		memcpy(marked + 0x0c + i * 4, &w, 4);
		}
	unsigned short deBits = 0x0027u;
	memcpy(marked + 0xde, &deBits, 2);

	unsigned dL = 2166136261u;
	typedef unsigned (__thiscall* FlagBitsFn)(const void*, unsigned);
	FlagBitsFn flagBitsO =
		reinterpret_cast<FlagBitsFn>(const_cast<unsigned char*>(base) + 0x257d0);
	const unsigned masksL[] = { 0x00000007u, 0x000000ffu, 0x0000ffffu,
		0x00000018u, 0xffffffffu };
	for(unsigned m = 0; m < 5; ++m)
		dL = nxFold(dL, flagBitsO(marked, masksL[m]));
	unsigned recL[15];
	memset(recL, 0, sizeof(recL));
	typedef void (__thiscall* FillFn)(const void*, unsigned*);
	FillFn fillO =
		reinterpret_cast<FillFn>(const_cast<unsigned char*>(base) + 0x20490);
	fillO(marked, recL);
	for(unsigned w = 0; w < 15; ++w)
		dL = nxFold(dL, recL[w]);

	oSlate12Digest = dL;
	oracleDigest = nxFold(oracleDigest, dL);
	printf("shapeleaf row=oracle digest=%08x\n", dL);
	}

	// -----------------------------------------------------------------------
	// The box-shape constructor, phys_fn_000977 (0x00021870): __thiscall,
	// `ret 8`, both arguments forwarded to phys_fn_001273. Driven on a
	// poisoned 0x228 buffer with a null owner and a marked second argument.
	// Masked words: the two final vtables (+0x00/+0xe0), the heap collision
	// object pointer (+0x9c), and the four base-ctor pointer words.
	{
	NxShapeCtorFn boxCtor = (NxShapeCtorFn) (base + 0x00021870);
	static const unsigned kPointerWords[] = { 0x00, 0x9c, 0xa4, 0xa8, 0xb0, 0xb4, 0xe0 };
	unsigned char object[0x228];
	memset(object, 0xcd, sizeof(object));
	const unsigned kArg2 = 0x5a5a5a5au;

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned faultAddr = 0, faultCode = 0;
	nxGuardedCtor(boxCtor, object, 0, kArg2, faultCode, faultAddr);
	if(faultCode)
		{
		fprintf(stderr, "FAIL box ctor fault code=%08x at=%08x rva=%08x\n",
			faultCode, faultAddr, faultAddr - (unsigned) (uintptr_t) base);
		return 1;
		}

	unsigned digest = 2166136261u;
	for(unsigned i = 0; i < sizeof(object); i += 4)
		{
		bool pointer = false;
		for(size_t p = 0; p < sizeof(kPointerWords) / sizeof(kPointerWords[0]); ++p)
			if(kPointerWords[p] == i)
				pointer = true;
		if(pointer)
			continue;
		unsigned word;
		memcpy(&word, object + i, 4);
		digest = nxFold(digest, word);
		}
	oBoxDigest = digest;
	oracleDigest = nxFold(oracleDigest, digest);

	// Structural facts about the masked collision-object word: it must point
	// at a live 0x1c-byte object whose +8/+0x18 carry THIS buffer and whose
	// member slot is non-null.
	unsigned colobj = 0;
	memcpy(&colobj, object + 0x9c, 4);
	bool colobjOk = colobj != 0;
	unsigned colobjArg8 = 0, colobjArg18 = 0, colobjMember = 0;
	if(colobjOk)
		{
		memcpy(&colobjArg8, (unsigned char*) colobj + 0x08, 4);
		memcpy(&colobjArg18, (unsigned char*) colobj + 0x18, 4);
		memcpy(&colobjMember, (unsigned char*) colobj + 0x0c, 4);
		colobjOk = colobjArg8 == (unsigned) (uintptr_t) object
			&& colobjArg18 == (unsigned) (uintptr_t) object
			&& colobjMember != 0;
		}

	unsigned dims04, dims08, dims0c, poisonVerts, poisonFaceFloats, face0w0, face5w0, sentinelBox, argD4Box;
	dims04 = dims08 = dims0c = poisonVerts = poisonFaceFloats = face0w0 = face5w0 = sentinelBox = argD4Box = 0;
	memcpy(&sentinelBox, object + 0xd0, 4);
	memcpy(&argD4Box, object + 0xd4, 4);
	memcpy(&dims04, object + 0xe4, 4);
	memcpy(&dims08, object + 0xe8, 4);
	memcpy(&dims0c, object + 0xec, 4);
	memcpy(&poisonVerts, object + 0xf0, 4);			// first untouched vertex word
	memcpy(&poisonFaceFloats, object + 0x164, 4);	// record 0 float data
	memcpy(&face0w0, object + 0x150, 4);			// record 0 corners
	memcpy(&face5w0, object + 0x204, 4);			// record 5 corners
	printf("boxshape ctor=phys_fn_000977 size=%u digest=%08x sentinel_d0=%u arg_d4=%08x "
		"dims=%08x.%08x.%08x face0_corners=%08x face5_corners=%08x verts_poison=%08x "
		"floats_poison=%08x colobj_ok=%u\n",
		(unsigned) sizeof(object), digest, sentinelBox, argD4Box,
		dims04, dims08, dims0c, face0w0, face5w0, poisonVerts, poisonFaceFloats,
		colobjOk ? 1u : 0u);
	}

	// -----------------------------------------------------------------------
	// The sphere-shape constructor, phys_fn_001349 (0x000277c0): __thiscall,
	// `ret 8`, both arguments forwarded to phys_fn_001273. Driven on a
	// poisoned 0xe4 buffer with a null owner and a marked second argument.
	// Masked words: the final vtable (+0x00), the collision object (+0x9c)
	// and the four base-ctor pointer words.
	{
	NxShapeCtorFn sphereCtor = (NxShapeCtorFn) (base + 0x000277c0);
	static const unsigned kPointerWords[] = { 0x00, 0x9c, 0xa4, 0xa8, 0xb0, 0xb4 };
	unsigned char object[0xe4];
	memset(object, 0xcd, sizeof(object));
	const unsigned kArg2 = 0x5a5a5a5au;

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned faultAddr = 0, faultCode = 0;
	nxGuardedCtor(sphereCtor, object, 0, kArg2, faultCode, faultAddr);
	if(faultCode)
		{
		fprintf(stderr, "FAIL sphere ctor fault code=%08x at=%08x rva=%08x\n",
			faultCode, faultAddr, faultAddr - (unsigned) (uintptr_t) base);
		return 1;
		}

	unsigned digest = 2166136261u;
	for(unsigned i = 0; i < sizeof(object); i += 4)
		{
		bool pointer = false;
		for(size_t p = 0; p < sizeof(kPointerWords) / sizeof(kPointerWords[0]); ++p)
			if(kPointerWords[p] == i)
				pointer = true;
		if(pointer)
			continue;
		unsigned word;
		memcpy(&word, object + i, 4);
		digest = nxFold(digest, word);
		}
	oSphereDigest = digest;
	oracleDigest = nxFold(oracleDigest, digest);

	unsigned colobj = 0;
	memcpy(&colobj, object + 0x9c, 4);
	bool colobjOk = colobj != 0;
	unsigned colobjArg8 = 0, colobjArg18 = 0, colobjMember = 0;
	if(colobjOk)
		{
		memcpy(&colobjArg8, (unsigned char*) colobj + 0x08, 4);
		memcpy(&colobjArg18, (unsigned char*) colobj + 0x18, 4);
		memcpy(&colobjMember, (unsigned char*) colobj + 0x0c, 4);
		colobjOk = colobjArg8 == (unsigned) (uintptr_t) object
			&& colobjArg18 == (unsigned) (uintptr_t) object
			&& colobjMember != 0;
		}

	unsigned radiusWord = 0, sentinelSphere = 0;
	memcpy(&radiusWord, object + 0xe0, 4);
	memcpy(&sentinelSphere, object + 0xd0, 4);
	printf("sphere ctor=phys_fn_001349 size=%u digest=%08x sentinel_d0=%u arg_d4=%08x "
		"radius_e0=%08x colobj_ok=%u\n",
		(unsigned) sizeof(object), digest, sentinelSphere, kArg2, radiusWord,
		colobjOk ? 1u : 0u);
	}

	// -----------------------------------------------------------------------
	// The capsule-shape constructor, phys_fn_000987 (0x00021a60): __thiscall,
	// `ret 8`, both arguments forwarded to phys_fn_001273. Driven on a
	// poisoned 0xe8 buffer with a null owner and a marked second argument.
	// Masked words: vtable (+0x00), colobj (+0x9c) and the four base-ctor
	// pointer words.
	{
	NxShapeCtorFn capsuleCtor = (NxShapeCtorFn) (base + 0x00021a60);
	static const unsigned kPointerWords[] = { 0x00, 0x9c, 0xa4, 0xa8, 0xb0, 0xb4 };
	unsigned char object[0xec];
	memset(object, 0xcd, sizeof(object));
	const unsigned kArg2 = 0x5a5a5a5au;

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned faultAddr = 0, faultCode = 0;
	nxGuardedCtor(capsuleCtor, object, 0, kArg2, faultCode, faultAddr);
	if(faultCode)
		{
		fprintf(stderr, "FAIL capsule ctor fault code=%08x at=%08x rva=%08x\n",
			faultCode, faultAddr, faultAddr - (unsigned) (uintptr_t) base);
		return 1;
		}

	unsigned digest = 2166136261u;
	for(unsigned i = 0; i < sizeof(object); i += 4)
		{
		bool pointer = false;
		for(size_t p = 0; p < sizeof(kPointerWords) / sizeof(kPointerWords[0]); ++p)
			if(kPointerWords[p] == i)
				pointer = true;
		if(pointer)
			continue;
		unsigned word;
		memcpy(&word, object + i, 4);
		digest = nxFold(digest, word);
		}
	oCapsuleDigest = digest;
	oracleDigest = nxFold(oracleDigest, digest);

	unsigned colobj = 0;
	memcpy(&colobj, object + 0x9c, 4);
	bool colobjOk = colobj != 0;
	unsigned colobjArg8 = 0, colobjArg18 = 0, colobjMember = 0;
	if(colobjOk)
		{
		memcpy(&colobjArg8, (unsigned char*) colobj + 0x08, 4);
		memcpy(&colobjArg18, (unsigned char*) colobj + 0x18, 4);
		memcpy(&colobjMember, (unsigned char*) colobj + 0x0c, 4);
		colobjOk = colobjArg8 == (unsigned) (uintptr_t) object
			&& colobjArg18 == (unsigned) (uintptr_t) object
			&& colobjMember != 0;
		}

	unsigned fE0 = 0, fE4 = 0, sentinelCapsule = 0;
	memcpy(&fE0, object + 0xe0, 4);
	memcpy(&fE4, object + 0xe4, 4);
	memcpy(&sentinelCapsule, object + 0xd0, 4);
	printf("capsule ctor=phys_fn_000987 size=%u digest=%08x sentinel_d0=%u arg_d4=%08x "
		"float_e0=%08x float_e4=%08x colobj_ok=%u\n",
		(unsigned) sizeof(object), digest, sentinelCapsule, kArg2, fE0, fE4,
		colobjOk ? 1u : 0u);
	}

	// -----------------------------------------------------------------------
	// The plane-shape constructor, phys_fn_001247 (0x00024ed0): __thiscall,
	// `ret 8`, both arguments forwarded to phys_fn_001273. Driven on a
	// poisoned 0x10c buffer with a null owner and a marked second argument.
	// The tangents are folded IN: with the default normal (0,1,0) the
	// NxNormalToTangents arithmetic is exact, so both foundations produce
	// identical bits. Masked words: vtable (+0x00), colobj (+0x9c) and the
	// four base-ctor pointer words.
	{
	NxShapeCtorFn planeCtor = (NxShapeCtorFn) (base + 0x00024ed0);
	static const unsigned kPointerWords[] = { 0x00, 0x9c, 0xa4, 0xa8, 0xb0, 0xb4 };
	unsigned char object[0x10c];
	memset(object, 0xcd, sizeof(object));
	const unsigned kArg2 = 0x5a5a5a5au;

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned faultAddr = 0, faultCode = 0;
	nxGuardedCtor(planeCtor, object, 0, kArg2, faultCode, faultAddr);
	if(faultCode)
		{
		fprintf(stderr, "FAIL plane ctor fault code=%08x at=%08x rva=%08x\n",
			faultCode, faultAddr, faultAddr - (unsigned) (uintptr_t) base);
		return 1;
		}

	unsigned digest = 2166136261u;
	for(unsigned i = 0; i < sizeof(object); i += 4)
		{
		bool pointer = false;
		for(size_t p = 0; p < sizeof(kPointerWords) / sizeof(kPointerWords[0]); ++p)
			if(kPointerWords[p] == i)
				pointer = true;
		if(pointer)
			continue;
		unsigned word;
		memcpy(&word, object + i, 4);
		digest = nxFold(digest, word);
		}
	oPlaneDigest = digest;
	oracleDigest = nxFold(oracleDigest, digest);

	unsigned colobj = 0;
	memcpy(&colobj, object + 0x9c, 4);
	bool colobjOk = colobj != 0;
	unsigned colobjArg8 = 0, colobjArg18 = 0, colobjMember = 0;
	if(colobjOk)
		{
		memcpy(&colobjArg8, (unsigned char*) colobj + 0x08, 4);
		memcpy(&colobjArg18, (unsigned char*) colobj + 0x18, 4);
		memcpy(&colobjMember, (unsigned char*) colobj + 0x0c, 4);
		colobjOk = colobjArg8 == (unsigned) (uintptr_t) object
			&& colobjArg18 == (unsigned) (uintptr_t) object
			&& colobjMember != 0;
		}

	unsigned nX = 0, nY = 0, nZ = 0, distEC = 0, word108 = 0, sentinelPlane = 0;
	unsigned tangentF0[3] = { 0, 0, 0 }, binormalFC[3] = { 0, 0, 0 };
	memcpy(&nX, object + 0xe0, 4);
	memcpy(&nY, object + 0xe4, 4);
	memcpy(&nZ, object + 0xe8, 4);
	memcpy(&distEC, object + 0xec, 4);
	memcpy(&word108, object + 0x108, 4);
	memcpy(&sentinelPlane, object + 0xd0, 4);
	memcpy(tangentF0, object + 0xf0, 12);
	memcpy(binormalFC, object + 0xfc, 12);
	printf("plane ctor=phys_fn_001247 size=%u digest=%08x sentinel_d0=%u arg_d4=%08x "
		"normal=%08x.%08x.%08x dist_ec=%08x word108=%u tangent_f0=%08x.%08x.%08x "
		"binormal_fc=%08x.%08x.%08x colobj_ok=%u\n",
		(unsigned) sizeof(object), digest, sentinelPlane, kArg2,
		nX, nY, nZ, distEC, word108,
		tangentF0[0], tangentF0[1], tangentF0[2],
		binormalFC[0], binormalFC[1], binormalFC[2],
		colobjOk ? 1u : 0u);
	}

	// -----------------------------------------------------------------------
	// The mesh-shape constructor, phys_fn_001379 (0x00027db0): __thiscall,
	// `ret 8`, both arguments forwarded to phys_fn_001273. Driven on a
	// poisoned 0xe8 buffer with a null owner and a marked second argument.
	// Masked words: vtable (+0x00), colobj (+0x9c) and the four base-ctor
	// pointer words.
	{
	NxShapeCtorFn meshCtor = (NxShapeCtorFn) (base + 0x00027db0);
	static const unsigned kPointerWords[] = { 0x00, 0x9c, 0xa4, 0xa8, 0xb0, 0xb4 };
	unsigned char object[0xe8];
	memset(object, 0xcd, sizeof(object));
	const unsigned kArg2 = 0x5a5a5a5au;

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned faultAddr = 0, faultCode = 0;
	nxGuardedCtor(meshCtor, object, 0, kArg2, faultCode, faultAddr);
	if(faultCode)
		{
		fprintf(stderr, "FAIL mesh ctor fault code=%08x at=%08x rva=%08x\n",
			faultCode, faultAddr, faultAddr - (unsigned) (uintptr_t) base);
		return 1;
		}

	unsigned digest = 2166136261u;
	for(unsigned i = 0; i < sizeof(object); i += 4)
		{
		bool pointer = false;
		for(size_t p = 0; p < sizeof(kPointerWords) / sizeof(kPointerWords[0]); ++p)
			if(kPointerWords[p] == i)
				pointer = true;
		if(pointer)
			continue;
		unsigned word;
		memcpy(&word, object + i, 4);
		digest = nxFold(digest, word);
		}
	oMeshDigest = digest;
	oracleDigest = nxFold(oracleDigest, digest);

	unsigned colobj = 0;
	memcpy(&colobj, object + 0x9c, 4);
	bool colobjOk = colobj != 0;
	unsigned colobjArg8 = 0, colobjArg18 = 0, colobjMember = 0;
	if(colobjOk)
		{
		memcpy(&colobjArg8, (unsigned char*) colobj + 0x08, 4);
		memcpy(&colobjArg18, (unsigned char*) colobj + 0x18, 4);
		memcpy(&colobjMember, (unsigned char*) colobj + 0x0c, 4);
		colobjOk = colobjArg8 == (unsigned) (uintptr_t) object
			&& colobjArg18 == (unsigned) (uintptr_t) object
			&& colobjMember != 0;
		}

	unsigned wE0 = 0, wE4 = 0, sentinelMesh = 0;
	memcpy(&wE0, object + 0xe0, 4);
	memcpy(&wE4, object + 0xe4, 4);
	memcpy(&sentinelMesh, object + 0xd0, 4);
	printf("mesh ctor=phys_fn_001379 size=%u digest=%08x sentinel_d0=%u arg_d4=%08x "
		"word_e0=%08x word_e4=%08x colobj_ok=%u\n",
		(unsigned) sizeof(object), digest, sentinelMesh, kArg2, wE0, wE4,
		colobjOk ? 1u : 0u);
	}

	// -----------------------------------------------------------------------
	// The BASE vtable's stub rows, driven on a dummy this (the rows read
	// nothing): slot 4 phys_fn_001249 (two args -> false), slot 5
	// phys_fn_004812 (four args -> null), slot 7 phys_fn_001035 (one arg ->
	// false, the sweep stub). The candidate side answers through ShapeBase's
	// member transcriptions.
	{
	typedef bool (__thiscall* NxSlot4Fn)(void* self, void* a1, void* a2);
	typedef void* (__thiscall* NxSlot5Fn)(void* self, void* a1, void* a2, void* a3, void* a4);
	typedef bool (__thiscall* NxSlot7Fn)(void* self, void* a1);
	NxSlot4Fn slot4 = (NxSlot4Fn) (base + 0x00024f70);
	NxSlot5Fn slot5 = (NxSlot5Fn) (base + 0x000b4070);
	NxSlot7Fn slot7 = (NxSlot7Fn) (base + 0x00022dd0);
	unsigned char dummyThis[0x20];
	memset(dummyThis, 0xcd, sizeof(dummyThis));
	const unsigned kA1 = 0x11111111u, kA2 = 0x22222222u,
		kA3 = 0x33333333u, kA4 = 0x44444444u;

	bool r4 = slot4(dummyThis, (void*) kA1, (void*) kA2);
	void* r5 = slot5(dummyThis, (void*) kA1, (void*) kA2, (void*) kA3, (void*) kA4);
	bool r7 = slot7(dummyThis, (void*) kA1);
	unsigned r5bits = 0;
	memcpy(&r5bits, &r5, 4);
	oracleDigest = nxFold(oracleDigest, r4 ? 1u : 0u);
	oracleDigest = nxFold(oracleDigest, r5bits);
	oracleDigest = nxFold(oracleDigest, r7 ? 1u : 0u);
	printf("basevt slots=4:001249,5:004812,7:001035 ret4=%u ret5=%08x ret7=%u\n",
		r4 ? 1u : 0u, r5bits, r7 ? 1u : 0u);
	}

	// -----------------------------------------------------------------------
	// BASE slot 2, phys_fn_001277: save-to-descriptor. A real sphere is
	// constructed first (shim allocator), then the row fills a poisoned
	// descriptor record; every byte it writes is module-independent.
	{
	NxShapeCtorFn sphereCtor2 = (NxShapeCtorFn) (base + 0x000277c0);
	typedef bool (__thiscall* NxSaveStateFn)(void* self, void* record);
	NxSaveStateFn saveState = (NxSaveStateFn) (base + 0x000256f0);

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char shape[0xe4];
	memset(shape, 0xcd, sizeof(shape));
	sphereCtor2(shape, 0, 0);

	unsigned char record[0x48];
	memset(record, 0xcd, sizeof(record));
	bool saved = saveState(shape, record);

	unsigned digest = 2166136261u;
	for(unsigned i = 0; i < sizeof(record); i += 4)
		{
		unsigned word;
		memcpy(&word, record + i, 4);
		digest = nxFold(digest, word);
		}
	oSaveStateDigest = digest;
	oracleDigest = nxFold(oracleDigest, digest);

	unsigned poseDiag = 0, word38 = 0, word3c = 0, word40 = 0, poisonHead = 0;
	memcpy(&poseDiag, record + 8, 4);
	memcpy(&word38, record + 0x38, 4);
	memcpy(&word3c, record + 0x3c, 4);
	memcpy(&word40, record + 0x40, 4);
	memcpy(&poisonHead, record, 4);
	memcpy(sSaveStateRecord, record, sizeof(record));
	printf("basesave row=phys_fn_001277 saved=%u digest=%08x pose_diag=%08x "
		"word38=%08x word3c=%08x word40=%08x poison_head=%08x\n",
		saved ? 1u : 0u, digest, poseDiag, word38, word3c, word40, poisonHead);
	}

	// -----------------------------------------------------------------------
	// BOX-table slot 10, phys_fn_000937: pose-one translation + sqrt of the
	// squared dims. Driven on a real constructed box (default dims 1,1,1,
	// where every association of the sum is exact).
	{
	typedef void (__thiscall* NxBoxRowFn)(void* self, float* out);
	NxBoxRowFn boxRow10 = (NxBoxRowFn) (base + 0x00020670);
	typedef void (__thiscall* NxShapeCtor2Fn)(void* self, void* owner, unsigned argument);

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char shape[0x228];
	memset(shape, 0xcd, sizeof(shape));
	NxShapeCtor2Fn boxCtor2 = (NxShapeCtor2Fn) (base + 0x00021870);
	boxCtor2(shape, 0, 0);

	float out[4] = { 0, 0, 0, 0 };
	boxRow10(shape, out);

	unsigned digest = 2166136261u;
	for(int i = 0; i < 4; ++i)
		{
		unsigned word;
		memcpy(&word, out + i, 4);
		digest = nxFold(digest, word);
		}
	oBoxRowDigest = digest;
	oracleDigest = nxFold(oracleDigest, digest);
	unsigned ob[4] = { 0, 0, 0, 0 };
	for(int i = 0; i < 4; ++i)
		memcpy(&ob[i], out + i, 4);
	printf("boxrow slot10=phys_fn_000937 out=%08x.%08x.%08x.%08x\n",
		ob[0], ob[1], ob[2], ob[3]);
	}

	// -----------------------------------------------------------------------
	// ORACLE CONTRACT ONLY -- BOX slot 5 (phys_fn_000949 at 0x20880).
	// Not candidate coverage: no family digest/registration. Eight cases
	// assert exact returns, all hit-record words and boundary canaries.
	// These reject moving the distance test before the point/t stores.
	{
	typedef void* (__thiscall* NxBoxRayCapFn)(void* self, const float* ray,
		float a2, float a3, unsigned flags, void* hit);
	NxBoxRayCapFn boxRay5Cap = (NxBoxRayCapFn) (base + 0x00020880);
	unsigned char shapeCap[0x228];
	memset(shapeCap, 0xcd, sizeof(shapeCap));
	typedef void (__thiscall* NxBoxCtorCapFn)(void* self, void* owner, unsigned argument);
	NxBoxCtorCapFn boxCtorCap = (NxBoxCtorCapFn) (base + 0x00021870);
	boxCtorCap(shapeCap, 0, 0);
	unsigned* tableCap = *reinterpret_cast<unsigned**>(shapeCap);
	const unsigned imageBaseCap = reinterpret_cast<unsigned>(base);
	if(tableCap[4] - imageBaseCap != 0x20850
		|| tableCap[5] - imageBaseCap != 0x20880)
		return nxFail("BOX oracle slot-4/5 mapping changed");
	printf("boxray oracle slots4-5=00020850,00020880\n");

	struct RayCapCase { float ray[6]; float a2; float a3; unsigned flags; };
	// o = origin triple, d = direction triple. A: +x into the box.
	// B: same with normal arm. C: away. D: gate probe. E: inside origin.
	// F: diagonal.
	const RayCapCase capCases[] =
		{
		{ { 2, 0, 0, -1, 0, 0 }, 111.0f, 10.0f, 0 },
		{ { 2, 0, 0, -1, 0, 0 }, 111.0f, 10.0f, 4 },
		{ { 2, 0, 0, 1, 0, 0 }, 111.0f, 10.0f, 4 },
		{ { 2, 0, 0, -1, 0, 0 }, 111.0f, 0.5f, 0 },
		{ { 0, 0, 0, -1, 0, 0 }, 111.0f, 10.0f, 0 },
		{ { 2, 2, 0, -0.70710677f, -0.70710677f, 0 }, 111.0f, 10.0f, 4 },
		// Real max-distance rejection and equality; a3 remains distinct.
		{ { 2, 0, 0, -1, 0, 0 }, 0.5f, 111.0f, 4 },
		{ { 2, 0, 0, -1, 0, 0 }, 1.0f, 111.0f, 4 },
		};
	for(unsigned c = 0; c < sizeof(capCases) / sizeof(capCases[0]); ++c)
		{
		unsigned guarded[14];
		memset(guarded, 0xcd, sizeof(guarded));
		unsigned char* rec = reinterpret_cast<unsigned char*>(guarded + 1);
		void* okc = boxRay5Cap(shapeCap, capCases[c].ray, capCases[c].a2,
			capCases[c].a3, capCases[c].flags, rec);
		unsigned rw[12];
		for(unsigned w = 0; w < 12; ++w)
			memcpy(&rw[w], rec + w * 4, 4);
		printf("boxraycap case=%u ret=%u rec=%08x %08x %08x %08x %08x %08x "
			"%08x %08x %08x %08x %08x %08x\n",
			c, okc != 0 ? 1u : 0u,
			rw[0], rw[1], rw[2], rw[3], rw[4], rw[5],
			rw[6], rw[7], rw[8], rw[9], rw[10], rw[11]);

		// Literal reference values for this identity-pose fixture. In
		// particular, distance rejection writes point/t but NOT colobj,
		// normal, face words or tag (stores 0x20a10..1d precede 0x20a2f).
		unsigned expected[12];
		memset(expected, 0xcd, sizeof(expected));
		const bool kernelHit = c != 2 && c != 4;
		const bool accepted = kernelHit && c != 6;
		if(kernelHit)
			{
			expected[1] = 0x3f800000u;
			expected[2] = c == 5 ? 0x3f800000u : 0u;
			expected[3] = 0;
			expected[8] = c == 5 ? 0x3fb504f3u : 0x3f800000u;
			}
		if(accepted)
			{
			memcpy(&expected[0], shapeCap + 0x9c, 4);
			expected[7] = expected[9] = expected[10] = 0;
			expected[11] = 0x13;
			if(capCases[c].flags & 4)
				{
				expected[4] = 0x3f800000u;
				expected[5] = expected[6] = 0;
				expected[11] = 0x17;
				}
			}
		if(okc != (accepted ? static_cast<void*>(shapeCap) : nullptr)
			|| memcmp(rw, expected, sizeof(expected)) != 0
			|| guarded[0] != 0xcdcdcdcdu || guarded[13] != 0xcdcdcdcdu)
			{
			fprintf(stderr, "FAIL boxray contract case=%u return/record/canary mismatch\n", c);
			return 1;
			}
		}
	printf("boxray contract cases=8 failures=0 mode=oracle-only\n");

	// Candidate-side check on the same eight cases. The reconstruction's
	// wrapper is provisional; this only fails if it disagrees with the
	// oracle contract above.
	BoxShape& shapeRef = *reinterpret_cast<BoxShape*>(shapeCap);
	for(unsigned c = 0; c < sizeof(capCases) / sizeof(capCases[0]); ++c)
		{
		unsigned char recC[0x30];
		memset(recC, 0xcd, sizeof(recC));
		void* okC = shapeRef.nxBoxRaycast(capCases[c].ray, capCases[c].a2,
			reinterpret_cast<const unsigned&>(capCases[c].a3),
			capCases[c].flags, recC);
		unsigned char recO[0x30];
		memset(recO, 0xcd, sizeof(recO));
		void* okO = boxRay5Cap(shapeCap, capCases[c].ray, capCases[c].a2,
			capCases[c].a3, capCases[c].flags, recO);
		if(okC != okO
			|| memcmp(recC, recO, sizeof(recC)) != 0)
			{
			fprintf(stderr, "FAIL boxray candidate case=%u disagrees\n", c);
			return 1;
			}
		}
	printf("boxray candidate8 mode=provisional\n");

	// Six faces under identity, translation, and a cyclic rotation. Compare
	// actual return pointers, every output word, and canaries on both sides.
	// Direct member invocation tests candidate code on an oracle-layout fixture;
	// it does not establish candidate construction or virtual dispatch.
	unsigned sweepFailures = 0, sweepCases = 0;
	for(unsigned pose = 0; pose < 4; ++pose)
		{
		float rotation[9] = { 0 };
		for(unsigned row = 0; row < 3; ++row)
			rotation[row * 3 + (row + (pose == 2 ? 1 : 0)) % 3] = 1.0f;
		if(pose == 3)
			{
			const float quarterTurn[9] = { 0, -1, 0, 1, 0, 0, 0, 0, 1 };
			memcpy(rotation, quarterTurn, sizeof(rotation));
			}
		float translation[3] = { 0, 0, 0 };
		if(pose) { translation[0] = 3; translation[1] = -5; translation[2] = 7; }
		memcpy(shapeCap + 0x0c, rotation, sizeof(rotation));
		memcpy(shapeCap + 0x30, translation, sizeof(translation));
		for(unsigned face = 0; face < 6; ++face)
		for(unsigned distance = 0; distance < 3; ++distance)
			{
			float ray[6];
			const unsigned axis = face / 2;
			const float sign = (face & 1) ? -1.0f : 1.0f;
			for(unsigned row = 0; row < 3; ++row)
				{
				ray[row] = translation[row] + rotation[row * 3 + axis] * sign * 2;
				ray[3 + row] = -rotation[row * 3 + axis] * sign;
				}
			float maximum = distance == 0 ? 10.0f : 0.5f;
			if(distance == 2) { const unsigned nan = 0x7fc00000u; memcpy(&maximum, &nan, 4); }
			unsigned candidate[14], oracle[14];
			memset(candidate, 0xcd, sizeof(candidate));
			memset(oracle, 0xcd, sizeof(oracle));
			void* retO = boxRay5Cap(shapeCap, ray, maximum, 111.0f, 4, oracle + 1);
			void* retC = shapeRef.nxBoxRaycast(ray, maximum, 0, 4, candidate + 1);
			bool match = retO == retC && memcmp(candidate, oracle, sizeof(candidate)) == 0
				&& candidate[0] == 0xcdcdcdcdu && candidate[13] == 0xcdcdcdcdu;
			++sweepCases;
			if(!match)
				{
				++sweepFailures;
				fprintf(stderr, "boxray sweep mismatch pose=%u face=%u distance=%u returns=%u/%u\n",
					pose, face, distance, retO != 0, retC != 0);
				for(unsigned w = 0; w < 14; ++w)
					if(candidate[w] != oracle[w]) fprintf(stderr, " word=%u oracle=%08x candidate=%08x\n", w, oracle[w], candidate[w]);
				}
			}
		}
	printf("boxray sweep cases=%u failures=%u mode=provisional\n", sweepCases, sweepFailures);
	if(sweepFailures) return nxFail("boxray six-face differential mismatch");
	}

	// -----------------------------------------------------------------------
	// ORACLE CAPTURE ONLY -- phys_fn_001305 (0x25960), the debug-render row
	// BOX slot 3 calls. Not a family: no digest, no registration. Truth
	// table the listing implies: guards A (.data 0x123bc8) and B
	// (0x123bd8) SKIP their blocks on zero (fld zero; fld guard; fucompp;
	// fnstsw; test ah,0x44; jnp skip -- equality gives AH=0x40, odd
	// parity). The shipped image has both zero, so row 1 asserts the
	// no-op through a POISONED renderer vtable: any renderer call faults
	// under the guard. Row 2 writes both guards 1.0 (writable .data) and
	// records what the real binary passes to fake renderer slots +0x20
	// (index 8) and +0x38 (index 14); the scale constant 0x123b4c becomes
	// 1.0 so the recorded buffers are plain pose data.
	{
	unsigned char shapeR[0x228];
	memset(shapeR, 0xcd, sizeof(shapeR));
	typedef void (__thiscall* NxBoxCtorRFn)(void* self, void* owner, unsigned argument);
	NxBoxCtorRFn boxCtorR = (NxBoxCtorRFn) (base + 0x00021870);
	boxCtorR(shapeR, 0, 0);

	void* rendererTable[16];
	for(unsigned i = 0; i < 16; ++i)
		rendererTable[i] = reinterpret_cast<void*>(0xdeadbe00u + i);
	rendererTable[8] = reinterpret_cast<void*>(&nxRenderOn20);
	rendererTable[14] = reinterpret_cast<void*>(&nxRenderOn38);
	// The oracle double-dereferences: mov eax,[ebx] = vtable, call
	// [eax+0x20]. Pass the ADDRESS OF the table as the renderer object.
	void* rendererObject[1] = { rendererTable };
	void** renderer = rendererObject;

	typedef void (__thiscall* NxDebugRenderFn)(void* self, void* renderer);
	NxDebugRenderFn debugRender = (NxDebugRenderFn) (base + 0x00025960);

	// Bind the candidate to the SAME live storage the drive mutates.
	nxBindDebugRenderGuards(
		reinterpret_cast<float*>(const_cast<unsigned char*>(base) + 0x123bc8),
		reinterpret_cast<float*>(const_cast<unsigned char*>(base) + 0x123bd8),
		reinterpret_cast<float*>(const_cast<unsigned char*>(base) + 0x123b4c),
		reinterpret_cast<float*>(const_cast<unsigned char*>(base) + 0x1041f0));
	BoxShape& shapeRender = *reinterpret_cast<BoxShape*>(shapeR);

	// Row 1: shipped guards (0.0) -- the poisoned renderer object proves
	// the no-op: any vtable dispatch faults on the 0xcd fill.
	gRenderN20 = 0;
	gRenderN38 = 0;
	{
	unsigned char recPoison[0x40];
	memset(recPoison, 0xcd, sizeof(recPoison));
	void* poisoned[16];
	for(unsigned i = 0; i < 16; ++i)
		poisoned[i] = recPoison;			// *[ *[obj] + 0x20 ] faults on call
	debugRender(shapeR, poisoned);
	}
	if(gRenderN20 != 0 || gRenderN38 != 0)
		{
		fprintf(stderr, "FAIL rendercap row1 guards-zero drew n20=%u n38=%u\n",
			gRenderN20, gRenderN38);
		return 1;
		}
	printf("rendercap row1 guards-zero calls=0\n");

	// Independently drive both guards, including negative-zero equality.
	// All three globals share this page. Restore exact bits before assertions.
	unsigned char* wbase = const_cast<unsigned char*>(base);
	unsigned saved[3];
	const unsigned guardRvas[3] = { 0x123bc8, 0x123bd8, 0x123b4c };
	for(unsigned i = 0; i < 3; ++i) memcpy(saved + i, base + guardRvas[i], 4);
	if((saved[0] & 0x7fffffffu) || (saved[1] & 0x7fffffffu))
		return nxFail("rendercap shipped guards are not zero");
	const unsigned one = 0x3f800000u;
	const unsigned colors[3] = { 0xcf0000u, 0xcf00u, 0xcfu };
	for(unsigned mask = 0; mask < 6; ++mask)
		{
		unsigned bits[3] = { (mask & 1) ? one : 0u, (mask & 2) ? one : 0u,
			mask == 5 ? 0x40000000u : one };
		if(mask == 4) bits[0] = bits[1] = 0x80000000u;
		DWORD oldProt = 0, ignored = 0;
		if(!VirtualProtect(wbase + 0x123b4c, 0x90, PAGE_READWRITE, &oldProt))
			return nxFail("cannot unlock the guard constants");
		for(unsigned i = 0; i < 3; ++i) memcpy(wbase + guardRvas[i], bits + i, 4);
		gRenderN20 = gRenderN38 = 0;
		memset(gRenderLine, 0xcd, sizeof(gRenderLine));
		memset(gRenderCircle, 0xcd, sizeof(gRenderCircle));
		debugRender(shapeR, renderer);
		// Capture the ORACLE output before anything can overwrite it.
		const unsigned oN20 = gRenderN20, oN38 = gRenderN38;
		unsigned oLine[8][7], oCircle[8][16];
		memcpy(oLine, gRenderLine, sizeof(oLine));
		memcpy(oCircle, gRenderCircle, sizeof(oCircle));
		// Candidate differential inside the SAME mutated window: the
		// candidate reads the bound live guards, so it must run before the
		// restore below. Unconditional call; each block self-gates.
		unsigned cN20 = 0, cN38 = 0;
		unsigned char cLine[8][7 * 4], cCircle[8][16 * 4];
		memset(cLine, 0xcd, sizeof(cLine));
		memset(cCircle, 0xcd, sizeof(cCircle));
		gRenderN20 = gRenderN38 = 0;
		shapeRender.nxDebugRender(renderer);
		cN20 = gRenderN20;
		cN38 = gRenderN38;
		memcpy(cLine, gRenderLine, sizeof(cLine));
		memcpy(cCircle, gRenderCircle, sizeof(cCircle));
		for(unsigned i = 0; i < 3; ++i) memcpy(wbase + guardRvas[i], saved + i, 4);
		if(!VirtualProtect(wbase + 0x123b4c, 0x90, oldProt, &ignored))
			return nxFail("cannot restore guard page protection");
		// Lens 1: the ORACLE contract, over the captured oracle bytes.
		// The mask-5 endpoint literal derives K = scale * guardA = 2.0
		// (0x25985..0x2598e, single scaling), established by the oracle
		// itself in r10-scale.log; scale-1 masks keep the byte-identical
		// literals verified in 3z26.
		const unsigned kBits = 0x40000000u;	// 2.0f bits
		bool matches = oN20 == ((mask & 1) ? 3u : 0u)
			&& oN38 == ((mask & 2) ? 3u : 0u);
		for(unsigned i = 0; i < oN20 && i < 8; ++i)
			{
			unsigned expected[7] = { 0, 0, 0, 0, 0, 0, 0 };
			if(i < 3)
				{
				expected[3 + i] = mask == 5 ? kBits : one;
				expected[6] = colors[i];
				}
			matches = matches && memcmp(expected, oLine[i], sizeof(expected)) == 0;
			}
		for(unsigned i = 0; i < oN38 && i < 8; ++i)
			{
			unsigned expected[16] = { 0x14 };
			// Cyclic columns: identity, (y,z,x), (z,x,y), then zero center.
			for(unsigned row = 0; row < 3; ++row)
				for(unsigned col = 0; col < 3; ++col)
					expected[1 + row * 3 + col] = row == (col + i) % 3 ? one : 0u;
			expected[13] = 0xffff00ffu;
			expected[14] = 0x3fddb3d7u; // sqrt(1+1+1), slot-10 fourth word
			matches = matches && memcmp(expected, oCircle[i], sizeof(expected)) == 0;
			}
		if(!matches)
			{
			fprintf(stderr, "FAIL rendercap contract mask=%u n20=%u n38=%u\n",
				mask, oN20, oN38);
			// Row dwords, not bytes: oLine rows are 28 bytes of 7 dwords.
			for(unsigned i = 0; i < oN20 && i < 8; ++i)
				fprintf(stderr, " oracle line %u: %08x %08x %08x %08x %08x %08x c=%08x\n",
					i, oLine[i][0], oLine[i][1], oLine[i][2],
					oLine[i][3], oLine[i][4], oLine[i][5], oLine[i][6]);
			return 1;
			}
		printf("rendercap contract mask=%u n20=%u n38=%u payloads=match\n",
			mask, oN20, oN38);

		// Lens 2: candidate-vs-oracle, byte-exact on counts and payloads.
		bool candMatches = cN20 == oN20 && cN38 == oN38;
		if(candMatches)
			{
			for(unsigned i = 0; i < oN20 && i < 8; ++i)
				candMatches = candMatches
					&& memcmp(oLine[i], cLine[i], sizeof(oLine[i])) == 0;
			for(unsigned i = 0; i < oN38 && i < 8; ++i)
				candMatches = candMatches
					&& memcmp(oCircle[i], cCircle[i], sizeof(oCircle[i])) == 0;
			}
		if(!candMatches)
			{
			fprintf(stderr, "FAIL rendercap candidate mask=%u c(%u,%u) expected(%u,%u)\n",
				mask, cN20, cN38, oN20, oN38);
			return 1;
			}
		printf("rendercap candidate mask=%u agree n20=%u n38=%u\n",
			mask, cN20, cN38);
		}
	printf("rendercap candidate masks=6 agree\n");
	}

	// -----------------------------------------------------------------------
	// ORACLE CAPTURE ONLY -- BOX slot 3, phys_fn_000945 (0x207e0, 104 B),
	// the debug-render dispatcher. Not a family: no digest, no registration.
	// Decode: (1) call 001287 -- zero return exits; (2) call 001305 with the
	// renderer argument; (3) guard ref(0x1041f0) vs C(0x123bc4), equality
	// skips the payload; (4) nxFillShapeDescriptor into a local, then
	// renderer slot +0x28 with (descriptor, branchless-computed word, 0)
	// -- color is 0xffffffff when (+0xde & 7)==0, otherwise 0xffff00ff.
	// The 60-byte descriptor is translation[3], dimensions[3], rotation[9].
	{
	unsigned char shapeS[0x228];
	memset(shapeS, 0xcd, sizeof(shapeS));
	typedef void (__thiscall* NxBoxCtorSFn)(void* self, void* owner, unsigned argument);
	NxBoxCtorSFn boxCtorS = (NxBoxCtorSFn) (base + 0x00021870);
	boxCtorS(shapeS, 0, 0);

	gRenderN28 = 0;
	memset(gRenderFill, 0xcd, sizeof(gRenderFill));
	gRenderArg2 = 0;
	gRenderArg3 = 0;
	void* rendererTableS[16];
	for(unsigned i = 0; i < 16; ++i)
		rendererTableS[i] = reinterpret_cast<void*>(0xdeadbe00u + i);
	rendererTableS[8] = reinterpret_cast<void*>(&nxRenderOn20);
	rendererTableS[14] = reinterpret_cast<void*>(&nxRenderOn38);
	rendererTableS[10] = reinterpret_cast<void*>(&nxRenderOn28);
	void* rendererObjectS[1] = { rendererTableS };
	void** rendererS = rendererObjectS;

	typedef void (__thiscall* NxSlot3Fn)(void* self, void* renderer);
	NxSlot3Fn slot3 = (NxSlot3Fn) (base + 0x000207e0);

	// Independent axes catch wrong enable-bit tests, bit-2-only colors,
	// equality/unordered guard mistakes, and descriptor field-order errors.
	// Guard A/B stay at their verified shipped zero values in this contract.
	const unsigned guardCases[4] = { 0, 0x80000000u, 0x3f800000u, 0x7fc00000u };
	const unsigned expectedColors[8] = { 0xffffffffu, 0xffff00ffu,
		0xffff00ffu, 0xffff00ffu, 0xffff00ffu, 0xffff00ffu, 0xffff00ffu, 0xffff00ffu };
	const unsigned expectedFill[15] = {
		0x40800000u, 0xc0000000u, 0x41000000u, // translation: 4,-2,8
		0x3f800000u, 0x40000000u, 0x40400000u, // dimensions: 1,2,3
		0, 0x3f800000u, 0, 0, 0, 0x3f800000u, 0x3f800000u, 0, 0 };
	memcpy(shapeS + 0x30, expectedFill, 12);
	memcpy(shapeS + 0xe4, expectedFill + 3, 12);
	memcpy(shapeS + 0x0c, expectedFill + 6, 36);
	BoxShape& shapeS_obj = *reinterpret_cast<BoxShape*>(shapeS);
	const float* guardCRva = reinterpret_cast<const float*>(base + 0x123bc4);
	nxBindDebugRenderGuardC(const_cast<float*>(guardCRva));
	unsigned cases = 0;
	unsigned lastCandN28 = 0;
	for(unsigned enabled = 0; enabled < 2; ++enabled)
	for(unsigned low = 0; low < 8; ++low)
	for(unsigned guard = 0; guard < 4; ++guard)
		{
		const unsigned short flags = static_cast<unsigned short>((enabled ? 8 : 0) | low);
		memcpy(shapeS + 0xde, &flags, 2);
		unsigned char* wbaseS = const_cast<unsigned char*>(base);
		unsigned savedC;
		memcpy(&savedC, base + 0x123bc4, 4);
		DWORD oldProtS = 0, ignoredS = 0;
		if(!VirtualProtect(wbaseS + 0x123bc4, 8, PAGE_READWRITE, &oldProtS))
			return nxFail("cannot unlock guard C");
		memcpy(wbaseS + 0x123bc4, guardCases + guard, 4);
		gRenderN20 = gRenderN38 = gRenderN28 = 0;
		memset(gRenderFill, 0xcd, sizeof(gRenderFill));
		gRenderArg2 = gRenderArg3 = 0;
		slot3(shapeS, rendererS);
		// Capture the ORACLE output before anything can overwrite it.
		const unsigned oN28 = gRenderN28;
		const unsigned oArg2 = gRenderArg2, oArg3 = gRenderArg3;
		unsigned oFill[15];
		memcpy(oFill, gRenderFill, sizeof(oFill));
		// Candidate differential inside the SAME mutated window: the
		// candidate reads the bound live guard C, so it must run before
		// the restore below.
		gRenderN20 = gRenderN38 = gRenderN28 = 0;
		memset(gRenderFill, 0xcd, sizeof(gRenderFill));
		gRenderArg2 = gRenderArg3 = 0;
		shapeS_obj.nxDebugRenderDispatch(rendererS);
		const unsigned cN28 = gRenderN28;
		const unsigned cArg2 = gRenderArg2, cArg3 = gRenderArg3;
		unsigned cFill[15];
		memcpy(cFill, gRenderFill, sizeof(cFill));
		memcpy(wbaseS + 0x123bc4, &savedC, 4);
		if(!VirtualProtect(wbaseS + 0x123bc4, 8, oldProtS, &ignoredS))
			return nxFail("cannot restore guard C page");
		const unsigned expectedCount = enabled && guard >= 2 ? 1u : 0u;
		// Lens 1: the ORACLE contract (byte-exact literals, 3z28).
		bool match = oN28 == expectedCount && gRenderN20 == 0 && gRenderN38 == 0;
		if(expectedCount)
			match = match && oArg2 == expectedColors[low] && oArg3 == 0
				&& memcmp(oFill, expectedFill, sizeof(expectedFill)) == 0;
		if(!match)
			{
			fprintf(stderr, "FAIL slot3 contract enabled=%u low=%u guard=%u n28=%u color=%08x\n",
				enabled, low, guard, oN28, oArg2);
			return 1;
			}
		// Lens 2: candidate-vs-oracle, byte-exact, with a local copy of the
		// failure payload so the report cannot be clobbered before print.
		bool candMatch = cN28 == oN28;
		unsigned fArg2 = cArg2, fArg3 = cArg3;
		unsigned fFill[15];
		memcpy(fFill, cFill, sizeof(fFill));
		if(candMatch && expectedCount)
			candMatch = cArg2 == oArg2 && cArg3 == oArg3
				&& memcmp(cFill, oFill, sizeof(cFill)) == 0;
		if(!candMatch)
			{
			fprintf(stderr, "FAIL slot3 candidate enabled=%u low=%u guard=%u n28=%u arg2=%08x arg3=%08x\n",
				enabled, low, guard, cN28, fArg2, fArg3);
			for(unsigned i = 0; i < 15; ++i)
				fprintf(stderr, " fill[%u]=%08x\n", i, fFill[i]);
			return 1;
			}
		++cases;
		lastCandN28 = cN28;
		}
	printf("slot3 contract cases=%u failures=0 mode=oracle-only\n", cases);
	printf("slot3 candidate masks=%u agree n28-first=%u\n", cases, lastCandN28);

	// Ordering + active-dependency case: with BOTH 001305 guards drawn
	// (A=B=1.0, unequal ref) AND guard C unequal, the dependency's line
	// draws must land BEFORE the descriptor draw. This is the case a
	// mutant that drops the 001305 call would fail. The oracle pair runs
	// first, then the candidate, both fully sequenced.
	{
	unsigned short* flagsDEo = reinterpret_cast<unsigned short*>(shapeS + 0xde);
	*flagsDEo = 0x000fu;	// bit 3 enabled, low bits = 7
	unsigned char* wbaseS = const_cast<unsigned char*>(base);
	float savedA, savedB, savedC;
	memcpy(&savedA, base + 0x123bc8, 4);
	memcpy(&savedB, base + 0x123bd8, 4);
	memcpy(&savedC, base + 0x123bc4, 4);
	DWORD oldProtS = 0, ignoredS = 0;
	if(!VirtualProtect(wbaseS + 0x123bc4, 0x90, PAGE_READWRITE, &oldProtS))
		return nxFail("cannot unlock the guard block");
	const unsigned drawn = 0x3f800000u;
	memcpy(wbaseS + 0x123bc8, &drawn, 4);
	memcpy(wbaseS + 0x123bd8, &drawn, 4);
	memcpy(wbaseS + 0x123bc4, &drawn, 4);
	gRenderN20 = gRenderN38 = gRenderN28 = 0;
	gRenderCallSeq = 0;
	slot3(shapeS, rendererS);
	const unsigned oCalls = gRenderN20 + gRenderN38 + gRenderN28;
	const unsigned oSeq28 = gSeqN28First;
	const unsigned oSeq20 = gSeqLast20, oSeq38 = gSeqLast38;
	gRenderN20 = gRenderN38 = gRenderN28 = 0;
	gRenderCallSeq = 0;
	shapeS_obj.nxDebugRenderDispatch(rendererS);
	const unsigned cCalls = gRenderN20 + gRenderN38 + gRenderN28;
	const unsigned cSeq28 = gSeqN28First;
	const unsigned cSeq20 = gSeqLast20, cSeq38 = gSeqLast38;
	memcpy(wbaseS + 0x123bc8, &savedA, 4);
	memcpy(wbaseS + 0x123bd8, &savedB, 4);
	memcpy(wbaseS + 0x123bc4, &savedC, 4);
	if(!VirtualProtect(wbaseS + 0x123bc4, 0x90, oldProtS, &ignoredS))
		return nxFail("cannot restore guard block page");
	bool orderOk = oCalls == 7 && cCalls == 7
		&& oSeq28 > oSeq20 && oSeq28 > oSeq38
		&& cSeq28 > cSeq20 && cSeq28 > cSeq38;
	if(!orderOk)
		{
		fprintf(stderr, "FAIL slot3 order o(calls=%u 20@%u 38=%u 28=%u) "
			"c(calls=%u 20=%u 38=%u 28=%u)\n",
			oCalls, oSeq20, oSeq38, oSeq28, cCalls, cSeq20, cSeq38, cSeq28);
		return 1;
		}
	printf("slot3 order case agree calls=%u seq28>20/38 both\n", cCalls);
	}
	}

	// -----------------------------------------------------------------------
	// BOX-table slots 11 and 13, phys_fn_000939 and phys_fn_000927. Slot 11
	// zeroes the vec3 and writes the diagonal; slot 13 writes dims into a
	// descriptor record at +0x4c then reuses the BASE save-state row.
	{
	typedef void (__thiscall* NxBoxRowFn)(void* self, float* out);
	typedef bool (__thiscall* NxBoxSaveFn)(void* self, void* record);
	NxBoxRowFn boxRow11 = (NxBoxRowFn) (base + 0x000206c0);
	NxBoxSaveFn boxSave13 = (NxBoxSaveFn) (base + 0x00020450);

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char shape[0x228];
	memset(shape, 0xcd, sizeof(shape));
	typedef void (__thiscall* NxShapeCtor3Fn)(void* self, void* owner, unsigned argument);
	NxShapeCtor3Fn boxCtor3 = (NxShapeCtor3Fn) (base + 0x00021870);
	boxCtor3(shape, 0, 0);

	float out11[4] = { 1, 1, 1, 1 };
	boxRow11(shape, out11);
	unsigned d11 = 2166136261u;
	for(int i = 0; i < 4; ++i)
		{
		unsigned w; memcpy(&w, out11 + i, 4);
		d11 = nxFold(d11, w);
		}
	oBoxSlot11Digest = d11;
	oracleDigest = nxFold(oracleDigest, d11);

	unsigned char record[0x58];
	memset(record, 0xcd, sizeof(record));
	bool saved13 = boxSave13(shape, record);
	unsigned d13 = 2166136261u;
	for(unsigned i = 0; i < sizeof(record); i += 4)
		{
		unsigned w; memcpy(&w, record + i, 4);
		d13 = nxFold(d13, w);
		}
	oBoxSlot13Digest = d13;
	oracleDigest = nxFold(oracleDigest, d13);

	unsigned ob11[4] = { 0, 0, 0, 0 };
	unsigned dims4c = 0;
	for(int i = 0; i < 4; ++i)
		memcpy(&ob11[i], out11 + i, 4);
	memcpy(&dims4c, record + 0x4c, 4);
	printf("boxrow2 slot11=phys_fn_000939 out=%08x.%08x.%08x.%08x "
		"slot13=phys_fn_000927 saved=%u digest=%08x dims_at_4c=%08x\n",
		ob11[0], ob11[1], ob11[2], ob11[3],
		saved13 ? 1u : 0u, d13, dims4c);
	}

	// -----------------------------------------------------------------------
	// BOX-table slots 8 and 9, phys_fn_000941 and phys_fn_000935: local AABB
	// and world AABB from pose one. Driven on a real constructed box
	// (identity pose, dims 1,1,1 -- exact arithmetic).
	{
	typedef void (__thiscall* NxBoxRowFn)(void* self, float* out);
	NxBoxRowFn boxRow8 = (NxBoxRowFn) (base + 0x00020700);
	NxBoxRowFn boxRow9 = (NxBoxRowFn) (base + 0x000205a0);

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char shape[0x228];
	memset(shape, 0xcd, sizeof(shape));
	typedef void (__thiscall* NxCtorFn4)(void* self, void* owner, unsigned argument);
	NxCtorFn4 boxCtor4 = (NxCtorFn4) (base + 0x00021870);
	boxCtor4(shape, 0, 0);

	float out8[6] = { 0, 0, 0, 0, 0, 0 };
	boxRow8(shape, out8);
	float out9[6] = { 0, 0, 0, 0, 0, 0 };
	boxRow9(shape, out9);

	unsigned d8 = 2166136261u;
	for(int i = 0; i < 6; ++i)
		{
		unsigned w; memcpy(&w, out8 + i, 4);
		d8 = nxFold(d8, w);
		}
	oBoxSlot8Digest = d8;
	oracleDigest = nxFold(oracleDigest, d8);

	unsigned d9 = 2166136261u;
	for(int i = 0; i < 6; ++i)
		{
		unsigned w; memcpy(&w, out9 + i, 4);
		d9 = nxFold(d9, w);
		}
	oBoxSlot9Digest = d9;
	oracleDigest = nxFold(oracleDigest, d9);

	unsigned b8[6] = { 0, 0, 0, 0, 0, 0 }, b9[6] = { 0, 0, 0, 0, 0, 0 };
	for(int i = 0; i < 6; ++i)
		{
		memcpy(&b8[i], out8 + i, 4);
		memcpy(&b9[i], out9 + i, 4);
		}
	printf("boxrow3 slot8=phys_fn_000941 minmax=%08x.%08x.%08x.%08x.%08x.%08x "
		"slot9=phys_fn_000935 minmax=%08x.%08x.%08x.%08x.%08x.%08x\n",
		b8[0], b8[1], b8[2], b8[3], b8[4], b8[5],
		b9[0], b9[1], b9[2], b9[3], b9[4], b9[5]);
	}

	// -----------------------------------------------------------------------
	// BOX-table slots 14-16, phys_fn_001391: the identity row. Driven by
	// pointer-equality on a real constructed box -- the returned address must
	// be the object itself on both sides (never folded; ASLR moves it).
	{
	typedef void* (__thiscall* NxSelfFn)(void* self);
	NxSelfFn self14 = (NxSelfFn) (base + 0x00027f00);
	NxSelfFn self15 = (NxSelfFn) (base + 0x00027f00);
	NxSelfFn self16 = (NxSelfFn) (base + 0x00027f00);

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char shape[0x228];
	memset(shape, 0xcd, sizeof(shape));
	typedef void (__thiscall* NxCtorFn5)(void* self, void* owner, unsigned argument);
	NxCtorFn5 boxCtor5 = (NxCtorFn5) (base + 0x00021870);
	boxCtor5(shape, 0, 0);

	void* r14 = self14(shape);
	void* r15 = self15(shape);
	void* r16 = self16(shape);
	bool stable = r14 == shape && r15 == shape && r16 == shape;
	oracleDigest = nxFold(oracleDigest, stable ? 1u : 0u);
	printf("boxrow4 slots14-16=phys_fn_001391 stable=%u\n", stable ? 1u : 0u);
	}

	// -----------------------------------------------------------------------
	// BOX-table slot 0, phys_fn_000979: scalar deleting destructor, flag=0
	// path. Driven on a fresh constructed box; post-dtor bytes folded under
	// the six-pointer mask (both sides' destruction-time vptrs differ by
	// module and are masked).
	{
	typedef void (__thiscall* NxBoxDtorFn)(void* self, unsigned flags);
	NxBoxDtorFn boxDtor = (NxBoxDtorFn) (base + 0x00021940);
	// +0xe0 is masked here: the dtor dance restores the facade vptr word, a
	// module-specific pointer like every other vptr slot.
	static const unsigned kPointerWords[] = { 0x00, 0x9c, 0xa4, 0xa8, 0xb0, 0xb4, 0xe0 };

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char shape[0x228];
	memset(shape, 0xcd, sizeof(shape));
	typedef void (__thiscall* NxCtorFn6)(void* self, void* owner, unsigned argument);
	NxCtorFn6 boxCtor6 = (NxCtorFn6) (base + 0x00021870);
	boxCtor6(shape, 0, 0);

	{
	unsigned colobjW = 0, cVT = 0, cSlot0 = 0;
	memcpy(&colobjW, shape + 0x9c, 4);
	if(colobjW) memcpy(&cVT, (void*) colobjW, 4);
	if(cVT) memcpy(&cSlot0, (void*) cVT, 4);
	fprintf(stderr, "DBG colobj=%08x vtbl=%08x slot0=%08x rva=%08x\n",
		colobjW, cVT, cSlot0, cSlot0 ? cSlot0 - (unsigned) (uintptr_t) base : 0);
	fflush(stderr);
	}

	unsigned faultAddr = 0, faultCode = 0;
	nxGuardedBoxDtor(boxDtor, shape, 0);
	faultCode = gDtorFaultCode; faultAddr = gDtorFaultAddr;
	if(faultCode)
		{
		fprintf(stderr, "FAIL box dtor fault code=%08x at=%08x rva=%08x\n",
			faultCode, faultAddr, faultAddr - (unsigned) (uintptr_t) base);
		return 1;
		}

	unsigned digest = 2166136261u;
	for(unsigned i = 0; i < sizeof(shape); i += 4)
		{
		bool pointer = false;
		for(size_t p = 0; p < sizeof(kPointerWords) / sizeof(kPointerWords[0]); ++p)
			if(kPointerWords[p] == i)
				pointer = true;
		if(pointer)
			continue;
		unsigned word;
		memcpy(&word, shape + i, 4);
		digest = nxFold(digest, word);
		}
	oBoxDtorDigest = digest;
	memcpy(sBoxDtorReference, shape, sizeof(shape));
	oracleDigest = nxFold(oracleDigest, digest);
	printf("boxdtor row=phys_fn_000979 digest=%08x\n", digest);
	}

	// -----------------------------------------------------------------------
	// PLANE-table slot 13, phys_fn_001251: save-to-descriptor with the
	// NEGATED distance. Driven on a real constructed plane; every written
	// byte is module-independent.
	{
	typedef bool (__thiscall* NxPlaneSaveFn)(void* self, void* record);
	NxPlaneSaveFn planeSave = (NxPlaneSaveFn) (base + 0x00024f80);

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char shape[0x10c];
	memset(shape, 0xcd, sizeof(shape));
	typedef void (__thiscall* NxCtorFn7)(void* self, void* owner, unsigned argument);
	NxCtorFn7 planeCtor2 = (NxCtorFn7) (base + 0x00024ed0);
	planeCtor2(shape, 0, 0);

	unsigned char record[0x58];
	memset(record, 0xcd, sizeof(record));
	bool saved = planeSave(shape, record);

	unsigned digest = 2166136261u;
	for(unsigned i = 0; i < sizeof(record); i += 4)
		{
		unsigned word;
		memcpy(&word, record + i, 4);
		digest = nxFold(digest, word);
		}
	oPlaneSaveDigest = digest;
	oracleDigest = nxFold(oracleDigest, digest);

	unsigned normal = 0, negD = 0;
	memcpy(&normal, record + 0x50, 4);
	memcpy(&negD, record + 0x58, 4);
	printf("planesave row=phys_fn_001251 saved=%u digest=%08x normal_y=%08x neg_d=%08x\n",
		saved ? 1u : 0u, digest, normal, negD);
	}

	// -----------------------------------------------------------------------
	// SPHERE-table slots 15/13/11/10, phys_fn_001359/1355/1365/1363: radius
	// getter, radius save-state, zero-center+radius, center+radius. Driven on
	// a real constructed sphere (fresh radius = 0; every output byte is a
	// deterministic zero or the untouched poison).
	{
	typedef float (__thiscall* NxSphRadFn)(void* self);
	typedef bool (__thiscall* NxSphSaveFn)(void* self, void* record);
	typedef void (__thiscall* NxSphOutFn)(void* self, float* out);
	NxSphRadFn sphRad = (NxSphRadFn) (base + 0x00027920);
	NxSphSaveFn sphSave = (NxSphSaveFn) (base + 0x000278a0);
	NxSphOutFn sphZCR = (NxSphOutFn) (base + 0x000279b0);
	NxSphOutFn sphCR = (NxSphOutFn) (base + 0x00027980);

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char shape[0xe4];
	memset(shape, 0xcd, sizeof(shape));
	typedef void (__thiscall* NxCtorFn8)(void* self, void* owner, unsigned argument);
	NxCtorFn8 sphereCtor3 = (NxCtorFn8) (base + 0x000277c0);
	sphereCtor3(shape, 0, 0);

	float rad = sphRad(shape);
	unsigned radBits = 0;
	memcpy(&radBits, &rad, 4);

	unsigned char record[0x58];
	memset(record, 0xcd, sizeof(record));
	bool saved13 = sphSave(shape, record);

	float out11[4] = { 1, 1, 1, 1 };
	sphZCR(shape, out11);
	float out10[4] = { 1, 1, 1, 1 };
	sphCR(shape, out10);

	unsigned d13 = 2166136261u;
	for(unsigned i = 0; i < sizeof(record); i += 4)
		{
		unsigned w; memcpy(&w, record + i, 4);
		d13 = nxFold(d13, w);
		}
	oSphereRowsDigest = d13;
	oSphereRadBits = radBits;
	oracleDigest = nxFold(oracleDigest, d13);
	oracleDigest = nxFold(oracleDigest, radBits);
	unsigned z11[4] = { 0, 0, 0, 0 }, c10[4] = { 0, 0, 0, 0 };
	for(int i = 0; i < 4; ++i)
		{
		memcpy(&z11[i], out11 + i, 4);
		memcpy(&c10[i], out10 + i, 4);
		}
	printf("sphererows r15=%08x save13=%u d13=%08x zcr=%08x.%08x.%08x.%08x "
		"cr=%08x.%08x.%08x.%08x\n",
		radBits, saved13 ? 1u : 0u, d13,
		z11[0], z11[1], z11[2], z11[3], c10[0], c10[1], c10[2], c10[3]);
	}

	// -----------------------------------------------------------------------
	// CAPSULE-table slot 13, phys_fn_000991: radius / 2*half-height / raw
	// third word into the descriptor, then the BASE save row. Driven on a
	// real constructed capsule; the untouched +0xe8 poison is identical.
	{
	typedef bool (__thiscall* NxCapSaveFn)(void* self, void* record);
	NxCapSaveFn capSave = (NxCapSaveFn) (base + 0x00021b40);

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char shape[0xec];
	memset(shape, 0xcd, sizeof(shape));
	typedef void (__thiscall* NxCtorFn9)(void* self, void* owner, unsigned argument);
	NxCtorFn9 capsuleCtor2 = (NxCtorFn9) (base + 0x00021a60);
	capsuleCtor2(shape, 0, 0);

	unsigned char record[0x58];
	memset(record, 0xcd, sizeof(record));
	bool saved = capSave(shape, record);

	unsigned digest = 2166136261u;
	for(unsigned i = 0; i < sizeof(record); i += 4)
		{
		unsigned w; memcpy(&w, record + i, 4);
		digest = nxFold(digest, w);
		}
	oCapsuleSaveDigest = digest;
	memcpy(sCapsuleSaveReference, record, sizeof(record));
	oracleDigest = nxFold(oracleDigest, digest);

	unsigned rad4c = 0, hgt50 = 0;
	memcpy(&rad4c, record + 0x4c, 4);
	memcpy(&hgt50, record + 0x50, 4);
	printf("capsave row=phys_fn_000991 saved=%u digest=%08x rad4c=%08x hgt50=%08x\n",
		saved ? 1u : 0u, digest, rad4c, hgt50);
	}

	// -----------------------------------------------------------------------
	// MESH-table slot 17, phys_fn_001381: dereferences the +0xe0 pointer and
	// returns its +0xe4 word. The probe plants a record for it on both sides
	// because the real mesh assignment is a later task.
	{
	typedef unsigned (__thiscall* NxMeshWordFn)(void* self);
	NxMeshWordFn meshWord = (NxMeshWordFn) (base + 0x00027e20);

	unsigned char shape[0xe8];
	memset(shape, 0xcd, sizeof(shape));
	typedef void (__thiscall* NxCtorFn10)(void* self, void* owner, unsigned argument);
	NxCtorFn10 meshCtor2 = (NxCtorFn10) (base + 0x00027db0);
	meshCtor2(shape, 0, 0);

	static unsigned char fakeMesh[0xe8];
	memset(fakeMesh, 0, sizeof(fakeMesh));
	const unsigned kMark = 0x13572468u;
	memcpy(fakeMesh + 0xe4, &kMark, 4);
	void* fm = fakeMesh;
	memcpy(shape + 0xe0, &fm, 4);			// plant

	unsigned got = meshWord(shape);
	bool hit = got == kMark;
	oracleDigest = nxFold(oracleDigest, hit ? 1u : 0u);
	printf("meshword row=phys_fn_001381 mark_hit=%u\n", hit ? 1u : 0u);
	}

	// -----------------------------------------------------------------------
	// MESH slot 8, phys_fn_001389: copies six words from mesh+0x44 to out.
	{
	typedef void (__thiscall* NxWords44Fn)(void* self, unsigned* out);
	NxWords44Fn words44 = (NxWords44Fn) (base + 0x00027ec0);

	unsigned char mshape[0xe8];
	memset(mshape, 0xcd, sizeof(mshape));
	typedef void (__thiscall* NxCtorFn14)(void* self, void* owner, unsigned argument);
	NxCtorFn14 meshCtor5 = (NxCtorFn14) (base + 0x00027db0);
	meshCtor5(mshape, 0, 0);

	static unsigned char fm44[0x100];
	memset(fm44, 0, sizeof(fm44));
	const unsigned kM44 = 0xdeadbeefu, kM48 = 0xcafebabeu, kM4c = 0x12345678u;
	memcpy(fm44 + 0x44, &kM44, 4);
	memcpy(fm44 + 0x48, &kM48, 4);
	memcpy(fm44 + 0x4c, &kM4c, 4);
	void* fmp2 = fm44;
	memcpy(mshape + 0xe0, &fmp2, 4);

	unsigned out44[6] = { 0, 0, 0, 0, 0, 0 };
	words44(mshape, out44);

	unsigned d44 = 2166136261u;
	for(int i = 0; i < 6; ++i)
		{
		unsigned w; memcpy(&w, out44 + i, 4);
		d44 = nxFold(d44, w);
		}
	oMesh44Digest = d44;
	oracleDigest = nxFold(oracleDigest, d44);

	unsigned ab44[6] = { 0, 0, 0, 0, 0, 0 };
	for(int i = 0; i < 6; ++i)
		memcpy(&ab44[i], out44 + i, 4);
	printf("meshwords44 row=phys_fn_001389 out=%08x.%08x.%08x.%08x.%08x.%08x\n",
		ab44[0], ab44[1], ab44[2], ab44[3], ab44[4], ab44[5]);
	}

	// -----------------------------------------------------------------------
	// SPHERE slot 9 (phys_fn_001361) world AABB and CAPSULE slots 10/11
	// (phys_fn_001001 / phys_fn_001003) center+radius rows. Fresh shapes:
	// radius/half-height zero, so every output word is a deterministic zero.
	{
	typedef void (__thiscall* NxOutFn)(void* self, float* out);
	NxOutFn sphAABB = (NxOutFn) (base + 0x00027930);
	NxOutFn capCR = (NxOutFn) (base + 0x00021c30);
	NxOutFn capZCR = (NxOutFn) (base + 0x00021c60);

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char sshape[0xe4];
	memset(sshape, 0xcd, sizeof(sshape));
	typedef void (__thiscall* NxCtorFna)(void* self, void* owner, unsigned argument);
	NxCtorFna sphereCtor4 = (NxCtorFna) (base + 0x000277c0);
	sphereCtor4(sshape, 0, 0);
	float so[6] = { 0, 0, 0, 0, 0, 0 };
	sphAABB(sshape, so);

	unsigned char cshape[0xec];
	memset(cshape, 0xcd, sizeof(cshape));
	NxCtorFna capsuleCtor3 = (NxCtorFna) (base + 0x00021a60);
	capsuleCtor3(cshape, 0, 0);
	float c10[4] = { 0, 0, 0, 0 };
	capCR(cshape, c10);
	float c11[4] = { 0, 0, 0, 0 };
	capZCR(cshape, c11);

	unsigned d9 = 2166136261u;
	for(int i = 0; i < 6; ++i)
		{
		unsigned w; memcpy(&w, so + i, 4);
		d9 = nxFold(d9, w);
		}
	oSphereAABBDigest = d9;
	oracleDigest = nxFold(oracleDigest, d9);

	unsigned dc = 2166136261u;
	for(int i = 0; i < 4; ++i)
		{
		unsigned w; memcpy(&w, c10 + i, 4);
		dc = nxFold(dc, w);
		}
	for(int i = 0; i < 4; ++i)
		{
		unsigned w; memcpy(&w, c11 + i, 4);
		dc = nxFold(dc, w);
		}
	oCapsuleCRDigest = dc;
	oracleDigest = nxFold(oracleDigest, dc);

	unsigned sb[6] = { 0, 0, 0, 0, 0, 0 }, cb10[4] = { 0, 0, 0, 0 }, cb11[4] = { 0, 0, 0, 0 };
	for(int i = 0; i < 6; ++i)
		memcpy(&sb[i], so + i, 4);
	for(int i = 0; i < 4; ++i)
		{
		memcpy(&cb10[i], c10 + i, 4);
		memcpy(&cb11[i], c11 + i, 4);
		}
	printf("aabbrows sph9=phys_fn_001361 minmax=%08x.%08x.%08x.%08x.%08x.%08x "
		"cap10=phys_fn_001001 cr=%08x.%08x.%08x.%08x "
		"cap11=phys_fn_001003 cr=%08x.%08x.%08x.%08x\n",
		sb[0], sb[1], sb[2], sb[3], sb[4], sb[5],
		cb10[0], cb10[1], cb10[2], cb10[3],
		cb11[0], cb11[1], cb11[2], cb11[3]);
	}

	// -----------------------------------------------------------------------
	// MESH-table slots 13 and 11, phys_fn_001385 / phys_fn_001387. The probe
	// plants a fake mesh record (marked +0xe4 word and marked 0x5c quad) on
	// both sides; every compared byte is then module-independent.
	{
	typedef bool (__thiscall* NxMeshSaveFn)(void* self, void* record);
	typedef void (__thiscall* NxMeshWordsFn)(void* self, unsigned* out);
	NxMeshSaveFn meshSave13 = (NxMeshSaveFn) (base + 0x00027e60);
	NxMeshWordsFn meshWords11 = (NxMeshWordsFn) (base + 0x00027e90);

	unsigned char shape[0xe8];
	memset(shape, 0xcd, sizeof(shape));
	typedef void (__thiscall* NxCtorFnb)(void* self, void* owner, unsigned argument);
	NxCtorFnb meshCtor3 = (NxCtorFnb) (base + 0x00027db0);
	meshCtor3(shape, 0, 0);

	static unsigned char fm[0xe8];
	memset(fm, 0, sizeof(fm));
	const unsigned kM4 = 0x2468ace0u, kW0 = 0x0badf00du, kW3 = 0x13579bdfu;
	memcpy(fm + 0xe4, &kM4, 4);
	memcpy(fm + 0x5c, &kW0, 4);
	memcpy(fm + 0x68, &kW3, 4);
	void* fmp = fm;
	memcpy(shape + 0xe0, &fmp, 4);
	const unsigned kFlags = 0x5a5a5a5au;
	memcpy(shape + 0xe4, &kFlags, 4);

	unsigned char record[0x58];
	memset(record, 0xcd, sizeof(record));
	bool saved = meshSave13(shape, record);
	unsigned words[4] = { 0, 0, 0, 0 };
	meshWords11(shape, words);

	unsigned d13m = 2166136261u;
	for(unsigned i = 0; i < sizeof(record); i += 4)
		{
		unsigned w; memcpy(&w, record + i, 4);
		d13m = nxFold(d13m, w);
		}
	oMeshSaveDigest = d13m;
	unsigned dw = 2166136261u;
	for(int i = 0; i < 4; ++i)
		dw = nxFold(dw, words[i]);
	oMeshWordsDigest = dw;
	oracleDigest = nxFold(oracleDigest, d13m);
	oracleDigest = nxFold(oracleDigest, dw);
	printf("meshrows slot13=phys_fn_001385 saved=%u digest=%08x "
		"slot11=phys_fn_001387 words=%08x.%08x.%08x.%08x\n",
		saved ? 1u : 0u, d13m, words[0], words[1], words[2], words[3]);
	}

	// -----------------------------------------------------------------------
	// SPHERE slot 8, phys_fn_001367: local AABB. On a fresh sphere the mins
	// are -0.0f (0x80000000) and maxes +0.0f -- the sign bits are the claim.
	{
	typedef void (__thiscall* NxOutFn)(void* self, float* out);
	NxOutFn sphLocal = (NxOutFn) (base + 0x000279d0);

	unsigned char sshape[0xe4];
	memset(sshape, 0xcd, sizeof(sshape));
	typedef void (__thiscall* NxCtorFnc)(void* self, void* owner, unsigned argument);
	NxCtorFnc sphereCtor5 = (NxCtorFnc) (base + 0x000277c0);
	sphereCtor5(sshape, 0, 0);

	float lo[6] = { 0, 0, 0, 0, 0, 0 };
	sphLocal(sshape, lo);

	unsigned d8s = 2166136261u;
	for(int i = 0; i < 6; ++i)
		{
		unsigned w; memcpy(&w, lo + i, 4);
		d8s = nxFold(d8s, w);
		}
	oSphereLocalDigest = d8s;
	oracleDigest = nxFold(oracleDigest, d8s);

	unsigned lb[6] = { 0, 0, 0, 0, 0, 0 };
	for(int i = 0; i < 6; ++i)
		memcpy(&lb[i], lo + i, 4);
	printf("sphlocal row=phys_fn_001367 minmax=%08x.%08x.%08x.%08x.%08x.%08x\n",
		lb[0], lb[1], lb[2], lb[3], lb[4], lb[5]);
	}

	// -----------------------------------------------------------------------
	// SPHERE slot 14, phys_fn_001357: set-radius. Driven with a valid radius
	// on both sides (the invalid-report arm needs Task 2's error stream).
	{
	typedef void (__thiscall* NxSetRadFn)(void* self, float r);
	NxSetRadFn setRad = (NxSetRadFn) (base + 0x000278c0);
	const float kR = 1.25f;

	unsigned char sshape[0xe4];
	memset(sshape, 0xcd, sizeof(sshape));
	typedef void (__thiscall* NxCtorFnd)(void* self, void* owner, unsigned argument);
	NxCtorFnd sphereCtor6 = (NxCtorFnd) (base + 0x000277c0);
	sphereCtor6(sshape, 0, 0);
	setRad(sshape, kR);

	float got = 0.0f;
	memcpy(&got, sshape + 0xe0, 4);
	unsigned radBits = 0;
	memcpy(&radBits, &got, 4);
	oSphereSetDigest = radBits;
	oracleDigest = nxFold(oracleDigest, radBits);

	const float kR2 = kR;
	unsigned cBits = 0;
	memcpy(&cBits, &kR2, 4);
	printf("setrad row=phys_fn_001357 stored=%08x expected=%08x\n",
		radBits, cBits);
	}

	// -----------------------------------------------------------------------
	// CAPSULE slot 14, phys_fn_000995: set-radius. Driven with 1.5f.
	{
	typedef void (__thiscall* NxSetRadFn)(void* self, float r);
	NxSetRadFn capSetRad = (NxSetRadFn) (base + 0x00021be0);
	const float kR = 1.5f;

	unsigned char cshape[0xec];
	memset(cshape, 0xcd, sizeof(cshape));
	typedef void (__thiscall* NxCtorFne)(void* self, void* owner, unsigned argument);
	NxCtorFne capsuleCtor4 = (NxCtorFne) (base + 0x00021a60);
	capsuleCtor4(cshape, 0, 0);
	capSetRad(cshape, kR);

	float got = 0.0f;
	memcpy(&got, cshape + 0xe0, 4);
	unsigned radBits = 0;
	memcpy(&radBits, &got, 4);
	oCapsuleSetDigest = radBits;
	oracleDigest = nxFold(oracleDigest, radBits);

	unsigned cBits = 0;
	memcpy(&cBits, &kR, 4);
	printf("capsetrad row=phys_fn_000995 stored=%08x expected=%08x\n",
		radBits, cBits);
	}

	// -----------------------------------------------------------------------
	// SPHERE slot 0, phys_fn_001375: scalar deleting destructor, flag=0.
	{
	typedef void (__thiscall* NxSphDtorFn)(void* self, unsigned flags);
	NxSphDtorFn sphDtor = (NxSphDtorFn) (base + 0x00027c30);

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char shape[0xe4];
	memset(shape, 0xcd, sizeof(shape));
	typedef void (__thiscall* NxCtorFng)(void* self, void* owner, unsigned argument);
	NxCtorFng sphereCtor7 = (NxCtorFng) (base + 0x000277c0);
	sphereCtor7(shape, 0, 0);

	unsigned fc = gDtorFaultCode, fa = gDtorFaultAddr;
	nxGuardedBoxDtor((NxBoxDtorFn) sphDtor, shape, 0);
	fc = gDtorFaultCode; fa = gDtorFaultAddr;
	if(fc)
		return nxFail("sphere dtor faulted");

	unsigned digest = 2166136261u;
	for(unsigned i = 0; i < sizeof(shape); i += 4)
		{
		bool pointer = false;
		for(size_t p = 0; p < sizeof(kDtorMask) / sizeof(kDtorMask[0]); ++p)
			if(kDtorMask[p] == i)
				pointer = true;
		if(pointer)
			continue;
		unsigned w; memcpy(&w, shape + i, 4);
		digest = nxFold(digest, w);
		}
	oSphereDtorDigest = digest;
	oracleDigest = nxFold(oracleDigest, digest);
	printf("sphdtor row=phys_fn_001375 digest=%08x\n", digest);
	}

	// SPHERE slot 12, phys_fn_001353: loadFromDesc. NULL name record.
	{
	typedef void (__thiscall* NxSphLoadFn)(void* self, const void* rec);
	NxSphLoadFn sphLoad = (NxSphLoadFn) (base + 0x00027850);

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char sshape[0xe4];
	memset(sshape, 0xcd, sizeof(sshape));
	typedef void (__thiscall* NxCtorFnk)(void* self, void* owner, unsigned argument);
	NxCtorFnk sphereCtor10 = (NxCtorFnk) (base + 0x000277c0);
	sphereCtor10(sshape, 0, 0);

	unsigned char record[0x58];
	memset(record, 0xcd, sizeof(record));
	const float kR = 2.5f;
	memcpy(record + 0x4c, &kR, 4);
	const unsigned short kGrp = 0x0006u;
	memcpy(record + 0x3c, &kGrp, 2);
	memset(record + 0x44, 0, 4);

	sphLoad(sshape, record);

	float gotRad = 0.0f;
	memcpy(&gotRad, sshape + 0xe0, 4);
	unsigned radBits = 0;
	memcpy(&radBits, &gotRad, 4);
	oSphereLoadRadBits = radBits;

	unsigned short hwD8 = 0;
	memcpy(&hwD8, sshape + 0xd8, 2);
	oSphereLoadGroup = hwD8;

	oracleDigest = nxFold(oracleDigest, radBits);
	oracleDigest = nxFold(oracleDigest, hwD8);
	printf("sphload row=phys_fn_001353 rad=%08x group=%04x\n", radBits, hwD8);
	}

	// sphere; +0xd8 must carry it (the dirty-flag arm is a null-owner no-op).
	{
	typedef void (__thiscall* NxGroupFn)(void* self, unsigned short g);
	NxGroupFn setGroup = (NxGroupFn) (base + 0x00026d90);

	unsigned char sshape[0xe4];
	memset(sshape, 0xcd, sizeof(sshape));
	typedef void (__thiscall* NxCtorFni)(void* self, void* owner, unsigned argument);
	NxCtorFni sphereCtor8 = (NxCtorFni) (base + 0x000277c0);
	sphereCtor8(sshape, 0, 0);

	setGroup(sshape, 7);

	unsigned short hw = 0;
	memcpy(&hw, sshape + 0xd8, 2);
	oGroupDigest = hw;
	oracleDigest = nxFold(oracleDigest, hw);
	printf("setgroup row=phys_fn_001329 hw_d8=%04x expected=0007\n", hw);
	}

	// -----------------------------------------------------------------------
	// BASE slot 6, phys_fn_001315: owner-update no-op for detached shapes.
	{
	typedef void (__thiscall* NxOwnerUpdFn)(void* self, unsigned flags);
	NxOwnerUpdFn ownerUpd = (NxOwnerUpdFn) (base + 0x000266a0);

	unsigned char oshape[0xe4];
	memset(oshape, 0xcd, sizeof(oshape));
	typedef void (__thiscall* NxCtorFnOU)(void* self, void* owner, unsigned argument);
	NxCtorFnOU sphereCtorOU = (NxCtorFnOU) (base + 0x000277c0);
	sphereCtorOU(oshape, 0, 0);

	unsigned char pre[16];
	memcpy(pre, oshape, sizeof(pre));
	ownerUpd(oshape, 0);

	bool noop = memcmp(pre, oshape, sizeof(pre)) == 0;
	oracleDigest = nxFold(oracleDigest, noop ? 1u : 0u);
	printf("ownerupd row=phys_fn_001315 noop=%u\n", noop ? 1u : 0u);
	}

	// -----------------------------------------------------------------------
	// SPHERE slot 4, phys_fn_000851: the compute-mass row. Two drives -- a
	// scaled density (2.0f, through 0x1c5c0) and the exact .rdata 1.0f
	// sentinel (skipping it) -- folded over all thirteen words of each
	// destination frame. The optional payload pointer stays null: its
	// parallel-axis pair (0x1bdc0/0x1c040) is not decoded yet.
	{
	typedef void (__thiscall* NxMassFn)(void* self, float density,
		float radius, const void* extra);
	NxMassFn massFn = (NxMassFn) (base + 0x0001c930);

	unsigned char destA[0x34];
	memset(destA, 0, sizeof(destA));
	massFn(destA, 2.0f, 2.5f, 0);

	unsigned char destB[0x34];
	memset(destB, 0, sizeof(destB));
	massFn(destB, 1.0f, 2.5f, 0);

	unsigned md = 2166136261u;
	for(int v = 0; v < 2; ++v)
		{
		const unsigned char* src = (v == 0) ? destA : destB;
		for(int i = 0; i < 0x34; i += 4)
			{
			unsigned w;
			memcpy(&w, src + i, 4);
			md = nxFold(md, w);
			}
		}
	oMassFrameDigest = md;

	unsigned massA, massB;
	memcpy(&massA, destA + 0x30, 4);
	memcpy(&massB, destB + 0x30, 4);
	oracleDigest = nxFold(oracleDigest, md);
	printf("massframe row=phys_fn_000851 mass_scaled=%08x mass_unit=%08x digest=%08x\n",
		massA, massB, md);
	}

	// -----------------------------------------------------------------------
	// Actual BOX slot 4, phys_fn_000947: distinct from its mass helper.
	// Separate objects prevent accidental oracle dispatch on candidate side.
	{
	typedef bool (__thiscall* Slot4MassFn)(void*, void*, float, unsigned);
	Slot4MassFn oracleSlot4 = reinterpret_cast<Slot4MassFn>(base + 0x20850);
	typedef void (__thiscall* BoxCtorMassFn)(void*, void*, unsigned);
	BoxCtorMassFn oracleCtor = reinterpret_cast<BoxCtorMassFn>(base + 0x21870);
	unsigned char oracleShape[0x228], candidateShape[0x228];
	memset(oracleShape, 0xcd, sizeof(oracleShape));
	memset(candidateShape, 0xcd, sizeof(candidateShape));
	oracleCtor(oracleShape, 0, 0);
	BoxShape& candidate = *new(candidateShape) BoxShape(0, 0);
	const float dims[3] = { 1, 2, 3 };
	float poses[4][12] = {
		{ 1,0,0, 0,1,0, 0,0,1, 0,0,0 },
		{ 0,1,0, 0,0,1, 1,0,0, 0,0,0 },
		{ 1,0,0, 0,1,0, 0,0,1, 2,-3,4 } };
	// Combined non-identity rotation + non-axis translation (a proper
	// 30-degree rotation about z, then a general translation), which
	// exercises the full rotation+translation pose path rather than each
	// alone. Values are not axis-aligned, so a transpose/association error
	// in nxBoxComputeMassFrame's pose arm is exposed.
	memcpy(poses[3], poses[2], sizeof(poses[2]));
	{
	const float c30 = 0.866025403784f, s30 = 0.5f;
	// Proper 30-degree rotation about z, row-major (3 rows x 3 cols):
	// row0 = (c, -s, 0), row1 = (s, c, 0), row2 = (0, 0, 1). The
	// rotation+translation pose is not axis-aligned, so a transpose or
	// association error in the mass arm is exposed. NOTE: the box-mass
	// pose path reads the rotation through nxMassFrameFoldPayload, which
	// treats the nine words as a SYMMETRIC matrix k00..k22 -- a general
	// 30-degree-about-z rotation is asymmetric, so this fixture verifies
	// how (or whether) the fold degrades a non-symmetric input against the
	// oracle's identical read.
	poses[3][0]=c30; poses[3][1]=-s30; poses[3][2]=0;
	poses[3][3]=s30; poses[3][4]=c30; poses[3][5]=0;
	poses[3][6]=0; poses[3][7]=0; poses[3][8]=1;
	poses[3][9]=2; poses[3][10]=-3; poses[3][11]=4;
	}
	unsigned caseCount = 0;
	for(unsigned pose = 0; pose < 4; ++pose)
	for(unsigned low = 0; low < 8; ++low)
	for(unsigned densityCase = 0; densityCase < 2; ++densityCase)
		{
		memcpy(oracleShape + 0xe4, dims, sizeof(dims));
		memcpy(candidateShape + 0xe4, dims, sizeof(dims));
		memcpy(oracleShape + 0x6c, poses[pose], sizeof(poses[pose]));
		memcpy(candidateShape + 0x6c, poses[pose], sizeof(poses[pose]));
		const unsigned short flags = static_cast<unsigned short>(8 | low);
		memcpy(oracleShape + 0xde, &flags, 2);
		memcpy(candidateShape + 0xde, &flags, 2);
		MassFrame oFrame, cFrame;
		memset(&oFrame, 0, sizeof(oFrame));
		memset(&cFrame, 0, sizeof(cFrame));
		// Nonzero destination catches overwrite-instead-of-merge and
		// destructive flag-suppression paths that zero-filled fixtures hide.
		oFrame.mInertia[0] = cFrame.mInertia[0] = 2.0f;
		oFrame.mInertia[4] = cFrame.mInertia[4] = 3.0f;
		oFrame.mInertia[8] = cFrame.mInertia[8] = 5.0f;
		oFrame.mMass = cFrame.mMass = 1.0f;
		MassFrame untouched = oFrame;
		const float density = densityCase ? 2.0f : 1.0f;
		const bool oRet = oracleSlot4(oracleShape, &oFrame, density, 0xdeadbeefu);
		const bool cRet = candidate.nxBoxAccumulateMass(&cFrame, density, 0xdeadbeefu);
		if(!oRet || !cRet || memcmp(&oFrame, &cFrame, sizeof(oFrame)) != 0
			|| (low != 0 && memcmp(&oFrame, &untouched, sizeof(oFrame)) != 0))
			{
			fprintf(stderr, "FAIL boxslot4 pose=%u low=%u density=%u returns=%u/%u\n",
				pose, low, densityCase, oRet, cRet);
			unsigned ow[13], cw[13];
			memcpy(ow, &oFrame, sizeof(ow)); memcpy(cw, &cFrame, sizeof(cw));
			for(unsigned i=0; i<13; ++i)
				if(ow[i] != cw[i]) fprintf(stderr, " mass[%u] oracle=%08x candidate=%08x\n", i, ow[i], cw[i]);
			return 1;
			}
		++caseCount;
		}
	printf("boxslot4 candidate cases=%u failures=0 provisional=1\n", caseCount);
	// Isolate the merge precision regression from all wrapper/pose code.
	// 1c695 stores reciprocal 1/(1+96) to float before center scaling.
	MassFrame incoming, oracleMerge, candidateMerge;
	memset(&incoming, 0, sizeof(incoming));
	memset(&oracleMerge, 0, sizeof(oracleMerge));
	incoming.mMass = 96.0f;
	incoming.mOffset.x = 2.0f; incoming.mOffset.y = -3.0f; incoming.mOffset.z = 4.0f;
	oracleMerge.mMass = 1.0f;
	candidateMerge = oracleMerge;
	typedef void (__thiscall* MergeRegressionFn)(void*, const void*);
	(reinterpret_cast<MergeRegressionFn>(base + 0x1c630))(&oracleMerge, &incoming);
	candidateMerge.nxMassFrameMerge(incoming);
	if(memcmp(&oracleMerge, &candidateMerge, sizeof(oracleMerge)) != 0)
		{
		fprintf(stderr, "FAIL massmerge reciprocal-store regression\n");
		return 1;
		}
	printf("massmerge reciprocal-store regression agree words=13\n");
	for(unsigned n = 1; n <= 16; ++n)
		{
		memset(&incoming, 0, sizeof(incoming));
		memset(&oracleMerge, 0, sizeof(oracleMerge));
		incoming.mMass = 0.137f * n;
		oracleMerge.mMass = 1.019f + n * 0.073f;
		incoming.mOffset.x = 2.031f * n;
		incoming.mOffset.y = -3.071f / n;
		incoming.mOffset.z = 0.119f + n;
		oracleMerge.mOffset.x = -1.023f / n;
		oracleMerge.mOffset.y = 0.017f * n;
		oracleMerge.mOffset.z = -4.073f * n;
		candidateMerge = oracleMerge;
		(reinterpret_cast<MergeRegressionFn>(base + 0x1c630))(&oracleMerge, &incoming);
		candidateMerge.nxMassFrameMerge(incoming);
		if(memcmp(&oracleMerge, &candidateMerge, sizeof(oracleMerge)) != 0)
			{
			fprintf(stderr, "FAIL massmerge fractional case=%u\n", n);
			return 1;
			}
		}
	printf("massmerge fractional centers agree cases=16 words=13\n");
	}

	// -----------------------------------------------------------------------
	// MassFrame translate, phys_fn_000833 (0x1c040): drive oracle vs
	// candidate nxMassFrameTranslate over varied offset+translation inputs.
	// PROVISIONAL; the displaced parallel-axis path is being transcribed.
	{
	typedef void (__thiscall* MfTranslateOracle)(void*, const void*);
	MfTranslateOracle mfTrO = reinterpret_cast<MfTranslateOracle>(base + 0x1c040);
	unsigned trFail = 0, trRun = 0;
	const float dVectors[][3] = {
		{0,0,0}, {1,0,0}, {0,2,0}, {0,0,3}, {1,2,3},
		{-1,0.5f,-2}, {0.25f,-0.5f,4}, {-1.5f,2.75f,-6.25f}, {7,-3,0.1f} };
	const float off[][3] = {
		{0,0,0}, {1,0,0}, {0,2,0}, {-1,1,5}, {0.5f,-0.25f,1.75f}, {-3,4,-2} };
	for(unsigned oi=0; oi<6; ++oi)
	for(unsigned di=0; di<9; ++di)
		{
		MassFrame oF, cF;
		memset(&oF,0xCD,sizeof(oF)); memset(&cF,0xCD,sizeof(cF));
		for(int i=0;i<9;++i) oF.mInertia[i]=cF.mInertia[i]=(float)(i+1);
		oF.mOffset.x=cF.mOffset.x=off[oi][0];
		oF.mOffset.y=cF.mOffset.y=off[oi][1];
		oF.mOffset.z=cF.mOffset.z=off[oi][2];
		oF.mMass=cF.mMass=6.0f;
		mfTrO(&oF, dVectors[di]);
		cF.nxMassFrameTranslate(dVectors[di]);
		++trRun;
		if(memcmp(&oF,&cF,sizeof(oF)) != 0)
			{
			++trFail;
			fprintf(stderr,"mftranslate oi=%u di=%u mismatch\n", oi, di);
			for(int i=0;i<13;++i){
				unsigned a,b; memcpy(&a,((float*)&oF)+i,4); memcpy(&b,((float*)&cF)+i,4);
				if(a!=b) fprintf(stderr,"  word%u o=%08x c=%08x\n",i,a,b);}
			}
		}
	printf("mftranslate candidate run=%u failures=%u provisional=1\n", trRun, trFail);
	}

	// -- phys_fn_000841 (0x1c720): the negated-offset translate wrapper.
	// __thiscall ret 0: builds { -offset } on the stack and calls 000833, so
	// the frame is moved so its center lands at the origin. Drive against the
	// oracle 000841 using the closed 000833 (nxMassFrameTranslate).
	{
	typedef void (__thiscall* NfNegOracle)(void*);
	NfNegOracle negO = reinterpret_cast<NfNegOracle>(base + 0x1c720);
	unsigned ngFail = 0, ngRun = 0;
	const float ngOff[][3] = { {1,0,0}, {0,2,0}, {1,2,3}, {-1,0.5f,-2}, {0.25f,-0.5f,4} };
	for(unsigned oi = 0; oi < 5; ++oi)
		{
		MassFrame oF, cF;
		memset(&oF, 0xCD, sizeof(oF)); memset(&cF, 0xCD, sizeof(cF));
		for(int i=0;i<9;++i) oF.mInertia[i]=cF.mInertia[i]=(float)(i+1);
		oF.mOffset.x=cF.mOffset.x=ngOff[oi][0];
		oF.mOffset.y=cF.mOffset.y=ngOff[oi][1];
		oF.mOffset.z=cF.mOffset.z=ngOff[oi][2];
		oF.mMass=cF.mMass=6.0f;
		negO(&oF);
		float neg[3] = { -cF.mOffset.x, -cF.mOffset.y, -cF.mOffset.z };
		cF.nxMassFrameTranslate(neg);
		++ngRun;
		if(memcmp(&oF,&cF,sizeof(oF)) != 0)
			{
			++ngFail;
			for(int i=0;i<13;++i){ unsigned a,b; memcpy(&a,((float*)&oF)+i,4); memcpy(&b,((float*)&cF)+i,4);
			if(a!=b) fprintf(stderr,"negtrans oi=%u w%u o=%08x c=%08x\n",oi,i,a,b);}
			}
		}
	printf("negtrans candidate run=%u failures=%u provisional=1\n", ngRun, ngFail);
	}

	// -- phys_fn_000835 (0x1c5a0): the two-record mass combo -- fold the
	// {d;K} payload (000831) at param, then translate by the second record's
	// d (000833) at param+0x24. Both helpers are closed; this is the thin
	// combination. `ret 4` __thiscall.
	{
	typedef void (__thiscall* MFComboOracle)(void*, const void*);
	MFComboOracle comboO = reinterpret_cast<MFComboOracle>(base + 0x1c5a0);
	unsigned cbFail = 0, cbRun = 0;
	for(unsigned ci = 0; ci < 6; ++ci)
		{
		float payload[0x30/4];
		memset(payload, 0, sizeof(payload));
		payload[0]=ci+1; payload[1]=-(ci+1)*0.5f; payload[2]=2.0f;
		payload[3]=1.2f; payload[4]=0.1f; payload[5]=0.2f; payload[6]=3.3f;
		payload[7]=0.4f; payload[8]=2.5f;					// first {d;K}
		payload[12]=ci+1; payload[13]=-(ci+1); payload[14]=0.5f;	// second d
		MassFrame oF, cF;
		memset(&oF,0xCD,sizeof(oF)); memset(&cF,0xCD,sizeof(cF));
		for(int i=0;i<9;++i) oF.mInertia[i]=cF.mInertia[i]=(float)(i+1);
		oF.mMass=cF.mMass=6.0f;
		comboO(&oF, payload);
		cF.nxMassFrameFoldPayload(payload);
		cF.nxMassFrameTranslate(payload + 9);			// param+0x24 = payload+9
		++cbRun;
		if(memcmp(&oF,&cF,sizeof(oF)) != 0)
			{
			++cbFail;
			for(int i=0;i<13;++i){ unsigned a,b; memcpy(&a,((float*)&oF)+i,4); memcpy(&b,((float*)&cF)+i,4);
			if(a!=b) fprintf(stderr,"mfcombo ci=%u w%u o=%08x c=%08x\n",ci,i,a,b);}
			}
		}
	printf("mfcombo candidate run=%u failures=%u provisional=1\n", cbRun, cbFail);
	}

	// -- phys_fn_000010 (0x1390): __thiscall ret 4 pure copy of 0x78 bytes
	// from [esp+4] into this (rep movsd 9 + individual dwords through +0x74).
	{
	typedef void (__thiscall* PoseCopyOracle)(void*, const void*);
	PoseCopyOracle poseCopyO = reinterpret_cast<PoseCopyOracle>(base + 0x1390);
	unsigned pcFail = 0, pcRun = 0;
	for(unsigned ci = 0; ci < 4; ++ci)
		{
		unsigned char src[0x80], dstO[0x80], dstC[0x80];
		for(unsigned w = 0; w < 0x80/4; ++w)
			{ unsigned v = 0xA0000000u + ci*0x1000u + w*4u; memcpy(src + w*4, &v, 4); }
		memset(dstO, 0xCD, sizeof(dstO)); memset(dstC, 0xCD, sizeof(dstC));
		poseCopyO(dstO, src);
		memcpy(dstC, src, 0x78);
		++pcRun;
		if(memcmp(dstO, dstC, 0x78) != 0
			|| memcmp(dstO+0x78, dstC+0x78, 8) != 0)	// canaries untouched past 0x78
			{
			++pcFail;
			fprintf(stderr, "posecopy ci=%u mismatch (src %08x..)\n", ci, *reinterpret_cast<unsigned*>(src));
			}
		}
	printf("posecopy candidate run=%u failures=%u provisional=1\n", pcRun, pcFail);
	}

	// -- Small shape getters/copies (phys_fn_000929 / 001291 / 001283):
	// drive each oracle row against a direct candidate to confirm the exact
	// bytes moved / pointer returned.
	{
	typedef void* (__thiscall* DimsGetterOracle)(void*);		// 000929 -> this+0xe4
	DimsGetterOracle dimsG = reinterpret_cast<DimsGetterOracle>(base + 0x20480);
	typedef void* (__thiscall* D0GetterOracle)(void*);			// 001283 -> this+0xd0
	D0GetterOracle d0G = reinterpret_cast<D0GetterOracle>(base + 0x257b0);
	typedef void (__thiscall* Pose3CopyOracle)(void*, void*);	// 001291: copy 36B from this+0x6c
	Pose3CopyOracle p3c = reinterpret_cast<Pose3CopyOracle>(base + 0x25810);
	unsigned dg2 = 0;
	unsigned char shapeG[0x130];
	memset(shapeG, 0xAB, sizeof(shapeG));
	unsigned f; unsigned dg = 0xb0b0b0b0u; memcpy(shapeG+0xd0,&dg,4);
	if(dimsG(shapeG) != shapeG + 0xe4) { fprintf(stderr,"dimsgetter fail\n"); ++dg2; }
	// 001283 returns *this+0xd0 (the DWORD value), not the address.
	if(d0G(shapeG) != reinterpret_cast<void*>(0xb0b0b0b0u)) { fprintf(stderr,"d0getter fail\n"); ++dg2; }
	unsigned char out3[36]; memset(out3, 0xCD, sizeof(out3));
	for(unsigned w=0;w<9;++w){ unsigned v=0x12345678u+w; memcpy(shapeG+0x6c+w*4,&v,4); }
	p3c(shapeG, out3);
	if(memcmp(out3, shapeG+0x6c, 36) != 0) { fprintf(stderr,"pose3copy fail\n"); ++dg2; }
	printf("shapegetters candidate failures=%u provisional=1\n", dg2);
	}

	// -- Second batch of pure shape-row getters/init:
	//   000925 (0x20440) zero [this+0..8]; 001285 (0x257c0) -> word+d8;
	//   001293 (0x25830) -> word+da; 000999 (0x21c20) -> 2*(float)+e4.
	{
	typedef void (__thiscall* Zero3Oracle)(void*);
	Zero3Oracle z3 = reinterpret_cast<Zero3Oracle>(base + 0x20440);
	typedef unsigned short (__thiscall* WordGetOracle)(void*);
	WordGetOracle wd8 = reinterpret_cast<WordGetOracle>(base + 0x257c0);
	WordGetOracle wda = reinterpret_cast<WordGetOracle>(base + 0x25830);
	typedef float (__thiscall* D2Oracle)(void*);
	D2Oracle d2 = reinterpret_cast<D2Oracle>(base + 0x21c20);
	unsigned g2f = 0;
	unsigned char shape2[0x100];
	memset(shape2, 0x76, sizeof(shape2));
	z3(shape2);
	if(memcmp(shape2, "\0\0\0\0\0\0\0\0\0\0\0\0", 12) != 0) { fprintf(stderr,"zero3 fail\n"); ++g2f; }
	const unsigned short d8v = 0x1234, dav = 0x5678;
	memcpy(shape2+0xd8, &d8v, 2); memcpy(shape2+0xda, &dav, 2);
	if(wd8(shape2) != d8v) { fprintf(stderr,"wordd8 fail\n"); ++g2f; }
	if(wda(shape2) != dav) { fprintf(stderr,"wordda fail\n"); ++g2f; }
	const unsigned e4v = 0x3f000000u; memcpy(shape2+0xe4, &e4v, 4); // 0.5
	float d2r = d2(shape2);
	unsigned dv; memcpy(&dv, &d2r, 4);
	if(dv != 0x3f800000u) { fprintf(stderr,"dims2 fail got %08x\n", dv); ++g2f; } // 2*0.5=1.0
	printf("shapegetters2 candidate failures=%u provisional=1\n", g2f);
	}

	// -- Pointer-based getters/setters across later-phase rows:
	//   002211 (0x54800) -> *[this+0x9c]+0x18; 002213 -> *[this+0x9c]+0xc;
	//   002215 -> *[this+0x9c]+0x10; 002387 (0x5b8e0) -> byte *[this+4]+8;
	//   002383 (0x5b8b0) sets byte *[this+4]+8 = 1; 002398 (0x5b9d0) ret 4
	//   stores [esp+4] into this+0x10. Drive vs oracle.
	{
	typedef void* (__thiscall* PtrGetOracle)(void*);
	PtrGetOracle pg0 = reinterpret_cast<PtrGetOracle>(base + 0x54800);
	PtrGetOracle pg1 = reinterpret_cast<PtrGetOracle>(base + 0x54810);
	PtrGetOracle pg2 = reinterpret_cast<PtrGetOracle>(base + 0x54820);
	typedef void (__thiscall* ByteSetOracle)(void*);
	ByteSetOracle bs = reinterpret_cast<ByteSetOracle>(base + 0x5b8b0);
	typedef unsigned char (__thiscall* ByteGetOracle)(void*);
	ByteGetOracle bg = reinterpret_cast<ByteGetOracle>(base + 0x5b8e0);
	typedef void (__thiscall* StoreOracle)(void*, void*);
	StoreOracle st = reinterpret_cast<StoreOracle>(base + 0x5b9d0);
	unsigned pf = 0;
	unsigned char inner[0x20], obj[0x130];
	*(reinterpret_cast<void**>(inner + 0xc)) = reinterpret_cast<void*>(0x11111111u);
	*(reinterpret_cast<void**>(inner + 0x10)) = reinterpret_cast<void*>(0x22222222u);
	*(reinterpret_cast<void**>(obj + 0x9c)) = inner;
	if(pg0(obj) != inner + 0x18) { fprintf(stderr,"ptrget0 fail\n"); ++pf; }
	if(pg1(obj) != reinterpret_cast<void*>(0x11111111u)) { fprintf(stderr,"ptrget1 fail\n"); ++pf; }
	if(pg2(obj) != reinterpret_cast<void*>(0x22222222u)) { fprintf(stderr,"ptrget2 fail\n"); ++pf; }
	unsigned char* owned = obj + 0x14; *(reinterpret_cast<void**>(obj+0x4)) = owned;
	owned[8] = 0xEE;
	bg(obj);	// returns byte; verify value matches
	if(bg(obj) != 0xEEu) { fprintf(stderr,"byteget fail %02x\n", bg(obj)); ++pf; }
	memset(owned, 0, 16); owned[8] = 0x00;
	bs(obj);
	if(owned[8] != 1) { fprintf(stderr,"byteset fail\n"); ++pf; }
	st(obj, reinterpret_cast<void*>(0x33333333u));
	if(*(reinterpret_cast<unsigned*>(obj + 0x10)) != 0x33333333u) { fprintf(stderr,"store fail\n"); ++pf; }
	printf("ptrgetters candidate failures=%u provisional=1\n", pf);
	}

	// -- Simple non-pointer getters (constant-address + field-get)
	//   005149/005151/005153 @0xe3190/... -> fixed rdata addresses;
	//   004070 (0x95a80) -> [this+0x168]; 004078 (0x95bb0) -> ((this+0x2c)>>3)&3.
	{
	typedef void* (__thiscall* ConstGetOracle)(void*);
	ConstGetOracle cg0 = reinterpret_cast<ConstGetOracle>(base + 0xe3190);
	ConstGetOracle cg1 = reinterpret_cast<ConstGetOracle>(base + 0xe31a0);
	ConstGetOracle cg2 = reinterpret_cast<ConstGetOracle>(base + 0xe31b0);
	typedef unsigned (__thiscall* FieldGetOracle)(void*);
	FieldGetOracle fg = reinterpret_cast<FieldGetOracle>(base + 0x95a80);
	FieldGetOracle bitGet = reinterpret_cast<FieldGetOracle>(base + 0x95bb0);
	unsigned gf = 0;
	void* dummy = 0;
	if(cg0(dummy) != reinterpret_cast<void*>(0x10122370u)) { fprintf(stderr,"const0 fail\n"); ++gf; }
	if(cg1(dummy) != reinterpret_cast<void*>(0x101223d0u)) { fprintf(stderr,"const1 fail\n"); ++gf; }
	if(cg2(dummy) != reinterpret_cast<void*>(0x10122430u)) { fprintf(stderr,"const2 fail\n"); ++gf; }
	unsigned char cobj[0x200]; memset(cobj, 0, sizeof(cobj));
	const unsigned fv = 0x5A5A5A5Au; memcpy(cobj + 0x168, &fv, 4);
	if(fg(cobj) != fv) { fprintf(stderr,"fieldget fail %08x\n", (unsigned)fg(cobj)); ++gf; }
	const unsigned bv = 0xA3u; memcpy(cobj + 0x2c, &bv, 4); // 0xA3>>3 & 3 = 0x14&3? 0xA3>>3=0x14, &3=0
	if(bitGet(cobj) != 0u) { fprintf(stderr,"bitget fail %u\n", (unsigned)bitGet(cobj)); ++gf; }
	printf("simplegetters candidate failures=%u provisional=1\n", gf);
	}

	// -- Batch of field-return / pointer / constant getters:
	//   000287(+0x24) 000523(+0x3c) 000547/551/555(+0x6ac/6b0/6b4)
	//   002198(ret 1) 003952(+0x14) 004290(+0x1d0) 005604 005622
	//   002334(lea+0x28) 003661(lea+8). Drive each vs oracle.
	{
	typedef unsigned (__thiscall* UGetOracle)(void*);
	UGetOracle g24 = reinterpret_cast<UGetOracle>(base + 0xc3f0);
	UGetOracle g3c = reinterpret_cast<UGetOracle>(base + 0x10400);
	UGetOracle g6ac = reinterpret_cast<UGetOracle>(base + 0x107f0);
	UGetOracle g6b0 = reinterpret_cast<UGetOracle>(base + 0x10810);
	UGetOracle g6b4 = reinterpret_cast<UGetOracle>(base + 0x10830);
	UGetOracle g14 = reinterpret_cast<UGetOracle>(base + 0x8f0f0);
	UGetOracle g1d0 = reinterpret_cast<UGetOracle>(base + 0xa2f30);
	UGetOracle g5604 = reinterpret_cast<UGetOracle>(base + 0xf2eb0);
	UGetOracle g5622 = reinterpret_cast<UGetOracle>(base + 0xf3620);
	typedef void* (__thiscall* PGetOracle)(void*);
	PGetOracle p28 = reinterpret_cast<PGetOracle>(base + 0x5a870);
	PGetOracle p8 = reinterpret_cast<PGetOracle>(base + 0x8ac40);
	unsigned bf = 0;
	unsigned char b2[0x800]; memset(b2, 0, sizeof(b2));
	#define SETU(off,val) { unsigned _v=(val); memcpy(b2+(off),&_v,4); }
	SETU(0x24,0x11111111); SETU(0x3c,0x22222222);
	SETU(0x6ac,0x33333333); SETU(0x6b0,0x44444444); SETU(0x6b4,0x55555555);
	SETU(0x14,0x66666666); SETU(0x1d0,0x77777777); SETU(0x4,8);
	if(g24(b2)!=0x11111111u){fprintf(stderr,"g24 fail\n");++bf;}
	if(g3c(b2)!=0x22222222u){fprintf(stderr,"g3c fail\n");++bf;}
	if(g6ac(b2)!=0x33333333u){fprintf(stderr,"g6ac fail\n");++bf;}
	if(g6b0(b2)!=0x44444444u){fprintf(stderr,"g6b0 fail\n");++bf;}
	if(g6b4(b2)!=0x55555555u){fprintf(stderr,"g6b4 fail\n");++bf;}
	if(g14(b2)!=0x66666666u){fprintf(stderr,"g14 fail\n");++bf;}
	if(g1d0(b2)!=0x77777777u){fprintf(stderr,"g1d0 fail\n");++bf;}
	if(g5604(b2)!=(8u<<5)+8u){fprintf(stderr,"g5604 fail %08x\n",g5604(b2));++bf;} // (8<<5)+8
	if(g5622(b2)!=(8u+2u)<<4u){fprintf(stderr,"g5622 fail %08x\n",g5622(b2));++bf;} // (8+2)<<4
	if(p28(b2)!=b2+0x28){fprintf(stderr,"p28 fail\n");++bf;}
	if(p8(b2)!=b2+8){fprintf(stderr,"p8 fail\n");++bf;}
	#undef SETU
	printf("batchgetters candidate failures=%u provisional=1\n", bf);
	}

	// -- More tiny rows: constant-return / zero-store / bit-and.
	//   002196(mov al,1;ret 8) 002198(mov eax,1;ret) 005533/535(xor al,al)
	//   004214(zero this+0x1cc) 004186(ret [this+0x44]) 004897(and ~0xc)
	//   003455(ret [this+0x58]&arg) 003595(ret [this+0x10]&arg).
	{
	typedef unsigned (__thiscall* TinyOracle)(void*, unsigned);
	TinyOracle ret1 = reinterpret_cast<TinyOracle>(base + 0x54630);
	TinyOracle retA1 = reinterpret_cast<TinyOracle>(base + 0x54620);
	TinyOracle ret0_5 = reinterpret_cast<TinyOracle>(base + 0xf1580);
	TinyOracle ret0_4 = reinterpret_cast<TinyOracle>(base + 0xf1590);
	TinyOracle zero1cc = reinterpret_cast<TinyOracle>(base + 0x9e3c0);
	TinyOracle g44 = reinterpret_cast<TinyOracle>(base + 0x9b5d0);
	typedef unsigned (__thiscall* AndOracle)(void*, unsigned);
	AndOracle andFlags = reinterpret_cast<AndOracle>(base + 0xb5710);
	AndOracle g58a = reinterpret_cast<AndOracle>(base + 0x84f00);
	AndOracle g10a = reinterpret_cast<AndOracle>(base + 0x88130);
	unsigned tf = 0;
	unsigned char t2[0x300]; memset(t2, 0xA5, sizeof(t2));
	if(ret1(t2,0) != 1u){fprintf(stderr,"ret1 fail\n");++tf;}
	// retA1/ret0_5/ret0_4 return byte in al (0 or 1); compare low byte.
	if((retA1(t2,0)&0xFFu)!=1u){fprintf(stderr,"retA1 fail\n");++tf;}
	if((ret0_5(t2,0)&0xFFu)!=0u){fprintf(stderr,"ret0_5 fail\n");++tf;}
	if((ret0_4(t2,0)&0xFFu)!=0u){fprintf(stderr,"ret0_4 fail\n");++tf;}
	t2[0x1cc]=0x7F; zero1cc(t2,0);
	if(t2[0x1cc]!=0){fprintf(stderr,"zero1cc fail\n");++tf;}
	unsigned v44=0x5A5A5A5Au; memcpy(t2+0x44,&v44,4);
	if(g44(t2,0)!=v44){fprintf(stderr,"g44 fail\n");++tf;}
	t2[4]=0xFF; andFlags(t2,0);
	if(t2[4]!=((unsigned char)(0xFFu & 0xFFFFFFF3u))){fprintf(stderr,"andflags fail %02x\n",t2[4]);++tf;}
	unsigned v58=0x3; memcpy(t2+0x58,&v58,4);
	if(g58a(t2, 0x1u)!= (3u&1u)){fprintf(stderr,"g58a fail\n");++tf;}
	unsigned v10=0xBC; memcpy(t2+0x10,&v10,4);
	if(g10a(t2, 0x10u)!= (0xBCu&0x10u)){fprintf(stderr,"g10a fail\n");++tf;}
	printf("tinygetters candidate failures=%u provisional=1\n", tf);
	}

	// -- Setter/getter batch: 000538(store +0x544) 000545(store +0x6ac)
	//   000559(+0x6c8) 000561(+0x6c4) 002868(ptr-ptr) 004988(zero+and)
	//   005212(inc+0x38) 005584(imul) 005640(lea) 004336(+0x1a8).
	{
	typedef void (__thiscall* Store1Oracle)(void*, unsigned);
	Store1Oracle s544 = reinterpret_cast<Store1Oracle>(base + 0x106e0);
	Store1Oracle s6ac = reinterpret_cast<Store1Oracle>(base + 0x107e0);
	typedef unsigned (__thiscall* UGetOracle2)(void*);
	UGetOracle2 g6c8 = reinterpret_cast<UGetOracle2>(base + 0x10860);
	UGetOracle2 g6c4 = reinterpret_cast<UGetOracle2>(base + 0x10870);
	UGetOracle2 g1a8 = reinterpret_cast<UGetOracle2>(base + 0xa8fb0);
	typedef unsigned (__cdecl* DiffOracle)(const unsigned*, const unsigned*);
	DiffOracle diff = reinterpret_cast<DiffOracle>(base + 0x6da40);
	typedef void (__thiscall* ZeroAndOracle)(void*);
	ZeroAndOracle za = reinterpret_cast<ZeroAndOracle>(base + 0xd14a0);
	typedef unsigned (__thiscall* IncOracle)(void*, unsigned);
	IncOracle inc38 = reinterpret_cast<IncOracle>(base + 0xe50f0);
	UGetOracle2 g584 = reinterpret_cast<UGetOracle2>(base + 0xf2570);
	UGetOracle2 g640 = reinterpret_cast<UGetOracle2>(base + 0xf3d30);
	unsigned s2f = 0;
	unsigned char s2[0x800]; memset(s2, 0, sizeof(s2));
	s544(s2, 0xA1A1A1A1u); if(*(unsigned*)(s2+0x544)!=0xA1A1A1A1u){fprintf(stderr,"s544 fail\n");++s2f;}
	s6ac(s2, 0xA2A2A2A2u); if(*(unsigned*)(s2+0x6ac)!=0xA2A2A2A2u){fprintf(stderr,"s6ac fail\n");++s2f;}
	const unsigned w = 0x12345678u;
	memcpy(s2+0x6c8,&w,4); memcpy(s2+0x6c4,&w,4); memcpy(s2+0x1a8,&w,4);
	if(g6c8(s2)!=w){fprintf(stderr,"g6c8 fail\n");++s2f;}
	if(g6c4(s2)!=w){fprintf(stderr,"g6c4 fail\n");++s2f;}
	if(g1a8(s2)!=w){fprintf(stderr,"g1a8 fail\n");++s2f;}
	unsigned pa=0x30u,pb=0x10u;
	if(diff(&pa,&pb)!=0x20u){fprintf(stderr,"diff fail %u\n",diff(&pa,&pb));++s2f;}
	memset(s2+0x2c,0xA5,8); s2[4]=0xFF;
	za(s2);
	if(*(unsigned*)(s2+0x2c)!=0 || *(unsigned*)(s2+0x30)!=0 || s2[4]!=((unsigned char)(0xFFu&0xFFFFFFF3u))){fprintf(stderr,"za fail\n");++s2f;}
	*(unsigned*)(s2+0x38)=5; inc38(s2,0);
	if(*(unsigned*)(s2+0x38)!=6){fprintf(stderr,"inc38 fail\n");++s2f;}
	const unsigned v4=9; memcpy(s2+4,&v4,4);
	if(g584(s2)!=(9u*0x1cu+8u)){fprintf(stderr,"g584 fail\n");++s2f;}
	if(g640(s2)!=(9u*5u*4u+0x20u)){fprintf(stderr,"g640 fail %08x\n",g640(s2));++s2f;}
	printf("setget2 candidate failures=%u provisional=1\n", s2f);
	}

	// -- Setter/copy/vptr/noop batch: 000549/000553(stores) 000563/000565
	//   (copy) 005329/001554(vptr store) 004248/004411/005242(bare retN)
	//   005202(const 0x101224c0).
	{
	typedef void (__thiscall* Store4Oracle)(void*, unsigned);
	Store4Oracle s6b0 = reinterpret_cast<Store4Oracle>(base + 0x10800);
	Store4Oracle s6b4 = reinterpret_cast<Store4Oracle>(base + 0x10820);
	typedef void (__thiscall* CopyOracle)(void*);
	CopyOracle c59c = reinterpret_cast<CopyOracle>(base + 0x10880);
	CopyOracle c5a4 = reinterpret_cast<CopyOracle>(base + 0x10890);
	CopyOracle vptr1 = reinterpret_cast<CopyOracle>(base + 0xe82b0);
	CopyOracle vptr2 = reinterpret_cast<CopyOracle>(base + 0x2e210);
	typedef void* (__thiscall* ConstGetOracle2)(void*);
	ConstGetOracle2 cg = reinterpret_cast<ConstGetOracle2>(base + 0xe4ca0);
	typedef void (__thiscall* BareRetOracle)(void*);
	BareRetOracle r4 = reinterpret_cast<BareRetOracle>(base + 0xa0f60);
	BareRetOracle rc = reinterpret_cast<BareRetOracle>(base + 0xb03a0);
	BareRetOracle r8 = reinterpret_cast<BareRetOracle>(base + 0xe5890);
	unsigned a2f = 0;
	unsigned char a2[0x800]; memset(a2, 0, sizeof(a2));
	s6b0(a2, 0x11111111u); if(*(unsigned*)(a2+0x6b0)!=0x11111111u){fprintf(stderr,"s6b0 fail\n");++a2f;}
	s6b4(a2, 0x22222222u); if(*(unsigned*)(a2+0x6b4)!=0x22222222u){fprintf(stderr,"s6b4 fail\n");++a2f;}
	*(unsigned*)(a2+0x59c)=0x12121212u; *(unsigned*)(a2+0x6bc)=0;
	c59c(a2); if(*(unsigned*)(a2+0x6bc)!=0x12121212u){fprintf(stderr,"c59c fail\n");++a2f;}
	*(unsigned*)(a2+0x5a4)=0x34343434u; *(unsigned*)(a2+0x6c0)=0;
	c5a4(a2); if(*(unsigned*)(a2+0x6c0)!=0x34343434u){fprintf(stderr,"c5a4 fail\n");++a2f;}
	vptr1(a2); if(*(unsigned*)(a2)!=0x1011ba40u){fprintf(stderr,"vptr1 fail\n");++a2f;}
	vptr2(a2); if(*(unsigned*)(a2)!=0x10107848u){fprintf(stderr,"vptr2 fail\n");++a2f;}
	if(cg(a2)!=reinterpret_cast<void*>(0x101224c0u)){fprintf(stderr,"cg fail\n");++a2f;}
	r4(a2); rc(a2); r8(a2);	// bare retN are no-ops; just confirm no fault
	printf("setget3 candidate failures=%u provisional=1\n", a2f);
	}

	// -- Zero/init/global-write batch: 001536(zero 0/4) 001012(zero arg,ret8)
	//   001439(zero 0/2/4) 005328(vptr+0xffffffff) 004081(global write)
	//   004805(global write, returns 1).
	{
	typedef void (__thiscall* ZeroInitOracle)(void*);
	ZeroInitOracle z04 = reinterpret_cast<ZeroInitOracle>(base + 0x2dae0);
	ZeroInitOracle z024 = reinterpret_cast<ZeroInitOracle>(base + 0x2a610);
	ZeroInitOracle vf = reinterpret_cast<ZeroInitOracle>(base + 0xe82a0);
	typedef void* (__thiscall* ArgInitOracle)(void*, void*);
	ArgInitOracle az = reinterpret_cast<ArgInitOracle>(base + 0x225d0);
	unsigned z2f = 0;
	unsigned char z2[0x400]; memset(z2, 0xA5, sizeof(z2));
	z04(z2); if(*(unsigned*)(z2+0)!=0 || *(unsigned*)(z2+4)!=0){fprintf(stderr,"z04 fail\n");++z2f;}
	unsigned char zarg[8]; memset(zarg,0xA5,8);
	az(z2, zarg); if(*(unsigned*)(zarg)!=0){fprintf(stderr,"az fail\n");++z2f;}
	z024(z2);
	if(*(unsigned short*)(z2+0)!=0 || *(unsigned short*)(z2+2)!=0 || *(unsigned*)(z2+4)!=0){fprintf(stderr,"z024 fail\n");++z2f;}
	vf(z2); if(*(unsigned*)(z2+0)!=0x1011ba40u || *(unsigned*)(z2+0x38)!=0xFFFFFFFFu){fprintf(stderr,"vf fail\n");++z2f;}
	// 004081/004805 write to static .data globals; proven by listing (static
	// proof) rather than an in-process global read, which is relocation-fragile.
	printf("zeroinit candidate failures=%u provisional=1\n", z2f);
	}

	// -- Global-write rows 004081 (0x95c90) and 004805 (0xb4020): each stores
	// to a fixed .data global. Prove by first scribbling a sentinel at the
	// relocated target, calling the oracle, then reading it back.
	{
	typedef void (__thiscall* GwOracle1)(void*);
	GwOracle1 mo817180 = reinterpret_cast<GwOracle1>(base + 0x95c90);
	typedef void (__thiscall* GwOracle2)(void*, void*);
	GwOracle2 mo842845c = reinterpret_cast<GwOracle2>(base + 0xb4020);
	unsigned g2f = 0;
	unsigned char gsrc[0x40]; memset(gsrc, 0, sizeof(gsrc));
	// RVA of globals: va - 0x10000000 = 0x127180 / 0x12845c.
	unsigned* glb1 = reinterpret_cast<unsigned*>(const_cast<unsigned char*>(base + 0x127180));
	unsigned* glb2 = reinterpret_cast<unsigned*>(const_cast<unsigned char*>(base + 0x12845c));
	*glb1 = 0; *glb2 = 0;
	unsigned v1 = 0x13579BDFu; memcpy(gsrc + 0x20, &v1, 4);
	mo817180(gsrc);
	if(*glb1 != v1){fprintf(stderr,"mo817180 fail glb=%08x\n",*glb1);++g2f;}
	unsigned v2 = 0x2468ACE0u;
	mo842845c(gsrc, &v2);
	unsigned g2addr = static_cast<unsigned>(reinterpret_cast<size_t>(&v2));	// 004805 stores the ARG address
	if(*glb2 != g2addr){fprintf(stderr,"mo842845c fail glb=%08x exp=%08x\n",*glb2,g2addr);++g2f;}
	printf("globalwrite candidate failures=%u provisional=1\n", g2f);
	}

	// -- Small ctor/vptr/init batch: 001373(store+return1,ret8)
	//   001421(zero 24-30) 001552(vptr+zero4/8) 002140(vptr+arg,ret4)
	//   005355(zero 0-c).
	{
	typedef bool (__thiscall* StoreE0Oracle)(void*, unsigned*);
	StoreE0Oracle sE0 = reinterpret_cast<StoreE0Oracle>(base + 0x27c10);
	typedef void (__thiscall* ZeroInitOracle4)(void*);
	ZeroInitOracle4 zs24 = reinterpret_cast<ZeroInitOracle4>(base + 0x29a10);
	ZeroInitOracle4 v2a = reinterpret_cast<ZeroInitOracle4>(base + 0x2e1f0);
	ZeroInitOracle4 zs0 = reinterpret_cast<ZeroInitOracle4>(base + 0xe8fa0);
	typedef void (__thiscall* CtorArgOracle)(void*, unsigned);
	CtorArgOracle ctr = reinterpret_cast<CtorArgOracle>(base + 0x53290);
	unsigned s5f = 0;
	unsigned char s5[0x400]; memset(s5, 0, sizeof(s5));
	const unsigned e0v = 0xABCDEF01u; memcpy(s5 + 0xe0, &e0v, 4);
	unsigned outE0 = 0;
	if(!sE0(s5, &outE0)){fprintf(stderr,"sE0 ret fail\n");++s5f;}
	if(outE0 != e0v){fprintf(stderr,"sE0 val fail\n");++s5f;}
	memset(s5+0x24,0x99,0x10);
	zs24(s5); if(memcmp(s5+0x24,"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",16)!=0){fprintf(stderr,"zs24 fail\n");++s5f;}
	memset(s5+4,0x77,8);
	v2a(s5); if(*(unsigned*)(s5+0)!=0x10107848u || *(unsigned*)(s5+4)!=0 || *(unsigned*)(s5+8)!=0){fprintf(stderr,"v2a fail\n");++s5f;}
	ctr(s5, 0x12345678u); if(*(unsigned*)(s5+0)!=0x1010829cu || *(unsigned*)(s5+4)!=0x12345678u){fprintf(stderr,"ctr fail\n");++s5f;}
	memset(s5+0,0x55,0x10);
	zs0(s5); if(memcmp(s5,"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0",16)!=0){fprintf(stderr,"zs0 fail\n");++s5f;}
	printf("smallctor candidate failures=%u provisional=1\n", s5f);
	}

	// -- Zero/store/copy/arith batch: 001663(zero 0-10) 003265(store 2 args)
	//   004076(copy 2 fields) 004282(zero 1f0-1f8) 000571(link-insert).
	{
	typedef void (__thiscall* ZeroMultiOracle)(void*);
	ZeroMultiOracle zm0 = reinterpret_cast<ZeroMultiOracle>(base + 0x32590);
	ZeroMultiOracle zm1 = reinterpret_cast<ZeroMultiOracle>(base + 0x51060);
	ZeroMultiOracle zm2 = reinterpret_cast<ZeroMultiOracle>(base + 0xa2bf0);
	typedef void (__thiscall* StorePairOracle)(void*, unsigned, unsigned);
	StorePairOracle sp = reinterpret_cast<StorePairOracle>(base + 0x7e520);
	typedef void (__thiscall* CopyPairOracle)(void*, unsigned*, unsigned*);
	CopyPairOracle cp = reinterpret_cast<CopyPairOracle>(base + 0x95b90);
	typedef void (__thiscall* LinkInsertOracle)(void*, unsigned*);
	LinkInsertOracle li = reinterpret_cast<LinkInsertOracle>(base + 0x108e0);
	unsigned bf = 0;
	unsigned char b[0x5000]; memset(b, 0, sizeof(b));
	zm0(b);
	if(*(unsigned*)(b+0)+*(unsigned*)(b+4)+*(unsigned*)(b+8)+*(unsigned*)(b+0xc)+*(unsigned*)(b+0x10)!=0){fprintf(stderr,"zm0 fail\n");++bf;}
	zm1(b);
	if(*(unsigned*)(b+0)+*(unsigned*)(b+4)+*(unsigned*)(b+0xc)+*(unsigned*)(b+0x10)+*(unsigned*)(b+0x14)!=0){fprintf(stderr,"zm1 fail\n");++bf;}
	zm2(b);
	if(*(unsigned*)(b+0x1f0)+*(unsigned*)(b+0x1f4)+*(unsigned*)(b+0x1f8)!=0){fprintf(stderr,"zm2 fail\n");++bf;}
	sp(b, 0xA0A0A0A0u, 0xB0B0B0B0u);
	if(*(unsigned*)(b+0x404c)!=0xA0A0A0A0u || *(unsigned*)(b+0x4050)!=0xB0B0B0B0u){fprintf(stderr,"sp fail\n");++bf;}
	unsigned v3c=0x31313131u, v40=0x42424242u; memcpy(b+0x3c,&v3c,4); memcpy(b+0x40,&v40,4);
	unsigned o1=0,o2=0; cp(b,&o1,&o2);
	if(o1!=v3c||o2!=v40){fprintf(stderr,"cp fail %08x/%08x\n",o1,o2);++bf;}
	// 000571: [arg+4]=old head; [this+0x620]=arg (ret 4) -- the row writes past
	// the argument, so it must be a buffer, not a bare local.
	unsigned char node[0x10]; memset(node, 0, sizeof(node));
	{
	unsigned nv = 0x12345678u; memcpy(node, &nv, 4);
	}
	unsigned oldHead = 0xCAFEBABEu; memcpy(b+0x620, &oldHead, 4);
	li(b,(unsigned*)node);
	if(*(unsigned*)(b+0x620) != static_cast<unsigned>(reinterpret_cast<size_t>(node))){fprintf(stderr,"li head fail %08x\n",*(unsigned*)(b+0x620));++bf;}
	if(*(unsigned*)(node + 4) != oldHead){fprintf(stderr,"li next fail %08x\n",*(unsigned*)(node+4));++bf;}
	printf("zmix candidate failures=%u provisional=1\n", bf);
	}

	// -- Misc simple rows: 002152(sbb flag, ret 0xc) 004328(zero +1ac/1b0/1b4)
	//   000557(link-insert, ret 4) 003565(copy3 fields, ret 4).
	{
	typedef int (__thiscall* SbbFn)(void*, unsigned, unsigned);
	SbbFn sbb3 = reinterpret_cast<SbbFn>(base + 0x538a0);
	typedef void (__thiscall* ZeroOracle3)(void*);
	ZeroOracle3 z3 = reinterpret_cast<ZeroOracle3>(base + 0xa8d20);
	typedef void (__thiscall* Link2Oracle)(void*, unsigned*);
	Link2Oracle l2 = reinterpret_cast<Link2Oracle>(base + 0x10840);
	typedef void (__thiscall* Copy3Oracle)(void*, float*);
	Copy3Oracle c3 = reinterpret_cast<Copy3Oracle>(base + 0x87e50);
	unsigned mf = 0;
	// 002152: reads [esp+8] (2nd stack arg); returns 1 if [this+4] < arg else 0
	unsigned char fs[0x40]; memset(fs,0,sizeof(fs));
	const unsigned fv=10; memcpy(fs+4,&fv,4);
	if(sbb3(fs,0,20)!=1){fprintf(stderr,"sbb fail\n");++mf;} // arg(E+8)=20; 10<20 -> 1
	if(sbb3(fs,0,5)!=0){fprintf(stderr,"sbb2 fail\n");++mf;}  // arg(E+8)=5; 10<5? no -> 0
	// 004328 zero
	unsigned char zb[0x500]; memset(zb,0x55,sizeof(zb)); z3(zb);
	if(*(unsigned*)(zb+0x1ac)+*(unsigned*)(zb+0x1b0)+*(unsigned*)(zb+0x1b4)!=0){fprintf(stderr,"z3 fail\n");++mf;}
	// 000557: [arg+0x10]=old head; [this+0x5a0]=arg  (ret 4)
	unsigned char lb[0x800]; memset(lb,0,sizeof(lb));
	unsigned lhead=0xCAFEu; memcpy(lb+0x5a0,&lhead,4);
	// The row writes [arg+0x10], so the argument must be a buffer of at
	// least 0x14 bytes -- passing a bare unsigned overran wmain's frame
	// and could clobber the recorded oracle digest locals.
	unsigned char node2[0x20]; memset(node2,0,sizeof(node2));
	{
	unsigned nv=0x1234u; memcpy(node2,&nv,4);
	}
	l2(lb,(unsigned*)node2);
	if(*(unsigned*)(lb+0x5a0) != static_cast<unsigned>(reinterpret_cast<size_t>(node2))){fprintf(stderr,"l2 head fail\n");++mf;}
	if(*(unsigned*)(node2+0x10) != lhead){fprintf(stderr,"l2 next fail %08x\n",*(unsigned*)(node2+0x10));++mf;}
	// 003565: copy [this+0x3c/40/44] to arg[0/4/8]  (ret 4)
	unsigned char cb[0x100]; memset(cb,0,sizeof(cb));
	const unsigned c3a=0x11111111u,c3b=0x22222222u,c3c=0x33333333u;
	memcpy(cb+0x3c,&c3a,4); memcpy(cb+0x40,&c3b,4); memcpy(cb+0x44,&c3c,4);
	float outc[3]={0,0,0};
	c3(cb, outc);
	unsigned oa,ob,oc; memcpy(&oa,outc+0,4); memcpy(&ob,outc+1,4); memcpy(&oc,outc+2,4);
	if(oa!=c3a||ob!=c3b||oc!=c3c){fprintf(stderr,"c3 fail %08x/%08x/%08x\n",oa,ob,oc);++mf;}
	printf("zmix2 candidate failures=%u provisional=1\n", mf);
	}

	// -- Init/ptr-diff/pop rows: 005157(bbox-init) 005301(zero) 005475(zero)
	//   002896(ptr-field diff) 004776(pop float from container).
	{
	typedef void (__thiscall* BBoxInitOracle)(void*);
	BBoxInitOracle bb = reinterpret_cast<BBoxInitOracle>(base + 0xe32c0);
	BBoxInitOracle zsec = reinterpret_cast<BBoxInitOracle>(base + 0xe7360);
	BBoxInitOracle zpan = reinterpret_cast<BBoxInitOracle>(base + 0xefeb0);
	typedef unsigned (__cdecl* PtrFieldDiffOracle)(void**, void**);
	PtrFieldDiffOracle pfd = reinterpret_cast<PtrFieldDiffOracle>(base + 0x6e650);
	typedef float (__thiscall* PopOracle)(void*);
	PopOracle pop = reinterpret_cast<PopOracle>(base + 0xb3ae0);
	unsigned nf = 0;
	unsigned char bb1[0x40]; memset(bb1, 0x55, sizeof(bb1));
	bb(bb1);
	if(*(unsigned*)(bb1+0)!=0x80000000u || *(unsigned*)(bb1+4)!=0 || *(unsigned*)(bb1+8)!=0
		|| *(unsigned*)(bb1+0xc)!=0 || *(unsigned*)(bb1+0x10)!=0 || bb1[0x14]!=1){
		fprintf(stderr,"bb fail\n");++nf;}
	unsigned char zs[0x60]; memset(zs,0x66,sizeof(zs));
	zsec(zs);
	if(*(unsigned*)(zs+0x18)+*(unsigned*)(zs+0x1c)+*(unsigned*)(zs+0x20)+*(unsigned*)(zs+0x24)
		+*(unsigned*)(zs+0x44)+*(unsigned*)(zs+0x48)+*(unsigned*)(zs+0x4c)!=0){fprintf(stderr,"zsec fail\n");++nf;}
	unsigned char zp[0x20]; memset(zp,0x77,sizeof(zp));
	zpan(zp);
	if(*(unsigned*)(zp+0)+*(unsigned*)(zp+4)+*(unsigned*)(zp+8)+*(unsigned short*)(zp+0xc)
		+*(unsigned short*)(zp+0xe)+*(unsigned*)(zp+0x10)+*(unsigned*)(zp+0x14)!=0){fprintf(stderr,"zpan fail\n");++nf;}
	// 002896: (*(void**)[E+4])[0x10] - (*(void**)[E+8])[0x10]
	unsigned obA[8]={0}, obB[8]={0}; obA[4]=0xA0000000u; obB[4]=0x20000000u;
	void* pA=&obA[0]; void* pB=&obB[0];
	unsigned pdV = pfd(&pA, &pB);
	if(pdV != 0xA0000000u - 0x20000000u){fprintf(stderr,"pfd fail %08x\n",pdV);++nf;}
	// 004776: pop -- [this]=container ptr; (*[this])[0]=base data ptr,
	// [*this+4]=byte offset; returns float at base+offset, increments offset by 4.
	unsigned char ict[0x30]; memset(ict,0,sizeof(ict));
	float data[8]; for(int k=0;k<8;++k) data[k]=(float)(k+1);
	unsigned pool[2]; pool[0]=(unsigned)(size_t)data; pool[1]=0;
	*(void**)(ict+0)=pool;
	float of = pop(&ict[0]);
	if(of != data[0] || *(unsigned*)(pool+1)!=4 || *(unsigned*)(ict+0x10)!=(unsigned)(size_t)data){
		fprintf(stderr,"pop fail of=%08x newoff=%u\n", reinterpret_cast<unsigned&>(of), *(unsigned*)(pool+1));++nf;}
	printf("initbatch candidate failures=%u provisional=1\n", nf);
	}

	// -- Final small batch: 001645(set3+zero,ret8) 002150(x87 avg,ret0x10)
	//   004147(zero+0x18=-1).
	{
	typedef void (__thiscall* Set3Oracle)(void*, unsigned, unsigned);
	Set3Oracle s3 = reinterpret_cast<Set3Oracle>(base + 0x31680);
	typedef float (__thiscall* AvgOracle)(void*, void*, void*, float*, int);
	AvgOracle avg = reinterpret_cast<AvgOracle>(base + 0x53880);
	typedef void (__thiscall* ZeroEnOracle)(void*);
	ZeroEnOracle ze = reinterpret_cast<ZeroEnOracle>(base + 0x9a4e0);
	unsigned lf = 0;
	unsigned char ls[0x20]; memset(ls,0x55,sizeof(ls));
	s3(ls, 0xAAAAAAAAu, 0xBBBBBBBBu);
	if(*(unsigned*)(ls+0)!=0xBBBBBBBBu || *(unsigned*)(ls+4)!=0xAAAAAAAAu
		|| *(unsigned*)(ls+8)!=0 || *(unsigned*)(ls+0xc)!=0 || *(unsigned*)(ls+0x10)!=0){
		fprintf(stderr,"s3 fail\n");++lf;}
	// 002150: (arr[i+3]+arr[i])*0.5 ; arr=[E+0xc](3rd arg), i=[E+0x10](4th)
	float av[8]; for(int k=0;k<8;++k) av[k]=(float)k;
	float of = avg((void*)0, (void*)0, (void*)0, av, 2); // i=2: (5+2)*.5=3.5
	if(of != 3.5f){fprintf(stderr,"avg fail %f\n",(double)of);++lf;}
	// 004147: zero +0..0x14, +0x18=0xffffffff
	unsigned char zs2[0x20]; memset(zs2,0x66,sizeof(zs2));
	ze(zs2);
	if(*(unsigned*)(zs2+0)+*(unsigned*)(zs2+4)+*(unsigned*)(zs2+8)+*(unsigned*)(zs2+0xc)
		+*(unsigned*)(zs2+0x10)+*(unsigned*)(zs2+0x14)!=0 || *(unsigned*)(zs2+0x18)!=0xFFFFFFFFu){
		fprintf(stderr,"ze fail\n");++lf;}
	printf("finalbatch candidate failures=%u provisional=1\n", lf);
	}

	// -- Triple-field copy/store rows: 000509/001297(copy to arg)
	//   003968/000540(store args to this) 000507(copy from arg) 002686(ptr diff).
	{
	typedef void (__thiscall* CopyToOracle)(void*, unsigned*);
	CopyToOracle c520 = reinterpret_cast<CopyToOracle>(base + 0x10200);
	CopyToOracle c90 = reinterpret_cast<CopyToOracle>(base + 0x25870);
	typedef void (__thiscall* Store4Oracle)(void*, unsigned, unsigned, unsigned, unsigned);
	Store4Oracle s4 = reinterpret_cast<Store4Oracle>(base + 0x8f520);
	typedef void (__thiscall* Store3Oracle)(void*, unsigned, unsigned, unsigned);
	Store3Oracle s3a = reinterpret_cast<Store3Oracle>(base + 0x106f0);
	typedef void (__thiscall* CopyFromOracle)(void*, const unsigned*);
	CopyFromOracle cfr = reinterpret_cast<CopyFromOracle>(base + 0x101d0);
	typedef unsigned (__cdecl* PtrMaskDiffOracle)(void**, void**);
	PtrMaskDiffOracle pmd = reinterpret_cast<PtrMaskDiffOracle>(base + 0x662c0);
	unsigned gb = 0;
	unsigned char go[0x800]; memset(go, 0, sizeof(go));
	const unsigned a520=0x1111, a524=0x2222, a528=0x3333;
	memcpy(go+0x520,&a520,4); memcpy(go+0x524,&a524,4); memcpy(go+0x528,&a528,4);
	unsigned out1[3]={0,0,0}; c520(go,out1);
	if(out1[0]!=a520||out1[1]!=a524||out1[2]!=a528){fprintf(stderr,"c520 fail\n");++gb;}
	memcpy(go+0x90,&a520,4); memcpy(go+0x94,&a524,4); memcpy(go+0x98,&a528,4);
	unsigned out2[3]={0,0,0}; c90(go,out2);
	if(out2[0]!=a520||out2[1]!=a524||out2[2]!=a528){fprintf(stderr,"c90 fail\n");++gb;}
	s4(go,0xA1,0xB2,0xC3,0xD4);
	if(*(unsigned*)(go+0x58)!=0xA1||*(unsigned*)(go+0x5c)!=0xB2||*(unsigned*)(go+0x60)!=0xC3||*(unsigned*)(go+0x64)!=0xD4){fprintf(stderr,"s4 fail\n");++gb;}
	s3a(go,0xE1,0xF2,0x13);
	if(*(unsigned*)(go+0x52c)!=0xE1||*(unsigned*)(go+0x530)!=0xF2||*(unsigned*)(go+0x534)!=0x13){fprintf(stderr,"s3a fail\n");++gb;}
	unsigned in3[3]={0x41,0x52,0x63}; cfr(go,in3);
	if(*(unsigned*)(go+0x520)!=0x41||*(unsigned*)(go+0x524)!=0x52||*(unsigned*)(go+0x528)!=0x63){fprintf(stderr,"cfr fail\n");++gb;}
	unsigned obC[0x40]={0}, obD[0x40]={0}; obC[0x50/4]=0x400; obD[0x50/4]=0x100;
	void* pC=&obC[0]; void* pD=&obD[0];
	unsigned pmd1=pmd(&pC,&pD);
	// (0x400 & 0x1ff) - (0x100 & 0x1ff) = 0 - 0x100 = -256
	if(pmd1 != static_cast<unsigned>(0 - 256)){fprintf(stderr,"pmd fail %08x\n",pmd1);++gb;}
	printf("triplecopy candidate failures=%u provisional=1\n", gb);
	}

	// -- Multi-zero / multi-store / push_at / lane-init batch:
	//   005289(zero 0..28) 003966(store 5 args) 004778(cursor push) 002346(lane init).
	{
	typedef void (__thiscall* ZeroManyOracle)(void*);
	ZeroManyOracle zm = reinterpret_cast<ZeroManyOracle>(base + 0xe7180);
	typedef void (__thiscall* Store5Oracle)(void*, unsigned, unsigned, unsigned, unsigned, unsigned);
	Store5Oracle s5o = reinterpret_cast<Store5Oracle>(base + 0x8f4f0);
	typedef unsigned* (__thiscall* PushAtOracle)(void*, unsigned);
	PushAtOracle pa = reinterpret_cast<PushAtOracle>(base + 0xb3b00);
	typedef void (__thiscall* LaneInitOracle)(void*);
	LaneInitOracle li2 = reinterpret_cast<LaneInitOracle>(base + 0x5ab50);
	unsigned qb = 0;
	unsigned char q[0x100]; memset(q, 0x77, sizeof(q));
	zm(q);
	{
	size_t sum=0; for(int i=0;i<=0x28;i+=4) sum += *(unsigned*)(q+i) & 0xFF; (void)sum;
	if(*(unsigned*)(q+0)+*(unsigned*)(q+4)+*(unsigned*)(q+8)+*(unsigned*)(q+0xc)+*(unsigned*)(q+0x10)
		+*(unsigned*)(q+0x14)+*(unsigned*)(q+0x18)+*(unsigned*)(q+0x1c)+*(unsigned*)(q+0x20)+*(unsigned*)(q+0x24)+*(unsigned*)(q+0x28)!=0){fprintf(stderr,"zm fail\n");++qb;}
	}
	s5o(q,9,8,7,6,5);
	if(*(unsigned*)(q+0x44)!=9||*(unsigned*)(q+0x48)!=8||*(unsigned*)(q+0x4c)!=7||*(unsigned*)(q+0x50)!=6||*(unsigned*)(q+0x54)!=5){fprintf(stderr,"s5o fail\n");++qb;}
	// 004778: cursor push -- [this]=container, returns old cursor [this+0x10] and advances [ct+4]
	unsigned char ct2[0x40]; memset(ct2,0,sizeof(ct2));
	unsigned base2=(unsigned)(size_t)(q+0x80); // a base address
	unsigned cpool[2]; cpool[0]=base2; cpool[1]=8;
	*(void**)(ct2+0)=cpool;
	unsigned* ocv = pa(&ct2[0], 4);
	if(ocv != reinterpret_cast<unsigned*>(base2+8) || *(unsigned*)(cpool+1)!=12){fprintf(stderr,"pa fail ocv=%08x off=%u\n",(unsigned)(size_t)ocv,*(unsigned*)(cpool+1));++qb;}
	// check [this+0x10] holds base2+8 too
	if(*(unsigned*)(ct2+0x10) != base2+8){fprintf(stderr,"pa store fail\n");++qb;}
	// 002346: two-lane list init
	unsigned char lt[0x30]; memset(lt,0x55,sizeof(lt));
	li2(lt);
	if(*(unsigned*)(lt+0)!= (unsigned)(size_t)(lt+8) || *(unsigned*)(lt+4)!=(unsigned)(size_t)(lt+0x18)
		|| *(unsigned*)(lt+8)+*(unsigned*)(lt+0xc)+*(unsigned*)(lt+0x10)+*(unsigned*)(lt+0x18)+*(unsigned*)(lt+0x1c)+*(unsigned*)(lt+0x20)!=0){fprintf(stderr,"li2 fail\n");++qb;}
	printf("quadbatch candidate failures=%u provisional=1\n", qb);
	}

	// -- Multi-field copy + bit-get batch: 003975(copy4) 000542(copy3)
	//   003483(copy7) 004346/004350(copy3 + bit get).
	{
	typedef unsigned (__thiscall* Copy4Oracle)(void*, unsigned*, unsigned*, unsigned*, unsigned*);
	Copy4Oracle cp4 = reinterpret_cast<Copy4Oracle>(base + 0x8f690);
	typedef unsigned (__thiscall* Copy3bOracle)(void*, unsigned*, unsigned*, unsigned*);
	Copy3bOracle cp3 = reinterpret_cast<Copy3bOracle>(base + 0x10720);
	typedef unsigned (__thiscall* CopyNOracle)(void*, unsigned*);
	CopyNOracle cp7 = reinterpret_cast<CopyNOracle>(base + 0x85780);
	typedef unsigned (__thiscall* Copy3BitOracle)(void*, float*);
	Copy3BitOracle cb1 = reinterpret_cast<Copy3BitOracle>(base + 0xa91b0);
	Copy3BitOracle cb2 = reinterpret_cast<Copy3BitOracle>(base + 0xa9290);
	unsigned rf = 0;
	unsigned char r0[0x800]; memset(r0, 0, sizeof(r0));
	#define ST4(off,i) { unsigned _v=0x10000000u+i; memcpy(r0+(off),&_v,4); }
	ST4(0x58,1); ST4(0x5c,2); ST4(0x60,3); ST4(0x64,4);
	unsigned a0=0,a1=0,a2=0,a3=0; cp4(r0,&a0,&a1,&a2,&a3);
	if(a0!=0x10000001u||a1!=0x10000002u||a2!=0x10000003u||a3!=0x10000004u){fprintf(stderr,"cp4 fail\n");++rf;}
	ST4(0x52c,5); ST4(0x530,6); ST4(0x534,7);
	unsigned b0=0,b1=0,b2=0; cp3(r0,&b0,&b1,&b2);
	if(b0!=0x10000005u||b1!=0x10000006u||b2!=0x10000007u){fprintf(stderr,"cp3 fail\n");++rf;}
	ST4(0x5c,0x11); ST4(0x60,0x12); ST4(0x64,0x13); ST4(0x68,0x14); ST4(0x6c,0x15); ST4(0x70,0x16);
	unsigned o7[8]={0}; cp7(r0,o7);
	if(o7[0]!=0x10000011u||o7[4]!=0x10000015u||o7[5]!=0x10000016u){fprintf(stderr,"cp7 fail\n");++rf;}
	ST4(0x184,0x21); ST4(0x188,0x22); ST4(0x18c,0x23);
	unsigned b1v=0x2u; memcpy(r0+0x1a8,&b1v,4); // >>1 &1 -> 1
	unsigned f1[4]={0}; unsigned r1=cb1(r0,(float*)f1);
	if(r1!=1u||f1[0]!=0x10000021u||f1[1]!=0x10000022u||f1[2]!=0x10000023u){fprintf(stderr,"cb1 fail\n");++rf;}
	ST4(0x190,0x31); ST4(0x194,0x32); ST4(0x198,0x33);
	unsigned b2v=0x4u; memcpy(r0+0x1a8,&b2v,4); // >>2 &1 -> 1
	unsigned f2[4]={0}; unsigned r2=cb2(r0,(float*)f2);
	if(r2!=1u||f2[0]!=0x10000031u||f2[1]!=0x10000032u||f2[2]!=0x10000033u){fprintf(stderr,"cb2 fail\n");++rf;}
	#undef ST4
	printf("multicopy candidate failures=%u provisional=1\n", rf);
	}

	// -- rep-movsd pose-copy rows (001289/001295/001301/003563) and the
	//   clamp/LCG global rows (002515/002513).
	{
	typedef void (__thiscall* PoseCopy2Oracle)(void*, float*);
	PoseCopy2Oracle pc6c = reinterpret_cast<PoseCopy2Oracle>(base + 0x257e0);
	PoseCopy2Oracle pc6c2 = reinterpret_cast<PoseCopy2Oracle>(base + 0x25840);
	PoseCopy2Oracle pc0c = reinterpret_cast<PoseCopy2Oracle>(base + 0x258c0);
	PoseCopy2Oracle pc18 = reinterpret_cast<PoseCopy2Oracle>(base + 0x87e20);
	unsigned zf = 0;
	unsigned char pc[0x100]; memset(pc, 0, sizeof(pc));
	// fill 0x6c..0x8f and 0xc..0x2f and 0x18..0x3b with distinct words
	for(int i=0;i<12;++i){ unsigned v=0xA0000000u+i; memcpy(pc+0x6c+i*4,&v,4); }
	for(int i=0;i<12;++i){ unsigned v=0xB0000000u+i; memcpy(pc+0xc+i*4,&v,4); }
	for(int i=0;i<12;++i){ unsigned v=0xC0000000u+i; memcpy(pc+0x18+i*4,&v,4); }
	float outA[12]={0}; pc6c(pc, outA);
	if(memcmp(outA, pc+0x6c, 48)!=0){fprintf(stderr,"pc6c fail\n");++zf;}
	float outB[12]={0}; pc6c2(pc, outB);
	if(memcmp(outB, pc+0x6c, 48)!=0){fprintf(stderr,"pc6c2 fail\n");++zf;}
	float outC[12]={0}; pc0c(pc, outC);
	if(memcmp(outC, pc+0xc, 48)!=0){fprintf(stderr,"pc0c fail\n");++zf;}
	float outD[12]={0}; pc18(pc, outD);
	if(memcmp(outD, pc+0x18, 48)!=0){fprintf(stderr,"pc18 fail\n");++zf;}
	printf("posecopy2 candidate failures=%u provisional=1\n", zf);
	}

	// -- Clamp (002515) and LCG step (002513) on the global .data[0x10122340].
	{
	typedef void (__cdecl* ClampOracle)(int);	// reads [esp+4] as the single cdecl arg
	ClampOracle clampO = reinterpret_cast<ClampOracle>(base + 0x5fe50);
	typedef void (__cdecl* LcgOracle)(void*);
	LcgOracle lcgO = reinterpret_cast<LcgOracle>(base + 0x5fe20);
	int* gclamp = reinterpret_cast<int*>(const_cast<unsigned char*>(base + 0x122340));
	unsigned cf = 0;
	// clamp cases
	int cv[] = {-5, 0, 1, 2, 0x7ffffffe, 0x7fffffff};
	for(unsigned ci=0; ci<sizeof(cv)/sizeof(cv[0]); ++ci) {
		int v = cv[ci];
		*gclamp = 0;
		clampO(v);
		int expV = v<1 ? 1 : (v>=0x7fffffff ? 0x7ffffffe : v);
		if(*gclamp != expV){
			fprintf(stderr,"clamp fail v=%d g=%d\n",v,*gclamp); ++cf;
		}
	}
	// LCG step: seed, run 5 times, compare to oracle
	// formula: a=0x1f31d; eax = x/a, edx=x%a; edx*0xb14 - eax*0x41a7... verify
	*gclamp = 12345;
	unsigned s0 = *gclamp;
	// replicate: x -> ( (x%0x1f31d)*0xb14 ) - ( (x/0x1f31d)*0x41a7 )
	int x = s0;
	int q = x / 0x1f31d, r = x % 0x1f31d; // idiv: eax=q, edx=r
	// imul eax,eax,0xb14; imul edx,edx,0x41a7; sub edx,eax -> r*0xb14 - q*0x41a7? listing: imul eax,eax,0xb14 (q*0xb14); imul edx,edx,0x41a7 (r*0x41a7); sub edx,eax -> r*0x41a7 - q*0xb14
	long nx = (long)r*0x41a7 - (long)q*0xb14;
	if(nx <= 0) nx += 0x7fffffff;
	lcgO(0);
	if(*gclamp != (int)nx){fprintf(stderr,"lcg fail got=%d exp=%d\n",*gclamp,(int)nx); ++cf;}
	printf("clampfcg candidate failures=%u provisional=1\n", cf);
	}

	// -- Char-header init (003274), copy5 (003974), bit set/clear (003453).
	{
	typedef void (__thiscall* CharHeaderOracle)(void*, unsigned, unsigned);
	CharHeaderOracle ch = reinterpret_cast<CharHeaderOracle>(base + 0x7e8f0);
	typedef unsigned (__thiscall* Copy5Oracle)(void*, unsigned*, unsigned*, unsigned*, unsigned*, unsigned*);
	Copy5Oracle c5 = reinterpret_cast<Copy5Oracle>(base + 0x8f660);
	typedef void (__thiscall* BitSetClearOracle)(void*, unsigned, unsigned);	// reads [esp+4] and [esp+8]
	BitSetClearOracle bsc = reinterpret_cast<BitSetClearOracle>(base + 0x84ed0);
	unsigned hf = 0;
	unsigned char ch1[0x20]; memset(ch1, 0xA5, sizeof(ch1));
	ch(ch1, 0x11111111u, 0x22222222u);
	if(memcmp(ch1, "JOHNRAT", 7)!=0 || ch1[7]!=0 || *(unsigned*)(ch1+8)!=0x11111111u || *(unsigned*)(ch1+0xc)!=0x22222222u){
		fprintf(stderr,"ch fail bytes=%c%c%c%c\n",ch1[0],ch1[1],ch1[2],ch1[3]); ++hf;}
	// 003974: copy [this+0x44..0x54] to 5 outs
	unsigned char c5o[0x100]; memset(c5o,0,sizeof(c5o));
	#define SET5(off,i) { unsigned _v=0x50000000u+i; memcpy(c5o+(off),&_v,4); }
	SET5(0x44,1); SET5(0x48,2); SET5(0x4c,3); SET5(0x50,4); SET5(0x54,5);
	unsigned rA=0,rB=0,rC=0,rD=0,rE=0; c5(c5o,&rA,&rB,&rC,&rD,&rE);
	if(rA!=0x50000001u||rB!=0x50000002u||rC!=0x50000003u||rD!=0x50000004u||rE!=0x50000005u){fprintf(stderr,"c5 fail\n");++hf;}
	// 003453: flag byte at [esp+8]; if nonzero OR [esp+4] into [this+0x58], else clear
	unsigned char bs[0x80]; memset(bs,0,sizeof(bs));
	bs[0x58]=0x0Fu;
	bsc(bs, 0xA0u, 1u);	// flag=1, mask=0xA0 -> set
	if(bs[0x58] != 0xAFu){fprintf(stderr,"bs set fail %02x\n",bs[0x58]); ++hf;}
	bs[0x58]=0x0Fu;
	bsc(bs, 0xA0u, 0u);	// flag=0 -> clear
	if(bs[0x58] != 0x0Fu & ~0xA0u){fprintf(stderr,"bs clear fail %02x\n",bs[0x58]); ++hf;}
	#undef SET5
	printf("misc3 candidate failures=%u provisional=1\n", hf);
	}

	// -- Template-init rows: 000496 (0xfd10), 001455 (0x2ace0), 002314 (0x5a0a0).
	{
	typedef void (__thiscall* TmplInitOracle)(void*);
	TmplInitOracle ti0 = reinterpret_cast<TmplInitOracle>(base + 0xfd10);
	TmplInitOracle ti1 = reinterpret_cast<TmplInitOracle>(base + 0x2ace0);
	TmplInitOracle ti2 = reinterpret_cast<TmplInitOracle>(base + 0x5a0a0);
	unsigned mf = 0;
	unsigned char t0[0x100]; memset(t0, 0xA5, sizeof(t0));
	ti0(t0);
	if(*(unsigned*)(t0+0xc)+*(unsigned*)(t0+0x10)+*(unsigned*)(t0+0x14)+*(unsigned*)(t0+0x1c)
		+*(unsigned*)(t0+0x20)+*(unsigned*)(t0+0x24)!=0){fprintf(stderr,"ti0 zero fail\n");++mf;}
	if(*(unsigned*)(t0+8)!=0x3f800000u||*(unsigned*)(t0+0x18)!=0x3f800000u||*(unsigned*)(t0+0x28)!=0x3f800000u){fprintf(stderr,"ti0 one fail\n");++mf;}
	if(*(unsigned*)(t0+0x38)!=8u||*(int*)(t0+0x44)!=0 || *(int*)(t0+0x40)!=0){fprintf(stderr,"ti0 tail fail\n");++mf;}
	unsigned char t1[0x80]; memset(t1, 0xA5, sizeof(t1));
	ti1(t1);
	if(*(unsigned*)(t1+0)!=0x1010769cu){fprintf(stderr,"ti1 vptr fail\n");++mf;}
	for(int k=4;k<=0x48;k+=4) if(*(unsigned*)(t1+k)!=0){fprintf(stderr,"ti1 zero fail\n");++mf;break;}
	unsigned char t2[0x80]; memset(t2, 0xA5, sizeof(t2));
	ti2(t2);
	if(*(unsigned*)(t2+8)!=0x3f800000u||*(unsigned*)(t2+0x4c)+*(unsigned*)(t2+0x50)+*(unsigned*)(t2+0x54)!=0){fprintf(stderr,"ti2 fail\n");++mf;}
	printf("tmplinit candidate failures=%u provisional=1\n", mf);
	}

	// -- Cross-product row (002465, 0x5eac0): when [esp+4]==3 computes the
	//   3D double cross product A x B into C (arrays of doubles).
	{
	typedef void (__cdecl* CrossOracle)(void*, void*, void*, void*);
	CrossOracle cr = reinterpret_cast<CrossOracle>(base + 0x5eac0);
	unsigned cf2 = 0;
	const double A[3] = { 1.0, 2.0, 3.0 };
	const double B[3] = { 4.0, 5.0, 6.0 };
	{
	double outC[3];
	// cross = { a1*b2-b1*a2, a2*b0-b2*a0, a0*b1-b0*a1 }
	double expC[3] = { A[1]*B[2]-B[1]*A[2], A[2]*B[0]-B[2]*A[0], A[0]*B[1]-B[0]*A[1] };
	// need to place 3 in [esp+4] as the mode; call with a dummy preceding arg
	unsigned char aA[0x40], aB[0x40], aC[0x40];
	memcpy(aA, A, sizeof(A)); memcpy(aB, B, sizeof(B));
	// 002465 reads [esp+4](mode), [esp+8](A), [esp+0xc](B), [esp+0x10](C)
	// as cdecl 4 args; mode must equal 3 as an integer.
	cr(reinterpret_cast<void*>(3), aA, aB, aC);
	double* oc = reinterpret_cast<double*>(aC);
	for(int i=0;i<3;++i){
		if(oc[i]!=expC[i]){ fprintf(stderr,"cross fail %d got=%.6f exp=%.6f\n",i,(double)oc[i],(double)expC[i]); ++cf2; }
	}
	(void)outC;
	}
	printf("crossprod candidate failures=%u provisional=1\n", cf2);
	}

	// -- Bounded push row (003261, 0x7e4b0, ret 0xc): if [this+0x14] <
	//   [this+0x10], store 3 args into [this+0xc + [this+0x14]*12] and inc.
	{
	typedef void (__thiscall* BoundedPushOracle)(void*, unsigned, unsigned, unsigned);
	BoundedPushOracle bp = reinterpret_cast<BoundedPushOracle>(base + 0x7e4b0);
	unsigned pb = 0;
	unsigned char bp1[0x100]; memset(bp1, 0, sizeof(bp1));
	unsigned char* bpBase = bp1 + 0x40;		// writable scratch base
	*(void**)(bp1+0xc) = bpBase;			// [this+0xc] = base pointer
	unsigned cap=2; memcpy(bp1+0x10,&cap,4); *(unsigned*)(bp1+0x14)=0;
	bp(bp1, 0x11, 0x22, 0x33);
	if(*(unsigned*)(bpBase+0)!=0x11||*(unsigned*)(bpBase+4)!=0x22 || *(unsigned*)(bpBase+8)!=0x33){fprintf(stderr,"bp0 fail\n");++pb;}
	if(*(unsigned*)(bp1+0x14)!=1){fprintf(stderr,"bp count fail=%u\n",*(unsigned*)(bp1+0x14));++pb;}
	bp(bp1, 0x44, 0x55, 0x66);	// second into the next slot
	if(*(unsigned*)(bpBase+0xc)!=0x44|| *(unsigned*)(bp1+0x14)!=2){fprintf(stderr,"bp1 fail\n");++pb;}
	bp(bp1, 0x77, 0x88, 0x99);	// full now (0x14 >= cap 2) -> no write
	if(*(unsigned*)(bpBase+0x18)!=0 || *(unsigned*)(bp1+0x14)!=2){fprintf(stderr,"bp full fail\n");++pb;}
	printf("boundedpush candidate failures=%u provisional=1\n", pb);
	}

	// -- Template-init rows: 005360 (0xe9060), 003983 (0x8fc00),
	//   003989 (0x8fd00), 000499 (0xfec0).
	{
	typedef void (__thiscall* TmplInit2Oracle)(void*);
	TmplInit2Oracle tw0 = reinterpret_cast<TmplInit2Oracle>(base + 0xe9060);
	TmplInit2Oracle tw1 = reinterpret_cast<TmplInit2Oracle>(base + 0x8fc00);
	TmplInit2Oracle tw2 = reinterpret_cast<TmplInit2Oracle>(base + 0x8fd00);
	TmplInit2Oracle tw3 = reinterpret_cast<TmplInit2Oracle>(base + 0xfec0);
	unsigned ff = 0;
	unsigned char u0[0x60]; memset(u0, 0xA5, sizeof(u0));
	tw0(u0);
	if(*(unsigned*)(u0+0)!=0x1011bab4u||*(unsigned*)(u0+4)!=1u||*(unsigned*)(u0+8)!=0x7FFFFFFFu||*(unsigned*)(u0+0x10)!=0xFFFFFFFFu
		||*(unsigned*)(u0+0xc)+*(unsigned*)(u0+0x14)+*(unsigned*)(u0+0x18)+*(unsigned*)(u0+0x1c)+*(unsigned*)(u0+0x3c)+*(unsigned*)(u0+0x40)+*(unsigned*)(u0+0x44)!=0){fprintf(stderr,"tw0 fail\n");++ff;}
	unsigned char u1[0x80]; memset(u1, 0xA5, sizeof(u1));
	tw1(u1);
	if(*(unsigned*)(u1+8)+*(unsigned*)(u1+0x18)+*(unsigned*)(u1+0x28)!=3u*0x3f800000u){fprintf(stderr,"tw1 one fail\n");++ff;}
	if(*(unsigned*)(u1+0x38)!=8u||*(unsigned*)(u1+0x44)+*(unsigned*)(u1+0x4c)+*(unsigned*)(u1+0x50)+*(unsigned*)(u1+0x54)!=0){fprintf(stderr,"tw1 tail fail\n");++ff;}
	unsigned char u2[0x80]; memset(u2, 0xA5, sizeof(u2));
	tw2(u2);
	if(*(unsigned*)(u2+8)!=0x3f800000u||*(unsigned*)(u2+0x4c)+*(unsigned*)(u2+0x50)!=0||*(unsigned*)(u2+0x38)!=8u){fprintf(stderr,"tw2 fail\n");++ff;}
	unsigned char u3[0x80]; memset(u3, 0xA5, sizeof(u3));
	tw3(u3);
	if(*(unsigned*)(u3+0x50)!=0x3f800000u||*(unsigned*)(u3+0x54)+*(unsigned*)(u3+0x58)+*(unsigned*)(u3+0x4c)!=0){fprintf(stderr,"tw3 fail\n");++ff;}
	printf("tmplinit2 candidate failures=%u provisional=1\n", ff);
	}

	// -- Conditional dot-delta row (001668, 0x32810): sum of [this]<<2 etc
	//   gated by non-zero flags at +8/+0xc/+0x10; ret 0.
	{
	typedef unsigned (__thiscall* CondSumOracle)(void*);
	CondSumOracle csum = reinterpret_cast<CondSumOracle>(base + 0x32810);
	unsigned sf2 = 0;
	unsigned char s2[0x20]; memset(s2, 0, sizeof(s2));
	const unsigned v0=3, v1=5; memcpy(s2+0,&v0,4); memcpy(s2+4,&v1,4);
	unsigned f8=1,fc=0,f10=1; memcpy(s2+8,&f8,4); memcpy(s2+0xc,&fc,4); memcpy(s2+0x10,&f10,4);
	// flag8 -> [this]=3<<2=12; flag10 -> [this+4]=5<<2=20; sum=32
	unsigned r = csum(s2);
	if(r != 12u+20u){fprintf(stderr,"csum0 fail %u\n",r);++sf2;}
	memset(s2+0xc,0,4); f8=1;fc=0;f10=0; memcpy(s2+8,&f8,4); memcpy(s2+0x10,&f10,4);
	r = csum(s2);
	if(r != 12u){fprintf(stderr,"csum1 fail %u\n",r);++sf2;}
	printf("condsum candidate failures=%u provisional=1\n", sf2);
	}

	// -- Buffer reset (000505) and copy6+bit (004342).
	{
	typedef unsigned (__thiscall* BufResetOracle)(void*);
	BufResetOracle br = reinterpret_cast<BufResetOracle>(base + 0x10190);
	typedef unsigned (__thiscall* Copy6BitOracle)(void*, unsigned*);
	Copy6BitOracle c6b = reinterpret_cast<Copy6BitOracle>(base + 0xa90c0);
	unsigned gr = 0;
	// 000505: [this+8] points to a buffer, [this+4]=size to zero; then
	// [this+14]=size; returns [this+14]
	unsigned char brb[0x40]; memset(brb, 0, sizeof(brb));
	unsigned char brbuf[16]; memset(brbuf, 0xEE, sizeof(brbuf));
	*(void**)(brb+8)=brbuf; unsigned sz=7; memcpy(brb+4,&sz,4);
	unsigned full = 0xFFFFFFFFu; memcpy(brb+0x14,&full,4);	// inc wraps to 0 -> reset path
	unsigned szret = br(brb);
	if(szret != 7u || *(unsigned*)(brb+0x14)!=7u){fprintf(stderr,"br ret fail\n");++gr;}
	for(int i=0;i<7;++i) if(brbuf[i]!=0){fprintf(stderr,"br zero fail %d=%02x\n",i,brbuf[i]);++gr;}
	// 004342: copy [this+0x16c..0x17c] (6 dwords incl +0x174) to arg[0..0x14]
	unsigned char c6b8[0x800]; memset(c6b8, 0, sizeof(c6b8));
	#define SET6(off,i) { unsigned _v=0x60000000u+i; memcpy(c6b8+(off),&_v,4); }
	SET6(0x16c,1); SET6(0x170,2); SET6(0x174,3); SET6(0x178,4); SET6(0x17c,5); SET6(0x180,6);
	unsigned out6[6]={0};
	unsigned bitret = c6b(c6b8, out6);
	if(out6[0]!=0x60000001u||out6[5]!=0x60000006u){fprintf(stderr,"c6b copy fail\n");++gr;}
	#undef SET6
	printf("bufreset candidate failures=%u provisional=1\n", gr);
	}

	// -- Template/sentinel bbox init rows: 003987 (0x8fcb0), 003985
	//   (0x8fc50), 002148 (0x53810).
	{
	typedef void (__thiscall* TmplFmiOracle)(void*);
	TmplFmiOracle fm0 = reinterpret_cast<TmplFmiOracle>(base + 0x8fcb0);
	TmplFmiOracle fm1 = reinterpret_cast<TmplFmiOracle>(base + 0x8fc50);
	TmplFmiOracle fm2 = reinterpret_cast<TmplFmiOracle>(base + 0x53810);
	unsigned fmf = 0;
	unsigned char m0[0x60]; memset(m0, 0xA5, sizeof(m0));
	fm0(m0);
	if(*(unsigned*)(m0+8)+*(unsigned*)(m0+0x18)+*(unsigned*)(m0+0x28)!=3u*0x3f800000u){fprintf(stderr,"fm0 one fail\n");++fmf;}
	if(*(unsigned*)(m0+0x38)!=8u||*(unsigned*)(m0+0xc)!=0||*(unsigned*)(m0+0x4c)+*(unsigned*)(m0+0x44)+*(unsigned*)(m0+0x40)!=0){fprintf(stderr,"fm0 zero fail\n");++fmf;}
	unsigned char m1[0x80]; memset(m1, 0xA5, sizeof(m1));
	fm1(m1);
	if(*(unsigned*)(m1+0x58)+*(unsigned*)(m1+0x5c)!=2u*0x7f7fffffu||*(unsigned*)(m1+0x68)!=2u){fprintf(stderr,"fm1 sent fail\n");++fmf;}
	if(*(unsigned*)(m1+0x30)!=0x3f800000u||*(unsigned*)(m1+0x30)!=*(unsigned*)(m1+0x10)||*(unsigned*)(m1+0x3c)!=0x3f800000u){fprintf(stderr,"fm1 id fail\n");++fmf;}
	unsigned char m2[0x30]; memset(m2, 0xA5, sizeof(m2));
	fm2(m2);
	if(*(unsigned*)(m2+0)+*(unsigned*)(m2+4)+*(unsigned*)(m2+8)!=3u*0x7f7fffffu){fprintf(stderr,"fm2 max fail\n");++fmf;}
	if(*(unsigned*)(m2+0xc)+*(unsigned*)(m2+0x10)+*(unsigned*)(m2+0x14)!=3u*0xFF7FFFFFu){fprintf(stderr,"fm2 min fail\n");++fmf;}
	printf("tmplfm candidate failures=%u provisional=1\n", fmf);
	}

	// -- Buffer-pop row (001655, 0x32410, ret 4): read slot index into arg,
	//   increment index, reset when reaching capacity. Returns a byte flag.
	{
	typedef unsigned char (__thiscall* BufPopOracle)(void*, unsigned*);
	BufPopOracle bpop = reinterpret_cast<BufPopOracle>(base + 0x32410);
	unsigned bpf = 0;
	unsigned slots[4] = { 0xAAAA, 0xBBBB, 0xCCCC, 0xDDDD };
	unsigned char bp2[0x40]; memset(bp2, 0, sizeof(bp2));
	unsigned size=4; memcpy(bp2+4,&size,4);
	*(void**)(bp2+8)=slots;
	unsigned idx1=0; memcpy(bp2+0x10,&idx1,4);
	unsigned out=0;
	if(bpop(bp2,&out)!=1u || out!=0xAAAAu){fprintf(stderr,"bpop0 fail out=%08x\n",out);++bpf;}
	if(*(unsigned*)(bp2+0x10)!=1){fprintf(stderr,"bpop idx fail\n");++bpf;}
	bpop(bp2,&out); // slot1
	if(out!=0xBBBBu){fprintf(stderr,"bpop1 fail\n");++bpf;}
	printf("bufpop candidate failures=%u provisional=1\n", bpf);
	}

	// -- Ctor with list-link row (004407, 0xb0310, ret 0xc): sets vptr
	//   0x1011a648, stores [esp+4]/[esp+8]/[esp+0xc] and links into a list.
	{
	typedef void (__thiscall* CtorLinkOracle)(void*, void*, unsigned, unsigned);
	CtorLinkOracle crg = reinterpret_cast<CtorLinkOracle>(base + 0xb0310);
	unsigned clf = 0;
	unsigned char lc[0x40]; memset(lc, 0x00, sizeof(lc));
	unsigned char node[0x30]; memset(node, 0x00, sizeof(node));
	crg(lc, node, 0x1111u, 0x2222u);
	if(*(unsigned*)(lc+0)!=0x1011a648u){fprintf(stderr,"crg vptr fail\n");++clf;}
	// node != null -> link: [node+0xc]=lc, [lc+8]=node, [lc+0x10]=[node+0xc]? based on listing:
	// mov esi,[ecx+0xc]; [eax+0x10]=esi; [ecx+0x1c]=eax; [ecx+0xc]=eax; [eax+8]=ecx
	unsigned* nodeC = reinterpret_cast<unsigned*>(node);
	if(*(void**)(lc+8)!=node){fprintf(stderr,"crg link fail\n");++clf;}
	if(*(void**)(lc+0x10)!=0){fprintf(stderr,"crg oldprev fail mine=%p\n",*(void**)(lc+0x10));++clf;}
	if(*(void**)(node+0xc)!=reinterpret_cast<void*>(lc)){fprintf(stderr,"crg newprev fail\n");++clf;}
	if(*(unsigned*)(lc+0x14)!=0x1111u||*(unsigned*)(lc+0x18)!=0x2222u){fprintf(stderr,"crg args fail\n");++clf;}
	printf("ctorlink candidate failures=%u provisional=1\n", clf);
	}

	// -- Builder/four-copy rows: 002166 (0x53c80) and 004218 (0x9e470).
	{
	typedef unsigned char (__thiscall* BuilderOracle)(void*, unsigned*);
	BuilderOracle bd = reinterpret_cast<BuilderOracle>(base + 0x53c80);
	typedef void (__thiscall* Copy10Oracle)(void*, const unsigned*);
	Copy10Oracle c10 = reinterpret_cast<Copy10Oracle>(base + 0x9e470);
	unsigned bdf = 0;
	unsigned char src4[0x100]; memset(src4, 0, sizeof(src4));
	#define SET4(off,i) { unsigned _v=0x70000000u+i; memcpy(src4+(off),&_v,4); }
	SET4(8,1); SET4(0xc,2); SET4(0x10,3); SET4(0x14,4);
	SET4(0xa0,5); SET4(0x18,6); SET4(0x7c,7); SET4(0x80,8);
	unsigned out4[0x40]; memset(out4, 0, sizeof(out4));
	unsigned char br = bd(src4, out4);
	if(br!=1u){fprintf(stderr,"bd ret fail %u\n",br);++bdf;}
	if(out4[0]!=0x70000001u||out4[1]!=0x70000002u||out4[2]!=0xCu||out4[3]!=0xCu){fprintf(stderr,"bd front fail\n");++bdf;}
	if(out4[4]!=0x70000003u||out4[5]!=0x70000004u||out4[6]!=4u||out4[7]!=2u||out4[8]!=0x70000006u){fprintf(stderr,"bd mid fail\n");++bdf;}
	if(out4[9]!=0x70000007u||out4[10]!=0x70000008u||out4[11]!=0u){fprintf(stderr,"bd tail fail\n");++bdf;}
	// 004218: copy [arg+0x6c..0x90] to [this+0x16c..0x190]
	unsigned char c10b[0x200]; memset(c10b, 0, sizeof(c10b));
	unsigned char c10in[0xa0]; memset(c10in, 0, sizeof(c10in));
	for(int i=0;i<10;++i){ unsigned v=0x80000000u+i; memcpy(c10in+0x6c+i*4,&v,4); }
	c10(c10b, reinterpret_cast<const unsigned*>(c10in));
	unsigned okc=1;
	for(int i=0;i<10;++i) if(*(unsigned*)(c10b+0x16c+i*4)!=0x80000000u+i) okc=0;
	if(!okc){fprintf(stderr,"c10 fail\n");++bdf;}
	#undef SET4
	printf("builder4 candidate failures=%u provisional=1\n", bdf);
	}

	// -- Mark-degenerate loop (004091, 0x95d60): for i in [base, base+delta),
	//   OR 0x20 into array[slot(i) + 0xc] where array = [*[this+0x30] + 0x5b8]
	//   and slot stride is 0x50.
	{
	typedef void (__thiscall* MarkDegOracle)(void*);
	MarkDegOracle md = reinterpret_cast<MarkDegOracle>(base + 0x95d60);
	unsigned mdf = 0;
	unsigned char inner[0x1000]; memset(inner, 0, sizeof(inner));	// the +0x30 target
	unsigned char* cmpObj = inner + 0x200;						// the object [this+0x30] points to
	unsigned char* arr = inner + 0x800;							// [*target + 0x5b8] points here
	*(void**)(cmpObj + 0x5b8) = arr;							// target[0x5b8] -> array
	unsigned char mdck[0x200]; memset(mdck, 0x00, sizeof(mdck));
	*(void**)(mdck+0x30) = cmpObj;								// [this+0x30] -> obj
	unsigned base=1, delta=3; memcpy(mdck+0x160,&base,4); memcpy(mdck+0x164,&delta,4);
	// pre-mark base+2 with 0x10 to verify OR (0x10|0x20=0x30)
	unsigned pre=0x10; memcpy(arr + 2*0x50 + 0xc, &pre, 4);
	md(mdck);
	unsigned f0=0, f1=0, f2=0;
	memcpy(&f0, arr + 1*0x50 + 0xc, 4);
	memcpy(&f1, arr + 2*0x50 + 0xc, 4);
	memcpy(&f2, arr + 3*0x50 + 0xc, 4);
	if(f0!=0x20u){fprintf(stderr,"md0 fail %08x\n",f0);++mdf;}
	if(f1!=0x30u){fprintf(stderr,"md1 fail %08x\n",f1);++mdf;}
	if(f2!=0x20u){fprintf(stderr,"md2 fail %08x\n",f2);++mdf;}
	// outside range untouched
	unsigned other=0; memcpy(&other, arr + 4*0x50 + 0xc, 4);
	if(other!=0){fprintf(stderr,"md out fail\n");++mdf;}
	printf("markdeg candidate failures=%u provisional=1\n", mdf);
	}

	// -- x87 accumulate row (004087, 0x95cc0, ret 0xc): scale = a/b,
	//   accumulate scale*vec into [this+0x154/0x158/0x15c].
	{
	typedef void (__thiscall* XAccumOracle)(void*, float, const float*, float);
	XAccumOracle xa = reinterpret_cast<XAccumOracle>(base + 0x95cc0);
	unsigned xaf = 0;
	unsigned char xb[0x180]; memset(xb, 0, sizeof(xb));
	float iv[3] = { 2.0f, 3.0f, 4.0f };
	float stale[3]={0.5f,0.5f,0.5f};
	memcpy(xb+0x154,stale+0,4); memcpy(xb+0x158,stale+1,4); memcpy(xb+0x15c,stale+2,4);
	xa(xb, 1.5f, iv, 1.0f);	// scale = 1.5/1.0
	float p154, p158, p15c;
	memcpy(&p154,xb+0x154,4); memcpy(&p158,xb+0x158,4); memcpy(&p15c,xb+0x15c,4);
	float e154 = 0.5f + 1.5f*2.0f;	// 0.5 + 3 = 3.5
	float e158 = 0.5f + 1.5f*3.0f;	// 0.5 + 4.5 = 5.0
	float e15c = 0.5f + 1.5f*4.0f;	// 0.5 + 6 = 6.5
	if(p154!=e154){fprintf(stderr,"xa154 fail %f/%f\n",(double)p154,(double)e154);++xaf;}
	if(p158!=e158){fprintf(stderr,"xa158 fail %f/%f\n",(double)p158,(double)e158);++xaf;}
	if(p15c!=e15c){fprintf(stderr,"xa15c fail %f/%f\n",(double)p15c,(double)e15c);++xaf;}
	printf("xaccum candidate failures=%u provisional=1\n", xaf);
	}

	// -- Flag-based pointer-select row (004903, 0xb5770): returns one of a
	//   set of static table pointers from [this+0x84]/[this+4]/[this+0x8c].
	{
	typedef unsigned (__thiscall* FlagSelOracle)(void*);
	FlagSelOracle fso = reinterpret_cast<FlagSelOracle>(base + 0xb5770);
	unsigned fsf = 0;
	{
	unsigned char fr1[0x100]; memset(fr1,0,sizeof(fr1));
	float v1=1.0f; memcpy(fr1+0x84,&v1,4); // ordered > 0 -> skip the fcomp return
	unsigned b1=2u; memcpy(fr1+4,&b1,4); fr1[0x8c]=0x00;
	// edx=2 nonzero; al&1 = 0 -> not jne -> return 0x1011b6ec
	if(fso(fr1)!=0x1011b6ecu){fprintf(stderr,"fs1 fail %08x\n",fso(fr1));++fsf;}
	}
	{
	unsigned char fr2[0x100]; memset(fr2,0,sizeof(fr2));
	float v2=1.0f; memcpy(fr2+0x84,&v2,4);
	unsigned b2=0x10u; memcpy(fr2+4,&b2,4); fr2[0x8c]=0x00;
	// edx=0 and cl=0 -> L5: al&0x10 = 0x10 -> return 0x1011b638
	if(fso(fr2)!=0x1011b638u){fprintf(stderr,"fs2 fail %08x\n",fso(fr2));++fsf;}
	}
	{
	unsigned char fr3[0x100]; memset(fr3,0,sizeof(fr3));
	float v3=1.0f; memcpy(fr3+0x84,&v3,4);
	unsigned b3=0u; memcpy(fr3+4,&b3,4); fr3[0x8c]=0x00;
	// L5 with al bit4 clear (0) -> returns 0
	if(fso(fr3)!=0u){fprintf(stderr,"fs3 fail %08x\n",fso(fr3));++fsf;}
	}
	printf("flagsel candidate failures=%u provisional=1\n", fsf);
	}

	// -- LCG-step-to-float row (002517, 0x5fe80): computes the LCG value on
	//   .data[0x10122340], then converts to a float via fild * qword[0x10124828]
	//   + qword[0x10124830].
	{
	typedef float (__cdecl* LcgFloatOracle)();
	LcgFloatOracle lfo = reinterpret_cast<LcgFloatOracle>(base + 0x5fe80);
	unsigned lff = 0;
	int* gval = reinterpret_cast<int*>(const_cast<unsigned char*>(base + 0x122340));
	double* gmult = reinterpret_cast<double*>(const_cast<unsigned char*>(base + 0x124828));
	double* gadd = reinterpret_cast<double*>(const_cast<unsigned char*>(base + 0x124830));
	*gval = 12345;
	int x0 = *gval;
	int q = x0 / 0x1f31d, r = x0 % 0x1f31d;
	long nx = (long)r*0x41a7 - (long)q*0xb14;
	if(nx <= 0) nx += 0x7fffffff;
	float rf = lfo();
	if(*gval != (int)nx){fprintf(stderr,"lfo state fail %d/%d\n",*gval,(int)nx);++lff;}
	float ef = (float)((double)nx * (*gmult) + (*gadd));
	if(rf != ef){fprintf(stderr,"lfo float fail %f/%f\n",(double)rf,(double)ef);++lff;}
	printf("lcgfloat candidate failures=%u provisional=1\n", lff);
	}

	// -- Compact decision rows: 003928 (cond zero-store), 005356 (all-nonzero),
	//   002687 (neg-diff), 001457 (bit-sum).
	{
	typedef void (__thiscall* CondZeroOracle)(void*, unsigned, unsigned);
	CondZeroOracle cz = reinterpret_cast<CondZeroOracle>(base + 0x8edb0);
	typedef unsigned char (__thiscall* AllSetOracle)(void*);
	AllSetOracle as = reinterpret_cast<AllSetOracle>(base + 0xe8fb0);
	unsigned zf2 = 0;
	// 003928: if [esp+4]==0x100 and [this+0x24]==[esp+8] -> [this+0x24]=0 else [this+0x28]=0 (ret 8)
	unsigned char czb[0x40]; memset(czb,0,sizeof(czb));
	unsigned v24=0x4242; memcpy(czb+0x24,&v24,4);
	cz(czb, 0x100u, 0x4242u);
	if(*(unsigned*)(czb+0x24)!=0){fprintf(stderr,"cz0 fail\n");++zf2;}
	unsigned v243=0x4343u, v283=0x4444u; memcpy(czb+0x24,&v243,4); memcpy(czb+0x28,&v283,4);
	cz(czb, 0x100u, 0xEEEEu); // [this+0x24]=0x4343 != 0xeeee -> zero [this+0x28]
	if(*(unsigned*)(czb+0x28)!=0 || *(unsigned*)(czb+0x24)!=0x4343u){fprintf(stderr,"cz1 fail\n");++zf2;}
	unsigned v242=0x4242u; memcpy(czb+0x24,&v242,4);
	cz(czb, 0x200u, 0x4242u); // mode != 0x100 -> zero [this+0x28]
	if(*(unsigned*)(czb+0x28)!=0){fprintf(stderr,"cz2 fail\n");++zf2;}
	// 005356: 1 if all [this+0/4/8/0xc] nonzero
	unsigned char asb[0x20]; memset(asb,0,sizeof(asb));
	if(as(asb)!=0){fprintf(stderr,"as0 fail\n");++zf2;}
	*(unsigned*)(asb+0)=1; *(unsigned*)(asb+4)=1; *(unsigned*)(asb+8)=1; *(unsigned*)(asb+0xc)=1;
	if(as(asb)!=1){fprintf(stderr,"as1 fail\n");++zf2;}
	*(unsigned*)(asb+0xc)=0;
	if(as(asb)!=0){fprintf(stderr,"as2 fail\n");++zf2;}
	printf("compact decisions failure=%u provisional=1\n", zf2);
	}

	// -- Neg-diff (002687, 0x662e0) and conditional bit-sum (001457, 0x2ad30).
	{
	typedef int (__cdecl* NegDiffOracle)(void**, void**);
	NegDiffOracle nd = reinterpret_cast<NegDiffOracle>(base + 0x662e0);
	typedef unsigned (__thiscall* BitSumOracle)(void*);
	BitSumOracle bs2 = reinterpret_cast<BitSumOracle>(base + 0x2ad30);
	unsigned dbf = 0;
	// 002687: a = (*ea)[0x48] or negate (*ea)[0x4c] if 0; same b; return a-b
	unsigned ca[0x60]={0}, cb[0x60]={0};
	ca[0x48/4]=0xA0u; cb[0x48/4]=0x20u;
	void* pa=ca; void* pb=cb;
	if(nd(&pa,&pb)!=0x80){fprintf(stderr,"nd0 fail %d\n",nd(&pa,&pb));++dbf;}
	// a[0x48]=0 -> use neg a[0x4c]=0x5 -> -5 ; b[0x48]=0x20 -> 0x20 ; result -5-0x20
	unsigned moc=0x5u; ca[0x48/4]=0; ca[0x4c/4]=moc; cb[0x48/4]=0x20;
	if(nd(&pa,&pb)!=-5-0x20){fprintf(stderr,"nd1 fail %d\n",nd(&pa,&pb));++dbf;}
	// 001457: if [this+8]!=0 add ([this+4]*6 to eax); if [this+0x10]!=0 add ([this+0xc]*12)
	unsigned char dbs[0x20]; memset(dbs,0,sizeof(dbs));
	unsigned sd=3, v8=1; memcpy(dbs+4,&sd,4); memcpy(dbs+8,&v8,4);
	unsigned sc=4, v10=1; memcpy(dbs+0xc,&sc,4); memcpy(dbs+0x10,&v10,4);
	// [this+4]=3 -> x3? lea eax,[eax+eax*2]; shl 1 -> x6 = 18; [this+0xc]=4 -> ecx*3 then *4 = 48
	if(bs2(dbs)!=18u+48u){fprintf(stderr,"bs2 fail %u\n",bs2(dbs));++dbf;}
	printf("negdiff candidate failures=%u provisional=1\n", dbf);
	}

	// -- Double signed compare (002894, 0x6e620): returns +1 if *(*pa) <=
	//   *(*pb), else -1.
	{
	typedef int (__cdecl* DCmpOracle)(double**, double**);
	DCmpOracle dc = reinterpret_cast<DCmpOracle>(base + 0x6e620);
	unsigned dcf = 0;
	double va=2.0, vb=5.0;
	double* pa2=&va; double* pb2=&vb;
	if(dc(&pa2,&pb2)!=-1){fprintf(stderr,"dc0 fail\n");++dcf;}	// 2<=5 -> -1
	va=9.0; vb=3.0;
	if(dc(&pa2,&pb2)!=1){fprintf(stderr,"dc1 fail\n");++dcf;}		// 9>3 -> +1
	va=4.0; vb=4.0;
	if(dc(&pa2,&pb2)!=-1){fprintf(stderr,"dc2 fail\n");++dcf;}	// 4==4 -> -1
	printf("dcmp candidate failures=%u provisional=1\n", dcf);
	}

	// -- Indexed double-deref lookup (001958, 0x4be90, ret 4): if
	//   [this+0x1c] nonzero, returns [*( [table+0x18] + [table+4]*4 ) [idx] ] + 4.
	{
	typedef unsigned (__thiscall* IndexedLookupOracle)(void*, unsigned);
	IndexedLookupOracle il = reinterpret_cast<IndexedLookupOracle>(base + 0x4be90);
	unsigned ilf = 0;
	unsigned char tbl[0x20]; memset(tbl,0,sizeof(tbl));
	unsigned subcount=1; memcpy(tbl+4,&subcount,4);
	unsigned base=0x1000; memcpy(tbl+0x18,&base,4);	// base + subcount*4 points into an array
	// array element array at 'base' (simulate with a buffer pointer)
	unsigned char arr[0x40]; memset(arr,0,sizeof(arr));
	unsigned elem0=0x2000; memcpy(arr+0, &elem0, 4);	// arr[0] = ptr to an object
	unsigned objA=0x3000; // an object whose +4 field is the return
	unsigned char objbuf[8]; memset(objbuf,0,sizeof(objbuf)); unsigned field=0xA5A5; memcpy(objbuf+4,&field,4);
	// Need real addresses; rebuild with actual pointers
	unsigned char ilo[0x100]; memset(ilo,0,sizeof(ilo));
	unsigned sub=1; memcpy(ilo+4,&sub,4);
	unsigned oxt = (unsigned)(size_t)(ilo+0x40); 		// table+0x18 -> a base
	// The slot array is at base + sub*4 (i.e. oxt+4)
	unsigned char* slotArr = reinterpret_cast<unsigned char*>(oxt + 4);
	unsigned char obj[8]; unsigned fl=0x5A5Au; memcpy(obj+4,&fl,4);
	*(unsigned*)(slotArr+0)= (unsigned)(size_t)obj;		// slot[0] = &obj
	// [this+0x1c] points to the table
	*(unsigned*)(ilo+0x18)= oxt;
	*(void**)(ilo+0x1c)= ilo;							// [this+0x1c]=ilo (table)
	if(il(ilo, 0)!=0x5A5Au){fprintf(stderr,"il0 fail %u\n",il(ilo,0));++ilf;}
	printf("indexedlookup candidate failures=%u provisional=1\n", ilf);
	}

	// -- List-contains (003296, 0x7f020): walks the pointer array at
	//   [list+4], returning 1 if key appears (terminator 0), else 0.
	{
	typedef unsigned (__cdecl* ContainsOracle)(void*, unsigned);
	ContainsOracle ct = reinterpret_cast<ContainsOracle>(base + 0x7f020);
	unsigned ctf = 0;
	unsigned char chead[0x30]; memset(chead, 0, sizeof(chead));
	unsigned char nA[4], nB[4], nC[4];
	unsigned pA=(unsigned)(size_t)&nA, pB=(unsigned)(size_t)&nB, pC=(unsigned)(size_t)&nC;
	unsigned* arr = reinterpret_cast<unsigned*>(chead+4);
	arr[0]=pA; arr[1]=pB; arr[2]=pC; arr[3]=0;
	if(ct(chead, pB)!=1u){fprintf(stderr,"ct found fail\n");++ctf;}
	if(ct(chead, 0xF00DF00Du)!=0u){fprintf(stderr,"ct miss fail\n");++ctf;}
	printf("containsf candidate failures=%u provisional=1\n", ctf);
	}

	// -- Lookup-by-field (003105, 0x76200) and indexed-deref variant
	//   (001962, 0x4bee0, ret 4).
	{
	typedef unsigned (__cdecl* LookupFieldOracle)(void*, unsigned);
	LookupFieldOracle lf2 = reinterpret_cast<LookupFieldOracle>(base + 0x76200);
	unsigned l2f = 0;
	unsigned char headb[0x20]; memset(headb,0,sizeof(headb));
	unsigned char nodeX[0x20], nodeY[0x20]; memset(nodeX,0,0x20); memset(nodeY,0,0x20);
	// [headb+8]=key (headb IS both base and key); [headb+4] = node array
	unsigned* nArr = reinterpret_cast<unsigned*>(headb+4);
	nArr[0]=(unsigned)(size_t)nodeX; nArr[1]=(unsigned)(size_t)nodeY; nArr[2]=0;
	*(unsigned*)(nodeY+8)=(unsigned)(size_t)headb;	// nodeY[8]==headb -> found
	// 003105: returns node whose [node+8]==[esp+8] (== headb)
	unsigned r1 = lf2(headb, (unsigned)(size_t)headb);
	if(r1!=(unsigned)(size_t)nodeY){fprintf(stderr,"lf found fail %08x\n",r1);++l2f;}
	unsigned char nodeZ[0x20]; memset(nodeZ,0,0x20);
	*(unsigned*)(nodeZ+8)=0x12345678u;	// no node[8]==[esp+8]=headb in [headb+4]? nodeY does match
	// make all mismatch: clear nodeY[8]
	*(unsigned*)(nodeY+8)=0x11111111u;
	unsigned r2 = lf2(headb, (unsigned)(size_t)headb);
	if(r2!=0u){fprintf(stderr,"lf miss fail %08x\n",r2);++l2f;}
	printf("lookupfield candidate failures=%u provisional=1\n", l2f);
	}

	// -- Nested indexed deref (001962, 0x4bee0, ret 4): table =
	//   [this + [this+0x70]*4 + 0x1c]; then returns *( *(table+0x18 +
	//   [table+4]*4)[idx] ) + 4.
	{
	typedef unsigned (__thiscall* NestLookupOracle)(void*, unsigned);
	NestLookupOracle nl = reinterpret_cast<NestLookupOracle>(base + 0x4bee0);
	unsigned nlf = 0;
	unsigned char nbase[0x100]; memset(nbase,0,sizeof(nbase));
	// [this+0x1c] array; [this+0x70]=0 so table = *[this+0x1c]
	unsigned char* tbl = nbase + 0x40;
	*(void**)(nbase+0x1c) = tbl;
	const unsigned idx0=0; memcpy(nbase+0x70,&idx0,4);
	// inner table: [tbl+4]=sub, [tbl+0x18]=base, slots at base+sub*4
	unsigned sub=1; memcpy(tbl+4,&sub,4);
	unsigned char* oxt = nbase + 0x48; memcpy(tbl+0x18,&oxt,4);
	unsigned char* slotArr = oxt + sub*4;			// oxt(0x48)+4 = nbase+0x4c
	unsigned char obj7[8]; unsigned f7=0xBEEFu; memcpy(obj7+4,&f7,4);
	*(unsigned*)(slotArr+0) = (unsigned)(size_t)obj7;
	if(nl(nbase, 0)!=0xBEEFu){fprintf(stderr,"nl fail %u\n",nl(nbase,0));++nlf;}
	printf("nestlookup candidate failures=%u provisional=1\n", nlf);
	}

	// -- Jump-table switch rows: 002202 (0x546b0) and 002208 (0x547b0),
	//   both ret 8. When [esp+4]!=0 or [esp+8]>4 -> 0; else case 0..4.
	{
	typedef unsigned (__thiscall* SwitchOracle)(void*, unsigned, unsigned);
	SwitchOracle sw22 = reinterpret_cast<SwitchOracle>(base + 0x546b0);
	SwitchOracle sw28 = reinterpret_cast<SwitchOracle>(base + 0x547b0);
	unsigned swf = 0;
	unsigned char swb[0x100]; memset(swb,0,sizeof(swb));
	unsigned a0=5; memcpy(swb+0xa0,&a0,4);
	// 002202: cases {4,1,1,[a0]!=0,[a0]?4:0} for key 0..4
	unsigned e22[] = {4,1,1,1,4};	// [a0]=5 nonzero
	if(sw22(swb, 0u, 0u)!=e22[0]||sw22(swb,0u,1u)!=e22[1]||sw22(swb,0u,2u)!=e22[2]||sw22(swb,0u,3u)!=e22[3]||sw22(swb,0u,4u)!=e22[4]){fprintf(stderr,"sw22 case fail\n");++swf;}
	if(sw22(swb, 0u, 5u)!=0u){fprintf(stderr,"sw22 oob fail\n");++swf;}
	if(sw22(swb, 1u, 0u)!=0u){fprintf(stderr,"sw22 first fail\n");++swf;}
	// 002208: cases {0xc,0xc,0xc,[a0]?0xc:0,0} for key 0..4; [a0]=5
	unsigned e28[] = {0xc,0xc,0xc,0xc,0};
	if(sw28(swb, 0u, 0u)!=e28[0]||sw28(swb,0u,1u)!=e28[1]||sw28(swb,0u,2u)!=e28[2]||sw28(swb,0u,3u)!=e28[3]||sw28(swb,0u,4u)!=e28[4]){fprintf(stderr,"sw28 case fail\n");++swf;}
	if(sw28(swb, 0u, 5u)!=0u){fprintf(stderr,"sw28 oob fail\n");++swf;}
	printf("switch candidate failures=%u provisional=1\n", swf);
	}

	// -- Adjacent-pair search (005189, 0xe41a0, ret 8): finds which element
	//   pair of [this],[this+4],[this+8] equals (arg1,arg2) and returns its
	//   index 0/1/2, or -1.
	{
	typedef unsigned char (__thiscall* PairSearchOracle)(void*, unsigned, unsigned);
	PairSearchOracle ps = reinterpret_cast<PairSearchOracle>(base + 0xe41a0);
	unsigned psf = 0;
	unsigned char pr[0x20]; memset(pr,0,sizeof(pr));
	const unsigned K1=11, K2=22;
	// case 0: [this]=K1, [this+4]=K2
	*(unsigned*)(pr+0)=K1; *(unsigned*)(pr+4)=K2; *(unsigned*)(pr+8)=0x99;
	if(ps(pr,K1,K2)!=0){fprintf(stderr,"ps0 fail %u\n",ps(pr,K1,K2));++psf;}
	// case 1: [this]=K1, [this+8]=K2
	*(unsigned*)(pr+0)=K1; *(unsigned*)(pr+4)=0x99; *(unsigned*)(pr+8)=K2;
	if(ps(pr,K1,K2)!=1){fprintf(stderr,"ps1 fail %u\n",ps(pr,K1,K2));++psf;}
	// case 2: [this+4]=K1, [this+8]=K2
	*(unsigned*)(pr+0)=0x99; *(unsigned*)(pr+4)=K1; *(unsigned*)(pr+8)=K2;
	if(ps(pr,K1,K2)!=2){fprintf(stderr,"ps2 fail %u\n",ps(pr,K1,K2));++psf;}
	// miss: none match
	*(unsigned*)(pr+0)=1; *(unsigned*)(pr+4)=2; *(unsigned*)(pr+8)=3;
	if(ps(pr,K1,K2)!=0xFF){fprintf(stderr,"ps miss fail %u\n",ps(pr,K1,K2));++psf;}
	printf("pairsearch candidate failures=%u provisional=1\n", psf);
	}

	// -- Array reverse (001657, 0x32460, ret 0): reverses the first
	//   [esp+4] words of the [esp+8] array (swaps in place). Returns 1.
	{
	typedef unsigned char (__cdecl* ReverseOracle)(unsigned, unsigned*);
	ReverseOracle rev = reinterpret_cast<ReverseOracle>(base + 0x32460);
	unsigned rvf = 0;
	unsigned arr[6] = {1,2,3,4,5,6};
	unsigned char r = rev(6u, arr);
	if(r!=1u){fprintf(stderr,"rev ret fail %u\n",r);++rvf;}
	if(arr[0]!=6||arr[1]!=5||arr[2]!=4||arr[3]!=3||arr[4]!=2||arr[5]!=1){fprintf(stderr,"rev order fail\n");++rvf;}
	// reverse of just 3 leaves the rest
	unsigned arr2[4] = {7,8,9,10};
	rev(3u, arr2);
	if(arr2[0]!=9||arr2[1]!=8||arr2[2]!=7||arr2[3]!=10){fprintf(stderr,"rev3 fail\n");++rvf;}
	printf("reversearr candidate failures=%u provisional=1\n", rvf);
	}

	// -- Flag-and-compare row (002684, 0x66270): derefs two object pointers,
	//   checks the 0x100000 flag in [+0x50], then compares the [+0x20] doubles.
	{
	typedef int (__cdecl* FlagCmpOracle)(void**, void**);
	FlagCmpOracle fco = reinterpret_cast<FlagCmpOracle>(base + 0x66270);
	unsigned fof = 0;
	unsigned char oA[0x60], oB[0x60]; memset(oA,0,sizeof(oA)); memset(oB,0,sizeof(oB));
	unsigned fa=0x100000u; memcpy(oA+0x50,&fa,4); memcpy(oB+0x50,&fa,4);
	void* pA=&oA; void* pB=&oB;
	// A flag clear -> -1
	unsigned clr=0u; memcpy(oA+0x50,&clr,4);
	if(fco(&pA,&pB)!=-1){fprintf(stderr,"fco0 fail %d\n",fco(&pA,&pB));++fof;}
	// A set, B clear -> +1
	memcpy(oA+0x50,&fa,4); memcpy(oB+0x50,&clr,4);
	if(fco(&pA,&pB)!=1){fprintf(stderr,"fco1 fail %d\n",fco(&pA,&pB));++fof;}
	// both set, A[+0x20]=1.0 B[+0x20]=2.0 (A<=B) -> -1
	memcpy(oB+0x50,&fa,4);
	double a1=1.0,b2=2.0; memcpy(oA+0x20,&a1,8); memcpy(oB+0x20,&b2,8);
	if(fco(&pA,&pB)!=-1){fprintf(stderr,"fco2 fail %d\n",fco(&pA,&pB));++fof;}
	printf("flagcmp candidate failures=%u provisional=1\n", fof);
	}

	// -- Chained-field copy (004068, 0x95a40, ret 8): writes the +0x19c word
	//   of [this+8] (or 0) to out1, and of [this+0xc] (or 0) to out2.
	{
	typedef void (__thiscall* ChainCopyOracle)(void*, unsigned*, unsigned*);
	ChainCopyOracle cch = reinterpret_cast<ChainCopyOracle>(base + 0x95a40);
	unsigned cgf = 0;
	unsigned char nA[0x200], nB[0x200]; memset(nA,0,sizeof(nA)); memset(nB,0,sizeof(nB));
	unsigned char cf[0x100]; memset(cf,0,sizeof(cf));
	const unsigned aAi=0xAAAA, aBi=0xBBBB;
	memcpy(nA+0x19c,&aAi,4); memcpy(nB+0x19c,&aBi,4);
	*(void**)(cf+8)=nA; *(void**)(cf+0xc)=nB;
	unsigned o1=0,o2=0;
	cch(cf,&o1,&o2);
	if(o1!=0xAAAAu||o2!=0xBBBBu){fprintf(stderr,"cch fail %u/%u\n",o1,o2);++cgf;}
	// null branch
	*(void**)(cf+8)=0; *(void**)(cf+0xc)=0;
	o1=0x77;o2=0x77;
	cch(cf,&o1,&o2);
	if(o1!=0u||o2!=0u){fprintf(stderr,"cch null fail\n");++cgf;}
	printf("chaincopy candidate failures=%u provisional=1\n", cgf);
	}

	// -- List-index peek (003300, 0x7f0a0, ret 0): [esp+4]=list;
	//   returns the slot data selected by [list] and the slot ref count.
	{
	typedef unsigned (__cdecl* ListPeekOracle)(void*);
	ListPeekOracle lpk = reinterpret_cast<ListPeekOracle>(base + 0x7f0a0);
	unsigned lpf = 0;
	// case A: [list]=0 (index 0); slot0 = [list+4]=? ; [list+8]=0 -> slot[+4]=0
	//   -> eax=[list] -> return 0
	{
	unsigned lst[8]={0};
	if(lpk(lst)!=0u){fprintf(stderr,"lpkA fail %u\n",lpk(lst));++lpf;}
	}
	// case B: [list]=1, [list+8]=refcount=2 -> slot at list+1*4; ecx=[list+8]=2
	//   >1 -> eax=[list + 2*4 - 4]=[list+4]; return [list+4]
	{
	unsigned lst[16]={0}; lst[0]=1; lst[2]=2; lst[1]=0x1234;
	if(lpk(lst)!=0x1234u){fprintf(stderr,"lpkB fail %u\n",lpk(lst));++lpf;}
	}
	// case C: [list]=1, [list+8]=1 (refcount 1) -> cmp le -> return 0
	{
	unsigned lst[16]={0}; lst[0]=1; lst[1]=0xAAAA; lst[2]=1;
	if(lpk(lst)!=0u){fprintf(stderr,"lpkC fail %u\n",lpk(lst));++lpf;}
	}
	printf("listpeek candidate failures=%u provisional=1\n", lpf);
	}

	// -- Delimiter scan (004002, 0x90db0, ret 4) and big ctor (003257).
	{
	typedef unsigned (__cdecl* DelimScanOracle)(const char*);
	DelimScanOracle ds = reinterpret_cast<DelimScanOracle>(base + 0x90db0);
	unsigned dsf = 0;
	if(ds("hello")!=0u){fprintf(stderr,"ds0 fail\n");++dsf;}
	if(ds("a,b")!=1u){fprintf(stderr,"ds1 fail\n");++dsf;}
	if(ds("x")!=0u){fprintf(stderr,"ds2 fail\n");++dsf;}
	printf("delimscan candidate failures=%u provisional=1\n", dsf);
	}

	// -- Big ctor (003257, 0x7e370, ret 4): sets vptr, stores the arg, zeros
	//   several words, and rep-stosd zeroes 0x1000 dwords at [+0x34].
	{
	typedef void* (__thiscall* BigInitOracle)(void*, unsigned);
	BigInitOracle big = reinterpret_cast<BigInitOracle>(base + 0x7e370);
	unsigned bif = 0;
	unsigned char bb[0x1000*4 + 0x200]; memset(bb, 0xA5, sizeof(bb));
	const unsigned argVal = 0x5A5Au;
	void* r = big(bb, argVal);
	if(r != bb){fprintf(stderr,"big ret fail\n");++bif;}
	if(*(unsigned*)(bb+0)!=0x10113614u){fprintf(stderr,"big vptr fail\n");++bif;}
	if(*(unsigned*)(bb+0x4048)!=argVal){fprintf(stderr,"big arg fail\n");++bif;}
	if(*(unsigned*)(bb+0xc)+*(unsigned*)(bb+8)+*(unsigned*)(bb+0x24)+*(unsigned*)(bb+0x28)
		+*(unsigned*)(bb+0x2c)+*(unsigned*)(bb+0x30)+*(unsigned*)(bb+0x4044)+*(unsigned*)(bb+0x4038)
		+*(unsigned*)(bb+0x4034)+*(unsigned*)(bb+0x403c)+*(unsigned*)(bb+0x404c)+*(unsigned*)(bb+0x4050)!=0){fprintf(stderr,"big zero fail\n");++bif;}
	// rep stosd zeroed [+0x34, +0x34+0x4000): sample at +0x34 and +0x34+0x3ffc
	if(*(unsigned*)(bb+0x34)!=0 || *(unsigned*)(bb+0x34+0x3ffc)!=0){fprintf(stderr,"big stosd fail\n");++bif;}
	printf("biginit candidate failures=%u provisional=1\n", bif);
	}

	// -- Clean small rows: 000017 (chain-field getter), 004072 (setne),
	//   002874 (global zero + store).
	{
	typedef unsigned (__thiscall* ChainGetOracle)(void*);
	ChainGetOracle cg = reinterpret_cast<ChainGetOracle>(base + 0x1520);
	unsigned sff = 0;
	// 000017: [this+0x10] -> head; if [*head+0xd0]==5 return [*head+0xe0];
	//   else returns the head pointer (stale eax from lea).
	unsigned char chn[0x20]; memset(chn,0,sizeof(chn));
	unsigned char head[0x100]; memset(head,0,sizeof(head));
	*(void**)(chn+0x10)=head;
	unsigned d0=5; memcpy(head+0xd0,&d0,4); unsigned e0=0x1234; memcpy(head+0xe0,&e0,4);
	if(cg(chn)!=0x1234u){fprintf(stderr,"cg match fail %u\n",cg(chn));++sff;}
	*(unsigned*)(head+0xd0)=6;	// !=5 -> returns this+0x10 (stale eax from lea)
	if(cg(chn)!=(unsigned)(size_t)(chn+0x10)){fprintf(stderr,"cg miss fail %u\n",cg(chn));++sff;}
	// null head -> 0
	*(void**)(chn+0x10)=0;
	if(cg(chn)!=0u){fprintf(stderr,"cg null fail\n");++sff;}
	printf("smallclean candidate failures=%u provisional=1\n", sff);
	}

	// -- Conditional-set (005187, 0xe4160, ret 8): if [this]==arg1 set it to
	//   arg2 return 1; try [this+4],[this+8]; else 0. Returns a byte.
	{
	typedef unsigned char (__thiscall* CondSetB)(void*, unsigned, unsigned);
	CondSetB csB = reinterpret_cast<CondSetB>(base + 0xe4160);
	unsigned csf2 = 0;
	unsigned char s2b[0x40]; memset(s2b, 0, sizeof(s2b));
	*(unsigned*)(s2b+0)=0x1111u; *(unsigned*)(s2b+4)=0x2222u; *(unsigned*)(s2b+8)=0x3333u;
	if(csB(s2b,0x1111u,0xA0A0u)!=1 || *(unsigned*)(s2b+0)!=0xA0A0u){fprintf(stderr,"csB0 fail\n");++csf2;}
	if(csB(s2b,0x2222u,0xB0B0u)!=1 || *(unsigned*)(s2b+4)!=0xB0B0u){fprintf(stderr,"csB1 fail\n");++csf2;}
	if(csB(s2b,0x3333u,0xC0C0u)!=1 || *(unsigned*)(s2b+8)!=0xC0C0u){fprintf(stderr,"csB2 fail\n");++csf2;}
	if(csB(s2b,0xFFFFu,0xDDDDu)!=0){fprintf(stderr,"csB3 fail\n");++csf2;}
	printf("condsetb candidate failures=%u provisional=1\n", csf2);
	}

	// -- Six-float vector comparison (005145, 0xe2f70, ret 4): returns 1 when
	//   the source vector at [esp+4] compares "within bounds" of the reference
	//   [this], else 0.
	{
	typedef unsigned char (__thiscall* Vec6CmpOracle)(void*, const float*);
	Vec6CmpOracle v6 = reinterpret_cast<Vec6CmpOracle>(base + 0xe2f70);
	unsigned v6f = 0;
	unsigned char ref6[0x20]; memset(ref6,0,sizeof(ref6));
	float reff[6]={0,0,0,0,0,0};
	memcpy(ref6, reff, sizeof(reff));
	float srcSAME[6]={0,0,0,0,0,0};
	float srcBIG[6]={100,100,100,100,100,100};
	// Same -> all comparisons "equal"/pass; big mismatch -> some fail
	unsigned rSame=v6(ref6, srcSAME), rBig=v6(ref6, srcBIG);
	fprintf(stderr,"v6 same=%u big=%u\n",rSame,rBig);
	if(rSame!=1u){++v6f;}
	if(rBig!=0u){++v6f;}
	printf("vec6cmp candidate failures=%u provisional=1\n", v6f);
	}

	// -- vptr-ctor wrappers (001565/001571/001575) vs candidate.
	{
	typedef void* (__thiscall* CtorWrapOracle)(void*, unsigned);
	CtorWrapOracle cw565 = reinterpret_cast<CtorWrapOracle>(base + 0x2e5a0);
	CtorWrapOracle cw571 = reinterpret_cast<CtorWrapOracle>(base + 0x2e640);
	CtorWrapOracle cw575 = reinterpret_cast<CtorWrapOracle>(base + 0x2e7c0);
	unsigned cwf = 0;
	unsigned args[] = { 0, 1, 0x12345678u, 0xFFFF0000u };
	for(unsigned pass = 0; pass < 3; ++pass)
	for(unsigned ai = 0; ai < 4; ++ai)
		{
		unsigned o[0x40], c[0x40];
		for(unsigned w = 0; w < 0x40; ++w) o[w] = c[w] = 0x7b000000u + w;
		void* ro=0;
		if(pass==0) ro=cw565(o, args[ai]);
		else if(pass==1) ro=cw571(o, args[ai]);
		else ro=cw575(o, args[ai]);
		if(ro != o){fprintf(stderr,"cw ret fail pass=%u\n",pass);++cwf;}
		if(pass==0) reinterpret_cast<BoxShape*>(c)->nxCtorWrap565(args[ai]);
		else if(pass==1) reinterpret_cast<BoxShape*>(c)->nxCtorWrap571(args[ai]);
		else reinterpret_cast<BoxShape*>(c)->nxCtorWrap575(args[ai]);
		if(memcmp(o, c, 0x20) != 0)
			{
			fprintf(stderr,"ctorwrap fail pass=%u arg=%08x ai=%u\n", pass, args[ai], ai);
			for(int w = 0; w < 8; ++w) if(o[w]!=c[w])
				fprintf(stderr,"  w%u o=%08x c=%08x\n", w, o[w], c[w]);
			++cwf;
			}
		}
	printf("ctorwrap candidate failures=%u provisional=1\n", cwf);
	}

	// -- Zero-init wrapper (002065, 0x51ec0): zero [this+0..0x30].
	{
	typedef void* (__thiscall* ZeroWrapOracle)(void*);
	ZeroWrapOracle zw = reinterpret_cast<ZeroWrapOracle>(base + 0x51ec0);
	unsigned zwf = 0;
	unsigned o[0x40], c[0x40];
	for(unsigned tmp = 0; tmp < 10; ++tmp)
		{
		for(unsigned w = 0; w < 0x40; ++w) o[w] = c[w] = 0x7c000000u + w;
		zw(o);
		reinterpret_cast<BoxShape*>(c)->nxWrapZero2065();
		if(memcmp(o, c, 0x3c) != 0)
			{
			fprintf(stderr,"zerowrap fail tmp=%u\n", tmp);
			for(int w = 0; w < 0x10; ++w) if(o[w]!=c[w])
				fprintf(stderr,"  w%u o=%08x c=%08x\n", w, o[w], c[w]);
			++zwf;
			}
		}
	printf("zerowrap candidate failures=%u provisional=1\n", zwf);
	}

	// -- vptr+zero wrapper (001409, 0x29750): vptr 0x1010767c, zero +4..0x48
	//   and +0x64..0x7c.
	{
	typedef void* (__thiscall* VZOracle)(void*);
	VZOracle vz = reinterpret_cast<VZOracle>(base + 0x29750);
	unsigned vzf = 0;
	for(unsigned tmp = 0; tmp < 6; ++tmp)
		{
		unsigned o[0x40], c[0x40];
		for(unsigned w = 0; w < 0x40; ++w) o[w] = c[w] = 0x7d000000u + w;
		vz(o);
		reinterpret_cast<BoxShape*>(c)->nxWrap1409();
		if(memcmp(o, c, 0x80) != 0)
			{
			fprintf(stderr,"vz fail tmp=%u\n", tmp);
			for(int w = 0; w < 0x20; ++w) if(o[w]!=c[w])
				fprintf(stderr,"  w%u o=%08x c=%08x\n", w, o[w], c[w]);
			++vzf;
			}
		}
	printf("vzwrap candidate failures=%u provisional=1\n", vzf);
	}

	// -- Container-index wrapper (000240, 0xb7c0).
	{
	typedef unsigned (__thiscall* Wrap240Oracle)(void*, unsigned);
	Wrap240Oracle w240 = reinterpret_cast<Wrap240Oracle>(base + 0xb7c0);
	unsigned wf = 0;
	// container: [+8]=buf ptr, [+0xc]=byte size; slots point to objects with
	// +0x6cc words.
	unsigned char ctr[0x20]; memset(ctr,0,sizeof(ctr));
	unsigned char slots[3][0x6e0];
	for(int s=0;s<3;++s) memset(slots[s],0,sizeof(slots[s]));
	*(unsigned*)(slots[0]+0x6cc)=0xA1111111u;
	*(unsigned*)(slots[1]+0x6cc)=0xB2222222u;
	unsigned char* buf[3]; buf[0]=slots[0]; buf[1]=slots[1];
	unsigned bufp=(unsigned)(size_t)buf; memcpy(ctr+8,&bufp,4);
	unsigned endp=bufp+2u*4u; memcpy(ctr+0xc,&endp,4);	// end = start + size
	unsigned char own[0x20]; memset(own,0,sizeof(own));
	*(void**)(own+4)=ctr;
	// index 0 -> slots[0]+0x6cc = 0xA1111111; index 1 -> 0xB2222222
	unsigned r0 = w240(own, 0u), e0 = *(unsigned*)(slots[0]+0x6cc);
	unsigned r1 = w240(own, 1u), e1 = *(unsigned*)(slots[1]+0x6cc);
	if(r0!=e0 || r1!=e1)
		{fprintf(stderr,"w240 mismatch\n");++wf;}
	unsigned rc = reinterpret_cast<BoxShape*>(own)->nxWrap240(0u);
	if(rc != e0){fprintf(stderr,"w240 cand mismatch\n");++wf;}
	printf("wrap240 candidate failures=%u provisional=1\n", wf);
	}

	// -- Small flag/link-advance rows: 002170 (setne +9c), 004083 (setne
	//   global), 001957 (sum fields), 000567/000569 (link advance 6bc/6c0).
	{
	typedef unsigned char (__thiscall* SetNe9cOracle)(void*);
	SetNe9cOracle sn9 = reinterpret_cast<SetNe9cOracle>(base + 0x53e10);
	typedef unsigned char (__cdecl* SetNeGlbOracle)();
	SetNeGlbOracle sng = reinterpret_cast<SetNeGlbOracle>(base + 0x95ca0);
	typedef unsigned (__thiscall* SumFieldsOracle)(void*);
	SumFieldsOracle sfm = reinterpret_cast<SumFieldsOracle>(base + 0x4be80);
	typedef unsigned (__thiscall* LinkAdvOracle)(void*);
	LinkAdvOracle la6bc = reinterpret_cast<LinkAdvOracle>(base + 0x108a0);
	LinkAdvOracle la6c0 = reinterpret_cast<LinkAdvOracle>(base + 0x108c0);
	unsigned sf = 0;
	unsigned char a1[0xc0]; memset(a1,0,sizeof(a1));
	if(sn9(a1)!=0){fprintf(stderr,"sn9 zero fail\n");++sf;}
	unsigned v9c=7; memcpy(a1+0x9c,&v9c,4);
	if(sn9(a1)!=1){fprintf(stderr,"sn9 set fail\n");++sf;}
	// 004083: global [0x10127180]=0 at load; set it and check.
	unsigned* g27180 = reinterpret_cast<unsigned*>(const_cast<unsigned char*>(base+0x127180));
	unsigned save27180=*g27180;
	unsigned z0=0; *g27180=0;
	unsigned char s0=sng();
	unsigned vv=5; *g27180=5;
	unsigned char s1=sng();
	*g27180=save27180;
	if(s0!=0 || s1!=1){fprintf(stderr,"sng fail %u/%u\n",s0,s1);++sf;}
	// 001957: [this+0x1c]=obj -> [obj+0xc]+[obj+8]
	unsigned char t7[0x20]; memset(t7,0,sizeof(t7));
	unsigned char obj7[0x20]; memset(obj7,0,sizeof(obj7));
	unsigned f8=0x33, fc=0x44; memcpy(obj7+8,&f8,4); memcpy(obj7+0xc,&fc,4);
	*(void**)(t7+0x1c)=obj7;
	if(sfm(t7)!=0x77u){fprintf(stderr,"sfm fail %u\n",sfm(t7));++sf;}
	memset(t7,0,sizeof(t7));
	if(sfm(t7)!=0u){fprintf(stderr,"sfm null fail\n");++sf;}
	// 000567/569: [this+6bc]->obj; [this+6bc]=[obj+0x10]; returns the same? check
	unsigned char c1[0x700]; memset(c1,0,sizeof(c1));
	unsigned char o1[0x20]; memset(o1,0,sizeof(o1));
	unsigned o10=0x89; memcpy(o1+0x10,&o10,4);
	*(void**)(c1+0x6bc)=o1;
	la6bc(c1);
	if(*(unsigned*)(c1+0x6bc)!=0x89u){fprintf(stderr,"la6bc fail\n");++sf;}
	memset(c1+0x6bc,0,4);
	la6bc(c1);
	if(*(unsigned*)(c1+0x6bc)!=0u){fprintf(stderr,"la6bc null fail\n");++sf;}
	printf("smallflag candidate failures=%u provisional=1\n", sf);
	}

	// -- More flag/getter rows: 000738 (flag->offset ptr), 004942 (2-bit
	//   select), 003628 (cond store).
	{
	typedef unsigned (__thiscall* FlagOffOracle)(void*);
	FlagOffOracle fo = reinterpret_cast<FlagOffOracle>(base + 0x16c00);
	typedef unsigned (__thiscall* TwoBitSelOracle)(void*);
	TwoBitSelOracle ts = reinterpret_cast<TwoBitSelOracle>(base + 0xbb570);
	typedef void (__thiscall* CondStoreOracle)(void*, unsigned, unsigned);
	CondStoreOracle cso = reinterpret_cast<CondStoreOracle>(base + 0x89bb0);
	unsigned sf2 = 0;
	// 000738: [this+0x1e4]&0x200 ? this+0x244 : 0
	unsigned char f2[0x280]; memset(f2,0,sizeof(f2));
	unsigned fl=0x200; memcpy(f2+0x1e4,&fl,4);
	if(fo(f2)!=(unsigned)(size_t)(f2+0x244)){fprintf(stderr,"fo set fail\n");++sf2;}
	*(unsigned*)(f2+0x1e4)=0;
	if(fo(f2)!=0u){fprintf(stderr,"fo clr fail\n");++sf2;}
	// 004942: bit1(2) set and bit0(1) clear -> 0x1011b6ec else 0
	unsigned char t2[0x40]; memset(t2,0,sizeof(t2));
	unsigned b=6u; memcpy(t2+4,&b,4);	// 6 = 110 -> bit1 set, bit0 CLEAR -> pointer
	if(ts(t2)!=0x1011b6ecu){fprintf(stderr,"ts b1 fail\n");++sf2;}
	unsigned b3=3u; memcpy(t2+4,&b3,4);	// 3 -> bit0 set -> 0
	if(ts(t2)!=0u){fprintf(stderr,"ts b3 fail\n");++sf2;}
	unsigned b2=0u; memcpy(t2+4,&b2,4);	// 0 -> bit1 clear -> 0
	if(ts(t2)!=0u){fprintf(stderr,"ts b2 fail\n");++sf2;}
	// 003628: if [this+0x2b]!=0 and [esp+8]==0 then [this+0x28]=1
	unsigned char c3[0x40]; memset(c3,0,sizeof(c3));
	c3[0x2b]=5;
	cso(c3, 0u, 0u);
	if(c3[0x28]!=1){fprintf(stderr,"cso set fail %02x\n",c3[0x28]);++sf2;}
	c3[0x28]=0;
	cso(c3, 0u, 1u);	// [esp+8]=1 -> no store
	if(c3[0x28]!=0){fprintf(stderr,"cso arg1 fail %02x\n",c3[0x28]);++sf2;}
	printf("smallflag2 candidate failures=%u provisional=1\n", sf2);
	}

	// -- rep-movsd structured copies (001299/003425/003427/003567) and the
	//   x87 control-word read (000537).
	{
	typedef void (__thiscall* RepCopyOracle)(void*, unsigned*);
	RepCopyOracle rc_6c = reinterpret_cast<RepCopyOracle>(base + 0x258a0);
	RepCopyOracle rc_28 = reinterpret_cast<RepCopyOracle>(base + 0x84d10);
	RepCopyOracle rc_18 = reinterpret_cast<RepCopyOracle>(base + 0x87e70);
	RepCopyOracle rc_in = reinterpret_cast<RepCopyOracle>(base + 0x84d30);
	unsigned spf = 0;
	unsigned char sc[0x100]; memset(sc, 0, sizeof(sc));
	for(unsigned w = 0; w < 12; ++w) *(unsigned*)(sc + 0x6c + w*4) = 0x11000000u + w;
	for(unsigned w = 0; w < 12; ++w) *(unsigned*)(sc + 0x28 + w*4) = 0x13000000u + w;
	for(unsigned w = 0; w < 12; ++w) *(unsigned*)(sc + 0x18 + w*4) = 0x14000000u + w;
	unsigned out6c[12]; memset(out6c,0,sizeof(out6c));
	rc_6c(sc, out6c);
	for(unsigned i = 0; i < 9; ++i)
		if(out6c[i] != 0x11000000u + i){fprintf(stderr,"rc_6c fail %u\n",i);++spf;}
	// 003425: 11 dwords from [this+0x28]
	unsigned out28[12]; memset(out28,0,sizeof(out28));
	rc_28(sc, out28);
	for(unsigned i = 0; i < 11; ++i)
		if(out28[i] != *(unsigned*)(sc + 0x28 + i*4)){fprintf(stderr,"rc_28 fail %u\n",i);++spf;}
	// 003567: 9 dwords from [this+0x18]
	unsigned out18[12]; memset(out18,0,sizeof(out18));
	rc_18(sc, out18);
	for(unsigned i = 0; i < 9; ++i)
		if(out18[i] != *(unsigned*)(sc + 0x18 + i*4)){fprintf(stderr,"rc_18 fail %u\n",i);++spf;}
	// 003427: 11 dwords from [esp+8] (arg) into [this+0x28]
	unsigned in11[12]; for(unsigned i=0;i<12;++i) in11[i]=0x22000000u+i;
	unsigned char sc27[0x100]; memset(sc27,0,sizeof(sc27));
	rc_in(sc27, in11);
	for(unsigned i = 0; i < 11; ++i)
		if(*(unsigned*)(sc27 + 0x28 + i*4) != 0x22000000u + i){fprintf(stderr,"rc_in fail %u\n",i);++spf;}
	printf("repcopy candidate failures=%u provisional=1\n", spf);
	}

	// -- 002176 (indirect field) and 003479 (interval flag).
	{
	typedef unsigned (__thiscall* IndFieldOracle)(void*);
	IndFieldOracle ind = reinterpret_cast<IndFieldOracle>(base + 0x53f20);
	typedef unsigned (__thiscall* Iv79Oracle)(void*);
	Iv79Oracle iv79 = reinterpret_cast<Iv79Oracle>(base + 0x85590);
	unsigned sf4 = 0;
	unsigned char v1[0xa0]; memset(v1,0,sizeof(v1));
	unsigned char o1[0x60]; memset(o1,0,sizeof(o1));
	unsigned f5c=0x99; memcpy(o1+0x5c,&f5c,4);
	*(void**)(v1+0x9c)=o1;
	if(ind(v1)!=0x99u){fprintf(stderr,"ind set fail\n");++sf4;}
	*(void**)(v1+0x9c)=0;
	if(ind(v1)!=0u){fprintf(stderr,"ind null fail\n");++sf4;}
	unsigned char v2[0x20]; memset(v2,0,sizeof(v2));
	unsigned lo=0x100, hi=0x100; memcpy(v2+0x14,&lo,4); memcpy(v2+0x18,&hi,4);
	if(iv79(v2)!=0u){fprintf(stderr,"iv79 zero fail\n");++sf4;}
	hi=0x120; memcpy(v2+0x18,&hi,4);
	if(iv79(v2)!=lo){fprintf(stderr,"iv79 nz fail %u\n",iv79(v2));++sf4;}
	printf("indfield candidate failures=%u provisional=1\n", sf4);
	}

	// -- x87 control-word read (000537, 0x106d0): writes the x87 status word
	//   (fnstcw) into [this+0]. Confirmed to run and produce a control word.
	{
	typedef void (__thiscall* CwReadOracle)(void*);
	CwReadOracle cwr = reinterpret_cast<CwReadOracle>(base + 0x106d0);
	unsigned cwf = 0;
	unsigned short cw1 = 0, cw2 = 0;
	unsigned char buf1[0x20], buf2[0x20];
	memset(buf1,0xFF,sizeof(buf1)); memset(buf2,0xEE,sizeof(buf2));
	cwr(buf1); cwr(buf2);
	memcpy(&cw1, buf1, 2); memcpy(&cw2, buf2, 2);
	fprintf(stderr,"cw read=0x%04x/0x%04x\n",cw1,cw2);
	// x87 control words typically carry the exception flags (0x3f of the low
	// byte plus the stack/wait bits); require the low 6 bits to be the
	// standard sticky exception flags or a plausible value, and that neither
	// is untouched 0xff/0xee sentinel.
	if(cw1 == 0xffff || cw2 == 0xeeee || (cw1 & 0x3f) != (cw2 & 0x3f))
		{fprintf(stderr,"cw inconsistent\n");++cwf;}
	printf("cwread candidate failures=%u provisional=1\n", cwf);
	}

	// -- Indexed-sum probe 001960 (0x4bec0): [this+1c + [this+0x70]*4] -> sum.
	{
	typedef unsigned (__thiscall* IxSum2Oracle)(void*);
	IxSum2Oracle ix2 = reinterpret_cast<IxSum2Oracle>(base + 0x4bec0);
	unsigned char sb[0x100]; memset(sb,0,sizeof(sb));
	unsigned char so[0x20]; memset(so,0,sizeof(so));
	unsigned f8=0x11, fc=0x22; memcpy(so+8,&f8,4); memcpy(so+0xc,&fc,4);
	unsigned zi=0; memcpy(sb+0x70,&zi,4);
	*(void**)(sb+0x1c)=so;
	unsigned r0 = ix2(sb);
	fprintf(stderr,"ix2 r=%u\n",r0);
	unsigned fail=0;
	if(r0 != 0x33u)++fail;
	memset(sb+0x1c,0,4);
	if(ix2(sb) != 0)++fail;
	printf("ixsum2 candidate failures=%u provisional=1\n", fail);
	}

	// -- Element-count row 003477 (0x85580): ([this+8]-[this+4]) >> 2.
	{
	typedef int (__thiscall* ElemCountOracle)(void*);
	ElemCountOracle ec = reinterpret_cast<ElemCountOracle>(base + 0x85580);
	unsigned ef = 0;
	unsigned char eb[0x20]; memset(eb,0,sizeof(eb));
	unsigned lo=0x1000, hi=0x1000+12; memcpy(eb+4,&lo,4); memcpy(eb+8,&hi,4);
	if(ec(eb)!=3){fprintf(stderr,"ec3 fail %d\n",ec(eb));++ef;}
	hi=0x1000; memcpy(eb+8,&hi,4);
	if(ec(eb)!=0){fprintf(stderr,"ec0 fail\n");++ef;}
	lo=0x1000; hi=0x1000-8; memcpy(eb+4,&lo,4); memcpy(eb+8,&hi,4);
	if(ec(eb)!=-2){fprintf(stderr,"ecneg fail %d\n",ec(eb));++ef;}
	printf("elemcount candidate failures=%u provisional=1\n", ef);
	}

	// -- MESH vtable slot 4 transform-point (001403, 0x29190, ret 4).
	{
	typedef void (__thiscall* TransformPtOracle)(void*, float*);
	TransformPtOracle tp = reinterpret_cast<TransformPtOracle>(base + 0x29190);
	unsigned tpf = 0;
	unsigned char body[0x60]; memset(body, 0, sizeof(body));
	unsigned char own[0x100]; memset(own, 0, sizeof(own));
	float vec[4] = { 1.0f, 2.0f, 3.0f, 7.0f };
	memcpy(body + 0x5c, vec, sizeof(vec));
	*(void**)(own + 0xe0) = body;
	// matrix rows at +0xc..0x2c, translation at +0x30..0x38
	float m[9] = { 1,2,3, 4,5,6, 7,8,9 };
	float t[3] = { 10, 20, 30 };
	memcpy(own + 0xc, m, sizeof(m));
	memcpy(own + 0x30, t, sizeof(t));
	float outO[4], outC[4];
	memset(outO, 0, sizeof(outO)); memset(outC, 0, sizeof(outC));
	tp(own, outO);
	reinterpret_cast<BoxShape*>(own)->nxTransformPoint1403(outC);
	if(memcmp(outO, outC, sizeof(outO)) != 0)
		{
		fprintf(stderr,"tp1403 fail: o=%.3f,%.3f,%.3f,%.3f c=%.3f,%.3f,%.3f,%.3f\n",
			(double)outO[0],(double)outO[1],(double)outO[2],(double)outO[3],
			(double)outC[0],(double)outC[1],(double)outC[2],(double)outC[3]);
		++tpf;
		}
	printf("transpt1403 candidate failures=%u provisional=1\n", tpf);
	}

	// -- SPHERE vtable slot 4 mass wrapper (001371, 0x27be0, ret 0xc).
	{
	typedef bool (__thiscall* SphMassOracle)(void*, void*, float, unsigned);
	SphMassOracle sm = reinterpret_cast<SphMassOracle>(base + 0x27be0);
	unsigned smf = 0;
	for(unsigned low = 0; low < 8; ++low)
	for(unsigned dc = 0; dc < 2; ++dc)
		{
		unsigned char sb[0x100]; memset(sb, 0, sizeof(sb));
		unsigned short flags = static_cast<unsigned short>(8 | low);
		memcpy(sb + 0xde, &flags, 2);
		float radius = 2.5f; memcpy(sb + 0xe0, &radius, 4);
		unsigned char pose[0x24]; memset(pose, 0, sizeof(pose));
		memcpy(sb + 0x6c, pose, sizeof(pose));
		const float density = dc ? 2.0f : 1.0f;
		MassFrame oF, cF;
		memset(&oF, 0, sizeof(oF)); memset(&cF, 0, sizeof(cF));
		bool oR = sm(sb, &oF, density, 0xdeadbeefu);
		bool cR = reinterpret_cast<SphereShape*>(sb)->nxSphereAccumulateMass(
			&cF, density, 0xdeadbeefu);
		if(!oR || !cR || memcmp(&oF, &cF, sizeof(oF)) != 0)
			{
			fprintf(stderr,"sphmass low=%u dc=%u oR=%u cR=%u\n", low, dc, oR?1u:0u, cR?1u:0u);
			unsigned ow[13], cw[13];
			memcpy(ow,&oF,sizeof(ow)); memcpy(cw,&cF,sizeof(cw));
			for(unsigned i=0;i<13;++i) if(ow[i]!=cw[i])
				fprintf(stderr,"  m[%u] o=%08x c=%08x\n", i, ow[i], cw[i]);
			++smf;
			}
		}
	printf("sphmass candidate failures=%u provisional=1\n", smf);
	}

	// -- CAPSULE vtable slot 4 mass wrapper (001008, 0x22440, ret 0xc).
	{
	typedef bool (__thiscall* CapMassOracle)(void*, void*, float, unsigned);
	CapMassOracle cm = reinterpret_cast<CapMassOracle>(base + 0x22440);
	unsigned cmf = 0;
	for(unsigned low = 0; low < 8; ++low)
	for(unsigned dc = 0; dc < 2; ++dc)
		{
		unsigned char sb[0x100]; memset(sb, 0, sizeof(sb));
		unsigned short flags = static_cast<unsigned short>(8 | low);
		memcpy(sb + 0xde, &flags, 2);
		float r0 = 1.5f, r4 = 0.5f;
		memcpy(sb + 0xe0, &r0, 4); memcpy(sb + 0xe4, &r4, 4);
		unsigned char pose[0x24]; memset(pose, 0, sizeof(pose));
		memcpy(sb + 0x6c, pose, sizeof(pose));
		const float density = dc ? 2.0f : 1.0f;
		MassFrame oF, cF;
		memset(&oF, 0, sizeof(oF)); memset(&cF, 0, sizeof(cF));
		bool oR = cm(sb, &oF, density, 0xdeadbeefu);
		bool cR = reinterpret_cast<CapsuleShape*>(sb)->nxCapsuleAccumulateMass(
			&cF, density, 0xdeadbeefu);
		if(!oR || !cR || memcmp(&oF, &cF, sizeof(oF)) != 0)
			{
			fprintf(stderr,"capmass2 low=%u dc=%u oR=%u cR=%u\n", low, dc, oR?1u:0u, cR?1u:0u);
			unsigned ow[13], cw[13];
			memcpy(ow,&oF,sizeof(ow)); memcpy(cw,&cF,sizeof(cw));
			for(unsigned i=0;i<13;++i) if(ow[i]!=cw[i])
				fprintf(stderr,"  m[%u] o=%08x c=%08x\n", i, ow[i], cw[i]);
			++cmf;
			}
		}
	printf("capmass2 candidate failures=%u provisional=1\n", cmf);
	}

	// -- PLANE vtable slot 9 local AABB (001255, 0x25090, ret 4).
	{
	typedef void (__thiscall* PlaneAabbOracle)(void*, float*);
	PlaneAabbOracle pa = reinterpret_cast<PlaneAabbOracle>(base + 0x25090);
	unsigned paf = 0;
	const float normals[7][3] = {
		{1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}, {0.5f,0.5f,0.5f} };
	for(unsigned ni = 0; ni < 7; ++ni)
	for(unsigned di = 0; di < 2; ++di)
		{
		unsigned char pb[0x120]; memset(pb, 0, sizeof(pb));
		float d = di ? -3.5f : 2.25f;
		memcpy(pb + 0xe0, normals[ni], 12);
		memcpy(pb + 0xec, &d, 4);
		float outO[6], outC[6];
		memset(outO, 0, sizeof(outO)); memset(outC, 0, sizeof(outC));
		pa(pb, outO);
		reinterpret_cast<PlaneShape*>(pb)->nxPlaneLocalAABB1255(outC);
		if(memcmp(outO, outC, sizeof(outO)) != 0)
			{
			fprintf(stderr,"planeaabb n=%u d=%u\n", ni, di);
			for(unsigned i=0;i<6;++i) if(outO[i]!=outC[i])
				fprintf(stderr,"  o[%u] o=%08x c=%08x\n", i,
					*(unsigned*)(outO+i), *(unsigned*)(outC+i));
			++paf;
			}
		}
	printf("planeaabb candidate failures=%u provisional=1\n", paf);
	}

	// -- Actor vtable thunks 000038 (slot 0x104) and 000040 (slot 0x108).
	{
	typedef void* (__thiscall* ActorThunkOracle)(void*, void*, unsigned*);
	ActorThunkOracle t104 = reinterpret_cast<ActorThunkOracle>(base + 0x2400);
	ActorThunkOracle t108 = reinterpret_cast<ActorThunkOracle>(base + 0x2430);
	unsigned atf = 0;
	for(unsigned si = 0; si < 2; ++si)
		{
		// stub record + stub slot body
		unsigned rec[3] = { 0x11111111u + si, 0x22222222u + si, 0x33333333u + si };
		void* vt[0x50 / 4 + 1];
		memset(vt, 0, sizeof(vt));
		// a __thiscall stub returning rec: implemented via a lambda-free fn ptr
		struct Stub { static void* __thiscall call(void*, void*, void* r) { return r; } };
		// pass rec through arg1 so the stub can return it
		vt[(si ? 0x108 : 0x104) / 4] = reinterpret_cast<void*>(&Stub::call);
		unsigned char ob[0x20]; memset(ob, 0, sizeof(ob));
		*(void**)(ob) = vt;
		unsigned outO[4] = {0,0,0,0}, outC[4] = {0,0,0,0};
		if(si == 0) { t104(ob, rec, outO); nxActorVtThunk104(ob, rec, outC); }
		else        { t108(ob, rec, outO); nxActorVtThunk108(ob, rec, outC); }
		if(memcmp(outO, outC, 12) != 0)
			{
			fprintf(stderr,"actorthunk si=%u o=%08x,%08x,%08x c=%08x,%08x,%08x\n", si,
				outO[0],outO[1],outO[2],outC[0],outC[1],outC[2]);
			++atf;
			}
		}
	printf("actorthunk candidate failures=%u provisional=1\n", atf);
	}

	// -- PLANE vtable slot 8 indexed-record copy (001267, 0x25490, ret 4).
	{
	typedef void (__thiscall* PlaneIx6Oracle)(void*, unsigned*);
	PlaneIx6Oracle pi = reinterpret_cast<PlaneIx6Oracle>(base + 0x25490);
	unsigned pif = 0;
	// inner table: [this+0xc4] -> inner; inner+0x14 -> base array of 24-byte
	// records; index from [this+0xa4+0x28].
	unsigned char recs[3 * 24];
	for(unsigned i = 0; i < 3 * 24; i += 4) *(unsigned*)(recs + i) = 0x55000000u + i;
	unsigned char inner[0x20]; memset(inner, 0, sizeof(inner));
	*(void**)(inner + 0x14) = recs;
	unsigned char pb[0x100]; memset(pb, 0, sizeof(pb));
	*(void**)(pb + 0xc4) = inner;
	// skip the 004886 arm: set bit 2 at [0xa4+8]
	pb[0xa4 + 8] = 2;
	for(unsigned idx = 0; idx < 3; ++idx)
		{
		unsigned short w = static_cast<unsigned short>(idx);
		memcpy(pb + 0xa4 + 0x28, &w, 2);
		unsigned outO[8], outC[8];
		memset(outO, 0, sizeof(outO)); memset(outC, 0, sizeof(outC));
		pi(pb, outO);
		reinterpret_cast<PlaneShape*>(pb)->nxPlaneIndexed6_1267(outC);
		if(memcmp(outO, outC, 24) != 0)
			{
			fprintf(stderr,"planeix6 idx=%u\n", idx);
			for(unsigned i=0;i<6;++i) if(outO[i]!=outC[i])
				fprintf(stderr,"  o[%u] o=%08x c=%08x\n", i, outO[i], outC[i]);
			++pif;
			}
		}
	printf("planeix6 candidate failures=%u provisional=1\n", pif);
	}

	// -- CAPSULE vtable slot 9 world AABB (001016, 0x22620, ret 4).
	{
	typedef void (__thiscall* CapAabbOracle)(void*, float*);
	CapAabbOracle ca = reinterpret_cast<CapAabbOracle>(base + 0x22620);
	unsigned caf = 0;
	const float poses[4][3] = { {1,0,0}, {0,1,0}, {0,0,1}, {0.5f,-0.25f,0.75f} };
	for(unsigned pi2 = 0; pi2 < 4; ++pi2)
	for(unsigned vi = 0; vi < 2; ++vi)
		{
		unsigned char pb[0x120]; memset(pb, 0, sizeof(pb));
		float hh = vi ? 1.25f : 2.0f, r = 0.5f;
		float t[3] = { 1.0f, -2.0f, 0.75f };
		// the row reads M[0][1] at +0x10, M[1][1] at +0x1c, M[2][1] at +0x28
		memcpy(pb + 0x10, &poses[pi2][0], 4);
		memcpy(pb + 0x1c, &poses[pi2][1], 4);
		memcpy(pb + 0x28, &poses[pi2][2], 4);
		memcpy(pb + 0xe0, &r, 4);
		memcpy(pb + 0xe4, &hh, 4);
		memcpy(pb + 0x30, &t[0], 4);
		memcpy(pb + 0x34, &t[1], 4);
		memcpy(pb + 0x38, &t[2], 4);
		float outO[6], outC[6];
		// seed both with the same existing AABB so the merge is exercised
		for(unsigned i = 0; i < 6; ++i) { outO[i] = outC[i] = (i < 3) ? -10.0f : 10.0f; }
		ca(pb, outO);
		reinterpret_cast<CapsuleShape*>(pb)->nxCapsuleWorldAABB1016(outC);
		if(memcmp(outO, outC, sizeof(outO)) != 0)
			{
			fprintf(stderr,"capaabb p=%u v=%u\n", pi2, vi);
			for(unsigned i=0;i<6;++i) if(outO[i]!=outC[i])
				fprintf(stderr,"  o[%u] o=%08x c=%08x\n", i,
					*(unsigned*)(outO+i), *(unsigned*)(outC+i));
			++caf;
			}
		}
	printf("capaabb candidate failures=%u provisional=1\n", caf);
	}

	// -- Global-region zero 003390 (0x83a60): zeroes the 0xdd bytes at
	//   .data[0x1012626b .. +0xdc]. Read-back verified.
	{
	typedef void (__cdecl* GlobZeroRegion2Oracle)();
	GlobZeroRegion2Oracle gz2 = reinterpret_cast<GlobZeroRegion2Oracle>(base + 0x83a60);
	unsigned gzf = 0;
	unsigned char* gbase = const_cast<unsigned char*>(base) + 0x12626b;
	for(unsigned i = 0; i < 0xdd; ++i) gbase[i] = 0xA5;
	gz2();
	unsigned okz = 1;
	for(unsigned i = 0; i < 0xdd; ++i) if(gbase[i] != 0) okz = 0;
	if(!okz){fprintf(stderr,"gz2 fail\n");++gzf;}
	printf("globzero2 candidate failures=%u provisional=1\n", gzf);
	}

	// -- Pose copy with tail 000827 (0x1bcc0, ret 0xc).
	{
	typedef void (__thiscall* PoseCopy827Oracle)(void*, const void*, const unsigned*, unsigned);
	PoseCopy827Oracle pc827 = reinterpret_cast<PoseCopy827Oracle>(base + 0x1bcc0);
	unsigned pcf = 0;
	for(unsigned ci = 0; ci < 3; ++ci)
		{
		unsigned char ob[0x40], cb[0x40];
		for(unsigned w = 0; w < 0x40; ++w) ob[w] = cb[w] = static_cast<unsigned char>(0x90u + w);
		unsigned src[9];
		for(unsigned i = 0; i < 9; ++i) src[i] = 0x66000000u + i + ci;
		unsigned extra[3] = { 0x77000001u + ci, 0x77000002u + ci, 0x77000003u + ci };
		unsigned xv = 0x88000000u + ci;
		pc827(ob, src, extra, xv);
		reinterpret_cast<BoxShape*>(cb)->nxPoseCopyWithTail0827(src, extra, xv);
		if(memcmp(ob, cb, 0x34) != 0)
			{
			fprintf(stderr,"posecopy827 ci=%u\n", ci);
			for(unsigned i = 0; i < 0x34; i += 4) if(*(unsigned*)(ob+i)!=*(unsigned*)(cb+i))
				fprintf(stderr,"  w%02x o=%08x c=%08x\n", i,
					*(unsigned*)(ob+i), *(unsigned*)(cb+i));
			++pcf;
			}
		}
	printf("posecopy827 candidate failures=%u provisional=1\n", pcf);
	}

	// -- Batch index/vertex append 003268 (0x7e560, ret 8).
	{
	typedef void (__thiscall* BatchAppendOracle)(void*, unsigned, const unsigned*);
	BatchAppendOracle ba = reinterpret_cast<BatchAppendOracle>(base + 0x7e560);
	unsigned baf = 0;
	for(unsigned ci = 0; ci < 3; ++ci)
		{
		unsigned verts[9];  for(unsigned i=0;i<9;++i)  verts[i]  = 0x31000000u + i;
		unsigned map[4];    for(unsigned i=0;i<4;++i)  map[i]    = (ci==1 && i==1) ? 2u : 0u;
		unsigned outArr[12]; memset(outArr, 0, sizeof(outArr));
		unsigned aux[8];     memset(aux, 0, sizeof(aux));
		unsigned mapC[4], outC[12], auxC[8];
		memcpy(mapC, map, sizeof(map));
		memset(outC, 0, sizeof(outC)); memset(auxC, 0, sizeof(auxC));
		unsigned idxs[3] = { 1u, 2u, 1u };
		unsigned char ob[0x5000], cb[0x5000];
		memset(ob, 0, sizeof(ob)); memset(cb, 0, sizeof(cb));
		unsigned* pO = reinterpret_cast<unsigned*>(ob);
		unsigned* pC = reinterpret_cast<unsigned*>(cb);
		// +0x18 count=0, +0x1c cap=2, +0x10 vertCount=4, +0x4040 auxCap=8
		*(unsigned*)(ob+0x1c)=2u; *(unsigned*)(cb+0x1c)=2u;
		*(unsigned*)(ob+0x10)=4u; *(unsigned*)(cb+0x10)=4u;
		*(unsigned*)(ob+0x4040)=8u; *(unsigned*)(cb+0x4040)=8u;
		*(void**)(ob+8)=map;   *(void**)(cb+8)=mapC;
		*(void**)(ob+0xc)=verts; *(void**)(cb+0xc)=verts;
		*(void**)(ob+0x4038)=outArr; *(void**)(cb+0x4038)=outC;
		*(void**)(ob+0x4044)=aux; *(void**)(cb+0x4044)=auxC;
		(void)pO; (void)pC;
		ba(ob, 3u, idxs);
		nxBatchAppend3268(cb, 3u, idxs);
		// The fixture stores different array addresses in the four pointer
		// slots (+8/+0xc/+0x4038/+0x4044); compare everything else, plus the
		// three data arrays themselves.
		bool ptrOnly = true;
		for(unsigned i = 0; i < 0x4050; i += 4)
			{
			if(i == 8 || i == 0xc || i == 0x4038 || i == 0x4044) continue;
			if(*(unsigned*)(ob+i) != *(unsigned*)(cb+i)) ptrOnly = false;
			}
		if(!ptrOnly || memcmp(outArr, outC, sizeof(outArr)) != 0
			|| memcmp(aux, auxC, sizeof(aux)) != 0 || memcmp(map, mapC, sizeof(map)) != 0)
			{
			fprintf(stderr,"batch3268 ci=%u\n", ci);
			for(unsigned i = 0; i < 0x4050; i += 4)
				if(i != 8 && i != 0xc && i != 0x4038 && i != 0x4044
					&& *(unsigned*)(ob+i)!=*(unsigned*)(cb+i))
					fprintf(stderr,"  this+%04x o=%08x c=%08x\n", i,
						*(unsigned*)(ob+i), *(unsigned*)(cb+i));
			for(unsigned i = 0; i < 12; ++i) if(outArr[i]!=outC[i])
				fprintf(stderr,"  out[%u] o=%08x c=%08x\n", i, outArr[i], outC[i]);
			for(unsigned i = 0; i < 8; ++i) if(aux[i]!=auxC[i])
				fprintf(stderr,"  aux[%u] o=%08x c=%08x\n", i, aux[i], auxC[i]);
			for(unsigned i = 0; i < 4; ++i) if(map[i]!=mapC[i])
				fprintf(stderr,"  map[%u] o=%08x c=%08x\n", i, map[i], mapC[i]);
			++baf;
			}
		}
	printf("batch3268 candidate failures=%u provisional=1\n", baf);
	}

	// -- AABB aggregation over a shape list 001030 (0x22bf0, ret 4).
	{
	typedef void (__thiscall* AggAabbOracle)(void*, float*);
	AggAabbOracle ag = reinterpret_cast<AggAabbOracle>(base + 0x22bf0);
	unsigned agf = 0;
	// two plane-like shapes whose slot-8 record selection yields known floats
	unsigned char recs[2][3 * 24];
	for(unsigned s = 0; s < 2; ++s)
		for(unsigned i = 0; i < 3 * 24; i += 4)
			*(unsigned*)(recs[s] + i) = 0x40000000u * (s + 1) + i;
	unsigned char inner[2][0x20];
	unsigned char shapes[2][0xc8];
	unsigned char list[2];
	memset(inner, 0, sizeof(inner)); memset(shapes, 0, sizeof(shapes));
	for(unsigned s = 0; s < 2; ++s)
		{
		*(void**)(inner[s] + 0x14) = recs[s];
		*(void**)(shapes[s] + 0xc4) = inner[s];
		*(unsigned short*)(shapes[s] + 0xa4 + 0x28) = 1;	// index 1
		shapes[s][0xa4 + 8] = 2;							// skip 004886
		}
	unsigned* lp = reinterpret_cast<unsigned*>(list);
	(void)lp;
	unsigned char ob[0x80], cb[0x80];
	memset(ob, 0, sizeof(ob)); memset(cb, 0, sizeof(cb));
	unsigned char sh1[0xc8], sh2[0xc8];
	memcpy(sh1, shapes[0], 0xc8); memcpy(sh2, shapes[1], 0xc8);
	void* lstO[2] = { sh1, sh2 };
	void* lstC[2] = { sh1, sh2 };
	*(void**)(ob + 0xe0) = lstO; *(void**)(ob + 0xe4) = lstO + 2;
	*(void**)(cb + 0xe0) = lstC; *(void**)(cb + 0xe4) = lstC + 2;
	float outO[6], outC[6];
	ag(ob, outO);
	nxAggregateAABB1030(cb, outC);
	if(memcmp(outO, outC, sizeof(outO)) != 0)
		{
		fprintf(stderr,"aggaabb1030 mismatch\n");
		for(unsigned i=0;i<6;++i) if(outO[i]!=outC[i])
			fprintf(stderr,"  o[%u] o=%08x c=%08x\n", i,
				*(unsigned*)(outO+i), *(unsigned*)(outC+i));
		++agf;
		}
	printf("aggaabb1030 candidate failures=%u provisional=1\n", agf);
	}

	// -- Nine doubles to nine floats 002156 (0x538e0, ret 4).
	{
	typedef void (__thiscall* D2F9Oracle)(void*, float*);
	D2F9Oracle d2f = reinterpret_cast<D2F9Oracle>(base + 0x538e0);
	unsigned d2fF = 0;
	for(unsigned ci = 0; ci < 3; ++ci)
		{
		unsigned char sb[0x80]; memset(sb, 0, sizeof(sb));
		double src[9];
		for(unsigned k = 0; k < 9; ++k)
			{
			src[k] = (ci == 0) ? static_cast<double>(k) + 0.5
				: (ci == 1) ? -1.0e120 * (k + 1)
				: 1.0e-300 * (k + 1);
			memcpy(sb + 0x18 + k * 8, &src[k], 8);
			}
		float outO[9], outC[9];
		memset(outO, 0, sizeof(outO)); memset(outC, 0, sizeof(outC));
		d2f(sb, outO);
		nxDoubleToFloat9_2156(sb, outC);
		if(memcmp(outO, outC, sizeof(outO)) != 0)
			{
			fprintf(stderr,"d2f9 ci=%u\n", ci);
			for(unsigned k = 0; k < 9; ++k) if(*(unsigned*)(outO+k)!=*(unsigned*)(outC+k))
				fprintf(stderr,"  k%u o=%08x c=%08x\n", k,
					*(unsigned*)(outO+k), *(unsigned*)(outC+k));
			++d2fF;
			}
		}
	printf("d2f9 candidate failures=%u provisional=1\n", d2fF);
	}

	// -- Slate row 000046: descriptor gather, with the Foundation lock API
	//    bound to no-op stubs in the oracle image.
	{
	typedef bool (__thiscall* GatherOracle)(void*, unsigned*);
	GatherOracle go = reinterpret_cast<GatherOracle>(base + 0x24c0);
	unsigned char* img = const_cast<unsigned char*>(base);
	void** g10 = reinterpret_cast<void**>(img + 0x104010);
	void** g2c = reinterpret_cast<void**>(img + 0x10402c);
	void** g44 = reinterpret_cast<void**>(img + 0x104044);
	void** g14 = reinterpret_cast<void**>(img + 0x104014);
	void* s10 = *g10, *s2c = *g2c, *s44 = *g44, *s14 = *g14;
	// The pointer slots sit in a read-only page in the loaded image; the
	// loader fills them before the page is protected, so unlocking is needed
	// to bind them from here.
	DWORD oldProt = 0;
	void* pageBase = reinterpret_cast<void*>(
		reinterpret_cast<size_t>(img + 0x104000) & ~static_cast<size_t>(0xFFF));
	bool protOk = VirtualProtect(pageBase, 0x2000, PAGE_READWRITE, &oldProt) != 0;
	*g10 = reinterpret_cast<void*>(&nxLockStub1);
	*g2c = reinterpret_cast<void*>(&nxLockStub3);
	*g44 = reinterpret_cast<void*>(&nxLockStubQuery);
	*g14 = reinterpret_cast<void*>(&nxLockStub1);
	DWORD tmp = 0;
	if(protOk) VirtualProtect(pageBase, 0x2000, oldProt, &tmp);
	unsigned gf046 = 0;
	unsigned char rec[0x200]; memset(rec, 0, sizeof(rec));
	for(unsigned i = 0; i < 0x200; i += 4) *(unsigned*)(rec + i) = 0x71000000u + i;
	float sr[3] = { 4.0f, 9.0f, 16.0f };
	memcpy(rec + 0xd0, &sr[0], 4); memcpy(rec + 0xd4, &sr[1], 4); memcpy(rec + 0xd8, &sr[2], 4);
	// null-record arm must return false and write nothing
	unsigned char lockObj[0x40]; memset(lockObj, 0, sizeof(lockObj));
	unsigned char subObj[0x40]; memset(subObj, 0, sizeof(subObj));
	*(void**)(lockObj) = subObj;
	unsigned char shNull[0x20]; memset(shNull, 0, sizeof(shNull));
	unsigned char bodyNull[0x20]; memset(bodyNull, 0, sizeof(bodyNull));
	*(void**)(shNull + 0x10) = lockObj;
	*(void**)(shNull + 0x14) = bodyNull;
	unsigned probeOut[32]; memset(probeOut, 0xEE, sizeof(probeOut));
	bool rNullO = go(shNull, probeOut);
	bool rNullC = nxGatherDescriptor0046(shNull, probeOut);
	if(rNullO || rNullC){fprintf(stderr,"gather0046 null arm o=%u c=%u\n", rNullO?1u:0u, rNullC?1u:0u);++gf046;}
	// present arm
	unsigned char sh[0x20]; memset(sh, 0, sizeof(sh));
	unsigned char body[0x20]; memset(body, 0, sizeof(body));
	*(void**)(body + 8) = rec;
	*(void**)(sh + 0x10) = lockObj;
	*(void**)(sh + 0x14) = body;
	unsigned outO[32], outC[32];
	memset(outO, 0, sizeof(outO)); memset(outC, 0, sizeof(outC));
	bool rO = go(sh, outO);
	bool rC = nxGatherDescriptor0046(sh, outC);
	if(!rO || !rC || memcmp(outO, outC, sizeof(outO)) != 0)
		{
		fprintf(stderr,"gather0046 rO=%u rC=%u\n", rO?1u:0u, rC?1u:0u);
		for(unsigned i = 0; i < 32; ++i) if(outO[i]!=outC[i])
			fprintf(stderr,"  out[%u] o=%08x c=%08x\n", i, outO[i], outC[i]);
		++gf046;
		}
	if(protOk)
		{
		DWORD t2 = 0;
		VirtualProtect(pageBase, 0x2000, PAGE_READWRITE, &t2);
		*g10 = s10; *g2c = s2c; *g44 = s44; *g14 = s14;
		VirtualProtect(pageBase, 0x2000, oldProt, &t2);
		}
	printf("gather0046 candidate failures=%u provisional=1\n", gf046);
	}

	// -- Slate row 000132: quaternion to 3x3 matrix, lock API bound.
	{
	typedef float* (__thiscall* QuatMxOracle)(void*, float*);
	QuatMxOracle qm = reinterpret_cast<QuatMxOracle>(base + 0x46c0);
	NxLockApiSaved sv132 = nxBindLockApi(base);
	unsigned qf = 0;
	unsigned char lockObj2[0x40]; memset(lockObj2, 0, sizeof(lockObj2));
	unsigned char subObj2[0x40]; memset(subObj2, 0, sizeof(subObj2));
	*(void**)(lockObj2) = subObj2;
	// null-record arm: copies the cached 36 bytes at body+0x20
	unsigned char bodyN[0x60]; memset(bodyN, 0, sizeof(bodyN));
	for(unsigned i = 0x20; i < 0x44; i += 4) *(unsigned*)(bodyN + i) = 0x33000000u + i;
	unsigned char shN[0x20]; memset(shN, 0, sizeof(shN));
	*(void**)(shN + 0x10) = lockObj2;
	*(void**)(shN + 0x14) = bodyN;
	float nO[9], nC[9];
	memset(nO, 0, sizeof(nO)); memset(nC, 0, sizeof(nC));
	qm(shN, nO);
	float* nRet = nxQuatToMatrix0132(shN, nC);
	if(memcmp(nO, nC, sizeof(nO)) != 0 || nRet != nC)
		{
		fprintf(stderr,"quatm0132 null arm\n");
		for(unsigned i=0;i<9;++i) if(*(unsigned*)(nO+i)!=*(unsigned*)(nC+i))
			fprintf(stderr,"  m[%u] o=%08x c=%08x\n", i,
				*(unsigned*)(nO+i), *(unsigned*)(nC+i));
		++qf;
		}
	// present arm: a unit quaternion at record+0x5c..0x68
	unsigned char rec2[0x80]; memset(rec2, 0, sizeof(rec2));
	float qv[4] = { 0.1825742f, 0.3651484f, 0.5477226f, 0.7302967f };
	memcpy(rec2 + 0x5c, qv, sizeof(qv));
	unsigned char body2[0x20]; memset(body2, 0, sizeof(body2));
	*(void**)(body2 + 8) = rec2;
	unsigned char sh2[0x20]; memset(sh2, 0, sizeof(sh2));
	*(void**)(sh2 + 0x10) = lockObj2;
	*(void**)(sh2 + 0x14) = body2;
	float pO[9], pC[9];
	memset(pO, 0, sizeof(pO)); memset(pC, 0, sizeof(pC));
	qm(sh2, pO);
	nxQuatToMatrix0132(sh2, pC);
	if(memcmp(pO, pC, sizeof(pO)) != 0)
		{
		fprintf(stderr,"quatm0132 present arm\n");
		for(unsigned i=0;i<9;++i) if(*(unsigned*)(pO+i)!=*(unsigned*)(pC+i))
			fprintf(stderr,"  m[%u] o=%08x c=%08x\n", i,
				*(unsigned*)(pO+i), *(unsigned*)(pC+i));
		++qf;
		}
	nxUnbindLockApi(base, sv132);
	printf("quatm0132 candidate failures=%u provisional=1\n", qf);
	}

	// -- Slate row 000130: full 0x30-byte pose, lock API bound.
	{
	typedef float* (__thiscall* PoseQuatOracle)(void*, float*);
	PoseQuatOracle pq = reinterpret_cast<PoseQuatOracle>(base + 0x4580);
	NxLockApiSaved sv130 = nxBindLockApi(base);
	unsigned pqf = 0;
	unsigned char lockObj3[0x40]; memset(lockObj3, 0, sizeof(lockObj3));
	unsigned char subObj3[0x40]; memset(subObj3, 0, sizeof(subObj3));
	*(void**)(lockObj3) = subObj3;
	// null arm: cached pose at body+0x20
	unsigned char bodyN3[0x60]; memset(bodyN3, 0, sizeof(bodyN3));
	for(unsigned i = 0x20; i < 0x50; i += 4) *(unsigned*)(bodyN3 + i) = 0x44000000u + i;
	unsigned char shN3[0x20]; memset(shN3, 0, sizeof(shN3));
	*(void**)(shN3 + 0x10) = lockObj3;
	*(void**)(shN3 + 0x14) = bodyN3;
	float n3O[12], n3C[12];
	memset(n3O, 0, sizeof(n3O)); memset(n3C, 0, sizeof(n3C));
	pq(shN3, n3O);
	float* n3Ret = nxPoseFromQuat0130(shN3, n3C);
	if(memcmp(n3O, n3C, sizeof(n3O)) != 0 || n3Ret != n3C)
		{
		fprintf(stderr,"poseq0130 null arm ret=%u\n", n3Ret==n3C?1u:0u);
		for(unsigned i=0;i<12;++i) if(*(unsigned*)(n3O+i)!=*(unsigned*)(n3C+i))
			fprintf(stderr,"  p[%u] o=%08x c=%08x\n", i,
				*(unsigned*)(n3O+i), *(unsigned*)(n3C+i));
		++pqf;
		}
	// present arm: quaternion + translation
	unsigned char rec3[0x80]; memset(rec3, 0, sizeof(rec3));
	float qv3[4] = { 0.1825742f, 0.3651484f, 0.5477226f, 0.7302967f };
	memcpy(rec3 + 0x5c, qv3, sizeof(qv3));
	float tv[3] = { 1.5f, -2.25f, 3.75f };
	memcpy(rec3 + 0x50, tv, sizeof(tv));
	unsigned char body3[0x20]; memset(body3, 0, sizeof(body3));
	*(void**)(body3 + 8) = rec3;
	unsigned char sh3[0x20]; memset(sh3, 0, sizeof(sh3));
	*(void**)(sh3 + 0x10) = lockObj3;
	*(void**)(sh3 + 0x14) = body3;
	float p3O[12], p3C[12];
	memset(p3O, 0, sizeof(p3O)); memset(p3C, 0, sizeof(p3C));
	pq(sh3, p3O);
	nxPoseFromQuat0130(sh3, p3C);
	if(memcmp(p3O, p3C, sizeof(p3O)) != 0)
		{
		fprintf(stderr,"poseq0130 present arm\n");
		for(unsigned i=0;i<12;++i) if(*(unsigned*)(p3O+i)!=*(unsigned*)(p3C+i))
			fprintf(stderr,"  p[%u] o=%08x c=%08x\n", i,
				*(unsigned*)(p3O+i), *(unsigned*)(p3C+i));
		++pqf;
		}
	nxUnbindLockApi(base, sv130);
	printf("poseq0130 candidate failures=%u provisional=1\n", pqf);
	}

	// -- Slate row 000094: orientation quaternion, lock API bound.
	{
	typedef float* (__thiscall* OrientOracle)(void*, float*);
	OrientOracle orq = reinterpret_cast<OrientOracle>(base + 0x2f30);
	NxLockApiSaved sv094 = nxBindLockApi(base);
	unsigned orf = 0;
	unsigned char lockObj4[0x40]; memset(lockObj4, 0, sizeof(lockObj4));
	unsigned char subObj4[0x40]; memset(subObj4, 0, sizeof(subObj4));
	*(void**)(lockObj4) = subObj4;
	// present arm: stored quaternion at record+0x5c
	unsigned char rec4[0x80]; memset(rec4, 0, sizeof(rec4));
	float sv4[4] = { 0.25f, -0.5f, 0.75f, 0.625f };
	memcpy(rec4 + 0x5c, sv4, sizeof(sv4));
	unsigned char body4[0x60]; memset(body4, 0, sizeof(body4));
	*(void**)(body4 + 8) = rec4;
	unsigned char sh4[0x20]; memset(sh4, 0, sizeof(sh4));
	*(void**)(sh4 + 0x10) = lockObj4;
	*(void**)(sh4 + 0x14) = body4;
	float p4O[4], p4C[4];
	memset(p4O, 0, sizeof(p4O)); memset(p4C, 0, sizeof(p4C));
	orq(sh4, p4O);
	float* r4 = nxOrientation0094(sh4, p4C);
	if(memcmp(p4O, p4C, sizeof(p4O)) != 0 || r4 != p4C)
		{
		fprintf(stderr,"orient0094 present arm\n");
		for(unsigned i=0;i<4;++i) if(*(unsigned*)(p4O+i)!=*(unsigned*)(p4C+i))
			fprintf(stderr,"  q[%u] o=%08x c=%08x\n", i,
				*(unsigned*)(p4O+i), *(unsigned*)(p4C+i));
		++orf;
		}
	// fallback arm: cached matrix at body+0x20, walked across all four
	// diagonal branches (trace arm plus the three largest-diagonal cases)
	float mats[4][9];
	{
	float r0[9] = { 1,2,3, 4,5,6, 7,8,9 };            memcpy(mats[0], r0, sizeof(r0));
	float r1[9] = { -2,1,3, 4,-5,6, 7,8,9 };          memcpy(mats[1], r1, sizeof(r1));
	float r2[9] = { -2,1,3, 4,-5,6, 7,8,12 };         memcpy(mats[2], r2, sizeof(r2));
	float r3[9] = { 0.5f,0.25f,-0.125f, 0.75f,0.5f,0.375f, -0.25f,0.625f,0.5f };
	memcpy(mats[3], r3, sizeof(r3));
	}
	for(unsigned mi = 0; mi < 4; ++mi)
		{
		unsigned char bodyF[0x60]; memset(bodyF, 0, sizeof(bodyF));
		memcpy(bodyF + 0x20, mats[mi], 36);
		unsigned char shF[0x20]; memset(shF, 0, sizeof(shF));
		*(void**)(shF + 0x10) = lockObj4;
		*(void**)(shF + 0x14) = bodyF;
		float fO[4], fC[4];
		memset(fO, 0, sizeof(fO)); memset(fC, 0, sizeof(fC));
		orq(shF, fO);
		nxOrientation0094(shF, fC);
		if(memcmp(fO, fC, sizeof(fO)) != 0)
			{
			fprintf(stderr,"orient0094 fallback mi=%u\n", mi);
			for(unsigned i=0;i<4;++i) if(*(unsigned*)(fO+i)!=*(unsigned*)(fC+i))
				fprintf(stderr,"  q[%u] o=%08x c=%08x\n", i,
					*(unsigned*)(fO+i), *(unsigned*)(fC+i));
			++orf;
			}
		}
	nxUnbindLockApi(base, sv094);
	printf("orient0094 candidate failures=%u provisional=1\n", orf);
	}

	// -- Locked accessor batch: 003950 (locked self) and 000418 (field read).
	{
	typedef void* (__thiscall* LockedSelfOracle)(void*);
	typedef unsigned (__thiscall* FieldReadOracle)(void*);
	LockedSelfOracle ls = reinterpret_cast<LockedSelfOracle>(base + 0x8f0d0);
	FieldReadOracle fr = reinterpret_cast<FieldReadOracle>(base + 0xda50);
	NxLockApiSaved svAcc = nxBindLockApi(base);
	unsigned af = 0;
	unsigned char lockObj5[0x40]; memset(lockObj5, 0, sizeof(lockObj5));
	unsigned char subObj5[0x40]; memset(subObj5, 0, sizeof(subObj5));
	*(void**)(lockObj5) = subObj5;
	// 003950
	unsigned char sh5[0x40]; memset(sh5, 0, sizeof(sh5));
	*(void**)(sh5 + 0x10) = lockObj5;
	void* rO = ls(sh5);
	void* rC = nxLockedSelf3950(sh5);
	if(rO != sh5 || rC != sh5){fprintf(stderr,"lockself o=%p c=%p\n", rO, rC);++af;}
	// 000418
	unsigned char owner[0x600]; memset(owner, 0, sizeof(owner));
	*(unsigned*)(owner + 0x55c) = 0xC0DE1234u;
	unsigned char sh6[0x40]; memset(sh6, 0, sizeof(sh6));
	*(void**)(sh6 + 0x10) = lockObj5;
	*(void**)(sh6 + 0x24) = owner;
	unsigned fO = fr(sh6);
	unsigned fC = nxFieldRead0418(sh6);
	if(fO != fC || fO != 0xC0DE1234u)
		{fprintf(stderr,"fieldread o=%08x c=%08x\n", fO, fC);++af;}
	nxUnbindLockApi(base, svAcc);
	printf("lockacc candidate failures=%u provisional=1\n", af);
	}

	// -- Locked accessor family: lock, getter on [self+0x24], unlock, return.
	{
	struct LockedGetter { unsigned rva; unsigned offset; const char* name; };
	static const LockedGetter kLocked[] = {
		{ 0xc8a0, 0x3c,   "000317" }, { 0xc910, 0x6c8, "000321" },
		{ 0xc9a0, 0x6c4,  "000327" }, { 0xcd20, 0x6ac, "000352" },
		{ 0xcdb0, 0x6b0,  "000356" }, { 0xce40, 0x6b4, "000360" },
	};
	NxLockApiSaved svLG = nxBindLockApi(base);
	unsigned lgf = 0;
	unsigned char lockObj6[0x40]; memset(lockObj6, 0, sizeof(lockObj6));
	unsigned char subObj6[0x40]; memset(subObj6, 0, sizeof(subObj6));
	*(void**)(lockObj6) = subObj6;
	for(unsigned i = 0; i < sizeof(kLocked) / sizeof(kLocked[0]); ++i)
		{
		typedef unsigned (__thiscall* LockedGetOracle)(void*);
		LockedGetOracle fn = reinterpret_cast<LockedGetOracle>(base + kLocked[i].rva);
		unsigned char field[0x800]; memset(field, 0, sizeof(field));
		*(unsigned*)(field + kLocked[i].offset) = 0xB0000000u + kLocked[i].offset;
		unsigned char sh[0x40]; memset(sh, 0, sizeof(sh));
		*(void**)(sh + 0x10) = lockObj6;
		*(void**)(sh + 0x24) = field;
		unsigned o = fn(sh);
		unsigned c = nxLockedFieldRead(sh, kLocked[i].offset);
		if(o != c || o != 0xB0000000u + kLocked[i].offset)
			{
			fprintf(stderr,"lockedget %s o=%08x c=%08x\n", kLocked[i].name, o, c);
			++lgf;
			}
		}
	nxUnbindLockApi(base, svLG);
	printf("lockedget candidate failures=%u provisional=1\n", lgf);
	}

	// -- Second locked accessor batch: dword getters and one pointer getter.
	{
	struct LockedGet2 { unsigned rva; unsigned lockOff; unsigned fieldOff; unsigned dataOff; const char* name; };
	static const LockedGet2 kLocked2[] = {
		{ 0x248a0, 0x14, 0x18, 0xd0,  "001203" },
		{ 0xb0730, 0x14, 0x18, 0x168, "004443" },
	};
	NxLockApiSaved svLG2 = nxBindLockApi(base);
	unsigned lg2f = 0;
	unsigned char lockObj7[0x40]; memset(lockObj7, 0, sizeof(lockObj7));
	unsigned char subObj7[0x40]; memset(subObj7, 0, sizeof(subObj7));
	*(void**)(lockObj7) = subObj7;
	for(unsigned i = 0; i < sizeof(kLocked2) / sizeof(kLocked2[0]); ++i)
		{
		typedef unsigned (__thiscall* LockedGetOracle2)(void*);
		LockedGetOracle2 fn = reinterpret_cast<LockedGetOracle2>(base + kLocked2[i].rva);
		unsigned char field[0x200]; memset(field, 0, sizeof(field));
		*(unsigned*)(field + kLocked2[i].dataOff) = 0x5A000000u + kLocked2[i].dataOff;
		unsigned char sh[0x40]; memset(sh, 0, sizeof(sh));
		*(void**)(sh + kLocked2[i].lockOff) = lockObj7;
		*(void**)(sh + kLocked2[i].fieldOff) = field;
		unsigned o = fn(sh);
		unsigned c = nxLockedFieldReadEx(sh, kLocked2[i].lockOff,
			kLocked2[i].fieldOff, kLocked2[i].dataOff);
		if(o != c || o != 0x5A000000u + kLocked2[i].dataOff)
			{
			fprintf(stderr,"lockedget2 %s o=%08x c=%08x\n", kLocked2[i].name, o, c);
			++lg2f;
			}
		}
	// 003824: deref of a lea getter
	{
	typedef unsigned (__thiscall* DerefOracle)(void*);
	DerefOracle fn = reinterpret_cast<DerefOracle>(base + 0x8c9c0);
	unsigned char field[0x40]; memset(field, 0, sizeof(field));
	unsigned target = 0x5B5B5B5Bu;
	*(void**)(field + 0x14) = &target;		// the row dereferences this
	unsigned char sh[0x40]; memset(sh, 0, sizeof(sh));
	*(void**)(sh + 0x10) = lockObj7;
	*(void**)(sh + 0x14) = field;
	unsigned o = fn(sh);
	unsigned c = nxLockedDeref3824(sh);
	if(o != c || o != 0x5B5B5B5Bu){fprintf(stderr,"deref3824 o=%08x c=%08x\n", o, c);++lg2f;}
	}
	// 003872: twelve-dword copy to out
	{
	typedef void* (__thiscall* Copy12Oracle)(void*, unsigned*);
	Copy12Oracle fn = reinterpret_cast<Copy12Oracle>(base + 0x8d1f0);
	unsigned char field[0x60]; memset(field, 0, sizeof(field));
	for(unsigned i = 0; i < 48; i += 4) *(unsigned*)(field + 8 + i) = 0x6C000000u + i;
	unsigned char sh[0x40]; memset(sh, 0, sizeof(sh));
	*(void**)(sh + 0x10) = lockObj7;
	*(void**)(sh + 0x14) = field;
	unsigned o12[12], c12[12];
	memset(o12, 0, sizeof(o12)); memset(c12, 0, sizeof(c12));
	void* ro = fn(sh, o12);
	void* rc = nxLockedCopy12_3872(sh, c12);
	if(memcmp(o12, c12, sizeof(o12)) != 0 || ro != o12 || rc != c12)
		{fprintf(stderr,"copy12_3872 mismatch\n");++lg2f;}
	}
	// 004479: match-or-zero
	{
	typedef unsigned (__thiscall* MatchOracle)(void*, unsigned);
	MatchOracle fn = reinterpret_cast<MatchOracle>(base + 0xb0d20);
	unsigned char field[0x200]; memset(field, 0, sizeof(field));
	*(unsigned*)(field + 0x168) = 0x7777AAAAu;
	unsigned char sh[0x40]; memset(sh, 0, sizeof(sh));
	*(void**)(sh + 0x14) = lockObj7;
	*(void**)(sh + 0x18) = field;
	unsigned o1 = fn(sh, 0x7777AAAAu);
	unsigned c1 = nxLockedMatch4479(sh, 0x7777AAAAu);
	unsigned o2 = fn(sh, 0x12345678u);
	unsigned c2 = nxLockedMatch4479(sh, 0x12345678u);
	if(o1 != c1 || o2 != c2 || o1 != static_cast<unsigned>(reinterpret_cast<size_t>(sh)) || o2 != 0u)
		{fprintf(stderr,"match4479 o=%08x/%08x c=%08x/%08x\n", o1, o2, c1, c2);++lg2f;}
	}
	nxUnbindLockApi(base, svLG2);
	printf("lockedget2 candidate failures=%u provisional=1\n", lg2f);
	}

	// -- Locked copy family: lock, helper copies from the field to out, unlock.
	//    The lock slot differs by variant (0x14 vs 0x10); the first attempt
	//    placed the field where variant B reads its lock, so the lock helper
	//    wrote through the field's first word.
	{
	struct LockedCopy { unsigned rva; unsigned lockOff; unsigned fieldOff; unsigned dataOff; unsigned count; bool pose; const char* name; };
	static const LockedCopy kCopy[] = {
		{ 0x24b40, 0x14, 0x18, 0x90, 3,  false, "001221" },
		{ 0x24100, 0x14, 0x18, 0x90, 3,  false, "001149" },
		{ 0x8c210, 0x10, 0x14, 0x5c, 6,  false, "003784" },
		{ 0x24b70, 0x14, 0x18, 0x6c, 9,  false, "001223" },
		{ 0x246d0, 0x14, 0x18, 0x6c, 9,  false, "001187" },
		{ 0x8b530, 0x10, 0x14, 0x28, 11, false, "003712" },
		{ 0x8c8d0, 0x10, 0x14, 0x18, 9,  false, "003820" },
		{ 0x24b10, 0x14, 0x18, 0x6c, 12, true,  "001219" },
		{ 0x23a60, 0x14, 0x18, 0x6c, 12, true,  "001107" },
		{ 0x8c870, 0x10, 0x14, 0x18, 12, true,  "003816" },
	};
	NxLockApiSaved svLC = nxBindLockApi(base);
	unsigned lcf = 0;
	unsigned char lockObj8[0x40]; memset(lockObj8, 0, sizeof(lockObj8));
	unsigned char subObj8[0x40]; memset(subObj8, 0, sizeof(subObj8));
	*(void**)(lockObj8) = subObj8;
	for(unsigned i = 0; i < sizeof(kCopy) / sizeof(kCopy[0]); ++i)
		{
		typedef void (__thiscall* CopyOracle)(void*, unsigned*);
		CopyOracle fn = reinterpret_cast<CopyOracle>(base + kCopy[i].rva);
		unsigned char field[0x200]; memset(field, 0, sizeof(field));
		for(unsigned w = 0; w < 0x200; w += 4) *(unsigned*)(field + w) = 0x9A000000u + w;
		unsigned char sh[0x40]; memset(sh, 0, sizeof(sh));
		*(void**)(sh + kCopy[i].lockOff) = lockObj8;
		*(void**)(sh + kCopy[i].fieldOff) = field;
		unsigned n = kCopy[i].pose ? 12u : kCopy[i].count;
		unsigned o[12], c[12];
		memset(o, 0, sizeof(o)); memset(c, 0, sizeof(c));
		fn(sh, o);
		if(kCopy[i].pose)
			nxLockedCopyPose(sh, kCopy[i].fieldOff, kCopy[i].dataOff, c);
		else
			nxLockedCopyOut(sh, kCopy[i].fieldOff, kCopy[i].dataOff, n, c);
		if(memcmp(o, c, n * 4u) != 0)
			{
			fprintf(stderr,"lockedcopy %s\n", kCopy[i].name);
			for(unsigned w = 0; w < n; ++w) if(o[w]!=c[w])
				fprintf(stderr,"  w%u o=%08x c=%08x\n", w, o[w], c[w]);
			++lcf;
			}
		}
	nxUnbindLockApi(base, svLC);
	printf("lockedcopy candidate failures=%u provisional=1\n", lcf);
	}

	// -- Locked accessor third batch: copy-to-out, mask, bit-extract.
	{
	NxLockApiSaved svB3 = nxBindLockApi(base);
	unsigned b3f = 0;
	unsigned char lockObj9[0x40]; memset(lockObj9, 0, sizeof(lockObj9));
	unsigned char subObj9[0x40]; memset(subObj9, 0, sizeof(subObj9));
	*(void**)(lockObj9) = subObj9;
	unsigned char field[0x800]; memset(field, 0, sizeof(field));
	for(unsigned w = 0; w < 0x800; w += 4) *(unsigned*)(field + w) = 0x3C000000u + w;
	unsigned char sh[0x40];
	// 000291: copy 3 dwords from field+0x520 to out
	{
	typedef void (__thiscall* Copy3Oracle)(void*, unsigned*);
	Copy3Oracle fn = reinterpret_cast<Copy3Oracle>(base + 0xc460);
	memset(sh, 0, sizeof(sh));
	*(void**)(sh + 0x10) = lockObj9;
	*(void**)(sh + 0x24) = field;
	unsigned o[4], c[4];
	memset(o, 0, sizeof(o)); memset(c, 0, sizeof(c));
	fn(sh, o);
	nxLockedCopyOut(sh, 0x24, 0x520, 3, c);
	if(memcmp(o, c, 12) != 0){fprintf(stderr,"b3 000291\n");++b3f;}
	}
	// 003818: copy 3 dwords from field+0x3c, returns out
	{
	typedef void* (__thiscall* Copy3RetOracle)(void*, unsigned*);
	Copy3RetOracle fn = reinterpret_cast<Copy3RetOracle>(base + 0x8c8a0);
	memset(sh, 0, sizeof(sh));
	*(void**)(sh + 0x10) = lockObj9;
	*(void**)(sh + 0x14) = field;
	unsigned o[4], c[4];
	memset(o, 0, sizeof(o)); memset(c, 0, sizeof(c));
	void* ro = fn(sh, o);
	nxLockedCopyOut(sh, 0x14, 0x3c, 3, c);
	if(memcmp(o, c, 12) != 0 || ro != o){fprintf(stderr,"b3 003818\n");++b3f;}
	}
	// 003746 / 003852: mask getters
	{
	struct MaskRow { unsigned rva; unsigned dataOff; const char* name; };
	static const MaskRow kMask[] = { { 0x8be40, 0x58, "003746" }, { 0x8cf20, 0x10, "003852" } };
	for(unsigned i = 0; i < 2; ++i)
		{
		typedef unsigned (__thiscall* MaskOracle)(void*, unsigned);
		MaskOracle fn = reinterpret_cast<MaskOracle>(base + kMask[i].rva);
		memset(sh, 0, sizeof(sh));
		*(void**)(sh + 0x10) = lockObj9;
		*(void**)(sh + 0x14) = field;
		unsigned o = fn(sh, 0x0F0F0F0Fu);
		unsigned c = nxLockedAndRead(sh, 0x14, kMask[i].dataOff, 0x0F0F0F0Fu);
		if(o != c){fprintf(stderr,"b3 mask %s o=%08x c=%08x\n", kMask[i].name, o, c);++b3f;}
		}
	}
	nxUnbindLockApi(base, svB3);
	printf("lockacc3 candidate failures=%u provisional=1\n", b3f);
	}

	// -- Locked accessor fourth batch: count, flag, bit-extract, address.
	{
	NxLockApiSaved svB4 = nxBindLockApi(base);
	unsigned b4f = 0;
	unsigned char lockObjA[0x40]; memset(lockObjA, 0, sizeof(lockObjA));
	unsigned char subObjA[0x40]; memset(subObjA, 0, sizeof(subObjA));
	*(void**)(lockObjA) = subObjA;
	unsigned char field[0x200];
	unsigned char sh[0x40];
	// 003700: element count from [field+8]-[field+4]
	{
	typedef int (__thiscall* CountOracle)(void*);
	CountOracle fn = reinterpret_cast<CountOracle>(base + 0x8b1f0);
	for(unsigned ci = 0; ci < 3; ++ci)
		{
		memset(field, 0, sizeof(field));
		unsigned lo = 0x1000, hi = 0x1000 + 4u * ci;
		memcpy(field + 4, &lo, 4); memcpy(field + 8, &hi, 4);
		memset(sh, 0, sizeof(sh));
		*(void**)(sh + 0x10) = lockObjA;
		*(void**)(sh + 0x14) = field;
		int o = fn(sh);
		int c = nxLockedElementCount(sh, 0x14);
		if(o != c){fprintf(stderr,"b4 count ci=%u o=%d c=%d\n", ci, o, c);++b4f;}
		}
	}
	// 003702: interval flag
	{
	typedef unsigned (__thiscall* FlagOracle)(void*);
	FlagOracle fn = reinterpret_cast<FlagOracle>(base + 0x8b220);
	for(unsigned ci = 0; ci < 4; ++ci)
		{
		memset(field, 0, sizeof(field));
		unsigned lo = 0x2000 + ci, hi = lo + (ci == 0 ? 0u : (ci == 1 ? 1u : (ci == 2 ? 3u : 4u)));
		memcpy(field + 0x14, &lo, 4); memcpy(field + 0x18, &hi, 4);
		memset(sh, 0, sizeof(sh));
		*(void**)(sh + 0x10) = lockObjA;
		*(void**)(sh + 0x14) = field;
		unsigned o = fn(sh);
		unsigned c = nxLockedIntervalFlag(sh, 0x14);
		if(o != c){fprintf(stderr,"b4 flag ci=%u o=%08x c=%08x\n", ci, o, c);++b4f;}
		}
	}
	// 004483: bit extract ([field+0x2c]>>3)&3
	{
	typedef unsigned (__thiscall* BitOracle)(void*);
	BitOracle fn = reinterpret_cast<BitOracle>(base + 0xb0dc0);
	for(unsigned ci = 0; ci < 4; ++ci)
		{
		memset(field, 0, sizeof(field));
		unsigned v = 0xAB000000u | (ci * 8u + 2u);
		memcpy(field + 0x2c, &v, 4);
		memset(sh, 0, sizeof(sh));
		*(void**)(sh + 0x14) = lockObjA;
		*(void**)(sh + 0x18) = field;
		unsigned o = fn(sh);
		unsigned c = nxLockedBitExtract(sh, 0x18, 0x2c);
		if(o != c){fprintf(stderr,"b4 bit ci=%u o=%08x c=%08x\n", ci, o, c);++b4f;}
		}
	}
	// 001071: address getter (lea [field+0xe4])
	{
	typedef void* (__thiscall* AddrOracle)(void*);
	AddrOracle fn = reinterpret_cast<AddrOracle>(base + 0x23520);
	memset(field, 0, sizeof(field));
	memset(sh, 0, sizeof(sh));
	*(void**)(sh + 0x14) = lockObjA;
	*(void**)(sh + 0x18) = field;
	void* o = fn(sh);
	void* c = nxLockedFieldAddress(sh, 0x18, 0xe4);
	if(o != c || o != field + 0xe4){fprintf(stderr,"b4 addr o=%p c=%p\n", o, c);++b4f;}
	}
	nxUnbindLockApi(base, svB4);
	printf("lockacc4 candidate failures=%u provisional=1\n", b4f);
	}

	// -- Locked accessor fifth batch: word-mask, two-pointer copy, float.
	{
	NxLockApiSaved svB5 = nxBindLockApi(base);
	unsigned b5f = 0;
	unsigned char lockObjB[0x40]; memset(lockObjB, 0, sizeof(lockObjB));
	unsigned char subObjB[0x40]; memset(subObjB, 0, sizeof(subObjB));
	*(void**)(lockObjB) = subObjB;
	unsigned char field[0x200];
	unsigned char sh[0x40];
	// 001085: (word [field+0xde]) & arg
	{
	typedef unsigned (__thiscall* WordOracle)(void*, unsigned);
	WordOracle fn = reinterpret_cast<WordOracle>(base + 0x236d0);
	memset(field, 0, sizeof(field));
	unsigned short w = 0xBEEFu;
	memcpy(field + 0xde, &w, 2);
	memset(sh, 0, sizeof(sh));
	*(void**)(sh + 0x14) = lockObjB;
	*(void**)(sh + 0x18) = field;
	unsigned o = fn(sh, 0x0F0Fu);
	unsigned c = nxLockedWordAndRead(sh, 0x18, 0xde, 0x0F0Fu);
	if(o != c || o != 0x0E0Fu){fprintf(stderr,"b5 word o=%08x c=%08x\n", o, c);++b5f;}
	}
	// 004573: two out pointers
	{
	typedef void (__thiscall* TwoPtrOracle)(void*, unsigned*, unsigned*);
	TwoPtrOracle fn = reinterpret_cast<TwoPtrOracle>(base + 0xb1bd0);
	memset(field, 0, sizeof(field));
	unsigned v1 = 0x11112222u, v2 = 0x33334444u;
	memcpy(field + 0x3c, &v1, 4); memcpy(field + 0x40, &v2, 4);
	memset(sh, 0, sizeof(sh));
	*(void**)(sh + 0x14) = lockObjB;
	*(void**)(sh + 0x18) = field;
	unsigned o1 = 0, o2 = 0, c1 = 0, c2 = 0;
	fn(sh, &o1, &o2);
	nxLockedCopyTwoPointers(sh, 0x18, &c1, &c2);
	if(o1 != c1 || o2 != c2 || o1 != v1 || o2 != v2)
		{fprintf(stderr,"b5 twoptr o=%08x/%08x c=%08x/%08x\n", o1, o2, c1, c2);++b5f;}
	}
	// 001121: float returned in st(0)
	{
	typedef float (__thiscall* FloatOracle)(void*);
	FloatOracle fn = reinterpret_cast<FloatOracle>(base + 0x23c80);
	for(unsigned ci = 0; ci < 3; ++ci)
		{
		memset(field, 0, sizeof(field));
		float v = (ci == 0) ? 2.5f : (ci == 1) ? -0.75f : 1.0e20f;
		memcpy(field + 0xe4, &v, 4);
		memset(sh, 0, sizeof(sh));
		*(void**)(sh + 0x14) = lockObjB;
		*(void**)(sh + 0x18) = field;
		float o = fn(sh);
		float c = nxLockedDoubleField(sh, 0x18, 0xe4);
		if(memcmp(&o, &c, 4) != 0){fprintf(stderr,"b5 float ci=%u o=%08x c=%08x\n", ci,
			*(unsigned*)&o, *(unsigned*)&c);++b5f;}
		}
	}
	nxUnbindLockApi(base, svB5);
	printf("lockacc5 candidate failures=%u provisional=1\n", b5f);
	}

	// -- Locked accessor sixth batch: copy+flag members and a three-pointer copy.
	{
	NxLockApiSaved svB6 = nxBindLockApi(base);
	unsigned b6f = 0;
	unsigned char lockObjC[0x40]; memset(lockObjC, 0, sizeof(lockObjC));
	unsigned char subObjC[0x40]; memset(subObjC, 0, sizeof(subObjC));
	*(void**)(lockObjC) = subObjC;
	unsigned char field[0x800];
	unsigned char sh[0x40];
	struct CopyFlag { unsigned rva; unsigned dataOff; unsigned count; unsigned shift; const char* name; };
	static const CopyFlag kCF[] = {
		{ 0xb3240, 0x16c, 6, 0, "004711" },
		{ 0xb3280, 0x184, 3, 1, "004715" },
		{ 0xb3310, 0x190, 3, 2, "004719" },
	};
	for(unsigned ci = 0; ci < 3; ++ci)
		{
		typedef unsigned char (__thiscall* CopyFlagOracle)(void*, unsigned*);
		CopyFlagOracle fn = reinterpret_cast<CopyFlagOracle>(base + kCF[ci].rva);
		for(unsigned fl = 0; fl < 2; ++fl)
			{
			memset(field, 0, sizeof(field));
			for(unsigned w = 0; w < 0x800; w += 4) *(unsigned*)(field + w) = 0xD0000000u + w;
			unsigned flags = fl ? (1u << kCF[ci].shift) : 0u;
			memcpy(field + 0x1a8, &flags, 4);
			memset(sh, 0, sizeof(sh));
			*(void**)(sh + 0x14) = lockObjC;
			*(void**)(sh + 0x18) = field;
			unsigned o[6], c[6];
			memset(o, 0, sizeof(o)); memset(c, 0, sizeof(c));
			unsigned char ro = fn(sh, o);
			unsigned char rc = nxLockedCopyAndFlag(sh, 0x18, kCF[ci].dataOff,
				kCF[ci].count, kCF[ci].shift, c);
			if(memcmp(o, c, kCF[ci].count * 4u) != 0 || ro != rc
				|| ro != static_cast<unsigned char>(fl))
				{
				fprintf(stderr,"b6 %s fl=%u ro=%u rc=%u\n", kCF[ci].name, fl, ro, rc);
				++b6f;
				}
			}
		}
	// 000340: three out pointers
	{
	typedef void (__thiscall* ThreePtrOracle)(void*, unsigned*, unsigned*, unsigned*);
	ThreePtrOracle fn = reinterpret_cast<ThreePtrOracle>(base + 0xcb50);
	memset(field, 0, sizeof(field));
	unsigned v1 = 0xA1A1A1A1u, v2 = 0xB2B2B2B2u, v3 = 0xC3C3C3C3u;
	memcpy(field + 0x52c, &v1, 4); memcpy(field + 0x530, &v2, 4); memcpy(field + 0x534, &v3, 4);
	memset(sh, 0, sizeof(sh));
	*(void**)(sh + 0x10) = lockObjC;
	*(void**)(sh + 0x24) = field;
	unsigned o1=0,o2=0,o3=0,c1=0,c2=0,c3=0;
	fn(sh, &o1, &o2, &o3);
	nxLockedCopyThreePointers(sh, 0x24, &c1, &c2, &c3);
	if(o1!=c1 || o2!=c2 || o3!=c3 || o1!=v1 || o2!=v2 || o3!=v3)
		{fprintf(stderr,"b6 000340 o=%08x/%08x/%08x c=%08x/%08x/%08x\n", o1,o2,o3,c1,c2,c3);++b6f;}
	}
	nxUnbindLockApi(base, svB6);
	printf("lockacc6 candidate failures=%u provisional=1\n", b6f);
	}

	// -- Locked accessor seventh batch: pure-field members.
	{
	NxLockApiSaved svB7 = nxBindLockApi(base);
	unsigned b7f = 0;
	unsigned char lockObjD[0x40]; memset(lockObjD, 0, sizeof(lockObjD));
	unsigned char subObjD[0x40]; memset(subObjD, 0, sizeof(subObjD));
	*(void**)(lockObjD) = subObjD;
	unsigned char field[0x800];
	unsigned char sh[0x40];
	// 000416: element count ([field+0x560]-[field+0x55c])>>2
	{
	typedef int (__thiscall* Count2Oracle)(void*);
	Count2Oracle fn = reinterpret_cast<Count2Oracle>(base + 0xda10);
	for(unsigned ci = 0; ci < 3; ++ci)
		{
		memset(field, 0, sizeof(field));
		unsigned lo = 0x3000, hi = 0x3000 + 4u * ci;
		memcpy(field + 0x55c, &lo, 4); memcpy(field + 0x560, &hi, 4);
		memset(sh, 0, sizeof(sh));
		*(void**)(sh + 0x10) = lockObjD;
		*(void**)(sh + 0x24) = field;
		int o = fn(sh);
		int c = nxLockedElementCountAt(sh, 0x24, 0x560, 0x55c);
		if(o != c){fprintf(stderr,"b7 count ci=%u o=%d c=%d\n", ci, o, c);++b7f;}
		}
	}
	// 003808: copy 9 dwords from field+0x48, returns out
	{
	typedef void* (__thiscall* Copy9Oracle)(void*, unsigned*);
	Copy9Oracle fn = reinterpret_cast<Copy9Oracle>(base + 0x8c620);
	memset(field, 0, sizeof(field));
	for(unsigned w = 0; w < 0x800; w += 4) *(unsigned*)(field + w) = 0xE0000000u + w;
	memset(sh, 0, sizeof(sh));
	*(void**)(sh + 0x10) = lockObjD;
	*(void**)(sh + 0x14) = field;
	unsigned o[9], c[9];
	memset(o, 0, sizeof(o)); memset(c, 0, sizeof(c));
	void* ro = fn(sh, o);
	nxLockedCopyOut(sh, 0x14, 0x48, 9, c);
	if(memcmp(o, c, 36) != 0 || ro != o){fprintf(stderr,"b7 003808\n");++b7f;}
	}
	// 003806: copy 3 dwords from field+0x6c, returns out
	{
	typedef void* (__thiscall* Copy3bOracle)(void*, unsigned*);
	Copy3bOracle fn = reinterpret_cast<Copy3bOracle>(base + 0x8c5e0);
	memset(field, 0, sizeof(field));
	for(unsigned w = 0; w < 0x800; w += 4) *(unsigned*)(field + w) = 0xF0000000u + w;
	memset(sh, 0, sizeof(sh));
	*(void**)(sh + 0x10) = lockObjD;
	*(void**)(sh + 0x14) = field;
	unsigned o[4], c[4];
	memset(o, 0, sizeof(o)); memset(c, 0, sizeof(c));
	void* ro = fn(sh, o);
	nxLockedCopyOut(sh, 0x14, 0x6c, 3, c);
	if(memcmp(o, c, 12) != 0 || ro != o){fprintf(stderr,"b7 003806\n");++b7f;}
	}
	// 000421: conditional deref of [field+0x61c]
	{
	typedef unsigned (__thiscall* DerefCondOracle)(void*);
	DerefCondOracle fn = reinterpret_cast<DerefCondOracle>(base + 0xdac0);
	unsigned target = 0x1234ABCDu;
	unsigned char inner[0x40]; memset(inner, 0, sizeof(inner));
	memcpy(inner + 0x14, &target, 4);
	for(unsigned ci = 0; ci < 2; ++ci)
		{
		memset(field, 0, sizeof(field));
		if(ci == 0) *(void**)(field + 0x61c) = inner;
		memset(sh, 0, sizeof(sh));
		*(void**)(sh + 0x10) = lockObjD;
		*(void**)(sh + 0x24) = field;
		unsigned o = fn(sh);
		unsigned c = nxLockedDerefField(sh, 0x24, 0x61c, 0x14);
		unsigned want = ci == 0 ? target : 0u;
		if(o != c || o != want){fprintf(stderr,"b7 deref ci=%u o=%08x c=%08x\n", ci, o, c);++b7f;}
		}
	}
	nxUnbindLockApi(base, svB7);
	printf("lockacc7 candidate failures=%u provisional=1\n", b7f);
	}
	// -- Locked accessor eighth batch: pose copy, conditional count, nested
	//    derefs, and N-pointer copies.
	{
	NxLockApiSaved svB8 = nxBindLockApi(base);
	unsigned b8f = 0;
	unsigned char lockObjE[0x40]; memset(lockObjE, 0, sizeof(lockObjE));
	unsigned char subObjE[0x40]; memset(subObjE, 0, sizeof(subObjE));
	*(void**)(lockObjE) = subObjE;
	unsigned char field[0x800];
	unsigned char sh[0x40];
	for(unsigned w = 0; w < 0x800; w += 4) *(unsigned*)(field + w) = 0x2B000000u + w;
	// 003804: pose copy (9 from field+0x48, then 3 at +0x24)
	{
	typedef void* (__thiscall* PoseCopyOracle)(void*, unsigned*);
	PoseCopyOracle fn = reinterpret_cast<PoseCopyOracle>(base + 0x8c590);
	memset(sh, 0, sizeof(sh));
	*(void**)(sh + 0x10) = lockObjE;
	*(void**)(sh + 0x14) = field;
	unsigned o[12], c[12];
	memset(o, 0, sizeof(o)); memset(c, 0, sizeof(c));
	void* ro = fn(sh, o);
	nxLockedCopyPose(sh, 0x14, 0x48, c);
	if(memcmp(o, c, 48) != 0 || ro != o){fprintf(stderr,"b8 003804\n");++b8f;}
	}
	// 000420: conditional count through [field+0x61c]
	{
	typedef int (__thiscall* CondCountOracle)(void*);
	CondCountOracle fn = reinterpret_cast<CondCountOracle>(base + 0xda80);
	unsigned char inner[0x40]; memset(inner, 0, sizeof(inner));
	unsigned lo = 0x5000, hi = 0x5000 + 8;
	memcpy(inner + 4, &lo, 4); memcpy(inner + 8, &hi, 4);
	for(unsigned ci = 0; ci < 2; ++ci)
		{
		memset(field, 0, sizeof(field));
		if(ci == 0) *(void**)(field + 0x61c) = inner;
		memset(sh, 0, sizeof(sh));
		*(void**)(sh + 0x10) = lockObjE;
		*(void**)(sh + 0x24) = field;
		int o = fn(sh);
		int c = nxLockedConditionalCount(sh, 0x24, 0x61c, 8, 4);
		int want = ci == 0 ? 2 : 0;
		if(o != c || o != want){fprintf(stderr,"b8 000420 ci=%u o=%d c=%d\n", ci, o, c);++b8f;}
		}
	}
	// 004539: two nested derefs, two out args
	{
	typedef void (__thiscall* NestedOracle)(void*, unsigned*, unsigned*);
	NestedOracle fn = reinterpret_cast<NestedOracle>(base + 0xb1600);
	unsigned char in1[0x200], in2[0x200];
	memset(in1, 0, sizeof(in1)); memset(in2, 0, sizeof(in2));
	unsigned v1 = 0x0A0A0A0Au, v2 = 0x0B0B0B0Bu;
	unsigned* pv1 = &v1; unsigned* pv2 = &v2;
	// the row reads [node+0x19c] and then dereferences it, so the fixture must
	// hold a pointer there rather than the value itself.
	*(void**)(in1 + 0x19c) = pv1;
	*(void**)(in2 + 0x19c) = pv2;
	memset(field, 0, sizeof(field));
	*(void**)(field + 8) = in1;
	*(void**)(field + 0xc) = in2;
	memset(sh, 0, sizeof(sh));
	*(void**)(sh + 0x14) = lockObjE;
	*(void**)(sh + 0x18) = field;
	unsigned o1=0,o2=0,c1=0,c2=0;
	fn(sh, &o1, &o2);
	nxLockedTwoNestedDerefs(sh, 0x18, 8, 0xc, 0x19c, &c1, &c2);
	if(o1!=c1 || o2!=c2 || o1!=v1 || o2!=v2)
		{fprintf(stderr,"b8 004539 o=%08x/%08x c=%08x/%08x\n", o1,o2,c1,c2);++b8f;}
	}
	// 003948 (4 pointers from +0x58) and 003946 (5 pointers from +0x44)
	{
	struct NRow { unsigned rva; unsigned dataOff; unsigned count; const char* name; };
	static const NRow kN[] = { { 0x8f090, 0x58, 4, "003948" }, { 0x8f050, 0x44, 5, "003946" } };
	for(unsigned i = 0; i < 2; ++i)
		{
		void* fnp = const_cast<unsigned char*>(base) + kN[i].rva;
		unsigned v[5] = { 0, 0, 0, 0, 0 };
		unsigned* outs[5] = { &v[0], &v[1], &v[2], &v[3], &v[4] };
		unsigned c[5] = { 0, 0, 0, 0, 0 };
		unsigned* couts[5] = { &c[0], &c[1], &c[2], &c[3], &c[4] };
		memset(field, 0, sizeof(field));
		for(unsigned w = 0; w < 0x800; w += 4) *(unsigned*)(field + w) = 0x4C000000u + w;
		memset(sh, 0, sizeof(sh));
		*(void**)(sh + 0x10) = lockObjE;
		*(void**)(sh + 0x14) = field;
		if(kN[i].count == 4)
			reinterpret_cast<void (__thiscall*)(void*, unsigned*, unsigned*, unsigned*, unsigned*)>(fnp)
				(sh, outs[0], outs[1], outs[2], outs[3]);
		else
			reinterpret_cast<void (__thiscall*)(void*, unsigned*, unsigned*, unsigned*, unsigned*, unsigned*)>(fnp)
				(sh, outs[0], outs[1], outs[2], outs[3], outs[4]);
		nxLockedCopyNPointers(sh, 0x14, kN[i].dataOff, kN[i].count, couts);
		if(memcmp(v, c, kN[i].count * 4u) != 0)
			{fprintf(stderr,"b8 %s mismatch\n", kN[i].name);++b8f;}
		}
	}
	nxUnbindLockApi(base, svB8);
	printf("lockacc8 candidate failures=%u provisional=1\n", b8f);
	}
	// -- Locked accessor ninth batch: match-or-self and list advance.
	{
	NxLockApiSaved svB9 = nxBindLockApi(base);
	unsigned b9f = 0;
	unsigned char lockObjF[0x40]; memset(lockObjF, 0, sizeof(lockObjF));
	unsigned char subObjF[0x40]; memset(subObjF, 0, sizeof(subObjF));
	*(void**)(lockObjF) = subObjF;
	unsigned char field[0x800];
	unsigned char sh[0x40];
	// 001109: match-or-self over [field+0xd0]
	{
	typedef unsigned (__thiscall* MatchOracle2)(void*, unsigned);
	MatchOracle2 fn = reinterpret_cast<MatchOracle2>(base + 0x23a90);
	memset(field, 0, sizeof(field));
	unsigned fv = 0x7777AAAAu;
	memcpy(field + 0xd0, &fv, 4);
	memset(sh, 0, sizeof(sh));
	*(void**)(sh + 0x14) = lockObjF;
	*(void**)(sh + 0x18) = field;
	unsigned o1 = fn(sh, fv);
	unsigned c1 = nxLockedMatchEx(sh, 0x18, 0xd0, fv);
	unsigned o2 = fn(sh, 0x12345678u);
	unsigned c2 = nxLockedMatchEx(sh, 0x18, 0xd0, 0x12345678u);
	if(o1 != c1 || o2 != c2 || o1 != static_cast<unsigned>(reinterpret_cast<size_t>(sh)) || o2 != 0u)
		{fprintf(stderr,"b9 001109 o=%08x/%08x c=%08x/%08x\n", o1,o2,c1,c2);++b9f;}
	}
	// 000325 (+0x6bc/+0x10/+0x48) and 000331 (+0x6c0/+0x18/+0x20)
	{
	struct AdvRow { unsigned rva; unsigned linkOff; unsigned nextOff; unsigned readOff; const char* name; };
	static const AdvRow kAdv[] = {
		{ 0xc960, 0x6bc, 0x10, 0x48, "000325" },
		{ 0xc9f0, 0x6c0, 0x18, 0x20, "000331" },
	};
	for(unsigned i = 0; i < 2; ++i)
		{
		typedef unsigned (__thiscall* AdvOracle)(void*);
		AdvOracle fn = reinterpret_cast<AdvOracle>(base + kAdv[i].rva);
		unsigned char node[0x80], node2[0x80];
		memset(node, 0, sizeof(node)); memset(node2, 0, sizeof(node2));
		unsigned rv = 0x5EED0000u + kAdv[i].readOff;
		memcpy(node + kAdv[i].readOff, &rv, 4);
		*(void**)(node + kAdv[i].nextOff) = node2;
		// null-head arm
		memset(field, 0, sizeof(field));
		memset(sh, 0, sizeof(sh));
		*(void**)(sh + 0x10) = lockObjF;
		*(void**)(sh + 0x24) = field;
		unsigned oN = fn(sh);
		unsigned cN = nxLockedAdvanceRead(sh, 0x24, kAdv[i].linkOff, kAdv[i].nextOff, kAdv[i].readOff);
		if(oN != 0u || cN != 0u){fprintf(stderr,"b9 %s null o=%08x c=%08x\n", kAdv[i].name, oN, cN);++b9f;}
		// head-present arm: both sides must relink and read
		memset(field, 0, sizeof(field));
		*(void**)(field + kAdv[i].linkOff) = node;
		unsigned char sh2[0x40]; memset(sh2, 0, sizeof(sh2));
		*(void**)(sh2 + 0x10) = lockObjF;
		*(void**)(sh2 + 0x24) = field;
		unsigned oP = fn(sh2);
		unsigned linkedO = 0;
		memcpy(&linkedO, field + kAdv[i].linkOff, 4);
		memset(field, 0, sizeof(field));
		*(void**)(field + kAdv[i].linkOff) = node;
		unsigned cP = nxLockedAdvanceRead(sh, 0x24, kAdv[i].linkOff, kAdv[i].nextOff, kAdv[i].readOff);
		unsigned linkedC = 0;
		memcpy(&linkedC, field + kAdv[i].linkOff, 4);
		if(oP != cP || oP != rv || linkedO != linkedC
			|| linkedO != static_cast<unsigned>(reinterpret_cast<size_t>(node2)))
			{fprintf(stderr,"b9 %s head o=%08x c=%08x lo=%08x lc=%08x\n",
				kAdv[i].name, oP, cP, linkedO, linkedC);++b9f;}
		}
	}
	nxUnbindLockApi(base, svB9);
	printf("lockacc9 candidate failures=%u provisional=1\n", b9f);
	}
	// -- 004886-calling wrapper 005450, with the callback slot bound.
	{
	typedef unsigned char (__thiscall* W5450Oracle)(void*, void*);
	W5450Oracle w5450 = reinterpret_cast<W5450Oracle>(base + 0xef690);
	NxCallbackSaved svCb = nxBindCallbackSlot(base);
	unsigned w5f = 0;
	for(unsigned ci = 0; ci < 3; ++ci)
		{
		unsigned char ob[0x80], cb[0x80];
		memset(ob, 0, sizeof(ob)); memset(cb, 0, sizeof(cb));
		unsigned char nodeO[0x40], nodeC[0x40];
		memset(nodeO, 0, sizeof(nodeO)); memset(nodeC, 0, sizeof(nodeC));
		unsigned char tgtO[0x40], tgtC[0x40];
		memset(tgtO, 0, sizeof(tgtO)); memset(tgtC, 0, sizeof(tgtC));
		*(void**)(ob + 0x14) = tgtO; *(void**)(cb + 0x14) = tgtC;
		unsigned short w = 0x0000; if(ci == 1) w = 0xffff;
		memcpy(nodeO + 0x28, &w, 2); memcpy(nodeC + 0x28, &w, 2);
		if(ci == 2){ nodeO[8] = 2; nodeC[8] = 2; }
		unsigned char rO = w5450(ob, nodeO);
		unsigned char rC = nxWrap5450(cb, nodeC);
		if(rO != rC || *(unsigned*)(ob+0x38) != *(unsigned*)(cb+0x38)
			|| memcmp(nodeO, nodeC, 0x40) != 0)
			{
			fprintf(stderr,"wrap5450 ci=%u rO=%u rC=%u c38=%u/%u\n", ci, rO, rC,
				*(unsigned*)(ob+0x38), *(unsigned*)(cb+0x38));
			++w5f;
			}
		}
	nxUnbindCallbackSlot(base, svCb);
	printf("wrap5450 candidate failures=%u provisional=1\n", w5f);
	}
	// -- 004886-calling sibling 001787 (caller-cleaned, object is arg2).
	{
	typedef unsigned char (__cdecl* W1787Oracle)(void*, void*);
	W1787Oracle w1787 = reinterpret_cast<W1787Oracle>(base + 0x3f570);
	NxCallbackSaved svCb2 = nxBindCallbackSlot(base);
	unsigned w7f = 0;
	for(unsigned ci = 0; ci < 3; ++ci)
		{
		unsigned char nodeO[0x100], nodeC[0x100];
		memset(nodeO, 0, sizeof(nodeO)); memset(nodeC, 0, sizeof(nodeC));
		unsigned char innerO[0x40], innerC[0x40];
		memset(innerO, 0, sizeof(innerO)); memset(innerC, 0, sizeof(innerC));
		unsigned short w = 0x0000; if(ci == 1) w = 0xffff;
		memcpy(nodeO + 0xcc, &w, 2); memcpy(nodeC + 0xcc, &w, 2);
		if(ci == 2){ nodeO[0xa4 + 8] = 2; nodeC[0xa4 + 8] = 2; }
		*(void**)(nodeO + 0xc4) = innerO + 0x14;
		*(void**)(nodeC + 0xc4) = innerC + 0x14;
		unsigned char rO = w1787(nullptr, nodeO);
		unsigned char rC = nxWrap1787(nullptr, nodeC);
		// the fixture puts a different inner pointer at +0xc4 on each side;
		// clear it before comparing so only the row's own writes count.
		*(void**)(nodeO + 0xc4) = nullptr;
		*(void**)(nodeC + 0xc4) = nullptr;
		if(rO != rC || memcmp(nodeO, nodeC, 0x100) != 0)
			{
			fprintf(stderr,"wrap1787 ci=%u rO=%u rC=%u\n", ci, rO, rC);
			for(unsigned k = 0; k < 0x100; ++k)
				if(nodeO[k] != nodeC[k]) fprintf(stderr,"  n[%02x] o=%02x c=%02x\n", k, nodeO[k], nodeC[k]);
			++w7f;
			}
		}
	nxUnbindCallbackSlot(base, svCb2);
	printf("wrap1787 candidate failures=%u provisional=1\n", w7f);
	}






	// -----------------------------------------------------------------------
	// Mass helper phys_fn_000849: the compute-mass row over three
	// half-extents {1.5, 2.0, 2.5}. Two drives -- density 2.0f and the
	// exact 1.0f sentinel -- folded over all thirteen words of each frame.
	// Extra payload pointer stays null (parallel-axis pair undecoded).
	{
	typedef void (__thiscall* NxBoxMassFn)(void* self, float density,
		const float* halfExtents, const void* extra);
	NxBoxMassFn boxMassFn = (NxBoxMassFn) (base + 0x0001c8c0);

	static const float kHalfExt[3] = { 1.5f, 2.0f, 2.5f };
	unsigned char destC[0x34];
	memset(destC, 0, sizeof(destC));
	boxMassFn(destC, 2.0f, kHalfExt, 0);

	unsigned char destD[0x34];
	memset(destD, 0, sizeof(destD));
	boxMassFn(destD, 1.0f, kHalfExt, 0);

	unsigned bd = 2166136261u;
	for(int v = 0; v < 2; ++v)
		{
		const unsigned char* src = (v == 0) ? destC : destD;
		for(int i = 0; i < 0x34; i += 4)
			{
			unsigned w;
			memcpy(&w, src + i, 4);
			bd = nxFold(bd, w);
			}
		}
	oBoxMassDigest = bd;

	unsigned mC, mD;
	memcpy(&mC, destC + 0x30, 4);
	memcpy(&mD, destD + 0x30, 4);
	oracleDigest = nxFold(oracleDigest, bd);
	printf("boxmass row=phys_fn_000849 mass_scaled=%08x mass_unit=%08x digest=%08x\n",
		mC, mD, bd);
	}

	// -----------------------------------------------------------------------
	// CAPSULE slot 4, phys_fn_000853: the compute-mass row. Drive one:
	// axisSelector 2 (axial on +0x20), radius 1.25, cylHalfHeight 2.0,
	// density 2.0f. Drive two: selector 0 (the other fully-written path),
	// same geometry, density exactly 1.0f. Selector 1 is not driven: its
	// path never writes +0x00, so the fold would compare uninitialised
	// memory on both sides.
	{
	typedef void (__thiscall* NxCapMassFn)(void* self, float density,
		unsigned axisSelector, float radius, float cylHalfHeight,
		const void* extra);
	NxCapMassFn capMassFn = (NxCapMassFn) (base + 0x0001c980);

	unsigned char destE[0x34];
	memset(destE, 0, sizeof(destE));
	capMassFn(destE, 2.0f, 2u, 1.25f, 2.0f, 0);

	unsigned char destF[0x34];
	memset(destF, 0, sizeof(destF));
	capMassFn(destF, 1.0f, 0u, 1.25f, 2.0f, 0);

	unsigned cd = 2166136261u;
	for(int v = 0; v < 2; ++v)
		{
		const unsigned char* src = (v == 0) ? destE : destF;
		for(int i = 0; i < 0x34; i += 4)
			{
			unsigned w;
			memcpy(&w, src + i, 4);
			cd = nxFold(cd, w);
			}
		}
	oCapMassDigest = cd;

	unsigned mE, mF;
	memcpy(&mE, destE + 0x30, 4);
	memcpy(&mF, destF + 0x30, 4);
	unsigned eI0, eI4, eI8, fI0, fI4, fI8;
	memcpy(&eI0, destE, 4);
	memcpy(&eI4, destE + 0x10, 4);
	memcpy(&eI8, destE + 0x20, 4);
	memcpy(&fI0, destF, 4);
	memcpy(&fI4, destF + 0x10, 4);
	memcpy(&fI8, destF + 0x20, 4);
	oracleDigest = nxFold(oracleDigest, cd);
	printf("capmass row=phys_fn_000853 e=%08x.%08x.%08x.%08x f=%08x.%08x.%08x.%08x digest=%08x\n",
		eI0, eI4, eI8, mE, fI0, fI4, fI8, mF, cd);
	}

	// -----------------------------------------------------------------------
	// phys_fn_000831 driven DIRECTLY: the payload fold step. Two crafted
	// (frame, payload) pairs -- full non-symmetric inertia, non-zero offset,
	// every payload field non-zero -- folded over all thirteen result words.
	// The mass word is known untouched by the decode; it rides along in the
	// fold anyway.
	{
	typedef void (__thiscall* NxFoldFn)(void* self, const void* payload);
	NxFoldFn foldFn = (NxFoldFn) (base + 0x0001bdc0);

	unsigned char frO[0x34];
	memset(frO, 0, sizeof(frO));
	unsigned char payO[0x24];
	memset(payO, 0, sizeof(payO));
	{
	const float fi[12] = { 1.5f, -0.5f, 0.25f, -0.5f, 2.0f, 0.75f,
		0.25f, 0.75f, 3.0f, 0.5f, -0.25f, 1.75f };
	const float fp[9] = { 1.5f, -2.0f, 0.75f, 2.0f, -0.5f, 1.25f,
		3.0f, -1.5f, 0.5f };
	memcpy(frO, fi, sizeof(fi));
	const float m7 = 7.0f;
	memcpy(frO + 0x30, &m7, 4);
	memcpy(payO, fp, sizeof(fp));
	}
	foldFn(frO, payO);

	unsigned char frP[0x34];
	memset(frP, 0, sizeof(frP));
	unsigned char payP[0x24];
	memset(payP, 0, sizeof(payP));
	{
	const float fi[12] = { -1.25f, 2.5f, 0.0f, 4.0f, 0.125f, -2.0f,
		0.5f, -0.75f, 1.0f, -3.0f, 0.5f, 2.25f };
	const float fp[9] = { -0.5f, 1.0f, 2.0f, 0.25f, 1.5f, -2.5f,
		-1.0f, 0.5f, 4.0f };
	memcpy(frP, fi, sizeof(fi));
	memcpy(frP + 0x30, fi + 9, 4);	// mass word distinct
	memcpy(payP, fp, sizeof(fp));
	}
	foldFn(frP, payP);

	unsigned pd = 2166136261u;
	for(int v = 0; v < 2; ++v)
		{
		const unsigned char* src = (v == 0) ? frO : frP;
		for(int i = 0; i < 0x34; i += 4)
			{
			unsigned w;
			memcpy(&w, src + i, 4);
			pd = nxFold(pd, w);
			}
		}
	oFoldDigest = pd;

	unsigned w00, w01;
	memcpy(&w00, frO, 4);
	memcpy(&w01, frP, 4);
	oracleDigest = nxFold(oracleDigest, pd);
	printf("paxis row=phys_fn_000831 s0=%08x q0=%08x digest=%08x\n", w00, w01, pd);
	}

	// -----------------------------------------------------------------------
	// Task 2 keystone: the error stream, exercised through sphere
	// setRadius's invalid-radius arm. The image reports through an indirect
	// cdecl five-arg call at [base+0x1041b4], guarded by a non-zero flag
	// word behind [base+0x1041b0]. We install a capture sink over the slot,
	// drive radius=-1 (report fires) and radius=2.5 (silent), restore the
	// slot, and fold kind/line/code plus both literal strings.
	{
	typedef void (__thiscall* NxSetRadFn)(void* self, float radius);
	NxSetRadFn setRadFn = (NxSetRadFn) (base + 0x000278c0);

	unsigned* slotPtr = (unsigned*) (base + 0x001041b4);
	unsigned* guardPtrPtr = (unsigned*) (base + 0x001041b0);	memset(&g_errCap, 0, sizeof(g_errCap));
	typedef void(__cdecl* NxReportFnO)(int, const char*, int, int,
		const char*);
	NxReportFnO savedSink = reinterpret_cast<NxReportFnO>(*slotPtr);

	// The slot and its guard live on .rdata pages -- read-only. Flip the
	// page while we patch; restore when the drives are done. (The allocator
	// shim never needed this: it writes through its pointer into heap.)
	DWORD oldProtect = 0;
	if(!VirtualProtect(slotPtr, 8, PAGE_READWRITE, &oldProtect))
		return nxFail("errstream: VirtualProtect over the report slot failed");

	if(*guardPtrPtr != 0 && *reinterpret_cast<unsigned*>(*guardPtrPtr) == 0)
		*reinterpret_cast<unsigned*>(*guardPtrPtr) = 1;	// satisfy the assert

	unsigned char sphE[0xe4];
	memset(sphE, 0xcd, sizeof(sphE));
	typedef void (__thiscall* NxCtorFnER)(void*, void*, unsigned);
	NxCtorFnER ctorER = (NxCtorFnER) (base + 0x000277c0);
	ctorER(sphE, 0, 0);

	// The shipped reporter itself is FATAL when no user stream is installed
	// -- that is what the zero flag word guards -- so the sink must be ours
	// before any invalid-radius drive. Both writes above sit on .rdata
	// pages: flip protection while we patch, restore after.
	*slotPtr = reinterpret_cast<unsigned>(&g_errSink);
	setRadFn(sphE, -1.0f);
	int firedAfterInvalid = g_errCap.fired;
	setRadFn(sphE, 2.5f);
	int firedAfterValid = g_errCap.fired;

	*slotPtr = reinterpret_cast<unsigned>(savedSink);
	VirtualProtect(slotPtr, 8, oldProtect, &oldProtect);

	unsigned ed = nxFoldErrCap(2166136261u);
	oErrDigest = ed;

	oracleDigest = nxFold(oracleDigest, ed);
	printf("errstream row=phys_fn_001357 invalid_fires=%u valid_fires=%u digest=%08x\n",
		firedAfterInvalid, firedAfterValid - firedAfterInvalid, ed);
	}

	// -----------------------------------------------------------------------
	// The group-validation arm of applyGroup (phys_fn_001329): drive
	// group=0xFF -- report fires, the store is SKIPPED, yet the dirty-flag
	// and mask paths still run. Fold captures kind/line/code/strings plus
	// the post-drive +0xd8 halfword pair and the +0xc8 prunable-mask dword.
	{
	typedef void (__thiscall* NxSetGrpFn)(void* self, unsigned short g);
	NxSetGrpFn setGrpFn = (NxSetGrpFn) (base + 0x00026d90);

	unsigned* slotPtrG = (unsigned*) (base + 0x001041b4);
	unsigned* guardPtrPtrG = (unsigned*) (base + 0x001041b0);

	memset(&g_errCap, 0, sizeof(g_errCap));
	typedef void(__cdecl* NxReportFnO)(int, const char*, int, int,
		const char*);
	DWORD oldProtectG = 0;
	if(!VirtualProtect(slotPtrG, 8, PAGE_READWRITE, &oldProtectG))
		return nxFail("grouperr: VirtualProtect over the report slot failed");
	if(*guardPtrPtrG != 0 && *reinterpret_cast<unsigned*>(*guardPtrPtrG) == 0)
		*reinterpret_cast<unsigned*>(*guardPtrPtrG) = 1;

	unsigned char sphG[0xe4];
	memset(sphG, 0xcd, sizeof(sphG));
	typedef void (__thiscall* NxCtorFnEG)(void*, void*, unsigned);
	NxCtorFnEG ctorEG = (NxCtorFnEG) (base + 0x000277c0);
	ctorEG(sphG, 0, 0);

	NxReportFnO savedSinkG = reinterpret_cast<NxReportFnO>(*slotPtrG);
	*slotPtrG = reinterpret_cast<unsigned>(&g_errSink);
	setGrpFn(sphG, 0xFFu);
	int groupFiredInvalid = g_errCap.fired;
	setGrpFn(sphG, 5u);					// a valid drive on top
	*slotPtrG = reinterpret_cast<unsigned>(savedSinkG);
	VirtualProtect(slotPtrG, 8, oldProtectG, &oldProtectG);

	unsigned gd = nxFoldErrCap(2166136261u);
	unsigned d8word, c8mask;
	memcpy(&d8word, sphG + 0xd8, 4);
	memcpy(&c8mask, sphG + 0xc8, 4);
	gd = nxFold(gd, d8word);
	gd = nxFold(gd, c8mask);
	oGroupErrDigest = gd;

	oracleDigest = nxFold(oracleDigest, gd);
	printf("grouperr row=phys_fn_001329 invalid_fires=%u d8=%08x c8=%08x digest=%08x\n",
		groupFiredInvalid, d8word, c8mask, gd);
	}

	// -----------------------------------------------------------------------
	// The loadFromDesc validation arms: an invalid radius is STORED
	// unconditionally AND reported, then the BASE apply-desc tail runs
	// anyway. Sphere drives radius=-1 through 0x27850; capsule drives it
	// through 0x21ad0 (which also halves height into +0xe4). Fold captures
	// plus each shape's post-drive data words.
	{
	typedef void (__thiscall* NxSphLoadFn)(void* self, const void* rec);
	NxSphLoadFn sphLoadErrFn = (NxSphLoadFn) (base + 0x00027850);
	typedef void (__thiscall* NxCapLoadFn)(void* self, const void* rec);
	NxCapLoadFn capLoadErrFn = (NxCapLoadFn) (base + 0x00021ad0);

	unsigned* slotPtrL = (unsigned*) (base + 0x001041b4);
	unsigned* guardPtrPtrL = (unsigned*) (base + 0x001041b0);

	memset(&g_errCap, 0, sizeof(g_errCap));
	typedef void(__cdecl* NxReportFnO)(int, const char*, int, int,
		const char*);
	DWORD oldProtectL = 0;
	if(!VirtualProtect(slotPtrL, 8, PAGE_READWRITE, &oldProtectL))
		return nxFail("loaderr: VirtualProtect over the report slot failed");
	if(*guardPtrPtrL != 0 && *reinterpret_cast<unsigned*>(*guardPtrPtrL) == 0)
		*reinterpret_cast<unsigned*>(*guardPtrPtrL) = 1;
	NxReportFnO savedSinkL = reinterpret_cast<NxReportFnO>(*slotPtrL);
	*slotPtrL = reinterpret_cast<unsigned>(&g_errSink);

	unsigned char sphL[0xe4];
	memset(sphL, 0xcd, sizeof(sphL));
	typedef void (__thiscall* NxCtorFnEL)(void*, void*, unsigned);
	NxCtorFnEL ctorEL = (NxCtorFnEL) (base + 0x000277c0);
	ctorEL(sphL, 0, 0);

	const float negOne = -1.0f;
	unsigned char recS[0x58];
	memset(recS, 0, sizeof(recS));
	memcpy(recS + 0x4c, &negOne, 4);
	sphLoadErrFn(sphL, recS);
	int sphereFired = g_errCap.fired;

	unsigned char capL[0xec];
	memset(capL, 0xcd, sizeof(capL));
	typedef void (__thiscall* NxCtorFnCL)(void*, void*, unsigned);
	NxCtorFnCL ctorCL = (NxCtorFnCL) (base + 0x00021a60);
	ctorCL(capL, 0, 0);

	unsigned char recC[0x58];
	memset(recC, 0, sizeof(recC));
	memcpy(recC + 0x4c, &negOne, 4);
	capLoadErrFn(capL, recC);
	int capsFired = g_errCap.fired - sphereFired;

	*slotPtrL = reinterpret_cast<unsigned>(savedSinkL);
	VirtualProtect(slotPtrL, 8, oldProtectL, &oldProtectL);

	unsigned ld = nxFoldErrCap(2166136261u);
	unsigned sphRad, capRad, capHH;
	memcpy(&sphRad, sphL + 0xe0, 4);
	memcpy(&capRad, capL + 0xe0, 4);
	memcpy(&capHH, capL + 0xe4, 4);
	ld = nxFold(ld, sphRad);
	ld = nxFold(ld, capRad);
	ld = nxFold(ld, capHH);
	oLoadErrDigest = ld;

	oracleDigest = nxFold(oracleDigest, ld);
	printf("loaderr row=sphere+capsule fires=%u/%u rad=%08x.%08x hh=%08x digest=%08x\n",
		sphereFired, capsFired, sphRad, capRad, capHH, ld);
	}

	// -----------------------------------------------------------------------
	// Task 4: owned-arm registration through a FAKE scene. Container header
	// uses exact-offset dword slots: registrar vector at +0x90/94/98, four
	// bookkeeping vectors at +0x00/04/08, +0x10/14/18, +0x20/24/28,
	// +0x30/34/38 -- every count 8 > slot 3, so all paths stay in-place.
	{
	static unsigned arrSent[64];
	static unsigned arrCntB[64];
	static unsigned arrMirror[64];
	static unsigned arrExtra[64];
	static unsigned arrShapes[64];
	for(int i = 0; i < 8; ++i)
		{
		arrSent[i] = 0x11111111u;
		arrCntB[i] = 0x22222222u;
		arrMirror[i] = 0x33333333u;
		arrExtra[i] = 0x55555555u;
		arrShapes[i] = 0x44444444u;
		}
	unsigned hdr[64];
	memset(hdr, 0, sizeof(hdr));
	hdr[0x00 / 4] = reinterpret_cast<unsigned>(arrSent);
	hdr[0x04 / 4] = reinterpret_cast<unsigned>(arrSent + 8);
	hdr[0x08 / 4] = reinterpret_cast<unsigned>(arrSent + 64);
	hdr[0x10 / 4] = reinterpret_cast<unsigned>(arrCntB);
	hdr[0x14 / 4] = reinterpret_cast<unsigned>(arrCntB + 8);
	hdr[0x18 / 4] = reinterpret_cast<unsigned>(arrCntB + 64);
	hdr[0x20 / 4] = reinterpret_cast<unsigned>(arrMirror);
	hdr[0x24 / 4] = reinterpret_cast<unsigned>(arrMirror + 8);
	hdr[0x28 / 4] = reinterpret_cast<unsigned>(arrMirror + 64);
	hdr[0x30 / 4] = reinterpret_cast<unsigned>(arrExtra);
	hdr[0x34 / 4] = reinterpret_cast<unsigned>(arrExtra + 8);
	hdr[0x38 / 4] = reinterpret_cast<unsigned>(arrExtra + 64);
	hdr[0x90 / 4] = reinterpret_cast<unsigned>(arrShapes);
	hdr[0x94 / 4] = reinterpret_cast<unsigned>(arrShapes + 8);
	hdr[0x98 / 4] = reinterpret_cast<unsigned>(arrShapes + 64);

	unsigned fakeScene[32];
	memset(fakeScene, 0, sizeof(fakeScene));
	fakeScene[0x48 / 4] = reinterpret_cast<unsigned>(hdr);
	unsigned fakeOwner[4];
	memset(fakeOwner, 0, sizeof(fakeOwner));
	fakeOwner[1] = reinterpret_cast<unsigned>(fakeScene);

	const unsigned SLOT = 3;
	typedef void (__thiscall* NxCtorFnFO)(void*, void*, unsigned);
	NxCtorFnFO ctorFO = (NxCtorFnFO) (base + 0x000277c0);
	unsigned char sphO[0xe4];
	memset(sphO, 0xcd, sizeof(sphO));
	ctorFO(sphO, fakeOwner, SLOT);

	unsigned d4o = 0, shpO = 0, sentO = 0, mirO = 0;
	memcpy(&d4o, sphO + 0xd4, 4);
	shpO = arrShapes[SLOT];
	sentO = arrSent[SLOT];
	mirO = arrMirror[SLOT];

	unsigned od = 2166136261u;
	od = nxFold(od, d4o);
	od = nxFold(od, sentO == 0xFFFFFFFFu ? 1u : 0u);
	od = nxFold(od, mirO == 8u ? 1u : 0u);
	od = nxFold(od, (shpO != 0x44444444u && shpO != 0u) ? 1u : 0u);
	oOwnDigest = od;
	oracleDigest = nxFold(oracleDigest, od);
	printf("ownctor row=oracle d4=%08x sent_ok=%u mirror_ok=%u shp_ok=%u digest=%08x\n",
		d4o,
		sentO == 0xFFFFFFFFu ? 1u : 0u,
		mirO == 8u ? 1u : 0u,
		(shpO != 0x44444444u && shpO != 0u) ? 1u : 0u, od);
	}

	// -----------------------------------------------------------------------
	// Task 4: owned-DTOR deregistration. Register a sphere into a fake
	// scene (ctor writes shapes[slot], sentinel -1, and the v1 count into
	// mirror[slot]), then scalar-delete it -- the base dtor's owner arms
	// must clear the registration across all containers. Structural fold:
	// poison word, cleared slots, count pop, swap-move, freelist untouched
	// (sentinel -1 suppresses it), pair compaction, slot freepush, dirty
	// flag.
	{
	static unsigned dSent[64];
	static unsigned dCntA[64];
	static unsigned dCntB[64];
	static unsigned* dCntBEnd = dCntB + 8;
	static unsigned dMir[64];
	static unsigned dFlArr[64];
	static unsigned* dFlBegin = dFlArr;
	static unsigned* dFlEndCur = dFlArr;
	static unsigned dShapes[64];
	static unsigned dHdr[64];
	static unsigned dPairs[16];
	static unsigned dPairHdr[16];
	static unsigned dSlotFreeArr[64];
	static unsigned dSlotHdr[8];
	for(int i = 0; i < 64; ++i)
		{
		dSent[i] = 0u;
		dCntA[i] = 0xA0000000u + static_cast<unsigned>(i);
		dCntB[i] = 0u;
		dMir[i] = static_cast<unsigned>(i);
		dShapes[i] = 0u;
		}
	dCntB[7] = 7u;

	unsigned fakeSceneD2[512];
	memset(fakeSceneD2, 0, sizeof(fakeSceneD2));
	unsigned fakeOwnerD2[4];
	memset(fakeOwnerD2, 0, sizeof(fakeOwnerD2));

	typedef void (__thiscall* NxCtorFnOD)(void*, void*, unsigned);
	NxCtorFnOD ctorOD = (NxCtorFnOD) (base + 0x000277c0);
	typedef void (__thiscall* NxDtorFnOD)(void*, unsigned);
	NxDtorFnOD dtorOD = (NxDtorFnOD) (base + 0x00027c30);
	const unsigned SLOT_OD = 3;

	fakeSceneD2[0x48 / 4] = reinterpret_cast<unsigned>(dHdr);
	fakeSceneD2[0x5d4 / 4] = reinterpret_cast<unsigned>(dPairHdr);
	fakeSceneD2[0x6e4 / 4] = reinterpret_cast<unsigned>(dSlotHdr);
	fakeOwnerD2[1] = reinterpret_cast<unsigned>(fakeSceneD2);

	dHdr[0x00 / 4] = reinterpret_cast<unsigned>(dSent);
	dHdr[0x04 / 4] = reinterpret_cast<unsigned>(dSent + 8);
	dHdr[0x08 / 4] = reinterpret_cast<unsigned>(dSent + 64);
	dHdr[0x10 / 4] = reinterpret_cast<unsigned>(dCntA);
	dHdr[0x14 / 4] = reinterpret_cast<unsigned>(dCntBEnd);
	dHdr[0x18 / 4] = reinterpret_cast<unsigned>(dCntB + 64);
	dHdr[0x20 / 4] = reinterpret_cast<unsigned>(dMir);
	dHdr[0x24 / 4] = reinterpret_cast<unsigned>(dMir + 64);
	dHdr[0x28 / 4] = reinterpret_cast<unsigned>(dMir + 64);
	dHdr[0x30 / 4] = reinterpret_cast<unsigned>(dFlArr);
	dHdr[0x34 / 4] = reinterpret_cast<unsigned>(dFlEndCur);
	dHdr[0x38 / 4] = reinterpret_cast<unsigned>(dFlArr + 64);
	dHdr[0x90 / 4] = reinterpret_cast<unsigned>(dShapes);
	dHdr[0x94 / 4] = reinterpret_cast<unsigned>(dShapes + 64);
	dHdr[0x98 / 4] = reinterpret_cast<unsigned>(dShapes + 64);

	unsigned char sphD2[0xe4];
	memset(sphD2, 0xcd, sizeof(sphD2));
	ctorOD(sphD2, fakeOwnerD2, SLOT_OD);
	unsigned selfAddr = reinterpret_cast<unsigned>(sphD2);

	// one self-referencing pair plus two unrelated ones
	dPairs[0] = selfAddr;			dPairs[1] = 0xDEAD0001u;
	dPairs[2] = 0xDEAD0002u;		dPairs[3] = 0xDEAD0003u;
	dPairs[4] = 0xDEAD0004u;		dPairs[5] = 0xDEAD0005u;
	dPairHdr[0x00 / 4] = reinterpret_cast<unsigned>(dPairs);
	dPairHdr[0x04 / 4] = reinterpret_cast<unsigned>(dPairs + 12);

	// remover #3: free-list cursor (+0x08) below its limit (+0x0c)
	dSlotHdr[0x08 / 4] = reinterpret_cast<unsigned>(dSlotFreeArr + 2);
	dSlotHdr[0x0c / 4] = reinterpret_cast<unsigned>(dSlotFreeArr + 40);

	dtorOD(sphD2, 0);

	// --- structural verification ---
	unsigned sceneFlag = *reinterpret_cast<unsigned*>(
		reinterpret_cast<unsigned char*>(fakeSceneD2) + 0x70c);
	int shapesCleared = (dShapes[SLOT_OD] == 0);
	int sentZeroed = (dSent[SLOT_OD] == 0);
	int poisoned = (dMir[SLOT_OD] == 0xD00BEED0u);
	int cntPopped = (*reinterpret_cast<unsigned*>(dHdr[0x14 / 4]) ==
		reinterpret_cast<unsigned>(dCntBEnd - 4));
	int cntAMoved = (dCntA[SLOT_OD] == 7u && dMir[7] == 8u);
	int flUntouched = (dFlEndCur == dFlArr);
	int pairsCompacted = (dPairs[0] == 0xDEAD0004u &&
		dPairs[1] == 0xDEAD0005u &&
		*reinterpret_cast<unsigned*>(dPairHdr[0x04 / 4]) ==
			reinterpret_cast<unsigned>(dPairs + 8));
	int slotFreed = (dSlotFreeArr[2] == SLOT_OD &&
		dSlotHdr[0x08 / 4] == reinterpret_cast<unsigned>(dSlotFreeArr + 12));

	unsigned dd = 2166136261u;
	dd = nxFold(dd, sceneFlag == 2u ? 1u : 0u);
	const int checks[] = { shapesCleared, sentZeroed, poisoned, cntPopped,
		cntAMoved, flUntouched, pairsCompacted, slotFreed };
	for(int k = 0; k < 8; ++k)
		dd = nxFold(dd, static_cast<unsigned>(checks[k]));
	oOwnDtorDigest = dd;

	oracleDigest = nxFold(oracleDigest, dd);
	printf("owndtor row=oracle flag=%08x clr=%u/%u/%u pop=%u mv=%u fl=%u pair=%u freed=%u digest=%08x\n",
		sceneFlag, shapesCleared, sentZeroed, poisoned, cntPopped,
		cntAMoved, flUntouched, pairsCompacted, slotFreed, dd);
	}

	// -----------------------------------------------------------------------
	// phys_fn_000028: dword-vector push_back driven through both reallocs.
	// Eight pushes from an empty VC9 header -- p0 allocates capacity 2,
	// p2 grows 2 -> 6, p6 grows 6 -> 14 (new capacity 2*size + 2 dwords
	// through the shim's arena) -- then a second drive pushes once into a
	// pre-reserved header and must not touch the heap at all. The proxy word
	// is folded untouched because this row never reads it.
	{
	unsigned vecO[8];
	memset(vecO, 0xcd, sizeof(vecO));
	vecO[0] = 0xC0C0C0C0u;					// _Myproxy marker
	vecO[1] = 0;							// _Myfirst: empty
	vecO[2] = 0;							// _Mylast
	vecO[3] = 0;							// _Myend
	typedef void (__thiscall* NxPushFnOD)(void* self, unsigned value);
	NxPushFnOD pushOD = (NxPushFnOD) (base + 0x00001b90);

	NxHeapMark mgD = nxHeapMarkNow();
	for(unsigned k = 0; k < 8; ++k)
		pushOD(vecO, 0x51510000u + k);
	int proxyUntouched = (vecO[0] == 0xC0C0C0C0u);
	const unsigned* vecElemsO = reinterpret_cast<const unsigned*>(vecO[1]);
	unsigned cnt = (vecO[2] - vecO[1]) >> 2;
	unsigned cap = (vecO[3] - vecO[1]) >> 2;
	int elemsOk = (cnt == 8);
	for(unsigned k = 0; k < cnt && k < 8; ++k)
		if(vecElemsO[k] != 0x51510000u + k)
			elemsOk = 0;

	unsigned dv = 2166136261u;
	dv = nxFold(dv, proxyUntouched ? 1u : 0u);
	dv = nxFold(dv, cnt);
	dv = nxFold(dv, cap);
	for(unsigned k = 0; k < cnt && k < 8; ++k)
		dv = nxFold(dv, vecElemsO[k]);
	dv = nxFoldHeapDelta(dv, mgD);
	oVecGrowDigest = dv;
	oracleDigest = nxFold(oracleDigest, dv);
	printf("vecgrow row=oracle proxy=%08x count=%u cap=%u elems=%u mops=%u mbytes=%u fops=%u fbytes=%u digest=%08x\n",
		vecO[0], cnt, cap, elemsOk,
		g_heapMallocOps - mgD.mo, g_heapMallocBytes - mgD.mb,
		g_heapFreeOps - mgD.fo, g_heapFreeBytes - mgD.fb, dv);

	static unsigned rsvStoreO[16];
	unsigned rsvO[8];
	memset(rsvO, 0, sizeof(rsvO));
	rsvO[1] = reinterpret_cast<unsigned>(rsvStoreO);
	rsvO[2] = reinterpret_cast<unsigned>(rsvStoreO + 1);
	rsvO[3] = reinterpret_cast<unsigned>(rsvStoreO + 16);
	NxHeapMark mrD = nxHeapMarkNow();
	pushOD(rsvO, 0x5E5E0001u);
	int rsvNoAlloc = (g_heapMallocOps == mrD.mo && g_heapFreeOps == mrD.fo &&
		rsvO[2] == reinterpret_cast<unsigned>(rsvStoreO + 2));
	int rsvStored = (rsvStoreO[1] == 0x5E5E0001u);
	unsigned dr = 2166136261u;
	dr = nxFold(dr, rsvNoAlloc ? 1u : 0u);
	dr = nxFold(dr, rsvStored ? 1u : 0u);
	oVecGrowReservedDigest = dr;
	oracleDigest = nxFold(oracleDigest, dr);
	printf("vecgrow2 row=oracle noalloc=%u stored=%u digest=%08x\n",
		rsvNoAlloc, rsvStored, dr);
	}

	// -----------------------------------------------------------------------
	// phys_fn_002410 direct: the release arm with the free vector AT
	// capacity, so its inlined push_back must realloc mid-release and leave
	// the unlink intact behind it. Three indices cover the real arm
	// structure -- sentinel != -1 gates ONLY the push, sentinel == 0 gates
	// only the unlink:
	//   idx 3 live (0xA5A50003): push grows 8 -> 18, then unlink pops.
	//   idx 5 released (0):      duplicate push, no unlink.
	//   idx 7 virgin (-1):       NO push, but the unlink still runs --
	//                            cntA[3] is rewritten to 6 and the cursor
	//                            pops a second time. The quirk this family
	//                            exists to pin.
	//
	// The initial free-vector block comes FROM the emulator arena, not from
	// a static array: growth releases the old block through the shim, and a
	// foreign block turns that release into a silent no-op whose offset
	// arithmetic is only guarded by luck.
	{
	typedef void (__thiscall* NxRelFnOD)(void* self, unsigned idx);
	NxRelFnOD relOD = (NxRelFnOD) (base + 0x0005bac0);

	static unsigned rSent[64];
	static unsigned rCntA[64];
	static unsigned rCntB[64];
	static unsigned* rCntBEnd = rCntB + 8;
	static unsigned rMir[64];
	static unsigned rHdr[64];
	memset(rHdr, 0, sizeof(rHdr));
	for(int i = 0; i < 64; ++i)
		{
		rSent[i] = 0xFFFFFFFFu;
		rCntA[i] = 0xB0000000u + static_cast<unsigned>(i);
		rMir[i] = static_cast<unsigned>(i);
		}
	rCntB[7] = 7u;
	rSent[3] = 0xA5A50003u;					// live
	rSent[5] = 0u;							// released once already

	unsigned* rFl = static_cast<unsigned*>(nxHeapAlloc(8 * sizeof(unsigned)));
	for(int i = 0; i < 8; ++i)
		rFl[i] = 0x11110000u + static_cast<unsigned>(i);

	rHdr[0x00 / 4] = reinterpret_cast<unsigned>(rSent);
	rHdr[0x10 / 4] = reinterpret_cast<unsigned>(rCntA);
	rHdr[0x14 / 4] = reinterpret_cast<unsigned>(rCntBEnd);
	rHdr[0x20 / 4] = reinterpret_cast<unsigned>(rMir);
	rHdr[0x30 / 4] = reinterpret_cast<unsigned>(rFl);
	rHdr[0x34 / 4] = reinterpret_cast<unsigned>(rFl + 8);
	rHdr[0x38 / 4] = reinterpret_cast<unsigned>(rFl + 8);

	NxHeapMark mhD = nxHeapMarkNow();
	relOD(rHdr, 3);							// grow + unlink
	relOD(rHdr, 5);							// duplicate push only
	NxHeapMark mAfterDup = nxHeapMarkNow();
	relOD(rHdr, 7);							// virgin: no push, unlink runs

	int virginNoPush = (g_heapMallocOps == mAfterDup.mo &&
		g_heapFreeOps == mAfterDup.fo);
	int sent3Zeroed = (rSent[3] == 0u && rSent[7] == 0u);
	int mirPoisoned = (rMir[3] == 0xD00BEED0u && rMir[7] == 0xD00BEED0u);
	int secondUnlinkMoved = (rCntA[3] == 0u && rMir[0] == 3u &&
		*reinterpret_cast<unsigned**>(reinterpret_cast<unsigned char*>(rHdr) + 0x14)
			== rCntB + 6);
	unsigned flCount = (rHdr[0x34 / 4] - rHdr[0x30 / 4]) >> 2;
	unsigned flCap = (rHdr[0x38 / 4] - rHdr[0x30 / 4]) >> 2;
	const unsigned* flLive = *reinterpret_cast<unsigned* const*>(
		reinterpret_cast<unsigned char*>(rHdr) + 0x30);
	int dupPresent = (flCount >= 10 && flLive[9] == 5u);

	unsigned dq = 2166136261u;
	dq = nxFold(dq, virginNoPush ? 1u : 0u);
	dq = nxFold(dq, sent3Zeroed ? 1u : 0u);
	dq = nxFold(dq, mirPoisoned ? 1u : 0u);
	dq = nxFold(dq, secondUnlinkMoved ? 1u : 0u);
	dq = nxFold(dq, flCount);
	dq = nxFold(dq, flCap);
	for(unsigned k = 0; k < flCount && k < 18; ++k)
		dq = nxFold(dq, flLive[k]);
	dq = nxFold(dq, dupPresent ? 1u : 0u);
	dq = nxFoldHeapDelta(dq, mhD);
	oRelGrowDigest = dq;
	oracleDigest = nxFold(oracleDigest, dq);
	printf("relgrow row=oracle nopush=%u s37zero=%u poison=%u mv2=%u fl=%u/%u dup=%u mops=%u mbytes=%u fops=%u fbytes=%u digest=%08x\n",
		virginNoPush, sent3Zeroed, mirPoisoned, secondUnlinkMoved,
		flCount, flCap, dupPresent,
		g_heapMallocOps - mhD.mo, g_heapMallocBytes - mhD.mb,
		g_heapFreeOps - mhD.fo, g_heapFreeBytes - mhD.fb, dq);
	}

	// -----------------------------------------------------------------------
	// phys_fn_002344 direct: pair-list swap-remove over five sub-drives --
	// middle removal (swap-with-last moves real words), matched-last
	// (shrink without copy), duplicate matches in one call (the loop
	// re-scans the slot a swap just filled), no match, empty list.
	//
	// Contract from the call site (0x00026c13): `this` is the address of
	// the scene's +0x5d4 FIELD; *[this] names the {begin,end} header of
	// the stride-8 array. The drives reproduce both levels.
	{
	typedef void (__thiscall* NxPairRmFnOD)(void* self, unsigned value);
	NxPairRmFnOD pairRmOD = (NxPairRmFnOD) (base + 0x0005aae0);

	static unsigned pwO[5][16];				// five pair arrays
	static unsigned phO[5][4];				// {begin, end} each
	static unsigned pcO[5];					// *[this] -> header
	memset(pwO, 0, sizeof(pwO));
	memset(phO, 0, sizeof(phO));

	for(int cs = 0; cs < 5; ++cs)
		pcO[cs] = reinterpret_cast<unsigned>(phO[cs]);

	// c0: {(a,b),(c,d),(e,f)} remove c -- the LAST pair swaps into slot 1.
	pwO[0][0] = 0x7A010001u; pwO[0][1] = 0x7A010002u;
	pwO[0][2] = 0x7A020001u; pwO[0][3] = 0x7A020002u;
	pwO[0][4] = 0x7A030001u; pwO[0][5] = 0x7A030002u;
	phO[0][0] = reinterpret_cast<unsigned>(pwO[0]);
	phO[0][1] = reinterpret_cast<unsigned>(pwO[0] + 6);
	pairRmOD(&pcO[0], 0x7A020001u);

	// c1: remove by SECOND half at matched-last: shrink with no move.
	pwO[1][0] = 0x7B110001u; pwO[1][1] = 0x7B110002u;
	pwO[1][2] = 0x7B120001u; pwO[1][3] = 0x7B120002u;
	phO[1][0] = reinterpret_cast<unsigned>(pwO[1]);
	phO[1][1] = reinterpret_cast<unsigned>(pwO[1] + 4);
	pairRmOD(&pcO[1], 0x7B120002u);

	// c2: duplicates -- two records share an exact first-half value; one
	// call removes both through the swap chain and leaves the unrelated
	// survivor intact at slot 0 (the loop rescans the slot a swap just
	// filled, which is what makes duplicates fall in one pass).
	const unsigned DUP_V = 0x7C220000u;
	pwO[2][0] = DUP_V;			pwO[2][1] = 0x7C200002u;
	pwO[2][2] = 0x7C210001u;	pwO[2][3] = 0x7C210002u;
	pwO[2][4] = DUP_V;			pwO[2][5] = 0x7C200004u;
	phO[2][0] = reinterpret_cast<unsigned>(pwO[2]);
	phO[2][1] = reinterpret_cast<unsigned>(pwO[2] + 6);
	pairRmOD(&pcO[2], DUP_V);

	// c3: no match -- nothing moves.
	pwO[3][0] = 0x7D310001u; pwO[3][1] = 0x7D310002u;
	phO[3][0] = reinterpret_cast<unsigned>(pwO[3]);
	phO[3][1] = reinterpret_cast<unsigned>(pwO[3] + 2);
	pairRmOD(&pcO[3], 0xDEADBEEFu);

	// c4: empty list -- begin == end, the loop never runs.
	phO[4][0] = reinterpret_cast<unsigned>(pwO[4]);
	phO[4][1] = reinterpret_cast<unsigned>(pwO[4]);
	pairRmOD(&pcO[4], 0x7A010001u);

	unsigned dp = 2166136261u;
	for(int cs = 0; cs < 5; ++cs)
		{
		dp = nxFold(dp, phO[cs][1] - phO[cs][0]);	// end offset rel begin
		for(int w = 0; w < 16; ++w)
			dp = nxFold(dp, pwO[cs][w]);
		}
	oPairRmDigest = dp;
	oracleDigest = nxFold(oracleDigest, dp);
	printf("pairrm row=oracle cases=5 digest=%08x\n", dp);
	}

	// -----------------------------------------------------------------------
	// Task 4, actor small slots: guarded body reads driven on a fake actor
	// {+0x10 scene lock context, +0x14 body} with marked fields. The guards
	// run real kernel32 primitives over a real CRITICAL_SECTION, so both
	// sides execute identical locking; the fold covers every return value,
	// the negative arms through their own sub-actors, and the lock block's
	// writer flag and tid after the drives.
	{
	typedef bool (__thiscall* NxActorBoolFn)(void* self);
	typedef bool (__thiscall* NxActorMaskFn)(void* self, unsigned mask);
	typedef unsigned short (__thiscall* NxActorWordFn)(void* self);
	typedef float (__thiscall* NxActorFloatFn)(void* self);
	typedef unsigned (__thiscall* NxActorUintFn)(void* self);
	typedef void* (__thiscall* NxActorPtrFn)(void* self);
	NxActorBoolFn hasBodyOD = (NxActorBoolFn)(base + 0x0003580);	// slot 19
	NxActorWordFn grpWordOD = (NxActorWordFn)(base + 0x0003610);	// slot 86
	NxActorMaskFn flagsMaskOD = (NxActorMaskFn)(base + 0x0002c60);	// slot 77
	NxActorFloatFn sqrtD0OD = (NxActorFloatFn)(base + 0x00029e0);	// slot 69
	NxActorFloatFn sqrtD4OD = (NxActorFloatFn)(base + 0x0002a30);	// slot 71
	NxActorUintFn recCountOD = (NxActorUintFn)(base + 0x0002d00);	// slot 15
	NxActorPtrFn colObjOD = (NxActorPtrFn)(base + 0x0002d30);		// slot 16
	NxActorPtrFn boundOD = (NxActorPtrFn)(base + 0x0002d60);		// slot 84

	static unsigned sCs[16];				// CS (6 words) + flag + tid
	static unsigned sScene[4];				// [0] -> CS
	static unsigned sBody[64];
	static unsigned sRec[0x40];				// [body+8]: the nested record
	static unsigned sShapeMesh[0x40];
	static unsigned sShapeBox[0x40];
	static unsigned sMeshArr[8];
	InitializeCriticalSection((LPCRITICAL_SECTION)sCs);
	sScene[0] = reinterpret_cast<unsigned>(sCs);
	memset(sBody, 0, sizeof(sBody));
	memset(sRec, 0, sizeof(sRec));
	memset(sShapeMesh, 0, sizeof(sShapeMesh));
	memset(sShapeBox, 0, sizeof(sShapeBox));
	sBody[2] = reinterpret_cast<unsigned>(sRec);	// +8: record pointer
	sBody[5] = 0x00000030u;					// +0x14: flag word
	sBody[7] = 0x0000BEEFu;					// +0x1c: group word
	sRec[0x34] = 0x41E8909Bu;				// +0xd0: sqrt input
	sRec[0x35] = 0x42F00000u;				// +0xd4: 120.0f
	sBody[4] = reinterpret_cast<unsigned>(sShapeMesh);
	sShapeMesh[0x34] = 5;					// +0xd0: NX_SHAPE_MESH
	sShapeMesh[0x38] = reinterpret_cast<unsigned>(sMeshArr);
	sShapeMesh[0x39] = reinterpret_cast<unsigned>(sMeshArr + 12);
	sShapeMesh[0x3c] = 0x0BADF00Du;			// +0xf0: hull pointer mark
	sShapeBox[0x34] = 0;					// non-mesh type

	unsigned char actO[0x20];
	memset(actO, 0xcd, sizeof(actO));
	unsigned* actFields = reinterpret_cast<unsigned*>(actO);
	actFields[4] = reinterpret_cast<unsigned>(sScene);		// +0x10
	actFields[5] = reinterpret_cast<unsigned>(sBody);		// +0x14

	unsigned da = 2166136261u;
	da = nxFold(da, hasBodyOD(actO) ? 1u : 0u);
	da = nxFold(da, grpWordOD(actO));
	da = nxFold(da, flagsMaskOD(actO, 0x10u) ? 1u : 0u);
	da = nxFold(da, flagsMaskOD(actO, 0xC0u) ? 1u : 0u);
	float s0v = sqrtD0OD(actO);
	float s4v = sqrtD4OD(actO);
	unsigned b0, b4;
	memcpy(&b0, &s0v, 4);
	memcpy(&b4, &s4v, 4);
	da = nxFold(da, b0);
	da = nxFold(da, b4);
	da = nxFold(da, recCountOD(actO));

	unsigned coMesh = reinterpret_cast<unsigned>(colObjOD(actO));
	da = nxFold(da, coMesh == 0x0BADF00Du ? 1u : 0u);

	// negative arms: a non-mesh shape and an absent shape list
	unsigned char actNegO[0x20];
	memset(actNegO, 0xcd, sizeof(actNegO));
	unsigned* negFields = reinterpret_cast<unsigned*>(actNegO);
	negFields[4] = reinterpret_cast<unsigned>(sScene);
	negFields[5] = reinterpret_cast<unsigned>(sBody);
	sBody[4] = reinterpret_cast<unsigned>(sShapeBox);
	da = nxFold(da, recCountOD(actNegO));
	unsigned boxBase = reinterpret_cast<unsigned>(sShapeBox);
	da = nxFold(da,
		reinterpret_cast<unsigned>(colObjOD(actNegO)) == boxBase + 0x9c
			? 1u : 0u);
	sBody[4] = 0;
	da = nxFold(da, recCountOD(actNegO));
	da = nxFold(da, colObjOD(actNegO) == nullptr ? 1u : 0u);

	// null-record arm: body present, [body+8] null -- the exact guard the
	// image carries -- returns an exact float zero
	unsigned char actNoBodyO[0x20];
	memset(actNoBodyO, 0xcd, sizeof(actNoBodyO));
	unsigned* nbFields = reinterpret_cast<unsigned*>(actNoBodyO);
	nbFields[4] = reinterpret_cast<unsigned>(sScene);
	nbFields[5] = reinterpret_cast<unsigned>(sBody);
	sBody[2] = 0;
	float nb = sqrtD0OD(actNoBodyO);
	unsigned nbBits;
	memcpy(&nbBits, &nb, 4);
	da = nxFold(da, nbBits);
	sBody[2] = reinterpret_cast<unsigned>(sRec);

	// SDK pointer binding keyed on the body: unbound, then bound through
	// each side's own setter (phys_fn_000480 is cdecl), then removed again.
	da = nxFold(da, boundOD(actO) == nullptr ? 1u : 0u);
	typedef int (__cdecl* NxBindSetFn)(void*, void*);
	NxBindSetFn bindSetOD = (NxBindSetFn)(base + 0x000edc0);
	bindSetOD(reinterpret_cast<void*>(0x13570001u),
		reinterpret_cast<void*>(0x5A5A1000u));
	da = nxFold(da, reinterpret_cast<unsigned>(boundOD(actO)) == 0x5A5A1000u
		? 1u : 0u);
	bindSetOD(reinterpret_cast<void*>(0x13570001u), nullptr);
	da = nxFold(da, boundOD(actO) == nullptr ? 1u : 0u);

	da = nxFold(da, sCs[6]);
	da = nxFold(da, sCs[7] != 0 ? 1u : 0u);	// tid is live state; pin only
	oActorsmDigest = da;					// that it was recorded
	oracleDigest = nxFold(oracleDigest, da);
	printf("actorsm row=oracle digest=%08x\n", da);
	}

	// -----------------------------------------------------------------------
	// phys_fn_000044 / 000118 / 000116 / 000042 / 002404 / 002406: the actor
	// construction and destruction trio plus the +8 adjustor thunk. The
	// destructors release through the emulator arena, so post-free vtable
	// words are read back deterministically and folded.
	{
	typedef void (__thiscall* NxCtor1Fn)(void* self, void* body);
	typedef void (__thiscall* NxDtor1Fn)(void* self, unsigned flags);
	NxCtor1Fn ctorOD = (NxCtor1Fn)(base + 0x0002480);		// 000044
	NxDtor1Fn delDtorOD = (NxDtor1Fn)(base + 0x0003650);	// 000118
	NxDtor1Fn adjDtorOD = (NxDtor1Fn)(base + 0x0003640);	// 000116 (+8)
	NxDtor1Fn wallDtorOD = (NxDtor1Fn)(base + 0x0002460);	// 000042

	unsigned fakeBodyMark = 0x0B0DF00Du;
	unsigned actCtorO[8];
	memset(actCtorO, 0xcd, sizeof(actCtorO));
	ctorOD(actCtorO, reinterpret_cast<void*>(fakeBodyMark));
	int ctVptr = (actCtorO[0] == 0x10104530u);
	int ctOwner = (actCtorO[1] == 0);
	int ctMember = (actCtorO[2] == 0x1010468cu);
	int ctMemberZeroed = (actCtorO[3] == 0 && actCtorO[4] == 0);
	int ctBody = (actCtorO[5] == fakeBodyMark);

	unsigned* dynA = static_cast<unsigned*>(
		nxHeapAlloc(sizeof(unsigned) * 8));
	for(int i = 0; i < 8; ++i)
		dynA[i] = 0xFEEDF00Du;
	NxHeapMark mdD = nxHeapMarkNow();
	delDtorOD(dynA, 1);
	int ddWall = (dynA[0] == 0x101043d0u);
	int ddMember = (dynA[2] == 0x101088b8u);
	int ddFreed = (g_heapFreeOps == mdD.fo + 1 &&
		g_heapMallocOps == mdD.mo);

	unsigned* dynB = static_cast<unsigned*>(
		nxHeapAlloc(sizeof(unsigned) * 8));
	for(int i = 0; i < 8; ++i)
		dynB[i] = 0xFEEDF00Du;
	adjDtorOD(dynB + 2, 1);					// this = actor + 8
	int adjWall = (dynB[0] == 0x101043d0u);
	int adjMember = (dynB[2] == 0x101088b8u);
	int adjFreed = (g_heapFreeOps == mdD.fo + 2);

	// phys_fn_000042 frees through ITS OWN linked CRT (0x0002471 ->
	// 0x100f41f0), not through the adapter -- hand it a block from that
	// same CRT's malloc (0x000f4722) and do not read past the free. The
	// flags=0 arm folds its vtable store from a stack buffer.
	typedef void* (__cdecl* NxCrtMallocFn)(unsigned);
	NxCrtMallocFn crtMallocOD = (NxCrtMallocFn)(base + 0x000f4722);
	unsigned* dynW = static_cast<unsigned*>(crtMallocOD(sizeof(unsigned) * 8));
	for(int i = 0; i < 8; ++i)
		dynW[i] = 0xFEEDF00Du;
	wallDtorOD(dynW, 1);
	int wFreed = 1;
	unsigned stackW[8];
	memset(stackW, 0xcd, sizeof(stackW));
	wallDtorOD(stackW, 0);
	int wWall = (stackW[0] == 0x101043d0u);
	int wUntouched = (stackW[2] == 0xCDCDCDCDu);
	int wNoFree = (g_heapFreeOps == mdD.fo + 2);

	unsigned dc = 2166136261u;
	const int checks[] = { ctVptr, ctOwner, ctMember, ctMemberZeroed,
		ctBody, ddWall, ddMember, ddFreed, adjWall, adjMember, adjFreed,
		wWall, wUntouched, wFreed };
	for(int k = 0; k < 14; ++k)
		dc = nxFold(dc, static_cast<unsigned>(checks[k]));
	dc = nxFoldHeapDelta(dc, mdD);
	oActorCtorDigest = dc;
	oracleDigest = nxFold(oracleDigest, dc);
	printf("actorctor row=oracle ct=%u/%u/%u/%u/%u dd=%u/%u/%u adj=%u/%u/%u w=%u/%u/%u digest=%08x\n",
		ctVptr, ctOwner, ctMember, ctMemberZeroed, ctBody,
		ddWall, ddMember, ddFreed, adjWall, adjMember, adjFreed,
		wWall, wUntouched, wFreed, dc);
	}

	// -----------------------------------------------------------------------
	// Actor slate 2: the record energy word (slot 62 over helper phys_fn_
	// 000742, direct-driven at its own address too), the +0x84 zero test
	// (slot 68), and the write-guard flag writers (slots 75/76) including
	// their deadlock-report arms -- a contended writer flag fires kind 2
	// through the error stream and leaves the flags untouched.
	{
	typedef float (__thiscall* NxRecEnergyFn)(void* rec);
	NxRecEnergyFn recEnergyOD = (NxRecEnergyFn)(base + 0x0016dd0);
	typedef float (__thiscall* NxActorFloatFn2)(void* self);
	typedef bool (__thiscall* NxActorBoolFn2)(void* self);
	typedef void (__thiscall* NxActorMaskFn2)(void* self, unsigned mask);
	NxActorFloatFn2 energyOD = (NxActorFloatFn2)(base + 0x0002900);	// slot 62
	NxActorBoolFn2 w84OD = (NxActorBoolFn2)(base + 0x0002990);		// slot 68
	NxActorMaskFn2 raiseOD = (NxActorMaskFn2)(base + 0x0002ba0);	// slot 75
	NxActorMaskFn2 clearOD = (NxActorMaskFn2)(base + 0x0002c00);	// slot 76

	static unsigned sR2Cs[16];
	static unsigned sR2Scene[4];
	static unsigned sR2Body[64];
	static unsigned sR2Rec[0x70];
	InitializeCriticalSection((LPCRITICAL_SECTION)sR2Cs);
	sR2Scene[0] = reinterpret_cast<unsigned>(sR2Cs);
	memset(sR2Body, 0, sizeof(sR2Body));
	memset(sR2Rec, 0, sizeof(sR2Rec));
	sR2Body[2] = reinterpret_cast<unsigned>(sR2Rec);
	sR2Body[5] = 0x00000030u;
	float v6c = 1.5f, v70 = 2.0f, v74 = 2.5f;
	float m78 = 3.0f, m7c = 4.0f, m80 = 5.0f;
	float v18c = 0.25f, v190 = 0.5f, v194 = 0.75f, m188 = 6.0f;
	memcpy(sR2Rec + 0x1b, &v6c, 4);
	memcpy(sR2Rec + 0x1c, &v70, 4);
	memcpy(sR2Rec + 0x1d, &v74, 4);
	memcpy(sR2Rec + 0x1e, &m78, 4);
	memcpy(sR2Rec + 0x1f, &m7c, 4);
	memcpy(sR2Rec + 0x20, &m80, 4);
	memcpy(sR2Rec + 0x62, &m188, 4);
	memcpy(sR2Rec + 0x63, &v18c, 4);
	memcpy(sR2Rec + 0x64, &v190, 4);
	memcpy(sR2Rec + 0x65, &v194, 4);

	unsigned char actE[0x20];
	memset(actE, 0xcd, sizeof(actE));
	unsigned* ef = reinterpret_cast<unsigned*>(actE);
	ef[3] = reinterpret_cast<unsigned>(sR2Scene);	// +0x0c member ctx
	ef[4] = reinterpret_cast<unsigned>(sR2Scene);	// +0x10 read ctx
	ef[5] = reinterpret_cast<unsigned>(sR2Body);

	unsigned d2 = 2166136261u;
	float eDirect = recEnergyOD(sR2Rec);
	float eViaSlot = energyOD(actE);
	unsigned eb0, eb1;
	memcpy(&eb0, &eDirect, 4);
	memcpy(&eb1, &eViaSlot, 4);
	d2 = nxFold(d2, eb0);
	d2 = nxFold(d2, eb1);
	d2 = nxFold(d2, w84OD(actE) ? 1u : 0u);
	sR2Rec[0x21] = 0x00000077u;				// +0x84 nonzero
	d2 = nxFold(d2, w84OD(actE) ? 1u : 0u);
	sR2Rec[0x21] = 0;

	raiseOD(actE, 0x40u);
	d2 = nxFold(d2, sR2Body[5]);
	clearOD(actE, 0x10u);
	d2 = nxFold(d2, sR2Body[5]);

	// contended arms: a writer flag held by another thread makes both rows
	// report kind 2 and skip the mutation entirely. The oracle reports
	// through [.rdata 0x101041b4] -- flip the page, install the capture
	// sink, satisfy the guard word, restore afterwards (the errstream
	// family's pattern).
	memset(&g_errCap, 0, sizeof(g_errCap));
	unsigned* slot2 = (unsigned*) (base + 0x001041b4);
	unsigned* guard2Ptr = (unsigned*) (base + 0x001041b0);
	typedef void(__cdecl* NxReportFnO2)(int, const char*, int, int,
		const char*);
	NxReportFnO2 savedSink2 = reinterpret_cast<NxReportFnO2>(*slot2);
	DWORD oldProt2 = 0;
	if(!VirtualProtect(slot2, 8, PAGE_READWRITE, &oldProt2))
		return nxFail("actorsm2: VirtualProtect over the report slot failed");
	if(*guard2Ptr != 0 && *reinterpret_cast<unsigned*>(*guard2Ptr) == 0)
		*reinterpret_cast<unsigned*>(*guard2Ptr) = 1;
	*slot2 = reinterpret_cast<unsigned>(&g_errSink);
	sR2Cs[6] = 1;
	sR2Cs[7] = ::GetCurrentThreadId() + 1u;
	raiseOD(actE, 0x40u);
	clearOD(actE, 0x10u);
	*slot2 = reinterpret_cast<unsigned>(savedSink2);
	VirtualProtect(slot2, 8, oldProt2, &oldProt2);
	d2 = nxFoldErrCap(d2);
	d2 = nxFold(d2, sR2Body[5]);			// unchanged 0x60
	d2 = nxFold(d2, sR2Cs[6]);				// foreign hold left standing

	oActorsm2Digest = d2;
	oracleDigest = nxFold(oracleDigest, d2);
	printf("actorsm2 row=oracle energy=%08x/%08x digest=%08x\n",
		eb0, eb1, d2);
	}

	// -----------------------------------------------------------------------
	// Actor slate 3: the damping getters (slots 42/44) and the +0x188 field
	// reader (slot 36). Present-record arms fold the field values; the
	// null-record arms report kind 1 through the oracle's .rdata slot --
	// captured with the VirtualProtect dance -- and return exact zero.
	{
	typedef float (__thiscall* NxActorFloatFn3)(void* self);
	NxActorFloatFn3 linDampOD = (NxActorFloatFn3)(base + 0x0002610);
	NxActorFloatFn3 angDampOD = (NxActorFloatFn3)(base + 0x0002680);
	NxActorFloatFn3 f188OD = (NxActorFloatFn3)(base + 0x00025c0);

	static unsigned sR3Cs[16];
	static unsigned sR3Scene[4];
	static unsigned sR3Body[64];
	static unsigned sR3Rec[0x80];
	InitializeCriticalSection((LPCRITICAL_SECTION)sR3Cs);
	sR3Scene[0] = reinterpret_cast<unsigned>(sR3Cs);
	memset(sR3Body, 0, sizeof(sR3Body));
	memset(sR3Rec, 0, sizeof(sR3Rec));
	sR3Body[2] = reinterpret_cast<unsigned>(sR3Rec);
	float linMark = 0.35f, angMark = 0.125f, f188Mark = 7.5f;
	memcpy(sR3Rec + 0x2e, &linMark, 4);		// +0xb8
	memcpy(sR3Rec + 0x2f, &angMark, 4);		// +0xbc
	memcpy(sR3Rec + 0x62, &f188Mark, 4);	// +0x188

	unsigned char actG[0x20];
	memset(actG, 0xcd, sizeof(actG));
	unsigned* gf = reinterpret_cast<unsigned*>(actG);
	gf[4] = reinterpret_cast<unsigned>(sR3Scene);
	gf[5] = reinterpret_cast<unsigned>(sR3Body);

	unsigned d3 = 2166136261u;
	float ldv = linDampOD(actG), adv = angDampOD(actG), f188v = f188OD(actG);
	unsigned lb, ab, fb;
	memcpy(&lb, &ldv, 4);
	memcpy(&ab, &adv, 4);
	memcpy(&fb, &f188v, 4);
	d3 = nxFold(d3, lb);
	d3 = nxFold(d3, ab);
	d3 = nxFold(d3, fb);

	// null-record arms: each kind-1 warning folded immediately -- a single
	// shared cap would keep only the second report
	memset(&g_errCap, 0, sizeof(g_errCap));
	unsigned* slot3 = (unsigned*) (base + 0x001041b4);
	unsigned* guard3Ptr = (unsigned*) (base + 0x001041b0);
	typedef void(__cdecl* NxReportFnO3)(int, const char*, int, int,
		const char*);
	NxReportFnO3 savedSink3 = reinterpret_cast<NxReportFnO3>(*slot3);
	DWORD oldProt3 = 0;
	if(!VirtualProtect(slot3, 8, PAGE_READWRITE, &oldProt3))
		return nxFail("actorsm3: VirtualProtect over the report slot failed");
	if(*guard3Ptr != 0 && *reinterpret_cast<unsigned*>(*guard3Ptr) == 0)
		*reinterpret_cast<unsigned*>(*guard3Ptr) = 1;
	*slot3 = reinterpret_cast<unsigned>(&g_errSink);
	sR3Body[2] = 0;
	float nz1 = linDampOD(actG);
	unsigned z1;
	memcpy(&z1, &nz1, 4);
	d3 = nxFold(d3, z1);
	d3 = nxFoldErrCap(d3);
	memset(&g_errCap, 0, sizeof(g_errCap));
	float nz2 = angDampOD(actG);
	unsigned z2;
	memcpy(&z2, &nz2, 4);
	d3 = nxFold(d3, z2);
	d3 = nxFoldErrCap(d3);
	*slot3 = reinterpret_cast<unsigned>(savedSink3);
	VirtualProtect(slot3, 8, oldProt3, &oldProt3);

	oActorsm3Digest = d3;
	oracleDigest = nxFold(oracleDigest, d3);
	printf("actorsm3 row=oracle lin=%08x ang=%08x digest=%08x\n",
		lb, ab, d3);
	}

	// -----------------------------------------------------------------------
	// Actor slate 4: the guarded binding WRITE (slot 83) and the pose-word
	// read with body-default fallback (slot 6). The binding key is each
	// side's own body pointer, so value-equality predicates fold instead of
	// addresses; the failed-upgrade arm reports line 0x1ff.
	{
	typedef void (__thiscall* NxActorSetBoundFn)(void* self, void* value);
	NxActorSetBoundFn setBoundOD = (NxActorSetBoundFn)(base + 0x0002d90);
	typedef void* (__thiscall* NxActorPoseFn)(void* self, void* out);
	NxActorPoseFn poseOD = (NxActorPoseFn)(base + 0x0002ed0);

	static unsigned sR4Cs[16];
	static unsigned sR4Scene[4];
	static unsigned sR4Body[64];
	static unsigned sR4Rec[0x40];
	InitializeCriticalSection((LPCRITICAL_SECTION)sR4Cs);
	sR4Scene[0] = reinterpret_cast<unsigned>(sR4Cs);
	memset(sR4Body, 0, sizeof(sR4Body));
	memset(sR4Rec, 0, sizeof(sR4Rec));
	sR4Body[2] = reinterpret_cast<unsigned>(sR4Rec);
	sR4Rec[0x14] = 0x00500001u;				// +0x50
	sR4Rec[0x15] = 0x00540002u;				// +0x54
	sR4Rec[0x16] = 0x00580003u;				// +0x58
	sR4Body[0x11] = 0x00440004u;			// +0x44 fallback
	sR4Body[0x12] = 0x00480005u;			// +0x48
	sR4Body[0x13] = 0x004c0006u;			// +0x4c

	unsigned char actH[0x20];
	memset(actH, 0xcd, sizeof(actH));
	unsigned* hf = reinterpret_cast<unsigned*>(actH);
	hf[3] = reinterpret_cast<unsigned>(sR4Scene);
	hf[4] = reinterpret_cast<unsigned>(sR4Scene);
	hf[5] = reinterpret_cast<unsigned>(sR4Body);

	unsigned d4 = 2166136261u;
	unsigned poseO[4];
	poseOD(actH, poseO);
	d4 = nxFold(d4, poseO[0]);
	d4 = nxFold(d4, poseO[1]);
	d4 = nxFold(d4, poseO[2]);

	// fallback arm: drop the record
	sR4Body[2] = 0;
	poseOD(actH, poseO);
	d4 = nxFold(d4, poseO[0]);
	d4 = nxFold(d4, poseO[1]);
	d4 = nxFold(d4, poseO[2]);
	sR4Body[2] = reinterpret_cast<unsigned>(sR4Rec);

	// binding write under the write guard: unbound check, bind to a magic
	// value, verify through the read accessor, contended failure
	setBoundOD(actH, reinterpret_cast<void*>(0x5A5A2000u));
	typedef void* (__thiscall* NxActorPtrFn2)(void* self);
	NxActorPtrFn2 boundOD2 = (NxActorPtrFn2)(base + 0x0002d60);
	d4 = nxFold(d4,
		reinterpret_cast<unsigned>(boundOD2(actH)) == 0x5A5A2000u ? 1u : 0u);

	memset(&g_errCap, 0, sizeof(g_errCap));
	unsigned* slot4 = (unsigned*) (base + 0x001041b4);
	unsigned* guard4Ptr = (unsigned*) (base + 0x001041b0);
	typedef void(__cdecl* NxReportFnO4)(int, const char*, int, int,
		const char*);
	NxReportFnO4 savedSink4 = reinterpret_cast<NxReportFnO4>(*slot4);
	DWORD oldProt4 = 0;
	if(!VirtualProtect(slot4, 8, PAGE_READWRITE, &oldProt4))
		return nxFail("actorsm4: VirtualProtect over the report slot failed");
	if(*guard4Ptr != 0 && *reinterpret_cast<unsigned*>(*guard4Ptr) == 0)
		*reinterpret_cast<unsigned*>(*guard4Ptr) = 1;
	*slot4 = reinterpret_cast<unsigned>(&g_errSink);
	sR4Cs[6] = 1;
	sR4Cs[7] = ::GetCurrentThreadId() + 1u;
	setBoundOD(actH, reinterpret_cast<void*>(0xDEAD2000u));
	nxInstallReportSink(nullptr);
	*slot4 = reinterpret_cast<unsigned>(savedSink4);
	VirtualProtect(slot4, 8, oldProt4, &oldProt4);
	d4 = nxFoldErrCap(d4);
	d4 = nxFold(d4,
		reinterpret_cast<unsigned>(boundOD2(actH)) == 0x5A5A2000u ? 1u : 0u);

	oActorsm4Digest = d4;
	oracleDigest = nxFold(oracleDigest, d4);
	printf("actorsm4 row=oracle digest=%08x\n", d4);
	}

	// -----------------------------------------------------------------------
	// Actor slate 5: the sleep chain. phys_fn_000713 path-compresses the
	// +0x1e8 caches recursively; phys_fn_000744 compresses then walks the
	// +0x1fc list over the +0x84 words; slot 67 wraps both under the read
	// guard with a null-record true. The compression itself is folded via
	// post-drive cache reads.
	{
	typedef unsigned (__thiscall* NxFixFn)(void* rec);
	NxFixFn fixOD = (NxFixFn)(base + 0x00015d50);			// 000713
	typedef bool (__thiscall* NxSettledFn)(void* rec);
	NxSettledFn settledOD = (NxSettledFn)(base + 0x00016e30);	// 000744
	typedef bool (__thiscall* NxActorBoolFn5)(void* self);
	NxActorBoolFn5 hasChainOD = (NxActorBoolFn5)(base + 0x0002950); // slot 67

	static unsigned sR5Cs[16];
	static unsigned sR5Scene[4];
	static unsigned sR5Body[64];
	static unsigned sR5RecA[0x90];
	static unsigned sR5RecM[0x90];
	static unsigned sR5RecR[0x90];
	InitializeCriticalSection((LPCRITICAL_SECTION)sR5Cs);
	sR5Scene[0] = reinterpret_cast<unsigned>(sR5Cs);
	memset(sR5Body, 0, sizeof(sR5Body));
	memset(sR5RecA, 0, sizeof(sR5RecA));
	memset(sR5RecM, 0, sizeof(sR5RecM));
	memset(sR5RecR, 0, sizeof(sR5RecR));
	unsigned rA = reinterpret_cast<unsigned>(sR5RecA);
	unsigned rM = reinterpret_cast<unsigned>(sR5RecM);
	unsigned rR = reinterpret_cast<unsigned>(sR5RecR);
	// chain A -> M -> R(self-rooted)
	sR5RecA[0x7a] = rM;						// +0x1e8
	sR5RecM[0x7a] = rR;
	sR5RecR[0x7a] = rR;						// self-parented root
	float awake = 4.0f;

	unsigned char actI[0x20];
	memset(actI, 0xcd, sizeof(actI));
	unsigned* i5 = reinterpret_cast<unsigned*>(actI);
	i5[4] = reinterpret_cast<unsigned>(sR5Scene);
	i5[5] = reinterpret_cast<unsigned>(sR5Body);

	unsigned d5 = 2166136261u;
	// direct fix over the two-hop chain: returns the root, compresses A
	unsigned fixed = fixOD(sR5RecA);
	d5 = nxFold(d5, fixed == rR ? 1u : 0u);
	d5 = nxFold(d5, sR5RecA[0x7a] == rR ? 1u : 0u);

	sR5Body[2] = rA;
	d5 = nxFold(d5, hasChainOD(actI) ? 1u : 0u);	// all zero -> settled

	// one awake node in the group
	sR5RecM[0x21] = *reinterpret_cast<unsigned*>(&awake);
	d5 = nxFold(d5, hasChainOD(actI) ? 1u : 0u);
	sR5RecM[0x21] = 0;

	// null-record arm of the wrapper
	sR5Body[2] = 0;
	d5 = nxFold(d5, hasChainOD(actI) ? 1u : 0u);

	oActorsm5Digest = d5;
	oracleDigest = nxFold(oracleDigest, d5);
	printf("actorsm5 row=oracle digest=%08x\n", d5);
	}

	// -----------------------------------------------------------------------
	// Actor slate 6: the CMass local frame getters. Present-record arms
	// fold marked matrix+translation words; static arms report kind 1 (one
	// capture per arm) and hand back identity-plus-zero / identity.
	{
	typedef void* (__thiscall* NxActorPoseOutFn)(void* self, void* out);
	NxActorPoseOutFn cmPoseOD = (NxActorPoseOutFn)(base + 0x0003140);
	NxActorPoseOutFn cmOriOD = (NxActorPoseOutFn)(base + 0x00032a0);

	static unsigned sR6Cs[16];
	static unsigned sR6Scene[4];
	static unsigned sR6Body[64];
	static unsigned sR6Rec[0x50];
	InitializeCriticalSection((LPCRITICAL_SECTION)sR6Cs);
	sR6Scene[0] = reinterpret_cast<unsigned>(sR6Cs);
	memset(sR6Body, 0, sizeof(sR6Body));
	memset(sR6Rec, 0, sizeof(sR6Rec));
	sR6Body[2] = reinterpret_cast<unsigned>(sR6Rec);
	for(int i = 0; i < 12; ++i)
		sR6Rec[0x37 + i] = 0x00C0FFEEu + static_cast<unsigned>(i);
			// +0xdc: nine rotation words then three translation words

	unsigned char actJ[0x20];
	memset(actJ, 0xcd, sizeof(actJ));
	unsigned* jf = reinterpret_cast<unsigned*>(actJ);
	jf[4] = reinterpret_cast<unsigned>(sR6Scene);
	jf[5] = reinterpret_cast<unsigned>(sR6Body);

	unsigned poseJ[12];
	memset(poseJ, 0xcd, sizeof(poseJ));

	unsigned d6 = 2166136261u;
	cmPoseOD(actJ, poseJ);
	for(int i = 0; i < 12; ++i)
		d6 = nxFold(d6, poseJ[i]);
	memset(poseJ, 0xcd, sizeof(poseJ));
	cmOriOD(actJ, poseJ);
	for(int i = 0; i < 9; ++i)
		d6 = nxFold(d6, poseJ[i]);

	// static-actor arms: warnings fire, defaults come back
	unsigned* slot6 = (unsigned*) (base + 0x001041b4);
	unsigned* guard6Ptr = (unsigned*) (base + 0x001041b0);
	typedef void(__cdecl* NxReportFnO6)(int, const char*, int, int,
		const char*);
	NxReportFnO6 savedSink6 = reinterpret_cast<NxReportFnO6>(*slot6);
	DWORD oldProt6 = 0;
	if(!VirtualProtect(slot6, 8, PAGE_READWRITE, &oldProt6))
		return nxFail("actorsm6: VirtualProtect over the report slot failed");
	if(*guard6Ptr != 0 && *reinterpret_cast<unsigned*>(*guard6Ptr) == 0)
		*reinterpret_cast<unsigned*>(*guard6Ptr) = 1;
	*slot6 = reinterpret_cast<unsigned>(&g_errSink);

	sR6Body[2] = 0;
	memset(&g_errCap, 0, sizeof(g_errCap));
	cmPoseOD(actJ, poseJ);
	d6 = nxFoldErrCap(d6);
	const unsigned kIdent[9] =
		{ 0x3f800000u, 0, 0, 0, 0x3f800000u, 0, 0, 0, 0x3f800000u };
	int poseOk = memcmp(poseJ, kIdent, 36) == 0
		&& poseJ[9] == 0 && poseJ[10] == 0 && poseJ[11] == 0;
	d6 = nxFold(d6, poseOk ? 1u : 0u);

	memset(&g_errCap, 0, sizeof(g_errCap));
	cmOriOD(actJ, poseJ);
	d6 = nxFoldErrCap(d6);
	int oriOk = memcmp(poseJ, kIdent, 36) == 0;
	d6 = nxFold(d6, oriOk ? 1u : 0u);

	*slot6 = reinterpret_cast<unsigned>(savedSink6);
	VirtualProtect(slot6, 8, oldProt6, &oldProt6);

	oActorsm6Digest = d6;
	oracleDigest = nxFold(oracleDigest, d6);
	printf("actorsm6 row=oracle digest=%08x\n", d6);
	}

	// -----------------------------------------------------------------------
	// Actor slate 7: five guarded three-word readers. Each folds its
	// present-record marks, then its static arm -- warning captured per
	// arm and the default triple folded as words.
	{
	typedef void (__thiscall* NxActorFill3Fn)(void* self, void* out);
	NxActorFill3Fn cmPosOD = (NxActorFill3Fn)(base + 0x0003200);	// 000098
	NxActorFill3Fn inertiaOD = (NxActorFill3Fn)(base + 0x0003310);	// 000102
	NxActorFill3Fn linVelOD = (NxActorFill3Fn)(base + 0x00033b0);	// 000104
	NxActorFill3Fn angVelOD = (NxActorFill3Fn)(base + 0x0003440);	// 000106
	NxActorFill3Fn linMomOD = (NxActorFill3Fn)(base + 0x00034d0);	// 000108

	static unsigned sR7Cs[16];
	static unsigned sR7Scene[4];
	static unsigned sR7Body[64];
	static unsigned sR7Rec[0x70];
	InitializeCriticalSection((LPCRITICAL_SECTION)sR7Cs);
	sR7Scene[0] = reinterpret_cast<unsigned>(sR7Cs);
	memset(sR7Body, 0, sizeof(sR7Body));
	memset(sR7Rec, 0, sizeof(sR7Rec));
	sR7Body[2] = reinterpret_cast<unsigned>(sR7Rec);
	for(int i = 0; i < 3; ++i)
		{
		unsigned w = 0x43B00000u + static_cast<unsigned>(i) * 0x01000000u;
		memcpy(sR7Rec + 0x40 + i, &w, 4);	// +0x100..: distinct floats
		memcpy(sR7Rec + 0x63 + i, &w, 4);	// +0x18c..: same marks (inertia)
		}
	float v6c = 1.25f, v70 = 2.5f, v74 = 3.75f;
	float a78 = 0.5f, a7c = 1.0f, a80 = 1.5f;
	float mass = 8.0f;
	memcpy(sR7Rec + 0x1b, &v6c, 4);			// +0x6c velocity
	memcpy(sR7Rec + 0x1c, &v70, 4);
	memcpy(sR7Rec + 0x1d, &v74, 4);
	memcpy(sR7Rec + 0x1e, &a78, 4);			// +0x78 angular
	memcpy(sR7Rec + 0x1f, &a7c, 4);
	memcpy(sR7Rec + 0x20, &a80, 4);
	memcpy(sR7Rec + 0x62, &mass, 4);		// +0x188

	unsigned char actK[0x20];
	memset(actK, 0xcd, sizeof(actK));
	unsigned* kf = reinterpret_cast<unsigned*>(actK);
	kf[4] = reinterpret_cast<unsigned>(sR7Scene);
	kf[5] = reinterpret_cast<unsigned>(sR7Body);

	unsigned d7 = 2166136261u;
	unsigned out7[4];

	struct NxArm { NxActorFill3Fn fn; const char* tag; };
	NxArm arms[5] =
		{
			{ cmPosOD, "cmpos" },
			{ inertiaOD, "inertia" },
			{ linVelOD, "linvel" },
			{ angVelOD, "angvel" },
			{ linMomOD, "linmom" },
		};
	for(int k = 0; k < 5; ++k)
		{
		arms[k].fn(actK, out7);
		d7 = nxFold(d7, out7[0]);
		d7 = nxFold(d7, out7[1]);
		d7 = nxFold(d7, out7[2]);
		}

	// static arms with per-arm capture
	unsigned* slot7 = (unsigned*) (base + 0x001041b4);
	unsigned* guard7Ptr = (unsigned*) (base + 0x001041b0);
	typedef void(__cdecl* NxReportFnO7)(int, const char*, int, int,
		const char*);
	NxReportFnO7 savedSink7 = reinterpret_cast<NxReportFnO7>(*slot7);
	DWORD oldProt7 = 0;
	if(!VirtualProtect(slot7, 8, PAGE_READWRITE, &oldProt7))
		return nxFail("actorsm7: VirtualProtect over the report slot failed");
	if(*guard7Ptr != 0 && *reinterpret_cast<unsigned*>(*guard7Ptr) == 0)
		*reinterpret_cast<unsigned*>(*guard7Ptr) = 1;
	*slot7 = reinterpret_cast<unsigned>(&g_errSink);
	sR7Body[2] = 0;
	for(int k = 0; k < 5; ++k)
		{
		memset(&g_errCap, 0, sizeof(g_errCap));
		arms[k].fn(actK, out7);
		d7 = nxFold(d7, out7[0]);
		d7 = nxFold(d7, out7[1]);
		d7 = nxFold(d7, out7[2]);
		d7 = nxFoldErrCap(d7);
		}
	*slot7 = reinterpret_cast<unsigned>(savedSink7);
	VirtualProtect(slot7, 8, oldProt7, &oldProt7);

	oActorsm7Digest = d7;
	oracleDigest = nxFold(oracleDigest, d7);
	printf("actorsm7 row=oracle digest=%08x\n", d7);
	}

	// -----------------------------------------------------------------------
	// Actor slate 8: the group WRITER (slot 85, with its contended line
	// 0x3cd report), the sub-object virtual forwarder (000004) driven
	// through a planted vtable sentinel, and the id-allocator primitive
	// (000012) over both arms.
	{
	typedef void (__thiscall* NxActorSetGrpFn)(void* self, unsigned g);
	NxActorSetGrpFn setGrpOD = (NxActorSetGrpFn)(base + 0x00035b0);
	typedef unsigned (__thiscall* NxFwdFn)(void* self, void* arg);
	NxFwdFn fwdOD = (NxFwdFn)(base + 0x0001070);
	typedef unsigned (__thiscall* NxAllocFn2)(void* container);
	NxAllocFn2 allocOD = (NxAllocFn2)(base + 0x0001430);

	static unsigned sR8Cs[16];
	static unsigned sR8Scene[4];
	static unsigned sR8Body[64];
	InitializeCriticalSection((LPCRITICAL_SECTION)sR8Cs);
	sR8Scene[0] = reinterpret_cast<unsigned>(sR8Cs);
	memset(sR8Body, 0, sizeof(sR8Body));

	unsigned char actL[0x20];
	memset(actL, 0xcd, sizeof(actL));
	unsigned* lf = reinterpret_cast<unsigned*>(actL);
	lf[3] = reinterpret_cast<unsigned>(sR8Scene);
	lf[4] = reinterpret_cast<unsigned>(sR8Scene);
	lf[5] = reinterpret_cast<unsigned>(sR8Body);

	unsigned d8 = 2166136261u;

	// setGroup: write under guard, read back through slot 86's oracle
	setGrpOD(actL, 0x0007u);
	static unsigned sR8ReadScene[4];
	static unsigned sR8ReadCs[16];
	InitializeCriticalSection((LPCRITICAL_SECTION)sR8ReadCs);
	sR8ReadScene[0] = reinterpret_cast<unsigned>(sR8ReadCs);
	unsigned char actR[0x20];
	memset(actR, 0xcd, sizeof(actR));
	unsigned* rf = reinterpret_cast<unsigned*>(actR);
	rf[4] = reinterpret_cast<unsigned>(sR8ReadScene);
	rf[5] = reinterpret_cast<unsigned>(sR8Body);
	typedef unsigned short (__thiscall* NxActorWordFn2)(void*);
	NxActorWordFn2 grpReadOD = (NxActorWordFn2)(base + 0x0003610);
	d8 = nxFold(d8, grpReadOD(actR));
	d8 = nxFold(d8, sR8Cs[6]);				// writer flag released

	// contended setGroup: kind-2 report at line 0x3cd, group unchanged
	setGrpOD(actL, 0x0009u);				// group now 9
	memset(&g_errCap, 0, sizeof(g_errCap));
	unsigned* slot8 = (unsigned*) (base + 0x001041b4);
	unsigned* guard8Ptr = (unsigned*) (base + 0x001041b0);
	typedef void(__cdecl* NxReportFnO8)(int, const char*, int, int,
		const char*);
	NxReportFnO8 savedSink8 = reinterpret_cast<NxReportFnO8>(*slot8);
	DWORD oldProt8 = 0;
	if(!VirtualProtect(slot8, 8, PAGE_READWRITE, &oldProt8))
		return nxFail("actorsm8: VirtualProtect over the report slot failed");
	if(*guard8Ptr != 0 && *reinterpret_cast<unsigned*>(*guard8Ptr) == 0)
		*reinterpret_cast<unsigned*>(*guard8Ptr) = 1;
	*slot8 = reinterpret_cast<unsigned>(&g_errSink);
	sR8Cs[6] = 1;
	sR8Cs[7] = ::GetCurrentThreadId() + 1u;
	setGrpOD(actL, 0x000Bu);
	nxInstallReportSink(nullptr);
	*slot8 = reinterpret_cast<unsigned>(savedSink8);
	VirtualProtect(slot8, 8, oldProt8, &oldProt8);
	d8 = nxFoldErrCap(d8);
	d8 = nxFold(d8, grpReadOD(actR));		// unchanged 9

	// sub-object forwarder through a planted sentinel. The planted target
	// is a member function so its calling convention really is __thiscall,
	// matching the oracle's tail-jump with this=sub and one stack argument.
	static unsigned g_fwdGotSub;
	static unsigned g_fwdGotArg;
	static unsigned g_fwdRetMark;
	struct NxFwdTarget
		{
		unsigned __thiscall slot18(void* arg)
			{
			g_fwdGotSub = reinterpret_cast<unsigned>(this);
			g_fwdGotArg = reinterpret_cast<unsigned>(arg);
			return g_fwdRetMark;
			}
		};
	g_fwdGotSub = 0;
	g_fwdGotArg = 0;
	g_fwdRetMark = 0x12340005u;
	static unsigned sFwdSubVt[8];
	static unsigned sFwdSub[4];
	union NxFwdAddr
		{
		unsigned (NxFwdTarget::* pmf)(void*);
		unsigned addr;
		};
	NxFwdAddr fa;
	fa.pmf = &NxFwdTarget::slot18;
	sFwdSubVt[0x18 / 4] = fa.addr;
	sFwdSub[0] = reinterpret_cast<unsigned>(sFwdSubVt);
	unsigned fwdSelf[8];
	memset(fwdSelf, 0xcd, sizeof(fwdSelf));
	fwdSelf[4] = reinterpret_cast<unsigned>(sFwdSub);	// +0x10 sub-object
	unsigned retFwd = fwdOD(fwdSelf,
		reinterpret_cast<void*>(0x00BEEF00u));
	int fwdOk = (g_fwdGotSub == reinterpret_cast<unsigned>(sFwdSub)
		&& g_fwdGotArg == 0x00BEEF00u
		&& retFwd == g_fwdRetMark);
	d8 = nxFold(d8, fwdOk ? 1u : 0u);
	// NOTE: the null sub-object path returns whatever eax already held
	// (the image just does ret 4), so it is deliberately not driven -- a
	// nondeterministic value cannot be pinned on either side.

	// id allocator: counter arm then freelist pop arm
	static unsigned sIdC[8];				// {counter, begin, cursor}
	static unsigned sIdFree[4];
	memset(sIdC, 0, sizeof(sIdC));
	memset(sIdFree, 0, sizeof(sIdFree));
	d8 = nxFold(d8, allocOD(sIdC));			// counter arm: 0, bumps to 1
	d8 = nxFold(d8, allocOD(sIdC));			// 1 -> 2
	sIdC[1] = reinterpret_cast<unsigned>(sIdFree);
	sIdC[2] = reinterpret_cast<unsigned>(sIdFree + 2);
	sIdFree[0] = 0x000000AAu;
	sIdFree[1] = 0x000000BBu;
	unsigned popped = allocOD(sIdC);		// pops BB, cursor shrinks
	d8 = nxFold(d8, popped);
	d8 = nxFold(d8, allocOD(sIdC));			// pops AA
	d8 = nxFold(d8, sIdC[2] == reinterpret_cast<unsigned>(sIdFree) ? 1u : 0u);

	oMiscsmDigest = d8;
	oracleDigest = nxFold(oracleDigest, d8);
	printf("miscsm row=oracle digest=%08x\n", d8);
	}

	// -----------------------------------------------------------------------
	// Actor slate 9: readBodyFlag (both arms), the member deleting dtor
	// (linked-CRT release), the shapes-clear entry driving the release arm
	// end to end, and two bound-pool deleting dtors with adapter releases.
	{
	typedef bool (__thiscall* NxActorFlagFn)(void* self, unsigned mask);
	NxActorFlagFn readFlagOD = (NxActorFlagFn)(base + 0x0002c90);
	typedef void (__thiscall* NxOneArgDtorFn)(void* self, void* arg);
	NxOneArgDtorFn clearOD = (NxOneArgDtorFn)(base + 0x0005bbb0);
	typedef void (__thiscall* NxDelFn)(void* self, unsigned flags);
	NxDelFn memDelOD = (NxDelFn)(base + 0x0005baa0);
	NxDelFn b798OD = (NxDelFn)(base + 0x0005a440);
	NxDelFn b84cOD = (NxDelFn)(base + 0x0005aa60);

	static unsigned sR9Cs[16];
	static unsigned sR9Scene[4];
	static unsigned sR9Body[64];
	static unsigned sR9Rec[0x40];
	InitializeCriticalSection((LPCRITICAL_SECTION)sR9Cs);
	sR9Scene[0] = reinterpret_cast<unsigned>(sR9Cs);
	memset(sR9Body, 0, sizeof(sR9Body));
	memset(sR9Rec, 0, sizeof(sR9Rec));
	sR9Body[2] = reinterpret_cast<unsigned>(sR9Rec);
	sR9Rec[0x43] = 0x000000A5u;				// +0x10c byte: bits 0,2,5,7

	unsigned char actM[0x20];
	memset(actM, 0xcd, sizeof(actM));
	unsigned* mf = reinterpret_cast<unsigned*>(actM);
	mf[4] = reinterpret_cast<unsigned>(sR9Scene);
	mf[5] = reinterpret_cast<unsigned>(sR9Body);

	unsigned d9 = 2166136261u;
	bool q1 = readFlagOD(actM, 0x25u);
	bool q2 = readFlagOD(actM, 0x08u);
	d9 = nxFold(d9, q1 ? 1u : 0u);	// hits bits 0+2+5? A5: yes
	d9 = nxFold(d9, q2 ? 1u : 0u);	// bit3 not set -> false

	// static arm: kind-1 warning captured per arm, returns false
	unsigned* slot9 = (unsigned*) (base + 0x001041b4);
	unsigned* guard9Ptr = (unsigned*) (base + 0x001041b0);
	typedef void(__cdecl* NxReportFnO9)(int, const char*, int, int,
		const char*);
	NxReportFnO9 savedSink9 = reinterpret_cast<NxReportFnO9>(*slot9);
	DWORD oldProt9 = 0;
	memset(&g_errCap, 0, sizeof(g_errCap));
	if(!VirtualProtect(slot9, 8, PAGE_READWRITE, &oldProt9))
		return nxFail("miscsm2: VirtualProtect over the report slot failed");
	if(*guard9Ptr != 0 && *reinterpret_cast<unsigned*>(*guard9Ptr) == 0)
		*reinterpret_cast<unsigned*>(*guard9Ptr) = 1;
	*slot9 = reinterpret_cast<unsigned>(&g_errSink);
	sR9Body[2] = 0;
	bool stFlag = readFlagOD(actM, 0x25u);
	nxInstallReportSink(nullptr);
	d9 = nxFoldErrCap(d9);
	d9 = nxFold(d9, stFlag ? 1u : 0u);

	// member deleting dtor: linked-CRT block, post-free vtable fold is not
	// readable -- fold the pre-call third-table install on a stack block
	unsigned stackMem[4];
	memset(stackMem, 0xcd, sizeof(stackMem));
	memDelOD(stackMem, 0);
	int memVt = (stackMem[0] == 0x101088b8u);

	// shapes-clear entry: THIS is the pool base; its +0x40 byte offset is
	// the release-arm header (sentinel/counts/mirrors/free vector), and
	// +0x80 names the shapes array. Freelist AT capacity so the release
	// reallocs through the arena; slot 3 live.
	static unsigned sR9Sent[64];
	static unsigned sR9CntA[64];
	static unsigned sR9CntB[64];
	static unsigned* sR9CntBEnd = sR9CntB + 8;
	static unsigned sR9Mir[64];
	static unsigned sR9Fl[8];
	static unsigned sR9Hdr[128];			// pool base: hdr at +0x40 bytes
	static unsigned sR9Shapes[64];
	memset(sR9Hdr, 0, sizeof(sR9Hdr));
	for(int i = 0; i < 64; ++i)
		{
		sR9Sent[i] = 0xFFFFFFFFu;
		sR9CntA[i] = 0xC0000000u + static_cast<unsigned>(i);
		sR9CntB[i] = 0;
		sR9Mir[i] = static_cast<unsigned>(i);
		sR9Shapes[i] = 0x13572468u;
		}
	sR9CntB[7] = 7u;
	unsigned* r9Fl = static_cast<unsigned*>(nxHeapAlloc(8 * sizeof(unsigned)));
	for(int i = 0; i < 8; ++i)
		r9Fl[i] = 0x22220000u + static_cast<unsigned>(i);
	sR9Hdr[0x10] = reinterpret_cast<unsigned>(sR9Sent);		// hdr+0x00
	sR9Hdr[0x14] = reinterpret_cast<unsigned>(sR9CntA);		// hdr+0x10
	sR9Hdr[0x15] = reinterpret_cast<unsigned>(sR9CntBEnd);	// hdr+0x14
	sR9Hdr[0x18] = reinterpret_cast<unsigned>(sR9Mir);		// hdr+0x20
	sR9Hdr[0x1c] = reinterpret_cast<unsigned>(r9Fl);		// hdr+0x30
	sR9Hdr[0x1d] = reinterpret_cast<unsigned>(r9Fl + 8);	// hdr+0x34
	sR9Hdr[0x1e] = reinterpret_cast<unsigned>(r9Fl + 8);	// hdr+0x38
	sR9Hdr[0x20] = reinterpret_cast<unsigned>(sR9Shapes);	// +0x80 array
	sR9Sent[3] = 0x00C0FFEEu;					// live slot 3

	static unsigned sR9ShapeArg[0x42];
	memset(sR9ShapeArg, 0, sizeof(sR9ShapeArg));
	sR9ShapeArg[0x41] = 3u;						// +0x104: slot index

	NxHeapMark m9 = nxHeapMarkNow();
	clearOD(sR9Hdr, sR9ShapeArg);
	int slotFreed = (sR9Sent[3] == 0u && sR9Mir[3] == 0xD00BEED0u);
	int shapeCleared = (sR9Shapes[3] == 0u);
	int flGrew = ((sR9Hdr[0x1d] - sR9Hdr[0x1c]) >> 2) == 9;
	int heapMoved = (g_heapMallocOps == m9.mo + 1);

	d9 = nxFold(d9, memVt ? 1u : 0u);
	d9 = nxFold(d9, slotFreed ? 1u : 0u);
	d9 = nxFold(d9, shapeCleared ? 1u : 0u);
	d9 = nxFold(d9, flGrew ? 1u : 0u);
	d9 = nxFold(d9, heapMoved ? 1u : 0u);

	// bound-pool deleting dtors: arena blocks, adapter release, post-free
	// third-table vptr reads
	unsigned* bAO = static_cast<unsigned*>(nxHeapAlloc(sizeof(unsigned) * 4));
	for(int i = 0; i < 4; ++i) bAO[i] = 0xFEEDF00Du;
	b798OD(bAO, 1);
	int b798Ok = (bAO[0] == 0x10108798u);
	unsigned* bBO = static_cast<unsigned*>(nxHeapAlloc(sizeof(unsigned) * 4));
	for(int i = 0; i < 4; ++i) bBO[i] = 0xFEEDF00Du;
	b84cOD(bBO, 1);
	int b84cOk = (bBO[0] == 0x1010884cu);
	d9 = nxFold(d9, b798Ok ? 1u : 0u);
	d9 = nxFold(d9, b84cOk ? 1u : 0u);

	oMiscsm2Digest = d9;
	oracleDigest = nxFold(oracleDigest, d9);
	printf("miscsm2 row=oracle digest=%08x\n", d9);
	}

	// -----------------------------------------------------------------------
	// Slate 11: pool-class lifecycle rows driven minimally. All on stack
	// blocks or with flags=0 (no free) to avoid any allocator interaction.
	{
	typedef void (__thiscall* NxDelFnS)(void* self, unsigned flags);
	NxDelFnS chainOD = (NxDelFnS)(base + 0x0005a470);		// 002328
	NxDelFnS adjOD = (NxDelFnS)(base + 0x0005a230);			// 002322
	NxDelFnS crtOD = (NxDelFnS)(base + 0x0005a080);			// 002312
	typedef void (__thiscall* NxListFnS)(void* self);
	NxListFnS listOD10 = (NxListFnS)(base + 0x0005a1e0);		// 002320

	unsigned dA = 2166136261u;

	// 002328: chained dtor, stack block, flags=0
	unsigned chX[4];
	memset(chX, 0xcd, sizeof(chX));
	chainOD(chX, 0);
	dA = nxFold(dA, chX[0] == 0x1010878cu ? 1u : 0u);
	dA = nxFold(dA, chX[2] == 0x10108798u ? 1u : 0u);

	// 002322: adjustor thunk to same dtor, stack block, flags=0
	unsigned chY[4];
	memset(chY, 0xcd, sizeof(chY));
	adjOD(chY + 2, 0);
	dA = nxFold(dA, chY[0] == 0x1010878cu ? 1u : 0u);
	dA = nxFold(dA, chY[2] == 0x10108798u ? 1u : 0u);

	// 002312: CRT-free dtor, stack block, flags=0
	unsigned stk78[4];
	memset(stk78, 0xcd, sizeof(stk78));
	crtOD(stk78, 0);
	dA = nxFold(dA, stk78[0] == 0x1010878cu ? 1u : 0u);

	// 002320: cached-list destroyer over two planted nodes
	static unsigned sLNVT[4];
	sLNVT[0] = reinterpret_cast<unsigned>(&NxS11KillThunk);
	g_sl11KillCount = 0;
	unsigned nA[16], nM[16];
	memset(nA, 0, sizeof(nA));
	memset(nM, 0, sizeof(nM));
	nA[0] = reinterpret_cast<unsigned>(sLNVT);
	nA[3] = reinterpret_cast<unsigned>(nM);		// +0x30 link (word 3)
	nM[0] = reinterpret_cast<unsigned>(sLNVT);
	nM[3] = 0;
	static unsigned sLH[0x170];
	memset(sLH, 0, sizeof(sLH));
	sLH[0x16a] = reinterpret_cast<unsigned>(nA);	// byte +0x5a8
	listOD10(sLH);
	dA = nxFold(dA, g_sl11KillCount == 2 ? 1u : 0u); // two nodes deleted

	oSlate11Digest = dA;
	oracleDigest = nxFold(oracleDigest, dA);
	printf("slate11 row=oracle digest=%08x\n", dA);
	}

	// -----------------------------------------------------------------------
	// phys_fn_000847: the conditional mass-frame zeroizer. Two drives --
	// flag=1 (zeroes all 13 words) and flag=0 (leaves untouched) -- against
	// pre-populated frames.
	{
	typedef void (__thiscall* NxZeroFn)(void* self, unsigned flag);
	NxZeroFn zeroFn = (NxZeroFn) (base + 0x0001c880);

	unsigned char zA[0x34];
	memset(zA, 0xcd, sizeof(zA));
	for(int i = 0; i < 13; ++i)
		{
		unsigned w = 0x41414141u + static_cast<unsigned>(i);
		memcpy(zA + i * 4, &w, 4);
		}
	unsigned char zB[0x34];
	memset(zB, 0xcd, sizeof(zB));
	for(int i = 0; i < 13; ++i)
		{
		unsigned w = 0x42424242u + static_cast<unsigned>(i);
		memcpy(zB + i * 4, &w, 4);
		}
	zeroFn(zA, 1);
	zeroFn(zB, 0);

	unsigned zd = 2166136261u;
	for(int v = 0; v < 2; ++v)
		{
		const unsigned char* src = (v == 0) ? zA : zB;
		for(int i = 0; i < 0x34; i += 4)
			{
			unsigned w;
			memcpy(&w, src + i, 4);
			zd = nxFold(zd, w);
			}
		}
	oZeroDigest = zd;

	oracleDigest = nxFold(oracleDigest, zd);
	printf("mzero row=phys_fn_000847 zA=%08x zB=%08x digest=%08x\n",
		*reinterpret_cast<const unsigned*>(zA),
		*reinterpret_cast<const unsigned*>(zB), zd);
	}

	// -----------------------------------------------------------------------


	// -----------------------------------------------------------------------
	// The default material template at .data 0x1220a0: the shipped 72 bytes
	// folded raw, versus the transcription's fresh record + the internal
	// flag bit the image sets on the template after copying it into the
	// SDK array.
	{
	unsigned mt = 2166136261u;
	for(int i = 0; i < 0x48; i += 4)
		{
		unsigned w;
		memcpy(&w, base + 0x001220a0 + i, 4);
		mt = nxFold(mt, w);
		}
	oMaterialTemplateDigest = mt;

	unsigned flagsWord;
	memcpy(&flagsWord, base + 0x001220a0 + 0x38, 4);
	oracleDigest = nxFold(oracleDigest, mt);
	printf("material row=template flags=%08x digest=%08x\n", flagsWord, mt);

	// Post-creation state (0x0000e9ee): the image sets bit 31 of the
	// template's flags AFTER copying it into the SDK array. Replicate that
	// single store against .rdata (VirtualProtect, flip, restore) and fold
	// again -- this is the state NxMaterialRecord + setInternalFlagBit31
	// must reproduce.
	unsigned* tmplFlags = reinterpret_cast<unsigned*>(
		const_cast<unsigned char*>(base) + 0x001220a0 + 0x38);
	DWORD oldProtM = 0;
	if(!VirtualProtect(tmplFlags, 4, PAGE_READWRITE, &oldProtM))
		return nxFail("material: VirtualProtect over the template failed");
	*tmplFlags |= 0x80000000u;
	unsigned mtBoot = 2166136261u;
	for(int i = 0; i < 0x48; i += 4)
		{
		unsigned w;
		memcpy(&w, base + 0x001220a0 + i, 4);
		mtBoot = nxFold(mtBoot, w);
		}
	*tmplFlags &= ~0x80000000u;
	VirtualProtect(tmplFlags, 4, oldProtM, &oldProtM);
	oMaterialBootedDigest = mtBoot;

	unsigned flagsBoot = *reinterpret_cast<const unsigned*>(base + 0x001220a0 + 0x38);
	oracleDigest = nxFold(oracleDigest, mtBoot);
	printf("materialboot row=template flags=%08x digest=%08x\n", flagsBoot, mtBoot);
	}

	// -----------------------------------------------------------------------
	// PLANE slots 9/11, phys_fn_001257: zero vec3 + +FLT_MAX reach.
	{
	typedef void (__thiscall* NxPlaneExtFn)(void* self, float* out);
	NxPlaneExtFn planeExt = (NxPlaneExtFn) (base + 0x000251d0);

	unsigned char pshape[0x10c];
	memset(pshape, 0xcd, sizeof(pshape));
	typedef void (__thiscall* NxCtorFnf)(void* self, void* owner, unsigned argument);
	NxCtorFnf planeCtor3 = (NxCtorFnf) (base + 0x00024ed0);
	planeCtor3(pshape, 0, 0);

	float out[4] = { 0, 0, 0, 0 };
	planeExt(pshape, out);

	unsigned dp = 2166136261u;
	for(int i = 0; i < 4; ++i)
		{
		unsigned w; memcpy(&w, out + i, 4);
		dp = nxFold(dp, w);
		}
	oPlaneExtentDigest = dp;
	oracleDigest = nxFold(oracleDigest, dp);

	unsigned pb[4] = { 0, 0, 0, 0 };
	for(int i = 0; i < 4; ++i)
		memcpy(&pb[i], out + i, 4);
	printf("planeext row=phys_fn_001257 out=%08x.%08x.%08x.%08x\n",
		pb[0], pb[1], pb[2], pb[3]);
	}


	// -----------------------------------------------------------------------
	// PLANE slot 0 (phys_fn_001263) and MESH slot 0 (phys_fn_001399):
	// scalar deleting destructors, flag=0. The mesh one decrements the bound
	// mesh's refcount when non-null -- fresh meshes have none, so the dec
	// arm is skipped on both sides.
	{
	typedef void (__thiscall* NxPlaneDtorFn)(void* self, unsigned flags);
	typedef unsigned (__thiscall* NxMeshDtorFn)(void* self, unsigned flags);
	NxPlaneDtorFn planeDtor = (NxPlaneDtorFn) (base + 0x00025420);
	NxMeshDtorFn meshDtor = (NxMeshDtorFn) (base + 0x00028e80);
	static const unsigned kMask[] = { 0x00, 0x9c, 0xa4, 0xa8, 0xb0, 0xb4 };

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char pshape[0x10c];
	memset(pshape, 0xcd, sizeof(pshape));
	typedef void (__thiscall* NxCtorFnh)(void* self, void* owner, unsigned argument);
	NxCtorFnh planeCtor4 = (NxCtorFnh) (base + 0x00024ed0);
	planeCtor4(pshape, 0, 0);

	unsigned fc = 0, fa = 0;
	nxGuardedBoxDtor((NxBoxDtorFn) planeDtor, pshape, 0);
	fc = gDtorFaultCode; fa = gDtorFaultAddr;
	if(fc)
		return nxFail("plane dtor faulted");

	unsigned dpl = 2166136261u;
	for(unsigned i = 0; i < sizeof(pshape); i += 4)
		{
		bool pointer = false;
		for(size_t p = 0; p < sizeof(kMask) / sizeof(kMask[0]); ++p)
			if(kMask[p] == i)
				pointer = true;
		if(pointer)
			continue;
		unsigned w; memcpy(&w, pshape + i, 4);
		dpl = nxFold(dpl, w);
		}
	oPlaneDtorDigest = dpl;
	oracleDigest = nxFold(oracleDigest, dpl);

	unsigned char mshape[0xe8];
	memset(mshape, 0xcd, sizeof(mshape));
	NxCtorFnh meshCtor4 = (NxCtorFnh) (base + 0x00027db0);
	meshCtor4(mshape, 0, 0);

	nxGuardedBoxDtor((NxBoxDtorFn) meshDtor, mshape, 0);
	fc = gDtorFaultCode; fa = gDtorFaultAddr;
	if(fc)
		return nxFail("mesh dtor faulted");

	unsigned dm = 2166136261u;
	for(unsigned i = 0; i < sizeof(mshape); i += 4)
		{
		bool pointer = false;
		for(size_t p = 0; p < sizeof(kMask) / sizeof(kMask[0]); ++p)
			if(kMask[p] == i)
				pointer = true;
		if(pointer)
			continue;
		unsigned w;
		memcpy(&w, mshape + i, 4);
		dm = nxFold(dm, w);
		}
	oMeshDtorDigest = dm;
	oracleDigest = nxFold(oracleDigest, dm);

	printf("dtors2 plane=phys_fn_001263 digest=%08x mesh=phys_fn_001399 digest=%08x\n",
		dpl, dm);
	}

	// -----------------------------------------------------------------------
	// CAPSULE slot 0, phys_fn_001014: scalar deleting destructor, flag=0.
	{
	typedef void (__thiscall* NxCapDtorFn)(void* self, unsigned flags);
	NxCapDtorFn capDtor = (NxCapDtorFn) (base + 0x000225e0);

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char cshape[0xec];
	memset(cshape, 0xcd, sizeof(cshape));
	typedef void (__thiscall* NxCtorFn12)(void* self, void* owner, unsigned argument);
	NxCtorFn12 capsuleCtor5 = (NxCtorFn12) (base + 0x00021a60);
	capsuleCtor5(cshape, 0, 0);

	nxGuardedBoxDtor((NxBoxDtorFn) capDtor, cshape, 0);
	if(gDtorFaultCode)
		return nxFail("capsule dtor faulted");

	unsigned digest = 2166136261u;
	for(unsigned i = 0; i < sizeof(cshape); i += 4)
		{
		bool pointer = false;
		for(size_t p = 0; p < 6; ++p)
			if(kDtorMask[p] == i)
				pointer = true;
		if(pointer)
			continue;
		unsigned w; memcpy(&w, cshape + i, 4);
		digest = nxFold(digest, w);
		}
	oCapsuleDtorDigest = digest;
	oracleDigest = nxFold(oracleDigest, digest);
	printf("capdtor row=phys_fn_001014 digest=%08x\n", digest);
	}

	// -----------------------------------------------------------------------
	// CAPSULE slot 12, phys_fn_000989: loadFromDesc.
	{
	typedef void (__thiscall* NxCapLoadFn)(void* self, const void* rec);
	NxCapLoadFn capLoad = (NxCapLoadFn) (base + 0x00021ad0);

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char cshape[0xec];
	memset(cshape, 0xcd, sizeof(cshape));
	typedef void (__thiscall* NxCtorFnCL)(void* self, void* owner, unsigned argument);
	NxCtorFnCL capsuleCtorCL = (NxCtorFnCL) (base + 0x00021a60);
	capsuleCtorCL(cshape, 0, 0);

	unsigned char crec[0x58];
	memset(crec, 0, sizeof(crec));
	const float kR2 = 1.5f;
	memcpy(crec + 0x4c, &kR2, 4);
	const unsigned short kGrp2 = 3u;
	memcpy(crec + 0x3c, &kGrp2, 2);

	nxGuardedBoxDtor((NxBoxDtorFn) capLoad, cshape, reinterpret_cast<unsigned>(crec));
	if(gDtorFaultCode)
		return nxFail("capsule loadFromDesc faulted");

	float gotRad = 0.0f;
	memcpy(&gotRad, cshape + 0xe0, 4);
	unsigned radBits = 0;
	memcpy(&radBits, &gotRad, 4);
	oCapsuleLoadRadBits = radBits;
	oracleDigest = nxFold(oracleDigest, radBits);
	printf("capload row=phys_fn_000989 rad=%08x\n", radBits);
	}
	// -----------------------------------------------------------------------
	// PLANE slot 12, phys_fn_001265: loadFromDesc.
	{
	typedef void (__thiscall* NxPlaneLoadFn)(void* self, const void* rec);
	NxPlaneLoadFn planeLoad = (NxPlaneLoadFn) (base + 0x00025460);

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char pshape[0x110];
	memset(pshape, 0xcd, sizeof(pshape));
	typedef void (__thiscall* NxCtorFnPL)(void* self, void* owner, unsigned argument);
	NxCtorFnPL planeCtorPL = (NxCtorFnPL) (base + 0x00024ed0);
	planeCtorPL(pshape, 0, 0);

	unsigned char prec[0x58];
	memset(prec, 0, sizeof(prec));
	const float kNY = 1.0f;
	memcpy(prec + 0x50, &kNY, 4);		// normal.y = 1
	const unsigned short kGp = 0u;
	memcpy(prec + 0x3c, &kGp, 2);

	nxGuardedBoxDtor((NxBoxDtorFn) planeLoad, pshape, reinterpret_cast<unsigned>(prec));
	if(gDtorFaultCode)
		return nxFail("plane loadFromDesc faulted");

	float gotNy = 0.0f;
	memcpy(&gotNy, pshape + 0xe4, 4);
	unsigned nyBits = 0;
	memcpy(&nyBits, &gotNy, 4);
	oPlaneLoadNyBits = nyBits;
	oracleDigest = nxFold(oracleDigest, nyBits);
	printf("planeload row=phys_fn_001265 ny=%08x\n", nyBits);
	}
	// -----------------------------------------------------------------------
	// MESH slot 12, phys_fn_001383: loadFromDesc. The record holds a
	// wrapper pointer; shape+0xe0 gets *(wrapper+4) and the refcount at
	// that inner object +0x74 is incremented (decoded from 46 B listing).
	{
	typedef void (__thiscall* NxMeshLoadFn)(void* self, const void* rec);
	NxMeshLoadFn meshLoad = (NxMeshLoadFn) (base + 0x00027e30);

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char mshape[0xe8];
	memset(mshape, 0xcd, sizeof(mshape));
	typedef void (__thiscall* NxCtorFnML)(void* self, void* owner, unsigned argument);
	NxCtorFnML meshCtorML = (NxCtorFnML) (base + 0x00027db0);
	meshCtorML(mshape, 0, 0);

	static unsigned char fwrapper[0x10];
	static unsigned char finner[0x100];
	memset(fwrapper, 0, sizeof(fwrapper));
	memset(finner, 0, sizeof(finner));
	void* innerPtr = finner;
	memcpy(fwrapper + 4, &innerPtr, 4);
	unsigned* refcnt = reinterpret_cast<unsigned*>(finner + 0x74);
	*refcnt = 0;
	void* wrapperPtr = fwrapper;

	unsigned char mrec[0x58];
	memset(mrec, 0, sizeof(mrec));
	memcpy(mrec + 0x4c, &wrapperPtr, 4);
	const unsigned short kGm = 4u;
	memcpy(mrec + 0x3c, &kGm, 2);

	nxGuardedBoxDtor((NxBoxDtorFn) meshLoad, mshape, reinterpret_cast<unsigned>(mrec));
	if(gDtorFaultCode)
		return nxFail("mesh loadFromDesc faulted");

	void* gotInner = nullptr;
	memcpy(&gotInner, mshape + 0xe0, 4);
	bool okM = gotInner == innerPtr && *refcnt == 1;
	oracleDigest = nxFold(oracleDigest, okM ? 1u : 0u);
	printf("meshload row=phys_fn_001383 bound=%u\n", okM ? 1u : 0u);
	}
	// -----------------------------------------------------------------------
	// BOX slot 12, phys_fn_000981: loadFromDesc (RVA 0x00021990, 55 B).
	{
	typedef void (__thiscall* NxBoxLoadFn)(void* self, const void* rec);
	NxBoxLoadFn boxLoad = (NxBoxLoadFn) (base + 0x00021990);

	if(!nxInstallAllocatorShim(base))
		return nxFail("the allocator holder word moved; re-pin the probe");

	unsigned char bshape[0x230];
	memset(bshape, 0xcd, sizeof(bshape));
	typedef void (__thiscall* NxCtorFnBL)(void* self, void* owner, unsigned argument);
	NxCtorFnBL boxCtorBL = (NxCtorFnBL) (base + 0x00021870);
	boxCtorBL(bshape, 0, 0);

	unsigned char brec[0x58];
	memset(brec, 0, sizeof(brec));
	const unsigned kDx = 0x3f800000u;
	const unsigned kDy = 0x40000000u;
	const unsigned kDz = 0x40400000u;
	memcpy(brec + 0x4c, &kDx, 4);
	memcpy(brec + 0x50, &kDy, 4);
	memcpy(brec + 0x54, &kDz, 4);
	const unsigned short kGb = 2u;
	memcpy(brec + 0x3c, &kGb, 2);

	nxGuardedBoxDtor((NxBoxDtorFn) boxLoad, bshape, reinterpret_cast<unsigned>(brec));
	if(gDtorFaultCode)
		return nxFail("box loadFromDesc faulted");

	unsigned gotX = 0; memcpy(&gotX, bshape + 0xe4, 4);
	unsigned gotY = 0; memcpy(&gotY, bshape + 0xe8, 4);
	bool okB = gotX == kDx && gotY == kDy;
	oracleDigest = nxFold(oracleDigest, okB ? 1u : 0u);
	printf("boxload row=phys_fn_000981 stored=%u\n", okB ? 1u : 0u);
	}

	// -----------------------------------------------------------------------
	// CAPSULE slot 8, phys_fn_001004: local AABB. Fresh capsule (all zeros).
	{
	typedef void (__thiscall* NxCapAABBFn)(void* self, float* out);
	NxCapAABBFn capAABB = (NxCapAABBFn) (base + 0x00021c80);

	unsigned char cshape[0xec];
	memset(cshape, 0xcd, sizeof(cshape));
	typedef void (__thiscall* NxCtorFn13)(void* self, void* owner, unsigned argument);
	NxCtorFn13 capsuleCtor6 = (NxCtorFn13) (base + 0x00021a60);
	capsuleCtor6(cshape, 0, 0);

	float out[6] = { 0, 0, 0, 0, 0, 0 };
	capAABB(cshape, out);

	unsigned da = 2166136261u;
	for(int i = 0; i < 6; ++i)
		{
		unsigned w; memcpy(&w, out + i, 4);
		da = nxFold(da, w);
		}
	oCapsuleAABBDigest = da;
	oracleDigest = nxFold(oracleDigest, da);

	unsigned ab[6] = { 0, 0, 0, 0, 0, 0 };
	for(int i = 0; i < 6; ++i)
		memcpy(&ab[i], out + i, 4);
	printf("capaabb row=phys_fn_001004 minmax=%08x.%08x.%08x.%08x.%08x.%08x\n",
		ab[0], ab[1], ab[2], ab[3], ab[4], ab[5]);
	}

	printf("layout coverage tables=%u colobj=1 owner=1 hull=1 shapebase=1 boxshape=1 sphere=1 capsule=1 plane=1 mesh=1 basevt=3 basesave=1 boxrow=6 planesave=1 sphererows=4 capsave=1 meshword=1 aabbrows=3 meshrows=2 sphlocal=1 setrad=1 capsetrad=1 planeext=1 sphdtor=1 capdtor=1 setgroup=1 dtors2=2 sphload=1 slot1wrapper=1 addthunk=1 shapeleaf=1\n",
		(unsigned) (sizeof(nxTables) / sizeof(nxTables[0])));
	printf("layout oracle digest=%08x\n", oracleDigest);

	// The candidate side. Families whose rows are transcribed answer through
	// the reconstruction; the rest stay CANDIDATE-MISSING and keep the gate
	// RED until their task lands.
	if(!selfOnly)
		{
		unsigned candidateFold = 2166136261u;

		// -- collision object: the transcription constructs the same
		// post-construction state. The vptr words are module-specific and are
		// folded only on the oracle side; what must match is zeroed +04, the
		// argument at BOTH +8 and +0x18, and a member present at +0xc.
		{
		const unsigned kArg = 0xa5a5a5a5u;
		CollisionObject object(reinterpret_cast<void*>(kArg));
		unsigned char bytes[0x1c];
		memcpy(bytes, &object, sizeof(bytes));
		unsigned w04, w08, w18, memberVptr;
		memcpy(&w04, bytes + 0x04, 4);
		memcpy(&w08, bytes + 0x08, 4);
		memcpy(&memberVptr, bytes + 0x0c, 4);
		memcpy(&w18, bytes + 0x18, 4);
		bool ok = w04 == 0 && w08 == kArg && w18 == kArg && memberVptr != 0;
		printf("colobj candidate ok=%u zero04=%08x arg8=%08x arg18=%08x member=%08x\n",
			ok ? 1u : 0u, w04, w08, w18, memberVptr);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 1u);
		}

		// -- owner accessor: same fake shape the oracle side used.
		{
		unsigned char fake[16];
		memset(fake, 0, sizeof(fake));
		const unsigned kMark = 0x13579bdfu;
		memcpy(fake + 0x04, &kMark, 4);
		const void* got = nxShapeOwner(fake);
		unsigned value;
		memcpy(&value, &got, 4);
		bool ok = value == kMark;
		printf("owner candidate ok=%u returned=%08x\n", ok ? 1u : 0u, value);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 2u);
		}

		// -- box hull facade: constants, pointer arithmetic and static-table
		// content must match the oracle's rows on a twin buffer.
		{
		BoxHullFacade facade;
		memset(&facade, 0xcd, sizeof(facade));
		bool ok = BoxHullFacade::kVertexCount == 8
			&& BoxHullFacade::kFaceCount == 6
			&& facade.vertices() == &facade.mVertices[0]
			&& facade.face(2) == &facade.mFaces[2];
		unsigned offFace = (unsigned) ((const unsigned char*) facade.face(2) - (const unsigned char*) &facade);
		unsigned offVerts = (unsigned) ((const unsigned char*) facade.vertices() - (const unsigned char*) &facade);
		ok = ok && offFace == 0x70 + 2 * 0x24 && offVerts == 0x10;
		for(int t = 0; t < 3; ++t)
			{
			// .rdata rvas 0x00122180 / 0x001221e0 / 0x00122240, twelve words each.
			const NxU32* mine[3] = { BoxHullFacade::edgeTable(), BoxHullFacade::faceCornerTable(), BoxHullFacade::adjacencyTable() };
			if(memcmp(mine[t], base + 0x00122180 + t * 0x60, 48) != 0)
				ok = false;
			}
		printf("hull candidate ok=%u face2_offset=%#x vertices_offset=%#x\n",
			ok ? 1u : 0u, offFace, offVerts);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 3u);
		}

		// -- support mapping: same seed, candidate method, bitwise compare.
		{
		BoxHullFacade seeded;
		memset(&seeded, 0xcd, sizeof(seeded));
		memcpy(seeded.mVertices, nxSupportPose, sizeof(nxSupportPose));
		float cMin = 0.0f, cMax = 0.0f;
		seeded.supportBounds(nxSupportDirection, &cMin, &cMax, nxSupportPose);
		memcpy(&cMinBits, &cMin, 4);
		memcpy(&cMaxBits, &cMax, 4);
		bool ok = memcmp(&oMinBits, &cMinBits, 4) == 0 && memcmp(&oMaxBits, &cMaxBits, 4) == 0;
		printf("hull support candidate ok=%u min_bits=%08x max_bits=%08x\n",
			ok ? 1u : 0u, cMinBits, cMaxBits);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 4u);
		}

		// -- shared hook: stable pointer, zero content, same shape as oracle.
		{
		const void* m1 = BoxHullFacade::sharedHook();
		const void* m2 = BoxHullFacade::sharedHook();
		unsigned words[3] = { 1, 1, 1 };
		if(m1)
			memcpy(words, m1, 12);
		bool ok = m1 != 0 && m1 == m2 && words[0] == 0 && words[1] == 0 && words[2] == 0;
		printf("hull sharedhook candidate ok=%u stable=%u\n",
			ok ? 1u : 0u, (m1 != 0 && m1 == m2) ? 1u : 0u);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 5u);
		}

		// -- base shape: the transcription's constructor must leave the same
		// post-construction words the oracle's does under the identical mask,
		// and the named fields must carry the marked values.
		{
		static const unsigned kPointerWords[] = { 0x00, 0xa4, 0xa8, 0xb0, 0xb4 };
		const unsigned kArg2 = 0x5a5a5a5au;
		ShapeBase shape(0, kArg2);
		unsigned char bytes[0xe0];
		memcpy(bytes, &shape, sizeof(bytes));

		unsigned digest = 2166136261u;
		for(unsigned i = 0; i < sizeof(bytes); i += 4)
			{
			bool pointer = false;
			for(size_t p = 0; p < sizeof(kPointerWords) / sizeof(kPointerWords[0]); ++p)
				if(kPointerWords[p] == i)
					pointer = true;
			if(pointer)
				continue;
			unsigned word;
			memcpy(&word, bytes + i, 4);
			digest = nxFold(digest, word);
			}

		unsigned w08, pose00c, w9c, wa0, prun24, prun28, sentinel, argD4, hwDC, hwDE;
		w08 = pose00c = w9c = wa0 = prun24 = prun28 = sentinel = argD4 = hwDC = hwDE = 0;
		memcpy(&w08, bytes + 0x08, 4);
		memcpy(&pose00c, bytes + 0x0c, 4);
		memcpy(&w9c, bytes + 0x9c, 4);
		memcpy(&wa0, bytes + 0xa0, 4);
		memcpy(&prun24, bytes + 0xc8, 4);
		memcpy(&prun28, bytes + 0xcc, 4);
		memcpy(&sentinel, bytes + 0xd0, 4);
		memcpy(&argD4, bytes + 0xd4, 4);
		memcpy(&hwDC, bytes + 0xdc, 2);
		memcpy(&hwDE, bytes + 0xde, 2);

		const unsigned oneBits = 0x3f800000u;
		bool ok = digest == oShapeBaseDigest
			&& w08 == 0 && pose00c == oneBits && w9c == 0 && wa0 == 0
			&& prun24 == 0xffffffffu && prun28 == 0x0000ffffu
			&& sentinel == 0x7fffffffu && argD4 == kArg2
			&& hwDC == 6 && hwDE == 8
			// and the owner registration: the shape is its prunable's owner.
			&& shape.mPrunable.mOwner == &shape;
		printf("shapebase candidate ok=%u digest=%08x owner_self=%u\n",
			ok ? 1u : 0u, digest, shape.mPrunable.mOwner == &shape ? 1u : 0u);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 6u);

		// -- slate 12 candidate: the same marked record through the
		// transcription, folded identically to the oracle drive.
		{
		unsigned char markedC[0xf0];
		memset(markedC, 0xcd, sizeof(markedC));
		const unsigned kMarkC = 0x7e7e0000u;
		for(unsigned i = 0; i < 12; ++i)
			{
			unsigned w = kMarkC + 0x1000u + i;
			memcpy(markedC + 0x30 + i * 4, &w, 4);
			}
		for(unsigned i = 0; i < 3; ++i)
			{
			unsigned w = kMarkC + 0x2000u + i;
			memcpy(markedC + 0xe4 + i * 4, &w, 4);
			}
		for(unsigned i = 0; i < 9; ++i)
			{
			unsigned w = kMarkC + 0x3000u + i;
			memcpy(markedC + 0x0c + i * 4, &w, 4);
			}
		unsigned short deBitsC = 0x0027u;
		memcpy(markedC + 0xde, &deBitsC, 2);

		unsigned dC = 2166136261u;
		const unsigned masksC[] = { 0x00000007u, 0x000000ffu, 0x0000ffffu,
			0x00000018u, 0xffffffffu };
		for(unsigned m = 0; m < 5; ++m)
			dC = nxFold(dC,
				reinterpret_cast<const ShapeBase*>(markedC)
					->nxFlagBitsDE(masksC[m]));
		unsigned recC[15];
		memset(recC, 0, sizeof(recC));
		reinterpret_cast<const BoxShape*>(markedC)
			->nxFillShapeDescriptor(recC);
		for(unsigned w = 0; w < 15; ++w)
			dC = nxFold(dC, recC[w]);

		// Independent edge checks: high-bit zero extension and ordered
		// transfers when the output aliases the source. A dimensions-first
		// copy or snapshot/memmove of the rotation must fail these cases.
		bool leafEdgesOk = true;
		typedef unsigned (__thiscall* LeafFlagsFn)(const void*, unsigned);
		LeafFlagsFn flagsOracle = reinterpret_cast<LeafFlagsFn>(
			const_cast<unsigned char*>(base) + 0x257d0);
		const unsigned short edgeBits[] = { 0, 0x8000, 0x80a7, 0xffff };
		for(unsigned e = 0; e < 4; ++e)
			{
			memcpy(markedC + 0xde, &edgeBits[e], 2);
			const ShapeBase* s = reinterpret_cast<const ShapeBase*>(markedC);
			leafEdgesOk = leafEdgesOk
				&& flagsOracle(markedC, 0xffffffffu) == edgeBits[e]
				&& s->nxFlagBitsDE(0xffffffffu) == edgeBits[e]
				&& flagsOracle(markedC, 0xffff0000u) == 0
				&& s->nxFlagBitsDE(0xffff0000u) == 0;
			}
		typedef void (__thiscall* LeafFillFn)(const void*, unsigned*);
		LeafFillFn fillOracle = reinterpret_cast<LeafFillFn>(
			const_cast<unsigned char*>(base) + 0x20490);
		const unsigned outputOffsets[] = { 0, 4, 0x0c, 0x24, 0xcc, 0xe4, 0x240 };
		for(unsigned c = 0; c < 7; ++c)
			{
			unsigned twinO[176], twinC[176];
			for(unsigned w = 0; w < 176; ++w)
				twinO[w] = twinC[w] = 0x6d000000u + w;
			fillOracle(twinO, twinO + outputOffsets[c] / 4);
			reinterpret_cast<const BoxShape*>(twinC)->nxFillShapeDescriptor(
				twinC + outputOffsets[c] / 4);
			if(memcmp(twinO, twinC, sizeof(twinO)) != 0)
				{
				printf("shapeleaf overlap mismatch offset=%x\n", outputOffsets[c]);
				leafEdgesOk = false;
				}
			}
		bool okL = dC == oSlate12Digest && leafEdgesOk;
		printf("shapeleaf candidate ok=%u digest=%08x\n", okL ? 1u : 0u, dC);
		if(!okL)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 65u);
		}
		}

		// -- box shape: the transcription's constructor over the same poisoned
		// twin buffer must match the oracle's masked fold, preserve the poison
		// the ctor never touches, zero the face pointer words, overwrite the
		// sentinel with 2, and build a live collision object pointing back.
		{
		static const unsigned kPointerWords[] = { 0x00, 0x9c, 0xa4, 0xa8, 0xb0, 0xb4, 0xe0 };
		const unsigned kArg2 = 0x5a5a5a5au;
		unsigned char bytes[0x228];
		memset(bytes, 0xcd, sizeof(bytes));
		BoxShape& shape = *new(bytes) BoxShape(0, kArg2);

		unsigned digest = 2166136261u;
		for(unsigned i = 0; i < sizeof(bytes); i += 4)
			{
			bool pointer = false;
			for(size_t p = 0; p < sizeof(kPointerWords) / sizeof(kPointerWords[0]); ++p)
				if(kPointerWords[p] == i)
					pointer = true;
			if(pointer)
				continue;
			unsigned word;
			memcpy(&word, bytes + i, 4);
			digest = nxFold(digest, word);
			}

		unsigned colobj = 0;
		memcpy(&colobj, bytes + 0x9c, 4);
		bool colobjOk = colobj != 0;
		unsigned colobjArg8 = 0, colobjArg18 = 0, colobjMember = 0;
		if(colobjOk)
			{
			memcpy(&colobjArg8, (unsigned char*) colobj + 0x08, 4);
			memcpy(&colobjArg18, (unsigned char*) colobj + 0x18, 4);
			memcpy(&colobjMember, (unsigned char*) colobj + 0x0c, 4);
			colobjOk = colobjArg8 == (unsigned) (uintptr_t) bytes
				&& colobjArg18 == (unsigned) (uintptr_t) bytes
				&& colobjMember != 0;
			}

		const unsigned kPoison = 0xcdcdcdcdu;
		const float kOne = 1.0f;
		float d0, d1, d2;
		memcpy(&d0, bytes + 0xe4, 4);
		memcpy(&d1, bytes + 0xe8, 4);
		memcpy(&d2, bytes + 0xec, 4);
		unsigned sentinelBox = 0;
		memcpy(&sentinelBox, bytes + 0xd0, 4);
		bool ok = digest == oBoxDigest
			&& sentinelBox == 2
			&& d0 == kOne && d1 == kOne && d2 == kOne
			&& memcmp(bytes + 0xf0, &kPoison, 4) == 0
			&& memcmp(bytes + 0x164, &kPoison, 4) == 0
			&& colobjOk
			// the hull's static tables still answer through the facade
			&& BoxHullFacade::kVertexCount == 8 && BoxHullFacade::kFaceCount == 6
			&& shape.mHull.face(2) == &shape.mHull.mFaces[2];
		printf("boxshape candidate ok=%u digest=%08x sentinel_d0=%u\n",
			ok ? 1u : 0u, digest, sentinelBox);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 7u);

		// Diagnostic only: +0x00 is excluded from the layout digest above.
		// Do not dispatch an unvalidated pointer. The previous dormant slot-5
		// probe dereferenced code bytes as a pointer and used the wrong ABI.
		// A future table gate must establish candidate ownership and supply
		// the full slot signature on a candidate-constructed object first.
		{
		unsigned vptr = 0;
		memcpy(&vptr, bytes + 0x00, 4);
		printf("boxvptr candidate word=%08x\n", vptr);
		printf("boxvptr candidate dispatch=unverified\n");

		// BOX slot-7 differential (corrected model). The oracle's arg2 is a
		// per-axis swept record (3z36): the box-side is proven over varied
		// shapes/poses/swept records. The exploratory record probes that
		// pinned this model are recorded in docs (3z36/3z37), not kept here.
		{
		typedef bool (__thiscall* BoxSweepOracleFn)(void*, void*, const void*);
		BoxSweepOracleFn oracleSweepD = reinterpret_cast<BoxSweepOracleFn>(base + 0x20b20);
		typedef void (__thiscall* BoxCtorD)(void*, void*, unsigned);
		BoxCtorD oracleCtorD = reinterpret_cast<BoxCtorD>(base + 0x21870);
		unsigned dFail = 0, dRun = 0;
		const float dShapes[2][3] = { {1.f,1.5f,2.f}, {4.f,65.f,7.f} };
		const float rotS2[9] = { 0,-1,0, 1,0,0, 0,0,1 };
		const float trnS2[3] = { 1.f, 2.f, 3.f };
		for(unsigned sh = 0; sh < 2; ++sh)
		for(unsigned pose = 0; pose < 3; ++pose)
			{
			unsigned char oS[0x228], cS[0x228];
			memset(oS, 0xcd, sizeof(oS)); memset(cS, 0xcd, sizeof(cS));
			BoxShape& cObj = *new(cS) BoxShape(0, 0);
			oracleCtorD(oS, 0, 0);
			const float dR0[9] = {1,0,0, 0,1,0, 0,0,1};
			const float dR2[9] = {0,0,1, 0,1,0, -1,0,0};
			const float dT0[3] = {0,0,0};
			const float* R = pose==0 ? (const float*)rotS2
				: pose==1 ? (const float*)dR0 : (const float*)dR2;
			const float* T = pose==0 ? (const float*)trnS2 : (const float*)dT0;
			memcpy(oS+0xe4, dShapes[sh], 12); memcpy(cS+0xe4, dShapes[sh], 12);
			memcpy(oS+0x0c, R, 36);           memcpy(cS+0x0c, R, 36);
			memcpy(oS+0x30, T, 12);           memcpy(cS+0x30, T, 12);
			for(unsigned sw = 0; sw < 8; ++sw)
				{
				float svec[3] = {1.f, 2.f, 3.f};
				if(sw==1) { svec[0]=2.f; svec[1]=0.f; svec[2]=0.f; }
				if(sw==2) { svec[0]=3.f; svec[1]=5.f; svec[2]=0.f; }
				if(sw==3) { svec[0]=0.f; svec[1]=0.f; svec[2]=0.f; }
				if(sw==4) { svec[0]=7.f; svec[1]=0.5f; svec[2]=100.f; }
				if(sw==5) { svec[0]=-3.f; svec[1]=-2.f; svec[2]=-1.f; }
				if(sw==6) { svec[0]=0.25f; svec[1]=-0.5f; svec[2]=4.f; }
				if(sw==7) { svec[0]=-1.5f; svec[1]=2.75f; svec[2]=-6.25f; }
				float outO=0.5f, outC=0.5f;
				const bool ro = oracleSweepD(oS, &outO, svec);
				const bool rc = cObj.nxBoxSweep(&outC, svec);
				++dRun;
				if(ro != rc || (ro && (outO != outC)))
					{
					++dFail;
					fprintf(stderr, "boxsweep sh=%u p=%u sw=%u ret=%u/%u out=%08x/%08x\n",
						sh, pose, sw, ro, rc, reinterpret_cast<unsigned&>(outO),
						reinterpret_cast<unsigned&>(outC));
					if(sh==1 && pose==1 && sw==0)
						fprintf(stderr, "  introspect svec=%g,%g,%g fresh:\n",
							svec[0],svec[1],svec[2]);
					}
				}
			}
		printf("boxsweep-diff candidate run=%u failures=%u provisional=1\n", dRun, dFail);
		if(dFail == 0)
			candidateFold = nxFold(candidateFold, 66u);	// registered box-side drive
		else
			++candidateMissing;
		}
		}
		}

		// -- sphere shape: identical contract to boxshape on a 0xe4 twin; the
		// ctor must zero the radius word, overwrite the sentinel with 1 and
		// build the collision object through the GENERIC row phys_fn_001193.
		{
		static const unsigned kPointerWords[] = { 0x00, 0x9c, 0xa4, 0xa8, 0xb0, 0xb4 };
		const unsigned kArg2 = 0x5a5a5a5au;
		unsigned char bytes[0xe4];
		memset(bytes, 0xcd, sizeof(bytes));
		SphereShape& shape = *new(bytes) SphereShape(0, kArg2);

		unsigned digest = 2166136261u;
		for(unsigned i = 0; i < sizeof(bytes); i += 4)
			{
			bool pointer = false;
			for(size_t p = 0; p < sizeof(kPointerWords) / sizeof(kPointerWords[0]); ++p)
				if(kPointerWords[p] == i)
					pointer = true;
			if(pointer)
				continue;
			unsigned word;
			memcpy(&word, bytes + i, 4);
			digest = nxFold(digest, word);
			}

		unsigned colobj = 0;
		memcpy(&colobj, bytes + 0x9c, 4);
		bool colobjOk = colobj != 0;
		unsigned colobjArg8 = 0, colobjArg18 = 0, colobjMember = 0;
		if(colobjOk)
			{
			memcpy(&colobjArg8, (unsigned char*) colobj + 0x08, 4);
			memcpy(&colobjArg18, (unsigned char*) colobj + 0x18, 4);
			memcpy(&colobjMember, (unsigned char*) colobj + 0x0c, 4);
			colobjOk = colobjArg8 == (unsigned) (uintptr_t) bytes
				&& colobjArg18 == (unsigned) (uintptr_t) bytes
				&& colobjMember != 0;
			}

		unsigned radiusWord = 0, sentinelSphere = 0;
		memcpy(&radiusWord, bytes + 0xe0, 4);
		memcpy(&sentinelSphere, bytes + 0xd0, 4);
		bool ok = digest == oSphereDigest
			&& radiusWord == 0 && sentinelSphere == 1 && colobjOk;
		printf("sphere candidate ok=%u digest=%08x sentinel_d0=%u\n",
			ok ? 1u : 0u, digest, sentinelSphere);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 8u);
		}

		// -- capsule shape: same contract on a 0xe8 twin; the ctor zeroes its
		// two data words and overwrites the sentinel with NX_SHAPE_CAPSULE=3.
		{
		static const unsigned kPointerWords[] = { 0x00, 0x9c, 0xa4, 0xa8, 0xb0, 0xb4 };
		const unsigned kArg2 = 0x5a5a5a5au;
		unsigned char bytes[0xec];
		memset(bytes, 0xcd, sizeof(bytes));
		CapsuleShape& shape = *new(bytes) CapsuleShape(0, kArg2);

		unsigned digest = 2166136261u;
		for(unsigned i = 0; i < sizeof(bytes); i += 4)
			{
			bool pointer = false;
			for(size_t p = 0; p < sizeof(kPointerWords) / sizeof(kPointerWords[0]); ++p)
				if(kPointerWords[p] == i)
					pointer = true;
			if(pointer)
				continue;
			unsigned word;
			memcpy(&word, bytes + i, 4);
			digest = nxFold(digest, word);
			}

		unsigned colobj = 0;
		memcpy(&colobj, bytes + 0x9c, 4);
		bool colobjOk = colobj != 0;
		unsigned colobjArg8 = 0, colobjArg18 = 0, colobjMember = 0;
		if(colobjOk)
			{
			memcpy(&colobjArg8, (unsigned char*) colobj + 0x08, 4);
			memcpy(&colobjArg18, (unsigned char*) colobj + 0x18, 4);
			memcpy(&colobjMember, (unsigned char*) colobj + 0x0c, 4);
			colobjOk = colobjArg8 == (unsigned) (uintptr_t) bytes
				&& colobjArg18 == (unsigned) (uintptr_t) bytes
				&& colobjMember != 0;
			}

		unsigned fE0 = 0, fE4 = 0, sentinelCapsule = 0;
		memcpy(&fE0, bytes + 0xe0, 4);
		memcpy(&fE4, bytes + 0xe4, 4);
		memcpy(&sentinelCapsule, bytes + 0xd0, 4);
		bool ok = digest == oCapsuleDigest
			&& fE0 == 0 && fE4 == 0 && sentinelCapsule == 3 && colobjOk;
		printf("capsule candidate ok=%u digest=%08x sentinel_d0=%u\n",
			ok ? 1u : 0u, digest, sentinelCapsule);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 10u);
		}

		// -- plane shape: same contract on a 0x10c twin; the ctor must plant
		// the default plane equation (normal (0,1,0), D=0), overwrite the
		// sentinel with NX_SHAPE_PLANE=0, fill the tangents through
		// NxNormalToTangents and set +0x108 to 1.
		{
		static const unsigned kPointerWords[] = { 0x00, 0x9c, 0xa4, 0xa8, 0xb0, 0xb4 };
		const unsigned kArg2 = 0x5a5a5a5au;
		unsigned char bytes[0x10c];
		memset(bytes, 0xcd, sizeof(bytes));
		PlaneShape& shape = *new(bytes) PlaneShape(0, kArg2);

		unsigned digest = 2166136261u;
		for(unsigned i = 0; i < sizeof(bytes); i += 4)
			{
			bool pointer = false;
			for(size_t p = 0; p < sizeof(kPointerWords) / sizeof(kPointerWords[0]); ++p)
				if(kPointerWords[p] == i)
					pointer = true;
			if(pointer)
				continue;
			unsigned word;
			memcpy(&word, bytes + i, 4);
			digest = nxFold(digest, word);
			}

		unsigned colobj = 0;
		memcpy(&colobj, bytes + 0x9c, 4);
		bool colobjOk = colobj != 0;
		unsigned colobjArg8 = 0, colobjArg18 = 0, colobjMember = 0;
		if(colobjOk)
			{
			memcpy(&colobjArg8, (unsigned char*) colobj + 0x08, 4);
			memcpy(&colobjArg18, (unsigned char*) colobj + 0x18, 4);
			memcpy(&colobjMember, (unsigned char*) colobj + 0x0c, 4);
			colobjOk = colobjArg8 == (unsigned) (uintptr_t) bytes
				&& colobjArg18 == (unsigned) (uintptr_t) bytes
				&& colobjMember != 0;
			}

		const unsigned kOneBits = 0x3f800000u, kZero = 0, kMinusOneBits = 0xbf800000u,
			kMinusZeroBits = 0x80000000u;
		unsigned nX = 0, nY = 0, nZ = 0, distEC = 0, word108 = 0, sentinelPlane = 0;
		memcpy(&nX, bytes + 0xe0, 4);
		memcpy(&nY, bytes + 0xe4, 4);
		memcpy(&nZ, bytes + 0xe8, 4);
		memcpy(&distEC, bytes + 0xec, 4);
		memcpy(&word108, bytes + 0x108, 4);
		memcpy(&sentinelPlane, bytes + 0xd0, 4);
		bool ok = digest == oPlaneDigest
			&& sentinelPlane == 0 && nX == kZero && nY == kOneBits && nZ == kZero
			&& distEC == kZero && word108 == 1 && colobjOk
			// exact arithmetic for the default normal: t1=(-1,0,0),
			// t2=(-n.z*t1.y, n.z*t1.x, a*k) = (-0,-0,1) by IEEE sign rules
			&& memcmp(bytes + 0xf0, &kMinusOneBits, 4) == 0
			&& memcmp(bytes + 0xf4, &kZero, 4) == 0
			&& memcmp(bytes + 0xf8, &kZero, 4) == 0
			&& memcmp(bytes + 0xfc, &kMinusZeroBits, 4) == 0
			&& memcmp(bytes + 0x100, &kMinusZeroBits, 4) == 0
			&& memcmp(bytes + 0x104, &kOneBits, 4) == 0;
		printf("plane candidate ok=%u digest=%08x sentinel_d0=%u\n",
			ok ? 1u : 0u, digest, sentinelPlane);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 9u);
		}

		// -- mesh shape: same contract on a 0xe8 twin; the ctor zeroes its two
		// data words and overwrites the sentinel with NX_SHAPE_MESH=4.
		{
		static const unsigned kPointerWords[] = { 0x00, 0x9c, 0xa4, 0xa8, 0xb0, 0xb4 };
		const unsigned kArg2 = 0x5a5a5a5au;
		unsigned char bytes[0xe8];
		memset(bytes, 0xcd, sizeof(bytes));
		MeshShape& shape = *new(bytes) MeshShape(0, kArg2);

		unsigned digest = 2166136261u;
		for(unsigned i = 0; i < sizeof(bytes); i += 4)
			{
			bool pointer = false;
			for(size_t p = 0; p < sizeof(kPointerWords) / sizeof(kPointerWords[0]); ++p)
				if(kPointerWords[p] == i)
					pointer = true;
			if(pointer)
				continue;
			unsigned word;
			memcpy(&word, bytes + i, 4);
			digest = nxFold(digest, word);
			}

		unsigned colobj = 0;
		memcpy(&colobj, bytes + 0x9c, 4);
		bool colobjOk = colobj != 0;
		unsigned colobjArg8 = 0, colobjArg18 = 0, colobjMember = 0;
		if(colobjOk)
			{
			memcpy(&colobjArg8, (unsigned char*) colobj + 0x08, 4);
			memcpy(&colobjArg18, (unsigned char*) colobj + 0x18, 4);
			memcpy(&colobjMember, (unsigned char*) colobj + 0x0c, 4);
			colobjOk = colobjArg8 == (unsigned) (uintptr_t) bytes
				&& colobjArg18 == (unsigned) (uintptr_t) bytes
				&& colobjMember != 0;
			}

		unsigned wE0 = 0, wE4 = 0, sentinelMesh = 0;
		memcpy(&wE0, bytes + 0xe0, 4);
		memcpy(&wE4, bytes + 0xe4, 4);
		memcpy(&sentinelMesh, bytes + 0xd0, 4);
		bool ok = digest == oMeshDigest
			&& wE0 == 0 && wE4 == 0 && sentinelMesh == 4 && colobjOk;
		printf("mesh candidate ok=%u digest=%08x sentinel_d0=%u\n",
			ok ? 1u : 0u, digest, sentinelMesh);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 11u);
		}

		// -- base vtable stubs: the transcription's members must answer the
		// marked arguments exactly as the oracle rows do.
		{
		const unsigned kA1 = 0x11111111u, kA2 = 0x22222222u,
			kA3 = 0x33333333u, kA4 = 0x44444444u;
		ShapeBase shape(0, 0);
		bool c4 = shape.nxBaseSlot4((void*) kA1, (void*) kA2);
		void* c5 = shape.nxBaseSlot5((void*) kA1, (void*) kA2, (void*) kA3, (void*) kA4);
		bool c7 = shape.nxBaseSlot7((void*) kA1);
		unsigned c5bits = 0;
		memcpy(&c5bits, &c5, 4);
		bool ok = !c4 && c5bits == 0 && !c7;
		printf("basevt candidate ok=%u ret4=%u ret5=%08x ret7=%u\n",
			ok ? 1u : 0u, c4 ? 1u : 0u, c5bits, c7 ? 1u : 0u);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 12u);
		}

		// -- base save-state: the transcription fills a poisoned twin record
		// from a constructed sphere; every byte must match the oracle's.
		{
		unsigned char bytes[0xe4];
		memset(bytes, 0xcd, sizeof(bytes));
		SphereShape& shape = *new(bytes) SphereShape(0, 0);

		unsigned char record[0x48];
		memset(record, 0xcd, sizeof(record));
		bool saved = shape.mBase.nxBaseSaveState(record);

		unsigned digest = 2166136261u;
		for(unsigned i = 0; i < sizeof(record); i += 4)
			{
			unsigned word;
			memcpy(&word, record + i, 4);
			digest = nxFold(digest, word);
			}

		const unsigned kOneBits = 0x3f800000u, kPoison = 0xcdcdcdcdu, kEight = 8u;
		unsigned poseDiag = 0, word38 = 0, word40 = 0, poisonHead = 0;
		memcpy(&poseDiag, record + 8, 4);
		memcpy(&word38, record + 0x38, 4);
		memcpy(&word40, record + 0x40, 4);
		memcpy(&poisonHead, record, 4);
		bool ok = saved && digest == oSaveStateDigest
			&& poseDiag == kOneBits && word38 == kEight && word40 == 0
			&& poisonHead == kPoison;	// bytes the row never touches stay poison
		if(!ok && digest != oSaveStateDigest)
			{
			for(unsigned i = 0; i < sizeof(record); ++i)
				if(record[i] != sSaveStateRecord[i])
					{
					fprintf(stderr, "FAIL basesave first mismatch at record+%#x: "
						"cand=%02x oracle=%02x\n", i, record[i], sSaveStateRecord[i]);
					break;
					}
			}
		printf("basesave candidate ok=%u digest=%08x\n", ok ? 1u : 0u, digest);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 13u);
		}

		// -- box row 10: the transcription's member must reproduce the oracle
		// words bitwise on a twin constructed box.
		{
		unsigned char bytes[0x228];
		memset(bytes, 0xcd, sizeof(bytes));
		BoxShape& shape = *new(bytes) BoxShape(0, 0);

		float out[4] = { 0, 0, 0, 0 };
		shape.nxBoxCenterAndDiagonal(out);

		unsigned digest = 2166136261u;
		for(int i = 0; i < 4; ++i)
			{
			unsigned word;
			memcpy(&word, out + i, 4);
			digest = nxFold(digest, word);
			}
		bool ok = digest == oBoxRowDigest;
		printf("boxrow candidate ok=%u digest=%08x\n", ok ? 1u : 0u, digest);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 14u);
		}

		// -- box rows 11 and 13: twin drives through the transcription.
		{
		unsigned char bytes[0x228];
		memset(bytes, 0xcd, sizeof(bytes));
		BoxShape& shape = *new(bytes) BoxShape(0, 0);

		float out11[4] = { 1, 1, 1, 1 };
		shape.nxBoxZeroCenterAndDiagonal(out11);
		unsigned d11 = 2166136261u;
		for(int i = 0; i < 4; ++i)
			{
			unsigned w; memcpy(&w, out11 + i, 4);
			d11 = nxFold(d11, w);
			}

		unsigned char record[0x58];
		memset(record, 0xcd, sizeof(record));
		bool saved13 = shape.nxBoxSaveState(record);
		unsigned d13 = 2166136261u;
		for(unsigned i = 0; i < sizeof(record); i += 4)
			{
			unsigned w; memcpy(&w, record + i, 4);
			d13 = nxFold(d13, w);
			}

		const unsigned kOneBits = 0x3f800000u;
		unsigned dims4c = 0;
		memcpy(&dims4c, record + 0x4c, 4);
		bool ok = d11 == oBoxSlot11Digest && d13 == oBoxSlot13Digest && saved13
			&& out11[0] == 0.0f && out11[1] == 0.0f && out11[2] == 0.0f
			&& dims4c == kOneBits
			// the base save-state half of slot 13 still fills record+8..+0x40
			&& memcmp(record + 8, sSaveStateRecord + 8, 0x38) == 0;
		printf("boxrow2 candidate ok=%u d11=%08x d13=%08x\n",
			ok ? 1u : 0u, d11, d13);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 15u);
		}

		// -- box rows 8 and 9: twin drives through the transcription.
		{
		unsigned char bytes[0x228];
		memset(bytes, 0xcd, sizeof(bytes));
		BoxShape& shape = *new(bytes) BoxShape(0, 0);

		float out8[6] = { 0, 0, 0, 0, 0, 0 };
		shape.nxBoxLocalAABB(out8);
		float out9[6] = { 0, 0, 0, 0, 0, 0 };
		shape.nxBoxWorldAABB(out9);

		unsigned d8 = 2166136261u;
		for(int i = 0; i < 6; ++i)
			{
			unsigned w; memcpy(&w, out8 + i, 4);
			d8 = nxFold(d8, w);
			}
		unsigned d9 = 2166136261u;
		for(int i = 0; i < 6; ++i)
			{
			unsigned w; memcpy(&w, out9 + i, 4);
			d9 = nxFold(d9, w);
			}

		const unsigned kOneBits = 0x3f800000u, kMinusOneBits = 0xbf800000u;
		bool ok = d8 == oBoxSlot8Digest && d9 == oBoxSlot9Digest
			&& out8[0] == -1.0f && out8[3] == 1.0f
			&& memcmp(&out9[0], &kMinusOneBits, 4) == 0
			&& memcmp(&out9[3], &kOneBits, 4) == 0;
		printf("boxrow3 candidate ok=%u d8=%08x d9=%08x\n",
			ok ? 1u : 0u, d8, d9);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 16u);
		}

		// -- box identity row: the member returns this on both sides.
		{
		unsigned char bytes[0x228];
		memset(bytes, 0xcd, sizeof(bytes));
		BoxShape& shape = *new(bytes) BoxShape(0, 0);

		bool ok = shape.nxBoxSelf() == &shape;
		printf("boxrow4 candidate ok=%u\n", ok ? 1u : 0u);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 17u);
		}

		// -- box dtor: the transcription's member must leave the same
		// post-dtor words under the mask (flag=0 path). +0xe0 masked: the
		// dtor dance restores the facade vptr word.
		{
		static const unsigned kPointerWords[] = { 0x00, 0x9c, 0xa4, 0xa8, 0xb0, 0xb4, 0xe0 };
		unsigned char bytes[0x228];
		memset(bytes, 0xcd, sizeof(bytes));
		BoxShape& shape = *new(bytes) BoxShape(0, 0);
		shape.nxBoxScalarDeletingDtor(0);

		unsigned digest = 2166136261u;
		for(unsigned i = 0; i < sizeof(bytes); i += 4)
			{
			bool pointer = false;
			for(size_t p = 0; p < sizeof(kPointerWords) / sizeof(kPointerWords[0]); ++p)
				if(kPointerWords[p] == i)
					pointer = true;
			if(pointer)
				continue;
			unsigned word;
			memcpy(&word, bytes + i, 4);
			digest = nxFold(digest, word);
			}
		bool ok = digest == oBoxDtorDigest;
		if(!ok)
			for(unsigned i = 0; i < sizeof(bytes); i += 4)
				{
				bool pointer = false;
				for(size_t p = 0; p < sizeof(kPointerWords) / sizeof(kPointerWords[0]); ++p)
					if(kPointerWords[p] == i)
						pointer = true;
				if(pointer)
					continue;
				if(memcmp(bytes + i, sBoxDtorReference + i, 4) != 0)
					{
					unsigned cb = 0, ob = 0;
					memcpy(&cb, bytes + i, 4);
					memcpy(&ob, sBoxDtorReference + i, 4);
					fprintf(stderr, "FAIL boxdtor first unmasked mismatch at +%#x: cand=%08x oracle=%08x\n",
						i, cb, ob);
					break;
					}
				}
		printf("boxdtor candidate ok=%u digest=%08x\n", ok ? 1u : 0u, digest);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 18u);
		}

		// -- plane save-state: twin drive; the negated-D word is asserted.
		{
		unsigned char bytes[0x10c];
		memset(bytes, 0xcd, sizeof(bytes));
		PlaneShape& shape = *new(bytes) PlaneShape(0, 0);

		unsigned char record[0x58];
		memset(record, 0xcd, sizeof(record));
		bool saved = shape.nxPlaneSaveState(record);

		unsigned digest = 2166136261u;
		for(unsigned i = 0; i < sizeof(record); i += 4)
			{
			unsigned word;
			memcpy(&word, record + i, 4);
			digest = nxFold(digest, word);
			}

		const unsigned kOneBits = 0x3f800000u;
		unsigned normalY = 0, negD = 0;
		memcpy(&normalY, record + 0x50, 4);
		memcpy(&negD, record + 0x58, 4);
		// -(+0.0f) carries the sign bit: the negated distance is -0.0f.
		bool ok = saved && digest == oPlaneSaveDigest
			&& normalY == kOneBits && negD == 0x80000000u;
		printf("planesave candidate ok=%u digest=%08x\n", ok ? 1u : 0u, digest);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 19u);
		}

		// -- sphere rows 15/13/11/10: twin drives through the transcription.
		{
		unsigned char bytes[0xe4];
		memset(bytes, 0xcd, sizeof(bytes));
		SphereShape& shape = *new(bytes) SphereShape(0, 0);

		float rad = shape.nxSphereGetRadius();
		unsigned radBits = 0;
		memcpy(&radBits, &rad, 4);

		unsigned char record[0x58];
		memset(record, 0xcd, sizeof(record));
		bool saved13 = shape.nxSphereSaveState(record);

		float out11[4] = { 1, 1, 1, 1 };
		shape.nxSphereZeroCenterRadius(out11);
		float out10[4] = { 1, 1, 1, 1 };
		shape.nxSphereCenterRadius(out10);

		unsigned d13 = 2166136261u;
		for(unsigned i = 0; i < sizeof(record); i += 4)
			{
			unsigned w; memcpy(&w, record + i, 4);
			d13 = nxFold(d13, w);
			}

		const unsigned kZero = 0;
		bool ok = d13 == oSphereRowsDigest && radBits == oSphereRadBits
			&& saved13
			&& out11[0] == 0.0f && out11[1] == 0.0f && out11[2] == 0.0f
			&& out11[3] == 0.0f
			&& out10[0] == 0.0f && out10[1] == 0.0f && out10[2] == 0.0f
			&& out10[3] == 0.0f;
		printf("sphererows candidate ok=%u d13=%08x rad=%08x\n",
			ok ? 1u : 0u, d13, radBits);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 20u);
		}

		// -- capsule save-state and mesh word: twin drives.
		{
		unsigned char cbytes[0xec];
		memset(cbytes, 0xcd, sizeof(cbytes));
		CapsuleShape& cap = *new(cbytes) CapsuleShape(0, 0);

		unsigned char record[0x58];
		memset(record, 0xcd, sizeof(record));
		bool saved = cap.nxCapsuleSaveState(record);

		unsigned digest = 2166136261u;
		for(unsigned i = 0; i < sizeof(record); i += 4)
			{
			unsigned w; memcpy(&w, record + i, 4);
			digest = nxFold(digest, w);
			}
		const unsigned kZero = 0;
		unsigned rad4c = 0, hgt50 = 0;
		memcpy(&rad4c, record + 0x4c, 4);
		memcpy(&hgt50, record + 0x50, 4);
		bool okCap = saved && digest == oCapsuleSaveDigest
			&& rad4c == kZero && hgt50 == kZero;
		if(digest != oCapsuleSaveDigest)
			for(unsigned i = 0; i < sizeof(record); ++i)
				if(record[i] != sCapsuleSaveReference[i])
					{
					fprintf(stderr, "FAIL capsave first mismatch at +%#x: cand=%02x oracle=%02x\n",
						i, record[i], sCapsuleSaveReference[i]);
					break;
					}

		unsigned char mbytes[0xe8];
		memset(mbytes, 0xcd, sizeof(mbytes));
		MeshShape& mesh = *new(mbytes) MeshShape(0, 0);
		static unsigned char fakeMesh2[0xe8];
		memset(fakeMesh2, 0, sizeof(fakeMesh2));
		const unsigned kMark = 0x13572468u;
		memcpy(fakeMesh2 + 0xe4, &kMark, 4);
		mesh.mWordE0 = reinterpret_cast<NxU32>(fakeMesh2);
		bool okMesh = mesh.nxMeshGetMeshWord() == kMark;

		printf("morerows candidate okCap=%u okMesh=%u\n",
			okCap ? 1u : 0u, okMesh ? 1u : 0u);
		bool ok = okCap && okMesh;
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 21u);
		}

		// -- aabb rows: sphere world AABB and capsule center+radius twins.
		{
		unsigned char sbytes[0xe4];
		memset(sbytes, 0xcd, sizeof(sbytes));
		SphereShape& sph = *new(sbytes) SphereShape(0, 0);

		unsigned char cbytes2[0xec];
		memset(cbytes2, 0xcd, sizeof(cbytes2));
		CapsuleShape& cap = *new(cbytes2) CapsuleShape(0, 0);

		float so[6] = { 0, 0, 0, 0, 0, 0 };
		sph.nxSphereWorldAABB(so);
		float c10[4] = { 0, 0, 0, 0 };
		cap.nxCapsuleCenterRadius(c10);
		float c11[4] = { 0, 0, 0, 0 };
		cap.nxCapsuleZeroCenterRadius(c11);

		unsigned d9 = 2166136261u;
		for(int i = 0; i < 6; ++i)
			{
			unsigned w; memcpy(&w, so + i, 4);
			d9 = nxFold(d9, w);
			}
		unsigned dc = 2166136261u;
		for(int i = 0; i < 4; ++i)
			{
			unsigned w; memcpy(&w, c10 + i, 4);
			dc = nxFold(dc, w);
			}
		for(int i = 0; i < 4; ++i)
			{
			unsigned w; memcpy(&w, c11 + i, 4);
			dc = nxFold(dc, w);
			}
		bool ok = d9 == oSphereAABBDigest && dc == oCapsuleCRDigest
			&& so[0] == 0.0f && so[5] == 0.0f
			&& c10[3] == 0.0f && c11[3] == 0.0f;
		printf("aabbrows candidate ok=%u d9=%08x dc=%08x\n",
			ok ? 1u : 0u, d9, dc);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 22u);
		}

		// -- mesh rows 13 and 11: twin drives with the same planted record.
		{
		unsigned char mbytes[0xe8];
		memset(mbytes, 0xcd, sizeof(mbytes));
		MeshShape& mesh = *new(mbytes) MeshShape(0, 0);

		static unsigned char fm2[0xe8];
		memset(fm2, 0, sizeof(fm2));
		const unsigned kM4 = 0x2468ace0u, kW0 = 0x0badf00du, kW3 = 0x13579bdfu;
		memcpy(fm2 + 0xe4, &kM4, 4);
		memcpy(fm2 + 0x5c, &kW0, 4);
		memcpy(fm2 + 0x68, &kW3, 4);
		mesh.mWordE0 = reinterpret_cast<NxU32>(fm2);
		const unsigned kFlags = 0x5a5a5a5au;
		memcpy(&mesh.mWordE4, &kFlags, 4);

		unsigned char record[0x58];
		memset(record, 0xcd, sizeof(record));
		bool saved = mesh.nxMeshSaveState(record);
		unsigned words[4] = { 0, 0, 0, 0 };
		mesh.nxMeshGetWords5C(words);

		unsigned d13m = 2166136261u;
		for(unsigned i = 0; i < sizeof(record); i += 4)
			{
			unsigned w; memcpy(&w, record + i, 4);
			d13m = nxFold(d13m, w);
			}
		unsigned dw = 2166136261u;
		for(int i = 0; i < 4; ++i)
			dw = nxFold(dw, words[i]);

		bool ok = d13m == oMeshSaveDigest && dw == oMeshWordsDigest && saved
			&& words[0] == kW0 && words[3] == kW3;
		printf("meshrows candidate ok=%u d13=%08x dw=%08x\n",
			ok ? 1u : 0u, d13m, dw);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 23u);
		}

		// -- mesh slot 8 words-0x44: twin drive.
		{
		unsigned char mb[0xe8];
		memset(mb, 0xcd, sizeof(mb));
		MeshShape& mesh2 = *new(mb) MeshShape(0, 0);
		static unsigned char fm44[0x100];
		memset(fm44, 0, sizeof(fm44));
		const unsigned kM44v = 0xdeadbeefu;
		const unsigned kM48v = 0xcafebabeu;
		memcpy(fm44 + 0x44, &kM44v, 4);
		memcpy(fm44 + 0x48, &kM48v, 4);
		const unsigned kM4cv = 0x12345678u;
		memcpy(fm44 + 0x4c, &kM4cv, 4);
		mesh2.mWordE0 = reinterpret_cast<NxU32>(fm44);
		unsigned out44[6] = { 0, 0, 0, 0, 0, 0 };
		mesh2.nxMeshGetWords44(out44);
		unsigned d44 = 2166136261u;
		for(int i = 0; i < 6; ++i)
			{
			unsigned w; memcpy(&w, out44 + i, 4);
			d44 = nxFold(d44, w);
			}
		bool ok44 = d44 == oMesh44Digest && out44[0] == kM44v;
		printf("meshwords44 candidate ok=%u d44=%08x\n", ok44 ? 1u : 0u, d44);
		if(!ok44)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 31u);
		}

		// -- sphere local AABB: the transcription's negations must produce the
		// same -0.0f sign bits the oracle does.
		{
		unsigned char sbytes[0xe4];
		memset(sbytes, 0xcd, sizeof(sbytes));
		SphereShape& sph = *new(sbytes) SphereShape(0, 0);

		float lo[6] = { 0, 0, 0, 0, 0, 0 };
		sph.nxSphereLocalAABB(lo);

		unsigned d8s = 2166136261u;
		for(int i = 0; i < 6; ++i)
			{
			unsigned w; memcpy(&w, lo + i, 4);
			d8s = nxFold(d8s, w);
			}
		const unsigned kMinusZero = 0x80000000u;
		bool ok = d8s == oSphereLocalDigest
			&& memcmp(&lo[0], &kMinusZero, 4) == 0
			&& memcmp(&lo[1], &kMinusZero, 4) == 0
			&& memcmp(&lo[2], &kMinusZero, 4) == 0
			&& lo[3] == 0.0f && lo[4] == 0.0f && lo[5] == 0.0f;
		printf("sphlocal candidate ok=%u d8=%08x\n", ok ? 1u : 0u, d8s);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 24u);
		}

		// -- plane and mesh dtors: transcription members leave the same
		// post-dtor words under the mask (flag=0 path).
		{
		unsigned char pbytes[0x10c];
		memset(pbytes, 0xcd, sizeof(pbytes));
		PlaneShape& plane = *new(pbytes) PlaneShape(0, 0);
		plane.nxPlaneScalarDeletingDtor(0);

		unsigned dpl = 2166136261u;
		for(unsigned i = 0; i < sizeof(pbytes); i += 4)
			{
			bool pointer = false;
			for(size_t p = 0; p < 6; ++p)
				if(kDtorMask[p] == i)
					pointer = true;
			if(pointer)
				continue;
			unsigned w;
			memcpy(&w, pbytes + i, 4);
			dpl = nxFold(dpl, w);
			}

		unsigned char mbytes[0xe8];
		memset(mbytes, 0xcd, sizeof(mbytes));
		MeshShape& mesh = *new(mbytes) MeshShape(0, 0);
		mesh.nxMeshScalarDeletingDtor(0);

		unsigned dm = 2166136261u;
		for(unsigned i = 0; i < sizeof(mbytes); i += 4)
			{
			bool pointer = false;
			for(size_t p = 0; p < 6; ++p)
				if(kDtorMask[p] == i)
					pointer = true;
			if(pointer)
				continue;
			unsigned w;
			memcpy(&w, mbytes + i, 4);
			dm = nxFold(dm, w);
			}

		bool ok = dpl == oPlaneDtorDigest && dm == oMeshDtorDigest;
		printf("dtors2 candidate ok=%u plane=%08x mesh=%08x\n",
			ok ? 1u : 0u, dpl, dm);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 26u);
		}

		// -- capsule slot 8 local AABB: twin drive.
		{
		unsigned char cbytes[0xec];
		memset(cbytes, 0xcd, sizeof(cbytes));
		CapsuleShape& cap = *new(cbytes) CapsuleShape(0, 0);
		float aout[6] = { 0, 0, 0, 0, 0, 0 };
		cap.nxCapsuleLocalAABB(aout);
		unsigned da = 2166136261u;
		for(int i = 0; i < 6; ++i)
			{
			unsigned w; memcpy(&w, aout + i, 4);
			da = nxFold(da, w);
			}
		bool okA = da == oCapsuleAABBDigest;
		printf("capaabb candidate ok=%u da=%08x\n", okA ? 1u : 0u, da);
		if(!okA)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 29u);
		}
		// -- capsule loadFromDesc: twin drive through transcription.
		{
		unsigned char cb[0xec];
		memset(cb, 0xcd, sizeof(cb));
		CapsuleShape& capL = *new(cb) CapsuleShape(0, 0);
		unsigned char cr[0x58];
		memset(cr, 0, sizeof(cr));
		const float kR3 = 1.5f;
		memcpy(cr + 0x4c, &kR3, 4);
		const unsigned short kG3 = 3u;
		memcpy(cr + 0x3c, &kG3, 2);
		capL.mBase.nxApplyDescriptor(cr);
		capL.nxCapsuleLoadFromDesc(cr);
		float gr2 = 0.0f;
		memcpy(&gr2, cb + 0xe0, 4);
		bool okCL = gr2 == kR3;
		printf("capload candidate ok=%u rad=%08x\n", okCL ? 1u : 0u, *reinterpret_cast<unsigned*>(&gr2));
		if(!okCL)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 36u);
		}
		// -- plane loadFromDesc: twin drive through transcription.
		{
		unsigned char pb[0x110];
		memset(pb, 0xcd, sizeof(pb));
		PlaneShape& pln = *new(pb) PlaneShape(0, 0);
		unsigned char pr[0x58];
		memset(pr, 0, sizeof(pr));
		const float kNY2 = 1.0f;
		memcpy(pr + 0x50, &kNY2, 4);
		pln.mBase.nxApplyDescriptor(pr);
		pln.nxPlaneLoadFromDesc(pr);
		float gny = 0.0f;
		memcpy(&gny, pb + 0xe4, 4);
		bool okPL = gny == kNY2 && *reinterpret_cast<unsigned*>(&gny) == oPlaneLoadNyBits;
		printf("planeload candidate ok=%u ny=%08x\n", okPL ? 1u : 0u, *reinterpret_cast<unsigned*>(&gny));
		if(!okPL)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 37u);
		}
		// -- mesh loadFromDesc: twin drive through transcription.
		{
		unsigned char mb[0xe8];
		memset(mb, 0xcd, sizeof(mb));
		MeshShape& mshL = *new(mb) MeshShape(0, 0);
		static unsigned char fw2[0x10];
		static unsigned char fi2[0x100];
		memset(fw2, 0, sizeof(fw2));
		memset(fi2, 0, sizeof(fi2));
		void* ip2 = fi2;
		memcpy(fw2 + 4, &ip2, 4);
		unsigned* rc2 = reinterpret_cast<unsigned*>(fi2 + 0x74);
		*rc2 = 0;
		void* wp2 = fw2;
		unsigned char mr2[0x58];
		memset(mr2, 0, sizeof(mr2));
		memcpy(mr2 + 0x4c, &wp2, 4);
		mshL.mBase.nxApplyDescriptor(mr2);
		bool okML = mshL.nxMeshLoadFromDesc(mr2);
		void* gi2 = nullptr;
		memcpy(&gi2, mb + 0xe0, 4);
		bool okM2 = okML && gi2 == ip2 && *rc2 == 1;
		printf("meshload candidate ok=%u\n", okM2 ? 1u : 0u);
		if(!okM2)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 38u);
		}
		// -- box loadFromDesc: twin drive through transcription.
		{
		unsigned char bb[0x230];
		memset(bb, 0xcd, sizeof(bb));
		BoxShape& bxL = *new(bb) BoxShape(0, 0);
		unsigned char br[0x58];
		memset(br, 0, sizeof(br));
		const unsigned kDx3 = 0x3f800000u;
		memcpy(br + 0x4c, &kDx3, 4);
		bxL.mBase.nxApplyDescriptor(br);
		bxL.nxBoxLoadFromDesc(br);
		float gx = 0.0f;
		memcpy(&gx, bb + 0xe4, 4);
		bool okBL = *reinterpret_cast<unsigned*>(&gx) == kDx3;
		printf("boxload candidate ok=%u\n", okBL ? 1u : 0u);
		if(!okBL)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 39u);
		}
		// -- base slot 6 owner-update: detached no-op, twin drive.
		{
		unsigned char obytes[0xe4];
		memset(obytes, 0xcd, sizeof(obytes));
		SphereShape& sphOU = *new(obytes) SphereShape(0, 0);
		unsigned char preOU[16];
		memcpy(preOU, obytes, sizeof(preOU));
		sphOU.mBase.nxApplyOwnerUpdate(0);
		bool okOU = memcmp(preOU, obytes, sizeof(preOU)) == 0;
		printf("ownerupd candidate ok=%u\n", okOU ? 1u : 0u);
		if(!okOU)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 40u);
		}
		// -- sphere slot 4 compute-mass: twin drive through transcription.
		// Same two drives as the oracle block, same fold order.
		{
		unsigned char sbytesM[0xe4];
		memset(sbytesM, 0xcd, sizeof(sbytesM));
		SphereShape& sphM = *new(sbytesM) SphereShape(0, 0);
		unsigned char dA[0x34];
		memset(dA, 0, sizeof(dA));
		sphM.nxSphereComputeMassFrame(reinterpret_cast<MassFrame*>(dA), 2.0f, 2.5f, 0);
		unsigned char dB[0x34];
		memset(dB, 0, sizeof(dB));
		sphM.nxSphereComputeMassFrame(reinterpret_cast<MassFrame*>(dB), 1.0f, 2.5f, 0);

		unsigned md2 = 2166136261u;
		for(int v = 0; v < 2; ++v)
			{
			const unsigned char* src = (v == 0) ? dA : dB;
			for(int i = 0; i < 0x34; i += 4)
				{
				unsigned w;
				memcpy(&w, src + i, 4);
				md2 = nxFold(md2, w);
				}
			}
		bool okMF = md2 == oMassFrameDigest;
		printf("massframe candidate ok=%u digest=%08x\n", okMF ? 1u : 0u, md2);
		if(!okMF)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 41u);
		}
		// -- box slot 4 compute-mass: twin drive through transcription.
		{
		unsigned char bbytesM[0x228];
		memset(bbytesM, 0xcd, sizeof(bbytesM));
		BoxShape& bxsM = *new(bbytesM) BoxShape(0, 0);
		static const float kHalfExt2[3] = { 1.5f, 2.0f, 2.5f };
		unsigned char dC[0x34];
		memset(dC, 0, sizeof(dC));
		bxsM.nxBoxComputeMassFrame(reinterpret_cast<MassFrame*>(dC), 2.0f, kHalfExt2, 0);
		unsigned char dD[0x34];
		memset(dD, 0, sizeof(dD));
		bxsM.nxBoxComputeMassFrame(reinterpret_cast<MassFrame*>(dD), 1.0f, kHalfExt2, 0);

		unsigned bd2 = 2166136261u;
		for(int v = 0; v < 2; ++v)
			{
			const unsigned char* src = (v == 0) ? dC : dD;
			for(int i = 0; i < 0x34; i += 4)
				{
				unsigned w;
				memcpy(&w, src + i, 4);
				bd2 = nxFold(bd2, w);
				}
			}
		bool okBM = bd2 == oBoxMassDigest;
		printf("boxmass candidate ok=%u digest=%08x\n", okBM ? 1u : 0u, bd2);
		if(!okBM)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 42u);
		}
		// -- capsule slot 4 compute-mass: twin drive through transcription.
		{
		unsigned char cbytesM[0xec];
		memset(cbytesM, 0xcd, sizeof(cbytesM));
		CapsuleShape& capM = *new(cbytesM) CapsuleShape(0, 0);
		unsigned char dE[0x34];
		memset(dE, 0, sizeof(dE));
		capM.nxCapsuleComputeMassFrame(reinterpret_cast<MassFrame*>(dE), 2.0f, 2u, 1.25f, 2.0f, 0);
		unsigned char dF[0x34];
		memset(dF, 0, sizeof(dF));
		capM.nxCapsuleComputeMassFrame(reinterpret_cast<MassFrame*>(dF), 1.0f, 0u, 1.25f, 2.0f, 0);

		unsigned cd2 = 2166136261u;
		for(int v = 0; v < 2; ++v)
			{
			const unsigned char* src = (v == 0) ? dE : dF;
			for(int i = 0; i < 0x34; i += 4)
				{
				unsigned w;
				memcpy(&w, src + i, 4);
				cd2 = nxFold(cd2, w);
				}
			}
		bool okCM = cd2 == oCapMassDigest;
		printf("capmass candidate ok=%u digest=%08x\n", okCM ? 1u : 0u, cd2);
		if(!okCM)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 43u);
		}
		// -- payload fold step: twin drive through transcription. Same two
		// crafted pairs as the oracle block, same fold order.
		{
		unsigned char frO2[0x34];
		memset(frO2, 0, sizeof(frO2));
		unsigned char payQ[0x24];
		memset(payQ, 0, sizeof(payQ));
		{
		const float fi[12] = { 1.5f, -0.5f, 0.25f, -0.5f, 2.0f, 0.75f,
			0.25f, 0.75f, 3.0f, 0.5f, -0.25f, 1.75f };
		const float fp[9] = { 1.5f, -2.0f, 0.75f, 2.0f, -0.5f, 1.25f,
			3.0f, -1.5f, 0.5f };
		memcpy(frO2, fi, sizeof(fi));
		const float m7 = 7.0f;
		memcpy(frO2 + 0x30, &m7, 4);
		memcpy(payQ, fp, sizeof(fp));
		}
		MassFrame& fo = *reinterpret_cast<MassFrame*>(frO2);
		fo.nxMassFrameFoldPayload(payQ);

		unsigned char frP2[0x34];
		memset(frP2, 0, sizeof(frP2));
		unsigned char payR[0x24];
		memset(payR, 0, sizeof(payR));
		{
		const float fi[12] = { -1.25f, 2.5f, 0.0f, 4.0f, 0.125f, -2.0f,
			0.5f, -0.75f, 1.0f, -3.0f, 0.5f, 2.25f };
		const float fp[9] = { -0.5f, 1.0f, 2.0f, 0.25f, 1.5f, -2.5f,
			-1.0f, 0.5f, 4.0f };
		memcpy(frP2, fi, sizeof(fi));
		memcpy(frP2 + 0x30, fi + 9, 4);
		memcpy(payR, fp, sizeof(fp));
		}
		MassFrame& fp2 = *reinterpret_cast<MassFrame*>(frP2);
		fp2.nxMassFrameFoldPayload(payR);

		unsigned pd2 = 2166136261u;
		for(int v = 0; v < 2; ++v)
			{
			const unsigned char* src = (v == 0) ? frO2 : frP2;
			for(int i = 0; i < 0x34; i += 4)
				{
				unsigned w;
				memcpy(&w, src + i, 4);
				pd2 = nxFold(pd2, w);
				}
			}
		bool okPD = pd2 == oFoldDigest;
		printf("paxis candidate ok=%u digest=%08x\n", okPD ? 1u : 0u, pd2);
		if(!okPD)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 44u);
		}
		// -- error stream: twin drive through the reconstruction. Same two
		// setRadius drives against our own report sink.
		{
		memset(&g_errCap, 0, sizeof(g_errCap));
		nxInstallReportSink(&g_errSink);

		unsigned char sphC[0xe4];
		memset(sphC, 0xcd, sizeof(sphC));
		SphereShape& sphER = *new(sphC) SphereShape(0, 0);
		sphER.nxSphereSetRadius(-1.0f);
		int firedInvalid = g_errCap.fired;
		sphER.nxSphereSetRadius(2.5f);

		nxInstallReportSink(nullptr);	// restore silence

		unsigned ed2 = nxFoldErrCap(2166136261u);
		bool okES = ed2 == oErrDigest && firedInvalid == 1;
		printf("errstream candidate ok=%u digest=%08x\n", okES ? 1u : 0u, ed2);
		if(!okES)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 45u);
		}
		// -- group validation arm: twin drive through the transcription.
		{
		memset(&g_errCap, 0, sizeof(g_errCap));
		nxInstallReportSink(&g_errSink);

		unsigned char sphGG[0xe4];
		memset(sphGG, 0xcd, sizeof(sphGG));
		SphereShape& sphGR = *new(sphGG) SphereShape(0, 0);
		sphGR.mBase.nxApplyGroup(0xFFu);
		int groupFired = g_errCap.fired;
		sphGR.mBase.nxApplyGroup(5u);
		nxInstallReportSink(nullptr);

		unsigned gd2 = nxFoldErrCap(2166136261u);
		unsigned d8w2 = 0, c8m2 = 0;
		memcpy(&d8w2, sphGG + 0xd8, 4);
		memcpy(&c8m2, sphGG + 0xc8, 4);
		gd2 = nxFold(gd2, d8w2);
		gd2 = nxFold(gd2, c8m2);

		bool okGE = gd2 == oGroupErrDigest && groupFired == 1;
		printf("grouperr candidate ok=%u digest=%08x\n", okGE ? 1u : 0u, gd2);
		if(!okGE)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 46u);
		}
		// -- loadFromDesc validation arms: twin drive through the
		// transcription. Same invalid-radius descriptors.
		{
		memset(&g_errCap, 0, sizeof(g_errCap));
		nxInstallReportSink(&g_errSink);

		unsigned char sphL2[0xe4];
		memset(sphL2, 0xcd, sizeof(sphL2));
		SphereShape& sphLR = *new(sphL2) SphereShape(0, 0);
		unsigned char recS2[0x58];
		memset(recS2, 0, sizeof(recS2));
		const float negOne2 = -1.0f;
		memcpy(recS2 + 0x4c, &negOne2, 4);
		sphLR.nxSphereLoadFromDesc(recS2);
		int sphereFired2 = g_errCap.fired;

		unsigned char capL2[0xec];
		memset(capL2, 0xcd, sizeof(capL2));
		CapsuleShape& capLR = *new(capL2) CapsuleShape(0, 0);
		unsigned char recC2[0x58];
		memset(recC2, 0, sizeof(recC2));
		memcpy(recC2 + 0x4c, &negOne2, 4);
		capLR.nxCapsuleLoadFromDesc(recC2);
		int capsFired2 = g_errCap.fired - sphereFired2;

		nxInstallReportSink(nullptr);

		unsigned ld2 = nxFoldErrCap(2166136261u);
		unsigned sphRad2, capRad2, capHH2;
		memcpy(&sphRad2, sphL2 + 0xe0, 4);
		memcpy(&capRad2, capL2 + 0xe0, 4);
		memcpy(&capHH2, capL2 + 0xe4, 4);
		ld2 = nxFold(ld2, sphRad2);
		ld2 = nxFold(ld2, capRad2);
		ld2 = nxFold(ld2, capHH2);

		bool okLE = ld2 == oLoadErrDigest && sphereFired2 == 1 &&
			capsFired2 == 1;
		printf("loaderr candidate ok=%u digest=%08x\n", okLE ? 1u : 0u, ld2);
		if(!okLE)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 47u);
		}
		// -- default material template: fresh record vs the shipped .data
		// template. The harness never creates an SDK, so the template is in
		// its pre-creation state: pure setToDefault, flags bit31 not yet set.
		{
		unsigned char matC[0x48];
		memset(matC, 0xcd, sizeof(matC));
		new (matC) NxMaterialRecord();
		unsigned ct = 2166136261u;
		for(int i = 0; i < 0x48; i += 4)
			{
			unsigned w;
			memcpy(&w, matC + i, 4);
			ct = nxFold(ct, w);
			}
		bool okMT = ct == oMaterialTemplateDigest;
		printf("material candidate ok=%u digest=%08x\n", okMT ? 1u : 0u, ct);
		if(!okMT)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 48u);
		}

		// -- owned-DTOR deregistration: twin drive through the
		// transcription. Same fake scene, register-then-delete.
		{
		static unsigned dSent2[64];
		static unsigned dCntA2[64];
		static unsigned dCntB2[64];
		static unsigned* dCntBEnd2 = dCntB2 + 8;
		static unsigned dMir2[64];
		static unsigned dFlArr2[64];
		static unsigned* dFlBegin2 = dFlArr2;
		static unsigned* dFlEndCur2 = dFlArr2;
		static unsigned dShapes2[64];
		static unsigned dHdr2[64];
		static unsigned dPairs2[16];
		static unsigned dPairHdr2[16];
		static unsigned dSlotFreeArr2[64];
		static unsigned dSlotHdr2[8];
		for(int i = 0; i < 64; ++i)
			{
			dSent2[i] = 0u;
			dCntA2[i] = 0xA0000000u + static_cast<unsigned>(i);
			dMir2[i] = static_cast<unsigned>(i);
			dShapes2[i] = 0u;
			}
		dCntB2[7] = 7u;

		unsigned fakeSceneD3[512];
		memset(fakeSceneD3, 0, sizeof(fakeSceneD3));
		unsigned fakeOwnerD3[4];
		memset(fakeOwnerD3, 0, sizeof(fakeOwnerD3));

		const unsigned SLOT_OD2 = 3;

		fakeSceneD3[0x48 / 4] = reinterpret_cast<unsigned>(dHdr2);
		fakeSceneD3[0x5d4 / 4] = reinterpret_cast<unsigned>(dPairHdr2);
		fakeSceneD3[0x6e4 / 4] = reinterpret_cast<unsigned>(dSlotHdr2);
		fakeOwnerD3[1] = reinterpret_cast<unsigned>(fakeSceneD3);

		dHdr2[0x00 / 4] = reinterpret_cast<unsigned>(dSent2);
		dHdr2[0x10 / 4] = reinterpret_cast<unsigned>(dCntA2);
		dHdr2[0x14 / 4] = reinterpret_cast<unsigned>(dCntBEnd2);
		dHdr2[0x20 / 4] = reinterpret_cast<unsigned>(dMir2);
		dHdr2[0x30 / 4] = reinterpret_cast<unsigned>(dFlArr2);
		dHdr2[0x34 / 4] = reinterpret_cast<unsigned>(dFlEndCur2);
		dHdr2[0x38 / 4] = reinterpret_cast<unsigned>(dFlArr2 + 64);
		dHdr2[0x90 / 4] = reinterpret_cast<unsigned>(dShapes2);
		dHdr2[0x94 / 4] = reinterpret_cast<unsigned>(dShapes2 + 8);
		dHdr2[0x98 / 4] = reinterpret_cast<unsigned>(dShapes2 + 8);

		unsigned char sphD3[0xe4];
		memset(sphD3, 0xcd, sizeof(sphD3));
		SphereShape& sphDR = *new(sphD3) SphereShape(fakeOwnerD3, SLOT_OD2);
		unsigned selfAddr3 = reinterpret_cast<unsigned>(sphD3);

		dPairs2[0] = selfAddr3;			dPairs2[1] = 0xDEAD0001u;
		dPairs2[2] = 0xDEAD0002u;		dPairs2[3] = 0xDEAD0003u;
		dPairs2[4] = 0xDEAD0004u;		dPairs2[5] = 0xDEAD0005u;
		dPairHdr2[0x00 / 4] = reinterpret_cast<unsigned>(dPairs2);
		dPairHdr2[0x04 / 4] = reinterpret_cast<unsigned>(dPairs2 + 12);

		dSlotHdr2[0x08 / 4] = reinterpret_cast<unsigned>(dSlotFreeArr2 + 2);
		dSlotHdr2[0x0c / 4] = reinterpret_cast<unsigned>(dSlotFreeArr2 + 40);

		sphDR.nxSphereScalarDeletingDtor(0);

		unsigned sceneFlag3 = *reinterpret_cast<unsigned*>(
			reinterpret_cast<unsigned char*>(fakeSceneD3) + 0x70c);
		int shapesCleared = (dShapes2[SLOT_OD2] == 0);
		int sentZeroed = (dSent2[SLOT_OD2] == 0);
		int poisoned = (dMir2[SLOT_OD2] == 0xD00BEED0u);
		int cntPopped = (*reinterpret_cast<unsigned*>(dHdr2[0x14 / 4]) ==
			reinterpret_cast<unsigned>(dCntBEnd2 - 4));
		int cntAMoved = (dCntA2[SLOT_OD2] == 7u && dMir2[7] == 8u);
		int flUntouched = (dFlEndCur2 == dFlArr2);
		int pairsCompacted = (dPairs2[0] == 0xDEAD0004u &&
			dPairs2[1] == 0xDEAD0005u &&
			*reinterpret_cast<unsigned*>(dPairHdr2[0x04 / 4]) ==
				reinterpret_cast<unsigned>(dPairs2 + 8));
		int slotFreed = (dSlotFreeArr2[2] == SLOT_OD2 &&
			dSlotHdr2[0x08 / 4] == reinterpret_cast<unsigned>(dSlotFreeArr2 + 12));

		unsigned dd3 = 2166136261u;
		dd3 = nxFold(dd3, sceneFlag3 == 2u ? 1u : 0u);
		const int checks3[] = { shapesCleared, sentZeroed, poisoned,
			cntPopped, cntAMoved, flUntouched, pairsCompacted, slotFreed };
		for(int k = 0; k < 8; ++k)
			dd3 = nxFold(dd3, static_cast<unsigned>(checks3[k]));

		bool okOWD = dd3 == oOwnDtorDigest;
		printf("owndtor candidate ok=%u digest=%08x\n", okOWD ? 1u : 0u, dd3);
		if(!okOWD)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 50u);
		}

		// -- vecgrow: the transcription's push_back through both reallocs,
		// then the pre-reserved no-alloc drive. Twin of the oracle family.
		{
		unsigned vecC[8];
		memset(vecC, 0xcd, sizeof(vecC));
		vecC[0] = 0xC0C0C0C0u;
		vecC[1] = 0;
		vecC[2] = 0;
		vecC[3] = 0;

		NxHeapMark mgC = nxHeapMarkNow();
		for(unsigned k = 0; k < 8; ++k)
			nxU32VectorPushBack(vecC, 0x51510000u + k);
		int proxyUntouched = (vecC[0] == 0xC0C0C0C0u);
		const unsigned* vecElemsC = reinterpret_cast<const unsigned*>(vecC[1]);
		unsigned cnt = (vecC[2] - vecC[1]) >> 2;
		unsigned cap = (vecC[3] - vecC[1]) >> 2;
		int elemsOk = (cnt == 8);
		for(unsigned k = 0; k < cnt && k < 8; ++k)
			if(vecElemsC[k] != 0x51510000u + k)
				elemsOk = 0;

		unsigned dvC = 2166136261u;
		dvC = nxFold(dvC, proxyUntouched ? 1u : 0u);
		dvC = nxFold(dvC, cnt);
		dvC = nxFold(dvC, cap);
		for(unsigned k = 0; k < cnt && k < 8; ++k)
			dvC = nxFold(dvC, vecElemsC[k]);
		dvC = nxFoldHeapDelta(dvC, mgC);

		static unsigned rsvStoreC[16];
		unsigned rsvC[8];
		memset(rsvC, 0, sizeof(rsvC));
		rsvC[1] = reinterpret_cast<unsigned>(rsvStoreC);
		rsvC[2] = reinterpret_cast<unsigned>(rsvStoreC + 1);
		rsvC[3] = reinterpret_cast<unsigned>(rsvStoreC + 16);
		NxHeapMark mrC = nxHeapMarkNow();
		nxU32VectorPushBack(rsvC, 0x5E5E0001u);
		int rsvNoAlloc = (g_heapMallocOps == mrC.mo && g_heapFreeOps == mrC.fo &&
			rsvC[2] == reinterpret_cast<unsigned>(rsvStoreC + 2));
		int rsvStored = (rsvStoreC[1] == 0x5E5E0001u);
		unsigned drC = 2166136261u;
		drC = nxFold(drC, rsvNoAlloc ? 1u : 0u);
		drC = nxFold(drC, rsvStored ? 1u : 0u);

		bool okVG = dvC == oVecGrowDigest && drC == oVecGrowReservedDigest;
		printf("vecgrow candidate ok=%u count=%u cap=%u elems=%u digest=%08x/%08x\n",
			okVG ? 1u : 0u, cnt, cap, elemsOk, dvC, drC);
		if(!okVG)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 51u);
		}

		// -- relgrow: nxSceneReleaseIndex with the free vector at capacity.
		// Twin of the oracle family: grow mid-release, duplicate push on a
		// released slot, virgin sentinel skips the push but still unlinks.
		// Initial block from the arena, like the oracle drive.
		{
		static unsigned rSentC[64];
		static unsigned rCntAC[64];
		static unsigned rCntBC[64];
		static unsigned* rCntBEndC = rCntBC + 8;
		static unsigned rMirC[64];
		static unsigned rHdrC[64];
		memset(rHdrC, 0, sizeof(rHdrC));
		for(int i = 0; i < 64; ++i)
			{
			rSentC[i] = 0xFFFFFFFFu;
			rCntAC[i] = 0xB0000000u + static_cast<unsigned>(i);
			rMirC[i] = static_cast<unsigned>(i);
			}
		rCntBC[7] = 7u;
		rSentC[3] = 0xA5A50003u;
		rSentC[5] = 0u;

		unsigned* rFlC = static_cast<unsigned*>(nxHeapAlloc(8 * sizeof(unsigned)));
		for(int i = 0; i < 8; ++i)
			rFlC[i] = 0x11110000u + static_cast<unsigned>(i);

		rHdrC[0x00 / 4] = reinterpret_cast<unsigned>(rSentC);
		rHdrC[0x10 / 4] = reinterpret_cast<unsigned>(rCntAC);
		rHdrC[0x14 / 4] = reinterpret_cast<unsigned>(rCntBEndC);
		rHdrC[0x20 / 4] = reinterpret_cast<unsigned>(rMirC);
		rHdrC[0x30 / 4] = reinterpret_cast<unsigned>(rFlC);
		rHdrC[0x34 / 4] = reinterpret_cast<unsigned>(rFlC + 8);
		rHdrC[0x38 / 4] = reinterpret_cast<unsigned>(rFlC + 8);

		NxHeapMark mhC = nxHeapMarkNow();
		nxSceneReleaseIndex(rHdrC, 3);
		nxSceneReleaseIndex(rHdrC, 5);
		NxHeapMark mAfterDupC = nxHeapMarkNow();
		nxSceneReleaseIndex(rHdrC, 7);

		int virginNoPush = (g_heapMallocOps == mAfterDupC.mo &&
			g_heapFreeOps == mAfterDupC.fo);
		int sent3Zeroed = (rSentC[3] == 0u && rSentC[7] == 0u);
		int mirPoisoned = (rMirC[3] == 0xD00BEED0u && rMirC[7] == 0xD00BEED0u);
		int secondUnlinkMoved = (rCntAC[3] == 0u && rMirC[0] == 3u &&
			*reinterpret_cast<unsigned**>(
				reinterpret_cast<unsigned char*>(rHdrC) + 0x14) == rCntBC + 6);
		unsigned flCount = (rHdrC[0x34 / 4] - rHdrC[0x30 / 4]) >> 2;
		unsigned flCap = (rHdrC[0x38 / 4] - rHdrC[0x30 / 4]) >> 2;
		const unsigned* flLiveC = *reinterpret_cast<unsigned* const*>(
			reinterpret_cast<unsigned char*>(rHdrC) + 0x30);
		int dupPresent = (flCount >= 10 && flLiveC[9] == 5u);

		unsigned dqC = 2166136261u;
		dqC = nxFold(dqC, virginNoPush ? 1u : 0u);
		dqC = nxFold(dqC, sent3Zeroed ? 1u : 0u);
		dqC = nxFold(dqC, mirPoisoned ? 1u : 0u);
		dqC = nxFold(dqC, secondUnlinkMoved ? 1u : 0u);
		dqC = nxFold(dqC, flCount);
		dqC = nxFold(dqC, flCap);
		for(unsigned k = 0; k < flCount && k < 18; ++k)
			dqC = nxFold(dqC, flLiveC[k]);
		dqC = nxFold(dqC, dupPresent ? 1u : 0u);
		dqC = nxFoldHeapDelta(dqC, mhC);

		bool okRG = dqC == oRelGrowDigest;
		printf("relgrow candidate ok=%u nopush=%u s37zero=%u poison=%u mv2=%u fl=%u/%u dup=%u digest=%08x\n",
			okRG ? 1u : 0u, virginNoPush, sent3Zeroed, mirPoisoned,
			secondUnlinkMoved, flCount, flCap, dupPresent, dqC);
		if(!okRG)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 52u);
		}

		// -- pairrm: nxSceneRemovePairs over the five twin sub-drives,
		// through the same field-address contract as the oracle drive.
		{
		static unsigned pwC[5][16];
		static unsigned phC[5][4];
		static unsigned pcC[5];
		memset(pwC, 0, sizeof(pwC));
		memset(phC, 0, sizeof(phC));

		for(int cs = 0; cs < 5; ++cs)
			pcC[cs] = reinterpret_cast<unsigned>(phC[cs]);

		pwC[0][0] = 0x7A010001u; pwC[0][1] = 0x7A010002u;
		pwC[0][2] = 0x7A020001u; pwC[0][3] = 0x7A020002u;
		pwC[0][4] = 0x7A030001u; pwC[0][5] = 0x7A030002u;
		phC[0][0] = reinterpret_cast<unsigned>(pwC[0]);
		phC[0][1] = reinterpret_cast<unsigned>(pwC[0] + 6);
		nxSceneRemovePairs(&pcC[0], reinterpret_cast<void*>(0x7A020001u));

		pwC[1][0] = 0x7B110001u; pwC[1][1] = 0x7B110002u;
		pwC[1][2] = 0x7B120001u; pwC[1][3] = 0x7B120002u;
		phC[1][0] = reinterpret_cast<unsigned>(pwC[1]);
		phC[1][1] = reinterpret_cast<unsigned>(pwC[1] + 4);
		nxSceneRemovePairs(&pcC[1], reinterpret_cast<void*>(0x7B120002u));

		const unsigned DUP_VC = 0x7C220000u;
		pwC[2][0] = DUP_VC;			pwC[2][1] = 0x7C200002u;
		pwC[2][2] = 0x7C210001u;	pwC[2][3] = 0x7C210002u;
		pwC[2][4] = DUP_VC;			pwC[2][5] = 0x7C200004u;
		phC[2][0] = reinterpret_cast<unsigned>(pwC[2]);
		phC[2][1] = reinterpret_cast<unsigned>(pwC[2] + 6);
		nxSceneRemovePairs(&pcC[2], reinterpret_cast<void*>(DUP_VC));

		pwC[3][0] = 0x7D310001u; pwC[3][1] = 0x7D310002u;
		phC[3][0] = reinterpret_cast<unsigned>(pwC[3]);
		phC[3][1] = reinterpret_cast<unsigned>(pwC[3] + 2);
		nxSceneRemovePairs(&pcC[3], reinterpret_cast<void*>(0xDEADBEEFu));

		phC[4][0] = reinterpret_cast<unsigned>(pwC[4]);
		phC[4][1] = reinterpret_cast<unsigned>(pwC[4]);
		nxSceneRemovePairs(&pcC[4], reinterpret_cast<void*>(0x7A010001u));

		unsigned dpC = 2166136261u;
		for(int cs = 0; cs < 5; ++cs)
			{
			dpC = nxFold(dpC, phC[cs][1] - phC[cs][0]);
			for(int w = 0; w < 16; ++w)
				dpC = nxFold(dpC, pwC[cs][w]);
			}

		bool okPR = dpC == oPairRmDigest;
		printf("pairrm candidate ok=%u digest=%08x\n", okPR ? 1u : 0u, dpC);
		if(!okPR)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 53u);
		}

		// -- actorsm: the eight guarded actor accessors through the
		// transcriptions. Twin of the oracle family, same marks.
		{
		static unsigned sCsC[16];
		static unsigned sSceneC[4];
		static unsigned sBodyC[64];
		static unsigned sRecC[0x40];
		static unsigned sShapeMeshC[0x40];
		static unsigned sShapeBoxC[0x40];
		static unsigned sMeshArrC[8];
		InitializeCriticalSection((LPCRITICAL_SECTION)sCsC);
		sSceneC[0] = reinterpret_cast<unsigned>(sCsC);
		memset(sBodyC, 0, sizeof(sBodyC));
		memset(sRecC, 0, sizeof(sRecC));
		memset(sShapeMeshC, 0, sizeof(sShapeMeshC));
		memset(sShapeBoxC, 0, sizeof(sShapeBoxC));
		sBodyC[2] = reinterpret_cast<unsigned>(sRecC);
		sBodyC[5] = 0x00000030u;
		sBodyC[7] = 0x0000BEEFu;
		sRecC[0x34] = 0x41E8909Bu;
		sRecC[0x35] = 0x42F00000u;
		sBodyC[4] = reinterpret_cast<unsigned>(sShapeMeshC);
		sShapeMeshC[0x34] = 5;
		sShapeMeshC[0x38] = reinterpret_cast<unsigned>(sMeshArrC);
		sShapeMeshC[0x39] = reinterpret_cast<unsigned>(sMeshArrC + 12);
		sShapeMeshC[0x3c] = 0x0BADF00Du;
		sShapeBoxC[0x34] = 0;

		unsigned char actC[0x20];
		memset(actC, 0xcd, sizeof(actC));
		unsigned* actFieldsC = reinterpret_cast<unsigned*>(actC);
		actFieldsC[4] = reinterpret_cast<unsigned>(sSceneC);
		actFieldsC[5] = reinterpret_cast<unsigned>(sBodyC);

		unsigned daC = 2166136261u;
		daC = nxFold(daC, nxActorBodyPresent(actC) ? 1u : 0u);
		daC = nxFold(daC, nxActorGetGroupWord(actC));
		daC = nxFold(daC, nxActorFlagsMasked(actC, 0x10u) ? 1u : 0u);
		daC = nxFold(daC, nxActorFlagsMasked(actC, 0xC0u) ? 1u : 0u);
		float s0c = nxActorSqrtFieldD0(actC);
		float s4c = nxActorSqrtFieldD4(actC);
		unsigned c0, c4;
		memcpy(&c0, &s0c, 4);
		memcpy(&c4, &s4c, 4);
		daC = nxFold(daC, c0);
		daC = nxFold(daC, c4);
		daC = nxFold(daC, nxActorShapeRecordCount(actC));
		unsigned coMeshC = reinterpret_cast<unsigned>(nxActorCollisionObject(actC));
		daC = nxFold(daC, coMeshC == 0x0BADF00Du ? 1u : 0u);

		unsigned char actNegC[0x20];
		memset(actNegC, 0xcd, sizeof(actNegC));
		unsigned* negFieldsC = reinterpret_cast<unsigned*>(actNegC);
		negFieldsC[4] = reinterpret_cast<unsigned>(sSceneC);
		negFieldsC[5] = reinterpret_cast<unsigned>(sBodyC);
		sBodyC[4] = reinterpret_cast<unsigned>(sShapeBoxC);
		daC = nxFold(daC, nxActorShapeRecordCount(actNegC));
		unsigned boxBaseC = reinterpret_cast<unsigned>(sShapeBoxC);
		daC = nxFold(daC,
			reinterpret_cast<unsigned>(nxActorCollisionObject(actNegC))
				== boxBaseC + 0x9c ? 1u : 0u);
		sBodyC[4] = 0;
		daC = nxFold(daC, nxActorShapeRecordCount(actNegC));
		daC = nxFold(daC, nxActorCollisionObject(actNegC) == nullptr ? 1u : 0u);

		// null-record arm: body present, [body+8] null
		unsigned char actNoBodyC[0x20];
		memset(actNoBodyC, 0xcd, sizeof(actNoBodyC));
		unsigned* nbFieldsC = reinterpret_cast<unsigned*>(actNoBodyC);
		nbFieldsC[4] = reinterpret_cast<unsigned>(sSceneC);
		nbFieldsC[5] = reinterpret_cast<unsigned>(sBodyC);
		sBodyC[2] = 0;
		float nbC = nxActorSqrtFieldD0(actNoBodyC);
		unsigned nbBitsC;
		memcpy(&nbBitsC, &nbC, 4);
		daC = nxFold(daC, nbBitsC);
		sBodyC[2] = reinterpret_cast<unsigned>(sRecC);

		daC = nxFold(daC, nxActorBoundTarget(actC) == nullptr ? 1u : 0u);
		nxSetSdkPointerBinding(reinterpret_cast<void*>(0x13570001u),
			reinterpret_cast<void*>(0x5A5A1000u));
		daC = nxFold(daC,
			reinterpret_cast<unsigned>(nxActorBoundTarget(actC)) == 0x5A5A1000u
				? 1u : 0u);
		nxSetSdkPointerBinding(reinterpret_cast<void*>(0x13570001u), nullptr);
		daC = nxFold(daC, nxActorBoundTarget(actC) == nullptr ? 1u : 0u);

		daC = nxFold(daC, sCsC[6]);
		daC = nxFold(daC, sCsC[7] != 0 ? 1u : 0u);
		bool okAS = daC == oActorsmDigest;
		printf("actorsm candidate ok=%u digest=%08x\n", okAS ? 1u : 0u, daC);
		if(!okAS)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 54u);
		}

		// -- actorctor: construction tail, deleting dtor with arena release,
		// +8 adjustor thunk, wall dtor, member init/reset.
		{
		unsigned fakeBodyMarkC = 0x0B0DF00Du;
		unsigned actCtorC[8];
		memset(actCtorC, 0xcd, sizeof(actCtorC));
		nxActorConstruct(actCtorC, reinterpret_cast<void*>(fakeBodyMarkC));
		int ctVptr = (actCtorC[0] == 0x10104530u);
		int ctOwner = (actCtorC[1] == 0);
		int ctMember = (actCtorC[2] == 0x1010468cu);
		int ctMemberZeroed = (actCtorC[3] == 0 && actCtorC[4] == 0);
		int ctBody = (actCtorC[5] == fakeBodyMarkC);

		unsigned* dynAC = static_cast<unsigned*>(
			nxHeapAlloc(sizeof(unsigned) * 8));
		for(int i = 0; i < 8; ++i)
			dynAC[i] = 0xFEEDF00Du;
		NxHeapMark mdDC = nxHeapMarkNow();
		nxActorDeletingDtor(dynAC, 1);
		int ddWall = (dynAC[0] == 0x101043d0u);
		int ddMember = (dynAC[2] == 0x101088b8u);
		int ddFreed = (g_heapFreeOps == mdDC.fo + 1 &&
			g_heapMallocOps == mdDC.mo);

		unsigned* dynBC = static_cast<unsigned*>(
			nxHeapAlloc(sizeof(unsigned) * 8));
		for(int i = 0; i < 8; ++i)
			dynBC[i] = 0xFEEDF00Du;
		// the thunk's own body: sub ecx,8 then the same deleting dtor
		nxActorDeletingDtorThunk(
			reinterpret_cast<unsigned char*>(dynBC) + 8, 1);
		int adjWall = (dynBC[0] == 0x101043d0u);
		int adjMember = (dynBC[2] == 0x101088b8u);
		int adjFreed = (g_heapFreeOps == mdDC.fo + 2);

		unsigned* dynWC = static_cast<unsigned*>(::malloc(sizeof(unsigned) * 8));
		for(int i = 0; i < 8; ++i)
			dynWC[i] = 0xFEEDF00Du;
		nxActorInterfaceDtor(dynWC, 1);
		int wFreed = 1;
		unsigned stackWC[8];
		memset(stackWC, 0xcd, sizeof(stackWC));
		nxActorInterfaceDtor(stackWC, 0);
		int wWall = (stackWC[0] == 0x101043d0u);
		int wUntouched = (stackWC[2] == 0xCDCDCDCDu);
		int wNoFree = (g_heapFreeOps == mdDC.fo + 2);

		unsigned dcC = 2166136261u;
		const int checksC[] = { ctVptr, ctOwner, ctMember, ctMemberZeroed,
			ctBody, ddWall, ddMember, ddFreed, adjWall, adjMember, adjFreed,
			wWall, wUntouched, wFreed };
		for(int k = 0; k < 14; ++k)
			dcC = nxFold(dcC, static_cast<unsigned>(checksC[k]));
		dcC = nxFoldHeapDelta(dcC, mdDC);

		bool okAC = dcC == oActorCtorDigest;
		printf("actorctor candidate ok=%u ct=%u/%u/%u/%u/%u dd=%u/%u/%u adj=%u/%u/%u w=%u/%u/%u digest=%08x\n",
			okAC ? 1u : 0u,
			ctVptr, ctOwner, ctMember, ctMemberZeroed, ctBody,
			ddWall, ddMember, ddFreed, adjWall, adjMember, adjFreed,
			wWall, wUntouched, wFreed, dcC);
		if(!okAC)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 55u);
		}

		// -- actorsm2: energy word, +0x84 test, flag writers with their
		// deadlock-report arms. Twin of the oracle family.
		{
		static unsigned sR2CsC[16];
		static unsigned sR2SceneC[4];
		static unsigned sR2BodyC[64];
		static unsigned sR2RecC[0x70];
		InitializeCriticalSection((LPCRITICAL_SECTION)sR2CsC);
		sR2SceneC[0] = reinterpret_cast<unsigned>(sR2CsC);
		memset(sR2BodyC, 0, sizeof(sR2BodyC));
		memset(sR2RecC, 0, sizeof(sR2RecC));
		sR2BodyC[2] = reinterpret_cast<unsigned>(sR2RecC);
		sR2BodyC[5] = 0x00000030u;
		float v6c = 1.5f, v70 = 2.0f, v74 = 2.5f;
		float m78 = 3.0f, m7c = 4.0f, m80 = 5.0f;
		float v18c = 0.25f, v190 = 0.5f, v194 = 0.75f, m188 = 6.0f;
		memcpy(sR2RecC + 0x1b, &v6c, 4);
		memcpy(sR2RecC + 0x1c, &v70, 4);
		memcpy(sR2RecC + 0x1d, &v74, 4);
		memcpy(sR2RecC + 0x1e, &m78, 4);
		memcpy(sR2RecC + 0x1f, &m7c, 4);
		memcpy(sR2RecC + 0x20, &m80, 4);
		memcpy(sR2RecC + 0x62, &m188, 4);
		memcpy(sR2RecC + 0x63, &v18c, 4);
		memcpy(sR2RecC + 0x64, &v190, 4);
		memcpy(sR2RecC + 0x65, &v194, 4);

		unsigned char actE2[0x20];
		memset(actE2, 0xcd, sizeof(actE2));
		unsigned* ef2 = reinterpret_cast<unsigned*>(actE2);
		ef2[3] = reinterpret_cast<unsigned>(sR2SceneC);
		ef2[4] = reinterpret_cast<unsigned>(sR2SceneC);
		ef2[5] = reinterpret_cast<unsigned>(sR2BodyC);

		unsigned d2C = 2166136261u;
		float eDirectC = nxBodyRecordEnergyWord(sR2RecC);
		float eViaSlotC = nxActorRecordEnergyWord(actE2);
		unsigned ec0, ec1;
		memcpy(&ec0, &eDirectC, 4);
		memcpy(&ec1, &eViaSlotC, 4);
		d2C = nxFold(d2C, ec0);
		d2C = nxFold(d2C, ec1);
		d2C = nxFold(d2C, nxActorRecordWord84Zero(actE2) ? 1u : 0u);
		sR2RecC[0x21] = 0x00000077u;
		d2C = nxFold(d2C, nxActorRecordWord84Zero(actE2) ? 1u : 0u);
		sR2RecC[0x21] = 0;

		nxActorRaiseFlags(actE2, 0x40u);
		d2C = nxFold(d2C, sR2BodyC[5]);
		nxActorClearFlags(actE2, 0x10u);
		d2C = nxFold(d2C, sR2BodyC[5]);

		memset(&g_errCap, 0, sizeof(g_errCap));
		nxInstallReportSink(&g_errSink);
		sR2CsC[6] = 1;
		sR2CsC[7] = ::GetCurrentThreadId() + 1u;
		nxActorRaiseFlags(actE2, 0x40u);
		nxActorClearFlags(actE2, 0x10u);
		nxInstallReportSink(nullptr);
		d2C = nxFoldErrCap(d2C);
		d2C = nxFold(d2C, sR2BodyC[5]);
		d2C = nxFold(d2C, sR2CsC[6]);

		bool okA2 = d2C == oActorsm2Digest;
		printf("actorsm2 candidate ok=%u energy=%08x/%08x digest=%08x\n",
			okA2 ? 1u : 0u, ec0, ec1, d2C);
		if(!okA2)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 56u);
		}

		// -- actorsm3: damping getters with their null-record warnings.
		// Twin of the oracle family.
		{
		static unsigned sR3CsC[16];
		static unsigned sR3SceneC[4];
		static unsigned sR3BodyC[64];
		static unsigned sR3RecC[0x80];
		InitializeCriticalSection((LPCRITICAL_SECTION)sR3CsC);
		sR3SceneC[0] = reinterpret_cast<unsigned>(sR3CsC);
		memset(sR3BodyC, 0, sizeof(sR3BodyC));
		memset(sR3RecC, 0, sizeof(sR3RecC));
		sR3BodyC[2] = reinterpret_cast<unsigned>(sR3RecC);
		float linMark = 0.35f, angMark = 0.125f, f188Mark = 7.5f;
		memcpy(sR3RecC + 0x2e, &linMark, 4);
		memcpy(sR3RecC + 0x2f, &angMark, 4);
		memcpy(sR3RecC + 0x62, &f188Mark, 4);

		unsigned char actG2[0x20];
		memset(actG2, 0xcd, sizeof(actG2));
		unsigned* gf2 = reinterpret_cast<unsigned*>(actG2);
		gf2[4] = reinterpret_cast<unsigned>(sR3SceneC);
		gf2[5] = reinterpret_cast<unsigned>(sR3BodyC);

		unsigned d3C = 2166136261u;
		float ldv = nxActorGetLinearDamping(actG2);
		float adv = nxActorGetAngularDamping(actG2);
		float f188v = nxActorRecordField188(actG2);
		unsigned lb, ab, fb;
		memcpy(&lb, &ldv, 4);
		memcpy(&ab, &adv, 4);
		memcpy(&fb, &f188v, 4);
		d3C = nxFold(d3C, lb);
		d3C = nxFold(d3C, ab);
		d3C = nxFold(d3C, fb);

		memset(&g_errCap, 0, sizeof(g_errCap));
		nxInstallReportSink(&g_errSink);
		sR3BodyC[2] = 0;
		float nz1 = nxActorGetLinearDamping(actG2);
		unsigned z1;
		memcpy(&z1, &nz1, 4);
		d3C = nxFold(d3C, z1);
		d3C = nxFoldErrCap(d3C);
		memset(&g_errCap, 0, sizeof(g_errCap));
		float nz2 = nxActorGetAngularDamping(actG2);
		unsigned z2;
		memcpy(&z2, &nz2, 4);
		d3C = nxFold(d3C, z2);
		d3C = nxFoldErrCap(d3C);
		nxInstallReportSink(nullptr);

		bool okA3 = d3C == oActorsm3Digest;
		printf("actorsm3 candidate ok=%u lin=%08x ang=%08x digest=%08x\n",
			okA3 ? 1u : 0u, lb, ab, d3C);
		if(!okA3)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 57u);
		}

		// -- actorsm4: binding write + pose read with fallback. Twin of the
		// oracle family.
		{
		static unsigned sR4CsC[16];
		static unsigned sR4SceneC[4];
		static unsigned sR4BodyC[64];
		static unsigned sR4RecC[0x40];
		InitializeCriticalSection((LPCRITICAL_SECTION)sR4CsC);
		sR4SceneC[0] = reinterpret_cast<unsigned>(sR4CsC);
		memset(sR4BodyC, 0, sizeof(sR4BodyC));
		memset(sR4RecC, 0, sizeof(sR4RecC));
		sR4BodyC[2] = reinterpret_cast<unsigned>(sR4RecC);
		sR4RecC[0x14] = 0x00500001u;
		sR4RecC[0x15] = 0x00540002u;
		sR4RecC[0x16] = 0x00580003u;
		sR4BodyC[0x11] = 0x00440004u;
		sR4BodyC[0x12] = 0x00480005u;
		sR4BodyC[0x13] = 0x004c0006u;

		unsigned char actH2[0x20];
		memset(actH2, 0xcd, sizeof(actH2));
		unsigned* hf2 = reinterpret_cast<unsigned*>(actH2);
		hf2[3] = reinterpret_cast<unsigned>(sR4SceneC);
		hf2[4] = reinterpret_cast<unsigned>(sR4SceneC);
		hf2[5] = reinterpret_cast<unsigned>(sR4BodyC);

		unsigned d4C = 2166136261u;
		unsigned poseO2[4];
		nxActorGetPoseWords(actH2, poseO2);
		d4C = nxFold(d4C, poseO2[0]);
		d4C = nxFold(d4C, poseO2[1]);
		d4C = nxFold(d4C, poseO2[2]);

		sR4BodyC[2] = 0;
		nxActorGetPoseWords(actH2, poseO2);
		d4C = nxFold(d4C, poseO2[0]);
		d4C = nxFold(d4C, poseO2[1]);
		d4C = nxFold(d4C, poseO2[2]);
		sR4BodyC[2] = reinterpret_cast<unsigned>(sR4RecC);

		nxActorSetBoundTarget(actH2, reinterpret_cast<void*>(0x5A5A2000u));
		d4C = nxFold(d4C,
			reinterpret_cast<unsigned>(nxActorBoundTarget(actH2))
				== 0x5A5A2000u ? 1u : 0u);

		memset(&g_errCap, 0, sizeof(g_errCap));
		nxInstallReportSink(&g_errSink);
		sR4CsC[6] = 1;
		sR4CsC[7] = ::GetCurrentThreadId() + 1u;
		nxActorSetBoundTarget(actH2, reinterpret_cast<void*>(0xDEAD2000u));
		nxInstallReportSink(nullptr);
		d4C = nxFoldErrCap(d4C);
		d4C = nxFold(d4C,
			reinterpret_cast<unsigned>(nxActorBoundTarget(actH2))
				== 0x5A5A2000u ? 1u : 0u);

		bool okA4 = d4C == oActorsm4Digest;
		printf("actorsm4 candidate ok=%u digest=%08x\n", okA4 ? 1u : 0u, d4C);
		if(!okA4)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 58u);
		}

		// -- actorsm5: the sleep chain through the transcriptions. Twin of
		// the oracle family.
		{
		static unsigned sR5CsC[16];
		static unsigned sR5SceneC[4];
		static unsigned sR5BodyC[64];
		static unsigned sR5RecAC[0x90];
		static unsigned sR5RecMC[0x90];
		static unsigned sR5RecRC[0x90];
		InitializeCriticalSection((LPCRITICAL_SECTION)sR5CsC);
		sR5SceneC[0] = reinterpret_cast<unsigned>(sR5CsC);
		memset(sR5BodyC, 0, sizeof(sR5BodyC));
		memset(sR5RecAC, 0, sizeof(sR5RecAC));
		memset(sR5RecMC, 0, sizeof(sR5RecMC));
		memset(sR5RecRC, 0, sizeof(sR5RecRC));
		unsigned rAC = reinterpret_cast<unsigned>(sR5RecAC);
		unsigned rMC = reinterpret_cast<unsigned>(sR5RecMC);
		unsigned rRC = reinterpret_cast<unsigned>(sR5RecRC);
		sR5RecAC[0x7a] = rMC;
		sR5RecMC[0x7a] = rRC;
		sR5RecRC[0x7a] = rRC;
		float awake = 4.0f;

		unsigned char actI2[0x20];
		memset(actI2, 0xcd, sizeof(actI2));
		unsigned* i6 = reinterpret_cast<unsigned*>(actI2);
		i6[4] = reinterpret_cast<unsigned>(sR5SceneC);
		i6[5] = reinterpret_cast<unsigned>(sR5BodyC);

		unsigned d5C = 2166136261u;
		unsigned fixed = nxBodyRecordFixRoot(sR5RecAC);
		d5C = nxFold(d5C, fixed == rRC ? 1u : 0u);
		d5C = nxFold(d5C, sR5RecAC[0x7a] == rRC ? 1u : 0u);

		sR5BodyC[2] = rAC;
		d5C = nxFold(d5C, nxActorChainSettled(actI2) ? 1u : 0u);

		sR5RecMC[0x21] = *reinterpret_cast<unsigned*>(&awake);
		d5C = nxFold(d5C, nxActorChainSettled(actI2) ? 1u : 0u);
		sR5RecMC[0x21] = 0;

		sR5BodyC[2] = 0;
		d5C = nxFold(d5C, nxActorChainSettled(actI2) ? 1u : 0u);

		bool okA5 = d5C == oActorsm5Digest;
		printf("actorsm5 candidate ok=%u digest=%08x\n", okA5 ? 1u : 0u, d5C);
		if(!okA5)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 59u);
		}

		// -- actorsm6: CMass local frame getters. Twin of the oracle
		// family.
		{
		static unsigned sR6CsC[16];
		static unsigned sR6SceneC[4];
		static unsigned sR6BodyC[64];
		static unsigned sR6RecC[0x50];
		InitializeCriticalSection((LPCRITICAL_SECTION)sR6CsC);
		sR6SceneC[0] = reinterpret_cast<unsigned>(sR6CsC);
		memset(sR6BodyC, 0, sizeof(sR6BodyC));
		memset(sR6RecC, 0, sizeof(sR6RecC));
		sR6BodyC[2] = reinterpret_cast<unsigned>(sR6RecC);
		for(int i = 0; i < 12; ++i)
			sR6RecC[0x37 + i] = 0x00C0FFEEu + static_cast<unsigned>(i);

		unsigned char actJ2[0x20];
		memset(actJ2, 0xcd, sizeof(actJ2));
		unsigned* jf2 = reinterpret_cast<unsigned*>(actJ2);
		jf2[4] = reinterpret_cast<unsigned>(sR6SceneC);
		jf2[5] = reinterpret_cast<unsigned>(sR6BodyC);

		unsigned poseJ2[12];

		unsigned d6C = 2166136261u;
		nxActorGetCMassLocalPose(actJ2, poseJ2);
		for(int i = 0; i < 12; ++i)
			d6C = nxFold(d6C, poseJ2[i]);
		memset(poseJ2, 0xcd, sizeof(poseJ2));
		nxActorGetCMassLocalOrientation(actJ2, poseJ2);
		for(int i = 0; i < 9; ++i)
			d6C = nxFold(d6C, poseJ2[i]);

		memset(&g_errCap, 0, sizeof(g_errCap));
		nxInstallReportSink(&g_errSink);
		sR6BodyC[2] = 0;
		nxActorGetCMassLocalPose(actJ2, poseJ2);
		d6C = nxFoldErrCap(d6C);
		const unsigned kIdent[9] =
			{ 0x3f800000u, 0, 0, 0, 0x3f800000u, 0, 0, 0, 0x3f800000u };
		int poseOk = memcmp(poseJ2, kIdent, 36) == 0
			&& poseJ2[9] == 0 && poseJ2[10] == 0 && poseJ2[11] == 0;
		d6C = nxFold(d6C, poseOk ? 1u : 0u);

		memset(&g_errCap, 0, sizeof(g_errCap));
		nxActorGetCMassLocalOrientation(actJ2, poseJ2);
		d6C = nxFoldErrCap(d6C);
		int oriOk = memcmp(poseJ2, kIdent, 36) == 0;
		d6C = nxFold(d6C, oriOk ? 1u : 0u);
		nxInstallReportSink(nullptr);

		bool okA6 = d6C == oActorsm6Digest;
		printf("actorsm6 candidate ok=%u digest=%08x\n", okA6 ? 1u : 0u, d6C);
		if(!okA6)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 60u);
		}

		// -- actorsm7: the five three-word readers through the
		// transcription helper. Twin of the oracle family.
		{
		static unsigned sR7CsC[16];
		static unsigned sR7SceneC[4];
		static unsigned sR7BodyC[64];
		static unsigned sR7RecC[0x70];
		InitializeCriticalSection((LPCRITICAL_SECTION)sR7CsC);
		sR7SceneC[0] = reinterpret_cast<unsigned>(sR7CsC);
		memset(sR7BodyC, 0, sizeof(sR7BodyC));
		memset(sR7RecC, 0, sizeof(sR7RecC));
		sR7BodyC[2] = reinterpret_cast<unsigned>(sR7RecC);
		for(int i = 0; i < 3; ++i)
			{
			unsigned w = 0x43B00000u + static_cast<unsigned>(i) * 0x01000000u;
			memcpy(sR7RecC + 0x40 + i, &w, 4);
			memcpy(sR7RecC + 0x63 + i, &w, 4);
			}
		float v6c = 1.25f, v70 = 2.5f, v74 = 3.75f;
		float a78 = 0.5f, a7c = 1.0f, a80 = 1.5f;
		float mass = 8.0f;
		memcpy(sR7RecC + 0x1b, &v6c, 4);
		memcpy(sR7RecC + 0x1c, &v70, 4);
		memcpy(sR7RecC + 0x1d, &v74, 4);
		memcpy(sR7RecC + 0x1e, &a78, 4);
		memcpy(sR7RecC + 0x1f, &a7c, 4);
		memcpy(sR7RecC + 0x20, &a80, 4);
		memcpy(sR7RecC + 0x62, &mass, 4);

		unsigned char actK2[0x20];
		memset(actK2, 0xcd, sizeof(actK2));
		unsigned* kf2 = reinterpret_cast<unsigned*>(actK2);
		kf2[4] = reinterpret_cast<unsigned>(sR7SceneC);
		kf2[5] = reinterpret_cast<unsigned>(sR7BodyC);

		unsigned d7C = 2166136261u;
		unsigned out7C[4];

		struct NxArmC { void (*fn)(void*, void*); };
		NxArmC armsC[5] =
			{
				{ nxActorGetCMassLocalPosition },
				{ nxActorGetMassSpaceInertia },
				{ nxActorGetLinearVelocity },
				{ nxActorGetAngularVelocity },
				{ nxActorGetLinearMomentum },
			};
		for(int k = 0; k < 5; ++k)
			{
			armsC[k].fn(actK2, out7C);
			d7C = nxFold(d7C, out7C[0]);
			d7C = nxFold(d7C, out7C[1]);
			d7C = nxFold(d7C, out7C[2]);
			}

		memset(&g_errCap, 0, sizeof(g_errCap));
		nxInstallReportSink(&g_errSink);
		sR7BodyC[2] = 0;
		for(int k = 0; k < 5; ++k)
			{
			memset(&g_errCap, 0, sizeof(g_errCap));
			armsC[k].fn(actK2, out7C);
			d7C = nxFold(d7C, out7C[0]);
			d7C = nxFold(d7C, out7C[1]);
			d7C = nxFold(d7C, out7C[2]);
			d7C = nxFoldErrCap(d7C);
			}
		nxInstallReportSink(nullptr);

		bool okA7 = d7C == oActorsm7Digest;
		printf("actorsm7 candidate ok=%u digest=%08x\n", okA7 ? 1u : 0u, d7C);
		if(!okA7)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 61u);
		}

		// -- miscsm: setGroup with contended report, sub-object forwarder
		// through the planted __thiscall sentinel, id allocator both arms.
		// Twin of the oracle family.
		{
		static unsigned sR8CsC[16];
		static unsigned sR8SceneC[4];
		static unsigned sR8BodyC[64];
		InitializeCriticalSection((LPCRITICAL_SECTION)sR8CsC);
		sR8SceneC[0] = reinterpret_cast<unsigned>(sR8CsC);
		memset(sR8BodyC, 0, sizeof(sR8BodyC));

		unsigned char actL2[0x20];
		memset(actL2, 0xcd, sizeof(actL2));
		unsigned* lf2 = reinterpret_cast<unsigned*>(actL2);
		lf2[3] = reinterpret_cast<unsigned>(sR8SceneC);
		lf2[4] = reinterpret_cast<unsigned>(sR8SceneC);
		lf2[5] = reinterpret_cast<unsigned>(sR8BodyC);

		unsigned d8C = 2166136261u;

		nxActorSetGroupWord(actL2, 0x0007u);
		static unsigned sR8ReadCsC[16];
		static unsigned sR8ReadSceneC[4];
		InitializeCriticalSection((LPCRITICAL_SECTION)sR8ReadCsC);
		sR8ReadSceneC[0] = reinterpret_cast<unsigned>(sR8ReadCsC);
		unsigned char actR2[0x20];
		memset(actR2, 0xcd, sizeof(actR2));
		unsigned* rf2 = reinterpret_cast<unsigned*>(actR2);
		rf2[4] = reinterpret_cast<unsigned>(sR8ReadSceneC);
		rf2[5] = reinterpret_cast<unsigned>(sR8BodyC);
		d8C = nxFold(d8C, nxActorGetGroupWord(actR2));
		d8C = nxFold(d8C, sR8CsC[6]);

		nxActorSetGroupWord(actL2, 0x0009u);
		memset(&g_errCap, 0, sizeof(g_errCap));
		nxInstallReportSink(&g_errSink);
		sR8CsC[6] = 1;
		sR8CsC[7] = ::GetCurrentThreadId() + 1u;
		nxActorSetGroupWord(actL2, 0x000Bu);
		nxInstallReportSink(nullptr);
		d8C = nxFoldErrCap(d8C);
		d8C = nxFold(d8C, nxActorGetGroupWord(actR2));

		static unsigned g_fwdGotSubC;
		static unsigned g_fwdGotArgC;
		static unsigned g_fwdRetMarkC;
		struct NxFwdTargetC
			{
			unsigned __thiscall slot18(void* arg)
				{
				g_fwdGotSubC = reinterpret_cast<unsigned>(this);
				g_fwdGotArgC = reinterpret_cast<unsigned>(arg);
				return g_fwdRetMarkC;
				}
			};
		g_fwdGotSubC = 0;
		g_fwdGotArgC = 0;
		g_fwdRetMarkC = 0x12340005u;
		static unsigned sFwdSubVtC[8];
		static unsigned sFwdSubC[4];
		union NxFwdAddrC
			{
			unsigned (NxFwdTargetC::* pmf)(void*);
			unsigned addr;
			};
		NxFwdAddrC faC;
		faC.pmf = &NxFwdTargetC::slot18;
		sFwdSubVtC[0x18 / 4] = faC.addr;
		sFwdSubC[0] = reinterpret_cast<unsigned>(sFwdSubVtC);
		unsigned fwdSelfC[8];
		memset(fwdSelfC, 0xcd, sizeof(fwdSelfC));
		fwdSelfC[4] = reinterpret_cast<unsigned>(sFwdSubC);
		unsigned retFwdC = nxForwardSubobjectCall(fwdSelfC,
			reinterpret_cast<void*>(0x00BEEF00u));
		int fwdOk = (g_fwdGotSubC == reinterpret_cast<unsigned>(sFwdSubC)
			&& g_fwdGotArgC == 0x00BEEF00u
			&& retFwdC == g_fwdRetMarkC);
		d8C = nxFold(d8C, fwdOk ? 1u : 0u);

		static unsigned sIdCC[8];
		static unsigned sIdFreeC[4];
		memset(sIdCC, 0, sizeof(sIdCC));
		memset(sIdFreeC, 0, sizeof(sIdFreeC));
		d8C = nxFold(d8C, nxIdAllocNext(sIdCC));
		d8C = nxFold(d8C, nxIdAllocNext(sIdCC));
		sIdCC[1] = reinterpret_cast<unsigned>(sIdFreeC);
		sIdCC[2] = reinterpret_cast<unsigned>(sIdFreeC + 2);
		sIdFreeC[0] = 0x000000AAu;
		sIdFreeC[1] = 0x000000BBu;
		d8C = nxFold(d8C, nxIdAllocNext(sIdCC));
		d8C = nxFold(d8C, nxIdAllocNext(sIdCC));
		d8C = nxFold(d8C, sIdCC[2] == reinterpret_cast<unsigned>(sIdFreeC)
			? 1u : 0u);

		bool okM8 = d8C == oMiscsmDigest;
		printf("miscsm candidate ok=%u digest=%08x\n", okM8 ? 1u : 0u, d8C);
		if(!okM8)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 62u);
		}

		// -- miscsm2: readBodyFlag both arms, member deleting dtor,
		// shapes-clear entry through the release arm, two bound-pool
		// deleting dtors. Twin of the oracle family.
		{
		static unsigned sR9CsC[16];
		static unsigned sR9SceneC[4];
		static unsigned sR9BodyC[64];
		static unsigned sR9RecC[0x40];
		InitializeCriticalSection((LPCRITICAL_SECTION)sR9CsC);
		sR9SceneC[0] = reinterpret_cast<unsigned>(sR9CsC);
		memset(sR9BodyC, 0, sizeof(sR9BodyC));
		memset(sR9RecC, 0, sizeof(sR9RecC));
		sR9BodyC[2] = reinterpret_cast<unsigned>(sR9RecC);
		sR9RecC[0x43] = 0x000000A5u;

		unsigned char actM2[0x20];
		memset(actM2, 0xcd, sizeof(actM2));
		unsigned* mf2 = reinterpret_cast<unsigned*>(actM2);
		mf2[4] = reinterpret_cast<unsigned>(sR9SceneC);
		mf2[5] = reinterpret_cast<unsigned>(sR9BodyC);

		unsigned d9C = 2166136261u;
		bool q1C = nxActorReadBodyFlag(actM2, 0x25u);
		bool q2C = nxActorReadBodyFlag(actM2, 0x08u);
		d9C = nxFold(d9C, q1C ? 1u : 0u);
		d9C = nxFold(d9C, q2C ? 1u : 0u);

		memset(&g_errCap, 0, sizeof(g_errCap));
		nxInstallReportSink(&g_errSink);
		sR9BodyC[2] = 0;
		bool stFlag = nxActorReadBodyFlag(actM2, 0x25u);
		nxInstallReportSink(nullptr);
		d9C = nxFoldErrCap(d9C);
		d9C = nxFold(d9C, stFlag ? 1u : 0u);

		unsigned stackMemC[4];
		memset(stackMemC, 0xcd, sizeof(stackMemC));
		nxMemberDeletingDtor(stackMemC, 0);
		int memVt = (stackMemC[0] == 0x101088b8u);

		static unsigned sR9SentC[64];
		static unsigned sR9CntAC[64];
		static unsigned sR9CntBC[64];
		static unsigned* sR9CntBEndC = sR9CntBC + 8;
		static unsigned sR9MirC[64];
		static unsigned sR9FlC[8];
		static unsigned sR9HdrC[128];		// pool base: hdr at +0x40 bytes
		static unsigned sR9ShapesC[64];
		memset(sR9HdrC, 0, sizeof(sR9HdrC));
		for(int i = 0; i < 64; ++i)
			{
			sR9SentC[i] = 0xFFFFFFFFu;
			sR9CntAC[i] = 0xC0000000u + static_cast<unsigned>(i);
			sR9CntBC[i] = 0;
			sR9MirC[i] = static_cast<unsigned>(i);
			sR9ShapesC[i] = 0x13572468u;
			}
		sR9CntBC[7] = 7u;
		unsigned* r9FlC = static_cast<unsigned*>(
			nxHeapAlloc(8 * sizeof(unsigned)));
		for(int i = 0; i < 8; ++i)
			r9FlC[i] = 0x22220000u + static_cast<unsigned>(i);
		sR9HdrC[0x10] = reinterpret_cast<unsigned>(sR9SentC);
		sR9HdrC[0x14] = reinterpret_cast<unsigned>(sR9CntAC);
		sR9HdrC[0x15] = reinterpret_cast<unsigned>(sR9CntBEndC);
		sR9HdrC[0x18] = reinterpret_cast<unsigned>(sR9MirC);
		sR9HdrC[0x1c] = reinterpret_cast<unsigned>(r9FlC);
		sR9HdrC[0x1d] = reinterpret_cast<unsigned>(r9FlC + 8);
		sR9HdrC[0x1e] = reinterpret_cast<unsigned>(r9FlC + 8);
		sR9HdrC[0x20] = reinterpret_cast<unsigned>(sR9ShapesC);
		sR9SentC[3] = 0x00C0FFEEu;

		static unsigned sR9ShapeArgC[0x42];
		memset(sR9ShapeArgC, 0, sizeof(sR9ShapeArgC));
		sR9ShapeArgC[0x41] = 3u;

		NxHeapMark m9C = nxHeapMarkNow();
		nxSceneClearShapeSlot(sR9HdrC, sR9ShapeArgC);
		int slotFreed = (sR9SentC[3] == 0u && sR9MirC[3] == 0xD00BEED0u);
		int shapeCleared = (sR9ShapesC[3] == 0u);
		int flGrew = ((sR9HdrC[0x1d] - sR9HdrC[0x1c]) >> 2) == 9;
		int heapMoved = (g_heapMallocOps == m9C.mo + 1);

		d9C = nxFold(d9C, memVt ? 1u : 0u);
		d9C = nxFold(d9C, slotFreed ? 1u : 0u);
		d9C = nxFold(d9C, shapeCleared ? 1u : 0u);
		d9C = nxFold(d9C, flGrew ? 1u : 0u);
		d9C = nxFold(d9C, heapMoved ? 1u : 0u);

		// bound-pool dtors: arena blocks, adapter release, post-free vptr
		unsigned* bA = static_cast<unsigned*>(nxHeapAlloc(sizeof(unsigned) * 4));
		for(int i = 0; i < 4; ++i) bA[i] = 0xFEEDF00Du;
		nxBoundDeletingDtor798(bA, 1);
		d9C = nxFold(d9C, bA[0] == 0x10108798u ? 1u : 0u);
		unsigned* bB = static_cast<unsigned*>(nxHeapAlloc(sizeof(unsigned) * 4));
		for(int i = 0; i < 4; ++i) bB[i] = 0xFEEDF00Du;
		nxBoundDeletingDtor84c(bB, 1);
		d9C = nxFold(d9C, bB[0] == 0x1010884cu ? 1u : 0u);

		bool okM9 = d9C == oMiscsm2Digest;
		printf("miscsm2 candidate ok=%u digest=%08x\n", okM9 ? 1u : 0u, d9C);
		if(!okM9)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 63u);
		}

		// -- slate11: pool-class lifecycle rows through the transcriptions.
		{
		unsigned dSC = 2166136261u;

		unsigned chXC[4];
		memset(chXC, 0xcd, sizeof(chXC));
		nxChainedDeletingDtor(chXC, 0);
		dSC = nxFold(dSC, chXC[0] == 0x1010878cu ? 1u : 0u);
		dSC = nxFold(dSC, chXC[2] == 0x10108798u ? 1u : 0u);

		unsigned chYC[4];
		memset(chYC, 0xcd, sizeof(chYC));
		nxChainedDeletingDtorThunk(chYC + 2, 0);
		dSC = nxFold(dSC, chYC[0] == 0x1010878cu ? 1u : 0u);
		dSC = nxFold(dSC, chYC[2] == 0x10108798u ? 1u : 0u);

		unsigned stk78C[4];
		memset(stk78C, 0xcd, sizeof(stk78C));
		nxPoolDeletingDtor78c(stk78C, 0);
		dSC = nxFold(dSC, stk78C[0] == 0x1010878cu ? 1u : 0u);

		// 002320: cached-list destroyer over two planted nodes
		static unsigned sLNVT[4];
		sLNVT[0] = reinterpret_cast<unsigned>(&NxS11KillThunk);
		g_sl11KillCount = 0;
		unsigned nA[16], nM[16];
		memset(nA, 0, sizeof(nA));
		memset(nM, 0, sizeof(nM));
		nA[0] = reinterpret_cast<unsigned>(sLNVT);
		nA[3] = reinterpret_cast<unsigned>(nM);		// +0x30 link (word 3)
		nM[0] = reinterpret_cast<unsigned>(sLNVT);
		nM[3] = 0;
		static unsigned sLH[0x170];
		memset(sLH, 0, sizeof(sLH));
		sLH[0x16a] = reinterpret_cast<unsigned>(nA);	// byte +0x5a8
		nxDestroyCachedList(sLH);
		dSC = nxFold(dSC, g_sl11KillCount == 2 ? 1u : 0u); // two nodes deleted

		bool okS11 = dSC == oSlate11Digest;
		printf("slate11 candidate ok=%u digest=%08x\n", okS11 ? 1u : 0u, dSC);
		if(!okS11)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 64u);
		}
		// -- post-creation template state: fresh record + internal bit31.
		{
		unsigned char matB[0x48];
		memset(matB, 0xcd, sizeof(matB));
		new (matB) NxMaterialRecord();
		reinterpret_cast<NxMaterialRecord*>(matB)->setInternalFlagBit31();
		unsigned cb = 2166136261u;
		for(int i = 0; i < 0x48; i += 4)
			{
			unsigned w;
			memcpy(&w, matB + i, 4);
			cb = nxFold(cb, w);
			}
		bool okMB = cb == oMaterialBootedDigest;
		printf("materialboot candidate ok=%u digest=%08x\n", okMB ? 1u : 0u, cb);
		if(!okMB)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 49u);
		}
		// -- conditional mass-frame zeroizer: twin drive.
		{
		unsigned char zA2[0x34];
		memset(zA2, 0xcd, sizeof(zA2));
		unsigned char zB2[0x34];
		memset(zB2, 0xcd, sizeof(zB2));
		for(int i = 0; i < 13; ++i)
			{
			unsigned wa = 0x41414141u + static_cast<unsigned>(i);
			unsigned wb = 0x42424242u + static_cast<unsigned>(i);
			memcpy(zA2 + i * 4, &wa, 4);
			memcpy(zB2 + i * 4, &wb, 4);
			}
		MassFrame& mfA = *reinterpret_cast<MassFrame*>(zA2);
		MassFrame& mfB = *reinterpret_cast<MassFrame*>(zB2);
		mfA.nxMassFrameConditionalZero(1);
		mfB.nxMassFrameConditionalZero(0);

		unsigned zd2 = 2166136261u;
		for(int v = 0; v < 2; ++v)
			{
			const unsigned char* src = (v == 0) ? zA2 : zB2;
			for(int i = 0; i < 0x34; i += 4)
				{
				unsigned w;
				memcpy(&w, src + i, 4);
				zd2 = nxFold(zd2, w);
				}
			}
		bool okMZ = zd2 == oZeroDigest;
		printf("mzero candidate ok=%u digest=%08x\n", okMZ ? 1u : 0u, zd2);
		if(!okMZ)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 51u);
		}
		// -- owned-arm registration: twin drive through the transcription.
		{
		static unsigned arrSent2[64];
		static unsigned arrCntB2[64];
		static unsigned arrMirror2[64];
		static unsigned arrExtra2[64];
		static unsigned arrShapes2[64];
		for(int i = 0; i < 8; ++i)
			{
			arrSent2[i] = 0x11111111u;
			arrCntB2[i] = 0x22222222u;
			arrMirror2[i] = 0x33333333u;
			arrExtra2[i] = 0x55555555u;
			arrShapes2[i] = 0x44444444u;
			}
		unsigned hdr2[64];
		memset(hdr2, 0, sizeof(hdr2));
		hdr2[0x00 / 4] = reinterpret_cast<unsigned>(arrSent2);
		hdr2[0x04 / 4] = reinterpret_cast<unsigned>(arrSent2 + 8);
		hdr2[0x08 / 4] = reinterpret_cast<unsigned>(arrSent2 + 64);
		hdr2[0x10 / 4] = reinterpret_cast<unsigned>(arrCntB2);
		hdr2[0x14 / 4] = reinterpret_cast<unsigned>(arrCntB2 + 8);
		hdr2[0x18 / 4] = reinterpret_cast<unsigned>(arrCntB2 + 64);
		hdr2[0x20 / 4] = reinterpret_cast<unsigned>(arrMirror2);
		hdr2[0x24 / 4] = reinterpret_cast<unsigned>(arrMirror2 + 8);
		hdr2[0x28 / 4] = reinterpret_cast<unsigned>(arrMirror2 + 64);
		hdr2[0x30 / 4] = reinterpret_cast<unsigned>(arrExtra2);
		hdr2[0x34 / 4] = reinterpret_cast<unsigned>(arrExtra2 + 8);
		hdr2[0x38 / 4] = reinterpret_cast<unsigned>(arrExtra2 + 64);
		hdr2[0x90 / 4] = reinterpret_cast<unsigned>(arrShapes2);
		hdr2[0x94 / 4] = reinterpret_cast<unsigned>(arrShapes2 + 8);
		hdr2[0x98 / 4] = reinterpret_cast<unsigned>(arrShapes2 + 64);

		unsigned fakeScene2[32];
		memset(fakeScene2, 0, sizeof(fakeScene2));
		fakeScene2[0x48 / 4] = reinterpret_cast<unsigned>(hdr2);
		unsigned fakeOwner2[4];
		memset(fakeOwner2, 0, sizeof(fakeOwner2));
		fakeOwner2[1] = reinterpret_cast<unsigned>(fakeScene2);

		const unsigned SLOT2 = 3;
		unsigned char sphC3[0xe4];
		memset(sphC3, 0xcd, sizeof(sphC3));
		SphereShape& sphCR = *new(sphC3) SphereShape(fakeOwner2, SLOT2);

		unsigned d4c = 0;
		memcpy(&d4c, sphC3 + 0xd4, 4);
		unsigned cd = 2166136261u;
		cd = nxFold(cd, d4c);
		cd = nxFold(cd, arrSent2[SLOT2] == 0xFFFFFFFFu ? 1u : 0u);
		cd = nxFold(cd, arrMirror2[SLOT2] == 8u ? 1u : 0u);
		cd = nxFold(cd,
			(arrShapes2[SLOT2] != 0x44444444u && arrShapes2[SLOT2] != 0u)
				? 1u : 0u);

		bool okOW = cd == oOwnDigest;
		printf("ownctor candidate ok=%u digest=%08x\n", okOW ? 1u : 0u, cd);
		if(!okOW)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 49u);
		}






		// -- capsule dtor: twin drive.
		{
		unsigned char cbytes[0xec];
		memset(cbytes, 0xcd, sizeof(cbytes));
		CapsuleShape& cap = *new(cbytes) CapsuleShape(0, 0);
		cap.nxCapsuleScalarDeletingDtor(0);

		 unsigned dc = 2166136261u;
		for(unsigned i = 0; i < sizeof(cbytes); i += 4)
			{
			bool pointer = false;
			for(size_t p = 0; p < 6; ++p)
				if(kDtorMask[p] == i)
					pointer = true;
			if(pointer)
				continue;
			unsigned w;
			memcpy(&w, cbytes + i, 4);
			dc = nxFold(dc, w);
			}

		bool ok = dc == oCapsuleDtorDigest;
		printf("capdtor candidate ok=%u dc=%08x\n", ok ? 1u : 0u, dc);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 29u);
		}

		// -- sphere set-radius: stored word must equal the driven value.
		{
		unsigned char sbytes[0xe4];
		memset(sbytes, 0xcd, sizeof(sbytes));
		SphereShape& sph = *new(sbytes) SphereShape(0, 0);
		sph.nxSphereSetRadius(1.25f);

		float got = sph.nxSphereGetRadius();
		unsigned radBits = 0;
		memcpy(&radBits, &got, 4);
		const unsigned kExpected = 0x3fa00000u;	// 1.25f
		bool ok = radBits == kExpected && oSphereSetDigest == kExpected;
		printf("setrad candidate ok=%u rad=%08x\n", ok ? 1u : 0u, radBits);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 25u);
		}

		// -- capsule set-radius: stored word must equal the driven value.
		{
		unsigned char sbytes[0xec];
		memset(sbytes, 0xcd, sizeof(sbytes));
		CapsuleShape& cap = *new(sbytes) CapsuleShape(0, 0);
		cap.nxCapsuleSetRadius(1.5f);

		float got = cap.mFloatE0;
		unsigned radBits = 0;
		memcpy(&radBits, &got, 4);
		const unsigned kExpected = 0x3fc00000u;	// 1.5f
		bool ok = radBits == kExpected && oCapsuleSetDigest == kExpected;
		printf("capsetrad candidate ok=%u rad=%08x\n", ok ? 1u : 0u, radBits);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 26u);
		}

		// -- group setter: the transcription must carry the group at +0xd8.
		{
		unsigned char sbytes[0xe4];
		memset(sbytes, 0xcd, sizeof(sbytes));
		SphereShape& sph = *new(sbytes) SphereShape(0, 0);
		sph.mBase.nxApplyGroup(7);

		unsigned short hw = 0;
		memcpy(&hw, sbytes + 0xd8, 2);
		bool ok = hw == 7 && oGroupDigest == 7;
		printf("setgroup candidate ok=%u hw_d8=%04x\n", ok ? 1u : 0u, hw);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 28u);
		

		// -- sphere loadFromDesc: twin drive with crafted record.
		{
		unsigned char sbytes[0xe4];
		memset(sbytes, 0xcd, sizeof(sbytes));
		SphereShape& sph = *new(sbytes) SphereShape(0, 0);

		unsigned char rec2[0x58];
		memset(rec2, 0xcd, sizeof(rec2));
		const float kR2 = 2.5f;
		memcpy(rec2 + 0x4c, &kR2, 4);
		const unsigned short kG2 = 0x0006u;
		memcpy(rec2 + 0x3c, &kG2, 2);
		memset(rec2 + 0x44, 0, 4);

		sph.mBase.nxApplyDescriptor(rec2);
		sph.nxSphereLoadFromDesc(rec2);

		float gotRad = 0.0f;
		memcpy(&gotRad, sbytes + 0xe0, 4);
		unsigned radBits = 0;
		memcpy(&radBits, &gotRad, 4);
		unsigned short hwD8 = 0;
		memcpy(&hwD8, sbytes + 0xd8, 2);

		bool ok = radBits == oSphereLoadRadBits && hwD8 == oSphereLoadGroup;
		printf("sphload candidate ok=%u rad=%08x group=%04x\n",
			ok ? 1u : 0u, radBits, hwD8);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 28u);
		}}

		// -- plane extent row: zero vec3 + +FLT_MAX reach, bitwise.
		{
		unsigned char pbytes[0x10c];
		memset(pbytes, 0xcd, sizeof(pbytes));
		PlaneShape& plane = *new(pbytes) PlaneShape(0, 0);

		float out[4] = { 0, 0, 0, 0 };
		plane.nxPlaneExtentRow(out);

		unsigned dp = 2166136261u;
		for(int i = 0; i < 4; ++i)
			{
			unsigned w; memcpy(&w, out + i, 4);
			dp = nxFold(dp, w);
			}
		const unsigned kBig = 0x7f7fffffu;
		bool ok = dp == oPlaneExtentDigest
			&& out[0] == 0.0f && out[1] == 0.0f && out[2] == 0.0f
			&& memcmp(&out[3], &kBig, 4) == 0;
		printf("planeext candidate ok=%u dp=%08x\n", ok ? 1u : 0u, dp);
		if(!ok)
			++candidateMissing;
		else
			candidateFold = nxFold(candidateFold, 27u);
		}

		printf("candidate CANDIDATE-MISSING family=vtables reason=shape finals/actor classes are Tasks 3-4\n");
		++candidateMissing;

	// -- Pure assert-report rows: bind the report slot and compare the
	//    captured (code, file, line, zero, expression) tuples.
	{
	struct RepRow { unsigned rva; unsigned nargs; void (*candidate)(); const char* name; };
	static const RepRow kRep[] = {
		{ 0xcea0, 0u, &nxAssertReport0364, "000364" },
		{ 0xd900, 0u, &nxAssertReport0408, "000408" },
		{ 0xd930, 0u, &nxAssertReport0410, "000410" },
		{ 0x8bea0, 0u, &nxAssertReport3750, "003750" },
		{ 0x8bf00, 0u, &nxAssertReport3754, "003754" },
		{ 0x8c1e0, 0u, &nxAssertReport3782, "003782" },
		{ 0xce70, 1u, &nxAssertReport000362, "000362" },
		{ 0xd8a0, 1u, &nxAssertReport000404, "000404" },
		{ 0xd8d0, 1u, &nxAssertReport000406, "000406" },
		{ 0x8bb50, 0u, &nxAssertReport003736, "003736" },
		{ 0x8bc70, 0u, &nxAssertReport003740, "003740" },
		{ 0x8be70, 1u, &nxAssertReport003748, "003748" },
		{ 0x8bed0, 1u, &nxAssertReport003752, "003752" },
		{ 0x8bf30, 2u, &nxAssertReport003756, "003756" },
		{ 0x8bf60, 1u, &nxAssertReport003758, "003758" },
		{ 0x8bf90, 2u, &nxAssertReport003760, "003760" },
		{ 0x8bfc0, 1u, &nxAssertReport003762, "003762" },
		{ 0x8bff0, 2u, &nxAssertReport003764, "003764" },
		{ 0x8c020, 1u, &nxAssertReport003766, "003766" },
		{ 0x8d4a0, 0u, &nxAssertReport003884, "003884" },
	};
	NxReportSaved svRep = nxBindReportSlot(base);
	nxSetAssertReport(&nxReportRecorder);
	unsigned rf = 0;
	for(unsigned i = 0; i < sizeof(kRep) / sizeof(kRep[0]); ++i)
		{
		typedef void (__thiscall* RepOracle)(void*);
		RepOracle fn = reinterpret_cast<RepOracle>(base + kRep[i].rva);
		unsigned char self[0x40]; memset(self, 0, sizeof(self));
		unsigned o[5];
		gRepCount = 0; memset(gRepCap, 0, sizeof(gRepCap));
		// the row pops its own argument count, so push exactly that many:
		// mis-calling a ret-8 row with one argument shifts the stack and
		// corrupts wmain's frame (the 3z156 mechanism).
		if(kRep[i].nargs == 0)
			reinterpret_cast<void (__thiscall*)(void*)>(fn)(self);
		else if(kRep[i].nargs == 1)
			reinterpret_cast<void (__thiscall*)(void*, unsigned)>(fn)(self, 0u);
		else
			reinterpret_cast<void (__thiscall*)(void*, unsigned, unsigned)>(fn)(self, 0u, 0u);
		memcpy(o, gRepCap, sizeof(o));
		unsigned nO = gRepCount;
		gRepCount = 0; memset(gRepCap, 0, sizeof(gRepCap));
		kRep[i].candidate();
		unsigned c[5];
		memcpy(c, gRepCap, sizeof(c));
		unsigned nC = gRepCount;
		if(nO != 1 || nC != 1 || memcmp(o, c, sizeof(o)) != 0)
			{
			fprintf(stderr,"assertrow %s nO=%u nC=%u O=%08x %08x %08x %08x %08x C=%08x %08x %08x %08x %08x\n",
				kRep[i].name, nO, nC, o[0],o[1],o[2],o[3],o[4], c[0],c[1],c[2],c[3],c[4]);
			++rf;
			}
		}
	nxSetAssertReport(nullptr);
	nxUnbindReportSlot(base, svRep);
	printf("assertrows candidate failures=%u provisional=1\n", rf);
	}
	// -- Word-return accessors: the high half is deterministic because the
	//    unlock helper leaves eax at 1 before the row's `mov ax,si` mask.
	{
	struct WordRow { unsigned rva; unsigned lockOff; unsigned fieldOff; unsigned dataOff; const char* name; };
	static const WordRow kW[] = {
		{ 0x24930, 0x14, 0x18, 0xd8, "001207" },
		{ 0x233a0, 0x14, 0x18, 0xda, "001061" },
	};
	NxLockApiSaved svW = nxBindLockApi(base);
	unsigned wf_ = 0;
	unsigned char lockObjW[0x40]; memset(lockObjW, 0, sizeof(lockObjW));
	unsigned char subObjW[0x40]; memset(subObjW, 0, sizeof(subObjW));
	*(void**)(lockObjW) = subObjW;
	unsigned char field[0x200];
	unsigned char sh[0x40];
	for(unsigned i = 0; i < sizeof(kW) / sizeof(kW[0]); ++i)
		{
		typedef unsigned (__thiscall* WordOracle)(void*);
		WordOracle fn = reinterpret_cast<WordOracle>(base + kW[i].rva);
		for(unsigned ci = 0; ci < 2; ++ci)
			{
			memset(field, 0, sizeof(field));
			unsigned short w = (ci == 0) ? 0xBEEFu : 0x0042u;
			memcpy(field + kW[i].dataOff, &w, 2);
			memset(sh, 0, sizeof(sh));
			*(void**)(sh + kW[i].lockOff) = lockObjW;
			*(void**)(sh + kW[i].fieldOff) = field;
			unsigned o = fn(sh);
			unsigned c = nxLockedWordRead(sh, kW[i].lockOff, kW[i].fieldOff, kW[i].dataOff);
			if(o != c || o != static_cast<unsigned>(w))
				{fprintf(stderr,"wordrow %s ci=%u o=%08x c=%08x want=%08x\n",
					kW[i].name, ci, o, c, (unsigned)w);++wf_;}
			}
		}
	nxUnbindLockApi(base, svW);
	printf("wordrows candidate failures=%u provisional=1\n", wf_);
	}
	// -- Gated locked reader 003708: both arms, gate slot bound each way.
	{
	typedef unsigned (__thiscall* Gate3708Oracle)(void*);
	Gate3708Oracle fn = reinterpret_cast<Gate3708Oracle>(base + 0x8b420);
	NxReportSaved svG = nxBindReportSlot(base);
	nxSetAssertReport(&nxReportRecorder);
	NxLockApiSaved svGL = nxBindLockApi(base);
	unsigned gf3708 = 0;
	unsigned char lockObjG[0x40]; memset(lockObjG, 0, sizeof(lockObjG));
	unsigned char subObjG[0x40]; memset(subObjG, 0, sizeof(subObjG));
	*(void**)(lockObjG) = subObjG;
	for(unsigned arm = 0; arm < 2; ++arm)
		{
		unsigned char value = (arm == 0) ? 1u : 0u;
		NxGateSaved svGate = nxBindGateSlot(base, value);
		nxSetGate3708(value);
		unsigned char field[0x40]; memset(field, 0, sizeof(field));
		unsigned char inner[0x60]; memset(inner, 0, sizeof(inner));
		unsigned rv = 0x600D0000u + arm;
		memcpy(inner + 0x38, &rv, 4);
		*(void**)(field + 0x24) = inner;
		unsigned char self[0x40]; memset(self, 0, sizeof(self));
		*(void**)(self + 0x10) = lockObjG;
		*(void**)(self + 0x14) = field;
		unsigned char selfC[0x40]; memcpy(selfC, self, sizeof(self));
		unsigned o[5];
		gRepCount = 0; memset(gRepCap, 0, sizeof(gRepCap));
		unsigned ro = fn(self);
		memcpy(o, gRepCap, sizeof(o));
		unsigned nO = gRepCount;
		gRepCount = 0; memset(gRepCap, 0, sizeof(gRepCap));
		unsigned rc = nxGuardedField3708(selfC);
		unsigned c[5];
		memcpy(c, gRepCap, sizeof(c));
		unsigned nC = gRepCount;
		if(ro != rc || nO != nC || memcmp(o, c, sizeof(o)) != 0)
			{fprintf(stderr,"gate3708 arm=%u ro=%08x rc=%08x nO=%u nC=%u\n", arm, ro, rc, nO, nC);++gf3708;}
		nxUnbindGateSlot(base, svGate);
		}
	nxSetGate3708(1u);
	nxUnbindLockApi(base, svGL);
	nxSetAssertReport(nullptr);
	nxUnbindReportSlot(base, svG);
	printf("gate3708 candidate failures=%u provisional=1\n", gf3708);
	}
	// -- Lock probe 000392: both arms, owner word set to match the query stub.
	{
	typedef unsigned char (__thiscall* ProbeOracle)(void*);
	ProbeOracle fn = reinterpret_cast<ProbeOracle>(base + 0xd660);
	NxLockApiSaved svP = nxBindLockApi(base);
	nxSetLockOwner(0x2222u);
	unsigned pf = 0;
	for(unsigned arm = 0; arm < 2; ++arm)
		{
		unsigned char lockObj[0x40]; memset(lockObj, 0, sizeof(lockObj));
		unsigned char subObj[0x40]; memset(subObj, 0, sizeof(subObj));
		*(void**)(lockObj) = subObj;
		unsigned owner = (arm == 0) ? 0x2222u : 0x1111u;
		memcpy(subObj + 0x1c, &owner, 4);
		unsigned char self[0x40]; memset(self, 0, sizeof(self));
		*(void**)(self + 0xc) = lockObj;
		unsigned char selfC[0x40]; memcpy(selfC, self, sizeof(self));
		unsigned char ro = fn(self);
		unsigned char rc = nxLockProbe0392(selfC);
		if(ro != rc || ro != static_cast<unsigned char>(arm == 0 ? 1 : 0))
			{fprintf(stderr,"probe0392 arm=%u ro=%u rc=%u\n", arm, ro, rc);++pf;}
		}
	nxSetLockOwner(0x2222u);
	nxUnbindLockApi(base, svP);
	printf("probe0392 candidate failures=%u provisional=1\n", pf);
	}
	// -- Mutex-guarded virtual dispatch family (31 rows): both arms each,
	//    with the vtable slots bound through the fixture table.
	{
	struct MvRow { unsigned rva; unsigned slot; unsigned code; unsigned file; unsigned line; unsigned expr; const char* name; };
	static const MvRow kMv[] = {
		{ 0x23460, 0x34, 0x2u, 0x10106d94u, 0x017u, 0x10104760u, "001067" },
		{ 0x23ad0, 0x34, 0x2u, 0x10106f0cu, 0x01au, 0x10104760u, "001111" },
		{ 0x23b90, 0x38, 0x2u, 0x10106f0cu, 0x02bu, 0x10104760u, "001115" },
		{ 0x24190, 0x34, 0x2u, 0x1010707cu, 0x020u, 0x10104760u, "001155" },
		{ 0x24700, 0x34, 0x2u, 0x101071e0u, 0x021u, 0x10104760u, "001189" },
		{ 0x24760, 0x38, 0x2u, 0x101071e0u, 0x029u, 0x10104760u, "001191" },
		{ 0x24de0, 0x34, 0x2u, 0x1010734cu, 0x022u, 0x10104760u, "001239" },
		{ 0xb0990, 0x24, 0x2u, 0x1011a794u, 0x015u, 0x10104760u, "004457" },
		{ 0xb09f0, 0x28, 0x2u, 0x1011a794u, 0x020u, 0x10104760u, "004459" },
		{ 0xb1050, 0x24, 0x2u, 0x1011a8fcu, 0x014u, 0x10104760u, "004501" },
		{ 0xb10b0, 0x28, 0x2u, 0x1011a8fcu, 0x01fu, 0x10104760u, "004503" },
		{ 0xb1490, 0x24, 0x2u, 0x1011aa5cu, 0x014u, 0x10104760u, "004527" },
		{ 0xb14f0, 0x28, 0x2u, 0x1011aa5cu, 0x01fu, 0x10104760u, "004529" },
		{ 0xb1960, 0x24, 0x2u, 0x1011abbcu, 0x014u, 0x10104760u, "004557" },
		{ 0xb19c0, 0x28, 0x2u, 0x1011abbcu, 0x01fu, 0x10104760u, "004559" },
		{ 0xb1e00, 0x24, 0x2u, 0x1011ad1cu, 0x013u, 0x10104760u, "004587" },
		{ 0xb1e60, 0x28, 0x2u, 0x1011ad1cu, 0x01eu, 0x10104760u, "004589" },
		{ 0xb2240, 0x24, 0x2u, 0x1011ae7cu, 0x014u, 0x10104760u, "004613" },
		{ 0xb22a0, 0x28, 0x2u, 0x1011ae7cu, 0x01fu, 0x10104760u, "004615" },
		{ 0xb26c0, 0x24, 0x2u, 0x1011afecu, 0x013u, 0x10104760u, "004641" },
		{ 0xb2720, 0x28, 0x2u, 0x1011afecu, 0x01eu, 0x10104760u, "004643" },
		{ 0xb2780, 0x2c, 0x2u, 0x1011afecu, 0x027u, 0x10104760u, "004645" },
		{ 0xb27e0, 0x34, 0x2u, 0x1011afecu, 0x034u, 0x10104760u, "004647" },
		{ 0xb2bc0, 0x24, 0x2u, 0x1011b15cu, 0x015u, 0x10104760u, "004671" },
		{ 0xb2c20, 0x28, 0x2u, 0x1011b15cu, 0x020u, 0x10104760u, "004673" },
		{ 0xb3000, 0x24, 0x2u, 0x1011b2ecu, 0x012u, 0x10104760u, "004697" },
		{ 0xb3060, 0x28, 0x2u, 0x1011b2ecu, 0x01du, 0x10104760u, "004699" },
		{ 0xb30c0, 0x2c, 0x2u, 0x1011b2ecu, 0x025u, 0x10104760u, "004701" },
		{ 0xb3150, 0x34, 0x2u, 0x1011b2ecu, 0x032u, 0x10104760u, "004705" },
		{ 0xb3750, 0x24, 0x2u, 0x1011b47cu, 0x013u, 0x10104760u, "004749" },
		{ 0xb37b0, 0x28, 0x2u, 0x1011b47cu, 0x01eu, 0x10104760u, "004751" },
	};
	NxReportSaved svMv = nxBindReportSlot(base);
	nxSetAssertReport(&nxReportRecorder);
	NxLockApiSaved svMvL = nxBindLockApi(base);
	nxSetLockOwner(0x2222u);
	unsigned mvf = 0;
	for(unsigned i = 0; i < sizeof(kMv) / sizeof(kMv[0]); ++i)
	for(unsigned arm = 0; arm < 2; ++arm)
		{
		typedef void (__thiscall* MvOracle)(void*, void*);
		MvOracle fn = reinterpret_cast<MvOracle>(base + kMv[i].rva);
		unsigned char lockObj[0x40]; memset(lockObj, 0, sizeof(lockObj));
		unsigned char subObj[0x40]; memset(subObj, 0, sizeof(subObj));
		*(void**)(lockObj) = subObj;
		unsigned owner = (arm == 0) ? 0x2222u : 0x1111u;
		memcpy(subObj + 0x1c, &owner, 4);
		void* vt[0x40 / 4 + 1];
		memset(vt, 0, sizeof(vt));
		for(unsigned s = 0x20; s <= 0x3c; s += 4)
			vt[s / 4] = reinterpret_cast<void*>(&nxSlot38Stub);
		unsigned char obj[0x20]; memset(obj, 0, sizeof(obj));
		*(void**)(obj) = vt;
		unsigned char self[0x40]; memset(self, 0, sizeof(self));
		*(void**)(self + 0x10) = lockObj;
		*(void**)(self + 0x18) = obj;
		unsigned char selfC[0x40]; memcpy(selfC, self, sizeof(self));
		unsigned o[5];
		gRepCount = 0; memset(gRepCap, 0, sizeof(gRepCap));
		gSlot38Hits = 0;
		fn(self, reinterpret_cast<void*>(0x1234u));
		memcpy(o, gRepCap, sizeof(o));
		unsigned nO = gRepCount;
		unsigned hO = gSlot38Hits;
		gRepCount = 0; memset(gRepCap, 0, sizeof(gRepCap));
		gSlot38Hits = 0;
		nxMutexVirtualEx(selfC, reinterpret_cast<void*>(0x1234u), kMv[i].slot,
			kMv[i].code, kMv[i].file, kMv[i].line, kMv[i].expr);
		unsigned c[5];
		memcpy(c, gRepCap, sizeof(c));
		unsigned nC = gRepCount;
		unsigned hC = gSlot38Hits;
		if(nO != nC || memcmp(o, c, sizeof(o)) != 0 || hO != hC
			|| memcmp(self, selfC, sizeof(self)) != 0)
			{fprintf(stderr,"mutexfamily %s arm=%u nO=%u nC=%u hO=%u hC=%u\n",
				kMv[i].name, arm, nO, nC, hO, hC);++mvf;}
		}
	nxSetLockOwner(0x2222u);
	nxUnbindLockApi(base, svMvL);
	nxSetAssertReport(nullptr);
	nxUnbindReportSlot(base, svMv);
	printf("mutexfamily candidate failures=%u provisional=1\n", mvf);
	}
	// -- Constant-argument virtual thunks (nine rows at 0xb0580..0xb0600).
	{
	struct VtRow { unsigned rva; unsigned slot; unsigned arg; const char* name; };
	static const VtRow kVt[] = {
		{ 0xb0580, 0x4c, 1, "004417" }, { 0xb0590, 0x4c, 5, "004419" },
		{ 0xb05a0, 0x4c, 4, "004421" }, { 0xb05b0, 0x4c, 0, "004423" },
		{ 0xb05c0, 0x4c, 2, "004425" }, { 0xb05d0, 0x4c, 3, "004427" },
		{ 0xb05e0, 0x4c, 8, "004429" }, { 0xb05f0, 0x4c, 6, "004431" },
		{ 0xb0600, 0x4c, 7, "004433" },
	};
	unsigned vtf = 0;
	for(unsigned i = 0; i < sizeof(kVt) / sizeof(kVt[0]); ++i)
		{
		typedef void (__thiscall* VtOracle)(void*);
		VtOracle fn = reinterpret_cast<VtOracle>(base + kVt[i].rva);
		void* vt[0x60 / 4 + 1];
		memset(vt, 0, sizeof(vt));
		vt[kVt[i].slot / 4] = reinterpret_cast<void*>(&nxVtConstStub);
		unsigned char self[0x20]; memset(self, 0, sizeof(self));
		*(void**)(self) = vt;
		unsigned char selfC[0x20]; memcpy(selfC, self, sizeof(self));
		gVtConstHits = 0; gVtConstArg = 0xffffffffu;
		fn(self);
		unsigned hO = gVtConstHits, aO = gVtConstArg;
		gVtConstHits = 0; gVtConstArg = 0xffffffffu;
		nxVtConstEx(selfC, kVt[i].slot, kVt[i].arg);
		unsigned hC = gVtConstHits, aC = gVtConstArg;
		if(hO != hC || aO != aC || hO != 1u || aO != kVt[i].arg)
			{fprintf(stderr,"vtconst %s hO=%u hC=%u aO=%u aC=%u\n",
				kVt[i].name, hO, hC, aO, aC);++vtf;}
		}
	printf("vtconst candidate failures=%u provisional=1\n", vtf);
	}
	// -- Multi-argument virtual dispatch thunks: 002390, 003924, 001965.
	{
	unsigned thf = 0;
	// 002390: __cdecl slot, one argument from [obj+0x10]
	{
	typedef void (__thiscall* T2390)(void*);
	T2390 fn = reinterpret_cast<T2390>(base + 0x5b910);
	unsigned char obj[0x20]; memset(obj, 0, sizeof(obj));
	*(void**)(obj + 0xc) = reinterpret_cast<void*>(&nxRec1C);
	unsigned arg = 0xA1B2C3D4u; memcpy(obj + 0x10, &arg, 4);
	unsigned char self[0x20]; memset(self, 0, sizeof(self));
	*(void**)(self + 4) = obj;
	unsigned char selfC[0x20]; memcpy(selfC, self, sizeof(self));
	gVtRecN = 0; memset(gVtRec, 0, sizeof(gVtRec));
	fn(self);
	unsigned nO = gVtRecN, rO = gVtRec[0];
	gVtRecN = 0; memset(gVtRec, 0, sizeof(gVtRec));
	nxVtCall2390(selfC);
	unsigned nC = gVtRecN, rC = gVtRec[0];
	if(nO != nC || rO != rC || nO != 1u || rO != arg)
		{fprintf(stderr,"thunk2390 nO=%u nC=%u rO=%08x rC=%08x\n", nO, nC, rO, rC);++thf;}
	}
	// 003924: __stdcall slot, two arguments from [self+0x24]/[self+0x28]
	{
	typedef void (__thiscall* T3924)(void*);
	T3924 fn = reinterpret_cast<T3924>(base + 0x8ed50);
	void* vt[0x10 / 4 + 1]; memset(vt, 0, sizeof(vt));
	vt[0xc / 4] = reinterpret_cast<void*>(&nxRec2S);
	unsigned char self[0x40]; memset(self, 0, sizeof(self));
	*(void**)(self) = vt;
	unsigned a = 0x11112222u, b = 0x33334444u;
	memcpy(self + 0x24, &a, 4); memcpy(self + 0x28, &b, 4);
	unsigned char selfC[0x40]; memcpy(selfC, self, sizeof(self));
	gVtRecN = 0; memset(gVtRec, 0, sizeof(gVtRec));
	fn(self);
	unsigned nO = gVtRecN, rO0 = gVtRec[0], rO1 = gVtRec[1];
	gVtRecN = 0; memset(gVtRec, 0, sizeof(gVtRec));
	nxVtCall3924(selfC);
	unsigned nC = gVtRecN, rC0 = gVtRec[0], rC1 = gVtRec[1];
	if(nO != nC || rO0 != rC0 || rO1 != rC1 || nO != 2u || rO0 != a || rO1 != b)
		{fprintf(stderr,"thunk3924 nO=%u nC=%u %08x/%08x vs %08x/%08x\n", nO, nC, rO0, rO1, rC0, rC1);++thf;}
	}
	// 001965: __stdcall slot, three arguments, object is the second arg
	{
	typedef void (__cdecl* T1965)(void*, void*);
	T1965 fn = reinterpret_cast<T1965>(base + 0x4c000);
	void* vt[0x30 / 4 + 1]; memset(vt, 0, sizeof(vt));
	vt[0x2c / 4] = reinterpret_cast<void*>(&nxRec3S);
	unsigned char obj[0x20]; memset(obj, 0, sizeof(obj));
	*(void**)(obj) = vt;
	unsigned char selfC[0x20]; memset(selfC, 0, sizeof(selfC));
	gVtRecN = 0; memset(gVtRec, 0, sizeof(gVtRec));
	fn(reinterpret_cast<void*>(0x55667788u), obj);
	unsigned nO = gVtRecN, rO0 = gVtRec[0], rO1 = gVtRec[1], rO2 = gVtRec[2];
	gVtRecN = 0; memset(gVtRec, 0, sizeof(gVtRec));
	nxVtCall1965(reinterpret_cast<void*>(0x55667788u), obj);
	unsigned nC = gVtRecN, rC0 = gVtRec[0], rC1 = gVtRec[1], rC2 = gVtRec[2];
	if(nO != nC || rO0 != rC0 || rO1 != rC1 || rO2 != rC2 || nO != 3u)
		{fprintf(stderr,"thunk1965 nO=%u nC=%u %08x/%08x/%08x vs %08x/%08x/%08x\n",
			nO, nC, rO0, rO1, rO2, rC0, rC1, rC2);++thf;}
	}
	printf("thunks3 candidate failures=%u provisional=1\n", thf);
	}
	// -- Lock-bracketed vtable calls: 001237 (slot +0x44) and 001119 (+0x3c).
	{
	NxLockApiSaved svLV = nxBindLockApi(base);
	unsigned lvf = 0;
	// 001237: unsigned result
	{
	typedef unsigned (__thiscall* T1237)(void*);
	T1237 fn = reinterpret_cast<T1237>(base + 0x24db0);
	void* vt[0x48 / 4 + 1]; memset(vt, 0, sizeof(vt));
	vt[0x44 / 4] = reinterpret_cast<void*>(&nxRecThis44);
	unsigned char obj[0x20]; memset(obj, 0, sizeof(obj));
	*(void**)(obj) = vt;
	unsigned char lockObj[0x40]; memset(lockObj, 0, sizeof(lockObj));
	unsigned char subObj[0x40]; memset(subObj, 0, sizeof(subObj));
	*(void**)(lockObj) = subObj;
	unsigned char self[0x40]; memset(self, 0, sizeof(self));
	*(void**)(self + 0x14) = lockObj;
	*(void**)(self + 0x18) = obj;
	unsigned char selfC[0x40]; memcpy(selfC, self, sizeof(self));
	gVtRecN = 0; memset(gVtRec, 0, sizeof(gVtRec));
	unsigned ro = fn(self);
	unsigned nO = gVtRecN, sO = gVtRec[0];
	gVtRecN = 0; memset(gVtRec, 0, sizeof(gVtRec));
	unsigned rc = nxLockedVtCall1237(selfC);
	unsigned nC = gVtRecN, sC = gVtRec[0];
	if(ro != rc || nO != nC || sO != sC || nO != 1u
		|| sO != static_cast<unsigned>(reinterpret_cast<size_t>(obj)))
		{fprintf(stderr,"vtcall1237 ro=%08x rc=%08x nO=%u nC=%u sO=%08x sC=%08x\n",
			ro, rc, nO, nC, sO, sC);++lvf;}
	}
	// 001119: float result returned in st(0)
	{
	typedef float (__thiscall* T1119)(void*);
	T1119 fn = reinterpret_cast<T1119>(base + 0x23c50);
	void* vt[0x40 / 4 + 1]; memset(vt, 0, sizeof(vt));
	vt[0x3c / 4] = reinterpret_cast<void*>(&nxRecThis3c);
	unsigned char obj[0x20]; memset(obj, 0, sizeof(obj));
	*(void**)(obj) = vt;
	unsigned char lockObj[0x40]; memset(lockObj, 0, sizeof(lockObj));
	unsigned char subObj[0x40]; memset(subObj, 0, sizeof(subObj));
	*(void**)(lockObj) = subObj;
	unsigned char self[0x40]; memset(self, 0, sizeof(self));
	*(void**)(self + 0x14) = lockObj;
	*(void**)(self + 0x18) = obj;
	unsigned char selfC[0x40]; memcpy(selfC, self, sizeof(self));
	gVtRecN = 0; memset(gVtRec, 0, sizeof(gVtRec));
	float ro = fn(self);
	unsigned nO = gVtRecN, sO = gVtRec[0];
	gVtRecN = 0; memset(gVtRec, 0, sizeof(gVtRec));
	float rc = nxLockedVtCall1119(selfC);
	unsigned nC = gVtRecN, sC = gVtRec[0];
	if(memcmp(&ro, &rc, 4) != 0 || nO != nC || sO != sC || nO != 1u)
		{fprintf(stderr,"vtcall1119 ro=%08x rc=%08x nO=%u nC=%u\n",
			*(unsigned*)&ro, *(unsigned*)&rc, nO, nC);++lvf;}
	}
	nxUnbindLockApi(base, svLV);
	printf("vtcall pair candidate failures=%u provisional=1\n", lvf);
	}
	// -- Per-element virtual dispatch loop 001022.
	{
	typedef void (__thiscall* T1022)(void*, void*);
	T1022 fn = reinterpret_cast<T1022>(base + 0x22970);
	unsigned lf = 0;
	for(unsigned count = 0; count < 3; ++count)
		{
		void* vt[0x10 / 4 + 1]; memset(vt, 0, sizeof(vt));
		vt[0xc / 4] = reinterpret_cast<void*>(&nxLoopStub);
		unsigned char elems[3][0x20];
		memset(elems, 0, sizeof(elems));
		for(unsigned k = 0; k < 3; ++k) *(void**)(elems[k]) = vt;
		unsigned char self[0x100]; memset(self, 0, sizeof(self));
		unsigned* list = reinterpret_cast<unsigned*>(self + 0x40);
		for(unsigned k = 0; k < count; ++k) list[k] = static_cast<unsigned>(reinterpret_cast<size_t>(elems[k]));
		*(void**)(self + 0xe0) = list;
		*(void**)(self + 0xe4) = list + count;
		unsigned char selfC[0x100]; memcpy(selfC, self, sizeof(self));
		gLoopHits = 0; gLoopArg = 0;
		fn(self, reinterpret_cast<void*>(0xC0DE1234u));
		unsigned hO = gLoopHits, aO = gLoopArg;
		gLoopHits = 0; gLoopArg = 0;
		nxArrayVtCall1022(selfC, reinterpret_cast<void*>(0xC0DE1234u));
		unsigned hC = gLoopHits, aC = gLoopArg;
		// with no elements there are no calls, so the argument is never recorded
		const unsigned wantArg = (count == 0) ? 0u : 0xC0DE1234u;
		if(hO != hC || aO != aC || hO != count || aO != wantArg)
			{fprintf(stderr,"arrayloop count=%u hO=%u hC=%u aO=%08x aC=%08x\n",
				count, hO, hC, aO, aC);++lf;}
		}
	printf("arrayloop candidate failures=%u provisional=1\n", lf);
	}
	// -- Scalar deleting destructor 002142: both arms, allocator slot bound.
	{
	typedef void* (__thiscall* T2142)(void*, unsigned);
	T2142 fn = reinterpret_cast<T2142>(base + 0x532b0);
	// a fake allocator: holder -> object -> vtable[0x14]
	void* allocVt[0x18 / 4 + 1]; memset(allocVt, 0, sizeof(allocVt));
	allocVt[0x14 / 4] = reinterpret_cast<void*>(&nxFreeRecorder);
	unsigned char allocObj[0x20]; memset(allocObj, 0, sizeof(allocObj));
	*(void**)(allocObj) = allocVt;
	unsigned char holder[0x10]; memset(holder, 0, sizeof(holder));
	*(void**)(holder) = allocObj;
	NxAllocSaved svAl = nxBindAllocSlot(base, holder);
	nxSetAllocFree(&nxFreeRecorder);
	unsigned df = 0;
	for(unsigned arm = 0; arm < 2; ++arm)
		{
		unsigned flags = arm ? 1u : 0u;
		unsigned char self[0x40], selfC[0x40];
		memset(self, 0xcd, sizeof(self)); memset(selfC, 0xcd, sizeof(selfC));
		gFreeHits = 0; gFreeArg = nullptr;
		void* ro = fn(self, flags);
		unsigned hO = gFreeHits; void* aO = gFreeArg;
		gFreeHits = 0; gFreeArg = nullptr;
		void* rc = nxScalarDeletingDtor2142(selfC, flags);
		unsigned hC = gFreeHits; void* aC = gFreeArg;
		// each side frees its OWN buffer, so compare the freed pointer against
		// that side's self rather than against the other side's
		if(ro != self || rc != selfC || hO != hC
			|| memcmp(self, selfC, sizeof(self)) != 0 || hO != arm
			|| (arm == 1 && (aO != self || aC != selfC)))
			{fprintf(stderr,"dtor2142 arm=%u ro=%p rc=%p hO=%u hC=%u aO=%p aC=%p\n",
				arm, ro, rc, hO, hC, aO, aC);++df;}
		}
	nxSetAllocFree(nullptr);
	nxUnbindAllocSlot(base, svAl);
	printf("dtor2142 candidate failures=%u provisional=1\n", df);
	}
	// -- Mutex-guarded direct-call family: group A calls 001329, group B calls
	//    the bare-ret no-op 004248. Both arms each.
	{
	struct MdRow { unsigned rva; unsigned code; unsigned file; unsigned line; const char* name; };
	static const MdRow kMdA[] = {
		{ 0x23040, 0x2u, 0x10106d94u, 0x00fu, "001043" },
		{ 0x23610, 0x2u, 0x10106f0cu, 0x010u, "001081" },
		{ 0x23d40, 0x2u, 0x1010707cu, 0x016u, "001129" },
		{ 0x242e0, 0x2u, 0x101071e0u, 0x016u, "001165" },
		{ 0x248d0, 0x2u, 0x1010734cu, 0x010u, "001205" },
	};
	static const MdRow kMdB[] = {
		{ 0xb0a50, 0x2u, 0x1011a794u, 0x028u, "004461" },
		{ 0xb0ab0, 0x2u, 0x1011a794u, 0x02fu, "004463" },
		{ 0xb0b10, 0x2u, 0x1011a794u, 0x036u, "004465" },
		{ 0xb0b70, 0x2u, 0x1011a794u, 0x03du, "004467" },
	};
	NxReportSaved svMd = nxBindReportSlot(base);
	nxSetAssertReport(&nxReportRecorder);
	NxLockApiSaved svMdL = nxBindLockApi(base);
	nxSetLockOwner(0x2222u);
	unsigned mdf = 0;
	for(unsigned grp = 0; grp < 2; ++grp)
		{
		const MdRow* rows = (grp == 0) ? kMdA : kMdB;
		const unsigned n = (grp == 0) ? (sizeof(kMdA) / sizeof(kMdA[0]))
			: (sizeof(kMdB) / sizeof(kMdB[0]));
		for(unsigned i = 0; i < n; ++i)
		for(unsigned arm = 0; arm < 2; ++arm)
			{
			typedef void (__thiscall* MdOracle)(void*, unsigned);
			MdOracle fn = reinterpret_cast<MdOracle>(base + rows[i].rva);
			unsigned char lockObj[0x40]; memset(lockObj, 0, sizeof(lockObj));
			unsigned char subObj[0x40]; memset(subObj, 0, sizeof(subObj));
			*(void**)(lockObj) = subObj;
			unsigned owner = (arm == 0) ? 0x2222u : 0x1111u;
			memcpy(subObj + 0x1c, &owner, 4);
			unsigned char obj[0x200]; memset(obj, 0, sizeof(obj));
			unsigned char self[0x40]; memset(self, 0, sizeof(self));
			*(void**)(self + 0x10) = lockObj;
			*(void**)(self + 0x18) = obj;
			unsigned char selfC[0x40]; memcpy(selfC, self, sizeof(self));
			unsigned char objC[0x200]; memcpy(objC, obj, sizeof(objC));
			// selfC was copied AFTER self+0x18 was set, so it still points at
			// obj -- re-point it at objC or both sides write the same buffer.
			*(void**)(selfC + 0x18) = objC;
			unsigned o[5];
			gRepCount = 0; memset(gRepCap, 0, sizeof(gRepCap));
			fn(self, 0x00050003u);
			memcpy(o, gRepCap, sizeof(o));
			unsigned nO = gRepCount;
			gRepCount = 0; memset(gRepCap, 0, sizeof(gRepCap));
			if(grp == 0)
				nxMutexApplyGroupEx(selfC, 0x00050003u, rows[i].code, rows[i].file, rows[i].line, 0x10104760u);
			else
				nxMutexNoopEx(selfC, rows[i].code, rows[i].file, rows[i].line, 0x10104760u);
			unsigned c[5];
			memcpy(c, gRepCap, sizeof(c));
			unsigned nC = gRepCount;
			if(nO != nC || memcmp(o, c, sizeof(o)) != 0
				|| memcmp(obj, objC, sizeof(obj)) != 0)
				{
				fprintf(stderr,"mutexdirect %s arm=%u nO=%u nC=%u\n",
					rows[i].name, arm, nO, nC);
				for(unsigned k = 0; k < sizeof(obj); ++k)
					if(obj[k] != objC[k])
						{ fprintf(stderr,"  obj[%03x] o=%02x c=%02x\n", k, obj[k], objC[k]); break; }
				++mdf;
			}
			}
		}
	nxSetLockOwner(0x2222u);
	nxUnbindLockApi(base, svMdL);
	nxSetAssertReport(nullptr);
	nxUnbindReportSlot(base, svMd);
	printf("mutexdirect candidate failures=%u provisional=1\n", mdf);
	}
	// -- Direct probe of 001329 (ShapeBase::nxApplyGroup) against its
	//    candidate: this is the field the 3z167 divergence pointed at.
	{
	typedef void (__thiscall* ApplyGroupOracle)(void*, unsigned short);
	ApplyGroupOracle ag = reinterpret_cast<ApplyGroupOracle>(base + 0x26d90);
	unsigned agf = 0;
	for(unsigned gi = 0; gi < 3; ++gi)
		{
		unsigned short grp = (gi == 0) ? 3u : ((gi == 1) ? 0x21u : 0u);
		unsigned char a[0x200], b[0x200];
		memset(a, 0, sizeof(a)); memset(b, 0, sizeof(b));
		unsigned char aCopy[0x200]; memcpy(aCopy, a, sizeof(a));
		ag(a, grp);
		((ShapeBase*)b)->nxApplyGroup(grp);
		if(memcmp(a, b, sizeof(a)) != 0)
			{
			unsigned k = 0;
			while(k < sizeof(a) && a[k] == b[k]) ++k;
			fprintf(stderr,"applygroup grp=%04x first diff at %03x o=%02x c=%02x (d8 o=%02x c=%02x)\n",
				grp, k, a[k], b[k], a[0xd8], b[0xd8]);
			++agf;
			}
		(void)aCopy;
		}
	printf("applygroup probe failures=%u provisional=1\n", agf);
	}
	// -- Lock-bracketed vtable calls with caller-chosen slots: 004703 and 001209.
	{
	NxLockApiSaved svLv = nxBindLockApi(base);
	unsigned lvf2 = 0;
	// 004703: slot +0x30, no arguments, returns the slot value
	{
	typedef unsigned (__thiscall* T4703)(void*);
	T4703 fn = reinterpret_cast<T4703>(base + 0xb3120);
	void* vt[0x34 / 4 + 1]; memset(vt, 0, sizeof(vt));
	vt[0x30 / 4] = reinterpret_cast<void*>(&nxRecThis30);
	unsigned char obj[0x20]; memset(obj, 0, sizeof(obj));
	*(void**)(obj) = vt;
	unsigned char lockObj[0x40]; memset(lockObj, 0, sizeof(lockObj));
	unsigned char subObj[0x40]; memset(subObj, 0, sizeof(subObj));
	*(void**)(lockObj) = subObj;
	unsigned char self[0x40]; memset(self, 0, sizeof(self));
	*(void**)(self + 0x14) = lockObj;
	*(void**)(self + 0x18) = obj;
	unsigned char selfC[0x40]; memcpy(selfC, self, sizeof(self));
	gVtRecN = 0; memset(gVtRec, 0, sizeof(gVtRec));
	unsigned ro = fn(self);
	unsigned nO = gVtRecN, sO = gVtRec[0];
	gVtRecN = 0; memset(gVtRec, 0, sizeof(gVtRec));
	unsigned rc = nxLockedVtCallNoArg(selfC, 0x30);
	unsigned nC = gVtRecN, sC = gVtRec[0];
	if(ro != rc || nO != nC || sO != sC || nO != 1u
		|| sO != static_cast<unsigned>(reinterpret_cast<size_t>(obj)))
		{fprintf(stderr,"vtcall4703 ro=%08x rc=%08x nO=%u nC=%u\n", ro, rc, nO, nC);++lvf2;}
	}
	// 001209: slot +0x24, one argument, void result
	{
	typedef void (__thiscall* T1209)(void*, void*);
	T1209 fn = reinterpret_cast<T1209>(base + 0x24960);
	void* vt[0x28 / 4 + 1]; memset(vt, 0, sizeof(vt));
	vt[0x24 / 4] = reinterpret_cast<void*>(&nxRecArg24);
	unsigned char obj[0x20]; memset(obj, 0, sizeof(obj));
	*(void**)(obj) = vt;
	unsigned char lockObj[0x40]; memset(lockObj, 0, sizeof(lockObj));
	unsigned char subObj[0x40]; memset(subObj, 0, sizeof(subObj));
	*(void**)(lockObj) = subObj;
	unsigned char self[0x40]; memset(self, 0, sizeof(self));
	*(void**)(self + 0x14) = lockObj;
	*(void**)(self + 0x18) = obj;
	unsigned char selfC[0x40]; memcpy(selfC, self, sizeof(self));
	gVtRecN = 0; memset(gVtRec, 0, sizeof(gVtRec));
	fn(self, reinterpret_cast<void*>(0x77000000u));
	unsigned nO = gVtRecN, sO = gVtRec[0];
	gVtRecN = 0; memset(gVtRec, 0, sizeof(gVtRec));
	nxLockedVtCallArg(selfC, 0x24, reinterpret_cast<void*>(0x77000000u));
	unsigned nC = gVtRecN, sC = gVtRec[0];
	if(nO != nC || sO != sC || nO != 1u || sO != 0x77000000u)
		{fprintf(stderr,"vtcall1209 nO=%u nC=%u sO=%08x sC=%08x\n", nO, nC, sO, sC);++lvf2;}
	}
	nxUnbindLockApi(base, svLv);
	printf("vtcall slots candidate failures=%u provisional=1\n", lvf2);
	}
	// -- Report-once dispatch rows: 000335, 000336, 000390. Each is driven on
	//    the first call (gate clear -> report) and again with the gate set.
	{
	struct OnceRow { unsigned rva; unsigned gateRva; unsigned slot; unsigned nargs;
		unsigned a1kind; unsigned a2kind; unsigned code; unsigned file; unsigned line;
		unsigned expr; const char* name; };
	// a1kind/a2kind: 0 = literal 1, 1 = the row's first argument, 2 = its second
	static const OnceRow kOnce[] = {
		{ 0xca40, 0x1237c1, 0x100, 1, 1, 0, 0xd0, 0x10105ba8, 0x114, 0x10105bd8, "000335" },
		{ 0xca90, 0x1237c2, 0x108, 2, 0, 0, 0xd0, 0x10105ba8, 0x11a, 0x10105c30, "000336" },
		{ 0xd600, 0x1237c4, 0x104, 2, 0, 2, 0xd0, 0x10105ba8, 0x204, 0x10105f68, "000390" },
	};
	NxReportSaved svOn = nxBindReportSlot(base);
	nxSetAssertReport(&nxReportRecorder);
	unsigned onf = 0;
	static unsigned char gOnceGate[3];
	for(unsigned i = 0; i < sizeof(kOnce) / sizeof(kOnce[0]); ++i)
		{
		NxGateSaved svG1 = nxBindGateSlot(base, 0);
		gOnceGate[i] = 0;
		void* vt[0x110 / 4 + 1]; memset(vt, 0, sizeof(vt));
		vt[kOnce[i].slot / 4] = (kOnce[i].nargs <= 1)
			? reinterpret_cast<void*>(&nxRecOnce1)
			: reinterpret_cast<void*>(&nxRecOnce2);
		unsigned char self[0x20]; memset(self, 0, sizeof(self));
		*(void**)(self) = vt;
		unsigned char selfC[0x20]; memcpy(selfC, self, sizeof(self));
		for(unsigned pass = 0; pass < 2; ++pass)
			{
			unsigned o[5];
			gRepCount = 0; memset(gRepCap, 0, sizeof(gRepCap));
			gVtRecN = 0; memset(gVtRec, 0, sizeof(gVtRec));
			unsigned nO, nC, rnO, rnC;
			unsigned rO[2] = { 0, 0 }, rC[2] = { 0, 0 };
			if(kOnce[i].rva == 0xca40)
				{ reinterpret_cast<void (__thiscall*)(void*, unsigned)>(base + kOnce[i].rva)(self, 0x11u); }
			else if(kOnce[i].rva == 0xca90)
				{ reinterpret_cast<void (__thiscall*)(void*)>(base + kOnce[i].rva)(self); }
			else
				{ reinterpret_cast<void (__thiscall*)(void*, unsigned, unsigned)>(base + kOnce[i].rva)(self, 0x11u, 0x22u); }
			memcpy(o, gRepCap, sizeof(o));
			nO = gRepCount;
			rnO = gVtRecN; rO[0] = gVtRec[0]; rO[1] = gVtRec[1];
			unsigned a1 = (kOnce[i].a1kind == 1) ? 0x11u : ((kOnce[i].a1kind == 2) ? 0x22u : 1u);
			unsigned a2 = (kOnce[i].a2kind == 1) ? 0x11u : ((kOnce[i].a2kind == 2) ? 0x22u : 1u);
			gRepCount = 0; memset(gRepCap, 0, sizeof(gRepCap));
			gVtRecN = 0; memset(gVtRec, 0, sizeof(gVtRec));
			nxOnceReportVtEx(selfC, &gOnceGate[i], kOnce[i].slot, kOnce[i].nargs,
				a1, a2, kOnce[i].code, kOnce[i].file, kOnce[i].line, kOnce[i].expr);
			unsigned c[5];
			memcpy(c, gRepCap, sizeof(c));
			nC = gRepCount;
			rnC = gVtRecN; rC[0] = gVtRec[0]; rC[1] = gVtRec[1];
			if(nO != nC || memcmp(o, c, sizeof(o)) != 0 || rnO != rnC
				|| rO[0] != rC[0] || rO[1] != rC[1])
				{fprintf(stderr,"oncedispatch %s pass=%u nO=%u nC=%u rnO=%u rnC=%u\n",
					kOnce[i].name, pass, nO, nC, rnO, rnC);++onf;}
			}
		nxUnbindGateSlot(base, svG1);
		}
	nxSetAssertReport(nullptr);
	nxUnbindReportSlot(base, svOn);
	printf("oncedispatch candidate failures=%u provisional=1\n", onf);
	}
	// -- Deleting destructor with a global call: 003938, both arms.
	{
	typedef void* (__thiscall* T3938)(void*, unsigned);
	T3938 fn = reinterpret_cast<T3938>(base + 0x8eec0);
	NxGlobalSaved svGl = nxBindGlobalSlot(base);
	void* allocVt[0x18 / 4 + 1]; memset(allocVt, 0, sizeof(allocVt));
	allocVt[0x14 / 4] = reinterpret_cast<void*>(&nxFreeRecorder);
	unsigned char allocObj[0x20]; memset(allocObj, 0, sizeof(allocObj));
	*(void**)(allocObj) = allocVt;
	unsigned char holder[0x10]; memset(holder, 0, sizeof(holder));
	*(void**)(holder) = allocObj;
	NxAllocSaved svAl3 = nxBindAllocSlot(base, holder);
	nxSetAllocFree(&nxFreeRecorder);
	nxSetGlobalHook3938(&nxGlobalRecorder);
	unsigned gf3 = 0;
	for(unsigned arm = 0; arm < 2; ++arm)
		{
		unsigned flags = arm ? 1u : 0u;
		unsigned char self[0x40], selfC[0x40];
		memset(self, 0xcd, sizeof(self)); memset(selfC, 0xcd, sizeof(selfC));
		gGlobalHits = 0; gFreeHits = 0; gFreeArg = nullptr;
		void* ro = fn(self, flags);
		unsigned gO = gGlobalHits, hO = gFreeHits; void* aO = gFreeArg;
		gGlobalHits = 0; gFreeHits = 0; gFreeArg = nullptr;
		void* rc = nxDtorWithGlobal3938(selfC, flags);
		unsigned gC = gGlobalHits, hC = gFreeHits; void* aC = gFreeArg;
		if(ro != self || rc != selfC || gO != gC || hO != hC
			|| memcmp(self, selfC, sizeof(self)) != 0
			|| gO != 1u || hO != arm
			|| (arm == 1 && (aO != self || aC != selfC)))
			{fprintf(stderr,"dtor3938 arm=%u gO=%u gC=%u hO=%u hC=%u\n", arm, gO, gC, hO, hC);++gf3;}
		}
	nxSetGlobalHook3938(nullptr);
	nxSetAllocFree(nullptr);
	nxUnbindAllocSlot(base, svAl3);
	nxUnbindGlobalSlot(base, svGl);
	printf("dtor3938 candidate failures=%u provisional=1\n", gf3);
	}
	// -- Guarded store family: 004184, 004288, 004292, 004338. Both arms each.
	{
	struct GsRow { unsigned rva; unsigned fieldOff; unsigned file; unsigned line;
		unsigned expr; const char* name; };
	static const GsRow kGs[] = {
		{ 0xa8f10, 0x1a8, 0x1011a204, 0x09d, 0x1011a290, "004334" },
		{ 0x9b590, 0x044, 0x101195b0, 0x0b1, 0x10119628, "004184" },
		{ 0xa2ee0, 0x1d0, 0x10119e64, 0x083, 0x10119ef0, "004288" },
		{ 0xa2f40, 0x044, 0x10119e64, 0x08e, 0x10119f40, "004292" },
		{ 0xa8fc0, 0x044, 0x1011a204, 0x0ae, 0x1011a2e0, "004338" },
	};
	NxReportSaved svGs = nxBindReportSlot(base);
	nxSetAssertReport(&nxReportRecorder);
	unsigned gsf = 0;
	for(unsigned i = 0; i < sizeof(kGs) / sizeof(kGs[0]); ++i)
	for(unsigned arm = 0; arm < 2; ++arm)
		{
		typedef void (__thiscall* GsOracle)(void*, unsigned);
		GsOracle fn = reinterpret_cast<GsOracle>(base + kGs[i].rva);
		unsigned char self[0x200], selfC[0x200];
		memset(self, 0, sizeof(self)); memset(selfC, 0, sizeof(selfC));
		// arm 0: the guard byte is clear -> store. arm 1: equal 0x10 -> report.
		self[0x2c] = (arm == 0) ? 0x00 : 0x10;
		selfC[0x2c] = self[0x2c];
		unsigned o[5];
		gRepCount = 0; memset(gRepCap, 0, sizeof(gRepCap));
		fn(self, 0x11223344u);
		memcpy(o, gRepCap, sizeof(o));
		unsigned nO = gRepCount;
		gRepCount = 0; memset(gRepCap, 0, sizeof(gRepCap));
		nxGuardedStoreEx(selfC, 0x11223344u, kGs[i].fieldOff, 1u, kGs[i].file,
			kGs[i].line, kGs[i].expr);
		unsigned c[5];
		memcpy(c, gRepCap, sizeof(c));
		unsigned nC = gRepCount;
		unsigned storedO = 0, storedC = 0;
		memcpy(&storedO, self + kGs[i].fieldOff, 4);
		memcpy(&storedC, selfC + kGs[i].fieldOff, 4);
		if(nO != nC || memcmp(o, c, sizeof(o)) != 0
			|| memcmp(self, selfC, sizeof(self)) != 0
			|| nO != arm || storedO != storedC
			|| (arm == 0 && storedO != 0x11223344u))
			{fprintf(stderr,"guardedstore %s arm=%u nO=%u nC=%u storedO=%08x storedC=%08x\n",
				kGs[i].name, arm, nO, nC, storedO, storedC);++gsf;}
		}
	nxSetAssertReport(nullptr);
	nxUnbindReportSlot(base, svGs);
	printf("guardedstore candidate failures=%u provisional=1\n", gsf);
	}
	// -- Lock-first direct-call family: 000350/000354/000358 store the argument
	//    through their helper, 003870's helper is a no-op. Both arms each.
	{
	struct MwRow { unsigned rva; unsigned lockOff; unsigned objOff; int workKind;
		unsigned storeOff; unsigned file; unsigned line; unsigned expr; const char* name; };
	static const MwRow kMw[] = {
		{ 0xccc0, 0x0c, 0x24, 1, 0x6ac, 0x10105ba8, 0x150, 0x10104760, "000350" },
		{ 0xcd50, 0x0c, 0x24, 1, 0x6b0, 0x10105ba8, 0x15d, 0x10104760, "000354" },
		{ 0xcde0, 0x0c, 0x24, 1, 0x6b4, 0x10105ba8, 0x16a, 0x10104760, "000358" },
		{ 0x8d140, 0x0c, 0x14, 0, 0, 0x1011681c, 0x00c, 0x10104760, "003870" },
	};
	NxReportSaved svMw = nxBindReportSlot(base);
	nxSetAssertReport(&nxReportRecorder);
	NxLockApiSaved svMwL = nxBindLockApi(base);
	nxSetLockOwner(0x2222u);
	unsigned mwf = 0;
	for(unsigned i = 0; i < sizeof(kMw) / sizeof(kMw[0]); ++i)
	for(unsigned arm = 0; arm < 2; ++arm)
		{
		typedef void (__thiscall* MwOracle)(void*, unsigned);
		MwOracle fn = reinterpret_cast<MwOracle>(base + kMw[i].rva);
		unsigned char lockObj[0x40]; memset(lockObj, 0, sizeof(lockObj));
		unsigned char subObj[0x40]; memset(subObj, 0, sizeof(subObj));
		*(void**)(lockObj) = subObj;
		unsigned owner = (arm == 0) ? 0x2222u : 0x1111u;
		memcpy(subObj + 0x1c, &owner, 4);
		unsigned char obj[0x800]; memset(obj, 0, sizeof(obj));
		unsigned char self[0x40]; memset(self, 0, sizeof(self));
		*(void**)(self + kMw[i].lockOff) = lockObj;
		*(void**)(self + kMw[i].objOff) = obj;
		unsigned char selfC[0x40]; memcpy(selfC, self, sizeof(self));
		unsigned char objC[0x800]; memcpy(objC, obj, sizeof(objC));
		// re-point the candidate's fixture at its OWN object buffer
		*(void**)(selfC + kMw[i].objOff) = objC;
		unsigned o[5];
		gRepCount = 0; memset(gRepCap, 0, sizeof(gRepCap));
		fn(self, 0x0BADF00Du);
		memcpy(o, gRepCap, sizeof(o));
		unsigned nO = gRepCount;
		gRepCount = 0; memset(gRepCap, 0, sizeof(gRepCap));
		unsigned wargs[3] = { 0x0BADF00Du, 0x0BADF00Du, 0x0BADF00Du };
		nxMutexWorkEx(selfC, kMw[i].lockOff, kMw[i].objOff, wargs, 3u,
			kMw[i].workKind, kMw[i].storeOff, 1u, 2u, kMw[i].file, kMw[i].line, kMw[i].expr);
		unsigned c[5];
		memcpy(c, gRepCap, sizeof(c));
		unsigned nC = gRepCount;
		if(nO != nC || memcmp(o, c, sizeof(o)) != 0
			|| memcmp(obj, objC, sizeof(obj)) != 0 || nO != arm)
			{fprintf(stderr,"mutexwork %s arm=%u nO=%u nC=%u\n", kMw[i].name, arm, nO, nC);++mwf;}
		}
	nxSetLockOwner(0x2222u);
	nxUnbindLockApi(base, svMwL);
	nxSetAssertReport(nullptr);
	nxUnbindReportSlot(base, svMw);
	printf("mutexwork candidate failures=%u provisional=1\n", mwf);
	}
	// -- Wide work kinds of the same family: 000289, 000338, 003942, 003944,
	//    003714, 003744.
	{
	struct WwRow { unsigned rva; unsigned lockOff; unsigned objOff; int workKind;
		unsigned baseOff; unsigned nwords; unsigned nargs; unsigned file;
		unsigned line; unsigned expr; const char* name; };
	static const WwRow kWw[] = {
		{ 0x0c400, 0x0c, 0x24, 3, 0x520, 3,  1, 0x10105ba8, 0x05b, 0x10104760, "000289" },
		{ 0x0cae0, 0x0c, 0x24, 2, 0x52c, 3,  3, 0x10105ba8, 0x120, 0x10104760, "000338" },
		{ 0x8ef70, 0x0c, 0x14, 2, 0x044, 5,  5, 0x1011796c, 0x022, 0x10104760, "003942" },
		{ 0x8efe0, 0x0c, 0x14, 2, 0x058, 4,  4, 0x1011796c, 0x029, 0x10104760, "003944" },
		{ 0x8b560, 0x0c, 0x14, 3, 0x028, 11, 1, 0x101160cc, 0x064, 0x10104760, "003714" },
		{ 0x8bd90, 0x0c, 0x14, 4, 0x058, 1,  2, 0x101160cc, 0x0dd, 0x10104760, "003744" },
	};
	NxReportSaved svWw = nxBindReportSlot(base);
	nxSetAssertReport(&nxReportRecorder);
	NxLockApiSaved svWwL = nxBindLockApi(base);
	nxSetLockOwner(0x2222u);
	unsigned wwf = 0;
	for(unsigned i = 0; i < sizeof(kWw) / sizeof(kWw[0]); ++i)
	for(unsigned arm = 0; arm < 2; ++arm)
		{
		typedef void (__thiscall* WwOracle)(void*);
		WwOracle fn = reinterpret_cast<WwOracle>(base + kWw[i].rva);
		unsigned char lockObj[0x40]; memset(lockObj, 0, sizeof(lockObj));
		unsigned char subObj[0x40]; memset(subObj, 0, sizeof(subObj));
		*(void**)(lockObj) = subObj;
		unsigned owner = (arm == 0) ? 0x2222u : 0x1111u;
		memcpy(subObj + 0x1c, &owner, 4);
		unsigned char obj[0x800]; memset(obj, 0, sizeof(obj));
		unsigned char self[0x40]; memset(self, 0, sizeof(self));
		*(void**)(self + kWw[i].lockOff) = lockObj;
		*(void**)(self + kWw[i].objOff) = obj;
		unsigned char selfC[0x40]; memcpy(selfC, self, sizeof(self));
		unsigned char objC[0x800]; memcpy(objC, obj, sizeof(objC));
		*(void**)(selfC + kWw[i].objOff) = objC;
		// the row is driven with its own arity; workKind 3 needs a pointer source
		unsigned char src[0x80]; memset(src, 0, sizeof(src));
		for(unsigned k = 0; k < 0x20; ++k) *(unsigned*)(src + 4 * k) = 0xC0DE0000u + k;
		unsigned args[5] = { 0, 0, 0, 0, 0 };
		if(kWw[i].workKind == 3)
			args[0] = static_cast<unsigned>(reinterpret_cast<size_t>(src));
		else
			for(unsigned k = 0; k < kWw[i].nargs && k < 5; ++k)
				args[k] = 0x11110000u + k;
		if(kWw[i].workKind == 4) args[1] = (arm == 0) ? 1u : 0u;
		unsigned o[5];
		gRepCount = 0; memset(gRepCap, 0, sizeof(gRepCap));
		switch(kWw[i].nargs)
			{
			case 1: reinterpret_cast<void (__thiscall*)(void*, unsigned)>(base + kWw[i].rva)(self, args[0]); break;
			case 2: reinterpret_cast<void (__thiscall*)(void*, unsigned, unsigned)>(base + kWw[i].rva)(self, args[0], args[1]); break;
			case 3: reinterpret_cast<void (__thiscall*)(void*, unsigned, unsigned, unsigned)>(base + kWw[i].rva)(self, args[0], args[1], args[2]); break;
			case 4: reinterpret_cast<void (__thiscall*)(void*, unsigned, unsigned, unsigned, unsigned)>(base + kWw[i].rva)(self, args[0], args[1], args[2], args[3]); break;
			default: reinterpret_cast<void (__thiscall*)(void*, unsigned, unsigned, unsigned, unsigned, unsigned)>(base + kWw[i].rva)(self, args[0], args[1], args[2], args[3], args[4]); break;
			}
		memcpy(o, gRepCap, sizeof(o));
		unsigned nO = gRepCount;
		(void) fn;
		gRepCount = 0; memset(gRepCap, 0, sizeof(gRepCap));
		nxMutexWorkEx(selfC, kWw[i].lockOff, kWw[i].objOff, args, kWw[i].nargs,
			kWw[i].workKind, kWw[i].baseOff, kWw[i].nwords, 2u, kWw[i].file,
			kWw[i].line, kWw[i].expr);
		unsigned c[5];
		memcpy(c, gRepCap, sizeof(c));
		unsigned nC = gRepCount;
		if(nO != nC || memcmp(o, c, sizeof(o)) != 0
			|| memcmp(obj, objC, sizeof(obj)) != 0 || nO != arm)
			{
			unsigned k = 0;
			while(k < sizeof(obj) && obj[k] == objC[k]) ++k;
			fprintf(stderr,"mutexwide %s arm=%u nO=%u nC=%u firstdiff=%03x\n",
				kWw[i].name, arm, nO, nC, k);
			++wwf;
			}
		}
	nxSetLockOwner(0x2222u);
	nxUnbindLockApi(base, svWwL);
	nxSetAssertReport(nullptr);
	nxUnbindReportSlot(base, svWw);
	printf("mutexwide candidate failures=%u provisional=1\n", wwf);
	}





		printf("layout candidate mismatches=%u mode=differential candidate_fold=%08x\n",
			candidateMissing, candidateFold);
		return nxFail("the Phase 5 reconstruction is incomplete; this gate is RED on purpose");
		}
	printf("layout candidate mismatches=0 mode=self\n");
	printf("layout result=self-pass\n");
	return 0;
	}
