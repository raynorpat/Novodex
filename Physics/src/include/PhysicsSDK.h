#ifndef NX_PHYSICS_PHYSICSSDK
#define NX_PHYSICS_PHYSICSSDK
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "PhysicsInternal.h"
#include "NxArray.h"
#include "NxMaterial.h"
#include "NxPhysicsSDK.h"

class NpPhysicsSDK;
class NxDebugRenderable;
class NxUserDebugRenderer;

/**
The SDK-side singleton. The public NxPhysicsSDK the user is handed is the
NpPhysicsSDK this object owns; this class holds no user-visible interface of its
own and has exactly one virtual, the destructor, matching the single entry
vtable at .rdata 0x00106384.

Layout is measured from phys_fn_000472 (the constructor, 0x0000e1b0) and its
callers: 0x0000fb44 allocates 0x38 bytes, 0x0000fb59 reads the wrapper at +4,
and the three arrays are the (first,last,memEnd,allocator) quadruples the
constructor clears at +8/+0xc/+0x10, +0x18/+0x1c/+0x20 and +0x28/+0x2c/+0x30.
*/
class PhysicsSDK : public NxAllocateable
	{
	public:
	PhysicsSDK();
	virtual ~PhysicsSDK();

	// phys_fn_000425 (0x0000db30)
	void release();
	// phys_fn_000427 (0x0000db50)
	bool setParameter(NxParameter paramEnum, NxReal paramValue);
	// phys_fn_000429 (0x0000dc00)
	NxReal getParameter(NxParameter paramEnum) const;
	// phys_fn_000448 (0x0000def0)
	NxU32 getNbScenes() const;
	// phys_fn_000450 (0x0000df00)
	Scene* getScene(NxU32 index);
	// phys_fn_000458 (0x0000e030)
	NxU32 getNbMaterials() const;

	// phys_fn_000452 (0x0000df30)
	void setGroupCollisionFlag(NxCollisionGroup group1, NxCollisionGroup group2, bool enable);
	// phys_fn_000431 (0x0000dc50)
	bool getGroupCollisionFlag(NxCollisionGroup group1, NxCollisionGroup group2) const;
	// phys_fn_000482 (0x0000ef50)
	NxMaterialIndex addMaterial(const NxMaterial& material);
	// phys_fn_000456 (0x0000dfd0). Reads the singleton rather than this object.
	NxMaterial* getMaterial(NxMaterialIndex index);
	// phys_fn_000484 (0x0000f0d0)
	void setMaterialAtIndex(NxMaterialIndex index, const NxMaterial* material);
	// phys_fn_000486 with phys_fn_000488 as its tail (0x0000f690)
	void purgeMaterials();
	// phys_fn_000446 (0x0000dee0)
	bool setPerformanceInspector(NxPerformanceInspector* inspector);

	// The three visualization entry points. None of them reads this object -- the
	// Foundation and the cached renderable are both file scope in PhysicsSDK.cpp
	// -- but all three are entered with ecx holding a PhysicsSDK, so they are
	// members and not free functions: 0x0000bb4b loads the SDK into ecx before
	// calling phys_fn_000439, and every NxFluidDebug* export loads the singleton
	// from .data 0x00123c04 into ecx before calling phys_fn_000443.
	//
	// phys_fn_000439 (0x0000de10)
	void visualize(const NxUserDebugRenderer& renderer);
	// phys_fn_000443 (0x0000deb0)
	NxDebugRenderable* getDebugRenderable();
	// phys_fn_000445 (0x0000ded0). Its one caller, phys_fn_000608 at 0x00011190,
	// is Phase 7, so no Phase 2 differential can reach it.
	void clearDebugRenderable();

	NpPhysicsSDK* getNp() const { return mNp; }

	// .data 0x00123c04
	static PhysicsSDK* instance;

	// SDK-internal state, left accessible so the measured offsets can be asserted.
	NpPhysicsSDK* mNp;
	NxArraySDK<Scene*> mScenes;
	NxArraySDK<TriangleMesh*> mTriangleMeshes;
	NxArraySDK<NxMaterial> mMaterials;
	};

#endif
