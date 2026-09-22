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
// is an absent one. This one builds, runs, calls the oracle 30 times -- fourteen
// penetration-map fixtures, six mesh-header rejects, nine mesh-writer
// serialisations and one release probe -- prints every oracle answer, and then
// reports any part of the reconstruction that disagrees or is missing.
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

// The reconstruction under test.
#include "PMap.h"
#include "TriangleMesh.h"
#include "NxStream.h"
#include "NxUserOutputStream.h"
class NxTriangleMesh;
#include "NxPMap.h"

// The oracle-RVA to candidate-address table, generated from the census's recorded symbols joined to
// the rebuilt module's linker map. It is what lets this harness drive a module that is not the pinned
// one: the census records where the ORACLE put each row and the map records where the rebuild did.
#include "oracle_rva_to_candidate.h"
#include "NxUserAllocator.h"

// The SDK defines this variable, and this harness compiles the SDK's own sources -- so it provides the
// definition rather than importing it. Importing it is what made the loader resolve NxFoundation.dll
// before any code here ran, from the ordinary search path rather than the pair directory, so the PAIR's
// copy was never the one loaded (round 75). The dependency is exactly one variable, reached through
// NX_ALLOC in ReadWriteLock's constructor (round 76).
//
// The allocator behind it is the CRT heap: nothing on this harness's paths needs the Foundation's own,
// and nxGetSdkAllocator already falls back to a local allocator of its own.
namespace
	{
	struct NxAssetAllocator : public NxUserAllocator
		{
		// The four pure virtuals, and the two that carry default bodies are left alone. Named by
		// C2259 on the first attempt, then read from the header.
		void* malloc(size_t size) override
			{ return ::malloc(size); }
		void* malloc(size_t size, NxMemoryType type) override
			{ (void)type; return ::malloc(size); }
		void* mallocDEBUG(size_t size, const char* fileName, int line) override
			{ (void)fileName; (void)line; return ::malloc(size); }
		void* realloc(void* memory, size_t size) override
			{ return ::realloc(memory, size); }
		void free(void* memory) override
			{ ::free(memory); }
		};

	NxAssetAllocator gAssetAllocator;
	}

// The definition the Foundation would otherwise provide.
NxUserAllocator* nxFoundationSDKAllocator = &gAssetAllocator;

// ---------------------------------------------------------------------------
// The recovered addresses. Every one is an inventory row this phase owns; the
// evidence for what each does is evidence/phase4-formats.md.

static const unsigned kPMapCtorRva      = 0x000505f0;	// phys_fn_002045, returns this
static const unsigned kPMapDtorRva      = 0x0004cae0;	// phys_fn_001984
static const unsigned kPMapCreateRva    = 0x00050640;	// phys_fn_002047, PenetrationMap::Create
static const unsigned kStreamCtorRva    = 0x000b3ce0;	// phys_fn_004788, (size, buffer)
static const unsigned kStreamSeekRva    = 0x000b3b30;	// phys_fn_004780
static const unsigned kStreamDtorRva    = 0x000b3db0;	// phys_fn_004791, jmp to phys_fn_004784
static const unsigned kMeshHeaderRva    = 0x00055cb0;	// phys_fn_002262, the NxStream mesh loader
static const unsigned kReleasePMapRva   = 0x00051040;	// phys_fn_002051, NxReleasePMap

// The mesh writer and its growable stream's store half. The writer builds its
// own stream through kStreamCtorRva/kStreamDtorRva; these are what stand in
// for BaseModel slot 5 on the oracle side.
static const unsigned kMeshWriterRva    = 0x000539d0;	// phys_fn_002162, TriangleMesh slot 18
static const unsigned kStoreDwordRva    = 0x000b3f00;	// phys_fn_004797, the row Model::Save calls

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
typedef char (__thiscall* NxMeshWriterFn)(void* self, void* stream);
typedef void* (__thiscall* NxStoreDwordFn)(void* self, unsigned value);

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

// The writer fixtures. These are NOT recorded oracle answers -- they are
// inputs, the same kind of thing the pmap hex strings are: a mesh description
// handed identically to both sides, each of which serialises it through its
// own module. What the oracle's answers get folded into is the per-case digest
// over everything its writer stored, and that is what the candidate must
// reproduce byte for byte.
//
// The optional-buffer flags are exercised every way singly as well as all
// together, because the writer has four separate conditional arms and a
// combined-only fixture would let two wrong conditions cancel.
struct NxWriterFixture
	{
	const char* name;
	const char* dimension;
	unsigned vertexCount;
	unsigned triangleCount;
	unsigned hullModeBit;		// TriangleMesh+0x40 bit 0 -> flags bit 3
	int hasMaterials;			// internal+0x10 non-null -> flags bit 0
	int hasRemap;				// internal+0x14 non-null -> flags bit 1
	int hasArrayA;				// +0x94 non-null
	int hasArrayB;				// +0x98 non-null
	int hasHull;				// +0xa0 non-null -> flags bit 2
	float threshold;			// +0x6c, storeFloat
	unsigned axis;				// +0x7c, storeDword
	float extent;				// +0x80, storeFloat
	unsigned flagA;				// +0x8c, storeDword unconditional
	unsigned flagB;				// +0x90, storeDword unconditional
	};

static const NxWriterFixture nxWriterFixtures[] =
	{
	{ "writer.full",          "everything",   7, 9, 1, 1, 1, 1, 1, 1,   0.001f, 0xffu, 0.0f,    0x11223344u, 0x55667788u },
	{ "writer.minimal",       "nothing_optional", 3, 1, 0, 0, 0, 0, 0, 0,  0.001f, 0xffu, 0.0f,    0, 0 },
	{ "writer.empty_mesh",    "zero_counts",  0, 0, 0, 0, 0, 0, 0, 0,      0.001f, 0xffu, 0.0f,    0, 0 },
	{ "writer.materials_only","single_flag",  2, 3, 0, 1, 0, 0, 0, 0,      0.0025f, 2u, -1.5f,     1, 0 },
	{ "writer.remap_only",    "single_flag",  2, 3, 0, 0, 1, 0, 0, 0,      0.0025f, 2u, -1.5f,     0, 1 },
	{ "writer.array_a_only",  "single_flag",  2, 3, 0, 0, 0, 1, 0, 0,      0.0025f, 2u, -1.5f,     0, 0 },
	{ "writer.array_b_only",  "single_flag",  2, 3, 0, 0, 0, 0, 1, 0,      0.0025f, 2u, -1.5f,     0, 0 },
	{ "writer.hull_present",  "hull",         5, 6, 0, 0, 0, 0, 0, 1,      0.01f, 1u, 2.5f,          7, 7 },
	{ "writer.hull_mode_bit", "hull",         5, 6, 1, 0, 0, 0, 0, 0,      0.01f, 1u, 2.5f,          0, 0 }
	};

