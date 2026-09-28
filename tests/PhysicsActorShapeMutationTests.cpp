#include "PhysicsPairLoader.h"
#include "NxPageGuardedAllocator.h"

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxActor.h"
#include "NxBoxShapeDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxCapsuleShapeDesc.h"
#include "NxPlaneShapeDesc.h"
#include "NxBodyDesc.h"
#include "NxShape.h"
#include "PhysicsActorErrorStream.h"

#include <stdio.h>
#include <string.h>

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(
	NxU32, NxUserAllocator*, NxUserOutputStream*);

static unsigned shapeRootType(const NxActor* actor)
	{
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x14);
	const unsigned char* shape = *reinterpret_cast<unsigned char* const*>(body + 0x10);
	return shape ? *reinterpret_cast<const unsigned*>(shape + 0xd0) : 0xffffffffu;
	}

static const unsigned char* shapeRoot(const NxActor* actor)
	{
	const unsigned char* body = *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x14);
	return *reinterpret_cast<unsigned char* const*>(body + 0x10);
	}

static void printAllocations(const char* label, const NxPageGuardedAllocator& allocator,
	unsigned beforeAlloc, unsigned beforeFree)
	{
	const unsigned added = allocator.allocations() - beforeAlloc;
	const unsigned removed = allocator.frees() - beforeFree;
	printf("shape_mutation %s=%u.%u", label, added, removed);
	for(unsigned i = 0; i < added; ++i)
		printf(".%u", allocator.allocSizeFromEnd(added - 1 - i));
	printf(".f");
	for(unsigned i = 0; i < removed; ++i)
		printf(".%u", allocator.freedSizeFromEnd(removed - 1 - i));
	printf("\n");
	}

// NpActor.cpp completion Task 4: createShape (000070 -> Actor.cpp 000036) and
// releaseShape (000072 -> Actor.cpp 000024) on static and dynamic actors, every
// shape family the harness can build, group promotion, appends that grow the
// group arrays, swap-removal at every position, releasing down to no shape,
// installing into an empty actor, and the Actor.cpp E1 reports.
static unsigned word(const void* base, unsigned offset)
	{
	unsigned value;
	memcpy(&value, static_cast<const unsigned char*>(base) + offset, 4);
	return value;
	}

static unsigned half(const void* base, unsigned offset)
	{
	unsigned short value;
	memcpy(&value, static_cast<const unsigned char*>(base) + offset, 2);
	return value;
	}

static const unsigned char* actorBody(const NxActor* actor)
	{
	return *reinterpret_cast<unsigned char* const*>(
		reinterpret_cast<const unsigned char*>(actor) + 0x14);
	}

// 001957 + 001960 over the Scene's pruning collection at +0x624: the two
// pruner counts 000036 hands 000503.
static unsigned prunerCount(const unsigned char* scene)
	{
	const unsigned char* pruning = scene + 0x624;
	const unsigned char* first = reinterpret_cast<const unsigned char*>(word(pruning, 0x1c));
	const unsigned char* indexed = reinterpret_cast<const unsigned char*>(
		word(pruning, 0x1c + word(pruning, 0x70) * 4));
	return (first ? word(first, 0xc) + word(first, 8) : 0) * 0x10000u +
		(indexed ? word(indexed, 0xc) + word(indexed, 8) : 0);
	}

