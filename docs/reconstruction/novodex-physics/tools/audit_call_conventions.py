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

DEFAULT_REPO_ROOT = Path(__file__).resolve().parents[4]
DEFAULT_ORACLE_ROOT = Path(r'D:\FlamingEnt__\Unreal_3\Binaries')

TD = re.compile(
    r'typedef\s+[\w\s\*]+\(\s*(__cdecl|__stdcall|__fastcall|__thiscall)\s*\*\s*(\w+)\s*\)'
    r'\s*\(([^)]*)\)\s*;')
CAST = re.compile(r'reinterpret_cast<(\w+)>\s*\(\s*base\s*\+\s*(0x[0-9a-fA-F]+)\s*\)')


def bounded_switch_targets(ins, code, md, rva, raw, sections, image_base):
    """Resolve a simple absolute x86 switch only when its index is bounded.

    Accepted shape: `cmp index, limit; ja default; jmp [index*4 + VA]`.
    This proves the table has exactly limit+1 reachable entries. Every entry
    must point back into this function row; all other indirect jumps are left
    undecidable.
    """
    start = 0x10000000 + rva
    end = start + len(code)
    instructions = list(md.disasm(code, start))
    position = next((i for i, item in enumerate(instructions)
                     if item.address == ins.address), None)
    if position is None or position < 2:
        return None
    compare, bound = instructions[position - 2:position]
    if (compare.mnemonic != 'cmp' or bound.mnemonic != 'ja' or
            len(compare.operands) != 2 or len(bound.operands) != 1 or
            compare.operands[0].type != capstone.x86.X86_OP_REG or
            compare.operands[1].type != capstone.x86.X86_OP_IMM or
            bound.operands[0].type != capstone.x86.X86_OP_IMM or
            bound.address + bound.size != ins.address):
        return None

    operand = ins.operands[0] if ins.operands else None
    if (operand is None or operand.type != capstone.x86.X86_OP_MEM or
            operand.mem.base != capstone.x86.X86_REG_INVALID or
            operand.mem.index != compare.operands[0].reg or
            operand.mem.scale != 4):
        return None
    count = compare.operands[1].imm + 1
    if count <= 0 or count > 256:
        return None
    table_va = operand.mem.disp
    table_rva = table_va - image_base
    table_offset = None
    for section in sections:
        if (section['rva'] <= table_rva and
                table_rva + count * 4 <= section['rva'] + section['raw_size']):
            table_offset = section['raw_offset'] + table_rva - section['rva']
            break
    if table_offset is None or table_offset + count * 4 > len(raw):
        return None
    targets = []
    for index in range(count):
        target_va = int.from_bytes(raw[table_offset + index * 4:
                                       table_offset + index * 4 + 4], 'little')
        target = 0x10000000 + target_va - image_base
        if not start <= target < end:
            return None
        targets.append(target)
    return targets


def row_cleanup(code, md, rva, function_size=None, jump_table_reader=None):
    """Return a row's common callee cleanup across reachable return paths.

    The byte range must be bounded to the inventory's exact function size.
    Direct branches are followed only while their targets remain inside that
    range. Indirect branches and external tail calls remain undecidable.
    """
    if function_size is not None:
        code = code[:function_size]
    md.detail = True
    start = 0x10000000 + rva
    end = start + len(code)
    pending = [start]
    visited = set()
    cleanups = set()
    while pending:
        address = pending.pop()
        if address in visited:
            continue
        if address < start or address >= end:
            return None
        offset = address - start
        ins = next(md.disasm(code[offset:], address, count=1), None)
        if ins is None or ins.address != address:
            return None
        visited.add(address)
        next_address = address + ins.size

        if ins.mnemonic in ('ret', 'retf'):
            cleanups.add(int(ins.op_str, 0) if ins.op_str else 0)
            continue
        if ins.mnemonic == 'jmp':
            if ins.operands and ins.operands[0].type == capstone.x86.X86_OP_IMM:
                target = ins.operands[0].imm
                if not start <= target < end:
                    return None
                pending.append(target)
                continue
            targets = jump_table_reader(ins) if jump_table_reader else None
            if not targets or any(not start <= target < end for target in targets):
                return None
            pending.extend(targets)
            continue
        if ins.group(capstone.CS_GRP_JUMP):
            if not ins.operands or ins.operands[0].type != capstone.x86.X86_OP_IMM:
                return None
            target = ins.operands[0].imm
            if not start <= target < end or next_address >= end:
                return None
            pending.extend((target, next_address))
            continue
        if ins.mnemonic in ('int3', 'ud2', 'hlt'):
            continue
        if next_address >= end:
            return None
        pending.append(next_address)

    if not cleanups or len(cleanups) != 1:
        return None
    return cleanups.pop()


def expected_cleanup(conv, nargs):
    """Argument bytes popped by the callee for a 32-bit function pointer.

    `nargs` counts the parameters written in the typedef, including an
    explicit `this` placeholder for `__thiscall` and register placeholders
    for `__fastcall`. The first two fastcall arguments use ECX and EDX.
    """
    if conv == '__cdecl':
        return 0
    if conv == '__stdcall':
        return 4 * nargs
    if conv == '__fastcall':
        return 4 * max(nargs - 2, 0)
    if conv == '__thiscall':
        return 4 * max(nargs - 1, 0)
    raise ValueError('unsupported x86 calling convention: %s' % conv)


def resolve_inputs(repo_root=None, oracle_root=None):
    repo = Path(repo_root) if repo_root is not None else DEFAULT_REPO_ROOT
    oracle = Path(oracle_root) if oracle_root is not None else DEFAULT_ORACLE_ROOT
    return (
        repo / 'tests' / 'PhysicsObjectLayoutTests.cpp',
        oracle / 'NxPhysics.dll',
        repo / 'docs' / 'reconstruction' / 'novodex-physics' / 'oracle' / 'pe.json',
    )


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo-root', type=Path, default=DEFAULT_REPO_ROOT)
    parser.add_argument('--oracle-root', type=Path, default=DEFAULT_ORACLE_ROOT)
    args = parser.parse_args(argv)
    harness, dll, pe_path = resolve_inputs(args.repo_root, args.oracle_root)

    pe = json.load(open(pe_path))
    secs = pe['sections']
    raw = open(dll, 'rb').read()
    inventory_path = args.repo_root / 'docs' / 'reconstruction' / 'novodex-physics' / 'inventory.json'
    inventory = json.load(open(inventory_path, encoding='utf-8'))
    function_sizes = {int(row['rva'], 16): row['size']
                      for row in inventory['functions']}

    def rva_to_off(rva):
        for s in secs:
            if s['rva'] <= rva < s['rva'] + max(s['virtual_size'], s['raw_size']):
                return s['raw_offset'] + (rva - s['rva'])
        return None

    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    image_base = pe['image']['image_base']

    # The layout translation unit is committed as UTF-16LE with a BOM (it is the
    # only test source that is), so decode by its own BOM rather than assuming
    # UTF-8.
    rawtext = open(harness, 'rb').read()
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
            function_size = function_sizes.get(rva)
            if function_size is None:
                undecided.append((name, rva, uline, 'no inventory row'))
                continue
            code = raw[off:off + function_size]
            table_reader = lambda ins: bounded_switch_targets(
                ins, code, md, rva, raw, secs, image_base)
            pops = row_cleanup(code, md, rva, function_size=function_size,
                               jump_table_reader=table_reader)
            if pops is None:
                undecided.append((name, rva, uline, 'row not decidable'))
                continue
            expected = expected_cleanup(conv, nargs)
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
