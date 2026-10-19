/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "PMap.h"
#include "NxPhysicsBackend.h"
#if !NX_PHYSICS_USE_X87
#include "portable/NxScalarGeometry.h"
#endif

#include "NxUserOutputStream.h"
#include "TriangleMesh.h"

// The pinned public NxPMap.h names both of these in NxCreatePMap's signature and
// includes neither; NxPhysics.h is what supplies them in a consumer build.
class NxTriangleMesh;
#include "NxPMap.h"

#include "Opcode.h"

static void nxPMapCollectFaceOrder(const Opcode::AABBQuantizedNoLeafNode* node,
	NxU32* faces, NxU32& count)
	{
	const NxU32 children[2] = { node->mPosData, node->mNegData };
	for(unsigned child = 0; child < 2; ++child)
		{
		if(children[child] & 1)
			faces[count++] = children[child] >> 1;
		else
			nxPMapCollectFaceOrder(reinterpret_cast<const Opcode::AABBQuantizedNoLeafNode*>(
				static_cast<size_t>(children[child])), faces, count);
		}
	}

static void nxPMapCollectFaceOrder(const Opcode::AABBNoLeafNode* node,
	NxU32* faces, NxU32& count)
	{
	const NxU32 children[2] = { node->mPosData, node->mNegData };
	for(unsigned child = 0; child < 2; ++child)
		{
		if(children[child] & 1)
			faces[count++] = children[child] >> 1;
		else
			nxPMapCollectFaceOrder(reinterpret_cast<const Opcode::AABBNoLeafNode*>(
				static_cast<size_t>(children[child])), faces, count);
		}
	}

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// The reported file name is the oracle's, for the same reason PhysicsSDK.cpp's
// is: the two rejection sites pass __FILE__ and reproducing what the SDK reports
// means reproducing that string. Measured at .rdata 0x0010802c.
#define NX_PENETRATION_MAP_CPP	"\\Epic\\Novodex\\SDKs\\Physics\\src\\PenetrationMap.cpp"

// The two rejection sites report NovodeX's own __LINE__, pushed as immediates at
// 0x0005070c and 0x000506d7. This file is not that file and its own __LINE__ is
// its own, so the two numbers are named constants: recorded from the image, not
// manufactured by padding this file out to 986 lines.
static const NxI32	kPMapBadHeaderLine		= 0x3d3;	// 979
static const NxI32	kPMapBadVersionLine		= 0x3da;	// 986

// .rdata 0x00107f78, 0x00107fb8 and 0x00108000.
static const char* const kPMapBadHeaderMessage =
	"PenetrationMap::Create: the pmap file is invalid (bad header)";
static const char* const kPMapBadVersionMessage =
	"PenetrationMap::Create: the pmap file is invalid (bad version number)";
static const char* const kPMapNoMeshMessage =
	"PenetrationMap::Create: no triangle mesh";
static const NxI32	kPMapNoMeshLine			= 0x3c8;	// 968, pushed at 0x00050661

// The tag is four SEPARATE byte reads compared one at a time -- 0x0005069a,
// 0x000506a8, 0x000506b3 and 0x000506be -- so it has no byte order to get wrong.
static const NxU8	kPMapTag[4]				= { 'P', 'M', 'A', 'P' };
// `cmp eax,4` at 0x000506c9. Exactly four; 3 and 5 are both rejected.
static const NxU32	kPMapVersion			= 4;

// 0x00050110's terminator test is `cmp ebp,-1` at 0x0005020d.
static const NxU32	kPMapEndOfValues		= 0xffffffffu;

// The bit the sign block ORs in at 0x00050290 and the bit the corner pass ORs in
// at 0x00050466.
static const NxU32	kPMapCellFilled			= 0x80000000u;
static const NxU32	kPMapCellInterior		= 0x40000000u;
static const NxU32	kPMapEmptyValue			= 0x3fffffffu;

// phys_fn_002008's jump table gives the exact codebook for adjacent cells.
// Coordinates are ordered x (the fastest-varying grid coordinate), y, z.
struct PMapCellStep
	{
	NxI32 x, y, z;
	};

static const PMapCellStep kPMapCellSteps[26] =
	{
	{ -1,  0,  0 }, {  1,  0,  0 }, {  0, -1,  0 }, {  0,  1,  0 },
	{  0,  0, -1 }, {  0,  0,  1 }, { -1, -1,  0 }, {  1,  1,  0 },
	{ -1,  1,  0 }, {  1, -1,  0 }, {  0, -1, -1 }, {  0,  1,  1 },
	{  0, -1,  1 }, {  0,  1, -1 }, { -1,  0, -1 }, {  1,  0,  1 },
	{ -1,  0,  1 }, {  1,  0, -1 }, { -1, -1, -1 }, {  1,  1,  1 },
	{ -1, -1,  1 }, {  1,  1, -1 }, {  1, -1, -1 }, { -1,  1,  1 },
	{  1, -1,  1 }, { -1,  1, -1 }
	};

struct PMapMortonCell
	{
	NxU32 index;
	NxU32 key;
	};

static int comparePMapMortonCell(const void* lhs, const void* rhs)
	{
	const PMapMortonCell* a = static_cast<const PMapMortonCell*>(lhs);
	const PMapMortonCell* b = static_cast<const PMapMortonCell*>(rhs);
	if(a->key < b->key) return -1;
	if(a->key > b->key) return 1;
	return a->index < b->index ? -1 : (a->index > b->index ? 1 : 0);
	}

struct PMapValueCell
	{
	NxU32 value;
	NxU32 index;
	};

static int comparePMapValueCell(const void* lhs, const void* rhs)
	{
	const PMapValueCell* a = static_cast<const PMapValueCell*>(lhs);
	const PMapValueCell* b = static_cast<const PMapValueCell*>(rhs);
	if(a->value < b->value) return -1;
	if(a->value > b->value) return 1;
	return a->index < b->index ? -1 : (a->index > b->index ? 1 : 0);
	}

static_assert(sizeof(NxU32) == 4, "the grid is dwords");

struct NxPMapPointTriangleTerms
	{
	NxReal a00;
	NxReal a01;
	NxReal a11;
	NxReal b0;
	NxReal b1;
	NxReal det;
	NxReal s;
	NxReal dx;
	NxReal dy;
	NxReal dz;
	NxReal e1x;
	NxReal e1y;
	NxReal e1z;
	};

// The first edge's three products (0x000329eb..0x00032a9e): a00 = (x^2 + z^2)
// + y^2, a01 = (e1.x x + e1.z z) + e1.y y and b0 = (d.x x + d.z z) + d.y y,
// with the first edge on the FPU stack throughout. A function of its own, entered
// only with stored floats: written inside the whole setup, MSVC held the edge in
// three 8-byte slots (cutting it to 53 bits under 0x0f7f) and read a01 and b0
// from those copies.
static __declspec(noinline) void nxPMapPointTriangleFirstEdge(const NxReal* v0, const NxReal* v1,
	NxPMapPointTriangleTerms* k)
	{
	const double e0x = (double) v1[0] - v0[0];
	const double e0y = (double) v1[1] - v0[1];
	const double e0z = (double) v1[2] - v0[2];
	k->a00 = (NxReal) ((e0x * e0x + e0z * e0z) + e0y * e0y);
	k->a01 = (NxReal) (((double) k->e1x * e0x + (double) k->e1z * e0z) + (double) k->e1y * e0y);
	k->b0 = (NxReal) (((double) k->dx * e0x + (double) k->dz * e0z) + (double) k->dy * e0y);
	}

// 0x000329e0..0x00032b0c. The first edge is on the FPU stack; the second edge
// and the offset are stored. a00 is (x^2 + z^2) + y^2 of the first edge.
static __declspec(noinline) void nxPMapPointTriangleTerms(const NxReal* point, const NxReal* v0,
	const NxReal* v1, const NxReal* v2, NxPMapPointTriangleTerms* k)
	{
	k->e1x = (NxReal) ((double) v2[0] - v0[0]);
	k->e1y = (NxReal) ((double) v2[1] - v0[1]);
	k->e1z = (NxReal) ((double) v2[2] - v0[2]);
	k->dx = (NxReal) ((double) v0[0] - point[0]);
	k->dy = (NxReal) ((double) v0[1] - point[1]);
	k->dz = (NxReal) ((double) v0[2] - point[2]);
	nxPMapPointTriangleFirstEdge(v0, v1, k);

	const NxReal e1x = k->e1x;
	const NxReal e1y = k->e1y;
	const NxReal e1z = k->e1z;
	k->a11 = (NxReal) (((double) e1z * e1z + (double) e1y * e1y) + (double) e1x * e1x);
	k->b1 = (NxReal) (((double) k->dz * e1z + (double) k->dy * e1y) + (double) k->dx * e1x);
	k->det = (NxReal) fabs((double) k->a11 * k->a00 - (double) k->a01 * k->a01);
	k->s = (NxReal) ((double) k->b1 * k->a01 - (double) k->b0 * k->a11);
	}

// c, the squared offset (0x00032ac8..0x00032ae2): (z^2 + y^2) + x^2.
static __forceinline double nxPMapPointTriangleC(const NxPMapPointTriangleTerms* k)
	{
	return ((double) k->dz * k->dz + (double) k->dy * k->dy) + (double) k->dx * k->dx;
	}

// t before any division (0x00032b10..0x00032b20): b0 a01 - b1 a00.
static __forceinline double nxPMapPointTriangleT(const NxPMapPointTriangleTerms* k)
	{
	return (double) k->b0 * k->a01 - (double) k->b1 * k->a00;
	}

// The denominator of the three quotient leaves: (a00 - (a01 + a01)) + a11.
static __forceinline double nxPMapPointTriangleDenominator(const NxPMapPointTriangleTerms* k)
	{
	return ((double) k->a00 - ((double) k->a01 + k->a01)) + k->a11;
	}

// 0x00032eec: the parameters, then |squared|.
static __forceinline double nxPMapPointTriangleFinish(NxReal* sParam, NxReal* tParam,
	NxReal s, NxReal t, double squared)
	{
	if(sParam)
		*sParam = s;
	if(tParam)
		*tParam = t;
	return fabs(squared);
	}

// Vertex 1 (0x00032cc9): s = 1, t = 0, ((b0 + b0) + c) + a00.
static __declspec(noinline) double nxPMapPointTriangleVertex1(const NxPMapPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	return nxPMapPointTriangleFinish(sParam, tParam, 1.0f, 0.0f,
		(((double) k->b0 + k->b0) + nxPMapPointTriangleC(k)) + k->a00);
	}

// Vertex 2 (0x00032d0c, 0x00032d9e, 0x00032e5c): s = 0, t = 1,
// ((b1 + b1) + c) + a11.
static __declspec(noinline) double nxPMapPointTriangleVertex2(const NxPMapPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	return nxPMapPointTriangleFinish(sParam, tParam, 0.0f, 1.0f,
		(((double) k->b1 + k->b1) + nxPMapPointTriangleC(k)) + k->a11);
	}

// Vertex 0 (0x00032bb9, 0x00032c01 with 0x00032c1e, 0x00032d2d, 0x00032e1c):
// both parameters 0 and the squared distance c.
static __declspec(noinline) double nxPMapPointTriangleVertex0(const NxPMapPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	return nxPMapPointTriangleFinish(sParam, tParam, 0.0f, 0.0f, nxPMapPointTriangleC(k));
	}

// Edge 0's quotient (0x00032e2d): s = -(b0 / a00), kept wide for s b0 + c.
static __declspec(noinline) double nxPMapPointTriangleEdge0Quotient(const NxPMapPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	const double s = -((double) k->b0 / k->a00);
	return nxPMapPointTriangleFinish(sParam, tParam, (NxReal) s, 0.0f, s * k->b0 + nxPMapPointTriangleC(k));
	}

// Edge 1's quotient (0x00032d3a/0x00032d3e): t = -(b1 / a11), kept wide.
static __declspec(noinline) double nxPMapPointTriangleEdge1Quotient(const NxPMapPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	const double t = -((double) k->b1 / k->a11);
	return nxPMapPointTriangleFinish(sParam, tParam, 0.0f, (NxReal) t, t * k->b1 + nxPMapPointTriangleC(k));
	}

// Edge 0, t = 0 (0x00032b72, from regions 4 and 5): past the far vertex when
// -b0 >= a00.
static __declspec(noinline) double nxPMapPointTriangleEdge0(const NxPMapPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	if(-k->b0 >= k->a00)
		return nxPMapPointTriangleVertex1(k, sParam, tParam);
	return nxPMapPointTriangleEdge0Quotient(k, sParam, tParam);
	}

// Edge 1, s = 0 (0x00032ba0, regions 3 and 4).
static __declspec(noinline) double nxPMapPointTriangleEdge1(const NxPMapPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	if(k->b1 >= 0.0f)
		return nxPMapPointTriangleVertex0(k, sParam, tParam);
	if(-k->b1 >= k->a11)
		return nxPMapPointTriangleVertex2(k, sParam, tParam);
	return nxPMapPointTriangleEdge1Quotient(k, sParam, tParam);
	}

// The interior sum (0x00032eb1): the first t is `first`, every later read of
// t and s is the narrowed copy; ((X + Y) + c) with
// X = t ((first a11 + s a01) + (b1 + b1)) and Y = s ((t a01 + s a00) + (b0 + b0)).
static __forceinline double nxPMapPointTriangleInterior(const NxPMapPointTriangleTerms* k,
	NxReal s, NxReal t, double first)
	{
	return ((first * k->a11 + (double) s * k->a01) + ((double) k->b1 + k->b1)) * t
		+ (((double) t * k->a01 + (double) s * k->a00) + ((double) k->b0 + k->b0)) * s
		+ nxPMapPointTriangleC(k);
	}

// The interior entered from a quotient s (0x00032e9b..0x00032ead): t = 1 - s
// stored with `fst` and its wide value used first.
static __declspec(noinline) double nxPMapPointTriangleInteriorFromS(const NxPMapPointTriangleTerms* k,
	NxReal s, NxReal* sParam, NxReal* tParam)
	{
	const double first = 1.0f - (double) s;
	const NxReal t = (NxReal) first;
	return nxPMapPointTriangleFinish(sParam, tParam, s, t, nxPMapPointTriangleInterior(k, s, t, first));
	}

// The interior entered with t reloaded (0x00032c79, 0x00032dd8).
static __declspec(noinline) double nxPMapPointTriangleInteriorNarrow(const NxPMapPointTriangleTerms* k,
	NxReal s, NxReal t, NxReal* sParam, NxReal* tParam)
	{
	return nxPMapPointTriangleFinish(sParam, tParam, s, t, nxPMapPointTriangleInterior(k, s, t, t));
	}

// Region 0 (0x00032c2b): the determinant-zero interior returns FLT_MAX
// (0x10106858) with both parameters 0; otherwise s and t are scaled by the
// wide reciprocal and narrowed.
static __declspec(noinline) double nxPMapPointTriangleRegion0(const NxPMapPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	if(k->det == 0.0f)
		return nxPMapPointTriangleFinish(sParam, tParam, 0.0f, 0.0f, 3.402823466e+38f);
	const double inverse = 1.0f / (double) k->det;
	const NxReal s = (NxReal) ((double) k->s * inverse);
	const NxReal t = (NxReal) (inverse * nxPMapPointTriangleT(k));
	return nxPMapPointTriangleInteriorNarrow(k, s, t, sParam, tParam);
	}

// Region 2's quotient (0x00032cab..0x00032cc5): numer = (b1 + a11) - tmp0.
static __declspec(noinline) double nxPMapPointTriangleRegion2Quotient(const NxPMapPointTriangleTerms* k,
	NxReal tmp0, NxReal* sParam, NxReal* tParam)
	{
	const double numer = ((double) k->b1 + k->a11) - tmp0;
	const double denom = nxPMapPointTriangleDenominator(k);
	if(numer >= denom)
		return nxPMapPointTriangleVertex1(k, sParam, tParam);
	return nxPMapPointTriangleInteriorFromS(k, (NxReal) (numer / denom), sParam, tParam);
	}

// Region 2 (0x00032c8b): tmp0 = b0 + a01 narrowed into the t slot, tmp1 =
// b1 + a11 wide.
static __declspec(noinline) double nxPMapPointTriangleRegion2(const NxPMapPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	const NxReal tmp0 = (NxReal) ((double) k->b0 + k->a01);
	const double tmp1 = (double) k->b1 + k->a11;
	if(tmp1 > tmp0)
		return nxPMapPointTriangleRegion2Quotient(k, tmp0, sParam, tParam);
	if(tmp1 <= 0.0)
		return nxPMapPointTriangleVertex2(k, sParam, tParam);
	if(k->b1 >= 0.0f)
		return nxPMapPointTriangleVertex0(k, sParam, tParam);
	return nxPMapPointTriangleEdge1Quotient(k, sParam, tParam);
	}

