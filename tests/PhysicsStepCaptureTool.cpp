// Captures live calls of the oracle's phys_fn_002350 (0x0005ae50), the 1,982-byte row of the simulation step, while a scene
// with continuous collision detection runs, and writes the memory each call could reach to a fixture file for the collision
// harness to replay. The same machinery as NxPhysicsSweepCapture (see that file), with a different hook and scene.
//
// usage: NxPhysicsStepCapture <oracle pair directory> <output file>
#include "PhysicsPairLoader.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxSimpleTriangleMesh.h"
#include "NxTriangleMeshDesc.h"
#include "NxTriangleMeshShapeDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxSphereShapeDesc.h"
#include "NxCapsuleShapeDesc.h"
#include "NxPlaneShapeDesc.h"
#include "NxBodyDesc.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

class NxGroupsMask;
class NxControllerProbe
	{
	public:
	virtual ~NxControllerProbe() {}
	virtual void move(const NxVec3&, NxU32, NxReal, NxU32&, NxReal = 1.0f, const NxGroupsMask* = 0) = 0;
	virtual const NxVec3& getPosition() const = 0;
	};

static unsigned nxFloatBits(NxReal value)
	{
	unsigned bits = 0;
	memcpy(&bits, &value, sizeof(bits));
	return bits;
	}

struct CapRegion { unsigned lo, hi; std::vector<unsigned char> bytes; };
struct CapCall { unsigned eax, ecx, stack[9]; std::vector<CapRegion> regions; std::vector<unsigned> dangling; };
static std::vector<CapCall> gCalls;
static unsigned char* gResume;

