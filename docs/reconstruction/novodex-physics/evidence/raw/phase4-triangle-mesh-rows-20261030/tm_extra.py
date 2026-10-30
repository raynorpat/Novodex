SYMBOLS = {0xf4b10: '__CIpow', 0x2e2f0: '?nxSupportMapInit@@YI_NPAUIceSupportMap@@II@Z', 0x2e640: '?nxSupportMapPlaneConstruct@@YIPAUIceSupportMap@@PAU1@IPAVConvexHull@@@Z', 0x2e7c0: '?nxSupportMapVertexConstruct@@YIPAUIceSupportMap@@PAU1@IPBVConvexHull@@@Z', 0x2e5a0: '?nxSupportMapHullConstruct@@YIPAUIceSupportMap@@PAU1@IPAVConvexHull@@@Z',
           0x533c0: '_NxBuildSmoothNormals', 0xe33c0: '?Sort@RadixSort@IceCore@@QAEAAV12@PBIIW4RadixHint@2@@Z',
           0x54460: '?createEdgeList@TriangleMesh@@QAEXXZ', 0x29980: '?nxHullShapeBuild@@YAXXZ', 0x299a0: '?nxHullShapeInit@@YAXXZ', 0x5b700: '?lock@ReadWriteLock@@QAE_NXZ', 0x5b730: '?tryLock@ReadWriteLock@@QAE_NXZ', 0x5b790: '?unlock@ReadWriteLock@@QAE_NXZ',
           0xb4000: '?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ',
           0xdef0: '?getNbScenes@PhysicsSDK@@QBEIXZ', 0xdf00: '?getScene@PhysicsSDK@@QAEPAVScene@@I@Z'}
DATA = {0x10123c04: 'nxSdkInstanceSlot', 0x10123c10: 'gNxApiReentry', 0x10122050: 'gNxApiReentryMessage'}
DATADECL = ['extern "C" void* nxSdkInstanceSlot;', '#pragma comment(linker, "/alternatename:_nxSdkInstanceSlot=?instance@PhysicsSDK@@2PAV1@A")',
            'extern "C" unsigned gNxApiReentry;', 'extern "C" const char* gNxApiReentryMessage;']
INCLUDES = ['NxPhysics.h']

PUSHES = {0x10107724: 'nxTMHullTable', 0x10027f00: 'nxTMIdentityCtor', 0x1010767c: 'nxTMVtHullShape', 0x1010769c: 'nxTMVtHull',
          0x10107848: 'nxTMVtSupportBase', 0x1010786c: 'nxTMVtSupportPlane', 0x10107890: 'nxTMVtSupportVertex', 0x1010785c: 'nxTMVtSupportHull'}
TABLES = {0x1005469c: 5, 0x10054794: 5}
HEAD_COMMENT = """TriangleMeshRows.cpp: the internal triangle mesh's rows not yet reconstructed (the oracle's TriangleMesh.cpp):
phys_fn_001524 and 001528 (the hull subclass the mesh builds for its convex form: its constructor over 001409 and
its scalar deleting destructor, with its table nxTMHullTable, the oracle's 0x10107724), 002077, 002178, 002184,
002190 to 002194, 002200, 002205, 002239, 002243 with 002245, 002247, 002255 and 002256. Naked listing
transcriptions. The small rows they call that the candidate has only as harness models or with another calling
convention are transcribed here as helpers (marked "helper" below; their own records are unchanged). The oracle's
vtables are the candidate's tables by name (aliases). Compare script:
docs/reconstruction/novodex-physics/evidence/phase4-triangle-mesh-rows-listing-compare.py."""
DATADECL = DATADECL + [
 'extern "C" void* nxTMIdentityCtor;', '#pragma comment(linker, "/alternatename:_nxTMIdentityCtor=_nxIceIdentityConstruct")',
 'extern "C" void* nxTMVtHullShape;', '#pragma comment(linker, "/alternatename:_nxTMVtHullShape=?nxHullShapeVtable@@3PAPAXA")',
 'extern "C" void* nxTMVtHull;', '#pragma comment(linker, "/alternatename:_nxTMVtHull=?nxHullVtable@@3PAPAXA")',
 'extern "C" void* nxTMVtSupportBase;', '#pragma comment(linker, "/alternatename:_nxTMVtSupportBase=?gIceSupportMapBaseTable@@3QBQBXB")',
 'extern "C" void* nxTMVtSupportHull;', '#pragma comment(linker, "/alternatename:_nxTMVtSupportHull=?gIceSupportMapHullTable@@3QBQBXB")',
 'extern "C" void* nxTMVtSupportPlane;', '#pragma comment(linker, "/alternatename:_nxTMVtSupportPlane=?gIceSupportMapPlaneTable@@3QBQBXB")',
 'extern "C" void* nxTMVtSupportVertex;', '#pragma comment(linker, "/alternatename:_nxTMVtSupportVertex=?gIceSupportMapVertexTable@@3QBQBXB")']
