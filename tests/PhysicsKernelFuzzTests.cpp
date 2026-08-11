// The randomized kernel differential.
//
// Why this exists, separately from NxPhysicsGeometryTests. That harness runs a
// hand-authored matrix of 299 cases, and the matrix is blind to two properties
// of these kernels that the disassembly says are real:
//
//   * the floating-point model. All 106 mass and inertia cases pass under an
//     evaluation that rounds to 32 bits after every operation, and the shipped
//     DLL does not do that -- it is x87 with the process's _PC_53 precision
//     control, so intermediates carry a 53-bit significand until they are
//     stored. Measured: NxComputeSphereMass(0x40505969, 0x404c999e) returns
//     0x43e70125 from the shipped export and 0x43e70124 from a float32
//     evaluation. Two more inputs below separate the two models the same way,
//     and two controls agree under both so a difference cannot be read out of a
//     broken instrument.
//   * NaN payload propagation. When an operation has two NaN operands, x87
//     yields the one with the larger significand and SSE yields the first
//     source. Reproducing that is why Geometry.cpp and MassProperties.cpp are
//     built /arch:IA32.
//
// Both were found here and neither can be found by the case matrix. The first
// transcription of MassProperties.cpp, which used NxReal temporaries where the
// oracle keeps an x87 register live, passed all 299 matrix cases and was
// rejected by this harness on 525,995 of 4,800,000 checks -- every one of them
// a single ulp.
//
// How to read the transcript. Nothing here decides whether a result is right:
// this prints a digest of every value each export returned, and
// run_differential.ps1 compares the digest from the shipped pair against the
// digest from the rebuilt pair. A digest that matches means every one of the
// checks behind it agreed, bit for bit, including the NaN payloads.
//
// Reproducibility. The generator is a fixed xorshift32 with the seeds named at
// each block, and the iteration counts are the constants below, so a reader who
// builds this target and runs run_differential.ps1 gets the numbers this
// transcript reports and not merely numbers of the same shape.

#include "PhysicsPairLoader.h"

#include <math.h>
#include <string.h>

#include "Nxf.h"

// The two blocks of ordinary random inputs, and the aimed ray/triangle block.
// Chosen so a full run is a few seconds per pair rather than a few minutes; the
// counts, not the wall clock, are what a reader reproduces.
static const unsigned kScalarIterations = 120000;
static const unsigned kVectorIterations = 40000;
static const unsigned kAimedIterations = 60000;
static const unsigned kBoxIterations = 40000;
static const unsigned kAimedBoxIterations = 40000;
static const unsigned kCapsuleIterations = 30000;
static const unsigned kSatIterations = 20000;
static const unsigned kNormalsIterations = 4000;

// FNV-1a, 64 bit, over the raw result words. Every export folds its return
// value and every output word it wrote into one of these, so one line of
// transcript stands for every check behind it.
struct NxDigest
	{
	unsigned __int64 state;
	unsigned checks;
	};

static void nxDigestInit(NxDigest* d)
	{
	d->state = 0xcbf29ce484222325ULL;
	d->checks = 0;
	}

static void nxDigestWord(NxDigest* d, NxU32 word)
	{
	for(int byte = 0; byte < 4; ++byte)
		{
		d->state ^= (unsigned char) (word >> (byte * 8));
		d->state *= 0x100000001b3ULL;
		}
	++d->checks;
	}

static void nxDigestFloat(NxDigest* d, float value)
	{
	NxU32 word;
	memcpy(&word, &value, 4);
	nxDigestWord(d, word);
	}

static void nxDigestReport(const char* name, int present, const NxDigest* d)
	{
	if(!present)
		{
		printf("fuzz name=%s present=0 skipped=export_missing\n", name);
		return;
		}
	printf("fuzz name=%s present=1 checks=%u digest=%016llx\n", name, d->checks, d->state);
	}

// xorshift32. Seeded per block so a change to one block cannot silently shift
// the inputs of another.
struct NxRandom
	{
	NxU32 state;
	};

static NxU32 nxNext(NxRandom* r)
	{
	r->state ^= r->state << 13;
	r->state ^= r->state >> 17;
	r->state ^= r->state << 5;
	return r->state;
	}

static float nxFromBits(NxU32 bits)
	{
	float value;
	memcpy(&value, &bits, 4);
	return value;
	}

// The input mixture. Raw 32-bit patterns are the point of case 0: they produce
// NaN of both signs and arbitrary payload, infinity, denormals and negative
// zero without any of them having to be written down. The rest keep the run
// anchored on values a caller would really pass.
static float nxPick(NxRandom* r, unsigned mode)
	{
	switch(mode & 7)
		{
		case 0:  return nxFromBits(nxNext(r));
		case 1:  return (float) ((int) (nxNext(r) % 9) - 4);
		case 2:  return (float) ((int) (nxNext(r) % 9) - 4) * 0.5f;
		case 7:  return 0.0f;
		case 6:  return nxFromBits((((110 + (nxNext(r) % 35)) & 0xff) << 23)
					| (nxNext(r) & 0x7fffff) | ((nxNext(r) & 1) << 31));
		default: return nxFromBits((((120 + (nxNext(r) % 16)) & 0xff) << 23)
					| (nxNext(r) & 0x7fffff) | ((nxNext(r) & 1) << 31));
		}
	}

// Output buffers are poisoned before every call with the same cdcd000N ladder
// the case matrix uses, and the poison is folded into the digest along with
// everything else. Zero-filling them instead would make "wrote zero" and "did
// not write" the same transcript, and at least one kernel here -- the parallel
// path of NxSegmentPlaneIntersect, which returns without touching dist -- turns
// on exactly that distinction.
static void nxPoison(float* out, unsigned count)
	{
	for(unsigned i = 0; i < count; ++i)
		{
		const NxU32 word = 0xcdcd0000u + i;
		memcpy(out + i, &word, 4);
		}
	}

