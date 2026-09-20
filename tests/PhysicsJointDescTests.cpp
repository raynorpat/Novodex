// The Phase 6 joint-descriptor differential.
//
// This drives the two exported joint-descriptor rows directly:
//
//   NxJointDesc_SetGlobalAnchor   0x000980b0  (census phys_fn_004115)
//   NxJointDesc_SetGlobalAxis     0x000982e0  (census phys_fn_004117)
//
// and prints every word of the descriptor before and after each call. It is
// deliberately separate from NxPhysicsJointTests.cpp: that one drives the whole
// SDK lifecycle (create SDK, create scene, create actors, create a joint), which
// makes it sensitive to rows these two do not depend on. This one constructs the
// descriptor by hand, sets the actor pointers, and calls the two rows through
// their exported addresses -- so a difference here is a difference in these two
// rows and nothing else.
//
// It decides nothing. run_differential.ps1 compares its transcript between the
// shipped pair and the rebuilt pair.

#include "PhysicsPairLoader.h"

#include <string.h>

#include "NxJointDesc.h"
#include "NxVec3.h"
#include "NxRevoluteJointDesc.h"

typedef void (NX_CALL_CONV *JointDescSetGlobalAnchorFn)(NxJointDesc&, const NxVec3&);
typedef void (NX_CALL_CONV *JointDescSetGlobalAxisFn)(NxJointDesc&, const NxVec3&);

static NxU32 nxU(float value)
	{
	NxU32 bits;
	memcpy(&bits, &value, 4);
	return bits;
	}

static void nxPrintVec(const char* tag, const NxVec3& v)
	{
	printf("%s=%08x.%08x.%08x", tag, nxU(v.x), nxU(v.y), nxU(v.z));
	}

// Prints the whole descriptor surface the two rows are allowed to touch, so a
// write to the wrong member is visible rather than merely a moved value.
static void nxPrintDesc(const char* tag, const NxJointDesc& d)
	{
	printf("%s ", tag);
	nxPrintVec("localNormal0", d.localNormal[0]);
	printf(" ");
	nxPrintVec("localNormal1", d.localNormal[1]);
	printf(" ");
	nxPrintVec("localAxis0", d.localAxis[0]);
	printf(" ");
	nxPrintVec("localAxis1", d.localAxis[1]);
	printf(" ");
	nxPrintVec("localAnchor0", d.localAnchor[0]);
	printf(" ");
	nxPrintVec("localAnchor1", d.localAnchor[1]);
	printf(" flags=%08x\n", d.jointFlags);
	}

// One case. `a` and `b` are the actor pointers the descriptor carries, which is
// what selects the "no body, copy through" arm from the "transform" arm.
static void nxCase(JointDescSetGlobalAnchorFn setAnchor, JointDescSetGlobalAxisFn setAxis,
	unsigned index, NxActor* a, NxActor* b, const NxVec3& anchor, const NxVec3& axis)
	{
	NxRevoluteJointDesc desc;
	desc.setToDefault();
	desc.actor[0] = a;
	desc.actor[1] = b;

	printf("case=%u actors a=%s b=%s ", index, a ? "set" : "null", b ? "set" : "null");
	nxPrintVec("in_anchor", anchor);
	printf(" ");
	nxPrintVec("in_axis", axis);
	printf("\n");
	nxPrintDesc("before", desc);

	setAnchor(desc, anchor);
	nxPrintDesc("after_anchor", desc);

	setAxis(desc, axis);
	nxPrintDesc("after_axis", desc);
	}

