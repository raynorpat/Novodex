// The scene raycast staged-pair differential (scene-raycast block, Task 3).
//
// Builds one scene of static and dynamic actors -- box, sphere, capsule and
// plane shapes, in several collision groups, one with raycasting disabled --
// and drives every public NxScene raycast over it: raycastAnyBounds,
// raycastAnyShape, raycastAllBounds, raycastAllShapes, raycastClosestBounds
// and raycastClosestShape. The rays cover hits and misses, rays that start
// inside a shape, rays from below the plane, each NxShapesType, group masks
// and maximum distances, and the hint flags. The error paths (a direction
// that is not a unit vector, a maximum distance that is not positive) are
// driven too; the SDK's reports are printed through an NxUserOutputStream.
// Two compound actors (a box and a sphere, static and dynamic) are queried,
// then three actors are released and the queries repeated (the pools drop
// them and the static pruner rebuilds its tree), and a static actor created
// after the tree was built is queried.
//
// No triangle mesh: the candidate's NxPhysicsSDK::createTriangleMesh
// (000242 -> 000478) still returns 0, so a mesh cannot be built on both sides.
//
// Every NxUserRaycastReport::onHit call is printed as the NxRaycastHit's words
// the hit's flags declare valid (shape, impact, normal, face, distance, u/v),
// plus the flags word itself. Words the flags do not declare are not printed:
// the hit the loop rows pass lives on their stack, and its undeclared words are
// whatever that stack held. A report can stop the query after N hits by
// returning false, which the transcript records as the stop.
//
// The closest queries write into a caller-owned NxRaycastHit that the harness
// fills with 0xcd bytes first. It is printed the same way, plus the distance
// word even when the flags do not declare it: a query writes it first
// (FLT_MAX), so an untouched hit shows cdcdcdcd. A kept shape hit copies the
// shape's whole local hit, whose undeclared words are the query's stack (the
// capsule writes no normal), so those are not printed.
//
// Every pointer is printed as a name (the shape's), never as an address.

#include "PhysicsPairLoader.h"
#include "NxPageGuardedAllocator.h"

#include <math.h>
#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxShape.h"
#include "NxBoxShapeDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxCapsuleShapeDesc.h"
#include "NxPlaneShapeDesc.h"
#include "NxUserRaycastReport.h"
#include "NxUserOutputStream.h"
#include "NxRay.h"
#include "NxBoxShape.h"
#include "NxBounds3.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

static NxU32 nxU(NxReal value)
	{
	NxU32 bits;
	memcpy(&bits, &value, 4);
	return bits;
	}

// ---------------------------------------------------------------------------
// Pointer names.

struct NxSymbol
	{
	const void* pointer;
	const char* name;
	};

static NxSymbol nxSymbols[64];
static unsigned nxSymbolCount = 0;

static void nxSymbolAdd(const void* pointer, const char* name)
	{
	if(pointer && nxSymbolCount < 64)
		{
		nxSymbols[nxSymbolCount].pointer = pointer;
		nxSymbols[nxSymbolCount].name = name;
		nxSymbolCount++;
		}
	}

// A name for a pointer that may already carry one: a shape allocated after a
// release can reuse a released shape's address, which keeps its old name in
// the table (the box resize block, created after the releases, renames).
static void nxSymbolSet(const void* pointer, const char* name)
	{
	for(unsigned i = 0; i < nxSymbolCount; i++)
		if(nxSymbols[i].pointer == pointer)
			{
			nxSymbols[i].name = name;
			return;
			}
	nxSymbolAdd(pointer, name);
	}

static void nxPrintWord(NxU32 word, bool first)
	{
	if(!first)
		printf(".");
	for(unsigned i = 0; i < nxSymbolCount; i++)
		if(word == reinterpret_cast<NxU32>(nxSymbols[i].pointer))
			{
			printf("%s", nxSymbols[i].name);
			return;
			}
	printf("%08x", static_cast<unsigned>(word));
	}

// ---------------------------------------------------------------------------
// The SDK's error reports.

class NxRecordingOutput : public NxUserOutputStream
	{
	public:
	virtual void reportError(NxErrorCode code, const char* message, const char* file, int line)
		{
		const char* base = file ? strrchr(file, '\\') : 0;
		printf("raycast report_error code=%d file=%s line=%d message=%s\n", static_cast<int>(code),
			base ? base + 1 : (file ? file : "null"), line, message ? message : "null");
		}
	virtual NxAssertResponse reportAssertViolation(const char* message, const char* file, int line)
		{
		const char* base = file ? strrchr(file, '\\') : 0;
		printf("raycast report_assert file=%s line=%d message=%s\n",
			base ? base + 1 : (file ? file : "null"), line, message ? message : "null");
		return NX_AR_CONTINUE;
		}
	virtual void print(const char* message)
		{
		printf("raycast print message=%s\n", message ? message : "null");
		}
	};

// ---------------------------------------------------------------------------
// The case label every line starts with.

static char nxLabel[160];

