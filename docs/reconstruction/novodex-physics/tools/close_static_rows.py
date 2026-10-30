#!/usr/bin/env python3
"""Record first-party rows reconstructed with a static proof only (the remaining-rows pass).

usage: python close_static_rows.py --manifest <m.json> [--inventory inventory.json] [--closure-dir gates]
                                   [--map ../../../build/Release/NxPhysics.map]

The manifest:
  {"source": "Physics/src/<area>/<Topic>Rows.cpp", "compare": "evidence/phase<N>-<topic>-listing-compare.py",
   "doc": "evidence/phase<N>-<topic>.md", "instructions": <int>, "mappings": <int>,
   "rows": {"phys_fn_NNNNNN": {"symbol": "<candidate function name>", "what": "<what the row does>"}}}

Each row must be `discovered` and first-party (vendored rows follow the vendored promotion policy). It becomes
`reconstructed` with its source, symbol, map reference and static proof, and no dynamic proof; its closure entry
moves from `not_reconstructed_in_phase` to `reconstructed_not_falsified` with the ledger's counts updated.
"""
import argparse
import json
import re
from pathlib import Path

MAP_PATH = 'build/Release/NxPhysics.map'
AWAITS = 'awaits the testing pass'


def read_map(path):
    """{symbol: (address, object)} from an MSVC linker map."""
    out = {}
    for line in open(path, encoding='latin-1'):
        m = re.match(r'\s*000[1-9]:[0-9a-f]+\s+(\S+)\s+([0-9a-f]{8})\s+(?:f\s+)?(?:i\s+)?(\S+)', line)
        if m:
            out.setdefault(m.group(1), ('0x' + m.group(2), m.group(3)))
    return out


def find_symbol(name, mapsyms):
    found = [k for k in mapsyms if k.startswith('?' + name + '@@') or k in ('_' + name, name)]
    if len(found) != 1:
        raise ValueError('symbol %s: %d matches in the map %s' % (name, len(found), found[:4]))
    return found[0]


def close(manifest, inventory, closures, mapsyms):
    by = {r['id']: r for r in inventory['functions']}
    for rid, spec in manifest['rows'].items():
        r = by[rid]
        if r.get('third_party'):
            raise ValueError('%s is vendored (%s): it follows the vendored promotion policy' % (rid, r['third_party']))
        if r['state'] != 'discovered':
            raise ValueError('%s is %s, not discovered' % (rid, r['state']))
        sym = find_symbol(spec['symbol'], mapsyms)
        address, obj = mapsyms[sym]
        r['state'] = 'reconstructed'
        r['source'] = manifest['source']
        r['implementation'] = manifest['source']
        r['implementation_symbol'] = sym
        r['candidate_map_reference'] = {'path': MAP_PATH, 'symbol': sym, 'address': address, 'object': obj}
        r['static_proof'] = ('Capstone listing at RVA %s: %s. Transcribed instruction for instruction into a naked row in %s; '
                             '%s disassembles the built candidate and the oracle and finds the %d instructions of its rows equal, '
                             'with the %d call mappings as expected. See %s.'
                             % (r['rva'], spec['what'], manifest['source'], manifest['compare'], manifest['instructions'],
                                manifest['mappings'], manifest['doc']))
        r['dynamic_proof'] = None
        r['notes'] = 'Listing transcription with a static proof only; it %s (no differential, no mutation yet).' % AWAITS
        phase = int(r['phase'])
        ledger = closures[phase]
        entries = [e for e in ledger['deferred'] if e['id'] == rid]
        if len(entries) != 1:
            raise ValueError('%s: %d deferred entries in the phase %d closure' % (rid, len(entries), phase))
        e = entries[0]
        if e['reason'] not in ('not_reconstructed_in_phase', 'blocked_on_later_phase'):
            raise ValueError('%s is deferred as %s' % (rid, e['reason']))
        counts = ledger['counts']
        counts['deferred_' + e['reason']] -= 1
        counts['deferred_reconstructed_not_falsified'] = counts.get('deferred_reconstructed_not_falsified', 0) + 1
        e['reason'] = 'reconstructed_not_falsified'
        e['note'] = ('Transcribed from its listing (%s) with a static proof only; it %s, so no mutation has been aimed '
                     'at it yet.' % (manifest['doc'], AWAITS))
    return inventory, closures


def _read(p):
    raw = open(p, encoding='utf-8', newline='').read()
    return json.loads(raw), ('\r\n' if '\r\n' in raw else '\n')


def _write(p, obj, nl):
    open(p, 'w', encoding='utf-8', newline='').write((json.dumps(obj, indent=2, ensure_ascii=False) + '\n').replace('\n', nl))


def main(argv=None):
    here = Path(__file__).resolve().parents[1]
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--manifest', required=True)
    ap.add_argument('--inventory', default=str(here / 'inventory.json'))
    ap.add_argument('--closure-dir', default=str(here / 'gates'))
    ap.add_argument('--map', default=str(here.parents[2] / MAP_PATH))
    a = ap.parse_args(argv)
    manifest = json.load(open(a.manifest, encoding='utf-8'))
    inventory, inl = _read(a.inventory)
    phases = {int(r['phase']) for r in inventory['functions'] if r['id'] in manifest['rows']}
    closures, cnl = {}, {}
    for ph in phases:
        closures[ph], cnl[ph] = _read(Path(a.closure_dir) / ('phase%d-closure.json' % ph))
    close(manifest, inventory, closures, read_map(a.map))
    _write(a.inventory, inventory, inl)
    for ph in phases:
        _write(Path(a.closure_dir) / ('phase%d-closure.json' % ph), closures[ph], cnl[ph])
    print('closed %d rows; phases %s' % (len(manifest['rows']), sorted(phases)))


if __name__ == '__main__':
    main()
