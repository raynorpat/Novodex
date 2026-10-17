// The scene visualisation staged-pair differential (scene-raycast block,
// Task 4, visualisation sub-area). Phase 7 staged-pair target.
//
// Drives NxScene::visualize (NpScene slot 31 -> 000344 -> 000657) over a
// scene of static and dynamic actors, and prints what an
// NxUserDebugRenderer receives from NxPhysicsSDK::visualize (the Foundation's
// renderDebugData over every renderable it holds; the Scene's own renderable
// is created by 000579 on the first visualize with a non-zero
// NX_VISUALIZATION_SCALE). Every point, line and triangle is printed as its
// float words and colour.
//
// The parameter stages select the rows' branches one at a time, then
// together: 000657's world axes; 000020's actor axes (every actor, static and
// dynamic); 000766's body axes, inertia box (addOBB), linear and angular
// velocity arrows (the bodies carry non-zero velocities and a non-uniform
// inertia), and a body without NX_BF_VISUALIZATION; NX_VISUALIZE_BODY_JOINT_GROUPS
// runs 000766's island test (+0x1e0 is 0 on both sides before a simulation
// step, so 004163 is not reached). Contact-pair visualization has a separate
// 60-step plane/sphere scene so rows 000907 and 000869 see a live pair.
// Fluid visualization remains disabled because its manager is not created.
//
// Then the scale goes back to 0 (000657 clears the renderable and draws
// nothing), bodies move, and after releaseScene the renderer must receive no
// renderable (000663 releases it through the Foundation).
//
// Every body is given its mass and massSpaceInertia: the creation path
// (Scene.cpp nxActorBuildRecord, 000026) derives mass and inertia from the
// shapes (000008) only when the tensor is all zero, so giving both keeps
// these rows' output independent of that path.

#include "PhysicsPairLoader.h"
#include "NxPageGuardedAllocator.h"

#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxBounds3.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxPlaneShapeDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxCapsuleShapeDesc.h"
#include "NxUserOutputStream.h"
#include "NxUserContactReport.h"
#include "NxDebugRenderable.h"
#include "NxUserDebugRenderer.h"
#include "NxRay.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

static NxU32 nxU(NxReal value)
	{
	NxU32 bits;
	memcpy(&bits, &value, 4);
	return bits;
	}

class NxRecordingOutput : public NxUserOutputStream
	{
	public:
	virtual void reportError(NxErrorCode code, const char* message, const char* file, int line)
		{
		const char* base = file ? strrchr(file, '\\') : 0;
		printf("visualize report_error code=%d file=%s line=%d message=%s\n", static_cast<int>(code),
			base ? base + 1 : (file ? file : "null"), line, message ? message : "null");
		}
	virtual NxAssertResponse reportAssertViolation(const char* message, const char* file, int line)
		{
		const char* base = file ? strrchr(file, '\\') : 0;
		printf("visualize report_assert file=%s line=%d message=%s\n",
			base ? base + 1 : (file ? file : "null"), line, message ? message : "null");
		return NX_AR_CONTINUE;
		}
	virtual void print(const char* message)
		{
		printf("visualize print message=%s\n", message ? message : "null");
		}
	};

static const char* nxStage = "";
static bool nxFreePrunerBlueAabbsObserved = false;

static void nxPrintVec(const char* name, const NxVec3& v)
	{
	printf(" %s=%08x.%08x.%08x", name, nxU(v.x), nxU(v.y), nxU(v.z));
	}