static void nxPrintHitDeclared(const NxRaycastHit& hit)
	{
	const NxU32 flags = hit.flags;
	printf(" flags=%08x", static_cast<unsigned>(flags));
	if(flags & NX_RAYCAST_SHAPE)
		{
		printf(" shape=");
		nxPrintWord(reinterpret_cast<NxU32>(hit.shape), true);
		}
	if(flags & NX_RAYCAST_IMPACT)
		printf(" impact=%08x.%08x.%08x", nxU(hit.worldImpact.x), nxU(hit.worldImpact.y), nxU(hit.worldImpact.z));
	if(flags & NX_RAYCAST_NORMAL)
		printf(" normal=%08x.%08x.%08x", nxU(hit.worldNormal.x), nxU(hit.worldNormal.y), nxU(hit.worldNormal.z));
	if(flags & NX_RAYCAST_FACE_INDEX)
		printf(" face=%08x", static_cast<unsigned>(hit.faceID));
	if(flags & NX_RAYCAST_DISTANCE)
		printf(" distance=%08x", nxU(hit.distance));
	if(flags & NX_RAYCAST_UV)
		printf(" uv=%08x.%08x", nxU(hit.u), nxU(hit.v));
	}

// A closest query's hit: the declared fields, and the distance word even when
// it is not declared (the query writes it first, FLT_MAX).
static void nxPrintClosestHit(const NxRaycastHit& hit)
	{
	nxPrintHitDeclared(hit);
	if(!(hit.flags & NX_RAYCAST_DISTANCE))
		printf(" distance_word=%08x", nxU(hit.distance));
	}

class NxRecordingReport : public NxUserRaycastReport
	{
	public:
	NxRecordingReport() : mCalls(0), mStopAfter(0) {}
	virtual bool onHit(const NxRaycastHit& hit)
		{
		mCalls++;
		const bool keepGoing = mStopAfter == 0 || mCalls < mStopAfter;
		printf("raycast %s on_hit=%u", nxLabel, mCalls);
		nxPrintHitDeclared(hit);
		printf(" return=%u\n", keepGoing ? 1u : 0u);
		return keepGoing;
		}
	unsigned mCalls;
	unsigned mStopAfter;
	};

// ---------------------------------------------------------------------------
// The scene.

struct NxShapeCase
	{
	const char* name;
	NxShapeType type;
	bool dynamic;
	NxVec3 position;
	NxVec3 size;				// box: dimensions; sphere: radius in x; capsule: radius, height
	NxU16 group;
	NxU32 flags;
	};

static const NxShapeCase nxShapeCases[] =
	{
	{ "s_box", NX_SHAPE_BOX, false, NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 1.0f, 1.0f), 1, NX_SF_VISUALIZATION },
	{ "s_sphere", NX_SHAPE_SPHERE, false, NxVec3(5.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f), 2, NX_SF_VISUALIZATION },
	{ "s_capsule", NX_SHAPE_CAPSULE, false, NxVec3(10.0f, 0.0f, 0.0f), NxVec3(0.5f, 2.0f, 0.0f), 3, NX_SF_VISUALIZATION },
	{ "s_plane", NX_SHAPE_PLANE, false, NxVec3(0.0f, 1.0f, 0.0f), NxVec3(-5.0f, 0.0f, 0.0f), 4, NX_SF_VISUALIZATION },
	{ "s_hidden", NX_SHAPE_BOX, false, NxVec3(15.0f, 0.0f, 0.0f), NxVec3(1.0f, 1.0f, 1.0f), 1, NX_SF_VISUALIZATION | NX_SF_DISABLE_RAYCASTING },
	{ "s_rotated", NX_SHAPE_BOX, false, NxVec3(20.0f, 0.0f, 0.0f), NxVec3(0.5f, 2.0f, 1.0f), 8, NX_SF_VISUALIZATION },
	{ "d_box", NX_SHAPE_BOX, true, NxVec3(0.0f, 0.0f, 5.0f), NxVec3(0.5f, 0.5f, 0.5f), 5, NX_SF_VISUALIZATION },
	{ "d_sphere", NX_SHAPE_SPHERE, true, NxVec3(5.0f, 0.0f, 5.0f), NxVec3(0.75f, 0.0f, 0.0f), 6, NX_SF_VISUALIZATION },
	{ "d_capsule", NX_SHAPE_CAPSULE, true, NxVec3(10.0f, 0.0f, 5.0f), NxVec3(0.5f, 1.0f, 0.0f), 7, NX_SF_VISUALIZATION },
	{ "d_rotated", NX_SHAPE_BOX, true, NxVec3(20.0f, 0.0f, 5.0f), NxVec3(1.0f, 0.5f, 0.25f), 9, NX_SF_VISUALIZATION },
	};
static const unsigned kShapeCaseCount = sizeof(nxShapeCases) / sizeof(nxShapeCases[0]);

static NxActor* nxActors[kShapeCaseCount];

