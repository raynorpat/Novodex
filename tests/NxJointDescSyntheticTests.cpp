// A synthetic-actor fixture for the two exported joint-descriptor rows.
//
// The rows reach an actor's world pose through raw pointer arithmetic:
//
//     actor + 0x10               -> NxActorDesc*
//     actorDesc + 8              -> NxArray<NxShapeDesc*>::first
//     *first                     -> NxShapeDesc*
//     shape + 8                  -> NxBodyDesc*
//     body + 0x19c               -> pose
//     pose + 8                   -> cached matrix, or null for the quaternion arm
//     pose + 0x50/0x54/0x58      -> translation
//     pose + 0x5c/0x60/0x64/0x68 -> quaternion x, y, z, w
//
// NxArray is three pointers (first, last, memEnd) then an allocator, so its first
// element sits at +0. Nothing in that chain requires a real actor: the row only
// reads, and both sides read the same bytes. So a byte buffer with the chain built
// into it exercises the transform arm without a Scene, which the candidate does
// not have (NpPhysicsSDK.cpp: createScene "needs Scene, Phase 3").
//
// This is the fixture the mutation in evidence 7s showed was missing: without it
// every case passes a null actor, the row takes its copy-through arm, and a
// mutation aimed at the transform cannot be caught.

#include "PhysicsPairLoader.h"

#include <string.h>

#include "NxJointDesc.h"
#include "NxVec3.h"
#include "NxRevoluteJointDesc.h"

typedef void (NX_CALL_CONV *JointDescSetGlobalAnchorFn)(NxJointDesc&, const NxVec3&);
typedef void (NX_CALL_CONV *JointDescSetGlobalAxisFn)(NxJointDesc&, const NxVec3&);

static NxU32 nxU(float v) { NxU32 b; memcpy(&b, &v, 4); return b; }

static void nxPrintVec(const char* tag, const NxVec3& v)
	{
	printf("%s=%08x.%08x.%08x", tag, nxU(v.x), nxU(v.y), nxU(v.z));
	}

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

// A synthetic actor. The buffer is 0x200 bytes: actor+0x10 holds the descriptor,
// the descriptor's shape array holds one shape, the shape's body pointer is the
// body, and the body's pose is either the cached matrix or the quaternion.
struct SyntheticActor
	{
	unsigned char buffer[0x200];
	unsigned char desc[0x40];
	unsigned char shape[0x40];
	unsigned char body[0x200];
	unsigned char pose[0x80];

	void build(bool quaternionArm, const float* translation, const float* quat,
		const float* matrix)
		{
		memset(buffer, 0, sizeof(buffer));
		memset(desc, 0, sizeof(desc));
		memset(shape, 0, sizeof(shape));
		memset(body, 0, sizeof(body));
		memset(pose, 0, sizeof(pose));

		// actor + 0x10 -> descriptor
		*reinterpret_cast<void**>(buffer + 0x10) = desc;
		// descriptor + 8 -> the shape array's `first`
		*reinterpret_cast<void**>(desc + 8) = shape;
		// shape + 8 -> the body
		*reinterpret_cast<void**>(shape + 8) = body;
		// body + 0x19c -> the pose
		*reinterpret_cast<void**>(body + 0x19c) = pose;

		if(quaternionArm)
			{
			*reinterpret_cast<void**>(pose + 8) = 0;	// no cached matrix
			memcpy(pose + 0x50, translation, 12);
			memcpy(pose + 0x5c, quat, 16);
			}
		else
			{
			// A cached matrix: 3x3 then the translation at +0x24.
			memcpy(pose + 0x08, matrix, 36);
			memcpy(pose + 0x08 + 36, translation, 12);
			*reinterpret_cast<void**>(pose + 8) = pose + 0x08;
			}
		}

	NxActor* actor() { return reinterpret_cast<NxActor*>(buffer); }
	};

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

	setAnchor(desc, anchor);
	nxPrintDesc("after_anchor", desc);
	setAxis(desc, axis);
	nxPrintDesc("after_axis", desc);
	}

int wmain(int argc, wchar_t** argv)
	{
	if(argc != 3)
		{
		fprintf(stderr, "usage: %s <absolute oracle directory> <NxPhysics.dll sha256>\n",
			"NxJointDescSyntheticTests");
		return 2;
		}

	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc - 1, argv, "NxJointDescSyntheticTests", pairDirectory, &physics);
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
		return nxFail("a joint-descriptor row is missing");
		}

	// Identity translation and quaternion: the pose is the identity, so the
	// transform is a pure copy and a reader can check the result by eye.
	const float t0[3] = { 0.0f, 0.0f, 0.0f };
	const float qId[4] = { 0.0f, 0.0f, 0.0f, 1.0f };	// x, y, z, w
	const float mId[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };

	SyntheticActor quatActor;
	quatActor.build(true, t0, qId, 0);
	printf("fixture=quaternion\n");
	nxCase(setAnchor, setAxis, 0, quatActor.actor(), 0,
		NxVec3(1.0f, 2.0f, 3.0f), NxVec3(0.0f, 1.0f, 0.0f));

	SyntheticActor matActor;
	matActor.build(false, t0, 0, mId);
	printf("fixture=cached_matrix\n");
	nxCase(setAnchor, setAxis, 1, matActor.actor(), 0,
		NxVec3(1.0f, 2.0f, 3.0f), NxVec3(0.0f, 1.0f, 0.0f));

	// A translated pose, so the subtraction is exercised rather than an identity.
	const float t1[3] = { 10.0f, 20.0f, 30.0f };
	SyntheticActor movedActor;
	movedActor.build(true, t1, qId, 0);
	printf("fixture=translated\n");
	nxCase(setAnchor, setAxis, 2, movedActor.actor(), 0,
		NxVec3(11.0f, 22.0f, 33.0f), NxVec3(1.0f, 0.0f, 0.0f));

	return nxReportPairIdentity(pairDirectory);
	}