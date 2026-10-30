"""The internal shape's rows (Physics/src/ShapeRows.cpp, phys_fn_001303 to 001345): compare the built candidate functions with the oracle listing, instruction by instruction.

Usage (from the repository root, after building NxPhysics Release Win32):

    python docs/reconstruction/novodex-physics/evidence/phase3-shape-rows-listing-compare.py build/Release/NxPhysics.dll build/Release/NxPhysics.map
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import listing_compare_lib as lib

ROWS = [('nxShp1303', 0x258f0, 0x25952),
        ('nxShp1305', 0x25960, 0x25c0d),
        ('nxShp1307', 0x25c10, 0x25f5e),
        ('nxShp1309', 0x25f60, 0x2627d),
        ('nxShp1311', 0x26280, 0x26428),
        ('nxShp1313', 0x26430, 0x26699),
        ('nxShp1317', 0x26ad0, 0x26b6d),
        ('nxShp1319', 0x26b70, 0x26b99),
        ('nxShp1321', 0x26ba0, 0x26bc8),
        ('nxShp1325', 0x26c90, 0x26d83),
        ('nxShp1331', 0x26df0, 0x26e25),
        ('nxShp1333', 0x26e30, 0x26ebb),
        ('nxShp1335', 0x26ec0, 0x26f41),
        ('nxShp1337', 0x26f50, 0x26fc5),
        ('nxShp1339', 0x26fd0, 0x27359),
        ('nxShp1341', 0x27360, 0x276e7),
        ('nxShp1345', 0x27710, 0x27732)]
EXPECTED = {0x16c00: '?row000738@Row000738Fixture@@QAEPAXXZ',
            0x25f60: 'nxShp1309',
            0x26280: 'nxShp1311',
            0x26430: 'nxShp1313',
            0x26bd0: '?nxBaseDtor@ShapeBase@@QAEXXZ',
            0x26c90: 'nxShp1325',
            0xb55b0: '?UpdateWorldAABB@Prunable@@QAEXPAVAABB@IceMaths@@@Z'}
TABLES = {}
DATA = [(0x10123c10, '_gNxApiReentry', 'api_reentry'), (0x10122050, '_gNxApiReentryMessage', 'api_reentry_message')]
STATIC_RANGES = [(0x10123b18, 0x10123c04, 'gParameter@@')]
lib.run(ROWS, EXPECTED, TABLES, DATA, static_ranges=STATIC_RANGES)
