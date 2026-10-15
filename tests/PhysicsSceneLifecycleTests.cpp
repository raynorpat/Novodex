// Focused public-DLL scene-array lifecycle differential. Exercises repeated
// creation, replace-with-last removal, stable public wrappers, and teardown.
#include "PhysicsPairLoader.h"

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32,
	NxUserAllocator*, NxUserOutputStream*);

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
		if(sdk->getNbScenes() != 3 || sdk->getScene(2) != appended)
			++mismatches;

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
