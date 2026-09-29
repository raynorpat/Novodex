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

// A draw in [0, 1), exact as a float (24 bits of the state times 2^-24).
//
// Returned in a four-byte struct, never as a float (convex-mesh gap Task 2g
// review). Under the x86 ABI a float return travels in st(0), and where this
// helper was not inlined the caller carried on in x87 at the control word's 53
// bits -- `u * 1.5f + 0.05f` rounded once -- where an inlined copy ran in SSE
// and rounded after each operation. Which sites were inlined followed the size
// of the translation unit, so moving code elsewhere in this file moved
// registered digests. A four-byte struct comes back in eax whether or not the
// call is inlined, so every site now computes in SSE float, as the inlined
// sites always did; the two sites that were compiled to x87 (nxPickWordFrom's
// `* 8 - 4` and nxFillGeometry's capsule radius) write the double arithmetic
// they were compiled to, so their registered values are unchanged.
struct NxUnitDraw
	{
	float value;
	};

static NxUnitDraw nxUnit(unsigned* state)
	{
	NxUnitDraw draw;
	draw.value = (float) (nxNext(state) >> 8) * (1.0f / 16777216.0f);
	return draw;
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
		const float value = nxUnit(state).value * 1e-6f;
		memcpy(&bits, &value, 4);
		}
	else if(choice == 3)
		// Sign from the low bit, payload from the rest; a zero payload is an
		// infinity and anything else is a NaN, so both arrive without either
		// being spelled out.
		bits = (nxNext(state) & 0x807fffffu) | 0x7f800000u;
	else
		{
		// Compiled to x87 until the Task 2g review (the draw came back in st(0)):
		// the double arithmetic it ran is written out. Exact either way here.
		const float value = (float) ((double) nxUnit(state).value * 8.0 - 4.0);
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
	{ "contact_box_box", { 346, 346 }, { 78, 78 }, { 236, 236 } },
	{ "step_smooth_normals", { 15, 15 }, { 0, 0 }, { 0, 0 } },
	{ "contact_emit", { 3738, 3738 }, { 0, 0 }, { 0, 0 } },
	{ "shape_raycast_plane", { 1720, 1720 }, { 0, 0 }, { 0, 0 } },
	{ "contact_plane_capsule", { 470, 470 }, { 0, 0 }, { 0, 0 } },
	{ "shape_raycast_sphere", { 2171, 2171 }, { 0, 0 }, { 0, 0 } },
	{ "contact_sphere_capsule", { 804, 804 }, { 0, 0 }, { 0, 0 } },
	{ "sphere_box_contact", { 3181, 3181 }, { 0, 0 }, { 0, 0 } },
	{ "contact_sphere_box", { 2022, 2022 }, { 0, 0 }, { 0, 0 } },
	{ "contact_box_capsule", { 99, 93 }, { 7, 7 }, { 40, 40 } },
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
		q[i] = nxUnit(state).value * 2.0f - 1.0f;
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
				n[i] = nxUnit(state).value * 2.0f - 1.0f;
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
			shape->geometry[3] = nxUnit(state).value * 4.0f - 2.0f;
		else
			nxPickWord(state, &shape->geometry[3]);
		}
	else if(type == 1)
		{
		if(tame)
			shape->geometry[0] = nxUnit(state).value * 2.0f + 0.05f;
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
				shape->geometry[i] = nxUnit(state).value * 2.0f + 0.05f;
			else
				nxPickWord(state, &shape->geometry[i]);
		}
	else
		{
		if(tame)
			// x87 until the Task 2g review, rounded once: written as the double
			// arithmetic it was compiled to, so the registered draws are unchanged.
			shape->geometry[0] = (float) ((double) nxUnit(state).value * 1.5 + (double) 0.05f);
		else
			nxPickWord(state, &shape->geometry[0]);
		if(tame)
			shape->geometry[1] = nxUnit(state).value * 2.0f + 0.05f;
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
//
// Written through a pointer, as a double (convex-mesh gap Task 2g review): it
// used to return a float, which came back in st(0) unrounded -- its body was
// x87 at the control word's 53 bits -- and the aimed block's sum of two reaches
// was added there too. The double arithmetic below is what that code computed
// (every input is a tame, finite geometry word), so the aimed draws, and the
// registered digests over them, are unchanged; and no longer depend on
// whether the compiler inlines it.
static void nxReach(const NxCollisionShape* shape, double* out)
	{
	if(shape->type == 1)
		*out = shape->geometry[0];
	// The mean extent, not the sum: the sum aims at a separation the two boxes
	// can essentially never span, and an aimed block that never overlaps is
	// the same as no aimed block.
	else if(shape->type == 2)
		*out = (((double) shape->geometry[2] + shape->geometry[1]) + shape->geometry[3])
			* (double) (1.0f / 3.0f);
	else if(shape->type == 3)
		*out = (double) shape->geometry[1] + shape->geometry[0];
	else
		*out = 0.0;
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
// Written through a pointer (Task 2g review), not returned in st(0).
static void nxCapsulePseudoExtent(NxReal radius, NxReal* out)
	{
	static const float scale = 0.666f;
	NxReal product;
	__asm
		{
		fld  radius
		fmul scale
		fstp product
		}
	*out = product;
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
			box->center[k] = nxUnit(state).value * 4.0f - 2.0f;
		else
			nxPickWord(state, &box->center[k]);
		if(aimed)
			box->extents[k] = nxUnit(state).value * 2.0f + 0.05f;
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
		u[k] = nxUnit(state).value * 2.0f - 1.0f;
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
void nxCandidateMatrixInvert(float* m);

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
			v[i][k] = nxUnit(state).value * 4.0f - 2.0f;
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
	float u = nxUnit(state).value * 3.0f - 1.0f;
	float w = nxUnit(state).value * 3.0f - 1.0f;
	// Sequenced: as one expression the draws' order was the compiler's.
	const bool heightFull = (nxNext(state) & 1) != 0;
	const float heightDraw = nxUnit(state).value;
	float h = (heightDraw * 2.0f - 1.0f) * (heightFull ? 1.0f : 0.05f);
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
				o0[k] = nxUnit(&state).value * 4.0f - 2.0f;
				d0[k] = nxUnit(&state).value * 4.0f - 2.0f;
				o1[k] = nxUnit(&state).value * 4.0f - 2.0f;
				d1[k] = nxUnit(&state).value * 4.0f - 2.0f;
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
				const float s = nxUnit(&state).value * 1.4f - 0.2f;
				const float t = nxUnit(&state).value * 1.4f - 0.2f;
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
				const float u = nxUnit(&state).value * 0.5f;
				const float w = nxUnit(&state).value * 0.5f;
				float inner[3];
				for(int k = 0; k < 3; ++k)
					inner[k] = v[0][k] + u * (v[1][k] - v[0][k]) + w * (v[2][k] - v[0][k]);
				const float up = nxUnit(&state).value + 0.1f;
				const float down = nxUnit(&state).value + 0.1f;
				float tilt[3];
				for(int k = 0; k < 3; ++k)
					tilt[k] = (nxUnit(&state).value - 0.5f) * 0.5f;
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
				words[k] = nxUnit(&reviewState).value * 8.0f - 4.0f;
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
					basis[r][k] = nxUnit(&state).value * 2.0f - 1.0f;
			float centre[3];
			for(int k = 0; k < 3; ++k)
				centre[k] = nxUnit(&state).value * 4.0f - 2.0f;
			const float radius = nxUnit(&state).value * 2.0f + 0.1f;
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
			const float reach = nxUnit(&state).value * 1.3f;
			const float angle = nxUnit(&state).value * 6.2831853f;
			float target[3];
			for(int k = 0; k < 3; ++k)
				target[k] = centre[k] + reach * radius * (cosf(angle) * basis[0][k] + sinf(angle) * basis[1][k]);
			const float lift = (nxNext(&state) & 7) == 0 ? 0.0f : (nxUnit(&state).value * 6.0f - 3.0f);
			if(lift == 0.0f)
				++inPlane;
			const float origin[3] = {
				target[0] + lift * basis[2][0] + (nxUnit(&state).value - 0.5f) * basis[0][0],
				target[1] + lift * basis[2][1] + (nxUnit(&state).value - 0.5f) * basis[0][1],
				target[2] + lift * basis[2][2] + (nxUnit(&state).value - 0.5f) * basis[0][2] };
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
	printf("collision divergent name=ray_inflated_tris cause=phys_fn_005185 fans=%u fans_simulate=%u default_mismatches=%u simulate_mismatches=%u\n",
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
				const float c = nxUnit(&state).value * 4.0f - 2.0f;
				const float e = nxUnit(&state).value * 2.0f;
				boxMin[k] = c - e;
				boxMax[k] = c + e;
				origin[k] = c + (nxUnit(&state).value * 2.0f - 1.0f) * e * 2.0f;
				const unsigned kind = nxNext(&state) % 8;
				if(kind == 0)
					dir[k] = (nxNext(&state) & 1) ? -0.0f : 0.0f;
				else if(kind == 1)
					dir[k] = (nxUnit(&state).value * 2.0f - 1.0f) * 1.1920928e-7f * 0.99f;
				else if(kind == 2)
					dir[k] = (nxNext(&state) & 1) ? -1.1920928955078125e-7f : 1.1920928955078125e-7f;
				else
					dir[k] = nxUnit(&state).value * 4.0f - 2.0f;
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
			const float along = nxUnit(&state).value * 2.0f - 0.5f;
			// Sequenced: as one expression the draws' order was the compiler's.
			const bool upFull = (nxNext(&state) & 1) != 0;
			const float upDraw = nxUnit(&state).value;
			const float up = (upDraw * 2.0f - 1.0f) * (upFull ? 1.0f : 0.1f);
			float p[3];
			for(int k = 0; k < 3; ++k)
				p[k] = v[a][k] + along * edge[k] + up * axis[k];
			float d[3];
			for(int k = 0; k < 3; ++k)
				d[k] = nxUnit(&state).value * 2.0f - 1.0f;
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
					d[k] += across[k] * (nxUnit(&state).value * 4.0f - 2.0f);
			const float l0 = nxUnit(&state).value * 1.5f + 0.05f;
			const float l1 = nxUnit(&state).value * 1.5f + 0.05f;
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

// convex-mesh gap Task 2g's families, defined at the end of the file (see the
// block).
static unsigned nxDriveTask2g(unsigned char* base);

// convex-mesh gap Task 2h's families (sub-unit M's first half), defined after
// Task 2g's, whose fixture they reuse.
static unsigned nxDriveTask2h(unsigned char* base);

// convex-mesh gap Task 2i's families (sub-unit M's second half and 002081),
// defined after Task 2h's, whose helpers they reuse.
static unsigned nxDriveTask2i(unsigned char* base);
static unsigned nxDriveTask2j(unsigned char* base);
static unsigned nxDriveTask2k(unsigned char* base);
static unsigned nxDriveTask2lAccumulator(unsigned char* base);
struct Nx2iSide;
static unsigned nxDriveTask2lEdgeNormal(unsigned char* base, Nx2iSide* sides);

// Candidate box/mesh entries implemented from the listing in ContactBoxMeshICE.cpp.
void __cdecl NxContactBoxMesh(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*);
bool __cdecl NxOverlapBoxMesh(const NxCollisionShape*, const NxCollisionShape*, void*);
void __cdecl NxContactCapsuleMesh(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*);
extern "C" bool __cdecl nxOverlapMeshMesh(const NxCollisionShape*, const NxCollisionShape*, void*);
bool __cdecl NxOverlapCapsuleMesh(const NxCollisionShape*, const NxCollisionShape*, void*);

int wmain(int argc, wchar_t** argv)
	{
	const bool task2jOnly = argc == 4 && wcscmp(argv[3], L"--task2j-only") == 0;
	const bool task2kOnly = argc == 4 && wcscmp(argv[3], L"--task2k-only") == 0;
	if(argc != 3 && !task2jOnly && !task2kOnly)
		{
		fprintf(stderr, "usage: NxPhysicsCollisionTests <oracle directory> <NxPhysics.dll sha256> [--task2j-only]\n");
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
	if(task2jOnly)
		{
		const unsigned mismatches = nxDriveTask2j(base);
		printf("collision matrix_wrong=0 index_wrong=0 mismatches=%u\n", mismatches);
		printf("collision=%s\n", mismatches ? "fail" : "pass");
		return mismatches ? 1 : 0;
		}
	if(task2kOnly)
		{
		const unsigned mismatches = nxDriveTask2k(base);
		printf("collision capsule_mesh mismatches=%u\n", mismatches);
		return mismatches ? 1 : 0;
		}

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
					shape0->translation[k] = nxUnit(&state).value * 6.0f - 3.0f;
				else
					nxPickWord(&state, &shape0->translation[k]);
				if(tame)
					shape1->translation[k] = nxUnit(&state).value * 6.0f - 3.0f;
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
				shape0->translation[k] = nxUnit(&state).value * 2.0f - 1.0f;
			nxFillGeometry(&state, shape0, type0, true);
			nxFillGeometry(&state, shape1, type1, true);

			float direction[3];
			float length = 0.0f;
			for(int k = 0; k < 3; ++k)
				{
				direction[k] = nxUnit(&state).value * 2.0f - 1.0f;
				length += direction[k] * direction[k];
				}
			length = (float) sqrt((double) length);
			if(length < 1e-4f)
				{ direction[0] = 1.0f; direction[1] = 0.0f; direction[2] = 0.0f; length = 1.0f; }
			for(int k = 0; k < 3; ++k)
				direction[k] /= length;

			// shape1's reach was stored narrow before shape0's, still wide in
			// st(0), was added to it (x87, 53 bits) and the sum narrowed.
			double reach0, reach1;
			nxReach(shape1, &reach1);
			nxReach(shape0, &reach0);
			float reach = (float) (reach0 + (double) (float) reach1);
			if(type0 == 0)
				{
				// Against a plane the interesting family is a shape straddling
				// the plane, so aim along the normal from a point on it.
				const float* n = shape0->geometry;
				const float unit = nxUnit(&state).value;
				double reachWide;
				nxReach(shape1, &reachWide);
				const float reachNarrow = (float) reachWide;
				float offset = (unit * 2.0f - 1.0f) * (reachNarrow * 1.5f + 0.25f);
				for(int k = 0; k < 3; ++k)
					shape1->translation[k] = n[k] * (offset - shape0->geometry[3]);
				}
			else
				{
				float separation = reach * (0.2f + nxUnit(&state).value * 1.4f);
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
				shape0->translation[k] = nxUnit(&state).value * 6.0f - 3.0f;
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
				box.center[k] = nxUnit(&state).value * 2.0f - 1.0f;
			else
				nxPickWord(&state, &box.center[k]);
			box.extents[k] = shape1->geometry[k + 1];
			}
		for(int k = 0; k < 9; ++k)
			box.rotation[k] = shape1->rotation[k];
		if(tame)
			sphere.radius = nxUnit(&state).value * 1.5f + 0.02f;
		else
			nxPickWord(&state, &sphere.radius);

		// Half the run puts the centre inside the box, which is the one path
		// that returns before the closest point is ever transformed back.
		bool inside = (nxNext(&state) & 1) != 0;
		if(inside && tame)
			{
			++insideCount;
			for(int k = 0; k < 3; ++k)
				sphere.center[k] = box.center[k] + (nxUnit(&state).value * 2.0f - 1.0f) * box.extents[k] * 0.5f;
			}
		else
			{
			for(int k = 0; k < 3; ++k)
				if(tame)
					sphere.center[k] = nxUnit(&state).value * 4.0f - 2.0f;
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
			const NxReal y0 = nxUnit(&state).value * 4.0f - 2.0f;
			const NxReal z0 = nxUnit(&state).value * 4.0f - 2.0f;
			const NxReal y1 = y0 + nxUnit(&state).value * 3.0f + 0.05f;
			const NxReal z1 = z0 + nxUnit(&state).value * 3.0f + 0.05f;
			const bool accepted = (nxNext(&state) & 1) != 0;
			// quad[0] (y0,z0), quad[1] (y0,z1), quad[2] (y1,z1), quad[3] (y1,z0)
			// is the winding whose four cross products are all negative for an
			// interior point; the reverse of it is rejected at index 0.
			const NxReal ys[4] = { y0, y0, y1, y1 };
			const NxReal zs[4] = { z0, z1, z1, z0 };
			for(int k = 0; k < 4; ++k)
				{
				const int slot = accepted ? k : (3 - k);
				corner[slot].x = nxUnit(&state).value * 4.0f - 2.0f;
				corner[slot].y = ys[k];
				corner[slot].z = zs[k];
				}
			if(!accepted)
				++reversed;

			if((nxNext(&state) & 1) != 0)
				{
				pointY = y0 + nxUnit(&state).value * (y1 - y0);
				pointZ = z0 + nxUnit(&state).value * (z1 - z0);
				if(accepted)
					++aimedInside;
				}
			else
				{
				// Outside on a random side, which is what walks the loop to a
				// different index before it leaves.
				pointY = nxUnit(&state).value * 8.0f - 4.0f;
				pointZ = nxUnit(&state).value * 8.0f - 4.0f;
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
					extentA[k] = nxUnit(&state).value * 1.5f + 0.25f;

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
					extentB[k] = extentA[k] * scale * (nxUnit(&state).value * 0.6f + 0.7f);

				unsigned char rotationStore[kShapeBytes];
				NxCollisionShape* const rotation = (NxCollisionShape*) rotationStore;
				nxIdentity(rotation);
				if(aimedMode == 1 || aimedMode == 3)
					nxRandomRotation(&state, rotation);
				else
					{
					// A small tilt, so the face pair is nearly parallel and the
					// corners really do land inside.
					const float t = (nxUnit(&state).value * 2.0f - 1.0f) * 0.15f;
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
					? (nxUnit(&state).value * 6.0f - 3.0f)
					: (nxUnit(&state).value * 0.8f - 0.1f) * extentA[0];
				const float spread = (aimedMode == 3) ? 3.0f : 1.2f;
				poseB[9] = depth;
				poseB[10] = (nxUnit(&state).value * 2.0f - 1.0f) * extentA[1] * spread;
				poseB[11] = (nxUnit(&state).value * 2.0f - 1.0f) * extentA[2] * spread;
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
								const float t = (nxUnit(&state).value * 2.0f - 1.0f) * 0.05f;
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
							extent[p][boxIndex][k] = nxUnit(&state).value * 1.4f + 0.3f;
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
						pose[p][0][9 + k] = nxUnit(&state).value * 4.0f - 2.0f;
						pose[p][1][9 + k] = pose[p][0][9 + k]
							+ (nxUnit(&state).value * 2.0f - 1.0f) * reach;
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
						extent[p][boxIndex][k] = nxUnit(&state).value * 1.4f + 0.3f;
						pose[p][boxIndex][9 + k] = boxIndex == 0
							? nxUnit(&state).value * 2.0f - 1.0f
							: pose[p][0][9 + k] + (nxUnit(&state).value * 2.0f - 1.0f)
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
						box0Shape->translation[k] = nxUnit(&local).value * 2.0f - 1.0f;
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
							+ (nxUnit(&local).value * 2.0f - 1.0f) * spread
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
				nxSnanDiscrete(mode, world[0].sink.separatingAxis, world[1].sink.separatingAxis);
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
					(&v[k].x)[c] = nxUnit(&state).value * 4.0f - 2.0f;
				else
					nxPickRawWord(&state, &(&v[k].x)[c]);
		// Half of the tame iterations aim the ray through a random barycentric
		// point of the triangle, so the tail past both range tests is reached
		// rather than left to chance -- the same correction the fuzz harness
		// needed for this export.
		if(tame && (nxNext(&state) & 1))
			{
			float a = nxUnit(&state).value, b = nxUnit(&state).value * (1.0f - a);
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
					(&verts[vertex].x)[c] = nxUnit(&state).value * 4.0f - 2.0f;
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
						sphereShape->translation[k] = nxUnit(&local).value * 3.0f - 1.5f;
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
						(&point.x)[k] = nxUnit(&local).value * 4.0f - 2.0f;
					else
						nxPickWord(&local, &(&point.x)[k]);
					if(tame)
						(&normal.x)[k] = nxUnit(&local).value * 2.0f - 1.0f;
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
				(&ray.orig.x)[k] = nxUnit(&state).value * 4.0f - 2.0f;
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
				tangent[k] = nxUnit(&state).value * 6.0f - 3.0f;
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
			maxDistance = length * (0.2f + nxUnit(&state).value * 1.6f);
			}
		else
			{
			for(int k = 0; k < 3; ++k)
				if(tame)
					(&ray.dir.x)[k] = nxUnit(&state).value * 2.0f - 1.0f;
				else
					nxPickWord(&state, &(&ray.dir.x)[k]);
			if(tame)
				maxDistance = nxUnit(&state).value * 6.0f;
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
						pick[k] = nxUnit(&local).value * 2.0f - 1.0f;
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
					const float offset = (nxUnit(&local).value * 2.0f - 1.0f) * (span * 1.5f + 0.25f);
					for(int k = 0; k < 3; ++k)
						capsuleShape->translation[k] =
							planeNormal[k] * (offset - planeShape->geometry[3]);
					}
				else
					for(int k = 0; k < 3; ++k)
						if(tame)
							capsuleShape->translation[k] = nxUnit(&local).value * 3.0f - 1.5f;
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
				shape0->translation[k] = nxUnit(&state).value * 4.0f - 2.0f;
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
				direction[k] = nxUnit(&state).value * 2.0f - 1.0f;
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
			const float along = (nxUnit(&state).value * 3.0f + 0.5f) * (behind ? -1.0f : 1.0f);
			// An offset across the ray, so the sphere is grazed as well as hit
			// through the middle and missed outright.
			const float across = (nxUnit(&state).value * 2.0f - 1.0f) * shape0->geometry[0] * 1.4f;
			for(int k = 0; k < 3; ++k)
				(&ray.orig.x)[k] = shape0->translation[k] - (&ray.dir.x)[k] * along
					+ ((k + 1) % 3 == 0 ? across : -across) * 0.5f;
			maxDistance = (along < 0.0f ? -along : along) * (0.2f + nxUnit(&state).value * 1.6f);
			}
		else
			{
			for(int k = 0; k < 3; ++k)
				{
				if(tame)
					(&ray.orig.x)[k] = nxUnit(&state).value * 4.0f - 2.0f;
				else
					nxPickWord(&state, &(&ray.orig.x)[k]);
				if(tame)
					(&ray.dir.x)[k] = nxUnit(&state).value * 2.0f - 1.0f;
				else
					nxPickWord(&state, &(&ray.dir.x)[k]);
				}
			if(tame)
				maxDistance = nxUnit(&state).value * 6.0f;
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
						capsuleShape->translation[k] = nxUnit(&local).value * 3.0f - 1.5f;
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
					capsuleShape->rotation[1] = nxUnit(&local).value * 4.0f - 2.0f;
					capsuleShape->rotation[4] = nxUnit(&local).value * 4.0f - 2.0f;
					capsuleShape->rotation[7] = nxUnit(&local).value * 4.0f - 2.0f;
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
						const float along = nxUnit(&local).value * 3.0f - 1.5f;
						const bool onAxis = (nxNext(&local) & 7) == 0;
						const float span = sphereShape->geometry[0] + capsuleShape->geometry[0];
						const float across = onAxis ? 0.0f
							: (nxUnit(&local).value * 1.8f + 0.05f) * span;
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
							sphereShape->translation[k] = nxUnit(&local).value * 3.0f - 1.5f;
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
						const float offset = sink6 ? -(reach + nxUnit(&local).value * 2.0f)
							: (nxUnit(&local).value * 2.4f - 1.2f) * reach;
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
				shape0->translation[k] = nxUnit(&state).value * 4.0f - 2.0f;
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
				direction[k] = nxUnit(&state).value * 2.0f - 1.0f;
				length += direction[k] * direction[k];
				}
			length = (float) sqrt((double) length);
			if(length < 1e-4f)
				{ direction[0] = 1.0f; direction[1] = 0.0f; direction[2] = 0.0f; length = 1.0f; }
			for(int k = 0; k < 3; ++k)
				(&ray.dir.x)[k] = direction[k] / length;
			const float along = nxUnit(&state).value * 4.0f + 0.5f;
			const float across = (nxUnit(&state).value * 2.0f - 1.0f) * shape0->geometry[0] * 1.4f;
			for(int k = 0; k < 3; ++k)
				(&ray.orig.x)[k] = shape0->translation[k] - (&ray.dir.x)[k] * along
					+ ((k + 1) % 3 == 0 ? across : -across) * 0.5f;
			maxDistance = along * (0.2f + nxUnit(&state).value * 1.6f);
			}
		else
			{
			for(int k = 0; k < 3; ++k)
				{
				if(tame)
					(&ray.orig.x)[k] = nxUnit(&state).value * 4.0f - 2.0f;
				else
					nxPickWord(&state, &(&ray.orig.x)[k]);
				if(tame)
					(&ray.dir.x)[k] = nxUnit(&state).value * 2.0f - 1.0f;
				else
					nxPickWord(&state, &(&ray.dir.x)[k]);
				}
			if(tame)
				maxDistance = nxUnit(&state).value * 8.0f;
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
						first->translation[k] = nxUnit(&local).value * 3.0f - 1.5f;
					else
						nxPickWord(&local, &first->translation[k]);

				const unsigned axisMode = nxNext(&local) % 4;
				if(axisMode == 0)
					nxRandomRotation(&local, first);
				else if(axisMode == 2)
					{
					first->rotation[1] = nxUnit(&local).value * 4.0f - 2.0f;
					first->rotation[4] = nxUnit(&local).value * 4.0f - 2.0f;
					first->rotation[7] = nxUnit(&local).value * 4.0f - 2.0f;
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
						const float wobble = nxUnit(&local).value * 0.05f;
						for(int k = 0; k < 3; ++k)
							second->rotation[1 + k * 3] =
								axis[k] * sign + (nxUnit(&local).value * 2.0f - 1.0f) * wobble;
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
							: matched ? nxUnit(&local).value * 0.7f - 0.35f
							: nxUnit(&local).value * 3.2f - 1.6f;
						const bool onAxis = (nxNext(&local) & 7) == 0;
						const float span = first->geometry[0] + second->geometry[0];
						const float across = onAxis ? 0.0f
							: (matched ? nxUnit(&local).value * 0.9f
								: nxUnit(&local).value * 1.9f + 0.02f) * span;
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
							second->translation[k] = nxUnit(&local).value * 3.0f - 1.5f;
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
				words0[k] = nxUnit(&state).value * 6.0f - 3.0f;
			else
				nxPickWord(&state, &words0[k]);
			if(tame)
				words1[k] = nxUnit(&state).value * 6.0f - 3.0f;
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
			// Two draws, sequenced in the order the build had evaluated the
			// one expression they used to share: the sign word, then the
			// magnitude (every registered segment_segment line reproduces).
			const unsigned signWord = nxNext(&state);
			const float magnitude = nxUnit(&state).value * 1.8f + 0.2f;
			const float scale = magnitude * ((signWord & 1) ? 1.0f : -1.0f);
			const float jitter = nxUnit(&state).value * 4e-3f;
			for(int k = 0; k < 3; ++k)
				{
				segment[1].p0[k] = segment[0].p0[k] + (nxUnit(&state).value * 2.0f - 1.0f);
				segment[1].p1[k] = segment[1].p0[k] + direction[k] * scale
					+ (nxUnit(&state).value * 2.0f - 1.0f) * jitter;
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
			const float along = nxUnit(&state).value;
			float target[3];
			for(int k = 0; k < 3; ++k)
				target[k] = segment[0].p0[k] + direction[k] * along;
			float across[3];
			across[0] = direction[1];
			across[1] = -direction[0];
			across[2] = direction[2] * 0.5f + 0.25f;
			const float offset = nxUnit(&state).value * 2.0f - 1.0f;
			for(int k = 0; k < 3; ++k)
				{
				const float half = (nxUnit(&state).value * 2.0f - 1.0f) * 1.5f;
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
							firstShape->translation[k] = nxUnit(&local).value * 4.0f - 2.0f;
						else
							nxPickWord(&local, &firstShape->translation[k]);

					// The placement is along a direction the generator builds
					// itself, so nothing multiplies a value it also allows to be
					// non-finite: the direction is always three finite units.
					float direction[3];
					float length = 0.0f;
					for(int k = 0; k < 3; ++k)
						{
						direction[k] = nxUnit(&local).value * 2.0f - 1.0f;
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
							? nxUnit(&local).value * 1e-2f
							: reach * (nxUnit(&local).value * 1.4f + 0.2f);
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
				box.center[k] = nxUnit(&state).value * 2.0f - 1.0f;
			else
				nxPickWord(&state, &box.center[k]);
			box.extents[k] = boxShape->geometry[k + 1];
			}
		for(int k = 0; k < 9; ++k)
			box.rotation[k] = boxShape->rotation[k];
		if(tame)
			sphere.radius = nxUnit(&state).value * 1.5f + 0.02f;
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
				localOffset[k] = (nxUnit(&state).value * 2.0f - 1.0f) * box.extents[k] * 0.9f;
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
					sphere.center[k] = nxUnit(&state).value * 4.0f - 2.0f;
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
							boxShape->translation[k] = nxUnit(&local).value * 2.0f - 1.0f;
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
							localOffset[k] = (nxUnit(&local).value * 2.0f - 1.0f)
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
				direction[k] = (nxUnit(&state).value * 2.0f - 1.0f) * 3.0f;
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
					v[k] = (nxUnit(&state).value * 2.0f - 1.0f) * 0.9f;
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
						box->translation[k] = nxUnit(&local).value * 2.0f - 1.0f;
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
						const float push = 1.0f + (nxUnit(&local).value * 1.5f - 0.5f)
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
				nxCapsulePseudoExtent(world[0].sphere->geometry[0], &pseudoExtents[0]);
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
			nxSnanDiscrete(mode, world[0].sink.separatingAxis, world[1].sink.separatingAxis);
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
						const float c = nxUnit(&state).value * 4.0f - 2.0f;
						const float h = nxUnit(&state).value * 1.5f + 0.02f;
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
					primitive->translation[k] = lo + span * (nxUnit(&state).value * 2.4f - 0.7f);
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

	// convex-mesh gap Task 2g's families (P-Emit, P-Plane, P-Mesh and sub-unit L),
	// in a function of their own; see nxDriveTask2g.
	totalMismatch += nxDriveTask2g(base);

	// convex-mesh gap Task 2h's families (sub-unit M's first half, the
	// convex/mesh separating axes and contacts); see nxDriveTask2h.
	totalMismatch += nxDriveTask2h(base);

	// convex-mesh gap Task 2i's families (sub-unit M's second half: the convex /
	// height-field and convex / triangle-mesh entries, and 002081); see nxDriveTask2i.
	totalMismatch += nxDriveTask2i(base);

	// convex-mesh gap Task 2j's box/mesh entries; see nxDriveTask2j.
	totalMismatch += nxDriveTask2j(base);

	// convex-mesh gap Task 2k's capsule/mesh entries; see nxDriveTask2k.
	totalMismatch += nxDriveTask2k(base);
	// Task 2l's callback has a direct accumulator differential before the mesh
	// pair entry is added.
	totalMismatch += nxDriveTask2lAccumulator(base);

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

// ---------------------------------------------------------------------------
// convex-mesh gap Task 2g (units/convex-mesh-gap-contract.md: P-Emit, P-Plane,
// P-Mesh and sub-unit L). Two families, both on raw words written as bits:
//
// contact_emit_ext drives phys_fn_000875 (the emitter with feature words) at its
// own address, as contact_emit drives 000873: shapes staged by nxStageWorld with
// the flag byte +0xde drawn (bit 0x20 turns on the feature-word flag 4), 16-bit
// ids drawn real or 0xffff with junk in the high halves of their slots, 32-bit
// feature words drawn at most or above 0xffff, and points and normals drawn by
// nxPickRawWord (signalling NaNs kept: the row is naked and loads the normal
// with `fld`, which quiets them on both sides), one normal in four the previous
// one again (the cached-normal skip). The stream never fills (0x2000 words), so
// 004840's growth is not reached here.
//
// contact_convex_convex drives phys_fn_001820 (and through it 001818, 001816,
// 001809, 001807, 001805, 001803, 001812/001814, 001810, 001909/001911,
// 001903/001905, 001907, 000875, 001653, 001661 and the polygon interface slots
// 0, 2, 3, 4, 9, 10, 11 with 001496, 001516, 002249, 001530, 000505, 001556) over
// pairs of box hulls. Each side builds its own images: a TriangleMesh image
// (+0x04 the side's polygon table -- the oracle's 0x101085d4 or
// gTriangleMeshPolygonTable -- +0xa0 the hull, +0xa8 a kind C support map built
// by the side's own constructor and Init, or null), a hull image of a closed box
// on lattice words (kLattice2g: every face axis-aligned, so the vendored
// Plane::Set / Triangle::Normal in 001472's build are exact) whose polygons and
// edges the side's own 001472 and 001502 build through slots 3 and 6 before the
// pairs (under 0x027f), a centre drawn from the lattice words and the vertex
// graph at +0x64 (the box's triangle edges; 001530 climbs it when the map is
// null), and a scratch record (the context: +0x04 count, +0x08 visited, +0x14
// stamp, +0x4e0 / +0x4f0 two Containers built by the side's own constructor).
// The shapes are staged by nxStageWorld; their pruning handles (+0xa4 Prunable
// flags with bit 2 set, so 004886 is not called; +0xc4 a pruner whose +0x14
// holds the world boxes; +0xcc the handle) carry drawn box words.
//
// Fixed-input rules (the inputs that would make the oracle read memory it does
// not own are not driven): a pose word is never a NaN or an infinity. With a NaN
// axis every comparison in 001809's face pass fails, its best index stays -1
// (0x0003ffe2), and 001816 hands that index to slot 4 (0x000407cd), which reads
// the polygon before the array; an infinity reaches the same through inf - inf.
// The handle is never 0xffff (001818 then passes a null box, and 001812's box test
// 001810 reads through it). Pose words are drawn from signed permutations,
// rotations about an axis by fixed (cos, sin) words, and tables of finite words
// with denormals and -0; translations and boxes from lattice and drawn finite
// words.

// Where this block stands no longer matters. When it was first written before
// wmain, thirteen registered lines moved (the nine `.random` input digests,
// box_corner's and box_corner.snan's oracle and input digests). The cause was
// not the ICE headers (their `#pragma inline_depth(255)` repeats Nx.h's): nxUnit
// and nxReach returned floats through st(0), and adding code flipped whether
// they were inlined, so a site's arithmetic ran in x87 at 53 bits or in SSE
// (see nxUnit). They no longer return floats; with this block moved back before
// wmain in a throwaway build every line reproduced (evidence/convex-mesh-gap.md,
// Task 2g review). It stays at the end of the file.
#pragma push_macro("min")
#pragma push_macro("max")
#pragma push_macro("random")
#undef min
#undef max
#include "TriangleMeshPolygons.h"
#include "EdgeList.h"
#pragma pop_macro("random")
#pragma pop_macro("max")
#pragma pop_macro("min")

static const unsigned kEmitFeaturesRva = 0x0001d8e0;		// phys_fn_000875
static const unsigned kConvexConvexRva = 0x000411a0;		// phys_fn_001820
static const unsigned kPolygonTableRva = 0x001085d4;		// the TriangleMesh +0x04 table
static const unsigned kVertexMapCtorRva = 0x0002e7c0;		// phys_fn_001575
static const unsigned kSupportMapInitRva = 0x0002e2f0;		// phys_fn_001558
static const unsigned kIceContainerCtorRva = 0x000b4d70;	// phys_fn_004836
static const unsigned kIceContainerDtorRva = 0x000b4f50;	// phys_fn_004846
static const unsigned kIceContainerSetSizeRva = 0x000b4e90;	// Container::SetSize
static const unsigned kSdkAllocatorGetterRva = 0x000b4000;	// phys_fn_004803
static const unsigned kEmitExtIterations = 8000;
static const unsigned kConvexPairs = 10000;
// contact_convex_convex.pose_divergent's ceiling: the differing words and runs
// measured when the split was registered (a count may fall, never rise).
static const unsigned kConvexPoseDivergentWords = 2;
static const unsigned kConvexPoseDivergentRuns = 1;
// contact_convex_hulls (the polytopes, Task 2g review): its pairs and its split's ceiling.
static const unsigned kConvexHullPairs = 6000;
static const unsigned kConvexHullsPoseDivergentWords = 14;
static const unsigned kConvexHullsPoseDivergentRuns = 4;

// ecx the object, edx cleared: the oracle's thiscall rows and the candidate's
// __fastcall forms alike.
static unsigned nx2gCall0(const void* fn, void* self)
	{
	unsigned r;
	__asm
		{
		mov		ecx, self
		xor		edx, edx
		call	fn
		mov		r, eax
		}
	return r;
	}
static unsigned nx2gCall1(const void* fn, void* self, unsigned a0)
	{
	unsigned r;
	__asm
		{
		push	a0
		mov		ecx, self
		xor		edx, edx
		call	fn
		mov		r, eax
		}
	return r;
	}

// The lattice words (convex-mesh gap Task 2f's kChLattice): each row increases.
static const unsigned kLattice2g[8][5] =
	{
	{ 0x00000000u, 0x3f800000u, 0x40000000u, 0x40400000u, 0x40800000u },
	{ 0xbfc00000u, 0xbf000000u, 0x3e800000u, 0x3fe00000u, 0x41200000u },
	{ 0x42c00000u, 0x43000000u, 0x43200000u, 0x43600000u, 0x43800000u },
	{ 0x3e000000u, 0x3ec00000u, 0x3f000000u, 0x3f200000u, 0x40600000u },
	{ 0xc0800000u, 0xc0400000u, 0xc0000000u, 0xbf800000u, 0x80000000u },
	{ 0x80000000u, 0x3f000000u, 0x3fc00000u, 0x40200000u, 0x40c00000u },
	{ 0xc1000000u, 0xc0c00000u, 0xc0000000u, 0xbf800000u, 0xbf400000u },
	{ 0x45000000u, 0x45400000u, 0x45800000u, 0x45c00000u, 0x46000000u },
	};

// Finite words for poses and boxes: +-1, +-0, the (cos, sin) words below, 0.5,
// denormals.
static const unsigned kPoseWords2g[16] =
	{
	0x3f800000u, 0xbf800000u, 0x00000000u, 0x80000000u, 0x3f19999au, 0x3f4ccccdu, 0xbf19999au, 0xbf4ccccdu,
	0x3f3504f3u, 0xbf3504f3u, 0x3f000000u, 0x3f5db3d7u, 0x00000003u, 0x80400000u, 0x3e800000u, 0x40000000u
	};
static const unsigned kCosSin2g[4][2] =
	{
	{ 0x3f19999au, 0x3f4ccccdu }, { 0x3f4ccccdu, 0x3f19999au }, { 0x3f3504f3u, 0x3f3504f3u }, { 0x3f5db3d7u, 0x3f000000u }
	};

// A finite word of +-[2^-7, 2^8), drawn as bits.
static unsigned nx2gMidWord(unsigned* state)
	{
	const unsigned mantissa = nxNext(state);
	const unsigned exponent = nxNext(state);
	return (mantissa & 0x807fffffu) | ((120u + exponent % 15u) << 23);
	}

static void nx2gSetWord(void* p, unsigned word)
	{
	memcpy(p, &word, 4);
	}

// A rotation's nine words.
static void nx2gRotation(unsigned* state, unsigned* r)
	{
	const unsigned kind = nxNext(state) % 4;
	if(kind < 2)
		{
		// A signed permutation.
		const unsigned perm = nxNext(state) % 6;
		static const unsigned kPerm[6][3] = { { 0, 1, 2 }, { 0, 2, 1 }, { 1, 0, 2 }, { 1, 2, 0 }, { 2, 0, 1 }, { 2, 1, 0 } };
		const unsigned signs = nxNext(state);
		for(unsigned i = 0; i < 9; ++i)
			r[i] = 0;
		for(unsigned row = 0; row < 3; ++row)
			r[3 * row + kPerm[perm][row]] = ((signs >> row) & 1) ? 0xbf800000u : 0x3f800000u;
		}
	else if(kind == 2)
		{
		// About one axis by fixed (cos, sin) words.
		const unsigned axis = nxNext(state) % 3;
		const unsigned pair = nxNext(state) % 4;
		const unsigned flip = nxNext(state) & 1;
		const unsigned c = kCosSin2g[pair][0], s = kCosSin2g[pair][1];
		const unsigned a = (axis + 1) % 3, b = (axis + 2) % 3;
		for(unsigned i = 0; i < 9; ++i)
			r[i] = 0;
		r[3 * axis + axis] = 0x3f800000u;
		r[3 * a + a] = c;
		r[3 * b + b] = c;
		r[3 * a + b] = flip ? s : s | 0x80000000u;
		r[3 * b + a] = flip ? s | 0x80000000u : s;
		}
	else
		{
		// Words from the table (not orthonormal: the rows do not renormalise).
		for(unsigned i = 0; i < 9; ++i)
			{
			const unsigned pick = nxNext(state);
			r[i] = kPoseWords2g[pick % 16];
			}
		}
	}

// A box mesh: 8 corners (corner i + 2j + 4k), 12 outward triangles.
struct Nx2gBox
	{
	unsigned		verts[24];
	unsigned short	tris[36];
	unsigned		centre[3];
	unsigned		lo[3], hi[3];
	};

// The box sides: pairs of lattice words whose difference is a power of two, so
// every face's cross product is a power of two on one axis and the vendored
// Plane::Set in the side's own 001472 build normalises it exactly (with other
// spans its 1/c is inexact and the vendored and oracle 005155 normals differ in
// the last place). The small rows' pairs, then rows 2 and 7's.
static const unsigned kSpans2g[44][2] =
	{
	{ 0x00000000u, 0x3f800000u },	// 1
	{ 0x00000000u, 0x40000000u },	// 2
	{ 0x00000000u, 0x40800000u },	// 4
	{ 0x3f800000u, 0x40000000u },	// 1
	{ 0x3f800000u, 0x40400000u },	// 2
	{ 0x40000000u, 0x40400000u },	// 1
	{ 0x40000000u, 0x40800000u },	// 2
	{ 0x40400000u, 0x40800000u },	// 1
	{ 0xbfc00000u, 0xbf000000u },	// 1
	{ 0x3e000000u, 0x3ec00000u },	// 0.25
	{ 0x3e000000u, 0x3f200000u },	// 0.5
	{ 0x3ec00000u, 0x3f000000u },	// 0.125
	{ 0x3ec00000u, 0x3f200000u },	// 0.25
	{ 0x3f000000u, 0x3f200000u },	// 0.125
	{ 0xc0800000u, 0xc0400000u },	// 1
	{ 0xc0800000u, 0xc0000000u },	// 2
	{ 0xc0800000u, 0x80000000u },	// 4
	{ 0xc0400000u, 0xc0000000u },	// 1
	{ 0xc0400000u, 0xbf800000u },	// 2
	{ 0xc0000000u, 0xbf800000u },	// 1
	{ 0xc0000000u, 0x80000000u },	// 2
	{ 0xbf800000u, 0x80000000u },	// 1
	{ 0x80000000u, 0x3f000000u },	// 0.5
	{ 0x3f000000u, 0x3fc00000u },	// 1
	{ 0x3f000000u, 0x40200000u },	// 2
	{ 0x3fc00000u, 0x40200000u },	// 1
	{ 0xc1000000u, 0xc0c00000u },	// 2
	{ 0xc0c00000u, 0xc0000000u },	// 4
	{ 0xc0000000u, 0xbf800000u },	// 1
	{ 0xbf800000u, 0xbf400000u },	// 0.25
	{ 0x42c00000u, 0x43000000u },	// 32
	{ 0x42c00000u, 0x43200000u },	// 64
	{ 0x42c00000u, 0x43600000u },	// 128
	{ 0x43000000u, 0x43200000u },	// 32
	{ 0x43000000u, 0x43800000u },	// 128
	{ 0x43200000u, 0x43600000u },	// 64
	{ 0x43600000u, 0x43800000u },	// 32
	{ 0x45000000u, 0x45400000u },	// 1024
	{ 0x45000000u, 0x45800000u },	// 2048
	{ 0x45000000u, 0x45c00000u },	// 4096
	{ 0x45400000u, 0x45800000u },	// 1024
	{ 0x45800000u, 0x45c00000u },	// 2048
	{ 0x45800000u, 0x46000000u },	// 4096
	{ 0x45c00000u, 0x46000000u },	// 2048
	};
static const unsigned kNbSmallSpans2g = 30;

// Each pair's midpoint (the pairs are exact, so each midpoint is a float): the
// hull's centre in three boxes of four, so that when one box's centre lies inside
// the other no face of it passes 001809's centre test and 001807 runs.
static const unsigned kSpanMid2g[44] =
	{
	0x3f000000u, 0x3f800000u, 0x40000000u, 0x3fc00000u, 0x40000000u, 0x40200000u, 0x40400000u, 0x40600000u,
	0xbf800000u, 0x3e800000u, 0x3ec00000u, 0x3ee00000u, 0x3f000000u, 0x3f100000u, 0xc0600000u, 0xc0400000u,
	0xc0000000u, 0xc0200000u, 0xc0000000u, 0xbfc00000u, 0xbf800000u, 0xbf000000u, 0x3e800000u, 0x3f800000u,
	0x3fc00000u, 0x40000000u, 0xc0e00000u, 0xc0800000u, 0xbfc00000u, 0xbf600000u, 0x42e00000u, 0x43000000u,
	0x43200000u, 0x43100000u, 0x43400000u, 0x43400000u, 0x43700000u, 0x45200000u, 0x45400000u, 0x45800000u,
	0x45600000u, 0x45a00000u, 0x45c00000u, 0x45e00000u
	};

// The lattice rows of small words (0..10), for the translations.
static const unsigned kSmallRows2g[6] = { 0, 1, 3, 4, 5, 6 };
static const unsigned kNearRows2g[3] = { 0, 3, 5 };

static void nx2gBuildBox(unsigned* state, Nx2gBox& box, bool large)
	{
	const unsigned nbSpans = sizeof(kSpans2g) / sizeof(kSpans2g[0]);
	for(unsigned axis = 0; axis < 3; ++axis)
		{
		const unsigned spanDraw = nxNext(state);
		const unsigned span = large ? kNbSmallSpans2g + spanDraw % (nbSpans - kNbSmallSpans2g)
			: spanDraw % kNbSmallSpans2g;
		const unsigned centreDraw = nxNext(state);
		box.centre[axis] = centreDraw % 4 == 0 ? kSpans2g[(span + centreDraw % 3) % nbSpans][(centreDraw >> 2) & 1]
			: kSpanMid2g[span];
		box.lo[axis] = kSpans2g[span][0];
		box.hi[axis] = kSpans2g[span][1];
		}
	for(unsigned c = 0; c < 8; ++c)
		{
		box.verts[3 * c + 0] = (c & 1) ? box.hi[0] : box.lo[0];
		box.verts[3 * c + 1] = (c & 2) ? box.hi[1] : box.lo[1];
		box.verts[3 * c + 2] = (c & 4) ? box.hi[2] : box.lo[2];
		}
	static const unsigned kQuads[6][4] = { { 1, 3, 7, 5 }, { 0, 4, 6, 2 }, { 2, 6, 7, 3 }, { 0, 1, 5, 4 }, { 4, 5, 7, 6 }, { 0, 2, 3, 1 } };
	for(unsigned q = 0; q < 6; ++q)
		{
		const unsigned diagonal = nxNext(state) & 1;
		unsigned short* t = &box.tris[6 * q];
		const unsigned a = kQuads[q][0], b = kQuads[q][1], c = kQuads[q][2], d = kQuads[q][3];
		if(diagonal)
			{
			t[0] = (unsigned short) a; t[1] = (unsigned short) b; t[2] = (unsigned short) c;
			t[3] = (unsigned short) a; t[4] = (unsigned short) c; t[5] = (unsigned short) d;
			}
		else
			{
			t[0] = (unsigned short) a; t[1] = (unsigned short) b; t[2] = (unsigned short) d;
			t[3] = (unsigned short) b; t[4] = (unsigned short) c; t[5] = (unsigned short) d;
			}
		}
	}

// The vertex graph 001530 climbs: +0x08 the neighbour counts, +0x0c the offsets
// into +0x10, the neighbours (every triangle edge, both ways, once).
struct Nx2gGraph
	{
	unsigned		word0, word4;
	const unsigned*	counts;
	const unsigned*	offsets;
	const unsigned*	neighbours;
	unsigned		countStore[64], offsetStore[64], neighbourStore[512];
	};

static void nx2gBuildGraph(const Nx2gBox& box, Nx2gGraph& g)
	{
	bool adjacent[8][8];
	memset(adjacent, 0, sizeof(adjacent));
	for(unsigned t = 0; t < 12; ++t)
		for(unsigned e = 0; e < 3; ++e)
			{
			const unsigned a = box.tris[3 * t + e], b = box.tris[3 * t + (e + 1) % 3];
			adjacent[a][b] = adjacent[b][a] = true;
			}
	unsigned next = 0;
	for(unsigned v = 0; v < 8; ++v)
		{
		g.offsetStore[v] = next;
		g.countStore[v] = 0;
		for(unsigned w = 0; w < 8; ++w)
			if(adjacent[v][w])
				{
				g.neighbourStore[next++] = w;
				++g.countStore[v];
				}
		}
	g.word0 = 0xcdcd6400u;
	g.word4 = 0xcdcd6404u;
	g.counts = g.countStore;
	g.offsets = g.offsetStore;
	g.neighbours = g.neighbourStore;
	}

// One side's images of one hull.
struct Nx2gHullSide
	{
	unsigned		hull[0x80 / 4];
	unsigned char	mesh[0xb0];
	unsigned		map[6];
	bool			hasMap;
	bool			handBuilt;			// a polytope: its polygon and reference arrays are the storage below
	unsigned		polyStore[80 * 9];
	unsigned		refStore[256];
	};

static const unsigned kNb2gHulls = 12;

struct Nx2gSide
	{
	bool				oracle;
	unsigned char*		base;
	Nx2gHullSide		hulls[kNb2gHulls];
	unsigned char		scratch[0x500];
	unsigned			visited[64];
	unsigned			pruner[8];
	float				boxes[2][6];
	};

static void nx2gContainerCtor(Nx2gSide& s, void* object)
	{
	if(s.oracle)
		nx2gCall0(s.base + kIceContainerCtorRva, object);
	else
		new(object) IceCore::Container;
	}

static void nx2gContainerDtor(Nx2gSide& s, void* object)
	{
	if(s.oracle)
		nx2gCall0(s.base + kIceContainerDtorRva, object);
	else
		((IceCore::Container*) object)->~Container();
	}

// A block of the side's own 004803 allocator, released through its slot 3.
static void nx2gFree(Nx2gSide& s, void* block)
	{
	if(!block)
		return;
	if(s.oracle)
		{
		typedef void** (__cdecl* GetterFn)();
		typedef void (__thiscall* FreeFn)(void*, void*);
		void** allocator = ((GetterFn) (s.base + kSdkAllocatorGetterRva))();
		((FreeFn) ((void**) *allocator)[3])(allocator, block);
		}
	else
		nxGetSdkAllocator()->free(block);
	}

static void nx2gBuildHull(Nx2gSide& s, Nx2gHullSide& h, const Nx2gBox& box, const Nx2gGraph* graph,
	unsigned mapSubdiv)
	{
	memset(h.hull, 0, sizeof(h.hull));
	h.handBuilt = false;
	h.hull[0] = 0xcdcd8000u;
	h.hull[1] = 12;
	h.hull[2] = (unsigned) (size_t) box.tris;
	h.hull[3] = 8;
	h.hull[4] = (unsigned) (size_t) box.verts;
	h.hull[6] = box.centre[0];
	h.hull[7] = box.centre[1];
	h.hull[8] = box.centre[2];
	h.hull[25] = (unsigned) (size_t) graph;
	memset(h.mesh, 0, sizeof(h.mesh));
	const void* table = s.oracle ? (const void*) (s.base + kPolygonTableRva) : (const void*) gTriangleMeshPolygonTable;
	*(const void**) (h.mesh + 0x04) = table;
	*(unsigned**) (h.mesh + 0xa0) = h.hull;
	*(unsigned*) (h.mesh + 0xa4) = 0xcdcda4a4u;
	const void* const* slots = (const void* const*) table;
	nxSetControl(kControlDefault);
	nx2gCall0(slots[3], h.mesh + 4);		// 002221: 001472
	nx2gCall0(slots[6], h.mesh + 4);		// 002227: 001502
	h.hasMap = mapSubdiv != 0;
	if(h.hasMap)
		{
		for(unsigned i = 0; i < 6; ++i)
			h.map[i] = 0xcdcdc000u + i;
		nx2gCall1(s.oracle ? (const void*) (s.base + kVertexMapCtorRva) : (const void*) &nxSupportMapVertexConstruct,
			h.map, (unsigned) (size_t) h.hull);
		nx2gCall1(s.oracle ? (const void*) (s.base + kSupportMapInitRva) : (const void*) &nxSupportMapInit,
			h.map, mapSubdiv);
		*(unsigned**) (h.mesh + 0xa8) = h.map;
		}
	}

static void nx2gReleaseHull(Nx2gSide& s, Nx2gHullSide& h)
	{
	if(h.hasMap)
		{
		// Slot 0 of the map's own table without the free bit: the bytes released.
		const void* const* table = (const void* const*) (size_t) h.map[0];
		nx2gCall1(table[0], h.map, 0);
		}
	const unsigned cookieFields[2] = { 10, 15 };
	const unsigned plainFields[5] = { 11, 12, 16, 17, 18 };
	for(int i = 0; i < 2; ++i)
		if(h.hull[cookieFields[i]] && !(h.handBuilt && cookieFields[i] == 10))
			nx2gFree(s, (unsigned*) (size_t) h.hull[cookieFields[i]] - 1);
	for(int i = 0; i < 5; ++i)
		if(h.hull[plainFields[i]] && !(h.handBuilt && plainFields[i] == 11))
			nx2gFree(s, (void*) (size_t) h.hull[plainFields[i]]);
	}

// The two sides' built edge arrays, word for word: the edges (+0x3c, two words
// each), the edge normals (+0x40, three), the edge-to-polygon descriptors
// (+0x44, two), the polygons by edge (+0x48, one per link) and every polygon's
// edge numbers (+0x30, one per outline vertex). A count of differing words for
// the name line's build_mismatches; nothing here enters a digest.
static unsigned nx2gCompareEdges(const unsigned* a, const unsigned* b)
	{
	if(a[9] != b[9] || a[14] != b[14])
		return 1;
	unsigned differing = 0;
	const unsigned nbEdges = a[14];
	const unsigned sizes[3][2] = { { 15, 2 }, { 16, 3 }, { 17, 2 } };
	for(unsigned k = 0; k < 3; ++k)
		{
		const unsigned* ea = (const unsigned*) (size_t) a[sizes[k][0]];
		const unsigned* eb = (const unsigned*) (size_t) b[sizes[k][0]];
		if(!ea || !eb)
			{
			differing += (ea != 0) != (eb != 0);
			continue;
			}
		for(unsigned i = 0; i < sizes[k][1] * nbEdges; ++i)
			differing += ea[i] != eb[i];
		}
	unsigned links = 0;
	const unsigned* descs = (const unsigned*) (size_t) a[17];
	for(unsigned e = 0; descs && e < nbEdges; ++e)
		links += descs[2 * e] >> 16;
	const unsigned* ta = (const unsigned*) (size_t) a[18];
	const unsigned* tb = (const unsigned*) (size_t) b[18];
	for(unsigned i = 0; ta && tb && i < links; ++i)
		differing += ta[i] != tb[i];
	unsigned total = 0;
	const unsigned* polys = (const unsigned*) (size_t) a[10];
	for(unsigned q = 0; polys && q < a[9]; ++q)
		total += polys[9 * q];
	const unsigned* ra = (const unsigned*) (size_t) a[12];
	const unsigned* rb = (const unsigned*) (size_t) b[12];
	for(unsigned i = 0; ra && rb && i < total; ++i)
		differing += ra[i] != rb[i];
	return differing;
	}

static void nx2gFoldScratch(NxDigest* digest, const unsigned char* scratch)
	{
	for(unsigned c = 0; c < 2; ++c)
		{
		const unsigned* container = (const unsigned*) (scratch + 0x4e0 + 0x10 * c);
		const unsigned count = container[1];
		nxFoldInput(digest, &count, 4);
		const unsigned* entries = (const unsigned*) (size_t) container[2];
		if(entries && count)
			nxFoldInput(digest, entries, 4 * count);
		}
	nxFoldInput(digest, scratch + 0x14, 4);
	}

static unsigned nx2gCompareScratch(const unsigned char* a, const unsigned char* b)
	{
	unsigned differing = 0;
	for(unsigned c = 0; c < 2; ++c)
		{
		const unsigned* ca = (const unsigned*) (a + 0x4e0 + 0x10 * c);
		const unsigned* cb = (const unsigned*) (b + 0x4e0 + 0x10 * c);
		if(ca[1] != cb[1])
			{
			++differing;
			continue;
			}
		const unsigned* ea = (const unsigned*) (size_t) ca[2];
		const unsigned* eb = (const unsigned*) (size_t) cb[2];
		for(unsigned i = 0; i < ca[1]; ++i)
			if(ea[i] != eb[i])
				++differing;
		}
	if(memcmp(a + 0x14, b + 0x14, 4) != 0)
		++differing;
	return differing;
	}

// The polytopes: hand-built hull images of convex hulls that are not boxes
// (convex-mesh gap Task 2g review; the contract's test route asks for prisms and
// a sphere-like hull). L's rows read a hull only through the polygon interface,
// so each side gets the same vertex, outline and plane words and builds its own
// edges with its own 001502 (slot 6); its own 001472 never runs (the polygon
// count is set), so no vendored Plane::Set / Triangle::Normal is reached and the
// images are identical on both sides.
// Generated offline (scratchpad genpoly.py): vertex words, and per polygon its
// outline (counter-clockwise seen from outside), its plane words (the normal of
// the first three outline vertices normalised in double and narrowed, d = -n.v0)
// and its least and greatest vertex projection. Inputs, fixed words.
// triangular prism: 6 vertices, 5 polygons
static const unsigned kPolyVerts0[18] =
	{
	0x24b07d7eu, 0x3fa00000u, 0xbf400000u,
	0xbf8a9067u, 0xbf200000u, 0xbf400000u,
	0x3f8a9067u, 0xbf200000u, 0xbf400000u,
	0x24b07d7eu, 0x3fa00000u, 0x3f400000u,
	0xbf8a9067u, 0xbf200000u, 0x3f400000u,
	0x3f8a9067u, 0xbf200000u, 0x3f400000u
	};
static const unsigned kPolyRefs0[18] = { 2, 1, 0, 3, 4, 5, 0, 1, 4, 3, 1, 2, 5, 4, 2, 0, 3, 5 };
static const unsigned kPolyFaces0[5][8] =
	{
	{ 3, 0, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf400000u, 0xbf400000u, 0x3f400000u },
	{ 3, 3, 0x80000000u, 0x00000000u, 0x3f800000u, 0xbf400000u, 0xbf400000u, 0x3f400000u },
	{ 4, 6, 0xbf5db3d7u, 0x3f000000u, 0x00000000u, 0xbf200000u, 0xbfa00000u, 0x3f200001u },
	{ 4, 10, 0x00000000u, 0xbf800000u, 0x00000000u, 0xbf200000u, 0xbfa00000u, 0x3f200000u },
	{ 4, 14, 0x3f5db3d7u, 0x3f000000u, 0x80000000u, 0xbf200001u, 0xbfa00000u, 0x3f200001u },
	};
// hexagonal prism: 12 vertices, 8 polygons
static const unsigned kPolyVerts1[36] =
	{
	0x3f800000u, 0x00000000u, 0xbf000000u,
	0x3f000000u, 0x3f5db3d7u, 0xbf000000u,
	0xbf000000u, 0x3f5db3d7u, 0xbf000000u,
	0xbf800000u, 0x250d3132u, 0xbf000000u,
	0xbf000000u, 0xbf5db3d7u, 0xbf000000u,
	0x3f000000u, 0xbf5db3d7u, 0xbf000000u,
	0x3f800000u, 0x00000000u, 0x3f000000u,
	0x3f000000u, 0x3f5db3d7u, 0x3f000000u,
	0xbf000000u, 0x3f5db3d7u, 0x3f000000u,
	0xbf800000u, 0x250d3132u, 0x3f000000u,
	0xbf000000u, 0xbf5db3d7u, 0x3f000000u,
	0x3f000000u, 0xbf5db3d7u, 0x3f000000u
	};
static const unsigned kPolyRefs1[36] = { 5, 4, 3, 2, 1, 0, 6, 7, 8, 9, 10, 11, 0, 1, 7, 6, 1, 2, 8, 7, 2, 3, 9, 8, 3, 4, 10, 9, 4, 5, 11, 10, 5, 0, 6, 11 };
static const unsigned kPolyFaces1[8][8] =
	{
	{ 6, 0, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbf000000u, 0xbf000000u, 0x3f000000u },
	{ 6, 6, 0x00000000u, 0x00000000u, 0x3f800000u, 0xbf000000u, 0xbf000000u, 0x3f000000u },
	{ 4, 12, 0x3f5db3d7u, 0x3f000000u, 0x80000000u, 0xbf5db3d7u, 0xbf5db3d7u, 0x3f5db3d7u },
	{ 4, 16, 0x00000000u, 0x3f800000u, 0x80000000u, 0xbf5db3d7u, 0xbf5db3d7u, 0x3f5db3d7u },
	{ 4, 20, 0xbf5db3d7u, 0x3f000000u, 0x00000000u, 0xbf5db3d7u, 0xbf5db3d7u, 0x3f5db3d7u },
	{ 4, 24, 0xbf5db3d7u, 0xbf000000u, 0x00000000u, 0xbf5db3d7u, 0xbf5db3d7u, 0x3f5db3d7u },
	{ 4, 28, 0x00000000u, 0xbf800000u, 0x00000000u, 0xbf5db3d7u, 0xbf5db3d7u, 0x3f5db3d7u },
	{ 4, 32, 0x3f5db3d7u, 0xbf000000u, 0x00000000u, 0xbf5db3d7u, 0xbf5db3d7u, 0x3f5db3d7u },
	};
// pentagonal prism, tall: 10 vertices, 7 polygons
static const unsigned kPolyVerts2[30] =
	{
	0x3f376cb3u, 0x3e62f5a3u, 0xbfc00000u,
	0x3c2dfbb6u, 0x3f3ffb13u, 0xbfc00000u,
	0xbf35be97u, 0x3e77a49cu, 0xbfc00000u,
	0xbee615e4u, 0xbf19b7c2u, 0xbfc00000u,
	0x3edd49ceu, 0xbf1ce9e0u, 0xbfc00000u,
	0x3f376cb3u, 0x3e62f5a3u, 0x3fc00000u,
	0x3c2dfbb6u, 0x3f3ffb13u, 0x3fc00000u,
	0xbf35be97u, 0x3e77a49cu, 0x3fc00000u,
	0xbee615e4u, 0xbf19b7c2u, 0x3fc00000u,
	0x3edd49ceu, 0xbf1ce9e0u, 0x3fc00000u
	};
static const unsigned kPolyRefs2[30] = { 4, 3, 2, 1, 0, 5, 6, 7, 8, 9, 0, 1, 6, 5, 1, 2, 7, 6, 2, 3, 8, 7, 3, 4, 9, 8, 4, 0, 5, 9 };
static const unsigned kPolyFaces2[7][8] =
	{
	{ 5, 0, 0x00000000u, 0x00000000u, 0xbf800000u, 0xbfc00000u, 0xbfc00000u, 0x3fc00000u },
	{ 5, 5, 0x00000000u, 0x00000000u, 0x3f800000u, 0xbfc00000u, 0xbfc00000u, 0x3fc00000u },
	{ 4, 10, 0x3f1963eeu, 0x3f4cf503u, 0x80000000u, 0xbf1b54ceu, 0xbf400000u, 0x3f1b54ceu },
	{ 4, 14, 0xbf13868au, 0x3f5137d6u, 0x00000000u, 0xbf1b54ceu, 0xbf400000u, 0x3f1b54ceu },
	{ 4, 18, 0xbf7490eeu, 0xbe974e6eu, 0x00000000u, 0xbf1b54cdu, 0xbf400000u, 0x3f1b54ceu },
	{ 4, 22, 0xbc67fa34u, 0xbf7ff96eu, 0x00000000u, 0xbf1b54cdu, 0xbf400000u, 0x3f1b54cdu },
	{ 4, 26, 0x3f725374u, 0xbea51869u, 0x00000000u, 0xbf1b54ceu, 0xbf400001u, 0x3f1b54ceu },
	};
// octahedron: 6 vertices, 8 polygons
static const unsigned kPolyVerts3[18] =
	{
	0x3fc00000u, 0x00000000u, 0x00000000u,
	0xbfc00000u, 0x00000000u, 0x00000000u,
	0x00000000u, 0x3fc00000u, 0x00000000u,
	0x00000000u, 0xbfc00000u, 0x00000000u,
	0x00000000u, 0x00000000u, 0x3fc00000u,
	0x00000000u, 0x00000000u, 0xbfc00000u
	};
static const unsigned kPolyRefs3[24] = { 0, 2, 4, 2, 1, 4, 1, 3, 4, 3, 0, 4, 2, 0, 5, 1, 2, 5, 3, 1, 5, 0, 3, 5 };
static const unsigned kPolyFaces3[8][8] =
	{
	{ 3, 0, 0x3f13cd3au, 0x3f13cd3au, 0x3f13cd3au, 0xbf5db3d7u, 0xbf5db3d7u, 0x3f5db3d7u },
	{ 3, 3, 0xbf13cd3au, 0x3f13cd3au, 0x3f13cd3au, 0xbf5db3d7u, 0xbf5db3d7u, 0x3f5db3d7u },
	{ 3, 6, 0xbf13cd3au, 0xbf13cd3au, 0x3f13cd3au, 0xbf5db3d7u, 0xbf5db3d7u, 0x3f5db3d7u },
	{ 3, 9, 0x3f13cd3au, 0xbf13cd3au, 0x3f13cd3au, 0xbf5db3d7u, 0xbf5db3d7u, 0x3f5db3d7u },
	{ 3, 12, 0x3f13cd3au, 0x3f13cd3au, 0xbf13cd3au, 0xbf5db3d7u, 0xbf5db3d7u, 0x3f5db3d7u },
	{ 3, 15, 0xbf13cd3au, 0x3f13cd3au, 0xbf13cd3au, 0xbf5db3d7u, 0xbf5db3d7u, 0x3f5db3d7u },
	{ 3, 18, 0xbf13cd3au, 0xbf13cd3au, 0xbf13cd3au, 0xbf5db3d7u, 0xbf5db3d7u, 0x3f5db3d7u },
	{ 3, 21, 0x3f13cd3au, 0xbf13cd3au, 0xbf13cd3au, 0xbf5db3d7u, 0xbf5db3d7u, 0x3f5db3d7u },
	};
// icosphere (80 triangles): 42 vertices, 80 polygons
static const unsigned kPolyVerts4[126] =
	{
	0xbf283be5u, 0x3f881aa8u, 0x00000000u,
	0x3f283be5u, 0x3f881aa8u, 0x00000000u,
	0xbf283be5u, 0xbf881aa8u, 0x00000000u,
	0x3f283be5u, 0xbf881aa8u, 0x00000000u,
	0x00000000u, 0xbf283be5u, 0x3f881aa8u,
	0x00000000u, 0x3f283be5u, 0x3f881aa8u,
	0x00000000u, 0xbf283be5u, 0xbf881aa8u,
	0x00000000u, 0x3f283be5u, 0xbf881aa8u,
	0x3f881aa8u, 0x00000000u, 0xbf283be5u,
	0x3f881aa8u, 0x00000000u, 0x3f283be5u,
	0xbf881aa8u, 0x00000000u, 0xbf283be5u,
	0xbf881aa8u, 0x00000000u, 0x3f283be5u,
	0xbf817156u, 0x3f200000u, 0x3ec5c558u,
	0xbf200000u, 0x3ec5c558u, 0x3f817156u,
	0xbec5c558u, 0x3f817156u, 0x3f200000u,
	0x3ec5c558u, 0x3f817156u, 0x3f200000u,
	0x00000000u, 0x3fa00000u, 0x00000000u,
	0x3ec5c558u, 0x3f817156u, 0xbf200000u,
	0xbec5c558u, 0x3f817156u, 0xbf200000u,
	0xbf200000u, 0x3ec5c558u, 0xbf817156u,
	0xbf817156u, 0x3f200000u, 0xbec5c558u,
	0xbfa00000u, 0x00000000u, 0x00000000u,
	0x3f200000u, 0x3ec5c558u, 0x3f817156u,
	0x3f817156u, 0x3f200000u, 0x3ec5c558u,
	0xbf200000u, 0xbec5c558u, 0x3f817156u,
	0x00000000u, 0x00000000u, 0x3fa00000u,
	0xbf817156u, 0xbf200000u, 0xbec5c558u,
	0xbf817156u, 0xbf200000u, 0x3ec5c558u,
	0x00000000u, 0x00000000u, 0xbfa00000u,
	0xbf200000u, 0xbec5c558u, 0xbf817156u,
	0x3f817156u, 0x3f200000u, 0xbec5c558u,
	0x3f200000u, 0x3ec5c558u, 0xbf817156u,
	0x3f817156u, 0xbf200000u, 0x3ec5c558u,
	0x3f200000u, 0xbec5c558u, 0x3f817156u,
	0x3ec5c558u, 0xbf817156u, 0x3f200000u,
	0xbec5c558u, 0xbf817156u, 0x3f200000u,
	0x00000000u, 0xbfa00000u, 0x00000000u,
	0xbec5c558u, 0xbf817156u, 0xbf200000u,
	0x3ec5c558u, 0xbf817156u, 0xbf200000u,
	0x3f200000u, 0xbec5c558u, 0xbf817156u,
	0x3f817156u, 0xbf200000u, 0xbec5c558u,
	0x3fa00000u, 0x00000000u, 0x00000000u
	};
static const unsigned kPolyRefs4[240] = { 0, 12, 14, 11, 13, 12, 5, 14, 13, 12, 13, 14, 0, 14, 16, 5, 15, 14, 1, 16, 15, 14, 15, 16, 0, 16, 18, 1, 17, 16, 7, 18, 17, 16, 17, 18, 0, 18, 20, 7, 19, 18, 10, 20, 19, 18, 19, 20, 0, 20, 12, 10, 21, 20, 11, 12, 21, 20, 21, 12, 1, 15, 23, 5, 22, 15, 9, 23, 22, 15, 22, 23, 5, 13, 25, 11, 24, 13, 4, 25, 24, 13, 24, 25, 11, 21, 27, 10, 26, 21, 2, 27, 26, 21, 26, 27, 10, 19, 29, 7, 28, 19, 6, 29, 28, 19, 28, 29, 7, 17, 31, 1, 30, 17, 8, 31, 30, 17, 30, 31, 3, 32, 34, 9, 33, 32, 4, 34, 33, 32, 33, 34, 3, 34, 36, 4, 35, 34, 2, 36, 35, 34, 35, 36, 3, 36, 38, 2, 37, 36, 6, 38, 37, 36, 37, 38, 3, 38, 40, 6, 39, 38, 8, 40, 39, 38, 39, 40, 3, 40, 32, 8, 41, 40, 9, 32, 41, 40, 41, 32, 4, 33, 25, 9, 22, 33, 5, 25, 22, 33, 22, 25, 2, 35, 27, 4, 24, 35, 11, 27, 24, 35, 24, 27, 6, 37, 29, 2, 26, 37, 10, 29, 26, 37, 26, 29, 8, 39, 31, 6, 28, 39, 7, 31, 28, 39, 28, 31, 9, 41, 23, 8, 30, 41, 1, 23, 30, 41, 30, 23 };
static const unsigned kPolyFaces4[80][8] =
	{
	{ 3, 0, 0xbf1547eeu, 0x3f3fb046u, 0x3ea153fdu, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 3, 0xbf3fb046u, 0x3ea153fdu, 0x3f1547eeu, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 6, 0xbea153fdu, 0x3f1547eeu, 0x3f3fb046u, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 9, 0xbf13cd3au, 0x3f13cd3au, 0x3f13cd3au, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 12, 0xbe893bdfu, 0x3f718aacu, 0x3e476997u, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 15, 0x00000000u, 0x3f472254u, 0x3f20e0adu, 0xbf96f6abu, 0xbf96f6abu, 0x3f96f6abu },
	{ 3, 18, 0x3e893bdfu, 0x3f718aacu, 0x3e476997u, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 21, 0x80000000u, 0x3f6f25ebu, 0x3eb6b163u, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 24, 0xbe893bdfu, 0x3f718aacu, 0xbe476997u, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 27, 0x3e893bdfu, 0x3f718aacu, 0xbe476997u, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 30, 0x00000000u, 0x3f472254u, 0xbf20e0adu, 0xbf96f6abu, 0xbf96f6abu, 0x3f96f6abu },
	{ 3, 33, 0x00000000u, 0x3f6f25ebu, 0xbeb6b163u, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 36, 0xbf1547eeu, 0x3f3fb046u, 0xbea153fdu, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 39, 0xbea153fdu, 0x3f1547eeu, 0xbf3fb046u, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 42, 0xbf3fb046u, 0x3ea153fdu, 0xbf1547eeu, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 45, 0xbf13cd3au, 0x3f13cd3au, 0xbf13cd3au, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 48, 0xbf472254u, 0x3f20e0adu, 0x00000000u, 0xbf96f6abu, 0xbf96f6abu, 0x3f96f6abu },
	{ 3, 51, 0xbf718aacu, 0x3e476997u, 0xbe893bdfu, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 54, 0xbf718aacu, 0x3e476997u, 0x3e893bdfu, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 57, 0xbf6f25ebu, 0x3eb6b163u, 0x00000000u, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 60, 0x3f1547eeu, 0x3f3fb046u, 0x3ea153fdu, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 63, 0x3ea153fdu, 0x3f1547eeu, 0x3f3fb046u, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 66, 0x3f3fb046u, 0x3ea153fdu, 0x3f1547eeu, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 69, 0x3f13cd3au, 0x3f13cd3au, 0x3f13cd3au, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 72, 0xbe476997u, 0x3e893bdfu, 0x3f718aacu, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 75, 0xbf20e0adu, 0x00000000u, 0x3f472254u, 0xbf96f6abu, 0xbf96f6abu, 0x3f96f6abu },
	{ 3, 78, 0xbe476997u, 0xbe893bdfu, 0x3f718aacu, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 81, 0xbeb6b163u, 0x00000000u, 0x3f6f25ebu, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 84, 0xbf718aacu, 0xbe476997u, 0x3e893bdfu, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 87, 0xbf718aacu, 0xbe476997u, 0xbe893bdfu, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 90, 0xbf472254u, 0xbf20e0adu, 0x80000000u, 0xbf96f6abu, 0xbf96f6abu, 0x3f96f6abu },
	{ 3, 93, 0xbf6f25ebu, 0xbeb6b163u, 0x00000000u, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 96, 0xbf20e0adu, 0x80000000u, 0xbf472254u, 0xbf96f6abu, 0xbf96f6abu, 0x3f96f6abu },
	{ 3, 99, 0xbe476997u, 0x3e893bdfu, 0xbf718aacu, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 102, 0xbe476997u, 0xbe893bdfu, 0xbf718aacu, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 105, 0xbeb6b163u, 0x00000000u, 0xbf6f25ebu, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 108, 0x3ea153fdu, 0x3f1547eeu, 0xbf3fb046u, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 111, 0x3f1547eeu, 0x3f3fb046u, 0xbea153fdu, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 114, 0x3f3fb046u, 0x3ea153fdu, 0xbf1547eeu, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 117, 0x3f13cd3au, 0x3f13cd3au, 0xbf13cd3au, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 120, 0x3f1547eeu, 0xbf3fb046u, 0x3ea153fdu, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 123, 0x3f3fb046u, 0xbea153fdu, 0x3f1547eeu, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 126, 0x3ea153fdu, 0xbf1547eeu, 0x3f3fb046u, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 129, 0x3f13cd3au, 0xbf13cd3au, 0x3f13cd3au, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 132, 0x3e893bdfu, 0xbf718aacu, 0x3e476997u, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 135, 0x00000000u, 0xbf472254u, 0x3f20e0adu, 0xbf96f6abu, 0xbf96f6abu, 0x3f96f6abu },
	{ 3, 138, 0xbe893bdfu, 0xbf718aacu, 0x3e476997u, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 141, 0x00000000u, 0xbf6f25ebu, 0x3eb6b163u, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 144, 0x3e893bdfu, 0xbf718aacu, 0xbe476997u, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 147, 0xbe893bdfu, 0xbf718aacu, 0xbe476997u, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 150, 0x80000000u, 0xbf472254u, 0xbf20e0adu, 0xbf96f6abu, 0xbf96f6abu, 0x3f96f6abu },
	{ 3, 153, 0x00000000u, 0xbf6f25ebu, 0xbeb6b163u, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 156, 0x3f1547eeu, 0xbf3fb046u, 0xbea153fdu, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 159, 0x3ea153fdu, 0xbf1547eeu, 0xbf3fb046u, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 162, 0x3f3fb046u, 0xbea153fdu, 0xbf1547eeu, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 165, 0x3f13cd3au, 0xbf13cd3au, 0xbf13cd3au, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 168, 0x3f472254u, 0xbf20e0adu, 0x00000000u, 0xbf96f6abu, 0xbf96f6abu, 0x3f96f6abu },
	{ 3, 171, 0x3f718aacu, 0xbe476997u, 0xbe893bdfu, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 174, 0x3f718aacu, 0xbe476997u, 0x3e893bdfu, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 177, 0x3f6f25ebu, 0xbeb6b163u, 0x00000000u, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 180, 0x3e476997u, 0xbe893bdfu, 0x3f718aacu, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 183, 0x3f20e0adu, 0x00000000u, 0x3f472254u, 0xbf96f6abu, 0xbf96f6abu, 0x3f96f6abu },
	{ 3, 186, 0x3e476997u, 0x3e893bdfu, 0x3f718aacu, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 189, 0x3eb6b163u, 0x80000000u, 0x3f6f25ebu, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 192, 0xbf1547eeu, 0xbf3fb046u, 0x3ea153fdu, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 195, 0xbea153fdu, 0xbf1547eeu, 0x3f3fb046u, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 198, 0xbf3fb046u, 0xbea153fdu, 0x3f1547eeu, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 201, 0xbf13cd3au, 0xbf13cd3au, 0x3f13cd3au, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 204, 0xbea153fdu, 0xbf1547eeu, 0xbf3fb046u, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 207, 0xbf1547eeu, 0xbf3fb046u, 0xbea153fdu, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 210, 0xbf3fb046u, 0xbea153fdu, 0xbf1547eeu, 0xbf96f6acu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 213, 0xbf13cd3au, 0xbf13cd3au, 0xbf13cd3au, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 216, 0x3f20e0adu, 0x00000000u, 0xbf472254u, 0xbf96f6abu, 0xbf96f6abu, 0x3f96f6abu },
	{ 3, 219, 0x3e476997u, 0xbe893bdfu, 0xbf718aacu, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 222, 0x3e476997u, 0x3e893bdfu, 0xbf718aacu, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 225, 0x3eb6b163u, 0x00000000u, 0xbf6f25ebu, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	{ 3, 228, 0x3f718aacu, 0x3e476997u, 0x3e893bdfu, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 231, 0x3f718aacu, 0x3e476997u, 0xbe893bdfu, 0xbf96f6abu, 0xbf96f6acu, 0x3f96f6acu },
	{ 3, 234, 0x3f472254u, 0x3f20e0adu, 0x00000000u, 0xbf96f6abu, 0xbf96f6abu, 0x3f96f6abu },
	{ 3, 237, 0x3f6f25ebu, 0x3eb6b163u, 0x80000000u, 0xbf9577b3u, 0xbf9577b3u, 0x3f9577b3u },
	};
struct Nx2gPolytope
	{
	const char* name;
	unsigned nbVerts, nbPolygons, nbRefs;
	const unsigned* verts;
	const unsigned* refs;
	const unsigned (*faces)[8];
	};
static const Nx2gPolytope kPolytopes2g[5] =
	{
	{ "triangular prism", 6, 5, 18, kPolyVerts0, kPolyRefs0, kPolyFaces0 },
	{ "hexagonal prism", 12, 8, 36, kPolyVerts1, kPolyRefs1, kPolyFaces1 },
	{ "pentagonal prism, tall", 10, 7, 30, kPolyVerts2, kPolyRefs2, kPolyFaces2 },
	{ "octahedron", 6, 8, 24, kPolyVerts3, kPolyRefs3, kPolyFaces3 },
	{ "icosphere (80 triangles)", 42, 80, 240, kPolyVerts4, kPolyRefs4, kPolyFaces4 },
	};

static const unsigned kNb2gPolytopeHulls = 10;	// each polytope twice: without and with a kind C map

static void nx2gBuildPolytopeGraph(const Nx2gPolytope& p, Nx2gGraph& g)
	{
	static bool adjacent[64][64];
	memset(adjacent, 0, sizeof(adjacent));
	for(unsigned f = 0; f < p.nbPolygons; ++f)
		{
		const unsigned n = p.faces[f][0], first = p.faces[f][1];
		for(unsigned e = 0; e < n; ++e)
			{
			const unsigned a = p.refs[first + e], b = p.refs[first + (e + 1) % n];
			adjacent[a][b] = adjacent[b][a] = true;
			}
		}
	unsigned next = 0;
	for(unsigned v = 0; v < p.nbVerts; ++v)
		{
		g.offsetStore[v] = next;
		g.countStore[v] = 0;
		for(unsigned w = 0; w < p.nbVerts; ++w)
			if(adjacent[v][w])
				{
				g.neighbourStore[next++] = w;
				++g.countStore[v];
				}
		}
	g.word0 = 0xcdcd6400u;
	g.word4 = 0xcdcd6404u;
	g.counts = g.countStore;
	g.offsets = g.offsetStore;
	g.neighbours = g.neighbourStore;
	}

static void nx2gBuildPolytope(Nx2gSide& s, Nx2gHullSide& h, const Nx2gPolytope& p, const Nx2gGraph* graph,
	unsigned mapSubdiv)
	{
	memset(h.hull, 0, sizeof(h.hull));
	h.handBuilt = true;
	for(unsigned r = 0; r < p.nbRefs; ++r)
		h.refStore[r] = p.refs[r];
	for(unsigned f = 0; f < p.nbPolygons; ++f)
		{
		unsigned* poly = &h.polyStore[9 * f];
		poly[0] = p.faces[f][0];
		poly[1] = (unsigned) (size_t) &h.refStore[p.faces[f][1]];
		poly[2] = 0;
		for(unsigned k = 0; k < 6; ++k)
			poly[3 + k] = p.faces[f][2 + k];
		}
	h.hull[0] = 0xcdcd8000u;
	h.hull[3] = p.nbVerts;
	h.hull[4] = (unsigned) (size_t) p.verts;
	// The centre: the origin, which every polytope surrounds.
	h.hull[9] = p.nbPolygons;
	h.hull[10] = (unsigned) (size_t) h.polyStore;
	h.hull[11] = (unsigned) (size_t) h.refStore;
	h.hull[25] = (unsigned) (size_t) graph;
	memset(h.mesh, 0, sizeof(h.mesh));
	const void* table = s.oracle ? (const void*) (s.base + kPolygonTableRva) : (const void*) gTriangleMeshPolygonTable;
	*(const void**) (h.mesh + 0x04) = table;
	*(unsigned**) (h.mesh + 0xa0) = h.hull;
	*(unsigned*) (h.mesh + 0xa4) = 0xcdcda4a4u;
	const void* const* slots = (const void* const*) table;
	nxSetControl(kControlDefault);
	nx2gCall0(slots[6], h.mesh + 4);		// 002227: 001502 (the polygons are set, so no 001472)
	h.hasMap = mapSubdiv != 0;
	if(h.hasMap)
		{
		for(unsigned i = 0; i < 6; ++i)
			h.map[i] = 0xcdcdc000u + i;
		nx2gCall1(s.oracle ? (const void*) (s.base + kVertexMapCtorRva) : (const void*) &nxSupportMapVertexConstruct,
			h.map, (unsigned) (size_t) h.hull);
		nx2gCall1(s.oracle ? (const void*) (s.base + kSupportMapInitRva) : (const void*) &nxSupportMapInit,
			h.map, mapSubdiv);
		*(unsigned**) (h.mesh + 0xa8) = h.map;
		}
	}

// Oracle-side counts of one pair family.
struct Nx2gPairStats
	{
	unsigned pairs, pairsWithContacts, contacts, headers, mapPairs, graphPairs, nullHolders, stampWraps, axes,
		splitPairs;
	};

// One pair family over the hulls of `sides`: each pair under both control words,
// the streams and the scratch records compared, a pose with a denormal word under
// 0x0f7f split off (see contact_convex_convex). `polytopes` draws translations
// near the origin, which the polytopes surround; the boxes keep their draws.
static unsigned nx2gRunPairs(unsigned char* base, Nx2gSide* sides, unsigned nbHulls, unsigned seed, unsigned nbPairs,
	bool polytopes, NxDigest* inputDigest, NxDigest* oracleDigest, NxDigest* candidateDigest, NxDigest* splitOracle,
	NxDigest* splitCandidate, unsigned* perMode, unsigned* splitWords, unsigned* splitRuns, Nx2gPairStats& st)
	{
	typedef void(__cdecl* NxOracleContactFn)(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*);
	NxOracleContactFn oracleContact = (NxOracleContactFn) (base + kConvexConvexRva);
	static NxContactWorld world[2];
	memset(&st, 0, sizeof(st));
	for(unsigned i = 0; i < nbPairs; ++i)
		{
		unsigned local = seed ^ (i * 0x9e3779b9u + 1u);
		const unsigned hullDraw0 = nxNext(&local);
		const unsigned hullDraw1 = nxNext(&local);
		// Half the pairs are one hull twice: kind 0 the same pose but for one
		// translation word, kind 1 the same translation under another rotation;
		// the rest are two hulls on words of rows 0, 3 and 5 up to 2.
		const unsigned pairKind = nxNext(&local) % 4;
		const unsigned h0 = hullDraw0 % nbHulls;
		const unsigned h1 = pairKind < 2 ? h0 : hullDraw1 % nbHulls;
		unsigned rotation0[9], rotation1[9], translation0[3], translation1[3];
		nx2gRotation(&local, rotation0);
		nx2gRotation(&local, rotation1);
		const unsigned ownAxis = nxNext(&local) % 3;
		for(unsigned k = 0; k < 3; ++k)
			{
			const unsigned pick0 = nxNext(&local);
			const unsigned pick1 = nxNext(&local);
			const unsigned table0 = kNearRows2g[nxNext(&local) % 3];
			const unsigned table1 = kNearRows2g[nxNext(&local) % 3];
			if(polytopes)
				{
				// Words of rows 0, 3 and 5 up to 1.5, signed, one in sixteen a drawn
				// finite word: the polytopes are about two units across.
				translation0[k] = (pick0 & 15) == 0 ? nx2gMidWord(&local)
					: kLattice2g[table0][(pick0 >> 4) % 2] | ((pick0 >> 8) & 0x80000000u);
				translation1[k] = (pick1 & 15) == 0 ? nx2gMidWord(&local)
					: kLattice2g[table1][(pick1 >> 4) % 3] | ((pick1 >> 8) & 0x80000000u);
				}
			else
				{
				translation0[k] = (pick0 & 15) == 0 ? nx2gMidWord(&local)
					: kLattice2g[table0][(pick0 >> 4) % 3] | ((pick0 >> 8) & 0x80000000u);
				translation1[k] = (pick1 & 15) == 0 ? nx2gMidWord(&local)
					: (pick1 & 3) == 1 ? kLattice2g[table1][(pick1 >> 4) % 5] | ((pick1 >> 8) & 0x80000000u)
					: kLattice2g[table1][(pick1 >> 4) % 3] | ((pick1 >> 8) & 0x80000000u);
				}
			if((pairKind == 0 && k != ownAxis) || pairKind == 1)
				translation1[k] = translation0[k];
			}
		// Kind 0: the same rotation too, so the two hulls are one hull shifted along
		// one axis (face against face, or apart).
		if(pairKind == 0)
			memcpy(rotation1, rotation0, sizeof(rotation1));
		unsigned boxWords[2][6];
		for(unsigned b = 0; b < 2; ++b)
			for(unsigned k = 0; k < 6; ++k)
				{
				const unsigned pick = nxNext(&local);
				const unsigned sign = k < 3 ? 0x80000000u : 0u;
				boxWords[b][k] = (pick & 3) == 0 ? (0x7149f2cau | sign)
					: (pick & 3) == 1 ? nx2gMidWord(&local) : (kLattice2g[0][pick % 5] | (((pick >> 8) & 1) ? sign : 0u));
				}
		const unsigned flags0 = nxNext(&local);
		const unsigned flags1 = nxNext(&local);
		const unsigned holderDraw = nxNext(&local);
		const bool nullHolder0 = holderDraw % 8 == 1;
		const bool nullHolder1 = holderDraw % 8 == 2;
		const NxU32 material0 = nxNext(&local) & 0xff;
		const NxU32 material1 = nxNext(&local) & 0xff;
		const bool orient = (nxNext(&local) & 1) != 0;
		const unsigned stampDraw = nxNext(&local);
		const unsigned stamp = (stampDraw & 15) == 0 ? 0xfffffffeu : stampDraw & 0xffffu;
		const unsigned orientWord = orient ? 1u : 0u;
		nxFoldInput(inputDigest, &h0, 4);
		nxFoldInput(inputDigest, &h1, 4);
		nxFoldInput(inputDigest, rotation0, sizeof(rotation0));
		nxFoldInput(inputDigest, rotation1, sizeof(rotation1));
		nxFoldInput(inputDigest, translation0, sizeof(translation0));
		nxFoldInput(inputDigest, translation1, sizeof(translation1));
		nxFoldInput(inputDigest, boxWords, sizeof(boxWords));
		nxFoldInput(inputDigest, &flags0, 4);
		nxFoldInput(inputDigest, &flags1, 4);
		nxFoldInput(inputDigest, &holderDraw, 4);
		nxFoldInput(inputDigest, &material0, 4);
		nxFoldInput(inputDigest, &material1, 4);
		nxFoldInput(inputDigest, &orientWord, 4);
		nxFoldInput(inputDigest, &stamp, 4);
		bool poseDenormal = false;
		for(unsigned k = 0; k < 9; ++k)
			poseDenormal |= ((rotation0[k] | rotation1[k]) & 0x7f800000u) != 0x7f800000u
				&& (((rotation0[k] & 0x7f800000u) == 0 && (rotation0[k] & 0x007fffffu))
					|| ((rotation1[k] & 0x7f800000u) == 0 && (rotation1[k] & 0x007fffffu)));
		++st.pairs;
		if(sides[0].hulls[h0].hasMap || sides[0].hulls[h1].hasMap)
			++st.mapPairs;
		if(!sides[0].hulls[h0].hasMap || !sides[0].hulls[h1].hasMap)
			++st.graphPairs;
		if(nullHolder0 || nullHolder1)
			++st.nullHolders;
		for(int mode = 0; mode < 2; ++mode)
			{
			for(int side = 0; side < 2; ++side)
				{
				Nx2gSide& s = sides[side];
				static unsigned char store[2][2][kShapeBytes];
				unsigned char* shape[2] = { store[side][0], store[side][1] };
				for(unsigned b = 0; b < 2; ++b)
					{
					nxIdentity((NxCollisionShape*) shape[b]);
					memcpy(((NxCollisionShape*) shape[b])->rotation, b ? rotation1 : rotation0, 36);
					memcpy(((NxCollisionShape*) shape[b])->translation, b ? translation1 : translation0, 12);
					((NxCollisionShape*) shape[b])->type = 4;
					*(unsigned char**) (shape[b] + 0xe0) = s.hulls[b ? h1 : h0].mesh;
					shape[b][0xac] = 2;								// Prunable flags: no refresh
					*(unsigned**) (shape[b] + 0xc4) = s.pruner;		// the pruner
					*(unsigned short*) (shape[b] + 0xcc) = (unsigned short) b;	// the handle
					shape[b][0xde] = (unsigned char) ((b ? flags1 : flags0) & 0x3f);
					memcpy(s.boxes[b], boxWords[b], 24);
					}
				nxResetWorld(&world[side]);
				nxStageWorld(&world[side], (NxCollisionShape*) shape[0], (NxCollisionShape*) shape[1],
					true, true, material0, material1, nullHolder0, nullHolder1, orient);
				*(unsigned*) (s.scratch + 0x14) = stamp;
				}
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			oracleContact(world[0].plane, world[0].sphere, &world[0].sink, sides[0].scratch);
			NxContactConvexConvex(world[1].plane, world[1].sphere, &world[1].sink, sides[1].scratch);
			nxSetControl(kControlDefault);
			// The split (a rule on the fixed input): a pose with a denormal word, run
			// under 0x0f7f. There the candidate's 001653 relative poses can differ in
			// the last places, through the vendored InvertPRMatrix it calls (005191;
			// with the oracle's bound in, none do: the bind patch in the evidence).
			const bool split = mode == 1 && poseDenormal;
			nxFoldStream(split ? splitOracle : oracleDigest, &world[0]);
			nxFoldStream(split ? splitCandidate : candidateDigest, &world[1]);
			nx2gFoldScratch(split ? splitOracle : oracleDigest, sides[0].scratch);
			nx2gFoldScratch(split ? splitCandidate : candidateDigest, sides[1].scratch);
			const unsigned differing = nxCompareStreams(&world[0], &world[1], mode)
				+ nx2gCompareScratch(sides[0].scratch, sides[1].scratch);
			if(split)
				{
				*splitWords += differing;
				if(differing)
					++*splitRuns;
				}
			else
				perMode[mode] += differing;
			if(mode == 1 && poseDenormal)
				++st.splitPairs;
			if(mode == 0)
				{
				if(world[0].sink.contactCount)
					++st.pairsWithContacts;
				st.contacts += world[0].sink.contactCount;
				st.headers += world[0].stream[0];
				const unsigned after = *(const unsigned*) (sides[0].scratch + 0x14);
				if(stamp > 0xfffffff0u && after < stamp)
					++st.stampWraps;
				st.axes += ((const unsigned*) (sides[0].scratch + 0x4e0))[1]
					+ ((const unsigned*) (sides[0].scratch + 0x4f0))[1];
				}
			}
		}
	return 0;
	}

static void nx2gInitScratch(Nx2gSide& s)
	{
	memset(s.scratch, 0, sizeof(s.scratch));
	*(unsigned*) (s.scratch + 0x04) = 64;
	*(unsigned**) (s.scratch + 0x08) = s.visited;
	memset(s.visited, 0, sizeof(s.visited));
	nx2gContainerCtor(s, s.scratch + 0x4e0);
	nx2gContainerCtor(s, s.scratch + 0x4f0);
	memset(s.pruner, 0, sizeof(s.pruner));
	s.pruner[5] = (unsigned) (size_t) s.boxes;	// +0x14, the world boxes
	}

static __declspec(noinline) unsigned nxDriveTask2g(unsigned char* base)
	{
	unsigned total = 0;

	// -----------------------------------------------------------------------
	// contact_emit_ext: phys_fn_000875.
	{
	typedef void(__thiscall* NxOracleEmitExtFn)(NxContactSink*, void*, void*, NxU32, const NxVec3*,
		const NxVec3*, NxU32, NxU32, NxU32, NxU32);
	NxOracleEmitExtFn oracleEmit = (NxOracleEmitExtFn) (base + kEmitFeaturesRva);
	static NxContactWorld world[2];
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	unsigned perMode[2] = { 0, 0 };
	unsigned grown = 0;
	unsigned calls = 0, headers = 0, flagIds = 0, flagWords = 0, wideWords = 0, swapped = 0, repeatedNormal = 0,
		inputSnan = 0;
	unsigned state = 0x2e875000u;
	for(unsigned i = 0; i < kEmitExtIterations; ++i)
		{
		const unsigned sequenceSeed = nxNext(&state);
		const unsigned pairsDraw = nxNext(&state);
		const unsigned pairs = 1 + pairsDraw % 4;
		// One sequence in eight streams into a Container each side builds with its
		// own constructor and SetSize(4): the fourth word fills it, so 000875 grows
		// it through the side's own 004840 (Resize), as a nearly full stream would.
		const bool growth = i % 8 == 5;
		nxFoldInput(&inputDigest, &pairs, 4);
		for(int mode = 0; mode < 2; ++mode)
			{
			unsigned growthStore[2][4];
			for(int side = 0; side < 2; ++side)
				{
				nxResetWorld(&world[side]);
				if(growth)
					{
					// The sink's +0x38 is a Container: capacity, count, entries, growth.
					if(side == 0)
						{
						nx2gCall0(base + kIceContainerCtorRva, growthStore[side]);
						nx2gCall1(base + kIceContainerSetSizeRva, growthStore[side], 4);
						}
					else
						{
						new(growthStore[side]) IceCore::Container;
						((IceCore::Container*) growthStore[side])->SetSize(4);
						}
					NxU32* entries = (NxU32*) (size_t) growthStore[side][2];
					entries[0] = 0;
					world[side].sink.streamCapacity = growthStore[side][0];
					world[side].sink.streamCount = 1;
					world[side].sink.stream = entries;
					memcpy(world[side].sink.gap1, &growthStore[side][3], 4);
					}
				}
			unsigned local = sequenceSeed;
			NxVec3 normal;
			memset(&normal, 0, sizeof(normal));
			for(unsigned p = 0; p < pairs; ++p)
				{
				static unsigned char store0[kShapeBytes];
				static unsigned char store1[kShapeBytes];
				NxCollisionShape* s0 = (NxCollisionShape*) store0;
				NxCollisionShape* s1 = (NxCollisionShape*) store1;
				nxIdentity(s0);
				nxIdentity(s1);
				const unsigned flags0 = nxNext(&local);
				const unsigned flags1 = nxNext(&local);
				store0[0xde] = (unsigned char) (flags0 & 0x3f);
				store1[0xde] = (unsigned char) (flags1 & 0x3f);
				const unsigned idDraw0 = nxNext(&local);
				const unsigned idDraw1 = nxNext(&local);
				const unsigned junk0 = nxNext(&local);
				const unsigned junk1 = nxNext(&local);
				const NxU32 id0 = ((idDraw0 & 1) ? ((idDraw0 >> 1) & 0x7fff) : 0xffffu) | (junk0 & 0xffff0000u);
				const NxU32 id1 = ((idDraw1 & 1) ? ((idDraw1 >> 1) & 0x7fff) : 0xffffu) | (junk1 & 0xffff0000u);
				const unsigned wordDraw0 = nxNext(&local);
				const unsigned wordDraw1 = nxNext(&local);
				const unsigned wordSize = nxNext(&local);
				const NxU32 word0 = (wordSize & 3) == 0 ? wordDraw0 : wordDraw0 & 0xffffu;
				const NxU32 word1 = (wordSize & 12) == 0 ? wordDraw1 : wordDraw1 & 0xffffu;
				NxVec3 point;
				for(int k = 0; k < 3; ++k)
					nxPickRawWord(&local, &(&point.x)[k]);
				const unsigned repeat = nxNext(&local);
				if(p == 0 || (repeat & 3) != 0)
					for(int k = 0; k < 3; ++k)
						nxPickRawWord(&local, &(&normal.x)[k]);
				else if(mode == 0)
					++repeatedNormal;
				const NxU32 separationBits = nxNext(&local);
				const bool newIdentity0 = (p == 0) || ((nxNext(&local) & 3) == 0);
				const bool newIdentity1 = (p == 0) || ((nxNext(&local) & 3) == 0);
				const NxU32 material0 = nxNext(&local) & 0xff;
				const NxU32 material1 = nxNext(&local) & 0xff;
				const bool nullHolder1 = (nxNext(&local) & 7) == 0;
				const bool orient = (nxNext(&local) & 1) != 0;
				for(int side = 0; side < 2; ++side)
					nxStageWorld(&world[side], s0, s1, newIdentity0, newIdentity1, material0, material1,
						false, nullHolder1, orient);
				if(mode == 0)
					{
					nxFoldInput(&inputDigest, store0 + 0xde, 1);
					nxFoldInput(&inputDigest, store1 + 0xde, 1);
					nxFoldInput(&inputDigest, &id0, 4);
					nxFoldInput(&inputDigest, &id1, 4);
					nxFoldInput(&inputDigest, &word0, 4);
					nxFoldInput(&inputDigest, &word1, 4);
					nxFoldInput(&inputDigest, &point, sizeof(point));
					nxFoldInput(&inputDigest, &normal, sizeof(normal));
					nxFoldInput(&inputDigest, &separationBits, 4);
					const unsigned staging[6] = { newIdentity0 ? 1u : 0u, newIdentity1 ? 1u : 0u, material0,
						material1, nullHolder1 ? 1u : 0u, orient ? 1u : 0u };
					nxFoldInput(&inputDigest, staging, sizeof(staging));
					for(int k = 0; k < 3; ++k)
						{
						const NxU32 pw = nxBits((&point.x)[k]), nw = nxBits((&normal.x)[k]);
						inputSnan += ((pw & 0x7fc00000u) == 0x7f800000u && (pw & 0x3fffffu)) ? 1 : 0;
						inputSnan += ((nw & 0x7fc00000u) == 0x7f800000u && (nw & 0x3fffffu)) ? 1 : 0;
						}
					}
				const NxU32 countBefore = world[0].sink.streamCount;
				const void* lastBefore = world[0].sink.lastObject1;
				nxSetControl(mode ? kControlSimulate : kControlDefault);
				oracleEmit(&world[0].sink, world[0].sphere->collisionObject, world[0].plane->collisionObject,
					separationBits, &point, &normal, id0, id1, word0, word1);
				NxEmitContactFeatures(&world[1].sink, 0, world[1].sphere->collisionObject,
					world[1].plane->collisionObject, separationBits, &point, &normal, id0, id1, word0, word1);
				nxSetControl(kControlDefault);
				if(mode == 0)
					{
					++calls;
					const NxU32 flag = world[0].sink.featurePairValid;
					if(world[0].sink.lastObject1 != lastBefore || countBefore == 1)
						++headers;
					if(flag & 1)
						++flagIds;
					if(flag & 4)
						{
						++flagWords;
						if(word0 > 0xffffu || word1 > 0xffffu)
							++wideWords;
						}
					if(orient)
						++swapped;
					}
				}
			if(growth)
				for(int side = 0; side < 2; ++side)
					{
					// The grown stream back into the world's array for the comparison,
					// then the Container released by the side's own destructor.
					NxContactSink& sink = world[side].sink;
					if(side == 0 && mode == 0 && sink.streamCapacity > 4)
						++grown;
					memcpy(world[side].stream, sink.stream, 4 * sink.streamCount);
					growthStore[side][0] = sink.streamCapacity;
					growthStore[side][1] = sink.streamCount;
					growthStore[side][2] = (unsigned) (size_t) sink.stream;
					memcpy(&growthStore[side][3], sink.gap1, 4);
					world[side].sink.stream = world[side].stream;
					if(side == 0)
						nx2gCall0(base + kIceContainerDtorRva, growthStore[side]);
					else
						((IceCore::Container*) growthStore[side])->~Container();
					}
			nxFoldStream(&oracleDigest, &world[0]);
			nxFoldStream(&candidateDigest, &world[1]);
			perMode[mode] += nxCompareStreams(&world[0], &world[1], mode);
			}
		}
	total += perMode[0] + perMode[1];
	printf("collision name=contact_emit_ext index=- rva=0x%08x owner=phys_fn_000875 checks=%u oracle=%016llx candidate=%016llx mismatches=%u default_mismatches=%u simulate_mismatches=%u\n",
		kEmitFeaturesRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state, perMode[0] + perMode[1],
		perMode[0], perMode[1]);
	nxPrintInput("contact_emit_ext", &inputDigest);
	printf("collision coverage name=contact_emit_ext calls=%u headers=%u flag_ids=%u flag_words=%u wide_words=%u swapped=%u repeated_normal=%u input_snan=%u grown=%u\n",
		calls, headers, flagIds, flagWords, wideWords, swapped, repeatedNormal, inputSnan, grown);
	}

	// -----------------------------------------------------------------------
	// contact_convex_convex: phys_fn_001820 over the box hulls.
	{
	static Nx2gSide sides[2];
	static Nx2gBox boxes[kNb2gHulls];
	static Nx2gGraph graphs[kNb2gHulls];
	NxDigest oracleDigest, candidateDigest, inputDigest, splitOracle, splitCandidate;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	nxDigestInit(&splitOracle);
	nxDigestInit(&splitCandidate);

	unsigned state = 0x2e820000u;
	unsigned mapSubdiv[kNb2gHulls];
	for(unsigned h = 0; h < kNb2gHulls; ++h)
		{
		nx2gBuildBox(&state, boxes[h], h >= kNb2gHulls - 2);
		nx2gBuildGraph(boxes[h], graphs[h]);
		static const unsigned kSubdiv[4] = { 1, 2, 3, 5 };
		const unsigned subdivDraw = nxNext(&state);
		mapSubdiv[h] = (h & 1) ? kSubdiv[subdivDraw % 4] : 0;
		nxFoldInput(&inputDigest, boxes[h].verts, sizeof(boxes[h].verts));
		nxFoldInput(&inputDigest, boxes[h].tris, sizeof(boxes[h].tris));
		nxFoldInput(&inputDigest, boxes[h].centre, sizeof(boxes[h].centre));
		nxFoldInput(&inputDigest, &mapSubdiv[h], 4);
		}
	unsigned builtPolygons = 0, builtEdges = 0;
	for(int side = 0; side < 2; ++side)
		{
		Nx2gSide& s = sides[side];
		s.oracle = side == 0;
		s.base = base;
		for(unsigned h = 0; h < kNb2gHulls; ++h)
			nx2gBuildHull(s, s.hulls[h], boxes[h], &graphs[h], mapSubdiv[h]);
		nx2gInitScratch(s);
		}
	// The two sides' builds, compared once (discrete words).
	unsigned buildMismatches = 0;
	for(unsigned h = 0; h < kNb2gHulls; ++h)
		{
		const unsigned* a = sides[0].hulls[h].hull;
		const unsigned* b = sides[1].hulls[h].hull;
		buildMismatches += nx2gCompareEdges(a, b);
		builtPolygons += a[9];
		builtEdges += a[14];
		}
	unsigned splitWords = 0, splitRuns = 0;
	unsigned perMode[2] = { 0, 0 };
	Nx2gPairStats st;
	nx2gRunPairs(base, sides, kNb2gHulls, 0x2e821000u, kConvexPairs, false, &inputDigest, &oracleDigest,
		&candidateDigest, &splitOracle, &splitCandidate, perMode, &splitWords, &splitRuns, st);
	for(int side = 0; side < 2; ++side)
		{
		Nx2gSide& s = sides[side];
		for(unsigned h = 0; h < kNb2gHulls; ++h)
			nx2gReleaseHull(s, s.hulls[h]);
		nx2gContainerDtor(s, s.scratch + 0x4e0);
		nx2gContainerDtor(s, s.scratch + 0x4f0);
		}
	const bool splitOver = splitWords > kConvexPoseDivergentWords || splitRuns > kConvexPoseDivergentRuns;
	total += perMode[0] + perMode[1] + buildMismatches + (splitOver ? 1 : 0);
	printf("collision name=contact_convex_convex index=- rva=0x%08x owner=phys_fn_001820 checks=%u oracle=%016llx candidate=%016llx mismatches=%u default_mismatches=%u simulate_mismatches=%u build_mismatches=%u\n",
		kConvexConvexRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state,
		perMode[0] + perMode[1] + buildMismatches, perMode[0], perMode[1], buildMismatches);
	printf("collision name=contact_convex_convex.pose_divergent index=- rva=0x%08x owner=phys_fn_001820 checks=%u oracle=%016llx candidate=%016llx words=%u runs=%u ceiling_words=%u ceiling_runs=%u ceiling=%s\n",
		kConvexConvexRva, splitOracle.checks, splitOracle.state, splitCandidate.state, splitWords, splitRuns,
		kConvexPoseDivergentWords, kConvexPoseDivergentRuns, splitOver ? "exceeded" : "ok");
	nxPrintInput("contact_convex_convex", &inputDigest);
	printf("collision coverage name=contact_convex_convex hulls=%u polygons=%u edges=%u pairs=%u pairs_with_contacts=%u contacts=%u headers=%u map_pairs=%u graph_pairs=%u null_holders=%u stamp_wraps=%u axes=%u split_pairs=%u\n",
		kNb2gHulls, builtPolygons, builtEdges, st.pairs, st.pairsWithContacts, st.contacts, st.headers, st.mapPairs,
		st.graphPairs, st.nullHolders, st.stampWraps, st.axes, st.splitPairs);
	}

	// -----------------------------------------------------------------------
	// contact_convex_hulls: phys_fn_001820 over the polytopes (Task 2g review):
	// triangular, hexagonal and tall pentagonal prisms, an octahedron and an
	// icosphere of 80 triangles, each with and without a kind C map, so the
	// polygons 001909 clips are triangles, quads, pentagons and hexagons, and the
	// face pairs are not parallel.
	{
	static Nx2gSide sides[2];
	static Nx2gGraph graphs[5];
	NxDigest oracleDigest, candidateDigest, inputDigest, splitOracle, splitCandidate;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	nxDigestInit(&splitOracle);
	nxDigestInit(&splitCandidate);
	static const unsigned kSubdiv[5] = { 1, 2, 3, 5, 8 };
	for(unsigned k = 0; k < 5; ++k)
		{
		nx2gBuildPolytopeGraph(kPolytopes2g[k], graphs[k]);
		nxFoldInput(&inputDigest, kPolytopes2g[k].verts, 12 * kPolytopes2g[k].nbVerts);
		nxFoldInput(&inputDigest, kPolytopes2g[k].refs, 4 * kPolytopes2g[k].nbRefs);
		nxFoldInput(&inputDigest, kPolytopes2g[k].faces, 32 * kPolytopes2g[k].nbPolygons);
		nxFoldInput(&inputDigest, &kSubdiv[k], 4);
		}
	unsigned polygons = 0, edges = 0, buildMismatches = 0;
	for(int side = 0; side < 2; ++side)
		{
		Nx2gSide& s = sides[side];
		s.oracle = side == 0;
		s.base = base;
		for(unsigned h = 0; h < kNb2gPolytopeHulls; ++h)
			nx2gBuildPolytope(s, s.hulls[h], kPolytopes2g[h % 5], &graphs[h % 5], h >= 5 ? kSubdiv[h % 5] : 0);
		nx2gInitScratch(s);
		}
	for(unsigned h = 0; h < kNb2gPolytopeHulls; ++h)
		{
		const unsigned* a = sides[0].hulls[h].hull;
		const unsigned* b = sides[1].hulls[h].hull;
		buildMismatches += nx2gCompareEdges(a, b);
		polygons += a[9];
		edges += a[14];
		}
	unsigned splitWords = 0, splitRuns = 0;
	unsigned perMode[2] = { 0, 0 };
	Nx2gPairStats st;
	nx2gRunPairs(base, sides, kNb2gPolytopeHulls, 0x2e823000u, kConvexHullPairs, true, &inputDigest, &oracleDigest,
		&candidateDigest, &splitOracle, &splitCandidate, perMode, &splitWords, &splitRuns, st);
	for(int side = 0; side < 2; ++side)
		{
		Nx2gSide& s = sides[side];
		for(unsigned h = 0; h < kNb2gPolytopeHulls; ++h)
			nx2gReleaseHull(s, s.hulls[h]);
		nx2gContainerDtor(s, s.scratch + 0x4e0);
		nx2gContainerDtor(s, s.scratch + 0x4f0);
		}
	const bool splitOver = splitWords > kConvexHullsPoseDivergentWords || splitRuns > kConvexHullsPoseDivergentRuns;
	total += perMode[0] + perMode[1] + buildMismatches + (splitOver ? 1 : 0);
	printf("collision name=contact_convex_hulls index=- rva=0x%08x owner=phys_fn_001820 checks=%u oracle=%016llx candidate=%016llx mismatches=%u default_mismatches=%u simulate_mismatches=%u build_mismatches=%u\n",
		kConvexConvexRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state,
		perMode[0] + perMode[1] + buildMismatches, perMode[0], perMode[1], buildMismatches);
	printf("collision name=contact_convex_hulls.pose_divergent index=- rva=0x%08x owner=phys_fn_001820 checks=%u oracle=%016llx candidate=%016llx words=%u runs=%u ceiling_words=%u ceiling_runs=%u ceiling=%s\n",
		kConvexConvexRva, splitOracle.checks, splitOracle.state, splitCandidate.state, splitWords, splitRuns,
		kConvexHullsPoseDivergentWords, kConvexHullsPoseDivergentRuns, splitOver ? "exceeded" : "ok");
	nxPrintInput("contact_convex_hulls", &inputDigest);
	printf("collision coverage name=contact_convex_hulls hulls=%u polygons=%u edges=%u pairs=%u pairs_with_contacts=%u contacts=%u headers=%u map_pairs=%u graph_pairs=%u null_holders=%u stamp_wraps=%u axes=%u split_pairs=%u\n",
		kNb2gPolytopeHulls, polygons, edges, st.pairs, st.pairsWithContacts, st.contacts, st.headers, st.mapPairs,
		st.graphPairs, st.nullHolders, st.stampWraps, st.axes, st.splitPairs);
	}
	return total;
	}

// ---------------------------------------------------------------------------
// convex-mesh gap Task 2h (units/convex-mesh-gap-contract.md: sub-unit M's first
// half, ContactConvexHeightfield.cpp). The rows' callers, 001844 and 001849, are
// Task 2i's, so each entry row is driven here at its own address through a
// register thunk (nx2hCall) with the registers and caller-cleaned stack 001849 /
// 001844 give it, over each side's own images: the hulls of the Task 2g fixture
// (the small box hulls each side builds with its own 001472 / 001502, and the
// hand-built polytopes, with and without a kind C map), TriangleMesh images of
// eight fixed triangle meshes (a height-field-like terrain, a flat grid, a roof, a
// valley, a closed pyramid, one tilted triangle, a strip with a zero-area and a
// collinear triangle, a 30-vertex terrain: +0x0c, +0x10, +0x14; +0x88
// the EdgeList each side's own 002188 builds the first time 001834 needs it),
// and a scratch record (+0x04 / +0x08 a visited array, +0x14 the stamp, the two
// Containers at +0x4e0 / +0x4f0 built by the side's own constructor). Every case
// runs under both control words; every float input is a word written as bits.
//
//   convex_mesh_ray       001822 (with 001472 on a hull whose polygons are not
//                         built yet, 001708 and the vendored Matrix4x4::Invert)
//                         over rays from inside and outside the hulls, with no
//                         pose, a signed-permutation pose, a dyadic one (exact
//                         inverse) and a general one.
//   convex_mesh_faces     001832 (001830, 001828, 001826, 001824, slot 11) over
//                         a group of a mesh's triangles and a point in the hull's
//                         frame: inside (every face faces away: 001828's arm),
//                         on the surface and drawn.
//   convex_mesh_edges     001840 (001833, 001826, 001834 with 002188 and 001661)
//                         over the same kind of group, a polygon and a triangle.
//   convex_mesh_cross     001836 with 001838 (001661, 001826) right after it in
//                         the same case, on the edge directions 001840 left in
//                         +0x4e0 (each side its own).
//   convex_mesh_contacts  001842 (001909, 001903, 001907, 000875) over a hull
//                         polygon and a triangle, staged shapes and a sink as in
//                         contact_convex_convex.
//
// The poses are 4x4s (rows +0x00, +0x10, +0x20 with a zero fourth column, the
// translation at +0x30 and 1.0f). When the rotation is a signed permutation the
// hull-to-mesh pose's inverse is written word for word (the transpose, and each
// translation word taken from the row it lands on with its sign bit flipped:
// -(t . r_j) with one nonzero term), so the two relative poses agree without any
// arithmetic here; the other rotations (about an axis by fixed (cos, sin) words,
// and table words) get the transpose with a drawn translation.
//
// Fixed-input rules (the oracle would read memory it does not own): every
// triangle index, polygon index and vertex reference is in range (drawn modulo
// the counts); 001840's polygon index is an input, never 001832's -1; the
// contact family's words are finite (001909 is not driven with a NaN normal).
// NaN, infinite, denormal and -0 words reach 001822's ray, 001832's point,
// 001840's normal and axis, and 001836's plane.
struct Nx2hMesh
	{
	const char*			name;
	unsigned			nbVerts, nbTris;
	const unsigned*		verts;
	const unsigned*		tris;
	const unsigned*		planes;		// per triangle: its unit normal and d = -n . v0 (offline, narrowed)
	};

// Generated offline (scratchpad genmesh.py): vertex words on a lattice of small
// integers and halves, 32-bit triangles counter-clockwise from above, and each
// triangle's plane words.
// terrain: 16 vertices, 18 triangles
static const unsigned kMeshVerts2h0[48] =
	{
	0x00000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x40c00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x40000000u, 0x40000000u, 0x3f800000u, 0x40800000u, 0x40000000u, 0x3f800000u, 0x40c00000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x40000000u, 0x40800000u, 0x3f800000u, 0x40800000u, 0x40800000u, 0x40000000u, 0x40c00000u, 0x40800000u, 0x3f000000u, 0x00000000u, 0x40c00000u, 0x00000000u, 0x40000000u, 0x40c00000u, 0x00000000u, 0x40800000u, 0x40c00000u, 0x3f000000u, 0x40c00000u, 0x40c00000u, 0x00000000u
	};
static const unsigned kMeshTris2h0[54] =
	{
	0, 1, 4, 1, 5, 4, 1, 2, 6, 1, 6, 5, 2, 3, 6, 3, 7, 6, 4, 5, 9, 4, 9, 8, 5, 6, 9, 6, 10, 9, 6, 7, 11, 6, 11, 10, 8, 9, 12, 9, 13, 12, 9, 10, 14, 9, 14, 13, 10, 11, 14, 11, 15, 14
	};
static const unsigned kMeshPlanes2h0[72] =
	{
	0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0xbed105ecu, 0xbed105ecu, 0x3f5105ecu, 0x3f5105ecu, 0x00000000u, 0xbee4f92eu, 0x3f64f92eu, 0x80000000u, 0x00000000u, 0xbee4f92eu, 0x3f64f92eu, 0x80000000u, 0x00000000u, 0xbee4f92eu, 0x3f64f92eu, 0x80000000u, 0x3ee4f92eu, 0x80000000u, 0x3f64f92eu, 0xc02bbae2u, 0xbee4f92eu, 0x00000000u, 0x3f64f92eu, 0x80000000u, 0xbee4f92eu, 0x00000000u, 0x3f64f92eu, 0x80000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0xbf800000u, 0xbed105ecu, 0xbed105ecu, 0x3f5105ecu, 0x3fd105ecu, 0x3edf7483u, 0xbe5f7483u, 0x3f5f7483u, 0xc00ba8d2u, 0x3f0e9d30u, 0xbebe26ebu, 0x3f3e26ebu, 0xc00e9d30u, 0xbee4f92eu, 0x00000000u, 0x3f64f92eu, 0x80000000u, 0x00000000u, 0x3ee4f92eu, 0x3f64f92eu, 0xc02bbae2u, 0xbebe26ebu, 0x3f0e9d30u, 0x3f3e26ebu, 0xc00e9d30u, 0xbe5f7483u, 0x3edf7483u, 0x3f5f7483u, 0xc00ba8d2u, 0x3f03b5feu, 0x3f03b5feu, 0x3f2f9d53u, 0xc0af9d53u, 0x3e715befu, 0x3e715befu, 0x3f715befu, 0xc03504f3u
	};
// flat: 9 vertices, 8 triangles
static const unsigned kMeshVerts2h1[27] =
	{
	0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x41000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x40800000u, 0x40800000u, 0x00000000u, 0x41000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x41000000u, 0x00000000u, 0x40800000u, 0x41000000u, 0x00000000u, 0x41000000u, 0x41000000u, 0x00000000u
	};
static const unsigned kMeshTris2h1[24] =
	{
	0, 1, 4, 0, 4, 3, 1, 2, 5, 1, 5, 4, 3, 4, 7, 3, 7, 6, 4, 5, 8, 4, 8, 7
	};
static const unsigned kMeshPlanes2h1[32] =
	{
	0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u
	};
// roof: 6 vertices, 4 triangles
static const unsigned kMeshVerts2h2[18] =
	{
	0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x40000000u, 0x40800000u, 0x40000000u, 0x40000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x40800000u, 0x40800000u, 0x00000000u
	};
static const unsigned kMeshTris2h2[12] =
	{
	0, 1, 3, 0, 3, 2, 2, 3, 5, 2, 5, 4
	};
static const unsigned kMeshPlanes2h2[16] =
	{
	0x00000000u, 0xbf3504f3u, 0x3f3504f3u, 0x80000000u, 0x00000000u, 0xbf3504f3u, 0x3f3504f3u, 0x80000000u, 0x80000000u, 0x3f3504f3u, 0x3f3504f3u, 0xc03504f3u, 0x00000000u, 0x3f3504f3u, 0x3f3504f3u, 0xc03504f3u
	};
// valley: 6 vertices, 4 triangles
static const unsigned kMeshVerts2h3[18] =
	{
	0x00000000u, 0x00000000u, 0x3f800000u, 0x40800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x40800000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x3f800000u, 0x40800000u, 0x40800000u, 0x3f800000u
	};
static const unsigned kMeshTris2h3[12] =
	{
	0, 1, 3, 0, 3, 2, 2, 3, 5, 2, 5, 4
	};
static const unsigned kMeshPlanes2h3[16] =
	{
	0x80000000u, 0x3ee4f92eu, 0x3f64f92eu, 0xbf64f92eu, 0x00000000u, 0x3ee4f92eu, 0x3f64f92eu, 0xbf64f92eu, 0x00000000u, 0xbee4f92eu, 0x3f64f92eu, 0x3f64f92eu, 0x00000000u, 0xbee4f92eu, 0x3f64f92eu, 0x3f64f92eu
	};
// pyramid: 5 vertices, 6 triangles
static const unsigned kMeshVerts2h4[15] =
	{
	0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x40000000u, 0x40000000u, 0x40400000u
	};
static const unsigned kMeshTris2h4[18] =
	{
	0, 1, 4, 1, 2, 4, 2, 3, 4, 3, 0, 4, 0, 2, 1, 0, 3, 2
	};
static const unsigned kMeshPlanes2h4[24] =
	{
	0x00000000u, 0xbf550140u, 0x3f0e00d5u, 0x80000000u, 0x3f550140u, 0x80000000u, 0x3f0e00d5u, 0xc0550140u, 0x00000000u, 0x3f550140u, 0x3f0e00d5u, 0xc0550140u, 0xbf550140u, 0x00000000u, 0x3f0e00d5u, 0x80000000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x80000000u, 0x00000000u, 0x00000000u, 0xbf800000u, 0x80000000u
	};
// triangle: 3 vertices, 1 triangles
static const unsigned kMeshVerts2h5[9] =
	{
	0x00000000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x3f000000u, 0x00000000u, 0x40800000u, 0x3f800000u
	};
static const unsigned kMeshTris2h5[3] =
	{
	0, 1, 2
	};
static const unsigned kMeshPlanes2h5[4] =
	{
	0xbdf68cdcu, 0xbe768cdcu, 0x3f768cdcu, 0x80000000u
	};
// degenerate: 9 vertices, 7 triangles
static const unsigned kMeshVerts2h6[27] =
	{
	0x00000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x40000000u, 0x40000000u, 0x3f000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x40000000u, 0x3f800000u, 0x40c00000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x00000000u
	};
static const unsigned kMeshTris2h6[21] =
	{
	0, 1, 2, 1, 3, 2, 1, 4, 3, 4, 5, 6, 4, 6, 3, 5, 7, 6, 0, 8, 1
	};
static const unsigned kMeshPlanes2h6[28] =
	{
	0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0xbe715befu, 0xbe715befu, 0x3f715befu, 0x3ef15befu, 0x00000000u, 0xbe785b42u, 0x3f785b42u, 0x80000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0xbe5f7483u, 0xbedf7483u, 0x3f5f7483u, 0x3f5f7483u, 0x00000000u, 0xbee4f92eu, 0x3f64f92eu, 0x80000000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u
	};
// wide terrain: 30 vertices, 40 triangles
static const unsigned kMeshVerts2h7[90] =
	{
	0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f000000u, 0x40000000u, 0x00000000u, 0x3f800000u, 0x40400000u, 0x00000000u, 0x3f800000u, 0x40800000u, 0x00000000u, 0x3f000000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x40000000u, 0x3f800000u, 0x3fc00000u, 0x40400000u, 0x3f800000u, 0x40000000u, 0x40800000u, 0x3f800000u, 0x3f800000u, 0x40a00000u, 0x3f800000u, 0x00000000u, 0x00000000u, 0x40000000u, 0x3f000000u, 0x3f800000u, 0x40000000u, 0x3f800000u, 0x40000000u, 0x40000000u, 0x40000000u, 0x40400000u, 0x40000000u, 0x40000000u, 0x40800000u, 0x40000000u, 0x3fc00000u, 0x40a00000u, 0x40000000u, 0x3f000000u, 0x00000000u, 0x40400000u, 0x00000000u, 0x3f800000u, 0x40400000u, 0x3f000000u, 0x40000000u, 0x40400000u, 0x3f800000u, 0x40400000u, 0x40400000u, 0x3fc00000u, 0x40800000u, 0x40400000u, 0x3f800000u, 0x40a00000u, 0x40400000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x3f800000u, 0x40800000u, 0x00000000u, 0x40000000u, 0x40800000u, 0x3f000000u, 0x40400000u, 0x40800000u, 0x3f000000u, 0x40800000u, 0x40800000u, 0x00000000u, 0x40a00000u, 0x40800000u, 0x00000000u
	};
static const unsigned kMeshTris2h7[120] =
	{
	0, 1, 6, 1, 7, 6, 1, 2, 8, 1, 8, 7, 2, 3, 8, 3, 9, 8, 3, 4, 10, 3, 10, 9, 4, 5, 10, 5, 11, 10, 6, 7, 13, 6, 13, 12, 7, 8, 13, 8, 14, 13, 8, 9, 15, 8, 15, 14, 9, 10, 15, 10, 16, 15, 10, 11, 17, 10, 17, 16, 12, 13, 18, 13, 19, 18, 13, 14, 20, 13, 20, 19, 14, 15, 20, 15, 21, 20, 15, 16, 22, 15, 22, 21, 16, 17, 22, 17, 23, 22, 18, 19, 25, 18, 25, 24, 19, 20, 25, 20, 26, 25, 20, 21, 27, 20, 27, 26, 21, 22, 27, 22, 28, 27, 22, 23, 29, 22, 29, 28
	};
static const unsigned kMeshPlanes2h7[160] =
	{
	0xbee4f92eu, 0x00000000u, 0x3f64f92eu, 0x80000000u, 0xbf2aaaabu, 0xbeaaaaabu, 0x3f2aaaabu, 0x3eaaaaabu, 0xbed105ecu, 0xbed105ecu, 0x3f5105ecu, 0x80000000u, 0xbed105ecu, 0xbed105ecu, 0x3f5105ecu, 0x80000000u, 0x00000000u, 0xbee4f92eu, 0x3f64f92eu, 0xbf64f92eu, 0xbeaaaaabu, 0xbf2aaaabu, 0x3f2aaaabu, 0x3eaaaaabu, 0x3ed105ecu, 0xbed105ecu, 0x3f5105ecu, 0xc002a3b4u, 0x3f13cd3au, 0xbf13cd3au, 0x3f13cd3au, 0xc013cd3au, 0x3ed105ecu, 0xbed105ecu, 0x3f5105ecu, 0xc002a3b4u, 0x3f3504f3u, 0x80000000u, 0x3f3504f3u, 0xc0624630u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x80000000u, 0xbed105ecu, 0xbed105ecu, 0x3f5105ecu, 0x3ed105ecu, 0xbee4f92eu, 0x00000000u, 0x3f64f92eu, 0xbee4f92eu, 0xbf2aaaabu, 0xbeaaaaabu, 0x3f2aaaabu, 0x3f2aaaabu, 0xbee4f92eu, 0x00000000u, 0x3f64f92eu, 0xbee4f92eu, 0x00000000u, 0xbee4f92eu, 0x3f64f92eu, 0xbf64f92eu, 0x3f3504f3u, 0x80000000u, 0x3f3504f3u, 0xc0624630u, 0x3ed105ecu, 0xbed105ecu, 0x3f5105ecu, 0xc002a3b4u, 0x3f2aaaabu, 0xbeaaaaabu, 0x3f2aaaabu, 0xc0400000u, 0x3f2aaaabu, 0xbeaaaaabu, 0x3f2aaaabu, 0xc0400000u, 0xbed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0xbf9cc471u, 0xbed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0xbf9cc471u, 0xbf13cd3au, 0x3f13cd3au, 0x3f13cd3au, 0xbf93cd3au, 0xbed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0xbf9cc471u, 0x80000000u, 0x3f3504f3u, 0x3f3504f3u, 0xc03504f3u, 0xbed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0xbf9cc471u, 0x3ed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0xc06b26aau, 0x3ed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0xc06b26aau, 0x3f2aaaabu, 0x3eaaaaabu, 0x3f2aaaabu, 0xc08aaaabu, 0x3f2aaaabu, 0x3eaaaaabu, 0x3f2aaaabu, 0xc08aaaabu, 0xbed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0xbf9cc471u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0xbed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0xbf9cc471u, 0xbed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0xbf9cc471u, 0xbeaaaaabu, 0x3f2aaaabu, 0x3f2aaaabu, 0xc0000000u, 0x00000000u, 0x3ee4f92eu, 0x3f64f92eu, 0xc00f1bbdu, 0x3eaaaaabu, 0x3f2aaaabu, 0x3f2aaaabu, 0xc0800000u, 0x3eaaaaabu, 0x3f2aaaabu, 0x3f2aaaabu, 0xc0800000u, 0x3f3504f3u, 0x00000000u, 0x3f3504f3u, 0xc0624630u, 0x00000000u, 0x3f3504f3u, 0x3f3504f3u, 0xc03504f3u
	};
static const Nx2hMesh kMeshes2h[8] =
	{
	{ "terrain", 16, 18, kMeshVerts2h0, kMeshTris2h0, kMeshPlanes2h0 },
	{ "flat", 9, 8, kMeshVerts2h1, kMeshTris2h1, kMeshPlanes2h1 },
	{ "roof", 6, 4, kMeshVerts2h2, kMeshTris2h2, kMeshPlanes2h2 },
	{ "valley", 6, 4, kMeshVerts2h3, kMeshTris2h3, kMeshPlanes2h3 },
	{ "pyramid", 5, 6, kMeshVerts2h4, kMeshTris2h4, kMeshPlanes2h4 },
	{ "triangle", 3, 1, kMeshVerts2h5, kMeshTris2h5, kMeshPlanes2h5 },
	{ "degenerate", 9, 7, kMeshVerts2h6, kMeshTris2h6, kMeshPlanes2h6 },
	{ "wide terrain", 30, 40, kMeshVerts2h7, kMeshTris2h7, kMeshPlanes2h7 },
	};

static const unsigned kNb2hMeshes = sizeof(kMeshes2h) / sizeof(kMeshes2h[0]);

static const unsigned kConvexMeshRayRva = 0x00041360;			// phys_fn_001822
static const unsigned kConvexMeshFaceAxesRva = 0x000419b0;		// phys_fn_001832
static const unsigned kConvexMeshEdgeAxesRva = 0x00042460;		// phys_fn_001840
static const unsigned kConvexMeshCrossAxesRva = 0x00041fe0;		// phys_fn_001836 (and 001838)
static const unsigned kConvexMeshContactsRva = 0x00042560;		// phys_fn_001842
static const unsigned kEdgeListDtorRva = 0x000515d0;			// phys_fn_002060
static const unsigned kRay2hCases = 8000;
static const unsigned kSat2hCases = 6000;
static const unsigned kContact2hCases = 5000;
// convex_mesh_ray's split (see the family): the hulls (bit k) with a polygon fan
// on which the oracle's Triangle::Inflate (005185) and the candidate's vendored
// one differ, under 0x027f and under 0x0f7f; the (case, control word) runs whose
// pose the oracle's Matrix4x4::Invert (005197) and the candidate's vendored one
// invert to different words (the bitmap below, with its count and the digest of
// its run numbers) -- both frozen, both route the split, and the family's
// pre-flights guard them (Task 2h re-review: a run that diverges outside a list
// fails; one on a list that stops diverging stays split and is logged IMPROVED)
// -- and the split's ceiling: the differing words and runs
// measured when it was registered (a count may fall, never rise).
static const unsigned kRay2hInflateDivergent[2] = { 0x00252000u, 0x002f7000u };
static const unsigned kRay2hInvertDivergentRuns = 2963;
static const unsigned __int64 kRay2hInvertDivergentDigest = 0x773d5c7f92630fd0ull;
// The runs (2 * case + control word, 0 for 0x027f) whose pose the oracle's
// Matrix4x4::Invert (005197) and the candidate's vendored one invert to different
// words: a frozen bitmap of the 16,000 runs (Task 2h re-review), which routes
// convex_mesh_ray's split. Its run count and the digest of its run numbers in
// order are kRay2hInvertDivergentRuns / Digest, checked before the family runs.
static const unsigned kRay2hInvertDivergentMap[500] =
	{
	0x03000c33u, 0x080240c0u, 0x0000030cu, 0x04c00000u, 0x00031303u, 0x00ec3c34u, 0x0030c004u, 0x0c000330u,
	0x3500c033u, 0x4f0403c0u, 0x3c000000u, 0x00c0300cu, 0x00c00000u, 0x00000003u, 0x00042b42u, 0xcc300c04u,
	0x303c0030u, 0x4000000cu, 0x00000000u, 0x00030c00u, 0x30070030u, 0x00030c00u, 0xcf03cc00u, 0x00000cc0u,
	0x0030c0f0u, 0x000c0503u, 0x30403000u, 0xc000c03cu, 0x0c010000u, 0x0000000cu, 0x00c3c300u, 0xc00c0000u,
	0x00303001u, 0x04c04330u, 0x0300304cu, 0x000300c0u, 0x300301c3u, 0x0000040cu, 0x00000003u, 0x000000c0u,
	0x00030030u, 0xc0000000u, 0x0c0f030cu, 0x00200c20u, 0x3fcc33c0u, 0x0c100031u, 0xc003430fu, 0x1000cf0cu,
	0x12003030u, 0x03033000u, 0x0c000000u, 0x30007003u, 0x300cc00cu, 0xc000030cu, 0x0330000cu, 0x000c0030u,
	0x00cc0000u, 0x00001003u, 0x010c3003u, 0x30001300u, 0x0c3c0330u, 0x0040cc03u, 0x300000c0u, 0x00cc0008u,
	0x3030c103u, 0xf030c00cu, 0x00f300c3u, 0x03003000u, 0x0f3c2000u, 0x00003000u, 0x00000d03u, 0xd3040800u,
	0x300000c0u, 0xd3ccc400u, 0xc0300000u, 0xc0c00030u, 0xc0003030u, 0xc3300000u, 0xc000c023u, 0x000070c0u,
	0x0f040000u, 0x00c00000u, 0x30000003u, 0x0c000000u, 0x00403fc0u, 0x0ffc030cu, 0x300c00c4u, 0x7c040100u,
	0x30000010u, 0x0000303fu, 0x33300300u, 0x03400400u, 0x0030c102u, 0x033c000cu, 0x00000f0cu, 0x03300d0cu,
	0x4c33f000u, 0x03000c00u, 0x010000c3u, 0x3003f0c0u, 0x0303000fu, 0x04007330u, 0x00300000u, 0xcf030000u,
	0x01000cc0u, 0x00f2c000u, 0x000c00c0u, 0x0400c300u, 0x000000c0u, 0x0080000cu, 0x00200300u, 0x01300d00u,
	0x33001000u, 0x00030c00u, 0x0fd00080u, 0xc30000c1u, 0x00333f03u, 0x0cc32040u, 0x00f0c080u, 0xc0000100u,
	0x0033c000u, 0x300f3d0fu, 0x003000c8u, 0x00f000c0u, 0x0000fc00u, 0x00003c04u, 0xc300000cu, 0x00030000u,
	0xc0300008u, 0x04000000u, 0x00000003u, 0x3088c030u, 0x00c0c330u, 0xf030c0c0u, 0x00400000u, 0x0001c000u,
	0xc00c13c3u, 0x00d01000u, 0x0000003cu, 0x003000f0u, 0x0c030000u, 0xfc000040u, 0x0003cc00u, 0x40000c02u,
	0xc0303f00u, 0x0c0c0003u, 0x703c0c00u, 0x030000c0u, 0x03c3004cu, 0x30000000u, 0xc73303f3u, 0x00000000u,
	0x000000c3u, 0x3030c300u, 0x00000c03u, 0x00003c00u, 0x0330320cu, 0x00000003u, 0x0033c310u, 0x00000300u,
	0x0003000cu, 0x400033ccu, 0x01333004u, 0x23003704u, 0x10300c00u, 0x00030300u, 0x702c3c0cu, 0x000c0033u,
	0x0000c000u, 0x0300c002u, 0x00c00c10u, 0x00000330u, 0x00001013u, 0x30030030u, 0x0c0000c3u, 0x0100000cu,
	0x0c83c300u, 0x30000000u, 0x30f03300u, 0x30003fc0u, 0x003f0000u, 0x30008013u, 0x00003cc0u, 0xc0003003u,
	0xc0c00102u, 0x00006000u, 0x00000c03u, 0xf3003003u, 0x00301030u, 0x0c307c40u, 0x00030000u, 0x00003030u,
	0x30100003u, 0x0003000cu, 0xc0c00000u, 0x00300030u, 0x31000002u, 0x0040c00cu, 0x0ccc0030u, 0x0003b00cu,
	0x0c0c0003u, 0x2313000du, 0x00c3030cu, 0x0f030000u, 0x0303000cu, 0x00003003u, 0x4d300301u, 0xf0070300u,
	0xc40c3000u, 0x3c000000u, 0x0000030fu, 0x00200000u, 0xcc012000u, 0x3000cc00u, 0xf3030100u, 0x00000008u,
	0x00103000u, 0x0c000c8cu, 0x00000300u, 0x0030301fu, 0x330cc300u, 0xc0303000u, 0x0000c0f0u, 0x0003031cu,
	0xc304c000u, 0x03000f33u, 0x00f00cc0u, 0x00330004u, 0x00030300u, 0x00000400u, 0x040c0c33u, 0x00004c00u,
	0x1000cc0du, 0x000000c0u, 0x00003c00u, 0x03004cccu, 0x40000c00u, 0x0000c00cu, 0xc0000cc0u, 0x000c08c0u,
	0x00000000u, 0x40c00030u, 0x10004c00u, 0xc3000003u, 0x03c3c00cu, 0x000c0330u, 0x0d0c0000u, 0x00cc3000u,
	0x3c0003c0u, 0x00030700u, 0x0000300fu, 0x30113030u, 0x01f40003u, 0x0403003cu, 0xfc00f300u, 0x00003000u,
	0x3f4c0002u, 0xc30030c0u, 0x00000c10u, 0x00000300u, 0x00003cc3u, 0x33400010u, 0x0c03f33cu, 0x000c0000u,
	0x100000c0u, 0x0c0300c0u, 0xc0400c30u, 0x00f00cc3u, 0x00033000u, 0x30c33408u, 0x00c40c00u, 0x0c00c103u,
	0x001c0b8cu, 0xf0400032u, 0x83044fc0u, 0x03000ccfu, 0x00000303u, 0x30100000u, 0xcc000000u, 0x030300c0u,
	0x00308000u, 0x00c00300u, 0x03c0814cu, 0x0cc00440u, 0x10300100u, 0xc0c00c00u, 0x30000033u, 0x0c000c0cu,
	0x00000103u, 0x0400f000u, 0x31000e00u, 0x03003000u, 0x0003030cu, 0xc1003000u, 0x00301000u, 0x30c00000u,
	0x03000300u, 0x10007300u, 0x0f300c00u, 0xc030c00cu, 0x00000030u, 0x0000cc3cu, 0x0c003000u, 0x03101c30u,
	0xc0000003u, 0x0000c0c0u, 0x01130103u, 0x04330053u, 0x4cc0003cu, 0x0703c00cu, 0x000c0300u, 0xcf031000u,
	0x02000000u, 0x00003000u, 0x00c00400u, 0x0c03000fu, 0x30c01000u, 0x00004300u, 0x00313400u, 0xc00c3000u,
	0x000c0300u, 0x003030f0u, 0x00cc00cdu, 0xcc004000u, 0x0c0001d0u, 0x3000c004u, 0x00c0c000u, 0x00304510u,
	0x0004000cu, 0x0000403cu, 0x04c02010u, 0x000c0000u, 0xc4340003u, 0xf0c00000u, 0x03f00c0cu, 0x30cc0000u,
	0x00200030u, 0x0300c003u, 0x00031000u, 0x4000000fu, 0x3001c030u, 0x31003000u, 0x0e200100u, 0x00c130c0u,
	0x0cf00100u, 0xcd000000u, 0x300300c2u, 0x00f30010u, 0xd0030000u, 0x000c0000u, 0x00c00c00u, 0x00003303u,
	0x0003c300u, 0x00403004u, 0xc300c300u, 0xc0c43000u, 0x000f0000u, 0xc0000030u, 0x0000cc10u, 0x00300c00u,
	0x8003c000u, 0x00030004u, 0x0c0000ccu, 0x30001000u, 0x30f40c0fu, 0x0f001000u, 0x030c0008u, 0x0c000300u,
	0x00000000u, 0x00000030u, 0x30000700u, 0x10300100u, 0x0c003301u, 0x01300000u, 0x03c00000u, 0x0c0030c0u,
	0x00000cc0u, 0x0cf000f0u, 0xf4c00300u, 0x00003ffcu, 0x00003000u, 0x0030c000u, 0x01000030u, 0x000c0cc1u,
	0xc003c03cu, 0x70403c00u, 0x0003000cu, 0x30802303u, 0xc04c4300u, 0x0c000000u, 0x03303000u, 0x00f0c4c1u,
	0x10030c01u, 0x00003400u, 0x00c02003u, 0x0031c040u, 0x08003c0cu, 0x0dcfc008u, 0x0000cc30u, 0x00c0c403u,
	0x00c01400u, 0x00000030u, 0x0c100d00u, 0x00131000u, 0x080300fcu, 0x0d330303u, 0xf0cc003fu, 0xcc3c0004u,
	0xc00fc300u, 0x00000000u, 0x010000c0u, 0x00303000u, 0x0003c080u, 0x0c000000u, 0x00c00300u, 0x0c0c31c0u,
	0x030030c3u, 0x0d000000u, 0x0c333300u, 0x00000000u, 0x3023000cu, 0x0300d000u, 0x0cc00c1cu, 0x0000f000u,
	0x3c0f0450u, 0x00000003u, 0x0000ccc0u, 0x00000500u, 0x0c300000u, 0x00001710u, 0xc0000c10u, 0x30c00000u,
	0xc0f33000u, 0xc08c1000u, 0xc0300004u, 0x00f00000u, 0x0c030d00u, 0xc0033000u, 0x8c400000u, 0x0c000003u,
	0x30040c00u, 0x50330000u, 0x31303030u, 0x000c0003u, 0xc0003305u, 0xc0300300u, 0x00c04cccu, 0x8002000cu,
	0x00000000u, 0x0ccc3dc0u, 0x03000000u, 0xc0004300u, 0x00000c31u, 0x1f001000u, 0x3cc0000cu, 0x30100000u,
	0x301d300cu, 0x02273000u, 0x00000000u, 0x0000000cu, 0x0cf03c03u, 0x3300f0f0u, 0x30000000u, 0x0300c000u,
	0x33cc1c0fu, 0x00330c03u, 0x03000030u, 0x3ccc00c3u, 0x0c034000u, 0xc030c03cu, 0x0003001cu, 0x33c030c3u,
	0x03303000u, 0x0b00cc0cu, 0x00033303u, 0xc000330cu, 0x30000030u, 0x00040c00u, 0x03030070u, 0x4c000000u,
	0x03c00003u, 0x31f00c40u, 0x043c30c0u, 0x0000c004u, 0x04c0fc01u, 0x000c3300u, 0x00c30000u, 0x00f30000u,
	0x03c00000u, 0x4300033cu, 0x000c0300u, 0x00000f01u, 0x30300833u, 0x80030003u, 0x03000000u, 0x00300c0cu,
	0x0040000cu, 0x00000c0cu, 0x00040033u, 0xc0000383u,
	};
static const unsigned kRay2hCalleeDivergentWords = 184;
static const unsigned kRay2hCalleeDivergentRuns = 109;

// A register-argument call: eax, ecx, edx, ebx, esi and edi from `r`, `count`
// stack words pushed last first, the caller cleaning, as 001844 and 001849 call
// these rows; eax comes back (the rows' al in its low byte). ebx, esi and edi
// are saved around it (the rows take them as arguments); the rows keep ebp.
struct Nx2hRegs
	{
	unsigned eax, ecx, edx, ebx, esi, edi;
	};

static unsigned nx2hCall(const void* fn, const Nx2hRegs* r, const unsigned* stack, unsigned count)
	{
	unsigned result;
	__asm
		{
		push	ebx
		push	esi
		push	edi
		mov		ecx, count
		mov		edx, stack
	nx2hPushNext:
		test	ecx, ecx
		je		nx2hPushed
		dec		ecx
		push	dword ptr [edx + ecx * 4]
		jmp		nx2hPushNext
	nx2hPushed:
		mov		eax, r
		mov		ecx, dword ptr [eax + 4]
		mov		edx, dword ptr [eax + 8]
		mov		ebx, dword ptr [eax + 12]
		mov		esi, dword ptr [eax + 16]
		mov		edi, dword ptr [eax + 20]
		mov		eax, dword ptr [eax]
		call	fn
		mov		result, eax
		mov		ecx, count
		lea		esp, [esp + ecx * 4]
		pop		edi
		pop		esi
		pop		ebx
		}
	return result;
	}

// One side's own images: the scratch record, and a TriangleMesh image per mesh.
struct Nx2hSide
	{
	unsigned char	scratch[0x500];
	unsigned		visited[256];
	unsigned		mesh[kNb2hMeshes][0x40];
	};

static void nx2hInitSide(Nx2gSide& owner, Nx2hSide& s)
	{
	memset(s.scratch, 0, sizeof(s.scratch));
	*(unsigned*) (s.scratch + 0x04) = 256;
	*(unsigned**) (s.scratch + 0x08) = s.visited;
	memset(s.visited, 0, sizeof(s.visited));
	nx2gContainerCtor(owner, s.scratch + 0x4e0);
	nx2gContainerCtor(owner, s.scratch + 0x4f0);
	for(unsigned m = 0; m < kNb2hMeshes; ++m)
		{
		for(unsigned i = 0; i < 0x40; ++i)
			s.mesh[m][i] = 0xcdcd8000u + i;
		s.mesh[m][0x0c / 4] = kMeshes2h[m].nbTris;
		s.mesh[m][0x10 / 4] = (unsigned) (size_t) kMeshes2h[m].verts;
		s.mesh[m][0x14 / 4] = (unsigned) (size_t) kMeshes2h[m].tris;
		s.mesh[m][0x88 / 4] = 0;
		}
	}

// The EdgeLists the side's own 002188 built: ~EdgeList (002060 / the
// candidate's) and the release through the side's own 004803 allocator.
static void nx2hReleaseSide(Nx2gSide& owner, Nx2hSide& s)
	{
	for(unsigned m = 0; m < kNb2hMeshes; ++m)
		{
		void* edgeList = (void*) (size_t) s.mesh[m][0x88 / 4];
		if(!edgeList)
			continue;
		if(owner.oracle)
			nx2gCall0(owner.base + kEdgeListDtorRva, edgeList);
		else
			((EdgeList*) edgeList)->~EdgeList();
		nx2gFree(owner, edgeList);
		s.mesh[m][0x88 / 4] = 0;
		}
	nx2gContainerDtor(owner, s.scratch + 0x4e0);
	nx2gContainerDtor(owner, s.scratch + 0x4f0);
	}

// The two sides' EdgeList face words (+0x0c: three per triangle, bit 31 the
// active flag) for the meshes both built, word for word: a count of differing
// words for the edges family's name line.
static unsigned nx2hCompareEdgeLists(const Nx2hSide& a, const Nx2hSide& b, unsigned* built)
	{
	unsigned differing = 0;
	for(unsigned m = 0; m < kNb2hMeshes; ++m)
		{
		const unsigned* ea = (const unsigned*) (size_t) a.mesh[m][0x88 / 4];
		const unsigned* eb = (const unsigned*) (size_t) b.mesh[m][0x88 / 4];
		if(!ea || !eb)
			{
			differing += (ea != 0) != (eb != 0);
			continue;
			}
		++*built;
		const unsigned* fa = (const unsigned*) (size_t) ea[3];
		const unsigned* fb = (const unsigned*) (size_t) eb[3];
		differing += ea[0] != eb[0];
		for(unsigned i = 0; fa && fb && i < 3 * kMeshes2h[m].nbTris; ++i)
			differing += fa[i] != fb[i];
		}
	return differing;
	}

// Words folded as they are (no NaN canonicalisation: both sides run the same
// instructions) and counted where they differ.
static unsigned nx2hFold(NxDigest* oracle, NxDigest* candidate, const unsigned* a, const unsigned* b, unsigned n)
	{
	nxFoldInput(oracle, a, 4 * n);
	nxFoldInput(candidate, b, 4 * n);
	unsigned differing = 0;
	for(unsigned i = 0; i < n; ++i)
		differing += a[i] != b[i];
	return differing;
	}

static unsigned nx2hFoldContainer(NxDigest* oracle, NxDigest* candidate, const unsigned char* a,
	const unsigned char* b)
	{
	const unsigned* ca = (const unsigned*) a;
	const unsigned* cb = (const unsigned*) b;
	unsigned differing = nx2hFold(oracle, candidate, &ca[1], &cb[1], 1);
	if(ca[1] == cb[1] && ca[1])
		differing += nx2hFold(oracle, candidate, (const unsigned*) (size_t) ca[2], (const unsigned*) (size_t) cb[2],
			ca[1]);
	return differing;
	}

// The hull of index k: 0..11 the box hulls (10 and 11 the large ones), 12..21
// the polytopes.
static Nx2gHullSide& nx2hHull(Nx2gSide* boxes, Nx2gSide* polytopes, int side, unsigned k)
	{
	return k < kNb2gHulls ? boxes[side].hulls[k] : polytopes[side].hulls[k - kNb2gHulls];
	}

// The small hulls (the large boxes are thousands of units across).
static unsigned nx2hSmallHull(unsigned draw)
	{
	const unsigned k = draw % 20;
	return k < 10 ? k : k + 2;
	}

// Words for translations near the meshes (0..6 across, heights 0..3).
static const unsigned kPlace2h[3][8] =
	{
	{ 0x00000000u, 0x3f800000u, 0x40000000u, 0x40400000u, 0x40800000u, 0x40a00000u, 0x3fc00000u, 0x40200000u },
	{ 0x00000000u, 0x3f800000u, 0x40000000u, 0x40400000u, 0x40800000u, 0x40a00000u, 0x3fc00000u, 0x40200000u },
	{ 0xbf800000u, 0xbf000000u, 0x00000000u, 0x3f000000u, 0x3f800000u, 0x3fc00000u, 0x40000000u, 0x40400000u },
	};

// A pose whose inverse is exact (Task 2h review): a signed permutation times a
// unit upper-triangular shear with power-of-two scales on the diagonal, S * P,
// every entry a word of S with P's sign bit (no arithmetic: P has one nonzero
// per row), and a translation from kPlace2h. Its determinant is a power of two,
// so the oracle's and the vendored Matrix4x4::Invert return the same words (the
// pre-flight checks it per case), and 001822's pose transform is compared in the
// exact family.
static const unsigned kShear2h[6] = { 0x00000000u, 0x3f000000u, 0xbe800000u, 0x3f800000u, 0xc0000000u, 0x3e800000u };
static const unsigned kScale2h[3] = { 0x3f800000u, 0x40000000u, 0x3f000000u };

static void nx2hDyadicPose(unsigned* state, NxDigest* input, unsigned m[16])
	{
	const unsigned permDraw = nxNext(state);
	const unsigned signDraw = nxNext(state);
	const unsigned shearDraw = nxNext(state);
	const unsigned scaleDraw = nxNext(state);
	const unsigned placeDraw = nxNext(state);
	const unsigned draws[5] = { permDraw, signDraw, shearDraw, scaleDraw, placeDraw };
	nxFoldInput(input, draws, sizeof(draws));
	static const unsigned kPerm[6][3] = { { 0, 1, 2 }, { 0, 2, 1 }, { 1, 0, 2 }, { 1, 2, 0 }, { 2, 0, 1 }, { 2, 1, 0 } };
	const unsigned* column = kPerm[permDraw % 6];
	unsigned s[3][3];
	for(unsigned i = 0; i < 3; ++i)
		for(unsigned j = 0; j < 3; ++j)
			s[i][j] = j < i ? 0u : j == i ? kScale2h[(scaleDraw >> (4 * i)) % 3] : kShear2h[(shearDraw >> (4 * (i + j))) % 6];
	memset(m, 0, 64);
	for(unsigned i = 0; i < 3; ++i)
		for(unsigned j = 0; j < 3; ++j)
			m[4 * i + column[j]] = s[i][j] ^ (((signDraw >> j) & 1) ? 0x80000000u : 0u);
	for(unsigned k = 0; k < 3; ++k)
		m[12 + k] = kPlace2h[k][(placeDraw >> (4 * k)) % 8];
	m[15] = 0x3f800000u;
	nxFoldInput(input, m, 64);
	}

// A pose and the pose the other way (see the block comment); `exact` when the
// second is the first's inverse word for word.
static void nx2hPose(unsigned* state, NxDigest* input, unsigned m[16], unsigned inv[16], bool* exact)
	{
	unsigned r[9];
	nx2gRotation(state, r);
	unsigned t[3], tOther[3];
	for(unsigned k = 0; k < 3; ++k)
		{
		const unsigned pick = nxNext(state);
		const unsigned pickOther = nxNext(state);
		t[k] = (pick & 15) == 0 ? nx2gMidWord(state) : kPlace2h[k][(pick >> 4) % 8];
		tOther[k] = (pickOther & 15) == 0 ? nx2gMidWord(state) : kPlace2h[k][(pickOther >> 4) % 8] | ((pickOther >> 8) & 0x80000000u);
		nxFoldInput(input, &pick, 4);
		nxFoldInput(input, &pickOther, 4);
		}
	nxFoldInput(input, r, sizeof(r));
	nxFoldInput(input, t, sizeof(t));
	nxFoldInput(input, tOther, sizeof(tOther));
	bool permutation = true;
	for(unsigned row = 0; row < 3; ++row)
		{
		unsigned nonzero = 0;
		for(unsigned col = 0; col < 3; ++col)
			{
			const unsigned word = r[3 * row + col];
			if(word == 0x3f800000u || word == 0xbf800000u)
				++nonzero;
			else if(word != 0 && word != 0x80000000u)
				permutation = false;
			}
		if(nonzero != 1)
			permutation = false;
		}
	memset(m, 0, 64);
	memset(inv, 0, 64);
	for(unsigned row = 0; row < 3; ++row)
		for(unsigned col = 0; col < 3; ++col)
			{
			m[4 * row + col] = r[3 * row + col];
			inv[4 * col + row] = r[3 * row + col];
			}
	for(unsigned k = 0; k < 3; ++k)
		m[12 + k] = t[k];
	m[15] = 0x3f800000u;
	inv[15] = 0x3f800000u;
	for(unsigned j = 0; j < 3; ++j)
		{
		if(permutation)
			{
			// -(t . r_j): row j's one nonzero word picks t's word, its sign and the
			// negation give the sign bit.
			unsigned word = 0;
			for(unsigned col = 0; col < 3; ++col)
				{
				const unsigned rw = r[3 * j + col];
				if(rw == 0x3f800000u || rw == 0xbf800000u)
					word = t[col] ^ (rw & 0x80000000u) ^ 0x80000000u;
				}
			inv[12 + j] = word;
			}
		else
			inv[12 + j] = tOther[j];
		}
	*exact = permutation;
	}

// A triangle group of a mesh: `count` indices (1..8, repeats possible) and a
// second list of up to three of them.
struct Nx2hGroup
	{
	unsigned count, list[8], count2, list2[3];
	};

static void nx2hGroup(unsigned* state, NxDigest* input, const Nx2hMesh& mesh, Nx2hGroup& g)
	{
	const unsigned countDraw = nxNext(state);
	const unsigned startDraw = nxNext(state);
	const unsigned stepDraw = nxNext(state);
	const unsigned count2Draw = nxNext(state);
	nxFoldInput(input, &countDraw, 4);
	nxFoldInput(input, &startDraw, 4);
	nxFoldInput(input, &stepDraw, 4);
	nxFoldInput(input, &count2Draw, 4);
	const unsigned most = mesh.nbTris < 8 ? mesh.nbTris : 8;
	g.count = 1 + countDraw % most;
	const unsigned step = 1 + stepDraw % 5;
	for(unsigned i = 0; i < 8; ++i)
		g.list[i] = (startDraw + i * step) % mesh.nbTris;
	g.count2 = 1 + count2Draw % (g.count < 3 ? g.count : 3);
	for(unsigned i = 0; i < 3; ++i)
		g.list2[i] = g.list[(i + (count2Draw >> 8)) % g.count];
	nxFoldInput(input, &g.count, 4);
	nxFoldInput(input, g.list, sizeof(g.list));
	nxFoldInput(input, &g.count2, 4);
	nxFoldInput(input, g.list2, sizeof(g.list2));
	}

// Three words of a direction: an axis, a (cos, sin) pair, lattice words, a drawn
// finite word, or (when `raw`) nxPickRawWord's words, signalling NaNs kept.
static void nx2hDirection(unsigned* state, NxDigest* input, unsigned d[3], bool raw, unsigned* snan)
	{
	const unsigned kind = nxNext(state);
	const unsigned axis = nxNext(state);
	const unsigned sign = nxNext(state);
	nxFoldInput(input, &kind, 4);
	nxFoldInput(input, &axis, 4);
	nxFoldInput(input, &sign, 4);
	const unsigned k = kind % 8;
	if(k < 3)
		{
		d[0] = d[1] = d[2] = 0;
		d[axis % 3] = 0x3f800000u | (sign & 0x80000000u);
		}
	else if(k < 5)
		{
		d[0] = d[1] = d[2] = 0;
		const unsigned a = axis % 3, b = (axis + 1 + (sign & 1)) % 3;
		d[a] = kCosSin2g[(sign >> 4) % 4][0] | (sign & 0x80000000u);
		d[b] = kCosSin2g[(sign >> 4) % 4][1] | ((sign << 1) & 0x80000000u);
		}
	else if(k < 6)
		for(unsigned c = 0; c < 3; ++c)
			{
			const unsigned pick = nxNext(state);
			nxFoldInput(input, &pick, 4);
			d[c] = kLattice2g[pick % 8][(pick >> 3) % 5] | ((pick >> 8) & 0x80000000u);
			}
	else if(k < 7 || !raw)
		for(unsigned c = 0; c < 3; ++c)
			d[c] = nx2gMidWord(state);
	else
		for(unsigned c = 0; c < 3; ++c)
			{
			float word;
			nxPickRawWord(state, &word);
			memcpy(&d[c], &word, 4);
			*snan += ((d[c] & 0x7fc00000u) == 0x7f800000u && (d[c] & 0x3fffffu)) ? 1 : 0;
			}
	nxFoldInput(input, d, 12);
	}

// Three words of a point for hull k: its centre (boxes; the origin for the
// polytopes), a box corner / polytope vertex, lattice words, drawn finite words,
// or (when `raw`) raw words.
static void nx2hPoint(unsigned* state, NxDigest* input, const Nx2gBox* boxes, unsigned k, unsigned p[3], bool raw,
	unsigned* snan)
	{
	const unsigned kind = nxNext(state);
	const unsigned pick = nxNext(state);
	nxFoldInput(input, &kind, 4);
	nxFoldInput(input, &pick, 4);
	const unsigned c = kind % 8;
	if(c < 3)
		{
		for(unsigned a = 0; a < 3; ++a)
			p[a] = k < kNb2gHulls ? boxes[k].centre[a] : 0;
		}
	else if(c < 5)
		{
		const Nx2gPolytope& poly = kPolytopes2g[(k - kNb2gHulls) % 5];
		for(unsigned a = 0; a < 3; ++a)
			p[a] = k < kNb2gHulls ? boxes[k].verts[3 * (pick % 8) + a] : poly.verts[3 * (pick % poly.nbVerts) + a];
		}
	else if(c < 6)
		for(unsigned a = 0; a < 3; ++a)
			{
			const unsigned word = nxNext(state);
			nxFoldInput(input, &word, 4);
			p[a] = kLattice2g[word % 8][(word >> 3) % 5] | ((word >> 8) & 0x80000000u);
			}
	else if(c < 7 || !raw)
		for(unsigned a = 0; a < 3; ++a)
			p[a] = nx2gMidWord(state);
	else
		for(unsigned a = 0; a < 3; ++a)
			{
			float word;
			nxPickRawWord(state, &word);
			memcpy(&p[a], &word, 4);
			*snan += ((p[a] & 0x7fc00000u) == 0x7f800000u && (p[a] & 0x3fffffu)) ? 1 : 0;
			}
	nxFoldInput(input, p, 12);
	}

static __declspec(noinline) unsigned nxDriveTask2h(unsigned char* base)
	{
	unsigned total = 0;
	static Nx2gSide boxSides[2], polySides[2];
	static Nx2gBox boxes[kNb2gHulls];
	static Nx2gGraph boxGraphs[kNb2gHulls], polyGraphs[5];
	static Nx2hSide sides[2];
	static const unsigned kSubdiv[5] = { 1, 2, 3, 5, 8 };

	// The fixture: its words are fixed inputs, folded into every family's input.
	NxDigest fixture;
	nxDigestInit(&fixture);
	unsigned state = 0x2e8a0000u;
	unsigned mapSubdiv[kNb2gHulls];
	for(unsigned h = 0; h < kNb2gHulls; ++h)
		{
		nx2gBuildBox(&state, boxes[h], h >= kNb2gHulls - 2);
		nx2gBuildGraph(boxes[h], boxGraphs[h]);
		const unsigned subdivDraw = nxNext(&state);
		mapSubdiv[h] = (h & 1) ? kSubdiv[subdivDraw % 4] : 0;
		nxFoldInput(&fixture, &subdivDraw, 4);
		nxFoldInput(&fixture, boxes[h].verts, sizeof(boxes[h].verts));
		nxFoldInput(&fixture, boxes[h].tris, sizeof(boxes[h].tris));
		nxFoldInput(&fixture, boxes[h].centre, sizeof(boxes[h].centre));
		nxFoldInput(&fixture, &mapSubdiv[h], 4);
		}
	for(unsigned k = 0; k < 5; ++k)
		{
		nx2gBuildPolytopeGraph(kPolytopes2g[k], polyGraphs[k]);
		nxFoldInput(&fixture, kPolytopes2g[k].verts, 12 * kPolytopes2g[k].nbVerts);
		nxFoldInput(&fixture, kPolytopes2g[k].refs, 4 * kPolytopes2g[k].nbRefs);
		nxFoldInput(&fixture, kPolytopes2g[k].faces, 32 * kPolytopes2g[k].nbPolygons);
		nxFoldInput(&fixture, &kSubdiv[k], 4);
		}
	for(unsigned m = 0; m < kNb2hMeshes; ++m)
		{
		nxFoldInput(&fixture, kMeshes2h[m].verts, 12 * kMeshes2h[m].nbVerts);
		nxFoldInput(&fixture, kMeshes2h[m].tris, 12 * kMeshes2h[m].nbTris);
		nxFoldInput(&fixture, kMeshes2h[m].planes, 16 * kMeshes2h[m].nbTris);
		}
	for(int side = 0; side < 2; ++side)
		{
		for(int which = 0; which < 2; ++which)
			{
			Nx2gSide& s = which ? polySides[side] : boxSides[side];
			s.oracle = side == 0;
			s.base = base;
			}
		for(unsigned h = 0; h < kNb2gHulls; ++h)
			nx2gBuildHull(boxSides[side], boxSides[side].hulls[h], boxes[h], &boxGraphs[h], mapSubdiv[h]);
		for(unsigned h = 0; h < kNb2gPolytopeHulls; ++h)
			nx2gBuildPolytope(polySides[side], polySides[side].hulls[h], kPolytopes2g[h % 5], &polyGraphs[h % 5],
				h >= 5 ? kSubdiv[h % 5] : 0);
		nx2hInitSide(boxSides[side], sides[side]);
		}
	unsigned buildMismatches = 0;
	for(unsigned h = 0; h < kNb2gHulls; ++h)
		buildMismatches += nx2gCompareEdges(boxSides[0].hulls[h].hull, boxSides[1].hulls[h].hull);
	for(unsigned h = 0; h < kNb2gPolytopeHulls; ++h)
		buildMismatches += nx2gCompareEdges(polySides[0].hulls[h].hull, polySides[1].hulls[h].hull);

	// -----------------------------------------------------------------------
	// convex_mesh_ray: phys_fn_001822.
	{
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	nxFoldInput(&inputDigest, &fixture.state, 8);
	unsigned perMode[2] = { 0, 0 };
	unsigned cases = 0, hits = 0, misses = 0, lazy = 0, lazyPolygons = 0, posed = 0, posedExact = 0, posedDyadic = 0,
		inputSnan = 0;
	typedef void*(__thiscall* NxInvertFn)(void*);
	const NxInvertFn oracleInvert = (NxInvertFn) (base + 0x000e4400);		// phys_fn_005197
	// The frozen Invert map against its own count and digest (the run numbers in
	// order, as the pre-flight first derived them).
	{
	unsigned mapRuns = 0;
	NxDigest mapDigest;
	nxDigestInit(&mapDigest);
	for(unsigned run = 0; run < 2 * kRay2hCases; ++run)
		if((kRay2hInvertDivergentMap[run >> 5] >> (run & 31)) & 1)
			{
			nxFoldInput(&mapDigest, &run, 4);
			++mapRuns;
			}
	if(mapRuns != kRay2hInvertDivergentRuns || mapDigest.state != kRay2hInvertDivergentDigest)
		{
		fprintf(stderr, "FAIL convex_mesh_ray: the frozen Invert map holds %u runs (digest %016llx), not %u (%016llx)\n",
			mapRuns, mapDigest.state, kRay2hInvertDivergentRuns, kRay2hInvertDivergentDigest);
		++total;
		}
	}
	unsigned invertImproved = 0;
	static Nx2gHullSide lazyHull[2];
	NxDigest splitOracle, splitCandidate;
	nxDigestInit(&splitOracle);
	nxDigestInit(&splitCandidate);
	unsigned splitWords = 0, splitRuns = 0, splitCases = 0;
	// The pre-flight: every fan triangle of every hull (the oracle side's
	// polygons; the builds are compared above) inflated by the oracle's 005185 and
	// by the candidate's vendored Triangle::Inflate under each control word. 001822
	// reaches Inflate through 001708, each side its own, so a hull with a fan on
	// which the two differ goes to the split under that word. The list is frozen
	// (kRay2hInflateDivergent); a pre-flight that no longer derives it fails here,
	// with the difference on stderr.
	{
	typedef void(__thiscall* NxInflateFn)(void*, float, bool);
	const NxInflateFn oracleInflate = (NxInflateFn) (base + kIceTriangleInflateRva);
	unsigned derived[2] = { 0, 0 };
	for(int mode = 0; mode < 2; ++mode)
		for(unsigned k = 0; k < kNb2gHulls + kNb2gPolytopeHulls; ++k)
			{
			const unsigned* hull = nx2hHull(boxSides, polySides, 0, k).hull;
			const unsigned* verts = (const unsigned*) (size_t) hull[4];
			const unsigned* polys = (const unsigned*) (size_t) hull[10];
			for(unsigned q = 0; q < hull[9]; ++q)
				{
				const unsigned count = polys[9 * q];
				const unsigned* refs = (const unsigned*) (size_t) polys[9 * q + 1];
				for(unsigned f = 0; f + 2 < count; ++f)
					{
					float inflated[2][9];
					memcpy(&inflated[0][0], &verts[3 * refs[0]], 12);
					memcpy(&inflated[0][3], &verts[3 * refs[f + 1]], 12);
					memcpy(&inflated[0][6], &verts[3 * refs[f + 2]], 12);
					memcpy(&inflated[1], &inflated[0], sizeof(inflated[0]));
					nxSetControl(mode ? kControlSimulate : kControlDefault);
					oracleInflate(&inflated[0], 0.02f, false);
					nxCandidateTriangleInflate(inflated[1], 0.02f, false);
					nxSetControl(kControlDefault);
					if(memcmp(&inflated[0], &inflated[1], sizeof(inflated[0])) != 0)
						derived[mode] |= 1u << k;
					}
				}
			}
	for(int mode = 0; mode < 2; ++mode)
		if(derived[mode] != kRay2hInflateDivergent[mode])
			{
			fprintf(stderr, "FAIL convex_mesh_ray pre-flight: control word %04x derives Inflate-divergent hulls %08x, the frozen list is %08x\n",
				mode ? kControlSimulate : kControlDefault, derived[mode], kRay2hInflateDivergent[mode]);
			++total;
			}
	}
	unsigned local = 0x2e8a1000u;
	for(unsigned i = 0; i < kRay2hCases; ++i)
		{
		const unsigned hullDraw = nxNext(&local);
		const unsigned lazyDraw = nxNext(&local);
		const unsigned poseDraw = nxNext(&local);
		nxFoldInput(&inputDigest, &hullDraw, 4);
		nxFoldInput(&inputDigest, &lazyDraw, 4);
		nxFoldInput(&inputDigest, &poseDraw, 4);
		const unsigned k = hullDraw % (kNb2gHulls + kNb2gPolytopeHulls);
		// One box case in eight on an image whose polygons are not built: 001822
		// builds them (001472).
		const bool lazyCase = k < kNb2gHulls && lazyDraw % 8 == 0;
		unsigned origin[3], direction[3];
		nx2hPoint(&local, &inputDigest, boxes, k, origin, true, &inputSnan);
		nx2hDirection(&local, &inputDigest, direction, true, &inputSnan);
		unsigned pose[16], inverse[16];
		bool exact = false;
		nx2hPose(&local, &inputDigest, pose, inverse, &exact);
		// Kinds: no pose (half), the pose with its translation zeroed and the origin
		// taken through its rotation word for word (a signed permutation: the ray
		// stays inside), or the pose as drawn. Task 2h review: one case in eight of
		// the no-pose half takes a dyadic pose (nx2hDyadicPose), and one in two of
		// those puts the origin at the pose's translation words, where 001822's
		// transform takes it to the hull's origin.
		const unsigned poseKind = poseDraw % 8;
		const unsigned dyadicDraw = nxNext(&local);
		nxFoldInput(&inputDigest, &dyadicDraw, 4);
		const bool dyadic = poseKind == 0;
		const bool withPose = poseKind >= 4 || dyadic;
		if(dyadic)
			{
			nx2hDyadicPose(&local, &inputDigest, pose);
			if(dyadicDraw & 1)
				memcpy(origin, &pose[12], 12);
			}
		if(poseKind >= 4 && poseKind < 6 && exact)
			{
			pose[12] = pose[13] = pose[14] = 0;
			unsigned moved[3];
			for(unsigned col = 0; col < 3; ++col)
				for(unsigned row = 0; row < 3; ++row)
					if(pose[4 * row + col] == 0x3f800000u || pose[4 * row + col] == 0xbf800000u)
						moved[col] = origin[row] ^ (pose[4 * row + col] & 0x80000000u);
			memcpy(origin, moved, sizeof(moved));
			}
		nxFoldInput(&inputDigest, pose, sizeof(pose));
		nxFoldInput(&inputDigest, origin, sizeof(origin));
		++cases;
		if(withPose)
			{
			++posed;
			if(dyadic)
				++posedDyadic;
			else if(exact)
				++posedExact;
			}
		// The Invert split is routed by the frozen map. The pre-flight (the pose
		// inverted by the oracle's 005197 and by the candidate's vendored
		// Matrix4x4::Invert under each control word) only guards it: a run that
		// diverges off the map fails, as does any signed-permutation or dyadic pose
		// that diverges at all; a run on the map that no longer diverges stays in the
		// split and is logged IMPROVED on stderr.
		bool invertDivergent[2] = { false, false };
		for(int mode = 0; mode < 2; ++mode)
			{
			const unsigned run = 2 * i + (unsigned) mode;
			invertDivergent[mode] = ((kRay2hInvertDivergentMap[run >> 5] >> (run & 31)) & 1) != 0;
			bool live = false;
			if(withPose)
				{
				unsigned inverted[2][16];
				memcpy(inverted[0], pose, 64);
				memcpy(inverted[1], pose, 64);
				nxSetControl(mode ? kControlSimulate : kControlDefault);
				oracleInvert(inverted[0]);
				nxCandidateMatrixInvert((float*) inverted[1]);
				nxSetControl(kControlDefault);
				live = memcmp(inverted[0], inverted[1], 64) != 0;
				}
			if(live && (dyadic || (poseKind >= 4 && exact)))
				{
				fprintf(stderr, "FAIL convex_mesh_ray pre-flight: run %u's %s pose inverts differently\n", run,
					dyadic ? "dyadic" : "signed-permutation");
				++total;
				}
			if(live && !invertDivergent[mode])
				{
				fprintf(stderr, "FAIL convex_mesh_ray pre-flight: run %u's pose inverts differently and is not on the frozen map\n", run);
				++total;
				}
			if(!live && invertDivergent[mode])
				{
				fprintf(stderr, "IMPROVED convex_mesh_ray pre-flight: run %u's pose inverts identically now; it stays in the split\n", run);
				++invertImproved;
				}
			}
		if(lazyCase)
			++lazy;
		for(int mode = 0; mode < 2; ++mode)
			{
			unsigned out[2][32];
			for(int side = 0; side < 2; ++side)
				{
				Nx2gHullSide& h = nx2hHull(boxSides, polySides, side, k);
				unsigned* hull = h.hull;
				if(lazyCase)
					{
					Nx2gHullSide& fresh = lazyHull[side];
					memset(fresh.hull, 0, sizeof(fresh.hull));
					for(unsigned f = 0; f < 9; ++f)
						fresh.hull[f] = h.hull[f];
					fresh.hull[25] = h.hull[25];
					fresh.hasMap = false;
					fresh.handBuilt = false;
					hull = fresh.hull;
					}
				unsigned t = 0xcdcd0001u, dirOut[3] = { 0xcdcd0002u, 0xcdcd0003u, 0xcdcd0004u };
				Nx2hRegs r = { (unsigned) (size_t) direction, (unsigned) (size_t) origin, 0, (unsigned) (size_t) hull, 0, 0 };
				const unsigned stack[3] = { withPose ? (unsigned) (size_t) pose : 0u, (unsigned) (size_t) &t,
					(unsigned) (size_t) dirOut };
				nxSetControl(mode ? kControlSimulate : kControlDefault);
				const unsigned result = nx2hCall(side == 0 ? (const void*) (base + kConvexMeshRayRva)
					: (const void*) &nxConvexMeshRay, &r, stack, 3);
				nxSetControl(kControlDefault);
				unsigned n = 0;
				out[side][n++] = result & 0xff;
				out[side][n++] = t;
				out[side][n++] = dirOut[0];
				out[side][n++] = dirOut[1];
				out[side][n++] = dirOut[2];
				if(lazyCase)
					{
					// The polygons 001472 built: count, then each one's count, plane and
					// extents (not its reference pointer).
					out[side][n++] = hull[9];
					const unsigned* polys = (const unsigned*) (size_t) hull[10];
					for(unsigned q = 0; polys && q < hull[9] && n + 8 <= 32; ++q)
						{
						out[side][n++] = polys[9 * q];
						for(unsigned w = 3; w < 9 && n < 32; ++w)
							out[side][n++] = polys[9 * q + w];
						}
					if(side == 0 && mode == 0)
						lazyPolygons += hull[9];
					nx2gReleaseHull(side == 0 ? boxSides[0] : boxSides[1], lazyHull[side]);
					}
				while(n < 32)
					out[side][n++] = 0;
				}
			// The split (two frozen lists): a hull on the Inflate list under this word,
			// or a run whose pose the two Matrix4x4::Inverts (005197) invert to different
			// words (the frozen Invert map). With the oracle's 005185 bound into 001708
			// and its 005197 into 001822 it reads 0 (the bind patch in the evidence, one
			// bit each).
			const bool split = ((kRay2hInflateDivergent[mode] >> k) & 1) != 0 || invertDivergent[mode];
			const unsigned differing = nx2hFold(split ? &splitOracle : &oracleDigest,
				split ? &splitCandidate : &candidateDigest, out[0], out[1], 32);
			if(split)
				{
				++splitCases;
				splitWords += differing;
				if(differing)
					++splitRuns;
				}
			else
				perMode[mode] += differing;
			if(mode == 0)
				{
				if(out[0][0])
					++hits;
				else
					++misses;
				}
			}
		}
	if(invertImproved)
		fprintf(stderr, "IMPROVED convex_mesh_ray pre-flight: %u frozen Invert runs invert identically now\n", invertImproved);
	const bool splitOver = splitWords > kRay2hCalleeDivergentWords || splitRuns > kRay2hCalleeDivergentRuns;
	total += perMode[0] + perMode[1] + (splitOver ? 1 : 0);
	printf("collision name=convex_mesh_ray index=- rva=0x%08x owner=phys_fn_001822 checks=%u oracle=%016llx candidate=%016llx mismatches=%u default_mismatches=%u simulate_mismatches=%u build_mismatches=%u\n",
		kConvexMeshRayRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state,
		perMode[0] + perMode[1] + buildMismatches, perMode[0], perMode[1], buildMismatches);
	printf("collision name=convex_mesh_ray.callee_divergent index=- rva=0x%08x owner=phys_fn_001822 checks=%u oracle=%016llx candidate=%016llx words=%u runs=%u ceiling_words=%u ceiling_runs=%u ceiling=%s\n",
		kConvexMeshRayRva, splitOracle.checks, splitOracle.state, splitCandidate.state, splitWords, splitRuns,
		kRay2hCalleeDivergentWords, kRay2hCalleeDivergentRuns, splitOver ? "exceeded" : "ok");
	nxPrintInput("convex_mesh_ray", &inputDigest);
	printf("collision coverage name=convex_mesh_ray hulls=%u cases=%u hits=%u misses=%u lazy=%u lazy_polygons=%u posed=%u posed_exact=%u posed_dyadic=%u invert_split_runs=%u split_runs=%u input_snan=%u\n",
		kNb2gHulls + kNb2gPolytopeHulls, cases, hits, misses, lazy, lazyPolygons, posed, posedExact, posedDyadic,
		kRay2hInvertDivergentRuns, splitCases, inputSnan);
	}

	// -----------------------------------------------------------------------
	// convex_mesh_faces: phys_fn_001832.
	{
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	nxFoldInput(&inputDigest, &fixture.state, 8);
	unsigned perMode[2] = { 0, 0 };
	unsigned cases = 0, separated = 0, overlapping = 0, facing = 0, listedAll = 0, mapCases = 0, stampWraps = 0,
		inputSnan = 0;
	unsigned local = 0x2e8a2000u;
	for(unsigned i = 0; i < kSat2hCases; ++i)
		{
		const unsigned hullDraw = nxNext(&local);
		const unsigned meshDraw = nxNext(&local);
		const unsigned depthDraw = nxNext(&local);
		const unsigned stampDraw = nxNext(&local);
		nxFoldInput(&inputDigest, &hullDraw, 4);
		nxFoldInput(&inputDigest, &meshDraw, 4);
		nxFoldInput(&inputDigest, &depthDraw, 4);
		nxFoldInput(&inputDigest, &stampDraw, 4);
		const unsigned k = nx2hSmallHull(hullDraw);
		const unsigned m = meshDraw % kNb2hMeshes;
		Nx2hGroup g;
		nx2hGroup(&local, &inputDigest, kMeshes2h[m], g);
		unsigned pose[16], inverse[16];
		bool exact = false;
		nx2hPose(&local, &inputDigest, pose, inverse, &exact);
		unsigned point[3];
		nx2hPoint(&local, &inputDigest, boxes, k, point, true, &inputSnan);
		const unsigned depth = depthDraw % 8 < 6 ? 0x7f7fffffu : kLattice2g[depthDraw % 8][(depthDraw >> 8) % 5];
		const unsigned stamp = (stampDraw & 15) == 0 ? 0xfffffffeu : stampDraw & 0xffffu;
		nxFoldInput(&inputDigest, &depth, 4);
		nxFoldInput(&inputDigest, &stamp, 4);
		const unsigned nbPolygons = nx2hHull(boxSides, polySides, 0, k).hull[9];
		++cases;
		if(nx2hHull(boxSides, polySides, 0, k).hasMap)
			++mapCases;
		for(int mode = 0; mode < 2; ++mode)
			{
			unsigned out[2][104];
			for(int side = 0; side < 2; ++side)
				{
				Nx2gHullSide& h = nx2hHull(boxSides, polySides, side, k);
				Nx2hSide& s = sides[side];
				*(unsigned*) (s.scratch + 0x14) = stamp;
				memset(s.visited, 0, sizeof(s.visited));
				unsigned bestDepth = depth, axis[3] = { 0xcdcd0001u, 0xcdcd0002u, 0xcdcd0003u }, index = 0xcdcd0004u;
				unsigned polygons[128], nbFacing = 0xcdcd0005u;
				for(unsigned q = 0; q < 128; ++q)
					polygons[q] = 0xcdcd1000u + q;
				Nx2hRegs r = { 0, 0, 0, (unsigned) (size_t) pose, 0, 0 };
				const unsigned stack[12] = { (unsigned) (size_t) s.scratch, (unsigned) (size_t) point,
					(unsigned) (size_t) (h.mesh + 4), *(unsigned*) (h.mesh + 0xa8), g.count, (unsigned) (size_t) g.list,
					(unsigned) (size_t) s.mesh[m], (unsigned) (size_t) &bestDepth, (unsigned) (size_t) axis,
					(unsigned) (size_t) &index, (unsigned) (size_t) polygons, (unsigned) (size_t) &nbFacing };
				nxSetControl(mode ? kControlSimulate : kControlDefault);
				const unsigned result = nx2hCall(side == 0 ? (const void*) (base + kConvexMeshFaceAxesRva)
					: (const void*) &nxConvexMeshFaceAxes, &r, stack, 12);
				nxSetControl(kControlDefault);
				unsigned n = 0;
				out[side][n++] = result & 0xff;
				out[side][n++] = bestDepth;
				out[side][n++] = axis[0];
				out[side][n++] = axis[1];
				out[side][n++] = axis[2];
				out[side][n++] = index;
				out[side][n++] = nbFacing;
				out[side][n++] = *(unsigned*) (s.scratch + 0x14);
				for(unsigned q = 0; q < 32; ++q)
					out[side][n++] = polygons[q];
				for(unsigned v = 0; v < 64; ++v)
					out[side][n++] = s.visited[v];
				}
			perMode[mode] += nx2hFold(&oracleDigest, &candidateDigest, out[0], out[1], 104);
			if(mode == 0)
				{
				if(out[0][0])
					++overlapping;
				else
					++separated;
				if(out[0][0] && out[0][5] != 0xffffffffu)
					++facing;
				if(out[0][0] && out[0][6] == nbPolygons)
					++listedAll;
				if(stamp > 0xfffffff0u && out[0][7] < stamp)
					++stampWraps;
				}
			}
		}
	total += perMode[0] + perMode[1];
	printf("collision name=convex_mesh_faces index=- rva=0x%08x owner=phys_fn_001832 checks=%u oracle=%016llx candidate=%016llx mismatches=%u default_mismatches=%u simulate_mismatches=%u\n",
		kConvexMeshFaceAxesRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state, perMode[0] + perMode[1],
		perMode[0], perMode[1]);
	nxPrintInput("convex_mesh_faces", &inputDigest);
	printf("collision coverage name=convex_mesh_faces meshes=%u cases=%u overlapping=%u separated=%u best_set=%u listed_all=%u map_cases=%u stamp_wraps=%u input_snan=%u\n",
		kNb2hMeshes, cases, overlapping, separated, facing, listedAll, mapCases, stampWraps, inputSnan);
	}

	// -----------------------------------------------------------------------
	// convex_mesh_edges and convex_mesh_cross: phys_fn_001840, then
	// phys_fn_001836 on the edge directions it gathered.
	{
	NxDigest edgesOracle, edgesCandidate, edgesInput, crossOracle, crossCandidate, crossInput;
	nxDigestInit(&edgesOracle);
	nxDigestInit(&edgesCandidate);
	nxDigestInit(&edgesInput);
	nxDigestInit(&crossOracle);
	nxDigestInit(&crossCandidate);
	nxDigestInit(&crossInput);
	nxFoldInput(&edgesInput, &fixture.state, 8);
	nxFoldInput(&crossInput, &fixture.state, 8);
	unsigned edgesMode[2] = { 0, 0 }, crossMode[2] = { 0, 0 };
	unsigned cases = 0, edgesTrue = 0, edgesFalse = 0, directions = 0, kept = 0, crossTrue = 0, crossFalse = 0,
		crossAxes = 0, crossRuns = 0, edgesSnan = 0, crossSnan = 0;
	unsigned local = 0x2e8a3000u;
	for(unsigned i = 0; i < kSat2hCases; ++i)
		{
		const unsigned hullDraw = nxNext(&local);
		const unsigned meshDraw = nxNext(&local);
		const unsigned polygonDraw = nxNext(&local);
		const unsigned depthDraw = nxNext(&local);
		const unsigned keepDraw = nxNext(&local);
		const unsigned normalDraw = nxNext(&local);
		const unsigned planeDraw = nxNext(&local);
		const unsigned listDraw = nxNext(&local);
		const unsigned stampDraw = nxNext(&local);
		const unsigned draws[9] = { hullDraw, meshDraw, polygonDraw, depthDraw, keepDraw, normalDraw, planeDraw, listDraw,
			stampDraw };
		nxFoldInput(&edgesInput, draws, sizeof(draws));
		nxFoldInput(&crossInput, draws, sizeof(draws));
		const unsigned k = nx2hSmallHull(hullDraw);
		const unsigned m = meshDraw % kNb2hMeshes;
		const Nx2hMesh& mesh = kMeshes2h[m];
		Nx2hGroup g;
		nx2hGroup(&local, &edgesInput, mesh, g);
		nxFoldInput(&crossInput, &g, sizeof(g));
		unsigned pose[16], inverse[16];
		bool exact = false;
		nx2hPose(&local, &edgesInput, pose, inverse, &exact);
		nxFoldInput(&crossInput, pose, sizeof(pose));
		nxFoldInput(&crossInput, inverse, sizeof(inverse));
		const unsigned nbPolygons = nx2hHull(boxSides, polySides, 0, k).hull[9];
		const unsigned polygon = polygonDraw % nbPolygons;
		const unsigned depth = depthDraw % 4 < 3 ? 0x7f7fffffu : kLattice2g[depthDraw % 8][(depthDraw >> 8) % 5];
		unsigned axisIn[3];
		nx2hDirection(&local, &edgesInput, axisIn, false, &edgesSnan);
		// The triangle's normal: the mesh's own words for its first listed
		// triangle, or a drawn direction (raw words included).
		unsigned normal[3];
		const unsigned* plane = &mesh.planes[4 * g.list2[0]];
		if(normalDraw % 4 != 0)
			memcpy(normal, plane, 12);
		else
			nx2hDirection(&local, &edgesInput, normal, true, &edgesSnan);
		// The plane 001836 takes: the same triangle's, or drawn words.
		unsigned crossPlane[4];
		memcpy(crossPlane, plane, 16);
		if(planeDraw % 4 == 0)
			{
			nx2hDirection(&local, &crossInput, crossPlane, true, &crossSnan);
			const unsigned dWord = nxNext(&local);
			nxFoldInput(&crossInput, &dWord, 4);
			crossPlane[3] = kLattice2g[dWord % 8][(dWord >> 3) % 5] | ((dWord >> 8) & 0x80000000u);
			}
		// 001836's polygons: from `listDraw`, one to all of the hull's.
		unsigned crossPolygons[128];
		const unsigned nbCross = 1 + listDraw % nbPolygons;
		for(unsigned q = 0; q < nbCross; ++q)
			crossPolygons[q] = (listDraw / 7 + q * 3) % nbPolygons;
		const bool keep = keepDraw % 4 == 0;
		const unsigned stamp = (stampDraw & 15) == 0 ? 0xfffffffeu : stampDraw & 0xffffu;
		nxFoldInput(&edgesInput, &polygon, 4);
		nxFoldInput(&edgesInput, &depth, 4);
		nxFoldInput(&edgesInput, normal, 12);
		nxFoldInput(&edgesInput, &stamp, 4);
		// 001836 reads what 001840 left, so 001840's own inputs are this family's
		// inputs too (Task 2h review).
		nxFoldInput(&crossInput, &polygon, 4);
		nxFoldInput(&crossInput, &depth, 4);
		nxFoldInput(&crossInput, axisIn, 12);
		nxFoldInput(&crossInput, normal, 12);
		nxFoldInput(&crossInput, crossPlane, 16);
		nxFoldInput(&crossInput, &nbCross, 4);
		nxFoldInput(&crossInput, crossPolygons, 4 * nbCross);
		nxFoldInput(&crossInput, &stamp, 4);
		++cases;
		for(int mode = 0; mode < 2; ++mode)
			{
			unsigned edgesOut[2][40], crossOut[2][8];
			unsigned edgesDiffer = 0, crossDiffer = 0;
			for(int side = 0; side < 2; ++side)
				{
				Nx2gHullSide& h = nx2hHull(boxSides, polySides, side, k);
				Nx2hSide& s = sides[side];
				*(unsigned*) (s.scratch + 0x14) = stamp;
				memset(s.visited, 0, sizeof(s.visited));
				// 001849 empties +0x4e0 for each group; one case in four keeps the
				// directions the side's previous case left (and mode 1 those of mode 0).
				if(!keep && mode == 0)
					((unsigned*) (s.scratch + 0x4e0))[1] = 0;
				const unsigned map = *(unsigned*) (h.mesh + 0xa8);
				unsigned axisOut[3] = { 0xcdcd0001u, 0xcdcd0002u, 0xcdcd0003u };
				unsigned depthOut = 0xcdcd0004u, indexOut = 0xcdcd0005u;
				Nx2hRegs r = { 0, 0, 0, (unsigned) (size_t) axisOut, 0, 0 };
				const unsigned stack[16] = { (unsigned) (size_t) (s.scratch + 0x4e0), polygon, depth,
					(unsigned) (size_t) axisIn, (unsigned) (size_t) &depthOut, (unsigned) (size_t) &indexOut,
					(unsigned) (size_t) s.scratch, (unsigned) (size_t) (h.mesh + 4), (unsigned) (size_t) pose, map,
					g.count, (unsigned) (size_t) g.list, (unsigned) (size_t) s.mesh[m], g.count2,
					(unsigned) (size_t) g.list2, (unsigned) (size_t) normal };
				nxSetControl(mode ? kControlSimulate : kControlDefault);
				const unsigned result = nx2hCall(side == 0 ? (const void*) (base + kConvexMeshEdgeAxesRva)
					: (const void*) &nxConvexMeshEdgeAxes, &r, stack, 16);
				nxSetControl(kControlDefault);
				unsigned n = 0;
				edgesOut[side][n++] = result & 0xff;
				edgesOut[side][n++] = axisOut[0];
				edgesOut[side][n++] = axisOut[1];
				edgesOut[side][n++] = axisOut[2];
				edgesOut[side][n++] = depthOut;
				edgesOut[side][n++] = indexOut;
				edgesOut[side][n++] = *(unsigned*) (s.scratch + 0x14);
				while(n < 40)
					edgesOut[side][n++] = 0;
				for(unsigned v = 0; v < 33; ++v)
					edgesOut[side][7 + v] = s.visited[v];

				// 001836: eax the pose the other way, ebx the pose, edx the plane.
				unsigned crossAxis[3] = { 0xcdcd0011u, 0xcdcd0012u, 0xcdcd0013u }, crossDepth = 0xcdcd0014u;
				Nx2hRegs rc = { (unsigned) (size_t) inverse, 0, (unsigned) (size_t) crossPlane, (unsigned) (size_t) pose, 0, 0 };
				const unsigned crossStack[11] = { (unsigned) (size_t) (s.scratch + 0x4e0), (unsigned) (size_t) s.scratch,
					nbCross, (unsigned) (size_t) crossPolygons, (unsigned) (size_t) s.mesh[m],
					(unsigned) (size_t) (h.mesh + 4), map, g.count, (unsigned) (size_t) g.list,
					(unsigned) (size_t) crossAxis, (unsigned) (size_t) &crossDepth };
				nxSetControl(mode ? kControlSimulate : kControlDefault);
				const unsigned crossResult = nx2hCall(side == 0 ? (const void*) (base + kConvexMeshCrossAxesRva)
					: (const void*) &nxConvexMeshCrossAxes, &rc, crossStack, 11);
				nxSetControl(kControlDefault);
				crossOut[side][0] = crossResult & 0xff;
				crossOut[side][1] = crossAxis[0];
				crossOut[side][2] = crossAxis[1];
				crossOut[side][3] = crossAxis[2];
				crossOut[side][4] = crossDepth;
				crossOut[side][5] = *(unsigned*) (s.scratch + 0x14);
				crossOut[side][6] = 0;
				crossOut[side][7] = 0;
				}
			edgesDiffer += nx2hFold(&edgesOracle, &edgesCandidate, edgesOut[0], edgesOut[1], 40);
			edgesDiffer += nx2hFoldContainer(&edgesOracle, &edgesCandidate, sides[0].scratch + 0x4e0,
				sides[1].scratch + 0x4e0);
			crossDiffer += nx2hFold(&crossOracle, &crossCandidate, crossOut[0], crossOut[1], 8);
			crossDiffer += nx2hFoldContainer(&crossOracle, &crossCandidate, sides[0].scratch + 0x4f0,
				sides[1].scratch + 0x4f0);
			edgesMode[mode] += edgesDiffer;
			crossMode[mode] += crossDiffer;
			if(mode == 0)
				{
				if(edgesOut[0][0])
					++edgesTrue;
				else
					++edgesFalse;
				directions += ((const unsigned*) (sides[0].scratch + 0x4e0))[1];
				if(keep)
					++kept;
				if(crossOut[0][0])
					++crossTrue;
				else
					++crossFalse;
				const unsigned gathered = ((const unsigned*) (sides[0].scratch + 0x4f0))[1];
				crossAxes += gathered;
				if(gathered)
					++crossRuns;
				}
			}
		}
	unsigned edgeListsBuilt = 0;
	const unsigned edgeListMismatches = nx2hCompareEdgeLists(sides[0], sides[1], &edgeListsBuilt);
	total += edgesMode[0] + edgesMode[1] + crossMode[0] + crossMode[1] + edgeListMismatches;
	printf("collision name=convex_mesh_edges index=- rva=0x%08x owner=phys_fn_001840 checks=%u oracle=%016llx candidate=%016llx mismatches=%u default_mismatches=%u simulate_mismatches=%u build_mismatches=%u\n",
		kConvexMeshEdgeAxesRva, edgesOracle.checks, edgesOracle.state, edgesCandidate.state,
		edgesMode[0] + edgesMode[1] + edgeListMismatches, edgesMode[0], edgesMode[1], edgeListMismatches);
	nxPrintInput("convex_mesh_edges", &edgesInput);
	printf("collision coverage name=convex_mesh_edges meshes=%u cases=%u true=%u false=%u directions=%u kept=%u input_snan=%u\n",
		kNb2hMeshes, cases, edgesTrue, edgesFalse, directions, kept, edgesSnan);
	printf("collision name=convex_mesh_cross index=- rva=0x%08x owner=phys_fn_001836 checks=%u oracle=%016llx candidate=%016llx mismatches=%u default_mismatches=%u simulate_mismatches=%u\n",
		kConvexMeshCrossAxesRva, crossOracle.checks, crossOracle.state, crossCandidate.state,
		crossMode[0] + crossMode[1], crossMode[0], crossMode[1]);
	nxPrintInput("convex_mesh_cross", &crossInput);
	printf("collision coverage name=convex_mesh_cross cases=%u true=%u false=%u axes=%u runs_with_axes=%u input_snan=%u\n",
		cases, crossTrue, crossFalse, crossAxes, crossRuns, crossSnan);
	}

	// -----------------------------------------------------------------------
	// convex_mesh_contacts: phys_fn_001842.
	{
	static NxContactWorld world[2];
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	nxFoldInput(&inputDigest, &fixture.state, 8);
	unsigned perMode[2] = { 0, 0 };
	unsigned calls = 0, withContacts = 0, contacts = 0, headers = 0, meshPosed = 0, swapped = 0;
	unsigned local = 0x2e8a4000u;
	static const unsigned kIdentity2h[16] =
		{
		0x3f800000u, 0, 0, 0, 0, 0x3f800000u, 0, 0, 0, 0, 0x3f800000u, 0, 0, 0, 0, 0x3f800000u
		};
	for(unsigned i = 0; i < kContact2hCases; ++i)
		{
		const unsigned hullDraw = nxNext(&local);
		const unsigned meshDraw = nxNext(&local);
		const unsigned triangleDraw = nxNext(&local);
		const unsigned polygonDraw = nxNext(&local);
		const unsigned normalDraw = nxNext(&local);
		const unsigned meshPoseDraw = nxNext(&local);
		const unsigned wordDraw = nxNext(&local);
		const unsigned flagsDraw0 = nxNext(&local);
		const unsigned flagsDraw1 = nxNext(&local);
		const unsigned materialDraw0 = nxNext(&local);
		const unsigned materialDraw1 = nxNext(&local);
		const unsigned holderDraw = nxNext(&local);
		const unsigned orientDraw = nxNext(&local);
		const unsigned draws[13] = { hullDraw, meshDraw, triangleDraw, polygonDraw, normalDraw, meshPoseDraw, wordDraw,
			flagsDraw0, flagsDraw1, materialDraw0, materialDraw1, holderDraw, orientDraw };
		nxFoldInput(&inputDigest, draws, sizeof(draws));
		const unsigned k = nx2hSmallHull(hullDraw);
		const unsigned m = meshDraw % kNb2hMeshes;
		const Nx2hMesh& mesh = kMeshes2h[m];
		const unsigned triangle = triangleDraw % mesh.nbTris;
		const unsigned nbPolygons = nx2hHull(boxSides, polySides, 0, k).hull[9];
		const unsigned polygon = polygonDraw % nbPolygons;
		unsigned pose[16], inverse[16];
		bool exact = false;
		nx2hPose(&local, &inputDigest, pose, inverse, &exact);
		unsigned meshPose[16], meshInverse[16];
		bool meshExact = false;
		nx2hPose(&local, &inputDigest, meshPose, meshInverse, &meshExact);
		const bool posedMesh = meshPoseDraw % 4 == 0;
		if(!posedMesh)
			memcpy(meshPose, kIdentity2h, sizeof(meshPose));
		// The hull's world pose (ebx) is an object of its own, as 001849's is (Task
		// 2h review): one case in two its words are the relative pose's (the mesh at
		// the identity makes the two agree), otherwise a pose drawn apart.
		unsigned convexWorld[16], convexWorldOther[16];
		bool convexWorldExact = false;
		nx2hPose(&local, &inputDigest, convexWorld, convexWorldOther, &convexWorldExact);
		if((meshPoseDraw >> 8) & 1)
			memcpy(convexWorld, pose, sizeof(convexWorld));
		nxFoldInput(&inputDigest, convexWorld, sizeof(convexWorld));
		const unsigned* plane = &mesh.planes[4 * triangle];
		unsigned normal[3];
		const unsigned normalKind = normalDraw % 8;
		if(normalKind < 4)
			{
			for(unsigned c = 0; c < 3; ++c)
				normal[c] = plane[c] ^ ((normalKind & 1) ? 0x80000000u : 0u);
			}
		else
			{
			unsigned ignored = 0;
			nx2hDirection(&local, &inputDigest, normal, false, &ignored);
			}
		const NxU32 word = (wordDraw & 3) == 0 ? 0xffffu : wordDraw & 0xffffu;
		const unsigned shapeWord = (wordDraw >> 16) & 0xffffu;
		const bool nullHolder0 = holderDraw % 8 == 1;
		const bool nullHolder1 = holderDraw % 8 == 2;
		const bool orient = (orientDraw & 1) != 0;
		const NxU32 material0 = materialDraw0 & 0xff;
		const NxU32 material1 = materialDraw1 & 0xff;
		nxFoldInput(&inputDigest, &triangle, 4);
		nxFoldInput(&inputDigest, &polygon, 4);
		nxFoldInput(&inputDigest, meshPose, sizeof(meshPose));
		nxFoldInput(&inputDigest, normal, 12);
		nxFoldInput(&inputDigest, &word, 4);
		++calls;
		if(posedMesh)
			++meshPosed;
		if(orient)
			++swapped;
		for(int mode = 0; mode < 2; ++mode)
			{
			for(int side = 0; side < 2; ++side)
				{
				Nx2gHullSide& h = nx2hHull(boxSides, polySides, side, k);
				Nx2hSide& s = sides[side];
				static unsigned char store[2][2][kShapeBytes];
				unsigned char* convexShape = store[side][0];
				unsigned char* meshShape = store[side][1];
				nxIdentity((NxCollisionShape*) convexShape);
				nxIdentity((NxCollisionShape*) meshShape);
				((NxCollisionShape*) convexShape)->type = 4;
				((NxCollisionShape*) meshShape)->type = 4;
				*(unsigned char**) (convexShape + 0xe0) = h.mesh;
				*(unsigned**) (meshShape + 0xe0) = s.mesh[m];
				*(unsigned short*) (convexShape + 0xda) = (unsigned short) shapeWord;
				convexShape[0xde] = (unsigned char) (flagsDraw0 & 0x3f);
				meshShape[0xde] = (unsigned char) (flagsDraw1 & 0x3f);
				nxResetWorld(&world[side]);
				nxStageWorld(&world[side], (NxCollisionShape*) convexShape, (NxCollisionShape*) meshShape, true, true,
					material0, material1, nullHolder0, nullHolder1, orient);
				const unsigned* verts = mesh.verts;
				const unsigned* tri = &mesh.tris[3 * triangle];
				Nx2hRegs r = { 0, 0, polygon, (unsigned) (size_t) convexWorld, (unsigned) (size_t) meshPose,
					(unsigned) (size_t) plane };
				const unsigned stack[13] = { (unsigned) (size_t) normal, (unsigned) (size_t) (h.mesh + 4),
					(unsigned) (size_t) &verts[3 * tri[0]], (unsigned) (size_t) &verts[3 * tri[1]],
					(unsigned) (size_t) &verts[3 * tri[2]], triangle, (unsigned) (size_t) world[side].sphere,
					(unsigned) (size_t) world[side].plane, (unsigned) (size_t) pose, (unsigned) (size_t) inverse,
					(unsigned) (size_t) &world[side].sink, 0, word };
				nxSetControl(mode ? kControlSimulate : kControlDefault);
				nx2hCall(side == 0 ? (const void*) (base + kConvexMeshContactsRva) : (const void*) &nxConvexMeshContacts,
					&r, stack, 13);
				nxSetControl(kControlDefault);
				}
			nxFoldStream(&oracleDigest, &world[0]);
			nxFoldStream(&candidateDigest, &world[1]);
			perMode[mode] += nxCompareStreams(&world[0], &world[1], mode);
			if(mode == 0)
				{
				if(world[0].sink.contactCount)
					++withContacts;
				contacts += world[0].sink.contactCount;
				headers += world[0].stream[0];
				}
			}
		}
	total += perMode[0] + perMode[1];
	printf("collision name=convex_mesh_contacts index=- rva=0x%08x owner=phys_fn_001842 checks=%u oracle=%016llx candidate=%016llx mismatches=%u default_mismatches=%u simulate_mismatches=%u\n",
		kConvexMeshContactsRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state, perMode[0] + perMode[1],
		perMode[0], perMode[1]);
	nxPrintInput("convex_mesh_contacts", &inputDigest);
	printf("collision coverage name=convex_mesh_contacts calls=%u calls_with_contacts=%u contacts=%u headers=%u mesh_posed=%u swapped=%u\n",
		calls, withContacts, contacts, headers, meshPosed, swapped);
	}

	for(int side = 0; side < 2; ++side)
		{
		nx2hReleaseSide(boxSides[side], sides[side]);
		for(unsigned h = 0; h < kNb2gHulls; ++h)
			nx2gReleaseHull(boxSides[side], boxSides[side].hulls[h]);
		for(unsigned h = 0; h < kNb2gPolytopeHulls; ++h)
			nx2gReleaseHull(polySides[side], polySides[side].hulls[h]);
		}
	return total + buildMismatches;
	}


// ---------------------------------------------------------------------------
// convex-mesh gap Task 2i (units/convex-mesh-gap-contract.md: sub-unit M's
// second half, ContactConvexHeightfield.cpp, and 002081). Three families, each
// entry at its own address with its own calling convention, over each side's own
// images:
//
//   contact_convex_heightfield  001847 (matrix-A signature), and through it
//                               001844/001846, 002081, 001822, 001461, 001502,
//                               001760, 001855, 001692, 000875, 000505, 002188
//                               and the vendored OBBCollider::Collide.
//   contact_convex_mesh         001853, the five-byte jmp, and through it 001851,
//                               001849, 001653, 001832, 001836, 001840, 001842,
//                               002188, RadixSort, Triangle::Area / Center and
//                               OBBCollider::Collide.
//   mesh_vertex_normals         002081 (thiscall on the internal mesh, TriangleMesh
//                               +0x08) over every mesh image, with 002146.
//
// The images: the Task 2g hulls (twelve box hulls each side builds with its own
// 001472 / 001502 and the ten hand-built polytope hulls; their convex mesh images
// carry the local bounds 001851 reads at +0x44..+0x58: the box's lattice words,
// or kPolyBounds2i), TriangleMesh images of the ten meshes (the Task 2h eight and
// two larger height fields: 289 vertices / 512 triangles and 90 / 144), each with
// +0x08 / +0x0c the counts, +0x10 / +0x14 the words, +0x18 materials and +0x1c a
// remap (per case one or both null), +0x20 the vertex normals (002081 on first
// use), +0x24 the triangles' plane words, +0x28 an OPCODE Model the side's own
// Model::Build built over a MeshInterface of its own (PhysicsCollisionOpcode.cpp),
// +0x78 / +0x7c the height field's flags and vertical axis (drawn per case),
// +0x88 the EdgeList (002188 on first use) and +0x94 / +0x98 the convex and flat
// parts (three layouts, drawn per case); and a context each (+0x04 / +0x08 a
// visited array, +0x14 the stamp, +0x110 an OBBCollider built by the side's own
// constructor, +0x244 its OBBCache whose Container the side's own constructor
// built, +0x4e0 / +0x4f0 the axis Containers). The shapes are staged by
// nxStageWorld (owners, holders, collision objects); the convex shape's Prunable
// (+0xa4) has flag bit 2 set, so 004886 is not called, and a pruner whose box
// array holds drawn box words. A Foundation SDK exists while the block runs
// (nx2iFoundationBegin): the failed-query reports go to a recording stream, and
// 002081's blocks come from a recording allocator.
//
// Fixed-input rules (the oracle would read memory it does not own, or break):
// the convex shape's pruning handle is never 0xffff (001847 then reads a null
// box); the single-triangle mesh is not a height field (its one-node model has no
// tree, which 001847's no-primitive-test query walks); the remap is a permutation
// of the triangles (001844 remaps in place per contact); every pose of contact_convex_mesh is finite (a NaN leaves 001832's index
// -1, which 001840 hands to slot 4); a failed query is driven only while the
// Foundation instance exists (the listing's guard breaks with none).
//
// Unsigned helpers only; every float input is a word written as bits.
unsigned nx2iObbColliderSize();
void nx2iCandidateObbColliderConstruct(void* at);
void nx2iCandidateLssColliderConstruct(void* at);
void nx2iCandidateLssColliderDestruct(void* at);
void nx2iCandidateObbColliderDestruct(void* at);
unsigned nx2iObbColliderLayoutOk();
void nx2lCandidateAabbTreeColliderConstruct(void* at);
void nx2lCandidateAabbTreeColliderDestruct(void* at);
unsigned nx2iModelSize();
void* nx2iMeshInterfaceNew(unsigned nbTris, unsigned nbVerts, const unsigned* tris, const unsigned* verts);
void nx2iMeshInterfaceDelete(void* iface);
void* nx2iCandidateModelBuild(void* iface, unsigned rules, unsigned kind, unsigned* built);
void nx2iCandidateModelDelete(void* model);
void nx2iOracleModelBuild(void* storage, const void* ctor, const void* build, void* iface, unsigned rules,
	unsigned kind, unsigned* built);
unsigned nx2iModelTreeWords(const void* model, unsigned* out, unsigned max);
void nx2iModelShape(const void* model, unsigned* usedBytes, unsigned* nodes);
unsigned nx2iModelMeshInterfaceOffset(const void* model, const void* iface);
unsigned nx2iFoundationBegin();
void nx2iFoundationEnd();
unsigned nx2iTakeReports(unsigned* out, unsigned max);
void nx2iFoundationFree(void* block);
void nx2iFoundationCounts(unsigned* mallocs, unsigned* frees, unsigned* lastSize);
// The candidate row of phys_fn_002081 (TriangleMeshTopology.cpp; TriangleMesh.h, which
// declares it, brings Opcode.h with it).
void nxMeshComputeVertexNormals();

// Generated offline (scratchpad genmesh2i.py), as the Task 2h meshes were: vertex
// words on a lattice, 32-bit triangles counter-clockwise from above, and each
// triangle's plane words (the unit normal and d, normalised in double and
// narrowed).
// large terrain: 289 vertices, 512 triangles
static const unsigned kMeshVerts2i0[867] =
	{
	0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3e800000u, 0x40000000u, 0x00000000u, 0x3f800000u, 0x40400000u, 0x00000000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x3fe00000u, 0x40a00000u, 0x00000000u, 0x3fe00000u, 0x40c00000u, 0x00000000u, 0x00000000u, 0x40e00000u, 0x00000000u, 0x3f800000u, 0x41000000u, 0x00000000u, 0x3e800000u, 0x41100000u, 0x00000000u, 0x00000000u, 0x41200000u, 0x00000000u, 0x3e800000u, 0x41300000u, 0x00000000u, 0x3f800000u, 0x41400000u, 0x00000000u, 0x00000000u, 0x41500000u, 0x00000000u, 0x3fe00000u, 0x41600000u, 0x00000000u, 0x3fe00000u, 0x41700000u, 0x00000000u, 0x00000000u, 0x41800000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x3f400000u, 0x3f800000u, 0x3f800000u, 0x3fa00000u, 0x40000000u, 0x3f800000u, 0x00000000u, 0x40400000u, 0x3f800000u, 0x3fc00000u, 0x40800000u, 0x3f800000u, 0x3fa00000u, 0x40a00000u, 0x3f800000u, 0x3fc00000u, 0x40c00000u, 0x3f800000u, 0x00000000u, 0x40e00000u, 0x3f800000u, 0x3fa00000u, 0x41000000u, 0x3f800000u, 0x3f400000u, 0x41100000u, 0x3f800000u, 0x3f400000u, 0x41200000u, 0x3f800000u, 0x3fa00000u, 0x41300000u, 0x3f800000u, 0x00000000u, 0x41400000u, 0x3f800000u, 0x3fc00000u, 0x41500000u, 0x3f800000u, 0x3fa00000u, 0x41600000u, 0x3f800000u, 0x3fc00000u, 0x41700000u, 0x3f800000u, 0x00000000u, 0x41800000u, 0x3f800000u, 0x3fa00000u, 0x00000000u, 0x40000000u, 0x3fc00000u, 0x3f800000u, 0x40000000u, 0x00000000u, 0x40000000u, 0x40000000u, 0x3fa00000u, 0x40400000u, 0x40000000u, 0x3f400000u, 0x40800000u, 0x40000000u, 0x3f400000u, 0x40a00000u, 0x40000000u, 0x3fa00000u, 0x40c00000u, 0x40000000u, 0x00000000u, 0x40e00000u, 0x40000000u, 0x3fc00000u, 0x41000000u, 0x40000000u, 0x3fa00000u, 0x41100000u, 0x40000000u, 0x3fc00000u, 0x41200000u, 0x40000000u, 0x00000000u, 0x41300000u, 0x40000000u, 0x3fa00000u, 0x41400000u, 0x40000000u, 0x3f400000u, 0x41500000u, 0x40000000u, 0x3f400000u, 0x41600000u, 0x40000000u, 0x3fa00000u, 0x41700000u, 0x40000000u, 0x00000000u, 0x41800000u, 0x40000000u, 0x3fc00000u, 0x00000000u, 0x40400000u, 0x00000000u, 0x3f800000u, 0x40400000u, 0x3f800000u, 0x40000000u, 0x40400000u, 0x3e800000u, 0x40400000u, 0x40400000u, 0x00000000u, 0x40800000u, 0x40400000u, 0x3e800000u, 0x40a00000u, 0x40400000u, 0x3f800000u, 0x40c00000u, 0x40400000u, 0x00000000u, 0x40e00000u, 0x40400000u, 0x3fe00000u, 0x41000000u, 0x40400000u, 0x3fe00000u, 0x41100000u, 0x40400000u, 0x00000000u, 0x41200000u, 0x40400000u, 0x3f800000u, 0x41300000u, 0x40400000u, 0x3e800000u, 0x41400000u, 0x40400000u, 0x00000000u, 0x41500000u, 0x40400000u, 0x3e800000u, 0x41600000u, 0x40400000u, 0x3f800000u, 0x41700000u, 0x40400000u, 0x00000000u, 0x41800000u, 0x40400000u, 0x3fe00000u, 0x00000000u, 0x40800000u, 0x3f400000u, 0x3f800000u, 0x40800000u, 0x40000000u, 0x40000000u, 0x40800000u, 0x3fc00000u, 0x40400000u, 0x40800000u, 0x3fc00000u, 0x40800000u, 0x40800000u, 0x40000000u, 0x40a00000u, 0x40800000u, 0x3f400000u, 0x40c00000u, 0x40800000u, 0x00000000u, 0x40e00000u, 0x40800000u, 0x40000000u, 0x41000000u, 0x40800000u, 0x00000000u, 0x41100000u, 0x40800000u, 0x3f400000u, 0x41200000u, 0x40800000u, 0x40000000u, 0x41300000u, 0x40800000u, 0x3fc00000u, 0x41400000u, 0x40800000u, 0x3fc00000u, 0x41500000u, 0x40800000u, 0x40000000u, 0x41600000u, 0x40800000u, 0x3f400000u, 0x41700000u, 0x40800000u, 0x00000000u, 0x41800000u, 0x40800000u, 0x40000000u, 0x00000000u, 0x40a00000u, 0x3fc00000u, 0x3f800000u, 0x40a00000u, 0x3f400000u, 0x40000000u, 0x40a00000u, 0x3f000000u, 0x40400000u, 0x40a00000u, 0x3f400000u, 0x40800000u, 0x40a00000u, 0x3fc00000u, 0x40a00000u, 0x40a00000u, 0x3f000000u, 0x40c00000u, 0x40a00000u, 0x00000000u, 0x40e00000u, 0x40a00000u, 0x00000000u, 0x41000000u, 0x40a00000u, 0x3f000000u, 0x41100000u, 0x40a00000u, 0x3fc00000u, 0x41200000u, 0x40a00000u, 0x3f400000u, 0x41300000u, 0x40a00000u, 0x3f000000u, 0x41400000u, 0x40a00000u, 0x3f400000u, 0x41500000u, 0x40a00000u, 0x3fc00000u, 0x41600000u, 0x40a00000u, 0x3f000000u, 0x41700000u, 0x40a00000u, 0x00000000u, 0x41800000u, 0x40a00000u, 0x00000000u, 0x00000000u, 0x40c00000u, 0x00000000u, 0x3f800000u, 0x40c00000u, 0x3fe00000u, 0x40000000u, 0x40c00000u, 0x3fe00000u, 0x40400000u, 0x40c00000u, 0x00000000u, 0x40800000u, 0x40c00000u, 0x3f800000u, 0x40a00000u, 0x40c00000u, 0x3e800000u, 0x40c00000u, 0x40c00000u, 0x00000000u, 0x40e00000u, 0x40c00000u, 0x3e800000u, 0x41000000u, 0x40c00000u, 0x3f800000u, 0x41100000u, 0x40c00000u, 0x00000000u, 0x41200000u, 0x40c00000u, 0x3fe00000u, 0x41300000u, 0x40c00000u, 0x3fe00000u, 0x41400000u, 0x40c00000u, 0x00000000u, 0x41500000u, 0x40c00000u, 0x3f800000u, 0x41600000u, 0x40c00000u, 0x3e800000u, 0x41700000u, 0x40c00000u, 0x00000000u, 0x41800000u, 0x40c00000u, 0x3e800000u, 0x00000000u, 0x40e00000u, 0x3f400000u, 0x3f800000u, 0x40e00000u, 0x3f000000u, 0x40000000u, 0x40e00000u, 0x3f400000u, 0x40400000u, 0x40e00000u, 0x3fc00000u, 0x40800000u, 0x40e00000u, 0x3f000000u, 0x40a00000u, 0x40e00000u, 0x00000000u, 0x40c00000u, 0x40e00000u, 0x00000000u, 0x40e00000u, 0x40e00000u, 0x3f000000u, 0x41000000u, 0x40e00000u, 0x3fc00000u, 0x41100000u, 0x40e00000u, 0x3f400000u, 0x41200000u, 0x40e00000u, 0x3f000000u, 0x41300000u, 0x40e00000u, 0x3f400000u, 0x41400000u, 0x40e00000u, 0x3fc00000u, 0x41500000u, 0x40e00000u, 0x3f000000u, 0x41600000u, 0x40e00000u, 0x00000000u, 0x41700000u, 0x40e00000u, 0x00000000u, 0x41800000u, 0x40e00000u, 0x3f000000u, 0x00000000u, 0x41000000u, 0x3fc00000u, 0x3f800000u, 0x41000000u, 0x3fc00000u, 0x40000000u, 0x41000000u, 0x40000000u, 0x40400000u, 0x41000000u, 0x3f400000u, 0x40800000u, 0x41000000u, 0x00000000u, 0x40a00000u, 0x41000000u, 0x40000000u, 0x40c00000u, 0x41000000u, 0x00000000u, 0x40e00000u, 0x41000000u, 0x3f400000u, 0x41000000u, 0x41000000u, 0x40000000u, 0x41100000u, 0x41000000u, 0x3fc00000u, 0x41200000u, 0x41000000u, 0x3fc00000u, 0x41300000u, 0x41000000u, 0x40000000u, 0x41400000u, 0x41000000u, 0x3f400000u, 0x41500000u, 0x41000000u, 0x00000000u, 0x41600000u, 0x41000000u, 0x40000000u, 0x41700000u, 0x41000000u, 0x00000000u, 0x41800000u, 0x41000000u, 0x3f400000u, 0x00000000u, 0x41100000u, 0x00000000u, 0x3f800000u, 0x41100000u, 0x3e800000u, 0x40000000u, 0x41100000u, 0x3f800000u, 0x40400000u, 0x41100000u, 0x00000000u, 0x40800000u, 0x41100000u, 0x3fe00000u, 0x40a00000u, 0x41100000u, 0x3fe00000u, 0x40c00000u, 0x41100000u, 0x00000000u, 0x40e00000u, 0x41100000u, 0x3f800000u, 0x41000000u, 0x41100000u, 0x3e800000u, 0x41100000u, 0x41100000u, 0x00000000u, 0x41200000u, 0x41100000u, 0x3e800000u, 0x41300000u, 0x41100000u, 0x3f800000u, 0x41400000u, 0x41100000u, 0x00000000u, 0x41500000u, 0x41100000u, 0x3fe00000u, 0x41600000u, 0x41100000u, 0x3fe00000u, 0x41700000u, 0x41100000u, 0x00000000u, 0x41800000u, 0x41100000u, 0x3f800000u, 0x00000000u, 0x41200000u, 0x3f400000u, 0x3f800000u, 0x41200000u, 0x3fa00000u, 0x40000000u, 0x41200000u, 0x00000000u, 0x40400000u, 0x41200000u, 0x3fc00000u, 0x40800000u, 0x41200000u, 0x3fa00000u, 0x40a00000u, 0x41200000u, 0x3fc00000u, 0x40c00000u, 0x41200000u, 0x00000000u, 0x40e00000u, 0x41200000u, 0x3fa00000u, 0x41000000u, 0x41200000u, 0x3f400000u, 0x41100000u, 0x41200000u, 0x3f400000u, 0x41200000u, 0x41200000u, 0x3fa00000u, 0x41300000u, 0x41200000u, 0x00000000u, 0x41400000u, 0x41200000u, 0x3fc00000u, 0x41500000u, 0x41200000u, 0x3fa00000u, 0x41600000u, 0x41200000u, 0x3fc00000u, 0x41700000u, 0x41200000u, 0x00000000u, 0x41800000u, 0x41200000u, 0x3fa00000u, 0x00000000u, 0x41300000u, 0x3fc00000u, 0x3f800000u, 0x41300000u, 0x00000000u, 0x40000000u, 0x41300000u, 0x3fa00000u, 0x40400000u, 0x41300000u, 0x3f400000u, 0x40800000u, 0x41300000u, 0x3f400000u, 0x40a00000u, 0x41300000u, 0x3fa00000u, 0x40c00000u, 0x41300000u, 0x00000000u, 0x40e00000u, 0x41300000u, 0x3fc00000u, 0x41000000u, 0x41300000u, 0x3fa00000u, 0x41100000u, 0x41300000u, 0x3fc00000u, 0x41200000u, 0x41300000u, 0x00000000u, 0x41300000u, 0x41300000u, 0x3fa00000u, 0x41400000u, 0x41300000u, 0x3f400000u, 0x41500000u, 0x41300000u, 0x3f400000u, 0x41600000u, 0x41300000u, 0x3fa00000u, 0x41700000u, 0x41300000u, 0x00000000u, 0x41800000u, 0x41300000u, 0x3fc00000u, 0x00000000u, 0x41400000u, 0x00000000u, 0x3f800000u, 0x41400000u, 0x3f800000u, 0x40000000u, 0x41400000u, 0x3e800000u, 0x40400000u, 0x41400000u, 0x00000000u, 0x40800000u, 0x41400000u, 0x3e800000u, 0x40a00000u, 0x41400000u, 0x3f800000u, 0x40c00000u, 0x41400000u, 0x00000000u, 0x40e00000u, 0x41400000u, 0x3fe00000u, 0x41000000u, 0x41400000u, 0x3fe00000u, 0x41100000u, 0x41400000u, 0x00000000u, 0x41200000u, 0x41400000u, 0x3f800000u, 0x41300000u, 0x41400000u, 0x3e800000u, 0x41400000u, 0x41400000u, 0x00000000u, 0x41500000u, 0x41400000u, 0x3e800000u, 0x41600000u, 0x41400000u, 0x3f800000u, 0x41700000u, 0x41400000u, 0x00000000u, 0x41800000u, 0x41400000u, 0x3fe00000u, 0x00000000u, 0x41500000u, 0x3f400000u, 0x3f800000u, 0x41500000u, 0x40000000u, 0x40000000u, 0x41500000u, 0x3fc00000u, 0x40400000u, 0x41500000u, 0x3fc00000u, 0x40800000u, 0x41500000u, 0x40000000u, 0x40a00000u, 0x41500000u, 0x3f400000u, 0x40c00000u, 0x41500000u, 0x00000000u, 0x40e00000u, 0x41500000u, 0x40000000u, 0x41000000u, 0x41500000u, 0x00000000u, 0x41100000u, 0x41500000u, 0x3f400000u, 0x41200000u, 0x41500000u, 0x40000000u, 0x41300000u, 0x41500000u, 0x3fc00000u, 0x41400000u, 0x41500000u, 0x3fc00000u, 0x41500000u, 0x41500000u, 0x40000000u, 0x41600000u, 0x41500000u, 0x3f400000u, 0x41700000u, 0x41500000u, 0x00000000u, 0x41800000u, 0x41500000u, 0x40000000u, 0x00000000u, 0x41600000u, 0x3fc00000u, 0x3f800000u, 0x41600000u, 0x3f400000u, 0x40000000u, 0x41600000u, 0x3f000000u, 0x40400000u, 0x41600000u, 0x3f400000u, 0x40800000u, 0x41600000u, 0x3fc00000u, 0x40a00000u, 0x41600000u, 0x3f000000u, 0x40c00000u, 0x41600000u, 0x00000000u, 0x40e00000u, 0x41600000u, 0x00000000u, 0x41000000u, 0x41600000u, 0x3f000000u, 0x41100000u, 0x41600000u, 0x3fc00000u, 0x41200000u, 0x41600000u, 0x3f400000u, 0x41300000u, 0x41600000u, 0x3f000000u, 0x41400000u, 0x41600000u, 0x3f400000u, 0x41500000u, 0x41600000u, 0x3fc00000u, 0x41600000u, 0x41600000u, 0x3f000000u, 0x41700000u, 0x41600000u, 0x00000000u, 0x41800000u, 0x41600000u, 0x00000000u, 0x00000000u, 0x41700000u, 0x00000000u, 0x3f800000u, 0x41700000u, 0x3fe00000u, 0x40000000u, 0x41700000u, 0x3fe00000u, 0x40400000u, 0x41700000u, 0x00000000u, 0x40800000u, 0x41700000u, 0x3f800000u, 0x40a00000u, 0x41700000u, 0x3e800000u, 0x40c00000u, 0x41700000u, 0x00000000u, 0x40e00000u, 0x41700000u, 0x3e800000u, 0x41000000u, 0x41700000u, 0x3f800000u, 0x41100000u, 0x41700000u, 0x00000000u, 0x41200000u, 0x41700000u, 0x3fe00000u, 0x41300000u, 0x41700000u, 0x3fe00000u, 0x41400000u, 0x41700000u, 0x00000000u, 0x41500000u, 0x41700000u, 0x3f800000u, 0x41600000u, 0x41700000u, 0x3e800000u, 0x41700000u, 0x41700000u, 0x00000000u, 0x41800000u, 0x41700000u, 0x3e800000u, 0x00000000u, 0x41800000u, 0x3f400000u, 0x3f800000u, 0x41800000u, 0x3f000000u, 0x40000000u, 0x41800000u, 0x3f400000u, 0x40400000u, 0x41800000u, 0x3fc00000u, 0x40800000u, 0x41800000u, 0x3f000000u, 0x40a00000u, 0x41800000u, 0x00000000u, 0x40c00000u, 0x41800000u, 0x00000000u, 0x40e00000u, 0x41800000u, 0x3f000000u, 0x41000000u, 0x41800000u, 0x3fc00000u, 0x41100000u, 0x41800000u, 0x3f400000u, 0x41200000u, 0x41800000u, 0x3f000000u, 0x41300000u, 0x41800000u, 0x3f400000u, 0x41400000u, 0x41800000u, 0x3fc00000u, 0x41500000u, 0x41800000u, 0x3f000000u, 0x41600000u, 0x41800000u, 0x00000000u, 0x41700000u, 0x41800000u, 0x00000000u, 0x41800000u, 0x41800000u, 0x3f000000u
	};
static const unsigned kMeshTris2i0[1536] =
	{
	0, 1, 17, 1, 18, 17, 1, 2, 19, 1, 19, 18, 2, 3, 19, 3, 20, 19, 3, 4, 21, 3, 21, 20, 4, 5, 21, 5, 22, 21, 5, 6, 23, 5, 23, 22, 6, 7, 23, 7, 24, 23, 7, 8, 25, 7, 25, 24, 8, 9, 25, 9, 26, 25, 9, 10, 27, 9, 27, 26, 10, 11, 27, 11, 28, 27, 11, 12, 29, 11, 29, 28, 12, 13, 29, 13, 30, 29, 13, 14, 31, 13, 31, 30, 14, 15, 31, 15, 32, 31, 15, 16, 33, 15, 33, 32, 17, 18, 35, 17, 35, 34, 18, 19, 35, 19, 36, 35, 19, 20, 37, 19, 37, 36, 20, 21, 37, 21, 38, 37, 21, 22, 39, 21, 39, 38, 22, 23, 39, 23, 40, 39, 23, 24, 41, 23, 41, 40, 24, 25, 41, 25, 42, 41, 25, 26, 43, 25, 43, 42, 26, 27, 43, 27, 44, 43, 27, 28, 45, 27, 45, 44, 28, 29, 45, 29, 46, 45, 29, 30, 47, 29, 47, 46, 30, 31, 47, 31, 48, 47, 31, 32, 49, 31, 49, 48, 32, 33, 49, 33, 50, 49, 34, 35, 51, 35, 52, 51, 35, 36, 53, 35, 53, 52, 36, 37, 53, 37, 54, 53, 37, 38, 55, 37, 55, 54, 38, 39, 55, 39, 56, 55, 39, 40, 57, 39, 57, 56, 40, 41, 57, 41, 58, 57, 41, 42, 59, 41, 59, 58, 42, 43, 59, 43, 60, 59, 43, 44, 61, 43, 61, 60, 44, 45, 61, 45, 62, 61, 45, 46, 63, 45, 63, 62, 46, 47, 63, 47, 64, 63, 47, 48, 65, 47, 65, 64, 48, 49, 65, 49, 66, 65, 49, 50, 67, 49, 67, 66, 51, 52, 69, 51, 69, 68, 52, 53, 69, 53, 70, 69, 53, 54, 71, 53, 71, 70, 54, 55, 71, 55, 72, 71, 55, 56, 73, 55, 73, 72, 56, 57, 73, 57, 74, 73, 57, 58, 75, 57, 75, 74, 58, 59, 75, 59, 76, 75, 59, 60, 77, 59, 77, 76, 60, 61, 77, 61, 78, 77, 61, 62, 79, 61, 79, 78, 62, 63, 79, 63, 80, 79, 63, 64, 81, 63, 81, 80, 64, 65, 81, 65, 82, 81, 65, 66, 83, 65, 83, 82, 66, 67, 83, 67, 84, 83, 68, 69, 85, 69, 86, 85, 69, 70, 87, 69, 87, 86, 70, 71, 87, 71, 88, 87, 71, 72, 89, 71, 89, 88, 72, 73, 89, 73, 90, 89, 73, 74, 91, 73, 91, 90, 74, 75, 91, 75, 92, 91, 75, 76, 93, 75, 93, 92, 76, 77, 93, 77, 94, 93, 77, 78, 95, 77, 95, 94, 78, 79, 95, 79, 96, 95, 79, 80, 97, 79, 97, 96, 80, 81, 97, 81, 98, 97, 81, 82, 99, 81, 99, 98, 82, 83, 99, 83, 100, 99, 83, 84, 101, 83, 101, 100, 85, 86, 103, 85, 103, 102, 86, 87, 103, 87, 104, 103, 87, 88, 105, 87, 105, 104, 88, 89, 105, 89, 106, 105, 89, 90, 107, 89, 107, 106, 90, 91, 107, 91, 108, 107, 91, 92, 109, 91, 109, 108, 92, 93, 109, 93, 110, 109, 93, 94, 111, 93, 111, 110, 94, 95, 111, 95, 112, 111, 95, 96, 113, 95, 113, 112, 96, 97, 113, 97, 114, 113, 97, 98, 115, 97, 115, 114, 98, 99, 115, 99, 116, 115, 99, 100, 117, 99, 117, 116, 100, 101, 117, 101, 118, 117, 102, 103, 119, 103, 120, 119, 103, 104, 121, 103, 121, 120, 104, 105, 121, 105, 122, 121, 105, 106, 123, 105, 123, 122, 106, 107, 123, 107, 124, 123, 107, 108, 125, 107, 125, 124, 108, 109, 125, 109, 126, 125, 109, 110, 127, 109, 127, 126, 110, 111, 127, 111, 128, 127, 111, 112, 129, 111, 129, 128, 112, 113, 129, 113, 130, 129, 113, 114, 131, 113, 131, 130, 114, 115, 131, 115, 132, 131, 115, 116, 133, 115, 133, 132, 116, 117, 133, 117, 134, 133, 117, 118, 135, 117, 135, 134, 119, 120, 137, 119, 137, 136, 120, 121, 137, 121, 138, 137, 121, 122, 139, 121, 139, 138, 122, 123, 139, 123, 140, 139, 123, 124, 141, 123, 141, 140, 124, 125, 141, 125, 142, 141, 125, 126, 143, 125, 143, 142, 126, 127, 143, 127, 144, 143, 127, 128, 145, 127, 145, 144, 128, 129, 145, 129, 146, 145, 129, 130, 147, 129, 147, 146, 130, 131, 147, 131, 148, 147, 131, 132, 149, 131, 149, 148, 132, 133, 149, 133, 150, 149, 133, 134, 151, 133, 151, 150, 134, 135, 151, 135, 152, 151, 136, 137, 153, 137, 154, 153, 137, 138, 155, 137, 155, 154, 138, 139, 155, 139, 156, 155, 139, 140, 157, 139, 157, 156, 140, 141, 157, 141, 158, 157, 141, 142, 159, 141, 159, 158, 142, 143, 159, 143, 160, 159, 143, 144, 161, 143, 161, 160, 144, 145, 161, 145, 162, 161, 145, 146, 163, 145, 163, 162, 146, 147, 163, 147, 164, 163, 147, 148, 165, 147, 165, 164, 148, 149, 165, 149, 166, 165, 149, 150, 167, 149, 167, 166, 150, 151, 167, 151, 168, 167, 151, 152, 169, 151, 169, 168, 153, 154, 171, 153, 171, 170, 154, 155, 171, 155, 172, 171, 155, 156, 173, 155, 173, 172, 156, 157, 173, 157, 174, 173, 157, 158, 175, 157, 175, 174, 158, 159, 175, 159, 176, 175, 159, 160, 177, 159, 177, 176, 160, 161, 177, 161, 178, 177, 161, 162, 179, 161, 179, 178, 162, 163, 179, 163, 180, 179, 163, 164, 181, 163, 181, 180, 164, 165, 181, 165, 182, 181, 165, 166, 183, 165, 183, 182, 166, 167, 183, 167, 184, 183, 167, 168, 185, 167, 185, 184, 168, 169, 185, 169, 186, 185, 170, 171, 187, 171, 188, 187, 171, 172, 189, 171, 189, 188, 172, 173, 189, 173, 190, 189, 173, 174, 191, 173, 191, 190, 174, 175, 191, 175, 192, 191, 175, 176, 193, 175, 193, 192, 176, 177, 193, 177, 194, 193, 177, 178, 195, 177, 195, 194, 178, 179, 195, 179, 196, 195, 179, 180, 197, 179, 197, 196, 180, 181, 197, 181, 198, 197, 181, 182, 199, 181, 199, 198, 182, 183, 199, 183, 200, 199, 183, 184, 201, 183, 201, 200, 184, 185, 201, 185, 202, 201, 185, 186, 203, 185, 203, 202, 187, 188, 205, 187, 205, 204, 188, 189, 205, 189, 206, 205, 189, 190, 207, 189, 207, 206, 190, 191, 207, 191, 208, 207, 191, 192, 209, 191, 209, 208, 192, 193, 209, 193, 210, 209, 193, 194, 211, 193, 211, 210, 194, 195, 211, 195, 212, 211, 195, 196, 213, 195, 213, 212, 196, 197, 213, 197, 214, 213, 197, 198, 215, 197, 215, 214, 198, 199, 215, 199, 216, 215, 199, 200, 217, 199, 217, 216, 200, 201, 217, 201, 218, 217, 201, 202, 219, 201, 219, 218, 202, 203, 219, 203, 220, 219, 204, 205, 221, 205, 222, 221, 205, 206, 223, 205, 223, 222, 206, 207, 223, 207, 224, 223, 207, 208, 225, 207, 225, 224, 208, 209, 225, 209, 226, 225, 209, 210, 227, 209, 227, 226, 210, 211, 227, 211, 228, 227, 211, 212, 229, 211, 229, 228, 212, 213, 229, 213, 230, 229, 213, 214, 231, 213, 231, 230, 214, 215, 231, 215, 232, 231, 215, 216, 233, 215, 233, 232, 216, 217, 233, 217, 234, 233, 217, 218, 235, 217, 235, 234, 218, 219, 235, 219, 236, 235, 219, 220, 237, 219, 237, 236, 221, 222, 239, 221, 239, 238, 222, 223, 239, 223, 240, 239, 223, 224, 241, 223, 241, 240, 224, 225, 241, 225, 242, 241, 225, 226, 243, 225, 243, 242, 226, 227, 243, 227, 244, 243, 227, 228, 245, 227, 245, 244, 228, 229, 245, 229, 246, 245, 229, 230, 247, 229, 247, 246, 230, 231, 247, 231, 248, 247, 231, 232, 249, 231, 249, 248, 232, 233, 249, 233, 250, 249, 233, 234, 251, 233, 251, 250, 234, 235, 251, 235, 252, 251, 235, 236, 253, 235, 253, 252, 236, 237, 253, 237, 254, 253, 238, 239, 255, 239, 256, 255, 239, 240, 257, 239, 257, 256, 240, 241, 257, 241, 258, 257, 241, 242, 259, 241, 259, 258, 242, 243, 259, 243, 260, 259, 243, 244, 261, 243, 261, 260, 244, 245, 261, 245, 262, 261, 245, 246, 263, 245, 263, 262, 246, 247, 263, 247, 264, 263, 247, 248, 265, 247, 265, 264, 248, 249, 265, 249, 266, 265, 249, 250, 267, 249, 267, 266, 250, 251, 267, 251, 268, 267, 251, 252, 269, 251, 269, 268, 252, 253, 269, 253, 270, 269, 253, 254, 271, 253, 271, 270, 255, 256, 273, 255, 273, 272, 256, 257, 273, 257, 274, 273, 257, 258, 275, 257, 275, 274, 258, 259, 275, 259, 276, 275, 259, 260, 277, 259, 277, 276, 260, 261, 277, 261, 278, 277, 261, 262, 279, 261, 279, 278, 262, 263, 279, 263, 280, 279, 263, 264, 281, 263, 281, 280, 264, 265, 281, 265, 282, 281, 265, 266, 283, 265, 283, 282, 266, 267, 283, 267, 284, 283, 267, 268, 285, 267, 285, 284, 268, 269, 285, 269, 286, 285, 269, 270, 287, 269, 287, 286, 270, 271, 287, 271, 288, 287
	};
static const unsigned kMeshPlanes2i0[2048] =
	{
	0xbe48d2abu, 0xbf169e00u, 0x3f48d2abu, 0x80000000u, 0xbeaaaaabu, 0xbf2aaaabu, 0x3f2aaaabu, 0x3e2aaaabu, 0xbeefe206u, 0x3f1fec04u, 0x3f1fec04u, 0x3e9fec04u, 0x3f298a47u, 0xbf07a1d2u, 0x3f07a1d2u, 0xbf4b72bcu, 0x3f13cd3au, 0x3f13cd3au, 0x3f13cd3au, 0xbfddb3d7u, 0xbf23bcf7u, 0xbf23bcf7u, 0x3eda514au, 0x3ff59b72u, 0xbf57bb40u, 0x3e768cdcu, 0x3ef68cdcu, 0x4021cc70u, 0x3e0ca83fu, 0xbf52fc5fu, 0x3f0ca83fu, 0xbed2fc5eu, 0x80000000u, 0x3ee4f92eu, 0x3f64f92eu, 0xbfc85a08u, 0xbe715befu, 0x3e715befu, 0x3f715befu, 0xbef15befu, 0x3f5e452fu, 0x00000000u, 0x3efe05ecu, 0xc0a6b3e3u, 0x3f52fc5fu, 0x3e0ca83fu, 0x3f0ca83fu, 0xc0a2a289u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x4087c3b6u, 0xbf45821fu, 0xbe1e01b3u, 0x3f1e01b3u, 0x409911a5u, 0x3f0e9d30u, 0xbebe26ebu, 0x3f3e26ebu, 0xc0948e67u, 0x3edf7483u, 0xbe5f7483u, 0x3f5f7483u, 0xc07b6313u, 0x3e5f7483u, 0xbedf7483u, 0x3f5f7483u, 0xbffb6313u, 0x00000000u, 0xbf19999au, 0x3f4ccccdu, 0x80000000u, 0xbe32416au, 0xbf32416au, 0x3f32416au, 0x3fc88997u, 0xbebe26ebu, 0xbf0e9d30u, 0x3f3e26ebu, 0x4055ebc8u, 0xbeefe206u, 0xbf1fec04u, 0x3f1fec04u, 0x4090ede4u, 0x3f298a47u, 0x3f07a1d2u, 0x3f07a1d2u, 0xc0fa125cu, 0x3ef85b42u, 0xbf3a4472u, 0x3ef85b42u, 0xc0ba4472u, 0xbf3a4472u, 0x3ef85b42u, 0x3ef85b42u, 0x40f09869u, 0xbf324f88u, 0xbf18d674u, 0x3ecbc89bu, 0x4105bba6u, 0x3e5f7483u, 0x3edf7483u, 0x3f5f7483u, 0xc08ba8d2u, 0x80000000u, 0x3e785b42u, 0x3f785b42u, 0xbfd94fdau, 0xbe5f7483u, 0x3edf7483u, 0x3f5f7483u, 0x3fa79762u, 0x3f5c9478u, 0x3dfc1764u, 0x3efc1764u, 0xc14ecb30u, 0x3f550140u, 0x80000000u, 0x3f0e00d5u, 0xc147b12cu, 0xbf32416au, 0xbe32416au, 0x3f32416au, 0x41271d53u, 0xbf47e705u, 0x00000000u, 0x3f1fec04u, 0x413b6895u, 0xbe98a61fu, 0x3f3ecfa6u, 0x3f18a61fu, 0xbf98a61fu, 0x3f44aa26u, 0xbec4aa26u, 0x3f031c1au, 0xb3000000u, 0x3f1d8e9fu, 0x3f1d8e9fu, 0x3efc1764u, 0xbfec55eeu, 0xbf1d8e9fu, 0xbf1d8e9fu, 0x3efc1764u, 0x3fec55eeu, 0xbf44aa26u, 0x3ec4aa26u, 0x3f031c1au, 0x3f937f9cu, 0x3e98a61fu, 0xbf3ecfa6u, 0x3f18a61fu, 0x3e18a61cu, 0x3e48d2abu, 0x3f169e00u, 0x3f48d2abu, 0xc0169e00u, 0x00000000u, 0x3ee4f92eu, 0x3f64f92eu, 0xbfc85a08u, 0xbe715befu, 0x3e715befu, 0x3f715befu, 0xbef15befu, 0xbed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0x3e5105ecu, 0x3f52fc5fu, 0x3e0ca83fu, 0x3f0ca83fu, 0xc0a2a289u, 0x3f47e705u, 0x80000000u, 0x3f1fec04u, 0xc095ed44u, 0xbf45821fu, 0xbe1e01b3u, 0x3f1e01b3u, 0x409911a5u, 0xbf550140u, 0x00000000u, 0x3f0e00d5u, 0x409fc0f0u, 0x3edf7483u, 0xbe5f7483u, 0x3f5f7483u, 0xc07b6313u, 0x3e5f7483u, 0xbedf7483u, 0x3f5f7483u, 0xbffb6313u, 0x00000000u, 0xbf19999au, 0x3f4ccccdu, 0x32800000u, 0xbe5f7483u, 0xbedf7483u, 0x3f5f7483u, 0x3fc385f3u, 0xbebe26ebu, 0xbf0e9d30u, 0x3f3e26ebu, 0x4055ebc8u, 0x3f2f0b1fu, 0x3f11de9au, 0x3ee9642au, 0xc0ff458du, 0x3f1d8e9fu, 0xbf1d8e9fu, 0x3efc1764u, 0xc0c4f247u, 0xbf1d8e9fu, 0x3f1d8e9fu, 0x3efc1764u, 0x409d8e9fu, 0xbf2f0b1fu, 0xbf11de9au, 0x3ee9642au, 0x4101758fu, 0x3ebe26ebu, 0x3f0e9d30u, 0x3f3e26ebu, 0xc0c41822u, 0x3e5f7483u, 0x3edf7483u, 0x3f5f7483u, 0xc08ba8d2u, 0x00000000u, 0x3f19999au, 0x3f4ccccdu, 0xbfe66667u, 0xbe5f7483u, 0x3edf7483u, 0x3f5f7483u, 0x3fa79762u, 0xbedf7483u, 0x3e5f7483u, 0x3f5f7483u, 0x4092a476u, 0x3f550140u, 0x00000000u, 0x3f0e00d5u, 0xc147b12cu, 0x3f45821fu, 0x3e1e01b3u, 0x3f1e01b3u, 0xc13e1a0bu, 0xbf47e705u, 0x00000000u, 0x3f1fec04u, 0x413b6895u, 0xbf52fc5fu, 0xbe0ca83fu, 0x3f0ca83fu, 0x414a31dbu, 0x3f23bcf7u, 0x3f23bcf7u, 0x3eda514au, 0xbff59b73u, 0xbf13cd3au, 0xbf13cd3au, 0x3f13cd3au, 0x3fddb3d7u, 0xbf298a47u, 0x3f07a1d2u, 0x3f07a1d2u, 0xbecb72bau, 0x3eefe206u, 0xbf1fec04u, 0x3f1fec04u, 0x3f47e705u, 0x3eaaaaabu, 0x3f2aaaabu, 0x3f2aaaabu, 0xc0355556u, 0x3e48d2abu, 0x3f169e00u, 0x3f48d2abu, 0xc0169e00u, 0x80000000u, 0x3ee4f92eu, 0x3f64f92eu, 0xbfc85a08u, 0xbe48d2abu, 0x3f169e00u, 0x3f48d2abu, 0xbf969e00u, 0xbed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0x3e5105ecu, 0xbf169e00u, 0x3e48d2abu, 0x3f48d2abu, 0x3fc8d2aau, 0x3f47e705u, 0x00000000u, 0x3f1fec04u, 0xc095ed44u, 0x3f32416au, 0x3e32416au, 0x3f32416au, 0xc0966731u, 0xbf550140u, 0x00000000u, 0x3f0e00d5u, 0x409fc0f0u, 0xbf5c9478u, 0xbdfc1764u, 0x3efc1764u, 0x40b14073u, 0x3e5f7483u, 0xbedf7483u, 0x3f5f7483u, 0xbffb6313u, 0x00000000u, 0xbe785b42u, 0x3f785b42u, 0xbf785b42u, 0xbe5f7483u, 0xbedf7483u, 0x3f5f7483u, 0x3fc385f3u, 0x3f324f88u, 0x3f18d674u, 0x3ecbc89bu, 0xc100f4f2u, 0x3f3a4472u, 0xbef85b42u, 0x3ef85b42u, 0xc0c9ca26u, 0xbef85b42u, 0x3f3a4472u, 0x3ef85b42u, 0x400bb355u, 0xbf298a47u, 0xbf07a1d2u, 0x3f07a1d2u, 0x40f5d54du, 0x3eefe206u, 0x3f1fec04u, 0x3f1fec04u, 0xc0e5e346u, 0x3ebe26ebu, 0x3f0e9d30u, 0x3f3e26ebu, 0xc0c41822u, 0x3e32416au, 0x3f32416au, 0x3f32416au, 0xc085b110u, 0x80000000u, 0x3f19999au, 0x3f4ccccdu, 0xbfe66667u, 0xbe5f7483u, 0x3edf7483u, 0x3f5f7483u, 0x3fa79762u, 0xbedf7483u, 0x3e5f7483u, 0x3f5f7483u, 0x4092a476u, 0xbf0e9d30u, 0x3ebe26ebu, 0x3f3e26ebu, 0x40be26ebu, 0x3f45821fu, 0x3e1e01b3u, 0x3f1e01b3u, 0xc13e1a0bu, 0x3f3504f3u, 0x80000000u, 0x3f3504f3u, 0xc129b4a4u, 0xbf52fc5fu, 0xbe0ca83fu, 0x3f0ca83fu, 0x414a31dbu, 0xbf5e452fu, 0x00000000u, 0x3efe05ecu, 0x415060dcu, 0xbf13cd3au, 0xbf13cd3au, 0x3f13cd3au, 0x3fddb3d7u, 0xbf3504f3u, 0xbed93924u, 0x3f10d0c3u, 0x3fa2eadbu, 0x3eefe206u, 0xbf1fec04u, 0x3f1fec04u, 0x3f47e705u, 0x3e98a61fu, 0xbf3ecfa6u, 0x3f18a61fu, 0x3fbecfa6u, 0x3e0ca83fu, 0xbf52fc5fu, 0x3f0ca83fu, 0x4003ddbbu, 0x00000000u, 0xbf47e705u, 0x3f1fec04u, 0x400bee84u, 0xbe0ca83fu, 0xbf52fc5fu, 0x3f0ca83fu, 0x40389cd3u, 0xbe768cdcu, 0xbf57bb40u, 0x3ef68cdcu, 0x4057bb40u, 0xbf169e00u, 0x3e48d2abu, 0x3f48d2abu, 0x3fc8d2aau, 0x3f06ec81u, 0xbf3ce4b5u, 0x3ed7e0cfu, 0x33900000u, 0x3f32416au, 0x3e32416au, 0x3f32416au, 0xc0966731u, 0x3f19999au, 0x80000000u, 0x3f4ccccdu, 0xc0666667u, 0xbf5c9478u, 0xbdfc1764u, 0x3efc1764u, 0x40b14073u, 0xbf64f92eu, 0x00000000u, 0x3ee4f92eu, 0x40abbae2u, 0x00000000u, 0xbe785b42u, 0x3f785b42u, 0xbf785b42u, 0x3f3450fcu, 0x3f1dc6ddu, 0x3eb450fcu, 0xc1019a35u, 0x3f5050d6u, 0xbeb28e6eu, 0x3eee133eu, 0xc0c8e03cu, 0xbeb28e6eu, 0x3f5050d6u, 0x3eee133eu, 0xbeee1340u, 0xbf1fec04u, 0xbeefe206u, 0x3f1fec04u, 0x40e0e3e6u, 0xbf298a47u, 0xbf07a1d2u, 0x3f07a1d2u, 0x40f5d54du, 0x3ed93924u, 0xbf3504f3u, 0x3f10d0c3u, 0xc02bf7e8u, 0x3eaaaaabu, 0xbf2aaaabu, 0x3f2aaaabu, 0xc0000000u, 0x3e1e01b3u, 0xbf45821fu, 0x3f1e01b3u, 0x3eed0288u, 0x00000000u, 0xbf550140u, 0x3f0e00d5u, 0x401fc0f0u, 0xbdfc1764u, 0xbf5c9478u, 0x3efc1764u, 0x4081fc10u, 0xbe88d677u, 0xbf4d41b3u, 0x3f08d677u, 0x40b3997cu, 0xbeb28e6eu, 0xbf5050d6u, 0x3eee133eu, 0x40db79bdu, 0x3f45821fu, 0x3e1e01b3u, 0x3f1e01b3u, 0xc13e1a0bu, 0x3f3504f3u, 0x00000000u, 0x3f3504f3u, 0xc129b4a4u, 0x3f169e00u, 0x3e48d2abu, 0x3f48d2abu, 0xc119c14bu, 0xbf5e452fu, 0x00000000u, 0x3efe05ecu, 0x415060dcu, 0xbf638e39u, 0xbde38e39u, 0x3ee38e39u, 0x415c71c7u, 0xbf3504f3u, 0xbed93924u, 0x3f10d0c3u, 0x3fa2eadbu, 0x3ed93924u, 0x3f3504f3u, 0x3f10d0c3u, 0xc08c4a3cu, 0x3eaaaaabu, 0x3f2aaaabu, 0x3f2aaaabu, 0xc08aaaabu, 0x3e1e01b3u, 0x3f45821fu, 0x3f1e01b3u, 0xc08f318au, 0x80000000u, 0x3f3504f3u, 0x3f3504f3u, 0xc078e6ceu, 0xbe48d2abu, 0x3f169e00u, 0x3f48d2abu, 0xc03c4580u, 0xbed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0xbfd105ecu, 0xbf03b5feu, 0x3f03b5feu, 0x3f2f9d53u, 0xbfc590fdu, 0x3f3ecfa6u, 0x3e98a61fu, 0x3f18a61fu, 0xc0abbae2u, 0x3f32416au, 0x3e32416au, 0x3f32416au, 0xc0966731u, 0x3f19999au, 0x00000000u, 0x3f4ccccdu, 0xc0666667u, 0x3edf7483u, 0x3e5f7483u, 0x3f5f7483u, 0xc06d6bcbu, 0xbf64f92eu, 0x00000000u, 0x3ee4f92eu, 0x40abbae2u, 0x00000000u, 0x3f64f92eu, 0x3ee4f92eu, 0xc08f1bbdu, 0x3f5f7483u, 0xbe5f7483u, 0x3edf7483u, 0xc0c385f3u, 0xbe5f7483u, 0x3f5f7483u, 0x3edf7483u, 0xc0358eaau, 0xbf0e9d30u, 0xbebe26ebu, 0x3f3e26ebu, 0x40be26ebu, 0xbf1fec04u, 0xbeefe206u, 0x3f1fec04u, 0x40e0e3e6u, 0xbf1d8e9fu, 0x3f1d8e9fu, 0x3efc1764u, 0x402d5015u, 0x3f03b5feu, 0xbf03b5feu, 0x3f2f9d53u, 0xc04590fdu, 0x3e98a61fu, 0x3f3ecfa6u, 0x3f18a61fu, 0xc0e4f92eu, 0x3e32416au, 0x3f32416au, 0x3f32416au, 0xc0b7d375u, 0x80000000u, 0x3f19999au, 0x3f4ccccdu, 0xc0666667u, 0xbe32416au, 0x3f32416au, 0x3f32416au, 0xbff519f2u, 0xbebe26ebu, 0x3f0e9d30u, 0x3f3e26ebu, 0x3f8e9d31u, 0xbf0e9d30u, 0x3ebe26ebu, 0x3f3e26ebu, 0x4088abf8u, 0x3f45821fu, 0x3e1e01b3u, 0x3f1e01b3u, 0xc13e1a0bu, 0x3f2aaaabu, 0x3eaaaaabu, 0x3f2aaaabu, 0xc1355556u, 0x3f169e00u, 0x3e48d2abu, 0x3f48d2abu, 0xc119c14bu, 0x3ee4f92eu, 0x80000000u, 0x3f64f92eu, 0xc0d6a99bu, 0xbf2aaaabu, 0x3f2aaaabu, 0x3eaaaaabu, 0x40eaaaabu, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0x3eefe206u, 0xbf1fec04u, 0x3f1fec04u, 0x400bee84u, 0xbf324f88u, 0x3f18d674u, 0x3ecbc89bu, 0xc06541aeu, 0x3e32416au, 0xbf32416au, 0x3f32416au, 0x4032416au, 0x00000000u, 0xbf47e705u, 0x3f1fec04u, 0x4065e346u, 0xbe48d2abu, 0x3f169e00u, 0x3f48d2abu, 0xc03c4580u, 0x3f3ce4b5u, 0xbf06ec81u, 0x3ed7e0cfu, 0x3f72dce7u, 0xbf03b5feu, 0x3f03b5feu, 0x3f2f9d53u, 0xbfc590fdu, 0xbf2aaaabu, 0x3eaaaaabu, 0x3f2aaaabu, 0x80000000u, 0x3f32416au, 0x3e32416au, 0x3f32416au, 0xc0966731u, 0x3f0e9d30u, 0x3ebe26ebu, 0x3f3e26ebu, 0xc0a6620eu, 0x3edf7483u, 0x3e5f7483u, 0x3f5f7483u, 0xc06d6bcbu, 0x3e785b42u, 0x80000000u, 0x3f785b42u, 0xbfba4472u, 0x00000000u, 0xbe785b42u, 0x3f785b42u, 0x3f9b3909u, 0xbe785b42u, 0x00000000u, 0x3f785b42u, 0x3fba4472u, 0xbedf7483u, 0xbe5f7483u, 0x3f5f7483u, 0x4084ad2eu, 0xbf0e9d30u, 0xbebe26ebu, 0x3f3e26ebu, 0x40be26ebu, 0xbef85b42u, 0x3f3a4472u, 0x3ef85b42u, 0xb4200000u, 0x3f2aaaabu, 0xbeaaaaabu, 0x3f2aaaabu, 0xc0800000u, 0x3ec4aa26u, 0x3f44aa26u, 0x3f031c1au, 0xc1010fa9u, 0xbf471c72u, 0xbee38e39u, 0x3ee38e39u, 0x411aaaabu, 0x3e1e01b3u, 0xbf45821fu, 0x3f1e01b3u, 0x3fed028bu, 0x00000000u, 0xbf3504f3u, 0x3f3504f3u, 0x40405542u, 0xbe1e01b3u, 0xbf45821fu, 0x3f1e01b3u, 0x40a7e1ceu, 0x3f5050d6u, 0x3eb28e6eu, 0x3eee133eu, 0xc13db755u, 0xbf0e9d30u, 0x3ebe26ebu, 0x3f3e26ebu, 0x4088abf9u, 0xbf1fec04u, 0x3eefe206u, 0x3f1fec04u, 0x4095ed44u, 0x3f2aaaabu, 0x3eaaaaabu, 0x3f2aaaabu, 0xc1355556u, 0x3f169e00u, 0x3e48d2abu, 0x3f48d2abu, 0xc119c14bu, 0x3ee4f92eu, 0x00000000u, 0x3f64f92eu, 0xc0d6a99bu, 0x3e715befu, 0x3e715befu, 0x3f715befu, 0xc09e6455u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0xbe715befu, 0xbe715befu, 0x3f715befu, 0x409e6455u, 0xbf5050d6u, 0xbeb28e6eu, 0x3eee133eu, 0x4005ead2u, 0x3e1e01b3u, 0x3f45821fu, 0x3f1e01b3u, 0xc0bba204u, 0x80000000u, 0x3f3504f3u, 0x3f3504f3u, 0xc0af5ccbu, 0xbe1e01b3u, 0x3f45821fu, 0x3f1e01b3u, 0xc0b1c1e9u, 0x3f471c72u, 0x3ee38e39u, 0x3ee38e39u, 0xc0a00000u, 0xbec4aa26u, 0xbf44aa26u, 0x3f031c1au, 0x40b85f84u, 0xbf2aaaabu, 0x3eaaaaabu, 0x3f2aaaabu, 0x80000000u, 0x3ef85b42u, 0xbf3a4472u, 0x3ef85b42u, 0x403a4472u, 0x3f0e9d30u, 0x3ebe26ebu, 0x3f3e26ebu, 0xc0a6620eu, 0x3edf7483u, 0x3e5f7483u, 0x3f5f7483u, 0xc06d6bcbu, 0x3e785b42u, 0x00000000u, 0x3f785b42u, 0xbfba4472u, 0x00000000u, 0x3e785b42u, 0x3f785b42u, 0xbfd94fdau, 0xbe785b42u, 0x00000000u, 0x3f785b42u, 0x3fba4472u, 0xbedf7483u, 0xbe5f7483u, 0x3f5f7483u, 0x4084ad2eu, 0xbf0e9d30u, 0xbebe26ebu, 0x3f3e26ebu, 0x40be26ebu, 0xbf32416au, 0xbe32416au, 0x3f32416au, 0x40b7d375u, 0x3f2aaaabu, 0xbeaaaaabu, 0x3f2aaaabu, 0xc0800000u, 0x3f03b5feu, 0xbf03b5feu, 0x3f2f9d53u, 0xbfc590fdu, 0xbf3ce4b5u, 0x3f06ec81u, 0x3ed7e0cfu, 0x405e9fd6u, 0x3e48d2abu, 0xbf169e00u, 0x3f48d2abu, 0x3fe1ed00u, 0x80000000u, 0x3f47e705u, 0x3f1fec04u, 0xc0b8e8e5u, 0xbe32416au, 0x3f32416au, 0x3f32416au, 0xc05ed1c4u, 0x3f324f88u, 0xbf18d674u, 0x3ecbc89bu, 0xc098d675u, 0xbeefe206u, 0x3f1fec04u, 0x3f1fec04u, 0x3e9fec04u, 0xbef85b42u, 0xbf3a4472u, 0x3ef85b42u, 0x4122fbe4u, 0x3f2aaaabu, 0x3eaaaaabu, 0x3f2aaaabu, 0xc1355556u, 0x3f169e00u, 0x3e48d2abu, 0x3f48d2abu, 0xc119c14bu, 0x3ed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0xc1092be3u, 0x3e715befu, 0x3e715befu, 0x3f715befu, 0xc09e6455u, 0x00000000u, 0x80000000u, 0x3f800000u, 0x80000000u, 0xbe715befu, 0xbe715befu, 0x3f715befu, 0x409e6455u, 0xbee4f92eu, 0x00000000u, 0x3f64f92eu, 0x40d6a99bu, 0x3e32416au, 0xbf32416au, 0x3f32416au, 0x408b431bu, 0x00000000u, 0xbf19999au, 0x3f4ccccdu, 0x40666667u, 0xbe32416au, 0xbf32416au, 0x3f32416au, 0x40966731u, 0xbe98a61fu, 0xbf3ecfa6u, 0x3f18a61fu, 0x40abbae2u, 0xbf03b5feu, 0x3f03b5feu, 0x3f2f9d53u, 0xc04590fdu, 0x3f1d8e9fu, 0xbf1d8e9fu, 0x3efc1764u, 0x402d5015u, 0x3f1fec04u, 0x3eefe206u, 0x3f1fec04u, 0xc0c2e7a5u, 0x3f0e9d30u, 0x3ebe26ebu, 0x3f3e26ebu, 0xc0a6620eu, 0x3e5f7483u, 0xbf5f7483u, 0x3edf7483u, 0x40a09bbeu, 0xbf5f7483u, 0x3e5f7483u, 0x3edf7483u, 0x3fdf7483u, 0x00000000u, 0xbf64f92eu, 0x3ee4f92eu, 0x40c85a08u, 0x3f64f92eu, 0x80000000u, 0x3ee4f92eu, 0xc0abbae2u, 0xbedf7483u, 0xbe5f7483u, 0x3f5f7483u, 0x4084ad2eu, 0xbf19999au, 0x00000000u, 0x3f4ccccdu, 0x40666667u, 0xbf32416au, 0xbe32416au, 0x3f32416au, 0x40b7d375u, 0xbf3ecfa6u, 0xbe98a61fu, 0x3f18a61fu, 0x40e4f92eu, 0x3f03b5feu, 0xbf03b5feu, 0x3f2f9d53u, 0xbfc590fdu, 0x3ed105ecu, 0xbed105ecu, 0x3f5105ecu, 0xbfd105ecu, 0x3e48d2abu, 0xbf169e00u, 0x3f48d2abu, 0x3fe1ed00u, 0x00000000u, 0xbf3504f3u, 0x3f3504f3u, 0x40931405u, 0xbe1e01b3u, 0xbf45821fu, 0x3f1e01b3u, 0x40d45248u, 0xbeaaaaabu, 0xbf2aaaabu, 0x3f2aaaabu, 0x40f55556u, 0xbed93924u, 0xbf3504f3u, 0x3f10d0c3u, 0x41131406u, 0x3f3504f3u, 0x3ed93924u, 0x3f10d0c3u, 0xc144dbc8u, 0x3f2aaaabu, 0x3eaaaaabu, 0x3f2aaaabu, 0xc1355556u, 0x3f03b5feu, 0x3f03b5feu, 0x3f2f9d53u, 0xc12cdeddu, 0x3ed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0xc1092be3u, 0xbf2aaaabu, 0xbf2aaaabu, 0x3eaaaaabu, 0x41600000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0x3f2aaaabu, 0xbf2aaaabu, 0x3eaaaaabu, 0xc0955556u, 0xbee4f92eu, 0x00000000u, 0x3f64f92eu, 0x40d6a99bu, 0xbf169e00u, 0xbe48d2abu, 0x3f48d2abu, 0x41264e75u, 0x80000000u, 0x3f550140u, 0x3f0e00d5u, 0xc0efa168u, 0xbe1e01b3u, 0x3f45821fu, 0x3f1e01b3u, 0xc0de3263u, 0xbeaaaaabu, 0x3f2aaaabu, 0x3f2aaaabu, 0xc0c00000u, 0xbed93924u, 0x3f3504f3u, 0x3f10d0c3u, 0xc0c29885u, 0x3f298a47u, 0x3f07a1d2u, 0x3f07a1d2u, 0xc0d3ecd8u, 0x3f1fec04u, 0x3eefe206u, 0x3f1fec04u, 0xc0c2e7a5u, 0x3eb28e6eu, 0xbf5050d6u, 0x3eee133eu, 0x40a3ad3au, 0xbf5050d6u, 0x3eb28e6eu, 0x3eee133eu, 0xbf328e6du, 0xbf3450fcu, 0xbf1dc6ddu, 0x3eb450fcu, 0x40f7ef5bu, 0x00000000u, 0x3e785b42u, 0x3f785b42u, 0xc0785b42u, 0x3f64f92eu, 0x00000000u, 0x3ee4f92eu, 0xc0abbae2u, 0x3f5c9478u, 0x3dfc1764u, 0x3efc1764u, 0xc0c8e2a4u, 0xbf19999au, 0x00000000u, 0x3f4ccccdu, 0x40666667u, 0xbf32416au, 0xbe32416au, 0x3f32416au, 0x40b7d375u, 0xbf06ec81u, 0x3f3ce4b5u, 0x3ed7e0cfu, 0xc021e89cu, 0x3f169e00u, 0xbe48d2abu, 0x3f48d2abu, 0xc048d2abu, 0x3e768cdcu, 0x3f57bb40u, 0x3ef68cdcu, 0xc11a1809u, 0x3e0ca83fu, 0x3f52fc5fu, 0x3f0ca83fu, 0xc10a759eu, 0x80000000u, 0x3f47e705u, 0x3f1fec04u, 0xc0e5e346u, 0xbe0ca83fu, 0x3f52fc5fu, 0x3f0ca83fu, 0xc0c5cc99u, 0xbe98a61fu, 0x3f3ecfa6u, 0x3f18a61fu, 0xc0780df1u, 0xbeefe206u, 0x3f1fec04u, 0x3f1fec04u, 0xbf8bee84u, 0x3f3504f3u, 0x3ed93924u, 0x3f10d0c3u, 0xc144dbc8u, 0x3f13cd3au, 0x3f13cd3au, 0x3f13cd3au, 0xc141fd5cu, 0x3f03b5feu, 0x3f03b5feu, 0x3f2f9d53u, 0xc12cdeddu, 0xbf27d610u, 0xbf27d610u, 0x3ebfd012u, 0x415c48f5u, 0xbf638e39u, 0x3de38e39u, 0x3ee38e39u, 0x412aaaabu, 0x00000000u, 0xbf5e452fu, 0x3efe05ecu, 0x40de452fu, 0x3f638e39u, 0x3de38e39u, 0x3ee38e39u, 0xc1638e39u, 0x3f5e452fu, 0x80000000u, 0x3efe05ecu, 0xc15060dcu, 0xbf169e00u, 0xbe48d2abu, 0x3f48d2abu, 0x41264e75u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x4129b4a4u, 0xbe32416au, 0xbf32416au, 0x3f32416au, 0x40c88997u, 0xbebe26ebu, 0xbf0e9d30u, 0x3f3e26ebu, 0x40a070d6u, 0xbeefe206u, 0xbf1fec04u, 0x3f1fec04u, 0x40bde845u, 0x3f298a47u, 0x3f07a1d2u, 0x3f07a1d2u, 0xc0d3ecd8u, 0x3ef85b42u, 0xbf3a4472u, 0x3ef85b42u, 0x40a2fbe4u, 0xbf3a4472u, 0x3ef85b42u, 0x3ef85b42u, 0xc0594fdau, 0xbf324f88u, 0xbf18d674u, 0x3ecbc89bu, 0x40eecf16u, 0x3e5f7483u, 0x3edf7483u, 0x3f5f7483u, 0xc0ca8197u, 0x80000000u, 0x3e785b42u, 0x3f785b42u, 0xc0785b42u, 0xbe5f7483u, 0x3edf7483u, 0x3f5f7483u, 0xc092a476u, 0x3f5c9478u, 0x3dfc1764u, 0x3efc1764u, 0xc0c8e2a4u, 0x3f550140u, 0x80000000u, 0x3f0e00d5u, 0xc09fc0f0u, 0xbf32416au, 0xbe32416au, 0x3f32416au, 0x40b7d375u, 0xbf47e705u, 0x00000000u, 0x3f1fec04u, 0x4095ed44u, 0x3f169e00u, 0xbe48d2abu, 0x3f48d2abu, 0xc048d2abu, 0x3ed105ecu, 0xbed105ecu, 0x3f5105ecu, 0x3e5105ecu, 0x3e48d2abu, 0xbf169e00u, 0x3f48d2abu, 0x4061ed00u, 0x00000000u, 0xbee4f92eu, 0x3f64f92eu, 0x407348c1u, 0xbe48d2abu, 0xbf169e00u, 0x3f48d2abu, 0x40e1ed00u, 0xbeaaaaabu, 0xbf2aaaabu, 0x3f2aaaabu, 0x4112aaabu, 0xbeefe206u, 0x3f1fec04u, 0x3f1fec04u, 0xbf8bee84u, 0x3f298a47u, 0xbf07a1d2u, 0x3f07a1d2u, 0xbffe4f6cu, 0x3f13cd3au, 0x3f13cd3au, 0x3f13cd3au, 0xc141fd5cu, 0xbf23bcf7u, 0xbf23bcf7u, 0x3eda514au, 0x4156e804u, 0xbf57bb40u, 0x3e768cdcu, 0x3ef68cdcu, 0x40fe4142u, 0x3e0ca83fu, 0xbf52fc5fu, 0x3f0ca83fu, 0x40b89cd3u, 0x80000000u, 0x3ee4f92eu, 0x3f64f92eu, 0xc0b2e2acu, 0xbe715befu, 0x3e715befu, 0x3f715befu, 0xbef15befu, 0x3f5e452fu, 0x00000000u, 0x3efe05ecu, 0xc15060dcu, 0x3f52fc5fu, 0x3e0ca83fu, 0x3f0ca83fu, 0xc15bc6e3u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x4129b4a4u, 0xbf45821fu, 0xbe1e01b3u, 0x3f1e01b3u, 0x4151da41u, 0xbebe26ebu, 0xbf0e9d30u, 0x3f3e26ebu, 0x40a070d6u, 0x3f2f0b1fu, 0x3f11de9au, 0x3ee9642au, 0xc0de7378u, 0x3f1d8e9fu, 0xbf1d8e9fu, 0x3efc1764u, 0x409d8e9fu, 0xbf1d8e9fu, 0x3f1d8e9fu, 0x3efc1764u, 0xc0c4f247u, 0xbf2f0b1fu, 0xbf11de9au, 0x3ee9642au, 0x40e21908u, 0x3ebe26ebu, 0x3f0e9d30u, 0x3f3e26ebu, 0xc0f99314u, 0x3e5f7483u, 0x3edf7483u, 0x3f5f7483u, 0xc0ca8197u, 0x00000000u, 0x3f19999au, 0x3f4ccccdu, 0xc0e66667u, 0xbe5f7483u, 0x3edf7483u, 0x3f5f7483u, 0xc092a476u, 0xbedf7483u, 0x3e5f7483u, 0x3f5f7483u, 0xbfa79762u, 0x3f550140u, 0x00000000u, 0x3f0e00d5u, 0xc09fc0f0u, 0x3f45821fu, 0x3e1e01b3u, 0x3f1e01b3u, 0xc0ca722du, 0xbf47e705u, 0x00000000u, 0x3f1fec04u, 0x4095ed44u, 0xbf52fc5fu, 0xbe0ca83fu, 0x3f0ca83fu, 0x40ce971du, 0x3ed105ecu, 0xbed105ecu, 0x3f5105ecu, 0x3e5105ecu, 0x3e715befu, 0xbe715befu, 0x3f715befu, 0xbef15befu, 0x00000000u, 0xbee4f92eu, 0x3f64f92eu, 0x407348c1u, 0xbe48d2abu, 0xbf169e00u, 0x3f48d2abu, 0x40e1ed00u, 0xbe98a61fu, 0x3f3ecfa6u, 0x3f18a61fu, 0xc0a6f5b1u, 0x3f44aa26u, 0xbec4aa26u, 0x3f031c1au, 0xc05d3f6bu, 0x3f1d8e9fu, 0x3f1d8e9fu, 0x3efc1764u, 0xc14ecb31u, 0xbf1d8e9fu, 0xbf1d8e9fu, 0x3efc1764u, 0x414ecb31u, 0xbf44aa26u, 0x3ec4aa26u, 0x3f031c1au, 0x40937f9cu, 0x3e98a61fu, 0xbf3ecfa6u, 0x3f18a61fu, 0x4085915au, 0x3e48d2abu, 0x3f169e00u, 0x3f48d2abu, 0xc1169e00u, 0x00000000u, 0x3ee4f92eu, 0x3f64f92eu, 0xc0b2e2acu, 0xbe715befu, 0x3e715befu, 0x3f715befu, 0xbef15befu, 0xbed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0x3e5105ecu, 0x3f52fc5fu, 0x3e0ca83fu, 0x3f0ca83fu, 0xc15bc6e3u, 0x3f47e705u, 0x80000000u, 0x3f1fec04u, 0xc13b6895u, 0xbf45821fu, 0xbe1e01b3u, 0x3f1e01b3u, 0x4151da41u, 0xbf550140u, 0x00000000u, 0x3f0e00d5u, 0x4147b12cu, 0x3f3a4472u, 0xbef85b42u, 0x3ef85b42u, 0x4093762fu, 0xbef85b42u, 0x3f3a4472u, 0x3ef85b42u, 0xc10bb355u, 0xbf298a47u, 0xbf07a1d2u, 0x3f07a1d2u, 0x40cfafcau, 0x3eefe206u, 0x3f1fec04u, 0x3f1fec04u, 0xc1096ed3u, 0x3ebe26ebu, 0x3f0e9d30u, 0x3f3e26ebu, 0xc0f99314u, 0x3e32416au, 0x3f32416au, 0x3f32416au, 0xc10e0c20u, 0x80000000u, 0x3f19999au, 0x3f4ccccdu, 0xc0e66667u, 0xbe5f7483u, 0x3edf7483u, 0x3f5f7483u, 0xc092a476u, 0xbedf7483u, 0x3e5f7483u, 0x3f5f7483u, 0xbfa79762u, 0xbf0e9d30u, 0x3ebe26ebu, 0x3f3e26ebu, 0xc01a7f9fu, 0x3f45821fu, 0x3e1e01b3u, 0x3f1e01b3u, 0xc0ca722du, 0x3f3504f3u, 0x80000000u, 0x3f3504f3u, 0xc087c3b6u, 0xbf52fc5fu, 0xbe0ca83fu, 0x3f0ca83fu, 0x40ce971du, 0xbf5e452fu, 0x00000000u, 0x3efe05ecu, 0x40a6b3e3u, 0x3e715befu, 0xbe715befu, 0x3f715befu, 0xbef15befu, 0x00000000u, 0xbee4f92eu, 0x3f64f92eu, 0x407348c1u, 0xbe0ca83fu, 0x3f52fc5fu, 0x3f0ca83fu, 0xc10a759eu, 0x3f57bb40u, 0xbe768cdcu, 0x3ef68cdcu, 0xc0963dd6u, 0x3f23bcf7u, 0x3f23bcf7u, 0x3eda514au, 0xc156e804u, 0xbf13cd3au, 0xbf13cd3au, 0x3f13cd3au, 0x4141fd5cu, 0xbf298a47u, 0x3f07a1d2u, 0x3f07a1d2u, 0x3f4b72c0u, 0x3eefe206u, 0xbf1fec04u, 0x3f1fec04u, 0x400bee84u, 0x3eaaaaabu, 0x3f2aaaabu, 0x3f2aaaabu, 0xc13d5556u, 0x3e48d2abu, 0x3f169e00u, 0x3f48d2abu, 0xc1169e00u, 0x80000000u, 0x3ee4f92eu, 0x3f64f92eu, 0xc0b2e2acu, 0xbe48d2abu, 0x3f169e00u, 0x3f48d2abu, 0xc0969e00u, 0xbed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0x3e5105ecu, 0xbf169e00u, 0x3e48d2abu, 0x3f48d2abu, 0x40a32b2au, 0x3f47e705u, 0x00000000u, 0x3f1fec04u, 0xc13b6895u, 0x3f32416au, 0x3e32416au, 0x3f32416au, 0xc1488997u, 0xbf550140u, 0x00000000u, 0x3f0e00d5u, 0x4147b12cu, 0xbf5c9478u, 0xbdfc1764u, 0x3efc1764u, 0x41666d62u, 0xbf1fec04u, 0xbeefe206u, 0x3f1fec04u, 0x40b3e984u, 0xbf298a47u, 0xbf07a1d2u, 0x3f07a1d2u, 0x40cfafcau, 0x3ed93924u, 0xbf3504f3u, 0x3f10d0c3u, 0x40efd9c2u, 0x3eaaaaabu, 0xbf2aaaabu, 0x3f2aaaabu, 0x40e00000u, 0x3e1e01b3u, 0xbf45821fu, 0x3f1e01b3u, 0x410cb983u, 0x00000000u, 0xbf550140u, 0x3f0e00d5u, 0x411fc0f0u, 0xbdfc1764u, 0xbf5c9478u, 0x3efc1764u, 0x412b57e6u, 0xbe88d677u, 0xbf4d41b3u, 0x3f08d677u, 0x4126c561u, 0xbeb28e6eu, 0xbf5050d6u, 0x3eee133eu, 0x4130b248u, 0x3f45821fu, 0x3e1e01b3u, 0x3f1e01b3u, 0xc0ca722du, 0x3f3504f3u, 0x00000000u, 0x3f3504f3u, 0xc087c3b6u, 0x3f169e00u, 0x3e48d2abu, 0x3f48d2abu, 0xc0c28c16u, 0xbf5e452fu, 0x00000000u, 0x3efe05ecu, 0x40a6b3e3u, 0xbf638e39u, 0xbde38e39u, 0x3ee38e39u, 0x40d8e38eu, 0x80000000u, 0x3f5e452fu, 0x3efe05ecu, 0xc1349836u, 0x3f638e39u, 0xbde38e39u, 0x3ee38e39u, 0xc0b55555u, 0x3f27d610u, 0x3f27d610u, 0x3ebfd012u, 0xc15c48f5u, 0xbf03b5feu, 0xbf03b5feu, 0x3f2f9d53u, 0x412cdeddu, 0xbf13cd3au, 0xbf13cd3au, 0x3f13cd3au, 0x4141fd5cu, 0xbf3504f3u, 0xbed93924u, 0x3f10d0c3u, 0x41374836u, 0x3eefe206u, 0xbf1fec04u, 0x3f1fec04u, 0x400bee84u, 0x3e98a61fu, 0xbf3ecfa6u, 0x3f18a61fu, 0x40b08013u, 0x3e0ca83fu, 0xbf52fc5fu, 0x3f0ca83fu, 0x4103ddbbu, 0x00000000u, 0xbf47e705u, 0x3f1fec04u, 0x41136d94u, 0xbe0ca83fu, 0xbf52fc5fu, 0x3f0ca83fu, 0x41389cd3u, 0xbe768cdcu, 0xbf57bb40u, 0x3ef68cdcu, 0x4151f3f3u, 0xbf169e00u, 0x3e48d2abu, 0x3f48d2abu, 0x40a32b2bu, 0x3f06ec81u, 0xbf3ce4b5u, 0x3ed7e0cfu, 0x3ff2dcebu, 0x3f32416au, 0x3e32416au, 0x3f32416au, 0xc1488997u, 0x3f19999au, 0x80000000u, 0x3f4ccccdu, 0xc1100000u, 0xbf5c9478u, 0xbdfc1764u, 0x3efc1764u, 0x41666d62u, 0xbf64f92eu, 0x00000000u, 0x3ee4f92eu, 0x4156a99bu, 0xbf1d8e9fu, 0x3f1d8e9fu, 0x3efc1764u, 0xc105ec6eu, 0x3f03b5feu, 0xbf03b5feu, 0x3f2f9d53u, 0x40c590fdu, 0x3e98a61fu, 0x3f3ecfa6u, 0x3f18a61fu, 0xc132e2acu, 0x3e32416au, 0x3f32416au, 0x3f32416au, 0xc1271d53u, 0x80000000u, 0x3f19999au, 0x3f4ccccdu, 0xc1100000u, 0xbe32416au, 0x3f32416au, 0x3f32416au, 0xc11bf93du, 0xbebe26ebu, 0x3f0e9d30u, 0x3f3e26ebu, 0xc0e7bf6eu, 0xbf0e9d30u, 0x3ebe26ebu, 0x3f3e26ebu, 0xc082bac2u, 0x3f45821fu, 0x3e1e01b3u, 0x3f1e01b3u, 0xc0ca722du, 0x3f2aaaabu, 0x3eaaaaabu, 0x3f2aaaabu, 0xc1055556u, 0x3f169e00u, 0x3e48d2abu, 0x3f48d2abu, 0xc0c28c16u, 0x3ee4f92eu, 0x80000000u, 0x3f64f92eu, 0xc02bbae2u, 0xbf2aaaabu, 0x3f2aaaabu, 0x3eaaaaabu, 0xc0955556u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0x3f2aaaabu, 0x3f2aaaabu, 0x3eaaaaabu, 0xc1600000u, 0xbed105ecu, 0xbed105ecu, 0x3f5105ecu, 0x41092be3u, 0xbf03b5feu, 0xbf03b5feu, 0x3f2f9d53u, 0x412cdeddu, 0xbf2aaaabu, 0xbeaaaaabu, 0x3f2aaaabu, 0x411aaaabu, 0xbf3504f3u, 0xbed93924u, 0x3f10d0c3u, 0x41374836u, 0x3ed93924u, 0x3f3504f3u, 0x3f10d0c3u, 0xc1690ff9u, 0x3eaaaaabu, 0x3f2aaaabu, 0x3f2aaaabu, 0xc1555556u, 0x3e1e01b3u, 0x3f45821fu, 0x3f1e01b3u, 0xc14cea34u, 0x80000000u, 0x3f3504f3u, 0x3f3504f3u, 0xc1240c7cu, 0xbe48d2abu, 0x3f169e00u, 0x3f48d2abu, 0xc0cf1940u, 0xbed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0xbfd105ecu, 0xbf03b5feu, 0x3f03b5feu, 0x3f2f9d53u, 0xbfc590fdu, 0x3f3ecfa6u, 0x3e98a61fu, 0x3f18a61fu, 0xc16c20f7u, 0x3f32416au, 0x3e32416au, 0x3f32416au, 0xc1488997u, 0x3f19999au, 0x00000000u, 0x3f4ccccdu, 0xc1100000u, 0x3edf7483u, 0x3e5f7483u, 0x3f5f7483u, 0xc119a01au, 0xbf64f92eu, 0x00000000u, 0x3ee4f92eu, 0x4156a99bu, 0x00000000u, 0x3f64f92eu, 0x3ee4f92eu, 0xc1485a08u, 0x3ec4aa26u, 0x3f44aa26u, 0x3f031c1au, 0xc1385f84u, 0xbf471c72u, 0xbee38e39u, 0x3ee38e39u, 0x40d55555u, 0x3e1e01b3u, 0xbf45821fu, 0x3f1e01b3u, 0x4122f1c0u, 0x00000000u, 0xbf3504f3u, 0x3f3504f3u, 0x4115e819u, 0xbe1e01b3u, 0xbf45821fu, 0x3f1e01b3u, 0x412cd1dbu, 0x3f5050d6u, 0x3eb28e6eu, 0x3eee133eu, 0xc0f583d7u, 0xbf0e9d30u, 0x3ebe26ebu, 0x3f3e26ebu, 0xc082bac2u, 0xbf1fec04u, 0x3eefe206u, 0x3f1fec04u, 0xc0a4eb64u, 0x3f2aaaabu, 0x3eaaaaabu, 0x3f2aaaabu, 0xc1055556u, 0x3f169e00u, 0x3e48d2abu, 0x3f48d2abu, 0xc0c28c16u, 0x3ee4f92eu, 0x00000000u, 0x3f64f92eu, 0xc02bbae2u, 0x3e715befu, 0x3e715befu, 0x3f715befu, 0xc09e6455u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0xbe715befu, 0xbe715befu, 0x3f715befu, 0x409e6455u, 0xbed105ecu, 0xbed105ecu, 0x3f5105ecu, 0x41092be3u, 0xbf169e00u, 0xbe48d2abu, 0x3f48d2abu, 0x40dba66bu, 0xbf2aaaabu, 0xbeaaaaabu, 0x3f2aaaabu, 0x411aaaabu, 0x3ef85b42u, 0x3f3a4472u, 0x3ef85b42u, 0xc17479d5u, 0x3eefe206u, 0xbf1fec04u, 0x3f1fec04u, 0x4065e346u, 0xbf324f88u, 0x3f18d674u, 0x3ecbc89bu, 0xc02bf141u, 0x3e32416au, 0xbf32416au, 0x3f32416au, 0x40ef87e6u, 0x00000000u, 0xbf47e705u, 0x3f1fec04u, 0x4129eac4u, 0xbe48d2abu, 0x3f169e00u, 0x3f48d2abu, 0xc0cf1940u, 0x3f3ce4b5u, 0xbf06ec81u, 0x3ed7e0cfu, 0xbf72dcedu, 0xbf03b5feu, 0x3f03b5feu, 0x3f2f9d53u, 0xbfc590fdu, 0xbf2aaaabu, 0x3eaaaaabu, 0x3f2aaaabu, 0x40400000u, 0x3f32416au, 0x3e32416au, 0x3f32416au, 0xc1488997u, 0x3f0e9d30u, 0x3ebe26ebu, 0x3f3e26ebu, 0xc158e464u, 0x3edf7483u, 0x3e5f7483u, 0x3f5f7483u, 0xc119a01au, 0x3e785b42u, 0x80000000u, 0x3f785b42u, 0xc068d58eu, 0x00000000u, 0xbe785b42u, 0x3f785b42u, 0x40594fdau, 0xbe785b42u, 0x00000000u, 0x3f785b42u, 0x4068d58eu, 0xbf3ce4b5u, 0x3f06ec81u, 0x3ed7e0cfu, 0xc0fcfb72u, 0x3e48d2abu, 0xbf169e00u, 0x3f48d2abu, 0x410d3420u, 0x80000000u, 0x3f47e705u, 0x3f1fec04u, 0xc14ce665u, 0xbe32416au, 0x3f32416au, 0x3f32416au, 0xc1350a70u, 0x3f324f88u, 0xbf18d674u, 0x3ecbc89bu, 0x40dbb447u, 0xbeefe206u, 0x3f1fec04u, 0x3f1fec04u, 0xc1186cf4u, 0xbef85b42u, 0xbf3a4472u, 0x3ef85b42u, 0x4145e8b9u, 0x3f2aaaabu, 0x3eaaaaabu, 0x3f2aaaabu, 0xc1055556u, 0x3f169e00u, 0x3e48d2abu, 0x3f48d2abu, 0xc0c28c16u, 0x3ed105ecu, 0x3ed105ecu, 0x3f5105ecu, 0xc1092be3u, 0x3e715befu, 0x3e715befu, 0x3f715befu, 0xc09e6455u, 0x00000000u, 0x80000000u, 0x3f800000u, 0x80000000u, 0xbe715befu, 0xbe715befu, 0x3f715befu, 0x409e6455u, 0xbee4f92eu, 0x00000000u, 0x3f64f92eu, 0x402bbae2u, 0xbf169e00u, 0xbe48d2abu, 0x3f48d2abu, 0x40dba66bu, 0xbf2aaaabu, 0xbeaaaaabu, 0x3f2aaaabu, 0x411aaaabu, 0x3f1fec04u, 0xbeefe206u, 0x3f1fec04u, 0x3fb3e984u, 0x3f0e9d30u, 0xbebe26ebu, 0x3f3e26ebu, 0x3ebe26efu, 0xbf5050d6u, 0xbeb28e6eu, 0x3eee133eu, 0x4148e03cu, 0x3e1e01b3u, 0x3f45821fu, 0x3f1e01b3u, 0xc1632271u, 0x80000000u, 0x3f3504f3u, 0x3f3504f3u, 0xc13d812eu, 0xbe1e01b3u, 0x3f45821fu, 0x3f1e01b3u, 0xc131c1e9u, 0x3f471c72u, 0x3ee38e39u, 0x3ee38e39u, 0xc1800000u, 0xbec4aa26u, 0xbf44aa26u, 0x3f031c1au, 0x41810fa9u, 0xbf2aaaabu, 0x3eaaaaabu, 0x3f2aaaabu, 0x40400000u, 0x3ef85b42u, 0xbf3a4472u, 0x3ef85b42u, 0x40a2fbe4u, 0x3f0e9d30u, 0x3ebe26ebu, 0x3f3e26ebu, 0xc158e464u, 0x3edf7483u, 0x3e5f7483u, 0x3f5f7483u, 0xc119a01au, 0x3e785b42u, 0x00000000u, 0x3f785b42u, 0xc068d58eu, 0x00000000u, 0x3e785b42u, 0x3f785b42u, 0xc0785b42u, 0xbe785b42u, 0x00000000u, 0x3f785b42u, 0x4068d58eu, 0xbedf7483u, 0xbe5f7483u, 0x3f5f7483u, 0x41209bbeu
	};
// steep terrain: 90 vertices, 144 triangles
static const unsigned kMeshVerts2i1[270] =
	{
	0x00000000u, 0x00000000u, 0x00000000u, 0x3f000000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x00000000u, 0x40000000u, 0x3fc00000u, 0x00000000u, 0x40000000u, 0x40000000u, 0x00000000u, 0x3f000000u, 0x40200000u, 0x00000000u, 0x00000000u, 0x40400000u, 0x00000000u, 0x00000000u, 0x40600000u, 0x00000000u, 0x40400000u, 0x40800000u, 0x00000000u, 0x3f800000u, 0x40900000u, 0x00000000u, 0x00000000u, 0x00000000u, 0x3f000000u, 0x00000000u, 0x3f000000u, 0x3f000000u, 0x00000000u, 0x3f800000u, 0x3f000000u, 0x3f000000u, 0x3fc00000u, 0x3f000000u, 0x40400000u, 0x40000000u, 0x3f000000u, 0x40400000u, 0x40200000u, 0x3f000000u, 0x40200000u, 0x40400000u, 0x3f000000u, 0x3f800000u, 0x40600000u, 0x3f000000u, 0x00000000u, 0x40800000u, 0x3f000000u, 0x00000000u, 0x40900000u, 0x3f000000u, 0x3fc00000u, 0x00000000u, 0x3f800000u, 0x00000000u, 0x3f000000u, 0x3f800000u, 0x00000000u, 0x3f800000u, 0x3f800000u, 0x3f000000u, 0x3fc00000u, 0x3f800000u, 0x40400000u, 0x40000000u, 0x3f800000u, 0x40400000u, 0x40200000u, 0x3f800000u, 0x40200000u, 0x40400000u, 0x3f800000u, 0x3f800000u, 0x40600000u, 0x3f800000u, 0x00000000u, 0x40800000u, 0x3f800000u, 0x00000000u, 0x40900000u, 0x3f800000u, 0x3fc00000u, 0x00000000u, 0x3fc00000u, 0x40000000u, 0x3f000000u, 0x3fc00000u, 0x3f000000u, 0x3f800000u, 0x3fc00000u, 0x00000000u, 0x3fc00000u, 0x3fc00000u, 0x00000000u, 0x40000000u, 0x3fc00000u, 0x40400000u, 0x40200000u, 0x3fc00000u, 0x3f800000u, 0x40400000u, 0x3fc00000u, 0x00000000u, 0x40600000u, 0x3fc00000u, 0x00000000u, 0x40800000u, 0x3fc00000u, 0x3f800000u, 0x40900000u, 0x3fc00000u, 0x40000000u, 0x00000000u, 0x40000000u, 0x00000000u, 0x3f000000u, 0x40000000u, 0x00000000u, 0x3f800000u, 0x40000000u, 0x3f000000u, 0x3fc00000u, 0x40000000u, 0x40400000u, 0x40000000u, 0x40000000u, 0x40400000u, 0x40200000u, 0x40000000u, 0x40200000u, 0x40400000u, 0x40000000u, 0x3f800000u, 0x40600000u, 0x40000000u, 0x00000000u, 0x40800000u, 0x40000000u, 0x00000000u, 0x40900000u, 0x40000000u, 0x3fc00000u, 0x00000000u, 0x40200000u, 0x00000000u, 0x3f000000u, 0x40200000u, 0x00000000u, 0x3f800000u, 0x40200000u, 0x3f000000u, 0x3fc00000u, 0x40200000u, 0x40400000u, 0x40000000u, 0x40200000u, 0x40400000u, 0x40200000u, 0x40200000u, 0x40200000u, 0x40400000u, 0x40200000u, 0x3f800000u, 0x40600000u, 0x40200000u, 0x00000000u, 0x40800000u, 0x40200000u, 0x00000000u, 0x40900000u, 0x40200000u, 0x3fc00000u, 0x00000000u, 0x40400000u, 0x00000000u, 0x3f000000u, 0x40400000u, 0x40400000u, 0x3f800000u, 0x40400000u, 0x3f800000u, 0x3fc00000u, 0x40400000u, 0x00000000u, 0x40000000u, 0x40400000u, 0x00000000u, 0x40200000u, 0x40400000u, 0x3f800000u, 0x40400000u, 0x40400000u, 0x40000000u, 0x40600000u, 0x40400000u, 0x40000000u, 0x40800000u, 0x40400000u, 0x3f000000u, 0x40900000u, 0x40400000u, 0x00000000u, 0x00000000u, 0x40600000u, 0x00000000u, 0x3f000000u, 0x40600000u, 0x00000000u, 0x3f800000u, 0x40600000u, 0x3f000000u, 0x3fc00000u, 0x40600000u, 0x40400000u, 0x40000000u, 0x40600000u, 0x40400000u, 0x40200000u, 0x40600000u, 0x40200000u, 0x40400000u, 0x40600000u, 0x3f800000u, 0x40600000u, 0x40600000u, 0x00000000u, 0x40800000u, 0x40600000u, 0x00000000u, 0x40900000u, 0x40600000u, 0x3fc00000u, 0x00000000u, 0x40800000u, 0x00000000u, 0x3f000000u, 0x40800000u, 0x00000000u, 0x3f800000u, 0x40800000u, 0x3f000000u, 0x3fc00000u, 0x40800000u, 0x40400000u, 0x40000000u, 0x40800000u, 0x40400000u, 0x40200000u, 0x40800000u, 0x40200000u, 0x40400000u, 0x40800000u, 0x3f800000u, 0x40600000u, 0x40800000u, 0x00000000u, 0x40800000u, 0x40800000u, 0x00000000u, 0x40900000u, 0x40800000u, 0x3fc00000u
	};
static const unsigned kMeshTris2i1[432] =
	{
	0, 1, 10, 1, 11, 10, 1, 2, 11, 2, 12, 11, 2, 3, 12, 3, 13, 12, 3, 4, 13, 4, 14, 13, 4, 5, 14, 5, 15, 14, 5, 6, 15, 6, 16, 15, 6, 7, 16, 7, 17, 16, 7, 8, 17, 8, 18, 17, 8, 9, 18, 9, 19, 18, 10, 11, 20, 11, 21, 20, 11, 12, 22, 11, 22, 21, 12, 13, 22, 13, 23, 22, 13, 14, 24, 13, 24, 23, 14, 15, 24, 15, 25, 24, 15, 16, 26, 15, 26, 25, 16, 17, 26, 17, 27, 26, 17, 18, 28, 17, 28, 27, 18, 19, 28, 19, 29, 28, 20, 21, 30, 21, 31, 30, 21, 22, 31, 22, 32, 31, 22, 23, 32, 23, 33, 32, 23, 24, 33, 24, 34, 33, 24, 25, 34, 25, 35, 34, 25, 26, 35, 26, 36, 35, 26, 27, 36, 27, 37, 36, 27, 28, 37, 28, 38, 37, 28, 29, 38, 29, 39, 38, 30, 31, 40, 31, 41, 40, 31, 32, 42, 31, 42, 41, 32, 33, 42, 33, 43, 42, 33, 34, 44, 33, 44, 43, 34, 35, 44, 35, 45, 44, 35, 36, 46, 35, 46, 45, 36, 37, 46, 37, 47, 46, 37, 38, 48, 37, 48, 47, 38, 39, 48, 39, 49, 48, 40, 41, 50, 41, 51, 50, 41, 42, 51, 42, 52, 51, 42, 43, 52, 43, 53, 52, 43, 44, 53, 44, 54, 53, 44, 45, 54, 45, 55, 54, 45, 46, 55, 46, 56, 55, 46, 47, 56, 47, 57, 56, 47, 48, 57, 48, 58, 57, 48, 49, 58, 49, 59, 58, 50, 51, 60, 51, 61, 60, 51, 52, 62, 51, 62, 61, 52, 53, 62, 53, 63, 62, 53, 54, 64, 53, 64, 63, 54, 55, 64, 55, 65, 64, 55, 56, 66, 55, 66, 65, 56, 57, 66, 57, 67, 66, 57, 58, 68, 57, 68, 67, 58, 59, 68, 59, 69, 68, 60, 61, 70, 61, 71, 70, 61, 62, 71, 62, 72, 71, 62, 63, 72, 63, 73, 72, 63, 64, 73, 64, 74, 73, 64, 65, 74, 65, 75, 74, 65, 66, 75, 66, 76, 75, 66, 67, 76, 67, 77, 76, 67, 68, 77, 68, 78, 77, 68, 69, 78, 69, 79, 78, 70, 71, 80, 71, 81, 80, 71, 72, 82, 71, 82, 81, 72, 73, 82, 73, 83, 82, 73, 74, 84, 73, 84, 83, 74, 75, 84, 75, 85, 84, 75, 76, 86, 75, 86, 85, 76, 77, 86, 77, 87, 86, 77, 78, 88, 77, 88, 87, 78, 79, 88, 79, 89, 88
	};
static const unsigned kMeshPlanes2i1[576] =
	{
	0xbf64f92eu, 0x00000000u, 0x3ee4f92eu, 0x80000000u, 0x00000000u, 0x3f64f92eu, 0x3ee4f92eu, 0xbee4f92eu, 0xbf2aaaabu, 0x3f2aaaabu, 0x3eaaaaabu, 0x80000000u, 0xbe9a5fb2u, 0x3f678f8bu, 0x3e9a5fb2u, 0xbe9a5fb2u, 0x80000000u, 0x3f72dce9u, 0x3ea1e89bu, 0xbf21e89bu, 0xbf69b1e9u, 0xbebaf4bau, 0x3e3af4bau, 0x3f808840u, 0x3f4d41b3u, 0xbf08d677u, 0x3e88d677u, 0xbfde5c82u, 0x00000000u, 0xbf7b0756u, 0x3e48d2abu, 0xbdc8d2abu, 0x3e4511a3u, 0xbf76560cu, 0x3e4511a3u, 0xbef6560cu, 0x3e4511a3u, 0xbf76560cu, 0x3e4511a3u, 0xbef6560cu, 0x00000000u, 0xbf7b0756u, 0x3e48d2abu, 0x80000000u, 0x3f4d41b3u, 0xbf08d677u, 0x3e88d677u, 0xc019f146u, 0xbf6fe206u, 0xbe9fec04u, 0x3e1fec04u, 0x4033e984u, 0x3e9fec04u, 0x3f6fe206u, 0x3e1fec04u, 0xbfc7e705u, 0x3f0ca83fu, 0x3f52fc5fu, 0x3e0ca83fu, 0xc01572c3u, 0x00000000u, 0x3f64f92eu, 0x3ee4f92eu, 0xbee4f92eu, 0x3f2aaaabu, 0x3f2aaaabu, 0x3eaaaaabu, 0xc0400000u, 0xbf3030f8u, 0xbf3030f8u, 0x3e6aebf5u, 0x40463717u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0x00000000u, 0x80000000u, 0x3f800000u, 0x80000000u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x3eb504f3u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x3eb504f3u, 0xbf7b0756u, 0x00000000u, 0x3e48d2abu, 0x3f61ed01u, 0xbf7b0756u, 0x00000000u, 0x3e48d2abu, 0x3f61ed01u, 0x00000000u, 0x00000000u, 0x3f800000u, 0xc0400000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0xc0400000u, 0x3f3504f3u, 0x80000000u, 0x3f3504f3u, 0xc0624630u, 0x3f3504f3u, 0x80000000u, 0x3f3504f3u, 0xc0624630u, 0x3f72dce9u, 0x00000000u, 0x3ea1e89bu, 0xc04a62c2u, 0x3f72dce9u, 0x80000000u, 0x3ea1e89bu, 0xc04a62c2u, 0x3f64f92eu, 0x80000000u, 0x3ee4f92eu, 0xc0485a08u, 0x3f64f92eu, 0x80000000u, 0x3ee4f92eu, 0xc0485a08u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0xbf72dce9u, 0x00000000u, 0x3ea1e89bu, 0x4072dce9u, 0xbf72dce9u, 0x00000000u, 0x3ea1e89bu, 0x4072dce9u, 0x00000000u, 0xbf785b42u, 0x3e785b42u, 0x3f785b42u, 0x3f678f8bu, 0xbe9a5fb2u, 0x3e9a5fb2u, 0xbe1a5fb2u, 0xbf13cd3au, 0xbf13cd3au, 0x3f13cd3au, 0x3f5db3d7u, 0x3f13cd3au, 0x3f13cd3au, 0x3f13cd3au, 0xbfb8c088u, 0xbf76560cu, 0x3e4511a3u, 0x3e4511a3u, 0x3f2c6f6fu, 0x00000000u, 0x3f7c8450u, 0x3e285835u, 0xbfbd633cu, 0x80000000u, 0x3f7c8450u, 0x3e285835u, 0xbfbd633cu, 0xbf7c8450u, 0x00000000u, 0x3e285835u, 0x3fbd633cu, 0x3f3504f3u, 0x80000000u, 0x3f3504f3u, 0xc0624630u, 0x3f48d2abu, 0x3f169e00u, 0x3e48d2abu, 0xc0428c16u, 0x3f3030f8u, 0x3f3030f8u, 0x3e6aebf5u, 0xc03edfb7u, 0x3f2aaaabu, 0x3f2aaaabu, 0x3eaaaaabu, 0xc0400000u, 0x3f2aaaabu, 0x3f2aaaabu, 0x3eaaaaabu, 0xc0400000u, 0x00000000u, 0x80000000u, 0x3f800000u, 0x80000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0xbf2aaaabu, 0xbf2aaaabu, 0x3eaaaaabu, 0x40555556u, 0xbf4d41b3u, 0xbf08d677u, 0x3e88d677u, 0x406f7751u, 0xbf5105ecu, 0xbed105ecu, 0x3ed105ecu, 0x405e164bu, 0x3f169e00u, 0x3f48d2abu, 0x3e48d2abu, 0xbfc8d2abu, 0x00000000u, 0x3f3504f3u, 0x3f3504f3u, 0xbfb504f3u, 0x3f13cd3au, 0xbf13cd3au, 0x3f13cd3au, 0x3e93cd3au, 0xbf13cd3au, 0x3f13cd3au, 0x3f13cd3au, 0xbf5db3d7u, 0x00000000u, 0xbf3504f3u, 0x3f3504f3u, 0x3f87c3b6u, 0xbf228f67u, 0xbf43127bu, 0x3e020c52u, 0x40061cb5u, 0xbf7c8450u, 0x00000000u, 0x3e285835u, 0x3fbd633cu, 0x00000000u, 0xbf7c8450u, 0x3e285835u, 0x3fbd633cu, 0x3f785b42u, 0x80000000u, 0x3e785b42u, 0xc02abebdu, 0x3e9a5fb2u, 0xbf678f8bu, 0x3e9a5fb2u, 0x3e9a5fb2u, 0x3f2aaaabu, 0xbf2aaaabu, 0x3eaaaaabu, 0xbf800000u, 0x3f3030f8u, 0xbf3030f8u, 0x3e6aebf5u, 0xbf6aebf5u, 0x00000000u, 0xbf64f92eu, 0x3ee4f92eu, 0x3fabbae2u, 0x3f64f92eu, 0x80000000u, 0x3ee4f92eu, 0xc0485a08u, 0xbf2aaaabu, 0x3f2aaaabu, 0x3eaaaaabu, 0x3faaaaabu, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0xbf2aaaabu, 0x3f2aaaabu, 0x3eaaaaabu, 0x3faaaaabu, 0xbf678f8bu, 0x3e9a5fb2u, 0x3e9a5fb2u, 0x4040f79eu, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0x00000000u, 0x80000000u, 0x3f800000u, 0x80000000u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x3eb504f3u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x3eb504f3u, 0xbf7b0756u, 0x00000000u, 0x3e48d2abu, 0x3f61ed01u, 0xbf7b0756u, 0x00000000u, 0x3e48d2abu, 0x3f61ed01u, 0x00000000u, 0x00000000u, 0x3f800000u, 0xc0400000u, 0x00000000u, 0x80000000u, 0x3f800000u, 0xc0400000u, 0x3f3504f3u, 0x80000000u, 0x3f3504f3u, 0xc0624630u, 0x3f3504f3u, 0x80000000u, 0x3f3504f3u, 0xc0624630u, 0x3f72dce9u, 0x80000000u, 0x3ea1e89bu, 0xc04a62c2u, 0x3f72dce9u, 0x80000000u, 0x3ea1e89bu, 0xc04a62c2u, 0x3f64f92eu, 0x80000000u, 0x3ee4f92eu, 0xc0485a08u, 0x3f64f92eu, 0x80000000u, 0x3ee4f92eu, 0xc0485a08u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0x00000000u, 0x80000000u, 0x3f800000u, 0x80000000u, 0xbf72dce9u, 0x00000000u, 0x3ea1e89bu, 0x4072dce9u, 0xbf72dce9u, 0x00000000u, 0x3ea1e89bu, 0x4072dce9u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0xbf33c674u, 0xbf33c674u, 0x3defb345u, 0x4006d4d7u, 0xbf13cd3au, 0xbf13cd3au, 0x3f13cd3au, 0x3fddb3d7u, 0x3f0ca83fu, 0xbf52fc5fu, 0x3e0ca83fu, 0x3fe49167u, 0xbf76560cu, 0xbe4511a3u, 0x3e4511a3u, 0x3fac6f6fu, 0x3e9fec04u, 0x3f6fe206u, 0x3e1fec04u, 0xc051e5c5u, 0x80000000u, 0x3f7c8450u, 0x3e285835u, 0xc03d633cu, 0x00000000u, 0x3f7c8450u, 0x3e285835u, 0xc03d633cu, 0x3e261d5fu, 0x3f792c0fu, 0x3e261d5fu, 0xc04fa4b7u, 0xbf08d677u, 0x3f4d41b3u, 0x3e88d677u, 0xbfab0c15u, 0x3f4d41b3u, 0xbf08d677u, 0x3e88d677u, 0xbfab0c15u, 0xbf08d677u, 0x3f4d41b3u, 0x3e88d677u, 0xbfab0c15u, 0x3f2aaaabu, 0xbf2aaaabu, 0x3eaaaaabu, 0xbf2aaaabu, 0x00000000u, 0xbf785b42u, 0x3e785b42u, 0x401b3909u, 0x00000000u, 0xbf3504f3u, 0x3f3504f3u, 0x3fe24630u, 0x3f169e00u, 0xbf48d2abu, 0x3e48d2abu, 0xbdc8d2a4u, 0xbf678f8bu, 0xbe9a5fb2u, 0x3e9a5fb2u, 0x408be6b9u, 0x3e9a5fb2u, 0x3f678f8bu, 0x3e9a5fb2u, 0xc08240beu, 0xbf7c8450u, 0x00000000u, 0x3e285835u, 0x80000000u, 0x00000000u, 0x3f7c8450u, 0x3e285835u, 0xc05cf3c6u, 0x3f0ca83fu, 0x3f52fc5fu, 0x3e0ca83fu, 0xc04a31dbu, 0xbf13cd3au, 0x3f13cd3au, 0x3f13cd3au, 0xbfddb3d7u, 0x3f5105ecu, 0x3ed105ecu, 0x3ed105ecu, 0xc01cc471u, 0xbf228f67u, 0xbf43127bu, 0x3e020c52u, 0x404f43a3u, 0x00000000u, 0xbf7c8450u, 0x3e285835u, 0x403d633cu, 0x00000000u, 0xbf7c8450u, 0x3e285835u, 0x403d633cu, 0xbe9fec04u, 0xbf6fe206u, 0x3e1fec04u, 0x405be486u, 0x3e9a5fb2u, 0xbf678f8bu, 0x3e9a5fb2u, 0x3fd44395u, 0xbf08d677u, 0xbf4d41b3u, 0x3e88d677u, 0x405e5c82u, 0x3f4d41b3u, 0x3f08d677u, 0x3e88d677u, 0xc09163dfu, 0x80000000u, 0x3f64f92eu, 0x3ee4f92eu, 0xc064f92eu, 0x3edf7483u, 0x3f5f7483u, 0x3e5f7483u, 0xc092a476u, 0x3f169e00u, 0x3f48d2abu, 0x3e48d2abu, 0xc099c14bu, 0x00000000u, 0x3f3504f3u, 0x3f3504f3u, 0xc01e6455u, 0x3f13cd3au, 0x3f13cd3au, 0x3f13cd3au, 0xc08a9066u, 0xbf3030f8u, 0xbf3030f8u, 0x3e6aebf5u, 0x40a52de8u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0x00000000u, 0x80000000u, 0x3f800000u, 0x80000000u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x3eb504f3u, 0xbf3504f3u, 0x00000000u, 0x3f3504f3u, 0x3eb504f3u, 0xbf7b0756u, 0x00000000u, 0x3e48d2abu, 0x3f61ed01u, 0xbf7b0756u, 0x00000000u, 0x3e48d2abu, 0x3f61ed01u, 0x00000000u, 0x00000000u, 0x3f800000u, 0xc0400000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0xc0400000u, 0x3f3504f3u, 0x80000000u, 0x3f3504f3u, 0xc0624630u, 0x3f3504f3u, 0x80000000u, 0x3f3504f3u, 0xc0624630u, 0x3f72dce9u, 0x00000000u, 0x3ea1e89bu, 0xc04a62c2u, 0x3f72dce9u, 0x80000000u, 0x3ea1e89bu, 0xc04a62c2u, 0x3f64f92eu, 0x80000000u, 0x3ee4f92eu, 0xc0485a08u, 0x3f64f92eu, 0x80000000u, 0x3ee4f92eu, 0xc0485a08u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0x80000000u, 0xbf72dce9u, 0x00000000u, 0x3ea1e89bu, 0x4072dce9u, 0xbf72dce9u, 0x00000000u, 0x3ea1e89bu, 0x4072dce9u
	};
static const Nx2hMesh kMeshes2i[2] =
	{
	{ "large terrain", 289, 512, kMeshVerts2i0, kMeshTris2i0, kMeshPlanes2i0 },
	{ "steep terrain", 90, 144, kMeshVerts2i1, kMeshTris2i1, kMeshPlanes2i1 },
	};
// Each polytope's local bounds (least and greatest vertex word per axis, chosen
// offline from kPolyVertsN), for the convex mesh image's +0x44..+0x58.
static const unsigned kPolyBounds2i[5][6] =
	{
	{ 0xbf8a9067u, 0xbf200000u, 0xbf400000u, 0x3f8a9067u, 0x3fa00000u, 0x3f400000u },
	{ 0xbf800000u, 0xbf5db3d7u, 0xbf000000u, 0x3f800000u, 0x3f5db3d7u, 0x3f000000u },
	{ 0xbf35be97u, 0xbf1ce9e0u, 0xbfc00000u, 0x3f376cb3u, 0x3f3ffb13u, 0x3fc00000u },
	{ 0xbfc00000u, 0xbfc00000u, 0xbfc00000u, 0x3fc00000u, 0x3fc00000u, 0x3fc00000u },
	{ 0xbfa00000u, 0xbfa00000u, 0xbfa00000u, 0x3fa00000u, 0x3fa00000u, 0x3fa00000u }
	};

static const unsigned kContactConvexHeightfieldRva = 0x000432d0;	// phys_fn_001847
static const unsigned kContactConvexMeshRva = 0x00044500;			// phys_fn_001853 (a jmp to 001851)
static const unsigned kMeshVertexNormalsRva = 0x00052240;			// phys_fn_002081
static const unsigned kOpcModelCtor2i = 0x000e90a0;					// phys_fn_005362
static const unsigned kOpcModelDtor2i = 0x000e90e0;					// phys_fn_005366
static const unsigned kOpcModelBuild2i = 0x000e9100;				// phys_fn_005368
static const unsigned kOpcObbCtor2i = 0x000d4d00;					// phys_fn_005029, OBBCollider::OBBCollider
static const unsigned kOpcObbDtor2i = 0x000d4d20;
static const unsigned kNb2iMeshes = 10;
static const unsigned kHeightfield2iCases = 6000;
static const unsigned kConvexMesh2iCases = 5000;
static const unsigned kNormals2iVariants = 4;
// The two contact families' splits (.callee_divergent): frozen lists of the runs
// (2 * case + control word, 0 for 0x027f) on which the candidate's vendored
// callee and the oracle's differ, measured when the families were registered and
// attributed by a throwaway bind build, one callee a bit
// (evidence/convex-mesh-gap-2i-bind-oracle-callees.patch): contact_convex_heightfield's
// two are Triangle::Inflate's (005185, reached through 001822 -> 001708; 0 with
// the oracle's bound in), contact_convex_mesh's 44 OBBCollider::Collide's
// (005067, its primitive tests under rotated poses; 0 with the oracle's bound
// in). The lists route the splits; each family's live comparison only guards
// them (a run off its list that diverges fails, with detail on stderr, as the
// family's mismatch; a listed run that no longer diverges stays in the split and
// is logged IMPROVED). Each list is checked against its count and the digest of
// its run numbers before its family runs. The ceilings are the words and runs
// measured when the splits were registered (a count may fall, never rise).
static const unsigned kHeightfield2iDivergent[2] =
	{
	49, 9651,
	};
static const unsigned __int64 kHeightfield2iDivergentDigest = 0x60f18262738970e0ull;
static const unsigned kHeightfield2iDivergentWords = 3;
static const unsigned kHeightfield2iDivergentRuns = 2;
static const unsigned kConvexMesh2iDivergent[44] =
	{
	341, 1011, 1244, 1308, 1309, 1948, 1949, 2136, 2474, 2475, 2580, 2581,
	2722, 2723, 3246, 4124, 4125, 4488, 4489, 4577, 4946, 5195, 5365, 5422,
	5423, 5614, 5986, 5987, 6745, 6938, 6939, 6948, 6949, 7446, 8468, 8534,
	8535, 8715, 9200, 9294, 9660, 9661, 9802, 9803,
	};
static const unsigned __int64 kConvexMesh2iDivergentDigest = 0x5478912a9c90a7caull;
static const unsigned kConvexMesh2iDivergentWords = 51;
static const unsigned kConvexMesh2iDivergentRuns = 44;

// The frozen list's own check: its count and the digest of its run numbers in
// order (1 when they are the registered ones).
static unsigned nx2iCheckFrozen(const char* family, const unsigned* runs, unsigned count, unsigned expected,
	unsigned __int64 digest)
	{
	NxDigest d;
	nxDigestInit(&d);
	for(unsigned i = 0; i < count; ++i)
		nxFoldInput(&d, &runs[i], 4);
	if(count != expected || d.state != digest)
		{
		fprintf(stderr, "FAIL %s: the frozen list holds %u runs (digest %016llx), not %u (%016llx)\n", family, count,
			d.state, expected, digest);
		return 1;
		}
	return 0;
	}

static bool nx2iListed(const unsigned* runs, unsigned count, unsigned run)
	{
	for(unsigned i = 0; i < count; ++i)
		if(runs[i] == run)
			return true;
	return false;
	}
// Each mesh's OPCODE tree: its splitting rules (SPLIT_LARGEST_AXIS 1,
// SPLIT_SPLATTER_POINTS 2, SPLIT_BEST_AXIS 4, SPLIT_BALANCED 8, SPLIT_FIFTY 16,
// SPLIT_GEOM_CENTER 32) and kind (bit 0 no-leaf; never quantized). Chosen by an
// offline probe of every rule set and kind (evidence/convex-mesh-gap.md, Task 2i):
// each side builds its own tree, and these are the ones whose trees the oracle's
// and the vendored Model::Build build word for word on these meshes (on lattice
// meshes the splits hang on ties, and no quantized tree of a multi-triangle mesh
// agrees -- the opcode_model_build_x87 divergence); the fixture compares the
// trees again (build_mismatches). The query order is the tree's, so the touched
// lists, and with them the contacts, follow it.
static const unsigned kModelRules2i[10] = { 16, 8, 34, 2, 48, 34, 34, 2, 40, 36 };
static const unsigned kModelKind2i[10] = { 0, 1, 0, 1, 1, 0, 0, 1, 0, 1 };

// The mesh of index m: the Task 2h eight, then the two larger height fields.
static const Nx2hMesh& nx2iMesh(unsigned m)
	{
	return m < kNb2hMeshes ? kMeshes2h[m] : kMeshes2i[m - kNb2hMeshes];
	}

// One side's images of one mesh.
struct Nx2iMeshSide
	{
	unsigned		image[0x40];
	void*			iface;
	void*			model;
	unsigned char	failModel[0x80];
	// 001762 reads a three-bit per-triangle field at mesh +0x3c, with one
	// 16-byte record per triangle (the low three bits of dword 3).
	unsigned		triangleFlags[512][4];
	unsigned short	materials[512];
	unsigned		remap[512];
	unsigned		parts[3][512];
	unsigned		flats[3][512];
	};

struct Nx2iSide
	{
	bool				oracle;
	unsigned char*		base;
	unsigned char		context[0x500];
	unsigned			visited[1024];
	unsigned			touched[4];
	unsigned			pruner[8];
	unsigned			boxes[2][6];
	Nx2iMeshSide		meshes[kNb2iMeshes];
	};

// The fixed per-mesh words both sides copy: materials, remap and the drawn part
// layout (layout 1 is one part and one flat part for all, layout 2 one each).
struct Nx2iMeshWords
	{
	unsigned short	materials[512];
	unsigned		remap[512];
	unsigned		parts[512];
	unsigned		flats[512];
	};

static void nx2iDrawMeshWords(unsigned* state, NxDigest* fixture, unsigned m, Nx2iMeshWords& w)
	{
	const unsigned nbTris = nx2iMesh(m).nbTris;
	for(unsigned t = 0; t < nbTris; ++t)
		{
		const unsigned materialDraw = nxNext(state);
		const unsigned remapDraw = nxNext(state);
		const unsigned partDraw = nxNext(state);
		const unsigned flatDraw = nxNext(state);
		const unsigned draws[4] = { materialDraw, remapDraw, partDraw, flatDraw };
		nxFoldInput(fixture, draws, sizeof(draws));
		w.materials[t] = (unsigned short) ((materialDraw & 7) == 0 ? 0xffffu : materialDraw & 0x3ff);
		w.remap[t] = remapDraw;
		w.parts[t] = partDraw % 5;
		w.flats[t] = flatDraw % 3;
		}
	// The remap is a rotation of the triangle indices (by the first triangle's
	// draw): 001844 applies it to the triangle index in place once per contact it
	// emits (0x00042dfb, 0x00042f6e, 0x0004322e), so a triangle's later contacts
	// read the remap at an index already remapped; out-of-range words would be
	// read out of bounds there (a fixed-input rule).
	const unsigned rotate = w.remap[0];
	for(unsigned t = 0; t < nbTris; ++t)
		w.remap[t] = (t + rotate) % nbTris;
	}

static void nx2iInitSide(Nx2gSide& owner, Nx2iSide& s, const Nx2iMeshWords* words, unsigned* buildFailures)
	{
	s.oracle = owner.oracle;
	s.base = owner.base;
	memset(s.context, 0, sizeof(s.context));
	*(unsigned*) (s.context + 0x04) = 1024;
	*(unsigned**) (s.context + 0x08) = s.visited;
	memset(s.visited, 0, sizeof(s.visited));
	if(s.oracle)
		nx2gCall0(s.base + kOpcObbCtor2i, s.context + 0x110);
	else
		nx2iCandidateObbColliderConstruct(s.context + 0x110);
	// The capsule/mesh rows query through an LSSCollider at +0x28c and an
	// LSSCache at +0x304. The cache owns no Container: its first word points at
	// the caller-owned result list, as in Scene's SdkContainer-backed setup.
	if(s.oracle)
		nx2gCall0(s.base + 0x000d3490, s.context + 0x28c);
	else
		nx2iCandidateLssColliderConstruct(s.context + 0x28c);
	nx2gContainerCtor(owner, s.context + 0x4e0);
	*(unsigned**) (s.context + 0x304) = (unsigned*) (s.context + 0x4e0);
	*(unsigned*) (s.context + 0x308) = 0;
	memset(s.context + 0x30c, 0, 7 * sizeof(unsigned));
	*(unsigned*) (s.context + 0x328) = 0x3f8ccccdu;
	// The OBBCache, as the scene constructor 000647 leaves it (+0x244..+0x288),
	// its Container pointer at a Container the side's own constructor built.
	nx2gContainerCtor(owner, s.touched);
	*(unsigned**) (s.context + 0x244) = s.touched;
	*(unsigned*) (s.context + 0x264) = 0x3f800000u;
	*(unsigned*) (s.context + 0x274) = 0x3f800000u;
	*(unsigned*) (s.context + 0x284) = 0x3f800000u;
	*(unsigned*) (s.context + 0x288) = 0x3f8ccccdu;
	nx2gContainerCtor(owner, s.context + 0x4f0);
	memset(s.pruner, 0, sizeof(s.pruner));
	s.pruner[5] = (unsigned) (size_t) s.boxes;
	for(unsigned m = 0; m < kNb2iMeshes; ++m)
		{
		const Nx2hMesh& mesh = nx2iMesh(m);
		Nx2iMeshSide& ms = s.meshes[m];
		for(unsigned i = 0; i < 0x40; ++i)
			ms.image[i] = 0xcdcd8000u + i;
		for(unsigned t = 0; t < mesh.nbTris; ++t)
			{
			ms.materials[t] = words[m].materials[t];
			ms.remap[t] = words[m].remap[t];
			ms.parts[0][t] = words[m].parts[t];
			ms.parts[1][t] = 0;
			ms.parts[2][t] = t;
			ms.flats[0][t] = words[m].flats[t];
			ms.flats[1][t] = 0;
			ms.flats[2][t] = t;
			}
		ms.iface = nx2iMeshInterfaceNew(mesh.nbTris, mesh.nbVerts, mesh.tris, mesh.verts);
		unsigned built = 0;
		const unsigned kind = kModelKind2i[m];
		const unsigned rules = kModelRules2i[m];
		if(s.oracle)
			{
			ms.model = malloc(nx2iModelSize() + 64);
			memset(ms.model, 0xcd, nx2iModelSize() + 64);
			nx2iOracleModelBuild(ms.model, s.base + kOpcModelCtor2i, s.base + kOpcModelBuild2i, ms.iface, rules, kind,
				&built);
			}
		else
			ms.model = nx2iCandidateModelBuild(ms.iface, rules, kind, &built);
		if(!built)
			++*buildFailures;
		memset(ms.failModel, 0, sizeof(ms.failModel));
		memcpy(ms.failModel, ms.model, nx2iModelSize());
		const unsigned ifaceOffset = nx2iModelMeshInterfaceOffset(ms.model, ms.iface);
		if(!ifaceOffset)
			++*buildFailures;
		else
			memset(ms.failModel + ifaceOffset, 0, 4);
		ms.image[0x08 / 4] = mesh.nbVerts;
		ms.image[0x0c / 4] = mesh.nbTris;
		ms.image[0x10 / 4] = (unsigned) (size_t) mesh.verts;
		ms.image[0x14 / 4] = (unsigned) (size_t) mesh.tris;
		ms.image[0x18 / 4] = 0;
		ms.image[0x1c / 4] = 0;
		ms.image[0x20 / 4] = 0;
		ms.image[0x24 / 4] = (unsigned) (size_t) mesh.planes;
		ms.image[0x28 / 4] = (unsigned) (size_t) ms.model;
		memset(ms.triangleFlags, 0, sizeof(ms.triangleFlags));
		ms.image[0x3c / 4] = (unsigned) (size_t) ms.triangleFlags;
		ms.image[0x78 / 4] = 2;
		ms.image[0x7c / 4] = 2;
		ms.image[0x80 / 4] = 0x40400000u;
		ms.image[0x84 / 4] = 0;
		ms.image[0x88 / 4] = 0;
		ms.image[0x8c / 4] = 1;
		ms.image[0x90 / 4] = 1;
		ms.image[0x94 / 4] = (unsigned) (size_t) ms.parts[0];
		ms.image[0x98 / 4] = (unsigned) (size_t) ms.flats[0];
		ms.image[0xa0 / 4] = 0;
		}
	}

static void nx2iReleaseSide(Nx2gSide& owner, Nx2iSide& s)
	{
	for(unsigned m = 0; m < kNb2iMeshes; ++m)
		{
		Nx2iMeshSide& ms = s.meshes[m];
		void* edgeList = (void*) (size_t) ms.image[0x88 / 4];
		if(edgeList)
			{
			if(s.oracle)
				nx2gCall0(s.base + kEdgeListDtorRva, edgeList);
			else
				((EdgeList*) edgeList)->~EdgeList();
			nx2gFree(owner, edgeList);
			ms.image[0x88 / 4] = 0;
			}
		void* normals = (void*) (size_t) ms.image[0x20 / 4];
		if(normals)
			nx2iFoundationFree(normals);
		ms.image[0x20 / 4] = 0;
		if(s.oracle)
			{
			nx2gCall0(s.base + kOpcModelDtor2i, ms.model);
			free(ms.model);
			}
		else
			nx2iCandidateModelDelete(ms.model);
		nx2iMeshInterfaceDelete(ms.iface);
		}
	if(s.oracle)
		nx2gCall0(s.base + kOpcObbDtor2i, s.context + 0x110);
	else
		nx2iCandidateObbColliderDestruct(s.context + 0x110);
	if(s.oracle)
		nx2gCall0(s.base + 0x000d34b0, s.context + 0x28c);
	else
		nx2iCandidateLssColliderDestruct(s.context + 0x28c);
	nx2gContainerDtor(owner, s.touched);
	nx2gContainerDtor(owner, s.context + 0x4e0);
	nx2gContainerDtor(owner, s.context + 0x4f0);
	}

// The two sides' EdgeList face words for the meshes both built (as nx2hCompareEdgeLists).
static unsigned nx2iCompareEdgeLists(const Nx2iSide& a, const Nx2iSide& b, unsigned* built)
	{
	unsigned differing = 0;
	for(unsigned m = 0; m < kNb2iMeshes; ++m)
		{
		const unsigned* ea = (const unsigned*) (size_t) a.meshes[m].image[0x88 / 4];
		const unsigned* eb = (const unsigned*) (size_t) b.meshes[m].image[0x88 / 4];
		if(!ea || !eb)
			{
			differing += (ea != 0) != (eb != 0);
			continue;
			}
		++*built;
		const unsigned* fa = (const unsigned*) (size_t) ea[3];
		const unsigned* fb = (const unsigned*) (size_t) eb[3];
		differing += ea[0] != eb[0];
		for(unsigned i = 0; fa && fb && i < 3 * nx2iMesh(m).nbTris; ++i)
			differing += fa[i] != fb[i];
		}
	return differing;
	}

// The two sides' models, compared by their used bytes, node counts and node
// words (links as offsets into each side's own array).
static unsigned nx2iCompareModels(const Nx2iSide& a, const Nx2iSide& b)
	{
	unsigned differing = 0;
	static unsigned words[2][8192];
	for(unsigned m = 0; m < kNb2iMeshes; ++m)
		{
		unsigned usedA, nodesA, usedB, nodesB;
		nx2iModelShape(a.meshes[m].model, &usedA, &nodesA);
		nx2iModelShape(b.meshes[m].model, &usedB, &nodesB);
		differing += (usedA != usedB) + (nodesA != nodesB);
		const unsigned na = nx2iModelTreeWords(a.meshes[m].model, words[0], 8192);
		const unsigned nb = nx2iModelTreeWords(b.meshes[m].model, words[1], 8192);
		differing += na != nb;
		for(unsigned w = 0; w < na && w < nb; ++w)
			differing += words[0][w] != words[1][w];
		}
	return differing;
	}

extern "C" void __cdecl nxMeshTriangleEdgeNormal(float*, const void*, const float*,
	const float*, const void*, unsigned, unsigned);

// Task 2l's 001857 helper over independently built adjacency words derived
// from each side's identical mesh indices. Test shared and boundary edges.
static unsigned nxDriveTask2lEdgeNormal(unsigned char* base, Nx2iSide* sides)
	{
	typedef void (__cdecl * OracleFn)(float*, const void*, const float*, const float*, const void*, unsigned, unsigned);
	OracleFn oracle = (OracleFn)(base + 0x00044860);
	static const float transform[12] = { 1,0,0, 0,1,0, 0,0,1, 0,0,0 };
	static const unsigned edgeOrder[3] = { 0, 2, 1 };
	unsigned mismatches = 0, cases = 0, adjacent = 0, boundary = 0;
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest); nxDigestInit(&candidateDigest); nxDigestInit(&inputDigest);
	for(unsigned meshIndex = 0; meshIndex < kNb2iMeshes; ++meshIndex)
		{
		const Nx2hMesh& mesh = nx2iMesh(meshIndex);
		static unsigned adjacency[2][512 * 3];
		unsigned adjacencyObject[2][2] = {};
		const unsigned* tris = (const unsigned*)mesh.tris;
		for(unsigned side = 0; side < 2; ++side)
			for(unsigned i = 0; i < 3 * mesh.nbTris; ++i)
				adjacency[side][i] = 0x1fffffffu;
		for(unsigned tri = 0; tri < mesh.nbTris; ++tri)
			for(unsigned edge = 0; edge < 3; ++edge)
				{
				unsigned a = tris[tri * 3 + (edge + 1) % 3];
				unsigned b = tris[tri * 3 + (edge + 2) % 3];
				if(a > b) { unsigned swap = a; a = b; b = swap; }
				for(unsigned other = tri + 1; other < mesh.nbTris; ++other)
					for(unsigned otherEdge = 0; otherEdge < 3; ++otherEdge)
						{
						unsigned c = tris[other * 3 + (otherEdge + 1) % 3];
						unsigned d = tris[other * 3 + (otherEdge + 2) % 3];
						if(c > d) { unsigned swap = c; c = d; d = swap; }
						if(a == c && b == d)
							for(unsigned side = 0; side < 2; ++side)
								{
								adjacency[side][tri * 3 + edgeOrder[edge]] = other;
								adjacency[side][other * 3 + edgeOrder[otherEdge]] = tri;
								}
						}
				}
		for(unsigned side = 0; side < 2; ++side)
			adjacencyObject[side][1] = (unsigned)(size_t)adjacency[side];
		for(unsigned tri = 0; tri < mesh.nbTris && tri < 8; ++tri)
			for(unsigned edge = 0; edge < 3; ++edge)
			for(unsigned mode = 0; mode < 2; ++mode)
				{
			float output[2][3];
			float seed[3] = { 0.25f + 0.03125f * tri, -0.5f + 0.0625f * edge, 0.75f - 0.125f * mode };
			const unsigned mapWord = adjacency[0][tri * 3 + edgeOrder[edge]];
			for(unsigned side = 0; side < 2; ++side) memcpy(output[side], seed, sizeof(seed));
			const unsigned neighbor = mapWord & 0x1fffffffu;
			if(neighbor == 0x1fffffffu) ++boundary; else ++adjacent;
			const unsigned draws[5] = { meshIndex, tri, edge, mode, mapWord };
			nxFoldInput(&inputDigest, draws, sizeof(draws));
			nxFoldInput(&inputDigest, seed, sizeof(seed));
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			oracle(output[0], sides[0].meshes[meshIndex].image,
				transform, seed, adjacencyObject[0], tri, edge);
			nxMeshTriangleEdgeNormal(output[1], sides[1].meshes[meshIndex].image,
				transform, seed, adjacencyObject[1], tri, edge);
			nxSetControl(kControlDefault);
			++cases;
			for(unsigned axis = 0; axis < 3; ++axis)
				{
			mismatches += memcmp(&output[0][axis], &output[1][axis], sizeof(float)) != 0;
			nxFoldInput(&oracleDigest, &output[0][axis], sizeof(float));
			nxFoldInput(&candidateDigest, &output[1][axis], sizeof(float));
			}
			}
		}
	printf("collision name=mesh_adjacent_normal index=- rva=0x00044860 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		cases, oracleDigest.state, candidateDigest.state, mismatches);
	printf("collision coverage name=mesh_adjacent_normal meshes=%u cases=%u adjacent=%u boundary=%u control_words=2\n",
		kNb2iMeshes, cases, adjacent, boundary);
	nxPrintInput("mesh_adjacent_normal", &inputDigest);
	return mismatches;
	}

// Task 2l's mesh/mesh matrix-B entry (001870), compared directly because the
// out-of-range 001876 dispatcher is not part of this plan.
static unsigned nxDriveTask2lMeshOverlap(unsigned char* base, Nx2iSide* sides)
	{
	typedef bool (__cdecl * OracleFn)(const NxCollisionShape*, const NxCollisionShape*, void*);
	const OracleFn oracle = (OracleFn) (base + 0x46550);
	static unsigned char shapeStore[2][2][kShapeBytes];
	static const unsigned pairs[][2] = { { 0, 1 }, { 2, 3 }, { 8, 9 } };
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	for(unsigned side = 0; side < 2; ++side)
		nx2lCandidateAabbTreeColliderConstruct(sides[side].context + 0x32c);
	unsigned cases = 0, mismatches = 0;
	for(unsigned p = 0; p < sizeof(pairs) / sizeof(pairs[0]); ++p)
		for(unsigned mode = 0; mode < 2; ++mode)
			{
		bool result[2] = { false, false };
		unsigned flags[2] = { 0, 0 };
		for(unsigned side = 0; side < 2; ++side)
			{
			for(unsigned slot = 0; slot < 2; ++slot)
				{
				NxCollisionShape* shape = (NxCollisionShape*) shapeStore[side][slot];
				nxIdentity(shape);
				shape->type = 4;
				Nx2iMeshSide& mesh = sides[side].meshes[pairs[p][slot]];
				*(unsigned**) (shapeStore[side][slot] + 0xe0) = mesh.image;
				}
			memset(sides[side].context + 0x330, 0, 4);
			memset(sides[side].context + 0x440, 0, 0x30);
			const unsigned control = mode ? kControlSimulate : kControlDefault;
			nxSetControl(control);
			result[side] = side == 0
				? oracle((const NxCollisionShape*) shapeStore[side][0],
					(const NxCollisionShape*) shapeStore[side][1], sides[side].context)
				: nxOverlapMeshMesh((const NxCollisionShape*) shapeStore[side][0],
					(const NxCollisionShape*) shapeStore[side][1], sides[side].context);
			nxSetControl(kControlDefault);
			flags[side] = *(unsigned*) (sides[side].context + 0x330);
			}
		const unsigned in[3] = { pairs[p][0], pairs[p][1], mode };
		nxFoldInput(&inputDigest, in, sizeof(in));
		const Nx2hMesh& mesh0 = nx2iMesh(pairs[p][0]);
		const Nx2hMesh& mesh1 = nx2iMesh(pairs[p][1]);
		nxFoldInput(&inputDigest, mesh0.verts, 12 * mesh0.nbVerts);
		nxFoldInput(&inputDigest, mesh0.tris, 12 * mesh0.nbTris);
		nxFoldInput(&inputDigest, mesh1.verts, 12 * mesh1.nbVerts);
		nxFoldInput(&inputDigest, mesh1.tris, 12 * mesh1.nbTris);
		nxFoldInputShape(&inputDigest, (const NxCollisionShape*) shapeStore[0][0]);
		nxFoldInputShape(&inputDigest, (const NxCollisionShape*) shapeStore[0][1]);
		const unsigned out[2][2] = { { result[0], flags[0] }, { result[1], flags[1] } };
		nxFoldInput(&oracleDigest, out[0], sizeof(out[0]));
		nxFoldInput(&candidateDigest, out[1], sizeof(out[1]));
		mismatches += result[0] != result[1] || flags[0] != flags[1];
		++cases;
		}
	printf("collision name=overlap_mesh_mesh index=- rva=0x00046550 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		oracleDigest.checks, oracleDigest.state, candidateDigest.state, mismatches);
	nxPrintInput("overlap_mesh_mesh", &inputDigest);
	printf("collision coverage name=overlap_mesh_mesh pairs=%u cases=%u control_words=2\n",
		(unsigned) (sizeof(pairs) / sizeof(pairs[0])), cases);
	for(unsigned side = 0; side < 2; ++side)
		nx2lCandidateAabbTreeColliderDestruct(sides[side].context + 0x32c);
	return mismatches;
	}

// Words of the height field's placement: translations (0..3 across, heights -1..1).
static const unsigned kMeshPlace2i[8] =
	{
	0x00000000u, 0x00000000u, 0x00000000u, 0x3f800000u, 0xbf800000u, 0x3f000000u, 0x40000000u, 0xc0400000u
	};

// World-box spans for the convex shape's Prunable box: (lo, hi) pairs of lattice
// words, across (x, y) and up (z); some cover a whole mesh, some are empty
// (lo > hi).
static const unsigned kBoxSpans2i[12][2] =
	{
	{ 0x00000000u, 0x3f800000u }, { 0x00000000u, 0x40000000u }, { 0x00000000u, 0x40800000u },
	{ 0x3f800000u, 0x40400000u }, { 0x40000000u, 0x40800000u }, { 0x40000000u, 0x40c00000u },
	{ 0x40400000u, 0x40a00000u }, { 0x40800000u, 0x41000000u }, { 0xbf800000u, 0x3f800000u },
	{ 0xbf800000u, 0x41000000u }, { 0xc0000000u, 0x41900000u }, { 0x40000000u, 0x3f800000u },
	};
static const unsigned kBoxSpansUp2i[8][2] =
	{
	{ 0xbf800000u, 0x3f000000u }, { 0x00000000u, 0x3f800000u }, { 0x00000000u, 0x40400000u },
	{ 0xc0000000u, 0x40800000u }, { 0x3f800000u, 0x40000000u }, { 0x40200000u, 0x40800000u },
	{ 0xbf000000u, 0x40000000u }, { 0x40000000u, 0x3f800000u },
	};

// A convex pose whose 4x4 has an exact inverse (the rows' 001822 inverts it with
// the vendored Matrix4x4::Invert): a signed permutation (kind 0, 1) or a dyadic
// pose (kind 2, nx2hDyadicPose's). The shape keeps the rotation row major; 001847
// and 001851 transpose it into the 4x4, so the shape's word (r, c) is the
// dyadic 4x4's (c, r). `raw` puts nxPickRawWord's word in one translation word
// in sixteen.
static void nx2iConvexPose(unsigned* state, NxDigest* input, unsigned rotation[9], unsigned translation[3], bool raw,
	unsigned* snan, bool* dyadic)
	{
	const unsigned kindDraw = nxNext(state);
	const unsigned permDraw = nxNext(state);
	const unsigned signDraw = nxNext(state);
	const unsigned rawDraw = nxNext(state);
	const unsigned draws[4] = { kindDraw, permDraw, signDraw, rawDraw };
	nxFoldInput(input, draws, sizeof(draws));
	static const unsigned kPerm[6][3] = { { 0, 1, 2 }, { 0, 2, 1 }, { 1, 0, 2 }, { 1, 2, 0 }, { 2, 0, 1 }, { 2, 1, 0 } };
	*dyadic = kindDraw % 3 == 2;
	if(*dyadic)
		{
		unsigned m[16];
		nx2hDyadicPose(state, input, m);
		for(unsigned r = 0; r < 3; ++r)
			for(unsigned c = 0; c < 3; ++c)
				rotation[3 * r + c] = m[4 * c + r];
		for(unsigned k = 0; k < 3; ++k)
			translation[k] = m[12 + k];
		}
	else
		{
		for(unsigned i = 0; i < 9; ++i)
			rotation[i] = 0;
		for(unsigned row = 0; row < 3; ++row)
			rotation[3 * row + kPerm[permDraw % 6][row]] = ((signDraw >> row) & 1) ? 0xbf800000u : 0x3f800000u;
		for(unsigned k = 0; k < 3; ++k)
			{
			const unsigned pick = nxNext(state);
			nxFoldInput(input, &pick, 4);
			translation[k] = kPlace2h[k][pick % 8];
			}
		}
	if(raw && (rawDraw & 15) == 0)
		{
		float word;
		nxPickRawWord(state, &word);
		memcpy(&translation[(rawDraw >> 4) % 3], &word, 4);
		const unsigned bits = translation[(rawDraw >> 4) % 3];
		*snan += ((bits & 0x7fc00000u) == 0x7f800000u && (bits & 0x3fffffu)) ? 1 : 0;
		}
	nxFoldInput(input, rotation, 36);
	nxFoldInput(input, translation, 12);
	}

// A general convex pose for contact_convex_mesh (its rows invert nothing with
// Matrix4x4::Invert): the exact kinds above, or a rotation about an axis by
// fixed (cos, sin) words (nx2gRotation's kind 2), with a kPlace2h translation.
static void nx2iMeshSidePose(unsigned* state, NxDigest* input, unsigned rotation[9], unsigned translation[3],
	unsigned* kind)
	{
	const unsigned kindDraw = nxNext(state);
	const unsigned axisDraw = nxNext(state);
	const unsigned pairDraw = nxNext(state);
	const unsigned draws[3] = { kindDraw, axisDraw, pairDraw };
	nxFoldInput(input, draws, sizeof(draws));
	*kind = kindDraw % 4;
	for(unsigned i = 0; i < 9; ++i)
		rotation[i] = 0;
	if(*kind < 2)
		{
		// The identity, or a signed permutation that keeps the z axis.
		rotation[0] = 0x3f800000u;
		rotation[4] = 0x3f800000u;
		rotation[8] = 0x3f800000u;
		if(*kind == 1)
			{
			rotation[0] = 0;
			rotation[4] = 0;
			rotation[1] = (axisDraw & 1) ? 0xbf800000u : 0x3f800000u;
			rotation[3] = (axisDraw & 1) ? 0x3f800000u : 0xbf800000u;
			}
		}
	else
		{
		// About z (kind 2) or about x or y (kind 3) by fixed (cos, sin) words.
		const unsigned axis = *kind == 2 ? 2 : axisDraw % 2;
		const unsigned c = kCosSin2g[pairDraw % 4][0], s = kCosSin2g[pairDraw % 4][1];
		const unsigned a = (axis + 1) % 3, b = (axis + 2) % 3;
		rotation[3 * axis + axis] = 0x3f800000u;
		rotation[3 * a + a] = c;
		rotation[3 * b + b] = c;
		rotation[3 * a + b] = (pairDraw & 4) ? s : s | 0x80000000u;
		rotation[3 * b + a] = (pairDraw & 4) ? s | 0x80000000u : s;
		}
	for(unsigned k = 0; k < 3; ++k)
		{
		const unsigned pick = nxNext(state);
		nxFoldInput(input, &pick, 4);
		translation[k] = kMeshPlace2i[pick % 8];
		}
	nxFoldInput(input, rotation, 36);
	nxFoldInput(input, translation, 12);
	}

// Folds one side's visible state after a run into its digest and returns the
// words that differ from the other side's: the touched Container, the context's
// collider flags and stamp, the visited words of the mesh's vertices, the mesh's
// vertex normals (or their absence) and the reports.
struct Nx2iRunOut
	{
	unsigned words[4096];
	unsigned count;
	};

static void nx2iCollect(Nx2iRunOut& out, const Nx2iSide& s, unsigned m, const unsigned* reports, unsigned nbReports,
	bool axisContainers)
	{
	out.count = 0;
	const unsigned* touched = s.touched;
	out.words[out.count++] = touched[1];
	const unsigned* entries = (const unsigned*) (size_t) touched[2];
	for(unsigned i = 0; entries && i < touched[1] && out.count < 1024; ++i)
		out.words[out.count++] = entries[i];
	out.words[out.count++] = *(const unsigned*) (s.context + 0x114);
	out.words[out.count++] = *(const unsigned*) (s.context + 0x14);
	const unsigned nbVerts = nx2iMesh(m).nbVerts;
	for(unsigned v = 0; v < nbVerts; ++v)
		out.words[out.count++] = s.visited[v];
	const unsigned* normals = (const unsigned*) (size_t) s.meshes[m].image[0x20 / 4];
	out.words[out.count++] = normals ? 1u : 0u;
	for(unsigned v = 0; normals && v < 3 * nbVerts; ++v)
		out.words[out.count++] = normals[v];
	out.words[out.count++] = nbReports;
	for(unsigned r = 0; r < 4 * (nbReports < 4 ? nbReports : 4); ++r)
		out.words[out.count++] = reports[r];
	if(axisContainers)
		for(unsigned c = 0; c < 2; ++c)
			{
			const unsigned* container = (const unsigned*) (s.context + 0x4e0 + 0x10 * c);
			out.words[out.count++] = container[1];
			const unsigned* axes = (const unsigned*) (size_t) container[2];
			for(unsigned i = 0; axes && i < container[1] && out.count < 4000; ++i)
				out.words[out.count++] = axes[i];
			}
	}

static unsigned nx2iCompareRun(NxDigest* oracle, NxDigest* candidate, const Nx2iRunOut& a, const Nx2iRunOut& b)
	{
	nxFoldInput(oracle, &a.count, 4);
	nxFoldInput(oracle, a.words, 4 * a.count);
	nxFoldInput(candidate, &b.count, 4);
	nxFoldInput(candidate, b.words, 4 * b.count);
	if(a.count != b.count)
		return 1;
	unsigned differing = 0;
	for(unsigned i = 0; i < a.count; ++i)
		differing += a.words[i] != b.words[i];
	return differing;
	}

// The shapes of one run, staged into the side's world: the convex shape (its
// mesh image, pose, Prunable box handle and flags, feature word) and the mesh
// shape.
static void nx2iStage(NxContactWorld* world, unsigned char* convexMesh, const unsigned* convexRotation,
	const unsigned* convexTranslation, unsigned* meshImage, const unsigned* meshRotation,
	const unsigned* meshTranslation, unsigned* pruner, unsigned handle, unsigned shapeWord, unsigned flags0,
	unsigned flags1, NxU32 material0, NxU32 material1, bool nullHolder0, bool nullHolder1, bool orient)
	{
	static unsigned char store[2][kShapeBytes];
	unsigned char* convexShape = store[0];
	unsigned char* meshShape = store[1];
	nxIdentity((NxCollisionShape*) convexShape);
	nxIdentity((NxCollisionShape*) meshShape);
	memcpy(((NxCollisionShape*) convexShape)->rotation, convexRotation, 36);
	memcpy(((NxCollisionShape*) convexShape)->translation, convexTranslation, 12);
	memcpy(((NxCollisionShape*) meshShape)->rotation, meshRotation, 36);
	memcpy(((NxCollisionShape*) meshShape)->translation, meshTranslation, 12);
	((NxCollisionShape*) convexShape)->type = 4;
	((NxCollisionShape*) meshShape)->type = 4;
	*(unsigned char**) (convexShape + 0xe0) = convexMesh;
	*(unsigned**) (meshShape + 0xe0) = meshImage;
	convexShape[0xac] = 2;											// Prunable flags: no refresh
	*(unsigned**) (convexShape + 0xc4) = pruner;
	*(unsigned short*) (convexShape + 0xcc) = (unsigned short) handle;
	*(unsigned short*) (convexShape + 0xda) = (unsigned short) shapeWord;
	convexShape[0xde] = (unsigned char) (flags0 & 0x3f);
	meshShape[0xde] = (unsigned char) (flags1 & 0x3f);
	nxResetWorld(world);
	nxStageWorld(world, (NxCollisionShape*) convexShape, (NxCollisionShape*) meshShape, true, true, material0,
		material1, nullHolder0, nullHolder1, orient);
	}

static __declspec(noinline) unsigned nxDriveTask2i(unsigned char* base)
	{
	unsigned total = 0;
	static Nx2gSide boxSides[2], polySides[2];
	static Nx2gBox boxes[kNb2gHulls];
	static Nx2gGraph boxGraphs[kNb2gHulls], polyGraphs[5];
	static Nx2iSide sides[2];
	static Nx2iMeshWords meshWords[kNb2iMeshes];
	static const unsigned kSubdiv[5] = { 1, 2, 3, 5, 8 };

	if(nx2iObbColliderSize() > 0x244 - 0x110 || !nx2iObbColliderLayoutOk())
		{
		fprintf(stderr, "FAIL task 2i: the vendored OBBCollider (%u bytes) does not fit the context's +0x110..+0x244 or its flags word\n",
			nx2iObbColliderSize());
		return 1;
		}
	if(!nx2iFoundationBegin())
		{
		fprintf(stderr, "FAIL task 2i: a Foundation SDK already existed; the report arms would not reach the recording stream\n");
		++total;
		}

	// The fixture: its words are fixed inputs, folded into every family's input.
	NxDigest fixture;
	nxDigestInit(&fixture);
	unsigned state = 0x2e8b0000u;
	unsigned mapSubdiv[kNb2gHulls];
	for(unsigned h = 0; h < kNb2gHulls; ++h)
		{
		nx2gBuildBox(&state, boxes[h], h >= kNb2gHulls - 2);
		nx2gBuildGraph(boxes[h], boxGraphs[h]);
		const unsigned subdivDraw = nxNext(&state);
		mapSubdiv[h] = (h & 1) ? kSubdiv[subdivDraw % 4] : 0;
		nxFoldInput(&fixture, &subdivDraw, 4);
		nxFoldInput(&fixture, boxes[h].verts, sizeof(boxes[h].verts));
		nxFoldInput(&fixture, boxes[h].tris, sizeof(boxes[h].tris));
		nxFoldInput(&fixture, boxes[h].centre, sizeof(boxes[h].centre));
		nxFoldInput(&fixture, &mapSubdiv[h], 4);
		}
	for(unsigned k = 0; k < 5; ++k)
		{
		nx2gBuildPolytopeGraph(kPolytopes2g[k], polyGraphs[k]);
		nxFoldInput(&fixture, kPolytopes2g[k].verts, 12 * kPolytopes2g[k].nbVerts);
		nxFoldInput(&fixture, kPolytopes2g[k].refs, 4 * kPolytopes2g[k].nbRefs);
		nxFoldInput(&fixture, kPolytopes2g[k].faces, 32 * kPolytopes2g[k].nbPolygons);
		nxFoldInput(&fixture, &kSubdiv[k], 4);
		nxFoldInput(&fixture, kPolyBounds2i[k], 24);
		}
	for(unsigned m = 0; m < kNb2iMeshes; ++m)
		{
		const Nx2hMesh& mesh = nx2iMesh(m);
		nxFoldInput(&fixture, mesh.verts, 12 * mesh.nbVerts);
		nxFoldInput(&fixture, mesh.tris, 12 * mesh.nbTris);
		nxFoldInput(&fixture, mesh.planes, 16 * mesh.nbTris);
		nx2iDrawMeshWords(&state, &fixture, m, meshWords[m]);
		}
	unsigned buildFailures = 0;
	for(int side = 0; side < 2; ++side)
		{
		for(int which = 0; which < 2; ++which)
			{
			Nx2gSide& s = which ? polySides[side] : boxSides[side];
			s.oracle = side == 0;
			s.base = base;
			}
		for(unsigned h = 0; h < kNb2gHulls; ++h)
			{
			Nx2gHullSide& hs = boxSides[side].hulls[h];
			nx2gBuildHull(boxSides[side], hs, boxes[h], &boxGraphs[h], mapSubdiv[h]);
			memcpy(hs.mesh + 0x44, boxes[h].lo, 12);
			memcpy(hs.mesh + 0x50, boxes[h].hi, 12);
			}
		for(unsigned h = 0; h < kNb2gPolytopeHulls; ++h)
			{
			Nx2gHullSide& hs = polySides[side].hulls[h];
			nx2gBuildPolytope(polySides[side], hs, kPolytopes2g[h % 5], &polyGraphs[h % 5], h >= 5 ? kSubdiv[h % 5] : 0);
			memcpy(hs.mesh + 0x44, kPolyBounds2i[h % 5], 24);
			}
		nx2iInitSide(boxSides[side], sides[side], meshWords, &buildFailures);
		}
	unsigned buildMismatches = buildFailures + nx2iCompareModels(sides[0], sides[1]);
	for(unsigned h = 0; h < kNb2gHulls; ++h)
		buildMismatches += nx2gCompareEdges(boxSides[0].hulls[h].hull, boxSides[1].hulls[h].hull);
	for(unsigned h = 0; h < kNb2gPolytopeHulls; ++h)
		buildMismatches += nx2gCompareEdges(polySides[0].hulls[h].hull, polySides[1].hulls[h].hull);

	typedef void(__cdecl* NxOracleContactFn)(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*);

	// -----------------------------------------------------------------------
	// contact_convex_heightfield: phys_fn_001847.
	{
	const NxOracleContactFn oracleEntry = (NxOracleContactFn) (base + kContactConvexHeightfieldRva);
	static NxContactWorld world[2];
	static Nx2iRunOut out[2];
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	nxFoldInput(&inputDigest, &fixture.state, 8);
	NxDigest splitOracle, splitCandidate;
	nxDigestInit(&splitOracle);
	nxDigestInit(&splitCandidate);
	unsigned splitWords = 0, splitRuns = 0, splitCount = 0, improved = 0;
	total += nx2iCheckFrozen("contact_convex_heightfield", kHeightfield2iDivergent, sizeof(kHeightfield2iDivergent) / 4,
		2, kHeightfield2iDivergentDigest);
	unsigned perMode[2] = { 0, 0 };
	unsigned cases = 0, withContacts = 0, contacts = 0, headers = 0, touchedTotal = 0, normalsBuilt = 0,
		failed = 0, polytopeCases = 0, dyadicCases = 0, otherAxes = 0, flipped = 0, rawBox = 0, meshPosed = 0,
		exactSimulateContacts = 0, largeMesh = 0, inputSnan = 0, maxStream = 0;
	unsigned local = 0x2e8b1000u;
	for(unsigned i = 0; i < kHeightfield2iCases; ++i)
		{
		const unsigned hullDraw = nxNext(&local);
		const unsigned meshDraw = nxNext(&local);
		const unsigned axisDraw = nxNext(&local);
		const unsigned handleDraw = nxNext(&local);
		const unsigned flagsDraw0 = nxNext(&local);
		const unsigned flagsDraw1 = nxNext(&local);
		const unsigned materialDraw0 = nxNext(&local);
		const unsigned materialDraw1 = nxNext(&local);
		const unsigned holderDraw = nxNext(&local);
		const unsigned orientDraw = nxNext(&local);
		const unsigned stampDraw = nxNext(&local);
		const unsigned arraysDraw = nxNext(&local);
		const unsigned normalsDraw = nxNext(&local);
		const unsigned failDraw = nxNext(&local);
		const unsigned shapeWordDraw = nxNext(&local);
		const unsigned draws[15] = { hullDraw, meshDraw, axisDraw, handleDraw, flagsDraw0, flagsDraw1, materialDraw0,
			materialDraw1, holderDraw, orientDraw, stampDraw, arraysDraw, normalsDraw, failDraw, shapeWordDraw };
		nxFoldInput(&inputDigest, draws, sizeof(draws));
		const unsigned k = hullDraw % (kNb2gHulls + kNb2gPolytopeHulls);
		// Every mesh but the single triangle (index 5): its model is one node with no
		// tree, and 001847's query sets OPC_NO_PRIMITIVE_TESTS, under which the
		// collider walks the tree it does not have (0x000de215 reads through null;
		// a fixed-input rule).
		const unsigned meshPick = meshDraw % (kNb2iMeshes - 1);
		const unsigned m = meshPick < 5 ? meshPick : meshPick + 1;
		unsigned convexRotation[9], convexTranslation[3];
		bool dyadic = false;
		nx2iConvexPose(&local, &inputDigest, convexRotation, convexTranslation, true, &inputSnan, &dyadic);
		unsigned meshRotation[9], meshTranslation[3], meshKind = 0;
		nx2iMeshSidePose(&local, &inputDigest, meshRotation, meshTranslation, &meshKind);
		// The Prunable box: per axis a span of lattice words, one word in sixteen a
		// raw word (signalling NaNs kept: the row loads them with `fld`).
		unsigned box[6];
		for(unsigned a = 0; a < 3; ++a)
			{
			const unsigned spanDraw = nxNext(&local);
			nxFoldInput(&inputDigest, &spanDraw, 4);
			const unsigned* span = a < 2 ? kBoxSpans2i[spanDraw % 12] : kBoxSpansUp2i[spanDraw % 8];
			box[a] = span[0];
			box[3 + a] = span[1];
			}
		const unsigned rawBoxDraw = nxNext(&local);
		nxFoldInput(&inputDigest, &rawBoxDraw, 4);
		if((rawBoxDraw & 15) == 0)
			{
			float word;
			nxPickRawWord(&local, &word);
			memcpy(&box[(rawBoxDraw >> 4) % 6], &word, 4);
			const unsigned bits = box[(rawBoxDraw >> 4) % 6];
			inputSnan += ((bits & 0x7fc00000u) == 0x7f800000u && (bits & 0x3fffffu)) ? 1 : 0;
			++rawBox;
			}
		nxFoldInput(&inputDigest, box, sizeof(box));
		// The height field's flags (+0x78: the up component in bits 0-1, bit 3 its
		// sign) and vertical axis (+0x7c): mostly z up as the meshes are built,
		// sometimes another axis, a flags word that disagrees with the axis, or
		// the component 3 (the up vector is then written to the local after it).
		const unsigned axisKind = axisDraw % 8;
		unsigned flagsWord = 2, axisWord = 2;
		if(axisKind == 5)
			flagsWord = axisWord = (axisDraw >> 3) % 2;
		else if(axisKind == 6)
			{
			flagsWord = (axisDraw >> 3) % 3;
			axisWord = (axisDraw >> 5) % 3;
			}
		else if(axisKind == 7)
			flagsWord = 3;
		if(((axisDraw >> 8) & 7) == 0)
			flagsWord |= 8;
		flagsWord |= axisDraw & 0xfff00000u;
		nxFoldInput(&inputDigest, &flagsWord, 4);
		nxFoldInput(&inputDigest, &axisWord, 4);
		const unsigned handle = handleDraw & 1;
		const unsigned stamp = (stampDraw & 15) == 0 ? 0xfffffffeu : stampDraw & 0xffffu;
		const bool withMaterials = (arraysDraw & 1) != 0;
		const bool withRemap = (arraysDraw & 2) != 0;
		const bool resetNormals = normalsDraw % 8 == 0;
		const bool fail = failDraw % 64 == 0;
		const bool nullHolder0 = holderDraw % 8 == 1;
		const bool nullHolder1 = holderDraw % 8 == 2;
		const bool orient = (orientDraw & 1) != 0;
		const NxU32 material0 = materialDraw0 & 0xff;
		const NxU32 material1 = materialDraw1 & 0xff;
		const unsigned shapeWord = shapeWordDraw & 0xffffu;
		const unsigned settings[6] = { handle, stamp, withMaterials ? 1u : 0u, withRemap ? 1u : 0u, resetNormals ? 1u : 0u,
			fail ? 1u : 0u };
		nxFoldInput(&inputDigest, settings, sizeof(settings));
		++cases;
		if(k >= kNb2gHulls)
			++polytopeCases;
		if(dyadic)
			++dyadicCases;
		if(axisKind >= 5)
			++otherAxes;
		if(flagsWord & 8)
			++flipped;
		if(meshKind)
			++meshPosed;
		if(m >= kNb2hMeshes)
			++largeMesh;
		if(fail)
			++failed;
		for(int mode = 0; mode < 2; ++mode)
			{
			unsigned reports[2][16];
			unsigned nbReports[2] = { 0, 0 };
			bool normalsBefore = true;
			for(int side = 0; side < 2; ++side)
				{
				Nx2iSide& s = sides[side];
				Nx2iMeshSide& ms = s.meshes[m];
				Nx2gHullSide& h = nx2hHull(boxSides, polySides, side, k);
				memcpy(s.boxes[handle], box, 24);
				ms.image[0x18 / 4] = withMaterials ? (unsigned) (size_t) ms.materials : 0u;
				ms.image[0x1c / 4] = withRemap ? (unsigned) (size_t) ms.remap : 0u;
				ms.image[0x28 / 4] = fail ? (unsigned) (size_t) ms.failModel : (unsigned) (size_t) ms.model;
				ms.image[0x78 / 4] = flagsWord;
				ms.image[0x7c / 4] = axisWord;
				if(mode == 0 && resetNormals && ms.image[0x20 / 4])
					{
					nx2iFoundationFree((void*) (size_t) ms.image[0x20 / 4]);
					ms.image[0x20 / 4] = 0;
					}
				if(side == 0)
					normalsBefore = ms.image[0x20 / 4] != 0;
				*(unsigned*) (s.context + 0x14) = stamp;
				// Every run starts from the same visited words (the stamp's own words are
				// drawn) and empty Containers (the touched list, the two axis lists; their
				// blocks kept), so one run's differences cannot reach the next.
				memset(s.visited, 0, sizeof(s.visited));
				s.touched[1] = 0;
				((unsigned*) (s.context + 0x4e0))[1] = 0;
				((unsigned*) (s.context + 0x4f0))[1] = 0;
				nx2iStage(&world[side], h.mesh, convexRotation, convexTranslation, ms.image, meshRotation,
					meshTranslation, s.pruner, handle, shapeWord, flagsDraw0, flagsDraw1, material0, material1,
					nullHolder0, nullHolder1, orient);
				}
			nx2iTakeReports(reports[0], 4);
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			oracleEntry(world[0].plane, world[0].sphere, &world[0].sink, sides[0].context);
			nxSetControl(kControlDefault);
			nbReports[0] = nx2iTakeReports(reports[0], 4);
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			NxContactConvexHeightfield(world[1].plane, world[1].sphere, &world[1].sink, sides[1].context);
			nxSetControl(kControlDefault);
			nbReports[1] = nx2iTakeReports(reports[1], 4);
			for(int side = 0; side < 2; ++side)
				{
				sides[side].meshes[m].image[0x28 / 4] = (unsigned) (size_t) sides[side].meshes[m].model;
				nx2iCollect(out[side], sides[side], m, reports[side], nbReports[side], false);
				}
			// The split: the runs on the frozen list (see kHeightfield2iDivergent).
			const unsigned run = 2 * i + (unsigned) mode;
			const bool split = nx2iListed(kHeightfield2iDivergent, sizeof(kHeightfield2iDivergent) / 4, run);
			nxFoldStream(split ? &splitOracle : &oracleDigest, &world[0]);
			nxFoldStream(split ? &splitCandidate : &candidateDigest, &world[1]);
			const unsigned differing = nxCompareStreams(&world[0], &world[1], mode)
				+ nx2iCompareRun(split ? &splitOracle : &oracleDigest, split ? &splitCandidate : &candidateDigest,
					out[0], out[1]);
			if(split)
				{
				++splitCount;
				splitWords += differing;
				if(differing)
					++splitRuns;
				else
					{
					fprintf(stderr, "IMPROVED contact_convex_heightfield: run %u (case %u, control word %04x) agrees now; it stays in the split\n",
						run, i, mode ? kControlSimulate : kControlDefault);
					++improved;
					}
				}
			else
				{
				perMode[mode] += differing;
				if(differing)
					fprintf(stderr, "FAIL contact_convex_heightfield: run %u (case %u, control word %04x, hull %u, mesh %u) differs in %u words and is not on the frozen list\n",
						run, i, mode ? kControlSimulate : kControlDefault, k, m, differing);
				}
			if(world[0].sink.streamCount > maxStream)
				maxStream = world[0].sink.streamCount;
			if(mode == 0)
				{
				if(world[0].sink.contactCount)
					++withContacts;
				contacts += world[0].sink.contactCount;
				headers += world[0].stream[0];
				touchedTotal += sides[0].touched[1];
				if(!normalsBefore && sides[0].meshes[m].image[0x20 / 4])
					++normalsBuilt;
				}
			else if(world[0].sink.contactCount && !split)
				++exactSimulateContacts;
			}
		}
	const bool splitOver = splitWords > kHeightfield2iDivergentWords || splitRuns > kHeightfield2iDivergentRuns;
	total += perMode[0] + perMode[1] + (splitOver ? 1 : 0);
	printf("collision name=contact_convex_heightfield index=- rva=0x%08x owner=phys_fn_001847 checks=%u oracle=%016llx candidate=%016llx mismatches=%u default_mismatches=%u simulate_mismatches=%u\n",
		kContactConvexHeightfieldRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state,
		perMode[0] + perMode[1], perMode[0], perMode[1]);
	printf("collision name=contact_convex_heightfield.callee_divergent index=- rva=0x%08x owner=phys_fn_001847 checks=%u oracle=%016llx candidate=%016llx words=%u runs=%u ceiling_words=%u ceiling_runs=%u ceiling=%s split_runs=%u improved=%u\n",
		kContactConvexHeightfieldRva, splitOracle.checks, splitOracle.state, splitCandidate.state, splitWords, splitRuns,
		kHeightfield2iDivergentWords, kHeightfield2iDivergentRuns, splitOver ? "exceeded" : "ok", splitCount, improved);
	nxPrintInput("contact_convex_heightfield", &inputDigest);
	printf("collision coverage name=contact_convex_heightfield hulls=%u meshes=%u cases=%u cases_with_contacts=%u contacts=%u headers=%u touched=%u normals_built=%u failed=%u polytope_cases=%u dyadic=%u other_axes=%u flipped=%u mesh_posed=%u large_mesh=%u raw_box=%u simulate_runs_with_contacts=%u max_stream=%u input_snan=%u\n",
		kNb2gHulls + kNb2gPolytopeHulls, kNb2iMeshes, cases, withContacts, contacts, headers, touchedTotal, normalsBuilt,
		failed, polytopeCases, dyadicCases, otherAxes, flipped, meshPosed, largeMesh, rawBox, exactSimulateContacts,
		maxStream, inputSnan);
	}

	// -----------------------------------------------------------------------
	// contact_convex_mesh: phys_fn_001853 (a jmp to 001851).
	{
	const NxOracleContactFn oracleEntry = (NxOracleContactFn) (base + kContactConvexMeshRva);
	static NxContactWorld world[2];
	static Nx2iRunOut out[2];
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	nxFoldInput(&inputDigest, &fixture.state, 8);
	NxDigest splitOracle, splitCandidate;
	nxDigestInit(&splitOracle);
	nxDigestInit(&splitCandidate);
	unsigned splitWords = 0, splitRuns = 0, splitCount = 0, improved = 0;
	total += nx2iCheckFrozen("contact_convex_mesh", kConvexMesh2iDivergent, sizeof(kConvexMesh2iDivergent) / 4,
		44, kConvexMesh2iDivergentDigest);
	unsigned perMode[2] = { 0, 0 };
	unsigned cases = 0, withContacts = 0, contacts = 0, headers = 0, touchedTotal = 0, failed = 0, polytopeCases = 0,
		rotated = 0, layouts[3] = { 0, 0, 0 }, axes = 0, largeMesh = 0, maxStream = 0, simulateContacts = 0,
		ccdPairs = 0;
	unsigned local = 0x2e8b2000u;
	for(unsigned i = 0; i < kConvexMesh2iCases; ++i)
		{
		const unsigned hullDraw = nxNext(&local);
		const unsigned meshDraw = nxNext(&local);
		const unsigned layoutDraw = nxNext(&local);
		const unsigned flagsDraw0 = nxNext(&local);
		const unsigned flagsDraw1 = nxNext(&local);
		const unsigned materialDraw0 = nxNext(&local);
		const unsigned materialDraw1 = nxNext(&local);
		const unsigned holderDraw = nxNext(&local);
		const unsigned orientDraw = nxNext(&local);
		const unsigned stampDraw = nxNext(&local);
		const unsigned arraysDraw = nxNext(&local);
		const unsigned failDraw = nxNext(&local);
		const unsigned shapeWordDraw = nxNext(&local);
		const unsigned draws[13] = { hullDraw, meshDraw, layoutDraw, flagsDraw0, flagsDraw1, materialDraw0, materialDraw1,
			holderDraw, orientDraw, stampDraw, arraysDraw, failDraw, shapeWordDraw };
		nxFoldInput(&inputDigest, draws, sizeof(draws));
		const unsigned k = nx2hSmallHull(hullDraw);
		const unsigned m = meshDraw % kNb2iMeshes;
		unsigned convexRotation[9], convexTranslation[3], convexKind = 0;
		const unsigned convexPoseDraw = nxNext(&local);
		nxFoldInput(&inputDigest, &convexPoseDraw, 4);
		if(convexPoseDraw % 2)
			{
			bool dyadic = false;
			unsigned ignored = 0;
			nx2iConvexPose(&local, &inputDigest, convexRotation, convexTranslation, false, &ignored, &dyadic);
			}
		else
			{
			nx2iMeshSidePose(&local, &inputDigest, convexRotation, convexTranslation, &convexKind);
			// Lift the convex off the translations' low words: kPlace2h's.
			for(unsigned a = 0; a < 3; ++a)
				{
				const unsigned pick = nxNext(&local);
				nxFoldInput(&inputDigest, &pick, 4);
				convexTranslation[a] = kPlace2h[a][pick % 8];
				}
			}
		unsigned meshRotation[9], meshTranslation[3], meshKind = 0;
		nx2iMeshSidePose(&local, &inputDigest, meshRotation, meshTranslation, &meshKind);
		const unsigned layout = layoutDraw % 3;
		const unsigned stamp = (stampDraw & 15) == 0 ? 0xfffffffeu : stampDraw & 0xffffu;
		const bool withMaterials = (arraysDraw & 1) != 0;
		const bool withRemap = (arraysDraw & 2) != 0;
		const bool fail = failDraw % 64 == 0;
		const bool nullHolder0 = holderDraw % 8 == 1;
		const bool nullHolder1 = holderDraw % 8 == 2;
		const bool orient = (orientDraw & 1) != 0;
		const NxU32 material0 = materialDraw0 & 0xff;
		const NxU32 material1 = materialDraw1 & 0xff;
		const unsigned shapeWord = shapeWordDraw & 0xffffu;
		const unsigned settings[6] = { layout, stamp, withMaterials ? 1u : 0u, withRemap ? 1u : 0u, fail ? 1u : 0u,
			shapeWord };
		nxFoldInput(&inputDigest, settings, sizeof(settings));
		++cases;
		++layouts[layout];
		if(k >= kNb2gHulls)
			++polytopeCases;
		if(convexKind >= 2 || meshKind >= 2)
			++rotated;
		if(m >= kNb2hMeshes)
			++largeMesh;
		if(fail)
			++failed;
		if(nullHolder1)
			++ccdPairs;
		for(int mode = 0; mode < 2; ++mode)
			{
			unsigned reports[2][16];
			unsigned nbReports[2] = { 0, 0 };
			for(int side = 0; side < 2; ++side)
				{
				Nx2iSide& s = sides[side];
				Nx2iMeshSide& ms = s.meshes[m];
				Nx2gHullSide& h = nx2hHull(boxSides, polySides, side, k);
				ms.image[0x18 / 4] = withMaterials ? (unsigned) (size_t) ms.materials : 0u;
				ms.image[0x1c / 4] = withRemap ? (unsigned) (size_t) ms.remap : 0u;
				ms.image[0x28 / 4] = fail ? (unsigned) (size_t) ms.failModel : (unsigned) (size_t) ms.model;
				ms.image[0x94 / 4] = (unsigned) (size_t) ms.parts[layout];
				ms.image[0x98 / 4] = (unsigned) (size_t) ms.flats[layout];
				*(unsigned*) (s.context + 0x14) = stamp;
				// Every run starts from the same visited words (the stamp's own words are
				// drawn) and empty Containers (the touched list, the two axis lists; their
				// blocks kept), so one run's differences cannot reach the next.
				memset(s.visited, 0, sizeof(s.visited));
				s.touched[1] = 0;
				((unsigned*) (s.context + 0x4e0))[1] = 0;
				((unsigned*) (s.context + 0x4f0))[1] = 0;
				nx2iStage(&world[side], h.mesh, convexRotation, convexTranslation, ms.image, meshRotation,
					meshTranslation, s.pruner, 0, shapeWord, flagsDraw0, flagsDraw1, material0, material1,
					nullHolder0, nullHolder1, orient);
				}
			nx2iTakeReports(reports[0], 4);
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			oracleEntry(world[0].plane, world[0].sphere, &world[0].sink, sides[0].context);
			nxSetControl(kControlDefault);
			nbReports[0] = nx2iTakeReports(reports[0], 4);
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			NxContactConvexMesh(world[1].plane, world[1].sphere, &world[1].sink, sides[1].context);
			nxSetControl(kControlDefault);
			nbReports[1] = nx2iTakeReports(reports[1], 4);
			for(int side = 0; side < 2; ++side)
				{
				sides[side].meshes[m].image[0x28 / 4] = (unsigned) (size_t) sides[side].meshes[m].model;
				nx2iCollect(out[side], sides[side], m, reports[side], nbReports[side], true);
				}
			// The split: the runs on the frozen list (see kHeightfield2iDivergent).
			const unsigned run = 2 * i + (unsigned) mode;
			const bool split = nx2iListed(kConvexMesh2iDivergent, sizeof(kConvexMesh2iDivergent) / 4, run);
			nxFoldStream(split ? &splitOracle : &oracleDigest, &world[0]);
			nxFoldStream(split ? &splitCandidate : &candidateDigest, &world[1]);
			const unsigned differing = nxCompareStreams(&world[0], &world[1], mode)
				+ nx2iCompareRun(split ? &splitOracle : &oracleDigest, split ? &splitCandidate : &candidateDigest,
					out[0], out[1]);
			if(split)
				{
				++splitCount;
				splitWords += differing;
				if(differing)
					++splitRuns;
				else
					{
					fprintf(stderr, "IMPROVED contact_convex_mesh: run %u (case %u, control word %04x) agrees now; it stays in the split\n",
						run, i, mode ? kControlSimulate : kControlDefault);
					++improved;
					}
				}
			else
				{
				perMode[mode] += differing;
				if(differing)
					fprintf(stderr, "FAIL contact_convex_mesh: run %u (case %u, control word %04x, hull %u, mesh %u) differs in %u words and is not on the frozen list\n",
						run, i, mode ? kControlSimulate : kControlDefault, k, m, differing);
				}
			if(world[0].sink.streamCount > maxStream)
				maxStream = world[0].sink.streamCount;
			if(mode == 0)
				{
				if(world[0].sink.contactCount)
					++withContacts;
				contacts += world[0].sink.contactCount;
				headers += world[0].stream[0];
				touchedTotal += sides[0].touched[1];
				axes += ((const unsigned*) (sides[0].context + 0x4e0))[1] + ((const unsigned*) (sides[0].context + 0x4f0))[1];
				}
			else if(world[0].sink.contactCount && !split)
				++simulateContacts;
			}
		}
	const bool splitOver = splitWords > kConvexMesh2iDivergentWords || splitRuns > kConvexMesh2iDivergentRuns;
	total += perMode[0] + perMode[1] + (splitOver ? 1 : 0);
	printf("collision name=contact_convex_mesh index=- rva=0x%08x owner=phys_fn_001853 checks=%u oracle=%016llx candidate=%016llx mismatches=%u default_mismatches=%u simulate_mismatches=%u\n",
		kContactConvexMeshRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state,
		perMode[0] + perMode[1], perMode[0], perMode[1]);
	printf("collision name=contact_convex_mesh.callee_divergent index=- rva=0x%08x owner=phys_fn_001853 checks=%u oracle=%016llx candidate=%016llx words=%u runs=%u ceiling_words=%u ceiling_runs=%u ceiling=%s split_runs=%u improved=%u\n",
		kContactConvexMeshRva, splitOracle.checks, splitOracle.state, splitCandidate.state, splitWords, splitRuns,
		kConvexMesh2iDivergentWords, kConvexMesh2iDivergentRuns, splitOver ? "exceeded" : "ok", splitCount, improved);
	nxPrintInput("contact_convex_mesh", &inputDigest);
	printf("collision coverage name=contact_convex_mesh hulls=%u meshes=%u cases=%u cases_with_contacts=%u contacts=%u headers=%u touched=%u axes=%u failed=%u polytope_cases=%u rotated=%u layouts=%u/%u/%u large_mesh=%u ccd_pairs=%u simulate_runs_with_contacts=%u max_stream=%u\n",
		kNb2gHulls + kNb2gPolytopeHulls, kNb2iMeshes, cases, withContacts, contacts, headers, touchedTotal, axes, failed,
		polytopeCases, rotated, layouts[0], layouts[1], layouts[2], largeMesh, ccdPairs, simulateContacts, maxStream);
	}

	// -----------------------------------------------------------------------
	// mesh_vertex_normals: phys_fn_002081.
	{
	NxDigest oracleDigest, candidateDigest, inputDigest;
	nxDigestInit(&oracleDigest);
	nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	unsigned perMode[2] = { 0, 0 };
	unsigned runs = 0, normals = 0, rawWords = 0, inputSnan = 0;
	static unsigned verts[2][3 * 512];
	static unsigned image[2][0x40];
	unsigned local = 0x2e8b3000u;
	for(unsigned m = 0; m < kNb2iMeshes; ++m)
		for(unsigned variant = 0; variant < kNormals2iVariants; ++variant)
			{
			const Nx2hMesh& mesh = nx2iMesh(m);
			// Variant 0 the mesh's words; the others with a few words replaced by
			// raw words (signalling NaNs, infinities, denormals, -0 kept).
			memcpy(verts[0], mesh.verts, 12 * mesh.nbVerts);
			if(variant)
				for(unsigned r = 0; r < 1 + variant; ++r)
					{
					const unsigned where = nxNext(&local);
					float word;
					nxPickRawWord(&local, &word);
					memcpy(&verts[0][where % (3 * mesh.nbVerts)], &word, 4);
					const unsigned bits = verts[0][where % (3 * mesh.nbVerts)];
					inputSnan += ((bits & 0x7fc00000u) == 0x7f800000u && (bits & 0x3fffffu)) ? 1 : 0;
					nxFoldInput(&inputDigest, &where, 4);
					++rawWords;
					}
			memcpy(verts[1], verts[0], 12 * mesh.nbVerts);
			nxFoldInput(&inputDigest, verts[0], 12 * mesh.nbVerts);
			nxFoldInput(&inputDigest, mesh.tris, 12 * mesh.nbTris);
			for(int mode = 0; mode < 2; ++mode)
				{
				unsigned sizes[2] = { 0, 0 };
				for(int side = 0; side < 2; ++side)
					{
					for(unsigned w = 0; w < 0x40; ++w)
						image[side][w] = 0xcdcd8000u + w;
					image[side][0x08 / 4] = mesh.nbVerts;
					image[side][0x0c / 4] = mesh.nbTris;
					image[side][0x10 / 4] = (unsigned) (size_t) verts[side];
					image[side][0x14 / 4] = (unsigned) (size_t) mesh.tris;
					image[side][0x20 / 4] = 0;
					nxSetControl(mode ? kControlSimulate : kControlDefault);
					nx2gCall0(side == 0 ? (const void*) (base + kMeshVertexNormalsRva) : (const void*) &nxMeshComputeVertexNormals,
						&image[side][0x08 / 4]);
					nxSetControl(kControlDefault);
					unsigned mallocs, frees;
					nx2iFoundationCounts(&mallocs, &frees, &sizes[side]);
					}
				const unsigned* a = (const unsigned*) (size_t) image[0][0x20 / 4];
				const unsigned* b = (const unsigned*) (size_t) image[1][0x20 / 4];
				unsigned differing = nx2hFold(&oracleDigest, &candidateDigest, &sizes[0], &sizes[1], 1);
				differing += (a != 0) != (b != 0);
				if(a && b)
					differing += nx2hFold(&oracleDigest, &candidateDigest, a, b, 3 * mesh.nbVerts);
				// The counts the row reads, left as they were (the pointers are each side's).
				differing += nx2hFold(&oracleDigest, &candidateDigest, &image[0][0x08 / 4], &image[1][0x08 / 4], 2);
				perMode[mode] += differing;
				++runs;
				if(mode == 0 && a)
					normals += mesh.nbVerts;
				for(int side = 0; side < 2; ++side)
					{
					nx2iFoundationFree((void*) (size_t) image[side][0x20 / 4]);
					image[side][0x20 / 4] = 0;
					}
				}
			}
	total += perMode[0] + perMode[1];
	printf("collision name=mesh_vertex_normals index=- rva=0x%08x owner=phys_fn_002081 checks=%u oracle=%016llx candidate=%016llx mismatches=%u default_mismatches=%u simulate_mismatches=%u\n",
		kMeshVertexNormalsRva, oracleDigest.checks, oracleDigest.state, candidateDigest.state,
		perMode[0] + perMode[1], perMode[0], perMode[1]);
	nxPrintInput("mesh_vertex_normals", &inputDigest);
	printf("collision coverage name=mesh_vertex_normals meshes=%u variants=%u runs=%u normals=%u raw_words=%u input_snan=%u\n",
		kNb2iMeshes, kNormals2iVariants, runs, normals, rawWords, inputSnan);
	}

	// The Task 2l triangle-edge normal helper shares these lazy EdgeLists.
	total += nxDriveTask2lEdgeNormal(base, sides);
	// The Task 2l mesh/mesh overlap row consumes each side's independently built
	// OPCODE models and its own scene OBB collider state.
	total += nxDriveTask2lMeshOverlap(base, sides);
	unsigned edgeListsBuilt = 0;
	buildMismatches += nx2iCompareEdgeLists(sides[0], sides[1], &edgeListsBuilt);
	// The hulls' vertex normals 001461 built inside 001844, word for word.
	for(unsigned k = 0; k < kNb2gHulls + kNb2gPolytopeHulls; ++k)
		{
		const unsigned* a = nx2hHull(boxSides, polySides, 0, k).hull;
		const unsigned* b = nx2hHull(boxSides, polySides, 1, k).hull;
		const unsigned* na = (const unsigned*) (size_t) a[5];
		const unsigned* nb = (const unsigned*) (size_t) b[5];
		buildMismatches += (na != 0) != (nb != 0);
		for(unsigned v = 0; na && nb && v < 3 * a[3]; ++v)
			buildMismatches += na[v] != nb[v];
		}
	printf("collision build name=task_2i models=%u edge_lists=%u build_mismatches=%u\n", kNb2iMeshes, edgeListsBuilt,
		buildMismatches);
	total += buildMismatches;

	for(int side = 0; side < 2; ++side)
		{
		nx2iReleaseSide(boxSides[side], sides[side]);
		for(unsigned h = 0; h < kNb2gHulls; ++h)
			{
			Nx2gHullSide& hs = boxSides[side].hulls[h];
			nx2gFree(boxSides[side], (void*) (size_t) hs.hull[5]);
			hs.hull[5] = 0;
			nx2gReleaseHull(boxSides[side], hs);
			}
		for(unsigned h = 0; h < kNb2gPolytopeHulls; ++h)
			{
			Nx2gHullSide& hs = polySides[side].hulls[h];
			nx2gFree(polySides[side], (void*) (size_t) hs.hull[5]);
			hs.hull[5] = 0;
			nx2gReleaseHull(polySides[side], hs);
			}
		}
	nx2iFoundationEnd();
	return total;
	}

// RED-stage Task 2j differential: matrix A [BOX][MESH] and matrix B [BOX][MESH]
// against the oracle, with each image's own OPCODE model, OBBCollider, contact
// graph and mesh arrays. The boxes enclose every fixture's local bounds, which
// guarantees overlap while exercising all ten mesh shapes from Task 2i.
static unsigned nxDriveTask2j(unsigned char* base)
	{
	unsigned total = 0;
	static const unsigned kBoxMeshFixtureIndexes[6] = { 0, 1, 2, 3, 4, 6 };
	static Nx2gSide owners[2];
	static Nx2iSide sides[2];
	static Nx2iMeshWords words[kNb2iMeshes];
	static NxContactWorld worlds[2];
	static unsigned char shapeStorage[2][2][kShapeBytes];
	NxCollisionShape* shapes[2][2];
	NxDigest fixture, oracleContact, candidateContact, oracleOverlap, candidateOverlap;
	nxDigestInit(&fixture);
	nxDigestInit(&oracleContact);
	nxDigestInit(&candidateContact);
	nxDigestInit(&oracleOverlap);
	nxDigestInit(&candidateOverlap);
	unsigned state = 0x2e8b020au;
	if(!nx2iFoundationBegin())
		{
		fprintf(stderr, "FAIL task 2j: could not install the recording Foundation SDK\n");
		return 1;
		}
	for(unsigned pick = 0; pick < 6; ++pick)
		{
		const unsigned m = kBoxMeshFixtureIndexes[pick];
		const Nx2hMesh& mesh = nx2iMesh(m);
		nxFoldInput(&fixture, mesh.verts, 12 * mesh.nbVerts);
		nxFoldInput(&fixture, mesh.tris, 12 * mesh.nbTris);
		nx2iDrawMeshWords(&state, &fixture, m, words[m]);
		}
	unsigned buildFailures = 0;
	for(int side = 0; side < 2; ++side)
		{
		owners[side].oracle = side == 0;
		owners[side].base = base;
		nx2iInitSide(owners[side], sides[side], words, &buildFailures);
		}
	if(buildFailures)
		{
		fprintf(stderr, "FAIL task 2j: fixture build failures=%u\n", buildFailures);
		total += buildFailures;
		}
	typedef void(__cdecl* NxOracleContactFn)(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*);
	const NxOracleContactFn oracleA = (NxOracleContactFn) (base + 0x0003d500);
	const NxOracleOverlapFn oracleB = (NxOracleOverlapFn) (base + 0x0003bcd0);
	unsigned contacts = 0, overlapPairs = 0, overlapTrue = 0, overlapMismatches = 0, contactMismatches = 0;
	unsigned reportMismatches = 0;
	unsigned scenarioCases[3] = { 0, 0, 0 };
	for(unsigned pick = 0; pick < 6; ++pick)
		{
		const unsigned m = kBoxMeshFixtureIndexes[pick];
		const Nx2hMesh& mesh = nx2iMesh(m);
		float lo[3] = { FLT_MAX, FLT_MAX, FLT_MAX };
		float hi[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
		for(unsigned v = 0; v < mesh.nbVerts; ++v)
			for(unsigned a = 0; a < 3; ++a)
				{
				const float value = ((const float*) mesh.verts)[3 * v + a];
				if(value < lo[a]) lo[a] = value;
				if(value > hi[a]) hi[a] = value;
				}
		for(unsigned scenario = 0; scenario < 3; ++scenario)
			{
			++scenarioCases[scenario];
			nxFoldInput(&fixture, &scenario, sizeof(scenario));
		for(int side = 0; side < 2; ++side)
			{
			shapes[side][0] = (NxCollisionShape*) shapeStorage[side][0];
			shapes[side][1] = (NxCollisionShape*) shapeStorage[side][1];
			NxCollisionShape& box = *shapes[side][0];
			NxCollisionShape& meshShape = *shapes[side][1];
			nxIdentity(&box);
			nxIdentity(&meshShape);
			box.type = 1; // NX_SHAPE_BOX
			meshShape.type = 4; // NX_SHAPE_MESH
			for(unsigned a = 0; a < 3; ++a)
				{
				const float span = hi[a] - lo[a];
				const float mid = (lo[a] + hi[a]) * 0.5f;
				const float smallHalf = span > 0.0f ? fminf(0.5f, span * 0.2f) : 0.1f;
				if(scenario == 0) // small box inside the mesh bounds
					{
					box.translation[a] = mid;
					box.geometry[a + 1] = smallHalf > 0.0f ? smallHalf : 0.1f;
					}
				else if(scenario == 1) // resting against the mesh's positive-y bound
					{
					box.translation[a] = a == 1 ? hi[a] + 0.25f : mid;
					box.geometry[a + 1] = a == 1 ? 0.25f : (smallHalf > 0.0f ? smallHalf : 0.1f);
					}
				else // straddling the center of the fixture
					{
					box.translation[a] = mid;
					box.geometry[a + 1] = smallHalf > 0.0f ? smallHalf : 0.1f;
					}
				}
			Nx2iMeshSide& ms = sides[side].meshes[m];
			*(unsigned char**) (&meshShape.geometry[0]) = (unsigned char*) ms.image;
			ms.image[0x18 / 4] = (unsigned) (size_t) ms.materials;
			ms.image[0x1c / 4] = (unsigned) (size_t) ms.remap;
			ms.image[0x28 / 4] = (unsigned) (size_t) ms.model;
			ms.image[0x20 / 4] = 0;
			ms.image[0x88 / 4] = 0;
			*(unsigned*) (sides[side].context + 0x114) = 0;
			sides[side].touched[1] = 0;
			memset(sides[side].visited, 0, sizeof(sides[side].visited));
			nxResetWorld(&worlds[side]);
			nxStageWorld(&worlds[side], &box, &meshShape, true, true, 3 + m, 9 + m, false, false, false);
			}
		for(int mode = 0; mode < 2; ++mode)
			{
			for(int side = 0; side < 2; ++side)
				{
				Nx2iMeshSide& ms = sides[side].meshes[m];
				memcpy(ms.remap, words[m].remap, 4 * mesh.nbTris);
				*(unsigned*) (sides[side].context + 0x114) = 0;
				sides[side].touched[1] = 0;
				memset(sides[side].visited, 0, sizeof(sides[side].visited));
				nxResetWorld(&worlds[side]);
				nxStageWorld(&worlds[side], shapes[side][0], shapes[side][1], true, true,
					3 + m, 9 + m, false, false, false);
				if(side == 0)
					{
					nxFoldInput(&fixture, shapes[side][0]->translation, sizeof(shapes[side][0]->translation));
					nxFoldInput(&fixture, shapes[side][0]->geometry, sizeof(shapes[side][0]->geometry));
					}
				}
			unsigned reports[2][16];
			unsigned nReports[2] = { 0, 0 };
			nx2iTakeReports(reports[0], 4);
			nxFoldInput(&fixture, &mode, sizeof(mode));
			++overlapPairs;
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			const unsigned oracleOverlapResult = oracleB(shapes[0][0], shapes[0][1], sides[0].context) ? 1 : 0;
			overlapTrue += oracleOverlapResult;
			nxSetControl(kControlDefault);
			nReports[0] = nx2iTakeReports(reports[0], 4);
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			const unsigned candidateOverlapResult = NxOverlapBoxMesh(shapes[1][0], shapes[1][1], sides[1].context) ? 1 : 0;
			nxSetControl(kControlDefault);
			nReports[1] = nx2iTakeReports(reports[1], 4);
			nxDigestByte(&oracleOverlap, (unsigned char) oracleOverlapResult);
			nxDigestByte(&candidateOverlap, (unsigned char) candidateOverlapResult);
			overlapMismatches += oracleOverlapResult != candidateOverlapResult;
			reportMismatches += nReports[0] != nReports[1];
			for(unsigned r = 0; r < 4 * (nReports[0] < nReports[1] ? nReports[0] : nReports[1]); ++r)
				reportMismatches += reports[0][r] != reports[1][r];
			nx2iTakeReports(reports[0], 4);
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			oracleA(worlds[0].plane, worlds[0].sphere, &worlds[0].sink, sides[0].context);
			nxSetControl(kControlDefault);
			nReports[0] = nx2iTakeReports(reports[0], 4);
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			NxContactBoxMesh(worlds[1].plane, worlds[1].sphere, &worlds[1].sink, sides[1].context);
			nxSetControl(kControlDefault);
			nReports[1] = nx2iTakeReports(reports[1], 4);
			const unsigned differing = nxCompareStreams(&worlds[0], &worlds[1], mode);
			contactMismatches += differing;
			contacts += worlds[0].sink.contactCount;
			reportMismatches += nReports[0] != nReports[1];
			for(unsigned r = 0; r < 4 * (nReports[0] < nReports[1] ? nReports[0] : nReports[1]); ++r)
				reportMismatches += reports[0][r] != reports[1][r];
			nxFoldStream(&oracleContact, &worlds[0]);
			nxFoldStream(&candidateContact, &worlds[1]);
			}
			}
		}
	printf("collision name=overlap_box_mesh index=- rva=0x0003bcd0 owner=phys_fn_001757 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		overlapPairs, oracleOverlap.state, candidateOverlap.state, overlapMismatches);
	printf("collision name=contact_box_mesh index=- rva=0x0003d500 owner=phys_fn_001772 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		overlapPairs, oracleContact.state, candidateContact.state, contactMismatches);
	nxPrintInput("overlap_box_mesh", &fixture);
	nxPrintInput("contact_box_mesh", &fixture);
	printf("collision coverage name=overlap_box_mesh meshes=%u cases=%u true=%u false=%u\n",
		6, overlapPairs, overlapTrue, overlapPairs - overlapTrue);
	printf("collision coverage name=contact_box_mesh meshes=%u cases=%u inside=%u resting=%u straddling=%u oracle_contacts=%u\n",
		6, overlapPairs / 2, scenarioCases[0], scenarioCases[1], scenarioCases[2], contacts);
	total += overlapMismatches + contactMismatches + reportMismatches + buildFailures;
	for(int side = 0; side < 2; ++side)
		nx2iReleaseSide(owners[side], sides[side]);
	nx2iFoundationEnd();
	return total;
	}

// Task 2k capsule/mesh differential. Reuses the Task 2i mesh trees and
// side-specific LSS context, exercising inside, resting and straddling poses.
static unsigned nxDriveTask2k(unsigned char* base)
	{
	unsigned total = 0, overlapMismatch = 0, contactMismatch = 0, reportMismatch = 0;
	static const unsigned picks[6] = { 0, 1, 2, 3, 4, 6 };
	static Nx2gSide owners[2];
	static Nx2iSide sides[2];
	static Nx2iMeshWords words[kNb2iMeshes];
	static NxContactWorld worlds[2];
	static unsigned char shapeStorage[2][2][kShapeBytes];
	NxCollisionShape* shapes[2][2];
	NxDigest oracleOverlapDigest, candidateOverlapDigest, oracleContactDigest, candidateContactDigest;
	nxDigestInit(&oracleOverlapDigest); nxDigestInit(&candidateOverlapDigest);
	nxDigestInit(&oracleContactDigest); nxDigestInit(&candidateContactDigest);
	if(!nx2iFoundationBegin())
		return 1;
	unsigned buildFailures = 0, state = 0x2e8b0210u;
	NxDigest fixture;
	nxDigestInit(&fixture);
	for(unsigned i = 0; i < 6; ++i)
		nx2iDrawMeshWords(&state, &fixture, picks[i], words[picks[i]]);
	for(int side = 0; side < 2; ++side)
		{
		owners[side].oracle = side == 0;
		owners[side].base = base;
		nx2iInitSide(owners[side], sides[side], words, &buildFailures);
		}
	typedef void(__cdecl* ContactFn)(const NxCollisionShape*, const NxCollisionShape*, NxContactSink*, void*);
	typedef bool(__cdecl* OverlapFn)(const NxCollisionShape*, const NxCollisionShape*, void*);
	const ContactFn oracleContact = (ContactFn)(base + 0x0003e530);
	const OverlapFn oracleOverlap = (OverlapFn)(base + 0x0003e370);
	unsigned cases = 0, oracleContacts = 0, overlapTrue = 0;
	unsigned scenarioCases[3] = { 0, 0, 0 };
	for(unsigned pi = 0; pi < 6; ++pi)
		{
		const unsigned m = picks[pi];
		const Nx2hMesh& mesh = nx2iMesh(m);
		float lo[3] = { FLT_MAX, FLT_MAX, FLT_MAX }, hi[3] = { -FLT_MAX, -FLT_MAX, -FLT_MAX };
		for(unsigned v = 0; v < mesh.nbVerts; ++v)
			for(unsigned a = 0; a < 3; ++a)
				{
				const float x = ((const float*)mesh.verts)[3 * v + a];
				if(x < lo[a]) lo[a] = x;
				if(x > hi[a]) hi[a] = x;
				}
		for(unsigned scenario = 0; scenario < 3; ++scenario)
			for(unsigned mode = 0; mode < 2; ++mode)
				{
				++cases;
				nxFoldInput(&fixture, &scenario, sizeof(scenario));
				nxFoldInput(&fixture, &mode, sizeof(mode));
				for(int side = 0; side < 2; ++side)
					{
					shapes[side][0] = (NxCollisionShape*)shapeStorage[side][0];
					shapes[side][1] = (NxCollisionShape*)shapeStorage[side][1];
					NxCollisionShape& capsule = *shapes[side][0];
					NxCollisionShape& meshShape = *shapes[side][1];
					nxIdentity(&capsule); nxIdentity(&meshShape);
					capsule.type = 3; meshShape.type = 4;
					capsule.geometry[0] = 0.35f; capsule.geometry[1] = 0.45f;
					for(unsigned a = 0; a < 3; ++a)
						{
						const float mid = (lo[a] + hi[a]) * 0.5f;
						capsule.translation[a] = scenario == 1 && a == 1 ? hi[a] + 0.35f :
							(scenario == 2 && a == 0 ? hi[a] + 0.2f : mid);
						}
					if(side == 0)
						{
						nxFoldInput(&fixture, capsule.geometry, 2 * sizeof(float));
						nxFoldInput(&fixture, capsule.translation, sizeof(capsule.translation));
						}
					Nx2iMeshSide& ms = sides[side].meshes[m];
					*(unsigned char**)(&meshShape.geometry[0]) = (unsigned char*)ms.image;
					ms.image[0x18 / 4] = (unsigned)(size_t)ms.materials;
					ms.image[0x1c / 4] = (unsigned)(size_t)ms.remap;
					ms.image[0x28 / 4] = (unsigned)(size_t)ms.model;
					*(unsigned*)(sides[side].context + 0x290) = 0;
				*(unsigned*)(sides[side].context + 0x4e4) = 0;
					sides[side].touched[1] = 0;
					memset(sides[side].visited, 0, sizeof(sides[side].visited));
					nxResetWorld(&worlds[side]);
					nxStageWorld(&worlds[side], &capsule, &meshShape, true, true, 3 + m, 9 + m, false, false, false);
					}
			unsigned result[2] = {};
			nxSetControl(mode ? kControlSimulate : kControlDefault);
			for(int side = 0; side < 2; ++side)
				{
				memcpy(sides[side].meshes[m].remap, words[m].remap,
					4 * nx2iMesh(m).nbTris);
				*(unsigned*)(sides[side].context + 0x290) = 0;
				*(unsigned*)(sides[side].context + 0x4e4) = 0;
				if(mode == 0)
					{
					if(side == 0) result[side] = oracleOverlap(shapes[0][0], shapes[0][1], sides[0].context);
					else result[side] = NxOverlapCapsuleMesh(shapes[1][0], shapes[1][1], sides[1].context);
					}
				else
					{
					if(side == 0) oracleContact(worlds[0].plane, worlds[0].sphere, &worlds[0].sink, sides[0].context);
					else NxContactCapsuleMesh(worlds[1].plane, worlds[1].sphere, &worlds[1].sink, sides[1].context);
					}
				}
			nxSetControl(kControlDefault);
			if(mode == 0) overlapMismatch += result[0] != result[1];
			if(mode == 0)
				{
				++scenarioCases[scenario];
				overlapTrue += result[0] != 0;
				nxDigestByte(&oracleOverlapDigest, (unsigned char)result[0]);
				nxDigestByte(&candidateOverlapDigest, (unsigned char)result[1]);
				}
			else
				{
					contactMismatch += nxCompareStreams(&worlds[0], &worlds[1], 0);
					reportMismatch += worlds[0].sink.contactCount != worlds[1].sink.contactCount;
					oracleContacts += worlds[0].sink.contactCount;
					nxFoldStream(&oracleContactDigest, &worlds[0]);
					nxFoldStream(&candidateContactDigest, &worlds[1]);
				}
			}
		}
	printf("collision name=overlap_capsule_mesh index=- rva=0x0003e370 checks=%u oracle=%016llx candidate=%016llx mismatches=%u\n",
		cases / 2, oracleOverlapDigest.state, candidateOverlapDigest.state, overlapMismatch);
	printf("collision name=contact_capsule_mesh index=- rva=0x0003e530 checks=%u oracle=%016llx candidate=%016llx oracle_contacts=%u mismatches=%u\n",
		cases / 2, oracleContactDigest.state, candidateContactDigest.state, oracleContacts, contactMismatch + reportMismatch);
	printf("collision coverage name=overlap_capsule_mesh meshes=6 cases=%u true=%u false=%u\n",
		cases / 2, overlapTrue, cases / 2 - overlapTrue);
	printf("collision coverage name=contact_capsule_mesh meshes=6 cases=%u inside=%u resting=%u straddling=%u oracle_contacts=%u\n",
		cases / 2, scenarioCases[0], scenarioCases[1], scenarioCases[2], oracleContacts);
	nxPrintInput("overlap_capsule_mesh", &fixture);
	nxPrintInput("contact_capsule_mesh", &fixture);
	total = overlapMismatch + contactMismatch + reportMismatch + buildFailures;
	for(int side = 0; side < 2; ++side) nx2iReleaseSide(owners[side], sides[side]);
	nx2iFoundationEnd();
	return total;
	}


extern "C" void __stdcall nxMeshContactAccumulate(unsigned, unsigned, unsigned, const unsigned*, const float*);
extern "C" unsigned nxMeshContactCount;
extern "C" float nxMeshContactSums[3];
extern "C" float nxMeshContactNormalAndMaterial[50];
extern "C" float nxMeshContactVertices[96];
extern "C" float nxMeshContactNormals[96];

// Isolate 001872's fixed 32-contact global accumulator from its mesh/mesh
// caller so its cap, unconditional sums, and each stored record are pinned.
static unsigned nxDriveTask2lAccumulator(unsigned char* base)
	{
	typedef void (__stdcall * AccumulateFn)(unsigned, unsigned, unsigned, const unsigned*, const float*);
	AccumulateFn oracle = (AccumulateFn)(base + 0x000466e0);
	unsigned char* og = base + 0x00123d8c;
	unsigned mismatches = 0;
	NxDigest oracleDigest, candidateDigest;
	NxDigest inputDigest;
	nxDigestInit(&oracleDigest); nxDigestInit(&candidateDigest);
	nxDigestInit(&inputDigest);
	memset(og - 0x94, 0, 0x2a0);
	nxMeshContactCount = 0;
	memset(nxMeshContactSums, 0, sizeof(nxMeshContactSums));
	memset(nxMeshContactNormalAndMaterial, 0, sizeof(nxMeshContactNormalAndMaterial));
	memset(nxMeshContactVertices, 0, sizeof(nxMeshContactVertices));
	memset(nxMeshContactNormals, 0, sizeof(nxMeshContactNormals));
	for(unsigned i = 0; i < 40; ++i)
		{
		unsigned vertex[3] = { 0x3f000000u + i, 0xbf000000u - i, 0x3e800000u + 3 * i };
		float normal[3] = { (float)(i + 1) * 0.125f, (float)(i - 7) * 0.0625f, (float)(9 - (int)i) * 0.03125f };
		unsigned a0 = i * 7, a1 = i * 11, a2 = i * 13;
		nxFoldInput(&inputDigest, &a0, sizeof(a0)); nxFoldInput(&inputDigest, &a1, sizeof(a1));
		nxFoldInput(&inputDigest, &a2, sizeof(a2)); nxFoldInput(&inputDigest, vertex, sizeof(vertex));
		nxFoldInput(&inputDigest, normal, sizeof(normal));
		oracle(i * 7, i * 11, i * 13, vertex, normal);
		nxMeshContactAccumulate(i * 7, i * 11, i * 13, vertex, normal);
		}
	unsigned oracleCount = *(unsigned*)(og);
	mismatches += oracleCount != nxMeshContactCount;
	const float* oracleSums = (const float*)(og + 4);
	for(unsigned i = 0; i < 3; ++i)
		{
		mismatches += memcmp(&oracleSums[i], &nxMeshContactSums[i], sizeof(float)) != 0;
		nxFoldInput(&oracleDigest, &oracleSums[i], sizeof(float));
		nxFoldInput(&candidateDigest, &nxMeshContactSums[i], sizeof(float));
		}
	const float* ov = (const float*)(base + 0x00123f20);
	const float* on = (const float*)(base + 0x00123da0);
	const float* om = (const float*)(base + 0x00123cf8);
	for(unsigned i = 0; i < 96; ++i)
		{
		mismatches += memcmp(&ov[i], &nxMeshContactVertices[i], sizeof(float)) != 0;
		mismatches += memcmp(&on[i], &nxMeshContactNormals[i], sizeof(float)) != 0;
		nxFoldInput(&oracleDigest, &ov[i], sizeof(float)); nxFoldInput(&candidateDigest, &nxMeshContactVertices[i], sizeof(float));
		nxFoldInput(&oracleDigest, &on[i], sizeof(float)); nxFoldInput(&candidateDigest, &nxMeshContactNormals[i], sizeof(float));
		}
	for(unsigned i = 0; i < 32; ++i)
		{
		mismatches += memcmp(&om[i], &nxMeshContactNormalAndMaterial[i], sizeof(float)) != 0;
		nxFoldInput(&oracleDigest, &om[i], sizeof(float)); nxFoldInput(&candidateDigest, &nxMeshContactNormalAndMaterial[i], sizeof(float));
		}
	printf("collision name=mesh_contact_accumulator index=- rva=0x000466e0 checks=40 oracle=%016llx candidate=%016llx mismatches=%u contacts=%u\n",
		oracleDigest.state, candidateDigest.state, mismatches, oracleCount);
	printf("collision coverage name=mesh_contact_accumulator calls=40 stored=%u cap=32 sums_after_cap=40\n", oracleCount);
	nxPrintInput("mesh_contact_accumulator", &inputDigest);
	return mismatches;
	}
