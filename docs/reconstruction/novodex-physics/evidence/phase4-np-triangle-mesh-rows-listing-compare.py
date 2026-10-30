"""The public triangle mesh's rows (Physics/src/NpTriangleMeshRows.cpp, phys_fn_002095 to 002138): compare the built candidate functions with the oracle listing, instruction by instruction.

Usage (from the repository root, after building NxPhysics Release Win32):

    python docs/reconstruction/novodex-physics/evidence/phase4-np-triangle-mesh-rows-listing-compare.py build/Release/NxPhysics.dll build/Release/NxPhysics.map
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import listing_compare_lib as lib

ROWS = [('nxNpMesh2095', 0x52a60, 0x52b50),
        ('nxNpMesh2098', 0x52b50, 0x52c40),
        ('nxNpMesh2101', 0x52c40, 0x52d30),
        ('nxNpMesh2104', 0x52d30, 0x52dbe),
        ('nxNpMesh2108', 0x52dc0, 0x52e4e),
        ('nxNpMesh2112', 0x52e50, 0x52eec),
        ('nxNpMesh2116', 0x52ef0, 0x52f7e),
        ('nxNpMesh2120', 0x52f80, 0x5300a),
        ('nxNpMesh2124', 0x53010, 0x530ac),
        ('nxNpMesh2128', 0x530b0, 0x5314c),
        ('nxNpMesh2132', 0x53150, 0x531ec),
        ('nxNpMesh2136', 0x531f0, 0x5328c)]
EXPECTED = {0xdef0: '?getNbScenes@PhysicsSDK@@QBEIXZ',
            0xdf00: '?getScene@PhysicsSDK@@QAEPAVScene@@I@Z',
            0x5b700: '?lock@ReadWriteLock@@QAE_NXZ',
            0x5b730: '?tryLock@ReadWriteLock@@QAE_NXZ',
            0x5b790: '?unlock@ReadWriteLock@@QAE_NXZ'}
TABLES = {}
DATA = [(0x10123c04, '?instance@PhysicsSDK@@2PAV1@A', 'sdk_instance')]
STATIC_RANGES = []
lib.run(ROWS, EXPECTED, TABLES, DATA, static_ranges=STATIC_RANGES)