class NxRecordingRenderer : public NxUserDebugRenderer
	{
	public:
	NxRecordingRenderer() : mCalls(0), mLines(0), mBlueLines(0) {}
	virtual void renderData(const NxDebugRenderable& data) const
		{
		mCalls++;
		const NxU32 points = data.getNbPoints();
		const NxU32 lines = data.getNbLines();
		mLines += lines;
		const NxU32 triangles = data.getNbTriangles();
		printf("visualize %s renderable=%u points=%u lines=%u triangles=%u\n", nxStage, mCalls,
			static_cast<unsigned>(points), static_cast<unsigned>(lines), static_cast<unsigned>(triangles));
		const NxDebugPoint* p = data.getPoints();
		for(NxU32 i = 0; i < points; i++)
			{
			printf("visualize %s point=%u", nxStage, static_cast<unsigned>(i));
			nxPrintVec("p", p[i].p);
			printf(" color=%08x\n", static_cast<unsigned>(p[i].color));
			}
		const NxDebugLine* l = data.getLines();
		for(NxU32 i = 0; i < lines; i++)
			{
			if(l[i].color == NX_ARGB_BLUE)
				++mBlueLines;
			printf("visualize %s line=%u", nxStage, static_cast<unsigned>(i));
			nxPrintVec("p0", l[i].p0);
			nxPrintVec("p1", l[i].p1);
			printf(" color=%08x\n", static_cast<unsigned>(l[i].color));
			}
		const NxDebugTriangle* t = data.getTriangles();
		for(NxU32 i = 0; i < triangles; i++)
			{
			printf("visualize %s triangle=%u", nxStage, static_cast<unsigned>(i));
			nxPrintVec("p0", t[i].p0);
			nxPrintVec("p1", t[i].p1);
			nxPrintVec("p2", t[i].p2);
			printf(" color=%08x\n", static_cast<unsigned>(t[i].color));
			}
		}
	mutable unsigned mCalls;
	mutable unsigned mLines;
	mutable unsigned mBlueLines;
	};

// The visualisation parameters the stages set; everything else stays at its
// default (0).
static const NxParameter nxVisParameters[] =
	{
	NX_VISUALIZATION_SCALE, NX_VISUALIZE_WORLD_AXES, NX_VISUALIZE_ACTOR_AXES, NX_VISUALIZE_BODY_AXES,
	NX_VISUALIZE_BODY_MASS_AXES, NX_VISUALIZE_BODY_LIN_VELOCITY, NX_VISUALIZE_BODY_ANG_VELOCITY,
	NX_VISUALIZE_BODY_JOINT_GROUPS, NX_VISUALIZE_COLLISION_SHAPES, NX_VISUALIZE_COLLISION_AABBS,
	NX_VISUALIZE_COLLISION_COMPOUNDS, NX_VISUALIZE_COLLISION_SAP, NX_VISUALIZE_COLLISION_STATIC,
	NX_VISUALIZE_COLLISION_DYNAMIC, NX_VISUALIZE_COLLISION_FREE
	};
static const unsigned kVisParameterCount = sizeof(nxVisParameters) / sizeof(nxVisParameters[0]);

struct NxStage
	{
	const char* name;
	NxReal values[15];		// in nxVisParameters' order
	};

static const NxStage nxStages[] =
	{
	{ "scale_zero", { 0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f } },
	{ "scale_only", { 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f } },
	{ "world_axes", { 2.0f, 1.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f } },
	{ "actor_axes", { 2.0f, 0.0f, 1.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f } },
	{ "body_axes", { 2.0f, 0.0f, 0.0f, 0.75f, 0.0f, 0.0f, 0.0f, 0.0f } },
	{ "mass_axes", { 2.0f, 0.0f, 0.0f, 0.0f, 0.75f, 0.0f, 0.0f, 0.0f } },
	{ "lin_velocity", { 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f, 0.0f, 0.0f } },
	{ "ang_velocity", { 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.7f, 0.0f } },
	{ "joint_groups", { 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f } },
	{ "collision_shapes", { 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f } },
	{ "collision_aabbs", { 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f } },
	{ "collision_sap", { 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f } },
	{ "collision_static", { 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f } },
	{ "collision_dynamic", { 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f } },
	{ "collision_free", { 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f } },
	{ "all", { 0.5f, 1.0f, 2.0f, 1.25f, 3.0f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f } },
	};
static const unsigned kStageCount = sizeof(nxStages) / sizeof(nxStages[0]);

