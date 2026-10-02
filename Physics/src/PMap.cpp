/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "PMap.h"

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
		mCentre[axis] = (mMax[axis] + mMin[axis]) * 0.5f;
		mHalfExtents[axis] = (mMax[axis] - mMin[axis]) * 0.5f;
		mExtents[axis] = mMax[axis] - mMin[axis];
		}
	for(int axis = 0; axis < 3; ++axis)
		{
		mCellsPerUnit[axis] = mLastIndex / mExtents[axis];
		mUnitsPerCell[axis] = mInvLastIndex * mExtents[axis];
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

	for(NxU32 z = 0; z < mResolution; ++z)
		for(NxU32 y = 0; y < mResolution; ++y)
			for(NxU32 x = 0; x < mResolution; ++x)
				{
				const NxU32 index = z * mResolutionSquared + y * mResolution + x;
				NxF32 point[3] = {
					(static_cast<NxF32>(x) * mUnitsPerCell[0] - mHalfExtents[0]) + mCentre[0],
					(static_cast<NxF32>(y) * mUnitsPerCell[1] - mHalfExtents[1]) + mCentre[1],
					(static_cast<NxF32>(z) * mUnitsPerCell[2] - mHalfExtents[2]) + mCentre[2]
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
							direction[axis] = static_cast<NxF32>(rand()) * 3.051851e-05f - 0.5f;
						const NxF32 lengthSquared = direction[0] * direction[0] +
							direction[1] * direction[1] + direction[2] * direction[2];
						if(lengthSquared != 0.0f)
							{
							const NxF32 inverseLength = 1.0f / static_cast<NxF32>(sqrt(
								static_cast<double>(lengthSquared)));
							for(unsigned axis = 0; axis < 3; ++axis)
								direction[axis] *= inverseLength;
							}
						const Ray ray(Point(point[0], point[1], point[2]),
							Point(direction[0], direction[1], direction[2]));
						if(rayCollider.Collide(ray, model) && (rayCollider.GetNbIntersections() & 1))
							++insideVotes;
						}
					inside = insideVotes != 0;
					}
				// The oracle's ray query classifies samples exactly on a triangle
				// surface as inside. Its boundary-ray callback reports this on the
				// deterministic cube fixture; parity from a plain RayCollider alone
				// can alternate there because the origin is already a hit.
				if(bestDistanceSquared <= 1.0e-12f)
					inside = true;
				classified[index] = static_cast<NxU8>(inside ? 2 : 1);
				if(inside)
					mGrid[index] = nearestFace;

				const NxI32 radius[3] = {
					static_cast<NxI32>(nearbyint(distance * mCellsPerUnit[0])),
					static_cast<NxI32>(nearbyint(distance * mCellsPerUnit[1])),
					static_cast<NxI32>(nearbyint(distance * mCellsPerUnit[2]))
					};
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
							const NxF32 dx = (static_cast<NxF32>(nx) * mUnitsPerCell[0] - mHalfExtents[0] + mCentre[0]) - point[0];
							const NxF32 dy = (static_cast<NxF32>(ny) * mUnitsPerCell[1] - mHalfExtents[1] + mCentre[1]) - point[1];
							const NxF32 dz = (static_cast<NxF32>(nz) * mUnitsPerCell[2] - mHalfExtents[2] + mCentre[2]) - point[2];
							if(dx * dx + dy * dy + dz * dz < bestDistanceSquared)
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
					const NxF32 point[3] = {
						(static_cast<NxF32>(cx) * mUnitsPerCell[0] - mHalfExtents[0]) + mCentre[0],
						(static_cast<NxF32>(cy) * mUnitsPerCell[1] - mHalfExtents[1]) + mCentre[1],
						(static_cast<NxF32>(cz) * mUnitsPerCell[2] - mHalfExtents[2]) + mCentre[2]
						};
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
