"""convex-mesh gap Task 2f: compare built candidate functions with the oracle listing,
instruction by instruction (the Task 2e script, extended for this task's rows).

Usage (from the repository root, after building NxPhysics Release Win32):

    python docs/reconstruction/novodex-physics/evidence/convex-mesh-gap-2f-listing-compare.py \
        build/Release/NxPhysics.dll build/Release/NxPhysics.map

With no further argument it checks every naked row of Task 2f (ROWS below). Each entry names a
candidate function (a substring of its decorated name in the map) and the oracle extent; a row
with continuations is one extent (the continuations and the alignment fillers between them
included, as the naked copy emits them). Both are disassembled with Capstone (the oracle from the
pinned NxPhysics.dll at D:/FlamingEnt__/Unreal_3/Binaries) and compared: the same mnemonic and
the same operands, except that
  - a branch target is compared as the index of the instruction it lands on;
  - a direct call target is not compared here (the second part of the output names, for each of
    the oracle's calls, the candidate function the call reaches);
  - an absolute memory operand is compared by the bytes it reads (4 for a dword, 8 for a qword),
    each in its own image, so 0x10107880's double 1e-7 must be what the candidate's constant holds;
  - an immediate that is a code address (a constructor pushed for 000001) is compared by the
    candidate function it names, printed with the calls;
  - 001558's `jmp dword ptr [ebx*4 + table]` is compared by the instructions its six entries land
    on (the oracle's table at 0x1002e550, the candidate's gIceSupportMapFaceCases), each as an
    instruction index.
It prints the differing instructions, ALL EQUAL or DIFFERENCES, then the calls. `--offsets`
prints the offsets of 001558's three case labels in the built row (the values of
kIceSupportMapCaseX/Y/Z in IceSupportMaps.cpp).
"""
import sys,re,pefile,capstone,struct
ROWS=[('nxIceVectorConstruct',0x1000,0x1030),('nxHullPolygonConstruct',0x20440,0x2044d),
 ('nxIceIdentityConstruct',0x27f00,0x27f03),('nxEdgeDescConstruct',0x2a610,0x2a61f),
 ('nxHullTriangleArea',0x2a620,0x2a6d2),('nxHullTriangleCenter',0x2a790,0x2a822),
 ('nxHullComputeCentroid',0x2ad60,0x2ae51),('nxHullPolygonPlane',0x2af30,0x2b081),
 ('nxHullComputePolygons',0x2b6f0,0x2b988),('nxHullSupportPolygon',0x2c8f0,0x2cb42),
 ('nxHullComputeEdges',0x2cb50,0x2d2d3),('nxSupportMapCubeFace',0x2e160,0x2e1e3),
 ('nxSupportMapLookup',0x2e220,0x2e2e4),('nxSupportMapInit',0x2e2f0,0x2e54e),
 ('nxSupportMapPlaneCompute',0x2e670,0x2e7b3),('nxSupportMapVertexCompute',0x2e890,0x2ea68),
 ('nxSupportMapNoop',0x2ea70,0x2ea71)]
TABLE=(0x2e550,'gIceSupportMapFaceCases')
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
def absolute(pe_,op):
    def value(m):
        va=int(m.group(2),16); n=SIZES.get(m.group(1),4)
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
if '--offsets' in sys.argv:
    name,va=lookup('nxSupportMapInit')
    o=dis(orc,0x1002e2f0,0x2e54e-0x2e2f0); c=dis(pe,va,0x400)[:len(o)]
    oi={i.address-0x10000000:k for k,i in enumerate(o)}
    print(' '.join('0x%x'%(c[oi[a]].address-va) for a in (0x2e380,0x2e3df,0x2e43c))); sys.exit(0)
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
for name,lo,hi in ROWS:
    o=dis(orc,0x10000000+lo,hi-lo)
    sym,va=lookup(name)
    c=dis(pe,va,hi-lo+256)[:len(o)]
    for a,b in zip(o,c):
        if (a.mnemonic=='call' or a.mnemonic=='push') and a.op_str.startswith('0x') and b.op_str.startswith('0x'):
            if a.mnemonic=='push' and not (0x10001000<=int(a.op_str,16)<0x10100000): continue
            print('  %s %s -> oracle %s ; candidate %s'%(a.mnemonic,name,a.op_str,rev.get(int(b.op_str,16),b.op_str)))
