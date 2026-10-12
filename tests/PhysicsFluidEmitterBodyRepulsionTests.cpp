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
static int gOracleEnabledResult = 1;

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
	return gOracleEnabledResult;
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
	unsigned result;
	unsigned noFlagReturnsActor;
	unsigned nullActorGuard;
	unsigned disabledCallbackGuard;
	unsigned negativeCallbackContinues;
	unsigned words[15];
	unsigned stackBalanced;
	};

struct FluidRunRecord
	{
	unsigned system;
	unsigned fluid;
	unsigned state;
	unsigned vector[3];
	unsigned timestep;
	unsigned option;
	};

struct EmitterStateRecord
	{
	unsigned kind;
	unsigned argument0;
	unsigned argument1;
	unsigned argument2;
	unsigned enabled;
	unsigned pose[12];
	};

struct LiveSubstepSnapshot
	{
	LiveBodySnapshot body;
	FluidRunRecord run;
	EmitterStateRecord emitterCallbacks[4];
	unsigned emitterCallbackCount;
	unsigned runCallbackCount;
	unsigned stateCleared;
	unsigned bodyStateChanged;
	};

typedef void (__cdecl *ResetCallbacksFn)();
typedef unsigned (__cdecl *CallbackCountFn)();
typedef int (__cdecl *GetCallbackRecordFn)(unsigned, RepulsionCallbackRecord*);
typedef void (__cdecl *SetEnabledResultFn)(int);

static bool nxRunLiveBody(HMODULE physics, bool oracle, void* target,
	ResetCallbacksFn reset, CallbackCountFn count, GetCallbackRecordFn get,
	SetEnabledResultFn setEnabledResult,
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
		snapshot->result = static_cast<unsigned>(
			reinterpret_cast<Apply>(target)(emitter));
		__asm mov espAfter, esp
		__asm fstp st(0)
		}
	else
		{
		typedef int (__cdecl *Apply)(void*);
		__asm mov espBefore, esp
		snapshot->result = static_cast<unsigned>(
			reinterpret_cast<Apply>(target)(emitter));
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
	*reinterpret_cast<unsigned*>(emitter + 0x10) = 0;
	if(oracle)
		{
		typedef int (__thiscall *Apply)(void*);
		float incoming = 0.375f;
		__asm fld incoming
		const unsigned result = static_cast<unsigned>(
			reinterpret_cast<Apply>(target)(emitter));
		snapshot->noFlagReturnsActor = result == reinterpret_cast<unsigned>(actorInternal);
		__asm fstp st(0)
		}
	else
		{
		typedef int (__cdecl *Apply)(void*);
		const unsigned result = static_cast<unsigned>(
			reinterpret_cast<Apply>(target)(emitter));
		snapshot->noFlagReturnsActor = result == reinterpret_cast<unsigned>(actorInternal);
		}
	*reinterpret_cast<unsigned*>(emitter + 0x10) = 4;
	*reinterpret_cast<void**>(emitter + 0x14) = 0;
	if(oracle)
		{
		typedef int (__thiscall *Apply)(void*);
		float incoming = 0.375f;
		__asm fld incoming
		const unsigned result = static_cast<unsigned>(
			reinterpret_cast<Apply>(target)(emitter));
		__asm fstp st(0)
		snapshot->nullActorGuard = result == 0;
		}
	else
		{
		typedef int (__cdecl *Apply)(void*);
		const unsigned result = static_cast<unsigned>(
			reinterpret_cast<Apply>(target)(emitter));
		snapshot->nullActorGuard = result == 0;
		}
	*reinterpret_cast<void**>(emitter + 0x14) = actorInternal;
	reset();
	gOracleRecordCount = 0;
	if(oracle)
		gOracleEnabledResult = 0;
	else
		setEnabledResult(0);
	if(oracle)
		{
		typedef int (__thiscall *Apply)(void*);
		float incoming = 0.375f;
		__asm fld incoming
		const unsigned result = static_cast<unsigned>(
			reinterpret_cast<Apply>(target)(emitter));
		__asm fstp st(0)
		snapshot->disabledCallbackGuard = result == 0 && gOracleRecordCount == 1;
		}
	else
		{
		typedef int (__cdecl *Apply)(void*);
		const unsigned result = static_cast<unsigned>(
			reinterpret_cast<Apply>(target)(emitter));
		snapshot->disabledCallbackGuard = result == 0 && count() == 1;
		}
	reset();
	gOracleRecordCount = 0;
	if(oracle)
		gOracleEnabledResult = -1;
	else
		setEnabledResult(-1);
	if(oracle)
		{
		typedef int (__thiscall *Apply)(void*);
		float incoming = 0.375f;
		__asm fld incoming
		(void)reinterpret_cast<Apply>(target)(emitter);
		__asm fstp st(0)
		snapshot->negativeCallbackContinues = gOracleRecordCount == 3;
		}
	else
		{
		typedef int (__cdecl *Apply)(void*);
		(void)reinterpret_cast<Apply>(target)(emitter);
		snapshot->negativeCallbackContinues = count() == 3;
		}
	gOracleEnabledResult = 1;
	setEnabledResult(1);
	scene->releaseActor(*actor);
	sdk->releaseScene(*scene);
	sdk->release();
	return true;
	}

