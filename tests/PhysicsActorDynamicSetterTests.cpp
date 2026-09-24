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
	int status = nxOpenPair(argc, argv, "NxPhysicsActorDynamicSetterTests",
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
	bodyDesc.linearDamping = 0.2f;
	bodyDesc.angularDamping = 0.3f;
	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&box);
	actorDesc.body = &bodyDesc;
	NxActor* actor = scene->createActor(actorDesc);
	printf("setter created=%u\n", actor ? 1u : 0u);
	if(!actor) return nxFail("actor creation failed");
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x14);
	const unsigned char* record = *reinterpret_cast<unsigned char* const*>(body + 8);
	printf("setter initial_wake=%x.%x.%x.%x\n",
		word(record, 0x84), word(record, 0x4c),
		word(record, 0xd0), word(record, 0xd4));
	const unsigned beforeAlloc = allocator.allocations();
	const unsigned beforeFree = allocator.frees();
	actor->setMass(8.0f);
	actor->setMassSpaceInertiaTensor(NxVec3(5.0f, 6.0f, 7.0f));
	printf("setter mass=%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
		word(record, 0x188), word(record, 0xc0), bits(actor->getMass()),
		word(record, 0x18c), word(record, 0x190), word(record, 0x194),
		word(record, 0xc4), word(record, 0xc8), word(record, 0xcc));
	actor->setLinearDamping(0.4f);
	actor->setAngularDamping(0.6f);
	printf("setter damping=%x.%x.%x.%x\n", word(record, 0xb8),
		word(record, 0xbc), bits(actor->getLinearDamping()),
		bits(actor->getAngularDamping()));
	actor->setLinearVelocity(NxVec3(7.0f, 8.0f, 9.0f));
	actor->setAngularVelocity(NxVec3(10.0f, 11.0f, 12.0f));
	NxVec3 linear = actor->getLinearVelocityVal();
	NxVec3 angular = actor->getAngularVelocityVal();
	printf("setter velocity=%x.%x.%x.%x.%x.%x\n",
		bits(linear.x), bits(linear.y), bits(linear.z),
		bits(angular.x), bits(angular.y), bits(angular.z));
	printf("setter record_velocity=%x.%x.%x.%x.%x.%x\n",
		word(record, 0x6c), word(record, 0x70), word(record, 0x74),
		word(record, 0x78), word(record, 0x7c), word(record, 0x80));
	printf("setter shadow_velocity=%x.%x.%x.%x.%x.%x\n",
		word(record, 0x34), word(record, 0x38), word(record, 0x3c),
		word(record, 0x40), word(record, 0x44), word(record, 0x48));
	printf("setter wake=%x.%x.%x.%x\n", word(record, 0x84),
		word(record, 0x4c), word(record, 0xd0), word(record, 0xd4));
	printf("setter mutation_allocs=%u.%u\n",
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
#define PROBE_DIRTY(label, expression) \
	managerFlags[id] = 0; \
	expression; \
	printf("setter dirty_" label "=%x.%u.%u\n", managerFlags[id], \
		static_cast<unsigned>(*activeEnd - activeBegin), managerIndex[id]); \
	managerFlags[id] = 0xffffffffu; \
	*activeEnd = savedEnd; \
	managerIndex[id] = savedIndex
	PROBE_DIRTY("mass", actor->setMass(9.0f));
	PROBE_DIRTY("inertia", actor->setMassSpaceInertiaTensor(NxVec3(8.0f, 9.0f, 10.0f)));
	PROBE_DIRTY("linear_damping", actor->setLinearDamping(0.5f));
	PROBE_DIRTY("angular_damping", actor->setAngularDamping(0.7f));
	PROBE_DIRTY("linear_velocity", actor->setLinearVelocity(NxVec3(13.0f, 14.0f, 15.0f)));
	PROBE_DIRTY("angular_velocity", actor->setAngularVelocity(NxVec3(16.0f, 17.0f, 18.0f)));
#undef PROBE_DIRTY
	actor->raiseBodyFlag(NX_BF_KINEMATIC);
	const unsigned beforeKinematicAlloc = allocator.allocations();
	const unsigned beforeKinematicFree = allocator.frees();
	actor->setLinearVelocity(NxVec3(19.0f, 20.0f, 21.0f));
	actor->setAngularVelocity(NxVec3(22.0f, 23.0f, 24.0f));
	printf("setter kinematic_velocity=%x.%x.%x.%x.%x.%x\n",
		word(record, 0x6c), word(record, 0x70), word(record, 0x74),
		word(record, 0x78), word(record, 0x7c), word(record, 0x80));
	printf("setter kinematic_allocs=%u.%u\n",
		allocator.allocations() - beforeKinematicAlloc,
		allocator.frees() - beforeKinematicFree);
	actor->clearBodyFlag(NX_BF_KINEMATIC);
	scene->releaseActor(*actor);
	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