// Region 6's quotient (0x00032d80..0x00032dd8): t = numer / denom narrowed,
// s = 1 - t narrowed, and the interior entered with t reloaded.
static __declspec(noinline) double nxPMapPointTriangleRegion6Quotient(const NxPMapPointTriangleTerms* k,
	NxReal tmp0, NxReal* sParam, NxReal* tParam)
	{
	const double numer = ((double) k->b0 + k->a00) - tmp0;
	const double denom = nxPMapPointTriangleDenominator(k);
	if(numer >= denom)
		return nxPMapPointTriangleVertex2(k, sParam, tParam);
	const NxReal t = (NxReal) (numer / denom);
	const NxReal s = (NxReal) (1.0f - (double) t);
	return nxPMapPointTriangleInteriorNarrow(k, s, t, sParam, tParam);
	}

// Region 6 (0x00032d65): tmp0 = b1 + a01 narrowed, tmp1 = b0 + a00 wide.
static __declspec(noinline) double nxPMapPointTriangleRegion6(const NxPMapPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	const NxReal tmp0 = (NxReal) ((double) k->b1 + k->a01);
	const double tmp1 = (double) k->b0 + k->a00;
	if(tmp1 > tmp0)
		return nxPMapPointTriangleRegion6Quotient(k, tmp0, sParam, tParam);
	if(tmp1 <= 0.0)
		return nxPMapPointTriangleVertex1(k, sParam, tParam);
	if(k->b0 >= 0.0f)
		return nxPMapPointTriangleVertex0(k, sParam, tParam);
	return nxPMapPointTriangleEdge0Quotient(k, sParam, tParam);
	}

// Region 1 (0x00032e40): numer = ((b1 + a11) - a01) - b0, stored narrowed
// into the t slot and tested wide against 0; the division reads the narrowed
// copy.
static __declspec(noinline) double nxPMapPointTriangleRegion1(const NxPMapPointTriangleTerms* k,
	NxReal* sParam, NxReal* tParam)
	{
	const double numerWide = (((double) k->b1 + k->a11) - k->a01) - k->b0;
	const NxReal numer = (NxReal) numerWide;
	if(numerWide <= 0.0)
		return nxPMapPointTriangleVertex2(k, sParam, tParam);
	const double denom = nxPMapPointTriangleDenominator(k);
	if(numer >= denom)
		return nxPMapPointTriangleVertex1(k, sParam, tParam);
	return nxPMapPointTriangleInteriorFromS(k, (NxReal) (numer / denom), sParam, tParam);
	}

// The region decision (0x00032b10..0x00032b68, 0x00032bf2, 0x00032c82,
// 0x00032d50): s + t against the determinant, then the signs of s, t and b0.
// Returns Eberly's region number.
static __declspec(noinline) int nxPMapPointTriangleRegion(const NxPMapPointTriangleTerms* k)
	{
	const double t = nxPMapPointTriangleT(k);
	if((double) k->s + t <= k->det)
		{
		if(k->s < 0.0f)
			return (t < 0.0 && k->b0 < 0.0f) ? 4 : 3;
		return t < 0.0 ? 5 : 0;
		}
	if(k->s < 0.0f)
		return 2;
	return t < 0.0 ? 6 : 1;
	}

// phys_fn_001672 (0x000329e0, 1326 B)
// Point to triangle (origin v0, edges v1 - v0 and v2 - v0), with the two edge
// parameters written through optional pointers. The first edge stays on the
// FPU stack (0x000329eb..0x000329fc) and is squared and dotted from there; the
// second edge and the offset v0 - p are stored. a00, a01, a11, b0, b1, the
// determinant and s are narrowed -- b0, the determinant and s into the
// caller's first three argument slots, b1 into the fourth -- while c (the
// squared offset, 0x00032ac8..0x00032ae2) and t stay in registers; c is added
// last in every leaf. The quotient leaves keep the parameter they just divided
// in st(0) (`fst` at 0x00032d43, 0x00032e33, 0x00032ead) and use it wide; the
// interior leaf (0x00032eb1) reads its first t either wide or reloaded, by
// entry. The determinant-zero interior returns FLT_MAX (0x10106858) with both
// parameters 0. Every comparison keeps the listing's NaN side: a NaN takes the
// branch the `test ah` pattern gives it (for example b1 NaN at 0x00032bb7 is
// "b1 < 0", the quotient). Regions 3 and 4 share the edge-1 leaf and regions 4
// and 5 the edge-0 leaf, as the listing's jumps do.
__declspec(noinline) double __cdecl nxPMapPointTriangleSquareDistance(const NxReal* point,
	const NxReal* v0, const NxReal* v1, const NxReal* v2, NxReal* sParam, NxReal* tParam)
	{
	NxPMapPointTriangleTerms k;
	nxPMapPointTriangleTerms(point, v0, v1, v2, &k);
	switch(nxPMapPointTriangleRegion(&k))
		{
		case 0:
			return nxPMapPointTriangleRegion0(&k, sParam, tParam);
		case 1:
			return nxPMapPointTriangleRegion1(&k, sParam, tParam);
		case 2:
			return nxPMapPointTriangleRegion2(&k, sParam, tParam);
		case 3:
			return nxPMapPointTriangleEdge1(&k, sParam, tParam);
		case 4:
			return nxPMapPointTriangleEdge0(&k, sParam, tParam);
		case 5:
			if(k.b0 >= 0.0f)
				return nxPMapPointTriangleVertex0(&k, sParam, tParam);
			return nxPMapPointTriangleEdge0(&k, sParam, tParam);
		default:
			return nxPMapPointTriangleRegion6(&k, sParam, tParam);
		}
	}



struct PMapMeshSamplingView
	{
	unsigned char prefix[0x10];
	const NxVec3* vertices;
	const NxU32* triangles;
	};

static_assert(offsetof(PMapMeshSamplingView, vertices) == 0x10, "PenetrationMap reads vertices at mesh + 0x10");
static_assert(offsetof(PMapMeshSamplingView, triangles) == 0x14, "PenetrationMap reads indices at mesh + 0x14");

