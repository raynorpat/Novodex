"""Source-level lifetime gate; no raw nonexistent Scene receiver is executed."""
import pathlib
import re

root = pathlib.Path(__file__).resolve().parents[2]
source = (root / 'Physics/src/Scene.cpp').read_text()
constructor = source.split('NxSceneInternal::NxSceneInternal()', 1)[1].split(
    'static void* __fastcall nxFluidManagerDeletingDestructor', 1)[0]
delete = source.split('static void nxSceneDelete(void* self, int flags)\n', 1)[1].split(
    'void NxSceneInternal::scalarDeletingDestructor', 1)[0]
assert 'nxSceneContactMembersConstruct(nxAt(p, 0x450))' in constructor, 'Scene must construct its actual member owner'
assert 'nxSceneContactMembersDestroy(' in delete, 'Scene must release both grown edge arrays and actual RayCollider'
assert delete.index('nxSceneContactMembersDestroy(') < delete.index('if(flags & 1)'), 'members die before owner storage free'
assert re.search(r'#if !NX_PHYSICS_USE_X87\s+alignas\(4\)',
                 (root / 'Physics/src/include/Scene.h').read_text()), 'scalar Scene storage must provide member alignment'
print('actual Scene member placement, lifetime selection and owner alignment: PASS')
