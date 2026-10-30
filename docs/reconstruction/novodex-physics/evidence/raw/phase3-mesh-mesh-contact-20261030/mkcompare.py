"""Write an evidence listing-compare script from a transcribe_rows config and its header.

usage: python mkcompare.py <cfg.py> <head.txt> <out.py> "<docstring first line>" [static ranges as lo:hi:substr,...] [named data abs:substr:canon,...]
"""
import importlib.util, re, sys
cfgp, headp, out, doc = sys.argv[1:5]
ranges = [x.split(':') for x in sys.argv[5].split(',')] if len(sys.argv) > 5 and sys.argv[5] else []
named = [x.split(':') for x in sys.argv[6].split(',')] if len(sys.argv) > 6 and sys.argv[6] else []
spec = importlib.util.spec_from_file_location('cfg', cfgp); cfg = importlib.util.module_from_spec(spec); spec.loader.exec_module(cfg)
alt = dict(re.findall(r'/alternatename:_(\w+)=(\S+?)"\)', open(headp).read()))
rows = []
for sym, lo, hi, tag, decl in cfg.ROWS:
    rows.append("('%s', 0x%x, 0x%x)" % (sym, lo, hi))
exp = {}
for t, nm in cfg.CALLS.items():
    exp[t - 0x10000000] = alt.get(nm, nm)
for t, nm in getattr(cfg, 'PUSHES', {}).items():
    if nm in [r[0] for r in cfg.ROWS] or nm in alt or (t < 0x10100000 and ' ' not in nm):
        exp[t - 0x10000000] = alt.get(nm, nm)
tables = {k: v for k, v in getattr(cfg, 'TABLES', {}).items()}
src = '''"""%s

Usage (from the repository root, after building NxPhysics Release Win32):

    python %s build/Release/NxPhysics.dll build/Release/NxPhysics.map
"""
import os, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import listing_compare_lib as lib

ROWS = [%s]
EXPECTED = {%s}
TABLES = {%s}
DATA = [%s]
STATIC_RANGES = [%s]
lib.run(ROWS, EXPECTED, TABLES, DATA, static_ranges=STATIC_RANGES)
''' % (doc, out.replace('\\', '/').split('Novodex/')[-1], ',\n        '.join(rows),
       ',\n            '.join('0x%x: %r' % kv for kv in sorted(exp.items())),
       ', '.join('0x%x: %d' % kv for kv in tables.items()),
       ', '.join('(0x%x, %r, %r)' % (int(a, 16), s, c) for a, s, c in named),
       ', '.join('(0x%x, 0x%x, %r)' % (int(a, 16), int(b, 16), s) for a, b, s in ranges))
open(out, 'w', newline='\n').write(src)
print('wrote', out, len(rows), 'rows', len(exp), 'expected')
