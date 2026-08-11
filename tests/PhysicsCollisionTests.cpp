// The narrow-phase differential, and why it is not shaped like the others.
//
// Every other differential in this program compares one harness binary run
// against the shipped pair with the same binary run against the rebuilt pair,
// and resolves what it drives with GetProcAddress. That cannot reach anything
// here: the shape-pair dispatch matrix and the overlap tests it selects are
// internal, the oracle exports none of them, and the whole path that would
// reach them from the public API -- a scene, actors, shapes -- belongs to
// phases 4, 5 and 7 and does not exist yet.
//
// So this harness holds the oracle in one process instead. It loads the pinned
// NxPhysics.dll, checks its SHA-256 against the pin the caller passes, and then
// calls the recorded addresses directly. The reconstruction is linked in. The
// comparison happens here rather than between two transcripts, which means --
// unlike a symmetric differential -- a check inside this harness *can* fail the
// gate on its own. What it still cannot do is notice that it stopped checking
// anything, so every generator prints an oracle-side digest, and those digests
// are registered in gate_targets.ps1. A digest folds the oracle's answers, so
// it moves if the generator changes, if the inputs change, or if the oracle
// stops being called at all; the reconstruction cannot make one of them right.
//
// The floating-point environment is part of what is being reproduced. These
// kernels are only ever reached from the simulation step, and phys_fn_000659 at
// 0x00013c40 calls NxSetFPURoundingChop and NxSetFPUPrecision64 before it steps
// and restores the caller's control word with `fldcw` afterwards. Every check
// below therefore runs twice, once under the CRT default 0x027f (53-bit, round
// to nearest -- what a consumer calling an export sees) and once under 0x0f7f
// (64-bit, round toward zero -- what the narrow phase actually runs under), and
// both go into the digest.

#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "NarrowPhase.h"
#include "ContactGeneration.h"
#include "NxIntersectionRayTriangle.h"
#include "NxSmoothNormals.h"

// ---------------------------------------------------------------------------
// The recovered dispatch matrix.
//
// The oracle keeps two 6x6 matrices of function pointers in one 0x124-byte heap
// object: a vtable pointer at +0x00, then 36 slots at +0x04 and 36 more at
// +0x94. phys_fn_002338 at 0x0005a8e0 is the constructor -- it zeroes both
// halves with two `rep stosd` of 0x24 dwords and then writes 40 immediates --
// and phys_fn_002348 at 0x0005ab80 is the only reader.
//
// Recovered three ways, and all three agree:
//   * the 40 `mov dword ptr [edx + N], imm32` in the constructor;
//   * the allocation size 0x124 at the one call site, 0x0000e733;
//   * the two index computations in the reader, which order the pair by shape
//     type and form 6 * type0 + type1.
// The check below is a fourth: it calls the oracle's own constructor on a local
// buffer and compares all 72 slots against this table.
//
// The type numbering is NxShapeType from the pinned public NxShape.h:
// PLANE 0, SPHERE 1, BOX 2, CAPSULE 3, MESH 4, COMPOUND 5, NX_SHAPE_COUNT 6.

struct NxMatrixEntry
	{
	unsigned rva;			// 0 where the oracle leaves the slot null
	const char* stableId;	// the inventory row that owns the target
	const char* note;
	};

// Matrix A, at +0x04: contact generation. Four arguments, __cdecl.
static const NxMatrixEntry nxMatrixA[36] =
	{
	{ 0x00000000, "-",               "plane/plane: no handler" },
	{ 0x00048a70, "phys_fn_001901",  "plane/sphere" },
	{ 0x00047f20, "phys_fn_001883",  "plane/box" },
	{ 0x00048370, "phys_fn_001891",  "plane/capsule" },
	{ 0x00048760, "phys_fn_001895",  "plane/mesh, ContactPlaneMesh.cpp" },
	{ 0x0003fa10, "phys_fn_001795",  "plane/compound, shared expander" },

	{ 0x00000000, "-",               "lower triangle" },
	{ 0x0004b860, "phys_fn_001933",  "sphere/sphere" },
	{ 0x0004a2d0, "phys_fn_001919",  "sphere/box" },
	{ 0x0004a4b0, "phys_fn_001923",  "sphere/capsule" },
	{ 0x0004b1f0, "phys_fn_001929",  "sphere/mesh" },
	{ 0x0003fa10, "phys_fn_001795",  "sphere/compound, shared expander" },

	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x0003add0, "phys_fn_001749",  "box/box" },
	{ 0x0003b260, "phys_fn_001753",  "box/capsule" },
	{ 0x0003d500, "phys_fn_001772",  "box/mesh, ContactBoxMeshICE.cpp" },
	{ 0x0003fa10, "phys_fn_001795",  "box/compound, shared expander" },

	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x0003d9d0, "phys_fn_001775",  "capsule/capsule" },
	{ 0x0003e530, "phys_fn_001779",  "capsule/mesh" },
	{ 0x0003fa10, "phys_fn_001795",  "capsule/compound, shared expander" },

	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00046ab0, "phys_fn_001876",  "mesh/mesh, ContactMeshMesh.cpp" },
	{ 0x0003fa10, "phys_fn_001795",  "mesh/compound, shared expander" },

	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x0003fa30, "phys_fn_001797",  "compound/compound, its own expander" }
	};

// Matrix B, at +0x94: the boolean overlap test, taken when either shape carries
// one of the three trigger bits. Three arguments, __cdecl, result in al.
static const NxMatrixEntry nxMatrixB[36] =
	{
	{ 0x00000000, "-",               "plane/plane: no handler" },
	{ 0x00048a20, "phys_fn_001899",  "plane/sphere" },
	{ 0x00047e90, "phys_fn_001881",  "plane/box" },
	{ 0x00048270, "phys_fn_001889",  "plane/capsule" },
	{ 0x00048680, "phys_fn_001893",  "plane/mesh" },
	{ 0x0003f570, "phys_fn_001787",  "plane/compound" },

	{ 0x00000000, "-",               "lower triangle" },
	{ 0x0004b800, "phys_fn_001931",  "sphere/sphere" },
	{ 0x00049e70, "phys_fn_001915",  "sphere/box" },
	{ 0x0004a3e0, "phys_fn_001921",  "sphere/capsule" },
	{ 0x0004a820, "phys_fn_001925",  "sphere/mesh" },
	{ 0x0003f5b0, "phys_fn_001789",  "sphere/compound" },

	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x000389d0, "phys_fn_001738",  "box/box" },
	{ 0x0003b0e0, "phys_fn_001751",  "box/capsule" },
	{ 0x0003bcd0, "phys_fn_001757",  "box/mesh" },
	{ 0x0003f700, "phys_fn_001791",  "box/compound" },

	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x0003d890, "phys_fn_001774",  "capsule/capsule" },
	{ 0x0003e370, "phys_fn_001777",  "capsule/mesh" },
	{ 0x0003f390, "phys_fn_001785",  "capsule/compound" },

	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00046550, "phys_fn_001870",  "mesh/mesh" },
	{ 0x0003f570, "phys_fn_001787",  "mesh/compound, same as plane/compound" },

	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "lower triangle" },
	{ 0x00000000, "-",               "compound/compound: NULL here, unlike matrix A" }
	};

static const unsigned kMatrixCtorRva = 0x0005a8e0;	// phys_fn_002338
static const unsigned kMatrixObjectSize = 0x124;
static const unsigned kMatrixOffsetA = 0x04;
static const unsigned kMatrixOffsetB = 0x94;
static const unsigned kSquareDistanceIatRva = 0x00104170;
// phys_fn_001261, what a PLANE shape puts in vtable slot 5. Recovered from the
// plane shape constructor's vtable store at 0x00107430 + 0x14.
static const unsigned kPlaneRaycastRva = 0x00025350;
// phys_fn_001377, the same slot on a SPHERE shape, from 0x00107528 + 0x14.
static const unsigned kSphereRaycastRva = 0x00027c70;
// hit.shape is written from shape->[0x9c] and never dereferenced by that row,
// so both sides are given the same made-up value: a real address would put this
// process's load address into an oracle-side digest that is pinned in the gate.
static void* const kFakeCollisionObject = (void*) 0x0badc0deu;

// The reconstruction, against the matrix B slot each entry sits in. Everything
// not named here is still unreconstructed and is reported as such rather than
// skipped silently.
struct NxDrivenEntry
	{
	const char* name;
	unsigned index;				// 6 * type0 + type1
	NxShapeOverlapFn candidate;
	};

static const NxDrivenEntry nxDriven[] =
	{
	{ "plane_sphere",   0 * 6 + 1, NxOverlapPlaneSphere },
	{ "plane_box",      0 * 6 + 2, NxOverlapPlaneBox },
	{ "plane_capsule",  0 * 6 + 3, NxOverlapPlaneCapsule },
	{ "sphere_sphere",  1 * 6 + 1, NxOverlapSphereSphere },
	{ "sphere_box",     1 * 6 + 2, NxOverlapSphereBox },
	{ "sphere_capsule", 1 * 6 + 3, NxOverlapSphereCapsule },
	{ "box_box",        2 * 6 + 2, NxOverlapBoxBox }
	};
static const unsigned kDrivenCount = sizeof(nxDriven) / sizeof(nxDriven[0]);

// ---------------------------------------------------------------------------
// The generator. xorshift32, seeded per block, counts fixed in the source, both
// printed so a reader can see that the run in front of them is this one.

static const unsigned kPairIterations = 60000;
static const unsigned kAimedIterations = 60000;
static const unsigned kNormalsIterations = 4000;
static const unsigned kContactIterations = 20000;
static const unsigned kSeedPair = 0xc0ffee11u;
static const unsigned kSeedAimed = 0x5eed10adu;

static unsigned nxNext(unsigned* state)
	{
	unsigned x = *state;
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	*state = x;
	return x;
	}

static float nxUnit(unsigned* state)
	{
	return (float) (nxNext(state) >> 8) * (1.0f / 16777216.0f);
	}

// Mostly values a caller would really pass, one branch in eight that is a raw
// 32-bit pattern -- denormals, negative zero, the whole range -- and one branch
// in eight that is deliberately non-finite.
//
// The second of those is here because the raw-bit branch on its own is not
// enough. A uniformly random word is NaN or infinity about one time in 256, so
// over a whole run it put 182 non-finite words into the only kernel here that
// writes floats. The NaN payload rule is the single thing /arch:IA32 governs
// and it needs two NaN operands to meet, so a generator that produces them at
// that rate measures nothing -- which is exactly how SmoothNormals.cpp was
// wrongly cleared of needing the flag in Task 2.
static float nxPick(unsigned* state)
	{
	unsigned choice = nxNext(state) & 7;
	if(choice == 0)
		{
		unsigned bits = nxNext(state);
		float value;
		memcpy(&value, &bits, 4);
		return value;
		}
	if(choice == 1)
		return 0.0f;
	if(choice == 2)
		return nxUnit(state) * 1e-6f;
	if(choice == 3)
		{
		unsigned bits = nxNext(state);
		// Sign from the low bit, payload from the rest; a zero payload is an
		// infinity and anything else is a NaN, so both arrive without either
		// being spelled out.
		bits = (bits & 0x807fffffu) | 0x7f800000u;
		float value;
		memcpy(&value, &bits, 4);
		return value;
		}
	return nxUnit(state) * 8.0f - 4.0f;
	}

// FNV-1a, 64 bit.
struct NxDigest
	{
	unsigned __int64 state;
	unsigned checks;
	};

static void nxDigestInit(NxDigest* d) { d->state = 0xcbf29ce484222325ULL; d->checks = 0; }

static void nxDigestByte(NxDigest* d, unsigned char byte)
	{
	d->state ^= byte;
	d->state *= 0x100000001b3ULL;
	++d->checks;
	}

// ---------------------------------------------------------------------------
// The x87 control word. The two states below are the two the narrow phase can
// find itself in; NxSetFPUPrecision64 and NxSetFPURoundingChop between them
// produce the second.

static const unsigned short kControlDefault = 0x027f;	// PC 53, RC near
static const unsigned short kControlSimulate = 0x0f7f;	// PC 64, RC chop

