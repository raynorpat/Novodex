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
	const unsigned char* initialShape = *reinterpret_cast<unsigned char* const*>(body + 0x10);
	const unsigned char* initialPruner = *reinterpret_cast<unsigned char* const*>(initialShape + 0xc4);
	const unsigned char* initialEntries = initialPruner
		? *reinterpret_cast<unsigned char* const*>(initialPruner + 0x14) : 0;
	printf("setter initial_pruner=%u.%x.%x.%x.%x.%x.%x.%x\n",
		*reinterpret_cast<const unsigned char*>(initialShape + 0xcf),
		initialPruner ? word(initialPruner, 0x38) : 0u,
		initialEntries ? word(initialEntries, 0) : 0u,
		initialEntries ? word(initialEntries, 4) : 0u,
		initialEntries ? word(initialEntries, 8) : 0u,
		initialEntries ? word(initialEntries, 12) : 0u,
		initialEntries ? word(initialEntries, 16) : 0u,
		initialEntries ? word(initialEntries, 20) : 0u);
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
	const unsigned char* positionShape = *reinterpret_cast<unsigned char* const*>(body + 0x10);
	const unsigned char* positionPruner = *reinterpret_cast<unsigned char* const*>(
		positionShape + 0xc4);
	const unsigned beforePositionPrunerEpoch = positionPruner
		? word(positionPruner, 0x38) : 0u;
	PROBE_DIRTY("position", actor->setGlobalPosition(NxVec3(3.0f, -2.0f, 5.0f)));
	const NxVec3 updatedPosition = actor->getGlobalPositionVal();
	printf("setter position=%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
		bits(updatedPosition.x), bits(updatedPosition.y), bits(updatedPosition.z),
		word(record, 0x50), word(record, 0x54), word(record, 0x58),
		word(record, 0x18), word(record, 0x1c), word(record, 0x20),
		word(record, 0x158), word(record, 0x15c), word(record, 0x160));
	const unsigned char* movedShape = *reinterpret_cast<unsigned char* const*>(body + 0x10);
	printf("setter shape_position=%x.%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
		word(movedShape, 0x30), word(movedShape, 0x34), word(movedShape, 0x38),
		word(movedShape, 0x60), word(movedShape, 0x64), word(movedShape, 0x68),
		word(movedShape, 0x90), word(movedShape, 0x94), word(movedShape, 0x98),
		word(movedShape, 0xdc));
	unsigned char* mutableShape = const_cast<unsigned char*>(movedShape);
	unsigned char oldCurrentQuaternion[16], oldShadowQuaternion[16], oldLocalPose[0x30];
	unsigned char oldMassOffset[12];
	memcpy(oldCurrentQuaternion, record + 0x5c, sizeof(oldCurrentQuaternion));
	memcpy(oldShadowQuaternion, record + 0x24, sizeof(oldShadowQuaternion));
	memcpy(oldLocalPose, mutableShape + 0x6c, sizeof(oldLocalPose));
	memcpy(oldMassOffset, record + 0x100, sizeof(oldMassOffset));
	const float quarterQuaternion[4] = {0.0f, 0.0f, 0.70710677f, 0.70710677f};
	const float localPose[12] = {1.0f,0.0f,0.0f,0.0f,0.0f,-1.0f,
		0.0f,1.0f,0.0f,0.5f,-0.25f,0.75f};
	const float massOffset[3] = {0.25f, -0.5f, 0.75f};
	memcpy(mutableRecord + 0x5c, quarterQuaternion, sizeof(quarterQuaternion));
	memcpy(mutableRecord + 0x24, quarterQuaternion, sizeof(quarterQuaternion));
	memcpy(mutableShape + 0x6c, localPose, sizeof(localPose));
	memcpy(mutableRecord + 0x100, massOffset, sizeof(massOffset));
	actor->setGlobalPosition(NxVec3(1.0f, -3.0f, 2.0f));
	printf("setter rotated_center=%x.%x.%x\n", word(record, 0x158),
		word(record, 0x15c), word(record, 0x160));
	printf("setter shape_rotated_pose=");
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", word(movedShape, 0x0c + 4 * i));
	printf(".%x\n", word(movedShape, 0xdc));
	printf("setter shape_pruner_state=%x.%x.%x.%x.%x\n",
		word(movedShape, 0xac), word(movedShape, 0xc8),
		word(movedShape, 0xcc), word(movedShape, 0xd0),
		word(movedShape, 0xd4));
	const unsigned char* broadphase = *reinterpret_cast<unsigned char* const*>(
		movedShape + 0xc4);
	const unsigned char* broadphaseEntry = broadphase
		? *reinterpret_cast<unsigned char* const*>(broadphase + 0x14) : 0;
	const unsigned shapeIndex = *reinterpret_cast<const unsigned short*>(
		movedShape + 0xcc);
	printf("setter pruner_update=%u.%u.%u.%u.%x.%x.%x.%x.%x.%x\n",
		broadphase ? 1u : 0u,
		*reinterpret_cast<const unsigned char*>(movedShape + 0xcf),
		shapeIndex,
		broadphase ? word(broadphase, 0x38) - beforePositionPrunerEpoch : 0u,
		broadphaseEntry ? word(broadphaseEntry + shapeIndex * 0x18, 0) : 0u,
		broadphaseEntry ? word(broadphaseEntry + shapeIndex * 0x18, 4) : 0u,
		broadphaseEntry ? word(broadphaseEntry + shapeIndex * 0x18, 8) : 0u,
		broadphaseEntry ? word(broadphaseEntry + shapeIndex * 0x18, 12) : 0u,
		broadphaseEntry ? word(broadphaseEntry + shapeIndex * 0x18, 16) : 0u,
		broadphaseEntry ? word(broadphaseEntry + shapeIndex * 0x18, 20) : 0u);
	memcpy(mutableRecord + 0x5c, oldCurrentQuaternion, sizeof(oldCurrentQuaternion));
	memcpy(mutableRecord + 0x24, oldShadowQuaternion, sizeof(oldShadowQuaternion));
	memcpy(mutableShape + 0x6c, oldLocalPose, sizeof(oldLocalPose));
	memcpy(mutableRecord + 0x100, oldMassOffset, sizeof(oldMassOffset));
	actor->setGlobalPosition(NxVec3(3.0f, -2.0f, 5.0f));
	// The actor setter is driven with a known local pose. Shape creation's
	// default local-pose initialization is tracked separately in Phase 4.
	const float identityLocalPose[12] = {1.0f,0.0f,0.0f,
		0.0f,1.0f,0.0f,0.0f,0.0f,1.0f,0.0f,0.0f,0.0f};
	memcpy(mutableShape + 0x6c, identityLocalPose, sizeof(identityLocalPose));
	NxQuat rotation;
	rotation.x = 0.0f;
	rotation.y = 0.0f;
	rotation.z = 0.70710677f;
	rotation.w = 0.70710677f;
	PROBE_DIRTY("orientation_quat", actor->setGlobalOrientationQuat(rotation));
	printf("setter orientation_quat=%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
		word(record, 0x5c), word(record, 0x60), word(record, 0x64), word(record, 0x68),
		word(record, 0x24), word(record, 0x28), word(record, 0x2c), word(record, 0x30),
		word(record, 0x158), word(record, 0x15c), word(record, 0x160));
	printf("setter shape_orientation_quat=");
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", word(movedShape, 0x0c + 4 * i));
	printf(".%x\n", word(movedShape, 0xdc));
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
	staticActor->setGlobalPosition(NxVec3(-4.0f, 6.0f, -8.0f));
	const unsigned char* staticBody = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(staticActor) + 0x14);
	const NxVec3 staticPosition = staticActor->getGlobalPositionVal();
	printf("setter static_position=%x.%x.%x.%x.%x.%x\n",
		bits(staticPosition.x), bits(staticPosition.y), bits(staticPosition.z),
		word(staticBody, 0x44), word(staticBody, 0x48), word(staticBody, 0x4c));
	staticActor->setGlobalOrientationQuat(rotation);
	printf("setter static_orientation_quat=");
	for(unsigned i = 0; i < 9; ++i)
		printf("%s%x", i ? "." : "", word(staticBody, 0x20 + 4 * i));
	printf("\n");
	scene->releaseActor(*staticActor);
	NxBoxShapeDesc secondBox;
	secondBox.dimensions = NxVec3(0.5f, 1.0f, 1.5f);
	NxActorDesc multiDesc = actorDesc;
	multiDesc.shapes.pushBack(&secondBox);
	NxActor* multiActor = scene->createActor(multiDesc);
	printf("setter multi_created=%u\n", multiActor ? 1u : 0u);
	if(!multiActor) return nxFail("multi-shape actor creation failed");
	multiActor->setGlobalPosition(NxVec3(2.0f, 3.0f, -4.0f));
	const unsigned char* multiBody = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(multiActor) + 0x14);
	const unsigned char* multiGroup = *reinterpret_cast<unsigned char* const*>(
		multiBody + 0x10);
	const unsigned char* const* firstChild = *reinterpret_cast<unsigned char* const* const*>(
		multiGroup + 0xe0);
	const unsigned char* const* lastChild = *reinterpret_cast<unsigned char* const* const*>(
		multiGroup + 0xe4);
	printf("setter multi_position=%u", static_cast<unsigned>(lastChild - firstChild));
	for(const unsigned char* const* child = firstChild; child != lastChild; ++child)
		printf(".%x.%x.%x", word(*child, 0x30), word(*child, 0x34),
			word(*child, 0x38));
	printf("\n");
	scene->releaseActor(*multiActor);
	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
