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
static unsigned word(const unsigned char* bytes, unsigned offset)
{ unsigned u; memcpy(&u, bytes + offset, 4); return u; }
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
static void recordState(const char* tag, NxActor* actor)
{
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(actor) + 0x14);
	unsigned char* record = *reinterpret_cast<unsigned char**>(body + 8);
	unsigned char* aux = *reinterpret_cast<unsigned char**>(record + 0x120);
	unsigned* flags = *reinterpret_cast<unsigned**>(aux + 0x40);
	unsigned id = word(record, 0x11c);
	printf("cmass %s record=%x.%x.%x.%x.%x.%x.%x\n", tag,
		word(record, 0x124), word(record, 0x128),
		word(record, 0x12c), word(record, 0x130),
		word(record, 0x198), word(record, 0x84), flags[id]);
}
static void transformState(const char* tag, NxActor* actor)
{
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(actor) + 0x14);
	unsigned char* record = *reinterpret_cast<unsigned char**>(body + 8);
	printf("cmass %s transform=%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
		tag, word(record, 0x18), word(record, 0x1c), word(record, 0x20),
		word(record, 0x24), word(record, 0x28), word(record, 0x2c), word(record, 0x30),
		word(record, 0x50), word(record, 0x54), word(record, 0x58),
		word(record, 0x5c), word(record, 0x60), word(record, 0x64), word(record, 0x68),
		word(record, 0x100), word(record, 0x104), word(record, 0x108),
		word(record, 0x158), word(record, 0x15c), word(record, 0x160));
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
		if(variant == 0)
		{
			actor->wakeUp(0.1f);
			recordState("low_wake_before", actor);
			actor->setCMassOffsetLocalPosition(NxVec3(1.0f, 2.0f, 3.0f));
			probe("low_wake_after", actor);
			recordState("low_wake_after", actor);
		}
		if(variant == 1)
		{
			recordState("offset_initial", actor);
			actor->setCMassOffsetLocalPosition(NxVec3(2.0f, 3.0f, 4.0f));
			probe("set_local_position", actor);
			recordState("set_local_position", actor);
			NxMat33 xRotation(NX_IDENTITY_MATRIX);
			xRotation.setRow(1, NxVec3(0.0f, 0.0f, -1.0f));
			xRotation.setRow(2, NxVec3(0.0f, 1.0f, 0.0f));
			actor->setCMassOffsetLocalOrientation(xRotation);
			probe("set_local_orientation", actor);
			recordState("set_local_orientation", actor);
			NxMat34 localPose;
			localPose.id();
			localPose.M.setRow(0, NxVec3(0.0f, -1.0f, 0.0f));
			localPose.M.setRow(1, NxVec3(1.0f, 0.0f, 0.0f));
			localPose.t = NxVec3(3.0f, 4.0f, 5.0f);
			actor->setCMassOffsetLocalPose(localPose);
			probe("set_local_pose", actor);
			recordState("set_local_pose", actor);
			actor->setCMassOffsetGlobalPosition(NxVec3(7.0f, 8.0f, 9.0f));
			probe("set_global_offset_position", actor);
			pose("set_global_offset_position", "actor_pose", actor->getGlobalPose());
			recordState("set_global_offset_position", actor);
			transformState("set_global_offset_position", actor);
			actor->setCMassOffsetGlobalOrientation(xRotation);
			probe("set_global_offset_orientation", actor);
			pose("set_global_offset_orientation", "actor_pose", actor->getGlobalPose());
			recordState("set_global_offset_orientation", actor);
			NxMat34 worldPose;
			worldPose.id();
			worldPose.M.setRow(0, NxVec3(0.0f, 0.0f, 1.0f));
			worldPose.M.setRow(2, NxVec3(-1.0f, 0.0f, 0.0f));
			worldPose.t = NxVec3(8.0f, 9.0f, 10.0f);
			actor->setCMassOffsetGlobalPose(worldPose);
			probe("set_global_offset_pose", actor);
			pose("set_global_offset_pose", "actor_pose", actor->getGlobalPose());
			recordState("set_global_offset_pose", actor);
		}
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
