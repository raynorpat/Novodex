"""Actual production lifetime regression; never execute a counterfeit Scene."""
import pathlib

root = pathlib.Path(__file__).resolve().parents[2]
source = (root / 'Physics/src/Scene.cpp').read_text()
ctor = source.split('NxSceneInternal::NxSceneInternal()', 1)[1].split(
    'static void* __fastcall nxFluidManagerDeletingDestructor', 1)[0]
delete = source.split('static void nxSceneDelete(void* self, int flags)\n', 1)[1].split(
    'void NxSceneInternal::scalarDeletingDestructor', 1)[0]
assert 'nxSceneScratchConstruct(nxAt(p, 0x000)' in ctor, 'Scene must construct its sole existing scratch prefix'
assert 'nxSceneScratchReleaseBuffers(' in delete, 'Scene must release the genuine scratch owner buffers'
assert 'nxSceneScratchDestroy(' in delete, 'Scene must end the actual scratch prefix lifetime'
assert delete.index('nxSceneScratchReleaseBuffers(') < delete.index('nxSceneScratchDestroy(') < delete.index('if(flags & 1)'), 'release at original site; end lifetime before self-free'
engine = source.split('void nxSceneMember4CA30(void* self)', 1)[1].split('// phys_fn_002415', 1)[0]
assert 'nxScenePrunerCollectionConstruct(nxAt(p, 0x1c))' in engine, 'engine must construct the actual four-pruner collection'
pruner = (root / 'Physics/src/opcode/IcePruner.cpp').read_text().split('void StaticPruner::SetExternalBuffer', 1)[1].split('// phys_fn_005216', 1)[0]
assert 'nxIceContainerSetExternalBuffer(mTouched,' in pruner, 'scalar dispatch must use the actual IceCore::Container receiver'
print('actual scratch prefix, collection and static-container receiver lifetime: PASS')
