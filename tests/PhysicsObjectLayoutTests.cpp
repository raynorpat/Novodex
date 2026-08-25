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
