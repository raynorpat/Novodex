"""Turn autocfg's all_cfg.py / all_head.txt into cfg.py and head.txt.

Adds PlanesCollider::ValidateSettings as a second definition of phys_fn_005111's body (the oracle ICF-folds the two
symbols), and moves the callee aliases and data declarations into head.txt before @@PROTOS@@.
"""
import re

ns = {}
cfg = open('all_cfg.py').read()
exec(cfg, ns)
rows = ns['ROWS']
obb = [r for r in rows if r[0].startswith('?ValidateSettings@OBB')][0]
rows.append(('?ValidateSettings@PlanesCollider@Opcode@@UAEPBDXZ', obb[1], obb[2],
             'phys_fn_005111 (ICF-folded: the same body for PlanesCollider)',
             '__declspec(naked) const char* Opcode::PlanesCollider::ValidateSettings()'))
cfg = re.sub(r'^ROWS = .*$', lambda m: 'ROWS = ' + repr(rows), cfg, flags=re.M)
open('cfg.py', 'w').write(cfg)

h = open('all_head.txt').read().split('\n')
extra = [l for l in h[h.index('#include "Stdafx.h"') + 1:h.index('@@PROTOS@@')] if l.strip()]
head = open('head.txt').read()
marker = '// The callees, by their candidate symbols'
if marker in head:
    head = head[:head.index(marker)] + '@@PROTOS@@' + head.split('@@PROTOS@@', 1)[1]
head = head.replace('@@PROTOS@@', marker + ' (opcode_match.csv), and the data the rows name.\n' + '\n'.join(extra) + '\n\n@@PROTOS@@')
open('head.txt', 'w').write(head)
print('rows', len(rows), 'head lines', len(extra))
