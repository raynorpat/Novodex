#ifndef NX_PHYSICS_SCENE_VISUALIZE
#define NX_PHYSICS_SCENE_VISUALIZE
/*----------------------------------------------------------------------------*\
|
|								NovodeX Technology
|
|							     www.novodex.com
|
\*----------------------------------------------------------------------------*/
// The debug visualisation rows NxScene::visualize reaches (scene-raycast
// block, Task 4, visualisation sub-area). The public path is
//
//   NxScene::visualize (NpScene table slot 31) -> phys_fn_000344 (NpScene.cpp)
//     -> phys_fn_000657 NxSceneInternal::visualize (Scene.cpp)
//        -> phys_fn_000579 NxSceneInternal::getDebugRenderable (Scene.cpp)
//        -> phys_fn_000020 NxActorVisualRecord::visualize, per actor (+0x14)
//           -> phys_fn_000766 NxBodyVisualRecord::visualize (the body, +0x08)
//        -> phys_fn_000907 NxPairNode::row000907, per contact pair node
//           -> phys_fn_000869 NxActorPair::row000869 (the node's +0x14)
//
// and the Scene's debug renderable (+0x6b8, a Foundation NxDebugRenderable)
// reaches the user through NxPhysicsSDK::visualize, which renders every
// renderable the Foundation holds.
//
// The receivers are offset-addressed records the oracle reads by raw offset;
// the classes below have no data members and are reached by casting the
// record's address, so that each row is a member function (__thiscall with
// its stack arguments, as the image's are) without pretending the layouts
// are recovered.

#include "Nxp.h"

class NxDebugRenderable;

// The 0x260-byte dynamic body record (vtable 0x10106890).
class NxBodyVisualRecord
	{
	public:
	void visualize(NxDebugRenderable& renderable);
	};

// The 0x50-byte actor record at NpActor +0x14 (body at +0x08, the static
// pose at +0x20).
class NxActorVisualRecord
	{
	public:
	void visualize(NxDebugRenderable& renderable);
	};

// The contact rows 000907 and 000869 are members of the contact-pair
// manager's NxPairNode and NxActorPair (ContactPairManager.h): the Scene's
// pair list at +0x674 holds the nodes (link +0x08, stamp +0x104), and the
// actor pair at the node's +0x14 owns the contact stream 000873/000875 write.

// Rows phys_fn_000657 calls that are not written (placeholders, not claimed;
// see SceneVisualize.cpp). Each is reached only when a visualisation
// parameter other than the body, actor and world axes ones is set, except
// 001978, which runs whenever NX_VISUALIZATION_SCALE is non-zero and draws
// nothing unless NX_VISUALIZE_COLLISION_{SAP,STATIC,DYNAMIC,FREE} is set.
void nxSceneVisualizeCollisionPruners(void* engine, NxDebugRenderable* renderable);				// 001978
void nxSceneVisualizeCollisionBounds(void* engine, NxDebugRenderable* renderable, NxU32 colour,
	bool compounds);																				// 000638
void nxSceneVisualizeCollisionShapes(void* engine, NxDebugRenderable* renderable);				// 000581
void nxSceneVisualizeFluids(void* fluids, NxDebugRenderable* renderable);						// 003639

#endif