static void nxRunStage(NxPhysicsSDK* sdk, NxScene* scene, const NxStage& stage, const char* tag)
	{
	char label[96];
	sprintf(label, "%s stage=%s", tag, stage.name);
	nxStage = label;
	for(unsigned i = 0; i < kVisParameterCount; i++)
		sdk->setParameter(nxVisParameters[i], stage.values[i]);
	if(strcmp(stage.name, "bounded_dynamic") == 0)
		printf("visualize %s parameters sap=%g static=%g dynamic=%g free=%g\n", label,
			sdk->getParameter(NX_VISUALIZE_COLLISION_SAP),
			sdk->getParameter(NX_VISUALIZE_COLLISION_STATIC),
			sdk->getParameter(NX_VISUALIZE_COLLISION_DYNAMIC),
			sdk->getParameter(NX_VISUALIZE_COLLISION_FREE));
	scene->visualize();
	NxRecordingRenderer renderer;
	sdk->visualize(renderer);
	printf("visualize %s renderables=%u\n", label, renderer.mCalls);
	if(strcmp(stage.name, "collision_free") == 0)
		{
		nxFreePrunerBlueAabbsObserved = renderer.mBlueLines == 60;
		printf("visualize row001937 free_pruner_blue_lines=%u expected=60 matched=%u\n",
			renderer.mBlueLines, nxFreePrunerBlueAabbsObserved ? 1u : 0u);
		}
	nxStage = "";
	}

struct NxActorCase
	{
	const char* name;
	NxShapeType type;
	bool dynamic;
	bool visualization;		// NX_BF_VISUALIZATION on the body
	bool rotated;
	NxVec3 position;
	NxVec3 size;			// box: dimensions; sphere: radius in x; capsule: radius, height
	NxReal mass;
	NxVec3 inertia;			// massSpaceInertia, always given (see below)
	NxVec3 linear;
	NxVec3 angular;
	};

static const NxActorCase nxActorCases[] =
	{
	{ "s_box", NX_SHAPE_BOX, false, false, false, NxVec3(0.0f, 0.0f, 0.0f), NxVec3(1.0f, 1.0f, 1.0f), 0.0f,
		NxVec3(0.0f, 0.0f, 0.0f), NxVec3(0.0f, 0.0f, 0.0f), NxVec3(0.0f, 0.0f, 0.0f) },
	{ "s_rotated", NX_SHAPE_BOX, false, false, true, NxVec3(4.0f, 0.0f, 0.0f), NxVec3(0.5f, 2.0f, 1.0f), 0.0f,
		NxVec3(0.0f, 0.0f, 0.0f), NxVec3(0.0f, 0.0f, 0.0f), NxVec3(0.0f, 0.0f, 0.0f) },
	{ "d_box", NX_SHAPE_BOX, true, true, false, NxVec3(0.0f, 1.0f, 5.0f), NxVec3(0.5f, 1.0f, 1.5f), 2.0f,
		NxVec3(2.0f, 0.5f, 1.25f), NxVec3(1.0f, 2.0f, 3.0f), NxVec3(0.5f, -1.0f, 0.25f) },
	{ "d_rotated", NX_SHAPE_BOX, true, true, true, NxVec3(5.0f, 2.0f, 5.0f), NxVec3(1.0f, 0.5f, 0.25f), 3.0f,
		NxVec3(0.3f, 1.1f, 0.9f), NxVec3(-0.75f, 0.0f, 0.1f), NxVec3(0.0f, 0.0f, 2.0f) },
	{ "d_sphere", NX_SHAPE_SPHERE, true, true, false, NxVec3(10.0f, 0.0f, 5.0f), NxVec3(0.75f, 0.0f, 0.0f), 1.5f,
		NxVec3(1.0f, 2.0f, 3.0f), NxVec3(0.0f, 0.0f, 0.0f), NxVec3(0.0f, 0.0f, 0.0f) },
	{ "d_capsule", NX_SHAPE_CAPSULE, true, true, true, NxVec3(15.0f, 3.0f, -2.0f), NxVec3(0.5f, 1.0f, 0.0f), 1.0f,
		NxVec3(0.5f, 0.25f, 0.5f), NxVec3(0.0f, -3.0f, 0.0f), NxVec3(1.0f, 1.0f, 1.0f) },
	{ "d_hidden", NX_SHAPE_BOX, true, false, false, NxVec3(20.0f, 0.0f, 5.0f), NxVec3(1.0f, 1.0f, 1.0f), 1.0f,
		NxVec3(1.0f, 1.0f, 1.0f), NxVec3(1.0f, 0.0f, 0.0f), NxVec3(0.0f, 1.0f, 0.0f) },
	};
