// Direct differential for the otherwise-unreachable kind-5 joint support row.
// The oracle's public joint factories do not emit kind 5, so this fixture builds
// the measured internal Scene arrays and calls the per-island solver in both the
// pinned DLL and the reconstruction linked into this test process.

#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>

#include "PhysicsPairLoader.h"
#include "NxVec3.h"
#include "core/JointSupport.h"
#include "Scene.h"
#include "NxUserAllocator.h"
#include "NxFoundationSDK.h"

// Unused JointSupport.cpp helper rows reference these host functions. The
// fixture drives only 004399, which has no Foundation allocator or actor-force
// call; local inert definitions keep those unrelated rows out of the DLL load.
NxUserAllocator* nxFoundationSDKAllocator = 0;
void nxNpActorApplyForce(unsigned char*, const NxVec3*, const NxVec3*, unsigned, bool)
	{}

static const unsigned kOracleSolverWrapperRva = 0x0009b240;
static const unsigned kOracleIterationsRva = 0x0012718c;
static const unsigned kOracleStepCounterRva = 0x00127184;
static const unsigned short kSimulateControl = 0x0f7f;

typedef int (__thiscall *OracleSolveFn)(void*, NxReal);

class JointCapture
	{
	public:
	JointCapture(): calls(0), force(0.0f), axis(0.0f, 0.0f, 0.0f), step(0.0f) {}
	virtual void slot0() {}
	virtual void slot1() {}
	virtual void slot2() {}
	virtual void slot3(NxReal value, const NxVec3& direction, NxReal timestep)
		{ ++calls; force = value; axis = direction; step = timestep; }
	unsigned calls;
	NxReal force;
	NxVec3 axis;
	NxReal step;
	};

struct JointSupportFixture
	{
	JointSupportBody body;
	JointSupportRecord record;
	JointCapture joint;
	NxU32 iterations;
	NxU32 stepCounter;
	__declspec(align(16)) unsigned char scene[0x710];
	};

static void nxSetControl(unsigned short word)
	{
	unsigned short value = word;
	__asm { fldcw value }
	}

static unsigned nxFloatBits(NxReal value)
	{
	unsigned bits;
	memcpy(&bits, &value, sizeof(bits));
	return bits;
	}

static unsigned long long nxFold(unsigned long long state, unsigned value)
	{
	for(unsigned i = 0; i != 4; ++i)
		{
		state ^= (value >> (i * 8)) & 0xffu;
		state *= 1099511628211ull;
		}
	return state;
	}

static void nxInitFixture(JointSupportFixture& fixture, NxReal maxForce)
	{
	memset(&fixture.body, 0, sizeof(fixture.body));
	memset(&fixture.record, 0, sizeof(fixture.record));
	memset(fixture.scene, 0, sizeof(fixture.scene));
	fixture.joint.calls = 0;
	fixture.joint.force = 0.0f;
	fixture.joint.axis = NxVec3(0.0f, 0.0f, 0.0f);
	fixture.joint.step = 0.0f;
	fixture.iterations = 1;
	fixture.stepCounter = 0x13579bdfu;

	fixture.body.mUnknown000 = NxVec3(2.0f, 0.0f, 0.0f);
	fixture.body.mUnknown00c = 1.0f;
	fixture.body.mUnknown05c = 1;
	fixture.record.mUnknown000 = NxVec3(1.0f, 0.0f, 0.0f);
	fixture.record.mFlags = 5;
	fixture.record.mBody[0] = &fixture.body;
	fixture.record.mUnknown030 = &fixture.joint;
	fixture.record.mUnknown03c = 0.5f;
	fixture.record.mUnknown048 = maxForce;
	fixture.record.mUnknown04c = 1.0f;

	*reinterpret_cast<JointSupportBody**>(fixture.scene + 0x5ac) = &fixture.body;
	*reinterpret_cast<NxU32*>(fixture.scene + 0x5b0) = 1;
	*reinterpret_cast<JointSupportRecord**>(fixture.scene + 0x5b8) = &fixture.record;
	*reinterpret_cast<NxU32*>(fixture.scene + 0x5bc) = 1;
	}

