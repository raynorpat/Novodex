"""Immutable direct shipped Pruner process capture before scalar translation."""
import argparse,gzip,hashlib,json,pathlib,re,subprocess
p=argparse.ArgumentParser()
for flag in ('exporter','output-dir'): p.add_argument('--'+flag,type=pathlib.Path,required=True)
for flag in ('pair-directory','reference-revision'): p.add_argument('--'+flag,required=True)
a=p.parse_args();root=pathlib.Path.cwd();stem='pruner-registration-x87'
revision=subprocess.check_output(['git','rev-parse','HEAD'],text=True).strip()
if revision!=a.reference_revision: raise SystemExit('HEAD differs from explicit reference revision')
output=a.output_dir/(stem+'.nxpf');manifest=output.with_suffix('.json');index=a.output_dir/(stem+'-sources.json')
if any(x.exists() for x in (output,manifest,index)): raise SystemExit('capture destination already exists; immutable capture refused')
build=a.exporter.parent.parent;compiler=list((build/'CMakeFiles').glob('*/CMakeCXXCompiler.cmake'))
if not compiler or a.exporter.parent.name!='Release': raise SystemExit('requires configured MSVC Release exporter')
compiler=compiler[0].read_text()
if 'set(CMAKE_CXX_COMPILER_ID "MSVC")' not in compiler or 'set(CMAKE_CXX_SIZEOF_DATA_PTR "4")' not in compiler: raise SystemExit('requires actual MSVC Win32')
version=re.search(r'set\(CMAKE_CXX_COMPILER_VERSION "([^"]+)"\)',compiler)[1]
project=(build/'NxPortableExportPrunerRegistration.vcxproj').read_text()
for token in ('Precise','NoExtensions','Qfast_transcendentals','MultiThreaded'):
 if token not in project: raise SystemExit('missing actual reference profile '+token)
sources=['tests/portable/SceneVisitedGuardAllocator.h','tests/portable/ExportShippedPrunerRegistration.cpp','tests/PhysicsPairLoader.h','tests/portable/PrunerRegistrationDomain.h','tests/portable/PrunerRegistrationTests.cpp','tests/portable/PrunerRegistrationTargets.cmake','tests/portable/PrunerRegistrationPlacementTests.py','tests/portable/CMakeLists.txt','Physics/src/Scene.cpp','Physics/src/PhysicsSDK.cpp','Physics/src/include/NxSdkAllocator.h','Physics/src/include/NxSdkAllocatorAccess.inl','Physics/src/opcode/IcePruner.cpp','Physics/src/opcode/IcePruner.h','Physics/src/opcode/IcePrunable.cpp','Physics/src/opcode/IcePrunable.h','Physics/src/opcode/IcePruningEngine.cpp','docs/reconstruction/novodex-physics/tools/capture_pruner_registration.py']
sources += [str(x.relative_to(root)).replace('\\','/') for directory in ('Physics/include','Foundation/include') for x in sorted((root/directory).glob('*.h'))]
# These product sources must still be literally preimplementation at capture.
for source in ('Physics/src/Scene.cpp','Physics/src/PhysicsSDK.cpp','Physics/src/opcode/IcePruner.cpp','Physics/src/opcode/IcePrunable.h','Physics/src/opcode/IcePruningEngine.cpp'):
 if subprocess.check_output(['git','show',revision+':'+source]) != (root/source).read_bytes().replace(b'\r\n',b'\n'): raise SystemExit('product changed before protected capture: '+source)
snapshots=a.output_dir/'capture-source';planned=[snapshots/(stem+'-'+str(i)+'-'+pathlib.Path(s).name+'.gz') for i,s in enumerate(sources)]
if any(x.exists() for x in planned): raise SystemExit('source snapshot exists; immutable capture refused')
if any((root/s).stat().st_mtime>a.exporter.stat().st_mtime for s in sources): raise SystemExit('exporter older than protected source; rebuild')
a.output_dir.mkdir(parents=True,exist_ok=True)
run=subprocess.run([str(a.exporter.resolve()),a.pair_directory,str(output.resolve())],capture_output=True,text=True,check=True);print(run.stdout,end='')
if run.stderr: raise SystemExit('original probe diagnostic: '+run.stderr)
if 'reference raw=027f crt=0009001f fenv=0' not in run.stdout or 'observations=2502' not in run.stdout or 'failures=0' not in run.stdout: raise SystemExit('missing genuine hardware/result proof')
sha=lambda b:hashlib.sha256(b).hexdigest();snapshots.mkdir(exist_ok=True);entries=[]
for source,target in zip(sources,planned):
 data=(root/source).read_bytes();compressed=gzip.compress(data,mtime=0);target.write_bytes(compressed)
 entries.append(dict(source=source,source_sha256=sha(data),snapshot=str(target.relative_to(a.output_dir)).replace('\\','/'),snapshot_sha256=sha(compressed)))
index.write_text(json.dumps(entries,indent=2)+'\n')
manifest.write_text(json.dumps(dict(schema_version=1,capture_id=stem,provenance_kind='shipped-reference',source_revision=revision,compiler='MSVC '+version,configuration='Release Win32',flags=['/arch:IA32','/fp:precise','/Qfast_transcendentals','/O2','/MT'],exporter_sha256=sha(a.exporter.read_bytes()),fixture_sha256=sha(output.read_bytes()),snapshot_index=index.name,source_sha256={e['source']:e['source_sha256'] for e in entries},physics_sha256='4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c',foundation_sha256='7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990',measured_hardware_state=dict(raw_x87_control='027f',crt_control='0009001f',fenv_rounding='0'),record_width=20,record_count=2502,groups=72,operation_ids={'0':'exact registry counts/capacity/free-count/member identity/index maps/generation/actual handle/timestamp/null-owner-after-release'},domain='Three genuine shipped SDK lifetimes, each five genuine B5090 factory pruners (types0/1/2), growth2/4/8, real virtual timestamp updates, nonlast dense removal, free-index reuse, stale/duplicate/ffff-index/wrong-generation safe removals,65536 actual producer/destructor reuse cycles generating complete u16 rollover, exact process-owner free order and shared lifetime until SDK release; no uninitialized slots or relocated addresses captured',captured_before_registry_translation=True,probe_stdout=run.stdout),indent=2)+'\n')
print('protected shipped capture',output,'sha256',sha(output.read_bytes()),'snapshots',len(entries))
