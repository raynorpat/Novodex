"""The bounded dynamic pruner's loose octree (Physics/src/opcode/PrunerOctree.cpp): phys_fn_005295, 005303, 005305,
005307, 005309, 005313, 005315, 005317 and 005318, and the queries over it (005545 to 005560), with the two AABB helpers they inline (000923, 005145): compare the
built candidate functions with the oracle listing, instruction by instruction. The octree's jump table is a compare
chain in the candidate; listing_compare_lib collapses it and checks its targets against the table.

Usage (from the repository root, after building NxPhysics Release Win32):

    python docs/reconstruction/novodex-physics/evidence/phase4-pruner-octree-listing-compare.py build/Release/NxPhysics.dll build/Release/NxPhysics.map
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import listing_compare_lib as lib

ROWS = [('nxPQSphereAABB', 0xf1970, 0xf1a22), ('nxPQAppendNode', 0xf1a30, 0xf1a68), ('nxPQAppendSubtree', 0xf1a70, 0xf1ac0),
        ('nxPQPlaneWalk', 0xf1ac0, 0xf1c5c), ('nxPQPlaneQuery', 0xf1c60, 0xf1ca6), ('nxPQSegmentWalk', 0xf1cb0, 0xf1d0d),
        ('nxPQSegmentQuery', 0xf1d10, 0xf1d41), ('nxPQRayWalk', 0xf1d50, 0xf1dad), ('nxPQRayQuery', 0xf1db0, 0xf1de1),
        ('nxPOSetCenterExtents', 0x203c0, 0x20438), ('nxPOIsInside', 0xe2f70, 0xe2fca), ('nxPOScaleBounds', 0xe7250, 0xe7329),
        ('nxPOSetNodeBounds', 0xe7380, 0xe7544), ('nxPOWalk', 0xe7570, 0xe75c9), ('nxPOLeafCallback', 0xe75d0, 0xe75e5),
        ('nxPORemove', 0xe75f0, 0xe7665), ('nxPOForEach', 0xe7690, 0xe76b1), ('nxPOBuild', 0xe76c0, 0xe7847),
        ('nxPOInsert', 0xe7850, 0xe7b80), ('nxPOUpdate', 0xe7b80, 0xe7c08)]
EXPECTED = {0xb4000: '?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ', 0xe2e50: '?MakeCube@AABB@IceMaths@@QBEMAAV12@@Z',
            0xf7a3c: '__ftol2', 0x203c0: 'nxPOSetCenterExtents', 0xe2f70: 'nxPOIsInside', 0xe7250: 'nxPOScaleBounds',
            0xe7380: 'nxPOSetNodeBounds', 0xe7570: 'nxPOWalk', 0xe75d0: 'nxPOLeafCallback', 0xe75f0: 'nxPORemove',
            0xe7850: 'nxPOInsert', 0xb4de0: '?resize@SdkContainer@@QAE_NI@Z', 0xf1600: '?nxSegmentAABB@@YA_NABVSegment@IceMaths@@ABVPoint@2@1@Z',
            0xf17c0: '?nxRayAABB@@YA_NABVRay@IceMaths@@ABVPoint@2@1@Z', 0xf1a30: 'nxPQAppendNode', 0xf1a70: 'nxPQAppendSubtree', 0xf1ac0: 'nxPQPlaneWalk',
            0xf1cb0: 'nxPQSegmentWalk', 0xf1d50: 'nxPQRayWalk'}
TABLES = {0x100e7544: 8}
DATA = [(0x10128500, 'gPrunableAdapterNotify', 'adapter_notify')]
lib.run(ROWS, EXPECTED, TABLES, DATA)