static unsigned nxCompareFixture(const JointSupportFixture& oracle,
	const JointSupportFixture& candidate, unsigned long long& oracleDigest,
	unsigned long long& candidateDigest)
	{
	unsigned mismatches = 0;
	const unsigned oracleWords[] = {
		oracle.record.mFlags, nxFloatBits(oracle.record.mUnknown034),
		nxFloatBits(oracle.record.mUnknown03c), oracle.record.mUnknown044,
		nxFloatBits(oracle.record.mUnknown048), oracle.record.mUnknown04c,
		nxFloatBits(oracle.body.mUnknown000.x), nxFloatBits(oracle.body.mUnknown000.y),
		nxFloatBits(oracle.body.mUnknown000.z), nxFloatBits(oracle.body.mUnknown010.x),
		nxFloatBits(oracle.body.mUnknown010.y), nxFloatBits(oracle.body.mUnknown010.z),
		nxFloatBits(oracle.body.mUnknown044.x), nxFloatBits(oracle.body.mUnknown044.y),
		nxFloatBits(oracle.body.mUnknown044.z), nxFloatBits(oracle.body.mUnknown050.x),
		nxFloatBits(oracle.body.mUnknown050.y), nxFloatBits(oracle.body.mUnknown050.z), oracle.joint.calls,
		nxFloatBits(oracle.joint.force), nxFloatBits(oracle.joint.axis.x),
		nxFloatBits(oracle.joint.axis.y), nxFloatBits(oracle.joint.axis.z),
		nxFloatBits(oracle.joint.step), oracle.iterations, oracle.stepCounter
		};
	const unsigned candidateWords[] = {
		candidate.record.mFlags, nxFloatBits(candidate.record.mUnknown034),
		nxFloatBits(candidate.record.mUnknown03c), candidate.record.mUnknown044,
		nxFloatBits(candidate.record.mUnknown048), candidate.record.mUnknown04c,
		nxFloatBits(candidate.body.mUnknown000.x), nxFloatBits(candidate.body.mUnknown000.y),
		nxFloatBits(candidate.body.mUnknown000.z), nxFloatBits(candidate.body.mUnknown010.x),
		nxFloatBits(candidate.body.mUnknown010.y), nxFloatBits(candidate.body.mUnknown010.z),
		nxFloatBits(candidate.body.mUnknown044.x), nxFloatBits(candidate.body.mUnknown044.y),
		nxFloatBits(candidate.body.mUnknown044.z), nxFloatBits(candidate.body.mUnknown050.x),
		nxFloatBits(candidate.body.mUnknown050.y), nxFloatBits(candidate.body.mUnknown050.z), candidate.joint.calls,
		nxFloatBits(candidate.joint.force), nxFloatBits(candidate.joint.axis.x),
		nxFloatBits(candidate.joint.axis.y), nxFloatBits(candidate.joint.axis.z),
		nxFloatBits(candidate.joint.step), candidate.iterations, candidate.stepCounter
		};
	for(unsigned i = 0; i != sizeof(oracleWords) / sizeof(oracleWords[0]); ++i)
		{
		mismatches += oracleWords[i] != candidateWords[i];
		oracleDigest = nxFold(oracleDigest, oracleWords[i]);
		candidateDigest = nxFold(candidateDigest, candidateWords[i]);
		}
	return mismatches;
	}

