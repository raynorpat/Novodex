out=open('mt_out.txt').read()
assert out.startswith('\n// phys_fn_002067')
consts=''
rest=out.lstrip('\n')
parts=rest.split('\n// phys_fn_')
rows={}
for i,p in enumerate(parts):
    if i>0: p='// phys_fn_'+p
    rows[p.split('\n')[0].split(' ')[1]]=p.rstrip()+'\n'
rel=rows['phys_fn_002067']; des=rows['phys_fn_002085']; bld=rows['phys_fn_002087+002089+002091+002093']
des=des.replace('static void __fastcall nxMTDestructRow','void __fastcall nxInternalMeshDestruct')
head=open('mt_head.txt').read()
p='D:/github/Novodex/Physics/src/InternalTriangleMesh.cpp'
t=open(p,encoding='utf-8',newline='').read(); nl='\r\n' if '\r\n' in t else '\n'
tail=open('mt_tail.txt').read()
i=t.index('// phys_fn_002067 (0x00051ef0).'); j=t.index('// phys_fn_002069 (0x00051fa0).')
t=t[:i]+(head+rel+'\n'+des+'\n'+bld+'\n'+tail).replace('\n',nl)+t[j:]
i=t.index('// phys_fn_002087 (0x000523c0). Rebuild'); j=t.index('// phys_fn_002079 (0x000521b0).')
t=t[:i]+t[j:]
open(p,'w',encoding='utf-8',newline='').write(t)
