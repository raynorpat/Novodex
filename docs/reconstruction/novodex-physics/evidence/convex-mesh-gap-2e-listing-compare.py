"""convex-mesh gap Task 2e: compare a built candidate function with the oracle listing,
instruction by instruction.

Usage (from the repository root, after building NxPhysics Release Win32):

    python docs/reconstruction/novodex-physics/evidence/convex-mesh-gap-2e-listing-compare.py         build/Release/NxPhysics.dll build/Release/NxPhysics.map         nxSmoothNormalsAngleAtVertex=0x532e0:0x533b9 nxMeshNormalsCompute=0x318d0:0x31da8         nxIcePosePair=0x31db0:0x32402 nxIceAddUniqueAxis=0x324f0:0x3258b

Each argument names a candidate function (a substring of its decorated name in the map) and
the oracle row's extent. Both are disassembled with Capstone (the oracle from the pinned
NxPhysics.dll at D:/FlamingEnt__/Unreal_3/Binaries) and compared: the same mnemonic and the
same operands, except that a branch target is compared as the index of the instruction it
lands on, a direct call target is not compared (the second part of the output names, for each
of the oracle's calls, the candidate function the call reaches), and an absolute memory operand
(a constant) is compared by the bytes it reads -- 4 for a dword -- each in its own image: the
oracle's from the pinned DLL, the candidate's from the built one (so 0x101078cc's 0x3f7ff972 must
be what the candidate's kIceMeshToolsAxisLimit holds). The listing's alignment fillers are emitted as their
bytes, so they compare as instructions. It prints the differing instructions and ALL EQUAL or
DIFFERENCES.
"""
import sys,re,pefile,capstone,pickle,os
img,mp=sys.argv[1],sys.argv[2]
pe=pefile.PE(img); base=pe.OPTIONAL_HEADER.ImageBase
syms={}
for line in open(mp,encoding='latin-1'):
    m=re.match(r'\s*0001:[0-9a-f]+\s+(\S+)\s+([0-9a-f]{8})\s',line)
    if m: syms[m.group(1)]=int(m.group(2),16)
md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_32)
orc=pefile.PE(r'D:/FlamingEnt__/Unreal_3/Binaries/NxPhysics.dll')
SIZES={'byte':1,'word':2,'dword':4,'qword':8,'tbyte':10}
def absolute(pe_,op):
    """Each absolute memory operand as the bytes it reads in its own image: 4 unless the
    operand's size says otherwise (every one in these rows is a dword constant)."""
    def value(m):
        va=int(m.group(2),16)
        n=SIZES.get(m.group(1),4)
        return '%s[=%s]'%(m.group(1) or '',pe_.get_data(va-pe_.OPTIONAL_HEADER.ImageBase,n).hex())
    return re.sub(r'(?:(byte|word|dword|qword|tbyte) ptr )?\[0x([0-9a-f]+)\]',value,op)
def norm(ins,lo,hi,addrs,pe_):
    out=[]
    idx={a:i for i,a in enumerate(addrs)}
    for i in ins:
        op=i.op_str
        if i.mnemonic.startswith('j') or i.mnemonic=='loop':
            t=int(op,16); op='@%d'%idx.get(t,-1)
        elif i.mnemonic=='call' and op.startswith('0x'):
            op='<call>'
        op=absolute(pe_,op)
        out.append((i.mnemonic,op))
    return out
def dis(pe_,va,size):
    data=pe_.get_data(va-pe_.OPTIONAL_HEADER.ImageBase,size)
    return list(md.disasm(data,va))
ok=True
for spec in sys.argv[3:]:
    name,rng=spec.split('=');lo,hi=[int(x,16) for x in rng.split(':')]
    o=dis(orc,0x10000000+lo,hi-lo)
    cand=[k for k in syms if name in k]
    assert len(cand)==1,(name,cand)
    va=syms[cand[0]]
    c=dis(pe,va,hi-lo+64)
    # candidate: cut at same instruction count
    c=c[:len(o)]
    on=norm(o,0,0,[i.address for i in o],orc); cn=norm(c,0,0,[i.address for i in c],pe)
    diff=[(k,a,b) for k,(a,b) in enumerate(zip(on,cn)) if a!=b]
    print('%s %s instructions=%d differing=%d'%(name,cand[0],len(o),len(diff)))
    for k,a,b in diff[:40]:
        print('   #%d %08x oracle=%s candidate=%s'%(k,o[k].address,a,b))
    ok&=not diff
print('ALL EQUAL' if ok else 'DIFFERENCES')
# call targets of the candidate functions, by map name
rev={v:k for k,v in syms.items()}
for spec in sys.argv[3:]:
    name,rng=spec.split('=');lo,hi=[int(x,16) for x in rng.split(':')]
    o=dis(orc,0x10000000+lo,hi-lo)
    va=syms[[k for k in syms if name in k][0]]
    c=dis(pe,va,hi-lo+64)[:len(o)]
    for a,b in zip(o,c):
        if a.mnemonic=='call' and a.op_str.startswith('0x'):
            print('  call %s -> oracle %s ; candidate %s'%(name,a.op_str,rev.get(int(b.op_str,16),b.op_str)))