// phys_fn_002029 (0x0004f070). Samples the eight Morton-spread cell corners
// in the oracle's order, accumulating the high-bit and low-bit classes apart.
// Transcribed from the pinned listing, 0x0004f070..0x0004ff17 (1,054 instructions;
// the compare script is evidence/phase3-mesh-pmap-listing-compare.py). The C++
// version this replaced matched seven hand-built fixtures and disagreed with the
// oracle on 40% of 4,000 drawn samples over real maps (pmap_sample_dense).
static const float nxPMapOne = 1.0f;
static const float nxPMapZero = 0.0f;
static const float nxPMapTwo32 = 4294967296.0f;
__declspec(naked) bool PenetrationMap::sampleCell(NxU32 x, NxU32 y, NxU32 z, NxF32* output,
	const NxF32* point, const NxF32* direction, const NxF32* matrix) const
	{
	__asm
		{
		sub esp, 0x6c		// 0x0004f070
		mov edx, dword ptr [esp + 0x70]		// 0x0004f073
		push ebx		// 0x0004f077
		mov ebx, dword ptr [esp + 0x78]		// 0x0004f078
		push ebp		// 0x0004f07c
		mov ebp, dword ptr [esp + 0x80]		// 0x0004f07d
		push esi		// 0x0004f084
		mov esi, ecx		// 0x0004f085
		mov eax, dword ptr [esi + 4]		// 0x0004f087
		mov ebp, dword ptr [eax + ebp*4]		// 0x0004f08a
		mov ecx, dword ptr [eax + edx*4]		// 0x0004f08d
		mov edx, dword ptr [eax + edx*4 + 4]		// 0x0004f090
		shl ebp, 2		// 0x0004f094
		mov dword ptr [esp + 0x34], ebp		// 0x0004f097
		mov ebp, dword ptr [esp + 0x84]		// 0x0004f09b
		push edi		// 0x0004f0a2
		mov edi, dword ptr [eax + ebx*4]		// 0x0004f0a3
		mov ebx, dword ptr [eax + ebx*4 + 4]		// 0x0004f0a6
		mov eax, dword ptr [eax + ebp*4 + 4]		// 0x0004f0aa
		mov ebp, dword ptr [esp + 0x38]		// 0x0004f0ae
		shl edi, 1		// 0x0004f0b2
		add ebp, edi		// 0x0004f0b4
		shl ebx, 1		// 0x0004f0b6
		shl eax, 2		// 0x0004f0b8
		mov dword ptr [esp + 0x34], ebp		// 0x0004f0bb
		mov ebp, dword ptr [esp + 0x38]		// 0x0004f0bf
		add ebp, ebx		// 0x0004f0c3
		add edi, eax		// 0x0004f0c5
		add eax, ebx		// 0x0004f0c7
		mov ebx, dword ptr [esp + 0x34]		// 0x0004f0c9
		add ebx, ecx		// 0x0004f0cd
		mov dword ptr [esp + 0x3c], ebx		// 0x0004f0cf
		mov ebx, dword ptr [esp + 0x34]		// 0x0004f0d3
		add ebx, edx		// 0x0004f0d7
		mov dword ptr [esp + 0x30], ebx		// 0x0004f0d9
		lea ebx, [ecx + ebp]		// 0x0004f0dd
		mov dword ptr [esp + 0x34], ebx		// 0x0004f0e0
		lea ebx, [edi + ecx]		// 0x0004f0e4
		add ecx, eax		// 0x0004f0e7
		add eax, edx		// 0x0004f0e9
		mov dword ptr [esp + 0x10], eax		// 0x0004f0eb
		mov eax, dword ptr [esi + 0x5c]		// 0x0004f0ef
		add ebp, edx		// 0x0004f0f2
		add edi, edx		// 0x0004f0f4
		mov dword ptr [esp + 0x24], ecx		// 0x0004f0f6
		mov ecx, dword ptr [esp + 0x80]		// 0x0004f0fa
		dec eax		// 0x0004f101
		cmp ecx, eax		// 0x0004f102
		mov dword ptr [esp + 0x2c], ebp		// 0x0004f104
		mov dword ptr [esp + 0x38], ebx		// 0x0004f108
		mov dword ptr [esp + 0x1c], edi		// 0x0004f10c
		jne L4f125		// 0x0004f110
		or edx, 0xffffffff		// 0x0004f112
		mov dword ptr [esp + 0x10], edx		// 0x0004f115
		mov dword ptr [esp + 0x1c], edx		// 0x0004f119
		mov dword ptr [esp + 0x2c], edx		// 0x0004f11d
		mov dword ptr [esp + 0x30], edx		// 0x0004f121
L4f125:
		mov edi, dword ptr [esp + 0x84]		// 0x0004f125
		or ebx, 0xffffffff		// 0x0004f12c
		cmp edi, eax		// 0x0004f12f
		jne L4f143		// 0x0004f131
		mov dword ptr [esp + 0x10], ebx		// 0x0004f133
		mov dword ptr [esp + 0x24], ebx		// 0x0004f137
		mov dword ptr [esp + 0x2c], ebx		// 0x0004f13b
		mov dword ptr [esp + 0x34], ebx		// 0x0004f13f
L4f143:
		mov edx, dword ptr [esp + 0x88]		// 0x0004f143
		cmp edx, eax		// 0x0004f14a
		jne L4f15e		// 0x0004f14c
		mov dword ptr [esp + 0x10], ebx		// 0x0004f14e
		mov dword ptr [esp + 0x24], ebx		// 0x0004f152
		mov dword ptr [esp + 0x1c], ebx		// 0x0004f156
		mov dword ptr [esp + 0x38], ebx		// 0x0004f15a
L4f15e:
		test ecx, ecx		// 0x0004f15e
		mov dword ptr [esp + 0x88], ecx		// 0x0004f160
		fild dword ptr [esp + 0x88]		// 0x0004f167
		jge L4f176		// 0x0004f16e
		fadd dword ptr [nxPMapTwo32]		// 0x0004f170
L4f176:
		test edi, edi		// 0x0004f176
		fmul dword ptr [esi + 0x50]		// 0x0004f178
		mov dword ptr [esp + 0x88], edi		// 0x0004f17b
		fsub dword ptr [esi + 0x2c]		// 0x0004f182
		fadd dword ptr [esi + 0x20]		// 0x0004f185
		fstp dword ptr [esp + 0x4c]		// 0x0004f188
		fild dword ptr [esp + 0x88]		// 0x0004f18c
		jge L4f19b		// 0x0004f193
		fadd dword ptr [nxPMapTwo32]		// 0x0004f195
L4f19b:
		test edx, edx		// 0x0004f19b
		fmul dword ptr [esi + 0x54]		// 0x0004f19d
		mov dword ptr [esp + 0x88], edx		// 0x0004f1a0
		fsub dword ptr [esi + 0x30]		// 0x0004f1a7
		fadd dword ptr [esi + 0x24]		// 0x0004f1aa
		fstp dword ptr [esp + 0x50]		// 0x0004f1ad
		fild dword ptr [esp + 0x88]		// 0x0004f1b1
		jge L4f1c0		// 0x0004f1b8
		fadd dword ptr [nxPMapTwo32]		// 0x0004f1ba
L4f1c0:
		fmul dword ptr [esi + 0x58]		// 0x0004f1c0
		mov eax, dword ptr [esp + 0x90]		// 0x0004f1c3
		xor ecx, ecx		// 0x0004f1ca
		mov dword ptr [esp + 0x40], ecx		// 0x0004f1cc
		fsub dword ptr [esi + 0x34]		// 0x0004f1d0
		mov dword ptr [esp + 0x44], ecx		// 0x0004f1d3
		mov dword ptr [esp + 0x48], ecx		// 0x0004f1d7
		fadd dword ptr [esi + 0x28]		// 0x0004f1db
		fstp dword ptr [esp + 0x54]		// 0x0004f1de
		fld dword ptr [eax]		// 0x0004f1e2
		fsub dword ptr [esp + 0x4c]		// 0x0004f1e4
		fmul dword ptr [esi + 0x44]		// 0x0004f1e8
		fstp dword ptr [esp + 0x84]		// 0x0004f1eb
		fld dword ptr [eax + 4]		// 0x0004f1f2
		fsub dword ptr [esp + 0x50]		// 0x0004f1f5
		fmul dword ptr [esi + 0x48]		// 0x0004f1f9
		fstp dword ptr [esp + 0x80]		// 0x0004f1fc
		fld dword ptr [eax + 8]		// 0x0004f203
		mov eax, dword ptr [esp + 0x8c]		// 0x0004f206
		fsub dword ptr [esp + 0x54]		// 0x0004f20d
		fmul dword ptr [esi + 0x4c]		// 0x0004f211
		mov dword ptr [eax + 8], ecx		// 0x0004f214
		mov dword ptr [eax + 4], ecx		// 0x0004f217
		fstp dword ptr [esp + 0x88]		// 0x0004f21a
		mov dword ptr [eax], ecx		// 0x0004f221
		fld dword ptr [nxPMapOne]		// 0x0004f223
		mov eax, dword ptr [esp + 0x3c]		// 0x0004f229
		cmp eax, ebx		// 0x0004f22d
		fsub dword ptr [esp + 0x84]		// 0x0004f22f
		fstp dword ptr [esp + 0x20]		// 0x0004f236
		fld dword ptr [nxPMapOne]		// 0x0004f23a
		fsub dword ptr [esp + 0x80]		// 0x0004f240
		fstp dword ptr [esp + 0x18]		// 0x0004f247
		fld dword ptr [nxPMapOne]		// 0x0004f24b
		fsub dword ptr [esp + 0x88]		// 0x0004f251
		fstp dword ptr [esp + 0x28]		// 0x0004f258
		fld dword ptr [esp + 0x4c]		// 0x0004f25c
		fadd dword ptr [esi + 0x50]		// 0x0004f260
		fstp dword ptr [esp + 0x58]		// 0x0004f263
		fld dword ptr [esp + 0x50]		// 0x0004f267
		fadd dword ptr [esi + 0x54]		// 0x0004f26b
		fstp dword ptr [esp + 0x5c]		// 0x0004f26e
		fld dword ptr [esp + 0x54]		// 0x0004f272
		fadd dword ptr [esi + 0x58]		// 0x0004f276
		fstp dword ptr [esp + 0x60]		// 0x0004f279
		je L4f3d1		// 0x0004f27d
		mov edx, dword ptr [esi + 0x70]		// 0x0004f283
		mov eax, dword ptr [edx + eax*4]		// 0x0004f286
		mov ecx, dword ptr [esi + 0x74]		// 0x0004f289
		mov edx, dword ptr [ecx + 0x14]		// 0x0004f28c
		mov ebp, dword ptr [ecx + 0x10]		// 0x0004f28f
		and eax, 0x3fffffff		// 0x0004f292
		lea eax, [eax + eax*2]		// 0x0004f297
		mov ecx, dword ptr [edx + eax*4]		// 0x0004f29a
		lea eax, [edx + eax*4]		// 0x0004f29d
		lea ecx, [ecx + ecx*2]		// 0x0004f2a0
		lea edi, [ebp + ecx*4]		// 0x0004f2a3
		mov ecx, dword ptr [eax + 4]		// 0x0004f2a7
		mov eax, dword ptr [eax + 8]		// 0x0004f2aa
		lea edx, [ecx + ecx*2]		// 0x0004f2ad
		lea ebx, [ebp + edx*4]		// 0x0004f2b0
		lea ecx, [esp + 0x14]		// 0x0004f2b4
		push ecx		// 0x0004f2b8
		lea eax, [eax + eax*2]		// 0x0004f2b9
		lea edx, [esp + 0x94]		// 0x0004f2bc
		push edx		// 0x0004f2c3
		lea ebp, [ebp + eax*4]		// 0x0004f2c4
		push ebp		// 0x0004f2c8
		push ebx		// 0x0004f2c9
		lea eax, [esp + 0x5c]		// 0x0004f2ca
		push edi		// 0x0004f2ce
		push eax		// 0x0004f2cf
		call nxPMapPointTriangleSquareDistance		// 0x0004f2d0
		fstp st(0)		// 0x0004f2d5
		fld dword ptr [nxPMapOne]		// 0x0004f2d7
		mov ecx, dword ptr [esi + 0x70]		// 0x0004f2dd
		fsub dword ptr [esp + 0xa8]		// 0x0004f2e0
		mov edx, dword ptr [esp + 0x54]		// 0x0004f2e7
		mov eax, dword ptr [ecx + edx*4]		// 0x0004f2eb
		shr eax, 0x1f		// 0x0004f2ee
		fsub dword ptr [esp + 0x2c]		// 0x0004f2f1
		not eax		// 0x0004f2f5
		fld dword ptr [esp + 0x2c]		// 0x0004f2f7
		add esp, 0x18		// 0x0004f2fb
		test al, 1		// 0x0004f2fe
		fmul dword ptr [ebp]		// 0x0004f300
		fld dword ptr [esp + 0x90]		// 0x0004f303
		fmul dword ptr [ebx]		// 0x0004f30a
		faddp st(1), st(0)		// 0x0004f30c
		fld st(1)		// 0x0004f30e
		fmul dword ptr [edi]		// 0x0004f310
		faddp st(1), st(0)		// 0x0004f312
		fsub dword ptr [esp + 0x4c]		// 0x0004f314
		fstp dword ptr [esp + 0x64]		// 0x0004f318
		fld st(0)		// 0x0004f31c
		fmul dword ptr [edi + 4]		// 0x0004f31e
		fld dword ptr [esp + 0x90]		// 0x0004f321
		fmul dword ptr [ebx + 4]		// 0x0004f328
		faddp st(1), st(0)		// 0x0004f32b
		fld dword ptr [esp + 0x14]		// 0x0004f32d
		fmul dword ptr [ebp + 4]		// 0x0004f331
		faddp st(1), st(0)		// 0x0004f334
		fsub dword ptr [esp + 0x50]		// 0x0004f336
		fstp dword ptr [esp + 0x68]		// 0x0004f33a
		fmul dword ptr [edi + 8]		// 0x0004f33e
		fld dword ptr [esp + 0x90]		// 0x0004f341
		fmul dword ptr [ebx + 8]		// 0x0004f348
		faddp st(1), st(0)		// 0x0004f34b
		fld dword ptr [esp + 0x14]		// 0x0004f34d
		fmul dword ptr [ebp + 8]		// 0x0004f351
		faddp st(1), st(0)		// 0x0004f354
		fsub dword ptr [esp + 0x54]		// 0x0004f356
		fstp dword ptr [esp + 0x6c]		// 0x0004f35a
		fld dword ptr [esp + 0x28]		// 0x0004f35e
		fmul dword ptr [esp + 0x18]		// 0x0004f362
		fmul dword ptr [esp + 0x20]		// 0x0004f366
		fstp dword ptr [esp + 0x90]		// 0x0004f36a
		fld dword ptr [esp + 0x64]		// 0x0004f371
		fmul dword ptr [esp + 0x90]		// 0x0004f375
		fld dword ptr [esp + 0x68]		// 0x0004f37c
		fmul dword ptr [esp + 0x90]		// 0x0004f380
		fstp dword ptr [esp + 0x68]		// 0x0004f387
		fld dword ptr [esp + 0x6c]		// 0x0004f38b
		fmul dword ptr [esp + 0x90]		// 0x0004f38f
		fstp dword ptr [esp + 0x6c]		// 0x0004f396
		je L4f3b2		// 0x0004f39a
		mov ecx, dword ptr [esp + 0x68]		// 0x0004f39c
		fstp dword ptr [esp + 0x40]		// 0x0004f3a0
		mov edx, dword ptr [esp + 0x6c]		// 0x0004f3a4
		mov dword ptr [esp + 0x44], ecx		// 0x0004f3a8
		mov dword ptr [esp + 0x48], edx		// 0x0004f3ac
		jmp L4f3d1		// 0x0004f3b0
L4f3b2:
		mov eax, dword ptr [esp + 0x8c]		// 0x0004f3b2
		fadd dword ptr [eax]		// 0x0004f3b9
		fstp dword ptr [eax]		// 0x0004f3bb
		fld dword ptr [esp + 0x68]		// 0x0004f3bd
		fadd dword ptr [eax + 4]		// 0x0004f3c1
		fstp dword ptr [eax + 4]		// 0x0004f3c4
		fld dword ptr [esp + 0x6c]		// 0x0004f3c7
		fadd dword ptr [eax + 8]		// 0x0004f3cb
		fstp dword ptr [eax + 8]		// 0x0004f3ce
L4f3d1:
		mov eax, dword ptr [esp + 0x30]		// 0x0004f3d1
		cmp eax, -1		// 0x0004f3d5
		je L4f54d		// 0x0004f3d8
		mov ecx, dword ptr [esp + 0x58]		// 0x0004f3de
		mov edx, dword ptr [esp + 0x50]		// 0x0004f3e2
		mov dword ptr [esp + 0x70], ecx		// 0x0004f3e6
		mov ecx, dword ptr [esp + 0x54]		// 0x0004f3ea
		mov dword ptr [esp + 0x74], edx		// 0x0004f3ee
		mov edx, dword ptr [esi + 0x70]		// 0x0004f3f2
		mov dword ptr [esp + 0x78], ecx		// 0x0004f3f5
		mov eax, dword ptr [edx + eax*4]		// 0x0004f3f9
		mov ecx, dword ptr [esi + 0x74]		// 0x0004f3fc
		mov edx, dword ptr [ecx + 0x14]		// 0x0004f3ff
		mov ebx, dword ptr [ecx + 0x10]		// 0x0004f402
		and eax, 0x3fffffff		// 0x0004f405
		lea eax, [eax + eax*2]		// 0x0004f40a
		mov ecx, dword ptr [edx + eax*4]		// 0x0004f40d
		lea eax, [edx + eax*4]		// 0x0004f410
		lea ecx, [ecx + ecx*2]		// 0x0004f413
		lea edi, [ebx + ecx*4]		// 0x0004f416
		mov ecx, dword ptr [eax + 4]		// 0x0004f419
		mov eax, dword ptr [eax + 8]		// 0x0004f41c
		lea edx, [ecx + ecx*2]		// 0x0004f41f
		lea ebp, [ebx + edx*4]		// 0x0004f422
		lea ecx, [esp + 0x14]		// 0x0004f425
		push ecx		// 0x0004f429
		lea eax, [eax + eax*2]		// 0x0004f42a
		lea edx, [esp + 0x94]		// 0x0004f42d
		push edx		// 0x0004f434
		lea ebx, [ebx + eax*4]		// 0x0004f435
		push ebx		// 0x0004f438
		push ebp		// 0x0004f439
		lea eax, [esp + 0x80]		// 0x0004f43a
		push edi		// 0x0004f441
		push eax		// 0x0004f442
		call nxPMapPointTriangleSquareDistance		// 0x0004f443
		fstp st(0)		// 0x0004f448
		fld dword ptr [nxPMapOne]		// 0x0004f44a
		mov ecx, dword ptr [esi + 0x70]		// 0x0004f450
		fsub dword ptr [esp + 0xa8]		// 0x0004f453
		mov edx, dword ptr [esp + 0x48]		// 0x0004f45a
		mov eax, dword ptr [ecx + edx*4]		// 0x0004f45e
		add esp, 0x18		// 0x0004f461
		fsub dword ptr [esp + 0x14]		// 0x0004f464
		fld dword ptr [esp + 0x14]		// 0x0004f468
		fmul dword ptr [ebx]		// 0x0004f46c
		fld dword ptr [esp + 0x90]		// 0x0004f46e
		fmul dword ptr [ebp]		// 0x0004f475
		faddp st(1), st(0)		// 0x0004f478
		fld st(1)		// 0x0004f47a
		fmul dword ptr [edi]		// 0x0004f47c
		faddp st(1), st(0)		// 0x0004f47e
		fsub dword ptr [esp + 0x70]		// 0x0004f480
		fld st(1)		// 0x0004f484
		fmul dword ptr [edi + 4]		// 0x0004f486
		fld dword ptr [esp + 0x90]		// 0x0004f489
		fmul dword ptr [ebp + 4]		// 0x0004f490
		faddp st(1), st(0)		// 0x0004f493
		fld dword ptr [esp + 0x14]		// 0x0004f495
		fmul dword ptr [ebx + 4]		// 0x0004f499
		faddp st(1), st(0)		// 0x0004f49c
		fsub dword ptr [esp + 0x74]		// 0x0004f49e
		fstp dword ptr [esp + 0x68]		// 0x0004f4a2
		fxch st(1)		// 0x0004f4a6
		fmul dword ptr [edi + 8]		// 0x0004f4a8
		fld dword ptr [esp + 0x90]		// 0x0004f4ab
		fmul dword ptr [ebp + 8]		// 0x0004f4b2
		faddp st(1), st(0)		// 0x0004f4b5
		fld dword ptr [esp + 0x14]		// 0x0004f4b7
		fmul dword ptr [ebx + 8]		// 0x0004f4bb
		faddp st(1), st(0)		// 0x0004f4be
		fsub dword ptr [esp + 0x78]		// 0x0004f4c0
		fstp dword ptr [esp + 0x6c]		// 0x0004f4c4
		fld dword ptr [esp + 0x28]		// 0x0004f4c8
		fmul dword ptr [esp + 0x18]		// 0x0004f4cc
		fmul dword ptr [esp + 0x84]		// 0x0004f4d0
		fstp dword ptr [esp + 0x90]		// 0x0004f4d7
		fmul dword ptr [esp + 0x90]		// 0x0004f4de
		fld dword ptr [esp + 0x68]		// 0x0004f4e5
		fmul dword ptr [esp + 0x90]		// 0x0004f4e9
		fstp dword ptr [esp + 0x68]		// 0x0004f4f0
		fld dword ptr [esp + 0x6c]		// 0x0004f4f4
		fmul dword ptr [esp + 0x90]		// 0x0004f4f8
		fstp dword ptr [esp + 0x6c]		// 0x0004f4ff
		shr eax, 0x1f		// 0x0004f503
		not eax		// 0x0004f506
		test al, 1		// 0x0004f508
		je L4f52e		// 0x0004f50a
		fadd dword ptr [esp + 0x40]		// 0x0004f50c
		fstp dword ptr [esp + 0x40]		// 0x0004f510
		fld dword ptr [esp + 0x68]		// 0x0004f514
		fadd dword ptr [esp + 0x44]		// 0x0004f518
		fstp dword ptr [esp + 0x44]		// 0x0004f51c
		fld dword ptr [esp + 0x6c]		// 0x0004f520
		fadd dword ptr [esp + 0x48]		// 0x0004f524
		fstp dword ptr [esp + 0x48]		// 0x0004f528
		jmp L4f54d		// 0x0004f52c
L4f52e:
		mov eax, dword ptr [esp + 0x8c]		// 0x0004f52e
		fadd dword ptr [eax]		// 0x0004f535
		fstp dword ptr [eax]		// 0x0004f537
		fld dword ptr [esp + 0x68]		// 0x0004f539
		fadd dword ptr [eax + 4]		// 0x0004f53d
		fstp dword ptr [eax + 4]		// 0x0004f540
		fld dword ptr [esp + 0x6c]		// 0x0004f543
		fadd dword ptr [eax + 8]		// 0x0004f547
		fstp dword ptr [eax + 8]		// 0x0004f54a
L4f54d:
		mov eax, dword ptr [esp + 0x34]		// 0x0004f54d
		cmp eax, -1		// 0x0004f551
		je L4f6cb		// 0x0004f554
		mov ecx, dword ptr [esp + 0x4c]		// 0x0004f55a
		mov edx, dword ptr [esp + 0x5c]		// 0x0004f55e
		mov dword ptr [esp + 0x70], ecx		// 0x0004f562
		mov ecx, dword ptr [esp + 0x54]		// 0x0004f566
		mov dword ptr [esp + 0x74], edx		// 0x0004f56a
		mov edx, dword ptr [esi + 0x70]		// 0x0004f56e
		mov dword ptr [esp + 0x78], ecx		// 0x0004f571
		mov eax, dword ptr [edx + eax*4]		// 0x0004f575
		mov ecx, dword ptr [esi + 0x74]		// 0x0004f578
		mov edx, dword ptr [ecx + 0x14]		// 0x0004f57b
		mov ebx, dword ptr [ecx + 0x10]		// 0x0004f57e
		and eax, 0x3fffffff		// 0x0004f581
		lea eax, [eax + eax*2]		// 0x0004f586
		mov ecx, dword ptr [edx + eax*4]		// 0x0004f589
		lea eax, [edx + eax*4]		// 0x0004f58c
		lea ecx, [ecx + ecx*2]		// 0x0004f58f
		lea edi, [ebx + ecx*4]		// 0x0004f592
		mov ecx, dword ptr [eax + 4]		// 0x0004f595
		mov eax, dword ptr [eax + 8]		// 0x0004f598
		lea edx, [ecx + ecx*2]		// 0x0004f59b
		lea ebp, [ebx + edx*4]		// 0x0004f59e
		lea ecx, [esp + 0x14]		// 0x0004f5a1
		push ecx		// 0x0004f5a5
		lea eax, [eax + eax*2]		// 0x0004f5a6
		lea edx, [esp + 0x94]		// 0x0004f5a9
		push edx		// 0x0004f5b0
		lea ebx, [ebx + eax*4]		// 0x0004f5b1
		push ebx		// 0x0004f5b4
		push ebp		// 0x0004f5b5
		lea eax, [esp + 0x80]		// 0x0004f5b6
		push edi		// 0x0004f5bd
		push eax		// 0x0004f5be
		call nxPMapPointTriangleSquareDistance		// 0x0004f5bf
		fstp st(0)		// 0x0004f5c4
		fld dword ptr [nxPMapOne]		// 0x0004f5c6
		mov ecx, dword ptr [esi + 0x70]		// 0x0004f5cc
		fsub dword ptr [esp + 0xa8]		// 0x0004f5cf
		mov edx, dword ptr [esp + 0x4c]		// 0x0004f5d6
		add esp, 0x18		// 0x0004f5da
		fsub dword ptr [esp + 0x14]		// 0x0004f5dd
		fld dword ptr [esp + 0x90]		// 0x0004f5e1
		fmul dword ptr [ebp]		// 0x0004f5e8
		fld st(1)		// 0x0004f5eb
		fmul dword ptr [edi]		// 0x0004f5ed
		faddp st(1), st(0)		// 0x0004f5ef
		fld dword ptr [esp + 0x14]		// 0x0004f5f1
		fmul dword ptr [ebx]		// 0x0004f5f5
		faddp st(1), st(0)		// 0x0004f5f7
		fsub dword ptr [esp + 0x70]		// 0x0004f5f9
		fld dword ptr [esp + 0x90]		// 0x0004f5fd
		fmul dword ptr [ebp + 4]		// 0x0004f604
		fld dword ptr [esp + 0x14]		// 0x0004f607
		fmul dword ptr [ebx + 4]		// 0x0004f60b
		faddp st(1), st(0)		// 0x0004f60e
		fld st(2)		// 0x0004f610
		fmul dword ptr [edi + 4]		// 0x0004f612
		faddp st(1), st(0)		// 0x0004f615
		fsub dword ptr [esp + 0x74]		// 0x0004f617
		fstp dword ptr [esp + 0x68]		// 0x0004f61b
		fld dword ptr [esp + 0x90]		// 0x0004f61f
		fmul dword ptr [ebp + 8]		// 0x0004f626
		fld dword ptr [esp + 0x14]		// 0x0004f629
		fmul dword ptr [ebx + 8]		// 0x0004f62d
		faddp st(1), st(0)		// 0x0004f630
		fxch st(2)		// 0x0004f632
		fmul dword ptr [edi + 8]		// 0x0004f634
		faddp st(2), st(0)		// 0x0004f637
		fxch st(1)		// 0x0004f639
		fsub dword ptr [esp + 0x78]		// 0x0004f63b
		fstp dword ptr [esp + 0x6c]		// 0x0004f63f
		fld dword ptr [esp + 0x28]		// 0x0004f643
		fmul dword ptr [esp + 0x20]		// 0x0004f647
		fmul dword ptr [esp + 0x80]		// 0x0004f64b
		fstp dword ptr [esp + 0x90]		// 0x0004f652
		fmul dword ptr [esp + 0x90]		// 0x0004f659
		fld dword ptr [esp + 0x68]		// 0x0004f660
		fmul dword ptr [esp + 0x90]		// 0x0004f664
		fstp dword ptr [esp + 0x68]		// 0x0004f66b
		fld dword ptr [esp + 0x6c]		// 0x0004f66f
		fmul dword ptr [esp + 0x90]		// 0x0004f673
		fstp dword ptr [esp + 0x6c]		// 0x0004f67a
		mov eax, dword ptr [ecx + edx*4]		// 0x0004f67e
		shr eax, 0x1f		// 0x0004f681
		not eax		// 0x0004f684
		test al, 1		// 0x0004f686
		je L4f6ac		// 0x0004f688
		fadd dword ptr [esp + 0x40]		// 0x0004f68a
		fstp dword ptr [esp + 0x40]		// 0x0004f68e
		fld dword ptr [esp + 0x68]		// 0x0004f692
		fadd dword ptr [esp + 0x44]		// 0x0004f696
		fstp dword ptr [esp + 0x44]		// 0x0004f69a
		fld dword ptr [esp + 0x6c]		// 0x0004f69e
		fadd dword ptr [esp + 0x48]		// 0x0004f6a2
		fstp dword ptr [esp + 0x48]		// 0x0004f6a6
		jmp L4f6cb		// 0x0004f6aa
L4f6ac:
		mov eax, dword ptr [esp + 0x8c]		// 0x0004f6ac
		fadd dword ptr [eax]		// 0x0004f6b3
		fstp dword ptr [eax]		// 0x0004f6b5
		fld dword ptr [esp + 0x68]		// 0x0004f6b7
		fadd dword ptr [eax + 4]		// 0x0004f6bb
		fstp dword ptr [eax + 4]		// 0x0004f6be
		fld dword ptr [esp + 0x6c]		// 0x0004f6c1
		fadd dword ptr [eax + 8]		// 0x0004f6c5
		fstp dword ptr [eax + 8]		// 0x0004f6c8
L4f6cb:
		mov eax, dword ptr [esp + 0x38]		// 0x0004f6cb
		cmp eax, -1		// 0x0004f6cf
		je L4f84a		// 0x0004f6d2
		mov ecx, dword ptr [esp + 0x4c]		// 0x0004f6d8
		mov edx, dword ptr [esp + 0x50]		// 0x0004f6dc
		mov dword ptr [esp + 0x70], ecx		// 0x0004f6e0
		mov ecx, dword ptr [esp + 0x60]		// 0x0004f6e4
		mov dword ptr [esp + 0x74], edx		// 0x0004f6e8
		mov edx, dword ptr [esi + 0x70]		// 0x0004f6ec
		mov dword ptr [esp + 0x78], ecx		// 0x0004f6ef
		mov eax, dword ptr [edx + eax*4]		// 0x0004f6f3
		mov ecx, dword ptr [esi + 0x74]		// 0x0004f6f6
		mov edx, dword ptr [ecx + 0x14]		// 0x0004f6f9
		mov ebp, dword ptr [ecx + 0x10]		// 0x0004f6fc
		and eax, 0x3fffffff		// 0x0004f6ff
		lea eax, [eax + eax*2]		// 0x0004f704
		mov ecx, dword ptr [edx + eax*4]		// 0x0004f707
		lea eax, [edx + eax*4]		// 0x0004f70a
		lea ecx, [ecx + ecx*2]		// 0x0004f70d
		lea edi, [ebp + ecx*4]		// 0x0004f710
		mov ecx, dword ptr [eax + 4]		// 0x0004f714
		mov eax, dword ptr [eax + 8]		// 0x0004f717
		lea edx, [ecx + ecx*2]		// 0x0004f71a
		lea ebx, [ebp + edx*4]		// 0x0004f71d
		lea ecx, [esp + 0x14]		// 0x0004f721
		push ecx		// 0x0004f725
		lea eax, [eax + eax*2]		// 0x0004f726
		lea edx, [esp + 0x94]		// 0x0004f729
		push edx		// 0x0004f730
		lea ebp, [ebp + eax*4]		// 0x0004f731
		push ebp		// 0x0004f735
		push ebx		// 0x0004f736
		lea eax, [esp + 0x80]		// 0x0004f737
		push edi		// 0x0004f73e
		push eax		// 0x0004f73f
		call nxPMapPointTriangleSquareDistance		// 0x0004f740
		fstp st(0)		// 0x0004f745
		fld dword ptr [nxPMapOne]		// 0x0004f747
		mov ecx, dword ptr [esi + 0x70]		// 0x0004f74d
		fsub dword ptr [esp + 0xa8]		// 0x0004f750
		mov edx, dword ptr [esp + 0x50]		// 0x0004f757
		mov eax, dword ptr [ecx + edx*4]		// 0x0004f75b
		add esp, 0x18		// 0x0004f75e
		fsub dword ptr [esp + 0x14]		// 0x0004f761
		fld st(0)		// 0x0004f765
		fmul dword ptr [edi]		// 0x0004f767
		fld dword ptr [esp + 0x14]		// 0x0004f769
		fmul dword ptr [ebp]		// 0x0004f76d
		faddp st(1), st(0)		// 0x0004f770
		fld dword ptr [esp + 0x90]		// 0x0004f772
		fmul dword ptr [ebx]		// 0x0004f779
		faddp st(1), st(0)		// 0x0004f77b
		fsub dword ptr [esp + 0x70]		// 0x0004f77d
		fld st(1)		// 0x0004f781
		fmul dword ptr [edi + 4]		// 0x0004f783
		fld dword ptr [esp + 0x90]		// 0x0004f786
		fmul dword ptr [ebx + 4]		// 0x0004f78d
		faddp st(1), st(0)		// 0x0004f790
		fld dword ptr [esp + 0x14]		// 0x0004f792
		fmul dword ptr [ebp + 4]		// 0x0004f796
		faddp st(1), st(0)		// 0x0004f799
		fsub dword ptr [esp + 0x74]		// 0x0004f79b
		fstp dword ptr [esp + 0x68]		// 0x0004f79f
		fxch st(1)		// 0x0004f7a3
		fmul dword ptr [edi + 8]		// 0x0004f7a5
		fld dword ptr [esp + 0x90]		// 0x0004f7a8
		fmul dword ptr [ebx + 8]		// 0x0004f7af
		faddp st(1), st(0)		// 0x0004f7b2
		fld dword ptr [esp + 0x14]		// 0x0004f7b4
		fmul dword ptr [ebp + 8]		// 0x0004f7b8
		faddp st(1), st(0)		// 0x0004f7bb
		fsub dword ptr [esp + 0x78]		// 0x0004f7bd
		fstp dword ptr [esp + 0x6c]		// 0x0004f7c1
		fld dword ptr [esp + 0x18]		// 0x0004f7c5
		fmul dword ptr [esp + 0x20]		// 0x0004f7c9
		fmul dword ptr [esp + 0x88]		// 0x0004f7cd
		fstp dword ptr [esp + 0x90]		// 0x0004f7d4
		fmul dword ptr [esp + 0x90]		// 0x0004f7db
		fld dword ptr [esp + 0x68]		// 0x0004f7e2
		fmul dword ptr [esp + 0x90]		// 0x0004f7e6
		fstp dword ptr [esp + 0x68]		// 0x0004f7ed
		fld dword ptr [esp + 0x6c]		// 0x0004f7f1
		fmul dword ptr [esp + 0x90]		// 0x0004f7f5
		fstp dword ptr [esp + 0x6c]		// 0x0004f7fc
		shr eax, 0x1f		// 0x0004f800
		not eax		// 0x0004f803
		test al, 1		// 0x0004f805
		je L4f82b		// 0x0004f807
		fadd dword ptr [esp + 0x40]		// 0x0004f809
		fstp dword ptr [esp + 0x40]		// 0x0004f80d
		fld dword ptr [esp + 0x68]		// 0x0004f811
		fadd dword ptr [esp + 0x44]		// 0x0004f815
		fstp dword ptr [esp + 0x44]		// 0x0004f819
		fld dword ptr [esp + 0x6c]		// 0x0004f81d
		fadd dword ptr [esp + 0x48]		// 0x0004f821
		fstp dword ptr [esp + 0x48]		// 0x0004f825
		jmp L4f84a		// 0x0004f829
L4f82b:
		mov eax, dword ptr [esp + 0x8c]		// 0x0004f82b
		fadd dword ptr [eax]		// 0x0004f832
		fstp dword ptr [eax]		// 0x0004f834
		fld dword ptr [esp + 0x68]		// 0x0004f836
		fadd dword ptr [eax + 4]		// 0x0004f83a
		fstp dword ptr [eax + 4]		// 0x0004f83d
		fld dword ptr [esp + 0x6c]		// 0x0004f840
		fadd dword ptr [eax + 8]		// 0x0004f844
		fstp dword ptr [eax + 8]		// 0x0004f847
L4f84a:
		mov eax, dword ptr [esp + 0x1c]		// 0x0004f84a
		cmp eax, -1		// 0x0004f84e
		je L4f9c9		// 0x0004f851
		mov ecx, dword ptr [esp + 0x58]		// 0x0004f857
		mov edx, dword ptr [esp + 0x50]		// 0x0004f85b
		mov dword ptr [esp + 0x70], ecx		// 0x0004f85f
		mov ecx, dword ptr [esp + 0x60]		// 0x0004f863
		mov dword ptr [esp + 0x74], edx		// 0x0004f867
		mov edx, dword ptr [esi + 0x70]		// 0x0004f86b
		mov dword ptr [esp + 0x78], ecx		// 0x0004f86e
		mov eax, dword ptr [edx + eax*4]		// 0x0004f872
		mov ecx, dword ptr [esi + 0x74]		// 0x0004f875
		mov edx, dword ptr [ecx + 0x14]		// 0x0004f878
		mov edi, dword ptr [ecx + 0x10]		// 0x0004f87b
		and eax, 0x3fffffff		// 0x0004f87e
		lea eax, [eax + eax*2]		// 0x0004f883
		mov ecx, dword ptr [edx + eax*4]		// 0x0004f886
		lea eax, [edx + eax*4]		// 0x0004f889
		lea ecx, [ecx + ecx*2]		// 0x0004f88c
		lea ebx, [edi + ecx*4]		// 0x0004f88f
		mov ecx, dword ptr [eax + 4]		// 0x0004f892
		mov eax, dword ptr [eax + 8]		// 0x0004f895
		lea edx, [ecx + ecx*2]		// 0x0004f898
		lea ebp, [edi + edx*4]		// 0x0004f89b
		lea ecx, [esp + 0x14]		// 0x0004f89e
		push ecx		// 0x0004f8a2
		lea eax, [eax + eax*2]		// 0x0004f8a3
		lea edx, [esp + 0x94]		// 0x0004f8a6
		push edx		// 0x0004f8ad
		lea edi, [edi + eax*4]		// 0x0004f8ae
		push edi		// 0x0004f8b1
		push ebp		// 0x0004f8b2
		lea eax, [esp + 0x80]		// 0x0004f8b3
		push ebx		// 0x0004f8ba
		push eax		// 0x0004f8bb
		call nxPMapPointTriangleSquareDistance		// 0x0004f8bc
		fstp st(0)		// 0x0004f8c1
		fld dword ptr [nxPMapOne]		// 0x0004f8c3
		mov ecx, dword ptr [esi + 0x70]		// 0x0004f8c9
		fsub dword ptr [esp + 0xa8]		// 0x0004f8cc
		mov edx, dword ptr [esp + 0x34]		// 0x0004f8d3
		mov eax, dword ptr [ecx + edx*4]		// 0x0004f8d7
		add esp, 0x18		// 0x0004f8da
		fsub dword ptr [esp + 0x14]		// 0x0004f8dd
		fld dword ptr [esp + 0x14]		// 0x0004f8e1
		fmul dword ptr [edi]		// 0x0004f8e5
		fld dword ptr [esp + 0x90]		// 0x0004f8e7
		fmul dword ptr [ebp]		// 0x0004f8ee
		faddp st(1), st(0)		// 0x0004f8f1
		fld st(1)		// 0x0004f8f3
		fmul dword ptr [ebx]		// 0x0004f8f5
		faddp st(1), st(0)		// 0x0004f8f7
		fsub dword ptr [esp + 0x70]		// 0x0004f8f9
		fld st(1)		// 0x0004f8fd
		fmul dword ptr [ebx + 4]		// 0x0004f8ff
		fld dword ptr [esp + 0x90]		// 0x0004f902
		fmul dword ptr [ebp + 4]		// 0x0004f909
		faddp st(1), st(0)		// 0x0004f90c
		fld dword ptr [esp + 0x14]		// 0x0004f90e
		fmul dword ptr [edi + 4]		// 0x0004f912
		faddp st(1), st(0)		// 0x0004f915
		fsub dword ptr [esp + 0x74]		// 0x0004f917
		fstp dword ptr [esp + 0x68]		// 0x0004f91b
		fxch st(1)		// 0x0004f91f
		fmul dword ptr [ebx + 8]		// 0x0004f921
		fld dword ptr [esp + 0x90]		// 0x0004f924
		fmul dword ptr [ebp + 8]		// 0x0004f92b
		faddp st(1), st(0)		// 0x0004f92e
		fld dword ptr [esp + 0x14]		// 0x0004f930
		fmul dword ptr [edi + 8]		// 0x0004f934
		faddp st(1), st(0)		// 0x0004f937
		fsub dword ptr [esp + 0x78]		// 0x0004f939
		fstp dword ptr [esp + 0x6c]		// 0x0004f93d
		fld dword ptr [esp + 0x18]		// 0x0004f941
		fmul dword ptr [esp + 0x88]		// 0x0004f945
		fmul dword ptr [esp + 0x84]		// 0x0004f94c
		fstp dword ptr [esp + 0x90]		// 0x0004f953
		fmul dword ptr [esp + 0x90]		// 0x0004f95a
		fld dword ptr [esp + 0x68]		// 0x0004f961
		fmul dword ptr [esp + 0x90]		// 0x0004f965
		fstp dword ptr [esp + 0x68]		// 0x0004f96c
		fld dword ptr [esp + 0x6c]		// 0x0004f970
		fmul dword ptr [esp + 0x90]		// 0x0004f974
		fstp dword ptr [esp + 0x6c]		// 0x0004f97b
		shr eax, 0x1f		// 0x0004f97f
		not eax		// 0x0004f982
		test al, 1		// 0x0004f984
		je L4f9aa		// 0x0004f986
		fadd dword ptr [esp + 0x40]		// 0x0004f988
		fstp dword ptr [esp + 0x40]		// 0x0004f98c
		fld dword ptr [esp + 0x68]		// 0x0004f990
		fadd dword ptr [esp + 0x44]		// 0x0004f994
		fstp dword ptr [esp + 0x44]		// 0x0004f998
		fld dword ptr [esp + 0x6c]		// 0x0004f99c
		fadd dword ptr [esp + 0x48]		// 0x0004f9a0
		fstp dword ptr [esp + 0x48]		// 0x0004f9a4
		jmp L4f9c9		// 0x0004f9a8
L4f9aa:
		mov eax, dword ptr [esp + 0x8c]		// 0x0004f9aa
		fadd dword ptr [eax]		// 0x0004f9b1
		fstp dword ptr [eax]		// 0x0004f9b3
		fld dword ptr [esp + 0x68]		// 0x0004f9b5
		fadd dword ptr [eax + 4]		// 0x0004f9b9
		fstp dword ptr [eax + 4]		// 0x0004f9bc
		fld dword ptr [esp + 0x6c]		// 0x0004f9bf
		fadd dword ptr [eax + 8]		// 0x0004f9c3
		fstp dword ptr [eax + 8]		// 0x0004f9c6
L4f9c9:
		mov eax, dword ptr [esp + 0x24]		// 0x0004f9c9
		cmp eax, -1		// 0x0004f9cd
		je L4fb4b		// 0x0004f9d0
		mov ecx, dword ptr [esp + 0x4c]		// 0x0004f9d6
		mov edx, dword ptr [esp + 0x5c]		// 0x0004f9da
		mov dword ptr [esp + 0x70], ecx		// 0x0004f9de
		mov ecx, dword ptr [esp + 0x60]		// 0x0004f9e2
		mov dword ptr [esp + 0x74], edx		// 0x0004f9e6
		mov edx, dword ptr [esi + 0x70]		// 0x0004f9ea
		mov dword ptr [esp + 0x78], ecx		// 0x0004f9ed
		mov eax, dword ptr [edx + eax*4]		// 0x0004f9f1
		mov ecx, dword ptr [esi + 0x74]		// 0x0004f9f4
		mov edx, dword ptr [ecx + 0x14]		// 0x0004f9f7
		mov ebp, dword ptr [ecx + 0x10]		// 0x0004f9fa
		and eax, 0x3fffffff		// 0x0004f9fd
		lea eax, [eax + eax*2]		// 0x0004fa02
		mov ecx, dword ptr [edx + eax*4]		// 0x0004fa05
		lea eax, [edx + eax*4]		// 0x0004fa08
		lea ecx, [ecx + ecx*2]		// 0x0004fa0b
		lea edi, [ebp + ecx*4]		// 0x0004fa0e
		mov ecx, dword ptr [eax + 4]		// 0x0004fa12
		mov eax, dword ptr [eax + 8]		// 0x0004fa15
		lea edx, [ecx + ecx*2]		// 0x0004fa18
		lea ebx, [ebp + edx*4]		// 0x0004fa1b
		lea ecx, [esp + 0x1c]		// 0x0004fa1f
		push ecx		// 0x0004fa23
		lea eax, [eax + eax*2]		// 0x0004fa24
		lea edx, [esp + 0x94]		// 0x0004fa27
		push edx		// 0x0004fa2e
		lea ebp, [ebp + eax*4]		// 0x0004fa2f
		push ebp		// 0x0004fa33
		push ebx		// 0x0004fa34
		lea eax, [esp + 0x80]		// 0x0004fa35
		push edi		// 0x0004fa3c
		push eax		// 0x0004fa3d
		call nxPMapPointTriangleSquareDistance		// 0x0004fa3e
		fstp st(0)		// 0x0004fa43
		fld dword ptr [nxPMapOne]		// 0x0004fa45
		mov ecx, dword ptr [esi + 0x70]		// 0x0004fa4b
		fsub dword ptr [esp + 0xa8]		// 0x0004fa4e
		mov edx, dword ptr [esp + 0x3c]		// 0x0004fa55
		mov eax, dword ptr [ecx + edx*4]		// 0x0004fa59
		add esp, 0x18		// 0x0004fa5c
		fsub dword ptr [esp + 0x1c]		// 0x0004fa5f
		fld dword ptr [esp + 0x90]		// 0x0004fa63
		fmul dword ptr [ebx]		// 0x0004fa6a
		fld st(1)		// 0x0004fa6c
		fmul dword ptr [edi]		// 0x0004fa6e
		faddp st(1), st(0)		// 0x0004fa70
		fld dword ptr [esp + 0x1c]		// 0x0004fa72
		fmul dword ptr [ebp]		// 0x0004fa76
		faddp st(1), st(0)		// 0x0004fa79
		fsub dword ptr [esp + 0x70]		// 0x0004fa7b
		fld st(1)		// 0x0004fa7f
		fmul dword ptr [edi + 4]		// 0x0004fa81
		fld dword ptr [esp + 0x90]		// 0x0004fa84
		fmul dword ptr [ebx + 4]		// 0x0004fa8b
		faddp st(1), st(0)		// 0x0004fa8e
		fld dword ptr [esp + 0x1c]		// 0x0004fa90
		fmul dword ptr [ebp + 4]		// 0x0004fa94
		faddp st(1), st(0)		// 0x0004fa97
		fsub dword ptr [esp + 0x74]		// 0x0004fa99
		fstp dword ptr [esp + 0x68]		// 0x0004fa9d
		fxch st(1)		// 0x0004faa1
		fmul dword ptr [edi + 8]		// 0x0004faa3
		fld dword ptr [esp + 0x90]		// 0x0004faa6
		fmul dword ptr [ebx + 8]		// 0x0004faad
		faddp st(1), st(0)		// 0x0004fab0
		fld dword ptr [esp + 0x1c]		// 0x0004fab2
		fmul dword ptr [ebp + 8]		// 0x0004fab6
		faddp st(1), st(0)		// 0x0004fab9
		fsub dword ptr [esp + 0x78]		// 0x0004fabb
		fstp dword ptr [esp + 0x6c]		// 0x0004fabf
		fld dword ptr [esp + 0x20]		// 0x0004fac3
		fmul dword ptr [esp + 0x88]		// 0x0004fac7
		fmul dword ptr [esp + 0x80]		// 0x0004face
		fstp dword ptr [esp + 0x90]		// 0x0004fad5
		fmul dword ptr [esp + 0x90]		// 0x0004fadc
		fld dword ptr [esp + 0x68]		// 0x0004fae3
		fmul dword ptr [esp + 0x90]		// 0x0004fae7
		fstp dword ptr [esp + 0x68]		// 0x0004faee
		fld dword ptr [esp + 0x6c]		// 0x0004faf2
		fmul dword ptr [esp + 0x90]		// 0x0004faf6
		fstp dword ptr [esp + 0x6c]		// 0x0004fafd
		shr eax, 0x1f		// 0x0004fb01
		not eax		// 0x0004fb04
		test al, 1		// 0x0004fb06
		je L4fb2c		// 0x0004fb08
		fadd dword ptr [esp + 0x40]		// 0x0004fb0a
		fstp dword ptr [esp + 0x40]		// 0x0004fb0e
		fld dword ptr [esp + 0x68]		// 0x0004fb12
		fadd dword ptr [esp + 0x44]		// 0x0004fb16
		fstp dword ptr [esp + 0x44]		// 0x0004fb1a
		fld dword ptr [esp + 0x6c]		// 0x0004fb1e
		fadd dword ptr [esp + 0x48]		// 0x0004fb22
		fstp dword ptr [esp + 0x48]		// 0x0004fb26
		jmp L4fb4b		// 0x0004fb2a
L4fb2c:
		mov eax, dword ptr [esp + 0x8c]		// 0x0004fb2c
		fadd dword ptr [eax]		// 0x0004fb33
		fstp dword ptr [eax]		// 0x0004fb35
		fld dword ptr [esp + 0x68]		// 0x0004fb37
		fadd dword ptr [eax + 4]		// 0x0004fb3b
		fstp dword ptr [eax + 4]		// 0x0004fb3e
		fld dword ptr [esp + 0x6c]		// 0x0004fb41
		fadd dword ptr [eax + 8]		// 0x0004fb45
		fstp dword ptr [eax + 8]		// 0x0004fb48
L4fb4b:
		mov eax, dword ptr [esp + 0x2c]		// 0x0004fb4b
		cmp eax, -1		// 0x0004fb4f
		je L4fccd		// 0x0004fb52
		mov ecx, dword ptr [esp + 0x58]		// 0x0004fb58
		mov edx, dword ptr [esp + 0x5c]		// 0x0004fb5c
		mov dword ptr [esp + 0x70], ecx		// 0x0004fb60
		mov ecx, dword ptr [esp + 0x54]		// 0x0004fb64
		mov dword ptr [esp + 0x74], edx		// 0x0004fb68
		mov edx, dword ptr [esi + 0x70]		// 0x0004fb6c
		mov dword ptr [esp + 0x78], ecx		// 0x0004fb6f
		mov eax, dword ptr [edx + eax*4]		// 0x0004fb73
		mov ecx, dword ptr [esi + 0x74]		// 0x0004fb76
		mov edx, dword ptr [ecx + 0x14]		// 0x0004fb79
		mov ebp, dword ptr [ecx + 0x10]		// 0x0004fb7c
		and eax, 0x3fffffff		// 0x0004fb7f
		lea eax, [eax + eax*2]		// 0x0004fb84
		mov ecx, dword ptr [edx + eax*4]		// 0x0004fb87
		lea eax, [edx + eax*4]		// 0x0004fb8a
		lea ecx, [ecx + ecx*2]		// 0x0004fb8d
		lea edi, [ebp + ecx*4]		// 0x0004fb90
		mov ecx, dword ptr [eax + 4]		// 0x0004fb94
		mov eax, dword ptr [eax + 8]		// 0x0004fb97
		lea edx, [ecx + ecx*2]		// 0x0004fb9a
		lea ebx, [ebp + edx*4]		// 0x0004fb9d
		lea ecx, [esp + 0x24]		// 0x0004fba1
		push ecx		// 0x0004fba5
		lea eax, [eax + eax*2]		// 0x0004fba6
		lea edx, [esp + 0x94]		// 0x0004fba9
		push edx		// 0x0004fbb0
		lea ebp, [ebp + eax*4]		// 0x0004fbb1
		push ebp		// 0x0004fbb5
		push ebx		// 0x0004fbb6
		lea eax, [esp + 0x80]		// 0x0004fbb7
		push edi		// 0x0004fbbe
		push eax		// 0x0004fbbf
		call nxPMapPointTriangleSquareDistance		// 0x0004fbc0
		fstp st(0)		// 0x0004fbc5
		fld dword ptr [nxPMapOne]		// 0x0004fbc7
		mov ecx, dword ptr [esi + 0x70]		// 0x0004fbcd
		fsub dword ptr [esp + 0xa8]		// 0x0004fbd0
		mov edx, dword ptr [esp + 0x44]		// 0x0004fbd7
		mov eax, dword ptr [ecx + edx*4]		// 0x0004fbdb
		add esp, 0x18		// 0x0004fbde
		fsub dword ptr [esp + 0x24]		// 0x0004fbe1
		fld dword ptr [esp + 0x90]		// 0x0004fbe5
		fmul dword ptr [ebx]		// 0x0004fbec
		fld st(1)		// 0x0004fbee
		fmul dword ptr [edi]		// 0x0004fbf0
		faddp st(1), st(0)		// 0x0004fbf2
		fld dword ptr [esp + 0x24]		// 0x0004fbf4
		fmul dword ptr [ebp]		// 0x0004fbf8
		faddp st(1), st(0)		// 0x0004fbfb
		fsub dword ptr [esp + 0x70]		// 0x0004fbfd
		fld st(1)		// 0x0004fc01
		fmul dword ptr [edi + 4]		// 0x0004fc03
		fld dword ptr [esp + 0x90]		// 0x0004fc06
		fmul dword ptr [ebx + 4]		// 0x0004fc0d
		faddp st(1), st(0)		// 0x0004fc10
		fld dword ptr [esp + 0x24]		// 0x0004fc12
		fmul dword ptr [ebp + 4]		// 0x0004fc16
		faddp st(1), st(0)		// 0x0004fc19
		fsub dword ptr [esp + 0x74]		// 0x0004fc1b
		fstp dword ptr [esp + 0x68]		// 0x0004fc1f
		fxch st(1)		// 0x0004fc23
		fmul dword ptr [edi + 8]		// 0x0004fc25
		fld dword ptr [esp + 0x90]		// 0x0004fc28
		fmul dword ptr [ebx + 8]		// 0x0004fc2f
		faddp st(1), st(0)		// 0x0004fc32
		fld dword ptr [esp + 0x24]		// 0x0004fc34
		fmul dword ptr [ebp + 8]		// 0x0004fc38
		faddp st(1), st(0)		// 0x0004fc3b
		fsub dword ptr [esp + 0x78]		// 0x0004fc3d
		fstp dword ptr [esp + 0x6c]		// 0x0004fc41
		fld dword ptr [esp + 0x28]		// 0x0004fc45
		fmul dword ptr [esp + 0x80]		// 0x0004fc49
		fmul dword ptr [esp + 0x84]		// 0x0004fc50
		fstp dword ptr [esp + 0x90]		// 0x0004fc57
		fmul dword ptr [esp + 0x90]		// 0x0004fc5e
		fld dword ptr [esp + 0x68]		// 0x0004fc65
		fmul dword ptr [esp + 0x90]		// 0x0004fc69
		fstp dword ptr [esp + 0x68]		// 0x0004fc70
		fld dword ptr [esp + 0x6c]		// 0x0004fc74
		fmul dword ptr [esp + 0x90]		// 0x0004fc78
		fstp dword ptr [esp + 0x6c]		// 0x0004fc7f
		shr eax, 0x1f		// 0x0004fc83
		not eax		// 0x0004fc86
		test al, 1		// 0x0004fc88
		je L4fcae		// 0x0004fc8a
		fadd dword ptr [esp + 0x40]		// 0x0004fc8c
		fstp dword ptr [esp + 0x40]		// 0x0004fc90
		fld dword ptr [esp + 0x68]		// 0x0004fc94
		fadd dword ptr [esp + 0x44]		// 0x0004fc98
		fstp dword ptr [esp + 0x44]		// 0x0004fc9c
		fld dword ptr [esp + 0x6c]		// 0x0004fca0
		fadd dword ptr [esp + 0x48]		// 0x0004fca4
		fstp dword ptr [esp + 0x48]		// 0x0004fca8
		jmp L4fccd		// 0x0004fcac
L4fcae:
		mov eax, dword ptr [esp + 0x8c]		// 0x0004fcae
		fadd dword ptr [eax]		// 0x0004fcb5
		fstp dword ptr [eax]		// 0x0004fcb7
		fld dword ptr [esp + 0x68]		// 0x0004fcb9
		fadd dword ptr [eax + 4]		// 0x0004fcbd
		fstp dword ptr [eax + 4]		// 0x0004fcc0
		fld dword ptr [esp + 0x6c]		// 0x0004fcc3
		fadd dword ptr [eax + 8]		// 0x0004fcc7
		fstp dword ptr [eax + 8]		// 0x0004fcca
L4fccd:
		mov eax, dword ptr [esp + 0x10]		// 0x0004fccd
		cmp eax, -1		// 0x0004fcd1
		je L4fe39		// 0x0004fcd4
		mov edx, dword ptr [esi + 0x70]		// 0x0004fcda
		mov eax, dword ptr [edx + eax*4]		// 0x0004fcdd
		mov ecx, dword ptr [esi + 0x74]		// 0x0004fce0
		mov edx, dword ptr [ecx + 0x14]		// 0x0004fce3
		mov ebp, dword ptr [ecx + 0x10]		// 0x0004fce6
		and eax, 0x3fffffff		// 0x0004fce9
		lea eax, [eax + eax*2]		// 0x0004fcee
		mov ecx, dword ptr [edx + eax*4]		// 0x0004fcf1
		lea eax, [edx + eax*4]		// 0x0004fcf4
		lea ecx, [ecx + ecx*2]		// 0x0004fcf7
		lea edi, [ebp + ecx*4]		// 0x0004fcfa
		mov ecx, dword ptr [eax + 4]		// 0x0004fcfe
		mov eax, dword ptr [eax + 8]		// 0x0004fd01
		lea edx, [ecx + ecx*2]		// 0x0004fd04
		lea ebx, [ebp + edx*4]		// 0x0004fd07
		lea ecx, [esp + 0x90]		// 0x0004fd0b
		push ecx		// 0x0004fd12
		lea eax, [eax + eax*2]		// 0x0004fd13
		lea edx, [esp + 0x30]		// 0x0004fd16
		push edx		// 0x0004fd1a
		lea ebp, [ebp + eax*4]		// 0x0004fd1b
		push ebp		// 0x0004fd1f
		push ebx		// 0x0004fd20
		lea eax, [esp + 0x68]		// 0x0004fd21
		push edi		// 0x0004fd25
		push eax		// 0x0004fd26
		call nxPMapPointTriangleSquareDistance		// 0x0004fd27
		fstp st(0)		// 0x0004fd2c
		fld dword ptr [nxPMapOne]		// 0x0004fd2e
		mov ecx, dword ptr [esi + 0x70]		// 0x0004fd34
		fsub dword ptr [esp + 0x44]		// 0x0004fd37
		mov edx, dword ptr [esp + 0x28]		// 0x0004fd3b
		mov eax, dword ptr [ecx + edx*4]		// 0x0004fd3f
		shr eax, 0x1f		// 0x0004fd42
		fsub dword ptr [esp + 0xa8]		// 0x0004fd45
		not eax		// 0x0004fd4c
		fld dword ptr [esp + 0x44]		// 0x0004fd4e
		add esp, 0x18		// 0x0004fd52
		test al, 1		// 0x0004fd55
		fmul dword ptr [ebx]		// 0x0004fd57
		fld dword ptr [esp + 0x90]		// 0x0004fd59
		fmul dword ptr [ebp]		// 0x0004fd60
		faddp st(1), st(0)		// 0x0004fd63
		fld st(1)		// 0x0004fd65
		fmul dword ptr [edi]		// 0x0004fd67
		faddp st(1), st(0)		// 0x0004fd69
		fsub dword ptr [esp + 0x58]		// 0x0004fd6b
		fld dword ptr [esp + 0x90]		// 0x0004fd6f
		fmul dword ptr [ebp + 4]		// 0x0004fd76
		fld st(2)		// 0x0004fd79
		fmul dword ptr [edi + 4]		// 0x0004fd7b
		faddp st(1), st(0)		// 0x0004fd7e
		fld dword ptr [esp + 0x2c]		// 0x0004fd80
		fmul dword ptr [ebx + 4]		// 0x0004fd84
		faddp st(1), st(0)		// 0x0004fd87
		fsub dword ptr [esp + 0x5c]		// 0x0004fd89
		fstp dword ptr [esp + 0x68]		// 0x0004fd8d
		fld dword ptr [esp + 0x90]		// 0x0004fd91
		fmul dword ptr [ebp + 8]		// 0x0004fd98
		fxch st(2)		// 0x0004fd9b
		fmul dword ptr [edi + 8]		// 0x0004fd9d
		faddp st(2), st(0)		// 0x0004fda0
		fld dword ptr [esp + 0x2c]		// 0x0004fda2
		fmul dword ptr [ebx + 8]		// 0x0004fda6
		faddp st(2), st(0)		// 0x0004fda9
		fxch st(1)		// 0x0004fdab
		fsub dword ptr [esp + 0x60]		// 0x0004fdad
		fstp dword ptr [esp + 0x6c]		// 0x0004fdb1
		fld dword ptr [esp + 0x88]		// 0x0004fdb5
		fmul dword ptr [esp + 0x80]		// 0x0004fdbc
		fmul dword ptr [esp + 0x84]		// 0x0004fdc3
		fstp dword ptr [esp + 0x88]		// 0x0004fdca
		fmul dword ptr [esp + 0x88]		// 0x0004fdd1
		fld dword ptr [esp + 0x68]		// 0x0004fdd8
		fmul dword ptr [esp + 0x88]		// 0x0004fddc
		fstp dword ptr [esp + 0x68]		// 0x0004fde3
		fld dword ptr [esp + 0x6c]		// 0x0004fde7
		fmul dword ptr [esp + 0x88]		// 0x0004fdeb
		fstp dword ptr [esp + 0x6c]		// 0x0004fdf2
		je L4fe1a		// 0x0004fdf6
		fadd dword ptr [esp + 0x40]		// 0x0004fdf8
		fstp dword ptr [esp + 0x40]		// 0x0004fdfc
		fld dword ptr [esp + 0x68]		// 0x0004fe00
		fadd dword ptr [esp + 0x44]		// 0x0004fe04
		fstp dword ptr [esp + 0x44]		// 0x0004fe08
		fld dword ptr [esp + 0x6c]		// 0x0004fe0c
		fadd dword ptr [esp + 0x48]		// 0x0004fe10
		fstp dword ptr [esp + 0x48]		// 0x0004fe14
		jmp L4fe39		// 0x0004fe18
L4fe1a:
		mov eax, dword ptr [esp + 0x8c]		// 0x0004fe1a
		fadd dword ptr [eax]		// 0x0004fe21
		fstp dword ptr [eax]		// 0x0004fe23
		fld dword ptr [esp + 0x68]		// 0x0004fe25
		fadd dword ptr [eax + 4]		// 0x0004fe29
		fstp dword ptr [eax + 4]		// 0x0004fe2c
		fld dword ptr [esp + 0x6c]		// 0x0004fe2f
		fadd dword ptr [eax + 8]		// 0x0004fe33
		fstp dword ptr [eax + 8]		// 0x0004fe36
L4fe39:
		mov edx, dword ptr [esp + 0x8c]		// 0x0004fe39
		fld dword ptr [esp + 0x40]		// 0x0004fe40
		fadd dword ptr [edx]		// 0x0004fe44
		pop edi		// 0x0004fe46
		pop esi		// 0x0004fe47
		pop ebp		// 0x0004fe48
		fst dword ptr [edx]		// 0x0004fe49
		pop ebx		// 0x0004fe4b
		fld dword ptr [esp + 0x34]		// 0x0004fe4c
		fadd dword ptr [edx + 4]		// 0x0004fe50
		fst dword ptr [edx + 4]		// 0x0004fe53
		fld dword ptr [esp + 0x38]		// 0x0004fe56
		fadd dword ptr [edx + 8]		// 0x0004fe5a
		fst dword ptr [edx + 8]		// 0x0004fe5d
		fxch st(2)		// 0x0004fe60
		fmul dword ptr [esp + 0x30]		// 0x0004fe62
		fld dword ptr [esp + 0x34]		// 0x0004fe66
		fmul st(0), st(2)		// 0x0004fe6a
		faddp st(1), st(0)		// 0x0004fe6c
		fld dword ptr [esp + 0x38]		// 0x0004fe6e
		fmul st(0), st(3)		// 0x0004fe72
		faddp st(1), st(0)		// 0x0004fe74
		fcomp dword ptr [nxPMapZero]		// 0x0004fe76
		fnstsw ax		// 0x0004fe7c
		fstp st(1)		// 0x0004fe7e
		test ah, 5		// 0x0004fe80
		fstp st(0)		// 0x0004fe83
		jnp L4ff04		// 0x0004fe85
		mov eax, dword ptr [esp + 0x84]		// 0x0004fe87
		test eax, eax		// 0x0004fe8e
		je L4ff0c		// 0x0004fe90
		mov ecx, dword ptr [esp + 0x88]		// 0x0004fe92
		test ecx, ecx		// 0x0004fe99
		je L4fedd		// 0x0004fe9b
		fld dword ptr [ecx + 8]		// 0x0004fe9d
		fmul dword ptr [eax + 8]		// 0x0004fea0
		fld dword ptr [ecx + 4]		// 0x0004fea3
		fmul dword ptr [eax + 4]		// 0x0004fea6
		faddp st(1), st(0)		// 0x0004fea9
		fld dword ptr [eax]		// 0x0004feab
		fmul dword ptr [ecx]		// 0x0004fead
		faddp st(1), st(0)		// 0x0004feaf
		fld dword ptr [ecx + 0xc]		// 0x0004feb1
		fmul dword ptr [eax]		// 0x0004feb4
		fld dword ptr [ecx + 0x14]		// 0x0004feb6
		fmul dword ptr [eax + 8]		// 0x0004feb9
		faddp st(1), st(0)		// 0x0004febc
		fld dword ptr [ecx + 0x10]		// 0x0004febe
		fmul dword ptr [eax + 4]		// 0x0004fec1
		faddp st(1), st(0)		// 0x0004fec4
		fld dword ptr [ecx + 0x18]		// 0x0004fec6
		fmul dword ptr [eax]		// 0x0004fec9
		fld dword ptr [ecx + 0x20]		// 0x0004fecb
		fmul dword ptr [eax + 8]		// 0x0004fece
		faddp st(1), st(0)		// 0x0004fed1
		fld dword ptr [ecx + 0x1c]		// 0x0004fed3
		fmul dword ptr [eax + 4]		// 0x0004fed6
		faddp st(1), st(0)		// 0x0004fed9
		jmp L4fee5		// 0x0004fedb
L4fedd:
		fld dword ptr [eax]		// 0x0004fedd
		fld dword ptr [eax + 4]		// 0x0004fedf
		fld dword ptr [eax + 8]		// 0x0004fee2
L4fee5:
		fxch st(1)		// 0x0004fee5
		fmul dword ptr [edx + 4]		// 0x0004fee7
		fxch st(1)		// 0x0004feea
		fmul dword ptr [edx + 8]		// 0x0004feec
		faddp st(1), st(0)		// 0x0004feef
		fxch st(1)		// 0x0004fef1
		fmul dword ptr [edx]		// 0x0004fef3
		faddp st(1), st(0)		// 0x0004fef5
		fcomp dword ptr [nxPMapZero]		// 0x0004fef7
		fnstsw ax		// 0x0004fefd
		test ah, 0x41		// 0x0004feff
		jne L4ff0c		// 0x0004ff02
L4ff04:
		xor eax, eax		// 0x0004ff04
		add esp, 0x6c		// 0x0004ff06
		ret 0x1c		// 0x0004ff09
L4ff0c:
		mov eax, 1		// 0x0004ff0c
		add esp, 0x6c		// 0x0004ff11
		ret 0x1c		// 0x0004ff14
		}
	}

