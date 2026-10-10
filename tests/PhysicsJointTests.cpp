// The Phase 6 joint differential.
//
// It drives the exported SDK over a fixed matrix of joint cases and prints, for
// every case, the input words it passed, the return value and every output word,
// all as raw 32-bit hexadecimal. Nothing here decides whether a result is right:
// the oracle decides, by run_differential.ps1 comparing this transcript from the
// shipped pair against the same transcript from the rebuilt pair.
//
// WHAT THIS FILE IS AND IS NOT, as of its first version:
//
//   * it is the harness Phase 6 needs, and it exists because Phase 6's own plan
//     calls for tests/PhysicsJointTests.cpp;
//   * it is a TRANSCRIPT generator for the revolute family and, since
//     joint-families Tasks 3a-3g, the prismatic, cylindrical, spherical,
//     point-on-line, point-in-plane, distance and pulley families. The other
//     two families -- D6 and fixed -- are not driven yet, and the file says so
//     rather than reporting a coverage number that overstates;
//   * it is registered as an oracle differential only when its transcript is
//     judged stable, which is a separate step.
//
// The reason it is a new translation unit rather than more cases in
// PhysicsObjectLayoutTests.cpp: that harness's frame is ~250 KB and a 0xA5
// poisoned jump follows any change to it, which is recorded across
// evidence/phase5-object-model.md 3z262-3z289. A new file has no such history.
//
// Every output value is printed as its IEEE-754 bit pattern rather than as a
// decimal literal, because the comparison is bit-exact and decimal does not
// round-trip.

#include "PhysicsPairLoader.h"
#include "NxPageGuardedAllocator.h"

#include <math.h>
#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxUserOutputStream.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxJoint.h"
#include "NxJointDesc.h"
#include "NxRevoluteJoint.h"
#include "NxRevoluteJointDesc.h"
#include "NxPrismaticJoint.h"
#include "NxPrismaticJointDesc.h"
#include "NxCylindricalJoint.h"
#include "NxCylindricalJointDesc.h"
#include "NxSphericalJoint.h"
#include "NxSphericalJointDesc.h"
#include "NxPointOnLineJoint.h"
#include "NxPointOnLineJointDesc.h"
#include "NxPointInPlaneJoint.h"
#include "NxPointInPlaneJointDesc.h"
#include "NxDistanceJoint.h"
#include "NxDistanceJointDesc.h"
#include "NxPulleyJoint.h"
#include "NxPulleyJointDesc.h"
#include "NxFixedJoint.h"
#include "NxFixedJointDesc.h"
#include "NxBitField.h"
#include "NxD6Joint.h"
#include "NxD6JointDesc.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

extern "C" unsigned __cdecl nxJointAbiInvokeSretProbe(void* target,
	void* self, void* result);

extern "C" {
volatile unsigned nxJointAbiSretBaseline = 0;
volatile unsigned nxJointAbiSretFlags = 0;
void* volatile nxJointAbiSretTarget = 0;
void* volatile nxJointAbiSretThis = 0;
void* volatile nxJointAbiSretResult = 0;
}

// Raw x86 __thiscall call for an NxVec3 return: ECX carries `this`, the hidden
// result pointer is the sole stack argument, and the callee must pop four bytes.
extern "C" __declspec(naked) unsigned __cdecl nxJointAbiInvokeSretProbe(
	void*, void*, void*)
	{
	__asm {
		push ebp
		mov ebp, esp
		push ebx
		push esi
		push edi
		mov dword ptr [nxJointAbiSretBaseline], esp
		mov eax, dword ptr [ebp + 8]
		mov dword ptr [nxJointAbiSretTarget], eax
		mov eax, dword ptr [ebp + 12]
		mov dword ptr [nxJointAbiSretThis], eax
		mov eax, dword ptr [ebp + 16]
		mov dword ptr [nxJointAbiSretResult], eax
		mov dword ptr [nxJointAbiSretFlags], 0
		mov ebx, 0x6b13579b
		mov esi, 0x6c2468ac
		mov edi, 0x6d3579bd
		mov ebp, 0x6e468ace
		push dword ptr [nxJointAbiSretResult]
		mov ecx, dword ptr [nxJointAbiSretThis]
		call dword ptr [nxJointAbiSretTarget]
		cmp esp, dword ptr [nxJointAbiSretBaseline]
		je joint_sret_stack_ok
		or dword ptr [nxJointAbiSretFlags], 1
	joint_sret_stack_ok:
		cmp ebx, 0x6b13579b
		je joint_sret_ebx_ok
		or dword ptr [nxJointAbiSretFlags], 2
	joint_sret_ebx_ok:
		cmp esi, 0x6c2468ac
		je joint_sret_esi_ok
		or dword ptr [nxJointAbiSretFlags], 4
	joint_sret_esi_ok:
		cmp edi, 0x6d3579bd
		je joint_sret_edi_ok
		or dword ptr [nxJointAbiSretFlags], 8
	joint_sret_edi_ok:
		cmp ebp, 0x6e468ace
		je joint_sret_ebp_ok
		or dword ptr [nxJointAbiSretFlags], 16
	joint_sret_ebp_ok:
		mov esp, dword ptr [nxJointAbiSretBaseline]
		pop edi
		pop esi
		pop ebx
		pop ebp
		mov eax, dword ptr [nxJointAbiSretFlags]
		ret
	}
}

static NxU32 nxU(NxReal value);

static void nxProbeJointSret(const char* family, unsigned index, const NxJoint* joint)
	{
	const NxVec3 expectedAnchor = joint->getGlobalAnchorVal();
	const NxVec3 expectedAxis = joint->getGlobalAxisVal();
	NxVec3 rawAnchor, rawAxis;
	memset(&rawAnchor, 0xcd, sizeof(rawAnchor));
	memset(&rawAxis, 0xcd, sizeof(rawAxis));
	void** vtable = *reinterpret_cast<void***>(const_cast<NxJoint*>(joint));
	const unsigned anchorFlags = nxJointAbiInvokeSretProbe(vtable[6],
		const_cast<NxJoint*>(joint), &rawAnchor);
	const unsigned axisFlags = nxJointAbiInvokeSretProbe(vtable[7],
		const_cast<NxJoint*>(joint), &rawAxis);
	const unsigned flags = anchorFlags | axisFlags;
	const unsigned mismatches = memcmp(&expectedAnchor, &rawAnchor, sizeof(NxVec3)) != 0 ||
		memcmp(&expectedAxis, &rawAxis, sizeof(NxVec3)) != 0;
	printf("case=%s index=%u abi_sret cases=2 flags=%x mismatches=%u\n",
		family, index, flags, mismatches);
	printf("case=%s index=%u abi_sret_values anchor=%08x.%08x.%08x axis=%08x.%08x.%08x\n",
		family, index, nxU(expectedAnchor.x), nxU(expectedAnchor.y), nxU(expectedAnchor.z),
		nxU(expectedAxis.x), nxU(expectedAxis.y), nxU(expectedAxis.z));
	}

// NxJointDesc::setGlobalAnchor and setGlobalAxis are inline and call these two
// exported rows, so using the inline methods would add an import for them. This
// harness loads the pair by LoadLibraryEx and must not link against either side's
// import library, so the two rows are resolved at runtime like every other
// address it calls.
typedef void (NX_CALL_CONV *JointDescSetGlobalAnchorFn)(NxJointDesc&, const NxVec3&);
typedef void (NX_CALL_CONV *JointDescSetGlobalAxisFn)(NxJointDesc&, const NxVec3&);

static JointDescSetGlobalAnchorFn nxSetGlobalAnchor = 0;
static JointDescSetGlobalAxisFn nxSetGlobalAxis = 0;

static NxU32 nxU(NxReal value)
	{
	NxU32 bits;
	memcpy(&bits, &value, 4);
	return bits;
	}

// A vector is printed as three raw words, never as decimal.
static void nxPrintVec(const char* tag, const NxVec3& v)
	{
	printf("%s=%08x.%08x.%08x", tag, nxU(v.x), nxU(v.y), nxU(v.z));
	}

class NxJointErrorStream : public NxUserOutputStream
	{
	public:
	NxJointErrorStream() : enabled(false), reports(0), lastCode(NXE_NO_ERROR), lastLine(0) {}
	void reportError(NxErrorCode code, const char*, const char*, int line)
		{ if(enabled) { ++reports; lastCode = code; lastLine = line; } }
	NxAssertResponse reportAssertViolation(const char*, const char*, int)
		{ return NX_AR_CONTINUE; }
	void print(const char*) {}
	void reset() { reports = 0; lastCode = NXE_NO_ERROR; lastLine = 0; }
	bool enabled;
	unsigned reports;
	NxErrorCode lastCode;
	int lastLine;
	};

static NxJointErrorStream jointErrorStream;

static unsigned* nxSceneWriteLockState(NxScene& scene)
	{
	// Match the scene wrapper's +0x0c link and the lock words at block +0x18.
	unsigned char* link = *reinterpret_cast<unsigned char**>(reinterpret_cast<unsigned char*>(&scene) + 0x0c);
	unsigned char* block = *reinterpret_cast<unsigned char**>(link);
	return reinterpret_cast<unsigned*>(block + 0x18);
	}

// Builds the two-actor fixture every joint case needs. The bodies are dynamic
// because a joint needs at least one dynamic actor and neither may be static.
//
// The density matters: NxActorDesc::isValid() wants either a body with a mass
// AND a mass-space inertia, or a non-zero density with at least one shape, and a
// default NxBodyDesc carries mass 0 with a zero inertia. The first version of
// this fixture set neither, so createActor returned null and the harness failed
// before it reached a single joint. Density with shapes is the simpler of the two
// validity routes and the one this fixture takes.
static bool nxBuildFixture(NxScene& scene, NxActor** a, NxActor** b)
	{
	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 1.0f, 1.0f);

	NxBodyDesc body;

	NxActorDesc da;
	da.body = &body;
	da.density = 1.0f;
	da.shapes.pushBack(&box);
	da.globalPose.t = NxVec3(0.0f, 0.0f, 0.0f);
	*a = scene.createActor(da);
	if(!*a)
		return false;

	NxBoxShapeDesc box2;
	box2.dimensions = NxVec3(1.0f, 1.0f, 1.0f);
	NxBodyDesc body2;
	NxActorDesc db;
	db.body = &body2;
	db.density = 1.0f;
	db.shapes.pushBack(&box2);
	db.globalPose.t = NxVec3(4.0f, 0.0f, 0.0f);
	*b = scene.createActor(db);
	return *b != 0;
	}

// What the scene reports about its joints (joint-open-items Task 2): the
// getNbJoints count, then one pass of resetJointIterator/getNextJoint -- how
// many joints it yields, the type of each in order, and whether `joint` is
// among them. `joint` is only compared, never dereferenced, so the pass after
// its release is safe. The pass stops at 64 so a cycle in the list cannot hang
// the run; one more getNextJoint after the end is printed too.
static void nxPrintSceneJoints(NxScene& scene, const char* family, unsigned index,
	const char* when, const NxJoint* joint)
	{
	const NxU32 count = scene.getNbJoints();
	scene.resetJointIterator();
	NxU32 enumerated = 0;
	bool found = false;
	char order[256];
	order[0] = 0;
	int used = 0;
	while(NxJoint* next = scene.getNextJoint())
		{
		if(next == joint)
			found = true;
		if(used + 16 < static_cast<int>(sizeof(order)))
			used += sprintf(order + used, "%s%u", enumerated ? "." : "",
				static_cast<unsigned>(next->getType()));
		if(++enumerated == 64)
			break;
		}
	const bool endIsNull = scene.getNextJoint() == 0;
	printf("case=%s index=%u scene_joints when=%s count=%u enumerated=%u order=%s self=%s end=%s\n",
		family, index, when, static_cast<unsigned>(count), static_cast<unsigned>(enumerated),
		enumerated ? order : "none", found ? "yes" : "no", endIsNull ? "null" : "joint");
	}

// The internal joint's orientation-dependent words (joint-open-items Task 4),
// printed only for the Task 4 cases (nxPrintInternal). The public object keeps
// the internal joint at +0x18. Printed: the Joint base block +0x4c..+0x14b
// (local normal/cross/axis/anchor pairs, the two frame quaternions, their
// world copies and the two body stamps; floats and counters only, no
// pointers) and each family's own words from +0x16c to its size, which hold
// the relative rotations that 004378 (prismatic) and 004244 (fixed) record
// at creation. Pulley's +0x16c block is skipped: its lever words are left
// uninitialised by the oracle (joint-families.md "Oracle quirks reproduced").
static bool nxPrintInternal = false;

