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
#include "NxBoxDistance.h"
#include "NxTriangleDistance.h"
#include "NxGeometryHelpers.h"
#include "NxMeshContactHelpers.h"
#include "NxRay.h"
#include "NxPlane.h"
#include "NxIntersectionRayPlane.h"
#include "NxIntersectionSegmentBox.h"

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
	{ "box_box",        2 * 6 + 2, NxOverlapBoxBox },
	// convex-mesh gap Task 2a: phys_fn_001751 and phys_fn_001774.
	{ "box_capsule",    2 * 6 + 3, NxOverlapBoxCapsule },
	{ "capsule_capsule", 3 * 6 + 3, NxOverlapCapsuleCapsule }
	};
static const unsigned kDrivenCount = sizeof(nxDriven) / sizeof(nxDriven[0]);

// The matrix B slots driven by blocks of their own rather than by the random
// and aimed generator above: the three compound entries (convex-mesh gap
// Task 2a), which need a compound shape the generator does not build.
static const unsigned nxDrivenOverlapOwnBlock[] =
	{
	1 * 6 + 5,		// phys_fn_001789
	2 * 6 + 5,		// phys_fn_001791
	3 * 6 + 5		// phys_fn_001785
	};

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
	2 * 6 + 2,		// phys_fn_001749
	3 * 6 + 3,		// phys_fn_001775
	2 * 6 + 3		// phys_fn_001753, convex-mesh gap Task 2a
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
//
// The word is WRITTEN INTO ITS SLOT AS BITS and never returned as a float. This
// used to be `float nxPick(unsigned*)`, and under the x86 ABI a float return
// travels in st(0) even from this SSE2 translation unit: loading a signalling
// NaN there quiets it, while an inlined call keeps the value in an XMM register
// and hands it on intact. Which draws reached the oracle quieted therefore
// depended on the compiler's inlining decisions (Opcode.h's inline_depth pragma
// and wmain's size both moved them), so registered oracle digests pinned this
// harness's code generation (convex-mesh gap harness hardening). The draw stream
// is the old one: same choice, same number of xorshift steps per branch.
//
// nxPickWordFrom writes the word exactly as drawn. nxPickWord -- what every
// family written before Task 2b draws through -- then QUIETS a NaN explicitly
// (sets bit 22, keeping sign and payload: what an x87 load does to it), because
// that is what those families' registered digests were measured on: with the
// quieting written here every registered digest of theirs reproduces and their
// input digests equal the float-returning generator's -- every call site had in
// fact been going through st(0). Handed the signalling
// NaNs instead, 15 of those blocks' candidates differ from the oracle (NaN
// payloads -- a signalling memory operand loses to a quiet register operand on
// x87 whatever the significands, and the candidates load operands in orders the
// listings do not -- and, where a sign-bit test reads such a NaN, branches);
// those blocks also run as `<block>.snan` variants, on the same draws with the
// signalling NaNs kept (gKeepSignallingNaN, below), divergent under enforced
// ceilings (kSnanCeilings; evidence/convex-mesh-gap.md, `Harness hardening`).
// nxPickRawWord always keeps signalling NaNs, for the families whose rows are
// exact on them (step_ray_tri; Task 2b's use nxPickBits).
static void nxPickWordFrom(unsigned* state, unsigned choice, float* out)
	{
	unsigned bits;
	if(choice == 0)
		bits = nxNext(state);
	else if(choice == 1)
		bits = 0;
	else if(choice == 2)
		{
		const float value = nxUnit(state) * 1e-6f;
		memcpy(&bits, &value, 4);
		}
	else if(choice == 3)
		// Sign from the low bit, payload from the rest; a zero payload is an
		// infinity and anything else is a NaN, so both arrive without either
		// being spelled out.
		bits = (nxNext(state) & 0x807fffffu) | 0x7f800000u;
	else
		{
		const float value = nxUnit(state) * 8.0f - 4.0f;
		memcpy(&bits, &value, 4);
		}
	memcpy(out, &bits, 4);
	}

// ---------------------------------------------------------------------------
// The `.snan` variants (convex-mesh gap harness hardening, controller decision).
//
// Fifteen pre-Task-2b blocks draw their NaNs quiet (nxPickWord), and are exact on
// those draws. Handed the same draws with their signalling NaNs kept, their
// candidates differ from the oracle: x87 lets a signalling memory operand lose to
// a quiet register operand whatever the significands, and `fld` quiets what it
// loads, so a candidate that loads an operand its listing uses from memory (or the
// reverse) propagates another NaN -- and where a sign-bit test or a compare reads
// that NaN, takes another branch. Each of those blocks therefore runs twice: its
// registered family on quiet draws (gating, exact), then `<block>.snan` on the
// same draws with the signalling NaNs kept, which does not gate on equality but on
// the ceilings below (evidence/convex-mesh-gap.md, Harness hardening). Fixing one
// takes listing-faithful memory operands; the naked transcription of
// phys_fn_001712 in Geometry.cpp is the precedent.
//
// Per control word (0x027f, 0x0f7f) a .snan pass counts the differing words
// (`words`), how many of them are discrete -- a verdict, a count, a stream
// header, a word that is not a float -- (`discrete`), and how many are float
// words that are not a NaN on both sides (`non_nan`: a branch, or a NaN against
// a number, rather than a payload). The ceilings are the counts measured when the
// variants were registered; a count may fall, never rise.
static bool gKeepSignallingNaN = false;

struct NxSnanTally
	{
	unsigned words[2];
	unsigned discrete[2];
	unsigned nonNan[2];
	};
static NxSnanTally gSnanTally;

struct NxSnanCeiling
	{
	const char* name;
	unsigned words[2];
	unsigned discrete[2];
	unsigned nonNan[2];
	};

static const NxSnanCeiling kSnanCeilings[] =
	{
	//  block, words (0x027f, 0x0f7f), discrete, non_nan
	{ "box_corner", { 268, 268 }, { 0, 0 }, { 0, 0 } },
	{ "box_quad_depth", { 2, 2 }, { 0, 0 }, { 0, 0 } },
	{ "box_clip.random", { 19717, 19700 }, { 589, 588 }, { 18520, 18504 } },
	{ "box_axis.random", { 30212, 30190 }, { 2904, 2897 }, { 25904, 25889 } },
	{ "box_shim", { 5105, 5105 }, { 435, 435 }, { 4335, 4335 } },
	{ "contact_box_box", { 307, 307 }, { 39, 39 }, { 236, 236 } },
	{ "step_smooth_normals", { 15, 15 }, { 0, 0 }, { 0, 0 } },
	{ "contact_emit", { 3738, 3738 }, { 0, 0 }, { 0, 0 } },
	{ "shape_raycast_plane", { 1720, 1720 }, { 0, 0 }, { 0, 0 } },
	{ "contact_plane_capsule", { 470, 470 }, { 0, 0 }, { 0, 0 } },
	{ "shape_raycast_sphere", { 2171, 2171 }, { 0, 0 }, { 0, 0 } },
	{ "contact_sphere_capsule", { 804, 804 }, { 0, 0 }, { 0, 0 } },
	{ "sphere_box_contact", { 3181, 3181 }, { 0, 0 }, { 0, 0 } },
	{ "contact_sphere_box", { 2022, 2022 }, { 0, 0 }, { 0, 0 } },
	{ "contact_box_capsule", { 96, 90 }, { 4, 4 }, { 40, 40 } },
	// the kernel fuzz harness's three (nxDriveFuzzSnan)
	{ "fuzz_ray_plane", { 15, 15 }, { 0, 0 }, { 0, 0 } },
	{ "fuzz_ray_aabb", { 1, 1 }, { 0, 0 }, { 0, 0 } },
	{ "fuzz_segment_box", { 2, 2 }, { 0, 0 }, { 0, 0 } }
	};

// Sets the pass for the scope of one iteration of a block's pass loop.
struct NxSnanPass
	{
	explicit NxSnanPass(int pass)
		{
		gKeepSignallingNaN = pass != 0;
		memset(&gSnanTally, 0, sizeof(gSnanTally));
		}
	~NxSnanPass() { gKeepSignallingNaN = false; }
	};

static const char* nxSnanName(const char* name)
	{
	if(!gKeepSignallingNaN)
		return name;
	static char buffer[4][64];
	static unsigned next = 0;
	char* out = buffer[next++ & 3];
	sprintf_s(out, 64, "%s.snan", name);
	return out;
	}

static bool nxWordIsNaN(NxU32 word)
	{
	return (word & 0x7f800000u) == 0x7f800000u && (word & 0x007fffffu) != 0;
	}

// A float word compared under a .snan pass.
static void nxSnanWord(int mode, NxU32 a, NxU32 b)
	{
	if(!gKeepSignallingNaN || a == b)
		return;
	++gSnanTally.words[mode];
	if(!(nxWordIsNaN(a) && nxWordIsNaN(b)))
		++gSnanTally.nonNan[mode];
	}

// A discrete word -- a verdict, a count, a header -- compared under a .snan pass.
static void nxSnanDiscrete(int mode, unsigned a, unsigned b)
	{
	if(!gKeepSignallingNaN || a == b)
		return;
	++gSnanTally.words[mode];
	++gSnanTally.discrete[mode];
	}

// A ten-byte register spill compared under a .snan pass: one word.
static void nxSnanWide(int mode, const unsigned char* a, const unsigned char* b)
	{
	if(!gKeepSignallingNaN || memcmp(a, b, 10) == 0)
		return;
	++gSnanTally.words[mode];
	const bool nanA = (a[9] & 0x7f) == 0x7f && a[8] == 0xff && (a[7] & 0x7f) | a[6] | a[5] | a[4] | a[3] | a[2] | a[1] | a[0];
	const bool nanB = (b[9] & 0x7f) == 0x7f && b[8] == 0xff && (b[7] & 0x7f) | b[6] | b[5] | b[4] | b[3] | b[2] | b[1] | b[0];
	if(!(nanA && nanB))
		++gSnanTally.nonNan[mode];
	}

// What a block adds to the run's mismatch total: its own gating count on the
// quiet pass; on the .snan pass, 1 if any count is over the block's ceiling (or it
// has none), after printing the counts.
static unsigned nxSnanGate(const char* name, unsigned gating)
	{
	if(!gKeepSignallingNaN)
		return gating;
	const NxSnanCeiling* ceiling = 0;
	for(unsigned i = 0; i < sizeof(kSnanCeilings) / sizeof(kSnanCeilings[0]); ++i)
		if(!strcmp(kSnanCeilings[i].name, name))
			ceiling = &kSnanCeilings[i];
	bool over = ceiling == 0;
	for(int mode = 0; mode < 2 && ceiling; ++mode)
		if(gSnanTally.words[mode] > ceiling->words[mode]
			|| gSnanTally.discrete[mode] > ceiling->discrete[mode]
			|| gSnanTally.nonNan[mode] > ceiling->nonNan[mode])
			over = true;
	printf("collision divergent name=%s.snan cause=signalling_nan words=%u words_simulate=%u discrete=%u discrete_simulate=%u non_nan=%u non_nan_simulate=%u ceiling=%s\n",
		name, gSnanTally.words[0], gSnanTally.words[1], gSnanTally.discrete[0],
		gSnanTally.discrete[1], gSnanTally.nonNan[0], gSnanTally.nonNan[1],
		over ? "exceeded" : "ok");
	return over ? 1 : 0;
	}

static void nxPickRawWord(unsigned* state, float* out)
	{
	nxPickWordFrom(state, nxNext(state) & 7, out);
	}

