PREFIX = 'nxPOK'
ROWS = [('nxPOSetCenterExtents', 132032, 132152, 'phys_fn_000923 (inlined AABB::SetCenterExtents)', '__declspec(naked) void __fastcall nxPOSetCenterExtents(void* aabb, void* edxUnused, const void* center, const void* extents)'),
 ('nxPOIsInside', 929648, 929738, 'phys_fn_005145 (AABB::IsInside)', '__declspec(naked) bool __fastcall nxPOIsInside(const void* self, void* edxUnused, const void* other)'),
 ('nxPOScaleBounds', 946768, 946985, 'phys_fn_005295', '__declspec(naked) void __fastcall nxPOScaleBounds(void* bounds, void* edxUnused, float scale)'),
 ('nxPOSetNodeBounds', 947072, 947524, 'phys_fn_005303', '__declspec(naked) void __fastcall nxPOSetNodeBounds(void* tree, void* edxUnused, unsigned node, const void* bounds)'),
 ('nxPOWalk', 947568, 947657, 'phys_fn_005305', '__declspec(naked) void __fastcall nxPOWalk(void* tree, void* edxUnused, unsigned node, void* callback, void* user0, void* user1)'),
 ('nxPOLeafCallback', 947664, 947685, 'phys_fn_005307', '__declspec(naked) bool __cdecl nxPOLeafCallback(void* a, void* b)'),
 ('nxPORemove', 947696, 947813, 'phys_fn_005309', '__declspec(naked) bool __fastcall nxPORemove(void* tree, void* edxUnused, void* object)'),
 ('nxPOForEach', 947856, 947889, 'phys_fn_005313', '__declspec(naked) bool __fastcall nxPOForEach(void* tree, void* edxUnused, void* callback, void* user)'),
 ('nxPOBuild', 947904, 948295, 'phys_fn_005315', '__declspec(naked) bool __fastcall nxPOBuild(void* tree, void* edxUnused, unsigned depth, const void* bounds)'),
 ('nxPOInsert', 948304, 949120, 'phys_fn_005317', '__declspec(naked) bool __fastcall nxPOInsert(void* tree, void* edxUnused, void* object, const void* bounds)'),
 ('nxPOUpdate', 949120, 949256, 'phys_fn_005318', '__declspec(naked) bool __fastcall nxPOUpdate(void* tree, void* edxUnused, void* object)'),
 ('nxPQSphereAABB', 989552, 989730, 'phys_fn_005545', '__declspec(naked) bool __cdecl nxPQSphereAABB(const float* center, float radius, const float* boxMin, const float* boxMax)'),
 ('nxPQAppendNode', 989744, 989800, 'phys_fn_005547', '__declspec(naked) void __fastcall nxPQAppendNode(void* node, void* edxUnused, void* container)'),
 ('nxPQAppendSubtree', 989808, 989888, 'phys_fn_005549', '__declspec(naked) void __cdecl nxPQAppendSubtree(unsigned node, unsigned count, void* nodes, void* container)'),
 ('nxPQPlaneWalk', 989888, 990300, 'phys_fn_005550', '__declspec(naked) void __cdecl nxPQPlaneWalk(unsigned node, unsigned mask, void* query)'),
 ('nxPQPlaneQuery', 990304, 990374, 'phys_fn_005552', '__declspec(naked) void __fastcall nxPQPlaneQuery(void* tree, void* edxUnused, const void* planes, unsigned nbPlanes, void* containerA, void* containerB)'),
 ('nxPQSegmentWalk', 990384, 990477, 'phys_fn_005554', '__declspec(naked) void __cdecl nxPQSegmentWalk(unsigned node, void* query)'),
 ('nxPQSegmentQuery', 990480, 990529, 'phys_fn_005556', '__declspec(naked) void __fastcall nxPQSegmentQuery(void* tree, void* edxUnused, const void* segment, void* container)'),
 ('nxPQRayWalk', 990544, 990637, 'phys_fn_005558', '__declspec(naked) void __cdecl nxPQRayWalk(unsigned node, void* query)'),
 ('nxPQRayQuery', 990640, 990689, 'phys_fn_005560', '__declspec(naked) void __fastcall nxPQRayQuery(void* tree, void* edxUnused, const void* ray, void* container)')]
CALLS = {269172736: 'nxPOCallAllocator', 269364816: 'nxPOCallMakeCube', 268567488: 'nxPOSetCenterExtents', 269365104: 'nxPOIsInside', 269449788: 'nxPOCallFtol', 269382224: 'nxPOScaleBounds', 269382528: 'nxPOSetNodeBounds', 269383024: 'nxPOWalk', 269383152: 'nxPORemove', 269383760: 'nxPOInsert', 269176288: 'nxPQCallContainerResize', 269424128: 'nxPQCallSegmentAABB', 269424576: 'nxPQCallRayAABB', 269425200: 'nxPQAppendNode', 269425264: 'nxPQAppendSubtree', 269425344: 'nxPQPlaneWalk', 269425840: 'nxPQSegmentWalk', 269426000: 'nxPQRayWalk'}
DATA = {269649152: 'nxPOAdapterNotify'}
PUSHES = {269383120: 'nxPOLeafCallback'}
TABLES = {269382980: 8}
