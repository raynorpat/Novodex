// NxPhysicsBodyCreationTests (scene-raycast block Task 4, sub-area body-creation):
// the dynamic body record the body constructor 000797 builds (with 000801,
// 000793/000795, 000722 and 000748) and the destructor 000776/000799 tears down,
// read through NxScene::createActor/releaseActor on both DLLs. Each line prints
// record words the constructor chain writes, never one it leaves to the
// allocation (the allocations are 0xcd-filled here, NX_PAGE_GUARDED_FILL): the
// defaults and the live SDK parameters the constructor falls back on, an
// explicit descriptor, a massSpaceInertia whose inverse is not finite, a body
// created kinematic (its 0x20-byte block, freed by 000799 on release), and the
// record id pool.
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
#include <stdlib.h>

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(
	NxU32, NxUserAllocator*, NxUserOutputStream*);

typedef void (__thiscall *MarkIslandDirtyFn)(void*);

static unsigned word(const unsigned char* bytes, unsigned offset);

static void writeWord(unsigned char* bytes, unsigned offset, unsigned value)
{
	memcpy(bytes + offset, &value, sizeof(value));
}

static unsigned markIslandDirtyRva(const wchar_t* pairDirectory)
{
	if(wcsstr(pairDirectory, L"oracle")) return 0x16f80;
	char path[MAX_PATH] = {};
	if(!GetModuleFileNameA(0, path, MAX_PATH)) return 0;
	char* slash = strrchr(path, '\\');
	if(!slash) return 0;
	strcpy(slash + 1, "NxPhysics.map");
	HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if(file == INVALID_HANDLE_VALUE) return 0;
	static char contents[65536];
	DWORD bytesRead = 0;
	const BOOL read = ReadFile(file, contents, sizeof(contents) - 1, &bytesRead, 0);
	CloseHandle(file);
	if(!read) return 0;
	contents[bytesRead] = 0;
	char* symbol = strstr(contents, "?markIslandDirty@DynamicBody@@QAEXXZ");
	if(!symbol) return 0;
	while(symbol > contents && symbol[-1] != '\n') --symbol;
	unsigned section = 0, offset = 0, address = 0;
	char name[128] = {};
	if(sscanf(symbol, "%x:%x %127s %x", &section, &offset, name, &address) != 4 ||
		strcmp(name, "?markIslandDirty@DynamicBody@@QAEXXZ")) return 0;
	return address - 0x10000000u;
}

// Public actor creation calls this row before an island exists. Use a minimal
// self-root record to exercise its non-null-island branch directly in each
// staged DLL; the row reads only +0x1bc, +0x1e0, and +0x1e4 on this path.
static bool testMarkIslandDirty(HMODULE physics, const wchar_t* pairDirectory)
{
	const unsigned rva = markIslandDirtyRva(pairDirectory);
	if(!rva) return false;
	MarkIslandDirtyFn mark = reinterpret_cast<MarkIslandDirtyFn>(
		reinterpret_cast<unsigned char*>(physics) + rva);
	if(!mark) return false;
	unsigned char record[0x1e8] = {};
	unsigned islandObject = 0;
	writeWord(record, 0x1bc, reinterpret_cast<NxU32>(record));
	writeWord(record, 0x1e0, reinterpret_cast<NxU32>(&islandObject));
	writeWord(record, 0x1e4, 1);
	mark(record);
	const unsigned withIsland = word(record, 0x1e4);

	writeWord(record, 0x1e0, 0);
	writeWord(record, 0x1e4, 5);
	mark(record);
	const unsigned withoutIsland = word(record, 0x1e4);
	printf("bodycreate mark_island_dirty=%x.%x\n", withIsland, withoutIsland);
	return withIsland == 3 && withoutIsland == 5;
}

static NxPageGuardedAllocator gAllocator;

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

static const unsigned char* recordOf(NxActor* actor)
{
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x14);
	return *reinterpret_cast<unsigned char* const*>(body + 8);
}

static const unsigned char* bodyOf(NxActor* actor)
{
	return *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x14);
}