static void nxPrintInternalWords(const char* family, unsigned index, const NxJoint* joint)
	{
	nxProbeJointSret(family, index, joint);
	if(!nxPrintInternal)
		return;
	const unsigned char* np = reinterpret_cast<const unsigned char*>(joint);
	const unsigned char* internal = *reinterpret_cast<const unsigned char* const*>(np + 0x18);
	unsigned size = 0x16c;
	switch(joint->getType())
		{
		case NX_JOINT_PRISMATIC:	size = 0x17c; break;
		case NX_JOINT_REVOLUTE:		size = 0x204; break;
		case NX_JOINT_SPHERICAL:	size = 0x23c; break;
		case NX_JOINT_DISTANCE:		size = 0x184; break;
		case NX_JOINT_FIXED:		size = 0x188; break;
		case NX_JOINT_D6:			size = 0x270; break;
		default:					size = 0x16c; break;
		}
	const unsigned ranges[2][2] = { { 0x4c, 0x14c }, { 0x16c, size } };
	for(unsigned r = 0; r < 2; r++)
		for(unsigned off = ranges[r][0]; off < ranges[r][1]; off += 0x20)
			{
			printf("case=%s index=%u internal off=%03x words=", family, index, off);
			for(unsigned w = off; w < off + 0x20 && w < ranges[r][1]; w += 4)
				{
				NxU32 word;
				memcpy(&word, internal + w, 4);
				printf("%s%08x", w == off ? "" : ".", static_cast<unsigned>(word));
				}
			printf("\n");
			}
	}

// The saved local frames of any family's saveToDesc: the anchors, axes and
// normals in each body's frame (joint-open-items Task 4).
static void nxPrintSavedFrames(const char* family, unsigned index, const NxJointDesc& saved)
	{
	printf("case=%s index=%u saved ", family, index);
	nxPrintVec("anchor0", saved.localAnchor[0]);
	printf(" ");
	nxPrintVec("anchor1", saved.localAnchor[1]);
	printf("\n");
	printf("case=%s index=%u saved ", family, index);
	nxPrintVec("axis0", saved.localAxis[0]);
	printf(" ");
	nxPrintVec("axis1", saved.localAxis[1]);
	printf("\n");
	printf("case=%s index=%u saved ", family, index);
	nxPrintVec("normal0", saved.localNormal[0]);
	printf(" ");
	nxPrintVec("normal1", saved.localNormal[1]);
	printf("\n");
	}

// One revolute case: build the descriptor, create, read every value back, then
// release. Nothing is asserted; everything is printed.
static void nxRevoluteCase(NxScene& scene, NxActor* a, NxActor* b,
	unsigned index, const NxVec3& anchor, const NxVec3& axis)
	{
	printf("case=revolute index=%u ", index);
	nxPrintVec("in_anchor", anchor);
	printf(" ");
	nxPrintVec("in_axis", axis);
	printf("\n");

	NxRevoluteJointDesc desc;
	desc.setToDefault();
	desc.actor[0] = a;
	desc.actor[1] = b;
	nxSetGlobalAnchor(desc, anchor);
	nxSetGlobalAxis(desc, axis);

	NxJoint* joint = scene.createJoint(desc);
	printf("case=revolute index=%u created=%s\n", index, joint ? "yes" : "no");
	if(!joint)
		return;

	NxVec3 gotAnchor(0.0f, 0.0f, 0.0f);
	NxVec3 gotAxis(0.0f, 0.0f, 0.0f);
	joint->getGlobalAnchor(gotAnchor);
	joint->getGlobalAxis(gotAxis);
	printf("case=revolute index=%u ", index);
	nxPrintVec("out_anchor", gotAnchor);
	printf(" ");
	nxPrintVec("out_axis", gotAxis);
	printf(" state=%u\n", static_cast<unsigned>(joint->getState()));
	nxPrintInternalWords("revolute", index, joint);

	NxActor* ra = 0;
	NxActor* rb = 0;
	joint->getActors(&ra, &rb);
	printf("case=revolute index=%u actors a=%s b=%s\n", index,
		ra == a ? "match" : (ra ? "other" : "null"),
		rb == b ? "match" : (rb ? "other" : "null"));

	// The saved local frames (joint-open-items Task 4): over rotated bodies
	// these are the words that depend on the body orientation.
	NxRevoluteJoint* revolute = joint->isRevoluteJoint();
	if(revolute)
		{
		printf("case=revolute index=%u flags=%08x\n", index,
			static_cast<unsigned>(revolute->getFlags()));
		NxJointLimitPairDesc limits;
		const bool limitsEnabled = revolute->getLimits(limits);
		printf("case=revolute index=%u limits enabled=%u low=%08x.%08x.%08x high=%08x.%08x.%08x\n",
			index, limitsEnabled ? 1u : 0u, nxU(limits.low.value),
			nxU(limits.low.restitution), nxU(limits.low.hardness),
			nxU(limits.high.value), nxU(limits.high.restitution), nxU(limits.high.hardness));
		NxMotorDesc motor;
		const bool motorEnabled = revolute->getMotor(motor);
		printf("case=revolute index=%u motor enabled=%u values=%08x.%08x.%08x\n",
			index, motorEnabled ? 1u : 0u, nxU(motor.velTarget), nxU(motor.maxForce),
			static_cast<unsigned>(motor.freeSpin));
		NxSpringDesc spring;
		const bool springEnabled = revolute->getSpring(spring);
		printf("case=revolute index=%u spring enabled=%u values=%08x.%08x.%08x\n",
			index, springEnabled ? 1u : 0u, nxU(spring.spring), nxU(spring.damper),
			nxU(spring.targetValue));
		if(index == 0)
			{
			NxJointLimitPairDesc updatedLimits;
			updatedLimits.low.value = 1.25f;
			updatedLimits.low.restitution = 0.5f;
			updatedLimits.low.hardness = 0.75f;
			updatedLimits.high.value = 2.5f;
			updatedLimits.high.restitution = 0.25f;
			updatedLimits.high.hardness = 1.0f;
			revolute->setLimits(updatedLimits);
			const NxMotorDesc updatedMotor(2.5f, 5.0f, true);
			revolute->setMotor(updatedMotor);
			const NxSpringDesc updatedSpring(10.0f, 0.5f, -0.25f);
			revolute->setSpring(updatedSpring);

			NxJointLimitPairDesc gotLimits;
			const bool gotLimitsEnabled = revolute->getLimits(gotLimits);
			NxMotorDesc gotMotor;
			const bool gotMotorEnabled = revolute->getMotor(gotMotor);
			NxSpringDesc gotSpring;
			const bool gotSpringEnabled = revolute->getSpring(gotSpring);
			printf("case=revolute index=0 updated limits enabled=%u low=%08x.%08x.%08x high=%08x.%08x.%08x\n",
				gotLimitsEnabled ? 1u : 0u, nxU(gotLimits.low.value),
				nxU(gotLimits.low.restitution), nxU(gotLimits.low.hardness),
				nxU(gotLimits.high.value), nxU(gotLimits.high.restitution),
				nxU(gotLimits.high.hardness));
			printf("case=revolute index=0 updated motor enabled=%u values=%08x.%08x.%08x\n",
				gotMotorEnabled ? 1u : 0u, nxU(gotMotor.velTarget),
				nxU(gotMotor.maxForce), static_cast<unsigned>(gotMotor.freeSpin));
			printf("case=revolute index=0 updated spring enabled=%u values=%08x.%08x.%08x\n",
				gotSpringEnabled ? 1u : 0u, nxU(gotSpring.spring),
				nxU(gotSpring.damper), nxU(gotSpring.targetValue));
			printf("case=revolute index=0 updated flags=%08x\n",
				static_cast<unsigned>(revolute->getFlags()));
			}
		NxRevoluteJointDesc saved;
		revolute->saveToDesc(saved);
		nxPrintSavedFrames("revolute", index, saved);
		}

	nxPrintSceneJoints(scene, "revolute", index, "before_release", joint);
	scene.releaseJoint(*joint);
	printf("case=revolute index=%u released=yes\n", index);
	nxPrintSceneJoints(scene, "revolute", index, "after_release", joint);
	}

// One prismatic case (joint-families Task 3a), modelled on nxRevoluteCase: build
// the descriptor over the same two actors, create, read every value the joint
// holds without a simulation step, then release. The family getter is
// saveToDesc (NxPrismaticJoint adds no other method), which returns the base
// descriptor fields the joint stored. Nothing is asserted; everything is
// printed.
static void nxPrismaticCase(NxScene& scene, NxActor* a, NxActor* b,
	unsigned index, const NxVec3& anchor, const NxVec3& axis)
	{
	printf("case=prismatic index=%u ", index);
	nxPrintVec("in_anchor", anchor);
	printf(" ");
	nxPrintVec("in_axis", axis);
	printf("\n");

	NxPrismaticJointDesc desc;
	desc.setToDefault();
	desc.actor[0] = a;
	desc.actor[1] = b;
	nxSetGlobalAnchor(desc, anchor);
	nxSetGlobalAxis(desc, axis);

	NxJoint* joint = scene.createJoint(desc);
	printf("case=prismatic index=%u created=%s\n", index, joint ? "yes" : "no");
	if(!joint)
		return;

	NxVec3 gotAnchor(0.0f, 0.0f, 0.0f);
	NxVec3 gotAxis(0.0f, 0.0f, 0.0f);
	joint->getGlobalAnchor(gotAnchor);
	joint->getGlobalAxis(gotAxis);
	printf("case=prismatic index=%u ", index);
	nxPrintVec("out_anchor", gotAnchor);
	printf(" ");
	nxPrintVec("out_axis", gotAxis);
	printf(" state=%u\n", static_cast<unsigned>(joint->getState()));
	nxPrintInternalWords("prismatic", index, joint);

	NxActor* ra = 0;
	NxActor* rb = 0;
	joint->getActors(&ra, &rb);
	printf("case=prismatic index=%u actors a=%s b=%s\n", index,
		ra == a ? "match" : (ra ? "other" : "null"),
		rb == b ? "match" : (rb ? "other" : "null"));

	NxPrismaticJoint* prismatic = joint->isPrismaticJoint();
	printf("case=prismatic index=%u type=%u is_prismatic=%s is_revolute=%s\n", index,
		static_cast<unsigned>(joint->getType()), prismatic ? "yes" : "no",
		joint->isRevoluteJoint() ? "yes" : "no");
	if(prismatic)
		{
		NxPrismaticJointDesc saved;
		prismatic->saveToDesc(saved);
		printf("case=prismatic index=%u saved ", index);
		nxPrintVec("anchor0", saved.localAnchor[0]);
		printf(" ");
		nxPrintVec("anchor1", saved.localAnchor[1]);
		printf("\n");
		printf("case=prismatic index=%u saved ", index);
		nxPrintVec("axis0", saved.localAxis[0]);
		printf(" ");
		nxPrintVec("axis1", saved.localAxis[1]);
		printf("\n");
		printf("case=prismatic index=%u saved ", index);
		nxPrintVec("normal0", saved.localNormal[0]);
		printf(" ");
		nxPrintVec("normal1", saved.localNormal[1]);
		printf("\n");
		printf("case=prismatic index=%u saved max_force=%08x max_torque=%08x flags=%08x actors a=%s b=%s\n",
			index, nxU(saved.maxForce), nxU(saved.maxTorque), static_cast<unsigned>(saved.jointFlags),
			saved.actor[0] == a ? "match" : (saved.actor[0] ? "other" : "null"),
			saved.actor[1] == b ? "match" : (saved.actor[1] ? "other" : "null"));
		}

	nxPrintSceneJoints(scene, "prismatic", index, "before_release", joint);
	scene.releaseJoint(*joint);
	printf("case=prismatic index=%u released=yes\n", index);
	nxPrintSceneJoints(scene, "prismatic", index, "after_release", joint);
	}