static const unsigned kWriterFixtureCount = sizeof(nxWriterFixtures) / sizeof(nxWriterFixtures[0]);

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

// Everything one writer run stored, in order. Both sides record through their
// own module's stream implementation and the comparison is element-wise, so a
// difference is reported at the event that first differs rather than only as
// an opaque digest. The digest is FNV-1a over the whole log and is what the
// oracle-side registration pins.
struct NxWriteLog
	{
	static const unsigned kMaxEvents = 256;
	static const unsigned kMaxBytes = 8192;
	unsigned kind[kMaxEvents];		// 1 dword, 2 float, 3 buffer
	unsigned value[kMaxEvents];		// the dword / the float's bits / the byte count
	unsigned bytes[kMaxBytes];		// buffer contents, one byte per word
	unsigned count;
	unsigned byteCount;

	void reset() { count = 0; byteCount = 0; }
	bool addEvent(unsigned eventKind, unsigned eventValue)
		{
		if(count >= kMaxEvents)
			return false;
		kind[count] = eventKind;
		value[count] = eventValue;
		++count;
		return true;
		}
	bool addByte(unsigned byte)
		{
		if(byteCount >= kMaxBytes)
			return false;
		bytes[byteCount++] = byte & 0xffu;
		return true;
		}
	};

static unsigned nxFold(unsigned digest, unsigned word);

static unsigned nxLogFold(const NxWriteLog& log)
	{
	unsigned digest = 2166136261u;
	unsigned offset = 0;
	for(unsigned i = 0; i < log.count; ++i)
		{
		digest = nxFold(digest, log.kind[i]);
		digest = nxFold(digest, log.value[i]);
		if(log.kind[i] == 3)
			{
			for(unsigned b = 0; b < log.value[i]; ++b)
				digest = nxFold(digest, log.bytes[offset + b]);
			offset += log.value[i];
			}
		}
	return digest;
	}

static unsigned nxLogFirstDifference(const NxWriteLog& candidate, const NxWriteLog& oracle)
	{
	unsigned n = candidate.count < oracle.count ? candidate.count : oracle.count;
	for(unsigned i = 0; i < n; ++i)
		{
		if(candidate.kind[i] != oracle.kind[i] || candidate.value[i] != oracle.value[i])
			return i;
		if(oracle.kind[i] == 3)
			for(unsigned b = 0; b < oracle.value[i]; ++b)
				if(candidate.bytes[b] != oracle.bytes[b])
					return i;
		}
	if(candidate.count != oracle.count)
		return n;
	return 0xffffffffu;
	}

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
// readBuffer in declaration order after the virtual destructor. The writer
// calls +0x24, +0x28 and +0x30 -- storeDword, storeFloat and storeBuffer, the
// slots five past their read partners.

