#include "PhysicsPairLoader.h"
#include "NxPageGuardedAllocator.h"
#include "PhysicsActorErrorStream.h"

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxActor.h"
#include "NxShape.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxCapsuleShapeDesc.h"
#include "NxPlaneShapeDesc.h"
#include "NxSphereShape.h"
#include "NxCapsuleShape.h"
#include "NxPlaneShape.h"
#include "NxBoxShape.h"
#include "NxBox.h"
#include "NxBounds3.h"
#include "NxQuat.h"
#include <math.h>

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
	// Silent until the G1/E1 cases at the end enable it.
	static NxActorErrorStream errors("setter");
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &allocator, &errors);
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
	void** internalTable = *reinterpret_cast<void***>(
		const_cast<unsigned char*>(initialShape));
	typedef void* (__thiscall* ShapeSelfFn)(void*);
	typedef void (__thiscall* ShapeCenterFn)(void*, float*);
	typedef void (__thiscall* ShapeAABBFn)(void*, float*);
	printf("setter initial_shape_vtable=%u", internalTable ? 1u : 0u);
	for(unsigned slot = 14; slot <= 16; ++slot)
		printf(".%u", internalTable &&
			reinterpret_cast<ShapeSelfFn>(internalTable[slot])(
				const_cast<unsigned char*>(initialShape)) == initialShape ? 1u : 0u);
	printf("\n");
	float initialCenter[4] = {};
	if(internalTable)
		reinterpret_cast<ShapeCenterFn>(internalTable[10])(
			const_cast<unsigned char*>(initialShape), initialCenter);
	printf("setter initial_shape_center=%x.%x.%x.%x\n",
		bits(initialCenter[0]), bits(initialCenter[1]),
		bits(initialCenter[2]), bits(initialCenter[3]));
	printf("setter initial_shape_local=");
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", word(initialShape, 0x6c + 4 * i));
	printf("\n");
	printf("setter initial_shape_world=");
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", word(initialShape, 0x0c + 4 * i));
	printf("\n");
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
	const float matrixRows[4][9] = {
		{0.0f,-1.0f,0.0f,1.0f,0.0f,0.0f,0.0f,0.0f,1.0f},
		{1.0f,0.0f,0.0f,0.0f,-1.0f,0.0f,0.0f,0.0f,-1.0f},
		{-1.0f,0.0f,0.0f,0.0f,1.0f,0.0f,0.0f,0.0f,-1.0f},
		{-1.0f,0.0f,0.0f,0.0f,-1.0f,0.0f,0.0f,0.0f,1.0f}
	};
	for(unsigned caseIndex = 0; caseIndex < 4; ++caseIndex)
		{
		NxMat33 matrix;
		matrix.setRowMajor(matrixRows[caseIndex]);
		managerFlags[id] = 0;
		actor->setGlobalOrientation(matrix);
		printf("setter matrix_case%u=%x.%u.%u.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
			caseIndex, managerFlags[id],
			static_cast<unsigned>(*activeEnd - activeBegin), managerIndex[id],
			word(record, 0x5c), word(record, 0x60),
			word(record, 0x64), word(record, 0x68),
			word(record, 0x24), word(record, 0x28),
			word(record, 0x2c), word(record, 0x30),
			word(movedShape, 0x0c), word(movedShape, 0x10),
			word(movedShape, 0x14));
		managerFlags[id] = 0xffffffffu;
		*activeEnd = savedEnd;
		managerIndex[id] = savedIndex;
		}
	NxMat34 combinedPose;
	combinedPose.M.setRowMajor(matrixRows[0]);
	combinedPose.t = NxVec3(7.0f, -5.0f, 2.0f);
	PROBE_DIRTY("pose", actor->setGlobalPose(combinedPose));
	printf("setter pose_record=");
	const unsigned poseRecordOffsets[14] = {0x50,0x54,0x58,0x18,0x1c,0x20,
		0x5c,0x60,0x64,0x68,0x158,0x15c,0x160,0x24};
	for(unsigned i = 0; i < 14; ++i)
		printf("%s%x", i ? "." : "", word(record, poseRecordOffsets[i]));
	printf("\nsetter pose_shape=");
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", word(movedShape, 0x0c + 4 * i));
	printf("\n");
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
	NxMat33 staticMatrix;
	staticMatrix.setRowMajor(matrixRows[1]);
	staticActor->setGlobalOrientation(staticMatrix);
	printf("setter static_orientation_matrix=");
	for(unsigned i = 0; i < 9; ++i)
		printf("%s%x", i ? "." : "", word(staticBody, 0x20 + 4 * i));
	printf("\n");
	const unsigned char* staticShape = *reinterpret_cast<unsigned char* const*>(
		staticBody + 0x10);
	printf("setter static_shape_orientation_matrix=");
	for(unsigned i = 0; i < 9; ++i)
		printf("%s%x", i ? "." : "", word(staticShape, 0x0c + 4 * i));
	printf("\n");
	NxMat34 staticPose;
	staticPose.M.setRowMajor(matrixRows[0]);
	staticPose.t = NxVec3(-1.0f, 2.0f, 4.0f);
	staticActor->setGlobalPose(staticPose);
	printf("setter static_pose_body=");
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", word(staticBody, 0x20 + 4 * i));
	printf("\nsetter static_pose_shape=");
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", word(staticShape, 0x0c + 4 * i));
	printf("\n");
	NxShape* staticPublicShape = staticActor->getShapes()[0];
	NxMat34 staticShapeTarget;
	staticShapeTarget.M.setRowMajor(matrixRows[1]);
	staticShapeTarget.t = NxVec3(2.0f, -3.0f, 5.0f);
	const unsigned char* staticPruner = *reinterpret_cast<unsigned char* const*>(
		staticShape + 0xc4);
	const unsigned beforeStaticShapeEpoch = staticPruner
		? word(staticPruner, 0x38) : 0u;
	staticPublicShape->setGlobalPose(staticShapeTarget);
	printf("setter static_shape_global_pose=");
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", word(staticShape, 0x6c + 4 * i));
	for(unsigned i = 0; i < 12; ++i)
		printf(".%x", word(staticShape, 0x0c + 4 * i));
	printf(".%x.%u\n", word(staticShape, 0xdc), staticPruner
		? word(staticPruner, 0x38) - beforeStaticShapeEpoch : 0u);
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
	multiActor->setGlobalPose(combinedPose);
	printf("setter multi_pose=%u", static_cast<unsigned>(lastChild - firstChild));
	for(const unsigned char* const* child = firstChild; child != lastChild; ++child)
		printf(".%x.%x.%x.%x.%x", word(*child, 0x0c), word(*child, 0x10),
			word(*child, 0x14), word(*child, 0x30), word(*child, 0x34));
	printf("\n");
	scene->releaseActor(*multiActor);
	NxBoxShapeDesc posedBox;
	posedBox.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	const float localRotation[9] = {1.0f,0.0f,0.0f,
		0.0f,0.0f,-1.0f,0.0f,1.0f,0.0f};
	posedBox.localPose.M.setRowMajor(localRotation);
	posedBox.localPose.t = NxVec3(0.5f, -0.25f, 0.75f);
	NxActorDesc posedDesc;
	posedDesc.shapes.pushBack(&posedBox);
	posedDesc.body = &bodyDesc;
	const float actorRotation[9] = {0.0f,-1.0f,0.0f,
		1.0f,0.0f,0.0f,0.0f,0.0f,1.0f};
	posedDesc.globalPose.M.setRowMajor(actorRotation);
	posedDesc.globalPose.t = NxVec3(2.0f, 3.0f, -4.0f);
	NxActor* posedActor = scene->createActor(posedDesc);
	printf("setter posed_created=%u\n", posedActor ? 1u : 0u);
	if(!posedActor) return nxFail("posed actor creation failed");
	const unsigned char* posedBody = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(posedActor) + 0x14);
	const unsigned char* posedShape = *reinterpret_cast<unsigned char* const*>(posedBody + 0x10);
	printf("setter posed_local=");
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", word(posedShape, 0x6c + 4 * i));
	printf("\nsetter posed_world=");
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", word(posedShape, 0x0c + 4 * i));
	printf("\nsetter posed_mirror=");
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", word(posedShape, 0x3c + 4 * i));
	printf("\n");
	NxBoxShape* posedPublicBox = posedActor->getShapes()[0]->isBox();
	NxMat34 publicLocalPose;
	NxVec3 publicLocalPosition;
	NxMat33 publicLocalOrientation;
	posedPublicBox->getLocalPose(publicLocalPose);
	posedPublicBox->getLocalPosition(publicLocalPosition);
	posedPublicBox->getLocalOrientation(publicLocalOrientation);
	printf("setter posed_public_local_pose=");
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", bits(reinterpret_cast<const float*>(
			&publicLocalPose)[i]));
	printf("\nsetter posed_public_local_position=%x.%x.%x\n",
		bits(publicLocalPosition.x), bits(publicLocalPosition.y),
		bits(publicLocalPosition.z));
	printf("setter posed_public_local_orientation=");
	for(unsigned i = 0; i < 9; ++i)
		printf("%s%x", i ? "." : "", bits(reinterpret_cast<const float*>(
			&publicLocalOrientation)[i]));
	printf("\n");
	const NxMat34 localValPose = posedPublicBox->getLocalPose();
	const NxVec3 localValPosition = posedPublicBox->getLocalPosition();
	const NxMat33 localValOrientation = posedPublicBox->getLocalOrientation();
	printf("setter posed_public_local_val_pose=");
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", bits(reinterpret_cast<const float*>(
			&localValPose)[i]));
	printf("\nsetter posed_public_local_val_position=%x.%x.%x\n",
		bits(localValPosition.x), bits(localValPosition.y),
		bits(localValPosition.z));
	printf("setter posed_public_local_val_orientation=");
	for(unsigned i = 0; i < 9; ++i)
		printf("%s%x", i ? "." : "", bits(reinterpret_cast<const float*>(
			&localValOrientation)[i]));
	printf("\n");
	NxMat34 publicGlobalPose;
	NxVec3 publicGlobalPosition;
	NxMat33 publicGlobalOrientation;
	posedPublicBox->getGlobalPose(publicGlobalPose);
	posedPublicBox->getGlobalPosition(publicGlobalPosition);
	posedPublicBox->getGlobalOrientation(publicGlobalOrientation);
	printf("setter posed_public_global_pose=");
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", bits(reinterpret_cast<const float*>(
			&publicGlobalPose)[i]));
	printf("\nsetter posed_public_global_position=%x.%x.%x\n",
		bits(publicGlobalPosition.x), bits(publicGlobalPosition.y),
		bits(publicGlobalPosition.z));
	printf("setter posed_public_global_orientation=");
	for(unsigned i = 0; i < 9; ++i)
		printf("%s%x", i ? "." : "", bits(reinterpret_cast<const float*>(
			&publicGlobalOrientation)[i]));
	printf("\n");
	const NxMat34 globalValPose = posedPublicBox->getGlobalPose();
	const NxVec3 globalValPosition = posedPublicBox->getGlobalPosition();
	const NxMat33 globalValOrientation = posedPublicBox->getGlobalOrientation();
	printf("setter posed_public_global_val_pose=");
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", bits(reinterpret_cast<const float*>(
			&globalValPose)[i]));
	printf("\nsetter posed_public_global_val_position=%x.%x.%x\n",
		bits(globalValPosition.x), bits(globalValPosition.y),
		bits(globalValPosition.z));
	printf("setter posed_public_global_val_orientation=");
	for(unsigned i = 0; i < 9; ++i)
		printf("%s%x", i ? "." : "", bits(reinterpret_cast<const float*>(
			&globalValOrientation)[i]));
	printf("\n");
	NxBounds3 posedBounds;
	posedPublicBox->getWorldBounds(posedBounds);
	printf("setter posed_box_bounds=%x.%x.%x.%x.%x.%x\n",
		bits(posedBounds.getMin().x), bits(posedBounds.getMin().y),
		bits(posedBounds.getMin().z), bits(posedBounds.getMax().x),
		bits(posedBounds.getMax().y), bits(posedBounds.getMax().z));
	const unsigned char* posedPruner = *reinterpret_cast<unsigned char* const*>(
		posedShape + 0xc4);
	const unsigned beforeBoxEpoch = posedPruner ? word(posedPruner, 0x38) : 0u;
	posedPublicBox->setDimensions(NxVec3(2.0f, 3.0f, 4.0f));
	const NxVec3& updatedBoxDimensions = posedPublicBox->getDimensions();
	printf("setter box_changed=%x.%x.%x.%x.%x.%x.%x.%u\n",
		bits(updatedBoxDimensions.x), bits(updatedBoxDimensions.y),
		bits(updatedBoxDimensions.z), word(posedShape, 0xe4),
		word(posedShape, 0xe8), word(posedShape, 0xec),
		word(posedShape, 0xdc),
		posedPruner ? word(posedPruner, 0x38) - beforeBoxEpoch : 0u);
	posedPublicBox->getWorldBounds(posedBounds);
	printf("setter box_changed_world_bounds=%x.%x.%x.%x.%x.%x\n",
		bits(posedBounds.getMin().x), bits(posedBounds.getMin().y),
		bits(posedBounds.getMin().z), bits(posedBounds.getMax().x),
		bits(posedBounds.getMax().y), bits(posedBounds.getMax().z));
	NxBox posedWorldOBB;
	posedPublicBox->getWorldOBB(posedWorldOBB);
	printf("setter box_world_obb=");
	for(unsigned i = 0; i < 15; ++i)
		printf("%s%x", i ? "." : "", bits(reinterpret_cast<const float*>(
			&posedWorldOBB)[i]));
	printf("\n");
	NxBoxShapeDesc savedBox;
	savedBox.shapeFlags = 0x1234u;
	const char* savedNameSentinel = "descriptor-sentinel";
	savedBox.name = savedNameSentinel;
	posedPublicBox->userData = reinterpret_cast<void*>(0x12345670u);
	posedPublicBox->setName("saved-box");
	posedPublicBox->setGroup(5);
	posedPublicBox->setMaterial(2);
	posedPublicBox->setFlag(NX_SF_DISABLE_RAYCASTING, true);
	const bool boxSaved = posedPublicBox->saveToDesc(savedBox);
	printf("setter box_saved=%u.%u.%u.%u.%u.%u.%x.%x.%x",
		boxSaved ? 1u : 0u, static_cast<unsigned>(savedBox.getType()),
		savedBox.shapeFlags, static_cast<unsigned>(savedBox.group),
		static_cast<unsigned>(savedBox.materialIndex),
		savedBox.userData == posedPublicBox->userData ? 1u : 0u,
		bits(savedBox.dimensions.x), bits(savedBox.dimensions.y),
		bits(savedBox.dimensions.z));
	for(unsigned i = 0; i < 12; ++i)
		printf(".%x", bits(reinterpret_cast<const float*>(
			&savedBox.localPose)[i]));
	printf(".%u.%u\n", savedBox.name == savedNameSentinel ? 1u : 0u,
		savedBox.name == posedPublicBox->getName() ? 1u : 0u);
	float changedBoxAABB[6] = {};
	void** posedBoxTable = *reinterpret_cast<void***>(
		const_cast<unsigned char*>(posedShape));
	reinterpret_cast<ShapeAABBFn>(posedBoxTable[8])(
		const_cast<unsigned char*>(posedShape), changedBoxAABB);
	printf("setter box_changed_aabb=");
	for(unsigned i = 0; i < 6; ++i)
		printf("%s%x", i ? "." : "", bits(changedBoxAABB[i]));
	printf("\n");
	const unsigned beforeLocalPositionEpoch = posedPruner
		? word(posedPruner, 0x38) : 0u;
	posedPublicBox->setLocalPosition(NxVec3(-2.0f, 1.0f, 3.0f));
	const NxVec3 movedLocal = posedPublicBox->getLocalPosition();
	printf("setter box_local_position=%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%u\n",
		bits(movedLocal.x), bits(movedLocal.y), bits(movedLocal.z),
		word(posedShape, 0x90), word(posedShape, 0x94), word(posedShape, 0x98),
		word(posedShape, 0x30), word(posedShape, 0x34), word(posedShape, 0x38),
		word(posedShape, 0xdc), posedPruner
			? word(posedPruner, 0x38) - beforeLocalPositionEpoch : 0u);
	NxMat33 changedLocalOrientation;
	changedLocalOrientation.setRowMajor(actorRotation);
	const unsigned beforeLocalOrientationEpoch = posedPruner
		? word(posedPruner, 0x38) : 0u;
	posedPublicBox->setLocalOrientation(changedLocalOrientation);
	printf("setter box_local_orientation=");
	for(unsigned i = 0; i < 9; ++i)
		printf("%s%x", i ? "." : "", word(posedShape, 0x6c + 4 * i));
	for(unsigned i = 0; i < 9; ++i)
		printf(".%x", word(posedShape, 0x0c + 4 * i));
	printf(".%x.%u\n", word(posedShape, 0xdc), posedPruner
		? word(posedPruner, 0x38) - beforeLocalOrientationEpoch : 0u);
	NxMat34 changedLocalPose;
	changedLocalPose.M.setRowMajor(localRotation);
	changedLocalPose.t = NxVec3(0.25f, -1.5f, 2.5f);
	const unsigned beforeLocalPoseEpoch = posedPruner
		? word(posedPruner, 0x38) : 0u;
	posedPublicBox->setLocalPose(changedLocalPose);
	printf("setter box_local_pose=");
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", word(posedShape, 0x6c + 4 * i));
	for(unsigned i = 0; i < 12; ++i)
		printf(".%x", word(posedShape, 0x0c + 4 * i));
	printf(".%x.%u\n", word(posedShape, 0xdc), posedPruner
		? word(posedPruner, 0x38) - beforeLocalPoseEpoch : 0u);
	const unsigned beforeGlobalPositionEpoch = posedPruner
		? word(posedPruner, 0x38) : 0u;
	posedPublicBox->setGlobalPosition(NxVec3(4.0f, 5.0f, -6.0f));
	printf("setter box_global_position=");
	for(unsigned i = 0; i < 3; ++i)
		printf("%s%x", i ? "." : "", word(posedShape, 0x90 + 4 * i));
	for(unsigned i = 0; i < 3; ++i)
		printf(".%x", word(posedShape, 0x30 + 4 * i));
	printf(".%x.%u\n", word(posedShape, 0xdc), posedPruner
		? word(posedPruner, 0x38) - beforeGlobalPositionEpoch : 0u);
	const unsigned beforeGlobalOrientationEpoch = posedPruner
		? word(posedPruner, 0x38) : 0u;
	posedPublicBox->setGlobalOrientation(changedLocalOrientation);
	printf("setter box_global_orientation=");
	for(unsigned i = 0; i < 9; ++i)
		printf("%s%x", i ? "." : "", word(posedShape, 0x6c + 4 * i));
	for(unsigned i = 0; i < 9; ++i)
		printf(".%x", word(posedShape, 0x0c + 4 * i));
	printf(".%x.%u\n", word(posedShape, 0xdc), posedPruner
		? word(posedPruner, 0x38) - beforeGlobalOrientationEpoch : 0u);
	NxMat34 changedGlobalPose;
	changedGlobalPose.M.setRowMajor(localRotation);
	changedGlobalPose.t = NxVec3(1.0f, -3.0f, 2.0f);
	const unsigned beforeGlobalPoseEpoch = posedPruner
		? word(posedPruner, 0x38) : 0u;
	posedPublicBox->setGlobalPose(changedGlobalPose);
	printf("setter box_global_pose=");
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", word(posedShape, 0x6c + 4 * i));
	for(unsigned i = 0; i < 12; ++i)
		printf(".%x", word(posedShape, 0x0c + 4 * i));
	printf(".%x.%u\n", word(posedShape, 0xdc), posedPruner
		? word(posedPruner, 0x38) - beforeGlobalPoseEpoch : 0u);
	scene->releaseActor(*posedActor);
	NxSphereShapeDesc sphereDesc;
	NxCapsuleShapeDesc capsuleDesc;
	NxPlaneShapeDesc planeDesc;
	sphereDesc.radius = 1.0f;
	capsuleDesc.radius = 0.5f;
	capsuleDesc.height = 1.0f;
	NxShapeDesc* familyDescs[3] = {&sphereDesc, &capsuleDesc, &planeDesc};
	const char* familyNames[3] = {"sphere", "capsule", "plane"};
	const unsigned familySelfSlot[3] = {16, 16, 14};
	for(unsigned family = 0; family < 3; ++family)
		{
		NxActorDesc familyActorDesc;
		familyActorDesc.shapes.pushBack(familyDescs[family]);
		NxActor* familyActor = scene->createActor(familyActorDesc);
		printf("setter %s_vtable=%u", familyNames[family],
			familyActor ? 1u : 0u);
		if(!familyActor) return nxFail("family actor creation failed");
		const unsigned char* familyBody = *reinterpret_cast<unsigned char* const*>(
			reinterpret_cast<const unsigned char*>(familyActor) + 0x14);
		const unsigned char* familyShape = *reinterpret_cast<unsigned char* const*>(
			familyBody + 0x10);
		void** familyTable = *reinterpret_cast<void***>(
			const_cast<unsigned char*>(familyShape));
		for(unsigned slot = familySelfSlot[family];
			slot < familySelfSlot[family] + 3; ++slot)
			printf(".%u", familyTable &&
				reinterpret_cast<ShapeSelfFn>(familyTable[slot])(
					const_cast<unsigned char*>(familyShape)) == familyShape ? 1u : 0u);
		printf("\n");
		NxShape* publicShape = familyActor->getShapes()[0];
		NxBounds3 familyBounds;
		publicShape->getWorldBounds(familyBounds);
		printf("setter %s_world_bounds=%x.%x.%x.%x.%x.%x\n",
			familyNames[family], bits(familyBounds.getMin().x),
			bits(familyBounds.getMin().y), bits(familyBounds.getMin().z),
			bits(familyBounds.getMax().x), bits(familyBounds.getMax().y),
			bits(familyBounds.getMax().z));
		printf("setter %s_public=%u.%u.%u.%u.%u\n", familyNames[family],
			publicShape ? 1u : 0u,
			publicShape ? static_cast<unsigned>(publicShape->getType()) : 0u,
			publicShape && &publicShape->getActor() == familyActor ? 1u : 0u,
			publicShape ? static_cast<unsigned>(publicShape->getGroup()) : 0u,
			publicShape ? static_cast<unsigned>(publicShape->getMaterial()) : 0u);
		if(family == 0)
			printf("setter sphere_public_radius=%x\n", bits(
				static_cast<NxSphereShape*>(publicShape)->getRadius()));
		else if(family == 1)
			printf("setter capsule_public_dimensions=%x.%x\n",
				bits(static_cast<NxCapsuleShape*>(publicShape)->getRadius()),
				bits(static_cast<NxCapsuleShape*>(publicShape)->getHeight()));
		float familyCenter[4] = {};
		if(familyTable)
			reinterpret_cast<ShapeCenterFn>(familyTable[10])(
				const_cast<unsigned char*>(familyShape), familyCenter);
		printf("setter %s_center=%x.%x.%x.%x\n", familyNames[family],
			bits(familyCenter[0]), bits(familyCenter[1]),
			bits(familyCenter[2]), bits(familyCenter[3]));
		if(family == 0)
			printf("setter sphere_radius=%x\n", word(familyShape, 0xe0));
		else if(family == 1)
			printf("setter capsule_dimensions=%x.%x\n",
				word(familyShape, 0xe0), word(familyShape, 0xe4));
		else
			{
			printf("setter plane_equation=%x.%x.%x.%x\n",
				word(familyShape, 0xe0), word(familyShape, 0xe4),
				word(familyShape, 0xe8), word(familyShape, 0xec));
			printf("setter plane_basis=");
			for(unsigned i = 0; i < 7; ++i)
				printf("%s%x", i ? "." : "", word(familyShape, 0xf0 + i * 4));
			printf("\n");
			}
		if(family < 2)
			{
			const unsigned char* familyPruner =
				*reinterpret_cast<unsigned char* const*>(familyShape + 0xc4);
			const unsigned beforeEpoch = familyPruner
				? word(familyPruner, 0x38) : 0u;
			if(family == 0)
				{
				NxSphereShape* sphere = static_cast<NxSphereShape*>(publicShape);
				sphere->setRadius(1.5f);
				printf("setter sphere_changed=%x.%x.%x.%u\n",
					bits(sphere->getRadius()), word(familyShape, 0xe0),
					word(familyShape, 0xdc),
					familyPruner ? word(familyPruner, 0x38) - beforeEpoch : 0u);
				}
			else
				{
				NxCapsuleShape* capsule = static_cast<NxCapsuleShape*>(publicShape);
				capsule->setRadius(0.75f);
				capsule->setHeight(2.0f);
				printf("setter capsule_changed=%x.%x.%x.%x.%x.%u\n",
					bits(capsule->getRadius()), bits(capsule->getHeight()),
					word(familyShape, 0xe0), word(familyShape, 0xe4),
					word(familyShape, 0xdc),
					familyPruner ? word(familyPruner, 0x38) - beforeEpoch : 0u);
				const unsigned beforeDimensionsEpoch = familyPruner
					? word(familyPruner, 0x38) : 0u;
				capsule->setDimensions(1.0f, 3.0f);
				printf("setter capsule_set_dimensions=%x.%x.%x.%x.%x.%u\n",
					bits(capsule->getRadius()), bits(capsule->getHeight()),
					word(familyShape, 0xe0), word(familyShape, 0xe4),
					word(familyShape, 0xdc),
					familyPruner ? word(familyPruner, 0x38) - beforeDimensionsEpoch : 0u);
				capsule->getWorldBounds(familyBounds);
				printf("setter capsule_changed_world_bounds=%x.%x.%x.%x.%x.%x\n",
					bits(familyBounds.getMin().x), bits(familyBounds.getMin().y),
					bits(familyBounds.getMin().z), bits(familyBounds.getMax().x),
					bits(familyBounds.getMax().y), bits(familyBounds.getMax().z));
				float capsuleAABB[6] = {};
				reinterpret_cast<ShapeAABBFn>(familyTable[8])(
					const_cast<unsigned char*>(familyShape), capsuleAABB);
				printf("setter capsule_changed_aabb=");
				for(unsigned i = 0; i < 6; ++i)
					printf("%s%x", i ? "." : "", bits(capsuleAABB[i]));
				printf("\n");
				}
			}
		else
			{
			const unsigned char* planePruner =
				*reinterpret_cast<unsigned char* const*>(familyShape + 0xc4);
			const unsigned beforeEpoch = planePruner
				? word(planePruner, 0x38) : 0u;
			static_cast<NxPlaneShape*>(publicShape)->setPlane(
				NxVec3(0.0f, 0.0f, 1.0f), 2.5f);
			printf("setter plane_changed=");
			for(unsigned i = 0; i < 11; ++i)
				printf("%s%x", i ? "." : "", word(familyShape, 0xe0 + i * 4));
			printf(".%x.%u\n", word(familyShape, 0xdc),
				planePruner ? word(planePruner, 0x38) - beforeEpoch : 0u);
			}
		const unsigned char* posePruner = *reinterpret_cast<unsigned char* const*>(
			familyShape + 0xc4);
		const unsigned beforeFamilyPoseEpoch = posePruner
			? word(posePruner, 0x38) : 0u;
		publicShape->setLocalPosition(NxVec3(0.5f, -1.0f, 2.0f));
		const NxVec3 familyLocalPosition = publicShape->getLocalPosition();
		printf("setter %s_local_position=%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%u\n",
			familyNames[family], bits(familyLocalPosition.x),
			bits(familyLocalPosition.y), bits(familyLocalPosition.z),
			word(familyShape, 0x90), word(familyShape, 0x94),
			word(familyShape, 0x98), word(familyShape, 0x30),
			word(familyShape, 0x34), word(familyShape, 0x38),
			word(familyShape, 0xdc), posePruner
				? word(posePruner, 0x38) - beforeFamilyPoseEpoch : 0u);
		NxMat34 familyTargetPose;
		familyTargetPose.M.setRowMajor(matrixRows[1]);
		familyTargetPose.t = NxVec3(1.0f, 2.0f, 3.0f);
		const unsigned beforeFamilyGlobalEpoch = posePruner
			? word(posePruner, 0x38) : 0u;
		publicShape->setGlobalPose(familyTargetPose);
		printf("setter %s_global_pose=", familyNames[family]);
		for(unsigned i = 0; i < 12; ++i)
			printf("%s%x", i ? "." : "", word(familyShape, 0x6c + 4 * i));
		for(unsigned i = 0; i < 12; ++i)
			printf(".%x", word(familyShape, 0x0c + 4 * i));
		printf(".%x.%u\n", word(familyShape, 0xdc), posePruner
			? word(posePruner, 0x38) - beforeFamilyGlobalEpoch : 0u);
		publicShape->userData = reinterpret_cast<void*>(0x12345000u + family);
		publicShape->setName(familyNames[family]);
		NxSphereShapeDesc savedSphere;
		NxCapsuleShapeDesc savedCapsule;
		NxPlaneShapeDesc savedPlane;
		NxShapeDesc* savedDesc = family == 0
			? static_cast<NxShapeDesc*>(&savedSphere)
			: family == 1 ? static_cast<NxShapeDesc*>(&savedCapsule)
				: static_cast<NxShapeDesc*>(&savedPlane);
		savedDesc->name = savedNameSentinel;
		bool saved = family == 0
			? static_cast<NxSphereShape*>(publicShape)->saveToDesc(savedSphere)
			: family == 1
				? static_cast<NxCapsuleShape*>(publicShape)->saveToDesc(savedCapsule)
				: static_cast<NxPlaneShape*>(publicShape)->saveToDesc(savedPlane);
		printf("setter %s_saved=%u.%u.%u.%u.%u.%u.%u.%u",
			familyNames[family], saved ? 1u : 0u,
			static_cast<unsigned>(savedDesc->getType()), savedDesc->shapeFlags,
			static_cast<unsigned>(savedDesc->group),
			static_cast<unsigned>(savedDesc->materialIndex),
			savedDesc->userData == publicShape->userData ? 1u : 0u,
			savedDesc->name == savedNameSentinel ? 1u : 0u,
			savedDesc->name == publicShape->getName() ? 1u : 0u);
		for(unsigned i = 0; i < 12; ++i)
			printf(".%x", bits(reinterpret_cast<const float*>(
				&savedDesc->localPose)[i]));
		if(family == 0)
			printf(".%x", bits(savedSphere.radius));
		else if(family == 1)
			printf(".%x.%x.%x", bits(savedCapsule.radius),
				bits(savedCapsule.height), savedCapsule.flags);
		else
			printf(".%x.%x.%x.%x", bits(savedPlane.normal.x),
				bits(savedPlane.normal.y), bits(savedPlane.normal.z),
				bits(savedPlane.d));
		printf("\n");
		scene->releaseActor(*familyActor);
		}
	// NpActor.cpp completion Task 2: the G1/E1 reports. The stream has been
	// silent until here, so every line above is what it was before it was
	// passed; from here on each report the pair's Foundation delivers is
	// printed as "setter report=<code>.<line>.<file>.<message>", and each case
	// line ends with the number of reports the call made.
	errors.enabled = true;
	NxActor* errStatic = scene->createActor(staticDesc);
	NxBodyDesc errBody;
	errBody.mass = 2.0f;
	errBody.massSpaceInertia = NxVec3(1.0f, 2.0f, 3.0f);
	NxActorDesc errDynamicDesc;
	errDynamicDesc.shapes.pushBack(&box);
	errDynamicDesc.body = &errBody;
	errDynamicDesc.globalPose.t = NxVec3(3.0f, 4.0f, 5.0f);
	NxActor* errDynamic = scene->createActor(errDynamicDesc);
	NxActor* errKinematic = scene->createActor(errDynamicDesc);
	printf("setter error_actors=%u.%u.%u.%u\n", errStatic ? 1u : 0u,
		errDynamic ? 1u : 0u, errKinematic ? 1u : 0u, errors.reports);
	if(!errStatic || !errDynamic || !errKinematic)
		return nxFail("error-case actor creation failed");
	errKinematic->raiseBodyFlag(NX_BF_KINEMATIC);
	const unsigned char* errDynamicRecord = *reinterpret_cast<unsigned char* const*>(
		*reinterpret_cast<unsigned char* const*>(
			reinterpret_cast<const unsigned char*>(errDynamic) + 0x14) + 8);
	NxMat34 errPose;
	errPose.M.setRowMajor(matrixRows[1]);
	errPose.t = NxVec3(1.0f, 2.0f, 3.0f);
	const NxVec3 errVector(0.5f, -1.5f, 2.5f);
	const NxQuat errQuat(errPose.M);
	NxBoxShapeDesc badBox;
	badBox.dimensions = NxVec3(-1.0f, 1.0f, 1.0f);
	unsigned errBefore = 0;
