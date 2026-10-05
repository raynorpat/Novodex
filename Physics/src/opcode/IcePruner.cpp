/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/

/*
NovodeX's pruners and the pruning engine's add and remove (scene-raycast block,
Task 3). See IcePruner.h. The rows, all from the Capstone listing:

	pool (the object at every pruner's +0x04)
		0x000efeb0	PruningPool::PruningPool (already recorded as phys_fn_005475)
		0x000effc0	PruningPool::Resize
		0x000f00c0	PruningPool::AddObject
		0x000f02a0	PruningPool::RemoveObject
	base pruner (table 0x0011bc18)
		0x000f1550	Pruner::Pruner                     not claimed: see below
		0x000e50c0	slot 1, AddObject
		0x000e50d0	slot 2, RemoveObject
		0x000e50f0	slot 3, UpdateObject (already recorded as phys_fn_005212)
		0x000e5890	slot 4, nothing (already recorded as phys_fn_005242)
		0x000f1580	slots 5 and 6, false; 0x000f1590 slots 7 and 8, false
	static pruner (table 0x0011b9c8, type 0, 0x90 bytes)
		0x000e56d0	StaticPruner::StaticPruner
		0x000e5210	slot 1, 0x000e5250 slot 2, 0x000e52a0 slot 3, 0x000e5100 slot 4
		0x000e5440	slot 6, Raycast
		0x000e5110	BuildTree
		0x000e53e0	ReportTouched
	dynamic pruner (table 0x0011bbc0, type 2, 0x3c bytes)
		0x000ef850	DynamicPruner::DynamicPruner
		0x000efa90	slot 6, Raycast
	the separating-axis tests
		0x000f17c0	nxRayAABB
		0x000f1600	nxSegmentAABB

The engine (Scene+0x624) that owns the pruners, its add and remove and its
factory, are IcePruningEngine.cpp.

NOT RECONSTRUCTED.
  * The base constructor's +0x34 member (0x000b4cc0): the image registers the
    pruner with a process-wide 0x1c-byte object it creates on first use
    (0x000ef270) and keeps the handle at +0x34. The engine's factory creates
    that object through Scene.cpp's existing emulation (nxOpcodeEnsurePool,
    which reproduces its allocations) and the handle is left 0. So 0x000f1550
    is not claimed, and neither are the destructors, whose base (0x000f15a0)
    undoes that registration (0x000b4d20).
  * Slot 5 across the pruner families, plus overlap slots 7/8 on the unbounded
    dynamic pruner. The base defaults remain false. Static and bounded-dynamic
    overlap slots 7/8 are reconstructed below.

PRECISION. Every row here runs at API time under the control word 0x027f
(53-bit precision, round to nearest): the raycasts, and the registration from
createActor/releaseActor. The x87 register lifetimes are written `double`, the
listing's float spills `float`. The file is nevertheless built /arch:IA32 /GR-,
as NxOpcode is (CMakeLists.txt): the tree build instantiates OPCODE's
AABBTreeOfAABBsBuilder, and the linker keeps this object's COMDAT copies of the
builder's inline virtuals and table, which must be OPCODE's code (built SSE2,
AABBTreeBuilder::GetSplittingValue, 002150, rounded its result to float).
*/

#include "IcePruner.h"
#include "OPC_AABBCollider.h"

#include <string.h>
#include <new>
#include <stdlib.h>

// SdkContainer::setExternalBuffer (phys_fn_004847) on a container of this
// layout, through Containers.cpp (Containers.h and OPCODE's headers do not mix).
void nxSdkContainerSetExternalBuffer(void* container, udword capacity, udword* entries);
void nxSdkContainerAppend(void* container, udword entry);

// The layouts the listing addresses (IcePruner.h, IcePrunable.h).
static_assert(sizeof(PruningPool) == 0x18, "PruningPool is 0x18 bytes");
static_assert(sizeof(Pruner) == 0x3c, "the base pruner is 0x3c bytes");
static_assert(sizeof(StaticPruner) == 0x90, "the static pruner is 0x90 bytes (0x000b50e7)");
static_assert(sizeof(DynamicPruner) == 0x3c, "the dynamic pruner is 0x3c bytes (0x000b50ad)");
static_assert(sizeof(Prunable) == 0x2c, "a prunable is 0x2c bytes");

///////////////////////////////////////////////////////////////////////////////
// The pool.

// 0x000efeb0 (phys_fn_005475, recorded elsewhere): every field zero.
PruningPool::PruningPool()
{
	mNbTotal		= 0;
	mMaxNbObjects	= 0;
	mWorldBoxes		= null;
	mObjects		= null;
	mNbObjects[0]	= 0;
	mNbObjects[1]	= 0;
	mNbObjects[2]	= 0;
}

// phys_fn_005481 (0x000effc0, 252 B)
// Grows only when full: the capacity doubles, or becomes 4. Both arrays are
// allocated before either is copied (a failed second allocation returns
// false and keeps the first, as the image does at 0x000f002b), then the old
// ones are freed, boxes first.
__declspec(noinline) bool PruningPool::Resize()
{
	if(mNbTotal == mMaxNbObjects)
	{
		if(mMaxNbObjects)	mMaxNbObjects = uword(mMaxNbObjects + mMaxNbObjects);
		else				mMaxNbObjects = 4;

		AABB* boxes = (AABB*)opcNovodeXAlloc(sizeof(AABB)*mMaxNbObjects);
		if(!boxes)
			return false;
		Prunable** objects = (Prunable**)opcNovodeXAlloc(sizeof(Prunable*)*mMaxNbObjects);
		if(!objects)
			return false;

		if(mWorldBoxes)	memcpy(boxes, mWorldBoxes, mNbTotal*sizeof(AABB));
		if(mObjects)	memcpy(objects, mObjects, mNbTotal*sizeof(Prunable*));
		if(mWorldBoxes)	{ opcNovodeXFree(mWorldBoxes);	mWorldBoxes = null;	}
		if(mObjects)	{ opcNovodeXFree(mObjects);		mObjects = null;	}
		mWorldBoxes	= boxes;
		mObjects	= objects;
	}
	return true;
}

