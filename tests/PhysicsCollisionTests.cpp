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
#include <malloc.h>
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
// phys_fn_001690, the segment/segment squared distance. A Phase 2 row, reached
// by direct call from both capsule/capsule entries -- 0x0003dc68 in matrix A's
// and from matrix B's at 0x0003d890.
static const unsigned kSegmentDistanceRva = 0x00033e80;
// phys_fn_001010, slot 5 on a CAPSULE shape, from 0x00106b20 + 0x14.
static const unsigned kCapsuleRaycastRva = 0x00022480;
// phys_fn_001281, the four-byte `mov eax,[ecx+4]; ret` both sphere entries of
// matrix A use to reach Shape+0x04.
static const unsigned kShapeOwnerRva = 0x000257a0;
// phys_fn_002266, the continuous-CD guard both sphere entries call when one of
// the two shapes has a null `owner->[8]`.
static const unsigned kContinuousCdRva = 0x00056650;
// phys_fn_000429, PhysicsSDK::getParameter -- a Phase 2 row, driven here only
// to read the oracle's own live parameter array back out of it.
static const unsigned kGetParameterRva = 0x0000dc00;
// .data 0x00123b18, the live parameter array phys_fn_000472 fills from the
// defaults at 0x001238b8 with `rep movsd` at 0x0000e72c. 0x3b entries.
static const unsigned kParameterArrayRva = 0x00123b18;
static const unsigned kParameterCount = 0x3b;
// NX_CONTINUOUS_CD's index, from the `push 0xb` at 0x00056656. The block below
// measures that the guard reads this one and no other rather than restating it.
static const unsigned kContinuousCdParameter = 11;
// phys_fn_001917, the sphere/box contact geometry.
static const unsigned kSphereBoxContactRva = 0x00049f00;
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

// The matrix A slots this target drives. It exists because the summary at the
// end reported every non-null A slot as unreconstructed whatever was driven, so
// five closed entries were being listed as absent -- a line that could not
// change and therefore could not be read.
static const unsigned nxDrivenContact[] =
	{
	0 * 6 + 1,		// phys_fn_001901
	0 * 6 + 2,		// phys_fn_001883
	0 * 6 + 3,		// phys_fn_001891
	1 * 6 + 1,		// phys_fn_001933
	1 * 6 + 2,		// phys_fn_001919
	1 * 6 + 3,		// phys_fn_001923
	3 * 6 + 3		// phys_fn_001775
	};
static const unsigned kDrivenContactCount = sizeof(nxDrivenContact) / sizeof(nxDrivenContact[0]);

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

// True for anything that is not an infinity and not a NaN. Used only to decide
// whether a generator may aim, never to decide an answer: a generator that does
// arithmetic on a value it also allows to be non-finite makes its own inputs
// depend on which operand the compiler put first, which is how the seventeenth
// gate defect in this program made an oracle-side digest move on a recompile of
// the candidate.
static bool nxFinite(float value)
	{
	unsigned bits;
	memcpy(&bits, &value, 4);
	return (bits & 0x7f800000u) != 0x7f800000u;
	}

// The stack the next callee is about to use, filled with one repeated dword.
//
// phys_fn_001775's swept path hands the emitter `hit.worldNormal`, and
// phys_fn_001010 -- the only slot-5 row a capsule pair can reach -- never writes
// it, so the three words that end up in the contact stream are whatever the
// caller left at that address. Neither implementation can make that
// deterministic; the harness can, from outside, by leaving a known pattern
// where both frames will land. `_alloca` reserves the region, the loop fills it,
// and the reservation is given back when this function returns, so the bytes are
// still there when the callee's frame is laid over them.
//
// One repeated dword rather than a varied fill, deliberately: an unwritten slot
// then reads the same value whatever offset each compiler chose for the hit,
// which is the only property available here. It makes the read reproducible; it
// does not prove the two frames agree about where the hit is, and nothing in
// this harness could.
static void nxSeedFrameBelow(NxU32 pattern)
	{
	volatile NxU32* block = (volatile NxU32*) _alloca(4096);
	for(int i = 0; i < 1024; ++i)
		block[i] = pattern;
	}

// A NaN in the 80-bit spill and in a 32-bit parameter. Returns true if the
// value was one, and rewrites it to a single pattern so that NaN compares equal
// to NaN and to nothing else. An infinity is left alone: its sign is a fact the
// row does reproduce.
static bool nxCanonicalWide(unsigned char wide[10])
	{
	const bool infiniteOrNan = (wide[9] & 0x7f) == 0x7f && wide[8] == 0xff;
	bool significand = false;
	for(int i = 0; i < 8; ++i)
		if(wide[i] != (i == 7 ? 0x80 : 0x00))
			significand = true;
	if(!infiniteOrNan || !significand)
		return false;
	memset(wide, 0, 8);
	wide[7] = 0xc0;
	wide[8] = 0xff;
	wide[9] = 0x7f;
	return true;
	}

static bool nxCanonicalNarrow(NxU32* word)
	{
	if((*word & 0x7f800000u) != 0x7f800000u || (*word & 0x007fffffu) == 0)
		return false;
	*word = 0x7fc00000u;
	return true;
	}