// One cylindrical case (joint-families Task 3b), modelled on nxPrismaticCase: build
// the descriptor over the same two actors, create, read every value the joint
// holds without a simulation step, then release. The family getter is
// saveToDesc (NxCylindricalJoint adds no other method), which returns the base
// descriptor fields the joint stored. Nothing is asserted; everything is
// printed.
static void nxCylindricalCase(NxScene& scene, NxActor* a, NxActor* b,
	unsigned index, const NxVec3& anchor, const NxVec3& axis)
	{
	printf("case=cylindrical index=%u ", index);
	nxPrintVec("in_anchor", anchor);
	printf(" ");
	nxPrintVec("in_axis", axis);
	printf("\n");

	NxCylindricalJointDesc desc;
	desc.setToDefault();
	desc.actor[0] = a;
	desc.actor[1] = b;
	nxSetGlobalAnchor(desc, anchor);
	nxSetGlobalAxis(desc, axis);

	NxJoint* joint = scene.createJoint(desc);
	printf("case=cylindrical index=%u created=%s\n", index, joint ? "yes" : "no");
	if(!joint)
		return;

	NxVec3 gotAnchor(0.0f, 0.0f, 0.0f);
	NxVec3 gotAxis(0.0f, 0.0f, 0.0f);
	joint->getGlobalAnchor(gotAnchor);
	joint->getGlobalAxis(gotAxis);
	printf("case=cylindrical index=%u ", index);
	nxPrintVec("out_anchor", gotAnchor);
	printf(" ");
	nxPrintVec("out_axis", gotAxis);
	printf(" state=%u\n", static_cast<unsigned>(joint->getState()));
	nxPrintInternalWords("cylindrical", index, joint);

	NxActor* ra = 0;
	NxActor* rb = 0;
	joint->getActors(&ra, &rb);
	printf("case=cylindrical index=%u actors a=%s b=%s\n", index,
		ra == a ? "match" : (ra ? "other" : "null"),
		rb == b ? "match" : (rb ? "other" : "null"));

	NxCylindricalJoint* cylindrical = joint->isCylindricalJoint();
	printf("case=cylindrical index=%u type=%u is_cylindrical=%s is_prismatic=%s\n", index,
		static_cast<unsigned>(joint->getType()), cylindrical ? "yes" : "no",
		joint->isPrismaticJoint() ? "yes" : "no");
	if(cylindrical)
		{
		NxCylindricalJointDesc saved;
		cylindrical->saveToDesc(saved);
		printf("case=cylindrical index=%u saved ", index);
		nxPrintVec("anchor0", saved.localAnchor[0]);
		printf(" ");
		nxPrintVec("anchor1", saved.localAnchor[1]);
		printf("\n");
		printf("case=cylindrical index=%u saved ", index);
		nxPrintVec("axis0", saved.localAxis[0]);
		printf(" ");
		nxPrintVec("axis1", saved.localAxis[1]);
		printf("\n");
		printf("case=cylindrical index=%u saved ", index);
		nxPrintVec("normal0", saved.localNormal[0]);
		printf(" ");
		nxPrintVec("normal1", saved.localNormal[1]);
		printf("\n");
		printf("case=cylindrical index=%u saved max_force=%08x max_torque=%08x flags=%08x actors a=%s b=%s\n",
			index, nxU(saved.maxForce), nxU(saved.maxTorque), static_cast<unsigned>(saved.jointFlags),
			saved.actor[0] == a ? "match" : (saved.actor[0] ? "other" : "null"),
			saved.actor[1] == b ? "match" : (saved.actor[1] ? "other" : "null"));
		}

	nxPrintSceneJoints(scene, "cylindrical", index, "before_release", joint);
	scene.releaseJoint(*joint);
	printf("case=cylindrical index=%u released=yes\n", index);
	nxPrintSceneJoints(scene, "cylindrical", index, "after_release", joint);
	}

// One spherical case (joint-families Task 3c), modelled on nxCylindricalCase:
// build a valid NxSphericalJointDesc over the two-actor fixture with the given
// anchor/axis and fixed non-default spherical fields, createJoint, then print
// what the public API reads without a simulation step: the world anchor/axis
// and state, the actors, the type, getFlags/getProjectionMode (internal slots
// 12 and 14) and every field saveToDesc writes back, then releaseJoint.
static void nxSphericalCase(NxScene& scene, NxActor* a, NxActor* b,
	unsigned index, const NxVec3& anchor, const NxVec3& axis)
	{
	printf("case=spherical index=%u ", index);
	nxPrintVec("in_anchor", anchor);
	printf(" ");
	nxPrintVec("in_axis", axis);
	printf("\n");

	NxSphericalJointDesc desc;
	desc.setToDefault();
	desc.actor[0] = a;
	desc.actor[1] = b;
	nxSetGlobalAnchor(desc, anchor);
	nxSetGlobalAxis(desc, axis);
	desc.swingAxis.set(0.6f, 0.0f, 0.8f);
	desc.projectionDistance = 0.25f;
	desc.twistLimit.low.value = -0.5f;
	desc.twistLimit.low.restitution = 0.25f;
	desc.twistLimit.high.value = 0.75f;
	desc.twistLimit.high.hardness = 0.5f;
	desc.swingLimit.value = 0.625f;
	desc.swingLimit.restitution = 0.5f;
	desc.swingLimit.hardness = 0.75f;
	desc.twistSpring.spring = 2.0f;
	desc.twistSpring.damper = 0.5f;
	desc.twistSpring.targetValue = 0.125f;
	desc.swingSpring.spring = 3.0f;
	desc.swingSpring.damper = 1.0f;
	desc.swingSpring.targetValue = 0.375f;
	desc.jointSpring.spring = 4.0f;
	desc.jointSpring.damper = 2.0f;
	desc.flags = NX_SJF_TWIST_LIMIT_ENABLED | NX_SJF_SWING_SPRING_ENABLED;
	desc.projectionMode = NX_JPM_POINT_MINDIST;

	NxJoint* joint = scene.createJoint(desc);
	printf("case=spherical index=%u created=%s\n", index, joint ? "yes" : "no");
	if(!joint)
		return;

	NxVec3 gotAnchor(0.0f, 0.0f, 0.0f);
	NxVec3 gotAxis(0.0f, 0.0f, 0.0f);
	joint->getGlobalAnchor(gotAnchor);
	joint->getGlobalAxis(gotAxis);
	printf("case=spherical index=%u ", index);
	nxPrintVec("out_anchor", gotAnchor);
	printf(" ");
	nxPrintVec("out_axis", gotAxis);
	printf(" state=%u\n", static_cast<unsigned>(joint->getState()));
	nxPrintInternalWords("spherical", index, joint);

	NxActor* ra = 0;
	NxActor* rb = 0;
	joint->getActors(&ra, &rb);
	printf("case=spherical index=%u actors a=%s b=%s\n", index,
		ra == a ? "match" : (ra ? "other" : "null"),
		rb == b ? "match" : (rb ? "other" : "null"));

	NxSphericalJoint* spherical = joint->isSphericalJoint();
	printf("case=spherical index=%u type=%u is_spherical=%s is_revolute=%s\n", index,
		static_cast<unsigned>(joint->getType()), spherical ? "yes" : "no",
		joint->isRevoluteJoint() ? "yes" : "no");
	if(spherical)
		{
		printf("case=spherical index=%u flags=%08x projection_mode=%u\n", index,
			static_cast<unsigned>(spherical->getFlags()),
			static_cast<unsigned>(spherical->getProjectionMode()));

		NxSphericalJointDesc saved;
		spherical->saveToDesc(saved);
		printf("case=spherical index=%u saved ", index);
		nxPrintVec("anchor0", saved.localAnchor[0]);
		printf(" ");
		nxPrintVec("anchor1", saved.localAnchor[1]);
		printf("\n");
		printf("case=spherical index=%u saved ", index);
		nxPrintVec("axis0", saved.localAxis[0]);
		printf(" ");
		nxPrintVec("axis1", saved.localAxis[1]);
		printf("\n");
		printf("case=spherical index=%u saved ", index);
		nxPrintVec("normal0", saved.localNormal[0]);
		printf(" ");
		nxPrintVec("normal1", saved.localNormal[1]);
		printf("\n");
		printf("case=spherical index=%u saved ", index);
		nxPrintVec("swing_axis", saved.swingAxis);
		printf(" projection_distance=%08x flags=%08x projection_mode=%u\n",
			nxU(saved.projectionDistance), static_cast<unsigned>(saved.flags),
			static_cast<unsigned>(saved.projectionMode));
		printf("case=spherical index=%u saved twist_limit=%08x.%08x.%08x.%08x.%08x.%08x"
			" swing_limit=%08x.%08x.%08x\n", index,
			nxU(saved.twistLimit.low.value), nxU(saved.twistLimit.low.restitution),
			nxU(saved.twistLimit.low.hardness), nxU(saved.twistLimit.high.value),
			nxU(saved.twistLimit.high.restitution), nxU(saved.twistLimit.high.hardness),
			nxU(saved.swingLimit.value), nxU(saved.swingLimit.restitution),
			nxU(saved.swingLimit.hardness));
		printf("case=spherical index=%u saved twist_spring=%08x.%08x.%08x"
			" swing_spring=%08x.%08x.%08x joint_spring=%08x.%08x.%08x\n", index,
			nxU(saved.twistSpring.spring), nxU(saved.twistSpring.damper),
			nxU(saved.twistSpring.targetValue), nxU(saved.swingSpring.spring),
			nxU(saved.swingSpring.damper), nxU(saved.swingSpring.targetValue),
			nxU(saved.jointSpring.spring), nxU(saved.jointSpring.damper),
			nxU(saved.jointSpring.targetValue));
		printf("case=spherical index=%u saved max_force=%08x max_torque=%08x flags=%08x actors a=%s b=%s\n",
			index, nxU(saved.maxForce), nxU(saved.maxTorque), static_cast<unsigned>(saved.jointFlags),
			saved.actor[0] == a ? "match" : (saved.actor[0] ? "other" : "null"),
			saved.actor[1] == b ? "match" : (saved.actor[1] ? "other" : "null"));
		}

	nxPrintSceneJoints(scene, "spherical", index, "before_release", joint);
	scene.releaseJoint(*joint);
	printf("case=spherical index=%u released=yes\n", index);
	nxPrintSceneJoints(scene, "spherical", index, "after_release", joint);
	}

// One point-on-line case (joint-families Task 3d), modelled on nxPrismaticCase: build
// the descriptor over the same two actors, create, read every value the joint
// holds without a simulation step, then release. The family getter is
// saveToDesc (NxPointOnLineJoint adds no other method), which returns the base
// descriptor fields the joint stored. Nothing is asserted; everything is
// printed.
static void nxPointOnLineCase(NxScene& scene, NxActor* a, NxActor* b,
	unsigned index, const NxVec3& anchor, const NxVec3& axis)
	{
	printf("case=point_on_line index=%u ", index);
	nxPrintVec("in_anchor", anchor);
	printf(" ");
	nxPrintVec("in_axis", axis);
	printf("\n");

	NxPointOnLineJointDesc desc;
	desc.setToDefault();
	desc.actor[0] = a;
	desc.actor[1] = b;
	nxSetGlobalAnchor(desc, anchor);
	nxSetGlobalAxis(desc, axis);

	NxJoint* joint = scene.createJoint(desc);
	printf("case=point_on_line index=%u created=%s\n", index, joint ? "yes" : "no");
	if(!joint)
		return;

	NxVec3 gotAnchor(0.0f, 0.0f, 0.0f);
	NxVec3 gotAxis(0.0f, 0.0f, 0.0f);
	joint->getGlobalAnchor(gotAnchor);
	joint->getGlobalAxis(gotAxis);
	printf("case=point_on_line index=%u ", index);
	nxPrintVec("out_anchor", gotAnchor);
	printf(" ");
	nxPrintVec("out_axis", gotAxis);
	printf(" state=%u\n", static_cast<unsigned>(joint->getState()));
	nxPrintInternalWords("point_on_line", index, joint);

	NxActor* ra = 0;
	NxActor* rb = 0;
	joint->getActors(&ra, &rb);
	printf("case=point_on_line index=%u actors a=%s b=%s\n", index,
		ra == a ? "match" : (ra ? "other" : "null"),
		rb == b ? "match" : (rb ? "other" : "null"));

	NxPointOnLineJoint* pointOnLine = joint->isPointOnLineJoint();
	printf("case=point_on_line index=%u type=%u is_point_on_line=%s is_revolute=%s\n", index,
		static_cast<unsigned>(joint->getType()), pointOnLine ? "yes" : "no",
		joint->isRevoluteJoint() ? "yes" : "no");
	if(pointOnLine)
		{
		NxPointOnLineJointDesc saved;
		pointOnLine->saveToDesc(saved);
		printf("case=point_on_line index=%u saved ", index);
		nxPrintVec("anchor0", saved.localAnchor[0]);
		printf(" ");
		nxPrintVec("anchor1", saved.localAnchor[1]);
		printf("\n");
		printf("case=point_on_line index=%u saved ", index);
		nxPrintVec("axis0", saved.localAxis[0]);
		printf(" ");
		nxPrintVec("axis1", saved.localAxis[1]);
		printf("\n");
		printf("case=point_on_line index=%u saved ", index);
		nxPrintVec("normal0", saved.localNormal[0]);
		printf(" ");
		nxPrintVec("normal1", saved.localNormal[1]);
		printf("\n");
		printf("case=point_on_line index=%u saved max_force=%08x max_torque=%08x flags=%08x actors a=%s b=%s\n",
			index, nxU(saved.maxForce), nxU(saved.maxTorque), static_cast<unsigned>(saved.jointFlags),
			saved.actor[0] == a ? "match" : (saved.actor[0] ? "other" : "null"),
			saved.actor[1] == b ? "match" : (saved.actor[1] ? "other" : "null"));
		}

	nxPrintSceneJoints(scene, "point_on_line", index, "before_release", joint);
	scene.releaseJoint(*joint);
	printf("case=point_on_line index=%u released=yes\n", index);
	nxPrintSceneJoints(scene, "point_on_line", index, "after_release", joint);
	}

