/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

/*
NovodeX's scene-query pruners: the static pruner (type 0, 0x90 bytes, table
.rdata:0x0011b9c8), the bounded dynamic pruner (type 1, 0x40 bytes, table
.rdata:0x0011b9f0), the unbounded dynamic pruner (type 2, 0x3c bytes, table
.rdata:0x0011bbc0), and the Scene's pruning engine that owns one pruner per
type (the object at Scene+0x624; the pruners are its words +0x1c..+0x28).

Like IcePrunable.h this is NovodeX's own code, not OPCODE: neither pinned
OPCODE 1.3 tree has a pruner. It uses OPCODE (the static pruner builds an
AABBTree over its world boxes and stabs it with a RayCollider).

Written by the scene-raycast block, Task 3, for the scene raycasts: the
queries walk the engine's pruners through slot 6 (phys_fn_004864), and the
pool the pruners keep is what the Scene's shape registration fills. Only the
rows the raycasts and the registration reach are here; see IcePruner.cpp (the
pool and the pruners) and IcePruningEngine.cpp (the engine) for the row map and
for what is not reconstructed.
*/

#ifndef NX_PHYSICS_ICEPRUNER_H
#define NX_PHYSICS_ICEPRUNER_H

#include "IcePrunable.h"

using namespace Opcode;

class StaticPruner : public Pruner
{
	public:
								StaticPruner();
	virtual						~StaticPruner();
	virtual	bool				AddObject(Prunable* object);
	virtual	bool				RemoveObject(Prunable* object);
	virtual	bool				UpdateObject(Prunable* object);
	virtual	void				SetExternalBuffer(udword max_nb, udword* entries);
	virtual	bool				Raycast(Container& objects, const Ray& world_ray, float max_dist,
									bool first_contact, udword mask);

			bool				BuildTree();
			void				ReportTouched(Container& objects, udword mask);
			bool				OverlapAABB(Container& objects, const Point& min, const Point& max,
								udword mask);

			AABBTree*			mTree;				//!< +0x3c
			Container			mTouched;			//!< +0x40
			// +0x50..+0x8f: two query caches the constructor points at mTouched
			// (+0x50 and +0x6c) and seeds with 1.1f (+0x68 and +0x8c). Nothing
			// the raycasts reach reads them; named for their offsets.
			udword				mCache50[7];		//!< +0x50..+0x68
			udword				mCache6C[9];		//!< +0x6c..+0x8c
};

class DynamicPruner : public Pruner
{
	public:
								DynamicPruner();
	virtual						~DynamicPruner();
	virtual	bool				Raycast(Container& objects, const Ray& world_ray, float max_dist,
									bool first_contact, udword mask);
};

// phys_fn_004852 creates the bounded dynamic pruner for type 1 (0x40 bytes).
// Its tree-backed query overrides are still reconstructed separately; scene
// registration uses the inherited pool operations while this tree is empty.
class BoundedDynamicPruner : public DynamicPruner
{
	public:
								BoundedDynamicPruner();
								~BoundedDynamicPruner();

		void*					mTree;				//!< +0x3c
};

static_assert(sizeof(BoundedDynamicPruner) == 0x40,
	"bounded dynamic pruner type 1 is a 0x40-byte object");

// The engine's two entry points the Scene's shape registration uses, and its
// pruner factory for the two pruning types shapes use.
bool		nxPruningEngineAddObject(void* engine, Prunable* object);
bool		nxPruningEngineRemoveObject(void* engine, Prunable* object);
Pruner*		nxPruningEngineCreatePruner(udword type);
// Destroys a pruner the engine created (its destructor, then its storage).
void		nxPruningEngineDestroyPruner(Pruner* pruner);
// Release the scene-owned coherent broadphase cache before pool mutation or
// engine destruction.
void		nxSceneEngineReleaseCoherent(void* engine);

// Ray and segment against an AABB, the separating-axis tests the dynamic
// pruner's raycast uses.
bool		nxRayAABB(const Ray& ray, const Point& min, const Point& max);
bool		nxSegmentAABB(const Segment& segment, const Point& min, const Point& max);

#endif // NX_PHYSICS_ICEPRUNER_H
