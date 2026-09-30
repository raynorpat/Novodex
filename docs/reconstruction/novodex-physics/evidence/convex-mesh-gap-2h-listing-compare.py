"""convex-mesh gap Task 2h: compare built candidate functions with the oracle listing,
instruction by instruction (the Task 2g script, with this task's rows).

Usage (from the repository root, after building NxPhysics Release Win32):

    python docs/reconstruction/novodex-physics/evidence/convex-mesh-gap-2h-listing-compare.py         build/Release/NxPhysics.dll build/Release/NxPhysics.map

It checks every naked row of Task 2h (ROWS below): each entry names a candidate function (a
substring of its decorated name in the map) and the oracle extent; a row with continuations is
one extent (the continuations and the alignment fillers between them included). Both are
disassembled with Capstone (the oracle from the pinned NxPhysics.dll at
D:/FlamingEnt__/Unreal_3/Binaries) and compared: the same mnemonic and the same operands, except
that
  - a branch target inside the extent is compared as the index of the instruction it lands on;
  - a direct call is not compared here: the second part asserts, for each, the candidate
    function it must reach (the vendored Matrix4x4::Invert and TriangleMesh::createEdgeList
    through their /alternatename aliases, the stack probe, and the rows of Tasks 2b..2g);
  - an absolute memory operand is compared by the bytes it reads (4 for a dword, 8 for a qword),
    each in its own image (0.0f, 1.0f and the double 1e-6).
It prints the differing instructions, ALL EQUAL or DIFFERENCES, then every call and external jump
with the candidate function it reaches (MAPPINGS ... unexpected=0), and exits non-zero on any
difference.
"""
import sys,re,pefile,capstone,struct
ROWS=[('?nxConvexMeshRay@',0x41360,0x4162d),('?nxConvexMeshProject@',0x41630,0x4178c),
 ('?nxConvexMeshAxis@',0x41790,0x41836),('?nxConvexMeshFaceAxesAll@',0x41840,0x4192b),
 ('?nxConvexMeshInterval@',0x41930,0x419ab),('?nxConvexMeshFaceAxes@',0x419b0,0x41ba0),
 ('?nxConvexMeshTriangleAxis@',0x41ba0,0x41c10),('?nxConvexMeshEdgeDirections@',0x41c10,0x41fd1),
 ('?nxConvexMeshCrossAxes@',0x41fe0,0x42456),('?nxConvexMeshEdgeAxes@',0x42460,0x42557),
 ('?nxConvexMeshContacts@',0x42560,0x427cf)]
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
EXPECTED={0xe4400:'?Invert@Matrix4x4@IceMaths@@QAEAAV12@XZ',0x2b6f0:'?nxHullComputePolygons@',
 0x36d90:'?NxRayInflatedTriangleFan@',0x10190:'?nxScratchStamp@',0x54460:'?createEdgeList@TriangleMesh@@QAEXXZ',
 0xf47b0:'__chkstk',0x324f0:'?nxIceAddUniqueAxis@',0x48e30:'?NxConvexPolygonContacts@',
 0x41630:'?nxConvexMeshProject@',0x41790:'?nxConvexMeshAxis@',0x41840:'?nxConvexMeshFaceAxesAll@',
 0x41930:'?nxConvexMeshInterval@',0x41ba0:'?nxConvexMeshTriangleAxis@',0x41c10:'?nxConvexMeshEdgeDirections@'}
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
