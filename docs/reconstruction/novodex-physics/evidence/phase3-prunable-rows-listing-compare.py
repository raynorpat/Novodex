"""The pruning rows of Physics/src/opcode/PrunableRows.cpp (phys_fn_001935, and 001945 with 001947): compare the built candidate functions with the oracle listing, instruction by instruction.

Usage (from the repository root, after building NxPhysics Release Win32):

    python docs/reconstruction/novodex-physics/evidence/phase3-prunable-rows-listing-compare.py build/Release/NxPhysics.dll build/Release/NxPhysics.map
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import listing_compare_lib as lib

ROWS = [('nxPrunerVisualizeNodeCallback', 0x4b9c0, 0x4b9ee),
        ('nxPrunerOwnerDestroyRow', 0x4bbd0, 0x4bc79)]
EXPECTED = {0x14f0: '?getNbShapes@JointActorBody@@QBEIXZ',
            0x1520: '?getShapes@JointActorBody@@QAEPAPAXXZ',
            0xdeb0: '?getDebugRenderable@PhysicsSDK@@QAEPAVNxDebugRenderable@@XZ',
            0xb4000: '?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ',
            0xb5260: '?Dispatch@PrunableOwnerDispatcher@@QAEIPAX@Z',
            0xe71b0: '??1SweepAndPrune@Opcode@@QAE@XZ'}
TABLES = {}
DATA = [(0x10123c04, '?instance@PhysicsSDK@@2PAV1@A', 'sdk_instance')]
STATIC_RANGES = []
lib.run(ROWS, EXPECTED, TABLES, DATA, static_ranges=STATIC_RANGES)
