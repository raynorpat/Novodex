"""All of Physics/src/opcode/PrunerOctree.cpp: the bounded dynamic pruner's loose octree (phys_fn_005295 to 005318), the queries over it (005545 to 005560),
the pruner cluster (005223, 005234 to 005260, 005301, 005320, 002148, the sphere-query walk and entry, the plane-cull walk, the CRT conversion and the five
static-initialiser thunks), with the AABB helpers they inline (000923, 005145): compare the built candidate functions
with the oracle listing, instruction by instruction. The octree's jump table is a compare chain in the candidate;
listing_compare_lib collapses it and checks its targets against the table.

Usage (from the repository root, after building NxPhysics Release Win32):

    python docs/reconstruction/novodex-physics/evidence/phase4-pruner-octree-listing-compare.py build/Release/NxPhysics.dll build/Release/NxPhysics.map
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import listing_compare_lib as lib

ROWS = [('nxPOFtol', 0xf7a3c, 0xf7ab1), ('nxPQSphereWalk', 0xf1df0, 0xf204d), ('nxPQSphereQuery', 0xf2050, 0xf2089), ('nxPCBoxEmpty', 0x53810, 0x53872),
        ('nxPCOctreeCtor', 0xe7360, 0xe737a), ('nxPCOctreeRelease', 0xe7c10, 0xe7c3c), ('nxPCCollectCallback', 0xe52d0, 0xe53de),
        ('nxPCTreeCull', 0xf1350, 0xf14f2), ('nxPCThunkA', 0x103050, 0x10305a), ('nxPCThunkB', 0x103060, 0x10306a),
        ('nxPCThunkC', 0x103070, 0x10307a), ('nxPCThunkD', 0x103080, 0x10308a), ('nxPCThunkE', 0x103090, 0x10309a),
        ('nxPCStaticDtorBody', 0xe5740, 0xe577d), ('nxPCStaticPlanes', 0xe5780, 0xe580d), ('nxPCStaticScalarDelete', 0xe5810, 0xe5866),
        ('nxPCBoundedCtor', 0xe5870, 0xe5889), ('nxPCBuildTree', 0xe58a0, 0xe59f5), ('nxPCAddObject', 0xe5a00, 0xe5a28),
        ('nxPCRemoveObject', 0xe5a30, 0xe5a8d), ('nxPCUpdateObject', 0xe5a90, 0xe5aac), ('nxPCQueryPlanes', 0xe5ab0, 0xe5dd7),
        ('nxPCQueryRays', 0xe5de0, 0xe6059), ('nxPCQuerySphere', 0xe6060, 0xe622c),
        ('nxPQSphereAABB', 0xf1970, 0xf1a22), ('nxPQAppendNode', 0xf1a30, 0xf1a68), ('nxPQAppendSubtree', 0xf1a70, 0xf1ac0),
        ('nxPQPlaneWalk', 0xf1ac0, 0xf1c5c), ('nxPQPlaneQuery', 0xf1c60, 0xf1ca6), ('nxPQSegmentWalk', 0xf1cb0, 0xf1d0d),
        ('nxPQSegmentQuery', 0xf1d10, 0xf1d41), ('nxPQRayWalk', 0xf1d50, 0xf1dad), ('nxPQRayQuery', 0xf1db0, 0xf1de1),
        ('nxPOSetCenterExtents', 0x203c0, 0x20438), ('nxPOIsInside', 0xe2f70, 0xe2fca), ('nxPOScaleBounds', 0xe7250, 0xe7329),
        ('nxPOSetNodeBounds', 0xe7380, 0xe7544), ('nxPOWalk', 0xe7570, 0xe75c9), ('nxPOLeafCallback', 0xe75d0, 0xe75e5),
        ('nxPORemove', 0xe75f0, 0xe7665), ('nxPOForEach', 0xe7690, 0xe76b1), ('nxPOBuild', 0xe76c0, 0xe7847),
        ('nxPOInsert', 0xe7850, 0xe7b80), ('nxPOUpdate', 0xe7b80, 0xe7c08)]
EXPECTED = {0xb4000: '?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ', 0xe2e50: '?MakeCube@AABB@IceMaths@@QBEMAAV12@@Z',
            0xf7a3c: 'nxPOFtol', 0x203c0: 'nxPOSetCenterExtents', 0xe2f70: 'nxPOIsInside', 0xe7250: 'nxPOScaleBounds',
            0xe7380: 'nxPOSetNodeBounds', 0xe7570: 'nxPOWalk', 0xe75d0: 'nxPOLeafCallback', 0xe75f0: 'nxPORemove',
            0xe7850: 'nxPOInsert', 0xe76c0: 'nxPOBuild', 0xe7b80: 'nxPOUpdate', 0xb4de0: '?resize@SdkContainer@@QAE_NI@Z', 0xf1600: '?nxSegmentAABB@@YA_NABVSegment@IceMaths@@ABVPoint@2@1@Z',
            0xf17c0: '?nxRayAABB@@YA_NABVRay@IceMaths@@ABVPoint@2@1@Z', 0xf1a30: 'nxPQAppendNode', 0xf1a70: 'nxPQAppendSubtree', 0xf1ac0: 'nxPQPlaneWalk',
            0xf1cb0: 'nxPQSegmentWalk', 0xf1d50: 'nxPQRayWalk',
            0xf1df0: 'nxPQSphereWalk', 0xf1970: 'nxPQSphereAABB', 0xf1c60: 'nxPQPlaneQuery', 0xf1d10: 'nxPQSegmentQuery', 0xf1db0: 'nxPQRayQuery',
            0xf2050: 'nxPQSphereQuery', 0x53810: 'nxPCBoxEmpty', 0xe7360: 'nxPCOctreeCtor', 0xe7c10: 'nxPCOctreeRelease', 0xf1350: 'nxPCTreeCull',
            0xe52d0: 'nxPCCollectCallback', 0xb4f50: '?empty@SdkContainer@@QAEXXZ', 0xb4d70: '??0SdkContainer@@QAE@XZ', 0xf1500: '??1AABBTree@Opcode@@QAE@XZ',
            0xf15a0: '??1Pruner@@UAE@XZ', 0xf1550: '??0Pruner@@QAE@XZ', 0xe5110: '?BuildTree@StaticPruner@@QAE_NXZ',
            0xb55b0: '?UpdateWorldAABB@Prunable@@QAEXPAVAABB@IceMaths@@@Z', 0xe2d20: '?Add@AABB@IceMaths@@QAEAAV12@ABV12@@Z',
            0xf00c0: '?AddObject@PruningPool@@QAE_NPAVPrunable@@@Z', 0xf02a0: '?RemoveObject@PruningPool@@QAEXPAVPrunable@@@Z', 0xf4669: '_atexit',
            0xe58a0: 'nxPCBuildTree', 0xe6130: 'nxPCQuerySphere', 0x103050: 'nxPCThunkA', 0x103060: 'nxPCThunkB', 0x103070: 'nxPCThunkC',
            0x103080: 'nxPCThunkD', 0x103090: 'nxPCThunkE', 0xe5920: 'nxPCBuildTree'}
TABLES = {0x100e7544: 8}
DATA = [(0x10128500, 'gPrunableAdapterNotify', 'adapter_notify')]
STATIC_RANGES = [(0x1012847c, 0x101284d8, 'nxPCStatics')]
lib.run(ROWS, EXPECTED, TABLES, DATA, static_ranges=STATIC_RANGES)