static double nxPMapTriangleDistance(const InternalTriangleMesh& mesh,
	const NxF32 point[3], NxU32 face)
	{
	const NxVec3* vertices = static_cast<const NxVec3*>(mesh.mVertices);
	const NxU32* triangles = static_cast<const NxU32*>(mesh.mTriangles);
	const NxVec3& a = vertices[triangles[face * 3 + 0]];
	const NxVec3& b = vertices[triangles[face * 3 + 1]];
	const NxVec3& c = vertices[triangles[face * 3 + 2]];
	const IceMaths::Point query(point[0], point[1], point[2]);
	const IceMaths::Point p0(a.x, a.y, a.z);
	const IceMaths::Point p1(b.x, b.y, b.z);
	const IceMaths::Point p2(c.x, c.y, c.z);
	const NxF32 queryValues[3] = { query.x, query.y, query.z };
	const NxF32 p0Values[3] = { p0.x, p0.y, p0.z };
	const NxF32 p1Values[3] = { p1.x, p1.y, p1.z };
	const NxF32 p2Values[3] = { p2.x, p2.y, p2.z };
	return nxPMapPointTriangleSquareDistance(queryValues, p0Values, p1Values, p2Values, 0, 0);
	}

static double nxPMapPointAABBDistanceSquared(const IceMaths::Point& point,
	const Opcode::CollisionAABB& bounds)
	{
	const IceMaths::Point delta = point - bounds.mCenter;
	const IceMaths::Point& extents = bounds.mExtents;
	double dx = 0.0;
	double dy = 0.0;
	double dz = 0.0;
	if(delta.x < -extents.x) dx = static_cast<double>(delta.x) + extents.x;
	else if(delta.x > extents.x) dx = static_cast<double>(delta.x) - extents.x;
	if(delta.y < -extents.y) dy = static_cast<double>(delta.y) + extents.y;
	else if(delta.y > extents.y) dy = static_cast<double>(delta.y) - extents.y;
	if(delta.z < -extents.z) dz = static_cast<double>(delta.z) + extents.z;
	else if(delta.z > extents.z) dz = static_cast<double>(delta.z) - extents.z;
	return (dx * dx + dy * dy) + dz * dz;
	}