static unsigned nxRunCase(unsigned char* oracleBase, NxReal maxForce,
	unsigned long long& oracleDigest, unsigned long long& candidateDigest,
	unsigned long long& inputDigest)
	{
	JointSupportFixture oracle, candidate;
	nxInitFixture(oracle, maxForce);
	nxInitFixture(candidate, maxForce);
	const unsigned inputWords[] = {
		oracle.record.mFlags, oracle.record.mBody[1] ? 1u : 0u,
		oracle.iterations, oracle.stepCounter, nxFloatBits(1.0f / 60.0f),
		nxFloatBits(oracle.body.mUnknown000.x), nxFloatBits(oracle.body.mUnknown000.y),
		nxFloatBits(oracle.body.mUnknown000.z), nxFloatBits(oracle.body.mUnknown00c),
		nxFloatBits(oracle.body.mUnknown010.x), nxFloatBits(oracle.body.mUnknown010.y),
		nxFloatBits(oracle.body.mUnknown010.z),
		nxFloatBits(oracle.body.mUnknown020[0]), nxFloatBits(oracle.body.mUnknown020[1]),
		nxFloatBits(oracle.body.mUnknown020[2]), nxFloatBits(oracle.body.mUnknown020[3]),
		nxFloatBits(oracle.body.mUnknown020[4]), nxFloatBits(oracle.body.mUnknown020[5]),
		nxFloatBits(oracle.body.mUnknown020[6]), nxFloatBits(oracle.body.mUnknown020[7]),
		nxFloatBits(oracle.body.mUnknown020[8]), oracle.body.mUnknown05c,
		nxFloatBits(oracle.record.mUnknown000.x), nxFloatBits(oracle.record.mUnknown000.y),
		nxFloatBits(oracle.record.mUnknown000.z),
		nxFloatBits(oracle.record.mUnknown018.x), nxFloatBits(oracle.record.mUnknown018.y),
		nxFloatBits(oracle.record.mUnknown018.z),
		nxFloatBits(oracle.record.mUnknown024.x), nxFloatBits(oracle.record.mUnknown024.y),
		nxFloatBits(oracle.record.mUnknown024.z),
		nxFloatBits(oracle.record.mUnknown034), nxFloatBits(oracle.record.mUnknown038),
		nxFloatBits(oracle.record.mUnknown03c), nxFloatBits(oracle.record.mUnknown040),
		oracle.record.mUnknown044, nxFloatBits(oracle.record.mUnknown048),
		oracle.record.mUnknown04c
		};
	for(unsigned i = 0; i != sizeof(inputWords) / sizeof(inputWords[0]); ++i)
		inputDigest = nxFold(inputDigest, inputWords[i]);

	// The shipped row reads its iteration count, clears the step counter, runs
	// the solver, then clears the iteration count. Seed both globals so the
	// wrapper's output state is observable alongside the record mutations.
	*reinterpret_cast<NxU32*>(oracleBase + kOracleIterationsRva) = oracle.iterations;
	*reinterpret_cast<NxU32*>(oracleBase + kOracleStepCounterRva) = oracle.stepCounter;
	nxSetControl(kSimulateControl);
	reinterpret_cast<OracleSolveFn>(oracleBase + kOracleSolverWrapperRva)(oracle.scene, 1.0f / 60.0f);
	nxSetControl(kSimulateControl);
	oracle.iterations = *reinterpret_cast<NxU32*>(oracleBase + kOracleIterationsRva);
	oracle.stepCounter = *reinterpret_cast<NxU32*>(oracleBase + kOracleStepCounterRva);

	nxSolveJointSupportIslandRecords(reinterpret_cast<NxSceneInternal*>(candidate.scene),
		1.0f / 60.0f, candidate.iterations, candidate.stepCounter, true);

	return nxCompareFixture(oracle, candidate, oracleDigest, candidateDigest);
	}

struct JointSupportImpulseFixture
	{
	JointSupportBody body[2];
	JointSupportRecord record;
	};

