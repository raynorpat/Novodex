SYMBOLS = {0x16c00: '?row000738@Row000738Fixture@@QAEPAXXZ',
           0xb55b0: '?UpdateWorldAABB@Prunable@@QAEXPAVAABB@IceMaths@@@Z',
           0x26bd0: '?nxBaseDtor@ShapeBase@@QAEXXZ'}
DATA = {0x10123bc8: 'gParameter + 0xb0', 0x10123b4c: 'gParameter + 0x34', 0x10123bd8: 'gParameter + 0xc0',
        0x10123c10: 'gNxApiReentry', 0x10122050: 'gNxApiReentryMessage'}
DATADECL = ['extern NxReal gParameter[];', 'extern "C" unsigned gNxApiReentry;', 'extern "C" const char* gNxApiReentryMessage;']
INCLUDES = ['NxPhysics.h']
HEAD_COMMENT = """ShapeRows.cpp: the internal shape's own rows that the public shape handles and the scene reach (Shape.cpp in the
oracle's source tree): phys_fn_001303 to 001345 less those reconstructed in ObjectModel.cpp. Naked listing
transcriptions; the SDK parameter array (.data 0x10123b18, gParameter), the API reentry flag and its message pointer
(.data 0x10123c10, 0x10122050) are the candidate's own variables by name. Compare script:
docs/reconstruction/novodex-physics/evidence/phase3-shape-rows-listing-compare.py."""
