#include "NpImplicitMesh.h"
#include "NxApiReentry.h"

#include "FoundationSDK.h"
#include "NpSceneGuard.h"

#include <string.h>

#define NX_NPIMPLICITMESH_CPP \
	"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\NpImplicitMesh.cpp"
#define NX_IMPLICITMESH_CPP \
	"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\ImplicitMesh.cpp"

NpImplicitMesh::NpImplicitMesh(void* internalRecord)
	: mInternalRecord(internalRecord)
	{
	}

// phys_fn_003663 (0x0008ac50): internal descriptor validator/copy helper.
// This is called by the public lock-taking wrapper, not by the public vtable
// entry itself. The embedded descriptor starts at record+0x08.
int NpImplicitMeshDataRecord::setMeshData(NxMeshData& data)
	{
	if(data.isValid())
		{
		memcpy(&mMeshData, &data, sizeof(data));
		return data.indicesBegin
			? reinterpret_cast<int>(data.indicesBegin)
			: data.indicesByteStride;
		}

	return static_cast<int>(NxFoundation::FoundationSDK::getInstance().error(
		NXE_INVALID_PARAMETER, NX_IMPLICITMESH_CPP, 106, 0,
		"Supplied NxMeshData is not valid. setMeshData has no effect."));
	}

// phys_fn_003663's public call path is NpImplicitMesh::setMeshData at
// 0x0008d230. It takes the write link from +0x0c, checks the callback
// re-entry guard, then dispatches the internal record at +0x14.
void NpImplicitMesh::setMeshData(NxMeshData& data)
	{
	void* link = mWriteLockLink;
	if(!nxNpSceneGuardWriteTry(link))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPIMPLICITMESH_CPP, 25, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return;
		}
	if(nxApiReentryIsActive())
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPIMPLICITMESH_CPP, 26, 0,
			"Reentry check: You may not call this API method from a callback!");
		nxNpSceneGuardLeave(link);
		return;
		}
	nxSetApiReentryActive(true);
	static_cast<NpImplicitMeshDataRecord*>(mInternalRecord)->setMeshData(data);
	nxSetApiReentryActive(false);
	nxNpSceneGuardLeave(link);
	}
