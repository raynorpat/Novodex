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

#include "Opcode.h"

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

// A row's transcript: every word either side produced, in order.
struct NxTape
	{
	enum { kMax = 262144 };
	unsigned words[kMax];
	unsigned count;
	unsigned overflow;

	void reset()					{ count = 0; overflow = 0; }
	void push(unsigned word)		{ if(count < kMax) words[count++] = word; else ++overflow; }
	void pushFloat(float value)		{ unsigned w; memcpy(&w, &value, sizeof(w)); push(w); }
	void pushDouble(double value)	{ unsigned w[2]; memcpy(w, &value, sizeof(w)); push(w[0]); push(w[1]); }
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

// One driven row: compare the two tapes word for word, print the oracle digest,
// and fold it into the run digest.
//
// `ulpTolerance` is 0 -- exact agreement required -- for every family but one.
// kDivergent marks a family that is measured and reported and does NOT assert
// agreement; a family driven that way is not a proof of its row, and the report
// says so by name. Its counts are still registered, so they cannot move
// unnoticed.
static const unsigned kDivergent = 0xffffffffu;

static void nxReport(const char* name, const char* rva, const char* owner, const char* source,
	bool selfOnly, unsigned ulpTolerance = 0)
	{
	const unsigned oracleDigest = gOracleTape.digest();
	unsigned mismatches = 0;
	unsigned fatal = 0;
	unsigned worstUlp = 0;
	if(!selfOnly)
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
					if(ulpTolerance != kDivergent && ulp > ulpTolerance)
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
	if(ulpTolerance == kDivergent)
		++gDivergent;
	gWordsCompared += gOracleTape.count;
	gRunDigest = nxFold(gRunDigest, oracleDigest);
	printf("thirdparty name=%s rva=%s owner=%s source=%s words=%u oracle=%08x mismatches=%u"
		" worst_ulp=%u verdict=%s\n",
		name, rva, owner, source, gOracleTape.count, oracleDigest, mismatches, worstUlp,
		ulpTolerance == kDivergent ? "divergent" : (fatal ? "FAILED" : "exact"));
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

static float nxNextFloat()
	{
	unsigned bits = nxNext();
	float value;
	memcpy(&value, &bits, sizeof(value));
	return value;
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
			floats[i] = nxNextFloat();
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

static void nxDriveSegment(const NxOracleRows& o, bool selfOnly)
	{
	// Integral coordinates in [-32, 32]. Every product and every sum below is
	// exactly representable and the final SquareMagnitude is a sum of squares --
	// all positive, so the result cannot cancel. The intermediate can: fT/SqrLen
	// is inexact and `Diff -= fT*Dir` then subtracts two nearly equal
	// quantities. So this family is NOT clean either. It reports
	// mismatches=9356 worst_ulp=67, is registered `divergent`, and is not
	// counted as a proof of the row -- the same as the wide family below and
	// for the same reason: st(0) at 53 bits against stores no C++ type names.
	// What separates the two is size, 67 ULP here against 8420 there.
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
		selfOnly, kDivergent);

	// THE SAME ROW OVER A DOMAIN WHERE THE TWO CODE GENERATORS DIVERGE, AND WHY
	// IT IS RECORDED RATHER THAN DROPPED OR TOLERATED.
	//
	// IceSegment.cpp is STOCK -- there is no local modification in it -- so what
	// this family compares is a 2003 x87 code generator against a 2026 one over
	// the same statements. They round in different places: the dot product and
	// the three-term SquareMagnitude live in st(0) at the process's 53-bit
	// precision and are stored to a 32-bit float at points no C++ type names.
	// On the grid family above that is unobservable. Over coordinates spanning
	// 1e5, `Diff -= fT*Dir` cancels and amplifies the last bit of fT into the
	// third decimal digit: 43% of draws differ and the worst is thousands of
	// ULP.
	//
	// So this family is NOT an assertion that the two agree, and its result is
	// NOT counted as a proof of the row -- `segment_sqrdist.wide` is reported as
	// DIVERGENT. What makes it worth running anyway is that its counts are
	// registered in gate_targets.ps1: they are a measurement of how far a
	// vendored stock row drifts under a 23-year compiler gap, and if that
	// measurement moves -- in either direction, including to zero -- the gate
	// fails and somebody has to say why.
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
		selfOnly, kDivergent);
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
	void	NovodeXPrunerSlot1()					{}
	void	RemoveObject(Prunable* object)			{ ++gPrunerRemovals; gPrunerLastRemoved = object; }
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
		candidatePruner->mWorldBoxes = candidateBoxes;

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
		candidatePruner->~NxCandidatePruner();
		gPrunableOwnerWorldAABB = 0;
		}
	*(PrunableWorldAABBFn*) (o.base + kDataOwnerWorldAABB) = 0;
	nxReport("prunable_pruner", "0x000b5590", "phys_fn_004884", "IcePrunable.cpp", selfOnly);
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
	nxLayout("Pruner.mWorldBoxes", (size_t) &(((Pruner*) 0)->mWorldBoxes), 0x14, "0x000b55a0");
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
	nxDriveSegment(o, selfOnly);
	nxDriveMeshInterface(o, selfOnly);

	// P4 Task 2b. The constructor family runs first, because it is the only one
	// that can see the two adapter slots go from null to installed.
	nxDrivePrunableCtor(o, selfOnly);
	nxDrivePrunableFlags(o, selfOnly);
	nxDrivePrunablePruner(o, selfOnly);
	nxDrivePrunableRanges(o, selfOnly);
	nxDriveRadixSetRankBuffers(o, selfOnly);

	printf("thirdparty coverage driven=%u divergent=%u words=%u layout_checks=%u\n",
		gDriven, gDivergent, gWordsCompared, gLayoutChecks);
	printf("thirdparty oracle digest=%08x\n", gRunDigest);
	printf("thirdparty candidate mismatches=%u layout_failures=%u\n", gMismatches, gLayoutFailures);

	if(gLayoutFailures)
		return nxFail("the vendored headers do not reproduce the oracle's layout");
	if(!selfOnly && gMismatches)
		return nxFail("the vendored sources disagree with the oracle");
	return 0;
	}
