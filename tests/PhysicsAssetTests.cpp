// The Phase 4 asset-format gate, and why it is RED.
//
// Phase 4 owns the mesh column's data: triangle meshes, their OPCODE
// acceleration trees, penetration maps, the convex hull builder and the byte
// formats all four are serialised in. None of it is reconstructed yet. This
// harness exists so that the formats recovered in Phase 4 Task 1 are DRIVEN
// against the shipped DLL rather than described, and so the reconstruction
// Task 2 writes has something that already fails.
//
// It is an oracle differential of the same kind as NxPhysicsCollisionTests: it
// loads the pinned NxPhysics.dll, re-hashes it, and calls recorded internal
// addresses directly, because none of what it drives is exported and the public
// path that would reach it -- an SDK, a mesh factory -- belongs to phases 2, 4
// and 5 and is not wired up for meshes. The reconstruction side is linked in;
// today it answers `unreconstructed` and every case is a mismatch.
//
// Three properties, and the third is the one that matters.
//
//   * The ORACLE side is checked against values recorded in
//     docs/reconstruction/novodex-physics/cases/assets/*.json. That check is a
//     real one: change a byte of a fixture and the oracle's own answer moves
//     and this harness fails, with no reconstruction involved at all.
//   * The CANDIDATE side is checked against the oracle side. That is what is
//     RED now.
//   * `--self` runs the oracle side alone. It is how "GREEN against the oracle"
//     is demonstrated without the tautology of comparing the oracle with
//     itself: the comparison is against the recorded expectations, and the run
//     prints the oracle-side digests that gate_targets.ps1 registers.
//
// A harness that fails because its target does not exist is not a RED gate, it
// is an absent one. This one builds, runs, calls the oracle 21 times, prints
// every oracle answer, and then reports the reconstruction missing.
//
// FIXTURES CARRY NO ORACLE PROCESS POINTERS. Every byte below is either a
// constant of the format or a value this file computes; nothing is copied out
// of the oracle's address space, so the fixtures mean the same thing in any
// process.

#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

// ---------------------------------------------------------------------------
// The recovered addresses. Every one is an inventory row this phase owns; the
// evidence for what each does is evidence/phase4-formats.md.

static const unsigned kPMapCtorRva      = 0x000505f0;	// phys_fn_002045, returns this
static const unsigned kPMapDtorRva      = 0x0004cae0;	// phys_fn_001984
static const unsigned kPMapCreateRva    = 0x00050640;	// phys_fn_002047, PenetrationMap::Create
static const unsigned kStreamCtorRva    = 0x000b3ce0;	// phys_fn_004788, (size, buffer)
static const unsigned kStreamSeekRva    = 0x000b3b30;	// phys_fn_004780
static const unsigned kStreamDtorRva    = 0x000b3db0;	// phys_fn_004791
static const unsigned kMeshHeaderRva    = 0x00055cb0;	// phys_fn_002262, the NxStream mesh loader
static const unsigned kReleasePMapRva   = 0x00051040;	// phys_fn_002051, NxReleasePMap

// sizeof, from the allocation sites rather than from a guess:
//   0x78 -- `push 0x78` at 0x00053d35 in TriangleMesh::loadPMap
//   0x1c -- `push 0x1c` at 0x00050184 in phys_fn_002035
static const unsigned kPMapObjectSize   = 0x78;
static const unsigned kStreamObjectSize = 0x1c;

// Offsets inside PenetrationMap, each established by the store that writes it.
static const unsigned kPMapResolution   = 0x5c;	// stored at 0x0004ff99
static const unsigned kPMapCellCount    = 0x6c;	// res*res*res, stored at 0x000500ca
static const unsigned kPMapGrid         = 0x70;	// res*res*res dwords, stored at 0x000500ef