static float nxUnit(NxRandom* r)
	{
	return (float) ((double) (nxNext(r) >> 8) / 16777216.0);
	}

typedef float (NX_CALL_CONV *NxFnFF)(float, float);
typedef float (NX_CALL_CONV *NxFnVF)(const float*, float);
typedef float (NX_CALL_CONV *NxFnFFF)(float, float, float);
typedef void (NX_CALL_CONV *NxFnBoxInertia)(float*, float, float, float, float);
typedef void (NX_CALL_CONV *NxFnSphereInertia)(float*, float, float, unsigned char);
typedef unsigned char (NX_CALL_CONV *NxFnRayPlane)(const float*, const float*, float*, float*);
typedef void (NX_CALL_CONV *NxFnSegmentPlane)(const float*, const float*, const float*, float*, float*);
typedef unsigned char (NX_CALL_CONV *NxFnRaySphere)(const float*, const float*, const float*, float, float*);
typedef unsigned char (NX_CALL_CONV *NxFnRayTri)(const float*, const float*, const float*,
	const float*, const float*, float*, float*, float*, unsigned char);

// The eleven box, capsule, swept-sphere, separating-axis and smooth-normal
// exports. Signatures are transcribed from the pinned public headers, not
// guessed from the disassembly, so a block here cannot drift from the API.
typedef unsigned char (NX_CALL_CONV *NxFnRayAABB)(const float*, const float*, const float*, const float*, float*);
typedef NxU32 (NX_CALL_CONV *NxFnRayAABB2)(const float*, const float*, const float*, const float*, float*, float*);
typedef unsigned char (NX_CALL_CONV *NxFnSegAABB)(const float*, const float*, const float*, const float*);
typedef unsigned char (NX_CALL_CONV *NxFnSegBox)(const float*, const float*, const float*, const float*, float*);
typedef unsigned char (NX_CALL_CONV *NxFnRayOBB)(const float*, const float*, const float*, const float*);
typedef unsigned char (NX_CALL_CONV *NxFnSegOBB)(const float*, const float*, const float*, const float*, const float*);
typedef NxU32 (NX_CALL_CONV *NxFnRayCapsule)(const float*, const float*, const float*, float*);
typedef unsigned char (NX_CALL_CONV *NxFnSweptSpheres)(const float*, const float*, const float*, const float*);
typedef unsigned char (NX_CALL_CONV *NxFnBoxBox)(const float*, const float*, const float*,
	const float*, const float*, const float*, unsigned char);
typedef int (NX_CALL_CONV *NxFnSepAxis)(const float*, const float*, const float*,
	const float*, const float*, const float*, unsigned char);
typedef unsigned char (NX_CALL_CONV *NxFnSmoothNormals)(NxU32, NxU32, const float*,
	const NxU32*, const unsigned short*, float*, unsigned char);

// A rotation from three angles. The exact matrix does not matter -- what
// matters is that it is a real rotation rather than nine random floats, because
// an OBB test fed a non-orthonormal matrix degenerates and stops discriminating
// between a correct implementation and a wrong one. The general blocks below
// still feed raw random matrices; this is for the aimed blocks.
static void nxRotation(float* m, float ax, float ay, float az)
	{
	const float ca = (float) cos(ax), sa = (float) sin(ax);
	const float cb = (float) cos(ay), sb = (float) sin(ay);
	const float cc = (float) cos(az), sc = (float) sin(az);
	m[0] = cb * cc;                    m[1] = cb * sc;                    m[2] = -sb;
	m[3] = sa * sb * cc - ca * sc;     m[4] = sa * sb * sc + ca * cc;     m[5] = sa * cb;
	m[6] = ca * sb * cc + sa * sc;     m[7] = ca * sb * sc - sa * cc;     m[8] = ca * cb;
	}

// The precision witness. These three inputs separate the x87 _PC_53 model from
// a float32-everywhere model in the last bit; the two controls agree under
// both, so a transcript where the controls disagree is an instrument fault and
// not a finding. Printed in full rather than folded into a digest, because the
// whole point is that a reader can see the value.
static void nxPrecisionWitness(HMODULE physics)
	{
	NxFnFF sphereMass = (NxFnFF) GetProcAddress(physics, "NxComputeSphereMass");
	printf("witness name=NxComputeSphereMass present=%d\n", sphereMass ? 1 : 0);
	if(!sphereMass)
		return;

	static const struct { NxU32 radius, density; const char* role; } witnesses[] =
		{
		{ 0x40505969u, 0x404c999eu, "separates_pc53_from_float32" },
		{ 0x3feda98cu, 0x3ff1c90du, "separates_pc53_from_float32" },
		{ 0x403d2baau, 0x40496196u, "separates_pc53_from_float32" },
		{ 0x3f800000u, 0x3f800000u, "control_models_agree" },
		{ 0x40000000u, 0x3f000000u, "control_models_agree" },
		};
	for(int i = 0; i < 5; ++i)
		{
		const float result = sphereMass(nxFromBits(witnesses[i].radius), nxFromBits(witnesses[i].density));
		NxU32 word;
		memcpy(&word, &result, 4);
		printf("witness radius=%08x density=%08x ret=%08x role=%s\n",
			witnesses[i].radius, witnesses[i].density, word, witnesses[i].role);
		}
	}

