SYMBOLS = {0x5b700: '?lock@ReadWriteLock@@QAE_NXZ', 0x5b730: '?tryLock@ReadWriteLock@@QAE_NXZ', 0x5b790: '?unlock@ReadWriteLock@@QAE_NXZ',
           0xb4000: '?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ',
           0xdef0: '?getNbScenes@PhysicsSDK@@QBEIXZ', 0xdf00: '?getScene@PhysicsSDK@@QAEPAVScene@@I@Z'}
DATA = {0x10123c04: 'nxSdkInstanceSlot', 0x10123c10: 'gNxApiReentry', 0x10122050: 'gNxApiReentryMessage'}
DATADECL = ['extern "C" void* nxSdkInstanceSlot;', '#pragma comment(linker, "/alternatename:_nxSdkInstanceSlot=?instance@PhysicsSDK@@2PAV1@A")',
            'extern "C" unsigned gNxApiReentry;', 'extern "C" const char* gNxApiReentryMessage;']
INCLUDES = ['NxPhysics.h']
