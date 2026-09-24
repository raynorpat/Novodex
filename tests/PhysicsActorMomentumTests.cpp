#include "PhysicsPairLoader.h"
#include "NxPageGuardedAllocator.h"

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxActor.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxMat33.h"

#include <stdio.h>
#include <string.h>

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(
	NxU32, NxUserAllocator*, NxUserOutputStream*);

static unsigned bits(NxReal value)
{
	unsigned result;
	memcpy(&result, &value, sizeof(result));
	return result;
}

static unsigned word(const unsigned char* bytes, unsigned offset)
{
	unsigned result;
	memcpy(&result, bytes + offset, sizeof(result));
	return result;
}

static void printVector(const char* name, const NxVec3& value)
{
	printf("momentum %s=%x.%x.%x\n", name,
		bits(value.x), bits(value.y), bits(value.z));
}

static void printMatrix(const char* name, const NxMat33& matrix)
{
	float values[9];
	matrix.getRowMajor(values);
	printf("momentum %s=%x.%x.%x.%x.%x.%x.%x.%x.%x\n", name,
		bits(values[0]), bits(values[1]), bits(values[2]),
		bits(values[3]), bits(values[4]), bits(values[5]),
		bits(values[6]), bits(values[7]), bits(values[8]));
}

int wmain(int argc, wchar_t** argv)
{
	setvbuf(stdout, 0, _IONBF, 0);
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsActorMomentumTests",
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
	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&box);
	actorDesc.body = &bodyDesc;
	NxActor* actor = scene->createActor(actorDesc);
	printf("momentum created=%u\n", actor ? 1u : 0u);
	if(!actor) return nxFail("actor creation failed");
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(actor) + 0x14);
	unsigned char* record = *reinterpret_cast<unsigned char**>(body + 8);
	printf("momentum inverse_tensor=%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
		word(record, 0x164), word(record, 0x168), word(record, 0x16c),
		word(record, 0x170), word(record, 0x174), word(record, 0x178),
		word(record, 0x17c), word(record, 0x180), word(record, 0x184));
	printf("momentum rotation_matrix=%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
		word(record, 0x134), word(record, 0x138), word(record, 0x13c),
		word(record, 0x140), word(record, 0x144), word(record, 0x148),
		word(record, 0x14c), word(record, 0x150), word(record, 0x154));
	printf("momentum inertia_frame=%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
		word(record, 0xdc), word(record, 0xe0), word(record, 0xe4),
		word(record, 0xe8), word(record, 0xec), word(record, 0xf0),
		word(record, 0xf4), word(record, 0xf8), word(record, 0xfc));
	printf("momentum initial_limit=%x\n", word(record, 0xd8));
	printVector("linear_initial", actor->getLinearMomentumVal());
	printVector("angular_initial", actor->getAngularMomentumVal());
	printMatrix("global_inertia_initial", actor->getGlobalInertiaTensorVal());
	printMatrix("global_inverse_initial", actor->getGlobalInertiaTensorInverseVal());
	printf("momentum energy_initial=%x\n", bits(actor->computeKineticEnergy()));
	const unsigned beforeAlloc = allocator.allocations();
	const unsigned beforeFree = allocator.frees();
	actor->setMaxAngularVelocity(9.0f);
	printf("momentum max_angular=%x\n", word(record, 0xd8));
	actor->setLinearMomentum(NxVec3(10.0f, 15.0f, 20.0f));
	printVector("linear_set", actor->getLinearMomentumVal());
	printVector("linear_velocity", actor->getLinearVelocityVal());
	printf("momentum linear_record=%x.%x.%x.%x.%x.%x\n",
		word(record, 0x6c), word(record, 0x70), word(record, 0x74),
		word(record, 0x34), word(record, 0x38), word(record, 0x3c));
	actor->setAngularMomentum(NxVec3(10.0f, 18.0f, 28.0f));
	printVector("angular_set", actor->getAngularMomentumVal());
	printVector("angular_velocity", actor->getAngularVelocityVal());
	printf("momentum angular_record=%x.%x.%x.%x.%x.%x\n",
		word(record, 0x78), word(record, 0x7c), word(record, 0x80),
		word(record, 0x40), word(record, 0x44), word(record, 0x48));
	printf("momentum energy_set=%x\n", bits(actor->computeKineticEnergy()));
	printf("momentum mutation_allocs=%u.%u\n",
		allocator.allocations() - beforeAlloc,
		allocator.frees() - beforeFree);
	unsigned char* aux = *reinterpret_cast<unsigned char**>(record + 0x120);
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
	printf("momentum dirty_" label "=%x.%u.%u\n", managerFlags[id], \
		static_cast<unsigned>(*activeEnd - activeBegin), managerIndex[id]); \
	managerFlags[id] = 0xffffffffu; \
	*activeEnd = savedEnd; \
	managerIndex[id] = savedIndex
	PROBE_DIRTY("limit", actor->setMaxAngularVelocity(10.0f));
	PROBE_DIRTY("linear", actor->setLinearMomentum(NxVec3(15.0f, 20.0f, 25.0f)));
	PROBE_DIRTY("angular", actor->setAngularMomentum(NxVec3(12.0f, 21.0f, 32.0f)));