typedef void* (__thiscall* NxPMapCtorFn)(void* self);
typedef void (__thiscall* NxPMapDtorFn)(void* self);
typedef char (__thiscall* NxPMapCreateFn)(void* self, const void* mesh, unsigned resolution,
	const char* filename, void* stream, int load, void* outputStream);
typedef void* (__thiscall* NxStreamCtorFn)(void* self, unsigned size, const void* buffer);
typedef void* (__thiscall* NxStreamSeekFn)(void* self, unsigned offset);
typedef void (__thiscall* NxStreamDtorFn)(void* self);
typedef char (__thiscall* NxMeshHeaderFn)(void* self, void* stream);
typedef unsigned char (__cdecl* NxReleasePMapFn)(void* pmap);

// ---------------------------------------------------------------------------
// The fixtures.
//
// Each is a byte string plus what the oracle did with it. The `expect` fields
// were recorded from the run whose transcript is
// cases/assets/oracle-transcript.txt and are checked here, so this table and
// that transcript cannot drift apart in silence.

struct NxPMapFixture
	{
	const char* name;
	const char* dimension;
	unsigned resolution;		// what the fixture's header declares
	const char* bytes;			// lower-case hex
	// recorded oracle answers
	unsigned expectAccepted;
	unsigned expectErrors;
	unsigned expectErrorLine;	// 0 when no error was reported
	unsigned expectCells;
	unsigned expectGrid;		// FNV-1a over the decoded grid, 0 when no grid
	};

static const NxPMapFixture nxPMapFixtures[] =
	{
	{ "pmap.minimal_valid",           "minimal_valid", 1,
	  "504d415004000000010000007fffffffc0",                                       1, 0, 0, 1, 0xe3160fb1 },
	{ "pmap.multi_value",             "multi_element", 2,
	  "504d415004000000020000000000000000000000400000001fffffffffe0",             1, 0, 0, 8, 0x5b517625 },
	{ "pmap.boundary_res4",           "boundary", 4,
	  "504d415004000000040000007fffffffaaaaaaaaaaaaaaaa80",                       1, 0, 0, 64, 0x06a34ac5 },
	{ "pmap.boundary_res1_signclear", "boundary", 1,
	  "504d415004000000010000007fffffff80",                                       1, 0, 0, 1, 0xe3160fb1 },
	{ "pmap.bad_magic_0",             "malformed", 1,
	  "584d415004000000010000007fffffffc0",                                       0, 1, 0x3d3, 0, 0 },
	{ "pmap.bad_magic_1",             "malformed", 1,
	  "5058415004000000010000007fffffffc0",                                       0, 1, 0x3d3, 0, 0 },
	{ "pmap.bad_magic_2",             "malformed", 1,
	  "504d585004000000010000007fffffffc0",                                       0, 1, 0x3d3, 0, 0 },
	{ "pmap.bad_magic_3",             "malformed", 1,
	  "504d415804000000010000007fffffffc0",                                       0, 1, 0x3d3, 0, 0 },
	{ "pmap.bad_version_00000000",    "malformed", 1,
	  "504d415000000000010000007fffffffc0",                                       0, 1, 0x3da, 0, 0 },
	{ "pmap.bad_version_00000003",    "malformed", 1,
	  "504d415003000000010000007fffffffc0",                                       0, 1, 0x3da, 0, 0 },
	{ "pmap.bad_version_00000005",    "malformed", 1,
	  "504d415005000000010000007fffffffc0",                                       0, 1, 0x3da, 0, 0 },
	{ "pmap.bad_version_ffffffff",    "malformed", 1,
	  "504d4150ffffffff010000007fffffffc0",                                       0, 1, 0x3da, 0, 0 },
	{ "pmap.truncated_after_magic",   "truncated", 0,
	  "504d4150000000000000000000000000000000000000000000000000000000000000000000000000"
	  "0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
	                                                                              0, 1, 0x3da, 0, 0 },
	{ "pmap.truncated_mid_tag",       "truncated", 0,
	  "504d0000000000000000000000000000000000000000000000000000000000000000000000000000"
	  "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000",
	                                                                              0, 1, 0x3d3, 0, 0 }
	};

