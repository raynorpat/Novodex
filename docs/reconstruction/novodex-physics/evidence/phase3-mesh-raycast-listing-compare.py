"""The convex mesh shape's raycast and sweep slots, phys_fn_001405 and 001407: compare the built candidate
functions with the oracle listing, instruction by instruction (the PMap compute-arm script with
these rows).

Usage (from the repository root, after building NxPhysics Release Win32):

    python docs/reconstruction/novodex-physics/evidence/phase3-mesh-raycast-listing-compare.py         build/Release/NxPhysics.dll build/Release/NxPhysics.map

A call target is asserted by the candidate symbol it must reach (ShapeRaycast.cpp and ObjectModel.cpp aliases).
"""
import sys,re,pefile,capstone,struct
ROWS=[('?NxShapeRaycastMesh@@YIPBUNxCollisionShape@@PBU1@PAXPBVNxRay@@MIIPAUNxRaycastHit@@@Z',0x29230,0x29602),('?nxMeshSweepPrepared@MeshShape@@QBE_NPAMPBM@Z',0x29610,0x2974a)]
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
STAMP_ORACLE=0x10123c74
STAMP_CANDIDATE=None
def absolute(pe_,op):
    global STAMP_CANDIDATE
    if STAMP_CANDIDATE is None: STAMP_CANDIDATE=0
    op=op.replace('[0x%x]'%STAMP_ORACLE,'[<stamp>]').replace('[0x%x]'%STAMP_CANDIDATE,'[<stamp>]')
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
    cand=[name] if name in syms else [k for k in syms if name in k]
    assert len(cand)==1,(name,cand)
    return cand[0],syms[cand[0]]
def table_targets(pe_,tva,addrs):
    idx={a:i for i,a in enumerate(addrs)}
    return tuple(idx.get(struct.unpack('<I',pe_.get_data(tva+4*k-pe_.OPTIONAL_HEADER.ImageBase,4))[0],-1) for k in range(6))
def section_of(pe_,va):
    rva=va-pe_.OPTIONAL_HEADER.ImageBase
    for sec in pe_.sections:
        if sec.VirtualAddress<=rva<sec.VirtualAddress+max(sec.Misc_VirtualSize,sec.SizeOfRawData):
            return sec
    return None
def norm(ins,pe_,tableva):
    out=[]; addrs=[i.address for i in ins]; idx={a:i for i,a in enumerate(addrs)}
    for i in ins:
        op=i.op_str
        if (i.mnemonic.startswith('j') or i.mnemonic=='loop') and op.startswith('0x'):
            op='@%d'%idx.get(int(op,16),-1)
        elif i.mnemonic=='call' and op.startswith('0x'):
            op='<call>'
        elif i.mnemonic=='push' and op.startswith('0x') and section_of(pe_,int(op,16)) is not None:
            sec=section_of(pe_,int(op,16))
            if sec.Characteristics & 0x20000000:
                op='<code>'
            else:
                d=pe_.get_data(int(op,16)-pe_.OPTIONAL_HEADER.ImageBase,256)
                op='<string %r>'%d[:d.index(bytes(1))]
        elif i.mnemonic=='mov' and re.search(r', 0x1[0-9a-f]{7}$',op) and section_of(pe_,int(op.rsplit(', ',1)[1],16)) is not None:
            op=op.rsplit(', ',1)[0]+', <ptr>'
        elif i.mnemonic=='jmp' and 'ebx*4' in op:
            m=re.search(r'0x([0-9a-f]+)\]',op)
            op='table%s'%(table_targets(pe_,int(m.group(1),16),addrs),)
        op=absolute(pe_,op)
        if i.mnemonic=='test' and re.fullmatch(r'[a-z]{2,3}, [a-z]{2,3}',op):
            op=', '.join(sorted(op.split(', ')))   # test is commutative; the assembler's register order is not the listing's
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
EXPECTED={0xb4d70:'??0SdkContainer@@QAE@XZ',0xb4f90:'?setExternalBuffer@SdkContainer@@QAEXIPAI@Z',0xb4f50:'?empty@SdkContainer@@QAEXXZ',
 0xba6f0:'?Collide@RayCollider@Opcode@@QAE_NABVRay@IceMaths@@ABVModel@2@PBVMatrix4x4@4@PAI@Z',0x52240:'?nxMeshComputeVertexNormals@@YAXXZ',
 0x2e220:'?nxSupportMapLookup@@YIIPBUIceSupportMap@@IPBVPoint@IceMaths@@@Z',0x2b6f0:'?nxHullComputePolygons@@YI_NPAVConvexHull@@@Z',0x36bb0:'_NxRayPlaneIntersect'}
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
