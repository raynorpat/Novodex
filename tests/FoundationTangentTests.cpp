// The staged-pair differential for NxNormalToTangents.
//
// The Foundation project's own gate for this export asserts invariants -- both
// branches reached, outputs orthonormal -- and both builds satisfy those while
// disagreeing on the words (evidence/phase6-joints.md 6o). This one prints the
// words. The runner compares the two transcripts line for line, so every case
// below is an exact-bits comparison against the pinned NxFoundation.dll.
//
// The branch test is `|n.z| > 0.70710678` against a double at [0x1001c3a0], so
// the fixed cases straddle it with the two floats on either side of 1/sqrt(2),
// and each arm gets a zero n.x, non-unit lengths, and the non-finite inputs that
// decide NaN signs and payloads. The sweep after them folds a few hundred
// thousand generated inputs into one digest, with a band aimed at the threshold,
// because the per-case lines alone did not find the one-ULP arm-two difference
// that a wider input set did.

#include "PhysicsPairLoader.h"

#include <string.h>

#include "NxVec3.h"

// Resolved from the loaded Foundation module like every other address these
// tests call; the harness links against neither side's import library.
typedef void (NX_CALL_CONV *NormalToTangentsFn)(const NxVec3&, NxVec3&, NxVec3&);

static NormalToTangentsFn nxNormalToTangents = 0;

static NxU32 nxBits(float v) { NxU32 b; memcpy(&b, &v, 4); return b; }
static float nxFloat(NxU32 b) { float v; memcpy(&v, &b, 4); return v; }

// The oracle's threshold is a double; these are the floats either side of it.
static const NxU32 nxBelowThreshold = 0x3f3504f3u;	// 0.70710677 -- takes the n.x/n.y arm
static const NxU32 nxAboveThreshold = 0x3f3504f4u;	// 0.70710683 -- takes the n.z arm

static NxU32 nxArmZ = 0;
static NxU32 nxArmXY = 0;

static bool nxTakesZArm(const NxVec3& n)
	{
	// The same test the oracle makes, done here only to count the arms so the
	// coverage line shows both were reached; the comparison is on the words.
	const double z = n.z < 0.0f ? -double(n.z) : double(n.z);
	return z > 0.7071067811865475244;
	}

static void nxRun(const NxVec3& n, NxVec3& t1, NxVec3& t2)
	{
	// Poisoned outputs, so a store the function skips shows up as a word
	// rather than as whatever the previous case left behind.
	t1.x = t1.y = t1.z = nxFloat(0x7fa5a5a5u);
	t2.x = t2.y = t2.z = nxFloat(0x7fa5a5a5u);
	nxNormalToTangents(n, t1, t2);
	if(nxTakesZArm(n))
		nxArmZ++;
	else
		nxArmXY++;
	}

static void nxShow(const char* tag, NxU32 x, NxU32 y, NxU32 z)
	{
	const NxVec3 n(nxFloat(x), nxFloat(y), nxFloat(z));
	NxVec3 t1, t2;
	nxRun(n, t1, t2);
	printf("tangent case=%s arm=%s n=%08x.%08x.%08x t1=%08x.%08x.%08x t2=%08x.%08x.%08x\n",
		tag, nxTakesZArm(n) ? "z" : "xy", x, y, z,
		nxBits(t1.x), nxBits(t1.y), nxBits(t1.z),
		nxBits(t2.x), nxBits(t2.y), nxBits(t2.z));
	}

static NxU32 nxState = 0x7a4e6c31u;

static NxU32 nxNext()
	{
	nxState ^= nxState << 13;
	nxState ^= nxState >> 17;
	nxState ^= nxState << 5;
	return nxState;
	}

// A finite float in [-range, range) with a random sign.
static float nxUniform(float range)
	{
	const float unit = float(nxNext() >> 8) * (1.0f / 16777216.0f);
	return (unit * 2.0f - 1.0f) * range;
	}

