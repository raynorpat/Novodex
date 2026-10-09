"""New, immutable reconstructed-reference capture; never rewrites Task 1."""
import argparse, hashlib, json, pathlib, re, subprocess
root = pathlib.Path.cwd()
p = argparse.ArgumentParser()
p.add_argument('--reference-revision', required=True)
p.add_argument('--capture-id', required=True)
p.add_argument('--exporter', type=pathlib.Path, required=True)
p.add_argument('--kind', choices=['math','rotations','conversions','geometry','ice-topology','ice-hull','ice-mesh-normals','ice-support-maps','foundation-lifecycle','foundation-public-math','opcode-model','triangle-mesh','triangle-fan'], required=True)
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
if a.kind == 'triangle-fan':
    from capture_triangle_fan import capture
    capture(a, root, revision, version[1])
    raise SystemExit(0)
if a.kind in ('opcode-model', 'triangle-mesh'):
    from capture_opcode_model import capture
    capture(a, root, revision, version[1])
    raise SystemExit(0)
if a.kind in ('foundation-lifecycle', 'foundation-public-math'):
    from capture_foundation_lifecycle import capture
    capture(a, root, revision, version[1])
    raise SystemExit(0)
def assembly(source):
    source = re.sub(r'//[^\n]*|/\*.*?\*/', '', source, flags=re.S)
    return [re.sub(r'\s+','', b).lower() for b in re.findall(r'__asm\s*\{([^}]+)\}',source)]
sources = (['Physics/src/ConvexHull.cpp','Physics/src/IceMeshTools.cpp','Physics/src/IceMeshBuilder2.cpp','Physics/src/EdgeList.cpp','Physics/src/IceAdjacencies.cpp'] if a.kind=='ice-hull' else
           ['Physics/src/EdgeList.cpp','Physics/src/IceAdjacencies.cpp'] if a.kind=='ice-topology' else
           ['Physics/src/Geometry.cpp','Physics/src/Distance.cpp','Physics/src/SmoothNormals.cpp','Physics/src/ShapeRaycast.cpp','Physics/src/PMap.cpp'] if a.kind=='geometry' else
           ['Physics/src/include/X87Sqrt.h','Physics/src/include/core/JointAcos.h']
           if a.kind != 'conversions' else ['Physics/src/Quantizer.cpp','Physics/src/core/SceneDump.cpp'])
if a.kind == 'ice-support-maps':
    sources = ['Physics/src/IceSupportMaps.cpp','Physics/src/ConvexHull.cpp','Physics/src/IceMeshTools.cpp',
        'Physics/src/IceMeshBuilder2.cpp','Physics/src/EdgeList.cpp','Physics/src/IceAdjacencies.cpp']
if a.kind == 'ice-mesh-normals':
    sources = ['Physics/src/ConvexHull.cpp','Physics/src/IceMeshTools.cpp',
        'Physics/src/IceMeshBuilder2.cpp','Physics/src/TriangleMeshTopology.cpp',
        'Physics/src/SmoothNormals.cpp','Physics/src/EdgeList.cpp','Physics/src/IceAdjacencies.cpp',
        'Foundation/src/FoundationSDK.cpp']
for source in sources:
    old = subprocess.check_output(['git','show',revision+':'+source], text=True)
    current = (root/source).read_text()
    if assembly(old) != assembly(current):
        raise SystemExit('legacy instruction body changed: '+source)
    if (root/source).stat().st_mtime > a.exporter.stat().st_mtime:
        raise SystemExit('exporter is older than source: rebuild before capture')
exporter_source=root/('tests/portable/SupportMapTests.cpp' if a.kind=='ice-support-maps' else 'tests/portable/MeshNormalsTests.cpp' if a.kind=='ice-mesh-normals' else 'tests/portable/HullTests.cpp' if a.kind=='ice-hull' else 'tests/portable/IceTopologyTests.cpp' if a.kind=='ice-topology' else 'tests/portable/GeometryDomain.cpp' if a.kind=='geometry' else 'tests/portable/ExportSharedMathFixtures.cpp' if a.kind!='conversions' else 'tests/portable/ExportConversionFixtures.cpp')
if exporter_source.stat().st_mtime > a.exporter.stat().st_mtime:
    raise SystemExit('exporter source is newer than executable: rebuild before capture')
if a.kind == 'conversions':
    exported = assembly((root/'tests/portable/ExportConversionFixtures.cpp').read_text())
    expected = [assembly((root/source).read_text())[0] for source in sources]
    # Exporter adds independent fnstcw/fldcw lines, outside these blocks.
    if exported != expected:
        raise SystemExit('conversion exporter differs from production instruction bodies')