// count.root-type, then for each public handle its index in `known` (the
// order the handles were created), the group or single-root words the
// chain writes, and per child: type, +0xdc, +8, +0xd4, the +0xa0 pruning
// link relative to the Scene, and whether [handle+8] is the child.
static void printState(const char* label, const NxActor* actor,
	NxShape* const* known, unsigned knownCount)
	{
	const unsigned char* body = actorBody(actor);
	const unsigned char* scene = *reinterpret_cast<unsigned char* const*>(body + 4);
	const unsigned char* root = *reinterpret_cast<unsigned char* const*>(body + 0x10);
	const NxU32 count = actor->getNbShapes();
	NxShape* const* handles = actor->getShapes();
	printf("shape_mutation %s=%u.%x", label, count, root ? word(root, 0xd0) : 0xffffffffu);
	for(NxU32 i = 0; i < count; ++i)
		{
		unsigned index = 0xff;
		for(unsigned j = 0; j < knownCount; ++j)
			if(handles[i] == known[j]) index = j;
		printf(".%x", index);
		}
	if(root && word(root, 0xd0) == 5)
		{
		const unsigned e0 = word(root, 0xe0), e4 = word(root, 0xe4), e8 = word(root, 0xe8);
		const unsigned f0 = word(root, 0xf0), f4 = word(root, 0xf4), f8 = word(root, 0xf8);
		printf(".g%u.%u.%u.%u.%x.%x.%x.%x.%x", (e4 - e0) / 4, (e8 - e0) / 4,
			(f4 - f0) / 4, (f8 - f0) / 4, word(root, 8), word(root, 0x10c),
			half(root, 0xdc), word(root, 0xd4),
			word(root, 0xa0) ? word(root, 0xa0) - reinterpret_cast<unsigned>(scene) : 0);
		}
	else if(root)
		printf(".s");
	for(NxU32 i = 0; i < count; ++i)
		{
		const unsigned char* child = reinterpret_cast<const unsigned char*>(
			word(handles[i], 8));
		const unsigned inArray = root && word(root, 0xd0) == 5
			? (word(reinterpret_cast<const void*>(word(root, 0xe0) + 4 * i), 0) ==
				reinterpret_cast<unsigned>(child) ? 1u : 0u)
			: (child == root ? 1u : 0u);
		printf(".c%x.%x.%x.%x.%x.%u", word(child, 0xd0), half(child, 0xdc),
			word(child, 8), word(child, 0xd4),
			word(child, 0xa0) ? word(child, 0xa0) - reinterpret_cast<unsigned>(scene) : 0,
			inArray);
		}
	printf(".p%x.%x.%x\n", word(scene, 4), prunerCount(scene), word(scene, 0x540));
	}

static void printWorldPose(const char* label, const NxShape* shape)
	{
	const unsigned char* child = reinterpret_cast<const unsigned char*>(word(shape, 8));
	printf("shape_mutation %s=", label);
	for(unsigned i = 0; i < 12; ++i)
		printf("%s%x", i ? "." : "", word(child, 0x0c + 4 * i));
	printf("\n");
	}

// The root group's pose one (+0x0c), pose two (+0x3c), +8 and +0xdc.
static void printGroupPose(const char* label, const NxActor* actor)
	{
	const unsigned char* root = *reinterpret_cast<unsigned char* const*>(actorBody(actor) + 0x10);
	printf("shape_mutation %s=", label);
	for(unsigned i = 0; i < 24; ++i)
		printf("%x.", word(root, 0x0c + 4 * i));
	printf("%x.%x\n", word(root, 8), half(root, 0xdc));
	}

#define CREATE_CASE(label, actor, desc, known, knownCount) \
	{ \
	const unsigned a0 = allocator.allocations(), f0 = allocator.frees(); \
	const unsigned e0 = errors.reports; \
	NxShape* created = (actor)->createShape(desc); \
	printAllocations(label "_memory", allocator, a0, f0); \
	printf("shape_mutation " label "_result=%u.%u\n", created ? 1u : 0u, errors.reports - e0); \
	if(created) (known)[(knownCount)++] = created; \
	printState(label, actor, known, knownCount); \
	if(created) printWorldPose(label "_world", created); \
	}

#define RELEASE_CASE(label, actor, shape, known, knownCount) \
	{ \
	const unsigned a0 = allocator.allocations(), f0 = allocator.frees(); \
	const unsigned e0 = errors.reports; \
	(actor)->releaseShape(shape); \
	printAllocations(label "_memory", allocator, a0, f0); \
	printf("shape_mutation " label "_errors=%u\n", errors.reports - e0); \
	printState(label, actor, known, knownCount); \
	}