static void nxRunScalarBlock(HMODULE physics)
	{
	struct { const char* name; void* fn; NxDigest digest; } table[] =
		{
		{ "NxComputeSphereMass", 0 }, { "NxComputeSphereDensity", 0 },
		{ "NxComputeBoxMass", 0 }, { "NxComputeBoxDensity", 0 },
		{ "NxComputeEllipsoidMass", 0 }, { "NxComputeEllipsoidDensity", 0 },
		{ "NxComputeCylinderMass", 0 }, { "NxComputeCylinderDensity", 0 },
		{ "NxComputeConeMass", 0 }, { "NxComputeConeDensity", 0 },
		{ "NxComputeBoxInertiaTensor", 0 }, { "NxComputeSphereInertiaTensor", 0 },
		};
	const int count = sizeof(table) / sizeof(table[0]);
	for(int i = 0; i < count; ++i)
		{
		table[i].fn = (void*) GetProcAddress(physics, table[i].name);
		nxDigestInit(&table[i].digest);
		}

	NxRandom random = { 0x13579bdfu };
	for(unsigned i = 0; i < kScalarIterations; ++i)
		{
		const float a = nxPick(&random, i);
		const float b = nxPick(&random, i >> 2);
		const float c = nxPick(&random, i >> 4);
		const float d = nxPick(&random, i >> 6);
		const float extents[3] = { a, b, c };
		const unsigned char hollow = (unsigned char) (nxNext(&random) & 3);

		if(table[0].fn) nxDigestFloat(&table[0].digest, ((NxFnFF) table[0].fn)(a, b));
		if(table[1].fn) nxDigestFloat(&table[1].digest, ((NxFnFF) table[1].fn)(a, b));
		if(table[2].fn) nxDigestFloat(&table[2].digest, ((NxFnVF) table[2].fn)(extents, d));
		if(table[3].fn) nxDigestFloat(&table[3].digest, ((NxFnVF) table[3].fn)(extents, d));
		if(table[4].fn) nxDigestFloat(&table[4].digest, ((NxFnVF) table[4].fn)(extents, d));
		if(table[5].fn) nxDigestFloat(&table[5].digest, ((NxFnVF) table[5].fn)(extents, d));
		if(table[6].fn) nxDigestFloat(&table[6].digest, ((NxFnFFF) table[6].fn)(a, b, c));
		if(table[7].fn) nxDigestFloat(&table[7].digest, ((NxFnFFF) table[7].fn)(a, b, c));
		if(table[8].fn) nxDigestFloat(&table[8].digest, ((NxFnFFF) table[8].fn)(a, b, c));
		if(table[9].fn) nxDigestFloat(&table[9].digest, ((NxFnFFF) table[9].fn)(a, b, c));

		if(table[10].fn)
			{
			float out[3];
			nxPoison(out, 3);
			((NxFnBoxInertia) table[10].fn)(out, a, b, c, d);
			for(int k = 0; k < 3; ++k)
				nxDigestFloat(&table[10].digest, out[k]);
			}
		if(table[11].fn)
			{
			float out[3];
			nxPoison(out, 3);
			((NxFnSphereInertia) table[11].fn)(out, a, b, hollow);
			for(int k = 0; k < 3; ++k)
				nxDigestFloat(&table[11].digest, out[k]);
			}
		}

	for(int i = 0; i < count; ++i)
		nxDigestReport(table[i].name, table[i].fn != 0, &table[i].digest);
	}

static void nxRunVectorBlock(HMODULE physics)
	{
	NxFnRayPlane rayPlane = (NxFnRayPlane) GetProcAddress(physics, "NxRayPlaneIntersect");
	NxFnSegmentPlane segmentPlane = (NxFnSegmentPlane) GetProcAddress(physics, "NxSegmentPlaneIntersect");
	NxFnRaySphere raySphere = (NxFnRaySphere) GetProcAddress(physics, "NxRaySphereIntersect");
	NxFnRayTri rayTri = (NxFnRayTri) GetProcAddress(physics, "NxRayTriIntersect");

	NxDigest dRayPlane, dSegmentPlane, dRaySphere, dRayTri;
	nxDigestInit(&dRayPlane);
	nxDigestInit(&dSegmentPlane);
	nxDigestInit(&dRaySphere);
	nxDigestInit(&dRayTri);

	NxRandom random = { 0x02468aceu };
	for(unsigned i = 0; i < kVectorIterations; ++i)
		{
		float w[15];
		for(int k = 0; k < 15; ++k)
			w[k] = nxPick(&random, i + k);

		if(rayPlane)
			{
			float point[3], distance;
			nxPoison(point, 3);
			nxPoison(&distance, 1);
			nxDigestWord(&dRayPlane, rayPlane(w + 0, w + 6, &distance, point));
			nxDigestFloat(&dRayPlane, distance);
			for(int k = 0; k < 3; ++k)
				nxDigestFloat(&dRayPlane, point[k]);
			}
		if(segmentPlane)
			{
			float point[3], distance;
			nxPoison(point, 3);
			nxPoison(&distance, 1);
			segmentPlane(w + 0, w + 3, w + 6, &distance, point);
			nxDigestFloat(&dSegmentPlane, distance);
			for(int k = 0; k < 3; ++k)
				nxDigestFloat(&dSegmentPlane, point[k]);
			}
		if(raySphere)
			{
			float coord[3];
			nxPoison(coord, 3);
			nxDigestWord(&dRaySphere, raySphere(w + 0, w + 3, w + 6, w[9], coord));
			for(int k = 0; k < 3; ++k)
				nxDigestFloat(&dRaySphere, coord[k]);
			}
		if(rayTri)
			for(unsigned char cull = 0; cull < 2; ++cull)
				{
				float barycentric[3];
				nxPoison(barycentric, 3);
				float& t = barycentric[0];
				float& u = barycentric[1];
				float& v = barycentric[2];
				nxDigestWord(&dRayTri, rayTri(w + 0, w + 3, w + 6, w + 9, w + 12, &t, &u, &v, cull));
				nxDigestFloat(&dRayTri, t);
				nxDigestFloat(&dRayTri, u);
				nxDigestFloat(&dRayTri, v);
				}
		}

	nxDigestReport("NxRayPlaneIntersect", rayPlane != 0, &dRayPlane);
	nxDigestReport("NxSegmentPlaneIntersect", segmentPlane != 0, &dSegmentPlane);
	nxDigestReport("NxRaySphereIntersect", raySphere != 0, &dRaySphere);
	nxDigestReport("NxRayTriIntersect", rayTri != 0, &dRayTri);
	}