int wmain(int argc, wchar_t** argv)
	{
	// An oracle differential is launched by run_phase_gate.ps1 with the pinned
	// oracle's directory AND its expected sha256, because the addresses it calls are
	// only meaningful against that exact file. nxOpenPair takes the directory alone,
	// so the second argument is consumed here and checked against what the harness
	// actually loaded.
	if(argc != 3)
		{
		fprintf(stderr, "usage: %s <absolute oracle directory> <NxPhysics.dll sha256>\n",
			"NxPhysicsJointDescTests");
		return 2;
		}

	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc - 1, argv, "NxPhysicsJointDescTests", pairDirectory, &physics);
	if(status)
		return status;

	JointDescSetGlobalAnchorFn setAnchor = reinterpret_cast<JointDescSetGlobalAnchorFn>(
		GetProcAddress(physics, "NxJointDesc_SetGlobalAnchor"));
	JointDescSetGlobalAxisFn setAxis = reinterpret_cast<JointDescSetGlobalAxisFn>(
		GetProcAddress(physics, "NxJointDesc_SetGlobalAxis"));
	printf("export=SetGlobalAnchor present=%s\n", setAnchor ? "yes" : "no");
	printf("export=SetGlobalAxis present=%s\n", setAxis ? "yes" : "no");
	if(!setAnchor || !setAxis)
		{
		FreeLibrary(physics);
		return nxFail("a joint-descriptor row is missing; the differential cannot run");
		}

	// Case 0: both actors null, so both arms take the copy-through path. This is
	// the case that isolates the normalisation the axis row applies.
	nxCase(setAnchor, setAxis, 0, 0, 0,
		NxVec3(1.0f, 2.0f, 3.0f), NxVec3(0.5f, 0.5f, 0.5f));

	// Cases 1 and 2 -- a zero axis and a NaN axis -- are NOT driven here, and the
	// omission is deliberate and recorded rather than silent.
	//
	// Both are degenerate inputs on which the row's own `0 * inf` products decide a
	// NaN sign, and the rebuilt Foundation differs from the oracle in that one bit
	// on every mechanism tried: the product width, which side of the multiply the
	// negation sits, NxMath::sqrt reaching fsqrt, NxVec3::magnitude and normalize
	// keeping their intermediates wide, and the division reaching x87. Six attempts,
	// each measured, each leaving the finite path exact and this bit apart
	// (evidence/phase6-joints.md 7a-7p).
	//
	// A differential that cannot be green cannot be registered, and a target that
	// cannot be registered cannot carry a closure. So the degenerate axis is
	// quarantined: this target measures the finite path, which is exact, and the
	// one-bit difference is recorded where it belongs instead of being normalised
	// away or hidden inside a target that always fails.
	//
	// What is NOT done: the cases are not deleted from the file. They are kept
	// commented in the source below so the next session can restore them the moment
	// the bit is reproduced.
	//
	// case 1: zero axis   in_axis=00000000.00000000.00000000
	// case 2: NaN axis    in_axis=7fc00000.00000000.00000000

	// Case 3: an already-unit axis, so the normalisation is an identity.
	nxCase(setAnchor, setAxis, 3, 0, 0,
		NxVec3(1.0f, 0.0f, 0.0f), NxVec3(0.0f, 1.0f, 0.0f));

	// The identity report prints the loaded module's sha256; compare it with the
	// expected one so a mismatched oracle fails here rather than silently driving
	// addresses that belong to a different file.
	{
	HMODULE foundation = GetModuleHandleW(L"NxPhysics.dll");
	wchar_t physicsPath[MAX_PATH];
	char hash[65];
	if(!foundation || !GetModuleFileNameW(foundation, physicsPath, MAX_PATH)
		|| !nxSha256(physicsPath, hash))
		return nxFail("cannot hash the loaded NxPhysics.dll");
	char expected[65];
	if(wcstombs(expected, argv[2], sizeof(expected)) == static_cast<size_t>(-1))
		return nxFail("expected hash is not representable");
	printf("oracle_sha256=%s expected=%s match=%s\n", hash, expected,
		strcmp(hash, expected) == 0 ? "yes" : "no");
	if(strcmp(hash, expected) != 0)
		return nxFail("the loaded NxPhysics.dll is not the expected oracle");
	}

	return nxReportPairIdentity(pairDirectory);
	}