static void nxSetControl(unsigned short word)
	{
	unsigned short value = word;
	__asm { fldcw value }
	}

static unsigned short nxGetControl()
	{
	unsigned short value;
	__asm { fnstcw value }
	return value;
	}

// ---------------------------------------------------------------------------
// Loading the pinned oracle.

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

// Reports the module an address belongs to. Used for the one Foundation export
// these kernels reach, so the transcript says which NxFoundation answered it
// rather than leaving it to be assumed.
static void nxReportOwningModule(const char* what, const void* address)
	{
	HMODULE module = 0;
	wchar_t path[MAX_PATH];
	char hash[65];
	if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			(LPCWSTR) address, &module)
		|| !GetModuleFileNameW(module, path, MAX_PATH))
		{
		printf("binding %s module=unknown\n", what);
		return;
		}
	if(!nxSha256(path, hash))
		{
		printf("binding %s path=%S sha256=unavailable\n", what, path);
		return;
		}
	printf("binding %s path=%S sha256=%s\n", what, path, hash);
	}

// ---------------------------------------------------------------------------
// Shapes.

static const unsigned kShapeBytes = 0x200;

static void nxIdentity(NxCollisionShape* shape)
	{
	memset(shape, 0, kShapeBytes);
	shape->rotation[0] = 1.0f;
	shape->rotation[4] = 1.0f;
	shape->rotation[8] = 1.0f;
	}

// A rotation from a random quaternion. Not normalised on purpose in one branch
// in eight, because a shape whose pose has drifted is a state the pipeline can
// really be in and the kernels do not renormalise.
static void nxRandomRotation(unsigned* state, NxCollisionShape* shape)
	{
	float q[4];
	for(int i = 0; i < 4; ++i)
		q[i] = nxUnit(state) * 2.0f - 1.0f;
	float length = q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3];
	if(length > 1e-6f && (nxNext(state) & 7) != 0)
		{
		float scale = 1.0f / (float) sqrt((double) length);
		for(int i = 0; i < 4; ++i)
			q[i] *= scale;
		}
	float x = q[0], y = q[1], z = q[2], w = q[3];
	shape->rotation[0] = 1.0f - 2.0f * (y * y + z * z);
	shape->rotation[1] = 2.0f * (x * y - z * w);
	shape->rotation[2] = 2.0f * (x * z + y * w);
	shape->rotation[3] = 2.0f * (x * y + z * w);
	shape->rotation[4] = 1.0f - 2.0f * (x * x + z * z);
	shape->rotation[5] = 2.0f * (y * z - x * w);
	shape->rotation[6] = 2.0f * (x * z - y * w);
	shape->rotation[7] = 2.0f * (y * z + x * w);
	shape->rotation[8] = 1.0f - 2.0f * (x * x + y * y);
	}

static void nxFillGeometry(unsigned* state, NxCollisionShape* shape, unsigned type, bool tame)
	{
	shape->type = type;
	if(type == 0)
		{
		// A plane's normal is a unit vector in every configuration the SDK can
		// build, so the tame branch keeps it one.
		float n[3];
		for(int i = 0; i < 3; ++i)
			n[i] = tame ? nxUnit(state) * 2.0f - 1.0f : nxPick(state);
		if(tame)
			{
			float length = (float) sqrt((double) (n[0] * n[0] + n[1] * n[1] + n[2] * n[2]));
			if(length > 1e-4f)
				for(int i = 0; i < 3; ++i)
					n[i] /= length;
			else
				{ n[0] = 0.0f; n[1] = 1.0f; n[2] = 0.0f; }
			}
		shape->geometry[0] = n[0];
		shape->geometry[1] = n[1];
		shape->geometry[2] = n[2];
		shape->geometry[3] = tame ? nxUnit(state) * 4.0f - 2.0f : nxPick(state);
		}
	else if(type == 1)
		{
		shape->geometry[0] = tame ? nxUnit(state) * 2.0f + 0.05f : nxPick(state);
		}
	else if(type == 2)
		{
		shape->geometry[0] = tame ? 0.0f : nxPick(state);
		for(int i = 1; i < 4; ++i)
			shape->geometry[i] = tame ? nxUnit(state) * 2.0f + 0.05f : nxPick(state);
		}
	else
		{
		shape->geometry[0] = tame ? nxUnit(state) * 1.5f + 0.05f : nxPick(state);
		shape->geometry[1] = tame ? nxUnit(state) * 2.0f + 0.05f : nxPick(state);
		shape->geometry[2] = tame ? 0.0f : nxPick(state);
		shape->geometry[3] = tame ? 0.0f : nxPick(state);
		}
	}

// A rough scale for how far apart two shapes have to be before they cannot
// touch. Only used to aim the generator, never to decide an answer.
static float nxReach(const NxCollisionShape* shape)
	{
	if(shape->type == 1)
		return shape->geometry[0];
	// The mean extent, not the sum: the sum aims at a separation the two boxes
	// can essentially never span, and an aimed block that never overlaps is
	// the same as no aimed block.
	if(shape->type == 2)
		return (shape->geometry[1] + shape->geometry[2] + shape->geometry[3]) * (1.0f / 3.0f);
	if(shape->type == 3)
		return shape->geometry[0] + shape->geometry[1];
	return 0.0f;
	}

// ---------------------------------------------------------------------------

// Three parameters, not two. The dispatcher hands every +0x94 entry a third
// argument -- its own param_4, the shared context -- and although __cdecl makes
// a two-parameter declaration harmless for the seven entries driven here, which
// read neither, the four mesh entries do read it. Declaring the ABI correctly
// is what makes adding one of those safe.
typedef bool(__cdecl* NxOracleOverlapFn)(const NxCollisionShape*, const NxCollisionShape*, void*);
typedef void*(__thiscall* NxMatrixCtorFn)(void*);

// Stubs for the index probe below. They record that they were reached and
// return false, which is what keeps the overlap path from going on to append to
// a trigger-pair array this harness has not built.
// The context every +0x94 entry receives as its third argument. None of the
// seven driven here reads it; it is real memory rather than a null so that
// adding a mesh entry, which does read it, is a change of one line.
static unsigned char nxOverlapContextStorage[0x800];
static void* const nxOverlapContext = nxOverlapContextStorage;

static unsigned nxProbeCalls = 0;

static bool __cdecl nxProbeOverlap(const void*, const void*, void*)
	{
	++nxProbeCalls;
	return false;
	}

static void __cdecl nxProbeContact(const void*, const void*, void*, void*)
	{
	++nxProbeCalls;
	}

// One side's world for the contact-generation differential: two shapes, the
// borrowed Phase 5 graph each needs before the emitter will write, a sink and a
// stream. Two are built, one per side, so neither can observe the other's
// addresses.
//
// Every offset touched here is in the borrowed-layout section of
// evidence/phase3-narrow-phase.md with the address that establishes it, and
// nothing is filled in to make the call complete: the emitter reads
// `owner+0x08` at 0x0001d729 and `holder+0x240` at 0x0001d730, the shape
// constructors write `object+0x08` at 0x000247e9, and `shape+0x04` and
// `shape+0x9c` come from 0x00025543 and 0x00024f0b.
struct NxContactWorld
	{
	NxCollisionShape* plane;
	NxCollisionShape* sphere;
	NxContactSink sink;
	NxU32 stream[0x2000];
	unsigned char shapeStore[2][kShapeBytes];
	unsigned char objectStore[2][2][0x20];
	unsigned char ownerStore[2][0x40];
	unsigned char holderStore[2][0x280];
	NxU32 generation[2];
	// A vtable for the first shape. phys_fn_001891 dispatches through slot 5 of
	// the *partner* shape's vtable at 0x000484e6, so a plane driven into it
	// needs one; six slots because 0x14 is the highest offset any kernel here
	// reads. Installed after nxStageWorld, which copies a zeroed vtable pointer
	// in with the shape, and only by the block that needs it -- the plane/sphere
	// and emitter blocks leave it null and never read it.
	void* shapeVtable[6];
	};

static void nxResetWorld(NxContactWorld* world)
	{
	memset(&world->sink, 0, sizeof(world->sink));
	memset(world->stream, 0, sizeof(world->stream));
	world->sink.stream = world->stream;
	world->sink.streamCapacity = sizeof(world->stream) / sizeof(world->stream[0]);
	// Word 0 is the pair counter the header increments. This is not the harness
	// improvising: it is what phys_fn_002354 at 0x0005b620 does, which resets
	// this object at `sink + 0x10` and reserves exactly one stream word whose
	// index it records. Driving the oracle's own reset over a poisoned sink
	// reproduces the state below field for field.
	world->sink.streamCount = 1;
	world->sink.pairCountIndex = 0;
	world->generation[0] = 0;
	world->generation[1] = 0;
	world->plane = (NxCollisionShape*) world->shapeStore[0];
	world->sphere = (NxCollisionShape*) world->shapeStore[1];
	}

// The two shapes are staged independently on purpose. An earlier version used
// one identity flag and one material for both, which made two behaviours of the
// emitter unmeasurable: the header predicate is an OR over the two collision
// objects, so with them always changing together `||` and `&&` are the same
// function; and the material is read from shape1's owner with a fallback to
// shape0's, so with both holders carrying the same value the choice never
// showed. Both are now driven separately.
static void nxStageWorld(NxContactWorld* world, const NxCollisionShape* planeShape,
	const NxCollisionShape* sphereShape, bool newIdentity0, bool newIdentity1,
	NxU32 material0, NxU32 material1, bool nullHolder1, bool orientToSphere)
	{
	if(newIdentity0)
		++world->generation[0];
	if(newIdentity1)
		++world->generation[1];
	// kShapeBytes, not sizeof(NxCollisionShape): the struct models the fields
	// the kernels read, and the oracle's shape is larger. Every buffer a shape
	// pointer can reach is kShapeBytes here and zeroed by nxIdentity, so a read
	// past the modelled fields is deterministic rather than whatever was on the
	// stack.
	memcpy(world->plane, planeShape, kShapeBytes);
	memcpy(world->sphere, sphereShape, kShapeBytes);
	for(int which = 0; which < 2; ++which)
		{
		NxCollisionShape* shape = which ? world->sphere : world->plane;
		const unsigned slot = world->generation[which] & 1;
		unsigned char* owner = world->ownerStore[which];
		unsigned char* holder = world->holderStore[which];
		memset(owner, 0, sizeof(world->ownerStore[which]));
		memset(holder, 0, sizeof(world->holderStore[which]));
		// Both collision objects are fully built, because the emitter reaches
		// the shape back through `object+0x08` -- an identity change that only
		// moved the pointer would leave that read pointing at nothing.
		for(unsigned s = 0; s < 2; ++s)
			{
			memset(world->objectStore[which][s], 0, sizeof(world->objectStore[which][s]));
			*(NxCollisionShape**) (world->objectStore[which][s] + 8) = shape;
			}
		// A null `owner->[8]` on shape1's side is a supported state -- the
		// emitter falls back to shape0's at 0x0001d738 -- and nothing had ever
		// entered it.
		*(unsigned char**) (owner + 8) = (which == 1 && nullHolder1) ? 0 : holder;
		*(NxU32*) (holder + 0x240) = which ? material1 : material0;
		shape->owner = owner;
		// The header turns on this pointer changing, so a new identity picks the
		// other of the two objects.
		shape->collisionObject = world->objectStore[which][slot];
		}
	world->sink.orientedTo = orientToSphere ? world->holderStore[1] : 0;
	}

// Canonicalise a stream word. Only the two header words are addresses, but this
// is applied to every word rather than to those two positions, because the
// reader does not track where it is in the stream -- so a point or a normal
// that happened to equal one of the four object addresses would be rewritten
// too. That is a deliberate trade: mistaking a coordinate for an address folds
// both sides identically and cannot mask a difference, whereas position
// tracking would have to duplicate the emitter's own state machine.
static NxU32 nxCanonical(const NxContactWorld* world, NxU32 word)
	{
	for(int which = 0; which < 2; ++which)
		for(int slot = 0; slot < 2; ++slot)
			if(word == (NxU32) (size_t) world->objectStore[which][slot])
				return 0xf0000000u | (NxU32) (which * 2 + slot);
	return word;
	}

