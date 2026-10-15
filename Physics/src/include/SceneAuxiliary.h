#ifndef NX_PHYSICS_SCENE_AUXILIARY
#define NX_PHYSICS_SCENE_AUXILIARY

// phys_fn_000649 (0x00013000): release and clear the ten array records in
// the 0xa8-byte auxiliary manager embedded at Scene+0x48.
void nxSceneAuxDestroy(void* auxiliary);

// phys_fn_000624 (0x000116b0): release and clear four array records in one
// 0x40-byte auxiliary block. This is an internal object view, not a public API.
struct NxSceneAuxiliaryArrayBlock
	{
	__declspec(noinline) void destroy();
	};

#endif
