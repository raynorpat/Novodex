#define NOMINMAX
#include "PhysicsPairLoader.h"
#include "NxPageGuardedAllocator.h"
#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"

#include <stdio.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include <string>

struct RepulsionCallbackRecord
	{
	unsigned kind;
	unsigned system;
	unsigned fluid;
	unsigned emitter;
	unsigned vectors[6];
	unsigned mass;
	};

static RepulsionCallbackRecord gOracleRecords[16];
static unsigned gOracleRecordCount;

static RepulsionCallbackRecord& nxNextOracleRecord(unsigned kind,
	unsigned system, unsigned fluid, unsigned emitter)
	{
	static RepulsionCallbackRecord overflow = {};
	RepulsionCallbackRecord& record = gOracleRecordCount
		< sizeof(gOracleRecords) / sizeof(gOracleRecords[0])
		? gOracleRecords[gOracleRecordCount++] : overflow;
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
	nxNextOracleRecord(1, system, fluid, emitter);
	return 1;
	}

extern "C" int __cdecl nxOracleEmitterGetBodyRepulsion(
	unsigned system, unsigned fluid, unsigned emitter, float* force, float* torque)
	{
	RepulsionCallbackRecord& record = nxNextOracleRecord(
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
	RepulsionCallbackRecord& record = nxNextOracleRecord(3, system, fluid, 0);
	const unsigned mass = 0x3f2aaaab;
	record.mass = mass;
	return *reinterpret_cast<const float*>(&mass);
	}

static void nxBindOracleCallbacks(unsigned char* image, void** oldValues,
	DWORD* oldProtection)
	{
	static const unsigned rvas[3] = { 0x126408, 0x1263e0, 0x126518 };
	void* const replacements[3] = {
		reinterpret_cast<void*>(&nxOracleEmitterGetEnabledFlag),
		reinterpret_cast<void*>(&nxOracleEmitterGetBodyRepulsion),
		reinterpret_cast<void*>(&nxOracleFluidGetParticleMass) };
	void* page = image + 0x126000;
	if(!VirtualProtect(page, 0x1000, PAGE_READWRITE, oldProtection))
		return;
	for(unsigned i = 0; i != 3; ++i)
		{
		void** slot = reinterpret_cast<void**>(image + rvas[i]);
		oldValues[i] = *slot;
		*slot = replacements[i];
		}
	DWORD ignored = 0;
	VirtualProtect(page, 0x1000, *oldProtection, &ignored);
	}

static void nxUnbindOracleCallbacks(unsigned char* image, void** oldValues,
	DWORD oldProtection)
	{
	static const unsigned rvas[3] = { 0x126408, 0x1263e0, 0x126518 };
	DWORD ignored = 0;
	void* page = image + 0x126000;
	VirtualProtect(page, 0x1000, PAGE_READWRITE, &ignored);
	for(unsigned i = 0; i != 3; ++i)
		*reinterpret_cast<void**>(image + rvas[i]) = oldValues[i];
	VirtualProtect(page, 0x1000, oldProtection, &ignored);
	}

static bool nxResolveCandidateRepulsion(HMODULE module,
	const wchar_t* pairDirectory, void** address)
	{
	wchar_t path[MAX_PATH];
	if(swprintf_s(path, L"%s\\NxPhysics.map", pairDirectory) < 0)
		return false;
	std::ifstream map(path);
	if(!map)
		return false;
	static const char symbol[] = "?nxNpFluidEmitterApplyBodyRepulsion@@YAHPAX@Z";
	unsigned preferredBase = 0;
	std::string line;
	while(std::getline(map, line))
		{
		static const char marker[] = "Preferred load address is ";
		const std::string::size_type basePosition = line.find(marker);
		if(basePosition != std::string::npos)
			{
			std::istringstream base(line.substr(basePosition + sizeof(marker) - 1));
			base >> std::hex >> preferredBase;
			}
		const std::string::size_type symbolPosition = line.find(symbol);
		if(symbolPosition == std::string::npos)
			continue;
		std::istringstream addressField(line.substr(symbolPosition + sizeof(symbol) - 1));
		std::string addressText;
		addressField >> addressText;
		unsigned preferredAddress = 0;
		std::istringstream addressValue(addressText);
		addressValue >> std::hex >> preferredAddress;
		IMAGE_DOS_HEADER* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(module);
		if(!preferredBase || preferredAddress < preferredBase
			|| dos->e_magic != IMAGE_DOS_SIGNATURE)
			return false;
		IMAGE_NT_HEADERS32* nt = reinterpret_cast<IMAGE_NT_HEADERS32*>(
			reinterpret_cast<unsigned char*>(module) + dos->e_lfanew);
		if(nt->Signature != IMAGE_NT_SIGNATURE
			|| preferredAddress - preferredBase >= nt->OptionalHeader.SizeOfImage)
			return false;
		*address = reinterpret_cast<unsigned char*>(module)
			+ (preferredAddress - preferredBase);
		return true;
		}
	return false;
	}

struct LiveBodySnapshot
	{
	RepulsionCallbackRecord callbacks[3];
	unsigned callbackCount;
	unsigned words[15];
	unsigned stackBalanced;
	};

typedef void (__cdecl *ResetCallbacksFn)();
typedef unsigned (__cdecl *CallbackCountFn)();
typedef int (__cdecl *GetCallbackRecordFn)(unsigned, RepulsionCallbackRecord*);

static bool nxRunLiveBody(HMODULE physics, bool oracle, void* target,
	ResetCallbacksFn reset, CallbackCountFn count, GetCallbackRecordFn get,
	LiveBodySnapshot* snapshot)
	{
	typedef NxPhysicsSDK* (__cdecl *CreateSDK)(NxU32, NxUserAllocator*, NxUserOutputStream*);
	CreateSDK create = reinterpret_cast<CreateSDK>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!create || !snapshot)
		return false;
	NxPageGuardedAllocator allocator;
	NxPhysicsSDK* sdk = create(NX_PHYSICS_SDK_VERSION, &allocator, 0);
	if(!sdk)
		return false;
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene)
		{
		sdk->release();
		return false;
		}
	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 2.0f, 3.0f);
	NxBodyDesc bodyDesc;
	bodyDesc.mass = 5.0f;
	bodyDesc.massSpaceInertia = NxVec3(2.0f, 3.0f, 4.0f);
	NxActorDesc actorDesc;
	actorDesc.shapes.pushBack(&box);
	actorDesc.body = &bodyDesc;
	NxActor* actor = scene->createActor(actorDesc);
	if(!actor)
		{
		sdk->releaseScene(*scene);
		sdk->release();
		return false;
		}
	unsigned char* actorInternal = *reinterpret_cast<unsigned char**>(
		reinterpret_cast<unsigned char*>(actor) + 0x14);
	unsigned char* record = actorInternal
		? *reinterpret_cast<unsigned char**>(actorInternal + 8) : 0;
	if(!actorInternal || !record)
		{
		scene->releaseActor(*actor);
		sdk->releaseScene(*scene);
		sdk->release();
		return false;
		}
	unsigned char fluid[0x84] = {}, backend[0x34] = {}, emitter[0x18] = {};
	*reinterpret_cast<void**>(fluid + 0x7c) = backend;
	*reinterpret_cast<unsigned*>(fluid + 0x80) = 0x55667788u;
	*reinterpret_cast<unsigned*>(backend + 0x30) = 0x10203040u;
	*reinterpret_cast<void**>(emitter + 4) = fluid;
	*reinterpret_cast<unsigned*>(emitter + 8) = 0x11223344u;
	*reinterpret_cast<unsigned*>(emitter + 0x10) = 4;
	*reinterpret_cast<void**>(emitter + 0x14) = actorInternal;
	reset();
	gOracleRecordCount = 0;
	unsigned espBefore = 0, espAfter = 0;
	if(oracle)
		{
		typedef int (__thiscall *Apply)(void*);
		float incoming = 0.375f;
		__asm fld incoming
		__asm mov espBefore, esp
		const int ignored = reinterpret_cast<Apply>(target)(emitter);
		(void)ignored;
		__asm mov espAfter, esp
		__asm fstp st(0)
		}
	else
		{
		typedef int (__cdecl *Apply)(void*);
		__asm mov espBefore, esp
		const int ignored = reinterpret_cast<Apply>(target)(emitter);
		(void)ignored;
		__asm mov espAfter, esp
		}
	memset(snapshot, 0, sizeof(*snapshot));
	snapshot->callbackCount = oracle ? gOracleRecordCount : count();
	for(unsigned i = 0; i != 3 && i < snapshot->callbackCount; ++i)
		{
		if(oracle)
			snapshot->callbacks[i] = gOracleRecords[i];
		else
			get(i, &snapshot->callbacks[i]);
		}
	static const unsigned offsets[15] = {
		0x34, 0x38, 0x3c, 0x40, 0x44, 0x48,
		0x6c, 0x70, 0x74, 0x78, 0x7c, 0x80,
		0x84, 0x4c, 0x114 };
	for(unsigned i = 0; i != 15; ++i)
		snapshot->words[i] = *reinterpret_cast<unsigned*>(record + offsets[i]);
	snapshot->stackBalanced = espBefore == espAfter;
	scene->releaseActor(*actor);
	sdk->releaseScene(*scene);
	sdk->release();
	return true;
	}

