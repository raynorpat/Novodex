"""convex-mesh gap Task 2g: compare built candidate functions with the oracle listing,
instruction by instruction (the Task 2f script, extended for this task's rows).

Usage (from the repository root, after building NxPhysics Release Win32):

    python docs/reconstruction/novodex-physics/evidence/convex-mesh-gap-2g-listing-compare.py         build/Release/NxPhysics.dll build/Release/NxPhysics.map

It checks every naked row of Task 2g (ROWS below): each entry names a candidate function (a
substring of its decorated name in the map) and the oracle extent; a row with continuations is
one extent (the continuations and the alignment fillers between them included). Both are
disassembled with Capstone (the oracle from the pinned NxPhysics.dll at
D:/FlamingEnt__/Unreal_3/Binaries) and compared: the same mnemonic and the same operands, except
that
  - a branch target inside the extent is compared as the index of the instruction it lands on;
  - a direct call or a jump out of the extent (the tail jumps of slots 9 and 10) is not compared
    here: the second part asserts, for each, the candidate function it must reach;
  - an absolute memory operand is compared by the bytes it reads (4 for a dword, 8 for a qword),
    each in its own image, except an import slot, which is compared by the imported name
    (0x10104174, NxFindRotationMatrix, against the candidate's __imp__NxFindRotationMatrix).
It prints the differing instructions, ALL EQUAL or DIFFERENCES, then every call and external jump
with the candidate function it reaches (MAPPINGS ... unexpected=0), and exits non-zero on any
difference.
"""
import sys,re,pefile,capstone,struct
ROWS=[('nxHullComputeEdgeAxes',0x2d2e0,0x2d499),('nxHullSupportFace',0x2d4a0,0x2d93a),
 ('nxHullClimbSupportVertex',0x2d9b0,0x2dadd),('nxScratchStamp',0x10190,0x101c2),
 ('nxMeshHullCentre',0x54800,0x5480a),('nxMeshHullVertexCount',0x54810,0x5481a),('nxMeshHullVertices',0x54820,0x5482a),
 ('nxMeshHullSupportPolygon',0x54830,0x5483b),('nxMeshHullSupportFace',0x54840,0x5484b),
 ('nxMeshHullPolygonCount',0x54850,0x5486a),('?nxMeshHullPolygon@',0x54870,0x54896),('nxMeshHullEdgeAxes',0x548a0,0x548ba),
 ('nxMeshHullEdges@',0x548c0,0x548da),('nxMeshHullEdgeToPolygons',0x548e0,0x548fa),('nxMeshHullEdgePolygons',0x54900,0x5491a),
 ('nxMeshHullProject',0x552c0,0x5548b),
 ('NxEmitContactFeatures',0x1d8e0,0x1dc73),('nxPolygonContainsPoint',0x48b30,0x48bcf),
 ('nxClipEdgeToPolygonPlane',0x48bd0,0x48e2f),('NxConvexPolygonContacts',0x48e30,0x49c98),
 ('nxConvexAxisOverlap',0x3fd80,0x3fe18),('nxConvexAxisSeparation',0x3fe20,0x3fe95),('nxConvexFaceAxes@',0x3fea0,0x3ffcd),
 ('nxConvexFaceAxesFirst',0x3ffd0,0x40180),('nxConvexEdgeBoxOverlap',0x40180,0x403e3),('nxConvexGatherEdgeAxes',0x403f0,0x405ed),
 ('nxConvexSeparatingAxis',0x405f0,0x40bc2),('nxConvexConvexContact',0x40bd0,0x41196),('NxContactConvexConvex',0x411a0,0x4135a)]
args=[a for a in sys.argv[1:] if not a.startswith('--')]
img,mp=args[0],args[1]
pe=pefile.PE(img); base=pe.OPTIONAL_HEADER.ImageBase
syms={}
for line in open(mp,encoding='latin-1'):
    m=re.match(r'\s*000[1-9]:[0-9a-f]+\s+(\S+)\s+([0-9a-f]{8})\s',line)
    if m: syms[m.group(1)]=int(m.group(2),16)
rev={}
for k,v in syms.items(): rev.setdefault(v,k)
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
orc=pefile.PE(r'D:/FlamingEnt__/Unreal_3/Binaries/NxPhysics.dll')
SIZES={'byte':1,'word':2,'dword':4,'qword':8,'tbyte':10}
def imports(pe_):
    out={}
    for e in getattr(pe_,'DIRECTORY_ENTRY_IMPORT',[]):
        for i in e.imports:
            if i.name: out[i.address]=i.name.decode()
    return out
IMPORTS={}
def absolute(pe_,op):
    if id(pe_) not in IMPORTS: IMPORTS[id(pe_)]=imports(pe_)
    def value(m):
        va=int(m.group(2),16); n=SIZES.get(m.group(1),4)
        if va in IMPORTS[id(pe_)]: return 'import[%s]'%IMPORTS[id(pe_)][va]
        try: return '%s[=%s]'%(m.group(1) or '',pe_.get_data(va-pe_.OPTIONAL_HEADER.ImageBase,n).hex())
        except Exception: return '%s[?%x]'%(m.group(1) or '',va)
    return re.sub(r'(?:(byte|word|dword|qword|tbyte) ptr )?\[0x([0-9a-f]+)\]',value,op)
def dis(pe_,va,size):
    return list(md.disasm(pe_.get_data(va-pe_.OPTIONAL_HEADER.ImageBase,size),va))
def lookup(name):
    cand=[k for k in syms if name in k]
    assert len(cand)==1,(name,cand)
    return cand[0],syms[cand[0]]
