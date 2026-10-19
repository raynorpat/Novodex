#ifndef NOVODEX_TREE_COLLIDER_CALLBACK_H
#define NOVODEX_TREE_COLLIDER_CALLBACK_H

#include "Opcode.h"

namespace Opcode
	{
// NovodeX's depth-limited, callback-reporting walk over two no-leaf trees.
// The callback receives the (tree-0 node, tree-1 node) pair at which the walk
// stopped; it is cdecl, as the oracle's entry (phys_fn_001874) is.
typedef void (__cdecl* NxTreeNodeCallback)(const AABBNoLeafNode* node0, const AABBNoLeafNode* node1);

// Derived only for access to AABBTreeCollider's protected state; it adds no
// data, so it has the layout of the oracle's collider.
class NxCallbackTreeCollider : public AABBTreeCollider
	{
public:
	// phys_fn_004982 (0x000d12b0)
	bool CollideNoLeafCallback(NxTreeNodeCallback callback, int depth,
		const Model* model0, const Model* model1,
		const Matrix4x4* world0, const Matrix4x4* world1);

	// The three rows under it, public so a harness can enter each directly, as the
	// oracle's own callers do, with the collider already set up.
	// phys_fn_004967 (0x000ca5a0): leaf triangle of tree 0 against a tree-1 node
	void CollideTriBoxCallback(const AABBNoLeafNode* node, NxTreeNodeCallback callback, int depth,
		const AABBNoLeafNode* node0);
	// phys_fn_004969 (0x000cbe50): leaf triangle of tree 1 against a tree-0 node
	void CollideBoxTriCallback(const AABBNoLeafNode* node, NxTreeNodeCallback callback, int depth,
		const AABBNoLeafNode* node1);
	// phys_fn_004971 (0x000cd700)
	void CollideCallback(const AABBNoLeafNode* a, const AABBNoLeafNode* b, NxTreeNodeCallback callback, int depth);

	// Harness seams: what 004982 leaves behind before it enters 004971, and what
	// 004971 leaves in the leaf slot before it enters 004967 or 004969.
	void Prepare(const MeshInterface* mesh0, const MeshInterface* mesh1,
		const Matrix4x4* world0, const Matrix4x4* world1);
	void FetchLeaf0(udword triangle);
	void FetchLeaf1(udword triangle);
	const Point* LeafVerts() const { return mLeafVerts; }
	// Offsets of the fields the oracle's rows use, against the oracle's.
	static bool LayoutMatchesOracle();

private:
	void TriTestCallback(const MeshInterface* mesh, udword triangle);
	void FetchLeaf(const MeshInterface* mesh, udword triangle, const Matrix3x3& rotation, const Point& translation);
	bool StepDone(NxTreeNodeCallback callback, int depth, const AABBNoLeafNode* node0,
		const AABBNoLeafNode* node1, bool leafStep);
	};
	}

#endif
