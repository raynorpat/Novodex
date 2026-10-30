SYMBOLS = {0xdeb0: '?getDebugRenderable@PhysicsSDK@@QAEPAVNxDebugRenderable@@XZ', 0x14f0: '?getNbShapes@JointActorBody@@QBEIXZ', 0x1520: '?getShapes@JointActorBody@@QAEPAPAXXZ',
           0xb5260: '?Dispatch@PrunableOwnerDispatcher@@QAEIPAX@Z', 0xe71b0: '??1SweepAndPrune@Opcode@@QAE@XZ',
           0xb4000: '?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ'}
NAMES = {'phys_fn_001935': 'nxPrunerVisualizeNodeCallback', 'phys_fn_001945': 'nxPrunerOwnerDestroyRow'}
DECLS = {'phys_fn_001935': '__declspec(naked) bool __cdecl nxPrunerVisualizeNodeCallback(void* unused, void* node)', 'phys_fn_001945': '__declspec(naked) void __fastcall nxPrunerOwnerDestroyRow(void* manager, void* edxUnused, void* owner)'}
HEAD_COMMENT = """PrunableRows.cpp: phys_fn_001945 with 001947 (0x0004bbd0, one function the inventory splits at the loop target
0x0004bc20): the scene's removal of an owner's primary and compound-child prunables through the pruners' dispatcher,
then the SweepAndPrune the manager owns destroyed and freed. A naked listing transcription; NxScenePrunerOwnerDestroy
(IcePrunable.cpp) keeps its C++ body, which the ObjectModel-only test targets link without the joint rows. Compare script:
docs/reconstruction/novodex-physics/evidence/phase3-prunable-owner-destroy-listing-compare.py."""
DATA = {0x10123c04: 'nxPodSdkInstance'}
DATADECL = ['extern "C" void* nxPodSdkInstance;', '#pragma comment(linker, "/alternatename:_nxPodSdkInstance=?instance@PhysicsSDK@@2PAV1@A")']
HEAD_COMMENT = HEAD_COMMENT + '''
phys_fn_001935 (0x0004b9c0): the callback the pruners' visualisation walk (001967) passes: a node with a box at
+0x18 is drawn through the SDK's debug renderable (slot +0x2c, colour 0xffff0000). The candidate's walk
(SceneVisualize.cpp) uses its own callback, so nothing calls this one yet.'''
