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

static unsigned word(const unsigned char* bytes, unsigned offset)
{
	unsigned result;
	memcpy(&result, bytes + offset, sizeof(result));
	return result;
}

static void printState(unsigned mode, const char* stage,
	const unsigned char* record, unsigned dirty, unsigned activeCount)
{
	printf("force mode=%u stage=%s dirty=%x.%u linear=%x.%x.%x angular=%x.%x.%x force=%x.%x.%x torque=%x.%x.%x smooth_force=%x.%x.%x smooth_torque=%x.%x.%x\n",
		mode, stage, dirty, activeCount,
		word(record, 0x6c), word(record, 0x70), word(record, 0x74),
		word(record, 0x78), word(record, 0x7c), word(record, 0x80),
		word(record, 0x88), word(record, 0x8c), word(record, 0x90),
		word(record, 0x94), word(record, 0x98), word(record, 0x9c),
		word(record, 0xa0), word(record, 0xa4), word(record, 0xa8),
		word(record, 0xac), word(record, 0xb0), word(record, 0xb4));
}

static void printAtPosState(const char* stage, const unsigned char* record)
{
	printf("force atpos %s linear=%x.%x.%x angular=%x.%x.%x force=%x.%x.%x torque=%x.%x.%x smooth_force=%x.%x.%x smooth_torque=%x.%x.%x\n",
		stage,
		word(record, 0x6c), word(record, 0x70), word(record, 0x74),
		word(record, 0x78), word(record, 0x7c), word(record, 0x80),
		word(record, 0x88), word(record, 0x8c), word(record, 0x90),
		word(record, 0x94), word(record, 0x98), word(record, 0x9c),
		word(record, 0xa0), word(record, 0xa4), word(record, 0xa8),
		word(record, 0xac), word(record, 0xb0), word(record, 0xb4));
}