// A rotation about one axis from a fixed cosine/sine pair (row-major).
static NxMat33 axisRotation(unsigned axis, float c, float sn)
	{
	float m[9] = { 1, 0, 0, 0, 1, 0, 0, 0, 1 };
	const unsigned a = (axis + 1) % 3, b = (axis + 2) % 3;
	m[a * 3 + a] = c; m[a * 3 + b] = -sn;
	m[b * 3 + a] = sn; m[b * 3 + b] = c;
	NxMat33 r;
	r.setRowMajor(m);
	return r;
	}

static void runTask4Cases(NxPhysicsSDK* sdk, NxPageGuardedAllocator& allocator,
	NxActorErrorStream& errors)
	{
	errors.enabled = true;
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) { printf("shape_mutation t4_scene=0\n"); return; }

	NxMat34 pose;
	pose.M = axisRotation(0, 0.87758255f, 0.47942555f);
	pose.t = NxVec3(1.0f, 2.0f, 3.0f);
	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxSphereShapeDesc sphere;
	sphere.radius = 0.75f;
	sphere.localPose.t = NxVec3(0.5f, -1.0f, 2.0f);
	NxCapsuleShapeDesc capsule;
	capsule.radius = 0.25f;
	capsule.height = 1.5f;
	capsule.localPose.M = axisRotation(1, 0.95533651f, 0.29552022f);
	capsule.localPose.t = NxVec3(-2.0f, 0.25f, 1.0f);
	NxPlaneShapeDesc plane;
	plane.normal = NxVec3(0.0f, 1.0f, 0.0f);
	plane.d = -4.0f;
	NxBoxShapeDesc box2;
	box2.dimensions = NxVec3(0.5f, 0.25f, 2.0f);
	box2.localPose.M = axisRotation(2, 0.76484221f, -0.64421767f);
	box2.localPose.t = NxVec3(3.0f, 1.0f, -1.0f);
	NxBoxShapeDesc badBox;
	badBox.dimensions = NxVec3(-1.0f, 1.0f, 1.0f);

	// Static actor: promotion by a sphere, appends that grow the arrays
	// (2 -> 6), every family, then releases at each position.
	NxActorDesc staticDesc;
	staticDesc.shapes.pushBack(&box);
	staticDesc.globalPose = pose;
	NxActor* st = scene->createActor(staticDesc);
	printf("shape_mutation t4_static=%u\n", st ? 1u : 0u);
	NxBodyDesc bodyDesc;
	bodyDesc.mass = 2.0f;
	bodyDesc.massSpaceInertia = NxVec3(1.0f, 2.0f, 3.0f);
	NxActorDesc dynamicDesc;
	dynamicDesc.shapes.pushBack(&box);
	dynamicDesc.globalPose = pose;
	dynamicDesc.body = &bodyDesc;
	NxActor* dy = scene->createActor(dynamicDesc);
	printf("shape_mutation t4_dynamic=%u\n", dy ? 1u : 0u);
	if(!st || !dy) return;
	NxShape* stKnown[16] = { st->getShapes()[0] };
	unsigned stCount = 1;
	NxShape* dyKnown[16] = { dy->getShapes()[0] };
	unsigned dyCount = 1;
	printState("t4_st_initial", st, stKnown, stCount);
	printState("t4_dy_initial", dy, dyKnown, dyCount);
	// The prunable type and kind bytes (+0xce/+0xcf) of each actor's root.
	printf("shape_mutation t4_prunables=%x.%x.%x.%x\n",
		*(reinterpret_cast<const unsigned char*>(word(stKnown[0], 8)) + 0xce),
		*(reinterpret_cast<const unsigned char*>(word(stKnown[0], 8)) + 0xcf),
		*(reinterpret_cast<const unsigned char*>(word(dyKnown[0], 8)) + 0xce),
		*(reinterpret_cast<const unsigned char*>(word(dyKnown[0], 8)) + 0xcf));

	CREATE_CASE("t4_st_invalid", st, badBox, stKnown, stCount);
	CREATE_CASE("t4_st_sphere", st, sphere, stKnown, stCount);
	CREATE_CASE("t4_st_capsule", st, capsule, stKnown, stCount);
	CREATE_CASE("t4_st_plane", st, plane, stKnown, stCount);
	CREATE_CASE("t4_st_box", st, box2, stKnown, stCount);
	CREATE_CASE("t4_st_sphere2", st, sphere, stKnown, stCount);
	CREATE_CASE("t4_st_capsule2", st, capsule, stKnown, stCount);

	CREATE_CASE("t4_dy_invalid", dy, badBox, dyKnown, dyCount);
	CREATE_CASE("t4_dy_capsule", dy, capsule, dyKnown, dyCount);
	CREATE_CASE("t4_dy_sphere", dy, sphere, dyKnown, dyCount);
	CREATE_CASE("t4_dy_box", dy, box2, dyKnown, dyCount);

	// The group's own poses: the promotion's 000531 runs the group's slot 6
	// (001018: each child's 001315, then 001315 on the group), and so do the
	// pose setters (000004 on the root).
	printGroupPose("t4_st_group_pose", st);
	printGroupPose("t4_dy_group_pose", dy);
	NxMat34 moved;
	moved.M = axisRotation(2, 0.87758255f, -0.47942555f);
	moved.t = NxVec3(-1.0f, 0.5f, 4.0f);
	st->setGlobalPose(moved);
	dy->setGlobalPose(moved);
	printGroupPose("t4_st_group_moved", st);
	printGroupPose("t4_dy_group_moved", dy);
	printWorldPose("t4_dy_child_moved", dyKnown[3]);

	// Swap-removal (001028): middle, first, last; a handle the group does
	// not hold is ignored without a report.
	RELEASE_CASE("t4_st_release_middle", st, *stKnown[3], stKnown, stCount);
	RELEASE_CASE("t4_st_release_first", st, *stKnown[0], stKnown, stCount);
	RELEASE_CASE("t4_st_release_last", st, *st->getShapes()[st->getNbShapes() - 1],
		stKnown, stCount);
	RELEASE_CASE("t4_st_release_foreign", st, *dyKnown[0], stKnown, stCount);
	while(st->getNbShapes() > 1)
		RELEASE_CASE("t4_st_release_down", st, *st->getShapes()[0], stKnown, stCount);
	// A static actor may not be left with no shapes: E1 0x18f.
	RELEASE_CASE("t4_st_release_only", st, *st->getShapes()[0], stKnown, stCount);

	// Dynamic: release down to nothing. The last release tears the group
	// down (000006 and the group's deleting destructor).
	RELEASE_CASE("t4_dy_release_middle", dy, *dyKnown[2], dyKnown, dyCount);
	while(dy->getNbShapes() > 1)
		RELEASE_CASE("t4_dy_release_down", dy, *dy->getShapes()[dy->getNbShapes() - 1],
			dyKnown, dyCount);
	RELEASE_CASE("t4_dy_release_only", dy, *dy->getShapes()[0], dyKnown, dyCount);
	// No shape at all: E1 0x1a4.
	RELEASE_CASE("t4_dy_release_empty", dy, *stKnown[0], dyKnown, dyCount);
	// Installing into an empty actor.
	CREATE_CASE("t4_dy_install", dy, sphere, dyKnown, dyCount);
	// A single root that is not the shape: E1 0x19d.
	RELEASE_CASE("t4_dy_release_mismatch", dy, *stKnown[0], dyKnown, dyCount);
	// Releasing the single root (000006 and the child's deleting destructor).
	RELEASE_CASE("t4_dy_release_single", dy, *dy->getShapes()[0], dyKnown, dyCount);
	CREATE_CASE("t4_dy_install2", dy, capsule, dyKnown, dyCount);
	CREATE_CASE("t4_dy_promote", dy, plane, dyKnown, dyCount);

	// A static actor's single shape: E1 0x19c.
	NxActorDesc singleDesc;
	singleDesc.shapes.pushBack(&sphere);
	NxActor* ss = scene->createActor(singleDesc);
	printf("shape_mutation t4_single=%u\n", ss ? 1u : 0u);
	if(ss)
		{
		NxShape* ssKnown[4] = { ss->getShapes()[0] };
		unsigned ssCount = 1;
		RELEASE_CASE("t4_ss_release_only", ss, *ssKnown[0], ssKnown, ssCount);
		CREATE_CASE("t4_ss_box", ss, box, ssKnown, ssCount);
		}

	const unsigned a0 = allocator.allocations(), f0 = allocator.frees();
	scene->releaseActor(*dy);
	printAllocations("t4_dy_actor_release_memory", allocator, a0, f0);
	const unsigned a1 = allocator.allocations(), f1 = allocator.frees();
	scene->releaseActor(*st);
	printAllocations("t4_st_actor_release_memory", allocator, a1, f1);
	sdk->releaseScene(*scene);

	// A fresh Scene whose first shape comes from createShape: a dynamic
	// actor built without shapes takes a box (000531 with the Scene's first
	// shape and first dynamic pruner), then a capsule (promotion), and is
	// released down to nothing again.
	NxScene* fresh = sdk->createScene(sceneDesc);
	if(!fresh) { printf("shape_mutation t4_fresh_scene=0\n"); return; }
	NxActorDesc bareDesc;
	bareDesc.globalPose = pose;
	bareDesc.body = &bodyDesc;
	const unsigned b0 = allocator.allocations(), g0 = allocator.frees();
	NxActor* bare = fresh->createActor(bareDesc);
	printAllocations("t4_bare_create_memory", allocator, b0, g0);
	printf("shape_mutation t4_bare=%u.%u\n", bare ? 1u : 0u, bare ? bare->getNbShapes() : 0u);
	if(bare)
		{
		NxShape* bareKnown[4] = { 0 };
		unsigned bareCount = 0;
		CREATE_CASE("t4_bare_box", bare, box, bareKnown, bareCount);
		CREATE_CASE("t4_bare_capsule", bare, capsule, bareKnown, bareCount);
		printGroupPose("t4_bare_group_pose", bare);
		if(bareCount == 2)
			{
			RELEASE_CASE("t4_bare_release_box", bare, *bareKnown[0], bareKnown, bareCount);
			RELEASE_CASE("t4_bare_release_capsule", bare, *bareKnown[1], bareKnown, bareCount);
			}
		const unsigned b1 = allocator.allocations(), g1 = allocator.frees();
		fresh->releaseActor(*bare);
		printAllocations("t4_bare_release_memory", allocator, b1, g1);
		}
	errors.enabled = false;
	sdk->releaseScene(*fresh);
	}

