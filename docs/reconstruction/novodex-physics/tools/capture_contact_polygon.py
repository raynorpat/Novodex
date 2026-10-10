"""Immutable complete original polygon contact and genuine stream capture."""
import gzip, hashlib, json, pathlib, subprocess

def capture(args, root, revision, compiler_version):
    def sha(data): return hashlib.sha256(data).hexdigest()
    output=args.output_dir/(args.capture_id+'.nxpf')
    manifest=output.with_suffix('.json')
    index_path=args.output_dir/(args.capture_id+'-sources.json')
    if any(p.exists() for p in (output,manifest,index_path)):
        raise SystemExit('capture destination already exists; immutable capture refused')
    original_revision=args.original_block_revision or revision
    original=subprocess.check_output(['git','show',original_revision+':Physics/src/ContactGeneration.cpp']).replace(b'\r\n',b'\n').replace(b'\n',b'\r\n')
    start=original.index(b'// ---------------------------------------------------------------------------\r\n// convex-mesh gap Task 2g: prerequisites')
    end=original.index(b'\r\nstatic bool nxCompoundAabbOverlap',start)
    block=original[start:end]
    if block not in (root/'Physics/src/ContactPolygon.cpp').read_bytes():
        raise SystemExit('whole original P-Emit/P-Plane block extraction changed')
    start=original.index(b'extern "C" void nxContactCallContainerResize();')
    end=original.index(b'\r\n// The stream never reallocates here.',start)
    if original[start:end] not in (root/'Physics/src/ContactStream.cpp').read_bytes():
        raise SystemExit('whole original reset extraction changed')
    for source, owner in [('ContactStreamLifecycle.inl','ContactPairManager.cpp'),
                          ('ContactPairBytes.inl','ContactPairManager.cpp'),
                          ('ContactPairEmitter.inl','ContactPairManager.cpp'),
                          ('ContactStreamRelease.inl','ObjectModel.cpp')]:
        prior=subprocess.check_output(['git','show',original_revision+':Physics/src/'+owner]).replace(b'\r\n',b'\n').replace(b'\n',b'\r\n')
        if (root/'Physics/src/include'/source).read_bytes() not in prior:
            raise SystemExit('mechanical actual owner extraction changed: '+source)
    sources=['CMakeLists.txt','cmake/NxPhysicsBackend.cmake','tests/portable/CMakeLists.txt',
        'tests/portable/IceTopologyTargets.cmake','tests/portable/ContactPolygonTargets.cmake',
        'tests/portable/ConvexContactTests.cpp','tests/portable/ContactPairEmitterKernel.cpp',
        'tests/portable/ContactPolygonImportSlot.cpp','tests/portable/SdkAllocatorKernel.cpp',
        'tests/portable/FixtureSupport.cpp','tests/portable/FixtureSupport.h',
        'Physics/src/ContactPolygon.cpp','Physics/src/ContactStream.cpp','Physics/src/ContactGeneration.cpp',
        'Physics/src/ContactPairManager.cpp','Physics/src/ObjectModel.cpp','Physics/src/Containers.cpp',
        'Physics/src/ThirdPartyHost.cpp','Physics/src/include/Containers.h','Physics/src/include/PhysicsInternal.h',
        'Physics/src/include/ContactGeneration.h','Physics/src/include/NarrowPhase.h','Physics/src/include/ContactPairManager.h',
        'Physics/src/include/NxSdkAllocator.h','Physics/src/include/NxSdkAllocatorAccess.inl',
        'Physics/src/include/ContactStreamLifecycle.inl','Physics/src/include/ContactStreamRelease.inl',
        'Physics/src/include/ContactPairBytes.inl','Physics/src/include/ContactPairEmitter.inl',
        'Physics/src/include/portable/NxPolygonContactInterfaces.h',
        'docs/reconstruction/novodex-physics/tools/capture_scalar_math.py',
        'docs/reconstruction/novodex-physics/tools/capture_contact_polygon.py',
        'Physics/src/include/portable/ContactPolygonScalar.inl','Physics/src/include/portable/ContactStreamScalar.inl']
    sources += [str(p.relative_to(root)).replace('\\','/') for base in
        ['Physics/include','Foundation/include','Foundation/src/include'] for p in sorted((root/base).glob('*.h'))]
    sources += ['Foundation/src/'+s+'.cpp' for s in
        ['FoundationSDK','Observable','DebugRenderable','Profiler','Time','Utilities','Box']]
    tree=args.exporter.resolve().parent.parent/'ice-topology-tree'
    effective={}
    for rel in ['Ice/'+s+'.cpp' for s in ['IceContainer','IceTriangle','IcePoint','IcePlane','IceMatrix3x3',
                'IceMatrix4x4','IceHPoint','IceIndexedTriangle','IceRandom','IceUtils']]:
        overlay=root/'External/opcode/novodex'/rel
        source=overlay if overlay.exists() else root/'External/opcode/upstream/Opcode'/rel
        if source.read_bytes()!=(tree/rel).read_bytes(): raise SystemExit('effective vendor changed: '+rel)
        effective[rel]=sha(source.read_bytes());sources.append(str(source.relative_to(root)).replace('\\','/'))
    for p in sorted(tree.rglob('*.h')):
        rel=p.relative_to(tree); overlay=root/'External/opcode/novodex'/rel
        source=overlay if overlay.exists() else root/'External/opcode/upstream/Opcode'/rel
        if source.read_bytes()!=p.read_bytes(): raise SystemExit('effective vendor header changed: '+str(rel))
        effective[str(rel).replace('\\','/')]=sha(source.read_bytes()); sources.append(str(source.relative_to(root)).replace('\\','/'))
    sources=list(dict.fromkeys(sources))
    for source in sources:
        if (root/source).stat().st_mtime>args.exporter.stat().st_mtime: raise SystemExit('exporter older than source: '+source)
    snapshots=args.output_dir/'capture-source'
    planned=[snapshots/(args.capture_id+'-'+str(i)+'-'+pathlib.Path(s).name+'.gz') for i,s in enumerate(sources)]
    if any(p.exists() for p in planned): raise SystemExit('source snapshot exists; immutable capture refused')
    args.output_dir.mkdir(parents=True,exist_ok=True)
    command=[str(args.exporter.resolve()),str(output.resolve())]
    if args.kind=='contact-polygon-pose': command.append('--pose-regression')
    result=subprocess.run(command,capture_output=True,text=True,check=True)
    if 'raw_control=027f crt_control=0009001f fe_round=0' not in result.stdout:
        raise SystemExit('actual raw/CRT/fenv control proof missing')
    print(result.stdout,end='')
    snapshots.mkdir(exist_ok=True);index=[]
    for source,path in zip(sources,planned):
        data=(root/source).read_bytes();compressed=gzip.compress(data,mtime=0);path.write_bytes(compressed)
        index.append({'source':source,'source_sha256':sha(data),'snapshot':str(path.relative_to(args.output_dir)).replace('\\','/'),
                      'snapshot_sha256':sha(compressed)})
    index_path.write_text(json.dumps(index,indent=2)+'\n')
    manifest.write_text(json.dumps({'schema_version':1,'capture_id':args.capture_id,'provenance_kind':'reconstructed-reference',
        'source_revision':revision,'working_tree_dirty':True,'whole_original_blocks_verified_against_revision':original_revision,
        'source_sha256':{e['source']:e['source_sha256'] for e in index},'effective_vendor_sha256':effective,
        'snapshot_index':index_path.name,'exporter_path':str(args.exporter),'exporter_sha256':sha(args.exporter.read_bytes()),
        'compiler':'MSVC '+compiler_version,'configuration':'Release Win32',
        'flags':['/O2','/arch:IA32','/fp:precise','/Qfast_transcendentals','NX_PHYSICS_USE_X87=1'],
        'actual_control_proof_stdout':result.stdout,'control_words':{'0x027f':'53-bit nearest'},
        'fixture_sha256':sha(output.read_bytes()),'record_width':20,'record_count':(output.stat().st_size-16)//20,
        'encoding':'NXPF v1 LE u32 kind/group/index/reserved/output-word',
        'quantities':{'1':'point coordinates: world length','2':'normal/displacement coordinates: source-defined magnitude',
                      '3':'depth or clip parameter: declared per input domain'},
        'chronology':'post-implementation pre-round1-fix original-source supplement' if args.kind=='contact-polygon-pose' else 'see acceptance chronology',
        'domain':('Eight full polygon groups934..941; true x/y tilted rigid pose, binary32 cancellation y=2^-54 or1e-5, matching cancellation translation, contained/crossing triangles, both shape orders, reset/reuse and unused-word invariance' if args.kind=='contact-polygon-pose' else 'Literal triangle/quad/pentagon/hexagon/tilted polygons, both corrected windings and orders, seven translations; '
                 'original22argument calls and leaf clipping/containment including analytic successful clip; nonuniform4:.125 geometry, nonzero plane d, two literal rigid poses/inverses; repeated genuine stream lifecycle/emission with feature flags/IDs')},indent=2)+'\n')