static bool nxResolveCandidateSymbol(HMODULE module, const wchar_t* pairDirectory,
	const char* symbol, void** address)
	{
	wchar_t path[MAX_PATH];
	if(swprintf_s(path, L"%s\\NxPhysics.map", pairDirectory) < 0)
		return false;
	std::ifstream map(path);
	if(!map)
		return false;
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
		std::istringstream addressField(line.substr(symbolPosition + strlen(symbol)));
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

static bool nxRunLiveSubstep(HMODULE physics, bool oracle, void* target,
	ResetCallbacksFn resetRepulsion, CallbackCountFn repulsionCount,
	GetCallbackRecordFn getRepulsion, void* fluidRunReset,
	void* fluidRunCount, void* fluidRunGet, void* emitterReset,
	void* emitterCount, void* emitterGet, LiveSubstepSnapshot* snapshot)
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
	unsigned char* bodyRecord = actorInternal
		? *reinterpret_cast<unsigned char**>(actorInternal + 8) : 0;
	if(!actorInternal || !bodyRecord)
		{
		scene->releaseActor(*actor);
		sdk->releaseScene(*scene);
		sdk->release();
		return false;
		}
	unsigned char fluid[0x84] = {}, backend[0x534] = {}, core[0x530] = {};
	unsigned char state[8] = {}, emitter[0x48] = {};
	void* emitterList[1] = { emitter };
	static const unsigned pose[12] = {
		0x3f800000u, 0, 0, 0, 0x3f800000u, 0,
		0, 0, 0x3f800000u, 0x3e800000u, 0xbf000000u, 0x3f400000u };
	static const unsigned systemVector[3] = {
		0x3e800001u, 0xbf000002u, 0x3f400003u };
	memcpy(core + 0x520, systemVector, sizeof(systemVector));
	*reinterpret_cast<void**>(backend + 0x24) = core;
	*reinterpret_cast<unsigned*>(backend + 0x30) = 0x10203040u;
	*reinterpret_cast<void***>(fluid + 4) = emitterList;
	*reinterpret_cast<void***>(fluid + 8) = emitterList + 1;
	*reinterpret_cast<void**>(fluid + 0x78) = state;
	*reinterpret_cast<void**>(fluid + 0x7c) = backend;
	*reinterpret_cast<unsigned*>(fluid + 0x80) = 0x55667788u;
	*reinterpret_cast<unsigned*>(state + 4) = 0xaaaaaaaa;
	*reinterpret_cast<void**>(emitter + 4) = fluid;
	*reinterpret_cast<unsigned*>(emitter + 8) = 0x11223344u;
	*reinterpret_cast<unsigned*>(emitter + 0x10) = 4;
	*reinterpret_cast<void**>(emitter + 0x14) = actorInternal;
	memcpy(emitter + 0x18, pose, sizeof(pose));
	typedef void (__cdecl *ResetFn)();
	typedef unsigned (__cdecl *CountFn)();
	typedef int (__cdecl *GetRunFn)(unsigned, FluidRunRecord*);
	typedef int (__cdecl *GetEmitterFn)(unsigned, EmitterStateRecord*);
	reinterpret_cast<ResetFn>(resetRepulsion)();
	reinterpret_cast<ResetFn>(fluidRunReset)();
	reinterpret_cast<ResetFn>(emitterReset)();
	gOracleRecordCount = 0;
	const unsigned timestepWord = 0x3eaaaaabu;
	const float timestep = *reinterpret_cast<const float*>(&timestepWord);
	static const unsigned offsets[15] = {
		0x34, 0x38, 0x3c, 0x40, 0x44, 0x48,
		0x6c, 0x70, 0x74, 0x78, 0x7c, 0x80,
		0x84, 0x4c, 0x114 };
	unsigned beforeWords[15];
	for(unsigned i = 0; i != 15; ++i)
		beforeWords[i] = *reinterpret_cast<unsigned*>(bodyRecord + offsets[i]);
	unsigned espBefore = 0, espAfter = 0;
	if(oracle)
		{
		typedef int (__thiscall *FluidStep)(void*, float);
		__asm mov espBefore, esp
		reinterpret_cast<FluidStep>(target)(fluid, timestep);
		__asm mov espAfter, esp
		}
	else
		{
		typedef int (__thiscall *FluidStep)(void*, float);
		__asm mov espBefore, esp
		reinterpret_cast<FluidStep>(target)(fluid, timestep);
		__asm mov espAfter, esp
		}
	memset(snapshot, 0, sizeof(*snapshot));
	snapshot->runCallbackCount = reinterpret_cast<CountFn>(fluidRunCount)();
	snapshot->emitterCallbackCount = reinterpret_cast<CountFn>(emitterCount)();
	if(snapshot->runCallbackCount == 1)
		reinterpret_cast<GetRunFn>(fluidRunGet)(0, &snapshot->run);
	for(unsigned i = 0; i < 4 && i < snapshot->emitterCallbackCount; ++i)
		reinterpret_cast<GetEmitterFn>(emitterGet)(i, &snapshot->emitterCallbacks[i]);
	snapshot->body.callbackCount = oracle ? gOracleRecordCount : repulsionCount();
	for(unsigned i = 0; i < 3 && i < snapshot->body.callbackCount; ++i)
		{
		if(oracle)
			snapshot->body.callbacks[i] = gOracleRecords[i];
		else
			getRepulsion(i, &snapshot->body.callbacks[i]);
		}
	for(unsigned i = 0; i != 15; ++i)
		snapshot->body.words[i] = *reinterpret_cast<unsigned*>(bodyRecord + offsets[i]);
	for(unsigned i = 0; i != 15; ++i)
		if(beforeWords[i] != snapshot->body.words[i])
			snapshot->bodyStateChanged = 1;
	snapshot->body.stackBalanced = espBefore == espAfter;
	snapshot->stateCleared = *reinterpret_cast<unsigned*>(fluid + 0x74) == timestepWord
		&& *reinterpret_cast<unsigned*>(state + 4) == 0;
	scene->releaseActor(*actor);
	sdk->releaseScene(*scene);
	sdk->release();
	return true;
	}

