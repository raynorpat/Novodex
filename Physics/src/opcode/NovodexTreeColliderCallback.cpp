#include "Opcode.h"

using namespace Opcode;

#include "OPC_BoxBoxOverlap.h"
#include "OPC_TriBoxOverlap.h"
// The header defines CoplanarTriTri outside the class, with external linkage, and
// NxOpcode already has it; this unit keeps its own copy under another name.
#define CoplanarTriTri NxCallbackCoplanarTriTri
#include "OPC_TriTriOverlap.h"
#undef CoplanarTriTri
#include "NovodexTreeColliderCallback.h"

// The oracle carries a second instantiation of the no-leaf walk (rows 004967,
// 004969, 004971, entered through 004982). It is the stock walk with four
// differences, each read off the listing:
//  - the tree-1/tree-0 triangles are read straight out of the mesh interface's
//    index and vertex arrays (+8 and +0xc, stride 12), not through GetTriangle;
//  - a triangle/triangle hit only sets OPC_CONTACT; no pair is recorded;
//  - the early-out tests OPC_CONTACT alone, not OPC_FIRST_CONTACT as well;
//  - every node pair carries a depth. After a child step the walk stops once
//    the depth is below one and a contact is set, and then reports the pair
//    itself if the depth is exactly zero; with a positive depth each triangle
//    test that leaves a contact set reports the pair and the walk goes on.

void NxCallbackTreeCollider::TriTestCallback(const MeshInterface* mesh, udword triangle)
	{
	const IndexedTriangle* tri = &mesh->GetTris()[triangle];
	const Point* verts = mesh->GetVerts();
	if(TriTriOverlap(mLeafVerts[0], mLeafVerts[1], mLeafVerts[2],
		verts[tri->mVRef[0]], verts[tri->mVRef[1]], verts[tri->mVRef[2]]))
		mFlags |= OPC_CONTACT;
	}

void NxCallbackTreeCollider::FetchLeaf(const MeshInterface* mesh, udword triangle,
	const Matrix3x3& rotation, const Point& translation)
	{
	mLeafIndex = triangle;
	const IndexedTriangle* tri = &mesh->GetTris()[triangle];
	const Point* verts = mesh->GetVerts();
	TransformPoint(mLeafVerts[0], verts[tri->mVRef[0]], rotation, translation);
	TransformPoint(mLeafVerts[1], verts[tri->mVRef[1]], rotation, translation);
	TransformPoint(mLeafVerts[2], verts[tri->mVRef[2]], rotation, translation);
	}

// True when the walk must stop after the step just taken.
bool NxCallbackTreeCollider::StepDone(NxTreeNodeCallback callback, int depth,
	const AABBNoLeafNode* node0, const AABBNoLeafNode* node1, bool leafStep)
	{
	const bool contact = (mFlags & OPC_CONTACT) != 0;
	if(leafStep && depth > 0)
		{
		if(contact) callback(node0, node1);
		return false;
		}
	if(depth < 1 && contact)
		{
		if(depth == 0) callback(node0, node1);
		return true;
		}
	return false;
	}

// phys_fn_004967
void NxCallbackTreeCollider::CollideTriBoxCallback(const AABBNoLeafNode* b,
	NxTreeNodeCallback callback, int depth, const AABBNoLeafNode* node0)
	{
	if(!TriBoxOverlap(b->mAABB.mCenter, b->mAABB.mExtents)) return;

	if(b->HasPosLeaf()) TriTestCallback(mIMesh1, b->GetPosPrimitive());
	else CollideTriBoxCallback(b->GetPos(), callback, depth - 1, node0);
	if(StepDone(callback, depth, node0, b, b->HasPosLeaf() != 0)) return;

	if(b->HasNegLeaf()) TriTestCallback(mIMesh1, b->GetNegPrimitive());
	else CollideTriBoxCallback(b->GetNeg(), callback, depth - 1, node0);
	StepDone(callback, depth, node0, b, b->HasNegLeaf() != 0);
	}

// phys_fn_004969
void NxCallbackTreeCollider::CollideBoxTriCallback(const AABBNoLeafNode* a,
	NxTreeNodeCallback callback, int depth, const AABBNoLeafNode* node1)
	{
	if(!TriBoxOverlap(a->mAABB.mCenter, a->mAABB.mExtents)) return;

	if(a->HasPosLeaf()) TriTestCallback(mIMesh0, a->GetPosPrimitive());
	else CollideBoxTriCallback(a->GetPos(), callback, depth - 1, node1);
	if(StepDone(callback, depth, a, node1, a->HasPosLeaf() != 0)) return;

	if(a->HasNegLeaf()) TriTestCallback(mIMesh0, a->GetNegPrimitive());
	else CollideBoxTriCallback(a->GetNeg(), callback, depth - 1, node1);
	StepDone(callback, depth, a, node1, a->HasNegLeaf() != 0);
	}