static const unsigned kActorCaseCount = sizeof(nxActorCases) / sizeof(nxActorCases[0]);

static NxActor* nxActors[kActorCaseCount];

static NxActor* nxCreateCase(NxScene* scene, unsigned index)
	{
	const NxActorCase& c = nxActorCases[index];
	NxBoxShapeDesc box;
	NxSphereShapeDesc sphere;
	NxCapsuleShapeDesc capsule;
	NxShapeDesc* shape = 0;
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
		default:
			return 0;
		}
	if(c.type != NX_SHAPE_BOX)
		shape->shapeFlags &= ~NX_SF_VISUALIZATION;
	NxActorDesc actor;
	actor.shapes.pushBack(shape);
	actor.globalPose.t = c.position;
	if(c.rotated)
		{
		// 0.6/0.8 rotations (exact in binary32): about z, or about x for the capsule.
		if(c.type == NX_SHAPE_CAPSULE)
			{
			actor.globalPose.M.setRow(0, NxVec3(1.0f, 0.0f, 0.0f));
			actor.globalPose.M.setRow(1, NxVec3(0.0f, 0.6f, -0.8f));
			actor.globalPose.M.setRow(2, NxVec3(0.0f, 0.8f, 0.6f));
			}
		else
			{
			actor.globalPose.M.setRow(0, NxVec3(0.6f, -0.8f, 0.0f));
			actor.globalPose.M.setRow(1, NxVec3(0.8f, 0.6f, 0.0f));
			actor.globalPose.M.setRow(2, NxVec3(0.0f, 0.0f, 1.0f));
			}
		}
	NxBodyDesc body;
	if(c.dynamic)
		{
		body.mass = c.mass;
		body.massSpaceInertia = c.inertia;
		body.linearVelocity = c.linear;
		body.angularVelocity = c.angular;
		if(!c.visualization)
			body.flags &= ~NX_BF_VISUALIZATION;
		actor.body = &body;
		}
	NxActor* created = scene->createActor(actor);
	printf("visualize create %s created=%u\n", c.name, created ? 1u : 0u);
	return created;
	}

