"""Real production member lifetime; no fabricated Scene runtime receiver."""
import pathlib
root = pathlib.Path(__file__).resolve().parents[2]
source = (root / 'Physics/src/Scene.cpp').read_text()
ctor = source.split('NxSceneInternal::NxSceneInternal()', 1)[1].split('static void* __fastcall nxFluidManagerDeletingDestructor', 1)[0]
destroy = source.split('static void nxSceneDelete(void* self, int flags)\n', 1)[1].split('void NxSceneInternal::scalarDeletingDestructor', 1)[0]
assert 'nxSceneActorIdPoolConstruct(nxAt(p, 0x6d0))' in ctor, 'Scene must construct its actual actor-ID member at +6d0'
assert 'nxSceneActorIdPoolDestroy(' in destroy, 'Scene must end actual actor-ID member lifetime at original free position'
take = source.split('void* nxSceneCreateActorBody(void* memory, void* scene)\n', 1)[1].split('// ---------------------------------------------------------------------------', 1)[0]
assert 'nxSceneActorIds(*static_cast<NxSceneInternal*>(scene)).take()' in take, 'actor emulation must use actual Scene ID member'
release = source.split('void nxActorDestroy(unsigned char* body)\n', 1)[1].split('// A static actor', 1)[0]
assert 'nxSceneActorIds(*scene).returnId(' in release, 'actor destruction must return its ID to actual member'
recycle = source.split('void nxSceneRecycleActorId(NxSceneInternal* scene, unsigned id)\n', 1)[1].split('// Dynamic-record', 1)[0]
assert 'nxSceneActorIds(*scene).returnId(id)' in recycle, 'all actor-ID routes must use the actual member'
print('actual Scene actor-ID constructor/consumer/destructor placement: PASS')
