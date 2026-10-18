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

NpImplicitMesh::~NpImplicitMesh()
	{
	}

static bool nxNpImplicitMeshWriteBegin(void* link, unsigned lockLine,
	unsigned reentryLine)
	{
	if(!nxNpSceneGuardWriteTry(link))
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPIMPLICITMESH_CPP, lockLine, 0,
			"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!");
		return false;
		}
	if(nxApiReentryIsActive())
		{
		NxFoundation::FoundationSDK::getInstance().error(NXE_INVALID_OPERATION,
			NX_NPIMPLICITMESH_CPP, reentryLine, 0,
			"Reentry check: You may not call this API method from a callback!");
		nxNpSceneGuardLeave(link);
		return false;
		}
	nxSetApiReentryActive(true);
	return true;
	}

static void nxNpImplicitMeshWriteEnd(void* link)
	{
	nxSetApiReentryActive(false);
	nxNpSceneGuardLeave(link);
	}

void NpImplicitMesh::setParticles(NxParticleData&) const
	{
	void* const link = mWriteLockLink;
	if(!nxNpImplicitMeshWriteBegin(link, 12, 13))
		return;
	nxNpImplicitMeshWriteEnd(link);
	}

NxMeshData NpImplicitMesh::getMeshData()
	{
	nxNpSceneGuardEnter(mReadLockLink);
	NxMeshData result;
	NpImplicitMeshDataRecord* const record =
		static_cast<NpImplicitMeshDataRecord*>(mInternalRecord);
	memcpy(&result, record->getMeshData(), sizeof(result));
	nxNpSceneGuardLeave(mReadLockLink);
	return result;
	}

// phys_fn_003661 (0x0008ac40): return the embedded NxMeshData record.
NxMeshData* NpImplicitMeshDataRecord::getMeshData()
	{
	return &mMeshData;
	}

NxReal NpImplicitMesh::getSurfaceRadius() const
	{
	nxNpSceneGuardEnter(mReadLockLink);
	const NxReal result = nxImplicitMeshGetSurfaceRadius(mInternalRecord);
	nxNpSceneGuardLeave(mReadLockLink);
	return result;
	}

void NpImplicitMesh::setSurfaceRadius(NxReal value)
	{
	void* const link = mWriteLockLink;
	if(!nxNpImplicitMeshWriteBegin(link, 38, 39))
		return;
	nxImplicitMeshSetSurfaceRadius(mInternalRecord, value);
	nxNpImplicitMeshWriteEnd(link);
	}

NxReal NpImplicitMesh::getSingleRadius() const
	{
	nxNpSceneGuardEnter(mReadLockLink);
	const NxReal result = nxImplicitMeshGetSingleRadius(mInternalRecord);
	nxNpSceneGuardLeave(mReadLockLink);
	return result;
	}

void NpImplicitMesh::setSingleRadius(NxReal value)
	{
	void* const link = mWriteLockLink;
	if(!nxNpImplicitMeshWriteBegin(link, 51, 52))
		return;
	nxImplicitMeshSetSingleRadius(mInternalRecord, value);
	nxNpImplicitMeshWriteEnd(link);
	}

NxReal NpImplicitMesh::getBlend() const
	{
	NxFoundation::FoundationSDK::getInstance().error(NXE_DB_WARNING,
		NX_NPIMPLICITMESH_CPP, 58, 0,
		"NxImplicitMesh::getBlend(): Feature not available!");
	return 0.0f;
	}

void NpImplicitMesh::setBlend(NxReal value)
	{
	NxFoundation::FoundationSDK::getInstance().error(NXE_DB_WARNING,
		NX_NPIMPLICITMESH_CPP, 65, 0,
		"NxImplicitMesh::setBlend(): Feature not available!");
	void* const link = mWriteLockLink;
	if(!nxNpImplicitMeshWriteBegin(link, 66, 67))
		return;
	nxImplicitMeshSetBlend(mInternalRecord, value);
	nxNpImplicitMeshWriteEnd(link);
	}

NxReal NpImplicitMesh::getTriangleSize() const
	{
	nxNpSceneGuardEnter(mReadLockLink);
	const NxReal result = nxImplicitMeshGetTriangleSize(mInternalRecord);
	nxNpSceneGuardLeave(mReadLockLink);
	return result;
	}

void NpImplicitMesh::setTriangleSize(NxReal value)
	{
	void* const link = mWriteLockLink;
	if(!nxNpImplicitMeshWriteBegin(link, 79, 80))
		return;
	nxImplicitMeshSetTriangleSize(mInternalRecord, value);
	nxNpImplicitMeshWriteEnd(link);
	}

NxU32 NpImplicitMesh::getSmoothingIterations() const
	{
	nxNpSceneGuardEnter(mReadLockLink);
	const NxU32 result = nxImplicitMeshGetSmoothingIterations(mInternalRecord);
	nxNpSceneGuardLeave(mReadLockLink);
	return result;
	}

void NpImplicitMesh::setSmoothingIterations(NxU32 value)
	{
	void* const link = mWriteLockLink;
	if(!nxNpImplicitMeshWriteBegin(link, 92, 93))
		return;
	nxImplicitMeshSetSmoothingIterations(mInternalRecord, value);
	nxNpImplicitMeshWriteEnd(link);
	}

void NpImplicitMesh::setName(const char* name)
	{
	void* const link = mWriteLockLink;
	if(!nxNpImplicitMeshWriteBegin(link, 103, 104))
		return;
	nxImplicitMeshSetName(mInternalRecord, name);
	nxNpImplicitMeshWriteEnd(link);
	}

const char* NpImplicitMesh::getName() const
	{
	nxNpSceneGuardEnter(mReadLockLink);
	const char* const result = nxImplicitMeshGetName(mInternalRecord);
	nxNpSceneGuardLeave(mReadLockLink);
	return result;
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