// The one move every section update makes: the box and the object at `from`
// go to `to`, and the moved object's handle becomes `to`.
static inline_ void nxPoolMove(PruningPool& pool, udword from, udword to)
{
	pool.mWorldBoxes[to]		= pool.mWorldBoxes[from];
	pool.mObjects[to]			= pool.mObjects[from];
	pool.mObjects[to]->mHandle	= uword(to);
}

// phys_fn_005483 (0x000f00c0, 476 B)
// Inserts into the object's section (its byte +0x2b) and keeps the three
// sections contiguous: a section-0 insertion moves the first object of section
// 2 to the end and the first of section 1 to the end of section 1; a section-1
// insertion moves the first of section 2 to the end. The new slot's box is
// set empty (0x00053810).
__declspec(noinline) bool PruningPool::AddObject(Prunable* object)
{
	if(!Resize())
		return false;

	const udword section = object->mPruningSection;
	if(section == 0)
	{
		const udword end1 = mNbObjects[1] + mNbObjects[0];
		if(mNbObjects[2])
			nxPoolMove(*this, end1, mNbTotal);
		const udword first1 = mNbObjects[0];
		if(mNbObjects[1])
			nxPoolMove(*this, first1, end1);
		mWorldBoxes[first1].SetEmpty();
		mObjects[first1] = object;
		mObjects[first1]->mHandle = uword(first1);
		mNbObjects[0]++;
		mNbTotal++;
		return true;
	}
	if(section == 1)
	{
		const udword end1 = mNbObjects[1] + mNbObjects[0];
		if(mNbObjects[2])
			nxPoolMove(*this, end1, mNbTotal);
		mWorldBoxes[end1].SetEmpty();
		mObjects[end1] = object;
		mObjects[end1]->mHandle = uword(end1);
		mNbObjects[1]++;
		mNbTotal++;
		return true;
	}
	mWorldBoxes[mNbTotal].SetEmpty();
	mObjects[mNbTotal] = object;
	mObjects[mNbTotal]->mHandle = mNbTotal;
	mNbObjects[2]++;
	mNbTotal++;
	return true;
}

// phys_fn_005485 (0x000f02a0, 622 B)
// The total drops first (0x000f02a0). The removed slot is filled from the last
// object of the object's own section, and each later section's last object
// fills the hole its predecessor left; the removed object's handle becomes
// 0xffff.
__declspec(noinline) void PruningPool::RemoveObject(Prunable* object)
{
	mNbTotal--;
	const udword section = object->mPruningSection;
	if(section == 0)
	{
		const udword last0 = mNbObjects[0] - 1;
		if(last0 != object->mHandle)
		{
			const udword hole = object->mHandle;
			mWorldBoxes[hole]		= mWorldBoxes[last0];
			mObjects[hole]			= mObjects[last0];
			mObjects[last0]->mHandle	= object->mHandle;
		}
		const udword last1 = mNbObjects[1] + mNbObjects[0] - 1;
		if(last1 != last0)
			nxPoolMove(*this, last1, last0);
		const udword last2 = mNbObjects[2] + mNbObjects[1] + mNbObjects[0] - 1;
		if(last2 != last1)
			nxPoolMove(*this, last2, last1);
		mNbObjects[0]--;
		object->mHandle = PRUNABLE_INVALID_HANDLE;
		return;
	}
	if(section == 1)
	{
		const udword last1 = mNbObjects[1] + mNbObjects[0] - 1;
		if(last1 != object->mHandle)
		{
			const udword hole = object->mHandle;
			mWorldBoxes[hole]		= mWorldBoxes[last1];
			mObjects[hole]			= mObjects[last1];
			mObjects[last1]->mHandle	= object->mHandle;
		}
		const udword last2 = mNbObjects[2] + mNbObjects[1] + mNbObjects[0] - 1;
		if(last2 != last1)
			nxPoolMove(*this, last2, last1);
		mNbObjects[1]--;
		object->mHandle = PRUNABLE_INVALID_HANDLE;
		return;
	}
	const udword last2 = mNbObjects[1] + mNbObjects[2] + mNbObjects[0] - 1;
	if(last2 != object->mHandle)
	{
		const udword hole = object->mHandle;
		mWorldBoxes[hole]		= mWorldBoxes[last2];
		mObjects[hole]			= mObjects[last2];
		mObjects[last2]->mHandle	= object->mHandle;
	}
	mNbObjects[2]--;
	object->mHandle = PRUNABLE_INVALID_HANDLE;
}

///////////////////////////////////////////////////////////////////////////////
// The base pruner.

// 0x000f1550, not claimed (see the file comment): the pool, the table, the
// +0x34 member (0x000b4cc0: its registration is emulated by the engine's
// factory, IcePruningEngine.cpp, and the handle left 0) and the empty bounds
// (0x00053810).
Pruner::Pruner()
{
	mTimestamp	= 0;
	mPruner34	= 0;
	mBounds.SetEmpty();
}