int wmain(int argc, wchar_t** argv)
	{
	setvbuf(stdout, 0, _IONBF, 0);
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsActorShapeMutationTests",
		pairDirectory, &physics);
	if(status) return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK) return nxFail("NxCreatePhysicsSDK is missing");
	static NxPageGuardedAllocator allocator;
	static NxActorErrorStream errors("shape_mutation");
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &allocator, &errors);
	if(!sdk) return nxFail("SDK creation failed");
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) return nxFail("scene creation failed");
	NxBoxShapeDesc first;
	first.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&first);
	NxActor* actor = scene->createActor(actorDesc);
	printf("shape_mutation actor=%u\n", actor ? 1u : 0u);
	if(!actor) return nxFail("actor creation failed");
	NxShape* original = actor->getShapes()[0];
	printf("shape_mutation initial=%u.%u.%u\n", actor->getNbShapes(),
		shapeRootType(actor), original ? 1u : 0u);
	NxBoxShapeDesc second;
	second.dimensions = NxVec3(0.5f, 1.0f, 1.5f);
	const unsigned beforeAddAlloc = allocator.allocations();
	const unsigned beforeAddFree = allocator.frees();
	NxShape* added = actor->createShape(second);
	printAllocations("add_memory", allocator, beforeAddAlloc, beforeAddFree);
	const NxU32 countAfterAdd = actor->getNbShapes();
	NxShape** handlesAfterAdd = actor->getShapes();
	printf("shape_mutation added=%u.%u.%u.%u.%u\n", added ? 1u : 0u,
		countAfterAdd, shapeRootType(actor),
		countAfterAdd > 0 && handlesAfterAdd[0] == original ? 1u : 0u,
		countAfterAdd > 1 && handlesAfterAdd[1] == added ? 1u : 0u);
	if(shapeRootType(actor) == 5)
		{
		const unsigned char* group = shapeRoot(actor);
		void* const* childFirst = *reinterpret_cast<void* const* const*>(group + 0xe0);
		void* const* childEnd = *reinterpret_cast<void* const* const*>(group + 0xe4);
		void* const* childCap = *reinterpret_cast<void* const* const*>(group + 0xe8);
		void* const* publicFirst = *reinterpret_cast<void* const* const*>(group + 0xf0);
		void* const* publicEnd = *reinterpret_cast<void* const* const*>(group + 0xf4);
		void* const* publicCap = *reinterpret_cast<void* const* const*>(group + 0xf8);
		printf("shape_mutation group=%u.%u.%u.%u.%u.%u\n",
			static_cast<unsigned>(childEnd - childFirst),
			static_cast<unsigned>(childCap - childFirst),
			static_cast<unsigned>(publicEnd - publicFirst),
			static_cast<unsigned>(publicCap - publicFirst),
			childFirst[0] == *reinterpret_cast<void* const*>(
				reinterpret_cast<const unsigned char*>(original) + 0x18) ? 1u : 0u,
			publicFirst[0] == original ? 1u : 0u);
		}
	if(added)
		{
		const unsigned beforeReleaseAlloc = allocator.allocations();
		const unsigned beforeReleaseFree = allocator.frees();
		actor->releaseShape(*added);
		printAllocations("release_memory", allocator,
			beforeReleaseAlloc, beforeReleaseFree);
		const NxU32 countAfterRelease = actor->getNbShapes();
		NxShape** handlesAfterRelease = actor->getShapes();
		printf("shape_mutation released=%u.%u.%u\n", countAfterRelease,
			shapeRootType(actor), countAfterRelease > 0 &&
			handlesAfterRelease[0] == original ? 1u : 0u);
		if(shapeRootType(actor) == 5)
			{
			const unsigned char* group = shapeRoot(actor);
			void* const* childFirst = *reinterpret_cast<void* const* const*>(group + 0xe0);
			void* const* childEnd = *reinterpret_cast<void* const* const*>(group + 0xe4);
			void* const* publicFirst = *reinterpret_cast<void* const* const*>(group + 0xf0);
			void* const* publicEnd = *reinterpret_cast<void* const* const*>(group + 0xf4);
			printf("shape_mutation released_group=%u.%u.%u.%u\n",
				static_cast<unsigned>(childEnd - childFirst),
				static_cast<unsigned>(publicEnd - publicFirst),
				childFirst[0] == *reinterpret_cast<void* const*>(
					reinterpret_cast<const unsigned char*>(original) + 0x18) ? 1u : 0u,
				publicFirst[0] == original ? 1u : 0u);
			}
		}
	const unsigned beforeActorReleaseAlloc = allocator.allocations();
	const unsigned beforeActorReleaseFree = allocator.frees();
	scene->releaseActor(*actor);
	printAllocations("actor_release_memory", allocator,
		beforeActorReleaseAlloc, beforeActorReleaseFree);
	sdk->releaseScene(*scene);
	runTask4Cases(sdk, allocator, errors);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
	}
