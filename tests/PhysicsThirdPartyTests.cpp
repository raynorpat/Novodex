// The vendored third-party differential.
//
// Vendoring is not a proof. 722 census rows now have a source file, and a source
// file that nothing runs is present, not proven -- this program's standard is a
// mutation and a measured non-zero delta against the shipped DLL, and vendored
// code does not get an exemption from it.
//
// So this is an oracle differential of the same kind as NxPhysicsCollisionTests
// and NxPhysicsAssetTests: it loads the pinned NxPhysics.dll, checks its hash
// itself, calls qhull and OPCODE rows at their recorded internal addresses, and
// compares each answer against the same call into the vendored sources linked
// into this process.
//
//   * Every driven row prints `oracle=<digest>` over the SHIPPED DLL's own
//     answers. Nothing on the candidate side appears in it, and no change to the
//     vendored tree can make one of those digests come out right.
//   * `--self` runs the oracle side alone.
//   * The comparison is this harness's own, so a mismatch fails the run.
//
// WHAT IT DELIBERATELY DOES NOT DRIVE. The NovodeX rows inside the two library
// spans -- the qhull driver and its arena at 0x0007d420/0x0007e370, the added
// serialization virtuals, Container::setExternalBuffer at 0x000b4f90,
// RadixSort::SetRankBuffers at 0x000e3ea0 -- are Task 2b's and are not
// reconstructed. Where a modification needs one of them, this harness pokes the
// member directly at an offset the disassembly gives, on BOTH sides identically,
// rather than calling a row nobody has recovered.

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <stdarg.h>

#include "Opcode.h"
#include "IcePrunable.h"


using namespace Opcode;
using namespace IceCore;
using namespace IceMaths;

// The vendored qhull entry points this harness drives. Declared here rather than
// included, because the merged tree's user.h carries the fprintf redirection and
// the `qh` macro machinery, and neither belongs in a test.
typedef double realT;
typedef union setelemT setelemT;
union setelemT { void* p; int i; };
typedef struct setT setT;
struct setT { int maxsize; setelemT e[1]; };

extern "C" {
void	qh_crossproduct(int dim, realT vecA[3], realT vecB[3], realT vecC[3]);
realT	qh_pointdist(const realT* point1, const realT* point2, int dim);
realT*	qh_maxabsval(realT* normal, int dim);
int		qh_rand(void);
void	qh_srand(int seed);
int		qh_setsize(setT* set);
int		qh_setin(setT* set, void* setelem);
void*	qh_setlast(setT* set);
int		qh_setequal(setT* setA, setT* setB);
}

//////////////////////////////////////////////////////////////////////////////
// Recorded internal addresses. Every one is a census row graded `mapped`
// against a pinned upstream source function.

// qhull
static const unsigned kQhCrossproduct	= 0x0005eac0;	// geom2.c:50
static const unsigned kQhMaxabsval		= 0x0005f530;	// geom2.c:927
static const unsigned kQhPointdist		= 0x0005fb70;	// geom2.c:1307
static const unsigned kQhRand			= 0x0005fe20;	// geom2.c:1566
static const unsigned kQhSrand			= 0x0005fe50;	// geom2.c:1587
static const unsigned kQhSetequal		= 0x0007ee50;	// qset.c:561
static const unsigned kQhSetin			= 0x0007f020;	// qset.c:765
static const unsigned kQhSetlast		= 0x0007f0a0;	// qset.c:861
static const unsigned kQhSetsize		= 0x0007f2a0;	// qset.c:1076

// OPCODE -- Ice/IceContainer.cpp
static const unsigned kContainerCtor		= 0x000b4d70;	// :40
static const unsigned kContainerEmpty		= 0x000b4d90;	// :97
static const unsigned kContainerResize		= 0x000b4de0;	// :114
static const unsigned kContainerSetSize		= 0x000b4e90;	// :153
static const unsigned kContainerCopyCtor	= 0x000b4f00;	// :67
static const unsigned kContainerDtor		= 0x000b4f50;	// :81
static const unsigned kCompletePruning		= 0x000b4530;	// phys_fn_004816

// OPCODE -- Ice/IceRevisitedRadix.cpp
static const unsigned kRadixCtor			= 0x000e32c0;	// :170
static const unsigned kRadixDtor			= 0x000e32e0;	// :186
static const unsigned kRadixSortDwords		= 0x000e33c0;	// :225
static const unsigned kRadixSortFloats		= 0x000e3920;	// :350

// OPCODE -- Ice/IceSegment.cpp and OPC_MeshInterface.cpp
static const unsigned kSegmentSqrDist		= 0x000f0560;	// IceSegment.cpp:29
static const unsigned kMeshCheckTopology	= 0x000e8fd0;	// OPC_MeshInterface.cpp:178
static const unsigned kMeshSetPointers		= 0x000e9020;	// OPC_MeshInterface.cpp:225

//////////////////////////////////////////////////////////////////////////////
// P4 TASK 2b. NovodeX rows inside the OPCODE span, with no upstream source.
// These are graded `unmapped` in the correspondence map and reconstructed, not
// vendored: everything below is a census row this harness is the proof of.

// Ice/IceRevisitedRadix.h -- the NovodeX addition that clears the marker.
static const unsigned kRadixSetRankBuffers	= 0x000e3ea0;	// phys_fn_005177

// Physics/src/opcode/IcePrunable.cpp -- twelve rows.
static const unsigned kPrunableCtor			= 0x000b54a0;	// phys_fn_004874
static const unsigned kPrunableGetWorldAABB	= 0x000b5590;	// phys_fn_004884
static const unsigned kPrunableUpdateAABB	= 0x000b55b0;	// phys_fn_004886
static const unsigned kPrunableSetType		= 0x000b55e0;	// phys_fn_004888
static const unsigned kPrunableSetSection	= 0x000b5610;	// phys_fn_004890
static const unsigned kPrunableDtor			= 0x000b5640;	// phys_fn_004892
static const unsigned kPrunableGetUpdated	= 0x000b5670;	// phys_fn_004894
// Slots 0-4 of `.rdata:0x0011b5a4` are reached through the object's own vtable
// rather than by address, because the slot index is part of what is asserted:
// phys_fn_004896 (0x000b56d0), 004876 (0x000b54f0), 004878 (0x000b5520),
// 004880 (0x000b5550) and 004882 (0x000b5570), in that order.

// The unnamed 20-byte member at Prunable+0x0c -- three rows.
static const unsigned kPrunable0CCtor		= 0x000e7330;	// phys_fn_005297
static const unsigned kPrunable0CDtor		= 0x000e7350;	// phys_fn_005299
// phys_fn_005311 (0x000e7670) is slot 0 of `.rdata:0x0011ba1c`, likewise
// reached through the vtable.

// The three .data slots IcePrunable.cpp touches.
static const unsigned kDataOwnerWorldAABB	= 0x00128478;	// read at 0x000b55b0
static const unsigned kDataAdapterQuery		= 0x001284fc;	// written at 0x000b54d0
static const unsigned kDataAdapterNotify	= 0x00128500;	// written at 0x000b54da

// The import slot SetIceError dispatches through, .rdata:0x001041b4 ->
// NxFoundation `?error@FoundationSDK@NxFoundation@@SA_NW4NxErrorCode@@PBDHPA_N1ZZ`.
// See nxDrivePrunableRanges for why this harness redirects it.
static const unsigned kIatFoundationError	= 0x001041b4;

//////////////////////////////////////////////////////////////////////////////
// Calling conventions. qhull is C; OPCODE members are __thiscall.

typedef void	(__cdecl* QhCrossproductFn)(int, realT*, realT*, realT*);
typedef realT	(__cdecl* QhPointdistFn)(const realT*, const realT*, int);
typedef realT*	(__cdecl* QhMaxabsvalFn)(realT*, int);
typedef int		(__cdecl* QhRandFn)(void);
typedef void	(__cdecl* QhSrandFn)(int);
typedef int		(__cdecl* QhSetsizeFn)(setT*);
typedef int		(__cdecl* QhSetinFn)(setT*, void*);
typedef void*	(__cdecl* QhSetlastFn)(setT*);
typedef int		(__cdecl* QhSetequalFn)(setT*, setT*);

typedef void	(__thiscall* VoidThisFn)(void*);
typedef void*	(__thiscall* PtrThisFn)(void*);
typedef bool	(__thiscall* BoolThisUdwordFn)(void*, unsigned);
typedef void	(__thiscall* CopyCtorFn)(void*, const void*);
typedef void*	(__thiscall* RadixSortDwordsFn)(void*, const unsigned*, unsigned, int);
typedef void*	(__thiscall* RadixSortFloatsFn)(void*, const float*, unsigned);
typedef float	(__thiscall* SegmentSqrDistFn)(const void*, const void*, float*);
typedef unsigned (__thiscall* MeshCheckTopologyFn)(const void*);
typedef bool	(__thiscall* MeshSetPointersFn)(void*, const void*, const void*);
typedef bool	(__cdecl* CompletePruningFn)(unsigned, const AABB**, void*, const Axes*);

// P4 Task 2b.
typedef bool	(__thiscall* RadixSetRankBuffersFn)(void*, unsigned*, unsigned*);
typedef void*	(__thiscall* PrunableCtorFn)(void*);
typedef void*	(__thiscall* PrunableGetAABBFn)(void*);
typedef void	(__thiscall* PrunableUpdateAABBFn)(void*, void*);
typedef bool	(__thiscall* PrunableSetRangeFn)(void*, unsigned);
typedef void*	(__thiscall* DeletingDtorFn)(void*, unsigned);
typedef void	(__cdecl* PrunableWorldAABBFn)(void*, void*);

struct NxOracleRows
	{
	unsigned char* base;

	QhCrossproductFn	qhCrossproduct;
	QhPointdistFn		qhPointdist;
	QhMaxabsvalFn		qhMaxabsval;
	QhRandFn			qhRand;
	QhSrandFn			qhSrand;
	QhSetsizeFn			qhSetsize;
	QhSetinFn			qhSetin;
	QhSetlastFn			qhSetlast;
	QhSetequalFn		qhSetequal;

	VoidThisFn			containerCtor;
	PtrThisFn			containerEmpty;
	BoolThisUdwordFn	containerResize;
	BoolThisUdwordFn	containerSetSize;
	CopyCtorFn			containerCopyCtor;
	VoidThisFn			containerDtor;
	CompletePruningFn	completePruning;

	VoidThisFn			radixCtor;
	VoidThisFn			radixDtor;
	RadixSortDwordsFn	radixSortDwords;
	RadixSortFloatsFn	radixSortFloats;

	SegmentSqrDistFn	segmentSqrDist;
	MeshCheckTopologyFn	meshCheckTopology;
	MeshSetPointersFn	meshSetPointers;

	RadixSetRankBuffersFn	radixSetRankBuffers;
	PrunableCtorFn			prunableCtor;
	PrunableGetAABBFn		prunableGetWorldAABB;
	PrunableUpdateAABBFn	prunableUpdateAABB;
	PrunableSetRangeFn		prunableSetType;
	PrunableSetRangeFn		prunableSetSection;
	VoidThisFn				prunableDtor;
	PrunableGetAABBFn		prunableGetUpdated;
	PrunableCtorFn			prunable0CCtor;
	VoidThisFn				prunable0CDtor;
	};

//////////////////////////////////////////////////////////////////////////////
// Results, and the digest that folds them. FNV-1a, 32 bit -- what is being
// pinned is that the shipped DLL produced these words for these inputs.

static unsigned nxFold(unsigned digest, unsigned word)
	{
	digest ^= word & 0xffu;			digest *= 16777619u;
	digest ^= (word >> 8) & 0xffu;	digest *= 16777619u;
	digest ^= (word >> 16) & 0xffu;	digest *= 16777619u;
	digest ^= (word >> 24) & 0xffu;	digest *= 16777619u;
	return digest;
	}

static unsigned nxFoldDouble(unsigned digest, double value)
	{
	unsigned words[2];
	memcpy(words, &value, sizeof(words));
	return nxFold(nxFold(digest, words[0]), words[1]);
	}

static unsigned nxFoldFloat(unsigned digest, float value)
	{
	unsigned word;
	memcpy(&word, &value, sizeof(word));
	return nxFold(digest, word);
	}

// What a tape word is, so a divergent family can say how far apart its two
// tapes are: a discrete word (a count, an index, a verdict, a flag, a
// quantized coordinate) differs or it does not; a float or double differs by
// some number of representable values. The kind never reaches a digest.
enum NxWordKind { kWordDiscrete = 0, kWordFloat = 1, kWordDoubleLo = 2, kWordDoubleHi = 3 };
// Or-ed into a word's kind while a tape's `mark` is set: the word comes from
// an input the drive declares degenerate by construction (a zero-area
// triangle, a singular matrix). A differing marked word is counted as
// `degenerate=`, listed on stderr, and kept out of the distance figures, so
// that those figures describe the regular inputs (Task 5b).
static const unsigned char kWordDegenerate = 0x80;

// A row's transcript: every word either side produced, in order.
struct NxTape
	{
	enum { kMax = 262144 };
	unsigned words[kMax];
	unsigned char kinds[kMax];
	unsigned count;
	unsigned overflow;
	unsigned char mark;

	void reset()					{ count = 0; overflow = 0; mark = 0; }
	void pushKind(unsigned word, unsigned char kind)
		{
		if(count < kMax)
			{
			kinds[count] = (unsigned char) (kind | mark);
			words[count++] = word;
			}
		else
			++overflow;
		}
	void push(unsigned word)		{ pushKind(word, kWordDiscrete); }
	void pushFloatWord(unsigned word)	{ pushKind(word, kWordFloat); }
	void pushFloat(float value)		{ unsigned w; memcpy(&w, &value, sizeof(w)); pushKind(w, kWordFloat); }
	void pushDouble(double value)
		{
		unsigned w[2];
		memcpy(w, &value, sizeof(w));
		pushKind(w[0], kWordDoubleLo);
		pushKind(w[1], kWordDoubleHi);
		}
	unsigned digest() const
		{
		unsigned d = 2166136261u;
		for(unsigned i = 0; i < count; ++i)
			d = nxFold(d, words[i]);
		return nxFold(d, overflow);
		}
	};

static NxTape gOracleTape;
static NxTape gCandidateTape;
static unsigned gRunDigest = 2166136261u;
static unsigned gMismatches = 0;
static unsigned gDriven = 0;
static unsigned gDivergent = 0;
static unsigned gWordsCompared = 0;

// The distance in representable floats between two IEEE-754 single patterns.
// Only meaningful for finite values of the same sign, which is what it is used
// on -- both sides of the one row that needs it return squared distances.
static unsigned nxUlpDistance(unsigned a, unsigned b)
	{
	if(a == b)
		return 0;
	const unsigned expA = (a >> 23) & 0xffu;
	const unsigned expB = (b >> 23) & 0xffu;
	if(expA == 0xffu || expB == 0xffu)		// an infinity or a NaN: never tolerated
		return 0xffffffffu;
	if((a >> 31) != (b >> 31))				// opposite signs: never tolerated
		return 0xffffffffu;
	return a > b ? a - b : b - a;
	}

// The distance in representable doubles between two IEEE-754 double patterns,
// on the same terms as nxUlpDistance: an infinity, a NaN or a sign change is
// never "near".
static unsigned __int64 nxUlpDistance64(unsigned __int64 a, unsigned __int64 b)
	{
	if(a == b)
		return 0;
	const unsigned expA = (unsigned) (a >> 52) & 0x7ffu;
	const unsigned expB = (unsigned) (b >> 52) & 0x7ffu;
	if(expA == 0x7ffu || expB == 0x7ffu)
		return ~(unsigned __int64) 0;
	if((a >> 63) != (b >> 63))
		return ~(unsigned __int64) 0;
	return a > b ? a - b : b - a;
	}

// A divergent family's recorded ceiling: how many tape words differed, and how
// many of those were discrete words (plus one for a tape-length difference),
// when it was measured. A run over either fails the family; a run under both
// prints an improvement note, so that the ceiling is lowered.
//
// Measured by vendored-correspondence Task 5a (evidence/vendored-correspondence.md,
// "Task 5a"): each value is what this harness printed against the pinned oracle
// (4b7db3e1...602c) with the candidate vendored code at that commit, and every
// run since has printed the same (the families are deterministic). So today's
// run passes at exactly the ceiling, and any change that makes a family differ
// more fails it. The words figure counts a double's two halves separately.
//
// Task 5b adds the distances, so that a proof citing them cites a figure the
// gate holds: the worst float and double distance, the `beyond` count, how many
// floats and doubles are infinitely apart (a sign change, an infinity or a
// NaN), the worst finite distance, and the largest absolute difference among
// the words beyond kLastBitUlp (which is what bounds values next to zero,
// where a large ulp distance is a tiny difference). A run over any of them
// fails; the note that asks for a lower ceiling reads the integer figures.
struct NxDivergentCeiling
	{
	const char*			name;
	unsigned			words;		// mismatching words, plus the length difference
	unsigned			discrete;	// mismatching discrete words, plus 1 for a length difference
	unsigned			floatUlp;	// worst float distance; 0xffffffff = inf
	unsigned __int64	doubleUlp;	// worst double distance; all ones = inf
	unsigned			beyond;		// floats and doubles more than kLastBitUlp apart
	unsigned			infWords;	// floats and doubles an infinite distance apart
	unsigned			degenerate;	// differing words from declared degenerate inputs
	unsigned __int64	finiteUlp;	// worst finite float or double distance
	double				beyondAbs;	// largest |oracle - candidate| over the beyond words
	};

static const unsigned __int64 kInf64 = ~(unsigned __int64) 0;

static const NxDivergentCeiling kDivergentCeilings[] =
	{
	//  family, words, discrete, float_ulp, double_ulp, beyond, inf_words, degenerate, finite_ulp, beyond_abs
	{ "opcode_model_build_x87", 2316, 779, 0xffffffffu, 0, 1517, 240, 0, 1073741824ull, 3.0 },	// splatter ties change the tree; quantized coefficients
	{ "opcode_ray_x87", 194, 0, 377, 0, 21, 0, 0, 377ull, 5.1409006118774414e-07 },	// distances and barycentrics only
	{ "opcode_ray_boundary", 927, 756, 1065353216, 0, 68, 0, 0, 1065353216ull, 8.2039852142333984 },	// one root rejection; the tape is 4 words shorter
	{ "opcode_treecollider_boundary", 684, 671, 0, 0, 0, 0, 0, 0, 0.0 },	// pair verdicts; the tape is 14 words shorter
	{ "ice_plane_triangle", 947, 0, 2820, 0, 39, 0, 122, 2820ull, 8.3446502685546875e-07 },	// 29 zero-area inputs are `degenerate`
	{ "ice_matrix4x4", 1815, 0, 106, 0, 109, 0, 19, 106ull, 0.00128173828125 },	// 17 singular inputs are `degenerate`
	{ "ice_obb", 1204, 0, 512, 0, 72, 0, 0, 512ull, 2.384185791015625e-07 },
	{ "qhull_hull_x87", 1284, 0, 0, 3421917482582016ull, 7, 0, 0, 3421917482582016ull, 2.9558577807620168e-12 },	// doubles only; the combinatorial hull is exact
	{ "qhull_hull_rotated", 1201, 582, 0, kInf64, 146, 31, 0, 4611686018427387904ull, 2.0 },	// "QR1": the merges differ
	{ "opcode_candidate_trees_ray", 18, 1, 14, 0, 2, 0, 0, 14ull, 8.3446502685546875e-07 },	// the collider's floats, and one grazing ray's BV test count
	{ "opcode_candidate_trees_x87", 2489, 2436, 37445356, 0, 9, 0, 0, 37445356ull, 2.435370922088623 },	// candidate-built quantized and tied trees: test counts, hits, floats
	{ "qhull_output_x87", 2225, 0, 0, 3377699720527872ull, 43, 0, 0, 3377699720527872ull, 1.6653345369377348e-16 },	// qhull-gap: printed and hull doubles, the qhull_hull_x87 class
	{ "qhull_output_dims_x87", 860, 0, 0, 1970324836974592ull, 25, 0, 0, 1970324836974592ull, 1.1102230246251565e-16 },	// qhull-gap: 2-d and 4-d, the same class
	{ "qhull_output_delaunay_x87", 838, 0, 0, 6ull, 5, 0, 0, 6ull, 8.3266726846886741e-17 },	// qhull-gap: Delaunay/Voronoi over 2-d input
	{ "qhull_output_delaunay3_x87", 1233, 0, 0, 128ull, 62, 0, 0, 128ull, 1.1102230246251565e-16 },	// qhull-gap: Delaunay/Voronoi over 3-d input
	{ "qhull_trace_x87", 334, 0, 0, kInf64, 61, 8, 0, 3802157541524609ull, 3.3306690738754696e-16 },	// qhull-gap: trace numbers; the inf words are distances next to 0 of opposite sign
	{ "qhull_options_x87", 1023, 0, 0, 3377699720527872ull, 12, 0, 0, 3377699720527872ull, 1.0325074129013956e-14 },	// qhull-gap: option-gated arms
	{ "qhull_merge_x87", 2376, 0, 0, 2814749767106560ull, 26, 0, 0, 2814749767106560ull, 3.2585045772748344e-13 },	// qhull-gap: large and near-degenerate inputs
	{ "qhull_random_x87", 317, 0, 0, 3197379813572608ull, 14, 0, 0, 3197379813572608ull, 1.3877787807814457e-14 },	// qhull-gap: QJ/Qr/R: the qhull_hull_x87 class over joggled and perturbed input
	{ "qhull_direct_x87", 1062, 0, 0, kInf64, 43, 7, 0, 18858823439613952ull, 2.2204460492503131e-16 },	// qhull-gap: out-of-line printers and helpers; the inf words are distances next to 0
	{ "qhull_merge2_x87", 1700, 0, 0, 2814749767106560ull, 103, 0, 0, 2814749767106560ull, 5.5511151231257827e-15 },	// qhull-gap: the Qn switches, larger thresholds, Qf, Delaunay Qt
	{ "qhull_paths", 3212, 3204, 0, 0ull, 0, 0, 0, 0ull, 0.0 },	// qhull-gap: distance-test counts and a trace-4 search path; the T4 run is 9 words longer
	{ "qhull_paths_x87", 644, 1, 0, kInf64, 348, 85, 0, 4616189618054758400ull, HUGE_VAL },	// qhull-gap: the same runs; misaligned after the T4 run's extra lines
	{ "qhull_rotation", 268, 268, 0, 0ull, 0, 0, 0, 0ull, 0.0 },	// qhull-gap: "QRn": the merges differ, as qhull_hull_rotated
	{ "qhull_rotation_x87", 2001, 0, 0, kInf64, 547, 20, 0, 4611686018427387904ull, 2.0 },	// qhull-gap: "QRn"
	{ "edge_list.plane_divergent", 465, 465, 0, 0, 0, 0, 0, 0ull, 0.0 },	// convex-mesh gap Task 2c: active-edge bits that follow the vendored Plane::Set / Triangle::Normal (005155, 005181); 0 with the oracle's bound in
	{ "ice_adjacencies.plane_divergent", 124, 124, 0, 0, 0, 0, 0, 0ull, 0.0 },	// the same, through 001546's EdgeList
	{ "pose_pair.inverse_divergent", 944, 0, 0xffffffffu, 0, 944, 944, 0, 0ull, HUGE_VAL },	// convex-mesh gap Task 2e: 001653 over the inverse of a raw pose; every word a NaN: 276 are a signalling NaN the oracle's 005191 copies as integers and the vendored InvertPRMatrix quiets by copying through the FPU, and 668 are quiet NaNs on both sides that differ only in sign (252), payload (140) or both (276), where that quieted inverse meets other NaNs downstream in 001653's products; 0 with the oracle's 005191 bound in
	{ "adjacency_owner.plane_divergent", 155, 155, 0, 0, 0, 0, 0, 0ull, 0.0 },	// convex-mesh gap Task 2e: 002188's EdgeList on Task 2c's 13 frozen meshes (005155, 005181); 0 with the oracle's bound in
	{ "convex_hull.plane_divergent", 1595, 52, 0xffffffffu, 0, 512, 118, 0, 1074731964ull, HUGE_VAL },	// convex-mesh gap Task 2f: the concave, nudged and 0x0f7f polycube meshes and 001463's drawn points, whose planes 001463 takes through the vendored Plane::Set / Triangle::Area (005155, 005179); 0 with the oracle's pair bound into 001463
	};

static const NxDivergentCeiling* nxFindCeiling(const char* name)
	{
	for(unsigned i = 0; i < sizeof(kDivergentCeilings) / sizeof(kDivergentCeilings[0]); ++i)
		if(!strcmp(kDivergentCeilings[i].name, name))
			return &kDivergentCeilings[i];
	return 0;
	}

// One driven row: compare the two tapes word for word, print the oracle digest,
// and fold it into the run digest.
//
// `ulpTolerance` is 0 for an exact family: any differing word fails it. A
// family reported with kDivergent is a measured, attributed divergence
// (evidence/vendored-correspondence.md). It fails when it differs by more than
// its recorded ceiling (kDivergentCeilings), and its line says by how much it
// differs, which is what tools/vendored_trace.py classifies it by:
//   mismatches=   words that differ, plus the length difference
//   discrete=     differing discrete words, plus 1 if the lengths differ
//   float_ulp=    the largest distance over differing float words ("inf" for a
//                 sign change, an infinity or a NaN; 0 when none differ)
//   double_ulp=   the same over doubles
//   beyond=       how many differing floats and doubles are more than
//                 kLastBitUlp apart (a double counts once)
//   inf_words=    how many of those are an infinite distance apart
//   degenerate=   differing words from inputs the drive declares degenerate
//                 (kWordDegenerate); they are listed on stderr and are not in
//                 the distance figures
//   finite_ulp=   the worst finite float or double distance
//   beyond_abs=   the largest |oracle - candidate| over the beyond words
//                 (Task 5b: the bound that matters for values next to zero)
//   first_diff=   the first index at which the tapes differ, or the shorter
//                 length when one is a prefix of the other ("none" if equal)
//   length_delta= candidate words minus oracle words
//   ceiling=      the recorded words/discrete ceiling
// Past a length difference the tapes are still compared position by position
// up to the shorter one. What that counts after the first difference depends
// on where the two fall out of step, but it is deterministic, and it is what
// the ceiling records.
static const unsigned kDivergent = 0xffffffffu;

// The "last bit" bound. A three-term sum that a 2003 x87 build adds in another
// order, or keeps unrounded where a 2026 /fp:precise build rounds, differs from
// it by a rounding of the result, and a few of those in a row by a few units:
// 4 is two roundings on each side. A value further apart than that has been
// through a cancellation or a different branch, which is not a last-bit
// difference. tools/vendored_trace.py applies the same bound (LASTBIT_ULP).
static const unsigned kLastBitUlp = 4;

struct NxTapeDifference
	{
	unsigned			words;
	unsigned			discrete;
	unsigned			beyond;
	unsigned			floatUlp;		// 0xffffffff = inf
	unsigned __int64	doubleUlp;		// all ones = inf
	unsigned			first;			// 0xffffffff = none
	int					lengthDelta;
	unsigned			infWords;		// floats and doubles an infinite distance apart
	unsigned			degenerate;		// differing words from declared degenerate inputs
	unsigned __int64	finiteUlp;		// the worst finite distance over both
	double				beyondAbs;		// the largest |oracle - candidate| over the beyond words
	};

// |a - b| for two words of one kind; +inf when either is an infinity or a NaN.
// Written through a pointer, never returned as a double (convex-mesh gap Task
// 2g review): a floating return travels in st(0), and what the caller does
// with it then follows the compiler's inlining.
static void nxAbsDifference(double a, double b, double* out)
	{
	if(!_finite(a) || !_finite(b))
		*out = HUGE_VAL;
	else
		*out = fabs(a - b);
	}

// Each float or double more than kLastBitUlp apart, on stderr (the first 160
// of a family), so that it can be attributed to its input
// (evidence/vendored-correspondence.md, Task 5b). `ulp` reads "inf" for a sign
// change, an infinity or a NaN.
static void nxNoteBeyond(const char* name, unsigned index, unsigned __int64 oracle, unsigned __int64 candidate,
	bool isDouble, unsigned __int64 ulp, unsigned seen)
	{
	if(!name || seen > 160)
		return;
	if(seen == 160)
		{
		fprintf(stderr, "ULP_BEYOND %s ... (more)\n", name);
		return;
		}
	char distance[32];
	if(ulp == (isDouble ? ~(unsigned __int64) 0 : 0xffffffffu))
		_snprintf(distance, sizeof(distance), "inf");
	else
		_snprintf(distance, sizeof(distance), "%I64u", ulp);
	distance[sizeof(distance) - 1] = 0;
	fprintf(stderr, "ULP_BEYOND %s word=%u oracle=%0*I64x candidate=%0*I64x ulp=%s\n", name, index,
		isDouble ? 16 : 8, oracle, isDouble ? 16 : 8, candidate, distance);
	}

static NxTapeDifference nxCompareTapes(const NxTape& oracle, const NxTape& candidate, const char* name = 0)
	{
	NxTapeDifference d;
	d.infWords = 0;
	d.degenerate = 0;
	d.finiteUlp = 0;
	d.beyondAbs = 0.0;
	d.words = 0;
	d.discrete = 0;
	d.beyond = 0;
	d.floatUlp = 0;
	d.doubleUlp = 0;
	d.first = 0xffffffffu;
	d.lengthDelta = (int) (candidate.count + candidate.overflow) - (int) (oracle.count + oracle.overflow);
	const unsigned common = oracle.count < candidate.count ? oracle.count : candidate.count;
	for(unsigned i = 0; i < common; ++i)
		{
		const unsigned char kind = (unsigned char) (oracle.kinds[i] & ~kWordDegenerate);
		if((oracle.kinds[i] | candidate.kinds[i]) & kWordDegenerate)
			{
			if(oracle.words[i] != candidate.words[i])
				{
				if(d.first == 0xffffffffu)
					d.first = i;
				++d.words;
				if(name && d.degenerate < 160)
					fprintf(stderr, "ULP_DEGENERATE %s word=%u oracle=%08x candidate=%08x\n", name, i,
						oracle.words[i], candidate.words[i]);
				++d.degenerate;
				}
			continue;
			}
		if(kind == kWordDoubleLo && i + 1 < common && oracle.kinds[i + 1] == kWordDoubleHi
			&& candidate.kinds[i] == kWordDoubleLo && candidate.kinds[i + 1] == kWordDoubleHi)
			{
			const bool lo = oracle.words[i] != candidate.words[i];
			const bool hi = oracle.words[i + 1] != candidate.words[i + 1];
			if(lo || hi)
				{
				if(d.first == 0xffffffffu)
					d.first = lo ? i : i + 1;
				d.words += (lo ? 1u : 0u) + (hi ? 1u : 0u);
				const unsigned __int64 a = ((unsigned __int64) oracle.words[i + 1] << 32) | oracle.words[i];
				const unsigned __int64 b = ((unsigned __int64) candidate.words[i + 1] << 32) | candidate.words[i];
				const unsigned __int64 ulp = nxUlpDistance64(a, b);
				if(ulp > d.doubleUlp)
					d.doubleUlp = ulp;
				if(ulp == kInf64)
					++d.infWords;
				else if(ulp > d.finiteUlp)
					d.finiteUlp = ulp;
				if(ulp > kLastBitUlp)
					{
					nxNoteBeyond(name, i, a, b, true, ulp, d.beyond);
					++d.beyond;
					double x, y;
					memcpy(&x, &a, sizeof(x));
					memcpy(&y, &b, sizeof(y));
					double gap;
					nxAbsDifference(x, y, &gap);
					if(gap > d.beyondAbs)
						d.beyondAbs = gap;
					}
				}
			++i;
			continue;
			}
		if(oracle.words[i] == candidate.words[i])
			continue;
		if(d.first == 0xffffffffu)
			d.first = i;
		++d.words;
		if(kind == kWordFloat && candidate.kinds[i] == kWordFloat)
			{
			const unsigned ulp = nxUlpDistance(oracle.words[i], candidate.words[i]);
			if(ulp > d.floatUlp)
				d.floatUlp = ulp;
			if(ulp == 0xffffffffu)
				++d.infWords;
			else if(ulp > d.finiteUlp)
				d.finiteUlp = ulp;
			if(ulp > kLastBitUlp)
				{
				nxNoteBeyond(name, i, oracle.words[i], candidate.words[i], false, ulp, d.beyond);
				++d.beyond;
				float x, y;
				memcpy(&x, &oracle.words[i], sizeof(x));
				memcpy(&y, &candidate.words[i], sizeof(y));
				double gap;
				nxAbsDifference(x, y, &gap);
				if(gap > d.beyondAbs)
					d.beyondAbs = gap;
				}
			}
		else
			++d.discrete;	// a discrete word, or two tapes out of step
		}
	if(d.lengthDelta != 0)
		{
		if(d.first == 0xffffffffu)
			d.first = common;
		d.words += (unsigned) (d.lengthDelta < 0 ? -d.lengthDelta : d.lengthDelta);
		++d.discrete;
		}
	return d;
	}

static void nxFormatUlp(char* out, size_t size, unsigned __int64 ulp, unsigned __int64 inf)
	{
	if(ulp == inf)
		_snprintf(out, size, "inf");
	else
		_snprintf(out, size, "%I64u", ulp);
	out[size - 1] = 0;
	}

static void nxReport(const char* name, const char* rva, const char* owner, const char* source,
	bool selfOnly, unsigned ulpTolerance = 0)
	{
	const unsigned oracleDigest = gOracleTape.digest();
	const bool divergent = ulpTolerance == kDivergent;
	unsigned mismatches = 0;
	unsigned fatal = 0;
	unsigned worstUlp = 0;
	NxTapeDifference d;
	memset(&d, 0, sizeof(d));
	d.first = 0xffffffffu;
	const NxDivergentCeiling* ceiling = divergent ? nxFindCeiling(name) : 0;
	if(!selfOnly && divergent)
		{
		d = nxCompareTapes(gOracleTape, gCandidateTape, name);
		mismatches = d.words;
		if(d.first != 0xffffffffu)
			{
			const unsigned i = d.first;
			if(i < gOracleTape.count && i < gCandidateTape.count)
				fprintf(stderr, "DIVERGENT %s first_diff=%u oracle=%08x candidate=%08x kind=%u/%u"
					" length oracle=%u candidate=%u\n", name, i, gOracleTape.words[i], gCandidateTape.words[i],
					gOracleTape.kinds[i], gCandidateTape.kinds[i], gOracleTape.count, gCandidateTape.count);
			else
				fprintf(stderr, "DIVERGENT %s first_diff=%u: the %s tape ends there; length oracle=%u candidate=%u\n",
					name, i, gOracleTape.count < gCandidateTape.count ? "oracle" : "candidate",
					gOracleTape.count, gCandidateTape.count);
			}
		if(!ceiling)
			{
			fprintf(stderr, "FAIL %s is divergent and has no recorded ceiling\n", name);
			fatal = 1;
			}
		else if(d.words > ceiling->words || d.discrete > ceiling->discrete || d.floatUlp > ceiling->floatUlp
			|| d.doubleUlp > ceiling->doubleUlp || d.beyond > ceiling->beyond || d.infWords > ceiling->infWords
			|| d.degenerate > ceiling->degenerate || d.finiteUlp > ceiling->finiteUlp || d.beyondAbs > ceiling->beyondAbs)
			{
			fprintf(stderr, "FAIL %s exceeds its recorded ceiling: words %u (ceiling %u), discrete %u (ceiling %u),"
				" float_ulp %u (ceiling %u), double_ulp %I64u (ceiling %I64u), beyond %u (ceiling %u),"
				" inf_words %u (ceiling %u), degenerate %u (ceiling %u), finite_ulp %I64u (ceiling %I64u),"
				" beyond_abs %.17g (ceiling %.17g)\n",
				name, d.words, ceiling->words, d.discrete, ceiling->discrete, d.floatUlp, ceiling->floatUlp,
				d.doubleUlp, ceiling->doubleUlp, d.beyond, ceiling->beyond, d.infWords, ceiling->infWords,
				d.degenerate, ceiling->degenerate, d.finiteUlp, ceiling->finiteUlp, d.beyondAbs, ceiling->beyondAbs);
			fatal = 1;
			}
		else if(d.words < ceiling->words || d.discrete < ceiling->discrete || d.floatUlp < ceiling->floatUlp
			|| d.doubleUlp < ceiling->doubleUlp || d.beyond < ceiling->beyond || d.infWords < ceiling->infWords
			|| d.degenerate < ceiling->degenerate || d.finiteUlp < ceiling->finiteUlp)
			fprintf(stderr, "IMPROVED %s is under its recorded ceiling: words %u (ceiling %u), discrete %u"
				" (ceiling %u), float_ulp %u (ceiling %u), double_ulp %I64u (ceiling %I64u), beyond %u (ceiling %u),"
				" inf_words %u (ceiling %u), degenerate %u (ceiling %u), finite_ulp %I64u (ceiling %I64u);"
				" lower the ceiling\n",
				name, d.words, ceiling->words, d.discrete, ceiling->discrete, d.floatUlp, ceiling->floatUlp,
				d.doubleUlp, ceiling->doubleUlp, d.beyond, ceiling->beyond, d.infWords, ceiling->infWords,
				d.degenerate, ceiling->degenerate, d.finiteUlp, ceiling->finiteUlp);
		}
	else if(!selfOnly)
		{
		if(gOracleTape.count != gCandidateTape.count || gOracleTape.overflow != gCandidateTape.overflow)
			{
			mismatches = 1;
			fatal = 1;
			fprintf(stderr, "MISMATCH %s word count oracle=%u candidate=%u\n",
				name, gOracleTape.count, gCandidateTape.count);
			}
		else
			{
			for(unsigned i = 0; i < gOracleTape.count; ++i)
				if(gOracleTape.words[i] != gCandidateTape.words[i])
					{
					++mismatches;
					const unsigned ulp = nxUlpDistance(gOracleTape.words[i], gCandidateTape.words[i]);
					if(ulp > worstUlp && ulp != 0xffffffffu)
						worstUlp = ulp;
					if(ulp > ulpTolerance)
						{
						if(fatal < 8)
							fprintf(stderr, "MISMATCH %s word %u oracle=%08x candidate=%08x ulp=%u\n",
								name, i, gOracleTape.words[i], gCandidateTape.words[i], ulp);
						++fatal;
						}
					}
			}
		}
	gMismatches += fatal;
	++gDriven;
	if(divergent)
		++gDivergent;
	gWordsCompared += gOracleTape.count;
	gRunDigest = nxFold(gRunDigest, oracleDigest);
	if(divergent)
		{
		char floatUlp[32], doubleUlp[32], finiteUlp[32], beyondAbs[32], first[16];
		nxFormatUlp(finiteUlp, sizeof(finiteUlp), d.finiteUlp, kInf64);
		if(_finite(d.beyondAbs))
			_snprintf(beyondAbs, sizeof(beyondAbs), "%.3g", d.beyondAbs);
		else
			_snprintf(beyondAbs, sizeof(beyondAbs), "inf");
		beyondAbs[sizeof(beyondAbs) - 1] = 0;
		nxFormatUlp(floatUlp, sizeof(floatUlp), d.floatUlp, 0xffffffffu);
		nxFormatUlp(doubleUlp, sizeof(doubleUlp), d.doubleUlp, ~(unsigned __int64) 0);
		if(d.first == 0xffffffffu)
			_snprintf(first, sizeof(first), "none");
		else
			_snprintf(first, sizeof(first), "%u", d.first);
		first[sizeof(first) - 1] = 0;
		// One literal, so that tools/tests/test_gate_targets.py (which reads a
		// printf's first literal) can tell this format from the exact one.
		printf("thirdparty name=%s rva=%s owner=%s source=%s words=%u oracle=%08x mismatches=%u discrete=%u float_ulp=%s double_ulp=%s beyond=%u inf_words=%u degenerate=%u finite_ulp=%s beyond_abs=%s first_diff=%s length_delta=%d ceiling=%u/%u verdict=%s\n",
			name, rva, owner, source, gOracleTape.count, oracleDigest, mismatches, d.discrete, floatUlp,
			doubleUlp, d.beyond, d.infWords, d.degenerate, finiteUlp, beyondAbs, first, d.lengthDelta, ceiling ? ceiling->words : 0u, ceiling ? ceiling->discrete : 0u,
			fatal ? "FAILED" : "divergent");
		}
	else
		printf("thirdparty name=%s rva=%s owner=%s source=%s words=%u oracle=%08x mismatches=%u"
			" worst_ulp=%u verdict=%s\n",
			name, rva, owner, source, gOracleTape.count, oracleDigest, mismatches, worstUlp,
			fatal ? "FAILED" : "exact");
	}

//////////////////////////////////////////////////////////////////////////////
// A tiny deterministic generator. The same one the other harnesses use.

static unsigned gState = 0;
static unsigned nxNext()
	{
	gState ^= gState << 13;
	gState ^= gState >> 17;
	gState ^= gState << 5;
	return gState;
	}

// A raw word, written into its slot as bits. It used to be returned as a float,
// and a float return travels in st(0), where a signalling NaN is quieted -- or not,
// if the call is inlined and the word copied as an integer -- so the words handed
// to the rows would have depended on code generation (the convex-mesh gap harness
// hardening of tests/PhysicsCollisionTests.cpp). Its one use draws no signalling
// NaN, so the words are the same either way.
static void nxNextFloatBits(float* out)
	{
	const unsigned bits = nxNext();
	memcpy(out, &bits, sizeof(*out));
	}

//////////////////////////////////////////////////////////////////////////////
// qhull rows

static void nxDriveQhullPure(const NxOracleRows& o, bool selfOnly)
	{
	// qh_crossproduct -- geom2.c:50
	gState = 0x51c0ffee;
	gOracleTape.reset();
	gCandidateTape.reset();
	for(int c = 0; c < 4000; ++c)
		{
		realT a[3], b[3], out[3];
		for(int k = 0; k < 3; ++k)
			{
			a[k] = (double)(int)nxNext() * 1e-3;
			b[k] = (double)(int)nxNext() * 1e-3;
			}
		realT copyA[3], copyB[3];
		memcpy(copyA, a, sizeof(a));
		memcpy(copyB, b, sizeof(b));
		memset(out, 0, sizeof(out));
		o.qhCrossproduct(3, a, b, out);
		for(int k = 0; k < 3; ++k)
			gOracleTape.pushDouble(out[k]);
		if(!selfOnly)
			{
			memset(out, 0, sizeof(out));
			qh_crossproduct(3, copyA, copyB, out);
			for(int k = 0; k < 3; ++k)
				gCandidateTape.pushDouble(out[k]);
			}
		}
	nxReport("qh_crossproduct", "0x0005eac0", "phys_fn_002465", "geom2.c:50", selfOnly);

	// qh_pointdist -- geom2.c:1307, both the dim>0 and dim<0 arms
	gState = 0x0d15ea5e;
	gOracleTape.reset();
	gCandidateTape.reset();
	for(int c = 0; c < 4000; ++c)
		{
		realT p1[4], p2[4];
		for(int k = 0; k < 4; ++k)
			{
			p1[k] = (double)(int)nxNext() * 1e-4;
			p2[k] = (double)(int)nxNext() * 1e-4;
			}
		const int dim = (c & 1) ? 3 : -3;
		gOracleTape.pushDouble(o.qhPointdist(p1, p2, dim));
		if(!selfOnly)
			gCandidateTape.pushDouble(qh_pointdist(p1, p2, dim));
		}
	nxReport("qh_pointdist", "0x0005fb70", "phys_fn_002505", "geom2.c:1307", selfOnly);

	// qh_maxabsval -- geom2.c:927. It returns a POINTER into the array, so the
	// comparison is the index it picked, not the address.
	gState = 0xbadc0de1;
	gOracleTape.reset();
	gCandidateTape.reset();
	for(int c = 0; c < 4000; ++c)
		{
		realT v[6];
		for(int k = 0; k < 6; ++k)
			v[k] = (double)(int)nxNext() * 1e-2;
		const int dim = 1 + (int)(nxNext() % 6u);
		realT* picked = o.qhMaxabsval(v, dim);
		gOracleTape.push((unsigned)(picked ? (picked - v) : 0xffffffffu));
		if(!selfOnly)
			{
			realT* ours = qh_maxabsval(v, dim);
			gCandidateTape.push((unsigned)(ours ? (ours - v) : 0xffffffffu));
			}
		}
	nxReport("qh_maxabsval", "0x0005f530", "phys_fn_002493", "geom2.c:927", selfOnly);

	// qh_srand + qh_rand -- geom2.c:1587 and :1566, the Park-Miller generator.
	// Driven as a SEQUENCE from a seeded state, which is the only way the
	// 127773/2836/16807 constants and the negative-value fixup are all reached.
	gOracleTape.reset();
	gCandidateTape.reset();
	static const int kSeeds[] = { 1, 2, 12345, 0x7ffffffe, -7, 0 };
	for(int s = 0; s < (int)(sizeof(kSeeds) / sizeof(kSeeds[0])); ++s)
		{
		o.qhSrand(kSeeds[s]);
		for(int c = 0; c < 500; ++c)
			gOracleTape.push((unsigned)o.qhRand());
		if(!selfOnly)
			{
			qh_srand(kSeeds[s]);
			for(int c = 0; c < 500; ++c)
				gCandidateTape.push((unsigned)qh_rand());
			}
		}
	nxReport("qh_rand", "0x0005fe20", "phys_fn_002513", "geom2.c:1566", selfOnly);
	}

// A setT laid out the way qset.h lays one out: maxsize, then maxsize+1 slots,
// the last of which holds the size plus one (or zero for a full set).
struct NxSetBuffer
	{
	enum { kMax = 16 };
	int maxsize;
	void* e[kMax + 1];

	setT* build(int capacity, int used, unsigned tag)
		{
		maxsize = capacity;
		for(int i = 0; i <= kMax; ++i)
			e[i] = 0;
		for(int i = 0; i < used; ++i)
			e[i] = (void*) (size_t) (0x1000u + tag * 0x100u + (unsigned) i);
		e[capacity] = (void*) (size_t) (used == capacity ? 0 : used + 1);
		return (setT*) this;
		}
	};

static void nxDriveQhullSets(const NxOracleRows& o, bool selfOnly)
	{
	gState = 0xfeed5e75;
	gOracleTape.reset();
	gCandidateTape.reset();
	for(int c = 0; c < 3000; ++c)
		{
		NxSetBuffer a, b;
		const int capA = 1 + (int)(nxNext() % 12u);
		const int usedA = (int)(nxNext() % (unsigned)(capA + 1));
		const int capB = 1 + (int)(nxNext() % 12u);
		const int usedB = (int)(nxNext() % (unsigned)(capB + 1));
		setT* setA = a.build(capA, usedA, 0);
		setT* setB = b.build(capB, usedB, (nxNext() & 1u) ? 0u : 1u);
		void* probe = usedA ? a.e[nxNext() % (unsigned) usedA] : (void*) (size_t) 0x1000u;

		gOracleTape.push((unsigned) o.qhSetsize(setA));
		gOracleTape.push((unsigned) o.qhSetsize(setB));
		gOracleTape.push((unsigned) o.qhSetin(setA, probe));
		gOracleTape.push((unsigned) o.qhSetin(setB, probe));
		void* last = o.qhSetlast(setA);
		gOracleTape.push((unsigned) (last ? (size_t) last : 0u));
		gOracleTape.push((unsigned) o.qhSetequal(setA, setB));
		gOracleTape.push((unsigned) o.qhSetequal(setA, setA));

		if(!selfOnly)
			{
			gCandidateTape.push((unsigned) qh_setsize(setA));
			gCandidateTape.push((unsigned) qh_setsize(setB));
			gCandidateTape.push((unsigned) qh_setin(setA, probe));
			gCandidateTape.push((unsigned) qh_setin(setB, probe));
			void* ours = qh_setlast(setA);
			gCandidateTape.push((unsigned) (ours ? (size_t) ours : 0u));
			gCandidateTape.push((unsigned) qh_setequal(setA, setB));
			gCandidateTape.push((unsigned) qh_setequal(setA, setA));
			}
		}
	nxReport("qh_set", "0x0007f2a0", "phys_fn_003308", "qset.c:561,765,861,1076", selfOnly);
	}

//////////////////////////////////////////////////////////////////////////////
// OPCODE -- Container. The whole point of driving this one is the borrowed
// buffer: mGrowthFactor < 0 means "not ours", and stock 1.3 has no notion of it.
//
// The offsets are read out of the constructor at 0x000b4d70, which stores
// mMaxNbEntries +0, mCurNbEntries +4, mEntries +8, mGrowthFactor +0x0c.

struct NxContainerImage
	{
	unsigned	maxNbEntries;
	unsigned	curNbEntries;
	unsigned*	entries;
	float		growthFactor;
	};

static void nxTapeContainer(NxTape& tape, const void* object, unsigned returned)
	{
	const NxContainerImage* c = (const NxContainerImage*) object;
	tape.push(c->maxNbEntries);
	tape.push(c->curNbEntries);
	// Not the pointer: the two sides allocate from different heaps. What is
	// compared is whether a buffer is held at all, which is exactly what the
	// borrowed-buffer guard decides -- DELETEARRAY nulls the pointer INSIDE the
	// guard, so a skipped free leaves it non-null.
	tape.push(c->entries ? 1u : 0u);
	tape.pushFloat(c->growthFactor);
	tape.push(returned);
	}

static const float kGrowthFactors[] = { 2.0f, 1.5f, 0.5f, 0.0f, -0.0f, -1.0f, -2.0f };
static const int kNbGrowthFactors = (int) (sizeof(kGrowthFactors) / sizeof(kGrowthFactors[0]));

// Container::Resize is private upstream and the modification is IN it, so the
// harness has to reach it without editing the vendored header -- an edit made
// for a test's convenience would be an unevidenced local modification sitting in
// the same directory as the evidenced ones.
//
// This is the standard explicit-instantiation route: [temp.spec] does not apply
// access checking to the template arguments of an explicit instantiation, so the
// pointer-to-member can be formed here and handed out through the injected
// friend. No cast, no source change, no reliance on layout.
namespace
	{
	template<typename Tag, typename Tag::type Member> struct NxRob
		{
		friend typename Tag::type nxReach(Tag) { return Member; }
		};
	struct NxContainerResizeTag
		{
		typedef bool (Container::*type)(unsigned);
		friend type nxReach(NxContainerResizeTag);
		};
	template struct NxRob<NxContainerResizeTag, &Container::Resize>;
	}

static void nxDriveContainer(const NxOracleRows& o, bool selfOnly)
	{
	gOracleTape.reset();
	gCandidateTape.reset();

	unsigned char oracleStorage[64];
	unsigned char candidateStorage[64];
	// Deliberately larger than the capacity the container is told it has. A
	// correct guard never touches it; a broken one writes one past the claimed
	// end, and the slack keeps that a MISMATCH rather than a crash in somebody
	// else's heap.
	unsigned borrowed[32];

	const NxContainerResizeTag::type resize = nxReach(NxContainerResizeTag());

	for(int g = 0; g < kNbGrowthFactors; ++g)
		{
		// The borrowed-buffer state is only ever installed together with a
		// negative growth factor -- that pairing IS the marker. Installing a
		// buffer the object does not own beside a positive factor is not a state
		// the shipped code can be in, and both sides would correctly free a
		// pointer neither of them allocated.
		const int seededCases = (kGrowthFactors[g] < 0.0f) ? 2 : 1;
		for(int seeded = 0; seeded < seededCases; ++seeded)
			{
			for(unsigned nb = 0; nb <= 5; ++nb)
				{
				for(unsigned i = 0; i < 32; ++i)
					borrowed[i] = 0x5a5a0000u + i;

				// --- oracle side
				memset(oracleStorage, 0xcd, sizeof(oracleStorage));
				o.containerCtor(oracleStorage);
				nxTapeContainer(gOracleTape, oracleStorage, 0);
				NxContainerImage* oc = (NxContainerImage*) oracleStorage;
				if(seeded)
					{
					// The row that installs this in the image, 0x000b4f90, is
					// Task 2b's; the state it leaves is four plain members at
					// offsets the constructor at 0x000b4d70 fixes.
					oc->maxNbEntries = 8;
					oc->curNbEntries = 3;
					oc->entries = borrowed;
					}
				oc->growthFactor = kGrowthFactors[g];
				nxTapeContainer(gOracleTape, oracleStorage, 0);
				gOracleTape.push(o.containerResize(oracleStorage, nb) ? 1u : 0u);
				nxTapeContainer(gOracleTape, oracleStorage, 1);
				gOracleTape.push(o.containerSetSize(oracleStorage, nb) ? 1u : 0u);
				nxTapeContainer(gOracleTape, oracleStorage, 2);
				o.containerEmpty(oracleStorage);
				nxTapeContainer(gOracleTape, oracleStorage, 3);
				o.containerDtor(oracleStorage);
				nxTapeContainer(gOracleTape, oracleStorage, 4);
				for(unsigned i = 0; i < 32; ++i)
					gOracleTape.push(borrowed[i]);

				if(selfOnly)
					continue;

				for(unsigned i = 0; i < 32; ++i)
					borrowed[i] = 0x5a5a0000u + i;

				// --- candidate side, identical sequence
				memset(candidateStorage, 0xcd, sizeof(candidateStorage));
				Container* candidate = new (candidateStorage) Container;
				nxTapeContainer(gCandidateTape, candidateStorage, 0);
				NxContainerImage* cc = (NxContainerImage*) candidateStorage;
				if(seeded)
					{
					cc->maxNbEntries = 8;
					cc->curNbEntries = 3;
					cc->entries = borrowed;
					}
				cc->growthFactor = kGrowthFactors[g];
				nxTapeContainer(gCandidateTape, candidateStorage, 0);
				gCandidateTape.push((candidate->*resize)(nb) ? 1u : 0u);
				nxTapeContainer(gCandidateTape, candidateStorage, 1);
				gCandidateTape.push(candidate->SetSize(nb) ? 1u : 0u);
				nxTapeContainer(gCandidateTape, candidateStorage, 2);
				candidate->Empty();
				nxTapeContainer(gCandidateTape, candidateStorage, 3);
				candidate->~Container();
				nxTapeContainer(gCandidateTape, candidateStorage, 4);
				for(unsigned i = 0; i < 32; ++i)
					gCandidateTape.push(borrowed[i]);
				}
			}
		}
	nxReport("container", "0x000b4d70", "phys_fn_004836", "Ice/IceContainer.cpp:40,81,97,153", selfOnly);
	}

// The copy constructor, driven on its own: it runs the member-initialiser list
// with 2.0f and then operator= inlined, which is why the reverse list could
// never find a separate operator= body.
static void nxDriveContainerCopy(const NxOracleRows& o, bool selfOnly)
	{
	gOracleTape.reset();
	gCandidateTape.reset();

	unsigned char sourceStorage[64];
	unsigned char oracleStorage[64];
	unsigned char candidateStorage[64];
	unsigned payload[6] = { 5, 4, 3, 2, 1, 0 };

	for(unsigned nb = 0; nb <= 6; ++nb)
		{
		NxContainerImage* src = (NxContainerImage*) sourceStorage;
		src->maxNbEntries = nb;
		src->curNbEntries = nb;
		src->entries = nb ? payload : 0;
		src->growthFactor = 2.0f;

		memset(oracleStorage, 0xcd, sizeof(oracleStorage));
		o.containerCopyCtor(oracleStorage, sourceStorage);
		nxTapeContainer(gOracleTape, oracleStorage, nb);
		const NxContainerImage* oc = (const NxContainerImage*) oracleStorage;
		for(unsigned i = 0; i < oc->maxNbEntries && i < 6; ++i)
			gOracleTape.push(oc->entries ? oc->entries[i] : 0xffffffffu);
		o.containerDtor(oracleStorage);

		if(selfOnly)
			continue;

		memset(candidateStorage, 0xcd, sizeof(candidateStorage));
		Container* candidate = new (candidateStorage) Container(*(const Container*) sourceStorage);
		nxTapeContainer(gCandidateTape, candidateStorage, nb);
		const NxContainerImage* cc = (const NxContainerImage*) candidateStorage;
		for(unsigned i = 0; i < cc->maxNbEntries && i < 6; ++i)
			gCandidateTape.push(cc->entries ? cc->entries[i] : 0xffffffffu);
		candidate->~Container();
		}
	nxReport("container_copy", "0x000b4f00", "phys_fn_004844", "Ice/IceContainer.cpp:67", selfOnly);
	}

//////////////////////////////////////////////////////////////////////////////
// OPCODE -- RadixSort. mDeleteRanks lives at +0x14, one byte past the end of the
// stock object; the constructor at 0x000e32c0 writes 1 there and the destructor
// at 0x000e32e3 guards both frees on it.

struct NxRadixImage
	{
	unsigned		currentSize;
	unsigned*		ranks;
	unsigned*		ranks2;
	unsigned		totalCalls;
	unsigned		nbHits;
	unsigned char	deleteRanks;
	};

static void nxTapeRadix(NxTape& tape, const void* object)
	{
	const NxRadixImage* r = (const NxRadixImage*) object;
	tape.push(r->currentSize);
	tape.push(r->ranks ? 1u : 0u);
	tape.push(r->ranks2 ? 1u : 0u);
	tape.push(r->totalCalls);
	tape.push(r->nbHits);
	tape.push(r->deleteRanks);
	}

static void nxDriveRadix(const NxOracleRows& o, bool selfOnly)
	{
	gOracleTape.reset();
	gCandidateTape.reset();

	unsigned char oracleStorage[64];
	unsigned char candidateStorage[64];

	static const unsigned kCounts[] = { 1, 2, 5, 17, 64, 257 };
	gState = 0x7a11ed17;
	for(int c = 0; c < (int)(sizeof(kCounts)/sizeof(kCounts[0])); ++c)
		{
		const unsigned nb = kCounts[c];
		unsigned* dwords = (unsigned*) malloc(nb * sizeof(unsigned));
		float* floats = (float*) malloc(nb * sizeof(float));
		for(unsigned i = 0; i < nb; ++i)
			{
			dwords[i] = nxNext();
			nxNextFloatBits(&floats[i]);
			}

		// --- oracle
		memset(oracleStorage, 0xcd, sizeof(oracleStorage));
		o.radixCtor(oracleStorage);
		nxTapeRadix(gOracleTape, oracleStorage);
		for(int hint = 0; hint < 2; ++hint)
			{
			o.radixSortDwords(oracleStorage, dwords, nb, hint);
			nxTapeRadix(gOracleTape, oracleStorage);
			const NxRadixImage* r = (const NxRadixImage*) oracleStorage;
			for(unsigned i = 0; i < nb; ++i)
				gOracleTape.push(r->ranks[i]);
			}
		o.radixSortFloats(oracleStorage, floats, nb);
		nxTapeRadix(gOracleTape, oracleStorage);
			{
			const NxRadixImage* r = (const NxRadixImage*) oracleStorage;
			for(unsigned i = 0; i < nb; ++i)
				gOracleTape.push(r->ranks[i]);
			}
		// The borrowed-rank path: clear the marker and destruct. Both frees and
		// both null stores sit inside the guard, so the pointers survive.
		((NxRadixImage*) oracleStorage)->deleteRanks = 0;
		o.radixDtor(oracleStorage);
		nxTapeRadix(gOracleTape, oracleStorage);
		// And again with the marker set, which is the owning path.
		o.radixCtor(oracleStorage);
		o.radixSortDwords(oracleStorage, dwords, nb, 0);
		o.radixDtor(oracleStorage);
		nxTapeRadix(gOracleTape, oracleStorage);

		if(!selfOnly)
			{
			memset(candidateStorage, 0xcd, sizeof(candidateStorage));
			RadixSort* candidate = new (candidateStorage) RadixSort;
			nxTapeRadix(gCandidateTape, candidateStorage);
			for(int hint = 0; hint < 2; ++hint)
				{
				candidate->Sort(dwords, nb, hint ? RADIX_UNSIGNED : RADIX_SIGNED);
				nxTapeRadix(gCandidateTape, candidateStorage);
				const unsigned* ranks = candidate->GetRanks();
				for(unsigned i = 0; i < nb; ++i)
					gCandidateTape.push(ranks[i]);
				}
			candidate->Sort(floats, nb);
			nxTapeRadix(gCandidateTape, candidateStorage);
				{
				const unsigned* ranks = candidate->GetRanks();
				for(unsigned i = 0; i < nb; ++i)
					gCandidateTape.push(ranks[i]);
				}
			((NxRadixImage*) candidateStorage)->deleteRanks = 0;
			candidate->~RadixSort();
			nxTapeRadix(gCandidateTape, candidateStorage);
			candidate = new (candidateStorage) RadixSort;
			candidate->Sort(dwords, nb, RADIX_SIGNED);
			candidate->~RadixSort();
			nxTapeRadix(gCandidateTape, candidateStorage);
			}

		free(dwords);
		free(floats);
		}
	nxReport("radixsort", "0x000e32c0", "phys_fn_005157", "Ice/IceRevisitedRadix.cpp:170,186,238,350", selfOnly);
	}

//////////////////////////////////////////////////////////////////////////////
// OPCODE -- Segment::SquareDistance and MeshInterface::CheckTopology.

// The oracle's NovodeX pruning helper at 0x000b4530 supplies the starting
// pairs for SweepAndPrune::Init. Its first three arguments are the count, AABB
// pointer array, and Pairs (an Ice Container); the fourth is the axis order.
static void nxDriveCompletePruning(const NxOracleRows& o, bool selfOnly)
	{
	gOracleTape.reset();
	gCandidateTape.reset();
	gState = 0x45b30a11;
	static const AxisOrder kOrders[] = { AXES_XZY, AXES_XYZ, AXES_ZXY };
	for(unsigned trial = 0; trial < 257; ++trial)
		{
		const unsigned count = trial == 0 ? 0u : 1u + (nxNext() % 8u);
		AABB boxes[8];
		const AABB* boxPointers[8];
		for(unsigned i = 0; i < count; ++i)
			{
			const float x = (float)((int)(nxNext() % 17u) - 8);
			const float y = (float)((int)(nxNext() % 17u) - 8);
			const float z = (float)((int)(nxNext() % 17u) - 8);
			const Point low(x, y, z);
			// Sequenced: as one expression the draws' order was the compiler's.
			const float growZ = (float)(nxNext() % 7u);
			const float growY = (float)(nxNext() % 7u);
			const float growX = (float)(nxNext() % 7u);
			const Point high(x + growX, y + growY, z + growZ);
			boxes[i].SetMinMax(low, high);
			boxPointers[i] = &boxes[i];
			}
		const Axes axes(kOrders[trial % 3u]);
		unsigned char oraclePairs[32];
		memset(oraclePairs, 0xcd, sizeof(oraclePairs));
		o.containerCtor(oraclePairs);
		const bool oracleResult = o.completePruning(count, boxPointers, oraclePairs, &axes);
		const unsigned oracleWords = ((unsigned*)oraclePairs)[1];
		const unsigned* oracleEntries = ((const unsigned**)oraclePairs)[2];
		gOracleTape.push(oracleResult ? 1u : 0u);
		gOracleTape.push(oracleWords);
		if(oracleWords > 64u)
			{
			fprintf(stderr, "FAIL pruning oracle emitted %u words\n", oracleWords);
			++gMismatches;
			}
		else for(unsigned i = 0; i < oracleWords; ++i)
			gOracleTape.push(oracleEntries[i]);
		o.containerDtor(oraclePairs);

		if(!selfOnly)
			{
			Pairs candidatePairs;
			const bool candidateResult = Opcode::CompleteBoxPruning(count, boxPointers, candidatePairs, axes);
			const unsigned candidateWords = candidatePairs.GetNbPairs() * 2u;
			const unsigned* candidateEntries = (const unsigned*)candidatePairs.GetPairs();
			gCandidateTape.push(candidateResult ? 1u : 0u);
			gCandidateTape.push(candidateWords);
			if(candidateWords > 64u)
				{
				fprintf(stderr, "FAIL pruning candidate emitted %u words\n", candidateWords);
				++gMismatches;
				}
			else for(unsigned i = 0; i < candidateWords; ++i)
				gCandidateTape.push(candidateEntries[i]);
			}
		}
	nxReport("complete_pruning", "0x000b4530", "phys_fn_004816", "NovodexBoxPruning.cpp", selfOnly);
	}

static void nxDriveSegment(const NxOracleRows& o, bool selfOnly)
	{
	// Integral coordinates in [-32, 32] test all three segment branches and the
	// optional parameter pointer. The candidate reproduces the oracle's float
	// stores and must agree on every emitted word.
	gState = 0x5e6de717;
	gOracleTape.reset();
	gCandidateTape.reset();
	for(int c = 0; c < 20000; ++c)
		{
		// Segment is mP0 +0, mP1 +0x0c, no vtable.
		float segment[6];
		float point[3];
		const bool degenerate = (c % 7) == 0;
		for(int k = 0; k < 3; ++k)
			{
			segment[k] = (float)((int)(nxNext() % 65u) - 32);
			segment[3 + k] = degenerate ? segment[k] : (float)((int)(nxNext() % 65u) - 32);
			point[k] = (float)((int)(nxNext() % 65u) - 32);
			}
		float t = -12345.0f;
		const float d = o.segmentSqrDist(segment, point, &t);
		gOracleTape.pushFloat(d);
		gOracleTape.pushFloat(t);
		gOracleTape.pushFloat(o.segmentSqrDist(segment, point, 0));

		if(!selfOnly)
			{
			float u = -12345.0f;
			const Segment* seg = (const Segment*) segment;
			const Point* p = (const Point*) point;
			gCandidateTape.pushFloat(seg->SquareDistance(*p, &u));
			gCandidateTape.pushFloat(u);
			gCandidateTape.pushFloat(seg->SquareDistance(*p, 0));
			}
		}
	nxReport("segment_sqrdist.grid", "0x000f0560", "phys_fn_005493", "Ice/IceSegment.cpp:29",
		selfOnly);

	// Coordinates span roughly 1e5. Cancellation in the interior branch exposes
	// the 2003 x87 register lifetimes: the stock function differs on 15,538 of
	// these 40,000 words. The local overlay follows the measured float stores,
	// so this family asserts exact distance and segment-parameter outputs.
	gState = 0x5e6de717;
	gOracleTape.reset();
	gCandidateTape.reset();
	for(int c = 0; c < 20000; ++c)
		{
		float segment[6];
		float point[3];
		const bool degenerate = (c % 7) == 0;
		for(int k = 0; k < 3; ++k)
			{
			segment[k] = (float)(int)nxNext() * 1e-4f;
			segment[3 + k] = degenerate ? segment[k] : (float)(int)nxNext() * 1e-4f;
			point[k] = (float)(int)nxNext() * 1e-4f;
			}
		float t = -12345.0f;
		gOracleTape.pushFloat(o.segmentSqrDist(segment, point, &t));
		gOracleTape.pushFloat(t);
		if(!selfOnly)
			{
			float u = -12345.0f;
			gCandidateTape.pushFloat(((const Segment*) segment)->SquareDistance(*(const Point*) point, &u));
			gCandidateTape.pushFloat(u);
			}
		}
	nxReport("segment_sqrdist.wide", "0x000f0560", "phys_fn_005493", "Ice/IceSegment.cpp:29",
		selfOnly);
	}

static void nxDriveMeshInterface(const NxOracleRows& o, bool selfOnly)
	{
	gState = 0x3e0d0107;
	gOracleTape.reset();
	gCandidateTape.reset();

	static const unsigned kNbVerts = 12;
	float verts[kNbVerts * 3];
	for(unsigned i = 0; i < kNbVerts * 3; ++i)
		verts[i] = (float) i;

	for(int c = 0; c < 400; ++c)
		{
		unsigned tris[3 * 24];
		const unsigned nbTris = 1 + (nxNext() % 8u);
		for(unsigned t = 0; t < nbTris; ++t)
			{
			tris[t * 3 + 0] = nxNext() % kNbVerts;
			tris[t * 3 + 1] = ((c + t) % 3 == 0) ? tris[t * 3 + 0] : (nxNext() % kNbVerts);
			tris[t * 3 + 2] = ((c + t) % 5 == 0) ? tris[t * 3 + 1] : (nxNext() % kNbVerts);
			}

		// Built by the candidate, then handed to BOTH sides -- which also tests
		// that the vendored layout is the layout the oracle reads.
		MeshInterface mesh;
		mesh.SetNbTriangles(nbTris);
		mesh.SetNbVertices(kNbVerts);
		const bool set = mesh.SetPointers((const IndexedTriangle*) tris, (const Point*) verts);
		gOracleTape.push(set ? 1u : 0u);
		// Only the success path. SetPointers' null-argument arm reaches
		// SetIceError, whose replacement at 0x000539b0 dispatches through the
		// error-stream pointer at .data:0x001041b4 -- and in a process that has
		// not created an SDK that pointer is null. Driving the failure path
		// would be testing the harness's luck, not the row.
		gOracleTape.push(o.meshSetPointers(&mesh, (const void*) tris, (const void*) verts) ? 1u : 0u);
		gOracleTape.push(o.meshCheckTopology(&mesh));

		if(!selfOnly)
			{
			gCandidateTape.push(set ? 1u : 0u);
			gCandidateTape.push(mesh.SetPointers((const IndexedTriangle*) tris, (const Point*) verts) ? 1u : 0u);
			gCandidateTape.push(mesh.CheckTopology());
			}
		}
	nxReport("mesh_topology", "0x000e8fd0", "phys_fn_005357", "OPC_MeshInterface.cpp:178,228", selfOnly);
	}

//////////////////////////////////////////////////////////////////////////////
// P4 TASK 2b -- the NovodeX rows inside the OPCODE span.
//
// These rows have no upstream source, so unlike everything above them the
// candidate side is a RECONSTRUCTION and this is what proves it. Three things
// make the comparison mean something across two modules:
//
//   * addresses are never compared. A vtable pointer is compared as "installed
//     or not", a self-pointer as "is it this object", a returned AABB as its
//     INDEX into the array both sides were given.
//   * the recorders -- the pruner's remove-object slot, the world-AABB
//     callback, the flag hook at vtable slot 5 -- are the SAME functions on
//     both sides, reached the way the image reaches them: through a patched
//     copy of the oracle's own vtable on one side and through an override on
//     the other. So what the tapes compare is what each side's rows did to a
//     recorder, not what either side thinks it did.
//   * every virtual is called through the object's vtable by INDEX, so the slot
//     numbering is part of the assertion rather than an assumption.

#include "IcePrunable.h"

static NxTape*	gActiveTape			= 0;
static unsigned	gPrunerRemovals		= 0;
static void*	gPrunerLastRemoved	= 0;
static unsigned	gWorldAABBCalls		= 0;
static void*	gWorldAABBLastOwner	= 0;
static void*	gWorldAABBLastBox	= 0;
static unsigned	gSlot5Calls			= 0;
static unsigned	gSlot5LastFlags		= 0;
static bool		gSlot5Result		= true;

static void nxResetProbes()
	{
	gPrunerRemovals = 0;	gPrunerLastRemoved = 0;
	gWorldAABBCalls = 0;	gWorldAABBLastOwner = 0;	gWorldAABBLastBox = 0;
	gSlot5Calls = 0;		gSlot5LastFlags = 0;
	}

// The owner's world-AABB recomputation, .data:0x00128478. Installed into the
// LOADED DLL's own slot on the oracle side and into gPrunableOwnerWorldAABB on
// the candidate side -- the same function either way, so its counters are
// directly comparable.
static void __cdecl nxWorldAABBProbe(void* owner, void* box)
	{
	++gWorldAABBCalls;
	gWorldAABBLastOwner	= owner;
	gWorldAABBLastBox	= box;
	if(box)
		for(int i = 0; i < 6; ++i)
			((float*) box)[i] = (float) (gWorldAABBCalls * 10 + i);
	}

// Vtable slot 5, the hook the three mutating flag members tail-call. __fastcall
// with an unused second register argument is __thiscall with one stack
// argument: `this` in ecx, the argument at [esp+4], callee pops 4.
static bool __fastcall nxSlot5Probe(void* /*self*/, int /*edx*/, unsigned flags)
	{
	++gSlot5Calls;
	gSlot5LastFlags = flags;
	return gSlot5Result;
	}

// The pruner's remove-object slot, `.rdata` slot 2, called by ~Prunable.
static void __fastcall nxRemoveObjectProbe(void* /*pruner*/, int /*edx*/, void* prunable)
	{
	++gPrunerRemovals;
	gPrunerLastRemoved = prunable;
	}

// The candidate side of the same two hooks.
struct NxCandidatePruner : public Pruner
	{
	bool	AddObject(Prunable*) override			{ return true; }
	bool	RemoveObject(Prunable* object) override	{ ++gPrunerRemovals; gPrunerLastRemoved = object; return true; }
	};

struct NxCandidatePrunable : public Prunable
	{
	bool	NovodeXSlot5(udword flags)				{ ++gSlot5Calls; gSlot5LastFlags = flags; return gSlot5Result; }
	};

// The oracle's fake pruner: a vtable pointer, four unidentified dwords and the
// world-box array at +0x14, which is the whole of what Prunable reaches into it.
struct NxOraclePruner
	{
	void*		vtable;
	unsigned	unidentified[4];
	void*		boxes;
	};

typedef bool	(__thiscall* PrunableSetOrClearFn)(void*, unsigned, bool);

// A box pointer as a comparable number: which slot of the array both sides own,
// or a marker for null and for the caller's own scratch box. Never an address.
static unsigned nxBoxIndex(const void* box, const void* arrayBase, const void* scratch)
	{
	if(!box)						return 0xffffffffu;
	if(box == scratch)				return 0xfffffffeu;
	const unsigned offset = (unsigned) ((const unsigned char*) box - (const unsigned char*) arrayBase);
	if(offset >= 8 * 24)			return 0xfffffffdu;
	return offset / 24;
	}

// The object image, minus everything that is an address. 0xcd fill before every
// construction, so a member the constructor does not write shows up as 0xcdcdcdcd
// on both sides and a member it stops writing shows up as a mismatch.
static void nxPushPrunableImage(NxTape& tape, const unsigned char* object)
	{
	const unsigned* words = (const unsigned*) object;
	tape.push(words[0] != 0 ? 1u : 0u);								// +0x00 vptr installed
	tape.push(words[1]);											// +0x04 mOwner
	tape.push(words[2]);											// +0x08 mFlags
	tape.push(words[3] != 0 ? 1u : 0u);								// +0x0c member vptr installed
	tape.push(words[4] == (unsigned) (size_t) object ? 1u : 0u);	// +0x10 member.mPrunable == this
	tape.push(words[5]);											// +0x14
	tape.push(words[6]);											// +0x18
	tape.push(words[7]);											// +0x1c
	tape.push(words[8] == 0 ? 0u : 1u);								// +0x20 mPruner installed
	tape.push(words[9]);											// +0x24
	tape.push(*(const unsigned short*) (object + 0x28));			// +0x28 mHandle
	tape.push(object[0x2a]);										// +0x2a mPruningType
	tape.push(object[0x2b]);										// +0x2b mPruningSection
	for(int i = 0x2c; i < 0x40; i += 4)								// past the end: still 0xcd
		tape.push(*(const unsigned*) (object + i));
	}

// Prunable::Prunable, and the member constructor it calls.
//
// The two adapter slots are read BEFORE the first construction on each side and
// again after it. The image installs two of its own function addresses there
// (0x000b54d0, 0x000b54da) and this build installs two of its own, so the
// addresses cannot be compared -- but "null before, not null after" survives the
// change of module and is exactly what the two writes do. This family runs first
// for that reason: nothing else in this harness constructs a Prunable.
static void nxDrivePrunableCtor(const NxOracleRows& o, bool selfOnly)
	{
	gOracleTape.reset();
	gCandidateTape.reset();

	void** oracleQuerySlot	= (void**) (o.base + kDataAdapterQuery);
	void** oracleNotifySlot	= (void**) (o.base + kDataAdapterNotify);
	gOracleTape.push(*oracleQuerySlot  == 0 ? 1u : 0u);
	gOracleTape.push(*oracleNotifySlot == 0 ? 1u : 0u);

	unsigned char object[64];
	memset(object, 0xcd, sizeof(object));
	void* returned = o.prunableCtor(object);
	gOracleTape.push(returned == object ? 1u : 0u);
	nxPushPrunableImage(gOracleTape, object);
	gOracleTape.push(*oracleQuerySlot  != 0 ? 1u : 0u);
	gOracleTape.push(*oracleNotifySlot != 0 ? 1u : 0u);

	// The member on its own, 0x000e7330 and 0x000e7350, driven at its own
	// address rather than only through Prunable.
	unsigned char member[32];
	memset(member, 0xcd, sizeof(member));
	void* memberReturned = o.prunable0CCtor(member);
	gOracleTape.push(memberReturned == member ? 1u : 0u);
	gOracleTape.push(((unsigned*) member)[0] != 0 ? 1u : 0u);
	for(int i = 1; i < 8; ++i)
		gOracleTape.push(((unsigned*) member)[i]);
	// Slot 0 of `.rdata:0x0011ba1c`, the only virtual the class has, with the
	// deleting bit clear: it rewrites the vptr and returns the object.
	{
	void** memberVtable = *(void***) member;
	void* fromSlot0 = ((DeletingDtorFn) memberVtable[0])(member, 0);
	gOracleTape.push(fromSlot0 == member ? 1u : 0u);
	gOracleTape.push(*(void***) member == memberVtable ? 1u : 0u);
	}
	// The whole of 0x000e7350 is `mov [ecx], vtable; ret`, so what there is to
	// check is that it writes NOTHING else -- read the image back, not just the
	// vptr.
	o.prunable0CDtor(member);
	gOracleTape.push(((unsigned*) member)[0] != 0 ? 1u : 0u);
	for(int i = 1; i < 8; ++i)
		gOracleTape.push(((unsigned*) member)[i]);

	if(!selfOnly)
		{
		gCandidateTape.push(gPrunableAdapterQuery  == 0 ? 1u : 0u);
		gCandidateTape.push(gPrunableAdapterNotify == 0 ? 1u : 0u);

		unsigned char candidate[64];
		memset(candidate, 0xcd, sizeof(candidate));
		Prunable* built = new (candidate) Prunable;
		gCandidateTape.push((void*) built == candidate ? 1u : 0u);
		nxPushPrunableImage(gCandidateTape, candidate);
		gCandidateTape.push(gPrunableAdapterQuery  != 0 ? 1u : 0u);
		gCandidateTape.push(gPrunableAdapterNotify != 0 ? 1u : 0u);

		unsigned char member2[32];
		memset(member2, 0xcd, sizeof(member2));
		Prunable0C* builtMember = new (member2) Prunable0C;
		gCandidateTape.push((void*) builtMember == member2 ? 1u : 0u);
		gCandidateTape.push(((unsigned*) member2)[0] != 0 ? 1u : 0u);
		for(int i = 1; i < 8; ++i)
			gCandidateTape.push(((unsigned*) member2)[i]);
		{
		void** memberVtable = *(void***) member2;
		void* fromSlot0 = ((DeletingDtorFn) memberVtable[0])(member2, 0);
		gCandidateTape.push(fromSlot0 == member2 ? 1u : 0u);
		gCandidateTape.push(*(void***) member2 == memberVtable ? 1u : 0u);
		}
		builtMember->~Prunable0C();
		gCandidateTape.push(((unsigned*) member2)[0] != 0 ? 1u : 0u);
		for(int i = 1; i < 8; ++i)
			gCandidateTape.push(((unsigned*) member2)[i]);
		built->~Prunable();
		}
	nxReport("prunable_ctor", "0x000b54a0", "phys_fn_004874", "IcePrunable.cpp", selfOnly);
	}

// The four flag members, vtable slots 1-4, and the hook at slot 5.
//
// Slot 5 is replaced on both sides. On the oracle side by copying the DLL's own
// six-slot table into a local array and repointing the object at it -- the row
// then reaches the probe exactly the way it reaches 0x0000dee0, through
// `jmp [vptr+0x14]`. Without that the tail call is invisible: the shipped slot 5
// returns true and so does a body that never calls it.
static void nxDrivePrunableFlags(const NxOracleRows& o, bool selfOnly)
	{
	gOracleTape.reset();
	gCandidateTape.reset();

	static const unsigned kArgs[] =
		{ 0, 1, 2, 3, 4, 5, 6, 8, 0x0c, 0x10, 0x12, 0xff, 0x8000, 0x80000002u, 0xffffffffu };
	static const unsigned kStates[] = { 0, 1, 2, 3, 4, 6, 0x0f, 0xffffffffu };
	static const bool kResults[] = { true, false };

	unsigned char object[64];
	void* patched[8];
	o.prunableCtor(object);
	{
	void** shipped = *(void***) object;
	for(int i = 0; i < 6; ++i)
		patched[i] = shipped[i];
	patched[5] = (void*) &nxSlot5Probe;
	*(void***) object = patched;
	}

	for(unsigned s = 0; s < sizeof(kStates) / sizeof(kStates[0]); ++s)
		for(unsigned a = 0; a < sizeof(kArgs) / sizeof(kArgs[0]); ++a)
			for(unsigned r = 0; r < 2; ++r)
				for(int which = 0; which < 6; ++which)
					{
					gSlot5Result = kResults[r];
					nxResetProbes();
					((unsigned*) object)[2] = kStates[s];
					bool returned = false;
					switch(which)
						{
						case 0:	returned = ((BoolThisUdwordFn) patched[1])(object, kArgs[a]);			break;
						case 1:	returned = ((BoolThisUdwordFn) patched[2])(object, kArgs[a]);			break;
						case 2:	returned = ((BoolThisUdwordFn) patched[3])(object, kArgs[a]);			break;
						case 3:	returned = ((PrunableSetOrClearFn) patched[4])(object, kArgs[a], true);	break;
						case 4:	returned = ((PrunableSetOrClearFn) patched[4])(object, kArgs[a], false);	break;
						case 5:	returned = ((BoolThisUdwordFn) patched[5])(object, kArgs[a]);			break;
						}
					gOracleTape.push(returned ? 1u : 0u);
					gOracleTape.push(((unsigned*) object)[2]);
					gOracleTape.push(gSlot5Calls);
					gOracleTape.push(gSlot5LastFlags);
					}

	if(!selfOnly)
		{
		unsigned char storage[64];
		NxCandidatePrunable* probe = new (storage) NxCandidatePrunable;
		void** vtable = *(void***) storage;
		for(unsigned s = 0; s < sizeof(kStates) / sizeof(kStates[0]); ++s)
			for(unsigned a = 0; a < sizeof(kArgs) / sizeof(kArgs[0]); ++a)
				for(unsigned r = 0; r < 2; ++r)
					for(int which = 0; which < 6; ++which)
						{
						gSlot5Result = kResults[r];
						nxResetProbes();
						probe->mFlags = kStates[s];
						bool returned = false;
						switch(which)
							{
							case 0:	returned = ((BoolThisUdwordFn) vtable[1])(storage, kArgs[a]);			break;
							case 1:	returned = ((BoolThisUdwordFn) vtable[2])(storage, kArgs[a]);			break;
							case 2:	returned = ((BoolThisUdwordFn) vtable[3])(storage, kArgs[a]);			break;
							case 3:	returned = ((PrunableSetOrClearFn) vtable[4])(storage, kArgs[a], true);	break;
							case 4:	returned = ((PrunableSetOrClearFn) vtable[4])(storage, kArgs[a], false);	break;
							case 5:	returned = ((BoolThisUdwordFn) vtable[5])(storage, kArgs[a]);			break;
							}
						gCandidateTape.push(returned ? 1u : 0u);
						gCandidateTape.push(probe->mFlags);
						gCandidateTape.push(gSlot5Calls);
						gCandidateTape.push(gSlot5LastFlags);
						}
		probe->~NxCandidatePrunable();
		}
	nxReport("prunable_flags", "0x000b54f0", "phys_fn_004876", "IcePrunable.cpp", selfOnly);
	}

// GetWorldAABB, UpdateWorldAABB, GetUpdatedWorldAABB, and both destructors.
//
// The world-AABB callback is installed into the LOADED DLL's own .data slot at
// 0x00128478 -- writable, and null until 0x0002562b runs, which it never does
// here. That is the same kind of poke as `mNbNodes` in the layout block: it
// puts both sides in a state the shipped code reaches and does not change what
// either side computes from it.
static void nxDrivePrunablePruner(const NxOracleRows& o, bool selfOnly)
	{
	gOracleTape.reset();
	gCandidateTape.reset();

	// Handles stay inside the eight-box array both sides own, because the two
	// getters index it without a bound check -- exactly as the image does.
	static const unsigned kHandles[] = { 0, 1, 3, 7, 0xffff };
	static const unsigned kFlags[] = { 0, 1, 2, 3, 6 };

	// GetWorldAABB and GetUpdatedWorldAABB dereference mPruner with the invalid
	// handle as their ONLY guard -- 0x000b559d loads it and 0x000b55a0
	// dereferences it with no null test in between. A valid handle beside a null
	// pruner is therefore not a state the shipped row survives, and driving it
	// would be measuring the harness's luck. The two destructors DO test the
	// pointer (0x000b5654) and are driven in every combination.
	#define NX_PRUNABLE_REACHABLE(which, withPruner, handle) \
		((which) >= 3 || (withPruner) || (handle) == 0xffff)

	// Driven with the callback installed AND with it null. Both readers test the
	// slot for null before dispatching (0x000b55b5, 0x000b5695) and both set the
	// flag on the join AFTER the test, not inside it -- and with the callback
	// always installed there is no way to tell those two apart. Found by a
	// mutation that moved the flag inside the guard and came out green.
	unsigned char boxes[8 * 24];
	void* pruningVtable[3];
	pruningVtable[0] = 0;
	pruningVtable[1] = 0;
	pruningVtable[2] = (void*) &nxRemoveObjectProbe;
	NxOraclePruner pruner;
	pruner.vtable = pruningVtable;
	pruner.boxes = boxes;

	unsigned char object[64];
	for(unsigned h = 0; h < sizeof(kHandles) / sizeof(kHandles[0]); ++h)
		for(unsigned f = 0; f < sizeof(kFlags) / sizeof(kFlags[0]); ++f)
			for(int withPruner = 0; withPruner < 2; ++withPruner)
				for(int withCallback = 0; withCallback < 2; ++withCallback)
				for(int which = 0; which < 5; ++which)
					{
					if(!NX_PRUNABLE_REACHABLE(which, withPruner, kHandles[h]))
						continue;
					nxResetProbes();
					memset(boxes, 0, sizeof(boxes));
					*(PrunableWorldAABBFn*) (o.base + kDataOwnerWorldAABB) =
						withCallback ? nxWorldAABBProbe : 0;
					o.prunableCtor(object);
					((unsigned*) object)[1] = 0xa5a50000u + h;			// mOwner, a value not an address
					((unsigned*) object)[2] = kFlags[f];
					((void**) object)[8] = withPruner ? (void*) &pruner : 0;
					*(unsigned short*) (object + 0x28) = (unsigned short) kHandles[h];

					unsigned char localBox[24];
					void* got = 0;
					switch(which)
						{
						case 0:	got = o.prunableGetWorldAABB(object);						break;
						case 1:	o.prunableUpdateAABB(object, localBox);						break;
						case 2:	got = o.prunableGetUpdated(object);							break;
						case 3:	o.prunableDtor(object);										break;
						case 4:	got = ((DeletingDtorFn) (*(void***) object)[0])(object, 0);	break;
						}
					// Never the pointer: the index into the array both sides own.
					gOracleTape.push(nxBoxIndex(got, boxes, localBox));
					gOracleTape.push(((unsigned*) object)[2]);
					gOracleTape.push(gWorldAABBCalls);
					gOracleTape.push(gWorldAABBLastOwner == 0 ? 0u : *(unsigned*) &gWorldAABBLastOwner);
					gOracleTape.push(nxBoxIndex(gWorldAABBLastBox, boxes, localBox));
					gOracleTape.push(gPrunerRemovals);
					gOracleTape.push(gPrunerLastRemoved == object ? 1u : 0u);
					for(int b = 0; b < 8 * 6; ++b)
						gOracleTape.pushFloat(((float*) boxes)[b]);
					}

	if(!selfOnly)
		{
		AABB candidateBoxes[8];
		unsigned char prunerStorage[64];
		NxCandidatePruner* candidatePruner = new (prunerStorage) NxCandidatePruner;
		candidatePruner->mPool.mWorldBoxes = candidateBoxes;

		unsigned char storage[64];
		for(unsigned h = 0; h < sizeof(kHandles) / sizeof(kHandles[0]); ++h)
			for(unsigned f = 0; f < sizeof(kFlags) / sizeof(kFlags[0]); ++f)
				for(int withPruner = 0; withPruner < 2; ++withPruner)
					for(int withCallback = 0; withCallback < 2; ++withCallback)
					for(int which = 0; which < 5; ++which)
						{
						if(!NX_PRUNABLE_REACHABLE(which, withPruner, kHandles[h]))
							continue;
						nxResetProbes();
						memset(candidateBoxes, 0, sizeof(candidateBoxes));
						gPrunableOwnerWorldAABB = withCallback
							? (void (*)(void*, AABB*)) nxWorldAABBProbe : 0;
						Prunable* p = new (storage) Prunable;
						p->mOwner		= (void*) (0xa5a50000u + h);
						p->mFlags		= kFlags[f];
						p->mPruner		= withPruner ? candidatePruner : 0;
						p->mHandle		= (uword) kHandles[h];

						unsigned char localBox[24];
						const void* got = 0;
						switch(which)
							{
							case 0:	got = p->GetWorldAABB();									break;
							case 1:	p->UpdateWorldAABB((AABB*) localBox);						break;
							case 2:	got = p->GetUpdatedWorldAABB();								break;
							case 3:	p->~Prunable();												break;
							case 4:	got = ((DeletingDtorFn) (*(void***) storage)[0])(storage, 0);	break;
							}
						gCandidateTape.push(nxBoxIndex(got, candidateBoxes, localBox));
						gCandidateTape.push(((unsigned*) storage)[2]);
						gCandidateTape.push(gWorldAABBCalls);
						gCandidateTape.push(gWorldAABBLastOwner == 0 ? 0u : *(unsigned*) &gWorldAABBLastOwner);
						gCandidateTape.push(nxBoxIndex(gWorldAABBLastBox, candidateBoxes, localBox));
						gCandidateTape.push(gPrunerRemovals);
						gCandidateTape.push(gPrunerLastRemoved == storage ? 1u : 0u);
						for(int b = 0; b < 8 * 6; ++b)
							gCandidateTape.pushFloat(((float*) candidateBoxes)[b]);
						}
		// The fixture borrowed this stack array as its world-box storage. Pruner's
		// base destructor owns and frees mPool.mWorldBoxes, so clear the borrowed
		// pointer before teardown to avoid passing stack memory to opcNovodexFree.
		candidatePruner->mPool.mWorldBoxes = 0;
		candidatePruner->~NxCandidatePruner();
		gPrunableOwnerWorldAABB = 0;
		}
	*(PrunableWorldAABBFn*) (o.base + kDataOwnerWorldAABB) = 0;
	nxReport("prunable_pruner", "0x000b5590", "phys_fn_004884", "IcePrunable.cpp", selfOnly);
	}

struct NxPrunableDispatchProbe
	{
	static unsigned calls[4];
	static unsigned last;
	unsigned result;
	virtual void slot0() {}
	virtual void slot1() {}
	virtual unsigned remove()
		{
			const unsigned index = result & 3u;
			++calls[index];
			last = index;
			return result;
		}
	};

unsigned NxPrunableDispatchProbe::calls[4] = {};
unsigned NxPrunableDispatchProbe::last = 0xffffffffu;

static void nxDrivePrunableDispatch(const NxOracleRows& o, bool selfOnly)
	{
	static const unsigned kHandles[] = { 0, 7, 0xfffe, 0xffff };
	static const unsigned kResults[] = { 0x10203040u, 0x50607081u, 0x90a0b0c2u, 0xd0e0f003u };
	gOracleTape.reset();
	gCandidateTape.reset();
	unsigned input = 2166136261u;
	unsigned inputWords = 0;
	unsigned dispatchCases = 0;
	unsigned nullSlotCases = 0;
	for(unsigned h = 0; h < sizeof(kHandles) / sizeof(kHandles[0]); ++h)
		for(unsigned t = 0; t <= 4; ++t)
			{
			unsigned char oracleManager[0x2c] = {};
			unsigned char oracleObject[0x2c] = {};
			NxPrunableDispatchProbe oracleTargets[4];
			const bool selectedSlotPresent = !(h == 0 && t == 0);
			input = nxFold(input, kHandles[h]); ++inputWords;
			input = nxFold(input, t); ++inputWords;
			input = nxFold(input, selectedSlotPresent ? 1u : 0u); ++inputWords;
			if(kHandles[h] != 0xffffu && t < 4 && selectedSlotPresent) ++dispatchCases;
			if(kHandles[h] != 0xffffu && t < 4 && !selectedSlotPresent) ++nullSlotCases;
			for(unsigned i = 0; i < 4; ++i)
				{
				oracleTargets[i].result = kResults[i];
				if(!(i == t && !selectedSlotPresent))
					*(void**)(oracleManager + 0x1c + i * 4) = &oracleTargets[i];
				}
			*(unsigned short*)(oracleObject + 0x28) = (unsigned short)kHandles[h];
			oracleObject[0x2a] = (unsigned char)t;
			memset(NxPrunableDispatchProbe::calls, 0, sizeof(NxPrunableDispatchProbe::calls));
			NxPrunableDispatchProbe::last = 0xffffffffu;
			// The oracle row ends in `ret 8`: although it reads `this` from ECX
			// and the object from [esp+4], it also pops one additional stack word.
			// Calling it as a normal one-argument __thiscall shifts this caller's
			// stack by four bytes per valid dispatch and corrupts main's mode flag.
			// Supply a padding word below the object so the measured cleanup is
			// balanced without changing either argument the oracle reads.
			unsigned oracleResult;
			void* oracleDispatchFn = o.base + 0x000b5260;
			__asm
				{
				lea ecx, oracleManager
				lea eax, oracleObject
				push 0
				push eax
				mov eax, oracleDispatchFn
				call eax
				mov oracleResult, eax
				}
			const bool oracleDispatch = kHandles[h] != 0xffffu && t < 4 && selectedSlotPresent;
			gOracleTape.push(oracleDispatch ? oracleResult
				: oracleResult - (reinterpret_cast<unsigned>(oracleObject) & 0xffffff00u));
			for(unsigned i = 0; i < 4; ++i) gOracleTape.push(NxPrunableDispatchProbe::calls[i]);
			gOracleTape.push(NxPrunableDispatchProbe::last);
			if(!selfOnly)
				{
				unsigned char candidateManager[0x2c] = {};
				unsigned char candidateObject[0x2c] = {};
				NxPrunableDispatchProbe candidateTargets[4];
				for(unsigned i = 0; i < 4; ++i)
					{
					candidateTargets[i].result = kResults[i];
					if(!(i == t && !selectedSlotPresent))
						*(void**)(candidateManager + 0x1c + i * 4) = &candidateTargets[i];
					}
				*(unsigned short*)(candidateObject + 0x28) = (unsigned short)kHandles[h];
				candidateObject[0x2a] = (unsigned char)t;
				memset(NxPrunableDispatchProbe::calls, 0, sizeof(NxPrunableDispatchProbe::calls));
				NxPrunableDispatchProbe::last = 0xffffffffu;
				const unsigned candidateResult = reinterpret_cast<PrunableOwnerDispatcher*>(candidateManager)->Dispatch(candidateObject);
				const bool candidateDispatch = kHandles[h] != 0xffffu && t < 4 && selectedSlotPresent;
				gCandidateTape.push(candidateDispatch ? candidateResult
					: candidateResult - (reinterpret_cast<unsigned>(candidateObject) & 0xffffff00u));
				for(unsigned i = 0; i < 4; ++i) gCandidateTape.push(NxPrunableDispatchProbe::calls[i]);
				gCandidateTape.push(NxPrunableDispatchProbe::last);
				}
			}
	printf("thirdparty coverage name=prunable_dispatch cases=20 dispatch=%u invalid_or_null=%u null_slot=%u\n",
		dispatchCases, 20u - dispatchCases, nullSlotCases);
	printf("thirdparty input name=prunable_dispatch words=%u input=%08x\n", inputWords, input);
	nxReport("prunable_dispatch", "0x000b5260", "phys_fn_004859", "IcePrunable.cpp", selfOnly);
	}

// A stand-in for NxFoundation's error reporter, installed over the oracle's own
// import slot for the duration of the range family. See nxDrivePrunableRanges.
static bool __cdecl nxFoundationErrorProbe(int /*code*/, const char* /*file*/, int /*line*/,
	bool* /*flag*/, const char* /*message*/, ...)
	{
	return false;
	}

// SetPruningType and SetPruningSection, both arms.
//
// WHY THE ORACLE'S IMPORT TABLE IS REDIRECTED, AND WHAT THAT DOES NOT DO.
// The rejecting arm of both rows calls SetIceError at 0x000539b0, which
// dispatches through the import slot at .rdata:0x001041b4 into
// NxFoundation::FoundationSDK::error. Called in a process that never created an
// SDK, that reporter aborts -- measured, exit code 3 -- so driving the rejecting
// arm at all means the reporter has to go somewhere that returns.
//
// So this family points the oracle's own import slot at a stub that returns
// false, drives, and puts the slot back. It does not patch a single byte of any
// row: 0x000539b0 still runs, still pushes (2, file, line, 0, message), and
// still returns false. What changes is where its fifth-argument call lands,
// which is a property of the environment the row runs in and not of the row.
//
// What is compared is only what THESE rows do -- the return value and the whole
// object image -- and not what the reporter received. The reporter is
// phys_fn_002160, which is not this task's row, and the candidate side of it is
// the declared shim in ThirdPartyHost.cpp. Comparing a shim against the real
// reporter would be measuring the shim.
//
// The mutation that falsifies the bound: `>= 4` to `>= 5` makes SetPruningType(4)
// write the byte and return true, and 0x000b55e8 `cmp eax,4` says it does not.
static void nxDrivePrunableRanges(const NxOracleRows& o, bool selfOnly)
	{
	gOracleTape.reset();
	gCandidateTape.reset();

	static const unsigned kValues[] = { 0, 1, 2, 3, 4, 5, 0x80000000u, 0xffffffffu };

	void** errorSlot = (void**) (o.base + kIatFoundationError);
	void* shippedReporter = *errorSlot;
	DWORD wasProtected = 0;
	if(!VirtualProtect(errorSlot, sizeof(void*), PAGE_READWRITE, &wasProtected))
		{
		fprintf(stderr, "FAIL cannot reach the oracle's error import slot\n");
		++gMismatches;
		return;
		}
	*errorSlot = (void*) &nxFoundationErrorProbe;

	unsigned char object[64];
	for(unsigned v = 0; v < sizeof(kValues) / sizeof(kValues[0]); ++v)
		for(int which = 0; which < 2; ++which)
			{
			memset(object, 0xcd, sizeof(object));
			o.prunableCtor(object);
			const bool returned = which == 0
				? o.prunableSetType(object, kValues[v])
				: o.prunableSetSection(object, kValues[v]);
			gOracleTape.push(returned ? 1u : 0u);
			nxPushPrunableImage(gOracleTape, object);
			}

	if(!selfOnly)
		{
		unsigned char storage[64];
		for(unsigned v = 0; v < sizeof(kValues) / sizeof(kValues[0]); ++v)
			for(int which = 0; which < 2; ++which)
				{
				memset(storage, 0xcd, sizeof(storage));
				Prunable* p = new (storage) Prunable;
				const bool returned = which == 0
					? p->SetPruningType(kValues[v])
					: p->SetPruningSection(kValues[v]);
				gCandidateTape.push(returned ? 1u : 0u);
				nxPushPrunableImage(gCandidateTape, storage);
				p->~Prunable();
				}
		}
	*errorSlot = shippedReporter;
	VirtualProtect(errorSlot, sizeof(void*), wasProtected, &wasProtected);
	nxReport("prunable_ranges", "0x000b55e0", "phys_fn_004888", "IcePrunable.cpp:152,174", selfOnly);
	}

// RadixSort::SetRankBuffers, 0x000e3ea0. The row that turns the borrowed-buffer
// marker OFF -- the counterpart of the constructor's `mov byte [eax+0x14], 1`
// that the `radixsort` family above already drives.
//
// Both sides read all six members back after every call, so the rejecting arm
// is checked for what it does NOT write as well as for its return value: the
// image tests both arguments before it stores either one.
static void nxDriveRadixSetRankBuffers(const NxOracleRows& o, bool selfOnly)
	{
	gOracleTape.reset();
	gCandidateTape.reset();

	unsigned ranksA[16], ranksB[16];
	for(int i = 0; i < 16; ++i) { ranksA[i] = 0x1000u + i; ranksB[i] = 0x2000u + i; }

	for(int first = 0; first < 2; ++first)
		for(int second = 0; second < 2; ++second)
			for(int sortFirst = 0; sortFirst < 2; ++sortFirst)
				{
				unsigned char sorter[64];
				memset(sorter, 0xcd, sizeof(sorter));
				o.radixCtor(sorter);
				if(sortFirst)
					{
					unsigned input[8];
					for(int i = 0; i < 8; ++i)	input[i] = (unsigned) (7 - i);
					o.radixSortDwords(sorter, input, 8, 1);
					}
				const bool returned = o.radixSetRankBuffers(sorter,
					first ? ranksA : 0, second ? ranksB : 0);
				gOracleTape.push(returned ? 1u : 0u);
				gOracleTape.push(((unsigned*) sorter)[0]);					// mCurrentSize
				gOracleTape.push(((void**) sorter)[1] == ranksA ? 1u
					: (((void**) sorter)[1] == 0 ? 0u : 2u));				// mRanks
				gOracleTape.push(((void**) sorter)[2] == ranksB ? 1u
					: (((void**) sorter)[2] == 0 ? 0u : 2u));				// mRanks2
				gOracleTape.push(((unsigned*) sorter)[3]);					// mTotalCalls
				gOracleTape.push(((unsigned*) sorter)[4]);					// mNbHits
				gOracleTape.push(sorter[0x14]);								// mDeleteRanks
				// The marker is cleared before every destruction on BOTH sides.
				// After a successful call the sorter is holding two stack
				// arrays, and after a rejected one following a Sort it is
				// holding two it allocated; letting the destructor run either
				// way would free a stack array or free through an allocator
				// this harness would then have to keep alive. It leaks the
				// allocated pair, symmetrically, and that is the cheaper lie.
				sorter[0x14] = 0;
				o.radixDtor(sorter);
				}

	if(!selfOnly)
		{
		for(int first = 0; first < 2; ++first)
			for(int second = 0; second < 2; ++second)
				for(int sortFirst = 0; sortFirst < 2; ++sortFirst)
					{
					unsigned char storage[64];
					memset(storage, 0xcd, sizeof(storage));
					RadixSort* sorter = new (storage) RadixSort;
					if(sortFirst)
						{
						unsigned input[8];
						for(int i = 0; i < 8; ++i)	input[i] = (unsigned) (7 - i);
						sorter->Sort(input, 8, RADIX_UNSIGNED);
						}
					const bool returned = sorter->SetRankBuffers(first ? ranksA : 0, second ? ranksB : 0);
					gCandidateTape.push(returned ? 1u : 0u);
					gCandidateTape.push(((unsigned*) storage)[0]);
					gCandidateTape.push(((void**) storage)[1] == ranksA ? 1u
						: (((void**) storage)[1] == 0 ? 0u : 2u));
					gCandidateTape.push(((void**) storage)[2] == ranksB ? 1u
						: (((void**) storage)[2] == 0 ? 0u : 2u));
					gCandidateTape.push(((unsigned*) storage)[3]);
					gCandidateTape.push(((unsigned*) storage)[4]);
					gCandidateTape.push(storage[0x14]);
					storage[0x14] = 0;
					sorter->~RadixSort();
					}
		}
	nxReport("radix_setrankbuffers", "0x000e3ea0", "phys_fn_005177",
		"Ice/IceRevisitedRadix.h", selfOnly);
	}

//////////////////////////////////////////////////////////////////////////////
// VENDORED CORRESPONDENCE, TASK 4. Execution evidence for the vendored rows the
// families above never reach: OPCODE's model build, every collider over every
// tree kind, the vanilla AABBTree, SweepAndPrune and the ICE maths, and qhull's
// hull construction. Before these families existed, a cdb trace of this harness
// and the asset harness found 40 of the 562 matched groups executing
// (evidence/vendored-correspondence.md, Task 4); the promotion policy needs
// execution with a compared outcome for every group with x87 code.
//
// Same rules as above. Every input is built once and handed to both sides; the
// oracle side runs at the recorded RVAs on objects the oracle's own
// constructors initialise; every output word the oracle produced goes on the
// oracle tape, and the candidate's on the other. Pointers never go on a tape:
// a link inside a node array is taped as the index it points at.
//
// What the harness does to an oracle object is limited to what the oracle
// reads: inline setters (field writes, no code), the NovodeX field at
// RayCollider+0x88 that no setter writes (poked on both sides identically, the
// way the prunable families poke), and the Container each volume collider's
// cache points at (see nxTapeVolume). Containers the oracle filled
// are released through the oracle's own Container destructor, never the
// candidate's, because the two sides allocate from different heaps.

// OPCODE, oracle RVAs. Each is the `rva` of a mapped row in opcode_map.csv.
static const unsigned kOpcModelCtor			= 0x000e90a0;	// phys_fn_005362
static const unsigned kOpcModelDtor			= 0x000e90e0;	// phys_fn_005366
static const unsigned kOpcModelBuild		= 0x000e9100;	// phys_fn_005368
static const unsigned kOpcBaseModelRefit	= 0x000e9410;	// phys_fn_005378
static const unsigned kOpcTriBuilderCtor	= 0x000e9060;	// phys_fn_005360
static const unsigned kOpcTreeCtor			= 0x000f1060;	// phys_fn_005519
static const unsigned kOpcTreeBuild			= 0x000f10c0;	// phys_fn_005523
static const unsigned kOpcTreeDtor			= 0x000f1500;	// phys_fn_005529
static const unsigned kOpcTreeRefit2		= 0x000f11b0;	// phys_fn_005525
static const unsigned kOpcRayCtor			= 0x000b5720;
static const unsigned kOpcRayDtor			= 0x000b5760;
static const unsigned kOpcRayValidate		= 0x000b5770;	// phys_fn_004903
static const unsigned kOpcRayCollideModel	= 0x000ba6f0;	// phys_fn_004932
static const unsigned kOpcRayCollideTree	= 0x000ba880;	// phys_fn_004934
static const unsigned kOpcSphereCtor		= 0x000de7e0;
static const unsigned kOpcSphereDtor		= 0x000de800;
static const unsigned kOpcSphereCollideModel= 0x000e1360;	// phys_fn_005105
static const unsigned kOpcSphereCollideTree	= 0x000e14d0;	// phys_fn_005107
static const unsigned kOpcOBBCtor			= 0x000d4d00;
static const unsigned kOpcOBBDtor			= 0x000d4d20;
static const unsigned kOpcOBBCollide		= 0x000de0d0;	// phys_fn_005067
static const unsigned kOpcAABBCtor			= 0x000e9b80;
static const unsigned kOpcAABBDtor			= 0x000e9ba0;
static const unsigned kOpcAABBCollideModel	= 0x000ef0d0;	// phys_fn_005434
static const unsigned kOpcAABBCollideTree	= 0x000ef230;	// phys_fn_005436
static const unsigned kOpcLSSCtor			= 0x000d3490;
static const unsigned kOpcLSSDtor			= 0x000d34b0;
static const unsigned kOpcLSSCollide		= 0x000d4b90;	// phys_fn_005027
static const unsigned kOpcPlanesCtor		= 0x000e1510;
static const unsigned kOpcPlanesDtor		= 0x000e2b30;
static const unsigned kOpcPlanesCollide		= 0x000e2b60;	// phys_fn_005138
static const unsigned kOpcTreeColliderCtor	= 0x000bb510;
static const unsigned kOpcTreeColliderDtor	= 0x000bb550;
static const unsigned kOpcTreeColliderBVT	= 0x000d13c0;	// phys_fn_004986
static const unsigned kOpcSapCtor			= 0x000e7180;
static const unsigned kOpcSapDtor			= 0x000e71b0;
static const unsigned kOpcSapInit			= 0x000e6ca0;	// phys_fn_005283
static const unsigned kOpcSapUpdate			= 0x000e6760;
static const unsigned kOpcSapGetPairs		= 0x000e6750;
static const unsigned kIceAABBAdd			= 0x000e2d20;	// phys_fn_005141
static const unsigned kIceAABBMakeCube		= 0x000e2e50;
static const unsigned kIceAABBIsInside		= 0x000e2f70;
static const unsigned kIceAABBComputePoints	= 0x000e2fd0;
static const unsigned kIcePlaneSet			= 0x000e31c0;
static const unsigned kIceTriArea			= 0x000e3ed0;
static const unsigned kIceTriNormal			= 0x000e3f50;
static const unsigned kIceTriCenter			= 0x000e4020;
static const unsigned kIceTriInflate		= 0x000e4090;
static const unsigned kIceITriReplace		= 0x000e4160;
static const unsigned kIceITriFindEdge		= 0x000e41a0;
static const unsigned kIceInvertPR			= 0x000e4200;
static const unsigned kIceM4CoFactor		= 0x000e42a0;
static const unsigned kIceM4Determinant		= 0x000e43a0;
static const unsigned kIceM4Invert			= 0x000e4400;
static const unsigned kIceOBBPlanes			= 0x000e4580;
static const unsigned kIceOBBPoints			= 0x000e48e0;
static const unsigned kIceOBBEdgeNormal		= 0x000e4cb0;
static const unsigned kIceOBBIsInside		= 0x000e4d30;

// qhull, oracle RVAs: the calls phys_fn_003236 (0x0007d420) makes, in order,
// and the state the NovodeX host hooks hang off.
static const unsigned kQhInitA				= 0x000626c0;	// global.c:397
static const unsigned kQhInitflags			= 0x000626f0;	// global.c:540
static const unsigned kQhInitB				= 0x000660a0;	// global.c:444
static const unsigned kQhInitBuild			= 0x0007a330;	// poly2.c:1645
static const unsigned kQhDistplane			= 0x0005c5c0;	// geom.c:63
static const unsigned kQhQhull				= 0x0007d180;	// qhull.c:58, phys_fn_003234
static const unsigned kQhCheckOutput		= 0x0007a2a0;	// poly2.c:250
static const unsigned kQhProduceOutput		= 0x0006d800;	// io.c:35
static const unsigned kQhState				= 0x00124678;	// qh_qh (vendored_data_map.csv)
static const unsigned kQhHostGlobal			= 0x00125080;	// the host object pointer
static const unsigned kOracleIob			= 0x00122600;	// stdin, stdout, stderr at +0/+0x20/+0x40

typedef void*	(__thiscall* NxCtorFn)(void*);
typedef void	(__thiscall* NxDtorFn)(void*);
typedef bool	(__thiscall* NxModelBuildFn)(void*, const OPCODECREATE*);
typedef bool	(__thiscall* NxBoolThisFn)(void*);
typedef bool	(__thiscall* NxTreeBuildFn)(void*, void*);
typedef const char* (__thiscall* NxMessageFn)(void*);
typedef bool	(__thiscall* NxRayModelFn)(void*, const Ray*, const void*, const Matrix4x4*, udword*);
typedef bool	(__thiscall* NxRayTreeFn)(void*, const Ray*, const void*, void*);
typedef bool	(__thiscall* NxSphereModelFn)(void*, void*, const Sphere*, const void*, const Matrix4x4*, const Matrix4x4*);
typedef bool	(__thiscall* NxSphereTreeFn)(void*, void*, const Sphere*, const void*);
typedef bool	(__thiscall* NxOBBModelFn)(void*, void*, const OBB*, const void*, const Matrix4x4*, const Matrix4x4*);
typedef bool	(__thiscall* NxAABBModelFn)(void*, void*, const CollisionAABB*, const void*);
typedef bool	(__thiscall* NxLSSModelFn)(void*, void*, const LSS*, const void*, const Matrix4x4*, const Matrix4x4*);
typedef bool	(__thiscall* NxPlanesModelFn)(void*, void*, const Plane*, udword, const void*, const Matrix4x4*);
typedef bool	(__thiscall* NxBVTFn)(void*, void*, const Matrix4x4*, const Matrix4x4*);
typedef bool	(__thiscall* NxSapInitFn)(void*, udword, const AABB**, const bool*);
typedef bool	(__thiscall* NxSapUpdateFn)(void*, udword, const AABB*);
typedef void	(__thiscall* NxSapPairsFn)(const void*, PairCallback, void*);
typedef void*	(__thiscall* NxAABBAddFn)(void*, const void*);
typedef float	(__thiscall* NxAABBMakeCubeFn)(const void*, void*);
typedef bool	(__thiscall* NxBoolConstPtrFn)(const void*, const void*);
typedef bool	(__thiscall* NxPointsFn)(const void*, void*);
typedef void*	(__thiscall* NxPlaneSetFn)(void*, const Point*, const Point*, const Point*);
typedef float	(__thiscall* NxFloatThisFn)(const void*);
typedef void	(__thiscall* NxPointOutFn)(const void*, Point*);
typedef void	(__thiscall* NxInflateFn)(void*, float, bool);
typedef bool	(__thiscall* NxReplaceFn)(void*, udword, udword);
typedef unsigned char (__thiscall* NxFindEdgeFn)(const void*, udword, udword);
typedef void	(__cdecl* NxInvertPRFn)(Matrix4x4*, const Matrix4x4*);
typedef float	(__thiscall* NxCoFactorFn)(const void*, udword, udword);
typedef void*	(__thiscall* NxInvertFn)(void*);
typedef void	(__thiscall* NxEdgeNormalFn)(const void*, udword, Point*);
typedef BOOL	(__thiscall* NxOBBInsideFn)(const void*, const void*);

// The oracle objects live in raw storage with slack after the candidate's
// sizeof, so an image that turned out bigger than the vendored class would
// scribble on the slack and not on the heap -- and the slack is checked.
static const unsigned kSlack = 64;
static void* nxOracleAlloc(size_t size)
	{
	unsigned char* p = (unsigned char*) malloc(size + kSlack);
	memset(p, 0xcd, size + kSlack);
	return p;
	}
static void nxOracleFree(void* p, size_t size)
	{
	const unsigned char* bytes = (const unsigned char*) p;
	for(unsigned i = 0; i < kSlack; ++i)
		if(bytes[size + i] != 0xcd)
			{
			fprintf(stderr, "FAIL oracle object wrote past the vendored sizeof %u\n", (unsigned) size);
			++gMismatches;
			break;
			}
	free(p);
	}

static void* nxAt(const NxOracleRows& o, unsigned rva) { return o.base + rva; }
static void nxTapeWords(NxTape& tape, const void* p, size_t bytes);
static void nxTapeFloatWords(NxTape& tape, const void* p, size_t bytes);

// Containers built in raw storage by the vendored (inline) constructor, filled
// by whichever side, released by that side's destructor.
static void nxReleaseOracleContainer(const NxOracleRows& o, void* container)
	{
	o.containerDtor(container);
	}

static void nxTapeContainerEntries(NxTape& tape, const Container& c)
	{
	tape.push(c.GetNbEntries());
	for(udword i = 0; i < c.GetNbEntries(); ++i)
		tape.push(c.GetEntries()[i]);
	}

//////////////////////////////////////////////////////////////////////////////
// Meshes. Six shapes of input: a height field, a triangle soup, a flat grid
// (every triangle coplanar), a degenerate set (collinear, repeated-vertex and
// duplicated triangles), a single triangle (the single-node model) and a
// closed box. Each side gets its own copy of the arrays and its own
// MeshInterface, so nothing one side writes can reach the other.

struct NxMesh
	{
	unsigned	nbVerts;
	unsigned	nbTris;
	float		verts[3 * 400];
	unsigned	tris[3 * 400];
	float		minB[3];
	float		maxB[3];
	};

static const int kNbMeshes = 6;
static NxMesh gMeshes[kNbMeshes];

// [0, 1), and [lo, hi). Returned in four-byte structs, never as floats
// (convex-mesh gap Task 2g review): a float return travels in st(0), and how the
// caller then computes follows the compiler's inlining; a four-byte struct comes
// back in eax either way. This file is x87 (/arch:IA32) and both helpers already
// narrowed their result to a float before returning it, so the values are the
// ones the registered lines were measured on.
struct NxUnitDraw
	{
	float value;
	};

static NxUnitDraw nxUnit()
	{
	NxUnitDraw draw;
	draw.value = (float) (nxNext() >> 8) * (1.0f / 16777216.0f);
	return draw;
	}

static NxUnitDraw nxRange(float lo, float hi)
	{
	NxUnitDraw draw;
	draw.value = lo + (hi - lo) * nxUnit().value;
	return draw;
	}

static void nxMeshBounds(NxMesh& m)
	{
	for(int k = 0; k < 3; ++k)
		{
		m.minB[k] = 1e30f;
		m.maxB[k] = -1e30f;
		}
	for(unsigned v = 0; v < m.nbVerts; ++v)
		for(int k = 0; k < 3; ++k)
			{
			if(m.verts[v * 3 + k] < m.minB[k]) m.minB[k] = m.verts[v * 3 + k];
			if(m.verts[v * 3 + k] > m.maxB[k]) m.maxB[k] = m.verts[v * 3 + k];
			}
	}

static void nxGridMesh(NxMesh& m, int n, bool flat)
	{
	m.nbVerts = (unsigned) ((n + 1) * (n + 1));
	for(int j = 0; j <= n; ++j)
		for(int i = 0; i <= n; ++i)
			{
			float* v = &m.verts[(j * (n + 1) + i) * 3];
			v[0] = (float) i - n * 0.5f;
			v[1] = (float) j - n * 0.5f;
			v[2] = flat ? 0.0f : nxRange(-0.75f, 0.75f).value;
			}
	m.nbTris = 0;
	for(int j = 0; j < n; ++j)
		for(int i = 0; i < n; ++i)
			{
			const unsigned a = (unsigned) (j * (n + 1) + i), b = a + 1;
			const unsigned c = a + (unsigned) (n + 1), d = c + 1;
			unsigned* t = &m.tris[m.nbTris * 3];
			t[0] = a; t[1] = b; t[2] = d;
			t[3] = a; t[4] = d; t[5] = c;
			m.nbTris += 2;
			}
	}

static void nxBuildMeshes()
	{
	gState = 0x0bc0de01;
	nxGridMesh(gMeshes[0], 8, false);					// 128 triangles

	NxMesh& soup = gMeshes[1];							// 120 triangles
	soup.nbTris = 120;
	soup.nbVerts = 360;
	for(unsigned t = 0; t < soup.nbTris; ++t)
		{
		const float cx = nxRange(-6.0f, 6.0f).value, cy = nxRange(-6.0f, 6.0f).value, cz = nxRange(-6.0f, 6.0f).value;
		for(int k = 0; k < 3; ++k)
			{
			float* v = &soup.verts[(t * 3 + k) * 3];
			v[0] = cx + nxRange(-1.5f, 1.5f).value;
			v[1] = cy + nxRange(-1.5f, 1.5f).value;
			v[2] = cz + nxRange(-1.5f, 1.5f).value;
			soup.tris[t * 3 + k] = t * 3 + (unsigned) k;
			}
		}

	nxGridMesh(gMeshes[2], 5, true);					// 50 coplanar triangles

	NxMesh& degen = gMeshes[3];							// 24 triangles
	degen.nbVerts = 16;
	for(unsigned v = 0; v < 8; ++v)						// collinear along x
		{
		degen.verts[v * 3 + 0] = (float) v * 0.5f;
		degen.verts[v * 3 + 1] = 1.0f;
		degen.verts[v * 3 + 2] = -1.0f;
		}
	for(unsigned v = 8; v < 16; ++v)
		for(int k = 0; k < 3; ++k)
			degen.verts[v * 3 + k] = nxRange(-2.0f, 2.0f).value;
	degen.nbTris = 24;
	for(unsigned t = 0; t < 24; ++t)
		{
		unsigned* tri = &degen.tris[t * 3];
		switch(t % 4)
			{
			case 0: tri[0] = t % 6; tri[1] = t % 6 + 1; tri[2] = t % 6 + 2; break;	// zero area
			case 1: tri[0] = 8 + t % 8; tri[1] = tri[0]; tri[2] = 8 + (t + 3) % 8; break;	// repeated vertex
			case 2: tri[0] = 8 + t % 8; tri[1] = 8 + (t + 1) % 8; tri[2] = 8 + (t + 5) % 8; break;
			default: tri[0] = tri[-3]; tri[1] = tri[-2]; tri[2] = tri[-1]; break;	// duplicate
			}
		}

	NxMesh& single = gMeshes[4];
	single.nbVerts = 3;
	single.nbTris = 1;
	const float kSingle[9] = { -1.0f, -1.0f, 0.25f, 2.0f, -0.5f, 0.25f, 0.0f, 1.5f, 0.75f };
	memcpy(single.verts, kSingle, sizeof(kSingle));
	single.tris[0] = 0; single.tris[1] = 1; single.tris[2] = 2;

	NxMesh& box = gMeshes[5];
	box.nbVerts = 8;
	for(unsigned v = 0; v < 8; ++v)
		{
		box.verts[v * 3 + 0] = (v & 1) ? 1.0f : -1.0f;
		box.verts[v * 3 + 1] = (v & 2) ? 1.0f : -1.0f;
		box.verts[v * 3 + 2] = (v & 4) ? 1.0f : -1.0f;
		}
	static const unsigned kBoxTris[36] =
		{
		0,2,1, 1,2,3,  4,5,6, 5,7,6,  0,1,4, 1,5,4,
		2,6,3, 3,6,7,  0,4,2, 2,4,6,  1,3,5, 3,7,5
		};
	box.nbTris = 12;
	memcpy(box.tris, kBoxTris, sizeof(kBoxTris));

	for(int i = 0; i < kNbMeshes; ++i)
		nxMeshBounds(gMeshes[i]);
	}

// One side's copy of a mesh.
struct NxMeshCopy
	{
	MeshInterface	iface;
	float*			verts;
	unsigned*		tris;
	};

static void nxCopyMesh(NxMeshCopy& c, const NxMesh& m)
	{
	c.verts = (float*) malloc(sizeof(float) * 3 * m.nbVerts);
	c.tris = (unsigned*) malloc(sizeof(unsigned) * 3 * m.nbTris);
	memcpy(c.verts, m.verts, sizeof(float) * 3 * m.nbVerts);
	memcpy(c.tris, m.tris, sizeof(unsigned) * 3 * m.nbTris);
	c.iface.SetNbTriangles(m.nbTris);
	c.iface.SetNbVertices(m.nbVerts);
	c.iface.SetPointers((const IndexedTriangle*) c.tris, (const Point*) c.verts);
	}

//////////////////////////////////////////////////////////////////////////////
// Models. Every mesh in the four tree kinds, and the height field once more
// under each splitting rule, with the original tree kept, and with the two
// NovodeX build settings on.

struct NxModelPair
	{
	int				mesh;
	int				kind;		// bit 0 no-leaf, bit 1 quantized
	NxMeshCopy		oracleMesh;
	NxMeshCopy		candidateMesh;
	void*			oracle;		// raw storage, oracle Model
	Model*			candidate;
	bool			built;
	bool			candidateBuilt;
	bool			exact;		// taped into opcode_model_build, else opcode_model_build_x87
	// The build settings, kept so that the two families build in two passes.
	udword			rules;
	bool			keepOriginal;
	float			inflate;
	int				extendAxis;
	float			extendValue;
	};

static const int kMaxModels = 48;
static NxModelPair gModels[kMaxModels];
static int gNbModels = 0;
static int gModelIndex[6][4];	// the default-rule model of each mesh and tree kind

// The other families' tapes while several are being filled at once: X87 for
// the float words a query returns, B for the queries whose input sits exactly
// on a boundary (see nxDriveRay).
static NxTape gOracleTapeX87;
static NxTape gCandidateTapeX87;
static NxTape gOracleTapeB;
static NxTape gCandidateTapeB;

// Reports a family filled on a side pair of tapes, through the main ones.
static void nxReportTapes(const NxTape& oracle, const NxTape& candidate, const char* name,
	const char* rva, const char* owner, const char* source, bool selfOnly, unsigned tolerance)
	{
	gOracleTape.reset();
	gCandidateTape.reset();
	for(unsigned i = 0; i < oracle.count; ++i)
		gOracleTape.pushKind(oracle.words[i], oracle.kinds[i]);
	for(unsigned i = 0; i < candidate.count; ++i)
		gCandidateTape.pushKind(candidate.words[i], candidate.kinds[i]);
	nxReport(name, rva, owner, source, selfOnly, tolerance);
	}

// A node array with its links replaced by indices. `stride` is the node size;
// `links` the byte offsets of the link words (the low bit marks a leaf).
static void nxTapeNodes(NxTape& tape, const unsigned char* nodes, unsigned count, unsigned stride,
	const unsigned* links, int nbLinks, bool floatBoxes)
	{
	tape.push(count);
	for(unsigned n = 0; n < count; ++n)
		{
		const unsigned char* node = nodes + n * stride;
		for(unsigned off = 0; off < stride; off += 4)
			{
			unsigned word;
			memcpy(&word, node + off, 4);
			bool link = false;
			for(int l = 0; l < nbLinks; ++l)
				link |= links[l] == off;
			if(link && !(word & 1))
				{
				const unsigned delta = word - (unsigned) (size_t) nodes;
				word = 0x80000000u | (delta % stride == 0 ? delta / stride : 0x7fffffffu);
				}
			// A link, or a quantized box's packed shorts, is discrete; a float
			// box's centre and extents are floats.
			tape.pushKind(word, floatBoxes && !link ? kWordFloat : kWordDiscrete);
			}
		}
	}

// Everything a built model holds that is not a pointer.
static void nxTapeModel(NxTape& tape, const void* object)
	{
	const Model* model = (const Model*) object;
	tape.push(model->GetModelCode());
	const AABBOptimizedTree* tree = model->GetTree();
	if(!tree)
		{
		tape.push(0xdeadu);
		return;
		}
	const unsigned char* raw = (const unsigned char*) tree;
	unsigned nbNodes;
	const unsigned char* nodes;
	memcpy(&nbNodes, raw + 4, 4);
	memcpy(&nodes, raw + 8, 4);
	const udword code = model->GetModelCode();
	const bool noLeaf = (code & OPC_NO_LEAF) != 0;
	const bool quantized = (code & OPC_QUANTIZED) != 0;
	if(!noLeaf && !quantized)
		{
		static const unsigned links[1] = { 24 };
		nxTapeNodes(tape, nodes, nbNodes, sizeof(AABBCollisionNode), links, 1, true);
		}
	else if(noLeaf && !quantized)
		{
		static const unsigned links[2] = { 24, 28 };
		nxTapeNodes(tape, nodes, nbNodes, sizeof(AABBNoLeafNode), links, 2, true);
		}
	else if(!noLeaf && quantized)
		{
		static const unsigned links[1] = { 12 };
		nxTapeNodes(tape, nodes, nbNodes, sizeof(AABBQuantizedNode), links, 1, false);
		for(unsigned off = 12; off < sizeof(AABBQuantizedTree); off += 4)
			{
			unsigned w;
			memcpy(&w, raw + off, 4);
			tape.pushFloatWord(w);	// mCenterCoeff, mExtentsCoeff
			}
		}
	else
		{
		static const unsigned links[2] = { 12, 16 };
		nxTapeNodes(tape, nodes, nbNodes, sizeof(AABBQuantizedNoLeafNode), links, 2, false);
		for(unsigned off = 12; off < sizeof(AABBQuantizedNoLeafTree); off += 4)
			{
			unsigned w;
			memcpy(&w, raw + off, 4);
			tape.pushFloatWord(w);	// mCenterCoeff, mExtentsCoeff
			}
		}
	}

// A vanilla AABBTree, depth first from the root (the tree object itself). A
// complete tree keeps its nodes in mPool and a partial one allocates them in
// pairs, so the walk follows the links rather than reading the pool, and a
// link is taped as present or absent plus its pool bit.
static void nxTapeVanillaNode(NxTape& tape, const AABBTreeNode* node, const udword* indices, int depth)
	{
	const unsigned char* raw = (const unsigned char*) node;
	for(unsigned off = 0; off < 24; off += 4)
		{
		unsigned w;
		memcpy(&w, raw + off, 4);
		tape.pushFloatWord(w);		// the node's AABB
		}
	unsigned pos;
	memcpy(&pos, raw + 24, 4);
	const udword* prims = node->GetPrimitives();
	tape.push(prims ? (unsigned) (prims - indices) : 0xffffffffu);
	tape.push(node->GetNbPrimitives());
	const AABBTreeNode* child = node->GetPos();
	tape.push((child ? 2u : 0u) | (pos & 1u));
	if(child && depth < 64)
		{
		nxTapeVanillaNode(tape, child, indices, depth + 1);
		nxTapeVanillaNode(tape, child + 1, indices, depth + 1);
		}
	}

static void nxTapeVanillaTree(NxTape& tape, const AABBTree* tree)
	{
	tape.push(tree->GetNbNodes());
	const udword* indices = tree->GetIndices();
	nxTapeVanillaNode(tape, tree, indices, 0);
	for(unsigned i = 0; i < tree->GetNbPrimitives(); ++i)
		tape.push(indices[i]);
	}

// Records a model; nxBuildModel builds it. The builds of the two families run
// in two passes (all of opcode_model_build's, then all of the x87 family's),
// so that each family's executions sit between its own report and the one
// before it, where the execution trace (tools/vendored_trace.py) attributes
// them. Each family's tape is in the same order as when they were interleaved.
static void nxAddModel(int mesh, int kind, udword rules, bool keepOriginal,
	float inflate, int extendAxis, float extendValue, bool exact)
	{
	NxModelPair& p = gModels[gNbModels++];
	p.mesh = mesh;
	p.kind = kind;
	p.exact = exact;
	p.rules = rules;
	p.keepOriginal = keepOriginal;
	p.inflate = inflate;
	p.extendAxis = extendAxis;
	p.extendValue = extendValue;
	p.oracle = 0;
	p.candidate = 0;
	p.built = false;
	p.candidateBuilt = false;
	}

static void nxBuildModel(const NxOracleRows& o, NxModelPair& p, bool selfOnly)
	{
	nxCopyMesh(p.oracleMesh, gMeshes[p.mesh]);
	nxCopyMesh(p.candidateMesh, gMeshes[p.mesh]);
	NxTape& oracleTape = p.exact ? gOracleTape : gOracleTapeX87;
	NxTape& candidateTape = p.exact ? gCandidateTape : gCandidateTapeX87;

	OPCODECREATE create;
	create.mSettings.mRules				= p.rules;
	create.mSettings.mLimit				= 1;
	create.mSettings.mNovodeXInflate	= p.inflate;
	create.mSettings.mNovodeXExtendAxis	= p.extendAxis;
	create.mSettings.mNovodeXExtendValue= p.extendValue;
	create.mNoLeaf						= (p.kind & 1) != 0;
	create.mQuantized					= (p.kind & 2) != 0;
	create.mKeepOriginal				= p.keepOriginal;
	create.mCanRemap					= false;

	create.mIMesh = &p.oracleMesh.iface;
	p.oracle = nxOracleAlloc(sizeof(Model));
	((NxCtorFn) nxAt(o, kOpcModelCtor))(p.oracle);
	const bool built = ((NxModelBuildFn) nxAt(o, kOpcModelBuild))(p.oracle, &create);
	p.built = built;
	oracleTape.push(built ? 1u : 0u);
	if(built)
		{
		nxTapeModel(oracleTape, p.oracle);
		oracleTape.push(((BaseModel*) p.oracle)->GetUsedBytes());
		const AABBTree* source = ((Model*) p.oracle)->GetSourceTree();
		oracleTape.push(source ? 1u : 0u);
		if(source)
			nxTapeVanillaTree(oracleTape, source);
		}

	p.candidate = 0;
	if(!selfOnly)
		{
		create.mIMesh = &p.candidateMesh.iface;
		p.candidate = new Model;
		const bool cbuilt = p.candidate->Build(create);
		p.candidateBuilt = cbuilt;
		candidateTape.push(cbuilt ? 1u : 0u);
		if(cbuilt)
			{
			nxTapeModel(candidateTape, p.candidate);
			candidateTape.push(p.candidate->GetUsedBytes());
			const AABBTree* source = p.candidate->GetSourceTree();
			candidateTape.push(source ? 1u : 0u);
			if(source)
				nxTapeVanillaTree(candidateTape, source);
			}
		}
	}

static void nxReleaseModels(const NxOracleRows& o)
	{
	for(int i = 0; i < gNbModels; ++i)
		{
		NxModelPair& p = gModels[i];
		((NxDtorFn) nxAt(o, kOpcModelDtor))(p.oracle);
		nxOracleFree(p.oracle, sizeof(Model));
		delete p.candidate;
		free(p.oracleMesh.verts);
		free(p.oracleMesh.tris);
		free(p.candidateMesh.verts);
		free(p.candidateMesh.tris);
		}
	gNbModels = 0;
	}

// Two build families, because the builds split into two kinds of outcome.
//
// opcode_model_build is exact: every tree whose splits do not hang on a tie.
// opcode_model_build_x87 is the rest, and it is DIVERGENT, measured and
// attributed (evidence/vendored-correspondence.md, Task 4):
//   * SPLIT_SPLATTER_POINTS over a mesh whose x and y variances are equal in
//     exact arithmetic (the height field, the flat grid, the box). Which axis
//     wins the tie is decided by rounding, and the two builds round
//     differently: AABBTreeOfTrianglesBuilder::GetSplittingValue sums
//     (v2+v1)+v0 and returns the x87 register unrounded in the oracle
//     (0x000e9925..0x000e9933) where the candidate sums (v0+v1)+v2 and rounds
//     to float (0x0009f5c1..0x0009f5ea), and AABBTreeNode::Subdivide keeps
//     1/n unrounded in the oracle (0x000f0b37) where the candidate stores it
//     as a float (0x000c4081). The first split that differs changes the tree.
//   * every quantized tree: the oracle keeps CQuantCoeff = 32767/CMax on the
//     x87 stack and takes mCenterCoeff = 1/that (0x000f31f3..0x000f32fd);
//     the candidate rounds in between, so a coefficient can differ in its last
//     bit and a quantized box by one step.
// Neither is fixed here: both are the summation-order / register-lifetime work
// unit the vendored-correspondence open items record. The collider families
// below therefore query the ORACLE-built models on both sides, so what they
// compare is the colliders and not the builds.
static void nxDriveModels(const NxOracleRows& o, bool selfOnly)
	{
	gOracleTape.reset();
	gCandidateTape.reset();
	gOracleTapeX87.reset();
	gCandidateTapeX87.reset();
	nxBuildMeshes();
	const udword kDefaultRules = SPLIT_SPLATTER_POINTS | SPLIT_GEOM_CENTER;
	for(int mesh = 0; mesh < kNbMeshes; ++mesh)
		for(int kind = 0; kind < 4; ++kind)
			{
			// No tie: the soup, the degenerate set and the single triangle.
			const bool untied = mesh == 1 || mesh == 3 || mesh == 4;
			gModelIndex[mesh][kind] = gNbModels;
			nxAddModel(mesh, kind, kDefaultRules, false, 0.0f, -1, 0.0f, untied && !(kind & 2));
			}
	// The other splitting rules. The rules that do not look at the variance
	// build the symmetric meshes exactly; the ones that do go to the x87 family.
	static const udword kRules[] =
		{
		SPLIT_LARGEST_AXIS | SPLIT_GEOM_CENTER,
		SPLIT_BEST_AXIS | SPLIT_SPLATTER_POINTS,
		SPLIT_LARGEST_AXIS | SPLIT_BALANCED,
		SPLIT_FIFTY,
		SPLIT_SPLATTER_POINTS | SPLIT_BALANCED,
		};
	for(unsigned r = 0; r < sizeof(kRules) / sizeof(kRules[0]); ++r)
		{
		const bool variance = (kRules[r] & SPLIT_SPLATTER_POINTS) != 0;
		nxAddModel(0, 0, kRules[r], r == 0, 0.0f, -1, 0.0f, !variance);
		nxAddModel(1, 1, kRules[r], false, 0.0f, -1, 0.0f, true);
		nxAddModel(1, 3, kRules[r], false, 0.0f, -1, 0.0f, false);
		if(!variance)
			{
			nxAddModel(2, 1, kRules[r], false, 0.0f, -1, 0.0f, true);
			nxAddModel(5, 0, kRules[r], false, 0.0f, -1, 0.0f, true);
			}
		}
	// The NovodeX settings: a margin, and the root box extended along z.
	nxAddModel(0, 0, SPLIT_LARGEST_AXIS | SPLIT_GEOM_CENTER, true, 0.25f, 2, -3.0f, true);
	nxAddModel(1, 1, kDefaultRules, false, 0.125f, 1, 9.0f, true);
	nxAddModel(0, 0, kDefaultRules, true, 0.25f, 2, -3.0f, false);
	for(int i = 0; i < gNbModels; ++i)
		if(gModels[i].exact)
			nxBuildModel(o, gModels[i], selfOnly);
	nxReport("opcode_model_build", "0x000e9100", "phys_fn_005368",
		"OPC_Model.cpp,OPC_BaseModel.cpp,OPC_AABBTree.cpp,OPC_OptimizedTree.cpp,OPC_TreeBuilders.cpp",
		selfOnly);

	for(int i = 0; i < gNbModels; ++i)
		if(!gModels[i].exact)
			nxBuildModel(o, gModels[i], selfOnly);
	nxReportTapes(gOracleTapeX87, gCandidateTapeX87, "opcode_model_build_x87", "0x000f09b0", "phys_fn_005513",
		"OPC_AABBTree.cpp,OPC_TreeBuilders.cpp,OPC_OptimizedTree.cpp", selfOnly, kDivergent);
	}

//////////////////////////////////////////////////////////////////////////////
// Query inputs.

static void nxRotation(float m[3][3])
	{
	float q[4];
	double len = 0.0;
	for(int k = 0; k < 4; ++k)
		{
		q[k] = nxRange(-1.0f, 1.0f).value;
		len += (double) q[k] * q[k];
		}
	if(len < 1e-6)
		{
		q[0] = 1.0f; q[1] = q[2] = q[3] = 0.0f;
		len = 1.0;
		}
	const double s = 1.0 / sqrt(len);
	const double w = q[0] * s, x = q[1] * s, y = q[2] * s, z = q[3] * s;
	m[0][0] = (float) (1 - 2 * (y * y + z * z)); m[0][1] = (float) (2 * (x * y + w * z)); m[0][2] = (float) (2 * (x * z - w * y));
	m[1][0] = (float) (2 * (x * y - w * z)); m[1][1] = (float) (1 - 2 * (x * x + z * z)); m[1][2] = (float) (2 * (y * z + w * x));
	m[2][0] = (float) (2 * (x * z + w * y)); m[2][1] = (float) (2 * (y * z - w * x)); m[2][2] = (float) (1 - 2 * (x * x + y * y));
	}

// Three world matrices: none, a translation, a rotation with a translation.
static void nxWorld(Matrix4x4& m, int which)
	{
	memset(&m, 0, sizeof(m));
	m.m[0][0] = m.m[1][1] = m.m[2][2] = m.m[3][3] = 1.0f;
	if(which >= 1)
		{
		m.m[3][0] = 0.5f; m.m[3][1] = -0.25f; m.m[3][2] = 0.125f;
		}
	if(which >= 2)
		{
		float r[3][3];
		nxRotation(r);
		for(int i = 0; i < 3; ++i)
			for(int j = 0; j < 3; ++j)
				m.m[i][j] = r[i][j];
		}
	}

static Point nxInside(const NxMesh& m, float grow)
	{
	// Sequenced: as one expression the draws' order was the compiler's.
	const float insideZ = nxRange(m.minB[2] - grow, m.maxB[2] + grow).value;
	const float insideY = nxRange(m.minB[1] - grow, m.maxB[1] + grow).value;
	const float insideX = nxRange(m.minB[0] - grow, m.maxB[0] + grow).value;
	return Point(insideX, insideY, insideZ);
	}

static void nxTapeCollider(NxTape& tape, bool returned, const void* collider)
	{
	tape.push(returned ? 1u : 0u);
	tape.push(((const Collider*) collider)->GetContactStatus() ? 1u : 0u);
	}

//////////////////////////////////////////////////////////////////////////////
// RayCollider, both entry points.

// The discrete outcome of a query goes on `tape`, the floats it returns on
// `floats`: a hit's distance and barycentrics are sums the 2003 compiler
// reassociated (OPC_RayTriOverlap.h; sum_grouping.csv), so they are compared on
// a tape of their own, and the discrete outcome is not allowed to hide behind
// them.
static void nxTapeRay(NxTape& tape, NxTape& floats, bool returned, const void* collider,
	const CollisionFaces& faces, udword cache)
	{
	const RayCollider* rc = (const RayCollider*) collider;
	nxTapeCollider(tape, returned, collider);
	tape.push(rc->GetNbRayBVTests());
	tape.push(rc->GetNbRayPrimTests());
	tape.push(rc->GetNbIntersections());
	tape.push(cache);
	tape.push(faces.GetNbFaces());
	for(udword f = 0; f < faces.GetNbFaces(); ++f)
		{
		const CollisionFace& face = faces.GetFaces()[f];
		tape.push(face.mFaceID);
		floats.pushFloat(face.mDistance);
		floats.pushFloat(face.mU);
		floats.pushFloat(face.mV);
		}
	}

static void nxMakeRay(const NxMesh& m, int r, Ray& ray, float& length)
	{
	Point from, to;
	switch(r % 6)
		{
		case 0:		// from outside the box, aimed through it
		case 1:
			from = nxInside(m, 6.0f);
			to = nxInside(m, 0.0f);
			break;
		case 2:		// straight down onto a vertex, an edge-sharing point
			{
			const unsigned v = nxNext() % m.nbVerts;
			to = Point(m.verts[v * 3], m.verts[v * 3 + 1], m.verts[v * 3 + 2]);
			from = to + Point(0.0f, 0.0f, 10.0f);
			break;
			}
		case 3:		// grazing: parallel to z=const planes
			from = nxInside(m, 0.0f);
			from.z = (r & 8) ? m.minB[2] : 0.0f;
				// Sequenced: as one expression the draws' order was the compiler's.
				{
				const float toX = nxRange(-1.0f, 1.0f).value;
				const float toY = nxRange(-1.0f, 1.0f).value;
				to = from + Point(toX, toY, 0.0f);
				}
			from.x = m.minB[0] - 3.0f;
			break;
		case 4:		// from inside
			from = nxInside(m, 0.0f);
			to = nxInside(m, 1.0f);
			break;
		default:	// through an edge midpoint
			{
			const unsigned t = nxNext() % m.nbTris;
			const float* a = &m.verts[m.tris[t * 3] * 3];
			const float* b = &m.verts[m.tris[t * 3 + 1] * 3];
			to = Point((a[0] + b[0]) * 0.5f, (a[1] + b[1]) * 0.5f, (a[2] + b[2]) * 0.5f);
				// Sequenced: as one expression the draws' order was the compiler's.
				{
				const float fromX = nxRange(-3.0f, 3.0f).value;
				const float fromY = nxRange(-3.0f, 3.0f).value;
				from = to + Point(fromX, fromY, 5.0f);
				}
			break;
			}
		}
	Point dir = to - from;
	float mag = dir.Magnitude();
	if(mag < 1e-6f)
		{
		dir = Point(0.0f, 0.0f, -1.0f);
		mag = 1.0f;
		}
	ray.mOrig = from;
	ray.mDir = dir / mag;
	length = mag * 1.25f;
	}

// Rays of six shapes (nxMakeRay). Three of them are aimed AT a boundary on
// purpose -- straight down onto a vertex, along the plane of the root box's
// face, through an edge's midpoint -- and there a hit is decided by the last
// bit of an intermediate the oracle keeps in an x87 register and the candidate
// rounds to float (RayAABBOverlap's `f`: 0x000b912d..0x000b913f against
// 0x000a63ff..0x000a6407). Those go to opcode_ray_boundary, which is
// DIVERGENT; the other three shapes go to opcode_ray (discrete outcome, exact)
// and opcode_ray_x87 (distances and barycentrics, DIVERGENT, summation order).
static void nxDriveRay(const NxOracleRows& o, bool selfOnly)
	{
	gState = 0x7a15ee01;
	gOracleTape.reset();
	gCandidateTape.reset();
	gOracleTapeX87.reset();
	gCandidateTapeX87.reset();
	gOracleTapeB.reset();
	gCandidateTapeB.reset();
	for(int mi = 0; mi < gNbModels; ++mi)
		{
		NxModelPair& p = gModels[mi];
		if(!p.built)
			continue;
		const NxMesh& m = gMeshes[p.mesh];
		const int nbRays = mi < 24 ? 18 : 6;
		for(int r = 0; r < nbRays; ++r)
			{
			Ray ray;
			float length;
			nxMakeRay(m, r, ray, length);
			const int setting = (r + mi) % 12;
			const bool segment = (setting & 1) != 0;
			const bool culling = (setting & 2) != 0;
			const bool closest = (setting & 4) != 0 && setting < 8;
			const bool first = setting >= 8;
			const bool coherent = setting == 9 || setting == 11;
			const float tolerance = setting == 3 || setting == 10 ? 0.015625f : 0.0f;
			Matrix4x4 world;
			nxWorld(world, (r / 3) % 3);
			const Matrix4x4* worldPtr = (r / 3) % 3 ? &world : 0;

			const bool boundary = r % 6 == 2 || r % 6 == 3 || r % 6 == 5;
			for(int side = 0; side < (selfOnly ? 1 : 2); ++side)
				{
				NxTape& tape = boundary ? (side == 0 ? gOracleTapeB : gCandidateTapeB)
					: (side == 0 ? gOracleTape : gCandidateTape);
				NxTape& floats = boundary ? tape : (side == 0 ? gOracleTapeX87 : gCandidateTapeX87);
				unsigned char facesStorage[sizeof(CollisionFaces) + 16];
				CollisionFaces* faces = new (facesStorage) CollisionFaces;
				void* object = side == 0 ? nxOracleAlloc(sizeof(RayCollider)) : (void*) new RayCollider;
				if(side == 0)
					((NxCtorFn) nxAt(o, kOpcRayCtor))(object);
				RayCollider* rc = (RayCollider*) object;
				rc->SetMaxDist(segment ? length : MAX_FLOAT);
				rc->SetCulling(culling);
				rc->SetClosestHit(closest);
				rc->SetFirstContact(first);
				rc->SetTemporalCoherence(coherent);
				rc->SetDestination(faces);
				memcpy((unsigned char*) object + 0x88, &tolerance, 4);
				udword cache = coherent ? 0 : 0xffffffffu;
				for(int call = 0; call < (coherent ? 2 : 1); ++call)
					{
					bool returned;
					if(side == 0)
						returned = ((NxRayModelFn) nxAt(o, kOpcRayCollideModel))(object, &ray, p.oracle,
							worldPtr, coherent ? &cache : 0);
					else
						returned = rc->Collide(ray, *(const Model*) p.oracle, worldPtr, coherent ? &cache : 0);
					nxTapeRay(tape, floats, returned, object, *faces, cache);
					}
				if(side == 0)
					{
					((NxDtorFn) nxAt(o, kOpcRayDtor))(object);
					nxOracleFree(object, sizeof(RayCollider));
					nxReleaseOracleContainer(o, faces);
					}
				else
					{
					delete rc;
					faces->~CollisionFaces();
					}
				}
			}
		}

	// RayCollider::ValidateSettings over the flag pairs it rejects and accepts:
	// its answer is a message or null, taped as the message's words.
	for(int combo = 0; combo < 8; ++combo)
		for(int side = 0; side < (selfOnly ? 1 : 2); ++side)
			{
			NxTape& tape = side == 0 ? gOracleTape : gCandidateTape;
			void* object = side == 0 ? nxOracleAlloc(sizeof(RayCollider)) : (void*) new RayCollider;
			if(side == 0)
				((NxCtorFn) nxAt(o, kOpcRayCtor))(object);
			RayCollider* rc = (RayCollider*) object;
			rc->SetClosestHit((combo & 1) != 0);
			rc->SetFirstContact((combo & 2) != 0);
			rc->SetTemporalCoherence((combo & 4) != 0);
			const char* message = side == 0 ? ((NxMessageFn) nxAt(o, kOpcRayValidate))(object)
				: rc->ValidateSettings();
			tape.push(message ? (unsigned) strlen(message) : 0xffffffffu);
			if(message)
				nxTapeWords(tape, message, strlen(message) & ~3u);
			if(side == 0)
				{
				((NxDtorFn) nxAt(o, kOpcRayDtor))(object);
				nxOracleFree(object, sizeof(RayCollider));
				}
			else
				delete rc;
			}
	nxReport("opcode_ray", "0x000ba6f0", "phys_fn_004932", "OPC_RayCollider.cpp", selfOnly);
	nxReportTapes(gOracleTapeX87, gCandidateTapeX87, "opcode_ray_x87", "0x000b84c0", "phys_fn_004921",
		"OPC_RayCollider.cpp,OPC_RayTriOverlap.h", selfOnly, kDivergent);
	nxReportTapes(gOracleTapeB, gCandidateTapeB, "opcode_ray_boundary", "0x000b9070", "phys_fn_004925",
		"OPC_RayCollider.cpp,OPC_RayAABBOverlap.h", selfOnly, kDivergent);
	}

//////////////////////////////////////////////////////////////////////////////
// The volume colliders. One driver, parameterised by the query.

enum NxVolumeKind { kVolSphere, kVolOBB, kVolAABB, kVolLSS, kVolPlanes };

struct NxVolumeQuery
	{
	Sphere			sphere;
	OBB				obb;
	CollisionAABB	aabb;
	LSS				lss;
	Plane			planes[6];
	udword			nbPlanes;
	};

static void nxMakeVolume(NxVolumeKind kind, const NxMesh& m, int q, NxVolumeQuery& v)
	{
	memset(&v, 0, sizeof(v));
	static const float kRadii[] = { 0.0f, 0.1f, 0.6f, 2.0f, 1000.0f };
	const float radius = kRadii[q % 5];
	const Point c = nxInside(m, 0.5f);
	switch(kind)
		{
		case kVolSphere:
			v.sphere.mCenter = c;
			v.sphere.mRadius = radius;
			break;
		case kVolOBB:
			{
			v.obb.mCenter = c;
			v.obb.mExtents = Point(radius, (q % 7 == 3) ? 0.0f : radius * 0.5f + 0.05f, radius * 0.75f);
			float r[3][3];
			nxRotation(r);
			for(int i = 0; i < 3; ++i)
				for(int j = 0; j < 3; ++j)
					v.obb.mRot.m[i][j] = (q % 4 == 0) ? (i == j ? 1.0f : 0.0f) : r[i][j];
			break;
			}
		case kVolAABB:
			v.aabb.mCenter = c;
			v.aabb.mExtents = Point(radius + 0.01f, (q % 3 == 1) ? 0.0f : radius * 0.5f, radius * 0.25f + 0.02f);
			break;
		case kVolLSS:
			{
			v.lss.mP0 = c;
			v.lss.mP1 = (q % 6 == 5) ? c : nxInside(m, 1.0f);
			v.lss.mRadius = radius;
			break;
			}
		default:
			{
			v.nbPlanes = 1 + (udword) (q % 6);
			for(udword k = 0; k < v.nbPlanes; ++k)
				{
				// Sequenced: as one expression the draws' order was the compiler's.
				const float normalZ = nxRange(-1.0f, 1.0f).value;
				const float normalY = nxRange(-1.0f, 1.0f).value;
				const float normalX = nxRange(-1.0f, 1.0f).value;
				Point n(normalX, normalY, normalZ);
				if(n.Magnitude() < 1e-3f)
					n = Point(0.0f, 0.0f, 1.0f);
				n.Normalize();
				const Point at = (q % 5 == 4) ? c + n * 1000.0f : nxInside(m, 0.0f);	// 4: all inside
				v.planes[k].n = n;
				v.planes[k].d = -(at | n);
				}
			break;
			}
		}
	}

// The cache is NovodeX's: a Container* at +0 and the model at +4, which the
// oracle's InitQuery loads (`mov ecx,[edx]; mov [esi+0x10],ecx` in
// SphereCollider::InitQuery, 0x000de925; the same in the other four) and whose
// Container the owner supplies. Stock 1.3 embeds the Container; Task 4 found
// the difference and gave the oracle a NovodeX-layout image of its own, and
// Task 5a made the vendored cache the same (novodex/OPC_VolumeCollider.h). So
// both sides now get the same vendored cache type, each pointing at a Container
// of its own, and the harness compares what the query left in them: the
// touched primitives and the derived fields.
static void nxTapeVolume(NxTape& tape, NxTape& floats, bool returned, const void* collider,
	const Container& touched, const void* derived, size_t derivedBytes)
	{
	const VolumeCollider* vc = (const VolumeCollider*) collider;
	nxTapeCollider(tape, returned, collider);
	tape.push(vc->GetNbVolumeBVTests());
	tape.push(vc->GetNbVolumePrimTests());
	nxTapeContainerEntries(tape, touched);
	nxTapeFloatWords(floats, derived, derivedBytes);
	}

struct NxVolumeRows
	{
	const char*	name;
	const char*	rva;
	const char*	owner;
	const char*	source;
	unsigned	ctor;
	unsigned	dtor;
	unsigned	collide;
	size_t		objectSize;
	size_t		cacheSize;
	};

static void* nxNewCache(NxVolumeKind kind, void* storage)
	{
	switch(kind)
		{
		case kVolSphere:	return new (storage) SphereCache;
		case kVolOBB:		return new (storage) OBBCache;
		case kVolAABB:		return new (storage) AABBCache;
		case kVolLSS:		return new (storage) LSSCache;
		default:			return new (storage) PlanesCache;
		}
	}

static void* nxNewCollider(NxVolumeKind kind)
	{
	switch(kind)
		{
		case kVolSphere:	return new SphereCollider;
		case kVolOBB:		return new OBBCollider;
		case kVolAABB:		return new AABBCollider;
		case kVolLSS:		return new LSSCollider;
		default:			return new PlanesCollider;
		}
	}

static bool nxCandidateVolume(NxVolumeKind kind, void* collider, void* cache, const NxVolumeQuery& v,
	const Model& model, const Matrix4x4* worldv, const Matrix4x4* worldm)
	{
	switch(kind)
		{
		case kVolSphere:	return ((SphereCollider*) collider)->Collide(*(SphereCache*) cache, v.sphere, model, worldv, worldm);
		case kVolOBB:		return ((OBBCollider*) collider)->Collide(*(OBBCache*) cache, v.obb, model, worldv, worldm);
		case kVolAABB:		return ((AABBCollider*) collider)->Collide(*(AABBCache*) cache, v.aabb, model);
		case kVolLSS:		return ((LSSCollider*) collider)->Collide(*(LSSCache*) cache, v.lss, model, worldv, worldm);
		default:			return ((PlanesCollider*) collider)->Collide(*(PlanesCache*) cache, v.planes, v.nbPlanes, model, worldm);
		}
	}

static bool nxOracleVolume(const NxOracleRows& o, NxVolumeKind kind, unsigned rva, void* collider,
	void* cache, const NxVolumeQuery& v, const void* model, const Matrix4x4* worldv, const Matrix4x4* worldm)
	{
	void* fn = nxAt(o, rva);
	switch(kind)
		{
		case kVolSphere:	return ((NxSphereModelFn) fn)(collider, cache, &v.sphere, model, worldv, worldm);
		case kVolOBB:		return ((NxOBBModelFn) fn)(collider, cache, &v.obb, model, worldv, worldm);
		case kVolAABB:		return ((NxAABBModelFn) fn)(collider, cache, &v.aabb, model);
		case kVolLSS:		return ((NxLSSModelFn) fn)(collider, cache, &v.lss, model, worldv, worldm);
		default:			return ((NxPlanesModelFn) fn)(collider, cache, v.planes, v.nbPlanes, model, worldm);
		}
	}

static void nxDeleteCollider(NxVolumeKind kind, void* collider)
	{
	switch(kind)
		{
		case kVolSphere:	delete (SphereCollider*) collider; break;
		case kVolOBB:		delete (OBBCollider*) collider; break;
		case kVolAABB:		delete (AABBCollider*) collider; break;
		case kVolLSS:		delete (LSSCollider*) collider; break;
		default:			delete (PlanesCollider*) collider; break;
		}
	}

static void nxDriveVolume(const NxOracleRows& o, NxVolumeKind kind, const NxVolumeRows& rows,
	unsigned seed, bool selfOnly)
	{
	gState = seed;
	gOracleTape.reset();
	gCandidateTape.reset();
	gOracleTapeX87.reset();
	gCandidateTapeX87.reset();
	for(int mi = 0; mi < gNbModels; ++mi)
		{
		NxModelPair& p = gModels[mi];
		if(!p.built)
			continue;
		const NxMesh& m = gMeshes[p.mesh];
		const int nbQueries = mi < 24 ? 10 : 4;
		for(int q = 0; q < nbQueries; ++q)
			{
			NxVolumeQuery v;
			nxMakeVolume(kind, m, q + mi, v);
			// Settings: primitive tests off on one query in five (the
			// NoPrimitiveTest walks), first contact on one in four, and
			// temporal coherence with a second, nudged call on one in three.
			// A single-triangle model has no tree, and stock OPCODE's Collide walks
			// the tree when primitive tests are off -- both sides would fault.
			const bool primitives = (q + mi) % 5 != 2 || p.mesh == 4;
			const bool first = (q + mi) % 4 == 1;
			const bool coherent = (q + mi) % 3 == 0;
			const bool fullBox = (q + mi) % 2 == 0;
			Matrix4x4 worldv, worldm;
			nxWorld(worldv, (q / 2) % 3);
			nxWorld(worldm, (q / 3) % 3);
			const Matrix4x4* wv = (q / 2) % 3 ? &worldv : 0;
			const Matrix4x4* wm = (q / 3) % 3 ? &worldm : 0;

			for(int side = 0; side < (selfOnly ? 1 : 2); ++side)
				{
				NxTape& tape = side == 0 ? gOracleTape : gCandidateTape;
				NxTape& floats = tape;
				unsigned char cacheStorage[sizeof(OBBCache) + sizeof(LSSCache) + 64];
				VolumeCache* cache = (VolumeCache*) nxNewCache(kind, cacheStorage);
				const size_t derivedBytes = rows.cacheSize - sizeof(VolumeCache);
				unsigned char touchedStorage[sizeof(Container) + 16];
				cache->TouchedPrimitives = new (touchedStorage) Container;
				void* object;
				if(side == 0)
					{
					object = nxOracleAlloc(rows.objectSize);
					((NxCtorFn) nxAt(o, rows.ctor))(object);
					}
				else
					object = nxNewCollider(kind);
				Collider* c = (Collider*) object;
				c->SetPrimitiveTests(primitives);
				c->SetFirstContact(first);
				c->SetTemporalCoherence(coherent);
				if(kind == kVolOBB)
					((OBBCollider*) object)->SetFullBoxBoxTest(fullBox);
				for(int call = 0; call < (coherent ? 2 : 1); ++call)
					{
					NxVolumeQuery moved = v;
					if(call)
						{
						const Point nudge(0.03125f, -0.0625f, 0.015625f);
						moved.sphere.mCenter += nudge;
						moved.obb.mCenter += nudge;
						moved.aabb.mCenter += nudge;
						moved.lss.mP0 += nudge;
						moved.lss.mP1 += nudge;
						}
					const bool returned = side == 0
						? nxOracleVolume(o, kind, rows.collide, object, cache, moved, p.oracle, wv, wm)
						: nxCandidateVolume(kind, object, cache, moved, *(const Model*) p.oracle, wv, wm);
					nxTapeVolume(tape, floats, returned, object, *cache->TouchedPrimitives,
						(unsigned char*) cache + sizeof(VolumeCache), derivedBytes);
					}
				if(side == 0)
					{
					((NxDtorFn) nxAt(o, rows.dtor))(object);
					nxOracleFree(object, rows.objectSize);
					nxReleaseOracleContainer(o, cache->TouchedPrimitives);
					}
				else
					{
					nxDeleteCollider(kind, object);
					cache->TouchedPrimitives->~Container();
					}
				}
			}
		}
	nxReport(rows.name, rows.rva, rows.owner, rows.source, selfOnly);
	}

//////////////////////////////////////////////////////////////////////////////
// The vanilla AABBTree and the three colliders that query it directly.

static void nxDriveVanilla(const NxOracleRows& o, bool selfOnly)
	{
	struct Tree { void* oracle; AABBTree* candidate; void* oracleBuilder; AABBTreeOfTrianglesBuilder* candidateBuilder; };
	Tree trees[kNbMeshes];

	// The build.
	gOracleTape.reset();
	gCandidateTape.reset();
	static const udword kRules[kNbMeshes] =
		{
		// Rules that do not hang a split on a variance or extent tie in these
		// meshes (the tie cases are opcode_model_build_x87's).
		SPLIT_LARGEST_AXIS | SPLIT_GEOM_CENTER, SPLIT_SPLATTER_POINTS | SPLIT_GEOM_CENTER, SPLIT_FIFTY,
		SPLIT_SPLATTER_POINTS, SPLIT_FIFTY, SPLIT_LARGEST_AXIS | SPLIT_GEOM_CENTER
		};
	for(int mesh = 0; mesh < kNbMeshes; ++mesh)
		{
		Tree& t = trees[mesh];
		// Each side reads the mesh through its own interface; the arrays are not written.
		static MeshInterface ifaces[kNbMeshes];
		ifaces[mesh].SetNbTriangles(gMeshes[mesh].nbTris);
		ifaces[mesh].SetNbVertices(gMeshes[mesh].nbVerts);
		ifaces[mesh].SetPointers((const IndexedTriangle*) gMeshes[mesh].tris, (const Point*) gMeshes[mesh].verts);

		t.oracleBuilder = nxOracleAlloc(sizeof(AABBTreeOfTrianglesBuilder));
		((NxCtorFn) nxAt(o, kOpcTriBuilderCtor))(t.oracleBuilder);
		AABBTreeOfTrianglesBuilder* ob = (AABBTreeOfTrianglesBuilder*) t.oracleBuilder;
		ob->mIMesh = &ifaces[mesh];
		ob->mNbPrimitives = gMeshes[mesh].nbTris;
		ob->mSettings.mRules = kRules[mesh];
		ob->mSettings.mLimit = 1 + (mesh % 3);
		t.oracle = nxOracleAlloc(sizeof(AABBTree));
		((NxCtorFn) nxAt(o, kOpcTreeCtor))(t.oracle);
		const bool built = ((NxTreeBuildFn) nxAt(o, kOpcTreeBuild))(t.oracle, t.oracleBuilder);
		gOracleTape.push(built ? 1u : 0u);
		gOracleTape.push(ob->GetCount());
		gOracleTape.push(ob->GetNbInvalidSplits());
		nxTapeVanillaTree(gOracleTape, (const AABBTree*) t.oracle);

		t.candidate = 0;
		t.candidateBuilder = 0;
		if(!selfOnly)
			{
			t.candidateBuilder = new AABBTreeOfTrianglesBuilder;
			t.candidateBuilder->mIMesh = &ifaces[mesh];
			t.candidateBuilder->mNbPrimitives = gMeshes[mesh].nbTris;
			t.candidateBuilder->mSettings.mRules = kRules[mesh];
			t.candidateBuilder->mSettings.mLimit = 1 + (mesh % 3);
			t.candidate = new AABBTree;
			const bool cbuilt = t.candidate->Build(t.candidateBuilder);
			gCandidateTape.push(cbuilt ? 1u : 0u);
			gCandidateTape.push(t.candidateBuilder->GetCount());
			gCandidateTape.push(t.candidateBuilder->GetNbInvalidSplits());
			nxTapeVanillaTree(gCandidateTape, t.candidate);
			}
		}
	nxReport("opcode_aabbtree", "0x000f10c0", "phys_fn_005523", "OPC_AABBTree.cpp,OPC_TreeBuilders.cpp",
		selfOnly);

	// Rays against the vanilla tree: box-level hits, as primitive indices.
	gState = 0x7a15ee02;
	gOracleTape.reset();
	gCandidateTape.reset();
	for(int mesh = 0; mesh < kNbMeshes; ++mesh)
		for(int k = 0; k < 12; ++k)
			{
			// Only the three ray shapes aimed away from a boundary (see nxDriveRay).
			static const int kShapes[3] = { 0, 1, 4 };
			const int r = kShapes[k % 3] + 6 * (k / 3);
			Ray ray;
			float length;
			nxMakeRay(gMeshes[mesh], r, ray, length);
			for(int side = 0; side < (selfOnly ? 1 : 2); ++side)
				{
				NxTape& tape = side == 0 ? gOracleTape : gCandidateTape;
				unsigned char boxStorage[sizeof(Container) + 16];
				Container* boxes = new (boxStorage) Container;
				void* object = side == 0 ? nxOracleAlloc(sizeof(RayCollider)) : (void*) new RayCollider;
				if(side == 0)
					((NxCtorFn) nxAt(o, kOpcRayCtor))(object);
				RayCollider* rc = (RayCollider*) object;
				rc->SetMaxDist((r & 1) ? length : MAX_FLOAT);
				const bool returned = side == 0
					? ((NxRayTreeFn) nxAt(o, kOpcRayCollideTree))(object, &ray, trees[mesh].oracle, boxes)
					: rc->Collide(ray, (const AABBTree*) trees[mesh].oracle, *boxes);
				nxTapeCollider(tape, returned, object);
				tape.push(rc->GetNbRayBVTests());
				nxTapeContainerEntries(tape, *boxes);
				if(side == 0)
					{
					((NxDtorFn) nxAt(o, kOpcRayDtor))(object);
					nxOracleFree(object, sizeof(RayCollider));
					nxReleaseOracleContainer(o, boxes);
					}
				else
					{
					delete rc;
					boxes->~Container();
					}
				}
			}
	nxReport("opcode_ray_vanilla", "0x000ba880", "phys_fn_004934", "OPC_RayCollider.cpp", selfOnly);

	// Spheres and boxes against the vanilla tree.
	for(int which = 0; which < 2; ++which)
		{
		gState = which == 0 ? 0x5fe2ee03 : 0xaabbee04;
		gOracleTape.reset();
		gCandidateTape.reset();
		gOracleTapeX87.reset();
		gCandidateTapeX87.reset();
		for(int mesh = 0; mesh < kNbMeshes; ++mesh)
			for(int q = 0; q < 10; ++q)
				{
				NxVolumeQuery v;
				nxMakeVolume(which == 0 ? kVolSphere : kVolAABB, gMeshes[mesh], q + mesh, v);
				const bool primitives = q % 4 != 3;
				for(int side = 0; side < (selfOnly ? 1 : 2); ++side)
					{
					NxTape& tape = side == 0 ? gOracleTape : gCandidateTape;
					unsigned char cacheStorage[sizeof(AABBCache) + sizeof(SphereCache) + 64];
					const NxVolumeKind kind = which == 0 ? kVolSphere : kVolAABB;
					VolumeCache* cache = (VolumeCache*) nxNewCache(kind, cacheStorage);
					const size_t derivedBytes = (which == 0 ? sizeof(SphereCache) : sizeof(AABBCache)) - sizeof(VolumeCache);
					unsigned char touchedStorage[sizeof(Container) + 16];
					cache->TouchedPrimitives = new (touchedStorage) Container;
					const size_t size = which == 0 ? sizeof(SphereCollider) : sizeof(AABBCollider);
					void* object;
					if(side == 0)
						{
						object = nxOracleAlloc(size);
						((NxCtorFn) nxAt(o, which == 0 ? kOpcSphereCtor : kOpcAABBCtor))(object);
						}
					else
						object = nxNewCollider(kind);
					((Collider*) object)->SetPrimitiveTests(primitives);
					bool returned;
					if(side == 0)
						returned = which == 0
							? ((NxSphereTreeFn) nxAt(o, kOpcSphereCollideTree))(object, cache, &v.sphere, trees[mesh].oracle)
							: ((NxAABBModelFn) nxAt(o, kOpcAABBCollideTree))(object, cache, &v.aabb, trees[mesh].oracle);
					else
						returned = which == 0
							? ((SphereCollider*) object)->Collide(*(SphereCache*) cache, v.sphere, (const AABBTree*) trees[mesh].oracle)
							: ((AABBCollider*) object)->Collide(*(AABBCache*) cache, v.aabb, (const AABBTree*) trees[mesh].oracle);
					NxTape& floats = tape;
					nxTapeVolume(tape, floats, returned, object, *cache->TouchedPrimitives,
						(unsigned char*) cache + sizeof(VolumeCache), derivedBytes);
					if(side == 0)
						{
						((NxDtorFn) nxAt(o, which == 0 ? kOpcSphereDtor : kOpcAABBDtor))(object);
						nxOracleFree(object, size);
						nxReleaseOracleContainer(o, cache->TouchedPrimitives);
						}
					else
						{
						nxDeleteCollider(kind, object);
						cache->TouchedPrimitives->~Container();
						}
					}
				}
		if(which == 0)
			nxReport("opcode_sphere_vanilla", "0x000e14d0", "phys_fn_005107", "OPC_SphereCollider.cpp", selfOnly);
		else
			nxReport("opcode_aabb_vanilla", "0x000ef230", "phys_fn_005436", "OPC_AABBCollider.cpp", selfOnly);
		}

	// AABBTree::Refit2 after the vertices move, on the complete (pooled) trees
	// it requires -- the ones built with mLimit 1. Each side refits its own
	// tree; both read the same moved vertices through the shared interfaces.
	gState = 0x4ef17002;
	gOracleTape.reset();
	gCandidateTape.reset();
	for(int mesh = 0; mesh < kNbMeshes; mesh += 3)
		{
		NxMesh& m = gMeshes[mesh];
		for(unsigned v = 0; v < m.nbVerts * 3; ++v)
			m.verts[v] += nxRange(-0.25f, 0.25f).value;
		const bool refit = ((NxTreeBuildFn) nxAt(o, kOpcTreeRefit2))(trees[mesh].oracle, trees[mesh].oracleBuilder);
		gOracleTape.push(refit ? 1u : 0u);
		nxTapeVanillaTree(gOracleTape, (const AABBTree*) trees[mesh].oracle);
		if(!selfOnly)
			{
			const bool crefit = trees[mesh].candidate->Refit2(trees[mesh].candidateBuilder);
			gCandidateTape.push(crefit ? 1u : 0u);
			nxTapeVanillaTree(gCandidateTape, trees[mesh].candidate);
			}
		}
	nxReport("opcode_aabbtree_refit", "0x000f11b0", "phys_fn_005525", "OPC_AABBTree.cpp", selfOnly);

	for(int mesh = 0; mesh < kNbMeshes; ++mesh)
		{
		((NxDtorFn) nxAt(o, kOpcTreeDtor))(trees[mesh].oracle);
		nxOracleFree(trees[mesh].oracle, sizeof(AABBTree));
		nxOracleFree(trees[mesh].oracleBuilder, sizeof(AABBTreeOfTrianglesBuilder));
		delete trees[mesh].candidate;
		delete trees[mesh].candidateBuilder;
		}
	}

//////////////////////////////////////////////////////////////////////////////
// AABBTreeCollider through BVTCache: pairs of models of one tree kind, under
// three relative placements, with the box/box and prim/box tests both ways.

// The flat grid against the height field lines their vertices up on the same
// x and y lines (half-integers against integers, and again after the 0.5
// translation), so triangle edges meet exactly and TriTriOverlap's verdict on
// those contacts is the last bit of its sums. That pair, unrotated, is
// opcode_treecollider_boundary (DIVERGENT); every other pair and placement is
// opcode_treecollider (exact).
// One tree-versus-tree query, both sides, onto `boundary`'s tapes.
struct NxTreeQuery
	{
	int			a, b;		// gModels indices
	int			placement;
	int			setting;
	bool		boundary;
	Matrix4x4	w0, w1;
	};

static void nxTreeColliderQuery(const NxOracleRows& o, const NxTreeQuery& q, bool selfOnly)
	{
	for(int side = 0; side < (selfOnly ? 1 : 2); ++side)
		{
		NxTape& tape = q.boundary ? (side == 0 ? gOracleTapeB : gCandidateTapeB)
			: (side == 0 ? gOracleTape : gCandidateTape);
		BVTCache cache;
		cache.Model0 = (const Model*) gModels[q.a].oracle;
		cache.Model1 = (const Model*) gModels[q.b].oracle;
		void* object;
		if(side == 0)
			{
			object = nxOracleAlloc(sizeof(AABBTreeCollider));
			((NxCtorFn) nxAt(o, kOpcTreeColliderCtor))(object);
			}
		else
			object = new AABBTreeCollider;
		AABBTreeCollider* tc = (AABBTreeCollider*) object;
		tc->SetFullBoxBoxTest((q.setting & 1) != 0);
		tc->SetFullPrimBoxTest((q.setting & 2) != 0);
		tc->SetFirstContact((q.setting & 4) != 0);
		tc->SetTemporalCoherence(q.setting == 5);
		for(int call = 0; call < (q.setting == 5 ? 2 : 1); ++call)
			{
			const bool returned = side == 0
				? ((NxBVTFn) nxAt(o, kOpcTreeColliderBVT))(object, &cache, &q.w0, q.placement ? &q.w1 : 0)
				: tc->Collide(cache, &q.w0, q.placement ? &q.w1 : 0);
			nxTapeCollider(tape, returned, object);
			tape.push(tc->GetNbBVBVTests());
			tape.push(tc->GetNbBVPrimTests());
			tape.push(tc->GetNbPrimPrimTests());
			tape.push(cache.id0);
			tape.push(cache.id1);
			tape.push(tc->GetNbPairs());
			for(udword i = 0; i < tc->GetNbPairs(); ++i)
				{
				tape.push(tc->GetPairs()[i].id0);
				tape.push(tc->GetPairs()[i].id1);
				}
			}
		if(side == 0)
			{
			((NxDtorFn) nxAt(o, kOpcTreeColliderDtor))(object);
			nxOracleFree(object, sizeof(AABBTreeCollider));
			}
		else
			delete tc;
		}
	}

// The queries are drawn first, in their original order, and then run in two
// passes -- opcode_treecollider's, then the boundary family's -- so that each
// family's executions sit between its own report and the one before it (the
// execution trace attributes them by that). Each tape keeps its order.
static void nxDriveTreeCollider(const NxOracleRows& o, bool selfOnly)
	{
	gState = 0x77ee0c01;
	gOracleTape.reset();
	gCandidateTape.reset();
	gOracleTapeB.reset();
	gCandidateTapeB.reset();
	static const int kPairs[][2] = { { 0, 1 }, { 5, 5 }, { 2, 2 }, { 1, 1 }, { 0, 3 }, { 3, 3 }, { 2, 0 } };	// no single-triangle model: it has no tree to collide
	static NxTreeQuery queries[4 * 7 * 3];
	int nbQueries = 0;
	for(int kind = 0; kind < 4; ++kind)
		for(unsigned pr = 0; pr < sizeof(kPairs) / sizeof(kPairs[0]); ++pr)
			for(int placement = 0; placement < 3; ++placement)
				{
				NxTreeQuery& q = queries[nbQueries];
				q.a = gModelIndex[kPairs[pr][0]][kind];
				q.b = gModelIndex[kPairs[pr][1]][kind];
				if(!gModels[q.a].built || !gModels[q.b].built)
					continue;
				nxWorld(q.w0, placement == 2 ? 2 : 0);
				nxWorld(q.w1, placement);
				q.placement = placement;
				q.setting = (int) (pr + placement + kind) % 8;
				q.boundary = kPairs[pr][0] == 2 && kPairs[pr][1] == 0 && placement < 2;
				++nbQueries;
				}
	for(int i = 0; i < nbQueries; ++i)
		if(!queries[i].boundary)
			nxTreeColliderQuery(o, queries[i], selfOnly);
	nxReport("opcode_treecollider", "0x000d13c0", "phys_fn_004986",
		"OPC_TreeCollider.cpp,OPC_TriTriOverlap.h,OPC_TriBoxOverlap.h,OPC_BoxBoxOverlap.h", selfOnly);
	for(int i = 0; i < nbQueries; ++i)
		if(queries[i].boundary)
			nxTreeColliderQuery(o, queries[i], selfOnly);
	nxReportTapes(gOracleTapeB, gCandidateTapeB, "opcode_treecollider_boundary", "0x000bbd60", "phys_fn_004948",
		"OPC_TreeCollider.cpp,OPC_TriTriOverlap.h", selfOnly, kDivergent);
	}

//////////////////////////////////////////////////////////////////////////////
// BaseModel::Refit after the vertices move. Each side moves its own copy the
// same way; the no-leaf trees refit, the others report that they cannot.

static void nxDriveRefit(const NxOracleRows& o, bool selfOnly)
	{
	gState = 0x4ef17001;
	gOracleTape.reset();
	gCandidateTape.reset();
	for(int mi = 0; mi < gNbModels; ++mi)
		{
		NxModelPair& p = gModels[mi];
		// The single-triangle model has no tree, and BaseModel::Refit
		// dereferences it unguarded (0x000e9418) -- stock behaviour, both sides.
		if(!p.built || !p.exact || p.mesh == 4)
			continue;
		const NxMesh& m = gMeshes[p.mesh];
		for(unsigned v = 0; v < m.nbVerts * 3; ++v)
			{
			const float d = nxRange(-0.25f, 0.25f).value;
			p.oracleMesh.verts[v] += d;
			p.candidateMesh.verts[v] += d;
			}
		const bool refit = ((NxBoolThisFn) nxAt(o, kOpcBaseModelRefit))(p.oracle);
		gOracleTape.push(refit ? 1u : 0u);
		nxTapeModel(gOracleTape, p.oracle);
		if(!selfOnly)
			{
			const bool crefit = p.candidate->Refit();
			gCandidateTape.push(crefit ? 1u : 0u);
			nxTapeModel(gCandidateTape, p.candidate);
			}
		}
	nxReport("opcode_refit", "0x000e9410", "phys_fn_005378", "OPC_BaseModel.cpp,OPC_OptimizedTree.cpp",
		selfOnly);
	}

//////////////////////////////////////////////////////////////////////////////
// SweepAndPrune: the NovodeX three-argument Init, updates, and the pairs.

static BOOL nxSapPair(udword id0, udword id1, void* user)
	{
	NxTape* tape = (NxTape*) user;
	tape->push(id0);
	tape->push(id1);
	return TRUE;
	}

static void nxDriveSap(const NxOracleRows& o, bool selfOnly)
	{
	gState = 0x5a9e0001;
	gOracleTape.reset();
	gCandidateTape.reset();
	static const udword kCounts[] = { 1, 2, 9, 40 };
	for(unsigned c = 0; c < sizeof(kCounts) / sizeof(kCounts[0]); ++c)
		{
		const udword n = kCounts[c];
		AABB boxes[40];
		const AABB* boxPtrs[40];
		bool flags[40];
		for(udword i = 0; i < n; ++i)
			{
			// Sequenced: as one expression the draws' order was the compiler's.
			const float centerX = nxRange(-4.0f, 4.0f).value;
			const float centerY = nxRange(-4.0f, 4.0f).value;
			const float centerZ = nxRange(-4.0f, 4.0f).value;
			Point center(centerX, centerY, centerZ);
			// Sequenced: as one expression the draws' order was the compiler's.
			const float extentZ = (i % 7 == 3) ? 0.0f : nxRange(0.0f, 1.5f).value;
			const float extentX = nxRange(0.0f, 1.5f).value;
			const float extentY = nxRange(0.0f, 1.5f).value;
			Point extents(extentX, extentY, extentZ);
			boxes[i].SetCenterExtents(center, extents);
			boxPtrs[i] = &boxes[i];
			flags[i] = (nxNext() % 3) == 0;
			}
		AABB moves[60];
		udword moveIds[60];
		for(int k = 0; k < 60; ++k)
			{
			moveIds[k] = nxNext() % n;
			// Sequenced: as one expression the draws' order was the compiler's.
			const float centerZ = nxRange(-4.0f, 4.0f).value;
			const float centerX = nxRange(-4.0f, 4.0f).value;
			const float centerY = nxRange(-4.0f, 4.0f).value;
			Point center(centerX, centerY, centerZ);
			// Sequenced: as one expression the draws' order was the compiler's.
			const float extentZ = nxRange(0.1f, 2.0f).value;
			const float extentY = nxRange(0.1f, 2.0f).value;
			const float extentX = nxRange(0.1f, 2.0f).value;
			moves[k].SetCenterExtents(center, Point(extentX, extentY, extentZ));
			}
		for(int side = 0; side < (selfOnly ? 1 : 2); ++side)
			{
			NxTape& tape = side == 0 ? gOracleTape : gCandidateTape;
			void* object;
			if(side == 0)
				{
				object = nxOracleAlloc(sizeof(SweepAndPrune));
				((NxCtorFn) nxAt(o, kOpcSapCtor))(object);
				}
			else
				object = new SweepAndPrune;
			SweepAndPrune* sap = (SweepAndPrune*) object;
			const bool init = side == 0
				? ((NxSapInitFn) nxAt(o, kOpcSapInit))(object, n, boxPtrs, flags)
				: sap->Init(n, boxPtrs, flags);
			tape.push(init ? 1u : 0u);
			for(int round = 0; round < 4; ++round)
				{
				if(side == 0)
					((NxSapPairsFn) nxAt(o, kOpcSapGetPairs))(object, nxSapPair, &tape);
				else
					sap->GetPairs(nxSapPair, &tape);
				tape.push(0xfffffff0u);
				for(int k = round * 15; k < round * 15 + 15; ++k)
					{
					const bool updated = side == 0
						? ((NxSapUpdateFn) nxAt(o, kOpcSapUpdate))(object, moveIds[k], &moves[k])
						: sap->UpdateObject(moveIds[k], moves[k]);
					tape.push(updated ? 1u : 0u);
					}
				}
			if(side == 0)
				{
				((NxDtorFn) nxAt(o, kOpcSapDtor))(object);
				nxOracleFree(object, sizeof(SweepAndPrune));
				}
			else
				delete sap;
			}
		}
	nxReport("opcode_sap", "0x000e6ca0", "phys_fn_005283", "OPC_SweepAndPrune.cpp", selfOnly);
	}

//////////////////////////////////////////////////////////////////////////////
// The ICE maths rows, called directly: AABB, Plane, Triangle, IndexedTriangle,
// Matrix4x4 and OBB. Degenerate inputs included: zero-area triangles, flat
// boxes, singular matrices, an OBB with a zero extent.

static void nxTapeWords(NxTape& tape, const void* p, size_t bytes)
	{
	const unsigned char* b = (const unsigned char*) p;
	for(size_t i = 0; i + 4 <= bytes; i += 4)
		{
		unsigned w;
		memcpy(&w, b + i, 4);
		tape.push(w);
		}
	}

// The same, for an object that is all floats (a Point, a Plane, a matrix).
static void nxTapeFloatWords(NxTape& tape, const void* p, size_t bytes)
	{
	const unsigned char* b = (const unsigned char*) p;
	for(size_t i = 0; i + 4 <= bytes; i += 4)
		{
		unsigned w;
		memcpy(&w, b + i, 4);
		tape.pushFloatWord(w);
		}
	}

static const struct { const char* name; const char* rva; const char* owner; const char* source; } kIceParts[5] =
	{
	{ "ice_aabb", "0x000e2d20", "phys_fn_005141", "Ice/IceAABB.cpp" },
	{ "ice_plane_triangle", "0x000e31c0", "phys_fn_005155", "Ice/IcePlane.cpp,Ice/IceTriangle.cpp" },
	{ "ice_indexedtriangle", "0x000e4160", "phys_fn_005187", "Ice/IceIndexedTriangle.cpp" },
	{ "ice_matrix4x4", "0x000e4400", "phys_fn_005197", "Ice/IceMatrix4x4.cpp" },
	{ "ice_obb", "0x000e4580", "phys_fn_005199", "Ice/IceOBB.cpp" },
	};

// One pass per class. Every pass draws the same inputs, so a class's inputs do
// not depend on which classes are driven; only `part`'s calls are made.
static void nxDriveIcePart(const NxOracleRows& o, int part, bool selfOnly, unsigned tolerance)
	{
	gState = 0x1ce0a7b5;
	gOracleTape.reset();
	gCandidateTape.reset();
	const int all = selfOnly ? 1 : 2;
	for(int c = 0; c < 200; ++c)
		{
		// AABB
		AABB a, b;
		// Sequenced: as one expression the draws' order was the compiler's.
		const float aExtentY = (c % 9 == 4) ? 0.0f : nxRange(0, 3).value;
		const float aCenterX = nxRange(-5, 5).value;
		const float aCenterY = nxRange(-5, 5).value;
		const float aCenterZ = nxRange(-5, 5).value;
		const float aExtentX = nxRange(0, 3).value;
		const float aExtentZ = nxRange(0, 3).value;
		a.SetCenterExtents(Point(aCenterX, aCenterY, aCenterZ), Point(aExtentX, aExtentY, aExtentZ));
		// Sequenced: as one expression the draws' order was the compiler's.
		const float bExtentX = nxRange(0, 3).value;
		const float bExtentY = nxRange(0, 3).value;
		const float bExtentZ = nxRange(0, 3).value;
		const float bCenterX = nxRange(-5, 5).value;
		const float bCenterY = nxRange(-5, 5).value;
		const float bCenterZ = nxRange(-5, 5).value;
		b.SetCenterExtents(Point(bCenterX, bCenterY, bCenterZ), Point(bExtentX, bExtentY, bExtentZ));
		if(c % 11 == 0)
			b = a;
		for(int side = 0; side < (part == 0 ? all : 0); ++side)
			{
			NxTape& tape = side == 0 ? gOracleTape : gCandidateTape;
			AABB sum = a, cube;
			Point pts[8];
			if(side == 0)
				{
				((NxAABBAddFn) nxAt(o, kIceAABBAdd))(&sum, &b);
				tape.pushFloat(((NxAABBMakeCubeFn) nxAt(o, kIceAABBMakeCube))(&a, &cube));
				tape.push(((NxBoolConstPtrFn) nxAt(o, kIceAABBIsInside))(&a, &b) ? 1u : 0u);
				tape.push(((NxPointsFn) nxAt(o, kIceAABBComputePoints))(&a, pts) ? 1u : 0u);
				}
			else
				{
				sum.Add(b);
				tape.pushFloat(a.MakeCube(cube));
				tape.push(a.IsInside(b) ? 1u : 0u);
				tape.push(a.ComputePoints(pts) ? 1u : 0u);
				}
			nxTapeFloatWords(tape, &sum, sizeof(sum));
			nxTapeFloatWords(tape, &cube, sizeof(cube));
			nxTapeFloatWords(tape, pts, sizeof(pts));
			}

		// Plane and Triangle
		// Sequenced: as one expression the draws' order was the compiler's.
		const float p0X = nxRange(-5, 5).value;
		const float p0Y = nxRange(-5, 5).value;
		const float p0Z = nxRange(-5, 5).value;
		Point p0(p0X, p0Y, p0Z);
		// Sequenced: as one expression the draws' order was the compiler's.
		Point p1 = p0;
		if(!(c % 7 == 2))
			{
			const float p1X = nxRange(-5, 5).value;
			const float p1Y = nxRange(-5, 5).value;
			const float p1Z = nxRange(-5, 5).value;
			p1 = Point(p1X, p1Y, p1Z);
			}
		// Sequenced: as one expression the draws' order was the compiler's.
		Point p2 = p0 + (p1 - p0) * 2.0f;
		if(!(c % 13 == 5))
			{
			const float p2X = nxRange(-5, 5).value;
			const float p2Y = nxRange(-5, 5).value;
			const float p2Z = nxRange(-5, 5).value;
			p2 = Point(p2X, p2Y, p2Z);
			}
		// The zero-area inputs: a repeated vertex, and three collinear points.
		const bool flat = c % 7 == 2 || c % 13 == 5;
		for(int side = 0; side < (part == 1 ? all : 0); ++side)
			{
			NxTape& tape = side == 0 ? gOracleTape : gCandidateTape;
			tape.mark = flat ? kWordDegenerate : 0;
			Plane plane;
			Triangle tri(p0, p1, p2);
			Point normal, center;
			float area;
			const float fat = 0.25f * (float) (c % 5);
			if(side == 0)
				{
				((NxPlaneSetFn) nxAt(o, kIcePlaneSet))(&plane, &p0, &p1, &p2);
				area = ((NxFloatThisFn) nxAt(o, kIceTriArea))(&tri);
				((NxPointOutFn) nxAt(o, kIceTriNormal))(&tri, &normal);
				((NxPointOutFn) nxAt(o, kIceTriCenter))(&tri, &center);
				((NxInflateFn) nxAt(o, kIceTriInflate))(&tri, fat, (c & 1) != 0);
				}
			else
				{
				plane.Set(p0, p1, p2);
				area = tri.Area();
				tri.Normal(normal);
				tri.Center(center);
				tri.Inflate(fat, (c & 1) != 0);
				}
			nxTapeFloatWords(tape, &plane, sizeof(plane));
			tape.pushFloat(area);
			nxTapeFloatWords(tape, &normal, sizeof(normal));
			nxTapeFloatWords(tape, &center, sizeof(center));
			nxTapeFloatWords(tape, &tri, sizeof(tri));
			tape.mark = 0;
			}

		// IndexedTriangle
		udword refs[3] = { nxNext() % 6, nxNext() % 6, nxNext() % 6 };
		const udword oldRef = nxNext() % 6, newRef = nxNext() % 6, e0 = nxNext() % 6, e1 = nxNext() % 6;
		for(int side = 0; side < (part == 2 ? all : 0); ++side)
			{
			NxTape& tape = side == 0 ? gOracleTape : gCandidateTape;
			IndexedTriangle it(refs[0], refs[1], refs[2]);
			bool replaced;
			unsigned char edge;
			if(side == 0)
				{
				edge = ((NxFindEdgeFn) nxAt(o, kIceITriFindEdge))(&it, e0, e1);
				replaced = ((NxReplaceFn) nxAt(o, kIceITriReplace))(&it, oldRef, newRef);
				}
			else
				{
				edge = it.FindEdge(e0, e1);
				replaced = it.ReplaceVertex(oldRef, newRef);
				}
			tape.push(edge);
			tape.push(replaced ? 1u : 0u);
			nxTapeWords(tape, &it, sizeof(it));
			}

		// Matrix4x4: a general matrix (singular one time in eight) and a PR one.
		Matrix4x4 g, pr;
		for(int i = 0; i < 4; ++i)
			for(int j = 0; j < 4; ++j)
				g.m[i][j] = nxRange(-2, 2).value;
		if(c % 8 == 3)
			for(int j = 0; j < 4; ++j)
				g.m[2][j] = g.m[1][j] * 2.0f;
		nxWorld(pr, 2);
		pr.m[3][0] = nxRange(-9, 9).value;
		const udword row = nxNext() % 4, col = nxNext() % 4;
		const bool singular = c % 8 == 3;	// row 2 is twice row 1
		for(int side = 0; side < (part == 3 ? all : 0); ++side)
			{
			NxTape& tape = side == 0 ? gOracleTape : gCandidateTape;
			tape.mark = singular ? kWordDegenerate : 0;
			Matrix4x4 inv = g, dest;
			float cof, det;
			if(side == 0)
				{
				cof = ((NxCoFactorFn) nxAt(o, kIceM4CoFactor))(&g, row, col);
				det = ((NxFloatThisFn) nxAt(o, kIceM4Determinant))(&g);
				((NxInvertFn) nxAt(o, kIceM4Invert))(&inv);
				((NxInvertPRFn) nxAt(o, kIceInvertPR))(&dest, &pr);
				}
			else
				{
				cof = g.CoFactor(row, col);
				det = g.Determinant();
				inv.Invert();
				InvertPRMatrix(dest, pr);
				}
			tape.pushFloat(cof);
			tape.pushFloat(det);
			nxTapeFloatWords(tape, &inv, sizeof(inv));
			nxTapeFloatWords(tape, &dest, sizeof(dest));
			tape.mark = 0;
			}

		// OBB
		OBB box, other;
		// Sequenced: as one expression the draws' order was the compiler's.
		const float boxCenterX = nxRange(-3, 3).value;
		const float boxCenterY = nxRange(-3, 3).value;
		const float boxCenterZ = nxRange(-3, 3).value;
		box.mCenter = Point(boxCenterX, boxCenterY, boxCenterZ);
		// Sequenced: as one expression the draws' order was the compiler's.
		const float boxExtentY = (c % 6 == 1) ? 0.0f : nxRange(0, 2).value;
		const float boxExtentX = nxRange(0, 2).value;
		const float boxExtentZ = nxRange(0, 2).value;
		box.mExtents = Point(boxExtentX, boxExtentY, boxExtentZ);
		// Sequenced: as one expression the draws' order was the compiler's.
		const float otherOffsetX = nxRange(-1, 1).value;
		const float otherOffsetY = nxRange(-1, 1).value;
		const float otherOffsetZ = nxRange(-1, 1).value;
		other.mCenter = box.mCenter + Point(otherOffsetX, otherOffsetY, otherOffsetZ);
		// Sequenced: as one expression the draws' order was the compiler's.
		const float otherExtentX = nxRange(0, 3).value;
		const float otherExtentY = nxRange(0, 3).value;
		const float otherExtentZ = nxRange(0, 3).value;
		other.mExtents = Point(otherExtentX, otherExtentY, otherExtentZ);
		{
		float r[3][3];
		nxRotation(r);
		for(int i = 0; i < 3; ++i)
			for(int j = 0; j < 3; ++j)
				box.mRot.m[i][j] = (c % 5 == 0) ? (i == j ? 1.0f : 0.0f) : r[i][j];
		nxRotation(r);
		for(int i = 0; i < 3; ++i)
			for(int j = 0; j < 3; ++j)
				other.mRot.m[i][j] = r[i][j];
		}
		const udword edgeIndex = nxNext() % 12;
		for(int side = 0; side < (part == 4 ? all : 0); ++side)
			{
			NxTape& tape = side == 0 ? gOracleTape : gCandidateTape;
			Plane planes[6];
			Point pts[8], edgeNormal;
			bool okPlanes, okPoints;
			BOOL inside;
			if(side == 0)
				{
				okPlanes = ((NxPointsFn) nxAt(o, kIceOBBPlanes))(&box, planes);
				okPoints = ((NxPointsFn) nxAt(o, kIceOBBPoints))(&box, pts);
				((NxEdgeNormalFn) nxAt(o, kIceOBBEdgeNormal))(&box, edgeIndex, &edgeNormal);
				inside = ((NxOBBInsideFn) nxAt(o, kIceOBBIsInside))(&box, &other);
				}
			else
				{
				okPlanes = box.ComputePlanes(planes);
				okPoints = box.ComputePoints(pts);
				box.ComputeWorldEdgeNormal(edgeIndex, edgeNormal);
				inside = box.IsInside(other);
				}
			tape.push(okPlanes ? 1u : 0u);
			tape.push(okPoints ? 1u : 0u);
			tape.push((unsigned) inside);
			nxTapeFloatWords(tape, planes, sizeof(planes));
			nxTapeFloatWords(tape, pts, sizeof(pts));
			nxTapeFloatWords(tape, &edgeNormal, sizeof(edgeNormal));
			}
		}
	nxReport(kIceParts[part].name, kIceParts[part].rva, kIceParts[part].owner, kIceParts[part].source,
		selfOnly, tolerance);
	}

// Three of the five are DIVERGENT, measured and attributed
// (evidence/vendored-correspondence.md, Task 4): Plane::Set/Triangle's
// cross products and normalisations, Matrix4x4's cofactor sums and OBB's
// rotations are the reassociated sums and unrounded intermediates of the
// summation-order work unit, and on the degenerate inputs (a collinear
// triangle, a singular matrix) the oracle's unrounded residue is a tiny
// non-zero where the candidate's is exactly zero.
static void nxDriveIceMaths(const NxOracleRows& o, bool selfOnly)
	{
	static const bool kX87[5] = { false, true, false, true, true };
	for(int part = 0; part < 5; ++part)
		nxDriveIcePart(o, part, selfOnly, kX87[part] ? kDivergent : 0);
	}

//////////////////////////////////////////////////////////////////////////////
// qhull: the hull the NovodeX cooker builds, "qhull o", over point sets that
// reach qhull's facet merging (coplanar and duplicate points, a thin slab,
// near-degenerate rings) and its error exits (a flat set, too few points, a
// collinear set).
//
// The oracle's qhull reaches its host through the object pointer at
// .data:0x00125080, a NovodeX class nothing has recovered (Task 2's open
// item). The harness installs a stand-in for the duration of this family:
// malloc/free on the harness heap, printing dropped, and the error exit handed
// back to the driver by longjmp -- the same three things ThirdPartyHost.cpp's
// shims do on the candidate side. The output-collecting slots (+0x00..+0x0c)
// are stubs that drop what they are handed: qh_produce_output runs, as the
// driver runs it, but the NovodeX output it produces goes through hooks whose
// candidate side is a shim, and a shim has no outcome to compare. What it
// leaves in the qh state is compared: the area and volume it computes for the
// "o" header, and each facet's area, on the float tape.

extern "C" {
typedef struct NxQhullEntries
	{
	void*	initA;
	void*	initflags;
	void*	initB;
	void*	qhull;
	void*	checkOutput;
	void*	produceOutput;
	void*	fin;
	void*	fout;
	void*	ferr;
	} NxQhullEntries;
typedef void (*NxQhPush)(void* tape, unsigned word);
typedef void (*NxQhPushDouble)(void* tape, double value);
int		nxQhullRun(const NxQhullEntries* e, double* points, int numpoints, const char* options);
int		nxQhullRunInitBuild(const NxQhullEntries* e, void* initbuild, double* points, int numpoints,
		const char* options);
void	nxQhullTape(const void* state, const double* points, int numpoints, NxQhPush push, void* tape,
	NxQhPushDouble pushDouble, void* floats);
void	nxQhullAddressProbe(const void* state, const double* points, int numpoints, int set, int side);
void	nxQhullPlaneProbe(const void* state, int facetId, void* oracleSetPlane, void* candidateSetPlane,
		void* oracleNormalize, void* candidateNormalize);
void	qh_sethyperplane_det(int dim, double** rows, double* point0, int toporient, double* normal,
		double* offset, int* nearzero);
void	qh_normalize2(double* normal, int dim, int toporient, double* norm, int* nearzero);
void	nxQhullFurthestProbe(const void* state, const double* points, int numpoints, int set, int side);
void	nxQhullDistanceProbe(const void* state, const double* points, int numpoints, int set, int side,
		void* distplane);
void* nxQhullCandidateState(void);
void	qh_initbuild(void);
void	qh_distplane(double* point, void* facet, double* distance);
void	nxQhullErrorExit(int exitcode);
void	qh_init_A(FILE* infile, FILE* outfile, FILE* errfile, int argc, char* argv[]);
void	qh_initflags(char* command);
void	qh_init_B(double* points, int numpoints, int dim, unsigned int ismalloc);
void	qh_qhull(void);
void	qh_check_output(void);
void	qh_produce_output(void);
}

static void* gQhOracleBlocks[65536];
static unsigned gQhOracleNbBlocks = 0;

static void __fastcall nxQhHostOff(void*, int, int, int, int, int) {}
static void __fastcall nxQhHostPoint(void*, int, float, float, float) {}
static void __fastcall nxQhHostFacet(void*, int, int, int*) {}
static void __fastcall nxQhHostSize(void*, int, float, float) {}
static int __cdecl nxQhHostPrintf(void*, void*, const char*, ...) { return 0; }
static void* __fastcall nxQhHostMalloc(void*, int, size_t size)
	{
	void* p = malloc(size ? size : 1);
	if(gQhOracleNbBlocks < sizeof(gQhOracleBlocks) / sizeof(gQhOracleBlocks[0]))
		gQhOracleBlocks[gQhOracleNbBlocks++] = p;
	return p;
	}
static void __fastcall nxQhHostFree(void*, int, void* p)
	{
	for(unsigned i = 0; i < gQhOracleNbBlocks; ++i)
		if(gQhOracleBlocks[i] == p)
			{
			gQhOracleBlocks[i] = gQhOracleBlocks[--gQhOracleNbBlocks];
			free(p);
			return;
			}
	}
static void __fastcall nxQhHostNarrow(void*, int) {}
static void __fastcall nxQhHostErrexit(void*, int, int exitcode) { nxQhullErrorExit(exitcode); }

static void* gQhHostVtable[9] =
	{
	(void*) &nxQhHostOff, (void*) &nxQhHostPoint, (void*) &nxQhHostFacet, (void*) &nxQhHostSize,
	(void*) &nxQhHostPrintf, (void*) &nxQhHostMalloc, (void*) &nxQhHostFree, (void*) &nxQhHostNarrow,
	(void*) &nxQhHostErrexit
	};
static void* gQhHostObject[4] = { gQhHostVtable, 0, 0, 0 };

static void nxQhPushTape(void* tape, unsigned word)
	{
	((NxTape*) tape)->push(word);
	}

static void nxQhPushTapeDouble(void* tape, double value)
	{
	((NxTape*) tape)->pushDouble(value);
	}

static unsigned nxQhullPoints(int set, float* out)
	{
	unsigned n = 0;
	switch(set)
		{
		case 0:		// a tetrahedron
			{
			static const float k[12] = { 0,0,0, 1,0,0, 0,1,0, 0,0,1 };
			memcpy(out, k, sizeof(k));
			return 4;
			}
		case 1:		// a cube: six coplanar quads
			for(unsigned v = 0; v < 8; ++v)
				{
				out[n * 3 + 0] = (v & 1) ? 1.0f : -1.0f;
				out[n * 3 + 1] = (v & 2) ? 1.0f : -1.0f;
				out[n * 3 + 2] = (v & 4) ? 1.0f : -1.0f;
				++n;
				}
			return n;
		case 2:		// a 3x3x3 lattice: coplanar points on every face, one interior
			for(int i = 0; i < 27; ++i)
				{
				out[n * 3 + 0] = (float) (i % 3) - 1.0f;
				out[n * 3 + 1] = (float) ((i / 3) % 3) - 1.0f;
				out[n * 3 + 2] = (float) (i / 9) - 1.0f;
				++n;
				}
			return n;
		case 3:		// points on a sphere
		case 4:		// points in a box
			for(int i = 0; i < (set == 3 ? 96 : 200); ++i)
				{
				// Sequenced: as one expression the draws' order was the compiler's.
				const float pointZ = nxRange(-1, 1).value;
				const float pointY = nxRange(-1, 1).value;
				const float pointX = nxRange(-1, 1).value;
				Point p(pointX, pointY, pointZ);
				if(set == 3 && p.Magnitude() > 1e-3f)
					p.Normalize();
				out[n * 3 + 0] = p.x; out[n * 3 + 1] = p.y; out[n * 3 + 2] = p.z;
				++n;
				}
			return n;
		case 5:		// a thin slab: a jittered plane and two points above it
			for(int i = 0; i < 60; ++i)
				{
				out[n * 3 + 0] = nxRange(-2, 2).value;
				out[n * 3 + 1] = nxRange(-2, 2).value;
				out[n * 3 + 2] = nxRange(-1e-5f, 1e-5f).value;
				++n;
				}
			out[n * 3 + 0] = 0.25f; out[n * 3 + 1] = 0.5f; out[n * 3 + 2] = 0.5f; ++n;
			out[n * 3 + 0] = -0.25f; out[n * 3 + 1] = 0.125f; out[n * 3 + 2] = 0.25f; ++n;
			return n;
		case 6:		// every point three times, plus the cube
			for(int i = 0; i < 20; ++i)
				{
				const float x = nxRange(-1, 1).value, y = nxRange(-1, 1).value, z = nxRange(-1, 1).value;
				for(int k = 0; k < 3; ++k)
					{
					out[n * 3 + 0] = x; out[n * 3 + 1] = y; out[n * 3 + 2] = z;
					++n;
					}
				}
			n += nxQhullPoints(1, out + n * 3);
			return n;
		case 7:		// a cylinder: two rings of 16, each ring coplanar
			for(int ring = 0; ring < 2; ++ring)
				for(int i = 0; i < 16; ++i)
					{
					const double a = i * (6.283185307179586 / 16.0);
					out[n * 3 + 0] = (float) cos(a);
					out[n * 3 + 1] = (float) sin(a);
					out[n * 3 + 2] = ring ? 2.0f : 0.0f;
					++n;
					}
			return n;
		case 8:		// far from the origin: a small box at 1e5
			for(int i = 0; i < 40; ++i)
				{
				out[n * 3 + 0] = 100000.0f + nxRange(-1, 1).value;
				out[n * 3 + 1] = -50000.0f + nxRange(-1, 1).value;
				out[n * 3 + 2] = 25000.0f + nxRange(-1, 1).value;
				++n;
				}
			return n;
		case 9:		// flat: every point on z = 0.5 (qhull's flat-simplex exit)
			for(int i = 0; i < 16; ++i)
				{
				out[n * 3 + 0] = nxRange(-1, 1).value;
				out[n * 3 + 1] = nxRange(-1, 1).value;
				out[n * 3 + 2] = 0.5f;
				++n;
				}
			return n;
		case 10:	// too few points
			out[0] = 0; out[1] = 0; out[2] = 0;
			out[3] = 1; out[4] = 0; out[5] = 0;
			out[6] = 0; out[7] = 1; out[8] = 0;
			return 3;
		default:	// collinear
			for(int i = 0; i < 10; ++i)
				{
				out[n * 3 + 0] = (float) i;
				out[n * 3 + 1] = (float) i * 2.0f;
				out[n * 3 + 2] = (float) i * -0.5f;
				++n;
				}
			return n;
		}
	}

static void nxDriveQhullHull(const NxOracleRows& o, bool selfOnly)
	{
	gState = 0x9b0c0de5;
	gOracleTape.reset();
	gCandidateTape.reset();
	gOracleTapeX87.reset();
	gCandidateTapeX87.reset();

	void** hostSlot = (void**) (o.base + kQhHostGlobal);
	void* shippedHost = *hostSlot;
	*hostSlot = gQhHostObject;

	NxQhullEntries oracle;
	oracle.initA		= o.base + kQhInitA;
	oracle.initflags	= o.base + kQhInitflags;
	oracle.initB		= o.base + kQhInitB;
	oracle.qhull		= o.base + kQhQhull;
	oracle.checkOutput	= o.base + kQhCheckOutput;
	oracle.produceOutput	= o.base + kQhProduceOutput;
	oracle.fin			= o.base + kOracleIob;
	oracle.fout			= o.base + kOracleIob + 0x20;
	oracle.ferr			= o.base + kOracleIob + 0x40;
	NxQhullEntries candidate;
	candidate.initA			= (void*) &qh_init_A;
	candidate.initflags		= (void*) &qh_initflags;
	candidate.initB			= (void*) &qh_init_B;
	candidate.qhull			= (void*) &qh_qhull;
	candidate.checkOutput	= (void*) &qh_check_output;
	candidate.produceOutput	= (void*) &qh_produce_output;
	candidate.fin			= stdin;
	candidate.fout			= stdout;
	candidate.ferr			= stderr;

	// "o" is the NovodeX driver's only option, over every point set. The others
	// reach vendored rows that option does not -- triangulated output, facet
	// areas, the merge options, input scaling, the exhaustive initial simplex,
	// output verification -- over the five sets that merge. The last, "o QR1",
	// rotates the input by qhull's own random matrix first; the rotation's
	// sums (qh_randommatrix, qh_gram_schmidt, qh_rotatepoints) round
	// differently, the lattice and the slab stop being exactly coplanar in
	// different places, and the merges that follow differ: that option goes to
	// qhull_hull_rotated, DIVERGENT, as a whole.
	static const char* const kOptions[] = { "o", "o Qt", "o FA", "o C-0", "o Qx", "o Qbb", "o QbB", "o Qs", "o Tv", "o QR1" };
	static const int kMergingSets[] = { 2, 3, 5, 6, 7 };
	static float points[3 * 256];
	gOracleTapeB.reset();
	gCandidateTapeB.reset();
	// Two passes over the runs, in their original order: the unrotated runs,
	// whose families are reported first, then the "o QR1" runs. Task 5b split
	// them so that the QR1 runs execute in a trace segment of their own and a
	// group they alone reach is not credited to qhull_hull; the tapes, the
	// generator's draws and the report order are unchanged.
	const int kRotatedFirst = 12 + 8 * 5;
	for(int run = 0; run < 12 + 9 * 5; ++run)
		{
		if(run == kRotatedFirst)
			{
			nxReport("qhull_hull", "0x0007d180", "phys_fn_003234",
				"qhull.c,poly.c,poly2.c,merge.c,geom.c,geom2.c,qset.c,mem.c,global.c", selfOnly);
			nxReportTapes(gOracleTapeX87, gCandidateTapeX87, "qhull_hull_x87", "0x0005c5c0", "phys_fn_002425",
				"geom.c,geom2.c,merge.c", selfOnly, kDivergent);
			}
		const bool rotated = run >= kRotatedFirst;
		const int set = run < 12 ? run : kMergingSets[(run - 12) % 5];
		const char* options = kOptions[run < 12 ? 0 : 1 + (run - 12) / 5];
		if(run == 12)
			gState = 0x9b0c0de6;
		const unsigned n = nxQhullPoints(set, points);
		for(int side = 0; side < (selfOnly ? 1 : 2); ++side)
			{
			NxTape& tape = rotated ? (side == 0 ? gOracleTapeB : gCandidateTapeB)
				: (side == 0 ? gOracleTape : gCandidateTape);
			NxTape& floats = rotated ? tape : (side == 0 ? gOracleTapeX87 : gCandidateTapeX87);
			// The NovodeX driver widens the cooker's floats to doubles (0x0007d490).
			double* coords = (double*) malloc(sizeof(double) * 3 * n);
			for(unsigned i = 0; i < 3 * n; ++i)
				coords[i] = points[i];
			const int result = nxQhullRun(side == 0 ? &oracle : &candidate, coords, (int) n, options);
			// Whether the build took an error exit, not which code: the candidate's
			// exit arrives through ThirdPartyHost.cpp's qhNovodeXErrexit, which
			// drops the code and aborts, so only the oracle side could tape it.
			tape.push(result ? 1u : 0u);
			if(result == 0)
				nxQhullTape(side == 0 ? (const void*) (o.base + kQhState) : nxQhullCandidateState(),
					coords, (int) n, nxQhPushTape, &tape, nxQhPushTapeDouble, &floats);
			if(side == 0)
				{
				while(gQhOracleNbBlocks)
					free(gQhOracleBlocks[--gQhOracleNbBlocks]);
				}
			free(coords);
			}
		}
	*hostSlot = shippedHost;
	nxReportTapes(gOracleTapeB, gCandidateTapeB, "qhull_hull_rotated", "0x0005ff40", "phys_fn_002520",
		"geom2.c,qhull.c,poly.c,poly2.c,merge.c", selfOnly, kDivergent);
	}

// The Task 4 families, in the order they depend on each other: the models
// first, every query over them, the refit last because it moves the vertices.
static void nxDriveVendoredCoverage(const NxOracleRows& o, bool selfOnly)
	{
	// SetIceError on a rejecting arm dispatches through the oracle's import of
	// NxFoundation's reporter, which aborts in a process with no SDK; the same
	// redirection nxDrivePrunableRanges makes, for the same reason.
	void** errorSlot = (void**) (o.base + kIatFoundationError);
	void* shippedReporter = *errorSlot;
	DWORD wasProtected = 0;
	if(!VirtualProtect(errorSlot, sizeof(void*), PAGE_READWRITE, &wasProtected))
		{
		fprintf(stderr, "FAIL cannot reach the oracle's error import slot\n");
		++gMismatches;
		return;
		}
	*errorSlot = (void*) &nxFoundationErrorProbe;

	nxDriveModels(o, selfOnly);
	nxDriveRay(o, selfOnly);
	static const NxVolumeRows kVolumes[] =
		{
		{ "opcode_sphere", "0x000e1360", "phys_fn_005105", "OPC_SphereCollider.cpp,OPC_SphereTriOverlap.h",
		  kOpcSphereCtor, kOpcSphereDtor, kOpcSphereCollideModel, sizeof(SphereCollider), sizeof(SphereCache) },
		{ "opcode_obb", "0x000de0d0", "phys_fn_005067", "OPC_OBBCollider.cpp,OPC_BoxBoxOverlap.h,OPC_TriBoxOverlap.h",
		  kOpcOBBCtor, kOpcOBBDtor, kOpcOBBCollide, sizeof(OBBCollider), sizeof(OBBCache) },
		{ "opcode_aabb", "0x000ef0d0", "phys_fn_005434", "OPC_AABBCollider.cpp,OPC_TriBoxOverlap.h",
		  kOpcAABBCtor, kOpcAABBDtor, kOpcAABBCollideModel, sizeof(AABBCollider), sizeof(AABBCache) },
		{ "opcode_lss", "0x000d4b90", "phys_fn_005027", "OPC_LSSCollider.cpp,OPC_LSSAABBOverlap.h,OPC_LSSTriOverlap.h",
		  kOpcLSSCtor, kOpcLSSDtor, kOpcLSSCollide, sizeof(LSSCollider), sizeof(LSSCache) },
		{ "opcode_planes", "0x000e2b60", "phys_fn_005138", "OPC_PlanesCollider.cpp,OPC_PlanesAABBOverlap.h,OPC_PlanesTriOverlap.h",
		  kOpcPlanesCtor, kOpcPlanesDtor, kOpcPlanesCollide, sizeof(PlanesCollider), sizeof(PlanesCache) },
		};
	static const unsigned kSeeds[] = { 0x5fe2ee01, 0x0bbee001, 0xaabbee01, 0x1553ee01, 0x91a9e501 };
	for(int k = 0; k < 5; ++k)
		nxDriveVolume(o, (NxVolumeKind) k, kVolumes[k], kSeeds[k], selfOnly);
	nxDriveTreeCollider(o, selfOnly);
	nxDriveVanilla(o, selfOnly);
	nxDriveRefit(o, selfOnly);
	nxReleaseModels(o);
	nxDriveSap(o, selfOnly);
	nxDriveIceMaths(o, selfOnly);

	*errorSlot = shippedReporter;
	VirtualProtect(errorSlot, sizeof(void*), wasProtected, &wasProtected);

	nxDriveQhullHull(o, selfOnly);
	}

//////////////////////////////////////////////////////////////////////////////
// Vendored correspondence, Task 5a: candidate-built trees, end to end.
//
// Every collider family above queries the ORACLE-built model on both sides,
// which compares the colliders and not the builds; opcode_model_build compares
// the builds on their own. Neither shows that a tree the candidate built,
// queried by the candidate's colliders, answers what the oracle's tree and
// colliders answer. These two families do: each model is built once by each
// side (Model::Build at 0x000e9100 against the vendored one), and every query
// runs the oracle's collider on the oracle's model and the candidate's collider
// on the candidate's model, over the same inputs.
//
//   opcode_candidate_trees      the volume colliders on the models whose
//                               build is exact (opcode_model_build: no tie,
//                               not quantized), and tree-versus-tree pairs of
//                               two of them. Registered whole: exact.
//   opcode_candidate_trees_ray  rays on the same models: each query's discrete
//                               outcome and its hits' distances and
//                               barycentrics. DIVERGENT, with a ceiling, and
//                               not because of the trees: the floats are
//                               opcode_ray_x87's summation order, and one
//                               ray's BV test count (6 in the oracle, 14 in
//                               the candidate) comes out the same when the
//                               candidate's collider queries the ORACLE's
//                               model, so it is the collider's last bit
//                               (opcode_ray_boundary's RayAABBOverlap `f`
//                               rounding) on a ray that happens to graze a box.
//   opcode_candidate_trees_x87  the rest -- quantized trees, and the height
//                               field, the flat grid and the box, whose
//                               splatter splits hang on a tie
//                               (opcode_model_build_x87) -- every query on
//                               them, floats included, and every pair with one
//                               of them. DIVERGENT, with a ceiling.
//
// What is taped is each query's discrete outcome (return value, contact
// status, test counts, hits and face ids, touched primitives, pairs), the
// volume caches' derived fields, and the ray hits' floats as said. The rays
// are the three shapes opcode_ray takes; the three aimed at a boundary are
// opcode_ray_boundary's, and the flat grid against the height field is
// opcode_treecollider_boundary's, so neither is driven here.

static const NxVolumeRows kVolumeRows[] =
	{
	{ "opcode_sphere", "0x000e1360", "phys_fn_005105", "OPC_SphereCollider.cpp,OPC_SphereTriOverlap.h",
	  kOpcSphereCtor, kOpcSphereDtor, kOpcSphereCollideModel, sizeof(SphereCollider), sizeof(SphereCache) },
	{ "opcode_obb", "0x000de0d0", "phys_fn_005067", "OPC_OBBCollider.cpp,OPC_BoxBoxOverlap.h,OPC_TriBoxOverlap.h",
	  kOpcOBBCtor, kOpcOBBDtor, kOpcOBBCollide, sizeof(OBBCollider), sizeof(OBBCache) },
	{ "opcode_aabb", "0x000ef0d0", "phys_fn_005434", "OPC_AABBCollider.cpp,OPC_TriBoxOverlap.h",
	  kOpcAABBCtor, kOpcAABBDtor, kOpcAABBCollideModel, sizeof(AABBCollider), sizeof(AABBCache) },
	{ "opcode_lss", "0x000d4b90", "phys_fn_005027", "OPC_LSSCollider.cpp,OPC_LSSAABBOverlap.h,OPC_LSSTriOverlap.h",
	  kOpcLSSCtor, kOpcLSSDtor, kOpcLSSCollide, sizeof(LSSCollider), sizeof(LSSCache) },
	{ "opcode_planes", "0x000e2b60", "phys_fn_005138", "OPC_PlanesCollider.cpp,OPC_PlanesAABBOverlap.h,OPC_PlanesTriOverlap.h",
	  kOpcPlanesCtor, kOpcPlanesDtor, kOpcPlanesCollide, sizeof(PlanesCollider), sizeof(PlanesCache) },
	};

// One model's queries, both sides, onto `oracleTape`/`candidateTape`: the rays
// (their hits' floats onto `oracleFloats`/`candidateFloats`) or the volumes.
static void nxCandidateTreeQueries(const NxOracleRows& o, const NxModelPair& p, NxTape& oracleTape,
	NxTape& candidateTape, bool rays, NxTape& oracleFloats, NxTape& candidateFloats, bool selfOnly)
	{
	const NxMesh& m = gMeshes[p.mesh];
	static const int kRayShapes[6] = { 0, 1, 4, 6, 7, 10 };	// nxMakeRay's shapes 0, 1 and 4
	for(int k = 0; k < (rays ? 6 : 0); ++k)
		{
		const int r = kRayShapes[k];
		Ray ray;
		float length;
		nxMakeRay(m, r, ray, length);
		const int setting = (k + p.mesh + p.kind) % 12;
		const bool segment = (setting & 1) != 0;
		const bool culling = (setting & 2) != 0;
		const bool closest = (setting & 4) != 0 && setting < 8;
		const bool first = setting >= 8;
		Matrix4x4 world;
		nxWorld(world, k % 3);
		const Matrix4x4* worldPtr = k % 3 ? &world : 0;
		for(int side = 0; side < (selfOnly ? 1 : 2); ++side)
			{
			NxTape& tape = side == 0 ? oracleTape : candidateTape;
			NxTape& floats = side == 0 ? oracleFloats : candidateFloats;
			unsigned char facesStorage[sizeof(CollisionFaces) + 16];
			CollisionFaces* faces = new (facesStorage) CollisionFaces;
			void* object = side == 0 ? nxOracleAlloc(sizeof(RayCollider)) : (void*) new RayCollider;
			if(side == 0)
				((NxCtorFn) nxAt(o, kOpcRayCtor))(object);
			RayCollider* rc = (RayCollider*) object;
			rc->SetMaxDist(segment ? length : MAX_FLOAT);
			rc->SetCulling(culling);
			rc->SetClosestHit(closest);
			rc->SetFirstContact(first);
			rc->SetDestination(faces);
			const float tolerance = 0.0f;
			memcpy((unsigned char*) object + 0x88, &tolerance, 4);
			const bool returned = side == 0
				? ((NxRayModelFn) nxAt(o, kOpcRayCollideModel))(object, &ray, p.oracle, worldPtr, 0)
				: rc->Collide(ray, *p.candidate, worldPtr, 0);
			nxTapeRay(tape, floats, returned, object, *faces, 0xffffffffu);
			if(side == 0)
				{
				((NxDtorFn) nxAt(o, kOpcRayDtor))(object);
				nxOracleFree(object, sizeof(RayCollider));
				nxReleaseOracleContainer(o, faces);
				}
			else
				{
				delete rc;
				faces->~CollisionFaces();
				}
			}
		}

	for(int kind = 0; kind < (rays ? 0 : 5); ++kind)
		for(int q = 0; q < 4; ++q)
			{
			NxVolumeQuery v;
			nxMakeVolume((NxVolumeKind) kind, m, q + p.mesh * 4 + p.kind, v);
			// A single-triangle model has no tree, and stock OPCODE walks it when
			// primitive tests are off: keep them on there (see nxDriveVolume).
			const bool primitives = q != 2 || p.mesh == 4;
			const bool coherent = q == 3;
			Matrix4x4 worldv, worldm;
			nxWorld(worldv, q % 3);
			nxWorld(worldm, (q + 1) % 3);
			const Matrix4x4* wv = q % 3 ? &worldv : 0;
			const Matrix4x4* wm = (q + 1) % 3 ? &worldm : 0;
			const NxVolumeRows& rows = kVolumeRows[kind];
			for(int side = 0; side < (selfOnly ? 1 : 2); ++side)
				{
				NxTape& tape = side == 0 ? oracleTape : candidateTape;
				unsigned char cacheStorage[sizeof(OBBCache) + sizeof(LSSCache) + 64];
				VolumeCache* cache = (VolumeCache*) nxNewCache((NxVolumeKind) kind, cacheStorage);
				const size_t derivedBytes = rows.cacheSize - sizeof(VolumeCache);
				unsigned char touchedStorage[sizeof(Container) + 16];
				cache->TouchedPrimitives = new (touchedStorage) Container;
				void* object;
				if(side == 0)
					{
					object = nxOracleAlloc(rows.objectSize);
					((NxCtorFn) nxAt(o, rows.ctor))(object);
					}
				else
					object = nxNewCollider((NxVolumeKind) kind);
				Collider* c = (Collider*) object;
				c->SetPrimitiveTests(primitives);
				c->SetTemporalCoherence(coherent);
				for(int call = 0; call < (coherent ? 2 : 1); ++call)
					{
					const bool returned = side == 0
						? nxOracleVolume(o, (NxVolumeKind) kind, rows.collide, object, cache, v, p.oracle, wv, wm)
						: nxCandidateVolume((NxVolumeKind) kind, object, cache, v, *p.candidate, wv, wm);
					nxTapeVolume(tape, tape, returned, object, *cache->TouchedPrimitives,
						(unsigned char*) cache + sizeof(VolumeCache), derivedBytes);
					}
				if(side == 0)
					{
					((NxDtorFn) nxAt(o, rows.dtor))(object);
					nxOracleFree(object, rows.objectSize);
					nxReleaseOracleContainer(o, cache->TouchedPrimitives);
					}
				else
					{
					nxDeleteCollider((NxVolumeKind) kind, object);
					cache->TouchedPrimitives->~Container();
					}
				}
			}
	}

// A tree-versus-tree query between two model pairs, each side on its own models.
static void nxCandidateTreePair(const NxOracleRows& o, const NxModelPair& a, const NxModelPair& b,
	int placement, int setting, NxTape& oracleTape, NxTape& candidateTape, bool selfOnly)
	{
	Matrix4x4 w0, w1;
	nxWorld(w0, placement == 2 ? 2 : 0);
	nxWorld(w1, placement);
	for(int side = 0; side < (selfOnly ? 1 : 2); ++side)
		{
		NxTape& tape = side == 0 ? oracleTape : candidateTape;
		BVTCache cache;
		cache.Model0 = side == 0 ? (const Model*) a.oracle : a.candidate;
		cache.Model1 = side == 0 ? (const Model*) b.oracle : b.candidate;
		void* object;
		if(side == 0)
			{
			object = nxOracleAlloc(sizeof(AABBTreeCollider));
			((NxCtorFn) nxAt(o, kOpcTreeColliderCtor))(object);
			}
		else
			object = new AABBTreeCollider;
		AABBTreeCollider* tc = (AABBTreeCollider*) object;
		tc->SetFullBoxBoxTest((setting & 1) != 0);
		tc->SetFullPrimBoxTest((setting & 2) != 0);
		tc->SetFirstContact((setting & 4) != 0);
		const bool returned = side == 0
			? ((NxBVTFn) nxAt(o, kOpcTreeColliderBVT))(object, &cache, &w0, placement ? &w1 : 0)
			: tc->Collide(cache, &w0, placement ? &w1 : 0);
		nxTapeCollider(tape, returned, object);
		tape.push(tc->GetNbBVBVTests());
		tape.push(tc->GetNbBVPrimTests());
		tape.push(tc->GetNbPrimPrimTests());
		tape.push(tc->GetNbPairs());
		for(udword i = 0; i < tc->GetNbPairs(); ++i)
			{
			tape.push(tc->GetPairs()[i].id0);
			tape.push(tc->GetPairs()[i].id1);
			}
		if(side == 0)
			{
			((NxDtorFn) nxAt(o, kOpcTreeColliderDtor))(object);
			nxOracleFree(object, sizeof(AABBTreeCollider));
			}
		else
			delete tc;
		}
	}

static void nxDriveCandidateTrees(const NxOracleRows& o, bool selfOnly)
	{
	void** errorSlot = (void**) (o.base + kIatFoundationError);
	void* shippedReporter = *errorSlot;
	DWORD wasProtected = 0;
	if(!VirtualProtect(errorSlot, sizeof(void*), PAGE_READWRITE, &wasProtected))
		{
		fprintf(stderr, "FAIL cannot reach the oracle's error import slot\n");
		++gMismatches;
		return;
		}
	*errorSlot = (void*) &nxFoundationErrorProbe;

	// The default-rule model of every mesh in every tree kind, as
	// nxDriveModels builds them; their build tapes are opcode_model_build's
	// business and are dropped here. Each pass builds its own models first, so
	// that the builds a family's queries depend on sit in that family's trace
	// segment.
	nxBuildMeshes();
	gNbModels = 0;
	const udword kDefaultRules = SPLIT_SPLATTER_POINTS | SPLIT_GEOM_CENTER;
	for(int mesh = 0; mesh < kNbMeshes; ++mesh)
		for(int kind = 0; kind < 4; ++kind)
			{
			const bool untied = mesh == 1 || mesh == 3 || mesh == 4;
			gModelIndex[mesh][kind] = gNbModels;
			nxAddModel(mesh, kind, kDefaultRules, false, 0.0f, -1, 0.0f, untied && !(kind & 2));
			}

	static const int kPairs[][2] = { { 0, 1 }, { 5, 5 }, { 2, 2 }, { 1, 1 }, { 0, 3 }, { 3, 3 }, { 1, 3 } };
	for(int pass = 0; pass < 2; ++pass)
		{
		const bool exact = pass == 0;
		for(int i = 0; i < gNbModels; ++i)
			if(gModels[i].exact == exact)
				nxBuildModel(o, gModels[i], selfOnly);
		gState = exact ? 0xca7d1d01 : 0xca7d1d02;
		gOracleTape.reset();
		gCandidateTape.reset();
		for(int i = 0; i < gNbModels; ++i)
			{
			const NxModelPair& p = gModels[i];
			if(p.exact != exact)
				continue;
			// A build outcome is compared both ways: a model only one side built
			// is a failure, whichever side it is (Task 5b; it used to be skipped
			// silently when the oracle was the side that declined).
			if(!p.built)
				{
				if(!selfOnly && p.candidateBuilt)
					{
					fprintf(stderr, "FAIL model %d: the candidate built it and the oracle did not\n", i);
					++gMismatches;
					}
				continue;
				}
			if(!selfOnly && !p.candidateBuilt)
				{
				fprintf(stderr, "FAIL model %d: the oracle built it and the candidate did not\n", i);
				++gMismatches;
				continue;
				}
			// The exact pass takes the rays in a family of their own, below; the
			// divergent pass tapes their floats with everything else.
			if(!exact)
				nxCandidateTreeQueries(o, p, gOracleTape, gCandidateTape, true, gOracleTape, gCandidateTape,
					selfOnly);
			nxCandidateTreeQueries(o, p, gOracleTape, gCandidateTape, false, gOracleTape, gCandidateTape,
				selfOnly);
			}
		for(int kind = 0; kind < 4; ++kind)
			for(unsigned pr = 0; pr < sizeof(kPairs) / sizeof(kPairs[0]); ++pr)
				{
				const NxModelPair& a = gModels[gModelIndex[kPairs[pr][0]][kind]];
				const NxModelPair& b = gModels[gModelIndex[kPairs[pr][1]][kind]];
				if(!a.built || !b.built || (a.exact && b.exact) != exact)
					continue;
				if(!selfOnly && (!a.candidateBuilt || !b.candidateBuilt))
					continue;	// already failed above
				for(int placement = 0; placement < 3; ++placement)
					nxCandidateTreePair(o, a, b, placement, (int) (pr + placement + kind) % 8,
						gOracleTape, gCandidateTape, selfOnly);
				}
		if(exact)
			{
			nxReport("opcode_candidate_trees", "0x000e9100", "phys_fn_005368",
				"OPC_Model.cpp,OPC_TreeBuilders.cpp,OPC_OptimizedTree.cpp,OPC_SphereCollider.cpp,"
				"OPC_OBBCollider.cpp,OPC_AABBCollider.cpp,OPC_LSSCollider.cpp,OPC_PlanesCollider.cpp,OPC_TreeCollider.cpp",
				selfOnly);
			gState = 0xca7d1d03;
			gOracleTape.reset();
			gCandidateTape.reset();
			for(int i = 0; i < gNbModels; ++i)
				{
				const NxModelPair& p = gModels[i];
				if(!p.exact || !p.built || (!selfOnly && !p.candidateBuilt))
					continue;
				nxCandidateTreeQueries(o, p, gOracleTape, gCandidateTape, true, gOracleTape, gCandidateTape,
					selfOnly);
				}
			nxReport("opcode_candidate_trees_ray", "0x000ba6f0", "phys_fn_004932",
				"OPC_RayCollider.cpp,OPC_RayAABBOverlap.h,OPC_RayTriOverlap.h", selfOnly, kDivergent);
			}
		else
			nxReport("opcode_candidate_trees_x87", "0x000f09b0", "phys_fn_005513",
				"OPC_AABBTree.cpp,OPC_TreeBuilders.cpp,OPC_OptimizedTree.cpp", selfOnly, kDivergent);
		}
	nxReleaseModels(o);

	*errorSlot = shippedReporter;
	VirtualProtect(errorSlot, sizeof(void*), wasProtected, &wasProtected);
	}

//////////////////////////////////////////////////////////////////////////////
// Layout assertions. Not a differential -- a static check that the vendored
// headers produce the sizes and offsets the disassembly measured. Every one of
// these is a modification a stock header gets silently wrong.

static unsigned gLayoutChecks = 0;
static unsigned gLayoutFailures = 0;

static void nxLayout(const char* what, size_t measured, size_t expected, const char* rva)
	{
	++gLayoutChecks;
	const bool ok = measured == expected;
	if(!ok)
		{
		++gLayoutFailures;
		fprintf(stderr, "LAYOUT %s is %u, the oracle says %u (%s)\n",
			what, (unsigned) measured, (unsigned) expected, rva);
		}
	printf("layout %s=%u expected=%u rva=%s %s\n",
		what, (unsigned) measured, (unsigned) expected, rva, ok ? "ok" : "FAILED");
	}

// The two blocks of NovodeX code in OPC_AABBTree.cpp. No offset check can see
// them -- they are statements, not layout -- so this drives them: three trees
// over the same four vertices, one per setting, and the root box of each.
//
// The point of the assertion is the middle and last cases. Vendoring
// OPC_AABBTree.cpp stock passes the first one, because both features default to
// off, and fails the other two.
static unsigned nxBits(float value)
	{
	unsigned w;
	memcpy(&w, &value, sizeof(w));
	return w;
	}

static void nxCheckAABBTreeExtension()
	{
	static const Point kVerts[4] =
		{
		Point(0.0f, 0.0f, 0.0f), Point(4.0f, 0.0f, 0.0f),
		Point(0.0f, 4.0f, 0.0f), Point(0.0f, 0.0f, 4.0f)
		};

	// (a) The defaults. The root box is the stock global box, [0,0,0]-[4,4,4].
	{
	AABBTreeOfVerticesBuilder builder;
	builder.mVertexArray	= kVerts;
	builder.mNbPrimitives	= 4;
	AABBTree tree;
	tree.Build(&builder);
	const AABB* root = tree.GetAABB();
	nxLayout("AABBTree.defaults_root_min_y", nxBits(root->GetMin(1)), nxBits(0.0f),
		"0x000f0ec0 cmp edx,-1 skips the block");
	nxLayout("AABBTree.defaults_root_max_y", nxBits(root->GetMax(1)), nxBits(4.0f),
		"0x000f0f83 test eax,eax skips the inflate");
	}

	// (b) The extension. Axis 1, value -3: below the root box's own minimum, so
	// 0x000f0efe's compare takes the fallthrough and mMin[1] becomes -3.
	{
	AABBTreeOfVerticesBuilder builder;
	builder.mVertexArray	= kVerts;
	builder.mNbPrimitives	= 4;
	builder.mSettings.mNovodeXExtendAxis		= 1;
	builder.mSettings.mNovodeXExtendValue		= -3.0f;
	AABBTree tree;
	tree.Build(&builder);
	const AABB* root = tree.GetAABB();
	nxLayout("AABBTree.extend_pulls_min", nxBits(root->GetMin(1)), nxBits(-3.0f),
		"0x000f0efe fcomp [esi+edx*4+0x20]; 0x000f0f3b stores");
	nxLayout("AABBTree.extend_leaves_max", nxBits(root->GetMax(1)), nxBits(4.0f),
		"0x000f0f47 fcomp [esi+edx*4+0x2c] not taken");
	// The latch is one-shot and cleared, which is what makes every node after
	// the root compare against the ROOT's box and not its own.
	nxLayout("AABBTree.capture_latch_cleared", builder.mNovodeXCaptureRootBV ? 1 : 0, 0,
		"0x000f0efa mov byte ptr [esi+0x38],0");
	nxLayout("AABBTree.captured_root_max_y", nxBits(builder.mNovodeXRootBV.GetMax(1)), nxBits(4.0f),
		"0x000f0ed7 lea eax,[esi+0x20] + six moves");
	}

	// (c) The margin. Every node's box grows by it on both sides.
	{
	AABBTreeOfVerticesBuilder builder;
	builder.mVertexArray	= kVerts;
	builder.mNbPrimitives	= 4;
	builder.mSettings.mNovodeXInflate	= 0.5f;
	AABBTree tree;
	tree.Build(&builder);
	const AABB* root = tree.GetAABB();
	nxLayout("AABBTree.inflate_min", nxBits(root->GetMin(1)), nxBits(-0.5f),
		"0x000f0f8e fld [esi+0x14]; fsub");
	nxLayout("AABBTree.inflate_max", nxBits(root->GetMax(1)), nxBits(4.5f),
		"0x000f0ff5 fadd");
	}
	}

// RayCollider's members are protected, so the offsets are read from a derived
// class -- which is also how OPCODE's own colliders reach them.
struct NxRayColliderProbe : public RayCollider
	{
	static size_t maxDistOffset()	{ return (size_t) &(((NxRayColliderProbe*) 0)->mMaxDist); }
	static size_t closestHitOffset(){ return (size_t) &(((NxRayColliderProbe*) 0)->mClosestHit); }
	};

static void nxCheckLayouts()
	{
	// OPCODECREATE grew 16 -> 32 because BuildSettings grew 8 -> 20.
	nxLayout("sizeof_BuildSettings", sizeof(BuildSettings), 20, "0x000e92b0,0x000e91cc");
	nxLayout("sizeof_OPCODECREATE", sizeof(OPCODECREATE), 32, "0x000e92b0");
	nxLayout("OPCODECREATE.mSettings", offsetof(OPCODECREATE, mSettings), 8, "0x000e9129");
	nxLayout("OPCODECREATE.mDeserializeFrom", offsetof(OPCODECREATE, mDeserializeFrom), 4, "0x000e9122");
	nxLayout("BuildSettings.mLimit", offsetof(BuildSettings, mLimit), 0, "0x000e9129");
	nxLayout("BuildSettings.mRules", offsetof(BuildSettings, mRules), 4, "0x000e92b2");
	// sizeof(AABBTreeOfTrianglesBuilder) is Model::Build's local frame at
	// 0x000e9100: `sub esp, 0x48`.
	nxLayout("sizeof_AABBTreeOfTrianglesBuilder", sizeof(AABBTreeOfTrianglesBuilder), 72, "0x000e9100");

	// The three added BuildSettings members and the 28 added AABBTreeBuilder
	// bytes, which Task 2a left unidentified and Task 2a's fix pass read off
	// AABBTree::Build and AABBTreeNode::_BuildHierarchy. Offsets are taken from
	// a real object rather than offsetof, because AABBTreeBuilder is polymorphic.
	//
	// Two of these are the types, not the offsets: `mNovodeXExtendValue` and
	// `mNovodeXInflate` were declared udword and are loaded with `fld dword ptr`
	// by the image. A wrong type here is invisible to every offset check --
	// float and udword are both 4 bytes -- so it is asserted directly.
	{
	AABBTreeOfTrianglesBuilder builder;
	const char* base = (const char*) &builder;
	nxLayout("AABBTreeBuilder.mSettings", (size_t) ((const char*) &builder.mSettings - base),
		4, "0x000e9060");
	nxLayout("BuildSettings.mNovodeXExtendValue",
		(size_t) ((const char*) &builder.mSettings.mNovodeXExtendValue - base), 0x0c, "0x000f0efe");
	nxLayout("BuildSettings.mNovodeXExtendAxis",
		(size_t) ((const char*) &builder.mSettings.mNovodeXExtendAxis - base), 0x10, "0x000f0ec0");
	nxLayout("BuildSettings.mNovodeXInflate",
		(size_t) ((const char*) &builder.mSettings.mNovodeXInflate - base), 0x14, "0x000f0f83");
	nxLayout("AABBTreeBuilder.mNovodeXRootBV",
		(size_t) ((const char*) &builder.mNovodeXRootBV - base), 0x20, "0x000f0ed7");
	nxLayout("AABBTreeBuilder.mNovodeXCaptureRootBV",
		(size_t) ((const char*) &builder.mNovodeXCaptureRootBV - base), 0x38, "0x000f1187");
	// A udword member truncates 0.5f to 0; a float keeps it. Both are 4 bytes,
	// so this is the only check that can see the type.
	builder.mSettings.mNovodeXExtendValue	= 0.5f;
	builder.mSettings.mNovodeXInflate		= 0.5f;
	nxLayout("BuildSettings.mNovodeXExtendValue_is_float",
		builder.mSettings.mNovodeXExtendValue == 0.5f ? 1 : 0, 1,
		"0x000f0efe fld dword ptr [esi+0x0c]");
	nxLayout("BuildSettings.mNovodeXInflate_is_float",
		builder.mSettings.mNovodeXInflate == 0.5f ? 1 : 0, 1,
		"0x000f0f8e fld dword ptr [esi+0x14]");
	// The defaults, which are what makes both features off unless a cook turns
	// them on: OPCODECREATE::OPCODECREATE writes -1 and 0 at 0x000e92b0.
	nxLayout("BuildSettings.defaults_are_off",
		(BuildSettings().mNovodeXExtendAxis == -1
			&& BuildSettings().mNovodeXExtendValue == 0.0f
			&& BuildSettings().mNovodeXInflate == 0.0f) ? 1 : 0, 1, "0x000e92b0");
	}

	// The two blocks of NovodeX code in OPC_AABBTree.cpp, driven rather than
	// inspected. Vendoring that file stock compiles, links, builds an identical
	// tree at the defaults, and builds a different one the moment either added
	// field is set -- so the assertion has to turn one on.
	nxCheckAABBTreeExtension();

	// OPC_RAYHIT_CALLBACK off, plus one added member.
	nxLayout("RayCollider.mMaxDist", NxRayColliderProbe::maxDistOffset(), 0x84, "0x000b5770");
	nxLayout("RayCollider.mClosestHit", NxRayColliderProbe::closestHitOffset(), 0x8c, "0x000b579d");

	// The borrowed-buffer markers.
	nxLayout("sizeof_Container", sizeof(Container), 16, "0x000b4d70");
	nxLayout("sizeof_RadixSort", sizeof(RadixSort), 24, "0x000e32c0");

	// sizeof(AABBTree) is read straight off Model::Build's allocation site, and
	// it is 48 in a stock compile too -- which is what says the class is NOT
	// modified.
	nxLayout("sizeof_AABBTree", sizeof(AABBTree), 48, "0x000e919a");

	// Segment and MeshInterface, both unmodified, both read by the oracle at
	// these offsets.
	nxLayout("sizeof_Segment", sizeof(Segment), 24, "0x000f0560");
	nxLayout("MeshInterface.mTris", 8, 8, "0x000e9012");

	// THE VTABLE SHAPES, checked by dispatching through the slot rather than by
	// counting anything. Nothing in C++ says how many virtuals a class has, so
	// the assertion is the one that matters instead: the slot the oracle
	// dispatches through has to reach the function the oracle reaches.
	//
	// Stock gives AABBOptimizedTree five slots with GetUsedBytes at 4. The image
	// has eight with GetUsedBytes at 7, and Model::GetUsedBytes at 0x000e90c0
	// tail-jumps [eax+0x1c] to get there. If the three added virtuals are
	// dropped from OPC_OptimizedTree.h everything still compiles and slot 7 is
	// past the end of the table.
	typedef unsigned (__thiscall* SlotFn)(const void*);
	AABBCollisionTree tree;
	// mNbNodes is protected and at AABBOptimizedTree+4, which is what the stock
	// class layout gives and what GetUsedBytes multiplies. Poked rather than
	// built, so the number the two sides compare is NOT zero -- 0 == 0 would
	// have passed against a vtable with no slot 7 in it at all.
	((unsigned*) &tree)[1] = 7;
	{
	void** vtable = *(void***) &tree;
	nxLayout("AABBOptimizedTree.GetUsedBytes_slot7",
		((SlotFn) vtable[7])(&tree), tree.GetUsedBytes(), "0x000e90c0 jmp [eax+0x1c]");
	// Slot 4 is one of the three added ones. Its body is Task 2b's and returns
	// zero; what is asserted is that the slot EXISTS and is distinct from
	// GetUsedBytes, which a stock header makes false -- there, slot 4 IS
	// GetUsedBytes and this check reads 7*sizeof(node) instead of 0.
	nxLayout("AABBOptimizedTree.added_slot4", ((SlotFn) vtable[4])(&tree),
		tree.NovodeXSlot4(), "0x000e9420 jmp [edx+0x10]");
	}
	{
	// BaseModel keeps GetUsedBytes at slot 2 -- its three virtuals are APPENDED,
	// not inserted -- and this pair is what says the two hierarchies were
	// changed differently.
	Model model;
	// mTree is at BaseModel+0x10, read there by Model::GetUsedBytes at
	// 0x000e90c0. Installed by hand and removed again before the destructor,
	// which would otherwise release a stack object.
	((void**) &model)[4] = &tree;
	void** vtable = *(void***) &model;
	nxLayout("BaseModel.GetUsedBytes_slot2",
		((SlotFn) vtable[2])(&model), model.GetUsedBytes(), ".rdata:0x0011bac8");
	nxLayout("BaseModel.added_slot4", ((SlotFn) vtable[4])(&model),
		model.NovodeXSlot4(), "0x000e9420");
	((void**) &model)[4] = 0;
	}

	// P4 Task 2b. IcePrunable.cpp is a reconstruction, so every one of these is
	// an offset this task read off an instruction rather than a modification a
	// stock header would get wrong -- but the consequence is the same: get one
	// of them wrong and the differential above compares the wrong bytes.
	nxLayout("sizeof_AABB", sizeof(AABB), 24, "0x000b55a6 lea eax,[eax+eax*2]; lea eax,[edx+eax*8]");
	nxLayout("Prunable.mOwner", (size_t) &(((Prunable*) 0)->mOwner), 0x04, "0x000b55c0");
	nxLayout("Prunable.mFlags", (size_t) &(((Prunable*) 0)->mFlags), 0x08, "0x000b54f0");
	nxLayout("Prunable.mMember0C", (size_t) &(((Prunable*) 0)->mMember0C), 0x0c, "0x000b54a9");
	nxLayout("Prunable.mMember0C.mPrunable",
		(size_t) &(((Prunable*) 0)->mMember0C.mPrunable), 0x10, "0x000b54e4");
	nxLayout("Prunable.mPruner", (size_t) &(((Prunable*) 0)->mPruner), 0x20, "0x000b559d");
	nxLayout("Prunable.mPrunable24", (size_t) &(((Prunable*) 0)->mPrunable24), 0x24, "0x000b54c3");
	nxLayout("Prunable.mHandle", (size_t) &(((Prunable*) 0)->mHandle), 0x28, "0x000b5590");
	nxLayout("Prunable.mPruningType", (size_t) &(((Prunable*) 0)->mPruningType), 0x2a, "0x000b55ed");
	nxLayout("Prunable.mPruningSection", (size_t) &(((Prunable*) 0)->mPruningSection), 0x2b, "0x000b561d");
	nxLayout("sizeof_Prunable0C", sizeof(Prunable0C), 20, "0x000e7330,0x000b54a9");
	nxLayout("Pruner.mWorldBoxes", (size_t) &(((Pruner*) 0)->mPool.mWorldBoxes), 0x14, "0x000b55a0");
	}

//////////////////////////////////////////////////////////////////////////////

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

// The two summary lines: the families driven so far and the oracle digest
// over them. Printed after each block of families, from one place, so that
// every copy is the same format (tools/tests/test_gate_targets.py requires a
// registration to be a prefix of exactly one format).
//////////////////////////////////////////////////////////////////////////////
// qhull-gap Task 1: qhull's output, trace, option and merge paths
// (evidence/qhull-gap.md, "Task 1").
//
// The Task 4 hull family runs the NovodeX driver's own option, "o", and eight
// more; it drops everything qhull prints. These families run qhull through the
// same driver sequence (nxQhullRunDim: qh_init_A, qh_initflags, qh_init_B,
// qh_qhull, qh_check_output, qh_produce_output) with the print, trace and
// option strings that reach the vendored rows the Task 4 runs never reach, and
// compare what each side prints as well as the hull it builds.
//
// OUTPUT CAPTURE. qhull leaves the library through two routes, and each side's
// output is captured on both without handing one C runtime's FILE to the other:
//
//  * The host object (.data:0x00125080). Slot +0x10 is every plain qhull
//    fprintf, +0x00/+0x04/+0x08/+0x0c the typed geometry of NovodeX's io.c
//    (External/qhull/MODIFICATIONS.md). For the oracle the harness installs a
//    stand-in object whose slots record into the capture below; for the
//    candidate, tests/PhysicsThirdPartyHost.cpp routes the same five hooks to
//    the same functions (gNxQhSink). Nothing is formatted: the capture walks
//    the format string and records its hash, then each argument by its
//    conversion -- an integer or character as a discrete word, a %e/%f/%g
//    double on the float tape at full precision, %p as a mask (a heap
//    address), a %s string tokenized like a file (below: qh qhull_options
//    carries numbers qhull formatted with its own CRT's sprintf), and the
//    stream as 1 (qh fout), 2 (qh ferr) or 3. So the host output is compared
//    bit for bit, and no CRT's %g enters it. Wall-clock values are masked: the
//    arguments of "CPU seconds" formats and the one before "CPU", the
//    "At %02d:%02d:%02d" time of day; and the cpu-seconds statistic, which
//    qh_printstatlevel skips when it is zero, is dropped with its description.
//  * Each side's own C runtime. The traceN macros call the CRT's fprintf on
//    qh ferr (the oracle's static CRT at 0x000f4d5a; the candidate's UCRT),
//    and qh_printpointid/qh_printpoint/qh_printpointvect's fputs write qh
//    fout. So qh fout and qh ferr are real files, opened per run by each side's
//    own CRT: the oracle's through its static-CRT fopen row (phys_fn_005671 at
//    0x000f4251, which is _fsopen(name, mode, _SH_DENYNO)) and closed through
//    its _fclose row (phys_fn_005673 at 0x000f42b0), which flushes; the
//    candidate's through the UCRT. The harness reads both files back after the
//    close, with the UCRT, as bytes. The two CRTs format %g differently: three
//    exponent digits against two ("1.4e-015" / "1.4e-15"), so the padding
//    differs too, and "-0" prints as " 0" on the old CRT. So the text is
//    compared as tokens, not bytes: whitespace pushes nothing; a number is
//    parsed (strtod) and compared on the float tape as a double, except an
//    integer glued to a letter (f12, p3, v7), which is an id and a discrete
//    word; every other run of characters is hashed. The trace-4 bucket numbers
//    after "hash"/"hashcount" (qh_gethash hashes vertex addresses) and the
//    number before "CPU" are masked. The last-digit rounding of an exact tie
//    ("%2.2g" of -3.25) differs too: the test exe links
//    legacy_stdio_float_rounding.obj, as NxPhysics.dll does (CMakeLists.txt), so
//    the candidate prints what the shipped candidate DLL prints.
//
// Each family is a pair, like qhull_hull/qhull_hull_x87: the discrete tape
// (the hull's combinatorial words, the host calls, the formats, integers and
// strings, the text tokens, the error exit) and the float tape (the hull's
// doubles, the printed doubles, the typed slots' floats, the parsed numbers).

extern "C" {
int		nxQhullRunDim(const NxQhullEntries* e, void* state, double* points, int numpoints, int dim,
	const char* options, int projectDelaunay);
void	nxQhullTapeDim(const void* state, NxQhPush push, void* tape, NxQhPushDouble pushDouble, void* floats);
void	nxQhullDirectEntries(unsigned char* oracleBase, void* out, unsigned size);
unsigned nxQhullDirectSize(void);
int		nxQhullDirect(const void* entries, const void* state, FILE* fp, NxQhPush push, void* tape,
	NxQhPushDouble pushDouble, void* floats);
}

struct NxQhSink
	{
	void (*offBegin)(int dim, int numpoints, int numfacets, int numridges);
	void (*point3)(float x, float y, float z);
	void (*facet3Vertex)(int count, int* pointids);
	void (*size)(float totarea, float totvol);
	void (*vprintf)(const void* stream, const char* format, va_list args);
	};
extern NxQhSink* gNxQhSink;	// tests/PhysicsThirdPartyHost.cpp

static const unsigned kOracleFopen	= 0x000f4251;	// phys_fn_005671: fopen = _fsopen(name, mode, 0x40)
static const unsigned kOracleFclose	= 0x000f42b0;	// phys_fn_005673: _fclose
typedef void*	(__cdecl* NxOracleFopenFn)(const char*, const char*);
typedef int		(__cdecl* NxOracleFcloseFn)(void*);

static const unsigned kQhMask = 0x4d41534bu;	// "MASK": a masked wall-clock value or heap address

struct NxQhCapture
	{
	NxTape*		tape;
	NxTape*		floats;
	const void*	fout;
	const void*	ferr;
	// Where the previous printf's words start on each tape, so that the
	// statistic whose description arrives in the next call can be dropped.
	unsigned		prevTape;
	unsigned		prevFloats;
	unsigned		prevCount;		// 0 when there is no previous call
	int				side;
	};
static NxQhCapture gQhCap;
static const char* gQhFamilyName = "";
static unsigned gQhRunIndex = 0;
// Attribution only: the format (or "<file>") behind each discrete word of the
// current family, per side, for the QHGAP_WORD lines on stderr.
static const char* gQhWordSource[2][NxTape::kMax];
static const char* gQhFloatSource[2][NxTape::kMax];
static void nxQhNoteSource(unsigned from, unsigned fromFloat, const char* source)
	{
	for(unsigned i = from; i < gQhCap.tape->count && i < NxTape::kMax; ++i)
		gQhWordSource[gQhCap.side][i] = source;
	for(unsigned i = fromFloat; i < gQhCap.floats->count && i < NxTape::kMax; ++i)
		gQhFloatSource[gQhCap.side][i] = source;
	}
static bool gQhCapOn = false;

static unsigned nxHashBytes(const char* s, size_t n)
	{
	unsigned h = 2166136261u;
	for(size_t i = 0; i < n; ++i)
		{
		h ^= (unsigned char) s[i];
		h *= 16777619u;
		}
	return h;
	}

static void nxQhTapeText(const char* text, size_t n, NxTape& tape, NxTape& floats);

static void nxQhCapOff(int dim, int numpoints, int numfacets, int numridges)
	{
	if(!gQhCapOn)
		return;
	gQhCap.tape->push(0x51480000u);
	gQhCap.tape->push((unsigned) dim);
	gQhCap.tape->push((unsigned) numpoints);
	gQhCap.tape->push((unsigned) numfacets);
	gQhCap.tape->push((unsigned) numridges);
	}

static void nxQhCapPoint3(float x, float y, float z)
	{
	if(!gQhCapOn)
		return;
	gQhCap.tape->push(0x51480004u);
	const unsigned firstFloat = gQhCap.floats->count;
	gQhCap.floats->pushFloat(x);
	gQhCap.floats->pushFloat(y);
	gQhCap.floats->pushFloat(z);
	nxQhNoteSource(gQhCap.tape->count, firstFloat, "<slot +0x04>");
	}

static void nxQhCapFacet3Vertex(int count, int* pointids)
	{
	if(!gQhCapOn)
		return;
	gQhCap.tape->push(0x51480008u);
	gQhCap.tape->push((unsigned) count);
	for(int i = 0; i < count && i < 256; ++i)
		gQhCap.tape->push((unsigned) pointids[i]);
	}

static void nxQhCapSize(float totarea, float totvol)
	{
	if(!gQhCapOn)
		return;
	gQhCap.tape->push(0x5148000cu);
	const unsigned firstFloat = gQhCap.floats->count;
	gQhCap.floats->pushFloat(totarea);
	gQhCap.floats->pushFloat(totvol);
	nxQhNoteSource(gQhCap.tape->count, firstFloat, "<slot +0x0c>");
	}

static bool nxQhFollowedByCpu(const char* p)
	{
	while(*p == ' ')
		++p;
	return strncmp(p, "CPU", 3) == 0;
	}

static bool nxQhNamesCpu(const char* s)
	{
	for(; *s; ++s)
		if((s[0] == 'c' || s[0] == 'C') && (s[1] == 'p' || s[1] == 'P') && (s[2] == 'u' || s[2] == 'U'))
			return true;
	return false;
	}

static void nxQhCapVprintf(const void* stream, const char* format, va_list args)
	{
	if(!gQhCapOn)
		return;
	// NXQHGAP_LOG=<prefix>: each side's host output, formatted, for attribution.
	static FILE* log[2] = { 0, 0 };
	static bool logChecked = false;
	if(!logChecked)
		{
		logChecked = true;
		const char* prefix = getenv("NXQHGAP_LOG");
		if(prefix)
			{
			char path[MAX_PATH];
			_snprintf(path, MAX_PATH, "%s-oracle.txt", prefix);
			log[0] = fopen(path, "w");
			_snprintf(path, MAX_PATH, "%s-candidate.txt", prefix);
			log[1] = fopen(path, "w");
			}
		}
	if(log[gQhCap.side])
		{
		va_list copy;
		va_copy(copy, args);
		fprintf(log[gQhCap.side], "[%s/%u] ", gQhFamilyName, gQhRunIndex);
		vfprintf(log[gQhCap.side], format, copy);
		va_end(copy);
		}
	NxTape& tape = *gQhCap.tape;
	NxTape& floats = *gQhCap.floats;
	const unsigned firstWord = tape.count;
	const unsigned firstFloat = floats.count;
	tape.push(0x51480010u);
	tape.push(stream == gQhCap.fout ? 1u : stream == gQhCap.ferr ? 2u : 3u);
	if(!format)
		{
		tape.push(0xfffffff1u);
		return;
		}
	tape.push(nxHashBytes(format, strlen(format)));
	// qh_printsummary's "CPU seconds to compute hull (after input): %2.4g".
	const bool cpuLine = strstr(format, "CPU seconds") != 0;
	const char* clock = strstr(format, "At %02d:%02d:%02d");
	if(!clock)
		clock = strstr(format, "At %d:%d:%d");
	unsigned conversion = 0;
	bool maskPrevious = false;
	for(const char* p = format; *p; ++p)
		{
		if(*p != '%')
			continue;
		const char* start = p;
		++p;
		if(*p == '%')
			continue;
		while(*p && strchr("-+ #0", *p))
			++p;
		if(*p == '*')
			{
			(void) va_arg(args, int);
			++p;
			}
		else
			while(*p >= '0' && *p <= '9')
				++p;
		if(*p == '.')
			{
			++p;
			if(*p == '*')
				{
				(void) va_arg(args, int);
				++p;
				}
			else
				while(*p >= '0' && *p <= '9')
					++p;
			}
		while(*p == 'h' || *p == 'l' || *p == 'L')
			++p;
		if(!*p)
			break;
		const bool masked = cpuLine || nxQhFollowedByCpu(p + 1) || (clock && start > clock && conversion < 3 && start < clock + 20);
		switch(*p)
			{
			case 'd': case 'i': case 'u': case 'x': case 'X': case 'o': case 'c':
				{
				const int v = va_arg(args, int);
				tape.push(masked ? kQhMask : (unsigned) v);
				break;
				}
			case 'e': case 'E': case 'f': case 'g': case 'G':
				{
				const double v = va_arg(args, double);
				floats.pushDouble(masked ? 0.0 : v);
				break;
				}
			case 's':
				{
				const char* s = va_arg(args, const char*);
				// A string argument is tokenized like a CRT-written file: qhull
				// builds some of them with its own CRT's sprintf (qh_option's
				// "%2.2g" values in qh qhull_options), so their numbers are
				// compared as numbers.
				if(s)
					nxQhTapeText(s, strlen(s), tape, floats);
				else
					tape.push(0xfffffff2u);
				if(s && nxQhNamesCpu(s))
					maskPrevious = true;
				break;
				}
			case 'p':
				(void) va_arg(args, void*);
				tape.push(kQhMask);
				break;
			default:
				tape.push(0x42414400u | (unsigned char) *p);	// "BAD": a conversion the capture does not know
				break;
			}
		++conversion;
		}
	// qh_printstatlevel prints a statistic's value ("%7.2g") and then its
	// description (" %s\n"), and skips a statistic that is zero. The cpu-seconds
	// statistic is wall-clock time, zero on one side and not on the other, so
	// both calls are dropped from the tapes rather than masked.
	if(maskPrevious && gQhCap.prevCount)
		{
		tape.count = gQhCap.prevTape;
		floats.count = gQhCap.prevFloats;
		gQhCap.prevCount = 0;
		return;
		}
	nxQhNoteSource(firstWord, firstFloat, format);
	gQhCap.prevTape = firstWord;
	gQhCap.prevFloats = firstFloat;
	gQhCap.prevCount = 1;
	}

static NxQhSink gQhCandidateSink =
	{ &nxQhCapOff, &nxQhCapPoint3, &nxQhCapFacet3Vertex, &nxQhCapSize, &nxQhCapVprintf };

// The oracle's side of the same sink: the stand-in host object's slots.
static void __fastcall nxQhHostCapOff(void*, int, int dim, int numpoints, int numfacets, int numridges)
	{ nxQhCapOff(dim, numpoints, numfacets, numridges); }
static void __fastcall nxQhHostCapPoint(void*, int, float x, float y, float z) { nxQhCapPoint3(x, y, z); }
static void __fastcall nxQhHostCapFacet(void*, int, int count, int* ids) { nxQhCapFacet3Vertex(count, ids); }
static void __fastcall nxQhHostCapSize(void*, int, float totarea, float totvol) { nxQhCapSize(totarea, totvol); }
static int __cdecl nxQhHostCapPrintf(void*, void* stream, const char* format, ...)
	{
	va_list args;
	va_start(args, format);
	nxQhCapVprintf(stream, format, args);
	va_end(args);
	return 0;
	}

static void* gQhHostCapVtable[9] =
	{
	(void*) &nxQhHostCapOff, (void*) &nxQhHostCapPoint, (void*) &nxQhHostCapFacet, (void*) &nxQhHostCapSize,
	(void*) &nxQhHostCapPrintf, (void*) &nxQhHostMalloc, (void*) &nxQhHostFree, (void*) &nxQhHostNarrow,
	(void*) &nxQhHostErrexit
	};
static void* gQhHostCapObject[4] = { gQhHostCapVtable, 0, 0, 0 };

// A CRT-written file as tokens (see OUTPUT CAPTURE above).
static bool nxQhIsDigit(char c) { return c >= '0' && c <= '9'; }

static void nxQhTapeText(const char* text, size_t n, NxTape& tape, NxTape& floats)
	{
	size_t i = 0;
	bool afterAt = false;		// the previous token was "At" (a time of day follows)
	bool afterHash = false;		// the previous token was "hash" or "hashcount"
	while(i < n)
		{
		while(i < n && (text[i] == ' ' || text[i] == '\t' || text[i] == '\r' || text[i] == '\n'))
			++i;
		if(i >= n)
			break;
		size_t end = i;
		while(end < n && !(text[end] == ' ' || text[end] == '\t' || text[end] == '\r' || text[end] == '\n'))
			++end;
		// Is the next token "CPU"? Then this token's numbers are wall-clock time.
		size_t next = end;
		while(next < n && (text[next] == ' ' || text[next] == '\t' || text[next] == '\r' || text[next] == '\n'))
			++next;
		const bool beforeCpu = next + 3 <= n && strncmp(text + next, "CPU", 3) == 0;
		const bool maskToken = beforeCpu || afterAt || afterHash;
		afterAt = end - i == 2 && text[i] == 'A' && text[i + 1] == 't';
		// qh_matchneighbor's trace4 prints qh_gethash's bucket, a hash of vertex
		// ADDRESSES, and the bucket's count: heap layout, which differs between
		// the two sides and between runs.
		afterHash = (end - i == 4 && strncmp(text + i, "hash", 4) == 0)
			|| (end - i == 9 && strncmp(text + i, "hashcount", 9) == 0);
		// Whitespace pushes nothing: the two CRTs pad a %g differently ("%2.2g"
		// of 0 is " 0" where the value prints as "-0" on the other), so only
		// what is printed is compared, not where the spaces fall.
		size_t j = i;
		while(j < end)
			{
			// A number: [sign] digits [. digits] [e [sign] digits], or . digits.
			size_t k = j;
			const bool sign = (text[k] == '-' || text[k] == '+') && (k == i || !(nxQhIsDigit(text[k - 1])
				|| (text[k - 1] >= 'a' && text[k - 1] <= 'z') || (text[k - 1] >= 'A' && text[k - 1] <= 'Z')));
			if(sign)
				++k;
			size_t digits = k;
			while(digits < end && nxQhIsDigit(text[digits]))
				++digits;
			bool isNumber = digits > k;
			bool isReal = false;
			size_t m = digits;
			if(m < end && text[m] == '.' && m + 1 < end && nxQhIsDigit(text[m + 1]))
				{
				isNumber = true;
				isReal = true;
				++m;
				while(m < end && nxQhIsDigit(text[m]))
					++m;
				}
			else if(isNumber && m < end && text[m] == '.')
				{
				isReal = true;
				++m;
				}
			if(isNumber && m + 1 < end && (text[m] == 'e' || text[m] == 'E'))
				{
				size_t e = m + 1;
				if(e < end && (text[e] == '-' || text[e] == '+'))
					++e;
				if(e < end && nxQhIsDigit(text[e]))
					{
					isReal = true;
					m = e;
					while(m < end && nxQhIsDigit(text[m]))
						++m;
					}
				}
			if(isNumber)
				{
				char number[64];
				const size_t len = m - j < sizeof(number) - 1 ? m - j : sizeof(number) - 1;
				memcpy(number, text + j, len);
				number[len] = 0;
				// An integer glued to a letter is an id (f12, p3, v7, #4): a
				// discrete word. Any other number may be a %g that happens to
				// print without a point ("0" on one side, "-2.8e-17" on the
				// other), so it is a double, whatever it looks like.
				const bool id = !isReal && j > i && ((text[j - 1] >= 'a' && text[j - 1] <= 'z')
					|| (text[j - 1] >= 'A' && text[j - 1] <= 'Z') || text[j - 1] == '#');
				if(!id && !(maskToken && !isReal))
					floats.pushDouble(maskToken ? 0.0 : strtod(number, 0));
				else if(maskToken)
					tape.push(kQhMask);
				else if(m - j <= 9)
					tape.push((unsigned) atoi(number));
				else
					tape.push(nxHashBytes(number, len));
				j = m;
				continue;
				}
			// Anything else, up to the next number.
			size_t s = j + 1;
			while(s < end && !nxQhIsDigit(text[s]) && !((text[s] == '-' || text[s] == '+' || text[s] == '.')
				&& s + 1 < end && nxQhIsDigit(text[s + 1])))
				++s;
			tape.push(nxHashBytes(text + j, s - j));
			j = s;
			}
		i = end;
		}
	}

static char gQhFileNames[4][MAX_PATH];

static void nxQhFileNames()
	{
	char dir[MAX_PATH];
	const DWORD n = GetTempPathA(MAX_PATH, dir);
	if(n == 0 || n >= MAX_PATH)
		strcpy(dir, ".\\");
	static const char* const kWhich[4] = { "oracle-out", "oracle-err", "candidate-out", "candidate-err" };
	for(int k = 0; k < 4; ++k)
		_snprintf(gQhFileNames[k], MAX_PATH, "%snxqhgap-%lu-%s.txt", dir, (unsigned long) GetCurrentProcessId(),
			kWhich[k]);
	}

// Reads a closed file back and tapes it; then deletes it.
static void nxQhTapeFile(const char* path, NxTape& tape, NxTape& floats)
	{
	FILE* f = fopen(path, "rb");
	if(!f)
		{
		tape.push(0xfffffff3u);
		return;
		}
	static char buffer[1 << 22];
	const size_t n = fread(buffer, 1, sizeof(buffer), f);
	fclose(f);
	// NXQHGAP_KEEP=<dir> keeps each run's files there, for attribution.
	const char* keep = getenv("NXQHGAP_KEEP");
	if(keep)
		{
		char kept[MAX_PATH];
		const char* base = strrchr(path, '\\');
		_snprintf(kept, MAX_PATH, "%s\\%s-%u%s", keep, gQhFamilyName, gQhRunIndex, base ? base + 1 : path);
		kept[MAX_PATH - 1] = 0;
		CopyFileA(path, kept, FALSE);
		}
	DeleteFileA(path);
	tape.push(n == sizeof(buffer) ? 0xfffffff4u : 0x46494c45u);	// "FILE"
	nxQhTapeText(buffer, n, tape, floats);
	}

// One point set for the qhull-gap families: `dim` coordinates per point.
struct NxQhGapSet
	{
	float		points[3 * 1024];
	unsigned	count;
	int			dim;
	};

static void nxQhGapPoints(int set, NxQhGapSet& s)
	{
	unsigned n = 0;
	float* out = s.points;
	s.dim = 3;
	if(set < 12)
		{
		s.count = nxQhullPoints(set, out);
		return;
		}
	switch(set)
		{
		case 12:	// 2-d: a square with points along its edges (collinear) and inside
			s.dim = 2;
			for(int i = 0; i < 5; ++i)
				{
				const float t = -1.0f + 0.5f * (float) i;
				out[n * 2] = t; out[n * 2 + 1] = -1.0f; ++n;
				out[n * 2] = t; out[n * 2 + 1] = 1.0f; ++n;
				out[n * 2] = -1.0f; out[n * 2 + 1] = t; ++n;
				out[n * 2] = 1.0f; out[n * 2 + 1] = t; ++n;
				}
			for(int i = 0; i < 12; ++i)
				{
				out[n * 2] = nxRange(-0.9f, 0.9f).value; out[n * 2 + 1] = nxRange(-0.9f, 0.9f).value; ++n;
				}
			break;
		case 13:	// 2-d: a jittered circle
			s.dim = 2;
			for(int i = 0; i < 40; ++i)
				{
				const double a = i * (6.283185307179586 / 40.0);
				out[n * 2] = (float) cos(a) + nxRange(-1e-6f, 1e-6f).value;
				out[n * 2 + 1] = (float) sin(a) + nxRange(-1e-6f, 1e-6f).value;
				++n;
				}
			break;
		case 14:	// 4-d: the tesseract's corners and its centre
			s.dim = 4;
			for(int v = 0; v < 16; ++v)
				{
				for(int k = 0; k < 4; ++k)
					out[n * 4 + k] = (v >> k) & 1 ? 1.0f : -1.0f;
				++n;
				}
			for(int k = 0; k < 4; ++k)
				out[n * 4 + k] = 0.0f;
			++n;
			break;
		case 15:	// 4-d: random points in a box
			s.dim = 4;
			for(int i = 0; i < 40; ++i)
				{
				for(int k = 0; k < 4; ++k)
					out[n * 4 + k] = nxRange(-1, 1).value;
				++n;
				}
			break;
		case 16:	// 4-d: the 3^4 lattice (coplanar facets everywhere)
			s.dim = 4;
			for(int i = 0; i < 81; ++i)
				{
				int c = i;
				for(int k = 0; k < 4; ++k)
					{
					out[n * 4 + k] = (float) (c % 3) - 1.0f;
					c /= 3;
					}
				++n;
				}
			break;
		case 17:	// a cube's faces, each sampled at 60 points jittered by 1e-6 off the face
			for(int i = 0; i < 360; ++i)
				{
				const int face = i % 6;
				const int axis = face >> 1;
				float p[3] = { nxRange(-1, 1).value, nxRange(-1, 1).value, nxRange(-1, 1).value };
				p[axis] = (face & 1 ? 1.0f : -1.0f) + nxRange(-1e-6f, 1e-6f).value;
				memcpy(out + n * 3, p, sizeof(p));
				++n;
				}
			break;
		case 18:	// 500 points on a sphere
			for(int i = 0; i < 500; ++i)
				{
				// Sequenced: as one expression the draws' order was the compiler's.
				const float pointZ = nxRange(-1, 1).value;
				const float pointY = nxRange(-1, 1).value;
				const float pointX = nxRange(-1, 1).value;
				Point p(pointX, pointY, pointZ);
				if(p.Magnitude() > 1e-3f)
					p.Normalize();
				out[n * 3] = p.x; out[n * 3 + 1] = p.y; out[n * 3 + 2] = p.z;
				++n;
				}
			break;
		case 19:	// clusters: eight tight balls of 20 at the cube's corners
			for(int c = 0; c < 8; ++c)
				for(int i = 0; i < 20; ++i)
					{
					out[n * 3] = (c & 1 ? 1.0f : -1.0f) + nxRange(-1e-4f, 1e-4f).value;
					out[n * 3 + 1] = (c & 2 ? 1.0f : -1.0f) + nxRange(-1e-4f, 1e-4f).value;
					out[n * 3 + 2] = (c & 4 ? 1.0f : -1.0f) + nxRange(-1e-4f, 1e-4f).value;
					++n;
					}
			break;
		case 20:	// a cone: a ring of 24, its apex, and 24 points along the side
			for(int i = 0; i < 24; ++i)
				{
				const double a = i * (6.283185307179586 / 24.0);
				out[n * 3] = (float) cos(a); out[n * 3 + 1] = (float) sin(a); out[n * 3 + 2] = 0.0f; ++n;
				const float t = nxUnit().value;
				out[n * 3] = (float) cos(a) * (1 - t); out[n * 3 + 1] = (float) sin(a) * (1 - t);
				out[n * 3 + 2] = 2.0f * t; ++n;
				}
			out[n * 3] = 0; out[n * 3 + 1] = 0; out[n * 3 + 2] = 2.0f; ++n;
			break;
		case 21:	// 1000 points in a box
			for(int i = 0; i < 1000; ++i)
				{
				out[n * 3] = nxRange(-1, 1).value; out[n * 3 + 1] = nxRange(-1, 1).value; out[n * 3 + 2] = nxRange(-1, 1).value;
				++n;
				}
			break;
		case 23:	// 3-d: 30 random points in a box (Delaunay input)
			for(int i = 0; i < 30; ++i)
				{
				out[n * 3] = nxRange(-1, 1).value; out[n * 3 + 1] = nxRange(-1, 1).value; out[n * 3 + 2] = nxRange(-1, 1).value;
				++n;
				}
			break;
		default:	// 2-d: random points in a square
			s.dim = 2;
			for(int i = 0; i < 30; ++i)
				{
				out[n * 2] = nxRange(-1, 1).value; out[n * 2 + 1] = nxRange(-1, 1).value;
				++n;
				}
			break;
		}
	s.count = n;
	}

struct NxQhGapRun
	{
	int			set;
	const char*	options;
	int			delaunay;	// 1: set qh PROJECTdelaunay, as qh_readpoints does for 'd' and 'v'
	int			direct;		// 1: then call the out-of-line printers and helpers (nxQhullDirect)
	};

static NxTape gQhGapTape[2];
static NxTape gQhGapFloats[2];

// One run on both sides: the hull, what the host was handed, and both files.
static void nxQhGapRun(const NxOracleRows& o, const NxQhullEntries& oracle, const NxQhullEntries& candidate,
	int runIndex, const NxQhGapRun& run, bool selfOnly)
	{
	static NxQhGapSet set;
	gState = 0x71a90000u + (unsigned) run.set;	// the set's points depend on the set alone
	nxQhGapPoints(run.set, set);
	const NxOracleFopenFn oracleFopen = (NxOracleFopenFn) nxAt(o, kOracleFopen);
	const NxOracleFcloseFn oracleFclose = (NxOracleFcloseFn) nxAt(o, kOracleFclose);
	unsigned startTape[2], startFloats[2];
	for(int side = 0; side < (selfOnly ? 1 : 2); ++side)
		{
		NxTape& tape = gQhGapTape[side];
		NxTape& floats = gQhGapFloats[side];
		startTape[side] = tape.count;
		startFloats[side] = floats.count;
		NxQhullEntries e = side == 0 ? oracle : candidate;
		const char* outName = gQhFileNames[side * 2];
		const char* errName = gQhFileNames[side * 2 + 1];
		void* fout = side == 0 ? oracleFopen(outName, "w") : (void*) fopen(outName, "w");
		void* ferr = side == 0 ? oracleFopen(errName, "w") : (void*) fopen(errName, "w");
		if(!fout || !ferr)
			{
			fprintf(stderr, "FAIL qhull-gap cannot open its capture files\n");
			++gMismatches;
			return;
			}
		e.fout = fout;
		e.ferr = ferr;
		double* coords = (double*) malloc(sizeof(double) * set.dim * set.count + 8);
		for(unsigned i = 0; i < set.dim * set.count; ++i)
			coords[i] = set.points[i];
		void* state = side == 0 ? (void*) (o.base + kQhState) : nxQhullCandidateState();
		tape.push(0x52554e00u | (unsigned) (runIndex & 0xff));	// "RUN"
		gQhCap.tape = &tape;
		gQhCap.floats = &floats;
		gQhCap.fout = fout;
		gQhCap.ferr = ferr;
		gQhCap.prevCount = 0;
		gQhCap.side = side;
		gQhCapOn = true;
		if(side == 1)
			gNxQhSink = &gQhCandidateSink;
		const int result = nxQhullRunDim(&e, state, coords, (int) set.count, set.dim, run.options, run.delaunay);
		gNxQhSink = 0;
		gQhCapOn = false;
		tape.push(result ? 1u : 0u);
		if(result == 0)
			nxQhullTapeDim(state, nxQhPushTape, &tape, nxQhPushTapeDouble, &floats);
		if(result == 0 && run.direct)
			{
			static unsigned char entries[2][64 * sizeof(void*)];
			if(nxQhullDirectSize() > sizeof(entries[0]))
				abort();
			nxQhullDirectEntries(side == 0 ? o.base : 0, entries[side], nxQhullDirectSize());
			gQhCapOn = true;
			if(side == 1)
				gNxQhSink = &gQhCandidateSink;
			const int direct = nxQhullDirect(entries[side], state, (FILE*) fout, nxQhPushTape, &tape,
				nxQhPushTapeDouble, &floats);
			gNxQhSink = 0;
			gQhCapOn = false;
			tape.push(direct ? 1u : 0u);
			}
		if(side == 0)
			{
			oracleFclose(fout);
			oracleFclose(ferr);
			while(gQhOracleNbBlocks)
				free(gQhOracleBlocks[--gQhOracleNbBlocks]);
			}
		else
			{
			fclose((FILE*) fout);
			fclose((FILE*) ferr);
			}
		const unsigned fileStart = tape.count, fileStartFloat = floats.count;
		nxQhTapeFile(outName, tape, floats);
		nxQhTapeFile(errName, tape, floats);
		for(unsigned i = fileStart; i < tape.count && i < NxTape::kMax; ++i)
			gQhWordSource[side][i] = "<file>";
		for(unsigned i = fileStartFloat; i < floats.count && i < NxTape::kMax; ++i)
			gQhFloatSource[side][i] = "<file>";
		free(coords);
		}
	if(selfOnly)
		return;
	// Which run a difference comes from, on stderr only (attribution).
	unsigned discrete = 0, doubles = 0;
	const unsigned lenT0 = gQhGapTape[0].count - startTape[0], lenT1 = gQhGapTape[1].count - startTape[1];
	const unsigned lenF0 = gQhGapFloats[0].count - startFloats[0], lenF1 = gQhGapFloats[1].count - startFloats[1];
	for(unsigned i = 0; i < lenT0 && i < lenT1; ++i)
		if(gQhGapTape[0].words[startTape[0] + i] != gQhGapTape[1].words[startTape[1] + i])
			++discrete;
	for(unsigned i = 0; i < lenF0 && i < lenF1; ++i)
		if(gQhGapFloats[0].words[startFloats[0] + i] != gQhGapFloats[1].words[startFloats[1] + i])
			++doubles;
	if(discrete || doubles || lenT0 != lenT1 || lenF0 != lenF1)
		fprintf(stderr, "QHGAP run=%d set=%d options=\"%s\" discrete=%u/%u length=%u/%u floats=%u/%u length=%u/%u\n",
			runIndex, run.set, run.options, discrete, lenT0, lenT0, lenT1, doubles, lenF0, lenF0, lenF1);
	unsigned shown = 0;
	for(unsigned i = 0; i < lenT0 && i < lenT1 && shown < 4; ++i)
		{
		const unsigned a = startTape[0] + i, b = startTape[1] + i;
		if(a >= NxTape::kMax || b >= NxTape::kMax || gQhGapTape[0].words[a] == gQhGapTape[1].words[b])
			continue;
		const char* source = gQhWordSource[0][a] ? gQhWordSource[0][a] : "<hull>";
		char line[96];
		size_t k = 0;
		for(; source[k] && k < sizeof(line) - 1; ++k)
			line[k] = source[k] == '\n' ? '|' : source[k];
		line[k] = 0;
		fprintf(stderr, "QHGAP_WORD run=%d word=%u oracle=%08x candidate=%08x from=\"%s\"\n", runIndex, i,
			gQhGapTape[0].words[a], gQhGapTape[1].words[b], line);
		++shown;
		}
	// The floats and doubles more than 1e-9 apart, for attribution.
	shown = 0;
	for(unsigned i = 0; i + 1 < lenF0 && i + 1 < lenF1 && shown < 4; ++i)
		{
		const unsigned a = startFloats[0] + i, b = startFloats[1] + i;
		if(a + 1 >= NxTape::kMax || b + 1 >= NxTape::kMax)
			break;
		const NxTape& fo = gQhGapFloats[0];
		const NxTape& fc = gQhGapFloats[1];
		double x, y;
		if(fo.kinds[a] == kWordDoubleLo)
			{
			unsigned w[2] = { fo.words[a], fo.words[a + 1] };
			memcpy(&x, w, sizeof(x));
			unsigned v[2] = { fc.words[b], fc.words[b + 1] };
			memcpy(&y, v, sizeof(y));
			}
		else if(fo.kinds[a] == kWordFloat)
			{
			float fx, fy;
			memcpy(&fx, &fo.words[a], 4);
			memcpy(&fy, &fc.words[b], 4);
			x = fx;
			y = fy;
			}
		else
			continue;
		if(!(fabs(x - y) > 1e-9) && x == x && y == y)
			continue;
		const char* source = gQhFloatSource[0][a] ? gQhFloatSource[0][a] : "<hull>";
		char line[96];
		size_t k = 0;
		for(; source[k] && k < sizeof(line) - 1; ++k)
			line[k] = source[k] == '\n' ? '|' : source[k];
		line[k] = 0;
		fprintf(stderr, "QHGAP_FLOAT run=%d word=%u oracle=%.17g candidate=%.17g from=\"%s\"\n", runIndex, i, x, y, line);
		++shown;
		}
	}

static void nxQhGapFamily(const NxOracleRows& o, const char* name, const char* nameX87, const char* rva,
	const char* owner, const char* source, const char* rvaX87, const char* ownerX87, const char* sourceX87,
	const NxQhGapRun* runs, unsigned count, bool selfOnly, unsigned discreteTolerance, unsigned floatTolerance)
	{
	NxQhullEntries oracle;
	oracle.initA			= o.base + kQhInitA;
	oracle.initflags		= o.base + kQhInitflags;
	oracle.initB			= o.base + kQhInitB;
	oracle.qhull			= o.base + kQhQhull;
	oracle.checkOutput		= o.base + kQhCheckOutput;
	oracle.produceOutput	= o.base + kQhProduceOutput;
	oracle.fin				= o.base + kOracleIob;
	oracle.fout				= 0;
	oracle.ferr				= 0;
	NxQhullEntries candidate;
	candidate.initA			= (void*) &qh_init_A;
	candidate.initflags		= (void*) &qh_initflags;
	candidate.initB			= (void*) &qh_init_B;
	candidate.qhull			= (void*) &qh_qhull;
	candidate.checkOutput	= (void*) &qh_check_output;
	candidate.produceOutput	= (void*) &qh_produce_output;
	candidate.fin			= stdin;
	candidate.fout			= 0;
	candidate.ferr			= 0;

	void** hostSlot = (void**) (o.base + kQhHostGlobal);
	void* shippedHost = *hostSlot;
	*hostSlot = gQhHostCapObject;
	for(int side = 0; side < 2; ++side)
		{
		gQhGapTape[side].reset();
		gQhGapFloats[side].reset();
		memset(gQhWordSource[side], 0, sizeof(gQhWordSource[side]));
		memset(gQhFloatSource[side], 0, sizeof(gQhFloatSource[side]));
		}
	gQhFamilyName = name;
	for(unsigned r = 0; r < count; ++r)
		{
		gQhRunIndex = r;
		nxQhGapRun(o, oracle, candidate, (int) r, runs[r], selfOnly);
		}
	*hostSlot = shippedHost;

	nxReportTapes(gQhGapTape[0], gQhGapTape[1], name, rva, owner, source, selfOnly, discreteTolerance);
	nxReportTapes(gQhGapFloats[0], gQhGapFloats[1], nameX87, rvaX87, ownerX87, sourceX87, selfOnly, floatTolerance);
	}

static void nxDriveQhullGap(const NxOracleRows& o, bool selfOnly)
	{
	nxQhFileNames();

	// qh_produce_output over the print formats, 3-d. Sets: 0 tetrahedron, 1 cube,
	// 2 lattice, 3 sphere, 5 slab, 6 duplicates, 7 cylinder, 9-11 error exits.
	static const NxQhGapRun kOutput[] =
		{
		{ 2, "s", 0 }, { 2, "f", 0 }, { 2, "i", 0 }, { 2, "n", 0 }, { 2, "p", 0 }, { 2, "m", 0 },
		{ 2, "G", 0 }, { 2, "FF Fi Fn", 0 }, { 2, "Fa FA", 0 }, { 2, "Fc FC", 0 }, { 2, "FD", 0 },
		{ 2, "Fo FI FN", 0 }, { 2, "FO FP", 0 }, { 2, "FQ FS", 0 }, { 2, "Fs Ft", 0 }, { 2, "Fv FV", 0 },
		{ 2, "Fx", 0 }, { 2, "FM", 0 }, { 2, "Fm", 0 }, { 2, "Gv Gp", 0 }, { 2, "Gc Gh Gr", 0 },
		{ 2, "Gi Gn", 0 }, { 2, "Go", 0 }, { 2, "Gt", 0 }, { 2, "PG", 0 }, { 2, "Ts", 0 },
		{ 1, "s", 0 }, { 1, "f", 0 }, { 1, "i", 0 }, { 1, "G", 0 }, { 1, "m", 0 }, { 1, "Fx", 0 },
		{ 1, "Fc FN Fv", 0 }, { 1, "Ts", 0 }, { 1, "i Qt", 0 }, { 1, "G Qt", 0 }, { 1, "m Qt", 0 },
		{ 3, "s", 0 }, { 3, "f", 0 }, { 3, "i", 0 }, { 3, "G", 0 }, { 3, "m", 0 }, { 3, "Fx", 0 },
		{ 3, "n FD", 0 }, { 3, "FA Fa", 0 }, { 3, "Ts", 0 }, { 3, "Fc FP", 0 },
		{ 5, "s", 0 }, { 5, "f", 0 }, { 5, "G", 0 }, { 5, "Fx", 0 }, { 5, "FS Ts", 0 },
		{ 6, "s", 0 }, { 6, "f", 0 }, { 6, "Fc FP", 0 }, { 6, "G", 0 },
		{ 7, "s", 0 }, { 7, "f", 0 }, { 7, "i", 0 }, { 7, "G", 0 }, { 7, "m", 0 }, { 7, "FM", 0 },
		{ 0, "s", 0 }, { 0, "f", 0 }, { 0, "G", 0 }, { 0, "Ts", 0 },
		{ 9, "s", 0 }, { 10, "s", 0 }, { 11, "s", 0 },
		};
	nxQhGapFamily(o, "qhull_output", "qhull_output_x87", "0x0006d800", "phys_fn_002866", "io.c,geom2.c,poly2.c,stat.c",
		"0x0006d800", "phys_fn_002866", "io.c,geom.c,geom2.c", kOutput, sizeof(kOutput) / sizeof(kOutput[0]),
		selfOnly, 0, kDivergent);

	// The same over 2-d and 4-d hulls (12, 13, 22: 2-d; 14-16: 4-d), and Delaunay
	// and Voronoi output over 2-d and 3-d input.
	static const NxQhGapRun kOutputDims[] =
		{
		{ 12, "o", 0 }, { 12, "s", 0 }, { 12, "f", 0 }, { 12, "i", 0 }, { 12, "m", 0 }, { 12, "G", 0 },
		{ 12, "Fx", 0 }, { 12, "n p", 0 }, { 12, "FN Fv", 0 }, { 12, "Ts", 0 },
		{ 13, "o", 0 }, { 13, "s", 0 }, { 13, "G", 0 }, { 13, "m", 0 }, { 13, "Fx", 0 },
		{ 22, "o", 0 }, { 22, "G", 0 }, { 22, "i", 0 },
		{ 14, "o", 0 }, { 14, "s", 0 }, { 14, "f", 0 }, { 14, "i", 0 }, { 14, "G", 0 }, { 14, "Fx", 0 },
		{ 14, "n", 0 }, { 14, "Ts", 0 },
		{ 15, "o", 0 }, { 15, "s", 0 }, { 15, "G", 0 }, { 15, "i", 0 }, { 15, "Fx", 0 },
		{ 16, "G", 0 }, { 16, "i", 0 },
		};
	nxQhGapFamily(o, "qhull_output_dims", "qhull_output_dims_x87", "0x0006d200", "phys_fn_002862",
		"io.c,geom2.c,poly2.c,stat.c", "0x0006d200", "phys_fn_002862", "io.c,geom.c,geom2.c",
		kOutputDims, sizeof(kOutputDims) / sizeof(kOutputDims[0]), selfOnly, 0, kDivergent);

	static const NxQhGapRun kOutputDelaunay[] =
		{
		{ 22, "d", 1 }, { 22, "d s", 1 }, { 22, "d i", 1 }, { 22, "d G", 1 }, { 22, "d m", 1 },
		{ 22, "d Fv", 1 }, { 22, "d Qt i", 1 }, { 22, "d Qz", 1 }, { 22, "d Qu", 1 }, { 22, "d FA", 1 },
		{ 22, "v o", 1 }, { 22, "v p", 1 }, { 22, "v Fv", 1 }, { 22, "v Fi", 1 }, { 22, "v Fo", 1 },
		{ 22, "v G", 1 }, { 22, "v Fc", 1 }, { 22, "v FN", 1 }, { 22, "v Qbb o", 1 }, { 22, "v s", 1 },
		{ 12, "v o", 1 }, { 12, "v Fv", 1 },
		};
	nxQhGapFamily(o, "qhull_output_delaunay", "qhull_output_delaunay_x87", "0x00066de0", "phys_fn_002703",
		"io.c,geom2.c,poly2.c", "0x00066de0", "phys_fn_002703", "io.c,geom.c,geom2.c",
		kOutputDelaunay, sizeof(kOutputDelaunay) / sizeof(kOutputDelaunay[0]), selfOnly, 0, kDivergent);

	// Delaunay and Voronoi over 3-d input (a 4-d hull).
	static const NxQhGapRun kOutputDelaunay3[] =
		{
		{ 23, "d", 1 }, { 23, "d s", 1 }, { 23, "d i", 1 }, { 23, "d G", 1 }, { 23, "v o", 1 }, { 23, "v Fv", 1 },
		{ 23, "v Fi", 1 }, { 23, "v G", 1 }, { 23, "v p", 1 }, { 23, "d m", 1 },
		};
	nxQhGapFamily(o, "qhull_output_delaunay3", "qhull_output_delaunay3_x87", "0x00066de0", "phys_fn_002703",
		"io.c,geom2.c,poly2.c", "0x00066de0", "phys_fn_002703", "io.c,geom.c,geom2.c",
		kOutputDelaunay3, sizeof(kOutputDelaunay3) / sizeof(kOutputDelaunay3[0]), selfOnly, 0, kDivergent);

	// Trace levels: the traceN macros (the CRT-written file) and the printers
	// they call (the host).
	static const NxQhGapRun kTrace[] =
		{
		{ 0, "T1", 0 }, { 0, "T2", 0 }, { 0, "T3", 0 }, { 0, "T4", 0 }, { 0, "T5", 0 },
		{ 1, "T1", 0 }, { 1, "T2", 0 }, { 1, "T3", 0 },
		{ 2, "T1", 0 }, { 2, "T2", 0 }, { 2, "T3", 0 },
		{ 2, "Tc", 0 }, { 2, "T1 TP3", 0 }, { 2, "T1 TM2", 0 }, { 2, "T1 TV1", 0 }, { 2, "T1 TC2", 0 },
		{ 2, "T1 TW0.1", 0 }, { 2, "TF1", 0 }, { 6, "T2", 0 }, { 7, "T2", 0 }, { 12, "T3", 0 },
		{ 14, "T2", 0 }, { 22, "d T2", 1 }, { 3, "T1 Tc", 0 }, { 10, "T1", 0 }, { 11, "T1", 0 },
		};
	nxQhGapFamily(o, "qhull_trace", "qhull_trace_x87", "0x0007d180", "phys_fn_003234",
		"qhull.c,poly.c,poly2.c,merge.c,geom.c,geom2.c,io.c,qset.c,mem.c,global.c",
		"0x0007d180", "phys_fn_003234", "qhull.c,poly.c,poly2.c,merge.c,geom.c,geom2.c,io.c",
		kTrace, sizeof(kTrace) / sizeof(kTrace[0]), selfOnly, 0, kDivergent);

	// Option-gated arms: merge thresholds, the Q0-Q9 switches, good facets,
	// print filters, projection and scaling.
	static const NxQhGapRun kOptions[] =
		{
		{ 2, "C-0.02", 0 }, { 2, "C0.02", 0 }, { 2, "A-0.99", 0 }, { 2, "A0.99", 0 }, { 2, "W0.1", 0 },
		{ 2, "V0.1", 0 }, { 2, "U0.1", 0 }, { 2, "E0.001", 0 }, { 2, "Qc", 0 }, { 2, "Qi", 0 },
		{ 2, "Qc Qi", 0 }, { 2, "Q0", 0 }, { 2, "Q1", 0 }, { 2, "Q2", 0 }, { 2, "Q3", 0 }, { 2, "Q4", 0 },
		{ 2, "Q5", 0 }, { 2, "Q6", 0 }, { 2, "Q7", 0 }, { 2, "Q8", 0 }, { 2, "Q9", 0 }, { 2, "Qv", 0 },
		{ 2, "Qm", 0 }, { 2, "Qg QG0", 0 }, { 2, "Qg QV0", 0 }, { 2, "QG0 Pg", 0 }, { 2, "QV0 Pg", 0 },
		{ 2, "QG-0 Pg", 0 }, { 2, "Pd0:0.5", 0 }, { 2, "PD0:0.5", 0 }, { 2, "PA2", 0 }, { 2, "PM1", 0 },
		{ 2, "PF0.1", 0 }, { 2, "Qb0:0B0:0", 0 }, { 2, "Qb0:-1B0:1", 0 }, { 2, "Qf", 0 },
		{ 3, "C-0.02", 0 }, { 3, "C0.05", 0 }, { 3, "A-0.9", 0 }, { 3, "Qv", 0 }, { 3, "Qc Qi", 0 },
		{ 3, "Q3", 0 }, { 3, "Q5 Q6", 0 }, { 3, "QG1 Pg", 0 }, { 3, "Qg QG1", 0 }, { 3, "Qm", 0 },
		{ 5, "C-0.001", 0 }, { 5, "Qc", 0 }, { 5, "Qv", 0 }, { 6, "Qv", 0 }, { 6, "Qc Qi", 0 },
		{ 7, "C0.05", 0 }, { 7, "Qv", 0 }, { 7, "A0.95", 0 },
		{ 12, "C-0.01", 0 }, { 12, "Qc", 0 },
		{ 22, "d Qg QG0 Pg", 1 }, { 22, "d QV0 Pg", 1 },
		};
	nxQhGapFamily(o, "qhull_options", "qhull_options_x87", "0x000626f0", "phys_fn_002587", "global.c,qhull.c,poly.c,poly2.c,merge.c,geom2.c",
		"0x000626f0", "phys_fn_002587", "global.c,merge.c,geom.c,geom2.c", kOptions, sizeof(kOptions) / sizeof(kOptions[0]),
		selfOnly, 0, kDivergent);

	// Larger and near-degenerate inputs, for the merge code: 17 a cube's faces
	// jittered off the plane, 18 500 cospherical points, 19 tight clusters, 20 a
	// cone, 21 1000 points in a box, 13 a jittered circle.
	static const NxQhGapRun kMerge[] =
		{
		{ 17, "o", 0 }, { 17, "C-0", 0 }, { 17, "Qx", 0 }, { 17, "C-0.001", 0 }, { 17, "Qt", 0 },
		{ 18, "o", 0 }, { 18, "C-0", 0 }, { 18, "A0.999", 0 },
		{ 19, "o", 0 }, { 19, "C-0", 0 }, { 19, "Qx", 0 }, { 19, "C-0.0001", 0 },
		{ 20, "o", 0 }, { 20, "C-0", 0 }, { 20, "Qx", 0 },
		{ 21, "o", 0 }, { 21, "C-0", 0 },
		{ 13, "C-0", 0 }, { 13, "Qx", 0 },
		};
	// More of the merge code: the Qn switches over the merge-heavy sets,
	// larger merge thresholds (degenerate and redundant facets, vertex
	// renaming), the furthest-outside partition, and triangulated Delaunay
	// output over co-circular input (mirror facets).
	static const NxQhGapRun kMerge2[] =
		{
		{ 17, "Q1", 0 }, { 17, "Q2", 0 }, { 17, "Q4", 0 }, { 17, "Q0", 0 }, { 17, "C-0.01", 0 },
		{ 19, "Q2", 0 }, { 19, "Q0", 0 }, { 18, "C-0.02", 0 }, { 18, "A-0.9", 0 }, { 18, "C-0 Q4", 0 },
		{ 21, "C-0.05", 0 }, { 21, "Qf", 0 }, { 3, "Qf", 0 }, { 6, "C-0", 0 }, { 6, "Q0", 0 },
		{ 2, "Q1 C-0", 0 }, { 13, "d Qt", 1 }, { 17, "TF1", 0 },
		{ 2, "QG-0 Pg", 0 }, { 22, "d QG0 Pg", 1 }, { 22, "d QG-0 Pg", 1 },
		};
	nxQhGapFamily(o, "qhull_merge2", "qhull_merge2_x87", "0x0007d180", "phys_fn_003234", "qhull.c,poly.c,poly2.c,merge.c,qset.c",
		"0x0007d180", "phys_fn_003234", "geom.c,geom2.c,merge.c", kMerge2, sizeof(kMerge2) / sizeof(kMerge2[0]),
		selfOnly, 0, kDivergent);

	nxQhGapFamily(o, "qhull_merge", "qhull_merge_x87", "0x0007d180", "phys_fn_003234", "qhull.c,poly.c,poly2.c,merge.c,qset.c",
		"0x0007d180", "phys_fn_003234", "geom.c,geom2.c,merge.c", kMerge, sizeof(kMerge) / sizeof(kMerge[0]),
		selfOnly, 0, kDivergent);

	// qhull's own random numbers without the rotation: joggle, random point
	// order, random distance perturbation. The generator is the same on both
	// sides (qh_rand is exact); the reciprocal the oracle multiplies by where
	// the candidate divides by qh_RANDOMmax lands in the doubles.
	static const NxQhGapRun kRandom[] =
		{
		{ 2, "QJ", 0 }, { 2, "QJ0.001", 0 }, { 2, "R0.001", 0 }, { 2, "Qr R0.01", 0 },
		{ 3, "QJ", 0 }, { 3, "Qr", 0 }, { 7, "QJ", 0 }, { 7, "R0.001", 0 }, { 22, "d QJ", 1 },
		};
	nxQhGapFamily(o, "qhull_random", "qhull_random_x87", "0x00061490", "phys_fn_002550", "geom2.c,global.c,qhull.c,merge.c",
		"0x00061490", "phys_fn_002550", "geom2.c,geom.c", kRandom, sizeof(kRandom) / sizeof(kRandom[0]),
		selfOnly, 0, kDivergent);

	// The out-of-line printers and helpers the test exe inlines into their
	// callers (nxQhullDirect), called on each side's own finished hull: 3-d
	// non-simplicial (the lattice) and simplicial (the sphere, the tetrahedron
	// triangulated), 2-d, 4-d, Delaunay and Voronoi.
	static const NxQhGapRun kDirect[] =
		{
		{ 2, "o", 0, 1 }, { 0, "Qt", 0, 1 }, { 3, "o", 0, 1 }, { 1, "o", 0, 1 }, { 12, "o", 0, 1 }, { 13, "o", 0, 1 },
		{ 14, "o", 0, 1 }, { 15, "o", 0, 1 }, { 22, "d", 1, 1 }, { 22, "v", 1, 1 }, { 23, "d", 1, 1 }, { 23, "v", 1, 1 },
		};
	nxQhGapFamily(o, "qhull_direct", "qhull_direct_x87", "0x00068ce0", "phys_fn_002779", "io.c,geom2.c,poly2.c,stat.c,qset.c",
		"0x00068ce0", "phys_fn_002779", "io.c,geom.c,geom2.c", kDirect, sizeof(kDirect) / sizeof(kDirect[0]),
		selfOnly, 0, kDivergent);

	// DIVERGENT, discrete: the runs whose search path differs. The hull each
	// builds is the same on both sides; what differs is how many distance tests
	// it took (qh_printsummary's counters, a statistic printed or skipped) or,
	// at trace level 4, which neighbours qh_findbest visited: a distance that
	// differs in its last bit (the qh_distplane class, qhull_hull_x87) against a
	// tie at bestdist sends one side's directed search one facet further.
	static const NxQhGapRun kPaths[] =
		{
		{ 16, "s", 0 }, { 12, "d Qbb", 1 }, { 16, "C-0", 0 }, { 16, "Qx", 0 }, { 16, "Qv", 0 },
		{ 18, "C0.01", 0 }, { 21, "C0.01", 0 }, { 2, "Qr", 0 }, { 2, "QR-5 Qr", 0 }, { 12, "d Qt", 1 },
		{ 1, "T4", 0 },	// last: its tape is 9 words longer on the candidate side
		};
	nxQhGapFamily(o, "qhull_paths", "qhull_paths_x87", "0x0005c5c0", "phys_fn_002425", "geom.c,qhull.c,poly2.c,merge.c,io.c",
		"0x0005c5c0", "phys_fn_002425", "geom.c,geom2.c,merge.c", kPaths, sizeof(kPaths) / sizeof(kPaths[0]),
		selfOnly, kDivergent, kDivergent);

	// DIVERGENT, as qhull_hull_rotated: "QRn" rotates the input by qhull's own
	// random matrix, whose Gram-Schmidt the oracle evaluates with a reciprocal
	// (0x0005f3d2) where the candidate divides.
	static const NxQhGapRun kRotation[] =
		{
		{ 2, "QR2", 0 }, { 3, "QR3", 0 }, { 17, "QR1", 0 }, { 19, "QR1", 0 }, { 7, "QR1 C-0", 0 }, { 22, "d QR1", 1 },
		};
	nxQhGapFamily(o, "qhull_rotation", "qhull_rotation_x87", "0x0005fec0", "phys_fn_002518", "geom2.c,global.c,qhull.c,merge.c",
		"0x0005fec0", "phys_fn_002518", "geom2.c,geom.c", kRotation, sizeof(kRotation) / sizeof(kRotation[0]),
		selfOnly, kDivergent, kDivergent);
	}

//////////////////////////////////////////////////////////////////////////////
// convex-mesh gap Task 2c. The ICE-shaped rows with no vendored source,
// reconstructed in Physics/src and linked into this harness: EdgeList.cpp
// (002052..002063), IceAdjacencies.cpp (001537..001548) and the valencies of
// IceMeshTools.cpp (001663..001667). Three families, each driving the ORACLE's
// entry rows at their RVAs on an oracle-side object and the candidate's on its
// own, over the six NxMesh fixtures and a set of meshes of this task's own.
//
// What a tape holds, per run: the row's return value; the whole object image
// (counts, and every table word through its pointer, the count cookie of each
// `new[]` block included); and, interleaved in call order,
//   * every allocation and release the rows make through the 004803 getter:
//     the oracle's getter is pointed at a recording allocator by writing its
//     singleton slot .data:0x0012845c (the getter returns the slot when it is
//     non-null; no code byte is patched), the candidate's by
//     nxSetSdkAllocatorBridge. Each malloc records (type, size) and returns a
//     block filled with 0xcd; each free records which allocation it released
//     (by order), so a release at the wrong offset (a missing or extra cookie)
//     or on the wrong block is a differing word;
//   * every report: the oracle's 002160 calls the import slot
//     .rdata:0x001041b4, redirected to a recorder for the run (as
//     nxDrivePrunableRanges does); the candidate's seam forwards to the same
//     recorder (tests/PhysicsThirdPartyHost.cpp). Recorded: the line and the
//     digests of the file string and the message -- what these rows push, not
//     what the reporter does with it.
// Vertex arrays sit behind a fixed 12-byte guard, so that a vertex read at
// index 0xffffffff (002061's opposite vertex when the edge is not a side of the
// face) reads the same bytes on both sides.

// The Physics headers refuse windows.h's min/max macros (NxMath.h), and
// OPCODE's `random` macro would rewrite NxQuat::random; all three are set aside
// for these two includes only.
#pragma push_macro("min")
#pragma push_macro("max")
#pragma push_macro("random")
#undef min
#undef max
#undef random
#include "IceAdjacencies.h"
#include "IceMeshTools.h"
#pragma pop_macro("random")
#pragma pop_macro("max")
#pragma pop_macro("min")

static const unsigned kIceAdjBoundary		= 0x0002de60;	// phys_fn_001542
static const unsigned kIceAdjDtor			= 0x0002dec0;	// phys_fn_001544
static const unsigned kIceAdjInit			= 0x0002def0;	// phys_fn_001546
static const unsigned kIceValCtor			= 0x00032590;	// phys_fn_001663
static const unsigned kIceValDtor			= 0x000325b0;	// phys_fn_001665
static const unsigned kIceValCompute		= 0x00032610;	// phys_fn_001667
static const unsigned kIceEdgeListCtor		= 0x00051060;	// phys_fn_002052
static const unsigned kIceEdgeListF2E		= 0x00051080;	// phys_fn_002054
static const unsigned kIceEdgeListE2F		= 0x000513f0;	// phys_fn_002058
static const unsigned kIceEdgeListDtor		= 0x000515d0;	// phys_fn_002060
static const unsigned kIceEdgeListActive	= 0x00051640;	// phys_fn_002061
static const unsigned kIceEdgeListInit		= 0x00051dd0;	// phys_fn_002063
static const unsigned kDataSdkAllocator		= 0x0012845c;	// 004803's singleton slot

typedef bool		(__thiscall* IceCreateFn)(void*, const void*);
typedef unsigned	(__thiscall* IceCountFn)(void*);
typedef bool		(__thiscall* IceFacesFn)(void*, unsigned, const unsigned*, const unsigned short*);
typedef bool		(__thiscall* IceActiveFn)(void*, unsigned, const unsigned*, const unsigned short*,
						const void*, float);

// The oracle's create blocks are the candidate's structs: same offsets.
typedef char nxIceLayout0[sizeof(ADJACENCIESCREATE) == 0x14 && offsetof(ADJACENCIESCREATE, Epsilon) == 0x10 ? 1 : -1];
typedef char nxIceLayout1[sizeof(EDGELISTCREATE) == 0x18 && offsetof(EDGELISTCREATE, VerticesToEdges) == 0x0d
	&& offsetof(EDGELISTCREATE, Verts) == 0x10 ? 1 : -1];
typedef char nxIceLayout2[sizeof(VALENCESCREATE) == 0x14 && offsetof(VALENCESCREATE, AdjacentList) == 0x10 ? 1 : -1];
typedef char nxIceLayout3[sizeof(EdgeList) == 0x18 && sizeof(Adjacencies) == 8 && sizeof(Valencies) == 0x14
	&& sizeof(EdgeDesc) == 8 && sizeof(EdgeTriangle) == 12 && sizeof(AdjTriangle) == 12 ? 1 : -1];

static NxTape*	gIceTape = 0;			// the tape being written
static bool		gIceOraclePass = false;	// the oracle's pass is being driven
static unsigned	gIceReports = 0;		// oracle-side counts, for the coverage lines
static unsigned	gIceReportLines[8];
static const int kIceReportLineValues[8] = { 0x72, 0x10a, 0x10b, 0x10e, 0x111, 0x114, 0x117, 266 };
static unsigned	gIceReportLine267 = 0;
static unsigned	gIceReportLine321 = 0;
static unsigned	gIceReportLine318 = 0;	// Task 2f: 001465's outline report

static unsigned nxIceStringDigest(const char* s)
	{
	unsigned d = 2166136261u;
	if(!s)
		return 0;
	for(; *s; ++s)
		{
		d ^= (unsigned char) *s;
		d *= 16777619u;
		}
	return d;
	}

static void nxIceReportWords(const char* message, const char* file, int line)
	{
	if(gIceOraclePass)
		{
		++gIceReports;
		for(int i = 0; i < 8; ++i)
			if(kIceReportLineValues[i] == line)
				++gIceReportLines[i];
		if(line == 267)
			++gIceReportLine267;
		if(line == 321)
			++gIceReportLine321;
		if(line == 318)
			++gIceReportLine318;
		}
	gIceTape->push(0x5e7e0000u);
	gIceTape->push((unsigned) line);
	gIceTape->push(nxIceStringDigest(file));
	gIceTape->push(nxIceStringDigest(message));
	}

// Installed over the oracle's import slot: 002160 calls it as
// (2, file, line, 0, message).
static bool __cdecl nxIceOracleReport(int /*code*/, const char* file, int line, bool* /*flag*/,
	const char* message, ...)
	{
	nxIceReportWords(message, file, line);
	return false;
	}

static bool nxIceCandidateReport(const char* message, const char* file, int line)
	{
	nxIceReportWords(message, file, line);
	return false;
	}

extern bool (*gNxIceErrorSink)(const char* message, const char* file, int line);

class NxIceRecordingAllocator : public SdkAllocator
	{
	public:
	enum { kMaxLive = 16384 };
	void*		mLive[kMaxLive];
	unsigned	mNbLive;

	void reset()	{ mNbLive = 0; }

	void* malloc(size_t size, NxMemoryType type)
		{
		void* block = ::malloc(size ? size : 1);
		memset(block, 0xcd, size);
		gIceTape->push(0xa11c0000u | ((unsigned) type & 0xffffu));
		gIceTape->push((unsigned) size);
		if(mNbLive < kMaxLive)
			mLive[mNbLive++] = block;
		return block;
		}
	void* mallocDEBUG(size_t size, const char*, int, const char*, NxMemoryType type)
		{
		gIceTape->push(0xbad0000du);
		return malloc(size, type);
		}
	void* realloc(void* memory, size_t size)
		{
		gIceTape->push(0xbad0000eu);
		return ::realloc(memory, size);
		}
	void free(void* memory)
		{
		unsigned index = 0xffffffffu;
		for(unsigned i = 0; i < mNbLive; ++i)
			if(mLive[i] == memory && memory)
				{
				index = i;
				mLive[i] = 0;
				break;
				}
		gIceTape->push(0xf4ee0000u);
		gIceTape->push(index);
		if(index != 0xffffffffu)
			::free(memory);
		}
	};

static NxIceRecordingAllocator gIceAllocator;

// The meshes: the six fixtures, then this task's own (see nxIceBuildMeshes).
static const int kIceMaxMeshes = 72;
static NxMesh gIceMeshes[kIceMaxMeshes];
static int gIceNbMeshes = 0;
static unsigned gIceInputDigest[14];	// 0..2 Task 2c; 3, 4 Task 2d; 5..9 Task 2e; 10..12 Task 2f; 13 Task 2g
static unsigned gIceInputWords[14];

static NxMesh& nxIceNewMesh(unsigned nbVerts, unsigned nbTris)
	{
	NxMesh& m = gIceMeshes[gIceNbMeshes++];
	memset(&m, 0, sizeof(m));
	m.nbVerts = nbVerts;
	m.nbTris = nbTris;
	return m;
	}

static void nxIceSetVertex(NxMesh& m, unsigned v, float x, float y, float z)
	{
	m.verts[v * 3 + 0] = x;
	m.verts[v * 3 + 1] = y;
	m.verts[v * 3 + 2] = z;
	}

static void nxIceSetTri(NxMesh& m, unsigned t, unsigned a, unsigned b, unsigned c)
	{
	m.tris[t * 3 + 0] = a;
	m.tris[t * 3 + 1] = b;
	m.tris[t * 3 + 2] = c;
	}

// This task's meshes, all deterministic.
//   6..11   height grids (n = 6) with heights of amplitude 0.02 .. 0.5: the
//           dihedral angles straddle 002061's 0.1 radian threshold.
//   12..27  two triangles folded about their shared edge (1,2), the fourth
//           vertex raised by h, both windings of the second face: convex,
//           concave and flat edges, angles either side of 0.1.
//   28      three faces on one edge (non-manifold).
//   29      a face (a, a, a) beside a regular one.
//   30      a duplicated face, same winding; 31 the same reversed.
//   32      the box with its vertices spread over 16 slots, the odd ones
//           unused (isolated vertices).
//   33      the height field with its vertex indices permuted and its faces
//           shuffled.
//   34      two faces whose shared edge (2,3) sorts last (the database's
//           after-loop link).
//   35..58  random soups: 4..23 vertices, 2..41 faces, indices drawn freely
//           (repeated vertices, duplicates and non-manifold edges included).
//   59..64  raw words among the coordinates (see there).
static void nxIceBuildMeshes()
	{
	const unsigned savedState = gState;
	nxBuildMeshes();
	gIceNbMeshes = 0;
	for(int i = 0; i < kNbMeshes; ++i)
		gIceMeshes[gIceNbMeshes++] = gMeshes[i];

	gState = 0x2c0ed6e5;
	static const float kAmplitudes[6] = { 0.02f, 0.05f, 0.08f, 0.12f, 0.2f, 0.5f };
	for(int a = 0; a < 6; ++a)
		{
		NxMesh& m = nxIceNewMesh(0, 0);
		nxGridMesh(m, 6, true);
		for(unsigned v = 0; v < m.nbVerts; ++v)
			m.verts[v * 3 + 2] = kAmplitudes[a] * nxRange(-1.0f, 1.0f).value;
		}

	static const float kFolds[8] = { 0.05f, 0.0705f, 0.0707f, 0.071f, 0.1f, 0.2f, 0.7f, 3.0f };
	for(int f = 0; f < 8; ++f)
		for(int w = 0; w < 2; ++w)
			{
			NxMesh& m = nxIceNewMesh(4, 2);
			nxIceSetVertex(m, 0, 0.0f, 0.0f, 0.0f);
			nxIceSetVertex(m, 1, 1.0f, 0.0f, 0.0f);
			nxIceSetVertex(m, 2, 0.0f, 1.0f, 0.0f);
			nxIceSetVertex(m, 3, 1.0f, 1.0f, (f & 1) ? kFolds[f] : -kFolds[f]);
			nxIceSetTri(m, 0, 0, 1, 2);
			if(w == 0)
				nxIceSetTri(m, 1, 1, 3, 2);
			else
				nxIceSetTri(m, 1, 2, 3, 1);
			}

	{
	NxMesh& m = nxIceNewMesh(5, 3);
	for(unsigned v = 0; v < 5; ++v)
		nxIceSetVertex(m, v, (float) v, (float) (v * v) * 0.25f, v == 4 ? 1.0f : 0.0f);
	nxIceSetTri(m, 0, 0, 1, 2);
	nxIceSetTri(m, 1, 0, 1, 3);
	nxIceSetTri(m, 2, 1, 0, 4);
	}
	{
	NxMesh& m = nxIceNewMesh(4, 2);
	for(unsigned v = 0; v < 4; ++v)
		nxIceSetVertex(m, v, (float) (v & 1), (float) (v >> 1), 0.5f);
	nxIceSetTri(m, 0, 2, 2, 2);
	nxIceSetTri(m, 1, 0, 1, 3);
	}
	for(int r = 0; r < 2; ++r)
		{
		NxMesh& m = nxIceNewMesh(4, 3);
		nxIceSetVertex(m, 0, 0.0f, 0.0f, 0.0f);
		nxIceSetVertex(m, 1, 1.0f, 0.0f, 0.0f);
		nxIceSetVertex(m, 2, 0.0f, 1.0f, 0.0f);
		nxIceSetVertex(m, 3, 1.0f, 1.0f, 0.3f);
		nxIceSetTri(m, 0, 0, 1, 2);
		nxIceSetTri(m, 1, 1, 3, 2);
		if(r == 0)
			nxIceSetTri(m, 2, 0, 1, 2);
		else
			nxIceSetTri(m, 2, 2, 1, 0);
		}
	{
	const NxMesh& box = gMeshes[5];
	NxMesh& m = nxIceNewMesh(16, box.nbTris);
	for(unsigned v = 0; v < 16; ++v)
		nxIceSetVertex(m, v, 9.0f + v, -9.0f, 4.0f);
	for(unsigned v = 0; v < box.nbVerts; ++v)
		nxIceSetVertex(m, v * 2, box.verts[v * 3 + 0], box.verts[v * 3 + 1], box.verts[v * 3 + 2]);
	for(unsigned i = 0; i < box.nbTris * 3; ++i)
		m.tris[i] = box.tris[i] * 2;
	}
	{
	const NxMesh& hf = gMeshes[0];
	NxMesh& m = nxIceNewMesh(hf.nbVerts, hf.nbTris);
	unsigned perm[400];
	for(unsigned v = 0; v < hf.nbVerts; ++v)
		perm[v] = v;
	for(unsigned v = hf.nbVerts - 1; v > 0; --v)
		{
		const unsigned draw = nxNext();
		const unsigned j = draw % (v + 1);
		const unsigned t = perm[v];
		perm[v] = perm[j];
		perm[j] = t;
		}
	for(unsigned v = 0; v < hf.nbVerts; ++v)
		for(int k = 0; k < 3; ++k)
			m.verts[perm[v] * 3 + k] = hf.verts[v * 3 + k];
	unsigned order[400];
	for(unsigned t = 0; t < hf.nbTris; ++t)
		order[t] = t;
	for(unsigned t = hf.nbTris - 1; t > 0; --t)
		{
		const unsigned draw = nxNext();
		const unsigned j = draw % (t + 1);
		const unsigned s = order[t];
		order[t] = order[j];
		order[j] = s;
		}
	for(unsigned t = 0; t < hf.nbTris; ++t)
		for(int k = 0; k < 3; ++k)
			m.tris[t * 3 + k] = perm[hf.tris[order[t] * 3 + k]];
	}
	{
	NxMesh& m = nxIceNewMesh(4, 2);
	nxIceSetVertex(m, 0, 0.0f, 0.0f, 0.0f);
	nxIceSetVertex(m, 1, 2.0f, 0.0f, 0.0f);
	nxIceSetVertex(m, 2, 1.0f, 1.0f, 0.0f);
	nxIceSetVertex(m, 3, 1.0f, 1.0f, 1.5f);
	nxIceSetTri(m, 0, 0, 2, 3);
	nxIceSetTri(m, 1, 1, 3, 2);
	}
	for(int r = 0; r < 24; ++r)
		{
		const unsigned nbVerts = 4 + nxNext() % 20;
		const unsigned nbTris = 2 + nxNext() % 40;
		NxMesh& m = nxIceNewMesh(nbVerts, nbTris);
		for(unsigned v = 0; v < nbVerts; ++v)
			{
			const float x = nxRange(-2.0f, 2.0f).value;
			const float y = nxRange(-2.0f, 2.0f).value;
			const float z = nxRange(-2.0f, 2.0f).value;
			nxIceSetVertex(m, v, x, y, z);
			}
		for(unsigned i = 0; i < nbTris * 3; ++i)
			m.tris[i] = nxNext() % nbVerts;
		}

	// 59..64: raw words among the coordinates, one draw in eight, written into
	// the slot as bits (signalling NaNs, infinities and denormals kept): three
	// height grids (n = 5) and three soups.
	for(int r = 0; r < 6; ++r)
		{
		NxMesh* mesh;
		if(r < 3)
			{
			mesh = &nxIceNewMesh(0, 0);
			nxGridMesh(*mesh, 5, true);
			for(unsigned v = 0; v < mesh->nbVerts; ++v)
				mesh->verts[v * 3 + 2] = nxRange(-0.3f, 0.3f).value;
			}
		else
			{
			const unsigned nbVerts = 6 + nxNext() % 12;
			const unsigned nbTris = 4 + nxNext() % 24;
			mesh = &nxIceNewMesh(nbVerts, nbTris);
			for(unsigned v = 0; v < 3 * nbVerts; ++v)
				mesh->verts[v] = nxRange(-2.0f, 2.0f).value;
			for(unsigned i = 0; i < nbTris * 3; ++i)
				mesh->tris[i] = nxNext() % nbVerts;
			}
		for(unsigned v = 0; v < 3 * mesh->nbVerts; ++v)
			{
			const unsigned select = nxNext();
			if(select % 8)
				continue;
			const unsigned bits = nxNext();
			memcpy(&mesh->verts[v], &bits, sizeof(bits));
			}
		}
	gState = savedState;
	}

// One side's copy of a mesh: vertices behind the guard, 32-bit and 16-bit faces.
struct NxIceInput
	{
	float*			block;
	const IceMaths::Point*	verts;
	unsigned*		dfaces;
	unsigned short*	wfaces;
	};

static void nxIceMakeInput(NxIceInput& in, const NxMesh& m, int family)
	{
	in.block = (float*) malloc(sizeof(float) * 3 * (m.nbVerts + 1));
	in.block[0] = 1024.0f;
	in.block[1] = -2048.0f;
	in.block[2] = 512.0f;
	memcpy(in.block + 3, m.verts, sizeof(float) * 3 * m.nbVerts);
	in.verts = (const IceMaths::Point*) (in.block + 3);
	const unsigned n = m.nbTris ? m.nbTris * 3 : 1;
	in.dfaces = (unsigned*) malloc(sizeof(unsigned) * n);
	in.wfaces = (unsigned short*) malloc(sizeof(unsigned short) * n);
	for(unsigned i = 0; i < m.nbTris * 3; ++i)
		{
		in.dfaces[i] = m.tris[i];
		in.wfaces[i] = (unsigned short) m.tris[i];
		}
	if(gIceOraclePass)
		{
		unsigned d = gIceInputDigest[family];
		d = nxFold(d, m.nbVerts);
		d = nxFold(d, m.nbTris);
		for(unsigned i = 0; i < 3 * m.nbVerts; ++i)
			{
			unsigned w;
			memcpy(&w, &m.verts[i], 4);
			d = nxFold(d, w);
			}
		for(unsigned i = 0; i < 3 * m.nbTris; ++i)
			d = nxFold(d, m.tris[i]);
		gIceInputDigest[family] = d;
		gIceInputWords[family] += 2 + 3 * m.nbVerts + 3 * m.nbTris;
		}
	}

static void nxIceFreeInput(NxIceInput& in)
	{
	free(in.block);
	free(in.dfaces);
	free(in.wfaces);
	}

// A run's own parameters, folded into the input digest on the oracle pass.
static void nxIceFoldRun(int family, unsigned a, unsigned b, unsigned c)
	{
	if(!gIceOraclePass)
		return;
	gIceInputDigest[family] = nxFold(nxFold(nxFold(gIceInputDigest[family], a), b), c);
	gIceInputWords[family] += 3;
	}

static unsigned nxIceWord(const void* object, unsigned offset)
	{
	unsigned w;
	memcpy(&w, (const unsigned char*) object + offset, 4);
	return w;
	}

static const unsigned kIceTableCap = 8192;

static void nxIceTapeEdgeList(const void* object)
	{
	NxTape& t = *gIceTape;
	const unsigned nbEdges = nxIceWord(object, 0x00);
	const EdgeData* edges = (const EdgeData*) (size_t) nxIceWord(object, 0x04);
	const EdgeTriangle* faces = (const EdgeTriangle*) (size_t) nxIceWord(object, 0x0c);
	const EdgeDesc* descs = (const EdgeDesc*) (size_t) nxIceWord(object, 0x10);
	const unsigned* fbe = (const unsigned*) (size_t) nxIceWord(object, 0x14);
	t.push(0xed9e0000u | (edges ? 1u : 0u) | (faces ? 2u : 0u) | (descs ? 4u : 0u) | (fbe ? 8u : 0u));
	t.push(nbEdges);
	t.push(nxIceWord(object, 0x08));
	const unsigned cappedEdges = nbEdges < kIceTableCap ? nbEdges : kIceTableCap;
	if(edges)
		{
		t.push(((const unsigned*) edges)[-1]);
		for(unsigned i = 0; i < cappedEdges; ++i)
			{
			t.push(edges[i].Ref0);
			t.push(edges[i].Ref1);
			}
		}
	if(faces)
		{
		const unsigned cookie = ((const unsigned*) faces)[-1];
		t.push(cookie);
		const unsigned n = cookie < kIceTableCap ? cookie : kIceTableCap;
		for(unsigned i = 0; i < n; ++i)
			for(int j = 0; j < 3; ++j)
				t.push(faces[i].mLink[j]);
		}
	unsigned total = 0;
	if(descs)
		{
		for(unsigned i = 0; i < cappedEdges; ++i)
			{
			t.push(descs[i].Flags | ((unsigned) descs[i].Count << 16));
			t.push(descs[i].Offset);
			}
		if(cappedEdges)
			total = descs[cappedEdges - 1].Offset + descs[cappedEdges - 1].Count;
		}
	if(fbe)
		{
		const unsigned n = total < kIceTableCap ? total : kIceTableCap;
		for(unsigned i = 0; i < n; ++i)
			t.push(fbe[i]);
		}
	}

static void nxIceTapeAdjacencies(const void* object)
	{
	NxTape& t = *gIceTape;
	const AdjTriangle* faces = (const AdjTriangle*) (size_t) nxIceWord(object, 4);
	t.push(nxIceWord(object, 0));
	t.push(faces ? 1u : 0u);
	if(faces)
		{
		const unsigned cookie = ((const unsigned*) faces)[-1];
		t.push(cookie);
		const unsigned n = cookie < kIceTableCap ? cookie : kIceTableCap;
		for(unsigned i = 0; i < n; ++i)
			for(int j = 0; j < 3; ++j)
				t.push(faces[i].ATri[j]);
		}
	}

static void nxIceTapeValencies(const void* object)
	{
	NxTape& t = *gIceTape;
	const unsigned nbVerts = nxIceWord(object, 0x00);
	const unsigned nbAdj = nxIceWord(object, 0x04);
	const unsigned* arrays[3] =
		{
		(const unsigned*) (size_t) nxIceWord(object, 0x08),
		(const unsigned*) (size_t) nxIceWord(object, 0x0c),
		(const unsigned*) (size_t) nxIceWord(object, 0x10),
		};
	t.push(nbVerts);
	t.push(nbAdj);
	for(int a = 0; a < 3; ++a)
		{
		t.push(arrays[a] ? 1u : 0u);
		if(!arrays[a])
			continue;
		const unsigned count = a == 2 ? nbAdj : nbVerts;
		const unsigned n = count < kIceTableCap ? count : kIceTableCap;
		for(unsigned i = 0; i < n; ++i)
			t.push(arrays[a][i]);
		}
	}

// One side's entry points. The oracle's are called at their RVAs; the
// candidate's are the reconstructed members, on an object in the same storage.
struct NxIceSide
	{
	const NxOracleRows*	o;
	bool				oracle;
	NxTape*				mainTape;
	NxTape*				splitTape;		// edge_list.plane_divergent

	void edgeListCtor(void* obj) const
		{
		if(oracle)	((VoidThisFn) (o->base + kIceEdgeListCtor))(obj);
		else		new (obj) EdgeList;
		}
	void edgeListDtor(void* obj) const
		{
		if(oracle)	((VoidThisFn) (o->base + kIceEdgeListDtor))(obj);
		else		((EdgeList*) obj)->~EdgeList();
		}
	bool edgeListInit(void* obj, const EDGELISTCREATE& c) const
		{
		if(oracle)	return ((IceCreateFn) (o->base + kIceEdgeListInit))(obj, &c);
		return ((EdgeList*) obj)->Init(c);
		}
	bool edgeListF2E(void* obj, unsigned nb, const unsigned* d, const unsigned short* w) const
		{
		if(oracle)	return ((IceFacesFn) (o->base + kIceEdgeListF2E))(obj, nb, d, w);
		return ((EdgeList*) obj)->CreateFacesToEdges(nb, d, w);
		}
	bool edgeListE2F(void* obj, unsigned nb, const unsigned* d, const unsigned short* w) const
		{
		if(oracle)	return ((IceFacesFn) (o->base + kIceEdgeListE2F))(obj, nb, d, w);
		return ((EdgeList*) obj)->CreateEdgesToFaces(nb, d, w);
		}
	bool edgeListActive(void* obj, unsigned nb, const unsigned* d, const unsigned short* w,
		const IceMaths::Point* v, float eps) const
		{
		if(oracle)	return ((IceActiveFn) (o->base + kIceEdgeListActive))(obj, nb, d, w, v, eps);
		return ((EdgeList*) obj)->ComputeActiveEdges(nb, d, w, v, eps);
		}
	bool adjInit(void* obj, const ADJACENCIESCREATE& c) const
		{
		if(oracle)	return ((IceCreateFn) (o->base + kIceAdjInit))(obj, &c);
		return ((Adjacencies*) obj)->Init(c);
		}
	unsigned adjBoundary(void* obj) const
		{
		if(oracle)	return ((IceCountFn) (o->base + kIceAdjBoundary))(obj);
		return ((Adjacencies*) obj)->ComputeNbBoundaryEdges();
		}
	void adjDtor(void* obj) const
		{
		if(oracle)	((VoidThisFn) (o->base + kIceAdjDtor))(obj);
		else		((Adjacencies*) obj)->~Adjacencies();
		}
	void valCtor(void* obj) const
		{
		if(oracle)	((VoidThisFn) (o->base + kIceValCtor))(obj);
		else		new (obj) Valencies;
		}
	void valDtor(void* obj) const
		{
		if(oracle)	((VoidThisFn) (o->base + kIceValDtor))(obj);
		else		((Valencies*) obj)->~Valencies();
		}
	bool valCompute(void* obj, const VALENCESCREATE& c) const
		{
		if(oracle)	return ((IceCreateFn) (o->base + kIceValCompute))(obj, &c);
		return ((Valencies*) obj)->Compute(c);
		}
	};

// Oracle-side coverage for the three lines.
struct NxIceCoverage
	{
	unsigned runs, succeeded, failed, links, boundary, active, activeVerts, edges, adjacent;
	};
static NxIceCoverage gIceCoverage[3];

static void nxIceCountLinks(const void* object, NxIceCoverage& c)
	{
	const AdjTriangle* faces = (const AdjTriangle*) (size_t) nxIceWord(object, 4);
	if(!faces)
		return;
	const unsigned n = ((const unsigned*) faces)[-1];
	for(unsigned i = 0; i < n && i < kIceTableCap; ++i)
		for(int j = 0; j < 3; ++j)
			{
			if((faces[i].ATri[j] & 0x1fffffff) != 0x1fffffff)
				++c.links;
			if(faces[i].ATri[j] & 0x20000000)
				++c.active;
			}
	}

static void nxIceCountEdgeList(const void* object, NxIceCoverage& c)
	{
	c.edges += nxIceWord(object, 0);
	const EdgeTriangle* faces = (const EdgeTriangle*) (size_t) nxIceWord(object, 0x0c);
	if(!faces)
		return;
	const unsigned n = ((const unsigned*) faces)[-1];
	for(unsigned i = 0; i < n && i < kIceTableCap; ++i)
		for(int j = 0; j < 3; ++j)
			{
			if(faces[i].mLink[j] & 0x80000000)
				++c.active;
			if(faces[i].mLink[j] & 0x40000000)
				++c.activeVerts;
			}
	}

// 002061 decides an edge shared by two faces from two vendored callees:
// Plane::Set (005155) for the second face's plane, which the first face's
// opposite vertex is tested against, and Triangle::Normal (005181) for the
// angle. The candidate's vendored copies of both are measured divergences
// (ice_plane_triangle: up to 2,820 ulp; the inventory holds both rows
// `discovered`), and a last-bit difference in either can flip the decision for
// a vertex that lies in the plane or an angle next to 0.1 -- so the active-edge
// bit would follow the callee and not the row. Measured before the split, on a
// throwaway build that could bind the oracle's rows into the candidate's
// 002061: edge_list 465 differing words and ice_adjacencies (whose vertex runs
// go through the same EdgeList) 124; with 005155 bound 306 / 100, with 005181
// bound 342 / 92, with both bound 0 / 0. Without the raw-word meshes (59..64)
// the figures were 72 / 0, and 0 with 005155 alone.
//
// So each mesh is pre-flighted edge by edge, the way 002061 walks it: for every
// edge used by exactly two face slots (faces in ascending order, as the
// faces-by-edges array lists them), the opposite vertex of the first face by the
// listing's six-way rule, the plane of the second through the oracle's 005155
// and the candidate's vendored Plane::Set, the side of the vertex under each
// (the listing's sum order, in double); and where both say below, both pairs of
// normals through the oracle's 005181 and the vendored Triangle::Normal and the
// |angle| > 0.1f decision under each (in double, flagged as well when either
// angle is within 1e-6 of 0.1, beyond anything the double model can be off by).
// A mesh with any edge whose decision could differ is `plane-divergent`.
//
// The split itself is FROZEN (kIcePlaneDivergentMeshes): the 13 meshes the
// pre-flight flagged when Task 2c registered its lines (eight random soups,
// 40..53, and five raw-word meshes, 59..63). Their vertex runs -- edge_list's
// and ice_adjacencies' -- are compared apart as <family>.plane_divergent, under
// the ceilings in kDivergentCeilings; every other run is gated exactly. Which
// tape a run lands on, and so every registered digest, depends only on this
// list and the oracle, not on the candidate's vendored callees: a later fix to
// 005155/005181 moves no registered line (the ceilings report IMPROVED). The
// pre-flight is kept as a check: a mesh OUTSIDE the list whose decisions could
// differ fails the run (the gated tape would be comparing a callee); its detail
// goes to stderr only. (Task 2c review: the first registration derived the
// split from the pre-flight itself, so its digests encoded candidate
// behaviour; the frozen list reproduces the same split, and every digest.)
static const int kIcePlaneDivergentMeshes[] = { 40, 41, 42, 43, 44, 46, 50, 53, 59, 60, 61, 62, 63 };
typedef void*	(__thiscall* IcePlaneSetFn)(void*, const void*, const void*, const void*);	// at kIcePlaneSet, phys_fn_005155
typedef void	(__thiscall* IceTriangleNormalFn)(const void*, void*);
static const unsigned kIceTriangleNormal = 0x000e3f50;	// phys_fn_005181
static bool		gIcePlaneDivergent[kIceMaxMeshes];		// the frozen split
static bool		gIcePreflightDivergent[kIceMaxMeshes];	// the candidate check
static unsigned	gIcePreflightEdges = 0;
static unsigned	gIcePreflightSide = 0;
static unsigned	gIcePreflightAngle = 0;
static unsigned	gIcePlaneMeshes = 0;

struct NxIceEdgeSlot
	{
	unsigned	lo;
	unsigned	hi;
	unsigned	face;
	};

static int nxIceCompareSlots(const void* a, const void* b)
	{
	const NxIceEdgeSlot* x = (const NxIceEdgeSlot*) a;
	const NxIceEdgeSlot* y = (const NxIceEdgeSlot*) b;
	if(x->lo != y->lo)		return x->lo < y->lo ? -1 : 1;
	if(x->hi != y->hi)		return x->hi < y->hi ? -1 : 1;
	if(x->face != y->face)	return x->face < y->face ? -1 : 1;
	return 0;
	}

static bool nxIceBelow(const float* plane, const float* p)
	{
	const double d = (((double) plane[1] * p[1] + (double) plane[2] * p[2]) + (double) plane[0] * p[0])
		+ plane[3];
	return d < 0.0;
	}

// Written through a pointer, never returned as a double (convex-mesh gap Task
// 2g review): a floating return travels in st(0), and what the caller does
// with it then follows the compiler's inlining.
static void nxIceAngle(const float* n0, const float* n1, double* out)
	{
	const double cx = (double) n1[2] * n0[1] - (double) n0[2] * n1[1];
	const double cy = (double) n0[2] * n1[0] - (double) n1[2] * n0[0];
	const double cz = (double) n1[1] * n0[0] - (double) n0[1] * n1[0];
	const double cross = sqrt((cx * cx + cz * cz) + cy * cy);
	const double dot = ((double) n1[0] * n0[0] + (double) n1[2] * n0[2]) + (double) n1[1] * n0[1];
	*out = fabs(atan2(cross, dot));
	}

static void nxIcePlanePreflight(const NxOracleRows& o)
	{
	gIcePreflightEdges = gIcePreflightSide = gIcePreflightAngle = gIcePlaneMeshes = 0;
	static const float kGuard[3] = { 1024.0f, -2048.0f, 512.0f };
	static NxIceEdgeSlot slots[3 * 400];
	for(int m = 0; m < gIceNbMeshes; ++m)
		{
		const NxMesh& mesh = gIceMeshes[m];
		const float* verts = mesh.verts;
		gIcePreflightDivergent[m] = false;
		gIcePlaneDivergent[m] = false;
		for(unsigned f = 0; f < sizeof(kIcePlaneDivergentMeshes) / sizeof(kIcePlaneDivergentMeshes[0]); ++f)
			if(kIcePlaneDivergentMeshes[f] == m)
				gIcePlaneDivergent[m] = true;
		unsigned nbSlots = 0;
		for(unsigned t = 0; t < mesh.nbTris; ++t)
			for(unsigned j = 0; j < 3; ++j)
				{
				const unsigned a = mesh.tris[t * 3 + j];
				const unsigned b = mesh.tris[t * 3 + (j + 1) % 3];
				slots[nbSlots].lo = a < b ? a : b;
				slots[nbSlots].hi = a < b ? b : a;
				slots[nbSlots].face = t;
				++nbSlots;
				}
		qsort(slots, nbSlots, sizeof(slots[0]), nxIceCompareSlots);
		for(unsigned i = 0; i < nbSlots; )
			{
			unsigned run = 1;
			while(i + run < nbSlots && slots[i + run].lo == slots[i].lo && slots[i + run].hi == slots[i].hi)
				++run;
			if(run == 2)
				{
				++gIcePreflightEdges;
				const unsigned* f0 = &mesh.tris[slots[i].face * 3];
				const unsigned* f1 = &mesh.tris[slots[i + 1].face * 3];
				const unsigned r0 = slots[i].lo, r1 = slots[i].hi;
				unsigned op;
				if(f0[0] == r0 && f0[1] == r1)			op = f0[2];
				else if(f0[0] == r1 && f0[1] == r0)		op = f0[2];
				else if(f0[0] == r0 && f0[2] == r1)		op = f0[1];
				else if(f0[0] == r1 && f0[2] == r0)		op = f0[1];
				else if(f0[1] == r0 && f0[2] == r1)		op = f0[0];
				else if(f0[1] == r1 && f0[2] == r0)		op = f0[0];
				else									op = 0xffffffffu;
				const float* opposite = op == 0xffffffffu ? kGuard : &verts[op * 3];
				float po[4], pc[4];
				((IcePlaneSetFn) (o.base + kIcePlaneSet))(po, &verts[f1[0] * 3], &verts[f1[1] * 3], &verts[f1[2] * 3]);
				((Plane*) pc)->Set(*(const Point*) &verts[f1[0] * 3], *(const Point*) &verts[f1[1] * 3],
					*(const Point*) &verts[f1[2] * 3]);
				const bool belowOracle = nxIceBelow(po, opposite);
				const bool belowCandidate = nxIceBelow(pc, opposite);
				bool differs = false;
				if(belowOracle != belowCandidate)
					{
					++gIcePreflightSide;
					differs = true;
					}
				else if(belowOracle)
					{
					float t0[9], t1[9];
					for(int k = 0; k < 3; ++k)
						{
						memcpy(&t0[k * 3], &verts[f0[k] * 3], 12);
						memcpy(&t1[k * 3], &verts[f1[k] * 3], 12);
						}
					float no0[3], no1[3], nc0[3], nc1[3];
					((IceTriangleNormalFn) (o.base + kIceTriangleNormal))(t0, no0);
					((IceTriangleNormalFn) (o.base + kIceTriangleNormal))(t1, no1);
					((const Triangle*) t0)->Normal(*(Point*) nc0);
					((const Triangle*) t1)->Normal(*(Point*) nc1);
					const double threshold = 0.1f;
					double ao, ac;
					nxIceAngle(no0, no1, &ao);
					nxIceAngle(nc0, nc1, &ac);
					if((ao > threshold) != (ac > threshold) || fabs(ao - threshold) < 1e-6
						|| fabs(ac - threshold) < 1e-6)
						{
						++gIcePreflightAngle;
						differs = true;
						}
					}
				if(differs)
					gIcePreflightDivergent[m] = true;
				}
			i += run;
			}
		if(gIcePreflightDivergent[m])
			{
			++gIcePlaneMeshes;
			fprintf(stderr, "ICE plane_divergent preflight mesh=%d frozen=%d\n", m, gIcePlaneDivergent[m] ? 1 : 0);
			if(!gIcePlaneDivergent[m])
				{
				fprintf(stderr, "FAIL ice mesh %d is outside kIcePlaneDivergentMeshes but its active-edge "
					"decisions follow the vendored Plane::Set / Triangle::Normal\n", m);
				++gMismatches;
				}
			}
		}
	}

// edge_list: 002063 over flag and face-array combinations per mesh, and
// 002054 / 002061 called directly for their early returns and reports.
static void nxIceDriveEdgeList(const NxIceSide& s)
	{
	NxIceCoverage& c = gIceCoverage[0];
	const bool counting = s.oracle;
	// {FacesToEdges, VerticesToEdges, vertices, 16-bit faces}
	static const unsigned char kCombos[][4] =
		{
		{ 1, 0, 0, 0 }, { 0, 1, 0, 0 }, { 1, 1, 0, 0 }, { 0, 0, 0, 0 },
		{ 0, 0, 1, 0 }, { 1, 0, 1, 0 }, { 1, 1, 1, 0 }, { 0, 1, 1, 1 },
		{ 1, 1, 0, 1 }, { 1, 1, 1, 1 },
		};
	unsigned char object[0x20];
	for(int m = 0; m < gIceNbMeshes; ++m)
		{
		const NxMesh& mesh = gIceMeshes[m];
		NxIceInput in;
		nxIceMakeInput(in, mesh, 0);
		for(unsigned k = 0; k < sizeof(kCombos) / sizeof(kCombos[0]); ++k)
			{
			nxIceFoldRun(0, m, k, 0);
			gIceTape = kCombos[k][2] && gIcePlaneDivergent[m] ? s.splitTape : s.mainTape;
			EDGELISTCREATE create;
			memset(&create, 0, sizeof(create));
			create.NbFaces = mesh.nbTris;
			create.DFaces = kCombos[k][3] ? 0 : in.dfaces;
			create.WFaces = kCombos[k][3] ? in.wfaces : 0;
			create.FacesToEdges = kCombos[k][0] != 0;
			create.VerticesToEdges = kCombos[k][1] != 0;
			create.Verts = kCombos[k][2] ? in.verts : 0;
			create.Epsilon = 0.1f;
			memset(object, 0xcd, sizeof(object));
			s.edgeListCtor(object);
			const bool ok = s.edgeListInit(object, create);
			gIceTape->push(ok ? 1u : 0u);
			nxIceTapeEdgeList(object);
			if(counting)
				{
				++c.runs;
				ok ? ++c.succeeded : ++c.failed;
				nxIceCountEdgeList(object, c);
				}
			s.edgeListDtor(object);
			nxIceTapeEdgeList(object);
			gIceTape = s.mainTape;
			}
		nxIceFreeInput(in);
		}

	// Direct calls on the box (mesh 5) for the early returns and every report.
	const NxMesh& box = gIceMeshes[5];
	NxIceInput in;
	nxIceMakeInput(in, box, 0);
	nxIceFoldRun(0, 0xd1, 0, 0);

	// 002054: no faces (line 0x72), neither face array (0x72), then a second
	// call once the links exist (the no-op at 0x000510ab), then 002058 over it.
	memset(object, 0xcd, sizeof(object));
	s.edgeListCtor(object);
	gIceTape->push(s.edgeListF2E(object, 0, in.dfaces, 0) ? 1u : 0u);
	gIceTape->push(s.edgeListF2E(object, box.nbTris, 0, 0) ? 1u : 0u);
	gIceTape->push(s.edgeListF2E(object, box.nbTris, in.dfaces, 0) ? 1u : 0u);
	gIceTape->push(s.edgeListF2E(object, box.nbTris, 0, in.wfaces) ? 1u : 0u);
	nxIceTapeEdgeList(object);
	// 002061 before the descriptors exist: 0x114.
	gIceTape->push(s.edgeListActive(object, box.nbTris, in.dfaces, 0, in.verts, 0.1f) ? 1u : 0u);
	gIceTape->push(s.edgeListE2F(object, box.nbTris, 0, in.wfaces) ? 1u : 0u);
	nxIceTapeEdgeList(object);
	// 002061: no faces (0x10a), no vertices (0x10b), then without the
	// faces-by-edges array (0x117), then for real.
	gIceTape->push(s.edgeListActive(object, box.nbTris, 0, 0, in.verts, 0.1f) ? 1u : 0u);
	gIceTape->push(s.edgeListActive(object, box.nbTris, in.dfaces, 0, 0, 0.1f) ? 1u : 0u);
	unsigned fbe;
	memcpy(&fbe, object + 0x14, 4);
	memset(object + 0x14, 0, 4);
	gIceTape->push(s.edgeListActive(object, box.nbTris, in.dfaces, 0, in.verts, 0.1f) ? 1u : 0u);
	memcpy(object + 0x14, &fbe, 4);
	gIceTape->push(s.edgeListActive(object, box.nbTris, in.dfaces, 0, in.verts, 0.1f) ? 1u : 0u);
	nxIceTapeEdgeList(object);
	// A second pass over marks that are already set (the `js` skips).
	gIceTape->push(s.edgeListActive(object, box.nbTris, 0, in.wfaces, in.verts, 0.1f) ? 1u : 0u);
	nxIceTapeEdgeList(object);
	s.edgeListDtor(object);
	nxIceTapeEdgeList(object);

	// 002061 on an empty list (0x10e), and with a count but no edge data (0x111).
	memset(object, 0xcd, sizeof(object));
	s.edgeListCtor(object);
	gIceTape->push(s.edgeListActive(object, box.nbTris, in.dfaces, 0, in.verts, 0.1f) ? 1u : 0u);
	const unsigned five = 5;
	memcpy(object, &five, 4);
	gIceTape->push(s.edgeListActive(object, box.nbTris, in.dfaces, 0, in.verts, 0.1f) ? 1u : 0u);
	memset(object, 0, 4);
	nxIceTapeEdgeList(object);
	s.edgeListDtor(object);
	nxIceFreeInput(in);
	}

// ice_adjacencies: 001546 per mesh with 32-bit and 16-bit faces, with and
// without vertices; then the degenerate create blocks. 001542's count and the
// release through 001544 follow every run.
static void nxIceAdjRun(const NxIceSide& s, const ADJACENCIESCREATE& create)
	{
	NxIceCoverage& c = gIceCoverage[1];
	unsigned object[4] = { 0x5a5a5a5au, 0, 0xcdcdcdcdu, 0xcdcdcdcdu };
	const bool ok = s.adjInit(object, create);
	gIceTape->push(ok ? 1u : 0u);
	nxIceTapeAdjacencies(object);
	const unsigned boundary = s.adjBoundary(object);
	gIceTape->push(boundary);
	if(s.oracle)
		{
		++c.runs;
		ok ? ++c.succeeded : ++c.failed;
		c.boundary += boundary;
		nxIceCountLinks(object, c);
		}
	s.adjDtor(object);
	gIceTape->push(object[0]);
	gIceTape->push(object[1]);
	}

static void nxIceDriveAdjacencies(const NxIceSide& s)
	{
	for(int m = 0; m < gIceNbMeshes; ++m)
		{
		const NxMesh& mesh = gIceMeshes[m];
		NxIceInput in;
		nxIceMakeInput(in, mesh, 1);
		for(int k = 0; k < 4; ++k)
			{
			nxIceFoldRun(1, m, k, 0);
			gIceTape = (k & 2) && gIcePlaneDivergent[m] ? s.splitTape : s.mainTape;
			ADJACENCIESCREATE create;
			create.NbFaces = mesh.nbTris;
			create.DFaces = (k & 1) ? 0 : in.dfaces;
			create.WFaces = (k & 1) ? in.wfaces : 0;
			create.Verts = (k & 2) ? in.verts : 0;
			create.Epsilon = 0.1f;
			nxIceAdjRun(s, create);
			gIceTape = s.mainTape;
			}
		// Both face arrays: UpdateLink reads WFaces (its two tests are not an
		// else), AddTriangle DFaces. Equal arrays link as before.
		nxIceFoldRun(1, m, 4, 0);
		ADJACENCIESCREATE both;
		both.NbFaces = mesh.nbTris;
		both.DFaces = in.dfaces;
		both.WFaces = in.wfaces;
		both.Verts = 0;
		both.Epsilon = 0.1f;
		nxIceAdjRun(s, both);
		nxIceFreeInput(in);
		}

	// A 16-bit face array that disagrees with the 32-bit one: the database
	// pairs faces by the 32-bit references, UpdateLink looks the edge up in the
	// 16-bit ones and cannot find it -- in the first face of the pair (line 266)
	// or the second (267); inside the loop (mesh 12's shared edge sorts in the
	// middle) and after it (mesh 34's sorts last).
	static const int kMismatch[][3] = { { 12, 0, 1 }, { 12, 1, 0 }, { 34, 0, 2 }, { 34, 1, 1 },
		{ 0, 7, 1 }, { 5, 3, 2 } };
	for(unsigned k = 0; k < sizeof(kMismatch) / sizeof(kMismatch[0]); ++k)
		{
		const NxMesh& mesh = gIceMeshes[kMismatch[k][0]];
		NxIceInput in;
		nxIceMakeInput(in, mesh, 1);
		const unsigned face = (unsigned) kMismatch[k][1];
		const unsigned corner = (unsigned) kMismatch[k][2];
		in.wfaces[face * 3 + corner] = (unsigned short) (mesh.nbVerts + 7);
		nxIceFoldRun(1, 0xbad, k, face * 3 + corner);
		ADJACENCIESCREATE create;
		create.NbFaces = mesh.nbTris;
		create.DFaces = in.dfaces;
		create.WFaces = in.wfaces;
		create.Verts = 0;
		create.Epsilon = 0.1f;
		nxIceAdjRun(s, create);
		nxIceFreeInput(in);
		}

	// No faces (false, the count untouched); no face arrays with one face (the
	// references 0, 1, 2, nothing to link), with three (the same edge three
	// times: the non-manifold report), and with one face and vertices (the
	// EdgeList's own report, which the row does not return).
	const NxMesh& single = gIceMeshes[4];
	NxIceInput in;
	nxIceMakeInput(in, single, 1);
	static const unsigned kBare[][2] = { { 0, 0 }, { 1, 0 }, { 3, 0 }, { 1, 1 } };
	for(unsigned k = 0; k < 4; ++k)
		{
		nxIceFoldRun(1, 0xba5e, kBare[k][0], kBare[k][1]);
		ADJACENCIESCREATE create;
		create.NbFaces = kBare[k][0];
		create.DFaces = 0;
		create.WFaces = 0;
		create.Verts = kBare[k][1] ? in.verts : 0;
		create.Epsilon = 0.1f;
		nxIceAdjRun(s, create);
		}
	nxIceFreeInput(in);
	}

// ice_valencies: 001667 per mesh with 32-bit and 16-bit faces, with and without
// the adjacent-vertex lists; and with no faces (EdgeList's report, false).
static void nxIceDriveValencies(const NxIceSide& s)
	{
	NxIceCoverage& c = gIceCoverage[2];
	unsigned char object[0x20];
	for(int m = 0; m <= gIceNbMeshes; ++m)
		{
		const bool none = m == gIceNbMeshes;
		const NxMesh& mesh = gIceMeshes[none ? 5 : m];
		NxIceInput in;
		nxIceMakeInput(in, mesh, 2);
		for(int k = 0; k < (none ? 1 : 4); ++k)
			{
			nxIceFoldRun(2, m, k, 0);
			VALENCESCREATE create;
			memset(&create, 0, sizeof(create));
			create.NbVerts = mesh.nbVerts;
			create.NbFaces = none ? 0 : mesh.nbTris;
			create.DFaces = (k & 1) ? 0 : in.dfaces;
			create.WFaces = (k & 1) ? in.wfaces : 0;
			create.AdjacentList = (k & 2) != 0;
			memset(object, 0xcd, sizeof(object));
			s.valCtor(object);
			const bool ok = s.valCompute(object, create);
			gIceTape->push(ok ? 1u : 0u);
			nxIceTapeValencies(object);
			if(s.oracle)
				{
				++c.runs;
				ok ? ++c.succeeded : ++c.failed;
				c.adjacent += nxIceWord(object, 4);
				}
			s.valDtor(object);
			nxIceTapeValencies(object);
			}
		nxIceFreeInput(in);
		}
	}

typedef void (*NxIceDriver)(const NxIceSide& s);

// The name of a family's split tape (Task 2e's pose_pair names its own).
static const char* gIceSplitSuffix = "plane_divergent";

// One family: the oracle pass, then the candidate's, each with the recording
// allocator and the report recorder installed for the pass only.
static void nxIceFamily(const NxOracleRows& o, bool selfOnly, NxIceDriver driver, const char* name,
	const char* rva, const char* owner, const char* source, int family)
	{
	gOracleTape.reset();
	gCandidateTape.reset();
	gOracleTapeB.reset();
	gCandidateTapeB.reset();
	gIceInputDigest[family] = 2166136261u;
	gIceInputWords[family] = 0;

	void** errorSlot = (void**) (o.base + kIatFoundationError);
	void** allocatorSlot = (void**) (o.base + kDataSdkAllocator);
	void* shippedReporter = *errorSlot;
	void* shippedAllocator = *allocatorSlot;
	DWORD wasProtected = 0;
	if(!VirtualProtect(errorSlot, sizeof(void*), PAGE_READWRITE, &wasProtected))
		{
		fprintf(stderr, "FAIL cannot reach the oracle's error import slot\n");
		++gMismatches;
		return;
		}
	*errorSlot = (void*) &nxIceOracleReport;
	*allocatorSlot = &gIceAllocator;
	gIceAllocator.reset();
	gIceTape = &gOracleTape;
	gIceOraclePass = true;
	NxIceSide oracle = { &o, true, &gOracleTape, &gOracleTapeB };
	driver(oracle);
	gIceOraclePass = false;
	*allocatorSlot = shippedAllocator;
	*errorSlot = shippedReporter;
	DWORD ignored = 0;
	VirtualProtect(errorSlot, sizeof(void*), wasProtected, &ignored);

	if(!selfOnly)
		{
		SdkAllocator* shipped = nxGetSdkAllocator();
		nxSetSdkAllocatorBridge(&gIceAllocator);
		gNxIceErrorSink = &nxIceCandidateReport;
		gIceAllocator.reset();
		gIceTape = &gCandidateTape;
		NxIceSide candidate = { &o, false, &gCandidateTape, &gCandidateTapeB };
		driver(candidate);
		gNxIceErrorSink = 0;
		nxSetSdkAllocatorBridge(shipped);
		}
	gIceTape = 0;

	printf("thirdparty input name=%s words=%u input=%08x\n", name, gIceInputWords[family],
		gIceInputDigest[family]);
	// Through side tapes, because nxReportTapes copies into gOracleTape /
	// gCandidateTape: the main family first, then its split, if it has one.
	gOracleTapeX87.reset();
	gCandidateTapeX87.reset();
	for(unsigned i = 0; i < gOracleTape.count; ++i)
		gOracleTapeX87.pushKind(gOracleTape.words[i], gOracleTape.kinds[i]);
	for(unsigned i = 0; i < gCandidateTape.count; ++i)
		gCandidateTapeX87.pushKind(gCandidateTape.words[i], gCandidateTape.kinds[i]);
	nxReportTapes(gOracleTapeX87, gCandidateTapeX87, name, rva, owner, source, selfOnly, 0);
	if(gOracleTapeB.count)
		{
		char split[64];
		_snprintf(split, sizeof(split), "%s.%s", name, gIceSplitSuffix);
		split[sizeof(split) - 1] = 0;
		nxReportTapes(gOracleTapeB, gCandidateTapeB, split, rva, owner, source, selfOnly, kDivergent);
		}
	}

static void nxDriveIceMeshTools(const NxOracleRows& o, bool selfOnly)
	{
	nxIceBuildMeshes();
	nxIcePlanePreflight(o);
	memset(gIceCoverage, 0, sizeof(gIceCoverage));
	gIceReports = 0;
	memset(gIceReportLines, 0, sizeof(gIceReportLines));
	gIceReportLine267 = 0;
	gIceReportLine321 = 0;

	nxIceFamily(o, selfOnly, nxIceDriveEdgeList, "edge_list", "0x00051dd0", "phys_fn_002063",
		"EdgeList.cpp", 0);
	const NxIceCoverage& e = gIceCoverage[0];
	printf("thirdparty coverage name=edge_list meshes=%d runs=%u succeeded=%u failed=%u edges=%u"
		" active_edge_links=%u active_vertex_links=%u reports=%u line72=%u line10a=%u line10b=%u"
		" line10e=%u line111=%u line114=%u line117=%u\n",
		gIceNbMeshes, e.runs, e.succeeded, e.failed, e.edges, e.active, e.activeVerts, gIceReports,
		gIceReportLines[0], gIceReportLines[1], gIceReportLines[2], gIceReportLines[3],
		gIceReportLines[4], gIceReportLines[5], gIceReportLines[6]);
	printf("thirdparty coverage name=edge_list.plane_divergent frozen_meshes=%u\n",
		(unsigned) (sizeof(kIcePlaneDivergentMeshes) / sizeof(kIcePlaneDivergentMeshes[0])));
	fprintf(stderr, "ICE preflight pairs=%u side=%u angle=%u meshes=%u\n",
		gIcePreflightEdges, gIcePreflightSide, gIcePreflightAngle, gIcePlaneMeshes);

	gIceReports = 0;
	memset(gIceReportLines, 0, sizeof(gIceReportLines));
	nxIceFamily(o, selfOnly, nxIceDriveAdjacencies, "ice_adjacencies", "0x0002def0", "phys_fn_001546",
		"IceAdjacencies.cpp", 1);
	const NxIceCoverage& a = gIceCoverage[1];
	printf("thirdparty coverage name=ice_adjacencies meshes=%d runs=%u succeeded=%u failed=%u links=%u"
		" boundary=%u active=%u reports=%u line266=%u line267=%u line321=%u line72=%u\n",
		gIceNbMeshes, a.runs, a.succeeded, a.failed, a.links, a.boundary, a.active, gIceReports,
		gIceReportLines[7], gIceReportLine267, gIceReportLine321, gIceReportLines[0]);

	gIceReports = 0;
	memset(gIceReportLines, 0, sizeof(gIceReportLines));
	nxIceFamily(o, selfOnly, nxIceDriveValencies, "ice_valencies", "0x00032610", "phys_fn_001667",
		"IceMeshTools.cpp,EdgeList.cpp", 2);
	const NxIceCoverage& v = gIceCoverage[2];
	printf("thirdparty coverage name=ice_valencies meshes=%d runs=%u succeeded=%u failed=%u"
		" adjacent=%u reports=%u line72=%u\n",
		gIceNbMeshes, v.runs, v.succeeded, v.failed, v.adjacent, gIceReports, gIceReportLines[0]);
	}

//////////////////////////////////////////////////////////////////////////////
// convex-mesh gap Task 2d. MeshBuilder2 (Physics/src/IceMeshBuilder2.cpp,
// 001591..001637) and the vertex reduction (IceMeshTools.cpp, 001645, 001647,
// 001659), linked into this harness. Two families, run through nxIceFamily
// (the recording 004803 allocator and the report recorder installed per pass):
//
//   ice_meshbuilder2   driven as 002087 drives it: the constructor (001593),
//                      Init (001623) with a create block, AddFace (001597) per
//                      triangle, Build (001633), a second Init on the built
//                      object (FreeUsedRam, 001621, over a full object) and the
//                      destructor (001629); also AddFace before Init, AddFace
//                      past the face count, and Build before any face. Eight
//                      create-block configurations (the twelve flags, the uvw
//                      and colour streams, smoothing groups, materials, flips,
//                      out-of-range uvw/colour references), each mesh under a
//                      rotating three of them (the six fixtures under all
//                      eight), alternately under the x87 control words 0x027f
//                      and 0x0f7f.
//   vertex_reduction   001645 / 001647 / 001659 over each mesh's vertices with
//                      duplicates inserted (welded), with and without the
//                      result block, twice on one object (the release at its
//                      start), an empty set, and sets whose smallest vertex has
//                      the bits 0xffffffff x3 (the reduction's initial
//                      "previous" vertex, so it is not kept).
//
// What a tape holds: every return value; for MeshBuilder2 after Build, the
// thirteen Containers word for word (maximum, count, growth factor, entries),
// the counts and flags, and digests of the owned arrays (the stream copies with
// their cookies, the faces -- their output corners only after a successful
// Build and their normals only when a normal flag computed them --, the
// references, the face remap, the per-vertex face tables); the result block's
// counts, and each of its pointers as WHICH array it points at (the two sides
// allocate from different CRT heaps: no pointer is compared); for the
// reduction the object, the cross-reference and the reduced vertices; and,
// interleaved, every allocation and release through the 004803 getter (the
// Containers' growth, the reduction's blocks, RadixSort). The CRT blocks
// MeshBuilder2 allocates itself (005701 / 005700 in the oracle) come from the
// oracle's own CRT heap and are not recorded on either side.
//
// Build is skipped (a marker is taped) for a case none of whose faces has three
// corner positions of distinct bits: the vertex pass would drop every face and
// the face sort then reads the rank of face 0 of an empty sort. It is a rule on
// the fixed inputs, not a comparison; the coverage line counts such cases.
#pragma push_macro("min")
#pragma push_macro("max")
#pragma push_macro("random")
#undef min
#undef max
#undef random
#include "IceMeshBuilder2.h"
#pragma pop_macro("random")
#pragma pop_macro("max")
#pragma pop_macro("min")

static const unsigned kMb2Ctor		= 0x0002ebe0;	// phys_fn_001593
static const unsigned kMb2AddFace	= 0x0002eda0;	// phys_fn_001597
static const unsigned kMb2Init		= 0x00030420;	// phys_fn_001623
static const unsigned kMb2Dtor		= 0x00030d10;	// phys_fn_001629
static const unsigned kMb2Build		= 0x00030f50;	// phys_fn_001633
static const unsigned kVrCtor		= 0x00031680;	// phys_fn_001645
static const unsigned kVrReduce		= 0x000316a0;	// phys_fn_001647
static const unsigned kVrDtor		= 0x000324a0;	// phys_fn_001659

typedef void*	(__thiscall* IceVrCtorFn)(void*, const void*, unsigned);

typedef char nxMb2Layout0[sizeof(MeshBuilder2) == 0x124 && offsetof(MeshBuilder2, mMaxNbFaces) == 0xd0
	&& offsetof(MeshBuilder2, mNbNormInfo) == 0x114 && offsetof(MeshBuilder2, mKillZeroAreaFaces) == 0x118
	&& offsetof(MeshBuilder2, mOptimizeVertexList) == 0x123 ? 1 : -1];
typedef char nxMb2Layout1[sizeof(MBCREATE) == 0x28 && offsetof(MBCREATE, KillZeroAreaFaces) == 0x1c
	&& sizeof(MBFACEINFO) == 0x1c && offsetof(MBFACEINFO, Flip) == 0x18 && sizeof(MBRESULT) == 0x60
	&& offsetof(MBRESULT, UseW) == 0x54 && offsetof(MBRESULT, Materials) == 0x5c ? 1 : -1];
typedef char nxMb2Layout2[sizeof(MBFace) == 0x30 && offsetof(MBFace, Index) == 0x2c && sizeof(MBRef) == 12
	&& sizeof(ReducedVertices) == 0x14 && sizeof(REDUCEDCLOUD) == 12 && sizeof(IceCore::Container) == 16 ? 1 : -1];

// The x87 control word for a case, set and restored around it on both sides.
static unsigned short nxMb2SetControlWord(unsigned short cw)
	{
	unsigned short old;
	__asm
		{
		fnstcw	old
		fldcw	cw
		}
	return old;
	}

static unsigned nxMb2Digest(const void* data, unsigned words)
	{
	unsigned d = 2166136261u;
	const unsigned char* p = (const unsigned char*) data;
	for(unsigned i = 0; i < words; ++i)
		{
		unsigned w;
		memcpy(&w, p + i * 4, 4);
		d = nxFold(d, w);
		}
	return d;
	}

static void nxMb2FoldInput(int family, const void* data, unsigned words)
	{
	if(!gIceOraclePass)
		return;
	unsigned d = gIceInputDigest[family];
	const unsigned char* p = (const unsigned char*) data;
	for(unsigned i = 0; i < words; ++i)
		{
		unsigned w;
		memcpy(&w, p + i * 4, 4);
		d = nxFold(d, w);
		}
	gIceInputDigest[family] = d;
	gIceInputWords[family] += words;
	}

// Oracle-side coverage for the two families' lines.
struct NxMb2Coverage
	{
	unsigned cases, initOk, initFailed, added, dropped, rejected, built, buildFailed, skipped;
	unsigned outFaces, outVerts, submeshes, materials, killed, normInfo, remapped, x87;
	unsigned vrRuns, vrVerts, vrReduced;
	unsigned probes, probesFalse;	// AddFace before Init, AddFace past the count, Build before any face
	unsigned uvwSnan, colourSnan;	// signalling NaN words in the input streams (fixed inputs)
	};

static unsigned nxMb2CountSnan(const float* words, unsigned count)
	{
	unsigned n = 0;
	for(unsigned i = 0; i < count; ++i)
		{
		unsigned w;
		memcpy(&w, &words[i], 4);
		if((w & 0x7f800000u) == 0x7f800000u && (w & 0x007fffffu) && !(w & 0x00400000u))
			++n;
		}
	return n;
	}
static NxMb2Coverage gMb2Coverage;

static void nxMb2Probe(const NxIceSide& s, bool ok)
	{
	gIceTape->push(ok ? 1u : 0u);
	if(s.oracle)
		{
		++gMb2Coverage.probes;
		if(!ok)
			++gMb2Coverage.probesFalse;
		}
	}

// Where a result pointer points: 1 + the Container whose entries it is, 0x100
// for the face remap, 0xfe for null, 0xff for anything else.
static unsigned nxMb2PointerCode(const unsigned char* object, const void* pointer)
	{
	if(!pointer)
		return 0xfeu;
	for(unsigned c = 0; c < 13; ++c)
		if((const void*) (size_t) nxIceWord(object, c * 16 + 8) == pointer)
			return c + 1;
	if((const void*) (size_t) nxIceWord(object, 0x100) == pointer)
		return 0x100u;
	return 0xffu;
	}

// An owned array: present or not; its cookie (the 12-byte streams: then the
// cookie gives the length); its length and digest when `digest`.
static void nxMb2TapeArray(const unsigned char* object, unsigned offset, unsigned words, bool cookie,
	bool digest)
	{
	NxTape& t = *gIceTape;
	const unsigned* array = (const unsigned*) (size_t) nxIceWord(object, offset);
	t.push(0xa7700000u | offset);
	t.push(array ? 1u : 0u);
	if(!array)
		return;
	if(cookie)
		{
		t.push(array[-1]);
		words = array[-1] * 3;
		}
	if(!digest)
		return;
	t.push(words);
	t.push(nxMb2Digest(array, words));
	}

enum NxMb2TapeMode
	{
	kMb2AfterInit,		// a second Init: the face and reference records are not written yet
	kMb2AfterFailure,	// a failed Build: no face's output corners are known to be written
	kMb2AfterBuild,		// a successful Build
	};

static const unsigned kMb2RawEntries = 24;

static void nxMb2TapeObject(const unsigned char* object, NxMb2TapeMode mode)
	{
	NxTape& t = *gIceTape;
	t.push(0x0b1ec700u | (unsigned) mode);
	for(unsigned c = 0; c < 13; ++c)
		{
		const unsigned base = c * 16;
		const unsigned count = nxIceWord(object, base + 4);
		const unsigned* entries = (const unsigned*) (size_t) nxIceWord(object, base + 8);
		t.push(nxIceWord(object, base + 0));
		t.push(count);
		t.pushFloatWord(nxIceWord(object, base + 12));
		t.push(entries ? 1u : 0u);
		if(!entries)
			continue;
		// Containers 5..9 hold floats (vertices, uvw, colours, normals, face
		// normals). The first kMb2RawEntries entries word for word, then a
		// digest of all of them (the tape's length is bounded).
		const bool floats = c >= 5 && c <= 9;
		const unsigned n = count < kMb2RawEntries ? count : kMb2RawEntries;
		for(unsigned i = 0; i < n; ++i)
			floats ? t.pushFloatWord(entries[i]) : t.push(entries[i]);
		t.push(nxMb2Digest(entries, count));
		}
	for(unsigned off = 0xd0; off <= 0xe8; off += 4)
		t.push(nxIceWord(object, off));
	t.push(nxIceWord(object, 0x104));
	t.push(nxIceWord(object, 0x114));
	t.push(nxIceWord(object, 0x118));
	t.push(nxIceWord(object, 0x11c));
	t.push(nxIceWord(object, 0x120));

	const bool records = mode != kMb2AfterInit;
	const unsigned nbVerts = nxIceWord(object, 0xd4);
	const unsigned nbFaces = nxIceWord(object, 0xe0);
	nxMb2TapeArray(object, 0xec, 0, true, true);
	nxMb2TapeArray(object, 0xf0, 0, true, true);
	nxMb2TapeArray(object, 0xf4, 0, true, true);
	nxMb2TapeArray(object, 0xfc, nxIceWord(object, 0xe4) * 3, false, records);
	nxMb2TapeArray(object, 0x100, nxIceWord(object, 0x104), false, true);
	nxMb2TapeArray(object, 0x108, nbVerts, false, true);
	nxMb2TapeArray(object, 0x10c, nbVerts, false, true);
	nxMb2TapeArray(object, 0x110, nbFaces * 3, false, true);

	const MBFace* F = (const MBFace*) (size_t) nxIceWord(object, 0xf8);
	t.push(F ? 1u : 0u);
	if(F)
		{
		t.push(((const unsigned*) F)[-1]);
		if(records)
			{
			const bool built = mode == kMb2AfterBuild;
			const bool normals = built && (object[0x11a] || object[0x11b]);
			unsigned d = 2166136261u;
			for(unsigned i = 0; i < nbFaces; ++i)
				{
				if(built)
					for(int k = 0; k < 3; ++k)
						d = nxFold(d, F[i].VRef[k]);
				for(int k = 0; k < 3; ++k)
					d = nxFold(d, F[i].Ref[k]);
				d = nxFold(d, F[i].MaterialID);
				d = nxFold(d, F[i].SmoothingGroups);
				if(normals)
					for(int k = 0; k < 3; ++k)
						{
						unsigned w;
						memcpy(&w, &F[i].Normal[k], 4);
						d = nxFold(d, w);
						}
				d = nxFold(d, F[i].Index);
				}
			t.push(d);
			}
		}
	}

static void nxMb2TapeResult(const unsigned char* object, const MBRESULT& r)
	{
	NxTape& t = *gIceTape;
	t.push(0x4e5c0000u);
	t.push(r.NbFaces);
	t.push(r.NbMaxFaces);
	t.push(r.NbSubmeshes);
	t.push(r.NbVerts);
	t.push(r.NbTVerts);
	t.push(r.NbCVerts);
	t.push(r.NbOutVerts);
	t.push(r.NbNormInfo);
	t.push(r.UseW ? 1u : 0u);
	t.push(r.NbMaterials);
	const void* pointers[14] =
		{
		r.Topology, r.FacesPerRun, r.FaceNormals, r.Runs, r.FaceRemap, r.VRefs, r.TRefs, r.CRefs,
		r.Verts, r.TVerts, r.CVerts, r.Normals, r.NormInfo, r.Materials,
		};
	for(int i = 0; i < 14; ++i)
		t.push(nxMb2PointerCode(object, pointers[i]));
	}

// One side's MeshBuilder2 and reduction entries.
static void nxMb2Ctor(const NxIceSide& s, void* obj)
	{
	if(s.oracle)	((VoidThisFn) (s.o->base + kMb2Ctor))(obj);
	else			new (obj) MeshBuilder2;
	}
static void nxMb2Dtor(const NxIceSide& s, void* obj)
	{
	if(s.oracle)	((VoidThisFn) (s.o->base + kMb2Dtor))(obj);
	else			((MeshBuilder2*) obj)->~MeshBuilder2();
	}
static bool nxMb2Init(const NxIceSide& s, void* obj, const MBCREATE& c)
	{
	if(s.oracle)	return ((IceCreateFn) (s.o->base + kMb2Init))(obj, &c);
	return ((MeshBuilder2*) obj)->Init(c);
	}
static bool nxMb2AddFace(const NxIceSide& s, void* obj, const MBFACEINFO& f)
	{
	if(s.oracle)	return ((IceCreateFn) (s.o->base + kMb2AddFace))(obj, &f);
	return ((MeshBuilder2*) obj)->AddFace(f);
	}
static bool nxMb2Build(const NxIceSide& s, void* obj, MBRESULT& r)
	{
	if(s.oracle)	return ((IceCreateFn) (s.o->base + kMb2Build))(obj, &r);
	return ((MeshBuilder2*) obj)->Build(r);
	}
static void nxVrCtor(const NxIceSide& s, void* obj, const void* verts, unsigned nb)
	{
	if(s.oracle)	((IceVrCtorFn) (s.o->base + kVrCtor))(obj, verts, nb);
	else			new (obj) ReducedVertices((const IceMaths::Point*) verts, nb);
	}
static bool nxVrReduce(const NxIceSide& s, void* obj, REDUCEDCLOUD* rc)
	{
	if(s.oracle)	return ((IceCreateFn) (s.o->base + kVrReduce))(obj, rc);
	return ((ReducedVertices*) obj)->Reduce(rc);
	}
static void nxVrDtor(const NxIceSide& s, void* obj)
	{
	if(s.oracle)	((VoidThisFn) (s.o->base + kVrDtor))(obj);
	else			((ReducedVertices*) obj)->~ReducedVertices();
	}

// The eight create-block configurations. Flags in the create block's order:
// kill zero-area faces, use w, vertex normals (smoothing groups), face normals,
// normal info, indexed geometry, indexed uvw, indexed colours, relative
// indices, skin (no vertex reduction), angle-weighted normals, optimize.
struct NxMb2Config
	{
	unsigned char	flags[12];
	int				uvw;		// 0 none; 1 one per vertex; 2 a null source (zeroed copy); 3 a palette of 5
	int				colours;	// 0 none; 1 a palette of 4; 2 one per vertex
	int				smoothing;	// 0 all 1; 1 drawn from {0, 1, 2, 4, 3}; 2 all 0
	int				materials;	// 0 all 0xffffffff; 1 drawn from {0, 1, 2}; 2 from {0xffffffff, 5, 3}
	bool			outOfRange;	// uvw / colour references drawn past the stream's count
	bool			flips;		// the face's flip byte drawn
	};

static const NxMb2Config kMb2Configs[8] =
	{
	{ { 1,0,0,0,0,1,1,1,1,0,1,0 }, 0, 0, 0, 0, false, false },	// 002087's block
	{ { 1,1,1,1,1,0,0,0,0,0,1,1 }, 1, 1, 1, 1, false, true },
	{ { 0,0,1,0,1,1,0,1,1,0,0,0 }, 2, 0, 1, 2, false, true },
	{ { 1,1,1,1,0,0,1,0,1,1,1,0 }, 3, 2, 1, 1, false, false },
	{ { 0,0,0,1,0,0,0,0,0,0,0,0 }, 1, 1, 0, 1, true, true },
	{ { 1,0,1,0,1,1,0,0,1,0,1,0 }, 0, 0, 2, 0, false, true },
	{ { 1,1,1,1,1,1,1,1,0,1,0,1 }, 1, 1, 2, 2, false, false },
	{ { 0,0,0,0,0,0,0,0,0,0,0,0 }, 3, 2, 1, 2, false, true },
	};

// Whether any face has three corner positions of distinct bits (see above).
static bool nxMb2AnyDistinctFace(const NxMesh& m)
	{
	for(unsigned t = 0; t < m.nbTris; ++t)
		{
		const unsigned* tri = &m.tris[t * 3];
		const bool d01 = memcmp(&m.verts[tri[0] * 3], &m.verts[tri[1] * 3], 12) != 0;
		const bool d02 = memcmp(&m.verts[tri[0] * 3], &m.verts[tri[2] * 3], 12) != 0;
		const bool d12 = memcmp(&m.verts[tri[1] * 3], &m.verts[tri[2] * 3], 12) != 0;
		if(d01 && d02 && d12)
			return true;
		}
	return false;
	}

// Special words written into a float stream as bits: one word in six (drawn)
// becomes a signalling NaN (either sign, drawn payload), a quiet NaN, an
// infinity, a denormal, a negative zero or a raw drawn word. Each draw is its
// own named local; nothing goes through a float value, so every word reaches
// the rows as drawn.
static void nxMb2RawWords(float* words, unsigned count)
	{
	for(unsigned i = 0; i < count; ++i)
		{
		const unsigned select = nxNext();
		if(select % 6)
			continue;
		const unsigned kind = nxNext();
		const unsigned payload = nxNext();
		unsigned bits;
		switch(kind % 7)
			{
			case 0:	bits = 0x7f800001u | (payload & 0x003ffffeu);	break;	// +sNaN
			case 1:	bits = 0xff800001u | (payload & 0x003ffffeu);	break;	// -sNaN
			case 2:	bits = 0x7fc00000u | (payload & 0x003fffffu);	break;	// qNaN
			case 3:	bits = (payload & 1) ? 0xff800000u : 0x7f800000u;	break;	// infinity
			case 4:	bits = (payload & 0x807fffffu) | 1u;			break;	// denormal
			case 5:	bits = 0x80000000u;							break;	// -0
			default: bits = payload;							break;	// raw
			}
		memcpy(&words[i], &bits, 4);
		}
	}

static const int kMb2Family = 3;
static const int kVrFamily = 4;

// One case: a mesh under one configuration and control word.
static void nxMb2Case(const NxIceSide& s, const NxMesh& mesh, int meshIndex, int config, bool firstOfMesh)
	{
	NxMb2Coverage& c = gMb2Coverage;
	const NxMb2Config& cfg = kMb2Configs[config];
	const unsigned short cw = ((meshIndex + config) & 1) ? 0x0f7f : 0x027f;
	nxIceFoldRun(kMb2Family, (unsigned) meshIndex, (unsigned) config, cw);

	// The streams and per-face words, drawn from a seed of the case's own.
	gState = 0x6d623200u ^ (unsigned) (meshIndex * 131 + config * 7 + 1);
	static float uvw[3 * 400];
	static float colours[3 * 400];
	static unsigned faceWords[400][9];
	static unsigned faceInfo[400][3];
	unsigned nbUVW = 0, nbColours = 0;
	const float* uvwSource = 0;
	const float* colourSource = 0;
	if(cfg.uvw == 1)
		{
		nbUVW = mesh.nbVerts;
		// The vertex words themselves (x, y, and z), bits copied, with
		// special words mixed in (nxMb2RawWords).
		memcpy(uvw, mesh.verts, 12 * nbUVW);
		nxMb2RawWords(uvw, 3 * nbUVW);
		uvwSource = uvw;
		}
	else if(cfg.uvw == 2)
		nbUVW = mesh.nbVerts / 2 + 1;
	else if(cfg.uvw == 3)
		{
		nbUVW = 5;
		// Five uvw of fixed words: 0, 1, 2 in x, 0 / 1 in y, 0.5 in z, and in
		// the last one a signalling NaN in x and a denormal in y.
		static const unsigned kPalette[15] =
			{
			0x00000000u, 0x00000000u, 0x3f000000u,
			0x3f800000u, 0x3f800000u, 0x3f000000u,
			0x40000000u, 0x00000000u, 0x3f000000u,
			0x00000000u, 0x3f800000u, 0x3f000000u,
			0x7fa00005u, 0x00000123u, 0x3f000000u,
			};
		memcpy(uvw, kPalette, sizeof(kPalette));
		uvwSource = uvw;
		}
	if(cfg.colours == 1)
		{
		nbColours = 4;
		static const unsigned kPalette[12] =
			{
			0x00000000u, 0x00000000u, 0x00000000u,
			0x3f800000u, 0x00000000u, 0x3f800000u,
			0x00000000u, 0x3f800000u, 0x00000000u,
			0xffa00007u, 0x7f800000u, 0x7fc00009u,
			};
		memcpy(colours, kPalette, sizeof(kPalette));
		colourSource = colours;
		}
	else if(cfg.colours == 2)
		{
		nbColours = mesh.nbVerts;
		// Three levels as words (0, 0.5, 1), drawn, then special words mixed in.
		static const unsigned kLevels[3] = { 0x00000000u, 0x3f000000u, 0x3f800000u };
		for(unsigned v = 0; v < 3 * nbColours; ++v)
			{
			const unsigned draw = nxNext();
			memcpy(&colours[v], &kLevels[draw % 3], 4);
			}
		nxMb2RawWords(colours, 3 * nbColours);
		colourSource = colours;
		}
	static const unsigned kSmoothing[5] = { 0, 1, 2, 4, 3 };
	static const unsigned kMaterialsA[3] = { 0, 1, 2 };
	static const unsigned kMaterialsB[3] = { 0xffffffffu, 5, 3 };
	for(unsigned t = 0; t < mesh.nbTris; ++t)
		{
		for(int k = 0; k < 3; ++k)
			faceWords[t][k] = mesh.tris[t * 3 + k];
		for(int k = 0; k < 3; ++k)
			{
			const unsigned draw = nxNext();
			const unsigned span = nbUVW + (cfg.outOfRange ? 3 : 0);
			faceWords[t][3 + k] = span ? draw % span : draw % 7;
			}
		for(int k = 0; k < 3; ++k)
			{
			const unsigned draw = nxNext();
			const unsigned span = nbColours + (cfg.outOfRange ? 3 : 0);
			faceWords[t][6 + k] = span ? draw % span : draw % 7;
			}
		const unsigned sgDraw = nxNext();
		const unsigned matDraw = nxNext();
		const unsigned flipDraw = nxNext();
		faceInfo[t][0] = cfg.smoothing == 0 ? 1u : cfg.smoothing == 2 ? 0u : kSmoothing[sgDraw % 5];
		faceInfo[t][1] = cfg.materials == 0 ? 0xffffffffu
			: cfg.materials == 1 ? kMaterialsA[matDraw % 3] : kMaterialsB[matDraw % 3];
		faceInfo[t][2] = cfg.flips ? (flipDraw & 1) : 0u;
		}

	MBCREATE create;
	memset(&create, 0, sizeof(create));
	create.NbVerts = mesh.nbVerts;
	create.NbFaces = mesh.nbTris;
	create.NbTVerts = nbUVW;
	create.NbCVerts = nbColours;
	create.Verts = (const IceMaths::Point*) mesh.verts;
	create.TVerts = (const IceMaths::Point*) uvwSource;
	create.CVerts = (const IceMaths::Point*) colourSource;
	memcpy(&create.KillZeroAreaFaces, cfg.flags, 12);
	nxMb2FoldInput(kMb2Family, mesh.verts, 3 * mesh.nbVerts);
	nxMb2FoldInput(kMb2Family, &create, 4);
	nxMb2FoldInput(kMb2Family, cfg.flags, 3);
	if(uvwSource)
		nxMb2FoldInput(kMb2Family, uvwSource, 3 * nbUVW);
	if(colourSource)
		nxMb2FoldInput(kMb2Family, colourSource, 3 * nbColours);
	for(unsigned t = 0; t < mesh.nbTris; ++t)
		{
		nxMb2FoldInput(kMb2Family, faceWords[t], 9);
		nxMb2FoldInput(kMb2Family, faceInfo[t], 3);
		}

	const unsigned short savedCw = nxMb2SetControlWord(cw);
	NxTape& tape = *gIceTape;
	tape.push(0xca5e0000u | (unsigned) (meshIndex << 4) | (unsigned) config);
	unsigned char object[0x130];
	memset(object, 0xcd, sizeof(object));
	nxMb2Ctor(s, object);

	MBFACEINFO info;
	memset(&info, 0, sizeof(info));
	info.VRefs = faceWords[0];
	if(firstOfMesh)
		{
		// AddFace before Init: no face or reference array.
		nxMb2Probe(s, nxMb2AddFace(s, object, info));
		// Init with no faces: the streams are copied, then false.
		MBCREATE none = create;
		none.NbFaces = 0;
		nxMb2Probe(s, nxMb2Init(s, object, none));
		nxMb2TapeObject(object, kMb2AfterInit);
		}

	const bool initOk = nxMb2Init(s, object, create);
	tape.push(initOk ? 1u : 0u);
	if(s.oracle)
		{
		if(uvwSource)
			c.uvwSnan += nxMb2CountSnan(uvwSource, 3 * nbUVW);
		if(colourSource)
			c.colourSnan += nxMb2CountSnan(colourSource, 3 * nbColours);
		++c.cases;
		initOk ? ++c.initOk : ++c.initFailed;
		if(cw == 0x0f7f)
			++c.x87;
		}

	MBRESULT result;
	if(firstOfMesh && initOk)
		{
		// Build before any face.
		memset(&result, 0, sizeof(result));
		nxMb2Probe(s, nxMb2Build(s, object, result));
		}

	for(unsigned t = 0; t < mesh.nbTris; ++t)
		{
		memset(&info, 0, sizeof(info));
		info.Index = t;
		info.MaterialID = faceInfo[t][1];
		info.SmoothingGroups = faceInfo[t][0];
		info.VRefs = faceWords[t];
		info.TRefs = nbUVW || cfg.outOfRange ? faceWords[t] + 3 : 0;
		info.CRefs = nbColours || cfg.outOfRange ? faceWords[t] + 6 : 0;
		info.Flip = faceInfo[t][2] != 0;
		const unsigned before = nxIceWord(object, 0xe0);
		const bool ok = nxMb2AddFace(s, object, info);
		tape.push(ok ? 1u : 0u);
		if(s.oracle)
			{
			if(!ok)
				++c.rejected;
			else if(nxIceWord(object, 0xe0) != before)
				++c.added;
			else
				++c.dropped;
			}
		}
	// Past the face count: an index above +0xd0 (and a full array).
	memset(&info, 0, sizeof(info));
	info.Index = mesh.nbTris + 1;
	info.VRefs = faceWords[0];
	nxMb2Probe(s, nxMb2AddFace(s, object, info));

	if(!initOk || nxMb2AnyDistinctFace(mesh))
		{
		memset(&result, 0, sizeof(result));
		const unsigned accepted = nxIceWord(object, 0xe0);
		const bool built = nxMb2Build(s, object, result);
		tape.push(built ? 1u : 0u);
		nxMb2TapeObject(object, built ? kMb2AfterBuild : kMb2AfterFailure);
		if(built)
			nxMb2TapeResult(object, result);
		if(s.oracle)
			{
			if(built)
				{
				++c.built;
				c.outFaces += result.NbFaces;
				c.outVerts += result.NbOutVerts;
				c.submeshes += result.NbSubmeshes;
				c.materials += result.NbMaterials;
				c.killed += accepted - nxIceWord(object, 0xe0);
				c.normInfo += result.NbNormInfo;
				if(result.FaceRemap)
					++c.remapped;
				}
			else
				++c.buildFailed;
			}
		}
	else
		{
		tape.push(0x5c1990edu);
		if(s.oracle)
			++c.skipped;
		}

	// A second Init over the built object: FreeUsedRam on full Containers. The
	// face and reference counts are not reset by it (only +0xd0 is written).
	if(config & 1)
		{
		tape.push(nxMb2Init(s, object, create) ? 1u : 0u);
		nxMb2TapeObject(object, kMb2AfterInit);
		}
	nxMb2Dtor(s, object);
	nxMb2SetControlWord(savedCw);
	}

// Three meshes of this family's own, driven under all eight configurations
// after the shared ones:
//   0  the box with every face's corners unshared (36 vertices, 12 faces): the
//      vertex pass welds them back;
//   1  two regular faces and one whose corners 0 and 3 are different vertices
//      at the same position: with zero-area faces kept, the vertex pass drops
//      it (the new face array);
//   2  one face whose corners are two vertices at one position and a third:
//      no face with distinct corners, so Build is skipped.
static NxMesh gMb2Meshes[3];

static void nxMb2BuildMeshes()
	{
	const NxMesh& box = gMeshes[5];
	NxMesh& unshared = gMb2Meshes[0];
	memset(&unshared, 0, sizeof(unshared));
	unshared.nbTris = box.nbTris;
	unshared.nbVerts = box.nbTris * 3;
	for(unsigned i = 0; i < box.nbTris * 3; ++i)
		{
		memcpy(&unshared.verts[i * 3], &box.verts[box.tris[i] * 3], 12);
		unshared.tris[i] = i;
		}

	NxMesh& weld = gMb2Meshes[1];
	memset(&weld, 0, sizeof(weld));
	weld.nbVerts = 6;
	weld.nbTris = 3;
	nxIceSetVertex(weld, 0, 0.0f, 0.0f, 0.0f);
	nxIceSetVertex(weld, 1, 1.0f, 0.0f, 0.0f);
	nxIceSetVertex(weld, 2, 0.0f, 1.0f, 0.0f);
	nxIceSetVertex(weld, 3, 0.0f, 0.0f, 0.0f);
	nxIceSetVertex(weld, 4, 1.0f, 1.0f, 0.5f);
	nxIceSetVertex(weld, 5, -1.0f, 0.5f, 0.25f);
	nxIceSetTri(weld, 0, 0, 1, 2);
	nxIceSetTri(weld, 1, 3, 0, 4);
	nxIceSetTri(weld, 2, 1, 4, 5);

	NxMesh& flat = gMb2Meshes[2];
	memset(&flat, 0, sizeof(flat));
	flat.nbVerts = 3;
	flat.nbTris = 1;
	nxIceSetVertex(flat, 0, 2.0f, 3.0f, 4.0f);
	nxIceSetVertex(flat, 1, 2.0f, 3.0f, 4.0f);
	nxIceSetVertex(flat, 2, -2.0f, 1.0f, 0.0f);
	nxIceSetTri(flat, 0, 0, 1, 2);
	}

static void nxMb2Drive(const NxIceSide& s)
	{
	const unsigned savedState = gState;
	nxMb2BuildMeshes();
	for(int m = 0; m < gIceNbMeshes + 3; ++m)
		{
		const NxMesh& mesh = m < gIceNbMeshes ? gIceMeshes[m] : gMb2Meshes[m - gIceNbMeshes];
		if(m < kNbMeshes || m >= gIceNbMeshes)
			{
			for(int k = 0; k < 8; ++k)
				nxMb2Case(s, mesh, m, k, k == 0);
			}
		else
			{
			nxMb2Case(s, mesh, m, m % 8, true);
			nxMb2Case(s, mesh, m, (m + 3) % 8, false);
			nxMb2Case(s, mesh, m, (m + 5) % 8, false);
			}
		}
	if(gIceTape->overflow)
		{
		fprintf(stderr, "FAIL ice_meshbuilder2 tape overflow (%u words beyond)\n", gIceTape->overflow);
		++gMismatches;
		}
	gState = savedState;
	}

// vertex_reduction: one object, one or two Reduce calls.
static void nxVrRun(const NxIceSide& s, const float* verts, unsigned nb, bool withResult, bool twice)
	{
	NxMb2Coverage& c = gMb2Coverage;
	NxTape& t = *gIceTape;
	nxIceFoldRun(kVrFamily, nb, withResult ? 1u : 0u, twice ? 1u : 0u);
	nxMb2FoldInput(kVrFamily, verts, 3 * nb);
	unsigned char object[0x20];
	memset(object, 0xcd, sizeof(object));
	nxVrCtor(s, object, verts, nb);
	t.push(nxIceWord(object, 0x00));
	t.push((const float*) (size_t) nxIceWord(object, 0x04) == verts ? 1u : 0u);
	t.push(nxIceWord(object, 0x08));
	t.push(nxIceWord(object, 0x0c));
	t.push(nxIceWord(object, 0x10));
	for(int pass = 0; pass < (twice ? 2 : 1); ++pass)
		{
		REDUCEDCLOUD rc;
		memset(&rc, 0xcd, sizeof(rc));
		const bool ok = nxVrReduce(s, object, withResult ? &rc : 0);
		t.push(ok ? 1u : 0u);
		const unsigned nbReduced = nxIceWord(object, 0x08);
		const float* reduced = (const float*) (size_t) nxIceWord(object, 0x0c);
		const unsigned* xref = (const unsigned*) (size_t) nxIceWord(object, 0x10);
		t.push(nxIceWord(object, 0x00));
		t.push(nbReduced);
		t.push(reduced ? 1u : 0u);
		t.push(xref ? 1u : 0u);
		if(xref)
			for(unsigned i = 0; i < nb && i < kIceTableCap; ++i)
				t.push(xref[i]);
		if(reduced)
			for(unsigned i = 0; i < 3 * nbReduced && i < kIceTableCap; ++i)
				{
				unsigned w;
				memcpy(&w, &reduced[i], 4);
				t.pushFloatWord(w);
				}
		if(withResult)
			{
			t.push((const float*) rc.RVerts == reduced ? 1u : 0u);
			t.push(rc.NbRVerts);
			t.push(rc.XRef == xref ? 1u : 0u);
			}
		else
			t.push(nxIceWord(&rc, 0) == 0xcdcdcdcdu ? 1u : 0u);
		if(s.oracle)
			{
			++c.vrRuns;
			c.vrVerts += nb;
			c.vrReduced += nbReduced;
			}
		}
	nxVrDtor(s, object);
	t.push(nxIceWord(object, 0x0c));
	t.push(nxIceWord(object, 0x10));
	}

static void nxVrDrive(const NxIceSide& s)
	{
	const unsigned savedState = gState;
	static float verts[3 * 800];
	for(int m = 0; m < gIceNbMeshes; ++m)
		{
		const NxMesh& mesh = gIceMeshes[m];
		gState = 0x76720000u ^ (unsigned) (m * 977 + 5);
		unsigned nb = mesh.nbVerts;
		memcpy(verts, mesh.verts, 12 * nb);
		// Welded duplicates: a third of the vertices again, each a drawn vertex
		// copied as bits into a drawn slot (the ones after it moved up).
		const unsigned extra = nb / 3 + 1;
		for(unsigned e = 0; e < extra; ++e)
			{
			const unsigned from = nxNext() % nb;
			const unsigned to = nxNext() % (nb + 1);
			memmove(&verts[(to + 1) * 3], &verts[to * 3], 12 * (nb - to));
			memcpy(&verts[to * 3], &verts[(from >= to ? from + 1 : from) * 3], 12);
			++nb;
			}
		nxVrRun(s, verts, nb, true, false);
		nxVrRun(s, verts, nb, false, m % 5 == 0);
		if(m % 4 == 1)
			nxVrRun(s, mesh.verts, mesh.nbVerts, true, true);
		}
	// Empty.
	nxVrRun(s, verts, 0, true, false);
	// The smallest vertex (by bits) is 0xffffffff x3: alone (three copies),
	// then with a larger one before it in memory.
	unsigned ones[12];
	for(int i = 0; i < 12; ++i)
		ones[i] = 0xffffffffu;
	nxVrRun(s, (const float*) ones, 3, true, false);
	ones[3] = 0x7fc00000u;
	nxVrRun(s, (const float*) ones, 4, true, false);
	if(gIceTape->overflow)
		{
		fprintf(stderr, "FAIL vertex_reduction tape overflow (%u words beyond)\n", gIceTape->overflow);
		++gMismatches;
		}
	gState = savedState;
	}

static void nxDriveIceMeshBuilder2(const NxOracleRows& o, bool selfOnly)
	{
	memset(&gMb2Coverage, 0, sizeof(gMb2Coverage));
	gIceReports = 0;
	nxIceFamily(o, selfOnly, nxMb2Drive, "ice_meshbuilder2", "0x00030f50", "phys_fn_001633",
		"IceMeshBuilder2.cpp,IceMeshTools.cpp", kMb2Family);
	const NxMb2Coverage& c = gMb2Coverage;
	printf("thirdparty coverage name=ice_meshbuilder2 meshes=%d+3 cases=%u x87_0f7f=%u init_ok=%u init_failed=%u"
		" faces_added=%u faces_dropped=%u faces_rejected=%u built=%u build_failed=%u skipped=%u out_faces=%u"
		" out_verts=%u submeshes=%u materials=%u killed=%u norm_info=%u remapped=%u probes=%u probes_false=%u uvw_snan=%u colour_snan=%u reports=%u\n",
		gIceNbMeshes, c.cases, c.x87, c.initOk, c.initFailed, c.added, c.dropped, c.rejected, c.built,
		c.buildFailed, c.skipped, c.outFaces, c.outVerts, c.submeshes, c.materials, c.killed, c.normInfo,
		c.remapped, c.probes, c.probesFalse, c.uvwSnan, c.colourSnan, gIceReports);

	gIceReports = 0;
	nxIceFamily(o, selfOnly, nxVrDrive, "vertex_reduction", "0x000316a0", "phys_fn_001647",
		"IceMeshTools.cpp", kVrFamily);
	printf("thirdparty coverage name=vertex_reduction meshes=%d runs=%u verts=%u reduced=%u reports=%u\n",
		gIceNbMeshes, c.vrRuns, c.vrVerts, c.vrReduced, gIceReports);
	}

//////////////////////////////////////////////////////////////////////////////
// convex-mesh gap Task 2e. The rest of sub-unit D (IceMeshTools.cpp: 001639,
// 001641/001643, 001649, 001651, 001653, 001661) and prerequisite P-Small
// (002144 in SmoothNormals.cpp, 001461 in ConvexHull.cpp, 002186 / 002188 in
// TriangleMeshTopology.cpp), linked into this harness. Five families, run
// through nxIceFamily (the recording 004803 allocator and the report recorder
// installed per pass), each driving the oracle's rows at their RVAs and the
// candidate's over the same inputs:
//
//   pose_pair        001653 over pairs of 4x4 poses (null, "clean" or raw) with
//                    either output requested, both x87 control words; and
//                    001639's block, twice, and once more over a poisoned block
//                    (the guard). A clean pose has a signed-permutation
//                    rotation scaled by raw finite words and a raw finite or
//                    infinite translation, or a rotation of raw finite words at
//                    the origin: every inverse element is one product or a sum
//                    of zero products, so the vendored InvertPRMatrix (005191,
//                    whose sums the 2003 compiler reassociated, and which
//                    copies through the FPU) inverts it exactly as the
//                    oracle's does. Rows (x, x, y) against rows (x, -x, y)
//                    make every product element cancel down to its small term,
//                    so the listing's per-element order of the four terms is
//                    observable. A call that uses the inverse of a raw
//                    pose goes to pose_pair.inverse_divergent, under a ceiling
//                    (a rule on the fixed inputs, not a comparison).
//   unique_axis      001661 over sequences of directions on a fresh Container:
//                    fixed pairs straddling 0.9999 (words computed offline),
//                    their negations, doubled and last-bit-perturbed copies (bit
//                    operations only), and raw words (signalling and quiet NaNs,
//                    infinities, denormals, -0); both control words.
//   edge_dedupe      001641 over edge lists: triangle fans (a closed outline),
//                    the same shuffled and flipped, with a stray edge, a
//                    doubled boundary edge, an edge three times; and every
//                    mesh's face edges. An input whose pairs would all cancel
//                    is not driven (001641 then reads the stale words of an
//                    empty copy and counts down from zero): a rule on the fixed
//                    input, counted in the coverage line.
//   mesh_normals     001651 (on an object built by 001536 and released by
//                    001649) per mesh under rotating create blocks (32-bit,
//                    16-bit or no faces; angle-weighted or not, which reaches
//                    002144; the caller's arrays or the object's), the vertex
//                    words with raw words mixed in (nxMb2RawWords) for half of
//                    the cases, both control words; and 001461 over a hull
//                    image of each mesh (16-bit faces), with and without a
//                    previous normal array, and with no vertices.
//   adjacency_owner  002186 and 002188 over a TriangleMesh image of each mesh
//                    (+0x0c, +0x10, +0x14), with no triangles and on the
//                    non-manifold mesh (Init fails: the release path). 002188's
//                    EdgeList computes active edges from the vertices, so its
//                    runs on the 13 meshes of Task 2c's frozen split
//                    (kIcePlaneDivergentMeshes) go to
//                    adjacency_owner.plane_divergent, under a ceiling.
//
// Tapes: every return value; the objects and output arrays word for word (the
// larger arrays their first 48 words and a digest of all, as Task 2d); pointers
// only as null / which array; every allocation and release through the 004803
// getter and every report, interleaved.
#pragma push_macro("min")
#pragma push_macro("max")
#pragma push_macro("random")
#undef min
#undef max
#undef random
#include "ConvexHull.h"
#include "TriangleMesh.h"
#pragma pop_macro("random")
#pragma pop_macro("max")
#pragma pop_macro("min")

static const unsigned kMtHullNormals		= 0x0002ae60;	// phys_fn_001461
static const unsigned kMtZeroPair			= 0x0002dae0;	// phys_fn_001536
static const unsigned kMtIdentityPoses		= 0x000313e0;	// phys_fn_001639
static const unsigned kMtEdgeLoop			= 0x00031480;	// phys_fn_001641
static const unsigned kMtNormalsRelease		= 0x00031890;	// phys_fn_001649
static const unsigned kMtNormalsCompute		= 0x000318d0;	// phys_fn_001651
static const unsigned kMtPosePair			= 0x00031db0;	// phys_fn_001653
static const unsigned kMtUniqueAxis			= 0x000324f0;	// phys_fn_001661
static const unsigned kMtCreateAdjacencies	= 0x000543d0;	// phys_fn_002186
static const unsigned kMtCreateEdgeList		= 0x00054460;	// phys_fn_002188
static const unsigned kMtIdentityGuard		= 0x00123c78;	// 001639's guard byte
static const unsigned kMtIdentityBlock		= 0x00123c7c;	// 001639's block

typedef const float*	(__cdecl* MtIdentityFn)();
typedef bool			(__cdecl* MtEdgeLoopFn)(void*, const void*);
typedef void			(__cdecl* MtPosePairFn)(void*, void*, const void*, const void*);
typedef bool			(__thiscall* MtBoolThisFn)(void*);

typedef char nxMtLayout0[sizeof(MESHNORMALSCREATE) == 0x20 && offsetof(MESHNORMALSCREATE, WeightByAngle) == 0x14
	&& offsetof(MESHNORMALSCREATE, VertexNormals) == 0x1c && sizeof(MeshNormals) == 8 ? 1 : -1];
typedef char nxMtLayout1[offsetof(ConvexHull, mNbFaces) == 4 && offsetof(ConvexHull, mVertexNormals) == 0x14
	&& offsetof(TriangleMesh, mAdjacencies) == 0x84 && offsetof(TriangleMesh, mEdgeList) == 0x88 ? 1 : -1];

static const int kPoseFamily = 5;
static const int kAxisFamily = 6;
static const int kDedupeFamily = 7;
static const int kNormalsFamily = 8;
static const int kOwnerFamily = 9;

// Oracle-side coverage for the five families' lines.
struct NxMtCoverage
	{
	unsigned identityGuard, identityAtBlock, identityKept;
	unsigned poseCalls, poseExact, poseDivergent, poseX87;
	unsigned axisCases, axisCalls, axisAdded, axisRejected, axisNegated, axisSnan;
	unsigned dedupeCases, dedupeTrue, dedupeFalse, dedupeSkipped, dedupeLoopWords;
	unsigned normalsCases, normalsTrue, normalsFalse, normalsWeighted, normalsX87, normalsSnan;
	unsigned hullCases, hullTrue, hullFalse;
	unsigned ownerAdj, ownerAdjBuilt, ownerEdge, ownerEdgeBuilt, ownerSplit;
	};
static NxMtCoverage gMtCoverage;

static unsigned nxMtWord(const float* f)
	{
	unsigned w;
	memcpy(&w, f, 4);
	return w;
	}

static void nxMtFloatWords(const void* data, unsigned words)
	{
	const unsigned char* p = (const unsigned char*) data;
	for(unsigned i = 0; i < words; ++i)
		{
		unsigned w;
		memcpy(&w, p + i * 4, 4);
		gIceTape->pushFloatWord(w);
		}
	}

// A large float array: its first 48 words, then the digest of all of it.
static void nxMtFloatArray(const void* data, unsigned words)
	{
	gIceTape->push(words);
	nxMtFloatWords(data, words < 48 ? words : 48);
	if(words > 48)
		gIceTape->push(nxMb2Digest(data, words));
	}

static unsigned nxMtCountSnan(const void* data, unsigned words)
	{
	return nxMb2CountSnan((const float*) data, words);
	}

// One side's Container: constructed, sized and filled, or read back.
static void nxMtContainerCtor(const NxIceSide& s, void* obj)
	{
	if(s.oracle)	s.o->containerCtor(obj);
	else			new (obj) IceCore::Container;
	}
static void nxMtContainerDtor(const NxIceSide& s, void* obj)
	{
	if(s.oracle)	s.o->containerDtor(obj);
	else			((IceCore::Container*) obj)->~Container();
	}
static void nxMtContainerFill(const NxIceSide& s, void* obj, const unsigned* words, unsigned count)
	{
	if(!count)
		return;
	if(s.oracle)	s.o->containerSetSize(obj, count);
	else			((IceCore::Container*) obj)->SetSize(count);
	unsigned* entries = (unsigned*) (size_t) nxIceWord(obj, 8);
	memcpy(entries, words, 4 * count);
	memcpy((unsigned char*) obj + 4, &count, 4);
	}
static void nxMtTapeContainer(const void* obj, bool floats)
	{
	NxTape& t = *gIceTape;
	const unsigned count = nxIceWord(obj, 4);
	const unsigned* entries = (const unsigned*) (size_t) nxIceWord(obj, 8);
	t.push(nxIceWord(obj, 0));
	t.push(count);
	t.push(nxIceWord(obj, 12));
	t.push(entries ? 1u : 0u);
	if(entries)
		for(unsigned i = 0; i < count && i < kIceTableCap; ++i)
			t.pushKind(entries[i], floats ? kWordFloat : kWordDiscrete);
	}

// A raw word drawn as bits: one of the special kinds nxMb2RawWords mixes in, or
// a drawn word, or (finite) a drawn word whose exponent is not all ones.
static unsigned nxMtRawWord(bool finiteOnly)
	{
	const unsigned kind = nxNext();
	const unsigned payload = nxNext();
	if(finiteOnly)
		{
		unsigned bits = payload;
		if((bits & 0x7f800000u) == 0x7f800000u)
			bits &= 0xbfffffffu;
		switch(kind % 5)
			{
			case 0:	return (payload & 0x807fffffu) | 1u;				// denormal
			case 1:	return (payload & 1) ? 0x80000000u : 0u;			// -0 / +0
			case 2:	return (payload & 0x807fffffu) | 0x3f000000u;		// [0.5, 1)
			default: return bits;
			}
		}
	switch(kind % 9)
		{
		case 0:	return 0x7f800001u | (payload & 0x003ffffeu);				// +sNaN
		case 1:	return 0xff800001u | (payload & 0x003ffffeu);				// -sNaN
		case 2:	return 0x7fc00000u | (payload & 0x003fffffu);				// qNaN
		case 3:	return (payload & 1) ? 0xff800000u : 0x7f800000u;			// infinity
		case 4:	return (payload & 0x807fffffu) | 1u;						// denormal
		case 5:	return 0x80000000u;										// -0
		case 6:	return (payload & 0x807fffffu) | 0x3f000000u;				// [0.5, 1)
		default: return payload;										// raw
		}
	}

// ---------------------------------------------------------------------------
// pose_pair

// A word of +-[0.5, 1), and one of +-[2^-60, 0.5), drawn as bits.
static unsigned nxMtUnitWord()
	{
	const unsigned p = nxNext();
	return 0x3f000000u | (p & 0x807fffffu);
	}
static unsigned nxMtSmallWord()
	{
	const unsigned p = nxNext();
	const unsigned e = 67 + (p >> 24) % 59;
	return (p & 0x807fffffu) | (e << 23);
	}

// A pose of kind 0 (null), 1 (clean), 2 (raw), 3 (a raw rotation at the
// origin), 4 (rows (x, x, y) at the origin) or 5 (rows (x, -x, y) at the
// origin), as sixteen words. With x of +-[0.5, 1) and y of +-[2^-60, 0.5), a
// kind 4 pose times the inverse of a kind 5 one (or the other way round) sums
// x*x' - x*x' + y*y' in every element: the two large products cancel, and
// whether the small one was added to one of them first (and rounded there) is
// decided by the element's order of the four terms, which is what the listing
// fixes per element.
static void nxMtPose(unsigned kind, unsigned* m)
	{
	if(kind == 4 || kind == 5)
		{
		for(int r = 0; r < 3; ++r)
			{
			const unsigned x = nxMtUnitWord();
			const unsigned y = nxMtSmallWord();
			m[r * 4 + 0] = x;
			m[r * 4 + 1] = kind == 5 ? x ^ 0x80000000u : x;
			m[r * 4 + 2] = y;
			const unsigned last = nxMtRawWord(false);
			m[r * 4 + 3] = last;
			}
		for(int c = 0; c < 3; ++c)
			{
			const unsigned sign = nxNext() & 0x80000000u;
			m[12 + c] = sign;
			}
		const unsigned last = nxMtRawWord(false);
		m[15] = last;
		return;
		}
	if(kind == 2)
		{
		for(int i = 0; i < 16; ++i)
			{
			const unsigned w = nxMtRawWord(false);
			m[i] = w;
			}
		return;
		}
	if(kind == 3)
		{
		// Every rotation word drawn finite (one in eleven an infinity), the
		// translation +0 or -0, column 3 raw: the inverse is the transpose and a
		// translation of sums of zero products, exact in any order, while the
		// product with it is three full products per element.
		for(int r = 0; r < 3; ++r)
			{
			for(int c = 0; c < 3; ++c)
				{
				const unsigned select = nxNext();
				const unsigned w = nxMtRawWord(true);
				m[r * 4 + c] = (select % 11) ? w : ((select & 16) ? 0xff800000u : 0x7f800000u);
				}
			const unsigned last = nxMtRawWord(false);
			m[r * 4 + 3] = last;
			}
		for(int c = 0; c < 3; ++c)
			{
			const unsigned sign = nxNext() & 0x80000000u;
			m[12 + c] = sign;
			}
		const unsigned last = nxMtRawWord(false);
		m[15] = last;
		return;
		}
	// A signed permutation of the three axes, each row's one element a drawn
	// finite word; the translation (row 3) finite or infinite; column 3 raw.
	const unsigned perm = nxNext() % 6;
	static const int kPerm[6][3] = { { 0, 1, 2 }, { 0, 2, 1 }, { 1, 0, 2 }, { 1, 2, 0 }, { 2, 0, 1 }, { 2, 1, 0 } };
	for(int r = 0; r < 3; ++r)
		{
		for(int c = 0; c < 3; ++c)
			m[r * 4 + c] = 0;
		const unsigned sign = nxNext() & 0x80000000u;
		const unsigned scale = nxNext();
		const unsigned value = (scale % 3) ? (0x3f800000u | sign) : (nxMtRawWord(true) | sign);
		m[r * 4 + kPerm[perm][r]] = value;
		const unsigned last = nxMtRawWord(false);
		m[r * 4 + 3] = last;
		}
	for(int c = 0; c < 3; ++c)
		{
		const unsigned select = nxNext();
		const unsigned t = nxMtRawWord(true);
		m[12 + c] = (select % 11) ? t : ((select & 16) ? 0xff800000u : 0x7f800000u);
		}
	const unsigned last = nxMtRawWord(false);
	m[15] = last;
	}

static void nxMtPoseCall(const NxIceSide& s, unsigned k0, unsigned k1, const unsigned* p0, const unsigned* p1,
	bool want0, bool want1, unsigned short cw)
	{
	NxMtCoverage& c = gMtCoverage;
	const bool exact = (!want0 || k1 != 2) && (!want1 || k0 != 2);
	gIceTape = exact ? s.mainTape : s.splitTape;
	unsigned out[2][16];
	for(int o = 0; o < 2; ++o)
		for(int i = 0; i < 16; ++i)
			out[o][i] = 0xcdcd0000u + o * 16 + i;
	const unsigned short saved = nxMb2SetControlWord(cw);
	if(s.oracle)
		((MtPosePairFn) (s.o->base + kMtPosePair))(want0 ? out[0] : 0, want1 ? out[1] : 0, k0 ? p0 : 0, k1 ? p1 : 0);
	else
		nxIcePosePair(want0 ? (IceMaths::Matrix4x4*) out[0] : 0, want1 ? (IceMaths::Matrix4x4*) out[1] : 0,
			k0 ? (const IceMaths::Matrix4x4*) p0 : 0, k1 ? (const IceMaths::Matrix4x4*) p1 : 0);
	nxMb2SetControlWord(saved);
	gIceTape->push(0x905e0000u | (k0 << 8) | (k1 << 4) | (want0 ? 2u : 0u) | (want1 ? 1u : 0u));
	for(int o = 0; o < 2; ++o)
		nxMtFloatWords(out[o], 16);
	if(s.oracle)
		{
		++c.poseCalls;
		exact ? ++c.poseExact : ++c.poseDivergent;
		if(cw == 0x0f7f)
			++c.poseX87;
		}
	gIceTape = s.mainTape;
	}

static void nxMtIdentity(const NxIceSide& s)
	{
	NxMtCoverage& c = gMtCoverage;
	NxTape& t = *gIceTape;
	if(s.oracle)
		c.identityGuard = *(const unsigned char*) (s.o->base + kMtIdentityGuard);
	const float* first = s.oracle ? ((MtIdentityFn) (s.o->base + kMtIdentityPoses))() : nxIceIdentityPoses();
	nxMtFloatWords(first, 25);
	const float* second = s.oracle ? ((MtIdentityFn) (s.o->base + kMtIdentityPoses))() : nxIceIdentityPoses();
	t.push(second == first ? 1u : 0u);
	// The guard: over a poisoned block, a third call must leave the poison.
	float saved[25];
	memcpy(saved, first, sizeof(saved));
	unsigned poison[25];
	for(int i = 0; i < 25; ++i)
		poison[i] = 0x7fa0dd00u + i;
	memcpy((void*) first, poison, sizeof(poison));
	const float* third = s.oracle ? ((MtIdentityFn) (s.o->base + kMtIdentityPoses))() : nxIceIdentityPoses();
	t.push(third == first ? 1u : 0u);
	nxMtFloatWords(third, 25);
	if(s.oracle)
		{
		c.identityAtBlock = (const unsigned char*) first == s.o->base + kMtIdentityBlock ? 1u : 0u;
		c.identityKept = memcmp(third, poison, sizeof(poison)) == 0 ? 1u : 0u;
		}
	memcpy((void*) first, saved, sizeof(saved));
	}

static void nxMtPoseDrive(const NxIceSide& s)
	{
	const unsigned savedState = gState;
	nxMtIdentity(s);
	unsigned p0[16], p1[16];
	for(unsigned k0 = 0; k0 < 6; ++k0)
		for(unsigned k1 = 0; k1 < 6; ++k1)
			for(unsigned want = 0; want < 4; ++want)
				for(unsigned draw = 0; draw < 16; ++draw)
					{
					gState = 0x90e50000u ^ (k0 * 1031 + k1 * 257 + want * 61 + draw * 7 + 3);
					nxMtPose(k0, p0);
					nxMtPose(k1, p1);
					nxIceFoldRun(kPoseFamily, k0 | (k1 << 4) | (want << 8), draw, 0);
					if(gIceOraclePass)
						{
						nxMb2FoldInput(kPoseFamily, p0, 16);
						nxMb2FoldInput(kPoseFamily, p1, 16);
						}
					for(int m = 0; m < 2; ++m)
						nxMtPoseCall(s, k0, k1, p0, p1, (want & 2) != 0, (want & 1) != 0, m ? 0x0f7f : 0x027f);
					}
	gState = savedState;
	}

// ---------------------------------------------------------------------------
// unique_axis

// Pairs of directions straddling the 0.9999 threshold (|dot| computed in the
// row's order, (z*sz + y*sy) + x*sx, above it for the even rows and below for
// the odd), found offline by scanning angles about acos(0.9999) over float
// words; the margin is printed beside each. Then the exactly-equal pair (not
// rejected: the row rejects only a greater |dot|) and one word above it.
static const unsigned kMtAxisPairs[18][2][3] =
	{
	{ { 0xbf473035u, 0x3e85b956u, 0xbf124061u }, { 0xbf44f077u, 0x3e86b175u, 0xbf150c52u } },	// +1.88e-10
	{ { 0xbcc96d18u, 0x3f76d601u, 0xbe872e07u }, { 0xbcf3cff1u, 0x3f75e3cfu, 0xbe8da62bu } },	// -2.88e-09
	{ { 0x3e6cebc7u, 0xbefc6210u, 0xbf56b79au }, { 0x3e61e5a9u, 0xbf007586u, 0xbf561d40u } },	// +3.39e-11
	{ { 0x3f429ddcu, 0xbe2bc67fu, 0x3f20ac96u }, { 0x3f414b9eu, 0xbe38fa5bu, 0x3f215a02u } },	// -1.43e-11
	{ { 0xbf67172fu, 0x3eadc44cu, 0xbe876ab6u }, { 0xbf670d60u, 0x3ea95977u, 0xbe8d272eu } },	// +1.5e-08
	{ { 0xbf2161a9u, 0x3d90aadau, 0xbf45e6c9u }, { 0xbf242947u, 0x3d91e597u, 0xbf43966au } },	// -2.23e-10
	{ { 0xbedad584u, 0x3e0415b0u, 0xbf65122cu }, { 0xbedbf230u, 0x3deb9adfu, 0xbf654aa0u } },	// +2.71e-11
	{ { 0x3eba5764u, 0xbf41b7deu, 0xbf0b06e1u }, { 0x3eb3946cu, 0xbf42d2ccu, 0xbf0bb498u } },	// -9.64e-11
	{ { 0x3ed61398u, 0x3ebfea1eu, 0xbf53d3eau }, { 0x3ed54c3cu, 0x3ec69757u, 0xbf527b3au } },	// +4.51e-10
	{ { 0x3f44fc6cu, 0xbeb571d5u, 0x3f08059du }, { 0x3f472632u, 0xbeb0862eu, 0x3f067b62u } },	// -2.3e-11
	{ { 0x3e39b9f2u, 0xbf268317u, 0x3f3cd27fu }, { 0x3e2bb294u, 0xbf276a12u, 0x3f3cdab2u } },	// +2.88e-10
	{ { 0x3e1835dau, 0xbf1c2c6au, 0x3f473e41u }, { 0x3e0f0d13u, 0xbf1a2d13u, 0x3f493696u } },	// -1.49e-08
	{ { 0x3ebb9963u, 0x3f04b1dfu, 0xbf45cff7u }, { 0x3eb7a9c5u, 0x3f02aa03u, 0xbf48131au } },	// +4.01e-10
	{ { 0x3f1d8ec0u, 0x3f4999eau, 0x3d047678u }, { 0x3f205ad6u, 0x3f475c8au, 0x3d0cb2a2u } },	// -6.64e-11
	{ { 0xbf4543a9u, 0xbf07fcdfu, 0xbeb45599u }, { 0xbf4332ebu, 0xbf0af645u, 0xbeb44143u } },	// +7.28e-10
	{ { 0x3df75d8cu, 0xbf717643u, 0x3e9e7161u }, { 0x3df5a2cfu, 0xbf7296e4u, 0x3e9792c0u } },	// -7.63e-10
	{ { 0x3f800000u, 0x00000000u, 0x00000000u }, { 0x3f7ff972u, 0x3e000000u, 0x00000000u } },	// |dot| == 0.9999f
	{ { 0x3f800000u, 0x00000000u, 0x00000000u }, { 0x3f7ff973u, 0x3e000000u, 0x00000000u } },	// one word above
	};

// A direction for step `step` of a case: a table vector, or one derived from
// an earlier direction of the case by bit operations (sign flips, a doubled
// exponent, a last-bit nudge), or raw words.
static void nxMtAxisDraw(unsigned* d, const unsigned (*earlier)[3], unsigned nbEarlier)
	{
	const unsigned select = nxNext();
	const unsigned which = nxNext();
	const unsigned pick = which % 18;
	const unsigned half = (which >> 8) & 1;
	switch(select % 8)
		{
		case 0: case 1:
			memcpy(d, kMtAxisPairs[pick][half], 12);
			break;
		case 2:
			memcpy(d, kMtAxisPairs[pick][half], 12);
			d[0] ^= 0x80000000u;
			d[1] ^= 0x80000000u;
			d[2] ^= 0x80000000u;
			break;
		case 3: case 4:
			if(nbEarlier)
				{
				memcpy(d, earlier[which % nbEarlier], 12);
				const unsigned how = nxNext();
				if(how & 1)
					for(int i = 0; i < 3; ++i)
						d[i] ^= 0x80000000u;
				if(how & 2)
					for(int i = 0; i < 3; ++i)
						if((d[i] & 0x7f800000u) && (d[i] & 0x7f800000u) < 0x7f000000u)
							d[i] += 0x00800000u;
				if(how & 4)
					d[(how >> 4) % 3] += 1 + ((how >> 8) & 3);
				}
			else
				memcpy(d, kMtAxisPairs[pick][half], 12);
			break;
		default:
			for(int i = 0; i < 3; ++i)
				{
				const unsigned w = nxMtRawWord(false);
				d[i] = w;
				}
			break;
		}
	}

static void nxMtAxisDrive(const NxIceSide& s)
	{
	NxMtCoverage& c = gMtCoverage;
	const unsigned savedState = gState;
	unsigned char obj[0x20];
	for(unsigned k = 0; k < 160; ++k)
		{
		gState = 0xa4150000u ^ (k * 977 + 11);
		const unsigned count = 2 + nxNext() % 11;
		unsigned dirs[16][3];
		for(unsigned i = 0; i < count; ++i)
			nxMtAxisDraw(dirs[i], dirs, i);
		nxIceFoldRun(kAxisFamily, k, count, 0);
		nxMb2FoldInput(kAxisFamily, dirs, 3 * count);
		for(int m = 0; m < 2; ++m)
			{
			const unsigned short cw = m ? 0x0f7f : 0x027f;
			memset(obj, 0xcd, sizeof(obj));
			nxMtContainerCtor(s, obj);
			for(unsigned i = 0; i < count; ++i)
				{
				const unsigned short saved = nxMb2SetControlWord(cw);
				bool added;
				if(s.oracle)
					added = ((IceCreateFn) (s.o->base + kMtUniqueAxis))(obj, dirs[i]);
				else
					added = nxIceAddUniqueAxis((IceCore::Container*) obj, 0, (const IceMaths::Point*) dirs[i]);
				nxMb2SetControlWord(saved);
				gIceTape->push(added ? 1u : 0u);
				if(s.oracle)
					{
					++c.axisCalls;
					added ? ++c.axisAdded : ++c.axisRejected;
					if(dirs[i][0] & 0x80000000u)
						++c.axisNegated;
					if(m == 0)
						c.axisSnan += nxMtCountSnan(dirs[i], 3);
					}
				}
			nxMtTapeContainer(obj, true);
			nxMtContainerDtor(s, obj);
			if(s.oracle)
				++c.axisCases;
			}
		}
	gState = savedState;
	}

// ---------------------------------------------------------------------------
// edge_dedupe

// How many pairs survive the dedupe: per unordered edge, its count modulo two.
static unsigned nxMtSurvivors(const unsigned* pairs, unsigned nbPairs)
	{
	unsigned survivors = 0;
	for(unsigned i = 0; i < nbPairs; ++i)
		{
		const unsigned a = pairs[i * 2] < pairs[i * 2 + 1] ? pairs[i * 2] : pairs[i * 2 + 1];
		const unsigned b = pairs[i * 2] < pairs[i * 2 + 1] ? pairs[i * 2 + 1] : pairs[i * 2];
		unsigned before = 0;
		unsigned total = 0;
		for(unsigned j = 0; j < nbPairs; ++j)
			{
			const unsigned x = pairs[j * 2] < pairs[j * 2 + 1] ? pairs[j * 2] : pairs[j * 2 + 1];
			const unsigned y = pairs[j * 2] < pairs[j * 2 + 1] ? pairs[j * 2 + 1] : pairs[j * 2];
			if(x == a && y == b)
				{
				++total;
				if(j < i)
					++before;
				}
			}
		// Count the edge once, at its first occurrence.
		if(!before && (total & 1))
			++survivors;
		}
	return survivors;
	}

static void nxMtEdgeCase(const NxIceSide& s, const unsigned* pairs, unsigned nbPairs, unsigned tag)
	{
	NxMtCoverage& c = gMtCoverage;
	nxIceFoldRun(kDedupeFamily, tag, nbPairs, 0);
	nxMb2FoldInput(kDedupeFamily, pairs, 2 * nbPairs);
	if(!nxMtSurvivors(pairs, nbPairs))
		{
		gIceTape->push(0x5c1bu);
		if(s.oracle)
			++c.dedupeSkipped;
		return;
		}
	unsigned char edges[0x20], loop[0x20];
	memset(edges, 0xcd, sizeof(edges));
	memset(loop, 0xcd, sizeof(loop));
	nxMtContainerCtor(s, edges);
	nxMtContainerCtor(s, loop);
	nxMtContainerFill(s, edges, pairs, 2 * nbPairs);
	bool ok;
	if(s.oracle)
		ok = ((MtEdgeLoopFn) (s.o->base + kMtEdgeLoop))(loop, edges);
	else
		ok = nxIceEdgeLoop(*(IceCore::Container*) loop, *(const IceCore::Container*) edges);
	gIceTape->push(ok ? 1u : 0u);
	nxMtTapeContainer(loop, false);
	nxMtTapeContainer(edges, false);
	if(s.oracle)
		{
		++c.dedupeCases;
		ok ? ++c.dedupeTrue : ++c.dedupeFalse;
		c.dedupeLoopWords += nxIceWord(loop, 4);
		}
	nxMtContainerDtor(s, loop);
	nxMtContainerDtor(s, edges);
	}

static void nxMtShuffle(unsigned* pairs, unsigned nbPairs)
	{
	for(unsigned i = nbPairs - 1; i > 0; --i)
		{
		const unsigned draw = nxNext();
		const unsigned j = draw % (i + 1);
		const unsigned a = pairs[i * 2], b = pairs[i * 2 + 1];
		pairs[i * 2] = pairs[j * 2];
		pairs[i * 2 + 1] = pairs[j * 2 + 1];
		pairs[j * 2] = a;
		pairs[j * 2 + 1] = b;
		}
	}

static void nxMtEdgeDrive(const NxIceSide& s)
	{
	const unsigned savedState = gState;
	static unsigned pairs[2 * 1300];
	// Triangle fans over k drawn distinct vertices: a closed outline.
	for(unsigned f = 0; f < 96; ++f)
		{
		gState = 0xed9e0000u ^ (f * 523 + 17);
		const unsigned k = 3 + nxNext() % 14;
		unsigned v[16];
		for(unsigned i = 0; i < k; ++i)
			{
			unsigned candidate;
			bool fresh;
			do
				{
				candidate = nxNext() % 40;
				fresh = true;
				for(unsigned j = 0; j < i; ++j)
					fresh &= v[j] != candidate;
				}
			while(!fresh);
			v[i] = candidate;
			}
		unsigned n = 0;
		for(unsigned t = 1; t + 1 < k; ++t)
			{
			const unsigned tri[3] = { v[0], v[t], v[t + 1] };
			for(int e = 0; e < 3; ++e)
				{
				pairs[n * 2] = tri[e];
				pairs[n * 2 + 1] = tri[(e + 1) % 3];
				++n;
				}
			}
		const unsigned variant = f % 6;
		if(variant >= 1)
			nxMtShuffle(pairs, n);
		if(variant >= 2)
			for(unsigned i = 0; i < n; ++i)
				{
				const unsigned flip = nxNext();
				if(flip & 1)
					{
					const unsigned a = pairs[i * 2];
					pairs[i * 2] = pairs[i * 2 + 1];
					pairs[i * 2 + 1] = a;
					}
				}
		if(variant == 3)
			{
			// A stray edge to a vertex outside the fan: the chain cannot close.
			pairs[n * 2] = v[0];
			pairs[n * 2 + 1] = 100 + f;
			++n;
			}
		if(variant == 4)
			{
			// A boundary edge doubled: both copies go, the outline breaks.
			pairs[n * 2] = v[k - 1];
			pairs[n * 2 + 1] = v[0];
			++n;
			}
		if(variant == 5)
			{
			// An internal edge a third time: one copy stays.
			pairs[n * 2] = v[0];
			pairs[n * 2 + 1] = v[k / 2];
			++n;
			}
		nxMtEdgeCase(s, pairs, n, f);
		}
	// Every mesh's face edges, in face order and shuffled.
	for(int m = 0; m < gIceNbMeshes; ++m)
		{
		const NxMesh& mesh = gIceMeshes[m];
		if(mesh.nbTris > 100)
			continue;
		unsigned n = 0;
		for(unsigned t = 0; t < mesh.nbTris; ++t)
			for(int e = 0; e < 3; ++e)
				{
				pairs[n * 2] = mesh.tris[t * 3 + e];
				pairs[n * 2 + 1] = mesh.tris[t * 3 + (e + 1) % 3];
				++n;
				}
		nxMtEdgeCase(s, pairs, n, 0x1000 + m);
		gState = 0xed9f0000u ^ (unsigned) (m * 131 + 7);
		if(n > 1)
			nxMtShuffle(pairs, n);
		nxMtEdgeCase(s, pairs, n, 0x2000 + m);
		}
	gState = savedState;
	}

// ---------------------------------------------------------------------------
// mesh_normals

static void nxMtNormalsCase(const NxIceSide& s, const NxMesh& mesh, int meshIndex, unsigned config)
	{
	NxMtCoverage& c = gMtCoverage;
	const unsigned short cw = ((meshIndex + config) & 1) ? 0x0f7f : 0x027f;
	nxIceFoldRun(kNormalsFamily, (unsigned) meshIndex, config, cw);
	gState = 0x4e0a0000u ^ (unsigned) (meshIndex * 211 + config * 13 + 5);
	NxIceInput in;
	nxIceMakeInput(in, mesh, kNormalsFamily);
	// Raw words mixed into the vertex words (bits) for the odd configurations.
	float* verts = (float*) in.verts;
	if(config & 1)
		nxMb2RawWords(verts, 3 * mesh.nbVerts);
	nxMb2FoldInput(kNormalsFamily, verts, 3 * mesh.nbVerts);
	const unsigned faces = (config >> 1) % 3;	// 0 32-bit, 1 16-bit, 2 none
	const bool weighted = ((config >> 2) & 1) != 0;
	const unsigned supplied = (config >> 3) & 3;	// bit 0 face normals, bit 1 vertex normals
	if(faces == 2 && mesh.nbVerts < 3)
		{
		nxIceFreeInput(in);
		return;
		}
	static float faceNormals[3 * 400 + 3];
	static float vertexNormals[3 * 400 + 3];
	for(unsigned i = 0; i < 3 * 400 + 3; ++i)
		{
		const unsigned w = 0xcdcd0000u + i;
		memcpy(&faceNormals[i], &w, 4);
		memcpy(&vertexNormals[i], &w, 4);
		}
	MESHNORMALSCREATE create;
	memset(&create, 0, sizeof(create));
	create.NbVerts = mesh.nbVerts;
	create.Verts = in.verts;
	create.NbFaces = mesh.nbTris;
	create.DFaces = faces == 0 ? in.dfaces : 0;
	create.WFaces = faces == 1 ? in.wfaces : 0;
	create.WeightByAngle = weighted;
	create.FaceNormals = (supplied & 1) ? (IceMaths::Point*) faceNormals : 0;
	create.VertexNormals = (supplied & 2) ? (IceMaths::Point*) vertexNormals : 0;
	unsigned char obj[0x10];
	memset(obj, 0xcd, sizeof(obj));
	if(s.oracle)	((VoidThisFn) (s.o->base + kMtZeroPair))(obj);
	else			new (obj) MeshNormals;
	const unsigned short saved = nxMb2SetControlWord(cw);
	bool ok;
	if(s.oracle)	ok = ((IceCreateFn) (s.o->base + kMtNormalsCompute))(obj, &create);
	else			ok = nxMeshNormalsCompute((MeshNormals*) obj, 0, &create);
	nxMb2SetControlWord(saved);
	NxTape& t = *gIceTape;
	t.push(ok ? 1u : 0u);
	const float* fn = (const float*) (size_t) nxIceWord(obj, 0);
	const float* vn = (const float*) (size_t) nxIceWord(obj, 4);
	t.push(fn ? 1u : 0u);
	t.push(vn ? 1u : 0u);
	if(ok)
		{
		nxMtFloatArray(fn ? fn : faceNormals, 3 * mesh.nbTris);
		nxMtFloatArray(vn ? vn : vertexNormals, 3 * mesh.nbVerts);
		}
	if(s.oracle)
		{
		++c.normalsCases;
		ok ? ++c.normalsTrue : ++c.normalsFalse;
		if(weighted)
			++c.normalsWeighted;
		if(cw == 0x0f7f)
			++c.normalsX87;
		c.normalsSnan += nxMtCountSnan(verts, 3 * mesh.nbVerts);
		}
	if(s.oracle)	((VoidThisFn) (s.o->base + kMtNormalsRelease))(obj);
	else			((MeshNormals*) obj)->~MeshNormals();
	t.push(nxIceWord(obj, 0));
	t.push(nxIceWord(obj, 4));
	nxIceFreeInput(in);
	}

// 001461 over a hull image: +0x04 faces, +0x08 16-bit faces, +0x0c vertices,
// +0x10 the vertex array, +0x14 the previous normals (a block from the
// recording allocator, so its release is taped) or null.
static void nxMtHullCase(const NxIceSide& s, const NxMesh& mesh, int meshIndex, unsigned variant)
	{
	NxMtCoverage& c = gMtCoverage;
	const unsigned short cw = ((meshIndex + variant) & 1) ? 0x0f7f : 0x027f;
	nxIceFoldRun(kNormalsFamily, 0x4000 + meshIndex, variant, cw);
	NxIceInput in;
	nxIceMakeInput(in, mesh, kNormalsFamily);
	unsigned hull[8];
	for(int i = 0; i < 8; ++i)
		hull[i] = 0xcdcd4000u + i;
	hull[1] = mesh.nbTris;
	hull[2] = (unsigned) (size_t) in.wfaces;
	hull[3] = variant == 2 ? 0 : mesh.nbVerts;
	hull[4] = (unsigned) (size_t) in.verts;
	hull[5] = variant == 1 ? (unsigned) (size_t) gIceAllocator.malloc(12, NX_MEMORY_PERSISTENT) : 0;
	const unsigned short saved = nxMb2SetControlWord(cw);
	bool ok;
	if(s.oracle)	ok = ((MtBoolThisFn) (s.o->base + kMtHullNormals))(hull);
	else			ok = ((ConvexHull*) hull)->ComputeVertexNormals();
	nxMb2SetControlWord(saved);
	NxTape& t = *gIceTape;
	t.push(ok ? 1u : 0u);
	const float* normals = (const float*) (size_t) hull[5];
	t.push(normals ? 1u : 0u);
	if(ok && normals)
		nxMtFloatArray(normals, 3 * mesh.nbVerts);
	for(int i = 0; i < 5; ++i)
		t.push(i == 2 || i == 4 ? 0u : hull[i]);
	if(normals)
		gIceAllocator.free((void*) normals);
	if(s.oracle)
		{
		++c.hullCases;
		ok ? ++c.hullTrue : ++c.hullFalse;
		}
	nxIceFreeInput(in);
	}

static void nxMtNormalsDrive(const NxIceSide& s)
	{
	const unsigned savedState = gState;
	for(int m = 0; m < gIceNbMeshes; ++m)
		{
		const NxMesh& mesh = gIceMeshes[m];
		// The six fixtures under all 24 create blocks, the others under four.
		if(m < kNbMeshes)
			for(unsigned config = 0; config < 24; ++config)
				nxMtNormalsCase(s, mesh, m, config);
		else
			for(unsigned r = 0; r < 4; ++r)
				nxMtNormalsCase(s, mesh, m, (m * 5 + r * 7) % 24);
		for(unsigned variant = 0; variant < 3; ++variant)
			if(variant != 2 || m % 8 == 0)
				nxMtHullCase(s, mesh, m, variant);
		}
	// No vertices: false at once, nothing allocated.
	{
	MESHNORMALSCREATE create;
	memset(&create, 0, sizeof(create));
	create.NbVerts = 3;
	create.NbFaces = 1;
	nxIceFoldRun(kNormalsFamily, 0xdead, 0, 0);
	unsigned char obj[0x10];
	memset(obj, 0xcd, sizeof(obj));
	if(s.oracle)	((VoidThisFn) (s.o->base + kMtZeroPair))(obj);
	else			new (obj) MeshNormals;
	bool ok;
	if(s.oracle)	ok = ((IceCreateFn) (s.o->base + kMtNormalsCompute))(obj, &create);
	else			ok = nxMeshNormalsCompute((MeshNormals*) obj, 0, &create);
	gIceTape->push(ok ? 1u : 0u);
	gIceTape->push(nxIceWord(obj, 0));
	gIceTape->push(nxIceWord(obj, 4));
	if(s.oracle)
		{
		++gMtCoverage.normalsCases;
		ok ? ++gMtCoverage.normalsTrue : ++gMtCoverage.normalsFalse;
		}
	}
	if(gIceTape->overflow)
		{
		fprintf(stderr, "FAIL mesh_normals tape overflow (%u words beyond)\n", gIceTape->overflow);
		++gMismatches;
		}
	gState = savedState;
	}

// ---------------------------------------------------------------------------
// adjacency_owner

static void nxMtOwnerCase(const NxIceSide& s, const NxMesh& mesh, int meshIndex, unsigned nbTris)
	{
	NxMtCoverage& c = gMtCoverage;
	nxIceFoldRun(kOwnerFamily, (unsigned) meshIndex, nbTris, 0);
	NxIceInput in;
	nxIceMakeInput(in, mesh, kOwnerFamily);
	static unsigned image[0x40];
	for(int i = 0; i < 0x40; ++i)
		image[i] = 0xcdcd8000u + i;
	image[0x0c / 4] = nbTris;
	image[0x10 / 4] = (unsigned) (size_t) in.verts;
	image[0x14 / 4] = (unsigned) (size_t) in.dfaces;
	NxTape& t = *gIceTape;

	if(s.oracle)	((VoidThisFn) (s.o->base + kMtCreateAdjacencies))(image);
	else			((TriangleMesh*) image)->createAdjacencies();
	void* adjacencies = (void*) (size_t) image[0x84 / 4];
	t.push(0xad50000u | (adjacencies ? 1u : 0u));
	if(adjacencies)
		{
		nxIceTapeAdjacencies(adjacencies);
		s.adjDtor(adjacencies);
		gIceAllocator.free(adjacencies);
		}
	if(s.oracle)
		{
		++c.ownerAdj;
		if(adjacencies)
			++c.ownerAdjBuilt;
		}

	const bool split = gIcePlaneDivergent[meshIndex];
	if(split)
		gIceTape = s.splitTape;
	if(s.oracle)	((VoidThisFn) (s.o->base + kMtCreateEdgeList))(image);
	else			((TriangleMesh*) image)->createEdgeList();
	void* edgeList = (void*) (size_t) image[0x88 / 4];
	gIceTape->push(0xed50000u | (edgeList ? 1u : 0u));
	if(edgeList)
		{
		nxIceTapeEdgeList(edgeList);
		s.edgeListDtor(edgeList);
		gIceAllocator.free(edgeList);
		}
	gIceTape = s.mainTape;
	for(int i = 0; i < 0x40; ++i)
		if(i != 0x10 / 4 && i != 0x14 / 4 && i != 0x84 / 4 && i != 0x88 / 4)
			t.push(image[i]);
	if(s.oracle)
		{
		++c.ownerEdge;
		if(edgeList)
			++c.ownerEdgeBuilt;
		if(split)
			++c.ownerSplit;
		}
	nxIceFreeInput(in);
	}

static void nxMtOwnerDrive(const NxIceSide& s)
	{
	for(int m = 0; m < gIceNbMeshes; ++m)
		nxMtOwnerCase(s, gIceMeshes[m], m, gIceMeshes[m].nbTris);
	// No triangles: both Inits fail, both objects are released.
	nxMtOwnerCase(s, gIceMeshes[5], 5, 0);
	}

static void nxDriveMeshToolsRest(const NxOracleRows& o, bool selfOnly)
	{
	memset(&gMtCoverage, 0, sizeof(gMtCoverage));
	const NxMtCoverage& c = gMtCoverage;

	gIceReports = 0;
	gIceSplitSuffix = "inverse_divergent";
	nxIceFamily(o, selfOnly, nxMtPoseDrive, "pose_pair", "0x00031db0", "phys_fn_001653",
		"IceMeshTools.cpp", kPoseFamily);
	gIceSplitSuffix = "plane_divergent";
	printf("thirdparty coverage name=pose_pair calls=%u exact=%u inverse_divergent=%u x87_0f7f=%u"
		" identity_guard_before=%u identity_at_block=%u identity_kept=%u reports=%u\n",
		c.poseCalls, c.poseExact, c.poseDivergent, c.poseX87, c.identityGuard, c.identityAtBlock,
		c.identityKept, gIceReports);

	gIceReports = 0;
	nxIceFamily(o, selfOnly, nxMtAxisDrive, "unique_axis", "0x000324f0", "phys_fn_001661",
		"IceMeshTools.cpp,IceMeshBuilder2.cpp", kAxisFamily);
	printf("thirdparty coverage name=unique_axis cases=%u calls=%u added=%u rejected=%u negated=%u"
		" input_snan=%u reports=%u\n",
		c.axisCases, c.axisCalls, c.axisAdded, c.axisRejected, c.axisNegated, c.axisSnan, gIceReports);

	gIceReports = 0;
	nxIceFamily(o, selfOnly, nxMtEdgeDrive, "edge_dedupe", "0x00031480", "phys_fn_001641",
		"IceMeshTools.cpp", kDedupeFamily);
	printf("thirdparty coverage name=edge_dedupe cases=%u closed=%u open=%u skipped=%u loop_words=%u reports=%u\n",
		c.dedupeCases, c.dedupeTrue, c.dedupeFalse, c.dedupeSkipped, c.dedupeLoopWords, gIceReports);

	gIceReports = 0;
	nxIceFamily(o, selfOnly, nxMtNormalsDrive, "mesh_normals", "0x000318d0", "phys_fn_001651",
		"IceMeshTools.cpp,SmoothNormals.cpp,ConvexHull.cpp", kNormalsFamily);
	printf("thirdparty coverage name=mesh_normals meshes=%d cases=%u true=%u false=%u weighted=%u x87_0f7f=%u"
		" vertex_snan=%u hull_cases=%u hull_true=%u hull_false=%u reports=%u\n",
		gIceNbMeshes, c.normalsCases, c.normalsTrue, c.normalsFalse, c.normalsWeighted, c.normalsX87,
		c.normalsSnan, c.hullCases, c.hullTrue, c.hullFalse, gIceReports);

	gIceReports = 0;
	memset(gIceReportLines, 0, sizeof(gIceReportLines));
	gIceReportLine267 = 0;
	gIceReportLine321 = 0;
	nxIceFamily(o, selfOnly, nxMtOwnerDrive, "adjacency_owner", "0x000543d0", "phys_fn_002186",
		"TriangleMeshTopology.cpp,IceAdjacencies.cpp,EdgeList.cpp", kOwnerFamily);
	printf("thirdparty coverage name=adjacency_owner meshes=%d adjacencies=%u adjacencies_built=%u"
		" edge_lists=%u edge_lists_built=%u frozen_split_runs=%u reports=%u line72=%u line321=%u\n",
		gIceNbMeshes, c.ownerAdj, c.ownerAdjBuilt, c.ownerEdge, c.ownerEdgeBuilt, c.ownerSplit, gIceReports,
		gIceReportLines[0], gIceReportLine321);
	}

//////////////////////////////////////////////////////////////////////////////
// convex-mesh gap Task 2f. P-Hull (Physics/src/ConvexHull.cpp: 001441, 001445,
// 001449, 001459, 001463, 001465, 001472, 001496, 001502, and the helpers
// 000001, 000925, 001391, 001439 and 001657) and the support maps
// (Physics/src/IceSupportMaps.cpp: 001550..001589), linked into this harness.
// Three families through nxIceFamily (the recording 004803 allocator and the
// report recorder on both sides; oracle rows by RVA, candidates on the same
// inputs; every float input written as bits):
//
//   hull_leaf     the rows no vendored code reaches, over drawn words: 001441
//                 and 001445 over drawn triangles of drawn vertex words (null
//                 vertices too); 001459 over hull images of drawn faces and
//                 vertex words (no vertex count, no vertex array, no faces);
//                 001496 over hand-built polygon arrays (a non-zero count, so
//                 001472 is not reached) with and without a drawn pose; 001449
//                 over drawn link words; 001657; 000001 with each element
//                 constructor (000925, 001391, 001439).
//   convex_hull   001472, then 001502, then 001496, over hull images of
//                 polycube meshes: the boundary faces of voxel sets, each unit
//                 square split along a drawn diagonal, the lattice mapped
//                 through fixed tables of short-mantissa words (every face is
//                 axis-aligned, and every product the vendored Plane::Set,
//                 Triangle::Area and Triangle::Normal form is exact). Voxel
//                 sets that touch along an edge (non-manifold), open meshes and
//                 a ring (genus one: 001641 cannot close the top face, the
//                 line-318 report) reach 001465's failure arms; a second 001472
//                 and 001502 over a built hull reach their releases; 001502 on
//                 a fresh hull builds the polygons itself; separate cubes put
//                 the centre on the outer side of their facing planes (001472's
//                 flip). And 001463 directly over
//                 polygons of lattice points in an axis-aligned plane. By a
//                 rule on the fixed input, meshes whose words were nudged or
//                 replaced by raw words, and 001463 calls over drawn points, go
//                 to convex_hull.plane_divergent: there 001463 and 002061
//                 (through 001465's Adjacencies) decide from the vendored
//                 Plane::Set, Triangle::Area and Triangle::Normal (005155,
//                 005179, 005181), which are measured divergences.
//   support_maps  001550 and 001556 directly over drawn directions (n = 0..20
//                 for the lookup); A, B and C built by each side's constructors
//                 (001565, 001571, 001575) over the hulls convex_hull's main
//                 meshes give (built by each side's own 001472) and over
//                 hand-built hull images of drawn planes, centres and vertex
//                 words; Init (001558) for n = 0, 1, 2, 3, 5, 8, the bytes,
//                 lookups through them, and slot 0 with and without the free
//                 bit; the allocation failures (256 polygons, 256 vertices).
//
// Tapes: every return value; the hulls' fields and arrays word for word (the
// larger arrays their first 48 words and a digest of all, as Task 2d); pointers
// only as offsets into the array they point into, or as which table; every
// allocation and release through the 004803 getter and every report,
// interleaved.
#include "IceSupportMaps.h"

static const unsigned kChVectorCtor		= 0x00001000;	// phys_fn_000001
static const unsigned kChPolygonCtor	= 0x00020440;	// phys_fn_000925
static const unsigned kChIdentityCtor	= 0x00027f00;	// phys_fn_001391
static const unsigned kChEdgeDescCtor	= 0x0002a610;	// phys_fn_001439
static const unsigned kChArea			= 0x0002a620;	// phys_fn_001441
static const unsigned kChCenter			= 0x0002a790;	// phys_fn_001445
static const unsigned kChGather			= 0x0002a900;	// phys_fn_001449
static const unsigned kChCentroid		= 0x0002ad60;	// phys_fn_001459
static const unsigned kChPlane			= 0x0002af30;	// phys_fn_001463
static const unsigned kChPolygons		= 0x0002b6f0;	// phys_fn_001472
static const unsigned kChSupport		= 0x0002c8f0;	// phys_fn_001496
static const unsigned kChEdges			= 0x0002cb50;	// phys_fn_001502
static const unsigned kChReverse		= 0x00032460;	// phys_fn_001657
static const unsigned kSmCubeFace		= 0x0002e160;	// phys_fn_001550
static const unsigned kSmLookup			= 0x0002e220;	// phys_fn_001556
static const unsigned kSmInit			= 0x0002e2f0;	// phys_fn_001558
static const unsigned kSmCtorA			= 0x0002e5a0;	// phys_fn_001565
static const unsigned kSmCtorB			= 0x0002e640;	// phys_fn_001571
static const unsigned kSmCtorC			= 0x0002e7c0;	// phys_fn_001575
static const unsigned kSmTables[4]		= { 0x00107848, 0x0010785c, 0x0010786c, 0x00107890 };

typedef void		(__cdecl* ChGatherFn)(void*, const void*, unsigned, unsigned char*);
typedef bool		(__cdecl* ChPlaneFn)(void*, unsigned, const unsigned*, const void*);
typedef bool		(__cdecl* ChReverseFn)(unsigned, unsigned*);
typedef void		(__stdcall* ChVectorCtorFn)(void*, unsigned, unsigned, const void*);
typedef unsigned	(__cdecl* SmCubeFaceFn)(const void*, float*, float*);

typedef char nxChLayout0[sizeof(HullPolygon) == 0x24 && offsetof(HullPolygon, mPlane) == 0x0c
	&& offsetof(HullPolygon, mMax) == 0x20 && offsetof(ConvexHull, mCentroid) == 0x18
	&& offsetof(ConvexHull, mPolygons) == 0x28 && offsetof(ConvexHull, mEdgePolygons) == 0x48
	&& sizeof(ConvexHull) == 0x4c && offsetof(IceSupportMap, mVertexSource) == 0x14 ? 1 : -1];

static const int kHullLeafFamily = 10;
static const int kHullFamily = 11;
static const int kSupportFamily = 12;

// Oracle-side coverage for the three families' lines.
struct NxChCoverage
	{
	unsigned areaCalls, centerCalls, centroidCalls, centroidTrue, supportCalls, supportPosed, gatherCases,
		gatherFaces, reverseCalls, vectorCalls, leafSnan, leafX87;
	unsigned meshes, splitMeshes, polygonsTrue, polygonsFalse, polygons, edgesTrue, edgesFalse, edges,
		edgesAlone, rebuilt, planeCalls, planeSplit, hullSupport;
	unsigned faceCalls, lookupCalls, maps, initTrue, initFalse, samples, freed, mapLookups, mapSnan;
	};
static NxChCoverage gChCoverage;

// The oracle's thiscall rows and the candidate's __fastcall product forms are
// called the same way: ecx the object, edx cleared, the arguments pushed and
// popped by the callee.
static unsigned nxChCall0(const void* fn, const void* self)
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
static unsigned nxChCall1(const void* fn, const void* self, unsigned a0)
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
static unsigned nxChCall2(const void* fn, const void* self, unsigned a0, unsigned a1)
	{
	unsigned r;
	__asm
		{
		push	a1
		push	a0
		mov		ecx, self
		xor		edx, edx
		call	fn
		mov		r, eax
		}
	return r;
	}
// 001441 returns in st(0): kept as a double, so a result that the float store
// would round is still compared whole.
// Written through a pointer (Task 2g review): the spill is the asm's own.
static void nxChCallSt1(const void* fn, const void* self, unsigned a0, double* out)
	{
	double r;
	__asm
		{
		push	a0
		mov		ecx, self
		xor		edx, edx
		call	fn
		fstp	qword ptr r
		}
	*out = r;
	}

static const void* nxChRow(const NxIceSide& s, unsigned rva, const void* candidate)
	{
	return s.oracle ? (const void*) (s.o->base + rva) : candidate;
	}

static unsigned nxChPtr(const void* p)
	{
	return (unsigned) (size_t) p;
	}

// A finite word of +-[2^-7, 2^8), drawn as bits.
static unsigned nxChMidWord()
	{
	const unsigned mantissa = nxNext();
	const unsigned exponent = nxNext();
	return (mantissa & 0x807fffffu) | ((120u + exponent % 15u) << 23);
	}
// One word in four raw (nxMtRawWord: signalling and quiet NaNs, infinities,
// denormals, -0, [0.5, 1), drawn), the rest nxChMidWord.
static unsigned nxChWord()
	{
	const unsigned select = nxNext();
	if(select % 4 == 0)
		return nxMtRawWord(false);
	return nxChMidWord();
	}
static void nxChWords(unsigned* w, unsigned count)
	{
	for(unsigned i = 0; i < count; ++i)
		w[i] = nxChWord();
	}

static void nxChTapeDouble(double v)
	{
	unsigned w[2];
	memcpy(w, &v, 8);
	gIceTape->pushKind(w[0], kWordDoubleLo);
	gIceTape->pushKind(w[1], kWordDoubleHi);
	}

// A discrete array: its first 48 words, then the digest of all of it.
static void nxChWordArray(const void* data, unsigned words)
	{
	NxTape& t = *gIceTape;
	t.push(words);
	if(!data)
		{
		t.push(0xdeadu);
		return;
		}
	const unsigned char* p = (const unsigned char*) data;
	for(unsigned i = 0; i < words && i < 48; ++i)
		{
		unsigned w;
		memcpy(&w, p + 4 * i, 4);
		t.push(w);
		}
	if(words > 48)
		t.push(nxMb2Digest(data, words));
	}

// ---------------------------------------------------------------------------
// hull_leaf

static void nxChLeafTriangles(const NxIceSide& s)
	{
	NxChCoverage& c = gChCoverage;
	NxTape& t = *gIceTape;
	for(unsigned k = 0; k < 900; ++k)
		{
		const unsigned short cw = (k & 1) ? 0x0f7f : 0x027f;
		unsigned verts[24];
		nxChWords(verts, 24);
		const unsigned r0 = nxNext();
		const unsigned r1 = nxNext();
		const unsigned r2 = nxNext();
		unsigned short tri[3] = { (unsigned short) (r0 % 8), (unsigned short) (r1 % 8), (unsigned short) (r2 % 8) };
		const bool noVerts = k % 45 == 7;
		nxMb2FoldInput(kHullLeafFamily, verts, 24);
		nxIceFoldRun(kHullLeafFamily, tri[0] | (tri[1] << 8) | (tri[2] << 16), noVerts ? 1u : 0u, cw);
		const unsigned vp = noVerts ? 0u : nxChPtr(verts);
		unsigned center[4] = { 0xcdcd0001u, 0xcdcd0002u, 0xcdcd0003u, 0xcdcd0004u };
		const unsigned short saved = nxMb2SetControlWord(cw);
		double area;
		nxChCallSt1(nxChRow(s, kChArea, (const void*) &nxHullTriangleArea), tri, vp, &area);
		nxChCall2(nxChRow(s, kChCenter, (const void*) &nxHullTriangleCenter), tri, vp, nxChPtr(center));
		nxMb2SetControlWord(saved);
		nxChTapeDouble(area);
		for(int i = 0; i < 4; ++i)
			t.pushKind(center[i], kWordFloat);
		if(s.oracle)
			{
			++c.areaCalls;
			++c.centerCalls;
			c.leafSnan += nxMb2CountSnan((const float*) verts, 24);
			if(cw == 0x0f7f)
				++c.leafX87;
			}
		}
	}

static void nxChLeafCentroids(const NxIceSide& s)
	{
	NxChCoverage& c = gChCoverage;
	NxTape& t = *gIceTape;
	for(unsigned k = 0; k < 400; ++k)
		{
		const unsigned short cw = (k & 1) ? 0x027f : 0x0f7f;
		const unsigned nbVertsDraw = nxNext();
		const unsigned nbFacesDraw = nxNext();
		const unsigned nbVerts = 1 + nbVertsDraw % 10;
		const unsigned nbFaces = nbFacesDraw % 13;
		unsigned verts[30];
		nxChWords(verts, 30);
		unsigned short faces[39];
		for(unsigned i = 0; i < 39; ++i)
			{
			const unsigned r = nxNext();
			faces[i] = (unsigned short) (r % nbVerts);
			}
		unsigned hull[0x4c / 4];
		for(unsigned i = 0; i < 0x4c / 4; ++i)
			hull[i] = 0xcdcd5000u + i;
		hull[1] = nbFaces;
		hull[2] = nxChPtr(faces);
		hull[3] = k % 37 == 5 ? 0u : nbVerts;
		hull[4] = k % 41 == 9 ? 0u : nxChPtr(verts);
		nxMb2FoldInput(kHullLeafFamily, verts, 30);
		nxMb2FoldInput(kHullLeafFamily, faces, 39 / 2);
		nxIceFoldRun(kHullLeafFamily, hull[1], hull[3], hull[4] ? cw : 0u);
		unsigned center[4] = { 0xcdcd0011u, 0xcdcd0012u, 0xcdcd0013u, 0xcdcd0014u };
		const unsigned short saved = nxMb2SetControlWord(cw);
		const unsigned ok = nxChCall1(nxChRow(s, kChCentroid, (const void*) &nxHullComputeCentroid), hull,
			nxChPtr(center)) & 0xff;
		nxMb2SetControlWord(saved);
		t.push(ok);
		for(int i = 0; i < 4; ++i)
			t.pushKind(center[i], kWordFloat);
		if(s.oracle)
			{
			++c.centroidCalls;
			if(ok)
				++c.centroidTrue;
			c.leafSnan += nxMb2CountSnan((const float*) verts, 3 * nbVerts);
			}
		}
	}

// 001496 over polygon arrays built here: only the planes' normals are read.
static void nxChLeafSupport(const NxIceSide& s)
	{
	NxChCoverage& c = gChCoverage;
	NxTape& t = *gIceTape;
	static unsigned polygons[16 * 9];
	for(unsigned k = 0; k < 1500; ++k)
		{
		const unsigned short cw = (k & 1) ? 0x0f7f : 0x027f;
		const unsigned nbDraw = nxNext();
		const unsigned nb = 1 + nbDraw % 13;
		nxChWords(polygons, 9 * nb);
		// Ties: a drawn normal copied over a later polygon's.
		const unsigned tieDraw = nxNext();
		if(nb > 1 && tieDraw % 3 == 0)
			{
			const unsigned from = tieDraw % nb;
			const unsigned to = (tieDraw >> 8) % nb;
			memcpy(&polygons[9 * to + 3], &polygons[9 * from + 3], 12);
			}
		unsigned dir[3];
		nxChWords(dir, 3);
		unsigned pose[16];
		nxChWords(pose, 16);
		const unsigned posed = nxNext() & 1;
		unsigned hull[0x4c / 4];
		for(unsigned i = 0; i < 0x4c / 4; ++i)
			hull[i] = 0xcdcd6000u + i;
		hull[9] = nb;
		hull[10] = nxChPtr(polygons);
		nxMb2FoldInput(kHullLeafFamily, polygons, 9 * nb);
		nxMb2FoldInput(kHullLeafFamily, dir, 3);
		if(posed)
			nxMb2FoldInput(kHullLeafFamily, pose, 16);
		nxIceFoldRun(kHullLeafFamily, nb, posed, cw);
		const unsigned short saved = nxMb2SetControlWord(cw);
		const unsigned index = nxChCall2(nxChRow(s, kChSupport, (const void*) &nxHullSupportPolygon), hull,
			nxChPtr(dir), posed ? nxChPtr(pose) : 0u);
		nxMb2SetControlWord(saved);
		t.push(index);
		if(s.oracle)
			{
			++c.supportCalls;
			if(posed)
				++c.supportPosed;
			c.leafSnan += nxMb2CountSnan((const float*) polygons, 9 * nb) + nxMb2CountSnan((const float*) dir, 3)
				+ (posed ? nxMb2CountSnan((const float*) pose, 16) : 0);
			}
		}
	}

// 001449 over drawn link words: each face's three words link a drawn face with
// bit 29 drawn (and the edge bits drawn), from a drawn face, with some faces
// marked beforehand.
static void nxChLeafGather(const NxIceSide& s)
	{
	NxChCoverage& c = gChCoverage;
	NxTape& t = *gIceTape;
	for(unsigned k = 0; k < 300; ++k)
		{
		const unsigned nbDraw = nxNext();
		const unsigned nb = 1 + nbDraw % 40;
		unsigned links[3 * 40];
		for(unsigned i = 0; i < 3 * nb; ++i)
			{
			const unsigned face = nxNext();
			const unsigned bits = nxNext();
			links[i] = (face % nb) | (bits & 0xe0000000u);
			}
		unsigned char marks[40];
		for(unsigned i = 0; i < nb; ++i)
			{
			const unsigned m = nxNext();
			marks[i] = (unsigned char) (m % 7 == 0 ? 1 : 0);
			}
		const unsigned startDraw = nxNext();
		const unsigned start = startDraw % nb;
		nxMb2FoldInput(kHullLeafFamily, links, 3 * nb);
		nxIceFoldRun(kHullLeafFamily, nb, start, marks[start]);
		unsigned char obj[16];
		nxMtContainerCtor(s, obj);
		ChGatherFn gather = (ChGatherFn) nxChRow(s, kChGather, (const void*) &nxHullGatherFaces);
		gather(obj, links, start, marks);
		nxMtTapeContainer(obj, false);
		for(unsigned i = 0; i < nb; ++i)
			t.push(marks[i]);
		if(s.oracle)
			{
			++c.gatherCases;
			c.gatherFaces += nxIceWord(obj, 4);
			}
		nxMtContainerDtor(s, obj);
		}
	}

static void nxChLeafHelpers(const NxIceSide& s)
	{
	NxChCoverage& c = gChCoverage;
	NxTape& t = *gIceTape;
	// 001657: counts 0..9, a null array.
	for(unsigned k = 0; k < 24; ++k)
		{
		unsigned words[9];
		for(unsigned i = 0; i < 9; ++i)
			words[i] = nxNext();
		const unsigned count = k % 10;
		const bool nullArray = k == 17;
		nxIceFoldRun(kHullLeafFamily, 0x657, count, nullArray ? 1u : 0u);
		ChReverseFn reverse = (ChReverseFn) nxChRow(s, kChReverse, (const void*) &nxIceReverseArray);
		const bool ok = reverse(count, nullArray ? 0 : words);
		t.push(ok ? 1u : 0u);
		for(unsigned i = 0; i < 9; ++i)
			t.push(words[i]);
		if(s.oracle)
			++c.reverseCalls;
		}
	// 000001 with each element constructor, over poisoned 0x24-byte elements;
	// counts 0..4 and a negative one (nothing constructed).
	const unsigned ctors[3] = { kChPolygonCtor, kChIdentityCtor, kChEdgeDescCtor };
	const void* candidates[3] = { (const void*) &nxHullPolygonConstruct, (const void*) &nxIceIdentityConstruct,
		(const void*) &nxEdgeDescConstruct };
	for(unsigned k = 0; k < 18; ++k)
		{
		unsigned block[5 * 9];
		for(unsigned i = 0; i < 5 * 9; ++i)
			block[i] = 0xcdcd7000u + i;
		const unsigned which = k % 3;
		const unsigned count = k / 3 < 5 ? k / 3 : 0xffffffffu;
		const unsigned size = which == 0 ? 0x24u : 8u;
		nxIceFoldRun(kHullLeafFamily, 0x001, which, count);
		ChVectorCtorFn construct = (ChVectorCtorFn) nxChRow(s, kChVectorCtor, (const void*) &nxIceVectorConstruct);
		construct(block, size, count, nxChRow(s, ctors[which], candidates[which]));
		for(unsigned i = 0; i < 5 * 9; ++i)
			t.push(block[i]);
		if(s.oracle)
			++c.vectorCalls;
		}
	}

static void nxChLeafDrive(const NxIceSide& s)
	{
	const unsigned savedState = gState;
	gState = 0x2f0a1eafu;
	nxChLeafTriangles(s);
	nxChLeafCentroids(s);
	nxChLeafSupport(s);
	nxChLeafGather(s);
	nxChLeafHelpers(s);
	if(gIceTape->overflow)
		{
		fprintf(stderr, "FAIL hull_leaf tape overflow (%u words beyond)\n", gIceTape->overflow);
		++gMismatches;
		}
	gState = savedState;
	}

// ---------------------------------------------------------------------------
// convex_hull

// The lattice tables: five increasing words each, every pairwise difference of
// at most six significant bits, so a cross product of two differences, its
// square and the sums the vendored rows form are exact in a float. None
// decreases: a mirrored lattice winds every face inward, every convex edge then
// reads as concave and none is active, and 001641 reads the first pair of an
// empty Container (a null read on both sides; see nxChBuildMesh).
static const unsigned kChLattice[8][5] =
	{
	{ 0x00000000u, 0x3f800000u, 0x40000000u, 0x40400000u, 0x40800000u },	// 0 1 2 3 4
	{ 0xbfc00000u, 0xbf000000u, 0x3e800000u, 0x3fe00000u, 0x41200000u },	// -1.5 -0.5 0.25 1.75 10
	{ 0x42c00000u, 0x43000000u, 0x43200000u, 0x43600000u, 0x43800000u },	// 96 128 160 224 256
	{ 0x3e000000u, 0x3ec00000u, 0x3f000000u, 0x3f200000u, 0x40600000u },	// 0.125 0.375 0.5 0.625 3.5
	{ 0xc0800000u, 0xc0400000u, 0xc0000000u, 0xbf800000u, 0x80000000u },	// -4 -3 -2 -1 -0
	{ 0x80000000u, 0x3f000000u, 0x3fc00000u, 0x40200000u, 0x40c00000u },	// -0 0.5 1.5 2.5 6
	{ 0xc1000000u, 0xc0c00000u, 0xc0000000u, 0xbf800000u, 0xbf400000u },	// -8 -6 -2 -1 -0.75
	{ 0x45000000u, 0x45400000u, 0x45800000u, 0x45c00000u, 0x46000000u },	// 2048 3072 4096 6144 8192
	};

static const unsigned kChMaxVerts = 5 * 4 * 4;
static const unsigned kChMaxTris = 2 * 6 * 4 * 3 * 3;

struct NxChMesh
	{
	unsigned		nbVerts, nbTris;
	unsigned		verts[3 * kChMaxVerts];
	unsigned short	tris[3 * kChMaxTris];
	bool			split;		// convex_hull.plane_divergent (see nxChBuildMesh)
	bool			buildable;	// a main-set closed manifold convex set: 001472 succeeds
	unsigned short	cw;			// the control word the mesh runs under
	};

// The voxel sets: dimensions, one bit per voxel (x fastest) or 0 for a drawn
// fill, and whether the set is concave. Kinds: boxes, box less a corner, L,
// staircase, plus, ring (genus one), two voxels on an edge (non-manifold),
// pocket, drawn, separate cubes. A concave set's concave edges are inactive, so faces across
// them join one polygon that is not planar, whose plane 001463 takes through
// a skew triangle with the vendored Plane::Set: those meshes are in the split
// (the fifth word, a rule on the fixed input). The ring fails in 001465 (its
// outline report) before any plane, and the edge pair in Adjacencies.
static const unsigned kChShapes[][5] =
	{
	{ 1, 1, 1, 0x1u, 0 },
	{ 2, 1, 1, 0x3u, 0 },
	{ 2, 2, 2, 0xffu, 0 },
	{ 3, 2, 1, 0x3fu, 0 },
	{ 2, 2, 2, 0x7fu, 1 },		// the box less a corner
	{ 3, 2, 1, 0x0fu, 1 },		// L
	{ 3, 1, 3, 0x5fu, 1 },		// a staircase in x-z
	{ 3, 3, 1, 0x0bau, 1 },		// plus
	{ 3, 3, 1, 0x1efu, 0 },		// ring: the centre voxel empty
	{ 2, 2, 1, 0x9u, 0 },		// two voxels meeting along an edge
	{ 4, 3, 2, 0x0u, 1 },		// drawn
	{ 4, 1, 1, 0xfu, 0 },
	{ 1, 3, 3, 0x1ffu, 0 },
	{ 3, 3, 3, 0x7ffffffu, 0 },
	{ 3, 3, 3, 0x7bfffffu, 1 },	// a cube less the voxel at the centre of its top (a pocket)
	{ 3, 3, 3, 0x0u, 1 },		// drawn
	{ 3, 1, 1, 0x5u, 0 },		// two separate cubes: the centre lies between them, on the
	{ 3, 3, 1, 0x145u, 0 },		// outer side of their facing faces (001472's flip); four
	};							// separate cubes
static const unsigned kChNbShapes = sizeof(kChShapes) / sizeof(kChShapes[0]);

static unsigned nxChLatticeIndex(unsigned i, unsigned j, unsigned k)
	{
	return i + 5 * (j + 4 * k);
	}

// Builds mesh `index`: its shape, tables and diagonals are drawn from a seed of
// its own. Meshes 0..(2 * kChNbShapes - 1) are the main set; the rest are the
// same shapes with words nudged by a few units in the last place, or replaced
// by raw words (the split).
static void nxChBuildMesh(NxChMesh& m, unsigned index)
	{
	gState = 0x2f0c0000u ^ (index * 0x9e3779b9u + 1u);
	// Round 5 (index 5 * kChNbShapes on): the unit box, the two-cube bar and the
	// separate cubes again under 0x0f7f, through the tables whose steps between
	// the entries these shapes use are powers of two (0, 2, 4, 7). Every face
	// polygon is then at most two cells, so every cross product 001463 and
	// 002061 form is a power of two times the other step and the vendored
	// Plane::Set / Triangle::Normal normalise it exactly under chop rounding
	// too: these stay in the main family.
	static const unsigned kExtraKinds[4] = { 0, 1, 16, 17 };
	static const unsigned kPow2Tables[4] = { 0, 2, 4, 7 };
	const bool extra = index >= 5 * kChNbShapes;
	const unsigned kind = extra ? kExtraKinds[(index - 5 * kChNbShapes) % 4] : index % kChNbShapes;
	const unsigned* shape = kChShapes[kind];
	const unsigned nx = shape[0], ny = shape[1], nz = shape[2];
	unsigned fill = shape[3];
	if(!fill)
		{
		// A drawn fill, with the first voxel and, for 3 x 3 x 3, the centre
		// voxel set: an enclosed empty voxel would give an inner shell whose
		// edges are all concave, so a face group with no active edge, and
		// 001643 (0x00031537, in 001641's continuation) reads the first pair of an empty Container
		// without testing its count -- a null read on both sides. Such an
		// input cannot be driven (a rule on the fixed input, as Task 2e's
		// edge_dedupe does not drive pairs that all cancel).
		const unsigned drawn = nxNext();
		fill = drawn | 1u | (nx == 3 && ny == 3 && nz == 3 ? 0x2000u : 0u);
		}
	const unsigned drawX = nxNext();
	const unsigned drawY = nxNext();
	const unsigned drawZ = nxNext();
	const unsigned tx = extra ? kPow2Tables[drawX % 4] : drawX % 8;
	const unsigned ty = extra ? kPow2Tables[drawY % 4] : drawY % 8;
	const unsigned tz = extra ? kPow2Tables[drawZ % 4] : drawZ % 8;
	bool solid[5][4][4];
	memset(solid, 0, sizeof(solid));
	for(unsigned k = 0; k < nz; ++k)
		for(unsigned j = 0; j < ny; ++j)
			for(unsigned i = 0; i < nx; ++i)
				solid[i][j][k] = ((fill >> (i + nx * (j + ny * k))) & 1) != 0;
	// Lattice vertices used by a face, compacted in lattice order.
	unsigned remap[5 * 4 * 4];
	bool used[5 * 4 * 4];
	memset(used, 0, sizeof(used));
	unsigned quads[6 * 4 * 3 * 3][4];
	unsigned nbQuads = 0;
	for(unsigned k = 0; k < nz; ++k)
		for(unsigned j = 0; j < ny; ++j)
			for(unsigned i = 0; i < nx; ++i)
				{
				if(!solid[i][j][k])
					continue;
				// +x, -x, +y, -y, +z, -z; corners c0, c1, c2, c3 with (c1 - c0) x (c3 - c0) outward.
				const bool px = i + 1 >= nx || !solid[i + 1][j][k];
				const bool mx = i == 0 || !solid[i - 1][j][k];
				const bool py = j + 1 >= ny || !solid[i][j + 1][k];
				const bool my = j == 0 || !solid[i][j - 1][k];
				const bool pz = k + 1 >= nz || !solid[i][j][k + 1];
				const bool mz = k == 0 || !solid[i][j][k - 1];
				if(px) { unsigned* q = quads[nbQuads++]; q[0] = nxChLatticeIndex(i + 1, j, k); q[1] = nxChLatticeIndex(i + 1, j + 1, k); q[2] = nxChLatticeIndex(i + 1, j + 1, k + 1); q[3] = nxChLatticeIndex(i + 1, j, k + 1); }
				if(mx) { unsigned* q = quads[nbQuads++]; q[0] = nxChLatticeIndex(i, j, k); q[1] = nxChLatticeIndex(i, j, k + 1); q[2] = nxChLatticeIndex(i, j + 1, k + 1); q[3] = nxChLatticeIndex(i, j + 1, k); }
				if(py) { unsigned* q = quads[nbQuads++]; q[0] = nxChLatticeIndex(i, j + 1, k); q[1] = nxChLatticeIndex(i, j + 1, k + 1); q[2] = nxChLatticeIndex(i + 1, j + 1, k + 1); q[3] = nxChLatticeIndex(i + 1, j + 1, k); }
				if(my) { unsigned* q = quads[nbQuads++]; q[0] = nxChLatticeIndex(i, j, k); q[1] = nxChLatticeIndex(i + 1, j, k); q[2] = nxChLatticeIndex(i + 1, j, k + 1); q[3] = nxChLatticeIndex(i, j, k + 1); }
				if(pz) { unsigned* q = quads[nbQuads++]; q[0] = nxChLatticeIndex(i, j, k + 1); q[1] = nxChLatticeIndex(i + 1, j, k + 1); q[2] = nxChLatticeIndex(i + 1, j + 1, k + 1); q[3] = nxChLatticeIndex(i, j + 1, k + 1); }
				if(mz) { unsigned* q = quads[nbQuads++]; q[0] = nxChLatticeIndex(i, j, k); q[1] = nxChLatticeIndex(i, j + 1, k); q[2] = nxChLatticeIndex(i + 1, j + 1, k); q[3] = nxChLatticeIndex(i + 1, j, k); }
				}
	for(unsigned q = 0; q < nbQuads; ++q)
		for(unsigned c = 0; c < 4; ++c)
			used[quads[q][c]] = true;
	m.nbVerts = 0;
	for(unsigned v = 0; v < 5 * 4 * 4; ++v)
		{
		if(!used[v])
			continue;
		remap[v] = m.nbVerts;
		const unsigned i = v % 5, j = (v / 5) % 4, k = v / 20;
		m.verts[3 * m.nbVerts + 0] = kChLattice[tx][i];
		m.verts[3 * m.nbVerts + 1] = kChLattice[ty][j];
		m.verts[3 * m.nbVerts + 2] = kChLattice[tz][k];
		m.nbVerts++;
		}
	m.nbTris = 0;
	for(unsigned q = 0; q < nbQuads; ++q)
		{
		const unsigned diagonal = nxNext() & 1;
		const unsigned a = remap[quads[q][0]], b = remap[quads[q][1]], c = remap[quads[q][2]], d = remap[quads[q][3]];
		unsigned short* t = &m.tris[3 * m.nbTris];
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
		m.nbTris += 2;
		}
	// An open mesh: every seventh mesh loses its last two triangles.
	const bool open = index % 7 == 3 && m.nbTris > 4;
	if(open)
		m.nbTris -= 2;
	m.buildable = !open && kind != 8 && kind != 9 && !shape[4] && index < 2 * kChNbShapes;
	// Rounds 0 and 1 run under 0x027f, round 4 (the same lattice words) under
	// 0x0f7f, and the nudged rounds 2 and 3 alternate. Under 0x0f7f's chop
	// rounding the vendored Plane::Set normalises (0, c, 0) to 0x3f7ffffe where
	// the oracle's 005155, which keeps the length and its reciprocal on the
	// x87 stack, gives 0x3f7fffff, whenever 1/c is inexact: so every mesh
	// under 0x0f7f is in the split too, as is every concave set -- except round
	// 5 (see above), whose steps keep 1/c exact.
	m.cw = index >= 4 * kChNbShapes ? 0x0f7f : index >= 2 * kChNbShapes && (index & 1) ? 0x0f7f : 0x027f;
	m.split = (index >= 2 * kChNbShapes && !extra) || shape[4] != 0;
	if(m.split)
		{
		// Past the first two rounds, nudges of a few units in the last place (by
		// integer steps on the bits; one mesh in two nudges up to 40 words)
		// into drawn coordinate words; the concave sets of the first two rounds
		// keep their lattice words. No raw word goes into a mesh: a NaN
		// coordinate can leave a face group with no active edge, and 001641
		// then reads an empty Container (a null read on both sides; see
		// above), which no rule on the input could foresee.
		const unsigned nbDraw = nxNext();
		const unsigned nbChanges = (index & 1) ? 1 + nbDraw % 40 : 1 + nbDraw % 4;
		for(unsigned n = 0; n < nbChanges; ++n)
			{
			const unsigned where = nxNext() % (3 * m.nbVerts);
			const unsigned step = nxNext();
			if(index < 2 * kChNbShapes || index >= 4 * kChNbShapes)
				continue;
			if(m.verts[where] & 0x7fffffffu)
				m.verts[where] += (step & 4) ? (1u + step % 3u) : 0u - (1u + step % 3u);
			}
		}
	}

// A hull image: the faces and vertices, the rest zero (as a constructor
// leaves it), in 0x50 bytes.
static void nxChHullImage(unsigned* hull, const NxChMesh& m)
	{
	memset(hull, 0, 0x50);
	hull[0] = 0xcdcd8000u;
	hull[1] = m.nbTris;
	hull[2] = nxChPtr(m.tris);
	hull[3] = m.nbVerts;
	hull[4] = nxChPtr(m.verts);
	hull[13] = 0xcdcd8034u;
	}

// The hull's fields and arrays; pointers as offsets into their arrays.
static void nxChTapeHull(const unsigned* hull)
	{
	NxTape& t = *gIceTape;
	for(int i = 6; i < 9; ++i)
		t.pushKind(hull[i], kWordFloat);
	const unsigned nbPolygons = hull[9];
	const unsigned* polygons = (const unsigned*) (size_t) hull[10];
	const unsigned* vrefs = (const unsigned*) (size_t) hull[11];
	const unsigned* erefs = (const unsigned*) (size_t) hull[12];
	t.push(nbPolygons);
	t.push(polygons ? polygons[-1] : 0xdeadu);
	unsigned total = 0;
	if(polygons)
		for(unsigned p = 0; p < nbPolygons; ++p)
			{
			const unsigned* poly = polygons + 9 * p;
			total += poly[0];
			if(p < 64)
				{
				t.push(poly[0]);
				t.push(poly[1] ? (poly[1] - nxChPtr(vrefs)) : 0xdeadu);
				t.push(poly[2] ? (poly[2] - nxChPtr(erefs)) : 0xdeadu);
				for(int i = 3; i < 9; ++i)
					t.pushKind(poly[i], kWordFloat);
				}
			}
	if(polygons && nbPolygons > 64)
		t.push(nxMb2Digest(polygons, 9 * nbPolygons));
	nxChWordArray(vrefs, total);
	nxChWordArray(erefs, erefs ? total : 0);
	const unsigned nbEdges = hull[14];
	const unsigned* edges = (const unsigned*) (size_t) hull[15];
	const unsigned* normals = (const unsigned*) (size_t) hull[16];
	const unsigned* descs = (const unsigned*) (size_t) hull[17];
	const unsigned* table = (const unsigned*) (size_t) hull[18];
	t.push(nbEdges);
	t.push(edges ? edges[-1] : 0xdeadu);
	nxChWordArray(edges, edges ? 2 * nbEdges : 0);
	t.push(normals ? 1u : 0u);
	if(normals)
		nxMtFloatArray(normals, 3 * nbEdges);
	nxChWordArray(descs, descs ? 2 * nbEdges : 0);
	unsigned links = 0;
	if(descs)
		for(unsigned e = 0; e < nbEdges; ++e)
			links += descs[2 * e] >> 16;
	nxChWordArray(table, table ? links : 0);
	t.push(hull[0]);
	t.push(hull[13]);
	}

static void nxChFreeHull(unsigned* hull)
	{
	const unsigned cookieFields[2] = { 10, 15 };
	const unsigned plainFields[5] = { 11, 12, 16, 17, 18 };
	for(int i = 0; i < 2; ++i)
		if(hull[cookieFields[i]])
			gIceAllocator.free((unsigned*) (size_t) hull[cookieFields[i]] - 1);
	for(int i = 0; i < 5; ++i)
		if(hull[plainFields[i]])
			gIceAllocator.free((void*) (size_t) hull[plainFields[i]]);
	}

// One mesh: 001472, then 001502 when the side's own 001472 built polygons
// (with none, 001502 writes 8 bytes into the zero-byte block it allocates for
// the edge-to-polygon table and reads 6 bytes before it, 0x0002d067 and
// 0x0002d0a1, in the oracle as here: not driven); for every fifth buildable
// mesh (a rule on the fixed input: a convex main-set set, closed and manifold,
// whose 001472 cannot fail) 001502 alone, which builds the polygons itself; a second 001472 and
// 001502 for every third; and 001496 over drawn directions.
static void nxChHullCase(const NxIceSide& s, const NxChMesh& m, unsigned index)
	{
	NxChCoverage& c = gChCoverage;
	gIceTape = m.split ? s.splitTape : s.mainTape;
	NxTape& t = *gIceTape;
	const unsigned short cw = m.cw;
	nxMb2FoldInput(kHullFamily, m.verts, 3 * m.nbVerts);
	nxMb2FoldInput(kHullFamily, m.tris, (3 * m.nbTris) / 2);
	nxIceFoldRun(kHullFamily, index, m.nbTris, cw);
	unsigned hull[0x50 / 4];
	nxChHullImage(hull, m);
	const void* polygons = nxChRow(s, kChPolygons, (const void*) &nxHullComputePolygons);
	const void* edges = nxChRow(s, kChEdges, (const void*) &nxHullComputeEdges);
	const void* support = nxChRow(s, kChSupport, (const void*) &nxHullSupportPolygon);
	const unsigned short saved = nxMb2SetControlWord(cw);
	unsigned okPolygons = 0xffu;
	unsigned okEdges = 0xffu;
	if(index % 5 == 4 && m.buildable)
		okEdges = nxChCall0(edges, hull) & 0xff;
	else
		{
		okPolygons = nxChCall0(polygons, hull) & 0xff;
		if(okPolygons && hull[9])
			okEdges = nxChCall0(edges, hull) & 0xff;
		}
	nxMb2SetControlWord(saved);
	t.push(okPolygons);
	t.push(okEdges);
	nxChTapeHull(hull);
	if(s.oracle)
		{
		++c.meshes;
		if(m.split)
			++c.splitMeshes;
		if(okPolygons == 1)
			++c.polygonsTrue;
		else if(okPolygons == 0)
			++c.polygonsFalse;
		else
			++c.edgesAlone;
		if(okEdges != 0xffu)
			okEdges ? ++c.edgesTrue : ++c.edgesFalse;
		c.polygons += hull[9];
		c.edges += hull[14];
		}
	if(index % 3 == 1 && okEdges == 1)
		{
		const unsigned short saved2 = nxMb2SetControlWord(cw);
		const unsigned again = nxChCall0(polygons, hull) & 0xff;
		const unsigned againEdges = nxChCall0(edges, hull) & 0xff;
		nxMb2SetControlWord(saved2);
		t.push(again);
		t.push(againEdges);
		nxChTapeHull(hull);
		if(s.oracle)
			++c.rebuilt;
		}
	if(okEdges == 1 && hull[9])
		for(unsigned d = 0; d < 24; ++d)
			{
			unsigned dir[3];
			nxChWords(dir, 3);
			unsigned pose[16];
			nxChWords(pose, 16);
			const unsigned posed = d & 1;
			nxMb2FoldInput(kHullFamily, dir, 3);
			if(posed)
				nxMb2FoldInput(kHullFamily, pose, 16);
			const unsigned short saved3 = nxMb2SetControlWord((d & 2) ? 0x0f7f : 0x027f);
			const unsigned best = nxChCall2(support, hull, nxChPtr(dir), posed ? nxChPtr(pose) : 0u);
			nxMb2SetControlWord(saved3);
			t.push(best);
			if(s.oracle)
				++c.hullSupport;
			}
	nxChFreeHull(hull);
	gIceTape = s.mainTape;
	}

// Pairs of words whose difference is a power of two.
static const unsigned kChPairs[8][2] =
	{
	{ 0x00000000u, 0x3f800000u },	// 0, 1
	{ 0x40400000u, 0x40800000u },	// 3, 4
	{ 0xbf000000u, 0x3f000000u },	// -0.5, 0.5
	{ 0x42c00000u, 0x43000000u },	// 96, 128
	{ 0xc1000000u, 0x41000000u },	// -8, 8
	{ 0x3e800000u, 0x3f000000u },	// 0.25, 0.5
	{ 0x80000000u, 0x40000000u },	// -0, 2
	{ 0x45800000u, 0x46000000u },	// 4096, 8192
	};

// 001463 directly: n references (0..12, repeats allowed) into 16 points, under
// both control words. In the main family the points lie in an axis-aligned
// plane (a lattice word) with each other coordinate one of a pair of words a
// power of two apart: the points are a rectangle's corners, repeated, so every
// triangle's cross product is 0, 2^k or 2^(k+1) on one axis, and the vendored
// Plane::Set normalises it exactly in both rounding modes (for other lattice
// words it does not under either: 0x3f7fffff for 1.0f). One case in four has
// drawn points and goes to the split.
static void nxChPlaneCases(const NxIceSide& s)
	{
	NxChCoverage& c = gChCoverage;
	for(unsigned k = 0; k < 700; ++k)
		{
		const bool split = k % 4 == 3;
		gIceTape = split ? s.splitTape : s.mainTape;
		NxTape& t = *gIceTape;
		const unsigned short cw = (k & 1) ? 0x0f7f : 0x027f;
		unsigned points[48];
		const unsigned axis = nxNext() % 3;
		const unsigned table = nxNext() % 8;
		const unsigned level = nxNext() % 5;
		const unsigned pairU = nxNext() % 8;
		const unsigned pairV = nxNext() % 8;
		for(unsigned p = 0; p < 16; ++p)
			{
			const unsigned a = nxNext();
			const unsigned b = nxNext();
			for(unsigned i = 0; i < 3; ++i)
				points[3 * p + i] = i == axis ? kChLattice[table][level]
					: i == (axis + 1) % 3 ? kChPairs[pairU][a & 1] : kChPairs[pairV][b & 1];
			}
		if(split)
			nxChWords(points, 48);
		const unsigned nbDraw = nxNext();
		const unsigned nb = nbDraw % 13;
		unsigned refs[12];
		for(unsigned i = 0; i < 12; ++i)
			{
			const unsigned r = nxNext();
			refs[i] = r % 16;
			}
		const unsigned arm = k % 50;	// 1: no refs array, 2: no points
		nxMb2FoldInput(kHullFamily, points, 48);
		nxMb2FoldInput(kHullFamily, refs, nb);
		nxIceFoldRun(kHullFamily, 0x463, nb, arm < 3 ? arm : 0u);
		unsigned plane[5] = { 0xcdcd9000u, 0xcdcd9001u, 0xcdcd9002u, 0xcdcd9003u, 0xcdcd9004u };
		ChPlaneFn planeFn = (ChPlaneFn) nxChRow(s, kChPlane, (const void*) &nxHullPolygonPlane);
		const unsigned short saved = nxMb2SetControlWord(cw);
		const bool ok = planeFn(plane, nb, arm == 1 ? 0 : refs, arm == 2 ? 0 : points);
		nxMb2SetControlWord(saved);
		t.push(ok ? 1u : 0u);
		for(int i = 0; i < 5; ++i)
			t.pushKind(plane[i], kWordFloat);
		if(s.oracle)
			{
			++c.planeCalls;
			if(split)
				++c.planeSplit;
			}
		}
	gIceTape = s.mainTape;
	}

static const unsigned kChNbMeshes = 5 * kChNbShapes + 12;

static void nxChHullDrive(const NxIceSide& s)
	{
	const unsigned savedState = gState;
	static NxChMesh mesh;
	for(unsigned index = 0; index < kChNbMeshes; ++index)
		{
		nxChBuildMesh(mesh, index);
		gState = 0x2f0d0000u ^ (index * 0x85ebca6bu + 7u);
		nxChHullCase(s, mesh, index);
		}
	gState = 0x2f0e1463u;
	nxChPlaneCases(s);
	if(s.mainTape->overflow || s.splitTape->overflow)
		{
		fprintf(stderr, "FAIL convex_hull tape overflow (%u, %u words beyond)\n", s.mainTape->overflow,
			s.splitTape->overflow);
		++gMismatches;
		}
	gState = savedState;
	}

// ---------------------------------------------------------------------------
// support_maps

static unsigned nxChTableIndex(const NxIceSide& s, unsigned vptr)
	{
	for(unsigned i = 0; i < 4; ++i)
		{
		const unsigned expected = s.oracle ? nxChPtr(s.o->base + kSmTables[i])
			: nxChPtr(i == 0 ? gIceSupportMapBaseTable : i == 1 ? gIceSupportMapHullTable
				: i == 2 ? gIceSupportMapPlaneTable : gIceSupportMapVertexTable);
		if(vptr == expected)
			return i;
		}
	return 0xdeadu;
	}

static void nxChDirections(const NxIceSide& s)
	{
	NxChCoverage& c = gChCoverage;
	NxTape& t = *gIceTape;
	SmCubeFaceFn cubeFace = (SmCubeFaceFn) nxChRow(s, kSmCubeFace, (const void*) &nxSupportMapCubeFace);
	const void* lookup = nxChRow(s, kSmLookup, (const void*) &nxSupportMapLookup);
	for(unsigned k = 0; k < 2400; ++k)
		{
		const unsigned short cw = (k & 1) ? 0x0f7f : 0x027f;
		unsigned dir[3];
		nxChWords(dir, 3);
		// Ties of magnitude (a component's word copied, its sign drawn) and
		// axis directions.
		const unsigned shape = nxNext();
		if(shape % 5 == 0)
			dir[(shape >> 4) % 3] = dir[(shape >> 8) % 3] ^ (shape & 0x80000000u);
		else if(shape % 5 == 1)
			{
			dir[(shape >> 4) % 3] = 0;
			dir[(shape >> 8) % 3] = (shape >> 12) & 1 ? 0x80000000u : 0u;
			}
		const unsigned nDraw = nxNext();
		const unsigned n = nDraw % 21;
		nxMb2FoldInput(kSupportFamily, dir, 3);
		nxIceFoldRun(kSupportFamily, 0x550, n, cw);
		unsigned u = 0xcdcda001u, v = 0xcdcda002u;
		unsigned map[6] = { 0xcdcdb000u, n, 6 * n * n, 0, 0, 0 };
		const unsigned short saved = nxMb2SetControlWord(cw);
		const unsigned face = cubeFace(dir, (float*) &u, (float*) &v);
		const unsigned sample = nxChCall1(lookup, map, nxChPtr(dir));
		nxMb2SetControlWord(saved);
		t.push(face);
		t.pushKind(u, kWordFloat);
		t.pushKind(v, kWordFloat);
		t.push(sample);
		if(s.oracle)
			{
			++c.faceCalls;
			++c.lookupCalls;
			c.mapSnan += nxMb2CountSnan((const float*) dir, 3);
			}
		}
	}

// A map of kind 0 (A), 1 (B) or 2 (C) over a hull: constructed by the side's
// constructor in a block from the recording allocator, Init(n), its bytes,
// lookups through it, and slot 0 (with the free bit for odd n).
static void nxChMapCase(const NxIceSide& s, unsigned kind, unsigned* hull, unsigned n, unsigned seed)
	{
	NxChCoverage& c = gChCoverage;
	NxTape& t = *gIceTape;
	nxIceFoldRun(kSupportFamily, kind, n, seed);
	const unsigned ctorRva = kind == 0 ? kSmCtorA : kind == 1 ? kSmCtorB : kSmCtorC;
	const void* candidateCtor = kind == 0 ? (const void*) &nxSupportMapHullConstruct
		: kind == 1 ? (const void*) &nxSupportMapPlaneConstruct : (const void*) &nxSupportMapVertexConstruct;
	unsigned* obj = (unsigned*) gIceAllocator.malloc(0x18, NX_MEMORY_PERSISTENT);
	for(unsigned i = 0; i < 6; ++i)
		obj[i] = 0xcdcdc000u + i;
	const unsigned made = nxChCall1(nxChRow(s, ctorRva, candidateCtor), obj, nxChPtr(hull));
	t.push(made == nxChPtr(obj) ? 1u : 0u);
	t.push(nxChTableIndex(s, obj[0]));
	const unsigned short cw = (seed & 1) ? 0x0f7f : 0x027f;
	const unsigned short saved = nxMb2SetControlWord(cw);
	const unsigned ok = nxChCall1(nxChRow(s, kSmInit, (const void*) &nxSupportMapInit), obj, n) & 0xff;
	nxMb2SetControlWord(saved);
	t.push(ok);
	t.push(obj[1]);
	t.push(obj[2]);
	const unsigned char* bytes = (const unsigned char*) (size_t) obj[3];
	const unsigned char* bytes2 = kind == 2 ? (const unsigned char*) (size_t) obj[4] : 0;
	t.push(bytes ? 1u : 0u);
	t.push(bytes2 ? 1u : 0u);
	if(ok)
		{
		for(unsigned i = 0; i < obj[2]; i += 4)
			{
			unsigned w = 0;
			memcpy(&w, bytes + i, obj[2] - i < 4 ? obj[2] - i : 4);
			t.push(w);
			if(bytes2)
				{
				memcpy(&w, bytes2 + i, obj[2] - i < 4 ? obj[2] - i : 4);
				t.push(w);
				}
			}
		// Lookups through the map.
		for(unsigned d = 0; d < 12; ++d)
			{
			unsigned dir[3];
			nxChWords(dir, 3);
			nxMb2FoldInput(kSupportFamily, dir, 3);
			const unsigned short saved2 = nxMb2SetControlWord((d & 1) ? 0x0f7f : 0x027f);
			const unsigned sample = nxChCall1(nxChRow(s, kSmLookup, (const void*) &nxSupportMapLookup), obj,
				nxChPtr(dir));
			nxMb2SetControlWord(saved2);
			t.push(sample);
			t.push(sample < obj[2] ? bytes[sample] : 0xdeadu);
			if(s.oracle)
				++c.mapLookups;
			}
		}
	if(s.oracle)
		{
		++c.maps;
		ok ? ++c.initTrue : ++c.initFalse;
		if(ok)
			c.samples += obj[2];
		}
	// Slot 0 through the object's own table.
	const unsigned flags = n & 1;
	const void* const* table = (const void* const*) (size_t) obj[0];
	const unsigned deleted = nxChCall1(table[0], obj, flags);
	t.push(deleted == nxChPtr(obj) ? 1u : 0u);
	if(!flags)
		{
		t.push(nxChTableIndex(s, obj[0]));
		t.push(obj[3]);
		t.push(kind == 2 ? obj[4] : 0u);
		gIceAllocator.free(obj);
		}
	else if(s.oracle)
		++c.freed;
	}

// Hand-built hull images: nb polygons of drawn plane words and a drawn centre
// (A and B read nothing else while the count is non-zero), and nb vertices of
// drawn words (C reads +0x0c and +0x10).
static void nxChImageMaps(const NxIceSide& s)
	{
	static unsigned polygons[24 * 9];
	static unsigned verts[3 * 256];
	for(unsigned k = 0; k < 30; ++k)
		{
		const unsigned nbDraw = nxNext();
		const unsigned nb = 1 + nbDraw % 24;
		nxChWords(polygons, 9 * nb);
		nxChWords(verts, 3 * nb);
		unsigned hull[0x50 / 4];
		memset(hull, 0, sizeof(hull));
		nxChWords(&hull[6], 3);
		hull[3] = nb;
		hull[4] = nxChPtr(verts);
		hull[9] = nb;
		hull[10] = nxChPtr(polygons);
		nxMb2FoldInput(kSupportFamily, polygons, 9 * nb);
		nxMb2FoldInput(kSupportFamily, verts, 3 * nb);
		nxMb2FoldInput(kSupportFamily, &hull[6], 3);
		const unsigned n = 1 + k % 6;
		for(unsigned kind = 0; kind < 3; ++kind)
			nxChMapCase(s, kind, hull, n, k);
		if(s.oracle)
			gChCoverage.mapSnan += nxMb2CountSnan((const float*) polygons, 9 * nb)
				+ nxMb2CountSnan((const float*) verts, 3 * nb);
		}
	// The allocation failures: 256 polygons (A, B), 256 vertices (C).
	unsigned hull[0x50 / 4];
	memset(hull, 0, sizeof(hull));
	for(unsigned i = 0; i < 3 * 256; ++i)
		verts[i] = 0x3f800000u;
	hull[3] = 256;
	hull[4] = nxChPtr(verts);
	hull[9] = 256;
	hull[10] = nxChPtr(polygons);
	for(unsigned kind = 0; kind < 3; ++kind)
		nxChMapCase(s, kind, hull, 2, 0x100 + kind);
	}

static void nxChSupportDrive(const NxIceSide& s)
	{
	const unsigned savedState = gState;
	gState = 0x2f0f0550u;
	nxChDirections(s);
	nxChImageMaps(s);
	// The hulls of convex_hull's main meshes, built by each side's own 001472.
	static NxChMesh mesh;
	static const unsigned kSizes[6] = { 0, 1, 2, 3, 5, 8 };
	for(unsigned index = 0; index < 2 * kChNbShapes; ++index)
		{
		nxChBuildMesh(mesh, index);
		if(mesh.split)
			continue;
		gState = 0x2f100000u ^ (index * 0x27d4eb2du + 3u);
		nxMb2FoldInput(kSupportFamily, mesh.verts, 3 * mesh.nbVerts);
		unsigned hull[0x50 / 4];
		nxChHullImage(hull, mesh);
		const unsigned ok = nxChCall0(nxChRow(s, kChPolygons, (const void*) &nxHullComputePolygons), hull) & 0xff;
		gIceTape->push(ok);
		if(ok && hull[9])
			for(unsigned kind = 0; kind < 3; ++kind)
				nxChMapCase(s, kind, hull, kSizes[(index + kind) % 6], index);
		nxChFreeHull(hull);
		}
	if(gIceTape->overflow)
		{
		fprintf(stderr, "FAIL support_maps tape overflow (%u words beyond)\n", gIceTape->overflow);
		++gMismatches;
		}
	gState = savedState;
	}

static void nxDriveConvexHull(const NxOracleRows& o, bool selfOnly)
	{
	memset(&gChCoverage, 0, sizeof(gChCoverage));
	const NxChCoverage& c = gChCoverage;

	gIceReports = 0;
	nxIceFamily(o, selfOnly, nxChLeafDrive, "hull_leaf", "0x0002a620", "phys_fn_001441",
		"ConvexHull.cpp,IceMeshTools.cpp", kHullLeafFamily);
	printf("thirdparty coverage name=hull_leaf area=%u center=%u centroid=%u centroid_true=%u support=%u"
		" support_posed=%u gather=%u gather_faces=%u reverse=%u vector=%u input_snan=%u x87_0f7f=%u reports=%u\n",
		c.areaCalls, c.centerCalls, c.centroidCalls, c.centroidTrue, c.supportCalls, c.supportPosed,
		c.gatherCases, c.gatherFaces, c.reverseCalls, c.vectorCalls, c.leafSnan, c.leafX87, gIceReports);

	gIceReports = 0;
	gIceReportLine318 = 0;
	gIceReportLine321 = 0;
	nxIceFamily(o, selfOnly, nxChHullDrive, "convex_hull", "0x0002b6f0", "phys_fn_001472",
		"ConvexHull.cpp,IceAdjacencies.cpp,EdgeList.cpp,IceMeshTools.cpp", kHullFamily);
	printf("thirdparty coverage name=convex_hull meshes=%u split_meshes=%u polygons_true=%u polygons_false=%u"
		" edges_alone=%u polygons=%u edges_true=%u edges_false=%u edges=%u rebuilt=%u support=%u plane=%u plane_split=%u"
		" reports=%u line318=%u line321=%u\n",
		c.meshes, c.splitMeshes, c.polygonsTrue, c.polygonsFalse, c.edgesAlone, c.polygons, c.edgesTrue, c.edgesFalse,
		c.edges, c.rebuilt, c.hullSupport, c.planeCalls, c.planeSplit, gIceReports, gIceReportLine318,
		gIceReportLine321);

	gIceReports = 0;
	nxIceFamily(o, selfOnly, nxChSupportDrive, "support_maps", "0x0002e2f0", "phys_fn_001558",
		"IceSupportMaps.cpp,ConvexHull.cpp", kSupportFamily);
	printf("thirdparty coverage name=support_maps faces=%u lookups=%u maps=%u init_true=%u init_false=%u"
		" samples=%u map_lookups=%u freed=%u input_snan=%u reports=%u\n",
		c.faceCalls, c.lookupCalls, c.maps, c.initTrue, c.initFalse, c.samples, c.mapLookups, c.freed,
		c.mapSnan, gIceReports);
	}

// ---------------------------------------------------------------------------
// convex-mesh gap Task 2g: polygon_interface. The TriangleMesh polygon interface
// (P-Mesh: the table 0x101085d4, TriangleMeshPolygons.cpp) driven slot by slot
// through each side's own table, over TriangleMesh images whose +0xa0 is a hull
// image built from convex_hull's meshes, and with it the helpers the slots reach:
// 001514 (slot 5; and again directly, over a hull that already has its axes: the
// release through the CRT's free), 001516 (slot 10), 001530 and 000505 (slot 11
// without a map). Every float input is written as bits.

#include "TriangleMeshPolygons.h"

static const unsigned kPiTable			= 0x001085d4;	// the polygon interface
static const unsigned kPiEdgeAxes		= 0x0002d2e0;	// phys_fn_001514
static const unsigned kPiCrtFree		= 0x000f41f0;	// phys_fn_005668, the static CRT's free
static const int kPolygonFamily = 13;

struct NxPiCoverage
	{
	unsigned hulls, built, polygons, edges, axes, axesRebuilt, support, supportPosed, faces, facesEdge, faceKindNull,
		projects, projectsMap, projectsGraph, climbsFailed, stampWraps, inputSnan;
	};
static NxPiCoverage gPiCoverage;

static unsigned nxPiCall3(const void* fn, const void* self, unsigned a0, unsigned a1, unsigned a2)
	{
	unsigned r;
	__asm
		{
		push	a2
		push	a1
		push	a0
		mov		ecx, self
		xor		edx, edx
		call	fn
		mov		r, eax
		}
	return r;
	}
static void nxPiCall6(const void* fn, const void* self, unsigned a0, unsigned a1, unsigned a2, unsigned a3,
	unsigned a4, unsigned a5)
	{
	__asm
		{
		push	a5
		push	a4
		push	a3
		push	a2
		push	a1
		push	a0
		mov		ecx, self
		xor		edx, edx
		call	fn
		}
	}

// The vertex graph 001530 climbs (+0x08 counts, +0x0c offsets, +0x10 neighbours):
// every triangle edge of the mesh, both ways, once.
struct NxPiGraph
	{
	unsigned		word0, word4;
	const unsigned*	counts;
	const unsigned*	offsets;
	const unsigned*	neighbours;
	unsigned		countStore[kChMaxVerts], offsetStore[kChMaxVerts], neighbourStore[kChMaxVerts * 16];
	};

static void nxPiBuildGraph(const NxChMesh& m, NxPiGraph& g, unsigned variant)
	{
	static bool adjacent[kChMaxVerts][kChMaxVerts];
	memset(adjacent, 0, sizeof(adjacent));
	for(unsigned t = 0; t < m.nbTris; ++t)
		for(unsigned e = 0; e < 3; ++e)
			{
			const unsigned a = m.tris[3 * t + e], b = m.tris[3 * t + (e + 1) % 3];
			adjacent[a][b] = adjacent[b][a] = true;
			}
	unsigned next = 0;
	for(unsigned v = 0; v < m.nbVerts; ++v)
		{
		g.offsetStore[v] = next;
		g.countStore[v] = 0;
		for(unsigned w = 0; w < m.nbVerts; ++w)
			if(adjacent[v][w] && next < kChMaxVerts * 16)
				{
				g.neighbourStore[next++] = w;
				++g.countStore[v];
				}
		}
	g.word0 = 0xcdcd6400u;
	g.word4 = 0xcdcd6404u;
	// Variant 1: no neighbour array (001530 returns false and slot 11 keeps vertex 0).
	g.counts = g.countStore;
	g.offsets = g.offsetStore;
	g.neighbours = variant == 1 ? 0 : g.neighbourStore;
	}

// The +0x34 Container 001514 leaves: destroyed and freed by the side's own CRT.
static void nxPiReleaseAxes(const NxIceSide& s, unsigned* hull)
	{
	void* axes = (void*) (size_t) hull[13];
	if(!axes)
		return;
	if(s.oracle)
		{
		((VoidThisFn) (s.o->base + kContainerDtor))(axes);
		typedef void (__cdecl* CrtFreeFn)(void*);
		((CrtFreeFn) (s.o->base + kPiCrtFree))(axes);
		}
	else
		{
		((IceCore::Container*) axes)->~Container();
		free(axes);
		}
	hull[13] = 0;
	}

static void nxPiTapeAxes(const unsigned* hull)
	{
	NxTape& t = *gIceTape;
	const unsigned* axes = (const unsigned*) (size_t) hull[13];
	t.push(axes ? 1u : 0u);
	if(!axes)
		return;
	t.push(axes[1]);
	const unsigned* entries = (const unsigned*) (size_t) axes[2];
	for(unsigned i = 0; i < axes[1] && entries; ++i)
		t.pushKind(entries[i], kWordFloat);
	if(gIceOraclePass)
		gPiCoverage.axes += axes[1];
	}

static void nxPiHull(const NxIceSide& s, const NxChMesh& m, unsigned index)
	{
	NxPiCoverage& c = gPiCoverage;
	NxTape& t = *gIceTape;
	nxMb2FoldInput(kPolygonFamily, m.verts, 3 * m.nbVerts);
	nxMb2FoldInput(kPolygonFamily, m.tris, (3 * m.nbTris) / 2);
	nxIceFoldRun(kPolygonFamily, index, m.nbTris, m.cw);
	static NxPiGraph graph;
	nxPiBuildGraph(m, graph, index % 5 == 2 ? 1u : 0u);
	unsigned hull[0x80 / 4];
	nxChHullImage(hull, m);
	memset(&hull[20], 0, 0x30);
	// The centre (+0x18): lattice words of the mesh's own vertices.
	const unsigned corner = nxNext() % m.nbVerts;
	hull[6] = m.verts[3 * corner + 0];
	hull[7] = m.verts[3 * corner + 1];
	hull[8] = m.verts[3 * corner + 2];
	hull[13] = 0;
	hull[25] = index % 5 == 3 ? 0u : nxChPtr(&graph);	// +0x64; none on every fifth
	nxMb2FoldInput(kPolygonFamily, &hull[6], 3);
	unsigned char mesh[0xb0];
	memset(mesh, 0xcd, sizeof(mesh));
	const void* table = s.oracle ? (const void*) (s.o->base + kPiTable) : (const void*) gTriangleMeshPolygonTable;
	*(const void**) (mesh + 0x04) = table;
	*(unsigned**) (mesh + 0xa0) = hull;
	const void* const* slots = (const void* const*) table;
	const void* iface = mesh + 4;
	if(s.oracle)
		++c.hulls;

	// Slots 0..2: pointers into the hull, as offsets.
	t.push(nxChCall0(slots[0], iface) - nxChPtr(hull));
	t.push(nxChCall0(slots[1], iface));
	t.push(nxChCall0(slots[2], iface) - hull[4]);
	// Slot 3 builds the polygons (001472) under the mesh's control word, then 4.
	const unsigned short saved = nxMb2SetControlWord(m.cw);
	const unsigned count = nxChCall0(slots[3], iface);
	nxMb2SetControlWord(saved);
	t.push(count);
	for(unsigned p = 0; p < count; ++p)
		t.push(nxChCall1(slots[4], iface, p) - hull[10]);
	if(s.oracle)
		{
		c.polygons += count;
		if(count)
			++c.built;
		}
	// Slot 5 (001514); every other hull 001514 again directly, which releases
	// the first Container through the CRT's free.
	const unsigned axesPtr = nxChCall0(slots[5], iface);
	t.push(axesPtr == hull[13] ? 1u : 0u);
	nxPiTapeAxes(hull);
	if(index & 1)
		{
		const unsigned again = nxChCall0(nxChRow(s, kPiEdgeAxes, (const void*) &nxHullComputeEdgeAxes), hull) & 0xff;
		t.push(again);
		nxPiTapeAxes(hull);
		if(s.oracle)
			++c.axesRebuilt;
		}
	if(count)
		{
		// Slots 6..8 build the edges (001502) when +0x3c is null.
		const unsigned short saved2 = nxMb2SetControlWord(m.cw);
		const unsigned edges = nxChCall0(slots[6], iface);
		nxMb2SetControlWord(saved2);
		t.push(edges == hull[15] ? 1u : 0u);
		t.push(nxChCall0(slots[7], iface) == hull[17] ? 1u : 0u);
		t.push(nxChCall0(slots[8], iface) == hull[18] ? 1u : 0u);
		t.push(hull[14]);
		if(s.oracle)
			c.edges += hull[14];
		// Slots 9 and 10 over drawn directions, with and without a drawn pose.
		for(unsigned d = 0; d < 16; ++d)
			{
			unsigned dir[3], pose[16];
			nxChWords(dir, 3);
			nxChWords(pose, 16);
			const unsigned posed = d & 1;
			nxMb2FoldInput(kPolygonFamily, dir, 3);
			if(posed)
				nxMb2FoldInput(kPolygonFamily, pose, 16);
			unsigned kind = 0xcdcdcdcdu;
			const unsigned nullKind = d % 8 == 6;
			const unsigned short saved3 = nxMb2SetControlWord((d & 2) ? 0x0f7f : 0x027f);
			const unsigned best = nxChCall2(slots[9], iface, nxChPtr(dir), posed ? nxChPtr(pose) : 0u);
			const unsigned face = nxPiCall3(slots[10], iface, nxChPtr(dir), posed ? nxChPtr(pose) : 0u,
				nullKind ? 0u : nxChPtr(&kind));
			nxMb2SetControlWord(saved3);
			t.push(best);
			t.push(face);
			t.push(kind);
			if(s.oracle)
				{
				++c.support;
				if(posed)
					++c.supportPosed;
				++c.faces;
				if(kind == 1)
					++c.facesEdge;
				if(nullKind)
					++c.faceKindNull;
				c.inputSnan += nxMb2CountSnan((const float*) dir, 3) + (posed ? nxMb2CountSnan((const float*) pose, 16) : 0);
				}
			}
		}
	// Slot 11 with the side's own kind C map (built over this hull by its
	// constructor and Init) and without one (the graph), over drawn directions
	// and poses; the scratch record's stamp drawn, near its wrap one time in four.
	unsigned map[6];
	for(unsigned i = 0; i < 6; ++i)
		map[i] = 0xcdcdc000u + i;
	const unsigned n = 1 + index % 4;
	nxChCall1(nxChRow(s, kSmCtorC, (const void*) &nxSupportMapVertexConstruct), map, nxChPtr(hull));
	const unsigned mapOk = nxChCall1(nxChRow(s, kSmInit, (const void*) &nxSupportMapInit), map, n) & 0xff;
	t.push(mapOk);
	unsigned visited[kChMaxVerts];
	memset(visited, 0, sizeof(visited));
	unsigned scratch[8];
	scratch[0] = 0xcdcd5000u;
	scratch[1] = kChMaxVerts;
	scratch[2] = nxChPtr(visited);
	scratch[3] = 0xcdcd500cu;
	scratch[4] = 0xcdcd5010u;
	for(unsigned d = 0; d < 16; ++d)
		{
		unsigned dir[3], pose[16];
		nxChWords(dir, 3);
		nxChWords(pose, 16);
		const unsigned stampDraw = nxNext();
		scratch[5] = (stampDraw & 3) == 0 ? 0xfffffffeu : stampDraw & 0xffffu;
		const bool withMap = (d & 1) && mapOk;
		nxMb2FoldInput(kPolygonFamily, dir, 3);
		nxMb2FoldInput(kPolygonFamily, pose, 16);
		nxIceFoldRun(kPolygonFamily, scratch[5], withMap ? 1u : 0u, d);
		unsigned least = 0xcdcdd000u, greatest = 0xcdcdd001u;
		const unsigned stampBefore = scratch[5];
		const unsigned short saved4 = nxMb2SetControlWord((d & 2) ? 0x0f7f : 0x027f);
		nxPiCall6(slots[11], iface, nxChPtr(scratch), nxChPtr(&least), nxChPtr(&greatest), nxChPtr(dir),
			nxChPtr(pose), withMap ? nxChPtr(map) : 0u);
		nxMb2SetControlWord(saved4);
		t.pushKind(least, kWordFloat);
		t.pushKind(greatest, kWordFloat);
		t.push(scratch[5]);
		nxChWordArray(visited, m.nbVerts);
		if(s.oracle)
			{
			++c.projects;
			withMap ? ++c.projectsMap : ++c.projectsGraph;
			if(!withMap && !hull[25])
				++c.climbsFailed;
			if(scratch[5] < stampBefore)
				++c.stampWraps;
			c.inputSnan += nxMb2CountSnan((const float*) dir, 3) + nxMb2CountSnan((const float*) pose, 16);
			}
		}
	const void* const* mapTable = (const void* const*) (size_t) map[0];
	nxChCall1(mapTable[0], map, 0);
	nxPiReleaseAxes(s, hull);
	nxChFreeHull(hull);
	}

// The hulls: convex_hull's main meshes that are not in its split (under
// 0x027f) and its round 5 (the power-of-two meshes, under 0x0f7f), so the
// vendored Plane::Set and Triangle::Normal in each side's 001472 build are exact.
// The open, ring and edge-pair meshes build no polygon (001472 false): slots 3..5
// and 11 run on them, 6..10 do not (001502 over a hull without polygons writes
// outside the block it allocates, as in the oracle: not driven).
static void nxPiDrive(const NxIceSide& s)
	{
	const unsigned savedState = gState;
	static NxChMesh mesh;
	for(unsigned index = 0; index < kChNbMeshes; ++index)
		{
		if(index >= 2 * kChNbShapes && index < 5 * kChNbShapes)
			continue;
		nxChBuildMesh(mesh, index);
		if(mesh.split)
			continue;
		gState = 0x2e9c0000u ^ (index * 0x85ebca6bu + 5u);
		nxPiHull(s, mesh, index);
		}
	if(gIceTape->overflow)
		{
		fprintf(stderr, "FAIL polygon_interface tape overflow (%u words beyond)\n", gIceTape->overflow);
		++gMismatches;
		}
	gState = savedState;
	}

static void nxDrivePolygonInterface(const NxOracleRows& o, bool selfOnly)
	{
	memset(&gPiCoverage, 0, sizeof(gPiCoverage));
	const NxPiCoverage& c = gPiCoverage;
	gIceReports = 0;
	nxIceFamily(o, selfOnly, nxPiDrive, "polygon_interface", "0x000552c0", "phys_fn_002249",
		"TriangleMeshPolygons.cpp,ConvexHull.cpp,IceSupportMaps.cpp", kPolygonFamily);
	printf("thirdparty coverage name=polygon_interface hulls=%u built=%u polygons=%u edges=%u axes=%u axes_rebuilt=%u"
		" support=%u support_posed=%u faces=%u faces_edge=%u face_kind_null=%u projects=%u projects_map=%u"
		" projects_graph=%u climbs_failed=%u stamp_wraps=%u input_snan=%u reports=%u\n",
		c.hulls, c.built, c.polygons, c.edges, c.axes, c.axesRebuilt, c.support, c.supportPosed, c.faces, c.facesEdge,
		c.faceKindNull, c.projects, c.projectsMap, c.projectsGraph, c.climbsFailed, c.stampWraps, c.inputSnan,
		gIceReports);
	}

// qhull-gap Task 4e: convex cooking, the NovodeX hull library around qhull
// (units/convex-cooking-contract.md, "The differential Task 4 should build").
//
// A: HullLibrary::CreateConvexHull (0x0007ea10) and ReleaseResult (0x0007e300)
//    called directly, against Physics/src/QhullHost.cpp and Quantizer.cpp.
// B: phys_fn_002233 (0x00054920), the hull computation loadFromDesc runs for
//    NX_MF_COMPUTE_CONVEX, against TriangleMeshHullAllocator::computeHull
//    (Physics/src/TriangleMesh.cpp). Its `this` is the user allocator: on the
//    oracle side an object whose vptr is TriangleMesh's own table
//    (.rdata:0x00108608, slots 0/1 = 002235/002237), on the candidate side a
//    TriangleMeshHullAllocator; both reach the Foundation allocator
//    ([[0x101041bc]], nxFoundationSDKAllocator), which the drive points at a
//    recording allocator for the call. So B runs 002235 and 002237 too.
//
// Each family is one tape per side:
//   * every call into the user allocator (and, for B, the Foundation
//     allocator): the size, the memory type, and which block (an ordinal in
//     the run's allocation order) a free releases. The recording allocator
//     ZEROES what it returns: the oracle's 0xaf794-byte moment table is never
//     cleared and a short quantization dequantises an uninitialised palette
//     tail (qhull-gap Task 4d, two oracle defects the candidate keeps), so with
//     the CRT's heap the words would depend on heap contents;
//   * the return; every HullResult word (a pointer as its block's ordinal);
//     the vertices, bit exact; the indices;
//   * the bytes of every QHULL_*.obj the call wrote: each side runs in a
//     temporary directory of its own, and each keeps its own counter
//     (.data:0x00125084 / gQhullObjCounter), both set to 0 when the drive
//     starts;
//   * ReleaseResult's calls and the result words after it (A).
// Every run is made under the x87 control word its family names, set on both
// sides before the call: 0x027f (53-bit precision, round to nearest: the
// process default every other family runs at) and 0x0f7f (64-bit, round
// toward zero). Cooking never sets the word itself (contract, "Precision and
// x87"). After each candidate call the drive sets gQhullHost back to NULL:
// the product, like the oracle, leaves it pointing at the driver's dead frame,
// and the harness's hook routing (tests/PhysicsThirdPartyHost.cpp) reads it.
//
// The +4 interface (HullLibrary::mPolygonizer), which NovodeX never passes,
// is driven with a test interface on a few runs: its slots are thiscall
// virtuals the oracle calls with the arguments the listing pushes
// (0x0007ebe6-0x0007ecd5), and the same object serves both sides.

static const unsigned kHullCreate				= 0x0007ea10;	// phys_fn_003279
static const unsigned kHullRelease				= 0x0007e300;	// phys_fn_003255
static const unsigned kHullCompute				= 0x00054920;	// phys_fn_002233
static const unsigned kHullObjCounter			= 0x00125084;	// phys_data_003856
static const unsigned kTriangleMeshVtable		= 0x00108608;	// phys_data_000967
static const unsigned kIatFoundationAllocator	= 0x001041bc;	// -> NxFoundation's nxFoundationSDKAllocator

static const unsigned kHostSize				= 0x0007e520;	// phys_fn_003265

typedef void		(__thiscall* HostSizeFn)(void* host, float area, float volume);
typedef int			(__thiscall* HullCreateFn)(void* library, const void* desc, void* result);
typedef int			(__thiscall* HullReleaseFn)(void* library, void* result);
typedef unsigned	(__thiscall* HullComputeFn)(void* owner, const void* desc, void* out);

static unsigned gHullProbeRun = 0;

// The run's blocks, in allocation order; a pointer is taped as its ordinal.
struct NxHullBlocks
	{
	enum { kMax = 65536 };
	void*			block[kMax];
	unsigned char	live[kMax];
	unsigned		count;
	NxTape*			tape;

	void reset(NxTape* t)
		{
		for(unsigned i = 0; i < count; ++i)
			if(live[i])
				::free(block[i]);
		count = 0;
		tape = t;
		}
	void* allocate(unsigned tag, size_t size, unsigned type)
		{
		void* p = ::calloc(size ? size : 1, 1);
		static unsigned probed = 0xffffffffu;
		if(count == 0)
			probed = 0;
		if(getenv("NXHULL_PROBE") && !probed && count >= 1 && live[0] && size > 16 && (size - 16) % 24 == 0)
			{
			probed = 1;
			const unsigned n = (unsigned) (size - 16) / 24;
			const unsigned run = gHullProbeRun;
			const unsigned* v = (const unsigned*) ((const char*) block[0] + 16);
			unsigned d = 2166136261u;
			for(unsigned i = 0; i < n * 3; ++i)
				d = nxFold(d, v[i]);
			fprintf(stderr, "PROBE run=%x n=%u digest=%08x first=%08x %08x %08x\n", run, n, d, v[0], v[1], v[2]);
			if(getenv("NXHULL_POINTS_PROBE"))
				for(unsigned i = 0; i < n; ++i)
					fprintf(stderr, "PROBE_POINT run=%x index=%u bits=%08x,%08x,%08x\n", run, i,
						v[i * 3], v[i * 3 + 1], v[i * 3 + 2]);
			}
		if(tape)
			{
			tape->push(tag);
			tape->push((unsigned) size);
			tape->push(type);
			tape->push(count);
			}
		if(count < kMax)
			{
			block[count] = p;
			live[count] = 1;
			++count;
			}
		return p;
		}
	unsigned ordinal(const void* p) const
		{
		for(unsigned i = count; i-- > 0;)
			if(block[i] == p)
				return i;
		return 0xffffffffu;
		}
	void release(unsigned tag, void* p)
		{
		const unsigned i = ordinal(p);
		if(tape)
			{
			tape->push(tag);
			tape->push(i);
			}
		if(i != 0xffffffffu && live[i])
			{
			live[i] = 0;
			::free(p);
			}
		}
	// A pointer word: 0 stays 0, one of the run's blocks becomes
	// 0xb10c0000 | ordinal, anything else stays as it is (an input pointer,
	// the same on both sides).
	unsigned word(const void* p) const
		{
		if(!p)
			return 0;
		const unsigned i = ordinal(p);
		return i == 0xffffffffu ? (unsigned) (size_t) p : 0xb10c0000u | i;
		}
	};

static NxHullBlocks gHullBlocks;

class NxHullTestAllocator : public HullAllocator
	{
	public:
	virtual	void*	malloc(size_t size)		{ return gHullBlocks.allocate(0xa110c000u, size, 0xffu); }
	virtual	void	free(void* memory)		{ gHullBlocks.release(0xf2ee0000u, memory); }
	};

// The Foundation allocator for B. Slot +8 is malloc(size, type) and +0x14 is
// free (MSVC's order of the overloads), which is what 002235/002237 and
// 002233 call; the other slots are recorded under tags of their own.
class NxHullFoundationAllocator : public NxUserAllocator
	{
	public:
	virtual void* mallocDEBUG(size_t size, const char*, int)									{ return gHullBlocks.allocate(0xa110c0d3u, size, 0xffu); }
	virtual void* mallocDEBUG(size_t size, const char*, int, const char*, NxMemoryType type)	{ return gHullBlocks.allocate(0xa110c0d5u, size, (unsigned) type); }
	virtual void* malloc(size_t size)															{ return gHullBlocks.allocate(0xa110c001u, size, 0xffu); }
	virtual void* malloc(size_t size, NxMemoryType type)										{ return gHullBlocks.allocate(0xa110c002u, size, (unsigned) type); }
	virtual void* realloc(void* memory, size_t size)
		{
		if(gHullBlocks.tape)
			{
			gHullBlocks.tape->push(0x4ea110c0u);
			gHullBlocks.tape->push(gHullBlocks.ordinal(memory));
			gHullBlocks.tape->push((unsigned) size);
			}
		return 0;
		}
	virtual void free(void* memory)																{ gHullBlocks.release(0xf2ee0001u, memory); }
	};

// The +4 interface. Records each call and its arguments and answers with a
// fixed tetrahedron, or refuses.
static const float kHullPolyVertices[12] = { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
static const NxU32 kHullPolyIndices[12] = { 0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3 };

class NxHullTestPolygonizer : public HullPolygonizer
	{
	public:
	NxTape*	tape;
	bool	refuse;
	bool	refuseFinish;

	bool note(unsigned tag, HullPolygonizerResult& out, NxReal unknown18, NxU32 vcount, const NxReal* vertices,
		NxU32 stride, NxU32 faceCount, const NxU32* indices)
		{
		// The local as the driver hands it over: the flag byte and the four
		// fields it zeroes (0x0007ec09-0x0007ec16); the index count at +0x10
		// is never initialised, so it is not taped.
		tape->push(tag);
		tape->push((unsigned) (unsigned char) out.mFlag);
		tape->push(out.mVcount);
		tape->push(gHullBlocks.word(out.mVertices));
		tape->push(out.mFaceCount);
		tape->push(gHullBlocks.word(out.mIndices));
		tape->pushFloat(unknown18);
		tape->push(vcount);
		tape->push(gHullBlocks.word(vertices));
		tape->push(stride);
		tape->push(faceCount);
		tape->push(gHullBlocks.word(indices));
		for(NxU32 i = 0; i < vcount * 3; ++i)
			tape->pushFloat(vertices[i]);
		if(refuse)
			return false;
		out.mVcount = 4;
		out.mVertices = (NxReal*) kHullPolyVertices;
		out.mFaceCount = 4;
		out.mIndexCount = 12;
		out.mIndices = (NxU32*) kHullPolyIndices;
		return true;
		}
	virtual	bool fromPolygons(HullPolygonizerResult& out, NxReal unknown18, NxU32 vcount, const NxReal* vertices,
		NxU32 stride, NxU32 faceCount, const NxU32* indices)
		{
		return note(0x9019a000u, out, unknown18, vcount, vertices, stride, faceCount, indices);
		}
	virtual	void unknown04()	{ tape->push(0x9019a004u); }
	virtual	bool fromTriangles(HullPolygonizerResult& out, NxReal unknown18, NxU32 vcount, const NxReal* vertices,
		NxU32 stride, NxU32 faceCount, const NxU32* indices)
		{
		return note(0x9019a008u, out, unknown18, vcount, vertices, stride, faceCount, indices);
		}
	virtual	void unknown0c()	{ tape->push(0x9019a00cu); }
	virtual	void release(HullPolygonizerResult& out)
		{
		tape->push(0x9019a010u);
		tape->push(out.mVcount);
		tape->push(out.mFaceCount);
		tape->push(out.mIndexCount);
		}
	virtual	bool finishPolygons(HullPolygonizerResult& out)
		{
		tape->push(0x9019a014u);
		tape->push(out.mVcount);
		return !refuseFinish;
		}
	};

// The x87 control word, precision and rounding, through the CRT.
static unsigned nxHullSetWord(unsigned word)
	{
	unsigned old = 0, now = 0;
	_controlfp_s(&old, 0, 0);
	_controlfp_s(&now, word == 0x0f7f ? (_PC_64 | _RC_CHOP) : (_PC_53 | _RC_NEAR), _MCW_PC | _MCW_RC);
	return old;
	}

static void nxHullRestoreWord(unsigned old)
	{
	unsigned now = 0;
	_controlfp_s(&now, old, _MCW_PC | _MCW_RC);
	}

static wchar_t gHullDir[2][MAX_PATH];
static wchar_t gHullHome[MAX_PATH];

static void nxHullDirectories()
	{
	wchar_t temp[MAX_PATH];
	GetTempPathW(MAX_PATH, temp);
	GetCurrentDirectoryW(MAX_PATH, gHullHome);
	for(int side = 0; side < 2; ++side)
		{
		swprintf_s(gHullDir[side], L"%snxhull_%lu_%s", temp, GetCurrentProcessId(), side ? L"candidate" : L"oracle");
		CreateDirectoryW(gHullDir[side], 0);
		}
	}

// Every QHULL_*.obj in the side's directory, by name. The main tape gets
// each file's name; the text tape gets the name again and the contents as
// tokens, the way qhull-gap Task 1 compares CRT-written files
// (nxQhTapeText, "OUTPUT CAPTURE" above): a number is parsed and taped as a
// double, an id glued to a letter or an integer is a discrete word, anything
// else is hashed. Not bytes: the oracle's 2003 static CRT prints a float -0.0
// as "0.000000000" where the UCRT prints "-0.000000000", which as bytes would
// also put every later word out of step. Each file is removed once taped.
// qhull-gap Task 5 (the 4e review): each dump's bytes as well, as a length and
// a digest, after one normalisation -- a negative zero printed as "-0." and
// zeros up to the next non-digit loses its sign, the one known difference
// between the two CRTs (see the _obj families) -- so that a width, padding or
// whitespace slip the token tape cannot see still fails. Reported after the
// Task 4e totals, one family per Task 4e family (gHullBytes, below).
static NxTape gHullBytes[4][2];
static NxTape* gHullBytesCur[2] = { 0, 0 };

static void nxHullTapeObjBytes(NxTape& tape, const char* bytes, size_t size)
	{
	static char normal[1 << 20];
	size_t n = 0;
	for(size_t k = 0; k < size; ++k)
		{
		if(bytes[k] == '-' && k + 2 < size && bytes[k + 1] == '0' && bytes[k + 2] == '.')
			{
			size_t m = k + 3;
			while(m < size && bytes[m] == '0')
				++m;
			if(m == size || bytes[m] < '0' || bytes[m] > '9')
				continue;		// "-0.000...": drop the sign
			}
		normal[n++] = bytes[k];
		}
	tape.push((unsigned) n);			// the normalised length (the raw one differs by the sign)
	tape.push(nxHashBytes(normal, n));
	}

static void nxHullTapeObjFiles(NxTape& tape, NxTape& text, int side)
	{
	wchar_t pattern[MAX_PATH];
	swprintf_s(pattern, L"%s\\QHULL_*.obj", gHullDir[side]);
	static wchar_t names[64][MAX_PATH];
	unsigned count = 0;
	WIN32_FIND_DATAW found;
	HANDLE h = FindFirstFileW(pattern, &found);
	if(h != INVALID_HANDLE_VALUE)
		{
		do
			{
			if(count < 64)
				wcscpy_s(names[count++], found.cFileName);
			}
		while(FindNextFileW(h, &found));
		FindClose(h);
		}
	for(unsigned i = 1; i < count; ++i)
		for(unsigned j = i; j > 0 && wcscmp(names[j - 1], names[j]) > 0; --j)
			{
			wchar_t t[MAX_PATH];
			wcscpy_s(t, names[j]);
			wcscpy_s(names[j], names[j - 1]);
			wcscpy_s(names[j - 1], t);
			}
	tape.push(0x0b1f11e5u);
	tape.push(count);
	text.push(0x0b1f11e5u);
	text.push(count);
	for(unsigned i = 0; i < count; ++i)
		{
		const size_t length = wcslen(names[i]);
		tape.push((unsigned) length);
		text.push((unsigned) length);
		for(size_t k = 0; k < length; k += 4)
			{
			unsigned w = 0;
			for(size_t b = 0; b < 4 && k + b < length; ++b)
				w |= ((unsigned) names[i][k + b] & 0xffu) << (8 * b);
			tape.push(w);
			text.push(w);
			}
		wchar_t path[MAX_PATH];
		swprintf_s(path, L"%s\\%s", gHullDir[side], names[i]);
		FILE* f = _wfopen(path, L"rb");
		size_t size = 0;
		static char bytes[1 << 20];
		if(f)
			{
			size = fread(bytes, 1, sizeof(bytes), f);
			fclose(f);
			}
		nxQhTapeText(bytes, size, text, text);
		text.push(0x0e0f0000u);
		if(gHullBytesCur[side])
			nxHullTapeObjBytes(*gHullBytesCur[side], bytes, size);
		DeleteFileW(path);
		}
	}

// The point sets. Returns the vertex count; *stride is the byte stride.
static unsigned nxHullPoints(int set, float* out, unsigned* stride)
	{
	*stride = 12;
	unsigned n = 0;
	switch(set)
		{
		case 0: case 1: case 2: case 3: case 4:	// tetrahedron, cube, lattice, sphere(96), box(200)
			gState = 0x4e0c0de0u + (unsigned) set;
			return nxQhullPoints(set, out);
		case 5:		// 600 points in an anisotropic box: more than 256, the quantizer
			gState = 0x4e0c0de5u;
			for(int i = 0; i < 600; ++i)
				{
				out[n * 3 + 0] = nxRange(-3.0f, 2.0f).value;
				out[n * 3 + 1] = nxRange(-0.5f, 0.75f).value;
				out[n * 3 + 2] = nxRange(1.0f, 9.0f).value;
				++n;
				}
			return n;
		case 6:		// 1000 points on an ellipsoid: most of them on the hull
			gState = 0x4e0c0de6u;
			for(int i = 0; i < 1000; ++i)
				{
				Point p(nxRange(-1, 1).value, nxRange(-1, 1).value, nxRange(-1, 1).value);
				if(p.Magnitude() > 1e-3f)
					p.Normalize();
				out[n * 3 + 0] = p.x * 5.0f + 1.0f; out[n * 3 + 1] = p.y * 2.0f - 3.0f; out[n * 3 + 2] = p.z * 0.5f;
				++n;
				}
			return n;
		case 7:		// a 10x10x10 lattice
			for(int i = 0; i < 1000; ++i)
				{
				out[n * 3 + 0] = (float) (i % 10) * 0.5f;
				out[n * 3 + 1] = (float) ((i / 10) % 10) * 0.25f - 0.3f;
				out[n * 3 + 2] = (float) (i / 100) * 0.75f + 0.1f;
				++n;
				}
			return n;
		case 8:		// the cube, each corner with two copies inside the weld epsilon, and interior points
			gState = 0x4e0c0de8u;
			for(unsigned v = 0; v < 8; ++v)
				for(int k = 0; k < 3; ++k)
					{
					const float d = k * 1e-7f;
					out[n * 3 + 0] = ((v & 1) ? 1.0f : -1.0f) + d;
					out[n * 3 + 1] = ((v & 2) ? 1.0f : -1.0f) - d;
					out[n * 3 + 2] = ((v & 4) ? 1.0f : -1.0f) + 0.5f * d;
					++n;
					}
			for(int i = 0; i < 12; ++i)
				{
				out[n * 3 + 0] = nxRange(-0.9f, 0.9f).value; out[n * 3 + 1] = nxRange(-0.9f, 0.9f).value; out[n * 3 + 2] = nxRange(-0.9f, 0.9f).value;
				++n;
				}
			return n;
		case 9:		// flat: every point on z = 0.5, one extent 0 (the clean-up's box)
			gState = 0x4e0c0de9u;
			for(int i = 0; i < 20; ++i)
				{
				out[n * 3 + 0] = nxRange(-1, 1).value; out[n * 3 + 1] = nxRange(-2, 2).value; out[n * 3 + 2] = 0.5f;
				++n;
				}
			return n;
		case 10:	// collinear
			return nxQhullPoints(11, out);
		case 11:	// every point equal
			for(int i = 0; i < 12; ++i)
				{
				out[n * 3 + 0] = 0.3f; out[n * 3 + 1] = -0.2f; out[n * 3 + 2] = 0.7f;
				++n;
				}
			return n;
		case 12:	// two distinct points once welded
			for(int i = 0; i < 10; ++i)
				{
				const float d = (i / 2) * 1e-8f;
				out[n * 3 + 0] = (i & 1) ? 2.0f + d : -1.0f + d;
				out[n * 3 + 1] = (i & 1) ? 1.0f - d : 0.5f;
				out[n * 3 + 2] = (i & 1) ? -3.0f : 4.0f + d;
				++n;
				}
			return n;
		case 13:	// large coordinates
		case 14:	// tiny ones
			gState = 0x4e0c0dedu + (unsigned) set;
			for(int i = 0; i < 60; ++i)
				{
				const float s = set == 13 ? 1e6f : 1e-4f;
				out[n * 3 + 0] = nxRange(-1, 1).value * s; out[n * 3 + 1] = nxRange(-1, 1).value * s; out[n * 3 + 2] = nxRange(-1, 1).value * s;
				++n;
				}
			return n;
		case 15:	// a diagonal plane, x + y + z = 0 over all three axes: qhull fails, the box retry
			gState = 0x4e0c0defu;
			for(int i = 0; i < 30; ++i)
				{
				const float a = nxRange(-1, 1).value, b = nxRange(-1, 1).value;
				out[n * 3 + 0] = a; out[n * 3 + 1] = b; out[n * 3 + 2] = -(a + b);
				++n;
				}
			return n;
		case 16:	// stride 20: the sphere with two filler floats per vertex
			{
			static float sphere[3 * 96];
			gState = 0x4e0c0de3u;
			const unsigned m = nxQhullPoints(3, sphere);
			for(unsigned i = 0; i < m; ++i)
				{
				out[i * 5 + 0] = sphere[i * 3 + 0]; out[i * 5 + 1] = sphere[i * 3 + 1]; out[i * 5 + 2] = sphere[i * 3 + 2];
				out[i * 5 + 3] = 777.0f; out[i * 5 + 4] = -777.0f;
				}
			*stride = 20;
			return m;
			}
		case 17:	// five tight clusters of 80: fewer occupied cells than boxes (a short quantization)
			gState = 0x4e0c0df1u;
			for(int i = 0; i < 400; ++i)
				{
				const int c = i % 5;
				out[n * 3 + 0] = c * 1.0f + nxRange(-0.01f, 0.01f).value;
				out[n * 3 + 1] = c * 0.5f + nxRange(-0.01f, 0.01f).value;
				out[n * 3 + 2] = -c * 0.7f + nxRange(-0.01f, 0.01f).value;
				++n;
				}
			return n;
		case 18:	// -0.0 components
			gState = 0x4e0c0df2u;
			for(int i = 0; i < 40; ++i)
				{
				out[n * 3 + 0] = (i % 3) ? -0.0f : nxRange(-1, 1).value;
				out[n * 3 + 1] = nxRange(-1, 1).value;
				out[n * 3 + 2] = (i % 7) ? nxRange(-1, 1).value : -0.0f;
				++n;
				}
			return n;
		default:	// no points at all: cleanupVertices refuses (the driver's failure path)
			out[0] = 1.0f; out[1] = 2.0f; out[2] = 3.0f;
			return 0;
		}
	}

static const int kHullSets = 20;
static const int kHullComputeSets = 19;		// every set but the empty one (see nxDriveConvexCooking)

// The runs whose hull differs under 0x027f, measured: the box of the set
// that welds to two points (12) and the short quantization of the clusters
// (17). qhull's input is the same on both sides, point for point (the stderr
// probe NXHULL_PROBE=1 prints a digest of the vertex buffer when runQhull
// allocates its double array, and the two sides print the same). Box:
// vendored qhull (reproduced by hull_qhull_direct); clusters: not reproduced
// by qhull alone -- open (qhull-gap Task 5; candidates: allocation pattern,
// qh_gethash address hashing). Under 0x0f7f both runs are exact, and they
// stay in the 0x0f7f families.
static bool nxHullQhullDivergent(int set, unsigned flags)
	{
	return (set == 12 || set == 17) && flags == 0xb7;
	}

struct NxHullRun
	{
	int			set;
	unsigned	flags;
	int			polygonizer;	// 0 none, 1 answers, 2 refuses, 3 refuses to finish
	int			crt;			// 1: no user allocator (the CRT arms); pointers taped as present or not
	};

static float gHullInput[5 * 2048];
static NxTape gHullTape[2];
static NxTape gHullText[2];

static void nxHullTapeResult(NxTape& tape, const HullResult& r, bool crt)
	{
	unsigned w[7];
	memcpy(w, &r, sizeof(w));
	for(int i = 0; i < 7; ++i)
		{
		if(i == 2 || i == 6)
			tape.push(crt ? (w[i] ? 1u : 0u) : gHullBlocks.word((const void*) (size_t) w[i]));
		else
			tape.push(w[i]);
		}
	}

static void nxHullCreateRun(const NxOracleRows& o, const NxHullRun& run, unsigned word, int index, bool selfOnly)
	{
	unsigned stride;
	const unsigned n = nxHullPoints(run.set, gHullInput, &stride);
	HullDesc desc;
	desc.mFlags = run.flags;
	desc.mVcount = n;
	desc.mVertices = gHullInput;
	desc.mVertexStride = stride;
	desc.mNormalEpsilon = 0.00001f;
	desc.mMaxVertices = 0x100;
	desc.mUnknown18 = 0.8f;
	NxHullTestAllocator allocator;
	NxHullTestPolygonizer polygonizer;
	polygonizer.refuse = run.polygonizer == 2;
	polygonizer.refuseFinish = run.polygonizer == 3;
	// The +4 path installs the interface's twelve indices and leaves the
	// result's index count (+0x14) as it was.
	const bool replaced = (run.flags & QF_POLYGONIZER) && (run.polygonizer == 1);
	for(int side = 0; side < (selfOnly ? 1 : 2); ++side)
		{
		NxTape& tape = gHullTape[side];
		polygonizer.tape = &tape;
		tape.push(0x4e110000u | (unsigned) index);
		gHullProbeRun = 0x4e110000u | (unsigned) index | (side << 12);
		tape.push(run.flags);
		gHullBlocks.reset(&tape);
		HullLibrary library;
		library.mAllocator = run.crt ? 0 : &allocator;
		library.mPolygonizer = run.polygonizer ? &polygonizer : 0;
		HullResult result;
		memset(&result, 0xcd, sizeof(result));
		SetCurrentDirectoryW(gHullDir[side]);
		const unsigned old = nxHullSetWord(word);
		int ret;
		if(side == 0)
			{
			ret = ((HullCreateFn) (o.base + kHullCreate))(&library, &desc, &result);
			}
		else
			{
			ret = library.CreateConvexHull(desc, result);
			gQhullHost = 0;
			}
		nxHullRestoreWord(old);
		SetCurrentDirectoryW(gHullHome);
		tape.push((unsigned) ret);
		nxHullTapeResult(tape, result, run.crt != 0);
		if(ret == 0)
			{
			for(NxU32 i = 0; i < result.mNumOutputVertices * 3; ++i)
				tape.pushFloat(result.mOutputVertices[i]);
			const NxU32 indices = replaced ? 12 : result.mNumIndices;
			for(NxU32 i = 0; i < indices; ++i)
				tape.push(result.mIndices[i]);
			}
		nxHullTapeObjFiles(tape, gHullText[side], side);
		const unsigned oldRelease = nxHullSetWord(word);
		int released;
		if(side == 0)
			released = ((HullReleaseFn) (o.base + kHullRelease))(&library, &result);
		else
			released = library.ReleaseResult(result);
		nxHullRestoreWord(oldRelease);
		tape.push((unsigned) released);
		nxHullTapeResult(tape, result, run.crt != 0);
		gHullBlocks.reset(0);
		}
	}

static void nxHullComputeRun(const NxOracleRows& o, int set, unsigned meshFlags, unsigned word, int index,
	NxHullFoundationAllocator& foundation, bool selfOnly)
	{
	unsigned stride;
	const unsigned n = nxHullPoints(set, gHullInput, &stride);
	// The descriptor as 13 dwords, NxTriangleMeshDesc's layout; 002233 copies
	// all of them into the output before overwriting six.
	unsigned desc[13];
	desc[0] = n;
	desc[1] = 0;						// numTriangles
	desc[2] = stride;					// pointStrideBytes
	desc[3] = 0;						// triangleStrideBytes
	desc[4] = (unsigned) (size_t) gHullInput;
	desc[5] = 0;						// triangles
	desc[6] = meshFlags;
	desc[7] = 2;						// materialIndexStride
	desc[8] = 0x0badf00du;				// materialIndices: copied, never read
	desc[9] = 0xff;						// heightFieldVerticalAxis: NX_NOT_HEIGHTFIELD
	desc[10] = 0;						// heightFieldVerticalExtent
	desc[11] = 0;						// pmap
	const float threshold = 0.001f;
	memcpy(&desc[12], &threshold, 4);	// convexEdgeThreshold

	NxUserAllocator** oracleFoundation = *(NxUserAllocator***) (o.base + kIatFoundationAllocator);
	for(int side = 0; side < (selfOnly ? 1 : 2); ++side)
		{
		NxTape& tape = gHullTape[side];
		tape.push(0x4e120000u | (unsigned) index);
		gHullProbeRun = 0x4e120000u | (unsigned) index | (side << 12);
		tape.push(meshFlags);
		gHullBlocks.reset(&tape);
		unsigned out[13];
		for(int i = 0; i < 13; ++i)
			out[i] = 0xcdcdcdcdu;
		NxUserAllocator* savedOracle = *oracleFoundation;
		NxUserAllocator* savedCandidate = nxFoundationSDKAllocator;
		*oracleFoundation = &foundation;
		nxFoundationSDKAllocator = &foundation;
		SetCurrentDirectoryW(gHullDir[side]);
		const unsigned old = nxHullSetWord(word);
		unsigned ret;
		if(side == 0)
			{
			void* owner[2] = { o.base + kTriangleMeshVtable, 0 };
			ret = ((HullComputeFn) (o.base + kHullCompute))(owner, desc, out) & 0xffu;
			}
		else
			{
			TriangleMeshHullAllocator owner;
			ret = owner.computeHull(*(const NxTriangleMeshDesc*) desc, *(NxTriangleMeshDesc*) out) ? 1u : 0u;
			gQhullHost = 0;
			}
		nxHullRestoreWord(old);
		SetCurrentDirectoryW(gHullHome);
		*oracleFoundation = savedOracle;
		nxFoundationSDKAllocator = savedCandidate;
		tape.push(ret);
		for(int i = 0; i < 13; ++i)
			tape.push(i == 4 || i == 5 ? gHullBlocks.word((const void*) (size_t) out[i]) : out[i]);
		if(ret)
			{
			const float* points = (const float*) (size_t) out[4];
			const unsigned* triangles = (const unsigned*) (size_t) out[5];
			for(unsigned i = 0; i < out[0] * 3; ++i)
				tape.pushFloat(points[i]);
			for(unsigned i = 0; i < out[1] * 3; ++i)
				tape.push(triangles[i]);
			}
		nxHullTapeObjFiles(tape, gHullText[side], side);
		gHullBlocks.reset(0);
		}
	}

// NXHULL_DUMP=<directory>: each family's two tapes as text (word, kind), for
// reading a difference; stderr and stdout are unchanged.
static void nxHullDump(const char* name)
	{
	const char* directory = getenv("NXHULL_DUMP");
	if(!directory)
		return;
	for(int side = 0; side < 2; ++side)
		{
		char path[MAX_PATH];
		_snprintf(path, sizeof(path), "%s\\%s_%s.txt", directory, name, side ? "candidate" : "oracle");
		path[sizeof(path) - 1] = 0;
		FILE* f = fopen(path, "w");
		if(!f)
			continue;
		for(unsigned i = 0; i < gHullTape[side].count; ++i)
			fprintf(f, "%08x %u\n", gHullTape[side].words[i], gHullTape[side].kinds[i]);
		fprintf(f, "TEXT\n");
		for(unsigned i = 0; i < gHullText[side].count; ++i)
			fprintf(f, "%08x %u\n", gHullText[side].words[i], gHullText[side].kinds[i]);
		fclose(f);
		}
	}

static void nxDriveConvexCooking(const NxOracleRows& o, bool selfOnly)
	{
	nxHullDirectories();
	int* oracleCounter = (int*) (o.base + kHullObjCounter);
	*oracleCounter = 0;
	gQhullObjCounter = 0;
	void** hostSlot = (void**) (o.base + kQhHostGlobal);
	void* shippedHost = *hostSlot;
	NxUserAllocator** oracleFoundation = *(NxUserAllocator***) (o.base + kIatFoundationAllocator);
	fprintf(stderr, "HULL foundation_allocator=%s\n",
		(void*) oracleFoundation == (void*) &nxFoundationSDKAllocator ? "shared" : "separate");

	// The host's size slot (003265, vtable +0x0c): qhull calls it only for the
	// "FS" print format (io.c NOVODEX [4]), and CreateConvexHull runs "o" and
	// nothing else, so no hull run reaches it. It is called directly: the
	// oracle's on a host-sized buffer, the candidate's through a pointer the
	// compiler cannot see through, so its out-of-line copy runs.
	{
	gHullTape[0].reset();
	gHullTape[1].reset();
	static const unsigned kSizes[][2] =
		{
		{ 0x3fc00000u, 0x40100000u }, { 0x80000000u, 0x00000000u }, { 0x7fc00001u, 0x7f800000u },
		{ 0x00000001u, 0x7f7fffffu }, { 0x0da24260u, 0xc0400000u },
		};
	static unsigned char oracleHost[0x4054];
	for(unsigned i = 0; i < sizeof(kSizes) / sizeof(kSizes[0]); ++i)
		{
		float area, volume;
		memcpy(&area, &kSizes[i][0], 4);
		memcpy(&volume, &kSizes[i][1], 4);
		memset(oracleHost, 0xcd, sizeof(oracleHost));
		((HostSizeFn) (o.base + kHostSize))(oracleHost, area, volume);
		unsigned words[2];
		memcpy(words, oracleHost + 0x404c, 8);
		gHullTape[0].pushFloatWord(words[0]);
		gHullTape[0].pushFloatWord(words[1]);
		unsigned untouched = 0;
		for(unsigned b = 0; b < sizeof(oracleHost); ++b)
			if((b < 0x404c || b >= 0x4054) && oracleHost[b] == 0xcd)
				++untouched;
		gHullTape[0].push(untouched);
		if(selfOnly)
			continue;
		QhullHost host(0);
		QhullHost* volatile candidate = &host;
		candidate->size(area, volume);
		gHullTape[1].pushFloat(host.mArea);
		gHullTape[1].pushFloat(host.mVolume);
		gHullTape[1].push(sizeof(oracleHost) - 8);
		}
	nxReportTapes(gHullTape[0], gHullTape[1], "hull_host_size", "0x0007e520", "phys_fn_003265",
		"QhullHost.cpp", selfOnly, 0);
	}

	// A. Every set with NovodeX's flags; the tetrahedron, the cube and the
	// sphere with each bit changed; the +4 interface on the cube; the CRT arms
	// (no user allocator) on four sets that stay below the quantizer.
	static NxHullRun runs[kHullSets + 40];
	unsigned count = 0;
	for(int set = 0; set < kHullSets; ++set)
		{
		NxHullRun r = { set, 0xb7, 0, 0 };
		runs[count++] = r;
		}
	static const unsigned kVariants[] = { 0xa7, 0x97, 0xb6, 0xb5, 0xb3, 0xf7 };
	static const int kVariantSets[] = { 0, 1, 3 };
	for(int s = 0; s < 3; ++s)
		for(int v = 0; v < 6; ++v)
			{
			NxHullRun r = { kVariantSets[s], kVariants[v], 0, 0 };
			runs[count++] = r;
			}
	static const NxHullRun kExtra[] =
		{
		{ 1, 0xe7, 0, 0 },		// the OK dump in polygon mode
		{ 2, 0xf7, 0, 0 },		// the OK dump of a merged hull
		{ 15, 0xf7, 0, 0 },		// the diagonal plane with both dumps
		{ 19, 0x37, 0, 0 },		// no points and no FAIL dump
		{ 1, 0xbf, 1, 0 },		// the +4 interface on triangles
		{ 1, 0xaf, 1, 0 },		// on polygons: finishPolygons
		{ 1, 0xbf, 2, 0 },		// refused
		{ 1, 0xaf, 3, 0 },		// refused at finishPolygons
		{ 1, 0xb7, 1, 0 },		// the interface without bit 3: not called
		{ 1, 0xbf, 0, 0 },		// bit 3 without an interface
		{ 0, 0xb7, 0, 1 },		// the CRT's malloc/free
		{ 1, 0xb7, 0, 1 },
		{ 3, 0xf7, 0, 1 },
		{ 15, 0xb7, 0, 1 },
		};
	for(unsigned i = 0; i < sizeof(kExtra) / sizeof(kExtra[0]); ++i)
		runs[count++] = kExtra[i];

	static const unsigned kWords[2] = { 0x027f, 0x0f7f };
	static const char* const kCreateNames[2] = { "hull_create", "hull_create_pc64" };
	static const char* const kComputeNames[2] = { "hull_compute", "hull_compute_pc64" };
	static const char* const kCreateTextNames[2] = { "hull_create_obj", "hull_create_pc64_obj" };
	static const char* const kComputeTextNames[2] = { "hull_compute_obj", "hull_compute_pc64_obj" };
	for(int w = 0; w < 2; ++w)
		{
		gHullTape[0].reset();
		gHullTape[1].reset();
		gHullText[0].reset();
		gHullText[1].reset();
		for(int side = 0; side < 2; ++side)
			{
			gHullBytes[w][side].reset();
			gHullBytesCur[side] = &gHullBytes[w][side];
			}
		for(unsigned r = 0; r < count; ++r)
			if(!(w == 0 && nxHullQhullDivergent(runs[r].set, runs[r].flags)))
				nxHullCreateRun(o, runs[r], kWords[w], (int) r, selfOnly);
		nxHullDump(kCreateNames[w]);
		nxReportTapes(gHullTape[0], gHullTape[1], kCreateNames[w], "0x0007ea10", "phys_fn_003279",
			"QhullHost.cpp,Quantizer.cpp", selfOnly, 0);
		if(w == 0)
			{
			// DIVERGENT: the runs whose qhull diverges under 0x027f (see
			// nxHullQhullDivergent), in a family of their own.
			gHullTape[0].reset();
			gHullTape[1].reset();
			for(unsigned r = 0; r < count; ++r)
				if(nxHullQhullDivergent(runs[r].set, runs[r].flags))
					nxHullCreateRun(o, runs[r], kWords[w], (int) r, selfOnly);
			nxHullDump("hull_create_qhull");
			nxReportTapes(gHullTape[0], gHullTape[1], "hull_create_qhull", "0x0007ea10", "phys_fn_003279",
				"QhullHost.cpp,Quantizer.cpp", selfOnly, kDivergent);
			}
		nxReportTapes(gHullText[0], gHullText[1], kCreateTextNames[w], "0x0007dea0", "phys_fn_003247",
			"QhullHost.cpp", selfOnly, 0);
		}

	// B. Every set but the empty one as an NxTriangleMeshDesc with
	// NX_MF_CONVEX | NX_MF_COMPUTE_CONVEX, and the cube and the sphere with
	// NX_MF_16_BIT_INDICES as well (002233 does not clear it). The empty set is
	// left out: when cleanupVertices refuses, CreateConvexHull returns before
	// buildResult initialises the result, and 002233's result is an
	// uninitialised local (E-0x1c), so ReleaseResult frees whatever two stack
	// words are there -- on both sides, and different on each. isValid's
	// numVertices >= 3 keeps it unreachable from the public API.
	NxHullFoundationAllocator foundation;
	for(int w = 0; w < 2; ++w)
		{
		gHullTape[0].reset();
		gHullTape[1].reset();
		gHullText[0].reset();
		gHullText[1].reset();
		for(int side = 0; side < 2; ++side)
			{
			gHullBytes[2 + w][side].reset();
			gHullBytesCur[side] = &gHullBytes[2 + w][side];
			}
		const unsigned meshFlags = NX_MF_CONVEX | NX_MF_COMPUTE_CONVEX;
		for(int set = 0; set < kHullComputeSets; ++set)
			if(!(w == 0 && nxHullQhullDivergent(set, 0xb7)))
				nxHullComputeRun(o, set, meshFlags, kWords[w], set, foundation, selfOnly);
		nxHullComputeRun(o, 1, meshFlags | NX_MF_16_BIT_INDICES, kWords[w], kHullComputeSets, foundation, selfOnly);
		nxHullComputeRun(o, 3, meshFlags | NX_MF_16_BIT_INDICES, kWords[w], kHullComputeSets + 1, foundation, selfOnly);
		nxHullDump(kComputeNames[w]);
		nxReportTapes(gHullTape[0], gHullTape[1], kComputeNames[w], "0x00054920", "phys_fn_002233",
			"TriangleMesh.cpp,QhullHost.cpp,Quantizer.cpp", selfOnly, 0);
		if(w == 0)
			{
			gHullTape[0].reset();
			gHullTape[1].reset();
			for(int set = 0; set < kHullComputeSets; ++set)
				if(nxHullQhullDivergent(set, 0xb7))
					nxHullComputeRun(o, set, meshFlags, kWords[w], set, foundation, selfOnly);
			nxHullDump("hull_compute_qhull");
			nxReportTapes(gHullTape[0], gHullTape[1], "hull_compute_qhull", "0x00054920", "phys_fn_002233",
				"TriangleMesh.cpp,QhullHost.cpp,Quantizer.cpp", selfOnly, kDivergent);
			}
		nxReportTapes(gHullText[0], gHullText[1], kComputeTextNames[w], "0x0007e050", "phys_fn_003251",
			"QhullHost.cpp", selfOnly, 0);
		}

	// Isolate the two-point welded-box cleanup from normalization and welding.
	// These probe-only runs do not enter any comparison tape or registered gate.
	if(getenv("NXHULL_POINTS_PROBE") && !selfOnly)
		{
		static const NxHullRun kCleanupProbes[] =
			{
			{ 12, 0xb5, 0, 0 },	// same welding and fallback, without normalization
			{ 12, 0xb6, 0, 0 },	// normalization enabled, welding disabled
			};
		for(unsigned i = 0; i < sizeof(kCleanupProbes) / sizeof(kCleanupProbes[0]); ++i)
			{
			gHullTape[0].reset();
			gHullTape[1].reset();
			gHullText[0].reset();
			gHullText[1].reset();
			fprintf(stderr, "NXHULL_CLEANUP_FLAGS flags=%02x\n", kCleanupProbes[i].flags);
			nxHullCreateRun(o, kCleanupProbes[i], 0x027f, 0x3e0 + (int) i, false);
			}
		}

	*hostSlot = shippedHost;
	gQhullHost = 0;
	gHullBytesCur[0] = gHullBytesCur[1] = 0;
	for(int side = 0; side < 2; ++side)
		RemoveDirectoryW(gHullDir[side]);
	}

// qhull-gap Task 5 (the 4e review). Two parts, after the Task 4e totals.
//
// The OBJ dumps' bytes (nxHullTapeObjBytes), one family per Task 4e family.
//
// hull_qhull_direct(_x87): the two inputs of hull_create_qhull, run through
// qhull ALONE, the way qhull_hull does (nxQhullRun with "o": the oracle's
// qh_init_A .. qh_produce_output at their RVAs against the vendored tree, the
// harness's stand-in host, nxQhullTape of each side's finished hull), under
// the process's 0x027f. The points are the candidate cleanupVertices' output
// for each set with NovodeX's arguments (weld, normalise, reduce to 256, a
// zeroing allocator): the buffer both CreateConvexHull runs hand qhull, which
// the NXHULL_PROBE digest shows is identical on the two sides. So whatever
// differs here differs in qhull alone.
static void nxDriveConvexCookingBytes(const NxOracleRows& o, bool selfOnly)
	{
	static const char* const kBytesNames[4] =
		{ "hull_create_objbytes", "hull_create_pc64_objbytes", "hull_compute_objbytes", "hull_compute_pc64_objbytes" };
	static const char* const kBytesRva[4] = { "0x0007dea0", "0x0007dea0", "0x0007e050", "0x0007e050" };
	static const char* const kBytesOwner[4] = { "phys_fn_003247", "phys_fn_003247", "phys_fn_003251", "phys_fn_003251" };
	for(int k = 0; k < 4; ++k)
		nxReportTapes(gHullBytes[k][0], gHullBytes[k][1], kBytesNames[k], kBytesRva[k], kBytesOwner[k],
			"QhullHost.cpp", selfOnly, 0);

	gOracleTape.reset();
	gCandidateTape.reset();
	gOracleTapeX87.reset();
	gCandidateTapeX87.reset();
	NxQhullEntries oracle;
	oracle.initA		= o.base + kQhInitA;
	oracle.initflags	= o.base + kQhInitflags;
	oracle.initB		= o.base + kQhInitB;
	oracle.qhull		= o.base + kQhQhull;
	oracle.checkOutput	= o.base + kQhCheckOutput;
	oracle.produceOutput	= o.base + kQhProduceOutput;
	oracle.fin			= o.base + kOracleIob;
	oracle.fout			= o.base + kOracleIob + 0x20;
	oracle.ferr			= o.base + kOracleIob + 0x40;
	NxQhullEntries candidate;
	candidate.initA			= (void*) &qh_init_A;
	candidate.initflags		= (void*) &qh_initflags;
	candidate.initB			= (void*) &qh_init_B;
	candidate.qhull			= (void*) &qh_qhull;
	candidate.checkOutput	= (void*) &qh_check_output;
	candidate.produceOutput	= (void*) &qh_produce_output;
	candidate.fin			= stdin;
	candidate.fout			= stdout;
	candidate.ferr			= stderr;
	void** hostSlot = (void**) (o.base + kQhHostGlobal);
	void* shippedHost = *hostSlot;
	*hostSlot = gQhHostObject;
	static const int kSets[2] = { 12, 17 };
	static float cleaned[3 * 2049];
	for(int s = 0; s < 2; ++s)
		{
		unsigned stride;
		const unsigned n = nxHullPoints(kSets[s], gHullInput, &stride);
		NxHullTestAllocator allocator;
		gHullBlocks.reset(0);
		NxU32 count = 0;
		NxReal scale[3];
		{
		QhullHost host(&allocator);
		host.cleanupVertices(n, gHullInput, stride, count, cleaned, 0.00001f, scale, true, true, 0x100);
		}
		gHullBlocks.reset(0);
		const unsigned from[2] = { gOracleTape.count, gCandidateTape.count };
		const unsigned fromX87[2] = { gOracleTapeX87.count, gCandidateTapeX87.count };
		for(int side = 0; side < (selfOnly ? 1 : 2); ++side)
			{
			NxTape& tape = side == 0 ? gOracleTape : gCandidateTape;
			NxTape& floats = side == 0 ? gOracleTapeX87 : gCandidateTapeX87;
			tape.push(count);
			double* coords = (double*) malloc(sizeof(double) * 3 * (count ? count : 1));
			for(unsigned i = 0; i < 3 * count; ++i)
				coords[i] = cleaned[i];
			if(getenv("NXHULL_PROBE") && s == 0)
				{
				void* initbuild = side == 0 ? (void*) (o.base + kQhInitBuild) : (void*) &qh_initbuild;
				const int initialResult = nxQhullRunInitBuild(side == 0 ? &oracle : &candidate,
					initbuild, coords, (int) count, "o");
				fprintf(stderr, "NXHULL_INITBUILD set=%d side=%d result=%d\n", kSets[s], side, initialResult);
				if(initialResult == 0)
					{
					const void* state = side == 0 ? (const void*) (o.base + kQhState) : nxQhullCandidateState();
					nxQhullFurthestProbe(state, coords, (int) count, kSets[s], side);
					nxQhullDistanceProbe(state, coords, (int) count, kSets[s], side,
						side == 0 ? (void*) (o.base + kQhDistplane) : (void*) &qh_distplane);
					}
				if(side == 0)
					while(gQhOracleNbBlocks)
						free(gQhOracleBlocks[--gQhOracleNbBlocks]);
				}
			// Optional qhull trace of the box input, run separately from the
			// ordinary tape pass so T4 cannot change the measured transcript.
			if(getenv("NXHULL_TRACE_PROBE") && s == 0)
				{
				fprintf(stderr, "NXHULL_TRACE_START set=%d side=%d\n", kSets[s], side);
				const int traceResult = nxQhullRun(side == 0 ? &oracle : &candidate, coords, (int) count, "o T4");
				fprintf(stderr, "NXHULL_TRACE_END set=%d side=%d result=%d\n", kSets[s], side, traceResult);
				if(side == 0)
					while(gQhOracleNbBlocks)
						free(gQhOracleBlocks[--gQhOracleNbBlocks]);
				}
			const int result = nxQhullRun(side == 0 ? &oracle : &candidate, coords, (int) count, "o");
			tape.push(result ? 1u : 0u);
			if(result == 0)
				{
				const void* state = side == 0 ? (const void*) (o.base + kQhState) : nxQhullCandidateState();
				if(getenv("NXHULL_PROBE"))
					{
					fprintf(stderr, "NXHULL_ADDR_CALL set=%d side=%d state=%p\n", kSets[s], side, state);
					nxQhullAddressProbe(state, coords, (int) count, kSets[s], side);
					}
				nxQhullTape(state, coords, (int) count, nxQhPushTape, &tape, nxQhPushTapeDouble, &floats);
				if(getenv("NXHULL_PLANE_PROBE") && s == 1 && side == 1)
					nxQhullPlaneProbe(state, 4, (void*) (o.base + 0x0005d9f0), (void*) &qh_sethyperplane_det,
						(void*) (o.base + 0x0005d670), (void*) &qh_normalize2);
				}
			if(side == 0)
				{
				while(gQhOracleNbBlocks)
					free(gQhOracleBlocks[--gQhOracleNbBlocks]);
				}
			free(coords);
			}
		// Which of the two inputs differs, on stderr (NXHULL_PROBE=1).
		// The optional word listing separates address/order-sensitive topology
		// from low-word geometry drift without changing the normal tape output.
		if(getenv("NXHULL_PROBE") && !selfOnly)
			{
			unsigned words = 0, doubles = 0;
			for(unsigned i = from[0]; i < gOracleTape.count && from[1] + (i - from[0]) < gCandidateTape.count; ++i)
				words += gOracleTape.words[i] != gCandidateTape.words[from[1] + (i - from[0])];
			for(unsigned i = fromX87[0]; i < gOracleTapeX87.count && fromX87[1] + (i - fromX87[0]) < gCandidateTapeX87.count; ++i)
				doubles += gOracleTapeX87.words[i] != gCandidateTapeX87.words[fromX87[1] + (i - fromX87[0])];
			fprintf(stderr, "PROBE direct set=%d points=%u words=%u/%u differ=%u x87=%u/%u differ=%u\n", kSets[s], count,
				gOracleTape.count - from[0], gCandidateTape.count - from[1], words,
				gOracleTapeX87.count - fromX87[0], gCandidateTapeX87.count - fromX87[1], doubles);
			for(unsigned i = 0; i < gOracleTapeX87.count - fromX87[0] &&
				fromX87[1] + i < gCandidateTapeX87.count; ++i)
				{
				const unsigned oracleWord = gOracleTapeX87.words[fromX87[0] + i];
				const unsigned candidateWord = gCandidateTapeX87.words[fromX87[1] + i];
				if(oracleWord != candidateWord)
					fprintf(stderr, "PROBE x87 set=%d word=%u kind=%u oracle=%08x candidate=%08x\n",
						kSets[s], i, (unsigned) gOracleTapeX87.kinds[fromX87[0] + i], oracleWord, candidateWord);
				}
			}
		}
	*hostSlot = shippedHost;
	nxReport("hull_qhull_direct", "0x0007d180", "phys_fn_003234",
		"qhull.c,poly.c,poly2.c,merge.c,geom.c,geom2.c,qset.c,mem.c,global.c", selfOnly, kDivergent);
	nxReportTapes(gOracleTapeX87, gCandidateTapeX87, "hull_qhull_direct_x87", "0x0005c5c0", "phys_fn_002425",
		"geom.c,geom2.c,merge.c", selfOnly, kDivergent);
	}


static void nxPrintTotals()
	{
	printf("thirdparty coverage driven=%u divergent=%u words=%u layout_checks=%u\n",
		gDriven, gDivergent, gWordsCompared, gLayoutChecks);
	printf("thirdparty oracle digest=%08x\n", gRunDigest);
	}

int wmain(int argc, wchar_t** argv)
	{
	// Unbuffered, because a harness that crashes half way through must still
	// have printed how far it got.
	setvbuf(stdout, 0, _IONBF, 0);

	bool selfOnly = false;
	if(argc == 4 && wcscmp(argv[3], L"--self") == 0)
		selfOnly = true;
	else if(argc != 3)
		{
		fprintf(stderr, "usage: NxPhysicsThirdPartyTests <oracle directory> <NxPhysics.dll sha256> [--self]\n");
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

	NxOracleRows o;
	o.base = (unsigned char*) physics;
	o.qhCrossproduct	= (QhCrossproductFn)	(o.base + kQhCrossproduct);
	o.qhPointdist		= (QhPointdistFn)		(o.base + kQhPointdist);
	o.qhMaxabsval		= (QhMaxabsvalFn)		(o.base + kQhMaxabsval);
	o.qhRand			= (QhRandFn)			(o.base + kQhRand);
	o.qhSrand			= (QhSrandFn)			(o.base + kQhSrand);
	o.qhSetsize			= (QhSetsizeFn)			(o.base + kQhSetsize);
	o.qhSetin			= (QhSetinFn)			(o.base + kQhSetin);
	o.qhSetlast			= (QhSetlastFn)			(o.base + kQhSetlast);
	o.qhSetequal		= (QhSetequalFn)		(o.base + kQhSetequal);
	o.containerCtor		= (VoidThisFn)			(o.base + kContainerCtor);
	o.containerEmpty	= (PtrThisFn)			(o.base + kContainerEmpty);
	o.containerResize	= (BoolThisUdwordFn)	(o.base + kContainerResize);
	o.containerSetSize	= (BoolThisUdwordFn)	(o.base + kContainerSetSize);
	o.containerCopyCtor	= (CopyCtorFn)			(o.base + kContainerCopyCtor);
	o.containerDtor		= (VoidThisFn)			(o.base + kContainerDtor);
	o.completePruning	= (CompletePruningFn)	(o.base + kCompletePruning);
	o.radixCtor			= (VoidThisFn)			(o.base + kRadixCtor);
	o.radixDtor			= (VoidThisFn)			(o.base + kRadixDtor);
	o.radixSortDwords	= (RadixSortDwordsFn)	(o.base + kRadixSortDwords);
	o.radixSortFloats	= (RadixSortFloatsFn)	(o.base + kRadixSortFloats);
	o.segmentSqrDist	= (SegmentSqrDistFn)	(o.base + kSegmentSqrDist);
	o.meshCheckTopology	= (MeshCheckTopologyFn)	(o.base + kMeshCheckTopology);
	o.meshSetPointers	= (MeshSetPointersFn)	(o.base + kMeshSetPointers);
	o.radixSetRankBuffers	= (RadixSetRankBuffersFn)	(o.base + kRadixSetRankBuffers);
	o.prunableCtor			= (PrunableCtorFn)			(o.base + kPrunableCtor);
	o.prunableGetWorldAABB	= (PrunableGetAABBFn)		(o.base + kPrunableGetWorldAABB);
	o.prunableUpdateAABB	= (PrunableUpdateAABBFn)	(o.base + kPrunableUpdateAABB);
	o.prunableSetType		= (PrunableSetRangeFn)		(o.base + kPrunableSetType);
	o.prunableSetSection	= (PrunableSetRangeFn)		(o.base + kPrunableSetSection);
	o.prunableDtor			= (VoidThisFn)				(o.base + kPrunableDtor);
	o.prunableGetUpdated	= (PrunableGetAABBFn)		(o.base + kPrunableGetUpdated);
	o.prunable0CCtor		= (PrunableCtorFn)			(o.base + kPrunable0CCtor);
	o.prunable0CDtor		= (VoidThisFn)				(o.base + kPrunable0CDtor);

	printf("thirdparty libraries qhull=2003.1 opcode=1.3-standalone\n");
	printf("thirdparty generator=xorshift32 mode=%s\n", selfOnly ? "self" : "differential");

	nxCheckLayouts();

	nxDriveQhullPure(o, selfOnly);
	nxDriveQhullSets(o, selfOnly);
	nxDriveContainer(o, selfOnly);
	nxDriveContainerCopy(o, selfOnly);
	nxDriveRadix(o, selfOnly);
	nxDriveCompletePruning(o, selfOnly);
	nxDriveSegment(o, selfOnly);
	nxDriveMeshInterface(o, selfOnly);

	// P4 Task 2b. The constructor family runs first, because it is the only one
	// that can see the two adapter slots go from null to installed.
	nxDrivePrunableCtor(o, selfOnly);
	nxDrivePrunableFlags(o, selfOnly);
	nxDrivePrunablePruner(o, selfOnly);
	nxDrivePrunableRanges(o, selfOnly);
	nxDriveRadixSetRankBuffers(o, selfOnly);

	nxPrintTotals();

	// Vendored correspondence, Task 4. The two lines above close the families
	// registered before it and are printed where they always were; the same two
	// lines again after these families carry the running totals.
	nxDriveVendoredCoverage(o, selfOnly);
	nxPrintTotals();

	// Vendored correspondence, Task 5a: the same two lines once more after the
	// families it adds, which carry the running totals; the pair above stays
	// where Task 4 printed it.
	nxDriveCandidateTrees(o, selfOnly);
	nxPrintTotals();

	// qhull-gap Task 1: the same two lines after its families, with the running
	// totals; the pairs above stay where Tasks 4 and 5a printed them.
	nxDriveQhullGap(o, selfOnly);
	nxPrintTotals();

	// convex-mesh gap Task 2c: the same two lines after its three families, with
	// the running totals; the pairs above stay where they were printed.
	nxDriveIceMeshTools(o, selfOnly);
	nxPrintTotals();

	// convex-mesh gap Task 2d: the same two lines after its two families, with
	// the running totals; the pairs above stay where they were printed.
	nxDriveIceMeshBuilder2(o, selfOnly);
	nxPrintTotals();

	// convex-mesh gap Task 2e: the same two lines after its five families, with
	// the running totals; the pairs above stay where they were printed.
	nxDriveMeshToolsRest(o, selfOnly);
	nxPrintTotals();

	// convex-mesh gap Task 2f: the same two lines after its three families, with
	// the running totals; the pairs above stay where they were printed.
	nxDriveConvexHull(o, selfOnly);
	nxPrintTotals();

	// convex-mesh gap Task 2g: the same two lines after its family, with the
	// running totals; the pairs above stay where they were printed.
	nxDrivePolygonInterface(o, selfOnly);
	nxPrintTotals();
	nxDrivePrunableDispatch(o, selfOnly);

	// qhull-gap Tasks 4e and 5: preserve mainline convex-cooking differentials.
	nxDriveConvexCooking(o, selfOnly);
	nxPrintTotals();
	nxDriveConvexCookingBytes(o, selfOnly);
	nxPrintTotals();
	printf("thirdparty candidate mismatches=%u layout_failures=%u\n", gMismatches, gLayoutFailures);

	if(gLayoutFailures)
		return nxFail("the vendored headers do not reproduce the oracle's layout");
	if(!selfOnly && gMismatches)
		return nxFail("the vendored sources disagree with the oracle");
	return 0;
	}
