"""Phase 6's remaining rows and their helpers (Physics/src/core/Phase6Rows.cpp): compare the built candidate functions with the oracle listing, instruction by instruction.

Usage (from the repository root, after building NxPhysics Release Win32):

    python docs/reconstruction/novodex-physics/evidence/phase6-rows-listing-compare.py build/Release/NxPhysics.dll build/Release/NxPhysics.map
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import listing_compare_lib as lib

ROWS = [('nxP64159', 0x9ab00, 0x9aba7),
        ('nxP64161', 0x9abb0, 0x9ac92),
        ('nxP64169', 0x9adb0, 0x9b0d0),
        ('nxP64248', 0xa0f60, 0xa0f63),
        ('nxP64401', 0xafcc0, 0xaffda),
        ('nxP64405', 0xb02e0, 0xb030e),
        ('nxP64407', 0xb0310, 0xb035f),
        ('nxP64409', 0xb0360, 0xb0399),
        ('nxP64411', 0xb03a0, 0xb03a3),
        ('nxP64413', 0xb03b0, 0xb0548),
        ('nxP64415', 0xb0550, 0xb0572),
        ('nxP64537', 0xb15e0, 0xb15ff),
        ('nxP64759', 0xb38a0, 0xb38c5),
        ('nxP64761', 0xb38d0, 0xb3917),
        ('nxP64763', 0xb3920, 0xb3980),
        ('nxP64764', 0xb3980, 0xb39a8)]
EXPECTED = {0x99ab0: '?row_slot6@Joint@@UAEXM@Z',
            0x9ab00: 'nxP64159',
            0x9abb0: 'nxP64161',
            0xb02e0: 'nxP64405',
            0xb0310: 'nxP64407',
            0xb0360: 'nxP64409',
            0xb38a0: 'nxP64759',
            0xf41f0: '_free',
            0x11a680: '??_7NxJoint@@6B@'}
TABLES = {}
DATA = []
STATIC_RANGES = [(0x10123b18, 0x10123c04, 'gParameter@@')]
lib.run(ROWS, EXPECTED, TABLES, DATA, static_ranges=STATIC_RANGES)