static void printAuxRegistration(const char* label, NxActor* actor)
{
	const unsigned char* record = recordOf(actor);
	const unsigned id = word(record, 0x11c);
	const unsigned char* aux = reinterpret_cast<const unsigned char*>(
		word(record, 0x120));
	const unsigned* records = *reinterpret_cast<unsigned* const*>(aux + 0x80);
	const unsigned* occupied = *reinterpret_cast<unsigned* const*>(aux + 0x40);
	const unsigned* active = *reinterpret_cast<unsigned* const*>(aux + 0x50);
	const unsigned* activeEnd = *reinterpret_cast<unsigned* const*>(aux + 0x54);
	const unsigned* indices = *reinterpret_cast<unsigned* const*>(aux + 0x60);
	const unsigned* secondary = *reinterpret_cast<unsigned* const*>(aux + 0x90);
	const unsigned* secondaryCapacity = *reinterpret_cast<unsigned* const*>(aux + 0x98);
	const unsigned index = indices[id];
	printf("bodycreate aux_%s=%x.%u.%u.%u.%u.%u.%u.%u\n", label, id,
		records[id] == reinterpret_cast<unsigned>(record + 0x18) ? 1u : 0u,
		occupied[id] == 0xffffffffu ? 1u : 0u,
		index < static_cast<unsigned>(activeEnd - active) ? 1u : 0u,
		index < static_cast<unsigned>(activeEnd - active) && active[index] == id ? 1u : 0u,
		static_cast<unsigned>(activeEnd - active),
		secondary ? 1u : 0u,
		secondary ? static_cast<unsigned>(secondaryCapacity - secondary) : 0u);
	const unsigned char* body = bodyOf(actor);
	const unsigned char* scene = *reinterpret_cast<unsigned char* const*>(body + 4);
	const unsigned char* shape = *reinterpret_cast<unsigned char* const*>(body + 0x10);
	const unsigned shapeId = word(shape, 0xd4);
	const unsigned char* shapeAux = *reinterpret_cast<unsigned char* const*>(scene + 0x48);
	const unsigned* shapeFlags = *reinterpret_cast<unsigned* const*>(shapeAux);
	const unsigned* shapeActive = *reinterpret_cast<unsigned* const*>(shapeAux + 0x10);
	const unsigned* shapeActiveEnd = *reinterpret_cast<unsigned* const*>(shapeAux + 0x14);
	const unsigned* shapeIndices = *reinterpret_cast<unsigned* const*>(shapeAux + 0x20);
	const unsigned* shapes = *reinterpret_cast<unsigned* const*>(shapeAux + 0x90);
	const unsigned* shapeCapacity = *reinterpret_cast<unsigned* const*>(shapeAux + 0x98);
	const unsigned shapeIndex = shapeIndices[shapeId];
	printf("bodycreate aux_shape_%s=%x.%u.%u.%u.%u.%u.%u\n", label, shapeId,
		shapeFlags[shapeId] == 0xffffffffu ? 1u : 0u,
		shapeIndex < static_cast<unsigned>(shapeActiveEnd - shapeActive) ? 1u : 0u,
		shapeIndex < static_cast<unsigned>(shapeActiveEnd - shapeActive) &&
			shapeActive[shapeIndex] == shapeId ? 1u : 0u,
		shapes[shapeId] == reinterpret_cast<unsigned>(shape) ? 1u : 0u,
		static_cast<unsigned>(shapeActiveEnd - shapeActive),
		static_cast<unsigned>(shapeCapacity - shapes));
}

// `count` consecutive words from `first`, dot-separated.
static void printRange(const char* label, const unsigned char* record,
	unsigned first, unsigned count)
{
	printf("bodycreate %s=", label);
	for(unsigned i = 0; i < count; ++i)
		printf(i ? ".%x" : "%x", word(record, first + 4 * i));
	printf("\n");
}

// A word that holds a pointer: printed as which of these it names.
static const char* pointerName(const unsigned char* record, unsigned offset,
	const unsigned char* self, const unsigned char* body)
{
	const unsigned value = word(record, offset);
	if(value == 0) return "0";
	if(value == reinterpret_cast<unsigned>(self)) return "self";
	if(value == reinterpret_cast<unsigned>(body)) return "body";
	return "other";
}