static void nxPointInPlaneCase(NxScene& scene, NxActor* a, NxActor* b,
	unsigned index, const NxVec3& anchor, const NxVec3& axis)
	{
	printf("case=point_in_plane index=%u ", index);
	nxPrintVec("in_anchor", anchor);
	printf(" ");
	nxPrintVec("in_axis", axis);
	printf("\n");

	NxPointInPlaneJointDesc desc;
	desc.setToDefault();
	desc.actor[0] = a;
	desc.actor[1] = b;
	nxSetGlobalAnchor(desc, anchor);
	nxSetGlobalAxis(desc, axis);

	NxJoint* joint = scene.createJoint(desc);
	printf("case=point_in_plane index=%u created=%s\n", index, joint ? "yes" : "no");
	if(!joint)
		return;

	NxVec3 gotAnchor(0.0f, 0.0f, 0.0f);
	NxVec3 gotAxis(0.0f, 0.0f, 0.0f);
	joint->getGlobalAnchor(gotAnchor);
	joint->getGlobalAxis(gotAxis);
	printf("case=point_in_plane index=%u ", index);
	nxPrintVec("out_anchor", gotAnchor);
	printf(" ");
	nxPrintVec("out_axis", gotAxis);
	printf(" state=%u\n", static_cast<unsigned>(joint->getState()));
	nxPrintInternalWords("point_in_plane", index, joint);

	NxActor* ra = 0;
	NxActor* rb = 0;
	joint->getActors(&ra, &rb);
	printf("case=point_in_plane index=%u actors a=%s b=%s\n", index,
		ra == a ? "match" : (ra ? "other" : "null"),
		rb == b ? "match" : (rb ? "other" : "null"));

	NxPointInPlaneJoint* pointInPlane = joint->isPointInPlaneJoint();
	printf("case=point_in_plane index=%u type=%u is_point_in_plane=%s is_revolute=%s\n", index,
		static_cast<unsigned>(joint->getType()), pointInPlane ? "yes" : "no",
		joint->isRevoluteJoint() ? "yes" : "no");
	if(pointInPlane)
		{
		NxPointInPlaneJointDesc saved;
		pointInPlane->saveToDesc(saved);
		printf("case=point_in_plane index=%u saved ", index);
		nxPrintVec("anchor0", saved.localAnchor[0]);
		printf(" ");
		nxPrintVec("anchor1", saved.localAnchor[1]);
		printf("\n");
		printf("case=point_in_plane index=%u saved ", index);
		nxPrintVec("axis0", saved.localAxis[0]);
		printf(" ");
		nxPrintVec("axis1", saved.localAxis[1]);
		printf("\n");
		printf("case=point_in_plane index=%u saved ", index);
		nxPrintVec("normal0", saved.localNormal[0]);
		printf(" ");
		nxPrintVec("normal1", saved.localNormal[1]);
		printf("\n");
		printf("case=point_in_plane index=%u saved max_force=%08x max_torque=%08x flags=%08x actors a=%s b=%s\n",
			index, nxU(saved.maxForce), nxU(saved.maxTorque), static_cast<unsigned>(saved.jointFlags),
			saved.actor[0] == a ? "match" : (saved.actor[0] ? "other" : "null"),
			saved.actor[1] == b ? "match" : (saved.actor[1] ? "other" : "null"));
		}

	nxPrintSceneJoints(scene, "point_in_plane", index, "before_release", joint);
	scene.releaseJoint(*joint);
	printf("case=point_in_plane index=%u released=yes\n", index);
	nxPrintSceneJoints(scene, "point_in_plane", index, "after_release", joint);
	}

static void nxDistanceCase(NxScene& scene, NxActor* a, NxActor* b,
	unsigned index, const NxVec3& anchor, const NxVec3& axis,
	NxReal maxDistance, NxReal minDistance, const NxSpringDesc& spring, NxU32 flags)
	{
	printf("case=distance index=%u ", index);
	nxPrintVec("in_anchor", anchor);
	printf(" ");
	nxPrintVec("in_axis", axis);
	printf(" max_distance=%08x min_distance=%08x spring=%08x.%08x.%08x flags=%08x\n",
		nxU(maxDistance), nxU(minDistance), nxU(spring.spring), nxU(spring.damper), nxU(spring.targetValue),
		static_cast<unsigned>(flags));

	NxDistanceJointDesc desc;
	desc.setToDefault(false);
	desc.actor[0] = a;
	desc.actor[1] = b;
	nxSetGlobalAnchor(desc, anchor);
	nxSetGlobalAxis(desc, axis);
	desc.maxDistance = maxDistance;
	desc.minDistance = minDistance;
	desc.spring = spring;
	desc.flags = flags;

	NxJoint* joint = scene.createJoint(desc);
	printf("case=distance index=%u created=%s\n", index, joint ? "yes" : "no");
	if(!joint)
		return;

	NxVec3 gotAnchor(0.0f, 0.0f, 0.0f);
	NxVec3 gotAxis(0.0f, 0.0f, 0.0f);
	joint->getGlobalAnchor(gotAnchor);
	joint->getGlobalAxis(gotAxis);
	printf("case=distance index=%u ", index);
	nxPrintVec("out_anchor", gotAnchor);
	printf(" ");
	nxPrintVec("out_axis", gotAxis);
	printf(" state=%u\n", static_cast<unsigned>(joint->getState()));
	nxPrintInternalWords("distance", index, joint);

	NxActor* ra = 0;
	NxActor* rb = 0;
	joint->getActors(&ra, &rb);
	printf("case=distance index=%u actors a=%s b=%s\n", index,
		ra == a ? "match" : (ra ? "other" : "null"),
		rb == b ? "match" : (rb ? "other" : "null"));

	NxDistanceJoint* distance = joint->isDistanceJoint();
	printf("case=distance index=%u type=%u is_distance=%s is_revolute=%s\n", index,
		static_cast<unsigned>(joint->getType()), distance ? "yes" : "no",
		joint->isRevoluteJoint() ? "yes" : "no");
	if(distance)
		{
		NxDistanceJointDesc saved;
		distance->saveToDesc(saved);
		printf("case=distance index=%u saved max_distance=%08x min_distance=%08x spring=%08x.%08x.%08x flags=%08x\n",
			index, nxU(saved.maxDistance), nxU(saved.minDistance), nxU(saved.spring.spring),
			nxU(saved.spring.damper), nxU(saved.spring.targetValue), static_cast<unsigned>(saved.flags));
		printf("case=distance index=%u saved ", index);
		nxPrintVec("anchor0", saved.localAnchor[0]);
		printf(" ");
		nxPrintVec("anchor1", saved.localAnchor[1]);
		printf("\n");
		printf("case=distance index=%u saved ", index);
		nxPrintVec("axis0", saved.localAxis[0]);
		printf(" ");
		nxPrintVec("axis1", saved.localAxis[1]);
		printf("\n");
		printf("case=distance index=%u saved ", index);
		nxPrintVec("normal0", saved.localNormal[0]);
		printf(" ");
		nxPrintVec("normal1", saved.localNormal[1]);
		printf("\n");
		printf("case=distance index=%u saved max_force=%08x max_torque=%08x flags=%08x actors a=%s b=%s\n",
			index, nxU(saved.maxForce), nxU(saved.maxTorque), static_cast<unsigned>(saved.jointFlags),
			saved.actor[0] == a ? "match" : (saved.actor[0] ? "other" : "null"),
			saved.actor[1] == b ? "match" : (saved.actor[1] ? "other" : "null"));
		}

	nxPrintSceneJoints(scene, "distance", index, "before_release", joint);
	scene.releaseJoint(*joint);
	printf("case=distance index=%u released=yes\n", index);
	nxPrintSceneJoints(scene, "distance", index, "after_release", joint);
	}

static void nxPulleyCase(NxScene& scene, NxActor* a, NxActor* b,
	unsigned index, const NxVec3& anchor, const NxVec3& axis,
	const NxVec3& pulley0, const NxVec3& pulley1, NxReal distance, NxReal stiffness, NxReal ratio, NxU32 flags)
	{
	printf("case=pulley index=%u ", index);
	nxPrintVec("in_anchor", anchor);
	printf(" ");
	nxPrintVec("in_axis", axis);
	printf("\n");
	printf("case=pulley index=%u ", index);
	nxPrintVec("in_pulley0", pulley0);
	printf(" ");
	nxPrintVec("in_pulley1", pulley1);
	printf(" distance=%08x stiffness=%08x ratio=%08x flags=%08x\n",
		nxU(distance), nxU(stiffness), nxU(ratio), static_cast<unsigned>(flags));

	NxPulleyJointDesc desc;
	desc.setToDefault();
	desc.actor[0] = a;
	desc.actor[1] = b;
	nxSetGlobalAnchor(desc, anchor);
	nxSetGlobalAxis(desc, axis);
	desc.pulley[0] = pulley0;
	desc.pulley[1] = pulley1;
	desc.distance = distance;
	desc.stiffness = stiffness;
	desc.ratio = ratio;
	desc.flags = flags;

	NxJoint* joint = scene.createJoint(desc);
	printf("case=pulley index=%u created=%s\n", index, joint ? "yes" : "no");
	if(!joint)
		return;

	NxVec3 gotAnchor(0.0f, 0.0f, 0.0f);
	NxVec3 gotAxis(0.0f, 0.0f, 0.0f);
	joint->getGlobalAnchor(gotAnchor);
	joint->getGlobalAxis(gotAxis);
	printf("case=pulley index=%u ", index);
	nxPrintVec("out_anchor", gotAnchor);
	printf(" ");
	nxPrintVec("out_axis", gotAxis);
	printf(" state=%u\n", static_cast<unsigned>(joint->getState()));
	nxPrintInternalWords("pulley", index, joint);

	NxActor* ra = 0;
	NxActor* rb = 0;
	joint->getActors(&ra, &rb);
	printf("case=pulley index=%u actors a=%s b=%s\n", index,
		ra == a ? "match" : (ra ? "other" : "null"),
		rb == b ? "match" : (rb ? "other" : "null"));

	NxPulleyJoint* pulley = joint->isPulleyJoint();
	printf("case=pulley index=%u type=%u is_pulley=%s is_distance=%s\n", index,
		static_cast<unsigned>(joint->getType()), pulley ? "yes" : "no",
		joint->isDistanceJoint() ? "yes" : "no");
	if(pulley)
		{
		NxPulleyJointDesc saved;
		pulley->saveToDesc(saved);
		printf("case=pulley index=%u saved ", index);
		nxPrintVec("pulley0", saved.pulley[0]);
		printf(" ");
		nxPrintVec("pulley1", saved.pulley[1]);
		printf("\n");
		printf("case=pulley index=%u saved distance=%08x stiffness=%08x ratio=%08x flags=%08x\n",
			index, nxU(saved.distance), nxU(saved.stiffness), nxU(saved.ratio), static_cast<unsigned>(saved.flags));
		printf("case=pulley index=%u saved ", index);
		nxPrintVec("anchor0", saved.localAnchor[0]);
		printf(" ");
		nxPrintVec("anchor1", saved.localAnchor[1]);
		printf("\n");
		printf("case=pulley index=%u saved ", index);
		nxPrintVec("axis0", saved.localAxis[0]);
		printf(" ");
		nxPrintVec("axis1", saved.localAxis[1]);
		printf("\n");
		printf("case=pulley index=%u saved ", index);
		nxPrintVec("normal0", saved.localNormal[0]);
		printf(" ");
		nxPrintVec("normal1", saved.localNormal[1]);
		printf("\n");
		printf("case=pulley index=%u saved max_force=%08x max_torque=%08x flags=%08x actors a=%s b=%s\n",
			index, nxU(saved.maxForce), nxU(saved.maxTorque), static_cast<unsigned>(saved.jointFlags),
			saved.actor[0] == a ? "match" : (saved.actor[0] ? "other" : "null"),
			saved.actor[1] == b ? "match" : (saved.actor[1] ? "other" : "null"));
		}

	nxPrintSceneJoints(scene, "pulley", index, "before_release", joint);
	scene.releaseJoint(*joint);
	printf("case=pulley index=%u released=yes\n", index);
	nxPrintSceneJoints(scene, "pulley", index, "after_release", joint);
	}