#undef PROBE_DIRTY
	actor->raiseBodyFlag(NX_BF_KINEMATIC);
	const unsigned angularX = word(record, 0x78);
	const unsigned angularY = word(record, 0x7c);
	const unsigned angularZ = word(record, 0x80);
	actor->setAngularMomentum(NxVec3(100.0f, 200.0f, 300.0f));
	printf("momentum kinematic_angular=%x.%x.%x.%x.%x.%x\n",
		angularX, angularY, angularZ,
		word(record, 0x78), word(record, 0x7c), word(record, 0x80));
	actor->setLinearMomentum(NxVec3(100.0f, 200.0f, 300.0f));
	printf("momentum kinematic_linear=%x.%x.%x\n",
		word(record, 0x6c), word(record, 0x70), word(record, 0x74));
	actor->clearBodyFlag(NX_BF_KINEMATIC);
	scene->releaseActor(*actor);
	actorDesc.globalPose.M.setRow(0, NxVec3(0.0f, -1.0f, 0.0f));
	actorDesc.globalPose.M.setRow(1, NxVec3(1.0f, 0.0f, 0.0f));
	actorDesc.globalPose.M.setRow(2, NxVec3(0.0f, 0.0f, 1.0f));
	NxActor* rotated = scene->createActor(actorDesc);
	printf("momentum rotated_created=%u\n", rotated ? 1u : 0u);
	if(!rotated) return nxFail("rotated actor creation failed");
	unsigned char* rotatedBody = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(rotated) + 0x14);
	unsigned char* rotatedRecord = *reinterpret_cast<unsigned char**>(rotatedBody + 8);
	printf("momentum rotated_quaternion=%x.%x.%x.%x\n",
		word(rotatedRecord, 0x5c), word(rotatedRecord, 0x60),
		word(rotatedRecord, 0x64), word(rotatedRecord, 0x68));
	printf("momentum rotated_inverse=%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
		word(rotatedRecord, 0x164), word(rotatedRecord, 0x168), word(rotatedRecord, 0x16c),
		word(rotatedRecord, 0x170), word(rotatedRecord, 0x174), word(rotatedRecord, 0x178),
		word(rotatedRecord, 0x17c), word(rotatedRecord, 0x180), word(rotatedRecord, 0x184));
	printf("momentum rotated_rotation=%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
		word(rotatedRecord, 0x134), word(rotatedRecord, 0x138), word(rotatedRecord, 0x13c),
		word(rotatedRecord, 0x140), word(rotatedRecord, 0x144), word(rotatedRecord, 0x148),
		word(rotatedRecord, 0x14c), word(rotatedRecord, 0x150), word(rotatedRecord, 0x154));
	printf("momentum rotated_frame=%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
		word(rotatedRecord, 0xdc), word(rotatedRecord, 0xe0), word(rotatedRecord, 0xe4),
		word(rotatedRecord, 0xe8), word(rotatedRecord, 0xec), word(rotatedRecord, 0xf0),
		word(rotatedRecord, 0xf4), word(rotatedRecord, 0xf8), word(rotatedRecord, 0xfc));
	printVector("rotated_angular_initial", rotated->getAngularMomentumVal());
	printMatrix("rotated_global_inertia", rotated->getGlobalInertiaTensorVal());
	printMatrix("rotated_global_inverse", rotated->getGlobalInertiaTensorInverseVal());
	rotated->setAngularMomentum(NxVec3(18.0f, 20.0f, 28.0f));
	printVector("rotated_angular_set", rotated->getAngularMomentumVal());
	printVector("rotated_angular_velocity", rotated->getAngularVelocityVal());
	printf("momentum rotated_energy=%x\n", bits(rotated->computeKineticEnergy()));
	rotated->setMassSpaceInertiaTensor(NxVec3(3.0f, 5.0f, 7.0f));
	printf("momentum rotated_changed_inverse=%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
		word(rotatedRecord, 0x164), word(rotatedRecord, 0x168), word(rotatedRecord, 0x16c),
		word(rotatedRecord, 0x170), word(rotatedRecord, 0x174), word(rotatedRecord, 0x178),
		word(rotatedRecord, 0x17c), word(rotatedRecord, 0x180), word(rotatedRecord, 0x184));
	printVector("rotated_changed_angular", rotated->getAngularMomentumVal());
	printMatrix("rotated_changed_inertia", rotated->getGlobalInertiaTensorVal());
	printMatrix("rotated_changed_global_inverse", rotated->getGlobalInertiaTensorInverseVal());
	rotated->setAngularMomentum(NxVec3(20.0f, 21.0f, 28.0f));
	printVector("rotated_changed_velocity", rotated->getAngularVelocityVal());
	scene->releaseActor(*rotated);
	actorDesc.globalPose.M.id();
	bodyDesc.massLocalPose.M.setRow(0, NxVec3(1.0f, 0.0f, 0.0f));
	bodyDesc.massLocalPose.M.setRow(1, NxVec3(0.0f, 0.0f, -1.0f));
	bodyDesc.massLocalPose.M.setRow(2, NxVec3(0.0f, 1.0f, 0.0f));
	NxActor* offset = scene->createActor(actorDesc);
	printf("momentum offset_created=%u\n", offset ? 1u : 0u);
	if(!offset) return nxFail("offset actor creation failed");
	unsigned char* offsetBody = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(offset) + 0x14);
	unsigned char* offsetRecord = *reinterpret_cast<unsigned char**>(offsetBody + 8);
	printf("momentum offset_quaternion=%x.%x.%x.%x\n",
		word(offsetRecord, 0x5c), word(offsetRecord, 0x60),
		word(offsetRecord, 0x64), word(offsetRecord, 0x68));
	printf("momentum offset_frame=%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
		word(offsetRecord, 0xdc), word(offsetRecord, 0xe0), word(offsetRecord, 0xe4),
		word(offsetRecord, 0xe8), word(offsetRecord, 0xec), word(offsetRecord, 0xf0),
		word(offsetRecord, 0xf4), word(offsetRecord, 0xf8), word(offsetRecord, 0xfc));
	printf("momentum offset_rotation=%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
		word(offsetRecord, 0x134), word(offsetRecord, 0x138), word(offsetRecord, 0x13c),
		word(offsetRecord, 0x140), word(offsetRecord, 0x144), word(offsetRecord, 0x148),
		word(offsetRecord, 0x14c), word(offsetRecord, 0x150), word(offsetRecord, 0x154));
	printf("momentum offset_inverse=%x.%x.%x.%x.%x.%x.%x.%x.%x\n",
		word(offsetRecord, 0x164), word(offsetRecord, 0x168), word(offsetRecord, 0x16c),
		word(offsetRecord, 0x170), word(offsetRecord, 0x174), word(offsetRecord, 0x178),
		word(offsetRecord, 0x17c), word(offsetRecord, 0x180), word(offsetRecord, 0x184));
	printMatrix("offset_global_inertia", offset->getGlobalInertiaTensorVal());
	printMatrix("offset_global_inverse", offset->getGlobalInertiaTensorInverseVal());
	printVector("offset_angular_initial", offset->getAngularMomentumVal());
	offset->setAngularMomentum(NxVec3(8.0f, 15.0f, 24.0f));
	printVector("offset_angular_velocity", offset->getAngularVelocityVal());
	printf("momentum offset_energy=%x\n", bits(offset->computeKineticEnergy()));
	scene->releaseActor(*offset);
	NxActorDesc staticDesc;
	staticDesc.shapes.pushBack(&box);
	NxActor* staticActor = scene->createActor(staticDesc);
	printf("momentum static_created=%u\n", staticActor ? 1u : 0u);
	if(!staticActor) return nxFail("static actor creation failed");
	printMatrix("static_global_inertia", staticActor->getGlobalInertiaTensorVal());
	printMatrix("static_global_inverse", staticActor->getGlobalInertiaTensorInverseVal());
	printf("momentum static_energy=%x\n", bits(staticActor->computeKineticEnergy()));
	scene->releaseActor(*staticActor);
	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