static bool nxRunContactStage(NxPhysicsSDK* sdk)
	{
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.gravity.set(0.0f, -9.81f, 0.0f);
	sceneDesc.timeStepMethod = NX_TIMESTEP_VARIABLE;
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene)
		return false;

	NxPlaneShapeDesc ground;
	NxActorDesc staticDesc;
	staticDesc.shapes.pushBack(&ground);
	NxActor* staticActor = scene->createActor(staticDesc);

	NxSphereShapeDesc dynamicSphere;
	dynamicSphere.radius = 0.5f;
	NxBodyDesc body;
	body.mass = 2.0f;
	body.massSpaceInertia.set(0.1f, 0.1f, 0.1f);
	NxActorDesc dynamicDesc;
	dynamicDesc.shapes.pushBack(&dynamicSphere);
	dynamicDesc.body = &body;
	dynamicDesc.globalPose.t.set(0.0f, 1.0f, 0.0f);
	NxActor* dynamicActor = scene->createActor(dynamicDesc);
	printf("visualize contacts actors static=%u dynamic=%u\n", staticActor ? 1u : 0u,
		dynamicActor ? 1u : 0u);
	if(!staticActor || !dynamicActor)
		{
		sdk->releaseScene(*scene);
		return false;
		}
	scene->setActorPairFlags(*staticActor, *dynamicActor,
		NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH);

	bool fetched = true;
	for(unsigned step = 0; step < 60; ++step)
		{
		scene->simulate(1.0f / 60.0f);
		fetched = scene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!fetched)
			break;
		}
	printf("visualize contacts fetched=%u\n", fetched ? 1u : 0u);
	for(unsigned i = 0; i < kVisParameterCount; i++)
		sdk->setParameter(nxVisParameters[i], 0.0f);
	NxStage sapStage = { "collision_sap_after_step", { 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f } };
	nxRunStage(sdk, scene, sapStage, "contacts");
	for(unsigned i = 0; i < kVisParameterCount; i++)
		sdk->setParameter(nxVisParameters[i], 0.0f);
	sdk->setParameter(NX_VISUALIZATION_SCALE, 1.0f);
	sdk->setParameter(NX_VISUALIZE_CONTACT_POINT, 1.0f);
	sdk->setParameter(NX_VISUALIZE_CONTACT_NORMAL, 0.0f);
	sdk->setParameter(NX_VISUALIZE_CONTACT_ERROR, 0.0f);
	sdk->setParameter(NX_VISUALIZE_CONTACT_FORCE, 0.0f);
	nxStage = "contacts stage=after_step";
	scene->visualize();
	NxRecordingRenderer renderer;
	sdk->visualize(renderer);
	printf("visualize contacts stage=after_step contact_lines=%s count=%u\n",
		renderer.mLines ? "observed" : "missing", renderer.mLines);
	const bool perStepClear = renderer.mLines == 3;
	printf("visualize row445 per-step-clear frames=60 contact-lines=%u exact=%u\n",
		renderer.mLines, perStepClear ? 1u : 0u);
	nxStage = "";
	sdk->releaseScene(*scene);
	return fetched && perStepClear;
	}

static bool nxRunBoundedDynamicStage(NxPhysicsSDK* sdk)
	{
	NxBounds3 bounds;
	bounds.set(NxVec3(-32.0f, -16.0f, -16.0f), NxVec3(32.0f, 16.0f, 16.0f));
	NxSceneDesc desc;
	desc.setToDefault();
	desc.maxBounds = &bounds;
	NxScene* scene = sdk->createScene(desc);
	if(!scene)
		return false;

	NxActor* first = nxCreateCase(scene, 2);
	NxActor* second = nxCreateCase(scene, 4);
	const NxRay cacheWarmRay(NxVec3(-100.0f, -100.0f, -100.0f), NxVec3(1.0f, 0.0f, 0.0f));
	const bool cacheWarmHit = scene->raycastAnyShape(cacheWarmRay, NX_ALL_SHAPES, 0xffffffff, 500.0f);
	printf("visualize bounded bounds_cache_warm hit=%u\n", cacheWarmHit ? 1u : 0u);
	NxStage stage = { "bounded_dynamic", { 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f } };
	nxRunStage(sdk, scene, stage, "bounded");
	sdk->releaseScene(*scene);
	scene = sdk->createScene(desc);
	if(!scene)
		return false;
	NxStage emptyStage = { "bounded_dynamic_empty", { 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f } };
	nxRunStage(sdk, scene, emptyStage, "bounded");
	sdk->releaseScene(*scene);
	return first != 0 && second != 0;
	}

static bool nxRunEmptyBoundedDynamicStage(NxPhysicsSDK* sdk)
	{
	NxBounds3 bounds;
	bounds.set(NxVec3(-32.0f, -16.0f, -16.0f), NxVec3(32.0f, 16.0f, 16.0f));
	NxSceneDesc desc;
	desc.setToDefault();
	desc.maxBounds = &bounds;
	NxScene* scene = sdk->createScene(desc);
	if(!scene)
		return false;
	NxStage stage = { "bounded_dynamic_fresh_empty", { 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f } };
	nxRunStage(sdk, scene, stage, "bounded");
	sdk->releaseScene(*scene);
	return true;
	}