// The aimed ray/triangle block.
//
// The mixture above almost never produces a ray that hits a triangle, so every
// early return in NxRayTriIntersect was taken and the whole tail of it -- the
// second barycentric, the reciprocal of the determinant, all three writes --
// was never reached. That is a defect in a generator and not a property of the
// function: a mutation to the tail was accepted by the block above. Here the
// ray is aimed at a random barycentric point of the triangle, so the hit path
// is entered by construction, and the hit count is printed. A generator that
// stops hitting therefore reads as a change in the transcript rather than as a
// pass.
static void nxRunAimedBlock(HMODULE physics)
	{
	NxFnRayTri rayTri = (NxFnRayTri) GetProcAddress(physics, "NxRayTriIntersect");
	NxDigest digest;
	nxDigestInit(&digest);
	unsigned hits = 0;

	NxRandom random = { 0xfeedfaceu };
	for(unsigned i = 0; i < kAimedIterations; ++i)
		{
		float v0[3], v1[3], v2[3], origin[3], direction[3];
		for(int k = 0; k < 3; ++k)
			{
			v0[k] = nxUnit(&random) * 8.0f - 4.0f;
			v1[k] = nxUnit(&random) * 8.0f - 4.0f;
			v2[k] = nxUnit(&random) * 8.0f - 4.0f;
			origin[k] = nxUnit(&random) * 12.0f - 6.0f;
			}
		float a = nxUnit(&random), b = nxUnit(&random);
		if(a + b > 1.0f)
			{
			a = 1.0f - a;
			b = 1.0f - b;
			}
		for(int k = 0; k < 3; ++k)
			direction[k] = (v0[k] + a * (v1[k] - v0[k]) + b * (v2[k] - v0[k])) - origin[k];

		if(!rayTri)
			continue;
		for(unsigned char cull = 0; cull < 2; ++cull)
			{
			float barycentric[3];
			nxPoison(barycentric, 3);
			float& t = barycentric[0];
			float& u = barycentric[1];
			float& v = barycentric[2];
			const unsigned char hit = rayTri(origin, direction, v0, v1, v2, &t, &u, &v, cull);
			hits += hit ? 1 : 0;
			nxDigestWord(&digest, hit);
			nxDigestFloat(&digest, t);
			nxDigestFloat(&digest, u);
			nxDigestFloat(&digest, v);
			}
		}

	if(!rayTri)
		{
		printf("fuzz name=NxRayTriIntersect.aimed present=0 skipped=export_missing\n");
		return;
		}
	printf("fuzz name=NxRayTriIntersect.aimed present=1 checks=%u digest=%016llx hits=%u\n",
		digest.checks, digest.state, hits);
	}

// The box kernels on ordinary random input.
//
// Six exports, all slab tests of one shape or another. Four of them return a
// bare bool and write nothing at all, so nothing but a digest over many inputs
// has any resolution on them: the hand-authored case matrix cannot tell a
// correct implementation from the oracle for NxRayOBBIntersect,
// NxSegmentOBBIntersect or NxSegmentAABBIntersect by construction.
//
// This block feeds the raw random mixture, which is mostly NaN, infinity and
// wildly-scaled values. That covers the rejection paths and the unordered
// comparisons thoroughly and the hit paths almost never -- which is a defect in
// a generator, not a property of the functions. nxRunAimedBoxBlock below is the
// other half and reports its hit counts so the coverage cannot quietly lapse.
static void nxRunBoxBlock(HMODULE physics)
	{
	NxFnRayAABB rayAabb = (NxFnRayAABB) GetProcAddress(physics, "NxRayAABBIntersect");
	NxFnRayAABB2 rayAabb2 = (NxFnRayAABB2) GetProcAddress(physics, "NxRayAABBIntersect2");
	NxFnSegAABB segAabb = (NxFnSegAABB) GetProcAddress(physics, "NxSegmentAABBIntersect");
	NxFnSegBox segBox = (NxFnSegBox) GetProcAddress(physics, "NxSegmentBoxIntersect");
	NxFnRayOBB rayObb = (NxFnRayOBB) GetProcAddress(physics, "NxRayOBBIntersect");
	NxFnSegOBB segObb = (NxFnSegOBB) GetProcAddress(physics, "NxSegmentOBBIntersect");

	NxDigest dRayAabb, dRayAabb2, dSegAabb, dSegBox, dRayObb, dSegObb;
	nxDigestInit(&dRayAabb);
	nxDigestInit(&dRayAabb2);
	nxDigestInit(&dSegAabb);
	nxDigestInit(&dSegBox);
	nxDigestInit(&dRayObb);
	nxDigestInit(&dSegObb);

	NxRandom random = { 0x1a2b3c4du };
	for(unsigned i = 0; i < kBoxIterations; ++i)
		{
		float w[24];
		for(int k = 0; k < 24; ++k)
			w[k] = nxPick(&random, i + k);

		if(rayAabb)
			{
			float coord[3];
			nxPoison(coord, 3);
			nxDigestWord(&dRayAabb, rayAabb(w + 0, w + 3, w + 6, w + 9, coord));
			for(int k = 0; k < 3; ++k)
				nxDigestFloat(&dRayAabb, coord[k]);
			}
		if(rayAabb2)
			{
			float coord[3], t;
			nxPoison(coord, 3);
			nxPoison(&t, 1);
			nxDigestWord(&dRayAabb2, rayAabb2(w + 0, w + 3, w + 6, w + 9, coord, &t));
			for(int k = 0; k < 3; ++k)
				nxDigestFloat(&dRayAabb2, coord[k]);
			nxDigestFloat(&dRayAabb2, t);
			}
		if(segAabb)
			nxDigestWord(&dSegAabb, segAabb(w + 0, w + 3, w + 6, w + 9));
		if(segBox)
			{
			float intercept[3];
			nxPoison(intercept, 3);
			nxDigestWord(&dSegBox, segBox(w + 0, w + 3, w + 6, w + 9, intercept));
			for(int k = 0; k < 3; ++k)
				nxDigestFloat(&dSegBox, intercept[k]);
			}
		if(rayObb)
			nxDigestWord(&dRayObb, rayObb(w + 0, w + 6, w + 9, w + 12));
		if(segObb)
			nxDigestWord(&dSegObb, segObb(w + 0, w + 3, w + 6, w + 9, w + 12));
		}

	nxDigestReport("NxRayAABBIntersect", rayAabb != 0, &dRayAabb);
	nxDigestReport("NxRayAABBIntersect2", rayAabb2 != 0, &dRayAabb2);
	nxDigestReport("NxSegmentAABBIntersect", segAabb != 0, &dSegAabb);
	nxDigestReport("NxSegmentBoxIntersect", segBox != 0, &dSegBox);
	nxDigestReport("NxRayOBBIntersect", rayObb != 0, &dRayObb);
	nxDigestReport("NxSegmentOBBIntersect", segObb != 0, &dSegObb);
	}

