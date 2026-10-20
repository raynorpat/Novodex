"""Generic naked-asm transcription generator.

usage: python gen11.py <config.py> <out.txt>

The config module defines:
  ROWS      list of (symbol, lo_rva, hi_rva, tag, declaration)
  CALLS     {absolute call/jmp target: name}   (names are extern "C" aliases or in-file symbols)
  SLOTS     {absolute address: name}           indirect call slots  `call dword ptr [abs]`
  DATA      {absolute address: name}           data operands `[abs]` and `mov r, [abs]` (named variables)
  STRINGS   {absolute address: name}           pushed string constants
  PUSHES    {absolute address: name}           pushed code/data addresses -> `offset name`
  TABLES    {table absolute address: entries}  jump tables to turn into compare chains
  PREFIX    constant name prefix
"""
import capstone, struct, re, sys, json, importlib.util

ORACLE = r'D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll'
f = open(ORACLE, 'rb').read()
pe = struct.unpack_from('<I', f, 0x3c)[0]
nsec = struct.unpack_from('<H', f, pe + 6)[0]
opt = struct.unpack_from('<H', f, pe + 20)[0]
secs = pe + 24 + opt


def off(r):
    for i in range(nsec):
        n, vs, va, rs, ro = struct.unpack_from('<8sIIII', f, secs + 40 * i)
        if va <= r < va + max(vs, rs):
            return ro + r - va


spec = importlib.util.spec_from_file_location('cfg', sys.argv[1])
cfg = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cfg)
ROWS = cfg.ROWS
CALLS = getattr(cfg, 'CALLS', {})
SLOTS = getattr(cfg, 'SLOTS', {})
DATA = getattr(cfg, 'DATA', {})
STRINGS = getattr(cfg, 'STRINGS', {})
PUSHES = getattr(cfg, 'PUSHES', {})
TABLES = getattr(cfg, 'TABLES', {})
PREFIX = getattr(cfg, 'PREFIX', 'nxK')

KSEEN = {}
SIZES = {'byte': 1, 'word': 2, 'dword': 4, 'qword': 8}


def konst(addr, size):
    n = SIZES[size]
    name = '%s%x_%d' % (PREFIX, addr, n)
    if name not in KSEEN:
        KSEEN[name] = (addr, n, int.from_bytes(f[off(addr - 0x10000000):off(addr - 0x10000000) + n], 'little'))
    return name


