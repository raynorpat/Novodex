import re
main=open('s6_ids.txt').read().strip().split(',')
ns={}; exec(open('s6_cfg.py').read(),ns)
rows=[]
for nm,lo,hi,tg,decl in ns['ROWS']:
    ids=[x for x in tg.split() if x.startswith('phys_fn_')]
    if not any(i in main for i in ids): tg='helper: '+tg+' (record unchanged)'
    rows.append((nm,lo,hi,tg,decl))
src=open('s6_cfg.py').read(); src=re.sub(r'^ROWS = .*$', 'ROWS = %r' % rows, src, flags=re.M); open('s6_cfg.py','w').write(src)
n={lo:nm for nm,lo,hi,tg,decl in rows}
def e(a): return '(void*) &%s' % (n[a] if a in n else 'nxP7PureCall')
tabs={'nxP7Tab108740':[0x58830,0x572a0],'nxP7TabR1':[0x5a080,0xf41dc,0xf41dc,0x5a440,0x5a080,0xf41dc,0xf41dc,0x5a230],'nxP7TabR2':[0x91de0,0x5a0a0,0x5a240,0x5a470,0x59710,0x5a870]}
txt="// The controller classes' tables, entry for entry as the oracle has them (0x10108740; 0x1010878c..0x101087ac, which\n// the oracle's 0x10108798, 0x1010879c and 0x101087a8 point into; 0x10108818..0x10108830, with 0x10108824 inside it).\n"
for k,v in tabs.items(): txt+='extern "C" void* %s[%d] = { %s };\n' % (k,len(v),', '.join(e(a) for a in v))
h=open('s6_head.txt').read(); h=h.replace('@@PROTOS@@','@@PROTOS@@\n'+txt,1); open('s6_head.txt','w').write(h)
print(len(rows), sum(1 for r in rows if r[3].startswith('helper')))
