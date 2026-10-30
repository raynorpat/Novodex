"""The penetration map's cell-run coder rows (Physics/src/PMapRows.cpp: phys_fn_001988, 001992 to 002006, 002008 with 002010 to 002014, 002031, and 005159 as a helper): compare the built candidate functions with the oracle listing, instruction by instruction.

Usage (from the repository root, after building NxPhysics Release Win32):

    python docs/reconstruction/novodex-physics/evidence/phase4-pmap-rows-listing-compare.py build/Release/NxPhysics.dll build/Release/NxPhysics.map
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import listing_compare_lib as lib

ROWS = [('nxPMapCoderReset', 0x4cba0, 0x4cc55),
        ('nxPMapEncodeCellRun', 0x4cee0, 0x4db9e),
        ('nxPMapDecodeCellRun', 0x4dba0, 0x4e112),
        ('nxPMapScalarDelete', 0x4ff20, 0x4ff71),
        ('nxPMapRelease5159', 0xe32e0, 0xe3328)]
EXPECTED = {0xb3a60: '?storeByte@MemoryStream@@QAEXE@Z',
            0xb3aa0: '?readByte@MemoryStream@@QAEEXZ',
            0xb4000: '?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ',
            0xb4de0: '?resize@SdkContainer@@QAE_NI@Z',
            0xb4f50: '?empty@SdkContainer@@QAEXXZ',
            0xe32e0: 'nxPMapRelease5159',
            0xf48bb: '_free',
            0x107f40: '??_7PenetrationMap@@6B@'}
TABLES = {0x1004e114: 32}
DATA = []
STATIC_RANGES = [(0x101222e8, 0x101222f4, 'nxPMapCursor'), (0x101240a0, 0x10124120, 'nxPMapCodeCounts')]
lib.run(ROWS, EXPECTED, TABLES, DATA, static_ranges=STATIC_RANGES)