static void nxPMapNearestNoLeafNode(const InternalTriangleMesh& mesh,
	const IceMaths::Point& point, const Opcode::AABBNoLeafNode* node,
	double& bestDistanceSquared, NxU32& faceOut)
	{
	if(!node || nxPMapPointAABBDistanceSquared(point, node->mAABB) > bestDistanceSquared)
		return;
	const NxU32 children[2] = { node->mPosData, node->mNegData };
	for(unsigned child = 0; child < 2; ++child)
		{
		if(children[child] & 1)
			{
			const NxU32 face = children[child] >> 1;
			const NxF32 pointValues[3] = { point.x, point.y, point.z };
			const double distanceSquared = nxPMapTriangleDistance(mesh, pointValues, face);
			if(distanceSquared < bestDistanceSquared)
				{
				bestDistanceSquared = static_cast<NxF32>(distanceSquared);
				faceOut = face;
				}
			}
		else
			nxPMapNearestNoLeafNode(mesh, point,
				reinterpret_cast<const Opcode::AABBNoLeafNode*>(static_cast<size_t>(children[child])),
				bestDistanceSquared, faceOut);
		}
	}

// Match the oracle's point-distance collider: test each no-leaf node's AABB,
// visit its positive child before its negative child, and prune against the
// best squared distance. A flat face scan changes which equal-distance face
// wins because it ignores the tree's data-dependent traversal order.
static void nxPMapNearestFace(const InternalTriangleMesh& mesh, const NxF32 point[3],
	const Opcode::Model& model, const NxU32* faceOrder, NxU32& faceOut,
	NxF32& distanceSquaredOut)
	{
	double bestDistanceSquared = 3.402823466e+38F;
	faceOut = 0;
	if(!model.HasLeafNodes() && !model.IsQuantized() && !model.HasSingleNode())
		{
		const Opcode::AABBNoLeafTree* tree =
			static_cast<const Opcode::AABBNoLeafTree*>(model.GetTree());
		if(tree && tree->GetNodes())
			nxPMapNearestNoLeafNode(mesh, IceMaths::Point(point[0], point[1], point[2]),
				tree->GetNodes(), bestDistanceSquared, faceOut);
		}
	else
		for(NxU32 order = 0; order < mesh.mTriangleCount; ++order)
			{
			const NxU32 face = faceOrder[order];
			const double distanceSquared = nxPMapTriangleDistance(mesh, point, face);
			if(distanceSquared < bestDistanceSquared)
				{
				bestDistanceSquared = static_cast<NxF32>(distanceSquared);
				faceOut = face;
				}
			}
	distanceSquaredOut = static_cast<NxF32>(bestDistanceSquared);
	}

