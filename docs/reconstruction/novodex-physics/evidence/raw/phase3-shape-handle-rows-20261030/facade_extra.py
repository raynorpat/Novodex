SYMBOLS = {0x5b700: '?lock@ReadWriteLock@@QAE_NXZ', 0x5b730: '?tryLock@ReadWriteLock@@QAE_NXZ', 0x5b790: '?unlock@ReadWriteLock@@QAE_NXZ',
           0x276f0: '?nxSetMaterialIndex@ShapeBase@@QAEXG@Z', 0x24fc0: '?nxPlaneSetEquation@PlaneShape@@QAEXPBMM@Z'}
DATA = {0x10123c10: 'gNxApiReentry', 0x10122050: 'gNxApiReentryMessage'}
DATADECL = ['extern "C" unsigned gNxApiReentry;', 'extern "C" const char* gNxApiReentryMessage;']
INCLUDES = ['NxPhysics.h']
HEAD_COMMENT = """NpShapeHandleRows.cpp: the public shape handles' methods (NpBoxShape.cpp, NpPlaneShape.cpp, NpSphereShape.cpp,
NpCapsuleShape.cpp and NpTriangleMeshShape.cpp in the oracle's tree) that were not yet reconstructed: each takes the
handle's read or write lock (+0x14, +0x10), calls the internal shape (+0x18) and releases the lock; a refused write
lock reports through the Foundation error stream. Naked listing transcriptions of phys_fn_001099 to 001235; they are
wired into the handles' vtables in NpActor.cpp by the oracle tables' slots. Compare script:
docs/reconstruction/novodex-physics/evidence/phase3-shape-handle-rows-listing-compare.py."""
