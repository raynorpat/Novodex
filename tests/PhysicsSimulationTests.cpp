#include "PhysicsPairLoader.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdlib.h>
#include <string.h>

#include "NxBitField.h"
#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxUserOutputStream.h"
#include "fluids/NxFluidDesc.h"
#include "fluids/NxImplicitMeshDesc.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxCapsuleShapeDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxBoxShape.h"
#include "NxPlaneShapeDesc.h"
#include "NxFixedJointDesc.h"
#include "NxSpringDesc.h"
#include "NxDistanceJointDesc.h"
#include "NxPrismaticJointDesc.h"
#include "NxRevoluteJointDesc.h"
#include "NxRevoluteJoint.h"
#include "NxSphericalJointDesc.h"
#include "NxD6JointDesc.h"
#include "NxSpringAndDamperEffector.h"
#include "NxSpringAndDamperEffectorDesc.h"
#include "NxJoint.h"
#include "NxUserNotify.h"
#include "NxMaterial.h"
#include "NxSimpleTriangleMesh.h"
#include "NxTriangleMeshDesc.h"
#include "NxTriangleMeshShapeDesc.h"
#include "NxBounds3.h"
#include "NxUserContactReport.h"
#include "NxFoundationSDK.h"
#include "NxUserAllocator.h"
#include "../Physics/src/include/NpSceneGuard.h"

struct NxFluidModelExportManifestEntry
	{
	const char* name;
	unsigned oracleGlobalRva;
	};

#define NX_FLUID_MODEL_EXPORT(name, oracleGlobalRva) { #name, oracleGlobalRva },
static const NxFluidModelExportManifestEntry nxFluidModelExportManifest[] =
	{
#include "../Physics/src/FluidModelExports.inc"
	};
#undef NX_FLUID_MODEL_EXPORT

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);
typedef NxFoundationSDK* (NX_CALL_CONV *CreateFoundationSDKFn)(NxU32, NxUserOutputStream*, NxUserAllocator*);
typedef void (NX_CALL_CONV *JointDescSetGlobalAnchorFn)(NxJointDesc&, const NxVec3&);
typedef void (NX_CALL_CONV *JointDescSetGlobalAxisFn)(NxJointDesc&, const NxVec3&);
static unsigned nxFloatBits(NxReal value);

// Private ABI probe for the three-entry controller vtable recovered from the
// pinned DLL. Keep this test-only: the public NxController declaration remains
// intentionally incomplete and unchanged.
class NxGroupsMask;
class NxControllerProbe
	{
	public:
	virtual ~NxControllerProbe() {}
	virtual void move(const NxVec3&, NxU32 activeGroups, NxReal minDistance,
		NxU32& collisionFlags, NxReal sharpness = 1.0f,
		const NxGroupsMask* groupsMask = 0) = 0;
	virtual const NxVec3& getPosition() const = 0;
	};

static unsigned gNxSimulationFluidDestructorCalls = 0;
static unsigned gNxSimulationFluidDestructorFlags = 0;
static void* gNxSimulationFluidDestructorObject = 0;

class NxSimulationCountingAllocator : public NxUserAllocator
	{
	public:
	NxSimulationCountingAllocator(): mallocCalls(0), freeCalls(0), liveAllocations(0) {}
	void* mallocDEBUG(size_t size, const char*, int) override { return malloc(size); }
	void* malloc(size_t size) override
		{
		void* const memory = ::malloc(size);
		if(memory)
			{
			++mallocCalls;
			++liveAllocations;
			}
		return memory;
		}
	void* realloc(void* memory, size_t size) override
		{
		void* const replacement = ::realloc(memory, size);
		if(replacement && !memory)
			{
			++mallocCalls;
			++liveAllocations;
			}
		return replacement;
		}
	void free(void* memory) override
		{
		if(memory)
			{
			++freeCalls;
			if(liveAllocations)
				--liveAllocations;
			::free(memory);
			}
		}
	unsigned mallocCalls;
	unsigned freeCalls;
	unsigned liveAllocations;
	};

static void* __fastcall nxSimulationFluidDeletingDestructor(
	void* fluid, void*, unsigned char deleteObject)
	{
	++gNxSimulationFluidDestructorCalls;
	gNxSimulationFluidDestructorFlags = deleteObject;
	gNxSimulationFluidDestructorObject = fluid;
	return fluid;
	}

class NxSimulationOutputStream : public NxUserOutputStream
	{
	public:
	NxSimulationOutputStream(): errors(0), code(NXE_NO_ERROR), line(0)
		{
		message[0] = 0;
		file[0] = 0;
		}
	void reportError(NxErrorCode errorCode, const char* errorMessage, const char* errorFile, int errorLine)
		{
		++errors;
		code = errorCode;
		line = errorLine;
		copy(message, sizeof(message), errorMessage);
		copy(file, sizeof(file), errorFile);
		}
	NxAssertResponse reportAssertViolation(const char*, const char*, int)	{ return NX_AR_CONTINUE; }
	void print(const char*) {}
	void resetLast()
		{
		code = NXE_NO_ERROR;
		line = 0;
		message[0] = 0;
		file[0] = 0;
		}
	unsigned errors;
	NxErrorCode code;
	int line;
	char message[128];
	char file[96];
	private:
	static void copy(char* destination, size_t capacity, const char* source)
		{
		if(!source)
			{
			destination[0] = 0;
			return;
			}
		strncpy_s(destination, capacity, source, _TRUNCATE);
		}
	};

// Redirect this process's HKLM view to an isolated HKCU key so the licensed
// NxFluid getter branch can be exercised without changing machine-wide state.
struct NxSimulationFluidLicenseOverride
	{
	NxSimulationFluidLicenseOverride(unsigned productCode = 465)
		: root(0), product(0), rootCreated(false), overridden(false), ready(false)
		{
		rootPath[0] = 0;
		sprintf_s(rootPath, sizeof(rootPath),
			"Software\\NxPhysicsFluidKeyTest-%lu-%lu",
			static_cast<unsigned long>(GetCurrentProcessId()),
			static_cast<unsigned long>(GetTickCount()));
		DWORD disposition = 0;
		if(RegCreateKeyExA(HKEY_CURRENT_USER, rootPath, 0, 0, 0, KEY_ALL_ACCESS,
			0, &root, &disposition) != ERROR_SUCCESS)
			return;
		rootCreated = disposition == REG_CREATED_NEW_KEY;
		if(!rootCreated)
			return;
		if(RegOverridePredefKey(HKEY_LOCAL_MACHINE, root) != ERROR_SUCCESS)
			return;
		overridden = true;
		if(RegCreateKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\NovodexSDK212", 0,
			0, 0, KEY_ALL_ACCESS, 0, &product, &disposition) != ERROR_SUCCESS)
			return;
		char fluidKey[32] = {};
		sprintf_s(fluidKey, sizeof(fluidKey), "a-%u-:", productCode);
		if(RegSetValueExA(product, "FluidKey", 0, REG_SZ,
			reinterpret_cast<const BYTE*>(fluidKey), sizeof(fluidKey)) != ERROR_SUCCESS)
			return;
		HKEY verify = 0;
		char readback[32] = {};
		DWORD type = 0;
		DWORD size = sizeof(readback);
		const bool readbackMatches =
			RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\NovodexSDK212", 0,
				KEY_QUERY_VALUE, &verify) == ERROR_SUCCESS &&
			RegQueryValueExA(verify, "FluidKey", 0, &type,
				reinterpret_cast<BYTE*>(readback), &size) == ERROR_SUCCESS &&
			type == REG_SZ && strcmp(readback, fluidKey) == 0;
		if(verify)
			RegCloseKey(verify);
		ready = readbackMatches;
		}
	~NxSimulationFluidLicenseOverride()
		{
		if(product)
			RegCloseKey(product);
		if(overridden)
			RegOverridePredefKey(HKEY_LOCAL_MACHINE, 0);
		if(root)
			RegCloseKey(root);
		if(rootCreated && rootPath[0])
			{
			char productPath[192];
			char softwarePath[160];
			sprintf_s(productPath, sizeof(productPath), "%s\\SOFTWARE\\NovodexSDK212",
				rootPath);
			sprintf_s(softwarePath, sizeof(softwarePath), "%s\\SOFTWARE", rootPath);
			RegDeleteKeyA(HKEY_CURRENT_USER, productPath);
			RegDeleteKeyA(HKEY_CURRENT_USER, softwarePath);
			RegDeleteKeyA(HKEY_CURRENT_USER, rootPath);
			}
		}
	HKEY root;
	HKEY product;
	bool rootCreated;
	bool overridden;
	bool ready;
	char rootPath[128];
	};

static unsigned gSimulationSurfaceMeshDeletingCalls;
static unsigned gSimulationSurfaceMeshDeleteFlags;

static void* __fastcall nxSimulationSurfaceMeshDeletingDestructor(
	void* mesh, void*, unsigned char deleting)
	{
	++gSimulationSurfaceMeshDeletingCalls;
	gSimulationSurfaceMeshDeleteFlags = deleting;
	return mesh;
	}

static void* gSimulationSurfaceMeshVtable[] = {
	reinterpret_cast<void*>(&nxSimulationSurfaceMeshDeletingDestructor)
	};

class NxSimulationFailSizeAllocator : public NxUserAllocator
	{
	public:
	NxSimulationFailSizeAllocator(NxUserAllocator& inner, size_t failSize)
		: mInner(inner), mFailSize(failSize), mFailCount(0)
		{
		}
	void* mallocDEBUG(size_t size, const char* fileName, int line) override
		{
			if(size == mFailSize && !mFailCount)
				{
				++mFailCount;
				return 0;
				}
			return mInner.mallocDEBUG(size, fileName, line);
		}
	void* malloc(size_t size) override
		{
			if(size == mFailSize && !mFailCount)
				{
				++mFailCount;
				return 0;
				}
			return mInner.malloc(size);
		}
	void* realloc(void* memory, size_t size) override
		{
			return mInner.realloc(memory, size);
		}
	void free(void* memory) override { mInner.free(memory); }
	unsigned mFailCount;
	private:
	NxUserAllocator& mInner;
	size_t mFailSize;
	};

static void nxPrintSimulationSceneState(NxScene* scene, unsigned selector, const char* phase)
	{
	NxSceneStats* const sceneStats = scene->getSceneStats();
	NxSceneLimits sceneLimits;
	scene->getLimits(sceneLimits);
	printf("simulation scene-stats phase=%s selector=%u present=%u contacts=%d max_contacts=%d actors=%d joints=%d awake=%d asleep=%d static_shapes=%d\n",
		phase, selector, sceneStats != 0, sceneStats ? sceneStats->numContacts : -1,
		sceneStats ? sceneStats->maxContacts : -1, sceneStats ? sceneStats->numActors : -1,
		sceneStats ? sceneStats->numJoints : -1, sceneStats ? sceneStats->numAwake : -1,
		sceneStats ? sceneStats->numAsleep : -1, sceneStats ? sceneStats->numStaticShapes : -1);
	printf("simulation scene-limits phase=%s selector=%u actors=%u bodies=%u static_shapes=%u dynamic_shapes=%u joints=%u\n",
		phase, selector, sceneLimits.maxNbActors, sceneLimits.maxNbBodies,
		sceneLimits.maxNbStaticShapes, sceneLimits.maxNbDynamicShapes,
		sceneLimits.maxNbJoints);
	}

struct NxSimulationHeldSceneWriteLock
	{
	void* link;
	HANDLE ready;
	HANDLE release;
	};

static DWORD WINAPI nxSimulationHoldSceneWriteLock(void* context)
	{
	NxSimulationHeldSceneWriteLock* held = static_cast<NxSimulationHeldSceneWriteLock*>(context);
	nxNpSceneGuardEnter(held->link);
	SetEvent(held->ready);
	WaitForSingleObject(held->release, INFINITE);
	nxNpSceneGuardLeave(held->link);
	return 0;
	}

class NxSimulationTriggerReport : public NxUserTriggerReport
	{
	public:
	NxShape* expectedTrigger;
	NxShape* expectedOther;
	unsigned calls;
	unsigned status;
	bool lastMatches;
	NxSimulationTriggerReport(NxShape* trigger, NxShape* other)
		: expectedTrigger(trigger), expectedOther(other), calls(0), status(0), lastMatches(false) {}
	virtual void onTrigger(NxShape& trigger, NxShape& other, NxTriggerFlag event)
		{
		++calls;
		status = static_cast<unsigned>(event);
		lastMatches = &trigger == expectedTrigger && &other == expectedOther;
		printf("simulation fetch-trigger trigger=%u other=%u status=%u\n",
			&trigger == expectedTrigger, &other == expectedOther, status);
		}
	};

class NxSimulationTriggerLifecycleReport : public NxUserTriggerReport
	{
	public:
	NxShape* expectedTrigger;
	NxShape* expectedOther;
	NxTriggerFlag events[8];
	unsigned calls;
	unsigned badShapes;
	NxSimulationTriggerLifecycleReport(NxShape* trigger, NxShape* other)
		: expectedTrigger(trigger), expectedOther(other), calls(0), badShapes(0) {}
	virtual void onTrigger(NxShape& trigger, NxShape& other, NxTriggerFlag event)
		{
		if(&trigger != expectedTrigger || &other != expectedOther)
			++badShapes;
		if(calls < sizeof(events) / sizeof(events[0]))
			events[calls] = event;
		++calls;
		printf("simulation trigger-lifecycle event=%u trigger=%u other=%u\n",
			static_cast<unsigned>(event), &trigger == expectedTrigger,
			&other == expectedOther);
		}
	};

class NxSimulationTriggerBatchReport : public NxUserTriggerReport
	{
	public:
	enum { BatchCount = 32 };
	NxShape* trigger;
	NxShape* others[BatchCount];
	unsigned eventCounts[BatchCount][3];
	unsigned calls;
	unsigned badShapes;
	NxSimulationTriggerBatchReport(NxShape* triggerShape, NxShape** otherShapes)
		: trigger(triggerShape), calls(0), badShapes(0)
		{
		memset(eventCounts, 0, sizeof(eventCounts));
		for(unsigned index = 0; index != BatchCount; ++index)
			others[index] = otherShapes[index];
		}
	virtual void onTrigger(NxShape& triggerShape, NxShape& otherShape, NxTriggerFlag event)
		{
		unsigned otherIndex = BatchCount;
		for(unsigned index = 0; index != BatchCount; ++index)
			if(&otherShape == others[index])
				otherIndex = index;
		unsigned eventIndex = event == NX_TRIGGER_ON_ENTER ? 0 :
			event == NX_TRIGGER_ON_STAY ? 1 : event == NX_TRIGGER_ON_LEAVE ? 2 : 3;
		if(&triggerShape != trigger || otherIndex == BatchCount || eventIndex == 3)
			++badShapes;
		else
			++eventCounts[otherIndex][eventIndex];
		++calls;
		printf("simulation trigger-batch event=%u other=%u trigger_ok=%u\n",
			static_cast<unsigned>(event), otherIndex,
			&triggerShape == trigger);
		}
	};

class NxSimulationCompoundTriggerReport : public NxUserTriggerReport
	{
	public:
	NxShape* triggers[2];
	NxShape* other;
	unsigned eventCounts[2][3];
	unsigned calls;
	unsigned badShapes;
	NxSimulationCompoundTriggerReport(NxShape* const* triggerShapes, NxShape* otherShape)
		: other(otherShape), calls(0), badShapes(0)
		{
		memset(eventCounts, 0, sizeof(eventCounts));
		triggers[0] = triggerShapes[0];
		triggers[1] = triggerShapes[1];
		}
	virtual void onTrigger(NxShape& trigger, NxShape& otherShape, NxTriggerFlag event)
		{
		unsigned triggerIndex = 2;
		for(unsigned index = 0; index != 2; ++index)
			if(&trigger == triggers[index])
				triggerIndex = index;
		const unsigned eventIndex = event == NX_TRIGGER_ON_ENTER ? 0 :
			event == NX_TRIGGER_ON_STAY ? 1 : event == NX_TRIGGER_ON_LEAVE ? 2 : 3;
		if(triggerIndex == 2 || &otherShape != other || eventIndex == 3)
			++badShapes;
		else
			++eventCounts[triggerIndex][eventIndex];
		++calls;
		printf("simulation trigger-compound event=%u trigger=%u other=%u\n",
			static_cast<unsigned>(event), triggerIndex,
			&otherShape == other);
		}
	};

class NxSimulationContactReport : public NxUserContactReport
	{
	public:
	NxActor* expectedActor0;
	NxActor* expectedActor1;
	unsigned calls;
	unsigned events;
	NxSimulationContactReport(NxActor* actor0, NxActor* actor1)
		: expectedActor0(actor0), expectedActor1(actor1), calls(0), events(0) {}
	virtual void onContactNotify(NxContactPair& pair, NxU32 eventFlags)
		{
		++calls;
		events = eventFlags;
		printf("simulation fetch-contact actor0=%u actor1=%u events=%08x force=%08x.%08x.%08x\n",
			pair.actors[0] == expectedActor0, pair.actors[1] == expectedActor1,
			eventFlags, nxFloatBits(pair.sumNormalForce.x),
			nxFloatBits(pair.sumNormalForce.y), nxFloatBits(pair.sumNormalForce.z));
		}
	};

static unsigned nxFloatBits(NxReal value)
	{
	unsigned bits;
	memcpy(&bits, &value, sizeof(bits));
	return bits;
	}

class NxSimulationJointBreakNotify : public NxUserNotify
	{
	public:
	NxSimulationJointBreakNotify(bool releaseJoint)
		: releaseJoint(releaseJoint), expectedJoint(0), calls(0),
		  forceBits(0), jointMatches(false), stateAtCallback(0) {}
	virtual bool onJointBreak(NxReal force, NxJoint& joint)
		{
		++calls;
		forceBits = nxFloatBits(force);
		jointMatches = &joint == expectedJoint;
		stateAtCallback = static_cast<unsigned>(joint.getState());
		return releaseJoint;
		}
	bool releaseJoint;
	NxJoint* expectedJoint;
	unsigned calls;
	unsigned forceBits;
	bool jointMatches;
	unsigned stateAtCallback;
	};

static void nxPrintActorState(const char* stage, NxActor& actor)
	{
	const NxVec3& p = actor.getGlobalPosition();
	NxVec3 v;
	actor.getLinearVelocity(v);
	printf("simulation stage=%s position=%08x.%08x.%08x velocity=%08x.%08x.%08x\n",
		stage, nxFloatBits(p.x), nxFloatBits(p.y), nxFloatBits(p.z),
		nxFloatBits(v.x), nxFloatBits(v.y), nxFloatBits(v.z));
	}

static void nxPrintActorBodyVelocity(const char* stage, NxActor& actor)
	{
	unsigned char* const wrapper = reinterpret_cast<unsigned char*>(&actor);
	unsigned char* const body = *reinterpret_cast<unsigned char**>(wrapper + 0x14);
	unsigned char* const record = *reinterpret_cast<unsigned char**>(body + 8);
	const NxReal* const velocity = reinterpret_cast<const NxReal*>(record + 0x34);
	printf("simulation body-state=%s velocity=%08x.%08x.%08x\n", stage,
		nxFloatBits(velocity[0]), nxFloatBits(velocity[1]), nxFloatBits(velocity[2]));
	}

// NpScene::mScene is at wrapper+0x24; the mode and pair-list offsets are
// pinned by the oracle's 000544 -> 001973 initialization path.
static unsigned nxReadSceneBroadPhaseMode(NxScene* scene)
	{
	unsigned char* const wrapper = reinterpret_cast<unsigned char*>(scene);
	unsigned char* const internal = *reinterpret_cast<unsigned char**>(wrapper + 0x24);
	return *reinterpret_cast<unsigned*>(internal + 0x654);
	}

static unsigned nxReadSceneBroadPhasePairCount(NxScene* scene)
	{
	unsigned char* const wrapper = reinterpret_cast<unsigned char*>(scene);
	unsigned char* const internal = *reinterpret_cast<unsigned char**>(wrapper + 0x24);
	unsigned char* node = *reinterpret_cast<unsigned char**>(internal + 0x674);
	unsigned count = 0;
	while(node)
		{
		++count;
		node = *reinterpret_cast<unsigned char**>(node + 8);
		}
	return count;
	}

static void nxPrintBroadPhasePairOrder(NxScene* scene, unsigned selector,
	NxActor** actors, unsigned actorCount)
	{
	unsigned char* const wrapper = reinterpret_cast<unsigned char*>(scene);
	unsigned char* const internal = *reinterpret_cast<unsigned char**>(wrapper + 0x24);
	unsigned char* node = *reinterpret_cast<unsigned char**>(internal + 0x674);
	printf("simulation broadphase order selector=%u pairs=", selector);
	bool first = true;
	while(node)
		{
		unsigned char* const pair = node + 0x14;
		unsigned char* owner[2] = {
			*reinterpret_cast<unsigned char**>(pair),
			*reinterpret_cast<unsigned char**>(pair + 4) };
		unsigned char* body[2] = {
			owner[0] ? *reinterpret_cast<unsigned char**>(owner[0] + 8) : 0,
			owner[1] ? *reinterpret_cast<unsigned char**>(owner[1] + 8) : 0 };
		if(body[0] && body[1])
			{
			const NxReal x0 = *reinterpret_cast<NxReal*>(body[0] + 0x158);
			const NxReal x1 = *reinterpret_cast<NxReal*>(body[1] + 0x158);
			unsigned a = actorCount;
			unsigned b = actorCount;
			for(unsigned actorIndex = 0; actorIndex != actorCount; ++actorIndex)
				{
				const NxReal actorX = actors[actorIndex]->getGlobalPosition().x;
				if(actorX == x0) a = actorIndex;
				if(actorX == x1) b = actorIndex;
				}
			printf("%s%u-%u", first ? "" : ",", a, b);
			first = false;
			}
		node = *reinterpret_cast<unsigned char**>(node + 8);
		}
	printf("\n");
	}

static void nxPrintBoxActorState(const char* stage, NxActor& actor)
	{
	nxPrintActorState(stage, actor);
	const NxQuat q = actor.getGlobalOrientationQuat();
	const NxVec3 w = actor.getAngularVelocity();
	const NxMat34 pose = actor.getGlobalPose();
	NxReal rotation[9];
	pose.M.getRowMajor(rotation);
	NxShape** shapes = actor.getShapes();
	const NxMat34 shapePose = shapes[0]->getGlobalPose();
	NxReal shapeRotation[9];
	shapePose.M.getRowMajor(shapeRotation);
	printf("simulation box-state stage=%s orientation=%08x.%08x.%08x.%08x angular=%08x.%08x.%08x matrix=%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x shape=%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x\n",
		stage, nxFloatBits(q.x), nxFloatBits(q.y), nxFloatBits(q.z), nxFloatBits(q.w),
		nxFloatBits(w.x), nxFloatBits(w.y), nxFloatBits(w.z),
		nxFloatBits(rotation[0]), nxFloatBits(rotation[1]), nxFloatBits(rotation[2]),
		nxFloatBits(rotation[3]), nxFloatBits(rotation[4]), nxFloatBits(rotation[5]),
		nxFloatBits(rotation[6]), nxFloatBits(rotation[7]), nxFloatBits(rotation[8]),
		nxFloatBits(shapePose.t.x), nxFloatBits(shapePose.t.y), nxFloatBits(shapePose.t.z),
		nxFloatBits(shapeRotation[0]), nxFloatBits(shapeRotation[1]), nxFloatBits(shapeRotation[2]),
		nxFloatBits(shapeRotation[3]), nxFloatBits(shapeRotation[4]), nxFloatBits(shapeRotation[5]),
		nxFloatBits(shapeRotation[6]), nxFloatBits(shapeRotation[7]), nxFloatBits(shapeRotation[8]));
	}

static void nxPrintActorMotionState(const char* stage, NxActor& actor)
	{
	const NxVec3& p = actor.getGlobalPosition();
	NxVec3 v;
	NxVec3 w;
	actor.getLinearVelocity(v);
	actor.getAngularVelocity(w);
	printf("simulation motion stage=%s position=%08x.%08x.%08x velocity=%08x.%08x.%08x angular=%08x.%08x.%08x\n",
		stage, nxFloatBits(p.x), nxFloatBits(p.y), nxFloatBits(p.z),
		nxFloatBits(v.x), nxFloatBits(v.y), nxFloatBits(v.z),
		nxFloatBits(w.x), nxFloatBits(w.y), nxFloatBits(w.z));
	}

struct NxSimulationContactStreamReport : NxUserContactReport
	{
	unsigned calls;
	unsigned events;
	unsigned pairs;
	unsigned patches;
	unsigned points;
	unsigned normal[3];
	unsigned sumNormalForce[3];
	unsigned sumFrictionForce[3];
	unsigned point[3];
	unsigned separation;
	unsigned capturedCallbacks;
	NxU32 lastEvents;
	NxActor* lastActors[2];
	unsigned callbackPointCount[8];
	unsigned callbackPoint[8][8][4];

	NxSimulationContactStreamReport()
		: calls(0), events(0), pairs(0), patches(0), points(0), separation(0),
			capturedCallbacks(0), lastEvents(0)
		{
		lastActors[0] = lastActors[1] = 0;
		memset(normal, 0, sizeof(normal));
		memset(sumNormalForce, 0, sizeof(sumNormalForce));
		memset(sumFrictionForce, 0, sizeof(sumFrictionForce));
		memset(point, 0, sizeof(point));
		memset(callbackPointCount, 0, sizeof(callbackPointCount));
		memset(callbackPoint, 0, sizeof(callbackPoint));
		}

	void onContactNotify(NxContactPair& pair, NxU32 eventFlags)
		{
		const unsigned callback = calls;
		++calls;
		lastEvents = eventFlags;
		lastActors[0] = pair.actors[0];
		lastActors[1] = pair.actors[1];
		if(callback < 8)
			capturedCallbacks = callback + 1;
		events |= eventFlags;
		sumNormalForce[0] = nxFloatBits(pair.sumNormalForce.x);
		sumNormalForce[1] = nxFloatBits(pair.sumNormalForce.y);
		sumNormalForce[2] = nxFloatBits(pair.sumNormalForce.z);
		sumFrictionForce[0] = nxFloatBits(pair.sumFrictionForce.x);
		sumFrictionForce[1] = nxFloatBits(pair.sumFrictionForce.y);
		sumFrictionForce[2] = nxFloatBits(pair.sumFrictionForce.z);
		NxContactStreamIterator iterator(pair.stream);
		while(iterator.goNextPair())
			{
			++pairs;
			while(iterator.goNextPatch())
				{
				++patches;
				while(iterator.goNextPoint())
					{
					++points;
					if(points == 1)
						{
						const NxVec3& n = iterator.getPatchNormal();
						const NxVec3& p = iterator.getPoint();
						for(unsigned axis = 0; axis < 3; ++axis)
							{
							normal[axis] = nxFloatBits(n[axis]);
							point[axis] = nxFloatBits(p[axis]);
							}
						separation = nxFloatBits(iterator.getSeparation());
						}
					if(callback < 8 && callbackPointCount[callback] < 8)
						{
						const NxVec3& p = iterator.getPoint();
						unsigned* out = callbackPoint[callback][callbackPointCount[callback]++];
						out[0] = nxFloatBits(p.x);
						out[1] = nxFloatBits(p.y);
						out[2] = nxFloatBits(p.z);
						out[3] = nxFloatBits(iterator.getSeparation());
						}
					}
				}
			}
		}
	};

static bool nxSimulationResolvePrivateRow(HMODULE physics, const wchar_t* pairDirectory,
	const char* decoratedSymbol, NxU32 oracleRva, void** function)
	{
	wchar_t mapPath[MAX_PATH];
	if(swprintf_s(mapPath, L"%s\\NxPhysics.map", pairDirectory) < 0)
		return false;
	if(GetFileAttributesW(mapPath) == INVALID_FILE_ATTRIBUTES)
		{
		*function = reinterpret_cast<unsigned char*>(physics) + oracleRva;
		return true;
		}
	FILE* map = 0;
	if(_wfopen_s(&map, mapPath, L"rt") != 0 || !map)
		return false;
	NxU32 preferredBase = 0;
	NxU32 symbolAddress = 0;
	char line[1024];
	while(fgets(line, sizeof(line), map))
		{
		unsigned long parsedBase = 0;
		if(sscanf_s(line, " Preferred load address is %lx", &parsedBase) == 1)
			preferredBase = static_cast<NxU32>(parsedBase);
		char* symbol = strstr(line, decoratedSymbol);
		if(symbol)
			{
			char* address = symbol + strlen(decoratedSymbol);
			while(*address == ' ' || *address == '\t') ++address;
			char* end = 0;
			const unsigned long parsedAddress = strtoul(address, &end, 16);
			if(end == address)
				{
				fclose(map);
				return false;
				}
			symbolAddress = static_cast<NxU32>(parsedAddress);
			break;
			}
		}
	fclose(map);
	if(!preferredBase || symbolAddress < preferredBase)
		return false;
	*function = reinterpret_cast<unsigned char*>(physics) + (symbolAddress - preferredBase);
	return true;
	}

static unsigned short nxSimulationReadX87ControlWord()
	{
	unsigned short controlWord = 0;
	__asm { fnstcw controlWord }
	return controlWord;
	}

static bool nxSimulationTestFluidModelExportResolution(HMODULE physics,
	const wchar_t* pairDirectory)
	{
	const size_t exportCount = sizeof(nxFluidModelExportManifest) /
		sizeof(nxFluidModelExportManifest[0]);
	if(exportCount != 169)
		return false;
	wchar_t mapPath[MAX_PATH];
	if(swprintf_s(mapPath, L"%s\\NxPhysics.map", pairDirectory) < 0)
		return false;
	const bool candidate = GetFileAttributesW(mapPath) != INVALID_FILE_ATTRIBUTES;
	if(!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32 |
		LOAD_LIBRARY_SEARCH_USER_DIRS))
		return false;
	DLL_DIRECTORY_COOKIE const directoryCookie = AddDllDirectory(pairDirectory);
	if(!directoryCookie)
		return false;
	unsigned loaded = 0;
	unsigned resolved = 0;
	void* resolverAddress = 0;
	void* exportAccessorAddress = 0;
	void* clearAddress = 0;
	void* unloadAddress = 0;
	void* callbackCountAddress = 0;
	if(candidate)
		{
		if(!nxSimulationResolvePrivateRow(physics, pairDirectory,
			"?nxFluidManagerResolveModel@@YA_NXZ", 0x0008d870u,
			&resolverAddress) ||
			!nxSimulationResolvePrivateRow(physics, pairDirectory,
				"_nxFluidModelGetResolvedExport", 0, &exportAccessorAddress) ||
			!nxSimulationResolvePrivateRow(physics, pairDirectory,
				"_nxFluidModelClearResolvedExports", 0x0008e4e0u,
				&clearAddress) ||
			!nxSimulationResolvePrivateRow(physics, pairDirectory,
				"_nxFluidModelUnloadLibrary", 0x0008d850u,
				&unloadAddress) ||
			!nxSimulationResolvePrivateRow(physics, pairDirectory,
				"_nxFluidModelCountResolvedCallbacks", 0,
				&callbackCountAddress))
			return false;
		typedef bool (__cdecl *ResolveFluidModelFn)();
		typedef FARPROC (__cdecl *GetResolvedExportFn)(unsigned);
		typedef unsigned (__cdecl *ClearResolvedExportsFn)();
		typedef unsigned (__cdecl *UnloadFluidModelFn)();
		typedef unsigned (__cdecl *CountResolvedCallbacksFn)();
		resolved = reinterpret_cast<ResolveFluidModelFn>(resolverAddress)() ? 1u : 0u;
		GetResolvedExportFn const getResolvedExport =
			reinterpret_cast<GetResolvedExportFn>(exportAccessorAddress);
		HMODULE const fluidModel = GetModuleHandleA("FluidModel.DLL");
		if(fluidModel)
			loaded = 1;
		if(!fluidModel)
			return false;
		unsigned matched = 0;
		unsigned present = 0;
		unsigned digest = 2166136261u;
		for(unsigned i = 0; i != exportCount; ++i)
			{
			FARPROC const expected = GetProcAddress(fluidModel,
				nxFluidModelExportManifest[i].name);
			FARPROC const actual = getResolvedExport(i);
			const bool equal = actual == expected;
			if(!equal)
				printf("fluid-model-export-mismatch side=candidate index=%u name=%s expected=%p actual=%p\n",
					i, nxFluidModelExportManifest[i].name, expected, actual);
			matched += equal ? 1u : 0u;
			present += actual != 0 ? 1u : 0u;
			digest = (digest ^ (equal ? 1u : 0u)) * 16777619u;
			digest = (digest ^ (actual != 0 ? 1u : 0u)) * 16777619u;
			}
		const unsigned clearResult = reinterpret_cast<ClearResolvedExportsFn>(
			clearAddress)();
		const unsigned callbacksRemaining = reinterpret_cast<
			CountResolvedCallbacksFn>(callbackCountAddress)();
		unsigned cleared = 0;
		for(unsigned i = 0; i != exportCount; ++i)
			cleared += getResolvedExport(i) == 0 ? 1u : 0u;
		const unsigned clearMatched = clearResult == 1 &&
			cleared == exportCount && callbacksRemaining == 0 ?
			static_cast<unsigned>(exportCount) : 0u;
		const unsigned unloadResult = reinterpret_cast<UnloadFluidModelFn>(
			unloadAddress)();
		const unsigned moduleAfter = GetModuleHandleA("FluidModel.DLL") ? 1u : 0u;
		RemoveDllDirectory(directoryCookie);
		printf("simulation fluid-model-export-resolve row=003903 slots=%u present=%u matched=%u digest=%08x\n",
			static_cast<unsigned>(exportCount), present, matched, digest);
		printf("simulation fluid-model-export-clear row=003905 slots=%u result=%u cleared=%u callbacks_remaining=%u matched=%u mismatches=%u\n",
			static_cast<unsigned>(exportCount), clearResult, cleared,
			callbacksRemaining, clearMatched,
			clearMatched == exportCount ? 0u : 1u);
		printf("simulation fluid-model-unload row=003902 result=%u module_after=%u matched=%u mismatches=%u\n",
			unloadResult, moduleAfter,
			unloadResult == 1 && moduleAfter == 0 ? 1u : 0u,
			unloadResult == 1 && moduleAfter == 0 ? 0u : 1u);
		return loaded && resolved && matched == exportCount && clearResult == 1 &&
			cleared == exportCount && callbacksRemaining == 0 &&
			unloadResult == 1 && moduleAfter == 0;
		}
	void* loadAddress = reinterpret_cast<unsigned char*>(physics) + 0x0008d820u;
	void* resolveAddress = reinterpret_cast<unsigned char*>(physics) + 0x0008d870u;
	typedef unsigned (__stdcall *OracleLoadFn)();
	typedef unsigned (__stdcall *OracleResolveFn)();
	loaded = reinterpret_cast<OracleLoadFn>(loadAddress)() & 0xffu;
	resolved = reinterpret_cast<OracleResolveFn>(resolveAddress)() & 0xffu;
	HMODULE const fluidModel = GetModuleHandleA("FluidModel.DLL");
	if(!fluidModel)
		return false;
	unsigned matched = 0;
	unsigned present = 0;
	unsigned digest = 2166136261u;
	for(unsigned i = 0; i != exportCount; ++i)
		{
		FARPROC const expected = GetProcAddress(fluidModel,
			nxFluidModelExportManifest[i].name);
		FARPROC const actual = *reinterpret_cast<FARPROC*>(
			reinterpret_cast<unsigned char*>(physics) +
			nxFluidModelExportManifest[i].oracleGlobalRva);
		const bool equal = actual == expected;
		if(!equal)
			printf("fluid-model-export-mismatch side=oracle index=%u name=%s expected=%p actual=%p rva=%08x\n",
				i, nxFluidModelExportManifest[i].name, expected, actual,
				nxFluidModelExportManifest[i].oracleGlobalRva);
		matched += equal ? 1u : 0u;
		present += actual != 0 ? 1u : 0u;
		digest = (digest ^ (equal ? 1u : 0u)) * 16777619u;
		digest = (digest ^ (actual != 0 ? 1u : 0u)) * 16777619u;
		}
	typedef unsigned (__stdcall *OracleClearFn)();
	const unsigned clearResult = reinterpret_cast<OracleClearFn>(
		reinterpret_cast<unsigned char*>(physics) + 0x0008e4e0u)() & 0xffu;
	unsigned cleared = 0;
	for(unsigned i = 0; i != exportCount; ++i)
		cleared += *reinterpret_cast<FARPROC*>(
			reinterpret_cast<unsigned char*>(physics) +
			nxFluidModelExportManifest[i].oracleGlobalRva) == 0 ? 1u : 0u;
	typedef unsigned (__stdcall *OracleUnloadFn)();
	const unsigned unloaded = reinterpret_cast<OracleUnloadFn>(
		reinterpret_cast<unsigned char*>(physics) + 0x0008d850u)() & 0xffu;
	const unsigned moduleAfter = GetModuleHandleA("FluidModel.DLL") ? 1u : 0u;
	const unsigned clearMatched = clearResult == 1 && cleared == exportCount ?
		static_cast<unsigned>(exportCount) : 0u;
	RemoveDllDirectory(directoryCookie);
	printf("simulation fluid-model-export-resolve row=003903 slots=%u present=%u matched=%u digest=%08x\n",
		static_cast<unsigned>(exportCount), present, matched, digest);
	printf("simulation fluid-model-export-clear row=003905 slots=%u result=%u cleared=%u callbacks_remaining=0 matched=%u mismatches=%u\n",
		static_cast<unsigned>(exportCount), clearResult, cleared,
		clearMatched, clearMatched == exportCount ? 0u : 1u);
	printf("simulation fluid-model-unload row=003902 result=%u module_after=%u matched=%u mismatches=%u\n",
		unloaded, moduleAfter, unloaded == 1 && moduleAfter == 0 ? 1u : 0u,
		unloaded == 1 && moduleAfter == 0 ? 0u : 1u);
	return loaded && resolved && unloaded && matched == exportCount &&
		clearResult == 1 && cleared == exportCount && moduleAfter == 0;
	}

static int nxSimulationRunFluidModelExportResolutionProbe(int argc,
	wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	const int status = nxOpenPair(argc - 1, argv + 1,
		"NxPhysicsSimulationTests --fluid-model-export-resolution",
		pairDirectory, &physics);
	if(status)
		return status;
	const bool matched = nxSimulationTestFluidModelExportResolution(physics,
		pairDirectory);
	FreeLibrary(physics);
	return matched ? 0 : 1;
	}

static bool nxSimulationRunFluidModelExportResolutionChild(
	const wchar_t* pairDirectory)
	{
	wchar_t executable[MAX_PATH];
	const DWORD executableLength = GetModuleFileNameW(0, executable, MAX_PATH);
	if(!executableLength || executableLength >= MAX_PATH)
		return false;
	wchar_t commandLine[2 * MAX_PATH + 100];
	const int commandLength = _snwprintf_s(commandLine, _countof(commandLine),
		_TRUNCATE, L"\"%s\" --fluid-model-export-resolution \"%s\"",
		executable, pairDirectory);
	if(commandLength < 0)
		return false;
	STARTUPINFOW startup = {};
	startup.cb = sizeof(startup);
	startup.dwFlags = STARTF_USESTDHANDLES;
	startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	PROCESS_INFORMATION process = {};
	if(!CreateProcessW(executable, commandLine, 0, 0, TRUE, CREATE_NO_WINDOW,
		0, 0, &startup, &process))
		return false;
	const DWORD waitResult = WaitForSingleObject(process.hProcess, 30000);
	if(waitResult != WAIT_OBJECT_0)
		{
		TerminateProcess(process.hProcess, 1);
		WaitForSingleObject(process.hProcess, INFINITE);
		}
	DWORD exitCode = 1;
	GetExitCodeProcess(process.hProcess, &exitCode);
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	return waitResult == WAIT_OBJECT_0 && exitCode == 0;
	}

static bool nxSimulationTestFluidModelLoadX87(HMODULE physics,
	const wchar_t* pairDirectory)
	{
	void* function = 0;
	if(!nxSimulationResolvePrivateRow(physics, pairDirectory,
		"?nxFluidManagerResolveModel@@YA_NXZ", 0x0008d820u, &function))
		return false;
	if(!SetEnvironmentVariableW(L"NX_TEST_FLUID_MODEL_X87_MUTATION", L"1") ||
		!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_USER_DIRS))
		return false;
	DLL_DIRECTORY_COOKIE directoryCookie = AddDllDirectory(pairDirectory);
	if(!directoryCookie)
		return false;
	const unsigned short before = nxSimulationReadX87ControlWord();
	typedef bool (__cdecl* ResolveFluidModelFn)();
	const bool initialized = reinterpret_cast<ResolveFluidModelFn>(function)();
	HMODULE fluidModel = GetModuleHandleA("FluidModel.DLL");
	const bool loaded = fluidModel != 0;
	wchar_t loadedPath[MAX_PATH] = {};
	const DWORD loadedPathLength = loaded ? GetModuleFileNameW(fluidModel,
		loadedPath, _countof(loadedPath)) : 0;
	const size_t pairDirectoryLength = wcslen(pairDirectory);
	const bool staged = loadedPathLength > pairDirectoryLength &&
		_wcsnicmp(loadedPath, pairDirectory, pairDirectoryLength) == 0 &&
		(loadedPath[pairDirectoryLength] == L'\\' ||
		loadedPath[pairDirectoryLength] == L'/');
	const unsigned short after = nxSimulationReadX87ControlWord();
	SetEnvironmentVariableW(L"NX_TEST_FLUID_MODEL_X87_MUTATION", 0);
	RemoveDllDirectory(directoryCookie);
	__asm { fldcw before }
	printf("simulation fluid-model-load-x87 row=003900 loaded=%u initialized=%u staged=%u control_before=%04x control_after=%04x matched=%u\n",
		loaded ? 1u : 0u, initialized ? 1u : 0u, staged ? 1u : 0u,
		before, after, loaded && initialized && staged && before == after ? 1u : 0u);
	return loaded && initialized && staged && before == after;
	}

static int nxSimulationRunFluidModelLoadX87Probe(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	const int status = nxOpenPair(argc - 1, argv + 1,
		"NxPhysicsSimulationTests --fluid-model-load-x87", pairDirectory, &physics);
	if(status)
		return status;
	const bool matched = nxSimulationTestFluidModelLoadX87(physics, pairDirectory);
	FreeLibrary(physics);
	return matched ? 0 : 1;
	}

static bool nxSimulationRunFluidModelLoadX87Child(const wchar_t* pairDirectory)
	{
	wchar_t executable[MAX_PATH];
	const DWORD executableLength = GetModuleFileNameW(0, executable, MAX_PATH);
	if(!executableLength || executableLength >= MAX_PATH)
		return false;
	wchar_t commandLine[2 * MAX_PATH + 80];
	const int commandLength = _snwprintf_s(commandLine, _countof(commandLine), _TRUNCATE,
		L"\"%s\" --fluid-model-load-x87 \"%s\"", executable, pairDirectory);
	if(commandLength < 0)
		return false;
	STARTUPINFOW startup = {};
	startup.cb = sizeof(startup);
	startup.dwFlags = STARTF_USESTDHANDLES;
	startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
	PROCESS_INFORMATION process = {};
	if(!CreateProcessW(executable, commandLine, 0, 0, TRUE, CREATE_NO_WINDOW,
		0, 0, &startup, &process))
		return false;
	const DWORD waitResult = WaitForSingleObject(process.hProcess, 30000);
	if(waitResult != WAIT_OBJECT_0)
		{
		TerminateProcess(process.hProcess, 1);
		WaitForSingleObject(process.hProcess, INFINITE);
		}
	DWORD exitCode = 1;
	GetExitCodeProcess(process.hProcess, &exitCode);
	CloseHandle(process.hThread);
	CloseHandle(process.hProcess);
	return waitResult == WAIT_OBJECT_0 && exitCode == 0;
	}

struct NxSimulationActorPairCleanupFixture
	{
	NxU8 scene[0x48];
	NxI32 buckets[4];
	NxI32 links[1];
	NxU8 entries[8];
	NxU8 body[0x14];
	NxU8 root[0xd8];
	};

static NxU32 nxSimulationPairBucket(NxU16 owner0, NxU16 owner1)
	{
	if(owner1 < owner0)
		{
		const NxU16 swap = owner0;
		owner0 = owner1;
		owner1 = swap;
		}
	NxU32 hash = ((NxU32) owner1 << 16) | owner0;
	hash += ~(hash << 15);
	hash = (((NxU32) ((NxI32) hash >> 10) ^ hash) * 9);
	hash ^= (NxU32) ((NxI32) hash >> 6);
	hash += ~(hash << 11);
	return (((NxU32) ((NxI32) hash >> 16) ^ hash) & 3u);
	}

static void nxSimulationInitActorPairCleanupFixture(
	NxSimulationActorPairCleanupFixture& fixture, bool hasRoot)
	{
	memset(&fixture, 0, sizeof(fixture));
	NxU8* const map = fixture.scene + 0x2c;
	*reinterpret_cast<NxU32*>(map + 4) = 3;
	*reinterpret_cast<NxI32**>(map + 8) = fixture.buckets;
	*reinterpret_cast<NxI32**>(map + 0x0c) = fixture.links;
	*reinterpret_cast<NxU32*>(map + 0x10) = 1;
	*reinterpret_cast<NxU8**>(map + 0x14) = fixture.entries;
	*reinterpret_cast<NxU32*>(map + 0x18) = 0xffffffffu;
	for(unsigned i = 0; i < 4; ++i)
		fixture.buckets[i] = -1;
	fixture.buckets[nxSimulationPairBucket(0x1234, 0x5678)] = 0;
	fixture.links[0] = -1;
	*reinterpret_cast<NxU16*>(fixture.entries) = 0x1234;
	*reinterpret_cast<NxU16*>(fixture.entries + 2) = 0x5678;
	*reinterpret_cast<NxU32*>(fixture.entries + 4) = 1; // tagged payload: no allocator call
	*reinterpret_cast<void**>(fixture.body + 0x10) = hasRoot ? fixture.root : 0;
	*reinterpret_cast<NxU32*>(fixture.root + 0xd4) = 0x5678;
	}

static bool nxSimulationSameActorPairCleanupState(
	const NxSimulationActorPairCleanupFixture& a,
	const NxSimulationActorPairCleanupFixture& b)
	{
	const NxU8* const mapA = a.scene + 0x2c;
	const NxU8* const mapB = b.scene + 0x2c;
	return memcmp(mapA + 4, mapB + 4, 4) == 0 &&
		memcmp(mapA + 0x10, mapB + 0x10, 4) == 0 &&
		memcmp(mapA + 0x18, mapB + 0x18, 4) == 0 &&
		memcmp(a.buckets, b.buckets, sizeof(a.buckets)) == 0 &&
		memcmp(a.links, b.links, sizeof(a.links)) == 0 &&
		memcmp(a.entries, b.entries, sizeof(a.entries)) == 0;
	}

static bool nxSimulationTestActorOwnerPairCleanupRow521(HMODULE physics,
	const wchar_t* pairDirectory)
	{
	void* row521Address = 0;
	if(!nxSimulationResolvePrivateRow(physics, pairDirectory,
		"?removeActorOwnerPairRecords@NxSceneInternal@@QAEXPBX@Z",
		0x000103e0, &row521Address))
		return false;
	typedef void (__thiscall *ActorPairCleanupFn)(void*, const void*);
	ActorPairCleanupFn const row521 = reinterpret_cast<ActorPairCleanupFn>(row521Address);
	NxSimulationActorPairCleanupFixture oracleRoot, candidateRoot;
	nxSimulationInitActorPairCleanupFixture(oracleRoot, true);
	nxSimulationInitActorPairCleanupFixture(candidateRoot, true);
	row521(oracleRoot.scene, oracleRoot.body);
	row521(candidateRoot.scene, candidateRoot.body);
	const NxU32 oracleRootCount = *reinterpret_cast<NxU32*>(oracleRoot.scene + 0x3c);
	const NxU32 candidateRootCount = *reinterpret_cast<NxU32*>(candidateRoot.scene + 0x3c);
	const bool rootRecordRemoved = oracleRootCount == 0 && candidateRootCount == 0 &&
		*reinterpret_cast<NxU16*>(oracleRoot.entries) == 0xffff &&
		*reinterpret_cast<NxU16*>(candidateRoot.entries) == 0xffff &&
		oracleRoot.buckets[nxSimulationPairBucket(0x1234, 0x5678)] == -1 &&
		candidateRoot.buckets[nxSimulationPairBucket(0x1234, 0x5678)] == -1 &&
		nxSimulationSameActorPairCleanupState(oracleRoot, candidateRoot);
	NxSimulationActorPairCleanupFixture oracleNoRoot, candidateNoRoot;
	nxSimulationInitActorPairCleanupFixture(oracleNoRoot, false);
	nxSimulationInitActorPairCleanupFixture(candidateNoRoot, false);
 row521(oracleNoRoot.scene, oracleNoRoot.body);
	row521(candidateNoRoot.scene, candidateNoRoot.body);
	const bool noRootPreserved =
		*reinterpret_cast<NxU32*>(oracleNoRoot.scene + 0x3c) == 1 &&
		*reinterpret_cast<NxU32*>(candidateNoRoot.scene + 0x3c) == 1 &&
		*reinterpret_cast<NxU16*>(oracleNoRoot.entries) == 0x1234 &&
		*reinterpret_cast<NxU16*>(candidateNoRoot.entries) == 0x1234 &&
		nxSimulationSameActorPairCleanupState(oracleNoRoot, candidateNoRoot);
	printf("simulation row521 direct root_removed=%u no_root_preserved=%u\n",
		rootRecordRemoved, noRootPreserved);
	return rootRecordRemoved && noRootPreserved;
	}

static bool nxSimulationTestDirtyIslandRow720(HMODULE physics,
	const wchar_t* pairDirectory, NxSimulationCountingAllocator& allocator)
	{
	void* row720Address = 0;
	void* row4167Address = 0;
	if(!nxSimulationResolvePrivateRow(physics, pairDirectory,
		"?row000720@Row000720Fixture@@QAEXXZ", 0x00016100, &row720Address) ||
		!nxSimulationResolvePrivateRow(physics, pairDirectory,
		"?row004167@Row004167Fixture@@QAEXXZ", 0x0009ad10, &row4167Address))
		return false;
	alignas(16) unsigned char root[0x260] = {};
	*reinterpret_cast<void**>(root + 0x1bc) = root;
	*reinterpret_cast<NxU32*>(root + 0x1e4) = 2;
	const unsigned mallocsBefore = allocator.mallocCalls;
	const unsigned freesBefore = allocator.freeCalls;
	const unsigned liveBefore = allocator.liveAllocations;
	typedef void (__thiscall *IslandRowFn)(void*);
	reinterpret_cast<IslandRowFn>(row720Address)(root);
	const NxU32 flags = *reinterpret_cast<NxU32*>(root + 0x1e4);
	void* const island = *reinterpret_cast<void**>(root + 0x1e0);
	printf("simulation island-row720 direct dirty_cleared=%u traversed=%u island=%u mallocs=%u frees=%u live_delta=%d\n",
		(flags & 2u) == 0, (flags & 8u) != 0, island != 0,
		allocator.mallocCalls - mallocsBefore, allocator.freeCalls - freesBefore,
		static_cast<int>(allocator.liveAllocations) - static_cast<int>(liveBefore));
	const bool rebuilt = (flags & 2u) == 0 && (flags & 8u) != 0 && island != 0;
	if(island)
		{
		reinterpret_cast<IslandRowFn>(row4167Address)(island);
		allocator.free(island);
		*reinterpret_cast<void**>(root + 0x1e0) = 0;
		}
	return rebuilt;
	}

static bool nxSimulationTestJointContactRow750(HMODULE physics,
	const wchar_t* pairDirectory)
	{
	void* row750Address = 0;
	if(!nxSimulationResolvePrivateRow(physics, pairDirectory,
		"?row000750@CpmJointedBody@@QBEIPBU1@@Z", 0x00016fb0, &row750Address))
		return false;
	alignas(16) unsigned char body[0x220] = {};
	alignas(16) unsigned char other[0x220] = {};
	alignas(16) unsigned char joint[0x40] = {};
	*reinterpret_cast<void**>(body + 0x1d8) = joint;
	*reinterpret_cast<void**>(joint + 0x08) = body;
	*reinterpret_cast<void**>(joint + 0x0c) = other;
	typedef NxU32 (__thiscall *JointContactRowFn)(const void*, const void*);
	JointContactRowFn const row750 = reinterpret_cast<JointContactRowFn>(row750Address);
	const NxU32 ownerListContactsEnabled = row750(body, other);
	*reinterpret_cast<NxU32*>(joint + 0x2c) = 0x100;
	const NxU32 ownerListContactsDisabled = row750(body, other);
	*reinterpret_cast<void**>(body + 0x1d8) = 0;
	*reinterpret_cast<void**>(other + 0x1d8) = joint;
	*reinterpret_cast<NxU32*>(joint + 0x2c) = 0;
	const NxU32 otherListContactsEnabled = row750(body, other);
	*reinterpret_cast<NxU32*>(joint + 0x2c) = 0x100;
	const NxU32 otherListContactsDisabled = row750(body, other);
	printf("simulation row750 joint-contact owner_scan=%u.%u fallback_scan=%u.%u\n",
		ownerListContactsEnabled, ownerListContactsDisabled,
		otherListContactsEnabled, otherListContactsDisabled);
	return ownerListContactsEnabled == 1 && ownerListContactsDisabled == 0 &&
		otherListContactsEnabled == 1 && otherListContactsDisabled == 0;
	}

static bool nxSimulationTestMaterialCombineRow855(HMODULE physics,
	const wchar_t* pairDirectory)
	{
	void* row855Address = 0;
	if(!nxSimulationResolvePrivateRow(physics, pairDirectory,
		"?cpmCombine0855@@YGNMMI@Z", 0x0001ca00, &row855Address))
		return false;
	typedef double (__stdcall *MaterialCombineFn)(NxReal, NxReal, NxU32);
	MaterialCombineFn const combine = reinterpret_cast<MaterialCombineFn>(row855Address);
	const NxU32 average = nxFloatBits(static_cast<NxReal>(combine(2.0f, 6.0f, 0)));
	const NxU32 minimum = nxFloatBits(static_cast<NxReal>(combine(2.0f, 6.0f, 1)));
	const NxU32 multiply = nxFloatBits(static_cast<NxReal>(combine(2.0f, 6.0f, 2)));
	const NxU32 maximum = nxFloatBits(static_cast<NxReal>(combine(2.0f, 6.0f, 3)));
	const NxU32 unknown = nxFloatBits(static_cast<NxReal>(combine(2.0f, 6.0f, 9)));
	printf("simulation row855 direct combine=%08x.%08x.%08x.%08x.%08x\n",
		average, minimum, multiply, maximum, unknown);
	return average == 0x40800000 && minimum == 0x40000000 &&
		multiply == 0x41400000 && maximum == 0x40c00000 &&
		unknown == 0x40c00000;
	}

struct NxSimulationFrictionParamsProbe
	{
	NxU32 spring;
	NxReal staticFrictionV;
	NxReal staticFriction;
	NxReal dynamicFriction;
	NxReal dynamicFrictionV;
	NxVec3 anisotropyDirection;
	NxU8 enabled;
	NxU8 pad[3];
	};

static_assert(sizeof(NxSimulationFrictionParamsProbe) == 0x24,
	"the row 000859 probe mirrors the private friction-parameter record");

static bool nxSimulationTestAnisotropicFrictionRow859(HMODULE physics,
	const wchar_t* pairDirectory)
	{
	void* row859Address = 0;
	if(!nxSimulationResolvePrivateRow(physics, pairDirectory,
		"?cpmAnisotropicFriction0859@@YGXPBEPBVNxMaterial@@1MPAUCpmFrictionParams@@@Z",
		0x0001cb30, &row859Address))
		return false;
	alignas(16) NxU8 shape[0x30] = {};
	const NxReal rotatedZ[9] = { 0.0f, -1.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };
	memcpy(shape + 0x0c, rotatedZ, sizeof(rotatedZ));
	NxMaterial anisotropic;
	anisotropic.setToDefault();
	anisotropic.flags = NX_MF_ANISOTROPIC;
	anisotropic.dynamicFriction = 0.25f;
	anisotropic.staticFriction = 0.5f;
	anisotropic.dynamicFrictionV = 0.75f;
	anisotropic.staticFrictionV = 1.0f;
	anisotropic.dirOfAnisotropy.set(1.0f, 0.0f, 0.0f);
	NxMaterial isotropic;
	isotropic.setToDefault();
	isotropic.dynamicFriction = 0.45f;
	isotropic.staticFriction = 0.85f;
	NxMaterial secondAnisotropic;
	secondAnisotropic.setToDefault();
	secondAnisotropic.flags = NX_MF_ANISOTROPIC;
	secondAnisotropic.dynamicFriction = 0.3f;
	secondAnisotropic.dynamicFrictionV = 0.7f;
	secondAnisotropic.staticFriction = 0.6f;
	secondAnisotropic.staticFrictionV = 1.0f;
	NxSimulationFrictionParamsProbe oneAnisotropic = {};
	NxSimulationFrictionParamsProbe twoAnisotropic = {};
	typedef void (__stdcall *AnisotropicFrictionFn)(const NxU8*,
		const NxMaterial*, const NxMaterial*, NxReal,
		NxSimulationFrictionParamsProbe*);
	AnisotropicFrictionFn const row859 =
		reinterpret_cast<AnisotropicFrictionFn>(row859Address);
	row859(shape, &anisotropic, &isotropic, 2.0f, &oneAnisotropic);
	row859(shape, &anisotropic, &secondAnisotropic, 2.0f, &twoAnisotropic);
	const bool rotated = nxFloatBits(oneAnisotropic.anisotropyDirection.x) == 0 &&
		nxFloatBits(oneAnisotropic.anisotropyDirection.y) == 0x3f800000 &&
		nxFloatBits(oneAnisotropic.anisotropyDirection.z) == 0 &&
		nxFloatBits(twoAnisotropic.anisotropyDirection.x) == 0 &&
		nxFloatBits(twoAnisotropic.anisotropyDirection.y) == 0x3f800000 &&
		nxFloatBits(twoAnisotropic.anisotropyDirection.z) == 0;
	const bool oneHasUAndV = oneAnisotropic.dynamicFriction > 0.0f &&
		oneAnisotropic.dynamicFrictionV > 0.0f &&
		oneAnisotropic.staticFriction > oneAnisotropic.dynamicFriction &&
		oneAnisotropic.staticFrictionV > oneAnisotropic.dynamicFrictionV;
	const bool twoHasUAndV = twoAnisotropic.dynamicFriction > 0.0f &&
		twoAnisotropic.dynamicFrictionV > 0.0f &&
		twoAnisotropic.staticFriction > twoAnisotropic.dynamicFriction &&
		twoAnisotropic.staticFrictionV > twoAnisotropic.dynamicFrictionV;
	const bool otherMaterialAveraged =
		nxFloatBits(oneAnisotropic.dynamicFriction) !=
			nxFloatBits(twoAnisotropic.dynamicFriction) &&
		nxFloatBits(oneAnisotropic.dynamicFrictionV) !=
			nxFloatBits(twoAnisotropic.dynamicFrictionV);
	printf("simulation row859 direct rotated=%u one_anisotropic=%u two_anisotropic=%u other_average=%u\n",
		rotated, oneHasUAndV, twoHasUAndV, otherMaterialAveraged);
	printf("simulation row859 coefficients one=%08x.%08x.%08x.%08x two=%08x.%08x.%08x.%08x\n",
		nxFloatBits(oneAnisotropic.staticFriction),
		nxFloatBits(oneAnisotropic.staticFrictionV),
		nxFloatBits(oneAnisotropic.dynamicFriction),
		nxFloatBits(oneAnisotropic.dynamicFrictionV),
		nxFloatBits(twoAnisotropic.staticFriction),
		nxFloatBits(twoAnisotropic.staticFrictionV),
		nxFloatBits(twoAnisotropic.dynamicFriction),
		nxFloatBits(twoAnisotropic.dynamicFrictionV));
	return rotated && oneHasUAndV && twoHasUAndV && otherMaterialAveraged;
	}

static bool nxSimulationTestAccumulatorRow867(HMODULE physics,
	const wchar_t* pairDirectory)
	{
	void* row867Address = 0;
	if(!nxSimulationResolvePrivateRow(physics, pairDirectory,
		"?nxAccumulateByKind0867@@YAXPAXM0M@Z", 0x0001d260, &row867Address))
		return false;
	wchar_t mapPath[MAX_PATH];
	if(swprintf_s(mapPath, L"%s\\NxPhysics.map", pairDirectory) < 0)
		return false;
	const bool candidate = GetFileAttributesW(mapPath) != INVALID_FILE_ATTRIBUTES;
	typedef void (__thiscall *OracleAccumulatorFn)(void*, NxReal, void*, NxReal);
	typedef void (__cdecl *CandidateAccumulatorFn)(void*, NxReal, void*, NxReal);
	OracleAccumulatorFn const oracleRow = reinterpret_cast<OracleAccumulatorFn>(row867Address);
	CandidateAccumulatorFn const candidateRow = reinterpret_cast<CandidateAccumulatorFn>(row867Address);
	struct Case { NxU32 kindAndFlags; NxReal c[3]; };
	static const Case cases[] = {
		{ 4, { 1.5f, -2.0f, 0.25f } },
		{ 5 | 0x40, { -0.5f, 3.0f, 1.0f } },
		{ 5, { 2.0f, 2.0f, 2.0f } },
		{ 7, { 1.0f, 1.0f, 1.0f } },
		{ 0x24, { 1.0f, 1.0f, 1.0f } }
		};
	NxU32 digest = 2166136261u;
	for(unsigned i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i)
	for(unsigned value = 0; value < 3; ++value)
		{
		NxReal a = 1.0f, b = 2.0f;
		if(value == 1) { a = -3.0f; b = 4.0f; }
		if(value == 2) { a = 0.0f; b = 1.0f; }
		alignas(16) NxU8 desc[0x20] = {};
		memcpy(desc, cases[i].c, sizeof(cases[i].c));
		memcpy(desc + 0x0c, &cases[i].kindAndFlags, sizeof(NxU32));
		alignas(16) NxU8 object[0x100] = {};
		const NxReal seed[4] = { 0.5f, -0.25f, 1.25f, 0.75f };
		memcpy(object + 0x58, seed, sizeof(seed));
		if(candidate)
			candidateRow(object, a, desc, b);
		else
			oracleRow(object, a, desc, b);
		for(unsigned byte = 0; byte < sizeof(object); ++byte)
			digest = (digest ^ object[byte]) * 16777619u;
		}
	printf("simulation row867 direct cases=15 digest=%08x\n", digest);
	return true;
	}

static bool nxSimulationTestJointArrayPushRow780(HMODULE physics,
	const wchar_t* pairDirectory)
	{
	void* row778Address = 0;
	if(!nxSimulationResolvePrivateRow(physics, pairDirectory,
		"?row000778@Row000778Fixture@@QAEXPAXPAPAX@Z", 0x000185f0, &row778Address))
		return false;
	alignas(16) unsigned char body[0x240] = {};
	alignas(16) unsigned char linkedJoint[0x40] = {};
	alignas(16) unsigned char excludedJoint[0x40] = {};
	void* storage[4] = {};
	void* jointArray[3] = { storage, storage, storage + 4 };
	*reinterpret_cast<void**>(body + 0x1bc) = body;
	*reinterpret_cast<void**>(body + 0x1d8) = linkedJoint;
	*reinterpret_cast<NxU32*>(body + 0x114) = 0x100;
	*reinterpret_cast<void**>(linkedJoint + 0x34) = excludedJoint;
	typedef void (__thiscall *JointDissolveFn)(void*, void*, void**);
	reinterpret_cast<JointDissolveFn>(row778Address)(body, excludedJoint, jointArray);
	void** const arrayBegin = static_cast<void**>(jointArray[0]);
	void** const arrayEnd = static_cast<void**>(jointArray[1]);
	const NxU32 queued = static_cast<NxU32>(arrayEnd - arrayBegin);
	const bool onlyOtherJointQueued = queued == 1 && arrayBegin[0] == linkedJoint;
	const bool linksCleared = *reinterpret_cast<void**>(linkedJoint + 0x34) == 0 &&
		*reinterpret_cast<void**>(excludedJoint + 0x34) == 0;
	const bool bodyReset = *reinterpret_cast<void**>(body + 0x1bc) == body &&
		*reinterpret_cast<void**>(body + 0x1d8) == 0;
	printf("simulation row780 direct queued=%u linked_cleared=%u body_reset=%u\n",
		onlyOtherJointQueued, linksCleared, bodyReset);
	return onlyOtherJointQueued && linksCleared && bodyReset;
	}

struct NxSimulationSurfaceMeshDtorProbe
	{
	unsigned calls;
	void* object;
	unsigned flags;
	};

static NxSimulationSurfaceMeshDtorProbe gNxSimulationSurfaceMeshDtorProbe = {};

static void* __fastcall nxSimulationSurfaceMeshDtorProbe(void* object,
	void*, unsigned char flags)
	{
	++gNxSimulationSurfaceMeshDtorProbe.calls;
	gNxSimulationSurfaceMeshDtorProbe.object = object;
	gNxSimulationSurfaceMeshDtorProbe.flags = flags;
	return reinterpret_cast<void*>(static_cast<size_t>(0x13572468u));
	}

static bool nxSimulationTestSurfaceMeshReleaseRow3421(HMODULE physics,
	const wchar_t* pairDirectory)
	{
	void* rowAddress = 0;
	if(!nxSimulationResolvePrivateRow(physics, pairDirectory,
		"?releaseSurfaceMesh@NxFluidSurfaceMeshOwnerSlot24@@QAEPAXXZ",
		0x00084a60,
		&rowAddress))
		return false;
	typedef void* (__thiscall *ReleaseSurfaceMeshFn)(void*);
	ReleaseSurfaceMeshFn const release =
		reinterpret_cast<ReleaseSurfaceMeshFn>(rowAddress);
	unsigned char owner[0x28] = {};
	unsigned char object[0x10] = {};
	void* vtable[1] = { reinterpret_cast<void*>(
		nxSimulationSurfaceMeshDtorProbe) };
	*reinterpret_cast<void***>(object) = vtable;
	*reinterpret_cast<void**>(owner + 0x24) = object;
	gNxSimulationSurfaceMeshDtorProbe = {};
	void* const result = release(owner);
	const bool calledOnce = gNxSimulationSurfaceMeshDtorProbe.calls == 1 &&
		gNxSimulationSurfaceMeshDtorProbe.object == object &&
		gNxSimulationSurfaceMeshDtorProbe.flags == 1;
	const bool cleared = *reinterpret_cast<void**>(owner + 0x24) == 0;
	const bool returned = result == reinterpret_cast<void*>(
		static_cast<size_t>(0x13572468u));
	printf("simulation row3421 surface_mesh_release called=%u cleared=%u result=%u\n",
		calledOnce ? 1u : 0u, cleared ? 1u : 0u, returned ? 1u : 0u);
	return calledOnce && cleared && returned;
	}

int wmain(int argc, wchar_t** argv)
	{
	setvbuf(stdout, 0, _IONBF, 0);
	if(argc == 3 && wcscmp(argv[1], L"--fluid-model-load-x87") == 0)
		return nxSimulationRunFluidModelLoadX87Probe(argc, argv);
	if(argc == 3 && wcscmp(argv[1], L"--fluid-model-export-resolution") == 0)
		return nxSimulationRunFluidModelExportResolutionProbe(argc, argv);
	if(argc != 2)
		return nxFail("usage: NxPhysicsSimulationTests <pair directory>");

	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsSimulationTests", pairDirectory, &physics);
	if(status)
		return status;
	if(!nxSimulationRunFluidModelExportResolutionChild(pairDirectory))
		{
		FreeLibrary(physics);
		return nxFail("FluidModel export-table child differential failed");
		}
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK)
		{
		FreeLibrary(physics);
		return nxFail("NxCreatePhysicsSDK missing");
		}

	NxSimulationOutputStream simulationOutput;
	NxSimulationCountingAllocator simulationAllocator;
	bool deferredContactFailed = false;
	bool controllerCreateFailed = false;
	bool shapeUserDataFailed = false;
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, &simulationAllocator, &simulationOutput);
	if(!sdk)
		{
		FreeLibrary(physics);
		return nxFail("SDK creation failed");
		}
	if(!nxSimulationTestActorOwnerPairCleanupRow521(physics, pairDirectory))
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("row 000521 failed the direct actor owner-pair cleanup probe");
		}
	if(!nxSimulationTestSurfaceMeshReleaseRow3421(physics, pairDirectory))
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("row 003421 failed the direct surface-mesh release probe");
		}
	if(!nxSimulationTestDirtyIslandRow720(physics, pairDirectory, simulationAllocator))
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("row 000720 failed the pinned direct dirty-island root probe");
		}
	if(!nxSimulationTestJointContactRow750(physics, pairDirectory))
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("row 000750 failed the pinned direct joint-contact probe");
		}
	if(!nxSimulationTestJointArrayPushRow780(physics, pairDirectory))
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("row 000780 failed the pinned direct joint-array probe");
		}
	if(!nxSimulationTestMaterialCombineRow855(physics, pairDirectory))
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("row 000855 failed the pinned direct material-combine probe");
		}
	if(!nxSimulationTestAnisotropicFrictionRow859(physics, pairDirectory))
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("row 000859 failed the pinned direct anisotropic-friction probe");
		}
	if(!nxSimulationTestAccumulatorRow867(physics, pairDirectory))
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("row 000867 failed the pinned direct accumulator probe");
		}
	// Exercise trigger-pair generation, enter/stay/leave reconciliation, and
	// fetch-time callback delivery through the public scene API.
	NxSceneDesc triggerLifecycleSceneDesc;
	triggerLifecycleSceneDesc.setToDefault();
	NxScene* const triggerLifecycleScene = sdk->createScene(triggerLifecycleSceneDesc);
	if(!triggerLifecycleScene)
		return nxFail("trigger lifecycle scene creation failed");
	triggerLifecycleScene->setGravity(NxVec3(0.0f, 0.0f, 0.0f));
	triggerLifecycleScene->setTiming(0.01f, 1, NX_TIMESTEP_VARIABLE);
	NxBoxShapeDesc lifecycleTriggerShapeDesc;
	lifecycleTriggerShapeDesc.dimensions = NxVec3(1.0f, 1.0f, 1.0f);
	lifecycleTriggerShapeDesc.shapeFlags |= NX_TRIGGER_ENABLE;
	NxActorDesc lifecycleTriggerActorDesc;
	lifecycleTriggerActorDesc.shapes.pushBack(&lifecycleTriggerShapeDesc);
	NxActor* const lifecycleTriggerActor =
		triggerLifecycleScene->createActor(lifecycleTriggerActorDesc);
	NxSphereShapeDesc lifecycleOtherShapeDesc;
	lifecycleOtherShapeDesc.radius = 0.5f;
	NxBodyDesc lifecycleOtherBodyDesc;
	NxActorDesc lifecycleOtherActorDesc;
	lifecycleOtherActorDesc.body = &lifecycleOtherBodyDesc;
	lifecycleOtherActorDesc.density = 1.0f;
	lifecycleOtherActorDesc.shapes.pushBack(&lifecycleOtherShapeDesc);
	NxActor* const lifecycleOtherActor =
		triggerLifecycleScene->createActor(lifecycleOtherActorDesc);
	if(!lifecycleTriggerActor || !lifecycleOtherActor)
		return nxFail("trigger lifecycle actors creation failed");
	NxShape* const lifecycleTriggerShape = lifecycleTriggerActor->getShapes()[0];
	NxShape* const lifecycleOtherShape = lifecycleOtherActor->getShapes()[0];
	NxSimulationTriggerLifecycleReport lifecycleTriggerReport(
		lifecycleTriggerShape, lifecycleOtherShape);
	triggerLifecycleScene->setUserTriggerReport(&lifecycleTriggerReport);
	bool lifecycleFetched = true;
	for(unsigned step = 0; step != 3 && lifecycleFetched; ++step)
		{
		lifecycleOtherActor->setGlobalPosition(NxVec3(0.1f * step, 0.0f, 0.0f));
		triggerLifecycleScene->simulate(0.01f);
		lifecycleFetched = triggerLifecycleScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		printf("simulation trigger-lifecycle tick=%u calls=%u\n", step + 1,
			lifecycleTriggerReport.calls);
		}
	lifecycleOtherActor->setGlobalPosition(NxVec3(4.0f, 0.0f, 0.0f));
	if(lifecycleFetched)
		{
		triggerLifecycleScene->simulate(0.01f);
		lifecycleFetched = triggerLifecycleScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		}
	const bool triggerLifecycleExact = lifecycleFetched &&
		lifecycleTriggerReport.calls == 3 && lifecycleTriggerReport.badShapes == 0 &&
		lifecycleTriggerReport.events[0] == NX_TRIGGER_ON_ENTER &&
		lifecycleTriggerReport.events[1] == NX_TRIGGER_ON_STAY &&
		lifecycleTriggerReport.events[2] == NX_TRIGGER_ON_LEAVE;
	printf("simulation trigger-lifecycle summary fetched=%u calls=%u bad_shapes=%u exact=%u\n",
		lifecycleFetched, lifecycleTriggerReport.calls,
		lifecycleTriggerReport.badShapes, triggerLifecycleExact);
	triggerLifecycleScene->releaseActor(*lifecycleOtherActor);
	triggerLifecycleScene->releaseActor(*lifecycleTriggerActor);
	sdk->releaseScene(*triggerLifecycleScene);
	if(!triggerLifecycleExact)
		return nxFail("trigger enter/stay/leave lifecycle did not match the public contract");
	// Several live pairs exercise hash collisions and event-buffer growth in one
	// scene. Each body enters, remains for a fetched step, then leaves.
	NxSceneDesc triggerBatchSceneDesc;
	triggerBatchSceneDesc.setToDefault();
	NxScene* const triggerBatchScene = sdk->createScene(triggerBatchSceneDesc);
	if(!triggerBatchScene)
		return nxFail("trigger batch scene creation failed");
	triggerBatchScene->setGravity(NxVec3(0.0f, 0.0f, 0.0f));
	triggerBatchScene->setTiming(0.01f, 1, NX_TIMESTEP_VARIABLE);
	NxBoxShapeDesc batchTriggerShapeDesc;
	batchTriggerShapeDesc.dimensions = NxVec3(2.0f, 2.0f, 2.0f);
	batchTriggerShapeDesc.shapeFlags |= NX_TRIGGER_ENABLE;
	batchTriggerShapeDesc.shapeFlags = (batchTriggerShapeDesc.shapeFlags & ~NX_TRIGGER_ENABLE) |
		NX_TRIGGER_ON_STAY;
	NxActorDesc batchTriggerActorDesc;
	batchTriggerActorDesc.shapes.pushBack(&batchTriggerShapeDesc);
	NxActor* const batchTriggerActor = triggerBatchScene->createActor(batchTriggerActorDesc);
	NxActor* batchOtherActors[NxSimulationTriggerBatchReport::BatchCount] = {};
	NxShape* batchOtherShapes[NxSimulationTriggerBatchReport::BatchCount] = {};
	for(unsigned index = 0; index != NxSimulationTriggerBatchReport::BatchCount; ++index)
		{
		NxSphereShapeDesc batchOtherShapeDesc;
		batchOtherShapeDesc.radius = 0.07f;
		NxBodyDesc batchOtherBodyDesc;
		NxActorDesc batchOtherActorDesc;
		batchOtherActorDesc.body = &batchOtherBodyDesc;
		batchOtherActorDesc.density = 1.0f;
		batchOtherActorDesc.globalPose.t = NxVec3(-1.55f + 0.1f * index, 0.0f, 0.0f);
		batchOtherActorDesc.shapes.pushBack(&batchOtherShapeDesc);
		batchOtherActors[index] = triggerBatchScene->createActor(batchOtherActorDesc);
		if(!batchOtherActors[index])
			return nxFail("trigger batch dynamic actor creation failed");
		batchOtherShapes[index] = batchOtherActors[index]->getShapes()[0];
		}
	if(!batchTriggerActor)
		return nxFail("trigger batch trigger creation failed");
	NxSimulationTriggerBatchReport triggerBatchReport(
		batchTriggerActor->getShapes()[0], batchOtherShapes);
	triggerBatchScene->setUserTriggerReport(&triggerBatchReport);
	bool triggerBatchFetched = true;
	for(unsigned step = 0; step != 3 && triggerBatchFetched; ++step)
		{
		if(step < 2)
			for(unsigned index = 0; index != NxSimulationTriggerBatchReport::BatchCount; ++index)
				batchOtherActors[index]->setGlobalPosition(
				NxVec3(-1.55f + 0.1f * index + 0.05f * step, 0.0f, 0.0f));
		triggerBatchScene->simulate(0.01f);
		triggerBatchFetched = triggerBatchScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		printf("simulation trigger-batch tick=%u calls=%u\n", step + 1,
			triggerBatchReport.calls);
		if(step == 1)
			for(unsigned index = 0; index != NxSimulationTriggerBatchReport::BatchCount; ++index)
				batchOtherActors[index]->putToSleep();
		}
	for(unsigned index = 0; index != NxSimulationTriggerBatchReport::BatchCount; ++index)
		batchOtherActors[index]->setGlobalPosition(NxVec3(4.0f + index, 0.0f, 0.0f));
	if(triggerBatchFetched)
		{
		triggerBatchScene->simulate(0.01f);
		triggerBatchFetched = triggerBatchScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		}
	bool triggerBatchExact = triggerBatchFetched && triggerBatchReport.calls == 96 &&
		triggerBatchReport.badShapes == 0;
	for(unsigned index = 0; index != NxSimulationTriggerBatchReport::BatchCount; ++index)
		for(unsigned event = 0; event != 3; ++event)
			triggerBatchExact = triggerBatchExact &&
				triggerBatchReport.eventCounts[index][event] == 1;
	printf("simulation trigger-batch summary fetched=%u calls=%u bad_shapes=%u exact=%u\n",
		triggerBatchFetched, triggerBatchReport.calls,
		triggerBatchReport.badShapes, triggerBatchExact);
	for(unsigned index = 0; index != NxSimulationTriggerBatchReport::BatchCount; ++index)
		triggerBatchScene->releaseActor(*batchOtherActors[index]);
	triggerBatchScene->releaseActor(*batchTriggerActor);
	sdk->releaseScene(*triggerBatchScene);
	if(!triggerBatchExact)
		return nxFail("multi-pair trigger lifecycle did not match the public contract");
	// Two trigger shapes on one actor must retain separate overlap pairs even
	// when both shapes overlap the same dynamic actor.
	NxSceneDesc triggerCompoundSceneDesc;
	triggerCompoundSceneDesc.setToDefault();
	NxScene* const triggerCompoundScene = sdk->createScene(triggerCompoundSceneDesc);
	if(!triggerCompoundScene)
		return nxFail("compound trigger scene creation failed");
	triggerCompoundScene->setGravity(NxVec3(0.0f, 0.0f, 0.0f));
	triggerCompoundScene->setTiming(0.01f, 1, NX_TIMESTEP_VARIABLE);
	NxBoxShapeDesc compoundTriggerShapeDescs[2];
	for(unsigned index = 0; index != 2; ++index)
		{
		compoundTriggerShapeDescs[index].dimensions = NxVec3(1.0f, 1.0f, 1.0f);
		compoundTriggerShapeDescs[index].shapeFlags |= NX_TRIGGER_ENABLE;
		}
	NxActorDesc compoundTriggerActorDesc;
	compoundTriggerActorDesc.shapes.pushBack(&compoundTriggerShapeDescs[0]);
	compoundTriggerActorDesc.shapes.pushBack(&compoundTriggerShapeDescs[1]);
	NxActor* const compoundTriggerActor =
		triggerCompoundScene->createActor(compoundTriggerActorDesc);
	NxSphereShapeDesc compoundOtherShapeDesc;
	compoundOtherShapeDesc.radius = 0.25f;
	NxBodyDesc compoundOtherBodyDesc;
	NxActorDesc compoundOtherActorDesc;
	compoundOtherActorDesc.body = &compoundOtherBodyDesc;
	compoundOtherActorDesc.density = 1.0f;
	compoundOtherActorDesc.shapes.pushBack(&compoundOtherShapeDesc);
	NxActor* const compoundOtherActor =
		triggerCompoundScene->createActor(compoundOtherActorDesc);
	if(!compoundTriggerActor || !compoundOtherActor)
		return nxFail("compound trigger actor creation failed");
	NxShape* const compoundTriggerShapes[] = {
		compoundTriggerActor->getShapes()[0], compoundTriggerActor->getShapes()[1]
		};
	NxSimulationCompoundTriggerReport compoundTriggerReport(
		compoundTriggerShapes, compoundOtherActor->getShapes()[0]);
	triggerCompoundScene->setUserTriggerReport(&compoundTriggerReport);
	bool triggerCompoundFetched = true;
	for(unsigned step = 0; step != 3 && triggerCompoundFetched; ++step)
		{
		triggerCompoundScene->simulate(0.01f);
		triggerCompoundFetched = triggerCompoundScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		}
	compoundOtherActor->setGlobalPosition(NxVec3(4.0f, 0.0f, 0.0f));
	if(triggerCompoundFetched)
		{
		triggerCompoundScene->simulate(0.01f);
		triggerCompoundFetched =
			triggerCompoundScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		}
	bool triggerCompoundExact = triggerCompoundFetched &&
		compoundTriggerReport.calls == 6 && compoundTriggerReport.badShapes == 0;
	for(unsigned trigger = 0; trigger != 2; ++trigger)
		for(unsigned event = 0; event != 3; ++event)
			triggerCompoundExact = triggerCompoundExact &&
				compoundTriggerReport.eventCounts[trigger][event] == 1;
	printf("simulation trigger-compound summary fetched=%u calls=%u bad_shapes=%u exact=%u\n",
		triggerCompoundFetched, compoundTriggerReport.calls,
		compoundTriggerReport.badShapes, triggerCompoundExact);
	triggerCompoundScene->releaseActor(*compoundOtherActor);
	triggerCompoundScene->releaseActor(*compoundTriggerActor);
	sdk->releaseScene(*triggerCompoundScene);
	if(!triggerCompoundExact)
		return nxFail("compound trigger lifecycle did not match the public contract");
	// A public mesh factory smoke case. The oracle accepts this valid two-face
	// descriptor; keeping it in the simulation corpus ensures a rebuilt mesh can
	// become the next real static-contact fixture rather than remaining an
	// internal-only asset object.
	const NxPoint meshPoints[] = {
		NxPoint(-2.0f, 0.0f, -2.0f), NxPoint(2.0f, 0.0f, -2.0f), NxPoint(-2.0f, 0.0f, 2.0f),
		NxPoint(2.0f, 0.0f, -2.0f), NxPoint(2.0f, 0.0f, 2.0f), NxPoint(-2.0f, 0.0f, 2.0f)
		};
	const NxU32 meshTriangles[] = { 0, 1, 2, 3, 4, 5 };
	NxTriangleMeshDesc meshDesc;
	meshDesc.numVertices = sizeof(meshPoints) / sizeof(meshPoints[0]);
	meshDesc.numTriangles = 2;
	meshDesc.pointStrideBytes = sizeof(NxPoint);
	meshDesc.triangleStrideBytes = 3 * sizeof(NxU32);
	meshDesc.points = meshPoints;
	meshDesc.triangles = meshTriangles;
	NxTriangleMesh* const publicMesh = sdk->createTriangleMesh(meshDesc);
	printf("simulation triangle-mesh create=%u\n", publicMesh != 0);
	if(publicMesh)
		{
		const NxU32* const meshIndices = static_cast<const NxU32*>(publicMesh->getBase(0, NX_ARRAY_TRIANGLES));
	const NxPoint* const copiedMeshPoints = static_cast<const NxPoint*>(publicMesh->getBase(0, NX_ARRAY_VERTICES));
	NxTriangleMeshDesc savedMeshDesc;
	const bool savedMesh = publicMesh->saveToDesc(savedMeshDesc);
	printf("simulation triangle-mesh data submeshes=%u vertices=%u triangles=%u vertex_format=%u vertex_stride=%u index_format=%u index_stride=%u first_index=%u last_index=%u first_vertex=%08x.%08x.%08x save=%u saved_counts=%u.%u saved_strides=%u.%u\n",
		publicMesh->getSubmeshCount(), publicMesh->getCount(0, NX_ARRAY_VERTICES),
		publicMesh->getCount(0, NX_ARRAY_TRIANGLES),
		publicMesh->getFormat(0, NX_ARRAY_VERTICES), publicMesh->getStride(0, NX_ARRAY_VERTICES),
		publicMesh->getFormat(0, NX_ARRAY_TRIANGLES), publicMesh->getStride(0, NX_ARRAY_TRIANGLES),
		meshIndices ? meshIndices[0] : 0, meshIndices ? meshIndices[5] : 0,
		copiedMeshPoints ? nxFloatBits(copiedMeshPoints[0].x) : 0,
		copiedMeshPoints ? nxFloatBits(copiedMeshPoints[0].y) : 0,
		copiedMeshPoints ? nxFloatBits(copiedMeshPoints[0].z) : 0,
		savedMesh, savedMeshDesc.numVertices, savedMeshDesc.numTriangles,
		savedMeshDesc.pointStrideBytes, savedMeshDesc.triangleStrideBytes);
		}
	if(publicMesh)
		sdk->releaseTriangleMesh(*publicMesh);
	// A triangle mesh in actual simulation must carry its per-triangle convex
	// and flat-part maps before the convex/mesh narrow phase reads them. This
	// overlapping box exercises the same contact path that was faulting in the
	// staged DemoGame PhysTest run.
	NxSceneDesc meshContactSceneDesc;
	meshContactSceneDesc.setToDefault();
	NxScene* const meshContactScene = sdk->createScene(meshContactSceneDesc);
	if(!meshContactScene)
		return nxFail("triangle-mesh contact scene creation failed");
	const NxU32 contactMeshTriangles[] = { 0, 2, 1, 3, 5, 4 };
	NxTriangleMeshDesc contactMeshDesc = meshDesc;
	contactMeshDesc.triangles = contactMeshTriangles;
	NxTriangleMesh* const contactMesh = sdk->createTriangleMesh(contactMeshDesc);
	if(!contactMesh)
		return nxFail("triangle-mesh contact asset creation failed");
	NxTriangleMeshShapeDesc contactMeshShapeDesc;
	contactMeshShapeDesc.meshData = contactMesh;
	NxActorDesc contactMeshActorDesc;
	contactMeshActorDesc.shapes.pushBack(&contactMeshShapeDesc);
	NxActor* const contactMeshActor = meshContactScene->createActor(contactMeshActorDesc);
	NxBoxShapeDesc contactBoxShapeDesc;
	contactBoxShapeDesc.dimensions = NxVec3(0.5f, 0.5f, 0.5f);
	NxBodyDesc contactBoxBodyDesc;
	NxActorDesc contactBoxActorDesc;
	contactBoxActorDesc.body = &contactBoxBodyDesc;
	contactBoxActorDesc.density = 1.0f;
	contactBoxActorDesc.globalPose.t = NxVec3(0.0f, 0.25f, 0.0f);
	contactBoxActorDesc.shapes.pushBack(&contactBoxShapeDesc);
	NxActor* const contactBoxActor = meshContactScene->createActor(contactBoxActorDesc);
	if(!contactMeshActor || !contactBoxActor)
		return nxFail("triangle-mesh contact actors failed");
	bool meshContactStep = true;
	for(unsigned step = 0; step != 4 && meshContactStep; ++step)
		{
		meshContactScene->simulate(0.01f);
		meshContactStep = meshContactScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		}
	const NxVec3 meshContactPosition = contactBoxActor->getGlobalPosition();
	const NxVec3 meshContactVelocity = contactBoxActor->getLinearVelocity();
	printf("simulation triangle-mesh box_contact fetched=%u position=%08x.%08x.%08x velocity=%08x.%08x.%08x\n",
		meshContactStep, nxFloatBits(meshContactPosition.x), nxFloatBits(meshContactPosition.y),
		nxFloatBits(meshContactPosition.z), nxFloatBits(meshContactVelocity.x),
		nxFloatBits(meshContactVelocity.y), nxFloatBits(meshContactVelocity.z));
	meshContactScene->releaseActor(*contactBoxActor);
	meshContactScene->releaseActor(*contactMeshActor);
	sdk->releaseTriangleMesh(*contactMesh);
	sdk->releaseScene(*meshContactScene);
	if(!meshContactStep)
		return nxFail("triangle-mesh box contact step failed");
	// 16-bit input indices are part of the public descriptor contract. The
	// triangle-mesh implementation must normalize them into the same public
	// indexed representation while preserving the caller's vertex ordering.
	const NxU16 meshTriangles16[] = { 0, 1, 2, 3, 4, 5 };
	NxTriangleMeshDesc mesh16Desc;
	mesh16Desc.numVertices = sizeof(meshPoints) / sizeof(meshPoints[0]);
	mesh16Desc.numTriangles = 2;
	mesh16Desc.pointStrideBytes = sizeof(NxPoint);
	mesh16Desc.triangleStrideBytes = 3 * sizeof(NxU16);
	mesh16Desc.points = meshPoints;
	mesh16Desc.triangles = meshTriangles16;
	mesh16Desc.flags = NX_MF_16_BIT_INDICES;
	NxTriangleMesh* const publicMesh16 = sdk->createTriangleMesh(mesh16Desc);
	printf("simulation triangle-mesh16 create=%u", publicMesh16 != 0);
	if(publicMesh16)
		{
		const NxU32* const indices16 = static_cast<const NxU32*>(publicMesh16->getBase(0, NX_ARRAY_TRIANGLES));
		printf(" vertices=%u triangles=%u index_format=%u index_stride=%u first_index=%u last_index=%u",
			publicMesh16->getCount(0, NX_ARRAY_VERTICES), publicMesh16->getCount(0, NX_ARRAY_TRIANGLES),
			publicMesh16->getFormat(0, NX_ARRAY_TRIANGLES), publicMesh16->getStride(0, NX_ARRAY_TRIANGLES),
			indices16 ? indices16[0] : 0, indices16 ? indices16[5] : 0);
		sdk->releaseTriangleMesh(*publicMesh16);
		}
	printf("\n");
	// Vehicle wheels discover the created capsule shape through the descriptor's
	// userData pointer, so this public-API case pins that descriptor-to-handle copy.
	static unsigned char capsuleUserDataMarker;
	NxSceneDesc capsuleUserDataSceneDesc;
	capsuleUserDataSceneDesc.setToDefault();
	NxScene* const capsuleUserDataScene = sdk->createScene(capsuleUserDataSceneDesc);
	if(!capsuleUserDataScene)
		return nxFail("capsule userData scene creation failed");
	NxCapsuleShapeDesc capsuleUserDataShape;
	capsuleUserDataShape.radius = 0.25f;
	capsuleUserDataShape.height = 0.5f;
	capsuleUserDataShape.userData = &capsuleUserDataMarker;
	NxActorDesc capsuleUserDataActorDesc;
	capsuleUserDataActorDesc.shapes.pushBack(&capsuleUserDataShape);
	NxActor* const capsuleUserDataActor = capsuleUserDataScene->createActor(capsuleUserDataActorDesc);
	NxShape** const capsuleUserDataShapes = capsuleUserDataActor
		? capsuleUserDataActor->getShapes() : 0;
	NxShape* const capsuleUserDataHandle = capsuleUserDataShapes ? capsuleUserDataShapes[0] : 0;
	const bool capsuleUserDataMatches = capsuleUserDataHandle &&
		capsuleUserDataHandle->userData == &capsuleUserDataMarker;
	printf("simulation capsule-shape userdata actor=%u shapes=%u marker=%u\n",
		capsuleUserDataActor != 0,
		capsuleUserDataActor ? capsuleUserDataActor->getNbShapes() : 0,
		capsuleUserDataMatches);
	shapeUserDataFailed = !capsuleUserDataActor || !capsuleUserDataMatches;
	if(capsuleUserDataActor)
		capsuleUserDataScene->releaseActor(*capsuleUserDataActor);
	sdk->releaseScene(*capsuleUserDataScene);
	JointDescSetGlobalAnchorFn setGlobalAnchor = reinterpret_cast<JointDescSetGlobalAnchorFn>(
		GetProcAddress(physics, "NxJointDesc_SetGlobalAnchor"));
	JointDescSetGlobalAxisFn setGlobalAxis = reinterpret_cast<JointDescSetGlobalAxisFn>(
		GetProcAddress(physics, "NxJointDesc_SetGlobalAxis"));
	if(!setGlobalAnchor || !setGlobalAxis)
		return nxFail("joint descriptor helpers are missing");
	for(unsigned selector = NX_BROADPHASE_QUADRATIC;
		selector <= NX_BROADPHASE_COHERENT; ++selector)
		{
		NxSceneDesc broadPhaseDesc;
		broadPhaseDesc.setToDefault();
		broadPhaseDesc.broadPhase = static_cast<NxBroadPhaseType>(selector);
		NxScene* broadPhaseScene = sdk->createScene(broadPhaseDesc);
		if(!broadPhaseScene)
			return nxFail("broadphase selector scene creation failed");
		if(selector == NX_BROADPHASE_QUADRATIC)
			{
			NxTriangleMesh* const sceneMesh = sdk->createTriangleMesh(meshDesc);
			printf("simulation triangle-mesh scene_owner create=%u\n", sceneMesh != 0);
			if(sceneMesh)
				{
				NxTriangleMeshShapeDesc meshShapeDesc;
				meshShapeDesc.meshData = sceneMesh;
				NxActorDesc meshActorDesc;
				meshActorDesc.shapes.pushBack(&meshShapeDesc);
				NxActor* const meshActor = broadPhaseScene->createActor(meshActorDesc);
				printf("simulation triangle-mesh actor=%u shapes=%u\n",
					meshActor != 0, meshActor ? meshActor->getNbShapes() : 0);
				NxShape** const actorShapes = meshActor ? meshActor->getShapes() : 0;
				NxShape* const actorShape = actorShapes ? actorShapes[0] : 0;
				NxTriangleMeshShape* const publicMeshShape =
					actorShape ? actorShape->isTriangleMesh() : 0;
				NxTriangleMesh* const recoveredMesh = publicMeshShape
					? &publicMeshShape->getTriangleMesh() : 0;
				NxBounds3 meshWorldBounds;
				if(actorShape)
					actorShape->getWorldBounds(meshWorldBounds);
				printf("simulation triangle-mesh handle type=%u mesh_same=%u\n",
					actorShape ? actorShape->getType() : NX_SHAPE_COUNT,
					recoveredMesh == sceneMesh);
				printf("simulation triangle-mesh world_bounds=%08x.%08x.%08x.%08x.%08x.%08x\n",
					nxFloatBits(meshWorldBounds.getMin().x), nxFloatBits(meshWorldBounds.getMin().y),
					nxFloatBits(meshWorldBounds.getMin().z), nxFloatBits(meshWorldBounds.getMax().x),
					nxFloatBits(meshWorldBounds.getMax().y), nxFloatBits(meshWorldBounds.getMax().z));
				if(meshActor)
					broadPhaseScene->releaseActor(*meshActor);
				sdk->releaseTriangleMesh(*sceneMesh);
				}
			NxSimulationHeldSceneWriteLock held = {
				*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(broadPhaseScene) + 0x0c),
				CreateEventA(0, TRUE, FALSE, 0), CreateEventA(0, TRUE, FALSE, 0) };
			if(!held.link || !held.ready || !held.release)
				return nxFail("triangle-mesh scene-lock fixture setup failed");
			HANDLE lockThread = CreateThread(0, 0, nxSimulationHoldSceneWriteLock, &held, 0, 0);
			if(!lockThread || WaitForSingleObject(held.ready, 5000) != WAIT_OBJECT_0)
				return nxFail("triangle-mesh scene-lock fixture did not acquire the lock");
			NxTriangleMesh* const lockedMesh = sdk->createTriangleMesh(meshDesc);
			printf("simulation triangle-mesh locked_create=%u\n", lockedMesh != 0);
			SetEvent(held.release);
			WaitForSingleObject(lockThread, INFINITE);
			CloseHandle(lockThread);
			CloseHandle(held.ready);
			CloseHandle(held.release);
			if(lockedMesh)
				sdk->releaseTriangleMesh(*lockedMesh);
			}
		printf("simulation broadphase selector=%u mode=%u\n",
			selector, nxReadSceneBroadPhaseMode(broadPhaseScene));
		nxPrintSimulationSceneState(broadPhaseScene, selector, "created");
		broadPhaseScene->setTiming(0.01f, 1, NX_TIMESTEP_VARIABLE);
		NxSphereShapeDesc separatedSphere;
		separatedSphere.radius = 0.5f;
		NxBodyDesc separatedBody0;
		NxActorDesc separatedActor0;
		separatedActor0.body = &separatedBody0;
		separatedActor0.density = 1.0f;
		separatedActor0.globalPose.t = NxVec3(-10.0f, 0.0f, 0.0f);
		separatedActor0.shapes.pushBack(&separatedSphere);
		NxBodyDesc separatedBody1;
		NxActorDesc separatedActor1;
		separatedActor1.body = &separatedBody1;
		separatedActor1.density = 1.0f;
		separatedActor1.globalPose.t = NxVec3(10.0f, 0.0f, 0.0f);
		separatedActor1.shapes.pushBack(&separatedSphere);
		if(!broadPhaseScene->createActor(separatedActor0)
			|| !broadPhaseScene->createActor(separatedActor1))
			return nxFail("separated-pair broadphase actors failed");
		broadPhaseScene->simulate(0.01f);
		if(!broadPhaseScene->checkResults(NX_RIGID_BODY_FINISHED, true)
			|| !broadPhaseScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
			return nxFail("separated-pair broadphase step failed");
		printf("simulation broadphase separated selector=%u pairs=%u\n",
			selector, nxReadSceneBroadPhasePairCount(broadPhaseScene));
		NxSphereShapeDesc staticSphere;
		staticSphere.radius = 1.0f;
		NxActorDesc staticActor0;
		staticActor0.globalPose.t = NxVec3(100.0f, 0.0f, 0.0f);
		staticActor0.shapes.pushBack(&staticSphere);
		NxActorDesc staticActor1;
		staticActor1.globalPose.t = NxVec3(100.0f, 0.0f, 0.0f);
		staticActor1.shapes.pushBack(&staticSphere);
		if(!broadPhaseScene->createActor(staticActor0)
			|| !broadPhaseScene->createActor(staticActor1))
			return nxFail("static-static broadphase actors failed");
		broadPhaseScene->simulate(0.01f);
		if(!broadPhaseScene->checkResults(NX_RIGID_BODY_FINISHED, true)
			|| !broadPhaseScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
			return nxFail("static-static broadphase step failed");
		printf("simulation broadphase static-static selector=%u pairs=%u\n",
			selector, nxReadSceneBroadPhasePairCount(broadPhaseScene));
		NxSphereShapeDesc overlappingSphere;
		overlappingSphere.radius = 0.5f;
		NxBodyDesc overlapBody0;
		NxActorDesc overlapActor0;
		overlapActor0.body = &overlapBody0;
		overlapActor0.density = 1.0f;
		overlapActor0.globalPose.t = NxVec3(200.0f, 0.0f, 0.0f);
		overlapActor0.shapes.pushBack(&overlappingSphere);
		NxBodyDesc overlapBody1;
		NxActorDesc overlapActor1;
		overlapActor1.body = &overlapBody1;
		overlapActor1.density = 1.0f;
		overlapActor1.globalPose.t = NxVec3(200.0f, 0.0f, 0.0f);
		overlapActor1.shapes.pushBack(&overlappingSphere);
		NxActorDesc overlapStatic;
		overlapStatic.globalPose.t = NxVec3(400.0f, 0.0f, 0.0f);
		overlapStatic.shapes.pushBack(&overlappingSphere);
		NxBodyDesc overlapDynamicBody;
		NxActorDesc overlapDynamic;
		overlapDynamic.body = &overlapDynamicBody;
		overlapDynamic.density = 1.0f;
		overlapDynamic.globalPose.t = NxVec3(400.0f, 0.0f, 0.0f);
		overlapDynamic.shapes.pushBack(&overlappingSphere);
		if(!broadPhaseScene->createActor(overlapActor0)
			|| !broadPhaseScene->createActor(overlapActor1)
			|| !broadPhaseScene->createActor(overlapStatic)
			|| !broadPhaseScene->createActor(overlapDynamic))
			return nxFail("overlapping broadphase actors failed");
		broadPhaseScene->simulate(0.01f);
		if(!broadPhaseScene->checkResults(NX_RIGID_BODY_FINISHED, true)
			|| !broadPhaseScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
			return nxFail("overlapping broadphase step failed");
		printf("simulation broadphase overlapping selector=%u pairs=%u\n",
			selector, nxReadSceneBroadPhasePairCount(broadPhaseScene));
		nxPrintSimulationSceneState(broadPhaseScene, selector, "active");
		sdk->releaseScene(*broadPhaseScene);
		NxSceneDesc chainDesc;
		chainDesc.setToDefault();
		chainDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
		chainDesc.broadPhase = static_cast<NxBroadPhaseType>(selector);
		NxScene* chainScene = sdk->createScene(chainDesc);
		if(!chainScene)
			return nxFail("overlapping broadphase chain scene creation failed");
		chainScene->setTiming(0.01f, 1, NX_TIMESTEP_VARIABLE);
		NxSphereShapeDesc chainSphere;
		chainSphere.radius = 0.5f;
		NxBodyDesc chainBody0;
		NxActorDesc chainActor0;
		chainActor0.body = &chainBody0;
		chainActor0.density = 1.0f;
		chainActor0.globalPose.t = NxVec3(6.0f, 0.0f, 0.0f);
		chainActor0.shapes.pushBack(&chainSphere);
		NxBodyDesc chainBody1;
		NxActorDesc chainActor1;
		chainActor1.body = &chainBody1;
		chainActor1.density = 1.0f;
		chainActor1.globalPose.t = NxVec3(6.75f, 0.0f, 0.0f);
		chainActor1.shapes.pushBack(&chainSphere);
		NxBodyDesc chainBody2;
		NxActorDesc chainActor2;
		chainActor2.body = &chainBody2;
		chainActor2.density = 1.0f;
		chainActor2.globalPose.t = NxVec3(7.5f, 0.0f, 0.0f);
		chainActor2.shapes.pushBack(&chainSphere);
		NxActor* chainActors[3] = {
			chainScene->createActor(chainActor0),
			chainScene->createActor(chainActor1),
			chainScene->createActor(chainActor2) };
		if(!chainActors[0] || !chainActors[1] || !chainActors[2])
			return nxFail("overlapping broadphase chain actors failed");
		chainScene->simulate(0.01f);
		if(!chainScene->checkResults(NX_RIGID_BODY_FINISHED, true)
			|| !chainScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
			return nxFail("overlapping broadphase chain step failed");
		if(selector == NX_BROADPHASE_COHERENT)
			{
			nxPrintActorBodyVelocity("broadphase2-contact1", *chainActors[1]);
			nxPrintActorBodyVelocity("broadphase2-contact2", *chainActors[2]);
			}
		for(unsigned chainIndex = 0; chainIndex != 3; ++chainIndex)
			{
			char stage[32];
			sprintf(stage, "broadphase%u-chain%u", selector, chainIndex);
			nxPrintActorState(stage, *chainActors[chainIndex]);
			}
		nxPrintBroadPhasePairOrder(chainScene, selector, chainActors, 3);
		if(selector == NX_BROADPHASE_COHERENT)
			{
			chainActors[0]->setGlobalPosition(NxVec3(100.0f, 0.0f, 0.0f));
			chainScene->simulate(0.01f);
			if(!chainScene->checkResults(NX_RIGID_BODY_FINISHED, true)
				|| !chainScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
				return nxFail("coherent broadphase separation step failed");
			nxPrintActorBodyVelocity("broadphase2-separated1", *chainActors[1]);
			nxPrintActorBodyVelocity("broadphase2-separated2", *chainActors[2]);
			printf("simulation broadphase moved-apart selector=%u pairs=%u\n",
				selector, nxReadSceneBroadPhasePairCount(chainScene));
			chainActors[0]->setGlobalPosition(NxVec3(6.0f, 0.0f, 0.0f));
			chainScene->simulate(0.01f);
			if(!chainScene->checkResults(NX_RIGID_BODY_FINISHED, true)
				|| !chainScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
				return nxFail("coherent broadphase rejoin step failed");
			nxPrintActorBodyVelocity("broadphase2-rejoin1", *chainActors[1]);
			nxPrintActorBodyVelocity("broadphase2-rejoin2", *chainActors[2]);
			printf("simulation broadphase moved-together selector=%u pairs=%u\n",
				selector, nxReadSceneBroadPhasePairCount(chainScene));
			}
		sdk->releaseScene(*chainScene);
		NxSceneDesc denseDesc;
		denseDesc.setToDefault();
		denseDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
		denseDesc.broadPhase = static_cast<NxBroadPhaseType>(selector);
		NxScene* denseScene = sdk->createScene(denseDesc);
		if(!denseScene)
			return nxFail("dense broadphase scene creation failed");
		denseScene->setTiming(0.01f, 1, NX_TIMESTEP_VARIABLE);
		NxSphereShapeDesc denseSphere;
		denseSphere.radius = 0.5f;
		NxBodyDesc denseBodies[4];
		NxActorDesc denseDescs[4];
		NxActor* denseActors[4] = { 0, 0, 0, 0 };
		for(unsigned denseIndex = 0; denseIndex != 4; ++denseIndex)
			{
			denseDescs[denseIndex].body = &denseBodies[denseIndex];
			denseDescs[denseIndex].density = 1.0f;
			denseDescs[denseIndex].globalPose.t = NxVec3(20.0f + denseIndex * 0.25f, 0.0f, 0.0f);
			denseDescs[denseIndex].shapes.pushBack(&denseSphere);
			denseActors[denseIndex] = denseScene->createActor(denseDescs[denseIndex]);
			if(!denseActors[denseIndex])
				return nxFail("dense broadphase actor creation failed");
			}
		denseScene->simulate(0.01f);
		if(!denseScene->checkResults(NX_RIGID_BODY_FINISHED, true)
			|| !denseScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
			return nxFail("dense broadphase step failed");
		printf("simulation broadphase dense selector=%u pairs=%u\n",
			selector, nxReadSceneBroadPhasePairCount(denseScene));
		nxPrintBroadPhasePairOrder(denseScene, selector, denseActors, 4);
		sdk->releaseScene(*denseScene);
		NxBounds3 boundedBounds;
		boundedBounds.set(-5.0f, -5.0f, -5.0f, 5.0f, 5.0f, 5.0f);
		NxSceneDesc boundedDesc;
		boundedDesc.setToDefault();
		boundedDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
		boundedDesc.broadPhase = static_cast<NxBroadPhaseType>(selector);
		boundedDesc.maxBounds = &boundedBounds;
		NxScene* boundedScene = sdk->createScene(boundedDesc);
		if(!boundedScene)
			return nxFail("bounded broadphase scene creation failed");
		boundedScene->setTiming(0.01f, 1, NX_TIMESTEP_VARIABLE);
		NxSphereShapeDesc boundedSphere;
		boundedSphere.radius = 0.5f;
		NxBodyDesc boundedBodies[4];
		NxActorDesc boundedDescs[4];
		const NxReal boundedX[4] = { 0.0f, 0.25f, 10.0f, 10.25f };
		for(unsigned boundedIndex = 0; boundedIndex != 4; ++boundedIndex)
			{
			boundedDescs[boundedIndex].body = &boundedBodies[boundedIndex];
			boundedDescs[boundedIndex].density = 1.0f;
			boundedDescs[boundedIndex].globalPose.t = NxVec3(boundedX[boundedIndex], 0.0f, 0.0f);
			boundedDescs[boundedIndex].shapes.pushBack(&boundedSphere);
			if(!boundedScene->createActor(boundedDescs[boundedIndex]))
				return nxFail("bounded broadphase actor creation failed");
			}
		boundedScene->simulate(0.01f);
		if(!boundedScene->checkResults(NX_RIGID_BODY_FINISHED, true)
			|| !boundedScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
			return nxFail("bounded broadphase step failed");
		printf("simulation broadphase bounded selector=%u pairs=%u\n",
			selector, nxReadSceneBroadPhasePairCount(boundedScene));
		sdk->releaseScene(*boundedScene);
		if(selector == NX_BROADPHASE_FULL)
			{
			NxSceneDesc planeDesc;
			planeDesc.setToDefault();
			planeDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
			planeDesc.broadPhase = static_cast<NxBroadPhaseType>(selector);
			NxScene* planeScene = sdk->createScene(planeDesc);
			if(!planeScene)
				return nxFail("full-pruner plane scene creation failed");
			planeScene->setTiming(0.01f, 1, NX_TIMESTEP_VARIABLE);
			NxPlaneShapeDesc planeShape;
			planeShape.normal = NxVec3(0.0f, 1.0f, 0.0f);
			NxActorDesc staticPlane;
			staticPlane.shapes.pushBack(&planeShape);
			NxSphereShapeDesc planeSphere;
			planeSphere.radius = 0.5f;
			NxBodyDesc planeBody;
			NxActorDesc dynamicSphere;
			dynamicSphere.body = &planeBody;
			dynamicSphere.density = 1.0f;
			dynamicSphere.globalPose.t = NxVec3(0.0f, 0.25f, 0.0f);
			dynamicSphere.shapes.pushBack(&planeSphere);
			if(!planeScene->createActor(staticPlane)
				|| !planeScene->createActor(dynamicSphere))
				return nxFail("full-pruner plane actors failed");
			planeScene->simulate(0.01f);
			if(!planeScene->checkResults(NX_RIGID_BODY_FINISHED, true)
				|| !planeScene->fetchResults(NX_RIGID_BODY_FINISHED, true))
				return nxFail("full-pruner plane step failed");
			printf("simulation broadphase plane selector=%u pairs=%u\n",
				selector, nxReadSceneBroadPhasePairCount(planeScene));
			sdk->releaseScene(*planeScene);
			}
		}
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene)
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("scene creation failed");
		}

	NxVec3 observedGravity;
	scene->getGravity(observedGravity);
	printf("simulation gravity=initial %08x.%08x.%08x\n",
		nxFloatBits(observedGravity.x), nxFloatBits(observedGravity.y), nxFloatBits(observedGravity.z));
	const NxVec3 changedGravity(0.0f, -20.0f, 0.0f);
	scene->setGravity(changedGravity);
	scene->getGravity(observedGravity);
	printf("simulation gravity=changed %08x.%08x.%08x\n",
		nxFloatBits(observedGravity.x), nxFloatBits(observedGravity.y), nxFloatBits(observedGravity.z));
	scene->setGravity(sceneDesc.gravity);

	NxReal maxTimestep = 0.0f;
	NxU32 maxIter = 0;
	NxTimeStepMethod method = NX_TIMESTEP_VARIABLE;
	scene->getTiming(maxTimestep, maxIter, method);
	printf("simulation timing=initial %08x.%u.%u\n",
		nxFloatBits(maxTimestep), maxIter, static_cast<unsigned>(method));
	scene->setTiming(0.125f, 4, NX_TIMESTEP_VARIABLE);
	scene->getTiming(maxTimestep, maxIter, method);
	printf("simulation timing=changed %08x.%u.%u\n",
		nxFloatBits(maxTimestep), maxIter, static_cast<unsigned>(method));
	scene->setTiming(sceneDesc.maxTimestep, sceneDesc.maxIter, sceneDesc.timeStepMethod);
	printf("simulation writable=initial %u\n", scene->isWritable() ? 1u : 0u);

	// Pin the worker/event completion path independently of body integration.
	NxScene* emptyScene = sdk->createScene(sceneDesc);
	if(!emptyScene)
		{
		sdk->releaseScene(*scene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("empty worker scene creation failed");
		}
	const unsigned simulateErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	if(!emptyScene->checkResults(static_cast<NxSimulationStatus>(0), false))
		return nxFail("checkResults returned pending for an unrequested status");
	emptyScene->simulate(-0.125f);
	printf("simulation negative-dt error=%u code=%u line=%d file=%s message=%s\n",
		simulationOutput.errors - simulateErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	emptyScene->simulate(0.0f);
	bool zeroDtReady = false;
	for(unsigned wait = 0; wait != 1000 && !zeroDtReady; ++wait)
		{
		zeroDtReady = emptyScene->checkResults(NX_RIGID_BODY_FINISHED, false);
		if(!zeroDtReady)
			::Sleep(1);
		}
	const bool zeroDtFetched = zeroDtReady
		? emptyScene->fetchResults(NX_RIGID_BODY_FINISHED, false) : false;
	printf("simulation zero-dt ready=%u fetched=%u\n",
		zeroDtReady ? 1u : 0u, zeroDtFetched ? 1u : 0u);
	unsigned char* emptyWrapper = reinterpret_cast<unsigned char*>(emptyScene);
	unsigned char* emptyInternal = *reinterpret_cast<unsigned char**>(emptyWrapper + 0x24);
	emptyScene->simulate(0.125f);
	emptyScene->simulate(0.25f);
	const unsigned queuedDtBits = nxFloatBits(
		*reinterpret_cast<NxReal*>(emptyInternal + 0x544));
	const unsigned char pendingFlag = emptyWrapper[0x20];
	bool submittedReady = false;
	for(unsigned wait = 0; wait != 1000 && !submittedReady; ++wait)
		{
		submittedReady = emptyScene->checkResults(NX_RIGID_BODY_FINISHED, false);
		if(!submittedReady)
			::Sleep(1);
		}
	const bool submittedFetched = submittedReady
		? emptyScene->fetchResults(NX_RIGID_BODY_FINISHED, false) : false;
	printf("simulation submit state dt=%08x pending=%u ready=%u fetched=%u finished=%u\n",
		queuedDtBits, static_cast<unsigned>(pendingFlag), submittedReady ? 1u : 0u,
		submittedFetched ? 1u : 0u, static_cast<unsigned>(emptyWrapper[0x20]));
	const bool idleReady = emptyScene->checkResults(NX_RIGID_BODY_FINISHED, false);
	const bool idleFetched = emptyScene->fetchResults(NX_RIGID_BODY_FINISHED, false);
	const bool idleFence = emptyScene->wait(NX_FENCE_RUN_FINISHED, false);
	printf("simulation nonblocking=idle ready=%u fetched=%u\n",
		idleReady ? 1u : 0u, idleFetched ? 1u : 0u);
	printf("simulation fence=idle ready=%u\n", idleFence ? 1u : 0u);
	emptyScene->simulate(0.125f);
	const bool submittedFence = emptyScene->wait(NX_FENCE_RUN_FINISHED, true);
	const bool emptyReady = emptyScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool emptyReadyNonblocking = emptyScene->checkResults(NX_RIGID_BODY_FINISHED, false);
	const bool emptyFetchedNonblocking = emptyScene->fetchResults(NX_RIGID_BODY_FINISHED, false);
	printf("simulation empty-step ready=%u fetched=%u\n",
		emptyReady ? 1u : 0u, emptyFetchedNonblocking ? 1u : 0u);
	printf("simulation fence=submitted ready=%u\n", submittedFence ? 1u : 0u);
	printf("simulation nonblocking=finished ready=%u fetched=%u\n",
		emptyReadyNonblocking ? 1u : 0u, emptyFetchedNonblocking ? 1u : 0u);
	sdk->releaseScene(*emptyScene);

	// Exercise conventional force accumulation through the real scene step,
	// without contact or gravity contributing to the velocity change.
	NxSceneDesc forceSceneDesc;
	forceSceneDesc.setToDefault();
	forceSceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* forceScene = sdk->createScene(forceSceneDesc);
	if(!forceScene)
		return nxFail("force scene creation failed");
	forceScene->setTiming(0.125f, 1, NX_TIMESTEP_VARIABLE);
	NxSphereShapeDesc forceSphere;
	forceSphere.radius = 0.5f;
	NxBodyDesc forceBody;
	NxActorDesc forceActorDesc;
	forceActorDesc.body = &forceBody;
	forceActorDesc.density = 1.0f;
	forceActorDesc.shapes.pushBack(&forceSphere);
	NxActor* forceActor = forceScene->createActor(forceActorDesc);
	if(!forceActor)
		return nxFail("force actor creation failed");
	forceActor->addForce(NxVec3(2.0f, 0.0f, 0.0f), NX_FORCE);
	forceScene->simulate(0.125f);
	const bool forceReady = forceScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool forceFetched = forceScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	if(!forceReady || !forceFetched)
		return nxFail("force simulation result was not ready and fetched");
	nxPrintActorState("force1", *forceActor);
	sdk->releaseScene(*forceScene);

	// Exercise the kinematic target branch of BodyStep through the public actor
	// move and scene simulate APIs.
	NxSceneDesc kinematicSceneDesc;
	kinematicSceneDesc.setToDefault();
	kinematicSceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* kinematicScene = sdk->createScene(kinematicSceneDesc);
	if(!kinematicScene)
		return nxFail("kinematic scene creation failed");
	kinematicScene->setTiming(0.125f, 1, NX_TIMESTEP_VARIABLE);
	NxBodyDesc kinematicBody;
	kinematicBody.flags |= NX_BF_KINEMATIC;
	NxActorDesc kinematicActorDesc;
	kinematicActorDesc.body = &kinematicBody;
	kinematicActorDesc.density = 1.0f;
	kinematicActorDesc.shapes.pushBack(&forceSphere);
	NxActor* kinematicActor = kinematicScene->createActor(kinematicActorDesc);
	if(!kinematicActor)
		return nxFail("kinematic actor creation failed");
	kinematicActor->moveGlobalPosition(NxVec3(1.0f, 0.0f, 0.5f));
	kinematicScene->simulate(0.125f);
	const bool kinematicReady = kinematicScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool kinematicFetched = kinematicScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	if(!kinematicReady || !kinematicFetched)
		return nxFail("kinematic simulation result was not ready and fetched");
	nxPrintActorState("kinematic1", *kinematicActor);
	printf("simulation kinematic flag=%u\n",
		kinematicActor->readBodyFlag(NX_BF_KINEMATIC) ? 1u : 0u);
	sdk->releaseScene(*kinematicScene);

	// Exercise automatic wake-counter expiry and the public wake transition on
	// an isolated, stationary body.
	NxSceneDesc sleepSceneDesc;
	sleepSceneDesc.setToDefault();
	sleepSceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* sleepScene = sdk->createScene(sleepSceneDesc);
	if(!sleepScene)
		return nxFail("sleep scene creation failed");
	sleepScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc sleepBody;
	NxActorDesc sleepActorDesc;
	sleepActorDesc.body = &sleepBody;
	sleepActorDesc.density = 1.0f;
	sleepActorDesc.shapes.pushBack(&forceSphere);
	NxActor* sleepActor = sleepScene->createActor(sleepActorDesc);
	if(!sleepActor)
		return nxFail("sleep actor creation failed");
	for(unsigned step = 0; step < 30; ++step)
		{
		sleepScene->simulate(0.02f);
		const bool ready = sleepScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = sleepScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("sleep simulation result was not ready and fetched");
		}
	printf("simulation sleep=automatic sleeping=%u\n", sleepActor->isSleeping() ? 1u : 0u);
	sleepActor->wakeUp(0.5f);
	printf("simulation sleep=woken sleeping=%u\n", sleepActor->isSleeping() ? 1u : 0u);
	sdk->releaseScene(*sleepScene);

	// Reach the reconstructed spring/damper effector through the real scene
	// step. The dynamic body starts beyond the relaxed length from the world
	// anchor; the oracle's pre-solver effector tick must pull it toward the anchor.
	NxSceneDesc effectorStepSceneDesc;
	effectorStepSceneDesc.setToDefault();
	effectorStepSceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* const effectorStepScene = sdk->createScene(effectorStepSceneDesc);
	if(!effectorStepScene)
		return nxFail("effector-step scene creation failed");
	effectorStepScene->setTiming(1.0f / 60.0f, 1, NX_TIMESTEP_FIXED);
	NxBoxShapeDesc effectorStepShape;
	effectorStepShape.setToDefault();
	effectorStepShape.dimensions = NxVec3(0.5f, 0.5f, 0.5f);
	NxBodyDesc effectorStepBody1;
	effectorStepBody1.setToDefault();
	NxActorDesc effectorStepActorDesc1;
	effectorStepActorDesc1.setToDefault();
	effectorStepActorDesc1.body = &effectorStepBody1;
	effectorStepActorDesc1.density = 2.0f;
	effectorStepActorDesc1.shapes.pushBack(&effectorStepShape);
	NxActor* const effectorStepActor1 = effectorStepScene->createActor(effectorStepActorDesc1);
	if(!effectorStepActor1)
		return nxFail("effector-step actor creation failed");
	NxSpringAndDamperEffectorDesc effectorStepDesc;
	effectorStepDesc.setToDefault();
	effectorStepDesc.body1 = effectorStepActor1;
	effectorStepDesc.pos1 = NxVec3(0.0f, 0.5f, 0.0f);
	effectorStepDesc.pos2 = NxVec3(2.0f, 0.0f, 0.0f);
	effectorStepDesc.springDistCompressSaturate = 0.5f;
	effectorStepDesc.springDistRelaxed = 1.0f;
	effectorStepDesc.springDistStretchSaturate = 4.0f;
	effectorStepDesc.springMaxCompressForce = 100.0f;
	effectorStepDesc.springMaxStretchForce = 100.0f;
	effectorStepDesc.damperVelCompressSaturate = -2.5f;
	effectorStepDesc.damperVelStretchSaturate = 1.75f;
	effectorStepDesc.damperMaxCompressForce = 7.0f;
	effectorStepDesc.damperMaxStretchForce = 9.5f;
	NxSpringAndDamperEffector* const stepEffector =
		effectorStepScene->createSpringAndDamperEffector(effectorStepDesc);
	if(!stepEffector)
		return nxFail("effector-step creation failed");
	NxReal effectorCompress, effectorRelaxed, effectorStretch, effectorMaxCompress, effectorMaxStretch;
	stepEffector->getLinearSpring(effectorCompress, effectorRelaxed, effectorStretch,
		effectorMaxCompress, effectorMaxStretch);
	NxReal damperCompress, damperStretch, damperMaxCompress, damperMaxStretch;
	stepEffector->getLinearDamper(damperCompress, damperStretch, damperMaxCompress, damperMaxStretch);
	const NxVec3 effectorInitialPosition1 = effectorStepActor1->getGlobalPosition();
	NxVec3 effectorStepCMass;
	effectorStepActor1->getCMassGlobalPosition(effectorStepCMass);
	NxVec3 effectorStepInertia;
	effectorStepActor1->getMassSpaceInertiaTensor(effectorStepInertia);
	printf("simulation effector-step setup count=%u awake=%u pos=%08x cmass=%08x.%08x.%08x inertia=%08x.%08x.%08x spring=%08x.%08x.%08x.%08x.%08x damper=%08x.%08x.%08x.%08x\n",
		effectorStepScene->getNbEffectors(), effectorStepActor1->isSleeping() ? 0u : 1u,
		nxFloatBits(effectorStepActor1->getGlobalPosition().x),
		nxFloatBits(effectorStepCMass.x), nxFloatBits(effectorStepCMass.y), nxFloatBits(effectorStepCMass.z),
		nxFloatBits(effectorStepInertia.x), nxFloatBits(effectorStepInertia.y), nxFloatBits(effectorStepInertia.z),
		nxFloatBits(effectorCompress),
		nxFloatBits(effectorRelaxed), nxFloatBits(effectorStretch),
		nxFloatBits(effectorMaxCompress), nxFloatBits(effectorMaxStretch),
		nxFloatBits(damperCompress), nxFloatBits(damperStretch),
		nxFloatBits(damperMaxCompress), nxFloatBits(damperMaxStretch));
	effectorStepActor1->wakeUp(1.0f);
	effectorStepScene->simulate(1.0f / 60.0f);
	const bool effectorStepReady = effectorStepScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool effectorStepFetched = effectorStepScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	if(!effectorStepReady || !effectorStepFetched)
		return nxFail("effector-step results were not ready and fetched");
	NxVec3 effectorStepVelocity1, effectorStepAngular1;
	NxQuat effectorStepOrientation1;
	effectorStepActor1->getLinearVelocity(effectorStepVelocity1);
	effectorStepActor1->getAngularVelocity(effectorStepAngular1);
	effectorStepActor1->getGlobalOrientationQuat(effectorStepOrientation1);
	printf("simulation effector-step ready=%u fetched=%u v=%08x.%08x.%08x w=%08x.%08x.%08x q=%08x.%08x.%08x.%08x\n",
		effectorStepReady ? 1u : 0u, effectorStepFetched ? 1u : 0u,
		nxFloatBits(effectorStepVelocity1.x), nxFloatBits(effectorStepVelocity1.y),
		nxFloatBits(effectorStepVelocity1.z), nxFloatBits(effectorStepAngular1.x),
		nxFloatBits(effectorStepAngular1.y), nxFloatBits(effectorStepAngular1.z),
		nxFloatBits(effectorStepOrientation1.x), nxFloatBits(effectorStepOrientation1.y),
		nxFloatBits(effectorStepOrientation1.z), nxFloatBits(effectorStepOrientation1.w));
	effectorStepScene->simulate(1.0f / 60.0f);
	const bool effectorStepReady2 = effectorStepScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool effectorStepFetched2 = effectorStepScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	if(!effectorStepReady2 || !effectorStepFetched2)
		return nxFail("second effector-step results were not ready and fetched");
	effectorStepActor1->getLinearVelocity(effectorStepVelocity1);
	effectorStepActor1->getAngularVelocity(effectorStepAngular1);
	effectorStepActor1->getGlobalOrientationQuat(effectorStepOrientation1);
	printf("simulation effector-step second ready=%u fetched=%u v=%08x.%08x.%08x w=%08x.%08x.%08x q=%08x.%08x.%08x.%08x\n",
		effectorStepReady2 ? 1u : 0u, effectorStepFetched2 ? 1u : 0u,
		nxFloatBits(effectorStepVelocity1.x), nxFloatBits(effectorStepVelocity1.y),
		nxFloatBits(effectorStepVelocity1.z), nxFloatBits(effectorStepAngular1.x),
		nxFloatBits(effectorStepAngular1.y), nxFloatBits(effectorStepAngular1.z),
		nxFloatBits(effectorStepOrientation1.x), nxFloatBits(effectorStepOrientation1.y),
		nxFloatBits(effectorStepOrientation1.z), nxFloatBits(effectorStepOrientation1.w));
	effectorStepScene->simulate(1.0f / 60.0f);
	const bool effectorStepReady3 = effectorStepScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool effectorStepFetched3 = effectorStepScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	if(!effectorStepReady3 || !effectorStepFetched3)
		return nxFail("third effector-step results were not ready and fetched");
	effectorStepActor1->getLinearVelocity(effectorStepVelocity1);
	effectorStepActor1->getAngularVelocity(effectorStepAngular1);
	effectorStepActor1->getGlobalOrientationQuat(effectorStepOrientation1);
	printf("simulation effector-step third ready=%u fetched=%u v=%08x.%08x.%08x w=%08x.%08x.%08x q=%08x.%08x.%08x.%08x\n",
		effectorStepReady3 ? 1u : 0u, effectorStepFetched3 ? 1u : 0u,
		nxFloatBits(effectorStepVelocity1.x), nxFloatBits(effectorStepVelocity1.y),
		nxFloatBits(effectorStepVelocity1.z), nxFloatBits(effectorStepAngular1.x),
		nxFloatBits(effectorStepAngular1.y), nxFloatBits(effectorStepAngular1.z),
		nxFloatBits(effectorStepOrientation1.x), nxFloatBits(effectorStepOrientation1.y),
		nxFloatBits(effectorStepOrientation1.z), nxFloatBits(effectorStepOrientation1.w));
	effectorStepScene->releaseEffector(*stepEffector);
	sdk->releaseScene(*effectorStepScene);

	// Pin each branch of phys_fn_003970 through the public simulation path.
	// A single step keeps the spring force observable without letting the
	// resulting velocity move a case into another distance regime.  The body
	// anchor is off-centre, matching the previously proven public effector setup
	// that produces an observable impulse during simulation.
	NxSceneDesc springBranchSceneDesc;
	springBranchSceneDesc.setToDefault();
	springBranchSceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* const springBranchScene = sdk->createScene(springBranchSceneDesc);
	if(!springBranchScene)
		return nxFail("spring-branch scene creation failed");
	springBranchScene->setTiming(1.0f / 60.0f, 1, NX_TIMESTEP_FIXED);
	const NxReal springBranchDistances[6] = { 2.0f, 4.0f, 1.0f, 0.75f,
		0.5f, 2.0f };
	const NxReal springBranchMaxForces[6] = { 100.0f, 100.0f, 100.0f,
		100.0f, 100.0f, 0.0f };
	NxActor* springBranchActors[6] = {};
	NxSpringAndDamperEffector* springBranchEffectors[6] = {};
	NxBoxShapeDesc springBranchShape;
	springBranchShape.setToDefault();
	springBranchShape.dimensions = NxVec3(0.25f, 0.25f, 0.25f);
	for(unsigned i = 0; i < 6; ++i)
		{
			NxBodyDesc body;
			body.setToDefault();
			NxActorDesc actorDesc;
			actorDesc.setToDefault();
			actorDesc.body = &body;
			actorDesc.density = 1.0f;
			const NxReal base = 10.0f * static_cast<NxReal>(i);
			actorDesc.globalPose.t = NxVec3(base, 0.0f, 0.0f);
			actorDesc.shapes.pushBack(&springBranchShape);
			springBranchActors[i] = springBranchScene->createActor(actorDesc);
			if(!springBranchActors[i])
				return nxFail("spring-branch actor creation failed");
			NxSpringAndDamperEffectorDesc desc;
			desc.setToDefault();
			desc.body1 = springBranchActors[i];
			desc.pos1 = NxVec3(base, 0.5f, 0.0f);
			desc.pos2 = NxVec3(base + springBranchDistances[i], 0.5f, 0.0f);
			desc.springDistCompressSaturate = 0.5f;
			desc.springDistRelaxed = 1.0f;
			desc.springDistStretchSaturate = 4.0f;
			desc.springMaxCompressForce = springBranchMaxForces[i];
			desc.springMaxStretchForce = springBranchMaxForces[i];
			springBranchEffectors[i] =
				springBranchScene->createSpringAndDamperEffector(desc);
			if(!springBranchEffectors[i])
				return nxFail("spring-branch effector creation failed");
			springBranchActors[i]->wakeUp(1.0f);
		}
	NxBodyDesc springPairBody1;
	springPairBody1.setToDefault();
	NxBodyDesc springPairBody2;
	springPairBody2.setToDefault();
	NxActorDesc springPairActorDesc1;
	springPairActorDesc1.setToDefault();
	springPairActorDesc1.body = &springPairBody1;
	springPairActorDesc1.density = 1.0f;
	springPairActorDesc1.globalPose.t = NxVec3(100.0f, 0.0f, 0.0f);
	springPairActorDesc1.shapes.pushBack(&springBranchShape);
	NxActorDesc springPairActorDesc2;
	springPairActorDesc2.setToDefault();
	springPairActorDesc2.body = &springPairBody2;
	springPairActorDesc2.density = 1.0f;
	springPairActorDesc2.globalPose.t = NxVec3(102.0f, 0.0f, 0.0f);
	springPairActorDesc2.shapes.pushBack(&springBranchShape);
	NxActor* const springPairActor1 = springBranchScene->createActor(springPairActorDesc1);
	NxActor* const springPairActor2 = springBranchScene->createActor(springPairActorDesc2);
	if(!springPairActor1 || !springPairActor2)
		return nxFail("two-body spring actor creation failed");
	NxSpringAndDamperEffectorDesc springPairDesc;
	springPairDesc.setToDefault();
	springPairDesc.body1 = springPairActor1;
	springPairDesc.body2 = springPairActor2;
	springPairDesc.pos1 = NxVec3(100.0f, 0.5f, 0.0f);
	springPairDesc.pos2 = NxVec3(102.0f, 0.5f, 0.0f);
	springPairDesc.springDistCompressSaturate = 0.5f;
	springPairDesc.springDistRelaxed = 1.0f;
	springPairDesc.springDistStretchSaturate = 4.0f;
	springPairDesc.springMaxCompressForce = 100.0f;
	springPairDesc.springMaxStretchForce = 100.0f;
	NxSpringAndDamperEffector* const springPairEffector =
		springBranchScene->createSpringAndDamperEffector(springPairDesc);
	if(!springPairEffector)
		return nxFail("two-body spring effector creation failed");
	springPairActor1->wakeUp(1.0f);
	springPairActor2->wakeUp(1.0f);
	springBranchScene->simulate(1.0f / 60.0f);
	const bool springBranchReady = springBranchScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool springBranchFetched = springBranchScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	if(!springBranchReady || !springBranchFetched)
		return nxFail("spring-branch simulation result was not ready and fetched");
	springBranchScene->simulate(1.0f / 60.0f);
	const bool springBranchReady2 = springBranchScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool springBranchFetched2 = springBranchScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	if(!springBranchReady2 || !springBranchFetched2)
		return nxFail("second spring-branch result was not ready and fetched");
	NxU32 springBranchVelocityBits[6][3] = {};
	for(unsigned i = 0; i < 6; ++i)
		{
			NxVec3 velocity;
			springBranchActors[i]->getLinearVelocity(velocity);
			springBranchVelocityBits[i][0] = nxFloatBits(velocity.x);
			springBranchVelocityBits[i][1] = nxFloatBits(velocity.y);
			springBranchVelocityBits[i][2] = nxFloatBits(velocity.z);
		}
	printf("simulation row3970 spring branches cases=6 ready=%u fetched=%u ready2=%u fetched2=%u velocity=",
		springBranchReady ? 1u : 0u, springBranchFetched ? 1u : 0u,
		springBranchReady2 ? 1u : 0u, springBranchFetched2 ? 1u : 0u);
	for(unsigned i = 0; i < 6; ++i)
		printf("%s%08x.%08x.%08x", i ? "/" : "", springBranchVelocityBits[i][0],
			springBranchVelocityBits[i][1], springBranchVelocityBits[i][2]);
	printf(" exact=%u\n", springBranchReady && springBranchFetched ? 1u : 0u);
	NxVec3 springPairVelocity1, springPairVelocity2;
	springPairActor1->getLinearVelocity(springPairVelocity1);
	springPairActor2->getLinearVelocity(springPairVelocity2);
	const bool springPairResponses = nxFloatBits(springPairVelocity1.x) != 0 &&
		nxFloatBits(springPairVelocity2.x) != 0 &&
		springPairVelocity1.x * springPairVelocity2.x < 0.0f;
	printf("simulation row3979 two-body pair=1 first=%08x.%08x.%08x second=%08x.%08x.%08x opposite=%u exact=%u\n",
		nxFloatBits(springPairVelocity1.x), nxFloatBits(springPairVelocity1.y),
		nxFloatBits(springPairVelocity1.z), nxFloatBits(springPairVelocity2.x),
		nxFloatBits(springPairVelocity2.y), nxFloatBits(springPairVelocity2.z),
		springPairResponses ? 1u : 0u,
		springPairVelocity1.x != springPairVelocity2.x ? 1u : 0u);
	for(unsigned i = 0; i < 6; ++i)
		springBranchScene->releaseEffector(*springBranchEffectors[i]);
	springBranchScene->releaseEffector(*springPairEffector);
	sdk->releaseScene(*springBranchScene);

	// Isolate quaternion integration from effectors and solver impulses: the
	// body starts with angular velocity and advances through the public step.
	NxSceneDesc angularStepSceneDesc;
	angularStepSceneDesc.setToDefault();
	angularStepSceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* const angularStepScene = sdk->createScene(angularStepSceneDesc);
	if(!angularStepScene)
		return nxFail("angular-step scene creation failed");
	angularStepScene->setTiming(1.0f / 60.0f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc angularStepBody;
	angularStepBody.setToDefault();
	angularStepBody.angularVelocity = NxVec3(0.0f, 0.0f, -0.85749924f);
	NxActorDesc angularStepActorDesc;
	angularStepActorDesc.setToDefault();
	angularStepActorDesc.body = &angularStepBody;
	angularStepActorDesc.density = 2.0f;
	angularStepActorDesc.shapes.pushBack(&effectorStepShape);
	NxActor* const angularStepActor = angularStepScene->createActor(angularStepActorDesc);
	if(!angularStepActor)
		return nxFail("angular-step actor creation failed");
	for(unsigned step = 0; step < 3; ++step)
		{
		angularStepScene->simulate(1.0f / 60.0f);
		const bool ready = angularStepScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = angularStepScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("angular-step results were not ready and fetched");
		NxQuat orientation;
		NxVec3 angularVelocity;
		angularStepActor->getGlobalOrientationQuat(orientation);
		angularStepActor->getAngularVelocity(angularVelocity);
		printf("simulation angular-step step=%u q=%08x.%08x.%08x.%08x w=%08x.%08x.%08x\n",
			step, nxFloatBits(orientation.x), nxFloatBits(orientation.y),
			nxFloatBits(orientation.z), nxFloatBits(orientation.w),
			nxFloatBits(angularVelocity.x), nxFloatBits(angularVelocity.y),
			nxFloatBits(angularVelocity.z));
		}
	sdk->releaseScene(*angularStepScene);

	// Exercise the public off-centre impulse path with a known pure torque.
	NxSceneDesc forceAtPosSceneDesc;
	forceAtPosSceneDesc.setToDefault();
	forceAtPosSceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* const forceAtPosScene = sdk->createScene(forceAtPosSceneDesc);
	if(!forceAtPosScene)
		return nxFail("force-at-position scene creation failed");
	forceAtPosScene->setTiming(1.0f / 60.0f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc forceAtPosBody;
	forceAtPosBody.setToDefault();
	NxActorDesc forceAtPosActorDesc;
	forceAtPosActorDesc.setToDefault();
	forceAtPosActorDesc.body = &forceAtPosBody;
	forceAtPosActorDesc.density = 2.0f;
	forceAtPosActorDesc.shapes.pushBack(&effectorStepShape);
	NxActor* const forceAtPosActor = forceAtPosScene->createActor(forceAtPosActorDesc);
	if(!forceAtPosActor)
		return nxFail("force-at-position actor creation failed");
	forceAtPosActor->addLocalForceAtPos(NxVec3(1.0f, -0.25f, 0.5f),
		NxVec3(0.25f, 0.5f, 0.125f), NX_IMPULSE);
	forceAtPosScene->simulate(1.0f / 60.0f);
	const bool forceAtPosReady = forceAtPosScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool forceAtPosFetched = forceAtPosScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	if(!forceAtPosReady || !forceAtPosFetched)
		return nxFail("force-at-position results were not ready and fetched");
	NxQuat forceAtPosOrientation;
	NxVec3 forceAtPosAngularVelocity;
	forceAtPosActor->getGlobalOrientationQuat(forceAtPosOrientation);
	forceAtPosActor->getAngularVelocity(forceAtPosAngularVelocity);
	printf("simulation force-at-position impulse ready=%u fetched=%u q=%08x.%08x.%08x.%08x w=%08x.%08x.%08x\n",
		forceAtPosReady ? 1u : 0u, forceAtPosFetched ? 1u : 0u,
		nxFloatBits(forceAtPosOrientation.x), nxFloatBits(forceAtPosOrientation.y),
		nxFloatBits(forceAtPosOrientation.z), nxFloatBits(forceAtPosOrientation.w),
		nxFloatBits(forceAtPosAngularVelocity.x), nxFloatBits(forceAtPosAngularVelocity.y),
		nxFloatBits(forceAtPosAngularVelocity.z));
	sdk->releaseScene(*forceAtPosScene);

	// Exercise a three-body contact island settling through the public solver
	// path, beyond the single impact pair.
	NxSceneDesc stackSceneDesc;
	stackSceneDesc.setToDefault();
	stackSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* stackScene = sdk->createScene(stackSceneDesc);
	if(!stackScene)
		return nxFail("stack scene creation failed");
	stackScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxPlaneShapeDesc stackPlane;
	NxActorDesc stackGroundDesc;
	stackGroundDesc.shapes.pushBack(&stackPlane);
	if(!stackScene->createActor(stackGroundDesc))
		return nxFail("stack ground creation failed");
	NxActor* stackActors[3] = {0};
	for(unsigned index = 0; index < 3; ++index)
		{
		NxBodyDesc stackBody;
		NxActorDesc stackActorDesc;
		stackActorDesc.body = &stackBody;
		stackActorDesc.density = 1.0f;
		stackActorDesc.globalPose.t = NxVec3(0.0f, 0.5f + float(index), 0.0f);
		stackActorDesc.shapes.pushBack(&forceSphere);
		stackActors[index] = stackScene->createActor(stackActorDesc);
		if(!stackActors[index])
			return nxFail("stack sphere creation failed");
		}
	for(unsigned step = 0; step < 1000; ++step)
		{
		stackScene->simulate(0.02f);
		const bool ready = stackScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = stackScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("stack simulation result was not ready and fetched");
		for(unsigned index = 0; index < 3; ++index)
			{
			char stage[16];
			sprintf_s(stage, "stack%u_%u", step, index);
			nxPrintActorState(stage, *stackActors[index]);
			}
		}
	printf("simulation stack=settled steps=1000 ready=1 fetched=1\n");
	for(unsigned index = 0; index < 3; ++index)
		{
		char stage[16];
		sprintf_s(stage, "stack%u", index);
		nxPrintActorState(stage, *stackActors[index]);
		}
	sdk->releaseScene(*stackScene);

	// Exercise fixed-joint angular and linear support rows through the island solver.
	NxSceneDesc fixedSceneDesc;
	fixedSceneDesc.setToDefault();
	fixedSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* fixedScene = sdk->createScene(fixedSceneDesc);
	if(!fixedScene)
		return nxFail("fixed-joint scene creation failed");
	fixedScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc fixedBody;
	NxActorDesc fixedActorDesc;
	fixedActorDesc.body = &fixedBody;
	fixedActorDesc.density = 1.0f;
	fixedActorDesc.globalPose.t = NxVec3(4.0f, 2.0f, 0.0f);
	fixedActorDesc.shapes.pushBack(&forceSphere);
	NxActor* fixedActor = fixedScene->createActor(fixedActorDesc);
	if(!fixedActor)
		return nxFail("fixed-joint actor creation failed");
	NxFixedJointDesc fixedDesc;
	fixedDesc.setToDefault();
	fixedDesc.actor[0] = fixedActor;
	fixedDesc.actor[1] = 0;
	setGlobalAnchor(fixedDesc, NxVec3(4.0f, 2.0f, 0.0f));
	setGlobalAxis(fixedDesc, NxVec3(0.0f, 0.0f, 1.0f));
	NxJoint* fixedJoint = fixedScene->createJoint(fixedDesc);
	if(!fixedJoint)
		return nxFail("fixed-joint creation failed");
	for(unsigned step = 0; step < 12; ++step)
		{
		fixedScene->simulate(0.02f);
		const bool ready = fixedScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = fixedScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("fixed-joint simulation result was not ready and fetched");
		char stage[24];
		sprintf_s(stage, "fixed%u", step);
		nxPrintActorState(stage, *fixedActor);
		}
	printf("simulation fixed-joint steps=12 ready=1 fetched=1\n");
	sdk->releaseScene(*fixedScene);

	// Keep the low-force fixed-joint break response in the registered
	// differential corpus; the historical four-step probe differed by six
	// output lines after the joint broke.
	NxSceneDesc breakSceneDesc;
	breakSceneDesc.setToDefault();
	breakSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* breakScene = sdk->createScene(breakSceneDesc);
	if(!breakScene)
		return nxFail("break-joint scene creation failed");
	breakScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc breakBody;
	NxActorDesc breakActorDesc;
	breakActorDesc.body = &breakBody;
	breakActorDesc.density = 1.0f;
	breakActorDesc.globalPose.t = NxVec3(6.0f, 2.0f, 0.0f);
	breakActorDesc.shapes.pushBack(&forceSphere);
	NxActor* breakActor = breakScene->createActor(breakActorDesc);
	if(!breakActor)
		return nxFail("break-joint actor creation failed");
	NxFixedJointDesc breakDesc;
	breakDesc.setToDefault();
	breakDesc.actor[0] = breakActor;
	breakDesc.actor[1] = 0;
	breakDesc.maxForce = 0.001f;
	setGlobalAnchor(breakDesc, NxVec3(6.0f, 2.0f, 0.0f));
	setGlobalAxis(breakDesc, NxVec3(0.0f, 0.0f, 1.0f));
	NxJoint* breakJoint = breakScene->createJoint(breakDesc);
	if(!breakJoint)
		return nxFail("break-joint creation failed");
	for(unsigned step = 0; step != 4; ++step)
		{
		breakScene->simulate(0.02f);
		const bool ready = breakScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = breakScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("break-joint simulation result was not ready and fetched");
		char stage[24];
		sprintf_s(stage, "break%u", step);
		nxPrintActorState(stage, *breakActor);
		printf("simulation break-state step=%u state=%u\n", step, (unsigned)breakJoint->getState());
		}
	printf("simulation break-joint steps=4 ready=1 fetched=1\n");
	sdk->releaseScene(*breakScene);

	// Exercise both NxUserNotify::onJointBreak return paths. Returning true
	// transfers release to fetchResults; returning false keeps the broken joint
	// detached from its bodies and queryable until the caller releases it.
	for(unsigned releaseJoint = 0; releaseJoint != 2; ++releaseJoint)
		{
		NxSimulationJointBreakNotify notify(releaseJoint != 0);
		NxSceneDesc notifySceneDesc;
		notifySceneDesc.setToDefault();
		notifySceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
		// Register through the public setter so the dedicated scene wrapper
		// setter/getter rows are exercised, rather than only copying the desc.
		notifySceneDesc.userNotify = 0;
		NxScene* notifyScene = sdk->createScene(notifySceneDesc);
		if(!notifyScene)
			return nxFail("joint-break notify scene creation failed");
		if(notifyScene->getUserNotify() != 0)
			return nxFail("joint-break notify getter did not preserve the null default");
		notifyScene->setUserNotify(&notify);
		const bool notifyGetter = notifyScene->getUserNotify() == &notify;
		if(!notifyGetter)
			return nxFail("joint-break notify setter/getter did not retain the callback");
		notifyScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
		NxBodyDesc notifyBody;
		NxActorDesc notifyActorDesc;
		notifyActorDesc.body = &notifyBody;
		notifyActorDesc.density = 1.0f;
		notifyActorDesc.globalPose.t = NxVec3(8.0f + 2.0f * releaseJoint, 2.0f, 0.0f);
		notifyActorDesc.shapes.pushBack(&forceSphere);
		NxActor* notifyActor = notifyScene->createActor(notifyActorDesc);
		if(!notifyActor)
			return nxFail("joint-break notify actor creation failed");
		NxFixedJointDesc notifyJointDesc;
		notifyJointDesc.setToDefault();
		notifyJointDesc.actor[0] = notifyActor;
		notifyJointDesc.actor[1] = 0;
		notifyJointDesc.maxForce = 0.001f;
		setGlobalAnchor(notifyJointDesc, notifyActorDesc.globalPose.t);
		setGlobalAxis(notifyJointDesc, NxVec3(0.0f, 0.0f, 1.0f));
		NxJoint* notifyJoint = notifyScene->createJoint(notifyJointDesc);
		if(!notifyJoint)
			return nxFail("joint-break notify joint creation failed");
		notify.expectedJoint = notifyJoint;
		for(unsigned step = 0; step != 4 && notify.calls == 0; ++step)
			{
			notifyScene->simulate(0.02f);
			const bool ready = notifyScene->checkResults(NX_RIGID_BODY_FINISHED, true);
			const bool fetched = notifyScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
			if(!ready || !fetched)
				return nxFail("joint-break notify result was not ready and fetched");
			char stage[32];
			sprintf_s(stage, "breaknotify%u_%u", releaseJoint, step);
			nxPrintActorState(stage, *notifyActor);
			}
		if(notify.calls != 1)
			return nxFail("joint-break notify callback count was not one");
		const unsigned retainedState = notify.releaseJoint ? 0
			: static_cast<unsigned>(notifyJoint->getState());
		NxActor* brokenActor0 = 0;
		NxActor* brokenActor1 = 0;
		unsigned detachedActors = 0;
		if(!notify.releaseJoint)
			{
			notifyJoint->getActors(&brokenActor0, &brokenActor1);
			detachedActors = !brokenActor0 && !brokenActor1;
			if(!detachedActors)
				return nxFail("retained broken joint actors were not detached");
			}
		printf("simulation break-notify result=%s calls=%u force=%08x joint_match=%u callback_state=%u retained_state=%u detached=%u joints=%u\n",
			notify.releaseJoint ? "release" : "retain", notify.calls, notify.forceBits,
			notify.jointMatches, notify.stateAtCallback, retainedState, detachedActors,
			static_cast<unsigned>(notifyScene->getNbJoints()));
		sdk->releaseScene(*notifyScene);
		}

	// Retain the revolute pendulum differential that exposed a small but
	// repeatable linear-velocity residual after the second solver step.
	NxSceneDesc revoluteSceneDesc;
	revoluteSceneDesc.setToDefault();
	revoluteSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* revoluteScene = sdk->createScene(revoluteSceneDesc);
	if(!revoluteScene)
		return nxFail("revolute-joint scene creation failed");
	revoluteScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc revoluteBody;
	NxActorDesc revoluteActorDesc;
	revoluteActorDesc.body = &revoluteBody;
	revoluteActorDesc.density = 1.0f;
	revoluteActorDesc.globalPose.t = NxVec3(7.0f, 2.0f, 0.0f);
	revoluteActorDesc.shapes.pushBack(&forceSphere);
	NxActor* revoluteActor = revoluteScene->createActor(revoluteActorDesc);
	if(!revoluteActor)
		return nxFail("revolute-joint actor creation failed");
	NxRevoluteJointDesc revoluteDesc;
	revoluteDesc.setToDefault();
	revoluteDesc.actor[0] = revoluteActor;
	revoluteDesc.actor[1] = 0;
	setGlobalAnchor(revoluteDesc, NxVec3(7.0f, 2.0f, 0.0f));
	setGlobalAxis(revoluteDesc, NxVec3(0.0f, 0.0f, 1.0f));
	NxJoint* revoluteJoint = revoluteScene->createJoint(revoluteDesc);
	if(!revoluteJoint)
		return nxFail("revolute-joint creation failed");
	for(unsigned step = 0; step < 12; ++step)
		{
		revoluteScene->simulate(0.02f);
		const bool ready = revoluteScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = revoluteScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("revolute-joint simulation result was not ready and fetched");
		char stage[24];
		sprintf_s(stage, "revolute%u", step);
		nxPrintActorState(stage, *revoluteActor);
		}
	printf("simulation revolute-joint steps=12 ready=1 fetched=1\n");
	sdk->releaseScene(*revoluteScene);


	// Regression coverage for kind-1 distance-row settling across repeated steps.
	NxSceneDesc distanceSceneDesc;
	distanceSceneDesc.setToDefault();
	distanceSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* distanceScene = sdk->createScene(distanceSceneDesc);
	if(!distanceScene) return nxFail("distance-joint scene creation failed");
	distanceScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc distanceBody;
	NxActorDesc distanceActorDesc;
	distanceActorDesc.body = &distanceBody;
	distanceActorDesc.density = 1.0f;
	distanceActorDesc.globalPose.t = NxVec3(4.0f, 2.0f, 0.0f);
	distanceActorDesc.shapes.pushBack(&forceSphere);
	NxActor* distanceActor = distanceScene->createActor(distanceActorDesc);
	if(!distanceActor) return nxFail("distance-joint actor creation failed");
	NxDistanceJointDesc distanceDesc;
	distanceDesc.setToDefault(false);
	distanceDesc.actor[0] = distanceActor;
	distanceDesc.actor[1] = 0;
	setGlobalAnchor(distanceDesc, NxVec3(4.0f, 2.0f, 0.0f));
	distanceDesc.minDistance = 0.0f;
	distanceDesc.maxDistance = 0.0f;
	distanceDesc.flags = NX_DJF_MIN_DISTANCE_ENABLED | NX_DJF_MAX_DISTANCE_ENABLED;
	NxJoint* distanceJoint = distanceScene->createJoint(distanceDesc);
	if(!distanceJoint) return nxFail("distance-joint creation failed");
	for(unsigned step = 0; step < 12; ++step)
		{
		distanceScene->simulate(0.02f);
		const bool ready = distanceScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = distanceScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched) return nxFail("distance-joint result was not ready and fetched");
		char stage[24];
		sprintf_s(stage, "distance%u", step);
		nxPrintActorState(stage, *distanceActor);
		}
	printf("simulation distance-joint steps=12 ready=1 fetched=1\n");
	sdk->releaseScene(*distanceScene);

	// Exercise the prismatic solver's transverse and angular rows over a
	// multistep public-scene simulation. The joint permits motion along X while
	// gravity loads the constrained Y direction.
	NxSceneDesc prismaticSceneDesc;
	prismaticSceneDesc.setToDefault();
	prismaticSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* prismaticScene = sdk->createScene(prismaticSceneDesc);
	if(!prismaticScene) return nxFail("prismatic-joint scene creation failed");
	prismaticScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxSphereShapeDesc prismaticSphere;
	prismaticSphere.radius = 0.1f;
	NxBodyDesc prismaticBody;
	NxActorDesc prismaticActorDesc;
	prismaticActorDesc.body = &prismaticBody;
	prismaticActorDesc.density = 1.0f;
	prismaticActorDesc.globalPose.t = NxVec3(12.0f, 4.0f, 0.0f);
	prismaticActorDesc.shapes.pushBack(&prismaticSphere);
	NxActor* prismaticActor = prismaticScene->createActor(prismaticActorDesc);
	if(!prismaticActor) return nxFail("prismatic-joint actor creation failed");
	NxPrismaticJointDesc prismaticDesc;
	prismaticDesc.setToDefault();
	prismaticDesc.actor[0] = prismaticActor;
	prismaticDesc.actor[1] = 0;
	setGlobalAnchor(prismaticDesc, NxVec3(12.0f, 4.0f, 0.0f));
	setGlobalAxis(prismaticDesc, NxVec3(1.0f, 0.0f, 0.0f));
	if(!prismaticScene->createJoint(prismaticDesc))
		return nxFail("prismatic-joint creation failed");
	for(unsigned step = 0; step < 8; ++step)
		{
		prismaticScene->simulate(0.02f);
		const bool ready = prismaticScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = prismaticScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("prismatic-joint simulation result was not ready and fetched");
		char stage[24];
		sprintf_s(stage, "prismatic%u", step);
		nxPrintActorState(stage, *prismaticActor);
		}
	printf("simulation prismatic-joint steps=8 ready=1 fetched=1\n");
	sdk->releaseScene(*prismaticScene);

	// Exercise the spherical point constraint with a nonzero anchor lever arm.
	// Gravity rotates the body around the world anchor while the joint keeps
	// the anchored point fixed.
	NxSceneDesc sphericalSceneDesc;
	sphericalSceneDesc.setToDefault();
	sphericalSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* sphericalScene = sdk->createScene(sphericalSceneDesc);
	if(!sphericalScene) return nxFail("spherical-joint scene creation failed");
	sphericalScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxSphereShapeDesc sphericalSphere;
	sphericalSphere.radius = 0.1f;
	NxBodyDesc sphericalBody;
	NxActorDesc sphericalActorDesc;
	sphericalActorDesc.body = &sphericalBody;
	sphericalActorDesc.density = 1.0f;
	sphericalActorDesc.globalPose.t = NxVec3(13.0f, 4.0f, 0.0f);
	sphericalActorDesc.shapes.pushBack(&sphericalSphere);
	NxActor* sphericalActor = sphericalScene->createActor(sphericalActorDesc);
	if(!sphericalActor) return nxFail("spherical-joint actor creation failed");
	NxSphericalJointDesc sphericalDesc;
	sphericalDesc.setToDefault();
	sphericalDesc.actor[0] = sphericalActor;
	sphericalDesc.actor[1] = 0;
	setGlobalAnchor(sphericalDesc, NxVec3(12.75f, 4.0f, 0.0f));
	if(!sphericalScene->createJoint(sphericalDesc))
		return nxFail("spherical-joint creation failed");
	for(unsigned step = 0; step < 8; ++step)
		{
		sphericalScene->simulate(0.02f);
		const bool ready = sphericalScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = sphericalScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("spherical-joint simulation result was not ready and fetched");
		char stage[24];
		sprintf_s(stage, "spherical%u", step);
		nxPrintActorState(stage, *sphericalActor);
		const NxQuat orientation = sphericalActor->getGlobalOrientationQuatVal();
		const NxVec3 angularVelocity = sphericalActor->getAngularVelocityVal();
		printf("simulation spherical-motion step=%u orientation=%08x.%08x.%08x.%08x angular=%08x.%08x.%08x\n",
			step, nxFloatBits(orientation.x), nxFloatBits(orientation.y),
			nxFloatBits(orientation.z), nxFloatBits(orientation.w),
			nxFloatBits(angularVelocity.x), nxFloatBits(angularVelocity.y),
			nxFloatBits(angularVelocity.z));
		}
	printf("simulation spherical-joint steps=8 ready=1 fetched=1\n");
	sdk->releaseScene(*sphericalScene);

	// Reproduce the D6 swing-limit angular-impulse cancellation case. The
	// actor begins 60 degrees off the world frame; one limited swing axis
	// drives a kind-2 row over four fixed solver steps.
	NxSceneDesc d6SwingSceneDesc;
	d6SwingSceneDesc.setToDefault();
	d6SwingSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* d6SwingScene = sdk->createScene(d6SwingSceneDesc);
	if(!d6SwingScene) return nxFail("D6 swing-limit scene creation failed");
	d6SwingScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxBodyDesc d6SwingBody;
	NxActorDesc d6SwingActorDesc;
	d6SwingActorDesc.body = &d6SwingBody;
	d6SwingActorDesc.density = 1.0f;
	d6SwingActorDesc.globalPose.t = NxVec3(8.0f, 2.0f, 0.0f);
	const NxReal d6SwingStartRotation[9] = {
		0.5f, 0.0f, 0.8660254f,
		0.0f, 1.0f, 0.0f,
		-0.8660254f, 0.0f, 0.5f };
	d6SwingActorDesc.globalPose.M.setRowMajor(d6SwingStartRotation);
	d6SwingActorDesc.shapes.pushBack(&forceSphere);
	NxActor* d6SwingActor = d6SwingScene->createActor(d6SwingActorDesc);
	if(!d6SwingActor) return nxFail("D6 swing-limit actor creation failed");
	NxD6JointDesc d6SwingDesc;
	d6SwingDesc.setToDefault();
	d6SwingDesc.actor[0] = d6SwingActor;
	d6SwingDesc.actor[1] = 0;
	d6SwingDesc.swing1Motion = NX_D6JOINT_MOTION_LIMITED;
	d6SwingDesc.swing1Limit.value = 0.2f;
	d6SwingDesc.projectionDistance = 0.0f;
	d6SwingDesc.projectionAngle = 0.0f;
	d6SwingDesc.projectionMode = NX_JPM_NONE;
	setGlobalAnchor(d6SwingDesc, NxVec3(8.0f, 2.0f, 0.0f));
	if(!d6SwingScene->createJoint(d6SwingDesc))
		return nxFail("D6 swing-limit joint creation failed");
	for(unsigned step = 0; step != 4; ++step)
		{
		d6SwingScene->simulate(0.02f);
		const bool ready = d6SwingScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = d6SwingScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("D6 swing-limit simulation result was not ready and fetched");
		}
	const NxQuat d6SwingOrientation = d6SwingActor->getGlobalOrientationQuatVal();
	const NxVec3 d6SwingAngularVelocity = d6SwingActor->getAngularVelocityVal();
	printf("simulation d6-swing-limit final orientation=%08x.%08x.%08x.%08x angular=%08x.%08x.%08x\n",
		nxFloatBits(d6SwingOrientation.x), nxFloatBits(d6SwingOrientation.y),
		nxFloatBits(d6SwingOrientation.z), nxFloatBits(d6SwingOrientation.w),
		nxFloatBits(d6SwingAngularVelocity.x), nxFloatBits(d6SwingAngularVelocity.y),
		nxFloatBits(d6SwingAngularVelocity.z));
	sdk->releaseScene(*d6SwingScene);

	// Exercise two different joint solver kinds in one connected island: a
	// fixed anchor on the first body and a minimum-distance row to the second.
	NxSceneDesc mixedJointSceneDesc;
	mixedJointSceneDesc.setToDefault();
	mixedJointSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* mixedJointScene = sdk->createScene(mixedJointSceneDesc);
	if(!mixedJointScene) return nxFail("mixed-joint scene creation failed");
	mixedJointScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxSphereShapeDesc mixedJointSphere;
	mixedJointSphere.radius = 0.1f;
	NxBodyDesc mixedJointBody0;
	NxActorDesc mixedJointActorDesc0;
	mixedJointActorDesc0.body = &mixedJointBody0;
	mixedJointActorDesc0.density = 1.0f;
	mixedJointActorDesc0.globalPose.t = NxVec3(16.0f, 6.0f, 0.0f);
	mixedJointActorDesc0.shapes.pushBack(&mixedJointSphere);
	NxActor* const mixedJointActor0 = mixedJointScene->createActor(mixedJointActorDesc0);
	NxBodyDesc mixedJointBody1;
	NxActorDesc mixedJointActorDesc1;
	mixedJointActorDesc1.body = &mixedJointBody1;
	mixedJointActorDesc1.density = 1.0f;
	mixedJointActorDesc1.globalPose.t = NxVec3(18.0f, 6.0f, 0.0f);
	mixedJointActorDesc1.shapes.pushBack(&mixedJointSphere);
	NxActor* const mixedJointActor1 = mixedJointScene->createActor(mixedJointActorDesc1);
	if(!mixedJointActor0 || !mixedJointActor1)
		return nxFail("mixed-joint actor creation failed");
	NxFixedJointDesc mixedFixedDesc;
	mixedFixedDesc.setToDefault();
	mixedFixedDesc.actor[0] = mixedJointActor0;
	mixedFixedDesc.actor[1] = 0;
	setGlobalAnchor(mixedFixedDesc, NxVec3(16.0f, 6.0f, 0.0f));
	setGlobalAxis(mixedFixedDesc, NxVec3(0.0f, 0.0f, 1.0f));
	if(!mixedJointScene->createJoint(mixedFixedDesc))
		return nxFail("mixed-joint fixed constraint creation failed");
	NxDistanceJointDesc mixedDistanceDesc;
	mixedDistanceDesc.setToDefault(false);
	mixedDistanceDesc.actor[0] = mixedJointActor1;
	mixedDistanceDesc.actor[1] = mixedJointActor0;
	setGlobalAnchor(mixedDistanceDesc, NxVec3(16.0f, 6.0f, 0.0f));
	mixedDistanceDesc.minDistance = 1.0f;
	mixedDistanceDesc.maxDistance = 1.0f;
	mixedDistanceDesc.flags = NX_DJF_MIN_DISTANCE_ENABLED | NX_DJF_MAX_DISTANCE_ENABLED;
	if(!mixedJointScene->createJoint(mixedDistanceDesc))
		return nxFail("mixed-joint distance constraint creation failed");
	for(unsigned step = 0; step != 12; ++step)
		{
		mixedJointScene->simulate(0.02f);
		const bool ready = mixedJointScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = mixedJointScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("mixed-joint simulation result was not ready and fetched");
		char stage0[24];
		char stage1[24];
		sprintf_s(stage0, "jointmix%u_0", step);
		sprintf_s(stage1, "jointmix%u_1", step);
		nxPrintActorState(stage0, *mixedJointActor0);
		nxPrintActorState(stage1, *mixedJointActor1);
		}
	printf("simulation solver-interaction fixed-distance steps=12 ready=1 fetched=1\n");
	sdk->releaseScene(*mixedJointScene);

	// A motorized two-joint chain exercises articulation-group construction:
	// both constraints connect dynamic bodies, and each motor drives motion
	// through the connected island.
	NxSceneDesc articulatedSceneDesc;
	articulatedSceneDesc.setToDefault();
	articulatedSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* articulatedScene = sdk->createScene(articulatedSceneDesc);
	if(!articulatedScene) return nxFail("articulated-chain scene creation failed");
	articulatedScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
	NxSphereShapeDesc articulatedSphere;
	articulatedSphere.radius = 0.25f;
	NxActor* articulatedActors[3] = {};
	for(unsigned actorIndex = 0; actorIndex != 3; ++actorIndex)
		{
		NxBodyDesc articulatedBody;
		NxActorDesc articulatedActorDesc;
		articulatedActorDesc.body = &articulatedBody;
		articulatedActorDesc.density = 1.0f;
		articulatedActorDesc.globalPose.t = NxVec3(24.0f + actorIndex * 2.0f, 8.0f, 0.0f);
		articulatedActorDesc.shapes.pushBack(&articulatedSphere);
		articulatedActors[actorIndex] = articulatedScene->createActor(articulatedActorDesc);
		if(!articulatedActors[actorIndex])
			return nxFail("articulated-chain actor creation failed");
		}
	NxRevoluteJoint* articulatedJoints[2] = {};
	for(unsigned jointIndex = 0; jointIndex != 2; ++jointIndex)
		{
		NxRevoluteJointDesc articulatedJointDesc;
		articulatedJointDesc.setToDefault();
		articulatedJointDesc.actor[0] = articulatedActors[jointIndex];
		articulatedJointDesc.actor[1] = articulatedActors[jointIndex + 1];
		// Put each hinge halfway between the connected body centers so the
		// chain starts as a valid constraint configuration.
		setGlobalAnchor(articulatedJointDesc, NxVec3(25.0f + jointIndex * 2.0f, 8.0f, 0.0f));
		setGlobalAxis(articulatedJointDesc, NxVec3(0.0f, 0.0f, 1.0f));
		articulatedJointDesc.motor.velTarget = jointIndex == 0 ? 2.0f : -1.5f;
		articulatedJointDesc.motor.maxForce = 6.0f;
		articulatedJointDesc.motor.freeSpin = false;
		articulatedJointDesc.flags = NX_RJF_MOTOR_ENABLED;
		articulatedJointDesc.projectionMode = NX_JPM_POINT_MINDIST;
		articulatedJointDesc.projectionDistance = 0.001f;
		articulatedJointDesc.projectionAngle = 0.05f;
		articulatedJoints[jointIndex] = static_cast<NxRevoluteJoint*>(
			articulatedScene->createJoint(articulatedJointDesc));
		if(!articulatedJoints[jointIndex])
			return nxFail("articulated-chain revolute joint creation failed");
		}
	for(unsigned step = 0; step != 16; ++step)
		{
		articulatedScene->simulate(0.02f);
		const bool ready = articulatedScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = articulatedScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("articulated-chain simulation result was not ready and fetched");
		for(unsigned actorIndex = 0; actorIndex != 3; ++actorIndex)
			{
			char stage[32];
			sprintf_s(stage, "artchain%u_actor%u", step, actorIndex);
			nxPrintActorState(stage, *articulatedActors[actorIndex]);
			const NxQuat orientation = articulatedActors[actorIndex]->getGlobalOrientationQuatVal();
			printf("simulation %s orientation=%08x.%08x.%08x.%08x\n", stage,
				nxFloatBits(orientation.x), nxFloatBits(orientation.y),
				nxFloatBits(orientation.z), nxFloatBits(orientation.w));
			}
		for(unsigned jointIndex = 0; jointIndex != 2; ++jointIndex)
			printf("simulation artchain%u_joint%u angle=%08x velocity=%08x\n", step,
				jointIndex, nxFloatBits(articulatedJoints[jointIndex]->getAngle()),
				nxFloatBits(articulatedJoints[jointIndex]->getVelocity()));
		}
	// Transition a member after the articulated island and its object have been
	// built. The body flag setter marks the root dirty; verify the public step's
	// rebuild ordering through the allocator balance as well as the trajectory.
	const unsigned islandLiveBeforeKinematic = simulationAllocator.liveAllocations;
	const unsigned islandMallocsBeforeKinematic = simulationAllocator.mallocCalls;
	const unsigned islandFreesBeforeKinematic = simulationAllocator.freeCalls;
	articulatedActors[1]->raiseBodyFlag(NX_BF_KINEMATIC);
	printf("simulation articulation-chain island-dirty transition=kinematic flag=%u\n",
		articulatedActors[1]->readBodyFlag(NX_BF_KINEMATIC) ? 1u : 0u);
	articulatedScene->simulate(0.02f);
	bool articulatedReady = articulatedScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	bool articulatedFetched = articulatedScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	if(!articulatedReady || !articulatedFetched)
		return nxFail("articulated-chain kinematic island rebuild was not ready and fetched");
	nxPrintActorState("artchain_dirty_kinematic", *articulatedActors[1]);
	printf("simulation articulation-chain island-dirty allocations transition=kinematic mallocs=%u frees=%u live_delta=%d\n",
		simulationAllocator.mallocCalls - islandMallocsBeforeKinematic,
		simulationAllocator.freeCalls - islandFreesBeforeKinematic,
		static_cast<int>(simulationAllocator.liveAllocations) - static_cast<int>(islandLiveBeforeKinematic));
	const unsigned islandLiveBeforeDynamic = simulationAllocator.liveAllocations;
	const unsigned islandMallocsBeforeDynamic = simulationAllocator.mallocCalls;
	const unsigned islandFreesBeforeDynamic = simulationAllocator.freeCalls;
	articulatedActors[1]->clearBodyFlag(NX_BF_KINEMATIC);
	printf("simulation articulation-chain island-dirty transition=dynamic flag=%u\n",
		articulatedActors[1]->readBodyFlag(NX_BF_KINEMATIC) ? 1u : 0u);
	articulatedScene->simulate(0.02f);
	articulatedReady = articulatedScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	articulatedFetched = articulatedScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	if(!articulatedReady || !articulatedFetched)
		return nxFail("articulated-chain dynamic island rebuild was not ready and fetched");
	nxPrintActorState("artchain_dirty_dynamic", *articulatedActors[1]);
	printf("simulation articulation-chain island-dirty allocations transition=dynamic mallocs=%u frees=%u live_delta=%d\n",
		simulationAllocator.mallocCalls - islandMallocsBeforeDynamic,
		simulationAllocator.freeCalls - islandFreesBeforeDynamic,
		static_cast<int>(simulationAllocator.liveAllocations) - static_cast<int>(islandLiveBeforeDynamic));
	printf("simulation articulation-chain actors=3 revolute_motors=2 steps=16 ready=1 fetched=1\n");
	sdk->releaseScene(*articulatedScene);

	NxPlaneShapeDesc groundPlane;
	groundPlane.normal = NxVec3(0.0f, 1.0f, 0.0f);
	groundPlane.d = 0.0f;
	NxActorDesc groundDesc;
	groundDesc.shapes.pushBack(&groundPlane);
	NxActor* ground = scene->createActor(groundDesc);
	if(!ground)
		return nxFail("static ground plane creation failed");
	printf("simulation ground=created\n");

	NxSphereShapeDesc sphere;
	sphere.radius = 0.5f;
	NxBodyDesc body;
	NxActorDesc actorDesc;
	actorDesc.body = &body;
	actorDesc.density = 1.0f;
	actorDesc.globalPose.t = NxVec3(0.0f, 10.0f, 0.0f);
	actorDesc.shapes.pushBack(&sphere);
	NxActor* actor = scene->createActor(actorDesc);
	if(!actor)
		{
		sdk->releaseScene(*scene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("dynamic sphere creation failed");
		}

	nxPrintActorState("initial", *actor);
	for(unsigned step = 0; step < 8; ++step)
		{
		const float dt = 0.125f;
		scene->simulate(dt);
		bool ready = scene->checkResults(NX_RIGID_BODY_FINISHED, true);
		bool fetched = scene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		printf("simulation step=%u ready=%u fetched=%u\n", step, ready ? 1u : 0u, fetched ? 1u : 0u);
		char stage[16];
		sprintf_s(stage, "step%u", step);
		nxPrintActorState(stage, *actor);
		}
	for(unsigned step = 8; step < 1000; ++step)
		{
		scene->simulate(0.125f);
		const bool ready = scene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = scene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("simulation soak result was not ready and fetched");
		}
	printf("simulation soak steps=1000 ready=1 fetched=1\n");
	nxPrintActorState("soak1000", *actor);

	// Exercise the kind-4 friction rows against explicit, nonzero material
	// coefficients while a sphere lands and slides on a static plane.
	NxMaterial frictionMaterial;
	frictionMaterial.staticFriction = 0.8f;
	frictionMaterial.dynamicFriction = 0.8f;
	const NxMaterialIndex frictionMaterialIndex = sdk->addMaterial(frictionMaterial);
	NxSceneDesc frictionSceneDesc;
	frictionSceneDesc.setToDefault();
	frictionSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	frictionSceneDesc.maxTimestep = 0.125f;
	frictionSceneDesc.maxIter = 1;
	NxScene* frictionScene = sdk->createScene(frictionSceneDesc);
	if(!frictionScene)
		return nxFail("friction scene creation failed");
	NxPlaneShapeDesc frictionPlane;
	frictionPlane.materialIndex = frictionMaterialIndex;
	NxActorDesc frictionGroundDesc;
	frictionGroundDesc.shapes.pushBack(&frictionPlane);
	if(!frictionScene->createActor(frictionGroundDesc))
		return nxFail("friction ground creation failed");
	NxSphereShapeDesc frictionSphere;
	frictionSphere.radius = 0.5f;
	frictionSphere.materialIndex = frictionMaterialIndex;
	NxBodyDesc sliderBody;
	NxActorDesc sliderDesc;
	sliderDesc.body = &sliderBody;
	sliderDesc.density = 1.0f;
	sliderDesc.globalPose.t = NxVec3(0.0f, 0.5f, 0.0f);
	sliderDesc.shapes.pushBack(&frictionSphere);
	NxActor* slider = frictionScene->createActor(sliderDesc);
	if(!slider)
		return nxFail("friction slider creation failed");
	slider->setLinearVelocity(NxVec3(2.0f, 0.0f, 0.0f));
	printf("simulation friction=created\n");
	for(unsigned step = 0; step < 8; ++step)
		{
		frictionScene->simulate(0.125f);
		const bool ready = frictionScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = frictionScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("friction simulation result was not ready and fetched");
		char stage[16];
		sprintf_s(stage, "friction%u", step);
		nxPrintActorState(stage, *slider);
		}
	sdk->releaseScene(*frictionScene);

	const unsigned flushStreamErrorsBefore = simulationOutput.errors;
	scene->flushStream();
	if(simulationOutput.errors != flushStreamErrorsBefore)
		return nxFail("flushStream unexpectedly emitted an error");
	printf("simulation flush-stream errors=%u\n",
		simulationOutput.errors - flushStreamErrorsBefore);

	NxSceneDesc legacySceneDesc;
	legacySceneDesc.setToDefault();
	legacySceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	legacySceneDesc.maxTimestep = 0.01f;
	legacySceneDesc.maxIter = 1;
	NxScene* const legacyScene = sdk->createScene(legacySceneDesc);
	if(!legacyScene)
		return nxFail("legacy scene creation failed");
	NxSphereShapeDesc legacyShape;
	legacyShape.radius = 0.5f;
	NxBodyDesc legacyBody;
	NxActorDesc legacyActorDesc;
	legacyActorDesc.body = &legacyBody;
	legacyActorDesc.density = 1.0f;
	legacyActorDesc.shapes.pushBack(&legacyShape);
	NxActor* const legacyActor = legacyScene->createActor(legacyActorDesc);
	if(!legacyActor)
		{
		sdk->releaseScene(*legacyScene);
		return nxFail("legacy actor creation failed");
		}
	legacyScene->setTiming(0.01f, 1, NX_TIMESTEP_VARIABLE);
	legacyActor->setLinearVelocity(NxVec3(1.0f, 0.0f, 0.0f));
	nxPrintActorState("legacy-initial", *legacyActor);
	legacyScene->startRun(0.01f);
	legacyScene->finishRun();
	nxPrintActorState("legacy-start-finish", *legacyActor);
	legacyScene->runFor(0.02f, 0.01f, 1, NX_TIMESTEP_VARIABLE);
	nxPrintActorState("legacy-runFor", *legacyActor);
	sdk->releaseScene(*legacyScene);

	scene->startRun(0.01f);
	scene->finishRun();
	printf("simulation legacy=start-finish returned\n");
	scene->runFor(0.01f, 0.1f, 1, NX_TIMESTEP_VARIABLE);
	printf("simulation legacy=runFor returned\n");

	// Exercise the contact solver with two dynamic bodies and nonzero tangential
	// velocity. This reaches both the coupled impulse rows and friction path,
	// beyond the single-body static-ground settling case above.
	NxSceneDesc pairSceneDesc;
	pairSceneDesc.setToDefault();
	pairSceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* pairScene = sdk->createScene(pairSceneDesc);
	if(!pairScene)
		return nxFail("dynamic-pair scene creation failed");
	pairScene->setTiming(0.025f, 8, NX_TIMESTEP_VARIABLE);
	NxSphereShapeDesc pairShape;
	pairShape.radius = 0.5f;
	NxBodyDesc movingBody;
	NxActorDesc movingDesc;
	movingDesc.body = &movingBody;
	movingDesc.density = 1.0f;
	movingDesc.globalPose.t = NxVec3(-2.0f, 0.0f, 0.0f);
	movingDesc.shapes.pushBack(&pairShape);
	NxActor* movingActor = pairScene->createActor(movingDesc);
	NxBodyDesc targetBody;
	NxActorDesc targetDesc;
	targetDesc.body = &targetBody;
	targetDesc.density = 1.0f;
	targetDesc.globalPose.t = NxVec3(0.0f, 0.0f, 0.0f);
	targetDesc.shapes.pushBack(&pairShape);
	NxActor* targetActor = pairScene->createActor(targetDesc);
	if(!movingActor || !targetActor)
		return nxFail("dynamic-pair actor creation failed");
	movingActor->setLinearVelocity(NxVec3(4.0f, 0.0f, 1.0f));
	printf("simulation pair=created\n");
	for(unsigned step = 0; step < 40; ++step)
		{
		pairScene->simulate(0.025f);
		const bool ready = pairScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = pairScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("dynamic-pair simulation result was not ready and fetched");
		if(step >= 8 && step <= 22)
			{
			char stage[16];
			NxActor& actor0 = *movingActor;
			NxActor& actor1 = *targetActor;
			NxVec3 velocity0;
			NxVec3 velocity1;
			actor0.getLinearVelocity(velocity0);
			actor1.getLinearVelocity(velocity1);
			const NxVec3& position0 = actor0.getGlobalPosition();
			const NxVec3& position1 = actor1.getGlobalPosition();
			sprintf_s(stage, "pair%u", step);
			printf("simulation stage=%s p0=%08x.%08x.%08x v0=%08x.%08x.%08x "
				"p1=%08x.%08x.%08x v1=%08x.%08x.%08x\n", stage,
				nxFloatBits(position0.x), nxFloatBits(position0.y), nxFloatBits(position0.z),
				nxFloatBits(velocity0.x), nxFloatBits(velocity0.y), nxFloatBits(velocity0.z),
				nxFloatBits(position1.x), nxFloatBits(position1.y), nxFloatBits(position1.z),
				nxFloatBits(velocity1.x), nxFloatBits(velocity1.y), nxFloatBits(velocity1.z));
			}
		}
	printf("simulation pair steps=40 ready=1 fetched=1\n");
	sdk->releaseScene(*pairScene);

	// Exercise contact-report generation through the public pair-flag API and
	// the real overlap/contact path (unlike the queue-injection dispatch fixture
	// below). The first frame must report the new touch at fetchResults.
	NxSceneDesc reportSceneDesc;
	reportSceneDesc.setToDefault();
	reportSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* reportScene = sdk->createScene(reportSceneDesc);
	if(!reportScene)
		return nxFail("contact-report scene creation failed");
	NxPlaneShapeDesc reportPlane;
	NxActorDesc reportGroundDesc;
	reportGroundDesc.shapes.pushBack(&reportPlane);
	NxActor* reportGround = reportScene->createActor(reportGroundDesc);
	NxSphereShapeDesc reportSphere;
	reportSphere.radius = 0.5f;
	NxBodyDesc reportBody;
	NxActorDesc reportDynamicDesc;
	reportDynamicDesc.body = &reportBody;
	reportDynamicDesc.density = 1.0f;
	reportDynamicDesc.globalPose.t = NxVec3(0.0f, 0.5f, 0.0f);
	reportDynamicDesc.shapes.pushBack(&reportSphere);
	NxActor* reportDynamic = reportScene->createActor(reportDynamicDesc);
	if(!reportGround || !reportDynamic)
		return nxFail("contact-report actors creation failed");
	NxSimulationContactReport generatedContactReport(reportGround, reportDynamic);
	reportScene->setUserContactReport(&generatedContactReport);
	reportGround->setGroup(7);
	reportDynamic->setGroup(3);
	sdk->setActorGroupPairFlags(7, 3, NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH);
	const NxU32 reportFlags = reportScene->getActorPairFlags(*reportGround, *reportDynamic);
	reportScene->simulate(0.125f);
	const bool reportReady = reportScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool reportFetched = reportScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	if(!reportReady || !reportFetched)
		return nxFail("contact-report simulation result was not ready and fetched");
	const NxU32 generatedPairCount = reportScene->getNbPairs();
	NxPairFlag generatedPairs[64];
	const bool generatedPairArray = reportScene->getPairFlagArray(generatedPairs, generatedPairCount);
	printf("simulation generated-contact summary flags=%08x calls=%u events=%08x ready=%u fetched=%u pairs=%u pair_array=%u",
		reportFlags, generatedContactReport.calls, generatedContactReport.events,
		reportReady, reportFetched, generatedPairCount, generatedPairArray);
	for(NxU32 pairIndex = 0; generatedPairArray && pairIndex < generatedPairCount && pairIndex < 64; ++pairIndex)
		printf(" pair%u_actor=%u_flags=%08x", pairIndex,
			generatedPairs[pairIndex].isActorPair(), generatedPairs[pairIndex].flags);
	printf("\n");
	reportScene->setActorPairFlags(*reportGround, *reportDynamic,
		NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH);
	const NxU32 notifyConfiguredPairCount = reportScene->getNbPairs();
	NxPairFlag notifyConfiguredPairs[64];
	const bool notifyConfiguredPairArray = reportScene->getPairFlagArray(notifyConfiguredPairs, notifyConfiguredPairCount);
	printf("simulation configured-pair-flags pairs=%u pair_array=%u",
		notifyConfiguredPairCount, notifyConfiguredPairArray);
	for(NxU32 pairIndex = 0; notifyConfiguredPairArray && pairIndex < notifyConfiguredPairCount && pairIndex < 64; ++pairIndex)
		printf(" pair%u_actor=%u_flags=%08x", pairIndex,
			notifyConfiguredPairs[pairIndex].isActorPair(), notifyConfiguredPairs[pairIndex].flags);
	printf("\n");
	reportScene->setActorPairFlags(*reportGround, *reportDynamic, NX_IGNORE_PAIR);
	const NxU32 configuredPairCount = reportScene->getNbPairs();
	NxPairFlag configuredPairs[64];
	const bool configuredPairArray = reportScene->getPairFlagArray(configuredPairs, configuredPairCount);
	printf("simulation configured-pair-flags count=%u array=%u", configuredPairCount, configuredPairArray);
	for(NxU32 pairIndex = 0; configuredPairArray && pairIndex < configuredPairCount && pairIndex < 64; ++pairIndex)
		printf(" pair%u_actor=%u_flags=%08x", pairIndex,
			configuredPairs[pairIndex].isActorPair(), configuredPairs[pairIndex].flags);
	printf("\n");
	reportScene->setActorPairFlags(*reportGround, *reportDynamic, NX_NOTIFY_ON_TOUCH);
	const NxU32 recordPairCount = reportScene->getNbPairs();
	NxPairFlag recordPairs[64];
	const bool recordPairArray = reportScene->getPairFlagArray(recordPairs, recordPairCount);
	printf("simulation record-pair-flags count=%u array=%u", recordPairCount, recordPairArray);
	for(NxU32 pairIndex = 0; recordPairArray && pairIndex < recordPairCount && pairIndex < 64; ++pairIndex)
		printf(" pair%u_actor=%u_flags=%08x", pairIndex,
			recordPairs[pairIndex].isActorPair(), recordPairs[pairIndex].flags);
	printf("\n");
	NxShape** const reportGroundShapes = reportGround->getShapes();
	NxShape** const reportDynamicShapes = reportDynamic->getShapes();
	if(!reportGroundShapes || !reportDynamicShapes || !reportGroundShapes[0] || !reportDynamicShapes[0])
		return nxFail("contact-report shapes unavailable for shape-pair flags");
	NxShape& reportGroundShape = *reportGroundShapes[0];
	NxShape& reportDynamicShape = *reportDynamicShapes[0];
	reportScene->setShapePairFlags(reportGroundShape, reportDynamicShape, NX_IGNORE_PAIR);
	const NxU32 configuredShapePairFlags = reportScene->getShapePairFlags(reportGroundShape, reportDynamicShape);
	const NxU32 configuredShapePairCount = reportScene->getNbPairs();
	NxPairFlag configuredShapePairs[64];
	const bool configuredShapePairArray = reportScene->getPairFlagArray(configuredShapePairs, configuredShapePairCount);
	printf("simulation configured-shape-pair-flags flags=%08x count=%u array=%u",
		configuredShapePairFlags, configuredShapePairCount, configuredShapePairArray);
	for(NxU32 pairIndex = 0; configuredShapePairArray && pairIndex < configuredShapePairCount && pairIndex < 64; ++pairIndex)
		printf(" pair%u_actor=%u_flags=%08x", pairIndex,
			configuredShapePairs[pairIndex].isActorPair(), configuredShapePairs[pairIndex].flags);
	printf("\n");
	reportScene->setShapePairFlags(reportGroundShape, reportDynamicShape, NX_NOTIFY_ON_TOUCH);
	const NxU32 recordShapePairFlags = reportScene->getShapePairFlags(reportGroundShape, reportDynamicShape);
	printf("simulation record-shape-pair-flags flags=%08x\n", recordShapePairFlags);
	reportScene->setShapePairFlags(reportGroundShape, reportDynamicShape, 0);
	printf("simulation cleared-shape-pair-flags flags=%08x\n",
		reportScene->getShapePairFlags(reportGroundShape, reportDynamicShape));
	sdk->setActorGroupPairFlags(7, 3, 0);
	sdk->releaseScene(*reportScene);

	// Exercise report timing for a dynamic compound that makes contact through
	// two independently pruned sphere children.
	NxSceneDesc compoundReportSceneDesc;
	compoundReportSceneDesc.setToDefault();
	compoundReportSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	NxScene* compoundReportScene = sdk->createScene(compoundReportSceneDesc);
	if(!compoundReportScene)
		return nxFail("compound contact-report scene creation failed");
	compoundReportScene->setTiming(1.0f / 60.0f, 8, NX_TIMESTEP_FIXED);
	NxActorDesc compoundGroundDesc;
	compoundGroundDesc.shapes.pushBack(&reportPlane);
	NxActor* compoundGround = compoundReportScene->createActor(compoundGroundDesc);
	NxSphereShapeDesc compoundSphere0;
	compoundSphere0.radius = 0.5f;
	compoundSphere0.localPose.t = NxVec3(-0.25f, 0.0f, 0.0f);
	NxSphereShapeDesc compoundSphere1;
	compoundSphere1.radius = 0.5f;
	compoundSphere1.localPose.t = NxVec3(0.25f, 0.0f, 0.0f);
	NxBodyDesc compoundReportBody;
	NxActorDesc compoundReportActorDesc;
	compoundReportActorDesc.body = &compoundReportBody;
	compoundReportActorDesc.density = 1.0f;
	// An elevated, rotated compound reaches an off-center contact and exercises
	// the child-pose refresh and angular solver state in the differential.
	compoundReportActorDesc.globalPose.t = NxVec3(0.0f, 0.7f, 0.0f);
	const NxReal compoundStartRotation[9] = {
		0.8660254f, -0.5f, 0.0f, 0.5f, 0.8660254f, 0.0f, 0.0f, 0.0f, 1.0f };
	compoundReportActorDesc.globalPose.M.setRowMajor(compoundStartRotation);
	compoundReportActorDesc.shapes.pushBack(&compoundSphere0);
	compoundReportActorDesc.shapes.pushBack(&compoundSphere1);
	NxActor* compoundReportActor = compoundReportScene->createActor(compoundReportActorDesc);
	if(!compoundGround || !compoundReportActor)
		return nxFail("compound contact-report actors creation failed");
	unsigned char* const compoundBody = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(compoundReportActor) + 0x14);
	unsigned char* const compoundRoot = *reinterpret_cast<unsigned char**>(compoundBody + 0x10);
	const NxVec3 actorStart = compoundReportActor->getGlobalPositionVal();
	printf("simulation compound-root-pose actor=%08x.%08x.%08x root=%08x.%08x.%08x\n",
		nxFloatBits(actorStart.x), nxFloatBits(actorStart.y), nxFloatBits(actorStart.z),
		nxFloatBits(*reinterpret_cast<float*>(compoundRoot + 0x30)),
		nxFloatBits(*reinterpret_cast<float*>(compoundRoot + 0x34)),
		nxFloatBits(*reinterpret_cast<float*>(compoundRoot + 0x38)));
	NxSimulationContactReport compoundReport(compoundGround, compoundReportActor);
	compoundReportScene->setUserContactReport(&compoundReport);
	compoundGround->setGroup(7);
	compoundReportActor->setGroup(3);
	sdk->setActorGroupPairFlags(7, 3, NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH);
	for(unsigned step = 0; step < 16; ++step)
		{
		compoundReportScene->simulate(1.0f / 60.0f);
		const bool ready = compoundReportScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = compoundReportScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("compound contact-report result was not ready and fetched");
		const NxVec3 compoundPosition = compoundReportActor->getGlobalPositionVal();
		const NxVec3 compoundVelocity = compoundReportActor->getLinearVelocityVal();
		const NxVec3 compoundAngularVelocity = compoundReportActor->getAngularVelocityVal();
		const NxQuat compoundOrientation = compoundReportActor->getGlobalOrientationQuatVal();
		printf("simulation compound-state step=%u position=%08x.%08x.%08x velocity=%08x.%08x.%08x angular=%08x.%08x.%08x orientation=%08x.%08x.%08x.%08x\n",
			step, nxFloatBits(compoundPosition.x), nxFloatBits(compoundPosition.y),
			nxFloatBits(compoundPosition.z), nxFloatBits(compoundVelocity.x),
			nxFloatBits(compoundVelocity.y), nxFloatBits(compoundVelocity.z),
			nxFloatBits(compoundAngularVelocity.x), nxFloatBits(compoundAngularVelocity.y),
			nxFloatBits(compoundAngularVelocity.z), nxFloatBits(compoundOrientation.x),
			nxFloatBits(compoundOrientation.y), nxFloatBits(compoundOrientation.z),
			nxFloatBits(compoundOrientation.w));
		printf("simulation compound-generated-contact step=%u calls=%u events=%08x\n",
			step, compoundReport.calls, compoundReport.events);
		}
	printf("simulation compound-generated-contact summary calls=%u events=%08x\n",
		compoundReport.calls, compoundReport.events);
	sdk->setActorGroupPairFlags(7, 3, 0);
	sdk->releaseScene(*compoundReportScene);

	// Seed the oracle-shaped fetch callback queues after a completed empty step.
	// This isolates the 000640 callback dispatch/list-reset contract from pair
	// generation, which is covered by the collision/contact reconstruction.
	NxSceneDesc callbackSceneDesc;
	callbackSceneDesc.setToDefault();
	NxScene* callbackScene = sdk->createScene(callbackSceneDesc);
	if(!callbackScene)
		return nxFail("fetch-callback scene creation failed");
	unsigned char callbackWrapper[0x28];
	memcpy(callbackWrapper, callbackScene, sizeof(callbackWrapper));
	unsigned char* const callbackInternal = *reinterpret_cast<unsigned char**>(callbackWrapper + 0x24);
	unsigned char internalShapes[2][0xa0];
	unsigned char publicShapeTokens[2];
	memset(internalShapes, 0, sizeof(internalShapes));
	memset(publicShapeTokens, 0, sizeof(publicShapeTokens));
	NxShape* const expectedTrigger = reinterpret_cast<NxShape*>(&publicShapeTokens[0]);
	NxShape* const expectedOther = reinterpret_cast<NxShape*>(&publicShapeTokens[1]);
	*reinterpret_cast<NxShape**>(internalShapes[0] + 0x9c) = expectedTrigger;
	*reinterpret_cast<NxShape**>(internalShapes[1] + 0x9c) = expectedOther;
	NxSimulationTriggerReport triggerReport(expectedTrigger, expectedOther);
	NxSimulationContactReport contactReport(reinterpret_cast<NxActor*>(&publicShapeTokens[0]),
		reinterpret_cast<NxActor*>(&publicShapeTokens[1]));
	callbackScene->setUserTriggerReport(&triggerReport);
	callbackScene->setUserContactReport(&contactReport);
	const bool triggerGetter = callbackScene->getUserTriggerReport() == &triggerReport;
	const bool contactGetter = callbackScene->getUserContactReport() == &contactReport;
	unsigned char triggerQueue[0x0c];
	*reinterpret_cast<void**>(triggerQueue + 0x00) = internalShapes[0];
	*reinterpret_cast<void**>(triggerQueue + 0x04) = internalShapes[1];
	*reinterpret_cast<NxU32*>(triggerQueue + 0x08) = NX_TRIGGER_ON_ENTER;
	unsigned char contactQueue[0x2c];
	memset(contactQueue, 0, sizeof(contactQueue));
	NxContactPair* const contactPair = reinterpret_cast<NxContactPair*>(contactQueue);
	contactPair->actors[0] = contactReport.expectedActor0;
	contactPair->actors[1] = contactReport.expectedActor1;
	contactPair->sumNormalForce = NxVec3(1.0f, 2.0f, 3.0f);
	*reinterpret_cast<NxU32*>(contactQueue + 0x28) = NX_NOTIFY_ON_TOUCH;
	void** const triggerBeginField = reinterpret_cast<void**>(callbackInternal + 0x5fc);
	void** const triggerEndField = reinterpret_cast<void**>(callbackInternal + 0x600);
	void** const triggerCapacityField = reinterpret_cast<void**>(callbackInternal + 0x604);
	void** const contactBeginField = reinterpret_cast<void**>(callbackInternal + 0x60c);
	void** const contactEndField = reinterpret_cast<void**>(callbackInternal + 0x610);
	void** const contactCapacityField = reinterpret_cast<void**>(callbackInternal + 0x614);
	void* const savedTriggerBegin = *triggerBeginField;
	void* const savedTriggerEnd = *triggerEndField;
	void* const savedTriggerCapacity = *triggerCapacityField;
	void* const savedContactBegin = *contactBeginField;
	void* const savedContactEnd = *contactEndField;
	void* const savedContactCapacity = *contactCapacityField;
	*triggerBeginField = triggerQueue;
	*triggerEndField = triggerQueue + sizeof(triggerQueue);
	*triggerCapacityField = triggerQueue + sizeof(triggerQueue);
	*contactBeginField = contactQueue;
	*contactEndField = contactQueue + sizeof(contactQueue);
	*contactCapacityField = contactQueue + sizeof(contactQueue);
	callbackScene->simulate(0.01f);
	const bool callbackReady = callbackScene->checkResults(NX_RIGID_BODY_FINISHED, true);
	const bool callbackFetched = callbackScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
	const bool triggerQueueCleared = *triggerEndField == *triggerBeginField;
	const bool contactQueueCleared = *contactEndField == *contactBeginField;
	*triggerBeginField = savedTriggerBegin;
	*triggerEndField = savedTriggerEnd;
	*triggerCapacityField = savedTriggerCapacity;
	*contactBeginField = savedContactBegin;
	*contactEndField = savedContactEnd;
	*contactCapacityField = savedContactCapacity;
	if(!callbackReady || !callbackFetched)
		return nxFail("fetch-callback simulation result was not ready and fetched");
	const bool callbackDispatchExact = triggerGetter && triggerReport.calls == 1 &&
		triggerReport.lastMatches && triggerReport.status == NX_TRIGGER_ON_ENTER &&
		triggerQueueCleared &&
		contactGetter && contactReport.calls == 1 &&
		contactReport.events == NX_NOTIFY_ON_TOUCH && contactQueueCleared;
	printf("simulation fetch-callback summary ready=%u fetched=%u trigger_get=%u trigger_calls=%u trigger_cleared=%u contact_get=%u contact_calls=%u contact_cleared=%u exact=%u\n",
		callbackReady, callbackFetched, triggerGetter, triggerReport.calls,
		triggerQueueCleared, contactGetter, contactReport.calls,
		contactQueueCleared, callbackDispatchExact);
	sdk->releaseScene(*callbackScene);
	if(!callbackDispatchExact)
		return nxFail("fetch-callback dispatch or queue reset did not match the oracle contract");

	// fetchResults must retain buffered contact reports while no user callback
	// is installed. Installing the listener before the next fetch must deliver
	// the pending record exactly once. The oracle's callback dispatcher only
	// resets the queue when it has a listener; a second unconditional drain would
	// silently discard this record.
	NxSceneDesc deferredContactSceneDesc;
	deferredContactSceneDesc.setToDefault();
	NxScene* deferredContactScene = sdk->createScene(deferredContactSceneDesc);
	if(!deferredContactScene)
		return nxFail("deferred contact-report scene creation failed");
	unsigned char deferredContactWrapper[0x28];
	memcpy(deferredContactWrapper, deferredContactScene, sizeof(deferredContactWrapper));
	unsigned char* const deferredContactInternal = *reinterpret_cast<unsigned char**>(
		deferredContactWrapper + 0x24);
	unsigned char deferredContactQueue[0x2c];
	memset(deferredContactQueue, 0, sizeof(deferredContactQueue));
	NxActor* const deferredExpectedActor0 = reinterpret_cast<NxActor*>(&publicShapeTokens[0]);
	NxActor* const deferredExpectedActor1 = reinterpret_cast<NxActor*>(&publicShapeTokens[1]);
	*reinterpret_cast<NxActor**>(deferredContactQueue + 0x00) = deferredExpectedActor0;
	*reinterpret_cast<NxActor**>(deferredContactQueue + 0x04) = deferredExpectedActor1;
	*reinterpret_cast<NxU32*>(deferredContactQueue + 0x0c) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(deferredContactQueue + 0x10) = nxFloatBits(2.0f);
	*reinterpret_cast<NxU32*>(deferredContactQueue + 0x14) = nxFloatBits(3.0f);
	*reinterpret_cast<NxU32*>(deferredContactQueue + 0x28) = NX_NOTIFY_ON_TOUCH;
	void** const deferredContactBeginField = reinterpret_cast<void**>(
		deferredContactInternal + 0x60c);
	void** const deferredContactEndField = reinterpret_cast<void**>(
		deferredContactInternal + 0x610);
	void** const deferredContactCapacityField = reinterpret_cast<void**>(
		deferredContactInternal + 0x614);
	void* const deferredSavedBegin = *deferredContactBeginField;
	void* const deferredSavedEnd = *deferredContactEndField;
	void* const deferredSavedCapacity = *deferredContactCapacityField;
	*deferredContactBeginField = deferredContactQueue;
	*deferredContactEndField = deferredContactQueue + sizeof(deferredContactQueue);
	*deferredContactCapacityField = deferredContactQueue + sizeof(deferredContactQueue);
	const bool deferredFirstFetch = deferredContactScene->fetchResults(
		static_cast<NxSimulationStatus>(0), false);
	const bool deferredPendingAfterFirstFetch =
		*deferredContactEndField == deferredContactQueue + sizeof(deferredContactQueue);
	NxSimulationContactReport deferredContactReport(
		deferredExpectedActor0, deferredExpectedActor1);
	deferredContactScene->setUserContactReport(&deferredContactReport);
	const bool deferredSecondFetch = deferredContactScene->fetchResults(
		static_cast<NxSimulationStatus>(0), false);
	printf("simulation deferred-contact-report first=%u pending=%u second=%u calls=%u events=%08x\n",
		deferredFirstFetch, deferredPendingAfterFirstFetch, deferredSecondFetch,
		deferredContactReport.calls, deferredContactReport.events);
	*deferredContactBeginField = deferredSavedBegin;
	*deferredContactEndField = deferredSavedEnd;
	*deferredContactCapacityField = deferredSavedCapacity;
	deferredContactFailed = !deferredFirstFetch || !deferredPendingAfterFirstFetch || !deferredSecondFetch
		|| deferredContactReport.calls != 1
		|| deferredContactReport.events != NX_NOTIFY_ON_TOUCH;
	if(deferredContactFailed)
		printf("FAIL deferred contact report was not retained and delivered exactly once\n");
	sdk->releaseScene(*deferredContactScene);

	sdk->releaseScene(*scene);

	{
	// Exercise contact generation, reporting and response through the same
	// public scene step/result path. Keep this in its own scene so the 1,000-step
	// gravity soak above cannot affect the collision fixture.
	NxSimulationContactStreamReport contactReport;
	NxSceneDesc contactSceneDesc;
	contactSceneDesc.setToDefault();
	contactSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	contactSceneDesc.timeStepMethod = NX_TIMESTEP_VARIABLE;
	contactSceneDesc.userContactReport = &contactReport;
	NxScene* contactScene = sdk->createScene(contactSceneDesc);
	if(!contactScene)
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("contact scene creation failed");
		}

	NxPlaneShapeDesc ground;
	NxActorDesc groundDesc;
	groundDesc.shapes.pushBack(&ground);
	NxActor* groundActor = contactScene->createActor(groundDesc);
	NxSphereShapeDesc fallingSphere;
	fallingSphere.radius = 0.5f;
	NxBodyDesc fallingBody;
	fallingBody.mass = 1.0f;
	fallingBody.massSpaceInertia = NxVec3(0.1f, 0.1f, 0.1f);
	NxActorDesc fallingDesc;
	fallingDesc.body = &fallingBody;
	fallingDesc.shapes.pushBack(&fallingSphere);
	fallingDesc.globalPose.t = NxVec3(0.0f, 1.0f, 0.0f);
	NxActor* fallingActor = contactScene->createActor(fallingDesc);
	if(!groundActor || !fallingActor)
		{
		sdk->releaseScene(*contactScene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("contact actors creation failed");
		}
	contactScene->setActorPairFlags(*groundActor, *fallingActor,
			NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH);
	for(unsigned step = 0; step < 60; ++step)
		{
		contactScene->simulate(1.0f / 60.0f);
		const bool ready = contactScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = contactScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			{
			sdk->releaseScene(*contactScene);
			sdk->release();
			FreeLibrary(physics);
			return nxFail("contact scene result was not ready and fetched");
			}
		char contactStage[16];
		sprintf_s(contactStage, "contact%u", step);
		nxPrintActorState(contactStage, *fallingActor);
		}
	printf("simulation contact callbacks=%u events=%08x pairs=%u patches=%u points=%u normal=%08x.%08x.%08x point=%08x.%08x.%08x separation=%08x\n",
		contactReport.calls, contactReport.events, contactReport.pairs,
		contactReport.patches, contactReport.points,
		contactReport.normal[0], contactReport.normal[1], contactReport.normal[2],
		contactReport.point[0], contactReport.point[1], contactReport.point[2],
		contactReport.separation);
	nxPrintActorState("contact60", *fallingActor);
	sdk->releaseScene(*contactScene);

	// Stop immediately after the first real contact report so scene release
	// observes a live touching pair and must emit its final end-touch callback
	// from phys_fn_000913 before destroying either actor wrapper.
	NxSimulationContactStreamReport sceneReleaseReport;
	NxSceneDesc sceneReleaseDesc;
	sceneReleaseDesc.setToDefault();
	sceneReleaseDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	sceneReleaseDesc.userContactReport = &sceneReleaseReport;
	NxScene* sceneReleaseScene = sdk->createScene(sceneReleaseDesc);
	if(!sceneReleaseScene)
		return nxFail("scene-release contact scene creation failed");
	NxPlaneShapeDesc sceneReleaseGround;
	NxActorDesc sceneReleaseGroundDesc;
	sceneReleaseGroundDesc.shapes.pushBack(&sceneReleaseGround);
	NxActor* sceneReleaseGroundActor = sceneReleaseScene->createActor(sceneReleaseGroundDesc);
	NxSphereShapeDesc sceneReleaseSphere;
	sceneReleaseSphere.radius = 0.5f;
	NxBodyDesc sceneReleaseBody;
	sceneReleaseBody.mass = 1.0f;
	sceneReleaseBody.massSpaceInertia = NxVec3(0.1f, 0.1f, 0.1f);
	NxActorDesc sceneReleaseSphereDesc;
	sceneReleaseSphereDesc.body = &sceneReleaseBody;
	sceneReleaseSphereDesc.shapes.pushBack(&sceneReleaseSphere);
	sceneReleaseSphereDesc.globalPose.t = NxVec3(0.0f, 1.0f, 0.0f);
	NxActor* sceneReleaseSphereActor = sceneReleaseScene->createActor(sceneReleaseSphereDesc);
	if(!sceneReleaseGroundActor || !sceneReleaseSphereActor)
		return nxFail("scene-release contact actor creation failed");
	sceneReleaseScene->setActorPairFlags(*sceneReleaseGroundActor,
		*sceneReleaseSphereActor, NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH |
		NX_NOTIFY_ON_END_TOUCH);
	for(unsigned step = 0; step < 60 &&
		!(sceneReleaseReport.events & NX_NOTIFY_ON_START_TOUCH); ++step)
		{
		sceneReleaseScene->simulate(1.0f / 60.0f);
		const bool ready = sceneReleaseScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = sceneReleaseScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("scene-release contact step was not ready and fetched");
		}
	const unsigned sceneReleaseCallsBefore = sceneReleaseReport.calls;
	const bool sceneReleaseStartedTouch = sceneReleaseCallsBefore != 0 &&
		(sceneReleaseReport.lastEvents & NX_NOTIFY_ON_START_TOUCH) != 0;
	sdk->releaseScene(*sceneReleaseScene);
	const bool sceneReleaseEndTouch =
		(sceneReleaseReport.lastEvents & NX_NOTIFY_ON_END_TOUCH) != 0;
	const bool sceneReleaseActorsMatch =
		(sceneReleaseReport.lastActors[0] == sceneReleaseGroundActor &&
		 sceneReleaseReport.lastActors[1] == sceneReleaseSphereActor) ||
		(sceneReleaseReport.lastActors[1] == sceneReleaseGroundActor &&
		 sceneReleaseReport.lastActors[0] == sceneReleaseSphereActor);
	printf("simulation scene-release-contact start=%u before=%u calls=%u events=%08x end_touch=%u actors=%u\n",
		sceneReleaseStartedTouch, sceneReleaseCallsBefore,
		sceneReleaseReport.calls, sceneReleaseReport.lastEvents,
		sceneReleaseEndTouch, sceneReleaseActorsMatch);
	if(!sceneReleaseStartedTouch || !sceneReleaseEndTouch || !sceneReleaseActorsMatch ||
		sceneReleaseReport.calls <= sceneReleaseCallsBefore)
		return nxFail("scene release did not deliver the final end-touch report");

	// The box-plane path exercises a distinct narrow-phase emitter and the
	// angular contact response. Keep every step in the differential transcript
	// so the first divergence identifies the responsible solver stage.
	NxSimulationContactStreamReport boxContactReport;
	NxSceneDesc boxContactSceneDesc;
	boxContactSceneDesc.setToDefault();
	boxContactSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	boxContactSceneDesc.timeStepMethod = NX_TIMESTEP_VARIABLE;
	boxContactSceneDesc.userContactReport = &boxContactReport;
	NxScene* boxContactScene = sdk->createScene(boxContactSceneDesc);
	if(!boxContactScene)
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("box contact scene creation failed");
		}
	NxPlaneShapeDesc boxGround;
	NxActorDesc boxGroundDesc;
	boxGroundDesc.shapes.pushBack(&boxGround);
	NxActor* boxGroundActor = boxContactScene->createActor(boxGroundDesc);
	NxBoxShapeDesc fallingBox;
	fallingBox.dimensions = NxVec3(0.5f, 0.5f, 0.5f);
	NxBodyDesc fallingBoxBody;
	fallingBoxBody.mass = 1.0f;
	fallingBoxBody.massSpaceInertia = NxVec3(1.0f / 6.0f, 1.0f / 6.0f, 1.0f / 6.0f);
	NxActorDesc fallingBoxDesc;
	fallingBoxDesc.body = &fallingBoxBody;
	fallingBoxDesc.shapes.pushBack(&fallingBox);
	fallingBoxDesc.globalPose.t = NxVec3(0.0f, 1.0f, 0.0f);
	NxActor* fallingBoxActor = boxContactScene->createActor(fallingBoxDesc);
	if(!boxGroundActor || !fallingBoxActor)
		{
		sdk->releaseScene(*boxContactScene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("box contact actors creation failed");
		}
	boxContactScene->setActorPairFlags(*boxGroundActor, *fallingBoxActor,
			NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH);
	nxPrintBoxActorState("boxcontact-init", *fallingBoxActor);
	for(unsigned step = 0; step < 60; ++step)
		{
		boxContactScene->simulate(1.0f / 60.0f);
		const bool ready = boxContactScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = boxContactScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			{
			sdk->releaseScene(*boxContactScene);
			sdk->release();
			FreeLibrary(physics);
			return nxFail("box contact scene result was not ready and fetched");
			}
		char boxContactStage[20];
		sprintf_s(boxContactStage, "boxcontact%u", step);
		nxPrintBoxActorState(boxContactStage, *fallingBoxActor);
		}
	printf("simulation box-contact callbacks=%u events=%08x pairs=%u patches=%u points=%u normal=%08x.%08x.%08x point=%08x.%08x.%08x separation=%08x\n",
		boxContactReport.calls, boxContactReport.events, boxContactReport.pairs,
		boxContactReport.patches, boxContactReport.points,
		boxContactReport.normal[0], boxContactReport.normal[1], boxContactReport.normal[2],
		boxContactReport.point[0], boxContactReport.point[1], boxContactReport.point[2],
		boxContactReport.separation);
	for(unsigned callback = 0; callback < boxContactReport.capturedCallbacks; ++callback)
		for(unsigned i = 0; i < boxContactReport.callbackPointCount[callback]; ++i)
			printf("simulation box-contact callback=%u point=%u xyzs=%08x.%08x.%08x.%08x\n", callback, i,
				boxContactReport.callbackPoint[callback][i][0], boxContactReport.callbackPoint[callback][i][1],
				boxContactReport.callbackPoint[callback][i][2], boxContactReport.callbackPoint[callback][i][3]);
	nxPrintBoxActorState("box-contact60", *fallingBoxActor);
	sdk->releaseScene(*boxContactScene);

	// A two-dynamic-body impact exercises pair ownership and impulse sharing;
	// the plane fixtures above only cover one movable body against static geometry.
	NxSimulationContactStreamReport spherePairReport;
	NxSceneDesc spherePairSceneDesc;
	spherePairSceneDesc.setToDefault();
	spherePairSceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	spherePairSceneDesc.timeStepMethod = NX_TIMESTEP_VARIABLE;
	spherePairSceneDesc.userContactReport = &spherePairReport;
	NxScene* spherePairScene = sdk->createScene(spherePairSceneDesc);
	if(!spherePairScene)
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("sphere-pair scene creation failed");
		}
	NxSphereShapeDesc pairSphereA;
	NxSphereShapeDesc pairSphereB;
	pairSphereA.radius = 0.5f;
	pairSphereB.radius = 0.5f;
	NxBodyDesc pairBodyA;
	pairBodyA.mass = 1.0f;
	pairBodyA.massSpaceInertia = NxVec3(0.1f, 0.1f, 0.1f);
	pairBodyA.linearVelocity = NxVec3(1.0f, 0.0f, 0.0f);
	NxActorDesc pairActorDescA;
	pairActorDescA.body = &pairBodyA;
	pairActorDescA.globalPose.t = NxVec3(-1.0f, 0.0f, 0.0f);
	pairActorDescA.shapes.pushBack(&pairSphereA);
	NxBodyDesc pairBodyB;
	pairBodyB.mass = 1.0f;
	pairBodyB.massSpaceInertia = NxVec3(0.1f, 0.1f, 0.1f);
	pairBodyB.linearVelocity = NxVec3(-1.0f, 0.0f, 0.0f);
	NxActorDesc pairActorDescB;
	pairActorDescB.body = &pairBodyB;
	pairActorDescB.globalPose.t = NxVec3(1.0f, 0.0f, 0.0f);
	pairActorDescB.shapes.pushBack(&pairSphereB);
	NxActor* pairActorA = spherePairScene->createActor(pairActorDescA);
	NxActor* pairActorB = spherePairScene->createActor(pairActorDescB);
	printf("simulation sphere-pair creation valid=%u.%u actors=%u.%u\n",
		pairActorDescA.isValid() ? 1u : 0u, pairActorDescB.isValid() ? 1u : 0u,
		pairActorA ? 1u : 0u, pairActorB ? 1u : 0u);
	if(!pairActorA || !pairActorB)
		{
		sdk->releaseScene(*spherePairScene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("sphere-pair actors creation failed");
		}
	spherePairScene->setActorPairFlags(*pairActorA, *pairActorB,
			NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH);
	nxPrintActorState("spherepair-init-a", *pairActorA);
	nxPrintActorState("spherepair-init-b", *pairActorB);
	for(unsigned step = 0; step < 60; ++step)
		{
		spherePairScene->simulate(1.0f / 60.0f);
		const bool ready = spherePairScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = spherePairScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			{
			sdk->releaseScene(*spherePairScene);
			sdk->release();
			FreeLibrary(physics);
			return nxFail("sphere-pair scene result was not ready and fetched");
			}
		char pairStage[24];
		sprintf_s(pairStage, "spherepair%u-a", step);
		nxPrintActorState(pairStage, *pairActorA);
		sprintf_s(pairStage, "spherepair%u-b", step);
		nxPrintActorState(pairStage, *pairActorB);
		}
	printf("simulation sphere-pair callbacks=%u events=%08x pairs=%u patches=%u points=%u\n",
		spherePairReport.calls, spherePairReport.events, spherePairReport.pairs,
		spherePairReport.patches, spherePairReport.points);
	nxPrintActorState("spherepair60-a", *pairActorA);
	nxPrintActorState("spherepair60-b", *pairActorB);
	sdk->releaseScene(*spherePairScene);

	// Tangential contact response: a moving sphere settles onto a plane and
	// loses horizontal speed while gaining angular velocity through friction.
	NxMaterial frictionMaterial;
	frictionMaterial.staticFriction = 0.8f;
	frictionMaterial.dynamicFriction = 0.6f;
	const NxMaterialIndex frictionIndex = sdk->addMaterial(frictionMaterial);
	NxSimulationContactStreamReport frictionReport;
	NxSceneDesc frictionSceneDesc;
	frictionSceneDesc.setToDefault();
	frictionSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	frictionSceneDesc.timeStepMethod = NX_TIMESTEP_VARIABLE;
	frictionSceneDesc.userContactReport = &frictionReport;
	NxScene* frictionScene = sdk->createScene(frictionSceneDesc);
	if(!frictionScene)
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("friction scene creation failed");
		}
	NxPlaneShapeDesc frictionGround;
	frictionGround.materialIndex = frictionIndex;
	NxActorDesc frictionGroundDesc;
	frictionGroundDesc.shapes.pushBack(&frictionGround);
	NxActor* frictionGroundActor = frictionScene->createActor(frictionGroundDesc);
	NxSphereShapeDesc slidingSphere;
	slidingSphere.radius = 0.5f;
	slidingSphere.materialIndex = frictionIndex;
	NxBodyDesc slidingBody;
	slidingBody.mass = 1.0f;
	slidingBody.massSpaceInertia = NxVec3(0.1f, 0.1f, 0.1f);
	slidingBody.linearVelocity = NxVec3(2.0f, 0.0f, 0.0f);
	NxActorDesc slidingDesc;
	slidingDesc.body = &slidingBody;
	slidingDesc.globalPose.t = NxVec3(0.0f, 0.5f, 0.0f);
	slidingDesc.shapes.pushBack(&slidingSphere);
	NxActor* slidingActor = frictionScene->createActor(slidingDesc);
	printf("simulation friction setup material=%u material_valid=%u ground_valid=%u desc_valid=%u actors=%u.%u\n",
		(unsigned)frictionIndex, frictionMaterial.isValid() ? 1u : 0u,
		frictionGround.isValid() ? 1u : 0u, slidingDesc.isValid() ? 1u : 0u,
		frictionGroundActor ? 1u : 0u, slidingActor ? 1u : 0u);
	if(!frictionGroundActor || !slidingActor)
		{
		sdk->releaseScene(*frictionScene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("friction actors creation failed");
		}
	frictionScene->setActorPairFlags(*frictionGroundActor, *slidingActor,
			NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH);
	nxPrintActorMotionState("friction-init", *slidingActor);
	for(unsigned step = 0; step < 60; ++step)
		{
		frictionScene->simulate(1.0f / 60.0f);
		const bool ready = frictionScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = frictionScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			{
			sdk->releaseScene(*frictionScene);
			sdk->release();
			FreeLibrary(physics);
			return nxFail("friction scene result was not ready and fetched");
			}
		char frictionStage[24];
		sprintf_s(frictionStage, "friction%u", step);
		nxPrintActorMotionState(frictionStage, *slidingActor);
		}
	printf("simulation friction callbacks=%u events=%08x pairs=%u patches=%u points=%u coefficients=%08x.%08x\n",
		frictionReport.calls, frictionReport.events, frictionReport.pairs,
		frictionReport.patches, frictionReport.points,
		nxFloatBits(frictionMaterial.staticFriction), nxFloatBits(frictionMaterial.dynamicFriction));
	printf("simulation friction pair-force normal=%08x.%08x.%08x friction=%08x.%08x.%08x\n",
		frictionReport.sumNormalForce[0], frictionReport.sumNormalForce[1],
		frictionReport.sumNormalForce[2], frictionReport.sumFrictionForce[0],
		frictionReport.sumFrictionForce[1], frictionReport.sumFrictionForce[2]);
	nxPrintActorMotionState("friction60", *slidingActor);
	sdk->releaseScene(*frictionScene);

	// Spring contact uses the swept-capsule material program and emits a
	// different solver record from ordinary penalty contact.
	NxSpringDesc contactSpring(1200.0f, 25.0f, 0.05f);
	NxMaterial springMaterial;
	springMaterial.flags |= NX_MF_SPRING_CONTACT;
	springMaterial.programData = &contactSpring;
	const NxMaterialIndex springMaterialIndex = sdk->addMaterial(springMaterial);
	NxSimulationContactStreamReport springReport;
	NxSceneDesc springSceneDesc;
	springSceneDesc.setToDefault();
	springSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
	springSceneDesc.timeStepMethod = NX_TIMESTEP_VARIABLE;
	springSceneDesc.userContactReport = &springReport;
	NxScene* springScene = sdk->createScene(springSceneDesc);
	if(!springScene)
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("spring-contact scene creation failed");
		}
	NxBoxShapeDesc springGround;
	springGround.dimensions = NxVec3(2.0f, 0.25f, 2.0f);
	springGround.materialIndex = springMaterialIndex;
	NxActorDesc springGroundDesc;
	springGroundDesc.globalPose.t = NxVec3(0.0f, -0.25f, 0.0f);
	springGroundDesc.shapes.pushBack(&springGround);
	NxActor* springGroundActor = springScene->createActor(springGroundDesc);
	NxCapsuleShapeDesc springCapsule;
	springCapsule.radius = 0.5f;
	springCapsule.height = 1.0f;
	springCapsule.flags = NX_SWEPT_SHAPE;
	springCapsule.materialIndex = springMaterialIndex;
	NxBodyDesc springBody;
	springBody.mass = 1.0f;
	springBody.massSpaceInertia = NxVec3(0.2f, 0.1f, 0.2f);
	springBody.linearVelocity = NxVec3(0.25f, -0.5f, 0.0f);
	NxActorDesc springActorDesc;
	springActorDesc.body = &springBody;
	springActorDesc.globalPose.t = NxVec3(0.0f, 1.5f, 0.0f);
	springActorDesc.shapes.pushBack(&springCapsule);
	NxActor* springActor = springScene->createActor(springActorDesc);
	if(!springGroundActor || !springActor)
		{
		sdk->releaseScene(*springScene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("spring-contact actors creation failed");
		}
	springScene->setActorPairFlags(*springGroundActor, *springActor,
			NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_TOUCH);
	for(unsigned step = 0; step < 60; ++step)
		{
		springScene->simulate(1.0f / 60.0f);
		const bool ready = springScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = springScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			{
			sdk->releaseScene(*springScene);
			sdk->release();
			FreeLibrary(physics);
			return nxFail("spring-contact scene result was not ready and fetched");
			}
		}
	NxVec3 springPosition;
	NxVec3 springVelocity;
	springActor->getGlobalPosition(springPosition);
	springActor->getLinearVelocity(springVelocity);
	printf("simulation spring-contact callbacks=%u events=%08x pairs=%u patches=%u points=%u position=%08x.%08x.%08x velocity=%08x.%08x.%08x normal-force=%08x.%08x.%08x friction-force=%08x.%08x.%08x\n",
		springReport.calls, springReport.events, springReport.pairs,
		springReport.patches, springReport.points,
		nxFloatBits(springPosition.x), nxFloatBits(springPosition.y), nxFloatBits(springPosition.z),
		nxFloatBits(springVelocity.x), nxFloatBits(springVelocity.y), nxFloatBits(springVelocity.z),
		springReport.sumNormalForce[0], springReport.sumNormalForce[1], springReport.sumNormalForce[2],
		springReport.sumFrictionForce[0], springReport.sumFrictionForce[1], springReport.sumFrictionForce[2]);
	sdk->releaseScene(*springScene);
	}

	// The shipped SDK exports the fluid API while this build disables its
	// implementation. Keep the deterministic unsupported contract covered and
	// release this scene without simulating it: the pinned manager's disabled
	// path leaves fields uninitialised that its later step path reads.
	NxSceneDesc fluidSceneDesc;
	fluidSceneDesc.setToDefault();
	NxScene* const fluidScene = sdk->createScene(fluidSceneDesc);
	if(!fluidScene)
		return nxFail("fluid unsupported scene creation failed");
	const unsigned fluidErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	const NxU32 emptyFluidCount = fluidScene->getNbFluids();
	const bool emptyFluidList = fluidScene->getFluids() != 0;
	NxFluidDesc fluidDesc;
	fluidDesc.setToDefault();
	NxFluid* const createdFluid = fluidScene->createFluid(fluidDesc);
	const NxU32 createdFluidCount = fluidScene->getNbFluids();
	const bool createdFluidList = fluidScene->getFluids() != 0;
	unsigned char* const fluidSceneInternal = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(fluidScene) + 0x24);
	const bool fluidManagerCreated = *reinterpret_cast<void**>(fluidSceneInternal + 0x61c) != 0;
	const unsigned fluidCreateErrorCount = simulationOutput.errors - fluidErrorsBefore;
	const bool fluidDisabledCreateExact = !createdFluid && fluidManagerCreated &&
		emptyFluidCount == 0 && !emptyFluidList && createdFluidCount == 0 &&
		!createdFluidList && fluidCreateErrorCount == 1 &&
		simulationOutput.code == NXE_DB_WARNING && simulationOutput.line == 0x8a;
	printf("simulation fluid unsupported create=%u manager=%u empty=%u.%u created=%u.%u errors=%u code=%u line=%d file=%s message=%s\n",
		createdFluid != 0, fluidManagerCreated ? 1u : 0u,
		emptyFluidCount, emptyFluidList ? 1u : 0u,
		createdFluidCount, createdFluidList ? 1u : 0u,
		simulationOutput.errors - fluidErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	if(!fluidDisabledCreateExact)
		return nxFail("disabled Scene::createFluid contract did not match");
	unsigned char* const enabledValidationFluidManager = static_cast<unsigned char*>(
		*reinterpret_cast<void**>(fluidSceneInternal + 0x61c));
	if(!enabledValidationFluidManager)
		return nxFail("fluid descriptor validation manager was not created");
	enabledValidationFluidManager[0x2b] = 1;
	NxFluidDesc invalidFluidDesc;
	invalidFluidDesc.setToDefault();
	invalidFluidDesc.restDensity = 0.0f;
	const unsigned fluidValidationErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	NxFluid* const invalidFluid = fluidScene->createFluid(invalidFluidDesc);
	enabledValidationFluidManager[0x2b] = 0;
	printf("simulation fluid enabled-invalid result=%u errors=%u code=%u line=%d file=%s message=%s\n",
		invalidFluid != 0,
		simulationOutput.errors - fluidValidationErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	unsigned char fluidStorage[0x40] = {};
	unsigned char fluidIdentity = 0;
	*reinterpret_cast<void**>(fluidStorage + 0x14) = &fluidIdentity;
	const unsigned fluidReleaseErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	fluidScene->releaseFluid(*reinterpret_cast<NxFluid*>(fluidStorage));
	const bool fluidManagerReleased = *reinterpret_cast<void**>(fluidSceneInternal + 0x61c) == 0;
	printf("simulation fluid unsupported release manager-cleared=%u errors=%u code=%u line=%d file=%s message=%s\n",
		fluidManagerReleased ? 1u : 0u,
		simulationOutput.errors - fluidReleaseErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	unsigned char fluidCallbackMarker = 0;
	NxUserFluidContactReport* const fluidCallback =
		reinterpret_cast<NxUserFluidContactReport*>(&fluidCallbackMarker);
	const unsigned fluidCallbackErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	fluidScene->setUserFluidContactReport(fluidCallback);
	printf("simulation fluid contact-report-set errors=%u code=%u line=%d file=%s message=%s\n",
		simulationOutput.errors - fluidCallbackErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	const unsigned fluidCallbackGetErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	NxUserFluidContactReport* const returnedFluidCallback = fluidScene->getUserFluidContactReport();
	printf("simulation fluid contact-report-get stored=%u errors=%u code=%u line=%d file=%s message=%s\n",
		returnedFluidCallback == fluidCallback ? 1u : 0u,
		simulationOutput.errors - fluidCallbackGetErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	NxImplicitMeshDesc implicitMeshDesc;
	implicitMeshDesc.setToDefault();
	const unsigned implicitCreateErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	NxImplicitMesh* const createdImplicitMesh = fluidScene->createImplicitMesh(implicitMeshDesc);
	printf("simulation implicit-mesh-create created=%u errors=%u code=%u line=%d file=%s message=%s\n",
		createdImplicitMesh != 0 ? 1u : 0u,
		simulationOutput.errors - implicitCreateErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	unsigned char implicitMeshMarker = 0;
	const unsigned implicitReleaseErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	fluidScene->releaseImplicitMesh(*reinterpret_cast<NxImplicitMesh*>(&implicitMeshMarker));
	printf("simulation implicit-mesh-release errors=%u code=%u line=%d file=%s message=%s\n",
		simulationOutput.errors - implicitReleaseErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	const unsigned implicitCountErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	const NxU32 implicitMeshCount = fluidScene->getNbImplicitMeshes();
	printf("simulation implicit-mesh-count count=%u errors=%u code=%u line=%d file=%s message=%s\n",
		implicitMeshCount,
		simulationOutput.errors - implicitCountErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	const unsigned implicitListErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	NxImplicitMesh** const implicitMeshes = fluidScene->getImplicitMeshes();
	printf("simulation implicit-mesh-list present=%u errors=%u code=%u line=%d file=%s message=%s\n",
		implicitMeshes != 0 ? 1u : 0u,
		simulationOutput.errors - implicitListErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	sdk->releaseScene(*fluidScene);
	// The disabled FluidManager is still stepped by the scene scheduler. Pin
	// its uninitialised extension dirty flag to the false branch on both paired
	// processes so this fixture measures the manager warnings without invoking
	// extension callbacks that the shipped installation does not provide.
	NxScene* const fluidStepScene = sdk->createScene(fluidSceneDesc);
	if(!fluidStepScene)
		return nxFail("fluid manager step scene creation failed");
	fluidStepScene->setTiming(1.0f / 60.0f, 4, NX_TIMESTEP_FIXED);
	if(fluidStepScene->createFluid(fluidDesc) != 0)
		return nxFail("disabled fluid manager unexpectedly created a fluid");
	unsigned char* const fluidStepSceneInternal = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(fluidStepScene) + 0x24);
	unsigned char* const fluidStepManager = static_cast<unsigned char*>(
		*reinterpret_cast<void**>(fluidStepSceneInternal + 0x61c));
	if(!fluidStepManager)
		return nxFail("fluid manager step fixture has no manager");
	fluidStepManager[0x28] = 0;
	simulationOutput.resetLast();
	const unsigned fluidStepErrorsBefore = simulationOutput.errors;
	bool fluidStepReady = true;
	bool fluidStepFetched = true;
	for(unsigned step = 0; step < 2; ++step)
		{
		fluidStepScene->simulate(1.0f / 60.0f);
		fluidStepReady = fluidStepReady &&
			fluidStepScene->checkResults(NX_RIGID_BODY_FINISHED, true);
		fluidStepFetched = fluidStepFetched &&
			fluidStepScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		}
	printf("simulation fluid manager-step ready=%u fetched=%u frames=2 errors=%u code=%u line=%d file=%s message=%s\n",
		fluidStepReady ? 1u : 0u, fluidStepFetched ? 1u : 0u,
		simulationOutput.errors - fluidStepErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	sdk->releaseScene(*fluidStepScene);
	// Seed the manager's two parallel fluid arrays with a concrete fake fluid
	// so release exercises its swap-removal and scalar-deleting dispatch, even
	// though the shipped build cannot create a backend fluid itself.
	NxScene* const fluidArrayScene = sdk->createScene(fluidSceneDesc);
	if(!fluidArrayScene)
		return nxFail("fluid array scene creation failed");
	fluidArrayScene->createFluid(fluidDesc);
	unsigned char* const fluidArraySceneInternal = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(fluidArrayScene) + 0x24);
	unsigned char* const fluidArrayManager = static_cast<unsigned char*>(
		*reinterpret_cast<void**>(fluidArraySceneInternal + 0x61c));
	if(!fluidArrayManager)
		return nxFail("fluid array manager was not installed");
	NxSphereShapeDesc fluidNotifyShape;
	fluidNotifyShape.setToDefault();
	fluidNotifyShape.radius = 0.5f;
	NxActorDesc fluidNotifyActorDesc;
	fluidNotifyActorDesc.setToDefault();
	fluidNotifyActorDesc.shapes.pushBack(&fluidNotifyShape);
	simulationOutput.resetLast();
	const unsigned fluidNotifyCreateErrorsBefore = simulationOutput.errors;
	NxActor* const fluidNotifyActor = fluidArrayScene->createActor(fluidNotifyActorDesc);
	printf("simulation fluid manager actor-created present=%u errors=%u code=%u line=%d file=%s message=%s\n",
		fluidNotifyActor ? 1u : 0u,
		simulationOutput.errors - fluidNotifyCreateErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	if(!fluidNotifyActor)
		return nxFail("fluid manager actor-create notification fixture failed");
	simulationOutput.resetLast();
	const unsigned fluidNotifyReleaseErrorsBefore = simulationOutput.errors;
	fluidArrayScene->releaseActor(*fluidNotifyActor);
	printf("simulation fluid manager actor-released errors=%u code=%u line=%d file=%s message=%s\n",
		simulationOutput.errors - fluidNotifyReleaseErrorsBefore,
		static_cast<unsigned>(simulationOutput.code), simulationOutput.line,
		simulationOutput.file, simulationOutput.message);
	void* fluidDeletingVtable[] = {
		reinterpret_cast<void*>(nxSimulationFluidDeletingDestructor)
		};
	unsigned char fluidInternalA[0x20] = {};
	unsigned char fluidInternalB[0x20] = {};
	*reinterpret_cast<void***>(fluidInternalA) = fluidDeletingVtable;
	*reinterpret_cast<void***>(fluidInternalB) = fluidDeletingVtable;
	unsigned fluidPointers[] = {
		reinterpret_cast<unsigned>(fluidInternalA),
		reinterpret_cast<unsigned>(fluidInternalB)
		};
	unsigned secondaryPointers[] = { 0x11111111u, 0x22222222u };
	*reinterpret_cast<unsigned*>(fluidArrayManager + 4) = reinterpret_cast<unsigned>(fluidPointers);
	*reinterpret_cast<unsigned*>(fluidArrayManager + 8) = reinterpret_cast<unsigned>(fluidPointers + 2);
	*reinterpret_cast<unsigned*>(fluidArrayManager + 0xc) = reinterpret_cast<unsigned>(fluidPointers + 2);
	*reinterpret_cast<unsigned*>(fluidArrayManager + 0x14) = reinterpret_cast<unsigned>(secondaryPointers);
	*reinterpret_cast<unsigned*>(fluidArrayManager + 0x18) = reinterpret_cast<unsigned>(secondaryPointers + 2);
	*reinterpret_cast<unsigned*>(fluidArrayManager + 0x1c) = reinterpret_cast<unsigned>(secondaryPointers + 2);
	unsigned char publicFluidStorage[0x40] = {};
	*reinterpret_cast<void**>(publicFluidStorage + 0x14) = fluidInternalA;
	gNxSimulationFluidDestructorCalls = 0;
	gNxSimulationFluidDestructorFlags = 0;
	gNxSimulationFluidDestructorObject = 0;
	fluidArrayScene->releaseFluid(*reinterpret_cast<NxFluid*>(publicFluidStorage));
	const unsigned fluidArrayRemaining =
		(*reinterpret_cast<unsigned*>(fluidArrayManager + 8) -
		 *reinterpret_cast<unsigned*>(fluidArrayManager + 4)) >> 2;
	const unsigned secondaryArrayRemaining =
		(*reinterpret_cast<unsigned*>(fluidArrayManager + 0x18) -
		 *reinterpret_cast<unsigned*>(fluidArrayManager + 0x14)) >> 2;
	printf("simulation fluid array-release remaining=%u secondary=%u swapped=%u.%u destructor=%u flags=%u target=%u\n",
		fluidArrayRemaining, secondaryArrayRemaining,
		fluidPointers[0] == reinterpret_cast<unsigned>(fluidInternalB) ? 1u : 0u,
		secondaryPointers[0] == 0x22222222u ? 1u : 0u,
		gNxSimulationFluidDestructorCalls, gNxSimulationFluidDestructorFlags,
		gNxSimulationFluidDestructorObject == fluidInternalA ? 1u : 0u);
	// The two arrays above are stack fixtures rather than allocator-owned SDK
	// storage. Empty their headers before releasing the scene so the manager
	// destructor does not try to free those test buffers.
	*reinterpret_cast<unsigned*>(fluidArrayManager + 4) = 0;
	*reinterpret_cast<unsigned*>(fluidArrayManager + 8) = 0;
	*reinterpret_cast<unsigned*>(fluidArrayManager + 0xc) = 0;
	*reinterpret_cast<unsigned*>(fluidArrayManager + 0x14) = 0;
	*reinterpret_cast<unsigned*>(fluidArrayManager + 0x18) = 0;
	*reinterpret_cast<unsigned*>(fluidArrayManager + 0x1c) = 0;
	sdk->releaseScene(*fluidArrayScene);
	// The manager is an internal C++ object. Exercise its first vtable slot
	// through the pinned scalar-deleting destructor instead of treating the
	// manager as a byte buffer only.
	NxScene* const fluidVtableScene = sdk->createScene(fluidSceneDesc);
	if(!fluidVtableScene)
		return nxFail("fluid vtable scene creation failed");
	fluidVtableScene->createFluid(fluidDesc);
	unsigned char* const fluidVtableSceneInternal = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(fluidVtableScene) + 0x24);
	void* const fluidManager = *reinterpret_cast<void**>(fluidVtableSceneInternal + 0x61c);
	if(!fluidManager)
		return nxFail("fluid manager was not installed by createFluid");
	unsigned char* const fluidManagerBytes = static_cast<unsigned char*>(fluidManager);
	const bool fluidManagerOwnerMatches =
		*reinterpret_cast<void**>(fluidManagerBytes + 0x24) == fluidVtableSceneInternal;
	const bool fluidManagerArraysEmpty =
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 4) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 8) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 0xc) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x14) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x18) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x1c) == 0;
	printf("simulation fluid manager-constructor owner=%u arrays-empty=%u initialized=%u extension=%u available=%u\n",
		fluidManagerOwnerMatches ? 1u : 0u, fluidManagerArraysEmpty ? 1u : 0u,
		fluidManagerBytes[0x29], fluidManagerBytes[0x2a], fluidManagerBytes[0x2b]);
	void** const fluidManagerVtable = fluidManager ? *reinterpret_cast<void***>(fluidManager) : 0;
	if(!fluidManagerVtable || !fluidManagerVtable[0])
		return nxFail("fluid manager vtable destructor missing");
	typedef void* (__thiscall *FluidManagerDeletingDestructorFn)(void*, unsigned char);
	FluidManagerDeletingDestructorFn fluidManagerDeletingDestructor =
		reinterpret_cast<FluidManagerDeletingDestructorFn>(fluidManagerVtable[0]);
	HMODULE foundation = GetModuleHandleW(L"NxFoundation.dll");
	CreateFoundationSDKFn createFoundation = foundation ?
		reinterpret_cast<CreateFoundationSDKFn>(GetProcAddress(foundation, "NxCreateFoundationSDK")) : 0;
	NxFoundationSDK* foundationSDK = createFoundation ?
		createFoundation(NX_FOUNDATION_SDK_VERSION, &simulationOutput, 0) : 0;
	if(!foundationSDK)
		return nxFail("Foundation allocator access failed for fluid manager teardown");
	NxUserAllocator* foundationAllocator = &foundationSDK->getAllocator();
	unsigned* managerFluids = static_cast<unsigned*>(
		foundationAllocator->malloc(2 * sizeof(unsigned), NX_MEMORY_PERSISTENT));
	unsigned* managerSecondary = static_cast<unsigned*>(
		foundationAllocator->malloc(sizeof(unsigned), NX_MEMORY_PERSISTENT));
	if(!managerFluids || !managerSecondary)
		return nxFail("fluid manager teardown arrays allocation failed");
	unsigned char managerFluidA[0x20] = {};
	unsigned char managerFluidB[0x20] = {};
	*reinterpret_cast<void***>(managerFluidA) = fluidDeletingVtable;
	*reinterpret_cast<void***>(managerFluidB) = fluidDeletingVtable;
	managerFluids[0] = reinterpret_cast<unsigned>(managerFluidA);
	managerFluids[1] = reinterpret_cast<unsigned>(managerFluidB);
	managerSecondary[0] = 0;
	*reinterpret_cast<unsigned*>(fluidManagerBytes + 4) = reinterpret_cast<unsigned>(managerFluids);
	*reinterpret_cast<unsigned*>(fluidManagerBytes + 8) = reinterpret_cast<unsigned>(managerFluids + 2);
	*reinterpret_cast<unsigned*>(fluidManagerBytes + 0xc) = reinterpret_cast<unsigned>(managerFluids + 2);
	*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x14) = reinterpret_cast<unsigned>(managerSecondary);
	*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x18) = reinterpret_cast<unsigned>(managerSecondary + 1);
	*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x1c) = reinterpret_cast<unsigned>(managerSecondary + 1);
	gNxSimulationFluidDestructorCalls = 0;
	gNxSimulationFluidDestructorFlags = 0;
	gNxSimulationFluidDestructorObject = 0;
	fluidManagerDeletingDestructor(fluidManager, 0);
	const bool managerArraysCleared =
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 4) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 8) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 0xc) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x14) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x18) == 0 &&
		*reinterpret_cast<unsigned*>(fluidManagerBytes + 0x1c) == 0;
	const bool populatedManagerDestroyed = gNxSimulationFluidDestructorCalls == 2 &&
		gNxSimulationFluidDestructorFlags == 1 &&
		gNxSimulationFluidDestructorObject == managerFluidB && managerArraysCleared;
	printf("simulation fluid manager-populated-destructor calls=%u flags=%u target=%u arrays-cleared=%u\n",
		gNxSimulationFluidDestructorCalls, gNxSimulationFluidDestructorFlags,
		gNxSimulationFluidDestructorObject == managerFluidB ? 1u : 0u,
		managerArraysCleared ? 1u : 0u);
	if(!populatedManagerDestroyed)
		return nxFail("fluid manager populated deleting destructor mismatch");
	fluidManagerDeletingDestructor(fluidManager, 1);
	*reinterpret_cast<void**>(fluidVtableSceneInternal + 0x61c) = 0;
	printf("simulation fluid manager-vtable deleting-destructor=1\n");
	sdk->releaseScene(*fluidVtableScene);
	// Exercise the real extension-enabled create/release path with a deliberately
	// small FluidModel companion fixture. Load it by absolute path after the
	// disabled-backend cases above so those continue to measure the shipped
	// missing-plugin behavior.
	wchar_t fluidModelPath[MAX_PATH];
	if(swprintf_s(fluidModelPath, L"%s\\FluidModel.dll", pairDirectory) < 0)
		return nxFail("fluid model fixture path construction failed");
	HMODULE fluidModel = LoadLibraryExW(fluidModelPath, 0,
		LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
	if(!fluidModel)
		return nxFail("staged fluid model callback fixture failed to load");
	typedef void (__cdecl *ResetFluidManagerCallbacksFn)();
	typedef unsigned (__cdecl *GetFluidDampingCallsFn)();
	typedef void (__cdecl *GetFluidSetStiffnessObservationFn)(
		unsigned*, unsigned*, unsigned*);
	typedef void (__cdecl *GetFluidSetViscosityObservationFn)(
		unsigned*, unsigned*, unsigned*);
	typedef void (__cdecl *GetFluidSetDampingObservationFn)(
		unsigned*, unsigned*, unsigned*);
	typedef void (__cdecl *GetFluidSetStaticCollisionRestitutionObservationFn)(
		unsigned*, unsigned*, unsigned*);
	typedef void (__cdecl *GetFluidSetStaticCollisionAdhesionObservationFn)(
		unsigned*, unsigned*, unsigned*);
	typedef void (__cdecl *GetFluidSetDynamicCollisionRestitutionObservationFn)(
		unsigned*, unsigned*, unsigned*);
	typedef void (__cdecl *GetFluidSetDynamicCollisionAdhesionObservationFn)(
		unsigned*, unsigned*, unsigned*);
	struct FluidManagerCallbackSummary
		{
		unsigned managerCreate;
		unsigned managerDestroy;
		unsigned fluidCreate;
		unsigned fluidDestroy;
		unsigned staticCollisionCreate;
		unsigned staticCollisionGet;
		unsigned fluidGetCellSize;
		unsigned staticCollisionDestroy;
		unsigned fluidCreateArguments[15];
		unsigned fluidGetRestParticleDistance;
		unsigned emitterCreate;
		unsigned emitterDestroy;
		unsigned emitterSetGlobalPose;
		unsigned emitterGetGlobalPose;
		unsigned emitterCreateArguments[4];
		unsigned fluidGetStaticCollisionRestitution;
		unsigned fluidGetStaticCollisionAdhesion;
		unsigned fluidGetMaxParticles;
		unsigned fluidGetKernelRadiusMultiplier;
		unsigned fluidGetRestDensity;
		unsigned fluidGetSimulationMethod;
		unsigned fluidGetParticleMass;
		unsigned implicitFluidMeshCreate;
		unsigned implicitFluidMeshCreateArguments[7];
		unsigned implicitFluidMeshDestroy;
		unsigned implicitFluidMeshGetSurfaceRadius;
		unsigned implicitFluidMeshSetSurfaceRadius;
		unsigned implicitFluidMeshGetSingleRadius;
		unsigned implicitFluidMeshSetSingleRadius;
		unsigned implicitFluidMeshSetBlend;
		unsigned implicitFluidMeshGetTriangleSize;
		unsigned implicitFluidMeshSetTriangleSize;
		unsigned implicitFluidMeshGetSmoothingIterations;
		unsigned implicitFluidMeshSetSmoothingIterations;
		unsigned implicitFluidMeshCallbackArgumentErrors;
		unsigned implicitFluidMeshReentryAttempts;
		};
	typedef void (__cdecl *GetFluidManagerCallbackSummaryFn)(FluidManagerCallbackSummary*);
	typedef void (__cdecl *SetFluidEmitterCreateNullIndexFn)(unsigned);
	typedef void (__cdecl *SetFluidCreateNullIndexFn)(unsigned);
	typedef unsigned (__cdecl *GetFluidCreationOrderFn)(char*, unsigned);
	typedef unsigned (__cdecl *GetFluidEmitterCreateRecordFn)(unsigned,
		unsigned*, unsigned);
	typedef void (__cdecl *SetImplicitMeshReentryTargetFn)(void*);
	typedef unsigned (__cdecl *GetFluidScalarCallsFn)();
	ResetFluidManagerCallbacksFn resetFluidManagerCallbacks = reinterpret_cast<
		ResetFluidManagerCallbacksFn>(GetProcAddress(fluidModel,
			"NxResetFluidManagerCallbacks"));
	GetFluidDampingCallsFn getFluidDampingCalls = reinterpret_cast<
		GetFluidDampingCallsFn>(GetProcAddress(fluidModel,
			"NxGetFluidDampingCalls"));
	GetFluidSetStiffnessObservationFn getFluidSetStiffnessObservation =
		reinterpret_cast<GetFluidSetStiffnessObservationFn>(GetProcAddress(
			fluidModel, "NxGetFluidSetStiffnessObservation"));
	GetFluidSetViscosityObservationFn getFluidSetViscosityObservation =
		reinterpret_cast<GetFluidSetViscosityObservationFn>(GetProcAddress(
			fluidModel, "NxGetFluidSetViscosityObservation"));
	GetFluidSetDampingObservationFn getFluidSetDampingObservation =
		reinterpret_cast<GetFluidSetDampingObservationFn>(GetProcAddress(
			fluidModel, "NxGetFluidSetDampingObservation"));
	GetFluidSetStaticCollisionRestitutionObservationFn getFluidSetStaticCollisionRestitutionObservation =
		reinterpret_cast<GetFluidSetStaticCollisionRestitutionObservationFn>(GetProcAddress(
			fluidModel, "NxGetFluidSetStaticCollisionRestitutionObservation"));
	GetFluidSetStaticCollisionAdhesionObservationFn getFluidSetStaticCollisionAdhesionObservation =
		reinterpret_cast<GetFluidSetStaticCollisionAdhesionObservationFn>(GetProcAddress(
			fluidModel, "NxGetFluidSetStaticCollisionAdhesionObservation"));
	GetFluidSetDynamicCollisionRestitutionObservationFn getFluidSetDynamicCollisionRestitutionObservation =
		reinterpret_cast<GetFluidSetDynamicCollisionRestitutionObservationFn>(GetProcAddress(
			fluidModel, "NxGetFluidSetDynamicCollisionRestitutionObservation"));
	GetFluidSetDynamicCollisionAdhesionObservationFn getFluidSetDynamicCollisionAdhesionObservation =
		reinterpret_cast<GetFluidSetDynamicCollisionAdhesionObservationFn>(GetProcAddress(
			fluidModel, "NxGetFluidSetDynamicCollisionAdhesionObservation"));
	GetFluidScalarCallsFn getFluidStaticRestitutionCalls = reinterpret_cast<
		GetFluidScalarCallsFn>(GetProcAddress(fluidModel,
			"NxGetFluidStaticCollisionRestitutionCalls"));
	GetFluidScalarCallsFn getFluidStaticAdhesionCalls = reinterpret_cast<
		GetFluidScalarCallsFn>(GetProcAddress(fluidModel,
			"NxGetFluidStaticCollisionAdhesionCalls"));
	GetFluidManagerCallbackSummaryFn getFluidManagerCallbackSummary = reinterpret_cast<
		GetFluidManagerCallbackSummaryFn>(GetProcAddress(fluidModel,
			"NxGetFluidManagerCallbackSummary"));
	SetFluidEmitterCreateNullIndexFn setFluidEmitterCreateNullIndex = reinterpret_cast<
		SetFluidEmitterCreateNullIndexFn>(GetProcAddress(fluidModel,
			"NxSetFluidEmitterCreateNullIndex"));
	SetFluidCreateNullIndexFn setFluidCreateNullIndex = reinterpret_cast<
		SetFluidCreateNullIndexFn>(GetProcAddress(fluidModel,
			"NxSetFluidCreateNullIndex"));
	GetFluidCreationOrderFn getFluidCreationOrder = reinterpret_cast<
		GetFluidCreationOrderFn>(GetProcAddress(fluidModel,
			"NxGetFluidCreationOrder"));
	GetFluidEmitterCreateRecordFn getFluidEmitterCreateRecord = reinterpret_cast<
		GetFluidEmitterCreateRecordFn>(GetProcAddress(fluidModel,
			"NxGetFluidEmitterCreateRecord"));
	SetImplicitMeshReentryTargetFn setImplicitMeshReentryTarget = reinterpret_cast<
		SetImplicitMeshReentryTargetFn>(GetProcAddress(fluidModel,
			"NxSetImplicitMeshReentryTarget"));
	if(!resetFluidManagerCallbacks || !getFluidDampingCalls ||
		!getFluidSetStiffnessObservation || !getFluidSetViscosityObservation ||
		!getFluidSetDampingObservation ||
		!getFluidSetStaticCollisionRestitutionObservation ||
		!getFluidSetStaticCollisionAdhesionObservation ||
		!getFluidSetDynamicCollisionRestitutionObservation ||
		!getFluidSetDynamicCollisionAdhesionObservation ||
		!getFluidStaticRestitutionCalls || !getFluidStaticAdhesionCalls ||
		!getFluidManagerCallbackSummary ||
		!setFluidEmitterCreateNullIndex || !setFluidCreateNullIndex ||
		!getFluidCreationOrder || !getFluidEmitterCreateRecord ||
		!setImplicitMeshReentryTarget)
		return nxFail("fluid model manager callback controls are missing");
	resetFluidManagerCallbacks();
	HMODULE const fluidFoundationModule = GetModuleHandleW(L"NxFoundation.dll");
	NxUserAllocator** const fluidFoundationAllocatorSlot = fluidFoundationModule
		? reinterpret_cast<NxUserAllocator**>(GetProcAddress(
			fluidFoundationModule, "?nxFoundationSDKAllocator@@3PAVNxUserAllocator@@A"))
		: 0;
	if(!fluidFoundationAllocatorSlot || !*fluidFoundationAllocatorSlot)
		return nxFail("fluid creation allocator hook is unavailable");
	NxScene* const managerAllocationFailureScene = sdk->createScene(fluidSceneDesc);
	if(!managerAllocationFailureScene)
		return nxFail("fluid manager allocation-failure scene creation failed");
	NxUserAllocator* const savedFluidAllocator = *fluidFoundationAllocatorSlot;
	NxSimulationFailSizeAllocator managerFailAllocator(*savedFluidAllocator, 0x34);
	*fluidFoundationAllocatorSlot = &managerFailAllocator;
	simulationOutput.resetLast();
	const unsigned managerAllocationFailureErrorsBefore = simulationOutput.errors;
	NxFluidDesc managerAllocationFailureDesc;
	managerAllocationFailureDesc.setToDefault();
	const NxFluid* const managerAllocationFailureFluid =
		managerAllocationFailureScene->createFluid(managerAllocationFailureDesc);
	*fluidFoundationAllocatorSlot = savedFluidAllocator;
	unsigned char* const managerAllocationFailureSceneInternal =
		*reinterpret_cast<unsigned char**>(reinterpret_cast<unsigned char*>(
			managerAllocationFailureScene) + 0x24);
	const bool managerAllocationFailure = !managerAllocationFailureFluid &&
		managerFailAllocator.mFailCount == 1 &&
		simulationOutput.errors == managerAllocationFailureErrorsBefore &&
		simulationOutput.code == NXE_NO_ERROR &&
		*reinterpret_cast<void**>(managerAllocationFailureSceneInternal + 0x61c) == 0;
	sdk->releaseScene(*managerAllocationFailureScene);
	// A null backend handle still leaves the oracle's local emitter wrapper
	// initialized and published in both fluid-owned arrays.
	setFluidEmitterCreateNullIndex(1);
	NxScene* const enabledFluidScene = sdk->createScene(fluidSceneDesc);
	if(!enabledFluidScene)
		return nxFail("enabled fluid scene creation failed");
	NxFluidDesc enabledInvalidDesc;
	enabledInvalidDesc.setToDefault();
	enabledInvalidDesc.restDensity = 0.0f;
	simulationOutput.resetLast();
	const bool enabledInvalidRejected = enabledFluidScene->createFluid(
		enabledInvalidDesc) == 0;
	const NxErrorCode enabledInvalidCode = simulationOutput.code;
	const int enabledInvalidLine = simulationOutput.line;
	unsigned char* const enabledFluidSceneInternal = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(enabledFluidScene) + 0x24);
	unsigned char* const enabledFluidManager = static_cast<unsigned char*>(
		*reinterpret_cast<void**>(enabledFluidSceneInternal + 0x61c));
	if(!enabledFluidManager)
		return nxFail("enabled fluid manager was not created");
	const bool enabledFluidManagerFlags = enabledFluidManager[0x2a] == 1 &&
		enabledFluidManager[0x2b] == 1 &&
		*reinterpret_cast<void**>(enabledFluidManager + 0x30) != 0;
	unsigned enabledDescriptorValidationPassed = 0;
	bool enabledDescriptorValidationDiagnosticsExact = true;
	NxU32 invalidParticleCount = 0;
	NxF32 invalidParticlePosition[3] = {};
	NxF32 invalidParticleVelocity[3] = {};
	NxF32 invalidParticleLife = 1.0f;
	NxU32 invalidParticleFlags = 0;
	NxU32 invalidMeshVertexCount = 0;
	NxU32 invalidMeshTriangleCount = 0;
	NxU16 invalidMeshIndex = 0;
	NxF32 invalidMeshPositions[3] = {};
	NxF32 invalidMeshNormals[3] = {};
	NxImplicitMeshDesc invalidSurfaceMesh;
	invalidSurfaceMesh.setToDefault();
	invalidSurfaceMesh.singleRadius = 0.0f;
	for(unsigned invalidCase = 0; invalidCase != 48; ++invalidCase)
		{
		NxFluidDesc invalidDesc;
		invalidDesc.setToDefault();
		switch(invalidCase)
			{
			case 0: invalidDesc.kernelRadiusMultiplier = 0.5f; break;
			case 1: invalidDesc.restParticlesPerMeter = 0.0f; break;
			case 2: invalidDesc.stiffness = 0.0f; break;
			case 3: invalidDesc.viscosity = 0.0f; break;
			case 4: invalidDesc.damping = 1.01f; break;
			case 5: invalidDesc.dynamicCollisionRestitution = -0.01f; break;
			case 6: invalidDesc.simulationMethod = 0; break;
			case 7: invalidDesc.maxParticles = 0x7d01; break;
			case 8:
				invalidDesc.initialParticleData.numParticlesPtr = &invalidParticleCount;
				break;
			case 9:
				invalidDesc.initialParticleData.numParticlesPtr = &invalidParticleCount;
				invalidDesc.initialParticleData.bufferPos = invalidParticlePosition;
				break;
			case 10:
				invalidDesc.particlesWriteData.numParticlesPtr = &invalidParticleCount;
				invalidDesc.particlesWriteData.bufferFlags = &invalidParticleFlags;
				break;
			case 11: invalidDesc.surfaceMeshDesc = &invalidSurfaceMesh; break;
			case 12:
				{
				NxFluidEmitterDesc invalidEmitter;
				invalidEmitter.setToDefault();
				invalidEmitter.dimensionX = -0.25f;
				invalidDesc.emitters.pushBack(invalidEmitter);
				break;
				}
			case 13:
				{
				NxFluidEmitterDesc validEmitter;
				validEmitter.setToDefault();
				for(unsigned i = 0; i != 21; ++i)
					invalidDesc.emitters.pushBack(validEmitter);
				break;
				}
			case 14: invalidDesc.damping = -0.01f; break;
			case 15: invalidDesc.dynamicCollisionAdhesion = -0.01f; break;
			case 16: invalidDesc.dynamicCollisionAdhesion = 1.01f; break;
			case 17: invalidDesc.dynamicCollisionRestitution = 1.01f; break;
			case 18: invalidDesc.staticCollisionAdhesion = -0.01f; break;
			case 19: invalidDesc.staticCollisionAdhesion = 1.01f; break;
			case 20: invalidDesc.staticCollisionRestitution = -0.01f; break;
			case 21: invalidDesc.staticCollisionRestitution = 1.01f; break;
			// Bit 0 is outside the two accepted turn-action bits. The shipped
			// XOR predicate rejects it for each of the three action fields.
			case 22: invalidDesc.onCollision = 1; break;
			case 23: invalidDesc.onSeparation = 1; break;
			case 24: invalidDesc.onLifetimeExpired = 1; break;
			case 25:
				invalidDesc.initialParticleData.bufferPos = invalidParticlePosition;
				break;
			case 26:
				invalidDesc.initialParticleData.numParticlesPtr = &invalidParticleCount;
				invalidDesc.initialParticleData.bufferVel = invalidParticlePosition;
				break;
			case 27:
				invalidDesc.initialParticleData.numParticlesPtr = &invalidParticleCount;
				invalidDesc.initialParticleData.bufferLife = invalidParticlePosition;
				break;
			case 28:
				invalidDesc.initialParticleData.numParticlesPtr = &invalidParticleCount;
				invalidDesc.initialParticleData.bufferFlags = &invalidParticleFlags;
				break;
			case 29:
				invalidDesc.particlesWriteData.bufferPos = invalidParticlePosition;
				break;
			case 30:
				invalidDesc.particlesWriteData.numParticlesPtr = &invalidParticleCount;
				invalidDesc.particlesWriteData.bufferPos = invalidParticlePosition;
				break;
			case 31:
				invalidSurfaceMesh.setToDefault();
				invalidSurfaceMesh.surfaceRadius = 0.0005f;
				invalidDesc.surfaceMeshDesc = &invalidSurfaceMesh;
				break;
			case 32:
				invalidSurfaceMesh.setToDefault();
				invalidSurfaceMesh.triangleSize = 0.03f;
				invalidDesc.surfaceMeshDesc = &invalidSurfaceMesh;
				break;
			case 33:
				invalidSurfaceMesh.setToDefault();
				invalidSurfaceMesh.blend = -0.01f;
				invalidDesc.surfaceMeshDesc = &invalidSurfaceMesh;
				break;
			case 34:
				invalidDesc.particlesWriteData.numParticlesPtr = &invalidParticleCount;
				invalidDesc.particlesWriteData.bufferFlags = &invalidParticleFlags;
				break;
			case 35:
				invalidDesc.particlesWriteData.numParticlesPtr = &invalidParticleCount;
				invalidDesc.particlesWriteData.bufferVel = invalidParticleVelocity;
				break;
			case 36:
				invalidDesc.particlesWriteData.numParticlesPtr = &invalidParticleCount;
				invalidDesc.particlesWriteData.bufferLife = &invalidParticleLife;
				break;
			case 37:
				invalidSurfaceMesh.setToDefault();
				invalidSurfaceMesh.triangleSize = 0.0f;
				invalidDesc.surfaceMeshDesc = &invalidSurfaceMesh;
				break;
			case 38:
				invalidSurfaceMesh.setToDefault();
				invalidSurfaceMesh.meshData.numVerticesPtr = &invalidMeshVertexCount;
				invalidDesc.surfaceMeshDesc = &invalidSurfaceMesh;
				break;
			case 39:
				invalidSurfaceMesh.setToDefault();
				invalidSurfaceMesh.meshData.verticesPosBegin = invalidMeshPositions;
				invalidDesc.surfaceMeshDesc = &invalidSurfaceMesh;
				break;
			case 40:
				invalidSurfaceMesh.setToDefault();
				invalidSurfaceMesh.meshData.numVerticesPtr = &invalidMeshVertexCount;
				invalidSurfaceMesh.meshData.verticesPosBegin = invalidMeshPositions;
				invalidSurfaceMesh.meshData.verticesNormalBegin = invalidMeshNormals;
				invalidSurfaceMesh.meshData.verticesPosByteStride = sizeof(NxF32) * 3;
				invalidDesc.surfaceMeshDesc = &invalidSurfaceMesh;
				break;
			case 41:
				invalidSurfaceMesh.setToDefault();
				invalidSurfaceMesh.meshData.numVerticesPtr = &invalidMeshVertexCount;
				invalidSurfaceMesh.meshData.verticesPosBegin = invalidMeshPositions;
				invalidSurfaceMesh.meshData.verticesNormalBegin = invalidMeshNormals;
				invalidSurfaceMesh.meshData.verticesPosByteStride = sizeof(NxF32) * 3;
				invalidSurfaceMesh.meshData.verticesNormalByteStride = sizeof(NxF32) * 3;
				invalidDesc.surfaceMeshDesc = &invalidSurfaceMesh;
				break;
			case 42:
				invalidSurfaceMesh.setToDefault();
				invalidSurfaceMesh.meshData.numVerticesPtr = &invalidMeshVertexCount;
				invalidSurfaceMesh.meshData.verticesPosBegin = invalidMeshPositions;
				invalidSurfaceMesh.meshData.verticesNormalBegin = invalidMeshNormals;
				invalidSurfaceMesh.meshData.verticesPosByteStride = sizeof(NxF32) * 3;
				invalidSurfaceMesh.meshData.verticesNormalByteStride = sizeof(NxF32) * 3;
				invalidSurfaceMesh.meshData.numTrianglesPtr = &invalidMeshTriangleCount;
				invalidDesc.surfaceMeshDesc = &invalidSurfaceMesh;
				break;
			case 43:
				invalidSurfaceMesh.setToDefault();
				invalidSurfaceMesh.meshData.numVerticesPtr = &invalidMeshVertexCount;
				invalidSurfaceMesh.meshData.verticesPosBegin = invalidMeshPositions;
				invalidSurfaceMesh.meshData.verticesNormalBegin = invalidMeshNormals;
				invalidSurfaceMesh.meshData.verticesPosByteStride = sizeof(NxF32) * 3;
				invalidSurfaceMesh.meshData.verticesNormalByteStride = sizeof(NxF32) * 3;
				invalidSurfaceMesh.meshData.numTrianglesPtr = &invalidMeshTriangleCount;
				invalidSurfaceMesh.meshData.indicesBegin = &invalidMeshIndex;
				invalidDesc.surfaceMeshDesc = &invalidSurfaceMesh;
				break;
			case 44:
				invalidSurfaceMesh.setToDefault();
				invalidSurfaceMesh.meshData.flags = 0;
				invalidSurfaceMesh.meshData.numVerticesPtr = &invalidMeshVertexCount;
				invalidSurfaceMesh.meshData.verticesPosBegin = invalidMeshPositions;
				invalidSurfaceMesh.meshData.verticesNormalBegin = invalidMeshNormals;
				invalidSurfaceMesh.meshData.verticesPosByteStride = sizeof(NxF32) * 3;
				invalidSurfaceMesh.meshData.verticesNormalByteStride = sizeof(NxF32) * 3;
				invalidSurfaceMesh.meshData.numTrianglesPtr = &invalidMeshTriangleCount;
				invalidDesc.surfaceMeshDesc = &invalidSurfaceMesh;
				break;
			case 45:
				invalidSurfaceMesh.setToDefault();
				invalidSurfaceMesh.meshData.numTrianglesPtr = &invalidMeshTriangleCount;
				invalidDesc.surfaceMeshDesc = &invalidSurfaceMesh;
				break;
			case 46:
				invalidSurfaceMesh.setToDefault();
				invalidSurfaceMesh.meshData.indicesBegin = &invalidMeshIndex;
				invalidDesc.surfaceMeshDesc = &invalidSurfaceMesh;
				break;
			default:
				invalidSurfaceMesh.setToDefault();
				invalidSurfaceMesh.meshData.numVerticesPtr = &invalidMeshVertexCount;
				invalidSurfaceMesh.meshData.verticesPosBegin = invalidMeshPositions;
				invalidSurfaceMesh.meshData.verticesNormalBegin = invalidMeshNormals;
				invalidSurfaceMesh.meshData.verticesNormalByteStride = sizeof(NxF32) * 3;
				invalidSurfaceMesh.meshData.numTrianglesPtr = &invalidMeshTriangleCount;
				invalidSurfaceMesh.meshData.indicesBegin = &invalidMeshIndex;
				invalidSurfaceMesh.meshData.indicesByteStride = sizeof(NxU16);
				invalidDesc.surfaceMeshDesc = &invalidSurfaceMesh;
				break;
			}
		const unsigned errorsBefore = simulationOutput.errors;
		simulationOutput.resetLast();
		const bool rejected = enabledFluidScene->createFluid(invalidDesc) == 0;
		if(rejected && simulationOutput.errors == errorsBefore + 1)
			++enabledDescriptorValidationPassed;
		else
			enabledDescriptorValidationDiagnosticsExact = false;
		if(simulationOutput.code != NXE_INVALID_PARAMETER ||
			simulationOutput.line != 140)
			enabledDescriptorValidationDiagnosticsExact = false;
		}
	// The initial manager collision-dirty flag must drive the same callback and
	// static-grid build sequence as the pinned FluidManager on first creation.
	enabledFluidManager[0x28] = 0;
	NxFluidDesc enabledFluidDesc;
	enabledFluidDesc.setToDefault();
	static unsigned enabledFluidUserDataMarker = 0x51f10u;
	enabledFluidDesc.userData = &enabledFluidUserDataMarker;
	NxBodyDesc fluidFrameBodyDesc;
	fluidFrameBodyDesc.setToDefault();
	fluidFrameBodyDesc.mass = 1.0f;
	fluidFrameBodyDesc.massSpaceInertia.set(1.0f, 1.0f, 1.0f);
	NxActorDesc fluidFrameActorDesc;
	fluidFrameActorDesc.body = &fluidFrameBodyDesc;
	NxReal fluidFrameRotation[9] = {
		-1.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		0.0f, 0.0f, 1.0f
		};
	fluidFrameActorDesc.globalPose.M.setRowMajor(fluidFrameRotation);
	fluidFrameActorDesc.globalPose.t.set(10.0f, -2.0f, 3.0f);
	NxActor* const fluidFrameActor = enabledFluidScene->createActor(
		fluidFrameActorDesc);
	if(!fluidFrameActor)
		return nxFail("fluid emitter frame actor creation failed");
	void* const fluidFrameActorInternal = *reinterpret_cast<void**>(
		reinterpret_cast<unsigned char*>(fluidFrameActor) + 0x14);
	NxFluidEmitterDesc enabledEmitterDesc;
	enabledEmitterDesc.setToDefault();
	enabledEmitterDesc.relPose.t.set(0.25f, -0.5f, 0.75f);
	enabledEmitterDesc.dimensionX = 0.375f;
	enabledEmitterDesc.dimensionY = 0.625f;
	enabledFluidDesc.emitters.pushBack(enabledEmitterDesc);
	NxFluidEmitterDesc nullBackendEmitterDesc;
	nullBackendEmitterDesc.setToDefault();
	nullBackendEmitterDesc.relPose.t.set(-0.75f, 0.125f, 1.25f);
	nullBackendEmitterDesc.dimensionX = 0.875f;
	nullBackendEmitterDesc.dimensionY = 0.25f;
	enabledFluidDesc.emitters.pushBack(nullBackendEmitterDesc);
	NxFluidEmitterDesc thirdEmitterDesc;
	thirdEmitterDesc.setToDefault();
	thirdEmitterDesc.relPose.t.set(1.5f, 0.25f, -0.375f);
	thirdEmitterDesc.dimensionX = 0.5f;
	thirdEmitterDesc.dimensionY = 0.75f;
	enabledFluidDesc.emitters.pushBack(thirdEmitterDesc);
	NxFluidEmitterDesc fourthEmitterDesc;
	fourthEmitterDesc.setToDefault();
	fourthEmitterDesc.relPose.t.set(-1.25f, 0.5f, 0.25f);
	fourthEmitterDesc.dimensionX = 0.625f;
	fourthEmitterDesc.dimensionY = 0.375f;
	fourthEmitterDesc.frameActor = fluidFrameActor;
	enabledFluidDesc.emitters.pushBack(fourthEmitterDesc);
	simulationOutput.resetLast();
	const unsigned enabledFluidErrorsBefore = simulationOutput.errors;
	NxSimulationFailSizeAllocator emitterFailAllocator(*savedFluidAllocator, 0x78);
	*fluidFoundationAllocatorSlot = &emitterFailAllocator;
	NxFluid* const enabledFluid = enabledFluidScene->createFluid(enabledFluidDesc);
	*fluidFoundationAllocatorSlot = savedFluidAllocator;
	const bool enabledFluidUserDataCopied = enabledFluid &&
		enabledFluid->userData == &enabledFluidUserDataMarker;
	printf("simulation fluid public-api descriptor-user-data copied=%u\n",
		enabledFluidUserDataCopied ? 1u : 0u);
	unsigned fluidBoundsExpected[6] = {
		0x3f000001u, 0xbf000002u, 0x3f800003u,
		0x40000004u, 0xc0000005u, 0x40400006u
	};
	unsigned fluidBoundsActual[6] = {
		0xdead0000u, 0xdead0001u, 0xdead0002u,
		0xdead0003u, 0xdead0004u, 0xdead0005u
	};
	unsigned fluidBoundsSaved[6] = {};
	void* const enabledFluidInternalForBounds = enabledFluid
		? *reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(
			enabledFluid) + 0x14) : 0;
	if(enabledFluidInternalForBounds)
		{
		unsigned char* const fluidBytes = static_cast<unsigned char*>(
			enabledFluidInternalForBounds);
		memcpy(fluidBoundsSaved, fluidBytes + 0x5c, sizeof(fluidBoundsSaved));
		memcpy(fluidBytes + 0x5c, fluidBoundsExpected,
			sizeof(fluidBoundsExpected));
		enabledFluid->getWorldBounds(*reinterpret_cast<NxBounds3*>(
			fluidBoundsActual));
		memcpy(fluidBytes + 0x5c, fluidBoundsSaved, sizeof(fluidBoundsSaved));
		}
	const bool enabledFluidWorldBoundsMatch = enabledFluidInternalForBounds &&
		memcmp(fluidBoundsActual, fluidBoundsExpected,
			sizeof(fluidBoundsExpected)) == 0;
	printf("simulation fluid public-api world-bounds matched=%u output=%08x.%08x.%08x.%08x.%08x.%08x\n",
		enabledFluidWorldBoundsMatch ? 1u : 0u,
		fluidBoundsActual[0], fluidBoundsActual[1], fluidBoundsActual[2],
		fluidBoundsActual[3], fluidBoundsActual[4], fluidBoundsActual[5]);
	unsigned fluidFlagsSaved = 0;
	if(enabledFluidInternalForBounds)
		{
		unsigned char* const fluidBytes = static_cast<unsigned char*>(
			enabledFluidInternalForBounds);
		unsigned* const fluidFlags = reinterpret_cast<unsigned*>(fluidBytes + 0x58);
		fluidFlagsSaved = *fluidFlags;
		*fluidFlags = NX_FF_MESH_INCLUDE_SEPARATED;
		}
	const NxU32 enabledFluidFlagInitial = enabledFluid
		? enabledFluid->getFlag(NX_FF_MESH_INCLUDE_SEPARATED) : 0;
	if(enabledFluid)
		enabledFluid->setFlag(NX_FF_MESH_INCLUDE_SEPARATED, false);
	const NxU32 enabledFluidFlagCleared = enabledFluid
		? enabledFluid->getFlag(NX_FF_MESH_INCLUDE_SEPARATED) : 0;
	if(enabledFluid)
		enabledFluid->setFlag(NX_FF_MESH_INCLUDE_SEPARATED, true);
	const NxU32 enabledFluidFlagSet = enabledFluid
		? enabledFluid->getFlag(NX_FF_MESH_INCLUDE_SEPARATED) : 0;
	if(enabledFluidInternalForBounds)
		*reinterpret_cast<unsigned*>(static_cast<unsigned char*>(
			enabledFluidInternalForBounds) + 0x58) = fluidFlagsSaved;
	const bool enabledFluidPublicFlagsMatch = enabledFluidInternalForBounds &&
		enabledFluidFlagInitial == NX_FF_MESH_INCLUDE_SEPARATED &&
		enabledFluidFlagCleared == 0 &&
		enabledFluidFlagSet == NX_FF_MESH_INCLUDE_SEPARATED;
	printf("simulation fluid public-api flags initial=%u cleared=%u set=%u\n",
		enabledFluidFlagInitial == NX_FF_MESH_INCLUDE_SEPARATED ? 1u : 0u,
		enabledFluidFlagCleared == 0 ? 1u : 0u,
		enabledFluidFlagSet == NX_FF_MESH_INCLUDE_SEPARATED ? 1u : 0u);
	unsigned char particleWriteDataSaved[44] = {};
	unsigned particleWriteDataInitial[11] = {
		0x10203040u, 0x11223344u, 0x22334455u, 0x33445566u,
		0x44556677u, 0x55667788u, 12u, 16u, 4u, 4u, 0x66778899u
	};
	NxParticleData particleWriteDataDesired;
	static NxU32 particleWriteDataCount = 7;
	static NxF32 particleWriteDataPositions[3] = { 1.25f, -2.5f, 3.75f };
	static NxF32 particleWriteDataVelocities[3] = { -4.5f, 5.25f, -6.75f };
	static NxF32 particleWriteDataLifetimes[1] = { 8.5f };
	static NxU32 particleWriteDataFlags[1] = { NX_FP_SEPARATED };
	static const char particleWriteDataName[] = "simulation-fluid-particle-write-data";
	particleWriteDataDesired.maxParticles = 0x76543210u;
	particleWriteDataDesired.numParticlesPtr = &particleWriteDataCount;
	particleWriteDataDesired.bufferPos = particleWriteDataPositions;
	particleWriteDataDesired.bufferVel = particleWriteDataVelocities;
	particleWriteDataDesired.bufferLife = particleWriteDataLifetimes;
	particleWriteDataDesired.bufferFlags = particleWriteDataFlags;
	particleWriteDataDesired.bufferPosByteStride = 12;
	particleWriteDataDesired.bufferVelByteStride = 16;
	particleWriteDataDesired.bufferLifeByteStride = 4;
	particleWriteDataDesired.bufferFlagsByteStride = 4;
	particleWriteDataDesired.name = particleWriteDataName;
	if(enabledFluidInternalForBounds)
		{
		unsigned char* const fluidBytes = static_cast<unsigned char*>(
			enabledFluidInternalForBounds);
		memcpy(particleWriteDataSaved, fluidBytes + 0x28,
			sizeof(particleWriteDataSaved));
		memcpy(fluidBytes + 0x28, particleWriteDataInitial,
			sizeof(particleWriteDataInitial));
		}
	NxParticleData particleWriteDataInitialRead;
	if(enabledFluid)
		particleWriteDataInitialRead = enabledFluid->getParticlesWriteData();
	const bool particleWriteDataInitialMatch = enabledFluidInternalForBounds &&
		memcmp(&particleWriteDataInitialRead, particleWriteDataInitial,
			sizeof(particleWriteDataInitial)) == 0;
	if(enabledFluid)
		enabledFluid->setParticlesWriteData(particleWriteDataDesired);
	const bool particleWriteDataStored = enabledFluidInternalForBounds &&
		memcmp(static_cast<unsigned char*>(enabledFluidInternalForBounds) + 0x28,
			&particleWriteDataDesired, sizeof(particleWriteDataDesired)) == 0;
	NxParticleData particleWriteDataRoundTrip;
	if(enabledFluid)
		particleWriteDataRoundTrip = enabledFluid->getParticlesWriteData();
	const bool particleWriteDataRoundTripMatch =
		memcmp(&particleWriteDataRoundTrip, &particleWriteDataDesired,
			sizeof(particleWriteDataDesired)) == 0;
	if(enabledFluidInternalForBounds)
		memcpy(static_cast<unsigned char*>(enabledFluidInternalForBounds) + 0x28,
			particleWriteDataSaved, sizeof(particleWriteDataSaved));
	const bool enabledFluidPublicParticleWriteDataMatch =
		particleWriteDataInitialMatch && particleWriteDataStored &&
		particleWriteDataRoundTripMatch;
	printf("simulation fluid public-api particle-write-data initial=%u stored=%u roundtrip=%u\n",
		particleWriteDataInitialMatch ? 1u : 0u,
		particleWriteDataStored ? 1u : 0u,
		particleWriteDataRoundTripMatch ? 1u : 0u);
	static const char enabledFluidName[] = "simulation-fluid-name";
	if(enabledFluid)
		enabledFluid->setName(enabledFluidName);
	const char* const enabledFluidNameRead = enabledFluid
		? enabledFluid->getName() : 0;
	const bool enabledFluidNameStored = enabledFluidNameRead &&
		strcmp(enabledFluidNameRead, enabledFluidName) == 0;
	if(enabledFluid)
		enabledFluid->setName(0);
	const bool enabledFluidNameCleared = enabledFluid &&
		enabledFluid->getName() == 0;
	printf("simulation fluid public-api name stored=%u cleared=%u\n",
		enabledFluidNameStored ? 1u : 0u,
		enabledFluidNameCleared ? 1u : 0u);
	const bool enabledEmitterAllocationFailure =
		emitterFailAllocator.mFailCount == 1;
	const NxU32 enabledPublicEmitterCount = enabledFluid
		? enabledFluid->getNbEmitters() : 0;
	NxFluidEmitter** const enabledPublicEmitterList = enabledFluid
		? enabledFluid->getEmitters() : 0;
	simulationOutput.resetLast();
	const unsigned enabledPublicStiffnessErrorsBefore = simulationOutput.errors;
	const NxReal enabledPublicStiffness = enabledFluid
		? enabledFluid->getStiffness() : 0.0f;
	const unsigned enabledPublicStiffnessBits =
		*reinterpret_cast<const unsigned*>(&enabledPublicStiffness);
	const unsigned enabledPublicStiffnessErrors =
		simulationOutput.errors - enabledPublicStiffnessErrorsBefore;
	const bool enabledPublicStiffnessDiagnosticExact =
		enabledPublicStiffnessErrors == 1 &&
		simulationOutput.code == NXE_DB_WARNING && simulationOutput.line == 109 &&
		strcmp(simulationOutput.file,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\NpFluid.cpp") == 0 &&
		strcmp(simulationOutput.message,
			"NxFluid::getStiffness(): Feature not available!") == 0;
	simulationOutput.resetLast();
	const unsigned enabledPublicViscosityErrorsBefore = simulationOutput.errors;
	const NxReal enabledPublicViscosity = enabledFluid
		? enabledFluid->getViscosity() : 0.0f;
	const unsigned enabledPublicViscosityBits =
		*reinterpret_cast<const unsigned*>(&enabledPublicViscosity);
	const unsigned enabledPublicViscosityErrors =
		simulationOutput.errors - enabledPublicViscosityErrorsBefore;
	const bool enabledPublicViscosityDiagnosticExact =
		enabledPublicViscosityErrors == 1 &&
		simulationOutput.code == NXE_DB_WARNING && simulationOutput.line == 128 &&
		strcmp(simulationOutput.file,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\NpFluid.cpp") == 0 &&
		strcmp(simulationOutput.message,
			"NxFluid::getViscosity(): Feature not available!") == 0;
	const unsigned enabledFluidDampingCallsBefore = getFluidDampingCalls();
	const NxReal enabledPublicDamping = enabledFluid
		? enabledFluid->getDamping() : -1.0f;
	const unsigned enabledPublicDampingBits =
		*reinterpret_cast<const unsigned*>(&enabledPublicDamping);
	const unsigned enabledFluidDampingCalls =
		getFluidDampingCalls() - enabledFluidDampingCallsBefore;
	const unsigned enabledFluidStaticRestitutionCallsBefore =
		getFluidStaticRestitutionCalls();
	const NxReal enabledPublicStaticRestitution = enabledFluid
		? enabledFluid->getStaticCollisionRestitution() : -1.0f;
	const unsigned enabledPublicStaticRestitutionBits =
		*reinterpret_cast<const unsigned*>(&enabledPublicStaticRestitution);
	const unsigned enabledFluidStaticRestitutionCalls =
		getFluidStaticRestitutionCalls() -
		enabledFluidStaticRestitutionCallsBefore;
	const unsigned enabledFluidStaticAdhesionCallsBefore =
		getFluidStaticAdhesionCalls();
	const NxReal enabledPublicStaticAdhesion = enabledFluid
		? enabledFluid->getStaticCollisionAdhesion() : -1.0f;
	const unsigned enabledPublicStaticAdhesionBits =
		*reinterpret_cast<const unsigned*>(&enabledPublicStaticAdhesion);
	const unsigned enabledFluidStaticAdhesionCalls =
		getFluidStaticAdhesionCalls() - enabledFluidStaticAdhesionCallsBefore;
	simulationOutput.resetLast();
	const unsigned enabledDynamicRestitutionErrorsBefore = simulationOutput.errors;
	const NxReal enabledPublicDynamicRestitution = enabledFluid
		? enabledFluid->getDynamicCollisionRestitution() : -1.0f;
	const unsigned enabledPublicDynamicRestitutionBits =
		*reinterpret_cast<const unsigned*>(&enabledPublicDynamicRestitution);
	const unsigned enabledDynamicRestitutionErrors =
		simulationOutput.errors - enabledDynamicRestitutionErrorsBefore;
	const bool enabledDynamicRestitutionDiagnosticExact =
		enabledDynamicRestitutionErrors == 1 &&
		simulationOutput.code == NXE_DB_WARNING && simulationOutput.line == 188 &&
		strcmp(simulationOutput.file,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\NpFluid.cpp") == 0 &&
		strcmp(simulationOutput.message,
			"NxFluid::getDynamicCollisionRestitution(): Feature not available!") == 0;
	simulationOutput.resetLast();
	const unsigned enabledDynamicAdhesionErrorsBefore = simulationOutput.errors;
	const NxReal enabledPublicDynamicAdhesion = enabledFluid
		? enabledFluid->getDynamicCollisionAdhesion() : -1.0f;
	const unsigned enabledPublicDynamicAdhesionBits =
		*reinterpret_cast<const unsigned*>(&enabledPublicDynamicAdhesion);
	const unsigned enabledDynamicAdhesionErrors =
		simulationOutput.errors - enabledDynamicAdhesionErrorsBefore;
	const bool enabledDynamicAdhesionDiagnosticExact =
		enabledDynamicAdhesionErrors == 1 &&
		simulationOutput.code == NXE_DB_WARNING && simulationOutput.line == 203 &&
		strcmp(simulationOutput.file,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\NpFluid.cpp") == 0 &&
		strcmp(simulationOutput.message,
			"NxFluid::getDynamicCollisionAdhesion(): Feature not available!") == 0;
	FluidManagerCallbackSummary fluidConfigCallbacksBefore = {};
	getFluidManagerCallbackSummary(&fluidConfigCallbacksBefore);
	const unsigned enabledPublicMaxParticles = enabledFluid
		? enabledFluid->getMaxParticles() : 0;
	simulationOutput.resetLast();
	const unsigned enabledKernelErrorsBefore = simulationOutput.errors;
	const NxReal enabledPublicKernelRadius = enabledFluid
		? enabledFluid->getKernelRadiusMultiplier() : 0.0f;
	const unsigned enabledKernelErrors = simulationOutput.errors -
		enabledKernelErrorsBefore;
	const bool enabledKernelDiagnosticExact = enabledKernelErrors == 1 &&
		simulationOutput.code == NXE_DB_WARNING && simulationOutput.line == 336 &&
		strcmp(simulationOutput.file,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\NpFluid.cpp") == 0 &&
		strcmp(simulationOutput.message,
			"NxFluid::getKernelRadiusMultiplier(): Feature not available!") == 0;
	const NxReal enabledPublicRestParticlesPerMeter = enabledFluid
		? enabledFluid->getRestParticlesPerMeter() : -1.0f;
	const NxReal enabledPublicRestDensity = enabledFluid
		? enabledFluid->getRestDensity() : -1.0f;
	const NxReal enabledPublicRestParticleDistance = enabledFluid
		? enabledFluid->getRestParticleDistance() : -1.0f;
	const NxReal enabledPublicParticleMass = enabledFluid
		? enabledFluid->getParticleMass() : -1.0f;
	const unsigned enabledPublicSimulationMethod = enabledFluid
		? enabledFluid->getSimulationMethod() : 0;
	simulationOutput.resetLast();
	const unsigned enabledCollisionMethodErrorsBefore = simulationOutput.errors;
	const unsigned enabledPublicCollisionMethod = enabledFluid
		? enabledFluid->getCollisionMethod() : 0;
	const unsigned enabledCollisionMethodErrors = simulationOutput.errors -
		enabledCollisionMethodErrorsBefore;
	const bool enabledCollisionMethodDiagnosticExact =
		enabledCollisionMethodErrors == 1 &&
		simulationOutput.code == NXE_DB_WARNING && simulationOutput.line == 374 &&
		strcmp(simulationOutput.file,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\NpFluid.cpp") == 0 &&
		strcmp(simulationOutput.message,
			"NxFluid::getCollisionMethod(): Feature not available!") == 0;
	FluidManagerCallbackSummary fluidConfigCallbacksAfter = {};
	getFluidManagerCallbackSummary(&fluidConfigCallbacksAfter);
	const bool enabledFluidConfigCallbacks =
		fluidConfigCallbacksAfter.fluidGetMaxParticles ==
			fluidConfigCallbacksBefore.fluidGetMaxParticles + 1 &&
		fluidConfigCallbacksAfter.fluidGetKernelRadiusMultiplier ==
			fluidConfigCallbacksBefore.fluidGetKernelRadiusMultiplier &&
		fluidConfigCallbacksAfter.fluidGetRestDensity ==
			fluidConfigCallbacksBefore.fluidGetRestDensity + 1 &&
		fluidConfigCallbacksAfter.fluidGetRestParticleDistance ==
			fluidConfigCallbacksBefore.fluidGetRestParticleDistance + 2 &&
		fluidConfigCallbacksAfter.fluidGetParticleMass ==
			fluidConfigCallbacksBefore.fluidGetParticleMass + 1 &&
		fluidConfigCallbacksAfter.fluidGetSimulationMethod ==
			fluidConfigCallbacksBefore.fluidGetSimulationMethod + 1;
	printf("simulation fluid public-api configuration max=%08x kernel=%08x.%u.%u rest-per-meter=%08x rest-density=%08x rest-distance=%08x particle-mass=%08x simulation=%08x collision=%08x.%u.%u callbacks=%u.%u.%u.%u.%u.%u.%u\n",
		enabledPublicMaxParticles, nxFloatBits(enabledPublicKernelRadius),
		enabledKernelErrors, enabledKernelDiagnosticExact ? 1u : 0u,
		nxFloatBits(enabledPublicRestParticlesPerMeter),
		nxFloatBits(enabledPublicRestDensity),
		nxFloatBits(enabledPublicRestParticleDistance),
		nxFloatBits(enabledPublicParticleMass), enabledPublicSimulationMethod,
		enabledPublicCollisionMethod, enabledCollisionMethodErrors,
		enabledCollisionMethodDiagnosticExact ? 1u : 0u,
		enabledFluidConfigCallbacks ? 1u : 0u,
		fluidConfigCallbacksAfter.fluidGetMaxParticles -
			fluidConfigCallbacksBefore.fluidGetMaxParticles,
		fluidConfigCallbacksAfter.fluidGetKernelRadiusMultiplier -
			fluidConfigCallbacksBefore.fluidGetKernelRadiusMultiplier,
		fluidConfigCallbacksAfter.fluidGetRestDensity -
			fluidConfigCallbacksBefore.fluidGetRestDensity,
		fluidConfigCallbacksAfter.fluidGetRestParticleDistance -
			fluidConfigCallbacksBefore.fluidGetRestParticleDistance,
		fluidConfigCallbacksAfter.fluidGetParticleMass -
			fluidConfigCallbacksBefore.fluidGetParticleMass,
		fluidConfigCallbacksAfter.fluidGetSimulationMethod -
			fluidConfigCallbacksBefore.fluidGetSimulationMethod);
	NxSimulationFailSizeAllocator fluidFailAllocator(*savedFluidAllocator, 0x84);
	*fluidFoundationAllocatorSlot = &fluidFailAllocator;
	simulationOutput.resetLast();
	const unsigned allocationFailureErrorsBefore = simulationOutput.errors;
	NxFluidDesc allocationFailureDesc;
	allocationFailureDesc.setToDefault();
	NxFluid* const allocationFailureFluid = enabledFluidScene->createFluid(
		allocationFailureDesc);
	*fluidFoundationAllocatorSlot = savedFluidAllocator;
	const bool enabledFluidAllocationFailure = !allocationFailureFluid &&
		fluidFailAllocator.mFailCount == 1 &&
		simulationOutput.errors == allocationFailureErrorsBefore &&
		simulationOutput.code == NXE_NO_ERROR;
	const NxU32 enabledFluidCount = enabledFluidScene->getNbFluids();
	NxFluid** const enabledFluidList = enabledFluidScene->getFluids();
	const bool enabledFluidListed = enabledFluidCount == 1 && enabledFluidList &&
		enabledFluidList[0] == enabledFluid;
	void** const enabledFluidPrimaryArray = *reinterpret_cast<void***>(
		enabledFluidManager + 4);
	void** const enabledFluidSecondaryArray = *reinterpret_cast<void***>(
		enabledFluidManager + 0x14);
	void* const enabledFluidInternal = enabledFluid ? *reinterpret_cast<void**>(
		reinterpret_cast<unsigned char*>(enabledFluid) + 0x14) : 0;
	const bool enabledFluidManagerArraysMatch = enabledFluidPrimaryArray &&
		enabledFluidSecondaryArray && enabledFluidPrimaryArray[0] == enabledFluidInternal &&
		enabledFluidSecondaryArray[0] == enabledFluid;
	unsigned* const enabledEmitterInternalArray = enabledFluidInternal
		? *reinterpret_cast<unsigned**>(static_cast<unsigned char*>(
			enabledFluidInternal) + 4) : 0;
	unsigned* const enabledEmitterInternalLast = enabledFluidInternal
		? *reinterpret_cast<unsigned**>(static_cast<unsigned char*>(
			enabledFluidInternal) + 8) : 0;
	unsigned* const enabledEmitterWrapperArray = enabledFluidInternal
		? *reinterpret_cast<unsigned**>(static_cast<unsigned char*>(
			enabledFluidInternal) + 0x14) : 0;
	unsigned* const enabledEmitterWrapperLast = enabledFluidInternal
		? *reinterpret_cast<unsigned**>(static_cast<unsigned char*>(
			enabledFluidInternal) + 0x18) : 0;
	const unsigned enabledEmitterCount = enabledEmitterInternalArray &&
		enabledEmitterInternalLast >= enabledEmitterInternalArray
		? static_cast<unsigned>(enabledEmitterInternalLast -
			enabledEmitterInternalArray) : 0;
	const unsigned enabledEmitterWrapperCount = enabledEmitterWrapperArray &&
		enabledEmitterWrapperLast >= enabledEmitterWrapperArray
		? static_cast<unsigned>(enabledEmitterWrapperLast -
			enabledEmitterWrapperArray) : 0;
	const bool enabledNullBackendEmitterRetained = enabledEmitterCount == 3 &&
		enabledEmitterWrapperCount == 3 && enabledEmitterInternalArray &&
		enabledEmitterWrapperArray && enabledEmitterInternalArray[1] &&
		enabledEmitterWrapperArray[1] &&
		*reinterpret_cast<unsigned*>(enabledEmitterInternalArray[1] + 8) == 0 &&
		*reinterpret_cast<unsigned*>(enabledEmitterInternalArray[1] + 0x0c) ==
			enabledEmitterWrapperArray[1] && enabledEmitterInternalArray[2] &&
		enabledEmitterWrapperArray[2] &&
		*reinterpret_cast<unsigned*>(enabledEmitterInternalArray[2] + 8) != 0 &&
		*reinterpret_cast<unsigned*>(enabledEmitterInternalArray[2] + 0x0c) ==
			enabledEmitterWrapperArray[2];
	const bool enabledEmitterFrameActorBound = fluidFrameActorInternal &&
		enabledEmitterInternalArray &&
		enabledEmitterInternalArray[2] &&
		*reinterpret_cast<void**>(enabledEmitterInternalArray[2] + 0x14) ==
			fluidFrameActorInternal;
	NxMat34 enabledEmitterFrameWorldPose;
	enabledEmitterFrameWorldPose.id();
	if(enabledEmitterWrapperArray && enabledEmitterWrapperCount >= 3)
		enabledEmitterFrameWorldPose = reinterpret_cast<NxFluidEmitter*>(
			enabledEmitterWrapperArray[2])->getGlobalPoseVal();
	const bool enabledEmitterFrameWorldPoseMatches =
		enabledEmitterFrameWorldPose.M(0, 0) == -1.0f &&
		enabledEmitterFrameWorldPose.M(0, 1) == 0.0f &&
		enabledEmitterFrameWorldPose.M(0, 2) == 0.0f &&
		enabledEmitterFrameWorldPose.M(1, 0) == 0.0f &&
		enabledEmitterFrameWorldPose.M(1, 1) == -1.0f &&
		enabledEmitterFrameWorldPose.M(1, 2) == 0.0f &&
		enabledEmitterFrameWorldPose.M(2, 0) == 0.0f &&
		enabledEmitterFrameWorldPose.M(2, 1) == 0.0f &&
		enabledEmitterFrameWorldPose.M(2, 2) == 1.0f &&
		enabledEmitterFrameWorldPose.t.x == 11.25f &&
		enabledEmitterFrameWorldPose.t.y == -2.5f &&
		enabledEmitterFrameWorldPose.t.z == 3.25f;
	const bool enabledPublicEmitterApiMatches = enabledPublicEmitterCount == 3 &&
		enabledPublicEmitterList == reinterpret_cast<NxFluidEmitter**>(
			enabledEmitterWrapperArray) && enabledPublicEmitterList &&
		enabledPublicEmitterList[0] == reinterpret_cast<NxFluidEmitter*>(
			enabledEmitterWrapperArray[0]) &&
		enabledPublicEmitterList[1] == reinterpret_cast<NxFluidEmitter*>(
			enabledEmitterWrapperArray[1]) &&
		enabledPublicEmitterList[2] == reinterpret_cast<NxFluidEmitter*>(
			enabledEmitterWrapperArray[2]);
	printf("simulation fluid public-api emitter-count=%u count-match=%u array-match=%u\n",
		enabledPublicEmitterCount,
		enabledPublicEmitterCount == 3 ? 1u : 0u,
		enabledPublicEmitterApiMatches ? 1u : 0u);
	// Exercise the public NxFluid emitter mutators against a live enabled
	// FluidModel. Releasing a non-tail entry must swap the last internal and
	// public handles into the same slot, then notify the backend exactly once.
	NxFluidEmitter* const initialEmitters[3] = {
		enabledPublicEmitterList && enabledPublicEmitterCount > 0
			? enabledPublicEmitterList[0] : 0,
		enabledPublicEmitterList && enabledPublicEmitterCount > 1
			? enabledPublicEmitterList[1] : 0,
		enabledPublicEmitterList && enabledPublicEmitterCount > 2
			? enabledPublicEmitterList[2] : 0
		};
	FluidManagerCallbackSummary emitterMutationBefore = {};
	static const char publicEmitterName[] = "simulation-public-emitter";
	static unsigned publicEmitterUserData = 0x6a71c0deu;
	NxFluidEmitterDesc publicEmitterDesc = enabledEmitterDesc;
	publicEmitterDesc.name = publicEmitterName;
	publicEmitterDesc.userData = &publicEmitterUserData;
	// Exercise phys_fn_003501's early return when the internal 0x78-byte
	// emitter record cannot be allocated. The public arrays and backend
	// callbacks must stay untouched, so the subsequent successful create still
	// runs against the original three-emitter state.
	unsigned initialEmitterInternalHandles[3] = {};
	if(enabledEmitterInternalArray && enabledEmitterCount == 3)
		memcpy(initialEmitterInternalHandles, enabledEmitterInternalArray,
			sizeof(initialEmitterInternalHandles));
	FluidManagerCallbackSummary emitterAllocationFailureBefore = {};
	getFluidManagerCallbackSummary(&emitterAllocationFailureBefore);
	NxSimulationFailSizeAllocator publicEmitterFailAllocator(
		*savedFluidAllocator, 0x78);
	*fluidFoundationAllocatorSlot = &publicEmitterFailAllocator;
	NxFluidEmitter* const failedPublicEmitter = enabledFluid
		? enabledFluid->createEmitter(publicEmitterDesc) : 0;
	*fluidFoundationAllocatorSlot = savedFluidAllocator;
	NxFluidEmitter** const emittersAfterAllocationFailure = enabledFluid
		? enabledFluid->getEmitters() : 0;
	const unsigned emitterCountAfterAllocationFailure = enabledFluid
		? enabledFluid->getNbEmitters() : 0;
	unsigned* const emitterInternalArrayAfterAllocationFailure =
		enabledFluidInternal ? *reinterpret_cast<unsigned**>(
			static_cast<unsigned char*>(enabledFluidInternal) + 4) : 0;
	unsigned* const emitterInternalLastAfterAllocationFailure =
		enabledFluidInternal ? *reinterpret_cast<unsigned**>(
			static_cast<unsigned char*>(enabledFluidInternal) + 8) : 0;
	const unsigned emitterInternalCountAfterAllocationFailure =
		emitterInternalArrayAfterAllocationFailure &&
		emitterInternalLastAfterAllocationFailure >=
			emitterInternalArrayAfterAllocationFailure
			? static_cast<unsigned>(emitterInternalLastAfterAllocationFailure -
				emitterInternalArrayAfterAllocationFailure) : 0;
	FluidManagerCallbackSummary emitterAllocationFailureAfter = {};
	getFluidManagerCallbackSummary(&emitterAllocationFailureAfter);
	const bool publicEmitterAllocationFailureMatch = !failedPublicEmitter &&
		publicEmitterFailAllocator.mFailCount == 1 &&
		emitterCountAfterAllocationFailure == 3 &&
		emittersAfterAllocationFailure &&
		emittersAfterAllocationFailure[0] == initialEmitters[0] &&
		emittersAfterAllocationFailure[1] == initialEmitters[1] &&
		emittersAfterAllocationFailure[2] == initialEmitters[2] &&
		emitterInternalArrayAfterAllocationFailure ==
			enabledEmitterInternalArray &&
		emitterInternalCountAfterAllocationFailure == 3 &&
		memcmp(emitterInternalArrayAfterAllocationFailure,
			initialEmitterInternalHandles,
			sizeof(initialEmitterInternalHandles)) == 0 &&
		emitterAllocationFailureAfter.fluidGetRestParticleDistance ==
			emitterAllocationFailureBefore.fluidGetRestParticleDistance &&
		emitterAllocationFailureAfter.emitterCreate ==
			emitterAllocationFailureBefore.emitterCreate;
	printf("simulation fluid public-api emitter-allocation-failure null=%u fail-count=%u count=%u array-match=%u callbacks-unchanged=%u\n",
		failedPublicEmitter == 0 ? 1u : 0u,
		publicEmitterFailAllocator.mFailCount,
		emitterCountAfterAllocationFailure,
		publicEmitterAllocationFailureMatch ? 1u : 0u,
		emitterAllocationFailureAfter.fluidGetRestParticleDistance ==
			emitterAllocationFailureBefore.fluidGetRestParticleDistance &&
			emitterAllocationFailureAfter.emitterCreate ==
				emitterAllocationFailureBefore.emitterCreate ? 1u : 0u);
	// phys_fn_003501 rejects a twenty-first emitter before allocation or backend
	// callbacks. Fill to the documented 0x50-byte maximum, probe the rejected
	// call with an allocator that would fail the emitter record, then release
	// the added tail in reverse order so the surrounding fixture is unchanged.
	NxFluidEmitter* capacityEmitters[17] = {};
	unsigned capacityEmittersCreated = 0;
	for(; enabledFluid && capacityEmittersCreated < 17;
		++capacityEmittersCreated)
		{
		capacityEmitters[capacityEmittersCreated] =
			enabledFluid->createEmitter(publicEmitterDesc);
		if(!capacityEmitters[capacityEmittersCreated])
			break;
		}
	unsigned capacityEmitterBackendHandles = 0;
	for(unsigned i = 0; i < capacityEmittersCreated; ++i)
		{
	unsigned char* const emitterBytes = capacityEmitters[i]
			? static_cast<unsigned char*>(*reinterpret_cast<void**>(
				reinterpret_cast<unsigned char*>(capacityEmitters[i]) + 0x14)) : 0;
	if(emitterBytes && *reinterpret_cast<unsigned*>(emitterBytes + 8))
		++capacityEmitterBackendHandles;
		}
	const unsigned emitterCountAtCapacity = enabledFluid
		? enabledFluid->getNbEmitters() : 0;
	FluidManagerCallbackSummary emitterCapacityBefore = {};
	getFluidManagerCallbackSummary(&emitterCapacityBefore);
	NxSimulationFailSizeAllocator emitterCapacityFailAllocator(
		*savedFluidAllocator, 0x78);
	*fluidFoundationAllocatorSlot = &emitterCapacityFailAllocator;
	NxFluidEmitter* const overCapacityEmitter = enabledFluid
		? enabledFluid->createEmitter(publicEmitterDesc) : 0;
	*fluidFoundationAllocatorSlot = savedFluidAllocator;
	FluidManagerCallbackSummary emitterCapacityAfter = {};
	getFluidManagerCallbackSummary(&emitterCapacityAfter);
	NxFluidEmitter** const emitterArrayAtCapacity = enabledFluid
		? enabledFluid->getEmitters() : 0;
	bool emitterArrayAtCapacityMatches = capacityEmittersCreated == 17 &&
		emitterCountAtCapacity == 20 && emitterArrayAtCapacity;
	if(emitterArrayAtCapacityMatches)
		{
		for(unsigned i = 0; i < 3; ++i)
			if(emitterArrayAtCapacity[i] != initialEmitters[i])
				emitterArrayAtCapacityMatches = false;
		for(unsigned i = 0; i < 17; ++i)
			if(emitterArrayAtCapacity[i + 3] != capacityEmitters[i])
				emitterArrayAtCapacityMatches = false;
		}
	const bool publicEmitterCapacityMatch = emitterArrayAtCapacityMatches &&
		!overCapacityEmitter && emitterCapacityFailAllocator.mFailCount == 0 &&
		emitterCapacityAfter.fluidGetRestParticleDistance ==
			emitterCapacityBefore.fluidGetRestParticleDistance &&
		emitterCapacityAfter.emitterCreate == emitterCapacityBefore.emitterCreate;
	printf("simulation fluid public-api emitter-capacity created=%u count=%u rejected=%u allocation-attempts=%u array-match=%u callbacks-unchanged=%u\n",
		capacityEmittersCreated, emitterCountAtCapacity,
		overCapacityEmitter == 0 ? 1u : 0u,
		emitterCapacityFailAllocator.mFailCount,
		emitterArrayAtCapacityMatches ? 1u : 0u,
		emitterCapacityAfter.fluidGetRestParticleDistance ==
			emitterCapacityBefore.fluidGetRestParticleDistance &&
			emitterCapacityAfter.emitterCreate ==
				emitterCapacityBefore.emitterCreate ? 1u : 0u);
	if(overCapacityEmitter)
		enabledFluid->releaseEmitter(*overCapacityEmitter);
	for(unsigned i = capacityEmittersCreated; i > 0; --i)
		if(capacityEmitters[i - 1])
			enabledFluid->releaseEmitter(*capacityEmitters[i - 1]);
	NxFluidEmitter** const emittersAfterCapacityProbe = enabledFluid
		? enabledFluid->getEmitters() : 0;
	const bool emitterCapacityRestored = enabledFluid &&
		enabledFluid->getNbEmitters() == 3 && emittersAfterCapacityProbe &&
		emittersAfterCapacityProbe[0] == initialEmitters[0] &&
		emittersAfterCapacityProbe[1] == initialEmitters[1] &&
		emittersAfterCapacityProbe[2] == initialEmitters[2];
	const bool publicEmitterCapacityExact = publicEmitterCapacityMatch &&
		emitterCapacityRestored;
	printf("simulation fluid public-api emitter-capacity restored=%u exact=%u\n",
		emitterCapacityRestored ? 1u : 0u,
		publicEmitterCapacityExact ? 1u : 0u);
	getFluidManagerCallbackSummary(&emitterMutationBefore);
	NxFluidEmitter* const publiclyCreatedEmitter = enabledFluid
		? enabledFluid->createEmitter(publicEmitterDesc) : 0;
	NxFluidEmitter** const emittersAfterCreate = enabledFluid
		? enabledFluid->getEmitters() : 0;
	const unsigned emitterCountAfterCreate = enabledFluid
		? enabledFluid->getNbEmitters() : 0;
	const char* const publiclyCreatedEmitterName = publiclyCreatedEmitter
		? publiclyCreatedEmitter->getName() : 0;
	const bool publicEmitterMetadataMatch = publiclyCreatedEmitter &&
		publiclyCreatedEmitter->userData == &publicEmitterUserData &&
		publiclyCreatedEmitterName &&
		strcmp(publiclyCreatedEmitterName, publicEmitterName) == 0;
	const bool publicEmitterCreateMatch = publiclyCreatedEmitter &&
		publicEmitterMetadataMatch &&
		emitterCountAfterCreate == 4 && emittersAfterCreate &&
		emittersAfterCreate[0] == initialEmitters[0] &&
		emittersAfterCreate[1] == initialEmitters[1] &&
		emittersAfterCreate[2] == initialEmitters[2] &&
		emittersAfterCreate[3] == publiclyCreatedEmitter;
	if(enabledFluid && initialEmitters[0])
		enabledFluid->releaseEmitter(*initialEmitters[0]);
	NxFluidEmitter** const emittersAfterRelease = enabledFluid
		? enabledFluid->getEmitters() : 0;
	const unsigned emitterCountAfterRelease = enabledFluid
		? enabledFluid->getNbEmitters() : 0;
	void* const fluidBytesForEmitterMutation = enabledFluidInternal
		? enabledFluidInternal : 0;
	unsigned* const internalEmittersAfterRelease = fluidBytesForEmitterMutation
		? *reinterpret_cast<unsigned**>(static_cast<unsigned char*>(
			fluidBytesForEmitterMutation) + 4) : 0;
	const bool publicEmitterReleaseMatch = publicEmitterCreateMatch &&
		emitterCountAfterRelease == 3 && emittersAfterRelease &&
		emittersAfterRelease[0] == publiclyCreatedEmitter &&
		emittersAfterRelease[1] == initialEmitters[1] &&
		emittersAfterRelease[2] == initialEmitters[2] &&
		internalEmittersAfterRelease &&
		*reinterpret_cast<void**>(internalEmittersAfterRelease) ==
			*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(
				publiclyCreatedEmitter) + 0x14) &&
		*reinterpret_cast<void**>(internalEmittersAfterRelease + 1) ==
			*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(
				initialEmitters[1]) + 0x14) &&
		*reinterpret_cast<void**>(internalEmittersAfterRelease + 2) ==
			*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(
				initialEmitters[2]) + 0x14);
	FluidManagerCallbackSummary emitterMutationAfter = {};
	getFluidManagerCallbackSummary(&emitterMutationAfter);
	const bool publicEmitterCallbackMatch =
		emitterMutationAfter.emitterCreate ==
			emitterMutationBefore.emitterCreate + 1 &&
		emitterMutationAfter.emitterDestroy ==
			emitterMutationBefore.emitterDestroy + 1;
	printf("simulation fluid public-api emitter-create-release created=%u.%u.%u.%u metadata=%u released=%u.%u.%u.%u callback=%u mismatches=%u\n",
		publicEmitterCreateMatch ? 1u : 0u,
		emitterCountAfterCreate, emittersAfterCreate ? 1u : 0u,
		publiclyCreatedEmitter ? 1u : 0u,
		publicEmitterMetadataMatch ? 1u : 0u,
		publicEmitterReleaseMatch ? 1u : 0u,
		emitterCountAfterRelease, internalEmittersAfterRelease ? 1u : 0u,
		emittersAfterRelease ? 1u : 0u,
		publicEmitterCallbackMatch ? 1u : 0u,
		publicEmitterCreateMatch && publicEmitterReleaseMatch &&
			publicEmitterCallbackMatch ? 0u : 1u);
	printf("simulation fluid public-api scalars stiffness=%08x.%u viscosity=%08x.%u diagnostics=%u.%u\n",
		enabledPublicStiffnessBits, enabledPublicStiffnessErrors,
		enabledPublicViscosityBits, enabledPublicViscosityErrors,
		enabledPublicStiffnessDiagnosticExact ? 1u : 0u,
		enabledPublicViscosityDiagnosticExact ? 1u : 0u);
	printf("simulation fluid public-api damping value=%08x calls=%u\n",
		enabledPublicDampingBits, enabledFluidDampingCalls);
	printf("simulation fluid public-api static-collision restitution=%08x calls=%u adhesion=%08x calls=%u\n",
		enabledPublicStaticRestitutionBits, enabledFluidStaticRestitutionCalls,
		enabledPublicStaticAdhesionBits, enabledFluidStaticAdhesionCalls);
	printf("simulation fluid public-api dynamic-collision restitution=%08x.%u.%u adhesion=%08x.%u.%u\n",
		enabledPublicDynamicRestitutionBits, enabledDynamicRestitutionErrors,
		enabledDynamicRestitutionDiagnosticExact ? 1u : 0u,
		enabledPublicDynamicAdhesionBits, enabledDynamicAdhesionErrors,
		enabledDynamicAdhesionDiagnosticExact ? 1u : 0u);
	const bool enabledFluidDirtyFlagsCleared = enabledFluidManager[0x28] == 0 &&
		enabledFluidManager[0x29] == 0;
	const bool enabledFluidCellSizeStored =
		*reinterpret_cast<unsigned*>(enabledFluidManager + 0x2c) == 0x3f000000u;
	// With the optional manager enabled, adding/removing a static shape sets
	// the shape-change dirty byte; dynamic shapes leave it clear. These public
	// actor operations enter phys_fn_003628 through Scene's shape lifecycle.
	NxPlaneShapeDesc fluidDirtyStaticPlane;
	NxActorDesc fluidDirtyStaticActorDesc;
	fluidDirtyStaticActorDesc.shapes.pushBack(&fluidDirtyStaticPlane);
	NxActor* const fluidDirtyStaticActor = enabledFluidScene->createActor(
		fluidDirtyStaticActorDesc);
	const bool fluidStaticAddDirty = fluidDirtyStaticActor &&
		enabledFluidManager[0x28] == 1;
	enabledFluidManager[0x28] = 0;
	NxSphereShapeDesc fluidDirtyDynamicSphere;
	fluidDirtyDynamicSphere.radius = 0.25f;
	NxBodyDesc fluidDirtyDynamicBody;
	NxActorDesc fluidDirtyDynamicActorDesc;
	fluidDirtyDynamicActorDesc.body = &fluidDirtyDynamicBody;
	fluidDirtyDynamicActorDesc.density = 1.0f;
	fluidDirtyDynamicActorDesc.shapes.pushBack(&fluidDirtyDynamicSphere);
	NxActor* const fluidDirtyDynamicActor = enabledFluidScene->createActor(
		fluidDirtyDynamicActorDesc);
	const bool fluidDynamicAddClean = fluidDirtyDynamicActor &&
		enabledFluidManager[0x28] == 0;
	if(fluidDirtyDynamicActor)
		enabledFluidScene->releaseActor(*fluidDirtyDynamicActor);
	const bool fluidDynamicRemoveClean = enabledFluidManager[0x28] == 0;
	if(fluidDirtyStaticActor)
		enabledFluidScene->releaseActor(*fluidDirtyStaticActor);
	const bool fluidStaticRemoveDirty = enabledFluidManager[0x28] == 1;
	printf("simulation fluid shape-change static-add-dirty=%u dynamic-add-clean=%u dynamic-remove-clean=%u static-remove-dirty=%u\n",
		fluidStaticAddDirty ? 1u : 0u, fluidDynamicAddClean ? 1u : 0u,
		fluidDynamicRemoveClean ? 1u : 0u, fluidStaticRemoveDirty ? 1u : 0u);
	if(enabledFluid)
		enabledFluidScene->releaseFluid(*enabledFluid);
	FluidManagerCallbackSummary fluidManagerCallbacks = {};
	getFluidManagerCallbackSummary(&fluidManagerCallbacks);
	const bool enabledFluidManagerCleared =
		*reinterpret_cast<void**>(enabledFluidSceneInternal + 0x61c) == 0;
	const unsigned enabledFluidErrors = simulationOutput.errors -
		enabledFluidErrorsBefore - enabledPublicStiffnessErrors -
		enabledPublicViscosityErrors - enabledDynamicRestitutionErrors -
		enabledDynamicAdhesionErrors - enabledKernelErrors -
		enabledCollisionMethodErrors;
	const bool enabledFluidCallbacks = fluidManagerCallbacks.managerCreate == 1 &&
		fluidManagerCallbacks.managerDestroy == 1 &&
		fluidManagerCallbacks.fluidCreate == 1 &&
		fluidManagerCallbacks.fluidDestroy == 1;
	printf("simulation fluid-model-load row=003900 module=%u callbacks=%u\n",
		fluidManagerCallbacks.managerCreate == 1 ? 1u : 0u,
		enabledFluidCallbacks ? 1u : 0u);
	const bool enabledFluidRefreshCallbacks =
		fluidManagerCallbacks.staticCollisionCreate == 1 &&
		fluidManagerCallbacks.staticCollisionGet == 1 &&
		fluidManagerCallbacks.fluidGetCellSize == 1 &&
		fluidManagerCallbacks.staticCollisionDestroy == 0;
	const bool enabledFluidEmitterCallbacks =
		fluidManagerCallbacks.fluidGetRestParticleDistance ==
			6 + capacityEmittersCreated &&
		fluidManagerCallbacks.emitterCreate == 4 + capacityEmittersCreated &&
		fluidManagerCallbacks.emitterDestroy ==
			1 + capacityEmitterBackendHandles &&
		fluidManagerCallbacks.emitterSetGlobalPose == 4 + capacityEmittersCreated &&
		fluidManagerCallbacks.emitterGetGlobalPose == 4 + capacityEmittersCreated;
	const bool row645FluidCreateExact = fluidDisabledCreateExact && enabledFluid &&
		enabledFluidListed && enabledFluidManagerArraysMatch &&
		enabledFluidCallbacks && enabledFluidRefreshCallbacks &&
			enabledFluidEmitterCallbacks && enabledDescriptorValidationPassed == 48 &&
		managerAllocationFailure && enabledFluidAllocationFailure &&
		enabledEmitterAllocationFailure && publicEmitterAllocationFailureMatch &&
		publicEmitterCapacityExact &&
		enabledFluidErrors == 0;
	printf("simulation row645 fluid-create disabled=%u enabled=%u listed=%u callbacks=%u validation=%u allocation-failures=%u exact=%u\n",
		fluidDisabledCreateExact ? 1u : 0u, enabledFluid ? 1u : 0u,
		enabledFluidListed ? 1u : 0u,
		enabledFluidCallbacks && enabledFluidRefreshCallbacks &&
			enabledFluidEmitterCallbacks ? 1u : 0u,
		enabledDescriptorValidationPassed,
		managerAllocationFailure && enabledFluidAllocationFailure &&
			enabledEmitterAllocationFailure ? 1u : 0u,
		row645FluidCreateExact ? 1u : 0u);
	printf("simulation fluid enabled valid=%u invalid=%u.%d.%d descriptor-validation=%u.%u manager-allocation-failure=%u.%u allocation-failure=%u.%u emitter-allocation-failure=%u.%u frame-actor=%u frame-pose=%u manager-flags=%u listed=%u count=%u arrays-match=%u manager-cleared=%u callbacks=%u.%u.%u.%u refresh=%u.%u.%u.%u distance=%u emitter=%u.%u.%u.%u emitter-arrays=%u.%u growth=%u null-backend-retained=%u dirty-cleared=%u cell-size=%u errors=%u\n",
		enabledFluid != 0, enabledInvalidRejected ? 1u : 0u,
		static_cast<unsigned>(enabledInvalidCode), enabledInvalidLine,
		enabledDescriptorValidationPassed,
		enabledDescriptorValidationDiagnosticsExact ? 1u : 0u,
		managerAllocationFailure ? 1u : 0u, managerFailAllocator.mFailCount,
		enabledFluidAllocationFailure ? 1u : 0u, fluidFailAllocator.mFailCount,
		enabledEmitterAllocationFailure ? 1u : 0u,
		emitterFailAllocator.mFailCount,
		enabledEmitterFrameActorBound ? 1u : 0u,
		enabledEmitterFrameWorldPoseMatches ? 1u : 0u,
		enabledFluidManagerFlags ? 1u : 0u, enabledFluidListed ? 1u : 0u,
		enabledFluidCount, enabledFluidManagerArraysMatch ? 1u : 0u,
		enabledFluidManagerCleared ? 1u : 0u,
		fluidManagerCallbacks.managerCreate, fluidManagerCallbacks.fluidCreate,
		fluidManagerCallbacks.fluidDestroy, fluidManagerCallbacks.managerDestroy,
		fluidManagerCallbacks.fluidGetCellSize,
		fluidManagerCallbacks.staticCollisionGet,
		fluidManagerCallbacks.staticCollisionDestroy,
		fluidManagerCallbacks.staticCollisionCreate,
		fluidManagerCallbacks.fluidGetRestParticleDistance,
		fluidManagerCallbacks.emitterCreate,
		fluidManagerCallbacks.emitterDestroy,
		fluidManagerCallbacks.emitterSetGlobalPose,
		fluidManagerCallbacks.emitterGetGlobalPose,
		enabledEmitterCount, enabledEmitterWrapperCount,
		enabledEmitterCount >= 3 && enabledEmitterWrapperCount >= 3 ? 1u : 0u,
		enabledNullBackendEmitterRetained ? 1u : 0u,
		enabledFluidDirtyFlagsCleared ? 1u : 0u,
		enabledFluidCellSizeStored ? 1u : 0u,
		enabledFluidErrors);
	if(!row645FluidCreateExact || !enabledInvalidRejected || enabledDescriptorValidationPassed != 48 ||
		!enabledDescriptorValidationDiagnosticsExact ||
		!managerAllocationFailure ||
		!enabledFluidAllocationFailure ||
		!enabledEmitterAllocationFailure ||
		!enabledEmitterFrameActorBound ||
		!enabledEmitterFrameWorldPoseMatches ||
		!enabledFluidManagerFlags || !enabledFluid ||
		enabledInvalidCode != NXE_INVALID_PARAMETER || enabledInvalidLine != 0x8c ||
		!enabledFluidListed || !enabledFluidManagerArraysMatch ||
		!enabledFluidManagerCleared || !enabledFluidCallbacks ||
		!enabledFluidRefreshCallbacks || !enabledFluidDirtyFlagsCleared ||
		!enabledFluidEmitterCallbacks || !publicEmitterCreateMatch ||
		!publicEmitterReleaseMatch || !publicEmitterCallbackMatch ||
		!publicEmitterAllocationFailureMatch ||
		!publicEmitterCapacityExact ||
		!enabledNullBackendEmitterRetained ||
		!enabledFluidWorldBoundsMatch ||
		!enabledFluidPublicFlagsMatch ||
		!enabledFluidPublicParticleWriteDataMatch ||
		!enabledFluidNameStored || !enabledFluidNameCleared ||
		!enabledFluidCellSizeStored || !fluidStaticAddDirty ||
		!enabledFluidConfigCallbacks || enabledPublicMaxParticles != 0x1234u ||
		nxFloatBits(enabledPublicRestParticlesPerMeter) != 0x42480000u ||
		nxFloatBits(enabledPublicRestDensity) != 0x447a0000u ||
		nxFloatBits(enabledPublicRestParticleDistance) != nxFloatBits(0.02f) ||
		nxFloatBits(enabledPublicParticleMass) != 0x3f2aaaabu ||
		enabledPublicSimulationMethod != 2 ||
		!enabledKernelDiagnosticExact ||
		!enabledCollisionMethodDiagnosticExact ||
		enabledPublicDampingBits != 0x3ec00000u ||
		enabledFluidDampingCalls != 1 ||
		enabledPublicStaticRestitutionBits != 0x3e000000u ||
		enabledFluidStaticRestitutionCalls != 1 ||
		enabledPublicStaticAdhesionBits != 0x3f200000u ||
		enabledFluidStaticAdhesionCalls != 1 ||
		enabledPublicDynamicRestitutionBits != 0 ||
		!enabledDynamicRestitutionDiagnosticExact ||
		enabledPublicDynamicAdhesionBits != 0 ||
		!enabledDynamicAdhesionDiagnosticExact ||
		!fluidDynamicAddClean || !fluidDynamicRemoveClean ||
		!fluidStaticRemoveDirty ||
		enabledFluidErrors != 0)
		return nxFail("enabled fluid creation lifecycle did not match the fixture contract");
	bool unlicensedSurfaceMeshGetterExact = false;
	bool unlicensedSurfaceMeshReleaseExact = false;
	bool unlicensedSurfaceMeshCreateWarningExact = false;
	bool partialSurfaceMeshCreateFlagsMatch = false;
	{
	NxSimulationFluidLicenseOverride partialSurfaceMeshLicenseOverride(465);
	NxScene* const partialSurfaceMeshScene = partialSurfaceMeshLicenseOverride.ready
		? sdk->createScene(fluidSceneDesc) : 0;
	NxFluidDesc partialSurfaceMeshFluidDesc;
	partialSurfaceMeshFluidDesc.setToDefault();
	NxImplicitMeshDesc unlicensedSurfaceMeshDesc;
	unlicensedSurfaceMeshDesc.setToDefault();
	partialSurfaceMeshFluidDesc.surfaceMeshDesc = &unlicensedSurfaceMeshDesc;
	simulationOutput.resetLast();
	const unsigned unlicensedSurfaceMeshCreateErrorsBefore =
		simulationOutput.errors;
	NxFluid* const partialSurfaceMeshFluid = partialSurfaceMeshScene
		? partialSurfaceMeshScene->createFluid(partialSurfaceMeshFluidDesc) : 0;
	unlicensedSurfaceMeshCreateWarningExact =
		partialSurfaceMeshLicenseOverride.ready && partialSurfaceMeshFluid &&
		simulationOutput.errors == unlicensedSurfaceMeshCreateErrorsBefore + 1 &&
		simulationOutput.code == NXE_DB_WARNING && simulationOutput.line == 280 &&
		strstr(simulationOutput.message,
			"NxFluid::createSurfaceMesh: Feature not available!") != 0;
	FluidManagerCallbackSummary partialSurfaceMeshCreateCallbacks = {};
	getFluidManagerCallbackSummary(&partialSurfaceMeshCreateCallbacks);
	partialSurfaceMeshCreateFlagsMatch = partialSurfaceMeshFluid &&
		partialSurfaceMeshLicenseOverride.ready &&
		unlicensedSurfaceMeshCreateWarningExact &&
		partialSurfaceMeshCreateCallbacks.fluidCreateArguments[13] == 1 &&
		partialSurfaceMeshCreateCallbacks.fluidCreateArguments[14] == 0;
	printf("simulation fluid create license=465 sparse=%u hardware=%u matched=%u\n",
		partialSurfaceMeshCreateCallbacks.fluidCreateArguments[13],
		partialSurfaceMeshCreateCallbacks.fluidCreateArguments[14],
		partialSurfaceMeshCreateFlagsMatch ? 1u : 0u);
	simulationOutput.resetLast();
	const unsigned partialGetterErrorsBefore = simulationOutput.errors;
	const NxImplicitMesh* const partialSurfaceMesh = partialSurfaceMeshFluid
		? partialSurfaceMeshFluid->getSurfaceMesh() : 0;
	const unsigned partialGetterErrors =
		simulationOutput.errors - partialGetterErrorsBefore;
	unlicensedSurfaceMeshGetterExact = partialSurfaceMeshLicenseOverride.ready &&
		partialSurfaceMeshFluid && partialSurfaceMesh == 0 &&
		partialGetterErrors == 1 && simulationOutput.code == NXE_DB_WARNING &&
		simulationOutput.line == 76 && strcmp(simulationOutput.file,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\NpFluid.cpp") == 0 &&
		strcmp(simulationOutput.message,
			"NxFluid::getSurfaceMesh(): Feature not available!") == 0;
	const unsigned partialReleaseErrorsBefore = simulationOutput.errors;
	if(partialSurfaceMeshFluid)
		partialSurfaceMeshFluid->releaseSurfaceMesh();
	unlicensedSurfaceMeshReleaseExact = partialSurfaceMeshFluid &&
		simulationOutput.errors - partialReleaseErrorsBefore == 1 &&
		simulationOutput.code == NXE_DB_WARNING && simulationOutput.line == 66 &&
		strcmp(simulationOutput.file,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\NpFluid.cpp") == 0 &&
		strcmp(simulationOutput.message,
			"NxFluid::releaseSurfaceMesh(): Feature not available!") == 0;
	if(partialSurfaceMeshFluid)
		partialSurfaceMeshScene->releaseFluid(*partialSurfaceMeshFluid);
	if(partialSurfaceMeshScene)
		sdk->releaseScene(*partialSurfaceMeshScene);
	}
	bool surfaceMeshCreateFlagsMatch = false;
	{
	NxSimulationFluidLicenseOverride surfaceMeshLicenseOverride(804);
	NxScene* const surfaceMeshScene = surfaceMeshLicenseOverride.ready
		? sdk->createScene(fluidSceneDesc) : 0;
	NxFluidDesc surfaceMeshFluidDesc;
	surfaceMeshFluidDesc.setToDefault();
	NxFluid* const surfaceMeshFluid = surfaceMeshScene
		? surfaceMeshScene->createFluid(surfaceMeshFluidDesc) : 0;
	FluidManagerCallbackSummary surfaceMeshCreateCallbacks = {};
	getFluidManagerCallbackSummary(&surfaceMeshCreateCallbacks);
	surfaceMeshCreateFlagsMatch = surfaceMeshFluid &&
		surfaceMeshLicenseOverride.ready &&
		surfaceMeshCreateCallbacks.fluidCreateArguments[13] == 1 &&
		surfaceMeshCreateCallbacks.fluidCreateArguments[14] == 1;
	printf("simulation fluid create license=804 sparse=%u hardware=%u matched=%u\n",
		surfaceMeshCreateCallbacks.fluidCreateArguments[13],
		surfaceMeshCreateCallbacks.fluidCreateArguments[14],
		surfaceMeshCreateFlagsMatch ? 1u : 0u);
	unsigned char surfaceMeshObject[0x3c] = {};
	static unsigned char returnedSurfaceMeshMarker;
	*reinterpret_cast<void***>(surfaceMeshObject) = gSimulationSurfaceMeshVtable;
	*reinterpret_cast<void**>(surfaceMeshObject + 0x38) =
		&returnedSurfaceMeshMarker;
	unsigned char* const surfaceMeshInternal = surfaceMeshFluid
		? *reinterpret_cast<unsigned char**>(reinterpret_cast<unsigned char*>(
			surfaceMeshFluid) + 0x14) : 0;
	void* savedSurfaceMesh = 0;
	if(surfaceMeshInternal)
		{
		savedSurfaceMesh = *reinterpret_cast<void**>(surfaceMeshInternal + 0x24);
		*reinterpret_cast<void**>(surfaceMeshInternal + 0x24) = surfaceMeshObject;
		}
	gSimulationSurfaceMeshDeletingCalls = 0;
	gSimulationSurfaceMeshDeleteFlags = 0;
	simulationOutput.resetLast();
	const unsigned surfaceMeshErrorsBefore = simulationOutput.errors;
	NxImplicitMesh* const returnedSurfaceMesh = surfaceMeshFluid
		? surfaceMeshFluid->getSurfaceMesh() : 0;
	if(surfaceMeshFluid)
		surfaceMeshFluid->releaseSurfaceMesh();
	const unsigned surfaceMeshErrors =
		simulationOutput.errors - surfaceMeshErrorsBefore;
	const bool surfaceMeshReleased = surfaceMeshInternal &&
		*reinterpret_cast<void**>(surfaceMeshInternal + 0x24) == 0;
	if(surfaceMeshInternal)
		*reinterpret_cast<void**>(surfaceMeshInternal + 0x24) = savedSurfaceMesh;
	const bool surfaceMeshPublicApiMatches = surfaceMeshLicenseOverride.ready &&
		surfaceMeshFluid &&
		returnedSurfaceMesh == reinterpret_cast<NxImplicitMesh*>(
			&returnedSurfaceMeshMarker) && surfaceMeshReleased &&
		gSimulationSurfaceMeshDeletingCalls == 1 &&
		gSimulationSurfaceMeshDeleteFlags == 1 && surfaceMeshErrors == 0 &&
		unlicensedSurfaceMeshGetterExact && unlicensedSurfaceMeshReleaseExact &&
		partialSurfaceMeshCreateFlagsMatch && surfaceMeshCreateFlagsMatch;
	NxImplicitMeshDesc licensedSurfaceMeshDesc;
	licensedSurfaceMeshDesc.setToDefault();
	licensedSurfaceMeshDesc.surfaceRadius = 0.04f;
	licensedSurfaceMeshDesc.singleRadius = 0.01f;
	licensedSurfaceMeshDesc.blend = 0.02f;
	licensedSurfaceMeshDesc.triangleSize = 0.005f;
	licensedSurfaceMeshDesc.smoothingIterations = 6;
	static const char licensedSurfaceMeshInitialName[] = "licensed-surface-mesh";
	licensedSurfaceMeshDesc.name = licensedSurfaceMeshInitialName;
	static unsigned char licensedSurfaceMeshUserData;
	licensedSurfaceMeshDesc.userData = &licensedSurfaceMeshUserData;
	NxFluidDesc licensedSurfaceMeshFluidDesc;
	licensedSurfaceMeshFluidDesc.setToDefault();
	licensedSurfaceMeshFluidDesc.surfaceMeshDesc = &licensedSurfaceMeshDesc;
	NxFluid* const licensedSurfaceMeshFluid = surfaceMeshScene
		? surfaceMeshScene->createFluid(licensedSurfaceMeshFluidDesc) : 0;
	NxImplicitMesh* const constructedSurfaceMesh = licensedSurfaceMeshFluid
		? licensedSurfaceMeshFluid->getSurfaceMesh() : 0;
	const bool constructedSurfaceMeshUserDataMatches = constructedSurfaceMesh &&
		constructedSurfaceMesh->userData == &licensedSurfaceMeshUserData;
	FluidManagerCallbackSummary licensedSurfaceMeshCallbacks = {};
	getFluidManagerCallbackSummary(&licensedSurfaceMeshCallbacks);
	const bool licensedSurfaceMeshConstructorMatches =
		licensedSurfaceMeshFluid && constructedSurfaceMesh &&
		constructedSurfaceMeshUserDataMatches &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshCreate == 1 &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshCreateArguments[0] != 0 &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshCreateArguments[1] != 0 &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshCreateArguments[2] == 0x3c23d70au &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshCreateArguments[3] == 0x3d23d70au &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshCreateArguments[4] == 0x3ca3d70au &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshCreateArguments[5] == 0x3ba3d70au &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshCreateArguments[6] == 6;
	resetFluidManagerCallbacks();
	NxImplicitMesh* const explicitSurfaceMesh = licensedSurfaceMeshFluid
		? licensedSurfaceMeshFluid->createSurfaceMesh(licensedSurfaceMeshDesc) : 0;
	const bool explicitSurfaceMeshUserDataMatches = explicitSurfaceMesh &&
		explicitSurfaceMesh->userData == &licensedSurfaceMeshUserData;
	getFluidManagerCallbackSummary(&licensedSurfaceMeshCallbacks);
	bool licensedSurfaceMeshPropertiesMatch = false;
	if(explicitSurfaceMesh)
		{
	const bool initialNameMatches = explicitSurfaceMesh->getName() ==
		licensedSurfaceMeshInitialName;
	const NxReal initialSurfaceRadius = explicitSurfaceMesh->getSurfaceRadius();
	const unsigned reentryErrorsBefore = simulationOutput.errors;
	setImplicitMeshReentryTarget(explicitSurfaceMesh);
	explicitSurfaceMesh->setSurfaceRadius(0.05f);
	setImplicitMeshReentryTarget(0);
	const unsigned reentryErrors = simulationOutput.errors - reentryErrorsBefore;
	const NxReal setSurfaceRadius = explicitSurfaceMesh->getSurfaceRadius();
	const NxReal initialSingleRadius = explicitSurfaceMesh->getSingleRadius();
	explicitSurfaceMesh->setSingleRadius(0.012f);
	const NxReal setSingleRadius = explicitSurfaceMesh->getSingleRadius();
	const unsigned blendErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	explicitSurfaceMesh->setBlend(0.03f);
	const unsigned setBlendErrors = simulationOutput.errors -
		blendErrorsBefore;
	const bool setBlendErrorMatches = setBlendErrors == 1 &&
		simulationOutput.code == NXE_DB_WARNING &&
		simulationOutput.line == 65 &&
		strcmp(simulationOutput.message,
			"NxImplicitMesh::setBlend(): Feature not available!") == 0;
	const unsigned setBlendErrorCode = simulationOutput.code;
	const int setBlendErrorLine = simulationOutput.line;
	const NxReal blend = explicitSurfaceMesh->getBlend();
	const NxReal initialTriangleSize = explicitSurfaceMesh->getTriangleSize();
	explicitSurfaceMesh->setTriangleSize(0.006f);
	const NxReal setTriangleSize = explicitSurfaceMesh->getTriangleSize();
	const NxU32 initialSmoothingIterations =
		explicitSurfaceMesh->getSmoothingIterations();
	explicitSurfaceMesh->setSmoothingIterations(8);
	const NxU32 setSmoothingIterations =
		explicitSurfaceMesh->getSmoothingIterations();
	static const char licensedSurfaceMeshUpdatedName[] = "licensed-mesh-renamed";
	explicitSurfaceMesh->setName(licensedSurfaceMeshUpdatedName);
	const bool updatedNameMatches = explicitSurfaceMesh->getName() ==
		licensedSurfaceMeshUpdatedName;
	const NxMeshData initialMeshData = explicitSurfaceMesh->getMeshData();
	const bool initialMeshDataMatches = memcmp(&initialMeshData,
		&licensedSurfaceMeshDesc.meshData, sizeof(initialMeshData)) == 0;
	static float implicitMeshPositions[9] = {};
	static float implicitMeshNormals[9] = {};
	static NxU32 implicitMeshIndices[3] = { 0, 1, 2 };
	static NxU32 implicitMeshVertexCount = 3;
	static NxU32 implicitMeshTriangleCount = 1;
	NxMeshData replacementMeshData;
	replacementMeshData.setToDefault();
	replacementMeshData.verticesPosBegin = implicitMeshPositions;
	replacementMeshData.verticesNormalBegin = implicitMeshNormals;
	replacementMeshData.verticesPosByteStride = sizeof(float) * 3;
	replacementMeshData.verticesNormalByteStride = sizeof(float) * 3;
	replacementMeshData.maxVertices = 3;
	replacementMeshData.numVerticesPtr = &implicitMeshVertexCount;
	replacementMeshData.indicesBegin = implicitMeshIndices;
	replacementMeshData.indicesByteStride = sizeof(NxU32);
	replacementMeshData.maxIndices = 3;
	replacementMeshData.numTrianglesPtr = &implicitMeshTriangleCount;
	replacementMeshData.flags = NX_MDF_INDEXED_MESH;
	static const char replacementMeshDataName[] = "replacement-mesh-data";
	replacementMeshData.name = replacementMeshDataName;
	explicitSurfaceMesh->setMeshData(replacementMeshData);
	bool implicitMeshCopyStepMatches = false;
	bool implicitMeshCopyReady = false;
	bool implicitMeshCopyFetched = false;
	bool implicitMeshCopyDataMatches = false;
	if(explicitSurfaceMesh && surfaceMeshScene)
		{
		implicitMeshPositions[0] = implicitMeshPositions[1] =
			implicitMeshPositions[2] = 0.0f;
		implicitMeshNormals[0] = implicitMeshNormals[1] =
			implicitMeshNormals[2] = 0.0f;
		implicitMeshIndices[0] = implicitMeshIndices[1] =
			implicitMeshIndices[2] = 0;
		implicitMeshVertexCount = implicitMeshTriangleCount = 0;
		surfaceMeshScene->simulate(1.0f / 60.0f);
		implicitMeshCopyReady = surfaceMeshScene->checkResults(
			NX_RIGID_BODY_FINISHED, true);
		implicitMeshCopyFetched = surfaceMeshScene->fetchResults(
			NX_RIGID_BODY_FINISHED, true);
		static const unsigned expectedVertices[2][3] = {
			{ 0x3f400001u, 0x3f400002u, 0x3f400003u },
			{ 0x3f800001u, 0x3f800002u, 0x3f800003u }
			};
		static const unsigned expectedNormals[2][3] = {
			{ 0x3fc00001u, 0x3fc00002u, 0x3fc00003u },
			{ 0x40000001u, 0x40000002u, 0x40000003u }
			};
		static const unsigned expectedIndices[2][3] = {
			{ 0x3e800001u, 0x3e800002u, 0x3e800003u },
			{ 0x3f000001u, 0x3f000002u, 0x3f000003u }
			};
		bool oneFixtureMeshMatched = false;
		for(unsigned fixture = 0; fixture != 2; ++fixture)
			oneFixtureMeshMatched = oneFixtureMeshMatched ||
				(memcmp(implicitMeshPositions, expectedVertices[fixture],
					3 * sizeof(unsigned)) == 0 &&
				 memcmp(implicitMeshNormals, expectedNormals[fixture],
					3 * sizeof(unsigned)) == 0 &&
				 memcmp(implicitMeshIndices, expectedIndices[fixture],
					sizeof(implicitMeshIndices)) == 0);
		implicitMeshCopyDataMatches = oneFixtureMeshMatched &&
			implicitMeshVertexCount == 1 && implicitMeshTriangleCount == 1;
		implicitMeshCopyStepMatches = implicitMeshCopyReady &&
			implicitMeshCopyFetched && implicitMeshCopyDataMatches;
		}
	printf("simulation implicit-mesh post-step-copy rows=003653.003655 matched=%u ready=%u fetched=%u data-match=%u vertices=%u triangles=%u data=%08x.%08x.%08x/%08x.%08x.%08x/%08x.%08x.%08x\n",
		implicitMeshCopyStepMatches ? 1u : 0u,
		implicitMeshCopyReady ? 1u : 0u,
		implicitMeshCopyFetched ? 1u : 0u,
		implicitMeshCopyDataMatches ? 1u : 0u,
		implicitMeshVertexCount, implicitMeshTriangleCount,
		*reinterpret_cast<unsigned*>(implicitMeshPositions),
		*reinterpret_cast<unsigned*>(implicitMeshPositions + 1),
		*reinterpret_cast<unsigned*>(implicitMeshPositions + 2),
		*reinterpret_cast<unsigned*>(implicitMeshNormals),
		*reinterpret_cast<unsigned*>(implicitMeshNormals + 1),
		*reinterpret_cast<unsigned*>(implicitMeshNormals + 2),
		implicitMeshIndices[0], implicitMeshIndices[1], implicitMeshIndices[2]);
	const NxMeshData replacedMeshData = explicitSurfaceMesh->getMeshData();
	const bool validMeshDataSetMatches = memcmp(&replacedMeshData,
		&replacementMeshData, sizeof(replacedMeshData)) == 0;
	NxMeshData invalidMeshData = replacementMeshData;
	invalidMeshData.verticesNormalBegin = 0;
	explicitSurfaceMesh->setMeshData(invalidMeshData);
	const NxMeshData afterInvalidMeshData = explicitSurfaceMesh->getMeshData();
	const bool invalidMeshDataPreserved = memcmp(&afterInvalidMeshData,
		&replacementMeshData, sizeof(afterInvalidMeshData)) == 0;
	NxParticleData ignoredParticleData;
	ignoredParticleData.setToDefault();
	explicitSurfaceMesh->setParticles(ignoredParticleData);
	getFluidManagerCallbackSummary(&licensedSurfaceMeshCallbacks);
	licensedSurfaceMeshPropertiesMatch =
		initialSurfaceRadius == 0.04f && setSurfaceRadius == 0.05f &&
		initialSingleRadius == 0.01f && setSingleRadius == 0.012f &&
		blend == 0.0f && initialTriangleSize == 0.005f &&
		setTriangleSize == 0.006f && initialSmoothingIterations == 6 &&
		setSmoothingIterations == 8 && initialNameMatches &&
		updatedNameMatches && initialMeshDataMatches &&
		validMeshDataSetMatches && invalidMeshDataPreserved &&
		implicitMeshCopyStepMatches &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshGetSurfaceRadius == 2 &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshSetSurfaceRadius == 1 &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshGetSingleRadius == 2 &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshSetSingleRadius == 1 &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshSetBlend == 1 &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshGetTriangleSize == 2 &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshSetTriangleSize == 1 &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshGetSmoothingIterations == 2 &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshSetSmoothingIterations == 1 &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshCallbackArgumentErrors == 0 &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshReentryAttempts == 1 &&
		reentryErrors == 1;
	printf("simulation implicit-mesh get-single-radius row=003880 matched=%u initial=%08x updated=%08x calls=%u\n",
		initialSingleRadius == 0.01f && setSingleRadius == 0.012f &&
			licensedSurfaceMeshCallbacks.implicitFluidMeshGetSingleRadius == 2 ? 1u : 0u,
		*reinterpret_cast<const unsigned*>(&initialSingleRadius),
		*reinterpret_cast<const unsigned*>(&setSingleRadius),
		licensedSurfaceMeshCallbacks.implicitFluidMeshGetSingleRadius);
	printf("simulation implicit-mesh set-single-radius row=003882 matched=%u initial=%08x updated=%08x calls=%u\n",
		initialSingleRadius == 0.01f && setSingleRadius == 0.012f &&
			licensedSurfaceMeshCallbacks.implicitFluidMeshSetSingleRadius == 1 ? 1u : 0u,
		*reinterpret_cast<const unsigned*>(&initialSingleRadius),
		*reinterpret_cast<const unsigned*>(&setSingleRadius),
		licensedSurfaceMeshCallbacks.implicitFluidMeshSetSingleRadius);
	printf("simulation implicit-mesh set-blend row=003886 matched=%u value=%08x calls=%u errors=%u code=%u line=%d\n",
		setBlendErrorMatches && blend == 0.0f &&
			licensedSurfaceMeshCallbacks.implicitFluidMeshSetBlend == 1 ? 1u : 0u,
		*reinterpret_cast<const unsigned*>(&blend),
		licensedSurfaceMeshCallbacks.implicitFluidMeshSetBlend,
		setBlendErrors, setBlendErrorCode, setBlendErrorLine);
	printf("simulation implicit-mesh set-triangle-size row=003890 matched=%u initial=%08x updated=%08x calls=%u callback-errors=%u\n",
		initialTriangleSize == 0.005f && setTriangleSize == 0.006f &&
			licensedSurfaceMeshCallbacks.implicitFluidMeshSetTriangleSize == 1 &&
			licensedSurfaceMeshCallbacks.implicitFluidMeshCallbackArgumentErrors == 0 ? 1u : 0u,
		*reinterpret_cast<const unsigned*>(&initialTriangleSize),
		*reinterpret_cast<const unsigned*>(&setTriangleSize),
		licensedSurfaceMeshCallbacks.implicitFluidMeshSetTriangleSize,
		licensedSurfaceMeshCallbacks.implicitFluidMeshCallbackArgumentErrors);
	printf("simulation implicit-mesh get-smoothing-iterations row=003892 matched=%u initial=%u updated=%u calls=%u\n",
		initialSmoothingIterations == 6 && setSmoothingIterations == 8 &&
			licensedSurfaceMeshCallbacks.implicitFluidMeshGetSmoothingIterations == 2 ? 1u : 0u,
		initialSmoothingIterations, setSmoothingIterations,
		licensedSurfaceMeshCallbacks.implicitFluidMeshGetSmoothingIterations);
	printf("simulation implicit-mesh set-smoothing-iterations row=003894 matched=%u value=%u calls=%u callback-errors=%u\n",
		setSmoothingIterations == 8 &&
			licensedSurfaceMeshCallbacks.implicitFluidMeshSetSmoothingIterations == 1 &&
			licensedSurfaceMeshCallbacks.implicitFluidMeshCallbackArgumentErrors == 0 ? 1u : 0u,
		setSmoothingIterations,
		licensedSurfaceMeshCallbacks.implicitFluidMeshSetSmoothingIterations,
		licensedSurfaceMeshCallbacks.implicitFluidMeshCallbackArgumentErrors);
	printf("simulation implicit-mesh set-name row=003896 matched=%u initial=%u updated=%u\n",
		initialNameMatches && updatedNameMatches ? 1u : 0u,
		initialNameMatches ? 1u : 0u, updatedNameMatches ? 1u : 0u);
	printf("simulation implicit-mesh set-surface-radius row=003878 matched=%u calls=%u reentry=%u errors=%u\n",
		licensedSurfaceMeshPropertiesMatch && setSurfaceRadius == 0.05f ? 1u : 0u,
		licensedSurfaceMeshCallbacks.implicitFluidMeshSetSurfaceRadius,
		licensedSurfaceMeshCallbacks.implicitFluidMeshReentryAttempts,
		licensedSurfaceMeshCallbacks.implicitFluidMeshCallbackArgumentErrors);
	}
	if(licensedSurfaceMeshFluid)
		licensedSurfaceMeshFluid->releaseSurfaceMesh();
	getFluidManagerCallbackSummary(&licensedSurfaceMeshCallbacks);
	const bool licensedSurfaceMeshDestroyMatches =
		licensedSurfaceMeshCallbacks.implicitFluidMeshCreate == 1 &&
		licensedSurfaceMeshCallbacks.implicitFluidMeshDestroy == 2;
	if(licensedSurfaceMeshFluid)
		surfaceMeshScene->releaseFluid(*licensedSurfaceMeshFluid);
	printf("simulation fluid licensed-surface-mesh created=%u callback=%u userdata=%u destroy=%u matched=%u\n",
		constructedSurfaceMesh ? 1u : 0u,
		licensedSurfaceMeshCallbacks.implicitFluidMeshCreate,
		constructedSurfaceMeshUserDataMatches ? 1u : 0u,
		licensedSurfaceMeshDestroyMatches ? 1u : 0u,
		licensedSurfaceMeshConstructorMatches ? 1u : 0u);
	printf("simulation fluid licensed-surface-mesh properties row=003704 matched=%u calls=%u.%u.%u.%u.%u.%u.%u.%u.%u reentry=%u errors=%u\n",
		licensedSurfaceMeshPropertiesMatch && explicitSurfaceMeshUserDataMatches ? 1u : 0u,
		licensedSurfaceMeshCallbacks.implicitFluidMeshGetSurfaceRadius,
		licensedSurfaceMeshCallbacks.implicitFluidMeshSetSurfaceRadius,
		licensedSurfaceMeshCallbacks.implicitFluidMeshGetSingleRadius,
		licensedSurfaceMeshCallbacks.implicitFluidMeshSetSingleRadius,
		licensedSurfaceMeshCallbacks.implicitFluidMeshSetBlend,
		licensedSurfaceMeshCallbacks.implicitFluidMeshGetTriangleSize,
		licensedSurfaceMeshCallbacks.implicitFluidMeshSetTriangleSize,
		licensedSurfaceMeshCallbacks.implicitFluidMeshGetSmoothingIterations,
		licensedSurfaceMeshCallbacks.implicitFluidMeshSetSmoothingIterations,
		licensedSurfaceMeshCallbacks.implicitFluidMeshReentryAttempts,
		licensedSurfaceMeshCallbacks.implicitFluidMeshCallbackArgumentErrors);
	printf("simulation fluid public-api surface-mesh get=%u released=%u calls=%u flags=%u errors=%u unlicensed=%u.%u create-warning=%u\n",
		returnedSurfaceMesh == reinterpret_cast<NxImplicitMesh*>(
			&returnedSurfaceMeshMarker) ? 1u : 0u,
		surfaceMeshReleased ? 1u : 0u,
		gSimulationSurfaceMeshDeletingCalls, gSimulationSurfaceMeshDeleteFlags,
		surfaceMeshErrors, unlicensedSurfaceMeshGetterExact ? 1u : 0u,
		unlicensedSurfaceMeshReleaseExact ? 1u : 0u,
		unlicensedSurfaceMeshCreateWarningExact ? 1u : 0u);
	if(surfaceMeshFluid)
		surfaceMeshScene->releaseFluid(*surfaceMeshFluid);
	resetFluidManagerCallbacks();
	NxFluidDesc surfaceMeshConstructorAllocationFailureDesc;
	surfaceMeshConstructorAllocationFailureDesc.setToDefault();
	surfaceMeshConstructorAllocationFailureDesc.surfaceMeshDesc =
		&licensedSurfaceMeshDesc;
	NxSimulationFailSizeAllocator surfaceMeshConstructorFailAllocator(
		*savedFluidAllocator, 0x3c);
	*fluidFoundationAllocatorSlot = &surfaceMeshConstructorFailAllocator;
	simulationOutput.resetLast();
	const unsigned surfaceMeshConstructorAllocationErrorsBefore =
		simulationOutput.errors;
	NxFluid* const surfaceMeshConstructorAllocationFailureFluid =
		surfaceMeshScene ? surfaceMeshScene->createFluid(
			surfaceMeshConstructorAllocationFailureDesc) : 0;
	*fluidFoundationAllocatorSlot = savedFluidAllocator;
	FluidManagerCallbackSummary surfaceMeshConstructorFailureCallbacks = {};
	getFluidManagerCallbackSummary(&surfaceMeshConstructorFailureCallbacks);
	const bool surfaceMeshConstructorAllocationFailureAccepted =
		surfaceMeshConstructorAllocationFailureFluid &&
		surfaceMeshConstructorFailAllocator.mFailCount == 1 &&
		simulationOutput.errors == surfaceMeshConstructorAllocationErrorsBefore &&
		simulationOutput.code == NXE_NO_ERROR &&
		surfaceMeshConstructorFailureCallbacks.implicitFluidMeshCreate == 0 &&
		surfaceMeshConstructorFailureCallbacks.implicitFluidMeshDestroy == 0;
	if(surfaceMeshConstructorAllocationFailureFluid)
		surfaceMeshScene->releaseFluid(
			*surfaceMeshConstructorAllocationFailureFluid);
	getFluidManagerCallbackSummary(&surfaceMeshConstructorFailureCallbacks);
	const bool surfaceMeshConstructorAllocationFailureMatches =
		surfaceMeshConstructorAllocationFailureAccepted &&
		surfaceMeshConstructorFailureCallbacks.implicitFluidMeshCreate == 0 &&
		surfaceMeshConstructorFailureCallbacks.implicitFluidMeshDestroy == 0;
	printf("simulation fluid constructor surface-mesh allocation-failure matched=%u attempts=%u callbacks=%u.%u\n",
		surfaceMeshConstructorAllocationFailureMatches ? 1u : 0u,
		surfaceMeshConstructorFailAllocator.mFailCount,
		surfaceMeshConstructorFailureCallbacks.implicitFluidMeshCreate,
		surfaceMeshConstructorFailureCallbacks.implicitFluidMeshDestroy);
	resetFluidManagerCallbacks();
	NxSimulationFailSizeAllocator surfaceMeshBackingFailAllocator(
		*savedFluidAllocator, 0x18);
	*fluidFoundationAllocatorSlot = &surfaceMeshBackingFailAllocator;
	simulationOutput.resetLast();
	const unsigned surfaceMeshBackingErrorsBefore = simulationOutput.errors;
	NxFluid* const surfaceMeshBackingFailureFluid = surfaceMeshScene
		? surfaceMeshScene->createFluid(surfaceMeshConstructorAllocationFailureDesc)
		: 0;
	*fluidFoundationAllocatorSlot = savedFluidAllocator;
	unsigned char* const surfaceMeshBackingInternal = surfaceMeshBackingFailureFluid
		? *reinterpret_cast<unsigned char**>(reinterpret_cast<unsigned char*>(
			surfaceMeshBackingFailureFluid) + 0x14) : 0;
	void* const surfaceMeshBackingOwner = surfaceMeshBackingInternal
		? *reinterpret_cast<void**>(surfaceMeshBackingInternal + 0x24) : 0;
	const bool surfaceMeshBackingAllocationRouteAccepted =
		surfaceMeshBackingFailureFluid && surfaceMeshBackingOwner &&
		*reinterpret_cast<void**>(static_cast<unsigned char*>(
			surfaceMeshBackingOwner) + 0x38) &&
		surfaceMeshBackingFailAllocator.mFailCount == 0 &&
		simulationOutput.errors == surfaceMeshBackingErrorsBefore &&
		simulationOutput.code == NXE_NO_ERROR;
	if(surfaceMeshBackingFailureFluid)
		surfaceMeshScene->releaseFluid(*surfaceMeshBackingFailureFluid);
	FluidManagerCallbackSummary surfaceMeshBackingCallbacks = {};
	getFluidManagerCallbackSummary(&surfaceMeshBackingCallbacks);
	const bool surfaceMeshBackingAllocationRouteMatches =
		surfaceMeshBackingAllocationRouteAccepted &&
		surfaceMeshBackingCallbacks.fluidCreate == 1 &&
		surfaceMeshBackingCallbacks.implicitFluidMeshCreate == 1 &&
		surfaceMeshBackingCallbacks.fluidDestroy == 1 &&
		surfaceMeshBackingCallbacks.implicitFluidMeshDestroy == 0;
	printf("simulation row3649 surface-mesh-allocation-route returned=%u owner=%u public-mesh=%u sdk-attempts=%u callbacks=%u.%u.%u.%u errors=%u exact=%u\n",
		surfaceMeshBackingFailureFluid ? 1u : 0u,
		surfaceMeshBackingOwner ? 1u : 0u,
		surfaceMeshBackingOwner && *reinterpret_cast<void**>(
			static_cast<unsigned char*>(surfaceMeshBackingOwner) + 0x38) ? 1u : 0u,
		surfaceMeshBackingFailAllocator.mFailCount,
		surfaceMeshBackingCallbacks.fluidCreate,
		surfaceMeshBackingCallbacks.implicitFluidMeshCreate,
		surfaceMeshBackingCallbacks.fluidDestroy,
		surfaceMeshBackingCallbacks.implicitFluidMeshDestroy,
		simulationOutput.errors - surfaceMeshBackingErrorsBefore,
		surfaceMeshBackingAllocationRouteMatches ? 1u : 0u);
	resetFluidManagerCallbacks();
	NxFluidDesc createArgumentDesc;
	createArgumentDesc.setToDefault();
	createArgumentDesc.maxParticles = 0x2345;
	createArgumentDesc.restParticlesPerMeter = 57.0f;
	createArgumentDesc.restDensity = 876.5f;
	createArgumentDesc.kernelRadiusMultiplier = 1.75f;
	createArgumentDesc.stiffness = 27.25f;
	createArgumentDesc.viscosity = 13.5f;
	createArgumentDesc.damping = 0.375f;
	createArgumentDesc.staticCollisionRestitution = 0.625f;
	createArgumentDesc.staticCollisionAdhesion = 0.125f;
	createArgumentDesc.dynamicCollisionRestitution = 0.875f;
	createArgumentDesc.dynamicCollisionAdhesion = 0.25f;
	createArgumentDesc.simulationMethod = NX_F_NO_PARTICLE_INTERACTION;
	createArgumentDesc.collisionMethod =
		NX_F_STATIC | NX_F_DYNAMIC_ACTOR_REACTION;
	NxFluid* const createArgumentFluid = surfaceMeshScene
		? surfaceMeshScene->createFluid(createArgumentDesc) : 0;
	FluidManagerCallbackSummary createArgumentCallbacks = {};
	getFluidManagerCallbackSummary(&createArgumentCallbacks);
	const unsigned expectedCreateArguments[15] = {
		0,
		nxFloatBits(createArgumentDesc.stiffness),
		nxFloatBits(createArgumentDesc.restDensity),
		nxFloatBits(createArgumentDesc.restParticlesPerMeter),
		nxFloatBits(createArgumentDesc.kernelRadiusMultiplier),
		nxFloatBits(createArgumentDesc.viscosity),
		nxFloatBits(createArgumentDesc.damping),
		nxFloatBits(createArgumentDesc.staticCollisionRestitution),
		nxFloatBits(createArgumentDesc.staticCollisionAdhesion),
		nxFloatBits(createArgumentDesc.dynamicCollisionRestitution),
		nxFloatBits(createArgumentDesc.dynamicCollisionAdhesion),
		1024u, createArgumentDesc.maxParticles, 1u, 1u
		};
	unsigned createArgumentDigest = 2166136261u;
	bool createArgumentMapMatches = createArgumentFluid != 0 &&
		createArgumentCallbacks.fluidCreate == 1 &&
		createArgumentCallbacks.fluidCreateArguments[0] != 0 &&
		surfaceMeshLicenseOverride.ready;
	for(unsigned argument = 1; argument != 15; ++argument)
		{
		const unsigned value = createArgumentCallbacks.fluidCreateArguments[argument];
		if(value != expectedCreateArguments[argument])
			createArgumentMapMatches = false;
		for(unsigned byte = 0; byte != 4; ++byte)
			createArgumentDigest = (createArgumentDigest ^
				static_cast<unsigned char>(value >> (byte * 8))) * 16777619u;
		}
	printf("simulation row3649 create-args arguments=14 digest=%08x manager=1 exact=%u\n",
		createArgumentDigest, createArgumentMapMatches ? 1u : 0u);
	if(createArgumentFluid)
		surfaceMeshScene->releaseFluid(*createArgumentFluid);
	resetFluidManagerCallbacks();
	NxFluidDesc combinedInitializationDesc;
	combinedInitializationDesc.setToDefault();
	combinedInitializationDesc.surfaceMeshDesc = &licensedSurfaceMeshDesc;
	NxFluidEmitterDesc combinedInitializationEmitter;
	combinedInitializationEmitter.setToDefault();
	combinedInitializationDesc.emitters.pushBack(
		combinedInitializationEmitter);
	NxU32 combinedParticleCount = 1;
	NxF32 combinedParticlePosition[3] = { 0.25f, -0.5f, 0.75f };
	combinedInitializationDesc.initialParticleData.numParticlesPtr =
		&combinedParticleCount;
	combinedInitializationDesc.initialParticleData.bufferPos =
		combinedParticlePosition;
	combinedInitializationDesc.initialParticleData.bufferPosByteStride =
		sizeof(combinedParticlePosition);
	NxFluid* const combinedInitializationFluid = surfaceMeshScene
		? surfaceMeshScene->createFluid(combinedInitializationDesc) : 0;
	FluidManagerCallbackSummary combinedCreateCallbacks = {};
	getFluidManagerCallbackSummary(&combinedCreateCallbacks);
	char combinedCreationOrder[16] = {};
	const unsigned combinedCreationOrderLength = getFluidCreationOrder(
		combinedCreationOrder, sizeof(combinedCreationOrder));
	if(combinedInitializationFluid)
		surfaceMeshScene->releaseFluid(*combinedInitializationFluid);
	FluidManagerCallbackSummary combinedDestroyCallbacks = {};
	getFluidManagerCallbackSummary(&combinedDestroyCallbacks);
	const bool combinedInitializationMatches = combinedInitializationFluid &&
		surfaceMeshLicenseOverride.ready &&
		combinedCreationOrderLength == 4 &&
		strcmp(combinedCreationOrder, "CMEP") == 0 &&
		combinedCreateCallbacks.fluidCreate == 1 &&
		combinedCreateCallbacks.implicitFluidMeshCreate == 1 &&
		combinedCreateCallbacks.emitterCreate == 1 &&
		combinedDestroyCallbacks.fluidDestroy == 1 &&
		combinedDestroyCallbacks.implicitFluidMeshDestroy == 0 &&
		combinedDestroyCallbacks.emitterDestroy == 0;
	printf("simulation row3649 combined-init order=%s length=%u callbacks=%u.%u.%u cleanup=%u.%u.%u exact=%u\n",
		combinedCreationOrder, combinedCreationOrderLength,
		combinedCreateCallbacks.fluidCreate,
		combinedCreateCallbacks.implicitFluidMeshCreate,
		combinedCreateCallbacks.emitterCreate,
		combinedDestroyCallbacks.fluidDestroy,
		combinedDestroyCallbacks.implicitFluidMeshDestroy,
		combinedDestroyCallbacks.emitterDestroy,
		combinedInitializationMatches ? 1u : 0u);
	if(surfaceMeshScene)
		sdk->releaseScene(*surfaceMeshScene);
	if(!surfaceMeshPublicApiMatches || !licensedSurfaceMeshConstructorMatches ||
		!licensedSurfaceMeshPropertiesMatch ||
		!licensedSurfaceMeshDestroyMatches ||
		!surfaceMeshConstructorAllocationFailureMatches ||
		!surfaceMeshBackingAllocationRouteMatches ||
		!createArgumentMapMatches || !combinedInitializationMatches)
		return nxFail("licensed NxFluid implicit-mesh API did not match the fixture contract");
	}
	bool fluidManagerArrayGrowthMatches = false;
	{
	NxSimulationFluidLicenseOverride fluidArrayLicenseOverride(804);
	resetFluidManagerCallbacks();
	simulationOutput.resetLast();
	const unsigned fluidArrayErrorsBefore = simulationOutput.errors;
	NxScene* const fluidArrayScene = fluidArrayLicenseOverride.ready
		? sdk->createScene(fluidSceneDesc) : 0;
	NxFluid* fluidArrayHandles[7] = {};
	NxFluidDesc fluidArrayDesc;
	fluidArrayDesc.setToDefault();
	for(unsigned i = 0; i != 7 && fluidArrayScene; ++i)
		fluidArrayHandles[i] = fluidArrayScene->createFluid(fluidArrayDesc);
	NxFluid** const fluidArrayPublished = fluidArrayScene
		? fluidArrayScene->getFluids() : 0;
	const unsigned fluidArrayCount = fluidArrayScene
		? fluidArrayScene->getNbFluids() : 0;
	unsigned char* const fluidArraySceneInternal = fluidArrayScene
		? *reinterpret_cast<unsigned char**>(reinterpret_cast<unsigned char*>(
			fluidArrayScene) + 0x24) : 0;
	unsigned char* const fluidArrayManager = fluidArraySceneInternal
		? static_cast<unsigned char*>(*reinterpret_cast<void**>(
			fluidArraySceneInternal + 0x61c)) : 0;
	unsigned* const fluidArrayPrimary = fluidArrayManager
		? reinterpret_cast<unsigned*>(fluidArrayManager + 4) : 0;
	unsigned* const fluidArraySecondary = fluidArrayManager
		? reinterpret_cast<unsigned*>(fluidArrayManager + 0x14) : 0;
	unsigned fluidArrayPublicCapacity = 0;
	unsigned fluidArrayInternalCapacity = 0;
	if(fluidArrayPrimary && fluidArrayPrimary[0] && fluidArrayPrimary[2])
		fluidArrayInternalCapacity = (fluidArrayPrimary[2] -
			fluidArrayPrimary[0]) / sizeof(unsigned);
	if(fluidArraySecondary && fluidArraySecondary[0] && fluidArraySecondary[2])
		fluidArrayPublicCapacity = (fluidArraySecondary[2] -
			fluidArraySecondary[0]) / sizeof(unsigned);
	bool fluidArrayOrderMatches = fluidArrayCount == 7 && fluidArrayPublished &&
		fluidArrayInternalCapacity == 14 && fluidArrayPublicCapacity == 14;
	bool fluidConstructorMatches = fluidArrayCount == 7 && fluidArrayPublished;
	bool fluidConstructorArraysMatch = fluidConstructorMatches;
	bool fluidConstructorBoundsMatch = fluidConstructorMatches;
	bool fluidConstructorSolverMatch = fluidConstructorMatches;
	for(unsigned i = 0; i != 7; ++i)
		{
		fluidArrayOrderMatches = fluidArrayOrderMatches && fluidArrayHandles[i] &&
			fluidArrayPublished && fluidArrayPublished[i] == fluidArrayHandles[i] &&
			fluidArrayPrimary && fluidArraySecondary &&
			reinterpret_cast<void**>(fluidArrayPrimary[0])[i] ==
				*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(
					fluidArrayHandles[i]) + 0x14) &&
			reinterpret_cast<void**>(fluidArraySecondary[0])[i] ==
				fluidArrayHandles[i];
		unsigned char* const internalFluid = fluidArrayHandles[i]
			? static_cast<unsigned char*>(*reinterpret_cast<void**>(
				reinterpret_cast<unsigned char*>(fluidArrayHandles[i]) + 0x14))
			: 0;
		unsigned char* const solver = internalFluid
			? static_cast<unsigned char*>(*reinterpret_cast<void**>(
				internalFluid + 0x54)) : 0;
		const bool fluidArraysDefault = internalFluid &&
			*reinterpret_cast<unsigned*>(internalFluid + 4) == 0 &&
			*reinterpret_cast<unsigned*>(internalFluid + 8) == 0 &&
			*reinterpret_cast<unsigned*>(internalFluid + 0xc) == 0 &&
			*reinterpret_cast<unsigned*>(internalFluid + 0x14) == 0 &&
			*reinterpret_cast<unsigned*>(internalFluid + 0x18) == 0 &&
			*reinterpret_cast<unsigned*>(internalFluid + 0x1c) == 0 &&
			*reinterpret_cast<unsigned*>(internalFluid + 0x24) == 0;
		const bool fluidBoundsDefault = internalFluid &&
			*reinterpret_cast<unsigned*>(internalFluid + 0x5c) == 0x7f7fffffu &&
			*reinterpret_cast<unsigned*>(internalFluid + 0x60) == 0x7f7fffffu &&
			*reinterpret_cast<unsigned*>(internalFluid + 0x64) == 0x7f7fffffu &&
			*reinterpret_cast<unsigned*>(internalFluid + 0x68) == 0xff7fffffu &&
			*reinterpret_cast<unsigned*>(internalFluid + 0x6c) == 0xff7fffffu &&
			*reinterpret_cast<unsigned*>(internalFluid + 0x70) == 0xff7fffffu &&
			*reinterpret_cast<unsigned*>(internalFluid + 0x74) == 0;
		const bool fluidSolverLinksMatch = internalFluid &&
			solver == reinterpret_cast<unsigned char*>(fluidArrayHandles[i]) &&
			*reinterpret_cast<void***>(internalFluid) != 0 &&
			*reinterpret_cast<unsigned*>(internalFluid + 0x58) ==
				NX_FF_VISUALIZATION &&
			*reinterpret_cast<void**>(internalFluid + 0x78) == 0 &&
			*reinterpret_cast<void**>(internalFluid + 0x7c) == fluidArrayManager &&
			*reinterpret_cast<void**>(internalFluid + 0x80) != 0 &&
			*reinterpret_cast<void***>(solver) != 0 &&
			*reinterpret_cast<void***>(solver + 8) != 0 &&
			*reinterpret_cast<void**>(solver + 0x14) == internalFluid;
		fluidConstructorArraysMatch = fluidConstructorArraysMatch &&
			fluidArraysDefault;
		fluidConstructorBoundsMatch = fluidConstructorBoundsMatch &&
			fluidBoundsDefault;
		fluidConstructorSolverMatch = fluidConstructorSolverMatch &&
			fluidSolverLinksMatch;
		fluidConstructorMatches = fluidConstructorMatches && fluidArraysDefault &&
			fluidBoundsDefault && fluidSolverLinksMatch;
		}
	printf("simulation fluid constructor initialized=%u fluids=%u arrays=%u bounds=%u solver-links=%u\n",
		fluidConstructorMatches ? 1u : 0u, fluidArrayCount,
		fluidConstructorArraysMatch ? 1u : 0u,
		fluidConstructorBoundsMatch ? 1u : 0u,
		fluidConstructorSolverMatch ? 1u : 0u);
	if(fluidArrayScene && fluidArrayHandles[2])
		fluidArrayScene->releaseFluid(*fluidArrayHandles[2]);
	NxFluid** const fluidArrayAfterSwap = fluidArrayScene
		? fluidArrayScene->getFluids() : 0;
	const unsigned fluidArrayCountAfterSwap = fluidArrayScene
		? fluidArrayScene->getNbFluids() : 0;
	const bool fluidArraySwapRemoveMatches = fluidArrayAfterSwap &&
		fluidArrayCountAfterSwap == 6 &&
		fluidArrayAfterSwap[0] == fluidArrayHandles[0] &&
		fluidArrayAfterSwap[1] == fluidArrayHandles[1] &&
		fluidArrayAfterSwap[2] == fluidArrayHandles[6] &&
		fluidArrayAfterSwap[3] == fluidArrayHandles[3] &&
		fluidArrayAfterSwap[4] == fluidArrayHandles[4] &&
		fluidArrayAfterSwap[5] == fluidArrayHandles[5];
	for(unsigned i = 0; i != 7; ++i)
		if(i != 2 && fluidArrayScene && fluidArrayHandles[i])
			fluidArrayScene->releaseFluid(*fluidArrayHandles[i]);
	FluidManagerCallbackSummary fluidArrayCallbacks = {};
	getFluidManagerCallbackSummary(&fluidArrayCallbacks);
	const bool fluidArrayManagerCleared = fluidArraySceneInternal &&
		*reinterpret_cast<void**>(fluidArraySceneInternal + 0x61c) == 0;
	fluidManagerArrayGrowthMatches = fluidArrayLicenseOverride.ready &&
		fluidArrayOrderMatches && fluidArraySwapRemoveMatches &&
		fluidArrayManagerCleared && fluidArrayCallbacks.managerCreate == 1 &&
		fluidArrayCallbacks.managerDestroy == 1 &&
		fluidArrayCallbacks.fluidCreate == 7 &&
		fluidArrayCallbacks.fluidDestroy == 7 &&
		simulationOutput.errors == fluidArrayErrorsBefore;
	printf("simulation fluid manager-array-growth created=%u listed=%u capacity=%u.%u swap-remove=%u order=%u callbacks=%u.%u manager-cleared=%u errors=%u\n",
		7u, fluidArrayCount, fluidArrayInternalCapacity,
		fluidArrayPublicCapacity, fluidArraySwapRemoveMatches ? 1u : 0u,
		fluidArrayOrderMatches ? 1u : 0u, fluidArrayCallbacks.fluidCreate,
		fluidArrayCallbacks.fluidDestroy, fluidArrayManagerCleared ? 1u : 0u,
		simulationOutput.errors - fluidArrayErrorsBefore);
	if(fluidArrayScene)
		sdk->releaseScene(*fluidArrayScene);
	if(!fluidManagerArrayGrowthMatches)
		return nxFail("fluid manager arrays did not preserve growth and removal order");
	}
	bool fluidCreateNullBackendPublished = false;
	{
	NxSimulationFluidLicenseOverride nullBackendLicenseOverride(804);
	resetFluidManagerCallbacks();
	setFluidCreateNullIndex(0);
	simulationOutput.resetLast();
	const unsigned nullBackendErrorsBefore = simulationOutput.errors;
	NxScene* const nullBackendScene = nullBackendLicenseOverride.ready
		? sdk->createScene(fluidSceneDesc) : 0;
	NxFluidDesc nullBackendDesc;
	nullBackendDesc.setToDefault();
	NxFluid* const nullBackendFluid = nullBackendScene
		? nullBackendScene->createFluid(nullBackendDesc) : 0;
	unsigned char* const nullBackendSceneInternal = nullBackendScene
		? *reinterpret_cast<unsigned char**>(reinterpret_cast<unsigned char*>(
			nullBackendScene) + 0x24) : 0;
	unsigned char* const nullBackendManager = nullBackendSceneInternal
		? static_cast<unsigned char*>(*reinterpret_cast<void**>(
			nullBackendSceneInternal + 0x61c)) : 0;
	unsigned char* const nullBackendInternalFluid = nullBackendFluid
		? static_cast<unsigned char*>(*reinterpret_cast<void**>(
			reinterpret_cast<unsigned char*>(nullBackendFluid) + 0x14)) : 0;
	unsigned* const nullBackendPrimary = nullBackendManager
		? reinterpret_cast<unsigned*>(nullBackendManager + 4) : 0;
	unsigned* const nullBackendSecondary = nullBackendManager
		? reinterpret_cast<unsigned*>(nullBackendManager + 0x14) : 0;
	NxFluid** const nullBackendPublishedFluids = nullBackendScene
		? nullBackendScene->getFluids() : 0;
	const bool nullBackendCreatePublished = nullBackendFluid &&
		nullBackendInternalFluid &&
		*reinterpret_cast<void**>(nullBackendInternalFluid + 0x80) == 0 &&
		nullBackendScene->getNbFluids() == 1 && nullBackendPublishedFluids &&
		nullBackendPublishedFluids[0] == nullBackendFluid &&
		nullBackendPrimary && nullBackendPrimary[0] && nullBackendPrimary[1] -
			nullBackendPrimary[0] == sizeof(unsigned) &&
		*reinterpret_cast<void**>(nullBackendPrimary[0]) == nullBackendInternalFluid &&
		nullBackendSecondary && nullBackendSecondary[0] &&
		nullBackendSecondary[1] - nullBackendSecondary[0] == sizeof(unsigned) &&
		*reinterpret_cast<void**>(nullBackendSecondary[0]) == nullBackendFluid;
	if(nullBackendFluid)
		nullBackendScene->releaseFluid(*nullBackendFluid);
	FluidManagerCallbackSummary nullBackendCallbacks = {};
	getFluidManagerCallbackSummary(&nullBackendCallbacks);
	const bool nullBackendCleanup = nullBackendSceneInternal &&
		*reinterpret_cast<void**>(nullBackendSceneInternal + 0x61c) == 0 &&
		nullBackendCallbacks.managerCreate == 1 &&
		nullBackendCallbacks.managerDestroy == 1 &&
		nullBackendCallbacks.fluidCreate == 1 &&
		nullBackendCallbacks.fluidDestroy == 0 &&
		simulationOutput.errors == nullBackendErrorsBefore;
	fluidCreateNullBackendPublished = nullBackendLicenseOverride.ready &&
		nullBackendCreatePublished && nullBackendCleanup;
	printf("simulation row3649 backend-null published=%u arrays=%u cleanup=%u callbacks=%u exact=%u\n",
		nullBackendFluid ? 1u : 0u, nullBackendCreatePublished ? 1u : 0u,
		nullBackendCleanup ? 1u : 0u,
		nullBackendCallbacks.managerCreate + nullBackendCallbacks.fluidCreate +
			nullBackendCallbacks.fluidDestroy + nullBackendCallbacks.managerDestroy,
		fluidCreateNullBackendPublished ? 1u : 0u);
	if(nullBackendScene)
		sdk->releaseScene(*nullBackendScene);
	if(!fluidCreateNullBackendPublished)
		return nxFail("null FluidCreate backend result did not preserve the oracle fluid lifecycle");
	}
	bool fluidInitializationCallbackOrderMatches = false;
	{
	NxSimulationFluidLicenseOverride initializationOrderLicenseOverride(804);
	resetFluidManagerCallbacks();
	NxScene* const initializationOrderScene = initializationOrderLicenseOverride.ready
		? sdk->createScene(fluidSceneDesc) : 0;
	NxFluidDesc initializationOrderDesc;
	initializationOrderDesc.setToDefault();
	NxFluidEmitterDesc initializationOrderEmitterA;
	initializationOrderEmitterA.setToDefault();
	initializationOrderEmitterA.relPose.t.set(0.25f, -0.5f, 0.75f);
	NxFluidEmitterDesc initializationOrderEmitterB;
	initializationOrderEmitterB.setToDefault();
	initializationOrderEmitterB.relPose.t.set(-1.25f, 0.5f, 0.25f);
	initializationOrderDesc.emitters.pushBack(initializationOrderEmitterA);
	initializationOrderDesc.emitters.pushBack(initializationOrderEmitterB);
	NxU32 initializationOrderCount = 1;
	NxF32 initializationOrderPosition[3] = {1.0f, 2.0f, 3.0f};
	initializationOrderDesc.initialParticleData.numParticlesPtr =
		&initializationOrderCount;
	initializationOrderDesc.initialParticleData.bufferPos =
		initializationOrderPosition;
	initializationOrderDesc.initialParticleData.bufferPosByteStride =
		3 * sizeof(NxF32);
	NxFluid* const initializationOrderFluid = initializationOrderScene
		? initializationOrderScene->createFluid(initializationOrderDesc) : 0;
	char initializationOrder[64] = {};
	const unsigned initializationOrderLength = getFluidCreationOrder(
		initializationOrder, sizeof(initializationOrder));
	unsigned emitterCreateRecordA[15] = {};
	unsigned emitterCreateRecordB[15] = {};
	const unsigned emitterCreateRecordWordsA = getFluidEmitterCreateRecord(0,
		emitterCreateRecordA, sizeof(emitterCreateRecordA) / sizeof(unsigned));
	const unsigned emitterCreateRecordWordsB = getFluidEmitterCreateRecord(1,
		emitterCreateRecordB, sizeof(emitterCreateRecordB) / sizeof(unsigned));
	unsigned expectedEmitterPoseA[12] = {};
	unsigned expectedEmitterPoseB[12] = {};
	memcpy(expectedEmitterPoseA, &initializationOrderEmitterA.relPose,
		sizeof(expectedEmitterPoseA));
	memcpy(expectedEmitterPoseB, &initializationOrderEmitterB.relPose,
		sizeof(expectedEmitterPoseB));
	const float expectedEmitterRestDistance = 0.02f;
	unsigned expectedEmitterRestDistanceBits = 0;
	memcpy(&expectedEmitterRestDistanceBits, &expectedEmitterRestDistance,
		sizeof(expectedEmitterRestDistanceBits));
	const bool emitterCallbackArgumentsMatch = emitterCreateRecordWordsA == 15 &&
		emitterCreateRecordWordsB == 15 && emitterCreateRecordA[0] != 0 &&
		emitterCreateRecordA[1] != 0 &&
		emitterCreateRecordA[0] == emitterCreateRecordB[0] &&
		emitterCreateRecordA[1] == emitterCreateRecordB[1] &&
		memcmp(&emitterCreateRecordA[2], expectedEmitterPoseA,
			sizeof(expectedEmitterPoseA)) == 0 &&
		memcmp(&emitterCreateRecordB[2], expectedEmitterPoseB,
			sizeof(expectedEmitterPoseB)) == 0 &&
		emitterCreateRecordA[14] == expectedEmitterRestDistanceBits &&
		emitterCreateRecordB[14] == expectedEmitterRestDistanceBits;
	fluidInitializationCallbackOrderMatches = initializationOrderLicenseOverride.ready &&
		initializationOrderFluid && initializationOrderLength == 4 &&
		initializationOrder[0] == 'C' && initializationOrder[1] == 'E' &&
		initializationOrder[2] == 'E' && initializationOrder[3] == 'P' &&
		emitterCallbackArgumentsMatch;
	printf("simulation row3649 initialization-order order=%s expected=CEEP emitter-args=%u.%u rest=%08x exact=%u\n",
		initializationOrder, emitterCreateRecordWordsA == 15 ? 1u : 0u,
		emitterCreateRecordWordsB == 15 ? 1u : 0u,
		emitterCreateRecordWordsB == 15 ? emitterCreateRecordB[14] : 0u,
		fluidInitializationCallbackOrderMatches ? 1u : 0u);
	if(initializationOrderFluid)
		initializationOrderScene->releaseFluid(*initializationOrderFluid);
	if(initializationOrderScene)
		sdk->releaseScene(*initializationOrderScene);
	if(!fluidInitializationCallbackOrderMatches)
		return nxFail("fluid initialization callback order did not match the oracle");
	}
	sdk->releaseScene(*enabledFluidScene);
	bool enabledLicensedKernelGetter = false;
	bool licensedStiffnessSetter = false;
	bool licensedViscositySetter = false;
	bool licensedDampingSetter = false;
	bool licensedStaticCollisionRestitutionSetter = false;
	bool licensedStaticCollisionAdhesionSetter = false;
	bool licensedDynamicCollisionRestitutionSetter = false;
	bool licensedDynamicCollisionAdhesionSetter = false;
	{
	NxSimulationFluidLicenseOverride licenseOverride;
	NxScene* const licensedFluidScene = licenseOverride.ready
		? sdk->createScene(fluidSceneDesc) : 0;
	NxFluidDesc licensedFluidDesc;
	licensedFluidDesc.setToDefault();
	NxFluid* const licensedFluid = licensedFluidScene
		? licensedFluidScene->createFluid(licensedFluidDesc) : 0;
	FluidManagerCallbackSummary licensedCallbacksBefore = {};
	getFluidManagerCallbackSummary(&licensedCallbacksBefore);
	unsigned stiffnessCallsBefore = 0, stiffnessBitsBefore = 0,
		stiffnessArgumentsBefore = 0;
	getFluidSetStiffnessObservation(&stiffnessCallsBefore, &stiffnessBitsBefore,
		&stiffnessArgumentsBefore);
	const unsigned licensedSetStiffnessErrorsBefore = simulationOutput.errors;
	if(licensedFluid)
		licensedFluid->setStiffness(0.625f);
	const unsigned licensedSetStiffnessErrors = simulationOutput.errors -
		licensedSetStiffnessErrorsBefore;
	unsigned stiffnessCallsAfter = 0, stiffnessBitsAfter = 0,
		stiffnessArgumentsAfter = 0;
	getFluidSetStiffnessObservation(&stiffnessCallsAfter, &stiffnessBitsAfter,
		&stiffnessArgumentsAfter);
	const unsigned licensedKernelErrorsBefore = simulationOutput.errors;
	const NxReal licensedKernelRadius = licensedFluid
		? licensedFluid->getKernelRadiusMultiplier() : 0.0f;
	const unsigned licensedKernelBits = nxFloatBits(licensedKernelRadius);
	const unsigned licensedKernelErrors = simulationOutput.errors -
		licensedKernelErrorsBefore;
	FluidManagerCallbackSummary licensedCallbacksAfter = {};
	getFluidManagerCallbackSummary(&licensedCallbacksAfter);
	enabledLicensedKernelGetter = licenseOverride.ready && licensedFluid &&
		licensedKernelBits == 0x3fe00000u &&
		licensedCallbacksAfter.fluidGetKernelRadiusMultiplier ==
			licensedCallbacksBefore.fluidGetKernelRadiusMultiplier + 1 &&
		licensedKernelErrors == 0;
	licensedStiffnessSetter = licenseOverride.ready && licensedFluid &&
		licensedSetStiffnessErrors == 0 &&
		stiffnessCallsAfter == stiffnessCallsBefore + 1 &&
		stiffnessBitsAfter == 0x3f200000u && stiffnessArgumentsAfter == 1;
	unsigned viscosityCallsBefore = 0, viscosityBitsBefore = 0,
		viscosityArgumentsBefore = 0;
	getFluidSetViscosityObservation(&viscosityCallsBefore, &viscosityBitsBefore,
		&viscosityArgumentsBefore);
	const unsigned licensedSetViscosityErrorsBefore = simulationOutput.errors;
	if(licensedFluid)
		licensedFluid->setViscosity(0.375f);
	const unsigned licensedSetViscosityErrors = simulationOutput.errors -
		licensedSetViscosityErrorsBefore;
	unsigned viscosityCallsAfter = 0, viscosityBitsAfter = 0,
		viscosityArgumentsAfter = 0;
	getFluidSetViscosityObservation(&viscosityCallsAfter, &viscosityBitsAfter,
		&viscosityArgumentsAfter);
	licensedViscositySetter = licenseOverride.ready && licensedFluid &&
		licensedSetViscosityErrors == 0 &&
		viscosityCallsAfter == viscosityCallsBefore + 1 &&
		viscosityBitsAfter == 0x3ec00000u && viscosityArgumentsAfter == 1;
	printf("simulation fluid public-api configuration licensed-set-viscosity value=%08x calls=%u args=%u errors=%u\n",
		viscosityBitsAfter, viscosityCallsAfter - viscosityCallsBefore,
		viscosityArgumentsAfter, licensedSetViscosityErrors);
	unsigned dampingCallsBefore = 0, dampingBitsBefore = 0,
		dampingArgumentsBefore = 0;
	getFluidSetDampingObservation(&dampingCallsBefore, &dampingBitsBefore,
		&dampingArgumentsBefore);
	const unsigned licensedSetDampingErrorsBefore = simulationOutput.errors;
	if(licensedFluid)
		licensedFluid->setDamping(0.25f);
	const unsigned licensedSetDampingErrors = simulationOutput.errors -
		licensedSetDampingErrorsBefore;
	unsigned dampingCallsAfter = 0, dampingBitsAfter = 0,
		dampingArgumentsAfter = 0;
	getFluidSetDampingObservation(&dampingCallsAfter, &dampingBitsAfter,
		&dampingArgumentsAfter);
	licensedDampingSetter = licenseOverride.ready && licensedFluid &&
		licensedSetDampingErrors == 0 &&
		dampingCallsAfter == dampingCallsBefore + 1 &&
		dampingBitsAfter == 0x3e800000u && dampingArgumentsAfter == 1;
	printf("simulation fluid public-api configuration licensed-set-damping value=%08x calls=%u args=%u errors=%u\n",
		dampingBitsAfter, dampingCallsAfter - dampingCallsBefore,
		dampingArgumentsAfter, licensedSetDampingErrors);
	unsigned staticRestitutionSetCallsBefore = 0, staticRestitutionSetBitsBefore = 0,
		staticRestitutionSetArgumentsBefore = 0;
	getFluidSetStaticCollisionRestitutionObservation(
		&staticRestitutionSetCallsBefore, &staticRestitutionSetBitsBefore,
		&staticRestitutionSetArgumentsBefore);
	const unsigned licensedSetStaticRestitutionErrorsBefore = simulationOutput.errors;
	if(licensedFluid)
		licensedFluid->setStaticCollisionRestitution(0.5f);
	const unsigned licensedSetStaticRestitutionErrors = simulationOutput.errors -
		licensedSetStaticRestitutionErrorsBefore;
	unsigned staticRestitutionSetCallsAfter = 0, staticRestitutionSetBitsAfter = 0,
		staticRestitutionSetArgumentsAfter = 0;
	getFluidSetStaticCollisionRestitutionObservation(
		&staticRestitutionSetCallsAfter, &staticRestitutionSetBitsAfter,
		&staticRestitutionSetArgumentsAfter);
	licensedStaticCollisionRestitutionSetter = licenseOverride.ready && licensedFluid &&
		licensedSetStaticRestitutionErrors == 0 &&
		staticRestitutionSetCallsAfter == staticRestitutionSetCallsBefore + 1 &&
		staticRestitutionSetBitsAfter == 0x3f000000u &&
		staticRestitutionSetArgumentsAfter == 1;
	printf("simulation fluid public-api configuration licensed-set-static-restitution value=%08x calls=%u args=%u errors=%u\n",
		staticRestitutionSetBitsAfter,
		staticRestitutionSetCallsAfter - staticRestitutionSetCallsBefore,
		staticRestitutionSetArgumentsAfter, licensedSetStaticRestitutionErrors);
	unsigned staticAdhesionSetCallsBefore = 0, staticAdhesionSetBitsBefore = 0,
		staticAdhesionSetArgumentsBefore = 0;
	getFluidSetStaticCollisionAdhesionObservation(&staticAdhesionSetCallsBefore,
		&staticAdhesionSetBitsBefore, &staticAdhesionSetArgumentsBefore);
	const unsigned licensedSetStaticAdhesionErrorsBefore = simulationOutput.errors;
	if(licensedFluid)
		licensedFluid->setStaticCollisionAdhesion(0.25f);
	const unsigned licensedSetStaticAdhesionErrors = simulationOutput.errors -
		licensedSetStaticAdhesionErrorsBefore;
	unsigned staticAdhesionSetCallsAfter = 0, staticAdhesionSetBitsAfter = 0,
		staticAdhesionSetArgumentsAfter = 0;
	getFluidSetStaticCollisionAdhesionObservation(&staticAdhesionSetCallsAfter,
		&staticAdhesionSetBitsAfter, &staticAdhesionSetArgumentsAfter);
	licensedStaticCollisionAdhesionSetter = licenseOverride.ready && licensedFluid &&
		licensedSetStaticAdhesionErrors == 0 &&
		staticAdhesionSetCallsAfter == staticAdhesionSetCallsBefore + 1 &&
		staticAdhesionSetBitsAfter == 0x3e800000u &&
		staticAdhesionSetArgumentsAfter == 1;
	printf("simulation fluid public-api configuration licensed-set-static-adhesion value=%08x calls=%u args=%u errors=%u\n",
		staticAdhesionSetBitsAfter,
		staticAdhesionSetCallsAfter - staticAdhesionSetCallsBefore,
		staticAdhesionSetArgumentsAfter, licensedSetStaticAdhesionErrors);
	unsigned dynamicRestitutionSetCallsBefore = 0,
		dynamicRestitutionSetBitsBefore = 0,
		dynamicRestitutionSetArgumentsBefore = 0;
	getFluidSetDynamicCollisionRestitutionObservation(
		&dynamicRestitutionSetCallsBefore, &dynamicRestitutionSetBitsBefore,
		&dynamicRestitutionSetArgumentsBefore);
	const unsigned licensedSetDynamicRestitutionErrorsBefore = simulationOutput.errors;
	if(licensedFluid)
		licensedFluid->setDynamicCollisionRestitution(0.75f);
	const unsigned licensedSetDynamicRestitutionErrors = simulationOutput.errors -
		licensedSetDynamicRestitutionErrorsBefore;
	const bool licensedSetDynamicRestitutionDiagnosticExact =
		licensedSetDynamicRestitutionErrors == 1 &&
		simulationOutput.code == NXE_DB_WARNING && simulationOutput.line == 195 &&
		strcmp(simulationOutput.file,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\NpFluid.cpp") == 0 &&
		strcmp(simulationOutput.message,
			"NxFluid::setDynamicCollisionRestitution(): Feature not available!") == 0;
	unsigned dynamicRestitutionSetCallsAfter = 0,
		dynamicRestitutionSetBitsAfter = 0,
		dynamicRestitutionSetArgumentsAfter = 0;
	getFluidSetDynamicCollisionRestitutionObservation(
		&dynamicRestitutionSetCallsAfter, &dynamicRestitutionSetBitsAfter,
		&dynamicRestitutionSetArgumentsAfter);
	licensedDynamicCollisionRestitutionSetter = licenseOverride.ready && licensedFluid &&
		licensedSetDynamicRestitutionDiagnosticExact &&
		dynamicRestitutionSetCallsAfter == dynamicRestitutionSetCallsBefore + 1 &&
		dynamicRestitutionSetBitsAfter == 0x3f400000u &&
		dynamicRestitutionSetArgumentsAfter == 1;
	printf("simulation fluid public-api configuration licensed-set-dynamic-restitution value=%08x calls=%u args=%u errors=%u diagnostic=%u\n",
		dynamicRestitutionSetBitsAfter,
		dynamicRestitutionSetCallsAfter - dynamicRestitutionSetCallsBefore,
		dynamicRestitutionSetArgumentsAfter, licensedSetDynamicRestitutionErrors,
		licensedSetDynamicRestitutionDiagnosticExact ? 1u : 0u);
	unsigned dynamicAdhesionSetCallsBefore = 0, dynamicAdhesionSetBitsBefore = 0,
		dynamicAdhesionSetArgumentsBefore = 0;
	getFluidSetDynamicCollisionAdhesionObservation(&dynamicAdhesionSetCallsBefore,
		&dynamicAdhesionSetBitsBefore, &dynamicAdhesionSetArgumentsBefore);
	const unsigned licensedSetDynamicAdhesionErrorsBefore = simulationOutput.errors;
	if(licensedFluid)
		licensedFluid->setDynamicCollisionAdhesion(0.125f);
	const unsigned licensedSetDynamicAdhesionErrors = simulationOutput.errors -
		licensedSetDynamicAdhesionErrorsBefore;
	const bool licensedSetDynamicAdhesionDiagnosticExact =
		licensedSetDynamicAdhesionErrors == 1 &&
		simulationOutput.code == NXE_DB_WARNING && simulationOutput.line == 210 &&
		strcmp(simulationOutput.file,
			"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\NpFluid.cpp") == 0 &&
		strcmp(simulationOutput.message,
			"NxFluid::setDynamicCollisionAdhesion(): Feature not available!") == 0;
	unsigned dynamicAdhesionSetCallsAfter = 0, dynamicAdhesionSetBitsAfter = 0,
		dynamicAdhesionSetArgumentsAfter = 0;
	getFluidSetDynamicCollisionAdhesionObservation(&dynamicAdhesionSetCallsAfter,
		&dynamicAdhesionSetBitsAfter, &dynamicAdhesionSetArgumentsAfter);
	licensedDynamicCollisionAdhesionSetter = licenseOverride.ready && licensedFluid &&
		licensedSetDynamicAdhesionDiagnosticExact &&
		dynamicAdhesionSetCallsAfter == dynamicAdhesionSetCallsBefore + 1 &&
		dynamicAdhesionSetBitsAfter == 0x3e000000u &&
		dynamicAdhesionSetArgumentsAfter == 1;
	printf("simulation fluid public-api configuration licensed-set-dynamic-adhesion value=%08x calls=%u args=%u errors=%u diagnostic=%u\n",
		dynamicAdhesionSetBitsAfter,
		dynamicAdhesionSetCallsAfter - dynamicAdhesionSetCallsBefore,
		dynamicAdhesionSetArgumentsAfter, licensedSetDynamicAdhesionErrors,
		licensedSetDynamicAdhesionDiagnosticExact ? 1u : 0u);
	printf("simulation fluid public-api configuration licensed-set-stiffness value=%08x calls=%u args=%u errors=%u\n",
		stiffnessBitsAfter, stiffnessCallsAfter - stiffnessCallsBefore,
		stiffnessArgumentsAfter, licensedSetStiffnessErrors);
	printf("simulation fluid public-api configuration licensed-kernel=%08x calls=%u errors=%u setup=%u fluid=%u\n",
		licensedKernelBits,
		licensedCallbacksAfter.fluidGetKernelRadiusMultiplier -
			licensedCallbacksBefore.fluidGetKernelRadiusMultiplier,
		licensedKernelErrors,
		licenseOverride.ready ? 1u : 0u, licensedFluid ? 1u : 0u);
	if(licensedFluid)
		licensedFluidScene->releaseFluid(*licensedFluid);
	if(licensedFluidScene)
		sdk->releaseScene(*licensedFluidScene);
	}
	if(!enabledLicensedKernelGetter || !licensedStiffnessSetter ||
		!licensedViscositySetter || !licensedDampingSetter ||
		!licensedStaticCollisionRestitutionSetter ||
		!licensedStaticCollisionAdhesionSetter ||
		!licensedDynamicCollisionRestitutionSetter ||
		!licensedDynamicCollisionAdhesionSetter)
		return nxFail("licensed NxFluid scalar setter or kernel-radius getter did not forward through FluidModel");
	// NxScene::createController forwards to the Scene-owned controller list in
	// the pinned DLL. NxControllerDesc is intentionally incomplete in this SDK
	// header set, so supply its recovered byte layout without changing public
	// headers. A zero type word selects the descriptor path accepted by the
	// oracle; the three dimensions at +0x30 are a half-meter box controller.
	NxSceneDesc controllerSceneDesc;
	controllerSceneDesc.setToDefault();
	NxScene* const controllerScene = sdk->createScene(controllerSceneDesc);
	if(!controllerScene)
		return nxFail("controller scene creation failed");
	const NxU32 controllerSceneActorCountBefore = controllerScene->getNbActors();
	alignas(4) unsigned char controllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(controllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(controllerDescStorage + 0x34) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(controllerDescStorage + 0x38) = nxFloatBits(0.5f);
	alignas(4) unsigned char rejectedControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(rejectedControllerDescStorage + 8) = 1;
	NxController* const rejectedController = controllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(rejectedControllerDescStorage));
	printf("simulation controller-create rejected=%u actors=%u\n",
		rejectedController == 0, controllerScene->getNbActors());
	controllerCreateFailed = rejectedController != 0 ||
		controllerScene->getNbActors() != controllerSceneActorCountBefore;
	NxController* const controller = controllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(controllerDescStorage));
	alignas(4) unsigned char secondControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(secondControllerDescStorage + 0x0c) = nxFloatBits(2.0f);
	*reinterpret_cast<NxU32*>(secondControllerDescStorage + 0x30) = nxFloatBits(0.25f);
	*reinterpret_cast<NxU32*>(secondControllerDescStorage + 0x34) = nxFloatBits(0.75f);
	*reinterpret_cast<NxU32*>(secondControllerDescStorage + 0x38) = nxFloatBits(0.25f);
	NxController* const secondController = controllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(secondControllerDescStorage));
	const NxU32 controllerSceneActorCountCreated = controllerScene->getNbActors();
	printf("simulation controller-create first=%u second=%u actors-before=%u actors-created=%u\n",
		controller != 0, secondController != 0, controllerSceneActorCountBefore,
		controllerSceneActorCountCreated);
	controllerCreateFailed = controllerCreateFailed || controller == 0 || secondController == 0 ||
		controllerSceneActorCountCreated != controllerSceneActorCountBefore + 2;
	if(controller && secondController)
		{
		const NxVec3& firstPosition =
			reinterpret_cast<NxControllerProbe*>(controller)->getPosition();
		const NxVec3& secondPosition =
			reinterpret_cast<NxControllerProbe*>(secondController)->getPosition();
		printf("simulation controller-position first=%08x.%08x.%08x second=%08x.%08x.%08x\n",
			nxFloatBits(firstPosition.x), nxFloatBits(firstPosition.y), nxFloatBits(firstPosition.z),
			nxFloatBits(secondPosition.x), nxFloatBits(secondPosition.y), nxFloatBits(secondPosition.z));
		controllerCreateFailed = controllerCreateFailed ||
			firstPosition.x != 0.0f || firstPosition.y != 0.0f || firstPosition.z != 0.0f ||
			secondPosition.x != 2.0f || secondPosition.y != 0.0f || secondPosition.z != 0.0f;
		NxU32 collisionFlags = 0xdeadbeef;
		const NxVec3 displacement(0.25f, 0.0f, 0.0f);
		reinterpret_cast<NxControllerProbe*>(controller)->move(
			displacement, 0xffffffff, 0.001f, collisionFlags);
		const NxVec3& movedPosition =
			reinterpret_cast<NxControllerProbe*>(controller)->getPosition();
		printf("simulation controller-move position=%08x.%08x.%08x flags=%08x\n",
			nxFloatBits(movedPosition.x), nxFloatBits(movedPosition.y),
			nxFloatBits(movedPosition.z), collisionFlags);
		controllerCreateFailed = controllerCreateFailed ||
			movedPosition.x != 0.25f || movedPosition.y != 0.0f ||
			movedPosition.z != 0.0f || collisionFlags != 0;
		NxVec3 actorPositionBeforeStep;
		NxActor** const controllerActorsBeforeStep = controllerScene->getActors();
		controllerActorsBeforeStep[controllerSceneActorCountBefore]->getGlobalPosition(actorPositionBeforeStep);
		printf("simulation controller-move actor-before-step=%08x.%08x.%08x\n",
			nxFloatBits(actorPositionBeforeStep.x), nxFloatBits(actorPositionBeforeStep.y),
			nxFloatBits(actorPositionBeforeStep.z));
		controllerCreateFailed = controllerCreateFailed || actorPositionBeforeStep.x != 0.0f ||
			actorPositionBeforeStep.y != 0.0f || actorPositionBeforeStep.z != 0.0f;
		}
	NxActor** const controllerActors = controllerScene->getActors();
	for(NxU32 index = 0; index != 2 && index < controllerSceneActorCountCreated; ++index)
		{
		NxVec3 actorPosition;
		controllerActors[controllerSceneActorCountBefore + index]->getGlobalPosition(actorPosition);
		NxShape** const actorShapes = controllerActors[controllerSceneActorCountBefore + index]->getShapes();
		if(!actorShapes || controllerActors[controllerSceneActorCountBefore + index]->getNbShapes() != 1)
			{
			controllerCreateFailed = true;
			printf("simulation controller-actor index=%u shape-layout-invalid\n", index);
			continue;
			}
		const NxVec3& actorDimensions = static_cast<NxBoxShape*>(actorShapes[0])->getDimensions();
		printf("simulation controller-actor index=%u position=%08x.%08x.%08x dimensions=%08x.%08x.%08x\n",
			index, nxFloatBits(actorPosition.x), nxFloatBits(actorPosition.y),
			nxFloatBits(actorPosition.z), nxFloatBits(actorDimensions.x),
			nxFloatBits(actorDimensions.y), nxFloatBits(actorDimensions.z));
		// The controller advances its exposed position immediately, while the
		// generated kinematic actor still exposes its pre-step global pose.
		const NxReal expectedX = index == 0 ? 0.0f : 2.0f;
		const NxU32 expectedHalfExtent = index == 0 ? 0x3f0ccccd : 0x3e8ccccd;
		const NxU32 expectedHeight = index == 0 ? 0x3f8ccccd : 0x3f533334;
		controllerCreateFailed = controllerCreateFailed ||
			actorPosition.x != expectedX || actorPosition.y != 0.0f || actorPosition.z != 0.0f ||
			nxFloatBits(actorDimensions.x) != expectedHalfExtent ||
			nxFloatBits(actorDimensions.y) != expectedHeight ||
			nxFloatBits(actorDimensions.z) != expectedHalfExtent;
		}
	// Releasing the first item exercises Scene::removeController's non-head walk;
	// releasing the second then checks removal of the remaining head.
	const unsigned controllerReleaseErrorsBefore = simulationOutput.errors;
	simulationOutput.resetLast();
	if(controller)
		controllerScene->releaseController(*controller);
	const NxU32 controllerSceneActorCountAfterFirstRelease = controllerScene->getNbActors();
	const bool controllerListHasStaleNext = secondController &&
		*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(secondController) + 0x38) != 0;
	const unsigned controllerReleaseErrorsAfterFirst =
		simulationOutput.errors - controllerReleaseErrorsBefore;
	if(secondController)
		controllerScene->releaseController(*secondController);
	const NxU32 controllerSceneActorCountReleased = controllerScene->getNbActors();
	const unsigned controllerReleaseErrorsAfterSecond =
		simulationOutput.errors - controllerReleaseErrorsBefore;
	printf("simulation controller-release actors-after-first=%u actors-after=%u stale-next-after-first=%u errors-after-first=%u errors-after-second=%u\n",
		controllerSceneActorCountAfterFirstRelease, controllerSceneActorCountReleased,
		controllerListHasStaleNext ? 1u : 0u,
		controllerReleaseErrorsAfterFirst, controllerReleaseErrorsAfterSecond);
	// The pinned DLL leaves the generated actor registered when its controller
	// is released. Preserve and compare that observed behavior rather than
	// imposing a stronger cleanup contract than the oracle has.
	controllerCreateFailed = controllerCreateFailed ||
		controllerSceneActorCountAfterFirstRelease != controllerSceneActorCountCreated ||
		controllerSceneActorCountReleased != controllerSceneActorCountCreated ||
		controllerListHasStaleNext ||
		controllerReleaseErrorsAfterFirst != 0 || controllerReleaseErrorsAfterSecond != 0;
	sdk->releaseScene(*controllerScene);

	// Exercise the collision-aware controller sweep with one static obstacle.
	// The implementation change this test should catch is continuing through a
	// solid wall (or reporting no side collision) when the controller requests a
	// displacement longer than the available clear path.
	NxSceneDesc controllerObstacleSceneDesc;
	controllerObstacleSceneDesc.setToDefault();
	NxScene* const controllerObstacleScene = sdk->createScene(controllerObstacleSceneDesc);
	if(!controllerObstacleScene)
		return nxFail("controller obstacle scene creation failed");
	NxBoxShapeDesc controllerObstacleShape;
	controllerObstacleShape.dimensions = NxVec3(0.5f, 1.0f, 1.0f);
	NxActorDesc controllerObstacleActorDesc;
	controllerObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	controllerObstacleActorDesc.shapes.pushBack(&controllerObstacleShape);
	NxActor* const controllerObstacleActor =
		controllerObstacleScene->createActor(controllerObstacleActorDesc);
	alignas(4) unsigned char obstacleControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(obstacleControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(obstacleControllerDescStorage + 0x34) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(obstacleControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const obstacleController = controllerObstacleScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(obstacleControllerDescStorage));
	if(!controllerObstacleActor || !obstacleController)
		return nxFail("controller obstacle fixture setup failed");
	NxU32 obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 obstacleDisplacement(2.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(obstacleController)->move(
		obstacleDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& obstacleControllerPosition =
		reinterpret_cast<NxControllerProbe*>(obstacleController)->getPosition();
	printf("simulation controller-obstacle position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(obstacleControllerPosition.x), nxFloatBits(obstacleControllerPosition.y),
		nxFloatBits(obstacleControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(obstacleControllerPosition.x) != 0x3f000000 ||
		nxFloatBits(obstacleControllerPosition.y) != 0 ||
		nxFloatBits(obstacleControllerPosition.z) != 0 || obstacleCollisionFlags != 4;
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(obstacleController)->move(
		obstacleDisplacement, 0, 0.001f, obstacleCollisionFlags);
	const NxVec3& filteredControllerPosition =
		reinterpret_cast<NxControllerProbe*>(obstacleController)->getPosition();
	printf("simulation controller-obstacle filtered position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(filteredControllerPosition.x), nxFloatBits(filteredControllerPosition.y),
		nxFloatBits(filteredControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(filteredControllerPosition.x) != 0x40200000 ||
		nxFloatBits(filteredControllerPosition.y) != 0 ||
		nxFloatBits(filteredControllerPosition.z) != 0 || obstacleCollisionFlags != 0;
	// A downward component keeps the controller grounded while it approaches a
	// low box. The descriptor enables a half-unit step with +Y as the up axis.
	NxSceneDesc stepControllerSceneDesc;
	stepControllerSceneDesc.setToDefault();
	NxScene* const stepControllerScene = sdk->createScene(stepControllerSceneDesc);
	if(!stepControllerScene)
		return nxFail("step controller scene creation failed");
	NxBoxShapeDesc stepFloorShape;
	stepFloorShape.dimensions = NxVec3(10.0f, 0.5f, 10.0f);
	NxActorDesc stepFloorActorDesc;
	stepFloorActorDesc.globalPose.t = NxVec3(0.0f, -0.5f, 0.0f);
	stepFloorActorDesc.shapes.pushBack(&stepFloorShape);
	NxActor* const stepFloorActor = stepControllerScene->createActor(stepFloorActorDesc);
	NxBoxShapeDesc stepObstacleShape;
	stepObstacleShape.dimensions = NxVec3(0.5f, 0.1f, 1.0f);
	NxActorDesc stepObstacleActorDesc;
	stepObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.1f, 0.0f);
	stepObstacleActorDesc.shapes.pushBack(&stepObstacleShape);
	NxActor* const stepObstacleActor = stepControllerScene->createActor(stepObstacleActorDesc);
	alignas(4) unsigned char stepControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x0c) = nxFloatBits(0.0f);
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x10) = nxFloatBits(0.8f);
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x1c) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x20) = 0;
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(stepControllerDescStorage + 0x2c) = nxFloatBits(0.5f);
	NxController* const stepController = stepControllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(stepControllerDescStorage));
	if(!stepFloorActor || !stepObstacleActor || !stepController)
		return nxFail("step controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 stepDisplacement(2.0f, -0.5f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(stepController)->move(
		stepDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& stepControllerPosition =
		reinterpret_cast<NxControllerProbe*>(stepController)->getPosition();
	printf("simulation controller-obstacle step position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(stepControllerPosition.x), nxFloatBits(stepControllerPosition.y),
		nxFloatBits(stepControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(stepControllerPosition.x) != 0x3f000000 ||
		nxFloatBits(stepControllerPosition.y) != 0x3f000000 ||
		nxFloatBits(stepControllerPosition.z) != 0 || obstacleCollisionFlags != 5;
	stepControllerScene->releaseController(*stepController);
	sdk->releaseScene(*stepControllerScene);
	// Isolate the grounded low-obstacle response from the earlier controller
	// fixture, using a fresh scene with no old controller actor.
	NxSceneDesc stepUpSceneDesc;
	stepUpSceneDesc.setToDefault();
	NxScene* const stepUpScene = sdk->createScene(stepUpSceneDesc);
	if(!stepUpScene)
		return nxFail("step-response controller scene creation failed");
	NxBoxShapeDesc stepUpFloorShape;
	stepUpFloorShape.dimensions = NxVec3(10.0f, 0.5f, 10.0f);
	NxActorDesc stepUpFloorActorDesc;
	stepUpFloorActorDesc.globalPose.t = NxVec3(0.0f, -0.5f, 0.0f);
	stepUpFloorActorDesc.shapes.pushBack(&stepUpFloorShape);
	NxActor* const stepUpFloorActor = stepUpScene->createActor(stepUpFloorActorDesc);
	NxBoxShapeDesc stepUpObstacleShape;
	stepUpObstacleShape.dimensions = NxVec3(0.5f, 0.1f, 1.0f);
	NxActorDesc stepUpObstacleActorDesc;
	stepUpObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.1f, 0.0f);
	stepUpObstacleActorDesc.shapes.pushBack(&stepUpObstacleShape);
	NxActor* const stepUpObstacleActor = stepUpScene->createActor(stepUpObstacleActorDesc);
	alignas(4) unsigned char stepUpControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x0c) = nxFloatBits(0.0f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x10) = nxFloatBits(0.8f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x14) = nxFloatBits(0.0f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x1c) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x20) = 0;
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x2c) = 0;
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const stepUpController = stepUpScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(stepUpControllerDescStorage));
	if(!stepUpFloorActor || !stepUpObstacleActor || !stepUpController)
		return nxFail("step-response controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 stepUpDisplacement(2.0f, -0.5f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(stepUpController)->move(
		stepUpDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& stepUpPosition = reinterpret_cast<NxControllerProbe*>(stepUpController)->getPosition();
	printf("simulation controller-obstacle step-offset-disabled position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(stepUpPosition.x), nxFloatBits(stepUpPosition.y),
		nxFloatBits(stepUpPosition.z), obstacleCollisionFlags);
	stepUpScene->releaseController(*stepUpController);
	sdk->releaseScene(*stepUpScene);
	// A controller touching the floor while sweeping into a low wall must
	// classify the zero-time vertical floor contact, not a side hit.
	NxSceneDesc groundedSweepSceneDesc;
	groundedSweepSceneDesc.setToDefault();
	NxScene* const groundedSweepScene = sdk->createScene(groundedSweepSceneDesc);
	if(!groundedSweepScene)
		return nxFail("grounded-sweep controller scene creation failed");
	NxBoxShapeDesc groundedSweepFloorShape;
	groundedSweepFloorShape.dimensions = NxVec3(10.0f, 0.5f, 10.0f);
	NxActorDesc groundedSweepFloorActorDesc;
	groundedSweepFloorActorDesc.globalPose.t = NxVec3(0.0f, -0.5f, 0.0f);
	groundedSweepFloorActorDesc.shapes.pushBack(&groundedSweepFloorShape);
	NxActor* const groundedSweepFloorActor = groundedSweepScene->createActor(groundedSweepFloorActorDesc);
	NxBoxShapeDesc groundedSweepObstacleShape;
	groundedSweepObstacleShape.dimensions = NxVec3(0.5f, 0.1f, 1.0f);
	NxActorDesc groundedSweepObstacleActorDesc;
	groundedSweepObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.1f, 0.0f);
	groundedSweepObstacleActorDesc.shapes.pushBack(&groundedSweepObstacleShape);
	NxActor* const groundedSweepObstacleActor = groundedSweepScene->createActor(groundedSweepObstacleActorDesc);
	alignas(4) unsigned char groundedSweepControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x0c) = nxFloatBits(0.0f);
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x10) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x14) = nxFloatBits(0.0f);
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x1c) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x20) = 0;
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x2c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(groundedSweepControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const groundedSweepController = groundedSweepScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(groundedSweepControllerDescStorage));
	if(!groundedSweepFloorActor || !groundedSweepObstacleActor || !groundedSweepController)
		return nxFail("grounded-sweep controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 groundedSweepDisplacement(2.0f, -0.1f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(groundedSweepController)->move(
		groundedSweepDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& groundedSweepPosition = reinterpret_cast<NxControllerProbe*>(groundedSweepController)->getPosition();
	printf("simulation controller-obstacle grounded-sweep position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(groundedSweepPosition.x), nxFloatBits(groundedSweepPosition.y),
		nxFloatBits(groundedSweepPosition.z), obstacleCollisionFlags);
	groundedSweepScene->releaseController(*groundedSweepController);
	sdk->releaseScene(*groundedSweepScene);
	// Pin the controller's private up-axis and step-offset state from the public
	// descriptor, then retain a blocked obstacle move as the current behavior.
	NxSceneDesc successfulStepSceneDesc;
	successfulStepSceneDesc.setToDefault();
	NxScene* const successfulStepScene = sdk->createScene(successfulStepSceneDesc);
	if(!successfulStepScene)
		return nxFail("successful-step controller scene creation failed");
	NxBoxShapeDesc successfulStepFloorShape;
	successfulStepFloorShape.dimensions = NxVec3(10.0f, 0.5f, 10.0f);
	NxActorDesc successfulStepFloorActorDesc;
	successfulStepFloorActorDesc.globalPose.t = NxVec3(0.0f, -10.5f, 0.0f);
	successfulStepFloorActorDesc.shapes.pushBack(&successfulStepFloorShape);
	NxActor* const successfulStepFloorActor = successfulStepScene->createActor(
		successfulStepFloorActorDesc);
	NxBoxShapeDesc successfulStepObstacleShape;
	successfulStepObstacleShape.dimensions = NxVec3(0.5f, 0.1f, 1.0f);
	NxActorDesc successfulStepObstacleActorDesc;
	successfulStepObstacleActorDesc.globalPose.t = NxVec3(2.5f, 0.1f, 0.0f);
	successfulStepObstacleActorDesc.shapes.pushBack(&successfulStepObstacleShape);
	NxActor* const successfulStepObstacleActor = successfulStepScene->createActor(
		successfulStepObstacleActorDesc);
	alignas(4) unsigned char successfulStepControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(successfulStepControllerDescStorage + 0x10) = nxFloatBits(0.55f);
	*reinterpret_cast<NxU32*>(successfulStepControllerDescStorage + 0x1c) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(successfulStepControllerDescStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(successfulStepControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(successfulStepControllerDescStorage + 0x2c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(successfulStepControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(successfulStepControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(successfulStepControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const successfulStepController = successfulStepScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(successfulStepControllerDescStorage));
	if(!successfulStepFloorActor || !successfulStepObstacleActor || !successfulStepController)
		return nxFail("successful-step controller fixture setup failed");
	const unsigned char* const successfulStepControllerBytes =
		reinterpret_cast<const unsigned char*>(successfulStepController);
	printf("simulation controller-descriptor up-axis=%08x step-offset=%08x\n",
		*reinterpret_cast<const NxU32*>(successfulStepControllerBytes + 0x14),
		*reinterpret_cast<const NxU32*>(successfulStepControllerBytes + 0x20));
	controllerCreateFailed = controllerCreateFailed ||
		*reinterpret_cast<const NxU32*>(successfulStepControllerBytes + 0x14) != 1 ||
		*reinterpret_cast<const NxU32*>(successfulStepControllerBytes + 0x20) != nxFloatBits(0.5f);
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 successfulStepDisplacement(3.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(successfulStepController)->move(
		successfulStepDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& successfulStepPosition =
		reinterpret_cast<NxControllerProbe*>(successfulStepController)->getPosition();
	printf("simulation controller-obstacle step-offset move position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(successfulStepPosition.x), nxFloatBits(successfulStepPosition.y),
		nxFloatBits(successfulStepPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(successfulStepPosition.x) != 0x3fc00000 ||
		nxFloatBits(successfulStepPosition.y) != 0x3f0ccccd ||
		nxFloatBits(successfulStepPosition.z) != 0 || obstacleCollisionFlags != 4;
	successfulStepScene->releaseController(*successfulStepController);
	sdk->releaseScene(*successfulStepScene);
	// A grounded +Y box controller requests horizontal and downward motion
	// toward a low box. The pinned oracle treats this as separate vertical and
	// horizontal probes and blocks at the near side; successful step-up remains
	// a separate, unverified path.
	NxSceneDesc stepOverSceneDesc;
	stepOverSceneDesc.setToDefault();
	NxScene* const stepOverScene = sdk->createScene(stepOverSceneDesc);
	if(!stepOverScene)
		return nxFail("step-over controller scene creation failed");
	NxBoxShapeDesc stepOverFloorShape;
	stepOverFloorShape.dimensions = NxVec3(10.0f, 0.5f, 10.0f);
	NxActorDesc stepOverFloorActorDesc;
	stepOverFloorActorDesc.globalPose.t = NxVec3(0.0f, -0.5f, 0.0f);
	stepOverFloorActorDesc.shapes.pushBack(&stepOverFloorShape);
	NxActor* const stepOverFloorActor = stepOverScene->createActor(stepOverFloorActorDesc);
	NxBoxShapeDesc stepOverObstacleShape;
	stepOverObstacleShape.dimensions = NxVec3(0.5f, 0.1f, 1.0f);
	NxActorDesc stepOverObstacleActorDesc;
	stepOverObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.1f, 0.0f);
	stepOverObstacleActorDesc.shapes.pushBack(&stepOverObstacleShape);
	NxActor* const stepOverObstacleActor = stepOverScene->createActor(stepOverObstacleActorDesc);
	alignas(4) unsigned char stepOverControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(stepOverControllerDescStorage + 0x10) = nxFloatBits(0.8f);
	*reinterpret_cast<NxU32*>(stepOverControllerDescStorage + 0x1c) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(stepOverControllerDescStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(stepOverControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(stepOverControllerDescStorage + 0x2c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepOverControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepOverControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepOverControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const stepOverController = stepOverScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(stepOverControllerDescStorage));
	if(!stepOverFloorActor || !stepOverObstacleActor || !stepOverController)
		return nxFail("step-over controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 stepOverDisplacement(3.0f, -0.5f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(stepOverController)->move(
		stepOverDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& stepOverPosition =
		reinterpret_cast<NxControllerProbe*>(stepOverController)->getPosition();
	printf("simulation controller-grounded-step-probe position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(stepOverPosition.x), nxFloatBits(stepOverPosition.y),
		nxFloatBits(stepOverPosition.z), obstacleCollisionFlags);
	stepOverScene->releaseController(*stepOverController);
	// Exercise the same grounded low-obstacle probe with +Z as the controller's
	// up axis. This keeps the geometry isolated while checking that the resolver
	// selects its probe axis from the descriptor rather than assuming +Y.
	NxSceneDesc zUpStepSceneDesc;
	zUpStepSceneDesc.setToDefault();
	NxScene* const zUpStepScene = sdk->createScene(zUpStepSceneDesc);
	if(!zUpStepScene)
		return nxFail("Z-up step-probe scene creation failed");
	NxBoxShapeDesc zUpFloorShape;
	zUpFloorShape.dimensions = NxVec3(10.0f, 10.0f, 0.5f);
	NxActorDesc zUpFloorActorDesc;
	zUpFloorActorDesc.globalPose.t = NxVec3(0.0f, 0.0f, -0.5f);
	zUpFloorActorDesc.shapes.pushBack(&zUpFloorShape);
	NxActor* const zUpFloorActor = zUpStepScene->createActor(zUpFloorActorDesc);
	NxBoxShapeDesc zUpObstacleShape;
	zUpObstacleShape.dimensions = NxVec3(0.5f, 1.0f, 0.1f);
	NxActorDesc zUpObstacleActorDesc;
	zUpObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.1f);
	zUpObstacleActorDesc.shapes.pushBack(&zUpObstacleShape);
	NxActor* const zUpObstacleActor = zUpStepScene->createActor(zUpObstacleActorDesc);
	alignas(4) unsigned char zUpStepControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(zUpStepControllerDescStorage + 0x14) = nxFloatBits(0.8f);
	*reinterpret_cast<NxU32*>(zUpStepControllerDescStorage + 0x18) = 2;
	*reinterpret_cast<NxU32*>(zUpStepControllerDescStorage + 0x1c) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(zUpStepControllerDescStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(zUpStepControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(zUpStepControllerDescStorage + 0x2c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(zUpStepControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(zUpStepControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(zUpStepControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const zUpStepController = zUpStepScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(zUpStepControllerDescStorage));
	if(!zUpFloorActor || !zUpObstacleActor || !zUpStepController)
		return nxFail("Z-up step-probe fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 zUpStepDisplacement(3.0f, 0.0f, -0.5f);
	reinterpret_cast<NxControllerProbe*>(zUpStepController)->move(
		zUpStepDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& zUpStepPosition =
		reinterpret_cast<NxControllerProbe*>(zUpStepController)->getPosition();
	printf("simulation controller-grounded-zup-step-probe position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(zUpStepPosition.x), nxFloatBits(zUpStepPosition.y),
		nxFloatBits(zUpStepPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(zUpStepPosition.x) != 0x40400000 ||
		nxFloatBits(zUpStepPosition.y) != 0 ||
		nxFloatBits(zUpStepPosition.z) != 0x3f000000 || obstacleCollisionFlags != 4;
	zUpStepScene->releaseController(*zUpStepController);
	sdk->releaseScene(*zUpStepScene);
	// Raise the stored threshold above a unit contact-normal component to pin
	// the high-threshold response after a downward sweep with step probing on.
	// Keep this controller in its own scene so generated controller bodies do
	// not become obstacles for one another in the candidate's broadphase path.
	alignas(4) unsigned char highThresholdControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(highThresholdControllerDescStorage + 0x10) = nxFloatBits(0.8f);
	*reinterpret_cast<NxU32*>(highThresholdControllerDescStorage + 0x1c) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(highThresholdControllerDescStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(highThresholdControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(highThresholdControllerDescStorage + 0x2c) = nxFloatBits(2.0f);
	*reinterpret_cast<NxU32*>(highThresholdControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(highThresholdControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(highThresholdControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxSceneDesc highThresholdSceneDesc;
	highThresholdSceneDesc.setToDefault();
	NxScene* const highThresholdScene = sdk->createScene(highThresholdSceneDesc);
	if(!highThresholdScene)
		return nxFail("high-threshold controller scene creation failed");
	NxBoxShapeDesc highThresholdFloorShape;
	highThresholdFloorShape.dimensions = NxVec3(10.0f, 0.5f, 10.0f);
	NxActorDesc highThresholdFloorActorDesc;
	highThresholdFloorActorDesc.globalPose.t = NxVec3(0.0f, -0.5f, 0.0f);
	highThresholdFloorActorDesc.shapes.pushBack(&highThresholdFloorShape);
	NxActor* const highThresholdFloor = highThresholdScene->createActor(highThresholdFloorActorDesc);
	NxBoxShapeDesc highThresholdObstacleShape;
	highThresholdObstacleShape.dimensions = NxVec3(0.5f, 0.1f, 1.0f);
	NxActorDesc highThresholdObstacleActorDesc;
	highThresholdObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.1f, 0.0f);
	highThresholdObstacleActorDesc.shapes.pushBack(&highThresholdObstacleShape);
	NxActor* const highThresholdObstacle = highThresholdScene->createActor(
		highThresholdObstacleActorDesc);
	NxController* const highThresholdController = highThresholdScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(highThresholdControllerDescStorage));
	if(!highThresholdFloor || !highThresholdObstacle || !highThresholdController)
		return nxFail("high-threshold controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(highThresholdController)->move(
		stepOverDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& highThresholdPosition =
		reinterpret_cast<NxControllerProbe*>(highThresholdController)->getPosition();
	printf("simulation controller-grounded-high-threshold position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(highThresholdPosition.x), nxFloatBits(highThresholdPosition.y),
		nxFloatBits(highThresholdPosition.z), obstacleCollisionFlags);
	highThresholdScene->releaseController(*highThresholdController);
	sdk->releaseScene(*highThresholdScene);
	sdk->releaseScene(*stepOverScene);
	{
	// A controller already resting at floor height receives a tiny downward
	// probe while moving into a low box. The oracle reports a side hit only;
	// it does not add a down-collision flag from the floor contact.
	NxSceneDesc stepUpSceneDesc;
	stepUpSceneDesc.setToDefault();
	NxScene* const stepUpScene = sdk->createScene(stepUpSceneDesc);
	if(!stepUpScene)
		return nxFail("step-up controller scene creation failed");
	NxBoxShapeDesc stepUpFloorShape;
	stepUpFloorShape.dimensions = NxVec3(10.0f, 0.5f, 10.0f);
	NxActorDesc stepUpFloorActorDesc;
	stepUpFloorActorDesc.globalPose.t = NxVec3(0.0f, -0.5f, 0.0f);
	stepUpFloorActorDesc.shapes.pushBack(&stepUpFloorShape);
	NxActor* const stepUpFloorActor = stepUpScene->createActor(stepUpFloorActorDesc);
	NxBoxShapeDesc stepUpObstacleShape;
	stepUpObstacleShape.dimensions = NxVec3(0.5f, 0.05f, 1.0f);
	NxActorDesc stepUpObstacleActorDesc;
	stepUpObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.05f, 0.0f);
	stepUpObstacleActorDesc.shapes.pushBack(&stepUpObstacleShape);
	NxActor* const stepUpObstacleActor = stepUpScene->createActor(stepUpObstacleActorDesc);
	alignas(4) unsigned char stepUpControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x0c) = nxFloatBits(0.0f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x10) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x14) = nxFloatBits(0.0f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x1c) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x2c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(stepUpControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const stepUpController = stepUpScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(stepUpControllerDescStorage));
	if(!stepUpFloorActor || !stepUpObstacleActor || !stepUpController)
		return nxFail("step-up controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxReal shortProbeDown = -0.001f;
	const NxVec3 lowStepDisplacement(3.0f, shortProbeDown, 0.0f);
	reinterpret_cast<NxControllerProbe*>(stepUpController)->move(
		lowStepDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& lowStepPosition = reinterpret_cast<NxControllerProbe*>(
		stepUpController)->getPosition();
	printf("simulation controller-grounded-short-step-probe position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(lowStepPosition.x), nxFloatBits(lowStepPosition.y),
		nxFloatBits(lowStepPosition.z), obstacleCollisionFlags);
	stepUpScene->releaseController(*stepUpController);
	sdk->releaseScene(*stepUpScene);
	}
	NxSceneDesc slideSceneDesc;
	slideSceneDesc.setToDefault();
	NxScene* const slideScene = sdk->createScene(slideSceneDesc);
	if(!slideScene)
		return nxFail("sliding controller scene creation failed");
	NxBoxShapeDesc slideObstacleShape;
	slideObstacleShape.dimensions = NxVec3(0.5f, 1.0f, 1.0f);
	NxActorDesc slideObstacleActorDesc;
	slideObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	slideObstacleActorDesc.shapes.pushBack(&slideObstacleShape);
	NxActor* const slideObstacleActor = slideScene->createActor(slideObstacleActorDesc);
	alignas(4) unsigned char slideControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(slideControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slideControllerDescStorage + 0x34) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(slideControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const slideController = slideScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(slideControllerDescStorage));
	if(!slideObstacleActor || !slideController)
		return nxFail("sliding controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 diagonalDisplacement(2.0f, 0.0f, 1.0f);
	reinterpret_cast<NxControllerProbe*>(slideController)->move(
		diagonalDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& slideControllerPosition =
		reinterpret_cast<NxControllerProbe*>(slideController)->getPosition();
	printf("simulation controller-obstacle slide position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(slideControllerPosition.x), nxFloatBits(slideControllerPosition.y),
		nxFloatBits(slideControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(slideControllerPosition.x) != 0x3f000000 ||
		nxFloatBits(slideControllerPosition.y) != 0 ||
		nxFloatBits(slideControllerPosition.z) != 0x3f800000 || obstacleCollisionFlags != 4;
	slideScene->releaseController(*slideController);
	sdk->releaseScene(*slideScene);
	NxSceneDesc overlapSceneDesc;
	overlapSceneDesc.setToDefault();
	NxScene* const overlapScene = sdk->createScene(overlapSceneDesc);
	if(!overlapScene)
		return nxFail("controller initial-overlap scene creation failed");
	NxBoxShapeDesc overlapObstacleShape;
	overlapObstacleShape.dimensions = NxVec3(0.5f, 1.0f, 1.0f);
	NxActorDesc overlapObstacleActorDesc;
	overlapObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	overlapObstacleActorDesc.shapes.pushBack(&overlapObstacleShape);
	NxActor* const overlapObstacleActor = overlapScene->createActor(overlapObstacleActorDesc);
	alignas(4) unsigned char overlapControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(overlapControllerDescStorage + 0x0c) = nxFloatBits(0.75f);
	*reinterpret_cast<NxU32*>(overlapControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(overlapControllerDescStorage + 0x34) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(overlapControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const overlapController = overlapScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(overlapControllerDescStorage));
	if(!overlapObstacleActor || !overlapController)
		return nxFail("controller initial-overlap fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 escapeDisplacement(-0.5f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(overlapController)->move(
		escapeDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& escapedControllerPosition =
		reinterpret_cast<NxControllerProbe*>(overlapController)->getPosition();
	printf("simulation controller-initial-overlap escape position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(escapedControllerPosition.x), nxFloatBits(escapedControllerPosition.y),
		nxFloatBits(escapedControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(escapedControllerPosition.x) != 0x3e800000 ||
		nxFloatBits(escapedControllerPosition.y) != 0 ||
		nxFloatBits(escapedControllerPosition.z) != 0 || obstacleCollisionFlags != 0;
	overlapScene->releaseController(*overlapController);
	NxController* const overlapControllerInward = overlapScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(overlapControllerDescStorage));
	if(!overlapControllerInward)
		return nxFail("controller inward-overlap fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 inwardDisplacement(0.5f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(overlapControllerInward)->move(
		inwardDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& inwardControllerPosition =
		reinterpret_cast<NxControllerProbe*>(overlapControllerInward)->getPosition();
	printf("simulation controller-initial-overlap inward position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(inwardControllerPosition.x), nxFloatBits(inwardControllerPosition.y),
		nxFloatBits(inwardControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(inwardControllerPosition.x) != 0x3f400000 ||
		nxFloatBits(inwardControllerPosition.y) != 0 ||
		nxFloatBits(inwardControllerPosition.z) != 0 || obstacleCollisionFlags != 4;
	overlapScene->releaseController(*overlapControllerInward);
	sdk->releaseScene(*overlapScene);
	// The rotated-box SAT path must handle a controller that starts inside the
	// obstacle and moves out through its expanded bound.
	NxSceneDesc rotatedOverlapSceneDesc;
	rotatedOverlapSceneDesc.setToDefault();
	NxScene* const rotatedOverlapScene = sdk->createScene(rotatedOverlapSceneDesc);
	if(!rotatedOverlapScene)
		return nxFail("rotated initial-overlap scene creation failed");
	NxBoxShapeDesc rotatedOverlapObstacleShape;
	rotatedOverlapObstacleShape.dimensions = NxVec3(1.0f, 0.5f, 0.1f);
	NxActorDesc rotatedOverlapObstacleActorDesc;
	rotatedOverlapObstacleActorDesc.globalPose.M =
		NxMat33(NxQuat(45.0f, NxVec3(0.0f, 1.0f, 0.0f)));
	rotatedOverlapObstacleActorDesc.shapes.pushBack(&rotatedOverlapObstacleShape);
	NxActor* const rotatedOverlapObstacleActor = rotatedOverlapScene->createActor(
		rotatedOverlapObstacleActorDesc);
	alignas(4) unsigned char rotatedOverlapControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(rotatedOverlapControllerDescStorage + 0x30) = nxFloatBits(0.1f);
	*reinterpret_cast<NxU32*>(rotatedOverlapControllerDescStorage + 0x34) = nxFloatBits(0.25f);
	*reinterpret_cast<NxU32*>(rotatedOverlapControllerDescStorage + 0x38) = nxFloatBits(0.1f);
	NxController* const rotatedOverlapController = rotatedOverlapScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(rotatedOverlapControllerDescStorage));
	if(!rotatedOverlapObstacleActor || !rotatedOverlapController)
		return nxFail("rotated initial-overlap controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 rotatedEscapeDisplacement(1.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(rotatedOverlapController)->move(
		rotatedEscapeDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& rotatedEscapedPosition =
		reinterpret_cast<NxControllerProbe*>(rotatedOverlapController)->getPosition();
	printf("simulation controller-rotated-initial-overlap escape position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(rotatedEscapedPosition.x), nxFloatBits(rotatedEscapedPosition.y),
		nxFloatBits(rotatedEscapedPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(rotatedEscapedPosition.x) != 0x3f800000 ||
		nxFloatBits(rotatedEscapedPosition.y) != 0 ||
		nxFloatBits(rotatedEscapedPosition.z) != 0 || obstacleCollisionFlags != 0;
	rotatedOverlapScene->releaseController(*rotatedOverlapController);
	NxController* const rotatedOverlapControllerReverse = rotatedOverlapScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(rotatedOverlapControllerDescStorage));
	if(!rotatedOverlapControllerReverse)
		return nxFail("rotated reverse-overlap controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 rotatedEscapeDisplacementReverse(-1.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(rotatedOverlapControllerReverse)->move(
		rotatedEscapeDisplacementReverse, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& rotatedEscapedPositionReverse =
		reinterpret_cast<NxControllerProbe*>(rotatedOverlapControllerReverse)->getPosition();
	printf("simulation controller-rotated-initial-overlap reverse position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(rotatedEscapedPositionReverse.x), nxFloatBits(rotatedEscapedPositionReverse.y),
		nxFloatBits(rotatedEscapedPositionReverse.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(rotatedEscapedPositionReverse.x) != 0xbf800000 ||
		nxFloatBits(rotatedEscapedPositionReverse.y) != 0 ||
		nxFloatBits(rotatedEscapedPositionReverse.z) != 0 || obstacleCollisionFlags != 0;
	rotatedOverlapScene->releaseController(*rotatedOverlapControllerReverse);
	sdk->releaseScene(*rotatedOverlapScene);
	NxBoxShapeDesc verticalObstacleShape;
	verticalObstacleShape.dimensions = NxVec3(1.0f, 0.5f, 1.0f);
	NxActorDesc verticalObstacleActorDesc;
	verticalObstacleActorDesc.globalPose.t = NxVec3(4.0f, 2.0f, 0.0f);
	verticalObstacleActorDesc.shapes.pushBack(&verticalObstacleShape);
	NxActor* const verticalObstacleActor =
		controllerObstacleScene->createActor(verticalObstacleActorDesc);
	alignas(4) unsigned char verticalControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(verticalControllerDescStorage + 0x0c) = nxFloatBits(4.0f);
	*reinterpret_cast<NxU32*>(verticalControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(verticalControllerDescStorage + 0x34) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(verticalControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const verticalController = controllerObstacleScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(verticalControllerDescStorage));
	if(!verticalObstacleActor || !verticalController)
		return nxFail("vertical controller obstacle fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 upwardDisplacement(0.0f, 2.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(verticalController)->move(
		upwardDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& verticalControllerPosition =
		reinterpret_cast<NxControllerProbe*>(verticalController)->getPosition();
	printf("simulation controller-obstacle vertical position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(verticalControllerPosition.x), nxFloatBits(verticalControllerPosition.y),
		nxFloatBits(verticalControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(verticalControllerPosition.x) != 0x40800000 ||
		nxFloatBits(verticalControllerPosition.y) != 0x3f000000 ||
		nxFloatBits(verticalControllerPosition.z) != 0 || obstacleCollisionFlags != 1;
	controllerObstacleScene->releaseController(*verticalController);
	NxBoxShapeDesc triggerObstacleShape;
	triggerObstacleShape.dimensions = NxVec3(0.5f, 1.0f, 1.0f);
	triggerObstacleShape.shapeFlags |= NX_TRIGGER_ENABLE;
	NxActorDesc triggerObstacleActorDesc;
	triggerObstacleActorDesc.globalPose.t = NxVec3(8.5f, 0.0f, 0.0f);
	triggerObstacleActorDesc.shapes.pushBack(&triggerObstacleShape);
	NxActor* const triggerObstacleActor =
		controllerObstacleScene->createActor(triggerObstacleActorDesc);
	alignas(4) unsigned char triggerControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(triggerControllerDescStorage + 0x0c) = nxFloatBits(7.0f);
	*reinterpret_cast<NxU32*>(triggerControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(triggerControllerDescStorage + 0x34) = nxFloatBits(1.0f);
	*reinterpret_cast<NxU32*>(triggerControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const triggerController = controllerObstacleScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(triggerControllerDescStorage));
	if(!triggerObstacleActor || !triggerController)
		return nxFail("trigger controller obstacle fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(triggerController)->move(
		obstacleDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& triggerControllerPosition =
		reinterpret_cast<NxControllerProbe*>(triggerController)->getPosition();
	printf("simulation controller-obstacle trigger position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(triggerControllerPosition.x), nxFloatBits(triggerControllerPosition.y),
		nxFloatBits(triggerControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(triggerControllerPosition.x) != 0x40f00000 ||
		nxFloatBits(triggerControllerPosition.y) != 0 ||
		nxFloatBits(triggerControllerPosition.z) != 0 || obstacleCollisionFlags != 4;
	controllerObstacleScene->releaseController(*triggerController);
	controllerObstacleScene->releaseController(*obstacleController);
	sdk->releaseScene(*controllerObstacleScene);

	// A rotated thin box has an axis-aligned world bound that contains a large
	// empty corner. Moving along X at an offset through that corner distinguishes
	// a true controller sweep from treating every obstacle as its world AABB.
	NxSceneDesc rotatedControllerSceneDesc;
	rotatedControllerSceneDesc.setToDefault();
	NxScene* const rotatedControllerScene = sdk->createScene(rotatedControllerSceneDesc);
	if(!rotatedControllerScene)
		return nxFail("rotated controller scene creation failed");
	NxBoxShapeDesc rotatedControllerObstacleShape;
	rotatedControllerObstacleShape.dimensions = NxVec3(1.0f, 0.5f, 0.1f);
	NxActorDesc rotatedControllerObstacleActorDesc;
	rotatedControllerObstacleActorDesc.globalPose.M =
		NxMat33(NxQuat(45.0f, NxVec3(0.0f, 1.0f, 0.0f)));
	rotatedControllerObstacleActorDesc.shapes.pushBack(&rotatedControllerObstacleShape);
	NxActor* const rotatedControllerObstacle = rotatedControllerScene->createActor(
		rotatedControllerObstacleActorDesc);
	alignas(4) unsigned char rotatedControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(rotatedControllerDescStorage + 0x0c) = nxFloatBits(-2.0f);
	*reinterpret_cast<NxU32*>(rotatedControllerDescStorage + 0x14) = nxFloatBits(0.8f);
	*reinterpret_cast<NxU32*>(rotatedControllerDescStorage + 0x30) = nxFloatBits(0.1f);
	*reinterpret_cast<NxU32*>(rotatedControllerDescStorage + 0x34) = nxFloatBits(0.25f);
	*reinterpret_cast<NxU32*>(rotatedControllerDescStorage + 0x38) = nxFloatBits(0.1f);
	NxController* const rotatedController = rotatedControllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(rotatedControllerDescStorage));
	if(!rotatedControllerObstacle || !rotatedController)
		return nxFail("rotated controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 rotatedObstacleDisplacement(4.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(rotatedController)->move(
		rotatedObstacleDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& rotatedControllerPosition =
		reinterpret_cast<NxControllerProbe*>(rotatedController)->getPosition();
	printf("simulation controller-obstacle rotated-corner position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(rotatedControllerPosition.x), nxFloatBits(rotatedControllerPosition.y),
		nxFloatBits(rotatedControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(rotatedControllerPosition.x) != 0xbf50704e ||
		nxFloatBits(rotatedControllerPosition.y) != 0 ||
		nxFloatBits(rotatedControllerPosition.z) != 0x3f4ccccd || obstacleCollisionFlags != 4;
	rotatedControllerScene->releaseController(*rotatedController);
	sdk->releaseScene(*rotatedControllerScene);

	// A sphere's expanded world AABB has square corners. This diagonal path
	// clips that empty corner for a box controller and a spherical obstacle.
	NxSceneDesc sphereControllerSceneDesc;
	sphereControllerSceneDesc.setToDefault();
	NxScene* const sphereControllerScene = sdk->createScene(sphereControllerSceneDesc);
	if(!sphereControllerScene)
		return nxFail("sphere controller scene creation failed");
	NxSphereShapeDesc sphereControllerObstacleShape;
	sphereControllerObstacleShape.radius = 0.5f;
	NxActorDesc sphereControllerObstacleActorDesc;
	sphereControllerObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	sphereControllerObstacleActorDesc.shapes.pushBack(&sphereControllerObstacleShape);
	NxActor* const sphereControllerObstacle = sphereControllerScene->createActor(
		sphereControllerObstacleActorDesc);
	alignas(4) unsigned char sphereControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(sphereControllerDescStorage + 0x0c) = nxFloatBits(-2.0f);
	*reinterpret_cast<NxU32*>(sphereControllerDescStorage + 0x10) = nxFloatBits(0.8f);
	*reinterpret_cast<NxU32*>(sphereControllerDescStorage + 0x14) = nxFloatBits(0.8f);
	*reinterpret_cast<NxU32*>(sphereControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(sphereControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(sphereControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const sphereController = sphereControllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(sphereControllerDescStorage));
	if(!sphereControllerObstacle || !sphereController)
		return nxFail("sphere controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 sphereObstacleDisplacement(4.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(sphereController)->move(
		sphereObstacleDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& sphereControllerPosition =
		reinterpret_cast<NxControllerProbe*>(sphereController)->getPosition();
	printf("simulation controller-obstacle sphere-corner position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(sphereControllerPosition.x), nxFloatBits(sphereControllerPosition.y),
		nxFloatBits(sphereControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(sphereControllerPosition.x) != 0x40000000 ||
		nxFloatBits(sphereControllerPosition.y) != 0x3f4ccccd ||
		nxFloatBits(sphereControllerPosition.z) != 0x3f4ccccd || obstacleCollisionFlags != 0;
	sphereControllerScene->releaseController(*sphereController);
	sdk->releaseScene(*sphereControllerScene);

	// The mesh bounds cover this controller path, but its triangular surface lies
	// beyond the moving box's rounded-free corner in the YZ plane.
	const NxPoint controllerMeshPoints[] = {
		NxPoint(0.0f, 0.0f, 0.0f), NxPoint(0.0f, 0.0f, 1.0f),
		NxPoint(0.0f, 1.0f, 0.0f), NxPoint(0.0f, 0.5f, 0.5f)};
	const NxU32 controllerMeshIndices[] = {0, 1, 3, 0, 3, 2};
	NxTriangleMeshDesc controllerMeshDesc;
	controllerMeshDesc.numVertices = 4;
	controllerMeshDesc.numTriangles = 2;
	controllerMeshDesc.pointStrideBytes = sizeof(NxPoint);
	controllerMeshDesc.triangleStrideBytes = 3 * sizeof(NxU32);
	controllerMeshDesc.points = controllerMeshPoints;
	controllerMeshDesc.triangles = controllerMeshIndices;
	NxTriangleMesh* const controllerMesh = sdk->createTriangleMesh(controllerMeshDesc);
	if(!controllerMesh)
		return nxFail("controller mesh fixture cooking failed");
	NxSceneDesc meshControllerSceneDesc;
	meshControllerSceneDesc.setToDefault();
	NxScene* const meshControllerScene = sdk->createScene(meshControllerSceneDesc);
	if(!meshControllerScene)
		return nxFail("mesh controller scene creation failed");
	NxTriangleMeshShapeDesc meshControllerObstacleShape;
	meshControllerObstacleShape.meshData = controllerMesh;
	NxActorDesc meshControllerObstacleActorDesc;
	meshControllerObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	meshControllerObstacleActorDesc.shapes.pushBack(&meshControllerObstacleShape);
	NxActor* const meshControllerObstacle = meshControllerScene->createActor(
		meshControllerObstacleActorDesc);
	alignas(4) unsigned char meshControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(meshControllerDescStorage + 0x0c) = nxFloatBits(-2.0f);
	*reinterpret_cast<NxU32*>(meshControllerDescStorage + 0x10) = nxFloatBits(1.1f);
	*reinterpret_cast<NxU32*>(meshControllerDescStorage + 0x14) = nxFloatBits(1.1f);
	*reinterpret_cast<NxU32*>(meshControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(meshControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(meshControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const meshController = meshControllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(meshControllerDescStorage));
	if(!meshControllerObstacle || !meshController)
		return nxFail("mesh controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 meshObstacleDisplacement(4.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(meshController)->move(
		meshObstacleDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& meshControllerPosition =
		reinterpret_cast<NxControllerProbe*>(meshController)->getPosition();
	printf("simulation controller-obstacle mesh-corner position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(meshControllerPosition.x), nxFloatBits(meshControllerPosition.y),
		nxFloatBits(meshControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(meshControllerPosition.x) != 0x40000000 ||
		nxFloatBits(meshControllerPosition.y) != 0x3f8ccccd ||
		nxFloatBits(meshControllerPosition.z) != 0x3f8ccccd || obstacleCollisionFlags != 0;
	meshControllerScene->releaseController(*meshController);
	sdk->releaseScene(*meshControllerScene);
	NxSceneDesc meshHitSceneDesc;
	meshHitSceneDesc.setToDefault();
	NxScene* const meshHitScene = sdk->createScene(meshHitSceneDesc);
	if(!meshHitScene)
		return nxFail("mesh-hit controller scene creation failed");
	NxActorDesc meshHitObstacleActorDesc;
	meshHitObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	meshHitObstacleActorDesc.shapes.pushBack(&meshControllerObstacleShape);
	NxActor* const meshHitObstacle = meshHitScene->createActor(meshHitObstacleActorDesc);
	alignas(4) unsigned char meshHitControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(meshHitControllerDescStorage + 0x0c) = nxFloatBits(-2.0f);
	*reinterpret_cast<NxU32*>(meshHitControllerDescStorage + 0x10) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(meshHitControllerDescStorage + 0x14) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(meshHitControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(meshHitControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(meshHitControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const meshHitController = meshHitScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(meshHitControllerDescStorage));
	if(!meshHitObstacle || !meshHitController)
		return nxFail("mesh-hit controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(meshHitController)->move(
		meshObstacleDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& meshHitControllerPosition =
		reinterpret_cast<NxControllerProbe*>(meshHitController)->getPosition();
	printf("simulation controller-obstacle mesh-hit position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(meshHitControllerPosition.x), nxFloatBits(meshHitControllerPosition.y),
		nxFloatBits(meshHitControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(meshHitControllerPosition.x) != 0x3f800000 ||
		nxFloatBits(meshHitControllerPosition.y) != 0x3e4ccccd ||
		nxFloatBits(meshHitControllerPosition.z) != 0x3e4ccccd || obstacleCollisionFlags != 4;
	meshHitScene->releaseController(*meshHitController);
	// Sweep across a rising triangle mesh face to pin the controller's slope
	// normal classification and stop pose.
	const NxPoint controllerSlopePoints[] = {
		NxPoint(0.0f, 0.0f, 0.0f), NxPoint(0.0f, 0.0f, 1.0f),
		NxPoint(1.0f, 0.5f, 0.0f), NxPoint(1.0f, 0.5f, 1.0f)};
	const NxU32 controllerSlopeIndices[] = {0, 1, 2, 1, 3, 2};
	NxTriangleMeshDesc controllerSlopeMeshDesc;
	controllerSlopeMeshDesc.numVertices = 4;
	controllerSlopeMeshDesc.numTriangles = 2;
	controllerSlopeMeshDesc.pointStrideBytes = sizeof(NxPoint);
	controllerSlopeMeshDesc.triangleStrideBytes = 3 * sizeof(NxU32);
	controllerSlopeMeshDesc.points = controllerSlopePoints;
	controllerSlopeMeshDesc.triangles = controllerSlopeIndices;
	NxTriangleMesh* const controllerSlopeMesh = sdk->createTriangleMesh(controllerSlopeMeshDesc);
	if(!controllerSlopeMesh)
		return nxFail("controller slope mesh cooking failed");
	NxSceneDesc slopeControllerSceneDesc;
	slopeControllerSceneDesc.setToDefault();
	NxScene* const slopeControllerScene = sdk->createScene(slopeControllerSceneDesc);
	if(!slopeControllerScene)
		return nxFail("slope controller scene creation failed");
	NxTriangleMeshShapeDesc slopeObstacleShape;
	slopeObstacleShape.meshData = controllerSlopeMesh;
	NxActorDesc slopeObstacleActorDesc;
	slopeObstacleActorDesc.shapes.pushBack(&slopeObstacleShape);
	NxActor* const slopeObstacle = slopeControllerScene->createActor(slopeObstacleActorDesc);
	alignas(4) unsigned char slopeControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(slopeControllerDescStorage + 0x0c) = nxFloatBits(-2.0f);
	*reinterpret_cast<NxU32*>(slopeControllerDescStorage + 0x10) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeControllerDescStorage + 0x14) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeControllerDescStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(slopeControllerDescStorage + 0x30) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(slopeControllerDescStorage + 0x34) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(slopeControllerDescStorage + 0x38) = nxFloatBits(0.2f);
	NxController* const slopeController = slopeControllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(slopeControllerDescStorage));
	if(!slopeObstacle || !slopeController)
		return nxFail("slope controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 slopeDisplacement(4.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(slopeController)->move(
		slopeDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& slopeControllerPosition =
		reinterpret_cast<NxControllerProbe*>(slopeController)->getPosition();
	printf("simulation controller-mesh-slope position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(slopeControllerPosition.x), nxFloatBits(slopeControllerPosition.y),
		nxFloatBits(slopeControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(slopeControllerPosition.x) != 0x3eccccd0 ||
		nxFloatBits(slopeControllerPosition.y) != 0x3f000000 ||
		nxFloatBits(slopeControllerPosition.z) != 0x3f000000 || obstacleCollisionFlags != 4;
	slopeControllerScene->releaseController(*slopeController);
	sdk->releaseScene(*slopeControllerScene);
	// Drive a high-threshold, step-enabled controller downward onto a sloped
	// triangle face. This gives the resolver a non-axis-aligned contact normal
	// for its conditional correction query.
	NxSceneDesc slopeCorrectionSceneDesc;
	slopeCorrectionSceneDesc.setToDefault();
	NxScene* const slopeCorrectionScene = sdk->createScene(slopeCorrectionSceneDesc);
	if(!slopeCorrectionScene)
		return nxFail("slope-correction controller scene creation failed");
	NxActorDesc slopeCorrectionObstacleDesc;
	slopeCorrectionObstacleDesc.shapes.pushBack(&slopeObstacleShape);
	NxActor* const slopeCorrectionObstacle = slopeCorrectionScene->createActor(
		slopeCorrectionObstacleDesc);
	alignas(4) unsigned char slopeCorrectionControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x0c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x10) = nxFloatBits(2.0f);
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x14) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x1c) = nxFloatBits(0.95f);
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x18) = 1;
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x2c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x30) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x34) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(slopeCorrectionControllerDescStorage + 0x38) = nxFloatBits(0.2f);
	NxController* const slopeCorrectionController = slopeCorrectionScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(slopeCorrectionControllerDescStorage));
	if(!slopeCorrectionObstacle || !slopeCorrectionController)
		return nxFail("slope-correction controller fixture setup failed");
	const NxVec3 slopeCorrectionDisplacement(0.0f, -3.0f, 0.0f);
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(slopeCorrectionController)->move(
		slopeCorrectionDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& slopeCorrectionPosition = reinterpret_cast<NxControllerProbe*>(
		slopeCorrectionController)->getPosition();
	printf("simulation controller-mesh-slope-correction position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(slopeCorrectionPosition.x), nxFloatBits(slopeCorrectionPosition.y),
		nxFloatBits(slopeCorrectionPosition.z), obstacleCollisionFlags);
	slopeCorrectionScene->releaseController(*slopeCorrectionController);
	// Use an isolated scene and disable only the slope correction threshold to
	// capture the pose after the resolver's original three probes. Controller
	// release retains its generated actor, so scene reuse would contaminate the
	// next sweep with a second controller obstacle.
	NxSceneDesc slopeBaselineSceneDesc;
	slopeBaselineSceneDesc.setToDefault();
	NxScene* const slopeBaselineScene = sdk->createScene(slopeBaselineSceneDesc);
	if(!slopeBaselineScene)
		return nxFail("slope-baseline controller scene creation failed");
	NxActorDesc slopeBaselineObstacleDesc;
	slopeBaselineObstacleDesc.shapes.pushBack(&slopeObstacleShape);
	NxActor* const slopeBaselineObstacle = slopeBaselineScene->createActor(
		slopeBaselineObstacleDesc);
	alignas(4) unsigned char slopeBaselineControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x0c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x10) = nxFloatBits(2.0f);
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x14) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x1c) = 0;
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x18) = 1;
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x2c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x30) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x34) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(slopeBaselineControllerDescStorage + 0x38) = nxFloatBits(0.2f);
	NxController* const slopeBaselineController = slopeBaselineScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(slopeBaselineControllerDescStorage));
	if(!slopeBaselineObstacle || !slopeBaselineController)
		return nxFail("slope-baseline controller fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(slopeBaselineController)->move(
		slopeCorrectionDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& slopeBaselinePosition = reinterpret_cast<NxControllerProbe*>(
		slopeBaselineController)->getPosition();
	printf("simulation controller-mesh-slope-baseline position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(slopeBaselinePosition.x), nxFloatBits(slopeBaselinePosition.y),
		nxFloatBits(slopeBaselinePosition.z), obstacleCollisionFlags);
	slopeBaselineScene->releaseController(*slopeBaselineController);
	sdk->releaseScene(*slopeBaselineScene);
	sdk->releaseScene(*slopeCorrectionScene);
	// Keep the descriptor's other axis-like word different so this scene proves
	// which field the resolver actually uses when it decomposes movement.
	NxSceneDesc slopeAxisSceneDesc;
	slopeAxisSceneDesc.setToDefault();
	NxScene* const slopeAxisScene = sdk->createScene(slopeAxisSceneDesc);
	if(!slopeAxisScene)
		return nxFail("slope-axis controller scene creation failed");
	NxActorDesc slopeAxisObstacleDesc;
	slopeAxisObstacleDesc.shapes.pushBack(&slopeObstacleShape);
	NxActor* const slopeAxisObstacle = slopeAxisScene->createActor(slopeAxisObstacleDesc);
	alignas(4) unsigned char slopeAxisControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x0c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x10) = nxFloatBits(2.0f);
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x14) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x1c) = 1;
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x18) = 0;
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x2c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x30) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x34) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(slopeAxisControllerDescStorage + 0x38) = nxFloatBits(0.2f);
	NxController* const slopeAxisController = slopeAxisScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(slopeAxisControllerDescStorage));
	if(!slopeAxisObstacle || !slopeAxisController)
		return nxFail("slope-axis controller fixture setup failed");
	const NxVec3 slopeAxisDisplacement(0.0f, -3.0f, 0.0f);
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(slopeAxisController)->move(
		slopeAxisDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& slopeAxisPosition = reinterpret_cast<NxControllerProbe*>(
		slopeAxisController)->getPosition();
	printf("simulation controller-mesh-slope-descriptor-axis position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(slopeAxisPosition.x), nxFloatBits(slopeAxisPosition.y),
		nxFloatBits(slopeAxisPosition.z), obstacleCollisionFlags);
	slopeAxisScene->releaseController(*slopeAxisController);
	sdk->releaseScene(*slopeAxisScene);
	sdk->releaseTriangleMesh(*controllerSlopeMesh);

	// Exercise the complete public path from 16-bit descriptor indices through
	// mesh cooking to the controller's triangle sweep.
	const NxU16 controllerMeshIndices16[] = {0, 1, 3, 0, 3, 2};
	NxTriangleMeshDesc controllerMesh16Desc;
	controllerMesh16Desc.numVertices = 4;
	controllerMesh16Desc.numTriangles = 2;
	controllerMesh16Desc.pointStrideBytes = sizeof(NxPoint);
	controllerMesh16Desc.triangleStrideBytes = 3 * sizeof(NxU16);
	controllerMesh16Desc.points = controllerMeshPoints;
	controllerMesh16Desc.triangles = controllerMeshIndices16;
	controllerMesh16Desc.flags = NX_MF_16_BIT_INDICES;
	NxTriangleMesh* const controllerMesh16 = sdk->createTriangleMesh(controllerMesh16Desc);
	if(!controllerMesh16)
		return nxFail("16-bit controller mesh cooking failed");
	NxSceneDesc mesh16HitSceneDesc;
	mesh16HitSceneDesc.setToDefault();
	NxScene* const mesh16HitScene = sdk->createScene(mesh16HitSceneDesc);
	if(!mesh16HitScene)
		return nxFail("16-bit mesh-hit scene creation failed");
	NxTriangleMeshShapeDesc mesh16ObstacleShape;
	mesh16ObstacleShape.meshData = controllerMesh16;
	NxActorDesc mesh16ObstacleActorDesc;
	mesh16ObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	mesh16ObstacleActorDesc.shapes.pushBack(&mesh16ObstacleShape);
	NxActor* const mesh16Obstacle = mesh16HitScene->createActor(mesh16ObstacleActorDesc);
	alignas(4) unsigned char mesh16ControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(mesh16ControllerDescStorage + 0x0c) = nxFloatBits(-2.0f);
	*reinterpret_cast<NxU32*>(mesh16ControllerDescStorage + 0x10) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(mesh16ControllerDescStorage + 0x14) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(mesh16ControllerDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(mesh16ControllerDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(mesh16ControllerDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const mesh16Controller = mesh16HitScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(mesh16ControllerDescStorage));
	if(!mesh16Obstacle || !mesh16Controller)
		return nxFail("16-bit mesh-hit fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(mesh16Controller)->move(
		meshObstacleDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& mesh16ControllerPosition =
		reinterpret_cast<NxControllerProbe*>(mesh16Controller)->getPosition();
	const NxU32 mesh16ControllerIndexFormat = controllerMesh16->getFormat(0, NX_ARRAY_TRIANGLES);
	const NxU32 mesh16ControllerIndexStride = controllerMesh16->getStride(0, NX_ARRAY_TRIANGLES);
	printf("simulation controller-obstacle mesh16-hit source_index_bits=16 index_format=%u index_stride=%u position=%08x.%08x.%08x flags=%08x\n",
		mesh16ControllerIndexFormat, mesh16ControllerIndexStride,
		nxFloatBits(mesh16ControllerPosition.x), nxFloatBits(mesh16ControllerPosition.y),
		nxFloatBits(mesh16ControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed || mesh16ControllerIndexFormat != 4 ||
		mesh16ControllerIndexStride != 3 * sizeof(NxU32) ||
		nxFloatBits(mesh16ControllerPosition.x) != 0x3f800000 ||
		nxFloatBits(mesh16ControllerPosition.y) != 0x3e4ccccd ||
		nxFloatBits(mesh16ControllerPosition.z) != 0x3e4ccccd || obstacleCollisionFlags != 4;
	mesh16HitScene->releaseController(*mesh16Controller);
	sdk->releaseScene(*mesh16HitScene);
	sdk->releaseTriangleMesh(*controllerMesh16);
	alignas(4) unsigned char meshOverlapOutDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(meshOverlapOutDescStorage + 0x0c) = nxFloatBits(1.5f);
	*reinterpret_cast<NxU32*>(meshOverlapOutDescStorage + 0x10) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(meshOverlapOutDescStorage + 0x14) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(meshOverlapOutDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(meshOverlapOutDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(meshOverlapOutDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const meshOverlapOutController = meshHitScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(meshOverlapOutDescStorage));
	if(!meshOverlapOutController)
		return nxFail("mesh-overlap-out controller fixture setup failed");
	NxSceneDesc meshOverlapInSceneDesc;
	meshOverlapInSceneDesc.setToDefault();
	NxScene* const meshOverlapInScene = sdk->createScene(meshOverlapInSceneDesc);
	if(!meshOverlapInScene)
		return nxFail("mesh-overlap-in scene creation failed");
	NxActorDesc meshOverlapInObstacleActorDesc;
	meshOverlapInObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	meshOverlapInObstacleActorDesc.shapes.pushBack(&meshControllerObstacleShape);
	NxActor* const meshOverlapInObstacle = meshOverlapInScene->createActor(
		meshOverlapInObstacleActorDesc);
	alignas(4) unsigned char meshOverlapInDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(meshOverlapInDescStorage + 0x0c) = nxFloatBits(1.5f);
	*reinterpret_cast<NxU32*>(meshOverlapInDescStorage + 0x10) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(meshOverlapInDescStorage + 0x14) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(meshOverlapInDescStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(meshOverlapInDescStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(meshOverlapInDescStorage + 0x38) = nxFloatBits(0.5f);
	NxController* const meshOverlapInController = meshOverlapInScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(meshOverlapInDescStorage));
	if(!meshOverlapInObstacle || !meshOverlapInController)
		return nxFail("mesh-overlap controller fixture setup failed");
	const NxVec3 meshOverlapOutDisplacement(-0.5f, 0.0f, 0.0f);
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(meshOverlapOutController)->move(
		meshOverlapOutDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& meshOverlapOutPosition =
		reinterpret_cast<NxControllerProbe*>(meshOverlapOutController)->getPosition();
	printf("simulation controller-obstacle mesh-overlap-out position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(meshOverlapOutPosition.x), nxFloatBits(meshOverlapOutPosition.y),
		nxFloatBits(meshOverlapOutPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(meshOverlapOutPosition.x) != 0x3f800000 ||
		nxFloatBits(meshOverlapOutPosition.y) != 0x3e4ccccd ||
		nxFloatBits(meshOverlapOutPosition.z) != 0x3e4ccccd || obstacleCollisionFlags != 0;
	const NxVec3 meshOverlapInDisplacement(0.5f, 0.0f, 0.0f);
	obstacleCollisionFlags = 0xdeadbeef;
	reinterpret_cast<NxControllerProbe*>(meshOverlapInController)->move(
		meshOverlapInDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& meshOverlapInPosition =
		reinterpret_cast<NxControllerProbe*>(meshOverlapInController)->getPosition();
	printf("simulation controller-obstacle mesh-overlap-in position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(meshOverlapInPosition.x), nxFloatBits(meshOverlapInPosition.y),
		nxFloatBits(meshOverlapInPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(meshOverlapInPosition.x) != 0x40000000 ||
		nxFloatBits(meshOverlapInPosition.y) != 0x3e4ccccd ||
		nxFloatBits(meshOverlapInPosition.z) != 0x3e4ccccd || obstacleCollisionFlags != 0;
	meshOverlapInScene->releaseController(*meshOverlapInController);
	sdk->releaseScene(*meshOverlapInScene);
	meshHitScene->releaseController(*meshOverlapOutController);
	sdk->releaseScene(*meshHitScene);
	sdk->releaseTriangleMesh(*controllerMesh);

	// Reverse the established controller mesh winding so the moving controller
	// approaches the back side of the same triangular face.
	const NxU32 backfaceMeshIndices[] = {0, 3, 1, 0, 2, 3};
	NxTriangleMeshDesc backfaceMeshDesc;
	backfaceMeshDesc.numVertices = 4;
	backfaceMeshDesc.numTriangles = 2;
	backfaceMeshDesc.pointStrideBytes = sizeof(NxPoint);
	backfaceMeshDesc.triangleStrideBytes = 3 * sizeof(NxU32);
	backfaceMeshDesc.points = controllerMeshPoints;
	backfaceMeshDesc.triangles = backfaceMeshIndices;
	NxTriangleMesh* const backfaceMesh = sdk->createTriangleMesh(backfaceMeshDesc);
	if(!backfaceMesh)
		return nxFail("controller back-face mesh cooking failed");
	NxSceneDesc backfaceControllerSceneDesc;
	backfaceControllerSceneDesc.setToDefault();
	NxScene* const backfaceControllerScene = sdk->createScene(backfaceControllerSceneDesc);
	if(!backfaceControllerScene)
		return nxFail("controller back-face scene creation failed");
	NxTriangleMeshShapeDesc backfaceObstacleShape;
	backfaceObstacleShape.meshData = backfaceMesh;
	NxActorDesc backfaceObstacleActorDesc;
	backfaceObstacleActorDesc.globalPose.t = NxVec3(1.5f, 0.0f, 0.0f);
	backfaceObstacleActorDesc.shapes.pushBack(&backfaceObstacleShape);
	NxActor* const backfaceObstacle = backfaceControllerScene->createActor(
		backfaceObstacleActorDesc);
	alignas(4) unsigned char backfaceControllerDescStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(backfaceControllerDescStorage + 0x0c) = nxFloatBits(-2.0f);
	*reinterpret_cast<NxU32*>(backfaceControllerDescStorage + 0x10) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(backfaceControllerDescStorage + 0x14) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(backfaceControllerDescStorage + 0x30) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(backfaceControllerDescStorage + 0x34) = nxFloatBits(0.2f);
	*reinterpret_cast<NxU32*>(backfaceControllerDescStorage + 0x38) = nxFloatBits(0.2f);
	NxController* const backfaceController = backfaceControllerScene->createController(
		*reinterpret_cast<const NxControllerDesc*>(backfaceControllerDescStorage));
	if(!backfaceObstacle || !backfaceController)
		return nxFail("controller back-face fixture setup failed");
	obstacleCollisionFlags = 0xdeadbeef;
	const NxVec3 backfaceDisplacement(4.0f, 0.0f, 0.0f);
	reinterpret_cast<NxControllerProbe*>(backfaceController)->move(
		backfaceDisplacement, 0xffffffff, 0.001f, obstacleCollisionFlags);
	const NxVec3& backfaceControllerPosition =
		reinterpret_cast<NxControllerProbe*>(backfaceController)->getPosition();
	printf("simulation controller-obstacle mesh-backface position=%08x.%08x.%08x flags=%08x\n",
		nxFloatBits(backfaceControllerPosition.x), nxFloatBits(backfaceControllerPosition.y),
		nxFloatBits(backfaceControllerPosition.z), obstacleCollisionFlags);
	controllerCreateFailed = controllerCreateFailed ||
		nxFloatBits(backfaceControllerPosition.x) != 0x40000000 ||
		nxFloatBits(backfaceControllerPosition.y) != 0x3e4ccccd ||
		nxFloatBits(backfaceControllerPosition.z) != 0x3e4ccccd || obstacleCollisionFlags != 0;
	backfaceControllerScene->releaseController(*backfaceController);
	sdk->releaseScene(*backfaceControllerScene);
	sdk->releaseTriangleMesh(*backfaceMesh);

	sdk->release();
	for(unsigned sdkCycle = 0; sdkCycle != 2; ++sdkCycle)
		{
		NxPhysicsSDK* const cycleSdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
		if(!cycleSdk)
			return nxFail("repeated lifecycle SDK creation failed");
		for(unsigned sceneCycle = 0; sceneCycle != 2; ++sceneCycle)
			{
			NxSceneDesc cycleSceneDesc;
			cycleSceneDesc.setToDefault();
			cycleSceneDesc.gravity = NxVec3(0.0f, -9.81f, 0.0f);
			NxScene* const cycleScene = cycleSdk->createScene(cycleSceneDesc);
			if(!cycleScene)
				return nxFail("repeated lifecycle scene creation failed");
			cycleScene->setTiming(0.02f, 1, NX_TIMESTEP_FIXED);
			NxSphereShapeDesc cycleSphere;
			cycleSphere.radius = 0.25f;
			NxBodyDesc cycleBody;
			NxActorDesc cycleActorDesc;
			cycleActorDesc.body = &cycleBody;
			cycleActorDesc.density = 1.0f;
			cycleActorDesc.globalPose.t = NxVec3((NxReal)(sdkCycle * 4 + sceneCycle), 1.0f, 0.0f);
			cycleActorDesc.shapes.pushBack(&cycleSphere);
			NxActor* const cycleActor = cycleScene->createActor(cycleActorDesc);
			if(!cycleActor)
				return nxFail("repeated lifecycle actor creation failed");
			for(unsigned step = 0; step != 4; ++step)
				{
				cycleScene->simulate(0.02f);
				const bool ready = cycleScene->checkResults(NX_RIGID_BODY_FINISHED, true);
				const bool fetched = cycleScene->fetchResults(NX_RIGID_BODY_FINISHED, true);
				if(!ready || !fetched)
					return nxFail("repeated lifecycle simulation result was not ready and fetched");
				}
			const NxVec3& position = cycleActor->getGlobalPosition();
			NxVec3 velocity;
			cycleActor->getLinearVelocity(velocity);
			printf("simulation lifecycle state sdk=%u scene=%u p=%08x.%08x.%08x v=%08x.%08x.%08x\n",
				sdkCycle, sceneCycle,
				nxFloatBits(position.x), nxFloatBits(position.y), nxFloatBits(position.z),
				nxFloatBits(velocity.x), nxFloatBits(velocity.y), nxFloatBits(velocity.z));
			cycleSdk->releaseScene(*cycleScene);
			printf("simulation lifecycle scene_released sdk=%u scene=%u\n", sdkCycle, sceneCycle);
			}
		cycleSdk->release();
		printf("simulation lifecycle sdk_released cycle=%u\n", sdkCycle);
		}
	if(!nxSimulationRunFluidModelLoadX87Child(pairDirectory))
		{
		FreeLibrary(physics);
		return nxFail("FluidModel loader x87 child probe failed");
		}
	status = nxReportPairIdentity(pairDirectory);
	if(deferredContactFailed)
		status = 1;
	if(controllerCreateFailed)
		status = 1;
	if(shapeUserDataFailed)
		status = 1;
	FreeLibrary(physics);
	return status;
	}
