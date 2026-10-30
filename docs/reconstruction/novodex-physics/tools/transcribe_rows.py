#!/usr/bin/env python3
"""Generate naked-asm listing transcriptions of oracle rows.

usage: python transcribe_rows.py <cfg.py> <dest.cpp> (--header <head.txt> | --append) [--oracle <dll>]

The config module defines:
  ROWS      list of (symbol, lo_rva, hi_rva, tag, declaration)
  CALLS     {absolute call/jmp target: name}   (names are extern "C" aliases or in-file symbols)
  SLOTS     {absolute address: name}           indirect call slots  `call dword ptr [abs]`
  DATA      {absolute address: name}           data operands `[abs]` (named variables)
  STRINGS   {absolute address: name}           pushed string constants
  PUSHES    {absolute address: name}           pushed code/data addresses -> `offset name`
  TABLES    {table absolute address: entries}  jump tables, emitted as compare chains
  PREFIX    constant name prefix

--header writes a new file from a header holding @@PROTOS@@ and @@BODY@@; the prototypes and bodies are
followed by `// @@PROTOS-END@@` and `// @@ROWS-END@@` markers, before which --append inserts later rows.
Output is CRLF. (Promoted from the session generator gen11.py and mkfile.py; the instruction rendering
is unchanged.)
"""
import argparse
import importlib.util
import json
import re
import struct

import capstone

ORACLE = r'D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll'
IMAGE_BASE = 0x10000000
SIZES = {'byte': 1, 'word': 2, 'dword': 4, 'qword': 8}
PROTOS_END = '// @@PROTOS-END@@'
ROWS_END = '// @@ROWS-END@@'


class Image:
    """Bytes of an image addressed by rva."""

    def __init__(self, data, base_rva, spans=None):
        self.spans = spans or [(base_rva, base_rva + len(data), data, 0)]

    @classmethod
    def from_pe(cls, path):
        f = open(path, 'rb').read()
        pe = struct.unpack_from('<I', f, 0x3c)[0]
        nsec = struct.unpack_from('<H', f, pe + 6)[0]
        opt = struct.unpack_from('<H', f, pe + 20)[0]
        spans = []
        for i in range(nsec):
            _, vs, va, rs, ro = struct.unpack_from('<8sIIII', f, pe + 24 + opt + 40 * i)
            spans.append((va, va + max(vs, rs), f, ro))
        return cls(None, None, spans)

    def read(self, rva, n):
        for lo, hi, data, ro in self.spans:
            if lo <= rva < hi:
                o = ro + rva - lo
                return data[o:o + n]
        raise ValueError('rva %x outside the image' % rva)

    def dword(self, va):
        return struct.unpack('<I', self.read(va - IMAGE_BASE, 4))[0]