if a.kind == 'geometry':
    # The only copied reference island is PMap normalization. Ignore symbolic
    # local-variable spelling, never operands, instructions or store order.
    reference=assembly((root/'Physics/src/PMap.cpp').read_text())[0].replace('direction','input')
    if assembly(exporter_source.read_text()) != [reference]:
        raise SystemExit('PMap exporter instruction body differs from production')
    old=subprocess.check_output(['git','show',revision+':Physics/src/NarrowPhase.cpp'],text=True)
    if 'double __cdecl NxSegmentSegmentSquareDistance(' in old:
        start=old.index('double __cdecl NxSegmentSegmentSquareDistance(')
        end=old.index('\n// ---------------------------------------------------------------------------',start)
        reference_body=old[start:end].strip()
    else:
        reference_body=subprocess.check_output(['git','show',revision+':Physics/src/include/NxSegmentSegmentDistance.inl'],text=True).split('\n',1)[1].strip()
    extracted=(root/'Physics/src/include/NxSegmentSegmentDistance.inl').read_text().split('\n',1)[1].strip()
    if reference_body!=extracted:
        raise SystemExit('segment-distance mechanical extraction changed body')
    sources+=['Physics/src/include/NxSegmentSegmentDistance.inl','tests/portable/GeometryDomainInputs.h',
        'tests/portable/GeometryDomain.cpp','tests/portable/ExportGeometry.cpp','tests/portable/GeometrySdkHeaderSeam.h']
    for source in sources:
        if (root/source).stat().st_mtime > a.exporter.stat().st_mtime:
            raise SystemExit('exporter older than relevant source: '+source)