static const unsigned kPMapFixtureCount = sizeof(nxPMapFixtures) / sizeof(nxPMapFixtures[0]);

struct NxMeshFixture
	{
	const char* name;
	const char* dimension;
	const char* bytes;
	unsigned expectAccepted;
	unsigned expectDwordsRead;
	};

static const NxMeshFixture nxMeshFixtures[] =
	{
	{ "mesh.bad_tag0",             "malformed", "5353584e4853454d", 0, 1 },
	{ "mesh.bad_tag0_zero",        "malformed", "000000004853454d", 0, 1 },
	{ "mesh.bad_tag0_byteswapped", "malformed", "4e5853544853454d", 0, 1 },
	{ "mesh.bad_tag1",             "malformed", "5453584e4753454d", 0, 2 },
	{ "mesh.bad_tag1_zero",        "malformed", "5453584e00000000", 0, 2 },
	{ "mesh.truncated_after_tag0", "truncated", "5453584e",         0, 2 }
	};

static const unsigned kMeshFixtureCount = sizeof(nxMeshFixtures) / sizeof(nxMeshFixtures[0]);

// ---------------------------------------------------------------------------
// Results, and the digest that folds them.

struct NxPMapResult
	{
	unsigned accepted;
	unsigned errors;
	unsigned errorLine;
	unsigned cells;
	unsigned grid;			// FNV-1a over the decoded grid
	unsigned resolution;	// what the object ended up holding
	};

struct NxMeshResult
	{
	unsigned accepted;
	unsigned dwordsRead;
	};

// FNV-1a, 32 bit. Small on purpose: what is being pinned is that the oracle
// produced these words for these bytes, and a wider digest would say the same.
static unsigned nxFold(unsigned digest, unsigned word)
	{
	digest ^= word & 0xffu;              digest *= 16777619u;
	digest ^= (word >> 8) & 0xffu;       digest *= 16777619u;
	digest ^= (word >> 16) & 0xffu;      digest *= 16777619u;
	digest ^= (word >> 24) & 0xffu;      digest *= 16777619u;
	return digest;
	}

static unsigned nxHexNibble(char c)
	{
	if(c >= '0' && c <= '9')
		return (unsigned) (c - '0');
	if(c >= 'a' && c <= 'f')
		return (unsigned) (c - 'a') + 10u;
	return 0xffffffffu;
	}

// Decodes hex into `out`, which the caller sized with padding. Returns the
// number of bytes decoded, or 0 on a malformed string.
static unsigned nxDecodeHex(const char* text, unsigned char* out, unsigned capacity)
	{
	unsigned length = (unsigned) strlen(text);
	if((length & 1) != 0 || length / 2 > capacity)
		return 0;
	for(unsigned i = 0; i < length / 2; ++i)
		{
		unsigned high = nxHexNibble(text[i * 2]);
		unsigned low = nxHexNibble(text[i * 2 + 1]);
		if(high > 15 || low > 15)
			return 0;
		out[i] = (unsigned char) ((high << 4) | low);
		}
	return length / 2;
	}

// ---------------------------------------------------------------------------
// The two callbacks the oracle makes into us.

// NxUserOutputStream, whose slot 0 is
// reportError(NxErrorCode, const char* message, const char* file, int line).
// Hand-rolled rather than derived, because the harness has to count the calls
// and read the line number the oracle passes.

struct NxErrorSink
	{
	const void** vtable;
	unsigned calls;
	unsigned lastCode;
	unsigned lastLine;
	const char* lastMessage;
	};

static void __fastcall nxSinkReportError(NxErrorSink* self, void*, unsigned code,
	const char* message, const char* file, int line)
	{
	(void) file;
	++self->calls;
	self->lastCode = code;
	self->lastLine = (unsigned) line;
	self->lastMessage = message;
	}