// The fixed family (joint-families Task 3h). NxFixedJointDesc has no field of
// its own, so saveToDesc brings back only the NxJointDesc base part.
static void nxFixedCase(NxScene& scene, NxActor* a, NxActor* b,
	unsigned index, const NxVec3& anchor, const NxVec3& axis)
	{
	printf("case=fixed index=%u ", index);
	nxPrintVec("in_anchor", anchor);
	printf(" ");
	nxPrintVec("in_axis", axis);
	printf("\n");

	NxFixedJointDesc desc;
	desc.setToDefault();
	desc.actor[0] = a;
	desc.actor[1] = b;
	nxSetGlobalAnchor(desc, anchor);
	nxSetGlobalAxis(desc, axis);

	NxJoint* joint = scene.createJoint(desc);
	printf("case=fixed index=%u created=%s\n", index, joint ? "yes" : "no");
	if(!joint)
		return;

	NxVec3 gotAnchor(0.0f, 0.0f, 0.0f);
	NxVec3 gotAxis(0.0f, 0.0f, 0.0f);
	joint->getGlobalAnchor(gotAnchor);
	joint->getGlobalAxis(gotAxis);
	printf("case=fixed index=%u ", index);
	nxPrintVec("out_anchor", gotAnchor);
	printf(" ");
	nxPrintVec("out_axis", gotAxis);
	printf(" state=%u\n", static_cast<unsigned>(joint->getState()));
	nxPrintInternalWords("fixed", index, joint);

	NxActor* ra = 0;
	NxActor* rb = 0;
	joint->getActors(&ra, &rb);
	printf("case=fixed index=%u actors a=%s b=%s\n", index,
		ra == a ? "match" : (ra ? "other" : "null"),
		rb == b ? "match" : (rb ? "other" : "null"));

	NxFixedJoint* fixed = joint->isFixedJoint();
	printf("case=fixed index=%u type=%u is_fixed=%s is_pulley=%s\n", index,
		static_cast<unsigned>(joint->getType()), fixed ? "yes" : "no",
		joint->isPulleyJoint() ? "yes" : "no");
	if(fixed)
		{
		NxFixedJointDesc saved;
		fixed->saveToDesc(saved);
		printf("case=fixed index=%u saved ", index);
		nxPrintVec("anchor0", saved.localAnchor[0]);
		printf(" ");
		nxPrintVec("anchor1", saved.localAnchor[1]);
		printf("\n");
		printf("case=fixed index=%u saved ", index);
		nxPrintVec("axis0", saved.localAxis[0]);
		printf(" ");
		nxPrintVec("axis1", saved.localAxis[1]);
		printf("\n");
		printf("case=fixed index=%u saved ", index);
		nxPrintVec("normal0", saved.localNormal[0]);
		printf(" ");
		nxPrintVec("normal1", saved.localNormal[1]);
		printf("\n");
		printf("case=fixed index=%u saved max_force=%08x max_torque=%08x flags=%08x actors a=%s b=%s\n",
			index, nxU(saved.maxForce), nxU(saved.maxTorque), static_cast<unsigned>(saved.jointFlags),
			saved.actor[0] == a ? "match" : (saved.actor[0] ? "other" : "null"),
			saved.actor[1] == b ? "match" : (saved.actor[1] ? "other" : "null"));
		}

	nxPrintSceneJoints(scene, "fixed", index, "before_release", joint);
	scene.releaseJoint(*joint);
	printf("case=fixed index=%u released=yes\n", index);
	nxPrintSceneJoints(scene, "fixed", index, "after_release", joint);
	}

// The D6 family's descriptor fields (joint-families Task 3i), one set per case.
struct NxD6CaseFields
	{
	NxD6JointMotion		motion[6];		// x, y, z, twist, swing1, swing2
	NxReal				linear[3];		// linearLimit value, restitution, hardness
	NxReal				twistLow;
	NxReal				twistHigh;
	NxReal				swing1;
	NxReal				swing2;
	NxReal				drive;			// base value; drive i gets drive + i
	bool				useSpherical;
	NxVec3				drivePosition;
	NxReal				driveOrientation[4];	// x, y, z, w
	NxVec3				driveLinearVelocity;
	NxVec3				driveAngularVelocity;
	NxReal				projectionDistance;
	NxReal				projectionAngle;
	NxJointProjectionMode	projectionMode;
	};

// Writes one field set into a D6 descriptor's family part. Every family field
// is written: the NxD6JointDesc constructor leaves projectionDistance,
// projectionAngle, projectionMode and useSpherical uninitialised.
static void nxD6Fill(NxD6JointDesc& desc, const NxD6CaseFields& f)
	{
	desc.xMotion = f.motion[0];
	desc.yMotion = f.motion[1];
	desc.zMotion = f.motion[2];
	desc.twistMotion = f.motion[3];
	desc.swing1Motion = f.motion[4];
	desc.swing2Motion = f.motion[5];
	desc.linearLimit.value = f.linear[0];
	desc.linearLimit.restitution = f.linear[1];
	desc.linearLimit.hardness = f.linear[2];
	desc.twistLimit.low.value = f.twistLow;
	desc.twistLimit.low.restitution = 0.125f;
	desc.twistLimit.low.hardness = 0.875f;
	desc.twistLimit.high.value = f.twistHigh;
	desc.twistLimit.high.restitution = 0.25f;
	desc.twistLimit.high.hardness = 0.625f;
	desc.swing1Limit.value = f.swing1;
	desc.swing1Limit.restitution = 0.375f;
	desc.swing1Limit.hardness = 0.5f;
	desc.swing2Limit.value = f.swing2;
	desc.swing2Limit.restitution = 0.0625f;
	desc.swing2Limit.hardness = 0.9375f;
	NxJointDriveDesc* drives[6] = { &desc.xDrive, &desc.yDrive, &desc.zDrive,
		&desc.swingDrive, &desc.twistDrive, &desc.sphericalDrive };
	for(unsigned i = 0; i < 6; i++)
		{
		drives[i]->driveType = (i & 1) ? NX_D6JOINT_DRIVE_VELOCITY : NX_D6JOINT_DRIVE_POSITION;
		drives[i]->spring = f.drive + static_cast<NxReal>(i);
		drives[i]->damping = 0.5f * (f.drive + static_cast<NxReal>(i));
		drives[i]->forceLimit = 100.0f + static_cast<NxReal>(i);
		}
	desc.useSpherical = f.useSpherical;
	desc.drivePosition = f.drivePosition;
	desc.driveOrientation.x = f.driveOrientation[0];
	desc.driveOrientation.y = f.driveOrientation[1];
	desc.driveOrientation.z = f.driveOrientation[2];
	desc.driveOrientation.w = f.driveOrientation[3];
	desc.driveLinearVelocity = f.driveLinearVelocity;
	desc.driveAngularVelocity = f.driveAngularVelocity;
	desc.projectionDistance = f.projectionDistance;
	desc.projectionAngle = f.projectionAngle;
	desc.projectionMode = f.projectionMode;
	}

// Prints every family field of a D6 descriptor as raw words.
static void nxD6PrintFields(unsigned index, const char* tag, const NxD6JointDesc& d)
	{
	printf("case=d6 index=%u %s motions=%u.%u.%u.%u.%u.%u\n", index, tag,
		static_cast<unsigned>(d.xMotion), static_cast<unsigned>(d.yMotion), static_cast<unsigned>(d.zMotion),
		static_cast<unsigned>(d.twistMotion), static_cast<unsigned>(d.swing1Motion),
		static_cast<unsigned>(d.swing2Motion));
	printf("case=d6 index=%u %s linear=%08x.%08x.%08x twist_low=%08x.%08x.%08x twist_high=%08x.%08x.%08x\n",
		index, tag, nxU(d.linearLimit.value), nxU(d.linearLimit.restitution), nxU(d.linearLimit.hardness),
		nxU(d.twistLimit.low.value), nxU(d.twistLimit.low.restitution), nxU(d.twistLimit.low.hardness),
		nxU(d.twistLimit.high.value), nxU(d.twistLimit.high.restitution), nxU(d.twistLimit.high.hardness));
	printf("case=d6 index=%u %s swing1=%08x.%08x.%08x swing2=%08x.%08x.%08x\n", index, tag,
		nxU(d.swing1Limit.value), nxU(d.swing1Limit.restitution), nxU(d.swing1Limit.hardness),
		nxU(d.swing2Limit.value), nxU(d.swing2Limit.restitution), nxU(d.swing2Limit.hardness));
	const NxJointDriveDesc* drives[6] = { &d.xDrive, &d.yDrive, &d.zDrive,
		&d.swingDrive, &d.twistDrive, &d.sphericalDrive };
	static const char* const names[6] = { "x", "y", "z", "swing", "twist", "spherical" };
	for(unsigned i = 0; i < 6; i++)
		{
		NxJointDriveDesc drive = *drives[i];
		printf("case=d6 index=%u %s drive_%s=%08x.%08x.%08x.%08x\n", index, tag, names[i],
			static_cast<unsigned>(static_cast<NxU32>(drive.driveType)), nxU(drive.spring), nxU(drive.damping),
			nxU(drive.forceLimit));
		}
	printf("case=d6 index=%u %s use_spherical=%u ", index, tag, d.useSpherical ? 1u : 0u);
	nxPrintVec("drive_position", d.drivePosition);
	printf(" drive_orientation=%08x.%08x.%08x.%08x\n", nxU(d.driveOrientation.x), nxU(d.driveOrientation.y),
		nxU(d.driveOrientation.z), nxU(d.driveOrientation.w));
	printf("case=d6 index=%u %s ", index, tag);
	nxPrintVec("drive_linear_velocity", d.driveLinearVelocity);
	printf(" ");
	nxPrintVec("drive_angular_velocity", d.driveAngularVelocity);
	printf("\n");
	printf("case=d6 index=%u %s projection distance=%08x angle=%08x mode=%u\n", index, tag,
		nxU(d.projectionDistance), nxU(d.projectionAngle), static_cast<unsigned>(d.projectionMode));
	}

