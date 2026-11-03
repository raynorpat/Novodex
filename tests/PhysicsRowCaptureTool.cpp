// Captures live calls of any one row of the oracle, while the controller scene of NxPhysicsSweepCapture runs, and writes the
// memory each call could reach to a fixture file for the collision harness to replay (format 'RWC1').
//
// usage: NxPhysicsRowCapture <oracle pair directory> <output file> <row rva, hex> <bytes to displace>
//
// A hook is written over the first instructions of the row (whole instructions, at least five bytes, none relative); it
// records eax, ecx, edx and sixteen stack words, runs the displaced bytes and resumes. Every pointer-looking word in a
// captured argument that points into private, committed, readable memory is followed (the regions are merged), so the
// fixture holds the whole structure graph the call read. Pointers into the oracle's image are left alone.
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
struct CapCall { unsigned eax, ecx, edx, cache, stack[16]; std::vector<CapRegion> regions; std::vector<unsigned> dangling; };
static std::vector<CapCall> gCalls;
static unsigned char* gStub;
static const unsigned* gCacheWord;

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

// The process heap also holds text (the environment block with session tokens and account ids, the command line, paths, user
// and machine names). A row reads structures, not strings, so every run of ten or more printable characters, narrow or wide,
// is zeroed in what is stored.
static void capScrubText(std::vector<unsigned char>& bytes)
	{
	size_t i = 0;
	while(i < bytes.size())
		{
		size_t n = 0;
		while(i + n < bytes.size() && bytes[i + n] >= 0x20 && bytes[i + n] < 0x7f) ++n;
		size_t w = 0;
		while(i + 2 * w + 1 < bytes.size() && bytes[i + 2 * w] >= 0x20 && bytes[i + 2 * w] < 0x7f && bytes[i + 2 * w + 1] == 0) ++w;
		if(n >= 10) { memset(&bytes[i], 0, n); i += n; }
		else if(w >= 10) { memset(&bytes[i], 0, 2 * w); i += 2 * w; }
		else ++i;
		}
	}

