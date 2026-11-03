#!/usr/bin/env python3
"""Take rows back from `dynamically_gated` to the state a git ref had them in, when their closing evidence was wrong.

usage: python reopen_rows.py --ids phys_fn_000001,phys_fn_000002 [--base main]

Each row's inventory record and its deferred closure entry are restored from the base ref, its closed entry is removed, the
ledger counts, program.json's per-phase counts and the gate artifacts' counts follow, and the line the closer published
into the phase's evidence file is dropped. The rows can then be closed again with correct evidence.
"""
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from close_static_rows import _read, _write  # noqa: E402


def drop_evidence_lines(text, ids):
    keep = [l for l in text.split(chr(10)) if not any(re.match(r'- `%s`:' % re.escape(i), l) for i in ids)]
    return chr(10).join(keep)


def reopen(ids, inventory, closures, base_inventory, base_closures, program):
    by = {r['id']: r for r in inventory['functions']}
    base_by = {r['id']: r for r in base_inventory['functions']}
    for rid in ids:
        row = by[rid]
        if row['state'] not in ('dynamically_gated', 'closed'):
            raise ValueError('%s is %s, not a closed row' % (rid, row['state']))
        phase = int(row['phase'])
        ledger = closures[phase]
        entries = [e for e in ledger['closed'] if e['id'] == rid]
        if len(entries) != 1:
            raise ValueError('%s: no closed entry in the phase %d closure' % (rid, phase))
        base_entries = [e for e in base_closures[phase]['deferred'] if e['id'] == rid]
        if len(base_entries) != 1:
            raise ValueError('%s: the base ref has no deferred entry for it' % rid)
        ledger['closed'].remove(entries[0])
        ledger['counts'][entries[0]['proof']] -= 1
        entry = dict(base_entries[0])
        ledger['deferred'].append(entry)
        key = 'deferred_' + entry['reason']
        ledger['counts'][key] = ledger['counts'].get(key, 0) + 1
        row.clear()
        row.update(base_by[rid])
        for rec in program['phases']:
            if rec['phase'] == phase and rec.get('closed_functions') is not None:
                rec['closed_functions'] -= 1
                rec['remaining_functions'] += 1
    return inventory, closures


def git_json(ref, path):
    out = subprocess.run(['git', 'show', '%s:%s' % (ref, path)], capture_output=True, check=True).stdout
    return json.loads(out.decode('utf-8'))


def main(argv=None):
    here = Path(__file__).resolve().parents[1]
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--ids', required=True)
    ap.add_argument('--base', default='main')
    a = ap.parse_args(argv)
    ids = [i for i in a.ids.split(',') if i]
    inventory, inl = _read(str(here / 'inventory.json'))
    base_inventory = git_json(a.base, 'docs/reconstruction/novodex-physics/inventory.json')
    phases = sorted({int(r['phase']) for r in inventory['functions'] if r['id'] in ids})
    closures, cnl, base_closures = {}, {}, {}
    for ph in phases:
        name = 'phase%d-closure.json' % ph
        closures[ph], cnl[ph] = _read(str(here / 'gates' / name))
        base_closures[ph] = git_json(a.base, 'docs/reconstruction/novodex-physics/gates/' + name)
    program, pnl = _read(str(here / 'program.json'))
    by_phase = {ph: sum(1 for r in inventory['functions'] if r['id'] in ids and int(r['phase']) == ph) for ph in phases}
    reopen(ids, inventory, closures, base_inventory, base_closures, program)
    _write(str(here / 'inventory.json'), inventory, inl)
    for ph in phases:
        _write(str(here / 'gates' / ('phase%d-closure.json' % ph)), closures[ph], cnl[ph])
        gate = here / 'gates' / ('phase%d.json' % ph)
        if gate.exists():
            artifact, gnl = _read(str(gate))
            artifact['counts']['closed'] -= by_phase[ph]
            artifact['counts']['deferred'] += by_phase[ph]
            _write(str(gate), artifact, gnl)
        ev = closures[ph].get('evidence_file')
        if ev and (here / ev).exists():
            p = here / ev
            p.write_text(drop_evidence_lines(p.read_text(encoding='utf-8'), ids), encoding='utf-8')
    _write(str(here / 'program.json'), program, pnl)
    print('reopened %d rows; phases %s' % (len(ids), phases))


if __name__ == '__main__':
    main()