static void nxPickWord(unsigned* state, float* out)
	{
	nxPickRawWord(state, out);
	unsigned bits;
	memcpy(&bits, out, 4);
	if(!gKeepSignallingNaN && (bits & 0x7f800000u) == 0x7f800000u && (bits & 0x007fffffu) != 0)
		{
		bits |= 0x00400000u;
		memcpy(out, &bits, 4);
		}
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

// The input digest. Every family folds the words it hands the oracle -- once per
// draw, not once per control word -- into a digest of its own and prints it as
// `collision input name=<family> words=<n> input=<digest>` next to its oracle
// line. An oracle digest moves when the inputs move as well as when the oracle's
// answers do; this line is what tells the two apart. It is over the exact words
// in the argument slots, so a raw word that changed on its way into its slot (a
// signalling NaN quieted by an x87 load, say) moves it too. Addresses are never
// folded: they differ per run.
static void nxFoldInput(NxDigest* input, const void* words, unsigned bytes)
	{
	const unsigned char* p = (const unsigned char*) words;
	for(unsigned i = 0; i < bytes; ++i)
		nxDigestByte(input, p[i]);
	}

// A shape's generated fields: rotation, translation, type, geometry (whose third
// word is the capsule's flag word), and the trigger-flag dword at +0xdc.
static void nxFoldInputShape(NxDigest* input, const NxCollisionShape* shape)
	{
	nxFoldInput(input, shape->rotation, sizeof(shape->rotation));
	nxFoldInput(input, shape->translation, sizeof(shape->translation));
	nxFoldInput(input, (const unsigned char*) shape + 0xdc, 4);
	nxFoldInput(input, &shape->type, sizeof(shape->type));
	nxFoldInput(input, shape->geometry, sizeof(shape->geometry));
	}

static void nxPrintInput(const char* name, const NxDigest* input)
	{
	printf("collision input name=%s words=%u input=%016llx\n", name, input->checks / 4,
		input->state);
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

static NxU32 nxBits(NxReal value)
	{
	NxU32 word;
	memcpy(&word, &value, 4);
	return word;
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

// phys_fn_001739 is the second row in this component entered through a register:
// its quad arrives in ecx and its two floats on the stack, and the CALLER cleans
// -- `add esp,8` at 0x0003993e. So the oracle side needs a thunk and the
// candidate, which takes ordinary parameters, needs its own. Both spill st(0)
// with `fstp tbyte` for the same reason nxCallSegmentDistance does: the one
// caller compares the register against 0.0f at 0x00039938 before it narrows it
// with `fst dword` at 0x0003994c, so a `double` return would round away the
// bits that decide that comparison under 0x0f7f.
//
// The two floats travel as their bit patterns, so nothing here can quiet a
// signalling NaN on the way in and the two sides are handed identical words.
static void nxCallQuadDepthOracle(const void* fn, const NxVec3* const* quad,
	NxU32 pointY, NxU32 pointZ, unsigned char wide[10])
	{
	__asm
		{
		mov  eax, pointZ
		push eax
		mov  eax, pointY
		push eax
		mov  ecx, quad
		mov  eax, fn
		call eax
		add  esp, 8
		mov  eax, wide
		fstp tbyte ptr [eax]
		}
	}

typedef double(__cdecl* NxQuadDepthFn)(const NxVec3* const*, NxReal, NxReal);

static void nxCallQuadDepthCandidate(NxQuadDepthFn fn, const NxVec3* const* quad,
	NxU32 pointY, NxU32 pointZ, unsigned char wide[10])
	{
	__asm
		{
		mov  eax, pointZ
		push eax
		mov  eax, pointY
		push eax
		mov  eax, quad
		push eax
		mov  eax, fn
		call eax
		add  esp, 12
		mov  eax, wide
		fstp tbyte ptr [eax]
		}
	}

// phys_fn_001741 is the third row in this component entered through registers,
// and the first entered through TWO: eax carries the incident box's pose and
// edx its three half extents. Neither is expressible in C++, and edx is the
// worse of the two -- four of phys_fn_001745's six dispatch arms never load it,
// so it survives untouched from phys_fn_001748 across a 4,271-byte function.
//
// The two extents travel as their bit patterns for the same reason the leaf's
// two floats do: a `push` of a float parameter can quiet a signalling NaN and
// the oracle's own caller pushes raw dwords.
//
// The count comes back in eax. The x87 stack is empty across the call on both
// sides, so nothing has to be spilled here.
static int nxCallClipFaceOracle(const void* fn, NxVec3* points, NxReal* separations,
	const NxReal* pose, NxU32 extentY, NxU32 extentZ,
	const NxReal* incidentPose, const NxReal* incidentExtents)
	{
	int result;
	__asm
		{
		mov  eax, extentZ
		push eax
		mov  eax, extentY
		push eax
		mov  eax, pose
		push eax
		mov  eax, separations
		push eax
		mov  eax, points
		push eax
		mov  edx, incidentExtents
		mov  eax, incidentPose
		mov  ecx, fn
		call ecx
		add  esp, 20
		mov  result, eax
		}
	return result;
	}

// phys_fn_001745 is the fourth row in this component entered through registers
// and the first entered through THREE: ebx carries box A's pose, eax the
// contact normal out-parameter and edx box B's half extents. Five arguments are
// on the stack and the caller cleans (`add esp,0x14` at 0x0003adc8).
//
// ebx is the one that needs care here. The oracle neither saves nor writes it
// -- it uses it as a base register throughout and phys_fn_001741 pushes it --
// but MSVC is free to have something of its own in it across this block, so it
// is pushed and popped rather than trusted. The oracle preserves ebp, esi and
// edi itself.
//
// Nothing is live in the x87 stack across the call on either side.
static int nxCallSeparatingAxisOracle(const void* fn, NxVec3* points,
	NxReal* separations, const NxReal* extentsA, const NxReal* poseB,
	unsigned char* cache, const NxReal* poseA, NxVec3* normal,
	const NxReal* extentsB)
	{
	int result;
	__asm
		{
		push ebx
		mov  eax, cache
		push eax
		mov  eax, poseB
		push eax
		mov  eax, extentsA
		push eax
		mov  eax, separations
		push eax
		mov  eax, points
		push eax
		mov  ebx, poseA
		mov  edx, extentsB
		mov  eax, normal
		mov  ecx, fn
		call ecx
		add  esp, 20
		mov  result, eax
		pop  ebx
		}
	return result;
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
			if(tame)
				n[i] = nxUnit(state) * 2.0f - 1.0f;
			else
				nxPickWord(state, &n[i]);
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
		if(tame)
			shape->geometry[3] = nxUnit(state) * 4.0f - 2.0f;
		else
			nxPickWord(state, &shape->geometry[3]);
		}
	else if(type == 1)
		{
		if(tame)
			shape->geometry[0] = nxUnit(state) * 2.0f + 0.05f;
		else
			nxPickWord(state, &shape->geometry[0]);
		}
	else if(type == 2)
		{
		if(tame)
			shape->geometry[0] = 0.0f;
		else
			nxPickWord(state, &shape->geometry[0]);
		for(int i = 1; i < 4; ++i)
			if(tame)
				shape->geometry[i] = nxUnit(state) * 2.0f + 0.05f;
			else
				nxPickWord(state, &shape->geometry[i]);
		}
	else
		{
		if(tame)
			shape->geometry[0] = nxUnit(state) * 1.5f + 0.05f;
		else
			nxPickWord(state, &shape->geometry[0]);
		if(tame)
			shape->geometry[1] = nxUnit(state) * 2.0f + 0.05f;
		else
			nxPickWord(state, &shape->geometry[1]);
		if(tame)
			shape->geometry[2] = 0.0f;
		else
			nxPickWord(state, &shape->geometry[2]);
		if(tame)
			shape->geometry[3] = 0.0f;
		else
			nxPickWord(state, &shape->geometry[3]);
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

// `mode` is the control word's index, for a .snan pass's counts: differing
// header counts are one discrete word, and the stream words both sides have are
// classified as floats (nxSnanWord).
static unsigned nxCompareStreams(const NxContactWorld* a, const NxContactWorld* b, int mode)
	{
	unsigned differing = 0;
	if(a->sink.streamCount != b->sink.streamCount
		|| a->sink.contactCount != b->sink.contactCount
		|| a->sink.featurePairValid != b->sink.featurePairValid)
		{
		++differing;
		nxSnanDiscrete(mode, 0, 1);
		}
	const unsigned common = a->sink.streamCount < b->sink.streamCount
		? a->sink.streamCount : b->sink.streamCount;
	for(unsigned w = 0; w < common; ++w)
		{
		const NxU32 wordA = nxCanonical(a, a->stream[w]);
		const NxU32 wordB = nxCanonical(b, b->stream[w]);
		if(wordA != wordB)
			{
			++differing;
			nxSnanWord(mode, wordA, wordB);
			}
		}
	return differing;
	}

struct NxBlockResult
	{
	NxDigest oracle;
	NxDigest candidate;
	NxDigest input;
	unsigned mismatches;
	unsigned trueCount;
	unsigned falseCount;
	unsigned swapDiffers;
	};

static void nxRunPair(NxOracleOverlapFn oracle, NxShapeOverlapFn candidate,
		const NxCollisionShape* a, const NxCollisionShape* b, NxBlockResult* result)
	{
	nxFoldInputShape(&result->input, a);
	nxFoldInputShape(&result->input, b);
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

// ---------------------------------------------------------------------------
// convex-mesh gap Task 2a: the box distance kernels and the entries that reach
// them.

// The oracle's box raycast, phys_fn_000949 -- what a BOX shape puts in vtable
// slot 5, from the public box vtable 0x00106dc8 + 0x14
// (evidence/phase5-box-flags.md). phys_fn_001753's swept path reaches it
// through the box's own vtable. It is not a row of this task and has no
// finished candidate, so BOTH worlds of contact_box_capsule are given the
// oracle's: the dispatch is data the shape carries, and what is compared is
// what phys_fn_001753 does with the hit.
static const unsigned kBoxRaycastRva = 0x00020880;
// .data 0x00128478, the owner callback Prunable::UpdateWorldAABB
// (phys_fn_004886) calls when it is set. The compound entries reach it; the
// harness requires it null, which is also the candidate's state.
static const unsigned kPrunableOwnerWorldAabbRva = 0x00128478;
// The three leaf kernels, by their recorded addresses.
static const unsigned kPointBoxRva = 0x00032840;		// phys_fn_001670
static const unsigned kLineBoxRva = 0x00033a50;		// phys_fn_001684
static const unsigned kSegmentBoxRva = 0x00033d00;		// phys_fn_001688

// All three leave their result in st(0) and two of them leave it unnarrowed
// (point/box always, segment/box past either end), so both sides go through
// this thunk and the register is spilled with `fstp tbyte`, the way
// nxCallSegmentDistance does it. Six dwords are always pushed; the five-
// argument point/box ignores the sixth, which __cdecl makes harmless.
// phys_fn_001753's crossing branch hands phys_fn_001748 the capsule as a box
// of half size (0.666 r, h, 0.666 r), the product formed by `fld; fmul dword
// [0x10107b54]; fst dword` under the live control word. The pre-flight probe
// below needs the same word, and this file is not built for x87, so it is
// formed here the way the oracle forms it.
static NxReal nxCapsulePseudoExtent(NxReal radius)
	{
	static const float scale = 0.666f;
	NxReal product;
	__asm
		{
		fld  radius
		fmul scale
		fstp product
		}
	return product;
	}

static void nxCallWide6(const void* fn, const void* a0, const void* a1, const void* a2,
	const void* a3, const void* a4, const void* a5, unsigned char wide[10])
	{
	__asm
		{
		mov  eax, a5
		push eax
		mov  eax, a4
		push eax
		mov  eax, a3
		push eax
		mov  eax, a2
		push eax
		mov  eax, a1
		push eax
		mov  eax, a0
		push eax
		mov  eax, fn
		call eax
		add  esp, 24
		mov  eax, wide
		fstp tbyte ptr [eax]
		}
	}

// Folds a 10-byte register spill and a run of 32-bit words into both digests
// and counts the words (and spills) that differ. NaN is canonicalised on both
// sides first, for the reason nxCanonicalWide gives.
static void nxFoldWide(NxDigest* oracle, NxDigest* candidate, unsigned char wide0[10],
	unsigned char wide1[10], unsigned* mismatches, unsigned* canonical)
	{
	if(nxCanonicalWide(wide0) | nxCanonicalWide(wide1))
		++*canonical;
	bool differs = false;
	for(int byte = 0; byte < 10; ++byte)
		{
		nxDigestByte(oracle, wide0[byte]);
		nxDigestByte(candidate, wide1[byte]);
		if(wide0[byte] != wide1[byte])
			differs = true;
		}
	if(differs)
		++*mismatches;
	}

static void nxFoldWords(NxDigest* oracle, NxDigest* candidate, const NxReal* words0,
	const NxReal* words1, unsigned count, unsigned* mismatches, unsigned* canonical)
	{
	for(unsigned w = 0; w < count; ++w)
		{
		NxU32 a, b;
		memcpy(&a, &words0[w], 4);
		memcpy(&b, &words1[w], 4);
		if(nxCanonicalNarrow(&a) | nxCanonicalNarrow(&b))
			++*canonical;
		for(int byte = 0; byte < 4; ++byte)
			{
			nxDigestByte(oracle, (unsigned char) (a >> (byte * 8)));
			nxDigestByte(candidate, (unsigned char) (b >> (byte * 8)));
			}
		if(a != b)
			++*mismatches;
		}
	}

// A box for the leaf kernels: centre, extents, rotation (NxCollisionBoxData).
// `aimed` keeps it physical (unit axes, positive extents, and one draw in
// eight zero-extent on one axis); otherwise every word can be anything nxPickWord
// gives. `identity` gives the axis-aligned frame, which is how the zero-
// direction arms of line/box are reached by construction.
static void nxFillBoxData(unsigned* state, NxCollisionBoxData* box, bool aimed, bool identity)
	{
	static unsigned char scratch[kShapeBytes];
	NxCollisionShape* shape = (NxCollisionShape*) scratch;
	nxIdentity(shape);
	if(!identity)
		nxRandomRotation(state, shape);
	for(int k = 0; k < 9; ++k)
		box->rotation[k] = shape->rotation[k];
	for(int k = 0; k < 3; ++k)
		{
		if(aimed)
			box->center[k] = nxUnit(state) * 4.0f - 2.0f;
		else
			nxPickWord(state, &box->center[k]);
		if(aimed)
			box->extents[k] = nxUnit(state) * 2.0f + 0.05f;
		else
			nxPickWord(state, &box->extents[k]);
		}
	if(aimed && (nxNext(state) & 7) == 0)
		box->extents[nxNext(state) % 3] = 0.0f;
	}

// A point in or around `box` in world space, from box coordinates `u` scaled
// by the extents. Only called on aimed boxes, whose words are all finite.
static void nxBoxPoint(const NxCollisionBoxData* box, const float* u, float* out)
	{
	float local[3];
	for(int k = 0; k < 3; ++k)
		local[k] = u[k] * box->extents[k];
	for(int r = 0; r < 3; ++r)
		out[r] = box->center[r] + box->rotation[r * 3 + 0] * local[0]
			+ box->rotation[r * 3 + 1] * local[1] + box->rotation[r * 3 + 2] * local[2];
	}

// A box-coordinate draw: inside, on a face, on an edge, at a corner or out
// past them, with the component count on the boundary chosen explicitly.
static void nxBoxCoordinate(unsigned* state, float* u)
	{
	const unsigned kind = nxNext(state) % 5;
	for(int k = 0; k < 3; ++k)
		u[k] = nxUnit(state) * 2.0f - 1.0f;
	if(kind >= 1 && kind <= 3)
		{
		// `kind` coordinates pinned to +-1: a face, an edge, a corner.
		const unsigned start = nxNext(state) % 3;
		for(unsigned j = 0; j < kind; ++j)
			u[(start + j) % 3] = (nxNext(state) & 1) ? 1.0f : -1.0f;
		}
	else if(kind == 4)
		for(int k = 0; k < 3; ++k)
			u[k] *= 2.5f;
	}

// convex-mesh gap Task 2b: the triangle distance kernels (Distance.cpp), the two
// Geometry.cpp helpers, the triangle plane (ContactBoxMeshICE.cpp) and the
// segment/triangle-edge test (ContactMeshHeightfield.cpp), each at its own
// recorded address.
static const unsigned kPointTriangleRva = 0x000329e0;	// phys_fn_001672
static const unsigned kLineLineRva = 0x000345b0;		// phys_fn_001692
static const unsigned kSegmentTriangleRva = 0x00034860;	// phys_fn_001694
static const unsigned kRayInflatedFanRva = 0x00036d90;	// phys_fn_001708
static const unsigned kRayAabbSlabRva = 0x00038050;	// phys_fn_001730
static const unsigned kTrianglePlaneRva = 0x0003c160;	// phys_fn_001760
static const unsigned kSegmentTriangleEdgeRva = 0x00044510;	// phys_fn_001855
static const unsigned kIceTriangleInflateRva = 0x000e4090;	// phys_fn_005185

// The candidate side of the pre-flight, in PhysicsCollisionInflate.cpp (why there).
void nxCandidateTriangleInflate(float* corners, float fatCoeff, bool constantBorder);

static const unsigned kRayTriIntersectRva = 0x00036f50;	// phys_fn_001712

// segment_triangle's second block: the Task 2b review's draws (see there).
static const unsigned kSegmentTriangleReviewDraws = 250000;

// Row phys_fn_001708 calls Triangle::Inflate (phys_fn_005185), a vendored row whose
// candidate differs from the oracle's in the last bits (held at `discovered` for
// that; evidence/vendored-correspondence.md), and NxRayTriIntersect
// (phys_fn_001712). Each side of ray_inflated_tris reaches its own, so the family
// pre-flights every fan: when both callees agree on all of its triangles (001712's
// outputs compared as the family folds words, a NaN as any NaN) the fan is
// compared exactly and gates; when either differs the fan's words are counted
// apart under these ceilings and do not gate, because what differs there is a
// callee, not 001708. 001712 differed on signalling NaNs until the harness
// hardening (75 / 31 fans) and agrees on every fan since its listing
// transcription; what is left is Inflate's. The ceilings are the counts measured
// when the family was last registered (fans under 0x027f / 0x0f7f; words under
// 0x027f / 0x0f7f).
static const unsigned kCalleeDivergentFanCeiling[2] = { 28725, 41198 };
static const unsigned kCalleeDivergentWordCeiling[2] = { 963, 1515 };

typedef void(__cdecl* NxLineLineFn)(NxReal*, NxReal*, const NxReal*, const NxReal*,
	const NxReal*, const NxReal*);
typedef bool(__cdecl* NxRayFanFn)(NxU32, const NxVec3*, const NxU32*, const NxRay*, NxReal*);
typedef int(__cdecl* NxSlabFn)(const NxReal*, const NxReal*, const NxReal*, const NxReal*,
	NxReal*, NxReal*);
typedef NxPlane*(__fastcall* NxTrianglePlaneFn)(NxPlane*, void*, const NxVec3*, const NxVec3*,
	const NxVec3*);
typedef bool(__cdecl* NxSegmentEdgeFn)(const NxReal*, const NxReal*, const NxReal*,
	const NxReal*, const NxReal*, NxReal*, NxReal*);

// Seven arguments, st(0) spilled whole: phys_fn_001694's shape.
static void nxCallWide7(const void* fn, const void* a0, const void* a1, const void* a2,
	const void* a3, const void* a4, const void* a5, const void* a6, unsigned char wide[10])
	{
	__asm
		{
		mov  eax, a6
		push eax
		mov  eax, a5
		push eax
		mov  eax, a4
		push eax
		mov  eax, a3
		push eax
		mov  eax, a2
		push eax
		mov  eax, a1
		push eax
		mov  eax, a0
		push eax
		mov  eax, fn
		call eax
		add  esp, 28
		mov  eax, wide
		fstp tbyte ptr [eax]
		}
	}

// Task 2b's raw words are written into their slots as bits, like every other
// family's since the harness hardening (see nxPickWord for why). nxPickBits draws
// what nxPickWord draws, plus a fifth branch of mixed exponents in place of one of
// the four ordinary ones; nxMixedBits is that branch alone:
// a finite word of random sign and significand with a magnitude in 2^-27..2^72, the
// range over which the wide intermediates of the triangle kernels differ from their
// 53-bit and 24-bit roundings (the review's scratch differential). One in sixteen
// is a zero of either sign.
static void nxMixedBits(unsigned* state, float* out)
	{
	unsigned bits;
	if((nxNext(state) & 15) == 0)
		bits = nxNext(state) & 0x80000000u;
	else
		{
		// Sequenced: as one expression the two draws' order was the compiler's.
		const unsigned significand = nxNext(state) & 0x807fffffu;
		bits = significand | ((100u + nxNext(state) % 100u) << 23);
		}
	memcpy(out, &bits, 4);
	}

static void nxPickBits(unsigned* state, float* out)
	{
	const unsigned choice = nxNext(state) & 7;
	if(choice == 4)
		nxMixedBits(state, out);
	else
		nxPickWordFrom(state, choice, out);
	}

// A word of a Task 2b draw that is not aimed: mixed exponents, or nxPickBits.
static void nxDrawBits(unsigned* state, bool mixed, float* out)
	{
	if(mixed)
		nxMixedBits(state, out);
	else
		nxPickBits(state, out);
	}

// A triangle for the triangle kernels. `aimed` keeps it finite: a random
// triangle in [-2, 2]^3, one draw in eight degenerate (a repeated vertex, or the
// third vertex on the line of the first two) and one in sixteen shrunk to a
// thousandth; otherwise every word is nxDrawBits's (mixed exponents when `mixed`).
// Returns true for a degenerate one.
static bool nxFillTriangle(unsigned* state, bool aimed, bool mixed, float v[3][3])
	{
	if(!aimed)
		{
		for(int i = 0; i < 3; ++i)
			for(int k = 0; k < 3; ++k)
				nxDrawBits(state, mixed, &v[i][k]);
		return false;
		}
	for(int i = 0; i < 3; ++i)
		for(int k = 0; k < 3; ++k)
			v[i][k] = nxUnit(state) * 4.0f - 2.0f;
	bool degenerate = false;
	const unsigned kind = nxNext(state) & 15;
	if(kind == 0 || kind == 1)
		{
		// A repeated vertex, any of the three pairs.
		const unsigned a = nxNext(state) % 3;
		const unsigned b = (a + 1 + nxNext(state) % 2) % 3;
		for(int k = 0; k < 3; ++k)
			v[b][k] = v[a][k];
		degenerate = true;
		}
	else if(kind == 2)
		{
		for(int k = 0; k < 3; ++k)
			v[2][k] = v[0][k] + (v[1][k] - v[0][k]) * 2.0f;
		degenerate = true;
		}
	else if(kind == 3)
		{
		for(int i = 1; i < 3; ++i)
			for(int k = 0; k < 3; ++k)
				v[i][k] = v[0][k] + (v[i][k] - v[0][k]) * 0.001f;
		}
	return degenerate;
	}

// The unit normal of a finite triangle, or (0, 0, 1) for a degenerate one. Only
// used to place inputs.
static void nxTriangleNormal(const float v[3][3], float n[3])
	{
	const float e0[3] = { v[1][0] - v[0][0], v[1][1] - v[0][1], v[1][2] - v[0][2] };
	const float e1[3] = { v[2][0] - v[0][0], v[2][1] - v[0][1], v[2][2] - v[0][2] };
	n[0] = e0[1] * e1[2] - e0[2] * e1[1];
	n[1] = e0[2] * e1[0] - e0[0] * e1[2];
	n[2] = e0[0] * e1[1] - e0[1] * e1[0];
	const float length = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
	if(length > 1e-12f)
		for(int k = 0; k < 3; ++k)
			n[k] /= length;
	else
		{
		n[0] = 0.0f;
		n[1] = 0.0f;
		n[2] = 1.0f;
		}
	}

// A point v0 + u e0 + w e1 + h n, with (u, w) drawn to land in each of the seven
// regions of the triangle's plane or exactly on a vertex or an edge (`onFeature`,
// h = 0 then).
static void nxTrianglePoint(unsigned* state, const float v[3][3], float out[3], bool* onFeature)
	{
	float n[3];
	nxTriangleNormal(v, n);
	float u = nxUnit(state) * 3.0f - 1.0f;
	float w = nxUnit(state) * 3.0f - 1.0f;
	float h = (nxUnit(state) * 2.0f - 1.0f) * ((nxNext(state) & 1) ? 1.0f : 0.05f);
	*onFeature = false;
	const unsigned feature = nxNext(state) & 7;
	if(feature == 0)
		{
		// A vertex.
		const unsigned which = nxNext(state) % 3;
		u = which == 1 ? 1.0f : 0.0f;
		w = which == 2 ? 1.0f : 0.0f;
		h = 0.0f;
		*onFeature = true;
		}
	else if(feature == 1)
		{
		// An edge, or the edge's line past its ends.
		const unsigned which = nxNext(state) % 3;
		if(which == 0)
			w = 0.0f;
		else if(which == 1)
			u = 0.0f;
		else
			w = 1.0f - u;
		h = 0.0f;
		*onFeature = true;
		}
	for(int k = 0; k < 3; ++k)
		out[k] = v[0][k] + u * (v[1][k] - v[0][k]) + w * (v[2][k] - v[0][k]) + h * n[k];
	}

// convex-mesh gap Task 2b (units/convex-mesh-gap-contract.md, sub-units E, F, I and
// N). The families are out of wmain because, before the harness hardening, putting
// them inside it made the compiler inline the old float-returning nxPick
// differently at earlier call sites, which moved registered oracle digests of Task
// 2a's compound families (see nxPickWord). Returns the gating mismatch count.
static __declspec(noinline) unsigned nxDriveTask2b(unsigned char* base)
	{
	unsigned totalMismatch = 0;
	// The triangle distance kernels, the ray/fan and slab helpers, the
	// triangle plane and the segment/triangle-edge test, at their own recorded
	// addresses, a third raw draws (every word through nxPickBits), a third aimed and a third
	// of mixed exponents (nxMixedBits), under
	// both control words. As for Task 2a's kernels only the default word's half
	// gates; the 0x0f7f count is in the registered coverage line.
	{
	// Row phys_fn_001672, point/triangle. The aimed half places the point in each
	// of the seven regions of the plane and exactly on vertices and edges; the
	// coverage counts which leaf the oracle's own parameters say ran.
	const void* const oracleFn = (const void*) (base + kPointTriangleRva);
	const void* const candidateFn = (const void*) &NxPointTriangleSquareDistance;
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	unsigned perMode[2] = { 0, 0 };
	unsigned canonical = 0;
	unsigned mixedDraws = 0;
	unsigned degenerate = 0;
	unsigned onFeature = 0;
	unsigned vertex[3] = { 0, 0, 0 };
	unsigned edgeS = 0;
	unsigned edgeT = 0;
	unsigned open = 0;
	unsigned floatMax = 0;
	unsigned nullOutputs = 0;
	unsigned state = 0x3a17c0d5u;
	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		const unsigned draw = nxNext(&state) % 3;
		const bool aimed = draw == 1;
		const bool mixed = draw == 2;
		if(mixed)
			++mixedDraws;
		float v[3][3];
		if(nxFillTriangle(&state, aimed, mixed, v))
			++degenerate;
		float point[3];
		if(aimed)
			{
			bool feature;
			nxTrianglePoint(&state, v, point, &feature);
			if(feature)
				++onFeature;
			}
		else
			for(int k = 0; k < 3; ++k)
				nxDrawBits(&state, mixed, &point[k]);
		const unsigned nulls = (nxNext(&state) % 8 == 0) ? (1 + nxNext(&state) % 3) : 0;
		if(nulls)
			++nullOutputs;
		nxFoldInput(&inputDigest, point, sizeof(point));
		nxFoldInput(&inputDigest, v, sizeof(v));
		nxFoldInput(&inputDigest, &nulls, sizeof(nulls));

		for(int mode = 0; mode < 2; ++mode)
			{
			unsigned char wide[2][10];
			NxReal out[2][2];
			memset(out, 0xcd, sizeof(out));
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			nxCallWide6(oracleFn, point, v[0], v[1], v[2],
				(nulls & 1) ? 0 : &out[0][0], (nulls & 2) ? 0 : &out[0][1], wide[0]);
			nxCallWide6(candidateFn, point, v[0], v[1], v[2],
				(nulls & 1) ? 0 : &out[1][0], (nulls & 2) ? 0 : &out[1][1], wide[1]);
			nxSetControl(kControlDefault);

			if(mode == 0 && !nulls)
				{
				const NxReal s = out[0][0];
				const NxReal t = out[0][1];
				if(s == 0.0f && t == 0.0f)
					++vertex[0];
				else if(s == 1.0f && t == 0.0f)
					++vertex[1];
				else if(s == 0.0f && t == 1.0f)
					++vertex[2];
				else if(s == 0.0f)
					++edgeS;
				else if(t == 0.0f)
					++edgeT;
				else
					++open;
				// FLT_MAX: the determinant-zero interior (0x00032c52).
				if(wide[0][9] == 0x40 && wide[0][8] == 0x7e && wide[0][7] == 0xff && wide[0][6] == 0xff && wide[0][5] == 0xff && wide[0][4] == 0)
					++floatMax;
				}
			nxFoldWide(&oracleDigest, &candidateDigest, wide[0], wide[1], &perMode[mode], &canonical);
			nxFoldWords(&oracleDigest, &candidateDigest, &out[0][0], &out[1][0], 2,
				&perMode[mode], &canonical);
			}
		}
	totalMismatch += perMode[0];
	printf("collision name=point_triangle index=- rva=0x%08x owner=phys_fn_001672 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kPointTriangleRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state,
		perMode[0] + perMode[1]);
	nxPrintInput("point_triangle", &inputDigest);
	printf("collision coverage name=point_triangle degenerate=%u on_feature=%u vertex0=%u vertex1=%u vertex2=%u edge_s0=%u edge_t0=%u open=%u flt_max=%u null_outputs=%u mixed=%u canonical_nan=%u default_mismatches=%u simulate_mismatches=%u\n",
		degenerate, onFeature, vertex[0], vertex[1], vertex[2], edgeS, edgeT, open, floatMax,
		nullOutputs, mixedDraws, canonical, perMode[0], perMode[1]);
	}

	{
	// Row phys_fn_001692, closest points of two lines. Aimed: finite origins and
	// directions, one draw in eight parallel (the second direction a multiple of
	// the first), one in sixteen with a zero direction, and one in sixteen
	// crossing (the second line through a point of the first).
	const NxLineLineFn oracleFn = (NxLineLineFn) (base + kLineLineRva);
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	unsigned perMode[2] = { 0, 0 };
	unsigned canonical = 0;
	unsigned mixedDraws = 0;
	unsigned parallel = 0;
	unsigned zeroDirection = 0;
	unsigned crossing = 0;
	unsigned atOrigin0 = 0;
	unsigned atOrigin1 = 0;
	unsigned state = 0x6c1b0e97u;
	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		const unsigned draw = nxNext(&state) % 3;
		const bool aimed = draw == 1;
		const bool mixed = draw == 2;
		if(mixed)
			++mixedDraws;
		float o0[3], d0[3], o1[3], d1[3];
		if(aimed)
			{
			for(int k = 0; k < 3; ++k)
				{
				o0[k] = nxUnit(&state) * 4.0f - 2.0f;
				d0[k] = nxUnit(&state) * 4.0f - 2.0f;
				o1[k] = nxUnit(&state) * 4.0f - 2.0f;
				d1[k] = nxUnit(&state) * 4.0f - 2.0f;
				}
			const unsigned kind = nxNext(&state) & 15;
			if(kind < 2)
				{
				const float scale = (nxNext(&state) & 1) ? -0.5f : 2.0f;
				for(int k = 0; k < 3; ++k)
					d1[k] = d0[k] * scale;
				++parallel;
				}
			else if(kind == 2)
				{
				float* zeroed = (nxNext(&state) & 1) ? d0 : d1;
				zeroed[0] = zeroed[1] = zeroed[2] = 0.0f;
				++zeroDirection;
				}
			else if(kind == 3)
				{
				const float s = nxUnit(&state) * 1.4f - 0.2f;
				const float t = nxUnit(&state) * 1.4f - 0.2f;
				for(int k = 0; k < 3; ++k)
					o1[k] = o0[k] + s * d0[k] - t * d1[k];
				++crossing;
				}
			}
		else
			for(int k = 0; k < 3; ++k)
				{
				nxDrawBits(&state, mixed, &o0[k]);
				nxDrawBits(&state, mixed, &d0[k]);
				nxDrawBits(&state, mixed, &o1[k]);
				nxDrawBits(&state, mixed, &d1[k]);
				}
		nxFoldInput(&inputDigest, o0, sizeof(o0));
		nxFoldInput(&inputDigest, d0, sizeof(d0));
		nxFoldInput(&inputDigest, o1, sizeof(o1));
		nxFoldInput(&inputDigest, d1, sizeof(d1));

		for(int mode = 0; mode < 2; ++mode)
			{
			NxReal out[2][6];
			memset(out, 0xcd, sizeof(out));
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			oracleFn(&out[0][0], &out[0][3], o0, d0, o1, d1);
			NxLineLineClosestPoints(&out[1][0], &out[1][3], o0, d0, o1, d1);
			nxSetControl(kControlDefault);
			if(mode == 0)
				{
				if(memcmp(&out[0][0], o0, 12) == 0)
					++atOrigin0;
				if(memcmp(&out[0][3], o1, 12) == 0)
					++atOrigin1;
				}
			nxFoldWords(&oracleDigest, &candidateDigest, &out[0][0], &out[1][0], 6,
				&perMode[mode], &canonical);
			}
		}
	totalMismatch += perMode[0];
	printf("collision name=line_line index=- rva=0x%08x owner=phys_fn_001692 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kLineLineRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state,
		perMode[0] + perMode[1]);
	nxPrintInput("line_line", &inputDigest);
	printf("collision coverage name=line_line parallel=%u zero_direction=%u crossing=%u at_origin0=%u at_origin1=%u mixed=%u canonical_nan=%u default_mismatches=%u simulate_mismatches=%u\n",
		parallel, zeroDirection, crossing, atOrigin0, atOrigin1, mixedDraws, canonical, perMode[0], perMode[1]);
	}

	{
	// Row phys_fn_001694, segment/triangle, which reaches both phys_fn_001690
	// and phys_fn_001672 (each side its own). Aimed: a finite triangle and a
	// segment whose ends are placed like point_triangle's points -- in each of
	// the seven regions of the plane, on vertices and edges, above and below
	// it -- so the ends straddle the plane or not and r falls below 0, on
	// [0, 1] or past 1; one draw in eight is parallel to the plane (both ends
	// at one height, the singular branch), one in sixteen of zero length, and
	// one in sixteen crossing the triangle's interior.
	const void* const oracleFn = (const void*) (base + kSegmentTriangleRva);
	const void* const candidateFn = (const void*) &NxSegmentTriangleSquareDistance;
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	unsigned perMode[2] = { 0, 0 };
	unsigned canonical = 0;
	unsigned mixedDraws = 0;
	unsigned degenerate = 0;
	unsigned parallel = 0;
	unsigned zeroLength = 0;
	unsigned crossing = 0;
	unsigned rStart = 0;
	unsigned rEnd = 0;
	unsigned rOpen = 0;
	unsigned sZero = 0;
	unsigned tZero = 0;
	unsigned intersecting = 0;
	unsigned nullOutputs = 0;
	unsigned state = 0x59d02c4bu;
	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		const unsigned draw = nxNext(&state) % 3;
		const bool aimed = draw == 1;
		const bool mixed = draw == 2;
		if(mixed)
			++mixedDraws;
		float v[3][3];
		if(nxFillTriangle(&state, aimed, mixed, v))
			++degenerate;
		NxSegment segment;
		if(aimed)
			{
			bool feature;
			nxTrianglePoint(&state, v, &segment.p0.x, &feature);
			nxTrianglePoint(&state, v, &segment.p1.x, &feature);
			const unsigned kind = nxNext(&state) & 15;
			if(kind < 2)
				{
				// Parallel: the second end moved to the first's height.
				float n[3];
				nxTriangleNormal(v, n);
				const float h0 = (segment.p0.x - v[0][0]) * n[0] + (segment.p0.y - v[0][1]) * n[1]
					+ (segment.p0.z - v[0][2]) * n[2];
				const float h1 = (segment.p1.x - v[0][0]) * n[0] + (segment.p1.y - v[0][1]) * n[1]
					+ (segment.p1.z - v[0][2]) * n[2];
				segment.p1.x -= (h1 - h0) * n[0];
				segment.p1.y -= (h1 - h0) * n[1];
				segment.p1.z -= (h1 - h0) * n[2];
				++parallel;
				}
			else if(kind == 2)
				{
				segment.p1 = segment.p0;
				++zeroLength;
				}
			else if(kind == 3)
				{
				// Through the interior: the ends either side of an inner point.
				float n[3];
				nxTriangleNormal(v, n);
				const float u = nxUnit(&state) * 0.5f;
				const float w = nxUnit(&state) * 0.5f;
				float inner[3];
				for(int k = 0; k < 3; ++k)
					inner[k] = v[0][k] + u * (v[1][k] - v[0][k]) + w * (v[2][k] - v[0][k]);
				const float up = nxUnit(&state) + 0.1f;
				const float down = nxUnit(&state) + 0.1f;
				float tilt[3];
				for(int k = 0; k < 3; ++k)
					tilt[k] = (nxUnit(&state) - 0.5f) * 0.5f;
				segment.p0.set(inner[0] + (n[0] + tilt[0]) * up, inner[1] + (n[1] + tilt[1]) * up,
					inner[2] + (n[2] + tilt[2]) * up);
				segment.p1.set(inner[0] - (n[0] + tilt[0]) * down, inner[1] - (n[1] + tilt[1]) * down,
					inner[2] - (n[2] + tilt[2]) * down);
				++crossing;
				}
			}
		else
			{
			float* words = &segment.p0.x;
			for(int k = 0; k < 6; ++k)
				nxDrawBits(&state, mixed, &words[k]);
			}
		const unsigned nulls = (nxNext(&state) % 8 == 0) ? (1 + nxNext(&state) % 7) : 0;
		if(nulls)
			++nullOutputs;
		nxFoldInput(&inputDigest, &segment, sizeof(segment));
		nxFoldInput(&inputDigest, v, sizeof(v));
		nxFoldInput(&inputDigest, &nulls, sizeof(nulls));

		for(int mode = 0; mode < 2; ++mode)
			{
			unsigned char wide[2][10];
			NxReal out[2][3];
			memset(out, 0xcd, sizeof(out));
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			nxCallWide7(oracleFn, &segment, v[0], v[1], v[2], (nulls & 1) ? 0 : &out[0][0],
				(nulls & 2) ? 0 : &out[0][1], (nulls & 4) ? 0 : &out[0][2], wide[0]);
			nxCallWide7(candidateFn, &segment, v[0], v[1], v[2], (nulls & 1) ? 0 : &out[1][0],
				(nulls & 2) ? 0 : &out[1][1], (nulls & 4) ? 0 : &out[1][2], wide[1]);
			nxSetControl(kControlDefault);
			if(mode == 0 && !nulls)
				{
				if(out[0][0] == 0.0f)
					++rStart;
				else if(out[0][0] == 1.0f)
					++rEnd;
				else if(out[0][0] > 0.0f && out[0][0] < 1.0f)
					++rOpen;
				if(out[0][1] == 0.0f)
					++sZero;
				if(out[0][2] == 0.0f)
					++tZero;
				if(wide[0][9] == 0 && wide[0][8] == 0)
					++intersecting;
				}
			nxFoldWide(&oracleDigest, &candidateDigest, wide[0], wide[1], &perMode[mode], &canonical);
			nxFoldWords(&oracleDigest, &candidateDigest, &out[0][0], &out[1][0], 3,
				&perMode[mode], &canonical);
			}
		}
	// The Task 2b review's draws, replayed as they were measured: 250,000 draws
	// from their own xorshift at 0x01234567, eighteen words each (a point, which
	// segment/triangle does not use, then the triangle, then the segment), every
	// word one time in four of mixed exponent (random sign and significand,
	// 2^-27..2^72) and otherwise uniform on [-4, 4), no null outputs. Two of them,
	// 171,426 and 237,762, are interior cancellations in which s cut to 53 bits
	// differs from the listing's wide s under 0x0f7f -- the C++ interior's spill
	// (convex-mesh gap harness hardening). No aimed construction reached that:
	// scaled, skinny, grazing and far-reaching crossings all left the C++ and the
	// assembly interiors agreeing on every draw.
	unsigned reviewState = 0x01234567u;
	for(unsigned i = 0; i < kSegmentTriangleReviewDraws; ++i)
		{
		float words[18];
		for(int k = 0; k < 18; ++k)
			{
			const unsigned choice = nxNext(&reviewState) & 7;
			if(choice == 2 || choice == 3)
				{
				const unsigned significand = nxNext(&reviewState) & 0x807fffffu;
				const unsigned bits = significand | ((100u + nxNext(&reviewState) % 100u) << 23);
				memcpy(&words[k], &bits, 4);
				}
			else
				words[k] = nxUnit(&reviewState) * 8.0f - 4.0f;
			}
		const float* const v = &words[3];
		NxSegment segment;
		memcpy(&segment, &words[12], sizeof(segment));
		nxFoldInput(&inputDigest, &segment, sizeof(segment));
		nxFoldInput(&inputDigest, v, 9 * sizeof(float));
		for(int mode = 0; mode < 2; ++mode)
			{
			unsigned char wide[2][10];
			NxReal out[2][3];
			memset(out, 0xcd, sizeof(out));
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			nxCallWide7(oracleFn, &segment, v, v + 3, v + 6, &out[0][0], &out[0][1], &out[0][2],
				wide[0]);
			nxCallWide7(candidateFn, &segment, v, v + 3, v + 6, &out[1][0], &out[1][1], &out[1][2],
				wide[1]);
			nxSetControl(kControlDefault);
			nxFoldWide(&oracleDigest, &candidateDigest, wide[0], wide[1], &perMode[mode], &canonical);
			nxFoldWords(&oracleDigest, &candidateDigest, &out[0][0], &out[1][0], 3,
				&perMode[mode], &canonical);
			}
		}
	totalMismatch += perMode[0];
	printf("collision name=segment_triangle index=- rva=0x%08x owner=phys_fn_001694 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kSegmentTriangleRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state,
		perMode[0] + perMode[1]);
	nxPrintInput("segment_triangle", &inputDigest);
	printf("collision coverage name=segment_triangle degenerate=%u parallel=%u zero_length=%u crossing=%u r_start=%u r_end=%u r_open=%u s_zero=%u t_zero=%u intersecting=%u null_outputs=%u mixed=%u canonical_nan=%u default_mismatches=%u simulate_mismatches=%u\n",
		degenerate, parallel, zeroLength, crossing, rStart, rEnd, rOpen, sZero, tZero, intersecting,
		nullOutputs, mixedDraws, canonical, perMode[0], perMode[1]);
	}

	{
	// Row phys_fn_001708, a ray against an inflated triangle fan. Each side reaches
	// its own Triangle::Inflate and NxRayTriIntersect. Aimed: a convex polygon of
	// 3..6 vertices in a random plane, the ray from off the plane towards a point
	// inside it, near its rim or past it, one draw in eight parallel to the plane
	// and one in sixteen with a count of 2 (no triangle).
	const NxRayFanFn oracleFn = (NxRayFanFn) (base + kRayInflatedFanRva);
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	unsigned perMode[2] = { 0, 0 };
	unsigned canonical = 0;
	unsigned mixedDraws = 0;
	unsigned hits = 0;
	unsigned misses = 0;
	unsigned countTwo = 0;
	unsigned inPlane = 0;
	unsigned inflateDivergent[2] = { 0, 0 };
	unsigned rayTriDivergent[2] = { 0, 0 };
	typedef bool(__cdecl* NxRayTriFn)(const NxVec3&, const NxVec3&, const NxVec3&, const NxVec3&,
		const NxVec3&, float&, float&, float&, bool);
	const NxRayTriFn oracleRayTri = (NxRayTriFn) (base + kRayTriIntersectRva);
	unsigned divergentMismatches[2] = { 0, 0 };
	// The fans that gate, per control word and per draw kind (raw, aimed, mixed).
	unsigned gatedFans[2][3] = { { 0, 0, 0 }, { 0, 0, 0 } };
	typedef void(__thiscall* NxInflateFn)(void*, float, bool);
	const NxInflateFn oracleInflate = (NxInflateFn) (base + kIceTriangleInflateRva);
	unsigned state = 0x0fa9d3e1u;
	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		const unsigned draw = nxNext(&state) % 3;
		const bool aimed = draw == 1;
		const bool mixed = draw == 2;
		if(mixed)
			++mixedDraws;
		NxVec3 vertices[8];
		NxU32 indices[8];
		NxU32 count = 3 + nxNext(&state) % 4;
		for(NxU32 k = 0; k < 8; ++k)
			indices[k] = (k + nxNext(&state)) % 8;
		NxRay ray;
		if(aimed)
			{
			float basis[3][3];
			for(int r = 0; r < 3; ++r)
				for(int k = 0; k < 3; ++k)
					basis[r][k] = nxUnit(&state) * 2.0f - 1.0f;
			float centre[3];
			for(int k = 0; k < 3; ++k)
				centre[k] = nxUnit(&state) * 4.0f - 2.0f;
			const float radius = nxUnit(&state) * 2.0f + 0.1f;
			for(NxU32 k = 0; k < 8; ++k)
				{
				const float angle = 6.2831853f * (float) k / (float) count + 0.3f;
				const float c = cosf(angle) * radius;
				const float s = sinf(angle) * radius;
				vertices[k].x = centre[0] + c * basis[0][0] + s * basis[1][0];
				vertices[k].y = centre[1] + c * basis[0][1] + s * basis[1][1];
				vertices[k].z = centre[2] + c * basis[0][2] + s * basis[1][2];
				}
			for(NxU32 k = 0; k < 8; ++k)
				indices[k] = k;
			const float reach = nxUnit(&state) * 1.3f;
			const float angle = nxUnit(&state) * 6.2831853f;
			float target[3];
			for(int k = 0; k < 3; ++k)
				target[k] = centre[k] + reach * radius * (cosf(angle) * basis[0][k] + sinf(angle) * basis[1][k]);
			const float lift = (nxNext(&state) & 7) == 0 ? 0.0f : (nxUnit(&state) * 6.0f - 3.0f);
			if(lift == 0.0f)
				++inPlane;
			const float origin[3] = {
				target[0] + lift * basis[2][0] + (nxUnit(&state) - 0.5f) * basis[0][0],
				target[1] + lift * basis[2][1] + (nxUnit(&state) - 0.5f) * basis[0][1],
				target[2] + lift * basis[2][2] + (nxUnit(&state) - 0.5f) * basis[0][2] };
			ray.orig.set(origin[0], origin[1], origin[2]);
			ray.dir.set(target[0] - origin[0], target[1] - origin[1], target[2] - origin[2]);
			if(nxNext(&state) & 1)
				ray.dir.normalize();
			}
		else
			{
			for(NxU32 k = 0; k < 8; ++k)
				{
				nxDrawBits(&state, mixed, &vertices[k].x);
				nxDrawBits(&state, mixed, &vertices[k].y);
				nxDrawBits(&state, mixed, &vertices[k].z);
				}
			nxDrawBits(&state, mixed, &ray.orig.x);
			nxDrawBits(&state, mixed, &ray.orig.y);
			nxDrawBits(&state, mixed, &ray.orig.z);
			nxDrawBits(&state, mixed, &ray.dir.x);
			nxDrawBits(&state, mixed, &ray.dir.y);
			nxDrawBits(&state, mixed, &ray.dir.z);
			}
		if((nxNext(&state) & 15) == 0)
			{
			count = 2;
			++countTwo;
			}
		nxFoldInput(&inputDigest, &count, sizeof(count));
		nxFoldInput(&inputDigest, vertices, sizeof(vertices));
		nxFoldInput(&inputDigest, indices, sizeof(indices));
		nxFoldInput(&inputDigest, &ray, sizeof(ray));

		for(int mode = 0; mode < 2; ++mode)
			{
			// The pre-flight: every triangle of the fan inflated by both sides'
			// Triangle::Inflate under this word, and the oracle-inflated triangle
			// put through both sides' NxRayTriIntersect. A fan on which either
			// callee differs is counted apart and does not gate (see
			// kCalleeDivergentFanCeiling).
			bool inflateSame = true;
			bool rayTriSame = true;
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			for(NxU32 k = 0; k + 2 < count; ++k)
				{
				float inflated[2][9];
				memcpy(&inflated[0][0], &vertices[indices[0]], sizeof(NxVec3));
				memcpy(&inflated[0][3], &vertices[indices[k + 1]], sizeof(NxVec3));
				memcpy(&inflated[0][6], &vertices[indices[k + 2]], sizeof(NxVec3));
				memcpy(&inflated[1], &inflated[0], sizeof(inflated[0]));
				oracleInflate(&inflated[0], 0.02f, false);
				nxCandidateTriangleInflate(inflated[1], 0.02f, false);
				if(memcmp(&inflated[0], &inflated[1], sizeof(inflated[0])) != 0)
					inflateSame = false;
				float hit[2][3];
				memset(hit, 0xcd, sizeof(hit));
				const bool rayTri0 = oracleRayTri(ray.orig, ray.dir, *(const NxVec3*) &inflated[0][0],
					*(const NxVec3*) &inflated[0][3], *(const NxVec3*) &inflated[0][6],
					hit[0][0], hit[0][1], hit[0][2], false);
				const bool rayTri1 = NxRayTriIntersect(ray.orig, ray.dir, *(const NxVec3*) &inflated[0][0],
					*(const NxVec3*) &inflated[0][3], *(const NxVec3*) &inflated[0][6],
					hit[1][0], hit[1][1], hit[1][2], false);
				// Compared as the family's own words are folded: a NaN is any
				// NaN (nxCanonicalNarrow), everything else bit for bit.
				bool sameHit = rayTri0 == rayTri1;
				for(int w = 0; w < 3; ++w)
					{
					NxU32 a = nxBits(hit[0][w]);
					NxU32 b = nxBits(hit[1][w]);
					nxCanonicalNarrow(&a);
					nxCanonicalNarrow(&b);
					if(a != b)
						sameHit = false;
					}
				if(!sameHit)
					rayTriSame = false;
				}
			NxReal t[2];
			memset(t, 0xcd, sizeof(t));
			const bool hit0 = oracleFn(count, vertices, indices, &ray, &t[0]);
			const bool hit1 = NxRayInflatedTriangleFan(count, vertices, indices, &ray, &t[1]);
			nxSetControl(kControlDefault);
			if(mode == 0)
				++*(hit0 ? &hits : &misses);
			if(!inflateSame)
				++inflateDivergent[mode];
			else if(!rayTriSame)
				++rayTriDivergent[mode];
			const bool exact = inflateSame && rayTriSame;
			if(exact)
				++gatedFans[mode][draw];
			unsigned* const counter = exact ? &perMode[mode] : &divergentMismatches[mode];
			nxDigestByte(&oracleDigest, hit0 ? 1 : 0);
			nxDigestByte(&candidateDigest, hit1 ? 1 : 0);
			if(hit0 != hit1)
				++*counter;
			nxFoldWords(&oracleDigest, &candidateDigest, &t[0], &t[1], 1, counter, &canonical);
			}
		}
	totalMismatch += perMode[0];
	printf("collision name=ray_inflated_tris index=- rva=0x%08x owner=phys_fn_001708 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kRayInflatedFanRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state,
		perMode[0] + perMode[1]);
	nxPrintInput("ray_inflated_tris", &inputDigest);
	printf("collision coverage name=ray_inflated_tris hits=%u misses=%u count_two=%u in_plane=%u inflate_divergent=%u inflate_divergent_simulate=%u raytri_divergent=%u raytri_divergent_simulate=%u mixed=%u canonical_nan=%u default_mismatches=%u simulate_mismatches=%u\n",
		hits, misses, countTwo, inPlane, inflateDivergent[0], inflateDivergent[1], rayTriDivergent[0], rayTriDivergent[1], mixedDraws, canonical, perMode[0], perMode[1]);
	printf("collision gated name=ray_inflated_tris raw=%u aimed=%u mixed=%u raw_simulate=%u aimed_simulate=%u mixed_simulate=%u\n",
		gatedFans[0][0], gatedFans[0][1], gatedFans[0][2], gatedFans[1][0], gatedFans[1][1],
		gatedFans[1][2]);
	// The fans on which a callee differs, and the words that differ on them under
	// each word; enforced ceilings (they may fall, never rise).
	const unsigned calleeFans[2] = { inflateDivergent[0] + rayTriDivergent[0],
		inflateDivergent[1] + rayTriDivergent[1] };
	printf("collision divergent name=ray_inflated_tris cause=phys_fn_005185,phys_fn_001712 fans=%u fans_simulate=%u default_mismatches=%u simulate_mismatches=%u\n",
		calleeFans[0], calleeFans[1], divergentMismatches[0], divergentMismatches[1]);
	if(calleeFans[0] > kCalleeDivergentFanCeiling[0]
		|| calleeFans[1] > kCalleeDivergentFanCeiling[1]
		|| divergentMismatches[0] > kCalleeDivergentWordCeiling[0]
		|| divergentMismatches[1] > kCalleeDivergentWordCeiling[1])
		{
		printf("collision divergent name=ray_inflated_tris ceiling=exceeded\n");
		++totalMismatch;
		}
	}

	{
	// Row phys_fn_001730 (and its continuation phys_fn_001732), the slab test.
	// Aimed: a finite box (one draw in eight inverted on one axis), an origin
	// in or around it, and a direction whose components are each, with some
	// probability, zero, inside (-FLT_EPSILON, FLT_EPSILON), exactly
	// +-FLT_EPSILON (the boundary, which divides), or ordinary.
	const NxSlabFn oracleFn = (NxSlabFn) (base + kRayAabbSlabRva);
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	unsigned perMode[2] = { 0, 0 };
	unsigned canonical = 0;
	unsigned mixedDraws = 0;
	unsigned faces[7] = { 0, 0, 0, 0, 0, 0, 0 };
	unsigned parallelAxes = 0;
	unsigned boundaryAxes = 0;
	unsigned inverted = 0;
	unsigned state = 0x2d5c93a7u;
	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		const unsigned draw = nxNext(&state) % 3;
		const bool aimed = draw == 1;
		const bool mixed = draw == 2;
		if(mixed)
			++mixedDraws;
		float boxMin[3], boxMax[3], origin[3], dir[3];
		if(aimed)
			{
			for(int k = 0; k < 3; ++k)
				{
				const float c = nxUnit(&state) * 4.0f - 2.0f;
				const float e = nxUnit(&state) * 2.0f;
				boxMin[k] = c - e;
				boxMax[k] = c + e;
				origin[k] = c + (nxUnit(&state) * 2.0f - 1.0f) * e * 2.0f;
				const unsigned kind = nxNext(&state) % 8;
				if(kind == 0)
					dir[k] = (nxNext(&state) & 1) ? -0.0f : 0.0f;
				else if(kind == 1)
					dir[k] = (nxUnit(&state) * 2.0f - 1.0f) * 1.1920928e-7f * 0.99f;
				else if(kind == 2)
					dir[k] = (nxNext(&state) & 1) ? -1.1920928955078125e-7f : 1.1920928955078125e-7f;
				else
					dir[k] = nxUnit(&state) * 4.0f - 2.0f;
				if(kind <= 1)
					++parallelAxes;
				else if(kind == 2)
					++boundaryAxes;
				}
			if((nxNext(&state) & 7) == 0)
				{
				const unsigned k = nxNext(&state) % 3;
				const float swap = boxMin[k];
				boxMin[k] = boxMax[k];
				boxMax[k] = swap;
				++inverted;
				}
			}
		else
			for(int k = 0; k < 3; ++k)
				{
				nxDrawBits(&state, mixed, &boxMin[k]);
				nxDrawBits(&state, mixed, &boxMax[k]);
				nxDrawBits(&state, mixed, &origin[k]);
				nxDrawBits(&state, mixed, &dir[k]);
				}
		nxFoldInput(&inputDigest, boxMin, sizeof(boxMin));
		nxFoldInput(&inputDigest, boxMax, sizeof(boxMax));
		nxFoldInput(&inputDigest, origin, sizeof(origin));
		nxFoldInput(&inputDigest, dir, sizeof(dir));

		for(int mode = 0; mode < 2; ++mode)
			{
			NxReal out[2][2];
			memset(out, 0xcd, sizeof(out));
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			const int face0 = oracleFn(boxMin, boxMax, origin, dir, &out[0][0], &out[0][1]);
			const int face1 = NxRayAABBSlab(boxMin, boxMax, origin, dir, &out[1][0], &out[1][1]);
			nxSetControl(kControlDefault);
			if(mode == 0)
				++faces[face0 >= 0 && face0 < 6 ? face0 + 1 : 0];
			nxDigestByte(&oracleDigest, (unsigned char) face0);
			nxDigestByte(&candidateDigest, (unsigned char) face1);
			if(face0 != face1)
				++perMode[mode];
			nxFoldWords(&oracleDigest, &candidateDigest, &out[0][0], &out[1][0], 2, &perMode[mode], &canonical);
			}
		}
	totalMismatch += perMode[0];
	printf("collision name=aabb_slab index=- rva=0x%08x owner=phys_fn_001730 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kRayAabbSlabRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state,
		perMode[0] + perMode[1]);
	nxPrintInput("aabb_slab", &inputDigest);
	printf("collision coverage name=aabb_slab miss=%u face0=%u face1=%u face2=%u face3=%u face4=%u face5=%u parallel_axes=%u boundary_axes=%u inverted=%u mixed=%u canonical_nan=%u default_mismatches=%u simulate_mismatches=%u\n",
		faces[0], faces[1], faces[2], faces[3], faces[4], faces[5], faces[6], parallelAxes,
		boundaryAxes, inverted, mixedDraws, canonical, perMode[0], perMode[1]);
	}

	{
	// Row phys_fn_001760, the triangle plane (__thiscall, called as __fastcall on
	// both sides). The plane is poisoned first so every word it leaves is one it
	// wrote; the returned pointer must be the plane.
	const NxTrianglePlaneFn oracleFn = (NxTrianglePlaneFn) (base + kTrianglePlaneRva);
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	unsigned perMode[2] = { 0, 0 };
	unsigned canonical = 0;
	unsigned mixedDraws = 0;
	unsigned degenerate = 0;
	unsigned zeroNormal = 0;
	unsigned wrongReturn = 0;
	unsigned state = 0x71e3a90bu;
	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		const unsigned draw = nxNext(&state) % 3;
		const bool aimed = draw == 1;
		const bool mixed = draw == 2;
		if(mixed)
			++mixedDraws;
		float v[3][3];
		if(nxFillTriangle(&state, aimed, mixed, v))
			++degenerate;
		nxFoldInput(&inputDigest, v, sizeof(v));
		for(int mode = 0; mode < 2; ++mode)
			{
			NxPlane plane[2];
			memset(plane, 0xcd, sizeof(plane));
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			NxPlane* r0 = oracleFn(&plane[0], 0, (const NxVec3*) v[0], (const NxVec3*) v[1], (const NxVec3*) v[2]);
			NxPlane* r1 = NxTrianglePlane(&plane[1], 0, (const NxVec3*) v[0], (const NxVec3*) v[1], (const NxVec3*) v[2]);
			nxSetControl(kControlDefault);
			if(r0 != &plane[0] || r1 != &plane[1])
				++wrongReturn;
			if(mode == 0 && plane[0].normal.x == 0.0f && plane[0].normal.y == 0.0f && plane[0].normal.z == 0.0f)
				++zeroNormal;
			nxFoldWords(&oracleDigest, &candidateDigest, &plane[0].normal.x, &plane[1].normal.x, 4,
				&perMode[mode], &canonical);
			}
		}
	totalMismatch += perMode[0] + wrongReturn;
	printf("collision name=triangle_plane index=- rva=0x%08x owner=phys_fn_001760 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kTrianglePlaneRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state,
		perMode[0] + perMode[1]);
	nxPrintInput("triangle_plane", &inputDigest);
	printf("collision coverage name=triangle_plane degenerate=%u zero_normal=%u wrong_return=%u mixed=%u canonical_nan=%u default_mismatches=%u simulate_mismatches=%u\n",
		degenerate, zeroNormal, wrongReturn, mixedDraws, canonical, perMode[0], perMode[1]);
	}

	{
	// Row phys_fn_001855, a segment against a triangle edge. The edge is two
	// vertices of a triangle and the axis its unit normal, as at the callers.
	// Aimed: the segment through a point near the edge's plane (on the edge,
	// past its ends, or off to either side along the axis), crossing it, one
	// draw in eight parallel to it and one in sixteen of zero length. `t` and
	// `hit` are poisoned, so which exit ran is read off what the oracle wrote.
	const NxSegmentEdgeFn oracleFn = (NxSegmentEdgeFn) (base + kSegmentTriangleEdgeRva);
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	unsigned perMode[2] = { 0, 0 };
	unsigned canonical = 0;
	unsigned mixedDraws = 0;
	unsigned onEdge = 0;
	unsigned exitEarly = 0;
	unsigned exitBehind = 0;
	unsigned exitOutside = 0;
	unsigned parallel = 0;
	unsigned zeroLength = 0;
	unsigned state = 0x4b8e2f61u;
	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		const unsigned draw = nxNext(&state) % 3;
		const bool aimed = draw == 1;
		const bool mixed = draw == 2;
		if(mixed)
			++mixedDraws;
		float v[3][3];
		nxFillTriangle(&state, aimed, mixed, v);
		float axis[3];
		float s0[3], s1[3];
		const unsigned a = nxNext(&state) % 3;
		const unsigned b = (a + 1) % 3;
		if(aimed)
			{
			nxTriangleNormal(v, axis);
			float edge[3];
			for(int k = 0; k < 3; ++k)
				edge[k] = v[b][k] - v[a][k];
			// The edge plane's normal, edge x axis, placed by the harness only.
			float across[3] = {
				edge[1] * axis[2] - edge[2] * axis[1],
				edge[2] * axis[0] - edge[0] * axis[2],
				edge[0] * axis[1] - edge[1] * axis[0] };
			const float along = nxUnit(&state) * 2.0f - 0.5f;
			const float up = (nxUnit(&state) * 2.0f - 1.0f) * ((nxNext(&state) & 1) ? 1.0f : 0.1f);
			float p[3];
			for(int k = 0; k < 3; ++k)
				p[k] = v[a][k] + along * edge[k] + up * axis[k];
			float d[3];
			for(int k = 0; k < 3; ++k)
				d[k] = nxUnit(&state) * 2.0f - 1.0f;
			const unsigned kind = nxNext(&state) & 15;
			if(kind < 2)
				{
				// Parallel to the edge plane: no component along `across`.
				const float aa = across[0] * across[0] + across[1] * across[1] + across[2] * across[2];
				if(aa > 0.0f)
					{
					const float k0 = (d[0] * across[0] + d[1] * across[1] + d[2] * across[2]) / aa;
					for(int k = 0; k < 3; ++k)
						d[k] -= k0 * across[k];
					}
				++parallel;
				}
			else
				for(int k = 0; k < 3; ++k)
					d[k] += across[k] * (nxUnit(&state) * 4.0f - 2.0f);
			const float l0 = nxUnit(&state) * 1.5f + 0.05f;
			const float l1 = nxUnit(&state) * 1.5f + 0.05f;
			for(int k = 0; k < 3; ++k)
				{
				s0[k] = p[k] + d[k] * l0;
				s1[k] = p[k] - d[k] * l1;
				}
			if(kind == 2)
				{
				for(int k = 0; k < 3; ++k)
					s1[k] = s0[k];
				++zeroLength;
				}
			}
		else
			for(int k = 0; k < 3; ++k)
				{
				nxDrawBits(&state, mixed, &axis[k]);
				nxDrawBits(&state, mixed, &s0[k]);
				nxDrawBits(&state, mixed, &s1[k]);
				}
		nxFoldInput(&inputDigest, v[a], sizeof(v[a]));
		nxFoldInput(&inputDigest, v[b], sizeof(v[b]));
		nxFoldInput(&inputDigest, axis, sizeof(axis));
		nxFoldInput(&inputDigest, s0, sizeof(s0));
		nxFoldInput(&inputDigest, s1, sizeof(s1));

		for(int mode = 0; mode < 2; ++mode)
			{
			NxReal out[2][4];
			memset(out, 0xcd, sizeof(out));
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			const bool r0 = oracleFn(v[a], v[b], axis, s0, s1, &out[0][0], &out[0][1]);
			const bool r1 = NxSegmentTriangleEdge(v[a], v[b], axis, s0, s1, &out[1][0], &out[1][1]);
			nxSetControl(kControlDefault);
			if(mode == 0)
				{
				NxU32 tWord;
				memcpy(&tWord, &out[0][0], 4);
				NxU32 hitWord;
				memcpy(&hitWord, &out[0][1], 4);
				if(r0)
					++onEdge;
				else if(hitWord == 0xcdcdcdcdu)
					++exitEarly;
				else if(tWord != 0xcdcdcdcdu && out[0][0] < 0.0f)
					++exitBehind;
				else
					++exitOutside;
				}
			nxDigestByte(&oracleDigest, r0 ? 1 : 0);
			nxDigestByte(&candidateDigest, r1 ? 1 : 0);
			if(r0 != r1)
				++perMode[mode];
			nxFoldWords(&oracleDigest, &candidateDigest, &out[0][0], &out[1][0], 4, &perMode[mode], &canonical);
			}
		}
	totalMismatch += perMode[0];
	printf("collision name=segment_triangle_edges index=- rva=0x%08x owner=phys_fn_001855 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kSegmentTriangleEdgeRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state,
		perMode[0] + perMode[1]);
	nxPrintInput("segment_triangle_edges", &inputDigest);
	printf("collision coverage name=segment_triangle_edges on_edge=%u exit_early=%u exit_behind=%u exit_outside=%u parallel=%u zero_length=%u mixed=%u canonical_nan=%u default_mismatches=%u simulate_mismatches=%u\n",
		onEdge, exitEarly, exitBehind, exitOutside, parallel, zeroLength, mixedDraws, canonical, perMode[0], perMode[1]);
	}

	return totalMismatch;
	}

