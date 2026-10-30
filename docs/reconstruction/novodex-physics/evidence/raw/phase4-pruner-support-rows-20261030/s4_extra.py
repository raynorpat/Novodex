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
SYMBOLS.update({0xe2f70: '?nxPOIsInside@@YI_NPBXPAX0@Z', 0x203c0: '?nxPOSetCenterExtents@@YIXPAX0PBX1@Z',
                0xb39b0: '?appendBlock@MemoryStream@@AAE_NPAUMemoryStreamBlock@@I@Z', 0xb3b50: '?initBlock@MemoryStream@@AAE_NIPBXII@Z',
                0xb3c70: '?flushBits@MemoryStream@@QAEXXZ'})
for k in range(32):
    DATA[0x1012847c + 4 * k] = 'nxPCStatics + %d' % (4 * k) if k else 'nxPCStatics'
DATADECL = DATADECL + ['extern unsigned nxPCStatics[32];']
PUSHES = {0x101284d8: 'nxPCStatics + 92', 0x101284e8: 'nxPCStatics + 108', 0x10128500 - 0x10000000 + 0x10000000: 'nxKnAdapterNotify',
          0x1011b9c8: 'nxKnVtStatic', 0x1011b9f0: 'nxKnVtBounded', 0x1011bc18: 'nxKnVtPruner', 0x1011bbc0: 'nxKnVtDynamic',
          0x1011bb98: 'nxKnVtType3'}
for v, s in (('nxKnVtStatic', '??_7StaticPruner@@6B@'), ('nxKnVtBounded', '??_7BoundedDynamicPruner@@6B@'),
             ('nxKnVtPruner', '??_7Pruner@@6B@'), ('nxKnVtDynamic', '??_7DynamicPruner@@6B@'), ('nxKnVtType3', '??_7Type3StaticPruner@@6B@')):
    DATADECL += ['extern "C" void* %s;' % v, '#pragma comment(linker, "/alternatename:_%s=%s")' % (v, s)]
NAMES = {'phys_fn_006332': 'nxP4ThunkF', 'phys_fn_006334': 'nxP4ThunkG'}
HEAD_COMMENT = "x"
PUSHES.update({0x1011b784: 'nxKnVtSphereCollider', 0x10108350: 'nxKnVtAABBsBuilder'})
for v, s in (('nxKnVtSphereCollider', '??_7SphereCollider@Opcode@@6B@'), ('nxKnVtAABBsBuilder', '??_7AABBTreeOfAABBsBuilder@Opcode@@6B@')):
    DATADECL += ['extern "C" void* %s;' % v, '#pragma comment(linker, "/alternatename:_%s=%s")' % (v, s)]
STRINGS = {0x101061c0: 'nxP4ModeRb', 0x101061bc: 'nxP4ModeWb'}
TABLES = {0x100b5124: 4}
SYMBOLS.update({0xe56d0: '??0StaticPruner@@QAE@XZ', 0xef850: '??0DynamicPruner@@QAE@XZ'})
HEAD_COMMENT = """PrunerSupportRows.cpp: Phase 4's remaining rows at 0x000b3d20 to 0x000f3ee4 (and their Phase 6 and 7 neighbours in the
same clusters): the penetration map's file read and write and the memory stream's float and block readers
(004790 to 004801, with 004776/004778 as helpers), the box-pruning sorters' lifetimes and the complete and bipartite
pruning (004820 to 004828), the pruner process pool's registration (004830 to 004834), the pruners' creation (004852)
and owner adapters (004870, 004872), the static, bounded and type-3 pruners' remaining slots and destructors (005231,
005262, 005263, 005279, 005448 to 005479, 005539), the pruning pool's file save and load (005586 to 005646), the
tree walks (005566, 005568) and the OBB rows (005380, 005386, 005404), with two more atexit thunks (006332,
006334). Naked listing transcriptions; the oracle's statics are the candidate's by name (nxPCStatics, widened to the
oracle's 32 words, the sorters, the pool singleton, the owner adapters), the oracle's tables are the candidate's
(aliases), and vendored OPCODE code is reached at its vendored symbols. Compare script:
docs/reconstruction/novodex-physics/evidence/phase4-pruner-support-rows-listing-compare.py."""