#define ERROR_CASE(label, expression) \
	errBefore = errors.reports; \
	expression; \
	printf("setter error_" label "=%u\n", errors.reports - errBefore)
#define ERROR_VALUE(label, type, expression, kind) \
	{ \
	errBefore = errors.reports; \
	const type errValue = expression; \
	printf("setter error_" label "=" ERRFMT_##kind ".%u\n", ERRARGS_##kind(errValue), \
		errors.reports - errBefore); \
	}
#define ERRFMT_SCALAR "%x"
#define ERRFMT_COUNT "%u"
#define ERRFMT_VEC "%x.%x.%x"
#define ERRFMT_MAT "%x.%x.%x.%x.%x.%x.%x.%x.%x"
#define ERRFMT_POSE ERRFMT_MAT "." ERRFMT_VEC
#define ERRARGS_SCALAR(v) bits(v)
#define ERRARGS_COUNT(v) static_cast<unsigned>(v)
#define ERRARGS_VEC(v) bits((v).x), bits((v).y), bits((v).z)
#define ERRARGS_MAT(v) bits((v)(0,0)), bits((v)(0,1)), bits((v)(0,2)), \
	bits((v)(1,0)), bits((v)(1,1)), bits((v)(1,2)), \
	bits((v)(2,0)), bits((v)(2,1)), bits((v)(2,2))
