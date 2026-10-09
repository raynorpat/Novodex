#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <stdio.h>
#include <string.h>

#include "fluids/NxFluidEmitter.h"
#include "fluids/NpFluidEmitter.h"

static unsigned gFailures;
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

// Only the six aggregate-return methods are production methods in this probe.
// Every other pure virtual is a test shim so the actual NpFluidEmitter methods
// can be called through the immutable public interface.
class FluidEmitterProbe : public NpFluidEmitter
	{
	public:
	FluidEmitterProbe(void* internal) : NpFluidEmitter(internal) {}
	virtual NxFluid& getFluid() const { return *reinterpret_cast<NxFluid*>(0x1000); }
	virtual void setGlobalPose(const NxMat34&) {}
	virtual void setGlobalPosition(const NxVec3&) {}
	virtual void setGlobalOrientation(const NxMat33&) {}
	virtual void setLocalPose(const NxMat34&) {}
	virtual void setLocalPosition(const NxVec3&) {}
	virtual void setLocalOrientation(const NxMat33&) {}
	virtual void setFrameActor(NxActor*) {}
	virtual NxActor* getFrameActor() const { return 0; }
	virtual NxReal getDimensionX() const { return 0; }
	virtual NxReal getDimensionY() const { return 0; }
	virtual void setRandomPos(NxVec3) {}
	virtual NxVec3 getRandomPos() const { return NxVec3(0, 0, 0); }
	virtual void setRandomAngle(NxReal) {}
	virtual NxReal getRandomAngle() const { return 0; }
	virtual void setFluidVelocityMagnitude(NxReal) {}
	virtual NxReal getFluidVelocityMagnitude() const { return 0; }
	virtual void setRate(NxReal) {}
	virtual NxReal getRate() const { return 0; }
	virtual void setParticleLifetime(NxReal) {}
	virtual NxReal getParticleLifetime() const { return 0; }
	virtual void setFlag(NxFluidEmitterFlag, bool) {}
	virtual NX_BOOL getFlag(NxFluidEmitterFlag) const { return 0; }
	virtual NX_BOOL getShape(NxEmitterShape) const { return 0; }
	virtual NX_BOOL getType(NxEmitterType) const { return 0; }
	virtual void setName(const char*) {}
	virtual const char* getName() const { return 0; }
	};

int wmain(int argc, wchar_t** argv)
	{
	setvbuf(stdout, 0, _IONBF, 0);
	if(argc != 3)
		{
		fprintf(stderr, "usage: NxPhysicsFluidEmitterAbiTests <oracle directory> <NxPhysics.dll sha256>\n");
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
	unsigned char* image = reinterpret_cast<unsigned char*>(module);
	NxLockSlots lockSlots = nxBindOracleLocks(image);
	if(!lockSlots.protection)
		return 1;

	unsigned char internal[0x90];
	memset(internal, 0, sizeof(internal));
	for(unsigned offset = 0; offset < sizeof(internal); offset += 4)
		*reinterpret_cast<unsigned*>(internal + offset) = 0x68000000u + offset;
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
	typedef void* (__thiscall* FluidEmitterCtor)(void*, void*);
	FluidEmitterCtor oracleCtor = reinterpret_cast<FluidEmitterCtor>(image + 0x8c2d0);
	if(oracleCtor(oracleBytes, internal) != oracleBytes)
		++gFailures;
	unsigned char oracleLockData[0x20];
	unsigned char oracleLockObject[4];
	memset(oracleLockData, 0, sizeof(oracleLockData));
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
	memcpy(reinterpret_cast<unsigned char*>(&candidate) + 0x10,
		&candidateSectionLink, sizeof(candidateSectionLink));
	NxFluidEmitter* candidateInterface = &candidate;
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
	for(unsigned i = 0; i < sizeof(aggregateSlots) / sizeof(aggregateSlots[0]); ++i)
		{
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
		if(oracleResult != oracleOut || candidateResult != candidateOut
			|| oracleStackDelta != 0 || candidateStackDelta != 0
			|| memcmp(oracleOut, candidateOut, aggregateSlots[i].bytes) != 0)
			{
			fprintf(stderr, "fluid emitter raw ABI mismatch: %s oracle_ret=%u candidate_ret=%u stack=%u/%u\n",
				aggregateSlots[i].name, oracleResult == oracleOut ? 1u : 0u,
				candidateResult == candidateOut ? 1u : 0u, oracleStackDelta, candidateStackDelta);
			++rawMismatches;
			}
		}
	gFailures += rawMismatches;
	printf("fluid emitter raw_abi cases=%u retptr=%u stack_balanced=%u mismatches=%u\n",
		rawCases, rawRetptr, rawStackBalanced, rawMismatches);

	NxMat34 oracleGlobalPose;
	oracleGlobalPose = oracle->getGlobalPoseVal();
	NxMat34 candidateGlobalPose = candidateInterface->getGlobalPoseVal();
	nxCheckEqual("global_pose", &oracleGlobalPose, &candidateGlobalPose, sizeof(oracleGlobalPose));
	NxVec3 oracleGlobalPosition;
	oracleGlobalPosition = oracle->getGlobalPositionVal();
	NxVec3 candidateGlobalPosition = candidateInterface->getGlobalPositionVal();
	nxCheckEqual("global_position", &oracleGlobalPosition, &candidateGlobalPosition, sizeof(oracleGlobalPosition));
	NxMat33 oracleGlobalOrientation;
	oracleGlobalOrientation = oracle->getGlobalOrientationVal();
	NxMat33 candidateGlobalOrientation = candidateInterface->getGlobalOrientationVal();
	nxCheckEqual("global_orientation", &oracleGlobalOrientation, &candidateGlobalOrientation, sizeof(oracleGlobalOrientation));
	NxMat34 oracleLocalPose;
	oracleLocalPose = oracle->getLocalPoseVal();
	NxMat34 candidateLocalPose = candidateInterface->getLocalPoseVal();
	nxCheckEqual("local_pose", &oracleLocalPose, &candidateLocalPose, sizeof(oracleLocalPose));
	NxVec3 oracleLocalPosition;
	oracleLocalPosition = oracle->getLocalPositionVal();
	NxVec3 candidateLocalPosition = candidateInterface->getLocalPositionVal();
	nxCheckEqual("local_position", &oracleLocalPosition, &candidateLocalPosition, sizeof(oracleLocalPosition));
	NxMat33 oracleLocalOrientation;
	oracleLocalOrientation = oracle->getLocalOrientationVal();
	NxMat33 candidateLocalOrientation = candidateInterface->getLocalOrientationVal();
	nxCheckEqual("local_orientation", &oracleLocalOrientation, &candidateLocalOrientation, sizeof(oracleLocalOrientation));

	DeleteCriticalSection(&candidateLock.section);
	nxUnbindOracleLocks(image, lockSlots);
	printf("fluid emitter aggregate cases=6 mismatches=%u\n", gFailures);
	return gFailures ? 1 : 0;
	}
