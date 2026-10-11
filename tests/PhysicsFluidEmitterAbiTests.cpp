#define NOMINMAX
#include <windows.h>
#include <bcrypt.h>
#include <fstream>
#include <stdio.h>
#include <sstream>
#include <string.h>
#include <string>

#include "fluids/NxFluidEmitter.h"
#include "fluids/NpFluidEmitter.h"

static unsigned gFailures;

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

static bool nxBindOracleEmitterFlagCallbacks(unsigned char* image, void** oldValues,
	DWORD* oldProtection)
	{
	static const unsigned slotRvas[] = {
		0x126628, 0x126404, 0x12657c, 0x12644c, 0x126614, 0x1263dc,
		0x1263f0, 0x12664c, 0x1263e8 };
	static void* const replacements[] = {
		reinterpret_cast<void*>(&nxOracleEmitterSetBodyRepulsionFlag),
		reinterpret_cast<void*>(&nxOracleEmitterSetAddBodyVelocityFlag),
		reinterpret_cast<void*>(&nxOracleEmitterSetEnabledFlag),
		reinterpret_cast<void*>(&nxOracleEmitterGetGlobalPose),
		reinterpret_cast<void*>(&nxOracleEmitterSetGlobalPose),
		reinterpret_cast<void*>(&nxOracleEmitterSetAttachedFlag),
		reinterpret_cast<void*>(&nxOracleEmitterSetBodyPosition),
		reinterpret_cast<void*>(&nxOracleEmitterSetBodyLinearVelocity),
		reinterpret_cast<void*>(&nxOracleEmitterSetBodyAngularVelocity),
		};
	void* page = reinterpret_cast<void*>(reinterpret_cast<size_t>(image + 0x126000)
		& ~static_cast<size_t>(0xfff));
	if(!VirtualProtect(page, 0x1000, PAGE_READWRITE, oldProtection))
		return false;
	for(unsigned i = 0; i < 9; ++i)
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
		0x126628, 0x126404, 0x12657c, 0x12644c, 0x126614, 0x1263dc,
		0x1263f0, 0x12664c, 0x1263e8 };
	DWORD ignored = 0;
	void* page = reinterpret_cast<void*>(reinterpret_cast<size_t>(image + 0x126000)
		& ~static_cast<size_t>(0xfff));
	VirtualProtect(page, 0x1000, PAGE_READWRITE, &ignored);
	for(unsigned i = 0; i < 9; ++i)
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
	HMODULE candidatePhysics = 0;
	void* candidateSetterAddress = 0;
	if(!nxLoadCandidateFluidEmitterSetter(&candidatePhysics, &candidateSetterAddress))
		{
		fprintf(stderr, "FAIL candidate NxPhysics.dll setFlag row could not be resolved from its map\n");
		return 1;
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
	unsigned dllRawMismatches = 0;
	unsigned dllRawCases = 0;
	unsigned dllRawRetptr = 0;
	unsigned dllRawStackBalanced = 0;
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
		unsigned char candidateDllOut[0x30];
		memset(candidateDllOut, 0, sizeof(candidateDllOut));
		void* candidateDllResult = 0;
		unsigned candidateDllStackDelta = 0xffffffffu;
		if(!nxTryFluidEmitterCall(candidateDllInterface, aggregateSlots[i].slot,
			candidateDllOut, &candidateDllResult, &candidateDllStackDelta))
			++dllRawMismatches;
		else
			{
			++dllRawCases;
			if(candidateDllResult == candidateDllOut)
				++dllRawRetptr;
			if(candidateDllStackDelta == 0)
				++dllRawStackBalanced;
			if(candidateDllResult != candidateDllOut || candidateDllStackDelta != 0
				|| memcmp(oracleOut, candidateDllOut, aggregateSlots[i].bytes) != 0)
				{
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
	typedef void (__cdecl *ResetCallbacksFn)();
	typedef unsigned (__cdecl *CallbackCountFn)();
	typedef int (__cdecl *GetCallbackRecordFn)(unsigned, NxFluidEmitterFlagCallbackRecord*);
	ResetCallbacksFn resetCallbacks = reinterpret_cast<ResetCallbacksFn>(resetFluidCallbacks);
	CallbackCountFn callbackCount = reinterpret_cast<CallbackCountFn>(getFluidCallbackCount);
	GetCallbackRecordFn getCallbackRecord = reinterpret_cast<GetCallbackRecordFn>(getFluidCallbackRecord);
	void* oldCallbackValues[9] = {};
	DWORD callbackPageProtection = 0;
	if(!nxBindOracleEmitterFlagCallbacks(image, oldCallbackValues, &callbackPageProtection))
		{
		fprintf(stderr, "FAIL oracle FluidModel callback slots could not be bound\n");
		return 1;
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

	nxUnbindOracleEmitterFlagCallbacks(image, oldCallbackValues, callbackPageProtection);
	FreeLibrary(fluidModel);
	FreeLibrary(candidatePhysics);

	DeleteCriticalSection(&candidateLock.section);
	nxUnbindOracleLocks(image, lockSlots);
	printf("fluid emitter aggregate cases=6 mismatches=%u\n", gFailures);
	return gFailures ? 1 : 0;
	}
