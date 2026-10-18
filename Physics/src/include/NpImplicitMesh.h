#ifndef NX_PHYSICS_NPIMPLICITMESH
#define NX_PHYSICS_NPIMPLICITMESH

#include "fluids/NxImplicitMesh.h"
#include "fluids/NxMeshData.h"

// Secondary +0x08 base in the public NpImplicitMesh wrapper. Its vptr is
// followed by the write and read lock links at +0x0c and +0x10.
class NpImplicitMeshLockBase
	{
	public:
	NpImplicitMeshLockBase() : mWriteLockLink(0), mReadLockLink(0) {}
	virtual ~NpImplicitMeshLockBase() {}
	protected:
	void* mWriteLockLink;
	void* mReadLockLink;
	};

// The private record is passed to the internal phys_fn_003663 validator. Its
// first two words precede the 0x30-byte NxMeshData copied at record+0x08.
class NpImplicitMeshDataRecord
	{
	public:
	NxMeshData* getMeshData();
	int setMeshData(NxMeshData& data);
	private:
	void* mVtable;
	void* mOwner;
	NxMeshData mMeshData;
	};

extern "C" NxReal __cdecl nxImplicitMeshGetSurfaceRadius(void* record);
extern "C" void __cdecl nxImplicitMeshSetSurfaceRadius(void* record, NxReal value);
extern "C" NxReal __cdecl nxImplicitMeshGetSingleRadius(void* record);
extern "C" void __cdecl nxImplicitMeshSetSingleRadius(void* record, NxReal value);
extern "C" void __cdecl nxImplicitMeshSetBlend(void* record, NxReal value);
extern "C" NxReal __cdecl nxImplicitMeshGetTriangleSize(void* record);
extern "C" void __cdecl nxImplicitMeshSetTriangleSize(void* record, NxReal value);
extern "C" NxU32 __cdecl nxImplicitMeshGetSmoothingIterations(void* record);
extern "C" void __cdecl nxImplicitMeshSetSmoothingIterations(void* record, NxU32 value);
extern "C" void __cdecl nxImplicitMeshSetName(void* record, const char* name);
extern "C" const char* __cdecl nxImplicitMeshGetName(void* record);

// Public wrapper recovered from the table beginning at 0x101167d8. The
// NxImplicitMesh interface occupies +0x00..+0x07, the lock base +0x08..+0x13,
// and the implementation record pointer is at +0x14.
class NpImplicitMesh : public NxImplicitMesh, public NpImplicitMeshLockBase
	{
	public:
	explicit NpImplicitMesh(void* internalRecord);
	~NpImplicitMesh();
	void setParticles(NxParticleData&) const override;
	void setMeshData(NxMeshData& data) override;
	NxMeshData getMeshData() override;
	NxReal getSurfaceRadius() const override;
	void setSurfaceRadius(NxReal) override;
	NxReal getSingleRadius() const override;
	void setSingleRadius(NxReal) override;
	NxReal getBlend() const override;
	void setBlend(NxReal) override;
	NxReal getTriangleSize() const override;
	void setTriangleSize(NxReal) override;
	NxU32 getSmoothingIterations() const override;
	void setSmoothingIterations(NxU32) override;
	void setName(const char*) override;
	const char* getName() const override;
	protected:
	void* mInternalRecord;
	};

static_assert(sizeof(NxMeshData) == 0x30, "NxMeshData is twelve words");
static_assert(sizeof(NpImplicitMeshDataRecord) == 0x38,
	"phys_fn_003663 copies to the record member at +0x08");
static_assert(sizeof(NpImplicitMeshLockBase) == 0x0c,
	"the lock base contains a vptr and two lock links");
static_assert(sizeof(NpImplicitMesh) == 0x18,
	"the public wrapper stores its internal record pointer at +0x14");

#endif
