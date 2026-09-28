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
#include <math.h>

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

static void printMoveState(const char* label, const unsigned char* state,
	const unsigned before[8], const unsigned char* record)
{
	unsigned mask = 0;
	for(unsigned i = 0; i < 8; ++i)
		if(word(state, 4 * i) != before[i]) mask |= 1u << i;
	printf("dynamics %s=%x", label, mask);
	for(unsigned i = 0; i < 8; ++i)
		if(mask & (1u << i)) printf(".%u:%x", i, word(state, 4 * i));
	printf(".%x.%x.%x.%x\n", word(record, 0x10c),
		word(record, 0x50), word(record, 0x54), word(record, 0x58));
}

// NpActor.cpp completion Task 3 (000124, 000126, 000090 and 000784): a
// kinematic actor with a rotated, offset mass frame. Each move prints the
// whole 0x20-byte target block, the wake words (+0x84, +0x4c, +0x114) and the
// record's dirty flags; the wake counter is set before each move to just
// below, at and above 0.39999998f, or the actor is put to sleep.
static NxMat33 rotationOf(float x, float y, float z, float w)
{
	const float inv = 1.0f / sqrtf(x * x + y * y + z * z + w * w);
	x *= inv; y *= inv; z *= inv; w *= inv;
	NxMat33 m;
	m.setRow(0, NxVec3(1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - w * z), 2.0f * (x * z + w * y)));
	m.setRow(1, NxVec3(2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z - w * x)));
	m.setRow(2, NxVec3(2.0f * (x * z - w * y), 2.0f * (y * z + w * x), 1.0f - 2.0f * (x * x + y * y)));
	return m;
}
static void printTarget(const char* label, NxActor* actor)
{
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x14);
	const unsigned char* record = *reinterpret_cast<unsigned char* const*>(body + 8);
	const unsigned char* state = *reinterpret_cast<unsigned char* const*>(record + 0x118);
	const unsigned char* aux = *reinterpret_cast<unsigned char* const*>(record + 0x120);
	const unsigned* flags = *reinterpret_cast<unsigned* const*>(aux + 0x40);
	printf("dynamics %s target=", label);
	for(unsigned i = 0; i < 8; ++i)
		printf("%s%x", i ? "." : "", word(state, 4 * i));
	printf(" wake=%x.%x.%x dirty=%x\n", word(record, 0x84), word(record, 0x4c),
		word(record, 0x114), flags[word(record, 0x11c)]);
}
// The target block is cleared and the record's dirty word zeroed before a
// move, so the move's own flag bits and wake mark (0x10) are what is printed;
// afterwards the dirty list, index and word are put back.
struct DirtyWindow
{
	unsigned* flags; unsigned* index; unsigned** end; unsigned id;
	unsigned savedFlags, savedIndex; unsigned* savedEnd;
	DirtyWindow(NxActor* actor)
	{
		const unsigned char* body = *reinterpret_cast<unsigned char* const*>(
			reinterpret_cast<const unsigned char*>(actor) + 0x14);
		const unsigned char* record = *reinterpret_cast<unsigned char* const*>(body + 8);
		unsigned char* state = *reinterpret_cast<unsigned char* const*>(record + 0x118);
		memset(state, 0, 0x20);
		unsigned char* aux = *reinterpret_cast<unsigned char* const*>(record + 0x120);
		flags = *reinterpret_cast<unsigned**>(aux + 0x40);
		index = *reinterpret_cast<unsigned**>(aux + 0x60);
		end = reinterpret_cast<unsigned**>(aux + 0x54);
		id = word(record, 0x11c);
		savedFlags = flags[id]; savedIndex = index[id]; savedEnd = *end;
	}
	void open() { flags[id] = 0; }
	void close() { flags[id] = savedFlags; index[id] = savedIndex; *end = savedEnd; }
};
static void kinematicMoves(NxScene* scene, const char* name, const NxMat33& massRotation,
	const NxMat33& first, const NxMat33& second)
{
	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxBodyDesc bodyDesc;
	bodyDesc.mass = 4.0f;
	bodyDesc.massSpaceInertia = NxVec3(1.0f, 2.0f, 3.0f);
	bodyDesc.massLocalPose.M = massRotation;
	bodyDesc.massLocalPose.t = NxVec3(0.3f, -0.7f, 1.1f);
	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&box);
	actorDesc.body = &bodyDesc;
	actorDesc.globalPose.t = NxVec3(2.0f, -1.0f, 0.5f);
	NxActor* actor = scene->createActor(actorDesc);
	printf("dynamics kin_%s created=%u\n", name, actor ? 1u : 0u);
	if(!actor) return;
	actor->raiseBodyFlag(NX_BF_KINEMATIC);
	char label[96];
	NxMat34 pose;
	pose.M = first;
	pose.t = NxVec3(-1.25f, 2.5f, 0.75f);
	actor->wakeUp(0.1f);
	{ DirtyWindow w(actor); w.open(); actor->moveGlobalPose(pose);
	sprintf(label, "kin_%s_pose_low", name); printTarget(label, actor); w.close(); }
	actor->wakeUp(0.39999998f);
	{ DirtyWindow w(actor); w.open(); actor->moveGlobalOrientation(second);
	sprintf(label, "kin_%s_orientation_at", name); printTarget(label, actor); w.close(); }
	actor->wakeUp(0.39999995f);
	{ DirtyWindow w(actor); w.open(); actor->moveGlobalPosition(NxVec3(3.5f, -4.25f, 1.125f));
	sprintf(label, "kin_%s_position_below", name); printTarget(label, actor); w.close(); }
	actor->putToSleep();
	{ DirtyWindow w(actor); w.open(); actor->moveGlobalPose(pose);
	sprintf(label, "kin_%s_pose_asleep", name); printTarget(label, actor); w.close(); }
	actor->wakeUp(0.5f);
	{ DirtyWindow w(actor); w.open(); actor->moveGlobalOrientation(first);
	sprintf(label, "kin_%s_orientation_high", name); printTarget(label, actor);
	actor->moveGlobalPosition(NxVec3(-0.5f, 0.25f, 6.0f));
	sprintf(label, "kin_%s_position_or", name); printTarget(label, actor); w.close(); }
	scene->releaseActor(*actor);
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
	actor->raiseBodyFlag(NX_BF_KINEMATIC);
	const unsigned char* moveState = *reinterpret_cast<unsigned char* const*>(
		record + 0x118);
	unsigned moveBefore[8];
	for(unsigned i = 0; i < 8; ++i) moveBefore[i] = word(moveState, 4 * i);
	managerFlags[id] = 0;
	const unsigned beforeMoveQueue = static_cast<unsigned>(*activeEnd - activeBegin);
	actor->moveGlobalPosition(NxVec3(7.0f, -8.0f, 9.0f));
	printMoveState("move_position", moveState, moveBefore, record);
	printf("dynamics move_position_dirty=%x.%u.%u\n", managerFlags[id],
		static_cast<unsigned>(*activeEnd - activeBegin) - beforeMoveQueue,
		managerIndex[id]);
	for(unsigned i = 0; i < 8; ++i) moveBefore[i] = word(moveState, 4 * i);
	const float moveRotationRows[9] = {0.0f, -1.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
	NxMat33 moveRotation;
	moveRotation.setRowMajor(moveRotationRows);
	actor->moveGlobalOrientation(moveRotation);
	printMoveState("move_orientation", moveState, moveBefore, record);
	for(unsigned i = 0; i < 8; ++i) moveBefore[i] = word(moveState, 4 * i);
	NxMat34 movePose;
	const float movePoseRows[9] = {1.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f};
	movePose.M.setRowMajor(movePoseRows);
	movePose.t = NxVec3(-2.0f, 3.0f, 4.0f);
	actor->moveGlobalPose(movePose);
	printMoveState("move_pose", moveState, moveBefore, record);
	for(unsigned i = 0; i < 8; ++i) moveBefore[i] = word(moveState, 4 * i);
	actor->moveGlobalPosition(NxVec3(-5.0f, 6.0f, 7.0f));
	printMoveState("move_position_after_pose", moveState, moveBefore, record);
	printf("dynamics move_pose_dirty=%x.%u.%u\n", managerFlags[id],
		static_cast<unsigned>(*activeEnd - activeBegin) - beforeMoveQueue,
		managerIndex[id]);
	managerFlags[id] = 0xffffffffu;
	*activeEnd = savedEnd;
	managerIndex[id] = savedIndex;
	actor->clearBodyFlag(NX_BF_KINEMATIC);
	actor->raiseBodyFlag(NX_BF_KINEMATIC);
	const unsigned char* freshMoveState = *reinterpret_cast<unsigned char* const*>(
		record + 0x118);
	actor->moveGlobalOrientation(moveRotation);
	printf("dynamics move_orientation_fresh=");
	for(unsigned i = 0; i < 8; ++i)
		printf("%s%x", i ? "." : "", word(freshMoveState, 4 * i));
	printf("\n");
	actor->clearBodyFlag(NX_BF_KINEMATIC);
	actor->moveGlobalPosition(NxVec3(11.0f, 12.0f, 13.0f));
	actor->moveGlobalOrientation(moveRotation);
	actor->moveGlobalPose(movePose);
	printf("dynamics move_nonkinematic=%x.%x.%x.%x.%u\n",
		word(record, 0x10c), word(record, 0x50),
		word(record, 0x54), word(record, 0x58),
		*reinterpret_cast<void* const*>(record + 0x118) ? 1u : 0u);
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
	// NpActor.cpp completion Task 3: the targets walk every arm of 000124's
	// and 000126's quaternion conversion (trace, and the x, y and z pivots).
	kinematicMoves(scene, "general", rotationOf(-0.3f, 0.5f, 0.2f, 0.8f),
		rotationOf(1.0f, 2.0f, 3.0f, 4.0f), rotationOf(0.95f, 0.2f, 0.1f, 0.2f));
	kinematicMoves(scene, "identity_frame", NxMat33(NX_IDENTITY_MATRIX),
		rotationOf(0.15f, 0.9f, -0.3f, 0.25f), rotationOf(-0.2f, 0.25f, 0.9f, 0.3f));
	kinematicMoves(scene, "rotated_frame", rotationOf(0.95f, 0.2f, 0.1f, 0.2f),
		rotationOf(0.0f, 0.0f, 1.0f, 0.0f), rotationOf(0.2f, -0.4f, 0.3f, 0.8f));
	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
