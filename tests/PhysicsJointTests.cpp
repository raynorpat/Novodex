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

#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxJoint.h"
#include "NxJointDesc.h"
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

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

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

static NxReal nxF(NxU32 bits)
	{
	NxReal value;
	memcpy(&value, &bits, 4);
	return value;
	}

// A vector is printed as three raw words, never as decimal.
static void nxPrintVec(const char* tag, const NxVec3& v)
	{
	printf("%s=%08x.%08x.%08x", tag, nxU(v.x), nxU(v.y), nxU(v.z));
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

	NxActor* ra = 0;
	NxActor* rb = 0;
	joint->getActors(&ra, &rb);
	printf("case=revolute index=%u actors a=%s b=%s\n", index,
		ra == a ? "match" : (ra ? "other" : "null"),
		rb == b ? "match" : (rb ? "other" : "null"));

	scene.releaseJoint(*joint);
	printf("case=revolute index=%u released=yes\n", index);
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

	scene.releaseJoint(*joint);
	printf("case=prismatic index=%u released=yes\n", index);
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

	scene.releaseJoint(*joint);
	printf("case=cylindrical index=%u released=yes\n", index);
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

	scene.releaseJoint(*joint);
	printf("case=spherical index=%u released=yes\n", index);
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

	scene.releaseJoint(*joint);
	printf("case=point_on_line index=%u released=yes\n", index);
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

	scene.releaseJoint(*joint);
	printf("case=point_in_plane index=%u released=yes\n", index);
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

	scene.releaseJoint(*joint);
	printf("case=distance index=%u released=yes\n", index);
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

	scene.releaseJoint(*joint);
	printf("case=pulley index=%u released=yes\n", index);
	}

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

	// Every SDK allocation goes through a page-guarded allocator, so a write past
	// the end of any block faults AT THE WRITE rather than corrupting a later one.
	static NxPageGuardedAllocator guardedAllocator;
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &guardedAllocator, 0);
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

	sdk->releaseScene(*scene);
	printf("scene=released\n");
	sdk->release();
	printf("sdk=released\n");

	return nxReportPairIdentity(pairDirectory);
	}