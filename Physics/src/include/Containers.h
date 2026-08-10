#ifndef NX_PHYSICS_CONTAINERS
#define NX_PHYSICS_CONTAINERS
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "PhysicsInternal.h"

/**
A growable array of four byte entries over a sixteen byte header, allocated
through the SDK allocator rather than through nxFoundationSDKAllocator.

This is NOT the array the SDK singleton uses. NxArraySDK<T> is
{first, last, memEnd, allocator}, entirely inline in the pinned public NxArray.h,
and grows by (1 + size()) * 2; this one is {capacity, count, entries,
growthFactor} with a float factor and grows by capacity * factor. Two container
types coexist in the image and nothing in the SDK core touches this one --
phys_fn_000482, phys_fn_000484 and phys_fn_000486 each call exactly one row, the
material copy, and none of them calls phys_fn_004840.

Layout, from phys_fn_004836 (0x000b4d70), which clears three words and stores
0x40000000 -- 2.0f -- in the fourth:
    +0x00 capacity      +0x04 count      +0x08 entries      +0x0c growthFactor

A negative growth factor means the buffer is not owned: phys_fn_004846 and
phys_fn_004847 both skip the free when it is negative, and phys_fn_004847 is what
writes -1.0f.

None of these four rows is reachable from any Phase 2 public entry point, so they
are gated by NxPhysicsInternalTests, a static-proof gate, and their behaviour has
never been compared against the oracle dynamically.
*/
class SdkContainer
	{
	public:
	// phys_fn_004836 (0x000b4d70)
	SdkContainer();
	// phys_fn_004840 (0x000b4de0)
	bool resize(NxU32 needed);
	// phys_fn_004846 (0x000b4f50)
	void empty();
	// phys_fn_004847 (0x000b4f90)
	void setExternalBuffer(NxU32 capacity, NxU32* entries);

	NxU32	mCapacity;
	NxU32	mCount;
	NxU32*	mEntries;
	NxF32	mGrowthFactor;
	};

#endif
