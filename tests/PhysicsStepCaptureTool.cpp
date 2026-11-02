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
#include "NxUserContactReport.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

class NxGroupsMask;
class NxControllerProbe
	{
	public:
	virtual ~NxControllerProbe() {}
	virtual void move(const NxVec3&, NxU32, NxReal, NxU32&, NxReal = 1.0f, const NxGroupsMask* = 0) = 0;
	virtual const NxVec3& getPosition() const = 0;
	};

// takes every contact report and does nothing with it: the step only has pairs to hand over when a report is set
class NxQuietContactReport : public NxUserContactReport
	{
	public:
	virtual void onContactNotify(NxContactPair&, NxU32) {}
	};

class NxQuietTriggerReport : public NxUserTriggerReport
	{
	public:
	virtual void onTrigger(NxShape&, NxShape&, NxTriggerFlag) {}
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

static unsigned kDepth = 4;
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
			const unsigned before = pass == 0 ? 0x100 : 0x40, after = pass == 0 ? 0x1000 : 0x300;
			unsigned lo = v > rlo + before ? v - before : rlo;
			unsigned hi = v + after < rhi ? v + after : rhi;
			// word-aligned, so the replay's word-by-word pointer rebasing lines up with the original fields
			lo &= ~3u;
			hi = (hi + 3u) & ~3u;
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
			if(call.regions.size() > 400) return;
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
	sceneDesc.gravity = NxVec3(0.0f, -9.8f, 0.0f);
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) return nxFail("scene creation failed");
	scene->setTiming(0.1f, 1, NX_TIMESTEP_VARIABLE);
	static NxQuietContactReport report;
	scene->setUserContactReport(&report);
	static NxQuietTriggerReport triggerReport;
	scene->setUserTriggerReport(&triggerReport);
	// trigger volumes the fast bodies and the pile cross: enter, stay and leave events in every step
	for(unsigned k = 0; k < 4; ++k)
		{
		NxBoxShapeDesc triggerShape;
		triggerShape.dimensions = NxVec3(1.0f + 0.5f * k, 1.5f, 1.5f);
		triggerShape.shapeFlags |= NX_TRIGGER_ENABLE;
		NxActorDesc triggerDesc;
		triggerDesc.shapes.pushBack(&triggerShape);
		triggerDesc.globalPose.t = NxVec3(-2.0f + 2.0f * k, -0.5f * k, -1.0f + 0.7f * k);
		scene->createActor(triggerDesc);
		}
	sdk->setActorGroupPairFlags(0, 0, NX_NOTIFY_ON_START_TOUCH | NX_NOTIFY_ON_END_TOUCH | NX_NOTIFY_ON_TOUCH);
	// two thin walls and a floor, and a crowd of fast bodies (boxes and spheres) crossing them in different directions
	NxBoxShapeDesc wallShape;
	wallShape.dimensions = NxVec3(0.1f, 3.0f, 3.0f);
	NxActorDesc wallDesc;
	wallDesc.shapes.pushBack(&wallShape);
	NxActor* wall = scene->createActor(wallDesc);
	NxBoxShapeDesc wall2Shape;
	wall2Shape.dimensions = NxVec3(3.0f, 3.0f, 0.1f);
	NxActorDesc wall2Desc;
	wall2Desc.shapes.pushBack(&wall2Shape);
	wall2Desc.globalPose.t = NxVec3(0.0f, 0.0f, 2.0f);
	scene->createActor(wall2Desc);
	NxBoxShapeDesc floorShape;
	floorShape.dimensions = NxVec3(6.0f, 0.1f, 6.0f);
	NxActorDesc floorDesc;
	floorDesc.shapes.pushBack(&floorShape);
	floorDesc.globalPose.t = NxVec3(0.0f, -2.0f, 0.0f);
	scene->createActor(floorDesc);
	NxActor* movers[8];
	for(unsigned i = 0; i < 8; ++i)
		{
		NxBoxShapeDesc boxMover;
		boxMover.dimensions = NxVec3(0.2f, 0.2f, 0.2f);
		NxSphereShapeDesc sphereMover;
		sphereMover.radius = 0.2f;
		NxBodyDesc body;
		NxActorDesc desc;
		desc.body = &body;
		desc.density = 1.0f + (float) i;
		if(i & 1) desc.shapes.pushBack(&sphereMover); else desc.shapes.pushBack(&boxMover);
		desc.globalPose.t = NxVec3(-3.0f + 0.3f * i, -1.0f + 0.4f * i, -1.5f + 0.5f * i);
		movers[i] = scene->createActor(desc);
		if(!movers[i]) return nxFail("fixture actor creation failed");
		movers[i]->setLinearVelocity(NxVec3(60.0f + 10.0f * i, (i & 2) ? 20.0f : -15.0f, (i & 4) ? 25.0f : 0.0f));
		}
	if(!wall) return nxFail("fixture actor creation failed");
	// a pile of overlapping boxes dropped on the floor, so the broadphase has pairs to report
	for(unsigned i = 0; i < 10; ++i)
		{
		NxBoxShapeDesc pileShape;
		pileShape.dimensions = NxVec3(0.3f, 0.3f, 0.3f);
		NxBodyDesc pileBody;
		NxActorDesc pileDesc;
		pileDesc.body = &pileBody;
		pileDesc.density = 1.0f;
		pileDesc.shapes.pushBack(&pileShape);
		pileDesc.globalPose.t = NxVec3(3.0f + 0.2f * (i % 3), -1.5f + 0.45f * i, 0.1f * (i % 4));
		if(!scene->createActor(pileDesc)) return nxFail("pile actor creation failed");
		}
	for(unsigned step = 0; step != 14; ++step)
		{
		scene->simulate(0.05f);
		scene->checkResults(NX_RIGID_BODY_FINISHED, true);
		scene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(step % 3 == 1) for(unsigned i = 0; i < 8; ++i) movers[i]->setLinearVelocity(NxVec3((i & 1) ? -70.0f : 70.0f, (float) (i * 3) - 10.0f, (i & 2) ? 30.0f : -30.0f));
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