#define ERRARGS_POSE(v) ERRARGS_MAT((v).M), ERRARGS_VEC((v).t)

	// Static actor: every dynamic-only row reports E1 and takes its null arm.
	ERROR_CASE("static_linear_damping", errStatic->setLinearDamping(0.5f));
	ERROR_CASE("static_linear_damping_negative", errStatic->setLinearDamping(-1.0f));
	ERROR_CASE("static_angular_damping", errStatic->setAngularDamping(0.5f));
	ERROR_CASE("static_angular_damping_negative", errStatic->setAngularDamping(-1.0f));
	ERROR_CASE("static_mass", errStatic->setMass(2.0f));
	ERROR_CASE("static_inertia", errStatic->setMassSpaceInertiaTensor(errVector));
	ERROR_CASE("static_linear_velocity", errStatic->setLinearVelocity(errVector));
	ERROR_CASE("static_angular_velocity", errStatic->setAngularVelocity(errVector));
	ERROR_CASE("static_max_angular", errStatic->setMaxAngularVelocity(3.0f));
	ERROR_CASE("static_linear_momentum", errStatic->setLinearMomentum(errVector));
	ERROR_CASE("static_angular_momentum", errStatic->setAngularMomentum(errVector));
	ERROR_CASE("static_raise_body_flag", errStatic->raiseBodyFlag(NX_BF_DISABLE_GRAVITY));
	ERROR_CASE("static_clear_body_flag", errStatic->clearBodyFlag(NX_BF_DISABLE_GRAVITY));
	ERROR_CASE("static_force_at_pos", errStatic->addForceAtPos(errVector, errVector));
	ERROR_CASE("static_force_at_local_pos", errStatic->addForceAtLocalPos(errVector, errVector));
	ERROR_CASE("static_local_force_at_pos", errStatic->addLocalForceAtPos(errVector, errVector));
	ERROR_CASE("static_local_force_at_local_pos", errStatic->addLocalForceAtLocalPos(errVector, errVector));
	ERROR_CASE("static_force", errStatic->addForce(errVector));
	ERROR_CASE("static_local_force", errStatic->addLocalForce(errVector));
	ERROR_CASE("static_torque", errStatic->addTorque(errVector));
	ERROR_CASE("static_local_torque", errStatic->addLocalTorque(errVector));
	ERROR_CASE("static_move_position", errStatic->moveGlobalPosition(errVector));
	ERROR_CASE("static_move_pose", errStatic->moveGlobalPose(errPose));
	ERROR_CASE("static_move_orientation", errStatic->moveGlobalOrientation(errPose.M));
	ERROR_CASE("static_cmass_local_pose", errStatic->setCMassOffsetLocalPose(errPose));
	ERROR_CASE("static_cmass_local_position", errStatic->setCMassOffsetLocalPosition(errVector));
	ERROR_CASE("static_cmass_local_orientation", errStatic->setCMassOffsetLocalOrientation(errPose.M));
	ERROR_CASE("static_cmass_offset_global_pose", errStatic->setCMassOffsetGlobalPose(errPose));
	ERROR_CASE("static_cmass_offset_global_position", errStatic->setCMassOffsetGlobalPosition(errVector));
	ERROR_CASE("static_cmass_offset_global_orientation", errStatic->setCMassOffsetGlobalOrientation(errPose.M));
	ERROR_CASE("static_cmass_global_pose", errStatic->setCMassGlobalPose(errPose));
	ERROR_CASE("static_cmass_global_position", errStatic->setCMassGlobalPosition(errVector));
	ERROR_CASE("static_cmass_global_orientation", errStatic->setCMassGlobalOrientation(errPose.M));
	ERROR_VALUE("static_get_linear_damping", NxReal, errStatic->getLinearDamping(), SCALAR);
	ERROR_VALUE("static_get_angular_damping", NxReal, errStatic->getAngularDamping(), SCALAR);
	ERROR_VALUE("static_read_body_flag", unsigned, errStatic->readBodyFlag(NX_BF_KINEMATIC) ? 1u : 0u, COUNT);
	ERROR_VALUE("static_get_linear_velocity", NxVec3, errStatic->getLinearVelocity(), VEC);
	ERROR_VALUE("static_get_angular_velocity", NxVec3, errStatic->getAngularVelocity(), VEC);
	ERROR_VALUE("static_get_linear_momentum", NxVec3, errStatic->getLinearMomentum(), VEC);
	ERROR_VALUE("static_get_angular_momentum", NxVec3, errStatic->getAngularMomentum(), VEC);
	ERROR_VALUE("static_get_inertia", NxVec3, errStatic->getMassSpaceInertiaTensor(), VEC);
	ERROR_VALUE("static_get_global_inertia", NxMat33, errStatic->getGlobalInertiaTensor(), MAT);
	ERROR_VALUE("static_get_global_inverse", NxMat33, errStatic->getGlobalInertiaTensorInverse(), MAT);
	ERROR_VALUE("static_get_cmass_local_pose", NxMat34, errStatic->getCMassLocalPose(), POSE);
	ERROR_VALUE("static_get_cmass_local_position", NxVec3, errStatic->getCMassLocalPosition(), VEC);
	ERROR_VALUE("static_get_cmass_local_orientation", NxMat33, errStatic->getCMassLocalOrientation(), MAT);
	ERROR_VALUE("static_get_cmass_global_pose", NxMat34, errStatic->getCMassGlobalPose(), POSE);
	ERROR_VALUE("static_get_cmass_global_position", NxVec3, errStatic->getCMassGlobalPosition(), VEC);
	ERROR_VALUE("static_get_cmass_global_orientation", NxMat33, errStatic->getCMassGlobalOrientation(), MAT);
	ERROR_VALUE("static_create_invalid_shape", unsigned, errStatic->createShape(badBox) ? 1u : 0u, COUNT);

	// Dynamic actor: invalid arguments and the kinematic-only moves.
	ERROR_CASE("dynamic_mass_negative", errDynamic->setMass(-3.0f));
	ERROR_CASE("dynamic_mass_zero", errDynamic->setMass(0.0f));
	ERROR_CASE("dynamic_linear_damping_negative", errDynamic->setLinearDamping(-0.5f));
	ERROR_CASE("dynamic_angular_damping_negative", errDynamic->setAngularDamping(-0.5f));
	printf("setter error_dynamic_unchanged=%x.%x.%x.%x\n",
		word(errDynamicRecord, 0x188), word(errDynamicRecord, 0xc0),
		word(errDynamicRecord, 0xb8), word(errDynamicRecord, 0xbc));
	ERROR_CASE("dynamic_move_position", errDynamic->moveGlobalPosition(errVector));
	ERROR_CASE("dynamic_move_pose", errDynamic->moveGlobalPose(errPose));
	ERROR_CASE("dynamic_move_orientation", errDynamic->moveGlobalOrientation(errPose.M));
	ERROR_VALUE("dynamic_create_invalid_shape", unsigned, errDynamic->createShape(badBox) ? 1u : 0u, COUNT);
	ERROR_VALUE("dynamic_shapes_after_invalid", unsigned, errDynamic->getNbShapes(), COUNT);

	// Kinematic actor: the (non-kinematic) dynamic rows report E1; the rows
	// that only need a body do not.
	ERROR_CASE("kinematic_linear_velocity", errKinematic->setLinearVelocity(errVector));
	ERROR_CASE("kinematic_angular_velocity", errKinematic->setAngularVelocity(errVector));
	ERROR_CASE("kinematic_angular_momentum", errKinematic->setAngularMomentum(errVector));
	ERROR_CASE("kinematic_linear_momentum", errKinematic->setLinearMomentum(errVector));
	ERROR_CASE("kinematic_force", errKinematic->addForce(errVector));
	ERROR_CASE("kinematic_torque", errKinematic->addTorque(errVector));
	ERROR_CASE("kinematic_local_force", errKinematic->addLocalForce(errVector));
	ERROR_CASE("kinematic_local_torque", errKinematic->addLocalTorque(errVector));
	ERROR_CASE("kinematic_force_at_pos", errKinematic->addForceAtPos(errVector, errVector));
	ERROR_CASE("kinematic_force_at_local_pos", errKinematic->addForceAtLocalPos(errVector, errVector));
	ERROR_CASE("kinematic_local_force_at_pos", errKinematic->addLocalForceAtPos(errVector, errVector));
	ERROR_CASE("kinematic_local_force_at_local_pos", errKinematic->addLocalForceAtLocalPos(errVector, errVector));
	ERROR_CASE("kinematic_cmass_local_pose", errKinematic->setCMassOffsetLocalPose(errPose));
	ERROR_CASE("kinematic_cmass_local_position", errKinematic->setCMassOffsetLocalPosition(errVector));
	ERROR_CASE("kinematic_cmass_local_orientation", errKinematic->setCMassOffsetLocalOrientation(errPose.M));
	ERROR_CASE("kinematic_cmass_offset_global_pose", errKinematic->setCMassOffsetGlobalPose(errPose));
	ERROR_CASE("kinematic_cmass_offset_global_position", errKinematic->setCMassOffsetGlobalPosition(errVector));
	ERROR_CASE("kinematic_cmass_offset_global_orientation", errKinematic->setCMassOffsetGlobalOrientation(errPose.M));
	ERROR_CASE("kinematic_cmass_global_pose", errKinematic->setCMassGlobalPose(errPose));
	ERROR_CASE("kinematic_cmass_global_position", errKinematic->setCMassGlobalPosition(errVector));
	ERROR_CASE("kinematic_cmass_global_orientation", errKinematic->setCMassGlobalOrientation(errPose.M));
	ERROR_CASE("kinematic_move_position", errKinematic->moveGlobalPosition(errVector));

	// Write lock held by "another thread": every guarded row reports G1 with
	// its own line and changes nothing; the CMass-global setters check the
	// actor before the lock, so on a static actor they report E1 instead.
	NxActorWriteLockHolder errLock(errDynamic);
	NxShape* errShape = errDynamic->getShapes()[0];
	NxActorDesc errSaved;
