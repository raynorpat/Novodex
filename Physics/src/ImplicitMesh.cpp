#include "NpImplicitMesh.h"

#include "FoundationSDK.h"

#include <string.h>

void NpImplicitMesh::setMeshData(NxMeshData& data)
	{
	if(data.isValid())
		{
		memcpy(&mMeshData, &data, sizeof(data));
		return;
		}

	NxFoundation::FoundationSDK::error(NXE_INVALID_PARAMETER,
		"\\Epic\\Novodex\\SDKs\\Physics\\src\\fluids\\ImplicitMesh.cpp",
		106, 0, "Supplied NxMeshData is not valid. setMeshData has no effect.");
	}
