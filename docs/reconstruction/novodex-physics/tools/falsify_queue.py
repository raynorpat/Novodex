#!/usr/bin/env python3
"""Which code rows still lack a caught mutation, grouped by how they can be worked.

usage: python falsify_queue.py [--inventory inventory.json] [--group NAME] [--out FILE] [--summary]

A row is in the queue when it is a code row at `reconstructed`, `discovered` or `statically_reviewed`. Each row carries
the C++ file and symbol that implement it (`implementation`, `implementation_symbol`; `source` for a naked-asm
transcription), the gated target named in its dynamic proof, and a group that decides the method (see the plan,
docs/superpowers/plans/2026-11-03-falsify-remaining-rows.md).
"""
import argparse
import collections
import json
import re
from pathlib import Path

UNFINISHED = ('reconstructed', 'discovered', 'statically_reviewed')
TARGET = re.compile(r'Nx[A-Za-z0-9]+Tests')


def _int(value):
    if isinstance(value, int):
        return value
    return int(str(value), 16) if str(value).lower().startswith('0x') else int(value)


def cpp_file(row):
    return row.get('implementation') or row.get('source') or ''


def group_of(row):
    third = row.get('third_party')
    if third == 'qhull':
        return 'vendored_qhull'
    if third == 'opcode':
        return 'vendored_opcode'
    if not row.get('implementation') and not row.get('source'):
        return 'no_mapping'
    if not row.get('dynamic_proof'):
        return 'no_driver'
    name = cpp_file(row).replace('\\', '/').split('/')[-1]
    if 'Joint' in name:
        return 'joints'
    if name.startswith(('Contact', 'Distance', 'Gjk')):
        return 'collision'
    if 'Mesh' in name or name.startswith(('Quantizer', 'PMap')):
        return 'mesh'
    if 'Pruner' in name or name.startswith('Ice'):
        return 'pruner'
    if name.startswith('Scene'):
        return 'scene'
    return 'other_first_party'


def build_queue(inventory):
    out = []
    for r in inventory['functions']:
        if r.get('kind') != 'code' or r.get('state') not in UNFINISHED:
            continue
        m = TARGET.search(str(r.get('dynamic_proof') or ''))
        out.append({
            'id': r['id'], 'rva': _int(r['rva']), 'size': _int(r['size']), 'phase': r.get('phase'), 'state': r['state'],
            'source': cpp_file(r), 'symbol': r.get('implementation_symbol'), 'third_party': r.get('third_party'),
            'target': m.group(0) if m else None, 'group': group_of(r), 'dynamic_proof': str(r.get('dynamic_proof') or '')})
    return out


def batches(rows, size):
    """Batches of at most `size` rows in which no two rows share a target (so a detection can be attributed)."""
    buckets = collections.OrderedDict()
    for r in rows:
        buckets.setdefault((r.get('target'), r.get('family')), []).append(r)
    out = []
    while any(buckets.values()):
        order = sorted((k for k in buckets if buckets[k]), key=lambda k: -len(buckets[k]))[:size]
        out.append([buckets[k].pop(0) for k in order])
    return out


FAMILY_TARGETS = ('NxPhysicsCollisionTests', 'NxPhysicsPhase6RowsTests', 'NxPhysicsThirdPartyTests', 'NxPhysicsAssetTests')


def family_of(row, families):
    """The one family of the row's target that its proof text names, or None. A family name is an identifier with an
    underscore or a dot in it (the generic words `build`, `compound` are not)."""
    known = families.get(row.get('target')) or ()
    tokens = set(re.findall(r'[A-Za-z][A-Za-z0-9_.]{3,}', row.get('dynamic_proof') or ''))
    tokens = {t.rstrip('.') for t in tokens}
    found = {t for t in tokens if t in known and ('_' in t or '.' in t)}
    return next(iter(found)) if len(found) == 1 else None


def plans(rows, staged, size, oracle=(), families=None):
    """(batch plans, unplanned): plan rows for mutate_cpp --plan. A staged-pair target is detected by its stdout delta; an
    oracle-differential target whose output is not read by family is detected by its output differing from the unmutated
    run (`oracle_only`); a family target needs a family chosen by hand and is unplanned, with the reason."""
    ok, unplanned = [], []
    for r in rows:
        t = r.get('target')
        if not t:
            unplanned.append({'id': r['id'], 'why': 'no gated target named'})
        elif t in staged:
            ok.append({'id': r['id'], 'target': t, 'staged': True, 'count_crash': True})
        elif t in FAMILY_TARGETS and t in oracle:
            fam = family_of(r, families or {})
            if fam:
                ok.append({'id': r['id'], 'target': t, 'family': fam, 'count_crash': True})
            else:
                unplanned.append({'id': r['id'], 'why': 'target %s needs a family (its output is read by family)' % t})
        elif t in oracle:
            ok.append({'id': r['id'], 'target': t, 'oracle_only': True, 'count_crash': True})
        else:
            unplanned.append({'id': r['id'], 'why': 'target %s is not a staged-pair gate' % t})
    return batches(ok, size), unplanned