static bool nxBindOracleSubstepCallbacks(unsigned char* image, HMODULE fluidModel,
	void** oldValues, DWORD* oldProtection)
	{
	static const unsigned rvas[5] = {
		0x1263c0, 0x126614, 0x1263f0, 0x12664c, 0x1263e8 };
	static const char* const names[5] = {
		"FluidRun", "EmitterSetGlobalPose", "EmitterSetBodyPosition",
		"EmitterSetBodyLinearVelocity", "EmitterSetBodyAngularVelocity" };
	void* replacements[5] = {};
	for(unsigned i = 0; i != 5; ++i)
		{
		replacements[i] = reinterpret_cast<void*>(GetProcAddress(fluidModel, names[i]));
		if(!replacements[i])
			return false;
		}
	void* page = reinterpret_cast<void*>(reinterpret_cast<size_t>(image + 0x126000)
		& ~static_cast<size_t>(0xfff));
	if(!VirtualProtect(page, 0x1000, PAGE_READWRITE, oldProtection))
		return false;
	for(unsigned i = 0; i != 5; ++i)
		{
		void** slot = reinterpret_cast<void**>(image + rvas[i]);
		oldValues[i] = *slot;
		*slot = replacements[i];
		}
	return true;
	}

static void nxUnbindOracleSubstepCallbacks(unsigned char* image, void** oldValues,
	DWORD oldProtection)
	{
	static const unsigned rvas[5] = {
		0x1263c0, 0x126614, 0x1263f0, 0x12664c, 0x1263e8 };
	DWORD ignored = 0;
	void* page = reinterpret_cast<void*>(reinterpret_cast<size_t>(image + 0x126000)
		& ~static_cast<size_t>(0xfff));
	if(!VirtualProtect(page, 0x1000, PAGE_READWRITE, &ignored))
		return;
	for(unsigned i = 0; i != 5; ++i)
		*reinterpret_cast<void**>(image + rvas[i]) = oldValues[i];
	VirtualProtect(page, 0x1000, oldProtection, &ignored);
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
	SetEnabledResultFn setEnabledResult = reinterpret_cast<SetEnabledResultFn>(
		GetProcAddress(fluidModel, "NxSetFluidEmitterEnabledResult"));
	if(!reset || !count || !get || !setEnabledResult)
		return nxFail("FluidModel callback probes are missing");
	void* oldValues[3] = {};
	DWORD oldProtection = 0;
	if(oracle)
		nxBindOracleCallbacks(reinterpret_cast<unsigned char*>(physics),
			oldValues, &oldProtection);
	LiveBodySnapshot snapshot = {};
	const bool ran = nxRunLiveBody(physics, oracle, target, reset, count, get,
		setEnabledResult, &snapshot);
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
	mismatches = mismatches || !snapshot.noFlagReturnsActor
		|| !snapshot.nullActorGuard || !snapshot.disabledCallbackGuard
		|| !snapshot.negativeCallbackContinues;
	printf("fluid emitter body repulsion live actor=1 callbacks=%u callback_mismatches=%u "
		"applied=%u result=%08x no_flag_actor_return=%u null_actor_guard=%u "
		"disabled_callback_guard=%u negative_callback_continues=%u stack=%u mismatches=%u\n",
		snapshot.callbackCount, callbackMismatches, applied, snapshot.result,
		snapshot.noFlagReturnsActor, snapshot.nullActorGuard,
		snapshot.disabledCallbackGuard, snapshot.negativeCallbackContinues,
		snapshot.stackBalanced, mismatches);
	printf("fluid emitter body repulsion live state=");
	for(unsigned i = 0; i != 15; ++i)
		printf("%s%08x", i ? "." : "", snapshot.words[i]);
	printf("\n");
	void* stepTarget = oracle ? reinterpret_cast<unsigned char*>(physics) + 0x85a40 : 0;
	if(!oracle && !nxResolveCandidateSymbol(physics, pairDirectory,
		"@nxNpFluidStep@12", &stepTarget))
		return nxFail("candidate fluid substep symbol missing from the staged map");
	void* resetRun = reinterpret_cast<void*>(GetProcAddress(fluidModel,
		"NxResetFluidRunCallbacks"));
	void* runCount = reinterpret_cast<void*>(GetProcAddress(fluidModel,
		"NxGetFluidRunCallbackCount"));
	void* getRun = reinterpret_cast<void*>(GetProcAddress(fluidModel,
		"NxGetFluidRunCallbackRecord"));
	void* resetEmitter = reinterpret_cast<void*>(GetProcAddress(fluidModel,
		"NxResetFluidEmitterFlagCallbacks"));
	void* emitterCount = reinterpret_cast<void*>(GetProcAddress(fluidModel,
		"NxGetFluidEmitterFlagCallbackCount"));
	void* getEmitter = reinterpret_cast<void*>(GetProcAddress(fluidModel,
		"NxGetFluidEmitterFlagCallbackRecord"));
	if(!resetRun || !runCount || !getRun || !resetEmitter || !emitterCount || !getEmitter)
		return nxFail("FluidModel substep callback probes are missing");
	void* oldRepulsionValues[3] = {};
	void* oldSubstepValues[5] = {};
	DWORD repulsionProtection = 0, substepProtection = 0;
	if(oracle)
		{
		nxBindOracleCallbacks(reinterpret_cast<unsigned char*>(physics),
			oldRepulsionValues, &repulsionProtection);
		if(!nxBindOracleSubstepCallbacks(reinterpret_cast<unsigned char*>(physics),
			fluidModel, oldSubstepValues, &substepProtection))
			return nxFail("oracle fluid substep callback binding failed");
		}
	LiveSubstepSnapshot substep = {};
	const bool substepRan = nxRunLiveSubstep(physics, oracle, stepTarget,
		reset, count, get, resetRun, runCount, getRun, resetEmitter,
		emitterCount, getEmitter, &substep);
	unsigned substepMismatches = !substepRan
		|| substep.emitterCallbackCount != 4 || substep.runCallbackCount != 1
		|| substep.body.callbackCount != 3 || !substep.body.stackBalanced
		|| !substep.stateCleared || !substep.bodyStateChanged;
	static const unsigned expectedKinds[4] = { 64, 256, 512, 1024 };
	for(unsigned i = 0; i < 4 && i < substep.emitterCallbackCount; ++i)
		{
		const EmitterStateRecord& callback = substep.emitterCallbacks[i];
		if(callback.kind != expectedKinds[i] || callback.argument0 != 0x10203040u
			|| callback.argument1 != 0x55667788u || callback.argument2 != 0x11223344u)
			++substepMismatches;
		}
	static const unsigned expectedStepVector[3] = {
		0x3e800001u, 0xbf000002u, 0x3f400003u };
	if(substep.run.system != 0x10203040u || substep.run.fluid != 0x55667788u
		|| !substep.run.state || memcmp(substep.run.vector, expectedStepVector,
			sizeof(expectedStepVector)) != 0
		|| substep.run.timestep != 0x3eaaaaabu || substep.run.option != 1)
		++substepMismatches;
	for(unsigned i = 0; i < 3 && i < substep.body.callbackCount; ++i)
		if(substep.body.callbacks[i].kind != i + 1
			|| substep.body.callbacks[i].system != 0x10203040u
			|| substep.body.callbacks[i].fluid != 0x55667788u
			|| substep.body.callbacks[i].emitter != (i == 2 ? 0u : 0x11223344u))
			++substepMismatches;
	static const unsigned expectedRepulsionVectors[6] = {
		0x3e800001u, 0xbf000002u, 0x3f400003u,
		0x40000004u, 0xc0400005u, 0x40800006u };
	if(substep.body.callbackCount > 1
		&& memcmp(substep.body.callbacks[1].vectors, expectedRepulsionVectors,
			sizeof(expectedRepulsionVectors)) != 0)
		++substepMismatches;
	if(substep.body.callbackCount > 2
		&& substep.body.callbacks[2].mass != 0x3f2aaaabu)
		++substepMismatches;
	if(oracle)
		{
		nxUnbindOracleSubstepCallbacks(reinterpret_cast<unsigned char*>(physics),
			oldSubstepValues, substepProtection);
		nxUnbindOracleCallbacks(reinterpret_cast<unsigned char*>(physics),
			oldRepulsionValues, repulsionProtection);
		}
	printf("fluid substep attached-live-body actor=1 emitter_callbacks=%u run_callbacks=%u "
		"repulsion_callbacks=%u state_clear=%u body_changed=%u stack=%u mismatches=%u\n",
		substep.emitterCallbackCount,
		substep.runCallbackCount, substep.body.callbackCount, substep.stateCleared,
		substep.bodyStateChanged, substep.body.stackBalanced, substepMismatches);
	printf("fluid substep attached-live-body run=%08x.%08x.%08x.%08x.%08x.%08x.%08x.%08x\n",
		substep.run.system, substep.run.fluid, substep.run.state ? 1u : 0u,
		substep.run.vector[0], substep.run.vector[1], substep.run.vector[2],
		substep.run.timestep, substep.run.option);
	for(unsigned i = 0; i < 4; ++i)
		{
		const EmitterStateRecord& callback = substep.emitterCallbacks[i];
		printf("fluid substep attached-live-body callback%u=%08x.%08x.%08x.%08x.%08x",
			i, substep.emitterCallbacks[i].kind,
			callback.argument0, callback.argument1, callback.argument2,
			callback.enabled);
		for(unsigned j = 0; j != 12; ++j)
			printf(".%08x", callback.pose[j]);
		printf("\n");
		}
	for(unsigned i = 0; i < 3; ++i)
		{
		const RepulsionCallbackRecord& callback = substep.body.callbacks[i];
		printf("fluid substep attached-live-body repulsion%u=%08x.%08x.%08x.%08x",
			i, callback.kind, callback.system, callback.fluid, callback.emitter);
		for(unsigned j = 0; j != 6; ++j)
			printf(".%08x", callback.vectors[j]);
		printf(".%08x\n", callback.mass);
		}
	printf("fluid substep attached-live-body state=");
	for(unsigned i = 0; i != 15; ++i)
		printf("%s%08x", i ? "." : "", substep.body.words[i]);
	printf("\n");
	mismatches += substepMismatches;
	if(nxAuditModules(pairDirectory))
		++mismatches;
	FreeLibrary(fluidModel);
	return mismatches ? 1 : 0;
	}
