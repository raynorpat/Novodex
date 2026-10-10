"""Immutable direct shipped Scene/000503 capture; requires genuine original SDK."""
import argparse, gzip, hashlib, json, pathlib, re, subprocess
p=argparse.ArgumentParser()
p.add_argument('--exporter',type=pathlib.Path,required=True)
p.add_argument('--output-dir',type=pathlib.Path,required=True)
p.add_argument('--pair-directory',required=True)
p.add_argument('--reference-revision',required=True)
a=p.parse_args(); root=pathlib.Path.cwd(); stem='scene-visited-buffers-x87'
revision=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()
if revision!=a.reference_revision: raise SystemExit('HEAD differs from explicit reference revision')
output=a.output_dir/(stem+'.nxpf'); manifest=output.with_suffix('.json'); index=a.output_dir/(stem+'-sources.json')
if any(x.exists() for x in (output,manifest,index)): raise SystemExit('capture destination already exists; immutable capture refused')
build=a.exporter.parent.parent
compiler=list((build/'CMakeFiles').glob('*/CMakeCXXCompiler.cmake'))
if not compiler or a.exporter.parent.name!='Release': raise SystemExit('requires configured MSVC Release exporter')
compiler=compiler[0].read_text()
if 'set(CMAKE_CXX_COMPILER_ID "MSVC")' not in compiler or 'set(CMAKE_CXX_SIZEOF_DATA_PTR "4")' not in compiler: raise SystemExit('requires actual MSVC Win32')
version=re.search(r'set\(CMAKE_CXX_COMPILER_VERSION "([^"]+)"\)',compiler)[1]
project=(build/'NxPortableExportSceneVisitedBuffers.vcxproj').read_text()
for token in ('Precise','NoExtensions','Qfast_transcendentals','MultiThreaded'):
    if token not in project: raise SystemExit('missing configured reference profile '+token)
sources=['tests/portable/SceneVisitedGuardAllocator.h','tests/portable/ExportShippedSceneVisitedBuffers.cpp','tests/PhysicsPairLoader.h','tests/portable/SceneVisitedBufferTests.cpp','tests/portable/SceneVisitedBufferTargets.cmake','tests/portable/SceneVisitedBufferPlacementTests.py','tests/portable/CMakeLists.txt','Physics/src/include/NxSceneVisitedBuffers.h','Physics/src/include/NxScenePrunerCollection.h','Physics/src/include/NxIceContainerExternalBuffer.h','Physics/src/include/NxPolygonScratch.h','Physics/src/Scene.cpp','Physics/src/include/Scene.h','Physics/src/Containers.cpp','Physics/src/include/Containers.h','Physics/src/opcode/IcePruner.cpp','Physics/src/opcode/IcePruner.h','Physics/src/opcode/IcePrunable.h','Physics/src/opcode/IcePruningEngine.cpp','Physics/src/ObjectModel.cpp','External/opcode/novodex/Ice/IceContainer.h','External/opcode/novodex/Ice/IceContainer.cpp','docs/reconstruction/novodex-physics/tools/capture_scene_visited_buffers.py']
sources += [str(x.relative_to(root)).replace('\\','/') for directory in ('Physics/include','Foundation/include') for x in sorted((root/directory).glob('*.h'))]
snapshots=a.output_dir/'capture-source'; planned=[snapshots/(stem+'-'+str(i)+'-'+pathlib.Path(s).name+'.gz') for i,s in enumerate(sources)]
if any(x.exists() for x in planned): raise SystemExit('source snapshot exists; immutable capture refused')
if any((root/s).stat().st_mtime>a.exporter.stat().st_mtime for s in sources): raise SystemExit('exporter older than protected source; rebuild')
a.output_dir.mkdir(parents=True,exist_ok=True)
run=subprocess.run([str(a.exporter.resolve()),a.pair_directory,str(output.resolve())],capture_output=True,text=True,check=True)
print(run.stdout,end='')
if run.stderr: raise SystemExit('original probe diagnostic: '+run.stderr)
if 'reference raw=027f crt=0009001f fenv=0' not in run.stdout or 'observations=246' not in run.stdout or 'failures=0' not in run.stdout: raise SystemExit('missing actual original hardware/result proof')
sha=lambda b:hashlib.sha256(b).hexdigest()
snapshots.mkdir(exist_ok=True); entries=[]
for source,target in zip(sources,planned):
 data=(root/source).read_bytes(); compressed=gzip.compress(data,mtime=0); target.write_bytes(compressed)
 entries.append(dict(source=source,source_sha256=sha(data),snapshot=str(target.relative_to(a.output_dir)).replace('\\','/'),snapshot_sha256=sha(compressed)))
index.write_text(json.dumps(entries,indent=2)+'\n')
manifest.write_text(json.dumps(dict(schema_version=1,capture_id=stem,provenance_kind='shipped-reference',source_revision=revision,compiler='MSVC '+version,configuration='Release Win32',flags=['/arch:IA32','/fp:precise','/Qfast_transcendentals','/O2','/MT'],exporter_sha256=sha(a.exporter.read_bytes()),fixture_sha256=sha(output.read_bytes()),snapshot_index=index.name,source_sha256={e['source']:e['source_sha256'] for e in entries},physics_sha256='4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c',foundation_sha256='7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990',measured_hardware_state=dict(raw_x87_control='027f',crt_control='0009001f',fenv_rounding='0'),record_width=20,record_count=246,groups=33,operation_ids={'0':'exact initialized prefix words/count/stamp/allocation count/borrowed buffer identity and header words'},domain='Three genuine original SDK-created Scene lifetimes; direct000503 count0/1/255/256/257/512/513/1024/1023, real registered static and dynamic shape/pruner callback probe and teardown; no undefined shared bytes or relocated pointers in oracle',captured_before_production_scene_integration=True,probe_stdout=run.stdout),indent=2)+'\n')
print('protected shipped capture',output,'sha256',sha(output.read_bytes()),'snapshots',len(entries))