// The box kernels with the ray and the segment aimed at the box.
//
// Same lesson as nxRunAimedBlock: a generator that never hits leaves the whole
// tail of a kernel untested and will accept a mutation to it. Here a box is
// built around a random centre, a target point is chosen inside it, and the ray
// or segment is aimed through that point from outside, so the hit path is
// entered by construction. The OBB variants get a genuine rotation and the same
// target transformed into world space.
//
// Each export's hit count is printed. A generator that stops hitting therefore
// changes the transcript rather than passing quietly, and the counts are
// registered in $NxRequiredCoverageLines so the phase gate fails on it -- a
// symmetric differential cannot catch that by itself, because a harness that
// stops covering something stops covering it identically on both pairs.
static void nxRunAimedBoxBlock(HMODULE physics)
	{
	NxFnRayAABB rayAabb = (NxFnRayAABB) GetProcAddress(physics, "NxRayAABBIntersect");
	NxFnRayAABB2 rayAabb2 = (NxFnRayAABB2) GetProcAddress(physics, "NxRayAABBIntersect2");
	NxFnSegAABB segAabb = (NxFnSegAABB) GetProcAddress(physics, "NxSegmentAABBIntersect");
	NxFnSegBox segBox = (NxFnSegBox) GetProcAddress(physics, "NxSegmentBoxIntersect");
	NxFnRayOBB rayObb = (NxFnRayOBB) GetProcAddress(physics, "NxRayOBBIntersect");
	NxFnSegOBB segObb = (NxFnSegOBB) GetProcAddress(physics, "NxSegmentOBBIntersect");

	NxDigest dRayAabb, dRayAabb2, dSegAabb, dSegBox, dRayObb, dSegObb;
	nxDigestInit(&dRayAabb);
	nxDigestInit(&dRayAabb2);
	nxDigestInit(&dSegAabb);
	nxDigestInit(&dSegBox);
	nxDigestInit(&dRayObb);
	nxDigestInit(&dSegObb);
	unsigned hitRayAabb = 0, hitRayAabb2 = 0, hitSegAabb = 0, hitSegBox = 0, hitRayObb = 0, hitSegObb = 0;

	NxRandom random = { 0x5e6f7a8bu };
	for(unsigned i = 0; i < kAimedBoxIterations; ++i)
		{
		float centre[3], extents[3], target[3], origin[3], direction[3], min[3], max[3];
		for(int k = 0; k < 3; ++k)
			{
			centre[k] = nxUnit(&random) * 8.0f - 4.0f;
			extents[k] = nxUnit(&random) * 3.0f + 0.03125f;
			min[k] = centre[k] - extents[k];
			max[k] = centre[k] + extents[k];
			// Inside the box for most iterations, a little outside for some, so
			// the near-miss boundary is exercised as well as the clean hit.
			target[k] = centre[k] + extents[k] * (nxUnit(&random) * 2.4f - 1.2f);
			origin[k] = nxUnit(&random) * 24.0f - 12.0f;
			direction[k] = target[k] - origin[k];
			}
		// The segment runs from the origin to a point past the target, so it
		// reaches the box rather than stopping short of it.
		float far_[3];
		for(int k = 0; k < 3; ++k)
			far_[k] = origin[k] + direction[k] * 1.75f;

		float ray[6];
		for(int k = 0; k < 3; ++k)
			{
			ray[k] = origin[k];
			ray[k + 3] = direction[k];
			}

		if(rayAabb)
			{
			float coord[3];
			nxPoison(coord, 3);
			const unsigned char hit = rayAabb(min, max, origin, direction, coord);
			hitRayAabb += hit ? 1 : 0;
			nxDigestWord(&dRayAabb, hit);
			for(int k = 0; k < 3; ++k)
				nxDigestFloat(&dRayAabb, coord[k]);
			}
		if(rayAabb2)
			{
			float coord[3], t;
			nxPoison(coord, 3);
			nxPoison(&t, 1);
			const NxU32 hit = rayAabb2(min, max, origin, direction, coord, &t);
			hitRayAabb2 += hit ? 1 : 0;
			nxDigestWord(&dRayAabb2, hit);
			for(int k = 0; k < 3; ++k)
				nxDigestFloat(&dRayAabb2, coord[k]);
			nxDigestFloat(&dRayAabb2, t);
			}
		if(segAabb)
			{
			const unsigned char hit = segAabb(origin, far_, min, max);
			hitSegAabb += hit ? 1 : 0;
			nxDigestWord(&dSegAabb, hit);
			}
		if(segBox)
			{
			float intercept[3];
			nxPoison(intercept, 3);
			const unsigned char hit = segBox(origin, far_, min, max, intercept);
			hitSegBox += hit ? 1 : 0;
			nxDigestWord(&dSegBox, hit);
			for(int k = 0; k < 3; ++k)
				nxDigestFloat(&dSegBox, intercept[k]);
			}

		// The oriented variants. The same target is expressed in box space and
		// pushed back out through the rotation, so the ray still aims at it.
		float rot[9];
		nxRotation(rot, nxUnit(&random) * 6.2831853f, nxUnit(&random) * 6.2831853f,
			nxUnit(&random) * 6.2831853f);
		float local[3], world[3];
		for(int k = 0; k < 3; ++k)
			local[k] = extents[k] * (nxUnit(&random) * 2.4f - 1.2f);
		for(int k = 0; k < 3; ++k)
			world[k] = centre[k] + rot[k] * local[0] + rot[k + 3] * local[1] + rot[k + 6] * local[2];
		float obbRay[6], obbFar[3];
		for(int k = 0; k < 3; ++k)
			{
			obbRay[k] = origin[k];
			obbRay[k + 3] = world[k] - origin[k];
			obbFar[k] = origin[k] + (world[k] - origin[k]) * 1.75f;
			}

		if(rayObb)
			{
			const unsigned char hit = rayObb(obbRay, centre, extents, rot);
			hitRayObb += hit ? 1 : 0;
			nxDigestWord(&dRayObb, hit);
			}
		if(segObb)
			{
			const unsigned char hit = segObb(origin, obbFar, centre, extents, rot);
			hitSegObb += hit ? 1 : 0;
			nxDigestWord(&dSegObb, hit);
			}
		}

	printf("fuzz name=NxRayAABBIntersect.aimed present=%d checks=%u digest=%016llx hits=%u\n",
		rayAabb ? 1 : 0, dRayAabb.checks, dRayAabb.state, hitRayAabb);
	printf("fuzz name=NxRayAABBIntersect2.aimed present=%d checks=%u digest=%016llx hits=%u\n",
		rayAabb2 ? 1 : 0, dRayAabb2.checks, dRayAabb2.state, hitRayAabb2);
	printf("fuzz name=NxSegmentAABBIntersect.aimed present=%d checks=%u digest=%016llx hits=%u\n",
		segAabb ? 1 : 0, dSegAabb.checks, dSegAabb.state, hitSegAabb);
	printf("fuzz name=NxSegmentBoxIntersect.aimed present=%d checks=%u digest=%016llx hits=%u\n",
		segBox ? 1 : 0, dSegBox.checks, dSegBox.state, hitSegBox);
	printf("fuzz name=NxRayOBBIntersect.aimed present=%d checks=%u digest=%016llx hits=%u\n",
		rayObb ? 1 : 0, dRayObb.checks, dRayObb.state, hitRayObb);
	printf("fuzz name=NxSegmentOBBIntersect.aimed present=%d checks=%u digest=%016llx hits=%u\n",
		segObb ? 1 : 0, dSegObb.checks, dSegObb.state, hitSegObb);
	}