static NxActor* nxCreateCase(NxScene* scene, unsigned index)
	{
	const NxShapeCase& c = nxShapeCases[index];
	NxBoxShapeDesc box;
	NxSphereShapeDesc sphere;
	NxCapsuleShapeDesc capsule;
	NxPlaneShapeDesc plane;
	NxShapeDesc* shape = 0;
	NxActorDesc actor;
	switch(c.type)
		{
		case NX_SHAPE_BOX:
			box.dimensions = c.size;
			shape = &box;
			break;
		case NX_SHAPE_SPHERE:
			sphere.radius = c.size.x;
			shape = &sphere;
			break;
		case NX_SHAPE_CAPSULE:
			capsule.radius = c.size.x;
			capsule.height = c.size.y;
			shape = &capsule;
			break;
		case NX_SHAPE_PLANE:
			plane.normal = c.position;
			plane.d = c.size.x;
			shape = &plane;
			break;
		default:
			return 0;
		}
	shape->group = c.group;
	shape->shapeFlags = c.flags;
	actor.shapes.pushBack(shape);
	if(c.type != NX_SHAPE_PLANE)
		actor.globalPose.t = c.position;
	if(strcmp(c.name, "s_rotated") == 0 || strcmp(c.name, "d_rotated") == 0)
		{
		// 0.6/0.8 rotation about z (exact in binary32 to the printed words).
		actor.globalPose.M.setRow(0, NxVec3(0.6f, -0.8f, 0.0f));
		actor.globalPose.M.setRow(1, NxVec3(0.8f, 0.6f, 0.0f));
		actor.globalPose.M.setRow(2, NxVec3(0.0f, 0.0f, 1.0f));
		}
	NxBodyDesc body;
	if(c.dynamic)
		{
		body.mass = 1.0f;
		body.massSpaceInertia = NxVec3(1.0f, 1.0f, 1.0f);
		actor.body = &body;
		}
	NxActor* created = scene->createActor(actor);
	printf("raycast create %s created=%u", c.name, created ? 1u : 0u);
	if(created)
		{
		printf(" shapes=%u", static_cast<unsigned>(created->getNbShapes()));
		NxShape* const* shapes = created->getShapes();
		if(created->getNbShapes() > 0)
			nxSymbolAdd(shapes[0], c.name);
		}
	printf("\n");
	return created;
	}

// ---------------------------------------------------------------------------
// The queries.

struct NxRayCase
	{
	const char* name;
	NxVec3 origin;
	NxVec3 direction;
	};

static const NxRayCase nxRayCases[] =
	{
	{ "x_statics", NxVec3(-5.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f) },
	{ "x_dynamics", NxVec3(-5.0f, 0.0f, 5.0f), NxVec3(1.0f, 0.0f, 0.0f) },
	{ "x_rotated", NxVec3(17.0f, 0.5f, 2.5f), NxVec3(1.0f, 0.0f, 0.0f) },
	{ "down_box", NxVec3(0.0f, 10.0f, 0.0f), NxVec3(0.0f, -1.0f, 0.0f) },
	{ "down_sphere", NxVec3(5.0f, 10.0f, 5.0f), NxVec3(0.0f, -1.0f, 0.0f) },
	{ "down_capsule", NxVec3(10.25f, 10.0f, 0.0f), NxVec3(0.0f, -1.0f, 0.0f) },
	{ "up_plane", NxVec3(3.0f, -10.0f, 3.0f), NxVec3(0.0f, 1.0f, 0.0f) },
	{ "inside_box", NxVec3(0.25f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f) },
	{ "inside_sphere", NxVec3(5.0f, 0.0f, 0.0f), NxVec3(0.6f, 0.8f, 0.0f) },
	{ "inside_capsule", NxVec3(10.0f, 0.0f, 5.0f), NxVec3(0.0f, 0.0f, 1.0f) },
	{ "diagonal", NxVec3(-3.0f, 0.0f, -4.0f), NxVec3(0.6f, 0.0f, 0.8f) },
	{ "slant", NxVec3(0.0f, 8.0f, -1.0f), NxVec3(0.0f, -0.8f, 0.6f) },
	{ "miss_high", NxVec3(-5.0f, 20.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f) },
	{ "miss_away", NxVec3(-5.0f, 0.0f, 0.0f), NxVec3(-1.0f, 0.0f, 0.0f) },
	{ "hidden", NxVec3(15.0f, 10.0f, 0.0f), NxVec3(0.0f, -1.0f, 0.0f) },
	};
static const unsigned kRayCaseCount = sizeof(nxRayCases) / sizeof(nxRayCases[0]);

static void nxQueryAll(NxScene* scene, const NxRay& ray, NxShapesType type, NxU32 groups, NxReal maxDist,
	NxU32 hint, const char* tag)
	{
	char base[160];
	sprintf(base, "%s type=%u groups=%08x max=%08x hint=%08x", tag, static_cast<unsigned>(type),
		static_cast<unsigned>(groups), nxU(maxDist), static_cast<unsigned>(hint));

	sprintf(nxLabel, "%s any_bounds", base);
	printf("raycast %s result=%u\n", nxLabel, scene->raycastAnyBounds(ray, type, groups, maxDist) ? 1u : 0u);
	sprintf(nxLabel, "%s any_shape", base);
	printf("raycast %s result=%u\n", nxLabel, scene->raycastAnyShape(ray, type, groups, maxDist) ? 1u : 0u);

	NxRaycastHit hit;
	memset(&hit, 0xcd, sizeof(hit));
	sprintf(nxLabel, "%s closest_bounds", base);
	NxShape* shape = scene->raycastClosestBounds(ray, type, hit, groups, maxDist, hint);
	printf("raycast %s result=", nxLabel);
	nxPrintWord(reinterpret_cast<NxU32>(shape), true);
	nxPrintClosestHit(hit);
	printf("\n");

	memset(&hit, 0xcd, sizeof(hit));
	sprintf(nxLabel, "%s closest_shape", base);
	shape = scene->raycastClosestShape(ray, type, hit, groups, maxDist, hint);
	printf("raycast %s result=", nxLabel);
	nxPrintWord(reinterpret_cast<NxU32>(shape), true);
	nxPrintClosestHit(hit);
	printf("\n");

	NxRecordingReport report;
	sprintf(nxLabel, "%s all_bounds", base);
	NxU32 count = scene->raycastAllBounds(ray, report, type, groups, maxDist, hint);
	printf("raycast %s result=%u calls=%u\n", nxLabel, static_cast<unsigned>(count), report.mCalls);

	NxRecordingReport shapes;
	sprintf(nxLabel, "%s all_shapes", base);
	count = scene->raycastAllShapes(ray, shapes, type, groups, maxDist, hint);
	printf("raycast %s result=%u calls=%u\n", nxLabel, static_cast<unsigned>(count), shapes.mCalls);
	}