// The pool's arrays, boxes first (0x000efed0), as the base destructor frees
// them; the +0x34 unregistration is not reproduced.
Pruner::~Pruner()
{
	if(mPool.mWorldBoxes)	{ opcNovodeXFree(mPool.mWorldBoxes);	mPool.mWorldBoxes = null;	}
	if(mPool.mObjects)		{ opcNovodeXFree(mPool.mObjects);		mPool.mObjects = null;		}
}

// phys_fn_005208 (0x000e50c0, 8 B)
bool Pruner::AddObject(Prunable* object)
{
	return mPool.AddObject(object);
}

// phys_fn_005210 (0x000e50d0, 21 B)
bool Pruner::RemoveObject(Prunable* object)
{
	mTimestamp++;
	mPool.RemoveObject(object);
	return true;
}

// 0x000e50f0 (phys_fn_005212, recorded elsewhere).
bool Pruner::UpdateObject(Prunable* /*object*/)
{
	mTimestamp++;
	return true;
}

// 0x000e5890 (phys_fn_005242, recorded elsewhere).
void Pruner::SetExternalBuffer(udword /*max_nb*/, udword* /*entries*/)
{
}

// 0x000f1580 (slots 5 and 6) and 0x000f1590 (slots 7 and 8).
bool Pruner::NovodeXPrunerSlot5(udword, udword, udword, udword, udword)
{
	return false;
}

bool Pruner::Raycast(Container& /*objects*/, const Ray& /*world_ray*/, float /*max_dist*/,
	bool /*first_contact*/, udword /*mask*/)
{
	return false;
}

bool Pruner::NovodeXPrunerSlot7(udword, udword, udword, udword)
{
	return false;
}

bool Pruner::NovodeXPrunerSlot8(udword, udword, udword, udword)
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// The static pruner.

// phys_fn_005232 (0x000e56d0, 108 B)
// The base, the table, no tree, the touched container (0x000b4d70), and the
// two caches: zeroed, 1.1f at +0x68 and +0x8c, and both pointed at the
// touched container last (0x000e5731, 0x000e5734).
StaticPruner::StaticPruner()
{
	mTree = null;
	for(udword i = 0; i < 6; i++)
		mCache50[i] = 0;
	const float fat = 1.1f;
	memcpy(&mCache50[6], &fat, 4);		// +0x68
	mCache6C[0] = 0;					// +0x6c
	mCache6C[1] = 0;					// +0x70
	memcpy(&mCache6C[8], &fat, 4);		// +0x8c
	mCache6C[4] = 0;					// +0x7c
	mCache6C[3] = 0;					// +0x78
	mCache6C[2] = 0;					// +0x74
	mCache6C[7] = 0;					// +0x88
	mCache6C[6] = 0;					// +0x84
	mCache6C[5] = 0;					// +0x80
	mCache50[0] = udword(&mTouched);	// +0x50
	mCache6C[0] = udword(&mTouched);	// +0x6c
}

// 0x000e5810 (not claimed): the tree, the touched container's storage, then
// the base.
StaticPruner::~StaticPruner()
{
	if(mTree)
	{
		delete mTree;
		mTree = null;
	}
	mTouched.Empty();
}

// phys_fn_005218 (0x000e5210, 56 B)
bool StaticPruner::AddObject(Prunable* object)
{
	if(mTree)
	{
		delete mTree;
		mTree = null;
	}
	return mPool.AddObject(object);
}

// phys_fn_005220 (0x000e5250, 65 B)
bool StaticPruner::RemoveObject(Prunable* object)
{
	if(mTree)
	{
		delete mTree;
		mTree = null;
	}
	mTimestamp++;
	mPool.RemoveObject(object);
	return true;
}

// phys_fn_005222 (0x000e52a0, 48 B)
bool StaticPruner::UpdateObject(Prunable* /*object*/)
{
	if(mTree)
	{
		delete mTree;
		mTree = null;
	}
	mTimestamp++;
	return true;
}

// phys_fn_005214 (0x000e5100, 8 B)
// The touched container borrows the buffer (0x000b4f90, phys_fn_004847).
void StaticPruner::SetExternalBuffer(udword max_nb, udword* entries)
{
	nxSdkContainerSetExternalBuffer(&mTouched, max_nb, entries);
}

// phys_fn_005216 (0x000e5110, 250 B)
// Rebuilds the tree over sections 0 and 1. Every object whose box is stale
// gets it recomputed first (phys_fn_004886); the builder is an
// AABBTreeOfAABBsBuilder on the stack over the pool's boxes, one primitive a
// leaf, rules SPLIT_SPLATTER_POINTS (0x000e51bd-0x000e51f5).
__declspec(noinline) bool StaticPruner::BuildTree()
{
	if(mTree)
	{
		delete mTree;
		mTree = null;
	}
	const udword nb = mPool.mNbObjects[1] + mPool.mNbObjects[0];
	if(!nb)
		return true;

	mTree = new AABBTree;
	if(!mTree)
		return false;

	Prunable** objects = mPool.mObjects;
	for(udword i = 0; i < nb; i++)
	{
		Prunable* current = objects[i];
		if(current->mHandle != PRUNABLE_INVALID_HANDLE && !(current->mFlags & PRUNABLE_FLAG_WORLD_AABB_VALID))
			current->UpdateWorldAABB(&mPool.mWorldBoxes[current->mHandle]);
	}

	AABBTreeOfAABBsBuilder builder;
	builder.mAABBArray				= mPool.mWorldBoxes;
	builder.mSettings.mLimit		= 1;
	builder.mSettings.mRules		= SPLIT_SPLATTER_POINTS;
	builder.mNbPrimitives			= nb;
	return mTree->Build(&builder);
}