// The capsule and swept-sphere kernels, general input then aimed.
//
// NxRayCapsuleIntersect returns a count and writes up to two roots, so the
// poison ladder is what distinguishes "returned 1 and wrote one root" from
// "returned 1 and wrote both". NxSweptSpheresIntersect returns a bare bool and
// writes nothing, so the aimed half -- two spheres actually put on a collision
// course -- is the only thing that exercises its interesting path at all.
static void nxRunCapsuleBlock(HMODULE physics)
	{
	NxFnRayCapsule rayCapsule = (NxFnRayCapsule) GetProcAddress(physics, "NxRayCapsuleIntersect");
	NxFnSweptSpheres swept = (NxFnSweptSpheres) GetProcAddress(physics, "NxSweptSpheresIntersect");

	NxDigest dCapsule, dSwept, dCapsuleAimed, dSweptAimed;
	nxDigestInit(&dCapsule);
	nxDigestInit(&dSwept);
	nxDigestInit(&dCapsuleAimed);
	nxDigestInit(&dSweptAimed);
	unsigned hitCapsule = 0, hitSwept = 0;

	NxRandom random = { 0x9c0d1e2fu };
	for(unsigned i = 0; i < kCapsuleIterations; ++i)
		{
		float w[16];
		for(int k = 0; k < 16; ++k)
			w[k] = nxPick(&random, i + k);

		if(rayCapsule)
			{
			float t[2];
			nxPoison(t, 2);
			nxDigestWord(&dCapsule, rayCapsule(w + 0, w + 3, w + 6, t));
			nxDigestFloat(&dCapsule, t[0]);
			nxDigestFloat(&dCapsule, t[1]);
			}
		if(swept)
			nxDigestWord(&dSwept, swept(w + 0, w + 4, w + 7, w + 11));

		// Aimed: a capsule with a real axis and radius, and a ray fired at a
		// point near its surface from outside.
		float capsule[7], origin[3], direction[3], target[3];
		for(int k = 0; k < 3; ++k)
			{
			capsule[k] = nxUnit(&random) * 6.0f - 3.0f;
			capsule[k + 3] = capsule[k] + nxUnit(&random) * 6.0f - 3.0f;
			}
		capsule[6] = nxUnit(&random) * 1.5f + 0.03125f;
		const float along = nxUnit(&random);
		for(int k = 0; k < 3; ++k)
			{
			const float axis = capsule[k] + along * (capsule[k + 3] - capsule[k]);
			target[k] = axis + capsule[6] * (nxUnit(&random) * 2.4f - 1.2f);
			origin[k] = nxUnit(&random) * 20.0f - 10.0f;
			direction[k] = target[k] - origin[k];
			}
		if(rayCapsule)
			{
			float t[2];
			nxPoison(t, 2);
			const NxU32 hit = rayCapsule(origin, direction, capsule, t);
			hitCapsule += hit ? 1 : 0;
			nxDigestWord(&dCapsuleAimed, hit);
			nxDigestFloat(&dCapsuleAimed, t[0]);
			nxDigestFloat(&dCapsuleAimed, t[1]);
			}

		// Aimed: two spheres whose relative velocity closes the gap between
		// them, so the quadratic actually has roots for a good share of them.
		float sphere0[4], sphere1[4], velocity0[3], velocity1[3];
		for(int k = 0; k < 3; ++k)
			{
			sphere0[k] = nxUnit(&random) * 6.0f - 3.0f;
			sphere1[k] = nxUnit(&random) * 6.0f - 3.0f;
			velocity0[k] = (sphere1[k] - sphere0[k]) * (nxUnit(&random) * 2.0f);
			velocity1[k] = (sphere0[k] - sphere1[k]) * (nxUnit(&random) * 2.0f);
			}
		sphere0[3] = nxUnit(&random) * 2.0f + 0.03125f;
		sphere1[3] = nxUnit(&random) * 2.0f + 0.03125f;
		if(swept)
			{
			const unsigned char hit = swept(sphere0, velocity0, sphere1, velocity1);
			hitSwept += hit ? 1 : 0;
			nxDigestWord(&dSweptAimed, hit);
			}
		}

	nxDigestReport("NxRayCapsuleIntersect", rayCapsule != 0, &dCapsule);
	nxDigestReport("NxSweptSpheresIntersect", swept != 0, &dSwept);
	printf("fuzz name=NxRayCapsuleIntersect.aimed present=%d checks=%u digest=%016llx hits=%u\n",
		rayCapsule ? 1 : 0, dCapsuleAimed.checks, dCapsuleAimed.state, hitCapsule);
	printf("fuzz name=NxSweptSpheresIntersect.aimed present=%d checks=%u digest=%016llx hits=%u\n",
		swept ? 1 : 0, dSweptAimed.checks, dSweptAimed.state, hitSwept);
	}

