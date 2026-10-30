"""Matrix A [MESH][MESH], phys_fn_001876 (Physics/src/ContactMeshMeshRow.cpp): compare the built candidate function with the oracle listing, instruction by instruction.

Usage (from the repository root, after building NxPhysics Release Win32):

    python docs/reconstruction/novodex-physics/evidence/phase3-mesh-mesh-contact-listing-compare.py build/Release/NxPhysics.dll build/Release/NxPhysics.map
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import listing_compare_lib as lib

ROWS = [('nxMeshMeshContactRow', 0x46ab0, 0x470ed)]
EXPECTED = {0x1d610: '?NxEmitContactThiscall@@YIXPAUNxContactSink@@IPAX1IPBVNxVec3@@2GG@Z',
            0x411a0: '?NxContactConvexConvex@@YAXPBUNxCollisionShape@@0PAUNxContactSink@@PAX@Z',
            0x432d0: '?NxContactConvexHeightfield@@YAXPBUNxCollisionShape@@0PAUNxContactSink@@PAX@Z',
            0x44500: '?NxContactConvexMesh@@YAXPBUNxCollisionShape@@0PAUNxContactSink@@PAX@Z',
            0x46510: '_nxMeshHeightfieldContact',
            0x46780: 'nxMeshMeshSphereCallback',
            0x47e50: '_nxMeshPMapContacts',
            0xd12b0: '?CollideNoLeafCallback@NxCallbackTreeCollider@Opcode@@QAE_NP6AXPBVAABBNoLeafNode@2@0@ZHPBVModel@2@2PBVMatrix4x4@IceMaths@@3@Z'}
TABLES = {}
DATA = [(0x10123d78, '_nxTask2lSphereMatrixA', 'matrix_a'), (0x10123d7c, '_nxTask2lSphereMatrixB', 'matrix_b'), (0x10123d80, 'nxMeshMeshStatic80', 'static80'), (0x10123d88, 'nxMeshMeshStatic88', 'static88')]
STATIC_RANGES = [(0x10123d8c, 0x10123d90, '_nxMeshContactCount'), (0x10123d90, 0x10123d9c, '_nxMeshContactSums'), (0x10123da0, 0x10123f20, '_nxMeshContactNormals'), (0x10123f20, 0x101240a0, '_nxMeshContactVertices'), (0x10123cf8, 0x10123d78, '_nxMeshContactNormalAndMaterial'), (0x10123b18, 0x10123c04, 'gParameter@@')]
lib.run(ROWS, EXPECTED, TABLES, DATA, static_ranges=STATIC_RANGES)
