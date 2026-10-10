"""Immutable genuine shipped Scene actor-ID member capture before translation."""
import argparse, gzip, hashlib, json, pathlib, re, subprocess
p=argparse.ArgumentParser()
for name in ('exporter','output-dir'): p.add_argument('--'+name,type=pathlib.Path,required=True)
for name in ('pair-directory','reference-revision'): p.add_argument('--'+name,required=True)
a=p.parse_args(); root=pathlib.Path.cwd(); stem='scene-actor-ids-x87'
revision=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()
if revision!=a.reference_revision: raise SystemExit('reference HEAD mismatch')
output=a.output_dir/(stem+'.nxpf'); manifest=output.with_suffix('.json'); index=a.output_dir/(stem+'-sources.json')
if any(x.exists() for x in (output,manifest,index)): raise SystemExit('immutable destination already exists')
build=a.exporter.parent.parent
compiler=next((build/'CMakeFiles').glob('*/CMakeCXXCompiler.cmake')).read_text()
if a.exporter.parent.name!='Release' or 'set(CMAKE_CXX_COMPILER_ID "MSVC")' not in compiler or 'set(CMAKE_CXX_SIZEOF_DATA_PTR "4")' not in compiler: raise SystemExit('actual MSVC Release Win32 required')
version=re.search(r'set\(CMAKE_CXX_COMPILER_VERSION "([^"]+)"\)',compiler)[1]
project=(build/'NxPortableExportSceneActorIds.vcxproj').read_text()
if not all(t in project for t in ('Precise','NoExtensions','Qfast_transcendentals','MultiThreaded')): raise SystemExit('reference profile mismatch')
sources=['tests/portable/ExportShippedSceneActorIds.cpp','tests/portable/SceneActorIdDomain.h','tests/portable/SceneVisitedGuardAllocator.h','tests/PhysicsPairLoader.h','tests/portable/SceneActorIdPlacementTests.py','tests/portable/SceneActorIdTargets.cmake','tests/portable/CMakeLists.txt','Physics/src/Scene.cpp','Physics/src/include/Scene.h','Physics/src/ObjectModel.cpp','Physics/src/NpActor.cpp','Physics/src/include/NpActor.h','Physics/src/NpScene.cpp','Physics/src/include/NpScene.h','docs/reconstruction/novodex-physics/tools/capture_scene_actor_ids.py']
scratch='.superpowers/sdd/2026-10-09-nxphysics-portable-scalar/'
sources += [scratch+x for x in ('task-5f2t-authoritative-actor.json','task-5f2t-authoritative-actor-id-pool.json','task-5f2t-scoped-brief.md')]
sources += [str(x.relative_to(root)).replace('\\','/') for folder in ('Physics/include','Foundation/include') for x in sorted((root/folder).glob('*.h'))]
for source in ('Physics/src/Scene.cpp','Physics/src/include/Scene.h','Physics/src/ObjectModel.cpp','Physics/src/NpActor.cpp','Physics/src/NpScene.cpp'):
    if subprocess.check_output(['git','show',revision+':'+source])!=(root/source).read_bytes().replace(b'\r\n',b'\n'): raise SystemExit('product edited before capture: '+source)
if any((root/s).stat().st_mtime>a.exporter.stat().st_mtime for s in sources): raise SystemExit('rebuild exporter after protected sources')
snapshots=a.output_dir/'capture-source'; planned=[snapshots/(stem+'-'+str(i)+'-'+pathlib.Path(s).name+'.gz') for i,s in enumerate(sources)]
if any(x.exists() for x in planned): raise SystemExit('immutable snapshot destination exists')
run=subprocess.run([str(a.exporter.resolve()),a.pair_directory,str(output.resolve())],capture_output=True,text=True,check=True)
print(run.stdout,end='')
if run.stderr or 'reference raw=027f crt=0009001f fenv=0' not in run.stdout or 'groups=180 observations=1731 allocations=60 releases=60 failures=0' not in run.stdout: raise SystemExit('missing exact hardware/ownership/domain proof')
sha=lambda data:hashlib.sha256(data).hexdigest(); snapshots.mkdir(exist_ok=True); entries=[]
for source,target in zip(sources,planned):
    data=(root/source).read_bytes(); compressed=gzip.compress(data,mtime=0); target.write_bytes(compressed)
    entries.append(dict(source=source,source_sha256=sha(data),snapshot=str(target.relative_to(a.output_dir)).replace('\\','/'),snapshot_sha256=sha(compressed)))
index.write_text(json.dumps(entries,indent=2)+'\n')
manifest.write_text(json.dumps(dict(schema_version=1,capture_id=stem,provenance_kind='shipped-reference',source_revision=revision,compiler='MSVC '+version,configuration='Release Win32',flags=['/arch:IA32','/fp:precise','/Qfast_transcendentals','/O2','/MT'],exporter_sha256=sha(a.exporter.read_bytes()),fixture_sha256=sha(output.read_bytes()),snapshot_index=index.name,source_sha256={e['source']:e['source_sha256'] for e in entries},physics_sha256='4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c',foundation_sha256='7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990',measured_hardware_state=dict(raw_x87_control='027f',crt_control='0009001f',fenv_rounding='0'),record_width=20,record_count=1731,groups=180,operation_ids={'0':'exact discrete ID/count/capacity/array words/allocation/free/cleanup observations'},domain='Three genuine SDK-created empty Scenes; actual member6d0, direct thiscall1430/1b90; freshIDs0..15;15 returns/LIFO takes,growth2/6/14/30; helper-only seeded u32 wrap and arbitrary duplicate words with no live actors, drained/restored initial counter before normal Scene/SDK cleanup; no pointer addresses/padding serialized',captured_before_member_translation=True,probe_stdout=run.stdout),indent=2)+'\n')
print('protected capture',output,'sha256',sha(output.read_bytes()),'snapshots',len(entries))
