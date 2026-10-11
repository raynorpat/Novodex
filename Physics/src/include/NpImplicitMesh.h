#ifndef NX_PHYSICS_NPIMPLICITMESH
#define NX_PHYSICS_NPIMPLICITMESH

#include "fluids/NxImplicitMesh.h"
#include "fluids/NxMeshData.h"

/**
Private implementation surface for the recovered implicit-mesh data row.
The public base occupies the vptr and userData words; its NxMeshData record
begins at +0x08, as observed by phys_fn_003663.
*/
class NpImplicitMesh : public NxImplicitMesh
	{
	public:
	void setMeshData(NxMeshData& data) override;

	private:
	NxMeshData mMeshData;
	};

static_assert(sizeof(NxMeshData) == 0x30, "NxMeshData is twelve words");
static_assert(sizeof(NpImplicitMesh) == 0x38,
	"the implicit mesh stores NxMeshData at object offset +0x08");

#endif
