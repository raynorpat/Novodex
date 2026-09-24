#include "PhysicsPairLoader.h"
#include "NxPageGuardedAllocator.h"
#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxActor.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"
#include <stdio.h>
#include <string.h>

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

static unsigned bits(float f) { unsigned u; memcpy(&u, &f, 4); return u; }
static void vector(const char* tag, const char* field, const NxVec3& v)
{
	printf("cmass %s %s=%x.%x.%x\n", tag, field, bits(v.x), bits(v.y), bits(v.z));
}
static void matrix(const char* tag, const char* field, const NxMat33& m)
{
	float v[9]; m.getRowMajor(v);
	printf("cmass %s %s=%x.%x.%x.%x.%x.%x.%x.%x.%x\n", tag, field,
		bits(v[0]), bits(v[1]), bits(v[2]), bits(v[3]), bits(v[4]),
		bits(v[5]), bits(v[6]), bits(v[7]), bits(v[8]));
}
static void pose(const char* tag, const char* field, const NxMat34& p)
{
	float v[9]; p.M.getRowMajor(v);
	printf("cmass %s %s=%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x\n", tag, field,
		bits(v[0]), bits(v[1]), bits(v[2]), bits(v[3]), bits(v[4]),
		bits(v[5]), bits(v[6]), bits(v[7]), bits(v[8]),
		bits(p.t.x), bits(p.t.y), bits(p.t.z));
}
static void probe(const char* tag, NxActor* actor)
{
	pose(tag, "local_pose", actor->getCMassLocalPose());
	vector(tag, "local_position", actor->getCMassLocalPosition());
	matrix(tag, "local_orientation", actor->getCMassLocalOrientation());
	pose(tag, "global_pose", actor->getCMassGlobalPose());
	vector(tag, "global_position", actor->getCMassGlobalPosition());
	matrix(tag, "global_orientation", actor->getCMassGlobalOrientation());
}
int wmain(int argc, wchar_t** argv)
{
	setvbuf(stdout, 0, _IONBF, 0);
	wchar_t pairDirectory[MAX_PATH]; HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsActorCMassTests", pairDirectory, &physics);
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
	NxBoxShapeDesc box; box.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxBodyDesc body; body.mass = 5.0f;
	body.massSpaceInertia = NxVec3(2.0f, 3.0f, 4.0f);
	NxActorDesc desc; desc.shapes.pushBack(&box); desc.body = &body;
	for(unsigned variant = 0; variant < 3; ++variant)
	{
		desc.globalPose.id(); body.massLocalPose.id();
		if(variant >= 1)
		{
			desc.globalPose.t = NxVec3(4.0f, 5.0f, 6.0f);
			desc.globalPose.M.setRow(0, NxVec3(0.0f, -1.0f, 0.0f));
			desc.globalPose.M.setRow(1, NxVec3(1.0f, 0.0f, 0.0f));
			desc.globalPose.M.setRow(2, NxVec3(0.0f, 0.0f, 1.0f));
			body.massLocalPose.t = NxVec3(1.0f, 2.0f, 3.0f);
		}
		if(variant >= 2)
		{
			body.massLocalPose.M.setRow(0, NxVec3(1.0f, 0.0f, 0.0f));
			body.massLocalPose.M.setRow(1, NxVec3(0.0f, 0.0f, -1.0f));
			body.massLocalPose.M.setRow(2, NxVec3(0.0f, 1.0f, 0.0f));
		}
		NxActor* actor = scene->createActor(desc);
		printf("cmass variant=%u created=%u\n", variant, actor ? 1u : 0u);
		if(!actor) return nxFail("actor creation failed");
		probe(variant == 0 ? "identity" : variant == 1 ? "offset" : "rotated", actor);
		scene->releaseActor(*actor);
	}
	desc.body = 0;
	desc.globalPose.id();
	NxActor* staticActor = scene->createActor(desc);
	printf("cmass static_created=%u\n", staticActor ? 1u : 0u);
	if(!staticActor) return nxFail("static actor creation failed");
	probe("static", staticActor);
	scene->releaseActor(*staticActor);
	sdk->releaseScene(*scene); sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
