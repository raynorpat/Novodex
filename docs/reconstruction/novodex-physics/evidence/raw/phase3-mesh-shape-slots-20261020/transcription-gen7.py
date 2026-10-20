"""Generate Physics/src/PMapCompute.cpp: naked transcriptions of the PMap compute-arm rows."""
import capstone, struct, re, sys

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


ROWS = [
 ('NxShapeRaycastMesh', 0x29230, 0x29602, 'phys_fn_001405'),
 ('MeshSweep', 0x29610, 0x2974a, 'phys_fn_001407'),
]
SYM = {}
EXTERN = {
    0x100b4d70: 'nxMRCallContainerCtor',
    0x100b4f90: 'nxMRCallSetExternal',
    0x100b4f50: 'nxMRCallEmpty',
    0x100ba6f0: 'nxMRCallRayCollide',
    0x10052240: 'nxMRCallVertexNormals',
    0x1002e220: 'nxMRCallSupportLookup',
    0x1002b6f0: 'nxMRCallHullPolygons',
    0x10036bb0: 'nxMRCallRayPlane',
}
CONST = {}
NULLSUB = 0

KSEEN = {}
SIZES = {'byte': 1, 'word': 2, 'dword': 4, 'qword': 8}
def konst(addr, size):
    n = SIZES[size]
    name = 'nxMRK%x_%d' % (addr, n)
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
    out = []
    for i in ins:
        if i.address in targets:
            out.append('L%x:' % (i.address - 0x10000000))
        m, op = i.mnemonic, i.op_str
        if (m.startswith('j') or m == 'loop' or m == 'call') and op.startswith('0x'):
            t = int(op, 16)
            if lo <= t < hi and m != 'call':
                op = 'L%x' % (t - 0x10000000)
            elif t in SYM:
                op = SYM[t]
            elif t in EXTERN:
                op = EXTERN[t]
            else:
                raise Exception('unmapped target %x from %x' % (t, i.address))
        else:
            def sub(mm):
                return '%s ptr [%s]' % (mm.group(1), konst(int(mm.group(2), 16), mm.group(1)))
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
    for sym, lo, hi, rows in ROWS:
        body, n = gen(lo, hi)
        total += n
        parts.append('// %s (0x%08x, %d B), %d instructions\n'
                     '__declspec(naked) void %s()\n\t{\n\t__asm\n\t\t{\n%s\n\t\t}\n\t}\n' % (rows, lo, hi - lo, n, sym, body))
    consts = ''
    for k, (ad, n, v) in sorted(KSEEN.items()):
        consts += 'static const unsigned %s%s = 0x%x%s;\n' % ('long long ' if n == 8 else '', k, v, 'ull' if n == 8 else '')
    open(path, 'w').write(consts + '\n' + '\n'.join(parts))
    print(total, 'instructions in', len(ROWS), 'rows')


if __name__ == '__main__':
    main(sys.argv[1])
