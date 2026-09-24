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

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(
	NxU32, NxUserAllocator*, NxUserOutputStream*);

static unsigned bits(NxReal value)
{
	unsigned out;
	memcpy(&out, &value, sizeof(out));
	return out;
}

static unsigned word(const unsigned char* bytes, unsigned offset)
{
	unsigned out;
	memcpy(&out, bytes + offset, sizeof(out));
	return out;
}

int wmain(int argc, wchar_t** argv)
{
	setvbuf(stdout, 0, _IONBF, 0);
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsActorDynamicsTests",
		pairDirectory, &physics);
	if(status) return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK) return nxFail("NxCreatePhysicsSDK is missing");
	static NxPageGuardedAllocator allocator;
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &allocator, 0);
	if(!sdk) return nxFail("SDK creation failed");
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) return nxFail("scene creation failed");
	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxBodyDesc bodyDesc;
	bodyDesc.mass = 5.0f;
	bodyDesc.massSpaceInertia = NxVec3(2.0f, 3.0f, 4.0f);
	bodyDesc.linearVelocity = NxVec3(1.0f, 2.0f, 3.0f);
	bodyDesc.angularVelocity = NxVec3(4.0f, 5.0f, 6.0f);
	bodyDesc.linearDamping = 0.2f;
	bodyDesc.angularDamping = 0.3f;
	bodyDesc.maxAngularVelocity = 7.0f;
	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&box);
	actorDesc.body = &bodyDesc;
	NxActor* actor = scene->createActor(actorDesc);
	printf("dynamics created=%u\n", actor ? 1u : 0u);
	if(!actor) return nxFail("actor creation failed");
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x14);
	const unsigned char* record = *reinterpret_cast<unsigned char* const*>(body + 8);
	printf("dynamics mass=%x.%x.%x.%x\n", word(record, 0x188),
		word(record, 0xc0), bits(actor->getMass()),
		word(record, 0x10c));
	printf("dynamics inertia=%x.%x.%x.%x.%x.%x\n",
		word(record, 0x18c), word(record, 0x190), word(record, 0x194),
		word(record, 0xc4), word(record, 0xc8), word(record, 0xcc));
	printf("dynamics damping=%x.%x.%x.%x\n", word(record, 0xb8),
		word(record, 0xbc), bits(actor->getLinearDamping()),
		bits(actor->getAngularDamping()));
	NxVec3 linear = actor->getLinearVelocityVal();
	NxVec3 angular = actor->getAngularVelocityVal();
	printf("dynamics velocity=%x.%x.%x.%x.%x.%x\n",
		bits(linear.x), bits(linear.y), bits(linear.z),
		bits(angular.x), bits(angular.y), bits(angular.z));
	printf("dynamics record_velocity=%x.%x.%x.%x.%x.%x\n",
		word(record, 0x6c), word(record, 0x70), word(record, 0x74),
		word(record, 0x78), word(record, 0x7c), word(record, 0x80));
	const unsigned beforeAlloc = allocator.allocations();
	const unsigned beforeFree = allocator.frees();
	actor->raiseBodyFlag(NX_BF_KINEMATIC);
	printf("dynamics kinematic=%x.%x.%x.%x.%x.%u\n",
		word(record, 0x10c), word(record, 0xc0), word(record, 0xc4),
		word(record, 0xc8), word(record, 0xcc),
		*reinterpret_cast<void* const*>(record + 0x118) ? 1u : 0u);
	actor->clearBodyFlag(NX_BF_KINEMATIC);
	printf("dynamics restored=%x.%x.%x.%x.%x.%u\n",
		word(record, 0x10c), word(record, 0xc0), word(record, 0xc4),
		word(record, 0xc8), word(record, 0xcc),
		*reinterpret_cast<void* const*>(record + 0x118) ? 1u : 0u);
	printf("dynamics transition_allocs=%u.%u\n",
		allocator.allocations() - beforeAlloc, allocator.frees() - beforeFree);
	unsigned char* aux = *reinterpret_cast<unsigned char**>(
		const_cast<unsigned char*>(record) + 0x120);
	const unsigned id = word(record, 0x11c);
	unsigned* managerFlags = *reinterpret_cast<unsigned**>(aux + 0x40);
	unsigned* managerIndex = *reinterpret_cast<unsigned**>(aux + 0x60);
	unsigned** activeEnd = reinterpret_cast<unsigned**>(aux + 0x54);
	unsigned* activeBegin = *reinterpret_cast<unsigned**>(aux + 0x50);
	unsigned* savedEnd = *activeEnd;
	const unsigned savedIndex = managerIndex[id];
	managerFlags[id] = 0;
	actor->raiseBodyFlag(NX_BF_KINEMATIC);
	printf("dynamics kinematic_dirty=%x.%u.%u\n", managerFlags[id],
		static_cast<unsigned>(*activeEnd - activeBegin), managerIndex[id]);
	actor->clearBodyFlag(NX_BF_KINEMATIC);
	managerFlags[id] = 0xffffffffu;
	*activeEnd = savedEnd;
	managerIndex[id] = savedIndex;
	scene->releaseActor(*actor);
	NxBodyDesc densityBody;
	NxActorDesc densityDesc;
	densityDesc.shapes.pushBack(&box);
	densityDesc.body = &densityBody;
	densityDesc.density = 2.0f;
	NxActor* densityActor = scene->createActor(densityDesc);
	printf("dynamics density_created=%u\n", densityActor ? 1u : 0u);
	if(!densityActor) return nxFail("density actor creation failed");
	const unsigned char* densityOuter = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(densityActor) + 0x14);
	const unsigned char* densityRecord = *reinterpret_cast<unsigned char* const*>(
		densityOuter + 8);
	printf("dynamics density_mass=%x.%x.%x\n",
		word(densityRecord, 0x188), word(densityRecord, 0xc0),
		bits(densityActor->getMass()));
	printf("dynamics density_inertia=%x.%x.%x.%x.%x.%x\n",
		word(densityRecord, 0x18c), word(densityRecord, 0x190),
		word(densityRecord, 0x194), word(densityRecord, 0xc4),
		word(densityRecord, 0xc8), word(densityRecord, 0xcc));
	scene->releaseActor(*densityActor);
	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