def table_targets(pe_,tva,addrs):
    idx={a:i for i,a in enumerate(addrs)}
    return tuple(idx.get(struct.unpack('<I',pe_.get_data(tva+4*k-pe_.OPTIONAL_HEADER.ImageBase,4))[0],-1) for k in range(6))
def norm(ins,pe_,tableva):
    out=[]; addrs=[i.address for i in ins]; idx={a:i for i,a in enumerate(addrs)}
    for i in ins:
        op=i.op_str
        if (i.mnemonic.startswith('j') or i.mnemonic=='loop') and op.startswith('0x'):
            op='@%d'%idx.get(int(op,16),-1)
        elif i.mnemonic=='call' and op.startswith('0x'):
            op='<call>'
        elif i.mnemonic=='push' and op.startswith('0x') and int(op,16)>=pe_.OPTIONAL_HEADER.ImageBase+0x1000 and int(op,16)<pe_.OPTIONAL_HEADER.ImageBase+0x100000:
            op='<code>'
        elif i.mnemonic=='jmp' and 'ebx*4' in op:
            m=re.search(r'0x([0-9a-f]+)\]',op)
            op='table%s'%(table_targets(pe_,int(m.group(1),16),addrs),)
        op=absolute(pe_,op)
        out.append((i.mnemonic,op))
    return out
ok=True
for name,lo,hi in ROWS:
    o=dis(orc,0x10000000+lo,hi-lo)
    assert sum(i.size for i in o)==hi-lo
    sym,va=lookup(name)
    c=dis(pe,va,hi-lo+256)[:len(o)]
    on=norm(o,orc,None); cn=norm(c,pe,None)
    diff=[(k,a,b) for k,(a,b) in enumerate(zip(on,cn)) if a!=b]
    if len(c)<len(o): diff.append((len(c),'(end)','(short)'))
    print('%s %s instructions=%d differing=%d'%(name,sym,len(o),len(diff)))
    for k,a,b in diff[:40]:
        print('   #%d %08x oracle=%s candidate=%s'%(k,o[k].address if k<len(o) else 0,a,b))
    ok&=not diff
print('ALL EQUAL' if ok else 'DIFFERENCES')
# Every direct call and every pushed constructor must reach the candidate function of the same
# stable ID (or the vendored member the oracle's row is): the oracle target's RVA -> a substring of
# the candidate's decorated name. Asserted, not only printed (Task 2f review).
EXPECTED={0x2b6f0:'?nxHullComputePolygons@',0x2cb50:'?nxHullComputeEdges@',0x2c8f0:'?nxHullSupportPolygon@',
 0x2d4a0:'?nxHullSupportFace@',0x2d2e0:'?nxHullComputeEdgeAxes@',0x2d9b0:'?nxHullClimbSupportVertex@',
 0x324f0:'?nxIceAddUniqueAxis@',0x31db0:'?nxIcePosePair@',0x2e220:'?nxSupportMapLookup@',0x10190:'?nxScratchStamp@',
 0xb4d70:'??0Container@IceCore@@QAE@XZ',0xb4f50:'??1Container@IceCore@@QAE@XZ',0xb4de0:'?Resize@Container@IceCore@@AAE_NI@Z',
 0xb55b0:'?UpdateWorldAABB@Prunable@@QAEXPAVAABB@IceMaths@@@Z',
 0xf41f0:'_free',0xf48c5:'??2@YAPAXIABUnothrow_t@std@@@Z',0xf47b0:'__chkstk',
 0x257a0:'?NxShapeOwner@',0x56650:'?NxContinuousCdPair@',0x1d8e0:'?NxEmitContactFeatures@',
 0x48b30:'?nxPolygonContainsPoint@',0x48bd0:'?nxClipEdgeToPolygonPlane@',0x48e30:'?NxConvexPolygonContacts@',
 0x3fd80:'?nxConvexAxisOverlap@',0x3fe20:'?nxConvexAxisSeparation@',0x3fea0:'?nxConvexFaceAxes@',
 0x3ffd0:'?nxConvexFaceAxesFirst@',0x40180:'?nxConvexEdgeBoxOverlap@',0x403f0:'?nxConvexGatherEdgeAxes@',
 0x405f0:'?nxConvexSeparatingAxis@',0x40bd0:'?nxConvexConvexContact@'}
checked=0; bad=0
for name,lo,hi in ROWS:
    o=dis(orc,0x10000000+lo,hi-lo)
    sym,va=lookup(name)
    c=dis(pe,va,hi-lo+256)[:len(o)]
    for a,b in zip(o,c):
        external=a.mnemonic.startswith('j') and a.op_str.startswith('0x') and not (0x10000000+lo<=int(a.op_str,16)<0x10000000+hi)
        if (a.mnemonic=='call' or a.mnemonic=='push' or external) and a.op_str.startswith('0x') and b.op_str.startswith('0x'):
            if a.mnemonic=='push' and not (0x10001000<=int(a.op_str,16)<0x10100000): continue
            target=int(b.op_str,16)
            want=EXPECTED.get(int(a.op_str,16)-0x10000000)
            good=want is not None and any(want in k and v==target for k,v in syms.items())
            checked+=1; bad+=not good
            print('  %s %s -> oracle %s ; candidate %s%s'%(a.mnemonic,name,a.op_str,rev.get(target,b.op_str),'' if good else '  <-- UNEXPECTED'))
print('MAPPINGS checked=%d unexpected=%d'%(checked,bad))
sys.exit(0 if ok and not bad else 1)