// The oracle keeps the multiply, subtract, and add in the x87 register stack,
// then rounds the final coordinate when it stores the point. Volatile float
// temporaries round too early and shift surface-adjacent ray origins.
static __declspec(noinline) NxF32 nxPMapCellCoordinate(NxU32 index, NxF32 unitsPerCell,
	NxF32 halfExtent, NxF32 centre)
	{
#if NX_PHYSICS_USE_X87
    return (static_cast<NxF32>(index) * unitsPerCell - halfExtent) + centre;
#else
    return static_cast<NxF32>((static_cast<NxF32>(index) * double(unitsPerCell) - halfExtent) + centre);
#endif
	}

// ---------------------------------------------------------------------------

// phys_fn_002045 at 0x000505f0. The AABB is seeded with FLT_MAX/-FLT_MAX and
// every word from +0x5c to +0x74 is cleared; +0x04 is NOT cleared here, it is
// written by the table build the constructor tail-calls at 0x0005062c.
PenetrationMap::PenetrationMap()
	{
	mMin[0] = mMin[1] = mMin[2] = 3.402823466e+38F;
	mMax[0] = mMax[1] = mMax[2] = -3.402823466e+38F;

	mResolution = 0;
	mResolutionSquared = 0;
	mLastIndex = 0.0f;
	mInvLastIndex = 0.0f;
	mCellCount = 0;
	mGrid = 0;
	mMesh = 0;

	// The constructor leaves the centre, extents and per-cell scales alone --
	// nothing between 0x000505f3 and 0x0005062c touches +0x20..+0x58 -- and so
	// does this one. They are written by setup() before anything reads them.
	mSpread = 0;
	buildSpreadTable();
	}

// phys_fn_001986 at 0x0004cb20. 256 dwords from the CRT heap, each entry the
// eight bits of its index spread three apart: bit i of the index lands at bit
// 3i. The seven `shl eax,2` and eight `and` pairs at 0x0004cb37..0x0004cb80 are
// that and nothing else, and the sort in finish() is the only reader. The image
// stores through the table pointer at 0x0004cb85 with no test after the
// allocation, so neither does this.
void PenetrationMap::buildSpreadTable()
	{
	mSpread = static_cast<NxU32*>(malloc(0x400));

	for(NxU32 i = 0; i < 0x100; ++i)
		{
		NxU32 spread = 0;
		for(NxU32 bit = 0; bit < 8; ++bit)
			if(i & (1u << bit))
				spread |= 1u << (bit * 3);
		mSpread[i] = spread;
		}
	}

// phys_fn_001984 at 0x0004cae0. The grid first, the spread table second, both
// through the CRT free at 0x000f48bb, and both pointers nulled after.
PenetrationMap::~PenetrationMap()
	{
	if(mGrid)
		{
		free(mGrid);
		mGrid = 0;
		}
	if(mSpread)
		{
		free(mSpread);
		mSpread = 0;
		}
	}

// ---------------------------------------------------------------------------

