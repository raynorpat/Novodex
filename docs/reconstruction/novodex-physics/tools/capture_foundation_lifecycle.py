"""Protected actual-source Foundation lifecycle capture, called by capture_scalar_math."""
import gzip
import hashlib
import json
import pathlib
import re
import subprocess


def capture(args, root, revision, compiler_version):
    output = args.output_dir / (args.capture_id + '.nxpf')
    manifest = output.with_suffix('.json')
    snapshot_index = args.output_dir / (args.capture_id + '-sources.json')
    if output.exists() or manifest.exists() or snapshot_index.exists():
        raise SystemExit('capture destination already exists; immutable capture refused')
    public_math = args.kind == 'foundation-public-math'
    exporter_source = 'tests/portable/' + ('FoundationPublicMathTests.cpp' if public_math else 'FoundationLifecycleTests.cpp')
    sources = [
        'Foundation/src/FoundationSDK.cpp', 'Foundation/src/Observable.cpp',
        'Foundation/src/DebugRenderable.cpp', 'Foundation/src/Profiler.cpp',
        'Foundation/src/Time.cpp', 'Foundation/src/Utilities.cpp', 'Foundation/src/Box.cpp',
        'Foundation/src/include/NxFoundationAllocatorAccess.inl',
        exporter_source, 'tests/portable/FoundationLifecycleTargets.cmake',
        'tests/portable/CMakeLists.txt', 'tests/portable/FixtureSupport.cpp',
        'tests/portable/FixtureSupport.h', 'cmake/NxPhysicsBackend.cmake',
        'docs/reconstruction/novodex-physics/tools/capture_scalar_math.py',
        'docs/reconstruction/novodex-physics/tools/capture_foundation_lifecycle.py',
    ]
    sources += [str(path.relative_to(root)).replace('\\', '/')
                for directory in ('Foundation/include', 'Foundation/src/include')
                for path in sorted((root / directory).rglob('*.h'))]
    legacy_sources = [source for source in sources if source.startswith('Foundation/')]
    def assembly(data):
        text = re.sub(r'//[^\n]*|/\*.*?\*/', '', data.decode('latin1'), flags=re.S)
        return [re.sub(r'\s+', '', block).lower() for block in
                re.findall(r'(?<!\w)(?:__asm|_asm)\s*(\{[^}]+\}|[^\r\n]+)', text)]
    for source in legacy_sources:
        present = subprocess.run(['git', 'cat-file', '-e', revision + ':' + source],
                                 stderr=subprocess.DEVNULL).returncode == 0
        original = subprocess.check_output(['git', 'show', revision + ':' + source]) if present else b''
        if assembly(original) != assembly((root / source).read_bytes()):
            raise SystemExit('legacy instruction body changed: ' + source)
    for source in sources:
        if (root / source).stat().st_mtime > args.exporter.stat().st_mtime:
            raise SystemExit('exporter older than relevant source: ' + source)
    args.output_dir.mkdir(parents=True, exist_ok=True)
    subprocess.run([str(args.exporter.resolve()), str(output.resolve())], check=True)
    sha = lambda data: hashlib.sha256(data).hexdigest()
    snapshots = args.output_dir / 'capture-source'
    snapshots.mkdir(exist_ok=True)
    index = []
    for ordinal, source in enumerate(sources):
        source_bytes = (root / source).read_bytes()
        path = snapshots / (args.capture_id + '-' + str(ordinal) + '-' + pathlib.Path(source).name + '.gz')
        if path.exists():
            raise SystemExit('source snapshot already exists; immutable capture refused')
        compressed = gzip.compress(source_bytes, mtime=0)
        path.write_bytes(compressed)
        index.append({'source': source, 'source_sha256': sha(source_bytes),
                      'snapshot': str(path.relative_to(args.output_dir)).replace('\\', '/'),
                      'snapshot_sha256': sha(compressed)})
    snapshot_index.write_text(json.dumps(index, indent=2) + '\n')
    manifest.write_text(json.dumps({
        'schema_version': 1, 'capture_id': args.capture_id,
        'provenance_kind': 'reconstructed-reference', 'source_revision': revision,
        'working_tree_dirty': bool(subprocess.check_output(['git', 'status', '--porcelain'], text=True).strip()),
        'source_sha256': {entry['source']: entry['source_sha256'] for entry in index},
        'legacy_instruction_bodies_verified_against_revision': revision,
        'snapshot_index': snapshot_index.name,
        'exporter_path': str(args.exporter), 'exporter_sha256': sha(args.exporter.read_bytes()),
        'exporter_source_sha256': sha((root / exporter_source).read_bytes()),
        'compiler': 'MSVC ' + compiler_version, 'configuration': 'Release Win32',
        'flags': ['/arch:IA32', '/fp:precise', '/Qfast_transcendentals', '/O2', 'NX_PHYSICS_USE_X87=1'],
        'control_words': {'0x027f': '53-bit nearest'}, 'fixture_sha256': sha(output.read_bytes()),
        'record_width': 20, 'record_count': (output.stat().st_size - 16) // 20,
        'encoding': 'NXPF v1 LE u32 kind/group/observation-index/reserved/exact-output-word',
        'operation_ids': {'1': 'dimensionless cosine then sine output binary32'} if public_math else
                         {'0': 'exact state/discrete/ownership/payload/basic-math/trap outcome'},
        'domain': '26 literal binary32 radian inputs: signed zeros, +/−tiny, binary32 pi/2,pi,2pi and their '
                  'adjacent words and negatives, pi/4 and +/−1. Direct real public NxSinCos cosine then sine '
                  'outputs, independent analytic signs/zero/maxima/bounds; no binary64 overload exists.' if public_math else
                  'actual Foundation create/version/singleton/custom/default allocator/error/assertion/observer-held '
                  'release, real basic debug payload ownership, profiling zone create/enter/leave/release, '
                  'sqrt4/sqrt9/sincos0, actual absent-instance breakpoint child; all outputs exact. '
                  'Profiler elapsed counter is nondeterministic and has no fixture-valued numeric output. '
                  'Long/nonterminated formatting and SDK destruction with retained debug base pointers excluded.',
    }, indent=2) + '\n')
    print('capture', output, 'sha256', sha(output.read_bytes()))