// phys_fn_004971
void NxCallbackTreeCollider::CollideCallback(const AABBNoLeafNode* a, const AABBNoLeafNode* b,
	NxTreeNodeCallback callback, int depth)
	{
	if(!BoxBoxOverlap(a->mAABB.mExtents, a->mAABB.mCenter, b->mAABB.mExtents, b->mAABB.mCenter)) return;

	const bool bPosLeaf = b->HasPosLeaf() != 0;
	const bool bNegLeaf = b->HasNegLeaf() != 0;

	if(a->HasPosLeaf())
		{
		FetchLeaf(mIMesh0, a->GetPosPrimitive(), mR0to1, mT0to1);
		if(bPosLeaf) TriTestCallback(mIMesh1, b->GetPosPrimitive());
		else CollideTriBoxCallback(b->GetPos(), callback, depth - 1, a);
		if(StepDone(callback, depth, a, b, bPosLeaf)) return;
		if(bNegLeaf) TriTestCallback(mIMesh1, b->GetNegPrimitive());
		else CollideTriBoxCallback(b->GetNeg(), callback, depth - 1, a);
		if(StepDone(callback, depth, a, b, bNegLeaf)) return;
		}
	else
		{
		if(bPosLeaf)
			{
			FetchLeaf(mIMesh1, b->GetPosPrimitive(), mR1to0, mT1to0);
			CollideBoxTriCallback(a->GetPos(), callback, depth - 1, b);
			}
		else CollideCallback(a->GetPos(), b->GetPos(), callback, depth - 1);
		if(StepDone(callback, depth, a, b, false)) return;
		if(bNegLeaf)
			{
			FetchLeaf(mIMesh1, b->GetNegPrimitive(), mR1to0, mT1to0);
			CollideBoxTriCallback(a->GetPos(), callback, depth - 1, b);
			}
		else CollideCallback(a->GetPos(), b->GetNeg(), callback, depth - 1);
		if(StepDone(callback, depth, a, b, false)) return;
		}

	if(a->HasNegLeaf())
		{
		FetchLeaf(mIMesh0, a->GetNegPrimitive(), mR0to1, mT0to1);
		if(bPosLeaf) TriTestCallback(mIMesh1, b->GetPosPrimitive());
		else CollideTriBoxCallback(b->GetPos(), callback, depth - 1, a);
		if(StepDone(callback, depth, a, b, bPosLeaf)) return;
		if(bNegLeaf) TriTestCallback(mIMesh1, b->GetNegPrimitive());
		else CollideTriBoxCallback(b->GetNeg(), callback, depth - 1, a);
		StepDone(callback, depth, a, b, bNegLeaf);
		}
	else
		{
		if(bPosLeaf)
			{
			FetchLeaf(mIMesh1, b->GetPosPrimitive(), mR1to0, mT1to0);
			CollideBoxTriCallback(a->GetNeg(), callback, depth - 1, b);
			}
		else CollideCallback(a->GetNeg(), b->GetPos(), callback, depth - 1);
		if(StepDone(callback, depth, a, b, false)) return;
		if(bNegLeaf)
			{
			FetchLeaf(mIMesh1, b->GetNegPrimitive(), mR1to0, mT1to0);
			CollideBoxTriCallback(a->GetNeg(), callback, depth - 1, b);
			}
		else CollideCallback(a->GetNeg(), b->GetNeg(), callback, depth - 1);
		StepDone(callback, depth, a, b, false);
		}
	}

// phys_fn_004982
bool NxCallbackTreeCollider::CollideNoLeafCallback(NxTreeNodeCallback callback, int depth,
	const Model* model0, const Model* model1, const Matrix4x4* world0, const Matrix4x4* world1)
	{
	mIMesh0 = model0->GetMeshInterface();
	mIMesh1 = model1->GetMeshInterface();
	if(!mIMesh0 || !mIMesh1) return false;
	if(model0->HasLeafNodes() || model0->IsQuantized() || model1->HasLeafNodes() || model1->IsQuantized())
		return false;

	const AABBNoLeafTree* tree0 = (const AABBNoLeafTree*) model0->GetTree();
	const AABBNoLeafTree* tree1 = (const AABBNoLeafTree*) model1->GetTree();
	InitQuery(world0, world1);
	CollideCallback(tree0->GetNodes(), tree1->GetNodes(), callback, depth);
	return true;
	}

void NxCallbackTreeCollider::Prepare(const MeshInterface* mesh0, const MeshInterface* mesh1,
	const Matrix4x4* world0, const Matrix4x4* world1)
	{
	mIMesh0 = mesh0;
	mIMesh1 = mesh1;
	InitQuery(world0, world1);
	}

void NxCallbackTreeCollider::FetchLeaf0(udword triangle) { FetchLeaf(mIMesh0, triangle, mR0to1, mT0to1); }
void NxCallbackTreeCollider::FetchLeaf1(udword triangle) { FetchLeaf(mIMesh1, triangle, mR1to0, mT1to0); }

#define NX_FIELD_OFFSET(field) ((size_t) &((NxCallbackTreeCollider*) 0)->field)
bool NxCallbackTreeCollider::LayoutMatchesOracle()
	{
	return NX_FIELD_OFFSET(mFlags) == 0x04
		&& NX_FIELD_OFFSET(mIMesh0) == 0x20 && NX_FIELD_OFFSET(mIMesh1) == 0x24
		&& NX_FIELD_OFFSET(mNbBVBVTests) == 0x28 && NX_FIELD_OFFSET(mNbPrimPrimTests) == 0x2c
		&& NX_FIELD_OFFSET(mNbBVPrimTests) == 0x30 && NX_FIELD_OFFSET(mAR) == 0x34
		&& NX_FIELD_OFFSET(mR0to1) == 0x58 && NX_FIELD_OFFSET(mR1to0) == 0x7c
		&& NX_FIELD_OFFSET(mT0to1) == 0xa0 && NX_FIELD_OFFSET(mT1to0) == 0xac
		&& NX_FIELD_OFFSET(mLeafVerts) == 0xe8 && NX_FIELD_OFFSET(mLeafIndex) == 0x10c
		&& NX_FIELD_OFFSET(mFullBoxBoxTest) == 0x110 && NX_FIELD_OFFSET(mFullPrimBoxTest) == 0x111;
	}
#undef NX_FIELD_OFFSET
