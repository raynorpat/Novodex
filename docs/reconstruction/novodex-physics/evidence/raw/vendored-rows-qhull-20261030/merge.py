"""Turn autocfg's all_cfg.py / all_head.txt into cfg.py and head.txt for the C rows file.

Moves the callee aliases and data declarations into head.txt before @@PROTOS@@, as C (`extern`, not `extern "C"`).
"""
ns = {}
cfg = open('all_cfg.py').read()
open('cfg.py', 'w').write(cfg)
h = open('all_head.txt').read().split('\n')
extra = [l.replace('extern "C" ', 'extern ') for l in h[h.index('#include "qhull_a.h"') + 1:h.index('@@PROTOS@@')] if l.strip()]
head = open('head.txt').read()
marker = '/* The callees and the data the rows name. */'
if marker in head:
    head = head[:head.index(marker)] + '@@PROTOS@@' + head.split('@@PROTOS@@', 1)[1]
head = head.replace('@@PROTOS@@', marker + '\n' + '\n'.join(extra) + '\n\n@@PROTOS@@')
open('head.txt', 'w').write(head)
print('head lines', len(extra))
