SYMBOLS = {0xb3a60: '?storeByte@MemoryStream@@QAEXE@Z', 0xb3aa0: '?readByte@MemoryStream@@QAEEXZ', 0xb4f50: '?empty@SdkContainer@@QAEXXZ', 0xb4de0: '?resize@SdkContainer@@QAE_NI@Z', 0xf48bb: '_free', 0x5b700: '?lock@ReadWriteLock@@QAE_NXZ', 0x5b730: '?tryLock@ReadWriteLock@@QAE_NXZ', 0x5b790: '?unlock@ReadWriteLock@@QAE_NXZ',
           0xb4000: '?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ',
           0xdef0: '?getNbScenes@PhysicsSDK@@QBEIXZ', 0xdf00: '?getScene@PhysicsSDK@@QAEPAVScene@@I@Z'}
DATA = {0x10123c04: 'nxSdkInstanceSlot', 0x10123c10: 'gNxApiReentry', 0x10122050: 'gNxApiReentryMessage'}
DATADECL = ['extern "C" void* nxSdkInstanceSlot;', '#pragma comment(linker, "/alternatename:_nxSdkInstanceSlot=?instance@PhysicsSDK@@2PAV1@A")',
            'extern "C" unsigned gNxApiReentry;', 'extern "C" const char* gNxApiReentryMessage;']
INCLUDES = ['NxPhysics.h']
DATA.update({269624040: 'nxPMapCursor', 269624044: 'nxPMapCursor + 4', 269624048: 'nxPMapCursor + 8', 269631648: 'nxPMapCodeCounts', 269631652: 'nxPMapCodeCounts + 4', 269631656: 'nxPMapCodeCounts + 8', 269631660: 'nxPMapCodeCounts + 12', 269631664: 'nxPMapCodeCounts + 16', 269631668: 'nxPMapCodeCounts + 20', 269631672: 'nxPMapCodeCounts + 24', 269631676: 'nxPMapCodeCounts + 28', 269631680: 'nxPMapCodeCounts + 32', 269631684: 'nxPMapCodeCounts + 36', 269631688: 'nxPMapCodeCounts + 40', 269631692: 'nxPMapCodeCounts + 44', 269631696: 'nxPMapCodeCounts + 48', 269631700: 'nxPMapCodeCounts + 52', 269631704: 'nxPMapCodeCounts + 56', 269631708: 'nxPMapCodeCounts + 60', 269631712: 'nxPMapCodeCounts + 64', 269631716: 'nxPMapCodeCounts + 68', 269631720: 'nxPMapCodeCounts + 72', 269631724: 'nxPMapCodeCounts + 76', 269631728: 'nxPMapCodeCounts + 80', 269631732: 'nxPMapCodeCounts + 84', 269631736: 'nxPMapCodeCounts + 88', 269631740: 'nxPMapCodeCounts + 92', 269631744: 'nxPMapCodeCounts + 96', 269631748: 'nxPMapCodeCounts + 100', 269631752: 'nxPMapCodeCounts + 104', 269631756: 'nxPMapCodeCounts + 108', 269631760: 'nxPMapCodeCounts + 112', 269631764: 'nxPMapCodeCounts + 116', 269631768: 'nxPMapCodeCounts + 120', 269631772: 'nxPMapCodeCounts + 124'})
DATADECL = DATADECL + ['// .data 0x101222e8 (three words, the cell cursor) and 0x101240a0 (32 words), the cell-run coder statics.',
  'static unsigned nxPMapCursor[3];', 'static unsigned nxPMapCodeCounts[32];']
NAMES = {'phys_fn_001988': 'nxPMapCoderReset', 'phys_fn_001992': 'nxPMapEncodeCellRun', 'phys_fn_002010': 'nxPMapDecodeCells', 'phys_fn_002031': 'nxPMapScalarDelete'}
HEAD_COMMENT = '''PMapRows.cpp: the penetration map's cell-run coder as the oracle has it, over .data statics (the cell cursor at
0x101222e8 and 32 words at 0x101240a0) rather than the candidate PMap.cpp's cursor struct: phys_fn_001988 (the
statics reset), 001992 to 002006 (the encoder, one function the inventory splits eight ways), 002010 to 002014 (the
cell decoder, three ways) and 002031 (the scalar deleting destructor). With two helpers the rows call that the
candidate has only in other shapes: the tail of 002008 from its loop at 0x0004dc22 (002010 calls that label) and
005159 as a thiscall. Naked listing transcriptions; PMap.cpp keeps its own coder, so nothing calls them yet.
Compare script: docs/reconstruction/novodex-physics/evidence/phase4-pmap-rows-listing-compare.py.'''
