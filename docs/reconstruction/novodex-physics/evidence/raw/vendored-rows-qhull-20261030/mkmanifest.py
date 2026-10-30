"""Write a close_static_rows --vendored manifest from a transcribe_rows config and vendored_coverage.csv.

usage: python mkmanifest.py <lib> <cfg.py> <rows file> <compare script> <listing-compare.txt> <out.json> [row id ...]

Each inventory row of the config (the ids in a ROWS tag) is listed with its symbol and its group, the function entry the
row lies in. Its family is the first family the group's coverage line records as `=exact`. Rows whose group is not
`family_best=exact` are printed and left out, as are rows not named when ids are given.
"""
import csv, json, re, sys

lib, cfgp, rows_file, compare, listing, out = sys.argv[1:7]
only = set(sys.argv[7:])
PREFER = ['container_copy', 'opcode_sphere', 'ice_plane_triangle', 'ice_matrix4x4', 'opcode_sap', 'opcode_model_build',
          'qhull_hull', 'qhull_output', 'qhull_merge', 'qhull_merge2', 'qhull_options', 'qhull_random', 'qhull_direct']
ROOT = 'D:/github/Novodex/docs/reconstruction/novodex-physics/'
ns = {}
exec(open(cfgp).read(), ns)
inv = {r['id']: r for r in json.load(open(ROOT + 'inventory.json', encoding='utf-8'))['functions']}
cov = {r['rva']: r for r in csv.DictReader(open(ROOT + 'evidence/phase4-third-party-map/vendored_coverage.csv', encoding='utf-8'))
       if r['library'] == lib}
match = {r['rva']: r for r in csv.DictReader(open(ROOT + 'evidence/phase4-third-party-map/%s_match.csv' % lib, encoding='utf-8'))}
text = open(listing).read()
insns = sum(int(x) for x in re.findall(r'instructions=(\d+)', text))
maps = int(re.search(r'MAPPINGS checked=(\d+)', text).group(1))
rows, held = {}, []
for sym, lo, hi, tag, decl in ns['ROWS']:
    group = '0x%08x' % lo
    for rid in re.findall(r'phys_fn_\d{6}', tag):
        if rid in rows or (only and rid not in only) or inv[rid]['state'] != 'discovered':
            continue
        line = cov.get(group)
        fams = re.findall(r'(\w+)=exact', line['differential']) if line else []
        # The family that drives the function itself, where there is one, before families that reach it in passing.
        fams.sort(key=lambda f: next((k for k, p in enumerate(PREFER) if f == p), len(PREFER)))
        if not line or line['family_best'] != 'exact' or not fams:
            held.append((rid, group, line and line['family_best']))
            continue
        rows[rid] = {'symbol': sym,  # close_static_rows matches a full mangled name, or `_` + a C name, exactly
                     'what': '%s %s' % (lib, match[inv[rid]['rva']]['source_function']),
                     'group': group, 'family': fams[0]}
json.dump({'source': rows_file, 'compare': compare, 'doc': 'evidence/vendored-correspondence.md',
           'instructions': insns, 'mappings': maps, 'rows': rows}, open(out, 'w'), indent=1)
print('manifest rows', len(rows), 'held', held)