// phys_fn_002033 at 0x0004ff80.
//
// NOTHING IS VALIDATED. There is no minimum, no maximum and no length field: a
// resolution of 0 allocates a zero-cell grid and a resolution of 0x400 asks for
// 4 GB, and both take the same path. The return value is `grid != 0`
// (`setne al` at 0x000500f2 and 0x00050104) and Create ignores it.
bool PenetrationMap::setup(NxU32 resolution, const NxF32* bounds)
	{
	mResolution = resolution;
	mResolutionSquared = resolution * resolution;

	// `lea eax,[ecx-1]` then `fild` at 0x0004ff9f, with the unsigned fixup at
	// 0x0004ffa5 that adds 2^32 when the value read as signed is negative --
	// which is `(NxF32)(NxU32)(resolution - 1)`.
	mLastIndex = static_cast<NxF32>(resolution - 1);
	mInvLastIndex = 1.0f / mLastIndex;

	mMin[0] = bounds[0]; mMin[1] = bounds[1]; mMin[2] = bounds[2];
	mMax[0] = bounds[3]; mMax[1] = bounds[4]; mMax[2] = bounds[5];

	for(int axis = 0; axis < 3; ++axis)
		{
#if NX_PHYSICS_USE_X87
        mCentre[axis] = (mMax[axis] + mMin[axis]) * 0.5f;
        mHalfExtents[axis] = (mMax[axis] - mMin[axis]) * 0.5f;
#else
        mCentre[axis] = static_cast<NxF32>((double(mMax[axis]) + mMin[axis]) * 0.5);
        mHalfExtents[axis] = static_cast<NxF32>((double(mMax[axis]) - mMin[axis]) * 0.5);
#endif
		mExtents[axis] = mMax[axis] - mMin[axis];
		}
	for(int axis = 0; axis < 3; ++axis)
		{
		mCellsPerUnit[axis] = mLastIndex / mExtents[axis];
		// The oracle recomputes 1/lastIndex for this product; it does not reuse
		// the float-rounded mInvLastIndex stored at +0x68. Keeping the reciprocal
		// in the x87 register through the multiply avoids a one-ULP grid-scale
		// difference on resolutions such as 64.
#if NX_PHYSICS_USE_X87
        mUnitsPerCell[axis] = (1.0f / mLastIndex) * mExtents[axis];
#else
        mUnitsPerCell[axis] = static_cast<NxF32>((1.0 / mLastIndex) * mExtents[axis]);
#endif
		}

	// `imul edi,ecx` at 0x000500b6 makes the third power out of the square, and
	// the store at 0x000500ca happens BEFORE the allocation, so a failed malloc
	// leaves the cell count set and the grid null.
	mCellCount = mResolutionSquared * resolution;
	mGrid = static_cast<NxU32*>(malloc(mCellCount * 4));
	if(mGrid)
		{
		// `rep stosd` with 0xffffffff at 0x000500ea, guarded by
		// `lea eax,[edi-1]; test eax,eax; jl` -- so a zero cell count writes
		// nothing rather than wrapping.
		for(NxU32 i = 0; i < mCellCount; ++i)
			mGrid[i] = 0xffffffffu;
		}
	return mGrid != 0;
	}

// ---------------------------------------------------------------------------

// phys_fn_002008 at 0x0004dba0.
//
// The first three instructions are the ONE place in this format where the
// resolution is interpreted: `cmp eax,0x20` -> 5, `cmp eax,0x40` -> 6,
// `cmp eax,0x50` -> 7 at 0x0004dbab, 0x0004dbb8 and 0x0004dbc4, defaulting to 0
// for every other resolution. So the cell-run encoding is defined for
// resolutions 32, 64 and 80 and for nothing else, while the header above accepts
// any resolution at all.
//
// Then a 32-bit element count MSB first, then the container is emptied
// (`[eax+4] = 0` at 0x0004dc13). Each element is a 5-bit code dispatched
// through the 32-way jump table at 0x0004dc75. Codes 0..25 move one step to a
// neighboring cell; codes 26..31 replace selected coordinates with absolute
// values using the resolution-dependent width above.
NxU32 PenetrationMap::decodeCellRun(MemoryStream& stream, IceCore::Container& cells,
	NxU32 resolution, PMapCellCursor& cursor)
	{
	NxU32 codeWidth = 0;
	if(resolution == 0x20)		codeWidth = 5;
	else if(resolution == 0x40)	codeWidth = 6;
	else if(resolution == 0x50)	codeWidth = 7;

	NxU32 count = stream.readBitsMsbFirst(32);
	cells.Reset();
	if(count == 0)
		return 0;

	// phys_fn_001988 initializes all three cursors to -1 before each value group.
	// Codes 0..25 are the 26 possible non-zero one-cell moves. Codes 26..31
	// replace one or more coordinates with an absolute value of codeWidth bits.
	for(NxU32 entry = 0; entry < count; ++entry)
		{
		const NxU32 code = stream.readBitsMsbFirst(5);
		if(code < 26)
			{
			cursor.x += kPMapCellSteps[code].x;
			cursor.y += kPMapCellSteps[code].y;
			cursor.z += kPMapCellSteps[code].z;
			}
		else
			{
			if(code == 26 || code == 29 || code == 30 || code == 31)
				cursor.x = static_cast<NxI32>(stream.readBitsMsbFirst(codeWidth));
			if(code == 27 || code == 29 || code == 31)
				cursor.y = static_cast<NxI32>(stream.readBitsMsbFirst(codeWidth));
			if(code == 28 || code == 30 || code == 31)
				cursor.z = static_cast<NxI32>(stream.readBitsMsbFirst(codeWidth));
			}

		const NxU32 index = (static_cast<NxU32>(cursor.z) * resolution +
			static_cast<NxU32>(cursor.y)) * resolution + static_cast<NxU32>(cursor.x);
		cells.Add(index);
		}
	return count;
	}

// phys_fn_001990 at 0x0004cc60. Each value group's cells are sorted by their
// Morton key, split into x/y/z, and represented as 5-bit neighbor codes. An
// escape carries only the coordinates that are not adjacent to the prior cell.

bool PenetrationMap::encodeCellRun(MemoryStream& stream, const NxU32* cells,
	NxU32 count, NxU32 resolution, const NxU32* spread, PMapCellCursor& cursor)
	{
	PMapMortonCell* ordered = count ? static_cast<PMapMortonCell*>(
		malloc(static_cast<size_t>(count) * sizeof(PMapMortonCell))) : 0;
	if(count && !ordered)
		return false;
	const NxU32 resolutionSquared = resolution * resolution;
	for(NxU32 i = 0; i < count; ++i)
		{
		const NxU32 index = cells[i];
		const NxU32 x = index % resolution;
		const NxU32 y = (index / resolution) % resolution;
		const NxU32 z = index / resolutionSquared;
		ordered[i].index = index;
		// phys_fn_001990 spreads z into the low three-bit lanes, y into the
		// middle lanes, and x into the high lanes (its bit loop shifts x by
		// bit*3+2 and z by bit*3).
		ordered[i].key = spread[z] + 2 * spread[y] + 4 * spread[x];
		}
	if(count > 1)
		qsort(ordered, count, sizeof(PMapMortonCell), comparePMapMortonCell);

	stream.storeBitsMsbFirst(count, 32);
	NxU32 codeWidth = 0;
	if(resolution == 0x20) codeWidth = 5;
	else if(resolution == 0x40) codeWidth = 6;
	else if(resolution == 0x50) codeWidth = 7;
	for(NxU32 i = 0; i < count; ++i)
		{
		const NxU32 index = ordered[i].index;
		const NxI32 x = static_cast<NxI32>(index % resolution);
		const NxI32 y = static_cast<NxI32>((index / resolution) % resolution);
		const NxI32 z = static_cast<NxI32>(index / resolutionSquared);
		const NxI32 dx = x - cursor.x;
		const NxI32 dy = y - cursor.y;
		const NxI32 dz = z - cursor.z;
		NxU32 code = 0xffffffffu;
		if(dx >= -1 && dx <= 1 && dy >= -1 && dy <= 1 && dz >= -1 && dz <= 1)
			for(NxU32 candidate = 0; candidate < 26; ++candidate)
				if(kPMapCellSteps[candidate].x == dx && kPMapCellSteps[candidate].y == dy &&
					kPMapCellSteps[candidate].z == dz)
					{
					code = candidate;
					break;
					}
		if(code == 0xffffffffu)
			{
			const bool changeX = dx != 0;
			const bool changeY = dy != 0;
			const bool changeZ = dz != 0;
			if(changeX && !changeY && !changeZ) code = 26;
			else if(!changeX && changeY && !changeZ) code = 27;
			else if(!changeX && !changeY && changeZ) code = 28;
			else if(changeX && changeY && !changeZ) code = 29;
			else if(changeX && !changeY && changeZ) code = 30;
			else code = 31; // Includes y/z-only escapes; there is no narrower code.
			}
		stream.storeBitsMsbFirst(code, 5);
		if(code == 26 || code == 29 || code == 30 || code == 31)
			stream.storeBitsMsbFirst(static_cast<NxU32>(x), codeWidth);
		if(code == 27 || code == 29 || code == 31)
			stream.storeBitsMsbFirst(static_cast<NxU32>(y), codeWidth);
		if(code == 28 || code == 30 || code == 31)
			stream.storeBitsMsbFirst(static_cast<NxU32>(z), codeWidth);
		cursor.x = x; cursor.y = y; cursor.z = z;
		}
	free(ordered);
	return true;
	}

// phys_fn_002017 at 0x0004e1a0. The non-empty cell ids are radix-sorted into
// equal-value groups; value ids use a repeat/delta bit followed by an absolute
// 32-bit value when the id is not consecutive. Each group's cell indices are
// then passed to phys_fn_001990, followed by the terminator and sign plane.
bool PenetrationMap::serialize(MemoryStream& stream) const
	{
	PMapValueCell* ordered = mCellCount ? static_cast<PMapValueCell*>(
		malloc(static_cast<size_t>(mCellCount) * sizeof(PMapValueCell))) : 0;
	NxU32* group = mCellCount ? static_cast<NxU32*>(
		malloc(static_cast<size_t>(mCellCount) * sizeof(NxU32))) : 0;
	if(mCellCount && (!ordered || !group))
		{
		free(ordered);
		free(group);
		return false;
		}
	for(NxU32 i = 0; i < mCellCount; ++i)
		{
		ordered[i].value = mGrid[i] & 0x3fffffffu;
		ordered[i].index = i;
		}
	if(mCellCount > 1)
		qsort(ordered, mCellCount, sizeof(PMapValueCell), comparePMapValueCell);

	stream.storeByte(kPMapTag[0]);
	stream.storeByte(kPMapTag[1]);
	stream.storeByte(kPMapTag[2]);
	stream.storeByte(kPMapTag[3]);
	stream.storeDword(kPMapVersion);
	stream.storeDword(mResolution);

	NxU32 previousValue = kPMapEmptyValue;
	NxU32 nextValue = 0;
	NxU32 groupCount = 0;
	PMapCellCursor cursor = { -1, -1, -1 };
	for(NxU32 i = 0; i < mCellCount; ++i)
		{
		const PMapValueCell& item = ordered[i];
		if(item.value == kPMapEmptyValue)
			continue;
		if(item.value != previousValue)
			{
			if(previousValue != kPMapEmptyValue)
				{
				if(previousValue == nextValue)
					stream.storeBit(1);
				else
					{
					stream.storeBit(0);
					stream.storeBitsMsbFirst(previousValue, 32);
					}
				if(!encodeCellRun(stream, groupCount ? group : 0,
					groupCount, mResolution, mSpread, cursor))
					{
					free(ordered);
					free(group);
					return false;
					}
				groupCount = 0;
				nextValue = previousValue + 1;
				}
			previousValue = item.value;
			}
		group[groupCount++] = item.index;
		}

	if(previousValue == nextValue)
		stream.storeBit(1);
	else
		{
		stream.storeBit(0);
		stream.storeBitsMsbFirst(previousValue, 32);
		}
	if(!encodeCellRun(stream, groupCount ? group : 0, groupCount, mResolution, mSpread, cursor))
		{
		free(ordered);
		free(group);
		return false;
		}

	// The end marker is the non-consecutive value 0xffffffff, then the one-bit
	// value prefix and its 32-bit payload, exactly as the decoder consumes it.
	stream.storeBit(0);
	stream.storeBitsMsbFirst(kPMapEndOfValues, 32);
	for(NxU32 i = 0; i < mCellCount; ++i)
		stream.storeBit((mGrid[i] & kPMapCellFilled) == 0 ? 1u : 0u);
	free(ordered);
	free(group);
	return true;
	}

// ---------------------------------------------------------------------------

// phys_fn_002035 at 0x00050110, the arm the loader takes when the caller supplies
// a stream. The filename arm at 0x00050123 and the build-your-own-stream arm at
// 0x0005017b are not reconstructed; see PMap.h.
bool PenetrationMap::loadPayload(MemoryStream& stream)
{
	// 0x00050157: the grid is refilled with 0xffffffff before the payload runs,
	// even though setup() has just done it. Reproduced because it is what the
	// row does, not because anything depends on it.
	//
	// GUARDED ON THE GRID, because setup() can return with a null one and a
	// non-zero count: its own comment records that the count is stored BEFORE the
	// allocation, so a failed malloc leaves `mCellCount` set and `mGrid` null, and
	// it returns `mGrid != 0` to say so. `create` ignores that return and comes
	// here, so the guard has to be the pointer. A count test would not do: the
	// count is exactly what is set when the allocation failed.
	if(mGrid)
		{
		for(NxU32 i = 0; i < mCellCount; ++i)
			mGrid[i] = 0xffffffffu;
		}

	// The value loop from 0x000501b0. `next` is the running previous-plus-one:
	// `mov ebp,edi; inc edi` at 0x0005020a, with edi zeroed once at 0x000501b0
	// and never reset, so a set flag bit means "the value before this one, plus
	// one" and the first record's implicit predecessor is -1.
	NxU32 next = 0;
	PMapCellCursor cursor = { -1, -1, -1 };
	for(;;)
		{
		NxU32 value;
		if(stream.readBit())
			value = next;
		else
			value = stream.readBitsMsbFirst(32);
		next = value + 1;

		if(value == kPMapEndOfValues)
			break;

		IceCore::Container cells;
		NxU32 count = decodeCellRun(stream, cells, mResolution, cursor);
		const udword* entries = cells.GetEntries();
		for(NxU32 i = 0; i < count; ++i)
			mGrid[entries[i]] = value;
		}

	// The sign block from 0x00050260: one bit per cell, and a CLEAR bit sets the
	// top bit of that cell. `setne cl; test cl,cl; jne` at 0x0005027b skips the
	// OR when the bit is set, which is the inverse of what it reads like.
	for(NxU32 i = 0; i < mCellCount; ++i)
		if(stream.readBit() == 0)
			mGrid[i] |= kPMapCellFilled;

	return true;
	}

// ---------------------------------------------------------------------------

// phys_fn_002037 at 0x000502d0 and its three continuations. Two passes.
//
// PASS ONE, 0x000502f0-0x0005049f. For every cell it takes the EIGHT corners of
// the unit cube whose low corner it is -- a, a+1, a+n, a+n+1, a+n*n, a+n*n+1,
// a+n*n+n, a+n*n+n+1, assembled at 0x00050310-0x0005034d -- drops the ones that
// would leave the grid (0x0005035a for k, 0x00050379 for j, 0x0005038c for i,
// each writing -1 into the four indices that axis invalidates) and sets bit 30
// of the cell when EVERY surviving corner has bit 31 set.
//
// The sense is worth being exact about. Each test is
// `shr edx,0x1f; not edx; test dl,1; jne <skip>`: bit 31 clear gives ~0 and
// takes the jump, bit 31 set gives ~1 and falls through. So the OR at 0x00050466
// happens when all eight corners are SET, not when they are clear.
//
// PASS TWO, 0x000504a5-0x000505d9. Builds one key per cell out of the spread
// table -- `table[k] + 2*table[j] + 4*table[i]` at 0x000504dc/0x000504df, which
// interleaves the three indices into a Morton code -- radix-sorts them, and
// permutes the grid into that order: newGrid[i] = grid[ranks[i]] at 0x00050599,
// copied back at 0x000505b6.
bool PenetrationMap::finish()
	{
	const NxU32 n = mResolution;
	const NxU32 nn = mResolutionSquared;

	for(NxU32 i = 0; i < n; ++i)
		for(NxU32 j = 0; j < n; ++j)
			for(NxU32 k = 0; k < n; ++k)
				{
				const NxU32 a = i * nn + j * n + k;
				NxI32 corner[8];
				corner[0] = static_cast<NxI32>(a);
				corner[1] = static_cast<NxI32>(a + 1);
				corner[2] = static_cast<NxI32>(a + n);
				corner[3] = static_cast<NxI32>(a + n + 1);
				corner[4] = static_cast<NxI32>(a + nn);
				corner[5] = static_cast<NxI32>(a + nn + 1);
				corner[6] = static_cast<NxI32>(a + nn + n);
				corner[7] = static_cast<NxI32>(a + nn + n + 1);
				if(k == n - 1)
					corner[1] = corner[3] = corner[5] = corner[7] = -1;
				if(j == n - 1)
					corner[2] = corner[3] = corner[6] = corner[7] = -1;
				if(i == n - 1)
					corner[4] = corner[5] = corner[6] = corner[7] = -1;

				bool interior = true;
				for(int c = 0; c < 8 && interior; ++c)
					if(corner[c] != -1 && (mGrid[corner[c]] & kPMapCellFilled) == 0)
						interior = false;
				if(interior)
					mGrid[a] |= kPMapCellInterior;
				}

	IceCore::Container keys;
	for(NxU32 i = 0; i < n; ++i)
		for(NxU32 j = 0; j < n; ++j)
			for(NxU32 k = 0; k < n; ++k)
				keys.Add(mSpread[k] + 2 * mSpread[j] + 4 * mSpread[i]);

	RadixSort sorter;
	const udword* ranks = sorter.Sort(keys.GetEntries(), keys.GetNbEntries(), RADIX_SIGNED).GetRanks();

	NxU32* sorted = static_cast<NxU32*>(malloc(mCellCount * 4));
	if(sorted)
		{
		for(NxU32 i = 0; i < mCellCount; ++i)
			sorted[i] = 0xffffffffu;
		for(NxU32 i = 0; i < mCellCount; ++i)
			sorted[i] = mGrid[ranks[i]];
		memcpy(mGrid, sorted, mCellCount * 4);
		free(sorted);
		}
	return true;
	}