// `fromShapes`: the mass came from the density, so the tensor is 000008's
// (the image's shape-mass pass, which the candidate emulates for one box and
// does not reproduce: its inertia differs in the last bit). The tensor lines
// and the world inverse tensor built from them are skipped for those bodies.
static void printRecord(const char* label, NxActor* actor, bool fromShapes)
{
	const unsigned char* record = recordOf(actor);
	const unsigned char* body = bodyOf(actor);
	char name[96];
	sprintf(name, "%s_mass", label);
	printRange(name, record, 0x188, 1);
	sprintf(name, "%s_inverse_mass", label);
	printRange(name, record, 0xc0, 1);
	if(!fromShapes)
		{
		sprintf(name, "%s_inertia", label);
		printRange(name, record, 0x18c, 3);
		sprintf(name, "%s_inverse_inertia", label);
		printRange(name, record, 0xc4, 3);
		}
	sprintf(name, "%s_sleep", label);
	printRange(name, record, 0xd0, 3);
	sprintf(name, "%s_damping", label);
	printRange(name, record, 0xb8, 2);
	sprintf(name, "%s_pose", label);
	printRange(name, record, 0x18, 7);
	sprintf(name, "%s_pose_copy", label);
	printRange(name, record, 0x50, 7);
	sprintf(name, "%s_velocity", label);
	printRange(name, record, 0x34, 7);
	sprintf(name, "%s_velocity_copy", label);
	printRange(name, record, 0x6c, 7);
	sprintf(name, "%s_base_zero", label);
	printRange(name, record, 0x88, 12);
	sprintf(name, "%s_mass_pose", label);
	printRange(name, record, 0xdc, 12);
	sprintf(name, "%s_flags", label);
	printRange(name, record, 0x10c, 3);
	sprintf(name, "%s_frame", label);
	printRange(name, record, 0x124, 16);
	if(!fromShapes)
		{
		sprintf(name, "%s_world_inverse_inertia", label);
		printRange(name, record, 0x164, 9);
		}
	sprintf(name, "%s_stamp", label);
	printRange(name, record, 0x198, 1);
	sprintf(name, "%s_zero", label);
	printRange(name, record, 0x1a0, 7);
	sprintf(name, "%s_island", label);
	printRange(name, record, 0x1c0, 4);
	sprintf(name, "%s_island_tail", label);
	printRange(name, record, 0x1d8, 4);
	sprintf(name, "%s_group", label);
	printRange(name, record, 0x1ec, 5);
	sprintf(name, "%s_saved_frame", label);
	printRange(name, record, 0x20c, 12);
	sprintf(name, "%s_bounds", label);
	printRange(name, record, 0x244, 6);
	printf("bodycreate %s_links=%s.%s.%s.%s.%s.%s.%s.%s.%x.%x.%x.%x\n", label,
		pointerName(record, 0x19c, record, body), pointerName(record, 0x1bc, record, body),
		pointerName(record, 0x1d0, record, body), pointerName(record, 0x1d4, record, body),
		pointerName(record, 0x1e8, record, body), pointerName(record, 0x1fc, record, body),
		pointerName(record, 0x200, record, body), pointerName(record, 0x118, record, body),
		word(record, 0x204), word(record, 0x208), word(record, 0x25c),
		word(record, 4) | word(record, 8) | word(record, 0xc));
	printf("bodycreate %s_getters=%x.%x.%x\n", label,
		bits(actor->getSleepLinearVelocity()), bits(actor->getSleepAngularVelocity()),
		bits(actor->getMass()));
}

static NxActor* createBox(NxScene* scene, NxBodyDesc& bodyDesc, NxReal density,
	const NxMat34& pose)
{
	NxBoxShapeDesc box;
	box.dimensions = NxVec3(0.5f, 1.25f, 2.0f);
	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&box);
	actorDesc.body = &bodyDesc;
	actorDesc.density = density;
	actorDesc.globalPose = pose;
	return scene->createActor(actorDesc);
}

// The allocations and frees since the given counts, with their sizes, newest
// last.
static void printTraffic(const char* label, unsigned allocs, unsigned frees)
{
	const unsigned a = gAllocator.allocations() - allocs;
	const unsigned f = gAllocator.frees() - frees;
	printf("bodycreate %s=%u.%u", label, a, f);
	printf(" a");
	for(unsigned i = a; i > 0; --i)
		printf(".%x", gAllocator.allocSizeFromEnd(i));
	printf(" f");
	for(unsigned i = f; i > 0; --i)
		printf(".%x", gAllocator.freedSizeFromEnd(i));
	printf("\n");
}

