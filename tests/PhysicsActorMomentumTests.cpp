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
#include <math.h>

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

// NpActor.cpp completion Task 3: the wake blocks of the velocity and
// momentum setters (000174, 000176, 000180, 000182), 000182's row sums and
// the kinetic energy (000060/000742). A rotated body with a rotated mass
// frame makes the world inverse inertia +0x164 general.
static NxMat33 task3Rotation(float x, float y, float z, float w)
{
	const float inv = 1.0f / sqrtf(x * x + y * y + z * z + w * w);
	x *= inv; y *= inv; z *= inv; w *= inv;
	NxMat33 m;
	m.setRow(0, NxVec3(1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y - w * z), 2.0f * (x * z + w * y)));
	m.setRow(1, NxVec3(2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z - w * x)));
	m.setRow(2, NxVec3(2.0f * (x * z - w * y), 2.0f * (y * z + w * x), 1.0f - 2.0f * (x * x + y * y)));
	return m;
}
static NxVec3 task3Bits(unsigned x, unsigned y, unsigned z)
{
	NxVec3 v;
	memcpy(&v.x, &x, 4); memcpy(&v.y, &y, 4); memcpy(&v.z, &z, 4);
	return v;
}
static unsigned char* task3Record(NxActor* actor)
{
	unsigned char* body = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(actor) + 0x14);
	return *reinterpret_cast<unsigned char**>(body + 8);
}
// The record's dirty word is zeroed before the setter, so the setter's own
// marks (4 or 8, and 0x10 when it wakes) are what is printed; the dirty list,
// index and word are put back afterwards.
struct Task3Dirty
{
	unsigned* flags; unsigned* index; unsigned** end; unsigned id;
	unsigned savedFlags, savedIndex; unsigned* savedEnd;
	Task3Dirty(NxActor* actor)
	{
		unsigned char* record = task3Record(actor);
		unsigned char* aux = *reinterpret_cast<unsigned char**>(record + 0x120);
		flags = *reinterpret_cast<unsigned**>(aux + 0x40);
		index = *reinterpret_cast<unsigned**>(aux + 0x60);
		end = reinterpret_cast<unsigned**>(aux + 0x54);
		id = word(record, 0x11c);
		savedFlags = flags[id]; savedIndex = index[id]; savedEnd = *end;
		flags[id] = 0;
	}
	unsigned mark() const { return flags[id]; }
	~Task3Dirty() { flags[id] = savedFlags; index[id] = savedIndex; *end = savedEnd; }
};
enum Task3Setter { T3_LINEAR_VELOCITY, T3_ANGULAR_VELOCITY, T3_LINEAR_MOMENTUM, T3_ANGULAR_MOMENTUM };
static void task3Wake(NxActor* actor, const char* name, Task3Setter setter,
	const NxVec3& value, bool asleep)
{
	if(asleep) actor->putToSleep();
	else actor->wakeUp(0.1f);
	unsigned char* record = task3Record(actor);
	unsigned mark;
	{
		Task3Dirty dirty(actor);
		switch(setter)
		{
			case T3_LINEAR_VELOCITY: actor->setLinearVelocity(value); break;
			case T3_ANGULAR_VELOCITY: actor->setAngularVelocity(value); break;
			case T3_LINEAR_MOMENTUM: actor->setLinearMomentum(value); break;
			case T3_ANGULAR_MOMENTUM: actor->setAngularMomentum(value); break;
		}
		mark = dirty.mark();
	}
	printf("momentum t3_wake %s in=%x.%x.%x lin=%x.%x.%x ang=%x.%x.%x wake=%x.%x.%x dirty=%x\n",
		name, bits(value.x), bits(value.y), bits(value.z),
		word(record, 0x6c), word(record, 0x70), word(record, 0x74),
		word(record, 0x78), word(record, 0x7c), word(record, 0x80),
		word(record, 0x84), word(record, 0x4c), word(record, 0x114), mark);
}
// The getters' world mass rotation W = R F (000134 and 000142 in one operand
// order, 000138, 000140 and 000144 in theirs) and the 000746 tensors. With
// the mass frame near R^T, W is near the identity and its off-diagonal
// elements come out of cancelling products, where the order decides the
// bits.
static void task3Getters(NxScene* scene, const char* name, const NxMat33& orientation,
	const NxMat33& massRotation)
{
	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxBodyDesc body;
	body.mass = 3.0f;
	body.massSpaceInertia = NxVec3(1.25f, 2.5f, 4.75f);
	body.massLocalPose.M = massRotation;
	body.massLocalPose.t = NxVec3(0.5f, -0.25f, 1.5f);
	body.angularVelocity = NxVec3(0.7f, -1.3f, 2.1f);
	NxActorDesc desc;
	desc.shapes.pushBack(&box);
	desc.body = &body;
	desc.globalPose.M = orientation;
	desc.globalPose.t = NxVec3(-1.0f, 0.5f, 2.0f);
	NxActor* actor = scene->createActor(desc);
	char tag[96];
	sprintf(tag, "t3_get_%s created=%u", name, actor ? 1u : 0u);
	printf("momentum %s\n", tag);
	if(!actor) return;
	sprintf(tag, "t3_get_%s cmass_orientation", name);
	printMatrix(tag, actor->getCMassGlobalPose().M);
	sprintf(tag, "t3_get_%s cmass_position", name);
	printVector(tag, actor->getCMassGlobalPose().t);
	sprintf(tag, "t3_get_%s cmass_orientation_only", name);
	printMatrix(tag, actor->getCMassGlobalOrientation());
	sprintf(tag, "t3_get_%s inertia", name);
	printMatrix(tag, actor->getGlobalInertiaTensor());
	sprintf(tag, "t3_get_%s inverse_inertia", name);
	printMatrix(tag, actor->getGlobalInertiaTensorInverse());
	sprintf(tag, "t3_get_%s angular_momentum", name);
	printVector(tag, actor->getAngularMomentum());
	scene->releaseActor(*actor);
}
static NxMat33 task3Transpose(const NxMat33& m)
{
	NxMat33 t;
	t.setTransposed(m);
	return t;
}
static void task3Cases(NxScene* scene)
{
	const NxMat33 r0 = task3Rotation(1.0f, 2.0f, 3.0f, 4.0f);
	const NxMat33 r1 = task3Rotation(0.95f, 0.2f, 0.1f, 0.2f);
	const NxMat33 r2 = task3Rotation(-0.2f, 0.25f, 0.9f, 0.3f);
	const NxMat33 r3 = task3Rotation(0.3f, -0.7f, 0.5f, 0.4f);
	task3Getters(scene, "general", r0, task3Rotation(-0.3f, 0.5f, 0.2f, 0.8f));
	task3Getters(scene, "inverse0", r0, task3Transpose(r0));
	task3Getters(scene, "inverse1", r1, task3Transpose(r1));
	task3Getters(scene, "inverse2", r2, task3Transpose(r2));
	task3Getters(scene, "inverse3", r3, task3Transpose(r3));
	task3Getters(scene, "near3", r3, task3Transpose(task3Rotation(0.3f, -0.7f, 0.5f, 0.41f)));
	// 000134's column 1 and 2 orders against 000138's: a stored body quaternion
	// and a local mass frame found so that one element of W differs between
	// the two orders (row 0 column 1, row 1 column 2).
	const unsigned rfCases[2][13] = {
		{ 0x3e06942du, 0x3ef77032u, 0xbf0368c5u, 0xbf32684fu, 0x3bc12f05u, 0x3f57adcdu,
			0x3f09e664u, 0xbf16a3c3u, 0x3ee07e9au, 0xbf2de891u, 0xbf4efb48u, 0xbea03d86u,
			0x3eff2673u },
		{ 0x3f4d6825u, 0xbd847e35u, 0xbd62d9b9u, 0x3f173960u, 0x3f7c496cu, 0xbe2d4f91u,
			0xbc4be2edu, 0xbd1d3950u, 0xbe9663e3u, 0x3f748284u, 0xbe294675u, 0xbf70d75cu,
			0xbe9788f6u },
	};
	for(unsigned i = 0; i < 2; ++i)
	{
		NxBoxShapeDesc box;
		box.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
		NxBodyDesc body;
		body.mass = 3.0f;
		body.massSpaceInertia = NxVec3(1.25f, 2.5f, 4.75f);
		NxActorDesc desc;
		desc.shapes.pushBack(&box);
		desc.body = &body;
		NxActor* actor = scene->createActor(desc);
		if(!actor) continue;
		float frame[9];
		memcpy(frame, rfCases[i] + 4, sizeof(frame));
		NxMat33 local;
		local.setRowMajor(frame);
		actor->setCMassOffsetLocalOrientation(local);
		NxQuat q;
		memcpy(&q.x, rfCases[i], 4); memcpy(&q.y, rfCases[i] + 1, 4);
		memcpy(&q.z, rfCases[i] + 2, 4); memcpy(&q.w, rfCases[i] + 3, 4);
		actor->setGlobalOrientationQuat(q);
		char tag[64];
		sprintf(tag, "t3_rf%u pose_orientation", i);
		printMatrix(tag, actor->getCMassGlobalPose().M);
		sprintf(tag, "t3_rf%u orientation", i);
		printMatrix(tag, actor->getCMassGlobalOrientation());
		sprintf(tag, "t3_rf%u inverse_inertia", i);
		printMatrix(tag, actor->getGlobalInertiaTensorInverse());
		scene->releaseActor(*actor);
	}
	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxBodyDesc body;
	body.mass = 2.0f;
	body.massSpaceInertia = NxVec3(1.5f, 2.25f, 3.75f);
	body.massLocalPose.M = task3Rotation(-0.3f, 0.5f, 0.2f, 0.8f);
	body.massLocalPose.t = NxVec3(0.25f, -0.5f, 0.75f);
	NxActorDesc desc;
	desc.shapes.pushBack(&box);
	desc.body = &body;
	desc.globalPose.M = task3Rotation(1.0f, 2.0f, 3.0f, 4.0f);
	desc.globalPose.t = NxVec3(1.0f, -2.0f, 3.0f);
	NxActor* actor = scene->createActor(desc);
	printf("momentum t3 created=%u\n", actor ? 1u : 0u);
	if(!actor) return;
	unsigned char* record = task3Record(actor);
	actor->setSleepLinearVelocity(0.5f);
	actor->setSleepAngularVelocity(0.75f);
	printf("momentum t3 thresholds=%x.%x inverse=", word(record, 0xd0), word(record, 0xd4));
	for(unsigned i = 0; i < 9; ++i)
		printf("%s%x", i ? "." : "", word(record, 0x164 + 4 * i));
	printf("\n");
	const float nan = sqrtf(-1.0f);
	// 000174 / 000176: the input's squared speed at, just below and just
	// above the threshold (0.25 and 0.5625), NaN, and on a sleeping actor.
	task3Wake(actor, "lv_equal", T3_LINEAR_VELOCITY, NxVec3(0.5f, 0.0f, 0.0f), false);
	task3Wake(actor, "lv_below", T3_LINEAR_VELOCITY, NxVec3(0.0f, 0.49999997f, 0.0f), false);
	task3Wake(actor, "lv_above", T3_LINEAR_VELOCITY, NxVec3(0.0f, 0.0f, 0.50000006f), false);
	task3Wake(actor, "lv_mixed", T3_LINEAR_VELOCITY, NxVec3(0.3f, 0.4f, 0.0f), false);
	task3Wake(actor, "lv_nan", T3_LINEAR_VELOCITY, NxVec3(nan, 0.0f, 0.0f), false);
	task3Wake(actor, "lv_asleep", T3_LINEAR_VELOCITY, NxVec3(3.0f, 4.0f, 5.0f), true);
	task3Wake(actor, "av_equal", T3_ANGULAR_VELOCITY, NxVec3(0.0f, 0.75f, 0.0f), false);
	task3Wake(actor, "av_below", T3_ANGULAR_VELOCITY, NxVec3(0.74999994f, 0.0f, 0.0f), false);
	task3Wake(actor, "av_above", T3_ANGULAR_VELOCITY, NxVec3(0.0f, 0.0f, 0.75000006f), false);
	task3Wake(actor, "av_mixed", T3_ANGULAR_VELOCITY, NxVec3(0.45f, 0.6f, 0.0f), false);
	task3Wake(actor, "av_nan", T3_ANGULAR_VELOCITY, NxVec3(0.0f, nan, 0.0f), false);
	task3Wake(actor, "av_asleep", T3_ANGULAR_VELOCITY, NxVec3(1.0f, 2.0f, 3.0f), true);
	// 000180: the stored velocity (1/m p, m = 2) against 0.25.
	task3Wake(actor, "lm_equal", T3_LINEAR_MOMENTUM, NxVec3(1.0f, 0.0f, 0.0f), false);
	task3Wake(actor, "lm_below", T3_LINEAR_MOMENTUM, NxVec3(0.0f, 0.99999994f, 0.0f), false);
	task3Wake(actor, "lm_above", T3_LINEAR_MOMENTUM, NxVec3(0.0f, 0.0f, 1.0000001f), false);
	task3Wake(actor, "lm_mixed", T3_LINEAR_MOMENTUM, NxVec3(0.6f, 0.8f, 0.0f), false);
	task3Wake(actor, "lm_nan", T3_LINEAR_MOMENTUM, NxVec3(0.0f, 0.0f, nan), false);
	task3Wake(actor, "lm_asleep", T3_LINEAR_MOMENTUM, NxVec3(3.0f, 4.0f, 5.0f), true);
	// 000182: the row sums over the general +0x164 with distinct components,
	// and the stored angular velocity against 0.5625.
	task3Wake(actor, "am_general", T3_ANGULAR_MOMENTUM, NxVec3(1.3f, -2.7f, 0.9f), false);
	task3Wake(actor, "am_general2", T3_ANGULAR_MOMENTUM, NxVec3(-0.37f, 0.11f, 5.3f), false);
	task3Wake(actor, "am_small", T3_ANGULAR_MOMENTUM, NxVec3(0.01f, -0.02f, 0.03f), false);
	task3Wake(actor, "am_large", T3_ANGULAR_MOMENTUM, NxVec3(0.7f, 0.9f, -1.1f), false);
	// Momenta chosen so that each row nearly cancels, where the listing's
	// summation order decides the last bit of w (rows 0, 1 and 2).
	task3Wake(actor, "am_cancel_x", T3_ANGULAR_MOMENTUM,
		task3Bits(0xc2838868u, 0xc1dbd53du, 0x439bf301u), false);
	task3Wake(actor, "am_cancel_y", T3_ANGULAR_MOMENTUM,
		task3Bits(0x444e6cfcu, 0x42614caau, 0xbeb57702u), false);
	task3Wake(actor, "am_cancel_z", T3_ANGULAR_MOMENTUM,
		task3Bits(0xc54e02beu, 0xc1e93addu, 0x44289883u), false);
	task3Wake(actor, "am_nan", T3_ANGULAR_MOMENTUM, NxVec3(nan, 1.0f, 1.0f), false);
	task3Wake(actor, "am_asleep", T3_ANGULAR_MOMENTUM, NxVec3(3.0f, 4.0f, 5.0f), true);
	// 000168: the inverse inertia is gated by _fpclass on each float 1/m.
	const unsigned inertiaCases[][3] = {
		{ 0x40000000u, 0x40400000u, 0x40800000u },	// 2, 3, 4
		{ 0xc0000000u, 0x40400000u, 0x40800000u },	// -2 keeps its inverse
		{ 0x00000000u, 0x3f800000u, 0x3f800000u },	// 0: +Inf
		{ 0x80000000u, 0x3f800000u, 0x3f800000u },	// -0: -Inf
		{ 0x3f800000u, 0x7fc00000u, 0x3f800000u },	// NaN
		{ 0x3f800000u, 0x3f800000u, 0x7f800000u },	// +Inf: inverse 0
		{ 0x00000100u, 0x3f800000u, 0x3f800000u },	// denormal: overflow
		{ 0x00800000u, 0x3f800000u, 0x3f800000u },	// FLT_MIN: 2^126
		{ 0x3e800000u, 0x3e800000u, 0xff800000u },	// -Inf: inverse -0
	};
	for(unsigned i = 0; i < sizeof(inertiaCases) / sizeof(inertiaCases[0]); ++i)
	{
		actor->setMassSpaceInertiaTensor(task3Bits(inertiaCases[i][0],
			inertiaCases[i][1], inertiaCases[i][2]));
		printf("momentum t3_inertia %u in=%x.%x.%x stored=%x.%x.%x inverse=%x.%x.%x\n", i,
			inertiaCases[i][0], inertiaCases[i][1], inertiaCases[i][2],
			word(record, 0x18c), word(record, 0x190), word(record, 0x194),
			word(record, 0xc4), word(record, 0xc8), word(record, 0xcc));
	}
	// 000166 on positive masses the report does not reach: +Inf, a denormal
	// (whose inverse overflows) and FLT_MIN.
	const unsigned massCases[] = { 0x7f800000u, 0x00000100u, 0x00800000u, 0x40a00000u };
	for(unsigned i = 0; i < sizeof(massCases) / sizeof(massCases[0]); ++i)
	{
		float mass; memcpy(&mass, &massCases[i], 4);
		actor->setMass(mass);
		printf("momentum t3_mass %u in=%x stored=%x inverse=%x\n", i, massCases[i],
			word(record, 0x188), word(record, 0xc0));
	}
	// 000060/000742: the kinetic energy with distinct components, and a case
	// built so that the listing's order decides the rounding: m v.v =
	// 1 + 2^-24 (a float midpoint once halved) and three spin terms of
	// 0.39 ulp each, which survive only when summed before they meet it.
	struct EnergyCase { unsigned m, i0, i1, i2, v0, v1, v2, w0, w1, w2; };
	const EnergyCase energyCases[] = {
		{ 0x40400000u, 0x3fc00000u, 0x40100000u, 0x40700000u, 0x3fa66666u, 0xc02ccccdu,
			0x3f666666u, 0xbebd70a4u, 0x3de147aeu, 0x40a9999au },
		{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x39800000u,
			0x00000000u, 0x32200000u, 0x32200000u, 0x32200000u },
		{ 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x3f800000u, 0x39800000u,
			0x00000000u, 0x00000000u, 0x00000000u, 0x00000000u },
	};
	for(unsigned i = 0; i < sizeof(energyCases) / sizeof(energyCases[0]); ++i)
	{
		const EnergyCase& c = energyCases[i];
		float mass; memcpy(&mass, &c.m, 4);
		actor->setMass(mass);
		actor->setMassSpaceInertiaTensor(task3Bits(c.i0, c.i1, c.i2));
		actor->setLinearVelocity(task3Bits(c.v0, c.v1, c.v2));
		actor->setAngularVelocity(task3Bits(c.w0, c.w1, c.w2));
		printf("momentum t3_energy %u energy=%x\n", i, bits(actor->computeKineticEnergy()));
	}
	scene->releaseActor(*actor);
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
	task3Cases(scene);
	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
}