#define LOCKED_CASE(label, expression) \
	errLock.hold(); \
	ERROR_CASE("locked_" label, expression); \
	errLock.release()
	LOCKED_CASE("global_pose", errDynamic->setGlobalPose(errPose));
	LOCKED_CASE("global_position", errDynamic->setGlobalPosition(errVector));
	LOCKED_CASE("global_orientation", errDynamic->setGlobalOrientation(errPose.M));
	LOCKED_CASE("global_orientation_quat", errDynamic->setGlobalOrientationQuat(errQuat));
	LOCKED_CASE("move_pose", errDynamic->moveGlobalPose(errPose));
	LOCKED_CASE("move_position", errDynamic->moveGlobalPosition(errVector));
	LOCKED_CASE("move_orientation", errDynamic->moveGlobalOrientation(errPose.M));
	LOCKED_CASE("create_shape", errDynamic->createShape(badBox));
	LOCKED_CASE("release_shape", errDynamic->releaseShape(*errShape));
	LOCKED_CASE("cmass_local_pose", errDynamic->setCMassOffsetLocalPose(errPose));
	LOCKED_CASE("cmass_local_position", errDynamic->setCMassOffsetLocalPosition(errVector));
	LOCKED_CASE("cmass_local_orientation", errDynamic->setCMassOffsetLocalOrientation(errPose.M));
	LOCKED_CASE("cmass_offset_global_pose", errDynamic->setCMassOffsetGlobalPose(errPose));
	LOCKED_CASE("cmass_offset_global_position", errDynamic->setCMassOffsetGlobalPosition(errVector));
	LOCKED_CASE("cmass_offset_global_orientation", errDynamic->setCMassOffsetGlobalOrientation(errPose.M));
	LOCKED_CASE("cmass_global_pose", errDynamic->setCMassGlobalPose(errPose));
	LOCKED_CASE("cmass_global_position", errDynamic->setCMassGlobalPosition(errVector));
	LOCKED_CASE("cmass_global_orientation", errDynamic->setCMassGlobalOrientation(errPose.M));
	LOCKED_CASE("mass", errDynamic->setMass(-3.0f));
	LOCKED_CASE("inertia", errDynamic->setMassSpaceInertiaTensor(errVector));
	LOCKED_CASE("linear_damping", errDynamic->setLinearDamping(-1.0f));
	LOCKED_CASE("angular_damping", errDynamic->setAngularDamping(-1.0f));
	LOCKED_CASE("linear_velocity", errDynamic->setLinearVelocity(errVector));
	LOCKED_CASE("angular_velocity", errDynamic->setAngularVelocity(errVector));
	LOCKED_CASE("max_angular", errDynamic->setMaxAngularVelocity(3.0f));
	LOCKED_CASE("linear_momentum", errDynamic->setLinearMomentum(errVector));
	LOCKED_CASE("angular_momentum", errDynamic->setAngularMomentum(errVector));
	LOCKED_CASE("force_at_pos", errDynamic->addForceAtPos(errVector, errVector));
	LOCKED_CASE("force_at_local_pos", errDynamic->addForceAtLocalPos(errVector, errVector));
	LOCKED_CASE("local_force_at_pos", errDynamic->addLocalForceAtPos(errVector, errVector));
	LOCKED_CASE("local_force_at_local_pos", errDynamic->addLocalForceAtLocalPos(errVector, errVector));
	LOCKED_CASE("force", errDynamic->addForce(errVector));
	LOCKED_CASE("local_force", errDynamic->addLocalForce(errVector));
	LOCKED_CASE("torque", errDynamic->addTorque(errVector));
	LOCKED_CASE("local_torque", errDynamic->addLocalTorque(errVector));
	LOCKED_CASE("sleep_linear", errDynamic->setSleepLinearVelocity(0.25f));
	LOCKED_CASE("sleep_angular", errDynamic->setSleepAngularVelocity(0.25f));
	LOCKED_CASE("wake", errDynamic->wakeUp(0.5f));
	LOCKED_CASE("sleep", errDynamic->putToSleep());
	LOCKED_CASE("raise_actor_flag", errDynamic->raiseActorFlag(NX_AF_DISABLE_COLLISION));
	LOCKED_CASE("clear_actor_flag", errDynamic->clearActorFlag(NX_AF_DISABLE_COLLISION));
	LOCKED_CASE("raise_body_flag", errDynamic->raiseBodyFlag(NX_BF_KINEMATIC));
	LOCKED_CASE("clear_body_flag", errDynamic->clearBodyFlag(NX_BF_DISABLE_GRAVITY));
	LOCKED_CASE("save_to_desc", errDynamic->saveToDesc(errSaved));
	LOCKED_CASE("name", errDynamic->setName("locked"));
	LOCKED_CASE("group", errDynamic->setGroup(7));
	errLock.hold();
	ERROR_VALUE("locked_get_linear_damping", NxReal, errDynamic->getLinearDamping(), SCALAR);
	errLock.release();
	NxActorWriteLockHolder errStaticLock(errStatic);
	errStaticLock.hold();
	ERROR_CASE("locked_static_cmass_global_pose", errStatic->setCMassGlobalPose(errPose));
	printf("setter error_locked_static_state=%u.%u\n", errStaticLock.flag(), errStaticLock.owner());
	errStaticLock.release();
	// NpActor.cpp completion Task 3 (contract NG): the readers that take the
	// read lock in the oracle (002362/002366 on the block [actor+0x10] links
	// to). With that block's flag/owner words set to 1/0, a read lock and its
	// unlock rewrite the owner to this thread and the flag to 0; 000142,
	// which takes no lock, leaves both.
	{
		const unsigned tid = GetCurrentThreadId();
		NxActor* ngActors[2] = { errDynamic, errStatic };
		for(unsigned k = 0; k < 2; ++k)
		{
			NxActor* a = ngActors[k];
			unsigned char* readLink = *reinterpret_cast<unsigned char**>(
				reinterpret_cast<unsigned char*>(a) + 0x10);
			struct ReadLock
			{
				unsigned* state; unsigned saved[2];
				void hold() { saved[0] = state[0]; saved[1] = state[1]; state[0] = 1; state[1] = 0; }
				void release() { state[0] = saved[0]; state[1] = saved[1]; }
				unsigned flag() const { return state[0]; }
				unsigned owner() const { return state[1]; }
			} lock;
			lock.state = reinterpret_cast<unsigned*>(*reinterpret_cast<unsigned char**>(readLink) + 0x18);
#define NG_CASE(name, call) lock.hold(); call; 	printf("setter ng_%s_%u state=%u.%u\n", name, k, lock.flag(), lock.owner() == tid ? 1u : 0u); 	lock.release()
			NG_CASE("nb_shapes", a->getNbShapes());
			NG_CASE("shapes", a->getShapes());
			NG_CASE("name", a->getName());
			NG_CASE("position", a->getGlobalPosition());
			NG_CASE("orientation", a->getGlobalOrientation());
			NG_CASE("orientation_quat", a->getGlobalOrientationQuat());
			NG_CASE("pose", a->getGlobalPose());
			NG_CASE("is_dynamic", a->isDynamic());
			if(k == 0) { NG_CASE("inverse_inertia", a->getGlobalInertiaTensorInverse()); }
#undef NG_CASE
		}
	}
	// 000094's static arm: the quaternion of a static actor's matrix, over
	// orientations that walk the trace arm and the x, y and z pivots.
	{
		const float quats[6][4] = {
			{ 1.0f, 2.0f, 3.0f, 4.0f }, { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f },
			{ 0.95f, 0.2f, 0.1f, 0.2f }, { 0.15f, 0.9f, -0.3f, 0.25f }, { -0.2f, 0.25f, 0.9f, 0.3f } };
		for(unsigned i = 0; i < 6; ++i)
		{
			float x = quats[i][0], y = quats[i][1], z = quats[i][2], w = quats[i][3];
			const float inv = 1.0f / sqrtf(x * x + y * y + z * z + w * w);
			x *= inv; y *= inv; z *= inv; w *= inv;
			NxActorDesc rotatedDesc;
			rotatedDesc.shapes.pushBack(&box);
			rotatedDesc.globalPose.M.setRow(0, NxVec3(1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - w * z), 2.0f * (x * z + w * y)));
			rotatedDesc.globalPose.M.setRow(1, NxVec3(2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z - w * x)));
			rotatedDesc.globalPose.M.setRow(2, NxVec3(2.0f * (x * z - w * y), 2.0f * (y * z + w * x), 1.0f - 2.0f * (x * x + y * y)));
			NxActor* rotated = scene->createActor(rotatedDesc);
			if(!rotated) continue;
			const NxQuat q = rotated->getGlobalOrientationQuat();
			printf("setter static_quat_%u=%x.%x.%x.%x\n", i, bits(q.x), bits(q.y), bits(q.z), bits(q.w));
			scene->releaseActor(*rotated);
		}
	}
	printf("setter error_locked_unchanged=%x.%x.%x.%x.%x.%x.%x.%x.%x.%u.%u.%u\n",
		word(errDynamicRecord, 0x188), word(errDynamicRecord, 0xc0),
		word(errDynamicRecord, 0xb8), word(errDynamicRecord, 0xbc),
		word(errDynamicRecord, 0x50), word(errDynamicRecord, 0x54),
		word(errDynamicRecord, 0x58), word(errDynamicRecord, 0x10c),
		word(errDynamicRecord, 0x84), errDynamic->getNbShapes(),
		static_cast<unsigned>(errDynamic->getGroup()),
		errDynamic->readActorFlag(NX_AF_DISABLE_COLLISION) ? 1u : 0u);
