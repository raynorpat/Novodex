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
            'target': m.group(0) if m else None, 'group': group_of(r)})
    return out


def batches(rows, size):
    """Batches of at most `size` rows in which no two rows share a target (so a detection can be attributed)."""
    buckets = collections.OrderedDict()
    for r in rows:
        buckets.setdefault(r.get('target'), []).append(r)
    out = []
    while any(buckets.values()):
        order = sorted((k for k in buckets if buckets[k]), key=lambda k: -len(buckets[k]))[:size]
        out.append([buckets[k].pop(0) for k in order])
    return out


def main(argv=None):
    here = Path(__file__).resolve().parents[1]
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--inventory', default=str(here / 'inventory.json'))
    ap.add_argument('--group')
    ap.add_argument('--out')
    ap.add_argument('--summary', action='store_true')
    a = ap.parse_args(argv)
    queue = build_queue(json.load(open(a.inventory, encoding='utf-8')))
    if a.group:
        queue = [r for r in queue if r['group'] == a.group]
    if a.summary or not a.out:
        counts = collections.Counter(r['group'] for r in queue)
        sizes = collections.Counter()
        for r in queue:
            sizes[r['group']] += r['size']
        for g, n in counts.most_common():
            print('%-18s rows=%5d bytes=%7d' % (g, n, sizes[g]))
        print('%-18s rows=%5d bytes=%7d' % ('total', len(queue), sum(sizes.values())))
    if a.out:
        Path(a.out).write_text(json.dumps(queue, indent=1), encoding='utf-8')
        print('wrote %d rows to %s' % (len(queue), a.out))


if __name__ == '__main__':
    main()