static void nxQueryStops(NxScene* scene, const NxRay& ray, const char* tag)
	{
	for(unsigned stop = 1; stop <= 2; stop++)
		{
		NxRecordingReport report;
		report.mStopAfter = stop;
		sprintf(nxLabel, "%s stop=%u all_bounds", tag, stop);
		NxU32 count = scene->raycastAllBounds(ray, report, NX_ALL_SHAPES);
		printf("raycast %s result=%u calls=%u\n", nxLabel, static_cast<unsigned>(count), report.mCalls);
		NxRecordingReport shapes;
		shapes.mStopAfter = stop;
		sprintf(nxLabel, "%s stop=%u all_shapes", tag, stop);
		count = scene->raycastAllShapes(ray, shapes, NX_ALL_SHAPES);
		printf("raycast %s result=%u calls=%u\n", nxLabel, static_cast<unsigned>(count), shapes.mCalls);
		}
	}

// ---------------------------------------------------------------------------
// Box resize (scene-raycast Task 4, box hull): NxBoxShape::setDimensions
// (001069 -> 000983 -> the hull rebuild 000973) on a static, a dynamic and a
// rotated dynamic box created after every query above, then the public
// queries that read the new size (getDimensions, getWorldBounds, raycasts),
// the internal shape's hull words (+0xe0..+0x227) and its facade's slots 1,
// 3, 9 and 10 (000957/000959) called through the table at +0xe0, and the
// scene's dirty word for the shape (001325's flags[shape id]).
//
// The internal shape is the handle's +0x18 word (001069 at 0x10023500). The
// hull's list pointers name the image's .rdata index lists, so the four
// words each points at are printed instead; the facade table's address is
// printed only as present or absent.

typedef NxU32 (__thiscall *NxFacadeCountFn)(const void*);
typedef NxU32 (__thiscall *NxFacadeSupportFn)(const void*, const float*, const float*);
typedef NxU32 (__thiscall *NxFacadeFeatureFn)(const void*, const float*, const float*, NxU32*);

static const unsigned char* nxInternalShape(const NxShape* shape)
	{
	return *reinterpret_cast<unsigned char* const*>(reinterpret_cast<const unsigned char*>(shape) + 0x18);
	}

static void nxPrintWords(const void* words, unsigned count)
	{
	const NxU32* w = static_cast<const NxU32*>(words);
	for(unsigned i = 0; i < count; i++)
		printf("%s%08x", i ? "." : "", static_cast<unsigned>(w[i]));
	}

static void nxPrintHull(const char* name, const char* when, const NxShape* shape)
	{
	const unsigned char* internal = nxInternalShape(shape);
	void* const* table = *reinterpret_cast<void* const* const*>(internal + 0xe0);
	printf("box_resize %s %s facade=%u dims=", name, when, table ? 1u : 0u);
	nxPrintWords(internal + 0xe4, 3);
	printf("\n");
	printf("box_resize %s %s vertices=", name, when);
	nxPrintWords(internal + 0xf0, 24);
	printf("\n");
	for(unsigned f = 0; f < 6; f++)
		{
		const unsigned char* face = internal + 0x150 + f * 0x24;
		const NxU32* listA = *reinterpret_cast<const NxU32* const*>(face + 4);
		const NxU32* listB = *reinterpret_cast<const NxU32* const*>(face + 8);
		printf("box_resize %s %s face=%u corners=", name, when, f);
		nxPrintWords(face, 1);
		printf(" list_a=");
		if(listA)
			nxPrintWords(listA, 4);
		else
			printf("null");
		printf(" list_b=");
		if(listB)
			nxPrintWords(listB, 4);
		else
			printf("null");
		printf(" plane=");
		nxPrintWords(face + 0x0c, 4);
		printf(" range=");
		nxPrintWords(face + 0x1c, 2);
		printf("\n");
		}
	if(!table)
		return;
	const void* facade = internal + 0xe0;
	printf("box_resize %s %s slot1=%u slot3=%u\n", name, when,
		static_cast<unsigned>(reinterpret_cast<NxFacadeCountFn>(table[1])(facade)),
		static_cast<unsigned>(reinterpret_cast<NxFacadeCountFn>(table[3])(facade)));
	static const float directions[][3] =
		{
		{ 1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0f },
		{ 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }, { 0.6f, 0.8f, 0.0f }, { -0.6f, 0.0f, 0.8f },
		{ 0.577f, 0.577f, 0.577f }, { 0.3f, -0.4f, 0.1f }, { 1.0f, 1.0f, 0.0f }, { 0.0f, -2.0f, 3.0f },
		};
	// Words 0-2, 4-6 and 8-10 are the rotation rows the slots read.
	static const float pose[12] =
		{ 0.6f, -0.8f, 0.0f, 0.0f, 0.8f, 0.6f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f };
	for(unsigned d = 0; d < sizeof(directions) / sizeof(directions[0]); d++)
		for(unsigned p = 0; p < 2; p++)
			{
			const float* posePointer = p ? pose : 0;
			NxU32 feature = 0xcdcdcdcdu;
			const NxU32 face = reinterpret_cast<NxFacadeSupportFn>(table[9])(facade, directions[d], posePointer);
			const NxU32 featureFace = reinterpret_cast<NxFacadeFeatureFn>(table[10])(facade, directions[d],
				posePointer, &feature);
			const NxU32 bareFace = reinterpret_cast<NxFacadeFeatureFn>(table[10])(facade, directions[d],
				posePointer, 0);
			printf("box_resize %s %s support dir=%u pose=%u face=%u feature_face=%u feature=%08x bare=%u\n", name,
				when, d, p, static_cast<unsigned>(face), static_cast<unsigned>(featureFace),
				static_cast<unsigned>(feature), static_cast<unsigned>(bareFace));
			}
	}

