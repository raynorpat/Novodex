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

typedef void (__thiscall* NxBoxCtorFn)(void* self, void* owner, unsigned argument);

// Runs the box ctor under SEH so a fault inside the oracle code reports its
// address instead of killing the transcript. No unwindable locals here.
static unsigned gFaultCode;
static unsigned gFaultAddr;
static void nxGuardedBoxCtor(NxBoxCtorFn fn, void* object, void* owner,
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
	typedef void (__thiscall* NxBoxCtorFn)(void* self, void* owner, unsigned argument);
	NxBoxCtorFn boxCtor = (NxBoxCtorFn) (base + 0x00021870);
	static const unsigned kPointerWords[] = { 0x00, 0x9c, 0xa4, 0xa8, 0xb0, 0xb4, 0xe0 };
	unsigned char object[0x228];
	memset(object, 0xcd, sizeof(object));
	const unsigned kArg2 = 0x5a5a5a5au;

	// Allocator shim. The ctor's collision-object arm allocates through the
	// SDK allocator singleton ([.data 0x101041bc] -> holder -> interface ->
	// vtable slot +8), and the shipped holder word is NULL until an
	// NxPhysicsSDK exists (phase4-formats.md). Creating one is Task-4/SDK
	// machinery this gate does not own, so the probe installs a minimal
	// interface whose slot +8 hands back a fixed block. Every instruction of
	// the rows under test -- phys_fn_000977 and phys_fn_001075 -- still runs
	// natively; only the 28 bytes come from the shim.
	static unsigned char gShimBlock[0x20];
	typedef void* (__fastcall* NxAllocFn)(void* ecx, void* edx, unsigned size, unsigned flag);
	static NxAllocFn gShimTable[4];
	struct NxShim { static void* __fastcall alloc(void*, void*, unsigned size, unsigned)
		{ return size <= sizeof(gShimBlock) ? gShimBlock : 0; } };
	gShimTable[2] = reinterpret_cast<NxAllocFn>(&NxShim::alloc);
	static void* gShimIface[1] = { gShimTable };
	unsigned holder = 0;
	memcpy(&holder, base + 0x001041bc, 4);
	if(!holder)
		return nxFail("the allocator holder word moved; re-pin the probe");
	void* shimIface = gShimIface;
	memcpy((void*) holder, &shimIface, 4);

	unsigned faultAddr = 0, faultCode = 0;
	nxGuardedBoxCtor(boxCtor, object, 0, kArg2, faultCode, faultAddr);
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

	printf("layout coverage tables=%u colobj=1 owner=1 hull=1 shapebase=1 boxshape=1\n",
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