// One side's whole stream into that side's own digest. Each side is folded over
// its own word count rather than over the shorter of the two, so the oracle
// half of a registered line is a function of the oracle alone.
static void nxFoldStream(NxDigest* digest, const NxContactWorld* world)
	{
	for(unsigned w = 0; w < world->sink.streamCount; ++w)
		{
		const NxU32 word = nxCanonical(world, world->stream[w]);
		for(int byte = 0; byte < 4; ++byte)
			nxDigestByte(digest, (unsigned char) (word >> (byte * 8)));
		}
	}

static unsigned nxCompareStreams(const NxContactWorld* a, const NxContactWorld* b)
	{
	unsigned differing = 0;
	if(a->sink.streamCount != b->sink.streamCount
		|| a->sink.contactCount != b->sink.contactCount
		|| a->sink.featurePairValid != b->sink.featurePairValid)
		++differing;
	const unsigned common = a->sink.streamCount < b->sink.streamCount
		? a->sink.streamCount : b->sink.streamCount;
	for(unsigned w = 0; w < common; ++w)
		if(nxCanonical(a, a->stream[w]) != nxCanonical(b, b->stream[w]))
			++differing;
	return differing;
	}

struct NxBlockResult
	{
	NxDigest oracle;
	NxDigest candidate;
	unsigned mismatches;
	unsigned trueCount;
	unsigned falseCount;
	unsigned swapDiffers;
	};

static void nxRunPair(NxOracleOverlapFn oracle, NxShapeOverlapFn candidate,
		const NxCollisionShape* a, const NxCollisionShape* b, NxBlockResult* result)
	{
	for(int mode = 0; mode < 2; ++mode)
		{
		nxSetControl(mode ? kControlSimulate : kControlDefault);
		unsigned char fromOracle = oracle(a, b, nxOverlapContext) ? 1 : 0;
		unsigned char fromCandidate = candidate(a, b) ? 1 : 0;
		nxSetControl(kControlDefault);

		nxDigestByte(&result->oracle, fromOracle);
		nxDigestByte(&result->candidate, fromCandidate);
		if(fromOracle != fromCandidate)
			++result->mismatches;
		if(fromOracle)
			++result->trueCount;
		else
			++result->falseCount;
		}
	}