static unsigned __fastcall nxSinkReportAssert(NxErrorSink*, void*, const char*, const char*, int)
	{
	return 0;
	}

static void __fastcall nxSinkPrint(NxErrorSink*, void*, const char*)
	{
	}

static const void* nxSinkVtable[3] =
	{
	(const void*) &nxSinkReportError,
	(const void*) &nxSinkReportAssert,
	(const void*) &nxSinkPrint
	};

static void nxResetSink(NxErrorSink* sink)
	{
	sink->vtable = nxSinkVtable;
	sink->calls = 0;
	sink->lastCode = 0;
	sink->lastLine = 0;
	sink->lastMessage = 0;
	}

// NxStream, from the pinned Foundation header. The slot offsets are not a
// guess: phys_fn_002262 calls +0x0c for every dword it reads, +0x10 for every
// float and +0x18 for every buffer, which is readDword, readFloat and
// readBuffer in declaration order after the virtual destructor.

struct NxHarnessStream
	{
	const void** vtable;
	const unsigned char* bytes;
	unsigned size;
	unsigned offset;
	unsigned dwordsRead;
	};

static unsigned nxStreamTake(NxHarnessStream* self, unsigned count)
	{
	unsigned value = 0;
	for(unsigned i = 0; i < count; ++i)
		{
		unsigned byte = self->offset < self->size ? self->bytes[self->offset] : 0u;
		value |= byte << (i * 8);
		++self->offset;
		}
	return value;
	}

static void __fastcall nxStreamDestruct(NxHarnessStream*, void*, int) { }
static unsigned char __fastcall nxStreamReadByte(NxHarnessStream* self, void*)
	{ return (unsigned char) nxStreamTake(self, 1); }
static unsigned short __fastcall nxStreamReadWord(NxHarnessStream* self, void*)
	{ return (unsigned short) nxStreamTake(self, 2); }
static unsigned __fastcall nxStreamReadDword(NxHarnessStream* self, void*)
	{ ++self->dwordsRead; return nxStreamTake(self, 4); }
static float __fastcall nxStreamReadFloat(NxHarnessStream* self, void*)
	{ unsigned bits = nxStreamTake(self, 4); float value; memcpy(&value, &bits, 4); return value; }
static double __fastcall nxStreamReadDouble(NxHarnessStream* self, void*)
	{ nxStreamTake(self, 4); nxStreamTake(self, 4); return 0.0; }
static void __fastcall nxStreamReadBuffer(NxHarnessStream* self, void*, void* buffer, unsigned size)
	{ memset(buffer, 0, size); self->offset += size; }
static void* __fastcall nxStreamStore(NxHarnessStream* self, void*, unsigned) { return self; }
static void* __fastcall nxStreamStoreBuffer(NxHarnessStream* self, void*, const void*, unsigned) { return self; }

static const void* nxHarnessStreamVtable[13] =
	{
	(const void*) &nxStreamDestruct,
	(const void*) &nxStreamReadByte,
	(const void*) &nxStreamReadWord,
	(const void*) &nxStreamReadDword,
	(const void*) &nxStreamReadFloat,
	(const void*) &nxStreamReadDouble,
	(const void*) &nxStreamReadBuffer,
	(const void*) &nxStreamStore,
	(const void*) &nxStreamStore,
	(const void*) &nxStreamStore,
	(const void*) &nxStreamStore,
	(const void*) &nxStreamStore,
	(const void*) &nxStreamStoreBuffer
	};

// ---------------------------------------------------------------------------
// The oracle side.

struct NxOracle
	{
	unsigned char* base;
	NxPMapCtorFn pmapCtor;
	NxPMapDtorFn pmapDtor;
	NxPMapCreateFn pmapCreate;
	NxStreamCtorFn streamCtor;
	NxStreamSeekFn streamSeek;
	NxStreamDtorFn streamDtor;
	NxMeshHeaderFn meshHeader;
	NxReleasePMapFn releasePMap;
	};

