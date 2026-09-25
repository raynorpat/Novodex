/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
#include "Nxp.h"
#include "PhysicsInternal.h"

// Scaffold only (Phase 6 Task 5). These rows are not Joint or RevoluteJoint
// members: phys_fn_004389/004391/004393 operate on a separate record (flags
// word at +0xc, two body pointers at +0x10/+0x14, vectors at +0x00..+0x2c,
// outputs at +0x3c/+0x40 -- see revolute-contract.md "## Row assignment",
// the 004389/004393 decision notes). phys_fn_000022/000571/000633/000758
// are deferred stubs only: each is owned by a unit outside the pilot (the
// gaps and Scene.cpp named in "## Dependency closure" > defer), declared
// here only so a pilot row can call through a named function instead of a
// raw address. No header is published for these yet because nothing calls
// them until a later task does; that task adds the header it needs, per
// its own file scope, and records any signature change in the contract.

// phys_fn_004389 (0x000af2d0, 227 B)
// (unimplemented)
NxReal row004389(void* record)
	{
	(void)record;
	NX_ASSERT(0);
	return 0.0f;
	}

// phys_fn_004391 (0x000af3c0, 837 B; deferred: solver slots 6/7)
// (unimplemented)
void row004391(void* record)
	{
	(void)record;
	NX_ASSERT(0);
	}

// phys_fn_004393 (0x000af710, 122 B)
// (unimplemented)
void row004393(void* record)
	{
	(void)record;
	NX_ASSERT(0);
	}

// phys_fn_000022 (0x00001840, 27 B; deferred: owner gap <start>..Actor.cpp)
// (unimplemented)
void row000022()
	{
	NX_ASSERT(0);
	}

// phys_fn_000571 (0x000108e0, 22 B; deferred: owner Scene.cpp, link-insert
// at Scene+0x620)
// (unimplemented)
void row000571(void* scene, void* joint)
	{
	(void)scene;
	(void)joint;
	NX_ASSERT(0);
	}

// phys_fn_000633 (0x00012660, 370 B; deferred: owner Scene.cpp, joint
// removal)
// (unimplemented)
void row000633(void* joint)
	{
	(void)joint;
	NX_ASSERT(0);
	}

// phys_fn_000758 (0x00017630, 214 B; deferred: owner gap
// SceneRaycast..CapsuleShape)
// (unimplemented)
void row000758()
	{
	NX_ASSERT(0);
	}