int wmain(int argc, wchar_t** argv)
{
	setvbuf(stdout, 0, _IONBF, 0);
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsActorForceTests",
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
	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&box);
	actorDesc.body = &bodyDesc;
	const NxVec3 force(10.0f, 18.0f, 28.0f);
	const NxVec3 torque(8.0f, 15.0f, 24.0f);
	for(unsigned mode = 0; mode < 5; ++mode)
		{
		NxActor* actor = scene->createActor(actorDesc);
		printf("force mode=%u created=%u\n", mode, actor ? 1u : 0u);
		if(!actor) return nxFail("actor creation failed");
		unsigned char* body = *reinterpret_cast<unsigned char**>(
			reinterpret_cast<unsigned char*>(actor) + 0x14);
		unsigned char* record = *reinterpret_cast<unsigned char**>(body + 8);
		unsigned char* aux = *reinterpret_cast<unsigned char**>(record + 0x120);
		const unsigned id = word(record, 0x11c);
		unsigned* flags = *reinterpret_cast<unsigned**>(aux + 0x40);
		unsigned* index = *reinterpret_cast<unsigned**>(aux + 0x60);
		unsigned** activeEnd = reinterpret_cast<unsigned**>(aux + 0x54);
		unsigned* activeBegin = *reinterpret_cast<unsigned**>(aux + 0x50);
		unsigned* savedEnd = *activeEnd;
		const unsigned savedIndex = index[id];
		flags[id] = 0;
		actor->addForce(force, static_cast<NxForceMode>(mode));
		printState(mode, "after_force", record, flags[id],
			static_cast<unsigned>(*activeEnd - activeBegin));
		flags[id] = 0xffffffffu;
		*activeEnd = savedEnd;
		index[id] = savedIndex;
		flags[id] = 0;
		actor->addTorque(torque, static_cast<NxForceMode>(mode));
		printState(mode, "after_torque", record, flags[id],
			static_cast<unsigned>(*activeEnd - activeBegin));
		flags[id] = 0xffffffffu;
		*activeEnd = savedEnd;
		index[id] = savedIndex;
		if(mode == 0)
			{
			flags[id] = 0;
			actor->addForce(force, NX_FORCE);
			actor->addTorque(torque, NX_FORCE);
			printState(mode, "repeated", record, flags[id],
				static_cast<unsigned>(*activeEnd - activeBegin));
			flags[id] = 0xffffffffu;
			*activeEnd = savedEnd;
			index[id] = savedIndex;
			}
		scene->releaseActor(*actor);
		}
	NxActor* sleepy = scene->createActor(actorDesc);
	printf("force sleepy_created=%u\n", sleepy ? 1u : 0u);
	if(!sleepy) return nxFail("sleepy actor creation failed");
	unsigned char* sleepyBody = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(sleepy) + 0x14);
	unsigned char* sleepyRecord = *reinterpret_cast<unsigned char**>(sleepyBody + 8);
	unsigned char* sleepyAux = *reinterpret_cast<unsigned char**>(sleepyRecord + 0x120);
	const unsigned sleepyId = word(sleepyRecord, 0x11c);
	unsigned* sleepyFlags = *reinterpret_cast<unsigned**>(sleepyAux + 0x40);
	unsigned* sleepyIndex = *reinterpret_cast<unsigned**>(sleepyAux + 0x60);
	unsigned** sleepyEnd = reinterpret_cast<unsigned**>(sleepyAux + 0x54);
	unsigned* sleepyBegin = *reinterpret_cast<unsigned**>(sleepyAux + 0x50);
	unsigned* sleepySavedEnd = *sleepyEnd;
	const unsigned sleepySavedIndex = sleepyIndex[sleepyId];
	sleepy->wakeUp(0.1f);
	printf("force low_wake_initial=%x.%x.%x\n",
		word(sleepyRecord, 0x84), word(sleepyRecord, 0x4c),
		word(sleepyRecord, 0x114));
	sleepyFlags[sleepyId] = 0;
	sleepy->addForce(force, NX_FORCE);
	printf("force low_wake_after=%x.%x.%x.%x.%u\n",
		word(sleepyRecord, 0x84), word(sleepyRecord, 0x4c),
		word(sleepyRecord, 0x114), sleepyFlags[sleepyId],
		static_cast<unsigned>(*sleepyEnd - sleepyBegin));
	sleepyFlags[sleepyId] = 0xffffffffu;
	*sleepyEnd = sleepySavedEnd;
	sleepyIndex[sleepyId] = sleepySavedIndex;
	sleepy->putToSleep();
	sleepyFlags[sleepyId] = 0;
	sleepy->addTorque(torque, NX_FORCE);
	printf("force forced_sleep_after=%x.%x.%x.%x.%u\n",
		word(sleepyRecord, 0x84), word(sleepyRecord, 0x4c),
		word(sleepyRecord, 0x114), sleepyFlags[sleepyId],
		static_cast<unsigned>(*sleepyEnd - sleepyBegin));
	sleepyFlags[sleepyId] = 0xffffffffu;
	*sleepyEnd = sleepySavedEnd;
	sleepyIndex[sleepyId] = sleepySavedIndex;
	scene->releaseActor(*sleepy);
	actorDesc.globalPose.M.setRow(0, NxVec3(0.0f, -1.0f, 0.0f));
	actorDesc.globalPose.M.setRow(1, NxVec3(1.0f, 0.0f, 0.0f));
	actorDesc.globalPose.M.setRow(2, NxVec3(0.0f, 0.0f, 1.0f));
	NxActor* rotated = scene->createActor(actorDesc);
	printf("force rotated_created=%u\n", rotated ? 1u : 0u);
	if(!rotated) return nxFail("rotated actor creation failed");
	unsigned char* rotatedBody = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(rotated) + 0x14);
	unsigned char* rotatedRecord = *reinterpret_cast<unsigned char**>(rotatedBody + 8);
	printf("force rotated_inverse=%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
		word(rotatedRecord, 0x164), word(rotatedRecord, 0x168), word(rotatedRecord, 0x16c),
		word(rotatedRecord, 0x170), word(rotatedRecord, 0x174), word(rotatedRecord, 0x178),
		word(rotatedRecord, 0x17c), word(rotatedRecord, 0x180), word(rotatedRecord, 0x184));
	rotated->addTorque(torque, NX_FORCE);
	printf("force rotated_accel=%x.%x.%x\n",
		word(rotatedRecord, 0x94), word(rotatedRecord, 0x98), word(rotatedRecord, 0x9c));
	rotated->addTorque(torque, NX_IMPULSE);
	printf("force rotated_velocity=%x.%x.%x\n",
		word(rotatedRecord, 0x78), word(rotatedRecord, 0x7c), word(rotatedRecord, 0x80));
	scene->releaseActor(*rotated);
	NxActor* local = scene->createActor(actorDesc);
	printf("force local_created=%u\n", local ? 1u : 0u);
	if(!local) return nxFail("local actor creation failed");
	unsigned char* localBody = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(local) + 0x14);
	unsigned char* localRecord = *reinterpret_cast<unsigned char**>(localBody + 8);
	local->addLocalForce(force, NX_FORCE);
	local->addLocalTorque(torque, NX_FORCE);
	printf("force local_accel=%x.%x.%x.%x.%x.%x\n",
		word(localRecord, 0x88), word(localRecord, 0x8c), word(localRecord, 0x90),
		word(localRecord, 0x94), word(localRecord, 0x98), word(localRecord, 0x9c));
	local->addLocalForce(force, NX_IMPULSE);
	local->addLocalTorque(torque, NX_IMPULSE);
	printf("force local_velocity=%x.%x.%x.%x.%x.%x\n",
		word(localRecord, 0x6c), word(localRecord, 0x70), word(localRecord, 0x74),
		word(localRecord, 0x78), word(localRecord, 0x7c), word(localRecord, 0x80));
	scene->releaseActor(*local);
	actorDesc.globalPose.M.id();
	actorDesc.globalPose.t = NxVec3(4.0f, 5.0f, 6.0f);
	NxActor* atpos = scene->createActor(actorDesc);
	printf("force atpos_created=%u\n", atpos ? 1u : 0u);
	if(!atpos) return nxFail("at-position actor creation failed");
	unsigned char* atposBody = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(atpos) + 0x14);
	unsigned char* atposRecord = *reinterpret_cast<unsigned char**>(atposBody + 8);
	printf("force atpos_cmass=%x.%x.%x\n", word(atposRecord, 0x158),
		word(atposRecord, 0x15c), word(atposRecord, 0x160));
	const NxVec3 worldPoint(5.0f, 7.0f, 9.0f);
	const NxVec3 localPoint(1.0f, 2.0f, 3.0f);
	atpos->addForceAtPos(force, worldPoint, NX_FORCE);
	printAtPosState("global_force", atposRecord);
	atpos->addForceAtLocalPos(force, localPoint, NX_IMPULSE);
	printAtPosState("local_position", atposRecord);
	atpos->addLocalForceAtPos(force, worldPoint, NX_SMOOTH_IMPULSE);
	printAtPosState("local_force", atposRecord);
	atpos->addLocalForceAtLocalPos(force, localPoint, NX_VELOCITY_CHANGE);
	printAtPosState("both_local", atposRecord);
	scene->releaseActor(*atpos);
	actorDesc.globalPose.M.setRow(0, NxVec3(0.0f, -1.0f, 0.0f));
	actorDesc.globalPose.M.setRow(1, NxVec3(1.0f, 0.0f, 0.0f));
	actorDesc.globalPose.M.setRow(2, NxVec3(0.0f, 0.0f, 1.0f));
	bodyDesc.massLocalPose.t = NxVec3(1.0f, 2.0f, 3.0f);
	NxActor* offset = scene->createActor(actorDesc);
	printf("force offset_created=%u\n", offset ? 1u : 0u);
	if(!offset) return nxFail("offset actor creation failed");
	unsigned char* offsetBody = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(offset) + 0x14);
	unsigned char* offsetRecord = *reinterpret_cast<unsigned char**>(offsetBody + 8);
	printf("force offset_cmass=%x.%x.%x\n", word(offsetRecord, 0x158),
		word(offsetRecord, 0x15c), word(offsetRecord, 0x160));
	offset->addForceAtLocalPos(force, localPoint, NX_FORCE);
	printAtPosState("offset_local_position", offsetRecord);
	offset->addLocalForceAtLocalPos(force, NxVec3(2.0f, 3.0f, 4.0f), NX_IMPULSE);
	printAtPosState("offset_both_local", offsetRecord);
	scene->releaseActor(*offset);
	bodyDesc.massLocalPose.t = NxVec3(0.0f, 0.0f, 0.0f);
	NxActor* kinematic = scene->createActor(actorDesc);
	printf("force kinematic_created=%u\n", kinematic ? 1u : 0u);
	if(!kinematic) return nxFail("kinematic actor creation failed");
	unsigned char* kinematicBody = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(kinematic) + 0x14);
	unsigned char* kinematicRecord = *reinterpret_cast<unsigned char**>(kinematicBody + 8);
	kinematic->raiseBodyFlag(NX_BF_KINEMATIC);
	const unsigned beforeKinematicAlloc = allocator.allocations();
	const unsigned beforeKinematicFree = allocator.frees();
	for(unsigned mode = 0; mode < 5; ++mode)
		{
		kinematic->addForce(force, static_cast<NxForceMode>(mode));
		kinematic->addTorque(torque, static_cast<NxForceMode>(mode));
		}
	printf("force kinematic_unchanged=%x.%x.%x.%x.%x.%x.%x.%x\n",
		word(kinematicRecord, 0x6c), word(kinematicRecord, 0x78),
		word(kinematicRecord, 0x88), word(kinematicRecord, 0x94),
		word(kinematicRecord, 0xa0), word(kinematicRecord, 0xac),
		allocator.allocations() - beforeKinematicAlloc,
		allocator.frees() - beforeKinematicFree);
	kinematic->clearBodyFlag(NX_BF_KINEMATIC);
	scene->releaseActor(*kinematic);
	// NpActor.cpp completion Task 2 (000150's x87 row sums): a body at an
	// irregular orientation with zero velocity takes local velocity changes,
	// so the record's velocity is exactly 000150's rotated vector, rounded
	// once at its store after the (R_i1 y + R_i2 z) + R_i0 x register sums.
	actorDesc.globalPose.M.id();
	actorDesc.globalPose.t = NxVec3(0.0f, 0.0f, 0.0f);
	NxActor* turned = scene->createActor(actorDesc);
	printf("force x87_created=%u\n", turned ? 1u : 0u);
	if(!turned) return nxFail("x87 actor creation failed");
	unsigned char* turnedRecord = *reinterpret_cast<unsigned char**>(
		*reinterpret_cast<unsigned char**>(
			reinterpret_cast<unsigned char*>(turned) + 0x14) + 8);
	NxQuat turn;
	turn.setXYZW(0.3137f, -0.5171f, 0.7043f, 0.3719f);
	turn.normalize();
	turned->setGlobalOrientationQuat(turn);
	printf("force x87_quat=%x.%x.%x.%x\n", word(turnedRecord, 0x5c),
		word(turnedRecord, 0x60), word(turnedRecord, 0x64), word(turnedRecord, 0x68));
	const float x87Inputs[6][3] = {
		{ 1.1f, -2.3f, 3.7f },
		{ -0.013f, 7.77f, 0.5003f },
		{ 1234.567f, -0.0021f, 89.1f },
		{ 3.3333333f, 3.3333333f, -3.3333333f },
		{ -17.25f, 0.071f, 1.0e-3f },
		{ 0.1f, 0.2f, 0.3f } };
	for(unsigned i = 0; i < 6; ++i)
		{
		const NxVec3 input(x87Inputs[i][0], x87Inputs[i][1], x87Inputs[i][2]);
		turned->setLinearVelocity(NxVec3(0.0f, 0.0f, 0.0f));
		turned->setAngularVelocity(NxVec3(0.0f, 0.0f, 0.0f));
		turned->addLocalForce(input, NX_VELOCITY_CHANGE);
		turned->addLocalTorque(input, NX_VELOCITY_CHANGE);
		printf("force x87_rotate_%u=%x.%x.%x.%x.%x.%x\n", i,
			word(turnedRecord, 0x6c), word(turnedRecord, 0x70), word(turnedRecord, 0x74),
			word(turnedRecord, 0x78), word(turnedRecord, 0x7c), word(turnedRecord, 0x80));
		}
	scene->releaseActor(*turned);
	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
