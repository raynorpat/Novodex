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
import csv
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


def exact_family(rid, lib, spec, coverage):
    """The coverage line proving a vendored row: its group must be exact in the manifest's family."""
    line = coverage.get((lib, spec['group']))
    if line is None:
        raise ValueError('%s: no coverage line for %s group %s' % (rid, lib, spec['group']))
    if line['family_best'] != 'exact':
        raise ValueError('%s: group %s is family_best=%s, not exact' % (rid, spec['group'], line['family_best']))
    # The cited family must belong to an execution (one `;` chunk: `harness:fam=cls+sibling=cls`) in which every
    # family is exact, and that execution must be the row's own code, not a copy inlined into a caller.
    for chunk in line['differential'].split(';'):
        if 'via-inline' in chunk:
            continue
        pairs = re.findall(r'([\w.]+)=(\w+)', chunk.split(':', 1)[-1])
        if any(name == spec['family'] for name, _ in pairs) and all(cls == 'exact' for _, cls in pairs):
            break
    else:
        raise ValueError('%s: no execution of group %s in which %s and every sibling are exact'
                         % (rid, spec['group'], spec['family']))
    return line


def divergent_families(line):
    """The families of a coverage line whose class is not exact, as `name (class)`, in order, once each."""
    seen = []
    for name, cls in re.findall(r'(\w+)=(lastbit|discrete|layout|uncompared)', line['differential']):
        item = '%s (%s)' % (name, cls)
        if item not in seen:
            seen.append(item)
    return seen


def close(manifest, inventory, closures, mapsyms, coverage=None):
    """First-party rows with a static proof; with `coverage`, vendored rows with an exact execution too."""
    by = {r['id']: r for r in inventory['functions']}
    vendored = coverage is not None
    for rid, spec in manifest['rows'].items():
        r = by[rid]
        if bool(r.get('third_party')) != vendored:
            raise ValueError('%s is %s: %s' % (rid, 'vendored (%s)' % r['third_party'] if r.get('third_party') else 'first-party',
                                               'it needs --vendored' if r.get('third_party') else 'not a --vendored row'))
        line = exact_family(rid, r['third_party'], spec, coverage) if vendored else None
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
        if vendored:
            lib = r['third_party']
            r['static_proof'] += ' Vendored %s function, upstream body guarded by NX_VENDORED_LISTING_ROWS.' % lib
            others = divergent_families(line)
            r['dynamic_proof'] = ('cdb trace, evidence/vendored-trace-%s.txt: family %s exact (every word compared with the '
                                  'oracle); vendored_coverage.csv group %s family_best=exact.' % (lib, spec['family'], spec['group'])
                                  + (' Families it also ran in that are not exact, each attributed in '
                                     'evidence/vendored-correspondence.md (vendored-rows pass): %s.' % ', '.join(others)
                                     if others else ' Every family it ran in is exact.'))
            held = r.get('notes') or ''
            r['notes'] = 'Vendored row transcribed and executed (vendored-rows pass).' + (
                ' Earlier: ' + held if held.startswith('Held back') else '')
        else:
            r['dynamic_proof'] = None
            r['notes'] = 'Listing transcription with a static proof only; it %s (no differential, no mutation yet).' % AWAITS
        phase = int(r['phase'])
        ledger = closures[phase]
        entries = [e for e in ledger['deferred'] if e['id'] == rid]
        if len(entries) != 1:
            raise ValueError('%s: %d deferred entries in the phase %d closure' % (rid, len(entries), phase))
        e = entries[0]
        if e['reason'] not in (('vendored_not_falsified',) if vendored else ('not_reconstructed_in_phase', 'blocked_on_later_phase')):
            raise ValueError('%s is deferred as %s' % (rid, e['reason']))
        counts = ledger['counts']
        if counts.get('deferred_' + e['reason'], 0) <= 0:
            raise ValueError('phase %d closure count deferred_%s is already %s' % (phase, e['reason'], counts.get('deferred_' + e['reason'])))
        counts['deferred_' + e['reason']] -= 1
        counts['deferred_reconstructed_not_falsified'] = counts.get('deferred_reconstructed_not_falsified', 0) + 1
        e['reason'] = 'reconstructed_not_falsified'
        e['note'] = ('Transcribed from its listing (%s) and executed exactly against the oracle in %s.'
                     % (manifest['doc'], spec['family']) if vendored else
                     'Transcribed from its listing (%s) with a static proof only; it %s, so no mutation has been aimed '
                     'at it yet.' % (manifest['doc'], AWAITS))
    return inventory, closures