static bool capReadable(unsigned address, unsigned* lo, unsigned* hi)
	{
	MEMORY_BASIC_INFORMATION mbi;
	if(!VirtualQuery(reinterpret_cast<void*>(address), &mbi, sizeof(mbi))) return false;
	if(mbi.State != MEM_COMMIT || mbi.Type != MEM_PRIVATE) return false;
	if(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
	if(!(mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) return false;
	*lo = reinterpret_cast<unsigned>(mbi.BaseAddress);
	*hi = *lo + static_cast<unsigned>(mbi.RegionSize);
	return true;
	}

static unsigned kDepth = 3;
static void capSnapshot(CapCall& call)
	{
	std::vector<unsigned> queue;
	queue.push_back(call.eax);
	queue.push_back(call.ecx);
	for(unsigned i = 0; i < 9; ++i) queue.push_back(call.stack[i]);
	for(unsigned pass = 0; pass < kDepth && !queue.empty(); ++pass)
		{
		std::vector<unsigned> next;
		for(size_t q = 0; q < queue.size(); ++q)
			{
			const unsigned v = queue[q];
			if(v < 0x10000u) continue;
			bool inside = false;
			for(size_t r = 0; r < call.regions.size(); ++r)
				if(v >= call.regions[r].lo && v < call.regions[r].hi) { inside = true; break; }
			if(inside) continue;
			unsigned rlo, rhi;
			if(!capReadable(v, &rlo, &rhi)) continue;
			// the roots are the scene and the object that steps it: the row reaches far into both
			const unsigned before = pass == 0 ? 0x100 : 0x20, after = pass == 0 ? 0x1000 : 0xe0;
			unsigned lo = v > rlo + before ? v - before : rlo;
			unsigned hi = v + after < rhi ? v + after : rhi;
			// merge with every region the new one touches
			for(size_t r = 0; r < call.regions.size();)
				{
				if(call.regions[r].hi >= lo && call.regions[r].lo <= hi)
					{
					if(call.regions[r].lo < lo) lo = call.regions[r].lo;
					if(call.regions[r].hi > hi) hi = call.regions[r].hi;
					call.regions.erase(call.regions.begin() + r);
					}
				else ++r;
				}
			if(hi > rhi) hi = rhi;
			CapRegion region;
			region.lo = lo;
			region.hi = hi;
			region.bytes.assign(reinterpret_cast<unsigned char*>(lo), reinterpret_cast<unsigned char*>(hi));
			call.regions.push_back(region);
			for(unsigned a = lo; a + 4 <= hi; a += 4) next.push_back(*reinterpret_cast<unsigned*>(a));
			if(call.regions.size() > 120) return;
			}
		queue.swap(next);
		}
	}

static bool capInside(const CapCall& call, unsigned v)
	{
	for(size_t r = 0; r < call.regions.size(); ++r)
		if(v >= call.regions[r].lo && v < call.regions[r].hi) return true;
	return false;
	}

// words (region index, byte offset pairs, flattened) that point at readable private memory the snapshot does not hold
static void capDangling(CapCall& call)
	{
	for(size_t r = 0; r < call.regions.size(); ++r)
		for(unsigned o = 0; o + 4 <= call.regions[r].bytes.size(); o += 4)
			{
			const unsigned v = *reinterpret_cast<const unsigned*>(&call.regions[r].bytes[o]);
			unsigned lo, hi;
			if(v >= 0x10000u && !capInside(call, v) && capReadable(v, &lo, &hi))
				{
				call.dangling.push_back(static_cast<unsigned>(r));
				call.dangling.push_back(o);
				}
			}
	}

static void __cdecl capRecord(unsigned eax, unsigned ecx, const unsigned* entryEsp)
	{
	CapCall call;
	call.eax = eax;
	call.ecx = ecx;
	for(unsigned i = 0; i < 9; ++i) call.stack[i] = entryEsp[1 + i];
	capSnapshot(call);
	capDangling(call);
	gCalls.push_back(call);
	}

__declspec(naked) static void capHook()
	{
	__asm
		{
		push eax
		push ecx
		push edx
		lea edx, [esp + 12]
		push edx
		push ecx
		push eax
		call capRecord
		add esp, 12
		pop edx
		pop ecx
		pop eax
		push ebp
		mov ebp, esp
		sub esp, 0x38
		jmp dword ptr [gResume]
		}
	}

static bool capInstall(HMODULE physics)
	{
	unsigned char* base = reinterpret_cast<unsigned char*>(physics);
	unsigned char* site = base + 0x0005ae50;
	static const unsigned char expected[6] = { 0x55, 0x8b, 0xec, 0x83, 0xec, 0x38 };
	if(memcmp(site, expected, 6)) return false;
	gResume = site + 6;
	DWORD protection;
	if(!VirtualProtect(site, 6, PAGE_EXECUTE_READWRITE, &protection)) return false;
	site[0] = 0xe9;
	*reinterpret_cast<int*>(site + 1) = static_cast<int>(reinterpret_cast<unsigned char*>(&capHook) - (site + 5));
	site[5] = 0x90;
	FlushInstructionCache(GetCurrentProcess(), site, 6);
	return true;
	}

static void capMove(NxController* controller, NxReal x, NxReal y, NxReal z)
	{
	NxU32 flags = 0;
	reinterpret_cast<NxControllerProbe*>(controller)->move(NxVec3(x, y, z), 0xffffffff, 0.001f, flags);
	}

int wmain(int argc, wchar_t** argv)
	{
	if(argc < 3) return 2;
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	wchar_t* openArgv[2] = { argv[0], argv[1] };
	int status = nxOpenPair(2, openArgv, "NxPhysicsSweepCapture", pairDirectory, &physics);
	if(status) return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK) return nxFail("NxCreatePhysicsSDK is missing");
	if(!capInstall(physics)) return nxFail("the hook site is not the expected instruction");
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
	if(!sdk) return nxFail("SDK creation failed");
	if(!sdk->setParameter(NX_CONTINUOUS_CD, 1.0f)) return nxFail("enabling continuous collision detection failed");
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) return nxFail("scene creation failed");
	scene->setTiming(0.1f, 1, NX_TIMESTEP_VARIABLE);
	NxBoxShapeDesc wallShape;
	wallShape.dimensions = NxVec3(0.1f, 2.0f, 2.0f);
	NxActorDesc wallDesc;
	wallDesc.shapes.pushBack(&wallShape);
	NxActor* wall = scene->createActor(wallDesc);
	NxBoxShapeDesc moverShape;
	moverShape.dimensions = NxVec3(0.25f, 0.25f, 0.25f);
	NxBodyDesc moverBody;
	NxActorDesc moverDesc;
	moverDesc.body = &moverBody;
	moverDesc.density = 1.0f;
	moverDesc.globalPose.t = NxVec3(-3.0f, 0.0f, 0.0f);
	moverDesc.shapes.pushBack(&moverShape);
	NxActor* mover = scene->createActor(moverDesc);
	if(!wall || !mover) return nxFail("fixture actor creation failed");
	mover->setLinearVelocity(NxVec3(100.0f, 0.0f, 0.0f));
	for(unsigned step = 0; step != 6; ++step)
		{
		scene->simulate(0.1f);
		scene->checkResults(NX_RIGID_BODY_FINISHED, true);
		scene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(step == 1) mover->setLinearVelocity(NxVec3(-80.0f, 5.0f, 0.0f));
		}
	sdk->releaseScene(*scene);
	sdk->setParameter(NX_CONTINUOUS_CD, 0.0f);
	sdk->release();

	FILE* out = _wfopen(argv[2], L"wb");
	if(!out) return nxFail("could not open the output file");
	const unsigned magic = 0x31505753u;	// 'SWP1'
	const unsigned count = static_cast<unsigned>(gCalls.size());
	fwrite(&magic, 4, 1, out);
	fwrite(&count, 4, 1, out);
	const unsigned imageBase = reinterpret_cast<unsigned>(physics);
	const unsigned imageSize = reinterpret_cast<const IMAGE_NT_HEADERS*>(reinterpret_cast<unsigned char*>(physics) + reinterpret_cast<const IMAGE_DOS_HEADER*>(physics)->e_lfanew)->OptionalHeader.SizeOfImage;
	fwrite(&imageBase, 4, 1, out);
	fwrite(&imageSize, 4, 1, out);
	HMODULE foundation = GetModuleHandleW(L"NxFoundation.dll");
	const unsigned foundationBase = reinterpret_cast<unsigned>(foundation);
	const unsigned foundationSize = foundation ? reinterpret_cast<const IMAGE_NT_HEADERS*>(reinterpret_cast<unsigned char*>(foundation) + reinterpret_cast<const IMAGE_DOS_HEADER*>(foundation)->e_lfanew)->OptionalHeader.SizeOfImage : 0;
	fwrite(&foundationBase, 4, 1, out);
	fwrite(&foundationSize, 4, 1, out);
	unsigned totalBytes = 0;
	for(size_t c = 0; c < gCalls.size(); ++c)
		{
		const CapCall& call = gCalls[c];
		fwrite(&call.eax, 4, 1, out);
		fwrite(&call.ecx, 4, 1, out);
		fwrite(call.stack, 4, 9, out);
		const unsigned nregions = static_cast<unsigned>(call.regions.size());
		fwrite(&nregions, 4, 1, out);
		for(size_t r = 0; r < call.regions.size(); ++r)
			{
			const unsigned size = call.regions[r].hi - call.regions[r].lo;
			fwrite(&call.regions[r].lo, 4, 1, out);
			fwrite(&size, 4, 1, out);
			fwrite(&call.regions[r].bytes[0], 1, size, out);
			totalBytes += size;
			}
		const unsigned ndangling = static_cast<unsigned>(call.dangling.size() / 2);
		fwrite(&ndangling, 4, 1, out);
		if(ndangling) fwrite(&call.dangling[0], 4, call.dangling.size(), out);
		}
	fclose(out);
	printf("captured calls=%u bytes=%u\n", count, totalBytes);
	return count ? 0 : 1;
	}
