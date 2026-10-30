"""Run autocfg, adding unresolved call targets that are row starts as in-file helpers, until nothing new resolves.

usage: python autohelp.py <name> <prefix> <id,id,...> <extra.py> [max_helper_bytes]
prints the helper ids it added (their records stay as they are).
"""
import subprocess, sys, re, json
name, prefix, ids, extra = sys.argv[1:5]
limit = int(sys.argv[5]) if len(sys.argv) > 5 else 600
inv = {r['id']: r for r in json.load(open('D:/github/Novodex/docs/reconstruction/novodex-physics/inventory.json', encoding='utf-8'))['functions']}
byrva = {int(r['rva'], 16): r for r in inv.values()}
ids = ids.split(',')
helpers = []
for _ in range(12):
    out = subprocess.run([sys.executable, 'autocfg.py', name, prefix, ','.join(ids + helpers), '--extra', extra], capture_output=True, text=True).stdout
    add = []
    for m in re.finditer(r'call ([0-9a-f]+) from [0-9a-f]+ -> (phys_fn_\d+)', out):
        t, rid = int(m.group(1), 16), m.group(2)
        r = byrva.get(t)
        if r and r['id'] == rid and int(r['size']) <= limit and rid not in ids + helpers + add and r.get('kind') == 'code':
            add.append(rid)
    if not add:
        break
    helpers += add
print(out)
print('HELPERS', ','.join(helpers))