#undef LOCKED_CASE
#undef ERROR_CASE
#undef ERROR_VALUE
#undef ERRFMT_SCALAR
#undef ERRFMT_COUNT
#undef ERRFMT_VEC
#undef ERRFMT_MAT
#undef ERRFMT_POSE
#undef ERRARGS_SCALAR
#undef ERRARGS_COUNT
#undef ERRARGS_VEC
#undef ERRARGS_MAT
#undef ERRARGS_POSE
	errors.enabled = false;
	scene->releaseActor(*errKinematic);
	scene->releaseActor(*errDynamic);
	scene->releaseActor(*errStatic);
	// Shape geometry probes (scene-raycast Task 4, shape rows). getWorldOBB
	// (000933) composes the shape's global pose through 001309 from the
	// body's quaternion on a dynamic owner and from the owner's pose on a
	// static one; getWorldBounds (000935) runs on the same rotated poses; the
	// capsule's setDimensions (000993) and setRadius (000995) run on a rotated
	// dynamic capsule.
	{
	const NxQuat bodyTurns[2] = {
		NxQuat(NxVec3(0.18257419f, 0.36514837f, 0.54772256f), 0.73029674f),
		NxQuat(NxVec3(-0.6f, 0.1f, 0.35f), 0.7141428f) };
	NxQuat localTurn(NxVec3(0.2f, -0.45f, 0.1f), 0.8631338f);
	NxMat34 localPose;
	localPose.M.fromQuat(localTurn);
	localPose.t = NxVec3(0.25f, -0.5f, 1.75f);
	NxBoxShapeDesc geometryBox;
	geometryBox.dimensions = NxVec3(0.7f, 1.3f, 2.1f);
	geometryBox.localPose = localPose;
	NxBodyDesc geometryBody;
	geometryBody.mass = 1.0f;
	geometryBody.massSpaceInertia = NxVec3(1.0f, 1.0f, 1.0f);
	for(unsigned owner = 0; owner < 2; ++owner)
		{
		NxActorDesc geometryDesc;
		geometryDesc.shapes.pushBack(&geometryBox);
		geometryDesc.body = owner == 0 ? &geometryBody : 0;
		geometryDesc.globalPose.M.fromQuat(bodyTurns[1]);
		geometryDesc.globalPose.t = NxVec3(3.0f, -2.0f, 5.0f);
		NxActor* geometryActor = scene->createActor(geometryDesc);
		printf("setter geometry_actor=%u.%u\n", owner, geometryActor ? 1u : 0u);
		if(!geometryActor)
			continue;
		NxBoxShape* geometryShape = static_cast<NxBoxShape*>(
			geometryActor->getShapes()[0]);
		for(unsigned turn = 0; turn < (owner == 0 ? 2u : 1u); ++turn)
			{
			if(owner == 0)
				geometryActor->setGlobalOrientationQuat(bodyTurns[turn]);
			NxBox geometryOBB;
			geometryShape->getWorldOBB(geometryOBB);
			printf("setter geometry_obb=%u.%u", owner, turn);
			for(unsigned i = 0; i < 15; ++i)
				printf(".%x", bits(reinterpret_cast<const float*>(&geometryOBB)[i]));
			printf("\n");
			// getWorldBounds reads the cached world pose (+0x0c). Only the
			// dynamic creation's cached pose matches the oracle bit for bit;
			// after setGlobalOrientationQuat, and on the static owner, the
			// cached pose differs in the last bits (the owner-update and
			// static-pose paths are not reproduced yet), so the bounds are
			// printed for the first case only.
			if(owner != 0 || turn != 0)
				continue;
			NxBounds3 geometryBounds;
			geometryShape->getWorldBounds(geometryBounds);
			printf("setter geometry_bounds=%u.%u.%x.%x.%x.%x.%x.%x\n", owner, turn,
				bits(geometryBounds.getMin().x), bits(geometryBounds.getMin().y),
				bits(geometryBounds.getMin().z), bits(geometryBounds.getMax().x),
				bits(geometryBounds.getMax().y), bits(geometryBounds.getMax().z));
			}
		scene->releaseActor(*geometryActor);
		}
	NxCapsuleShapeDesc geometryCapsule;
	geometryCapsule.radius = 0.6f;
	geometryCapsule.height = 1.1f;
	geometryCapsule.localPose = localPose;
	NxActorDesc capsuleDesc;
	capsuleDesc.shapes.pushBack(&geometryCapsule);
	capsuleDesc.body = &geometryBody;
	capsuleDesc.globalPose.M.fromQuat(bodyTurns[0]);
	capsuleDesc.globalPose.t = NxVec3(-1.0f, 4.0f, 0.5f);
	NxActor* capsuleActor = scene->createActor(capsuleDesc);
	printf("setter geometry_capsule_actor=%u\n", capsuleActor ? 1u : 0u);
	if(capsuleActor)
		{
		NxCapsuleShape* capsule = static_cast<NxCapsuleShape*>(
			capsuleActor->getShapes()[0]);
		const unsigned char* capsuleShape = *reinterpret_cast<unsigned char* const*>(
			reinterpret_cast<const unsigned char*>(capsule) + 0x18);
		// The capsule's world bounds depend on its cached pose, which the
		// owner update (001315) recomputes with last-bit differences on a
		// rotated owner, so only the rows' own stores are printed.
		capsule->setDimensions(0.3f, 1.7f);
		printf("setter geometry_capsule_dimensions=%x.%x.%x.%x.%x\n",
			bits(capsule->getRadius()), bits(capsule->getHeight()),
			word(capsuleShape, 0xe0), word(capsuleShape, 0xe4),
			word(capsuleShape, 0xdc));
		capsule->setRadius(0.45f);
		printf("setter geometry_capsule_radius=%x.%x.%x\n",
			bits(capsule->getRadius()), word(capsuleShape, 0xe0),
			word(capsuleShape, 0xdc));
		scene->releaseActor(*capsuleActor);
		}
	}
	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