// The kernel fuzz harness's `.snan` variants. tests/PhysicsKernelFuzzTests.cpp is a
// staged-pair differential -- one binary run against the shipped pair and against
// the rebuilt pair, the two transcripts compared line for line -- so it cannot
// carry a line on which the two sides are known to differ. Its families draw their
// NaNs quiet (exact, its lines unchanged); the three whose candidates differ once
// the signalling NaNs are kept are measured here instead, where the oracle is in
// process: the fuzz harness's own draws (its xorshift, seeds and mixture,
// replayed word for word by nxFuzzPickWord), both control words, the candidates
// as linked from Geometry.cpp.
static void nxFuzzPickWord(unsigned* state, unsigned mode, float* out)
	{
	NxU32 bits;
	switch(mode & 7)
		{
		case 0:
			bits = nxNext(state);
			break;
		case 1:
			{
			const float value = (float) ((int) (nxNext(state) % 9) - 4);
			memcpy(&bits, &value, 4);
			break;
			}
		case 2:
			{
			const float value = (float) ((int) (nxNext(state) % 9) - 4) * 0.5f;
			memcpy(&bits, &value, 4);
			break;
			}
		case 7:
			bits = 0;
			break;
		default:
			{
			const NxU32 exponent = (mode & 7) == 6
				? ((110 + (nxNext(state) % 35)) & 0xff) << 23
				: ((120 + (nxNext(state) % 16)) & 0xff) << 23;
			const NxU32 significand = nxNext(state) & 0x7fffff;
			bits = exponent | significand | ((nxNext(state) & 1) << 31);
			break;
			}
		}
	memcpy(out, &bits, 4);
	}