// phys_fn_001690 leaves its result in st(0) at whatever precision the control
// word says, and its one decoded caller uses it both ways: `fst` a narrowed
// copy into its own frame at 0x0003dc6d and then `fcompp` the wide register
// against the squared radius sum. Taking the result as a `double` would round
// the register to 53 bits before anything here could look at it, so both sides
// go through this thunk and the register is spilled with `fstp tbyte`. All ten
// bytes are compared and digested.
//
// The compiler assumes a call inside an __asm block destroys eax, ebx, ecx and
// edx, and the callee is __cdecl, so nothing else has to be saved. The x87
// stack is balanced across the block -- the call pushes one value and the
// `fstp` pops it -- which is what the compiler's own model of it expects.
static void nxCallSegmentDistance(const void* fn, const NxSegment* segment0,
	const NxSegment* segment1, NxReal* parameter0, NxReal* parameter1,
	unsigned char wide[10])
	{
	__asm
		{
		mov  eax, parameter1
		push eax
		mov  eax, parameter0
		push eax
		mov  eax, segment1
		push eax
		mov  eax, segment0
		push eax
		mov  eax, fn
		call eax
		add  esp, 16
		mov  eax, wide
		fstp tbyte ptr [eax]
		}
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
//
// `nullHolder0` was added for the two sphere entries of matrix A, which read
// BOTH owners' `+0x08` and branch on either being null (0x0004b874 and
// 0x0004b88f). Only shape1's had ever been driven, so the first of those two
// branches had never been taken. Both null at once is not offered, because the
// emitter's material fallback would then dereference a null holder -- the
// oracle does the same and it is a state no caller can be in.
static void nxStageWorld(NxContactWorld* world, const NxCollisionShape* planeShape,
	const NxCollisionShape* sphereShape, bool newIdentity0, bool newIdentity1,
	NxU32 material0, NxU32 material1, bool nullHolder0, bool nullHolder1,
	bool orientToSphere)
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
		const bool nullHolder = which ? nullHolder1 : nullHolder0;
		*(unsigned char**) (owner + 8) = nullHolder ? 0 : holder;
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
						false, nullHolder1, orientToSphere);

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
						material0, material1, false, nullHolder1, orientToSphere);

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
						false, nullHolder1, orientToSphere);
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
						false, nullHolder1, orientToSphere);
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

	// -----------------------------------------------------------------------
	// Matrix A [PLANE][BOX], the one entry that inlines the emitter.
	//
	// Three of the four differences between the inlined copy and phys_fn_000873
	// need a generator that reaches them, and none is reached by a random pair:
	//
	//   header_rewrite -- the inlined copy has no header predicate, so driving
	//     the *same* pair twice in a row into one sink makes it write a second
	//     header where the emitter would have written none. That needs a
	//     sequence that repeats a pair without changing either identity.
	//   zero_normal -- the only case where the missing normal predicate shows,
	//     since the header it just wrote cleared the cache.
	//   six -- the contact cap. Eight corners below the plane, which needs the
	//     box wholly under it.
	//
	// The fourth, the always-zero feature half, is structural: no matrix A entry
	// reachable today passes anything but 0xffff, so the fifth-word branch is
	// dead in both copies and the emitter block drives it directly instead.
	{
	typedef void(__cdecl* NxOracleContactFn)(const NxCollisionShape*, const NxCollisionShape*,
		NxContactSink*, void*);
	NxOracleContactFn oracleContact = (NxOracleContactFn) (base + nxMatrixA[2].rva);

	static NxContactWorld world[2];

	NxDigest oracleDigest, candidateDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	unsigned mismatches = 0;
	unsigned perMode[2] = { 0, 0 };
	unsigned emitted = 0;
	unsigned headerRewrite = 0;
	unsigned zeroNormal = 0;
	unsigned belowPlane = 0;
	unsigned negatedPath = 0;
	unsigned contactCounts[10];
	unsigned widths[48];
	memset(contactCounts, 0, sizeof(contactCounts));
	memset(widths, 0, sizeof(widths));
	unsigned state = 0x1c7a4e05u;

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
			static unsigned char boxStorage[kShapeBytes];
			NxCollisionShape* planeShape = (NxCollisionShape*) planeStorage;
			NxCollisionShape* boxShape = (NxCollisionShape*) boxStorage;
			for(unsigned p = 0; p < pairs; ++p)
				{
				// A repeated pair: the same shapes and the same identities as
				// the previous one, which is the only way to reach the missing
				// header predicate.
				const bool repeat = p > 0 && (nxNext(&local) & 1) != 0;
				bool zeroNormalPair = false;
				if(!repeat)
					{
					const bool tame = (nxNext(&local) & 3) != 0;
					nxIdentity(planeShape);
					nxIdentity(boxShape);
					nxFillGeometry(&local, planeShape, 0, tame);
					nxFillGeometry(&local, boxShape, 2, tame);
					zeroNormalPair = tame && (nxNext(&local) & 15) == 0;
					if(zeroNormalPair)
						{
						planeShape->geometry[0] = 0.0f;
						planeShape->geometry[1] = 0.0f;
						planeShape->geometry[2] = 0.0f;
						}
					nxRandomRotation(&local, boxShape);
					// The box placed relative to the plane along its own normal,
					// so "no corner below", "some below" and "all eight below"
					// are all reached. Gated on the normal being finite, since
					// the placement multiplies it.
					bool finite = tame;
					for(int k = 0; k < 3; ++k)
						if(!nxFinite(planeShape->geometry[k]))
							finite = false;
					if(!nxFinite(planeShape->geometry[3]))
						finite = false;
					const float reach = boxShape->geometry[1] + boxShape->geometry[2]
						+ boxShape->geometry[3];
					const bool sink6 = finite && (nxNext(&local) & 3) == 0;
					if(sink6)
						++belowPlane;
					if(finite)
						{
						const float offset = sink6 ? -(reach + nxUnit(&local) * 2.0f)
							: (nxUnit(&local) * 2.4f - 1.2f) * reach;
						for(int k = 0; k < 3; ++k)
							boxShape->translation[k] =
								planeShape->geometry[k] * (offset - planeShape->geometry[3]);
						}
					else
						for(int k = 0; k < 3; ++k)
							boxShape->translation[k] = nxPick(&local);
					}
				if(zeroNormalPair)
					++zeroNormal;

				const bool newIdentity0 = !repeat && ((p == 0) || ((nxNext(&local) & 3) == 0));
				const bool newIdentity1 = !repeat && ((p == 0) || ((nxNext(&local) & 3) == 0));
				const NxU32 material0 = nxNext(&local) & 0xff;
				const NxU32 material1 = nxNext(&local) & 0xff;
				const bool nullHolder1 = (nxNext(&local) & 7) == 0;
				const bool orientToBox = (nxNext(&local) & 1) != 0;
				if(orientToBox)
					++negatedPath;
				if(repeat)
					++headerRewrite;

				const unsigned before = world[0].sink.streamCount;
				const unsigned contactsBefore = world[0].sink.contactCount;
				for(int side = 0; side < 2; ++side)
					nxStageWorld(&world[side], planeShape, boxShape,
						newIdentity0, newIdentity1, material0, material1,
						false, nullHolder1, !orientToBox);

				nxSetControl(mode ? kControlSimulate : kControlDefault);
				oracleContact(world[0].plane, world[0].sphere, &world[0].sink, nxOverlapContext);
				NxContactPlaneBox(world[1].plane, world[1].sphere, &world[1].sink, nxOverlapContext);
				nxSetControl(kControlDefault);

				const unsigned appended = world[0].sink.streamCount - before;
				const unsigned contacts = world[0].sink.contactCount - contactsBefore;
				if(appended)
					++emitted;
				if(appended < 48)
					++widths[appended];
				if(contacts < 10)
					++contactCounts[contacts];
				}

			nxFoldStream(&oracleDigest, &world[0]);
			nxFoldStream(&candidateDigest, &world[1]);
			const unsigned differing = nxCompareStreams(&world[0], &world[1]);
			mismatches += differing;
			perMode[mode] += differing;
			}
		}
	totalMismatch += perMode[0];
	printf("collision name=contact_plane_box index=2 rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		nxMatrixA[2].rva, nxMatrixA[2].stableId,
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	// c6 is the cap: eight corners below the plane and the oracle stops at six.
	// header_rewrite counts the repeated pairs that reach the missing header
	// predicate, zero_normal the plane normals that reach the missing normal
	// predicate, and below_plane the placements aimed at the cap.
	printf("collision coverage name=contact_plane_box emitted=%u header_rewrite=%u zero_normal=%u below_plane=%u negated=%u c1=%u c2=%u c4=%u c6=%u c7=%u w11=%u w15=%u w31=%u default_mismatches=%u simulate_mismatches=%u\n",
		emitted, headerRewrite, zeroNormal, belowPlane, negatedPath,
		contactCounts[1], contactCounts[2], contactCounts[4], contactCounts[6],
		contactCounts[7], widths[11], widths[15], widths[31], perMode[0], perMode[1]);
	}

	// -----------------------------------------------------------------------
	// phys_fn_001010, driven at its own address.
	//
	// The entry cannot reach two of the things this row does. It always passes a
	// distance limit equal to the segment length it just measured, so the limit
	// gate is never anything but an exact fit; and it can only ever produce the
	// two-root case through capsule geometry, where a direct drive reaches the
	// one-root and zero-root returns of NxRayCapsuleIntersect as well.
	//
	// The hit is 0xcd-poisoned before every call and every word of it compared,
	// which is what turns "it writes no normal" from a reading of the
	// disassembly into a measurement: `untouched_normal` counts the calls after
	// which all three worldNormal words are still poison on the oracle side.
	{
	typedef const void*(__thiscall* NxOracleRaycastFn)(const void*, const NxRay*, NxReal,
		NxU32, NxU32, NxRaycastHit*);
	NxOracleRaycastFn oracleRaycast = (NxOracleRaycastFn) (base + kCapsuleRaycastRva);

	NxDigest oracleDigest, candidateDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	unsigned mismatches = 0;
	unsigned perMode[2] = { 0, 0 };
	unsigned hits = 0;
	unsigned aimedRays = 0;
	unsigned untouchedNormal = 0;
	unsigned zeroAxis = 0;
	unsigned state = 0x3d1f77a9u;

	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		const bool tame = (nxNext(&state) & 3) != 0;
		nxIdentity(shape0);
		nxFillGeometry(&state, shape0, 3, tame);
		for(int k = 0; k < 3; ++k)
			shape0->translation[k] = tame ? nxUnit(&state) * 4.0f - 2.0f : nxPick(&state);
		shape0->collisionObject = kFakeCollisionObject;
		const unsigned axisMode = nxNext(&state) % 4;
		if(axisMode == 0)
			nxRandomRotation(&state, shape0);
		else if(axisMode == 3)
			{
			memset(shape0->rotation, 0, sizeof(shape0->rotation));
			++zeroAxis;
			}

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
			const float along = nxUnit(&state) * 4.0f + 0.5f;
			const float across = (nxUnit(&state) * 2.0f - 1.0f) * shape0->geometry[0] * 1.4f;
			for(int k = 0; k < 3; ++k)
				(&ray.orig.x)[k] = shape0->translation[k] - (&ray.dir.x)[k] * along
					+ ((k + 1) % 3 == 0 ? across : -across) * 0.5f;
			maxDistance = along * (0.2f + nxUnit(&state) * 1.6f);
			}
		else
			{
			for(int k = 0; k < 3; ++k)
				{
				(&ray.orig.x)[k] = tame ? nxUnit(&state) * 4.0f - 2.0f : nxPick(&state);
				(&ray.dir.x)[k] = tame ? nxUnit(&state) * 2.0f - 1.0f : nxPick(&state);
				}
			maxDistance = tame ? nxUnit(&state) * 8.0f : nxPick(&state);
			}

		// Every hint flag combination, because this row ignores all of them and
		// that is the thing worth measuring.
		NxU32 hintFlags = nxNext(&state) & 0xffu;

		for(int mode = 0; mode < 2; ++mode)
			{
			NxRaycastHit hit[2];
			memset(hit, 0xcd, sizeof(hit));

			nxSetControl(mode ? kControlSimulate : kControlDefault);
			const unsigned char fromOracle =
				oracleRaycast(shape0, &ray, maxDistance, 0, hintFlags, &hit[0]) ? 1 : 0;
			const unsigned char fromCandidate =
				NxShapeRaycastCapsule(shape0, 0, &ray, maxDistance, 0, hintFlags, &hit[1]) ? 1 : 0;
			nxSetControl(kControlDefault);

			if(fromOracle)
				{
				++hits;
				NxU32 normalWords[3];
				memcpy(normalWords, &hit[0].worldNormal, 12);
				if(normalWords[0] == 0xcdcdcdcdu && normalWords[1] == 0xcdcdcdcdu
					&& normalWords[2] == 0xcdcdcdcdu)
					++untouchedNormal;
				}
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
	totalMismatch += perMode[0];
	printf("collision name=shape_raycast_capsule index=- rva=0x%08x owner=phys_fn_001010 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kCapsuleRaycastRva, oracleDigest.checks, oracleDigest.state,
		candidateDigest.state, mismatches);
	// untouched_normal equalling hits is the measurement, not a coincidence:
	// every hint flag combination is driven and the oracle never writes the
	// field on any of them.
	printf("collision coverage name=shape_raycast_capsule hits=%u untouched_normal=%u aimed=%u zero_axis=%u default_mismatches=%u simulate_mismatches=%u\n",
		hits, untouchedNormal, aimedRays, zeroAxis, perMode[0], perMode[1]);
	}

	// -----------------------------------------------------------------------
	// Matrix A [CAPSULE][CAPSULE].
	//
	// Two halves, and only one of them is a function of its arguments.
	//
	// The swept half hands the emitter hit.worldNormal, which phys_fn_001010
	// never writes, so the three words that reach the contact stream are
	// whatever the caller's stack left at that address. There is no way to make
	// that deterministic from inside either implementation, so the harness makes
	// it deterministic from outside: nxSeedFrameBelow fills the stack the callee
	// is about to use with one repeated dword before *each* of the two calls, so
	// an unwritten slot reads the same value on both sides whatever offset each
	// compiler put the hit at. That is a real limitation and it is stated rather
	// than hidden -- a uniform seed makes the read reproducible; it does not
	// prove the two frames put the hit in the same place, and it could not.
	//
	// What it does prove is that the read happens: the seed alternates between
	// two patterns and `seeded_normal` counts the swept contacts whose emitted
	// normal words are the pattern that was in force. A reconstruction that
	// computed a normal instead would not track it.
	//
	// The flag byte is driven independently on the two capsules because the
	// branch is an OR and the ray selector is shape1's bit alone: setting both
	// together drives three of the four combinations into one branch and cannot
	// tell the OR from an AND, which is the defect four of the emitter's own
	// behaviours had.
	{
	typedef void(__cdecl* NxOracleContactFn)(const NxCollisionShape*, const NxCollisionShape*,
		NxContactSink*, void*);
	NxOracleContactFn oracleContact = (NxOracleContactFn) (base + nxMatrixA[21].rva);

	static NxContactWorld world[2];
	world[0].shapeVtable[5] = (void*) (base + kCapsuleRaycastRva);
	world[1].shapeVtable[5] = (void*) &NxShapeRaycastCapsule;

	NxDigest oracleDigest, candidateDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	unsigned mismatches = 0;
	unsigned perMode[2] = { 0, 0 };
	unsigned emitted = 0;
	unsigned flagPairs[4] = { 0, 0, 0, 0 };
	unsigned sweptEmitted = 0;
	unsigned seededNormal = 0;
	unsigned parallelAxes = 0;
	unsigned zeroAxis = 0;
	unsigned coincident = 0;
	unsigned beyondEnd = 0;
	unsigned contactCounts[8];
	unsigned widths[40];
	memset(contactCounts, 0, sizeof(contactCounts));
	memset(widths, 0, sizeof(widths));
	unsigned state = 0x6b4de20fu;

	for(unsigned i = 0; i < kContactIterations; ++i)
		{
		const unsigned sequenceSeed = nxNext(&state);
		const unsigned pairs = 1 + (nxNext(&state) % 4);
		// Two patterns, alternating, so the stream can be asked whether the
		// normal really did come out of the seed.
		const NxU32 seed = (i & 1) ? 0xcdcdcdcdu : 0xa5a5a5a5u;

		for(int mode = 0; mode < 2; ++mode)
			{
			for(int side = 0; side < 2; ++side)
				nxResetWorld(&world[side]);

			unsigned local = sequenceSeed;
			for(unsigned p = 0; p < pairs; ++p)
				{
				static unsigned char firstStorage[kShapeBytes];
				static unsigned char secondStorage[kShapeBytes];
				NxCollisionShape* first = (NxCollisionShape*) firstStorage;
				NxCollisionShape* second = (NxCollisionShape*) secondStorage;
				const bool tame = (nxNext(&local) & 3) != 0;
				nxIdentity(first);
				nxIdentity(second);
				nxFillGeometry(&local, first, 3, tame);
				nxFillGeometry(&local, second, 3, tame);
				for(int k = 0; k < 3; ++k)
					first->translation[k] = tame ? nxUnit(&local) * 3.0f - 1.5f : nxPick(&local);

				const unsigned axisMode = nxNext(&local) % 4;
				if(axisMode == 0)
					nxRandomRotation(&local, first);
				else if(axisMode == 2)
					{
					first->rotation[1] = nxUnit(&local) * 4.0f - 2.0f;
					first->rotation[4] = nxUnit(&local) * 4.0f - 2.0f;
					first->rotation[7] = nxUnit(&local) * 4.0f - 2.0f;
					}
				else if(axisMode == 3)
					memset(first->rotation, 0, sizeof(first->rotation));

				// The second capsule's axis, and the branch that decides which
				// of the two algorithms in the non-swept half runs. Parallel and
				// anti-parallel are the only inputs that reach the endpoint clip
				// at all -- two independent rotations essentially never put the
				// axis dot product above 0.9998f -- and the sign is drawn so
				// both are reached.
				const bool parallel = tame && (nxNext(&local) & 1) != 0;
				bool axisPlaced = false;
				if(parallel)
					{
					float axis[3];
					bool finite = true;
					for(int k = 0; k < 3; ++k)
						{
						axis[k] = first->rotation[1 + k * 3];
						if(!nxFinite(axis[k]))
							finite = false;
						}
					if(finite)
						{
						// A small perturbation, so pairs land either side of the
						// threshold rather than all above it.
						const float sign = (nxNext(&local) & 1) ? 1.0f : -1.0f;
						const float wobble = nxUnit(&local) * 0.05f;
						for(int k = 0; k < 3; ++k)
							second->rotation[1 + k * 3] =
								axis[k] * sign + (nxUnit(&local) * 2.0f - 1.0f) * wobble;
						axisPlaced = true;
						++parallelAxes;
						}
					}
				if(!axisPlaced)
					{
					const unsigned secondMode = nxNext(&local) % 3;
					if(secondMode == 0)
						nxRandomRotation(&local, second);
					else if(secondMode == 2)
						memset(second->rotation, 0, sizeof(second->rotation));
					}

				// The endpoint clip can only produce four contacts when each
				// capsule's two endpoints both project inside the other's
				// parameter range, and that needs the two half heights to be
				// close as well as the axes to be. Independent draws never
				// arrange it, so a third of the parallel pairs get matched
				// lengths and a small offset along the axis.
				const bool matched = axisPlaced && (nxNext(&local) % 3) == 0;
				if(matched)
					second->geometry[1] = first->geometry[1];

				const bool degenerate = (nxNext(&local) & 7) == 0;
				if(degenerate)
					first->geometry[1] = 0.0f;
				const bool degenerate1 = (nxNext(&local) & 7) == 0;
				if(degenerate1)
					second->geometry[1] = 0.0f;
				if(degenerate || degenerate1 || axisMode == 3)
					++zeroAxis;

				// Independent flag words. The low bit is what the kernel reads
				// and the other 31 are randomised, so "only bit 0 matters" is
				// measured rather than assumed.
				NxU32 flagWord0 = nxNext(&local);
				NxU32 flagWord1 = nxNext(&local);
				const unsigned combination = nxNext(&local) & 3u;
				flagWord0 = (combination & 1) ? (flagWord0 | 1u) : (flagWord0 & ~1u);
				flagWord1 = (combination & 2) ? (flagWord1 | 1u) : (flagWord1 & ~1u);
				memcpy(&first->geometry[2], &flagWord0, 4);
				memcpy(&second->geometry[2], &flagWord1, 4);
				++flagPairs[combination];

				// The second capsule placed in the first's frame: `along` in half
				// heights so |along| > 1 is past an endpoint, `across` in units
				// of the two radii with zero putting the axes coincident. Gated
				// on the first capsule's axis being finite, so no aim ever
				// multiplies a value that could be a NaN.
				bool placed = false;
				if(tame && (nxNext(&local) & 3) != 0)
					{
					const float halfHeight = first->geometry[1];
					float axis[3];
					bool finite = true;
					for(int k = 0; k < 3; ++k)
						{
						axis[k] = first->rotation[1 + k * 3] * halfHeight;
						if(!nxFinite(axis[k]) || !nxFinite(first->translation[k]))
							finite = false;
						}
					float other[3] = { 0.0f, 0.0f, 0.0f };
					other[nxNext(&local) % 3] = 1.0f;
					float perp[3];
					perp[0] = axis[1] * other[2] - axis[2] * other[1];
					perp[1] = axis[2] * other[0] - axis[0] * other[2];
					perp[2] = axis[0] * other[1] - axis[1] * other[0];
					const float norm = (float) sqrt((double) (perp[0] * perp[0]
						+ perp[1] * perp[1] + perp[2] * perp[2]));
					if(finite && norm > 1e-6f)
						{
						// Four contacts need the two axes co-located to within
						// the 0.1% parameter tolerance, which no random offset
						// reaches, so a quarter of the matched pairs are placed
						// exactly.
						const bool aligned = matched && (nxNext(&local) & 3) == 0;
						const float along = aligned ? 0.0f
							: matched ? nxUnit(&local) * 0.7f - 0.35f
							: nxUnit(&local) * 3.2f - 1.6f;
						const bool onAxis = (nxNext(&local) & 7) == 0;
						const float span = first->geometry[0] + second->geometry[0];
						const float across = onAxis ? 0.0f
							: (matched ? nxUnit(&local) * 0.9f
								: nxUnit(&local) * 1.9f + 0.02f) * span;
						if(onAxis)
							++coincident;
						if(along < -1.0f || along > 1.0f)
							++beyondEnd;
						for(int k = 0; k < 3; ++k)
							second->translation[k] = first->translation[k]
								+ axis[k] * along + perp[k] * (across / norm);
						placed = true;
						}
					}
				if(!placed)
					for(int k = 0; k < 3; ++k)
						second->translation[k] =
							tame ? nxUnit(&local) * 3.0f - 1.5f : nxPick(&local);

				const bool newIdentity0 = (p == 0) || ((nxNext(&local) & 3) == 0);
				const bool newIdentity1 = (p == 0) || ((nxNext(&local) & 3) == 0);
				const NxU32 material0 = nxNext(&local) & 0xff;
				const NxU32 material1 = nxNext(&local) & 0xff;
				const bool nullHolder1 = (nxNext(&local) & 7) == 0;
				const bool orientToSecond = (nxNext(&local) & 1) != 0;

				const unsigned before = world[0].sink.streamCount;
				const unsigned contactsBefore = world[0].sink.contactCount;
				for(int side = 0; side < 2; ++side)
					{
					nxStageWorld(&world[side], first, second,
						newIdentity0, newIdentity1, material0, material1,
						false, nullHolder1, orientToSecond);
					// Both shapes need one: the receiver is whichever capsule is
					// not the ray, and the kernel picks that at runtime.
					*(void**) world[side].plane = world[side].shapeVtable;
					*(void**) world[side].sphere = world[side].shapeVtable;
					}

				nxSetControl(mode ? kControlSimulate : kControlDefault);
				nxSeedFrameBelow(seed);
				oracleContact(world[0].plane, world[0].sphere, &world[0].sink, nxOverlapContext);
				nxSeedFrameBelow(seed);
				NxContactCapsuleCapsule(world[1].plane, world[1].sphere, &world[1].sink, nxOverlapContext);
				nxSetControl(kControlDefault);

				const unsigned appended = world[0].sink.streamCount - before;
				const unsigned contacts = world[0].sink.contactCount - contactsBefore;
				if(appended)
					++emitted;
				if(appended < 40)
					++widths[appended];
				if(contacts < 8)
					++contactCounts[contacts];
				if((flagWord0 | flagWord1) & 1)
					{
					if(appended)
						{
						++sweptEmitted;
						// The seed, or the seed with its sign bit flipped: the
						// emitter negates all three normal components when the
						// pair is emitted against the body the sink is not
						// oriented to.
						for(unsigned w = before; w < world[0].sink.streamCount; ++w)
							if(world[0].sink.stream[w] == seed
								|| world[0].sink.stream[w] == (seed ^ 0x80000000u))
								{ ++seededNormal; break; }
						}
					}
				}

			nxFoldStream(&oracleDigest, &world[0]);
			nxFoldStream(&candidateDigest, &world[1]);
			const unsigned differing = nxCompareStreams(&world[0], &world[1]);
			mismatches += differing;
			perMode[mode] += differing;
			}
		}
	// The default-word half gates, as it does for the other three rows that
	// carry a pinned divergence under 0x0f7f. Under 0x027f the reconstruction
	// agrees on every one of the checks below; under 0x0f7f it differs on 43,
	// and the source is the one already escalated twice -- MSVC spilling a
	// `double` to an 8-byte slot, which truncates a 64-bit significand to 53.
	// Like phys_fn_001690 and unlike the two raycast rows, this entry is only
	// ever reached from inside the simulation step, so the half that gates is
	// not the half it runs under. The count is registered so it fails if it
	// moves in either direction.
	totalMismatch += perMode[0];
	printf("collision name=contact_capsule_capsule index=21 rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		nxMatrixA[21].rva, nxMatrixA[21].stableId,
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	// seeded_normal is the measurement that the swept normal is uninitialised
	// stack: it counts swept emissions whose stream carries the dword the
	// harness had just seeded the frame with, and the seed alternates between
	// two values so a match cannot be a constant the kernel happens to write.
	// f00..f11 are the four flag combinations, driven independently.
	printf("collision coverage name=contact_capsule_capsule emitted=%u f00=%u f01=%u f10=%u f11=%u swept_emitted=%u seeded_normal=%u parallel=%u zero_axis=%u coincident=%u beyond_end=%u c1=%u c2=%u c3=%u c4=%u default_mismatches=%u simulate_mismatches=%u\n",
		emitted, flagPairs[0], flagPairs[1], flagPairs[2], flagPairs[3],
		sweptEmitted, seededNormal, parallelAxes, zeroAxis, coincident, beyondEnd,
		contactCounts[1], contactCounts[2], contactCounts[3], contactCounts[4],
		perMode[0], perMode[1]);
	}

	// -----------------------------------------------------------------------
	// phys_fn_001690 at 0x00033e80, the segment/segment squared distance.
	//
	// A PHASE 2 row, driven here because Phase 3 is what creates the
	// reachability Phase 2 said it lacked: matrix A [CAPSULE][CAPSULE] at
	// 0x0003d9d0 calls it directly at 0x0003dc68 and matrix B's capsule pair at
	// 0x0003d890 calls it too. Phase 2's ledger deferred it `homeless_shared_code`
	// with `driving_phases: [3, 4]`; this block is the proof that discharges
	// that deferral, and it is recorded on Phase 2's ledger with
	// `discharged_by_phase: 3` rather than adopted into Phase 3's.
	//
	// It is driven at its own address rather than only through a caller for the
	// usual reason and one extra: the near-parallel tree at 0x000343cf is
	// unreachable from any pair of capsule axes a random rotation produces, and
	// two of its leaves are the only places in the function where `e` is
	// recomputed rather than read from its slot.
	{
	const void* const oracleSegment = (const void*) (base + kSegmentDistanceRva);
	const void* const candidateSegment = (const void*) &NxSegmentSegmentSquareDistance;

	NxDigest oracleDigest, candidateDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	unsigned mismatches = 0;
	unsigned perMode[2] = { 0, 0 };
	unsigned parallelPairs = 0;
	unsigned degeneratePairs = 0;
	unsigned nullParameters = 0;
	unsigned interiorPairs = 0;
	unsigned clampedS = 0;
	unsigned clampedT = 0;
	unsigned nonFinite = 0;
	unsigned canonicalised = 0;
	unsigned state = 0x51e6d0a7u;

	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		const bool tame = (nxNext(&state) & 3) != 0;
		const unsigned shape = nxNext(&state) % 5;

		NxSegment segment[2];
		float* const words0 = &segment[0].p0.x;
		float* const words1 = &segment[1].p0.x;
		for(int k = 0; k < 6; ++k)
			{
			words0[k] = tame ? nxUnit(&state) * 6.0f - 3.0f : nxPick(&state);
			words1[k] = tame ? nxUnit(&state) * 6.0f - 3.0f : nxPick(&state);
			}

		// Every aim below multiplies segment0's own direction, so every one is
		// gated on that direction being finite. A generator that does
		// arithmetic on a value it also allows to be non-finite makes the
		// *inputs* depend on which operand the compiler put first, and the
		// oracle-side digest then moves on a recompile of the candidate.
		float direction[3];
		bool aimable = tame;
		for(int k = 0; k < 3; ++k)
			{
			direction[k] = segment[0].p1[k] - segment[0].p0[k];
			if(!nxFinite(direction[k]) || !nxFinite(segment[0].p0[k]))
				aimable = false;
			}

		if(shape == 1 && aimable)
			{
			// Near-parallel: the only inputs that reach the second tree at
			// 0x000343cf at all. The scale carries a sign so anti-parallel
			// axes are driven as well, and the jitter is small enough to keep
			// |ac - b^2| under the 1e-5f epsilon for most draws and large
			// enough to cross it for some.
			++parallelPairs;
			const float scale = (nxUnit(&state) * 1.8f + 0.2f)
				* ((nxNext(&state) & 1) ? 1.0f : -1.0f);
			const float jitter = nxUnit(&state) * 4e-3f;
			for(int k = 0; k < 3; ++k)
				{
				segment[1].p0[k] = segment[0].p0[k] + (nxUnit(&state) * 2.0f - 1.0f);
				segment[1].p1[k] = segment[1].p0[k] + direction[k] * scale
					+ (nxUnit(&state) * 2.0f - 1.0f) * jitter;
				}
			}
		else if(shape == 2)
			{
			// A zero-length segment on either side or both. `a` or `c` is then
			// zero and the leaves that divide by it are reached with a zero
			// divisor, which is a state the geometry really can be in -- a
			// capsule with a zero half height builds exactly this.
			++degeneratePairs;
			const unsigned which = nxNext(&state) % 3;
			if(which != 1)
				for(int k = 0; k < 3; ++k)
					segment[0].p1[k] = segment[0].p0[k];
			if(which != 0)
				for(int k = 0; k < 3; ++k)
					segment[1].p1[k] = segment[1].p0[k];
			}
		else if(shape == 3 && aimable)
			{
			// Aimed through a point on segment0, so the closest points land in
			// the interior of both and the leaf at 0x0003402d is reached
			// rather than a corner.
			const float along = nxUnit(&state);
			float target[3];
			for(int k = 0; k < 3; ++k)
				target[k] = segment[0].p0[k] + direction[k] * along;
			float across[3];
			across[0] = direction[1];
			across[1] = -direction[0];
			across[2] = direction[2] * 0.5f + 0.25f;
			const float offset = nxUnit(&state) * 2.0f - 1.0f;
			for(int k = 0; k < 3; ++k)
				{
				const float half = (nxUnit(&state) * 2.0f - 1.0f) * 1.5f;
				segment[1].p0[k] = target[k] + across[k] * offset - half;
				segment[1].p1[k] = target[k] + across[k] * offset + half;
				}
			}
		// shape 0 and shape 4 leave the raw draws, which is where the
		// non-finite mixture reaches this row.

		// Both output pointers are optional and the oracle tests each one, so
		// the null cases are driven -- no caller in the census passes null, so
		// nothing else can reach those two branches.
		const unsigned nulls = (nxNext(&state) % 8 == 0) ? (1 + nxNext(&state) % 3) : 0;
		if(nulls)
			++nullParameters;

		for(int mode = 0; mode < 2; ++mode)
			{
			unsigned char wide[2][10];
			NxReal parameters[2][2];
			memset(wide, 0xcd, sizeof(wide));
			memset(parameters, 0xcd, sizeof(parameters));

			nxSetControl(mode ? kControlSimulate : kControlDefault);
			nxCallSegmentDistance(oracleSegment, &segment[0], &segment[1],
				(nulls & 1) ? 0 : &parameters[0][0], (nulls & 2) ? 0 : &parameters[0][1],
				wide[0]);
			nxCallSegmentDistance(candidateSegment, &segment[0], &segment[1],
				(nulls & 1) ? 0 : &parameters[1][0], (nulls & 2) ? 0 : &parameters[1][1],
				wide[1]);
			nxSetControl(kControlDefault);

			// The classification is taken from the oracle's own answers, not
			// from anything this harness computes: the two parameters say which
			// leaf ran, and a parameter left at its 0xcd poison says the null
			// branch was taken.
			if(!nulls)
				{
				const NxReal s = parameters[0][0];
				const NxReal t = parameters[0][1];
				if(s == 0.0f || s == 1.0f)
					++clampedS;
				if(t == 0.0f || t == 1.0f)
					++clampedT;
				if(s > 0.0f && s < 1.0f && t > 0.0f && t < 1.0f)
					++interiorPairs;
				}
			// The exponent byte of the 80-bit spill: 0x7fff is an infinity or a
			// NaN whatever the significand says.
			if((wide[0][9] & 0x7f) == 0x7f && wide[0][8] == 0xff)
				++nonFinite;

			// Both sides are canonicalised before the comparison, and only for
			// NaN. Which of two NaNs an x87 instruction returns is decided by
			// the significand with ties going to the *destination* operand, and
			// no C++ names the destination of an x87 instruction: MSVC picked
			// `fsubr` where the oracle has `fsub` on the very first subtraction
			// in this row. Three spellings of the negated dot products were
			// measured and moved 3,500, 3,500 and 5,956 words here and zero on
			// finite inputs, which is what says this is the machine and not the
			// transcription. Everything else -- which leaf ran, whether a
			// result is a NaN at all, every finite value and both infinities --
			// is still compared bit for bit, and the canonicalisation count is
			// registered so a generator that stops reaching NaN is a failure
			// rather than a quieter pass.
			if(nxCanonicalWide(wide[0]) | nxCanonicalWide(wide[1]))
				++canonicalised;
			for(int byte = 0; byte < 10; ++byte)
				{
				nxDigestByte(&oracleDigest, wide[0][byte]);
				nxDigestByte(&candidateDigest, wide[1][byte]);
				if(wide[0][byte] != wide[1][byte])
					{ ++mismatches; ++perMode[mode]; }
				}
			for(int half = 0; half < 2; ++half)
				{
				NxU32 a, b;
				memcpy(&a, &parameters[0][half], 4);
				memcpy(&b, &parameters[1][half], 4);
				if(nxCanonicalNarrow(&a) | nxCanonicalNarrow(&b))
					++canonicalised;
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
	// Only the default-word half gates, and here that is a weaker statement than
	// it is for the two raycast rows: those are also reachable from a consumer
	// call, and this one is not -- phys_fn_001690 is only ever entered from
	// inside the simulation step, so 0x0f7f is the *only* word it really runs
	// under. Under 0x027f the reconstruction agrees on all 1,080,000 checks.
	// Under 0x0f7f it differs on 662, and the cause is the one this program has
	// already escalated twice: MSVC spills a `double` to an 8-byte slot, which
	// truncates a 64-bit significand to 53, and no C++ says where the spills go.
	// This row keeps three of them live across the whole region tree where
	// phys_fn_001377 kept one, which is why 662 rather than 13.
	//
	// The count is registered rather than dropped, so it fails if it moves in
	// either direction including toward zero.
	totalMismatch += perMode[0];
	printf("collision name=segment_segment index=- rva=0x%08x owner=phys_fn_001690 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kSegmentDistanceRva, oracleDigest.checks, oracleDigest.state,
		candidateDigest.state, mismatches);
	printf("collision coverage name=segment_segment parallel=%u degenerate=%u null_params=%u interior=%u clamped_s=%u clamped_t=%u non_finite=%u canonical_nan=%u default_mismatches=%u simulate_mismatches=%u\n",
		parallelPairs, degeneratePairs, nullParameters, interiorPairs, clampedS, clampedT,
		nonFinite, canonicalised, perMode[0], perMode[1]);
	}

	// -----------------------------------------------------------------------
	// phys_fn_001281 at 0x000257a0, and phys_fn_002266 at 0x00056650. Both are
	// reached from the two sphere entries below and neither is observable in
	// the contact stream, so both are driven at their own addresses instead.
	//
	// phys_fn_002266 is the continuous-collision guard. Its `true` arm --
	// NX_CONTINUOUS_CD equal to 0.0f -- returns without touching the sink, and
	// that is the whole of it that is reachable under the SDK as it ships. Its
	// other arm calls phys_fn_002264 at 0x00055eb0, which THIS PROGRAM HAS NOT
	// RECONSTRUCTED and must not: it reads a second pose at Shape+0x3c..+0x68,
	// reads and writes `sink+0xdc`..`+0xe9` where the borrowed sink layout stops
	// at 0x44, and dispatches through vtable slot 7 at 0x000562c3. So this block
	// measures three things and stops:
	//
	//   * what NX_CONTINUOUS_CD actually holds, read back out of the oracle's
	//     own array through the oracle's own accessor. That is the reachability
	//     claim for phys_fn_002264, and it is measured rather than assumed.
	//   * that the guard's `true` arm leaves a poisoned sink byte for byte
	//     untouched, over more bytes than phys_fn_002264 would write.
	//   * that the guard reads parameter 11 and no other, by setting each of the
	//     other 58 non-zero in turn and checking the answer does not move. That
	//     replaces reading `push 0xb` off the disassembly with a measurement.
	//
	// What is NOT measured is the other arm being entered. Driving it would mean
	// executing phys_fn_002264 against a vtable slot nothing has resolved, which
	// is the thing the borrowed-layout rule exists to prevent.
	{
	typedef const void* (__fastcall* NxOracleShapeOwnerFn)(const NxCollisionShape*, void*);
	typedef bool (__cdecl* NxOracleContinuousFn)(const NxCollisionShape*, const NxCollisionShape*, void*);
	typedef float (__fastcall* NxOracleGetParameterFn)(void*, void*, int);

	NxOracleShapeOwnerFn oracleOwner = (NxOracleShapeOwnerFn) (base + kShapeOwnerRva);
	NxOracleContinuousFn oracleGuard = (NxOracleContinuousFn) (base + kContinuousCdRva);
	NxOracleGetParameterFn oracleGetParameter = (NxOracleGetParameterFn) (base + kGetParameterRva);
	float* const oracleParameters = (float*) (base + kParameterArrayRva);

	NxDigest oracleDigest, candidateDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	unsigned mismatches = 0;
	unsigned ownerProbes = 0;
	unsigned guardProbes = 0;
	unsigned guardTrue = 0;
	unsigned sinkUntouched = 0;
	unsigned indexProbes = 0;
	unsigned indexWrongHere = 0;
	unsigned state = 0x2b19f74du;

	// Read the live parameter through the oracle's own getParameter, exactly as
	// phys_fn_002266 does, and print the word rather than the float so a
	// negative zero or a denormal could not read as the same thing.
	const float continuous = oracleGetParameter(0, 0, (int) kContinuousCdParameter);
	NxU32 continuousBits;
	memcpy(&continuousBits, &continuous, 4);

	// A sink far larger than the borrowed layout, so a write anywhere
	// phys_fn_002264 would reach is caught. 0x300 is well past the `+0xe9` that
	// row's own frame reaches.
	static unsigned char poisonedSink[0x300];
	static unsigned char poisonedCopy[0x300];

	for(unsigned i = 0; i < kNormalsIterations; ++i)
		{
		static unsigned char shapeStorage[2][kShapeBytes];
		static unsigned char ownerStorage[2][0x40];
		static unsigned char holderStorage[2][0x280];
		NxCollisionShape* shapes[2];
		for(int which = 0; which < 2; ++which)
			{
			shapes[which] = (NxCollisionShape*) shapeStorage[which];
			nxIdentity(shapes[which]);
			nxFillGeometry(&state, shapes[which], 1, true);
			memset(ownerStorage[which], 0, sizeof(ownerStorage[which]));
			memset(holderStorage[which], 0, sizeof(holderStorage[which]));
			shapes[which]->owner = ownerStorage[which];
			}
		// One side static in each iteration, alternating, so the accessor is
		// asked about both an owner that carries a holder and one that does not.
		const unsigned staticSide = nxNext(&state) & 1;
		*(unsigned char**) (ownerStorage[staticSide] + 8) = 0;
		*(unsigned char**) (ownerStorage[1 - staticSide] + 8) = holderStorage[1 - staticSide];

		for(int which = 0; which < 2; ++which)
			{
			const void* fromOracle = oracleOwner(shapes[which], 0);
			const void* fromCandidate = NxShapeOwner(shapes[which], 0);
			++ownerProbes;
			// The answer is an address, so what is digested is whether it is the
			// field -- not the address itself, which differs per run.
			const unsigned char oracleMatches = fromOracle == shapes[which]->owner ? 1 : 0;
			const unsigned char candidateMatches = fromCandidate == shapes[which]->owner ? 1 : 0;
			nxDigestByte(&oracleDigest, oracleMatches);
			nxDigestByte(&candidateDigest, candidateMatches);
			if(fromOracle != fromCandidate || !oracleMatches)
				++mismatches;
			}

		for(int mode = 0; mode < 2; ++mode)
			{
			memset(poisonedSink, 0xcd, sizeof(poisonedSink));
			memcpy(poisonedCopy, poisonedSink, sizeof(poisonedSink));

			nxSetControl(mode ? kControlSimulate : kControlDefault);
			const unsigned char fromOracle = oracleGuard(shapes[1 - staticSide],
				shapes[staticSide], poisonedSink) ? 1 : 0;
			const bool untouched = memcmp(poisonedSink, poisonedCopy, sizeof(poisonedSink)) == 0;
			const unsigned char fromCandidate = NxContinuousCdPair(shapes[1 - staticSide],
				shapes[staticSide], (NxContactSink*) poisonedSink) ? 1 : 0;
			nxSetControl(kControlDefault);

			++guardProbes;
			if(fromOracle)
				++guardTrue;
			if(untouched)
				++sinkUntouched;
			nxDigestByte(&oracleDigest, fromOracle);
			nxDigestByte(&oracleDigest, untouched ? 1 : 0);
			nxDigestByte(&candidateDigest, fromCandidate);
			nxDigestByte(&candidateDigest, 1);
			if(fromOracle != fromCandidate || !untouched)
				++mismatches;
			}
		}

	// Which parameter the guard reads. Every index except 11 is set to 1.0f in
	// turn and the guard is asked again; if it read any of them the answer would
	// move. 11 itself is never disturbed, because that arm is the stop above.
	{
	static unsigned char probeShape[2][kShapeBytes];
	static unsigned char probeOwner[2][0x40];
	static unsigned char probeHolder[0x280];
	NxCollisionShape* first = (NxCollisionShape*) probeShape[0];
	NxCollisionShape* second = (NxCollisionShape*) probeShape[1];
	nxIdentity(first);
	nxIdentity(second);
	memset(probeOwner, 0, sizeof(probeOwner));
	memset(probeHolder, 0, sizeof(probeHolder));
	first->owner = probeOwner[0];
	second->owner = probeOwner[1];
	*(unsigned char**) (probeOwner[0] + 8) = probeHolder;

	for(unsigned index = 0; index < kParameterCount; ++index)
		{
		if(index == kContinuousCdParameter)
			continue;
		const float saved = oracleParameters[index];
		oracleParameters[index] = 1.0f;
		memset(poisonedSink, 0xcd, sizeof(poisonedSink));
		memcpy(poisonedCopy, poisonedSink, sizeof(poisonedSink));
		const bool answered = oracleGuard(first, second, poisonedSink);
		const bool untouched = memcmp(poisonedSink, poisonedCopy, sizeof(poisonedSink)) == 0;
		oracleParameters[index] = saved;
		++indexProbes;
		if(!answered || !untouched)
			++indexWrongHere;
		}
	}

	totalMismatch += mismatches;
	printf("collision name=shape_owner index=- rva=0x%08x owner=phys_fn_001281 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kShapeOwnerRva, oracleDigest.checks, oracleDigest.state,
		candidateDigest.state, mismatches);
	printf("collision coverage name=ccd_guard rva=0x%08x owner=phys_fn_002266 continuous_cd=%08x owner_probes=%u guard_probes=%u guard_true=%u sink_untouched=%u index_probes=%u index_wrong=%u\n",
		kContinuousCdRva, continuousBits, ownerProbes, guardProbes, guardTrue,
		sinkUntouched, indexProbes, indexWrongHere);
	if(indexWrongHere)
		return nxFail("the continuous-CD guard answered to a parameter other than NX_CONTINUOUS_CD");
	}

	// -----------------------------------------------------------------------
	// Matrix A [SPHERE][SPHERE].
	//
	// The entry is 341 bytes and drags six more rows in behind it, of which two
	// -- phys_fn_001281 and phys_fn_002266 -- are driven by the block above and
	// four are the stop it describes. What is left here is the geometry, and
	// three of its inputs no random pair reaches:
	//
	//   coincident -- two centres within 1e-5f of each other, the only way to
	//     reach the epsilon test at 0x0004b8ff. Without it the radius test is
	//     the only exit and the divide by the distance is never guarded.
	//   static0/static1 -- a null `owner->[8]` on each side in turn. The entry
	//     tests shape0 first and shape1 only if shape0's is non-null, so the
	//     first of those two branches needs the side the harness had never
	//     driven null.
	//   the sink oriented to the second sphere, which is the emitter's negated
	//     path with a normal this entry computed rather than borrowed.
	{
	typedef void(__cdecl* NxOracleContactFn)(const NxCollisionShape*, const NxCollisionShape*,
		NxContactSink*, void*);
	NxOracleContactFn oracleContact = (NxOracleContactFn) (base + nxMatrixA[7].rva);

	static NxContactWorld world[2];

	NxDigest oracleDigest, candidateDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	unsigned mismatches = 0;
	unsigned perMode[2] = { 0, 0 };
	unsigned emitted = 0;
	unsigned coincident = 0;
	unsigned coincidentEmitted = 0;
	unsigned repeated = 0;
	unsigned negatedPath = 0;
	unsigned staticSide[2] = { 0, 0 };
	unsigned widths[48];
	memset(widths, 0, sizeof(widths));
	unsigned state = 0x7d31c40bu;

	for(unsigned i = 0; i < kContactIterations; ++i)
		{
		const unsigned sequenceSeed = nxNext(&state);
		const unsigned pairs = 1 + (nxNext(&state) % 4);

		for(int mode = 0; mode < 2; ++mode)
			{
			for(int side = 0; side < 2; ++side)
				nxResetWorld(&world[side]);

			unsigned local = sequenceSeed;
			static unsigned char firstStorage[kShapeBytes];
			static unsigned char secondStorage[kShapeBytes];
			NxCollisionShape* firstShape = (NxCollisionShape*) firstStorage;
			NxCollisionShape* secondShape = (NxCollisionShape*) secondStorage;
			bool coincidentPair = false;
			for(unsigned p = 0; p < pairs; ++p)
				{
				// The same pair again with the same identities. The emitter
				// skips both the header and the normal block for it, which is
				// the ordering rule's third state and the only way this block
				// reaches a four-word contact.
				const bool repeat = p > 0 && (nxNext(&local) & 1) != 0;
				if(!repeat)
					{
					const bool tame = (nxNext(&local) & 3) != 0;
					nxIdentity(firstShape);
					nxIdentity(secondShape);
					nxFillGeometry(&local, firstShape, 1, tame);
					nxFillGeometry(&local, secondShape, 1, tame);
					for(int k = 0; k < 3; ++k)
						firstShape->translation[k] = tame ? nxUnit(&local) * 4.0f - 2.0f : nxPick(&local);

					// The placement is along a direction the generator builds
					// itself, so nothing multiplies a value it also allows to be
					// non-finite: the direction is always three finite units.
					float direction[3];
					float length = 0.0f;
					for(int k = 0; k < 3; ++k)
						{
						direction[k] = nxUnit(&local) * 2.0f - 1.0f;
						length += direction[k] * direction[k];
						}
					length = (float) sqrt((double) length);
					if(length > 1e-4f)
						for(int k = 0; k < 3; ++k)
							direction[k] /= length;
					else
						{ direction[0] = 0.0f; direction[1] = 1.0f; direction[2] = 0.0f; }

					coincidentPair = tame && (nxNext(&local) % 6) == 0;
					if(tame)
						{
						const float reach = firstShape->geometry[0] + secondShape->geometry[0];
						// Coincident draws span 0 to 1e-2, whose square straddles
						// the 1e-5f at 0x00107a08 about one time in three, so the
						// epsilon test is crossed in BOTH directions rather than
						// only entered. Everything else is 0.2 to 1.6 reaches,
						// which crosses the radius test the same way.
						const float span = coincidentPair
							? nxUnit(&local) * 1e-2f
							: reach * (nxUnit(&local) * 1.4f + 0.2f);
						for(int k = 0; k < 3; ++k)
							secondShape->translation[k] = firstShape->translation[k]
								+ direction[k] * span;
						}
					else
						for(int k = 0; k < 3; ++k)
							secondShape->translation[k] = nxPick(&local);
					}
				if(coincidentPair)
					++coincident;
				if(repeat && mode == 0)
					++repeated;

				const bool newIdentity0 = !repeat && ((p == 0) || ((nxNext(&local) & 3) == 0));
				const bool newIdentity1 = !repeat && ((p == 0) || ((nxNext(&local) & 3) == 0));
				const NxU32 material0 = nxNext(&local) & 0xff;
				const NxU32 material1 = nxNext(&local) & 0xff;
				// One side static in one call in six, and which side alternates.
				// Never both: the emitter's material fallback would then read
				// through a null holder, which the oracle does too.
				const unsigned staticDraw = nxNext(&local) % 12;
				const bool nullHolder0 = staticDraw == 0;
				const bool nullHolder1 = staticDraw == 1;
				const bool orientToSecond = (nxNext(&local) & 1) != 0;
				if(mode == 0)
					{
					if(nullHolder0)
						++staticSide[0];
					if(nullHolder1)
						++staticSide[1];
					if(orientToSecond)
						++negatedPath;
					}

				const unsigned before = world[0].sink.streamCount;
				for(int side = 0; side < 2; ++side)
					nxStageWorld(&world[side], firstShape, secondShape,
						newIdentity0, newIdentity1, material0, material1,
						nullHolder0, nullHolder1, !orientToSecond);

				nxSetControl(mode ? kControlSimulate : kControlDefault);
				oracleContact(world[0].plane, world[0].sphere, &world[0].sink, nxOverlapContext);
				NxContactSphereSphere(world[1].plane, world[1].sphere, &world[1].sink, nxOverlapContext);
				nxSetControl(kControlDefault);

				const unsigned appended = world[0].sink.streamCount - before;
				if(appended)
					++emitted;
				// The oracle's own answer for the coincident draws: the ones
				// that still emitted are the ones whose squared distance was
				// above the epsilon, so a generator that stopped straddling it
				// takes one of these two counts to zero.
				if(coincidentPair && appended)
					++coincidentEmitted;
				if(appended < 48)
					++widths[appended];
				}

			nxFoldStream(&oracleDigest, &world[0]);
			nxFoldStream(&candidateDigest, &world[1]);
			const unsigned differing = nxCompareStreams(&world[0], &world[1]);
			mismatches += differing;
			perMode[mode] += differing;
			}
		}
	totalMismatch += perMode[0];
	printf("collision name=contact_sphere_sphere index=7 rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		nxMatrixA[7].rva, nxMatrixA[7].stableId,
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	printf("collision coverage name=contact_sphere_sphere emitted=%u coincident=%u coincident_emitted=%u repeated=%u static0=%u static1=%u negated=%u w4=%u w8=%u w11=%u default_mismatches=%u simulate_mismatches=%u\n",
		emitted, coincident, coincidentEmitted, repeated, staticSide[0], staticSide[1],
		negatedPath, widths[4], widths[8], widths[11], perMode[0], perMode[1]);
	}

	// -----------------------------------------------------------------------
	// phys_fn_001917 at 0x00049f00, driven at its own address.
	//
	// The entry above it cannot reach all of it and the block after cannot
	// reach it often enough. Its second algorithm -- the sphere centre inside
	// the box, where there is no closest point and the contact comes from the
	// shallowest face -- is only entered when every axis is inside its extent,
	// which a placed pair reaches by accident. Half this generator puts the
	// centre inside on purpose, the same way the phys_fn_001913 block does for
	// the early return that path shares.
	{
	typedef bool (__cdecl* NxOracleSphereBoxContactFn)(const NxCollisionSphereData*,
		const NxCollisionBoxData*, NxVec3*, NxVec3*, NxReal*);
	NxOracleSphereBoxContactFn oracleContact =
		(NxOracleSphereBoxContactFn) (base + kSphereBoxContactRva);

	NxDigest oracleDigest, candidateDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	unsigned mismatches = 0;
	unsigned perMode[2] = { 0, 0 };
	unsigned trueCount = 0;
	unsigned insideCount = 0;
	unsigned axisWins[3] = { 0, 0, 0 };
	unsigned state = 0x9f2c81a3u;

	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		const bool tame = (nxNext(&state) & 3) != 0;
		static unsigned char boxStorage[kShapeBytes];
		NxCollisionShape* boxShape = (NxCollisionShape*) boxStorage;
		nxIdentity(boxShape);
		nxFillGeometry(&state, boxShape, 2, tame);
		nxRandomRotation(&state, boxShape);

		NxCollisionSphereData sphere;
		NxCollisionBoxData box;
		for(int k = 0; k < 3; ++k)
			{
			box.center[k] = tame ? nxUnit(&state) * 2.0f - 1.0f : nxPick(&state);
			box.extents[k] = boxShape->geometry[k + 1];
			}
		for(int k = 0; k < 9; ++k)
			box.rotation[k] = boxShape->rotation[k];
		sphere.radius = tame ? nxUnit(&state) * 1.5f + 0.02f : nxPick(&state);

		const bool inside = tame && (nxNext(&state) & 1) != 0;
		if(inside)
			{
			++insideCount;
			// Placed in box space and rotated back out, so the centre really is
			// inside every extent and not merely near the centre in world
			// coordinates -- which for a rotated box is not the same thing.
			float localOffset[3];
			for(int k = 0; k < 3; ++k)
				localOffset[k] = (nxUnit(&state) * 2.0f - 1.0f) * box.extents[k] * 0.9f;
			for(int k = 0; k < 3; ++k)
				sphere.center[k] = box.center[k]
					+ box.rotation[k * 3 + 0] * localOffset[0]
					+ box.rotation[k * 3 + 1] * localOffset[1]
					+ box.rotation[k * 3 + 2] * localOffset[2];
			}
		else
			{
			for(int k = 0; k < 3; ++k)
				sphere.center[k] = tame ? nxUnit(&state) * 4.0f - 2.0f : nxPick(&state);
			}

		for(int mode = 0; mode < 2; ++mode)
			{
			NxVec3 point[2], normal[2];
			NxReal separation[2];
			memset(point, 0xcd, sizeof(point));
			memset(normal, 0xcd, sizeof(normal));
			memset(separation, 0xcd, sizeof(separation));

			nxSetControl(mode ? kControlSimulate : kControlDefault);
			const unsigned char fromOracle =
				oracleContact(&sphere, &box, &point[0], &normal[0], &separation[0]) ? 1 : 0;
			const unsigned char fromCandidate =
				NxSphereBoxContactData(&sphere, &box, &point[1], &normal[1], &separation[1]) ? 1 : 0;
			nxSetControl(kControlDefault);

			if(fromOracle)
				++trueCount;
			nxDigestByte(&oracleDigest, fromOracle);
			nxDigestByte(&candidateDigest, fromCandidate);
			if(fromOracle != fromCandidate)
				{ ++mismatches; ++perMode[mode]; }

			// Which face won, taken from the oracle's own normal rather than
			// from anything this harness recomputes: on the inside path exactly
			// one local axis is +-1 and the world normal is that axis through
			// the rotation, so the winning column is the one it matches.
			if(fromOracle && mode == 0 && inside)
				{
				for(int axis = 0; axis < 3; ++axis)
					{
					const float* column = &box.rotation[axis];
					if(fabs((double) normal[0].x - column[0]) < 1e-3
						&& fabs((double) normal[0].y - column[3]) < 1e-3
						&& fabs((double) normal[0].z - column[6]) < 1e-3)
						++axisWins[axis];
					else if(fabs((double) normal[0].x + column[0]) < 1e-3
						&& fabs((double) normal[0].y + column[3]) < 1e-3
						&& fabs((double) normal[0].z + column[6]) < 1e-3)
						++axisWins[axis];
					}
				}

			const NxU32* words[2];
			words[0] = (const NxU32*) &point[0];
			words[1] = (const NxU32*) &point[1];
			for(int word = 0; word < 3; ++word)
				{
				nxDigestByte(&oracleDigest, (unsigned char) words[0][word]);
				nxDigestByte(&oracleDigest, (unsigned char) (words[0][word] >> 8));
				nxDigestByte(&oracleDigest, (unsigned char) (words[0][word] >> 16));
				nxDigestByte(&oracleDigest, (unsigned char) (words[0][word] >> 24));
				nxDigestByte(&candidateDigest, (unsigned char) words[1][word]);
				nxDigestByte(&candidateDigest, (unsigned char) (words[1][word] >> 8));
				nxDigestByte(&candidateDigest, (unsigned char) (words[1][word] >> 16));
				nxDigestByte(&candidateDigest, (unsigned char) (words[1][word] >> 24));
				if(words[0][word] != words[1][word])
					{ ++mismatches; ++perMode[mode]; }
				}
			const NxU32* normalWords[2];
			normalWords[0] = (const NxU32*) &normal[0];
			normalWords[1] = (const NxU32*) &normal[1];
			for(int word = 0; word < 3; ++word)
				{
				nxDigestByte(&oracleDigest, (unsigned char) normalWords[0][word]);
				nxDigestByte(&oracleDigest, (unsigned char) (normalWords[0][word] >> 8));
				nxDigestByte(&oracleDigest, (unsigned char) (normalWords[0][word] >> 16));
				nxDigestByte(&oracleDigest, (unsigned char) (normalWords[0][word] >> 24));
				nxDigestByte(&candidateDigest, (unsigned char) normalWords[1][word]);
				nxDigestByte(&candidateDigest, (unsigned char) (normalWords[1][word] >> 8));
				nxDigestByte(&candidateDigest, (unsigned char) (normalWords[1][word] >> 16));
				nxDigestByte(&candidateDigest, (unsigned char) (normalWords[1][word] >> 24));
				if(normalWords[0][word] != normalWords[1][word])
					{ ++mismatches; ++perMode[mode]; }
				}
			NxU32 separationWords[2];
			memcpy(&separationWords[0], &separation[0], 4);
			memcpy(&separationWords[1], &separation[1], 4);
			for(int half = 0; half < 2; ++half)
				for(int byte = 0; byte < 4; ++byte)
					nxDigestByte(half ? &candidateDigest : &oracleDigest,
						(unsigned char) (separationWords[half] >> (byte * 8)));
			if(separationWords[0] != separationWords[1])
				{ ++mismatches; ++perMode[mode]; }
			}
		}
	totalMismatch += perMode[0];
	printf("collision name=sphere_box_contact index=- rva=0x%08x owner=phys_fn_001917 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kSphereBoxContactRva, oracleDigest.checks, oracleDigest.state,
		candidateDigest.state, mismatches);
	printf("collision coverage name=sphere_box_contact true=%u centre_inside=%u axis_x=%u axis_y=%u axis_z=%u default_mismatches=%u simulate_mismatches=%u\n",
		trueCount, insideCount, axisWins[0], axisWins[1], axisWins[2],
		perMode[0], perMode[1]);
	}

	// -----------------------------------------------------------------------
	// Matrix A [SPHERE][BOX].
	//
	// The entry itself is 259 bytes of flattening plus the emitter call, and the
	// one thing in it that no other block in this target measures is which side
	// lands in which emitter slot: it hands the SPHERE as `object1` and the BOX
	// as `object0`, which is shape0 in the slot plane/sphere gives shape1. That
	// decides which owner the header material comes from first and which
	// collision object the ordering rule compares, so the two materials and the
	// two identities are driven apart here for the same reason the emitter block
	// drives them apart.
	{
	typedef void(__cdecl* NxOracleContactFn)(const NxCollisionShape*, const NxCollisionShape*,
		NxContactSink*, void*);
	NxOracleContactFn oracleContact = (NxOracleContactFn) (base + nxMatrixA[8].rva);

	static NxContactWorld world[2];

	NxDigest oracleDigest, candidateDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	unsigned mismatches = 0;
	unsigned perMode[2] = { 0, 0 };
	unsigned emitted = 0;
	unsigned insideCount = 0;
	unsigned repeated = 0;
	unsigned negatedPath = 0;
	unsigned staticSide[2] = { 0, 0 };
	unsigned widths[48];
	memset(widths, 0, sizeof(widths));
	unsigned state = 0x4ea6b03fu;

	for(unsigned i = 0; i < kContactIterations; ++i)
		{
		const unsigned sequenceSeed = nxNext(&state);
		const unsigned pairs = 1 + (nxNext(&state) % 4);

		for(int mode = 0; mode < 2; ++mode)
			{
			for(int side = 0; side < 2; ++side)
				nxResetWorld(&world[side]);

			unsigned local = sequenceSeed;
			static unsigned char sphereStorage[kShapeBytes];
			static unsigned char boxStorage[kShapeBytes];
			NxCollisionShape* sphereShape = (NxCollisionShape*) sphereStorage;
			NxCollisionShape* boxShape = (NxCollisionShape*) boxStorage;
			for(unsigned p = 0; p < pairs; ++p)
				{
				// The same pair again with the same identities, which is the
				// only way to a four-word contact: both the header and the
				// normal block are then skipped.
				const bool repeat = p > 0 && (nxNext(&local) & 1) != 0;
				bool insidePair = false;
				if(!repeat)
					{
					const bool tame = (nxNext(&local) & 3) != 0;
					nxIdentity(sphereShape);
					nxIdentity(boxShape);
					nxFillGeometry(&local, sphereShape, 1, tame);
					nxFillGeometry(&local, boxShape, 2, tame);
					nxRandomRotation(&local, boxShape);
					for(int k = 0; k < 3; ++k)
						boxShape->translation[k] = tame ? nxUnit(&local) * 2.0f - 1.0f : nxPick(&local);

					insidePair = tame && (nxNext(&local) % 3) == 0;
					if(tame)
						{
						// In box space and back out, so "inside" means inside
						// every extent of the ROTATED box, which is not the same
						// as near the centre in world coordinates.
						float localOffset[3];
						for(int k = 0; k < 3; ++k)
							localOffset[k] = (nxUnit(&local) * 2.0f - 1.0f)
								* boxShape->geometry[k + 1] * (insidePair ? 0.9f : 2.4f);
						for(int k = 0; k < 3; ++k)
							sphereShape->translation[k] = boxShape->translation[k]
								+ boxShape->rotation[k * 3 + 0] * localOffset[0]
								+ boxShape->rotation[k * 3 + 1] * localOffset[1]
								+ boxShape->rotation[k * 3 + 2] * localOffset[2];
						}
					else
						for(int k = 0; k < 3; ++k)
							sphereShape->translation[k] = nxPick(&local);
					}
				if(insidePair && mode == 0)
					++insideCount;
				if(repeat && mode == 0)
					++repeated;

				const bool newIdentity0 = !repeat && ((p == 0) || ((nxNext(&local) & 3) == 0));
				const bool newIdentity1 = !repeat && ((p == 0) || ((nxNext(&local) & 3) == 0));
				const NxU32 material0 = nxNext(&local) & 0xff;
				const NxU32 material1 = nxNext(&local) & 0xff;
				const unsigned staticDraw = nxNext(&local) % 12;
				const bool nullHolder0 = staticDraw == 0;
				const bool nullHolder1 = staticDraw == 1;
				const bool orientToBox = (nxNext(&local) & 1) != 0;
				if(mode == 0)
					{
					if(nullHolder0)
						++staticSide[0];
					if(nullHolder1)
						++staticSide[1];
					if(orientToBox)
						++negatedPath;
					}

				const unsigned before = world[0].sink.streamCount;
				for(int side = 0; side < 2; ++side)
					nxStageWorld(&world[side], sphereShape, boxShape,
						newIdentity0, newIdentity1, material0, material1,
						nullHolder0, nullHolder1, !orientToBox);

				nxSetControl(mode ? kControlSimulate : kControlDefault);
				oracleContact(world[0].plane, world[0].sphere, &world[0].sink, nxOverlapContext);
				NxContactSphereBox(world[1].plane, world[1].sphere, &world[1].sink, nxOverlapContext);
				nxSetControl(kControlDefault);

				const unsigned appended = world[0].sink.streamCount - before;
				if(appended)
					++emitted;
				if(appended < 48)
					++widths[appended];
				}

			nxFoldStream(&oracleDigest, &world[0]);
			nxFoldStream(&candidateDigest, &world[1]);
			const unsigned differing = nxCompareStreams(&world[0], &world[1]);
			mismatches += differing;
			perMode[mode] += differing;
			}
		}
	totalMismatch += perMode[0];
	printf("collision name=contact_sphere_box index=8 rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		nxMatrixA[8].rva, nxMatrixA[8].stableId,
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	printf("collision coverage name=contact_sphere_box emitted=%u centre_inside=%u repeated=%u static0=%u static1=%u negated=%u w4=%u w8=%u w11=%u default_mismatches=%u simulate_mismatches=%u\n",
		emitted, insideCount, repeated, staticSide[0], staticSide[1], negatedPath,
		widths[4], widths[8], widths[11], perMode[0], perMode[1]);
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
		{
		bool driven = false;
		for(unsigned entry = 0; entry < kDrivenContactCount; ++entry)
			if(nxDrivenContact[entry] == index)
				driven = true;
		if(!driven && nxMatrixA[index].rva)
			printf("collision unreconstructed half=A type0=%u type1=%u rva=0x%08x owner=%s\n",
				index / 6, index % 6, nxMatrixA[index].rva, nxMatrixA[index].stableId);
		}

	printf("collision matrix_wrong=%u index_wrong=%u mismatches=%u\n", matrixWrong, indexWrong, totalMismatch);
	if(matrixWrong || indexWrong || totalMismatch)
		return nxFail("the reconstruction does not agree with the pinned oracle");
	printf("collision=pass\n");
	return 0;
	}
