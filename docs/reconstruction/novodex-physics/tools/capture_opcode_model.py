"""Immutable genuine OPCODE model/tree/ray capture, with its effective headers."""
import gzip
import hashlib
import json
import pathlib
import re
import subprocess


def assembly(data):
    source = re.sub(r'//[^\n]*|/\*.*?\*/', '', data.decode('latin1'), flags=re.S)
    return [re.sub(r'\s+', '', block).lower() for block in
            re.findall(r'(?<!\w)(?:__asm|_asm)\s*(\{[^}]+\}|[^\r\n]+)', source)]


def capture(args, root, revision, compiler_version):
    output = args.output_dir / (args.capture_id + '.nxpf')
    manifest = output.with_suffix('.json')
    snapshot_index = args.output_dir / (args.capture_id + '-sources.json')
    if any(path.exists() for path in (output, manifest, snapshot_index)):
        raise SystemExit('capture destination already exists; immutable capture refused')
    sources = [
        'tests/portable/OpcodeModelTests.cpp', 'tests/portable/OpcodeModelTargets.cmake',
        'tests/portable/SupportMapDomainInputs.h', 'tests/portable/SdkAllocatorKernel.cpp',
        'tests/portable/FixtureSupport.cpp', 'tests/portable/FixtureSupport.h',
        'tests/portable/CMakeLists.txt', 'tests/portable/IceTopologyTargets.cmake',
        'Physics/src/ThirdPartyHost.cpp', 'Physics/src/include/PhysicsInternal.h',
        'Physics/src/include/NxSdkAllocator.h', 'Physics/src/include/NxSdkAllocatorAccess.inl',
        'cmake/NxPhysicsBackend.cmake', 'Physics/src/include/NxPhysicsBackend.h',
        'docs/reconstruction/novodex-physics/tools/capture_scalar_math.py',
        'docs/reconstruction/novodex-physics/tools/capture_opcode_model.py',
        'Foundation/src/include/NxFoundationAllocatorAccess.inl',
    ]
    sources += ['Foundation/src/' + unit + '.cpp' for unit in
                ('FoundationSDK', 'Observable', 'DebugRenderable', 'Profiler', 'Time', 'Utilities', 'Box')]
    sources += [str(path.relative_to(root)).replace('\\', '/')
                for directory in ('Foundation/include', 'Foundation/src/include')
                for path in sorted((root / directory).rglob('*.h'))]
    vendor = root / 'External/opcode'
    merged = args.exporter.resolve().parent.parent / 'ice-topology-tree'
    effective = {}
    relative_inputs = set(path.relative_to(vendor / 'upstream/Opcode')
                          for path in (vendor / 'upstream/Opcode').rglob('*.h'))
    relative_inputs.update(path.relative_to(vendor / 'novodex')
                           for path in (vendor / 'novodex').rglob('*.h'))
    relative_inputs.update(pathlib.Path(unit + '.cpp') for unit in
                          ('OPC_AABBTree', 'OPC_BaseModel', 'OPC_Collider', 'OPC_Common',
                           'OPC_MeshInterface', 'OPC_Model', 'OPC_OptimizedTree', 'OPC_RayCollider',
                           'OPC_TreeBuilders'))
    relative_inputs.update(pathlib.Path('Ice/' + unit + '.cpp') for unit in
                          ('IceAABB', 'IceHPoint', 'IceContainer', 'IceIndexedTriangle', 'IceMatrix3x3',
                           'IceMatrix4x4', 'IcePlane', 'IcePoint', 'IceRandom', 'IceRay',
                           'IceRevisitedRadix', 'IceTriangle', 'IceUtils'))
    sha = lambda data: hashlib.sha256(data).hexdigest()
    for relative in sorted(relative_inputs):
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
        original = subprocess.check_output(['git', 'show', revision + ':' + source]) if present else b''
        if path.suffix in ('.h', '.cpp', '.inl') and assembly(original) != assembly(path.read_bytes()):
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
    snapshot_index.write_text(json.dumps(index, indent=2) + '\n')
    manifest.write_text(json.dumps({
        'schema_version': 1, 'capture_id': args.capture_id,
        'provenance_kind': 'reconstructed-reference', 'source_revision': revision,
        'working_tree_dirty': bool(subprocess.check_output(['git', 'status', '--porcelain'], text=True).strip()),
        'source_sha256': {entry['source']: entry['source_sha256'] for entry in index},
        'effective_vendor_sha256': effective,
        'legacy_instruction_bodies_verified_against_revision': revision,
        'snapshot_index': snapshot_index.name,
        'exporter_path': str(args.exporter), 'exporter_sha256': sha(args.exporter.read_bytes()),
        'compiler': 'MSVC ' + compiler_version, 'configuration': 'Release Win32',
        'flags': ['/arch:IA32', '/fp:precise', '/Qfast_transcendentals', '/O2', 'NX_PHYSICS_USE_X87=1'],
        'control_words': {'0x027f': '53-bit nearest'}, 'fixture_sha256': sha(output.read_bytes()),
        'record_width': 20, 'record_count': (output.stat().st_size - 16) // 20,
        'encoding': 'NXPF v1 LE u32 kind/group/observation-index/reserved/output-word',
        'operation_ids': {'0': 'exact topology/count/index/decision/ownership',
                          '1': 'bounds/quantization coefficients in source world units',
                          '2': 'ray parameter distance in world units for unit direction',
                          '3': 'dimensionless triangle barycentrics'},
        'domain': 'Eight literal binary32 cube/tetra meshes, vertex words [-3,6], affine scale/shear/'
                  'translation, both windings, all four optimized variants, source and optimized walks, '
                  'reuse/refit, ray/segment/first/closest/cached queries, rigid translation, single/empty/'
                  'degenerate meshes, first checked allocation failure, five splitting rules on actual '
                  'vertex/AABB builders, extension/inflation/refit and source-tree ray queries. Direct '
                  'triangle epsilon/barycentric neighbors and axis/diagonal/parallel/zero directions; '
                  'eight cross-helper arguments independently exercised on equality/adjacent values, '
                  'cancellation/signedzero/NaN/infinity. Save/Load unreconstructed paths excluded.',
    }, indent=2) + '\n')
    print('capture', output, 'sha256', sha(output.read_bytes()), 'snapshots', len(index))
