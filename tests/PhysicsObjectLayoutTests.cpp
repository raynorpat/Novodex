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
	if(off >= sizeof(g_arena)) return;
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
	static NxAllocFn sTable[6];
	struct NxShim
		{
		// Adapter vtable layout: +0x00/+0x0c/+0x14 are free variants (one
		// pushed pointer), +0x08 is malloc (two pushed args: size, flags).
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
	if(argc == 4 && wcscmp(argv[3], L"--self") == 0)
		selfOnly = true;
	else if(argc != 3)
		{
		fprintf(stderr, "usage: NxPhysicsObjectLayoutTests <oracle directory> <NxPhysics.dll sha256> [--self]\n");
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

	unsigned oracleDigest = 2166136261u;
	unsigned candidateMissing = 0;
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
	// BOX slot 4, phys_fn_000849: the compute-mass row over three
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

	printf("layout coverage tables=%u colobj=1 owner=1 hull=1 shapebase=1 boxshape=1 sphere=1 capsule=1 plane=1 mesh=1 basevt=3 basesave=1 boxrow=6 planesave=1 sphererows=4 capsave=1 meshword=1 aabbrows=3 meshrows=2 sphlocal=1 setrad=1 capsetrad=1 planeext=1 sphdtor=1 capdtor=1 setgroup=1 dtors2=2 sphload=1\n",
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

		printf("layout candidate mismatches=%u mode=differential candidate_fold=%08x\n",
			candidateMissing, candidateFold);
		return nxFail("the Phase 5 reconstruction is incomplete; this gate is RED on purpose");
		}
	printf("layout candidate mismatches=0 mode=self\n");
	printf("layout result=self-pass\n");
	return 0;
	}