int wmain(int argc, wchar_t** argv)
	{
	setvbuf(stdout, 0, _IONBF, 0);
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsSceneVisualizeTests", pairDirectory, &physics);
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
	if(!nxRunBoundedDynamicStage(sdk))
		return nxFail("bounded dynamic visualization fixture could not create its actors");
	sdk->release();
	sdk = createSDK(NX_PHYSICS_SDK_VERSION, &allocator, &output);
	if(!sdk)
		return nxFail("SDK recreation after bounded dynamic fixture failed");
	if(!nxRunEmptyBoundedDynamicStage(sdk))
		return nxFail("fresh empty bounded dynamic visualization fixture could not create its scene");
	sdk->release();
	sdk = createSDK(NX_PHYSICS_SDK_VERSION, &allocator, &output);
	if(!sdk)
		return nxFail("SDK recreation after empty bounded dynamic fixture failed");
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene)
		return nxFail("scene creation failed");

	// Before any parameter is set: NX_VISUALIZATION_SCALE is 0, so visualize
	// creates nothing and the Foundation holds no renderable.
	{
		nxStage = "empty";
		scene->visualize();
		NxRecordingRenderer renderer;
		sdk->visualize(renderer);
		printf("visualize empty renderables=%u\n", renderer.mCalls);
		nxStage = "";
	}
	// Exercise the collision-AABB pass before either pruner exists. The oracle's
	// pruner-count helpers return zero for null static and selected dynamic
	// pruners; the Scene should still create an empty debug renderable.
	const NxStage emptyCollisionAabbs =
		{ "collision_aabbs", { 2.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f } };
	nxRunStage(sdk, scene, emptyCollisionAabbs, "empty_no_pruners");

	for(unsigned i = 0; i < kActorCaseCount; i++)
		nxActors[i] = nxCreateCase(scene, i);
	// Build the static and dynamic pruner bounds before exercising the cached
	// AABB visualizer. The ray misses the fixture, but forces both pruners to
	// populate their world-box caches from the owning shapes.
	const NxRay boundsCacheWarmRay(NxVec3(-100.0f, -100.0f, -100.0f), NxVec3(1.0f, 0.0f, 0.0f));
	const bool boundsCacheWarmHit = scene->raycastAnyShape(boundsCacheWarmRay, NX_ALL_SHAPES,
		0xffffffff, 500.0f);
	printf("visualize bounds_cache_warm hit=%u\n", boundsCacheWarmHit ? 1u : 0u);

	for(unsigned s = 0; s < kStageCount; s++)
		nxRunStage(sdk, scene, nxStages[s], "created");
	if(!nxFreePrunerBlueAabbsObserved)
		return nxFail("free-pruner AABB visualization did not match phys_fn_001937");

	// Twice with the same parameters: the renderable is cleared first, so the
	// second call's lines replace the first's.
	nxRunStage(sdk, scene, nxStages[kStageCount - 1], "again");

	// The scale back to 0: the renderable exists, is cleared and stays empty.
	nxRunStage(sdk, scene, nxStages[0], "cleared");

	// A body's state changes: pose, velocities.
	if(nxActors[2])
		{
		nxActors[2]->setGlobalPosition(NxVec3(1.0f, 2.0f, 3.0f));
		nxActors[2]->setLinearVelocity(NxVec3(0.0f, 0.0f, -4.0f));
		nxActors[2]->setAngularVelocity(NxVec3(0.0f, 0.0f, 0.0f));
		}
	if(nxActors[4])
		nxActors[4]->setAngularVelocity(NxVec3(3.0f, 0.0f, 4.0f));
	printf("visualize moved d_box d_sphere\n");
	nxRunStage(sdk, scene, nxStages[kStageCount - 1], "moved");

	// No actor is released before the scene: the oracle's NxScene::releaseActor
	// leaves a released actor on the array 000657 walks (it is still drawn),
	// the candidate's removes it at once, which is not these rows' behaviour.

	// Releasing the scene releases its renderable.
	sdk->releaseScene(*scene);
	{
		nxStage = "scene_released";
		NxRecordingRenderer renderer;
		sdk->visualize(renderer);
		printf("visualize scene_released renderables=%u\n", renderer.mCalls);
		nxStage = "";
	}
	if(!nxRunContactStage(sdk))
		return nxFail("contact visualization did not produce rendered contact points");
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
	}
