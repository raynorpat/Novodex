#!/usr/bin/env python3
"""Record why a row has no caught mutation: move its closure-ledger entry to a deferral reason, with a note.

usage: python defer_rows.py --manifest <m.json> [--inventory inventory.json] [--closure-dir gates]

The manifest:
  {"doc": "evidence/falsify-<group>.md",
   "rows": {"phys_fn_NNNNNN": {"reason": "<one of the ledger reasons>", "note": "<what was tried and what it showed>"}}}

The reason may be the entry's own (the note then records a partial survey, such as "5 caught, 4 survived"), or "keep". The ledger's
`deferred_<reason>` counts follow the move, and the row's inventory notes name the reason and the evidence file.
"""
import argparse
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from close_static_rows import _read, _write  # noqa: E402

REASONS = ('reconstructed_not_falsified', 'not_independently_falsifiable', 'homeless_shared_code', 'unreachable_in_phase_2',
           'not_reconstructed_in_phase', 'vendored_not_falsified', 'vendored_driven_divergent', 'blocked_on_later_phase')
VENDORED = ('vendored_not_falsified', 'vendored_driven_divergent')


def defer(manifest, inventory, closures):
    by = {r['id']: r for r in inventory['functions']}
    for rid, spec in manifest['rows'].items():
        reason, note = spec.get('reason'), (spec.get('note') or '').strip()
        if reason == 'keep':    # the entry keeps whatever reason it has; only its note changes
            kept = [e['reason'] for e in closures[int(by[rid]['phase'])]['deferred'] if e['id'] == rid]
            reason = kept[0] if kept else reason
        if reason not in REASONS:
            raise ValueError('%s: %r is not a deferral reason' % (rid, reason))
        if not note:
            raise ValueError('%s: a deferral needs a note saying what was tried' % (rid,))
        row = by[rid]
        if reason in VENDORED and not row.get('third_party'):
            raise ValueError('%s: %s is for a vendored row' % (rid, reason))
        if row.get('third_party') and row['state'] == 'discovered' and reason not in VENDORED:
            raise ValueError('%s: a vendored row at discovered gives %s' % (rid, ' or '.join(VENDORED)))
        ledger = closures[int(row['phase'])]
        entries = [e for e in ledger['deferred'] if e['id'] == rid]
        if len(entries) != 1:
            raise ValueError('%s: not a deferred entry in the phase %s closure' % (rid, row['phase']))
        e = entries[0]
        if e['reason'] != reason:
            counts = ledger['counts']
            counts['deferred_' + e['reason']] -= 1
            counts['deferred_' + reason] = counts.get('deferred_' + reason, 0) + 1
            e['reason'] = reason
        e['note'] = note
        row['notes'] = ('Deferred (%s): %s See %s.' % (reason, note, manifest['doc']))
    return inventory, closures


def main(argv=None):
    here = Path(__file__).resolve().parents[1]
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--manifest', required=True)
    ap.add_argument('--inventory', default=str(here / 'inventory.json'))
    ap.add_argument('--closure-dir', default=str(here / 'gates'))
    a = ap.parse_args(argv)
    manifest = json.load(open(a.manifest, encoding='utf-8'))
    inventory, inl = _read(a.inventory)
    phases = {int(r['phase']) for r in inventory['functions'] if r['id'] in manifest['rows']}
    closures, cnl = {}, {}
    for ph in phases:
        closures[ph], cnl[ph] = _read(Path(a.closure_dir) / ('phase%d-closure.json' % ph))
    defer(manifest, inventory, closures)
    _write(a.inventory, inventory, inl)
    for ph in phases:
        _write(Path(a.closure_dir) / ('phase%d-closure.json' % ph), closures[ph], cnl[ph])
    print('deferred %d rows; phases %s' % (len(manifest['rows']), sorted(phases)))


if __name__ == '__main__':
    main()
