# The shared oracle-to-candidate table for the remaining-rows pass (ABI-checked by hand: thiscall rows to QAE/QBE
# members or __fastcall(ecx, edx, ...) functions; cdecl rows to cdecl).
SYMBOLS = {
 0x5b700: '?lock@ReadWriteLock@@QAE_NXZ', 0x5b730: '?tryLock@ReadWriteLock@@QAE_NXZ', 0x5b790: '?unlock@ReadWriteLock@@QAE_NXZ',
 0xb4000: '?nxGetSdkAllocator@@YAPAVSdkAllocator@@XZ', 0xdef0: '?getNbScenes@PhysicsSDK@@QBEIXZ', 0xdf00: '?getScene@PhysicsSDK@@QAEPAVScene@@I@Z',
 0xb4de0: '?resize@SdkContainer@@QAE_NI@Z', 0xb4f50: '?empty@SdkContainer@@QAEXXZ', 0xb4d70: '??0SdkContainer@@QAE@XZ',
 0xe32c0: '??0RadixSort@IceCore@@QAE@XZ', 0xe32e0: '??1RadixSort@IceCore@@QAE@XZ', 0xe3920: '?Sort@RadixSort@IceCore@@QAEAAV12@PBMI@Z',
 0xe33c0: '?Sort@RadixSort@IceCore@@QAEAAV12@PBIIW4RadixHint@2@@Z',
 0xef270: '??0NxPrunerProcessPool@@QAE@XZ', 0xef3d0: '?add@NxPrunerProcessPool@@QAEIPAUNxPrunerRegistration@@@Z',
 0xef5c0: '?remove@NxPrunerProcessPool@@QAEXI@Z', 0xef660: '?nxOpcodeReleasePool@@YAXXZ',
 0xf48c0: '??2@YAPAXI@Z', 0xf48bb: '_free', 0xf4669: '_atexit', 0xf47b0: '__chkstk', 0xf7a3c: '?nxPOFtol@@YAXXZ',
 0xf4251: '_fopen', 0xf42b0: '_fclose', 0xf453d: '_fwrite', 0xf7806: '_fseek', 0xf7736: '_ftell',
 0xf1500: '??1AABBTree@Opcode@@QAE@XZ', 0xf15a0: '??1Pruner@@UAE@XZ', 0xf1550: '??0Pruner@@QAE@XZ',
 0xe5110: '?BuildTree@StaticPruner@@QAE_NXZ', 0xb55b0: '?UpdateWorldAABB@Prunable@@QAEXPAVAABB@IceMaths@@@Z',
 0xe2d20: '?Add@AABB@IceMaths@@QAEAAV12@ABV12@@Z', 0xe2e50: '?MakeCube@AABB@IceMaths@@QBEMAAV12@@Z',
 0xf00c0: '?AddObject@PruningPool@@QAE_NPAVPrunable@@@Z', 0xf02a0: '?RemoveObject@PruningPool@@QAEXPAVPrunable@@@Z',
 0xf1600: '?nxSegmentAABB@@YA_NABVSegment@IceMaths@@ABVPoint@2@1@Z', 0xf17c0: '?nxRayAABB@@YA_NABVRay@IceMaths@@ABVPoint@2@1@Z',
 0xb3a60: '?storeByte@MemoryStream@@QAEXE@Z', 0xb3aa0: '?readByte@MemoryStream@@QAEEXZ', 0xb3ac0: '?readDword@MemoryStream@@QAEIXZ',
 0xb3f00: '?storeDword@MemoryStream@@QAEXI@Z',
}
DATA = {0x10123c04: 'nxSdkInstanceSlot', 0x10123c10: 'gNxApiReentry', 0x10122050: 'gNxApiReentryMessage',
        0x10128460: 'nxKnCompleteSorter', 0x10128464: 'nxKnBipartiteSorter0', 0x10128468: 'nxKnBipartiteSorter1',
        0x1012846c: 'nxKnPrunerPool', 0x10128470: 'nxKnOwnerQuery', 0x10128474: 'nxKnOwnerNotify', 0x10128478: 'nxKnOwnerWorldAABB',
        0x10128500: 'nxKnAdapterNotify'}
DATADECL = ['extern "C" void* nxSdkInstanceSlot;', '#pragma comment(linker, "/alternatename:_nxSdkInstanceSlot=?instance@PhysicsSDK@@2PAV1@A")',
 'extern "C" unsigned gNxApiReentry;', 'extern "C" const char* gNxApiReentryMessage;']
for v, s in (('nxKnCompleteSorter', '?gCompleteSorter@@3PAVRadixSort@IceCore@@A'),
             ('nxKnBipartiteSorter0', '?gBipartiteSorter0@@3PAVRadixSort@IceCore@@A'),
             ('nxKnBipartiteSorter1', '?gBipartiteSorter1@@3PAVRadixSort@IceCore@@A'),
             ('nxKnPrunerPool', '?gNxPrunerProcessPool@@3PAVNxPrunerProcessPool@@A'),
             ('nxKnOwnerQuery', '?gPrunableOwnerQuery@@3P6AIPAX@ZA'),
             ('nxKnOwnerNotify', '?gPrunableOwnerNotify@@3P6AXPAXPAVAABB@IceMaths@@@ZA'),
             ('nxKnOwnerWorldAABB', '?gPrunableOwnerWorldAABB@@3P6AXPAXPAVAABB@IceMaths@@@ZA'),
             ('nxKnAdapterNotify', '?gPrunableAdapterNotify@@3P6AXPAVPrunable@@PAVAABB@IceMaths@@@ZA')):
    DATADECL += ['extern "C" void* %s;' % v, '#pragma comment(linker, "/alternatename:_%s=%s")' % (v, s)]
INCLUDES = ['NxPhysics.h']
