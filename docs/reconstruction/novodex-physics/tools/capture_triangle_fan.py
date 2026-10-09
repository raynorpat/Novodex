"""Protected actual-source full Geometry/triangle-fan reference capture."""
import gzip
import hashlib
import json
import pathlib
import subprocess
from capture_opcode_model import assembly


def capture(args, root, revision, compiler_version):
    output = args.output_dir / (args.capture_id + '.nxpf')
    manifest = output.with_suffix('.json')
    index_path = args.output_dir / (args.capture_id + '-sources.json')
    if any(path.exists() for path in (output, manifest, index_path)):
        raise SystemExit('capture destination already exists; immutable capture refused')
    sources = [
        'tests/portable/TriangleFanTests.cpp', 'tests/portable/TriangleFanDomainInputs.h',
        'tests/portable/TriangleFanTargets.cmake', 'tests/portable/CMakeLists.txt',
        'tests/portable/IceTopologyTargets.cmake', 'tests/portable/FixtureSupport.cpp',
        'tests/portable/FixtureSupport.h', 'Physics/src/Geometry.cpp',
        'cmake/NxPhysicsBackend.cmake',
        'docs/reconstruction/novodex-physics/tools/capture_scalar_math.py',
        'docs/reconstruction/novodex-physics/tools/capture_triangle_fan.py',
        'docs/reconstruction/novodex-physics/tools/capture_opcode_model.py',
    ]
    sources += [str(path.relative_to(root)).replace('\\', '/')
                for directory in ('Physics/include', 'Physics/src/include', 'Foundation/include')
                for path in sorted((root / directory).rglob('*')) if path.suffix in ('.h', '.inl')]
    vendor = root / 'External/opcode'
    merged = args.exporter.resolve().parent.parent / 'ice-topology-tree'
    inputs = set(path.relative_to(vendor / 'upstream/Opcode')
                 for path in (vendor / 'upstream/Opcode').rglob('*.h'))
    inputs.update(path.relative_to(vendor / 'novodex') for path in (vendor / 'novodex').rglob('*.h'))
    inputs.update(pathlib.Path('Ice/' + name + '.cpp') for name in
                  ('IceTriangle', 'IcePoint', 'IcePlane', 'IceMatrix3x3', 'IceMatrix4x4',
                   'IceHPoint', 'IceIndexedTriangle', 'IceRandom', 'IceUtils'))
    sha = lambda data: hashlib.sha256(data).hexdigest()
    effective = {}
    for relative in sorted(inputs):
        source = vendor / 'novodex' / relative
        if not source.exists():
            source = vendor / 'upstream/Opcode' / relative
        if source.read_bytes() != (merged / relative).read_bytes():
            raise SystemExit('effective vendor source differs from configured tree: ' + str(relative))
        effective[str(relative).replace('\\', '/')] = sha(source.read_bytes())
        sources.append(str(source.relative_to(root)).replace('\\', '/'))
    sources = list(dict.fromkeys(sources))
    for source in sources:
        path = root / source
        present = subprocess.run(['git', 'cat-file', '-e', revision + ':' + source],
                                 stderr=subprocess.DEVNULL).returncode == 0
        old = subprocess.check_output(['git', 'show', revision + ':' + source]) if present else b''
        if present and path.suffix in ('.h', '.inl', '.cpp') and assembly(old) != assembly(path.read_bytes()):
            raise SystemExit('legacy instruction body changed: ' + source)
        if path.stat().st_mtime > args.exporter.stat().st_mtime:
            raise SystemExit('exporter older than relevant source: ' + source)
    snapshots = args.output_dir / 'capture-source'
    planned = [snapshots / (args.capture_id + '-' + str(i) + '-' + pathlib.Path(source).name + '.gz')
               for i, source in enumerate(sources)]
    if any(path.exists() for path in planned):
        raise SystemExit('source snapshot already exists; immutable capture refused')
    args.output_dir.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(args.exporter.resolve()), str(output.resolve())], check=True)
    snapshots.mkdir(exist_ok=True)
    index = []
    for source, path in zip(sources, planned):
        data = (root / source).read_bytes()
        compressed = gzip.compress(data, mtime=0)
        path.write_bytes(compressed)
        index.append({'source': source, 'source_sha256': sha(data),
                      'snapshot': str(path.relative_to(args.output_dir)).replace('\\', '/'),
                      'snapshot_sha256': sha(compressed)})
    index_path.write_text(json.dumps(index, indent=2) + '\n')
    manifest.write_text(json.dumps({
        'schema_version': 1, 'capture_id': args.capture_id,
        'provenance_kind': 'reconstructed-reference', 'source_revision': revision,
        'working_tree_dirty': bool(subprocess.check_output(['git', 'status', '--porcelain'], text=True).strip()),
        'source_sha256': {entry['source']: entry['source_sha256'] for entry in index},
        'effective_vendor_sha256': effective, 'snapshot_index': index_path.name,
        'legacy_instruction_bodies_verified_against_revision': revision,
        'exporter_path': str(args.exporter), 'exporter_sha256': sha(args.exporter.read_bytes()),
        'compiler': 'MSVC ' + compiler_version, 'configuration': 'Release Win32',
        'flags': ['/arch:IA32', '/fp:precise', '/Qfast_transcendentals', '/O2', 'NX_PHYSICS_USE_X87=1'],
        'control_words': {'0x027f': '53-bit nearest'}, 'fixture_sha256': sha(output.read_bytes()),
        'record_width': 20, 'record_count': (output.stat().st_size - 16) // 20,
        'encoding': 'NXPF v1 LE u32 kind/group/index/reserved/output-word',
        'operation_ids': {'0': 'exact boolean/count/index/first-triangle/write-mask/untouched-output/canaries',
                          '1': 'vendor triangle center and inflated coordinates in world length',
                          '2': 'ray parameter, world length divided by direction magnitude',
                          '3': 'dimensionless barycentric, including defined partial miss writes'},
        'domain': 'Eight literal binary32 vertex arrays, two windings and twelve count2..6 index fans '
                  'including repeated indices, collinear and coincident triangles;32 literal rays each '
                  'include boundary neighbors, zero/parallel/tiny/subnormal directions, negative parameters, '
                  'later hit and first before nearest. All full-source calls use actual vendor Triangle '
                  'and actual Geometry kernels and production headers. count<2/OOB/null active arguments '
                  'and nonfinite/extreme overflow private contracts are excluded explicitly.',
    }, indent=2) + '\n')
    print('capture', output, 'sha256', sha(output.read_bytes()), 'snapshots', len(index))