static void nxInitImpulseFixture(JointSupportImpulseFixture& fixture,
	unsigned flags, unsigned bodyMask, NxReal inverseMass0, NxReal inverseMass1)
	{
	memset(&fixture, 0, sizeof(fixture));
	fixture.body[0].mUnknown000 = NxVec3(0.75f, -0.375f, 0.625f);
	fixture.body[1].mUnknown000 = NxVec3(-0.25f, 0.875f, -0.5f);
	fixture.body[0].mUnknown00c = inverseMass0;
	fixture.body[1].mUnknown00c = inverseMass1;
	fixture.body[0].mUnknown010 = NxVec3(-0.625f, 0.1875f, 0.4375f);
	fixture.body[1].mUnknown010 = NxVec3(0.3125f, -0.75f, 0.125f);
	const NxReal inertia0[9] = {
		0.5f, 0.125f, -0.0625f, 0.25f, 0.625f, 0.125f,
		-0.03125f, 0.0625f, 0.875f};
	const NxReal inertia1[9] = {
		0.375f, -0.125f, 0.0625f, 0.1875f, 0.75f, -0.25f,
		0.03125f, 0.125f, 0.5625f};
	memcpy(fixture.body[0].mUnknown020, inertia0, sizeof(inertia0));
	memcpy(fixture.body[1].mUnknown020, inertia1, sizeof(inertia1));
	fixture.record.mFlags = flags;
	fixture.record.mBody[0] = (bodyMask & 1) ? &fixture.body[0] : 0;
	fixture.record.mBody[1] = (bodyMask & 2) ? &fixture.body[1] : 0;
	fixture.record.mUnknown000 = NxVec3(0.375f, -0.625f, 0.875f);
	fixture.record.mUnknown018 = NxVec3(0.5f, 0.25f, -0.125f);
	fixture.record.mUnknown024 = NxVec3(-0.3125f, 0.1875f, 0.4375f);
	}

static unsigned nxRunImpulseCase(unsigned char* oracleBase, unsigned flags,
	unsigned bodyMask, NxReal inverseMass0, NxReal inverseMass1, NxReal impulse,
	unsigned short controlWord, int opaqueArgument, unsigned long long& oracleDigest,
	unsigned long long& candidateDigest, unsigned long long& inputDigest)
	{
	JointSupportImpulseFixture oracle, candidate;
	nxInitImpulseFixture(oracle, flags, bodyMask, inverseMass0, inverseMass1);
	nxInitImpulseFixture(candidate, flags, bodyMask, inverseMass0, inverseMass1);
	const unsigned inputWords[] = {
		flags, bodyMask, nxFloatBits(inverseMass0), nxFloatBits(inverseMass1),
		nxFloatBits(impulse), controlWord, static_cast<unsigned>(opaqueArgument),
		nxFloatBits(oracle.record.mUnknown000.x), nxFloatBits(oracle.record.mUnknown000.y),
		nxFloatBits(oracle.record.mUnknown000.z),
		nxFloatBits(oracle.record.mUnknown018.x), nxFloatBits(oracle.record.mUnknown018.y),
		nxFloatBits(oracle.record.mUnknown018.z),
		nxFloatBits(oracle.record.mUnknown024.x), nxFloatBits(oracle.record.mUnknown024.y),
		nxFloatBits(oracle.record.mUnknown024.z),
		nxFloatBits(oracle.body[0].mUnknown000.x), nxFloatBits(oracle.body[0].mUnknown000.y),
		nxFloatBits(oracle.body[0].mUnknown000.z), nxFloatBits(oracle.body[0].mUnknown00c),
		nxFloatBits(oracle.body[0].mUnknown010.x), nxFloatBits(oracle.body[0].mUnknown010.y),
		nxFloatBits(oracle.body[0].mUnknown010.z),
		nxFloatBits(oracle.body[1].mUnknown000.x), nxFloatBits(oracle.body[1].mUnknown000.y),
		nxFloatBits(oracle.body[1].mUnknown000.z), nxFloatBits(oracle.body[1].mUnknown00c),
		nxFloatBits(oracle.body[1].mUnknown010.x), nxFloatBits(oracle.body[1].mUnknown010.y),
		nxFloatBits(oracle.body[1].mUnknown010.z)
		};
	for(unsigned i = 0; i != sizeof(inputWords) / sizeof(inputWords[0]); ++i)
		inputDigest = nxFold(inputDigest, inputWords[i]);
	for(unsigned body = 0; body != 2; ++body)
		for(unsigned i = 0; i != 9; ++i)
			inputDigest = nxFold(inputDigest,
				nxFloatBits(oracle.body[body].mUnknown020[i]));

	typedef void (__thiscall *OracleApplyImpulseFn)(void*, NxReal, int);
	nxSetControl(controlWord);
	NxReal stepBits;
	memcpy(&stepBits, &opaqueArgument, sizeof(stepBits));
	reinterpret_cast<OracleApplyImpulseFn>(oracleBase + 0x000af790)(
		&oracle.record, impulse, opaqueArgument);
	nxSetControl(controlWord);
	reinterpret_cast<JointSupportApplyFixture*>(&candidate.record)->applyImpulse004395(
		impulse, stepBits);

	unsigned mismatches = 0;
	for(unsigned body = 0; body != 2; ++body)
		{
		const NxReal* oracleWords[] = {
			&oracle.body[body].mUnknown000.x, &oracle.body[body].mUnknown000.y,
			&oracle.body[body].mUnknown000.z, &oracle.body[body].mUnknown010.x,
			&oracle.body[body].mUnknown010.y, &oracle.body[body].mUnknown010.z};
		const NxReal* candidateWords[] = {
			&candidate.body[body].mUnknown000.x, &candidate.body[body].mUnknown000.y,
			&candidate.body[body].mUnknown000.z, &candidate.body[body].mUnknown010.x,
			&candidate.body[body].mUnknown010.y, &candidate.body[body].mUnknown010.z};
		for(unsigned i = 0; i != 6; ++i)
			{
			const unsigned oracleBits = nxFloatBits(*oracleWords[i]);
			const unsigned candidateBits = nxFloatBits(*candidateWords[i]);
			mismatches += oracleBits != candidateBits;
			oracleDigest = nxFold(oracleDigest, oracleBits);
			candidateDigest = nxFold(candidateDigest, candidateBits);
			}
		}
	return mismatches;
	}