int wmain(int argc, wchar_t** argv)
	{
	setvbuf(stdout, 0, _IONBF, 0);
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	const int openStatus = nxOpenPair(argc, argv,
		"NxPhysicsFluidEmitterBodyRepulsionTests", pairDirectory, &physics);
	if(openStatus)
		return openStatus;
	const wchar_t* slash = wcsrchr(pairDirectory, L'\\');
	const bool oracle = slash && _wcsicmp(slash + 1, L"oracle") == 0;
	void* target = oracle ? reinterpret_cast<void*>(
		reinterpret_cast<unsigned char*>(physics) + 0x881c0) : 0;
	if(!oracle && !nxResolveCandidateRepulsion(physics, pairDirectory, &target))
		return nxFail("candidate body-repulsion symbol missing from the staged map");
	wchar_t fluidModelPath[MAX_PATH];
	if(swprintf_s(fluidModelPath, L"%s\\FluidModel.dll", pairDirectory) < 0)
		return nxFail("FluidModel path is too long");
	HMODULE fluidModel = LoadLibraryExW(fluidModelPath, 0,
		LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
	if(!fluidModel)
		return nxFail("staged FluidModel callback fixture failed to load");
	ResetCallbacksFn reset = reinterpret_cast<ResetCallbacksFn>(
		GetProcAddress(fluidModel, "NxResetFluidRepulsionCallbacks"));
	CallbackCountFn count = reinterpret_cast<CallbackCountFn>(
		GetProcAddress(fluidModel, "NxGetFluidRepulsionCallbackCount"));
	GetCallbackRecordFn get = reinterpret_cast<GetCallbackRecordFn>(
		GetProcAddress(fluidModel, "NxGetFluidRepulsionCallbackRecord"));
	if(!reset || !count || !get)
		return nxFail("FluidModel callback probes are missing");
	void* oldValues[3] = {};
	DWORD oldProtection = 0;
	if(oracle)
		nxBindOracleCallbacks(reinterpret_cast<unsigned char*>(physics),
			oldValues, &oldProtection);
	LiveBodySnapshot snapshot = {};
	const bool ran = nxRunLiveBody(physics, oracle, target, reset, count, get, &snapshot);
	unsigned callbackMismatches = snapshot.callbackCount != 3;
	static const unsigned expectedVectors[6] = {
		0x3e800001u, 0xbf000002u, 0x3f400003u,
		0x40000004u, 0xc0400005u, 0x40800006u };
	for(unsigned i = 0; i != 3 && i < snapshot.callbackCount; ++i)
		{
		const RepulsionCallbackRecord& r = snapshot.callbacks[i];
		if(r.kind != i + 1 || r.system != 0x10203040u
			|| r.fluid != 0x55667788u
			|| r.emitter != (i == 2 ? 0u : 0x11223344u)
			|| (i == 1 && memcmp(r.vectors, expectedVectors, sizeof(expectedVectors)) != 0)
			|| (i == 2 && r.mass != 0x3f2aaaabu))
			++callbackMismatches;
		}
	bool applied = false;
	for(unsigned i = 6; i != 12; ++i)
		applied = applied || snapshot.words[i] != 0;
	unsigned mismatches = !ran || callbackMismatches || !applied
		|| !snapshot.stackBalanced;
	if(oracle)
		nxUnbindOracleCallbacks(reinterpret_cast<unsigned char*>(physics),
			oldValues, oldProtection);
	printf("fluid emitter body repulsion live actor=1 callbacks=%u callback_mismatches=%u "
		"applied=%u stack=%u mismatches=%u\n",
		snapshot.callbackCount, callbackMismatches, applied, snapshot.stackBalanced, mismatches);
	printf("fluid emitter body repulsion live state=");
	for(unsigned i = 0; i != 15; ++i)
		printf("%s%08x", i ? "." : "", snapshot.words[i]);
	printf("\n");
	if(nxAuditModules(pairDirectory))
		++mismatches;
	FreeLibrary(fluidModel);
	return mismatches ? 1 : 0;
	}