// The D6 family (joint-families Task 3i). The oracle's D6Joint::saveToDesc
// (phys_fn_004182) saves only the NxJointDesc base part, so every family field
// of the saved descriptor keeps what the case wrote into it before the call
// (the `sentinel` set); the transcript prints both.
static void nxD6Case(NxScene& scene, NxActor* a, NxActor* b,
	unsigned index, const NxVec3& anchor, const NxVec3& axis,
	const NxD6CaseFields& fields, const NxD6CaseFields& sentinel)
	{
	printf("case=d6 index=%u ", index);
	nxPrintVec("in_anchor", anchor);
	printf(" ");
	nxPrintVec("in_axis", axis);
	printf("\n");

	NxD6JointDesc desc;
	desc.actor[0] = a;
	desc.actor[1] = b;
	nxSetGlobalAnchor(desc, anchor);
	nxSetGlobalAxis(desc, axis);
	nxD6Fill(desc, fields);
	nxD6PrintFields(index, "in", desc);

	NxJoint* joint = scene.createJoint(desc);
	printf("case=d6 index=%u created=%s\n", index, joint ? "yes" : "no");
	if(!joint)
		return;

	NxVec3 gotAnchor(0.0f, 0.0f, 0.0f);
	NxVec3 gotAxis(0.0f, 0.0f, 0.0f);
	joint->getGlobalAnchor(gotAnchor);
	joint->getGlobalAxis(gotAxis);
	printf("case=d6 index=%u ", index);
	nxPrintVec("out_anchor", gotAnchor);
	printf(" ");
	nxPrintVec("out_axis", gotAxis);
	printf(" state=%u\n", static_cast<unsigned>(joint->getState()));
	nxPrintInternalWords("d6", index, joint);

	NxActor* ra = 0;
	NxActor* rb = 0;
	joint->getActors(&ra, &rb);
	printf("case=d6 index=%u actors a=%s b=%s\n", index,
		ra == a ? "match" : (ra ? "other" : "null"),
		rb == b ? "match" : (rb ? "other" : "null"));

	// This SDK has no isD6Joint(): the generic is() (folded phys_fn_004479)
	// returns the joint itself when the type matches.
	NxD6Joint* d6 = static_cast<NxD6Joint*>(joint->is(NX_JOINT_D6));
	printf("case=d6 index=%u type=%u is_d6=%s is_fixed=%s\n", index,
		static_cast<unsigned>(joint->getType()), d6 ? "yes" : "no",
		joint->isFixedJoint() ? "yes" : "no");
	if(d6)
		{
		// Exercise the D6 public wrapper's setGlobalAxis row with a value that
		// differs from the descriptor input, then read back the resulting frame.
		if(index == 3)
			{
			const NxVec3 replacementAnchor(2.25f, -1.5f, 0.75f);
			d6->setGlobalAnchor(replacementAnchor);
			NxVec3 replacementAnchorReadback(0.0f, 0.0f, 0.0f);
			d6->getGlobalAnchor(replacementAnchorReadback);
			printf("case=d6 index=%u set_anchor ", index);
			nxPrintVec("out_anchor", replacementAnchorReadback);
			printf("\n");

			const NxVec3 replacementAxis(0.75f, -2.0f, 1.25f);
			d6->setGlobalAxis(replacementAxis);
			NxVec3 replacementAxisReadback(0.0f, 0.0f, 0.0f);
			d6->getGlobalAxis(replacementAxisReadback);
			printf("case=d6 index=%u set_axis ", index);
			nxPrintVec("out_axis", replacementAxisReadback);
			printf("\n");

			d6->setBreakable(17.25f, 32.5f);
			NxReal breakForce = 0.0f;
			NxReal breakTorque = 0.0f;
			d6->getBreakable(breakForce, breakTorque);
			printf("case=d6 index=%u breakable force=%08x torque=%08x\n", index,
				nxU(breakForce), nxU(breakTorque));

			const NxVec3 replacementLimitPoint(-0.75f, 1.5f, 2.25f);
			d6->setLimitPoint(replacementLimitPoint, false);
			NxVec3 limitPointReadback(0.0f, 0.0f, 0.0f);
			const bool hasLimitPoint = d6->getLimitPoint(limitPointReadback);
			printf("case=d6 index=%u limit_point present=%u ", index, hasLimitPoint ? 1u : 0u);
			nxPrintVec("point", limitPointReadback);
			printf("\n");

			const NxVec3 planeNormal(0.6f, -0.8f, 0.25f);
			const NxVec3 planePoint(1.25f, -0.5f, 2.0f);
			const bool planeAdded = d6->addLimitPlane(planeNormal, planePoint);
			d6->resetLimitPlaneIterator();
			NxVec3 planeNormalReadback(0.0f, 0.0f, 0.0f);
			NxReal planeD = 0.0f;
			const bool planeRead = d6->getNextLimitPlane(planeNormalReadback, planeD);
			printf("case=d6 index=%u limit_plane added=%u read=%u ", index,
				planeAdded ? 1u : 0u, planeRead ? 1u : 0u);
			nxPrintVec("normal", planeNormalReadback);
			printf(" d=%08x\n", nxU(planeD));

			d6->setName("d6:phase6-setname");
			printf("case=d6 index=%u name=%s\n", index, d6->getName() ? d6->getName() : "null");
			}

		NxD6JointDesc saved;
		nxD6Fill(saved, sentinel);
		d6->saveToDesc(saved);
		nxD6PrintFields(index, "saved", saved);
		printf("case=d6 index=%u saved ", index);
		nxPrintVec("anchor0", saved.localAnchor[0]);
		printf(" ");
		nxPrintVec("anchor1", saved.localAnchor[1]);
		printf("\n");
		printf("case=d6 index=%u saved ", index);
		nxPrintVec("axis0", saved.localAxis[0]);
		printf(" ");
		nxPrintVec("axis1", saved.localAxis[1]);
		printf("\n");
		printf("case=d6 index=%u saved ", index);
		nxPrintVec("normal0", saved.localNormal[0]);
		printf(" ");
		nxPrintVec("normal1", saved.localNormal[1]);
		printf("\n");
		printf("case=d6 index=%u saved max_force=%08x max_torque=%08x flags=%08x actors a=%s b=%s\n",
			index, nxU(saved.maxForce), nxU(saved.maxTorque), static_cast<unsigned>(saved.jointFlags),
			saved.actor[0] == a ? "match" : (saved.actor[0] ? "other" : "null"),
			saved.actor[1] == b ? "match" : (saved.actor[1] ? "other" : "null"));

		// The four drive setters take the write lock and call the folded empty
		// internal body (phys_fn_004461-004467); nothing is stored, so a second
		// save shows the same family fields.
		NxQuat orientation;
		orientation.x = 0.0f;
		orientation.y = 0.0f;
		orientation.z = 0.6f;
		orientation.w = 0.8f;
		// The folded internal setter is empty; verify the wrapper's observable
		// rejected-write path with the lock marked as owned by another thread.
		unsigned* lockState = nxSceneWriteLockState(scene);
		const unsigned savedLockState[2] = { lockState[0], lockState[1] };
		lockState[0] = 1;
		lockState[1] = 0;
		jointErrorStream.reset();
		jointErrorStream.enabled = true;
		d6->setDrivePosition(NxVec3(9.0f, 8.0f, 7.0f));
		printf("case=d6 index=%u drive_contended reports=%u code=%u line=%d\n", index,
			jointErrorStream.reports, static_cast<unsigned>(jointErrorStream.lastCode),
			jointErrorStream.lastLine);
		jointErrorStream.reset();
		d6->setDriveOrientation(orientation);
		printf("case=d6 index=%u drive_orientation_contended reports=%u code=%u line=%d\n", index,
			jointErrorStream.reports, static_cast<unsigned>(jointErrorStream.lastCode),
			jointErrorStream.lastLine);
		jointErrorStream.reset();
		d6->setDriveLinearVelocity(NxVec3(-1.0f, -2.0f, -3.0f));
		printf("case=d6 index=%u drive_linear_velocity_contended reports=%u code=%u line=%d\n", index,
			jointErrorStream.reports, static_cast<unsigned>(jointErrorStream.lastCode),
			jointErrorStream.lastLine);
		jointErrorStream.reset();
		d6->setDriveAngularVelocity(NxVec3(0.25f, 0.5f, 0.75f));
		printf("case=d6 index=%u drive_angular_velocity_contended reports=%u code=%u line=%d\n", index,
			jointErrorStream.reports, static_cast<unsigned>(jointErrorStream.lastCode),
			jointErrorStream.lastLine);
		jointErrorStream.enabled = false;
		lockState[0] = savedLockState[0];
		lockState[1] = savedLockState[1];
		d6->setDrivePosition(NxVec3(7.0f, 8.0f, 9.0f));
		d6->setDriveOrientation(orientation);
		d6->setDriveLinearVelocity(NxVec3(-1.0f, -2.0f, -3.0f));
		d6->setDriveAngularVelocity(NxVec3(0.25f, 0.5f, 0.75f));
		printf("case=d6 index=%u drive_setters=called\n", index);
		NxD6JointDesc again;
		nxD6Fill(again, sentinel);
		d6->saveToDesc(again);
		nxD6PrintFields(index, "resaved", again);
		}

	nxPrintSceneJoints(scene, "d6", index, "before_release", joint);
	scene.releaseJoint(*joint);
	printf("case=d6 index=%u released=yes\n", index);
	nxPrintSceneJoints(scene, "d6", index, "after_release", joint);
	}

// The D6 field sets. Index 0 mixes locked, limited and free motions (twist and
// swing1 limited, so the constructor forms their half-angle cosines); index 3
// limits every motion. The sentinel set is what the saved descriptors hold
// before saveToDesc. The Task 4 cases reuse the index-0 set.
static const NxD6CaseFields nxD6First = {
	{ NX_D6JOINT_MOTION_LOCKED, NX_D6JOINT_MOTION_LIMITED, NX_D6JOINT_MOTION_FREE,
	  NX_D6JOINT_MOTION_LIMITED, NX_D6JOINT_MOTION_LIMITED, NX_D6JOINT_MOTION_FREE },
	{ 0.5f, 0.25f, 0.75f }, -0.5f, 0.75f, 0.625f, 0.375f, 2.0f, true,
	NxVec3(1.0f, 2.0f, 3.0f), { 0.0f, 0.6f, 0.0f, 0.8f },
	NxVec3(0.5f, -0.5f, 1.5f), NxVec3(-0.25f, 0.125f, 2.5f),
	0.125f, 0.0625f, NX_JPM_POINT_MINDIST };
static const NxD6CaseFields nxD6Second = {
	{ NX_D6JOINT_MOTION_LIMITED, NX_D6JOINT_MOTION_LIMITED, NX_D6JOINT_MOTION_LIMITED,
	  NX_D6JOINT_MOTION_LIMITED, NX_D6JOINT_MOTION_LIMITED, NX_D6JOINT_MOTION_LIMITED },
	{ 1.25f, 0.0f, 1.0f }, -1.0f, 1.5f, 0.25f, 1.0f, 4.0f, false,
	NxVec3(-2.0f, 0.5f, 4.0f), { 0.0f, 0.0f, 0.0f, 1.0f },
	NxVec3(0.0f, 0.0f, 0.0f), NxVec3(3.0f, 0.0f, -3.0f),
	2.0f, 0.5f, NX_JPM_NONE };
static const NxD6CaseFields nxD6Sentinel = {
	{ NX_D6JOINT_MOTION_FREE, NX_D6JOINT_MOTION_LOCKED, NX_D6JOINT_MOTION_FREE,
	  NX_D6JOINT_MOTION_LOCKED, NX_D6JOINT_MOTION_FREE, NX_D6JOINT_MOTION_LOCKED },
	{ 9.0f, 0.5f, 0.5f }, -9.0f, 9.5f, 8.0f, 7.0f, 20.0f, true,
	NxVec3(11.0f, 12.0f, 13.0f), { 0.5f, 0.5f, 0.5f, 0.5f },
	NxVec3(21.0f, 22.0f, 23.0f), NxVec3(31.0f, 32.0f, 33.0f),
	99.0f, 98.0f, NX_JPM_POINT_MINDIST };

// The release-then-create cycle (joint-open-items Task 2). Three joints are
// created over the fixture's actors (revolute, spherical, fixed), the middle
// one is released, a D6 is created, the D6 (the list head) and the revolute
// (the tail) are released, and a prismatic is created; after each step the
// scene's joint count and one enumeration pass are printed. Two joints are
// left registered on purpose, so the scene release that follows runs the
// Scene's own destruction of registered joints.
static NxJoint* nxCycleCreate(NxScene& scene, NxJointDesc& desc, NxActor* a, NxActor* b,
	const char* name)
	{
	desc.actor[0] = a;
	desc.actor[1] = b;
	nxSetGlobalAnchor(desc, NxVec3(1.0f, 2.0f, 3.0f));
	nxSetGlobalAxis(desc, NxVec3(0.0f, 1.0f, 0.0f));
	NxJoint* joint = scene.createJoint(desc);
	printf("case=cycle create=%s created=%s\n", name, joint ? "yes" : "no");
	return joint;
	}

static void nxReleaseCycleCase(NxScene& scene, NxActor* a, NxActor* b)
	{
	nxPrintSceneJoints(scene, "cycle", 0, "start", 0);

	NxRevoluteJointDesc revoluteDesc;
	NxJoint* revolute = nxCycleCreate(scene, revoluteDesc, a, b, "revolute");
	NxSphericalJointDesc sphericalDesc;
	NxJoint* spherical = nxCycleCreate(scene, sphericalDesc, a, b, "spherical");
	NxFixedJointDesc fixedDesc;
	NxJoint* fixed = nxCycleCreate(scene, fixedDesc, a, b, "fixed");
	if(!revolute || !spherical || !fixed)
		return;
	nxPrintSceneJoints(scene, "cycle", 1, "three_created", fixed);

	scene.releaseJoint(*spherical);
	printf("case=cycle release=spherical released=yes\n");
	nxPrintSceneJoints(scene, "cycle", 2, "middle_released", spherical);

	NxD6JointDesc d6Desc;
	NxJoint* d6 = nxCycleCreate(scene, d6Desc, a, b, "d6");
	if(!d6)
		return;
	nxPrintSceneJoints(scene, "cycle", 3, "d6_created", d6);

	scene.releaseJoint(*d6);
	printf("case=cycle release=d6 released=yes\n");
	nxPrintSceneJoints(scene, "cycle", 4, "head_released", d6);

	scene.releaseJoint(*revolute);
	printf("case=cycle release=revolute released=yes\n");
	nxPrintSceneJoints(scene, "cycle", 5, "tail_released", revolute);

	NxPrismaticJointDesc prismaticDesc;
	NxJoint* prismatic = nxCycleCreate(scene, prismaticDesc, a, b, "prismatic");
	if(!prismatic)
		return;
	nxPrintSceneJoints(scene, "cycle", 6, "prismatic_created", prismatic);
	printf("case=cycle left_for_scene_release=2\n");
	}

// Joint-open-items Task 4: every family over one anchor/axis pair, with each
// family's index-0 field set. Used for the near-z axes over the identity
// fixture and for every case over the rotated fixture.
static void nxAllFamiliesCase(NxScene& scene, NxActor* a, NxActor* b,
	unsigned index, const NxVec3& anchor, const NxVec3& axis)
	{
	nxRevoluteCase(scene, a, b, index, anchor, axis);
	nxPrismaticCase(scene, a, b, index, anchor, axis);
	nxCylindricalCase(scene, a, b, index, anchor, axis);
	nxSphericalCase(scene, a, b, index, anchor, axis);
	nxPointOnLineCase(scene, a, b, index, anchor, axis);
	nxPointInPlaneCase(scene, a, b, index, anchor, axis);
	nxDistanceCase(scene, a, b, index, anchor, axis,
		2.5f, 0.5f, NxSpringDesc(10.0f, 0.5f, 0.25f),
		NX_DJF_MAX_DISTANCE_ENABLED | NX_DJF_MIN_DISTANCE_ENABLED | NX_DJF_SPRING_ENABLED);
	nxPulleyCase(scene, a, b, index, anchor, axis,
		NxVec3(0.0f, 5.0f, 0.0f), NxVec3(4.0f, 5.0f, 0.0f), 6.0f, 0.75f, 1.5f, NX_PJF_IS_RIGID);
	nxFixedCase(scene, a, b, index, anchor, axis);
	nxD6Case(scene, a, b, index, anchor, axis, nxD6First, nxD6Sentinel);
	}

// A unit vector formed in the harness. The harness binary is the same for both
// sides of the pair, so both DLLs receive the same input words; the input is
// printed with every case anyway.
static NxVec3 nxUnit(NxReal x, NxReal y, NxReal z)
	{
	const NxReal inv = 1.0f / sqrtf(x * x + y * y + z * z);
	return NxVec3(x * inv, y * inv, z * inv);
	}

