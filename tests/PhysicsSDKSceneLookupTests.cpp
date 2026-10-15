#include "PhysicsPairLoader.h"

#include "NxPhysicsSDK.h"
#include "NxSceneDesc.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

int wmain(int argc, wchar_t** argv)
	{
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsSDKSceneLookupTests", pairDirectory, &physics);
	if(status)
		return status;

	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(
		GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK)
		{
		FreeLibrary(physics);
		return nxFail("NxCreatePhysicsSDK missing");
		}

	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, 0);
	if(!sdk)
		{
		FreeLibrary(physics);
		return nxFail("SDK creation failed");
		}

	NxSceneDesc desc;
	NxScene* first = sdk->createScene(desc);
	NxScene* second = sdk->createScene(desc);
	const bool ordered = first && second
		&& sdk->getNbScenes() == 2
		&& sdk->getScene(0) == first
		&& sdk->getScene(1) == second;
	printf("scene_lookup created=%u count=%u first=%u second=%u\n",
		first && second ? 1u : 0u, sdk->getNbScenes(),
		sdk->getScene(0) == first ? 1u : 0u,
		sdk->getScene(1) == second ? 1u : 0u);
	if(!ordered)
		{
		if(first) sdk->releaseScene(*first);
		if(second) sdk->releaseScene(*second);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("scene index lookup did not preserve insertion order");
		}

	sdk->releaseScene(*first);
	const bool survivor = sdk->getNbScenes() == 1 && sdk->getScene(0) == second;
	printf("scene_lookup after_release count=%u survivor=%u\n",
		sdk->getNbScenes(), survivor ? 1u : 0u);
	if(!survivor)
		{
		sdk->releaseScene(*second);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("swap-removal changed the surviving scene lookup");
		}

	sdk->releaseScene(*second);
	const bool empty = sdk->getNbScenes() == 0;
	printf("scene_lookup empty count=%u\n", sdk->getNbScenes());
	sdk->release();
	status = nxReportPairIdentity(pairDirectory);
	FreeLibrary(physics);
	if(status)
		return status;
	return empty ? 0 : nxFail("scene registry was not empty after release");
	}