def transcribe(cfg, img):
    """Return (prototypes, constants + bodies) for cfg.ROWS read from img."""
    calls, slots = getattr(cfg, 'CALLS', {}), getattr(cfg, 'SLOTS', {})
    data, strings = getattr(cfg, 'DATA', {}), getattr(cfg, 'STRINGS', {})
    pushes, tables = getattr(cfg, 'PUSHES', {}), getattr(cfg, 'TABLES', {})
    prefix = getattr(cfg, 'PREFIX', 'nxK')
    kseen = {}

    def konst(addr, size):
        n = SIZES[size]
        name = '%s%x_%d' % (prefix, addr, n)
        if name not in kseen:
            kseen[name] = (addr, n, int.from_bytes(img.read(addr - IMAGE_BASE, n), 'little'))
        return name

    def gen(entry, end):
        md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
        ins = list(md.disasm(img.read(entry, end - entry), IMAGE_BASE + entry))
        got = ins[-1].address + ins[-1].size - IMAGE_BASE if ins else entry
        if got != end:
            raise ValueError('row %x disassembles to %x, not %x' % (entry, got, end))
        lo, hi = IMAGE_BASE + entry, IMAGE_BASE + end
        addrs = {i.address for i in ins}
        targets = set()
        for i in ins:
            if (i.mnemonic.startswith('j') or i.mnemonic == 'loop') and i.op_str.startswith('0x'):
                t = int(i.op_str, 16)
                if lo <= t < hi:
                    if t not in addrs:
                        raise ValueError('jump into an instruction at %x' % t)
                    targets.add(t)
            m = re.fullmatch(r'dword ptr \[(\w+)\*4 \+ (0x[0-9a-f]+)\]', i.op_str) if i.mnemonic == 'jmp' else None
            if m:
                base = int(m.group(2), 16)
                for k in range(tables[base]):
                    t = img.dword(base + 4 * k)
                    if t not in addrs:
                        raise ValueError('table %x entry %d is not an instruction' % (base, k))
                    targets.add(t)
        out = []
        for i in ins:
            if i.address in targets:
                out.append('L%x:' % (i.address - IMAGE_BASE))
            m, op = i.mnemonic, i.op_str
            mt = re.fullmatch(r'dword ptr \[(\w+)\*4 \+ (0x[0-9a-f]+)\]', op) if m == 'jmp' else None
            if mt:
                base, reg = int(mt.group(2), 16), mt.group(1)
                n = tables[base]
                lines = []
                for k in range(n):
                    t = img.dword(base + 4 * k)
                    if k < n - 1:
                        lines.append('\t\tcmp %s, %d\t\t// jump table 0x%08x[%d]' % (reg, k, base - IMAGE_BASE, k))
                        lines.append('\t\tje L%x' % (t - IMAGE_BASE))
                    else:
                        lines.append('\t\tjmp L%x\t\t// jump table 0x%08x[%d]' % (t - IMAGE_BASE, base - IMAGE_BASE, k))
                out.append('\n'.join(lines) + '\t\t// 0x%08x' % (i.address - IMAGE_BASE))
                continue
            if (m.startswith('j') or m == 'loop' or m == 'call') and op.startswith('0x'):
                t = int(op, 16)
                if lo <= t < hi and m != 'call':
                    op = 'L%x' % (t - IMAGE_BASE)
                elif t in calls:
                    op = calls[t]
                else:
                    raise ValueError('unmapped target %x from %x' % (t, i.address))
            elif m in ('call', 'jmp') and op.startswith('dword ptr [0x') and int(op[len('dword ptr [0x'):-1], 16) in slots:
                op = 'dword ptr [%s]' % slots[int(op[len('dword ptr [0x'):-1], 16)]
            elif m == 'push' and op.startswith('0x') and int(op, 16) in strings:
                op = 'offset ' + strings[int(op, 16)]
            elif m == 'push' and op.startswith('0x') and int(op, 16) in pushes:
                op = 'offset ' + pushes[int(op, 16)]
            elif m == 'mov' and re.search(r', 0x1[0-9a-f]{7}$', op) and int(op.rsplit(', ', 1)[1], 16) in pushes:
                op = op.rsplit(', ', 1)[0] + ', offset ' + pushes[int(op.rsplit(', ', 1)[1], 16)]
            else:
                def sub(mm):
                    a = int(mm.group(2), 16)
                    if a in data:
                        return '%s ptr [%s]' % (mm.group(1), data[a])
                    return '%s ptr [%s]' % (mm.group(1), konst(a, mm.group(1)))
                op = re.sub(r'(byte|word|dword|qword) ptr \[(0x1[0-9a-f]{7})\]', sub, op)
                if m in ('faddp', 'fsubp', 'fmulp', 'fdivp', 'fsubrp', 'fdivrp') and ',' not in op:
                    op += ', st(0)'
                if m in ('fadd', 'fsub', 'fmul', 'fdiv', 'fsubr', 'fdivr') and re.fullmatch(r'st\(\d\)', op):
                    op = 'st(0), ' + op
                if m.startswith('rep'):
                    op = ''
            text = (m + (' ' + op if op else '')).rstrip()
            out.append('\t\t%s\t\t// 0x%08x' % (text, i.address - IMAGE_BASE))
        return '\n'.join(out), len(ins)

    parts, protos = [], ''
    for sym, lo, hi, tag, decl in cfg.ROWS:
        body, n = gen(lo, hi)
        parts.append('// %s (0x%08x, %d B), %d instructions\n%s\n\t{\n\t__asm\n\t\t{\n%s\n\t\t}\n\t}\n' % (tag, lo, hi - lo, n, decl, body))
        protos += decl.replace('__declspec(naked) ', '') + ';\n'
    consts = ''
    for k, (_, n, v) in sorted(kseen.items()):
        consts += 'static const unsigned %s%s = 0x%x%s;\n' % ('long long ' if n == 8 else '', k, v, 'ull' if n == 8 else '')
    for va, nm in strings.items():
        d = img.read(va - IMAGE_BASE, 400)
        consts += 'static const char %s[] = %s;\n' % (nm, json.dumps(d[:d.index(b'\0')].decode('latin1')))
    return protos, consts + '\n' + '\n'.join(parts)


def write(dest, protos, body, header=None, append=False):
    """Write a new file from a header, or insert before the markers of an existing one; CRLF."""
    if header is not None:
        text = open(header, encoding='utf-8').read().replace('\r\n', '\n')
        text = text.replace('@@PROTOS@@', protos + PROTOS_END).replace('@@BODY@@', body + ROWS_END)
    elif append:
        text = open(dest, encoding='utf-8', newline='').read().replace('\r\n', '\n')
        if PROTOS_END not in text or ROWS_END not in text:
            raise ValueError('%s has no %s / %s markers' % (dest, PROTOS_END, ROWS_END))
        text = text.replace(PROTOS_END, protos + PROTOS_END, 1).replace(ROWS_END, body + ROWS_END, 1)
    else:
        raise ValueError('need a header or --append')
    open(dest, 'w', encoding='utf-8', newline='').write(text.replace('\n', '\r\n'))


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('cfg')
    ap.add_argument('dest')
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument('--header')
    g.add_argument('--append', action='store_true')
    ap.add_argument('--oracle', default=ORACLE)
    a = ap.parse_args(argv)
    spec = importlib.util.spec_from_file_location('cfg', a.cfg)
    cfg = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(cfg)
    protos, body = transcribe(cfg, Image.from_pe(a.oracle))
    write(a.dest, protos, body, header=a.header, append=a.append)
    print('%d rows written to %s' % (len(cfg.ROWS), a.dest))


if __name__ == '__main__':
    main()