// flags[shape id] of the scene's shape-update list, the word 001325
// (0x10026c90) ORs its bit into: shape+4 -> +4 -> +0x48 -> [0], index +0xd4.
static void nxPrintDirty(const char* name, const char* when, const NxShape* shape)
	{
	const unsigned char* internal = nxInternalShape(shape);
	const unsigned char* owner = *reinterpret_cast<unsigned char* const*>(internal + 4);
	const unsigned char* sceneWords = owner ? *reinterpret_cast<unsigned char* const*>(owner + 4) : 0;
	const unsigned char* list = sceneWords ? *reinterpret_cast<unsigned char* const*>(sceneWords + 0x48) : 0;
	const NxU32* flags = list ? *reinterpret_cast<NxU32* const*>(list) : 0;
	printf("box_resize %s %s dirty=", name, when);
	if(flags)
		printf("%08x\n", static_cast<unsigned>(flags[*reinterpret_cast<const NxU32*>(internal + 0xd4)]));
	else
		printf("none\n");
	}

static void nxPrintBoxPublic(const char* name, const char* when, NxShape* shape)
	{
	NxBoxShape* box = shape->isBox();
	printf("box_resize %s %s is_box=%u", name, when, box ? 1u : 0u);
	if(box)
		{
		const NxVec3 dims = box->getDimensions();
		printf(" get_dims=%08x.%08x.%08x", nxU(dims.x), nxU(dims.y), nxU(dims.z));
		}
	NxBounds3 bounds;
	shape->getWorldBounds(bounds);
	printf(" world_bounds=%08x.%08x.%08x.%08x.%08x.%08x\n", nxU(bounds.getMin().x), nxU(bounds.getMin().y),
		nxU(bounds.getMin().z), nxU(bounds.getMax().x), nxU(bounds.getMax().y), nxU(bounds.getMax().z));
	}

struct NxResizeCase
	{
	const char* name;
	bool dynamic;
	bool rotated;
	NxVec3 position;
	NxVec3 created;
	NxVec3 grown;
	NxVec3 shrunk;
	NxU16 group;
	};

static void nxResizeQueries(NxScene* scene, const char* when)
	{
	static const NxRayCase rays[] =
		{
		{ "x_resize_s", NxVec3(35.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f) },
		{ "x_resize_d", NxVec3(35.0f, 0.0f, 5.0f), NxVec3(1.0f, 0.0f, 0.0f) },
		{ "x_resize_r", NxVec3(35.0f, 0.25f, 10.0f), NxVec3(1.0f, 0.0f, 0.0f) },
		{ "down_resize_s", NxVec3(41.5f, 10.0f, 0.0f), NxVec3(0.0f, -1.0f, 0.0f) },
		{ "down_resize_d", NxVec3(40.75f, 10.0f, 5.0f), NxVec3(0.0f, -1.0f, 0.0f) },
		{ "down_resize_r", NxVec3(40.0f, 10.0f, 11.0f), NxVec3(0.0f, -1.0f, 0.0f) },
		{ "z_resize", NxVec3(40.25f, 0.0f, -5.0f), NxVec3(0.0f, 0.0f, 1.0f) },
		};
	static const NxShapesType types[3] = { NX_STATIC_SHAPES, NX_DYNAMIC_SHAPES, NX_ALL_SHAPES };
	char tag[96];
	for(unsigned r = 0; r < sizeof(rays) / sizeof(rays[0]); r++)
		{
		sprintf(tag, "%s ray=%s", when, rays[r].name);
		const NxRay ray(rays[r].origin, rays[r].direction);
		for(unsigned t = 0; t < 3; t++)
			nxQueryAll(scene, ray, types[t], (1u << 13) | (1u << 14) | (1u << 15), NX_MAX_F32, 0xffffffffu, tag);
		nxQueryAll(scene, ray, NX_ALL_SHAPES, 0xffffffffu, 2.0f, 0xffffffffu, tag);
		}
	}