// A mesh stand-in. On the LOAD path -- `load != 0` -- PenetrationMap::Create
// reads six floats at +0x44 and stores the pointer at this+0x74, and touches
// nothing else: the first dereference is `lea ecx,[edi+0x44]` at 0x0005072d and
// the vertex and triangle arrays are only reached on the COMPUTE path. So the
// load fixtures need no triangle mesh, which is why they can be driven in this
// task at all.
struct NxMeshStandIn
	{
	unsigned char head[0x44];
	float bounds[6];
	unsigned char tail[0x20];
	};

static void nxRunPMapOracle(const NxOracle* oracle, const unsigned char* storage, unsigned length,
	NxPMapResult* result)
	{
	NxMeshStandIn mesh;
	memset(&mesh, 0, sizeof(mesh));
	mesh.bounds[0] = -1.0f; mesh.bounds[1] = -1.0f; mesh.bounds[2] = -1.0f;
	mesh.bounds[3] =  1.0f; mesh.bounds[4] =  1.0f; mesh.bounds[5] =  1.0f;

	NxErrorSink sink;
	nxResetSink(&sink);

	unsigned char stream[kStreamObjectSize];
	memset(stream, 0, sizeof(stream));
	oracle->streamCtor(stream, length, storage);
	oracle->streamSeek(stream, 0);

	unsigned char object[kPMapObjectSize];
	memset(object, 0, sizeof(object));
	oracle->pmapCtor(object);

	char accepted = oracle->pmapCreate(object, &mesh, 0, 0, stream, 1, &sink);

	result->accepted = accepted ? 1u : 0u;
	result->errors = sink.calls;
	result->errorLine = sink.lastLine;
	result->resolution = *(unsigned*) (object + kPMapResolution);
	result->cells = *(unsigned*) (object + kPMapCellCount);
	result->grid = 0;
	unsigned* grid = *(unsigned**) (object + kPMapGrid);
	if(accepted && grid)
		{
		unsigned digest = 2166136261u;
		for(unsigned i = 0; i < result->cells; ++i)
			digest = nxFold(digest, grid[i]);
		result->grid = digest;
		}

	oracle->pmapDtor(object);
	oracle->streamDtor(stream);
	}

static void nxRunMeshOracle(const NxOracle* oracle, const unsigned char* storage, unsigned length,
	NxMeshResult* result)
	{
	NxHarnessStream stream;
	stream.vtable = nxHarnessStreamVtable;
	stream.bytes = storage;
	stream.size = length;
	stream.offset = 0;
	stream.dwordsRead = 0;

	// The reject paths never touch `this`: the first store into it is
	// `fstp dword ptr [edi+0x6c]` at 0x00055cfa, which is past both tag tests.
	unsigned char object[0x100];
	memset(object, 0, sizeof(object));

	char accepted = oracle->meshHeader(object, &stream);
	result->accepted = accepted ? 1u : 0u;
	result->dwordsRead = stream.dwordsRead;
	}

// ---------------------------------------------------------------------------
// The candidate side.
//
// Phase 4 Task 2 replaces these three with calls into Physics/src. Until then
// they report that the row is not reconstructed, and every case is a mismatch.
//
// THEY TAKE FIXTURE BYTES AND NOTHING ELSE, and that is the part that has to
// survive Task 2. The first version handed them `const NxPMapFixture*` and
// `const NxMeshFixture*`, and those structs carry expectAccepted, expectErrors,
// expectErrorLine, expectCells, expectGrid and expectDwordsRead -- the oracle's
// own recorded answers. A reconstruction that copied them out would agree with
// the oracle on every case and turn this gate green without decoding a single
// byte of either format. A candidate that is handed only the input it is meant
// to parse cannot do that.
//
// For the same reason the release probe below hands the candidate its own
// NxPMap with the same seeded fields the oracle was given, and its own output
// variable, rather than the one the oracle already wrote its answer into.