// The two separating-axis exports.
//
// NxBoxBoxIntersect returns a bool and NxSeparatingAxis returns which axis
// separated the pair, so the second is the more informative of the two: it
// distinguishes fifteen outcomes where the first distinguishes two, and a
// reconstruction that tests the axes in the wrong order is caught by it and not
// by the bool. Both are run with fullTest set and clear, because clearing it is
// what skips the nine cross-product axes.
//
// Random boxes essentially never overlap, so the aimed half places the second
// box near the first. The overlap count is printed for the same reason the ray
// hit counts are.
static void nxRunSatBlock(HMODULE physics)
	{
	NxFnBoxBox boxBox = (NxFnBoxBox) GetProcAddress(physics, "NxBoxBoxIntersect");
	NxFnSepAxis sepAxis = (NxFnSepAxis) GetProcAddress(physics, "NxSeparatingAxis");

	NxDigest dBoxBox, dSepAxis, dBoxBoxAimed, dSepAxisAimed;
	nxDigestInit(&dBoxBox);
	nxDigestInit(&dSepAxis);
	nxDigestInit(&dBoxBoxAimed);
	nxDigestInit(&dSepAxisAimed);
	unsigned overlaps = 0;

	NxRandom random = { 0x0f1e2d3cu };
	for(unsigned i = 0; i < kSatIterations; ++i)
		{
		float w[30];
		for(int k = 0; k < 30; ++k)
			w[k] = nxPick(&random, i + k);
		for(unsigned char full = 0; full < 2; ++full)
			{
			if(boxBox)
				nxDigestWord(&dBoxBox, boxBox(w + 0, w + 3, w + 6, w + 15, w + 18, w + 21, full));
			}
		// NxSeparatingAxis is driven with fullTest set only, and that is a
		// measured limitation rather than an oversight. With fullTest clear the
		// oracle skips the nine cross-axis stores but still scans all fifteen
		// slots, so six of them hold whatever the caller last left on the
		// stack: on that path the shipped function is not a function of its
		// arguments and no reimplementation can match it in general. The three
		// slots that ARE deterministic -- the prologue's spill of rotation1's
		// third column -- are reproduced, and the case matrix covers them
		// through NxSeparatingAxis.05, which returns 9 for exactly that reason.
		if(sepAxis)
			nxDigestWord(&dSepAxis, (NxU32) sepAxis(w + 0, w + 3, w + 6, w + 15, w + 18, w + 21, 1));

		float extents0[3], centre0[3], rot0[9], extents1[3], centre1[3], rot1[9];
		for(int k = 0; k < 3; ++k)
			{
			extents0[k] = nxUnit(&random) * 2.0f + 0.0625f;
			extents1[k] = nxUnit(&random) * 2.0f + 0.0625f;
			centre0[k] = nxUnit(&random) * 4.0f - 2.0f;
			// Near box0, and sometimes far enough to separate, so both answers
			// occur often enough to discriminate.
			centre1[k] = centre0[k] + (nxUnit(&random) * 6.0f - 3.0f);
			}
		nxRotation(rot0, nxUnit(&random) * 6.2831853f, nxUnit(&random) * 6.2831853f,
			nxUnit(&random) * 6.2831853f);
		nxRotation(rot1, nxUnit(&random) * 6.2831853f, nxUnit(&random) * 6.2831853f,
			nxUnit(&random) * 6.2831853f);
		for(unsigned char full = 0; full < 2; ++full)
			{
			if(boxBox)
				{
				const unsigned char hit = boxBox(extents0, centre0, rot0, extents1, centre1, rot1, full);
				overlaps += hit ? 1 : 0;
				nxDigestWord(&dBoxBoxAimed, hit);
				}
			}
		if(sepAxis)
			nxDigestWord(&dSepAxisAimed,
				(NxU32) sepAxis(extents0, centre0, rot0, extents1, centre1, rot1, 1));
		}

	nxDigestReport("NxBoxBoxIntersect", boxBox != 0, &dBoxBox);
	nxDigestReport("NxSeparatingAxis", sepAxis != 0, &dSepAxis);
	printf("fuzz name=NxBoxBoxIntersect.aimed present=%d checks=%u digest=%016llx overlaps=%u\n",
		boxBox ? 1 : 0, dBoxBoxAimed.checks, dBoxBoxAimed.state, overlaps);
	nxDigestReport("NxSeparatingAxis.aimed", sepAxis != 0, &dSepAxisAimed);
	}

