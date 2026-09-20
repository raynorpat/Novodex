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
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsJointDescTests", pairDirectory, &physics);
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

	// Case 1: a zero axis, which the row must leave alone rather than divide by.
	nxCase(setAnchor, setAxis, 1, 0, 0,
		NxVec3(0.0f, 0.0f, 0.0f), NxVec3(0.0f, 0.0f, 0.0f));

	// Case 2: a NaN axis, to pin what the row does with a non-finite input. The
	// bits are supplied directly rather than through an arithmetic expression,
	// because 3z266's lesson is that a generator must not do arithmetic on a
	// value it also allows to be non-finite.
	NxVec3 nanAxis;
	memcpy(&nanAxis.x, "\x00\x00\xc0\x7f", 4);		// 0x7fc00000
	nanAxis.y = 0.0f;
	nanAxis.z = 0.0f;
	nxCase(setAnchor, setAxis, 2, 0, 0, NxVec3(-0.0f, 0.0f, 0.0f), nanAxis);

	// Case 3: an already-unit axis, so the normalisation is an identity.
	nxCase(setAnchor, setAxis, 3, 0, 0,
		NxVec3(1.0f, 0.0f, 0.0f), NxVec3(0.0f, 1.0f, 0.0f));

	return nxReportPairIdentity(pairDirectory);
	}