static bool nxCandidatePMapLoad(const unsigned char*, unsigned, NxPMapResult*)
	{
	return false;
	}

static bool nxCandidateMeshHeader(const unsigned char*, unsigned, NxMeshResult*)
	{
	return false;
	}

static bool nxCandidateReleasePMap(void*, unsigned char*)
	{
	return false;
	}

// ---------------------------------------------------------------------------

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
		fprintf(stderr, "usage: NxPhysicsAssetTests <oracle directory> <NxPhysics.dll sha256> [--self]\n");
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

	printf("oracle module path=%S sha256=%s\n", loadedPath, loadedHash);
	printf("oracle base=%p mode=%s\n", (void*) physics, selfOnly ? "self" : "differential");
	if(strcmp(loadedHash, expected) != 0)
		{
		fprintf(stderr, "FAIL loaded oracle is not the pinned one: expected %s\n", expected);
		return 1;
		}
	printf("oracle pin=matched\n");

	NxOracle oracle;
	oracle.base = (unsigned char*) physics;
	oracle.pmapCtor = (NxPMapCtorFn) (oracle.base + kPMapCtorRva);
	oracle.pmapDtor = (NxPMapDtorFn) (oracle.base + kPMapDtorRva);
	oracle.pmapCreate = (NxPMapCreateFn) (oracle.base + kPMapCreateRva);
	oracle.streamCtor = (NxStreamCtorFn) (oracle.base + kStreamCtorRva);
	oracle.streamSeek = (NxStreamSeekFn) (oracle.base + kStreamSeekRva);
	oracle.streamDtor = (NxStreamDtorFn) (oracle.base + kStreamDtorRva);
	oracle.meshHeader = (NxMeshHeaderFn) (oracle.base + kMeshHeaderRva);
	oracle.releasePMap = (NxReleasePMapFn) GetProcAddress(physics, "NxReleasePMap");
	if(!oracle.releasePMap)
		return nxFail("the pinned oracle does not export NxReleasePMap");
	if((unsigned char*) oracle.releasePMap - oracle.base != kReleasePMapRva)
		return nxFail("NxReleasePMap is not at the censused RVA");

	printf("asset fixtures pmap=%u mesh=%u release=1\n", kPMapFixtureCount, kMeshFixtureCount);
	printf("asset rows pmap_create=phys_fn_002047 pmap_load=phys_fn_002035 "
		"mesh_header=phys_fn_002262 release_pmap=phys_fn_002051\n");

	unsigned expectMismatch = 0;
	unsigned candidateMismatch = 0;
	unsigned oracleDigest = 2166136261u;
	unsigned drivenAccepted = 0;
	unsigned drivenRejected = 0;
	unsigned drivenErrors = 0;

	// -----------------------------------------------------------------------
	// The penetration-map byte format.
	for(unsigned i = 0; i < kPMapFixtureCount; ++i)
		{
		const NxPMapFixture* fixture = &nxPMapFixtures[i];
		unsigned char storage[256];
		memset(storage, 0, sizeof(storage));
		unsigned length = nxDecodeHex(fixture->bytes, storage, sizeof(storage));

		NxPMapResult actual;
		memset(&actual, 0, sizeof(actual));
		nxRunPMapOracle(&oracle, storage, length, &actual);

		oracleDigest = nxFold(oracleDigest, actual.accepted);
		oracleDigest = nxFold(oracleDigest, actual.errors);
		oracleDigest = nxFold(oracleDigest, actual.errorLine);
		oracleDigest = nxFold(oracleDigest, actual.cells);
		oracleDigest = nxFold(oracleDigest, actual.grid);
		if(actual.accepted)
			++drivenAccepted;
		else
			++drivenRejected;
		drivenErrors += actual.errors;

		printf("pmap case=%s dimension=%s bytes=%u accepted=%u errors=%u line=0x%03x "
			"resolution=%u cells=%u grid=%08x\n",
			fixture->name, fixture->dimension, (unsigned) (strlen(fixture->bytes) / 2),
			actual.accepted, actual.errors, actual.errorLine,
			actual.resolution, actual.cells, actual.grid);

		if(actual.accepted != fixture->expectAccepted
			|| actual.errors != fixture->expectErrors
			|| actual.errorLine != fixture->expectErrorLine
			|| actual.cells != fixture->expectCells
			|| actual.grid != fixture->expectGrid)
			{
			++expectMismatch;
			printf("pmap ORACLE-MISMATCH case=%s recorded accepted=%u errors=%u line=0x%03x cells=%u grid=%08x\n",
				fixture->name, fixture->expectAccepted, fixture->expectErrors,
				fixture->expectErrorLine, fixture->expectCells, fixture->expectGrid);
			}

		if(!selfOnly)
			{
			NxPMapResult candidate;
			memset(&candidate, 0, sizeof(candidate));
			if(!nxCandidatePMapLoad(storage, length, &candidate))
				{
				++candidateMismatch;
				printf("pmap CANDIDATE-MISSING case=%s: no reconstruction of phys_fn_002047\n",
					fixture->name);
				}
			else if(memcmp(&candidate, &actual, sizeof(candidate)) != 0)
				{
				// Every field the memcmp above compares is printed. errorLine and
				// resolution were compared and not printed, so a run that differed
				// only in one of them printed a line whose every field was
				// identical on both sides and said nothing about why it failed.
				++candidateMismatch;
				printf("pmap CANDIDATE-MISMATCH case=%s accepted=%u/%u errors=%u/%u line=0x%03x/0x%03x "
					"cells=%u/%u grid=%08x/%08x resolution=%u/%u\n",
					fixture->name, candidate.accepted, actual.accepted,
					candidate.errors, actual.errors, candidate.errorLine, actual.errorLine,
					candidate.cells, actual.cells, candidate.grid, actual.grid,
					candidate.resolution, actual.resolution);
				}
			}
		}

	// -----------------------------------------------------------------------
	// The triangle-mesh stream header.
	//
	// Only the reject arms are driven here. The accept arm at 0x00055ce1
	// allocates the vertex array through the Foundation SDK allocator that
	// 0x101041bc holds, which is null until an NxPhysicsSDK exists, so driving
	// it needs the mesh factory Task 2 reconstructs. That is recorded as a hole
	// rather than approximated.
	for(unsigned i = 0; i < kMeshFixtureCount; ++i)
		{
		const NxMeshFixture* fixture = &nxMeshFixtures[i];
		unsigned char storage[64];
		memset(storage, 0, sizeof(storage));
		unsigned length = nxDecodeHex(fixture->bytes, storage, sizeof(storage));

		NxMeshResult actual;
		memset(&actual, 0, sizeof(actual));
		nxRunMeshOracle(&oracle, storage, length, &actual);

		oracleDigest = nxFold(oracleDigest, actual.accepted);
		oracleDigest = nxFold(oracleDigest, actual.dwordsRead);
		if(actual.accepted)
			++drivenAccepted;
		else
			++drivenRejected;

		printf("mesh case=%s dimension=%s accepted=%u dwords_read=%u\n",
			fixture->name, fixture->dimension, actual.accepted, actual.dwordsRead);

		if(actual.accepted != fixture->expectAccepted || actual.dwordsRead != fixture->expectDwordsRead)
			{
			++expectMismatch;
			printf("mesh ORACLE-MISMATCH case=%s recorded accepted=%u dwords_read=%u\n",
				fixture->name, fixture->expectAccepted, fixture->expectDwordsRead);
			}

		if(!selfOnly)
			{
			NxMeshResult candidate;
			memset(&candidate, 0, sizeof(candidate));
			if(!nxCandidateMeshHeader(storage, length, &candidate))
				{
				++candidateMismatch;
				printf("mesh CANDIDATE-MISSING case=%s: no reconstruction of phys_fn_002262\n",
					fixture->name);
				}
			else if(memcmp(&candidate, &actual, sizeof(candidate)) != 0)
				{
				++candidateMismatch;
				printf("mesh CANDIDATE-MISMATCH case=%s accepted=%u/%u dwords_read=%u/%u\n",
					fixture->name, candidate.accepted, actual.accepted,
					candidate.dwordsRead, actual.dwordsRead);
				}
			}
		}

	// -----------------------------------------------------------------------
	// NxReleasePMap on a null buffer.
	//
	// The null arm is the whole of what can be driven without an NxCreatePMap,
	// and it is not nothing: the export returns TRUE for a PMap it did not
	// release and leaves dataSize alone, which a reimplementation that reported
	// failure or zeroed the size would get wrong.
	{
	unsigned char pmap[8];
	memset(pmap, 0, sizeof(pmap));
	*(unsigned*) pmap = 0x5a5a5a5au;		// dataSize, deliberately non-zero
	*(void**) (pmap + 4) = 0;				// data
	unsigned char returned = oracle.releasePMap(pmap);
	unsigned size = *(unsigned*) pmap;
	unsigned data = *(unsigned*) (pmap + 4);
	oracleDigest = nxFold(oracleDigest, returned);
	oracleDigest = nxFold(oracleDigest, size);
	oracleDigest = nxFold(oracleDigest, data);
	printf("release case=null_data returned=%u data_size=%08x data=%08x\n", returned, size, data);
	if(returned != 1 || size != 0x5a5a5a5au || data != 0)
		{
		++expectMismatch;
		printf("release ORACLE-MISMATCH case=null_data recorded returned=1 data_size=5a5a5a5a data=00000000\n");
		}
	if(!selfOnly)
		{
		// Its own NxPMap seeded exactly the way the oracle's was, and its own
		// output variable: handing it `&returned`, which already held the
		// oracle's answer, made a candidate that wrote nothing agree, and the
		// answer it did write was never compared with the oracle's at all.
		unsigned char candidatePMap[8];
		memset(candidatePMap, 0, sizeof(candidatePMap));
		*(unsigned*) candidatePMap = 0x5a5a5a5au;
		*(void**) (candidatePMap + 4) = 0;
		unsigned char candidateReturned = 0;
		if(!nxCandidateReleasePMap(candidatePMap, &candidateReturned))
			{
			++candidateMismatch;
			printf("release CANDIDATE-MISSING case=null_data: no reconstruction of phys_fn_002051\n");
			}
		else if(candidateReturned != returned
			|| *(unsigned*) candidatePMap != size
			|| *(unsigned*) (candidatePMap + 4) != data)
			{
			++candidateMismatch;
			printf("release CANDIDATE-MISMATCH case=null_data returned=%u/%u data_size=%08x/%08x data=%08x/%08x\n",
				candidateReturned, returned,
				*(unsigned*) candidatePMap, size,
				*(unsigned*) (candidatePMap + 4), data);
			}
		}
	}

	printf("asset coverage driven=%u accepted=%u rejected=%u errors=%u\n",
		kPMapFixtureCount + kMeshFixtureCount + 1, drivenAccepted, drivenRejected, drivenErrors);
	printf("asset oracle digest=%08x expect_mismatches=%u\n", oracleDigest, expectMismatch);
	printf("asset candidate mismatches=%u mode=%s\n",
		candidateMismatch, selfOnly ? "self" : "differential");

	if(expectMismatch)
		return nxFail("the oracle did not answer what the recorded cases say it answered");
	if(candidateMismatch)
		return nxFail("the reconstruction does not agree with the pinned oracle");
	printf("asset result=pass\n");
	return 0;
	}
