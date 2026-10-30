"""The public shape handles' rows (Physics/src/NpShapeHandleRows.cpp, phys_fn_001099 to 001235): compare the built candidate functions with the oracle listing, instruction by instruction.

Usage (from the repository root, after building NxPhysics Release Win32):

    python docs/reconstruction/novodex-physics/evidence/phase3-shape-handle-rows-listing-compare.py build/Release/NxPhysics.dll build/Release/NxPhysics.map
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import listing_compare_lib as lib

ROWS = [('nxFac1099', 0x23940, 0x23967),
        ('nxFac1101', 0x23970, 0x23997),
        ('nxFac1103', 0x239a0, 0x239f4),
        ('nxFac1105', 0x23a00, 0x23a54),
        ('nxFac1131', 0x23da0, 0x23df9),
        ('nxFac1133', 0x23e00, 0x23e54),
        ('nxFac1135', 0x23e60, 0x23eb4),
        ('nxFac1137', 0x23ec0, 0x23f14),
        ('nxFac1139', 0x23f20, 0x23f74),
        ('nxFac1141', 0x23f80, 0x23fd4),
        ('nxFac1143', 0x23fe0, 0x24034),
        ('nxFac1145', 0x24040, 0x24094),
        ('nxFac1147', 0x240a0, 0x240f4),
        ('nxFac1151', 0x24130, 0x2415b),
        ('nxFac1153', 0x24160, 0x2418b),
        ('nxFac1157', 0x241f0, 0x24249),
        ('nxFac1167', 0x24340, 0x24399),
        ('nxFac1181', 0x245e0, 0x24634),
        ('nxFac1211', 0x24990, 0x249e9),
        ('nxFac1213', 0x249f0, 0x24a44),
        ('nxFac1215', 0x24a50, 0x24aa4),
        ('nxFac1217', 0x24ab0, 0x24b04),
        ('nxFac1225', 0x24ba0, 0x24bf4),
        ('nxFac1227', 0x24c00, 0x24c54),
        ('nxFac1229', 0x24c60, 0x24cb4),
        ('nxFac1231', 0x24cc0, 0x24ce7),
        ('nxFac1233', 0x24cf0, 0x24d44),
        ('nxFac1235', 0x24d50, 0x24da4)]
EXPECTED = {0x24fc0: '?nxPlaneSetEquation@PlaneShape@@QAEXPBMM@Z',
            0x25c10: '?nxShp1307@@YIIPAX0I@Z',
            0x25f60: '?nxShp1309@@YIIPAX0I@Z',
            0x26280: '?nxShp1311@@YIIPAX0I@Z',
            0x26430: '?nxShp1313@@YIIPAX0I@Z',
            0x26ad0: '?nxShp1317@@YIIPAX0I@Z',
            0x26b70: '?nxShp1319@@YIIPAX0I@Z',
            0x26df0: '?nxShp1331@@YIIPAX0II@Z',
            0x26e30: '?nxShp1333@@YIIPAX0I@Z',
            0x26ec0: '?nxShp1335@@YIIPAX0I@Z',
            0x26f50: '?nxShp1337@@YIIPAX0I@Z',
            0x26fd0: '?nxShp1339@@YIIPAX0I@Z',
            0x27360: '?nxShp1341@@YIIPAX0I@Z',
            0x276f0: '?nxSetMaterialIndex@ShapeBase@@QAEXG@Z',
            0x5b700: '?lock@ReadWriteLock@@QAE_NXZ',
            0x5b730: '?tryLock@ReadWriteLock@@QAE_NXZ',
            0x5b790: '?unlock@ReadWriteLock@@QAE_NXZ',
            0x8aea0: '?setValue@SdkPointerBindingThunk@@QAE_NPAX@Z'}
TABLES = {}
DATA = [(0x10123c10, '_gNxApiReentry', 'api_reentry'), (0x10122050, '_gNxApiReentryMessage', 'api_reentry_message')]
STATIC_RANGES = []
lib.run(ROWS, EXPECTED, TABLES, DATA, static_ranges=STATIC_RANGES)
