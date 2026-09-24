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
	printf("setter group_initial=%u.%u.%u\n",
		actor->isGroupSleeping() ? 1u : 0u,
		word(record, 0x1e8) == reinterpret_cast<unsigned>(record) ? 1u : 0u,
		word(record, 0x1fc) == 0 ? 1u : 0u);
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
	PROBE_DIRTY("sleep_linear", actor->setSleepLinearVelocity(0.25f));
	PROBE_DIRTY("sleep_angular", actor->setSleepAngularVelocity(0.5f));
	printf("setter sleep_thresholds=%x.%x.%x.%x\n",
		word(record, 0xd0), word(record, 0xd4),
		bits(actor->getSleepLinearVelocity()),
		bits(actor->getSleepAngularVelocity()));
	PROBE_DIRTY("wake", actor->wakeUp(0.75f));
	printf("setter wake_state=%x.%x.%x.%u\n", word(record, 0x84),
		word(record, 0x4c), word(record, 0x114),
		actor->isSleeping() ? 1u : 0u);
	PROBE_DIRTY("sleep", actor->putToSleep());
	printf("setter asleep_state=%x.%x.%x.%u\n", word(record, 0x84),
		word(record, 0x4c), word(record, 0x114),
		actor->isSleeping() ? 1u : 0u);
	printf("setter group_asleep=%u\n", actor->isGroupSleeping() ? 1u : 0u);
	PROBE_DIRTY("rewake", actor->wakeUp(0.5f));
	printf("setter rewake_state=%x.%x.%x.%u\n", word(record, 0x84),
		word(record, 0x4c), word(record, 0x114),
		actor->isSleeping() ? 1u : 0u);
	printf("setter group_rewake=%u\n", actor->isGroupSleeping() ? 1u : 0u);
	PROBE_DIRTY("negative_sleep_linear", actor->setSleepLinearVelocity(-0.25f));
	printf("setter negative_sleep_linear=%x.%x\n", word(record, 0xd0),
		bits(actor->getSleepLinearVelocity()));
	actor->wakeUp(-0.5f);
	printf("setter negative_wake=%x.%u.%u\n", word(record, 0x84),
		actor->isSleeping() ? 1u : 0u,
		actor->isGroupSleeping() ? 1u : 0u);
	NxActor* other = scene->createActor(actorDesc);
	printf("setter group_two_created=%u\n", other ? 1u : 0u);
	if(!other) return nxFail("second actor creation failed");
	unsigned char* otherBody = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(other) + 0x14);
	unsigned char* otherRecord = *reinterpret_cast<unsigned char**>(otherBody + 8);
	unsigned char* mutableRecord = const_cast<unsigned char*>(record);
	*reinterpret_cast<unsigned char**>(otherRecord + 0x1e8) = mutableRecord;
	*reinterpret_cast<unsigned char**>(mutableRecord + 0x1fc) = otherRecord;
	printf("setter group_two_awake=%u.%u\n",
		actor->isGroupSleeping() ? 1u : 0u,
		other->isGroupSleeping() ? 1u : 0u);
	actor->putToSleep();
	printf("setter group_one_asleep=%u.%u\n",
		actor->isGroupSleeping() ? 1u : 0u,
		other->isGroupSleeping() ? 1u : 0u);
	other->putToSleep();
	printf("setter group_both_asleep=%u.%u\n",
		actor->isGroupSleeping() ? 1u : 0u,
		other->isGroupSleeping() ? 1u : 0u);
	actor->wakeUp(0.5f);
	printf("setter group_one_rewoke=%u.%u\n",
		actor->isGroupSleeping() ? 1u : 0u,
		other->isGroupSleeping() ? 1u : 0u);
	*reinterpret_cast<unsigned char**>(otherRecord + 0x1e8) = otherRecord;
	*reinterpret_cast<unsigned char**>(mutableRecord + 0x1fc) = 0;
	scene->releaseActor(*other);
#undef PROBE_DIRTY
	scene->releaseActor(*actor);
	NxActorDesc staticDesc;
	staticDesc.shapes.pushBack(&box);
	NxActor* staticActor = scene->createActor(staticDesc);
	printf("setter static_created=%u\n", staticActor ? 1u : 0u);
	if(!staticActor) return nxFail("static actor creation failed");
	printf("setter static_sleep=%u.%u.%x.%x\n",
		staticActor->isSleeping() ? 1u : 0u,
		staticActor->isGroupSleeping() ? 1u : 0u,
		bits(staticActor->getSleepLinearVelocity()),
		bits(staticActor->getSleepAngularVelocity()));
	staticActor->setSleepLinearVelocity(0.25f);
	staticActor->setSleepAngularVelocity(0.5f);
	staticActor->wakeUp(0.75f);
	staticActor->putToSleep();
	printf("setter static_after=%u.%u.%x.%x\n",
		staticActor->isSleeping() ? 1u : 0u,
		staticActor->isGroupSleeping() ? 1u : 0u,
		bits(staticActor->getSleepLinearVelocity()),
		bits(staticActor->getSleepAngularVelocity()));
	scene->releaseActor(*staticActor);
	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
