#ifndef NX_PHYSICS_SCENE_AUXILIARY
#define NX_PHYSICS_SCENE_AUXILIARY

// phys_fn_000649 (0x00013000): release and clear the ten array records in
// the 0xa8-byte auxiliary manager embedded at Scene+0x48.
void nxSceneAuxDestroy(void* auxiliary);

#endif