class JointSupportAllocator : public NxUserAllocator
	{
	public:
	JointSupportAllocator(): frees(0) {}
	void* mallocDEBUG(size_t size, const char*, int) { return ::malloc(size); }
	void* malloc(size_t size) { return ::malloc(size); }
	void* realloc(void* memory, size_t size) { return ::realloc(memory, size); }
	void free(void* memory)
		{
		++frees;
		::free(memory);
		}
	unsigned frees;
	};

static unsigned gIslandObjectCalls;
static unsigned gIslandObjectBadFlags;
static unsigned gIslandObjectOrder;

class IslandObjectDeleteProbe
	{
	public:
	IslandObjectDeleteProbe(unsigned id): mId(id) {}
	virtual void release(unsigned flags)
		{
		++gIslandObjectCalls;
		gIslandObjectBadFlags += flags != 1u;
		gIslandObjectOrder = gIslandObjectOrder * 33u + mId;
		}
	unsigned mId;
	};

struct IslandObjectRaw
	{
	void** mFirstBegin;
	void** mFirstEnd;
	void** mFirstCapacity;
	NxU32 mUnknown00c;
	void** mSecondBegin;
	void** mSecondEnd;
	void** mSecondCapacity;
	};

typedef NxFoundationSDK* (NX_CALL_CONV *CreateFoundationSDKFn)(NxU32,
	NxUserOutputStream*, NxUserAllocator*);
typedef void (__thiscall *OracleIslandObjectReleaseFn)(void*);

static void nxInitIslandObject(IslandObjectRaw& object, NxUserAllocator& allocator,
	IslandObjectDeleteProbe& first, IslandObjectDeleteProbe& second,
	IslandObjectDeleteProbe& third)
	{
	void** firstArray = static_cast<void**>(allocator.malloc(3 * sizeof(void*), NX_MEMORY_PERSISTENT));
	void** secondArray = static_cast<void**>(allocator.malloc(2 * sizeof(void*), NX_MEMORY_PERSISTENT));
	firstArray[0] = &first;
	firstArray[1] = 0;
	firstArray[2] = &second;
	secondArray[0] = &third;
	secondArray[1] = 0;
	object.mFirstBegin = firstArray;
	object.mFirstEnd = firstArray + 3;
	object.mFirstCapacity = firstArray + 3;
	object.mUnknown00c = 0xfeedbeefu;
	object.mSecondBegin = secondArray;
	object.mSecondEnd = secondArray + 2;
	object.mSecondCapacity = secondArray + 2;
	}

static bool nxIslandObjectCleared(const IslandObjectRaw& object)
	{
	return !object.mFirstBegin && !object.mFirstEnd && !object.mFirstCapacity
		&& object.mUnknown00c == 0xfeedbeefu
		&& !object.mSecondBegin && !object.mSecondEnd && !object.mSecondCapacity;
	}