static void nxResizeCases(NxScene* scene)
	{
	static const NxResizeCase cases[] =
		{
		{ "s_resize", false, false, NxVec3(40.0f, 0.0f, 0.0f), NxVec3(1.0f, 1.0f, 1.0f),
			NxVec3(2.0f, 0.5f, 3.0f), NxVec3(0.25f, 1.5f, 0.125f), 13 },
		{ "d_resize", true, false, NxVec3(40.0f, 0.0f, 5.0f), NxVec3(0.5f, 0.5f, 0.5f),
			NxVec3(1.0f, 2.0f, 0.75f), NxVec3(0.1f, 0.2f, 0.3f), 14 },
		{ "r_resize", true, true, NxVec3(40.0f, 0.0f, 10.0f), NxVec3(1.0f, 0.5f, 0.25f),
			NxVec3(3.0f, 0.5f, 1.5f), NxVec3(0.5f, 3.0f, 0.5f), 15 },
		};
	static const unsigned kCount = sizeof(cases) / sizeof(cases[0]);
	NxActor* actors[kCount];
	NxShape* shapes[kCount];
	for(unsigned i = 0; i < kCount; i++)
		{
		const NxResizeCase& c = cases[i];
		NxBoxShapeDesc box;
		box.dimensions = c.created;
		box.group = c.group;
		NxActorDesc actor;
		actor.shapes.pushBack(&box);
		actor.globalPose.t = c.position;
		if(c.rotated)
			{
			actor.globalPose.M.setRow(0, NxVec3(0.6f, -0.8f, 0.0f));
			actor.globalPose.M.setRow(1, NxVec3(0.8f, 0.6f, 0.0f));
			actor.globalPose.M.setRow(2, NxVec3(0.0f, 0.0f, 1.0f));
			}
		NxBodyDesc body;
		if(c.dynamic)
			{
			body.mass = 1.0f;
			body.massSpaceInertia = NxVec3(1.0f, 1.0f, 1.0f);
			actor.body = &body;
			}
		actors[i] = scene->createActor(actor);
		shapes[i] = actors[i] && actors[i]->getNbShapes() == 1 ? actors[i]->getShapes()[0] : 0;
		printf("box_resize create %s created=%u shape=%u\n", c.name, actors[i] ? 1u : 0u, shapes[i] ? 1u : 0u);
		if(shapes[i])
			{
			nxSymbolSet(shapes[i], c.name);
			nxPrintBoxPublic(c.name, "created", shapes[i]);
			nxPrintHull(c.name, "created", shapes[i]);
			nxPrintDirty(c.name, "created", shapes[i]);
			}
		}
	nxResizeQueries(scene, "box_resize created");

	for(unsigned step = 0; step < 2; step++)
		{
		const char* when = step ? "shrunk" : "grown";
		for(unsigned i = 0; i < kCount; i++)
			{
			if(!shapes[i] || !shapes[i]->isBox())
				continue;
			shapes[i]->isBox()->setDimensions(step ? cases[i].shrunk : cases[i].grown);
			nxPrintBoxPublic(cases[i].name, when, shapes[i]);
			nxPrintHull(cases[i].name, when, shapes[i]);
			nxPrintDirty(cases[i].name, when, shapes[i]);
			}
		char tag[64];
		sprintf(tag, "box_resize %s", when);
		nxResizeQueries(scene, tag);
		}

	for(unsigned i = 0; i < kCount; i++)
		if(actors[i])
			scene->releaseActor(*actors[i]);
	}

