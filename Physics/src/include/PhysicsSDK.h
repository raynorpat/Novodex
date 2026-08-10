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