// phys_fn_005225 (0x000e53e0, 94 B)
// The touched primitives are pool indices; every object whose mask (+0x24)
// meets the query's goes into the caller's container.
__declspec(noinline) void StaticPruner::ReportTouched(Container& objects, udword mask)
{
	udword nb = mTouched.GetNbEntries();
	if(!nb)
		return;
	const udword* touched = mTouched.GetEntries();
	Prunable** pool = mPool.mObjects;
	do
	{
		Prunable* current = pool[*touched++];
		if(mask & current->mPrunable24)
			objects.Add(udword(current));
	}
	while(--nb);
}

// phys_fn_005229 (0x000e5500, 133 B). The static AABB query uses the
// pruner's AABBCache at +0x6c and reports the AABB-tree traversal order.
bool StaticPruner::OverlapAABB(Container& objects, const Point& min, const Point& max,
	udword mask)
{
	if(!mTree)
	{
		BuildTree();
		if(!mTree)
			return false;
	}

	mTouched.Reset();
	CollisionAABB query;
	query.SetMinMax(min, max);
	AABBCollider collider;
	AABBCache& cache = *reinterpret_cast<AABBCache*>(mCache6C);
	if(!collider.Collide(cache, query, mTree))
		return false;

	ReportTouched(objects, mask);
	return true;
}

// phys_fn_005229 (0x000e5500), the static-pruner slot 8 ABI adapter.
bool StaticPruner::NovodeXPrunerSlot8(udword result, udword queryBounds,
	udword /*first_contact*/, udword mask)
{
	const float* const values = reinterpret_cast<const float*>(queryBounds);
	const Point min(values[0], values[1], values[2]);
	const Point max(values[3], values[4], values[5]);
	Container touched;
	if(!OverlapAABB(touched, min, max, mask))
		return false;
	const udword* entries = touched.GetEntries();
	for(udword i = 0; i < touched.GetNbEntries(); ++i)
		nxSdkContainerAppend(reinterpret_cast<void*>(result), entries[i]);
	return true;
}

// Bounded-shape AABB collection is slot 7 in the pruning-engine loop.
bool StaticPruner::NovodeXPrunerSlot7(udword result, udword queryBounds,
	udword first_contact, udword mask)
{
	return StaticPruner::NovodeXPrunerSlot8(result, queryBounds, first_contact, mask);
}

