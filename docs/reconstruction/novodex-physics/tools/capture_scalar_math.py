"""New, immutable reconstructed-reference capture; never rewrites Task 1."""
import argparse, hashlib, json, pathlib, re, subprocess
root = pathlib.Path.cwd()
p = argparse.ArgumentParser()
p.add_argument('--reference-revision', required=True)
p.add_argument('--capture-id', required=True)
p.add_argument('--exporter', type=pathlib.Path, required=True)
p.add_argument('--kind', choices=['math','rotations','conversions'], required=True)
p.add_argument('--output-dir', type=pathlib.Path, required=True)
a = p.parse_args()
revision = subprocess.check_output(['git','rev-parse','HEAD'], text=True).strip()
if revision != a.reference_revision:
    raise SystemExit('HEAD does not match explicit capture revision')
if not re.fullmatch(r'[a-z0-9-]+', a.capture_id):
    raise SystemExit('capture id must be a unique lowercase file stem')
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
# Capture only the audited MSVC Release/Win32 build tree, with source files no
# newer than its executable. Derive compiler identity instead of stamping it.
build_root=a.exporter.resolve().parent.parent
compiler_files=list((build_root/'CMakeFiles').glob('*/CMakeCXXCompiler.cmake'))
if not compiler_files or a.exporter.parent.name != 'Release':
    raise SystemExit('capture requires the configured Release MSVC Win32 exporter')
compiler_text=compiler_files[0].read_text()
version=re.search(r'set\(CMAKE_CXX_COMPILER_VERSION "([^"]+)"\)',compiler_text)
if 'set(CMAKE_CXX_COMPILER_ID "MSVC")' not in compiler_text or 'set(CMAKE_CXX_SIZEOF_DATA_PTR "4")' not in compiler_text or not version:
    raise SystemExit('exporter toolchain must be MSVC with four-byte pointers')
def assembly(source):
    source = re.sub(r'//[^\n]*|/\*.*?\*/', '', source, flags=re.S)
    return [re.sub(r'\s+','', b).lower() for b in re.findall(r'__asm\s*\{([^}]+)\}',source)]
sources = (['Physics/src/include/X87Sqrt.h','Physics/src/include/core/JointAcos.h']
           if a.kind != 'conversions' else ['Physics/src/Quantizer.cpp','Physics/src/core/SceneDump.cpp'])
for source in sources:
    old = subprocess.check_output(['git','show',revision+':'+source], text=True)
    current = (root/source).read_text()
    if assembly(old) != assembly(current):
        raise SystemExit('legacy instruction body changed: '+source)
    if (root/source).stat().st_mtime > a.exporter.stat().st_mtime:
        raise SystemExit('exporter is older than source: rebuild before capture')
exporter_source=root/('tests/portable/ExportSharedMathFixtures.cpp' if a.kind!='conversions' else 'tests/portable/ExportConversionFixtures.cpp')
if exporter_source.stat().st_mtime > a.exporter.stat().st_mtime:
    raise SystemExit('exporter source is newer than executable: rebuild before capture')
if a.kind == 'conversions':
    exported = assembly((root/'tests/portable/ExportConversionFixtures.cpp').read_text())
    expected = [assembly((root/source).read_text())[0] for source in sources]
    # Exporter adds independent fnstcw/fldcw lines, outside these blocks.
    if exported != expected:
        raise SystemExit('conversion exporter differs from production instruction bodies')
output = a.output_dir/(a.capture_id+'.nxpf')
manifest = output.with_suffix('.json')
if output.exists() or manifest.exists():
    raise SystemExit('capture destination already exists; immutable capture refused')
a.output_dir.mkdir(parents=True,exist_ok=True)
command = [str(a.exporter.resolve()),str(output.resolve())]
if a.kind == 'math': command += ['--physics-domain']
if a.kind == 'rotations': command += ['--rotation-domain']
subprocess.run(command,check=True)
manifest.write_text(json.dumps({
    'schema_version':1,'capture_id':a.capture_id,'provenance_kind':'reconstructed-reference',
    'source_revision':revision,'working_tree_dirty':bool(subprocess.check_output(['git','status','--porcelain'],text=True).strip()),
    'source_sha256':{source:sha(root/source) for source in sources},
    'legacy_instruction_bodies_verified_against_revision':revision,
    'exporter_path':str(a.exporter),'exporter_sha256':sha(a.exporter),
    'exporter_source_sha256':sha(root/('tests/portable/ExportSharedMathFixtures.cpp' if a.kind!='conversions' else 'tests/portable/ExportConversionFixtures.cpp')),
    'compiler':'MSVC '+version[1],'configuration':'Release Win32',
    'flags':['/arch:IA32','/fp:precise','/O2','NX_PHYSICS_USE_X87=1'],
    'control_words':{'0x027f':'53-bit nearest','0x0f7f':'64-bit chop; diagnostic only'},
    'fixture_sha256':sha(output),'record_width':80 if a.kind!='conversions' else 20,
    'record_count':(len(output.read_bytes())-16)//(80 if a.kind!='conversions' else 20),
    'encoding':'NXPF v1 LE u32 op/u32 CW/input and output IEEE binary64 words' if a.kind!='conversions' else 'NXPF v1 LE u32 op/u32 CW/binary64 input u64/signed output low32 u32',
    'operation_ids':list(range(18)) if a.kind!='conversions' else {'0':'wuFistp255','1':'sceneDumpRound'},
    'domain':('128 seeded finite cases per helper; cases128/129 are explicit 2^63/2^62 FSIN/FCOS probes' if a.kind=='math' else
              '32 cases per helper; trig rows cover signedzero, ordinary rotations, adjacent binary32 pi/2, pi, 2pi, negative pi/2pi; two dt/norm scales' if a.kind=='rotations' else
              'literal half-integers, int32/qword limits and adjacent values, nonfinite, signedzero, subnormal, quantizer half-index values'),
},indent=2)+'\n')
print('capture',output,'sha256',sha(output))