int wmain(int argc, wchar_t** argv)
{
	setvbuf(stdout, 0, _IONBF, 0);
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsBodyCreationTests",
		pairDirectory, &physics);
	if(status) return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK) return nxFail("NxCreatePhysicsSDK is missing");
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &gAllocator, 0);
	if(!sdk) return nxFail("SDK creation failed");
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) return nxFail("scene creation failed");
	if(!testMarkIslandDirty(physics, pairDirectory))
		return nxFail("DynamicBody::markIslandDirty differential fixture failed");

	NxMat34 identity;
	identity.id();
	NxMat34 rotated;
	{
		NxQuat q;
		q.fromAngleAxis(37.0f, NxVec3(0.36f, -0.48f, 0.8f));
		rotated.M.fromQuat(q);
		rotated.t = NxVec3(1.5f, -2.25f, 3.125f);
	}

	// The defaults: density only, so the mass and tensor come from the box
	// (000008 in the image, emulated for one box in the candidate); the
	// sleep velocities and the angular limit fall back on the SDK's.
	NxBodyDesc defaults;
	NxActor* a = createBox(scene, defaults, 1.5f, identity);
	printf("bodycreate default_created=%u\n", a ? 1u : 0u);
	if(!a) return nxFail("default actor creation failed");
	printRecord("default", a, true);

	// The live SDK parameters, not the shipped defaults.
	const bool setLin = sdk->setParameter(NX_DEFAULT_SLEEP_LIN_VEL_SQUARED, 0.36f);
	const bool setAng = sdk->setParameter(NX_DEFAULT_SLEEP_ANG_VEL_SQUARED, 0.0625f);
	const bool setMax = sdk->setParameter(NX_MAX_ANGULAR_VELOCITY, 5.5f);
	printf("bodycreate sdk_parameters_set=%u.%u.%u\n", setLin ? 1u : 0u,
		setAng ? 1u : 0u, setMax ? 1u : 0u);
	NxBodyDesc live;
	live.maxAngularVelocity = 0.0f;
	live.sleepLinearVelocity = 0.0f;
	live.sleepAngularVelocity = -3.0f;
	NxActor* b = createBox(scene, live, 2.0f, rotated);
	printf("bodycreate live_created=%u\n", b ? 1u : 0u);
	if(!b) return nxFail("live-parameter actor creation failed");
	printRecord("live", b, true);
	sdk->setParameter(NX_DEFAULT_SLEEP_LIN_VEL_SQUARED, 0.15f * 0.15f);
	sdk->setParameter(NX_DEFAULT_SLEEP_ANG_VEL_SQUARED, 0.14f * 0.14f);
	sdk->setParameter(NX_MAX_ANGULAR_VELOCITY, 7.0f);

	// An explicit descriptor: mass and tensor given, so the image does not
	// compute them from the shapes; every field the constructor copies.
	NxBodyDesc full;
	full.mass = 2.5f;
	full.massSpaceInertia = NxVec3(0.5f, 1.5f, 3.0f);
	{
		NxQuat q;
		q.fromAngleAxis(-23.0f, NxVec3(0.6f, 0.0f, 0.8f));
		full.massLocalPose.M.fromQuat(q);
		full.massLocalPose.t = NxVec3(0.1f, -0.2f, 0.3f);
	}
	full.linearVelocity = NxVec3(1.0f, -2.0f, 3.5f);
	full.angularVelocity = NxVec3(0.25f, -0.5f, 0.75f);
	full.wakeUpCounter = 0.7f;
	full.linearDamping = 0.125f;
	full.angularDamping = 0.3f;
	full.maxAngularVelocity = 9.0f;
	full.sleepLinearVelocity = 0.3f;
	full.sleepAngularVelocity = 0.2f;
	full.solverIterationCount = 7;
	full.flags = NX_BF_DISABLE_GRAVITY;
	NxActor* c = createBox(scene, full, 0.0f, rotated);
	printf("bodycreate full_created=%u\n", c ? 1u : 0u);
	if(!c) return nxFail("explicit actor creation failed");
	printRecord("full", c, false);

	// A tensor with a zero element: its inverse is infinite, so _fpclass
	// rejects all three and the inverse tensor is zero.
	NxBodyDesc flat;
	flat.mass = 1.0f;
	flat.massSpaceInertia = NxVec3(1.0f, 0.0f, 2.0f);
	NxActor* d = createBox(scene, flat, 0.0f, identity);
	printf("bodycreate flat_created=%u\n", d ? 1u : 0u);
	if(!d) return nxFail("zero-element tensor actor creation failed");
	printRecord("flat", d, false);

	// Created kinematic: 000795 calls setKinematic(true), which zeroes the
	// inverses and allocates the 0x20-byte block at +0x118.
	NxBodyDesc kinematic;
	kinematic.mass = 3.0f;
	kinematic.massSpaceInertia = NxVec3(1.0f, 2.0f, 4.0f);
	kinematic.flags = NX_BF_KINEMATIC | NX_BF_VISUALIZATION;
	unsigned allocs = gAllocator.allocations();
	unsigned frees = gAllocator.frees();
	NxActor* e = createBox(scene, kinematic, 0.0f, identity);
	printf("bodycreate kinematic_created=%u\n", e ? 1u : 0u);
	if(!e) return nxFail("kinematic actor creation failed");
	printTraffic("kinematic_create_traffic", allocs, frees);
	printRecord("kinematic", e, false);
	{
		const unsigned char* block = *reinterpret_cast<unsigned char* const*>(
			recordOf(e) + 0x118);
		printf("bodycreate kinematic_block=%x\n", block ? word(block, 0xc) : 0xffffffffu);
	}
	printf("bodycreate kinematic_flag=%u\n", e->readBodyFlag(NX_BF_KINEMATIC) ? 1u : 0u);

	// The record ids: a released record's id is the next one taken.
	printf("bodycreate ids=%x.%x.%x.%x.%x\n", word(recordOf(a), 0x11c),
		word(recordOf(b), 0x11c), word(recordOf(c), 0x11c), word(recordOf(d), 0x11c),
		word(recordOf(e), 0x11c));
	const unsigned char* releaseCRecord = recordOf(c);
	const unsigned releaseCId = word(releaseCRecord, 0x11c);
	const unsigned char* releaseCAux = reinterpret_cast<const unsigned char*>(
		word(releaseCRecord, 0x120));
	unsigned* releaseCRecords = *reinterpret_cast<unsigned* const*>(releaseCAux + 0x80);
	const unsigned releaseCRegistered = releaseCRecords[releaseCId]
		== reinterpret_cast<unsigned>(releaseCRecord + 0x18) ? 1u : 0u;
	allocs = gAllocator.allocations();
	frees = gAllocator.frees();
	scene->releaseActor(*c);
	printf("bodycreate release_c_record_slot=%u.%u\n", releaseCRegistered,
		releaseCRecords[releaseCId] == 0 ? 1u : 0u);
	printTraffic("release_traffic", allocs, frees);
	NxBodyDesc again;
	NxActor* f = createBox(scene, again, 1.0f, identity);
	printf("bodycreate reused_created=%u\n", f ? 1u : 0u);
	if(!f) return nxFail("reused-id actor creation failed");
	printf("bodycreate reused_id=%x\n", word(recordOf(f), 0x11c));
	printRecord("reused", f, true);

	// A kinematic record released as it is: 000799 frees its block.
	allocs = gAllocator.allocations();
	frees = gAllocator.frees();
	scene->releaseActor(*e);
	printTraffic("kinematic_release_traffic", allocs, frees);

	// Sub-area setters: 000785/000787 through NxActor::raiseBodyFlag and
	// clearBodyFlag (000188/000190) on the zero-element tensor body: entering
	// zeroes the inverses and allocates the 0x20-byte block; leaving takes
	// 1.0f / mass and 1.0f / each tensor element with no test (0x19abb,
	// 0x19bb6-0x19bce), so the zero element gives an infinite inverse, and
	// frees the block.
	allocs = gAllocator.allocations();
	frees = gAllocator.frees();
	d->raiseBodyFlag(NX_BF_KINEMATIC);
	printTraffic("setters_enter_traffic", allocs, frees);
	printRange("setters_enter_inverses", recordOf(d), 0xc0, 4);
	printRange("setters_enter_flags", recordOf(d), 0x10c, 1);
	{
		const unsigned char* block = *reinterpret_cast<unsigned char* const*>(
			recordOf(d) + 0x118);
		printf("bodycreate setters_enter_block=%x\n", block ? word(block, 0xc) : 0xffffffffu);
	}
	allocs = gAllocator.allocations();
	frees = gAllocator.frees();
	d->clearBodyFlag(NX_BF_KINEMATIC);
	printTraffic("setters_leave_traffic", allocs, frees);
	printRange("setters_leave_inverses", recordOf(d), 0xc0, 4);
	printRange("setters_leave_flags", recordOf(d), 0x10c, 1);
	printf("bodycreate setters_leave_block=%u\n", word(recordOf(d), 0x118) ? 1u : 0u);

	// 000782 with a force mode above 4 (`cmp eax,4; ja 0x1936f`) changes no
	// field but still runs the wake block: a counter below 0x3ecccccc is
	// raised at +0x84 and +0x4c.
	NxBodyDesc drowsy;
	drowsy.mass = 2.0f;
	drowsy.massSpaceInertia = NxVec3(1.0f, 1.0f, 1.0f);
	drowsy.wakeUpCounter = 0.125f;
	NxActor* g = createBox(scene, drowsy, 0.0f, identity);
	printf("bodycreate setters_drowsy_created=%u\n", g ? 1u : 0u);
	if(!g) return nxFail("drowsy actor creation failed");
	printf("bodycreate setters_drowsy_wake=%x.%x\n", word(recordOf(g), 0x84), word(recordOf(g), 0x4c));
	g->addForce(NxVec3(1.0f, 2.0f, 3.0f), static_cast<NxForceMode>(5));
	printf("bodycreate setters_mode5_wake=%x.%x\n", word(recordOf(g), 0x84), word(recordOf(g), 0x4c));
	printRange("setters_mode5_accumulators", recordOf(g), 0x88, 12);
	printRange("setters_mode5_velocity", recordOf(g), 0x6c, 6);
	scene->releaseActor(*g);

	scene->releaseActor(*f);
	scene->releaseActor(*d);
	scene->releaseActor(*b);
	scene->releaseActor(*a);

	// Release low IDs out of order so the next LIFO-reused ID is not the first
	// vacant table entry. Registration must use the body ID for all three maps.
	NxActor* reuseStress[6] = {};
	NxBodyDesc registrationBody;
	for(unsigned i = 0; i < 6; ++i)
		{
		reuseStress[i] = createBox(scene, registrationBody, 1.0f, identity);
		if(!reuseStress[i]) return nxFail("aux registration reuse actor creation failed");
		}
	scene->releaseActor(*reuseStress[0]);
	scene->releaseActor(*reuseStress[5]);
	NxActor* reusedRegistrationA = createBox(scene, registrationBody, 1.0f, identity);
	NxActor* reusedRegistrationB = createBox(scene, registrationBody, 1.0f, identity);
	if(!reusedRegistrationA || !reusedRegistrationB)
		return nxFail("aux registration recycled actor creation failed");
	printAuxRegistration("reused_lifo", reusedRegistrationA);
	printAuxRegistration("reused_hole", reusedRegistrationB);
	for(unsigned i = 1; i < 5; ++i) scene->releaseActor(*reuseStress[i]);
	scene->releaseActor(*reusedRegistrationA);
	scene->releaseActor(*reusedRegistrationB);

	// Cross the first 256-ID chunk. Keep these stress actors until process exit:
	// the shipped DLL's release path has a separate 256+ row still under study.
	NxActor* registrationStress[257] = {};
	for(unsigned i = 0; i < 257; ++i)
		{
		registrationStress[i] = createBox(scene, registrationBody, 1.0f, identity);
		if(!registrationStress[i]) return nxFail("aux registration stress actor creation failed");
		}
	printAuxRegistration("chunk_256", registrationStress[256]);
	return nxReportPairIdentity(pairDirectory);
}
