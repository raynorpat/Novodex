// Mass from shapes (actor-mass Task 1): dynamic actors created with a
// density and no massSpaceInertia, so that Scene::createActor's body
// creation (phys_fn_000026) computes mass, mass frame and inertia through
// phys_fn_000008 and the shapes' slot-4 rows. Each case prints the actor's
// mass, its mass-space inertia and its centre-of-mass local pose as words;
// the refusals print only whether createActor returned an actor.
#include "PhysicsPairLoader.h"
#include "NxPageGuardedAllocator.h"
#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxActor.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxCapsuleShapeDesc.h"
#include "NxPlaneShapeDesc.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

static unsigned bits(float f) { unsigned u; memcpy(&u, &f, 4); return u; }

// A rotation matrix from a unit quaternion (x, y, z, w), formed in the
// harness; both sides receive the same words.
static NxMat33 quatMatrix(float x, float y, float z, float w)
{
	const float inv = 1.0f / sqrtf(x * x + y * y + z * z + w * w);
	x *= inv; y *= inv; z *= inv; w *= inv;
	NxMat33 m;
	m.setRow(0, NxVec3(1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - w * z), 2.0f * (x * z + w * y)));
	m.setRow(1, NxVec3(2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z - w * x)));
	m.setRow(2, NxVec3(2.0f * (x * z - w * y), 2.0f * (y * z + w * x), 1.0f - 2.0f * (x * x + y * y)));
	return m;
}

static void report(const char* tag, NxActor* actor)
{
	printf("mass %s created=%u\n", tag, actor ? 1u : 0u);
	if(!actor) return;
	const NxVec3 inertia = actor->getMassSpaceInertiaTensor();
	printf("mass %s mass=%x inertia=%x.%x.%x\n", tag, bits(actor->getMass()),
		bits(inertia.x), bits(inertia.y), bits(inertia.z));
	const NxMat34 pose = actor->getCMassLocalPose();
	float m[9]; pose.M.getRowMajor(m);
	printf("mass %s cmass_local_pose=%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x\n", tag,
		bits(m[0]), bits(m[1]), bits(m[2]), bits(m[3]), bits(m[4]), bits(m[5]),
		bits(m[6]), bits(m[7]), bits(m[8]),
		bits(pose.t.x), bits(pose.t.y), bits(pose.t.z));
}

// One dynamic actor at a translated global pose over the given shapes.
static NxActor* create(NxScene* scene, NxShapeDesc* const* shapes, unsigned count,
	float density, float mass)
{
	NxBodyDesc body;
	body.mass = mass;
	NxActorDesc desc;
	for(unsigned i = 0; i < count; ++i)
		desc.shapes.pushBack(shapes[i]);
	desc.body = &body;
	desc.density = density;
	desc.globalPose.t = NxVec3(1.0f, 2.0f, 3.0f);
	return scene->createActor(desc);
}

int wmain(int argc, wchar_t** argv)
{
	setvbuf(stdout, 0, _IONBF, 0);
	wchar_t pairDirectory[MAX_PATH]; HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsActorMassTests", pairDirectory, &physics);
	if(status) return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK) return nxFail("NxCreatePhysicsSDK missing");
	static NxPageGuardedAllocator allocator;
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &allocator, 0);
	if(!sdk) return nxFail("SDK creation failed");
	NxSceneDesc sceneDesc; sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) return nxFail("scene creation failed");

	const NxMat33 rotation = quatMatrix(0.3f, -0.5f, 0.2f, 0.8f);
	const NxMat33 tilt = quatMatrix(-0.2f, 0.1f, 0.6f, 0.75f);

	// The single shapes, centred.
	NxSphereShapeDesc sphere; sphere.radius = 0.75f;
	NxBoxShapeDesc box; box.dimensions = NxVec3(0.5f, 1.25f, 2.0f);
	NxBoxShapeDesc cube; cube.dimensions = NxVec3(0.25f, 0.25f, 0.25f);
	NxCapsuleShapeDesc capsule; capsule.radius = 0.4f; capsule.height = 1.5f;
	NxShapeDesc* shapes[4];
	shapes[0] = &sphere; report("sphere", create(scene, shapes, 1, 2.0f, 0.0f));
	shapes[0] = &box; report("box", create(scene, shapes, 1, 1.5f, 0.0f));
	shapes[0] = &cube; report("cube", create(scene, shapes, 1, 3.0f, 0.0f));
	shapes[0] = &capsule; report("capsule", create(scene, shapes, 1, 0.5f, 0.0f));

	// The single shapes with a local pose: the frame's rotation (000831) and
	// translation (000833) before 000841 moves it back to its centre.
	NxSphereShapeDesc sphereAt; sphereAt.radius = 0.5f;
	sphereAt.localPose.t = NxVec3(0.25f, -1.0f, 0.5f);
	NxBoxShapeDesc boxAt; boxAt.dimensions = NxVec3(0.5f, 1.25f, 2.0f);
	boxAt.localPose.M = rotation; boxAt.localPose.t = NxVec3(-0.75f, 0.5f, 1.25f);
	NxCapsuleShapeDesc capsuleAt; capsuleAt.radius = 0.3f; capsuleAt.height = 2.0f;
	capsuleAt.localPose.M = tilt; capsuleAt.localPose.t = NxVec3(1.0f, 0.0f, -0.5f);
	shapes[0] = &sphereAt; report("sphere_pose", create(scene, shapes, 1, 2.0f, 0.0f));
	shapes[0] = &boxAt; report("box_pose", create(scene, shapes, 1, 1.5f, 0.0f));
	shapes[0] = &capsuleAt; report("capsule_pose", create(scene, shapes, 1, 0.5f, 0.0f));

	// The compound (001024): three posed children, then the same three with a
	// trigger child the walk skips.
	shapes[0] = &boxAt; shapes[1] = &sphereAt; shapes[2] = &capsuleAt;
	report("compound", create(scene, shapes, 3, 1.25f, 0.0f));
	NxBoxShapeDesc trigger; trigger.dimensions = NxVec3(3.0f, 3.0f, 3.0f);
	trigger.localPose.t = NxVec3(0.0f, 4.0f, 0.0f);
	trigger.shapeFlags |= NX_TRIGGER_ENABLE;
	shapes[3] = &trigger;
	report("compound_trigger", create(scene, shapes, 4, 1.25f, 0.0f));

	// The other two scaling arms of 000008: an explicit mass with a density
	// (the tensor scales by the density, the mass is kept) and an explicit
	// mass without one (the tensor scales by mass over the shapes' mass).
	shapes[0] = &boxAt; shapes[1] = &sphereAt;
	report("compound_mass_density", create(scene, shapes, 2, 1.25f, 7.0f));
	report("compound_mass_only", create(scene, shapes, 2, 0.0f, 7.0f));
	shapes[0] = &capsuleAt;
	report("capsule_mass_only", create(scene, shapes, 1, 0.0f, 3.0f));

	// The refusals: no non-trigger shape (000008 returns 2), alone and in a
	// compound.
	NxSphereShapeDesc sensor; sensor.radius = 1.0f;
	sensor.shapeFlags |= NX_TRIGGER_ON_ENTER;
	shapes[0] = &sensor;
	report("trigger_only", create(scene, shapes, 1, 1.0f, 0.0f));
	shapes[0] = &trigger; shapes[1] = &sensor;
	report("compound_trigger_only", create(scene, shapes, 2, 1.0f, 0.0f));

	sdk->releaseScene(*scene); sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