struct NxHarnessStream
	{
	const void** vtable;
	const unsigned char* bytes;
	unsigned size;
	unsigned offset;
	unsigned dwordsRead;
	NxWriteLog* log;		// non-null when the writer is being recorded
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
static void* __fastcall nxStreamStoreByte(NxHarnessStream* self, void*, unsigned char value)
	{
	if(self->log) { self->log->addEvent(1, value); }
	return self;
	}
static void* __fastcall nxStreamStoreWord(NxHarnessStream* self, void*, unsigned short value)
	{
	if(self->log) { self->log->addEvent(1, value); }
	return self;
	}
static void* __fastcall nxStreamStoreDword(NxHarnessStream* self, void*, unsigned value)
	{
	if(self->log) { self->log->addEvent(1, value); }
	return self;
	}
static void* __fastcall nxStreamStoreFloat(NxHarnessStream* self, void*, float value)
	{
	unsigned bits;
	memcpy(&bits, &value, 4);
	if(self->log) { self->log->addEvent(2, bits); }
	return self;
	}
static void* __fastcall nxStreamStoreDouble(NxHarnessStream* self, void*, double value)
	{
	unsigned bits[2];
	memcpy(bits, &value, 8);
	if(self->log) { self->log->addEvent(2, bits[0]); self->log->addEvent(2, bits[1]); }
	return self;
	}
static void* __fastcall nxStreamStoreBuffer(NxHarnessStream* self, void*, const void* buffer, unsigned size)
	{
	if(self->log)
		{
		self->log->addEvent(3, size);
		const unsigned char* p = (const unsigned char*) buffer;
		for(unsigned i = 0; i < size; ++i)
			self->log->addByte(p[i]);
		}
	return self;
	}

static const void* nxHarnessStreamVtable[13] =
	{
	(const void*) &nxStreamDestruct,
	(const void*) &nxStreamReadByte,
	(const void*) &nxStreamReadWord,
	(const void*) &nxStreamReadDword,
	(const void*) &nxStreamReadFloat,
	(const void*) &nxStreamReadDouble,
	(const void*) &nxStreamReadBuffer,
	(const void*) &nxStreamStoreByte,
	(const void*) &nxStreamStoreWord,
	(const void*) &nxStreamStoreDword,
	(const void*) &nxStreamStoreFloat,
	(const void*) &nxStreamStoreDouble,
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
	NxMeshWriterFn meshWriter;
	NxStoreDwordFn storeDword;
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

// A line written straight to the OS, with no CRT buffer to lose. The instrument this sequence has used
// everywhere else prints through printf and fflush, and round 88 found the canary's line absent in a way
// that is consistent with the flush itself faulting once the stack is broken. This cannot be broken that
// way: CreateFileW and WriteFile take the bytes to the file system.
static void nxRawLine(const wchar_t* path, const char* text)
	{
	// OPEN_ALWAYS with FILE_APPEND_DATA, not CREATE_ALWAYS: the harness writes a line per case, and
	// CREATE_ALWAYS truncated the file on every iteration -- so the case that faulted deleted what the
	// cases before it had written and left an empty file that said nothing. The instrument destroyed its
	// own evidence, which is the failure this sequence has recorded seven times now.
	HANDLE file = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ, 0, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, 0);
	if(file == INVALID_HANDLE_VALUE)
		return;
	DWORD written = 0;
	WriteFile(file, text, (DWORD) strlen(text), &written, 0);
	CloseHandle(file);
	}

static void nxRunPMapOracle(const NxOracle* oracle, const unsigned char* storage, unsigned length,
	NxPMapResult* result)
	{
	NxMeshStandIn mesh;
	memset(&mesh, 0, sizeof(mesh));
	mesh.bounds[0] = -1.0f; mesh.bounds[1] = -1.0f; mesh.bounds[2] = -1.0f;
	mesh.bounds[3] =  1.0f; mesh.bounds[4] =  1.0f; mesh.bounds[5] =  1.0f;

	NxErrorSink sink;
	nxResetSink(&sink);

	// The stream is the other stack object the case owns, and the only one never canaried. `object` was
	// checked in 17v and is clean; this is the same instrument in the same place.
	unsigned char streamGuard[16];
	memset(streamGuard, 0xA5, sizeof(streamGuard));
	unsigned char stream[kStreamObjectSize];
	memset(stream, 0, sizeof(stream));
	unsigned char streamCanary[16];
	memset(streamCanary, 0x5A, sizeof(streamCanary));
	oracle->streamCtor(stream, length, storage);
	printf("  step streamCtor done\n"); fflush(stdout);
	{
	unsigned gb = 0, ga = 0;
	for(unsigned i = 0; i < sizeof(streamGuard); ++i)
		if(streamGuard[i] != 0xA5) ++gb;
	for(unsigned i = 0; i < sizeof(streamCanary); ++i)
		if(streamCanary[i] != 0x5A) ++ga;
	printf("  streamCanary before=%u after=%u sizeof=%u\n", gb, ga, (unsigned) sizeof(stream));
	fflush(stdout);
	}
	oracle->streamSeek(stream, 0);
	printf("  step streamSeek done\n"); fflush(stdout);

	// The object sits in a fixed stack buffer of kPMapObjectSize, and the module writes it through the
	// two calls below. A canary either side says whether the module wrote past the buffer, which is the
	// question round 88 narrowed to and the one a canary answers without a debugger.
	unsigned char objectGuard[16];
	memset(objectGuard, 0xA5, sizeof(objectGuard));
	unsigned char object[kPMapObjectSize];
	memset(object, 0, sizeof(object));
	unsigned char objectCanary[16];
	memset(objectCanary, 0x5A, sizeof(objectCanary));
	// One line before the calls and one after, so an absent file, a one-line file and a two-line file are
	// three different readings.

	oracle->pmapCtor(object);
	printf("  step pmapCtor done\n"); fflush(stdout);

	char accepted = oracle->pmapCreate(object, &mesh, 0, 0, stream, 1, &sink);
	printf("  step pmapCreate done\n"); fflush(stdout);

	{
	unsigned before = 0, after = 0;
	char line[128];
	for(unsigned i = 0; i < sizeof(objectGuard); ++i)
		if(objectGuard[i] != 0xA5) ++before;
	for(unsigned i = 0; i < sizeof(objectCanary); ++i)
		if(objectCanary[i] != 0x5A) ++after;
	_snprintf_s(line, sizeof(line), _TRUNCATE,
		"canary case=i before=%u after=%u sizeof=%u accepted=%d\n",
		before, after, (unsigned) sizeof(object), (int) accepted);
	// Straight to the OS, so the report survives a broken CRT buffer.
	nxRawLine(L"canary.txt", line);
	printf("  canary before=%u after=%u sizeof(object)=%u\n",
		before, after, (unsigned) sizeof(object));
	fflush(stdout);
	}

	result->accepted = accepted ? 1u : 0u;
	result->errors = sink.calls;
	result->errorLine = sink.lastLine;
	result->resolution = *(unsigned*) (object + kPMapResolution);
	result->cells = *(unsigned*) (object + kPMapCellCount);
	result->grid = 0;
	unsigned* grid = *(unsigned**) (object + kPMapGrid);
	// What the CANDIDATE left in the object, printed before the digest loop reads it. The loop folds
	// `result->cells` entries of `grid`, so a count larger than the allocation reads past it -- and
	// these four values are what distinguishes that from a bad free in the destructors below.
	printf("  object accepted=%d resolution=%u cells=%u grid=%p\n",
		(int) accepted, result->resolution, result->cells, (void*) grid);
	fflush(stdout);
	if(accepted && grid)
		{
		unsigned digest = 2166136261u;
		for(unsigned i = 0; i < result->cells; ++i)
			digest = nxFold(digest, grid[i]);
		result->grid = digest;
		}

	oracle->pmapDtor(object);
	printf("  step pmapDtor done\n"); fflush(stdout);
	oracle->streamDtor(stream);
	printf("  step streamDtor done\n"); fflush(stdout);
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
// The mesh writer, oracle side.
//
// The writer is driven exactly the way the real caller would drive it: a
// TriangleMesh-shaped byte object at the measured offsets, and an NxStream.
// What stands in for the OPCODE model at +0x28 is a fake object whose slot 5
// -- BaseModel::Save, established by the Model vtable at .rdata:0x0011badc --
// writes four deterministic dwords through phys_fn_004797, the row the real
// Model::Save calls at 0x000e944e. The stand-in is INPUT, not reconstruction:
// both sides see the same four words, so the comparison pins everything the
// writer itself does around that call -- field order, sizes, conditional arms,
// the length store and the collapsed buffer -- without depending on the tree
// serialiser, which is unmapped census work.

// The four words the model save writes, derived from the fixture. Both sides
// derive them from the same fields, so any drift in the derivation shows up
// as an identical movement on both sides and cancels in the comparison.
static unsigned nxBlobWords[4];

static void nxSeedBlobWords(const NxWriterFixture* fixture)
	{
	unsigned digest = 2166136261u;
	for(const char* p = fixture->name; *p; ++p)
		digest = nxFold(digest, (unsigned char) *p);
	digest = nxFold(digest, fixture->vertexCount);
	digest = nxFold(digest, fixture->triangleCount);
	for(unsigned i = 0; i < 4; ++i)
		{
		nxBlobWords[i] = digest;
		digest = digest * 16777619u + 1u;
		}
	}

// The fake BaseModel. Seven slots: 0/1 the destructor pair, 2 GetUsedBytes,
// 3 Refit, 4 NovodeXSlot4, 5 Save, 6 Load. Only slot 5 is ever dispatched,
// because the only entry into this object is the writer's `call [edx+0x14]`.
struct NxFakeModel
	{
	const void** vtable;
	};

static char __fastcall nxFakeModelUnused(void*, void*) { return 0; }

// Set once per run to the oracle's phys_fn_004797. A static is the honest
// carrier here: the oracle process has exactly one of this row.
static NxStoreDwordFn nxOracleStoreDword = 0;

static char __fastcall nxFakeModelSave(NxFakeModel*, void*, void* growableStream)
	{
	for(unsigned i = 0; i < 4; ++i)
		nxOracleStoreDword(growableStream, nxBlobWords[i]);
	return 1;
	}

static const void* nxFakeModelVtable[7] =
	{
	(const void*) &nxFakeModelUnused,
	(const void*) &nxFakeModelUnused,
	(const void*) &nxFakeModelUnused,
	(const void*) &nxFakeModelUnused,
	(const void*) &nxFakeModelUnused,
	(const void*) &nxFakeModelSave,
	(const void*) &nxFakeModelUnused
	};

// The fixture's arrays, one allocation per run, deterministic contents. The
// face remap and the two presence arrays are three distinct buffers even
// though they have the same shape, so no two conditional arms can agree by
// accident of sharing.
struct NxWriterBuffers
	{
	unsigned vertices[16 * 3];		// NxVec3 triads
	unsigned triangles[16 * 3];		// three 32-bit indices per triangle
	unsigned short materials[16];
	unsigned faceRemap[16];
	unsigned arrayA[16];
	unsigned arrayB[16];
	};

static void nxFillWriterBuffers(const NxWriterFixture* fixture, NxWriterBuffers* buffers)
	{
	for(unsigned i = 0; i < fixture->vertexCount * 3; ++i)
		buffers->vertices[i] = 0x01020304u + i * 0x1010101u;
	for(unsigned i = 0; i < fixture->triangleCount * 3; ++i)
		buffers->triangles[i] = (i / 3) * 0x300u + (i % 3);
	for(unsigned i = 0; i < fixture->triangleCount; ++i)
		{
		buffers->materials[i] = (unsigned short) (0x1000 + i);
		buffers->faceRemap[i] = 0xf0000000u + i;
		buffers->arrayA[i] = 0xa0000000u + i;
		buffers->arrayB[i] = 0xb0000000u + i;
		}
	}

// Builds the byte object at the measured offsets and drives the oracle writer
// against the recording stream. The optional pointers are null exactly when
// their fixture flag says so -- that is what moves the four conditional arms.
static bool nxRunMeshWriterOracle(const NxOracle* oracle, const NxWriterFixture* fixture,
	NxWriteLog* log, unsigned* accepted)
	{
	NxWriterBuffers buffers;
	memset(&buffers, 0, sizeof(buffers));
	nxFillWriterBuffers(fixture, &buffers);

	NxFakeModel model;
	model.vtable = nxFakeModelVtable;

	// Offsets per TriangleMesh.h; every store here mirrors an address in the
	// class comment.
	unsigned char object[0xc0];
	memset(object, 0, sizeof(object));
	*(unsigned*) (object + 0x08) = fixture->vertexCount;
	*(unsigned*) (object + 0x0c) = fixture->triangleCount;
	*(void**) (object + 0x10) = buffers.vertices;
	*(void**) (object + 0x14) = buffers.triangles;
	*(void**) (object + 0x18) = fixture->hasMaterials ? (void*) buffers.materials : (void*) 0;
	*(void**) (object + 0x1c) = fixture->hasRemap ? (void*) buffers.faceRemap : (void*) 0;
	*(unsigned*) (object + 0x40) = fixture->hullModeBit;
	*(float*) (object + 0x6c) = fixture->threshold;
	*(unsigned*) (object + 0x7c) = fixture->axis;
	*(float*) (object + 0x80) = fixture->extent;
	*(unsigned*) (object + 0x8c) = fixture->flagA;
	*(unsigned*) (object + 0x90) = fixture->flagB;
	*(void**) (object + 0x94) = fixture->hasArrayA ? (void*) buffers.arrayA : (void*) 0;
	*(void**) (object + 0x98) = fixture->hasArrayB ? (void*) buffers.arrayB : (void*) 0;
	*(void**) (object + 0xa0) = fixture->hasHull ? (void*) &buffers : (void*) 0;
	*(void**) (object + 0x28) = &model;

	NxHarnessStream stream;
	memset(&stream, 0, sizeof(stream));
	stream.vtable = nxHarnessStreamVtable;
	stream.log = log;

	char ok = oracle->meshWriter(object, &stream);
	*accepted = ok ? 1u : 0u;
	return true;
	}


// ---------------------------------------------------------------------------
// The candidate side.
//
// Phase 4 Task 3 fills these three in from Physics/src.
//
// THEY TAKE FIXTURE BYTES AND NOTHING ELSE, and that is the part that had to
// survive. The first version handed them `const NxPMapFixture*` and
// `const NxMeshFixture*`, and those structs carry expectAccepted, expectErrors,
// expectErrorLine, expectCells, expectGrid and expectDwordsRead -- the oracle's
// own recorded answers. A reconstruction that copied them out would agree with
// the oracle on every case and turn this gate green without decoding a single
// byte of either format. A candidate that is handed only the input it is meant
// to parse cannot do that, and these three signatures are unchanged.
//
// For the same reason the release probe below hands the candidate its own
// NxPMap with the same seeded fields the oracle was given, and its own output
// variable, rather than the one the oracle already wrote its answer into.
//
// Everything the candidate needs beyond those bytes it builds for itself: its
// own error sink, its own mesh stand-in with the same bounds the oracle side
// seeds, and its own stream. None of the three ever sees the oracle's process.

// The candidate's error sink, on the PUBLIC NxUserOutputStream, which is what
// PenetrationMap::Create reports through. It counts and remembers the same two
// fields the oracle side reads out of its own hand-rolled sink.
struct NxCandidateSink : public NxUserOutputStream
	{
	unsigned calls;
	unsigned lastCode;
	unsigned lastLine;

	NxCandidateSink() : calls(0), lastCode(0), lastLine(0) { }

	virtual void reportError(NxErrorCode code, const char*, const char*, int line)
		{
		++calls;
		lastCode = (unsigned) code;
		lastLine = (unsigned) line;
		}
	virtual NxAssertResponse reportAssertViolation(const char*, const char*, int)
		{
		return NX_AR_CONTINUE;
		}
	virtual void print(const char*) { }
	};

// The candidate's NxStream over the fixture bytes, counting dwords the same way
// the oracle side's does. It is a real NxStream subclass rather than the
// hand-rolled __fastcall table above, because the reconstruction takes an
// NxStream& and the oracle takes a vtable at a recorded offset; the two sides
// reach the same bytes by their own module's calling convention.
struct NxCandidateStream : public NxStream
	{
	const unsigned char* bytes;
	unsigned size;
	mutable unsigned offset;
	mutable unsigned dwordsRead;
	NxWriteLog* log;

	NxCandidateStream(const unsigned char* b, unsigned n)
		: bytes(b), size(n), offset(0), dwordsRead(0), log(0) { }

	unsigned take(unsigned count) const
		{
		unsigned value = 0;
		for(unsigned i = 0; i < count; ++i)
			{
			unsigned byte = offset < size ? bytes[offset] : 0u;
			value |= byte << (i * 8);
			++offset;
			}
		return value;
		}

	virtual NxU8 readByte() const					{ return (NxU8) take(1); }
	virtual NxU16 readWord() const					{ return (NxU16) take(2); }
	virtual NxU32 readDword() const					{ ++dwordsRead; return take(4); }
	virtual NxF32 readFloat() const					{ unsigned b = take(4); NxF32 v; memcpy(&v, &b, 4); return v; }
	virtual NxF64 readDouble() const				{ take(4); take(4); return 0.0; }
	virtual void readBuffer(void* buffer, NxU32 n) const { memset(buffer, 0, n); offset += n; }

	virtual NxStream& storeByte(NxU8 v)				{ if(log) { log->addEvent(1, v); } return *this; }
	virtual NxStream& storeWord(NxU16 v)			{ if(log) { log->addEvent(1, v); } return *this; }
	virtual NxStream& storeDword(NxU32 v)			{ if(log) { log->addEvent(1, v); } return *this; }
	virtual NxStream& storeFloat(NxF32 v)
		{
		if(log) { unsigned bits; memcpy(&bits, &v, 4); log->addEvent(2, bits); }
		return *this;
		}
	virtual NxStream& storeDouble(NxF64 v)
		{
		if(log) { unsigned bits[2]; memcpy(bits, &v, 8); log->addEvent(2, bits[0]); log->addEvent(2, bits[1]); }
		return *this;
		}
	virtual NxStream& storeBuffer(const void* buffer, NxU32 n)
		{
		if(log)
			{
			log->addEvent(3, n);
			const unsigned char* p = (const unsigned char*) buffer;
			for(NxU32 i = 0; i < n; ++i)
				log->addByte(p[i]);
			}
		return *this;
		}
	};

static bool nxCandidatePMapLoad(const unsigned char* storage, unsigned length, NxPMapResult* result)
	{
	// The same six floats the oracle side seeds, in the candidate's own object.
	// They reach only the AABB fields at PenetrationMap+0x08..+0x58, none of
	// which the harness compares -- what is compared is acceptance, the error
	// count, the reported line, the cell count, the decoded grid and the stored
	// resolution.
	unsigned char mesh[0x64];
	memset(mesh, 0, sizeof(mesh));
	float bounds[6] = { -1.0f, -1.0f, -1.0f, 1.0f, 1.0f, 1.0f };
	memcpy(mesh + 0x44, bounds, sizeof(bounds));

	NxCandidateSink sink;
	// A mark BEFORE the constructor, so its absence-or-presence says whether this function was entered
	// at all. Round 85 inferred the constructor from a mark placed after it, which is sound but does not
	// distinguish "entered and faulted here" from "never entered".
	printf("  cand entered length=%u\n", length); fflush(stdout);
	// phys_fn_004788 builds the stream FULL over given bytes (the block starts
	// at offset = size), so the rewind is part of the construction sequence,
	// exactly as it is on the oracle side above.
	MemoryStream stream(length, storage);
	printf("  cand streamCtor done\n"); fflush(stdout);
	stream.seek(0);
	printf("  cand streamSeek done\n"); fflush(stdout);
	PenetrationMap pmap;
	printf("  cand pmapCtor done\n"); fflush(stdout);

	bool accepted = pmap.create(mesh, 0, 0, &stream, true, &sink);
	printf("  cand pmapCreate done accepted=%d\n", (int) accepted); fflush(stdout);

	printf("  cand state resolution=%u cells=%u grid=%p\n",
		pmap.getResolution(), pmap.getCellCount(), (void*) pmap.getGrid());
	fflush(stdout);

	result->accepted = accepted ? 1u : 0u;
	result->errors = sink.calls;
	result->errorLine = sink.lastLine;
	result->resolution = pmap.getResolution();
	result->cells = pmap.getCellCount();
	result->grid = 0;
	if(accepted && pmap.getGrid())
		{
		unsigned digest = 2166136261u;
		for(unsigned i = 0; i < result->cells; ++i)
			digest = nxFold(digest, pmap.getGrid()[i]);
		result->grid = digest;
		}
	printf("  cand digest done\n"); fflush(stdout);
	return true;
	}

static bool nxCandidateMeshHeader(const unsigned char* storage, unsigned length, NxMeshResult* result)
	{
	NxCandidateStream stream(storage, length);
	NxTriangleMeshHeader header = nxTriangleMeshReadHeader(stream);
	if(header != NX_TRIANGLE_MESH_HEADER_REJECTED)
		return false;		// fields 3-19 are not reconstructed; see TriangleMesh.h

	result->accepted = 0;
	result->dwordsRead = stream.dwordsRead;
	return true;
	}

static bool nxCandidateReleasePMap(void* pmap, unsigned char* returned)
	{
	*returned = NxReleasePMap(*(NxPMap*) pmap) ? 1u : 0u;
	return true;
	}

// ---------------------------------------------------------------------------
// The candidate's fake model. It derives from the vendored BaseModel -- whose
// seven-slot vtable shape NxPhysicsThirdPartyTests asserts against the image --
// and overrides Save with the same four-dword stand-in the oracle side uses,
// written through the reconstruction's own MemoryStream store row.

struct NxCandidateFakeModel : public Opcode::BaseModel
	{
	virtual bool Build(const Opcode::OPCODECREATE&) { return false; }
	virtual udword GetUsedBytes() const { return 0; }
	virtual bool Save(void* stream)
		{
		MemoryStream* growable = static_cast<MemoryStream*>(stream);
		for(unsigned i = 0; i < 4; ++i)
			growable->storeDword(nxBlobWords[i]);
		return true;
		}
	};

static bool nxCandidateMeshWriter(const NxWriterFixture* fixture, NxWriteLog* log, unsigned* accepted)
	{
	NxWriterBuffers buffers;
	memset(&buffers, 0, sizeof(buffers));
	nxFillWriterBuffers(fixture, &buffers);

	NxCandidateFakeModel model;

	TriangleMesh mesh;
	memset(&mesh, 0, sizeof(mesh));
	mesh.mInternal.mVertexCount = fixture->vertexCount;
	mesh.mInternal.mTriangleCount = fixture->triangleCount;
	mesh.mInternal.mVertices = buffers.vertices;
	mesh.mInternal.mTriangles = buffers.triangles;
	mesh.mInternal.mMaterialIndices = fixture->hasMaterials ? buffers.materials : 0;
	mesh.mInternal.mFaceRemap = fixture->hasRemap ? buffers.faceRemap : 0;
	mesh.mHullFlags = fixture->hullModeBit;
	mesh.mConvexEdgeThreshold = fixture->threshold;
	mesh.mHeightFieldVerticalAxis = fixture->axis;
	mesh.mHeightFieldVerticalExtent = fixture->extent;
	mesh.mPresenceFlagA = fixture->flagA;
	mesh.mPresenceFlagB = fixture->flagB;
	mesh.mArrayA = fixture->hasArrayA ? buffers.arrayA : 0;
	mesh.mArrayB = fixture->hasArrayB ? buffers.arrayB : 0;
	mesh.mConvexMesh = fixture->hasHull ? (void*) &buffers : 0;
	mesh.mInternal.mModel = &model;

	NxCandidateStream stream(0, 0);
	stream.log = log;

	bool ok = mesh.save(stream);
	*accepted = ok ? 1u : 0u;
	return true;
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

// Where a target lives in the module that was loaded.
//
// In DIFFERENTIAL mode the loaded module IS the pinned one, so the censused RVA is its offset and the
// arithmetic is exact. In `--self` mode the module is whatever was given -- which is the whole point of
// the flag -- and the censused RVA is an offset into a DIFFERENT file, so adding it to this module's
// base lands wherever the arithmetic falls. Round 69 measured that landing: the fault address was
// exactly `base + kStreamCtorRva`, which is an address with no code at it.
//
// The table carries the rows whose symbol the census records AND whose candidate address the map
// knows. An RVA it does not carry keeps the old arithmetic, and that is reported once rather than
// silently, because a silent fallback is the fault this resolves.
static const unsigned char* nxOracleTarget(bool selfOnly, const unsigned char* base,
                                           unsigned rva, const char* what)
	{
	// The decision is the MODULE's, not the flag's. A module loaded at its preferred image base is
	// the one the census describes, so the censused RVA is its offset and the arithmetic is exact --
	// whether it was named as the "oracle" or handed over as a staged pair. A module the loader
	// relocated is a rebuild, and the table is what says where the rebuild put each row.
	//
	// Measuring this rather than trusting the flag is what round 74 found: the oracle pair directory
	// passes exactly the same argument shape as a staged-pair run, and applying the table to the
	// oracle put the CANDIDATE's offset into the ORACLE's image, which faulted at NxPhysics+0xa240.
	const bool atPreferredBase =
		(reinterpret_cast<unsigned>(base) == kNxRvaPreferredImageBase);
	if(!selfOnly || atPreferredBase)
		return base + rva;
	for(unsigned i = 0; i < kNxRvaTranslationCount; ++i)
		{
		if(kNxRvaTranslations[i].oracleRva == rva)
			{
			// The table's address is in the module's PREFERRED layout, because that is what the
			// linker map records. The loader relocates the module, so the address has to be rebased:
			// `actualBase + (preferredAddress - preferredBase)`. Returning it unchanged was the fault
			// this fixes -- `1000a240` is not in any loaded module when the module sits at 6dbf0000.
			const unsigned offset = kNxRvaTranslations[i].candidateAddress - kNxRvaPreferredImageBase;
			return base + offset;
			}
		}
	fprintf(stderr, "WARNING: no translation for %s at oracle rva 0x%08x; "
		"this target is not in the rebuilt module's map\n", what, rva);
	return base + rva;
	}

int wmain(int argc, wchar_t** argv)
	{
	// Two invocation conventions, and this harness is called both ways:
	//   * as an ORACLE differential, `run_phase_gate.ps1` passes the pinned directory AND its
	//     expected sha256;
	//   * as a STAGED-PAIR differential, `run_differential.ps1` passes the pair directory ALONE,
	//     because it has already verified the pair's identity itself.
	// So the hash is optional and the pin is checked only when one was given.
	// A single argument means a staged-pair run. `run_differential.ps1` invokes a target as
	// `<exe> <pair directory>` and nothing else -- no hash, because it verified the pair's identity
	// itself, and no `--self`, because driving the given module is what a staged-pair target IS. The
	// explicit `--self` is accepted as the same thing, which is how this harness has been tested by
	// hand.
	bool selfOnly = false;
	bool haveHash = false;
	if(argc == 3 && wcscmp(argv[2], L"--self") == 0)
		selfOnly = true;
	else if(argc == 4 && wcscmp(argv[3], L"--self") == 0)
		{
		selfOnly = true;
		haveHash = true;
		}
	else if(argc == 3)
		haveHash = true;
	else if(argc == 2)
		selfOnly = true;
	else
		{
		fprintf(stderr, "usage: NxPhysicsAssetTests <pair directory> "
			"[NxPhysics.dll sha256] [--self]\n");
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
	if(haveHash)
		{
		if(wcstombs_s(&converted, expected, sizeof(expected), argv[2], _TRUNCATE) != 0)
			return nxFail("cannot read the expected hash argument");
		}
	else
		expected[0] = '\0';

	printf("oracle module path=%S sha256=%s\n", loadedPath, loadedHash);
	// The staged-pair runner asserts this exact shape for each module it stages -- read from its own
	// `$expected` construction -- so a target it drives has to report the identity it was given. The
	// line above is this harness's own form and is kept for the oracle-differential transcript.
	printf("loaded module=NxPhysics.dll path=%S sha256=%s\n", loadedPath, loadedHash);

	// The Foundation comes in as NxPhysics's dependency, so it is loaded by now and its handle can be
	// asked for. The runner asserts the identity of every module it staged, and this harness had only
	// ever named the one it loads by name.
	{
	HMODULE foundation = GetModuleHandleW(L"NxFoundation.dll");
	if(foundation)
		{
		wchar_t foundationPath[MAX_PATH];
		char foundationHash[65];
		if(GetModuleFileNameW(foundation, foundationPath, MAX_PATH)
			&& nxSha256(foundationPath, foundationHash))
			printf("loaded module=NxFoundation.dll path=%S sha256=%s\n",
				foundationPath, foundationHash);
		}
	}
	printf("oracle base=%p mode=%s\n", (void*) physics, selfOnly ? "self" : "differential");
	// The pin is checked in DIFFERENTIAL mode only. `--self` means the harness drives whatever module
	// it was given and compares its own candidate-side calls against it, so the pin is not its
	// subject. Before this guard the check ran unconditionally and `--self` could not be used on any
	// file but the pinned one, which made the flag decorative -- the same defect the layout harness
	// had, found in round 37.
	if(!selfOnly && haveHash)
		{
		if(strcmp(loadedHash, expected) != 0)
			{
			fprintf(stderr, "FAIL loaded oracle is not the pinned one: expected %s\n", expected);
			return 1;
			}
		printf("oracle pin=matched\n");
		}

	NxOracle oracle;
	oracle.base = (unsigned char*) physics;
	// Every one of these is a row the census names, so each is a translation-table lookup rather
	// than an addition. `selfOnly` is what decides which the resolver does.
	oracle.pmapCtor = (NxPMapCtorFn) nxOracleTarget(selfOnly, oracle.base, kPMapCtorRva,
		"phys_fn_002045 PenetrationMap::PenetrationMap");
	oracle.pmapDtor = (NxPMapDtorFn) nxOracleTarget(selfOnly, oracle.base, kPMapDtorRva,
		"phys_fn_001984 PenetrationMap::~PenetrationMap");
	oracle.pmapCreate = (NxPMapCreateFn) nxOracleTarget(selfOnly, oracle.base, kPMapCreateRva,
		"phys_fn_002047 PenetrationMap::create");
	oracle.streamCtor = (NxStreamCtorFn) nxOracleTarget(selfOnly, oracle.base, kStreamCtorRva,
		"phys_fn_004788 MemoryStream::MemoryStream");
	oracle.streamSeek = (NxStreamSeekFn) nxOracleTarget(selfOnly, oracle.base, kStreamSeekRva,
		"phys_fn_004780 MemoryStream::seek");
	oracle.streamDtor = (NxStreamDtorFn) nxOracleTarget(selfOnly, oracle.base, kStreamDtorRva,
		"phys_fn_004791 MemoryStream::~MemoryStream");
	oracle.meshHeader = (NxMeshHeaderFn) nxOracleTarget(selfOnly, oracle.base, kMeshHeaderRva,
		"phys_fn_002262 the NxStream mesh loader");
	oracle.releasePMap = (NxReleasePMapFn) GetProcAddress(physics, "NxReleasePMap");
	if(!oracle.releasePMap)
		return nxFail("the pinned oracle does not export NxReleasePMap");
	// The censused RVA is a fact about the SHIPPED DLL, so comparing a loaded module against it is
	// only meaningful in differential mode, where the loaded module IS that file. `--self` drives
	// whatever module it was given -- which is what makes it useful on the rebuilt one -- and the
	// comparison there would be asking about a file that is not loaded. The export itself is resolved
	// by name either way, so nothing about which function is called changes.
	if(!selfOnly && (unsigned char*) oracle.releasePMap - oracle.base != kReleasePMapRva)
		return nxFail("NxReleasePMap is not at the censused RVA");
	oracle.meshWriter = (NxMeshWriterFn) nxOracleTarget(selfOnly, oracle.base, kMeshWriterRva,
		"phys_fn_002162 the TriangleMesh writer");
	nxOracleStoreDword = (NxStoreDwordFn) nxOracleTarget(selfOnly, oracle.base, kStoreDwordRva,
		"phys_fn_004797 MemoryStream::storeDword");

	// Every target, against zero, before any is called. Round 78's fault was a call to address zero and
	// round 79 retracted the reading that named WHICH pointer, because it came from a frame the debugger
	// had flagged as unreliable. This answers it directly: each target is tested here, by name, and the
	// harness reports the null ones itself rather than a debugger having to infer them.
	{
	struct { const char* name; const void* value; } targets[] = {
		{ "pmapCtor     phys_fn_002045 PenetrationMap::PenetrationMap", oracle.pmapCtor },
		{ "pmapDtor     phys_fn_001984 PenetrationMap::~PenetrationMap", oracle.pmapDtor },
		{ "pmapCreate   phys_fn_002047 PenetrationMap::create", oracle.pmapCreate },
		{ "streamCtor   phys_fn_004788 MemoryStream::MemoryStream", oracle.streamCtor },
		{ "streamSeek   phys_fn_004780 MemoryStream::seek", oracle.streamSeek },
		{ "streamDtor   phys_fn_004791 MemoryStream::~MemoryStream", oracle.streamDtor },
		{ "meshHeader   phys_fn_002262 the NxStream mesh loader", oracle.meshHeader },
		{ "meshWriter   phys_fn_002162 the TriangleMesh writer", oracle.meshWriter },
		{ "releasePMap  phys_fn_002051 NxReleasePMap", oracle.releasePMap },
		{ "storeDword   phys_fn_004797 MemoryStream::storeDword", nxOracleStoreDword },
	};
	unsigned nulls = 0;
	for(unsigned i = 0; i < sizeof(targets) / sizeof(targets[0]); ++i)
		{
		if(targets[i].value == 0)
			{
			printf("target NULL  %s\n", targets[i].name);
			++nulls;
			}
		}
	printf("targets bound=%u null=%u mode=%s\n",
		(unsigned) (sizeof(targets) / sizeof(targets[0])), nulls,
		selfOnly ? "self" : "differential");
	fflush(stdout);
	}

	printf("asset fixtures pmap=%u mesh=%u writer=%u release=1\n",
		kPMapFixtureCount, kMeshFixtureCount, kWriterFixtureCount);
	printf("asset rows pmap_create=phys_fn_002047 pmap_load=phys_fn_002035 "
		"mesh_header=phys_fn_002262 mesh_writer=phys_fn_002162 release_pmap=phys_fn_002051\n");

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

		// Named and appended, so the last line identifies the case that got furthest -- the instrument
		// that CREATE_ALWAYS destroyed, corrected.
		{
		char mark[160];
		_snprintf_s(mark, sizeof(mark), _TRUNCATE, "case=%s storage=%u about to call oracle\n",
			fixture->name, length);
		nxRawLine(L"canary-before.txt", mark);
		}

		NxPMapResult actual;
		memset(&actual, 0, sizeof(actual));
		nxRunPMapOracle(&oracle, storage, length, &actual);

		oracleDigest = nxFold(oracleDigest, actual.accepted);
		printf("  gap fold1 done\n"); fflush(stdout);
		oracleDigest = nxFold(oracleDigest, actual.errors);
		oracleDigest = nxFold(oracleDigest, actual.errorLine);
		oracleDigest = nxFold(oracleDigest, actual.cells);
		printf("  gap fold2 done cells=%u\n", actual.cells); fflush(stdout);
		oracleDigest = nxFold(oracleDigest, actual.grid);
		printf("  gap fold3 done grid=%08x\n", actual.grid); fflush(stdout);
		if(actual.accepted)
			++drivenAccepted;
		else
			++drivenRejected;
		printf("  gap counters done\n"); fflush(stdout);
		drivenErrors += actual.errors;
		printf("  gap errors done errors=%u total=%u\n", actual.errors, drivenErrors);
		fflush(stdout);

		// The case-line's own arguments, read through the OS before the printf uses them. If these read
		// fine and the printf still faults, the fault is in the CRT's formatting rather than in the data.
		{
		char values[256];
		unsigned bytes = (unsigned) strlen(fixture->bytes);
		_snprintf_s(values, sizeof(values), _TRUNCATE,
			"values case=%s dim=%s bytes=%u accepted=%u errors=%u line=0x%03x res=%u cells=%u grid=%08x\n",
			fixture->name, fixture->dimension, bytes,
			actual.accepted, actual.errors, actual.errorLine,
			actual.resolution, actual.cells, actual.grid);
		nxRawLine(L"canary-values.txt", values);
		}
		// RESTORED. The removal narrowed the fault -- it did not move -- but it also took phase 4's gate
		// RED, because this line is part of the coverage the oracle differential asserts. A harness that
		// does not print its cases is not a differential, so the region is narrowed another way.
		printf("pmap case=%s dimension=%s bytes=%u accepted=%u errors=%u line=0x%03x "
			"resolution=%u cells=%u grid=%08x\n",
			fixture->name, fixture->dimension, (unsigned) (strlen(fixture->bytes) / 2),
			actual.accepted, actual.errors, actual.errorLine,
			actual.resolution, actual.cells, actual.grid);
		printf("  case-line printed\n"); fflush(stdout);

		printf("  comparison about to run\n"); fflush(stdout);
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
		// One flush per case, so a fault in the candidate half below names the case that produced it
		// rather than leaving the transcript short by an unknown amount. Round 80 did this for the
		// ten targets and it turned a null call into "all ten are fine".
		printf("pmap oracle-done case=%s\n", fixture->name);
		fflush(stdout);

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
	// The triangle-mesh stream writer. The same fixture description goes to
	// both sides; each serialises it through its own module, and what is
	// compared is the complete store log and the return value.
	for(unsigned i = 0; i < kWriterFixtureCount; ++i)
		{
		const NxWriterFixture* fixture = &nxWriterFixtures[i];
		nxSeedBlobWords(fixture);

		NxWriteLog oracleLog;
		oracleLog.reset();
		unsigned oracleAccepted = 0;
		nxRunMeshWriterOracle(&oracle, fixture, &oracleLog, &oracleAccepted);
		unsigned caseDigest = nxLogFold(oracleLog);

		oracleDigest = nxFold(oracleDigest, oracleAccepted);
		oracleDigest = nxFold(oracleDigest, caseDigest);
		if(oracleAccepted)
			++drivenAccepted;
		else
			++drivenRejected;

		printf("writer case=%s dimension=%s accepted=%u events=%u bytes=%u digest=%08x\n",
			fixture->name, fixture->dimension, oracleAccepted,
			oracleLog.count, oracleLog.byteCount, caseDigest);

		if(!selfOnly)
			{
			NxWriteLog candidateLog;
			candidateLog.reset();
			unsigned candidateAccepted = 0;
			if(!nxCandidateMeshWriter(fixture, &candidateLog, &candidateAccepted))
				{
				++candidateMismatch;
				printf("writer CANDIDATE-MISSING case=%s: no reconstruction of phys_fn_002162\n",
					fixture->name);
				}
			else if(candidateAccepted != oracleAccepted)
				{
				++candidateMismatch;
				printf("writer CANDIDATE-MISMATCH case=%s accepted=%u/%u\n",
					fixture->name, candidateAccepted, oracleAccepted);
				}
			else if(nxLogFold(candidateLog) != caseDigest)
				{
				++candidateMismatch;
				printf("writer CANDIDATE-MISMATCH case=%s digest=%08x/%08x first_differing_event=%u "
					"events=%u/%u bytes=%u/%u\n",
					fixture->name, nxLogFold(candidateLog), caseDigest,
					nxLogFirstDifference(candidateLog, oracleLog),
					candidateLog.count, oracleLog.count,
					candidateLog.byteCount, oracleLog.byteCount);
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
		kPMapFixtureCount + kMeshFixtureCount + kWriterFixtureCount + 1,
		drivenAccepted, drivenRejected, drivenErrors);
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
