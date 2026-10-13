// Pins phys_fn_000557, which inserts a joint whose actors are detached by
// setDynamic onto the Scene's second joint list at +0x5a0.
#include "PhysicsPairLoader.h"

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxActor.h"
#include "NxBoxShapeDesc.h"
#include "NxFixedJointDesc.h"
#include "NxJoint.h"

#include <stdio.h>

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(
	NxU32, NxUserAllocator*, NxUserOutputStream*);

static const unsigned char* actorBody(const NxActor* actor)
	{
	return *reinterpret_cast<const unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x14);
	}

static NxActor* createDynamicBox(NxScene* scene, float x)
	{
	NxBoxShapeDesc shape;
	shape.dimensions = NxVec3(0.5f, 0.5f, 0.5f);
	NxBodyDesc body;
	body.mass = 2.0f;
	body.massSpaceInertia = NxVec3(1.0f, 1.0f, 1.0f);
	NxActorDesc desc;
	desc.shapes.pushBack(&shape);
	desc.body = &body;
	desc.globalPose.t = NxVec3(x, 0.0f, 0.0f);
	return scene->createActor(desc);
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsJointWithoutBodiesTests",
		pairDirectory, &physics);
	if(status) return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK) return nxFail("NxCreatePhysicsSDK is missing");
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
	if(!sdk) return nxFail("SDK creation failed");
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) return nxFail("scene creation failed");
	NxActor* actor0 = createDynamicBox(scene, 0.0f);
	NxActor* actor1 = createDynamicBox(scene, 3.0f);
	if(!actor0 || !actor1) return nxFail("dynamic actors failed to create");
	NxFixedJointDesc jointDesc;
	jointDesc.actor[0] = actor0;
	jointDesc.actor[1] = actor1;
	NxJoint* joint = scene->createJoint(jointDesc);
	if(!joint) return nxFail("fixed joint failed to create");

	NxBodyDesc replacementBody;
	replacementBody.mass = 2.0f;
	replacementBody.massSpaceInertia = NxVec3(1.0f, 1.0f, 1.0f);
	actor0->setDynamic(replacementBody);
	const unsigned char* const internal =
		static_cast<const unsigned char*>(joint->appData);
	const unsigned char* const body = actorBody(actor0);
	const unsigned char* const sceneInternal =
		*reinterpret_cast<const unsigned char* const*>(body + 4);
	const unsigned char* node =
		*reinterpret_cast<const unsigned char* const*>(sceneInternal + 0x5a0);
	unsigned nodes = 0;
	unsigned memberships = 0;
	for(; node && nodes < 16; ++nodes)
		{
		if(node == internal) ++memberships;
		node = *reinterpret_cast<const unsigned char* const*>(node + 0x10);
		}
	const bool nextNull = *reinterpret_cast<const void* const*>(internal + 0x10) == 0;
	printf("joint bodyless list member=%u nodes=%u next_null=%u\n",
		memberships, nodes, nextNull ? 1u : 0u);
	if(memberships != 1 || nodes != 1 || !nextNull)
		status = nxFail("detached joint is absent from the Scene bodyless-joint list");

	scene->releaseJoint(*joint);
	scene->releaseActor(*actor0);
	scene->releaseActor(*actor1);
	sdk->releaseScene(*scene);
	sdk->release();
	if(status) return status;
	return nxReportPairIdentity(pairDirectory);
	}
