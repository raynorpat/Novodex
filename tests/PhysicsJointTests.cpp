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
#include "NxFixedJoint.h"
#include "NxFixedJointDesc.h"
#include "NxBitField.h"
#include "NxD6Joint.h"
#include "NxD6JointDesc.h"

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

	scene.releaseJoint(*joint);
	printf("case=fixed index=%u released=yes\n", index);
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

	scene.releaseJoint(*joint);
	printf("case=d6 index=%u released=yes\n", index);
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
	// The fixed family (joint-families Task 3h), over the same two
	// anchor/axis values.
	nxFixedCase(*scene, a, b, 0, NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f));
	nxFixedCase(*scene, a, b, 3, NxVec3(2.0f, 4.0f, 0.0f), NxVec3(0.5f, 0.5f, 0.5f));
	// The D6 family (joint-families Task 3i), over the same two anchor/axis
	// values. Index 0 mixes locked, limited and free motions (twist and swing1
	// limited, so the constructor forms their half-angle cosines); index 3
	// limits every motion. The sentinel set is what the saved descriptors hold
	// before saveToDesc.
	{
	NxD6CaseFields first = {
		{ NX_D6JOINT_MOTION_LOCKED, NX_D6JOINT_MOTION_LIMITED, NX_D6JOINT_MOTION_FREE,
		  NX_D6JOINT_MOTION_LIMITED, NX_D6JOINT_MOTION_LIMITED, NX_D6JOINT_MOTION_FREE },
		{ 0.5f, 0.25f, 0.75f }, -0.5f, 0.75f, 0.625f, 0.375f, 2.0f, true,
		NxVec3(1.0f, 2.0f, 3.0f), { 0.0f, 0.6f, 0.0f, 0.8f },
		NxVec3(0.5f, -0.5f, 1.5f), NxVec3(-0.25f, 0.125f, 2.5f),
		0.125f, 0.0625f, NX_JPM_POINT_MINDIST };
	NxD6CaseFields second = {
		{ NX_D6JOINT_MOTION_LIMITED, NX_D6JOINT_MOTION_LIMITED, NX_D6JOINT_MOTION_LIMITED,
		  NX_D6JOINT_MOTION_LIMITED, NX_D6JOINT_MOTION_LIMITED, NX_D6JOINT_MOTION_LIMITED },
		{ 1.25f, 0.0f, 1.0f }, -1.0f, 1.5f, 0.25f, 1.0f, 4.0f, false,
		NxVec3(-2.0f, 0.5f, 4.0f), { 0.0f, 0.0f, 0.0f, 1.0f },
		NxVec3(0.0f, 0.0f, 0.0f), NxVec3(3.0f, 0.0f, -3.0f),
		2.0f, 0.5f, NX_JPM_NONE };
	NxD6CaseFields sentinel = {
		{ NX_D6JOINT_MOTION_FREE, NX_D6JOINT_MOTION_LOCKED, NX_D6JOINT_MOTION_FREE,
		  NX_D6JOINT_MOTION_LOCKED, NX_D6JOINT_MOTION_FREE, NX_D6JOINT_MOTION_LOCKED },
		{ 9.0f, 0.5f, 0.5f }, -9.0f, 9.5f, 8.0f, 7.0f, 20.0f, true,
		NxVec3(11.0f, 12.0f, 13.0f), { 0.5f, 0.5f, 0.5f, 0.5f },
		NxVec3(21.0f, 22.0f, 23.0f), NxVec3(31.0f, 32.0f, 33.0f),
		99.0f, 98.0f, NX_JPM_POINT_MINDIST };
	nxD6Case(*scene, a, b, 0, NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f), first, sentinel);
	nxD6Case(*scene, a, b, 3, NxVec3(2.0f, 4.0f, 0.0f), NxVec3(0.5f, 0.5f, 0.5f), second, sentinel);
	}

	sdk->releaseScene(*scene);
	printf("scene=released\n");
	sdk->release();
	printf("sdk=released\n");

	return nxReportPairIdentity(pairDirectory);
	}