static NxU32 nxFold(NxU32 digest, NxU32 word)
	{
	for(int i = 0; i < 4; ++i)
		{
		digest ^= (word >> (i * 8)) & 0xffu;
		digest *= 16777619u;
		}
	return digest;
	}

static NxU32 nxFoldCase(NxU32 digest, const NxVec3& n)
	{
	NxVec3 t1, t2;
	nxRun(n, t1, t2);
	digest = nxFold(digest, nxBits(t1.x));
	digest = nxFold(digest, nxBits(t1.y));
	digest = nxFold(digest, nxBits(t1.z));
	digest = nxFold(digest, nxBits(t2.x));
	digest = nxFold(digest, nxBits(t2.y));
	return nxFold(digest, nxBits(t2.z));
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxFoundationTangentTests", pairDirectory, &physics);
	if(status)
		return status;

	HMODULE foundation = GetModuleHandleW(L"NxFoundation.dll");
	if(!foundation)
		{
		FreeLibrary(physics);
		return nxFail("NxFoundation.dll is not loaded");
		}
	nxNormalToTangents = reinterpret_cast<NormalToTangentsFn>(
		GetProcAddress(foundation, "NxNormalToTangents"));
	printf("tangent export=NxNormalToTangents present=%s\n", nxNormalToTangents ? "yes" : "no");
	if(!nxNormalToTangents)
		{
		FreeLibrary(physics);
		return nxFail("NxNormalToTangents is missing; the differential cannot run");
		}

	// The n.z arm. Unit and non-unit, a zero n.x (the one input where the
	// rebuild's extra factor of k on n.x could not show), and both signs.
	nxShow("z_axis", 0x00000000u, 0x00000000u, 0x3f800000u);				// (0,0,1)
	nxShow("z_axis_neg", 0x00000000u, 0x00000000u, 0xbf800000u);			// (0,0,-1)
	nxShow("z_x_zero", 0x00000000u, 0x3f19999au, 0x3f4ccccdu);				// (0,0.6,0.8)
	nxShow("z_unit", 0x3e99999au, 0x3e4ccccdu, 0x3f666666u);				// (0.3,0.2,0.9)
	nxShow("z_unit_neg", 0xbe99999au, 0x3e4ccccdu, 0xbf666666u);			// (-0.3,0.2,-0.9)
	nxShow("z_nonunit", 0x40400000u, 0x40800000u, 0x41400000u);				// (3,4,12)
	nxShow("z_nonunit_neg", 0xc0a00000u, 0x40000000u, 0xc1100000u);		// (-5,2,-9)
	nxShow("z_nonunit_mixed", 0x40400000u, 0xc0800000u, 0x40a00000u);		// (3,-4,5)
	nxShow("z_large", 0x60ad78ecu, 0x612d78ecu, 0x61d8d727u);				// (1e20,2e20,~5e20): y*y+z*z overflows the float spill
	nxShow("z_above", 0x3f000000u, 0x3f000000u, nxAboveThreshold);			// |z| just above
	nxShow("z_above_neg", 0x3f000000u, 0x3f000000u, nxAboveThreshold | 0x80000000u);
	nxShow("z_above_x_zero", 0x00000000u, 0x3f3504f3u, nxAboveThreshold);

	// The n.x/n.y arm, including the threshold's other side.
	nxShow("xy_x_axis", 0x3f800000u, 0x00000000u, 0x00000000u);			// (1,0,0)
	nxShow("xy_y_axis", 0x00000000u, 0x3f800000u, 0x00000000u);			// (0,1,0)
	nxShow("xy_unit", 0x3f19999au, 0x3f4ccccdu, 0x00000000u);				// (0.6,0.8,0)
	nxShow("xy_diagonal", 0x3f13cd3au, 0x3f13cd3au, 0x3f13cd3au);			// case 0's axis
	nxShow("xy_nonunit", 0x3f000000u, 0x3f000000u, 0x3f000000u);			// (0.5,0.5,0.5)
	nxShow("xy_nonunit_neg", 0x40400000u, 0xc0800000u, 0x3f000000u);		// (3,-4,0.5)
	nxShow("xy_small", 0x2edbe6ffu, 0x2f5be6ffu, 0x3089705fu);				// (1e-10,2e-10,1e-9): |n.z| is absolute
	nxShow("xy_large", 0x60ad78ecu, 0x612d78ecu, 0x3f000000u);				// (1e20,2e20,0.5): a overflows no float here
	nxShow("xy_x_zero", 0x00000000u, 0x3f4ccccdu, 0x3f19999au);			// (0,0.8,0.6)
	nxShow("xy_below", 0x3f000000u, 0x3f000000u, nxBelowThreshold);		// |z| just below
	nxShow("xy_below_neg", 0x3f000000u, 0x3f000000u, nxBelowThreshold | 0x80000000u);
	nxShow("xy_below_x_zero", 0x00000000u, 0x3f3504f4u, nxBelowThreshold);
	nxShow("xy_denormal", 0x000116c2u, 0x00000000u, 0x00000000u);			// (1e-40,0,0)

	// Degenerate and non-finite inputs: these decide the NaN signs the joint
	// differential sees, and the two-payload case is the one where x87 and SSE
	// propagate different NaNs.
	nxShow("zero", 0x00000000u, 0x00000000u, 0x00000000u);
	nxShow("zero_neg", 0x80000000u, 0x80000000u, 0x80000000u);
	nxShow("z_inf", 0x00000000u, 0x00000000u, 0x7f800000u);
	nxShow("z_inf_y", 0x00000000u, 0x3f800000u, 0x7f800000u);
	nxShow("x_inf", 0x7f800000u, 0x00000000u, 0x00000000u);
	nxShow("z_nan", 0x00000000u, 0x00000000u, 0x7fc00000u);
	nxShow("x_nan", 0x7fc00000u, 0x00000000u, 0x00000000u);
	nxShow("x_nan_neg", 0xffc00000u, 0x3f000000u, 0x00000000u);
	nxShow("xy_nan_payloads", 0x7fc12345u, 0x7fc54321u, 0x00000000u);
	nxShow("yz_nan_payloads", 0x3f000000u, 0x7fc12345u, 0x7fc54321u);

	// The sweep. Three bands: unit-scale inputs over both arms, a band aimed
	// at the threshold (|n.z| within 64 ULPs of 1/sqrt(2), with n.x and n.y
	// random), and non-unit inputs spread over many binades.
	const NxU32 unitCount = 120000;
	const NxU32 thresholdCount = 60000;
	const NxU32 scaledCount = 60000;
	NxU32 digest = 2166136261u;
	for(NxU32 i = 0; i < unitCount; ++i)
		digest = nxFoldCase(digest, NxVec3(nxUniform(1.0f), nxUniform(1.0f), nxUniform(1.0f)));
	for(NxU32 i = 0; i < thresholdCount; ++i)
		{
		NxU32 zBits = nxBelowThreshold - 64u + (nxNext() & 127u);
		if(nxNext() & 1u)
			zBits |= 0x80000000u;
		const float x = (nxNext() & 3u) == 0u ? 0.0f : nxUniform(0.7f);
		digest = nxFoldCase(digest, NxVec3(x, nxUniform(0.7f), nxFloat(zBits)));
		}
	for(NxU32 i = 0; i < scaledCount; ++i)
		{
		// A power of two between 2^-40 and 2^40, so the products stay finite
		// in a double and the arm split is the same as at unit scale.
		const float scale = nxFloat((127u - 40u + (nxNext() % 81u)) << 23);
		digest = nxFoldCase(digest, NxVec3(nxUniform(scale), nxUniform(scale), nxUniform(scale)));
		}
	printf("tangent sweep unit=%u threshold=%u scaled=%u digest=%08x\n",
		unitCount, thresholdCount, scaledCount, digest);
	printf("tangent coverage arm_z=%u arm_xy=%u\n", nxArmZ, nxArmXY);

	return nxReportPairIdentity(pairDirectory);
	}