// NxBuildSmoothNormals.
//
// The odd one out: it takes arrays rather than scalars and it allocates. The
// mesh is small and the indices are generated in range deliberately -- the
// export does not bounds-check them, and feeding it an out-of-range index would
// corrupt the harness's own heap identically on both pairs, which is neither a
// useful check nor a safe one. That the check is missing is recorded in the
// evidence rather than exercised here.
//
// Both index widths are driven, because the export takes a dword array and a
// word array and chooses between them, and so is the flip flag.
static void nxRunNormalsBlock(HMODULE physics)
	{
	NxFnSmoothNormals smooth = (NxFnSmoothNormals) GetProcAddress(physics, "NxBuildSmoothNormals");
	NxDigest digest;
	nxDigestInit(&digest);
	unsigned accepted = 0;

	NxRandom random = { 0x7b8a9d6eu };
	for(unsigned i = 0; i < kNormalsIterations; ++i)
		{
		const NxU32 nbVerts = 3 + (nxNext(&random) % 14);
		const NxU32 nbTris = 1 + (nxNext(&random) % 12);
		float verts[17 * 3];
		float normals[17 * 3];
		NxU32 dFaces[12 * 3];
		unsigned short wFaces[12 * 3];
		for(NxU32 v = 0; v < nbVerts; ++v)
			for(int k = 0; k < 3; ++k)
				verts[v * 3 + k] = nxUnit(&random) * 4.0f - 2.0f;
		for(NxU32 t = 0; t < nbTris * 3; ++t)
			{
			const NxU32 index = nxNext(&random) % nbVerts;
			dFaces[t] = index;
			wFaces[t] = (unsigned short) index;
			}

		if(!smooth)
			continue;
		const unsigned char flip = (unsigned char) (nxNext(&random) & 1);
		// The dword path.
		nxPoison(normals, (unsigned) nbVerts * 3);
		unsigned char ok = smooth(nbTris, nbVerts, verts, dFaces, 0, normals, flip);
		accepted += ok ? 1 : 0;
		nxDigestWord(&digest, ok);
		for(NxU32 k = 0; k < nbVerts * 3; ++k)
			nxDigestFloat(&digest, normals[k]);
		// The word path, same mesh, so a divergence between the two index
		// widths shows up as a digest change rather than as nothing.
		nxPoison(normals, (unsigned) nbVerts * 3);
		ok = smooth(nbTris, nbVerts, verts, 0, wFaces, normals, flip);
		accepted += ok ? 1 : 0;
		nxDigestWord(&digest, ok);
		for(NxU32 k = 0; k < nbVerts * 3; ++k)
			nxDigestFloat(&digest, normals[k]);
		}

	if(!smooth)
		{
		printf("fuzz name=NxBuildSmoothNormals present=0 skipped=export_missing\n");
		return;
		}
	printf("fuzz name=NxBuildSmoothNormals present=1 checks=%u digest=%016llx accepted=%u\n",
		digest.checks, digest.state, accepted);
	}

int wmain(int argc, wchar_t** argv)
	{
	setvbuf(stdout, 0, _IONBF, 0);

	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsKernelFuzzTests", pairDirectory, &physics);
	if(status)
		return status;

	printf("fuzz generator=xorshift32 scalar_iterations=%u vector_iterations=%u aimed_iterations=%u\n",
		kScalarIterations, kVectorIterations, kAimedIterations);
	printf("fuzz seeds scalar=13579bdf vector=02468ace aimed=feedface\n");

	printf("fuzz box_iterations=%u aimed_box_iterations=%u capsule_iterations=%u sat_iterations=%u normals_iterations=%u\n",
		kBoxIterations, kAimedBoxIterations, kCapsuleIterations, kSatIterations, kNormalsIterations);
	printf("fuzz seeds box=1a2b3c4d aimedbox=5e6f7a8b capsule=9c0d1e2f sat=0f1e2d3c normals=7b8a9d6e\n");

	nxPrecisionWitness(physics);
	nxRunScalarBlock(physics);
	nxRunVectorBlock(physics);
	nxRunAimedBlock(physics);
	nxRunBoxBlock(physics);
	nxRunAimedBoxBlock(physics);
	nxRunCapsuleBlock(physics);
	nxRunSatBlock(physics);
	nxRunNormalsBlock(physics);

	status = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	return status;
	}