// A rotation matrix from a unit quaternion (x, y, z, w), formed in the harness
// with the textbook formula; its rows are printed as the fixture's input.
static NxMat33 nxQuatMatrix(NxReal x, NxReal y, NxReal z, NxReal w)
	{
	const NxVec3 row0(1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - w * z), 2.0f * (x * z + w * y));
	const NxVec3 row1(2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z - w * x));
	const NxVec3 row2(2.0f * (x * z - w * y), 2.0f * (y * z + w * x), 1.0f - 2.0f * (x * x + y * y));
	return NxMat33(row0, row1, row2);
	}

static void nxPrintRows(const NxMat33& m)
	{
	NxVec3 r0, r1, r2;
	m.getRow(0, r0);
	m.getRow(1, r1);
	m.getRow(2, r2);
	nxPrintVec("row0", r0);
	printf(" ");
	nxPrintVec("row1", r1);
	printf(" ");
	nxPrintVec("row2", r2);
	}

// What an actor reports about its pose: the global position, the orientation
// quaternion and the orientation rows, as raw words. These are the body
// record's pose words that the joint rows read (+0x5c quaternion, +0xdc 3x3).
static void nxPrintActorPose(const char* tag, const NxActor& actor)
	{
	const NxMat34 pose = actor.getGlobalPoseVal();
	const NxQuat q = actor.getGlobalOrientationQuatVal();
	printf("rotated_fixture actor=%s ", tag);
	nxPrintVec("t", pose.t);
	printf(" quat=%08x.%08x.%08x.%08x\n", nxU(q.x), nxU(q.y), nxU(q.z), nxU(q.w));
	printf("rotated_fixture actor=%s ", tag);
	nxPrintRows(pose.M);
	printf("\n");

	// The body record the joint rows read: actor+0x14 -> body, body+8 ->
	// record. +0x5c quaternion, +0xdc mass-frame 3x3, +0x124 mass-frame
	// quaternion, +0x134 mass-frame world 3x3, +0x158 world centre of mass,
	// +0x164 world inverse inertia.
	const unsigned char* body = *reinterpret_cast<const unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(&actor) + 0x14);
	const unsigned char* record = body ? *reinterpret_cast<const unsigned char* const*>(body + 8) : 0;
	if(!record)
		return;
	static const unsigned blocks[6][2] = { { 0x5c, 4 }, { 0xdc, 9 }, { 0x124, 4 }, { 0x134, 9 }, { 0x158, 3 },
		{ 0x164, 9 } };
	for(unsigned k = 0; k < 6; k++)
		{
		printf("rotated_fixture actor=%s record off=%03x words=", tag, blocks[k][0]);
		for(unsigned w = 0; w < blocks[k][1]; w++)
			{
			NxU32 word;
			memcpy(&word, record + blocks[k][0] + 4 * w, 4);
			printf("%s%08x", w ? "." : "", static_cast<unsigned>(word));
			}
		printf("\n");
		}
	}

// The rotated-body fixture (joint-open-items Task 4): the same two dynamic
// boxes with the same density route as nxBuildFixture, but actor a is turned
// 90 degrees about y and actor b carries an arbitrary unit quaternion,
// (1, 2, 3, 4) / sqrt(30). Both poses are printed as given and as read back.
static bool nxBuildRotatedFixture(NxScene& scene, NxActor** a, NxActor** b)
	{
	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 1.0f, 1.0f);
	NxBodyDesc body;
	NxActorDesc da;
	da.body = &body;
	da.density = 1.0f;
	da.shapes.pushBack(&box);
	da.globalPose.M = NxMat33(NxVec3(0.0f, 0.0f, 1.0f), NxVec3(0.0f, 1.0f, 0.0f), NxVec3(-1.0f, 0.0f, 0.0f));
	da.globalPose.t = NxVec3(0.0f, 1.0f, 0.0f);

	const NxReal rs = 1.0f / sqrtf(30.0f);
	NxBoxShapeDesc box2;
	box2.dimensions = NxVec3(1.0f, 1.0f, 1.0f);
	NxBodyDesc body2;
	NxActorDesc db;
	db.body = &body2;
	db.density = 1.0f;
	db.shapes.pushBack(&box2);
	db.globalPose.M = nxQuatMatrix(1.0f * rs, 2.0f * rs, 3.0f * rs, 4.0f * rs);
	db.globalPose.t = NxVec3(4.0f, -1.0f, 2.0f);

	printf("rotated_fixture input=a ");
	nxPrintRows(da.globalPose.M);
	printf(" ");
	nxPrintVec("t", da.globalPose.t);
	printf("\n");
	printf("rotated_fixture input=b ");
	nxPrintRows(db.globalPose.M);
	printf(" ");
	nxPrintVec("t", db.globalPose.t);
	printf("\n");

	*a = scene.createActor(da);
	if(!*a)
		return false;
	*b = scene.createActor(db);
	if(!*b)
		return false;
	nxPrintActorPose("a", **a);
	nxPrintActorPose("b", **b);
	return true;
	}

// Joint-open-items Task 4 review: two more posed fixtures, each in a scene of
// its own, so that the matrix-to-quaternion conversions at actor creation
// (000801, and 000768's for +0x124) take their x, y and z pivot arms
// (non-positive trace): 180 degrees about x and about y, then two general
// rotations whose largest diagonal is x and y. The poses are printed as input
// and as read back, with the same body-record words as the rotated fixture.
static bool nxBuildPosedFixture(NxScene& scene, const char* label,
	const NxMat33& ma, const NxMat33& mb, NxActor** a, NxActor** b)
	{
	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 1.0f, 1.0f);
	NxBodyDesc body;
	NxActorDesc da;
	da.body = &body;
	da.density = 1.0f;
	da.shapes.pushBack(&box);
	da.globalPose.M = ma;
	da.globalPose.t = NxVec3(0.0f, 1.0f, 0.0f);
	NxBoxShapeDesc box2;
	box2.dimensions = NxVec3(1.0f, 1.0f, 1.0f);
	NxBodyDesc body2;
	NxActorDesc db;
	db.body = &body2;
	db.density = 1.0f;
	db.shapes.pushBack(&box2);
	db.globalPose.M = mb;
	db.globalPose.t = NxVec3(4.0f, -1.0f, 2.0f);
	printf("posed_fixture=%s input=a ", label);
	nxPrintRows(da.globalPose.M);
	printf("\n");
	printf("posed_fixture=%s input=b ", label);
	nxPrintRows(db.globalPose.M);
	printf("\n");
	*a = scene.createActor(da);
	*b = *a ? scene.createActor(db) : 0;
	printf("posed_fixture=%s a,%s b,%s\n", label, *a ? "created" : "null", *b ? "created" : "null");
	if(!*a || !*b)
		return false;
	char tag[64];
	sprintf(tag, "%s_a", label);
	nxPrintActorPose(tag, **a);
	sprintf(tag, "%s_b", label);
	nxPrintActorPose(tag, **b);
	return true;
	}

static void nxPosedScene(NxPhysicsSDK& sdk, const NxSceneDesc& sceneDesc, const char* label,
	const NxMat33& ma, const NxMat33& mb, unsigned firstIndex)
	{
	NxScene* scene = sdk.createScene(sceneDesc);
	printf("posed_scene=%s %s\n", label, scene ? "created" : "null");
	if(!scene)
		return;
	NxActor* a = 0;
	NxActor* b = 0;
	if(nxBuildPosedFixture(*scene, label, ma, mb, &a, &b))
		{
		nxAllFamiliesCase(*scene, a, b, firstIndex, NxVec3(1.0f, 2.0f, 3.0f), nxUnit(0.6f, -0.8f, 0.0f));
		nxAllFamiliesCase(*scene, a, b, firstIndex + 1, NxVec3(-1.5f, 0.25f, 8.0f), nxUnit(0.1f, 0.2f, 0.97f));
		}
	sdk.releaseScene(*scene);
	printf("posed_scene=%s released\n", label);
	}

#ifdef NX_PHYSICS_JOINT_ALLOCATOR_ONLY
// Joint-open-items Task 5 follow-up: which allocator the joint rows use.
//
// The oracle's joint rows allocate and free through the Foundation's allocator
// (`[[0x101041bc]]`, nxFoundationSDKAllocator), not through the allocator handed
// to NxCreatePhysicsSDK. The two are the same object only when the Physics SDK
// creates the Foundation. This process creates the Foundation first with
// allocator A and then the Physics SDK with a different allocator B, so the
// Foundation keeps A; it then creates and releases one joint of two families and
// prints how many blocks of which sizes each allocator saw, in each window.
// Only the joint windows are printed: scene and actor creation are other phases'
// rows and are not what this target measures.
class NxCountingAllocator : public NxUserAllocator
	{
	public:
	enum { LOG = 32 };
	explicit NxCountingAllocator(const char* name) : mName(name) { reset(); }

	void reset() { mMallocs = mFrees = mReallocs = 0; mLogged = 0; }

	virtual void* mallocDEBUG(size_t size, const char*, int) { return allocate(size); }
	virtual void* mallocDEBUG(size_t size, const char*, int, const char*, NxMemoryType) { return allocate(size); }
	virtual void* malloc(size_t size) { return allocate(size); }
	virtual void* malloc(size_t size, NxMemoryType) { return allocate(size); }
	virtual void* realloc(void* memory, size_t size) { mReallocs++; return ::realloc(memory, size); }
	virtual void free(void* memory) { if(memory) mFrees++; ::free(memory); }

	void print(const char* family, const char* window) const
		{
		printf("case=allocator family=%s window=%s allocator=%s mallocs=%u frees=%u reallocs=%u sizes=",
			family, window, mName, mMallocs, mFrees, mReallocs);
		for(unsigned i = 0; i < mLogged; i++)
			printf("%s%x", i ? "," : "", static_cast<unsigned>(mSizes[i]));
		printf("%s\n", mLogged ? "" : "none");
		}

	private:
	void* allocate(size_t size)
		{
		if(mLogged < LOG)
			mSizes[mLogged++] = size;
		mMallocs++;
		// Not zeroed: every block is filled with 0xcd before it is returned, so
		// a field that the constructors leave to the allocation reads 0xcdcdcdcd.
		// The oracle writes every Scene field it later reads; before the
		// Scene-initialisation fix the candidate relied on zeroed memory for
		// several of them and faulted in createActor's nxSceneArrayReserve (see
		// evidence/joint-open-items.md, Scene initialisation).
		void* memory = ::malloc(size);
		if(memory)
			memset(memory, 0xcd, size);
		return memory;
		}

	const char* mName;
	unsigned mMallocs, mFrees, mReallocs, mLogged;
	size_t mSizes[LOG];
	};

typedef NxFoundationSDK* (NX_CALL_CONV *CreateFoundationSDKFn)(NxU32, NxUserOutputStream*, NxUserAllocator*);

static void nxAllocatorWindow(const char* family, const char* window,
	NxCountingAllocator& foundation, NxCountingAllocator& physics)
	{
	foundation.print(family, window);
	physics.print(family, window);
	foundation.reset();
	physics.reset();
	}

static void nxAllocatorJoint(NxScene& scene, NxJointDesc& desc, NxActor* a, NxActor* b,
	const char* family, NxCountingAllocator& foundation, NxCountingAllocator& physics)
	{
	desc.actor[0] = a;
	desc.actor[1] = b;
	nxSetGlobalAnchor(desc, NxVec3(1.0f, 2.0f, 3.0f));
	nxSetGlobalAxis(desc, NxVec3(0.0f, 1.0f, 0.0f));
	foundation.reset();
	physics.reset();
	NxJoint* joint = scene.createJoint(desc);
	printf("case=allocator family=%s created=%s\n", family, joint ? "yes" : "no");
	nxAllocatorWindow(family, "create", foundation, physics);
	if(!joint)
		return;
	scene.releaseJoint(*joint);
	printf("case=allocator family=%s released=yes\n", family);
	nxAllocatorWindow(family, "release", foundation, physics);
	}

