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

typedef bool(__cdecl* NxOracleOverlapFn)(const NxCollisionShape*, const NxCollisionShape*);
typedef void*(__thiscall* NxMatrixCtorFn)(void*);

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
		unsigned char fromOracle = oracle(a, b) ? 1 : 0;
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

	// The index rule itself, over every ordered pair rather than over the ones
	// the table happens to fill.
	unsigned indexWrong = 0;
	for(unsigned t0 = 0; t0 < NX_COLLISION_SHAPE_TYPES; ++t0)
		for(unsigned t1 = t0; t1 < NX_COLLISION_SHAPE_TYPES; ++t1)
			{
			if(NxCollisionPairIndex(t0, t1) != t0 * 6 + t1)
				++indexWrong;
			if(NxCollisionPairIndex(t0, t1) != NxCollisionPairIndex(t0, t1))
				++indexWrong;
			}
	printf("matrix index_rule wrong=%u\n", indexWrong);

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
				bool ordered = oracle(shape0, shape1);
				bool swapped = oracle(shape1, shape0);
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
