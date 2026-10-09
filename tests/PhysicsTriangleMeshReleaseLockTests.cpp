#include "PhysicsPairLoader.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include <string.h>

#include "NxPhysicsSDK.h"
#include "NxScene.h"
#include "NxSceneDesc.h"
#include "NxSimpleTriangleMesh.h"
#include "NxTriangleMeshDesc.h"
#include "NxFoundationSDK.h"
#include "NxUserOutputStream.h"
#include "NxUserAllocator.h"
#include "../Physics/src/include/NpSceneGuard.h"

typedef NxPhysicsSDK* (NX_CALL_CONV *CreatePhysicsSDKFn)(NxU32, NxUserAllocator*, NxUserOutputStream*);

class NxTriangleMeshLockOutput : public NxUserOutputStream
	{
	public:
	NxTriangleMeshLockOutput() : errors(0), code(NXE_NO_ERROR), line(0)
		{ message[0] = 0; file[0] = 0; }
	void reportError(NxErrorCode errorCode, const char* errorMessage, const char* errorFile, int errorLine)
		{
		++errors;
		code = errorCode;
		line = errorLine;
		strncpy_s(message, sizeof(message), errorMessage ? errorMessage : "", _TRUNCATE);
		strncpy_s(file, sizeof(file), errorFile ? errorFile : "", _TRUNCATE);
		}
	NxAssertResponse reportAssertViolation(const char*, const char*, int) { return NX_AR_CONTINUE; }
	void print(const char*) {}
	void reset() { errors = 0; code = NXE_NO_ERROR; line = 0; message[0] = 0; file[0] = 0; }
	unsigned errors;
	NxErrorCode code;
	int line;
	char message[256];
	char file[128];
	};

struct NxHeldSceneWriteLock
	{
	void* link;
	HANDLE ready;
	HANDLE release;
	};

static DWORD WINAPI nxHoldSceneWriteLock(void* context)
	{
	NxHeldSceneWriteLock* held = static_cast<NxHeldSceneWriteLock*>(context);
	nxNpSceneGuardEnter(held->link);
	SetEvent(held->ready);
	WaitForSingleObject(held->release, 1500);
	nxNpSceneGuardLeave(held->link);
	return 0;
	}

int wmain(int argc, wchar_t** argv)
	{
	setvbuf(stdout, 0, _IONBF, 0);
	if(argc != 2)
		return nxFail("usage: NxPhysicsTriangleMeshReleaseLockTests <pair directory>");
	wchar_t pairDirectory[MAX_PATH];
	HMODULE physics = 0;
	int status = nxOpenPair(argc, argv, "NxPhysicsTriangleMeshReleaseLockTests", pairDirectory, &physics);
	if(status)
		return status;
	CreatePhysicsSDKFn createSDK = reinterpret_cast<CreatePhysicsSDKFn>(GetProcAddress(physics, "NxCreatePhysicsSDK"));
	if(!createSDK)
		return nxFail("NxCreatePhysicsSDK missing");

	NxTriangleMeshLockOutput output;
	NxPhysicsSDK* sdk = createSDK(NX_PHYSICS_SDK_VERSION, 0, &output);
	if(!sdk)
		return nxFail("SDK creation failed");
	printf("triangle_mesh_release stage=sdk\n");
	NxSceneDesc sceneDesc;
	sceneDesc.setToDefault();
	NxScene* scene = sdk->createScene(sceneDesc);
	if(!scene)
		return nxFail("scene creation failed");
	printf("triangle_mesh_release stage=scene\n");
	NxVec3 vertices[6] = {
		NxVec3(-2.0f, 0.0f, -2.0f), NxVec3(2.0f, 0.0f, -2.0f), NxVec3(-2.0f, 0.0f, 2.0f),
		NxVec3(2.0f, 0.0f, -2.0f), NxVec3(2.0f, 0.0f, 2.0f), NxVec3(-2.0f, 0.0f, 2.0f) };
	NxU32 indices[6] = { 0, 1, 2, 3, 4, 5 };
	NxTriangleMeshDesc meshDesc;
	meshDesc.numVertices = 6;
	meshDesc.numTriangles = 2;
	meshDesc.points = vertices;
	meshDesc.pointStrideBytes = sizeof(NxVec3);
	meshDesc.triangles = indices;
	meshDesc.triangleStrideBytes = 3 * sizeof(NxU32);
	NxTriangleMesh* mesh = sdk->createTriangleMesh(meshDesc);
	if(!mesh)
		return nxFail("triangle mesh creation failed");
	printf("triangle_mesh_release stage=mesh\n");

	NxHeldSceneWriteLock held = {
		*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(scene) + 0x0c),
		CreateEventW(0, TRUE, FALSE, 0), CreateEventW(0, TRUE, FALSE, 0) };
	if(!held.link || !held.ready || !held.release)
		return nxFail("scene write-lock fixture setup failed");
	HANDLE thread = CreateThread(0, 0, nxHoldSceneWriteLock, &held, 0, 0);
	if(!thread || WaitForSingleObject(held.ready, 5000) != WAIT_OBJECT_0)
		return nxFail("scene write-lock fixture did not acquire the lock");
	printf("triangle_mesh_release stage=locked\n");
	output.reset();
	const DWORD start = GetTickCount();
	sdk->releaseTriangleMesh(*mesh);
	const DWORD elapsed = GetTickCount() - start;
	const bool returnedWhileLocked = elapsed < 500;
	SetEvent(held.release);
	const bool threadJoined = WaitForSingleObject(thread, 5000) == WAIT_OBJECT_0;
	CloseHandle(thread);
	CloseHandle(held.ready);
	CloseHandle(held.release);
	const unsigned lockedErrors = output.errors;
	const bool rejected = output.errors == 1 && output.code == NXE_INVALID_OPERATION && output.line > 0;
	printf("triangle_mesh_release locked returned_while_locked=%u errors=%u code=%u line=%d file=%s message=%s\n",
		returnedWhileLocked, output.errors, static_cast<unsigned>(output.code), output.line, output.file, output.message);
	if(!threadJoined || !returnedWhileLocked || !rejected)
		return nxFail("releaseTriangleMesh did not reject a contended scene lock");

	output.reset();
	sdk->releaseTriangleMesh(*mesh);
	const bool releasedAfterUnlock = output.errors == 0;
	printf("triangle_mesh_release unlocked errors=%u released=%u\n", output.errors, releasedAfterUnlock);
	sdk->releaseScene(*scene);
	sdk->release();
	status = nxReportPairIdentity(pairDirectory);
	if(!releasedAfterUnlock || lockedErrors != 1)
		status = 1;
	FreeLibrary(physics);
	return status;
	}
