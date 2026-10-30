SYMBOLS = {0x5b700: '?lock@ReadWriteLock@@QAE_NXZ', 0x5b730: '?tryLock@ReadWriteLock@@QAE_NXZ', 0x5b790: '?unlock@ReadWriteLock@@QAE_NXZ',
           0xb4000: '?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ',
           0xdef0: '?getNbScenes@PhysicsSDK@@QBEIXZ', 0xdf00: '?getScene@PhysicsSDK@@QAEPAVScene@@I@Z'}
DATA = {0x10123c04: 'nxSdkInstanceSlot', 0x10123c10: 'gNxApiReentry', 0x10122050: 'gNxApiReentryMessage'}
DATADECL = ['extern "C" void* nxSdkInstanceSlot;', '#pragma comment(linker, "/alternatename:_nxSdkInstanceSlot=?instance@PhysicsSDK@@2PAV1@A")',
            'extern "C" unsigned gNxApiReentry;', 'extern "C" const char* gNxApiReentryMessage;']
INCLUDES = ['NxPhysics.h']
HEAD_COMMENT = """NpTriangleMeshRows.cpp: the public triangle mesh's methods (the oracle's NpTriangleMesh.cpp, vtable at .rdata
0x00108298): each one takes every scene's lock (the SDK's scene list), calls the internal mesh at +4 through its
table and releases the locks. Naked listing transcriptions of phys_fn_002095 to 002138, each a thunk-and-body pair
the inventory splits at the loop's jump target. The candidate has no public triangle-mesh facade object yet, so
nothing calls them. Compare script: docs/reconstruction/novodex-physics/evidence/phase4-np-triangle-mesh-rows-listing-compare.py."""
