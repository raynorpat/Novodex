"""Generate Physics/src/PMapCompute.cpp: naked transcriptions of the PMap compute-arm rows."""
import capstone, struct, re, sys, json

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
 ('nxMTReleaseRow', 0x51ef0, 0x51f99, 'phys_fn_002067', '__declspec(naked) static void __fastcall nxMTReleaseRow(void* mesh)'),
 ('nxMTDestructRow', 0x523a0, 0x523b1, 'phys_fn_002085', '__declspec(naked) static void __fastcall nxMTDestructRow(void* mesh)'),
 ('nxMTBuildTopologyRow', 0x523c0, 0x52a5c, 'phys_fn_002087+002089+002091+002093', '__declspec(naked) static bool __fastcall nxMTBuildTopologyRow(void* mesh)'),
]
SYM = {0x10051ef0: 'nxMTReleaseRow'}
EXTERN = {
    0x1002ea70: 'nxMTCallNoop',
    0x1002ebe0: 'nxMTCallBuilderCtor',
    0x10030420: 'nxMTCallBuilderInit',
    0x1002eda0: 'nxMTCallBuilderAddFace',
    0x10030f50: 'nxMTCallBuilderBuild',
    0x10030d10: 'nxMTCallBuilderDtor',
    0x10051060: 'nxMTCallEdgeListCtor',
    0x10051dd0: 'nxMTCallEdgeListInit',
    0x100515d0: 'nxMTCallEdgeListDtor',
    0x100b4d70: 'nxMTCallContainerCtor',
    0x100b4f50: 'nxMTCallContainerDtor',
    0x1002eb50: 'nxMTCallAddPoint',
    0x100e4160: 'nxMTCallReplaceVertex',
}
SLOTS = {}
STRINGS = {}
PARAMS = {0x101041bc: 'nxMTAllocSlot'}
NULLSUB = 0

KSEEN = {}
SIZES = {'byte': 1, 'word': 2, 'dword': 4, 'qword': 8}
def konst(addr, size):
    n = SIZES[size]
    name = 'nxMTK%x_%d' % (addr, n)
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
        elif m == 'call' and op.startswith('dword ptr [0x') and int(op[len('dword ptr [0x'):-1], 16) in SLOTS:
            op = 'dword ptr [%s]' % SLOTS[int(op[len('dword ptr [0x'):-1], 16)]
        elif m == 'push' and op.startswith('0x') and int(op, 16) in STRINGS:
            op = 'offset ' + STRINGS[int(op, 16)]
        else:
            def sub(mm):
                if int(mm.group(2), 16) in PARAMS:
                    return '%s ptr [%s]' % (mm.group(1), PARAMS[int(mm.group(2), 16)])
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
    for sym, lo, hi, rows, decl in ROWS:
        body, n = gen(lo, hi)
        total += n
        parts.append('// %s (0x%08x, %d B), %d instructions\n%s\n\t{\n\t__asm\n\t\t{\n%s\n\t\t}\n\t}\n' % (rows, lo, hi - lo, n, decl, body))
    consts = ''
    for k, (ad, n, v) in sorted(KSEEN.items()):
        consts += 'static const unsigned %s%s = 0x%x%s;\n' % ('long long ' if n == 8 else '', k, v, 'ull' if n == 8 else '')
    for va_, nm_ in STRINGS.items():
        d_ = f[off(va_ - 0x10000000):off(va_ - 0x10000000) + 400]
        txt_ = d_[:d_.index(b'\0')].decode('latin1')
        consts += 'static const char %s[] = %s;\n' % (nm_, json.dumps(txt_))
    open(path, 'w').write(consts + '\n' + '\n'.join(parts))
    print(total, 'instructions in', len(ROWS), 'rows')


if __name__ == '__main__':
    main(sys.argv[1])