// The fuzz harness's output poison, cdcd0000 + index.
static void nxFuzzPoison(float* out, unsigned count)
	{
	for(unsigned k = 0; k < count; ++k)
		{
		const NxU32 word = 0xcdcd0000u + k;
		memcpy(&out[k], &word, 4);
		}
	}

static __declspec(noinline) unsigned nxDriveFuzzSnan(HMODULE physics)
	{
	typedef unsigned char (NX_CALL_CONV* NxFnRayPlane)(const float*, const float*, float*, float*);
	typedef unsigned char (NX_CALL_CONV* NxFnRayAABB)(const float*, const float*, const float*,
		const float*, float*);
	typedef unsigned char (NX_CALL_CONV* NxFnSegBox)(const float*, const float*, const float*,
		const float*, float*);
	const NxFnRayPlane oracleRayPlane = (NxFnRayPlane) GetProcAddress(physics, "NxRayPlaneIntersect");
	const NxFnRayAABB oracleRayAabb = (NxFnRayAABB) GetProcAddress(physics, "NxRayAABBIntersect");
	const NxFnSegBox oracleSegBox = (NxFnSegBox) GetProcAddress(physics, "NxSegmentBoxIntersect");
	if(!oracleRayPlane || !oracleRayAabb || !oracleSegBox)
		return 1;
	unsigned totalMismatch = 0;

	// NxRayPlaneIntersect: the fuzz vector block (seed 02468ace, 15 words a draw,
	// 40,000 draws); the ray is words 0..5 and the plane words 6..9.
	for(int kernel = 0; kernel < 3; ++kernel)
		{
		static const char* const names[3] = { "fuzz_ray_plane", "fuzz_ray_aabb", "fuzz_segment_box" };
		static const char* const owners[3] = { "phys_fn_001704", "phys_fn_001722", "phys_fn_001714" };
		NxSnanPass snanScope(1);
		NxDigest oracleDigest, candidateDigest, inputDigest;
		nxDigestInit(&oracleDigest);
		nxDigestInit(&candidateDigest);
		nxDigestInit(&inputDigest);
		unsigned mismatches = 0;
		unsigned hits = 0;
		unsigned state = kernel == 0 ? 0x02468aceu : 0x1a2b3c4du;
		const unsigned wordsPerDraw = kernel == 0 ? 15u : 24u;
		for(unsigned i = 0; i < 40000; ++i)
			{
			float w[24];
			for(unsigned k = 0; k < wordsPerDraw; ++k)
				nxFuzzPickWord(&state, i + k, &w[k]);
			nxFoldInput(&inputDigest, w, wordsPerDraw * 4);
			for(int mode = 0; mode < 2; ++mode)
				{
				float out[2][4];
				nxFuzzPoison(out[0], 4);
				nxFuzzPoison(out[1], 4);
				unsigned char result[2];
				nxSetControl(mode ? kControlSimulate : kControlDefault);
				if(kernel == 0)
					{
					result[0] = oracleRayPlane(w + 0, w + 6, &out[0][3], out[0]);
					result[1] = NxRayPlaneIntersect(*(const NxRay*) (w + 0), *(const NxPlane*) (w + 6),
						out[1][3], *(NxVec3*) out[1]) ? 1 : 0;
					}
				else if(kernel == 1)
					{
					result[0] = oracleRayAabb(w + 0, w + 3, w + 6, w + 9, out[0]);
					result[1] = NxRayAABBIntersect(*(const NxVec3*) (w + 0), *(const NxVec3*) (w + 3),
						*(const NxVec3*) (w + 6), *(const NxVec3*) (w + 9), *(NxVec3*) out[1]) ? 1 : 0;
					}
				else
					{
					result[0] = oracleSegBox(w + 0, w + 3, w + 6, w + 9, out[0]);
					result[1] = NxSegmentBoxIntersect(*(const NxVec3*) (w + 0), *(const NxVec3*) (w + 3),
						*(const NxVec3*) (w + 6), *(const NxVec3*) (w + 9), *(NxVec3*) out[1]) ? 1 : 0;
					}
				nxSetControl(kControlDefault);
				result[0] = result[0] ? 1 : 0;
				if(mode == 0 && result[0])
					++hits;
				nxDigestByte(&oracleDigest, result[0]);
				nxDigestByte(&candidateDigest, result[1]);
				if(result[0] != result[1])
					++mismatches;
				nxSnanDiscrete(mode, result[0], result[1]);
				const unsigned count = kernel == 0 ? 4u : 3u;
				for(unsigned k = 0; k < count; ++k)
					{
					const NxU32 a = nxBits(out[0][k]);
					const NxU32 b = nxBits(out[1][k]);
					for(int byte = 0; byte < 4; ++byte)
						{
						nxDigestByte(&oracleDigest, (unsigned char) (a >> (byte * 8)));
						nxDigestByte(&candidateDigest, (unsigned char) (b >> (byte * 8)));
						}
					if(a != b)
						++mismatches;
					nxSnanWord(mode, a, b);
					}
				}
			}
		totalMismatch += nxSnanGate(names[kernel], mismatches);
		printf("collision name=%s index=- rva=export owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
			nxSnanName(names[kernel]), owners[kernel], oracleDigest.checks, oracleDigest.state,
			candidateDigest.state, mismatches);
		nxPrintInput(nxSnanName(names[kernel]), &inputDigest);
		printf("collision coverage name=%s hits=%u\n", nxSnanName(names[kernel]), hits);
		}
	return totalMismatch;
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
		nxDigestInit(&random.input);
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
				if(tame)
					shape0->translation[k] = nxUnit(&state) * 6.0f - 3.0f;
				else
					nxPickWord(&state, &shape0->translation[k]);
				if(tame)
					shape1->translation[k] = nxUnit(&state) * 6.0f - 3.0f;
				else
					nxPickWord(&state, &shape1->translation[k]);
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
		printf("collision input name=%s.random words=%u input=%016llx\n",
			nxDriven[entry].name, random.input.checks / 4, random.input.state);
		printf("collision name=%s.aimed index=%u rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
			nxDriven[entry].name, index, rva, nxMatrixB[index].stableId,
			aimed.oracle.checks, aimed.oracle.state, aimed.candidate.state, aimed.mismatches);
		printf("collision coverage name=%s.aimed true=%u false=%u swap_differs=%u\n",
			nxDriven[entry].name, aimed.trueCount, aimed.falseCount, aimed.swapDiffers);
		printf("collision input name=%s.aimed words=%u input=%016llx\n",
			nxDriven[entry].name, aimed.input.checks / 4, aimed.input.state);
		}

	// -----------------------------------------------------------------------
	// The two helper rows, driven at their own recorded addresses as well as
	// through their callers. phys_fn_000943 is the only kernel in this
	// component that writes floats rather than returning a bool, so it is the
	// only place the NaN payload rule -- the thing /arch:IA32 exists for -- can
	// be observed at all. Its output is poisoned before every call so that
	// "wrote nothing" and "wrote zero" are different transcripts.
	for(int snanPass = 0; snanPass < 2; ++snanPass)
	{
	NxSnanPass snanScope(snanPass);
	typedef void(__thiscall* NxOracleCornerFn)(const NxCollisionShape*, int, int, int, NxVec3*);
	NxOracleCornerFn oracleCorner = (NxOracleCornerFn) (base + 0x00020750);

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	unsigned mismatches = 0;
	unsigned nonFinite = 0;
	unsigned state = 0x1337c0deu;
	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		bool tame = (nxNext(&state) & 3) != 0;
		nxIdentity(shape0);
		nxRandomRotation(&state, shape0);
		for(int k = 0; k < 3; ++k)
			if(tame)
				shape0->translation[k] = nxUnit(&state) * 6.0f - 3.0f;
			else
				nxPickWord(&state, &shape0->translation[k]);
		nxFillGeometry(&state, shape0, 2, tame);
		int signX = (nxNext(&state) & 1) ? 1 : -1;
		int signY = (nxNext(&state) & 1) ? 1 : -1;
		int signZ = (nxNext(&state) & 1) ? 1 : -1;
		nxFoldInputShape(&inputDigest, shape0);
		const int signs[3] = { signX, signY, signZ };
		nxFoldInput(&inputDigest, signs, sizeof(signs));

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
				nxSnanWord(mode, a, b);
				if((a & 0x7f800000u) == 0x7f800000u)
					++nonFinite;
				}
			}
		}
	totalMismatch += nxSnanGate("box_corner", mismatches);
	printf("collision name=%s index=- rva=0x00020750 owner=phys_fn_000943 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n", nxSnanName("box_corner"),
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	nxPrintInput(nxSnanName("box_corner"), &inputDigest);
	printf("collision coverage name=%s non_finite_words=%u\n", nxSnanName("box_corner"), nonFinite);
	}

	{
	typedef bool(__cdecl* NxOracleSphereBoxFn)(const NxCollisionSphereData*, const NxCollisionBoxData*);
	NxOracleSphereBoxFn oracleSphereBox = (NxOracleSphereBoxFn) (base + 0x00049ca0);

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
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
			if(tame)
				box.center[k] = nxUnit(&state) * 2.0f - 1.0f;
			else
				nxPickWord(&state, &box.center[k]);
			box.extents[k] = shape1->geometry[k + 1];
			}
		for(int k = 0; k < 9; ++k)
			box.rotation[k] = shape1->rotation[k];
		if(tame)
			sphere.radius = nxUnit(&state) * 1.5f + 0.02f;
		else
			nxPickWord(&state, &sphere.radius);

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
				if(tame)
					sphere.center[k] = nxUnit(&state) * 4.0f - 2.0f;
				else
					nxPickWord(&state, &sphere.center[k]);
			}
		nxFoldInput(&inputDigest, &sphere, sizeof(sphere));
		nxFoldInput(&inputDigest, &box, sizeof(box));

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
	nxPrintInput("sphere_box_data", &inputDigest);
	printf("collision coverage name=sphere_box_data true=%u centre_inside=%u\n", trueCount, insideCount);
	}

	// -----------------------------------------------------------------------
	// phys_fn_001739 at 0x00038a90, the leaf of the box/box subtree.
	//
	// The containment test accepts one winding only -- every one of the four
	// cross products has to be strictly negative -- so a random quadruple
	// essentially never reaches the interpolation. Half the tame draws are
	// therefore built as a rectangle wound the way the test accepts, with the
	// point placed inside it, and the other half is the reversed winding, which
	// is rejected at the very first edge. Both are needed: without the first the
	// whole second half of the row is dead, and without the second the loop
	// never exits early.
	for(int snanPass = 0; snanPass < 2; ++snanPass)
	{
	NxSnanPass snanScope(snanPass);
	const void* oracleQuadDepth = (const void*) (base + 0x00038a90);
	// -1.0f as x87 leaves it in st(0): significand 0x8000000000000000,
	// exponent 0x3fff, sign set.
	static const unsigned char kOutside[10] =
		{ 0, 0, 0, 0, 0, 0, 0, 0x80, 0xff, 0xbf };

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	unsigned mismatches = 0;
	unsigned perMode[2] = { 0, 0 };
	unsigned aimedInside = 0;
	unsigned reversed = 0;
	unsigned interpolated = 0;
	unsigned nonFinite = 0;
	unsigned state = 0x71dbeef3u;
	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		const bool tame = (nxNext(&state) & 3) != 0;
		NxVec3 corner[4];
		NxReal pointY, pointZ;

		if(tame)
			{
			// Every operand below is finite by construction, so nothing here
			// multiplies a value it also allows to be non-finite.
			const NxReal y0 = nxUnit(&state) * 4.0f - 2.0f;
			const NxReal z0 = nxUnit(&state) * 4.0f - 2.0f;
			const NxReal y1 = y0 + nxUnit(&state) * 3.0f + 0.05f;
			const NxReal z1 = z0 + nxUnit(&state) * 3.0f + 0.05f;
			const bool accepted = (nxNext(&state) & 1) != 0;
			// quad[0] (y0,z0), quad[1] (y0,z1), quad[2] (y1,z1), quad[3] (y1,z0)
			// is the winding whose four cross products are all negative for an
			// interior point; the reverse of it is rejected at index 0.
			const NxReal ys[4] = { y0, y0, y1, y1 };
			const NxReal zs[4] = { z0, z1, z1, z0 };
			for(int k = 0; k < 4; ++k)
				{
				const int slot = accepted ? k : (3 - k);
				corner[slot].x = nxUnit(&state) * 4.0f - 2.0f;
				corner[slot].y = ys[k];
				corner[slot].z = zs[k];
				}
			if(!accepted)
				++reversed;

			if((nxNext(&state) & 1) != 0)
				{
				pointY = y0 + nxUnit(&state) * (y1 - y0);
				pointZ = z0 + nxUnit(&state) * (z1 - z0);
				if(accepted)
					++aimedInside;
				}
			else
				{
				// Outside on a random side, which is what walks the loop to a
				// different index before it leaves.
				pointY = nxUnit(&state) * 8.0f - 4.0f;
				pointZ = nxUnit(&state) * 8.0f - 4.0f;
				}
			}
		else
			{
			for(int k = 0; k < 4; ++k)
				{
				nxPickWord(&state, &corner[k].x);
				nxPickWord(&state, &corner[k].y);
				nxPickWord(&state, &corner[k].z);
				}
			nxPickWord(&state, &pointY);
			nxPickWord(&state, &pointZ);
			}

		const NxVec3* quad[4] = { &corner[0], &corner[1], &corner[2], &corner[3] };
		NxU32 pointYBits, pointZBits;
		memcpy(&pointYBits, &pointY, 4);
		memcpy(&pointZBits, &pointZ, 4);
		nxFoldInput(&inputDigest, corner, sizeof(corner));
		nxFoldInput(&inputDigest, &pointYBits, 4);
		nxFoldInput(&inputDigest, &pointZBits, 4);

		for(int mode = 0; mode < 2; ++mode)
			{
			unsigned char wide[2][10];
			memset(wide, 0xcd, sizeof(wide));

			nxSetControl(mode ? kControlSimulate : kControlDefault);
			nxCallQuadDepthOracle(oracleQuadDepth, quad, pointYBits, pointZBits, wide[0]);
			nxCallQuadDepthCandidate(NxBoxBoxQuadDepth, quad, pointYBits, pointZBits, wide[1]);
			nxSetControl(kControlDefault);

			// Classified from the oracle's own answer: -1.0f is the only value
			// the containment test can return, so anything else says the
			// interpolation ran.
			if(memcmp(wide[0], kOutside, 10) != 0)
				{
				++interpolated;
				if((wide[0][9] & 0x7f) == 0x7f && wide[0][8] == 0xff)
					++nonFinite;
				}

			// NOT canonicalised, unlike segment_segment: removing the
			// canonicalisation entirely leaves mismatches=0, so the two sides
			// agree on the NaN payloads as well as on which results are NaNs,
			// and a filter that hides nothing here would only weaken the block.
			// 12,737 of the 120,000 answers are non-finite and every one of them
			// is a NaN -- no draw produces an infinity.
			for(int byte = 0; byte < 10; ++byte)
				{
				nxDigestByte(&oracleDigest, wide[0][byte]);
				nxDigestByte(&candidateDigest, wide[1][byte]);
				if(wide[0][byte] != wide[1][byte])
					{ ++mismatches; ++perMode[mode]; }
				}
				nxSnanWide(mode, wide[0], wide[1]);
			}
		}
	totalMismatch += nxSnanGate("box_quad_depth", mismatches);
	printf("collision name=%s index=- rva=0x00038a90 owner=phys_fn_001739 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n", nxSnanName("box_quad_depth"),
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	nxPrintInput(nxSnanName("box_quad_depth"), &inputDigest);
	printf("collision coverage name=%s aimed_inside=%u reversed=%u interpolated=%u non_finite=%u default_mismatches=%u simulate_mismatches=%u\n", nxSnanName("box_quad_depth"),
		aimedInside, reversed, interpolated, nonFinite, perMode[0], perMode[1]);
	}

	// -----------------------------------------------------------------------
	// phys_fn_001741 at 0x00038ba0 with its continuation phys_fn_001743 at
	// 0x00039730 -- the clipping/manifold row of matrix A [BOX][BOX].
	//
	// Driven at its own address because phys_fn_001745, the separating-axis
	// search above it, is not reconstructed. That is not a limitation of this
	// block: the row's argument domain IS a reference face plus an incident
	// box, and the search only chooses which face. Driving the arguments
	// directly reaches configurations a search would take a long time to
	// produce, which is what the three manifold modes need.
	//
	// TWO FAMILIES. `.random` draws everything through nxPickWord, so poses are
	// unnormalised, extents can be negative and every coordinate can be
	// non-finite -- which is what reaches the integer extent tests' NaN
	// behaviour and the negative-extent case. `.aimed` builds real box pairs
	// and is where the manifold modes live.
	//
	// The aimed family gives the reference box an IDENTITY pose with a zero
	// centre, and that is what makes the mode attribution exact rather than
	// approximate. Stage 5 then maps every point through
	// `(1*x + z*0) + 0*y + 0`, which is the identity for a finite coordinate,
	// so the points come back in the reference frame and each contact says
	// which stage produced it: stage 4 puts BOTH of y and z exactly on the face
	// boundary, stage 3's four clip cases put exactly one there, its fifth case
	// emits a point whose x is an integer zero, and stage 2's corners are
	// strictly inside. Nothing in the attribution is computed from the
	// candidate. The incident box keeps a general pose, so the relative
	// geometry is fully general; the random family covers the reference pose.
	for(int snanPass = 0; snanPass < 2; ++snanPass)
	{
	NxSnanPass snanScope(snanPass);
	const void* oracleClip = (const void*) (base + 0x00038ba0);
	const int kSlots = 80;		// 8 corners + 5 cases on each of 12 edges + 4

	NxDigest oracleDigest[2], candidateDigest[2], inputDigest[2];
	for(int f = 0; f < 2; ++f)
		{
		nxDigestInit(&oracleDigest[f]);
		nxDigestInit(&candidateDigest[f]);
		nxDigestInit(&inputDigest[f]);
		}
	unsigned mismatches[2] = { 0, 0 };
	unsigned perMode[2][2] = { { 0, 0 }, { 0, 0 } };
	unsigned emitted[2] = { 0, 0 };
	unsigned zeroCount[2] = { 0, 0 };
	unsigned swapDiffers[2] = { 0, 0 };
	unsigned nanDepth[2] = { 0, 0 };
	unsigned overSixteen[2] = { 0, 0 };
	unsigned maxContacts[2] = { 0, 0 };
	unsigned faceFace = 0;
	unsigned edgeClip = 0;
	unsigned vertexFace = 0;
	unsigned planeCross = 0;
	int swapProbe = 0;

	for(int family = 0; family < 2; ++family)
		{
		if(snanPass && family)
			continue;
		unsigned state = family ? 0x2c1de5a7u : 0x9f31b70du;
		const unsigned iterations = family ? kAimedIterations : kPairIterations;
		for(unsigned i = 0; i < iterations; ++i)
			{
			NxReal poseA[12], poseB[12], extentA[3], extentB[3];
			int aimedMode = 0;

			if(!family)
				{
				for(int k = 0; k < 12; ++k)
					{
					nxPickWord(&state, &poseA[k]);
					nxPickWord(&state, &poseB[k]);
					}
				for(int k = 0; k < 3; ++k)
					{
					nxPickWord(&state, &extentA[k]);
					nxPickWord(&state, &extentB[k]);
					}
				}
			else
				{
				// Every operand below is finite by construction; nothing here
				// multiplies a value it also allows to be non-finite.
				aimedMode = (int) (nxNext(&state) & 3);
				for(int k = 0; k < 3; ++k)
					extentA[k] = nxUnit(&state) * 1.5f + 0.25f;

				// mode 0 -- an incident box small enough to sit wholly inside
				//           the reference face: stage 2 alone.
				// mode 1 -- comparable extents and a random rotation: edges
				//           crossing the face boundary.
				// mode 2 -- an incident face larger than the reference face,
				//           so no corner is inside and the reference face's own
				//           vertices are what land: stage 4.
				// mode 3 -- unaimed placement, which mostly produces nothing
				//           and is what makes `zero_count` non-trivial.
				const float scale = (aimedMode == 0) ? 0.35f
					: (aimedMode == 2) ? 3.0f : 1.0f;
				for(int k = 0; k < 3; ++k)
					extentB[k] = extentA[k] * scale * (nxUnit(&state) * 0.6f + 0.7f);

				unsigned char rotationStore[kShapeBytes];
				NxCollisionShape* const rotation = (NxCollisionShape*) rotationStore;
				nxIdentity(rotation);
				if(aimedMode == 1 || aimedMode == 3)
					nxRandomRotation(&state, rotation);
				else
					{
					// A small tilt, so the face pair is nearly parallel and the
					// corners really do land inside.
					const float t = (nxUnit(&state) * 2.0f - 1.0f) * 0.15f;
					rotation->rotation[4] = (float) cos((double) t);
					rotation->rotation[5] = -(float) sin((double) t);
					rotation->rotation[7] = (float) sin((double) t);
					rotation->rotation[8] = (float) cos((double) t);
					}

				// The reference face is box A's -x face: identity rotation and
				// the face centre at the origin, so "behind" is x >= 0.
				for(int k = 0; k < 9; ++k)
					poseA[k] = (k % 4) == 0 ? 1.0f : 0.0f;
				poseA[9] = poseA[10] = poseA[11] = 0.0f;

				for(int k = 0; k < 9; ++k)
					poseB[k] = rotation->rotation[k];
				const float depth = (aimedMode == 3)
					? (nxUnit(&state) * 6.0f - 3.0f)
					: (nxUnit(&state) * 0.8f - 0.1f) * extentA[0];
				const float spread = (aimedMode == 3) ? 3.0f : 1.2f;
				poseB[9] = depth;
				poseB[10] = (nxUnit(&state) * 2.0f - 1.0f) * extentA[1] * spread;
				poseB[11] = (nxUnit(&state) * 2.0f - 1.0f) * extentA[2] * spread;
				}
			nxFoldInput(&inputDigest[family], poseA, sizeof(poseA));
			nxFoldInput(&inputDigest[family], poseB, sizeof(poseB));
			nxFoldInput(&inputDigest[family], extentA, sizeof(extentA));
			nxFoldInput(&inputDigest[family], extentB, sizeof(extentB));

			for(int mode = 0; mode < 2; ++mode)
				{
				for(int orientation = 0; orientation < 2; ++orientation)
					{
					// Orientation 0 makes A's face the reference; orientation 1
					// makes B's. The second is the same geometry driven the
					// other way round, which is the only thing that says the
					// row is not accidentally symmetric.
					const NxReal* refPose = orientation ? poseB : poseA;
					const NxReal* incPose = orientation ? poseA : poseB;
					const NxReal* incExtent = orientation ? extentA : extentB;
					const NxReal refY = orientation ? extentB[1] : extentA[1];
					const NxReal refZ = orientation ? extentB[2] : extentA[2];

					NxVec3 pointsO[80], pointsC[80];
					NxReal separationsO[80], separationsC[80];
					memset(pointsO, 0xcd, sizeof(pointsO));
					memset(pointsC, 0xcd, sizeof(pointsC));
					memset(separationsO, 0xcd, sizeof(separationsO));
					memset(separationsC, 0xcd, sizeof(separationsC));

					NxU32 yBits, zBits;
					memcpy(&yBits, &refY, 4);
					memcpy(&zBits, &refZ, 4);

					nxSetControl(mode ? kControlSimulate : kControlDefault);
					const int countO = nxCallClipFaceOracle(oracleClip, pointsO,
						separationsO, refPose, yBits, zBits, incPose, incExtent);
					const int countC = NxBoxBoxClipFace(pointsC, separationsC,
						refPose, refY, refZ, incPose, incExtent);
					nxSetControl(kControlDefault);

					// Each side folds its own answer over its own count, the
					// way nxFoldStream does, so a candidate that returned fewer
					// contacts cannot shorten the oracle's digest.
					nxDigestByte(&oracleDigest[family], (unsigned char) countO);
					nxDigestByte(&candidateDigest[family], (unsigned char) countC);
					for(int c = 0; c < countO && c < kSlots; ++c)
						{
						const NxU32 words[4] = { nxBits(separationsO[c]),
							nxBits(pointsO[c].x), nxBits(pointsO[c].y), nxBits(pointsO[c].z) };
						for(int w = 0; w < 4; ++w)
							for(int byte = 0; byte < 4; ++byte)
								nxDigestByte(&oracleDigest[family],
									(unsigned char) (words[w] >> (byte * 8)));
						}
					for(int c = 0; c < countC && c < kSlots; ++c)
						{
						const NxU32 words[4] = { nxBits(separationsC[c]),
							nxBits(pointsC[c].x), nxBits(pointsC[c].y), nxBits(pointsC[c].z) };
						for(int w = 0; w < 4; ++w)
							for(int byte = 0; byte < 4; ++byte)
								nxDigestByte(&candidateDigest[family],
									(unsigned char) (words[w] >> (byte * 8)));
						}

					// The comparison covers all eighty slots rather than the
					// count, so a contact written past either side's answer is
					// a mismatch and not a silent agreement.
					unsigned differing = (countO != countC) ? 1u : 0u;
					nxSnanDiscrete(mode, (unsigned) countO, (unsigned) countC);
					for(int c = 0; c < kSlots; ++c)
						{
						if(nxBits(separationsO[c]) != nxBits(separationsC[c]))
							++differing;
						nxSnanWord(mode, nxBits(separationsO[c]), nxBits(separationsC[c]));
						if(nxBits(pointsO[c].x) != nxBits(pointsC[c].x))
							++differing;
						nxSnanWord(mode, nxBits(pointsO[c].x), nxBits(pointsC[c].x));
						if(nxBits(pointsO[c].y) != nxBits(pointsC[c].y))
							++differing;
						nxSnanWord(mode, nxBits(pointsO[c].y), nxBits(pointsC[c].y));
						if(nxBits(pointsO[c].z) != nxBits(pointsC[c].z))
							++differing;
						nxSnanWord(mode, nxBits(pointsO[c].z), nxBits(pointsC[c].z));
						}
					mismatches[family] += differing;
					perMode[family][mode] += differing;

					// Everything below is read off the oracle's own answer.
					emitted[family] += (unsigned) countO;
					if(countO == 0)
						++zeroCount[family];
					if(countO > (int) maxContacts[family])
						maxContacts[family] = (unsigned) countO;
					if(countO > 16)
						++overSixteen[family];
					for(int c = 0; c < countO && c < kSlots; ++c)
						if(!nxFinite(separationsO[c]))
							++nanDepth[family];

					if(family && orientation == 0)
						{
						for(int c = 0; c < countO && c < kSlots; ++c)
							{
							const bool onY = pointsO[c].y == refY || pointsO[c].y == -refY;
							const bool onZ = pointsO[c].z == refZ || pointsO[c].z == -refZ;
							if(onY && onZ)
								++vertexFace;
							else if(onY || onZ)
								++edgeClip;
							else if(nxBits(pointsO[c].x) == 0)
								++planeCross;
							else
								++faceFace;
							}
						}

					if(orientation == 0)
						swapProbe = countO;
					else if(countO != swapProbe)
						++swapDiffers[family];
					}
				}
			}
		}

	totalMismatch += nxSnanGate("box_clip.random", mismatches[0] + mismatches[1]);
	printf("collision name=%s index=- rva=0x00038ba0 owner=phys_fn_001741 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n", nxSnanName("box_clip.random"),
		oracleDigest[0].checks, oracleDigest[0].state, candidateDigest[0].state, mismatches[0]);
	nxPrintInput(nxSnanName("box_clip.random"), &inputDigest[0]);
	printf("collision coverage name=%s emitted=%u zero_count=%u swap_differs=%u nan_depth=%u over_sixteen=%u max_contacts=%u default_mismatches=%u simulate_mismatches=%u\n", nxSnanName("box_clip.random"),
		emitted[0], zeroCount[0], swapDiffers[0], nanDepth[0], overSixteen[0], maxContacts[0], perMode[0][0], perMode[0][1]);
	if(!snanPass)
		{
		printf("collision name=box_clip.aimed index=- rva=0x00038ba0 owner=phys_fn_001741 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
			oracleDigest[1].checks, oracleDigest[1].state, candidateDigest[1].state, mismatches[1]);
		nxPrintInput("box_clip.aimed", &inputDigest[1]);
		printf("collision coverage name=box_clip.aimed emitted=%u zero_count=%u face_face=%u edge_clip=%u vertex_face=%u plane_cross=%u swap_differs=%u nan_depth=%u over_sixteen=%u max_contacts=%u default_mismatches=%u simulate_mismatches=%u\n",
			emitted[1], zeroCount[1], faceFace, edgeClip, vertexFace, planeCross,
			swapDiffers[1], nanDepth[1], overSixteen[1], maxContacts[1], perMode[1][0], perMode[1][1]);
		}
	}


	// -----------------------------------------------------------------------
	// phys_fn_001745 at 0x00039c10 -- the fifteen-axis separating-axis search,
	// the row that chooses which face phys_fn_001741 clips against, and the
	// only row in this component that reads state it did not write.
	//
	// THE CACHE IS DRIVEN ACROSS PAIRS, WHICH IS THE POINT OF THIS BLOCK.
	// sink+0xe8 is one byte per SINK and not one per pair: the sink's
	// constructor writes 0xff there (0x0001efc8) and the per-step reset at
	// 0x0005b620 covers +0x10..+0x43 and never touches it. So each iteration
	// builds TWO different box pairs and drives both through ONE cache byte on
	// each side. A reimplementation that clears the byte per call agrees on the
	// first pair -- which starts from the constructor's sentinel anyway -- and
	// diverges from the second. A block that reset between pairs could not see
	// that at all, and the mutation that catches it is exactly "clear it".
	//
	// A THIRD CALL re-runs the first pair from a fresh cold byte, and that is
	// what `edge_only` counts: geometries the cold call separates and the warm
	// call does not. A warm byte does not merely bias the search -- it SKIPS
	// THE NINE EDGE-AXIS TESTS OUTRIGHT (`cmp al,0xff; jne 0x0003a2e8` at
	// 0x0003a01e) -- so that number is the one measurement that says both the
	// edge tests and the shortcut past them are reached, and every input to it
	// is the oracle's own answer.
	//
	// TWO FAMILIES, the split box_clip uses one row down and for the same
	// reasons. `.random` draws every pose element, extent and centre through
	// nxPickWord, which is what reaches the edge test's integer compare on a NaN
	// projection, the negative radius sum that separates nothing, and a
	// minimum search whose six candidates are all NaNs. `.aimed` builds real
	// box pairs, which is what puts a manifold behind each of the six arms.
	//
	// `arms0..arms5` are read off the byte the oracle itself wrote
	// (`inc cl; mov [eax],cl` at 0x0003a456), so the dispatch table's six slots
	// are counted rather than assumed; `negated` compares the normal the oracle
	// returned against the raw words of the row it came from, which is exact
	// because `fchs` always flips the sign bit.
	for(int snanPass = 0; snanPass < 2; ++snanPass)
	{
	NxSnanPass snanScope(snanPass);
	const void* oracleAxis = (const void*) (base + 0x00039c10);
	const int kSlots = 80;
	const unsigned kAxisIterations = 24000;
	const NxU32 kUntouched = 0xcdcdcdcdu;

	NxDigest oracleDigest[2], candidateDigest[2], inputDigest[2];
	for(int f = 0; f < 2; ++f)
		{
		nxDigestInit(&oracleDigest[f]);
		nxDigestInit(&candidateDigest[f]);
		nxDigestInit(&inputDigest[f]);
		}
	unsigned mismatches[2] = { 0, 0 };
	unsigned perMode[2][2] = { { 0, 0 }, { 0, 0 } };
	unsigned arms[2][6];
	unsigned separatedCount[2] = { 0, 0 };
	unsigned negatedCount[2] = { 0, 0 };
	unsigned warmEntry[2] = { 0, 0 };
	unsigned carryWarmed[2] = { 0, 0 };
	unsigned edgeOnly[2] = { 0, 0 };
	unsigned emitted[2] = { 0, 0 };
	unsigned overSixteen[2] = { 0, 0 };
	unsigned atSixteen[2] = { 0, 0 };
	unsigned maxContacts[2] = { 0, 0 };
	for(int f = 0; f < 2; ++f)
		for(int k = 0; k < 6; ++k)
			arms[f][k] = 0;

	for(int family = 0; family < 2; ++family)
		{
		if(snanPass && family)
			continue;
		unsigned state = family ? 0x5be13c09u : 0x71a3d84fu;
		for(unsigned it = 0; it < kAxisIterations; ++it)
			{
			NxReal pose[2][2][12];
			NxReal extent[2][2][3];

			if(!family)
				{
				for(int p = 0; p < 2; ++p)
					for(int boxIndex = 0; boxIndex < 2; ++boxIndex)
						{
						for(int k = 0; k < 12; ++k)
							nxPickWord(&state, &pose[p][boxIndex][k]);
						for(int k = 0; k < 3; ++k)
							nxPickWord(&state, &extent[p][boxIndex][k]);
						}
				}
			else
				{
				// Every operand below is finite by construction; nothing here
				// multiplies or adds a value it also allows to be non-finite.
				const int aimedMode = (int) (nxNext(&state) & 3);
				for(int p = 0; p < 2; ++p)
					{
					unsigned char rotationStore[kShapeBytes];
					NxCollisionShape* const rotation = (NxCollisionShape*) rotationStore;
					for(int boxIndex = 0; boxIndex < 2; ++boxIndex)
						{
						nxIdentity(rotation);
						if(aimedMode == 0)
							{
							// Box A keeps the identity and box B is tilted by a
							// couple of degrees, so the two frames are nearly
							// parallel and all nine edge cross products are
							// near zero -- the configuration the 1e-6f fudge at
							// 0x10106880 is added for.
							if(boxIndex == 1)
								{
								const float t = (nxUnit(&state) * 2.0f - 1.0f) * 0.05f;
								rotation->rotation[0] = (float) cos((double) t);
								rotation->rotation[1] = -(float) sin((double) t);
								rotation->rotation[3] = (float) sin((double) t);
								rotation->rotation[4] = (float) cos((double) t);
								}
							}
						else
							nxRandomRotation(&state, rotation);

						for(int k = 0; k < 9; ++k)
							pose[p][boxIndex][k] = rotation->rotation[k];
						for(int k = 0; k < 3; ++k)
							extent[p][boxIndex][k] = nxUnit(&state) * 1.4f + 0.3f;
						}

					// Mode 2 makes box B much the larger, so its faces win the
					// minimum search and arms 3..5 are reached; mode 1 makes it
					// much the smaller, so box A's do.
					const float scale = (aimedMode == 2) ? 2.5f
						: (aimedMode == 1) ? 0.4f : 1.0f;
					for(int k = 0; k < 3; ++k)
						extent[p][1][k] = extent[p][1][k] * scale;

					// Box A anywhere, box B offset per axis by a fraction of
					// the two extents' sum, so the placement straddles the
					// contact boundary. Mode 3 spreads it wider, which is where
					// the edge-axis tests get to decide; mode 0 keeps it tight,
					// which is the DEEP overlap of two nearly parallel boxes --
					// the configuration that produces the largest manifolds, and
					// so the one that decides whether a real caller can reach
					// phys_fn_001749's seventeenth contact.
					const float spread = (aimedMode == 3) ? 1.35f
						: (aimedMode == 0) ? 0.30f : 0.85f;
					for(int k = 0; k < 3; ++k)
						{
						const float reach = (extent[p][0][k] + extent[p][1][k]) * spread;
						pose[p][0][9 + k] = nxUnit(&state) * 4.0f - 2.0f;
						pose[p][1][9 + k] = pose[p][0][9 + k]
							+ (nxUnit(&state) * 2.0f - 1.0f) * reach;
						}
					}
				}

			// The byte the sink is carrying when the first pair arrives. 0xff is
			// what the constructor leaves, 0 is what phys_fn_001749 writes after
			// a call that produced nothing, and 1..6 is what an earlier pair in
			// the same sink left behind.
			const unsigned choice = nxNext(&state) & 7;
			const unsigned char seed = (choice == 0)
				? (unsigned char) 0xff : (unsigned char) (choice - 1);
			nxFoldInput(&inputDigest[family], pose, sizeof(pose));
			nxFoldInput(&inputDigest[family], extent, sizeof(extent));
			nxFoldInput(&inputDigest[family], &seed, 1);

			for(int mode = 0; mode < 2; ++mode)
				{
				unsigned char cacheO = seed;
				unsigned char cacheC = seed;
				bool warmProbe = false;
				bool warmReached = false;

				for(int call = 0; call < 3; ++call)
					{
					const int pair = (call == 1) ? 1 : 0;
					unsigned char coldO = 0xff;
					unsigned char coldC = 0xff;
					unsigned char* const cO = (call == 2) ? &coldO : &cacheO;
					unsigned char* const cC = (call == 2) ? &coldC : &cacheC;
					const unsigned char entering = *cO;

					NxVec3 pointsO[80], pointsC[80];
					NxReal separationsO[80], separationsC[80];
					NxVec3 normalO, normalC;
					memset(pointsO, 0xcd, sizeof(pointsO));
					memset(pointsC, 0xcd, sizeof(pointsC));
					memset(separationsO, 0xcd, sizeof(separationsO));
					memset(separationsC, 0xcd, sizeof(separationsC));
					memset(&normalO, 0xcd, sizeof(normalO));
					memset(&normalC, 0xcd, sizeof(normalC));

					nxSetControl(mode ? kControlSimulate : kControlDefault);
					const int countO = nxCallSeparatingAxisOracle(oracleAxis,
						pointsO, separationsO, extent[pair][0], pose[pair][1],
						cO, pose[pair][0], &normalO, extent[pair][1]);
					const int countC = NxBoxBoxSeparatingAxis(pointsC,
						separationsC, extent[pair][0], pose[pair][1], cC,
						pose[pair][0], &normalC, extent[pair][1]);
					nxSetControl(kControlDefault);

					// Each side folds its own count, its own cache byte, its own
					// normal and then its own contacts over its own count, so a
					// candidate that returned fewer contacts cannot shorten the
					// oracle's digest.
					nxDigestByte(&oracleDigest[family], (unsigned char) countO);
					nxDigestByte(&candidateDigest[family], (unsigned char) countC);
					nxDigestByte(&oracleDigest[family], *cO);
					nxDigestByte(&candidateDigest[family], *cC);
						{
						const NxU32 oWords[3] = { nxBits(normalO.x), nxBits(normalO.y), nxBits(normalO.z) };
						const NxU32 cWords[3] = { nxBits(normalC.x), nxBits(normalC.y), nxBits(normalC.z) };
						for(int w = 0; w < 3; ++w)
							for(int byte = 0; byte < 4; ++byte)
								{
								nxDigestByte(&oracleDigest[family], (unsigned char) (oWords[w] >> (byte * 8)));
								nxDigestByte(&candidateDigest[family], (unsigned char) (cWords[w] >> (byte * 8)));
								}
						}
					for(int c = 0; c < countO && c < kSlots; ++c)
						{
						const NxU32 words[4] = { nxBits(separationsO[c]),
							nxBits(pointsO[c].x), nxBits(pointsO[c].y), nxBits(pointsO[c].z) };
						for(int w = 0; w < 4; ++w)
							for(int byte = 0; byte < 4; ++byte)
								nxDigestByte(&oracleDigest[family],
									(unsigned char) (words[w] >> (byte * 8)));
						}
					for(int c = 0; c < countC && c < kSlots; ++c)
						{
						const NxU32 words[4] = { nxBits(separationsC[c]),
							nxBits(pointsC[c].x), nxBits(pointsC[c].y), nxBits(pointsC[c].z) };
						for(int w = 0; w < 4; ++w)
							for(int byte = 0; byte < 4; ++byte)
								nxDigestByte(&candidateDigest[family],
									(unsigned char) (words[w] >> (byte * 8)));
						}

					// The comparison covers the cache byte, the normal and all
					// eighty slots rather than the count, so a contact written
					// past either side's answer is a mismatch.
					unsigned differing = (countO != countC) ? 1u : 0u;
					nxSnanDiscrete(mode, (unsigned) countO, (unsigned) countC);
					if(*cO != *cC)
						++differing;
					nxSnanDiscrete(mode, *cO, *cC);
					if(nxBits(normalO.x) != nxBits(normalC.x))
						++differing;
					nxSnanWord(mode, nxBits(normalO.x), nxBits(normalC.x));
					if(nxBits(normalO.y) != nxBits(normalC.y))
						++differing;
					nxSnanWord(mode, nxBits(normalO.y), nxBits(normalC.y));
					if(nxBits(normalO.z) != nxBits(normalC.z))
						++differing;
					nxSnanWord(mode, nxBits(normalO.z), nxBits(normalC.z));
					for(int c = 0; c < kSlots; ++c)
						{
						if(nxBits(separationsO[c]) != nxBits(separationsC[c]))
							++differing;
						nxSnanWord(mode, nxBits(separationsO[c]), nxBits(separationsC[c]));
						if(nxBits(pointsO[c].x) != nxBits(pointsC[c].x))
							++differing;
						nxSnanWord(mode, nxBits(pointsO[c].x), nxBits(pointsC[c].x));
						if(nxBits(pointsO[c].y) != nxBits(pointsC[c].y))
							++differing;
						nxSnanWord(mode, nxBits(pointsO[c].y), nxBits(pointsC[c].y));
						if(nxBits(pointsO[c].z) != nxBits(pointsC[c].z))
							++differing;
						nxSnanWord(mode, nxBits(pointsO[c].z), nxBits(pointsC[c].z));
						}
					mismatches[family] += differing;
					perMode[family][mode] += differing;

					// Everything below is read off the oracle's own answer. The
					// normal is written unconditionally by every arm and by no
					// other path, so it is what says the dispatch was reached;
					// a separated pair returns from 0x0003acb3 and leaves the
					// sentinel in place.
					const bool reached = nxBits(normalO.x) != kUntouched
						|| nxBits(normalO.y) != kUntouched
						|| nxBits(normalO.z) != kUntouched;
					if(entering >= 1 && entering <= 6)
						++warmEntry[family];
					if(call == 1 && (seed == 0 || seed == 0xff)
						&& entering >= 1 && entering <= 6)
						++carryWarmed[family];
					if(!reached)
						++separatedCount[family];
					else
						{
						const int code = (int) *cO - 1;
						++arms[family][code];
						const NxReal* const ref = (code < 3) ? pose[pair][0] : pose[pair][1];
						if(nxBits(normalO.x) != nxBits(ref[3 * (code % 3)]))
							++negatedCount[family];
						}
					emitted[family] += (unsigned) countO;
					if(countO > (int) maxContacts[family])
						maxContacts[family] = (unsigned) countO;
					// The tail of the contact-count distribution, and the reason
					// it is registered rather than mentioned: phys_fn_001749's
					// points array holds SIXTEEN and its seventeenth entry is
					// the return address. `at_sixteen` is how often the shipped
					// chain fills it exactly and `over_sixteen` how often it
					// runs past. When phys_fn_001741 was driven with a synthetic
					// reference face those read 18 and 20; here the face comes
					// from the search, which is the measurement Phase 8 wanted.
					if(countO == 16)
						++atSixteen[family];
					if(countO > 16)
						++overSixteen[family];

					if(call == 0)
						{
						warmProbe = entering >= 1 && entering <= 6;
						warmReached = reached;
						}
					else if(call == 2 && warmProbe && warmReached && !reached)
						++edgeOnly[family];
					}
				}
			}
		}

	totalMismatch += nxSnanGate("box_axis.random", mismatches[0] + mismatches[1]);
	printf("collision name=%s index=- rva=0x00039c10 owner=phys_fn_001745 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n", nxSnanName("box_axis.random"),
		oracleDigest[0].checks, oracleDigest[0].state, candidateDigest[0].state, mismatches[0]);
	nxPrintInput(nxSnanName("box_axis.random"), &inputDigest[0]);
	printf("collision coverage name=%s arm0=%u arm1=%u arm2=%u arm3=%u arm4=%u arm5=%u separated=%u negated=%u warm_entry=%u carry_warmed=%u edge_only=%u emitted=%u at_sixteen=%u over_sixteen=%u max_contacts=%u default_mismatches=%u simulate_mismatches=%u\n", nxSnanName("box_axis.random"),
		arms[0][0], arms[0][1], arms[0][2], arms[0][3], arms[0][4], arms[0][5],
		separatedCount[0], negatedCount[0], warmEntry[0], carryWarmed[0], edgeOnly[0],
		emitted[0], atSixteen[0], overSixteen[0], maxContacts[0], perMode[0][0], perMode[0][1]);
	if(!snanPass)
		{
		printf("collision name=box_axis.aimed index=- rva=0x00039c10 owner=phys_fn_001745 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
			oracleDigest[1].checks, oracleDigest[1].state, candidateDigest[1].state, mismatches[1]);
		nxPrintInput("box_axis.aimed", &inputDigest[1]);
		printf("collision coverage name=box_axis.aimed arm0=%u arm1=%u arm2=%u arm3=%u arm4=%u arm5=%u separated=%u negated=%u warm_entry=%u carry_warmed=%u edge_only=%u emitted=%u at_sixteen=%u over_sixteen=%u max_contacts=%u default_mismatches=%u simulate_mismatches=%u\n",
			arms[1][0], arms[1][1], arms[1][2], arms[1][3], arms[1][4], arms[1][5],
			separatedCount[1], negatedCount[1], warmEntry[1], carryWarmed[1], edgeOnly[1],
			emitted[1], atSixteen[1], overSixteen[1], maxContacts[1], perMode[1][0], perMode[1][1]);
		}
	}


	// -----------------------------------------------------------------------
	// phys_fn_001748 at 0x0003ace0 -- the transpose-and-copy shim.
	//
	// 240 bytes, twenty-four integer moves and one call. It is driven as a row
	// of its own because the transposition is the whole of what it does, and
	// nothing above it can tell a transpose from its inverse unless the pose is
	// asymmetric -- which is exactly what a raw-bit pose is and what a rotation
	// matrix built from a random quaternion is. Every argument is on the stack
	// (`add esp,0x20` at 0x0003ae4f), so this one needs no thunk.
	//
	// Two pairs per iteration through one cache byte, for the same reason the
	// search's own block does it: the shim forwards that pointer and does not
	// touch it, so the carry has to survive one more frame.
	for(int snanPass = 0; snanPass < 2; ++snanPass)
	{
	NxSnanPass snanScope(snanPass);
	typedef int(__cdecl* NxOracleShimFn)(NxVec3*, NxReal*, NxVec3*, const NxReal*,
		const NxReal*, const NxReal*, const NxReal*, unsigned char*);
	NxOracleShimFn oracleShim = (NxOracleShimFn) (base + 0x0003ace0);
	const int kSlots = 80;
	const unsigned kShimIterations = 16000;
	const NxU32 kUntouched = 0xcdcdcdcdu;

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	unsigned mismatches = 0;
	unsigned perMode[2] = { 0, 0 };
	unsigned aimedCalls = 0;
	unsigned separatedCount = 0;
	unsigned emittedContacts = 0;
	unsigned carryWarmed = 0;
	unsigned maxContacts = 0;
	unsigned aimedMax = 0;
	unsigned overSixteen = 0;
	unsigned state = 0x3d70f21bu;

	for(unsigned it = 0; it < kShimIterations; ++it)
		{
		NxReal pose[2][2][12];
		NxReal extent[2][2][3];
		const bool aimed = (nxNext(&state) & 1) != 0;

		for(int p = 0; p < 2; ++p)
			for(int boxIndex = 0; boxIndex < 2; ++boxIndex)
				{
				if(!aimed)
					{
					for(int k = 0; k < 12; ++k)
						nxPickWord(&state, &pose[p][boxIndex][k]);
					for(int k = 0; k < 3; ++k)
						nxPickWord(&state, &extent[p][boxIndex][k]);
					}
				else
					{
					// Finite by construction, and a real rotation, so the
					// transposition is its own inverse only if the matrix is
					// symmetric -- which a random quaternion's is not.
					unsigned char rotationStore[kShapeBytes];
					NxCollisionShape* const rotation = (NxCollisionShape*) rotationStore;
					nxIdentity(rotation);
					nxRandomRotation(&state, rotation);
					for(int k = 0; k < 9; ++k)
						pose[p][boxIndex][k] = rotation->rotation[k];
					for(int k = 0; k < 3; ++k)
						{
						extent[p][boxIndex][k] = nxUnit(&state) * 1.4f + 0.3f;
						pose[p][boxIndex][9 + k] = boxIndex == 0
							? nxUnit(&state) * 2.0f - 1.0f
							: pose[p][0][9 + k] + (nxUnit(&state) * 2.0f - 1.0f)
								* (extent[p][0][k] + extent[p][1][k]) * 0.9f;
						}
					}
				}
		if(aimed)
			aimedCalls += 2;

		const unsigned choice = nxNext(&state) & 7;
		const unsigned char seed = (choice == 0)
			? (unsigned char) 0xff : (unsigned char) (choice - 1);
		nxFoldInput(&inputDigest, pose, sizeof(pose));
		nxFoldInput(&inputDigest, extent, sizeof(extent));
		nxFoldInput(&inputDigest, &seed, 1);

		for(int mode = 0; mode < 2; ++mode)
			{
			unsigned char cacheO = seed;
			unsigned char cacheC = seed;
			for(int pair = 0; pair < 2; ++pair)
				{
				const unsigned char entering = cacheO;
				NxVec3 pointsO[80], pointsC[80];
				NxReal separationsO[80], separationsC[80];
				NxVec3 normalO, normalC;
				memset(pointsO, 0xcd, sizeof(pointsO));
				memset(pointsC, 0xcd, sizeof(pointsC));
				memset(separationsO, 0xcd, sizeof(separationsO));
				memset(separationsC, 0xcd, sizeof(separationsC));
				memset(&normalO, 0xcd, sizeof(normalO));
				memset(&normalC, 0xcd, sizeof(normalC));

				nxSetControl(mode ? kControlSimulate : kControlDefault);
				const int countO = oracleShim(pointsO, separationsO, &normalO,
					extent[pair][0], pose[pair][0], extent[pair][1],
					pose[pair][1], &cacheO);
				const int countC = NxBoxBoxTransposedPair(pointsC, separationsC,
					&normalC, extent[pair][0], pose[pair][0], extent[pair][1],
					pose[pair][1], &cacheC);
				nxSetControl(kControlDefault);

				nxDigestByte(&oracleDigest, (unsigned char) countO);
				nxDigestByte(&candidateDigest, (unsigned char) countC);
				nxDigestByte(&oracleDigest, cacheO);
				nxDigestByte(&candidateDigest, cacheC);
					{
					const NxU32 o[3] = { nxBits(normalO.x), nxBits(normalO.y), nxBits(normalO.z) };
					const NxU32 c[3] = { nxBits(normalC.x), nxBits(normalC.y), nxBits(normalC.z) };
					for(int w = 0; w < 3; ++w)
						for(int byte = 0; byte < 4; ++byte)
							{
							nxDigestByte(&oracleDigest, (unsigned char) (o[w] >> (byte * 8)));
							nxDigestByte(&candidateDigest, (unsigned char) (c[w] >> (byte * 8)));
							}
					}
				for(int c = 0; c < countO && c < kSlots; ++c)
					{
					const NxU32 words[4] = { nxBits(separationsO[c]),
						nxBits(pointsO[c].x), nxBits(pointsO[c].y), nxBits(pointsO[c].z) };
					for(int w = 0; w < 4; ++w)
						for(int byte = 0; byte < 4; ++byte)
							nxDigestByte(&oracleDigest, (unsigned char) (words[w] >> (byte * 8)));
					}
				for(int c = 0; c < countC && c < kSlots; ++c)
					{
					const NxU32 words[4] = { nxBits(separationsC[c]),
						nxBits(pointsC[c].x), nxBits(pointsC[c].y), nxBits(pointsC[c].z) };
					for(int w = 0; w < 4; ++w)
						for(int byte = 0; byte < 4; ++byte)
							nxDigestByte(&candidateDigest, (unsigned char) (words[w] >> (byte * 8)));
					}

				unsigned differing = (countO != countC) ? 1u : 0u;
				nxSnanDiscrete(mode, (unsigned) countO, (unsigned) countC);
				if(cacheO != cacheC)
					++differing;
				nxSnanDiscrete(mode, cacheO, cacheC);
				if(nxBits(normalO.x) != nxBits(normalC.x))
					++differing;
				nxSnanWord(mode, nxBits(normalO.x), nxBits(normalC.x));
				if(nxBits(normalO.y) != nxBits(normalC.y))
					++differing;
				nxSnanWord(mode, nxBits(normalO.y), nxBits(normalC.y));
				if(nxBits(normalO.z) != nxBits(normalC.z))
					++differing;
				nxSnanWord(mode, nxBits(normalO.z), nxBits(normalC.z));
				for(int c = 0; c < kSlots; ++c)
					{
					if(nxBits(separationsO[c]) != nxBits(separationsC[c]))
						++differing;
					nxSnanWord(mode, nxBits(separationsO[c]), nxBits(separationsC[c]));
					if(nxBits(pointsO[c].x) != nxBits(pointsC[c].x))
						++differing;
					nxSnanWord(mode, nxBits(pointsO[c].x), nxBits(pointsC[c].x));
					if(nxBits(pointsO[c].y) != nxBits(pointsC[c].y))
						++differing;
					nxSnanWord(mode, nxBits(pointsO[c].y), nxBits(pointsC[c].y));
					if(nxBits(pointsO[c].z) != nxBits(pointsC[c].z))
						++differing;
					nxSnanWord(mode, nxBits(pointsO[c].z), nxBits(pointsC[c].z));
					}
				mismatches += differing;
				perMode[mode] += differing;

				const bool reached = nxBits(normalO.x) != kUntouched
					|| nxBits(normalO.y) != kUntouched
					|| nxBits(normalO.z) != kUntouched;
				if(!reached)
					++separatedCount;
				if(pair == 1 && (seed == 0 || seed == 0xff)
					&& entering >= 1 && entering <= 6)
					++carryWarmed;
				emittedContacts += (unsigned) countO;
				if(countO > (int) maxContacts)
					maxContacts = (unsigned) countO;
				// SPLIT BY FAMILY, because the two answer different questions
				// about phys_fn_001749's sixteen-contact frame. `aimed_max` is
				// the count with real rotations and physical extents -- the
				// configuration a scene can actually be in -- and
				// `over_sixteen` counts every call of either kind that returned
				// more than the entry's arrays hold.
				if(aimed && countO > (int) aimedMax)
					aimedMax = (unsigned) countO;
				if(countO > 16)
					++overSixteen;
				}
			}
		}

	totalMismatch += nxSnanGate("box_shim", mismatches);
	printf("collision name=%s index=- rva=0x0003ace0 owner=phys_fn_001748 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n", nxSnanName("box_shim"),
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	nxPrintInput(nxSnanName("box_shim"), &inputDigest);
	printf("collision coverage name=%s aimed=%u separated=%u emitted=%u carry_warmed=%u over_sixteen=%u aimed_max=%u max_contacts=%u default_mismatches=%u simulate_mismatches=%u\n", nxSnanName("box_shim"),
		aimedCalls, separatedCount, emittedContacts, carryWarmed, overSixteen,
		aimedMax, maxContacts, perMode[0], perMode[1]);
	}

	// -----------------------------------------------------------------------
	// phys_fn_001749 at 0x0003add0, matrix A [BOX][BOX] -- the last entry of
	// matrix A this phase can close, and the only one that emits a manifold.
	//
	// The stream half is the plane/box pattern, so what this block has to
	// distinguish is what is NOT shared: the header written with no predicate,
	// the normal block written with no predicate over a cache the entry has
	// just cleared, the orientation swap reading the FIRST shape's owner where
	// plane/box reads the second's, a contact loop with NO CAP, and a
	// separation that is negated and then masked.
	//
	// AND THE WARM START, END TO END. Each iteration drives up to four box
	// pairs into ONE sink without resetting it between them, which is the
	// state the shipped pipeline is in: sink+0xe8 is per sink, the per-step
	// reset never clears it, and the only thing that ever writes 0 there is
	// this entry, on a pair that produced nothing (0x0003ae5d). `warm_carried`
	// counts pairs that arrived with an index an earlier PAIR left behind and
	// `axis_cleared` counts the pairs that wiped it; a reimplementation that
	// treats the byte as per-call scratch agrees on the first pair of every
	// sequence and diverges after it.
	//
	// The sink's own constructor writes 0xff (0x0001efc8) and nxResetWorld
	// zeroes it, so the sequence is started at one sentinel or the other in
	// turn -- both mean "no cache" to the search and driving only one would
	// leave the other's branch untested.
	for(int snanPass = 0; snanPass < 2; ++snanPass)
	{
	NxSnanPass snanScope(snanPass);
	typedef void(__cdecl* NxOracleContactFn)(const NxCollisionShape*, const NxCollisionShape*,
		NxContactSink*, void*);
	NxOracleContactFn oracleContact = (NxOracleContactFn) (base + nxMatrixA[14].rva);
	typedef int(__cdecl* NxOracleShimFn)(NxVec3*, NxReal*, NxVec3*, const NxReal*,
		const NxReal*, const NxReal*, const NxReal*, unsigned char*);
	NxOracleShimFn oracleShim = (NxOracleShimFn) (base + 0x0003ace0);

	static NxContactWorld world[2];

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	unsigned mismatches = 0;
	unsigned perMode[2] = { 0, 0 };
	unsigned emitted = 0;
	unsigned negatedPath = 0;
	unsigned staticSide[2] = { 0, 0 };
	unsigned warmCarried = 0;
	unsigned axisCleared = 0;
	unsigned atSixteen = 0;
	unsigned maxContacts = 0;
	unsigned overflowSkipped = 0;
	unsigned probeMax = 0;
	unsigned widths[80];
	memset(widths, 0, sizeof(widths));
	unsigned state = 0x1c8b47e5u;

	for(unsigned i = 0; i < kContactIterations; ++i)
		{
		const unsigned sequenceSeed = nxNext(&state);
		const unsigned pairs = 1 + (nxNext(&state) % 4);
		const unsigned char sentinel = (i & 1) ? (unsigned char) 0xff : (unsigned char) 0;

		for(int mode = 0; mode < 2; ++mode)
			{
			for(int side = 0; side < 2; ++side)
				{
				nxResetWorld(&world[side]);
				world[side].sink.separatingAxis = sentinel;
				}

			unsigned local = sequenceSeed;
			static unsigned char box0Storage[kShapeBytes];
			static unsigned char box1Storage[kShapeBytes];
			NxCollisionShape* box0Shape = (NxCollisionShape*) box0Storage;
			NxCollisionShape* box1Shape = (NxCollisionShape*) box1Storage;
			for(unsigned p = 0; p < pairs; ++p)
				{
				const bool tame = (nxNext(&local) & 3) != 0;
				const bool deep = (nxNext(&local) & 1) != 0;
				nxIdentity(box0Shape);
				nxIdentity(box1Shape);
				nxFillGeometry(&local, box0Shape, 2, tame);
				nxFillGeometry(&local, box1Shape, 2, tame);
				nxRandomRotation(&local, box0Shape);
				nxRandomRotation(&local, box1Shape);
				for(int k = 0; k < 3; ++k)
					if(tame)
						box0Shape->translation[k] = nxUnit(&local) * 2.0f - 1.0f;
					else
						nxPickWord(&local, &box0Shape->translation[k]);
				if(tame)
					{
					// PER AXIS, not from a mean reach. The mean spreads the
					// same offset over three unequal extents and mostly
					// produces shallow contacts; scaling each axis by its own
					// pair of extents is what puts two boxes deeply into each
					// other, and a deep overlap is the only configuration whose
					// manifold can exceed phys_fn_001749's sixteen slots.
					const float spread = deep ? 0.6f : 1.2f;
					for(int k = 0; k < 3; ++k)
						box1Shape->translation[k] = box0Shape->translation[k]
							+ (nxUnit(&local) * 2.0f - 1.0f) * spread
								* (box0Shape->geometry[k + 1] + box1Shape->geometry[k + 1]);
					}
				else
					for(int k = 0; k < 3; ++k)
						nxPickWord(&local, &box1Shape->translation[k]);

				const bool newIdentity0 = (p == 0) || ((nxNext(&local) & 3) == 0);
				const bool newIdentity1 = (p == 0) || ((nxNext(&local) & 3) == 0);
				const NxU32 material0 = nxNext(&local) & 0xff;
				const NxU32 material1 = nxNext(&local) & 0xff;
				const unsigned staticDraw = nxNext(&local) % 12;
				const bool nullHolder0 = staticDraw == 0;
				const bool nullHolder1 = staticDraw == 1;
				const bool orientToBox1 = (nxNext(&local) & 1) != 0;
				if(mode == 0)
					{
					if(nullHolder0)
						++staticSide[0];
					if(nullHolder1)
						++staticSide[1];
					if(orientToBox1)
						++negatedPath;
					}

				const unsigned before = world[0].sink.streamCount;
				const unsigned char entering = world[0].sink.separatingAxis;
				for(int side = 0; side < 2; ++side)
					nxStageWorld(&world[side], box0Shape, box1Shape,
						newIdentity0, newIdentity1, material0, material1,
						nullHolder0, nullHolder1, !orientToBox1);

				// PRE-FLIGHT, AND IT IS NOT BELT AND BRACES.
				// phys_fn_001749's points array holds SIXTEEN and its
				// seventeenth entry IS its own return address, so a pair whose
				// manifold is larger does not merely disagree -- the oracle
				// returns into a contact coordinate and takes the harness with
				// it. phys_fn_001748 is called first with the identical
				// arguments and a COPY of the live cache byte, so the count it
				// returns is exactly the count the entry would produce, into
				// eighty slots of the harness's own where the overrun is
				// harmless. A pair that would overflow is COUNTED AND NOT
				// DRIVEN, and `overflow_skipped` is therefore the reachability
				// measurement itself: it is taken at the entry's own inputs.
				unsigned char probeAxis = world[0].sink.separatingAxis;
				NxVec3 probePoints[80];
				NxReal probeSeparations[80];
				NxVec3 probeNormal;
				nxSetControl(mode ? kControlSimulate : kControlDefault);
				const int probeCount = oracleShim(probePoints, probeSeparations,
					&probeNormal, &world[0].plane->geometry[1],
					&world[0].plane->rotation[0], &world[0].sphere->geometry[1],
					&world[0].sphere->rotation[0], &probeAxis);
				nxSetControl(kControlDefault);
				if(mode == 0 && probeCount > (int) probeMax)
					probeMax = (unsigned) probeCount;
				if(probeCount > 16)
					{
					if(mode == 0)
						++overflowSkipped;
					continue;
					}

				nxSetControl(mode ? kControlSimulate : kControlDefault);
				if(mode == 0)
					{
					nxFoldInputShape(&inputDigest, world[0].plane);
					nxFoldInputShape(&inputDigest, world[0].sphere);
					}
				oracleContact(world[0].plane, world[0].sphere, &world[0].sink, nxOverlapContext);
				NxContactBoxBox(world[1].plane, world[1].sphere, &world[1].sink, nxOverlapContext);
				nxSetControl(kControlDefault);

				// Read entirely off the oracle. `appended` is 7 + 4n for n
				// contacts, because both the header and the normal block are
				// unconditional here.
				const unsigned appended = world[0].sink.streamCount - before;
				if(appended)
					++emitted;
				if(appended < 80)
					++widths[appended];
				if(appended >= 11)
					{
					const unsigned contacts = (appended - 7) / 4;
					if(contacts > maxContacts)
						maxContacts = contacts;
					if(contacts == 16)
						++atSixteen;
					}
				if(mode == 0 && p > 0 && entering >= 1 && entering <= 6)
					++warmCarried;
				if(mode == 0 && world[0].sink.separatingAxis == 0 && entering != 0)
					++axisCleared;

				if(world[0].sink.separatingAxis != world[1].sink.separatingAxis)
					{
					++mismatches;
					++perMode[mode];
					}
				nxDigestByte(&oracleDigest, world[0].sink.separatingAxis);
				nxDigestByte(&candidateDigest, world[1].sink.separatingAxis);
				}

			nxFoldStream(&oracleDigest, &world[0]);
			nxFoldStream(&candidateDigest, &world[1]);
			const unsigned differing = nxCompareStreams(&world[0], &world[1], mode);
			mismatches += differing;
			perMode[mode] += differing;
			}
		}
	totalMismatch += nxSnanGate("contact_box_box", mismatches);
	printf("collision name=%s index=14 rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n", nxSnanName("contact_box_box"),
		nxMatrixA[14].rva, nxMatrixA[14].stableId,
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	nxPrintInput(nxSnanName("contact_box_box"), &inputDigest);
	printf("collision coverage name=%s emitted=%u negated=%u static0=%u static1=%u warm_carried=%u axis_cleared=%u w11=%u w15=%u w31=%u at_sixteen=%u max_contacts=%u overflow_skipped=%u probe_max=%u default_mismatches=%u simulate_mismatches=%u\n", nxSnanName("contact_box_box"),
		emitted, negatedPath, staticSide[0], staticSide[1], warmCarried, axisCleared,
		widths[11], widths[15], widths[31], atSixteen, maxContacts,
		overflowSkipped, probeMax, perMode[0], perMode[1]);
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
	// only one or two times in fifteen. NxRayTriIntersect's draws keep their
	// signalling NaNs (nxPickRawWord): since the harness hardening the row is the
	// listing's instructions, and a signalling NaN is what tells which operands it
	// loads (quieting them) and which it uses from memory.
	{
	typedef bool(NX_CALL_CONV* NxOracleRayTriFn)(const NxVec3&, const NxVec3&, const NxVec3&,
		const NxVec3&, const NxVec3&, float&, float&, float&, bool);
	typedef bool(NX_CALL_CONV* NxOracleNormalsFn)(NxU32, NxU32, const NxVec3*, const NxU32*,
		const NxU16*, NxVec3*, bool);
	NxOracleRayTriFn oracleRayTri = (NxOracleRayTriFn) GetProcAddress(physics, "NxRayTriIntersect");
	NxOracleNormalsFn oracleNormals = (NxOracleNormalsFn) GetProcAddress(physics, "NxBuildSmoothNormals");
	if(!oracleRayTri || !oracleNormals)
		return nxFail("the pinned oracle does not export the two step-reachable kernels");

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
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
				if(tame)
					(&v[k].x)[c] = nxUnit(&state) * 4.0f - 2.0f;
				else
					nxPickRawWord(&state, &(&v[k].x)[c]);
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
		nxFoldInput(&inputDigest, v, sizeof(v));
		nxFoldInput(&inputDigest, &cull, sizeof(cull));

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
	nxPrintInput("step_ray_tri", &inputDigest);
	printf("collision coverage name=step_ray_tri hits=%u non_finite_words=%u default_mismatches=%u simulate_mismatches=%u\n",
		hits, nonFinite, perMode[0], perMode[1]);

	for(int snanPass = 0; snanPass < 2; ++snanPass)
	{
	NxSnanPass snanScope(snanPass);
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
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
				if(tame)
					(&verts[vertex].x)[c] = nxUnit(&state) * 4.0f - 2.0f;
				else
					nxPickWord(&state, &(&verts[vertex].x)[c]);
		// Indices stay in range: the export bounds-checks none of them and an
		// out-of-range one would corrupt this harness's own heap.
		for(NxU32 index = 0; index < nbTris * 3; ++index)
			dFaces[index] = nxNext(&state) % nbVerts;
		bool flip = (nxNext(&state) & 1) != 0;
		nxFoldInput(&inputDigest, &nbTris, sizeof(nbTris));
		nxFoldInput(&inputDigest, &nbVerts, sizeof(nbVerts));
		nxFoldInput(&inputDigest, verts, nbVerts * sizeof(NxVec3));
		nxFoldInput(&inputDigest, dFaces, nbTris * 3 * sizeof(NxU32));
		nxFoldInput(&inputDigest, &flip, sizeof(flip));

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
			nxSnanDiscrete(mode, a, b);
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
					nxSnanWord(mode, x, y);
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
	totalMismatch += nxSnanGate("step_smooth_normals", perMode[0]);
	printf("collision name=%s index=- rva=export owner=phys_fn_002146 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		nxSnanName("step_smooth_normals"), oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	nxPrintInput(nxSnanName("step_smooth_normals"), &inputDigest);
	printf("collision coverage name=%s non_finite_words=%u default_mismatches=%u simulate_mismatches=%u\n",
		nxSnanName("step_smooth_normals"), nonFinite, perMode[0], perMode[1]);
	}
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
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
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
					if(tame)
						sphereShape->translation[k] = nxUnit(&local) * 3.0f - 1.5f;
					else
						nxPickWord(&local, &sphereShape->translation[k]);
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
				if(mode == 0)
					{
					nxFoldInputShape(&inputDigest, world[0].plane);
					nxFoldInputShape(&inputDigest, world[0].sphere);
					}
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
			mismatches += nxCompareStreams(&world[0], &world[1], mode);
			}
		}
	totalMismatch += mismatches;
	printf("collision name=contact_plane_sphere index=1 rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		nxMatrixA[1].rva, nxMatrixA[1].stableId,
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	nxPrintInput("contact_plane_sphere", &inputDigest);
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
	for(int snanPass = 0; snanPass < 2; ++snanPass)
	{
	NxSnanPass snanScope(snanPass);
	typedef void(__thiscall* NxOracleEmitFn)(NxContactSink*, void*, void*, NxU32,
		const NxVec3*, const NxVec3*, NxU16, NxU16);
	NxOracleEmitFn oracleEmit = (NxOracleEmitFn) (base + 0x0001d610);

	static NxContactWorld emitWorld[2];
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
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
					if(tame)
						(&point.x)[k] = nxUnit(&local) * 4.0f - 2.0f;
					else
						nxPickWord(&local, &(&point.x)[k]);
					if(tame)
						(&normal.x)[k] = nxUnit(&local) * 2.0f - 1.0f;
					else
						nxPickWord(&local, &(&normal.x)[k]);
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
				if(mode == 0)
					{
					nxFoldInputShape(&inputDigest, emitWorld[0].plane);
					nxFoldInputShape(&inputDigest, emitWorld[0].sphere);
					nxFoldInput(&inputDigest, &separationBits, sizeof(separationBits));
					nxFoldInput(&inputDigest, &point, sizeof(point));
					nxFoldInput(&inputDigest, &normal, sizeof(normal));
					nxFoldInput(&inputDigest, &id0, sizeof(id0));
					nxFoldInput(&inputDigest, &id1, sizeof(id1));
					}
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
			mismatches += nxCompareStreams(&emitWorld[0], &emitWorld[1], mode);
			}
		}
	totalMismatch += nxSnanGate("contact_emit", mismatches);
	printf("collision name=%s index=- rva=0x0001d610 owner=phys_fn_000873 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n", nxSnanName("contact_emit"),
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	nxPrintInput(nxSnanName("contact_emit"), &inputDigest);
	printf("collision coverage name=%s real_feature_pairs=%u fifth_words=%u\n", nxSnanName("contact_emit"),
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
	for(int snanPass = 0; snanPass < 2; ++snanPass)
	{
	NxSnanPass snanScope(snanPass);
	typedef const void*(__thiscall* NxOracleRaycastFn)(const void*, const NxRay*, NxReal,
		NxU32, NxU32, NxRaycastHit*);
	NxOracleRaycastFn oracleRaycast = (NxOracleRaycastFn) (base + kPlaneRaycastRva);

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
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
			if(tame)
				(&ray.orig.x)[k] = nxUnit(&state) * 4.0f - 2.0f;
			else
				nxPickWord(&state, &(&ray.orig.x)[k]);

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
				if(tame)
					(&ray.dir.x)[k] = nxUnit(&state) * 2.0f - 1.0f;
				else
					nxPickWord(&state, &(&ray.dir.x)[k]);
			if(tame)
				maxDistance = nxUnit(&state) * 6.0f;
			else
				nxPickWord(&state, &maxDistance);
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
			if(mode == 0)
				{
				nxFoldInputShape(&inputDigest, shape0);
				nxFoldInput(&inputDigest, &ray, sizeof(ray));
				nxFoldInput(&inputDigest, &maxDistance, sizeof(maxDistance));
				nxFoldInput(&inputDigest, &hintFlags, sizeof(hintFlags));
				}
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
			nxSnanDiscrete(mode, fromOracle, fromCandidate);
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
				nxSnanWord(mode, a, b);
				}
			}
		}
	totalMismatch += nxSnanGate("shape_raycast_plane", mismatches);
	printf("collision name=%s index=- rva=0x%08x owner=phys_fn_001261 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n", nxSnanName("shape_raycast_plane"),
		kPlaneRaycastRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	nxPrintInput(nxSnanName("shape_raycast_plane"), &inputDigest);
	printf("collision coverage name=%s hits=%u wrote_normal=%u aimed=%u\n", nxSnanName("shape_raycast_plane"),
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
	for(int snanPass = 0; snanPass < 2; ++snanPass)
	{
	NxSnanPass snanScope(snanPass);
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

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
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
						if(tame)
							capsuleShape->translation[k] = nxUnit(&local) * 3.0f - 1.5f;
						else
							nxPickWord(&local, &capsuleShape->translation[k]);

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
				if(mode == 0)
					{
					nxFoldInputShape(&inputDigest, world[0].plane);
					nxFoldInputShape(&inputDigest, world[0].sphere);
					}
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
			mismatches += nxCompareStreams(&world[0], &world[1], mode);
			}
		}
	totalMismatch += nxSnanGate("contact_plane_capsule", mismatches);
	printf("collision name=%s index=3 rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n", nxSnanName("contact_plane_capsule"),
		nxMatrixA[3].rva, nxMatrixA[3].stableId,
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	// one/two are the oracle's own contact count moving by one or by two, which
	// is the multiple-contact rule; swept_emitted is how often the vtable call
	// reported a hit. w15 is a header, a normal block and two records in one
	// call -- the state only this entry can produce.
	nxPrintInput(nxSnanName("contact_plane_capsule"), &inputDigest);
	printf("collision coverage name=%s emitted=%u one=%u two=%u swept=%u swept_emitted=%u zero_axis=%u w4=%u w8=%u w11=%u w15=%u\n", nxSnanName("contact_plane_capsule"),
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
	for(int snanPass = 0; snanPass < 2; ++snanPass)
	{
	NxSnanPass snanScope(snanPass);
	typedef const void*(__thiscall* NxOracleRaycastFn)(const void*, const NxRay*, NxReal,
		NxU32, NxU32, NxRaycastHit*);
	NxOracleRaycastFn oracleRaycast = (NxOracleRaycastFn) (base + kSphereRaycastRva);

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
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
			if(tame)
				shape0->translation[k] = nxUnit(&state) * 4.0f - 2.0f;
			else
				nxPickWord(&state, &shape0->translation[k]);
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
				if(tame)
					(&ray.orig.x)[k] = nxUnit(&state) * 4.0f - 2.0f;
				else
					nxPickWord(&state, &(&ray.orig.x)[k]);
				if(tame)
					(&ray.dir.x)[k] = nxUnit(&state) * 2.0f - 1.0f;
				else
					nxPickWord(&state, &(&ray.dir.x)[k]);
				}
			if(tame)
				maxDistance = nxUnit(&state) * 6.0f;
			else
				nxPickWord(&state, &maxDistance);
			}

		NxU32 hintFlags = nxNext(&state) & 0xfbu;
		if(nxNext(&state) & 1)
			hintFlags |= NX_RAYCAST_NORMAL;

		for(int mode = 0; mode < 2; ++mode)
			{
			NxRaycastHit hit[2];
			memset(hit, 0xcd, sizeof(hit));

			nxSetControl(mode ? kControlSimulate : kControlDefault);
			if(mode == 0)
				{
				nxFoldInputShape(&inputDigest, shape0);
				nxFoldInput(&inputDigest, &ray, sizeof(ray));
				nxFoldInput(&inputDigest, &maxDistance, sizeof(maxDistance));
				nxFoldInput(&inputDigest, &hintFlags, sizeof(hintFlags));
				}
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
			nxSnanDiscrete(mode, fromOracle, fromCandidate);
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
				nxSnanWord(mode, a, b);
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
	totalMismatch += nxSnanGate("shape_raycast_sphere", perMode[0]);
	printf("collision name=%s index=- rva=0x%08x owner=phys_fn_001377 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n", nxSnanName("shape_raycast_sphere"),
		kSphereRaycastRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	nxPrintInput(nxSnanName("shape_raycast_sphere"), &inputDigest);
	printf("collision coverage name=%s hits=%u wrote_normal=%u aimed=%u behind=%u default_mismatches=%u simulate_mismatches=%u\n", nxSnanName("shape_raycast_sphere"),
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
	for(int snanPass = 0; snanPass < 2; ++snanPass)
	{
	NxSnanPass snanScope(snanPass);
	typedef void(__cdecl* NxOracleContactFn)(const NxCollisionShape*, const NxCollisionShape*,
		NxContactSink*, void*);
	NxOracleContactFn oracleContact = (NxOracleContactFn) (base + nxMatrixA[9].rva);

	static NxContactWorld world[2];
	world[0].shapeVtable[5] = (void*) (base + kSphereRaycastRva);
	world[1].shapeVtable[5] = (void*) &NxShapeRaycastSphere;

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
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
					if(tame)
						capsuleShape->translation[k] = nxUnit(&local) * 3.0f - 1.5f;
					else
						nxPickWord(&local, &capsuleShape->translation[k]);

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
						if(tame)
							sphereShape->translation[k] = nxUnit(&local) * 3.0f - 1.5f;
						else
							nxPickWord(&local, &sphereShape->translation[k]);

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
				if(mode == 0)
					{
					nxFoldInputShape(&inputDigest, world[0].plane);
					nxFoldInputShape(&inputDigest, world[0].sphere);
					}
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
			mismatches += nxCompareStreams(&world[0], &world[1], mode);
			}
		}
	totalMismatch += nxSnanGate("contact_sphere_capsule", mismatches);
	printf("collision name=%s index=9 rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n", nxSnanName("contact_sphere_capsule"),
		nxMatrixA[9].rva, nxMatrixA[9].stableId,
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	nxPrintInput(nxSnanName("contact_sphere_capsule"), &inputDigest);
	printf("collision coverage name=%s emitted=%u swept=%u swept_emitted=%u zero_axis=%u coincident=%u beyond_end=%u w4=%u w8=%u w11=%u\n", nxSnanName("contact_sphere_capsule"),
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

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
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
							nxPickWord(&local, &boxShape->translation[k]);
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
				if(mode == 0)
					{
					nxFoldInputShape(&inputDigest, world[0].plane);
					nxFoldInputShape(&inputDigest, world[0].sphere);
					}
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
			const unsigned differing = nxCompareStreams(&world[0], &world[1], mode);
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
	nxPrintInput("contact_plane_box", &inputDigest);
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

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
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
			if(tame)
				shape0->translation[k] = nxUnit(&state) * 4.0f - 2.0f;
			else
				nxPickWord(&state, &shape0->translation[k]);
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
				if(tame)
					(&ray.orig.x)[k] = nxUnit(&state) * 4.0f - 2.0f;
				else
					nxPickWord(&state, &(&ray.orig.x)[k]);
				if(tame)
					(&ray.dir.x)[k] = nxUnit(&state) * 2.0f - 1.0f;
				else
					nxPickWord(&state, &(&ray.dir.x)[k]);
				}
			if(tame)
				maxDistance = nxUnit(&state) * 8.0f;
			else
				nxPickWord(&state, &maxDistance);
			}

		// Every hint flag combination, because this row ignores all of them and
		// that is the thing worth measuring.
		NxU32 hintFlags = nxNext(&state) & 0xffu;

		for(int mode = 0; mode < 2; ++mode)
			{
			NxRaycastHit hit[2];
			memset(hit, 0xcd, sizeof(hit));

			nxSetControl(mode ? kControlSimulate : kControlDefault);
			if(mode == 0)
				{
				nxFoldInputShape(&inputDigest, shape0);
				nxFoldInput(&inputDigest, &ray, sizeof(ray));
				nxFoldInput(&inputDigest, &maxDistance, sizeof(maxDistance));
				nxFoldInput(&inputDigest, &hintFlags, sizeof(hintFlags));
				}
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
	nxPrintInput("shape_raycast_capsule", &inputDigest);
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

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
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
					if(tame)
						first->translation[k] = nxUnit(&local) * 3.0f - 1.5f;
					else
						nxPickWord(&local, &first->translation[k]);

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
						if(tame)
							second->translation[k] = nxUnit(&local) * 3.0f - 1.5f;
						else
							nxPickWord(&local, &second->translation[k]);

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
				if(mode == 0)
					{
					nxFoldInputShape(&inputDigest, world[0].plane);
					nxFoldInputShape(&inputDigest, world[0].sphere);
					}
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
			const unsigned differing = nxCompareStreams(&world[0], &world[1], mode);
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
	nxPrintInput("contact_capsule_capsule", &inputDigest);
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

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
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
			if(tame)
				words0[k] = nxUnit(&state) * 6.0f - 3.0f;
			else
				nxPickWord(&state, &words0[k]);
			if(tame)
				words1[k] = nxUnit(&state) * 6.0f - 3.0f;
			else
				nxPickWord(&state, &words1[k]);
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
		nxFoldInput(&inputDigest, segment, sizeof(segment));
		nxFoldInput(&inputDigest, &nulls, sizeof(nulls));

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
	nxPrintInput("segment_segment", &inputDigest);
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

	// Two digests, because these are two rows: a registration that covered both
	// with one could lose either without the pinned number moving.
	NxDigest oracleDigest, candidateDigest, inputDigest;
	NxDigest guardOracleDigest, guardCandidateDigest;
	nxDigestInit(&inputDigest);
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&guardOracleDigest);
	nxDigestInit(&guardCandidateDigest);
	unsigned mismatches = 0;
	unsigned guardMismatches = 0;
	unsigned ownerProbes = 0;
	unsigned staticFirst = 0;
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
		if(staticSide == 0)
			++staticFirst;
		*(unsigned char**) (ownerStorage[staticSide] + 8) = 0;
		*(unsigned char**) (ownerStorage[1 - staticSide] + 8) = holderStorage[1 - staticSide];
		nxFoldInputShape(&inputDigest, shapes[0]);
		nxFoldInputShape(&inputDigest, shapes[1]);
		nxFoldInput(&inputDigest, &staticSide, sizeof(staticSide));

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
			nxDigestByte(&guardOracleDigest, fromOracle);
			nxDigestByte(&guardOracleDigest, untouched ? 1 : 0);
			nxDigestByte(&guardCandidateDigest, fromCandidate);
			nxDigestByte(&guardCandidateDigest, 1);
			if(fromOracle != fromCandidate || !untouched)
				++guardMismatches;
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

	totalMismatch += mismatches + guardMismatches;
	printf("collision name=shape_owner index=- rva=0x%08x owner=phys_fn_001281 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kShapeOwnerRva, oracleDigest.checks, oracleDigest.state,
		candidateDigest.state, mismatches);
	nxPrintInput("shape_owner", &inputDigest);
	printf("collision coverage name=shape_owner probes=%u static_first=%u\n",
		ownerProbes, staticFirst);
	printf("collision name=ccd_guard index=- rva=0x%08x owner=phys_fn_002266 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kContinuousCdRva, guardOracleDigest.checks, guardOracleDigest.state,
		guardCandidateDigest.state, guardMismatches);
	nxPrintInput("ccd_guard", &inputDigest);
	printf("collision coverage name=ccd_guard continuous_cd=%08x probes=%u returned_true=%u sink_untouched=%u index_probes=%u index_wrong=%u\n",
		continuousBits, guardProbes, guardTrue, sinkUntouched, indexProbes,
		indexWrongHere);
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

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
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
						if(tame)
							firstShape->translation[k] = nxUnit(&local) * 4.0f - 2.0f;
						else
							nxPickWord(&local, &firstShape->translation[k]);

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
							nxPickWord(&local, &secondShape->translation[k]);
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
				if(mode == 0)
					{
					nxFoldInputShape(&inputDigest, world[0].plane);
					nxFoldInputShape(&inputDigest, world[0].sphere);
					}
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
			const unsigned differing = nxCompareStreams(&world[0], &world[1], mode);
			mismatches += differing;
			perMode[mode] += differing;
			}
		}
	totalMismatch += perMode[0];
	printf("collision name=contact_sphere_sphere index=7 rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		nxMatrixA[7].rva, nxMatrixA[7].stableId,
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	nxPrintInput("contact_sphere_sphere", &inputDigest);
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
	for(int snanPass = 0; snanPass < 2; ++snanPass)
	{
	NxSnanPass snanScope(snanPass);
	typedef bool (__cdecl* NxOracleSphereBoxContactFn)(const NxCollisionSphereData*,
		const NxCollisionBoxData*, NxVec3*, NxVec3*, NxReal*);
	NxOracleSphereBoxContactFn oracleContact =
		(NxOracleSphereBoxContactFn) (base + kSphereBoxContactRva);

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
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
			if(tame)
				box.center[k] = nxUnit(&state) * 2.0f - 1.0f;
			else
				nxPickWord(&state, &box.center[k]);
			box.extents[k] = boxShape->geometry[k + 1];
			}
		for(int k = 0; k < 9; ++k)
			box.rotation[k] = boxShape->rotation[k];
		if(tame)
			sphere.radius = nxUnit(&state) * 1.5f + 0.02f;
		else
			nxPickWord(&state, &sphere.radius);

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
				if(tame)
					sphere.center[k] = nxUnit(&state) * 4.0f - 2.0f;
				else
					nxPickWord(&state, &sphere.center[k]);
			}
		nxFoldInput(&inputDigest, &sphere, sizeof(sphere));
		nxFoldInput(&inputDigest, &box, sizeof(box));

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
			nxSnanDiscrete(mode, fromOracle, fromCandidate);

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
				nxSnanWord(mode, words[0][word], words[1][word]);
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
				nxSnanWord(mode, normalWords[0][word], normalWords[1][word]);
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
			nxSnanWord(mode, separationWords[0], separationWords[1]);
			}
		}
	totalMismatch += nxSnanGate("sphere_box_contact", perMode[0]);
	printf("collision name=%s index=- rva=0x%08x owner=phys_fn_001917 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n", nxSnanName("sphere_box_contact"),
		kSphereBoxContactRva, oracleDigest.checks, oracleDigest.state,
		candidateDigest.state, mismatches);
	nxPrintInput(nxSnanName("sphere_box_contact"), &inputDigest);
	printf("collision coverage name=%s true=%u centre_inside=%u axis_x=%u axis_y=%u axis_z=%u default_mismatches=%u simulate_mismatches=%u\n", nxSnanName("sphere_box_contact"),
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
	for(int snanPass = 0; snanPass < 2; ++snanPass)
	{
	NxSnanPass snanScope(snanPass);
	typedef void(__cdecl* NxOracleContactFn)(const NxCollisionShape*, const NxCollisionShape*,
		NxContactSink*, void*);
	NxOracleContactFn oracleContact = (NxOracleContactFn) (base + nxMatrixA[8].rva);

	static NxContactWorld world[2];

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
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
						if(tame)
							boxShape->translation[k] = nxUnit(&local) * 2.0f - 1.0f;
						else
							nxPickWord(&local, &boxShape->translation[k]);

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
							nxPickWord(&local, &sphereShape->translation[k]);
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
				if(mode == 0)
					{
					nxFoldInputShape(&inputDigest, world[0].plane);
					nxFoldInputShape(&inputDigest, world[0].sphere);
					}
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
			const unsigned differing = nxCompareStreams(&world[0], &world[1], mode);
			mismatches += differing;
			perMode[mode] += differing;
			}
		}
	totalMismatch += nxSnanGate("contact_sphere_box", perMode[0]);
	printf("collision name=%s index=8 rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n", nxSnanName("contact_sphere_box"),
		nxMatrixA[8].rva, nxMatrixA[8].stableId,
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	nxPrintInput(nxSnanName("contact_sphere_box"), &inputDigest);
	printf("collision coverage name=%s emitted=%u centre_inside=%u repeated=%u static0=%u static1=%u negated=%u w4=%u w8=%u w11=%u default_mismatches=%u simulate_mismatches=%u\n", nxSnanName("contact_sphere_box"),
		emitted, insideCount, repeated, staticSide[0], staticSide[1], negatedPath,
		widths[4], widths[8], widths[11], perMode[0], perMode[1]);
	}

	// -----------------------------------------------------------------------
	// convex-mesh gap Task 2a: the box distance kernels of Distance.cpp at
	// their own recorded addresses (units/convex-mesh-gap-contract.md, sub-unit
	// E), then the entries that reach them. Each of the three leaf families is
	// half raw draws (every word through nxPickWord, which is what reaches the
	// NaN, infinity, negative-extent and denormal arms) and half aimed, and each
	// runs under both control words. As for phys_fn_001690, only the default
	// word's half gates: these rows are reached only from inside the step, and
	// under 0x0f7f MSVC's `double` spills cut 64-bit registers to 53 bits. The
	// 0x0f7f count is registered in the coverage line, so it fails if it moves.
	{
	// phys_fn_001670, point/box.
	const void* const oracleFn = (const void*) (base + kPointBoxRva);
	const void* const candidateFn = (const void*) &NxPointBoxSquareDistance;
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	unsigned perMode[2] = { 0, 0 };
	unsigned canonical = 0;
	unsigned inside = 0;
	unsigned clampedAxes[4] = { 0, 0, 0, 0 };
	unsigned nullClosest = 0;
	unsigned state = 0x70b0c5e1u;
	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		const bool aimed = (nxNext(&state) & 1) != 0;
		NxCollisionBoxData box;
		nxFillBoxData(&state, &box, aimed, aimed && (nxNext(&state) & 3) == 0);
		float point[3];
		if(aimed)
			{
			float u[3];
			nxBoxCoordinate(&state, u);
			nxBoxPoint(&box, u, point);
			}
		else
			for(int k = 0; k < 3; ++k)
				nxPickWord(&state, &point[k]);
		const bool withClosest = (nxNext(&state) % 8) != 0;
		if(!withClosest)
			++nullClosest;
		nxFoldInput(&inputDigest, point, sizeof(point));
		nxFoldInput(&inputDigest, &box, sizeof(box));
		nxFoldInput(&inputDigest, &withClosest, sizeof(withClosest));

		for(int mode = 0; mode < 2; ++mode)
			{
			unsigned char wide[2][10];
			NxReal closest[2][3];
			memset(closest, 0xcd, sizeof(closest));
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			nxCallWide6(oracleFn, point, box.center, box.extents, box.rotation,
				withClosest ? closest[0] : 0, 0, wide[0]);
			nxCallWide6(candidateFn, point, box.center, box.extents, box.rotation,
				withClosest ? closest[1] : 0, 0, wide[1]);
			nxSetControl(kControlDefault);

			if(withClosest && mode == 0)
				{
				unsigned onBoundary = 0;
				for(int k = 0; k < 3; ++k)
					if(closest[0][k] == box.extents[k] || closest[0][k] == -box.extents[k])
						++onBoundary;
				++clampedAxes[onBoundary];
				}
			if(mode == 0 && wide[0][9] == 0 && wide[0][8] == 0)
				++inside;
			nxFoldWide(&oracleDigest, &candidateDigest, wide[0], wide[1], &perMode[mode], &canonical);
			nxFoldWords(&oracleDigest, &candidateDigest, &closest[0][0], &closest[1][0], 3,
				&perMode[mode], &canonical);
			}
		}
	totalMismatch += perMode[0];
	printf("collision name=point_box index=- rva=0x%08x owner=phys_fn_001670 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kPointBoxRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state,
		perMode[0] + perMode[1]);
	nxPrintInput("point_box", &inputDigest);
	printf("collision coverage name=point_box inside=%u boundary0=%u boundary1=%u boundary2=%u boundary3=%u null_closest=%u canonical_nan=%u default_mismatches=%u simulate_mismatches=%u\n",
		inside, clampedAxes[0], clampedAxes[1], clampedAxes[2], clampedAxes[3], nullClosest,
		canonical, perMode[0], perMode[1]);
	}

	{
	// phys_fn_001684 with its continuation phys_fn_001686 and the five
	// register-convention helpers it dispatches to (001674 Face, 001676
	// CaseNoZeros, 001678 Case0, 001680 Case00, 001682 Case000). The aimed
	// half gives the box the identity frame and zeroes exactly `zeros` of the
	// direction's components, so the four dispatch arms are reached by
	// construction and counted by it; the direction's signs are drawn so the
	// reflection loop runs both ways on every axis. Lines through the box
	// (squared distance 0) and parallel to a face are aimed at explicitly.
	const void* const oracleFn = (const void*) (base + kLineBoxRva);
	const void* const candidateFn = (const void*) &NxLineBoxSquareDistance;
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	unsigned perMode[2] = { 0, 0 };
	unsigned canonical = 0;
	unsigned zeroCounts[4] = { 0, 0, 0, 0 };
	unsigned through = 0;
	unsigned intersecting = 0;
	unsigned boundary[4] = { 0, 0, 0, 0 };
	unsigned nullParam = 0;
	unsigned state = 0x11eb0c5au;
	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		const bool aimed = (nxNext(&state) & 1) != 0;
		const bool identity = aimed && (nxNext(&state) & 1) != 0;
		NxCollisionBoxData box;
		nxFillBoxData(&state, &box, aimed, identity);
		NxDistanceLine line;
		if(aimed)
			{
			float u[3];
			nxBoxCoordinate(&state, u);
			nxBoxPoint(&box, u, line.origin);
			float direction[3];
			for(int k = 0; k < 3; ++k)
				direction[k] = (nxUnit(&state) * 2.0f - 1.0f) * 3.0f;
			if(identity)
				{
				// In the identity frame the kernel's direction is this one
				// word for word, so `zeros` is exactly how many components
				// the dispatch sees as zero.
				const unsigned zeros = nxNext(&state) % 4;
				const unsigned start = nxNext(&state) % 3;
				for(unsigned j = 0; j < zeros; ++j)
					direction[(start + j) % 3] = (j == 0 && (nxNext(&state) & 1)) ? -0.0f : 0.0f;
				++zeroCounts[zeros];
				}
			if((nxNext(&state) & 3) == 0)
				{
				// Through the box: aim the direction at a point inside it.
				float target[3];
				float v[3];
				for(int k = 0; k < 3; ++k)
					v[k] = (nxUnit(&state) * 2.0f - 1.0f) * 0.9f;
				nxBoxPoint(&box, v, target);
				for(int k = 0; k < 3; ++k)
					direction[k] = target[k] - line.origin[k];
				++through;
				}
			for(int k = 0; k < 3; ++k)
				line.direction[k] = direction[k];
			}
		else
			for(int k = 0; k < 3; ++k)
				{
				nxPickWord(&state, &line.origin[k]);
				nxPickWord(&state, &line.direction[k]);
				}
		const bool withParam = (nxNext(&state) % 8) != 0;
		if(!withParam)
			++nullParam;
		nxFoldInput(&inputDigest, &line, sizeof(line));
		nxFoldInput(&inputDigest, &box, sizeof(box));
		nxFoldInput(&inputDigest, &withParam, sizeof(withParam));

		for(int mode = 0; mode < 2; ++mode)
			{
			unsigned char wide[2][10];
			NxReal out[2][4];
			memset(out, 0xcd, sizeof(out));
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			nxCallWide6(oracleFn, &line, &box, withParam ? &out[0][0] : 0,
				&out[0][1], &out[0][2], &out[0][3], wide[0]);
			nxCallWide6(candidateFn, &line, &box, withParam ? &out[1][0] : 0,
				&out[1][1], &out[1][2], &out[1][3], wide[1]);
			nxSetControl(kControlDefault);

			if(mode == 0)
				{
				if(wide[0][9] == 0 && wide[0][8] == 0)
					++intersecting;
				if(withParam)
					{
					unsigned onBoundary = 0;
					for(int k = 0; k < 3; ++k)
						if(out[0][1 + k] == box.extents[k] || out[0][1 + k] == -box.extents[k])
							++onBoundary;
					++boundary[onBoundary];
					}
				}
			nxFoldWide(&oracleDigest, &candidateDigest, wide[0], wide[1], &perMode[mode], &canonical);
			nxFoldWords(&oracleDigest, &candidateDigest, &out[0][0], &out[1][0], 4,
				&perMode[mode], &canonical);
			}
		}
	totalMismatch += perMode[0];
	printf("collision name=line_box index=- rva=0x%08x owner=phys_fn_001684 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kLineBoxRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state,
		perMode[0] + perMode[1]);
	nxPrintInput("line_box", &inputDigest);
	printf("collision coverage name=line_box zeros0=%u zeros1=%u zeros2=%u zeros3=%u through=%u intersecting=%u boundary0=%u boundary1=%u boundary2=%u boundary3=%u null_param=%u canonical_nan=%u default_mismatches=%u simulate_mismatches=%u\n",
		zeroCounts[0], zeroCounts[1], zeroCounts[2], zeroCounts[3], through, intersecting,
		boundary[0], boundary[1], boundary[2], boundary[3], nullParam, canonical,
		perMode[0], perMode[1]);
	}

	{
	// phys_fn_001688, segment/box: line/box, then the clamp to [0, 1] and
	// point/box at whichever end was passed. The segment parameter the oracle
	// returns says which of the three arms ran.
	const void* const oracleFn = (const void*) (base + kSegmentBoxRva);
	const void* const candidateFn = (const void*) &NxSegmentBoxSquareDistance;
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	unsigned perMode[2] = { 0, 0 };
	unsigned canonical = 0;
	unsigned atStart = 0;
	unsigned atEnd = 0;
	unsigned interior = 0;
	unsigned intersecting = 0;
	unsigned degenerate = 0;
	unsigned nullOutputs = 0;
	unsigned state = 0x5e9b0c33u;
	for(unsigned i = 0; i < kPairIterations; ++i)
		{
		const bool aimed = (nxNext(&state) & 1) != 0;
		const bool identity = aimed && (nxNext(&state) & 3) == 0;
		NxCollisionBoxData box;
		nxFillBoxData(&state, &box, aimed, identity);
		NxSegment segment;
		if(aimed)
			{
			float u0[3];
			float u1[3];
			nxBoxCoordinate(&state, u0);
			nxBoxCoordinate(&state, u1);
			nxBoxPoint(&box, u0, &segment.p0.x);
			nxBoxPoint(&box, u1, &segment.p1.x);
			const unsigned shape = nxNext(&state) % 6;
			if(shape == 0)
				{
				// Zero length: the line's direction is zero and line/box
				// takes Case000.
				segment.p1 = segment.p0;
				++degenerate;
				}
			else if(shape == 1 && identity)
				{
				// Parallel to a face of the axis-aligned box.
				const unsigned axis = nxNext(&state) % 3;
				(&segment.p1.x)[axis] = (&segment.p0.x)[axis];
				}
			}
		else
			{
			float* words = &segment.p0.x;
			for(int k = 0; k < 6; ++k)
				nxPickWord(&state, &words[k]);
			}
		const unsigned nulls = (nxNext(&state) % 8 == 0) ? (1 + nxNext(&state) % 3) : 0;
		if(nulls)
			++nullOutputs;
		nxFoldInput(&inputDigest, &segment, sizeof(segment));
		nxFoldInput(&inputDigest, &box, sizeof(box));
		nxFoldInput(&inputDigest, &nulls, sizeof(nulls));

		for(int mode = 0; mode < 2; ++mode)
			{
			unsigned char wide[2][10];
			NxReal out[2][4];
			memset(out, 0xcd, sizeof(out));
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			nxCallWide6(oracleFn, &segment, box.center, box.extents, box.rotation,
				(nulls & 1) ? 0 : &out[0][0], (nulls & 2) ? 0 : &out[0][1], wide[0]);
			nxCallWide6(candidateFn, &segment, box.center, box.extents, box.rotation,
				(nulls & 1) ? 0 : &out[1][0], (nulls & 2) ? 0 : &out[1][1], wide[1]);
			nxSetControl(kControlDefault);

			if(mode == 0)
				{
				if(!(nulls & 1))
					{
					if(out[0][0] == 0.0f)
						++atStart;
					else if(out[0][0] == 1.0f)
						++atEnd;
					else if(out[0][0] > 0.0f && out[0][0] < 1.0f)
						++interior;
					}
				if(wide[0][9] == 0 && wide[0][8] == 0)
					++intersecting;
				}
			nxFoldWide(&oracleDigest, &candidateDigest, wide[0], wide[1], &perMode[mode], &canonical);
			nxFoldWords(&oracleDigest, &candidateDigest, &out[0][0], &out[1][0], 4,
				&perMode[mode], &canonical);
			}
		}
	totalMismatch += perMode[0];
	printf("collision name=segment_box index=- rva=0x%08x owner=phys_fn_001688 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		kSegmentBoxRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state,
		perMode[0] + perMode[1]);
	nxPrintInput("segment_box", &inputDigest);
	printf("collision coverage name=segment_box at_start=%u at_end=%u interior=%u intersecting=%u zero_length=%u null_outputs=%u canonical_nan=%u default_mismatches=%u simulate_mismatches=%u\n",
		atStart, atEnd, interior, intersecting, degenerate, nullOutputs, canonical,
		perMode[0], perMode[1]);
	}

	// -----------------------------------------------------------------------
	// phys_fn_001753, matrix A [BOX][CAPSULE] (index 15), built like
	// contact_capsule_capsule: sequences of one to four pairs into one sink per
	// side, every stream folded whole. The capsule is placed against the box in
	// its own frame -- through a face, along an edge, at a corner, with its
	// axis parallel to a box axis, and centred in the box (the crossing
	// branch's box/box search) -- and the swept flag is driven on its own.
	// Slot 5 of the BOX is the oracle's raycast on both sides (see
	// kBoxRaycastRva).
	for(int snanPass = 0; snanPass < 2; ++snanPass)
	{
	NxSnanPass snanScope(snanPass);
	typedef void(__cdecl* NxOracleContactFn)(const NxCollisionShape*, const NxCollisionShape*,
		NxContactSink*, void*);
	NxOracleContactFn oracleContact = (NxOracleContactFn) (base + nxMatrixA[15].rva);

	typedef int(__cdecl* NxOracleShimFn)(NxVec3*, NxReal*, NxVec3*, const NxReal*,
		const NxReal*, const NxReal*, const NxReal*, unsigned char*);
	NxOracleShimFn oracleShim = (NxOracleShimFn) (base + 0x0003ace0);
	// Counted under each control word: the probe runs under the word the pair
	// is driven under, so the two could differ. The registered line prints the
	// default word's count and the harness fails if the simulate word's is not
	// the same.
	unsigned overflowSkipped[2] = { 0, 0 };
	unsigned probeMax = 0;

	static NxContactWorld world[2];
	world[0].shapeVtable[5] = (void*) (base + kBoxRaycastRva);
	world[1].shapeVtable[5] = (void*) (base + kBoxRaycastRva);

	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	unsigned perMode[2] = { 0, 0 };
	unsigned emitted = 0;
	unsigned swept = 0;
	unsigned sweptEmitted = 0;
	unsigned centred = 0;
	unsigned parallel = 0;
	unsigned zeroAxis = 0;
	unsigned contactCounts[8];
	unsigned maxContacts = 0;
	memset(contactCounts, 0, sizeof(contactCounts));
	unsigned state = 0x2b0c5e17u;

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
				static unsigned char boxStorage[kShapeBytes];
				static unsigned char capsuleStorage[kShapeBytes];
				NxCollisionShape* box = (NxCollisionShape*) boxStorage;
				NxCollisionShape* capsule = (NxCollisionShape*) capsuleStorage;
				const bool tame = (nxNext(&local) & 7) != 0;
				nxIdentity(box);
				nxIdentity(capsule);
				nxFillGeometry(&local, box, 2, tame);
				nxFillGeometry(&local, capsule, 3, tame);
				const bool boxRotated = (nxNext(&local) & 1) != 0;
				if(boxRotated)
					nxRandomRotation(&local, box);
				for(int k = 0; k < 3; ++k)
					if(tame)
						box->translation[k] = nxUnit(&local) * 2.0f - 1.0f;
					else
						nxPickWord(&local, &box->translation[k]);

				// The capsule's axis: random, along a box axis (so the axis
				// is parallel to four faces), or zero length.
				const unsigned axisMode = nxNext(&local) % 4;
				if(axisMode == 0 || axisMode == 3)
					nxRandomRotation(&local, capsule);
				else if(axisMode == 1)
					{
					const unsigned column = nxNext(&local) % 3;
					const float sign = (nxNext(&local) & 1) ? 1.0f : -1.0f;
					for(int r = 0; r < 3; ++r)
						capsule->rotation[r * 3 + 1] = box->rotation[r * 3 + column] * sign;
					++parallel;
					}
				else
					{
					capsule->geometry[1] = 0.0f;
					++zeroAxis;
					}

				// Placement in the box's frame, gated on a finite box.
				bool placed = false;
				if(tame)
					{
					NxCollisionBoxData data;
					for(int k = 0; k < 3; ++k)
						{
						data.center[k] = box->translation[k];
						data.extents[k] = box->geometry[1 + k];
						}
					for(int k = 0; k < 9; ++k)
						data.rotation[k] = box->rotation[k];
					float u[3];
					const unsigned where = nxNext(&local) % 4;
					if(where == 0)
						{
						u[0] = u[1] = u[2] = 0.0f;
						++centred;
						}
					else
						{
						nxBoxCoordinate(&local, u);
						// Pushed out along the outward direction by up to a
						// radius and a half, so separated, grazing and
						// penetrating placements are all reached.
						const float push = 1.0f + (nxUnit(&local) * 1.5f - 0.5f)
							* capsule->geometry[0] / (data.extents[0] + data.extents[1]
								+ data.extents[2] + 0.1f);
						for(int k = 0; k < 3; ++k)
							u[k] *= push;
						}
					nxBoxPoint(&data, u, capsule->translation);
					placed = true;
					}
				if(!placed)
					for(int k = 0; k < 3; ++k)
						nxPickWord(&local, &capsule->translation[k]);

				NxU32 flagWord = nxNext(&local);
				const bool isSwept = (nxNext(&local) % 5) == 0;
				flagWord = isSwept ? (flagWord | 1u) : (flagWord & ~1u);
				memcpy(&capsule->geometry[2], &flagWord, 4);
				if(isSwept)
					++swept;

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
					nxStageWorld(&world[side], box, capsule,
						newIdentity0, newIdentity1, material0, material1,
						false, nullHolder1, orientToSecond);
					*(void**) world[side].plane = world[side].shapeVtable;
					}

				// The pre-flight contact_box_box uses, for the same reason: the
				// crossing branch writes its manifold into sixteen slots whose
				// seventeenth is the return address. phys_fn_001748 is probed
				// with this entry's own arguments -- the pseudo box, the capsule's
				// pose, the box's -- and a copy of the live cache byte; a pair it
				// says would overflow is counted and not driven. Whether the entry
				// would have taken the crossing branch at all is not asked, so the
				// count is an upper bound on the pairs the overrun really costs.
				{
				nxSetControl(mode ? kControlSimulate : kControlDefault);
				NxReal pseudoExtents[3];
				pseudoExtents[0] = nxCapsulePseudoExtent(world[0].sphere->geometry[0]);
				pseudoExtents[1] = world[0].sphere->geometry[1];
				pseudoExtents[2] = pseudoExtents[0];
				unsigned char probeAxis = world[0].sink.separatingAxis;
				NxVec3 probePoints[80];
				NxReal probeSeparations[80];
				NxVec3 probeNormal;
				const int probeCount = oracleShim(probePoints, probeSeparations, &probeNormal,
					pseudoExtents, &world[0].sphere->rotation[0], &world[0].plane->geometry[1],
					&world[0].plane->rotation[0], &probeAxis);
				nxSetControl(kControlDefault);
				if(mode == 0 && probeCount > (int) probeMax)
					probeMax = (unsigned) probeCount;
				if(probeCount > 16)
					{
					++overflowSkipped[mode];
					continue;
					}
				}

				nxSetControl(mode ? kControlSimulate : kControlDefault);
				if(mode == 0)
					{
					nxFoldInputShape(&inputDigest, world[0].plane);
					nxFoldInputShape(&inputDigest, world[0].sphere);
					}
				oracleContact(world[0].plane, world[0].sphere, &world[0].sink, nxOverlapContext);
				NxContactBoxCapsule(world[1].plane, world[1].sphere, &world[1].sink, nxOverlapContext);
				nxSetControl(kControlDefault);

				if(mode == 0)
					{
					const unsigned appended = world[0].sink.streamCount - before;
					const unsigned contacts = world[0].sink.contactCount - contactsBefore;
					if(appended)
						++emitted;
					if(isSwept && appended)
						++sweptEmitted;
					++contactCounts[contacts < 7 ? contacts : 7];
					if(contacts > maxContacts)
						maxContacts = contacts;
					}
				}

			nxFoldStream(&oracleDigest, &world[0]);
			nxFoldStream(&candidateDigest, &world[1]);
			const unsigned differing = nxCompareStreams(&world[0], &world[1], mode);
			perMode[mode] += differing;
			if(world[0].sink.separatingAxis != world[1].sink.separatingAxis)
				++perMode[mode];
			nxDigestByte(&oracleDigest, world[0].sink.separatingAxis);
			nxDigestByte(&candidateDigest, world[1].sink.separatingAxis);
			}
		}
	totalMismatch += nxSnanGate("contact_box_capsule", perMode[0]);
	printf("collision name=%s index=15 rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n", nxSnanName("contact_box_capsule"),
		nxMatrixA[15].rva, nxMatrixA[15].stableId,
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, perMode[0] + perMode[1]);
	nxPrintInput(nxSnanName("contact_box_capsule"), &inputDigest);
	printf("collision coverage name=%s emitted=%u swept=%u swept_emitted=%u centred=%u parallel=%u zero_axis=%u c0=%u c1=%u c2=%u c3=%u c4=%u c5=%u c6=%u c7plus=%u max_contacts=%u overflow_skipped=%u probe_max=%u default_mismatches=%u simulate_mismatches=%u\n", nxSnanName("contact_box_capsule"),
		emitted, swept, sweptEmitted, centred, parallel, zeroAxis,
		contactCounts[0], contactCounts[1], contactCounts[2], contactCounts[3],
		contactCounts[4], contactCounts[5], contactCounts[6], contactCounts[7],
		maxContacts, overflowSkipped[0], probeMax, perMode[0], perMode[1]);
	if(overflowSkipped[1] != overflowSkipped[0])
		{
		printf("collision coverage name=contact_box_capsule overflow_skipped_simulate=%u differs\n",
			overflowSkipped[1]);
		++totalMismatch;
		}
	}

	// -----------------------------------------------------------------------
	// The three matrix B compound entries: [SPHERE][COMPOUND] (11, 001789),
	// [BOX][COMPOUND] (17, 001791), [CAPSULE][COMPOUND] (23, 001785). None of
	// them walks children: each tests its primitive against the compound's own
	// world bounds, read from the Prunable embedded at Shape+0xa4 -- flags at
	// +0xac, the pruner at +0xc4 (its world-box array at +0x14) and the handle
	// at +0xcc. Each side gets its own compound, pruner and box array, because
	// the entry sets the prunable's "world box valid" bit through
	// phys_fn_004886 when it is clear, and that bit is compared after the call.
	// The bounds are drawn physical, inverted (min above max, negative half
	// sizes), flat (one axis zero) or raw; the invalid handle 0xffff is not
	// driven, because both implementations then read through a null box.
	{
	const void* ownerCallback = *(void* const*) (base + kPrunableOwnerWorldAabbRva);
	printf("collision compound owner_world_aabb_callback=%s\n", ownerCallback ? "set" : "null");
	if(ownerCallback)
		return nxFail("the oracle's Prunable owner callback is set; the compound entries would call it");

	struct NxCompoundEntry
		{
		const char* name;
		unsigned index;
		NxShapeOverlapFn candidate;
		unsigned type0;
		};
	static const NxCompoundEntry entries[] =
		{
		{ "sphere_compound",  1 * 6 + 5, NxOverlapSphereCompound,  1 },
		{ "box_compound",     2 * 6 + 5, NxOverlapBoxCompound,     2 },
		{ "capsule_compound", 3 * 6 + 5, NxOverlapCapsuleCompound, 3 }
		};

	for(unsigned e = 0; e < sizeof(entries) / sizeof(entries[0]); ++e)
		{
		const unsigned index = entries[e].index;
		NxOracleOverlapFn oracle = (NxOracleOverlapFn) (base + nxMatrixB[index].rva);
		NxDigest oracleDigest, candidateDigest, inputDigest;
		nxDigestInit(&oracleDigest);
		nxDigestInit(&candidateDigest);
		nxDigestInit(&inputDigest);
		unsigned perMode[2] = { 0, 0 };
		unsigned trueCount = 0;
		unsigned falseCount = 0;
		unsigned refreshed = 0;
		unsigned unflagged = 0;
		unsigned inverted = 0;
		unsigned state = 0x3c0b0d11u ^ (index * 0x9e3779b9u);

		static unsigned char primitiveStorage[kShapeBytes];
		static unsigned char compoundStorage[2][kShapeBytes];
		NxCollisionShape* primitive = (NxCollisionShape*) primitiveStorage;
		NxU32 prunerStorage[2][8];
		NxReal boxes[2][4][6];

		for(unsigned i = 0; i < kPairIterations; ++i)
			{
			const bool tame = (nxNext(&state) & 7) != 0;
			nxIdentity(primitive);
			nxRandomRotation(&state, primitive);
			nxFillGeometry(&state, primitive, entries[e].type0, tame);

			// The bounds, in the harness's own box array; the handle picks
			// one of four entries.
			const unsigned handle = nxNext(&state) % 4;
			NxReal bounds[4][6];
			for(int b = 0; b < 4; ++b)
				for(int k = 0; k < 3; ++k)
					{
					if(tame)
						{
						const float c = nxUnit(&state) * 4.0f - 2.0f;
						const float h = nxUnit(&state) * 1.5f + 0.02f;
						bounds[b][k] = c - h;
						bounds[b][3 + k] = c + h;
						}
					else
						{
						// Raw: the two words are the draws themselves. They
						// were c - h and c + h over two raw draws, and SSE's
						// NaN rule keeps the FIRST operand's payload, so which
						// NaN the commutative sum carried depended on the
						// operand order the compiler emitted (harness hardening).
						nxPickWord(&state, &bounds[b][k]);
						nxPickWord(&state, &bounds[b][3 + k]);
						}
					}
			const unsigned boundsMode = nxNext(&state) % 8;
			if(tame && boundsMode == 0)
				{
				// Inverted: min above max, so every half size is negative.
				for(int k = 0; k < 3; ++k)
					{
					const NxReal swap = bounds[handle][k];
					bounds[handle][k] = bounds[handle][3 + k];
					bounds[handle][3 + k] = swap;
					}
				++inverted;
				}
			else if(tame && boundsMode == 1)
				{
				// Flat: one max bound set to one min bound. The two draws are
				// sequenced here; as one expression their order was the compiler's.
				const unsigned target = 3 + nxNext(&state) % 3;
				const unsigned source = nxNext(&state) % 3;
				bounds[handle][target] = bounds[handle][source];
				}

			// The primitive near the chosen bounds: inside, touching or
			// just clear, gated on the bounds being finite.
			for(int k = 0; k < 3; ++k)
				{
				if(tame)
					{
					const float lo = bounds[handle][k];
					const float hi = bounds[handle][3 + k];
					const float span = hi - lo;
					primitive->translation[k] = lo + span * (nxUnit(&state) * 2.4f - 0.7f);
					}
				else
					nxPickWord(&state, &primitive->translation[k]);
				}

			NxU32 boxFlags = nxNext(&state);
			if((nxNext(&state) & 3) == 0)
				boxFlags &= ~7u;
			memcpy((unsigned char*) primitive + 0xdc, &boxFlags, 4);
			if(entries[e].type0 == 2 && !(((const unsigned char*) primitive)[0xde] & 7))
				++unflagged;

			const bool valid = (nxNext(&state) & 1) != 0;
			const NxU32 flagWord = valid ? (nxNext(&state) | 2u) : (nxNext(&state) & ~2u);
			if(!valid)
				++refreshed;
			nxFoldInputShape(&inputDigest, primitive);
			nxFoldInput(&inputDigest, bounds, sizeof(bounds));
			nxFoldInput(&inputDigest, &handle, sizeof(handle));
			nxFoldInput(&inputDigest, &flagWord, sizeof(flagWord));

			for(int mode = 0; mode < 2; ++mode)
				{
				unsigned char result[2];
				NxU32 flagsAfter[2];
				for(int side = 0; side < 2; ++side)
					{
					unsigned char* compound = compoundStorage[side];
					memset(compound, 0, kShapeBytes);
					((NxCollisionShape*) compound)->type = 5;
					memcpy(boxes[side], bounds, sizeof(bounds));
					memset(prunerStorage[side], 0, sizeof(prunerStorage[side]));
					prunerStorage[side][5] = (NxU32) (size_t) &boxes[side][0][0];
					const NxU32 prunerAddress = (NxU32) (size_t) prunerStorage[side];
					const unsigned short handleWord = (unsigned short) handle;
					memcpy(compound + 0xac, &flagWord, 4);
					memcpy(compound + 0xc4, &prunerAddress, 4);
					memcpy(compound + 0xcc, &handleWord, 2);
					}

				nxSetControl(mode ? kControlSimulate : kControlDefault);
				result[0] = oracle(primitive, (const NxCollisionShape*) compoundStorage[0],
					nxOverlapContext) ? 1 : 0;
				result[1] = entries[e].candidate(primitive,
					(const NxCollisionShape*) compoundStorage[1]) ? 1 : 0;
				nxSetControl(kControlDefault);

				for(int side = 0; side < 2; ++side)
					memcpy(&flagsAfter[side], compoundStorage[side] + 0xac, 4);
				nxDigestByte(&oracleDigest, result[0]);
				nxDigestByte(&candidateDigest, result[1]);
				for(int byte = 0; byte < 4; ++byte)
					{
					nxDigestByte(&oracleDigest, (unsigned char) (flagsAfter[0] >> (byte * 8)));
					nxDigestByte(&candidateDigest, (unsigned char) (flagsAfter[1] >> (byte * 8)));
					}
				if(result[0] != result[1] || flagsAfter[0] != flagsAfter[1])
					++perMode[mode];
				if(mode == 0)
					{
					if(result[0])
						++trueCount;
					else
						++falseCount;
					}
				}
			}
		totalMismatch += perMode[0];
		printf("collision name=%s index=%u rva=0x%08x owner=%s checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
			entries[e].name, index, nxMatrixB[index].rva, nxMatrixB[index].stableId,
			oracleDigest.checks, oracleDigest.state, candidateDigest.state,
			perMode[0] + perMode[1]);
		printf("collision coverage name=%s true=%u false=%u refreshed=%u inverted=%u unflagged=%u default_mismatches=%u simulate_mismatches=%u\n",
			entries[e].name, trueCount, falseCount, refreshed, inverted, unflagged,
			perMode[0], perMode[1]);
		nxPrintInput(entries[e].name, &inputDigest);
		}
	}

	// convex-mesh gap Task 2b's families (units/convex-mesh-gap-contract.md,
	// sub-units E, F, I and N), in a function of their own; see nxDriveTask2b.
	totalMismatch += nxDriveTask2b(base);

	// The kernel fuzz harness's signalling-NaN variants; see nxDriveFuzzSnan.
	totalMismatch += nxDriveFuzzSnan(physics);

	// What is not covered, named rather than left as an absence.
	for(unsigned index = 0; index < 36; ++index)
		{
		bool driven = false;
		for(unsigned entry = 0; entry < kDrivenCount; ++entry)
			if(nxDriven[entry].index == index)
				driven = true;
		for(unsigned entry = 0; entry < sizeof(nxDrivenOverlapOwnBlock) / sizeof(nxDrivenOverlapOwnBlock[0]); ++entry)
			if(nxDrivenOverlapOwnBlock[entry] == index)
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
