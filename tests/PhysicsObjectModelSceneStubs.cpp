// The isolated shape layout/vtable targets compile ObjectModel.cpp without
// Scene.cpp. They do not exercise a live shape owner teardown, so they provide
// this link-only seam; product and collision targets link the real function.
void NxSceneRemoveOwnerPairRecords(void*, const void*) {}

// The isolated shape-layout targets still do not exercise live owner teardown.
// Keep the newly linked pruner calls local to those targets; the product and
// collision targets resolve the real implementations in Scene.cpp and
// IcePrunable.cpp.
extern "C" void __fastcall NxScenePrunerShapeRemove(void*, void*) {}