static int nxAllocatorCase(HMODULE physics, CreatePhysicsSDKFn createSDK, const wchar_t* pairDirectory)
	{
	// Unbuffered, so a fault leaves the lines before it.
	setvbuf(stdout, 0, _IONBF, 0);
	HMODULE foundationModule = GetModuleHandleW(L"NxFoundation.dll");
	CreateFoundationSDKFn createFoundation = foundationModule ?
		reinterpret_cast<CreateFoundationSDKFn>(GetProcAddress(foundationModule, "NxCreateFoundationSDK")) : 0;
	printf("export=NxCreateFoundationSDK present=%s\n", createFoundation ? "yes" : "no");
	if(!createFoundation)
		{
		FreeLibrary(physics);
		return nxFail("NxCreateFoundationSDK missing; the allocator case cannot be driven");
		}

	static NxCountingAllocator foundationAllocator("foundation");
	static NxCountingAllocator physicsAllocator("physics");
	NxFoundationSDK* foundation = createFoundation(NX_FOUNDATION_SDK_VERSION, 0, &foundationAllocator);
	printf("foundation=%s\n", foundation ? "created" : "null");
	NxPhysicsSDK* sdk = foundation ? createSDK(NX_PHYSICS_SDK_VERSION, &physicsAllocator, 0) : 0;
	printf("sdk=%s\n", sdk ? "created" : "null");
	if(!sdk)
		{
		FreeLibrary(physics);
		return nxFail("SDK creation failed");
		}

	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* scene = sdk->createScene(sceneDesc);
	printf("scene=%s\n", scene ? "created" : "null");
	NxActor* a = 0;
	NxActor* b = 0;
	if(!scene || !nxBuildFixture(*scene, &a, &b))
		{
		if(scene)
			sdk->releaseScene(*scene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("allocator fixture could not be created");
		}
	printf("fixture=a,%s b,%s\n", a ? "created" : "null", b ? "created" : "null");

	NxRevoluteJointDesc revoluteDesc;
	nxAllocatorJoint(*scene, revoluteDesc, a, b, "revolute", foundationAllocator, physicsAllocator);
	NxDistanceJointDesc distanceDesc;
	nxAllocatorJoint(*scene, distanceDesc, a, b, "distance", foundationAllocator, physicsAllocator);
	NxPrismaticJointDesc prismaticDesc;
	nxAllocatorJoint(*scene, prismaticDesc, a, b, "prismatic", foundationAllocator, physicsAllocator);

	sdk->releaseScene(*scene);
	printf("scene=released\n");
	sdk->release();
	printf("sdk=released\n");
	return nxReportPairIdentity(pairDirectory);
	}
#endif

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	// The gate launches an oracle differential with the pinned oracle's directory AND
	// its expected sha256, because the addresses it calls are only meaningful against
	// that exact file. nxOpenPair takes the directory alone, so the second argument is
	// consumed here and checked against what was actually loaded.
	if(argc != 2 && argc != 3)
		{
		fprintf(stderr, "usage: %s <absolute pair directory> [NxPhysics.dll sha256]\n",
			"NxPhysicsJointTests");
		return 2;
		}

	// The optional second argument is the expected sha256, supplied when this runs as
	// an oracle differential. A staged-pair run passes the directory alone.
	int status = nxOpenPair(argc == 3 ? argc - 1 : argc, argv,
		"NxPhysicsJointTests", pairDirectory, &physics);
	if(status)
		return status;

	CreatePhysicsSDKFn createSDK =
		reinterpret_cast<CreatePhysicsSDKFn>(GetProcAddress(physics, "NxCreatePhysicsSDK"));
	printf("export=NxCreatePhysicsSDK present=%s\n", createSDK ? "yes" : "no");
	if(!createSDK)
		{
		FreeLibrary(physics);
		return nxFail("NxCreatePhysicsSDK missing; joint cases cannot be driven");
		}

	nxSetGlobalAnchor = reinterpret_cast<JointDescSetGlobalAnchorFn>(
		GetProcAddress(physics, "NxJointDesc_SetGlobalAnchor"));
	nxSetGlobalAxis = reinterpret_cast<JointDescSetGlobalAxisFn>(
		GetProcAddress(physics, "NxJointDesc_SetGlobalAxis"));
	printf("export=NxJointDesc_SetGlobalAnchor present=%s\n", nxSetGlobalAnchor ? "yes" : "no");
	printf("export=NxJointDesc_SetGlobalAxis present=%s\n", nxSetGlobalAxis ? "yes" : "no");
	if(!nxSetGlobalAnchor || !nxSetGlobalAxis)
		{
		FreeLibrary(physics);
		return nxFail("the two exported joint-descriptor rows are missing");
		}
	printf("version=0x%08x\n", static_cast<unsigned>(NX_PHYSICS_SDK_VERSION));
#ifdef NX_PHYSICS_JOINT_ALLOCATOR_ONLY
	return nxAllocatorCase(physics, createSDK, pairDirectory);
#endif

	// Every SDK allocation goes through a page-guarded allocator, so a write past
	// the end of any block faults AT THE WRITE rather than corrupting a later one.
	static NxPageGuardedAllocator guardedAllocator;
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &guardedAllocator, &jointErrorStream);
	printf("sdk=%s\n", sdk ? "created" : "null");
	if(!sdk)
		{
		FreeLibrary(physics);
		return nxFail("SDK creation failed");
		}

	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* scene = sdk->createScene(sceneDesc);
	printf("scene=%s\n", scene ? "created" : "null");
	if(!scene)
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("scene creation failed");
		}

	NxActor* a = 0;
	NxActor* b = 0;
	if(!nxBuildFixture(*scene, &a, &b))
		{
		sdk->releaseScene(*scene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("joint fixture actors could not be created");
		}
	printf("fixture=a,%s b,%s\n", a ? "created" : "null", b ? "created" : "null");

	// The anchor and axis sweep. Four anchors and three axes, one word pattern
	// each, so a reader can see which word moved.
	nxRevoluteCase(*scene, a, b, 0, NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f));
	nxRevoluteCase(*scene, a, b, 1, NxVec3(1.0f, 2.0f, 3.0f), NxVec3(0.0f, 1.0f, 0.0f));
	nxRevoluteCase(*scene, a, b, 2, NxVec3(-1.5f, 0.25f, 8.0f), NxVec3(0.0f, 0.0f, 1.0f));
	nxRevoluteCase(*scene, a, b, 3, NxVec3(2.0f, 4.0f, 0.0f), NxVec3(0.5f, 0.5f, 0.5f));

	// The prismatic family (joint-families Task 3a), over the revolute table's
	// index-0 and index-3 anchor/axis values.
	nxPrismaticCase(*scene, a, b, 0, NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f));
	nxPrismaticCase(*scene, a, b, 3, NxVec3(2.0f, 4.0f, 0.0f), NxVec3(0.5f, 0.5f, 0.5f));

	// The cylindrical family (joint-families Task 3b), over the same two
	// anchor/axis values.
	nxCylindricalCase(*scene, a, b, 0, NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f));
	nxCylindricalCase(*scene, a, b, 3, NxVec3(2.0f, 4.0f, 0.0f), NxVec3(0.5f, 0.5f, 0.5f));

	// The spherical family (joint-families Task 3c), over the same two
	// anchor/axis values.
	nxSphericalCase(*scene, a, b, 0, NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f));
	nxSphericalCase(*scene, a, b, 3, NxVec3(2.0f, 4.0f, 0.0f), NxVec3(0.5f, 0.5f, 0.5f));

	// The point-on-line family (joint-families Task 3d), over the same two
	// anchor/axis values.
	nxPointOnLineCase(*scene, a, b, 0, NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f));
	nxPointOnLineCase(*scene, a, b, 3, NxVec3(2.0f, 4.0f, 0.0f), NxVec3(0.5f, 0.5f, 0.5f));
	nxPointInPlaneCase(*scene, a, b, 0, NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f));
	nxPointInPlaneCase(*scene, a, b, 3, NxVec3(2.0f, 4.0f, 0.0f), NxVec3(0.5f, 0.5f, 0.5f));
	// The distance family's fields: index 0 enables both limits and the
	// spring with distinct values; index 3 is a rigid rod (min == max, both
	// limits, no spring).
	nxDistanceCase(*scene, a, b, 0, NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f),
		2.5f, 0.5f, NxSpringDesc(10.0f, 0.5f, 0.25f),
		NX_DJF_MAX_DISTANCE_ENABLED | NX_DJF_MIN_DISTANCE_ENABLED | NX_DJF_SPRING_ENABLED);
	nxDistanceCase(*scene, a, b, 3, NxVec3(2.0f, 4.0f, 0.0f), NxVec3(0.5f, 0.5f, 0.5f),
		1.25f, 1.25f, NxSpringDesc(), NX_DJF_MAX_DISTANCE_ENABLED | NX_DJF_MIN_DISTANCE_ENABLED);
	// The pulley family's fields (this SDK's NxPulleyJointDesc has no motor):
	// index 0 is a rigid rope over two pulleys with distinct distance,
	// stiffness and ratio; index 3 moves both pulleys off the axes and clears
	// the flags.
	nxPulleyCase(*scene, a, b, 0, NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f),
		NxVec3(0.0f, 5.0f, 0.0f), NxVec3(4.0f, 5.0f, 0.0f), 6.0f, 0.75f, 1.5f, NX_PJF_IS_RIGID);
	nxPulleyCase(*scene, a, b, 3, NxVec3(2.0f, 4.0f, 0.0f), NxVec3(0.5f, 0.5f, 0.5f),
		NxVec3(1.0f, 2.0f, 3.0f), NxVec3(-2.0f, 6.0f, 0.5f), 3.25f, 0.5f, 2.0f, 0);
	// The fixed family (joint-families Task 3h), over the same two
	// anchor/axis values.
	nxFixedCase(*scene, a, b, 0, NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f));
	nxFixedCase(*scene, a, b, 3, NxVec3(2.0f, 4.0f, 0.0f), NxVec3(0.5f, 0.5f, 0.5f));
	// The D6 family (joint-families Task 3i), over the same two anchor/axis
	// values, with the field sets nxD6First/nxD6Second and nxD6Sentinel.
	nxD6Case(*scene, a, b, 0, NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f), nxD6First, nxD6Sentinel);
	nxD6Case(*scene, a, b, 3, NxVec3(2.0f, 4.0f, 0.0f), NxVec3(0.5f, 0.5f, 0.5f), nxD6Second, nxD6Sentinel);

	// The release-then-create cycle, after every family case has released its
	// joint; it leaves two joints for the scene release below.
	nxReleaseCycleCase(*scene, a, b);

	// Joint-open-items Task 4: near-z axes (|axis.z| > 0.707, the arm of
	// NxNormalToTangents that joint-families could not test) over the identity
	// fixture, every family: (0.1, 0.2, 0.97) normalised, and exactly (0, 0, 1).
	// The cycle's two joints are still registered, so the scene counts are 3/2.
	nxPrintInternal = true;
	nxAllFamiliesCase(*scene, a, b, 4, NxVec3(1.0f, -2.0f, 0.5f), nxUnit(0.1f, 0.2f, 0.97f));
	nxAllFamiliesCase(*scene, a, b, 5, NxVec3(-0.5f, 1.5f, 2.0f), NxVec3(0.0f, 0.0f, 1.0f));

	sdk->releaseScene(*scene);
	printf("scene=released\n");

	// Joint-open-items Task 4: the rotated-body fixture, in a scene of its own,
	// every family over a general axis, a diagonal axis and the two near-z axes.
	NxScene* rotatedScene = sdk->createScene(sceneDesc);
	printf("rotated_scene=%s\n", rotatedScene ? "created" : "null");
	if(rotatedScene)
		{
		NxActor* ra = 0;
		NxActor* rb = 0;
		const bool built = nxBuildRotatedFixture(*rotatedScene, &ra, &rb);
		printf("rotated_fixture=a,%s b,%s\n", ra ? "created" : "null", rb ? "created" : "null");
		if(built)
			{
			nxAllFamiliesCase(*rotatedScene, ra, rb, 10, NxVec3(1.0f, 2.0f, 3.0f), nxUnit(0.6f, -0.8f, 0.0f));
			nxAllFamiliesCase(*rotatedScene, ra, rb, 11, NxVec3(2.0f, 4.0f, 0.0f), NxVec3(0.5f, 0.5f, 0.5f));
			nxAllFamiliesCase(*rotatedScene, ra, rb, 12, NxVec3(-1.5f, 0.25f, 8.0f), nxUnit(0.1f, 0.2f, 0.97f));
			nxAllFamiliesCase(*rotatedScene, ra, rb, 13, NxVec3(0.0f, 0.0f, 0.0f), NxVec3(0.0f, 0.0f, 1.0f));
			}
		sdk->releaseScene(*rotatedScene);
		printf("rotated_scene=released\n");
		}

	// Joint-open-items Task 4 review: the pivot arms (see nxBuildPosedFixture).
	nxPosedScene(*sdk, sceneDesc, "flip_xy", nxQuatMatrix(1.0f, 0.0f, 0.0f, 0.0f),
		nxQuatMatrix(0.0f, 1.0f, 0.0f, 0.0f), 20);
	{
	const NxReal ix = 1.0f / sqrtf(0.95f * 0.95f + 0.2f * 0.2f + 0.1f * 0.1f + 0.2f * 0.2f);
	const NxReal iy = 1.0f / sqrtf(0.15f * 0.15f + 0.9f * 0.9f + 0.3f * 0.3f + 0.25f * 0.25f);
	nxPosedScene(*sdk, sceneDesc, "near_xy", nxQuatMatrix(0.95f * ix, 0.2f * ix, 0.1f * ix, 0.2f * ix),
		nxQuatMatrix(0.15f * iy, 0.9f * iy, -0.3f * iy, 0.25f * iy), 22);
	}
	sdk->release();
	printf("sdk=released\n");

	return nxReportPairIdentity(pairDirectory);
	}
