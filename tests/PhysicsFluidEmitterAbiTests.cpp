#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <fstream>
#include <stdio.h>
#include <sstream>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "NxFoundationSDK.h"
#include "NxUserOutputStream.h"
#include "fluids/NxFluidEmitter.h"
#include "fluids/NpFluidEmitter.h"
#include "fluids/NxMeshData.h"
#include "ContactPairManager.h"

static bool nxSha256(const wchar_t* path, char output[65]);

static unsigned gFailures;
static void* gCandidateTriangleIndexBuilderAddress;
static void* gCandidateFluidCollisionShapePackerAddress;
static void* gCandidateFluidCollisionShapeCounterAddress;
static void* gCandidateFluidCollisionSceneShapeCounterAddress;
static void* gCandidateFluidCollisionSphereAppendAddress;
static void* gCandidateFluidCollisionCapsuleAppendAddress;
static void* gCandidateFluidCollisionBoxAppendAddress;
static void* gCandidateFluidCollisionPlaneAppendAddress;
static void* gCandidateFluidCollisionMeshAppendAddress;
static void* gCandidateFluidCollisionShapeArrayRouterAddress;
static void* gCandidateFluidCollisionSceneShapeCollectorAddress;
static void* gCandidateFluidCollisionStaticRebuildAddress;

extern "C" void __cdecl nxFluidEmitterAbiUnexpectedWriteLockFailure(unsigned line)
	{
	fprintf(stderr, "FAIL test-only NpFluidEmitter unexpectedly rejected write lock at line %u\n",
		line);
	++gFailures;
	}

extern "C" void __cdecl nxFluidEmitterAbiUnexpectedReentryFailure(unsigned line)
	{
	fprintf(stderr, "FAIL test-only NpFluidEmitter unexpectedly rejected callback reentry at line %u\n",
		line);
	++gFailures;
	}

struct ImplicitMeshErrorRecord
	{
	NxErrorCode code;
	char message[512];
	char file[512];
	int line;
	unsigned count;
	};

class ImplicitMeshOutputStream : public NxUserOutputStream
	{
	public:
	ImplicitMeshErrorRecord record;
	ImplicitMeshOutputStream() { clear(); }
	void clear() { memset(&record, 0, sizeof(record)); }
	void reportError(NxErrorCode code, const char* message, const char* file, int line)
		{
		record.code = code;
		strncpy_s(record.message, message ? message : "", _TRUNCATE);
		strncpy_s(record.file, file ? file : "", _TRUNCATE);
		record.line = line;
		++record.count;
		}
	NxAssertResponse reportAssertViolation(const char*, const char*, int)
		{ return NX_AR_CONTINUE; }
	void print(const char*) {}
	};

static void nxCallImplicitMeshSetData(void* target, void* object,
	NxMeshData* data, unsigned* espBefore, unsigned* espAfter)
	{
	typedef void (__thiscall* SetMeshData)(void*, NxMeshData*);
	SetMeshData setMeshData = reinterpret_cast<SetMeshData>(target);
	unsigned before = 0, after = 0;
	__asm mov before, esp
	setMeshData(object, data);
	__asm mov after, esp
	*espBefore = before;
	*espAfter = after;
	}

static bool nxSameImplicitMeshError(const ImplicitMeshErrorRecord& a,
	const ImplicitMeshErrorRecord& b)
	{
	return a.code == b.code && a.line == b.line && a.count == b.count
		&& strcmp(a.message, b.message) == 0 && strcmp(a.file, b.file) == 0;
	}

static unsigned nxRunImplicitMeshSetDataDifferential(void* oracleAddress,
	void* candidateAddress, void* candidateWrapperAddress,
	void* candidateApiReentryGetter, void* candidateApiReentrySetter,
	NxFoundationSDK* foundation,
	ImplicitMeshOutputStream& output)
	{
	if(!oracleAddress || !candidateAddress || !candidateWrapperAddress
		|| !candidateApiReentryGetter || !candidateApiReentrySetter || !foundation)
		return 1;
	static_assert(sizeof(NxMeshData) == 0x30,
		"the pinned setMeshData row copies twelve words");
	unsigned mismatches = 0;
	unsigned validCases = 0;
	unsigned invalidCases = 0;
	unsigned stackChecks = 0;
	unsigned char positions[96] = {};
	unsigned char normals[96] = {};
	unsigned char indices[32] = {};
	NxU32 vertexCount = 11;
	NxU32 triangleCount = 3;

	auto initIndexed = [&](NxMeshData& data)
		{
		data.verticesPosBegin = positions;
		data.verticesNormalBegin = normals;
		data.verticesPosByteStride = 12;
		data.verticesNormalByteStride = 12;
		data.maxVertices = 8;
		data.numVerticesPtr = &vertexCount;
		data.indicesBegin = indices;
		data.indicesByteStride = 2;
		data.maxIndices = 24;
		data.numTrianglesPtr = &triangleCount;
		data.flags = NX_MDF_INDEXED_MESH | NX_MDF_16_BIT_INDICES;
		data.name = "mesh-data-indexed";
		};

	NxMeshData valid[3];
	initIndexed(valid[1]);
	valid[2].verticesPosBegin = positions;
	valid[2].verticesNormalBegin = normals;
	valid[2].verticesPosByteStride = 12;
	valid[2].verticesNormalByteStride = 12;
	valid[2].maxVertices = 8;
	valid[2].numVerticesPtr = &vertexCount;
	valid[2].flags = 0;
	valid[2].name = "mesh-data-triplets";
	for(unsigned i = 0; i < 3; ++i)
		{
		unsigned char oracleObject[0x40], candidateObject[0x40];
		unsigned char oracleExpected[0x40], candidateExpected[0x40];
		memset(oracleObject, 0xcd, sizeof(oracleObject));
		memset(candidateObject, 0xcd, sizeof(candidateObject));
		memcpy(oracleExpected, oracleObject, sizeof(oracleObject));
		memcpy(candidateExpected, candidateObject, sizeof(candidateObject));
		memcpy(oracleExpected + 8, &valid[i], sizeof(valid[i]));
		memcpy(candidateExpected + 8, &valid[i], sizeof(valid[i]));
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		nxCallImplicitMeshSetData(oracleAddress, oracleObject, &valid[i],
			&oracleBefore, &oracleAfter);
		nxCallImplicitMeshSetData(candidateAddress, candidateObject, &valid[i],
			&candidateBefore, &candidateAfter);
		if(memcmp(oracleObject, oracleExpected, sizeof(oracleObject)) != 0
			|| memcmp(candidateObject, candidateExpected, sizeof(candidateObject)) != 0
			|| memcmp(oracleObject, candidateObject, sizeof(oracleObject)) != 0
			|| oracleBefore != oracleAfter || candidateBefore != candidateAfter)
			++mismatches;
		else
			++validCases;
		stackChecks += oracleBefore == oracleAfter && candidateBefore == candidateAfter;
		}

	NxMeshData invalid[8];
	invalid[0].verticesPosBegin = positions;
	invalid[0].verticesPosByteStride = 12;
	invalid[0].numVerticesPtr = &vertexCount;
	initIndexed(invalid[1]); invalid[1].verticesPosByteStride = 0;
	invalid[2].numTrianglesPtr = &triangleCount;
	initIndexed(invalid[3]); invalid[3].numTrianglesPtr = 0;
	initIndexed(invalid[4]); invalid[4].indicesBegin = 0;
	initIndexed(invalid[5]); invalid[5].numTrianglesPtr = 0;
	initIndexed(invalid[6]); invalid[6].indicesByteStride = 0;
	valid[2].numTrianglesPtr = &triangleCount;
	memcpy(&invalid[7], &valid[2], sizeof(NxMeshData));

	static const char expectedFile[] =
		"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\ImplicitMesh.cpp";
	static const char expectedMessage[] =
		"Supplied NxMeshData is not valid. setMeshData has no effect.";
	for(unsigned i = 0; i < 8; ++i)
		{
		unsigned char oracleObject[0x40], candidateObject[0x40];
		unsigned char oracleBeforeBytes[0x40], candidateBeforeBytes[0x40];
		memset(oracleObject, 0x5a, sizeof(oracleObject));
		memset(candidateObject, 0x5a, sizeof(candidateObject));
		memcpy(oracleBeforeBytes, oracleObject, sizeof(oracleObject));
		memcpy(candidateBeforeBytes, candidateObject, sizeof(candidateObject));
		unsigned oracleEspBefore = 0, oracleEspAfter = 0;
		unsigned candidateEspBefore = 0, candidateEspAfter = 0;
		output.clear();
		nxCallImplicitMeshSetData(oracleAddress, oracleObject, &invalid[i],
			&oracleEspBefore, &oracleEspAfter);
		const ImplicitMeshErrorRecord oracleError = output.record;
		const NxErrorCode oracleLastError = foundation->getLastError();
		output.clear();
		nxCallImplicitMeshSetData(candidateAddress, candidateObject, &invalid[i],
			&candidateEspBefore, &candidateEspAfter);
		const ImplicitMeshErrorRecord candidateError = output.record;
		const NxErrorCode candidateLastError = foundation->getLastError();
		const bool oracleReported = oracleError.count == 1
			&& oracleError.code == NXE_INVALID_PARAMETER
			&& strcmp(oracleError.file, expectedFile) == 0
			&& oracleError.line == 106
			&& strcmp(oracleError.message, expectedMessage) == 0;
		const bool candidateReported = candidateError.count == 1
			&& candidateError.code == NXE_INVALID_PARAMETER
			&& strcmp(candidateError.file, expectedFile) == 0
			&& candidateError.line == 106
			&& strcmp(candidateError.message, expectedMessage) == 0;
		if(!oracleReported || !candidateReported
			|| !nxSameImplicitMeshError(oracleError, candidateError)
			|| oracleLastError != NXE_INVALID_PARAMETER
			|| candidateLastError != NXE_INVALID_PARAMETER
			|| memcmp(oracleObject, oracleBeforeBytes, sizeof(oracleObject)) != 0
			|| memcmp(candidateObject, candidateBeforeBytes, sizeof(candidateObject)) != 0
			|| memcmp(oracleObject, candidateObject, sizeof(oracleObject)) != 0
			|| oracleEspBefore != oracleEspAfter
			|| candidateEspBefore != candidateEspAfter)
			++mismatches;
		else
			++invalidCases;
		stackChecks += oracleEspBefore == oracleEspAfter
			&& candidateEspBefore == candidateEspAfter;
		}

	// phys_fn_003663 is the internal data-record validator/copy routine. The
	// public NxImplicitMesh::setMeshData vtable entry is the lock-taking wrapper
	// at RVA 0x8d230, which passes its +0x14 record pointer to that helper.
	// Probe both layers so a helper cannot accidentally stand in for the API.
	struct WrapperLock
		{
		CRITICAL_SECTION section;
		volatile long held;
		unsigned owner;
		};
	static_assert(offsetof(WrapperLock, held) == 0x18,
		"the pinned Scene lock state follows its 24-byte critical section");
	WrapperLock oracleLock, candidateLock;
	memset(&oracleLock, 0, sizeof(oracleLock));
	memset(&candidateLock, 0, sizeof(candidateLock));
	InitializeCriticalSection(&oracleLock.section);
	InitializeCriticalSection(&candidateLock.section);
	CRITICAL_SECTION* oracleSection = &oracleLock.section;
	CRITICAL_SECTION* candidateSection = &candidateLock.section;
	void* oracleLink = &oracleSection;
	void* candidateLink = &candidateSection;
	// Keep 0x20 bytes of guard beyond the 0x18-byte wrapper. The pre-fix
	// candidate incorrectly treats +0x08 as an embedded NxMeshData and writes
	// through +0x38; the red probe must catch that without corrupting its frame.
	unsigned char oracleWrapper[0x40], candidateWrapper[0x40];
	unsigned char oracleRecord[0x38], candidateRecord[0x38];
	void* oracleRecordPointer = oracleRecord;
	void* candidateRecordPointer = candidateRecord;
	memset(oracleWrapper, 0x5a, sizeof(oracleWrapper));
	memset(candidateWrapper, 0x5a, sizeof(candidateWrapper));
	memset(oracleRecord, 0xcd, sizeof(oracleRecord));
	memset(candidateRecord, 0xcd, sizeof(candidateRecord));
	memcpy(oracleWrapper + 0x0c, &oracleLink, sizeof(oracleLink));
	memcpy(candidateWrapper + 0x0c, &candidateLink, sizeof(candidateLink));
	memcpy(oracleWrapper + 0x10, &oracleLink, sizeof(oracleLink));
	memcpy(candidateWrapper + 0x10, &candidateLink, sizeof(candidateLink));
	memcpy(oracleWrapper + 0x14, &oracleRecordPointer, sizeof(oracleRecordPointer));
	memcpy(candidateWrapper + 0x14, &candidateRecordPointer, sizeof(candidateRecordPointer));
	unsigned wrapperMismatches = 0;
	unsigned wrapperValid = 0;
	unsigned wrapperInvalid = 0;
	unsigned wrapperLockRejected = 0;
	unsigned wrapperReentryRejected = 0;
	unsigned wrapperStackChecks = 0;
	const void* const oracleWrapperEntry = reinterpret_cast<const unsigned char*>(
		oracleAddress) + (0x8d230 - 0x8ac50);
	const bool* const oracleApiReentry = reinterpret_cast<const bool*>(
		reinterpret_cast<const unsigned char*>(oracleAddress) - 0x8ac50 + 0x123c10);
	const bool oracleApiReentryBefore = *oracleApiReentry;
	const bool candidateApiReentryBefore =
		reinterpret_cast<bool (__cdecl*)(void)>(candidateApiReentryGetter)();
	for(unsigned i = 0; i < 4; ++i)
		{
		const bool lockRejected = i == 2;
		const bool reentryRejected = i == 3;
		const NxMeshData& input = i == 1 ? invalid[1] : valid[1];
		const unsigned long otherThread = GetCurrentThreadId() ^ 0x80000000u;
		oracleLock.held = candidateLock.held = lockRejected ? 1 : 0;
		oracleLock.owner = candidateLock.owner = lockRejected ? otherThread : 0;
		*const_cast<bool*>(oracleApiReentry) = reentryRejected;
		if(candidateApiReentrySetter)
			reinterpret_cast<void (__cdecl*)(bool)>(candidateApiReentrySetter)(reentryRejected);
		unsigned char oracleBefore[0x40], candidateBefore[0x40];
		unsigned char oracleRecordBefore[0x38], candidateRecordBefore[0x38];
		memcpy(oracleBefore, oracleWrapper, sizeof(oracleWrapper));
		memcpy(candidateBefore, candidateWrapper, sizeof(candidateWrapper));
		memcpy(oracleRecordBefore, oracleRecord, sizeof(oracleRecord));
		memcpy(candidateRecordBefore, candidateRecord, sizeof(candidateRecord));
		unsigned oracleEspBefore = 0, oracleEspAfter = 0;
		unsigned candidateEspBefore = 0, candidateEspAfter = 0;
		output.clear();
		nxCallImplicitMeshSetData(const_cast<void*>(oracleWrapperEntry),
			oracleWrapper, const_cast<NxMeshData*>(&input),
			&oracleEspBefore, &oracleEspAfter);
		const ImplicitMeshErrorRecord oracleError = output.record;
		output.clear();
		nxCallImplicitMeshSetData(candidateWrapperAddress, candidateWrapper,
			const_cast<NxMeshData*>(&input), &candidateEspBefore,
			&candidateEspAfter);
		const ImplicitMeshErrorRecord candidateError = output.record;
		const bool validCase = i == 0;
		const bool rejected = lockRejected || reentryRejected;
		const bool wrappersEqual = memcmp(oracleWrapper, candidateWrapper, 0x0c) == 0
			&& memcmp(oracleWrapper + 0x18, candidateWrapper + 0x18, 0x28) == 0;
		const bool lockStateMatches = oracleLock.held == candidateLock.held
			&& oracleLock.owner == candidateLock.owner
			&& oracleLock.held == (lockRejected ? 1 : 0);
		const bool locksReleased = lockRejected
			? (oracleLock.held == 1 && candidateLock.held == 1)
			: (oracleLock.held == 0 && candidateLock.held == 0);
		const char* const expectedMessage = lockRejected
			? "PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!"
			: reentryRejected
				? "Reentry check: You may not call this API method from a callback!"
				: 0;
		const char* const expectedFile = rejected
			? "\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\NpImplicitMesh.cpp"
			: 0;
		const int expectedLine = lockRejected ? 25 : (reentryRejected ? 26 : 0);
		const unsigned expectedErrors = (rejected || i == 1) ? 1u : 0u;
		const bool errorMatches = oracleError.count == expectedErrors
			&& candidateError.count == expectedErrors
			&& (!rejected || (oracleError.code == NXE_INVALID_OPERATION
				&& candidateError.code == NXE_INVALID_OPERATION
				&& oracleError.code == candidateError.code
				&& oracleError.line == candidateError.line
				&& oracleError.line == expectedLine
				&& strcmp(oracleError.file, expectedFile) == 0
				&& strcmp(candidateError.file, expectedFile) == 0
				&& strcmp(oracleError.message, expectedMessage) == 0
				&& strcmp(candidateError.message, expectedMessage) == 0));
		const bool matched = wrappersEqual && locksReleased && lockStateMatches
			&& errorMatches
			&& memcmp(oracleRecord, candidateRecord, sizeof(oracleRecord)) == 0
			&& oracleEspBefore == oracleEspAfter
			&& candidateEspBefore == candidateEspAfter
			&& nxSameImplicitMeshError(oracleError, candidateError);
		if(!matched)
			++wrapperMismatches;
		else if(validCase)
			++wrapperValid;
		else if(i == 1)
			++wrapperInvalid;
		else if(lockRejected)
			++wrapperLockRejected;
		else
			++wrapperReentryRejected;
		wrapperStackChecks += oracleEspBefore == oracleEspAfter
			&& candidateEspBefore == candidateEspAfter;
		// A valid record is copied into the internal +0x08 member. Invalid
		// input must preserve both wrapper and internal bytes.
		if(validCase)
			{
			NxMeshData copied;
			memcpy(&copied, oracleRecord + 8, sizeof(copied));
			if(memcmp(&copied, &input, sizeof(input)) != 0
				|| memcmp(oracleWrapper, oracleBefore, 0x0c) != 0
				|| memcmp(oracleWrapper + 0x18, oracleBefore + 0x18, 0x28) != 0
				|| memcmp(candidateWrapper, candidateBefore, 0x0c) != 0
				|| memcmp(candidateWrapper + 0x18, candidateBefore + 0x18, 0x28) != 0)
				++wrapperMismatches;
			}
		else if(memcmp(oracleWrapper, oracleBefore, 0x0c) != 0
			|| memcmp(oracleWrapper + 0x18, oracleBefore + 0x18, 0x28) != 0
			|| memcmp(candidateWrapper, candidateBefore, 0x0c) != 0
			|| memcmp(candidateWrapper + 0x18, candidateBefore + 0x18, 0x28) != 0
			|| memcmp(oracleRecord, oracleRecordBefore, sizeof(oracleRecord)) != 0
			|| memcmp(candidateRecord, candidateRecordBefore, sizeof(candidateRecord)) != 0)
			++wrapperMismatches;
		oracleLock.held = candidateLock.held = 0;
		oracleLock.owner = candidateLock.owner = 0;
		}
	*const_cast<bool*>(oracleApiReentry) = oracleApiReentryBefore;
	reinterpret_cast<void (__cdecl*)(bool)>(candidateApiReentrySetter)(candidateApiReentryBefore);
	DeleteCriticalSection(&oracleLock.section);
	DeleteCriticalSection(&candidateLock.section);
	printf("implicit mesh setMeshData public wrapper valid=%u invalid=%u "
		"lock_rejected=%u reentry_rejected=%u stack_checks=%u mismatches=%u\n",
		wrapperValid, wrapperInvalid, wrapperLockRejected, wrapperReentryRejected,
		wrapperStackChecks, wrapperMismatches);
	mismatches += wrapperMismatches;
	foundation->release();
	printf("implicit mesh setMeshData valid_cases=%u invalid_cases=%u "
		"errors=%u stack_checks=%u mismatches=%u\n", validCases, invalidCases,
		invalidCases, stackChecks, mismatches);
	return mismatches;
	}

struct NxFluidEmitterFlagCallbackRecord
	{
	unsigned kind;
	unsigned argument0;
	unsigned argument1;
	unsigned argument2;
	unsigned enabled;
	unsigned pose[12];
	};

static NxFluidEmitterFlagCallbackRecord gOracleFlagCallbacks[16];
static unsigned gOracleFlagCallbackCount;
static float gOracleEmitterGlobalPose[12];

struct NxFluidPostStepCallbackRecord
	{
	unsigned kind;
	unsigned system;
	unsigned handle;
	unsigned values[6];
	};

struct NxFluidRunCallbackRecord
	{
	unsigned system;
	unsigned fluid;
	unsigned state;
	unsigned vector[3];
	unsigned timestep;
	unsigned option;
	};

struct NxFluidRepulsionCallbackRecord
	{
	unsigned kind;
	unsigned system;
	unsigned fluid;
	unsigned emitter;
	unsigned vectors[6];
	unsigned mass;
	};

static NxFluidRepulsionCallbackRecord gOracleFluidRepulsionRecords[16];
static unsigned gOracleFluidRepulsionRecordCount;

extern "C" int __cdecl nxOracleEmitterGetEnabledFlag(
	unsigned, unsigned, unsigned);
extern "C" int __cdecl nxOracleEmitterGetBodyRepulsion(
	unsigned, unsigned, unsigned, float*, float*);
extern "C" float __cdecl nxOracleFluidGetParticleMass(unsigned, unsigned);

struct NxFluidPostStepParticleStream
	{
	unsigned* data;
	unsigned count;
	};

struct NxImplicitFluidMeshFixture
	{
	unsigned* leadingVectors;
	unsigned leadingCount;
	unsigned unused0;
	unsigned* surfaceVertices;
	unsigned surfaceCount;
	unsigned unused1;
	unsigned* surfaceNormals;
	};

static NxFluidPostStepCallbackRecord gOraclePostStepRecords[16];
static unsigned gOraclePostStepRecordCount;
static unsigned gOraclePostStepParticles[2][2][23];
static NxFluidPostStepParticleStream gOraclePostStepStreams[2];
static unsigned gOracleLeadingVectors[2][3] = {
	{ 0x3e800001u, 0x3e800002u, 0x3e800003u },
	{ 0x3f000001u, 0x3f000002u, 0x3f000003u }
	};
static unsigned gOracleSurfaceVertices[2][3] = {
	{ 0x3f400001u, 0x3f400002u, 0x3f400003u },
	{ 0x3f800001u, 0x3f800002u, 0x3f800003u }
	};
static unsigned gOracleSurfaceNormals[2][3] = {
	{ 0x3fc00001u, 0x3fc00002u, 0x3fc00003u },
	{ 0x40000001u, 0x40000002u, 0x40000003u }
	};
static NxImplicitFluidMeshFixture gOraclePostStepMeshes[2];

static void nxRecordOraclePostStep(unsigned kind, unsigned system,
	unsigned handle, const unsigned* values, unsigned count)
	{
	if(gOraclePostStepRecordCount >= sizeof(gOraclePostStepRecords) / sizeof(gOraclePostStepRecords[0]))
		return;
	NxFluidPostStepCallbackRecord& record = gOraclePostStepRecords[gOraclePostStepRecordCount++];
	memset(&record, 0, sizeof(record));
	record.kind = kind;
	record.system = system;
	record.handle = handle;
	for(unsigned index = 0; values && index < count && index < 6; ++index)
		record.values[index] = values[index];
	}

extern "C" void __cdecl nxOracleFluidGetBounds(unsigned system,
	unsigned handle, float* minimum, float* maximum)
	{
	static const unsigned values[2][6] = {
		{ 0x3f800000u, 0x40000000u, 0x40400000u, 0x40800000u, 0x40a00000u, 0x40c00000u },
		{ 0x41300000u, 0x41400000u, 0x41500000u, 0x41600000u, 0x41700000u, 0x41800000u }
		};
	const unsigned index = system & 1u;
	if(minimum)
		memcpy(minimum, values[index], 3 * sizeof(unsigned));
	if(maximum)
		memcpy(maximum, values[index] + 3, 3 * sizeof(unsigned));
	nxRecordOraclePostStep(4096, system, handle, values[index], 6);
	}

extern "C" NxFluidPostStepParticleStream* __cdecl nxOracleFluidGetParticles(
	unsigned system, unsigned handle)
	{
	nxRecordOraclePostStep(8192, system, handle, 0, 0);
	return &gOraclePostStepStreams[system & 1u];
	}

extern "C" NxImplicitFluidMeshFixture* __cdecl nxOracleImplicitFluidMeshGetMesh(
	unsigned system, unsigned handle)
	{
	const unsigned index = system & 1u;
	nxRecordOraclePostStep(16384, system, handle, 0, 0);
	NxImplicitFluidMeshFixture& mesh = gOraclePostStepMeshes[index];
	mesh.leadingVectors = gOracleLeadingVectors[index];
	mesh.leadingCount = 1;
	mesh.unused0 = 0;
	mesh.surfaceVertices = gOracleSurfaceVertices[index];
	mesh.surfaceCount = 1;
	mesh.unused1 = 0;
	mesh.surfaceNormals = gOracleSurfaceNormals[index];
	return &mesh;
	}

static int nxRecordOracleFluidEmitterFlag(unsigned kind, unsigned argument0,
	unsigned argument1, unsigned argument2, unsigned char enabled)
	{
	if(gOracleFlagCallbackCount < sizeof(gOracleFlagCallbacks) / sizeof(gOracleFlagCallbacks[0]))
		{
		NxFluidEmitterFlagCallbackRecord& record = gOracleFlagCallbacks[gOracleFlagCallbackCount++];
		memset(&record, 0, sizeof(record));
		record.kind = kind;
		record.argument0 = argument0;
		record.argument1 = argument1;
		record.argument2 = argument2;
		record.enabled = enabled;
		}
	return static_cast<int>(kind ^ argument0 ^ argument1 ^ argument2 ^ enabled);
	}

extern "C" int __cdecl nxOracleEmitterSetBodyRepulsionFlag(
	unsigned a0, unsigned a1, unsigned a2, unsigned char enabled)
	{ return nxRecordOracleFluidEmitterFlag(4, a0, a1, a2, enabled); }
extern "C" int __cdecl nxOracleEmitterSetAddBodyVelocityFlag(
	unsigned a0, unsigned a1, unsigned a2, unsigned char enabled)
	{ return nxRecordOracleFluidEmitterFlag(8, a0, a1, a2, enabled); }
extern "C" int __cdecl nxOracleEmitterSetEnabledFlag(
	unsigned a0, unsigned a1, unsigned a2, unsigned char enabled)
	{ return nxRecordOracleFluidEmitterFlag(16, a0, a1, a2, enabled); }

extern "C" const float* __cdecl nxOracleEmitterGetGlobalPose(
	unsigned a0, unsigned a1, unsigned a2)
	{
	nxRecordOracleFluidEmitterFlag(32, a0, a1, a2, 0);
	const unsigned emitterHandle = a2;
	nxRecordOraclePostStep(32768, a0, a1, &emitterHandle, 1);
	return gOracleEmitterGlobalPose;
	}

extern "C" int __cdecl nxOracleEmitterSetGlobalPose(unsigned a0, unsigned a1,
	unsigned a2, const float* pose)
	{
	nxRecordOracleFluidEmitterFlag(64, a0, a1, a2, 0);
	if(gOracleFlagCallbackCount && pose)
		memcpy(gOracleFlagCallbacks[gOracleFlagCallbackCount - 1].pose,
			pose, sizeof(gOracleFlagCallbacks[0].pose));
	if(pose)
		memcpy(gOracleEmitterGlobalPose, pose, sizeof(gOracleEmitterGlobalPose));
	return 0;
	}

extern "C" int __cdecl nxOracleEmitterSetAttachedFlag(unsigned a0, unsigned a1,
	unsigned a2, unsigned char attached)
	{
	return nxRecordOracleFluidEmitterFlag(128, a0, a1, a2, attached);
	}

static int nxRecordOracleFluidEmitterBodyVector(unsigned kind, unsigned a0,
	unsigned a1, unsigned a2, const float* vector)
	{
	const int result = nxRecordOracleFluidEmitterFlag(kind, a0, a1, a2, 0);
	if(gOracleFlagCallbackCount && vector)
		memcpy(gOracleFlagCallbacks[gOracleFlagCallbackCount - 1].pose,
			vector, 3 * sizeof(float));
	return result;
	}

extern "C" int __cdecl nxOracleEmitterSetBodyPosition(unsigned a0, unsigned a1,
	unsigned a2, const float* position)
	{ return nxRecordOracleFluidEmitterBodyVector(256, a0, a1, a2, position); }
extern "C" int __cdecl nxOracleEmitterSetBodyLinearVelocity(unsigned a0, unsigned a1,
	unsigned a2, const float* velocity)
	{ return nxRecordOracleFluidEmitterBodyVector(512, a0, a1, a2, velocity); }
extern "C" int __cdecl nxOracleEmitterSetBodyAngularVelocity(unsigned a0, unsigned a1,
	unsigned a2, const float* velocity)
	{ return nxRecordOracleFluidEmitterBodyVector(1024, a0, a1, a2, velocity); }

extern "C" int __cdecl nxOracleImplicitFluidMeshRun(unsigned a0, unsigned a1,
	unsigned a2, unsigned a3)
	{
	if(gOracleFlagCallbackCount < sizeof(gOracleFlagCallbacks) / sizeof(gOracleFlagCallbacks[0]))
		{
		NxFluidEmitterFlagCallbackRecord& record = gOracleFlagCallbacks[gOracleFlagCallbackCount++];
		memset(&record, 0, sizeof(record));
		record.kind = 2048;
		record.argument0 = a0;
		record.argument1 = a1;
		record.argument2 = a2;
		record.enabled = a3;
		}
	return static_cast<int>(2048u ^ a0 ^ a1 ^ a2 ^ a3);
	}

static NxFluidRepulsionCallbackRecord& nxRecordOracleFluidRepulsion(
	unsigned kind, unsigned system, unsigned fluid, unsigned emitter)
	{
	static NxFluidRepulsionCallbackRecord overflow = {};
	NxFluidRepulsionCallbackRecord& record = gOracleFluidRepulsionRecordCount
		< sizeof(gOracleFluidRepulsionRecords) / sizeof(gOracleFluidRepulsionRecords[0])
		? gOracleFluidRepulsionRecords[gOracleFluidRepulsionRecordCount++] : overflow;
	memset(&record, 0, sizeof(record));
	record.kind = kind;
	record.system = system;
	record.fluid = fluid;
	record.emitter = emitter;
	return record;
	}

extern "C" int __cdecl nxOracleEmitterGetEnabledFlag(
	unsigned system, unsigned fluid, unsigned emitter)
	{
	nxRecordOracleFluidRepulsion(1, system, fluid, emitter);
	return 1;
	}

extern "C" int __cdecl nxOracleEmitterGetBodyRepulsion(
	unsigned system, unsigned fluid, unsigned emitter, float* force, float* torque)
	{
	NxFluidRepulsionCallbackRecord& record = nxRecordOracleFluidRepulsion(
		2, system, fluid, emitter);
	static const unsigned values[6] = {
		0x3e800001u, 0xbf000002u, 0x3f400003u,
		0x40000004u, 0xc0400005u, 0x40800006u };
	memcpy(record.vectors, values, sizeof(values));
	memcpy(force, values, 3 * sizeof(float));
	memcpy(torque, values + 3, 3 * sizeof(float));
	return 0;
	}

extern "C" float __cdecl nxOracleFluidGetParticleMass(unsigned system,
	unsigned fluid)
	{
	NxFluidRepulsionCallbackRecord& record = nxRecordOracleFluidRepulsion(
		3, system, fluid, 0);
	const unsigned mass = 0x3f2aaaab;
	record.mass = mass;
	return *reinterpret_cast<const float*>(&mass);
	}

static void __cdecl nxOracleFluidFindCell(int* foundIndex, int* cellStorage,
	int* bucket, const int* key)
	{
	*foundIndex = -1;
	const unsigned first = static_cast<unsigned>(bucket[1]);
	const unsigned count = static_cast<unsigned>(bucket[0]);
	const unsigned char* const records = reinterpret_cast<const unsigned char*>(
		static_cast<uintptr_t>(*cellStorage));
	for(unsigned i = 0; i < count; ++i)
		{
		const unsigned char* const record = records + (first + i) * 16;
		const unsigned xy = *reinterpret_cast<const unsigned*>(record);
		const short z = *reinterpret_cast<const short*>(record + 4);
		if(xy == static_cast<unsigned>(key[0])
			&& z == *reinterpret_cast<const short*>(key + 1))
			{
			*foundIndex = static_cast<int>(first + i);
			return;
			}
		}
	}

static bool nxBindOracleEmitterFlagCallbacks(unsigned char* image, void** oldValues,
	DWORD* oldProtection, void* triangleBoxOverlap, void* triangleCellGrow)
	{
	static const unsigned slotRvas[] = {
		0x1263c0,
		0x126628, 0x126404, 0x12657c, 0x12644c, 0x126614, 0x1263dc,
		0x1263f0, 0x12664c, 0x1263e8, 0x1264f8, 0x126584, 0x126590, 0x1264d0,
		0x126408, 0x1263e0, 0x126518, 0x126524, 0x126454, 0x12658c };
	static void* const replacements[] = {
		0,
		reinterpret_cast<void*>(&nxOracleEmitterSetBodyRepulsionFlag),
		reinterpret_cast<void*>(&nxOracleEmitterSetAddBodyVelocityFlag),
		reinterpret_cast<void*>(&nxOracleEmitterSetEnabledFlag),
		reinterpret_cast<void*>(&nxOracleEmitterGetGlobalPose),
		reinterpret_cast<void*>(&nxOracleEmitterSetGlobalPose),
		reinterpret_cast<void*>(&nxOracleEmitterSetAttachedFlag),
		reinterpret_cast<void*>(&nxOracleEmitterSetBodyPosition),
		reinterpret_cast<void*>(&nxOracleEmitterSetBodyLinearVelocity),
		reinterpret_cast<void*>(&nxOracleEmitterSetBodyAngularVelocity),
		reinterpret_cast<void*>(&nxOracleImplicitFluidMeshRun),
		reinterpret_cast<void*>(&nxOracleFluidGetParticles),
		reinterpret_cast<void*>(&nxOracleFluidGetBounds),
		reinterpret_cast<void*>(&nxOracleImplicitFluidMeshGetMesh),
		reinterpret_cast<void*>(&nxOracleEmitterGetEnabledFlag),
		reinterpret_cast<void*>(&nxOracleEmitterGetBodyRepulsion),
		reinterpret_cast<void*>(&nxOracleFluidGetParticleMass),
		triangleBoxOverlap,
		triangleCellGrow,
		reinterpret_cast<void*>(&nxOracleFluidFindCell),
		};
	void* page = reinterpret_cast<void*>(reinterpret_cast<size_t>(image + 0x126000)
		& ~static_cast<size_t>(0xfff));
	if(!VirtualProtect(page, 0x1000, PAGE_READWRITE, oldProtection))
		return false;
	for(unsigned i = 0; i < 20; ++i)
		{
		void** slot = reinterpret_cast<void**>(image + slotRvas[i]);
		oldValues[i] = *slot;
		*slot = replacements[i];
		}
	return true;
	}

static void nxUnbindOracleEmitterFlagCallbacks(unsigned char* image, void** oldValues,
	DWORD oldProtection)
	{
	static const unsigned slotRvas[] = {
		0x1263c0,
		0x126628, 0x126404, 0x12657c, 0x12644c, 0x126614, 0x1263dc,
		0x1263f0, 0x12664c, 0x1263e8, 0x1264f8, 0x126584, 0x126590, 0x1264d0,
		0x126408, 0x1263e0, 0x126518, 0x126524, 0x126454, 0x12658c };
	DWORD ignored = 0;
	void* page = reinterpret_cast<void*>(reinterpret_cast<size_t>(image + 0x126000)
		& ~static_cast<size_t>(0xfff));
	VirtualProtect(page, 0x1000, PAGE_READWRITE, &ignored);
	for(unsigned i = 0; i < 20; ++i)
		*reinterpret_cast<void**>(image + slotRvas[i]) = oldValues[i];
	VirtualProtect(page, 0x1000, oldProtection, &ignored);
	}

static bool nxLoadFluidModelCallbackFixture(HMODULE* module, FARPROC* reset,
	FARPROC* count, FARPROC* getRecord)
	{
	wchar_t path[MAX_PATH];
	if(!GetModuleFileNameW(0, path, MAX_PATH))
		return false;
	wchar_t* slash = wcsrchr(path, L'\\');
	if(!slash || static_cast<size_t>(slash - path) + 1 + 16 >= MAX_PATH)
		return false;
	wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - path), L"FluidModel.dll");
	*module = LoadLibraryExW(path, 0, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
	if(!*module)
		return false;
	*reset = GetProcAddress(*module, "NxResetFluidEmitterFlagCallbacks");
	*count = GetProcAddress(*module, "NxGetFluidEmitterFlagCallbackCount");
	*getRecord = GetProcAddress(*module, "NxGetFluidEmitterFlagCallbackRecord");
	return *reset && *count && *getRecord;
	}

static bool nxResolveCandidateFluidEmitterSymbol(HMODULE module, const char* expected,
	void** address)
	{
	wchar_t mapPath[MAX_PATH];
	if(!GetModuleFileNameW(0, mapPath, MAX_PATH))
		return false;
	wchar_t* slash = wcsrchr(mapPath, L'\\');
	if(!slash || static_cast<size_t>(slash - mapPath) + 1 + 14 >= MAX_PATH)
		return false;
	wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - mapPath), L"NxPhysics.map");
	std::ifstream map(mapPath);
	if(!map)
		{ fprintf(stderr, "candidate NxPhysics map open failed\n"); return false; }
	unsigned preferredAddress = 0;
	unsigned preferredImageBase = 0;
	std::string line;
	while(std::getline(map, line))
		{
		static const char preferredBaseMarker[] = "Preferred load address is ";
		std::string::size_type basePosition = line.find(preferredBaseMarker);
		if(basePosition != std::string::npos)
			{
			std::istringstream baseField(line.substr(basePosition + sizeof(preferredBaseMarker) - 1));
			baseField >> std::hex >> preferredImageBase;
			}
		std::string::size_type symbolPosition = line.find(expected);
		if(symbolPosition == std::string::npos)
			continue;
		std::istringstream addressField(line.substr(symbolPosition + strlen(expected)));
		std::string addressText;
		addressField >> addressText;
		std::istringstream addressValue(addressText);
		addressValue >> std::hex >> preferredAddress;
		if(!addressField || !addressValue)
			{ fprintf(stderr, "candidate NxPhysics map address parse failed: %s\n", line.c_str()); return false; }
		IMAGE_DOS_HEADER* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(module);
		if(dos->e_magic != IMAGE_DOS_SIGNATURE)
			{ fprintf(stderr, "candidate NxPhysics DOS signature mismatch\n"); return false; }
		IMAGE_NT_HEADERS32* nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(
			reinterpret_cast<unsigned char*>(module) + dos->e_lfanew);
		if(nt->Signature != IMAGE_NT_SIGNATURE || !preferredImageBase
			|| preferredAddress < preferredImageBase)
			{ fprintf(stderr, "candidate NxPhysics PE/map base mismatch: map=%08x preferred=%08x sig=%08x\n", preferredAddress, preferredImageBase, nt->Signature); return false; }
		*address = reinterpret_cast<unsigned char*>(module)
			+ (preferredAddress - preferredImageBase);
		return true;
		}
	fprintf(stderr, "candidate NxPhysics map has no symbol: %s\n", expected);
	return false;
	}

static unsigned nxRunFrictionPatchAssignment(void* oracleAddress,
	void* candidateAddress)
	{
	if(!oracleAddress || !candidateAddress)
		{
		fprintf(stderr, "FAIL friction-patch assignment row address unavailable\n");
		printf("contact patch assignment cases=0 oracle_return=0 candidate_return=0 "
			"payload_equal=0 vptr_preserved=0 padding_preserved=0 stack=0/0 mismatches=1\n");
		return 1;
		}
	typedef void* (__thiscall *AssignFn)(void*, const void*);
	AssignFn oracleAssign = reinterpret_cast<AssignFn>(oracleAddress);
	AssignFn candidateAssign = reinterpret_cast<AssignFn>(candidateAddress);
	unsigned mismatches = 0;
	unsigned oracleReturns = 0, candidateReturns = 0;
	unsigned payloadEqual = 0, vptrPreserved = 0, paddingPreserved = 0;
	unsigned stackBalanced = 0;
	for(unsigned test = 0; test != 3; ++test)
		{
		unsigned char oracleDestination[0x78], candidateDestination[0x78];
		unsigned char oracleSource[0x78], candidateSource[0x78];
		unsigned char expected[0x78];
		for(unsigned i = 0; i != sizeof(expected); ++i)
			{
			const unsigned char destinationByte = static_cast<unsigned char>(
				0x13u + test * 29u + i * 7u);
			const unsigned char sourceByte = static_cast<unsigned char>(
				0x91u + test * 17u + i * 11u);
			oracleDestination[i] = candidateDestination[i] = destinationByte;
			oracleSource[i] = candidateSource[i] = sourceByte;
			expected[i] = destinationByte;
			}
		memcpy(expected + 4, oracleSource + 4, 0x72);
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		void* const oracleResult = oracleAssign(oracleDestination, oracleSource);
		__asm mov oracleAfter, esp
		__asm mov candidateBefore, esp
		void* const candidateResult = candidateAssign(candidateDestination, candidateSource);
		__asm mov candidateAfter, esp
		const bool oracleReturn = oracleResult == oracleDestination;
		const bool candidateReturn = candidateResult == candidateDestination;
		const bool equal = memcmp(oracleDestination, candidateDestination,
			sizeof(oracleDestination)) == 0
			&& memcmp(oracleDestination, expected, sizeof(expected)) == 0;
		const bool preservedVptr = memcmp(oracleDestination, expected, 4) == 0
			&& memcmp(candidateDestination, expected, 4) == 0;
		const bool preservedPadding = memcmp(oracleDestination + 0x76,
			expected + 0x76, 2) == 0
			&& memcmp(candidateDestination + 0x76, expected + 0x76, 2) == 0;
		const bool balanced = oracleBefore == oracleAfter
			&& candidateBefore == candidateAfter;
		oracleReturns += oracleReturn;
		candidateReturns += candidateReturn;
		payloadEqual += equal;
		vptrPreserved += preservedVptr;
		paddingPreserved += preservedPadding;
		stackBalanced += balanced;
		if(!oracleReturn || !candidateReturn || !equal || !preservedVptr
			|| !preservedPadding || !balanced)
			++mismatches;
		}
	printf("contact patch assignment cases=3 oracle_return=%u candidate_return=%u "
		"payload_equal=%u vptr_preserved=%u padding_preserved=%u stack=%u/3 mismatches=%u\n",
		oracleReturns, candidateReturns, payloadEqual, vptrPreserved,
		paddingPreserved, stackBalanced, mismatches);
	return mismatches;
	}

static unsigned nxRunFluidEmitterGlobalPoseRows(void* oracleStoreAddress,
	void* candidateStoreAddress, void* oracleCopyAddress, void* candidateCopyAddress,
	void* fluidModel, void (__cdecl *resetCallbacks)(),
	unsigned (__cdecl *callbackCount)(),
	int (__cdecl *getCallbackRecord)(unsigned, NxFluidEmitterFlagCallbackRecord*))
	{
	if(!oracleStoreAddress || !candidateStoreAddress || !oracleCopyAddress
		|| !candidateCopyAddress || !fluidModel || !resetCallbacks
		|| !callbackCount || !getCallbackRecord)
		return 1;
	typedef void (__cdecl *SetEmitterPoseFn)(const float*);
	SetEmitterPoseFn setEmitterPose = reinterpret_cast<SetEmitterPoseFn>(
		GetProcAddress(static_cast<HMODULE>(fluidModel), "NxSetEmitterGlobalPose"));
	if(!setEmitterPose)
		return 1;
	static const unsigned poseWords[12] = {
		0x3f800001u, 0xbf000002u, 0x40000003u, 0xc0400004u,
		0x40800005u, 0x41000006u, 0xc1200007u, 0x41800008u,
		0x42000009u, 0x4220000au, 0xc230000bu, 0x4240000cu };
	static const unsigned expectedPoseWords[12] = {
		0x3f800001u, 0xc0400004u, 0xc1200007u, 0xbf000002u,
		0x40800005u, 0x41800008u, 0x40000003u, 0x41000006u,
		0x42000009u, 0x4220000au, 0xc230000bu, 0x4240000cu };
	const unsigned context = 0x12345678u;
	const unsigned fluidHandle = 0x23456789u;
	const unsigned emitterHandle = 0x3456789au;
	unsigned mismatches = 0;
	unsigned stackChecks = 0;
	unsigned callbackChecks = 0;
	unsigned char oracleFluid[0x84] = {};
	unsigned char candidateFluid[0x84] = {};
	unsigned char oracleBackend[0x34] = {};
	unsigned char candidateBackend[0x34] = {};
	unsigned char oracleEmitter[0x78] = {};
	unsigned char candidateEmitter[0x78] = {};
	*reinterpret_cast<void**>(oracleFluid + 0x7c) = oracleBackend;
	*reinterpret_cast<void**>(candidateFluid + 0x7c) = candidateBackend;
	*reinterpret_cast<unsigned*>(oracleFluid + 0x80) = fluidHandle;
	*reinterpret_cast<unsigned*>(candidateFluid + 0x80) = fluidHandle;
	*reinterpret_cast<unsigned*>(oracleBackend + 0x30) = context;
	*reinterpret_cast<unsigned*>(candidateBackend + 0x30) = context;
	*reinterpret_cast<void**>(oracleEmitter + 4) = oracleFluid;
	*reinterpret_cast<void**>(candidateEmitter + 4) = candidateFluid;
	*reinterpret_cast<unsigned*>(oracleEmitter + 8) = emitterHandle;
	*reinterpret_cast<unsigned*>(candidateEmitter + 8) = emitterHandle;
	memcpy(gOracleEmitterGlobalPose, poseWords, sizeof(poseWords));
	setEmitterPose(reinterpret_cast<const float*>(poseWords));
	typedef void* (__thiscall *OracleStore)(void*);
	typedef void* (__cdecl *CandidateStore)(void*);
	unsigned oracleBefore = 0, oracleAfter = 0;
	unsigned candidateBefore = 0, candidateAfter = 0;
	gOracleFlagCallbackCount = 0;
	__asm mov oracleBefore, esp
	const void* oracleStoreResult = reinterpret_cast<OracleStore>(oracleStoreAddress)(oracleEmitter);
	__asm mov oracleAfter, esp
	resetCallbacks();
	__asm mov candidateBefore, esp
	const void* candidateStoreResult = reinterpret_cast<CandidateStore>(candidateStoreAddress)(candidateEmitter);
	__asm mov candidateAfter, esp
	NxFluidEmitterFlagCallbackRecord candidateRecord = {};
	const bool oracleCallback = gOracleFlagCallbackCount == 1
		&& gOracleFlagCallbacks[0].kind == 32
		&& gOracleFlagCallbacks[0].argument0 == context
		&& gOracleFlagCallbacks[0].argument1 == fluidHandle
		&& gOracleFlagCallbacks[0].argument2 == emitterHandle;
	const bool candidateCallback = callbackCount() == 1
		&& getCallbackRecord(0, &candidateRecord)
		&& candidateRecord.kind == 32
		&& candidateRecord.argument0 == context
		&& candidateRecord.argument1 == fluidHandle
		&& candidateRecord.argument2 == emitterHandle;
	callbackChecks += oracleCallback && candidateCallback;
	if(!oracleCallback || !candidateCallback
		|| oracleStoreResult != oracleEmitter + 0x48
		|| candidateStoreResult != candidateEmitter + 0x48
		|| memcmp(oracleEmitter + 0x48, expectedPoseWords, sizeof(expectedPoseWords)) != 0
		|| memcmp(candidateEmitter + 0x48, expectedPoseWords, sizeof(expectedPoseWords)) != 0
		|| memcmp(oracleEmitter + 0x48, candidateEmitter + 0x48, sizeof(poseWords)) != 0
		|| oracleBefore != oracleAfter || candidateBefore != candidateAfter)
		++mismatches;
	stackChecks += oracleBefore == oracleAfter && candidateBefore == candidateAfter;

	unsigned char oracleOutput[0x38], candidateOutput[0x38];
	memset(oracleOutput, 0xa5, sizeof(oracleOutput));
	memset(candidateOutput, 0xa5, sizeof(candidateOutput));
	memcpy(gOracleEmitterGlobalPose, poseWords, sizeof(poseWords));
	setEmitterPose(reinterpret_cast<const float*>(poseWords));
	resetCallbacks();
	gOracleFlagCallbackCount = 0;
	typedef void* (__thiscall *OracleCopy)(void*, void*);
	typedef void* (__cdecl *CandidateCopy)(void*, void*);
	__asm mov oracleBefore, esp
	const void* oracleCopyResult = reinterpret_cast<OracleCopy>(oracleCopyAddress)(oracleEmitter, oracleOutput);
	__asm mov oracleAfter, esp
	resetCallbacks();
	__asm mov candidateBefore, esp
	const void* candidateCopyResult = reinterpret_cast<CandidateCopy>(candidateCopyAddress)(candidateEmitter, candidateOutput);
	__asm mov candidateAfter, esp
	memset(&candidateRecord, 0, sizeof(candidateRecord));
	const bool oracleCopyCallback = gOracleFlagCallbackCount == 1
		&& gOracleFlagCallbacks[0].kind == 32
		&& gOracleFlagCallbacks[0].argument0 == context
		&& gOracleFlagCallbacks[0].argument1 == fluidHandle
		&& gOracleFlagCallbacks[0].argument2 == emitterHandle;
	const bool candidateCopyCallback = callbackCount() == 1
		&& getCallbackRecord(0, &candidateRecord)
		&& candidateRecord.kind == 32
		&& candidateRecord.argument0 == context
		&& candidateRecord.argument1 == fluidHandle
		&& candidateRecord.argument2 == emitterHandle;
	callbackChecks += oracleCopyCallback && candidateCopyCallback;
	if(!oracleCopyCallback || !candidateCopyCallback
		|| oracleCopyResult != oracleOutput || candidateCopyResult != candidateOutput
		|| memcmp(oracleOutput, expectedPoseWords, sizeof(expectedPoseWords)) != 0
		|| memcmp(candidateOutput, expectedPoseWords, sizeof(expectedPoseWords)) != 0
		|| memcmp(oracleOutput, candidateOutput, sizeof(expectedPoseWords)) != 0
		|| oracleBefore != oracleAfter || candidateBefore != candidateAfter
		|| memcmp(oracleOutput + 0x30, "\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5", 8) != 0
		|| memcmp(candidateOutput + 0x30, "\xa5\xa5\xa5\xa5\xa5\xa5\xa5\xa5", 8) != 0)
		++mismatches;
	stackChecks += oracleBefore == oracleAfter && candidateBefore == candidateAfter;
	printf("fluid emitter global-pose rows=2 callback_checks=%u stack_checks=%u mismatches=%u\n",
		callbackChecks, stackChecks, mismatches);
	return mismatches;
	}

// phys_fn_003611 converts a requested world pose to the emitter's local frame.
static unsigned nxRunFluidEmitterSetGlobalPoseInternal(void* oracleAddress,
	void* candidateAddress, void* fluidModel,
	void (__cdecl *resetCallbacks)(), unsigned (__cdecl *callbackCount)(),
	int (__cdecl *getCallbackRecord)(unsigned, NxFluidEmitterFlagCallbackRecord*))
	{
	if(!oracleAddress || !candidateAddress || !fluidModel || !resetCallbacks
		|| !callbackCount || !getCallbackRecord)
		return 1;
	typedef int (__thiscall *OracleSetPose)(void*, const float*);
	typedef int (__cdecl *CandidateSetPose)(void*, const float*);
	OracleSetPose oracleSetPose = reinterpret_cast<OracleSetPose>(oracleAddress);
	CandidateSetPose candidateSetPose = reinterpret_cast<CandidateSetPose>(candidateAddress);
	static const float globalPose[12] = {
		0.812345f, -0.573219f, 0.105679f, 0.581357f, 0.798461f, -0.153791f,
		0.024681f, 0.184357f, 0.982531f, 5.12345f, -6.54321f, 7.67891f };
	static const float actorPose[12] = {
		0.934567f, -0.271357f, 0.127531f, 0.298461f, 0.912345f, -0.153791f,
		-0.024681f, 0.184357f, 0.982531f, 2.12345f, -3.54321f, 1.67891f };
	const unsigned context = 0x6a1b2c3du;
	const unsigned fluidHandle = 0x7b2c3d4eu;
	const unsigned emitterHandle = 0x8c3d4e5fu;
	unsigned mismatches = 0, stackChecks = 0, callbackChecks = 0;
	for(unsigned test = 0; test < 3; ++test)
		{
		unsigned char oracleInternal[0x90], candidateInternal[0x90];
		unsigned char oracleFluid[0x84], candidateFluid[0x84];
		unsigned char oracleBackend[0x34], candidateBackend[0x34];
		unsigned char oracleActor[0x50], candidateActor[0x50];
		unsigned char oracleBody[0x40], candidateBody[0x40];
		memset(oracleInternal, 0, sizeof(oracleInternal));
		memset(candidateInternal, 0, sizeof(candidateInternal));
		memset(oracleFluid, 0, sizeof(oracleFluid));
		memset(candidateFluid, 0, sizeof(candidateFluid));
		memset(oracleBackend, 0, sizeof(oracleBackend));
		memset(candidateBackend, 0, sizeof(candidateBackend));
		memset(oracleActor, 0, sizeof(oracleActor));
		memset(candidateActor, 0, sizeof(candidateActor));
		memset(oracleBody, 0, sizeof(oracleBody));
		memset(candidateBody, 0, sizeof(candidateBody));
		*reinterpret_cast<void**>(oracleInternal + 4) = oracleFluid;
		*reinterpret_cast<void**>(candidateInternal + 4) = candidateFluid;
		*reinterpret_cast<void**>(oracleInternal + 0x14) = 0;
		*reinterpret_cast<void**>(candidateInternal + 0x14) = 0;
		*reinterpret_cast<unsigned*>(oracleInternal + 8) = emitterHandle;
		*reinterpret_cast<unsigned*>(candidateInternal + 8) = emitterHandle;
		*reinterpret_cast<void**>(oracleFluid + 0x7c) = oracleBackend;
		*reinterpret_cast<void**>(candidateFluid + 0x7c) = candidateBackend;
		*reinterpret_cast<unsigned*>(oracleFluid + 0x80) = fluidHandle;
		*reinterpret_cast<unsigned*>(candidateFluid + 0x80) = fluidHandle;
		*reinterpret_cast<unsigned*>(oracleBackend + 0x30) = context;
		*reinterpret_cast<unsigned*>(candidateBackend + 0x30) = context;
		for(unsigned word = 0; word < 12; ++word)
			{
			const unsigned seed = 0x3e800000u + test * 0x100u + word;
			*reinterpret_cast<unsigned*>(oracleInternal + 0x18 + word * 4) = seed;
			*reinterpret_cast<unsigned*>(candidateInternal + 0x18 + word * 4) = seed;
			}
		if(test != 0)
			{
			*reinterpret_cast<void**>(oracleInternal + 0x14) = oracleActor;
			*reinterpret_cast<void**>(candidateInternal + 0x14) = candidateActor;
			if(test == 1)
				{
				memcpy(oracleActor + 0x20, actorPose, sizeof(actorPose));
				memcpy(candidateActor + 0x20, actorPose, sizeof(actorPose));
				}
			else
				{
				*reinterpret_cast<void**>(oracleActor + 8) = oracleBody;
				*reinterpret_cast<void**>(candidateActor + 8) = candidateBody;
				float* oracleState = reinterpret_cast<float*>(oracleBody);
				float* candidateState = reinterpret_cast<float*>(candidateBody);
				for(unsigned i = 0; i < 3; ++i)
					oracleState[6 + i] = candidateState[6 + i] = actorPose[9 + i];
				oracleState[9] = candidateState[9] = 0.173219f;
				oracleState[10] = candidateState[10] = -0.319753f;
				oracleState[11] = candidateState[11] = 0.427531f;
				oracleState[12] = candidateState[12] = 0.811357f;
			}
		}
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		gOracleFlagCallbackCount = 0;
		__asm mov oracleBefore, esp
		const int oracleResult = oracleSetPose(oracleInternal, globalPose);
		__asm mov oracleAfter, esp
		NxFluidEmitterFlagCallbackRecord oracleRecord = {};
		const unsigned oracleCallbacks = gOracleFlagCallbackCount;
		if(oracleCallbacks == 1)
			oracleRecord = gOracleFlagCallbacks[0];
		resetCallbacks();
		__asm mov candidateBefore, esp
		const int candidateResult = candidateSetPose(candidateInternal, globalPose);
		__asm mov candidateAfter, esp
		NxFluidEmitterFlagCallbackRecord candidateRecord = {};
		const unsigned candidateCallbacks = callbackCount();
		if(candidateCallbacks == 1)
			getCallbackRecord(0, &candidateRecord);
		const bool callbacksMatch = oracleCallbacks == 1 && candidateCallbacks == 1
			&& memcmp(&oracleRecord, &candidateRecord, sizeof(oracleRecord)) == 0;
		const bool stateMatch = memcmp(oracleInternal + 0x18,
			candidateInternal + 0x18, 0x60) == 0;
		const bool balanced = oracleBefore == oracleAfter
			&& candidateBefore == candidateAfter;
		callbackChecks += callbacksMatch;
		stackChecks += balanced;
		if(oracleResult != candidateResult || !callbacksMatch || !stateMatch || !balanced)
			{
			++mismatches;
			printf("fluid emitter global-to-local case=%u oracle_ret=%d candidate_ret=%d "
				"callbacks=%u/%u state=%u stack=%u/%u\n", test, oracleResult,
				candidateResult, oracleCallbacks, candidateCallbacks, stateMatch,
				oracleBefore == oracleAfter, candidateBefore == candidateAfter);
			for(unsigned word = 0; word < 24; ++word)
				{
				const unsigned oracleWord = *reinterpret_cast<unsigned*>(oracleInternal + 0x18 + word * 4);
				const unsigned candidateWord = *reinterpret_cast<unsigned*>(candidateInternal + 0x18 + word * 4);
				if(oracleWord != candidateWord)
					printf("  word[%u]=%08x/%08x\n", word, oracleWord, candidateWord);
				}
			if(!callbacksMatch)
				for(unsigned word = 0; word < 12; ++word)
					if(oracleRecord.pose[word] != candidateRecord.pose[word])
						printf("  callback[%u]=%08x/%08x\n", word,
							oracleRecord.pose[word], candidateRecord.pose[word]);
			}
		}
	printf("fluid emitter global-to-local cases=3 callback_checks=%u stack_checks=%u mismatches=%u\n",
		callbackChecks, stackChecks, mismatches);
	return mismatches;
	}

// phys_fn_003616 copies a requested local pose before refreshing the cached
// world pose and dispatching it to FluidModel.
static unsigned nxRunFluidEmitterSetLocalPoseInternal(void* oracleAddress,
	void* candidateAddress,
	void (__cdecl *setFixturePose)(const float*),
	void (__cdecl *resetCallbacks)(), unsigned (__cdecl *callbackCount)(),
	int (__cdecl *getCallbackRecord)(unsigned, NxFluidEmitterFlagCallbackRecord*))
	{
	if(!oracleAddress || !candidateAddress || !setFixturePose || !resetCallbacks
		|| !callbackCount || !getCallbackRecord)
		return 1;
	typedef int (__thiscall *OracleSetLocalPose)(void*, const float*);
	typedef int (__cdecl *CandidateSetLocalPose)(void*, const float*);
	OracleSetLocalPose oracleSetPose = reinterpret_cast<OracleSetLocalPose>(oracleAddress);
	CandidateSetLocalPose candidateSetPose = reinterpret_cast<CandidateSetLocalPose>(candidateAddress);
	static const float poses[2][12] = {
		{ 1, 0, 0, 0, 1, 0, 0, 0, 1, 1.25f, -2.5f, 4 },
		{ 0.812345f, -0.432109f, 0.123457f, 0.456789f, 0.901237f, -0.234567f,
		  0.098765f, 0.345679f, 0.765431f, -0.75319f, 2.24681f, 3.51234f }
	};
	const unsigned context = 0x31415926u;
	const unsigned fluidHandle = 0x27182818u;
	const unsigned emitterHandle = 0x16180339u;
	unsigned mismatches = 0, callbackChecks = 0, stackChecks = 0;
	for(unsigned test = 0; test < 2; ++test)
		{
		unsigned char oracleInternal[0x90], candidateInternal[0x90];
		unsigned char oracleFluid[0x84], candidateFluid[0x84];
		unsigned char oracleBackend[0x34], candidateBackend[0x34];
		memset(oracleInternal, 0xa5, sizeof(oracleInternal));
		memset(candidateInternal, 0xa5, sizeof(candidateInternal));
		memset(oracleFluid, 0, sizeof(oracleFluid));
		memset(candidateFluid, 0, sizeof(candidateFluid));
		memset(oracleBackend, 0, sizeof(oracleBackend));
		memset(candidateBackend, 0, sizeof(candidateBackend));
		*reinterpret_cast<void**>(oracleInternal + 4) = oracleFluid;
		*reinterpret_cast<void**>(candidateInternal + 4) = candidateFluid;
		*reinterpret_cast<void**>(oracleInternal + 0x14) = 0;
		*reinterpret_cast<void**>(candidateInternal + 0x14) = 0;
		*reinterpret_cast<unsigned*>(oracleInternal + 8) = emitterHandle;
		*reinterpret_cast<unsigned*>(candidateInternal + 8) = emitterHandle;
		*reinterpret_cast<void**>(oracleFluid + 0x7c) = oracleBackend;
		*reinterpret_cast<void**>(candidateFluid + 0x7c) = candidateBackend;
		*reinterpret_cast<unsigned*>(oracleFluid + 0x80) = fluidHandle;
		*reinterpret_cast<unsigned*>(candidateFluid + 0x80) = fluidHandle;
		*reinterpret_cast<unsigned*>(oracleBackend + 0x30) = context;
		*reinterpret_cast<unsigned*>(candidateBackend + 0x30) = context;
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		gOracleFlagCallbackCount = 0;
		setFixturePose(poses[test]);
		__asm mov oracleBefore, esp
		const int oracleResult = oracleSetPose(oracleInternal, poses[test]);
		__asm mov oracleAfter, esp
		NxFluidEmitterFlagCallbackRecord oracleRecord = {};
		const unsigned oracleCallbacks = gOracleFlagCallbackCount;
		if(oracleCallbacks == 1)
			oracleRecord = gOracleFlagCallbacks[0];
		resetCallbacks();
		setFixturePose(poses[test]);
		__asm mov candidateBefore, esp
		const int candidateResult = candidateSetPose(candidateInternal, poses[test]);
		__asm mov candidateAfter, esp
		NxFluidEmitterFlagCallbackRecord candidateRecord = {};
		const unsigned candidateCallbacks = callbackCount();
		if(candidateCallbacks == 1)
			getCallbackRecord(0, &candidateRecord);
		const bool callbackMatch = oracleCallbacks == 1 && candidateCallbacks == 1
			&& memcmp(&oracleRecord, &candidateRecord, sizeof(oracleRecord)) == 0;
		const bool stateMatch = memcmp(oracleInternal + 0x18,
			candidateInternal + 0x18, 0x60) == 0;
		const bool balanced = oracleBefore == oracleAfter
			&& candidateBefore == candidateAfter;
		callbackChecks += callbackMatch;
		stackChecks += balanced;
		if(oracleResult != candidateResult || !callbackMatch || !stateMatch || !balanced
			|| memcmp(oracleInternal + 0x60, candidateInternal + 0x60,
				sizeof(oracleInternal) - 0x60) != 0)
			{
			++mismatches;
			printf("fluid emitter local-pose case=%u oracle_ret=%d candidate_ret=%d "
				"callbacks=%u/%u state=%u stack=%u/%u\n", test, oracleResult,
				candidateResult, oracleCallbacks, candidateCallbacks, stateMatch,
				oracleBefore == oracleAfter, candidateBefore == candidateAfter);
			}
		}
	printf("fluid emitter local-pose setter cases=2 callback_checks=%u stack_checks=%u mismatches=%u\n",
		callbackChecks, stackChecks, mismatches);
	return mismatches;
	}

static bool nxLoadCandidateFluidEmitterSetter(HMODULE* module, void** setter)
	{
	wchar_t exePath[MAX_PATH];
	if(!GetModuleFileNameW(0, exePath, MAX_PATH))
		return false;
	wchar_t* slash = wcsrchr(exePath, L'\\');
	if(!slash || static_cast<size_t>(slash - exePath) + 1 + 23 >= MAX_PATH)
		return false;
	wcscpy_s(slash + 1, MAX_PATH - (slash + 1 - exePath), L"NxPhysicsCandidate.dll");
	*module = LoadLibraryExW(exePath, 0, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
	if(!*module)
		{ fprintf(stderr, "candidate NxPhysics load error=%lu\n", GetLastError()); return false; }
	wchar_t loadedPath[MAX_PATH];
	char actualHash[65];
	if(!GetModuleFileNameW(*module, loadedPath, MAX_PATH)
		|| !nxSha256(loadedPath, actualHash))
		{
		fprintf(stderr, "FAIL candidate NxPhysics identity could not be hashed\n");
		return false;
		}
	printf("loaded module=NxPhysicsCandidate.dll path=%S sha256=%s\n",
		loadedPath, actualHash);
	static const char expected[] = "?setFlag@NpFluidEmitter@@UAEXW4NxFluidEmitterFlag@@_N@Z";
	return nxResolveCandidateFluidEmitterSymbol(*module, expected, setter);
	}

static bool nxCandidateAddressBelongsToModule(HMODULE module, const void* address)
	{
	if(!module)
		return false;
	IMAGE_DOS_HEADER* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(module);
	if(dos->e_magic != IMAGE_DOS_SIGNATURE)
		return false;
	IMAGE_NT_HEADERS32* nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(
		reinterpret_cast<unsigned char*>(module) + dos->e_lfanew);
	if(nt->Signature != IMAGE_NT_SIGNATURE)
		return false;
	const size_t base = reinterpret_cast<size_t>(module);
	const size_t value = reinterpret_cast<size_t>(address);
	return value >= base && value < base + nt->OptionalHeader.SizeOfImage;
	}

struct NxFluidCollisionBucketRecord
	{
	unsigned kind;
	unsigned prefix;
	unsigned count;
	};

struct NxFluidCollisionBucketDescriptor
	{
	NxFluidCollisionBucketRecord* records;
	unsigned unknown04;
	unsigned count;
	};

static unsigned nxRunFluidCollisionBucketPrefixDifferential(void* oracleAddress,
	void* candidateAddress)
	{
	if(!oracleAddress || !candidateAddress)
		return 1;
	static_assert(sizeof(NxFluidCollisionBucketRecord) == 12,
		"phys_fn_003514 advances collision records by twelve bytes");
	static_assert(sizeof(NxFluidCollisionBucketDescriptor) == 12,
		"phys_fn_003514 reads its record count at descriptor +8");
	typedef unsigned (__cdecl* BucketPrefix)(NxFluidCollisionBucketDescriptor*, unsigned*);
	BucketPrefix oracle = reinterpret_cast<BucketPrefix>(oracleAddress);
	BucketPrefix candidate = reinterpret_cast<BucketPrefix>(candidateAddress);
	NxFluidCollisionBucketRecord oracleRecords[4] = {
		{ 0x10, 0xcccccccc, 3 }, { 0x20, 0xcccccccc, 0 },
		{ 0x30, 0xcccccccc, 2 }, { 0x40, 0xcccccccc, 4 } };
	NxFluidCollisionBucketRecord candidateRecords[4];
	memcpy(candidateRecords, oracleRecords, sizeof(oracleRecords));
	NxFluidCollisionBucketDescriptor oracleDescriptor = { oracleRecords, 0xa5a5a5a5, 4 };
	NxFluidCollisionBucketDescriptor candidateDescriptor = { candidateRecords, 0xa5a5a5a5, 4 };
	unsigned oracleTotal = 0xdddddddd, candidateTotal = 0xdddddddd;
	unsigned oracleBefore = 0, oracleAfter = 0, candidateBefore = 0, candidateAfter = 0;
	__asm mov oracleBefore, esp
	const unsigned oracleReturn = oracle(&oracleDescriptor, &oracleTotal);
	__asm mov oracleAfter, esp
	__asm mov candidateBefore, esp
	const unsigned candidateReturn = candidate(&candidateDescriptor, &candidateTotal);
	__asm mov candidateAfter, esp
	static const unsigned expectedPrefixes[4] = { 0, 3, 3, 5 };
	bool prefixesMatch = true;
	for(unsigned i = 0; i != 4; ++i)
		prefixesMatch = prefixesMatch && oracleRecords[i].prefix == expectedPrefixes[i]
			&& candidateRecords[i].prefix == expectedPrefixes[i]
			&& oracleRecords[i].kind == candidateRecords[i].kind
			&& oracleRecords[i].count == candidateRecords[i].count;
	const unsigned oracleNonemptyTotal = oracleTotal;
	const unsigned candidateNonemptyTotal = candidateTotal;
	const bool nonemptyMatches = prefixesMatch && oracleReturn == 4 && candidateReturn == 4
		&& oracleTotal == 9 && candidateTotal == 9
		&& oracleBefore == oracleAfter && candidateBefore == candidateAfter;

	NxFluidCollisionBucketRecord emptyOracleRecord = { 0x50, 0x12345678, 7 };
	NxFluidCollisionBucketRecord emptyCandidateRecord = emptyOracleRecord;
	NxFluidCollisionBucketDescriptor emptyOracle = { &emptyOracleRecord, 0x87654321, 0 };
	NxFluidCollisionBucketDescriptor emptyCandidate = { &emptyCandidateRecord, 0x87654321, 0 };
	oracleTotal = candidateTotal = 0xdddddddd;
	__asm mov oracleBefore, esp
	const unsigned emptyOracleReturn = oracle(&emptyOracle, &oracleTotal);
	__asm mov oracleAfter, esp
	__asm mov candidateBefore, esp
	const unsigned emptyCandidateReturn = candidate(&emptyCandidate, &candidateTotal);
	__asm mov candidateAfter, esp
	const bool emptyMatches = emptyOracleReturn == 0 && emptyCandidateReturn == 0
		&& oracleTotal == 0 && candidateTotal == 0
		&& memcmp(&emptyOracleRecord, &emptyCandidateRecord, sizeof(emptyOracleRecord)) == 0
		&& emptyOracleRecord.prefix == 0x12345678
		&& oracleBefore == oracleAfter && candidateBefore == candidateAfter;
	const unsigned mismatches = !nonemptyMatches + !emptyMatches;
	printf("fluid collision bucket-prefix records=4 oracle_total=%u candidate_total=%u "
		"prefixes=%u return=%u stack=%u/%u mismatches=%u\n",
		oracleNonemptyTotal, candidateNonemptyTotal, prefixesMatch,
		oracleReturn == candidateReturn, oracleBefore == oracleAfter,
		candidateBefore == candidateAfter, nonemptyMatches ? 0u : 1u);
	printf("fluid collision bucket-prefix empty total=%u return=%u preserved=%u "
		"stack=%u/%u mismatches=%u\n", candidateTotal,
		emptyOracleReturn == emptyCandidateReturn,
		memcmp(&emptyOracleRecord, &emptyCandidateRecord, sizeof(emptyOracleRecord)) == 0,
		oracleBefore == oracleAfter, candidateBefore == candidateAfter,
		emptyMatches ? 0u : 1u);
	return mismatches;
	}

struct NxFluidCollisionCellBucket
	{
	unsigned cellCount;
	unsigned firstCell;
	unsigned unknown08;
	};

struct NxFluidCollisionCellAttribute
	{
	unsigned words[4];
	};

struct NxFluidCollisionCellPrefixDescriptor
	{
	NxFluidCollisionCellBucket* buckets;
	unsigned unknown04;
	unsigned bucketCount;
	NxFluidCollisionCellAttribute* cells;
	unsigned unknown10;
	unsigned unknown14;
	unsigned unknown18;
	unsigned totalIndices;
	};

static unsigned nxRunFluidCollisionCellPrefixDifferential(void* oracleAddress,
	void* candidateAddress)
	{
	if(!oracleAddress || !candidateAddress)
		return 1;
	static_assert(sizeof(NxFluidCollisionCellBucket) == 12,
		"phys_fn_003516 advances bucket rows by twelve bytes");
	static_assert(sizeof(NxFluidCollisionCellAttribute) == 16,
		"phys_fn_003516 advances cell attributes by sixteen bytes");
	static_assert(sizeof(NxFluidCollisionCellPrefixDescriptor) == 0x20,
		"phys_fn_003516 reads descriptor fields through +0x1c");
	typedef unsigned (__cdecl* CellPrefix)(NxFluidCollisionCellPrefixDescriptor*);
	CellPrefix oracle = reinterpret_cast<CellPrefix>(oracleAddress);
	CellPrefix candidate = reinterpret_cast<CellPrefix>(candidateAddress);
	NxFluidCollisionCellBucket oracleBuckets[3] = {
		{ 2, 0, 0xaaaa0001 }, { 0, 2, 0xaaaa0002 }, { 1, 2, 0xaaaa0003 } };
	NxFluidCollisionCellBucket candidateBuckets[3];
	memcpy(candidateBuckets, oracleBuckets, sizeof(oracleBuckets));
	NxFluidCollisionCellAttribute oracleCells[3] = {
		{ { 0x10, 0x20, 3, 0xcccc0001 } },
		{ { 0x11, 0x21, 1, 0xcccc0002 } },
		{ { 0x12, 0x22, 2, 0xcccc0003 } } };
	NxFluidCollisionCellAttribute candidateCells[3];
	memcpy(candidateCells, oracleCells, sizeof(oracleCells));
	NxFluidCollisionCellPrefixDescriptor oracleDescriptor = {
		oracleBuckets, 0xdddd0001, 3, oracleCells, 0xdddd0002, 0xdddd0003,
		0xdddd0004, 0 };
	NxFluidCollisionCellPrefixDescriptor candidateDescriptor = oracleDescriptor;
	candidateDescriptor.buckets = candidateBuckets;
	candidateDescriptor.cells = candidateCells;
	unsigned oracleBefore = 0, oracleAfter = 0, candidateBefore = 0, candidateAfter = 0;
	__asm mov oracleBefore, esp
	const unsigned oracleReturn = oracle(&oracleDescriptor);
	__asm mov oracleAfter, esp
	__asm mov candidateBefore, esp
	const unsigned candidateReturn = candidate(&candidateDescriptor);
	__asm mov candidateAfter, esp
	static const unsigned expectedPrefixes[3] = { 0, 3, 4 };
	bool prefixesMatch = true;
	bool countsCleared = true;
	for(unsigned i = 0; i != 3; ++i)
		{
		countsCleared = countsCleared && oracleCells[i].words[2] == 0
			&& candidateCells[i].words[2] == 0;
		prefixesMatch = prefixesMatch
			&& oracleCells[i].words[3] == expectedPrefixes[i]
			&& candidateCells[i].words[3] == expectedPrefixes[i]
			&& oracleCells[i].words[0] == candidateCells[i].words[0]
			&& oracleCells[i].words[1] == candidateCells[i].words[1];
		}
	const bool cellsMatch = prefixesMatch && countsCleared;
	const bool bucketsMatch = memcmp(oracleBuckets, candidateBuckets,
		sizeof(oracleBuckets)) == 0;
	const unsigned oracleTotal = oracleDescriptor.totalIndices;
	const unsigned candidateTotal = candidateDescriptor.totalIndices;
	const bool nonemptyMatches = cellsMatch && bucketsMatch
		&& oracleTotal == 6 && candidateTotal == 6
		&& oracleReturn == 3 && candidateReturn == 3
		&& oracleBefore == oracleAfter && candidateBefore == candidateAfter
		&& oracleDescriptor.unknown04 == candidateDescriptor.unknown04
		&& oracleDescriptor.unknown10 == candidateDescriptor.unknown10
		&& oracleDescriptor.unknown14 == candidateDescriptor.unknown14
		&& oracleDescriptor.unknown18 == candidateDescriptor.unknown18;

	NxFluidCollisionCellBucket emptyOracleBucket = { 1, 0, 0xaaaa0010 };
	NxFluidCollisionCellBucket emptyCandidateBucket = emptyOracleBucket;
	NxFluidCollisionCellAttribute emptyOracleCell = { { 0x30, 0x40, 5, 0x12345678 } };
	NxFluidCollisionCellAttribute emptyCandidateCell = emptyOracleCell;
	NxFluidCollisionCellPrefixDescriptor emptyOracle = {
		&emptyOracleBucket, 0xdddd0010, 0, &emptyOracleCell, 0xdddd0011,
		0xdddd0012, 0xdddd0013, 0x87654321 };
	NxFluidCollisionCellPrefixDescriptor emptyCandidate = emptyOracle;
	emptyCandidate.buckets = &emptyCandidateBucket;
	emptyCandidate.cells = &emptyCandidateCell;
	__asm mov oracleBefore, esp
	const unsigned emptyOracleReturn = oracle(&emptyOracle);
	__asm mov oracleAfter, esp
	__asm mov candidateBefore, esp
	const unsigned emptyCandidateReturn = candidate(&emptyCandidate);
	__asm mov candidateAfter, esp
	const bool emptyPreserved = memcmp(&emptyOracleBucket, &emptyCandidateBucket,
		sizeof(emptyOracleBucket)) == 0
		&& memcmp(&emptyOracleCell, &emptyCandidateCell, sizeof(emptyOracleCell)) == 0;
	const bool emptyMatches = emptyPreserved && emptyOracleReturn == 0
		&& emptyCandidateReturn == 0
		&& emptyOracle.totalIndices == 0x87654321
		&& emptyCandidate.totalIndices == 0x87654321
		&& oracleBefore == oracleAfter && candidateBefore == candidateAfter;
	printf("fluid collision cell-prefix buckets=3 cells=3 oracle_total=%u candidate_total=%u "
		"prefixes=%u counts_cleared=%u return=%u stack=%u/%u mismatches=%u\n",
		oracleTotal, candidateTotal, prefixesMatch, countsCleared,
		oracleReturn == candidateReturn, oracleBefore == oracleAfter,
		candidateBefore == candidateAfter, nonemptyMatches ? 0u : 1u);
	printf("fluid collision cell-prefix empty preserved=%u return=%u total_preserved=%u "
		"stack=%u/%u mismatches=%u\n", emptyPreserved,
		emptyOracleReturn == emptyCandidateReturn,
		emptyOracle.totalIndices == emptyCandidate.totalIndices,
		oracleBefore == oracleAfter, candidateBefore == candidateAfter,
		emptyMatches ? 0u : 1u);
	return !nonemptyMatches + !emptyMatches;
	}

struct NxFluidCollisionBucketHashDescriptor
	{
	void* buckets;
	unsigned bucketCount;
	};

struct NxFluidCollisionInt16Vector3Array
	{
	short* data;
	int count;
	unsigned capacity;
	};

static unsigned nxRunFluidCollisionBucketHashDifferential(const unsigned char* oracleBase,
	void* candidateAddress)
	{
	if(!oracleBase || !candidateAddress)
		return 1;
	typedef int (__cdecl* CountBuckets)(NxFluidCollisionBucketHashDescriptor*,
		const NxFluidCollisionInt16Vector3Array*);
	CountBuckets oracle = reinterpret_cast<CountBuckets>(
		const_cast<unsigned char*>(oracleBase) + 0x86ad0);
	CountBuckets candidate = reinterpret_cast<CountBuckets>(candidateAddress);
	unsigned oracleBuckets[5][3] = {
		{ 0xaaaa0000, 0xbbbb0000, 11 }, { 0xaaaa0001, 0xbbbb0001, 12 },
		{ 0xaaaa0002, 0xbbbb0002, 13 }, { 0xaaaa0003, 0xbbbb0003, 14 },
		{ 0xaaaa0004, 0xbbbb0004, 15 } };
	unsigned candidateBuckets[5][3];
	memcpy(candidateBuckets, oracleBuckets, sizeof(oracleBuckets));
	short cells[7][3] = {
		{ 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 },
		{ -1, 0, 0 }, { 0, -1, 0 }, { 0, 0, -1 } };
	const NxFluidCollisionInt16Vector3Array oracleCells = { &cells[0][0], 7, 7 };
	const NxFluidCollisionInt16Vector3Array candidateCells = oracleCells;
	NxFluidCollisionBucketHashDescriptor oracleDescriptor = { oracleBuckets, 5 };
	NxFluidCollisionBucketHashDescriptor candidateDescriptor = { candidateBuckets, 5 };
	unsigned oracleBefore = 0, oracleAfter = 0, candidateBefore = 0, candidateAfter = 0;
	__asm mov oracleBefore, esp
	const int oracleReturn = oracle(&oracleDescriptor, &oracleCells);
	__asm mov oracleAfter, esp
	__asm mov candidateBefore, esp
	const int candidateReturn = candidate(&candidateDescriptor, &candidateCells);
	__asm mov candidateAfter, esp
	const unsigned expectedCounts[5] = { 3, 2, 1, 0, 1 };
	bool countsMatch = true;
	bool otherWordsPreserved = true;
	for(unsigned bucket = 0; bucket != 5; ++bucket)
		{
		countsMatch = countsMatch
			&& oracleBuckets[bucket][2] == 11 + bucket + expectedCounts[bucket]
			&& candidateBuckets[bucket][2] == 11 + bucket + expectedCounts[bucket];
		otherWordsPreserved = otherWordsPreserved
			&& oracleBuckets[bucket][0] == candidateBuckets[bucket][0]
			&& oracleBuckets[bucket][1] == candidateBuckets[bucket][1]
			&& oracleBuckets[bucket][0] == 0xaaaa0000 + bucket
			&& oracleBuckets[bucket][1] == 0xbbbb0000 + bucket;
		}
	const bool matched = countsMatch && otherWordsPreserved
		&& oracleReturn == 7 && candidateReturn == 7
		&& oracleBefore == oracleAfter && candidateBefore == candidateAfter;
	printf("fluid collision hash-count buckets=5 cells=7 counts=%u words_preserved=%u "
		"return=%u stack=%u/%u mismatches=%u\n", countsMatch,
		otherWordsPreserved, oracleReturn == candidateReturn,
		oracleBefore == oracleAfter, candidateBefore == candidateAfter,
		matched ? 0u : 1u);
	unsigned emptyOracleBuckets[2][3] = {
		{ 0x11111111, 0x22222222, 17 }, { 0x33333333, 0x44444444, 19 } };
	unsigned emptyCandidateBuckets[2][3];
	memcpy(emptyCandidateBuckets, emptyOracleBuckets, sizeof(emptyOracleBuckets));
	const NxFluidCollisionInt16Vector3Array emptyCells = { 0, 0, 0 };
	NxFluidCollisionBucketHashDescriptor emptyOracle = { emptyOracleBuckets, 2 };
	NxFluidCollisionBucketHashDescriptor emptyCandidate = { emptyCandidateBuckets, 2 };
	__asm mov oracleBefore, esp
	const int emptyOracleReturn = oracle(&emptyOracle, &emptyCells);
	__asm mov oracleAfter, esp
	__asm mov candidateBefore, esp
	const int emptyCandidateReturn = candidate(&emptyCandidate, &emptyCells);
	__asm mov candidateAfter, esp
	const bool emptyPreserved = memcmp(emptyOracleBuckets, emptyCandidateBuckets,
		sizeof(emptyOracleBuckets)) == 0
		&& emptyOracleBuckets[0][2] == 17 && emptyOracleBuckets[1][2] == 19;
	const bool emptyMatched = emptyPreserved && emptyOracleReturn == 0
		&& emptyCandidateReturn == 0 && oracleBefore == oracleAfter
		&& candidateBefore == candidateAfter;
	printf("fluid collision hash-count empty preserved=%u return=%u stack=%u/%u "
		"mismatches=%u\n", emptyPreserved,
		emptyOracleReturn == emptyCandidateReturn,
		oracleBefore == oracleAfter, candidateBefore == candidateAfter,
		emptyMatched ? 0u : 1u);
	return !matched + !emptyMatched;
	}

static unsigned gFluidCollisionTriangleException = 0;
static void* gFluidCollisionTriangleExceptionAddress = 0;
static void* gFluidCollisionTriangleFaultAddress = 0;
static unsigned gFluidCollisionTriangleExceptionParameters = 0;

static short nxCallOracleFluidCollisionTriangleCells(void* target, void* array,
	const float* first, unsigned __int64 secondAndThird,
	float margin, float cellSize)
	{
	typedef short (__cdecl* TriangleCells)(
		void*, const float*, unsigned __int64, float, float);
	__try
		{
		return reinterpret_cast<TriangleCells>(target)(array, first,
			secondAndThird, margin, cellSize);
		}
	__except((gFluidCollisionTriangleException = GetExceptionCode(),
		gFluidCollisionTriangleExceptionAddress =
			GetExceptionInformation()->ExceptionRecord->ExceptionAddress,
		gFluidCollisionTriangleExceptionParameters =
			GetExceptionInformation()->ExceptionRecord->NumberParameters,
		gFluidCollisionTriangleFaultAddress =
			GetExceptionInformation()->ExceptionRecord->NumberParameters > 1
			? reinterpret_cast<void*>(GetExceptionInformation()->ExceptionRecord->ExceptionInformation[1])
			: 0,
		EXCEPTION_EXECUTE_HANDLER))
		{
		return static_cast<short>(-32767);
		}
	}

static unsigned nxRunFluidCollisionTriangleCellsDifferential(
	const unsigned char* oracleBase, void* candidateAddress)
	{
	if(!oracleBase)
		return 1;
	typedef short (__cdecl* TriangleCells)(
		NxFluidCollisionInt16Vector3Array*, const float*,
		unsigned __int64, float, float);
	TriangleCells oracle = reinterpret_cast<TriangleCells>(
		const_cast<unsigned char*>(oracleBase) + 0x866d0);
	TriangleCells candidate = reinterpret_cast<TriangleCells>(candidateAddress);
	const float triangles[3][9] = {
		{ 0.25f, 0.25f, 0.25f, 1.25f, 0.25f, 0.25f,
		  0.25f, 1.25f, 0.25f },
		{ -1.2f, -0.3f, 0.4f, 0.7f, -0.2f, 0.4f,
		  -0.1f, 1.1f, 0.4f },
		{ 2.2f, 2.2f, 2.2f, 2.2f, 2.2f, 2.2f,
		  2.2f, 2.2f, 2.2f }
	};
	static const unsigned expectedCounts[] = { 3, 10, 1 };
	static const unsigned initialCapacities[] = { 512, 512, 0 };
	unsigned mismatches = 0;
	unsigned oracleDigest = 2166136261u;
	unsigned candidateDigest = 2166136261u;
	unsigned totalCells = 0;
	for(unsigned test = 0; test != 3; ++test)
		{
		const unsigned initialCapacity = initialCapacities[test];
		short* oracleStorage = initialCapacity
			? static_cast<short*>(malloc(initialCapacity * 3 * sizeof(short))) : 0;
		short* candidateStorage = initialCapacity
			? static_cast<short*>(malloc(initialCapacity * 3 * sizeof(short))) : 0;
		if(initialCapacity && (!oracleStorage || !candidateStorage))
			{
			free(oracleStorage);
			free(candidateStorage);
			++mismatches;
			continue;
			}
		if(initialCapacity)
			{
			memset(oracleStorage, 0x5a, initialCapacity * 3 * sizeof(short));
			memcpy(candidateStorage, oracleStorage,
				initialCapacity * 3 * sizeof(short));
			}
		NxFluidCollisionInt16Vector3Array oracleArray = {
			oracleStorage, 0, initialCapacity };
		NxFluidCollisionInt16Vector3Array candidateArray = {
			candidateStorage, 0, initialCapacity };
		const float* const tri = triangles[test];
		const unsigned __int64 secondAndThird =
			(static_cast<unsigned __int64>(reinterpret_cast<uintptr_t>(tri + 6)) << 32)
			| reinterpret_cast<uintptr_t>(tri + 3);
		const float margins[] = { 0.0f, 0.125f, 0.0f };
		const float cellSizes[] = { 1.0f, 0.75f, 1.0f };
		const float margin = margins[test];
		const float cellSize = cellSizes[test];
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		gFluidCollisionTriangleException = 0;
		gFluidCollisionTriangleExceptionAddress = 0;
		gFluidCollisionTriangleFaultAddress = 0;
		gFluidCollisionTriangleExceptionParameters = 0;
		const short oracleReturn = nxCallOracleFluidCollisionTriangleCells(
			reinterpret_cast<void*>(oracle), &oracleArray, tri,
			secondAndThird, margin, cellSize);
		__asm mov oracleAfter, esp
		if(gFluidCollisionTriangleException)
			printf("fluid collision triangle-cells oracle-exception case=%u code=%08x at=%p params=%u fault=%p\n",
				test, gFluidCollisionTriangleException,
				gFluidCollisionTriangleExceptionAddress,
				gFluidCollisionTriangleExceptionParameters,
				gFluidCollisionTriangleFaultAddress);
		if(gFluidCollisionTriangleException || oracleArray.count < 0
			|| static_cast<unsigned>(oracleArray.count) > oracleArray.capacity)
			{
			free(oracleArray.data);
			free(candidateArray.data);
			++mismatches;
			continue;
			}
		__asm mov candidateBefore, esp
		const short candidateReturn = candidate
			? candidate(&candidateArray, tri, secondAndThird, margin, cellSize)
			: static_cast<short>(-32768);
		__asm mov candidateAfter, esp
		if(candidate && (candidateArray.count < 0
			|| static_cast<unsigned>(candidateArray.count) > candidateArray.capacity))
			{
			free(oracleArray.data);
			free(candidateArray.data);
			++mismatches;
			continue;
			}
		const unsigned commonCells = oracleArray.count < candidateArray.count
			? oracleArray.count : candidateArray.count;
		const unsigned oracleCount = static_cast<unsigned>(oracleArray.count);
		const unsigned candidateCount = static_cast<unsigned>(candidateArray.count);
		for(unsigned i = 0; i < oracleCount * 3; ++i)
			oracleDigest = (oracleDigest ^ static_cast<unsigned short>(oracleArray.data[i]))
				* 16777619u;
		for(unsigned i = 0; i < candidateCount * 3; ++i)
			candidateDigest = (candidateDigest ^ static_cast<unsigned short>(candidateArray.data[i]))
				* 16777619u;
		const bool cellsMatch = candidate && oracleArray.count == candidateArray.count
			&& memcmp(oracleArray.data, candidateArray.data,
				commonCells * 3 * sizeof(short)) == 0;
		const bool capacityMatched = oracleArray.capacity == candidateArray.capacity;
		bool tailsPreserved = test == 2 || (oracleCount <= initialCapacity
			&& (!candidate || candidateCount <= initialCapacity));
		if(tailsPreserved)
			{
			for(unsigned i = oracleCount * 3; i < initialCapacity * 3; ++i)
				tailsPreserved = tailsPreserved && oracleArray.data[i] == 0x5a5a;
			if(candidate)
				for(unsigned i = candidateCount * 3; i < initialCapacity * 3; ++i)
					tailsPreserved = tailsPreserved && candidateArray.data[i] == 0x5a5a;
			}
		const bool stackBalanced = oracleBefore == oracleAfter
			&& (!candidate || candidateBefore == candidateAfter);
		const bool matched = oracleArray.count >= 0 && oracleCount == expectedCounts[test]
			&& (!candidate || (candidateArray.count >= 0 && candidateCount == expectedCounts[test]))
			&& capacityMatched && cellsMatch && tailsPreserved && stackBalanced
			&& candidate && oracleReturn == candidateReturn;
		printf("fluid collision triangle-cells case=%u cells=%u/%u capacity=%u/%u "
			"return=%d/%d cells_match=%u tails=%u stack=%u/%u mismatches=%u\n", test,
			oracleCount, candidateCount, oracleArray.capacity, candidateArray.capacity,
			oracleReturn, candidateReturn,
			cellsMatch, tailsPreserved, oracleBefore == oracleAfter,
			!candidate || candidateBefore == candidateAfter,
			matched ? 0u : 1u);
		if(!matched)
			++mismatches;
		totalCells += oracleCount;
		free(oracleArray.data);
		free(candidateArray.data);
		}
	printf("fluid collision triangle-cells cases=3 oracle_digest=%08x "
		"candidate_digest=%08x oracle_cells=%u candidate_present=%u mismatches=%u\n",
		oracleDigest, candidateDigest, totalCells, candidate != 0, mismatches);
	return mismatches;
	}

struct NxFluidCollisionTriangleMesh
	{
	unsigned triangleCount;
	const unsigned* indices;
	unsigned reserved;
	const float* vertices;
	float transform[12];
	};

static unsigned nxRunFluidCollisionTriangleBuilderDifferential(
	const unsigned char* oracleBase, void* candidateAddress)
	{
	if(!oracleBase || !candidateAddress)
		return 1;
	typedef void (__cdecl* BuildTriangleCells)(
		NxFluidCollisionBucketHashDescriptor*, const NxFluidCollisionTriangleMesh*,
		float, float, NxFluidCollisionInt16Vector3Array*);
	BuildTriangleCells oracle = reinterpret_cast<BuildTriangleCells>(
		const_cast<unsigned char*>(oracleBase) + 0x86b40);
	BuildTriangleCells candidate = reinterpret_cast<BuildTriangleCells>(candidateAddress);
	const unsigned indices[6] = { 0, 1, 2, 3, 4, 5 };
	const float vertices[18] = {
		0.125f, 0.25f, 0.375f, 1.125f, 0.25f, 0.375f,
		0.125f, 1.25f, 0.375f,
		-0.75f, 0.5f, 0.25f, 0.25f, 0.5f, 0.25f,
		-0.75f, 1.25f, 0.25f
		};
	const float transforms[2][12] = {
		{ 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
		  0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f },
		{ 0.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
		  0.0f, 0.0f, 1.0f, -0.25f, 1.5f, -0.125f }
		};
	unsigned mismatches = 0;
	unsigned digest = 2166136261u;
	unsigned totalCells = 0;
	for(unsigned test = 0; test != 3; ++test)
		{
		NxFluidCollisionTriangleMesh mesh = {};
		mesh.triangleCount = test == 2 ? 0 : 2;
		mesh.indices = indices;
		mesh.vertices = vertices;
		memcpy(mesh.transform, transforms[test], sizeof(mesh.transform));
		short oracleCellsStorage[512 * 3];
		short candidateCellsStorage[512 * 3];
		memset(oracleCellsStorage, 0x5a, sizeof(oracleCellsStorage));
		memcpy(candidateCellsStorage, oracleCellsStorage, sizeof(oracleCellsStorage));
		NxFluidCollisionInt16Vector3Array oracleCells = {
			oracleCellsStorage, 0, 512 };
		NxFluidCollisionInt16Vector3Array candidateCells = {
			candidateCellsStorage, 0, 512 };
		unsigned oracleBuckets[7][3];
		unsigned candidateBuckets[7][3];
		for(unsigned bucket = 0; bucket != 7; ++bucket)
			{
			oracleBuckets[bucket][0] = 0xabcdef00u + bucket;
			oracleBuckets[bucket][1] = 0x12345600u + bucket;
			oracleBuckets[bucket][2] = 10 + bucket;
			}
		memcpy(candidateBuckets, oracleBuckets, sizeof(oracleBuckets));
		NxFluidCollisionBucketHashDescriptor oracleHash = {
			&oracleBuckets[0][0], 7 };
		NxFluidCollisionBucketHashDescriptor candidateHash = {
			&candidateBuckets[0][0], 7 };
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		oracle(&oracleHash, &mesh, 0.0f, 1.0f, &oracleCells);
		__asm mov oracleAfter, esp
		__asm mov candidateBefore, esp
		candidate(&candidateHash, &mesh, 0.0f, 1.0f, &candidateCells);
		__asm mov candidateAfter, esp
		bool bucketsMatch = memcmp(oracleBuckets, candidateBuckets,
			sizeof(oracleBuckets)) == 0;
		bool tailsMatch = true;
		for(unsigned i = oracleCells.count * 3; i != 512 * 3; ++i)
			tailsMatch = tailsMatch && oracleCellsStorage[i] == 0x5a5a
				&& candidateCellsStorage[i] == 0x5a5a;
		const bool cellsMatch = oracleCells.count == candidateCells.count
			&& memcmp(oracleCells.data, candidateCells.data,
				static_cast<size_t>(oracleCells.count) * 3 * sizeof(short)) == 0;
		const bool stackBalanced = oracleBefore == oracleAfter
			&& candidateBefore == candidateAfter;
		const bool matched = oracleCells.count >= 0 && oracleCells.count <= 512
			&& candidateCells.count >= 0 && candidateCells.count <= 512
			&& cellsMatch && bucketsMatch && tailsMatch && stackBalanced;
		for(int i = 0; i < oracleCells.count * 3; ++i)
			digest = (digest ^ static_cast<unsigned short>(oracleCells.data[i]))
				* 16777619u;
		totalCells += static_cast<unsigned>(oracleCells.count);
		printf("fluid collision triangle-builder case=%u cells=%d/%d buckets=%u "
			"tails=%u stack=%u/%u mismatches=%u\n", test,
			oracleCells.count, candidateCells.count, bucketsMatch, tailsMatch,
			oracleBefore == oracleAfter, candidateBefore == candidateAfter,
			matched ? 0u : 1u);
		if(!matched)
			++mismatches;
		}
	printf("fluid collision triangle-builder cases=3 oracle_digest=%08x "
		"candidate_present=1 oracle_cells=%u mismatches=%u\n",
		digest, totalCells, mismatches);
	return mismatches;
	}

struct NxFluidCollisionGridCell
	{
	unsigned xy;
	short z;
	short opaqueZ;
	int sampleCount;
	int next;
	};
static_assert(sizeof(NxFluidCollisionGridCell) == 16,
	"phys_fn_003530 appends 16-byte hashed cell records");

static unsigned nxRunFluidCollisionCellHashDifferential(
	const unsigned char* oracleBase, void* candidateAddress)
	{
	if(!oracleBase || !candidateAddress)
		return 1;
	typedef int (__cdecl* AccumulateCells)(int*, NxFluidCollisionBucketHashDescriptor*,
		const NxFluidCollisionInt16Vector3Array*);
	AccumulateCells oracle = reinterpret_cast<AccumulateCells>(
		const_cast<unsigned char*>(oracleBase) + 0x86e00);
	AccumulateCells candidate = reinterpret_cast<AccumulateCells>(candidateAddress);
	const short inputCells[7][3] = {
		{ 0, 0, 0 }, { 1, 0, 0 }, { 0, 0, 0 }, { 3, 0, 0 },
		{ 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, -1 }
		};
	unsigned mismatches = 0;
	for(unsigned test = 0; test != 3; ++test)
		{
		NxFluidCollisionGridCell oracleRecords[9];
		NxFluidCollisionGridCell candidateRecords[9];
		memset(oracleRecords, 0x5a, sizeof(oracleRecords));
		memcpy(candidateRecords, oracleRecords, sizeof(oracleRecords));
		unsigned oracleBuckets[3][3] = {
			{ test == 1 ? 1u : 0u, 0u, 0xabcdef00u },
			{ 0u, 3u, 0xabcdef01u }, { 0u, 6u, 0xabcdef02u }
			};
		unsigned candidateBuckets[3][3];
		memcpy(candidateBuckets, oracleBuckets, sizeof(oracleBuckets));
		if(test == 1)
			{
			oracleRecords[0].xy = 0;
			oracleRecords[0].z = 0;
			oracleRecords[0].sampleCount = 9;
			oracleRecords[0].next = -1;
			memcpy(candidateRecords, oracleRecords, sizeof(oracleRecords));
			}
		int oracleRecordBase = reinterpret_cast<int>(oracleRecords);
		int candidateRecordBase = reinterpret_cast<int>(candidateRecords);
		NxFluidCollisionBucketHashDescriptor oracleHash = {
			&oracleBuckets[0][0], 3 };
		NxFluidCollisionBucketHashDescriptor candidateHash = {
			&candidateBuckets[0][0], 3 };
		const NxFluidCollisionInt16Vector3Array cells = {
			const_cast<short*>(&inputCells[0][0]), test == 2 ? 0 : 7, 7 };
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		const int oracleReturn = oracle(&oracleRecordBase, &oracleHash, &cells);
		__asm mov oracleAfter, esp
		__asm mov candidateBefore, esp
		const int candidateReturn = candidate(&candidateRecordBase,
			&candidateHash, &cells);
		__asm mov candidateAfter, esp
		bool expected = true;
		if(test != 2)
			{
			const unsigned expectedSamples[5] = {
				test == 1 ? 11u : 2u, 1u, 2u, 1u, 1u };
			const unsigned recordIndices[5] = { 0u, 1u, 3u, 6u, 7u };
			const unsigned expectedBucketCounts[3] = { 2u, 1u, 2u };
			const unsigned expectedFirsts[3] = { 0u, 3u, 6u };
			for(unsigned bucket = 0; bucket != 3; ++bucket)
				expected = expected && oracleBuckets[bucket][0] == expectedBucketCounts[bucket]
					&& oracleBuckets[bucket][1] == expectedFirsts[bucket];
			for(unsigned cell = 0; cell != 5; ++cell)
				expected = expected && static_cast<unsigned>(oracleRecords[recordIndices[cell]].sampleCount)
					== expectedSamples[cell];
			}
		const bool recordsMatch = memcmp(oracleRecords, candidateRecords,
			sizeof(oracleRecords)) == 0;
		const bool bucketsMatch = memcmp(oracleBuckets, candidateBuckets,
			sizeof(oracleBuckets)) == 0;
		const bool stackBalanced = oracleBefore == oracleAfter
			&& candidateBefore == candidateAfter;
		const bool matched = expected && recordsMatch && bucketsMatch
			&& oracleReturn == 1 && candidateReturn == 1 && stackBalanced;
		printf("fluid collision cell-hash case=%u records=%u buckets=%u expected=%u "
			"stack=%u/%u mismatches=%u\n", test, recordsMatch, bucketsMatch,
			expected, oracleBefore == oracleAfter, candidateBefore == candidateAfter,
			matched ? 0u : 1u);
		if(!matched)
			{
			++mismatches;
			for(unsigned bucket = 0; bucket != 3; ++bucket)
				printf("fluid collision cell-hash debug bucket=%u oracle=%u/%u candidate=%u/%u\n",
					bucket, oracleBuckets[bucket][0], oracleBuckets[bucket][1],
					candidateBuckets[bucket][0], candidateBuckets[bucket][1]);
			for(unsigned cell = 0; cell != 9; ++cell)
				printf("fluid collision cell-hash debug cell=%u oracle=%08x/%08x/%08x/%08x "
					"candidate=%08x/%08x/%08x/%08x\n", cell,
					oracleRecords[cell].xy,
					*reinterpret_cast<unsigned*>(
						reinterpret_cast<unsigned char*>(&oracleRecords[cell]) + 4),
					static_cast<unsigned>(oracleRecords[cell].sampleCount),
					static_cast<unsigned>(oracleRecords[cell].next),
					candidateRecords[cell].xy,
					*reinterpret_cast<unsigned*>(
						reinterpret_cast<unsigned char*>(&candidateRecords[cell]) + 4),
					static_cast<unsigned>(candidateRecords[cell].sampleCount),
					static_cast<unsigned>(candidateRecords[cell].next));
			}
		}
	printf("fluid collision cell-hash cases=3 inputs=7 unique=5 "
		"bucket_counts=2,1,2 candidate_present=1 mismatches=%u\n", mismatches);
	return mismatches;
	}

struct NxFluidCollisionIndexDescriptor
	{
	unsigned* buckets;
	unsigned bucketCount;
	unsigned reserved08;
	NxFluidCollisionGridCell* cells;
	unsigned reserved10;
	unsigned reserved14;
	unsigned* indices;
	};
static_assert(sizeof(NxFluidCollisionIndexDescriptor) == 28,
	"phys_fn_003535 reads the bucket descriptor through its index array");
static_assert(offsetof(NxFluidCollisionIndexDescriptor, cells) == 12,
	"phys_fn_003535 reads cell records at descriptor+0x0c");
static_assert(offsetof(NxFluidCollisionIndexDescriptor, indices) == 24,
	"phys_fn_003535 reads association storage at descriptor+0x18");

static unsigned nxRunFluidCollisionCellIndexDifferential(
	const unsigned char* oracleBase, void* candidateAddress)
	{
	if(!oracleBase || !candidateAddress)
		return 1;
	typedef int (__cdecl* AppendCellIndex)(NxFluidCollisionIndexDescriptor*,
		unsigned, unsigned, const NxFluidCollisionInt16Vector3Array*);
	AppendCellIndex oracle = reinterpret_cast<AppendCellIndex>(
		const_cast<unsigned char*>(oracleBase) + 0x871d0);
	AppendCellIndex candidate = reinterpret_cast<AppendCellIndex>(candidateAddress);
	const short inputCells[3][3] = {
		{ 0, 0, 0 }, { 1, 0, 0 }, { 0, 1, 0 }
		};
	unsigned mismatches = 0;
	for(unsigned test = 0; test != 3; ++test)
		{
		const unsigned bucketCount = test == 0 ? 5 : 7;
		unsigned oracleBuckets[7][3] = {};
		unsigned candidateBuckets[7][3] = {};
		NxFluidCollisionGridCell oracleCells[4];
		NxFluidCollisionGridCell candidateCells[4];
		memset(oracleCells, 0x5a, sizeof(oracleCells));
		memcpy(candidateCells, oracleCells, sizeof(oracleCells));
		unsigned oracleIndices[24];
		unsigned candidateIndices[24];
		for(unsigned i = 0; i != 24; ++i)
			oracleIndices[i] = candidateIndices[i] = 0xdeadbeefu;
		if(test == 0)
			{
			oracleBuckets[0][0] = 1;
			oracleBuckets[0][1] = 0;
			oracleCells[0].xy = 0;
			oracleCells[0].z = 0;
			oracleCells[0].sampleCount = 1;
			oracleCells[0].next = 2;
			memcpy(candidateBuckets, oracleBuckets, sizeof(oracleBuckets));
			memcpy(candidateCells, oracleCells, sizeof(oracleCells));
			}
		else if(test == 1)
			{
			oracleBuckets[1][0] = 1;
			oracleBuckets[1][1] = 0;
			oracleBuckets[3][0] = 1;
			oracleBuckets[3][1] = 1;
			oracleCells[0].xy = 1;
			oracleCells[0].z = 0;
			oracleCells[0].sampleCount = 2;
			oracleCells[0].next = 0;
			oracleCells[1].xy = 0x00010000u;
			oracleCells[1].z = 0;
			oracleCells[1].sampleCount = 3;
			oracleCells[1].next = 1;
			memcpy(candidateBuckets, oracleBuckets, sizeof(oracleBuckets));
			memcpy(candidateCells, oracleCells, sizeof(oracleCells));
			}
		NxFluidCollisionIndexDescriptor oracleDescriptor = {
			&oracleBuckets[0][0], bucketCount, 0x10203040u,
			oracleCells, 0x50607080u, 0x90a0b0c0u, oracleIndices };
		NxFluidCollisionIndexDescriptor candidateDescriptor = {
			&candidateBuckets[0][0], bucketCount, 0x10203040u,
			candidateCells, 0x50607080u, 0x90a0b0c0u, candidateIndices };
		const NxFluidCollisionInt16Vector3Array cells = {
			const_cast<short*>(&inputCells[0][0]), test == 0 ? 1 : (test == 1 ? 3 : 0), 3 };
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		const int oracleReturn = oracle(&oracleDescriptor, 4, 7, &cells);
		__asm mov oracleAfter, esp
		__asm mov candidateBefore, esp
		const int candidateReturn = candidate(&candidateDescriptor, 4, 7, &cells);
		__asm mov candidateAfter, esp
		const bool matched = memcmp(oracleBuckets, candidateBuckets,
			sizeof(oracleBuckets)) == 0
			&& memcmp(oracleCells, candidateCells, sizeof(oracleCells)) == 0
			&& memcmp(oracleIndices, candidateIndices, sizeof(oracleIndices)) == 0
			&& oracleReturn == 1 && candidateReturn == 1
			&& oracleBefore == oracleAfter && candidateBefore == candidateAfter;
		printf("fluid collision cell-index case=%u count=%d buckets=%u cells=%u "
			"indices=%u return=%d/%d stack=%u/%u mismatches=%u\n",
			test, cells.count,
			memcmp(oracleBuckets, candidateBuckets, sizeof(oracleBuckets)) == 0,
			memcmp(oracleCells, candidateCells, sizeof(oracleCells)) == 0,
			memcmp(oracleIndices, candidateIndices, sizeof(oracleIndices)) == 0,
			oracleReturn, candidateReturn, oracleBefore == oracleAfter,
			candidateBefore == candidateAfter, matched ? 0u : 1u);
		if(!matched)
			++mismatches;
		}
	printf("fluid collision cell-index cases=3 matched=1 missing=1 empty=1 "
		"candidate_present=1 mismatches=%u\n", mismatches);
	return mismatches;
	}

static unsigned nxRunFluidCollisionTriangleIndexBuilderDifferential(
	const unsigned char* oracleBase, void* candidateAddress)
	{
	if(!oracleBase || !candidateAddress)
		return 1;
	typedef int (__cdecl* BuildTriangleIndices)(NxFluidCollisionIndexDescriptor*,
		const NxFluidCollisionTriangleMesh*, unsigned, float, float,
		NxFluidCollisionInt16Vector3Array*);
	BuildTriangleIndices oracle = reinterpret_cast<BuildTriangleIndices>(
		const_cast<unsigned char*>(oracleBase) + 0x872a0);
	BuildTriangleIndices candidate = reinterpret_cast<BuildTriangleIndices>(candidateAddress);
	const unsigned indices[6] = { 0, 1, 2, 3, 4, 5 };
	const float vertices[18] = {
		0.125f, 0.25f, 0.375f, 1.125f, 0.25f, 0.375f,
		0.125f, 1.25f, 0.375f,
		-0.75f, 0.5f, 0.25f, 0.25f, 0.5f, 0.25f,
		-0.75f, 1.25f, 0.25f
		};
	const float transform[12] = {
		0.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f, -0.25f, 1.5f, -0.125f };
	const unsigned bucketCount = 31;
	const unsigned recordCount = 9 * 9 * 9;
	const unsigned indexCount = 8192;
	unsigned mismatches = 0;
	for(unsigned test = 0; test != 3; ++test)
		{
		NxFluidCollisionTriangleMesh mesh = {};
		mesh.triangleCount = test == 0 ? 2 : 0;
		mesh.indices = indices;
		mesh.vertices = vertices;
		memcpy(mesh.transform, transform, sizeof(transform));
		unsigned oracleBuckets[31][3] = {};
		unsigned candidateBuckets[31][3] = {};
		NxFluidCollisionGridCell oracleRecords[729];
		NxFluidCollisionGridCell candidateRecords[729];
		unsigned oracleIndices[8192];
		unsigned candidateIndices[8192];
		for(unsigned i = 0; i != indexCount; ++i)
			oracleIndices[i] = candidateIndices[i] = 0xdeadbeefu;
		unsigned record = 0;
		for(unsigned bucket = 0; bucket != bucketCount; ++bucket)
			{
			oracleBuckets[bucket][1] = record;
			oracleBuckets[bucket][2] = 0xabcdef00u + bucket;
			for(int x = -4; x <= 4; ++x)
				for(int y = -4; y <= 4; ++y)
					for(int z = -4; z <= 4; ++z)
						{
						const unsigned cellBucket = static_cast<unsigned>(x + 101 * y + 7919 * z)
							% bucketCount;
						if(cellBucket != bucket)
							continue;
						NxFluidCollisionGridCell& cell = oracleRecords[record++];
						cell.xy = static_cast<unsigned short>(x)
							| (static_cast<unsigned>(static_cast<unsigned short>(y)) << 16);
						cell.z = static_cast<short>(z);
						cell.opaqueZ = static_cast<short>(0x5a00u + bucket);
						cell.sampleCount = 0;
						cell.next = 0;
						++oracleBuckets[bucket][0];
						}
			}
		if(record != recordCount)
			return mismatches + 1;
		memcpy(candidateBuckets, oracleBuckets, sizeof(oracleBuckets));
		memcpy(candidateRecords, oracleRecords, sizeof(oracleRecords));
		NxFluidCollisionIndexDescriptor oracleDescriptor = {
			&oracleBuckets[0][0], bucketCount, 0x10203040u, oracleRecords,
			0x50607080u, 0x90a0b0c0u, oracleIndices };
		NxFluidCollisionIndexDescriptor candidateDescriptor = {
			&candidateBuckets[0][0], bucketCount, 0x10203040u, candidateRecords,
			0x50607080u, 0x90a0b0c0u, candidateIndices };
		short oracleCellStorage[512 * 3];
		short candidateCellStorage[512 * 3];
		memset(oracleCellStorage, 0x5a, sizeof(oracleCellStorage));
		memcpy(candidateCellStorage, oracleCellStorage, sizeof(oracleCellStorage));
		NxFluidCollisionInt16Vector3Array oracleCells = {
			oracleCellStorage, test == 1 ? 5 : 0, 512 };
		NxFluidCollisionInt16Vector3Array candidateCells = {
			candidateCellStorage, test == 1 ? 5 : 0, 512 };
		NxFluidCollisionInt16Vector3Array* oracleOutput = test == 2 ? 0 : &oracleCells;
		NxFluidCollisionInt16Vector3Array* candidateOutput = test == 2 ? 0 : &candidateCells;
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		const int oracleReturn = oracle(&oracleDescriptor, &mesh, 4, 0.0f, 1.0f,
			oracleOutput);
		__asm mov oracleAfter, esp
		__asm mov candidateBefore, esp
		const int candidateReturn = candidate(&candidateDescriptor, &mesh, 4,
			0.0f, 1.0f, candidateOutput);
		__asm mov candidateAfter, esp
		const bool bucketsMatch = memcmp(oracleBuckets, candidateBuckets,
			sizeof(oracleBuckets)) == 0;
		const bool recordsMatch = memcmp(oracleRecords, candidateRecords,
			sizeof(oracleRecords)) == 0;
		const bool indicesMatch = memcmp(oracleIndices, candidateIndices,
			sizeof(oracleIndices)) == 0;
		const bool cellsMatch = oracleCells.count == candidateCells.count
			&& oracleCells.capacity == candidateCells.capacity
			&& memcmp(oracleCellStorage, candidateCellStorage,
				sizeof(oracleCellStorage)) == 0;
		const bool matched = bucketsMatch && recordsMatch && indicesMatch
			&& cellsMatch && oracleReturn == (test == 2 ? 0 : 1)
			&& candidateReturn == (test == 2 ? 0 : 1)
			&& oracleBefore == oracleAfter && candidateBefore == candidateAfter;
		printf("fluid collision triangle-index-builder case=%u cells=%d/%d "
			"buckets=%u records=%u indices=%u return=%d/%d stack=%u/%u "
			"mismatches=%u\n", test, oracleCells.count, candidateCells.count,
			bucketsMatch, recordsMatch, indicesMatch, oracleReturn, candidateReturn,
			oracleBefore == oracleAfter, candidateBefore == candidateAfter,
			matched ? 0u : 1u);
		if(!matched)
			++mismatches;
		}
	printf("fluid collision triangle-index-builder cases=3 mesh=1 empty=1 "
		"null_output=1 candidate_present=1 mismatches=%u\n", mismatches);
	return mismatches;
	}

static unsigned nxRunFluidCollisionShapePackerDifferential(
	const unsigned char* oracleBase, void* candidateAddress)
	{
	if(!oracleBase || !candidateAddress)
		return 1;
	typedef int (__cdecl* PackShape)(unsigned*, void*);
	PackShape oracle = reinterpret_cast<PackShape>(
		const_cast<unsigned char*>(oracleBase) + 0x87580);
	PackShape candidate = reinterpret_cast<PackShape>(candidateAddress);
	unsigned mismatches = 0;
	for(unsigned test = 0; test != 2; ++test)
		{
		unsigned char shape[512];
		unsigned char shapeSource[32];
		unsigned records[2][6];
		memset(shape, 0, sizeof(shape));
		memset(shapeSource, 0, sizeof(shapeSource));
		for(unsigned i = 0; i != 12; ++i)
			reinterpret_cast<unsigned*>(shape + 12)[i] =
				0x3e800001u + i * 0x01010101u;
		for(unsigned record = 0; record != 2; ++record)
			for(unsigned word = 0; word != 6; ++word)
				records[record][word] = 0x41000000u + record * 0x100u + word;
		*reinterpret_cast<void**>(shape + 196) = shapeSource;
		*reinterpret_cast<void**>(shapeSource + 20) = records;
		*reinterpret_cast<unsigned short*>(shape + 204) =
			static_cast<unsigned short>(test);
		*reinterpret_cast<unsigned char*>(shape + 172) = 2;
		*reinterpret_cast<unsigned*>(shape + 444) = test ? 0x100u : 0u;
		*reinterpret_cast<unsigned*>(shape + 4) = 0x12340000u + test;
		unsigned oracleOutput[20], candidateOutput[20];
		memset(oracleOutput, 0xa5, sizeof(oracleOutput));
		memcpy(candidateOutput, oracleOutput, sizeof(oracleOutput));
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		const int oracleReturn = oracle(oracleOutput, shape);
		__asm mov oracleAfter, esp
		__asm mov candidateBefore, esp
		const int candidateReturn = candidate(candidateOutput, shape);
		__asm mov candidateAfter, esp
		const bool outputsMatch = memcmp(oracleOutput, candidateOutput,
			sizeof(oracleOutput)) == 0;
		const bool stackMatches = oracleBefore == oracleAfter
			&& candidateBefore == candidateAfter;
		const bool matched = outputsMatch && oracleReturn == candidateReturn
			&& stackMatches;
		printf("fluid collision shape-pack case=%u return=%08x/%08x "
			"output=%u stack=%u/%u mismatches=%u\n", test,
			static_cast<unsigned>(oracleReturn),
			static_cast<unsigned>(candidateReturn), outputsMatch,
			oracleBefore == oracleAfter, candidateBefore == candidateAfter,
			matched ? 0u : 1u);
		if(!matched)
			++mismatches;
		}
	printf("fluid collision shape-pack cases=2 selected_record=1 flag=1 "
		"candidate_present=1 mismatches=%u\n", mismatches);
	return mismatches;
	}

static unsigned nxRunFluidCollisionShapeCounterDifferential(
	const unsigned char* oracleBase, void* candidateAddress)
	{
	typedef int (__cdecl* CountShape)(void*, void*);
	CountShape oracle = oracleBase ? reinterpret_cast<CountShape>(
		const_cast<unsigned char*>(oracleBase) + 0x86400) : 0;
	CountShape candidate = reinterpret_cast<CountShape>(candidateAddress);
	struct Case
		{
		unsigned short shapeFlags;
		unsigned char ownerFlags;
		unsigned type;
		unsigned counterOffset;
		unsigned returnKind;
		};
	static const Case cases[] = {
		{ 0, 0, 0, 0x34, 0 }, { 0, 0, 1, 0x4c, 0 },
		{ 0, 0, 2, 0x40, 0 }, { 0, 0, 3, 0x58, 0 },
		{ 0, 0, 4, 0x04, 0 }, { 0, 0, 5, 0, 2 },
		{ 0x80, 0, 0, 0x70, 0 }, { 0x80, 0, 1, 0x7c, 0 },
		{ 0x80, 0, 2, 0x94, 0 }, { 0x80, 0, 3, 0x88, 0 },
		{ 0x80, 0, 4, 0, 2 }, { 0, 4, 2, 0, 1 },
		{ 0x200, 0, 0, 0, 3 }
		};
	if(!oracleBase || !candidateAddress)
		{
		printf("fluid collision shape-count-router cases=13 static=6 dynamic=5 "
			"excluded=1 refresh=1 candidate_present=%u oracle_contract=0 "
			"stack=0/0 mismatches=1\n", candidateAddress != 0);
		return 1;
		}
	unsigned mismatches = 0;
	unsigned oracleStackBalanced = 1, candidateStackBalanced = 1;
	unsigned oracleContract = 1;
	for(unsigned test = 0; test != sizeof(cases) / sizeof(cases[0]); ++test)
		{
		unsigned char oracleBuilder[256], candidateBuilder[256];
		unsigned char oracleOwner[32], candidateOwner[32];
		unsigned char oracleShape[224], candidateShape[224];
		memset(oracleBuilder, 0, sizeof(oracleBuilder));
		memset(candidateBuilder, 0, sizeof(candidateBuilder));
		memset(oracleOwner, 0, sizeof(oracleOwner));
		memset(candidateOwner, 0, sizeof(candidateOwner));
		memset(oracleShape, 0, sizeof(oracleShape));
		memset(candidateShape, 0, sizeof(candidateShape));
		*reinterpret_cast<unsigned short*>(oracleShape + 0xde) =
			cases[test].shapeFlags;
		*reinterpret_cast<unsigned short*>(candidateShape + 0xde) =
			cases[test].shapeFlags;
		oracleOwner[0x14] = cases[test].ownerFlags;
		candidateOwner[0x14] = cases[test].ownerFlags;
		*reinterpret_cast<void**>(oracleShape + 4) = oracleOwner;
		*reinterpret_cast<void**>(candidateShape + 4) = candidateOwner;
		*reinterpret_cast<unsigned*>(oracleShape + 0xd0) = cases[test].type;
		*reinterpret_cast<unsigned*>(candidateShape + 0xd0) = cases[test].type;
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		const int oracleReturn = oracle(oracleBuilder, oracleShape);
		__asm mov oracleAfter, esp
		__asm mov candidateBefore, esp
		const int candidateReturn = candidate(candidateBuilder, candidateShape);
		__asm mov candidateAfter, esp
		const bool oracleStack = oracleBefore == oracleAfter;
		const bool candidateStack = candidateBefore == candidateAfter;
		oracleStackBalanced = oracleStackBalanced && oracleStack;
		candidateStackBalanced = candidateStackBalanced && candidateStack;
		bool oracleReturnValid = false, candidateReturnValid = false;
		if(cases[test].returnKind == 0)
			{
			oracleReturnValid = oracleReturn == reinterpret_cast<int>(oracleBuilder);
			candidateReturnValid = candidateReturn == reinterpret_cast<int>(candidateBuilder);
			}
		else if(cases[test].returnKind == 1)
			{
			oracleReturnValid = oracleReturn == reinterpret_cast<int>(oracleOwner);
			candidateReturnValid = candidateReturn == reinterpret_cast<int>(candidateOwner);
			}
		else if(cases[test].returnKind == 2)
			{
			oracleReturnValid = oracleReturn == static_cast<int>(cases[test].type);
			candidateReturnValid = candidateReturn == static_cast<int>(cases[test].type);
			}
		else
			{
			oracleReturnValid = oracleReturn == 0x200;
			candidateReturnValid = candidateReturn == 0x200;
			}
		bool oracleCountsValid = true, candidateCountsValid = true;
		for(unsigned offset = 4; offset <= 0x94; offset += 4)
			{
				unsigned expected = 0;
				if(cases[test].counterOffset == offset)
					expected = 1;
				if(*reinterpret_cast<unsigned*>(oracleBuilder + offset) != expected)
					oracleCountsValid = false;
				if(*reinterpret_cast<unsigned*>(candidateBuilder + offset) != expected)
					candidateCountsValid = false;
			}
		const bool stateMatches = memcmp(oracleBuilder, candidateBuilder,
			sizeof(oracleBuilder)) == 0
			&& memcmp(oracleOwner, candidateOwner, sizeof(oracleOwner)) == 0
			&& memcmp(oracleShape, candidateShape, 4) == 0
			&& memcmp(oracleShape + 8, candidateShape + 8,
				sizeof(oracleShape) - 8) == 0;
		const bool matched = oracleReturnValid && candidateReturnValid
			&& oracleCountsValid && candidateCountsValid && stateMatches
			&& oracleStack && candidateStack;
		if(!matched)
			printf("fluid collision shape-count-router detail case=%u type=%u "
				"shape_flags=%04x owner=%02x return=%u/%u counts=%u/%u state=%u "
				"stack=%u/%u expected_offset=%x return_kind=%u\n", test,
				cases[test].type, cases[test].shapeFlags,
				cases[test].ownerFlags, oracleReturnValid, candidateReturnValid,
				oracleCountsValid, candidateCountsValid, stateMatches,
				oracleStack, candidateStack, cases[test].counterOffset,
				cases[test].returnKind);
		oracleContract &= oracleReturnValid && oracleCountsValid && oracleStack;
		if(!matched)
			++mismatches;
		}
	printf("fluid collision shape-count-router cases=13 static=6 dynamic=5 "
		"excluded=1 refresh=1 candidate_present=1 oracle_contract=%u "
		"stack=%u/%u mismatches=%u\n", oracleContract,
		oracleStackBalanced, candidateStackBalanced, mismatches);
	return mismatches;
	}

struct NxFluidCollisionSceneShapeNode
	{
	void* unused;
	void* shape;
	};

struct NxFluidCollisionSceneShapeRecord
	{
	unsigned char bytes[0xe8];
	unsigned char owner[0x20];
	};

struct NxFluidCollisionSceneShapeFixture
	{
	unsigned char scene[0x700];
	unsigned char container[32];
	void* entries[4];
	void* childPointers[4];
	NxFluidCollisionSceneShapeNode nodes[4];
	NxFluidCollisionSceneShapeRecord top[4];
	NxFluidCollisionSceneShapeRecord child[4];
	};

static void nxInitFluidCollisionSceneShape(NxFluidCollisionSceneShapeRecord* shape,
	unsigned type, unsigned short flags, unsigned char ownerFlags)
	{
	memset(shape, 0, sizeof(*shape));
	shape->owner[0x14] = ownerFlags;
	*reinterpret_cast<void**>(shape->bytes + 4) = shape->owner;
	*reinterpret_cast<unsigned*>(shape->bytes + 0xd0) = type;
	*reinterpret_cast<unsigned short*>(shape->bytes + 0xde) = flags;
	}

static void nxInitFluidCollisionSceneShapeCollection(
	NxFluidCollisionSceneShapeFixture* fixture, unsigned test)
	{
	memset(fixture, 0, sizeof(*fixture));
	unsigned topCount = 0;
	if(test == 1)
		{
		topCount = 4;
		nxInitFluidCollisionSceneShape(&fixture->top[0], 0, 0, 0);
		nxInitFluidCollisionSceneShape(&fixture->top[1], 4, 0, 0);
		nxInitFluidCollisionSceneShape(&fixture->top[2], 5, 0, 0);
		nxInitFluidCollisionSceneShape(&fixture->top[3], 9, 0, 0);
		nxInitFluidCollisionSceneShape(&fixture->child[0], 2, 0, 0);
		nxInitFluidCollisionSceneShape(&fixture->child[1], 3, 0, 0);
		fixture->childPointers[0] = fixture->child[0].bytes;
		fixture->childPointers[1] = fixture->child[1].bytes;
		*reinterpret_cast<void**>(fixture->top[2].bytes + 0xe0) =
			fixture->childPointers;
		*reinterpret_cast<void**>(fixture->top[2].bytes + 0xe4) =
			fixture->childPointers + 2;
		}
	else if(test == 2)
		{
		topCount = 2;
		nxInitFluidCollisionSceneShape(&fixture->top[0], 1, 0x80, 0);
		nxInitFluidCollisionSceneShape(&fixture->top[1], 5, 0, 0);
		nxInitFluidCollisionSceneShape(&fixture->child[0], 3, 0x80, 0);
		nxInitFluidCollisionSceneShape(&fixture->child[1], 1, 0, 4);
		nxInitFluidCollisionSceneShape(&fixture->child[2], 0, 0x200, 0);
		fixture->childPointers[0] = fixture->child[0].bytes;
		fixture->childPointers[1] = fixture->child[1].bytes;
		fixture->childPointers[2] = fixture->child[2].bytes;
		*reinterpret_cast<void**>(fixture->top[1].bytes + 0xe0) =
			fixture->childPointers;
		*reinterpret_cast<void**>(fixture->top[1].bytes + 0xe4) =
			fixture->childPointers + 3;
		}
	if(topCount == 0)
		return;
	*reinterpret_cast<void**>(fixture->scene + 0x624 + 0x1c) = fixture->container;
	*reinterpret_cast<unsigned*>(fixture->container + 4) = 0;
	*reinterpret_cast<unsigned*>(fixture->container + 8) = topCount;
	*reinterpret_cast<unsigned*>(fixture->container + 12) = 0;
	*reinterpret_cast<void***>(fixture->container + 24) = fixture->entries;
	for(unsigned i = 0; i != topCount; ++i)
		{
		fixture->entries[i] = &fixture->nodes[i];
		fixture->nodes[i].shape = fixture->top[i].bytes;
		}
	}

static unsigned nxRunFluidCollisionSceneShapeCounterDifferential(
	const unsigned char* oracleBase, void* candidateAddress)
	{
	typedef int (__cdecl* CountSceneShapes)(void*, void*);
	CountSceneShapes oracle = oracleBase ? reinterpret_cast<CountSceneShapes>(
		const_cast<unsigned char*>(oracleBase) + 0x87880) : 0;
	CountSceneShapes candidate = reinterpret_cast<CountSceneShapes>(candidateAddress);
	if(!oracleBase || !candidateAddress)
		{
		printf("fluid collision scene-shape-count cases=3 empty=1 collections=2 "
			"compound_lists=2 routed_shapes=9 candidate_present=%u "
			"oracle_contract=0 stack=0/0 mismatches=1\n", candidateAddress != 0);
		return 1;
		}
	unsigned mismatches = 0, oracleContract = 1;
	unsigned oracleStackBalanced = 1, candidateStackBalanced = 1;
	for(unsigned test = 0; test != 3; ++test)
		{
		NxFluidCollisionSceneShapeFixture oracleFixture, candidateFixture;
		nxInitFluidCollisionSceneShapeCollection(&oracleFixture, test);
		nxInitFluidCollisionSceneShapeCollection(&candidateFixture, test);
		unsigned char oracleBuilder[256], candidateBuilder[256];
		memset(oracleBuilder, 0, sizeof(oracleBuilder));
		memset(candidateBuilder, 0, sizeof(candidateBuilder));
		const unsigned expectedReturn = test == 1 ? 4u : test == 2 ? 2u : 0u;
		unsigned expected[256 / 4] = {};
		if(test == 1)
			{
			expected[0x34 / 4] = 1;
			expected[0x04 / 4] = 1;
			expected[0x40 / 4] = 1;
			expected[0x58 / 4] = 1;
			}
		else if(test == 2)
			{
			expected[0x7c / 4] = 1;
			expected[0x88 / 4] = 1;
			}
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		const int oracleReturn = oracle(oracleBuilder, oracleFixture.scene);
		__asm mov oracleAfter, esp
		__asm mov candidateBefore, esp
		const int candidateReturn = candidate(candidateBuilder, candidateFixture.scene);
		__asm mov candidateAfter, esp
		const bool oracleStack = oracleBefore == oracleAfter;
		const bool candidateStack = candidateBefore == candidateAfter;
		oracleStackBalanced = oracleStackBalanced && oracleStack;
		candidateStackBalanced = candidateStackBalanced && candidateStack;
		bool oracleCounts = true, candidateCounts = true;
		for(unsigned offset = 4; offset <= 0x94; offset += 4)
			{
			const unsigned count = expected[offset / 4];
			if(*reinterpret_cast<unsigned*>(oracleBuilder + offset) != count)
				oracleCounts = false;
			if(*reinterpret_cast<unsigned*>(candidateBuilder + offset) != count)
				candidateCounts = false;
			}
		const bool matched = oracleReturn == static_cast<int>(expectedReturn)
			&& candidateReturn == static_cast<int>(expectedReturn)
			&& oracleCounts && candidateCounts
			&& memcmp(oracleBuilder, candidateBuilder, sizeof(oracleBuilder)) == 0
			&& oracleStack && candidateStack;
		oracleContract &= oracleReturn == static_cast<int>(expectedReturn)
			&& oracleCounts && oracleStack;
		if(!matched)
			{
			++mismatches;
			printf("fluid collision scene-shape-count detail case=%u return=%d/%d "
				"counts=%u/%u stack=%u/%u\n", test, oracleReturn,
				candidateReturn, oracleCounts, candidateCounts,
				oracleStack, candidateStack);
			}
		}
	printf("fluid collision scene-shape-count cases=3 empty=1 collections=2 "
		"compound_lists=2 routed_shapes=9 candidate_present=1 oracle_contract=%u "
		"stack=%u/%u mismatches=%u\n", oracleContract,
		oracleStackBalanced, candidateStackBalanced, mismatches);
	return mismatches;
	}

static unsigned nxRunFluidCollisionSphereAppendDifferential(
	const unsigned char* oracleBase, void* candidateAddress)
	{
	typedef void* (__cdecl* AppendSphere)(void*, void*);
	AppendSphere oracle = oracleBase ? reinterpret_cast<AppendSphere>(
		const_cast<unsigned char*>(oracleBase) + 0x87740) : 0;
	AppendSphere candidate = reinterpret_cast<AppendSphere>(candidateAddress);
	if(!oracleBase || !candidateAddress)
		{
		printf("fluid collision sphere-array append cases=2 selected=1 ignored=1 "
			"candidate_present=%u oracle_contract=0 stack=0/0 mismatches=1\n",
			candidateAddress != 0);
		return 1;
		}
	struct ShapeFixture
		{
		unsigned char bytes[0x1c0];
		unsigned char data[0x20];
		unsigned bounds[6];
		unsigned char owner[0x20];
		};
	struct ArrayDescriptor
		{
		unsigned char* data;
		unsigned count;
		unsigned capacity;
		};
	unsigned mismatches = 0, oracleContract = 1;
	unsigned oracleStackBalanced = 1, candidateStackBalanced = 1;
	for(unsigned test = 0; test != 2; ++test)
		{
		ShapeFixture oracleShape, candidateShape;
		memset(&oracleShape, 0, sizeof(oracleShape));
		memset(&candidateShape, 0, sizeof(candidateShape));
		for(unsigned word = 0; word != 12; ++word)
			{
			*reinterpret_cast<unsigned*>(oracleShape.bytes + 0x0c + word * 4) =
				0x3e800001u + word * 0x01010101u;
			*reinterpret_cast<unsigned*>(candidateShape.bytes + 0x0c + word * 4) =
				0x3e800001u + word * 0x01010101u;
			}
		for(unsigned word = 0; word != 6; ++word)
			{
			oracleShape.bounds[word] = 0x41000000u + word;
			candidateShape.bounds[word] = 0x41000000u + word;
			}
		const unsigned type = test == 0 ? 1u : 2u;
		*reinterpret_cast<unsigned*>(oracleShape.bytes + 4) = 0x12340001u;
		*reinterpret_cast<unsigned*>(candidateShape.bytes + 4) = 0x12340001u;
		*reinterpret_cast<unsigned*>(oracleShape.bytes + 0xd0) = type;
		*reinterpret_cast<unsigned*>(candidateShape.bytes + 0xd0) = type;
		*reinterpret_cast<unsigned*>(oracleShape.bytes + 0xe0) = 0x55667788u;
		*reinterpret_cast<unsigned*>(candidateShape.bytes + 0xe0) = 0x55667788u;
		*reinterpret_cast<unsigned*>(oracleShape.bytes + 0xe4) = 0x11223344u;
		*reinterpret_cast<unsigned*>(candidateShape.bytes + 0xe4) = 0x11223344u;
		*reinterpret_cast<unsigned short*>(oracleShape.bytes + 0xcc) = 0;
		*reinterpret_cast<unsigned short*>(candidateShape.bytes + 0xcc) = 0;
		*reinterpret_cast<unsigned**>(oracleShape.bytes + 0xc4) =
			reinterpret_cast<unsigned*>(oracleShape.data);
		*reinterpret_cast<unsigned**>(candidateShape.bytes + 0xc4) =
			reinterpret_cast<unsigned*>(candidateShape.data);
		*reinterpret_cast<unsigned**>(oracleShape.data + 0x14) = oracleShape.bounds;
		*reinterpret_cast<unsigned**>(candidateShape.data + 0x14) = candidateShape.bounds;
		*reinterpret_cast<unsigned*>(oracleShape.bytes + 0xac) = 2;
		*reinterpret_cast<unsigned*>(candidateShape.bytes + 0xac) = 2;
		unsigned char oracleOutput[168], candidateOutput[168];
		memset(oracleOutput, 0xa5, sizeof(oracleOutput));
		memset(candidateOutput, 0xa5, sizeof(candidateOutput));
		ArrayDescriptor oracleArray = { oracleOutput, 0, 2 };
		ArrayDescriptor candidateArray = { candidateOutput, 0, 2 };
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		void* const oracleReturn = oracle(&oracleArray, oracleShape.bytes);
		__asm mov oracleAfter, esp
		__asm mov candidateBefore, esp
		void* const candidateReturn = candidate(&candidateArray, candidateShape.bytes);
		__asm mov candidateAfter, esp
		const bool selected = test == 0;
		const unsigned expectedCount = selected ? 1u : 0u;
		const bool oracleReturnValid = oracleReturn == (selected
			? static_cast<void*>(&oracleArray) : static_cast<void*>(oracleShape.bytes));
		const bool candidateReturnValid = candidateReturn == (selected
			? static_cast<void*>(&candidateArray) : static_cast<void*>(candidateShape.bytes));
		const bool countsMatch = oracleArray.count == expectedCount
			&& candidateArray.count == expectedCount;
		bool recordMatches = true, tailsMatch = true;
		if(selected)
			{
			recordMatches = memcmp(oracleOutput, candidateOutput, 80) == 0;
			tailsMatch = memcmp(oracleOutput + 84, candidateOutput + 84,
				sizeof(oracleOutput) - 84) == 0;
			}
		else
			recordMatches = memcmp(oracleOutput, candidateOutput,
				sizeof(oracleOutput)) == 0;
		const bool oracleStack = oracleBefore == oracleAfter;
		const bool candidateStack = candidateBefore == candidateAfter;
		oracleStackBalanced = oracleStackBalanced && oracleStack;
		candidateStackBalanced = candidateStackBalanced && candidateStack;
		const bool matched = oracleReturnValid && candidateReturnValid
			&& countsMatch && recordMatches && tailsMatch
			&& oracleStack && candidateStack;
		oracleContract &= oracleReturnValid && countsMatch && recordMatches && oracleStack;
		if(!matched)
			++mismatches;
		}
	printf("fluid collision sphere-array append cases=2 selected=1 ignored=1 "
		"candidate_present=1 oracle_contract=%u stack=%u/%u mismatches=%u\n",
		oracleContract, oracleStackBalanced, candidateStackBalanced, mismatches);
	return mismatches;
	}

static unsigned nxRunFluidCollisionCapsuleAppendDifferential(
	const unsigned char* oracleBase, void* candidateAddress)
	{
	typedef void* (__cdecl* AppendCapsule)(void*, void*);
	AppendCapsule oracle = oracleBase ? reinterpret_cast<AppendCapsule>(
		const_cast<unsigned char*>(oracleBase) + 0x877a0) : 0;
	AppendCapsule candidate = reinterpret_cast<AppendCapsule>(candidateAddress);
	if(!oracleBase || !candidateAddress)
		{
		printf("fluid collision capsule-array append cases=2 selected=1 ignored=1 "
			"candidate_present=%u oracle_contract=0 stack=0/0 mismatches=1\n",
			candidateAddress != 0);
		return 1;
		}
	struct ShapeFixture
		{
		unsigned char bytes[0x1c0];
		unsigned char data[0x20];
		unsigned bounds[6];
		};
	struct ArrayDescriptor
		{
		unsigned char* data;
		unsigned count;
		unsigned capacity;
		};
	unsigned mismatches = 0, oracleContract = 1;
	unsigned oracleStackBalanced = 1, candidateStackBalanced = 1;
	for(unsigned test = 0; test != 2; ++test)
		{
		ShapeFixture oracleShape, candidateShape;
		memset(&oracleShape, 0, sizeof(oracleShape));
		memset(&candidateShape, 0, sizeof(candidateShape));
		for(unsigned word = 0; word != 12; ++word)
			{
			*reinterpret_cast<unsigned*>(oracleShape.bytes + 0x0c + word * 4) =
				0x3f000003u + word * 0x01020305u;
			*reinterpret_cast<unsigned*>(candidateShape.bytes + 0x0c + word * 4) =
				0x3f000003u + word * 0x01020305u;
			}
		for(unsigned word = 0; word != 6; ++word)
			{
			oracleShape.bounds[word] = 0x42000000u + word * 3;
			candidateShape.bounds[word] = 0x42000000u + word * 3;
			}
		const unsigned type = test == 0 ? 3u : 2u;
		for(unsigned side = 0; side != 2; ++side)
			{
			ShapeFixture& shape = side == 0 ? oracleShape : candidateShape;
			*reinterpret_cast<unsigned*>(shape.bytes + 4) = 0x12340003u;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xd0) = type;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xe0) = 0x55667788u;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xe4) = 0x11223344u;
			*reinterpret_cast<unsigned short*>(shape.bytes + 0xcc) = 0;
			*reinterpret_cast<unsigned**>(shape.bytes + 0xc4) =
				reinterpret_cast<unsigned*>(shape.data);
			*reinterpret_cast<unsigned**>(shape.data + 0x14) = shape.bounds;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xac) = 2;
			}
		unsigned char oracleOutput[176], candidateOutput[176];
		memset(oracleOutput, 0xa5, sizeof(oracleOutput));
		memset(candidateOutput, 0xa5, sizeof(candidateOutput));
		ArrayDescriptor oracleArray = { oracleOutput, 0, 2 };
		ArrayDescriptor candidateArray = { candidateOutput, 0, 2 };
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		void* const oracleReturn = oracle(&oracleArray, oracleShape.bytes);
		__asm mov oracleAfter, esp
		__asm mov candidateBefore, esp
		void* const candidateReturn = candidate(&candidateArray, candidateShape.bytes);
		__asm mov candidateAfter, esp
		const bool selected = test == 0;
		const unsigned expectedCount = selected ? 1u : 0u;
		const void* const expectedOracleReturn = selected
			? static_cast<void*>(&oracleArray) : 0;
		const void* const expectedCandidateReturn = selected
			? static_cast<void*>(&candidateArray) : 0;
		const bool oracleReturnValid = oracleReturn == expectedOracleReturn;
		const bool candidateReturnValid = candidateReturn == expectedCandidateReturn;
		const bool countsMatch = oracleArray.count == expectedCount
			&& candidateArray.count == expectedCount;
		bool recordMatches = true, tailsMatch = true;
		if(selected)
			{
			recordMatches = memcmp(oracleOutput, candidateOutput, 84) == 0
				&& *reinterpret_cast<unsigned*>(oracleOutput) == 0x55667788u
				&& *reinterpret_cast<unsigned*>(oracleOutput + 4) == 0x11223344u;
			tailsMatch = memcmp(oracleOutput + 88, candidateOutput + 88,
				sizeof(oracleOutput) - 88) == 0;
			}
		else
			recordMatches = memcmp(oracleOutput, candidateOutput,
				sizeof(oracleOutput)) == 0;
		const bool oracleStack = oracleBefore == oracleAfter;
		const bool candidateStack = candidateBefore == candidateAfter;
		oracleStackBalanced = oracleStackBalanced && oracleStack;
		candidateStackBalanced = candidateStackBalanced && candidateStack;
		const bool matched = oracleReturnValid && candidateReturnValid
			&& countsMatch && recordMatches && tailsMatch
			&& oracleStack && candidateStack;
		oracleContract &= oracleReturnValid && countsMatch && recordMatches
			&& oracleStack;
		if(!matched)
			++mismatches;
		}
	printf("fluid collision capsule-array append cases=2 selected=1 ignored=1 "
		"candidate_present=1 oracle_contract=%u stack=%u/%u mismatches=%u\n",
		oracleContract, oracleStackBalanced, candidateStackBalanced, mismatches);
	return mismatches;
	}

static unsigned gFluidCollisionPlaneAppendVirtualCalls;

static void __declspec(naked) nxFluidCollisionPlaneAppendFixtureVirtual()
	{
	__asm
		{
		mov eax, dword ptr [esp + 4]
		mov dword ptr [eax + 050h], 13579bdfh
		mov dword ptr [eax + 054h], 2468ace0h
		mov dword ptr [eax + 058h], 0badf00dh
		mov dword ptr [eax + 05ch], 0cafebabeh
		inc dword ptr [gFluidCollisionPlaneAppendVirtualCalls]
		ret 4
		}
	}

static unsigned nxRunFluidCollisionPlaneAppendDifferential(
	const unsigned char* oracleBase, void* candidateAddress)
	{
	typedef void* (__cdecl* AppendPlane)(void*, void*);
	AppendPlane oracle = oracleBase ? reinterpret_cast<AppendPlane>(
		const_cast<unsigned char*>(oracleBase) + 0x87940) : 0;
	const bool candidatePresent = candidateAddress != 0;
	AppendPlane candidate = candidatePresent
		? reinterpret_cast<AppendPlane>(candidateAddress) : oracle;
	if(!oracleBase)
		{
		printf("fluid collision plane-array append cases=2 selected=1 ignored=1 "
			"candidate_present=%u oracle_contract=0 stack=0/0 mismatches=1\n",
			candidatePresent);
		return 1;
		}
	struct ShapeFixture
		{
		void** vtable;
		unsigned char bytes[0x1bc];
		};
	struct ArrayDescriptor
		{
		unsigned char* data;
		unsigned count;
		unsigned capacity;
		};
	unsigned mismatches = candidatePresent ? 0u : 1u, oracleContract = 1;
	unsigned oracleStackBalanced = 1, candidateStackBalanced = 1;
	gFluidCollisionPlaneAppendVirtualCalls = 0;
	for(unsigned test = 0; test != 2; ++test)
		{
		ShapeFixture oracleShape, candidateShape;
		void* oracleVtable[14];
		void* candidateVtable[14];
		memset(&oracleShape, 0, sizeof(oracleShape));
		memset(&candidateShape, 0, sizeof(candidateShape));
		for(unsigned slot = 0; slot != 14; ++slot)
			{
			oracleVtable[slot] = reinterpret_cast<void*>(
				&nxFluidCollisionPlaneAppendFixtureVirtual);
			candidateVtable[slot] = reinterpret_cast<void*>(
				&nxFluidCollisionPlaneAppendFixtureVirtual);
			}
		oracleShape.vtable = oracleVtable;
		candidateShape.vtable = candidateVtable;
		const unsigned type = test == 0 ? 0u : 1u;
		*reinterpret_cast<unsigned*>(reinterpret_cast<unsigned char*>(&oracleShape) + 0xd0) = type;
		*reinterpret_cast<unsigned*>(reinterpret_cast<unsigned char*>(&candidateShape) + 0xd0) = type;
		unsigned char oracleOutput[192], candidateOutput[192];
		memset(oracleOutput, 0xa5, sizeof(oracleOutput));
		memset(candidateOutput, 0xa5, sizeof(candidateOutput));
		ArrayDescriptor oracleArray = { oracleOutput, 0, 2 };
		ArrayDescriptor candidateArray = { candidateOutput, 0, 2 };
		const unsigned callsBefore = gFluidCollisionPlaneAppendVirtualCalls;
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		void* const oracleReturn = oracle(&oracleArray, &oracleShape);
		__asm mov oracleAfter, esp
		__asm mov candidateBefore, esp
		void* const candidateReturn = candidate(&candidateArray, &candidateShape);
		__asm mov candidateAfter, esp
		const bool selected = test == 0;
		const void* const expectedOracleReturn = selected
			? static_cast<void*>(&oracleArray) : 0;
		const void* const expectedCandidateReturn = selected
			? static_cast<void*>(&candidateArray) : 0;
		const bool returnValid = oracleReturn == expectedOracleReturn
			&& candidateReturn == expectedCandidateReturn;
		const unsigned expectedCount = selected ? 1u : 0u;
		const bool countsMatch = oracleArray.count == expectedCount
			&& candidateArray.count == expectedCount;
		bool recordMatches = true, tailsMatch = true;
		if(selected)
			{
			const unsigned expected[4] = {
			0, 0x13579bdfu, 0x2468ace0u, 0x0badf00du };
			recordMatches = memcmp(oracleOutput, candidateOutput, sizeof(expected)) == 0
				&& memcmp(oracleOutput, expected, sizeof(expected)) == 0;
			oracleContract &= memcmp(oracleOutput, expected, sizeof(expected)) == 0;
			tailsMatch = memcmp(oracleOutput + 96, candidateOutput + 96,
				sizeof(oracleOutput) - 96) == 0;
			}
		else
			recordMatches = memcmp(oracleOutput, candidateOutput,
				sizeof(oracleOutput)) == 0;
		const bool oracleStack = oracleBefore == oracleAfter;
		const bool candidateStack = candidateBefore == candidateAfter;
		const unsigned expectedCalls = selected ? 2u : 0u;
		const bool virtualCallsMatch =
			gFluidCollisionPlaneAppendVirtualCalls - callsBefore == expectedCalls;
		oracleStackBalanced = oracleStackBalanced && oracleStack;
		candidateStackBalanced = candidateStackBalanced && candidateStack;
		oracleContract &= (oracleReturn == expectedOracleReturn)
			&& oracleArray.count == expectedCount && oracleStack;
		if(!returnValid || !countsMatch || !recordMatches || !tailsMatch
			|| !virtualCallsMatch
			|| !oracleStack || !candidateStack)
			{
			++mismatches;
			printf("fluid collision plane-array append detail case=%u return=%p/%p "
				"count=%u/%u words=%08x.%08x.%08x.%08x "
				"calls=%u expected_calls=%u stack=%u/%u\n", test,
				oracleReturn, candidateReturn, oracleArray.count,
				candidateArray.count,
				*reinterpret_cast<unsigned*>(oracleOutput),
				*reinterpret_cast<unsigned*>(oracleOutput + 4),
				*reinterpret_cast<unsigned*>(oracleOutput + 8),
				*reinterpret_cast<unsigned*>(oracleOutput + 12),
				gFluidCollisionPlaneAppendVirtualCalls - callsBefore,
				expectedCalls, oracleStack, candidateStack);
			}
		}
	printf("fluid collision plane-array append cases=2 selected=1 ignored=1 "
		"candidate_present=%u oracle_contract=%u stack=%u/%u mismatches=%u\n",
		candidatePresent, oracleContract, oracleStackBalanced,
		candidateStackBalanced, mismatches);
	return mismatches;
	}

static unsigned nxRunFluidCollisionMeshAppendDifferential(
	const unsigned char* oracleBase, void* candidateAddress)
	{
	typedef void* (__cdecl* AppendMesh)(void*, void*);
	AppendMesh oracle = oracleBase ? reinterpret_cast<AppendMesh>(
		const_cast<unsigned char*>(oracleBase) + 0x876d0) : 0;
	const bool candidatePresent = candidateAddress != 0;
	AppendMesh candidate = candidatePresent
		? reinterpret_cast<AppendMesh>(candidateAddress) : oracle;
	if(!oracleBase)
		{
		printf("fluid collision mesh-array append cases=2 selected=1 ignored=1 "
			"candidate_present=%u oracle_contract=0 stack=0/0 mismatches=1\n",
			candidatePresent);
		return 1;
		}
	struct ShapeFixture
		{
		unsigned char bytes[0x1c0];
		unsigned char shapeData[0x20];
		unsigned bounds[6];
		unsigned meshWords[8];
	};
	struct ArrayDescriptor
		{
		unsigned char* data;
		unsigned count;
		unsigned capacity;
		};
	unsigned mismatches = candidatePresent ? 0u : 1u, oracleContract = 1;
	unsigned oracleStackBalanced = 1, candidateStackBalanced = 1;
	for(unsigned test = 0; test != 2; ++test)
		{
		ShapeFixture oracleShape, candidateShape;
		memset(&oracleShape, 0, sizeof(oracleShape));
		memset(&candidateShape, 0, sizeof(candidateShape));
		for(unsigned side = 0; side != 2; ++side)
			{
			ShapeFixture& shape = side == 0 ? oracleShape : candidateShape;
			for(unsigned word = 0; word != 12; ++word)
				*reinterpret_cast<unsigned*>(shape.bytes + 0x0c + word * 4) =
					0x3f21000bu + word * 0x01020407u;
			for(unsigned word = 0; word != 6; ++word)
				shape.bounds[word] = 0x44000000u + word * 7;
			shape.meshWords[0] = 0x11223344u;
			shape.meshWords[1] = 0x55667788u;
			shape.meshWords[2] = 0x99aabbccu;
			shape.meshWords[3] = 0xddeeff00u;
			shape.meshWords[4] = 0x13579bdfu;
			shape.meshWords[5] = 0x2468ace0u;
			*reinterpret_cast<unsigned*>(shape.bytes + 4) = 0x12340004u;
			*reinterpret_cast<unsigned short*>(shape.bytes + 0xcc) = 0;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xd0) = 4;
			*reinterpret_cast<unsigned**>(shape.bytes + 0xc4) =
				reinterpret_cast<unsigned*>(shape.shapeData);
			*reinterpret_cast<unsigned**>(shape.shapeData + 0x14) = shape.bounds;
			*reinterpret_cast<unsigned short*>(shape.bytes + 0xde) = 0;
			*reinterpret_cast<unsigned*>(shape.bytes + 0x1bc) = 0;
			*reinterpret_cast<unsigned**>(shape.bytes + 0xe0) =
				test == 0 ? shape.meshWords : reinterpret_cast<unsigned*>(-8);
			}
		unsigned char oracleOutput[192], candidateOutput[192];
		memset(oracleOutput, 0xa5, sizeof(oracleOutput));
		memset(candidateOutput, 0xa5, sizeof(candidateOutput));
		ArrayDescriptor oracleArray = { oracleOutput, 0, 2 };
		ArrayDescriptor candidateArray = { candidateOutput, 0, 2 };
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		void* const oracleReturn = oracle(&oracleArray, oracleShape.bytes);
		__asm mov oracleAfter, esp
		__asm mov candidateBefore, esp
		void* const candidateReturn = candidate(&candidateArray, candidateShape.bytes);
		__asm mov candidateAfter, esp
		const bool selected = test == 0;
		const void* const expectedOracleReturn = selected
			? static_cast<void*>(&oracleArray) : 0;
		const void* const expectedCandidateReturn = selected
			? static_cast<void*>(&candidateArray) : 0;
		const unsigned expectedCount = selected ? 1u : 0u;
		const bool oracleReturnValid = oracleReturn == expectedOracleReturn;
		const bool candidateReturnValid = candidateReturn == expectedCandidateReturn;
		const bool countsMatch = oracleArray.count == expectedCount
			&& candidateArray.count == expectedCount;
		bool recordMatches = true, tailsMatch = true;
		if(selected)
			{
			const unsigned expectedGeometry[4] = {
				0xddeeff00u, 0x2468ace0u, 0x99aabbccu, 0x13579bdfu };
			recordMatches = memcmp(oracleOutput, candidateOutput, 92) == 0
				&& memcmp(oracleOutput, expectedGeometry, sizeof(expectedGeometry)) == 0;
			oracleContract &= memcmp(oracleOutput, expectedGeometry,
				sizeof(expectedGeometry)) == 0;
			tailsMatch = memcmp(oracleOutput + 96, candidateOutput + 96,
				sizeof(oracleOutput) - 96) == 0;
			}
		else
			recordMatches = memcmp(oracleOutput, candidateOutput,
				sizeof(oracleOutput)) == 0;
		const bool oracleStack = oracleBefore == oracleAfter;
		const bool candidateStack = candidateBefore == candidateAfter;
		oracleStackBalanced = oracleStackBalanced && oracleStack;
		candidateStackBalanced = candidateStackBalanced && candidateStack;
		oracleContract &= oracleReturnValid && oracleArray.count == expectedCount
			&& oracleStack;
		if(!oracleReturnValid || !candidateReturnValid || !countsMatch
			|| !recordMatches || !tailsMatch || !oracleStack || !candidateStack)
			{
			++mismatches;
			printf("fluid collision mesh-array append detail case=%u "
				"return=%p/%p count=%u/%u stack=%u/%u\n", test,
				oracleReturn, candidateReturn, oracleArray.count,
				candidateArray.count, oracleStack, candidateStack);
			}
		}
	printf("fluid collision mesh-array append cases=2 selected=1 ignored=1 "
		"candidate_present=%u oracle_contract=%u stack=%u/%u mismatches=%u\n",
		candidatePresent, oracleContract, oracleStackBalanced,
		candidateStackBalanced, mismatches);
	return mismatches;
	}

static unsigned nxRunFluidCollisionShapeArrayRouterDifferential(
	const unsigned char* oracleBase, void* candidateAddress)
	{
	typedef void* (__cdecl* ShapeArrayRouter)(void*, void*);
	ShapeArrayRouter oracle = oracleBase ? reinterpret_cast<ShapeArrayRouter>(
		const_cast<unsigned char*>(oracleBase) + 0x87a40) : 0;
	const bool candidatePresent = candidateAddress != 0;
	ShapeArrayRouter candidate = candidatePresent
		? reinterpret_cast<ShapeArrayRouter>(candidateAddress) : oracle;
	if(!oracleBase)
		{
		printf("fluid collision shape-array router cases=13 candidate_present=%u "
			"oracle_contract=0 stack=0/0 mismatches=1\n", candidatePresent);
		return 1;
		}
	struct ShapeFixture
		{
		unsigned char bytes[0x1c0];
		unsigned char owner[0x20];
		unsigned char shapeData[0x20];
		unsigned bounds[6];
		unsigned meshWords[8];
		void* vtable[14];
		};
	struct BuilderFixture
		{
		unsigned char bytes[0x100];
		unsigned char outputs[9][192];
		};
	struct ArrayDescriptor
		{
		unsigned char* data;
		unsigned count;
		unsigned capacity;
		};
	static const unsigned descriptorOffsets[9] =
		{ 0, 0x30, 0x3c, 0x48, 0x54, 0x6c, 0x78, 0x84, 0x90 };
	unsigned mismatches = candidatePresent ? 0u : 1u, oracleContract = 1;
	unsigned oracleStackBalanced = 1, candidateStackBalanced = 1;
	for(unsigned test = 0; test != 13; ++test)
		{
		ShapeFixture oracleShape, candidateShape;
		BuilderFixture oracleBuilder, candidateBuilder;
		unsigned char ownerState[0x20] = {};
		memset(&oracleShape, 0, sizeof(oracleShape));
		memset(&candidateShape, 0, sizeof(candidateShape));
		memset(&oracleBuilder, 0, sizeof(oracleBuilder));
		memset(&candidateBuilder, 0, sizeof(candidateBuilder));
		for(unsigned i = 0; i != 9; ++i)
			{
			memset(oracleBuilder.outputs[i], 0xa5, sizeof(oracleBuilder.outputs[i]));
			memset(candidateBuilder.outputs[i], 0xa5, sizeof(candidateBuilder.outputs[i]));
			ArrayDescriptor* const oracleArray = reinterpret_cast<ArrayDescriptor*>(
				oracleBuilder.bytes + descriptorOffsets[i]);
			ArrayDescriptor* const candidateArray = reinterpret_cast<ArrayDescriptor*>(
				candidateBuilder.bytes + descriptorOffsets[i]);
			oracleArray->data = oracleBuilder.outputs[i];
			candidateArray->data = candidateBuilder.outputs[i];
			oracleArray->capacity = candidateArray->capacity = 2;
			}
		for(unsigned side = 0; side != 2; ++side)
			{
			ShapeFixture& shape = side == 0 ? oracleShape : candidateShape;
			for(unsigned word = 0; word != 12; ++word)
				*reinterpret_cast<unsigned*>(shape.bytes + 0x0c + word * 4) =
					0x3f33000du + word * 0x01020305u;
			for(unsigned word = 0; word != 6; ++word)
				shape.bounds[word] = 0x45000000u + word * 9;
			shape.owner[0x14] = 0;
			*reinterpret_cast<unsigned**>(shape.bytes + 4) =
				reinterpret_cast<unsigned*>(ownerState);
			*reinterpret_cast<unsigned short*>(shape.bytes + 0xcc) = 0;
			*reinterpret_cast<unsigned**>(shape.bytes + 0xc4) =
				reinterpret_cast<unsigned*>(shape.shapeData);
			*reinterpret_cast<unsigned**>(shape.shapeData + 0x14) = shape.bounds;
			*reinterpret_cast<unsigned*>(shape.bytes + 0x1bc) = 0;
			for(unsigned slot = 0; slot != 14; ++slot)
				shape.vtable[slot] = reinterpret_cast<void*>(
					&nxFluidCollisionPlaneAppendFixtureVirtual);
			*reinterpret_cast<void***>(shape.bytes) = shape.vtable;
			}
		const bool ownerCase = test == 11;
		const bool excludedCase = test == 10;
		const unsigned type = test < 5 ? test : (test < 10 ? test - 5
			: (test == 11 ? 1u : (test == 12 ? 5u : 2u)));
		const unsigned short flags = test >= 5 && test < 10 ? 0x80u
			: (excludedCase ? 0x200u : 0u);
		*reinterpret_cast<unsigned*>(reinterpret_cast<unsigned char*>(&oracleShape) + 0xd0) = type;
		*reinterpret_cast<unsigned*>(reinterpret_cast<unsigned char*>(&candidateShape) + 0xd0) = type;
		*reinterpret_cast<unsigned short*>(oracleShape.bytes + 0xde) = flags;
		*reinterpret_cast<unsigned short*>(candidateShape.bytes + 0xde) = flags;
		if(ownerCase)
			ownerState[0x14] = 4;
		if(type == 4)
			{
			*reinterpret_cast<unsigned**>(oracleShape.bytes + 0xe0) = oracleShape.meshWords;
			*reinterpret_cast<unsigned**>(candidateShape.bytes + 0xe0) = candidateShape.meshWords;
			}
		else
			{
			*reinterpret_cast<unsigned*>(oracleShape.bytes + 0xe0) = 0x55667788u;
			*reinterpret_cast<unsigned*>(candidateShape.bytes + 0xe0) = 0x55667788u;
			}
		for(unsigned side = 0; side != 2; ++side)
			{
			ShapeFixture& shape = side == 0 ? oracleShape : candidateShape;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xe4) = 0x11223344u;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xe8) = 0x99aabbccu;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xec) = 0xddeeff00u;
			shape.meshWords[0] = 0x01010101u;
			shape.meshWords[1] = 0x02020202u;
			shape.meshWords[2] = 0x03030303u;
			shape.meshWords[3] = 0x11223344u;
			shape.meshWords[4] = 0x13579bdfu;
			shape.meshWords[5] = 0x2468ace0u;
			}
		int selectedOffset = -1;
		if(!excludedCase && !ownerCase)
			{
			if(flags & 0x80u)
				{
				if(type == 0) selectedOffset = 0x6c;
				if(type == 1) selectedOffset = 0x78;
				if(type == 2) selectedOffset = 0x90;
				if(type == 3) selectedOffset = 0x84;
			}
			else
				{
				if(type == 0) selectedOffset = 0x30;
				if(type == 1) selectedOffset = 0x48;
				if(type == 2) selectedOffset = 0x3c;
				if(type == 3) selectedOffset = 0x54;
				if(type == 4) selectedOffset = 0;
				}
			}
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		void* const oracleReturn = oracle(oracleBuilder.bytes, oracleShape.bytes);
		__asm mov oracleAfter, esp
		__asm mov candidateBefore, esp
		void* const candidateReturn = candidate(candidateBuilder.bytes, candidateShape.bytes);
		__asm mov candidateAfter, esp
		void* expectedOracleReturn = reinterpret_cast<void*>(type);
		void* expectedCandidateReturn = reinterpret_cast<void*>(type);
		if(excludedCase)
			expectedOracleReturn = expectedCandidateReturn = reinterpret_cast<void*>(0x200);
		else if(ownerCase)
			{
			expectedOracleReturn = expectedCandidateReturn = ownerState;
			}
		else if(selectedOffset >= 0)
			{
			expectedOracleReturn = oracleBuilder.bytes + selectedOffset;
			expectedCandidateReturn = candidateBuilder.bytes + selectedOffset;
			}
		const bool oracleReturnValid = oracleReturn == expectedOracleReturn;
		const bool candidateReturnValid = candidateReturn == expectedCandidateReturn;
		const bool oracleStack = oracleBefore == oracleAfter;
		const bool candidateStack = candidateBefore == candidateAfter;
		oracleStackBalanced = oracleStackBalanced && oracleStack;
		candidateStackBalanced = candidateStackBalanced && candidateStack;
		bool buffersMatch = true, countsMatch = true;
		for(unsigned i = 0; i != 9; ++i)
			{
			ArrayDescriptor* const oracleArray = reinterpret_cast<ArrayDescriptor*>(
				oracleBuilder.bytes + descriptorOffsets[i]);
			ArrayDescriptor* const candidateArray = reinterpret_cast<ArrayDescriptor*>(
				candidateBuilder.bytes + descriptorOffsets[i]);
			const unsigned expectedCount = descriptorOffsets[i] ==
				static_cast<unsigned>(selectedOffset) ? 1u : 0u;
			if(oracleArray->count != expectedCount
				|| candidateArray->count != expectedCount)
				countsMatch = false;
			if(static_cast<int>(descriptorOffsets[i]) == selectedOffset)
				{
				const unsigned prefix = type == 0 ? 16u : type == 1 ? 80u
					: type == 2 ? 88u : type == 3 ? 84u : 92u;
				if(memcmp(oracleBuilder.outputs[i], candidateBuilder.outputs[i], prefix) != 0
					|| memcmp(oracleBuilder.outputs[i] + 96,
						candidateBuilder.outputs[i] + 96,
						sizeof(oracleBuilder.outputs[i]) - 96) != 0)
					buffersMatch = false;
				}
			else if(memcmp(oracleBuilder.outputs[i], candidateBuilder.outputs[i],
				sizeof(oracleBuilder.outputs[i])) != 0)
				buffersMatch = false;
			}
		oracleContract &= oracleReturnValid && countsMatch && oracleStack;
		if(!oracleReturnValid || !candidateReturnValid || !countsMatch
			|| !buffersMatch || !oracleStack || !candidateStack)
			{
			++mismatches;
			printf("fluid collision shape-array router detail case=%u type=%u flags=%04x "
				"return=%p/%p counts=%u buffers=%u stack=%u/%u\n", test,
				type, flags, oracleReturn, candidateReturn, countsMatch,
				buffersMatch, oracleStack, candidateStack);
			}
		}
	printf("fluid collision shape-array router cases=13 candidate_present=%u "
		"oracle_contract=%u stack=%u/%u mismatches=%u\n", candidatePresent,
		oracleContract, oracleStackBalanced, candidateStackBalanced, mismatches);
	return mismatches;
	}

static unsigned nxRunFluidCollisionSceneShapeCollectorDifferential(
	const unsigned char* oracleBase, void* candidateAddress)
	{
	typedef void (__cdecl* CollectSceneShapes)(void*, void*);
	CollectSceneShapes oracle = oracleBase ? reinterpret_cast<CollectSceneShapes>(
		const_cast<unsigned char*>(oracleBase) + 0x87b80) : 0;
	const bool candidatePresent = candidateAddress != 0;
	CollectSceneShapes candidate = candidatePresent
		? reinterpret_cast<CollectSceneShapes>(candidateAddress) : oracle;
	if(!oracleBase)
		{
		printf("fluid collision scene-shape-collector cases=2 empty=1 "
			"populated=1 compound_children=6 routed_shapes=12 "
			"candidate_present=%u oracle_contract=0 stack=0/0 mismatches=1\n",
			candidatePresent);
		return 1;
		}
	struct ShapeFixture
		{
		unsigned char bytes[0x1c0];
		unsigned char shapeData[0x20];
		unsigned bounds[6];
		unsigned meshWords[8];
		void* vtable[14];
		};
	struct Node { unsigned char prefix[4]; void* shape; };
	struct SceneFixture
		{
		unsigned char scene[0x700];
		unsigned char container[32];
		void* entries[10];
		Node nodes[8];
		ShapeFixture top[8];
		ShapeFixture child[6];
		void* childPointers[6];
		};
	struct BuilderFixture
		{
		unsigned char bytes[0xa0];
		unsigned char outputs[9][768];
		};
	struct ArrayDescriptor
		{
		unsigned char* data;
		unsigned count;
		unsigned capacity;
		};
	static const unsigned descriptorOffsets[9] =
		{ 0, 0x30, 0x3c, 0x48, 0x54, 0x6c, 0x78, 0x84, 0x90 };
	unsigned char owners[13][0x20] = {};
	SceneFixture oracleScene, candidateScene;
	BuilderFixture oracleBuilder, candidateBuilder;
	memset(&oracleScene, 0, sizeof(oracleScene));
	memset(&candidateScene, 0, sizeof(candidateScene));
	memset(&oracleBuilder, 0, sizeof(oracleBuilder));
	memset(&candidateBuilder, 0, sizeof(candidateBuilder));
	for(unsigned side = 0; side != 2; ++side)
		{
		SceneFixture& scene = side == 0 ? oracleScene : candidateScene;
		BuilderFixture& builder = side == 0 ? oracleBuilder : candidateBuilder;
		for(unsigned array = 0; array != 9; ++array)
			{
			memset(builder.outputs[array], 0xa5, sizeof(builder.outputs[array]));
			ArrayDescriptor* const descriptor = reinterpret_cast<ArrayDescriptor*>(
				builder.bytes + descriptorOffsets[array]);
			descriptor->data = builder.outputs[array];
			descriptor->count = 0;
			descriptor->capacity = 8;
			}
		auto initializeShape = [&](ShapeFixture& shape, unsigned logicalIndex,
			unsigned type, unsigned short flags)
			{
			memset(&shape, 0, sizeof(shape));
			for(unsigned word = 0; word != 12; ++word)
				*reinterpret_cast<unsigned*>(shape.bytes + 0x0c + word * 4) =
					0x3f100001u + logicalIndex * 0x00010101u + word * 0x01020407u;
			for(unsigned word = 0; word != 6; ++word)
				shape.bounds[word] = 0x41000000u + logicalIndex * 0x100u + word * 7u;
		for(unsigned word = 0; word != 8; ++word)
				shape.meshWords[word] = 0x51000000u + logicalIndex * 0x100u + word * 9u;
			*reinterpret_cast<void**>(shape.bytes + 4) = owners[logicalIndex];
			*reinterpret_cast<unsigned short*>(shape.bytes + 0xcc) = 0;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xd0) = type;
			*reinterpret_cast<unsigned short*>(shape.bytes + 0xde) = flags;
			*reinterpret_cast<void**>(shape.bytes + 0xc4) = shape.shapeData;
			*reinterpret_cast<void**>(shape.shapeData + 0x14) = shape.bounds;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xac) = 2;
			*reinterpret_cast<unsigned*>(shape.bytes + 0x1bc) = 0;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xe0) =
				0x61000000u + logicalIndex * 0x100u;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xe4) =
				0x62000000u + logicalIndex * 0x100u;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xe8) =
				0x63000000u + logicalIndex * 0x100u;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xec) =
				0x64000000u + logicalIndex * 0x100u;
			if(type == 4)
				*reinterpret_cast<void**>(shape.bytes + 0xe0) = shape.meshWords;
			for(unsigned slot = 0; slot != 14; ++slot)
				shape.vtable[slot] = reinterpret_cast<void*>(
					&nxFluidCollisionPlaneAppendFixtureVirtual);
			*reinterpret_cast<void***>(shape.bytes) = shape.vtable;
			};
		initializeShape(scene.top[0], 0, 1, 0); // sphere before the compound
		initializeShape(scene.top[1], 1, 2, 0); // ordinary box
		initializeShape(scene.top[2], 2, 5, 0); // compound
		initializeShape(scene.top[3], 3, 1, 0); // sphere after the compound
		initializeShape(scene.top[4], 4, 3, 0); // owner-managed capsule
		initializeShape(scene.top[5], 5, 4, 0x200); // excluded mesh
		initializeShape(scene.top[6], 6, 5, 0); // empty compound
		initializeShape(scene.top[7], 7, 1, 0); // final sphere
		static const unsigned childTypes[6] = { 2, 1, 0, 3, 4, 9 };
		for(unsigned child = 0; child != 6; ++child)
			{
			initializeShape(scene.child[child], 7 + child, childTypes[child], 0);
			scene.childPointers[child] = scene.child[child].bytes;
			}
		*reinterpret_cast<void**>(scene.top[2].bytes + 0xe0) =
			scene.childPointers;
		*reinterpret_cast<void**>(scene.top[2].bytes + 0xe4) =
			scene.childPointers + 6;
		*reinterpret_cast<void**>(scene.top[6].bytes + 0xe0) =
			scene.childPointers + 6;
		*reinterpret_cast<void**>(scene.top[6].bytes + 0xe4) =
			scene.childPointers + 6;
		owners[4][0x14] = 4;
		*reinterpret_cast<void**>(scene.scene + 0x640) = scene.container;
		*reinterpret_cast<unsigned*>(scene.container + 4) = 1;
		*reinterpret_cast<unsigned*>(scene.container + 8) = 3;
		*reinterpret_cast<unsigned*>(scene.container + 12) = 5;
		*reinterpret_cast<void***>(scene.container + 24) = scene.entries;
		for(unsigned index = 0; index != 8; ++index)
			{
			scene.entries[index + 1] = &scene.nodes[index];
			scene.nodes[index].shape = scene.top[index].bytes;
			}
		}
	// The two sides share logical owner addresses, as the packed record stores
	// the owner pointer rather than a shape-local address.
	owners[4][0x14] = 4;
	unsigned mismatches = candidatePresent ? 0u : 1u;
	bool oracleContract = true, oracleStackBalanced = true;
	bool candidateStackBalanced = true;
	const unsigned planeCallsBefore = gFluidCollisionPlaneAppendVirtualCalls;
	for(unsigned test = 0; test != 2; ++test)
		{
		SceneFixture* const oracleInput = test == 0 ? &oracleScene : &oracleScene;
		SceneFixture* const candidateInput = test == 0 ? &candidateScene : &candidateScene;
		if(test == 0)
			{
			*reinterpret_cast<void**>(oracleInput->scene + 0x640) = 0;
			*reinterpret_cast<void**>(candidateInput->scene + 0x640) = 0;
			}
		else
			{
			*reinterpret_cast<void**>(oracleInput->scene + 0x640) = oracleInput->container;
			*reinterpret_cast<void**>(candidateInput->scene + 0x640) = candidateInput->container;
			}
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		oracle(oracleBuilder.bytes, oracleInput->scene);
		__asm mov oracleAfter, esp
		__asm mov candidateBefore, esp
		candidate(candidateBuilder.bytes, candidateInput->scene);
		__asm mov candidateAfter, esp
		const bool oracleStack = oracleBefore == oracleAfter;
		const bool candidateStack = candidateBefore == candidateAfter;
		oracleStackBalanced &= oracleStack;
		candidateStackBalanced &= candidateStack;
		if(!oracleStack || !candidateStack)
			{
			++mismatches;
			printf("fluid collision scene-shape-collector stack detail case=%u "
				"balanced=%u/%u\n", test, oracleStack, candidateStack);
			}
		if(test == 0)
			{
			for(unsigned array = 0; array != 9; ++array)
				{
				ArrayDescriptor* const oracleDescriptor = reinterpret_cast<ArrayDescriptor*>(
					oracleBuilder.bytes + descriptorOffsets[array]);
				ArrayDescriptor* const candidateDescriptor = reinterpret_cast<ArrayDescriptor*>(
					candidateBuilder.bytes + descriptorOffsets[array]);
				if(oracleDescriptor->count != 0 || candidateDescriptor->count != 0
					|| memcmp(oracleBuilder.outputs[array], candidateBuilder.outputs[array],
						sizeof(oracleBuilder.outputs[array])) != 0)
					{
					++mismatches;
					printf("fluid collision scene-shape-collector empty detail array=%u "
						"count=%u/%u\n", array, oracleDescriptor->count,
						candidateDescriptor->count);
					}
				}
			}
		}
	static const unsigned expectedCounts[9] = { 1, 1, 2, 4, 1, 0, 0, 0, 0 };
	static const unsigned definedPrefixes[9] = { 92, 16, 88, 80, 84, 0, 0, 0, 0 };
	for(unsigned array = 0; array != 9; ++array)
		{
		ArrayDescriptor* const oracleDescriptor = reinterpret_cast<ArrayDescriptor*>(
			oracleBuilder.bytes + descriptorOffsets[array]);
		ArrayDescriptor* const candidateDescriptor = reinterpret_cast<ArrayDescriptor*>(
			candidateBuilder.bytes + descriptorOffsets[array]);
		if(oracleDescriptor->count != expectedCounts[array]
			|| candidateDescriptor->count != expectedCounts[array])
			{
			oracleContract = 0;
			++mismatches;
			printf("fluid collision scene-shape-collector count detail array=%u "
				"expected=%u actual=%u/%u\n", array, expectedCounts[array],
				oracleDescriptor->count, candidateDescriptor->count);
			continue;
			}
		const unsigned stride = array == 3 ? 84u : array == 4 ? 88u
			: array == 2 ? 92u : 96u;
		for(unsigned record = 0; record != expectedCounts[array]; ++record)
			{
			const unsigned prefix = definedPrefixes[array];
			const unsigned* const oracleRecord = reinterpret_cast<const unsigned*>(
				oracleBuilder.outputs[array] + record * stride);
			const unsigned* const candidateRecord = reinterpret_cast<const unsigned*>(
				candidateBuilder.outputs[array] + record * stride);
			if(prefix && memcmp(oracleRecord, candidateRecord, prefix) != 0)
				{
				++mismatches;
				unsigned firstDifferent = 0;
				while(firstDifferent < prefix
					&& oracleBuilder.outputs[array][record * stride + firstDifferent]
						== candidateBuilder.outputs[array][record * stride + firstDifferent])
					++firstDifferent;
				printf("fluid collision scene-shape-collector record detail array=%u "
					"record=%u prefix=%u oracle=%08x candidate=%08x diff=%u "
					"bytes=%02x/%02x\n", array,
					record, prefix,
					*reinterpret_cast<unsigned*>(oracleBuilder.outputs[array] + record * stride),
					*reinterpret_cast<unsigned*>(candidateBuilder.outputs[array] + record * stride),
					firstDifferent,
					oracleBuilder.outputs[array][record * stride + firstDifferent],
					candidateBuilder.outputs[array][record * stride + firstDifferent]);
				}
			}
		const unsigned used = expectedCounts[array] * stride;
		if(memcmp(oracleBuilder.outputs[array] + used,
			candidateBuilder.outputs[array] + used,
			sizeof(oracleBuilder.outputs[array]) - used) != 0)
			{
			++mismatches;
			printf("fluid collision scene-shape-collector tail detail array=%u used=%u "
				"oracle=%02x candidate=%02x\n", array, used,
				oracleBuilder.outputs[array][used], candidateBuilder.outputs[array][used]);
			}
		}
	const unsigned expectedPlane[4] = { 0, 0x13579bdfu, 0x2468ace0u, 0x0badf00du };
	const unsigned expectedSphereGeometry[4] = {
		0x61000000u, 0x61000800u, 0x61000300u, 0x61000700u };
	const unsigned expectedBoxGeometry[2][3] = {
		{ 0x62000100u, 0x63000100u, 0x64000100u },
		{ 0x62000700u, 0x63000700u, 0x64000700u } };
	const unsigned expectedCapsule[2] = { 0x61000a00u, 0x62000a00u };
	const unsigned expectedMesh[4] = { 0x51000b1bu, 0x51000b2du,
		0x51000b12u, 0x51000b24u };
	bool expectedGeometry =
		memcmp(oracleBuilder.outputs[1], expectedPlane, sizeof(expectedPlane)) == 0
		&& *reinterpret_cast<unsigned*>(oracleBuilder.outputs[3]) == expectedSphereGeometry[0]
		&& *reinterpret_cast<unsigned*>(oracleBuilder.outputs[3] + 84) == expectedSphereGeometry[1]
		&& *reinterpret_cast<unsigned*>(oracleBuilder.outputs[3] + 168) == expectedSphereGeometry[2]
		&& *reinterpret_cast<unsigned*>(oracleBuilder.outputs[3] + 252) == expectedSphereGeometry[3]
		&& memcmp(oracleBuilder.outputs[2], expectedBoxGeometry[0], 3 * sizeof(unsigned)) == 0
		&& memcmp(oracleBuilder.outputs[2] + 92, expectedBoxGeometry[1], 3 * sizeof(unsigned)) == 0
		&& memcmp(oracleBuilder.outputs[4], expectedCapsule,
			sizeof(expectedCapsule)) == 0
		&& memcmp(oracleBuilder.outputs[0], expectedMesh, sizeof(expectedMesh)) == 0;
	oracleContract &= expectedGeometry
		&& oracleStackBalanced
		&& gFluidCollisionPlaneAppendVirtualCalls - planeCallsBefore == 2;
	if(!expectedGeometry || !oracleStackBalanced
		|| gFluidCollisionPlaneAppendVirtualCalls - planeCallsBefore != 2)
		{
		++mismatches;
		printf("fluid collision scene-shape-collector oracle detail geometry=%u "
			"plane_calls=%u counts=", expectedGeometry,
			gFluidCollisionPlaneAppendVirtualCalls - planeCallsBefore);
		for(unsigned array = 0; array != 5; ++array)
			printf("%u%s", reinterpret_cast<ArrayDescriptor*>(oracleBuilder.bytes
				+ descriptorOffsets[array])->count, array == 4 ? "" : ",");
	printf(" plane=%08x.%08x.%08x.%08x box=%08x.%08x.%08x.%08x "
		"capsule=%08x.%08x sphere_words=%08x.%08x.%08x.%08x "
		"mesh_words=%08x.%08x.%08x.%08x\n",
		*reinterpret_cast<unsigned*>(oracleBuilder.outputs[1]),
		*reinterpret_cast<unsigned*>(oracleBuilder.outputs[1] + 4),
		*reinterpret_cast<unsigned*>(oracleBuilder.outputs[1] + 8),
		*reinterpret_cast<unsigned*>(oracleBuilder.outputs[1] + 12),
		*reinterpret_cast<unsigned*>(oracleBuilder.outputs[2]),
		*reinterpret_cast<unsigned*>(oracleBuilder.outputs[2] + 4),
		*reinterpret_cast<unsigned*>(oracleBuilder.outputs[2] + 8),
		*reinterpret_cast<unsigned*>(oracleBuilder.outputs[2] + 12),
		*reinterpret_cast<unsigned*>(oracleBuilder.outputs[4]),
		*reinterpret_cast<unsigned*>(oracleBuilder.outputs[4] + 4),
			*reinterpret_cast<unsigned*>(oracleBuilder.outputs[3]),
			*reinterpret_cast<unsigned*>(oracleBuilder.outputs[3] + 84),
			*reinterpret_cast<unsigned*>(oracleBuilder.outputs[3] + 168),
			*reinterpret_cast<unsigned*>(oracleBuilder.outputs[3] + 252),
			*reinterpret_cast<unsigned*>(oracleBuilder.outputs[0]),
			*reinterpret_cast<unsigned*>(oracleBuilder.outputs[0] + 4),
			*reinterpret_cast<unsigned*>(oracleBuilder.outputs[0] + 8),
			*reinterpret_cast<unsigned*>(oracleBuilder.outputs[0] + 12));
		}
	printf("fluid collision scene-shape-collector cases=2 empty=1 populated=1 "
		"compound_children=6 routed_shapes=12 candidate_present=%u "
		"oracle_contract=%u stack=%u/%u mismatches=%u\n", candidatePresent,
		oracleContract, oracleStackBalanced, candidateStackBalanced, mismatches);
	return mismatches;
	}

static unsigned nxRunFluidCollisionBoxAppendDifferential(
	const unsigned char* oracleBase, void* candidateAddress)
	{
	typedef void* (__cdecl* AppendBox)(void*, void*);
	AppendBox oracle = oracleBase ? reinterpret_cast<AppendBox>(
		const_cast<unsigned char*>(oracleBase) + 0x87810) : 0;
	AppendBox candidate = reinterpret_cast<AppendBox>(candidateAddress);
	if(!oracleBase || !candidateAddress)
		{
		printf("fluid collision box-array append cases=2 selected=1 ignored=1 "
			"candidate_present=%u oracle_contract=0 stack=0/0 mismatches=1\n",
			candidateAddress != 0);
		return 1;
		}
	struct ShapeFixture
		{
		unsigned char bytes[0x1c0];
		unsigned char data[0x20];
		unsigned bounds[6];
		};
	struct ArrayDescriptor
		{
		unsigned char* data;
		unsigned count;
		unsigned capacity;
		};
	unsigned mismatches = 0, oracleContract = 1;
	unsigned oracleStackBalanced = 1, candidateStackBalanced = 1;
	for(unsigned test = 0; test != 2; ++test)
		{
		ShapeFixture oracleShape, candidateShape;
		memset(&oracleShape, 0, sizeof(oracleShape));
		memset(&candidateShape, 0, sizeof(candidateShape));
		for(unsigned word = 0; word != 12; ++word)
			{
			*reinterpret_cast<unsigned*>(oracleShape.bytes + 0x0c + word * 4) =
				0x3f100007u + word * 0x01030507u;
			*reinterpret_cast<unsigned*>(candidateShape.bytes + 0x0c + word * 4) =
				0x3f100007u + word * 0x01030507u;
			}
		for(unsigned word = 0; word != 6; ++word)
			{
			oracleShape.bounds[word] = 0x43000000u + word * 5;
			candidateShape.bounds[word] = 0x43000000u + word * 5;
			}
		const unsigned type = test == 0 ? 2u : 1u;
		for(unsigned side = 0; side != 2; ++side)
			{
			ShapeFixture& shape = side == 0 ? oracleShape : candidateShape;
			*reinterpret_cast<unsigned*>(shape.bytes + 4) = 0x12340002u;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xd0) = type;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xe0) = 0x55667788u;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xe4) = 0x11223344u;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xe8) = 0x99aabbccu;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xec) = 0xddeeff00u;
			*reinterpret_cast<unsigned short*>(shape.bytes + 0xcc) = 0;
			*reinterpret_cast<unsigned**>(shape.bytes + 0xc4) =
				reinterpret_cast<unsigned*>(shape.data);
			*reinterpret_cast<unsigned**>(shape.data + 0x14) = shape.bounds;
			*reinterpret_cast<unsigned*>(shape.bytes + 0xac) = 2;
			}
		unsigned char oracleOutput[184], candidateOutput[184];
		memset(oracleOutput, 0xa5, sizeof(oracleOutput));
		memset(candidateOutput, 0xa5, sizeof(candidateOutput));
		ArrayDescriptor oracleArray = { oracleOutput, 0, 2 };
		ArrayDescriptor candidateArray = { candidateOutput, 0, 2 };
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		void* const oracleReturn = oracle(&oracleArray, oracleShape.bytes);
		__asm mov oracleAfter, esp
		__asm mov candidateBefore, esp
		void* const candidateReturn = candidate(&candidateArray, candidateShape.bytes);
		__asm mov candidateAfter, esp
		const bool selected = test == 0;
		const unsigned expectedCount = selected ? 1u : 0u;
		const void* const expectedOracleReturn = selected
			? static_cast<void*>(&oracleArray) : static_cast<void*>(oracleShape.bytes);
		const void* const expectedCandidateReturn = selected
			? static_cast<void*>(&candidateArray) : static_cast<void*>(candidateShape.bytes);
		const bool oracleReturnValid = oracleReturn == expectedOracleReturn;
		const bool candidateReturnValid = candidateReturn == expectedCandidateReturn;
		const bool countsMatch = oracleArray.count == expectedCount
			&& candidateArray.count == expectedCount;
		bool recordMatches = true, tailsMatch = true;
		if(selected)
			{
				recordMatches = memcmp(oracleOutput, candidateOutput, 88) == 0
					&& *reinterpret_cast<unsigned*>(oracleOutput) == 0x11223344u
					&& *reinterpret_cast<unsigned*>(oracleOutput + 4) == 0x99aabbccu
					&& *reinterpret_cast<unsigned*>(oracleOutput + 8) == 0xddeeff00u;
			tailsMatch = memcmp(oracleOutput + 92, candidateOutput + 92,
				sizeof(oracleOutput) - 92) == 0;
			}
		else
			recordMatches = memcmp(oracleOutput, candidateOutput,
				sizeof(oracleOutput)) == 0;
		const bool oracleStack = oracleBefore == oracleAfter;
		const bool candidateStack = candidateBefore == candidateAfter;
		oracleStackBalanced = oracleStackBalanced && oracleStack;
		candidateStackBalanced = candidateStackBalanced && candidateStack;
		const bool matched = oracleReturnValid && candidateReturnValid
			&& countsMatch && recordMatches && tailsMatch
			&& oracleStack && candidateStack;
		if(!matched)
			{
			printf("fluid collision box-array append detail selected=%u "
				"returns=%u/%u counts=%u/%u record=%u tails=%u "
				"stack=%u/%u oracle_words=%08x.%08x.%08x "
				"candidate_words=%08x.%08x.%08x\n", selected,
				oracleReturnValid, candidateReturnValid,
				oracleArray.count == expectedCount,
				candidateArray.count == expectedCount, recordMatches, tailsMatch,
				oracleStack, candidateStack,
				*reinterpret_cast<unsigned*>(oracleOutput),
				*reinterpret_cast<unsigned*>(oracleOutput + 4),
				*reinterpret_cast<unsigned*>(oracleOutput + 8),
				*reinterpret_cast<unsigned*>(candidateOutput),
				*reinterpret_cast<unsigned*>(candidateOutput + 4),
				*reinterpret_cast<unsigned*>(candidateOutput + 8));
			}
		oracleContract &= oracleReturnValid && countsMatch && recordMatches
			&& oracleStack;
		if(!matched)
			++mismatches;
		}
	printf("fluid collision box-array append cases=2 selected=1 ignored=1 "
		"candidate_present=1 oracle_contract=%u stack=%u/%u mismatches=%u\n",
		oracleContract, oracleStackBalanced, candidateStackBalanced, mismatches);
	return mismatches;
	}

static unsigned nxRunFluidCollisionIndexedMeshDifferential(
	const unsigned char* oracleBase, void* candidateAddress)
	{
	if(!oracleBase || !candidateAddress)
		return 1;
	typedef int (__cdecl* BuildIndexedGrid)(NxFluidCollisionBucketHashDescriptor*,
		int*, const NxFluidCollisionTriangleMesh*,
		float, float, NxFluidCollisionInt16Vector3Array*);
	BuildIndexedGrid oracle = reinterpret_cast<BuildIndexedGrid>(
		const_cast<unsigned char*>(oracleBase) + 0x86f00);
	BuildIndexedGrid candidate = reinterpret_cast<BuildIndexedGrid>(candidateAddress);
	const unsigned indices[6] = { 0, 1, 2, 3, 4, 5 };
	const float vertices[18] = {
		0.125f, 0.25f, 0.375f, 1.125f, 0.25f, 0.375f,
		0.125f, 1.25f, 0.375f,
		-0.75f, 0.5f, 0.25f, 0.25f, 0.5f, 0.25f,
		-0.75f, 1.25f, 0.25f
		};
	const float transform[12] = {
		0.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 1.0f, -0.25f, 1.5f, -0.125f };
	unsigned mismatches = 0;
	for(unsigned test = 0; test != 3; ++test)
		{
		NxFluidCollisionTriangleMesh mesh = {};
		mesh.triangleCount = test == 0 ? 2 : 0;
		mesh.indices = indices;
		mesh.vertices = vertices;
		memcpy(mesh.transform, transform, sizeof(transform));
		NxFluidCollisionGridCell oracleRecords[9];
		NxFluidCollisionGridCell candidateRecords[9];
		memset(oracleRecords, 0x5a, sizeof(oracleRecords));
		memcpy(candidateRecords, oracleRecords, sizeof(oracleRecords));
		int oracleRecordBase = reinterpret_cast<int>(oracleRecords);
		int candidateRecordBase = reinterpret_cast<int>(candidateRecords);
		unsigned oracleBuckets[3][3] = {
			{ 0, 0, 0xabcdef00u }, { 0, 3, 0xabcdef01u },
			{ 0, 6, 0xabcdef02u }
			};
		unsigned candidateBuckets[3][3];
		memcpy(candidateBuckets, oracleBuckets, sizeof(oracleBuckets));
		NxFluidCollisionBucketHashDescriptor oracleHash = {
			&oracleBuckets[0][0], 3 };
		NxFluidCollisionBucketHashDescriptor candidateHash = {
			&candidateBuckets[0][0], 3 };
		short oracleCellStorage[512 * 3];
		short candidateCellStorage[512 * 3];
		memset(oracleCellStorage, 0x5a, sizeof(oracleCellStorage));
		memcpy(candidateCellStorage, oracleCellStorage, sizeof(oracleCellStorage));
		NxFluidCollisionInt16Vector3Array oracleCells = {
			oracleCellStorage, test == 1 ? 77 : 0, 512 };
		NxFluidCollisionInt16Vector3Array candidateCells = {
			candidateCellStorage, test == 1 ? 77 : 0, 512 };
		NxFluidCollisionInt16Vector3Array* oracleOutput = test == 2 ? 0 : &oracleCells;
		NxFluidCollisionInt16Vector3Array* candidateOutput = test == 2 ? 0 : &candidateCells;
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		const int oracleReturn = oracle(&oracleHash, &oracleRecordBase, &mesh,
			0.0f, 1.0f, oracleOutput);
		__asm mov oracleAfter, esp
		__asm mov candidateBefore, esp
		const int candidateReturn = candidate(&candidateHash, &candidateRecordBase,
			&mesh, 0.0f, 1.0f, candidateOutput);
		__asm mov candidateAfter, esp
		const bool cellsMatch = oracleCells.count == candidateCells.count
			&& oracleCells.capacity == candidateCells.capacity
			&& memcmp(oracleCellStorage,
			candidateCellStorage, sizeof(oracleCellStorage)) == 0;
		const bool bucketsMatch = memcmp(oracleBuckets, candidateBuckets,
			sizeof(oracleBuckets)) == 0;
		const bool recordsMatch = memcmp(oracleRecords, candidateRecords,
			sizeof(oracleRecords)) == 0;
		const bool stackBalanced = oracleBefore == oracleAfter
			&& candidateBefore == candidateAfter;
		const int expectedReturn = test == 2 ? 0 : 1;
		const bool matched = oracleReturn == expectedReturn
			&& candidateReturn == expectedReturn && cellsMatch && bucketsMatch
			&& recordsMatch && stackBalanced;
		printf("fluid collision indexed-mesh-build case=%u cells=%d/%d "
			"buckets=%u records=%u return=%d/%d stack=%u/%u mismatches=%u\n",
			test, oracleCells.count, candidateCells.count, bucketsMatch,
			recordsMatch, oracleReturn, candidateReturn,
			oracleBefore == oracleAfter, candidateBefore == candidateAfter,
			matched ? 0u : 1u);
		if(!matched)
			++mismatches;
		}
	printf("fluid collision indexed-mesh-build cases=3 nonempty=1 empty=1 "
		"null_output=1 candidate_present=1 mismatches=%u\n", mismatches);
	return mismatches;
	}

struct NxFluidCollisionCopyDescriptor
	{
	void* buckets;
	unsigned bucketCapacity;
	unsigned bucketCount;
	void* cells;
	unsigned cellCount;
	unsigned cellCapacity;
	void* indices;
	unsigned indexCount;
	unsigned indexCapacity;
	};

static void __cdecl nxOracleBucketAttributesArrayCopy(void* destination,
	const NxFluidCollisionCopyDescriptor* source)
	{
	NxFluidCollisionCopyDescriptor* output =
		static_cast<NxFluidCollisionCopyDescriptor*>(destination);
	output->buckets = malloc(source->bucketCount * 12);
	output->bucketCapacity = source->bucketCapacity;
	output->bucketCount = source->bucketCount;
	if(source->bucketCount)
		memcpy(output->buckets, source->buckets, source->bucketCount * 12);
	}

static void __cdecl nxOracleCellAttributesArrayCreate(void* destination,
	unsigned count)
	{
	unsigned* words = static_cast<unsigned*>(destination);
	words[0] = reinterpret_cast<unsigned>(malloc(count * 16));
	words[1] = 0;
	words[2] = count;
	}

static void __cdecl nxOracleIndexArrayCreate(void* destination, unsigned count)
	{
	unsigned* words = static_cast<unsigned*>(destination);
	words[0] = reinterpret_cast<unsigned>(malloc(count * 8));
	words[1] = 0;
	words[2] = count;
	}

static unsigned nxRunFluidCollisionStaticCopyDifferential(const unsigned char* oracleBase,
	void* candidateAddress)
	{
	if(!oracleBase || !candidateAddress)
		return 1;
	static_assert(sizeof(NxFluidCollisionCopyDescriptor) == 36,
		"phys_fn_003518 copies the three pointer/count array headers");
	static_assert(sizeof(NxFluidCollisionCellBucket) == 12,
		"phys_fn_003518 advances bucket rows by twelve bytes");
	static_assert(sizeof(NxFluidCollisionCellAttribute) == 16,
		"phys_fn_003518 copies sixteen-byte cell attributes");
	unsigned char* const image = const_cast<unsigned char*>(oracleBase);
	void* savedBucketCopy = 0;
	void* savedCellCreate = 0;
	void* savedIndexCreate = 0;
	void* oracleBucketCopy = reinterpret_cast<void*>(&nxOracleBucketAttributesArrayCopy);
	void* oracleCellCreate = reinterpret_cast<void*>(&nxOracleCellAttributesArrayCreate);
	void* oracleIndexCreate = reinterpret_cast<void*>(&nxOracleIndexArrayCreate);
	memcpy(&savedBucketCopy, image + 0x126510, 4);
	memcpy(&savedCellCreate, image + 0x126434, 4);
	memcpy(&savedIndexCreate, image + 0x12645c, 4);
	memcpy(image + 0x126510, &oracleBucketCopy, 4);
	memcpy(image + 0x126434, &oracleCellCreate, 4);
	memcpy(image + 0x12645c, &oracleIndexCreate, 4);
	typedef unsigned (__cdecl* StaticCopy)(NxFluidCollisionCopyDescriptor*,
		const NxFluidCollisionCopyDescriptor*);
	StaticCopy oracle = reinterpret_cast<StaticCopy>(image + 0x86590);
	StaticCopy candidate = reinterpret_cast<StaticCopy>(candidateAddress);
	NxFluidCollisionCellBucket sourceBuckets[3] = {
		{ 2, 1, 0xaaaa0001 }, { 1, 0, 0xaaaa0002 }, { 0, 2, 0xaaaa0003 } };
	NxFluidCollisionCellAttribute sourceCells[3] = {
		{ { 0x10, 0x20, 0x30, 0xcccc0001 } },
		{ { 0x11, 0x21, 0x31, 0xcccc0002 } },
		{ { 0x12, 0x22, 0x32, 0xcccc0003 } } };
	unsigned sourceIndices[8] = {
		0x10000001, 0x20000001, 0x10000002, 0x20000002,
		0x10000003, 0x20000003, 0x10000004, 0x20000004 };
	NxFluidCollisionCopyDescriptor source = {
		sourceBuckets, 3, 3, sourceCells, 3, 3, sourceIndices, 4, 4 };
	NxFluidCollisionCopyDescriptor oracleOutput;
	NxFluidCollisionCopyDescriptor candidateOutput;
	memset(&oracleOutput, 0xcd, sizeof(oracleOutput));
	memset(&candidateOutput, 0xcd, sizeof(candidateOutput));
	unsigned oracleBefore = 0, oracleAfter = 0, candidateBefore = 0, candidateAfter = 0;
	__asm mov oracleBefore, esp
	const unsigned oracleReturn = oracle(&oracleOutput, &source);
	__asm mov oracleAfter, esp
	__asm mov candidateBefore, esp
	const unsigned candidateReturn = candidate(&candidateOutput, &source);
	__asm mov candidateAfter, esp
	bool rowsMatch = oracleOutput.bucketCount == 3 && candidateOutput.bucketCount == 3;
	const unsigned expectedBucketWords[3][2] = { { 2, 0 }, { 1, 2 }, { 0, 3 } };
	for(unsigned i = 0; i != 3; ++i)
		{
		const unsigned* oracleRow = reinterpret_cast<const unsigned*>(oracleOutput.buckets) + i * 3;
		const unsigned* candidateRow = reinterpret_cast<const unsigned*>(candidateOutput.buckets) + i * 3;
		rowsMatch = rowsMatch && oracleRow[0] == expectedBucketWords[i][0]
			&& candidateRow[0] == expectedBucketWords[i][0]
			&& oracleRow[1] == expectedBucketWords[i][1]
			&& candidateRow[1] == expectedBucketWords[i][1];
		}
	const bool cellsMatch = oracleOutput.cellCount == 3 && candidateOutput.cellCount == 3
			&& memcmp(oracleOutput.cells, candidateOutput.cells, 3 * 16) == 0
			&& memcmp(oracleOutput.cells, sourceCells + 1, 2 * 16) == 0
			&& memcmp(static_cast<unsigned char*>(oracleOutput.cells) + 32,
				sourceCells, 16) == 0;
	const bool indicesMatch = oracleOutput.indexCount == 4 && candidateOutput.indexCount == 4
		&& memcmp(oracleOutput.indices, candidateOutput.indices, sizeof(sourceIndices)) == 0
		&& memcmp(oracleOutput.indices, sourceIndices, sizeof(sourceIndices)) == 0;
	const bool headersMatch = oracleOutput.bucketCapacity == candidateOutput.bucketCapacity
		&& oracleOutput.cellCapacity == candidateOutput.cellCapacity
		&& oracleOutput.indexCapacity == candidateOutput.indexCapacity;
	const bool sourcePreserved = memcmp(sourceBuckets, source.buckets,
		sizeof(sourceBuckets)) == 0
		&& memcmp(sourceCells, source.cells, sizeof(sourceCells)) == 0
		&& memcmp(sourceIndices, source.indices, sizeof(sourceIndices)) == 0;
	const bool stackMatch = oracleBefore == oracleAfter && candidateBefore == candidateAfter;
	const bool matched = rowsMatch && cellsMatch && indicesMatch && headersMatch
		&& sourcePreserved
		&& oracleReturn == 4 && candidateReturn == 4 && stackMatch;
	printf("fluid collision static-copy buckets=3 cells=3 indices=4 oracle_return=%u "
		"candidate_return=%u rows=%u cells_match=%u indices_match=%u headers=%u "
		"source_preserved=%u stack=%u/%u mismatches=%u undefined_bucket_word=ignored\n",
		oracleReturn, candidateReturn, rowsMatch, cellsMatch, indicesMatch,
		headersMatch, sourcePreserved, oracleBefore == oracleAfter,
		candidateBefore == candidateAfter,
		matched ? 0u : 1u);
	free(oracleOutput.buckets);
	free(oracleOutput.cells);
	free(oracleOutput.indices);
	memcpy(image + 0x126510, &savedBucketCopy, 4);
	memcpy(image + 0x126434, &savedCellCreate, 4);
	memcpy(image + 0x12645c, &savedIndexCreate, 4);
	return matched ? 0 : 1;
	}

typedef int (__cdecl* NxFluidCollisionStaticRebuildFn)(void*, void*, float, float);

struct NxFluidCollisionBuilderArrayView
	{
	void* data;
	unsigned count;
	unsigned capacity;
	};

static int nxCallFluidCollisionStaticRebuild(
	NxFluidCollisionStaticRebuildFn function, void* state, void* scene,
	float restDistance, float kernelMultiplier)
	{
	__try { return function(state, scene, restDistance, kernelMultiplier); }
	__except(EXCEPTION_EXECUTE_HANDLER) { return static_cast<int>(0x80000000u); }
	}

typedef int (__thiscall* NxFluidManagerStepFn)(void*, float);

static int nxFluidManagerExceptionFilter(EXCEPTION_POINTERS* exception,
	unsigned* code, unsigned* address)
	{
	*code = exception->ExceptionRecord->ExceptionCode;
	*address = reinterpret_cast<unsigned>(exception->ExceptionRecord->ExceptionAddress);
	return EXCEPTION_EXECUTE_HANDLER;
	}

static int nxCallFluidManagerStep(NxFluidManagerStepFn function, void* manager,
	float timestep, unsigned* exceptionCode, unsigned* exceptionAddress)
	{
	*exceptionCode = 0;
	*exceptionAddress = 0;
	__try { return function(manager, timestep); }
	__except(nxFluidManagerExceptionFilter(GetExceptionInformation(),
		exceptionCode, exceptionAddress))
		{ return static_cast<int>(0x80000000u); }
	}

static unsigned nxRunFluidCollisionStaticRebuildDifferential(
	const unsigned char* oracleBase, void* candidateAddress,
	void* candidateManagerStepAddress, HMODULE fluidModel)
	{
	if(!oracleBase || !candidateAddress || !fluidModel)
		return 1;
	static const char* const names[16] = {
		"StaticCollisionCreate", "CDMeshArrayGrow", "CDPlaneArrayGrow",
		"CDSphereArrayGrow", "CDCapsuleArrayGrow", "CDBoxArrayGrow",
		"CDBucketAttributesArrayCreate", "CDBucketAttributesArrayInit",
		"CDCellAttributesArrayCreate", "CDIndexArrayCreate",
		"Int16Vector3ArrayCreate", "CDHashDestroy",
		"Int16Vector3ArrayDestroy", "CDBucketAttributesArrayCopy",
		"Int16Vector3ArrayGrow", "triBoxOverlap" };
	static const unsigned slots[16] = {
		0x1265d0, 0x126420, 0x1265c8, 0x126460, 0x1265f4, 0x1264e8,
		0x1264bc, 0x126560, 0x126434, 0x12645c, 0x126534, 0x1265fc,
		0x126504, 0x126510, 0x126454, 0x126524 };
	static const unsigned findCellSlot = 0x12658c;
	void* saved[17] = {};
	void* const page = reinterpret_cast<void*>(
		reinterpret_cast<size_t>(const_cast<unsigned char*>(oracleBase) + 0x126000)
		& ~static_cast<size_t>(0xfff));
	DWORD oldProtection = 0;
	if(!VirtualProtect(page, 0x1000, PAGE_READWRITE, &oldProtection))
		return 1;
	FARPROC callbacks[16] = {};
	for(unsigned i = 0; i != 16; ++i)
		{
		callbacks[i] = GetProcAddress(fluidModel, names[i]);
		if(!callbacks[i])
			{
			DWORD ignored = 0;
			VirtualProtect(page, 0x1000, oldProtection, &ignored);
			fprintf(stderr, "FAIL static-collision fixture lacks %s\n", names[i]);
			return 1;
			}
		void** const slot = reinterpret_cast<void**>(
			const_cast<unsigned char*>(oracleBase) + slots[i]);
		}
	for(unsigned i = 0; i != 16; ++i)
		{
		void** const slot = reinterpret_cast<void**>(
			const_cast<unsigned char*>(oracleBase) + slots[i]);
		saved[i] = *slot;
		*slot = reinterpret_cast<void*>(callbacks[i]);
		}
	void** const findCell = reinterpret_cast<void**>(
		const_cast<unsigned char*>(oracleBase) + findCellSlot);
	saved[16] = *findCell;
	*findCell = reinterpret_cast<void*>(&nxOracleFluidFindCell);

	struct ShapeFixture
		{
		unsigned char bytes[0x1c0];
		unsigned char shapeData[0x20];
		unsigned bounds[6];
		unsigned char meshData[0x40];
		unsigned indices[3];
		float vertices[9];
		unsigned char owner[0x20];
		} shape = {};
	struct Node { unsigned pad; void* shape; } node = {};
	unsigned char scene[0x700] = {};
	unsigned char container[32] = {};
	void* entries[2] = {};
	static const unsigned identity[12] = {
		0x3f800000u, 0, 0, 0, 0x3f800000u, 0,
		0, 0, 0x3f800000u, 0, 0, 0 };
	memcpy(shape.bytes + 0x0c, identity, sizeof(identity));
	shape.bounds[0] = 0xbf000000u; shape.bounds[1] = 0xbf000000u;
	shape.bounds[2] = 0xbf000000u; shape.bounds[3] = 0x3f000000u;
	shape.bounds[4] = 0x3f000000u; shape.bounds[5] = 0x3f000000u;
	shape.indices[0] = 0; shape.indices[1] = 1; shape.indices[2] = 2;
	shape.vertices[0] = 0.125f; shape.vertices[1] = 0.25f; shape.vertices[2] = 0.375f;
	shape.vertices[3] = 0.875f; shape.vertices[4] = 0.25f; shape.vertices[5] = 0.375f;
	shape.vertices[6] = 0.125f; shape.vertices[7] = 0.875f; shape.vertices[8] = 0.375f;
	*reinterpret_cast<void**>(shape.bytes + 4) = shape.owner;
	*reinterpret_cast<unsigned short*>(shape.bytes + 0xcc) = 0;
	*reinterpret_cast<unsigned*>(shape.bytes + 0xd0) = 4;
	*reinterpret_cast<unsigned short*>(shape.bytes + 0xde) = 0;
	*reinterpret_cast<void**>(shape.bytes + 0xc4) = shape.shapeData;
	*reinterpret_cast<void**>(shape.shapeData + 0x14) = shape.bounds;
	*reinterpret_cast<unsigned*>(shape.bytes + 0xac) = 2;
	*reinterpret_cast<unsigned*>(shape.bytes + 0x1bc) = 0x100;
	*reinterpret_cast<void**>(shape.bytes + 0xe0) = shape.meshData;
	*reinterpret_cast<unsigned*>(shape.meshData + 12) = 1;
	*reinterpret_cast<unsigned*>(shape.meshData + 20) = reinterpret_cast<unsigned>(shape.indices);
	*reinterpret_cast<unsigned*>(shape.meshData + 8) = 0x12345678u;
	*reinterpret_cast<unsigned*>(shape.meshData + 16) = reinterpret_cast<unsigned>(shape.vertices);
	node.shape = shape.bytes;
	*reinterpret_cast<void**>(scene + 0x640) = container;
	*reinterpret_cast<unsigned*>(container + 4) = 0;
	*reinterpret_cast<unsigned*>(container + 8) = 2;
	*reinterpret_cast<unsigned*>(container + 12) = 0;
	*reinterpret_cast<void***>(container + 24) = entries;
	entries[0] = &node;
	entries[1] = &node;

	NxFluidCollisionStaticRebuildFn const oracle =
		reinterpret_cast<NxFluidCollisionStaticRebuildFn>(
			const_cast<unsigned char*>(oracleBase) + 0x861b0);
	NxFluidCollisionStaticRebuildFn const candidate =
		reinterpret_cast<NxFluidCollisionStaticRebuildFn>(candidateAddress);
	static const unsigned arrayOffsets[9] =
		{ 0, 0x30, 0x3c, 0x48, 0x54, 0x6c, 0x78, 0x84, 0x90 };
	static const unsigned arrayStrides[9] = { 96, 96, 92, 84, 88, 96, 84, 88, 92 };
	unsigned mismatches = 0;
	for(unsigned test = 0; test != 2; ++test)
		{
		unsigned char oracleState[0xa0] = {};
		unsigned char candidateState[0xa0] = {};
		*reinterpret_cast<void**>(scene + 0x640) = test ? container : 0;
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		__asm mov oracleBefore, esp
		const int oracleReturn = nxCallFluidCollisionStaticRebuild(
			oracle, oracleState, scene, 0.0f, 0.125f);
		__asm mov oracleAfter, esp
		__asm mov candidateBefore, esp
		const int candidateReturn = nxCallFluidCollisionStaticRebuild(
			candidate, candidateState, scene, 0.0f, 0.125f);
		__asm mov candidateAfter, esp
		bool arraysMatch = true;
		for(unsigned i = 0; i != 9; ++i)
			{
			NxFluidCollisionBuilderArrayView* const oracleArray =
				reinterpret_cast<NxFluidCollisionBuilderArrayView*>(oracleState + arrayOffsets[i]);
			NxFluidCollisionBuilderArrayView* const candidateArray =
				reinterpret_cast<NxFluidCollisionBuilderArrayView*>(candidateState + arrayOffsets[i]);
			arraysMatch = arraysMatch && oracleArray->count == candidateArray->count;
			if(oracleArray->count == candidateArray->count && oracleArray->count)
				{
				bool rowMatch = true;
				if(i == 0)
					{
					// The final dword in each 96-byte mesh record is undefined in
					// the oracle; its semantic record ends at byte 92.
					for(unsigned row = 0; row != oracleArray->count; ++row)
						rowMatch = rowMatch && memcmp(
							static_cast<const unsigned char*>(oracleArray->data) + row * 96,
							static_cast<const unsigned char*>(candidateArray->data) + row * 96,
							92) == 0;
					}
				else
				rowMatch = memcmp(oracleArray->data, candidateArray->data,
					static_cast<size_t>(oracleArray->count) * arrayStrides[i]) == 0;
				arraysMatch = arraysMatch && rowMatch;
				if(!rowMatch)
					{
					unsigned offset = 0;
					const unsigned char* const oracleBytes =
						static_cast<const unsigned char*>(oracleArray->data);
					const unsigned char* const candidateBytes =
						static_cast<const unsigned char*>(candidateArray->data);
					while(offset != oracleArray->count * arrayStrides[i]
						&& oracleBytes[offset] == candidateBytes[offset])
						++offset;
					printf("fluid collision static-rebuild array-data-detail case=%u index=%u count=%u bytes=%u first_diff=%u oracle=%02x candidate=%02x\n",
						test, i, oracleArray->count,
						oracleArray->count * arrayStrides[i], offset,
						oracleBytes[offset], candidateBytes[offset]);
					}
				}
			if(oracleArray->count != candidateArray->count
				|| oracleArray->capacity != candidateArray->capacity)
				printf("fluid collision static-rebuild array-detail case=%u index=%u count=%u/%u capacity=%u/%u\n",
					test, i, oracleArray->count, candidateArray->count,
					oracleArray->capacity, candidateArray->capacity);
			}
	NxFluidCollisionCopyDescriptor* const oracleGrid =
		reinterpret_cast<NxFluidCollisionCopyDescriptor*>(oracleState + 12);
	NxFluidCollisionCopyDescriptor* const candidateGrid =
		reinterpret_cast<NxFluidCollisionCopyDescriptor*>(candidateState + 12);
	bool gridMatch = oracleGrid->bucketCount == candidateGrid->bucketCount
		&& oracleGrid->cellCount == candidateGrid->cellCount
		&& oracleGrid->indexCount == candidateGrid->indexCount
		&& oracleGrid->bucketCapacity == candidateGrid->bucketCapacity
		&& oracleGrid->cellCapacity == candidateGrid->cellCapacity
		&& oracleGrid->indexCapacity == candidateGrid->indexCapacity;
	if(!gridMatch)
		printf("fluid collision static-rebuild grid-detail case=%u buckets=%u/%u cap=%u/%u cells=%u/%u cap=%u/%u indices=%u/%u cap=%u/%u\n",
			test, oracleGrid->bucketCount, candidateGrid->bucketCount,
			oracleGrid->bucketCapacity, candidateGrid->bucketCapacity,
			oracleGrid->cellCount, candidateGrid->cellCount,
			oracleGrid->cellCapacity, candidateGrid->cellCapacity,
			oracleGrid->indexCount, candidateGrid->indexCount,
			oracleGrid->indexCapacity, candidateGrid->indexCapacity);
	if(gridMatch && oracleGrid->bucketCount)
		{
		const unsigned* const oracleBuckets =
			static_cast<const unsigned*>(oracleGrid->buckets);
		const unsigned* const candidateBuckets =
			static_cast<const unsigned*>(candidateGrid->buckets);
		for(unsigned i = 0; i != oracleGrid->bucketCount; ++i)
			if(oracleBuckets[i * 3] != candidateBuckets[i * 3]
				|| oracleBuckets[i * 3 + 1] != candidateBuckets[i * 3 + 1])
				{
				gridMatch = false;
				break;
				}
		}
	if(gridMatch && oracleGrid->cellCount)
		gridMatch = memcmp(oracleGrid->cells, candidateGrid->cells,
			static_cast<size_t>(oracleGrid->cellCount) * 16) == 0;
	if(gridMatch && oracleGrid->indexCount)
		gridMatch = memcmp(oracleGrid->indices, candidateGrid->indices,
			static_cast<size_t>(oracleGrid->indexCount) * 8) == 0;
	const bool parametersMatch = memcmp(oracleState + 0x64,
		candidateState + 0x64, 8) == 0;
	const bool stackMatch = oracleBefore == oracleAfter
		&& candidateBefore == candidateAfter;
	const bool matched = arraysMatch && gridMatch && parametersMatch
		&& oracleReturn == candidateReturn && stackMatch;
	printf("fluid collision static-rebuild case=%u empty=%u mesh=%u arrays=%u "
		"grid=%u parameters=%u return=%d/%d stack=%u/%u mismatches=%u\n",
		test, test == 0, test == 1, arraysMatch, gridMatch, parametersMatch,
		oracleReturn, candidateReturn, oracleBefore == oracleAfter,
		candidateBefore == candidateAfter, matched ? 0u : 1u);
	if(!matched)
		++mismatches;
		// Both sides own nested array allocations through different module
		// allocators. This short-lived harness retains these small outputs until
		// process exit rather than crossing module CRT heaps during cleanup.
	}
	FARPROC managerGetCallback = GetProcAddress(fluidModel,
		"FluidManagerGetStaticCollision");
	FARPROC managerDestroyCallback = GetProcAddress(fluidModel,
		"StaticCollisionDestroy");
	typedef void (__cdecl* ResetManagerCallbacks)();
	typedef void (__cdecl* SetManagerCollisions)(void*, void*);
	typedef unsigned (__cdecl* ManagerCallbackCount)();
	typedef int (__cdecl* GetManagerCallbackRecord)(unsigned, void*);
	ResetManagerCallbacks resetManagerCallbacks = reinterpret_cast<ResetManagerCallbacks>(
		GetProcAddress(fluidModel, "NxResetFluidManagerCallbacks"));
	SetManagerCollisions setManagerCollisions = reinterpret_cast<SetManagerCollisions>(
		GetProcAddress(fluidModel, "NxSetFluidManagerStaticCollisions"));
	ManagerCallbackCount managerCallbackCount = reinterpret_cast<ManagerCallbackCount>(
		GetProcAddress(fluidModel, "NxGetFluidManagerCallbackCount"));
	GetManagerCallbackRecord getManagerCallbackRecord =
		reinterpret_cast<GetManagerCallbackRecord>(GetProcAddress(fluidModel,
			"NxGetFluidManagerCallbackRecord"));
	if(!managerGetCallback || !managerDestroyCallback || !resetManagerCallbacks
		|| !setManagerCollisions || !managerCallbackCount || !getManagerCallbackRecord
		|| !candidateManagerStepAddress)
		{
		fprintf(stderr, "FAIL dirty fluid-manager fixture exports are incomplete\n");
		++mismatches;
		}
	else
		{
	unsigned char* const mutableOracleBase =
			const_cast<unsigned char*>(oracleBase);
	void** const managerGetSlot = reinterpret_cast<void**>(
		mutableOracleBase + 0x126424);
	void** const managerDestroySlot = reinterpret_cast<void**>(
		mutableOracleBase + 0x126494);
	void* const savedManagerGet = *managerGetSlot;
	void* const savedManagerDestroy = *managerDestroySlot;
	*managerGetSlot = reinterpret_cast<void*>(managerGetCallback);
	*managerDestroySlot = reinterpret_cast<void*>(managerDestroyCallback);
	struct ManagerCallbackRecord
		{
		unsigned kind;
		unsigned arguments[4];
		};
	unsigned char oracleScene[0x700] = {};
	unsigned char candidateScene[0x700] = {};
	unsigned char oracleContainer[32] = {};
	unsigned char candidateContainer[32] = {};
	void* oracleEntries[1] = {};
	void* candidateEntries[1] = {};
	*reinterpret_cast<void**>(oracleScene + 0x640) = oracleContainer;
	*reinterpret_cast<void**>(candidateScene + 0x640) = candidateContainer;
	*reinterpret_cast<void***>(oracleContainer + 24) = oracleEntries;
	*reinterpret_cast<void***>(candidateContainer + 24) = candidateEntries;
	unsigned emptyFluidRange[1] = {};
	unsigned char oracleManager[0x34] = {};
	unsigned char candidateManager[0x34] = {};
	*reinterpret_cast<void***>(oracleManager + 4) =
		reinterpret_cast<void**>(emptyFluidRange);
	*reinterpret_cast<void***>(oracleManager + 8) =
		reinterpret_cast<void**>(emptyFluidRange);
	*reinterpret_cast<void***>(candidateManager + 4) =
		reinterpret_cast<void**>(emptyFluidRange);
	*reinterpret_cast<void***>(candidateManager + 8) =
		reinterpret_cast<void**>(emptyFluidRange);
	*reinterpret_cast<void**>(oracleManager + 0x24) = oracleScene;
	*reinterpret_cast<void**>(candidateManager + 0x24) = candidateScene;
	oracleManager[0x28] = candidateManager[0x28] = 1;
	oracleManager[0x2b] = candidateManager[0x2b] = 1;
	const unsigned kernelWord = 0x3e800000u;
	*reinterpret_cast<unsigned*>(oracleManager + 0x2c) = kernelWord;
	*reinterpret_cast<unsigned*>(candidateManager + 0x2c) = kernelWord;
	const unsigned backendWord = 0x13572468u;
	*reinterpret_cast<unsigned*>(oracleManager + 0x30) = backendWord;
	*reinterpret_cast<unsigned*>(candidateManager + 0x30) = backendWord;
	unsigned char oracleOldCollision[0xa0];
	unsigned char oracleReplacementCollision[0xa0];
	unsigned char candidateOldCollision[0xa0];
	unsigned char candidateReplacementCollision[0xa0];
	memset(oracleOldCollision, 0xab, sizeof(oracleOldCollision));
	memset(oracleReplacementCollision, 0xcd, sizeof(oracleReplacementCollision));
	memset(candidateOldCollision, 0xab, sizeof(candidateOldCollision));
	memset(candidateReplacementCollision, 0xcd, sizeof(candidateReplacementCollision));
	const unsigned stepWord = 0x3e000000u;
	const float step = *reinterpret_cast<const float*>(&stepWord);
	NxFluidManagerStepFn const oracleStep = reinterpret_cast<NxFluidManagerStepFn>(
		mutableOracleBase + 0x89bd0);
	NxFluidManagerStepFn const candidateStep = reinterpret_cast<NxFluidManagerStepFn>(
		candidateManagerStepAddress);
	resetManagerCallbacks();
	setManagerCollisions(oracleOldCollision, oracleReplacementCollision);
	unsigned oracleBefore = 0, oracleAfter = 0;
	unsigned oracleException = 0;
	unsigned oracleExceptionAddress = 0;
	__asm mov oracleBefore, esp
	const int oracleManagerResult = nxCallFluidManagerStep(oracleStep, oracleManager,
		step, &oracleException, &oracleExceptionAddress);
	__asm mov oracleAfter, esp
	if(!oracleException) __asm fstp st(0)
	ManagerCallbackRecord oracleRecords[3] = {};
	const unsigned oracleRecordCount = managerCallbackCount();
	for(unsigned i = 0; i != 3; ++i)
		if(!getManagerCallbackRecord(i, &oracleRecords[i]))
			++mismatches;
	if(oracleException)
		for(unsigned i = 0; i != oracleRecordCount && i != 3; ++i)
			printf("fluid manager oracle callback[%u]=%u args=%08x.%08x.%08x.%08x\n",
				i, oracleRecords[i].kind, oracleRecords[i].arguments[0],
				oracleRecords[i].arguments[1], oracleRecords[i].arguments[2],
				oracleRecords[i].arguments[3]);
	resetManagerCallbacks();
	setManagerCollisions(candidateOldCollision, candidateReplacementCollision);
	unsigned candidateBefore = 0, candidateAfter = 0;
	unsigned candidateException = 0;
	unsigned candidateExceptionAddress = 0;
	__asm mov candidateBefore, esp
	const int candidateManagerResult = nxCallFluidManagerStep(candidateStep,
		candidateManager, step, &candidateException, &candidateExceptionAddress);
	__asm mov candidateAfter, esp
	if(!candidateException) __asm fstp st(0)
	ManagerCallbackRecord candidateRecords[3] = {};
	const unsigned candidateRecordCount = managerCallbackCount();
	for(unsigned i = 0; i != 3; ++i)
		if(!getManagerCallbackRecord(i, &candidateRecords[i]))
			++mismatches;
	unsigned callbackMismatches = oracleRecordCount != 3
		|| candidateRecordCount != 3;
	for(unsigned i = 0; i != 3; ++i)
		{
		if(i == 0)
			{
			if(memcmp(&oracleRecords[i], &candidateRecords[i],
				sizeof(oracleRecords[i])) != 0)
				++callbackMismatches;
			}
		else if(i == 1 && (oracleRecords[i].kind != 2 || candidateRecords[i].kind != 2
			|| oracleRecords[i].arguments[0] != reinterpret_cast<unsigned>(oracleOldCollision)
			|| candidateRecords[i].arguments[0] != reinterpret_cast<unsigned>(candidateOldCollision)
			|| memcmp(oracleRecords[i].arguments + 1, candidateRecords[i].arguments + 1,
				sizeof(oracleRecords[i].arguments) - sizeof(oracleRecords[i].arguments[0])) != 0))
			++callbackMismatches;
		else if(i == 2 && (oracleRecords[i].kind != 1 || candidateRecords[i].kind != 1
			|| memcmp(oracleRecords[i].arguments, candidateRecords[i].arguments,
				sizeof(oracleRecords[i].arguments[0])) != 0
			|| oracleRecords[i].arguments[1] != reinterpret_cast<unsigned>(oracleScene)
			|| candidateRecords[i].arguments[1] != reinterpret_cast<unsigned>(candidateScene)
			|| memcmp(oracleRecords[i].arguments + 2, candidateRecords[i].arguments + 2,
				sizeof(oracleRecords[i].arguments) - 2 * sizeof(oracleRecords[i].arguments[0])) != 0))
			++callbackMismatches;
		}
	const bool replacementBuilt =
		memcmp(oracleReplacementCollision + 0x64,
			candidateReplacementCollision + 0x64, 8) == 0
		&& *reinterpret_cast<unsigned*>(oracleReplacementCollision + 0x64)
			== 0x40000000u
		&& *reinterpret_cast<unsigned*>(oracleReplacementCollision + 0x68)
			== 0x3fd020c5u;
	unsigned char untouchedOldCollision[sizeof(oracleOldCollision)];
	memset(untouchedOldCollision, 0xab, sizeof(untouchedOldCollision));
	const bool oldCollisionPreserved =
		memcmp(oracleOldCollision, untouchedOldCollision,
			sizeof(oracleOldCollision)) == 0
		&& memcmp(candidateOldCollision, untouchedOldCollision,
			sizeof(candidateOldCollision)) == 0;
	const bool parametersMatch = replacementBuilt && oldCollisionPreserved;
	const bool dirtyCleared = oracleManager[0x28] == 0
		&& candidateManager[0x28] == 0;
	const bool stackMatch = oracleBefore == oracleAfter
		&& candidateBefore == candidateAfter;
	const bool matched = callbackMismatches == 0 && parametersMatch
		&& dirtyCleared && stackMatch
		&& oracleException == 0 && candidateException == 0
		&& oracleManagerResult == candidateManagerResult;
	printf("fluid manager step dirty-refresh callbacks=%u/%u order=%u "
		"parameters=%u dirty-clear=%u stack=%u/%u exceptions=%08x@%08x/%08x@%08x mismatches=%u\n",
		oracleRecordCount, candidateRecordCount, callbackMismatches == 0,
		parametersMatch, dirtyCleared,
		oracleBefore == oracleAfter, candidateBefore == candidateAfter,
		oracleException, oracleExceptionAddress,
		candidateException, candidateExceptionAddress,
		matched ? 0u : 1u);
	printf("fluid manager replacement-buffer built=%u old-preserved=%u\n",
		replacementBuilt, oldCollisionPreserved);
	if(!matched)
		++mismatches;
	*managerGetSlot = savedManagerGet;
	*managerDestroySlot = savedManagerDestroy;
	}
	DWORD ignored = 0;
	VirtualProtect(page, 0x1000, PAGE_READWRITE, &ignored);
	for(unsigned i = 0; i != 16; ++i)
		*reinterpret_cast<void**>(const_cast<unsigned char*>(oracleBase) + slots[i]) = saved[i];
	*reinterpret_cast<void**>(const_cast<unsigned char*>(oracleBase) + findCellSlot) = saved[16];
	VirtualProtect(page, 0x1000, oldProtection, &ignored);
	printf("fluid collision static-rebuild cases=2 empty=1 nonempty_mesh=1 "
		"candidate_present=1 mismatches=%u\n", mismatches);
	return mismatches;
	}

extern "C" volatile unsigned nxFluidEmitterRawStackDelta;
extern "C" volatile unsigned nxFluidEmitterRawStackDelta = 0xffffffffu;
extern "C" volatile unsigned nxFluidEmitterRawStackBefore;
extern "C" volatile unsigned nxFluidEmitterRawStackBefore = 0;

// Model the oracle's x86 member ABI directly: this in ecx, hidden result at
// [esp+4], and ret 4. The post-call stack delta and eax result are observed
// before returning to the C++ test harness.
extern "C" __declspec(naked) void* __cdecl nxFluidEmitterCallAggregate(
	void*, unsigned, void*)
	{
	__asm
		{
		mov ecx, [esp+4]
		mov edx, [esp+8]
		mov eax, [ecx]
		mov dword ptr [nxFluidEmitterRawStackBefore], esp
		push dword ptr [esp+0Ch]
		call dword ptr [eax+edx*4]
		mov ecx, esp
		sub ecx, dword ptr [nxFluidEmitterRawStackBefore]
		mov dword ptr [nxFluidEmitterRawStackDelta], ecx
		ret
		}
	}

static int nxFluidEmitterExceptionFilter(EXCEPTION_POINTERS* exception)
	{
	fprintf(stderr, "fluid emitter raw ABI exception code=%08lx address=%p\n",
		exception->ExceptionRecord->ExceptionCode, exception->ExceptionRecord->ExceptionAddress);
	return EXCEPTION_EXECUTE_HANDLER;
	}

static bool nxTryFluidEmitterCall(void* self, unsigned slot, void* output,
	void** result, unsigned* stackDelta)
	{
	__try
		{
		*result = nxFluidEmitterCallAggregate(self, slot, output);
		*stackDelta = nxFluidEmitterRawStackDelta;
		return true;
		}
	__except(nxFluidEmitterExceptionFilter(GetExceptionInformation()))
		{
		return false;
		}
	}

static bool nxCheckEqual(const char* name, const void* oracle, const void* candidate, size_t size)
	{
	if(memcmp(oracle, candidate, size) == 0)
		return true;
	++gFailures;
	fprintf(stderr, "fluid emitter aggregate mismatch: %s\n", name);
	if(size == 12)
		{
		const unsigned* expected = static_cast<const unsigned*>(oracle);
		const unsigned* actual = static_cast<const unsigned*>(candidate);
		fprintf(stderr, "  %08x.%08x.%08x expected %08x.%08x.%08x actual\n",
			expected[0], expected[1], expected[2], actual[0], actual[1], actual[2]);
		}
	return false;
	}

static bool nxSha256(const wchar_t* path, char output[65])
	{
	HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, 0);
	if(file == INVALID_HANDLE_VALUE)
		return false;
	LARGE_INTEGER length;
	if(!GetFileSizeEx(file, &length) || length.QuadPart < 0 || length.QuadPart > 0x7fffffff)
		{
		CloseHandle(file);
		return false;
		}
	const ULONG bytes = static_cast<ULONG>(length.QuadPart);
	unsigned char* buffer = static_cast<unsigned char*>(malloc(bytes ? bytes : 1));
	if(!buffer)
		{
		CloseHandle(file);
		return false;
		}
	DWORD read = 0;
	const bool readOk = ReadFile(file, buffer, bytes, &read, 0) != 0 && read == bytes;
	CloseHandle(file);
	BCRYPT_ALG_HANDLE algorithm = 0;
	BCRYPT_HASH_HANDLE hash = 0;
	unsigned char digest[32];
	const bool ok = readOk
		&& BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, 0, 0) == 0
		&& BCryptCreateHash(algorithm, &hash, 0, 0, 0, 0, 0) == 0
		&& BCryptHashData(hash, buffer, bytes, 0) == 0
		&& BCryptFinishHash(hash, digest, sizeof(digest), 0) == 0;
	if(hash) BCryptDestroyHash(hash);
	if(algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
	free(buffer);
	if(!ok)
		return false;
	for(unsigned i = 0; i < sizeof(digest); ++i)
		sprintf_s(output + i * 2, 3, "%02x", digest[i]);
	return true;
	}

struct NxLockSlots
	{
	void* values[4];
	void* page;
	DWORD protection;
	};

extern "C" int __stdcall nxFluidLockStub1(void*) { return 1; }
extern "C" int __stdcall nxFluidLockStub3(void*, int, int) { return 1; }
extern "C" int __stdcall nxFluidLockStubQuery() { return 0x2222; }

static NxLockSlots nxBindOracleLocks(unsigned char* image)
	{
	NxLockSlots slots;
	void** addresses[4] = {
		reinterpret_cast<void**>(image + 0x104010),
		reinterpret_cast<void**>(image + 0x10402c),
		reinterpret_cast<void**>(image + 0x104044),
		reinterpret_cast<void**>(image + 0x104014),
		};
	slots.values[0] = *addresses[0];
	slots.values[1] = *addresses[1];
	slots.values[2] = *addresses[2];
	slots.values[3] = *addresses[3];
	slots.page = reinterpret_cast<void*>(reinterpret_cast<size_t>(image + 0x104000)
		& ~static_cast<size_t>(0xfff));
	slots.protection = 0;
	if(!VirtualProtect(slots.page, 0x2000, PAGE_READWRITE, &slots.protection))
		return slots;
	*addresses[0] = reinterpret_cast<void*>(&nxFluidLockStub1);
	*addresses[1] = reinterpret_cast<void*>(&nxFluidLockStub3);
	*addresses[2] = reinterpret_cast<void*>(&nxFluidLockStubQuery);
	*addresses[3] = reinterpret_cast<void*>(&nxFluidLockStub1);
	return slots;
	}

static void nxUnbindOracleLocks(unsigned char* image, const NxLockSlots& slots)
	{
	if(!slots.protection)
		return;
	DWORD ignored = 0;
	VirtualProtect(slots.page, 0x2000, PAGE_READWRITE, &ignored);
	*reinterpret_cast<void**>(image + 0x104010) = slots.values[0];
	*reinterpret_cast<void**>(image + 0x10402c) = slots.values[1];
	*reinterpret_cast<void**>(image + 0x104044) = slots.values[2];
	*reinterpret_cast<void**>(image + 0x104014) = slots.values[3];
	VirtualProtect(slots.page, 0x2000, slots.protection, &ignored);
	}

static int nxCallOracleFluidRepulsion(void* target, void* emitter,
	unsigned* stackBefore, unsigned* stackAfter)
	{
	typedef int (__thiscall *Repulsion)(void*);
	float incoming = 0.375f;
	unsigned before = 0, after = 0;
	__asm fld incoming
	__asm mov before, esp
	const int result = reinterpret_cast<Repulsion>(target)(emitter);
	__asm mov after, esp
	// The oracle helper consumes its callback mass and returns with the caller's
	// FluidRun result still on the x87 stack.
	__asm fstp st(0)
	*stackBefore = before;
	*stackAfter = after;
	return result;
	}

static int nxCallCandidateFluidRepulsion(void* target, void* emitter,
	unsigned* stackBefore, unsigned* stackAfter)
	{
	typedef int (__cdecl *Repulsion)(void*);
	unsigned before = 0, after = 0;
	__asm mov before, esp
	const int result = reinterpret_cast<Repulsion>(target)(emitter);
	__asm mov after, esp
	*stackBefore = before;
	*stackAfter = after;
	return result;
	}

// The reconstructed pointer and aggregate getters are production methods in
// this probe. Other pure virtuals remain test shims until their rows are closed.
class FluidEmitterProbe : public NpFluidEmitter
	{
	public:
	FluidEmitterProbe(void* internal) : NpFluidEmitter(internal) {}
	virtual NxFluid& getFluid() const { return NpFluidEmitter::getFluid(); }
	virtual void setGlobalPose(const NxMat34& pose) { NpFluidEmitter::setGlobalPose(pose); }
	virtual void setGlobalPosition(const NxVec3& position)
		{ NpFluidEmitter::setGlobalPosition(position); }
	virtual void setGlobalOrientation(const NxMat33& orientation)
		{ NpFluidEmitter::setGlobalOrientation(orientation); }
	virtual void setLocalPose(const NxMat34&) {}
	virtual void setLocalPosition(const NxVec3& position)
		{ NpFluidEmitter::setLocalPosition(position); }
	virtual void setLocalOrientation(const NxMat33& orientation)
		{ NpFluidEmitter::setLocalOrientation(orientation); }
	virtual void setFrameActor(NxActor*) {}
	virtual NxActor* getFrameActor() const { return NpFluidEmitter::getFrameActor(); }
	virtual NxReal getDimensionX() const { return NpFluidEmitter::getDimensionX(); }
	virtual NxReal getDimensionY() const { return NpFluidEmitter::getDimensionY(); }
	virtual void setRandomPos(NxVec3 value) { NpFluidEmitter::setRandomPos(value); }
	virtual NxVec3 getRandomPos() const { return NpFluidEmitter::getRandomPos(); }
	virtual void setRandomAngle(NxReal value) { NpFluidEmitter::setRandomAngle(value); }
	virtual NxReal getRandomAngle() const { return NpFluidEmitter::getRandomAngle(); }
	virtual void setFluidVelocityMagnitude(NxReal value)
		{ NpFluidEmitter::setFluidVelocityMagnitude(value); }
	virtual NxReal getFluidVelocityMagnitude() const
		{ return NpFluidEmitter::getFluidVelocityMagnitude(); }
	virtual void setRate(NxReal value) { NpFluidEmitter::setRate(value); }
	virtual NxReal getRate() const { return NpFluidEmitter::getRate(); }
	virtual void setParticleLifetime(NxReal value)
		{ NpFluidEmitter::setParticleLifetime(value); }
	virtual NxReal getParticleLifetime() const
		{ return NpFluidEmitter::getParticleLifetime(); }
	virtual NX_BOOL getShape(NxEmitterShape) const { return 0; }
	virtual NX_BOOL getType(NxEmitterType) const { return 0; }
	virtual void setName(const char* name) { NpFluidEmitter::setName(name); }
	virtual const char* getName() const { return NpFluidEmitter::getName(); }
	};

static unsigned nxRunFluidEmitterWorldComponentSetters(NxFluidEmitter* oracle,
	NxFluidEmitter* candidate, NxFluidEmitter* candidateDll,
	unsigned char* oracleInternal, unsigned char* candidateInternal,
	unsigned char* candidateDllInternal, void* fluidModel)
	{
	if(!oracle || !candidate || !candidateDll || !oracleInternal
		|| !candidateInternal || !candidateDllInternal || !fluidModel)
		return 1;
	typedef void (__cdecl *SetPoseFn)(const float*);
	typedef void (__cdecl *ResetFn)();
	typedef unsigned (__cdecl *CountFn)();
	typedef int (__cdecl *GetFn)(unsigned, NxFluidEmitterFlagCallbackRecord*);
	SetPoseFn setCandidatePose = reinterpret_cast<SetPoseFn>(GetProcAddress(
		static_cast<HMODULE>(fluidModel), "NxSetEmitterGlobalPose"));
	ResetFn resetCallbacks = reinterpret_cast<ResetFn>(GetProcAddress(
		static_cast<HMODULE>(fluidModel), "NxResetFluidEmitterFlagCallbacks"));
	CountFn callbackCount = reinterpret_cast<CountFn>(GetProcAddress(
		static_cast<HMODULE>(fluidModel), "NxGetFluidEmitterFlagCallbackCount"));
	GetFn getCallback = reinterpret_cast<GetFn>(GetProcAddress(
		static_cast<HMODULE>(fluidModel), "NxGetFluidEmitterFlagCallbackRecord"));
	if(!setCandidatePose || !resetCallbacks || !callbackCount || !getCallback)
		return 1;
	static const unsigned localPose[12] = {
		0x3f2aaaab, 0xbecccccd, 0x3e4ccccd, 0x3e99999a,
		0x3f666666, 0xbf000000, 0x3dcccccd, 0x3e800000,
		0x3f400000, 0x40000000, 0xc0400000, 0x40a00000 };
	static const unsigned globalPose[12] = {
		0x3f000001, 0x3e000002, 0xbf000003, 0xbe800004,
		0x3f200005, 0x3e800006, 0x3f400007, 0xbf400008,
		0x3e400009, 0x41234567, 0xc2345678, 0x42abcdef };
	static const unsigned position[3] = { 0x40f12345, 0xc1023456, 0x41a54321 };
	static const unsigned orientation[9] = {
		0x3f700001, 0xbe900002, 0x3e100003, 0x3e800004,
		0x3f600005, 0xbe700006, 0xbdc00007, 0x3e500008,
		0x3f500009 };
	const unsigned context = 0x6a1b2c3du;
	const unsigned fluidHandle = 0x7b2c3d4eu;
	const unsigned emitterHandle = 0x8c3d4e5fu;
	unsigned mismatches = 0, callbackChecks = 0, stackChecks = 0;
	for(unsigned test = 0; test < 4; ++test)
		{
		unsigned char* internals[] = { oracleInternal, candidateInternal,
			candidateDllInternal };
		for(unsigned side = 0; side < 3; ++side)
			{
			memset(internals[side] + 0x14, 0, 0x64);
			*reinterpret_cast<void**>(internals[side] + 0x14) = 0;
			memcpy(internals[side] + 0x18, localPose, sizeof(localPose));
			memcpy(internals[side] + 0x48, globalPose, sizeof(globalPose));
			}
		memcpy(gOracleEmitterGlobalPose, globalPose, sizeof(globalPose));
		setCandidatePose(reinterpret_cast<const float*>(globalPose));
		NxFluidEmitterFlagCallbackRecord oracleRecord = {};
		NxFluidEmitterFlagCallbackRecord candidateRecord = {};
		NxFluidEmitterFlagCallbackRecord candidateDllRecord = {};
		unsigned oracleBefore = 0, oracleAfter = 0;
		unsigned candidateBefore = 0, candidateAfter = 0;
		unsigned candidateDllBefore = 0, candidateDllAfter = 0;
		gOracleFlagCallbackCount = 0;
		__asm mov oracleBefore, esp
		if(test == 0)
			oracle->setGlobalPosition(*reinterpret_cast<const NxVec3*>(position));
		else if(test == 1)
			oracle->setGlobalOrientation(*reinterpret_cast<const NxMat33*>(orientation));
		else if(test == 2)
			oracle->setLocalPosition(*reinterpret_cast<const NxVec3*>(position));
		else
			oracle->setLocalOrientation(*reinterpret_cast<const NxMat33*>(orientation));
		__asm mov oracleAfter, esp
		const unsigned oracleCallbacks = gOracleFlagCallbackCount;
		bool oracleSetCallback = false;
		for(unsigned callback = 0; callback < oracleCallbacks; ++callback)
			if(gOracleFlagCallbacks[callback].kind == 64)
				{
				oracleRecord = gOracleFlagCallbacks[callback];
				oracleSetCallback = true;
				}
		resetCallbacks();
		__asm mov candidateBefore, esp
		if(test == 0)
			candidate->setGlobalPosition(*reinterpret_cast<const NxVec3*>(position));
		else if(test == 1)
			candidate->setGlobalOrientation(*reinterpret_cast<const NxMat33*>(orientation));
		else if(test == 2)
			candidate->setLocalPosition(*reinterpret_cast<const NxVec3*>(position));
		else
			candidate->setLocalOrientation(*reinterpret_cast<const NxMat33*>(orientation));
		__asm mov candidateAfter, esp
		const unsigned candidateCallbacks = callbackCount();
		bool candidateSetCallback = false;
		for(unsigned callback = 0; callback < candidateCallbacks; ++callback)
			{
			NxFluidEmitterFlagCallbackRecord record = {};
			if(getCallback(callback, &record) && record.kind == 64)
				{
				candidateRecord = record;
				candidateSetCallback = true;
				}
			}
		resetCallbacks();
		__asm mov candidateDllBefore, esp
		if(test == 0)
			candidateDll->setGlobalPosition(*reinterpret_cast<const NxVec3*>(position));
		else if(test == 1)
			candidateDll->setGlobalOrientation(*reinterpret_cast<const NxMat33*>(orientation));
		else if(test == 2)
			candidateDll->setLocalPosition(*reinterpret_cast<const NxVec3*>(position));
		else
			candidateDll->setLocalOrientation(*reinterpret_cast<const NxMat33*>(orientation));
		__asm mov candidateDllAfter, esp
		const unsigned candidateDllCallbacks = callbackCount();
		bool candidateDllSetCallback = false;
		for(unsigned callback = 0; callback < candidateDllCallbacks; ++callback)
			{
			NxFluidEmitterFlagCallbackRecord record = {};
			if(getCallback(callback, &record) && record.kind == 64)
				{
				candidateDllRecord = record;
				candidateDllSetCallback = true;
				}
			}
		const bool callbacksMatch = oracleSetCallback
			&& candidateSetCallback && candidateDllSetCallback
			&& memcmp(&oracleRecord, &candidateRecord, sizeof(oracleRecord)) == 0
			&& memcmp(&oracleRecord, &candidateDllRecord, sizeof(oracleRecord)) == 0;
		const bool stateMatch = memcmp(oracleInternal + 0x18,
			candidateInternal + 0x18, 0x30) == 0
			&& memcmp(oracleInternal + 0x18, candidateDllInternal + 0x18, 0x30) == 0;
		const bool balanced = oracleBefore == oracleAfter
			&& candidateBefore == candidateAfter
			&& candidateDllBefore == candidateDllAfter;
		callbackChecks += callbacksMatch;
		stackChecks += balanced;
		if(!callbacksMatch || !stateMatch || !balanced)
			{
			++mismatches;
			printf("fluid emitter global component setter case=%u callbacks=%u/%u/%u state=%u stack=%u/%u/%u\n",
				test, oracleCallbacks, candidateCallbacks, candidateDllCallbacks,
				stateMatch, oracleBefore == oracleAfter,
				candidateBefore == candidateAfter, candidateDllBefore == candidateDllAfter);
			printf("  oracle callback kinds=");
			for(unsigned callback = 0; callback < oracleCallbacks; ++callback)
				printf("%s%u", callback ? "," : "", gOracleFlagCallbacks[callback].kind);
			printf(" candidate=%u candidate_dll=%u\n", candidateRecord.kind,
				candidateDllRecord.kind);
			for(unsigned word = 0; word < 24; ++word)
				{
				const unsigned oracleWord = *reinterpret_cast<unsigned*>(oracleInternal + 0x18 + word * 4);
				const unsigned candidateWord = *reinterpret_cast<unsigned*>(candidateInternal + 0x18 + word * 4);
				const unsigned candidateDllWord = *reinterpret_cast<unsigned*>(candidateDllInternal + 0x18 + word * 4);
				if(oracleWord != candidateWord || oracleWord != candidateDllWord)
					printf("  state[%u]=%08x/%08x/%08x\n", word, oracleWord,
						candidateWord, candidateDllWord);
				}
			}
		}
	printf("fluid emitter global and local component setters cases=4 callback_checks=%u stack_checks=%u mismatches=%u\n",
		callbackChecks, stackChecks, mismatches);
	return mismatches;
	}

static unsigned nxRunFluidEmitterNameRows(unsigned char* image,
	NxFluidEmitter* oracle, NxFluidEmitter* candidate,
	NxFluidEmitter* candidateDll, void* internal)
	{
	unsigned mismatches = 0;
	static const char firstName[] = "fluid-emitter-name-first";
	static const char replacementName[] = "fluid-emitter-name-replacement";
	static const char seededName[] = "fluid-emitter-name-seeded";
	static const char retainedName[] = "fluid-emitter-name-retained";
	unsigned records[4] = {
		static_cast<unsigned>(reinterpret_cast<size_t>(internal)),
		static_cast<unsigned>(reinterpret_cast<size_t>(seededName)),
		0x11110000u,
		static_cast<unsigned>(reinterpret_cast<size_t>(retainedName)) };
	unsigned* tracker[] = { records, records + 4, records + 4 };
	void** trackerGlobal = reinterpret_cast<void**>(image + 0x123c0c);
	void* originalTracker = *trackerGlobal;
	DWORD ignored = 0, protection = 0;
	if(!VirtualProtect(trackerGlobal, sizeof(*trackerGlobal), PAGE_READWRITE,
		&protection))
		{
		fprintf(stderr, "FAIL oracle fluid-emitter name tracker protection error=%lu\n",
			GetLastError());
		return 1;
		}
	*trackerGlobal = tracker;
	oracle->setName(firstName);
	candidate->setName(firstName);
	candidateDll->setName(firstName);
	if(oracle->getName() != firstName || candidate->getName() != firstName
		|| candidateDll->getName() != firstName
		|| strcmp(oracle->getName(), candidate->getName()) != 0
		|| strcmp(oracle->getName(), candidateDll->getName()) != 0)
		++mismatches;
	oracle->setName(replacementName);
	candidate->setName(replacementName);
	candidateDll->setName(replacementName);
	if(oracle->getName() != replacementName || candidate->getName() != replacementName
		|| candidateDll->getName() != replacementName
		|| strcmp(oracle->getName(), candidate->getName()) != 0)
		++mismatches;
	oracle->setName(0);
	candidate->setName(0);
	candidateDll->setName(0);
	if(oracle->getName() != 0 || candidate->getName() != 0
		|| candidateDll->getName() != 0)
		++mismatches;
	if(records[0] != 0x11110000u || records[1]
		!= static_cast<unsigned>(reinterpret_cast<size_t>(retainedName)))
		++mismatches;
	*trackerGlobal = originalTracker;
	VirtualProtect(trackerGlobal, sizeof(*trackerGlobal), protection, &ignored);
	printf("fluid emitter name rows=2 replacements=1 clears=1 candidate_dll=1 mismatches=%u\n",
		mismatches);
	return mismatches;
	}

struct NxFluidEmitterAttributeCallbackRecord
	{
	unsigned kind;
	unsigned context;
	unsigned fluid;
	unsigned emitter;
	unsigned values[3];
	};

static NxFluidEmitterAttributeCallbackRecord gOracleAttributeRecord;
static unsigned gOracleAttributeRecordCount;
static const unsigned gOracleRandomPositionWords[3] = {
	0x3f000001u, 0xc0000002u, 0x40400003u };

static NxFluidEmitterAttributeCallbackRecord& nxRecordOracleAttribute(
	unsigned kind, unsigned context, unsigned fluid, unsigned emitter)
	{
	memset(&gOracleAttributeRecord, 0, sizeof(gOracleAttributeRecord));
	gOracleAttributeRecord.kind = kind;
	gOracleAttributeRecord.context = context;
	gOracleAttributeRecord.fluid = fluid;
	gOracleAttributeRecord.emitter = emitter;
	gOracleAttributeRecordCount = 1;
	return gOracleAttributeRecord;
	}

extern "C" float __cdecl nxOracleEmitterGetDimensionX(
	unsigned context, unsigned fluid, unsigned emitter)
	{
	nxRecordOracleAttribute(1, context, fluid, emitter);
	const unsigned bits = 0x40500001u;
	return *reinterpret_cast<const float*>(&bits);
	}
extern "C" float __cdecl nxOracleEmitterGetDimensionY(
	unsigned context, unsigned fluid, unsigned emitter)
	{
	nxRecordOracleAttribute(2, context, fluid, emitter);
	const unsigned bits = 0x3fc00002u;
	return *reinterpret_cast<const float*>(&bits);
	}
extern "C" int __cdecl nxOracleEmitterSetRandomPos(unsigned context,
	unsigned fluid, unsigned emitter, const float* value)
	{
	NxFluidEmitterAttributeCallbackRecord& record = nxRecordOracleAttribute(
		3, context, fluid, emitter);
	if(value) memcpy(record.values, value, sizeof(record.values));
	return 3;
	}
extern "C" const float* __cdecl nxOracleEmitterGetRandomPos(
	unsigned context, unsigned fluid, unsigned emitter)
	{
	NxFluidEmitterAttributeCallbackRecord& record = nxRecordOracleAttribute(
		4, context, fluid, emitter);
	memcpy(record.values, gOracleRandomPositionWords, sizeof(record.values));
	return reinterpret_cast<const float*>(gOracleRandomPositionWords);
	}
extern "C" int __cdecl nxOracleEmitterSetRandomAngle(unsigned context,
	unsigned fluid, unsigned emitter, float value)
	{
	NxFluidEmitterAttributeCallbackRecord& record = nxRecordOracleAttribute(
		5, context, fluid, emitter);
	memcpy(record.values, &value, sizeof(value));
	return 5;
	}
extern "C" float __cdecl nxOracleEmitterGetRandomAngle(
	unsigned context, unsigned fluid, unsigned emitter)
	{
	nxRecordOracleAttribute(6, context, fluid, emitter);
	const unsigned bits = 0x3e000006u;
	return *reinterpret_cast<const float*>(&bits);
	}
extern "C" int __cdecl nxOracleEmitterSetFluidVelocityMagnitude(
	unsigned context, unsigned fluid, unsigned emitter, float value)
	{
	NxFluidEmitterAttributeCallbackRecord& record = nxRecordOracleAttribute(
		7, context, fluid, emitter);
	memcpy(record.values, &value, sizeof(value));
	return 7;
	}
extern "C" float __cdecl nxOracleEmitterGetFluidVelocityMagnitude(
	unsigned context, unsigned fluid, unsigned emitter)
	{
	nxRecordOracleAttribute(8, context, fluid, emitter);
	const unsigned bits = 0x40e00008u;
	return *reinterpret_cast<const float*>(&bits);
	}
extern "C" int __cdecl nxOracleEmitterSetRate(unsigned context,
	unsigned fluid, unsigned emitter, float value)
	{
	NxFluidEmitterAttributeCallbackRecord& record = nxRecordOracleAttribute(
		9, context, fluid, emitter);
	memcpy(record.values, &value, sizeof(value));
	return 9;
	}
extern "C" float __cdecl nxOracleEmitterGetRate(
	unsigned context, unsigned fluid, unsigned emitter)
	{
	nxRecordOracleAttribute(10, context, fluid, emitter);
	const unsigned bits = 0x42b4000au;
	return *reinterpret_cast<const float*>(&bits);
	}
extern "C" int __cdecl nxOracleEmitterSetParticleLifetime(
	unsigned context, unsigned fluid, unsigned emitter, float value)
	{
	NxFluidEmitterAttributeCallbackRecord& record = nxRecordOracleAttribute(
		11, context, fluid, emitter);
	memcpy(record.values, &value, sizeof(value));
	return 11;
	}
extern "C" float __cdecl nxOracleEmitterGetParticleLifetime(
	unsigned context, unsigned fluid, unsigned emitter)
	{
	nxRecordOracleAttribute(12, context, fluid, emitter);
	const unsigned bits = 0x4020000cu;
	return *reinterpret_cast<const float*>(&bits);
	}

static unsigned nxFluidEmitterFloatBits(float value)
	{
	unsigned bits;
	memcpy(&bits, &value, sizeof(bits));
	return bits;
	}

static unsigned nxRunFluidEmitterAttributeRows(unsigned char* image,
	HMODULE fluidModel, HMODULE candidatePhysics)
	{
	static const unsigned slotRvas[12] = {
		0x1265cc, 0x1265d4, 0x126564, 0x1264ec,
		0x126598, 0x12662c, 0x126464, 0x1264f4,
		0x1265c4, 0x1264c0, 0x1264c4, 0x126540 };
	static void* const replacements[12] = {
		reinterpret_cast<void*>(&nxOracleEmitterGetDimensionX),
		reinterpret_cast<void*>(&nxOracleEmitterGetDimensionY),
		reinterpret_cast<void*>(&nxOracleEmitterSetRandomPos),
		reinterpret_cast<void*>(&nxOracleEmitterGetRandomPos),
		reinterpret_cast<void*>(&nxOracleEmitterSetRandomAngle),
		reinterpret_cast<void*>(&nxOracleEmitterGetRandomAngle),
		reinterpret_cast<void*>(&nxOracleEmitterSetFluidVelocityMagnitude),
		reinterpret_cast<void*>(&nxOracleEmitterGetFluidVelocityMagnitude),
		reinterpret_cast<void*>(&nxOracleEmitterSetRate),
		reinterpret_cast<void*>(&nxOracleEmitterGetRate),
		reinterpret_cast<void*>(&nxOracleEmitterSetParticleLifetime),
		reinterpret_cast<void*>(&nxOracleEmitterGetParticleLifetime) };
	void* oldValues[12] = {};
	DWORD oldProtection = 0;
	void* const page = reinterpret_cast<void*>(
		reinterpret_cast<size_t>(image + 0x126000) & ~static_cast<size_t>(0xfff));
	if(!VirtualProtect(page, 0x1000, PAGE_READWRITE, &oldProtection))
		return 1;
	for(unsigned i = 0; i != 12; ++i)
		{
		void** slot = reinterpret_cast<void**>(image + slotRvas[i]);
		oldValues[i] = *slot;
		*slot = replacements[i];
		}
	typedef void (__cdecl *ResetFn)();
	typedef unsigned (__cdecl *CountFn)();
	typedef int (__cdecl *GetFn)(unsigned, NxFluidEmitterAttributeCallbackRecord*);
	ResetFn reset = reinterpret_cast<ResetFn>(GetProcAddress(fluidModel,
		"NxResetFluidEmitterAttributeCallbacks"));
	CountFn count = reinterpret_cast<CountFn>(GetProcAddress(fluidModel,
		"NxGetFluidEmitterAttributeCallbackCount"));
	GetFn get = reinterpret_cast<GetFn>(GetProcAddress(fluidModel,
		"NxGetFluidEmitterAttributeCallbackRecord"));
	unsigned mismatches = 0;
	if(!reset || !count || !get)
		{
		++mismatches;
		fprintf(stderr, "FAIL FluidModel attribute callback fixture exports missing\n");
		goto cleanup;
		}
	{
		unsigned char oracleFluid[0x84] = {};
		unsigned char candidateFluid[0x84] = {};
		unsigned char oracleBackend[0x34] = {};
		unsigned char candidateBackend[0x34] = {};
		unsigned char oracleInternal[0x78] = {};
		unsigned char candidateInternal[0x78] = {};
		unsigned char candidateDllInternal[0x78] = {};
		unsigned char oracleLockData[0x20] = {};
		void* oracleLockDataPointer = oracleLockData;
		void* oracleLockObject[1] = { oracleLockDataPointer };
		void* oracleLockLink = oracleLockObject;
		struct CandidateAttributeLockStorage
			{
			CRITICAL_SECTION section;
			unsigned writer;
			unsigned thread;
			};
		CandidateAttributeLockStorage candidateLock = {};
		InitializeCriticalSection(&candidateLock.section);
		CRITICAL_SECTION* candidateSectionPointer = &candidateLock.section;
		void* candidateSectionLink = &candidateSectionPointer;
		static const unsigned context = 0x12345678u;
		static const unsigned fluidHandle = 0x23456789u;
		static const unsigned emitterHandle = 0x3456789au;
		*reinterpret_cast<void**>(oracleFluid + 0x7c) = oracleBackend;
		*reinterpret_cast<void**>(candidateFluid + 0x7c) = candidateBackend;
		*reinterpret_cast<unsigned*>(oracleFluid + 0x80) = fluidHandle;
		*reinterpret_cast<unsigned*>(candidateFluid + 0x80) = fluidHandle;
		*reinterpret_cast<unsigned*>(oracleBackend + 0x30) = context;
		*reinterpret_cast<unsigned*>(candidateBackend + 0x30) = context;
		*reinterpret_cast<void**>(oracleInternal + 4) = oracleFluid;
		*reinterpret_cast<void**>(candidateInternal + 4) = candidateFluid;
		*reinterpret_cast<void**>(candidateDllInternal + 4) = candidateFluid;
		*reinterpret_cast<unsigned*>(oracleInternal + 8) = emitterHandle;
		*reinterpret_cast<unsigned*>(candidateInternal + 8) = emitterHandle;
		*reinterpret_cast<unsigned*>(candidateDllInternal + 8) = emitterHandle;
		unsigned char oracleObject[0x18] = {};
		typedef void* (__thiscall *WrapperCtor)(void*, void*);
		reinterpret_cast<WrapperCtor>(image + 0x8c2d0)(oracleObject, oracleInternal);
		memcpy(oracleObject + 0x0c, &oracleLockLink, sizeof(oracleLockLink));
		memcpy(oracleObject + 0x10, &oracleLockLink, sizeof(oracleLockLink));
		FluidEmitterProbe candidateObject(candidateInternal);
		memcpy(reinterpret_cast<unsigned char*>(&candidateObject) + 0x0c,
			&candidateSectionLink, sizeof(candidateSectionLink));
		memcpy(reinterpret_cast<unsigned char*>(&candidateObject) + 0x10,
			&candidateSectionLink, sizeof(candidateSectionLink));
		unsigned char candidateDllObject[0x18] = {};
		void* candidateCtorAddress = 0;
		if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
			"??0NpFluidEmitter@@QAE@PAX@Z", &candidateCtorAddress))
			{
			++mismatches;
			fprintf(stderr, "FAIL candidate DLL fluid-emitter constructor unavailable for attribute rows\n");
			DeleteCriticalSection(&candidateLock.section);
			goto cleanup;
			}
		typedef void* (__thiscall *CandidateCtor)(void*, void*);
		reinterpret_cast<CandidateCtor>(candidateCtorAddress)(candidateDllObject,
			candidateDllInternal);
		memcpy(candidateDllObject + 0x0c, &candidateSectionLink,
			sizeof(candidateSectionLink));
		memcpy(candidateDllObject + 0x10, &candidateSectionLink,
			sizeof(candidateSectionLink));
		NxFluidEmitter* const oracle = reinterpret_cast<NxFluidEmitter*>(oracleObject);
		NxFluidEmitter* const candidate = &candidateObject;
		NxFluidEmitter* const candidateDll = reinterpret_cast<NxFluidEmitter*>(candidateDllObject);
		unsigned cases = 0;
		unsigned callbacks = 0;
		auto check = [&](unsigned kind, const unsigned* expectedResult,
			const unsigned* candidateResult, const unsigned* candidateDllResult,
			const NxFluidEmitterAttributeCallbackRecord& candidateRecord,
			const NxFluidEmitterAttributeCallbackRecord& candidateDllRecord)
			{
				const bool oracleRecordOk = gOracleAttributeRecordCount == 1
					&& gOracleAttributeRecord.kind == kind
					&& gOracleAttributeRecord.context == context
					&& gOracleAttributeRecord.fluid == fluidHandle
					&& gOracleAttributeRecord.emitter == emitterHandle;
				const bool candidateRecordOk = candidateRecord.kind == kind
					&& candidateRecord.context == context
					&& candidateRecord.fluid == fluidHandle
					&& candidateRecord.emitter == emitterHandle
					&& memcmp(candidateRecord.values, gOracleAttributeRecord.values,
						sizeof(candidateRecord.values)) == 0;
				const bool candidateDllRecordOk = candidateDllRecord.kind == kind
					&& candidateDllRecord.context == context
					&& candidateDllRecord.fluid == fluidHandle
					&& candidateDllRecord.emitter == emitterHandle
					&& memcmp(candidateDllRecord.values, gOracleAttributeRecord.values,
						sizeof(candidateDllRecord.values)) == 0;
				const bool resultOk = !expectedResult || !candidateResult
					|| memcmp(expectedResult, candidateResult, 3 * sizeof(unsigned)) == 0;
				const bool dllResultOk = !expectedResult || !candidateDllResult
					|| memcmp(expectedResult, candidateDllResult,
						3 * sizeof(unsigned)) == 0;
				if(!oracleRecordOk || !candidateRecordOk || !candidateDllRecordOk
					|| !resultOk || !dllResultOk)
					++mismatches;
				++cases;
				callbacks += 2;
			};
		auto capture = [&]()
			{
				NxFluidEmitterAttributeCallbackRecord record = {};
				if(count() != 1 || !get(0, &record))
					++mismatches;
				return record;
			};
		unsigned oracleBits[3] = {}, candidateBits[3] = {};
		reset(); gOracleAttributeRecordCount = 0;
		oracleBits[0] = nxFluidEmitterFloatBits(oracle->getDimensionX());
		reset();
		candidateBits[0] = nxFluidEmitterFloatBits(candidate->getDimensionX());
		NxFluidEmitterAttributeCallbackRecord candidateRecord = capture();
		reset();
		unsigned candidateDllBits[3] = {};
		candidateDllBits[0] = nxFluidEmitterFloatBits(candidateDll->getDimensionX());
		NxFluidEmitterAttributeCallbackRecord candidateDllRecord = capture();
		check(1, oracleBits, candidateBits, candidateDllBits, candidateRecord, candidateDllRecord);
		reset(); gOracleAttributeRecordCount = 0;
		oracleBits[0] = nxFluidEmitterFloatBits(oracle->getDimensionY());
		reset();
		candidateBits[0] = nxFluidEmitterFloatBits(candidate->getDimensionY());
		candidateRecord = capture(); reset();
		candidateDllBits[0] = nxFluidEmitterFloatBits(candidateDll->getDimensionY());
		candidateDllRecord = capture();
		check(2, oracleBits, candidateBits, candidateDllBits, candidateRecord, candidateDllRecord);
		static const unsigned randomPosInput[3] = { 0x3e800011u, 0xbf400022u, 0x40400033u };
		reset(); gOracleAttributeRecordCount = 0;
		oracle->setRandomPos(*reinterpret_cast<const NxVec3*>(randomPosInput));
		reset(); candidate->setRandomPos(*reinterpret_cast<const NxVec3*>(randomPosInput));
		candidateRecord = capture(); reset();
		candidateDll->setRandomPos(*reinterpret_cast<const NxVec3*>(randomPosInput));
		candidateDllRecord = capture();
		check(3, 0, 0, 0, candidateRecord, candidateDllRecord);
		reset(); gOracleAttributeRecordCount = 0;
		NxVec3 oraclePosition = oracle->getRandomPos();
		memcpy(oracleBits, &oraclePosition, sizeof(oracleBits));
		reset();
		NxVec3 candidatePosition = candidate->getRandomPos();
		memcpy(candidateBits, &candidatePosition, sizeof(candidateBits));
		candidateRecord = capture(); reset();
		NxVec3 candidateDllPosition = candidateDll->getRandomPos();
		memcpy(candidateDllBits, &candidateDllPosition, sizeof(candidateDllBits));
		candidateDllRecord = capture();
		check(4, oracleBits, candidateBits, candidateDllBits, candidateRecord, candidateDllRecord);
		const float setValues[5] = { -0.1875f, 13.25f, -3.75f, 91.5f, 2.625f };
		reset(); gOracleAttributeRecordCount = 0;
		oracle->setRandomAngle(setValues[0]); reset(); candidate->setRandomAngle(setValues[0]);
		candidateRecord = capture(); reset(); candidateDll->setRandomAngle(setValues[0]);
		candidateDllRecord = capture(); check(5, 0, 0, 0, candidateRecord, candidateDllRecord);
		reset(); gOracleAttributeRecordCount = 0;
		oracleBits[0] = nxFluidEmitterFloatBits(oracle->getRandomAngle()); reset();
		candidateBits[0] = nxFluidEmitterFloatBits(candidate->getRandomAngle()); candidateRecord = capture(); reset();
		candidateDllBits[0] = nxFluidEmitterFloatBits(candidateDll->getRandomAngle()); candidateDllRecord = capture();
		check(6, oracleBits, candidateBits, candidateDllBits, candidateRecord, candidateDllRecord);
		reset(); gOracleAttributeRecordCount = 0;
		oracle->setFluidVelocityMagnitude(setValues[1]); reset(); candidate->setFluidVelocityMagnitude(setValues[1]);
		candidateRecord = capture(); reset(); candidateDll->setFluidVelocityMagnitude(setValues[1]);
		candidateDllRecord = capture(); check(7, 0, 0, 0, candidateRecord, candidateDllRecord);
		reset(); gOracleAttributeRecordCount = 0;
		oracleBits[0] = nxFluidEmitterFloatBits(oracle->getFluidVelocityMagnitude()); reset();
		candidateBits[0] = nxFluidEmitterFloatBits(candidate->getFluidVelocityMagnitude()); candidateRecord = capture(); reset();
		candidateDllBits[0] = nxFluidEmitterFloatBits(candidateDll->getFluidVelocityMagnitude()); candidateDllRecord = capture();
		check(8, oracleBits, candidateBits, candidateDllBits, candidateRecord, candidateDllRecord);
		reset(); gOracleAttributeRecordCount = 0;
		oracle->setRate(setValues[2]); reset(); candidate->setRate(setValues[2]); candidateRecord = capture(); reset();
		candidateDll->setRate(setValues[2]); candidateDllRecord = capture();
		check(9, 0, 0, 0, candidateRecord, candidateDllRecord);
		reset(); gOracleAttributeRecordCount = 0;
		oracleBits[0] = nxFluidEmitterFloatBits(oracle->getRate()); reset();
		candidateBits[0] = nxFluidEmitterFloatBits(candidate->getRate()); candidateRecord = capture(); reset();
		candidateDllBits[0] = nxFluidEmitterFloatBits(candidateDll->getRate()); candidateDllRecord = capture();
		check(10, oracleBits, candidateBits, candidateDllBits, candidateRecord, candidateDllRecord);
		reset(); gOracleAttributeRecordCount = 0;
		oracle->setParticleLifetime(setValues[4]); reset(); candidate->setParticleLifetime(setValues[4]); candidateRecord = capture(); reset();
		candidateDll->setParticleLifetime(setValues[4]); candidateDllRecord = capture();
		check(11, 0, 0, 0, candidateRecord, candidateDllRecord);
		reset(); gOracleAttributeRecordCount = 0;
		oracleBits[0] = nxFluidEmitterFloatBits(oracle->getParticleLifetime()); reset();
		candidateBits[0] = nxFluidEmitterFloatBits(candidate->getParticleLifetime()); candidateRecord = capture(); reset();
		candidateDllBits[0] = nxFluidEmitterFloatBits(candidateDll->getParticleLifetime()); candidateDllRecord = capture();
		check(12, oracleBits, candidateBits, candidateDllBits, candidateRecord, candidateDllRecord);
		mismatches += nxRunFluidEmitterWorldComponentSetters(oracle, candidate,
			candidateDll, oracleInternal, candidateInternal, candidateDllInternal,
			fluidModel);
		printf("fluid emitter attribute rows=12 cases=%u candidate_dll_cases=%u callbacks=%u mismatches=%u\n",
			cases, cases, callbacks, mismatches);
		DeleteCriticalSection(&candidateLock.section);
	}
cleanup:
	for(unsigned i = 0; i != 12; ++i)
		*reinterpret_cast<void**>(image + slotRvas[i]) = oldValues[i];
	DWORD ignored = 0;
	VirtualProtect(page, 0x1000, oldProtection, &ignored);
	return mismatches;
	}

static unsigned nxRunFluidEmitterRecordConstructor(unsigned char* image,
	HMODULE candidatePhysics)
	{
	void* candidateAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxNpFluidEmitterConstructRecord", &candidateAddress))
		{
		fprintf(stderr, "FAIL candidate NxPhysics.dll fluid-emitter record constructor could not be resolved from its map\n");
		return 1;
		}
	typedef void* (__thiscall *OracleCtor)(void*);
	typedef void* (__cdecl *CandidateCtor)(void*);
	typedef void* (__thiscall *OracleDtor)(void*, unsigned char);
	OracleCtor oracleCtor = reinterpret_cast<OracleCtor>(image + 0x87c40);
	CandidateCtor candidateCtor = reinterpret_cast<CandidateCtor>(candidateAddress);
	unsigned char oracleRecord[0x78];
	unsigned char candidateRecord[0x78];
	memset(oracleRecord, 0xa5, sizeof(oracleRecord));
	memset(candidateRecord, 0xa5, sizeof(candidateRecord));
	unsigned oracleBefore = 0, oracleAfter = 0;
	unsigned candidateBefore = 0, candidateAfter = 0;
	void* oracleResult = 0;
	void* candidateResult = 0;
	__asm mov oracleBefore, esp
	oracleResult = oracleCtor(oracleRecord);
	__asm mov oracleAfter, esp
	__asm mov candidateBefore, esp
	candidateResult = candidateCtor(candidateRecord);
	__asm mov candidateAfter, esp
	unsigned mismatches = 0;
	const unsigned identity[9] = { 0x3f800000u, 0, 0, 0,
		0x3f800000u, 0, 0, 0, 0x3f800000u };
	const bool returnsMatch = oracleResult == oracleRecord
		&& candidateResult == candidateRecord;
	const bool ctorFieldsMatch =
		*reinterpret_cast<unsigned*>(oracleRecord + 4) == 0
		&& *reinterpret_cast<unsigned*>(candidateRecord + 4) == 0
		&& *reinterpret_cast<unsigned*>(oracleRecord + 8) == 0xffffffffu
		&& *reinterpret_cast<unsigned*>(candidateRecord + 8) == 0xffffffffu
		&& *reinterpret_cast<void**>(oracleRecord + 0x0c) != 0
		&& *reinterpret_cast<void**>(candidateRecord + 0x0c) != 0
		&& *reinterpret_cast<void***>(oracleRecord) != 0
		&& *reinterpret_cast<void***>(candidateRecord) != 0
		&& memcmp(oracleRecord + 0x18, identity, sizeof(identity)) == 0
		&& memcmp(candidateRecord + 0x18, identity, sizeof(identity)) == 0
		&& memcmp(oracleRecord + 0x48, identity, sizeof(identity)) == 0
		&& memcmp(candidateRecord + 0x48, identity, sizeof(identity)) == 0
		&& memcmp(oracleRecord + 0x3c, "\0\0\0\0\0\0\0\0\0\0\0\0", 12) == 0
		&& memcmp(candidateRecord + 0x3c, "\0\0\0\0\0\0\0\0\0\0\0\0", 12) == 0
		&& memcmp(oracleRecord + 0x6c, "\0\0\0\0\0\0\0\0\0\0\0\0", 12) == 0
		&& memcmp(candidateRecord + 0x6c, "\0\0\0\0\0\0\0\0\0\0\0\0", 12) == 0
		&& *reinterpret_cast<unsigned*>(oracleRecord + 0x10) == 0xa5a5a5a5u
		&& *reinterpret_cast<unsigned*>(candidateRecord + 0x10) == 0xa5a5a5a5u
		&& *reinterpret_cast<unsigned*>(oracleRecord + 0x14) == 0xa5a5a5a5u
		&& *reinterpret_cast<unsigned*>(candidateRecord + 0x14) == 0xa5a5a5a5u;
	if(!returnsMatch || !ctorFieldsMatch || oracleBefore != oracleAfter
		|| candidateBefore != candidateAfter)
		++mismatches;
	for(unsigned offset = 0; offset < sizeof(oracleRecord); ++offset)
		if((offset < 4 || (offset >= 0x0c && offset < 0x10)))
			continue;
		else if(oracleRecord[offset] != candidateRecord[offset])
			++mismatches;
	void* const oracleWrapper = *reinterpret_cast<void**>(oracleRecord + 0x0c);
	void* const candidateWrapper = *reinterpret_cast<void**>(candidateRecord + 0x0c);
	if(oracleWrapper)
		{
		OracleDtor destroyOracle = reinterpret_cast<OracleDtor>(
			*reinterpret_cast<void***>(oracleRecord)[0]);
		destroyOracle(oracleRecord, 0);
		}
	if(candidateWrapper)
		delete static_cast<NpFluidEmitter*>(candidateWrapper);
	printf("fluid emitter internal ctor cases=1 identity_local=1 identity_world=1 wrapper=1 untouched=1 stack=1/1 mismatches=%u\n",
		mismatches);
	return mismatches;
	}

int wmain(int argc, wchar_t** argv)
	{
	setvbuf(stdout, 0, _IONBF, 0);
	if(argc != 5)
		{
		fprintf(stderr, "usage: NxPhysicsFluidEmitterAbiTests <staged directory> <NxPhysics.dll sha256> <oracle image path> <candidate NxPhysics.dll sha256>\n");
		return 2;
		}
	wchar_t physicsPath[MAX_PATH];
	if(swprintf_s(physicsPath, L"%s\\NxPhysics.dll", argv[1]) < 0)
		return 2;
	if(!SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_SYSTEM32) || !AddDllDirectory(argv[1]))
		return 1;
	HMODULE module = LoadLibraryExW(physicsPath, 0, LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32);
	if(!module)
		return 1;
	wchar_t loadedPath[MAX_PATH];
	char actualHash[65];
	char expectedHash[65];
	size_t converted = 0;
	if(!GetModuleFileNameW(module, loadedPath, MAX_PATH) || !nxSha256(loadedPath, actualHash)
		|| wcstombs_s(&converted, expectedHash, sizeof(expectedHash), argv[2], _TRUNCATE) != 0
		|| strcmp(actualHash, expectedHash) != 0)
		{
		fprintf(stderr, "FAIL fluid emitter probe did not load the pinned oracle\n");
		return 1;
		}
	printf("loaded module=NxPhysics.dll path=%S sha256=%s\n", loadedPath, actualHash);
	HMODULE foundationModule = GetModuleHandleW(L"NxFoundation.dll");
	wchar_t foundationPath[MAX_PATH];
	char foundationHash[65];
	if(!foundationModule
		|| !GetModuleFileNameW(foundationModule, foundationPath, MAX_PATH)
		|| !nxSha256(foundationPath, foundationHash))
		{
		fprintf(stderr, "FAIL fluid emitter probe did not resolve staged NxFoundation.dll\n");
		return 1;
		}
	printf("loaded module=NxFoundation.dll path=%S sha256=%s\n",
		foundationPath, foundationHash);
	HMODULE oracleImage = LoadLibraryExW(argv[3], 0, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
	if(!oracleImage)
		{
		fprintf(stderr, "FAIL fluid emitter probe did not load the isolated oracle image\n");
		return 1;
		}
	unsigned char* image = reinterpret_cast<unsigned char*>(oracleImage);
	NxLockSlots lockSlots = nxBindOracleLocks(image);
	if(!lockSlots.protection)
		return 1;

	unsigned char internal[0x90];
	memset(internal, 0, sizeof(internal));
	for(unsigned offset = 0; offset < sizeof(internal); offset += 4)
		*reinterpret_cast<unsigned*>(internal + offset) = 0x68000000u + offset;
	unsigned char ownerFluid[0x58] = {};
	void* const expectedOwnerFluid = reinterpret_cast<void*>(0x12345670u);
	*reinterpret_cast<void**>(ownerFluid + 0x54) = expectedOwnerFluid;
	unsigned char frameActorLink[8] = {};
	void* const expectedFrameActor = reinterpret_cast<void*>(0x12345674u);
	*reinterpret_cast<void**>(frameActorLink) = expectedFrameActor;
	*reinterpret_cast<void**>(frameActorLink + 4) = reinterpret_cast<void*>(0x12345678u);
	*reinterpret_cast<void**>(internal + 4) = ownerFluid;
	*reinterpret_cast<void**>(internal + 0x14) = frameActorLink;
	unsigned char oracleBytes[0x18];
	memset(oracleBytes, 0, sizeof(oracleBytes));
	struct CandidateLockStorage
		{
		CRITICAL_SECTION section;
		unsigned writer;
		unsigned thread;
		};
	static_assert(sizeof(CandidateLockStorage) == 0x20,
		"NpSceneGuard reads the writer/thread words through +0x1c");
	CandidateLockStorage candidateLock;
	memset(&candidateLock, 0, sizeof(candidateLock));
	InitializeCriticalSection(&candidateLock.section);
	CRITICAL_SECTION* candidateSectionPointer = &candidateLock.section;
	void* candidateSectionLink = &candidateSectionPointer;
	FluidEmitterProbe candidate(internal);
	HMODULE candidatePhysics = 0;
	void* candidateSetterAddress = 0;
	if(!nxLoadCandidateFluidEmitterSetter(&candidatePhysics, &candidateSetterAddress))
		{
		fprintf(stderr, "FAIL candidate NxPhysics.dll setFlag row could not be resolved from its map\n");
		return 1;
		}
	char expectedCandidateHash[65];
	expectedCandidateHash[0] = '\0';
	if(wcstombs(expectedCandidateHash, argv[4], sizeof(expectedCandidateHash)) == (size_t)-1
		|| strlen(expectedCandidateHash) != 64)
		{
		fprintf(stderr, "FAIL expected candidate NxPhysics.dll SHA-256 is invalid\n");
		return 1;
		}
	wchar_t candidatePath[MAX_PATH];
	char actualCandidateHash[65];
	actualCandidateHash[0] = '\0';
	if(!GetModuleFileNameW(candidatePhysics, candidatePath, MAX_PATH)
		|| !nxSha256(candidatePath, actualCandidateHash)
		|| _stricmp(actualCandidateHash, expectedCandidateHash) != 0)
		{
		fprintf(stderr, "FAIL candidate NxPhysics.dll hash mismatch expected=%s actual=%s\n",
			expectedCandidateHash, actualCandidateHash);
		return 1;
		}
	gFailures += nxRunFluidEmitterRecordConstructor(image, candidatePhysics);
	void* candidateFrictionPatchAssignAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"??4NxFrictionPatch@@QAEAAV0@ABV0@@Z", &candidateFrictionPatchAssignAddress))
		++gFailures;
	else
		gFailures += nxRunFrictionPatchAssignment(image + 0x1dc80,
			candidateFrictionPatchAssignAddress);
	void* candidateBucketPrefixAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionBucketPrefix", &candidateBucketPrefixAddress))
		++gFailures;
	else
		{
		gFailures += nxRunFluidCollisionBucketPrefixDifferential(
			image + 0x864e0, candidateBucketPrefixAddress);
		}
	void* candidateCellPrefixAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionCellPrefix", &candidateCellPrefixAddress))
		++gFailures;
	else
		gFailures += nxRunFluidCollisionCellPrefixDifferential(
			image + 0x86520, candidateCellPrefixAddress);
	void* candidateBucketHashAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionCountBuckets", &candidateBucketHashAddress))
		{
		fprintf(stderr, "FAIL candidate NxPhysics.dll collision bucket hash row could not be resolved from its map\n");
		++gFailures;
		}
	else
		gFailures += nxRunFluidCollisionBucketHashDifferential(image,
			candidateBucketHashAddress);
	void* candidateTriangleCellsAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionTriangleCells", &candidateTriangleCellsAddress))
		fprintf(stderr, "FAIL candidate NxPhysics.dll triangle-cell row could not be resolved from its map\n");
	void* candidateTriangleBuilderAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionBuildTriangleCells", &candidateTriangleBuilderAddress))
		fprintf(stderr, "FAIL candidate NxPhysics.dll triangle-builder row could not be resolved from its map\n");
	void* candidateCellHashAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionAccumulateCellCounts", &candidateCellHashAddress))
		fprintf(stderr, "FAIL candidate NxPhysics.dll cell-hash row could not be resolved from its map\n");
	void* candidateIndexedMeshAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionBuildIndexedGrid", &candidateIndexedMeshAddress))
		fprintf(stderr, "FAIL candidate NxPhysics.dll indexed-mesh build row could not be resolved from its map\n");
	void* candidateCellIndexAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionAppendCellIndex", &candidateCellIndexAddress))
		fprintf(stderr, "FAIL candidate NxPhysics.dll cell-index helper could not be resolved from its map\n");
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionBuildTriangleIndices", &gCandidateTriangleIndexBuilderAddress))
		fprintf(stderr, "FAIL candidate NxPhysics.dll triangle-index builder could not be resolved from its map\n");
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionPackShapeData", &gCandidateFluidCollisionShapePackerAddress))
		fprintf(stderr, "FAIL candidate NxPhysics.dll fluid shape-packing row could not be resolved from its map\n");
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionCountShapeType",
		&gCandidateFluidCollisionShapeCounterAddress))
		fprintf(stderr, "FAIL candidate NxPhysics.dll fluid shape-count router could not be resolved from its map\n");
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionCountSceneShapes",
		&gCandidateFluidCollisionSceneShapeCounterAddress))
		fprintf(stderr, "FAIL candidate NxPhysics.dll scene shape-count router could not be resolved from its map\n");
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionAppendSphere", &gCandidateFluidCollisionSphereAppendAddress))
		fprintf(stderr, "FAIL candidate NxPhysics.dll fluid sphere-array append row could not be resolved from its map\n");
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionAppendCapsule", &gCandidateFluidCollisionCapsuleAppendAddress))
		fprintf(stderr, "FAIL candidate NxPhysics.dll fluid capsule-array append row could not be resolved from its map\n");
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionAppendBox", &gCandidateFluidCollisionBoxAppendAddress))
		fprintf(stderr, "FAIL candidate NxPhysics.dll fluid box-array append row could not be resolved from its map\n");
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionAppendPlane", &gCandidateFluidCollisionPlaneAppendAddress))
		fprintf(stderr, "FAIL candidate NxPhysics.dll fluid plane-array append row could not be resolved from its map\n");
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionAppendMesh", &gCandidateFluidCollisionMeshAppendAddress))
		fprintf(stderr, "FAIL candidate NxPhysics.dll fluid mesh-array append row could not be resolved from its map\n");
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionShapeArrayRouter", &gCandidateFluidCollisionShapeArrayRouterAddress))
		fprintf(stderr, "FAIL candidate NxPhysics.dll fluid shape-array router could not be resolved from its map\n");
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionCollectSceneShapes",
		&gCandidateFluidCollisionSceneShapeCollectorAddress))
		fprintf(stderr, "FAIL candidate NxPhysics.dll fluid scene-shape collector could not be resolved from its map\n");
	void* candidateStaticCopyAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionStaticCopy", &candidateStaticCopyAddress))
		{
		fprintf(stderr, "FAIL candidate NxPhysics.dll static collision copy row could not be resolved from its map\n");
		++gFailures;
		}
	else
		gFailures += nxRunFluidCollisionStaticCopyDifferential(image,
			candidateStaticCopyAddress);
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxFluidCollisionBuildStaticCollision",
		&gCandidateFluidCollisionStaticRebuildAddress))
	{
		fprintf(stderr, "FAIL candidate NxPhysics.dll static-collision rebuild row could not be resolved from its map\n");
		++gFailures;
	}
	void* candidateNotifyActorReleasedAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxNpFluidManagerNotifyReleasedActor", &candidateNotifyActorReleasedAddress))
		{
		fprintf(stderr, "FAIL candidate NxPhysics.dll fluid-manager actor-release row could not be resolved from its map\n");
		return 1;
		}
	void* candidateSetActorAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxNpFluidEmitterSetActor", &candidateSetActorAddress))
		{
		fprintf(stderr, "FAIL candidate NxPhysics.dll fluid-emitter actor binding row could not be resolved from its map\n");
		return 1;
		}
	void* candidateUpdatePoseAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxNpFluidEmitterUpdateGlobalPose", &candidateUpdatePoseAddress))
		{
		fprintf(stderr, "FAIL candidate NxPhysics.dll fluid-emitter world-pose row could not be resolved from its map\n");
		++gFailures;
		}
	void* candidateUpdateBodyStateAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxNpFluidEmitterUpdateBodyState", &candidateUpdateBodyStateAddress))
		{
		fprintf(stderr, "FAIL candidate NxPhysics.dll fluid-emitter body-state update row could not be resolved from its map\n");
		++gFailures;
		}
	void* candidateGenerateSurfaceMeshAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxNpFluidEmitterGenerateSurfaceMesh", &candidateGenerateSurfaceMeshAddress))
		{
		fprintf(stderr, "FAIL candidate NxPhysics.dll fluid surface-mesh dispatch row could not be resolved from its map\n");
		++gFailures;
		}
	void* candidateFluidManagerGenerateSurfaceMeshesAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"?nxFluidManagerGenerateSurfaceMeshesDisabled@@YAXPAX@Z",
		&candidateFluidManagerGenerateSurfaceMeshesAddress))
		{
		fprintf(stderr, "FAIL candidate NxPhysics.dll FluidManager surface-mesh traversal row could not be resolved from its map\n");
		++gFailures;
		}
	void* candidateSetMeshDataAddress = 0;
	static const char candidateSetMeshDataName[] =
		"?setMeshData@NpImplicitMeshDataRecord@@QAEHAAVNxMeshData@@@Z";
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		candidateSetMeshDataName, &candidateSetMeshDataAddress))
		{
		fprintf(stderr, "FAIL candidate NxPhysics.dll implicit-mesh data helper could not be resolved from its map\n");
		++gFailures;
		}
	void* candidateSetMeshDataWrapperAddress = 0;
	static const char candidateSetMeshDataWrapperName[] =
		"?setMeshData@NpImplicitMesh@@UAEXAAVNxMeshData@@@Z";
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		candidateSetMeshDataWrapperName, &candidateSetMeshDataWrapperAddress))
		{
		fprintf(stderr, "FAIL candidate NxPhysics.dll public implicit-mesh wrapper could not be resolved from its map\n");
		++gFailures;
		}
	void* candidateApiReentryGetter = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxApiReentryIsActive", &candidateApiReentryGetter))
		fprintf(stderr, "FAIL candidate NxPhysics.dll shared API reentry getter could not be resolved\n");
	void* candidateApiReentrySetter = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxSetApiReentryActive", &candidateApiReentrySetter))
		fprintf(stderr, "FAIL candidate NxPhysics.dll shared API reentry state could not be resolved\n");
	static const char candidateCtorName[] = "??0NpFluidEmitter@@QAE@PAX@Z";
	void* candidateCtorAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics, candidateCtorName,
		&candidateCtorAddress))
		{
		fprintf(stderr, "FAIL candidate NxPhysics.dll fluid-emitter constructor could not be resolved from its map\n");
		return 1;
		}
	typedef void* (__thiscall* CandidateCtor)(void*, void*);
	CandidateCtor candidateCtor = reinterpret_cast<CandidateCtor>(candidateCtorAddress);
	typedef void* (__thiscall* FluidEmitterCtor)(void*, void*);
	FluidEmitterCtor oracleCtor = reinterpret_cast<FluidEmitterCtor>(image + 0x8c2d0);
	if(oracleCtor(oracleBytes, internal) != oracleBytes)
		++gFailures;
	unsigned char candidateDllBytes[0x18];
	memset(candidateDllBytes, 0, sizeof(candidateDllBytes));
	if(candidateCtor(candidateDllBytes, internal) != candidateDllBytes)
		++gFailures;
	NxFluidEmitter* candidateDllInterface = reinterpret_cast<NxFluidEmitter*>(candidateDllBytes);
	if(!nxCandidateAddressBelongsToModule(candidatePhysics,
		*reinterpret_cast<void**>(candidateDllBytes)))
		{
		fprintf(stderr, "FAIL candidate fluid-emitter primary vptr is outside candidate NxPhysics.dll\n");
		++gFailures;
		}
	unsigned dllCtorMismatches = 0;
	for(unsigned offset = 4; offset < sizeof(candidateDllBytes); offset += 4)
		if(offset != 8 && memcmp(oracleBytes + offset, candidateDllBytes + offset, 4) != 0)
			++dllCtorMismatches;
	if(!*reinterpret_cast<void**>(candidateDllBytes + 8))
		++dllCtorMismatches;
	gFailures += dllCtorMismatches;
	printf("fluid emitter candidate_dll ctor size=24 vptr_in_module=1 mismatches=%u\n",
		dllCtorMismatches);
	unsigned char oracleLockData[0x20];
	unsigned char oracleLockObject[4];
	memset(oracleLockData, 0, sizeof(oracleLockData));
	*reinterpret_cast<unsigned*>(oracleLockData + 0x18) = 1;
	*reinterpret_cast<unsigned*>(oracleLockData + 0x1c) = 0x2222;
	void* oracleLockDataPointer = oracleLockData;
	memcpy(oracleLockObject, &oracleLockDataPointer, sizeof(oracleLockDataPointer));
	void* oracleLockLink = oracleLockObject;
	NxFluidEmitter* oracle = reinterpret_cast<NxFluidEmitter*>(oracleBytes);
	unsigned ctorMismatches = 0;
	for(unsigned offset = 4; offset < 0x18; offset += 4)
		if(offset != 8 && memcmp(oracleBytes + offset,
			reinterpret_cast<unsigned char*>(&candidate) + offset, 4) != 0)
			++ctorMismatches;
	if(!*reinterpret_cast<void**>(oracleBytes + 8)
		|| !*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(&candidate) + 8))
		++ctorMismatches;
	gFailures += ctorMismatches;
	printf("fluid emitter ctor size=24 secondary_vptr_nonnull=1 internal=1 mismatches=%u\n", ctorMismatches);
	memcpy(oracleBytes + 0x10, &oracleLockLink, sizeof(oracleLockLink));
	memcpy(candidateDllBytes + 0x10, &candidateSectionLink, sizeof(candidateSectionLink));
	memcpy(reinterpret_cast<unsigned char*>(&candidate) + 0x10,
		&candidateSectionLink, sizeof(candidateSectionLink));
	memcpy(oracleBytes + 0x0c, &oracleLockLink, sizeof(oracleLockLink));
	memcpy(candidateDllBytes + 0x0c, &candidateSectionLink, sizeof(candidateSectionLink));
	memcpy(reinterpret_cast<unsigned char*>(&candidate) + 0x0c,
		&candidateSectionLink, sizeof(candidateSectionLink));
	NxFluidEmitter* candidateInterface = &candidate;
	const NxFluid* oracleOwnerFluid = &oracle->getFluid();
	const NxFluid* candidateOwnerFluid = &candidateInterface->getFluid();
	const NxFluid* candidateDllOwnerFluid = &candidateDllInterface->getFluid();
	NxActor* const oracleFrameActor = oracle->getFrameActor();
	NxActor* const candidateFrameActor = candidateInterface->getFrameActor();
	NxActor* const candidateDllFrameActor = candidateDllInterface->getFrameActor();
	const unsigned ownerFrameMismatches =
		(oracleOwnerFluid != expectedOwnerFluid)
		+ (candidateOwnerFluid != oracleOwnerFluid)
		+ (candidateDllOwnerFluid != oracleOwnerFluid)
		+ (oracleFrameActor != expectedFrameActor)
		+ (candidateFrameActor != oracleFrameActor)
		+ (candidateDllFrameActor != oracleFrameActor);
	gFailures += ownerFrameMismatches;
	printf("fluid emitter owner/frame cases=2 oracle_owner=%u candidate_owner=%u dll_owner=%u "
		"oracle_frame_actor=%u candidate_frame_actor=%u dll_frame_actor=%u mismatches=%u\n",
		oracleOwnerFluid == expectedOwnerFluid,
		candidateOwnerFluid == oracleOwnerFluid,
		candidateDllOwnerFluid == oracleOwnerFluid,
		oracleFrameActor == expectedFrameActor,
		candidateFrameActor == oracleFrameActor,
		candidateDllFrameActor == oracleFrameActor,
		ownerFrameMismatches);
	gFailures += nxRunFluidEmitterNameRows(image, oracle, candidateInterface,
		candidateDllInterface, internal);
	struct AggregateSlot { const char* name; unsigned slot; unsigned bytes; };
	static const AggregateSlot aggregateSlots[] = {
		{ "global_pose", 5, 0x30 }, { "global_position", 6, 0x0c },
		{ "global_orientation", 7, 0x24 }, { "local_pose", 11, 0x30 },
		{ "local_position", 12, 0x0c }, { "local_orientation", 13, 0x24 },
		};
	unsigned rawMismatches = 0;
	unsigned rawCases = 0;
	unsigned rawRetptr = 0;
	unsigned rawStackBalanced = 0;
	unsigned dllRawMismatches = 0;
	unsigned dllRawCases = 0;
	unsigned dllRawRetptr = 0;
	unsigned dllRawStackBalanced = 0;
	unsigned localCopyCases = 0;
	unsigned localCopyRetptr = 0;
	unsigned localCopyStackBalanced = 0;
	unsigned localCopyDllCases = 0;
	unsigned localCopyDllRetptr = 0;
	unsigned localCopyDllStackBalanced = 0;
	unsigned localCopyMismatches = 0;
	for(unsigned i = 0; i < sizeof(aggregateSlots) / sizeof(aggregateSlots[0]); ++i)
		{
		const bool localCopyRow = i >= 3;
		unsigned char oracleOut[0x30];
		unsigned char candidateOut[0x30];
		memset(oracleOut, 0, sizeof(oracleOut));
		memset(candidateOut, 0, sizeof(candidateOut));
		void* oracleResult = 0;
		unsigned oracleStackDelta = 0xffffffffu;
		if(!nxTryFluidEmitterCall(oracle, aggregateSlots[i].slot, oracleOut,
			&oracleResult, &oracleStackDelta))
			{
			++rawMismatches;
			continue;
			}
		void* candidateResult = 0;
		unsigned candidateStackDelta = 0xffffffffu;
		if(!nxTryFluidEmitterCall(candidateInterface, aggregateSlots[i].slot, candidateOut,
			&candidateResult, &candidateStackDelta))
			{
			++rawMismatches;
			continue;
			}
		++rawCases;
		if(oracleResult == oracleOut && candidateResult == candidateOut)
			++rawRetptr;
		if(oracleStackDelta == 0 && candidateStackDelta == 0)
			++rawStackBalanced;
		if(localCopyRow)
			{
			++localCopyCases;
			if(oracleResult == oracleOut && candidateResult == candidateOut)
				++localCopyRetptr;
			if(oracleStackDelta == 0 && candidateStackDelta == 0)
				++localCopyStackBalanced;
			}
		if(oracleResult != oracleOut || candidateResult != candidateOut
			|| oracleStackDelta != 0 || candidateStackDelta != 0
			|| memcmp(oracleOut, candidateOut, aggregateSlots[i].bytes) != 0)
			{
			if(localCopyRow)
				++localCopyMismatches;
			fprintf(stderr, "fluid emitter raw ABI mismatch: %s oracle_ret=%u candidate_ret=%u stack=%u/%u\n",
				aggregateSlots[i].name, oracleResult == oracleOut ? 1u : 0u,
				candidateResult == candidateOut ? 1u : 0u, oracleStackDelta, candidateStackDelta);
			++rawMismatches;
			}
		unsigned char candidateDllOut[0x30];
		memset(candidateDllOut, 0, sizeof(candidateDllOut));
		void* candidateDllResult = 0;
		unsigned candidateDllStackDelta = 0xffffffffu;
		if(!nxTryFluidEmitterCall(candidateDllInterface, aggregateSlots[i].slot,
			candidateDllOut, &candidateDllResult, &candidateDllStackDelta))
			{
			++dllRawMismatches;
			if(localCopyRow)
				++localCopyMismatches;
			}
		else
			{
			++dllRawCases;
			if(candidateDllResult == candidateDllOut)
				++dllRawRetptr;
			if(candidateDllStackDelta == 0)
				++dllRawStackBalanced;
			if(localCopyRow)
				{
				++localCopyDllCases;
				if(candidateDllResult == candidateDllOut)
					++localCopyDllRetptr;
				if(candidateDllStackDelta == 0)
					++localCopyDllStackBalanced;
				}
			if(candidateDllResult != candidateDllOut || candidateDllStackDelta != 0
				|| memcmp(oracleOut, candidateDllOut, aggregateSlots[i].bytes) != 0)
				{
				if(localCopyRow)
					++localCopyMismatches;
				fprintf(stderr, "candidate DLL fluid emitter raw ABI mismatch: %s retptr=%u stack=%u\n",
					aggregateSlots[i].name, candidateDllResult == candidateDllOut ? 1u : 0u,
					candidateDllStackDelta);
				++dllRawMismatches;
				}
			}
		}
	gFailures += rawMismatches;
	printf("fluid emitter raw_abi cases=%u retptr=%u stack_balanced=%u mismatches=%u\n",
		rawCases, rawRetptr, rawStackBalanced, rawMismatches);
	gFailures += dllRawMismatches;
	printf("fluid emitter candidate_dll raw_abi cases=%u retptr=%u stack_balanced=%u mismatches=%u\n",
		dllRawCases, dllRawRetptr, dllRawStackBalanced, dllRawMismatches);
	printf("fluid emitter local pose-copy rows=3 candidate_cases=%u candidate_retptr=%u candidate_stack=%u dll_cases=%u dll_retptr=%u dll_stack=%u mismatches=%u\n",
		localCopyCases, localCopyRetptr, localCopyStackBalanced,
		localCopyDllCases, localCopyDllRetptr, localCopyDllStackBalanced,
		localCopyMismatches);

	NxMat34 oracleGlobalPose;
	oracleGlobalPose = oracle->getGlobalPoseVal();
	NxMat34 candidateGlobalPose = candidateInterface->getGlobalPoseVal();
	nxCheckEqual("global_pose", &oracleGlobalPose, &candidateGlobalPose, sizeof(oracleGlobalPose));
	NxMat34 candidateDllGlobalPose = candidateDllInterface->getGlobalPoseVal();
	nxCheckEqual("candidate_dll_global_pose", &oracleGlobalPose, &candidateDllGlobalPose, sizeof(oracleGlobalPose));
	NxVec3 oracleGlobalPosition;
	oracleGlobalPosition = oracle->getGlobalPositionVal();
	NxVec3 candidateGlobalPosition = candidateInterface->getGlobalPositionVal();
	nxCheckEqual("global_position", &oracleGlobalPosition, &candidateGlobalPosition, sizeof(oracleGlobalPosition));
	NxVec3 candidateDllGlobalPosition = candidateDllInterface->getGlobalPositionVal();
	nxCheckEqual("candidate_dll_global_position", &oracleGlobalPosition, &candidateDllGlobalPosition, sizeof(oracleGlobalPosition));
	NxMat33 oracleGlobalOrientation;
	oracleGlobalOrientation = oracle->getGlobalOrientationVal();
	NxMat33 candidateGlobalOrientation = candidateInterface->getGlobalOrientationVal();
	nxCheckEqual("global_orientation", &oracleGlobalOrientation, &candidateGlobalOrientation, sizeof(oracleGlobalOrientation));
	NxMat33 candidateDllGlobalOrientation = candidateDllInterface->getGlobalOrientationVal();
	nxCheckEqual("candidate_dll_global_orientation", &oracleGlobalOrientation, &candidateDllGlobalOrientation, sizeof(oracleGlobalOrientation));
	NxMat34 oracleLocalPose;
	oracleLocalPose = oracle->getLocalPoseVal();
	NxMat34 candidateLocalPose = candidateInterface->getLocalPoseVal();
	nxCheckEqual("local_pose", &oracleLocalPose, &candidateLocalPose, sizeof(oracleLocalPose));
	NxMat34 candidateDllLocalPose = candidateDllInterface->getLocalPoseVal();
	nxCheckEqual("candidate_dll_local_pose", &oracleLocalPose, &candidateDllLocalPose, sizeof(oracleLocalPose));
	NxVec3 oracleLocalPosition;
	oracleLocalPosition = oracle->getLocalPositionVal();
	NxVec3 candidateLocalPosition = candidateInterface->getLocalPositionVal();
	nxCheckEqual("local_position", &oracleLocalPosition, &candidateLocalPosition, sizeof(oracleLocalPosition));
	NxVec3 candidateDllLocalPosition = candidateDllInterface->getLocalPositionVal();
	nxCheckEqual("candidate_dll_local_position", &oracleLocalPosition, &candidateDllLocalPosition, sizeof(oracleLocalPosition));
	NxMat33 oracleLocalOrientation;
	oracleLocalOrientation = oracle->getLocalOrientationVal();
	NxMat33 candidateLocalOrientation = candidateInterface->getLocalOrientationVal();
	nxCheckEqual("local_orientation", &oracleLocalOrientation, &candidateLocalOrientation, sizeof(oracleLocalOrientation));
	NxMat33 candidateDllLocalOrientation = candidateDllInterface->getLocalOrientationVal();
	nxCheckEqual("candidate_dll_local_orientation", &oracleLocalOrientation, &candidateDllLocalOrientation, sizeof(oracleLocalOrientation));

	unsigned char oracleFlagInternal[sizeof(internal)];
	unsigned char candidateFlagInternal[sizeof(internal)];
	memcpy(oracleFlagInternal, internal, sizeof(internal));
	memcpy(candidateFlagInternal, internal, sizeof(internal));
	*reinterpret_cast<unsigned*>(oracleFlagInternal + 0x10) = 0x80000100u;
	*reinterpret_cast<unsigned*>(candidateFlagInternal + 0x10) = 0x80000100u;
	unsigned char oracleFlagBytes[0x18];
	memset(oracleFlagBytes, 0, sizeof(oracleFlagBytes));
	FluidEmitterProbe candidateFlag(candidateFlagInternal);
	if(oracleCtor(oracleFlagBytes, oracleFlagInternal) != oracleFlagBytes)
		++gFailures;
	memcpy(oracleFlagBytes + 0x0c, &oracleLockLink, sizeof(oracleLockLink));
	memcpy(oracleFlagBytes + 0x10, &oracleLockLink, sizeof(oracleLockLink));
	memcpy(reinterpret_cast<unsigned char*>(&candidateFlag) + 0x0c,
		&candidateSectionLink, sizeof(candidateSectionLink));
	memcpy(reinterpret_cast<unsigned char*>(&candidateFlag) + 0x10,
		&candidateSectionLink, sizeof(candidateSectionLink));
	NxFluidEmitter* oracleFlag = reinterpret_cast<NxFluidEmitter*>(oracleFlagBytes);
	NxFluidEmitter* candidateFlagInterface = &candidateFlag;
	typedef void (__thiscall *CandidateSetFlagFn)(void*, NxFluidEmitterFlag, bool);
	CandidateSetFlagFn candidateSetFlag = reinterpret_cast<CandidateSetFlagFn>(candidateSetterAddress);
	unsigned flagMismatches = 0;
	oracleFlag->setFlag(NX_FEF_VISUALIZATION, true);
	candidateSetFlag(&candidateFlag, NX_FEF_VISUALIZATION, true);
	if(*reinterpret_cast<unsigned*>(oracleFlagInternal + 0x10) != 0x80000101u
		|| *reinterpret_cast<unsigned*>(candidateFlagInternal + 0x10) != 0x80000101u
		|| oracleFlag->getFlag(NX_FEF_VISUALIZATION) != NX_FEF_VISUALIZATION
		|| candidateFlagInterface->getFlag(NX_FEF_VISUALIZATION) != NX_FEF_VISUALIZATION)
		++flagMismatches;
	oracleFlag->setFlag(NX_FEF_VISUALIZATION, false);
	candidateSetFlag(&candidateFlag, NX_FEF_VISUALIZATION, false);
	if(*reinterpret_cast<unsigned*>(oracleFlagInternal + 0x10) != 0x80000100u
		|| *reinterpret_cast<unsigned*>(candidateFlagInternal + 0x10) != 0x80000100u
		|| oracleFlag->getFlag(NX_FEF_VISUALIZATION) != 0
		|| candidateFlagInterface->getFlag(NX_FEF_VISUALIZATION) != 0)
		++flagMismatches;
	gFailures += flagMismatches;
	printf("fluid emitter flags cases=2 mismatches=%u\n", flagMismatches);

	HMODULE fluidModel = 0;
	FARPROC resetFluidCallbacks = 0;
	FARPROC getFluidCallbackCount = 0;
	FARPROC getFluidCallbackRecord = 0;
	if(!nxLoadFluidModelCallbackFixture(&fluidModel, &resetFluidCallbacks,
		&getFluidCallbackCount, &getFluidCallbackRecord))
		{
		fprintf(stderr, "FAIL FluidModel flag callback fixture failed to load\n");
		return 1;
		}
	void* candidateManagerStepAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"@nxFluidManagerStep@12", &candidateManagerStepAddress))
		{
		fprintf(stderr, "FAIL candidate FluidManager step row could not be resolved from its map\n");
		++gFailures;
		}
	if(gCandidateFluidCollisionStaticRebuildAddress)
		gFailures += nxRunFluidCollisionStaticRebuildDifferential(image,
			gCandidateFluidCollisionStaticRebuildAddress,
			candidateManagerStepAddress, fluidModel);
	typedef void (__cdecl *ResetCallbacksFn)();
	typedef unsigned (__cdecl *CallbackCountFn)();
	typedef int (__cdecl *GetCallbackRecordFn)(unsigned, NxFluidEmitterFlagCallbackRecord*);
	ResetCallbacksFn resetCallbacks = reinterpret_cast<ResetCallbacksFn>(resetFluidCallbacks);
	CallbackCountFn callbackCount = reinterpret_cast<CallbackCountFn>(getFluidCallbackCount);
	GetCallbackRecordFn getCallbackRecord = reinterpret_cast<GetCallbackRecordFn>(getFluidCallbackRecord);
	typedef void (__cdecl *ResetPostStepFn)();
	typedef unsigned (__cdecl *PostStepCountFn)();
	typedef int (__cdecl *GetPostStepRecordFn)(unsigned, NxFluidPostStepCallbackRecord*);
	typedef void (__cdecl *SetPostStepParticleFn)(unsigned, const unsigned*, unsigned);
	ResetPostStepFn resetPostStep = reinterpret_cast<ResetPostStepFn>(
		GetProcAddress(fluidModel, "NxResetFluidPostStepCallbacks"));
	PostStepCountFn postStepCount = reinterpret_cast<PostStepCountFn>(
		GetProcAddress(fluidModel, "NxGetFluidPostStepCallbackCount"));
	GetPostStepRecordFn getPostStepRecord = reinterpret_cast<GetPostStepRecordFn>(
		GetProcAddress(fluidModel, "NxGetFluidPostStepCallbackRecord"));
	SetPostStepParticleFn setPostStepParticle = reinterpret_cast<SetPostStepParticleFn>(
		GetProcAddress(fluidModel, "NxSetFluidPostStepParticle"));
	if(!resetPostStep || !postStepCount || !getPostStepRecord || !setPostStepParticle)
		{
		fprintf(stderr, "FAIL FluidModel callback fixture lacks post-step probes\n");
		return 1;
		}
	FARPROC triangleBoxOverlap = GetProcAddress(fluidModel, "triBoxOverlap");
	FARPROC triangleCellGrow = GetProcAddress(fluidModel, "Int16Vector3ArrayGrow");
	if(!triangleBoxOverlap || !triangleCellGrow)
		{
		fprintf(stderr, "FAIL FluidModel fixture lacks triangle-cell reference exports\n");
		return 1;
		}
	void* oldCallbackValues[20] = {};
	DWORD callbackPageProtection = 0;
	if(!nxBindOracleEmitterFlagCallbacks(image, oldCallbackValues,
		&callbackPageProtection, reinterpret_cast<void*>(triangleBoxOverlap),
		reinterpret_cast<void*>(triangleCellGrow)))
		{
		fprintf(stderr, "FAIL oracle FluidModel callback slots could not be bound\n");
		return 1;
		}
	gFailures += nxRunFluidEmitterAttributeRows(image, fluidModel, candidatePhysics);
	void* candidateStoreGlobalPoseAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxNpFluidEmitterStoreGlobalPose", &candidateStoreGlobalPoseAddress))
		++gFailures;
	void* candidateCopyGlobalPoseAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxNpFluidEmitterCopyGlobalPose", &candidateCopyGlobalPoseAddress))
		++gFailures;
	gFailures += nxRunFluidEmitterGlobalPoseRows(image + 0x87ce0,
		candidateStoreGlobalPoseAddress, image + 0x87d80,
		candidateCopyGlobalPoseAddress, fluidModel, resetCallbacks,
		callbackCount, getCallbackRecord);
	void* candidateSetGlobalPoseInternalAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxNpFluidEmitterSetGlobalPoseInternal", &candidateSetGlobalPoseInternalAddress))
		++gFailures;
	gFailures += nxRunFluidEmitterSetGlobalPoseInternal(image + 0x89410,
		candidateSetGlobalPoseInternalAddress, fluidModel, resetCallbacks,
		callbackCount, getCallbackRecord);
	void* candidateSetLocalPoseInternalAddress = 0;
	if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
		"nxNpFluidEmitterSetLocalPoseInternal", &candidateSetLocalPoseInternalAddress))
		++gFailures;
	typedef void (__cdecl *SetFixturePoseFn)(const float*);
	SetFixturePoseFn setLocalFixturePose = reinterpret_cast<SetFixturePoseFn>(
		GetProcAddress(fluidModel, "NxSetEmitterGlobalPose"));
	gFailures += nxRunFluidEmitterSetLocalPoseInternal(image + 0x89850,
		candidateSetLocalPoseInternalAddress, setLocalFixturePose, resetCallbacks,
		callbackCount, getCallbackRecord);
	gFailures += nxRunFluidCollisionTriangleCellsDifferential(image,
		candidateTriangleCellsAddress);
	gFailures += nxRunFluidCollisionTriangleBuilderDifferential(image,
		candidateTriangleBuilderAddress);
	gFailures += nxRunFluidCollisionCellHashDifferential(image,
		candidateCellHashAddress);
	gFailures += nxRunFluidCollisionIndexedMeshDifferential(image,
		candidateIndexedMeshAddress);
	gFailures += nxRunFluidCollisionCellIndexDifferential(image,
		candidateCellIndexAddress);
	gFailures += nxRunFluidCollisionTriangleIndexBuilderDifferential(image,
		gCandidateTriangleIndexBuilderAddress);
	gFailures += nxRunFluidCollisionShapePackerDifferential(image,
		gCandidateFluidCollisionShapePackerAddress);
	gFailures += nxRunFluidCollisionShapeCounterDifferential(image,
		gCandidateFluidCollisionShapeCounterAddress);
	gFailures += nxRunFluidCollisionSceneShapeCounterDifferential(image,
		gCandidateFluidCollisionSceneShapeCounterAddress);
	gFailures += nxRunFluidCollisionSphereAppendDifferential(image,
		gCandidateFluidCollisionSphereAppendAddress);
	gFailures += nxRunFluidCollisionCapsuleAppendDifferential(image,
		gCandidateFluidCollisionCapsuleAppendAddress);
	gFailures += nxRunFluidCollisionBoxAppendDifferential(image,
		gCandidateFluidCollisionBoxAppendAddress);
	gFailures += nxRunFluidCollisionPlaneAppendDifferential(image,
		gCandidateFluidCollisionPlaneAppendAddress);
	gFailures += nxRunFluidCollisionMeshAppendDifferential(image,
		gCandidateFluidCollisionMeshAppendAddress);
	gFailures += nxRunFluidCollisionShapeArrayRouterDifferential(image,
		gCandidateFluidCollisionShapeArrayRouterAddress);
	gFailures += nxRunFluidCollisionSceneShapeCollectorDifferential(image,
		gCandidateFluidCollisionSceneShapeCollectorAddress);
	FARPROC resetFluidRun = GetProcAddress(fluidModel, "NxResetFluidRunCallbacks");
	FARPROC getFluidRunCount = GetProcAddress(fluidModel, "NxGetFluidRunCallbackCount");
	FARPROC getFluidRunRecord = GetProcAddress(fluidModel, "NxGetFluidRunCallbackRecord");
	FARPROC fluidRunCallback = GetProcAddress(fluidModel, "FluidRun");
	if(!resetFluidRun || !getFluidRunCount || !getFluidRunRecord || !fluidRunCallback)
		{
		fprintf(stderr, "FAIL FluidModel fixture lacks FluidRun callback probes\n");
		return 1;
		}
	*reinterpret_cast<void**>(image + 0x1263c0) = reinterpret_cast<void*>(fluidRunCallback);
	typedef void (__cdecl *ResetFluidRunFn)();
	typedef unsigned (__cdecl *FluidRunCountFn)();
	typedef int (__cdecl *GetFluidRunRecordFn)(unsigned, NxFluidRunCallbackRecord*);
	ResetFluidRunFn resetRun = reinterpret_cast<ResetFluidRunFn>(resetFluidRun);
	FluidRunCountFn fluidRunCount = reinterpret_cast<FluidRunCountFn>(getFluidRunCount);
	GetFluidRunRecordFn getFluidRun = reinterpret_cast<GetFluidRunRecordFn>(getFluidRunRecord);
	FARPROC resetFluidRepulsion = GetProcAddress(fluidModel,
		"NxResetFluidRepulsionCallbacks");
	FARPROC getFluidRepulsionCount = GetProcAddress(fluidModel,
		"NxGetFluidRepulsionCallbackCount");
	FARPROC getFluidRepulsionRecord = GetProcAddress(fluidModel,
		"NxGetFluidRepulsionCallbackRecord");
	if(!resetFluidRepulsion || !getFluidRepulsionCount || !getFluidRepulsionRecord)
		{
		fprintf(stderr, "FAIL FluidModel fixture lacks body-repulsion callback probes\n");
		return 1;
		}
	typedef void (__cdecl *ResetRepulsionFn)();
	typedef unsigned (__cdecl *RepulsionCountFn)();
	typedef int (__cdecl *GetRepulsionRecordFn)(unsigned, NxFluidRepulsionCallbackRecord*);
	ResetRepulsionFn resetRepulsion = reinterpret_cast<ResetRepulsionFn>(resetFluidRepulsion);
	RepulsionCountFn repulsionCount = reinterpret_cast<RepulsionCountFn>(getFluidRepulsionCount);
	GetRepulsionRecordFn getRepulsionRecord = reinterpret_cast<GetRepulsionRecordFn>(
		getFluidRepulsionRecord);
	{
		void* candidateRepulsionAddress = 0;
		if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
			"?nxNpFluidEmitterApplyBodyRepulsion@@YAHPAX@Z",
			&candidateRepulsionAddress))
			{
			fprintf(stderr, "FAIL candidate body-repulsion row could not be resolved from its map\n");
			++gFailures;
			}
		else
			{
			unsigned char oracleFluid[0x84] = {}, candidateFluid[0x84] = {};
			unsigned char oracleBackend[0x34] = {}, candidateBackend[0x34] = {};
			unsigned char oracleEmitter[0x18] = {}, candidateEmitter[0x18] = {};
			unsigned char oracleActor[0x0c] = {}, candidateActor[0x0c] = {};
			*reinterpret_cast<void**>(oracleFluid + 0x7c) = oracleBackend;
			*reinterpret_cast<void**>(candidateFluid + 0x7c) = candidateBackend;
			*reinterpret_cast<unsigned*>(oracleFluid + 0x80) = 0x55667788u;
			*reinterpret_cast<unsigned*>(candidateFluid + 0x80) = 0x55667788u;
			*reinterpret_cast<unsigned*>(oracleBackend + 0x30) = 0x10203040u;
			*reinterpret_cast<unsigned*>(candidateBackend + 0x30) = 0x10203040u;
			*reinterpret_cast<void**>(oracleEmitter + 4) = oracleFluid;
			*reinterpret_cast<void**>(candidateEmitter + 4) = candidateFluid;
			*reinterpret_cast<unsigned*>(oracleEmitter + 8) = 0x11223344u;
			*reinterpret_cast<unsigned*>(candidateEmitter + 8) = 0x11223344u;
			*reinterpret_cast<unsigned*>(oracleEmitter + 0x10) = 4;
			*reinterpret_cast<unsigned*>(candidateEmitter + 0x10) = 4;
			*reinterpret_cast<void**>(oracleEmitter + 0x14) = oracleActor;
			*reinterpret_cast<void**>(candidateEmitter + 0x14) = candidateActor;
			resetRepulsion();
			gOracleFluidRepulsionRecordCount = 0;
			unsigned oracleEspBefore = 0, oracleEspAfter = 0;
			const int oracleResult = nxCallOracleFluidRepulsion(image + 0x881c0,
				oracleEmitter, &oracleEspBefore, &oracleEspAfter);
			NxFluidRepulsionCallbackRecord oracleRecords[3] = {};
			bool oracleRecordsOk = gOracleFluidRepulsionRecordCount == 3;
			if(oracleRecordsOk)
				memcpy(oracleRecords, gOracleFluidRepulsionRecords, sizeof(oracleRecords));
			resetRepulsion();
			unsigned candidateEspBefore = 0, candidateEspAfter = 0;
			const int candidateResult = nxCallCandidateFluidRepulsion(
				candidateRepulsionAddress, candidateEmitter,
				&candidateEspBefore, &candidateEspAfter);
			NxFluidRepulsionCallbackRecord candidateRecords[3] = {};
			bool candidateRecordsOk = repulsionCount() == 3;
			for(unsigned i = 0; i != 3 && candidateRecordsOk; ++i)
				candidateRecordsOk = getRepulsionRecord(i, &candidateRecords[i]);
			const unsigned expectedVectors[6] = {
				0x3e800001u, 0xbf000002u, 0x3f400003u,
				0x40000004u, 0xc0400005u, 0x40800006u };
			const unsigned expectedKinds[3] = { 1, 2, 3 };
			unsigned callbackMismatches = !oracleRecordsOk || !candidateRecordsOk;
			for(unsigned i = 0; i != 3 && oracleRecordsOk && candidateRecordsOk; ++i)
				{
				const NxFluidRepulsionCallbackRecord& expected = oracleRecords[i];
				const NxFluidRepulsionCallbackRecord& actual = candidateRecords[i];
				if(expected.kind != expectedKinds[i] || actual.kind != expected.kind
					|| expected.system != 0x10203040u || actual.system != expected.system
					|| expected.fluid != (i == 2 ? 0x55667788u : 0x55667788u)
					|| actual.fluid != expected.fluid
					|| expected.emitter != (i == 2 ? 0u : 0x11223344u)
					|| actual.emitter != expected.emitter
					|| (i == 1 && memcmp(expected.vectors, expectedVectors,
						sizeof(expectedVectors)) != 0)
					|| (i == 1 && memcmp(actual.vectors, expected.vectors,
						sizeof(actual.vectors)) != 0)
					|| (i == 2 && (expected.mass != 0x3f2aaaabu
						|| actual.mass != expected.mass)))
					++callbackMismatches;
				}
			const unsigned mismatches = callbackMismatches
				+ (oracleResult != candidateResult)
				+ (oracleEspBefore != oracleEspAfter)
				+ (candidateEspBefore != candidateEspAfter);
			gFailures += mismatches;
			printf("fluid emitter body repulsion bodyless actor=1 callbacks=%u/%u order=%u "
				"vectors=%u mass=%u stack=%u/%u mismatches=%u\n",
				gOracleFluidRepulsionRecordCount, repulsionCount(),
				callbackMismatches == 0, callbackMismatches == 0,
				callbackMismatches == 0,
				oracleEspBefore == oracleEspAfter,
				candidateEspBefore == candidateEspAfter, mismatches);
			}
	}
	{
		void* candidateStepAddress = 0;
		if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
			"@nxNpFluidStep@12", &candidateStepAddress))
			{
			fprintf(stderr, "FAIL candidate Fluid step row could not be resolved from its map\n");
			++gFailures;
		}
		else
			{
			typedef int (__thiscall *FluidStep)(void*, float);
			FluidStep oracleStep = reinterpret_cast<FluidStep>(image + 0x85a40);
			FluidStep candidateStep = reinterpret_cast<FluidStep>(candidateStepAddress);
			unsigned char oracleFluid[0x84] = {}, candidateFluid[0x84] = {};
			unsigned char oracleBackend[0x534] = {}, candidateBackend[0x534] = {};
			unsigned char oracleState[8] = {}, candidateState[8] = {};
			unsigned char oracleCore[0x530] = {}, candidateCore[0x530] = {};
			const unsigned expectedVector[3] = { 0x3e800001u, 0xbf000002u, 0x3f400003u };
			memcpy(oracleCore + 0x520, expectedVector, sizeof(expectedVector));
			memcpy(candidateCore + 0x520, expectedVector, sizeof(expectedVector));
			*reinterpret_cast<void**>(oracleBackend + 0x24) = oracleCore;
			*reinterpret_cast<void**>(candidateBackend + 0x24) = candidateCore;
			*reinterpret_cast<unsigned*>(oracleBackend + 0x30) = 0x10203040u;
			*reinterpret_cast<unsigned*>(candidateBackend + 0x30) = 0x10203040u;
			*reinterpret_cast<void**>(oracleFluid + 4) = 0;
			*reinterpret_cast<void**>(candidateFluid + 4) = 0;
			*reinterpret_cast<void**>(oracleFluid + 8) = 0;
			*reinterpret_cast<void**>(candidateFluid + 8) = 0;
			*reinterpret_cast<void**>(oracleFluid + 0x78) = oracleState;
			*reinterpret_cast<void**>(candidateFluid + 0x78) = candidateState;
			*reinterpret_cast<unsigned*>(oracleState + 4) = 0xaaaaaaaa;
			*reinterpret_cast<unsigned*>(candidateState + 4) = 0xaaaaaaaa;
			*reinterpret_cast<void**>(oracleFluid + 0x7c) = oracleBackend;
			*reinterpret_cast<void**>(candidateFluid + 0x7c) = candidateBackend;
			*reinterpret_cast<unsigned*>(oracleFluid + 0x80) = 0x55667788u;
			*reinterpret_cast<unsigned*>(candidateFluid + 0x80) = 0x55667788u;
			const unsigned timestepWord = 0x3eaaaaabu;
			const float timestep = *reinterpret_cast<const float*>(&timestepWord);
			resetRun();
			const int oracleResult = oracleStep(oracleFluid, timestep);
			// The oracle leaves FluidRun's float in ST(0) when its emitter range
			// is empty; this direct-entry fixture owns and clears that result.
			__asm fstp st(0)
			NxFluidRunCallbackRecord oracleRecord = {};
			const bool oracleRecordOk = fluidRunCount() == 1 && getFluidRun(0, &oracleRecord);
			resetRun();
			const int candidateResult = candidateStep(candidateFluid, timestep);
			NxFluidRunCallbackRecord candidateRecord = {};
			const bool candidateRecordOk = fluidRunCount() == 1 && getFluidRun(0, &candidateRecord);
			const unsigned timestepBits = *reinterpret_cast<const unsigned*>(&timestep);
			const bool callbackMatch = oracleRecordOk && candidateRecordOk
				&& oracleRecord.system == 0x10203040u && candidateRecord.system == oracleRecord.system
				&& oracleRecord.fluid == 0x55667788u && candidateRecord.fluid == oracleRecord.fluid
				&& oracleRecord.state == reinterpret_cast<unsigned>(oracleState)
				&& candidateRecord.state == reinterpret_cast<unsigned>(candidateState)
				&& !memcmp(oracleRecord.vector, expectedVector, sizeof(expectedVector))
				&& !memcmp(candidateRecord.vector, oracleRecord.vector, sizeof(oracleRecord.vector))
				&& oracleRecord.timestep == timestepBits && candidateRecord.timestep == oracleRecord.timestep
				&& oracleRecord.option == 1 && candidateRecord.option == oracleRecord.option;
			const unsigned mismatches = !callbackMatch || oracleResult != candidateResult
				|| *reinterpret_cast<unsigned*>(oracleFluid + 0x74) != timestepBits
				|| *reinterpret_cast<unsigned*>(candidateFluid + 0x74) != timestepBits
				|| *reinterpret_cast<unsigned*>(oracleState + 4) != 0
				|| *reinterpret_cast<unsigned*>(candidateState + 4) != 0;
			gFailures += mismatches;
			printf("fluid substep empty-emitter cases=1 callback=%u timestep=%u state-clear=%u mismatches=%u\n",
				callbackMatch, oracleRecord.timestep == timestepBits && candidateRecord.timestep == timestepBits,
				*reinterpret_cast<unsigned*>(oracleState + 4) == 0
					&& *reinterpret_cast<unsigned*>(candidateState + 4) == 0, mismatches);
			}
	}
	{
		// A nonempty emitter range exercises the ordered 003624 -> FluidRun ->
		// 003601 path without inventing an actor layout. With no frame actor, the
		// emitter update still publishes its world pose and the repulsion helper
		// takes its observed early return.
		void* candidateStepAddress = 0;
		if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
			"@nxNpFluidStep@12", &candidateStepAddress))
			{
			fprintf(stderr, "FAIL candidate Fluid step row missing for nonempty-emitter probe\n");
			++gFailures;
			}
		else
			{
			typedef int (__thiscall *FluidStep)(void*, float);
			FluidStep oracleStep = reinterpret_cast<FluidStep>(image + 0x85a40);
			FluidStep candidateStep = reinterpret_cast<FluidStep>(candidateStepAddress);
			unsigned char oracleFluid[0x84] = {}, candidateFluid[0x84] = {};
			unsigned char oracleBackend[0x534] = {}, candidateBackend[0x534] = {};
			unsigned char oracleState[8] = {}, candidateState[8] = {};
			unsigned char oracleCore[0x530] = {}, candidateCore[0x530] = {};
			unsigned char oracleEmitter[0x48] = {}, candidateEmitter[0x48] = {};
			void* oracleEmitterList[1] = { oracleEmitter };
			void* candidateEmitterList[1] = { candidateEmitter };
			static const unsigned identityPose[12] = {
				0x3f800000u, 0, 0, 0, 0x3f800000u, 0,
				0, 0, 0x3f800000u, 0x3e800000u, 0xbf000000u, 0x3f400000u };
			static const unsigned systemVector[3] = {
				0x3e800001u, 0xbf000002u, 0x3f400003u };
			memcpy(oracleCore + 0x520, systemVector, sizeof(systemVector));
			memcpy(candidateCore + 0x520, systemVector, sizeof(systemVector));
			*reinterpret_cast<void**>(oracleBackend + 0x24) = oracleCore;
			*reinterpret_cast<void**>(candidateBackend + 0x24) = candidateCore;
			*reinterpret_cast<unsigned*>(oracleBackend + 0x30) = 0x10203040u;
			*reinterpret_cast<unsigned*>(candidateBackend + 0x30) = 0x10203040u;
			*reinterpret_cast<void***>(oracleFluid + 4) = oracleEmitterList;
			*reinterpret_cast<void***>(oracleFluid + 8) = oracleEmitterList + 1;
			*reinterpret_cast<void***>(candidateFluid + 4) = candidateEmitterList;
			*reinterpret_cast<void***>(candidateFluid + 8) = candidateEmitterList + 1;
			*reinterpret_cast<void**>(oracleFluid + 0x78) = oracleState;
			*reinterpret_cast<void**>(candidateFluid + 0x78) = candidateState;
			*reinterpret_cast<unsigned*>(oracleState + 4) = 0xaaaaaaaa;
			*reinterpret_cast<unsigned*>(candidateState + 4) = 0xaaaaaaaa;
			*reinterpret_cast<void**>(oracleFluid + 0x7c) = oracleBackend;
			*reinterpret_cast<void**>(candidateFluid + 0x7c) = candidateBackend;
			*reinterpret_cast<unsigned*>(oracleFluid + 0x80) = 0x55667788u;
			*reinterpret_cast<unsigned*>(candidateFluid + 0x80) = 0x55667788u;
			for(unsigned i = 0; i != 2; ++i)
				{
				unsigned char* emitter = i == 0 ? oracleEmitter : candidateEmitter;
				void* fluid = i == 0 ? static_cast<void*>(oracleFluid)
					: static_cast<void*>(candidateFluid);
				*reinterpret_cast<void**>(emitter + 4) = fluid;
				*reinterpret_cast<unsigned*>(emitter + 8) = 0x11223344u;
				memcpy(emitter + 0x18, identityPose, sizeof(identityPose));
				}
			const unsigned timestepWord = 0x3eaaaaabu;
			const float timestep = *reinterpret_cast<const float*>(&timestepWord);
			typedef void (__cdecl *SetEmitterGlobalPose)(const float*);
			SetEmitterGlobalPose setGlobalPose = reinterpret_cast<SetEmitterGlobalPose>(
				GetProcAddress(fluidModel, "NxSetEmitterGlobalPose"));
			if(!setGlobalPose)
				{
				fprintf(stderr, "FAIL FluidModel fixture lacks emitter pose initializer\n");
				++gFailures;
				}
			else
				{
				setGlobalPose(reinterpret_cast<const float*>(identityPose));
				resetCallbacks();
				gOracleFlagCallbackCount = 0;
				resetRun();
				resetRepulsion();
				gOracleFluidRepulsionRecordCount = 0;
				unsigned oracleEspBefore = 0, oracleEspAfter = 0;
				__asm mov oracleEspBefore, esp
				const int oracleResult = oracleStep(oracleFluid, timestep);
				__asm mov oracleEspAfter, esp
				__asm fstp st(0)
				NxFluidRunCallbackRecord oracleRun = {};
				const bool oracleRunOk = fluidRunCount() == 1 && getFluidRun(0, &oracleRun);
				NxFluidEmitterFlagCallbackRecord oracleEmitterCallback = {};
				const bool oracleEmitterCallbackOk = gOracleFlagCallbackCount == 1;
				if(oracleEmitterCallbackOk)
					oracleEmitterCallback = gOracleFlagCallbacks[0];

				setGlobalPose(reinterpret_cast<const float*>(identityPose));
				resetCallbacks();
				resetRun();
				resetRepulsion();
				unsigned candidateEspBefore = 0, candidateEspAfter = 0;
				__asm mov candidateEspBefore, esp
				const int candidateResult = candidateStep(candidateFluid, timestep);
				__asm mov candidateEspAfter, esp
				__asm fstp st(0)
				NxFluidRunCallbackRecord candidateRun = {};
				const bool candidateRunOk = fluidRunCount() == 1 && getFluidRun(0, &candidateRun);
				NxFluidEmitterFlagCallbackRecord candidateEmitterCallback = {};
				const bool candidateEmitterCallbackOk = callbackCount() == 1
					&& getCallbackRecord(0, &candidateEmitterCallback);
				bool callbackMatch = oracleRunOk && candidateRunOk
					&& oracleEmitterCallbackOk && candidateEmitterCallbackOk
					&& oracleEmitterCallback.kind == 64
					&& candidateEmitterCallback.kind == oracleEmitterCallback.kind
					&& oracleEmitterCallback.argument0 == 0x10203040u
					&& candidateEmitterCallback.argument0 == oracleEmitterCallback.argument0
					&& oracleEmitterCallback.argument1 == 0x55667788u
					&& candidateEmitterCallback.argument1 == oracleEmitterCallback.argument1
					&& oracleEmitterCallback.argument2 == 0x11223344u
					&& candidateEmitterCallback.argument2 == oracleEmitterCallback.argument2
					&& !memcmp(oracleEmitterCallback.pose, identityPose, sizeof(identityPose))
					&& !memcmp(candidateEmitterCallback.pose, oracleEmitterCallback.pose,
						sizeof(oracleEmitterCallback.pose))
					&& oracleRun.system == 0x10203040u
					&& candidateRun.system == oracleRun.system
					&& oracleRun.fluid == 0x55667788u
					&& candidateRun.fluid == oracleRun.fluid
					&& oracleRun.state == reinterpret_cast<unsigned>(oracleState)
					&& candidateRun.state == reinterpret_cast<unsigned>(candidateState)
					&& !memcmp(oracleRun.vector, systemVector, sizeof(systemVector))
					&& !memcmp(candidateRun.vector, oracleRun.vector, sizeof(oracleRun.vector))
					&& oracleRun.timestep == timestepWord
					&& candidateRun.timestep == oracleRun.timestep
					&& oracleRun.option == 1 && candidateRun.option == oracleRun.option;
				const unsigned timestepAndState =
					*reinterpret_cast<unsigned*>(oracleFluid + 0x74) == timestepWord
					&& *reinterpret_cast<unsigned*>(candidateFluid + 0x74) == timestepWord
					&& *reinterpret_cast<unsigned*>(oracleState + 4) == 0
					&& *reinterpret_cast<unsigned*>(candidateState + 4) == 0;
				const unsigned mismatches = !callbackMatch || !timestepAndState
					|| oracleResult != candidateResult
					|| oracleEspBefore != oracleEspAfter
					|| candidateEspBefore != candidateEspAfter
					|| gOracleFluidRepulsionRecordCount != 0 || repulsionCount() != 0;
				gFailures += mismatches;
				printf("fluid substep nonempty-emitter cases=1 pose_callback=%u fluid_run=%u "
					"payloads_match=%u timestep=%u state-clear=%u repulsion-skipped=%u "
					"stack=%u/%u mismatches=%u\n",
					oracleEmitterCallbackOk && candidateEmitterCallbackOk,
					oracleRunOk && candidateRunOk, callbackMatch,
					timestepAndState, timestepAndState,
					gOracleFluidRepulsionRecordCount == 0 && repulsionCount() == 0,
					oracleEspBefore == oracleEspAfter,
					candidateEspBefore == candidateEspAfter, mismatches);
				}
			}
	}
	{
		if(!candidateManagerStepAddress)
			{
			fprintf(stderr, "FAIL candidate FluidManager step row could not be resolved from its map\n");
			++gFailures;
			}
		else
			{
			typedef int (__thiscall *ManagerStep)(void*, float);
			ManagerStep oracleStep = reinterpret_cast<ManagerStep>(image + 0x89bd0);
			ManagerStep candidateStep = reinterpret_cast<ManagerStep>(candidateManagerStepAddress);
			unsigned char oracleManager[0x34] = {}, candidateManager[0x34] = {};
			unsigned oracleFluidWords[2] = {}, candidateFluidWords[2] = {};
			unsigned char oracleFluids[2][0x84] = {}, candidateFluids[2][0x84] = {};
			unsigned char oracleBackends[2][0x534] = {}, candidateBackends[2][0x534] = {};
			unsigned char oracleStates[2][8] = {}, candidateStates[2][8] = {};
			unsigned char oracleCores[2][0x530] = {}, candidateCores[2][0x530] = {};
			const unsigned expectedVectors[2][3] = {
				{ 0x3e800001u, 0xbf000002u, 0x3f400003u },
				{ 0x40000001u, 0xc0400002u, 0x40800003u } };
			for(unsigned i = 0; i != 2; ++i)
				{
				oracleFluidWords[i] = reinterpret_cast<unsigned>(oracleFluids[i]);
				candidateFluidWords[i] = reinterpret_cast<unsigned>(candidateFluids[i]);
				memcpy(oracleCores[i] + 0x520, expectedVectors[i], sizeof(expectedVectors[i]));
				memcpy(candidateCores[i] + 0x520, expectedVectors[i], sizeof(expectedVectors[i]));
				*reinterpret_cast<void**>(oracleBackends[i] + 0x24) = oracleCores[i];
				*reinterpret_cast<void**>(candidateBackends[i] + 0x24) = candidateCores[i];
				*reinterpret_cast<unsigned*>(oracleBackends[i] + 0x30) = 0x10203040u + i;
				*reinterpret_cast<unsigned*>(candidateBackends[i] + 0x30) = 0x10203040u + i;
				*reinterpret_cast<void**>(oracleFluids[i] + 0x78) = oracleStates[i];
				*reinterpret_cast<void**>(candidateFluids[i] + 0x78) = candidateStates[i];
				*reinterpret_cast<unsigned*>(oracleStates[i] + 4) = 0xaaaaaaaa;
				*reinterpret_cast<unsigned*>(candidateStates[i] + 4) = 0xaaaaaaaa;
				*reinterpret_cast<void**>(oracleFluids[i] + 0x7c) = oracleBackends[i];
				*reinterpret_cast<void**>(candidateFluids[i] + 0x7c) = candidateBackends[i];
				*reinterpret_cast<unsigned*>(oracleFluids[i] + 0x80) = 0x55667780u + i;
				*reinterpret_cast<unsigned*>(candidateFluids[i] + 0x80) = 0x55667780u + i;
				}
			*reinterpret_cast<void***>(oracleManager + 4) =
				reinterpret_cast<void**>(oracleFluidWords);
			*reinterpret_cast<void***>(oracleManager + 8) =
				reinterpret_cast<void**>(oracleFluidWords + 2);
			*reinterpret_cast<void***>(candidateManager + 4) =
				reinterpret_cast<void**>(candidateFluidWords);
			*reinterpret_cast<void***>(candidateManager + 8) =
				reinterpret_cast<void**>(candidateFluidWords + 2);
			oracleManager[0x2b] = candidateManager[0x2b] = 1;
			const unsigned timestepWord = 0x3eaaaaabu;
			const float timestep = *reinterpret_cast<const float*>(&timestepWord);
			unsigned oracleEspBefore = 0, oracleEspAfter = 0;
			unsigned candidateEspBefore = 0, candidateEspAfter = 0;
			resetRun();
			__asm mov oracleEspBefore, esp
			const int oracleResult = oracleStep(oracleManager, timestep);
			__asm mov oracleEspAfter, esp
			__asm fstp st(0)
			NxFluidRunCallbackRecord oracleRecords[2] = {};
			const bool oracleRecordsOk = fluidRunCount() == 2
				&& getFluidRun(0, &oracleRecords[0]) && getFluidRun(1, &oracleRecords[1]);
			resetRun();
			__asm mov candidateEspBefore, esp
			const int candidateResult = candidateStep(candidateManager, timestep);
			__asm mov candidateEspAfter, esp
			// The callback's float return remains in ST(0) after the final empty-
			// emitter substep in the oracle; the direct-entry fixture owns it.
			__asm fstp st(0)
			NxFluidRunCallbackRecord candidateRecords[2] = {};
			const bool candidateRecordsOk = fluidRunCount() == 2
				&& getFluidRun(0, &candidateRecords[0]) && getFluidRun(1, &candidateRecords[1]);
			unsigned callbackMismatches = !oracleRecordsOk || !candidateRecordsOk;
			for(unsigned i = 0; i != 2; ++i)
				{
				if(!oracleRecordsOk || !candidateRecordsOk)
					break;
				const NxFluidRunCallbackRecord& expected = oracleRecords[i];
				const NxFluidRunCallbackRecord& actual = candidateRecords[i];
				if(expected.system != 0x10203040u + i || actual.system != expected.system
					|| expected.fluid != 0x55667780u + i || actual.fluid != expected.fluid
					|| expected.state != reinterpret_cast<unsigned>(oracleStates[i])
					|| actual.state != reinterpret_cast<unsigned>(candidateStates[i])
					|| memcmp(expected.vector, expectedVectors[i], sizeof(expected.vector))
					|| memcmp(actual.vector, expected.vector, sizeof(actual.vector))
					|| expected.timestep != timestepWord || actual.timestep != expected.timestep
					|| expected.option != 1 || actual.option != expected.option)
					++callbackMismatches;
				}
			unsigned stateMismatches = 0;
			for(unsigned i = 0; i != 2; ++i)
				if(*reinterpret_cast<unsigned*>(oracleFluids[i] + 0x74) != timestepWord
					|| *reinterpret_cast<unsigned*>(candidateFluids[i] + 0x74) != timestepWord
					|| *reinterpret_cast<unsigned*>(oracleStates[i] + 4) != 0
					|| *reinterpret_cast<unsigned*>(candidateStates[i] + 4) != 0)
					++stateMismatches;
			const unsigned mismatches = callbackMismatches + stateMismatches
				+ (oracleResult != candidateResult)
				+ (oracleEspBefore != oracleEspAfter)
				+ (candidateEspBefore != candidateEspAfter);
			gFailures += mismatches;
			printf("fluid manager step live-fluids=2 oracle_callbacks=%u candidate_callbacks=%u order=%u timestep=%u state=%u stack=%u/%u mismatches=%u\n",
				oracleRecordsOk ? 2u : fluidRunCount(), candidateRecordsOk ? 2u : fluidRunCount(),
				callbackMismatches == 0, callbackMismatches == 0, stateMismatches == 0,
				oracleEspBefore == oracleEspAfter, candidateEspBefore == candidateEspAfter, mismatches);
			}
	}

	// The fetch-side 000619 row must update every fluid in Scene+0x61c. A
	// missing traversal leaves the candidate's bounds and particle outputs at
	// their poison values even though the oracle visits both seeded fluids.
	{
		typedef void (__thiscall *FinishSimulation)(void*);
		void* candidateFinishAddress = 0;
		static const char finishName[] = "?finishSimulation@NxSceneInternal@@QAEXXZ";
		if(!nxResolveCandidateFluidEmitterSymbol(candidatePhysics, finishName,
			&candidateFinishAddress))
			{
			fprintf(stderr, "FAIL candidate fetch-side finishSimulation row could not be resolved\n");
			return 1;
			}
		FinishSimulation oracleFinish = reinterpret_cast<FinishSimulation>(image + 0x114b0);
		FinishSimulation candidateFinish = reinterpret_cast<FinishSimulation>(candidateFinishAddress);
		unsigned char oracleScene[0x710] = {};
		unsigned char candidateScene[0x710] = {};
		unsigned char oracleManager[0x34] = {};
		unsigned char candidateManager[0x34] = {};
		unsigned oracleFluids[2] = {};
		unsigned candidateFluids[2] = {};
		unsigned char oracleFluid[2][0x84] = {};
		unsigned char candidateFluid[2][0x84] = {};
		unsigned char oracleBackend[2][0x34] = {};
		unsigned char candidateBackend[2][0x34] = {};
		unsigned oracleEmitterLists[2] = {};
		unsigned candidateEmitterLists[2] = {};
		unsigned char oracleEmitters[2][0x78] = {};
		unsigned char candidateEmitters[2][0x78] = {};
		unsigned char oracleMeshRecords[2][0x30] = {};
		unsigned char candidateMeshRecords[2][0x30] = {};
		unsigned oracleMeshCounts[2] = {};
		unsigned candidateMeshCounts[2] = {};
		unsigned oracleMeshLeadingCounts[2] = {};
		unsigned candidateMeshLeadingCounts[2] = {};
		unsigned oracleMeshVertices[2][3] = {};
		unsigned candidateMeshVertices[2][3] = {};
		unsigned oracleMeshSurfaceOutput[2][3] = {};
		unsigned candidateMeshSurfaceOutput[2][3] = {};
		unsigned oracleMeshLeading[2][3] = {};
		unsigned candidateMeshLeading[2][3] = {};
		unsigned oracleParticleCount[2] = {};
		unsigned candidateParticleCount[2] = {};
		unsigned oracleOutput[2][3][12] = {};
		unsigned candidateOutput[2][3][12] = {};
		static const unsigned particles[2][2][23] = {
			{
				{ 0, 0x3f800001u, 0x40000002u, 0x40400003u, 0x40800004u, 0x40a00005u,
					0x40c00006u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x40e00007u },
				{ 0, 0x41000010u, 0x41100011u, 0x41200012u, 0x41300013u, 0x41400014u,
					0x41500015u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x41600016u }
			},
			{
				{ 0, 0x42000020u, 0x42100021u, 0x42200022u, 0x42300023u, 0x42400024u,
					0x42500025u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x42600026u },
				{ 0, 0x43000030u, 0x43100031u, 0x43200032u, 0x43300033u, 0x43400034u,
					0x43500035u, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x43600036u }
			}
		};
		static const unsigned outputStrideBytes[2][3] = {
			{ 16, 20, 8 }, { 20, 16, 12 }
		};
		static const unsigned emitterPose[12] = {
			0x3f800000u, 0x40000000u, 0x40400000u, 0x40800000u,
			0x40a00000u, 0x40c00000u, 0x40e00000u, 0x41000000u,
			0x41100000u, 0x41200000u, 0x41300000u, 0x41400000u };
		static const unsigned expectedEmitterPose[12] = {
			0x3f800000u, 0x40800000u, 0x40e00000u, 0x40000000u,
			0x40a00000u, 0x41000000u, 0x40400000u, 0x40c00000u,
			0x41100000u, 0x41200000u, 0x41300000u, 0x41400000u };
		typedef void (__cdecl *SetEmitterPoseFn)(const float*);
		SetEmitterPoseFn setEmitterPose = reinterpret_cast<SetEmitterPoseFn>(
			GetProcAddress(fluidModel, "NxSetEmitterGlobalPose"));
		if(!setEmitterPose)
			{
			fprintf(stderr, "FAIL FluidModel callback fixture lacks emitter pose setup\n");
			return 1;
			}
		memcpy(gOracleEmitterGlobalPose, emitterPose, sizeof(emitterPose));
		setEmitterPose(reinterpret_cast<const float*>(emitterPose));
		memset(oracleOutput, 0xa5, sizeof(oracleOutput));
		memset(candidateOutput, 0xa5, sizeof(candidateOutput));
		for(unsigned i = 0; i != 2; ++i)
			{
				memcpy(gOraclePostStepParticles[i], particles[i], sizeof(particles[i]));
				gOraclePostStepStreams[i].data = gOraclePostStepParticles[i][0];
				gOraclePostStepStreams[i].count = 2;
				setPostStepParticle(i, &particles[i][0][0], 2);
				oracleFluids[i] = reinterpret_cast<unsigned>(oracleFluid[i]);
				candidateFluids[i] = reinterpret_cast<unsigned>(candidateFluid[i]);
				*reinterpret_cast<void**>(oracleFluid[i] + 0x7c) = oracleBackend[i];
				*reinterpret_cast<void**>(candidateFluid[i] + 0x7c) = candidateBackend[i];
				*reinterpret_cast<unsigned*>(oracleFluid[i] + 0x80) = 0x5100u + i;
				*reinterpret_cast<unsigned*>(candidateFluid[i] + 0x80) = 0x5100u + i;
				*reinterpret_cast<unsigned*>(oracleFluid[i] + 0x28) = i ? 1 : 3;
				*reinterpret_cast<unsigned*>(candidateFluid[i] + 0x28) = i ? 1 : 3;
				oracleEmitterLists[i] = reinterpret_cast<unsigned>(oracleEmitters[i]);
				candidateEmitterLists[i] = reinterpret_cast<unsigned>(candidateEmitters[i]);
				*reinterpret_cast<unsigned*>(oracleFluid[i] + 4) = reinterpret_cast<unsigned>(&oracleEmitterLists[i]);
				*reinterpret_cast<unsigned*>(oracleFluid[i] + 8) = reinterpret_cast<unsigned>(&oracleEmitterLists[i] + 1);
				*reinterpret_cast<unsigned*>(candidateFluid[i] + 4) = reinterpret_cast<unsigned>(&candidateEmitterLists[i]);
				*reinterpret_cast<unsigned*>(candidateFluid[i] + 8) = reinterpret_cast<unsigned>(&candidateEmitterLists[i] + 1);
				*reinterpret_cast<void**>(oracleFluid[i] + 0x24) = i ? 0 : oracleMeshRecords[i];
				*reinterpret_cast<void**>(candidateFluid[i] + 0x24) = i ? 0 : candidateMeshRecords[i];
				*reinterpret_cast<void**>(oracleMeshRecords[i] + 4) = oracleFluid[i];
				*reinterpret_cast<void**>(candidateMeshRecords[i] + 4) = candidateFluid[i];
				*reinterpret_cast<unsigned*>(oracleMeshRecords[i] + 8) = reinterpret_cast<unsigned>(oracleMeshVertices[i]);
				*reinterpret_cast<unsigned*>(candidateMeshRecords[i] + 8) = reinterpret_cast<unsigned>(candidateMeshVertices[i]);
				*reinterpret_cast<unsigned*>(oracleMeshRecords[i] + 0x0c) = reinterpret_cast<unsigned>(oracleMeshSurfaceOutput[i]);
				*reinterpret_cast<unsigned*>(candidateMeshRecords[i] + 0x0c) = reinterpret_cast<unsigned>(candidateMeshSurfaceOutput[i]);
				*reinterpret_cast<unsigned*>(oracleMeshRecords[i] + 0x10) = 12;
				*reinterpret_cast<unsigned*>(candidateMeshRecords[i] + 0x10) = 12;
				*reinterpret_cast<unsigned*>(oracleMeshRecords[i] + 0x14) = 12;
				*reinterpret_cast<unsigned*>(candidateMeshRecords[i] + 0x14) = 12;
				*reinterpret_cast<unsigned**>(oracleMeshRecords[i] + 0x1c) = &oracleMeshCounts[i];
				*reinterpret_cast<unsigned**>(candidateMeshRecords[i] + 0x1c) = &candidateMeshCounts[i];
				*reinterpret_cast<unsigned*>(oracleMeshRecords[i] + 0x20) = reinterpret_cast<unsigned>(oracleMeshLeading[i]);
				*reinterpret_cast<unsigned*>(candidateMeshRecords[i] + 0x20) = reinterpret_cast<unsigned>(candidateMeshLeading[i]);
				*reinterpret_cast<unsigned*>(oracleMeshRecords[i] + 0x24) = 4;
				*reinterpret_cast<unsigned*>(candidateMeshRecords[i] + 0x24) = 4;
				*reinterpret_cast<unsigned**>(oracleMeshRecords[i] + 0x2c) = &oracleMeshLeadingCounts[i];
				*reinterpret_cast<unsigned**>(candidateMeshRecords[i] + 0x2c) = &candidateMeshLeadingCounts[i];
				*reinterpret_cast<unsigned*>(oracleEmitters[i] + 4) = reinterpret_cast<unsigned>(oracleFluid[i]);
				*reinterpret_cast<unsigned*>(candidateEmitters[i] + 4) = reinterpret_cast<unsigned>(candidateFluid[i]);
				*reinterpret_cast<unsigned*>(oracleEmitters[i] + 8) = 0x6200u + i;
				*reinterpret_cast<unsigned*>(candidateEmitters[i] + 8) = 0x6200u + i;
				*reinterpret_cast<unsigned*>(oracleBackend[i] + 0x30) = i;
				*reinterpret_cast<unsigned*>(candidateBackend[i] + 0x30) = i;
				*reinterpret_cast<unsigned**>(oracleFluid[i] + 0x2c) = &oracleParticleCount[i];
				*reinterpret_cast<unsigned**>(candidateFluid[i] + 0x2c) = &candidateParticleCount[i];
				for(unsigned channel = 0; channel != 3; ++channel)
					{
						*reinterpret_cast<unsigned**>(oracleFluid[i] + 0x30 + channel * 4) = oracleOutput[i][channel];
						*reinterpret_cast<unsigned**>(candidateFluid[i] + 0x30 + channel * 4) = candidateOutput[i][channel];
						*reinterpret_cast<unsigned*>(oracleFluid[i] + 0x40 + channel * 4) = outputStrideBytes[i][channel];
						*reinterpret_cast<unsigned*>(candidateFluid[i] + 0x40 + channel * 4) = outputStrideBytes[i][channel];
					}
			}
		*reinterpret_cast<unsigned*>(oracleManager + 4) = reinterpret_cast<unsigned>(oracleFluids);
		*reinterpret_cast<unsigned*>(oracleManager + 8) = reinterpret_cast<unsigned>(oracleFluids + 2);
		*reinterpret_cast<unsigned*>(candidateManager + 4) = reinterpret_cast<unsigned>(candidateFluids);
		*reinterpret_cast<unsigned*>(candidateManager + 8) = reinterpret_cast<unsigned>(candidateFluids + 2);
		*reinterpret_cast<unsigned*>(oracleScene + 0x61c) = reinterpret_cast<unsigned>(oracleManager);
		*reinterpret_cast<unsigned*>(candidateScene + 0x61c) = reinterpret_cast<unsigned>(candidateManager);
		gOraclePostStepRecordCount = 0;
		gOracleFlagCallbackCount = 0;
		resetPostStep();
		resetCallbacks();
		oracleFinish(oracleScene);
		const unsigned oracleCallbacks = gOraclePostStepRecordCount;
		NxFluidPostStepCallbackRecord oracleRecords[8] = {};
		memcpy(oracleRecords, gOraclePostStepRecords,
			sizeof(oracleRecords[0]) * (oracleCallbacks < 8 ? oracleCallbacks : 8));
		candidateFinish(candidateScene);
		const unsigned candidateCallbacks = postStepCount();
		unsigned mismatches = oracleCallbacks != 7 || candidateCallbacks != 7;
		const unsigned oracleEmitterCallbacks = gOracleFlagCallbackCount;
		const unsigned candidateEmitterCallbacks = callbackCount();
		if(oracleEmitterCallbacks != 2 || candidateEmitterCallbacks != 2)
			++mismatches;
		for(unsigned i = 0; i != 7; ++i)
			{
				NxFluidPostStepCallbackRecord candidateRecord = {};
				if(i >= oracleCallbacks || !getPostStepRecord(i, &candidateRecord))
					{ ++mismatches; continue; }
				if(candidateRecord.kind != oracleRecords[i].kind
					|| candidateRecord.system != oracleRecords[i].system
					|| candidateRecord.handle != oracleRecords[i].handle)
					++mismatches;
				for(unsigned value = 0; value != 6; ++value)
					if(candidateRecord.values[value] != oracleRecords[i].values[value])
						++mismatches;
			}
		for(unsigned i = 0; i != 2; ++i)
			{
				const unsigned expectedBounds[6] = {
					i ? 0x41300000u : 0x3f800000u, i ? 0x41400000u : 0x40000000u,
					i ? 0x41500000u : 0x40400000u, i ? 0x41600000u : 0x40800000u,
					i ? 0x41700000u : 0x40a00000u, i ? 0x41800000u : 0x40c00000u };
				const unsigned expectedParticleCount = i ? 1u : 2u;
				if(oracleParticleCount[i] != expectedParticleCount
					|| candidateParticleCount[i] != expectedParticleCount)
					++mismatches;
				for(unsigned word = 0; word != 6; ++word)
					if(*reinterpret_cast<unsigned*>(oracleFluid[i] + 0x5c + word * 4) != expectedBounds[word]
						|| *reinterpret_cast<unsigned*>(candidateFluid[i] + 0x5c + word * 4) != expectedBounds[word])
						++mismatches;
				for(unsigned particle = 0; particle != expectedParticleCount; ++particle)
					for(unsigned channel = 0; channel != 3; ++channel)
						{
						const unsigned stride = outputStrideBytes[i][channel] / 4;
						const unsigned sourceOffset = channel == 0 ? 1 : channel == 1 ? 4 : 22;
						const unsigned componentCount = channel == 2 ? 1u : 3u;
						for(unsigned component = 0; component != componentCount; ++component)
							{
							const unsigned expected = particles[i][particle][sourceOffset + component];
							const unsigned outputIndex = particle * stride + component;
							if(oracleOutput[i][channel][outputIndex] != expected
								|| candidateOutput[i][channel][outputIndex] != expected)
								++mismatches;
							}
						}
				const unsigned expectedLeading[3] = {
					0x3e800001u, 0x3e800002u, 0x3e800003u };
				const unsigned expectedVertices[3] = {
					0x3f400001u, 0x3f400002u, 0x3f400003u };
				const unsigned expectedNormals[3] = {
					0x3fc00001u, 0x3fc00002u, 0x3fc00003u };
				const unsigned expectedMeshCount = i ? 0u : 1u;
				if(oracleMeshCounts[i] != expectedMeshCount || candidateMeshCounts[i] != expectedMeshCount
					|| oracleMeshLeadingCounts[i] != expectedMeshCount
					|| candidateMeshLeadingCounts[i] != expectedMeshCount)
					++mismatches;
				for(unsigned axis = 0; axis != 3; ++axis)
					if(oracleMeshVertices[i][axis] != (i ? 0u : expectedVertices[axis])
						|| candidateMeshVertices[i][axis] != (i ? 0u : expectedVertices[axis])
						|| oracleMeshSurfaceOutput[i][axis] != (i ? 0u : expectedNormals[axis])
						|| candidateMeshSurfaceOutput[i][axis] != (i ? 0u : expectedNormals[axis])
						|| oracleMeshLeading[i][axis] != (i ? 0u : expectedLeading[axis])
						|| candidateMeshLeading[i][axis] != (i ? 0u : expectedLeading[axis]))
						++mismatches;
				for(unsigned word = 0; word != 12; ++word)
					if(*reinterpret_cast<unsigned*>(oracleEmitters[i] + 0x48 + word * 4) != expectedEmitterPose[word]
						|| *reinterpret_cast<unsigned*>(candidateEmitters[i] + 0x48 + word * 4) != expectedEmitterPose[word])
						++mismatches;
			}
		gFailures += mismatches;
		printf("fluid post-step traversal fluids=2 oracle_callbacks=%u candidate_callbacks=%u particle_counts=%u/%u particle_strides=distinct emitter_tuples=2 optional_mesh_null=1 mismatches=%u\n",
			oracleCallbacks, candidateCallbacks, oracleParticleCount[0] + oracleParticleCount[1],
			candidateParticleCount[0] + candidateParticleCount[1], mismatches);
	}

	unsigned char oracleBackendFluid[0x84];
	unsigned char candidateBackendFluid[0x84];
	unsigned char oracleBackendContext[0x34];
	unsigned char candidateBackendContext[0x34];
	memset(oracleBackendFluid, 0, sizeof(oracleBackendFluid));
	memset(candidateBackendFluid, 0, sizeof(candidateBackendFluid));
	memset(oracleBackendContext, 0, sizeof(oracleBackendContext));
	memset(candidateBackendContext, 0, sizeof(candidateBackendContext));
	*reinterpret_cast<void**>(oracleFlagInternal + 4) = oracleBackendFluid;
	*reinterpret_cast<void**>(candidateFlagInternal + 4) = candidateBackendFluid;
	*reinterpret_cast<unsigned*>(oracleFlagInternal + 8) = 0x13579bdfu;
	*reinterpret_cast<unsigned*>(candidateFlagInternal + 8) = 0x13579bdfu;
	*reinterpret_cast<void**>(oracleBackendFluid + 0x7c) = oracleBackendContext;
	*reinterpret_cast<void**>(candidateBackendFluid + 0x7c) = candidateBackendContext;
	*reinterpret_cast<unsigned*>(oracleBackendContext + 0x30) = 0xa1b2c3d4u;
	*reinterpret_cast<unsigned*>(candidateBackendContext + 0x30) = 0xa1b2c3d4u;
	*reinterpret_cast<unsigned*>(oracleBackendFluid + 0x80) = 0x55667788u;
	*reinterpret_cast<unsigned*>(candidateBackendFluid + 0x80) = 0x55667788u;
	static const struct BackendFlagCase
		{
		NxFluidEmitterFlag flag;
		unsigned callbackKind;
		} backendFlagCases[] = {
			{ NX_FEF_FORCE_ON_ACTOR, 4 },
			{ NX_FEF_ADD_ACTOR_VELOCITY, 8 },
			{ NX_FEF_ENABLED, 16 },
		};
	unsigned backendCallbackMismatches = 0;
	unsigned backendStateMismatches = 0;
	unsigned backendCases = 0;
	for(unsigned i = 0; i < sizeof(backendFlagCases) / sizeof(backendFlagCases[0]); ++i)
		for(unsigned enabled = 0; enabled != 2; ++enabled)
			{
			++backendCases;
			const unsigned mask = static_cast<unsigned>(backendFlagCases[i].flag);
			const unsigned expectedFlags = enabled ? (0x80000100u | mask) : 0x80000100u;
			*reinterpret_cast<unsigned*>(oracleFlagInternal + 0x10) = 0x80000100u;
			*reinterpret_cast<unsigned*>(candidateFlagInternal + 0x10) = 0x80000100u;
			gOracleFlagCallbackCount = 0;
			oracleFlag->setFlag(backendFlagCases[i].flag, enabled != 0);
			NxFluidEmitterFlagCallbackRecord oracleRecord = {};
			if(gOracleFlagCallbackCount != 1)
				++backendCallbackMismatches;
			else
				oracleRecord = gOracleFlagCallbacks[0];
			if(*reinterpret_cast<unsigned*>(oracleFlagInternal + 0x10) != expectedFlags
				|| oracleRecord.kind != backendFlagCases[i].callbackKind
				|| oracleRecord.argument0 != 0xa1b2c3d4u
			|| oracleRecord.argument1 != 0x55667788u
			|| oracleRecord.argument2 != 0x13579bdfu
			|| oracleRecord.enabled != enabled)
				++backendStateMismatches;

	resetCallbacks();
			candidateSetFlag(&candidateFlag, backendFlagCases[i].flag, enabled != 0);
			NxFluidEmitterFlagCallbackRecord candidateRecord = {};
			if(callbackCount() != 1 || !getCallbackRecord(0, &candidateRecord))
				++backendCallbackMismatches;
			else if(candidateRecord.kind != oracleRecord.kind
				|| candidateRecord.argument0 != oracleRecord.argument0
				|| candidateRecord.argument1 != oracleRecord.argument1
				|| candidateRecord.argument2 != oracleRecord.argument2
				|| candidateRecord.enabled != oracleRecord.enabled)
				++backendCallbackMismatches;
			if(*reinterpret_cast<unsigned*>(candidateFlagInternal + 0x10) != expectedFlags)
				++backendStateMismatches;
			}

	++backendCases;
	*reinterpret_cast<unsigned*>(oracleFlagInternal + 0x10) = 0x80000100u;
	*reinterpret_cast<unsigned*>(candidateFlagInternal + 0x10) = 0x80000100u;
	gOracleFlagCallbackCount = 0;
	oracleFlag->setFlag(NX_FEF_BROKEN_ACTOR_REF, true);
	resetCallbacks();
	candidateSetFlag(&candidateFlag, NX_FEF_BROKEN_ACTOR_REF, true);
	if(gOracleFlagCallbackCount != 0 || callbackCount() != 0
		|| *reinterpret_cast<unsigned*>(oracleFlagInternal + 0x10) != 0x80000102u
		|| *reinterpret_cast<unsigned*>(candidateFlagInternal + 0x10) != 0x80000102u)
		++backendStateMismatches;
	gFailures += backendCallbackMismatches + backendStateMismatches;
	printf("fluid emitter backend flags cases=%u callback_mismatches=%u state_mismatches=%u\n",
		backendCases, backendCallbackMismatches, backendStateMismatches);

	// phys_fn_003635 walks manager fluids, whose phys_fn_003485 notification
	// marks only emitters attached to the released actor and detaches each one.
	// Seed two matches around a non-matching emitter so iteration, per-emitter
	// state, pose preservation, and callback order all have an oracle reference.
	typedef void (__thiscall *OracleManagerActorRelease)(void*, void*);
	typedef void (__cdecl *CandidateManagerActorRelease)(void*, void*);
	typedef void (__cdecl *SetFixturePoseFn)(const float*);
	OracleManagerActorRelease oracleManagerActorRelease =
		reinterpret_cast<OracleManagerActorRelease>(image + 0x89ce0);
	CandidateManagerActorRelease candidateManagerActorRelease =
		reinterpret_cast<CandidateManagerActorRelease>(candidateNotifyActorReleasedAddress);
	SetFixturePoseFn setFixturePose = reinterpret_cast<SetFixturePoseFn>(
		GetProcAddress(fluidModel, "NxSetEmitterGlobalPose"));
	if(!setFixturePose)
		{
		fprintf(stderr, "FAIL FluidModel transform callback fixture lacks pose setter\n");
		++gFailures;
		}
	else
		{
		unsigned char oracleManager[0x30], candidateManager[0x30];
		unsigned char oracleFluid[0x84], candidateFluid[0x84];
		unsigned char oracleBackend[0x34], candidateBackend[0x34];
		unsigned char oracleEmitters[3][0x48], candidateEmitters[3][0x48];
		unsigned char actorBody[4], otherBody[4];
		void* oracleFluidList[] = { oracleFluid };
		void* candidateFluidList[] = { candidateFluid };
		void* oracleEmitterList[] = {
			oracleEmitters[0], oracleEmitters[1], oracleEmitters[2] };
		void* candidateEmitterList[] = {
			candidateEmitters[0], candidateEmitters[1], candidateEmitters[2] };
		memset(oracleManager, 0, sizeof(oracleManager));
		memset(candidateManager, 0, sizeof(candidateManager));
		memset(oracleFluid, 0, sizeof(oracleFluid));
		memset(candidateFluid, 0, sizeof(candidateFluid));
		memset(oracleBackend, 0, sizeof(oracleBackend));
		memset(candidateBackend, 0, sizeof(candidateBackend));
		memset(oracleEmitters, 0, sizeof(oracleEmitters));
		memset(candidateEmitters, 0, sizeof(candidateEmitters));
		memset(actorBody, 0, sizeof(actorBody));
		memset(otherBody, 0, sizeof(otherBody));
		*reinterpret_cast<void***>(oracleManager + 4) = oracleFluidList;
		*reinterpret_cast<void***>(oracleManager + 8) = oracleFluidList + 1;
		*reinterpret_cast<void***>(candidateManager + 4) = candidateFluidList;
		*reinterpret_cast<void***>(candidateManager + 8) = candidateFluidList + 1;
		// +0x2b set means the manager's warning-only unavailable branch is not
		// taken; this isolates the live notification chain in both binaries.
		oracleManager[0x2b] = candidateManager[0x2b] = 1;
		*reinterpret_cast<void***>(oracleFluid + 4) = oracleEmitterList;
		*reinterpret_cast<void***>(oracleFluid + 8) = oracleEmitterList + 3;
		*reinterpret_cast<void***>(candidateFluid + 4) = candidateEmitterList;
		*reinterpret_cast<void***>(candidateFluid + 8) = candidateEmitterList + 3;
		*reinterpret_cast<void**>(oracleFluid + 0x7c) = oracleBackend;
		*reinterpret_cast<void**>(candidateFluid + 0x7c) = candidateBackend;
		*reinterpret_cast<unsigned*>(oracleFluid + 0x80) = 0x55667788u;
		*reinterpret_cast<unsigned*>(candidateFluid + 0x80) = 0x55667788u;
		*reinterpret_cast<unsigned*>(oracleBackend + 0x30) = 0xa1b2c3d4u;
		*reinterpret_cast<unsigned*>(candidateBackend + 0x30) = 0xa1b2c3d4u;
		const unsigned emitterHandles[] = { 0x11223344u, 0x22334455u, 0x33445566u };
		for(unsigned i = 0; i < 3; ++i)
			{
			*reinterpret_cast<void**>(oracleEmitters[i] + 4) = oracleFluid;
			*reinterpret_cast<void**>(candidateEmitters[i] + 4) = candidateFluid;
			*reinterpret_cast<unsigned*>(oracleEmitters[i] + 8) = emitterHandles[i];
			*reinterpret_cast<unsigned*>(candidateEmitters[i] + 8) = emitterHandles[i];
			*reinterpret_cast<unsigned*>(oracleEmitters[i] + 0x10) = 0x80000100u;
			*reinterpret_cast<unsigned*>(candidateEmitters[i] + 0x10) = 0x80000100u;
			void* attachedBody = (i == 1) ? static_cast<void*>(otherBody)
				: static_cast<void*>(actorBody);
			*reinterpret_cast<void**>(oracleEmitters[i] + 0x14) = attachedBody;
			*reinterpret_cast<void**>(candidateEmitters[i] + 0x14) = attachedBody;
			for(unsigned word = 0; word < 12; ++word)
				{
				*reinterpret_cast<unsigned*>(oracleEmitters[i] + 0x18 + word * 4) =
					0x3e000000u + i * 0x100u + word;
				*reinterpret_cast<unsigned*>(candidateEmitters[i] + 0x18 + word * 4) =
					0x3e000000u + i * 0x100u + word;
				}
			}
		static const unsigned globalPoseBits[12] = {
			0x3f800000u, 0x00000000u, 0x00000000u,
			0x00000000u, 0x3f800000u, 0x00000000u,
			0x00000000u, 0x00000000u, 0x3f800000u,
			0x41200000u, 0xc1200000u, 0x40400000u };
		memcpy(gOracleEmitterGlobalPose, globalPoseBits, sizeof(globalPoseBits));
		setFixturePose(reinterpret_cast<const float*>(globalPoseBits));
		gOracleFlagCallbackCount = 0;
		oracleManagerActorRelease(oracleManager, actorBody);
		NxFluidEmitterFlagCallbackRecord oracleRecords[6] = {};
		const unsigned oraclePoseCallbackCount = gOracleFlagCallbackCount;
		if(oraclePoseCallbackCount == 6)
			memcpy(oracleRecords, gOracleFlagCallbacks, sizeof(oracleRecords));
		unsigned oracleStateMismatches = oraclePoseCallbackCount == 6 ? 0 : 1;
		for(unsigned i = 0; i < 3; ++i)
			{
			const bool matched = i != 1;
			const unsigned expectedFlags = matched ? 0x80000102u : 0x80000100u;
			if(*reinterpret_cast<unsigned*>(oracleEmitters[i] + 0x10) != expectedFlags
				|| *reinterpret_cast<void**>(oracleEmitters[i] + 0x14) !=
					(matched ? 0 : static_cast<void*>(otherBody)))
				++oracleStateMismatches;
			for(unsigned word = 0; word < 12; ++word)
				{
				const unsigned expectedPose = matched ? globalPoseBits[word]
					: 0x3e000000u + i * 0x100u + word;
				if(*reinterpret_cast<unsigned*>(oracleEmitters[i] + 0x18 + word * 4)
					!= expectedPose)
					++oracleStateMismatches;
				}
			}
		resetCallbacks();
		setFixturePose(reinterpret_cast<const float*>(globalPoseBits));
		candidateManagerActorRelease(candidateManager, actorBody);
		const unsigned candidatePoseCallbackCount = callbackCount();
		unsigned candidateStateMismatches = candidatePoseCallbackCount == 6 ? 0 : 1;
		for(unsigned i = 0; i < 3; ++i)
			{
			const bool matched = i != 1;
			const unsigned expectedFlags = matched ? 0x80000102u : 0x80000100u;
			if(*reinterpret_cast<unsigned*>(candidateEmitters[i] + 0x10) != expectedFlags
				|| *reinterpret_cast<void**>(candidateEmitters[i] + 0x14) !=
					(matched ? 0 : static_cast<void*>(otherBody)))
				++candidateStateMismatches;
			for(unsigned word = 0; word < 12; ++word)
				{
				const unsigned expectedPose = matched ? globalPoseBits[word]
					: 0x3e000000u + i * 0x100u + word;
				if(*reinterpret_cast<unsigned*>(candidateEmitters[i] + 0x18 + word * 4)
					!= expectedPose)
					++candidateStateMismatches;
				}
			}
		unsigned callbackMismatches = 0;
		static const unsigned expectedKinds[] = { 32, 64, 128, 32, 64, 128 };
		static const unsigned expectedEmitterIndices[] = { 0, 0, 0, 2, 2, 2 };
		for(unsigned i = 0; i < 6; ++i)
			{
			NxFluidEmitterFlagCallbackRecord candidateRecord = {};
			if(!getCallbackRecord(i, &candidateRecord))
				{ ++callbackMismatches; continue; }
			const unsigned expectedEmitter = emitterHandles[expectedEmitterIndices[i]];
			if(candidateRecord.kind != expectedKinds[i]
				|| candidateRecord.argument0 != 0xa1b2c3d4u
				|| candidateRecord.argument1 != 0x55667788u
				|| candidateRecord.argument2 != expectedEmitter
				|| candidateRecord.enabled != 0)
				++callbackMismatches;
			if(candidateRecord.kind == 64
				&& memcmp(candidateRecord.pose, globalPoseBits, sizeof(globalPoseBits)) != 0)
				++callbackMismatches;
			if(memcmp(&candidateRecord, &oracleRecords[i], sizeof(candidateRecord)) != 0)
				++callbackMismatches;
			}
		gFailures += oracleStateMismatches + candidateStateMismatches + callbackMismatches;
		printf("fluid manager actor-release fluids=1 emitters=3 matched=2 "
			"oracle_callbacks=%u candidate_callbacks=%u state_mismatches=%u "
			"callback_mismatches=%u\n", oraclePoseCallbackCount,
			candidatePoseCallbackCount,
			oracleStateMismatches + candidateStateMismatches, callbackMismatches);
		}

	// phys_fn_003622's nonzero branch attaches an emitter to an actor while
	// preserving its backend world pose and deriving the corresponding local
	// pose. A quarter-turn actor frame exercises rotation cross terms.
	typedef int (__thiscall *OracleEmitterSetActor)(void*, void*);
	typedef int (__cdecl *CandidateEmitterSetActor)(void*, void*);
	OracleEmitterSetActor oracleSetActor = reinterpret_cast<OracleEmitterSetActor>(
		image + 0x898d0);
	CandidateEmitterSetActor candidateSetActor =
		reinterpret_cast<CandidateEmitterSetActor>(candidateSetActorAddress);
	unsigned char oracleAttachInternal[0x90], candidateAttachInternal[0x90];
	unsigned char oracleAttachFluid[0x84], candidateAttachFluid[0x84];
	unsigned char oracleAttachBackend[0x34], candidateAttachBackend[0x34];
	unsigned char actorFrame[0x50];
	memset(oracleAttachInternal, 0, sizeof(oracleAttachInternal));
	memset(candidateAttachInternal, 0, sizeof(candidateAttachInternal));
	memset(oracleAttachFluid, 0, sizeof(oracleAttachFluid));
	memset(candidateAttachFluid, 0, sizeof(candidateAttachFluid));
	memset(oracleAttachBackend, 0, sizeof(oracleAttachBackend));
	memset(candidateAttachBackend, 0, sizeof(candidateAttachBackend));
	memset(actorFrame, 0, sizeof(actorFrame));
	*reinterpret_cast<void**>(oracleAttachInternal + 4) = oracleAttachFluid;
	*reinterpret_cast<void**>(candidateAttachInternal + 4) = candidateAttachFluid;
	*reinterpret_cast<unsigned*>(oracleAttachInternal + 8) = 0x33445566u;
	*reinterpret_cast<unsigned*>(candidateAttachInternal + 8) = 0x33445566u;
	*reinterpret_cast<void**>(oracleAttachFluid + 0x7c) = oracleAttachBackend;
	*reinterpret_cast<void**>(candidateAttachFluid + 0x7c) = candidateAttachBackend;
	*reinterpret_cast<unsigned*>(oracleAttachFluid + 0x80) = 0x55667788u;
	*reinterpret_cast<unsigned*>(candidateAttachFluid + 0x80) = 0x55667788u;
	*reinterpret_cast<unsigned*>(oracleAttachBackend + 0x30) = 0xa1b2c3d4u;
	*reinterpret_cast<unsigned*>(candidateAttachBackend + 0x30) = 0xa1b2c3d4u;
	float actorPose[12] = {
		0, -1, 0, 1, 0, 0, 0, 0, 1, 2, -3, 1.5f };
	memcpy(actorFrame + 0x20, actorPose, sizeof(actorPose));
	static const unsigned attachGlobalPoseBits[12] = {
		0x3f800000u, 0, 0, 0, 0x3f800000u, 0,
		0, 0, 0x3f800000u, 0x41200000u, 0x41a00000u, 0x41f00000u };
	memcpy(gOracleEmitterGlobalPose, attachGlobalPoseBits, sizeof(attachGlobalPoseBits));
	setFixturePose(reinterpret_cast<const float*>(attachGlobalPoseBits));
	gOracleFlagCallbackCount = 0;
	const int oracleAttachResult = oracleSetActor(oracleAttachInternal, actorFrame);
	NxFluidEmitterFlagCallbackRecord oracleAttachCallbacks[3] = {};
	const unsigned oracleAttachCallbackCount = gOracleFlagCallbackCount;
	if(oracleAttachCallbackCount == 3)
		memcpy(oracleAttachCallbacks, gOracleFlagCallbacks, sizeof(oracleAttachCallbacks));
	resetCallbacks();
	setFixturePose(reinterpret_cast<const float*>(attachGlobalPoseBits));
	const int candidateAttachResult = candidateSetActor(candidateAttachInternal, actorFrame);
	NxFluidEmitterFlagCallbackRecord candidateAttachCallbacks[3] = {};
	const unsigned candidateAttachCallbackCount = callbackCount();
	for(unsigned i = 0; i < candidateAttachCallbackCount && i < 3; ++i)
		getCallbackRecord(i, &candidateAttachCallbacks[i]);
	unsigned attachMismatches = oracleAttachCallbackCount == 3
		&& candidateAttachCallbackCount == 3 ? 0 : 1;
	if(oracleAttachResult != candidateAttachResult
		|| *reinterpret_cast<void**>(oracleAttachInternal + 0x14) != actorFrame
		|| *reinterpret_cast<void**>(candidateAttachInternal + 0x14) != actorFrame
		|| memcmp(oracleAttachInternal + 0x18, candidateAttachInternal + 0x18, 0x30) != 0
		|| memcmp(oracleAttachInternal + 0x3c, candidateAttachInternal + 0x3c, 0x0c) != 0
		|| memcmp(oracleAttachCallbacks, candidateAttachCallbacks,
			sizeof(oracleAttachCallbacks)) != 0)
		++attachMismatches;
	if(oracleAttachResult != candidateAttachResult)
		printf("fluid emitter attach result oracle=%d candidate=%d\n",
			oracleAttachResult, candidateAttachResult);
	if(memcmp(oracleAttachInternal + 0x18, candidateAttachInternal + 0x18, 0x30) != 0)
		{
		printf("fluid emitter attach local pose differs");
		for(unsigned i = 0; i < 12; ++i)
			printf(" %08x/%08x", *reinterpret_cast<unsigned*>(oracleAttachInternal + 0x18 + i * 4),
				*reinterpret_cast<unsigned*>(candidateAttachInternal + 0x18 + i * 4));
		printf("\n");
		}
	if(memcmp(oracleAttachInternal + 0x3c, candidateAttachInternal + 0x3c, 0x0c) != 0)
		printf("fluid emitter attach local position differs\n");
	for(unsigned i = 0; i < 3; ++i)
		if(memcmp(&oracleAttachCallbacks[i], &candidateAttachCallbacks[i],
			sizeof(oracleAttachCallbacks[i])) != 0)
			printf("fluid emitter attach callback %u differs kind=%u/%u\n", i,
				oracleAttachCallbacks[i].kind, candidateAttachCallbacks[i].kind);
	gFailures += attachMismatches;
	printf("fluid emitter attach matrix-frame cases=1 oracle_callbacks=%u "
		"candidate_callbacks=%u mismatches=%u\n", oracleAttachCallbackCount,
		candidateAttachCallbackCount, attachMismatches);

	// Exercise the second actor-pose source: a dynamic body record with its
	// quaternion and translation stored in the body layout.
	unsigned char bodyRecord[0x40];
	memset(bodyRecord, 0, sizeof(bodyRecord));
	*reinterpret_cast<void**>(actorFrame + 8) = bodyRecord;
	float* bodyState = reinterpret_cast<float*>(bodyRecord);
	bodyState[6] = 2.0f; bodyState[7] = -3.0f; bodyState[8] = 1.5f;
	bodyState[9] = 0.5f; bodyState[10] = 0.5f;
	bodyState[11] = 0.5f; bodyState[12] = 0.5f;
	memset(oracleAttachInternal, 0, sizeof(oracleAttachInternal));
	memset(candidateAttachInternal, 0, sizeof(candidateAttachInternal));
	*reinterpret_cast<void**>(oracleAttachInternal + 4) = oracleAttachFluid;
	*reinterpret_cast<void**>(candidateAttachInternal + 4) = candidateAttachFluid;
	*reinterpret_cast<unsigned*>(oracleAttachInternal + 8) = 0x33445566u;
	*reinterpret_cast<unsigned*>(candidateAttachInternal + 8) = 0x33445566u;
	memcpy(gOracleEmitterGlobalPose, attachGlobalPoseBits, sizeof(attachGlobalPoseBits));
	setFixturePose(reinterpret_cast<const float*>(attachGlobalPoseBits));
	gOracleFlagCallbackCount = 0;
	const int oracleBodyAttachResult = oracleSetActor(oracleAttachInternal, actorFrame);
	NxFluidEmitterFlagCallbackRecord oracleBodyAttachCallbacks[3] = {};
	const unsigned oracleBodyAttachCallbackCount = gOracleFlagCallbackCount;
	if(oracleBodyAttachCallbackCount == 3)
		memcpy(oracleBodyAttachCallbacks, gOracleFlagCallbacks,
			sizeof(oracleBodyAttachCallbacks));
	resetCallbacks();
	setFixturePose(reinterpret_cast<const float*>(attachGlobalPoseBits));
	const int candidateBodyAttachResult = candidateSetActor(candidateAttachInternal, actorFrame);
	NxFluidEmitterFlagCallbackRecord candidateBodyAttachCallbacks[3] = {};
	const unsigned candidateBodyAttachCallbackCount = callbackCount();
	for(unsigned i = 0; i < candidateBodyAttachCallbackCount && i < 3; ++i)
		getCallbackRecord(i, &candidateBodyAttachCallbacks[i]);
	unsigned bodyAttachMismatches = oracleBodyAttachCallbackCount == 3
		&& candidateBodyAttachCallbackCount == 3 ? 0 : 1;
	if(oracleBodyAttachResult != candidateBodyAttachResult
		|| memcmp(oracleAttachInternal + 0x18, candidateAttachInternal + 0x18, 0x30) != 0
		|| memcmp(oracleBodyAttachCallbacks, candidateBodyAttachCallbacks,
			sizeof(oracleBodyAttachCallbacks)) != 0)
		++bodyAttachMismatches;
	if(memcmp(oracleAttachInternal + 0x18, candidateAttachInternal + 0x18, 0x30) != 0)
		{
		printf("fluid emitter body local pose differs");
		for(unsigned i = 0; i < 12; ++i)
			printf(" %08x/%08x", *reinterpret_cast<unsigned*>(oracleAttachInternal + 0x18 + i * 4),
				*reinterpret_cast<unsigned*>(candidateAttachInternal + 0x18 + i * 4));
		printf("\n");
		}
	for(unsigned i = 0; i < 3; ++i)
		if(memcmp(&oracleBodyAttachCallbacks[i], &candidateBodyAttachCallbacks[i],
			sizeof(oracleBodyAttachCallbacks[i])) != 0)
			printf("fluid emitter body callback %u differs\n", i);
	gFailures += bodyAttachMismatches;
	printf("fluid emitter attach quaternion-frame cases=1 oracle_callbacks=%u "
		"candidate_callbacks=%u mismatches=%u\n", oracleBodyAttachCallbackCount,
		candidateBodyAttachCallbackCount, bodyAttachMismatches);

	// phys_fn_003605 republishes the emitter's local pose in world space using
	// either its matrix-backed actor frame or its dynamic body's quaternion.
	// The callback transcript is the observable full 12-word world transform.
	if(candidateUpdatePoseAddress)
		{
		typedef int (__thiscall *OracleUpdatePose)(void*);
		typedef int (__cdecl *CandidateUpdatePose)(void*);
		OracleUpdatePose oracleUpdatePose = reinterpret_cast<OracleUpdatePose>(
			image + 0x88310);
		CandidateUpdatePose candidateUpdatePose = reinterpret_cast<CandidateUpdatePose>(
			candidateUpdatePoseAddress);
		static const float localPoses[3][12] = {
			{ 1, 0.25f, -0.125f, -0.5f, 1, 0.375f, 0.2f, -0.3f, 1, 1.25f, -2.5f, 4 },
			{ 0.812345f, -0.432109f, 0.123457f, 0.456789f, 0.901237f, -0.234567f,
			  0.098765f, 0.345679f, 0.765431f, -0.75319f, 2.24681f, 3.51234f },
			{ 0.513579f, 0.127531f, -0.251357f, 0.375319f, 0.751937f, 0.203579f,
			  -0.407531f, 0.109753f, 0.987531f, 4.01234f, -1.54321f, 0.257913f }
		};
		static const float matrixActorPose[12] = {
			0.812345f, -0.573219f, 0.105679f, 0.581357f, 0.798461f, -0.153791f,
			0.024681f, 0.184357f, 0.982531f, 2.12345f, -3.54321f, 1.67891f };
		unsigned worldPoseMismatches = candidateUpdatePoseAddress ? 0 : 1;
		for(unsigned poseCase = 0; poseCase < 3; ++poseCase)
			{
			unsigned char oracleInternal[0x90], candidateInternal[0x90];
			unsigned char oracleFluid[0x84], candidateFluid[0x84];
			unsigned char oracleBackend[0x34], candidateBackend[0x34];
			unsigned char oracleActor[0x50], candidateActor[0x50];
			unsigned char oracleBody[0x40], candidateBody[0x40];
			memset(oracleInternal, 0, sizeof(oracleInternal));
			memset(candidateInternal, 0, sizeof(candidateInternal));
			memset(oracleFluid, 0, sizeof(oracleFluid));
			memset(candidateFluid, 0, sizeof(candidateFluid));
			memset(oracleBackend, 0, sizeof(oracleBackend));
			memset(candidateBackend, 0, sizeof(candidateBackend));
			memset(oracleActor, 0, sizeof(oracleActor));
			memset(candidateActor, 0, sizeof(candidateActor));
			memset(oracleBody, 0, sizeof(oracleBody));
			memset(candidateBody, 0, sizeof(candidateBody));
			*reinterpret_cast<void**>(oracleInternal + 4) = oracleFluid;
			*reinterpret_cast<void**>(candidateInternal + 4) = candidateFluid;
			*reinterpret_cast<unsigned*>(oracleInternal + 8) = 0x33445566u;
			*reinterpret_cast<unsigned*>(candidateInternal + 8) = 0x33445566u;
			*reinterpret_cast<void**>(oracleFluid + 0x7c) = oracleBackend;
			*reinterpret_cast<void**>(candidateFluid + 0x7c) = candidateBackend;
			*reinterpret_cast<unsigned*>(oracleFluid + 0x80) = 0x55667788u;
			*reinterpret_cast<unsigned*>(candidateFluid + 0x80) = 0x55667788u;
			*reinterpret_cast<unsigned*>(oracleBackend + 0x30) = 0xa1b2c3d4u;
			*reinterpret_cast<unsigned*>(candidateBackend + 0x30) = 0xa1b2c3d4u;
			memcpy(oracleInternal + 0x18, localPoses[poseCase], 0x30);
			memcpy(candidateInternal + 0x18, localPoses[poseCase], 0x30);
			if(poseCase != 0)
				{
				*reinterpret_cast<void**>(oracleInternal + 0x14) = oracleActor;
				*reinterpret_cast<void**>(candidateInternal + 0x14) = candidateActor;
				if(poseCase == 1)
					{
					memcpy(oracleActor + 0x20, matrixActorPose, 0x30);
					memcpy(candidateActor + 0x20, matrixActorPose, 0x30);
					}
				else
					{
					*reinterpret_cast<void**>(oracleActor + 8) = oracleBody;
					*reinterpret_cast<void**>(candidateActor + 8) = candidateBody;
					float* oracleState = reinterpret_cast<float*>(oracleBody);
					float* candidateState = reinterpret_cast<float*>(candidateBody);
					for(unsigned i = 0; i < 3; ++i)
						oracleState[6 + i] = candidateState[6 + i] = matrixActorPose[9 + i];
					oracleState[9] = candidateState[9] = 0.173219f;
					oracleState[10] = candidateState[10] = -0.319753f;
					oracleState[11] = candidateState[11] = 0.427531f;
					oracleState[12] = candidateState[12] = 0.811357f;
					}
				}
			unsigned oracleEspBefore = 0, oracleEspAfter = 0;
			unsigned candidateEspBefore = 0, candidateEspAfter = 0;
			gOracleFlagCallbackCount = 0;
			__asm mov oracleEspBefore, esp
			const int oraclePoseResult = oracleUpdatePose(oracleInternal);
			__asm mov oracleEspAfter, esp
			NxFluidEmitterFlagCallbackRecord oraclePoseCallback = {};
			const unsigned oraclePoseCallbackCount = gOracleFlagCallbackCount;
			if(oraclePoseCallbackCount == 1)
				oraclePoseCallback = gOracleFlagCallbacks[0];
			resetCallbacks();
			__asm mov candidateEspBefore, esp
			const int candidatePoseResult = candidateUpdatePose(candidateInternal);
			__asm mov candidateEspAfter, esp
			NxFluidEmitterFlagCallbackRecord candidatePoseCallback = {};
			const unsigned candidatePoseCallbackCount = callbackCount();
			if(candidatePoseCallbackCount == 1)
				getCallbackRecord(0, &candidatePoseCallback);
			const bool matched = oraclePoseCallbackCount == 1
				&& candidatePoseCallbackCount == 1
				&& oraclePoseResult == candidatePoseResult
				&& oracleEspBefore == oracleEspAfter
				&& candidateEspBefore == candidateEspAfter
				&& memcmp(&oraclePoseCallback, &candidatePoseCallback,
					sizeof(oraclePoseCallback)) == 0;
			if(!matched)
				{
				++worldPoseMismatches;
				printf("fluid emitter world-pose case=%u oracle_ret=%d candidate_ret=%d "
					"oracle_callbacks=%u candidate_callbacks=%u stack=%u/%u\n",
					poseCase, oraclePoseResult, candidatePoseResult,
					oraclePoseCallbackCount, candidatePoseCallbackCount,
					oracleEspBefore == oracleEspAfter,
					candidateEspBefore == candidateEspAfter);
				for(unsigned word = 0; word < 12; ++word)
					printf("  pose[%u]=%08x/%08x\n", word,
						oraclePoseCallback.pose[word], candidatePoseCallback.pose[word]);
				}
			}
		gFailures += worldPoseMismatches;
		printf("fluid emitter world-pose cases=3 mismatches=%u\n",
			worldPoseMismatches);
		}

	// phys_fn_003624 publishes body position and velocities after refreshing
	// the emitter world pose. The no-actor arm emits only the pose callback;
	// the attached-body arm emits the three body vectors in oracle order.
	if(candidateUpdateBodyStateAddress)
		{
		typedef int (__thiscall *OracleUpdateBodyState)(void*);
		typedef int (__cdecl *CandidateUpdateBodyState)(void*);
		OracleUpdateBodyState oracleUpdateBodyState =
			reinterpret_cast<OracleUpdateBodyState>(image + 0x89930);
		CandidateUpdateBodyState candidateUpdateBodyState =
			reinterpret_cast<CandidateUpdateBodyState>(candidateUpdateBodyStateAddress);
		static const float localPose[12] = {
			0.91f, -0.13f, 0.27f, 0.19f, 0.87f, -0.31f,
			-0.23f, 0.17f, 0.93f, 1.137f, -2.319f, 0.427f };
		static const unsigned expectedKinds[4] = { 64, 256, 512, 1024 };
		unsigned bodyStateMismatches = 0;
		unsigned totalOracleCallbacks = 0, totalCandidateCallbacks = 0;
		for(unsigned updateCase = 0; updateCase < 2; ++updateCase)
			{
			unsigned char oracleInternal[0x90], candidateInternal[0x90];
			unsigned char oracleFluid[0x84], candidateFluid[0x84];
			unsigned char oracleBackend[0x34], candidateBackend[0x34];
			unsigned char oracleActor[0x10], candidateActor[0x10];
			unsigned char oracleBody[0x180], candidateBody[0x180];
			memset(oracleInternal, 0, sizeof(oracleInternal));
			memset(candidateInternal, 0, sizeof(candidateInternal));
			memset(oracleFluid, 0, sizeof(oracleFluid));
			memset(candidateFluid, 0, sizeof(candidateFluid));
			memset(oracleBackend, 0, sizeof(oracleBackend));
			memset(candidateBackend, 0, sizeof(candidateBackend));
			memset(oracleActor, 0, sizeof(oracleActor));
			memset(candidateActor, 0, sizeof(candidateActor));
			memset(oracleBody, 0, sizeof(oracleBody));
			memset(candidateBody, 0, sizeof(candidateBody));
			*reinterpret_cast<void**>(oracleInternal + 4) = oracleFluid;
			*reinterpret_cast<void**>(candidateInternal + 4) = candidateFluid;
			*reinterpret_cast<unsigned*>(oracleInternal + 8) = 0x13572468u;
			*reinterpret_cast<unsigned*>(candidateInternal + 8) = 0x13572468u;
			*reinterpret_cast<void**>(oracleFluid + 0x7c) = oracleBackend;
			*reinterpret_cast<void**>(candidateFluid + 0x7c) = candidateBackend;
			*reinterpret_cast<unsigned*>(oracleFluid + 0x80) = 0x24681357u;
			*reinterpret_cast<unsigned*>(candidateFluid + 0x80) = 0x24681357u;
			*reinterpret_cast<unsigned*>(oracleBackend + 0x30) = 0x31415926u;
			*reinterpret_cast<unsigned*>(candidateBackend + 0x30) = 0x31415926u;
			memcpy(oracleInternal + 0x18, localPose, sizeof(localPose));
			memcpy(candidateInternal + 0x18, localPose, sizeof(localPose));
			unsigned expectedCount = 1;
			if(updateCase == 1)
				{
				*reinterpret_cast<void**>(oracleInternal + 0x14) = oracleActor;
				*reinterpret_cast<void**>(candidateInternal + 0x14) = candidateActor;
				*reinterpret_cast<void**>(oracleActor + 8) = oracleBody;
				*reinterpret_cast<void**>(candidateActor + 8) = candidateBody;
				static const float bodyVectors[3][3] = {
					{ 12.137f, -4.319f, 0.527f },
					{ -1.713f, 3.091f, 7.227f },
					{ 0.319f, -8.173f, 2.619f } };
				memcpy(oracleBody + 0x158, bodyVectors[0], sizeof(bodyVectors[0]));
				memcpy(candidateBody + 0x158, bodyVectors[0], sizeof(bodyVectors[0]));
				memcpy(oracleBody + 0x34, bodyVectors[1], sizeof(bodyVectors[1]));
				memcpy(candidateBody + 0x34, bodyVectors[1], sizeof(bodyVectors[1]));
				memcpy(oracleBody + 0x40, bodyVectors[2], sizeof(bodyVectors[2]));
				memcpy(candidateBody + 0x40, bodyVectors[2], sizeof(bodyVectors[2]));
				expectedCount = 4;
				}
			gOracleFlagCallbackCount = 0;
			resetCallbacks();
			unsigned oracleEspBefore = 0, oracleEspAfter = 0;
			unsigned candidateEspBefore = 0, candidateEspAfter = 0;
			__asm mov oracleEspBefore, esp
			const int oracleResult = oracleUpdateBodyState(oracleInternal);
			__asm mov oracleEspAfter, esp
			const unsigned oracleCount = gOracleFlagCallbackCount;
			__asm mov candidateEspBefore, esp
			const int candidateResult = candidateUpdateBodyState(candidateInternal);
			__asm mov candidateEspAfter, esp
			const unsigned candidateCount = callbackCount();
			totalOracleCallbacks += oracleCount;
			totalCandidateCallbacks += candidateCount;
			if(oracleResult != candidateResult || oracleCount != expectedCount
				|| candidateCount != expectedCount
				|| oracleEspBefore != oracleEspAfter
				|| candidateEspBefore != candidateEspAfter)
				++bodyStateMismatches;
			for(unsigned callbackIndex = 0; callbackIndex < expectedCount; ++callbackIndex)
				{
				NxFluidEmitterFlagCallbackRecord oracleRecord = {};
				NxFluidEmitterFlagCallbackRecord candidateRecord = {};
				if(callbackIndex >= oracleCount || callbackIndex >= candidateCount
					|| !getCallbackRecord(callbackIndex, &candidateRecord))
					{
					++bodyStateMismatches;
					continue;
				}
				oracleRecord = gOracleFlagCallbacks[callbackIndex];
				if(oracleRecord.kind != expectedKinds[callbackIndex]
					|| memcmp(&oracleRecord, &candidateRecord, sizeof(oracleRecord)) != 0)
					++bodyStateMismatches;
				}
			}
		gFailures += bodyStateMismatches;
		printf("fluid emitter body-state update cases=2 oracle_callbacks=%u "
			"candidate_callbacks=%u mismatches=%u\n", totalOracleCallbacks,
			totalCandidateCallbacks, bodyStateMismatches);
		}

	// phys_fn_003419 dispatches one implicit surface-mesh run using the mesh
	// record's fluid context, fluid handle, mesh handle, and triangle count.
		{
		typedef int (__thiscall *OracleGenerateSurfaceMesh)(void*);
		typedef int (__cdecl *CandidateGenerateSurfaceMesh)(void*);
		OracleGenerateSurfaceMesh oracleGenerateSurfaceMesh =
			reinterpret_cast<OracleGenerateSurfaceMesh>(image + 0x84a50);
		CandidateGenerateSurfaceMesh candidateGenerateSurfaceMesh =
			reinterpret_cast<CandidateGenerateSurfaceMesh>(candidateGenerateSurfaceMeshAddress);
		unsigned char oracleEmitter[0x28], candidateEmitter[0x28];
		unsigned char oracleMesh[0x2c], candidateMesh[0x2c];
		unsigned char oracleFluid[0x84], candidateFluid[0x84];
		unsigned char oracleBackend[0x34], candidateBackend[0x34];
		memset(oracleEmitter, 0, sizeof(oracleEmitter));
		memset(candidateEmitter, 0, sizeof(candidateEmitter));
		memset(oracleMesh, 0, sizeof(oracleMesh));
		memset(candidateMesh, 0, sizeof(candidateMesh));
		memset(oracleFluid, 0, sizeof(oracleFluid));
		memset(candidateFluid, 0, sizeof(candidateFluid));
		memset(oracleBackend, 0, sizeof(oracleBackend));
		memset(candidateBackend, 0, sizeof(candidateBackend));
		*reinterpret_cast<void**>(oracleEmitter + 0x24) = oracleMesh;
		*reinterpret_cast<void**>(candidateEmitter + 0x24) = candidateMesh;
		*reinterpret_cast<void**>(oracleMesh + 4) = oracleFluid;
		*reinterpret_cast<void**>(candidateMesh + 4) = candidateFluid;
		*reinterpret_cast<unsigned*>(oracleMesh + 0x18) = 0x44556677u;
		*reinterpret_cast<unsigned*>(candidateMesh + 0x18) = 0x44556677u;
		*reinterpret_cast<unsigned*>(oracleMesh + 0x28) = 21;
		*reinterpret_cast<unsigned*>(candidateMesh + 0x28) = 21;
		*reinterpret_cast<void**>(oracleFluid + 0x7c) = oracleBackend;
		*reinterpret_cast<void**>(candidateFluid + 0x7c) = candidateBackend;
		*reinterpret_cast<unsigned*>(oracleFluid + 0x80) = 0x12345678u;
		*reinterpret_cast<unsigned*>(candidateFluid + 0x80) = 0x12345678u;
		*reinterpret_cast<unsigned*>(oracleBackend + 0x30) = 0x87654321u;
		*reinterpret_cast<unsigned*>(candidateBackend + 0x30) = 0x87654321u;
		gOracleFlagCallbackCount = 0;
		resetCallbacks();
		unsigned oracleEspBefore = 0, oracleEspAfter = 0;
		unsigned candidateEspBefore = 0, candidateEspAfter = 0;
		__asm mov oracleEspBefore, esp
		const int oracleResult = oracleGenerateSurfaceMesh(oracleEmitter);
		__asm mov oracleEspAfter, esp
		const unsigned oracleCount = gOracleFlagCallbackCount;
		NxFluidEmitterFlagCallbackRecord oracleRecord = {};
		if(oracleCount == 1)
			oracleRecord = gOracleFlagCallbacks[0];
		int candidateResult = 0;
		if(candidateGenerateSurfaceMesh)
			{
			__asm mov candidateEspBefore, esp
			candidateResult = candidateGenerateSurfaceMesh(candidateEmitter);
			__asm mov candidateEspAfter, esp
			}
		else
			candidateEspBefore = candidateEspAfter = 1;
		NxFluidEmitterFlagCallbackRecord candidateRecord = {};
		const unsigned candidateCount = callbackCount();
		const bool gotCandidateRecord = candidateCount == 1
			&& getCallbackRecord(0, &candidateRecord) != 0;
		const unsigned expectedArguments[4] = {
			0x87654321u, 0x12345678u, 0x44556677u, 7u };
		const bool oracleArgumentsMatch = oracleCount == 1
			&& oracleRecord.kind == 2048
			&& oracleRecord.argument0 == expectedArguments[0]
			&& oracleRecord.argument1 == expectedArguments[1]
			&& oracleRecord.argument2 == expectedArguments[2]
			&& oracleRecord.enabled == expectedArguments[3];
		const bool candidateArgumentsMatch = gotCandidateRecord
			&& candidateRecord.kind == oracleRecord.kind
			&& candidateRecord.argument0 == oracleRecord.argument0
			&& candidateRecord.argument1 == oracleRecord.argument1
			&& candidateRecord.argument2 == oracleRecord.argument2
			&& candidateRecord.enabled == oracleRecord.enabled;
		const unsigned meshMismatches = !oracleArgumentsMatch || !candidateArgumentsMatch
			|| oracleResult != candidateResult || oracleEspBefore != oracleEspAfter
			|| candidateEspBefore != candidateEspAfter;
		gFailures += meshMismatches;
		printf("fluid surface-mesh dispatch cases=1 oracle_callbacks=%u "
			"candidate_callbacks=%u args=%u return=%u stack=%u/%u mismatches=%u\n",
			oracleCount, candidateCount, oracleArgumentsMatch && candidateArgumentsMatch,
			oracleResult == candidateResult, oracleEspBefore == oracleEspAfter,
			candidateEspBefore == candidateEspAfter, meshMismatches);
		*reinterpret_cast<void**>(oracleEmitter + 0x24) = 0;
		*reinterpret_cast<void**>(candidateEmitter + 0x24) = 0;
		gOracleFlagCallbackCount = 0;
		resetCallbacks();
		__asm mov oracleEspBefore, esp
		oracleGenerateSurfaceMesh(oracleEmitter);
		__asm mov oracleEspAfter, esp
		if(candidateGenerateSurfaceMesh)
			{
			__asm mov candidateEspBefore, esp
			candidateGenerateSurfaceMesh(candidateEmitter);
			__asm mov candidateEspAfter, esp
			}
		else
			candidateEspBefore = candidateEspAfter = 1;
		const unsigned emptyMeshMismatches = gOracleFlagCallbackCount != 0
			|| callbackCount() != 0 || oracleEspBefore != oracleEspAfter
			|| candidateEspBefore != candidateEspAfter;
		gFailures += emptyMeshMismatches;
		printf("fluid surface-mesh empty cases=1 oracle_callbacks=%u "
			"candidate_callbacks=%u stack=%u/%u mismatches=%u\n",
			gOracleFlagCallbackCount, callbackCount(),
			oracleEspBefore == oracleEspAfter,
			candidateEspBefore == candidateEspAfter, emptyMeshMismatches);
		}

	// phys_fn_003632 and continuation 003634 walk every manager fluid and
	// dispatch its implicit surface-mesh update. A populated manager plus the
	// callback fixture makes that loop observable despite the absent engine
	// FluidModel module.
	{
		typedef void (__thiscall *OracleGenerateFluidMeshes)(void*);
		typedef void (__cdecl *CandidateGenerateFluidMeshes)(void*);
		OracleGenerateFluidMeshes oracleGenerateFluidMeshes =
			reinterpret_cast<OracleGenerateFluidMeshes>(image + 0x89c80);
		CandidateGenerateFluidMeshes candidateGenerateFluidMeshes =
			reinterpret_cast<CandidateGenerateFluidMeshes>(
				candidateFluidManagerGenerateSurfaceMeshesAddress);
		unsigned char oracleManager[0x34] = {};
		unsigned char candidateManager[0x34] = {};
		unsigned oracleFluids[2], candidateFluids[2];
		unsigned char oracleFluidInternals[2][0x28] = {};
		unsigned char candidateFluidInternals[2][0x28] = {};
		unsigned char oracleMeshes[2][0x2c] = {};
		unsigned char candidateMeshes[2][0x2c] = {};
		unsigned char oracleCallbackFluids[2][0x84] = {};
		unsigned char candidateCallbackFluids[2][0x84] = {};
		unsigned char oracleBackends[2][0x34] = {};
		unsigned char candidateBackends[2][0x34] = {};
		for(unsigned i = 0; i != 2; ++i)
			{
			oracleFluids[i] = reinterpret_cast<unsigned>(oracleFluidInternals[i]);
			candidateFluids[i] = reinterpret_cast<unsigned>(candidateFluidInternals[i]);
			*reinterpret_cast<void**>(oracleFluidInternals[i] + 0x24) = oracleMeshes[i];
			*reinterpret_cast<void**>(candidateFluidInternals[i] + 0x24) = candidateMeshes[i];
			*reinterpret_cast<void**>(oracleMeshes[i] + 4) = oracleCallbackFluids[i];
			*reinterpret_cast<void**>(candidateMeshes[i] + 4) = candidateCallbackFluids[i];
			*reinterpret_cast<unsigned*>(oracleMeshes[i] + 0x18) = 0x44556670u + i;
			*reinterpret_cast<unsigned*>(candidateMeshes[i] + 0x18) = 0x44556670u + i;
			*reinterpret_cast<unsigned*>(oracleMeshes[i] + 0x28) = 21;
			*reinterpret_cast<unsigned*>(candidateMeshes[i] + 0x28) = 21;
			*reinterpret_cast<void**>(oracleCallbackFluids[i] + 0x7c) = oracleBackends[i];
			*reinterpret_cast<void**>(candidateCallbackFluids[i] + 0x7c) = candidateBackends[i];
			*reinterpret_cast<unsigned*>(oracleCallbackFluids[i] + 0x80) = 0x12345670u + i;
			*reinterpret_cast<unsigned*>(candidateCallbackFluids[i] + 0x80) = 0x12345670u + i;
			*reinterpret_cast<unsigned*>(oracleBackends[i] + 0x30) = 0x87654320u + i;
			*reinterpret_cast<unsigned*>(candidateBackends[i] + 0x30) = 0x87654320u + i;
			}
		*reinterpret_cast<unsigned*>(oracleManager + 4) = reinterpret_cast<unsigned>(oracleFluids);
		*reinterpret_cast<unsigned*>(oracleManager + 8) = reinterpret_cast<unsigned>(oracleFluids + 2);
		*reinterpret_cast<unsigned*>(candidateManager + 4) = reinterpret_cast<unsigned>(candidateFluids);
		*reinterpret_cast<unsigned*>(candidateManager + 8) = reinterpret_cast<unsigned>(candidateFluids + 2);
		oracleManager[0x2b] = candidateManager[0x2b] = 1;
		gOracleFlagCallbackCount = 0;
		resetCallbacks();
		oracleGenerateFluidMeshes(&oracleManager);
		const unsigned oracleCallbacks = gOracleFlagCallbackCount;
		NxFluidEmitterFlagCallbackRecord oracleRecords[2] = {};
		for(unsigned i = 0; i < oracleCallbacks && i < 2; ++i)
			oracleRecords[i] = gOracleFlagCallbacks[i];
		if(candidateGenerateFluidMeshes)
			candidateGenerateFluidMeshes(&candidateManager);
		const unsigned candidateCallbacks = callbackCount();
		unsigned callbackMismatches = oracleCallbacks != 2 || candidateCallbacks != 2;
		for(unsigned i = 0; i < 2; ++i)
			{
			NxFluidEmitterFlagCallbackRecord candidateRecord = {};
			if(i >= oracleCallbacks || !getCallbackRecord(i, &candidateRecord))
				{ ++callbackMismatches; continue; }
			const NxFluidEmitterFlagCallbackRecord& oracleRecord = oracleRecords[i];
			if(oracleRecord.kind != 2048 || candidateRecord.kind != oracleRecord.kind
				|| candidateRecord.argument0 != oracleRecord.argument0
				|| candidateRecord.argument1 != oracleRecord.argument1
				|| candidateRecord.argument2 != oracleRecord.argument2
				|| candidateRecord.enabled != oracleRecord.enabled)
				++callbackMismatches;
			}
		gFailures += callbackMismatches;
		printf("fluid manager surface-mesh traversal fluids=2 oracle_callbacks=%u candidate_callbacks=%u mismatches=%u\n",
			oracleCallbacks, candidateCallbacks, callbackMismatches);
		}

	{
	// This public wrapper acquires the oracle's real x86 read/write lock. The
	// surrounding fluid-emitter probe replaces lock imports with deterministic
	// stubs, which would make this wrapper report a false write-lock collision.
	nxUnbindOracleLocks(image, lockSlots);
	ImplicitMeshOutputStream output;
	typedef NxFoundationSDK* (NX_CALL_CONV* CreateFoundationFn)(NxU32,
		NxUserOutputStream*, NxUserAllocator*);
	CreateFoundationFn createFoundation = reinterpret_cast<CreateFoundationFn>(
		GetProcAddress(foundationModule, "NxCreateFoundationSDK"));
	NxFoundationSDK* foundation = createFoundation
		? createFoundation(NX_FOUNDATION_SDK_VERSION, &output, 0) : 0;
	if(!foundation)
		{
		fprintf(stderr, "FAIL Foundation SDK creation failed for implicit-mesh validation cases\n");
		++gFailures;
		}
	else if(candidateSetMeshDataAddress && candidateSetMeshDataWrapperAddress)
		gFailures += nxRunImplicitMeshSetDataDifferential(image + 0x8ac50,
			candidateSetMeshDataAddress, candidateSetMeshDataWrapperAddress,
			candidateApiReentryGetter, candidateApiReentrySetter, foundation, output);
	else
		{
		foundation->release();
		printf("implicit mesh setMeshData valid_cases=0 invalid_cases=0 errors=0 "
			"stack_checks=0 mismatches=1\n");
		++gFailures;
		}
	}

	// Run this last because the oracle constructor installs process-global
	// FluidModel callbacks. Keeping this probe after the other ABI rows prevents
	// its setup from changing their independent callback baselines.
	{
		struct FluidManagerCallbackSummary
			{
			unsigned managerCreate, managerDestroy, fluidCreate, fluidDestroy;
			unsigned staticCollisionCreate, staticCollisionGet, fluidGetCellSize;
			unsigned staticCollisionDestroy;
			unsigned fluidCreateArguments[15];
			};
		typedef void (__cdecl *ResetManagerCallbacksFn)();
		typedef void (__cdecl *GetManagerCallbackSummaryFn)(
			FluidManagerCallbackSummary*);
		typedef void* (__cdecl *GetStaticCollisionFn)(void*);
		ResetManagerCallbacksFn resetManagerCallbacks = reinterpret_cast<
			ResetManagerCallbacksFn>(GetProcAddress(fluidModel,
				"NxResetFluidManagerCallbacks"));
		typedef void (__cdecl *SetManagerCollisionsFn)(void*, void*);
		SetManagerCollisionsFn setManagerCollisions = reinterpret_cast<
			SetManagerCollisionsFn>(GetProcAddress(fluidModel,
				"NxSetFluidManagerStaticCollisions"));
		GetManagerCallbackSummaryFn getManagerCallbackSummary = reinterpret_cast<
			GetManagerCallbackSummaryFn>(GetProcAddress(fluidModel,
				"NxGetFluidManagerCallbackSummary"));
		GetStaticCollisionFn getStaticCollision = reinterpret_cast<
			GetStaticCollisionFn>(GetProcAddress(fluidModel,
				"FluidManagerGetStaticCollision"));
		void* candidateManagerStepAddress = 0;
		const bool probesReady = resetManagerCallbacks && setManagerCollisions &&
			getManagerCallbackSummary && getStaticCollision &&
			nxResolveCandidateFluidEmitterSymbol(candidatePhysics,
				"@nxFluidManagerStep@12", &candidateManagerStepAddress);
		unsigned mismatches = probesReady ? 0u : 1u;
		if(probesReady)
			{
			typedef int (__thiscall *ManagerStep)(void*, float);
			typedef void* (__thiscall *ManagerConstructor)(void*, void*);
			ManagerStep oracleStep = reinterpret_cast<ManagerStep>(image + 0x89bd0);
			ManagerStep candidateStep = reinterpret_cast<ManagerStep>(candidateManagerStepAddress);
			ManagerConstructor oracleConstructor = reinterpret_cast<ManagerConstructor>(
				image + 0x89f50);
			unsigned char oracleManager[0x34] = {};
			unsigned char candidateManager[0x34] = {};
			unsigned char oracleManagerInit[0x34] = {};
			oracleConstructor(oracleManagerInit, 0);
			void* const backendManager =
				*reinterpret_cast<void**>(oracleManagerInit + 0x30);
			unsigned char collisionScene[0x800] = {};
			unsigned oracleFluidWords[2] = {}, candidateFluidWords[2] = {};
			unsigned char oracleFluids[2][0x84] = {}, candidateFluids[2][0x84] = {};
			unsigned char oracleBackends[2][0x534] = {}, candidateBackends[2][0x534] = {};
			unsigned char oracleStates[2][8] = {}, candidateStates[2][8] = {};
			unsigned char oracleCores[2][0x530] = {}, candidateCores[2][0x530] = {};
			const unsigned expectedVectors[2][3] = {
				{ 0x3e800001u, 0xbf000002u, 0x3f400003u },
				{ 0x40000001u, 0xc0400002u, 0x40800003u } };
			for(unsigned i = 0; i != 2; ++i)
				{
				oracleFluidWords[i] = reinterpret_cast<unsigned>(oracleFluids[i]);
				candidateFluidWords[i] = reinterpret_cast<unsigned>(candidateFluids[i]);
				memcpy(oracleCores[i] + 0x520, expectedVectors[i], sizeof(expectedVectors[i]));
				memcpy(candidateCores[i] + 0x520, expectedVectors[i], sizeof(expectedVectors[i]));
				*reinterpret_cast<void**>(oracleBackends[i] + 0x24) = oracleCores[i];
				*reinterpret_cast<void**>(candidateBackends[i] + 0x24) = candidateCores[i];
				*reinterpret_cast<unsigned*>(oracleBackends[i] + 0x30) = 0x10203040u + i;
				*reinterpret_cast<unsigned*>(candidateBackends[i] + 0x30) = 0x10203040u + i;
				*reinterpret_cast<void**>(oracleFluids[i] + 0x78) = oracleStates[i];
				*reinterpret_cast<void**>(candidateFluids[i] + 0x78) = candidateStates[i];
				*reinterpret_cast<unsigned*>(oracleStates[i] + 4) = 0xaaaaaaaau;
				*reinterpret_cast<unsigned*>(candidateStates[i] + 4) = 0xaaaaaaaau;
				*reinterpret_cast<void**>(oracleFluids[i] + 0x7c) = oracleBackends[i];
				*reinterpret_cast<void**>(candidateFluids[i] + 0x7c) = candidateBackends[i];
				*reinterpret_cast<unsigned*>(oracleFluids[i] + 0x80) = 0x55667780u + i;
				*reinterpret_cast<unsigned*>(candidateFluids[i] + 0x80) = 0x55667780u + i;
				}
			*reinterpret_cast<void***>(oracleManager + 4) =
				reinterpret_cast<void**>(oracleFluidWords);
			*reinterpret_cast<void***>(oracleManager + 8) =
				reinterpret_cast<void**>(oracleFluidWords + 2);
			*reinterpret_cast<void***>(candidateManager + 4) =
				reinterpret_cast<void**>(candidateFluidWords);
			*reinterpret_cast<void***>(candidateManager + 8) =
				reinterpret_cast<void**>(candidateFluidWords + 2);
			*reinterpret_cast<void**>(oracleManager + 0x24) = collisionScene;
			*reinterpret_cast<void**>(candidateManager + 0x24) = collisionScene;
			*reinterpret_cast<unsigned*>(oracleManager + 0x2c) = 0x3f000000u;
			*reinterpret_cast<unsigned*>(candidateManager + 0x2c) = 0x3f000000u;
			*reinterpret_cast<void**>(oracleManager + 0x30) = backendManager;
			*reinterpret_cast<void**>(candidateManager + 0x30) = backendManager;
			oracleManager[0x2b] = candidateManager[0x2b] = 1;
			oracleManager[0x28] = candidateManager[0x28] = 1;
			unsigned char oracleOldCollision[0xa0], oracleReplacementCollision[0xa0];
			unsigned char candidateOldCollision[0xa0], candidateReplacementCollision[0xa0];
			memset(oracleOldCollision, 0xab, sizeof(oracleOldCollision));
			memset(oracleReplacementCollision, 0xcd, sizeof(oracleReplacementCollision));
			memset(candidateOldCollision, 0xab, sizeof(candidateOldCollision));
			memset(candidateReplacementCollision, 0xcd, sizeof(candidateReplacementCollision));
			setManagerCollisions(oracleOldCollision, oracleReplacementCollision);
			resetManagerCallbacks();
			const unsigned timestepWord = 0x3eaaaaabu;
			const float dirtyTimestep = *reinterpret_cast<const float*>(&timestepWord);
			unsigned oracleEspBefore = 0, oracleEspAfter = 0;
			unsigned candidateEspBefore = 0, candidateEspAfter = 0;
			resetRun();
			__asm mov oracleEspBefore, esp
			const int oracleResult = oracleStep(oracleManager, dirtyTimestep);
			__asm mov oracleEspAfter, esp
			__asm fstp st(0)
			FluidManagerCallbackSummary oracleRefresh = {};
			getManagerCallbackSummary(&oracleRefresh);
			const unsigned oracleScale = *reinterpret_cast<unsigned*>(
				oracleReplacementCollision + 0x64);
			const unsigned oracleMargin = *reinterpret_cast<unsigned*>(
				oracleReplacementCollision + 0x68);
			unsigned char untouchedOldCollision[sizeof(oracleOldCollision)];
			memset(untouchedOldCollision, 0xab, sizeof(untouchedOldCollision));
			const bool oracleOldPreserved = memcmp(oracleOldCollision,
				untouchedOldCollision, sizeof(oracleOldCollision)) == 0;
			NxFluidRunCallbackRecord oracleRuns[2] = {};
			const bool oracleRunsOk = fluidRunCount() == 2 &&
				getFluidRun(0, &oracleRuns[0]) && getFluidRun(1, &oracleRuns[1]);
			const unsigned oracleFlag = oracleManager[0x28];
			setManagerCollisions(candidateOldCollision, candidateReplacementCollision);
			resetManagerCallbacks();
			resetRun();
			__asm mov candidateEspBefore, esp
			const int candidateResult = candidateStep(candidateManager, dirtyTimestep);
			__asm mov candidateEspAfter, esp
			__asm fstp st(0)
			FluidManagerCallbackSummary candidateRefresh = {};
			getManagerCallbackSummary(&candidateRefresh);
			const unsigned candidateScale = *reinterpret_cast<unsigned*>(
				candidateReplacementCollision + 0x64);
			const unsigned candidateMargin = *reinterpret_cast<unsigned*>(
				candidateReplacementCollision + 0x68);
			const bool candidateOldPreserved = memcmp(candidateOldCollision,
				untouchedOldCollision, sizeof(candidateOldCollision)) == 0;
			const bool replacementBuilt = oracleScale == 0x40800000u &&
				oracleMargin == 0x408ab2dcu && candidateScale == oracleScale &&
				candidateMargin == oracleMargin;
			NxFluidRunCallbackRecord candidateRuns[2] = {};
			const bool candidateRunsOk = fluidRunCount() == 2 &&
				getFluidRun(0, &candidateRuns[0]) && getFluidRun(1, &candidateRuns[1]);
			const unsigned candidateFlag = candidateManager[0x28];
			const bool callbacksMatch = oracleRefresh.staticCollisionGet == 2 &&
				candidateRefresh.staticCollisionGet == oracleRefresh.staticCollisionGet &&
				oracleRefresh.staticCollisionDestroy == 1 &&
				candidateRefresh.staticCollisionDestroy == oracleRefresh.staticCollisionDestroy &&
				oracleRefresh.staticCollisionCreate == 1 &&
				candidateRefresh.staticCollisionCreate == oracleRefresh.staticCollisionCreate;
			const bool dataMatch = replacementBuilt && oracleOldPreserved &&
				candidateOldPreserved;
			unsigned fluidMismatches = !oracleRunsOk || !candidateRunsOk;
			for(unsigned i = 0; i != 2 && oracleRunsOk && candidateRunsOk; ++i)
				{
				const NxFluidRunCallbackRecord& expected = oracleRuns[i];
				const NxFluidRunCallbackRecord& actual = candidateRuns[i];
				if(expected.system != 0x10203040u + i || actual.system != expected.system ||
					expected.fluid != 0x55667780u + i || actual.fluid != expected.fluid ||
					expected.state != reinterpret_cast<unsigned>(oracleStates[i]) ||
					actual.state != reinterpret_cast<unsigned>(candidateStates[i]) ||
					memcmp(expected.vector, expectedVectors[i], sizeof(expected.vector)) ||
					memcmp(actual.vector, expected.vector, sizeof(actual.vector)) ||
					expected.timestep != timestepWord || actual.timestep != expected.timestep ||
					expected.option != 1 || actual.option != expected.option ||
					*reinterpret_cast<unsigned*>(oracleStates[i] + 4) != 0 ||
					*reinterpret_cast<unsigned*>(candidateStates[i] + 4) != 0 ||
				*reinterpret_cast<unsigned*>(oracleFluids[i] + 0x74) != timestepWord ||
					*reinterpret_cast<unsigned*>(candidateFluids[i] + 0x74) != timestepWord)
					++fluidMismatches;
				}
			mismatches += !callbacksMatch || !dataMatch || oracleFlag != 0 ||
				candidateFlag != oracleFlag || oracleResult != candidateResult ||
				fluidMismatches ||
				oracleEspBefore != oracleEspAfter ||
				candidateEspBefore != candidateEspAfter;
			printf("fluid manager replacement-buffer built=%u old-preserved=%u\n",
				replacementBuilt, oracleOldPreserved && candidateOldPreserved);
		printf("fluid manager dirty-refresh get=%u destroy=%u create=%u data=%u flag=%u fluids=%u/%u callbacks=%u timestep=%u state=%u stack=%u/%u mismatches=%u\n",
				oracleRefresh.staticCollisionGet == 2 && candidateRefresh.staticCollisionGet == 2,
				oracleRefresh.staticCollisionDestroy == 1 && candidateRefresh.staticCollisionDestroy == 1,
				oracleRefresh.staticCollisionCreate == 1 && candidateRefresh.staticCollisionCreate == 1,
				dataMatch, oracleFlag == 0 && candidateFlag == oracleFlag,
				oracleRunsOk ? 2u : fluidRunCount(),
				candidateRunsOk ? 2u : fluidRunCount(),
				fluidMismatches == 0, fluidMismatches == 0,
				fluidMismatches == 0,
				oracleEspBefore == oracleEspAfter, candidateEspBefore == candidateEspAfter,
			mismatches);
		}
	gFailures += mismatches;
	}

	nxUnbindOracleEmitterFlagCallbacks(image, oldCallbackValues, callbackPageProtection);
	FreeLibrary(fluidModel);
	FreeLibrary(candidatePhysics);

	DeleteCriticalSection(&candidateLock.section);
	nxUnbindOracleLocks(image, lockSlots);
	printf("fluid emitter aggregate cases=6 mismatches=%u\n", gFailures);
	return gFailures ? 1 : 0;
	}