// ---------------------------------------------------------------------------

// phys_fn_002047 at 0x00050640, the LOAD arm.
bool PenetrationMap::create(const void* mesh, NxU32 resolution, const char* filename,
	MemoryStream* stream, bool load, NxUserOutputStream* outputStream)
	{
	// 0x00050652. A null mesh is refused before anything else, and the refusal
	// is silent when there is no output stream to report through: every one of
	// the three sites tests it first.
	if(!mesh)
		{
		if(outputStream)
			outputStream->reportError(NXE_INVALID_PARAMETER, kPMapNoMeshMessage,
				NX_PENETRATION_MAP_CPP, kPMapNoMeshLine);
		return false;
		}

	// 0x0005067f and 0x0005068a: the header is only read when BOTH a stream and
	// the load flag are present. Otherwise the resolution argument stands and
	// the COMPUTE arm runs.
	if(stream && load)
		{
		for(int i = 0; i < 4; ++i)
			if(stream->readByte() != kPMapTag[i])
				{
				if(outputStream)
					outputStream->reportError(NXE_INTERNAL_ERROR, kPMapBadHeaderMessage,
						NX_PENETRATION_MAP_CPP, kPMapBadHeaderLine);
				return false;
				}

		if(stream->readDword() != kPMapVersion)
			{
			if(outputStream)
				outputStream->reportError(NXE_INTERNAL_ERROR, kPMapBadVersionMessage,
					NX_PENETRATION_MAP_CPP, kPMapBadVersionLine);
			return false;
			}

		// 0x000506f7. The resolution comes out of the file and replaces the
		// argument -- `mov [ebp+0xc],ebx` at 0x000506fe.
		resolution = stream->readDword();
		}

	// 0x0005072d. The six floats at mesh+0x44 are the mesh's AABB.
	setup(resolution, reinterpret_cast<const NxF32*>(static_cast<const NxU8*>(mesh) + 0x44));
	mMesh = mesh;

	if(load && loadPayload(*stream))
		return finish();

	// 0x00050768. The routine's mesh argument is the concrete TriangleMesh
	// object: setup reads its bounds at +0x44, and the Opcode interface is built
	// from the counts and arrays at +0x08..+0x1c. The arrays also back the
	// per-voxel nearest-face query below.
	const TriangleMesh* sourceMesh = static_cast<const TriangleMesh*>(mesh);
	const InternalTriangleMesh* source = &sourceMesh->mInternal;
	if(!source->mVertices || !source->mTriangles || !source->mTriangleCount || !source->mModel ||
		!mGrid || !stream)
		return false;

	const NxVec3* vertices = static_cast<const NxVec3*>(source->mVertices);
	const NxU32* triangles = static_cast<const NxU32*>(source->mTriangles);
	NxU8* classified = static_cast<NxU8*>(malloc(mCellCount));
	if(!classified)
		return false;
	memset(classified, 0, mCellCount);

	// The oracle builds a short-lived Opcode model directly over the source
	// arrays for this compute operation; it does not reuse the mesh's cached
	// model. Keep the interface alive until the local model is destroyed.
	Opcode::MeshInterface pmapInterface;
	pmapInterface.SetNbTriangles(source->mTriangleCount);
	pmapInterface.SetNbVertices(source->mVertexCount);
	if(!pmapInterface.SetPointers(static_cast<const IndexedTriangle*>(source->mTriangles),
		static_cast<const Point*>(source->mVertices)))
		{
		free(classified);
		return false;
		}
	Opcode::OPCODECREATE modelCreate;
	modelCreate.mIMesh = &pmapInterface;
	modelCreate.mNoLeaf = true;
	modelCreate.mQuantized = false;
	Opcode::Model pmapModel;
	if(!pmapModel.Build(modelCreate))
		{
		free(classified);
		return false;
		}
	NxU32* faceOrder = static_cast<NxU32*>(malloc(
		static_cast<size_t>(source->mTriangleCount) * sizeof(NxU32)));
	if(!faceOrder)
		{
		free(classified);
		return false;
		}
	NxU32 faceOrderCount = 0;
	if(pmapModel.HasSingleNode())
		faceOrder[faceOrderCount++] = 0;
	else if(!pmapModel.HasLeafNodes() && pmapModel.IsQuantized())
		{
		const Opcode::AABBQuantizedNoLeafTree* tree =
			static_cast<const Opcode::AABBQuantizedNoLeafTree*>(pmapModel.GetTree());
		if(tree && tree->GetNodes())
			nxPMapCollectFaceOrder(tree->GetNodes(), faceOrder, faceOrderCount);
		}
	else if(!pmapModel.HasLeafNodes())
		{
		const Opcode::AABBNoLeafTree* tree =
			static_cast<const Opcode::AABBNoLeafTree*>(pmapModel.GetTree());
		if(tree && tree->GetNodes())
			nxPMapCollectFaceOrder(tree->GetNodes(), faceOrder, faceOrderCount);
		}
	else
		for(NxU32 face = 0; face < source->mTriangleCount; ++face)
			faceOrder[faceOrderCount++] = face;
	if(faceOrderCount != source->mTriangleCount)
		{
		free(faceOrder);
		free(classified);
		return false;
		}
	Opcode::RayCollider rayCollider;
	rayCollider.SetFirstContact(false);
	rayCollider.SetTemporalCoherence(false);
	rayCollider.SetCulling(false);
	const Opcode::Model& model = pmapModel;
	// The oracle's random-ray helper reads TriangleMesh+0x28, the cached mesh
	// model. Its nearest-distance query uses the temporary model above, but the
	// classifier does not.
	const Opcode::Model& rayModel = *static_cast<const Opcode::Model*>(source->mModel);

	for(NxU32 z = 0; z < mResolution; ++z)
		for(NxU32 y = 0; y < mResolution; ++y)
			for(NxU32 x = 0; x < mResolution; ++x)
				{
				const NxU32 index = z * mResolutionSquared + y * mResolution + x;
				NxF32 point[3] = {
					nxPMapCellCoordinate(x, mUnitsPerCell[0], mHalfExtents[0], mCentre[0]),
					nxPMapCellCoordinate(y, mUnitsPerCell[1], mHalfExtents[1], mCentre[1]),
					nxPMapCellCoordinate(z, mUnitsPerCell[2], mHalfExtents[2], mCentre[2])
					};
				NxF32 bestDistanceSquared = 3.402823466e+38F;
				double bestDistance = 3.402823466e+38F;
				NxU32 nearestFace = 0;
				nxPMapNearestFace(*source, point, pmapModel, faceOrder,
					nearestFace, bestDistanceSquared);
				bestDistance = bestDistanceSquared;
				NxF32 distance = static_cast<NxF32>(sqrt(static_cast<double>(bestDistanceSquared)));
				bool inside;
				if(classified[index])
					inside = classified[index] != 1;
				else
					{
					unsigned insideVotes = 0;
					// FUN_1004e5d0 is entered with EAX=3 by the oracle's PMap
					// loop. That mode casts one random ray and uses its parity.
					for(unsigned sample = 0; sample < 1; ++sample)
						{
						NxF32 direction[3];
						for(unsigned axis = 0; axis < 3; ++axis)
							{
							volatile NxF32 scaledRandom =
								static_cast<NxF32>(rand()) * 3.051851e-05f;
							direction[axis] = scaledRandom - 0.5f;
							}
						if(direction[0] != 0.0f || direction[1] != 0.0f || direction[2] != 0.0f)
							{
#if NX_PHYSICS_USE_X87
							const NxF32 one = 1.0f;
							// FUN_1004e5d0 sums z², y², then x² in extended x87
							// precision, and keeps 1/sqrt(sum) extended while storing
							// each normalized component back to its float slot.
							__asm
								{
								fld dword ptr [direction + 8]
								fmul st(0), st(0)
								fld dword ptr [direction + 4]
								fmul st(0), st(0)
								faddp st(1), st
								fld dword ptr [direction]
								fmul st(0), st(0)
								faddp st(1), st
								fsqrt
								fdivr dword ptr [one]
								fld dword ptr [direction]
								fmul st(0), st(1)
								fstp dword ptr [direction]
								fld dword ptr [direction + 4]
								fmul st(0), st(1)
								fstp dword ptr [direction + 4]
								fld dword ptr [direction + 8]
								fmul st(0), st(1)
								fstp dword ptr [direction + 8]
								fstp st(0)
								}
#else
							nxScalarNormalizeDirection3(direction);
#endif
							}
						const Ray ray(Point(point[0], point[1], point[2]),
							Point(direction[0], direction[1], direction[2]));
						if(rayCollider.Collide(ray, rayModel) && (rayCollider.GetNbIntersections() & 1))
							++insideVotes;
						}
					inside = insideVotes != 0;
					}
				classified[index] = static_cast<NxU8>(inside ? 2 : 1);
				if(inside)
					mGrid[index] = nearestFace;

#if NX_PHYSICS_USE_X87
                const NxI32 radius[3] = {
                    static_cast<NxI32>(nearbyint(distance * mCellsPerUnit[0])),
                    static_cast<NxI32>(nearbyint(distance * mCellsPerUnit[1])),
                    static_cast<NxI32>(nearbyint(distance * mCellsPerUnit[2]))
                    };
#else
                const NxI32 radius[3] = {
                    static_cast<NxI32>(nearbyint(double(distance) * mCellsPerUnit[0])),
                    static_cast<NxI32>(nearbyint(double(distance) * mCellsPerUnit[1])),
                    static_cast<NxI32>(nearbyint(double(distance) * mCellsPerUnit[2]))
                };
#endif
				const NxI32 low[3] = {
					static_cast<NxI32>(x) - radius[0], static_cast<NxI32>(y) - radius[1],
					static_cast<NxI32>(z) - radius[2]
					};
				const NxI32 high[3] = {
					static_cast<NxI32>(x) + radius[0], static_cast<NxI32>(y) + radius[1],
					static_cast<NxI32>(z) + radius[2]
					};
				for(NxI32 nz = low[2] < 0 ? 0 : low[2]; nz <= high[2] && nz < static_cast<NxI32>(mResolution); ++nz)
					for(NxI32 ny = low[1] < 0 ? 0 : low[1]; ny <= high[1] && ny < static_cast<NxI32>(mResolution); ++ny)
						for(NxI32 nx = low[0] < 0 ? 0 : low[0]; nx <= high[0] && nx < static_cast<NxI32>(mResolution); ++nx)
							{
							const NxU32 neighbor = static_cast<NxU32>(nz) * mResolutionSquared +
								static_cast<NxU32>(ny) * mResolution + static_cast<NxU32>(nx);
							if(classified[neighbor]) continue;
							volatile NxF32 dx = nxPMapCellCoordinate(static_cast<NxU32>(nx),
								mUnitsPerCell[0], mHalfExtents[0], mCentre[0]) - point[0];
							volatile NxF32 dy = nxPMapCellCoordinate(static_cast<NxU32>(ny),
								mUnitsPerCell[1], mHalfExtents[1], mCentre[1]) - point[1];
							volatile NxF32 dz = nxPMapCellCoordinate(static_cast<NxU32>(nz),
								mUnitsPerCell[2], mHalfExtents[2], mCentre[2]) - point[2];
							// FUN_10050640 tests z, then y, then x; reversing the sum can
							// change a propagated classification at the surface threshold.
#if NX_PHYSICS_USE_X87
                            if((dz * dz + dy * dy) + dx * dx < distance * distance)
#else
                            if((double(dz) * dz + double(dy) * dy) + double(dx) * dx < double(distance) * distance)
#endif
								classified[neighbor] = static_cast<NxU8>(inside ? 2 : 1);
							}
				}

	free(classified);

	// phys_fn_002025 marks the eight corners of any cell touching a surface
	// seed. A grid word whose filled bit is already set is either untouched
	// empty (0xffffffff) or was handled on an earlier cell.
	for(NxU32 z = 0; z < mResolution; ++z)
		for(NxU32 y = 0; y < mResolution; ++y)
			for(NxU32 x = 0; x < mResolution; ++x)
				{
				const NxU32 base = z * mResolutionSquared + y * mResolution + x;
				NxI32 corners[8] = {
					static_cast<NxI32>(base), static_cast<NxI32>(base + 1),
					static_cast<NxI32>(base + mResolution),
					static_cast<NxI32>(base + mResolution + 1),
					static_cast<NxI32>(base + mResolutionSquared),
					static_cast<NxI32>(base + mResolutionSquared + 1),
					static_cast<NxI32>(base + mResolutionSquared + mResolution),
					static_cast<NxI32>(base + mResolutionSquared + mResolution + 1)
					};
				if(x == mResolution - 1)
					corners[1] = corners[3] = corners[5] = corners[7] = -1;
				if(y == mResolution - 1)
					corners[2] = corners[3] = corners[6] = corners[7] = -1;
				if(z == mResolution - 1)
					corners[4] = corners[5] = corners[6] = corners[7] = -1;
				bool touchesSurface = false;
				for(unsigned c = 0; c < 8; ++c)
					if(corners[c] != -1 && !(mGrid[corners[c]] & kPMapCellFilled))
						touchesSurface = true;
				if(!touchesSurface) continue;
				for(unsigned c = 0; c < 8; ++c)
					{
					if(corners[c] == -1 || !(mGrid[corners[c]] & kPMapCellFilled)) continue;
					const NxU32 corner = static_cast<NxU32>(corners[c]);
					const NxU32 cx = corner % mResolution;
					const NxU32 cy = (corner / mResolution) % mResolution;
					const NxU32 cz = corner / mResolutionSquared;
#if NX_PHYSICS_USE_X87
                    const NxF32 point[3] = {
                        (static_cast<NxF32>(cx) * mUnitsPerCell[0] - mHalfExtents[0]) + mCentre[0],
                        (static_cast<NxF32>(cy) * mUnitsPerCell[1] - mHalfExtents[1]) + mCentre[1],
                        (static_cast<NxF32>(cz) * mUnitsPerCell[2] - mHalfExtents[2]) + mCentre[2]
                        };
#else
                    const NxF32 point[3] = {
                        nxPMapCellCoordinate(cx, mUnitsPerCell[0], mHalfExtents[0], mCentre[0]),
                        nxPMapCellCoordinate(cy, mUnitsPerCell[1], mHalfExtents[1], mCentre[1]),
                        nxPMapCellCoordinate(cz, mUnitsPerCell[2], mHalfExtents[2], mCentre[2])
                    };
#endif
					NxU32 face;
					NxF32 distanceSquared;
						nxPMapNearestFace(*source, point, pmapModel, faceOrder, face, distanceSquared);
					mGrid[corner] = face | kPMapCellFilled;
					}
				}
	free(faceOrder);
	const bool serialized = serialize(*stream);
	const bool finished = finish();
	return serialized && finished;
	}

// ---------------------------------------------------------------------------
// The exports.

// phys_fn_002049 at 0x00050f70. The public handle's first data word points to
// TriangleMesh; its InternalTriangleMesh begins at +0x08, matching the oracle's
// `mesh[1].__vftable` load followed by the member's +0x44 bounds access.
NX_C_EXPORT NXP_DLL_EXPORT bool NX_CALL_CONV NxCreatePMap(NxPMap& pmap,
	const NxTriangleMesh& mesh, NxU32 density, NxUserOutputStream* outputStream)
	{
	const NxU8* publicObject = reinterpret_cast<const NxU8*>(&mesh);
	const TriangleMesh* concrete = *reinterpret_cast<TriangleMesh* const*>(publicObject + 4);
	if(!concrete)
		return false;
	MemoryStream stream(0x1000, 0);
	PenetrationMap penetrationMap;
	if(!penetrationMap.create(concrete, density, 0, &stream, false, outputStream))
		return false;
	const NxU32 size = stream.getLength();
	void* data = malloc(size);
	if(!data)
		return false;
	stream.collapse(data);
	pmap.dataSize = size;
	pmap.data = data;
	return true;
	}

// phys_fn_002051 at 0x00051040, 32 bytes, every one of them driven.
//
// Three things a reimplementation gets wrong by default, and all three are
// measured by the asset differential:
//
//   * it frees `data` through the CRT free at 0x000f48bb, not through the SDK
//     allocator, because NxCreatePMap malloc'd it;
//   * it leaves `dataSize` ALONE. Nothing between 0x00051040 and 0x0005105f
//     writes [esi];
//   * it returns true unconditionally -- `mov al,1` at 0x0005105c is outside
//     the `test eax,eax` at 0x00051048 -- so it reports success for a PMap it
//     did not release.
NX_C_EXPORT NXP_DLL_EXPORT bool NX_CALL_CONV NxReleasePMap(NxPMap& pmap)
	{
	if(pmap.data)
		{
		free(pmap.data);
		pmap.data = 0;
		}
	return true;
	}
