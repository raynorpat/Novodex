// Dumps the REAL actor object graph through the oracle's own SDK, so the synthetic
// fixture can be built to measured offsets instead of derived ones.
//
// The oracle's SDK creates scenes and actors; the candidate's createScene returns 0
// (8e). So this probe runs only against the oracle, and what it prints is the
// layout the joint-descriptor rows actually walk.

#include "PhysicsPairLoader.h"

#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxActorDesc.h"
#include "NxBodyDesc.h"
#include "NxBoxShapeDesc.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

static void nxDump(const char* tag, const unsigned char* base, unsigned words)
	{
	printf("%s %p:", tag, base);
	for(unsigned i = 0; i < words; ++i)
		{
		unsigned v;
		memcpy(&v, base + i * 4, 4);
		printf(" %08x", v);
		}
	printf("\n");
	}

int wmain(int argc, wchar_t** argv)
	{
	if(argc != 3)
		{
		fprintf(stderr, "usage: %s <oracle directory> <sha256>\n", "NxSceneGraphProbe");
		return 2;
		}

	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc - 1, argv, "NxSceneGraphProbe", pairDirectory, &physics);
	if(status)
		return status;

	CreatePhysicsSDKFn createSDK =
		reinterpret_cast<CreatePhysicsSDKFn>(GetProcAddress(physics, "NxCreatePhysicsSDK"));
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

	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	sceneDesc.gravity = NxVec3(0.0f, 0.0f, 0.0f);
	NxScene* scene = sdk->createScene(sceneDesc);
	printf("scene=%p\n", static_cast<void*>(scene));
	if(!scene)
		{
		sdk->release();
		FreeLibrary(physics);
		return nxFail("scene creation failed");
		}

	NxBoxShapeDesc box;
	box.dimensions = NxVec3(1.0f, 1.0f, 1.0f);
	NxBodyDesc body;
	NxActorDesc da;
	da.body = &body;
	da.density = 1.0f;
	da.shapes.pushBack(&box);
	da.globalPose.t = NxVec3(0.0f, 0.0f, 0.0f);
	NxActor* actor = scene->createActor(da);
	printf("actor=%p\n", static_cast<void*>(actor));
	if(!actor)
		{
		sdk->releaseScene(*scene);
		sdk->release();
		FreeLibrary(physics);
		return nxFail("actor creation failed");
		}

	// The row's walk: actor+0x14 -> desc, desc+? -> shapes.first, shape+? -> body,
	// body+0x19c -> pose. Print each level's neighbourhood so the offsets are read
	// rather than assumed.
	const unsigned char* a = reinterpret_cast<const unsigned char*>(actor);
	nxDump("actor  ", a, 12);

	unsigned descPtr;
	memcpy(&descPtr, a + 0x14, 4);
	printf("actor+0x14=%08x\n", descPtr);
	if(descPtr)
		{
		const unsigned char* d = reinterpret_cast<const unsigned char*>(
			static_cast<size_t>(descPtr));
		nxDump("desc   ", d, 20);

		// The toolkit predates range-for over an initializer list, so the three
		// candidate offsets are walked explicitly.
		static const unsigned kShapeOffsets[3] = { 0x08u, 0x0cu, 0x10u };
		unsigned shapePtr = 0;
		for(unsigned k = 0; k < 3; ++k)
			{
			unsigned v;
			memcpy(&v, d + kShapeOffsets[k], 4);
			printf("desc+0x%02x=%08x\n", kShapeOffsets[k], v);
			if(v && shapePtr == 0)
				shapePtr = v;
			}

		if(shapePtr)
			{
			const unsigned char* s = reinterpret_cast<const unsigned char*>(
				static_cast<size_t>(shapePtr));
			nxDump("shape  ", s, 12);
			unsigned bodyPtr;
			memcpy(&bodyPtr, s + 8, 4);
			printf("shape+8=%08x\n", bodyPtr);
			if(bodyPtr)
				{
				const unsigned char* b = reinterpret_cast<const unsigned char*>(
					static_cast<size_t>(bodyPtr));
				unsigned posePtr;
				memcpy(&posePtr, b + 0x19c, 4);
				printf("body+0x19c=%08x\n", posePtr);
				if(posePtr)
					nxDump("pose   ", reinterpret_cast<const unsigned char*>(
						static_cast<size_t>(posePtr)), 28);
				}
			}
		}

	sdk->releaseScene(*scene);
	sdk->release();
	return nxReportPairIdentity(pairDirectory);
	}