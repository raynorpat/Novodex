#include "PhysicsPairLoader.h"

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"
#include <stdio.h>
#include <string.h>

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

static NxU32 nxCcdFloatBits(NxReal value)
	{
	union { NxReal real; NxU32 bits; } result;
	result.real = value;
	return result.bits;
	}

class NxCcdSphereTarget
	{
	public:
	NxCcdSphereTarget(): calls(0) {}
	virtual void slot0() {}
	virtual void slot1() {}
	virtual void slot2() {}
	virtual void slot3() {}
	virtual void slot4() {}
	virtual void slot5() {}
	virtual void slot6() {}
	virtual void slot7() {}
	virtual void slot8() {}
	virtual void slot9() {}
	virtual void slot10(NxReal* sphere)
		{
		++calls;
		sphere[0] = 4.0f;
		sphere[1] = 6.0f;
		sphere[2] = 3.0f;
		sphere[3] = 2.0f;
		}
	unsigned calls;
	};

static bool nxCcdResolvePrivateRow740(HMODULE physics, const wchar_t* pairDirectory,
	void** function)
	{
	const NxU32 oracleRva = 0x00016c20;
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
		const char* const decorated = "?row000740@Row000740Fixture@@QAEXM@Z";
		char* symbol = strstr(line, decorated);
		if(symbol)
			{
			char* address = symbol + strlen(decorated);
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

static bool nxCcdTestPrivateRow740(HMODULE physics, const wchar_t* pairDirectory)
	{
	void* rowAddress = 0;
	if(!nxCcdResolvePrivateRow740(physics, pairDirectory, &rowAddress))
		return false;
	alignas(16) unsigned char body[0x260] = {};
	alignas(16) unsigned char actorBody[0x20] = {};
	NxCcdSphereTarget sphereTarget;
	*reinterpret_cast<void**>(body + 0x19c) = actorBody;
	*reinterpret_cast<void**>(actorBody + 0x10) = &sphereTarget;
	*reinterpret_cast<NxU32*>(body + 0x1e4) = 0x10;
	*reinterpret_cast<NxReal*>(body + 0x158) = 1.0f;
	*reinterpret_cast<NxReal*>(body + 0x15c) = 2.0f;
	*reinterpret_cast<NxReal*>(body + 0x160) = 3.0f;
	*reinterpret_cast<NxReal*>(body + 0x1a0) = 2.0f;
	*reinterpret_cast<NxReal*>(body + 0x1a4) = -3.0f;
	*reinterpret_cast<NxReal*>(body + 0x1a8) = 0.0f;
	typedef void (__thiscall *CcdSweepRowFn)(void*, NxReal);
	reinterpret_cast<CcdSweepRowFn>(rowAddress)(body, 0.5f);
	const NxU32 flags = *reinterpret_cast<NxU32*>(body + 0x1e4);
	const NxReal minX = *reinterpret_cast<NxReal*>(body + 0x244);
	const NxReal minY = *reinterpret_cast<NxReal*>(body + 0x248);
	const NxReal minZ = *reinterpret_cast<NxReal*>(body + 0x24c);
	const NxReal maxX = *reinterpret_cast<NxReal*>(body + 0x250);
	const NxReal maxY = *reinterpret_cast<NxReal*>(body + 0x254);
	const NxReal maxZ = *reinterpret_cast<NxReal*>(body + 0x258);
	const bool matches = (flags & 0x210u) == 0x210u && sphereTarget.calls == 1 &&
		nxCcdFloatBits(minX) == 0xc0c00000 && nxCcdFloatBits(minY) == 0xc0d00000 &&
		nxCcdFloatBits(minZ) == 0xc0800000 && nxCcdFloatBits(maxX) == 0x41100000 &&
		nxCcdFloatBits(maxY) == 0x41100000 && nxCcdFloatBits(maxZ) == 0x41200000;
	printf("ccd row740 direct valid=%u bounds_min=%08x.%08x.%08x bounds_max=%08x.%08x.%08x\n",
		matches ? 1u : 0u, nxCcdFloatBits(minX), nxCcdFloatBits(minY), nxCcdFloatBits(minZ),
		nxCcdFloatBits(maxX), nxCcdFloatBits(maxY), nxCcdFloatBits(maxZ));
	return matches;
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	const int opened = nxOpenPair(argc, argv, "NxPhysicsCcdSimulationTests",
		pairDirectory, &physics);
	if(opened)
		return opened;
	if(!nxCcdTestPrivateRow740(physics, pairDirectory))
		{
		FreeLibrary(physics);
		return nxFail("row 000740 failed the controlled swept-bounds probe");
		}
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK)
		return nxFail("NxCreatePhysicsSDK missing");
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
	if(!sdk)
		return nxFail("SDK creation failed");
	if(!sdk->setParameter(NX_CONTINUOUS_CD, 1.0f))
		return nxFail("enabling continuous collision detection failed");

	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene)
		return nxFail("CCD scene creation failed");
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
	if(!wall || !mover)
		return nxFail("CCD fixture actor creation failed");
	mover->setLinearVelocity(NxVec3(100.0f, 0.0f, 0.0f));

	for(unsigned step = 0; step != 2; ++step)
		{
		scene->simulate(0.1f);
		const bool ready = scene->checkResults(NX_RIGID_BODY_FINISHED, true);
		const bool fetched = scene->fetchResults(NX_RIGID_BODY_FINISHED, true);
		if(!ready || !fetched)
			return nxFail("CCD simulation result was not ready and fetched");
		NxVec3 velocity;
		mover->getLinearVelocity(velocity);
		if(step == 1 && (velocity.x < 0.399f || velocity.x > 0.401f))
			return nxFail("CCD response velocity is outside its expected range");
		printf("ccd step=%u x=%08x vx=%.3f ready=%u fetched=%u\n", step,
			nxCcdFloatBits(mover->getGlobalPosition().x), velocity.x,
			ready ? 1u : 0u, fetched ? 1u : 0u);
		}

	sdk->releaseScene(*scene);
	sdk->setParameter(NX_CONTINUOUS_CD, 0.0f);
	sdk->release();
	const int audited = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	return audited;
	}
