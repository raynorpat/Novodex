"""OPCODE's vendored rows (External/opcode/novodex/OpcodeVendoredRows.cpp): compare the built candidate functions with the oracle listing, instruction by instruction.

Usage (from the repository root, after building NxPhysics Release Win32):

    python docs/reconstruction/novodex-physics/evidence/vendored-rows-opcode-listing-compare.py build/Release/NxPhysics.dll build/Release/NxPhysics.map
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import listing_compare_lib as lib

ROWS = [('??0Container@IceCore@@QAE@ABV01@@Z', 0xb4f00, 0xb4f49),
        ('?SphereTriOverlap@SphereCollider@Opcode@@IAEHABVPoint@IceMaths@@00@Z', 0xde240, 0xde7db),
        ('?ValidateSettings@OBBCollider@Opcode@@UAEPBDXZ', 0xe1530, 0xe1546),
        ('?Set@Plane@IceMaths@@QAEAAV12@ABVPoint@2@00@Z', 0xe31c0, 0xe32b5),
        ('?Area@Triangle@IceMaths@@QBEMXZ', 0xe3ed0, 0xe3f4e),
        ('?Normal@Triangle@IceMaths@@QBEXAAVPoint@2@@Z', 0xe3f50, 0xe401e),
        ('?Center@Triangle@IceMaths@@QBEXAAVPoint@2@@Z', 0xe4020, 0xe408b),
        ('?Inflate@Triangle@IceMaths@@QAEXM_N@Z', 0xe4090, 0xe4151),
        ('?CoFactor@Matrix4x4@IceMaths@@QBEMII@Z', 0xe42a0, 0xe439b),
        ('?Determinant@Matrix4x4@IceMaths@@QBEMXZ', 0xe43a0, 0xe43f3),
        ('?Invert@Matrix4x4@IceMaths@@QAEAAV12@XZ', 0xe4400, 0xe457b),
        ('?ComputeWorldEdgeNormal@OBB@IceMaths@@QBEXIAAVPoint@2@@Z', 0xe4cb0, 0xe4d2c),
        ('?Init@SAP_PairData@Opcode@@QAE_NI@Z', 0xe6c10, 0xe6c9e),
        ('nxOpModelDtor', 0xe90e0, 0xe90f6),
        ('?Build@Model@Opcode@@UAE_NABUOPCODECREATE@2@@Z', 0xe9100, 0xe9277),
        ('?ValidateSettings@PlanesCollider@Opcode@@UAEPBDXZ', 0xe1530, 0xe1546)]
EXPECTED = {0x213e0: '?ValidateSettings@VolumeCollider@Opcode@@UAEPBDXZ',
            0x539b0: '?opcNovodeXSetIceError@@YA_NPBD0H@Z',
            0xb4000: '?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ',
            0xb4e90: '?SetSize@Container@IceCore@@QAE_NI@Z',
            0xe4020: '?Center@Triangle@IceMaths@@QBEXAAVPoint@2@@Z',
            0xe42a0: '?CoFactor@Matrix4x4@IceMaths@@QBEMII@Z',
            0xe43a0: '?Determinant@Matrix4x4@IceMaths@@QBEMXZ',
            0xe8fb0: '?IsValid@MeshInterface@Opcode@@QBE_NXZ',
            0xe8fd0: '?CheckTopology@MeshInterface@Opcode@@QBEIXZ',
            0xe9060: '??0AABBTreeOfTrianglesBuilder@Opcode@@QAE@XZ',
            0xe9310: '?CreateTree@BaseModel@Opcode@@IAE_N_N0@Z',
            0xe9480: '?Release@Model@Opcode@@AAEXXZ',
            0xe9550: '??1BaseModel@Opcode@@UAE@XZ',
            0xf1060: '??0AABBTree@Opcode@@QAE@XZ',
            0xf10c0: '?Build@AABBTree@Opcode@@QAE_NPAVAABBTreeBuilder@2@@Z',
            0xf1500: '??1AABBTree@Opcode@@QAE@XZ',
            0x11bac8: '??_7Model@Opcode@@6B@'}
TABLES = {}
DATA = []
STATIC_RANGES = [(0x10122520, 0x101225b0, 'nxOpEdgeNormals')]
lib.run(ROWS, EXPECTED, TABLES, DATA, static_ranges=STATIC_RANGES)