if a.kind in ('ice-topology','ice-hull','ice-mesh-normals','ice-support-maps'):
    sources += ['Physics/src/include/NxSdkAllocator.h','Physics/src/include/NxSdkAllocatorAccess.inl',
        'Physics/src/include/EdgeList.h','Physics/src/include/IceAdjacencies.h',
        'tests/portable/IceTopologyTests.cpp','tests/portable/SdkAllocatorKernel.cpp',
        'tests/portable/IceTopologyTargets.cmake','tests/portable/GeometrySdkHeaderSeam.h']
    if a.kind in ('ice-hull','ice-mesh-normals','ice-support-maps'):
        sources += ['Physics/src/include/ConvexHull.h','Physics/src/include/IceMeshTools.h',
            'Physics/src/include/IceMeshBuilder2.h','Physics/src/include/portable/NxConvexInterfaces.h',
            'Physics/src/portable/ConvexHullScalar.inl','Physics/src/portable/IceUniqueAxisScalar.inl',
            'tests/portable/HullTests.cpp','tests/portable/HullTargets.cmake']
    if a.kind == 'ice-mesh-normals':
        sources += ['Physics/src/include/TriangleMesh.h','Physics/src/include/NxInternalTriangleMesh.h',
            'Physics/src/include/NxSmoothNormalsAngle.h',
            'Physics/src/portable/IceMeshToolsScalar.inl','Foundation/src/include/NxFoundationAllocatorAccess.inl',
            'tests/portable/FoundationAllocatorKernel.cpp','tests/portable/MeshNormalsTests.cpp',
            'tests/portable/MeshNormalsTargets.cmake','tests/portable/MeshNormalsBudgets.h']
    if a.kind == 'ice-support-maps':
        sources += ['Physics/src/include/IceSupportMaps.h','Physics/src/portable/IceSupportMapsScalar.inl',
            'Foundation/src/include/NxScalarConversions.h','tests/portable/SupportMapTests.cpp',
            'tests/portable/SupportMapDomainInputs.h','tests/portable/SupportMapTargets.cmake']
    # Pin the actual effective vendor inputs, including precompiled-header
    # parsing dependencies, rather than pretending upstream equals overlay.
    upstream=root/'External/opcode/upstream/Opcode'
    overlay=root/'External/opcode/novodex'
    for path in upstream.rglob('*'):
        if path.is_file() and path.suffix.lower() in ('.h','.cpp'):
            effective=overlay/path.relative_to(upstream)
            sources.append(str((effective if effective.exists() else path).relative_to(root)).replace('\\','/'))
    sources += [str(path.relative_to(root)).replace('\\','/') for path in overlay.rglob('*.h') if not (upstream/path.relative_to(overlay)).exists()]
    for source in sources:
        if (root/source).stat().st_mtime > a.exporter.stat().st_mtime:
            raise SystemExit('exporter older than relevant source: '+source)
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
    'exporter_source_sha256':sha(exporter_source),
    'compiler':'MSVC '+version[1],'configuration':'Release Win32',
    'flags':['/arch:IA32','/fp:precise','/O2','NX_PHYSICS_USE_X87=1']+(['/Qfast_transcendentals'] if a.kind in ('ice-topology','ice-hull','ice-mesh-normals','ice-support-maps') else []),
    'control_words':{'0x027f':'53-bit nearest'} if a.kind in ('geometry','ice-topology','ice-hull','ice-mesh-normals','ice-support-maps') else {'0x027f':'53-bit nearest','0x0f7f':'64-bit chop; diagnostic only'},
    'fixture_sha256':sha(output),'record_width':272 if a.kind=='geometry' else 20 if a.kind in ('ice-topology','ice-hull','ice-mesh-normals','ice-support-maps') else 80 if a.kind!='conversions' else 20,
    'record_count':(len(output.read_bytes())-16)//(272 if a.kind=='geometry' else 20 if a.kind in ('ice-topology','ice-hull','ice-mesh-normals','ice-support-maps') else 80 if a.kind!='conversions' else 20),
    'encoding':'NXPF v1 LE u32 kind/input-id/observation-index/reserved/output-binary32-or-discrete' if a.kind in ('ice-topology','ice-hull','ice-mesh-normals','ice-support-maps') else 'NXPF v1 LE u32 op/id, 32 binary32 input words, u32 discrete/count, sixteen binary64 outputs' if a.kind=='geometry' else 'NXPF v1 LE u32 op/u32 CW/input and output IEEE binary64 words' if a.kind!='conversions' else 'NXPF v1 LE u32 op/u32 CW/binary64 input u64/signed output low32 u32',
    'operation_ids':{'0':'exact boolean/table/ownership/count/sample-byte/support-index/stamp/canary','1':'exact dimensionless cube-face coordinate binary32 word'} if a.kind=='ice-support-maps' else {'0':'exact ownership/boolean/count/indices/canaries','1':'dimensionless normal or rotation component','2':'length: coordinate or pose translation','3':'exact binary32 corner angle radians'} if a.kind=='ice-mesh-normals' else {'0':'exact topology/boolean/report/count/support-index/stamp','1':'dimensionless normal/axis component','2':'length: centroid/plane-distance/projection-extremum','3':'area','4':'exact constructor canary'} if a.kind=='ice-hull' else {'0':'exact topology/boolean/report/count','1':'normal component','2':'plane distance','3':'exact vendor helper bits'} if a.kind=='ice-topology' else list(range(11)) if a.kind=='geometry' else list(range(18)) if a.kind!='conversions' else {'0':'wuFistp255','1':'sceneDumpRound'},
    'domain':('8 literal cube/tetra meshes with identity/nonuniform2:.5:1.5/shear/decimal shear and translation, final coordinates[-3,6], real A/B/C init subdivisions0,1,2,3,5,8, genuine table dispatch/lazy hulls/graph starts and stamps/allocator failures/deletion and repeated ownership. Direct C counts0..9/255 and finite cancellation vertices[-2^24,2^24]. Private cube/lookup only:43 literal binary32 directions (axes/ties/adjacent half-indices/zero/s-qNaN/inf/subnormal), subdivisions0,1,2,3,4,5,8,17,7fffffff,80000000,ffffffff; no sample-array dereference for extreme/nonfinite cases.20 independent binary64 qword-low32 conversion probes. No contacts, polygon+64 owner, native64 or full-engine claim' if a.kind=='ice-support-maps' else '8 literal four-vertex meshes bounded[-3,4.5],flat/positive-negative crease/nonuniform scale/shear/translation/coincident/collinear/signedzero;two windings,four index alternatives,two equal-unit-face/angle modes,four output-ownership modes,two cache calls;builder32 flag combinations,fresh receiver repeats,re-Init retained-count false-AddFace;optional3 rigid PR poses with translation[-3,3],both output retention flags;actual hull/internal owner and failure calls. Inputs in pinned source. Nonfinite/extreme/denormal/OOB/scaled-pose/second Build after failed AddFace excluded' if a.kind=='ice-mesh-normals' else '8 literal finite correctly oriented cube/tetra meshes under4 affine transforms bounded coordinates[-3,6], nonuniform scale/shear/translation,11 literal axes including ties/zero, optional rigid pose, graph starts/stamp reuse; real lazy/rebuild paths and leaf winding/degenerate/empty/open/threshold tests. Reversed closed hulls historically unsupported: empty EdgeLoop unchecked access. Inputs in pinned exporter source' if a.kind=='ice-hull' else '15 literal finite four-vertex meshes (five adjacent crease thresholds, translated tilted/nonuniform meshes, coordinates[-3,6]), both winding/index widths, four retention flags, direct register helpers and supported invalid/empty inputs; inputs embedded in pinned exporter source' if a.kind=='ice-topology' else '714 checked-in literal records, Python Random seed0x4e585034 materialized once, coordinates[-8,8] eighth-units; ten boundary reproducers; no full mesh/PMap integration' if a.kind=='geometry' else '128 seeded finite cases per helper; cases128/129 are explicit 2^63/2^62 FSIN/FCOS probes' if a.kind=='math' else
              '32 cases per helper; trig rows cover signedzero, ordinary rotations, adjacent binary32 pi/2, pi, 2pi, negative pi/2pi; two dt/norm scales' if a.kind=='rotations' else
              'literal half-integers, int32/qword limits and adjacent values, nonfinite, signedzero, subnormal, quantizer half-index values'),
},indent=2)+'\n')
print('capture',output,'sha256',sha(output))
