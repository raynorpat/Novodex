#!/usr/bin/env python
"""Audit every oracle-row call in the layout harness for a calling-convention
mismatch between its typedef and the row's own `ret N`.

A `ret N` row pops N bytes of arguments itself. Declaring it with a convention
whose cleanup disagrees with N makes every call move ESP by the difference, and
the error accumulates across a block. It is invisible until the drift walks the
frame into a fixture buffer, and then it presents as a jump through whatever byte
pattern that buffer held.

This reads the row's real cleanup from the pinned DLL and compares it with the
typedef the harness writes. It is the instrument that found
`DelimScanOracle = __cdecl` against a row ending in `ret 4`
(0x90db0) -- one of 24 such mismatches in this translation unit.

Scope: reads only. It never edits the harness.
"""

import argparse
import json
import re
import sys
from pathlib import Path

import capstone

DEFAULT_ORACLE_ROOT = Path(r'D:\FlamingEnt__\Unreal_3')

TD = re.compile(
    r'typedef\s+[\w\s\*]+\(\s*(__cdecl|__stdcall|__fastcall|__thiscall)\s*\*\s*(\w+)\s*\)'
    r'\s*\(([^)]*)\)\s*;')
CAST = re.compile(r'reinterpret_cast<(\w+)>\s*\(\s*base\s*\+\s*(0x[0-9a-fA-F]+)\s*\)')


def row_cleanup(code, md, rva):
    """Bytes the row's own terminating `ret` pops, or None if not decidable."""
    for ins in md.disasm(code, 0x10000000 + rva):
        if ins.mnemonic == 'ret':
            return int(ins.op_str, 0) if ins.op_str else 0
        if ins.mnemonic == 'jmp':
            return None
    return None


def input_paths(repo_root, oracle_root):
    """Resolve the harness, oracle DLL, and PE map for selected checkouts."""
    repo_root = Path(repo_root)
    oracle_root = Path(oracle_root)
    return (
        repo_root / 'tests' / 'PhysicsObjectLayoutTests.cpp',
        oracle_root / 'Binaries' / 'NxPhysics.dll',
        repo_root / 'docs' / 'reconstruction' / 'novodex-physics' / 'oracle' / 'pe.json',
    )


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        '--repo-root', type=Path, default=Path(__file__).resolve().parents[4],
        help='Novodex checkout to audit (default: inferred from this tool)',
    )
    parser.add_argument(
        '--oracle-root', type=Path, default=DEFAULT_ORACLE_ROOT,
        help='UE3 root containing Binaries/NxPhysics.dll',
    )
    args = parser.parse_args(argv)
    harness_path, dll_path, pe_path = input_paths(args.repo_root, args.oracle_root)

    pe = json.loads(pe_path.read_text(encoding='utf-8'))
    secs = pe['sections']
    raw = dll_path.read_bytes()

    def rva_to_off(rva):
        for s in secs:
            if s['rva'] <= rva < s['rva'] + max(s['virtual_size'], s['raw_size']):
                return s['raw_offset'] + (rva - s['rva'])
        return None

    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

    # The layout translation unit is committed as UTF-16LE with a BOM (it is the
    # only test source that is), so decode by its own BOM rather than assuming
    # UTF-8.
    rawtext = harness_path.read_bytes()
    if rawtext[:2] in (b'\xff\xfe', b'\xfe\xff'):
        text = rawtext.decode('utf-16')
    else:
        text = rawtext.decode('utf-8-sig')
    text = text.replace('\r\n', '\n')
    lines = text.split('\n')

    typedefs = {}
    for i, line in enumerate(lines, 1):
        m = TD.search(line)
        if m:
            typedefs[m.group(2)] = (m.group(1), m.group(3), i)

    casts = {}
    for i, line in enumerate(lines, 1):
        m = CAST.search(line)
        if m:
            casts.setdefault(m.group(1), []).append((int(m.group(2), 16), i))

    # The name is reused with different signatures in different scopes, so the
    # typedef that governs a use site is the last one declared at or before it.
    decls = []
    for i, line in enumerate(lines, 1):
        m = TD.search(line)
        if m:
            decls.append((i, m.group(2), m.group(1), m.group(3)))

    def governing(use_line, name):
        best = None
        for line, nm, conv, args in decls:
            if nm == name and line <= use_line:
                if best is None or line > best[0]:
                    best = (line, conv, args)
        return best

    mismatches = []
    undecided = []
    for name, uses in sorted(casts.items()):
        for rva, uline in uses:
            gov = governing(uline, name)
            if gov is None:
                continue
            _, conv, args = gov
            nargs = 0 if args.strip() in ('', 'void') else len(
                [a for a in args.split(',') if a.strip()])
            off = rva_to_off(rva)
            if off is None:
                undecided.append((name, rva, uline, 'no section'))
                continue
            pops = row_cleanup(raw[off:off + 0x1200], md, rva)
            if pops is None:
                undecided.append((name, rva, uline, 'row not decidable'))
                continue
            if conv in ('__cdecl', '__fastcall'):
                expected = 0
            elif conv == '__stdcall':
                expected = 4 * nargs
            else:  # __thiscall: the receiver travels in ECX
                expected = 4 * max(nargs - 1, 0)
            if pops != expected:
                mismatches.append((name, rva, conv, pops, expected, gov[0], uline))

    print('typedefs=%d  cast sites=%d' % (len(typedefs), sum(len(v) for v in casts.values())))
    print('--- calling-convention mismatches ---')
    for name, rva, conv, pops, expected, tline, uline in mismatches:
        print('  %-20s rva=0x%-7x %-11s row pops %-3d convention implies %-3d '
              '(typedef line %d, use line %d)' % (name, rva, conv, pops, expected, tline, uline))
    print('  total %d' % len(mismatches))
    if undecided:
        print('--- rows whose cleanup could not be read ---')
        for name, rva, uline, why in undecided:
            print('  %-20s rva=0x%-7x use line %d (%s)' % (name, rva, uline, why))
    return 1 if mismatches else 0


if __name__ == '__main__':
    sys.exit(main())