int wmain(int argc, wchar_t** argv)
	{
	setvbuf(stdout, 0, _IONBF, 0);
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsSceneRaycastTests", pairDirectory, &physics);
	if(status)
		return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK)
		return nxFail("NxCreatePhysicsSDK is missing");
	static NxPageGuardedAllocator allocator;
	static NxRecordingOutput output;
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &allocator, &output);
	if(!sdk)
		return nxFail("SDK creation failed");
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene)
		return nxFail("scene creation failed");

	for(unsigned i = 0; i < kShapeCaseCount; i++)
		nxActors[i] = nxCreateCase(scene, i);

	// Two compound actors, one static and one dynamic: a box and a sphere each.
	// The pool keeps a compound's shapes in section 0 and its group shape in
	// section 2; the raycasts walk sections 0 and 1.
	NxActor* compounds[2] = { 0, 0 };
	for(unsigned dynamicCompound = 0; dynamicCompound < 2; dynamicCompound++)
		{
		NxBoxShapeDesc box;
		box.dimensions = NxVec3(0.5f, 0.5f, 0.5f);
		box.group = 10;
		NxSphereShapeDesc sphere;
		sphere.radius = 0.5f;
		sphere.localPose.t = NxVec3(0.0f, 2.0f, 0.0f);
		sphere.group = 11;
		NxActorDesc actor;
		actor.shapes.pushBack(&box);
		actor.shapes.pushBack(&sphere);
		actor.globalPose.t = NxVec3(30.0f, 0.0f, dynamicCompound ? 5.0f : 0.0f);
		NxBodyDesc body;
		body.mass = 1.0f;
		body.massSpaceInertia = NxVec3(1.0f, 1.0f, 1.0f);
		if(dynamicCompound)
			actor.body = &body;
		compounds[dynamicCompound] = scene->createActor(actor);
		printf("raycast create %s created=%u", dynamicCompound ? "d_compound" : "s_compound",
			compounds[dynamicCompound] ? 1u : 0u);
		if(compounds[dynamicCompound])
			{
			printf(" shapes=%u", static_cast<unsigned>(compounds[dynamicCompound]->getNbShapes()));
			NxShape* const* shapes = compounds[dynamicCompound]->getShapes();
			nxSymbolAdd(shapes[0], dynamicCompound ? "dc_box" : "sc_box");
			nxSymbolAdd(shapes[1], dynamicCompound ? "dc_sphere" : "sc_sphere");
			}
		printf("\n");
		}

	// Every ray, every shapes type, all groups, no distance limit, every hint.
	static const NxShapesType types[3] = { NX_STATIC_SHAPES, NX_DYNAMIC_SHAPES, NX_ALL_SHAPES };
	char tag[96];
	for(unsigned r = 0; r < kRayCaseCount; r++)
		{
		const NxRay ray(nxRayCases[r].origin, nxRayCases[r].direction);
		sprintf(tag, "ray=%s", nxRayCases[r].name);
		for(unsigned t = 0; t < 3; t++)
			nxQueryAll(scene, ray, types[t], 0xffffffffu, NX_MAX_F32, 0xffffffffu, tag);
		}

	// The compounds.
	{
		const NxRay across(NxVec3(25.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f));
		const NxRay acrossHigh(NxVec3(25.0f, 2.0f, 5.0f), NxVec3(1.0f, 0.0f, 0.0f));
		const NxRay down(NxVec3(30.0f, 10.0f, 5.0f), NxVec3(0.0f, -1.0f, 0.0f));
		for(unsigned t = 0; t < 3; t++)
			{
			nxQueryAll(scene, across, types[t], 0xffffffffu, NX_MAX_F32, 0xffffffffu, "ray=compound_across");
			nxQueryAll(scene, acrossHigh, types[t], 0xffffffffu, NX_MAX_F32, 0xffffffffu, "ray=compound_high");
			nxQueryAll(scene, down, types[t], 0xffffffffu, NX_MAX_F32, 0xffffffffu, "ray=compound_down");
			}
		nxQueryAll(scene, down, NX_ALL_SHAPES, 1u << 11, NX_MAX_F32, 0xffffffffu, "ray=compound_down");
	}

	// Collision groups. Group bits: box 1, sphere 2, capsule 3, plane 4, the
	// dynamic box 5, sphere 6, capsule 7, the rotated boxes 8 and 9.
	static const NxU32 groupMasks[] = { 0u, 1u << 1, (1u << 2) | (1u << 3), (1u << 5) | (1u << 7) | (1u << 9),
		~((1u << 1) | (1u << 6)), 1u << 4 };
	static const unsigned groupRays[] = { 0, 1, 3, 4, 10 };
	for(unsigned g = 0; g < sizeof(groupMasks) / sizeof(groupMasks[0]); g++)
		for(unsigned r = 0; r < sizeof(groupRays) / sizeof(groupRays[0]); r++)
			{
			const NxRayCase& c = nxRayCases[groupRays[r]];
			sprintf(tag, "ray=%s", c.name);
			nxQueryAll(scene, NxRay(c.origin, c.direction), NX_ALL_SHAPES, groupMasks[g], NX_MAX_F32,
				0xffffffffu, tag);
			}

	// Maximum distances: finite limits take the dynamic pruner's segment test
	// and cut the hits short, including limits that end inside a shape or its
	// box, and exactly on a face.
	static const NxReal distances[] = { 20.0f, 9.5f, 7.0f, 4.0f, 3.9f, 1.0f, 0.25f };
	static const unsigned distanceRays[] = { 0, 1, 3, 4, 7, 10, 11 };
	for(unsigned d = 0; d < sizeof(distances) / sizeof(distances[0]); d++)
		for(unsigned r = 0; r < sizeof(distanceRays) / sizeof(distanceRays[0]); r++)
			{
			const NxRayCase& c = nxRayCases[distanceRays[r]];
			sprintf(tag, "ray=%s", c.name);
			nxQueryAll(scene, NxRay(c.origin, c.direction), NX_ALL_SHAPES, 0xffffffffu, distances[d],
				0xffffffffu, tag);
			}

	// Hint flags.
	static const NxU32 hints[] = { 0u, NX_RAYCAST_NORMAL, NX_RAYCAST_IMPACT, NX_RAYCAST_SHAPE | NX_RAYCAST_DISTANCE,
		NX_RAYCAST_NORMAL | NX_RAYCAST_FACE_INDEX | NX_RAYCAST_UV };
	static const unsigned hintRays[] = { 0, 1, 2, 3, 6, 8, 11 };
	for(unsigned h = 0; h < sizeof(hints) / sizeof(hints[0]); h++)
		for(unsigned r = 0; r < sizeof(hintRays) / sizeof(hintRays[0]); r++)
			{
			const NxRayCase& c = nxRayCases[hintRays[r]];
			sprintf(tag, "ray=%s", c.name);
			nxQueryAll(scene, NxRay(c.origin, c.direction), NX_ALL_SHAPES, 0xffffffffu, NX_MAX_F32, hints[h], tag);
			}

	// Reports that stop the query.
	for(unsigned r = 0; r < 4; r++)
		{
		static const unsigned stopRays[4] = { 0, 1, 3, 11 };
		const NxRayCase& c = nxRayCases[stopRays[r]];
		sprintf(tag, "ray=%s", c.name);
		nxQueryStops(scene, NxRay(c.origin, c.direction), tag);
		}

	// The error paths: a direction that is not a unit vector (each row's own
	// line), and a maximum distance that is not positive (each wrapper's).
	{
		const NxRay slanted(NxVec3(-5.0f, 0.0f, 0.0f), NxVec3(1.0f, 1.0f, 0.0f));
		const NxRay zero(NxVec3(-5.0f, 0.0f, 0.0f), NxVec3(0.0f, 0.0f, 0.0f));
		const NxRay nearlyUnit(NxVec3(-5.0f, 0.0f, 0.0f), NxVec3(1.00004f, 0.0f, 0.0f));
		const NxRay offUnit(NxVec3(-5.0f, 0.0f, 0.0f), NxVec3(1.0001f, 0.0f, 0.0f));
		nxQueryAll(scene, slanted, NX_ALL_SHAPES, 0xffffffffu, NX_MAX_F32, 0xffffffffu, "ray=non_unit");
		nxQueryAll(scene, zero, NX_ALL_SHAPES, 0xffffffffu, NX_MAX_F32, 0xffffffffu, "ray=zero_dir");
		nxQueryAll(scene, nearlyUnit, NX_ALL_SHAPES, 0xffffffffu, NX_MAX_F32, 0xffffffffu, "ray=nearly_unit");
		nxQueryAll(scene, offUnit, NX_ALL_SHAPES, 0xffffffffu, NX_MAX_F32, 0xffffffffu, "ray=off_unit");
		const NxRay good(nxRayCases[0].origin, nxRayCases[0].direction);
		nxQueryAll(scene, good, NX_ALL_SHAPES, 0xffffffffu, 0.0f, 0xffffffffu, "ray=x_statics");
		nxQueryAll(scene, good, NX_ALL_SHAPES, 0xffffffffu, -1.0f, 0xffffffffu, "ray=x_statics");
	}

	// Moving a shape marks its pruner box stale through the pruner's slot 3:
	// the dynamic pruner recomputes the box on the next query, the static
	// pruner drops its tree and rebuilds it.
	nxActors[7]->setGlobalPosition(NxVec3(5.0f, 1.5f, 5.0f));
	nxActors[0]->getShapes()[0]->setLocalPosition(NxVec3(0.0f, 0.0f, 3.0f));
	printf("raycast moved d_sphere s_box\n");
	static const unsigned movedRays[] = { 0, 1, 3, 4 };
	for(unsigned r = 0; r < sizeof(movedRays) / sizeof(movedRays[0]); r++)
		{
		const NxRayCase& c = nxRayCases[movedRays[r]];
		sprintf(tag, "moved ray=%s", c.name);
		nxQueryAll(scene, NxRay(c.origin, c.direction), NX_ALL_SHAPES, 0xffffffffu, NX_MAX_F32, 0xffffffffu, tag);
		}
	{
		// Only the moved static box is on this ray, and only at its new place:
		// a tree built before the move does not have it there.
		const NxRay atNewPlace(NxVec3(-5.0f, 0.0f, 3.0f), NxVec3(1.0f, 0.0f, 0.0f));
		for(unsigned t = 0; t < 3; t++)
			nxQueryAll(scene, atNewPlace, types[t], 0xffffffffu, NX_MAX_F32, 0xffffffffu, "moved ray=x_moved_box");
		nxQueryAll(scene, atNewPlace, NX_ALL_SHAPES, 0xffffffffu, 10.0f, 0xffffffffu, "moved ray=x_moved_box");
	}

	// Releasing actors removes their shapes from the pools (the static pruner
	// rebuilds its tree on the next query); query again.
	scene->releaseActor(*nxActors[1]);
	nxActors[1] = 0;
	scene->releaseActor(*nxActors[6]);
	nxActors[6] = 0;
	scene->releaseActor(*compounds[0]);
	compounds[0] = 0;
	printf("raycast released s_sphere d_box s_compound\n");
	static const unsigned afterRays[] = { 0, 1, 3, 4, 10 };
	for(unsigned r = 0; r < sizeof(afterRays) / sizeof(afterRays[0]); r++)
		{
		const NxRayCase& c = nxRayCases[afterRays[r]];
		sprintf(tag, "after_release ray=%s", c.name);
		for(unsigned t = 0; t < 3; t++)
			nxQueryAll(scene, NxRay(c.origin, c.direction), types[t], 0xffffffffu, NX_MAX_F32, 0xffffffffu, tag);
		}
	{
		const NxRay across(NxVec3(25.0f, 0.0f, 0.0f), NxVec3(1.0f, 0.0f, 0.0f));
		const NxRay acrossHigh(NxVec3(25.0f, 2.0f, 5.0f), NxVec3(1.0f, 0.0f, 0.0f));
		nxQueryAll(scene, across, NX_ALL_SHAPES, 0xffffffffu, NX_MAX_F32, 0xffffffffu, "after_release ray=compound_across");
		nxQueryAll(scene, acrossHigh, NX_ALL_SHAPES, 0xffffffffu, NX_MAX_F32, 0xffffffffu, "after_release ray=compound_high");
	}

	// A static actor created after the tree was built.
	{
		NxBoxShapeDesc box;
		box.dimensions = NxVec3(0.5f, 0.5f, 0.5f);
		box.group = 12;
		NxActorDesc actor;
		actor.shapes.pushBack(&box);
		actor.globalPose.t = NxVec3(2.5f, 0.0f, 0.0f);
		NxActor* late = scene->createActor(actor);
		printf("raycast create s_late created=%u\n", late ? 1u : 0u);
		if(late)
			nxSymbolAdd(late->getShapes()[0], "s_late");
		const NxRay ray(nxRayCases[0].origin, nxRayCases[0].direction);
		for(unsigned t = 0; t < 3; t++)
			nxQueryAll(scene, ray, types[t], 0xffffffffu, NX_MAX_F32, 0xffffffffu, "late ray=x_statics");
		if(late)
			scene->releaseActor(*late);
	}

	nxResizeCases(scene);

	for(unsigned i = 0; i < kShapeCaseCount; i++)
		if(nxActors[i])
			scene->releaseActor(*nxActors[i]);
	if(compounds[1])
		scene->releaseActor(*compounds[1]);
	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
	}