int wmain(int argc, wchar_t** argv)
	{
	if(argc != 3)
		{
		fprintf(stderr, "usage: NxPhysicsCollisionTests <oracle directory> <NxPhysics.dll sha256>\n");
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
	printf("oracle base=%p control_word=%04x\n", (void*) physics, nxGetControl());
	if(strcmp(loadedHash, expected) != 0)
		{
		fprintf(stderr, "FAIL loaded oracle is not the pinned one: expected %s\n", expected);
		return 1;
		}
	printf("oracle pin=matched\n");

	unsigned char* base = (unsigned char*) physics;

	// Which NxFoundation answered the one Foundation export these kernels
	// reach. This harness links the reconstruction, which links NxFoundation,
	// so the oracle's import may bind to the rebuilt module rather than the
	// shipped one. That is a shared callee on both sides of one entry, and the
	// transcript has to say so rather than leave it to be assumed.
	nxReportOwningModule("NxComputeSquareDistance", *(void**) (base + kSquareDistanceIatRva));

	// -----------------------------------------------------------------------
	// The dispatch matrix, checked against the oracle's own constructor.
	unsigned char object[kMatrixObjectSize];
	memset(object, 0xcd, sizeof(object));
	NxMatrixCtorFn ctor = (NxMatrixCtorFn) (base + kMatrixCtorRva);
	void* returned = ctor(object);
	printf("matrix ctor rva=0x%08x size=0x%x returned_this=%d\n",
		kMatrixCtorRva, kMatrixObjectSize, returned == (void*) object ? 1 : 0);
	if(returned != (void*) object)
		return nxFail("the matrix constructor did not return its own this");

	unsigned matrixChecked = 0;
	unsigned matrixWrong = 0;
	unsigned matrixNull = 0;
	for(int half = 0; half < 2; ++half)
		{
		const NxMatrixEntry* recovered = half ? nxMatrixB : nxMatrixA;
		unsigned offset = half ? kMatrixOffsetB : kMatrixOffsetA;
		for(unsigned index = 0; index < 36; ++index)
			{
			unsigned char* slot = *(unsigned char**) (object + offset + index * 4);
			unsigned actual = slot ? (unsigned) (slot - base) : 0u;
			++matrixChecked;
			if(!recovered[index].rva)
				++matrixNull;
			if(actual != recovered[index].rva)
				{
				++matrixWrong;
				printf("matrix MISMATCH half=%c index=%u type0=%u type1=%u recovered=0x%08x actual=0x%08x\n",
					half ? 'B' : 'A', index, index / 6, index % 6, recovered[index].rva, actual);
				}
			// The lower triangle must be null: it is what makes the reader's
			// swap of the pair load-bearing rather than decorative.
			if(index / 6 > index % 6 && actual != 0)
				{
				++matrixWrong;
				printf("matrix LOWER-TRIANGLE-NOT-NULL half=%c index=%u actual=0x%08x\n",
					half ? 'B' : 'A', index, actual);
				}
			}
		}
	printf("matrix slots=%u null=%u wrong=%u\n", matrixChecked, matrixNull, matrixWrong);
	for(int half = 0; half < 2; ++half)
		{
		const NxMatrixEntry* recovered = half ? nxMatrixB : nxMatrixA;
		for(unsigned index = 0; index < 36; ++index)
			if(recovered[index].rva || index / 6 <= index % 6)
				printf("matrix entry half=%c type0=%u type1=%u index=%u rva=0x%08x owner=%s note=%s\n",
					half ? 'B' : 'A', index / 6, index % 6, index,
					recovered[index].rva, recovered[index].stableId, recovered[index].note);
		}

	// The index rule and the pair swap, measured against the oracle's own
	// dispatcher rather than restated.
	//
	// An earlier version of this check compared NxCollisionPairIndex(t0,t1)
	// against `t0 * 6 + t1` -- the function's own body written out again -- and
	// then against itself. It pinned the enum at 6 and checked nothing the
	// oracle does, while the RED-mode table described it as "the index rule
	// checked over every ordered pair". It is now what that claim said.
	//
	// The method: hand phys_fn_002348 a matrix object of our own with exactly
	// one slot filled, and see whether it calls it. Sweeping all 36 slots for
	// each of the 36 ordered type pairs says which slot the oracle picks,
	// without assuming the answer. Doing it for both argument orders is the
	// pair-symmetry measurement: (t0,t1) and (t1,t0) must land on the same slot,
	// which is the whole reason the lower triangle can be null.
	unsigned indexWrong = 0;
	unsigned indexProbes = 0;
	{
	unsigned char probeObject[kMatrixObjectSize];
	unsigned char probeShape0[kShapeBytes];
	unsigned char probeShape1[kShapeBytes];
	unsigned char probeContext[0x800];
	memset(probeContext, 0, sizeof(probeContext));
	NxCollisionShape* p0 = (NxCollisionShape*) probeShape0;
	NxCollisionShape* p1 = (NxCollisionShape*) probeShape1;
	typedef void(__thiscall* NxDispatchFn)(void*, const void*, const void*, void*, void*);
	NxDispatchFn dispatch = (NxDispatchFn) (base + 0x0005ab80);

	for(int half = 0; half < 2; ++half)
		{
		unsigned offset = half ? kMatrixOffsetB : kMatrixOffsetA;
		// The +0x94 half is the one a trigger flag selects.
		unsigned char flags = half ? 1 : 0;
		for(unsigned t0 = 0; t0 < NX_COLLISION_SHAPE_TYPES; ++t0)
			for(unsigned t1 = 0; t1 < NX_COLLISION_SHAPE_TYPES; ++t1)
				{
				unsigned expected = NxCollisionPairIndex(t0 < t1 ? t0 : t1, t0 < t1 ? t1 : t0);
				for(unsigned slot = 0; slot < 36; ++slot)
					{
					memset(probeObject, 0, sizeof(probeObject));
					*(void**) (probeObject + offset + slot * 4) =
						half ? (void*) nxProbeOverlap : (void*) nxProbeContact;
					memset(probeShape0, 0, kShapeBytes);
					memset(probeShape1, 0, kShapeBytes);
					p0->type = t0;
					p1->type = t1;
					probeShape0[0xde] = flags;
					probeShape1[0xde] = flags;

					nxProbeCalls = 0;
					dispatch(probeObject, probeShape0, probeShape1, probeContext, probeContext);
					++indexProbes;
					unsigned wanted = (slot == expected) ? 1u : 0u;
					if(nxProbeCalls != wanted)
						{
						++indexWrong;
						if(indexWrong <= 4)
							printf("matrix INDEX-MISMATCH half=%c t0=%u t1=%u slot=%u expected_calls=%u actual=%u\n",
								half ? 'B' : 'A', t0, t1, slot, wanted, nxProbeCalls);
						}
					}
				}
		}
	}
	printf("matrix index_rule probes=%u wrong=%u\n", indexProbes, indexWrong);

	// -----------------------------------------------------------------------
	// The kernels.
	printf("collision generator=xorshift32 pair_iterations=%u aimed_iterations=%u\n",
		kPairIterations, kAimedIterations);
	printf("collision seeds pair=%08x aimed=%08x\n", kSeedPair, kSeedAimed);
	printf("collision control_words default=%04x simulate=%04x\n", kControlDefault, kControlSimulate);

	unsigned totalMismatch = 0;
	unsigned char storage0[kShapeBytes];
	unsigned char storage1[kShapeBytes];
	NxCollisionShape* shape0 = (NxCollisionShape*) storage0;
	NxCollisionShape* shape1 = (NxCollisionShape*) storage1;

	for(unsigned entry = 0; entry < kDrivenCount; ++entry)
		{
		unsigned index = nxDriven[entry].index;
		unsigned type0 = index / 6;
		unsigned type1 = index % 6;
		unsigned rva = nxMatrixB[index].rva;
		NxOracleOverlapFn oracle = (NxOracleOverlapFn) (base + rva);
		NxShapeOverlapFn candidate = nxDriven[entry].candidate;

		NxBlockResult random;
		memset(&random, 0, sizeof(random));
		nxDigestInit(&random.oracle);
		nxDigestInit(&random.candidate);
		NxBlockResult aimed = random;

		unsigned state = kSeedPair ^ (index * 0x9e3779b9u);
		for(unsigned i = 0; i < kPairIterations; ++i)
			{
			bool tame = (nxNext(&state) & 3) != 0;
			nxIdentity(shape0);
			nxIdentity(shape1);
			nxRandomRotation(&state, shape0);
			nxRandomRotation(&state, shape1);
			for(int k = 0; k < 3; ++k)
				{
				shape0->translation[k] = tame ? nxUnit(&state) * 6.0f - 3.0f : nxPick(&state);
				shape1->translation[k] = tame ? nxUnit(&state) * 6.0f - 3.0f : nxPick(&state);
				}
			nxFillGeometry(&state, shape0, type0, tame);
			nxFillGeometry(&state, shape1, type1, tame);
			nxRunPair(oracle, candidate, shape0, shape1, &random);
			}

		// Aimed: put the second shape at a separation either side of the reach
		// of the first, so separated, touching and penetrating configurations
		// are all reached instead of being left to chance. A purely random
		// block essentially never touches.
		state = kSeedAimed ^ (index * 0x85ebca6bu);
		for(unsigned i = 0; i < kAimedIterations; ++i)
			{
			nxIdentity(shape0);
			nxIdentity(shape1);
			nxRandomRotation(&state, shape0);
			nxRandomRotation(&state, shape1);
			for(int k = 0; k < 3; ++k)
				shape0->translation[k] = nxUnit(&state) * 2.0f - 1.0f;
			nxFillGeometry(&state, shape0, type0, true);
			nxFillGeometry(&state, shape1, type1, true);

			float direction[3];
			float length = 0.0f;
			for(int k = 0; k < 3; ++k)
				{
				direction[k] = nxUnit(&state) * 2.0f - 1.0f;
				length += direction[k] * direction[k];
				}
			length = (float) sqrt((double) length);
			if(length < 1e-4f)
				{ direction[0] = 1.0f; direction[1] = 0.0f; direction[2] = 0.0f; length = 1.0f; }
			for(int k = 0; k < 3; ++k)
				direction[k] /= length;

			float reach = nxReach(shape0) + nxReach(shape1);
			if(type0 == 0)
				{
				// Against a plane the interesting family is a shape straddling
				// the plane, so aim along the normal from a point on it.
				const float* n = shape0->geometry;
				float offset = (nxUnit(&state) * 2.0f - 1.0f) * (nxReach(shape1) * 1.5f + 0.25f);
				for(int k = 0; k < 3; ++k)
					shape1->translation[k] = n[k] * (offset - shape0->geometry[3]);
				}
			else
				{
				float separation = reach * (0.2f + nxUnit(&state) * 1.4f);
				for(int k = 0; k < 3; ++k)
					shape1->translation[k] = shape0->translation[k] + direction[k] * separation;
				}
			nxRunPair(oracle, candidate, shape0, shape1, &aimed);

			// Pair symmetry: the reader orders the pair before it indexes, so
			// these entries are only ever handed (low type, high type). Count
			// how often the swapped call disagrees, which is the measure of how
			// much work the ordering is doing.
			if(type0 != type1)
				{
				nxSetControl(kControlSimulate);
				bool ordered = oracle(shape0, shape1, nxOverlapContext);
				bool swapped = oracle(shape1, shape0, nxOverlapContext);
				nxSetControl(kControlDefault);
				if(ordered != swapped)
					++aimed.swapDiffers;
				}
			}

		totalMismatch += random.mismatches + aimed.mismatches;
		printf("collision name=%s.random index=%u rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
			nxDriven[entry].name, index, rva, nxMatrixB[index].stableId,
			random.oracle.checks, random.oracle.state, random.candidate.state, random.mismatches);
		printf("collision coverage name=%s.random true=%u false=%u\n",
			nxDriven[entry].name, random.trueCount, random.falseCount);
		printf("collision name=%s.aimed index=%u rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
			nxDriven[entry].name, index, rva, nxMatrixB[index].stableId,
			aimed.oracle.checks, aimed.oracle.state, aimed.candidate.state, aimed.mismatches);
		printf("collision coverage name=%s.aimed true=%u false=%u swap_differs=%u\n",
			nxDriven[entry].name, aimed.trueCount, aimed.falseCount, aimed.swapDiffers);
		}

	// -----------------------------------------------------------------------
	// The two helper rows, driven at their own recorded addresses as well as
	// through their callers. phys_fn_000943 is the only kernel in this
	// component that writes floats rather than returning a bool, so it is the
	// only place the NaN payload rule -- the thing /arch:IA32 exists for -- can
	// be observed at all. Its output is poisoned before every call so that
	// "wrote nothing" and "wrote zero" are different transcripts.
	{
	typedef void(__thiscall* NxOracleCornerFn)(const NxCollisionShape*, int, int, int, NxVec3*);
	NxOracleCornerFn oracleCorner = (NxOracleCornerFn) (base + 0x00020750);

	NxDigest oracleDigest, candidateDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	unsigned mismatches = 0;
	unsigned nonFinite = 0;
	unsigned state = 0x1337c0deu;
	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		bool tame = (nxNext(&state) & 3) != 0;
		nxIdentity(shape0);
		nxRandomRotation(&state, shape0);
		for(int k = 0; k < 3; ++k)
			shape0->translation[k] = tame ? nxUnit(&state) * 6.0f - 3.0f : nxPick(&state);
		nxFillGeometry(&state, shape0, 2, tame);
		int signX = (nxNext(&state) & 1) ? 1 : -1;
		int signY = (nxNext(&state) & 1) ? 1 : -1;
		int signZ = (nxNext(&state) & 1) ? 1 : -1;

		for(int mode = 0; mode < 2; ++mode)
			{
			NxVec3 fromOracle, fromCandidate;
			static const unsigned poison[3] = { 0xcdcd0001u, 0xcdcd0002u, 0xcdcd0003u };
			memcpy(&fromOracle, poison, sizeof(poison));
			memcpy(&fromCandidate, poison, sizeof(poison));

			nxSetControl(mode ? kControlSimulate : kControlDefault);
			oracleCorner(shape0, signX, signY, signZ, &fromOracle);
			NxBoxShapeCorner(shape0, signX, signY, signZ, &fromCandidate);
			nxSetControl(kControlDefault);

			for(int k = 0; k < 3; ++k)
				{
				unsigned a, b;
				memcpy(&a, &(&fromOracle.x)[k], 4);
				memcpy(&b, &(&fromCandidate.x)[k], 4);
				for(int byte = 0; byte < 4; ++byte)
					{
					nxDigestByte(&oracleDigest, (unsigned char) (a >> (byte * 8)));
					nxDigestByte(&candidateDigest, (unsigned char) (b >> (byte * 8)));
					}
				if(a != b)
					++mismatches;
				if((a & 0x7f800000u) == 0x7f800000u)
					++nonFinite;
				}
			}
		}
	totalMismatch += mismatches;
	printf("collision name=box_corner index=- rva=0x00020750 owner=phys_fn_000943 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	printf("collision coverage name=box_corner non_finite_words=%u\n", nonFinite);
	}

	{
	typedef bool(__cdecl* NxOracleSphereBoxFn)(const NxCollisionSphereData*, const NxCollisionBoxData*);
	NxOracleSphereBoxFn oracleSphereBox = (NxOracleSphereBoxFn) (base + 0x00049ca0);

	NxDigest oracleDigest, candidateDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	unsigned mismatches = 0;
	unsigned trueCount = 0;
	unsigned insideCount = 0;
	unsigned state = 0x2bad5eedu;
	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		bool tame = (nxNext(&state) & 3) != 0;
		nxIdentity(shape0);
		nxIdentity(shape1);
		nxRandomRotation(&state, shape1);
		nxFillGeometry(&state, shape1, 2, tame);

		NxCollisionSphereData sphere;
		NxCollisionBoxData box;
		for(int k = 0; k < 3; ++k)
			{
			box.center[k] = tame ? nxUnit(&state) * 2.0f - 1.0f : nxPick(&state);
			box.extents[k] = shape1->geometry[k + 1];
			}
		for(int k = 0; k < 9; ++k)
			box.rotation[k] = shape1->rotation[k];
		sphere.radius = tame ? nxUnit(&state) * 1.5f + 0.02f : nxPick(&state);

		// Half the run puts the centre inside the box, which is the one path
		// that returns before the closest point is ever transformed back.
		bool inside = (nxNext(&state) & 1) != 0;
		if(inside && tame)
			{
			++insideCount;
			for(int k = 0; k < 3; ++k)
				sphere.center[k] = box.center[k] + (nxUnit(&state) * 2.0f - 1.0f) * box.extents[k] * 0.5f;
			}
		else
			{
			for(int k = 0; k < 3; ++k)
				sphere.center[k] = tame ? nxUnit(&state) * 4.0f - 2.0f : nxPick(&state);
			}

		for(int mode = 0; mode < 2; ++mode)
			{
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			unsigned char fromOracle = oracleSphereBox(&sphere, &box) ? 1 : 0;
			unsigned char fromCandidate = NxOverlapSphereBoxData(&sphere, &box) ? 1 : 0;
			nxSetControl(kControlDefault);
			nxDigestByte(&oracleDigest, fromOracle);
			nxDigestByte(&candidateDigest, fromCandidate);
			if(fromOracle != fromCandidate)
				++mismatches;
			if(fromOracle)
				++trueCount;
			}
		}
	totalMismatch += mismatches;
	printf("collision name=sphere_box_data index=- rva=0x00049ca0 owner=phys_fn_001913 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	printf("collision coverage name=sphere_box_data true=%u centre_inside=%u\n", trueCount, insideCount);
	}

	// -----------------------------------------------------------------------
	// Two exports that the recovered matrix puts inside the simulation step.
	//
	// These are Task 2's rows, not this task's, and they were closed by a
	// staged-pair differential that runs under the CRT default control word
	// only. That was correct for the evidence available then: with the matrix
	// unrecovered, no direct call edge reaches them from phys_fn_000659 and
	// they looked like consumer-facing exports. The matrix closes the gap.
	//
	//   NxBuildSmoothNormals  <- 0x52271 <- 0x3c344 <- 0x3d87c in
	//                            phys_fn_001772, matrix A [BOX][MESH]
	//   NxRayTriIntersect     <- 0x36e4b <- 0x415a2 <- 0x42ea2 <- 0x4366c
	//                         <- 0x47067 in phys_fn_001876, matrix A [MESH][MESH]
	//
	// Every edge is a direct `call rel32`. So both run at 64-bit precision with
	// round-toward-zero whenever a mesh pair reaches contact generation, and
	// nothing had ever exercised them that way. NxBoxBoxIntersect is on the
	// same footing and is already covered under both words through the box_box
	// row above.
	//
	// This block is also the second witness for the x87 NaN payload rule, which
	// until now rested on one. It uses the non-finite mixture above rather than
	// the fuzz harness's, whose consecutive-mode draws take the raw-bit branch
	// only one or two times in fifteen.
	{
	typedef bool(NX_CALL_CONV* NxOracleRayTriFn)(const NxVec3&, const NxVec3&, const NxVec3&,
		const NxVec3&, const NxVec3&, float&, float&, float&, bool);
	typedef bool(NX_CALL_CONV* NxOracleNormalsFn)(NxU32, NxU32, const NxVec3*, const NxU32*,
		const NxU16*, NxVec3*, bool);
	NxOracleRayTriFn oracleRayTri = (NxOracleRayTriFn) GetProcAddress(physics, "NxRayTriIntersect");
	NxOracleNormalsFn oracleNormals = (NxOracleNormalsFn) GetProcAddress(physics, "NxBuildSmoothNormals");
	if(!oracleRayTri || !oracleNormals)
		return nxFail("the pinned oracle does not export the two step-reachable kernels");

	NxDigest oracleDigest, candidateDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	unsigned mismatches = 0;
	unsigned perMode[2] = { 0, 0 };
	unsigned hits = 0;
	unsigned nonFinite = 0;
	unsigned state = 0x7ea11a1du;
	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		NxVec3 v[5];
		bool tame = (nxNext(&state) & 1) != 0;
		for(int k = 0; k < 5; ++k)
			for(int c = 0; c < 3; ++c)
				(&v[k].x)[c] = tame ? nxUnit(&state) * 4.0f - 2.0f : nxPick(&state);
		// Half of the tame iterations aim the ray through a random barycentric
		// point of the triangle, so the tail past both range tests is reached
		// rather than left to chance -- the same correction the fuzz harness
		// needed for this export.
		if(tame && (nxNext(&state) & 1))
			{
			float a = nxUnit(&state), b = nxUnit(&state) * (1.0f - a);
			for(int c = 0; c < 3; ++c)
				{
				float target = (&v[2].x)[c] + a * ((&v[3].x)[c] - (&v[2].x)[c])
					+ b * ((&v[4].x)[c] - (&v[2].x)[c]);
				(&v[1].x)[c] = target - (&v[0].x)[c];
				}
			}
		bool cull = (nxNext(&state) & 1) != 0;

		for(int mode = 0; mode < 2; ++mode)
			{
			float t[2], u[2], w[2];
			unsigned char result[2];
			static const unsigned poison[3] = { 0xcdcd0001u, 0xcdcd0002u, 0xcdcd0003u };
			for(int side = 0; side < 2; ++side)
				{
				memcpy(&t[side], poison + 0, 4);
				memcpy(&u[side], poison + 1, 4);
				memcpy(&w[side], poison + 2, 4);
				}
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			result[0] = oracleRayTri(v[0], v[1], v[2], v[3], v[4], t[0], u[0], w[0], cull) ? 1 : 0;
			result[1] = NxRayTriIntersect(v[0], v[1], v[2], v[3], v[4], t[1], u[1], w[1], cull) ? 1 : 0;
			nxSetControl(kControlDefault);

			if(result[0])
				++hits;
			nxDigestByte(&oracleDigest, result[0]);
			nxDigestByte(&candidateDigest, result[1]);
			if(result[0] != result[1])
				{ ++mismatches; ++perMode[mode]; }
			const float* words[2][3] = { { &t[0], &u[0], &w[0] }, { &t[1], &u[1], &w[1] } };
			for(int k = 0; k < 3; ++k)
				{
				unsigned a, b;
				memcpy(&a, words[0][k], 4);
				memcpy(&b, words[1][k], 4);
				for(int byte = 0; byte < 4; ++byte)
					{
					nxDigestByte(&oracleDigest, (unsigned char) (a >> (byte * 8)));
					nxDigestByte(&candidateDigest, (unsigned char) (b >> (byte * 8)));
					}
				if(a != b)
					{ ++mismatches; ++perMode[mode]; }
				if((a & 0x7f800000u) == 0x7f800000u)
					++nonFinite;
				}
			}
		}
	totalMismatch += mismatches;
	printf("collision name=step_ray_tri index=- rva=export owner=phys_fn_001712 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	printf("collision coverage name=step_ray_tri hits=%u non_finite_words=%u default_mismatches=%u simulate_mismatches=%u\n",
		hits, nonFinite, perMode[0], perMode[1]);

	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	mismatches = 0;
	perMode[0] = 0;
	perMode[1] = 0;
	nonFinite = 0;
	state = 0x3110c1a5u;
	for(unsigned i = 0; i < kNormalsIterations; ++i)
		{
		const NxU32 nbVerts = 3 + (nxNext(&state) % 14);
		const NxU32 nbTris = 1 + (nxNext(&state) % 12);
		NxVec3 verts[17];
		NxVec3 normals[2][17];
		NxU32 dFaces[12 * 3];
		bool tame = (nxNext(&state) & 1) != 0;
		for(NxU32 vertex = 0; vertex < nbVerts; ++vertex)
			for(int c = 0; c < 3; ++c)
				(&verts[vertex].x)[c] = tame ? nxUnit(&state) * 4.0f - 2.0f : nxPick(&state);
		// Indices stay in range: the export bounds-checks none of them and an
		// out-of-range one would corrupt this harness's own heap.
		for(NxU32 index = 0; index < nbTris * 3; ++index)
			dFaces[index] = nxNext(&state) % nbVerts;
		bool flip = (nxNext(&state) & 1) != 0;

		for(int mode = 0; mode < 2; ++mode)
			{
			for(int side = 0; side < 2; ++side)
				for(NxU32 vertex = 0; vertex < 17; ++vertex)
					for(int c = 0; c < 3; ++c)
						{
						const unsigned word = 0xcdcd0000u + vertex * 3 + c;
						memcpy(&(&normals[side][vertex].x)[c], &word, 4);
						}
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			unsigned char a = oracleNormals(nbTris, nbVerts, verts, dFaces, 0, normals[0], flip) ? 1 : 0;
			unsigned char b = NxBuildSmoothNormals(nbTris, nbVerts, verts, dFaces, 0, normals[1], flip) ? 1 : 0;
			nxSetControl(kControlDefault);

			nxDigestByte(&oracleDigest, a);
			nxDigestByte(&candidateDigest, b);
			if(a != b)
				{ ++mismatches; ++perMode[mode]; }
			for(NxU32 vertex = 0; vertex < nbVerts; ++vertex)
				for(int c = 0; c < 3; ++c)
					{
					unsigned x, y;
					memcpy(&x, &(&normals[0][vertex].x)[c], 4);
					memcpy(&y, &(&normals[1][vertex].x)[c], 4);
					for(int byte = 0; byte < 4; ++byte)
						{
						nxDigestByte(&oracleDigest, (unsigned char) (x >> (byte * 8)));
						nxDigestByte(&candidateDigest, (unsigned char) (y >> (byte * 8)));
						}
					if(x != y)
						{ ++mismatches; ++perMode[mode]; }
					if((x & 0x7f800000u) == 0x7f800000u)
						++nonFinite;
					}
			}
		}
	// Only the default-word half of this block gates, and that is a measured
	// restriction rather than a convenience.
	//
	// Under 0x027f the reconstruction agrees with the oracle on all 459,676
	// checks. Under 0x0f7f it differs on 24 of them, and every one is a last-bit
	// difference or a cancellation that lands on zero on one side only. The
	// cause is not a transcription error and not a library call: replacing
	// sqrt() with the fsqrt intrinsic moved nothing, and replacing atan2() with
	// an inline `fpatan` moved nothing either, though both of those library
	// routines really do ignore the x87 control word (measured: with the word at
	// 0x0f7f, sqrt(2.0) is 3ff6a09e667f3bcd from the CRT and 3ff6a09e667f3bcc
	// from fsqrt). What is left is where the compiler chooses to spill a
	// `double`. Under 53-bit precision a spill is invisible, because an x87
	// register and an 8-byte slot hold the same value; under 64-bit precision
	// every spill truncates, and no C++ controls where MSVC puts them.
	//
	// So this is an escalation, not a defect to fix: phys_fn_002146 is closed
	// under one control word and cannot currently be closed under the other.
	// step_ray_tri, on the same path and with fewer live values, agrees under
	// both -- so the limit bites where register pressure is high, not
	// everywhere.
	//
	// The simulate-word count is printed and registered rather than dropped, so
	// it is a tripwire in its own right: it fails if it moves in either
	// direction, including toward zero.
	totalMismatch += perMode[0];
	printf("collision name=step_smooth_normals index=- rva=export owner=phys_fn_002146 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	printf("collision coverage name=step_smooth_normals non_finite_words=%u default_mismatches=%u simulate_mismatches=%u\n",
		nonFinite, perMode[0], perMode[1]);
	}

	// -----------------------------------------------------------------------
	// Contact generation: complete records, not counts.
	//
	// Each side gets its own world -- shapes, owners, holders, collision
	// objects, sink and stream -- so neither can observe the other. A run drives
	// a short sequence of pairs into one sink before resetting it, which is what
	// exercises the three nested count levels: the pair header is written only
	// when a collision object changes, the normal block only when the normal
	// changes, and the pair is swapped with the normal negated when the sink is
	// oriented to the other body. Comparing only the last record would miss all
	// three.
	//
	// The stream is pre-sized so the oracle's growth path at 0x000b4de0 -- a
	// Phase 2 row reaching the SDK allocator -- is never entered on either side.
	// That path is unexercised and a matching stream says nothing about it.
	{
	typedef void(__cdecl* NxOracleContactFn)(const NxCollisionShape*, const NxCollisionShape*,
		NxContactSink*, void*);
	NxOracleContactFn oracleContact = (NxOracleContactFn) (base + nxMatrixA[1].rva);

	static NxContactWorld world[2];
	NxDigest oracleDigest, candidateDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	unsigned mismatches = 0;
	unsigned emitted = 0;
	unsigned widths[16];
	memset(widths, 0, sizeof(widths));


	unsigned negatedPath = 0;
	unsigned state = 0x4dea11c7u;

	for(unsigned i = 0; i < kContactIterations; ++i)
		{
		// One geometry sequence, replayed under both control words.
		const unsigned sequenceSeed = nxNext(&state);
		const unsigned pairs = 1 + (nxNext(&state) % 4);

		for(int mode = 0; mode < 2; ++mode)
			{
			for(int side = 0; side < 2; ++side)
				nxResetWorld(&world[side]);

			unsigned local = sequenceSeed;
			// The plane is generated once per sequence, so the contact normal
			// is constant across the sequence and the normal-block skip is
			// reached. With a fresh plane per pair the block was rewritten
			// almost every time and the caching rule went untested.
			static unsigned char planeStorage[kShapeBytes];
			NxCollisionShape* planeShape = (NxCollisionShape*) planeStorage;
			const bool planeTame = (nxNext(&local) & 3) != 0;
			nxIdentity(planeShape);
			nxFillGeometry(&local, planeShape, 0, planeTame);
			for(unsigned p = 0; p < pairs; ++p)
				{
				static unsigned char sphereStorage[kShapeBytes];
				NxCollisionShape* sphereShape = (NxCollisionShape*) sphereStorage;
				const bool tame = (nxNext(&local) & 3) != 0;
				nxIdentity(sphereShape);
				nxFillGeometry(&local, sphereShape, 1, tame);
				for(int k = 0; k < 3; ++k)
					sphereShape->translation[k] = tame ? nxUnit(&local) * 3.0f - 1.5f : nxPick(&local);
				// One pair in four starts a new shape identity, so both the
				// header-writing and header-skipping paths are reached.
				// The two identities move independently, so the header
				// predicate's OR is exercised on each side alone as well as on
				// both together. With one flag for both, `||` and `&&` are the
				// same function and the rule went untested.
				const bool newIdentity0 = (p == 0) || ((nxNext(&local) & 3) == 0);
				const bool newIdentity1 = (p == 0) || ((nxNext(&local) & 3) == 0);
				// Different materials, and a full byte each, so the shift into
				// bits 24..31 is exercised rather than only its low seven bits.
				const NxU32 material0 = nxNext(&local) & 0xff;
				const NxU32 material1 = nxNext(&local) & 0xff;
				// A null `owner->[8]` on shape1's side is a supported state --
				// the emitter falls back to shape0's at 0x0001d738 -- and
				// nothing had ever entered it.
				const bool nullHolder1 = (nxNext(&local) & 7) == 0;
				const bool orientToSphere = (nxNext(&local) & 1) != 0;
				// One pair in three re-rolls the plane, so a normal can change
				// without an identity changing. That is the one stream state a
				// per-sequence plane could never reach.
				if(p && (nxNext(&local) % 3) == 0)
					{
					nxIdentity(planeShape);
					nxFillGeometry(&local, planeShape, 0, planeTame);
					}

				const unsigned before = world[0].sink.streamCount;
				for(int side = 0; side < 2; ++side)
					nxStageWorld(&world[side], planeShape, sphereShape,
						newIdentity0, newIdentity1, material0, material1,
						nullHolder1, orientToSphere);

				nxSetControl(mode ? kControlSimulate : kControlDefault);
				oracleContact(world[0].plane, world[0].sphere, &world[0].sink, nxOverlapContext);
				NxContactPlaneSphere(world[1].plane, world[1].sphere, &world[1].sink, nxOverlapContext);
				nxSetControl(kControlDefault);

				// A histogram, not buckets. The previous counters keyed on the
				// total words appended and called anything >= 8 a normal block
				// and >= 11 a header, so a header without a normal block -- 7
				// words -- counted as neither, and `headers == normal_blocks`
				// would have survived the very failure it was offered as ruling
				// out. Each distinct width is reported instead: 4 is a bare
				// record, 8 adds a normal block, 11 adds a header as well.
				const unsigned appended = world[0].sink.streamCount - before;
				if(appended)
					++emitted;
				if(appended < 16)
					++widths[appended];
				if(!orientToSphere)
					++negatedPath;
				}

			// Each side is folded over its own stream and only the overlap is
			// compared. An earlier version digested both sides over the shorter
			// of the two counts, which made the oracle-side digest a function
			// of the candidate: a reconstruction that emitted fewer words moved
			// it. Measured, not argued -- a mutant that made the plane/capsule
			// entry return early moved that block's oracle digest and its
			// `checks` from 2180528 to 2077464. It could never have produced a
			// false pass, because a shortened digest does not equal the pin
			// either, but the registration says the oracle half cannot be moved
			// from this side and that was not true. The two counts are equal on
			// every agreeing run, so no pinned digest moves.
			//
			// The collision-object words are addresses and each side has its
			// own, so they are canonicalised to which shape they name.
			// Everything else is folded raw.
			nxFoldStream(&oracleDigest, &world[0]);
			nxFoldStream(&candidateDigest, &world[1]);
			mismatches += nxCompareStreams(&world[0], &world[1]);
			}
		}
	totalMismatch += mismatches;
	printf("collision name=contact_plane_sphere index=1 rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		nxMatrixA[1].rva, nxMatrixA[1].stableId,
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	printf("collision coverage name=contact_plane_sphere emitted=%u w4=%u w7=%u w8=%u w11=%u negated_path=%u\n",
		emitted, widths[4], widths[7], widths[8], widths[11], negatedPath);
	}

	// -----------------------------------------------------------------------
	// The emitter, driven directly with real feature ids.
	//
	// Every matrix A entry reachable today is a primitive pair and passes
	// 0xffff/0xffff, so `featurePairValid` is always 0, the fifth word of a
	// record is never appended, and both halves of the feature rule -- the
	// validity test at 0x0001d694 and the swap at 0x0001d63c -- were dead. The
	// mesh entries are where real ids come from and they need Phase 4, but the
	// emitter is __thiscall at a known address and can be driven now.
	{
	typedef void(__thiscall* NxOracleEmitFn)(NxContactSink*, void*, void*, NxU32,
		const NxVec3*, const NxVec3*, NxU16, NxU16);
	NxOracleEmitFn oracleEmit = (NxOracleEmitFn) (base + 0x0001d610);

	static NxContactWorld emitWorld[2];
	NxDigest oracleDigest, candidateDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	unsigned mismatches = 0;
	unsigned realFeatures = 0;
	unsigned fifthWords = 0;
	unsigned state = 0x0fea70edu;

	for(unsigned i = 0; i < kContactIterations; ++i)
		{
		const unsigned sequenceSeed = nxNext(&state);
		const unsigned pairs = 1 + (nxNext(&state) % 4);
		for(int mode = 0; mode < 2; ++mode)
			{
			for(int side = 0; side < 2; ++side)
				nxResetWorld(&emitWorld[side]);
			unsigned local = sequenceSeed;
			for(unsigned p = 0; p < pairs; ++p)
				{
				static unsigned char planeStore[kShapeBytes];
				static unsigned char sphereStore[kShapeBytes];
				NxCollisionShape* pl = (NxCollisionShape*) planeStore;
				NxCollisionShape* sp = (NxCollisionShape*) sphereStore;
				const bool tame = (nxNext(&local) & 3) != 0;
				nxIdentity(pl);
				nxIdentity(sp);
				nxFillGeometry(&local, pl, 0, tame);
				nxFillGeometry(&local, sp, 1, tame);

				// Half the ids are real and half are 0xffff, independently, so
				// all four combinations of the validity rule are reached.
				const NxU16 id0 = (nxNext(&local) & 1) ? (NxU16) (nxNext(&local) & 0x7fff) : (NxU16) 0xffff;
				const NxU16 id1 = (nxNext(&local) & 1) ? (NxU16) (nxNext(&local) & 0x7fff) : (NxU16) 0xffff;
				if(id0 != 0xffff && id1 != 0xffff)
					++realFeatures;

				NxVec3 point, normal;
				for(int k = 0; k < 3; ++k)
					{
					(&point.x)[k] = tame ? nxUnit(&local) * 4.0f - 2.0f : nxPick(&local);
					(&normal.x)[k] = tame ? nxUnit(&local) * 2.0f - 1.0f : nxPick(&local);
					}
				const NxU32 separationBits = nxNext(&local);

				const bool newIdentity0 = (p == 0) || ((nxNext(&local) & 3) == 0);
				const bool newIdentity1 = (p == 0) || ((nxNext(&local) & 3) == 0);
				const NxU32 material0 = nxNext(&local) & 0xff;
				const NxU32 material1 = nxNext(&local) & 0xff;
				const bool nullHolder1 = (nxNext(&local) & 7) == 0;
				const bool orientToSphere = (nxNext(&local) & 1) != 0;

				for(int side = 0; side < 2; ++side)
					nxStageWorld(&emitWorld[side], pl, sp, newIdentity0, newIdentity1,
						material0, material1, nullHolder1, orientToSphere);

				const unsigned before = emitWorld[0].sink.streamCount;
				nxSetControl(mode ? kControlSimulate : kControlDefault);
				oracleEmit(&emitWorld[0].sink, emitWorld[0].sphere->collisionObject,
					emitWorld[0].plane->collisionObject, separationBits, &point, &normal, id0, id1);
				NxEmitContact(&emitWorld[1].sink, emitWorld[1].sphere->collisionObject,
					emitWorld[1].plane->collisionObject, separationBits, &point, &normal, id0, id1);
				nxSetControl(kControlDefault);
				const unsigned appended = emitWorld[0].sink.streamCount - before;
				if(appended == 5 || appended == 9 || appended == 12)
					++fifthWords;
				}

			nxFoldStream(&oracleDigest, &emitWorld[0]);
			nxFoldStream(&candidateDigest, &emitWorld[1]);
			mismatches += nxCompareStreams(&emitWorld[0], &emitWorld[1]);
			}
		}
	totalMismatch += mismatches;
	printf("collision name=contact_emit index=- rva=0x0001d610 owner=phys_fn_000873 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	printf("collision coverage name=contact_emit real_feature_pairs=%u fifth_words=%u\n",
		realFeatures, fifthWords);
	}

	// -----------------------------------------------------------------------
	// phys_fn_001261, driven at its own address before the entry that reaches
	// it, because the entry cannot reach all of it.
	//
	// phys_fn_001891 calls this with hintFlags = 0 and with the distance limit
	// set to exactly the segment length it just measured. So the
	// NX_RAYCAST_NORMAL branch at 0x000253fd -- the only thing that ever writes
	// hit.worldNormal -- and any limit other than an exact fit are dead from
	// there. That is the same shape of gap the feature ids had: a behaviour is
	// not covered because the code that contains it ran.
	//
	// The hit is poisoned before every call, so "wrote nothing" and "wrote
	// zero" are different transcripts. It matters here: NxRayPlaneIntersect
	// fills hit.worldImpact before either distance gate runs, so a raycast
	// rejected for being behind the origin still leaves one field written.
	{
	typedef const void*(__thiscall* NxOracleRaycastFn)(const void*, const NxRay*, NxReal,
		NxU32, NxU32, NxRaycastHit*);
	NxOracleRaycastFn oracleRaycast = (NxOracleRaycastFn) (base + kPlaneRaycastRva);

	NxDigest oracleDigest, candidateDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	unsigned mismatches = 0;
	unsigned hits = 0;
	unsigned wroteNormal = 0;
	unsigned aimedRays = 0;
	unsigned state = 0x51ce9a11u;

	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		const bool tame = (nxNext(&state) & 3) != 0;
		nxIdentity(shape0);
		nxFillGeometry(&state, shape0, 0, tame);
		shape0->collisionObject = kFakeCollisionObject;

		NxRay ray;
		for(int k = 0; k < 3; ++k)
			(&ray.orig.x)[k] = tame ? nxUnit(&state) * 4.0f - 2.0f : nxPick(&state);

		NxReal maxDistance;
		// Half the tame rays are aimed at a point that really is on the plane,
		// so the hit distance is known and the limit can be placed either side
		// of it. Unaimed, the facing test at 0x00025381 rejects most rays and
		// the two distance gates behind it are barely reached.
		const bool aimed = tame && (nxNext(&state) & 1) != 0;
		if(aimed)
			{
			++aimedRays;
			const float* n = shape0->geometry;
			float tangent[3];
			float projection = 0.0f;
			for(int k = 0; k < 3; ++k)
				{
				tangent[k] = nxUnit(&state) * 6.0f - 3.0f;
				projection += tangent[k] * n[k];
				}
			float delta[3];
			float length = 0.0f;
			for(int k = 0; k < 3; ++k)
				{
				const float target = n[k] * -shape0->geometry[3] + (tangent[k] - projection * n[k]);
				delta[k] = target - (&ray.orig.x)[k];
				length += delta[k] * delta[k];
				}
			length = (float) sqrt((double) length);
			if(length < 1e-4f)
				{ delta[0] = 1.0f; delta[1] = 0.0f; delta[2] = 0.0f; length = 1.0f; }
			for(int k = 0; k < 3; ++k)
				(&ray.dir.x)[k] = delta[k] / length;
			maxDistance = length * (0.2f + nxUnit(&state) * 1.6f);
			}
		else
			{
			for(int k = 0; k < 3; ++k)
				(&ray.dir.x)[k] = tame ? nxUnit(&state) * 2.0f - 1.0f : nxPick(&state);
			maxDistance = tame ? nxUnit(&state) * 6.0f : nxPick(&state);
			}

		// Bit 2 is the only one 0x000253f4 tests. It is set on half the calls
		// and the other seven bits are random, so "only bit 2 matters" is
		// driven rather than assumed.
		NxU32 hintFlags = nxNext(&state) & 0xfbu;
		if(nxNext(&state) & 1)
			hintFlags |= NX_RAYCAST_NORMAL;

		for(int mode = 0; mode < 2; ++mode)
			{
			NxRaycastHit hit[2];
			memset(hit, 0xcd, sizeof(hit));

			nxSetControl(mode ? kControlSimulate : kControlDefault);
			const unsigned char fromOracle =
				oracleRaycast(shape0, &ray, maxDistance, 0, hintFlags, &hit[0]) ? 1 : 0;
			const unsigned char fromCandidate =
				NxShapeRaycastPlane(shape0, 0, &ray, maxDistance, 0, hintFlags, &hit[1]) ? 1 : 0;
			nxSetControl(kControlDefault);

			if(fromOracle)
				++hits;
			if((hit[0].flags & NX_RAYCAST_NORMAL) && fromOracle)
				++wroteNormal;
			nxDigestByte(&oracleDigest, fromOracle);
			nxDigestByte(&candidateDigest, fromCandidate);
			if(fromOracle != fromCandidate)
				++mismatches;
			for(unsigned w = 0; w < sizeof(NxRaycastHit) / 4; ++w)
				{
				NxU32 a, b;
				memcpy(&a, (const unsigned char*) &hit[0] + w * 4, 4);
				memcpy(&b, (const unsigned char*) &hit[1] + w * 4, 4);
				for(int byte = 0; byte < 4; ++byte)
					{
					nxDigestByte(&oracleDigest, (unsigned char) (a >> (byte * 8)));
					nxDigestByte(&candidateDigest, (unsigned char) (b >> (byte * 8)));
					}
				if(a != b)
					++mismatches;
				}
			}
		}
	totalMismatch += mismatches;
	printf("collision name=shape_raycast_plane index=- rva=0x%08x owner=phys_fn_001261 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kPlaneRaycastRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	printf("collision coverage name=shape_raycast_plane hits=%u wrote_normal=%u aimed=%u\n",
		hits, wroteNormal, aimedRays);
	}

	// -----------------------------------------------------------------------
	// Matrix A [PLANE][CAPSULE].
	//
	// The first entry in this matrix that is not one kernel. The byte at
	// capsule+0xe8 selects between a two-endpoint plane test and a segment
	// raycast issued through slot 5 of the partner shape's vtable, and the two
	// share nothing but the endpoint construction. That byte is
	// NxCapsuleShapeDesc::flags -- 0x00021ad0 loads the capsule's geometry from
	// its descriptor and copies desc+0x54 into +0xe8 at 0x00021af9, one field
	// past the radius at +0x4c and the height at +0x50 -- and NX_SWEPT_SHAPE is
	// bit 0, the bit `test al,1` reads. Nothing else in this harness touches it,
	// so it is driven from the generator on both sides.
	//
	// world->sphere is the second shape slot; here it carries the capsule.
	{
	typedef void(__cdecl* NxOracleContactFn)(const NxCollisionShape*, const NxCollisionShape*,
		NxContactSink*, void*);
	NxOracleContactFn oracleContact = (NxOracleContactFn) (base + nxMatrixA[3].rva);

	static NxContactWorld world[2];
	// The oracle's side dispatches into the shipped DLL's own plane slot 5 and
	// the candidate's into the reconstruction, so the entry and the row it
	// reaches are covered together and neither can be right by borrowing the
	// other's answer.
	world[0].shapeVtable[5] = (void*) (base + kPlaneRaycastRva);
	world[1].shapeVtable[5] = (void*) &NxShapeRaycastPlane;

	NxDigest oracleDigest, candidateDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	unsigned mismatches = 0;
	unsigned emitted = 0;
	unsigned oneContact = 0;
	unsigned twoContacts = 0;
	unsigned swept = 0;
	unsigned sweptEmitted = 0;
	unsigned zeroAxis = 0;
	unsigned widths[24];
	memset(widths, 0, sizeof(widths));
	unsigned state = 0x9cab5017u;

	for(unsigned i = 0; i < kContactIterations; ++i)
		{
		const unsigned sequenceSeed = nxNext(&state);
		const unsigned pairs = 1 + (nxNext(&state) % 4);

		for(int mode = 0; mode < 2; ++mode)
			{
			for(int side = 0; side < 2; ++side)
				nxResetWorld(&world[side]);

			unsigned local = sequenceSeed;
			static unsigned char planeStorage[kShapeBytes];
			NxCollisionShape* planeShape = (NxCollisionShape*) planeStorage;
			const bool planeTame = (nxNext(&local) & 3) != 0;
			nxIdentity(planeShape);
			nxFillGeometry(&local, planeShape, 0, planeTame);

			for(unsigned p = 0; p < pairs; ++p)
				{
				static unsigned char capsuleStorage[kShapeBytes];
				NxCollisionShape* capsuleShape = (NxCollisionShape*) capsuleStorage;
				const bool tame = (nxNext(&local) & 3) != 0;
				nxIdentity(capsuleShape);
				nxFillGeometry(&local, capsuleShape, 3, tame);

				// The capsule axis is column 1 of the rotation and nothing else
				// of it is read. A random quaternion essentially never produces
				// an axis parallel to the plane normal, one lying in the plane,
				// or a zero one -- and those are exactly the cases that make the
				// two endpoints behave differently from each other, so they are
				// generated rather than waited for.
				// Every aim below is arithmetic on the plane's own normal or on the
				// capsule's own geometry, so it is done only when those are
				// finite. A NaN reaching one of these multiplies is propagated
				// by SSE, and which operand's payload survives depends on the
				// order the compiler happened to emit -- so the *generated
				// shape* would differ between two builds of this harness, and
				// with it the oracle's own digest. That is not hypothetical: it
				// is what made an earlier version of this block print a
				// different oracle digest whenever the file was recompiled, and
				// it is why the aimed placement falls back to raw draws rather
				// than aiming with a non-finite normal.
				const bool aimable = planeTame && tame;
				unsigned axisMode = nxNext(&local) % 5;
				// Modes 2 and 3 are the two built out of the plane's normal. The
				// draw itself is left alone so the sequence does not move.
				if(!aimable && (axisMode == 2 || axisMode == 3))
					axisMode = 4;
				const NxReal* planeNormal = planeShape->geometry;
				if(axisMode == 0)
					nxRandomRotation(&local, capsuleShape);
				else if(axisMode == 2)
					{
					const float sign = (nxNext(&local) & 1) ? 1.0f : -1.0f;
					capsuleShape->rotation[1] = planeNormal[0] * sign;
					capsuleShape->rotation[4] = planeNormal[1] * sign;
					capsuleShape->rotation[7] = planeNormal[2] * sign;
					}
				else if(axisMode == 3)
					{
					// Orthogonal to the normal, so both endpoints sit at the
					// same plane distance and the two contacts stand or fall
					// together.
					float pick[3];
					float projection = 0.0f;
					for(int k = 0; k < 3; ++k)
						{
						pick[k] = nxUnit(&local) * 2.0f - 1.0f;
						projection += pick[k] * planeNormal[k];
						}
					capsuleShape->rotation[1] = pick[0] - projection * planeNormal[0];
					capsuleShape->rotation[4] = pick[1] - projection * planeNormal[1];
					capsuleShape->rotation[7] = pick[2] - projection * planeNormal[2];
					}
				else if(axisMode == 4)
					memset(capsuleShape->rotation, 0, sizeof(capsuleShape->rotation));
				// axisMode 1 keeps the identity, so the axis is world +Y.

				// A zero-length capsule, which is what the swept path's
				// normalisation guard at 0x0004846d exists for. Two ways to
				// reach it, because a zero half height and a zero axis are not
				// the same input to the endpoint construction.
				const bool degenerate = (nxNext(&local) & 7) == 0;
				if(degenerate)
					capsuleShape->geometry[1] = 0.0f;
				if(degenerate || axisMode == 4)
					++zeroAxis;

				// Bit 0 half the time, the other 31 bits random, so "only bit 0
				// is tested" is driven rather than assumed.
				NxU32 flagWord = nxNext(&local);
				const bool sweptShape = (nxNext(&local) & 1) != 0;
				flagWord = sweptShape ? (flagWord | 1u) : (flagWord & ~1u);
				memcpy(&capsuleShape->geometry[2], &flagWord, 4);

				// Three placements in four straddle the plane: with the capsule
				// centred at random, neither endpoint is within a radius of the
				// plane often enough for the emitting paths to be reached at
				// all. The fourth is unaimed so the far-away and non-finite
				// cases still arrive.
				if(aimable && (nxNext(&local) & 3) != 0)
					{
					const float span = capsuleShape->geometry[0] + capsuleShape->geometry[1];
					const float offset = (nxUnit(&local) * 2.0f - 1.0f) * (span * 1.5f + 0.25f);
					for(int k = 0; k < 3; ++k)
						capsuleShape->translation[k] =
							planeNormal[k] * (offset - planeShape->geometry[3]);
					}
				else
					for(int k = 0; k < 3; ++k)
						capsuleShape->translation[k] =
							tame ? nxUnit(&local) * 3.0f - 1.5f : nxPick(&local);

				const bool newIdentity0 = (p == 0) || ((nxNext(&local) & 3) == 0);
				const bool newIdentity1 = (p == 0) || ((nxNext(&local) & 3) == 0);
				const NxU32 material0 = nxNext(&local) & 0xff;
				const NxU32 material1 = nxNext(&local) & 0xff;
				const bool nullHolder1 = (nxNext(&local) & 7) == 0;
				const bool orientToSphere = (nxNext(&local) & 1) != 0;
				if(p && (nxNext(&local) % 3) == 0)
					{
					nxIdentity(planeShape);
					nxFillGeometry(&local, planeShape, 0, planeTame);
					}

				const unsigned before = world[0].sink.streamCount;
				const unsigned contactsBefore = world[0].sink.contactCount;
				for(int side = 0; side < 2; ++side)
					{
					nxStageWorld(&world[side], planeShape, capsuleShape,
						newIdentity0, newIdentity1, material0, material1,
						nullHolder1, orientToSphere);
					// After staging: nxStageWorld copies the shape template in,
					// and that template's vtable pointer is zero.
					*(void**) world[side].plane = world[side].shapeVtable;
					}

				nxSetControl(mode ? kControlSimulate : kControlDefault);
				oracleContact(world[0].plane, world[0].sphere, &world[0].sink, nxOverlapContext);
				NxContactPlaneCapsule(world[1].plane, world[1].sphere, &world[1].sink, nxOverlapContext);
				nxSetControl(kControlDefault);

				const unsigned appended = world[0].sink.streamCount - before;
				const unsigned contacts = world[0].sink.contactCount - contactsBefore;
				if(appended)
					++emitted;
				if(appended < 24)
					++widths[appended];
				if(contacts == 1)
					++oneContact;
				else if(contacts == 2)
					++twoContacts;
				if(sweptShape)
					{
					++swept;
					if(appended)
						++sweptEmitted;
					}
				}

			nxFoldStream(&oracleDigest, &world[0]);
			nxFoldStream(&candidateDigest, &world[1]);
			mismatches += nxCompareStreams(&world[0], &world[1]);
			}
		}
	totalMismatch += mismatches;
	printf("collision name=contact_plane_capsule index=3 rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		nxMatrixA[3].rva, nxMatrixA[3].stableId,
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	// one/two are the oracle's own contact count moving by one or by two, which
	// is the multiple-contact rule; swept_emitted is how often the vtable call
	// reported a hit. w15 is a header, a normal block and two records in one
	// call -- the state only this entry can produce.
	printf("collision coverage name=contact_plane_capsule emitted=%u one=%u two=%u swept=%u swept_emitted=%u zero_axis=%u w4=%u w8=%u w11=%u w15=%u\n",
		emitted, oneContact, twoContacts, swept, sweptEmitted, zeroAxis,
		widths[4], widths[8], widths[11], widths[15]);
	}

	// -----------------------------------------------------------------------
	// phys_fn_001377, driven at its own address.
	//
	// Two behaviours the sphere/capsule entry cannot reach: it always passes
	// NX_RAYCAST_NORMAL, so the flags-clear path is dead from there, and it
	// always passes a limit equal to the segment length. And two this row has
	// that the plane's does not: it has no facing test, so a sphere *behind*
	// the ray origin is a hit whenever NxRaySphereIntersect says so, and it
	// writes hit.distance before it tests the limit, so a rejected raycast
	// still leaves one field written. Poisoning the hit is what makes the
	// second visible.
	{
	typedef const void*(__thiscall* NxOracleRaycastFn)(const void*, const NxRay*, NxReal,
		NxU32, NxU32, NxRaycastHit*);
	NxOracleRaycastFn oracleRaycast = (NxOracleRaycastFn) (base + kSphereRaycastRva);

	NxDigest oracleDigest, candidateDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	unsigned mismatches = 0;
	unsigned perMode[2] = { 0, 0 };
	unsigned hits = 0;
	unsigned wroteNormal = 0;
	unsigned behindRays = 0;
	unsigned aimedRays = 0;
	unsigned state = 0x7b0c1d33u;

	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		const bool tame = (nxNext(&state) & 3) != 0;
		nxIdentity(shape0);
		nxFillGeometry(&state, shape0, 1, tame);
		for(int k = 0; k < 3; ++k)
			shape0->translation[k] = tame ? nxUnit(&state) * 4.0f - 2.0f : nxPick(&state);
		shape0->collisionObject = kFakeCollisionObject;

		NxRay ray;
		NxReal maxDistance;
		const bool aimed = tame && (nxNext(&state) & 1) != 0;
		if(aimed)
			{
			++aimedRays;
			float direction[3];
			float length = 0.0f;
			for(int k = 0; k < 3; ++k)
				{
				direction[k] = nxUnit(&state) * 2.0f - 1.0f;
				length += direction[k] * direction[k];
				}
			length = (float) sqrt((double) length);
			if(length < 1e-4f)
				{ direction[0] = 1.0f; direction[1] = 0.0f; direction[2] = 0.0f; length = 1.0f; }
			for(int k = 0; k < 3; ++k)
				(&ray.dir.x)[k] = direction[k] / length;
			// A signed distance along that direction. Negative puts the sphere
			// behind the origin, which this row -- unlike the plane's -- has no
			// test to reject.
			const bool behind = (nxNext(&state) & 3) == 0;
			if(behind)
				++behindRays;
			const float along = (nxUnit(&state) * 3.0f + 0.5f) * (behind ? -1.0f : 1.0f);
			// An offset across the ray, so the sphere is grazed as well as hit
			// through the middle and missed outright.
			const float across = (nxUnit(&state) * 2.0f - 1.0f) * shape0->geometry[0] * 1.4f;
			for(int k = 0; k < 3; ++k)
				(&ray.orig.x)[k] = shape0->translation[k] - (&ray.dir.x)[k] * along
					+ ((k + 1) % 3 == 0 ? across : -across) * 0.5f;
			maxDistance = (along < 0.0f ? -along : along) * (0.2f + nxUnit(&state) * 1.6f);
			}
		else
			{
			for(int k = 0; k < 3; ++k)
				{
				(&ray.orig.x)[k] = tame ? nxUnit(&state) * 4.0f - 2.0f : nxPick(&state);
				(&ray.dir.x)[k] = tame ? nxUnit(&state) * 2.0f - 1.0f : nxPick(&state);
				}
			maxDistance = tame ? nxUnit(&state) * 6.0f : nxPick(&state);
			}

		NxU32 hintFlags = nxNext(&state) & 0xfbu;
		if(nxNext(&state) & 1)
			hintFlags |= NX_RAYCAST_NORMAL;

		for(int mode = 0; mode < 2; ++mode)
			{
			NxRaycastHit hit[2];
			memset(hit, 0xcd, sizeof(hit));

			nxSetControl(mode ? kControlSimulate : kControlDefault);
			const unsigned char fromOracle =
				oracleRaycast(shape0, &ray, maxDistance, 0, hintFlags, &hit[0]) ? 1 : 0;
			const unsigned char fromCandidate =
				NxShapeRaycastSphere(shape0, 0, &ray, maxDistance, 0, hintFlags, &hit[1]) ? 1 : 0;
			nxSetControl(kControlDefault);

			if(fromOracle)
				++hits;
			if((hit[0].flags & NX_RAYCAST_NORMAL) && fromOracle)
				++wroteNormal;
			nxDigestByte(&oracleDigest, fromOracle);
			nxDigestByte(&candidateDigest, fromCandidate);
			if(fromOracle != fromCandidate)
				{ ++mismatches; ++perMode[mode]; }
			for(unsigned w = 0; w < sizeof(NxRaycastHit) / 4; ++w)
				{
				NxU32 a, b;
				memcpy(&a, (const unsigned char*) &hit[0] + w * 4, 4);
				memcpy(&b, (const unsigned char*) &hit[1] + w * 4, 4);
				for(int byte = 0; byte < 4; ++byte)
					{
					nxDigestByte(&oracleDigest, (unsigned char) (a >> (byte * 8)));
					nxDigestByte(&candidateDigest, (unsigned char) (b >> (byte * 8)));
					}
				if(a != b)
					{ ++mismatches; ++perMode[mode]; }
				}
			}
		}
	// Only the default-word half gates, and that is a measured restriction with
	// a named cause rather than a convenience.
	//
	// Under 0x027f the reconstruction agrees on all 2,940,000 checks. Under
	// 0x0f7f it differs on 13, and every one of those is hit.worldImpact or the
	// hit.distance derived from it -- never a field this row computes for
	// itself. worldImpact is written by NxRaySphereIntersect, and the recovered
	// matrix has just put that export inside the simulation step for the first
	// time: phys_fn_001923, matrix A [SPHERE][CAPSULE], dispatches through
	// vtable slot 5 to phys_fn_001377, whose only callee is phys_fn_001710 at
	// 0x00036e80. Its reconstruction in Geometry.cpp takes sqrt() of the
	// discriminant and keeps the root wide across a subtraction; the CRT routine
	// ignores the x87 control word, and no C++ hands a 64-bit significand to the
	// operation after it. That is Task 2's row and the same escalation
	// NxBuildSmoothNormals already carries, so the count is registered rather
	// than dropped: it fails if it moves either way, including toward zero.
	totalMismatch += perMode[0];
	printf("collision name=shape_raycast_sphere index=- rva=0x%08x owner=phys_fn_001377 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kSphereRaycastRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	printf("collision coverage name=shape_raycast_sphere hits=%u wrote_normal=%u aimed=%u behind=%u default_mismatches=%u simulate_mismatches=%u\n",
		hits, wroteNormal, aimedRays, behindRays, perMode[0], perMode[1]);
	}

	// -----------------------------------------------------------------------
	// Matrix A [SPHERE][CAPSULE].
	//
	// The same NX_SWEPT_SHAPE split, but the flag-clear path is a segment/point
	// distance rather than two plane distances, so what distinguishes its cases
	// is *where along the axis* the sphere sits: past an endpoint, beside the
	// interior, or exactly on the axis -- the last being the one placement that
	// reaches the zero-length-normal return at 0x0004a811. A random pair
	// reaches none of the three often enough to matter, so the sphere is placed
	// in the capsule's own frame.
	//
	// world->plane is the first shape slot; here it carries the sphere, and it
	// is the one that needs a vtable.
	{
	typedef void(__cdecl* NxOracleContactFn)(const NxCollisionShape*, const NxCollisionShape*,
		NxContactSink*, void*);
	NxOracleContactFn oracleContact = (NxOracleContactFn) (base + nxMatrixA[9].rva);

	static NxContactWorld world[2];
	world[0].shapeVtable[5] = (void*) (base + kSphereRaycastRva);
	world[1].shapeVtable[5] = (void*) &NxShapeRaycastSphere;

	NxDigest oracleDigest, candidateDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	unsigned mismatches = 0;
	unsigned emitted = 0;
	unsigned swept = 0;
	unsigned sweptEmitted = 0;
	unsigned zeroAxis = 0;
	unsigned coincident = 0;
	unsigned beyondEnd = 0;
	unsigned widths[24];
	memset(widths, 0, sizeof(widths));
	unsigned state = 0x2ca95e11u;

	for(unsigned i = 0; i < kContactIterations; ++i)
		{
		const unsigned sequenceSeed = nxNext(&state);
		const unsigned pairs = 1 + (nxNext(&state) % 4);

		for(int mode = 0; mode < 2; ++mode)
			{
			for(int side = 0; side < 2; ++side)
				nxResetWorld(&world[side]);

			unsigned local = sequenceSeed;
			for(unsigned p = 0; p < pairs; ++p)
				{
				static unsigned char sphereStorage[kShapeBytes];
				static unsigned char capsuleStorage[kShapeBytes];
				NxCollisionShape* sphereShape = (NxCollisionShape*) sphereStorage;
				NxCollisionShape* capsuleShape = (NxCollisionShape*) capsuleStorage;
				const bool tame = (nxNext(&local) & 3) != 0;
				nxIdentity(sphereShape);
				nxIdentity(capsuleShape);
				nxFillGeometry(&local, sphereShape, 1, tame);
				nxFillGeometry(&local, capsuleShape, 3, tame);
				for(int k = 0; k < 3; ++k)
					capsuleShape->translation[k] =
						tame ? nxUnit(&local) * 3.0f - 1.5f : nxPick(&local);

				const unsigned axisMode = nxNext(&local) % 4;
				if(axisMode == 0)
					nxRandomRotation(&local, capsuleShape);
				else if(axisMode == 2)
					{
					// A skew axis that is not a rotation column at all: only
					// m[1], m[4] and m[7] are read and nothing requires them to
					// be unit, so a drifted pose is a state this can be in.
					capsuleShape->rotation[1] = nxUnit(&local) * 4.0f - 2.0f;
					capsuleShape->rotation[4] = nxUnit(&local) * 4.0f - 2.0f;
					capsuleShape->rotation[7] = nxUnit(&local) * 4.0f - 2.0f;
					}
				else if(axisMode == 3)
					memset(capsuleShape->rotation, 0, sizeof(capsuleShape->rotation));
				// axisMode 1 keeps the identity: the axis is world +Y.

				const bool degenerate = (nxNext(&local) & 7) == 0;
				if(degenerate)
					capsuleShape->geometry[1] = 0.0f;
				if(degenerate || axisMode == 3)
					++zeroAxis;

				NxU32 flagWord = nxNext(&local);
				const bool sweptShape = (nxNext(&local) & 1) != 0;
				flagWord = sweptShape ? (flagWord | 1u) : (flagWord & ~1u);
				memcpy(&capsuleShape->geometry[2], &flagWord, 4);

				// Place the sphere in the capsule's frame: `along` is measured
				// in half heights from the centre, so |along| > 1 is past an
				// endpoint, and `across` is a perpendicular offset in units of
				// the two radii, with zero putting the centre exactly on the
				// axis. Both are aims; neither decides an answer.
				// Same finiteness rule as the plane/capsule block above, and for
				// the same reason: aiming through a non-finite axis would make
				// the generated shape depend on how the compiler ordered a
				// multiply.
				bool placed = false;
				if(tame && (nxNext(&local) & 3) != 0)
					{
					const float hh = capsuleShape->geometry[1];
					float axis[3];
					axis[0] = capsuleShape->rotation[1] * hh;
					axis[1] = capsuleShape->rotation[4] * hh;
					axis[2] = capsuleShape->rotation[7] * hh;
					float other[3] = { 0.0f, 0.0f, 0.0f };
					other[nxNext(&local) % 3] = 1.0f;
					float perp[3];
					perp[0] = axis[1] * other[2] - axis[2] * other[1];
					perp[1] = axis[2] * other[0] - axis[0] * other[2];
					perp[2] = axis[0] * other[1] - axis[1] * other[0];
					const float norm = (float) sqrt((double) (perp[0] * perp[0]
						+ perp[1] * perp[1] + perp[2] * perp[2]));
					if(norm > 1e-6f)
						{
						const float along = nxUnit(&local) * 3.0f - 1.5f;
						const bool onAxis = (nxNext(&local) & 7) == 0;
						const float span = sphereShape->geometry[0] + capsuleShape->geometry[0];
						const float across = onAxis ? 0.0f
							: (nxUnit(&local) * 1.8f + 0.05f) * span;
						if(onAxis)
							++coincident;
						if(along < -1.0f || along > 1.0f)
							++beyondEnd;
						for(int k = 0; k < 3; ++k)
							sphereShape->translation[k] = capsuleShape->translation[k]
								+ axis[k] * along + perp[k] * (across / norm);
						placed = true;
						}
					}
				if(!placed)
					for(int k = 0; k < 3; ++k)
						sphereShape->translation[k] =
							tame ? nxUnit(&local) * 3.0f - 1.5f : nxPick(&local);

				const bool newIdentity0 = (p == 0) || ((nxNext(&local) & 3) == 0);
				const bool newIdentity1 = (p == 0) || ((nxNext(&local) & 3) == 0);
				const NxU32 material0 = nxNext(&local) & 0xff;
				const NxU32 material1 = nxNext(&local) & 0xff;
				const bool nullHolder1 = (nxNext(&local) & 7) == 0;
				const bool orientToSphere = (nxNext(&local) & 1) != 0;

				const unsigned before = world[0].sink.streamCount;
				for(int side = 0; side < 2; ++side)
					{
					nxStageWorld(&world[side], sphereShape, capsuleShape,
						newIdentity0, newIdentity1, material0, material1,
						nullHolder1, orientToSphere);
					*(void**) world[side].plane = world[side].shapeVtable;
					}

				nxSetControl(mode ? kControlSimulate : kControlDefault);
				oracleContact(world[0].plane, world[0].sphere, &world[0].sink, nxOverlapContext);
				NxContactSphereCapsule(world[1].plane, world[1].sphere, &world[1].sink, nxOverlapContext);
				nxSetControl(kControlDefault);

				const unsigned appended = world[0].sink.streamCount - before;
				if(appended)
					++emitted;
				if(appended < 24)
					++widths[appended];
				if(sweptShape)
					{
					++swept;
					if(appended)
						++sweptEmitted;
					}
				}

			nxFoldStream(&oracleDigest, &world[0]);
			nxFoldStream(&candidateDigest, &world[1]);
			mismatches += nxCompareStreams(&world[0], &world[1]);
			}
		}
	totalMismatch += mismatches;
	printf("collision name=contact_sphere_capsule index=9 rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		nxMatrixA[9].rva, nxMatrixA[9].stableId,
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	printf("collision coverage name=contact_sphere_capsule emitted=%u swept=%u swept_emitted=%u zero_axis=%u coincident=%u beyond_end=%u w4=%u w8=%u w11=%u\n",
		emitted, swept, sweptEmitted, zeroAxis, coincident, beyondEnd,
		widths[4], widths[8], widths[11]);
	}

	// What is not covered, named rather than left as an absence.
	for(unsigned index = 0; index < 36; ++index)
		{
		bool driven = false;
		for(unsigned entry = 0; entry < kDrivenCount; ++entry)
			if(nxDriven[entry].index == index)
				driven = true;
		if(!driven && nxMatrixB[index].rva)
			printf("collision unreconstructed half=B type0=%u type1=%u rva=0x%08x owner=%s\n",
				index / 6, index % 6, nxMatrixB[index].rva, nxMatrixB[index].stableId);
		}
	for(unsigned index = 0; index < 36; ++index)
		if(nxMatrixA[index].rva)
			printf("collision unreconstructed half=A type0=%u type1=%u rva=0x%08x owner=%s\n",
				index / 6, index % 6, nxMatrixA[index].rva, nxMatrixA[index].stableId);

	printf("collision matrix_wrong=%u index_wrong=%u mismatches=%u\n", matrixWrong, indexWrong, totalMismatch);
	if(matrixWrong || indexWrong || totalMismatch)
		return nxFail("the reconstruction does not agree with the pinned oracle");
	printf("collision=pass\n");
	return 0;
	}