def falsify(manifest, inventory, closures, oracle_targets, staged_targets):
    """Testing pass: a wired, driven row whose mutation a gate caught becomes dynamically_gated and its ledger entry closes.

    Row manifest: {"gate": target, "family": name, "mutation": text, "detected": "mismatches=N" | "stdout_delta=N"}.
    The proof kind follows the detection: mismatches= closes as oracle_differential_falsified (the gate must be a
    registered oracle-differential target), stdout_delta= as differential_falsified (a staged-pair target).
    """
    by = {r['id']: r for r in inventory['functions']}
    for rid, spec in manifest['rows'].items():
        r = by[rid]
        m = re.fullmatch(r'(mismatches|stdout_delta)=([1-9][0-9]*)', spec['detected'])
        if not m:
            raise ValueError('%s: detection %r is not mismatches=N or stdout_delta=N with N > 0' % (rid, spec['detected']))
        proof, registered = (('oracle_differential_falsified', oracle_targets) if m.group(1) == 'mismatches'
                             else ('differential_falsified', staged_targets))
        if spec['gate'] not in registered:
            raise ValueError('%s: gate %s is not a registered %s target' % (rid, spec['gate'], proof))
        gate_phases = sorted(registered[spec['gate']])
        if r['state'] != 'reconstructed' or AWAITS not in (r.get('notes') or ''):
            raise ValueError('%s is %s and does not await the testing pass' % (rid, r['state']))
        r['state'] = 'dynamically_gated'
        r['dynamic_proof'] = ('%s (%s): a mutation inside the row itself (%s) is detected (%s). See %s.'
                              % (spec['gate'], spec['family'], spec['mutation'], spec['detected'], manifest['doc']))
        r['notes'] = 'Listing transcription, wired and driven; falsified by the mutation above (testing pass).'
        phase = int(r['phase'])
        ledger = closures[phase]
        entries = [e for e in ledger['deferred'] if e['id'] == rid]
        if len(entries) != 1 or entries[0]['reason'] != 'reconstructed_not_falsified':
            raise ValueError('%s: not a reconstructed_not_falsified entry in the phase %d closure' % (rid, phase))
        counts = ledger['counts']
        if counts.get('deferred_reconstructed_not_falsified', 0) <= 0:
            raise ValueError('phase %d closure count deferred_reconstructed_not_falsified is already 0' % phase)
        e = entries[0]
        closed = {'id': rid, 'rva': e['rva'], 'proof': proof, 'gate': spec['gate'],
                  'falsification': {'mutation': spec['mutation'], 'detected': spec['detected']}}
        if phase not in gate_phases:
            # the gate does not run when this phase is gated: a later phase that does run it discharges the row
            later = [p for p in gate_phases if p > phase]
            if not later:
                raise ValueError('%s: gate %s runs in phases %s, none later than phase %d'
                                 % (rid, spec['gate'], gate_phases, phase))
            closed['discharged_by_phase'] = later[0]
            closed['driving_phases'] = later
        ledger['deferred'].remove(e)
        counts['deferred_reconstructed_not_falsified'] -= 1
        counts[proof] = counts.get(proof, 0) + 1
        ledger.setdefault('closed', []).append(closed)
    return inventory, closures


def bump_gate(artifact, n):
    """gates/phase<N>.json: n rows move from deferred to closed."""
    artifact['counts']['closed'] += n
    artifact['counts']['deferred'] -= n
    return artifact


def bump_program(program, closed_by_phase):
    """program.json's per-phase record: each closed row moves from remaining_functions to closed_functions."""
    for rec in program['phases']:
        if rec.get('closed_functions') is None:
            continue  # an evidence-only phase keeps no counts
        n = closed_by_phase.get(rec['phase'], 0)
        rec['closed_functions'] += n
        rec['remaining_functions'] -= n
    return program


def gate_targets(path):
    """({oracle-differential target: its phases}, {staged-pair target: its phases}) read from gate_targets.ps1."""
    text = open(path, encoding='utf-8-sig').read()

    def phases(variable):
        block = re.search(r'\$%s\s*=\s*\[ordered\]\s*@\{(.*?)\r?\n\}' % variable, text, re.S).group(1)
        out = {}
        for number, names in re.findall(r"'(\d)'\s*=\s*@\((.*?)\)", block, re.S):
            for name in re.findall(r"'(Nx\w+)'", names):
                out.setdefault(name, []).append(int(number))
        return out
    return phases('NxPhaseOracleDifferentialTargets'), phases('NxPhaseTestTargets')


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
    ap.add_argument('--vendored', action='store_true', help='vendored rows: each group must be exact in its family')
    ap.add_argument('--falsified', action='store_true', help='testing pass: record wired, driven rows with a caught mutation')
    ap.add_argument('--coverage', default=str(here / 'evidence' / 'phase4-third-party-map' / 'vendored_coverage.csv'))
    a = ap.parse_args(argv)
    coverage = None
    if a.vendored:
        coverage = {(c['library'], c['rva']): c for c in csv.DictReader(open(a.coverage, encoding='utf-8'))}
    manifest = json.load(open(a.manifest, encoding='utf-8'))
    inventory, inl = _read(a.inventory)
    phases = {int(r['phase']) for r in inventory['functions'] if r['id'] in manifest['rows']}
    closures, cnl = {}, {}
    for ph in phases:
        closures[ph], cnl[ph] = _read(Path(a.closure_dir) / ('phase%d-closure.json' % ph))
    if a.falsified:
        oracle, staged = gate_targets(str(here / 'tools' / 'gate_targets.ps1'))
        falsify(manifest, inventory, closures, oracle, staged)
        program, pnl = _read(str(here / 'program.json'))
        closed_by_phase = {ph: sum(1 for r in inventory['functions'] if r['id'] in manifest['rows'] and int(r['phase']) == ph)
                           for ph in phases}
        bump_program(program, closed_by_phase)
        _write(str(here / 'program.json'), program, pnl)
        for ph, n in closed_by_phase.items():
            artifact, anl = _read(str(Path(a.closure_dir) / ('phase%d.json' % ph)))
            bump_gate(artifact, n)
            _write(str(Path(a.closure_dir) / ('phase%d.json' % ph)), artifact, anl)
    else:
        close(manifest, inventory, closures, read_map(a.map), coverage)
    _write(a.inventory, inventory, inl)
    for ph in phases:
        _write(Path(a.closure_dir) / ('phase%d-closure.json' % ph), closures[ph], cnl[ph])
    print('closed %d rows; phases %s' % (len(manifest['rows']), sorted(phases)))


if __name__ == '__main__':
    main()
