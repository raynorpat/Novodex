"""Actual production state/link lifetime and legal guard routing source gate."""
import pathlib,re
root=pathlib.Path(__file__).resolve().parents[2]
scene=(root/'Physics/src/NpScene.cpp').read_text()
assert 'nxSceneLockCreate()' in scene, 'scalar NpScene lacks actual ReadWriteLock producer'
assert 'nxSceneLockDestroy(mWriteLock)' in scene and 'nxSceneLockDestroy(mReadLock)' in scene
unit=(root/'Physics/src/ReadWriteLockScalar.cpp').read_text()
assert 'new (memory) ReadWriteLockData' in unit, 'actual state lifetime missing'
assert 'new (memory) ReadWriteLock' in unit, 'actual link lifetime missing'
assert '->~ReadWriteLockData()' in unit and '->~ReadWriteLock()' in unit
assert 'static_cast<ReadWriteLock*>(link)->lock()' in unit
assert 'static_cast<ReadWriteLock*>(link)->tryLock()' in unit
assert 'static_cast<ReadWriteLock*>(link)->unlock()' in unit
guards=(root/'Physics/src/include/NpSceneGuard.h').read_text()
scalar=guards.split('#else',1)[1].split('#endif',1)[0]
assert 'reinterpret_cast' not in scalar and 'CRITICAL_SECTION' not in scalar
assert 'void nxNpSceneGuardEnter(void* link);' in scalar
def selected(text,backend):
    out=[]; stack=[]; active=True
    for line in text.splitlines(keepends=True):
        d=line.strip()
        if re.match(r'#\s*(if|ifdef|ifndef)\b',d):
            controlled=d in ('#if NX_PHYSICS_USE_X87','#if !NX_PHYSICS_USE_X87')
            condition=(backend==1) if d=='#if NX_PHYSICS_USE_X87' else (backend==0)
            stack.append((controlled,active,condition))
            if controlled: active=active and condition; continue
        elif d=='#else' and stack:
            controlled,parent,condition=stack[-1]
            if controlled: active=parent and not condition; continue
        elif d=='#endif' and stack:
            controlled,parent,condition=stack.pop()
            if controlled: active=parent; continue
        if active: out.append(line)
    return ''.join(out)
active=selected(scene,0)
ctor=active.split('NpScene::NpScene(',1)[1].split('NpScene::~NpScene()',1)[0]
assert ctor.index('CreateEventA')<ctor.index('mWriteLock = nxSceneLockCreate()')<ctor.index('mReadLock = nxSceneLockCreate()')<ctor.index('nxConditionConstruct')
dtor=active.split('NpScene::~NpScene()',1)[1].split('NxActor* NpScene::createActor',1)[0]
assert dtor.index('nxConditionStop')<dtor.index('nxSceneLockDestroy(mWriteLock)')<dtor.index('nxSceneLockDestroy(mReadLock)')<dtor.index('CloseHandle(*reinterpret_cast<HANDLE*>(mLockB))')<dtor.index('CloseHandle(*reinterpret_cast<HANDLE*>(mLockA))')
assert 'nxLockConstruct' not in active
assert 'mOwnerThreadId=' not in unit.split('ReadWriteLock::ReadWriteLock()',1)[0]
assert 'nxFoundationSDKAllocator->malloc' in unit and 'nxFoundationSDKAllocator->free' in unit
print('actual Scene lock lifetime/typed guard source PASS')