static int nxRunIslandObjectTeardown(unsigned char* oracleBase)
	{
	HMODULE foundationModule = GetModuleHandleW(L"NxFoundation.dll");
	CreateFoundationSDKFn createFoundation = foundationModule ?
		reinterpret_cast<CreateFoundationSDKFn>(GetProcAddress(foundationModule,
			"NxCreateFoundationSDK")) : 0;
	if(!createFoundation)
		return nxFail("NxCreateFoundationSDK missing for island-object teardown differential");
	JointSupportAllocator allocator;
	NxFoundationSDK* foundation = createFoundation(NX_FOUNDATION_SDK_VERSION, 0, &allocator);
	if(!foundation)
		return nxFail("NxCreateFoundationSDK failed for island-object teardown differential");
	nxFoundationSDKAllocator = &allocator;

	IslandObjectRaw oracle = {};
	IslandObjectRaw candidate = {};
	IslandObjectDeleteProbe oracleFirst(101), oracleSecond(102), oracleThird(103);
	IslandObjectDeleteProbe candidateFirst(101), candidateSecond(102), candidateThird(103);
	nxInitIslandObject(oracle, foundation->getAllocator(), oracleFirst, oracleSecond, oracleThird);
	unsigned oracleFreesBefore = allocator.frees;
	gIslandObjectCalls = gIslandObjectBadFlags = gIslandObjectOrder = 0;
	reinterpret_cast<OracleIslandObjectReleaseFn>(oracleBase + 0x0009ad10)(&oracle);
	const unsigned oracleCalls = gIslandObjectCalls;
	const unsigned oracleBadFlags = gIslandObjectBadFlags;
	const unsigned oracleOrder = gIslandObjectOrder;
	const unsigned oracleFrees = allocator.frees - oracleFreesBefore;
	const bool oracleCleared = nxIslandObjectCleared(oracle);

	nxInitIslandObject(candidate, allocator, candidateFirst, candidateSecond, candidateThird);
	unsigned candidateFreesBefore = allocator.frees;
	gIslandObjectCalls = gIslandObjectBadFlags = gIslandObjectOrder = 0;
	reinterpret_cast<Row004167Fixture*>(&candidate)->row004167();
	const unsigned candidateCalls = gIslandObjectCalls;
	const unsigned candidateBadFlags = gIslandObjectBadFlags;
	const unsigned candidateOrder = gIslandObjectOrder;
	const unsigned candidateFrees = allocator.frees - candidateFreesBefore;
	const bool candidateCleared = nxIslandObjectCleared(candidate);

	unsigned mismatches = 0;
	mismatches += oracleCalls != candidateCalls;
	mismatches += oracleBadFlags != candidateBadFlags;
	mismatches += oracleOrder != candidateOrder;
	mismatches += oracleFrees != candidateFrees;
	mismatches += oracleCleared != candidateCleared;
	const bool oracleOk = oracleCalls == 3 && oracleBadFlags == 0
		&& oracleFrees == 2 && oracleCleared;
	printf("joint_support island_teardown oracle=%u/%u/%08x/%u candidate=%u/%u/%08x/%u oracle_cleared=%u candidate_cleared=%u mismatches=%u\n",
		oracleCalls, oracleBadFlags, oracleOrder, oracleFrees,
		candidateCalls, candidateBadFlags, candidateOrder, candidateFrees,
		oracleCleared ? 1u : 0u, candidateCleared ? 1u : 0u, mismatches);

	nxFoundationSDKAllocator = 0;
	foundation->release();
	if(!oracleOk)
		return nxFail("oracle island-object teardown fixture did not exercise the expected contract");
	if(mismatches)
		return nxFail("candidate island-object teardown differs from the oracle");
	printf("joint_support coverage name=island_object_teardown calls=3 frees=2 cleared=1\n");
	return 0;
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	if(argc != 3)
		{
		fprintf(stderr, "usage: NxPhysicsJointSupportTests <oracle directory> <NxPhysics.dll sha256>\n");
		return 2;
		}
	int status = nxOpenPair(argc - 1, argv, "NxPhysicsJointSupportTests", pairDirectory, &physics);
	if(status)
		return status;

	wchar_t loadedPath[MAX_PATH];
	char loadedHash[65], expectedHash[65];
	if(!GetModuleFileNameW(physics, loadedPath, MAX_PATH) || !nxSha256(loadedPath, loadedHash))
		return nxFail("cannot identify the loaded oracle");
	if(wcstombs_s(0, expectedHash, sizeof(expectedHash), argv[2], _TRUNCATE) != 0 ||
		strcmp(loadedHash, expectedHash) != 0)
		return nxFail("loaded NxPhysics.dll is not the pinned oracle");
	unsigned char* base = reinterpret_cast<unsigned char*>(physics);

	unsigned long long oracleDigest = 14695981039346656037ull;
	unsigned long long candidateDigest = 14695981039346656037ull;
	unsigned long long inputDigest = 14695981039346656037ull;
	unsigned mismatches = 0;
	mismatches += nxRunCase(base, 100.0f, oracleDigest, candidateDigest, inputDigest);
	mismatches += nxRunCase(base, 0.25f, oracleDigest, candidateDigest, inputDigest);
	printf("joint_support kind5 cases=2 oracle=%016llx candidate=%016llx mismatches=%u\n",
		oracleDigest, candidateDigest, mismatches);
	printf("joint_support inputs=2 digest=%016llx\n", inputDigest);
	printf("joint_support coverage name=kind5_solver cases=2 passes=1,final callback=slot3\n");
	struct ImpulseCase
		{
		unsigned flags;
		unsigned bodyMask;
		NxReal inverseMass0;
		NxReal inverseMass1;
		NxReal impulse;
		unsigned short controlWord;
		int opaqueArgument;
		};
	const ImpulseCase impulseCases[] = {
		{0, 3, 0.75f, 1.25f, 0.625f, 0x027f, 0},
		{0x400, 3, 0.75f, 1.25f, -0.4375f, 0x0f7f, 0x12345678},
		{0, 1, 0.5f, 0.0f, 0.375f, 0x027f, static_cast<int>(0xdeadbeef)},
		{0, 2, 0.0f, 1.75f, -0.5625f, 0x0f7f, 0x01020304},
		{0x400, 1, 0.875f, 0.0f, 0.3125f, 0x027f, 0},
		{0x400, 2, 0.0f, 0.625f, 0.6875f, 0x0f7f, 0x7fffffff},
		{0, 3, 0.0f, 0.0f, -0.25f, 0x027f, static_cast<int>(0x80000000u)},
		{0x400, 3, 0.875f, 0.0f, -0.1875f, 0x0f7f, static_cast<int>(0xffffffffu)},
		{0, 0, 0.5f, 1.25f, 0.25f, 0x027f, 0x55667788}
		};
	unsigned long long impulseOracleDigest = 14695981039346656037ull;
	unsigned long long impulseCandidateDigest = 14695981039346656037ull;
	unsigned long long impulseInputDigest = 14695981039346656037ull;
	unsigned impulseMismatches = 0;
	for(unsigned i = 0; i != sizeof(impulseCases) / sizeof(impulseCases[0]); ++i)
		{
		const ImpulseCase& test = impulseCases[i];
		impulseMismatches += nxRunImpulseCase(base, test.flags, test.bodyMask,
			test.inverseMass0, test.inverseMass1, test.impulse, test.controlWord,
			test.opaqueArgument, impulseOracleDigest, impulseCandidateDigest,
			impulseInputDigest);
		}
	printf("joint_support applyImpulse cases=9 oracle=%016llx candidate=%016llx mismatches=%u\n",
		impulseOracleDigest, impulseCandidateDigest, impulseMismatches);
	printf("joint_support applyImpulse inputs=9 digest=%016llx\n", impulseInputDigest);
	int islandStatus = nxRunIslandObjectTeardown(base);
	if(islandStatus)
		return islandStatus;
	if(nxReportPairIdentity(pairDirectory))
		return 1;
	return (mismatches || impulseMismatches) ? 1 : 0;
	}
