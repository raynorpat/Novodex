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

	nxPrecisionWitness(physics);
	nxRunScalarBlock(physics);
	nxRunVectorBlock(physics);
	nxRunAimedBlock(physics);

	status = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	return status;
	}
