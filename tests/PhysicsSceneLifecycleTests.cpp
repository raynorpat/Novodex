// Focused public-DLL scene-array lifecycle differential. Exercises repeated
// creation, replace-with-last removal, stable wrappers, pruner-pool growth and
// generation-tagged slot reuse through small static/dynamic anchor actors.
#include "PhysicsPairLoader.h"

#include "NxPhysicsSDK.h"
#include "NxActor.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"
#include "NxScene.h"
#include "NxSceneDesc.h"

#include <string.h>

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32,
	NxUserAllocator*, NxUserOutputStream*);

static bool nxGetScenePrunerHandles(NxScene* scene, NxU32& staticHandle,
	NxU32& dynamicHandle)
	{
	NxU32 internalScene = 0;
	memcpy(&internalScene, reinterpret_cast<const NxU8*>(scene) + 0x24,
		sizeof(internalScene));
	if(!internalScene)
		return false;
	const NxU8* const engine = reinterpret_cast<const NxU8*>(internalScene) + 0x624;
	NxU32 staticPruner = 0;
	NxU32 dynamicPruner = 0;
	memcpy(&staticPruner, engine + 0x1c, sizeof(staticPruner));
	memcpy(&dynamicPruner, engine + 0x24, sizeof(dynamicPruner));
	if(!staticPruner || !dynamicPruner)
		{
		printf("scene registration growth pointers wrapper=%08x internal=%08x engine=%08x static=%08x dynamic=%08x\n",
			reinterpret_cast<NxU32>(scene), internalScene,
		reinterpret_cast<NxU32>(engine), staticPruner, dynamicPruner);
		return false;
		}
	memcpy(&staticHandle, reinterpret_cast<const NxU8*>(staticPruner) + 0x34,
		sizeof(staticHandle));
	memcpy(&dynamicHandle, reinterpret_cast<const NxU8*>(dynamicPruner) + 0x34,
		sizeof(dynamicHandle));
	return true;
	}

static bool nxCreatePrunerAnchor(NxScene* scene)
	{
	NxBoxShapeDesc box;
	NxActorDesc staticActor;
	staticActor.shapes.pushBack(&box);
	NxActor* createdStatic = scene->createActor(staticActor);
	NxBodyDesc body;
	body.mass = 1.0f;
	body.massSpaceInertia = NxVec3(1.0f, 1.0f, 1.0f);
	NxActorDesc dynamicActor;
	dynamicActor.body = &body;
	dynamicActor.shapes.pushBack(&box);
	NxActor* createdDynamic = scene->createActor(dynamicActor);
	return createdStatic != 0 && createdDynamic != 0;
	}

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsSceneLifecycleTests",
		pairDirectory, &physics);
	if(status) return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK) return nxFail("NxCreatePhysicsSDK is missing");
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
	if(!sdk) return nxFail("SDK creation failed");

	NxSceneDesc desc;
	desc.setToDefault();
	unsigned creates = 0;
	unsigned releases = 0;
	unsigned mismatches = sdk->getNbScenes() == 0 ? 0 : 1;
	for(unsigned cycle = 0; cycle < 8; ++cycle)
		{
		NxScene* first = sdk->createScene(desc);
		NxScene* middle = sdk->createScene(desc);
		NxScene* last = sdk->createScene(desc);
		creates += (first != 0) + (middle != 0) + (last != 0);
		if(!first || !middle || !last)
			{
			++mismatches;
			break;
			}
		if(sdk->getNbScenes() != 3 || sdk->getScene(0) != first ||
			sdk->getScene(1) != middle || sdk->getScene(2) != last)
			++mismatches;
		if(cycle == 0)
			{
			const bool anchors = nxCreatePrunerAnchor(first) &&
				nxCreatePrunerAnchor(middle) && nxCreatePrunerAnchor(last);
			if(!anchors)
				++mismatches;
			NxScene* scenes[] = {first, middle, last};
			NxU32 expected[] = {0, 1, 2, 3, 4, 5};
			NxU32 actual[6] = {0xffffffffu, 0xffffffffu, 0xffffffffu,
				0xffffffffu, 0xffffffffu, 0xffffffffu};
			bool growthHandlesMatch = true;
			for(unsigned i = 0; i < 3; ++i)
				{
				NxU32 staticHandle = 0xffffffffu;
				NxU32 dynamicHandle = 0xffffffffu;
				const bool read = nxGetScenePrunerHandles(scenes[i], staticHandle, dynamicHandle);
				actual[i * 2] = staticHandle;
				actual[i * 2 + 1] = dynamicHandle;
				growthHandlesMatch = growthHandlesMatch && read &&
					staticHandle == expected[i * 2] && dynamicHandle == expected[i * 2 + 1];
				}
			printf("scene registration growth active=6 handles=%08x.%08x.%08x.%08x.%08x.%08x stable=%u\n",
				actual[0], actual[1], actual[2], actual[3], actual[4], actual[5],
				growthHandlesMatch ? 1u : 0u);
			if(!growthHandlesMatch)
				++mismatches;
			}

		sdk->releaseScene(*middle);
		++releases;
		if(sdk->getNbScenes() != 2 || sdk->getScene(0) != first ||
			sdk->getScene(1) != last)
			++mismatches;

		NxScene* appended = sdk->createScene(desc);
		creates += appended != 0;
		if(!appended)
			{
			++mismatches;
			break;
			}
		if(cycle == 0 && !nxCreatePrunerAnchor(appended))
			++mismatches;
		if(sdk->getNbScenes() != 3 || sdk->getScene(2) != appended)
			++mismatches;
		if(cycle == 0)
			{
			NxU32 staticHandle = 0xffffffffu;
			NxU32 dynamicHandle = 0xffffffffu;
			const bool read = nxGetScenePrunerHandles(appended, staticHandle, dynamicHandle);
			const bool reused = read && staticHandle == 0x00010003u &&
				dynamicHandle == 0x00010002u;
			printf("scene registration growth reused static=%08x dynamic=%08x stable=%u\n",
				staticHandle, dynamicHandle, reused ? 1u : 0u);
			if(!reused)
				++mismatches;
			}

		// The SDK uses an unsorted scene array: removing the first member moves
		// the last scene into slot zero.
		sdk->releaseScene(*first);
		++releases;
		if(sdk->getNbScenes() != 2 || sdk->getScene(0) != appended ||
			sdk->getScene(1) != last)
			++mismatches;

		sdk->releaseScene(*last);
		++releases;
		if(sdk->getNbScenes() != 1 || sdk->getScene(0) != appended)
			++mismatches;

		sdk->releaseScene(*appended);
		++releases;
		if(sdk->getNbScenes() != 0)
			++mismatches;
		}

	const bool stable = mismatches == 0 && creates == 32 && releases == 32;
	printf("scene lifecycle cycles=8 creates=%u releases=%u stable=%u mismatches=%u\n",
		creates, releases, stable ? 1u : 0u, mismatches);
	sdk->release();
	if(!stable) return nxFail("repeated scene lifecycle did not match the public contract");
	return nxReportPairIdentity(pairDirectory);
	}