static unsigned kDepth = 3;
// how far the first level of the snapshot reaches around each root word; the rows that read deep into a scene need more
static unsigned kFirstBefore = 0x20, kFirstAfter = 0xe0;
static void capSnapshot(CapCall& call)
	{
	std::vector<unsigned> queue;
	queue.push_back(call.eax);
	queue.push_back(call.ecx);
	queue.push_back(call.edx);
	queue.push_back(call.cache);
	for(unsigned i = 0; i < 16; ++i) queue.push_back(call.stack[i]);
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
			const unsigned before = pass == 0 ? kFirstBefore : 0x20, after = pass == 0 ? kFirstAfter : 0xe0;
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
			capScrubText(region.bytes);
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

static void __cdecl capRecord(unsigned eax, unsigned ecx, unsigned edx, const unsigned* entryEsp)
	{
	CapCall call;
	call.eax = eax;
	call.ecx = ecx;
	call.edx = edx;
	call.cache = gCacheWord ? *gCacheWord : 0;
	for(unsigned i = 0; i < 16; ++i) call.stack[i] = entryEsp[1 + i];
	capSnapshot(call);
	capDangling(call);
	gCalls.push_back(call);
	}

static bool capInstall(HMODULE physics, unsigned rva, unsigned length)
	{
	unsigned char* base = reinterpret_cast<unsigned char*>(physics);
	unsigned char* site = base + rva;
	// the sweep cache pointer the rows read through an absolute operand (0x000572a3: mov eax, [address])
	gCacheWord = *reinterpret_cast<const unsigned* const*>(base + 0x572a4);
	if(length < 5 || length > 16) return false;
	unsigned char* stub = static_cast<unsigned char*>(VirtualAlloc(0, 128, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
	if(!stub) return false;
	unsigned char* p = stub;
	// push eax, ecx, edx; lea edx, [esp + 12]; push edx; push dword ptr [esp + 4] (the saved edx); push ecx; push eax
	static const unsigned char head[] = { 0x50, 0x51, 0x52, 0x8d, 0x54, 0x24, 0x0c, 0x52, 0xff, 0x74, 0x24, 0x04, 0x51, 0x50 };
	memcpy(p, head, sizeof(head)); p += sizeof(head);
	*p++ = 0xe8;
	*reinterpret_cast<int*>(p) = static_cast<int>(reinterpret_cast<unsigned char*>(&capRecord) - (p + 4)); p += 4;
	// add esp, 16; pop edx; pop ecx; pop eax
	static const unsigned char tail[] = { 0x83, 0xc4, 0x10, 0x5a, 0x59, 0x58 };
	memcpy(p, tail, sizeof(tail)); p += sizeof(tail);
	memcpy(p, site, length); p += length;
	*p++ = 0xe9;
	*reinterpret_cast<int*>(p) = static_cast<int>((site + length) - (p + 4)); p += 4;
	gStub = stub;
	DWORD protection;
	if(!VirtualProtect(site, length, PAGE_EXECUTE_READWRITE, &protection)) return false;
	site[0] = 0xe9;
	*reinterpret_cast<int*>(site + 1) = static_cast<int>(stub - (site + 5));
	for(unsigned i = 5; i < length; ++i) site[i] = 0x90;
	FlushInstructionCache(GetCurrentProcess(), site, length);
	return true;
	}

static void capMove(NxController* controller, NxReal x, NxReal y, NxReal z)
	{
	NxU32 flags = 0;
	reinterpret_cast<NxControllerProbe*>(controller)->move(NxVec3(x, y, z), 0xffffffff, 0.001f, flags);
	}

int wmain(int argc, wchar_t** argv)
	{
	if(argc < 5) return 2;
	if(argc > 5) { kFirstBefore = 0x100; kFirstAfter = (unsigned) wcstoul(argv[5], 0, 16); }
	const unsigned rowRva = (unsigned) wcstoul(argv[3], 0, 16), rowLength = (unsigned) wcstoul(argv[4], 0, 0);
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	wchar_t* openArgv[2] = { argv[0], argv[1] };
	int status = nxOpenPair(2, openArgv, "NxPhysicsRowCapture", pairDirectory, &physics);
	if(status) return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK) return nxFail("NxCreatePhysicsSDK is missing");
	if(!capInstall(physics, rowRva, rowLength)) return nxFail("the hook could not be installed");
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
	if(!sdk) return nxFail("SDK creation failed");

	// a small rising face and a second, tilted one, so the swept box meets more than one triangle
	const NxPoint points[] = {
		NxPoint(0.35f, 0.4f, 0.35f), NxPoint(0.35f, 0.4f, 0.65f), NxPoint(0.65f, 0.6f, 0.35f), NxPoint(0.65f, 0.6f, 0.65f),
		NxPoint(0.9f, 0.0f, 0.1f), NxPoint(1.4f, 0.3f, 0.2f), NxPoint(1.0f, 0.1f, 0.7f)};
	const NxU32 triangles[] = {0, 1, 2, 1, 3, 2, 4, 5, 6};
	NxTriangleMeshDesc meshDesc;
	meshDesc.numVertices = 7;
	meshDesc.numTriangles = 3;
	meshDesc.pointStrideBytes = sizeof(NxPoint);
	meshDesc.triangleStrideBytes = 3 * sizeof(NxU32);
	meshDesc.points = points;
	meshDesc.triangles = triangles;
	NxTriangleMesh* mesh = sdk->createTriangleMesh(meshDesc);
	if(!mesh) return nxFail("mesh cooking failed");
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene) return nxFail("scene creation failed");
	NxTriangleMeshShapeDesc shape;
	shape.meshData = mesh;
	NxActorDesc obstacleDesc;
	obstacleDesc.shapes.pushBack(&shape);
	NxActor* obstacle = scene->createActor(obstacleDesc);
	// static boxes and a sphere next to the mesh, so the controller's sweep meets more than triangles
	NxBoxShapeDesc boxShape;
	boxShape.dimensions = NxVec3(0.3f, 0.3f, 0.3f);
	NxActorDesc boxDesc;
	boxDesc.shapes.pushBack(&boxShape);
	boxDesc.globalPose.t = NxVec3(0.2f, 0.0f, 1.2f);
	scene->createActor(boxDesc);
	// a dynamic body, a capsule and a ground plane
	NxBoxShapeDesc bodyShape;
	bodyShape.dimensions = NxVec3(0.2f, 0.2f, 0.2f);
	NxBodyDesc bodyDesc;
	NxActorDesc dynamicDesc;
	dynamicDesc.shapes.pushBack(&bodyShape);
	dynamicDesc.body = &bodyDesc;
	dynamicDesc.density = 1.0f;
	dynamicDesc.globalPose.t = NxVec3(0.6f, 0.9f, 0.9f);
	scene->createActor(dynamicDesc);
	NxCapsuleShapeDesc capsuleShape;
	capsuleShape.radius = 0.2f;
	capsuleShape.height = 0.6f;
	NxActorDesc capsuleDesc;
	capsuleDesc.shapes.pushBack(&capsuleShape);
	capsuleDesc.globalPose.t = NxVec3(1.2f, 0.3f, 0.9f);
	scene->createActor(capsuleDesc);
	NxPlaneShapeDesc planeShape;
	planeShape.normal = NxVec3(0.0f, 1.0f, 0.0f);
	planeShape.d = -0.5f;
	NxActorDesc planeDesc;
	planeDesc.shapes.pushBack(&planeShape);
	scene->createActor(planeDesc);
	NxSphereShapeDesc sphereShape;
	sphereShape.radius = 0.4f;
	NxActorDesc sphereDesc;
	sphereDesc.shapes.pushBack(&sphereShape);
	sphereDesc.globalPose.t = NxVec3(-0.8f, 0.2f, 0.3f);
	scene->createActor(sphereDesc);

	alignas(4) unsigned char controllerStorage[0x80] = {};
	*reinterpret_cast<NxU32*>(controllerStorage + 0x0c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(controllerStorage + 0x10) = nxFloatBits(2.0f);
	*reinterpret_cast<NxU32*>(controllerStorage + 0x14) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(controllerStorage + 0x18) = 1;
	*reinterpret_cast<NxU32*>(controllerStorage + 0x1c) = nxFloatBits(0.95f);
	*reinterpret_cast<NxU32*>(controllerStorage + 0x20) = 1;
	*reinterpret_cast<NxU32*>(controllerStorage + 0x24) = nxFloatBits(0.7f);
	*reinterpret_cast<NxU32*>(controllerStorage + 0x2c) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(controllerStorage + 0x30) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(controllerStorage + 0x34) = nxFloatBits(0.5f);
	*reinterpret_cast<NxU32*>(controllerStorage + 0x38) = nxFloatBits(0.5f);
	NxController* controller = scene->createController(*reinterpret_cast<const NxControllerDesc*>(controllerStorage));
	if(!obstacle || !controller) return nxFail("fixture setup failed");
	// a second controller, so the first one's sweeps meet another controller's box
	alignas(4) unsigned char otherStorage[0x80];
	memcpy(otherStorage, controllerStorage, sizeof(otherStorage));
	NxController* other = scene->createController(*reinterpret_cast<const NxControllerDesc*>(otherStorage));
	if(!other) return nxFail("second controller setup failed");
	capMove(other, 0.9f, 0.0f, 0.2f);
	static const float moves[][3] = {
		{0.0f, -3.0f, 0.0f}, {0.6f, 0.0f, 0.1f}, {0.0f, -2.0f, 0.4f}, {-0.3f, 0.5f, 0.2f}, {0.5f, -1.0f, 0.5f},
		{0.0f, 0.0f, 0.6f}, {0.4f, -2.5f, -0.2f}, {-0.8f, 0.0f, 0.0f}, {0.2f, -1.5f, 0.3f}, {0.7f, 0.0f, -0.4f}};
	for(unsigned i = 0; i < sizeof(moves) / sizeof(moves[0]); ++i) capMove(controller, moves[i][0], moves[i][1], moves[i][2]);

	scene->releaseController(*other);
	scene->releaseController(*controller);
	sdk->releaseScene(*scene);
	sdk->releaseTriangleMesh(*mesh);
	sdk->release();

	FILE* out = _wfopen(argv[2], L"wb");
	if(!out) return nxFail("could not open the output file");
	const unsigned magic = 0x31435752u;	// 'RWC1'
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
		fwrite(&call.edx, 4, 1, out);
		fwrite(&call.cache, 4, 1, out);
		fwrite(call.stack, 4, 16, out);
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