// phys_fn_005227 (0x000e5440, 184 B)
// The tree is built on the first query after any change. A RayCollider on the
// stack stabs it, first-contact as asked, temporal coherence off, the maximum
// distance as given; the touched container is reset first (0x000e5468).
bool StaticPruner::Raycast(Container& objects, const Ray& world_ray, float max_dist,
	bool first_contact, udword mask)
{
	if(!mTree)
	{
		BuildTree();
		if(!mTree)
			return false;
	}
	mTouched.Reset();

	RayCollider collider;
	collider.SetFirstContact(first_contact);
	collider.SetTemporalCoherence(false);
	collider.SetMaxDist(max_dist);
	collider.Collide(world_ray, mTree, mTouched);

	ReportTouched(objects, mask);
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// The dynamic pruner.

// phys_fn_005464 (0x000ef850, 18 B)
DynamicPruner::DynamicPruner()
{
}

// phys_fn_005240 (0x000e5870): Pruner base, type-1 vtable, null tree.
BoundedDynamicPruner::BoundedDynamicPruner()
	: DynamicPruner(), mTree(0)
{
}

BoundedDynamicPruner::~BoundedDynamicPruner()
{
}

struct BoundedTreeQuery
{
	BoundedDynamicPruner* pruner;
	Container* objects;
	const Point* queryMin;
	const Point* queryMax;
	udword mask;
	Container* partialObjects;
	Container* containedObjects;
	udword* nodeHeads;
	udword* objectNext;
	udword* subtreeCounts;
	udword nodeCount;
};

static bool nxAABBOverlaps(const Point& min0, const Point& max0,
	const Point& min1, const Point& max1)
{
	return min0.x <= max1.x && max0.x >= min1.x
		&& min0.y <= max1.y && max0.y >= min1.y
		&& min0.z <= max1.z && max0.z >= min1.z;
}

static bool nxAABBContains(const Point& outerMin, const Point& outerMax,
	const Point& innerMin, const Point& innerMax)
{
	return outerMin.x <= innerMin.x && outerMax.x >= innerMax.x
		&& outerMin.y <= innerMin.y && outerMax.y >= innerMax.y
		&& outerMin.z <= innerMin.z && outerMax.z >= innerMax.z;
}

static void nxBoundedTreeNodeBounds(udword node, const Point& rootMin,
	const Point& rootMax, Point& nodeMin, Point& nodeMax)
{
	udword depth = 0;
	udword firstAtDepth = 0;
	udword levelCount = 1;
	while(depth < 5 && node >= firstAtDepth + levelCount)
		{
		firstAtDepth += levelCount;
		levelCount *= 8;
		++depth;
		}
	const udword path = node - firstAtDepth;
	Point center((rootMin.x + rootMax.x) * 0.5f,
		(rootMin.y + rootMax.y) * 0.5f, (rootMin.z + rootMax.z) * 0.5f);
	float side = rootMax.x - rootMin.x;
	for(udword level = 0; level < depth; ++level)
		{
		const udword shift = 3 * (depth - level - 1);
		const udword child = ((path >> shift) & 7u) + 1u;
		const udword octant = child - 1u;
		const float offset = side * 0.125f;
		center.x += (octant & 4u) ? offset : -offset;
		center.y += (octant & 2u) ? offset : -offset;
		center.z += (octant & 1u) ? offset : -offset;
		side *= 0.5f;
		}
	const float half = side * 0.5f;
	nodeMin = Point(center.x - half, center.y - half, center.z - half);
	nodeMax = Point(center.x + half, center.y + half, center.z + half);
}

static void nxBoundedTreeChildBounds(const Point& nodeMin, const Point& nodeMax,
	udword child, Point& childMin, Point& childMax)
{
	const Point center((nodeMin.x + nodeMax.x) * 0.5f,
		(nodeMin.y + nodeMax.y) * 0.5f, (nodeMin.z + nodeMax.z) * 0.5f);
	const float side = nodeMax.x - nodeMin.x;
	const float offset = side * 0.125f;
	const float half = side * 0.25f;
	const udword octant = child - 1u;
	const float childCenterX = center.x + ((octant & 4u) ? offset : -offset);
	const float childCenterY = center.y + ((octant & 2u) ? offset : -offset);
	const float childCenterZ = center.z + ((octant & 1u) ? offset : -offset);
	childMin = Point(childCenterX - half, childCenterY - half, childCenterZ - half);
	childMax = Point(childCenterX + half, childCenterY + half, childCenterZ + half);
}

static void nxBoundedTreeQueryNode(BoundedTreeQuery& query, udword node,
	const Point& nodeMin, const Point& nodeMax)
{
	if(!query.subtreeCounts[node]
		|| !nxAABBOverlaps(nodeMin, nodeMax, *query.queryMin, *query.queryMax))
		return;
	if(nxAABBContains(*query.queryMin, *query.queryMax, nodeMin, nodeMax))
		{
		// The oracle switches to a subtree collection path once a node is fully
		// inside the query. That stream is emitted after all partially crossed
		// nodes, so keep it separate from the boundary-cell results.
		for(udword i = query.nodeHeads[node]; i != 0xffffffffu; i = query.objectNext[i])
			{
			Prunable* const object = query.pruner->mPool.mObjects[i];
			if((query.mask & object->mPrunable24) && object->mHandle != PRUNABLE_INVALID_HANDLE)
				query.containedObjects->Add(udword(object));
			}
		for(udword child = 1; child <= 8; ++child)
			{
			const udword childNode = node * 8 + child;
			if(childNode >= query.nodeCount || !query.subtreeCounts[childNode])
				continue;
			Point childMin, childMax;
			nxBoundedTreeChildBounds(nodeMin, nodeMax, child, childMin, childMax);
			nxBoundedTreeQueryNode(query, childNode, childMin, childMax);
			}
		return;
		}
	for(udword i = query.nodeHeads[node]; i != 0xffffffffu; i = query.objectNext[i])
		{
		Prunable* const object = query.pruner->mPool.mObjects[i];
		if(!(query.mask & object->mPrunable24) || object->mHandle == PRUNABLE_INVALID_HANDLE)
			continue;
		if(!(object->mFlags & PRUNABLE_FLAG_WORLD_AABB_VALID))
			object->UpdateWorldAABB(&query.pruner->mPool.mWorldBoxes[object->mHandle]);
		Point objectMin, objectMax;
		query.pruner->mPool.mWorldBoxes[object->mHandle].GetMin(objectMin);
		query.pruner->mPool.mWorldBoxes[object->mHandle].GetMax(objectMax);
		if(nxAABBOverlaps(objectMin, objectMax, *query.queryMin, *query.queryMax))
			query.partialObjects->Add(udword(object));
		}
	for(udword child = 1; child <= 8; ++child)
		{
		const udword childNode = node * 8 + child;
		if(childNode >= query.nodeCount || !query.subtreeCounts[childNode])
			continue;
		Point childMin, childMax;
		nxBoundedTreeChildBounds(nodeMin, nodeMax, child, childMin, childMax);
		nxBoundedTreeQueryNode(query, childNode, childMin, childMax);
		}
}

// phys_fn_005258/005260 (0x000e6060/0x000e6130), the bounded-pruner AABB
// collector. Rebuild the fixed-depth tree from the current pool, assigning
// each object to its recovered center/radius cell, then follow the oracle's
// node-first, child-slots-1-through-8 traversal.
bool BoundedDynamicPruner::OverlapAABB(Container& objects, const Point& min, const Point& max,
	udword mask)
{
	const udword nb = mPool.mNbObjects[0] + mPool.mNbObjects[1];
	if(!nb)
		return true;

	Point rootMin, rootMax;
	mBounds.GetMin(rootMin);
	mBounds.GetMax(rootMax);
	if(rootMin.x > rootMax.x || rootMin.y > rootMax.y || rootMin.z > rootMax.z)
		{
		bool haveBounds = false;
		for(udword i = 0; i < nb; ++i)
			{
			Prunable* const object = mPool.mObjects[i];
			if(object->mHandle == PRUNABLE_INVALID_HANDLE)
				continue;
			if(!(object->mFlags & PRUNABLE_FLAG_WORLD_AABB_VALID))
				object->UpdateWorldAABB(&mPool.mWorldBoxes[object->mHandle]);
			Point objectMin, objectMax;
			mPool.mWorldBoxes[object->mHandle].GetMin(objectMin);
			mPool.mWorldBoxes[object->mHandle].GetMax(objectMax);
			if(!haveBounds)
				{
				rootMin = objectMin;
				rootMax = objectMax;
				haveBounds = true;
				}
			else
				{
				if(objectMin.x < rootMin.x) rootMin.x = objectMin.x;
				if(objectMin.y < rootMin.y) rootMin.y = objectMin.y;
				if(objectMin.z < rootMin.z) rootMin.z = objectMin.z;
				if(objectMax.x > rootMax.x) rootMax.x = objectMax.x;
				if(objectMax.y > rootMax.y) rootMax.y = objectMax.y;
				if(objectMax.z > rootMax.z) rootMax.z = objectMax.z;
				}
			}
		if(rootMin.x > rootMax.x || rootMin.y > rootMax.y || rootMin.z > rootMax.z)
			return true;
		}
	float rootSize = rootMax.x - rootMin.x;
	if(rootMax.y - rootMin.y > rootSize) rootSize = rootMax.y - rootMin.y;
	if(rootMax.z - rootMin.z > rootSize) rootSize = rootMax.z - rootMin.z;
	if(!(rootSize > 0.0f))
		return true;
	const Point rootCenter((rootMin.x + rootMax.x) * 0.5f,
		(rootMin.y + rootMax.y) * 0.5f, (rootMin.z + rootMax.z) * 0.5f);
	const float treeScale = rootSize;
	// BoundedTree's stored mBounds are the inner simulation bounds. The
	// oracle builds its octree root at twice that side length (tree scale is
	// the stored half-extent), so its nodes can extend one half-extent beyond
	// the simulation bounds.
	rootSize *= 2.0f;
	const float rootRadius = rootSize * 0.5f;
	rootMin = Point(rootCenter.x - rootRadius, rootCenter.y - rootRadius,
		rootCenter.z - rootRadius);
	rootMax = Point(rootCenter.x + rootRadius, rootCenter.y + rootRadius,
		rootCenter.z + rootRadius);
	const udword nodeCount = 37449u;
	BoundedTreeQuery query;
	query.pruner = this;
	query.objects = &objects;
	query.queryMin = &min;
	query.queryMax = &max;
	query.mask = mask;
	Container partialObjects;
	Container containedObjects;
	query.partialObjects = &partialObjects;
	query.containedObjects = &containedObjects;
	query.nodeCount = nodeCount;
	query.nodeHeads = static_cast<udword*>(malloc(sizeof(udword) * nodeCount));
	query.objectNext = static_cast<udword*>(malloc(sizeof(udword) * nb));
	query.subtreeCounts = static_cast<udword*>(calloc(nodeCount, sizeof(udword)));
	if(!query.nodeHeads || !query.objectNext || !query.subtreeCounts)
		{
		if(query.nodeHeads) free(query.nodeHeads);
		if(query.objectNext) free(query.objectNext);
		if(query.subtreeCounts) free(query.subtreeCounts);
		return false;
		}
	memset(query.nodeHeads, 0xff, sizeof(udword) * nodeCount);
	for(udword i = 0; i < nb; ++i)
	{
		Prunable* const object = mPool.mObjects[i];
		if(object->mHandle == PRUNABLE_INVALID_HANDLE)
			continue;
		if(!(object->mFlags & PRUNABLE_FLAG_WORLD_AABB_VALID))
			object->UpdateWorldAABB(&mPool.mWorldBoxes[object->mHandle]);
		const AABB& bounds = mPool.mWorldBoxes[object->mHandle];
		Point objectMin, objectMax;
		bounds.GetMin(objectMin);
		bounds.GetMax(objectMax);
		// subE7850 inserts by the owner's center/radius callback (vtable slot
		// +0x28), not by Prunable's cached world AABB. Convert that sphere to
		// the same enclosing cube the oracle uses before choosing its tree node.
		float centerRadius[4];
		void* const owner = object->mOwner;
		void** const ownerVtable = *reinterpret_cast<void***>(owner);
		typedef void (__thiscall* CenterRadiusFn)(void*, float*);
		reinterpret_cast<CenterRadiusFn>(ownerVtable[10])(owner, centerRadius);
		const float radius = centerRadius[3];
		const Point treeMin(centerRadius[0] - radius, centerRadius[1] - radius,
			centerRadius[2] - radius);
		const Point treeMax(centerRadius[0] + radius, centerRadius[1] + radius,
			centerRadius[2] + radius);
		if(!nxAABBOverlaps(treeMin, treeMax, rootMin, rootMax))
			continue;
		const float objectSize = radius + radius;
		udword depth = 0;
		while(depth < 5 && treeScale / float(1u << (depth + 1)) >= objectSize)
			++depth;
		const float centers[3] = {
			centerRadius[0], centerRadius[1], centerRadius[2] };
		Point nodeCenter = rootCenter;
		float nodeSide = rootSize;
		udword node = 0;
		for(udword level = 0; level < depth; ++level)
			{
			const udword octant = (centers[0] >= nodeCenter.x ? 4u : 0u)
				| (centers[1] >= nodeCenter.y ? 2u : 0u)
				| (centers[2] >= nodeCenter.z ? 1u : 0u);
			node = node * 8u + octant + 1u;
			const float offset = nodeSide * 0.125f;
			nodeCenter.x += (octant & 4u) ? offset : -offset;
			nodeCenter.y += (octant & 2u) ? offset : -offset;
			nodeCenter.z += (octant & 1u) ? offset : -offset;
			nodeSide *= 0.5f;
			}
		query.objectNext[i] = query.nodeHeads[node];
		query.nodeHeads[node] = i;
		for(udword ancestor = node;; ancestor = (ancestor - 1u) / 8u)
			{
			++query.subtreeCounts[ancestor];
			if(ancestor == 0)
				break;
			}
	}
	nxBoundedTreeQueryNode(query, 0u, rootMin, rootMax);
	if(partialObjects.GetNbEntries())
		objects.Add(partialObjects.GetEntries(), partialObjects.GetNbEntries());
	if(containedObjects.GetNbEntries())
		objects.Add(containedObjects.GetEntries(), containedObjects.GetNbEntries());
	free(query.nodeHeads);
	free(query.objectNext);
	free(query.subtreeCounts);
	return true;
}

// phys_fn_005258/005260 (0x000e6060/0x000e6130), bounded-pruner slot 8 ABI.
bool BoundedDynamicPruner::NovodeXPrunerSlot8(udword result, udword queryBounds,
	udword /*first_contact*/, udword mask)
{
	const float* const values = reinterpret_cast<const float*>(queryBounds);
	const Point min(values[0], values[1], values[2]);
	const Point max(values[3], values[4], values[5]);
	Container touched;
	if(!OverlapAABB(touched, min, max, mask))
		return false;
	const udword* entries = touched.GetEntries();
	for(udword i = 0; i < touched.GetNbEntries(); ++i)
		nxSdkContainerAppend(reinterpret_cast<void*>(result), entries[i]);
	return true;
}

bool BoundedDynamicPruner::NovodeXPrunerSlot7(udword result, udword queryBounds,
	udword first_contact, udword mask)
{
	return BoundedDynamicPruner::NovodeXPrunerSlot8(result, queryBounds, first_contact, mask);
}

// 0x000efe80 (not claimed): the base only.
DynamicPruner::~DynamicPruner()
{
}

// phys_fn_005468's pool walk adapted to the four-argument AABB query slot.
bool DynamicPruner::NovodeXPrunerSlot8(udword result, udword queryBounds,
	udword /*first_contact*/, udword mask)
{
	const float* const values = reinterpret_cast<const float*>(queryBounds);
	const Point min(values[0], values[1], values[2]);
	const Point max(values[3], values[4], values[5]);
	bool found = false;
	const udword objectCount = mPool.mNbTotal;
	for(udword i = 0; i < objectCount; ++i)
	{
		Prunable* const object = mPool.mObjects[i];
		if(!(mask & object->mPrunable24) || object->mHandle == PRUNABLE_INVALID_HANDLE)
			continue;
		if(!(object->mFlags & PRUNABLE_FLAG_WORLD_AABB_VALID))
			object->UpdateWorldAABB(&mPool.mWorldBoxes[object->mHandle]);
		const AABB& box = mPool.mWorldBoxes[object->mHandle];
		Point objectMin, objectMax;
		box.GetMin(objectMin);
		box.GetMax(objectMax);
		if(objectMin.x > max.x || objectMax.x < min.x
			|| objectMin.y > max.y || objectMax.y < min.y
			|| objectMin.z > max.z || objectMax.z < min.z)
			continue;
		nxSdkContainerAppend(reinterpret_cast<void*>(result), udword(object));
		found = true;
	}
	return found;
}

bool DynamicPruner::NovodeXPrunerSlot7(udword result, udword queryBounds,
	udword first_contact, udword mask)
{
	return DynamicPruner::NovodeXPrunerSlot8(result, queryBounds, first_contact, mask);
}

// phys_fn_005468 (0x000efa90, 503 B)
// A linear walk over sections 0 and 1. An infinite ray (max_dist exactly
// FLT_MAX, 0x000efaa4) is tested with nxRayAABB; otherwise the segment from
// the origin to origin + dir*max_dist, whose x end keeps the product
// unrounded while y and z spill it (0x000efb7e-0x000efbd5), with
// nxSegmentAABB. A stale box is recomputed first (phys_fn_004886); a hit whose
// mask (+0x24) meets the query's goes into the caller's container, and
// first_contact returns at the first.
bool DynamicPruner::Raycast(Container& objects, const Ray& world_ray, float max_dist,
	bool first_contact, udword mask)
{
	udword nb = mPool.mNbObjects[1] + mPool.mNbObjects[0];
	Prunable** pool = mPool.mObjects;
	if(max_dist == MAX_FLOAT)
	{
		while(nb)
		{
			Prunable* current = *pool;
			nb--;
			if(mask & current->mPrunable24)
			{
				const AABB* box;
				if(current->mHandle == PRUNABLE_INVALID_HANDLE)
					box = null;
				else
				{
					if(!(current->mFlags & PRUNABLE_FLAG_WORLD_AABB_VALID))
						current->UpdateWorldAABB(&mPool.mWorldBoxes[current->mHandle]);
					box = &mPool.mWorldBoxes[current->mHandle];
				}
				if(nxRayAABB(world_ray, reinterpret_cast<const Point*>(box)[0], reinterpret_cast<const Point*>(box)[1]))
				{
					objects.Add(udword(*pool));
					if(first_contact)
						return true;
				}
			}
			pool++;
		}
		return true;
	}

	const double dx = double(max_dist) * world_ray.mDir.x;
	const float dy = float(double(max_dist) * world_ray.mDir.y);
	const float dz = float(double(max_dist) * world_ray.mDir.z);
	Segment segment;
	segment.mP0 = world_ray.mOrig;
	segment.mP1.x = float(dx + world_ray.mOrig.x);
	segment.mP1.y = float(double(dy) + world_ray.mOrig.y);
	segment.mP1.z = float(double(dz) + world_ray.mOrig.z);
	while(nb)
	{
		Prunable* current = *pool;
		nb--;
		if(mask & current->mPrunable24)
		{
			const AABB* box;
			if(current->mHandle == PRUNABLE_INVALID_HANDLE)
				box = null;
			else
			{
				if(!(current->mFlags & PRUNABLE_FLAG_WORLD_AABB_VALID))
					current->UpdateWorldAABB(&mPool.mWorldBoxes[current->mHandle]);
				box = &mPool.mWorldBoxes[current->mHandle];
			}
			if(nxSegmentAABB(segment, reinterpret_cast<const Point*>(box)[0], reinterpret_cast<const Point*>(box)[1]))
			{
				objects.Add(udword(*pool));
				if(first_contact)
					return true;
			}
		}
		pool++;
	}
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// The separating-axis tests.

static inline_ udword nxAbsBits(float value)
{
	udword bits;
	memcpy(&bits, &value, 4);
	return bits & 0x7fffffff;
}

static inline_ udword nxBits(float value)
{
	udword bits;
	memcpy(&bits, &value, 4);
	return bits;
}

// phys_fn_005543 (0x000f17c0, 420 B)
// Per axis: the origin's offset from the box centre and the half extent, both
// spilled to float; separated when the offset's magnitude, compared as bits
// (unsigned), exceeds the extent and the direction points away (offset*dir
// >= 0). Then the three cross axes against |dir|, all in registers.
__declspec(noinline) bool nxRayAABB(const Ray& ray, const Point& min, const Point& max)
{
	const float diffX	= float(double(ray.mOrig.x) - (double(max.x) + min.x) * 0.5);
	const float extX	= float((double(max.x) - min.x) * 0.5);
	if(nxAbsBits(diffX) > nxBits(extX) && double(diffX) * ray.mDir.x >= 0.0)
		return false;
	const float diffY	= float(double(ray.mOrig.y) - (double(max.y) + min.y) * 0.5);
	const float extY	= float((double(max.y) - min.y) * 0.5);
	if(nxAbsBits(diffY) > nxBits(extY) && double(diffY) * ray.mDir.y >= 0.0)
		return false;
	const float diffZ	= float(double(ray.mOrig.z) - (double(max.z) + min.z) * 0.5);
	const float extZ	= float((double(max.z) - min.z) * 0.5);
	if(nxAbsBits(diffZ) > nxBits(extZ) && double(diffZ) * ray.mDir.z >= 0.0)
		return false;

	float absDirX, absDirY, absDirZ;
	{
		udword bits = nxAbsBits(ray.mDir.x);	memcpy(&absDirX, &bits, 4);
		bits = nxAbsBits(ray.mDir.y);			memcpy(&absDirY, &bits, 4);
		bits = nxAbsBits(ray.mDir.z);			memcpy(&absDirZ, &bits, 4);
	}

	double f = fabs(double(diffZ) * ray.mDir.y - double(diffY) * ray.mDir.z);
	if(double(absDirY) * extZ + double(absDirZ) * extY < f)
		return false;
	f = fabs(double(diffX) * ray.mDir.z - double(diffZ) * ray.mDir.x);
	if(double(absDirX) * extZ + double(absDirZ) * extX < f)
		return false;
	f = fabs(double(diffY) * ray.mDir.x - double(diffX) * ray.mDir.y);
	if(double(absDirX) * extY + double(absDirY) * extX < f)
		return false;
	return true;
}

// phys_fn_005541 (0x000f1600, 444 B)
// The segment's half direction, the box extent and the centre offset, per
// axis, all spilled to float; separated on an axis when |offset| exceeds
// |half direction| + extent, then the three cross axes.
__declspec(noinline) bool nxSegmentAABB(const Segment& segment, const Point& min, const Point& max)
{
	const float dirX	= float((double(segment.mP1.x) - segment.mP0.x) * 0.5);
	const float extX	= float((double(max.x) - min.x) * 0.5);
	const float diffX	= float(((double(segment.mP0.x) + segment.mP1.x) - (double(min.x) + max.x)) * 0.5);
	const float absDirX	= float(fabs(double(dirX)));
	if(double(absDirX) + extX < fabs(double(diffX)))
		return false;
	const float dirY	= float((double(segment.mP1.y) - segment.mP0.y) * 0.5);
	const float extY	= float((double(max.y) - min.y) * 0.5);
	const float diffY	= float(((double(segment.mP0.y) + segment.mP1.y) - (double(min.y) + max.y)) * 0.5);
	const float absDirY	= float(fabs(double(dirY)));
	if(double(absDirY) + extY < fabs(double(diffY)))
		return false;
	const float dirZ	= float((double(segment.mP1.z) - segment.mP0.z) * 0.5);
	const float extZ	= float((double(max.z) - min.z) * 0.5);
	const float diffZ	= float(((double(segment.mP0.z) + segment.mP1.z) - (double(min.z) + max.z)) * 0.5);
	const float absDirZ	= float(fabs(double(dirZ)));
	if(double(absDirZ) + extZ < fabs(double(diffZ)))
		return false;

	double f = fabs(double(diffZ) * dirY - double(dirZ) * diffY);
	if(double(extZ) * absDirY + double(absDirZ) * extY < f)
		return false;
	f = fabs(double(dirZ) * diffX - double(diffZ) * dirX);
	if(double(extZ) * absDirX + double(absDirZ) * extX < f)
		return false;
	f = fabs(double(diffY) * dirX - double(dirY) * diffX);
	if(double(extY) * absDirX + double(absDirY) * extX < f)
		return false;
	return true;
}