def gen(entry, end):
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    o = off(entry)
    ins = list(md.disasm(f[o:o + (end - entry)], 0x10000000 + entry))
    got = ins[-1].address + ins[-1].size - 0x10000000
    assert got == end, (hex(entry), hex(got), hex(end))
    lo, hi = 0x10000000 + entry, 0x10000000 + end
    addrs = {i.address for i in ins}
    targets = set()
    for i in ins:
        if (i.mnemonic.startswith('j') or i.mnemonic == 'loop') and i.op_str.startswith('0x'):
            t = int(i.op_str, 16)
            if lo <= t < hi:
                assert t in addrs, hex(t)
                targets.add(t)
        m = re.fullmatch(r'dword ptr \[(\w+)\*4 \+ (0x[0-9a-f]+)\]', i.op_str) if i.mnemonic == 'jmp' else None
        if m:
            base = int(m.group(2), 16)
            for k in range(TABLES[base]):
                t = struct.unpack_from('<I', f, off(base - 0x10000000) + 4 * k)[0]
                assert t in addrs, hex(t)
                targets.add(t)
    out = []
    for i in ins:
        if i.address in targets:
            out.append('L%x:' % (i.address - 0x10000000))
        m, op = i.mnemonic, i.op_str
        mt = re.fullmatch(r'dword ptr \[(\w+)\*4 \+ (0x[0-9a-f]+)\]', op) if m == 'jmp' else None
        if mt:
            base = int(mt.group(2), 16)
            n = TABLES[base]
            reg = mt.group(1)
            lines = []
            for k in range(n):
                t = struct.unpack_from('<I', f, off(base - 0x10000000) + 4 * k)[0]
                if k < n - 1:
                    lines.append('\t\tcmp %s, %d\t\t// jump table 0x%08x[%d]' % (reg, k, base - 0x10000000, k))
                    lines.append('\t\tje L%x' % (t - 0x10000000))
                else:
                    lines.append('\t\tjmp L%x\t\t// jump table 0x%08x[%d]' % (t - 0x10000000, base - 0x10000000, k))
            out.append('\n'.join(lines) + '\t\t// 0x%08x' % (i.address - 0x10000000))
            continue
        if (m.startswith('j') or m == 'loop' or m == 'call') and op.startswith('0x'):
            t = int(op, 16)
            if lo <= t < hi and m != 'call':
                op = 'L%x' % (t - 0x10000000)
            elif t in CALLS:
                op = CALLS[t]
            else:
                raise Exception('unmapped target %x from %x' % (t, i.address))
        elif m in ('call', 'jmp') and op.startswith('dword ptr [0x') and int(op[len('dword ptr [0x'):-1], 16) in SLOTS:
            op = 'dword ptr [%s]' % SLOTS[int(op[len('dword ptr [0x'):-1], 16)]
        elif m == 'push' and op.startswith('0x') and int(op, 16) in STRINGS:
            op = 'offset ' + STRINGS[int(op, 16)]
        elif m == 'push' and op.startswith('0x') and int(op, 16) in PUSHES:
            op = 'offset ' + PUSHES[int(op, 16)]
        elif m == 'mov' and re.search(r', 0x1[0-9a-f]{7}$', op) and int(op.rsplit(', ', 1)[1], 16) in PUSHES:
            op = op.rsplit(', ', 1)[0] + ', offset ' + PUSHES[int(op.rsplit(', ', 1)[1], 16)]
        else:
            def sub(mm):
                a = int(mm.group(2), 16)
                if a in DATA:
                    return '%s ptr [%s]' % (mm.group(1), DATA[a])
                return '%s ptr [%s]' % (mm.group(1), konst(a, mm.group(1)))
            op = re.sub(r'(byte|word|dword|qword) ptr \[(0x1[0-9a-f]{7})\]', sub, op)
            if m in ('faddp', 'fsubp', 'fmulp', 'fdivp', 'fsubrp', 'fdivrp') and ',' not in op:
                op += ', st(0)'
            if m in ('fadd', 'fsub', 'fmul', 'fdiv', 'fsubr', 'fdivr') and re.fullmatch(r'st\(\d\)', op):
                op = 'st(0), ' + op
            if m.startswith('rep'):
                op = ''
        text = (m + (' ' + op if op else '')).rstrip()
        out.append('\t\t%s\t\t// 0x%08x' % (text, i.address - 0x10000000))
    return '\n'.join(out), len(ins)


def main(path):
    parts = []
    total = 0
    for sym, lo, hi, tag, decl in ROWS:
        body, n = gen(lo, hi)
        total += n
        parts.append('// %s (0x%08x, %d B), %d instructions\n%s\n\t{\n\t__asm\n\t\t{\n%s\n\t\t}\n\t}\n' % (tag, lo, hi - lo, n, decl, body))
    consts = ''
    for k, (ad, n, v) in sorted(KSEEN.items()):
        consts += 'static const unsigned %s%s = 0x%x%s;\n' % ('long long ' if n == 8 else '', k, v, 'ull' if n == 8 else '')
    for va_, nm_ in STRINGS.items():
        d_ = f[off(va_ - 0x10000000):off(va_ - 0x10000000) + 400]
        txt_ = d_[:d_.index(b'\0')].decode('latin1')
        consts += 'static const char %s[] = %s;\n' % (nm_, json.dumps(txt_))
    open(path, 'w', newline='\n').write(consts + '\n' + '\n'.join(parts))
    print(total, 'instructions in', len(ROWS), 'rows')


main(sys.argv[2])
