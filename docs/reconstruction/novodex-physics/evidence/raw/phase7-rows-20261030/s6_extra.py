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
SYMBOLS.update({0x203c0: '?nxPOSetCenterExtents@@YIXPAX0PBX1@Z', 0x313e0: '?nxIceIdentityPoses@@YAPBMXZ',
                0xe3190: '?GetVertexNormals@AABB@IceMaths@@QBEPBVPoint@2@XZ', 0xe31a0: '?GetEdges@AABB@IceMaths@@QBEPBIXZ',
                0xe31b0: '?GetEdgeNormals@AABB@IceMaths@@QBEPBVPoint@2@XZ', 0xe4ca0: '?GetEdges@OBB@IceMaths@@QBEPBIXZ',
                0xf4722: '_malloc', 0xf4734: '_free', 0xf41dc: '__purecall', 0x27f00: '_nxIceIdentityConstruct'})
DATA.update({0x1012466c: 'nxP7SweepGuard', 0x10124670: 'nxP7SweepCache'})
for k in range(18):
    DATA[0x101222f8 + 4 * k] = 'nxP7Axes + %d' % (4 * k) if k else 'nxP7Axes'
PUSHES = {0x10108740: 'nxP7Tab108740', 0x10108798: 'nxP7TabR1 + 12', 0x1010879c: 'nxP7TabR1 + 16', 0x101087a8: 'nxP7TabR1 + 28',
          0x10108818: 'nxP7TabR2', 0x10108824: 'nxP7TabR2 + 12', 0x10027f00: 'nxP7IdentityCtor'}
DATADECL = DATADECL + [
 '// .data 0x1012466c and 0x10124670: a guard byte and the sweep cache 002300 to 002308 keep.',
 'static unsigned char nxP7SweepGuard; static unsigned nxP7SweepCache;',
 '// .data 0x101222f8: the six signed axes (written once by the image, read here as a table).',
 'static float nxP7Axes[18] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, -1.0f };',
 'extern "C" void* nxP7Tab108740[2]; extern "C" void* nxP7TabR1[8]; extern "C" void* nxP7TabR2[6];',
 'extern "C" void* nxP7IdentityCtor;', '#pragma comment(linker, "/alternatename:_nxP7IdentityCtor=_nxIceIdentityConstruct")',
 'extern "C" void nxP7PureCall();', '#pragma comment(linker, "/alternatename:_nxP7PureCall=__purecall")']
HEAD_COMMENT = """Phase7Rows.cpp: Phase 7's remaining rows: the continuous-collision sweep geometry at 0x00056690 to 0x0005a16a
(phys_fn_002268 to 002316: the box, capsule and triangle sweep tests, the sweep cache with its table, the per-shape
sweep dispatch), the character controllers' creation (002332, 002336), the step row 002350 and 002400. Naked listing
transcriptions. The oracle's .data is the candidate's by name where the candidate has it (the identity poses, the ICE
box tables through their accessors, the SDK and scene rows) and new statics here where it does not (the sweep cache,
the axis table). The controller classes' tables (the oracle's 0x10108740, 0x1010878c..0x101087ac and
0x10108818..0x10108830) are built here entry for entry, from rows transcribed as helpers (their records unchanged)
and the CRT's _purecall. Compare script: docs/reconstruction/novodex-physics/evidence/phase7-rows-listing-compare.py."""
SYMBOLS.update({0xf4140: '__fpclass', 0xf41f0: '_free'})
PUSHES.update({0x1010878c: 'nxP7TabR1', 0x10106414: 'nxP7VtShapeDesc'})
DATADECL = DATADECL + ['extern "C" void* nxP7VtShapeDesc;', '#pragma comment(linker, "/alternatename:_nxP7VtShapeDesc=??_7NxShapeDesc@@6B@")']