def vendored_plans(rows, coverage, names, size):
    """Batch plans for vendored rows gated by NxPhysicsThirdPartyTests: {id, target, lines: true, families}. A row's families
    are the ones its coverage row names that the target prints; rows whose families overlap never share a batch."""
    import vendor_gate
    plan_rows = []
    for r in rows:
        diff = coverage.get((r.get('third_party'), '0x%08x' % r['rva']), '')
        plan_rows.append({'id': r['id'], 'target': 'NxPhysicsThirdPartyTests', 'lines': True,
                          'families': vendor_gate.row_families(diff, names)})
    return vendor_gate.pack(plan_rows, size)


def main(argv=None):
    here = Path(__file__).resolve().parents[1]
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--inventory', default=str(here / 'inventory.json'))
    ap.add_argument('--group')
    ap.add_argument('--out')
    ap.add_argument('--summary', action='store_true')
    ap.add_argument('--plan-dir', help='write plan-<k>.json (staged-pair rows, no two sharing a target) and unplanned.json')
    ap.add_argument('--batch-size', type=int, default=3)
    ap.add_argument('--vendored-plans', help='write plan-<k>.json for the vendored rows of the queue (needs --coverage and --names)')
    ap.add_argument('--coverage', help='evidence/phase4-third-party-map/vendored_coverage.csv')
    ap.add_argument('--names', help='the family names NxPhysicsThirdPartyTests prints, one per line')
    ap.add_argument('--families', help='a file with the family names the target prints, one per line (NxPhysicsCollisionTests default run)')
    ap.add_argument('--force-target', help='the gated target to try for rows whose proof names none')
    ap.add_argument('--exclude-log', action='append', default=[], help='skip rows that already have a SURVEY line in this mutate_cpp log')
    a = ap.parse_args(argv)
    queue = build_queue(json.load(open(a.inventory, encoding='utf-8')))
    if a.group:
        queue = [r for r in queue if r['group'] == a.group]
    if a.force_target:
        queue = [dict(r, target=r['target'] or a.force_target) for r in queue]
    if a.exclude_log:
        done = set()
        for log in a.exclude_log:
            done |= set(re.findall(r'^SURVEY (phys_fn_\d+) ', open(log, encoding='utf-8').read(), re.M))
        queue = [r for r in queue if r['id'] not in done]
    if a.summary or not a.out:
        counts = collections.Counter(r['group'] for r in queue)
        sizes = collections.Counter()
        for r in queue:
            sizes[r['group']] += r['size']
        for g, n in counts.most_common():
            print('%-18s rows=%5d bytes=%7d' % (g, n, sizes[g]))
        print('%-18s rows=%5d bytes=%7d' % ('total', len(queue), sum(sizes.values())))
    if a.vendored_plans:
        import csv
        cov = {(c['library'], c['rva']): c['differential'] for c in csv.DictReader(open(a.coverage, encoding='utf-8'))}
        names = set(x.strip() for x in open(a.names, encoding='utf-8') if x.strip())
        pl = vendored_plans([r for r in queue if r.get('third_party')], cov, names, a.batch_size)
        Path(a.vendored_plans).mkdir(parents=True, exist_ok=True)
        for k, rows in enumerate(pl):
            (Path(a.vendored_plans) / ('plan-%03d.json' % k)).write_text(json.dumps({'rows': rows}, indent=1), encoding='utf-8')
        print('%d vendored plans' % len(pl))
    if a.plan_dir:
        import close_static_rows
        oracle, staged = close_static_rows.gate_targets(str(Path(a.inventory).parent / 'tools' / 'gate_targets.ps1'))
        fams = {'NxPhysicsCollisionTests': set(x.strip() for x in open(a.families, encoding='utf-8'))} if a.families else {}
        pl, un = plans(queue, set(staged), a.batch_size, set(oracle), fams)
        Path(a.plan_dir).mkdir(parents=True, exist_ok=True)
        for k, rows in enumerate(pl):
            (Path(a.plan_dir) / ('plan-%02d.json' % k)).write_text(json.dumps({'rows': rows}, indent=1), encoding='utf-8')
        (Path(a.plan_dir) / 'unplanned.json').write_text(json.dumps(un, indent=1), encoding='utf-8')
        print('%d plans, %d unplanned' % (len(pl), len(un)))
    if a.out:
        Path(a.out).write_text(json.dumps(queue, indent=1), encoding='utf-8')
        print('wrote %d rows to %s' % (len(queue), a.out))


if __name__ == '__main__':
    main()
