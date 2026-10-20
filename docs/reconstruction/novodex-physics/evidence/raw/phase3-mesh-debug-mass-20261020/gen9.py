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
 ('nxMeshRow2156', 0x538e0, 0x5390e, 'phys_fn_002156', '__declspec(naked) static void __fastcall nxMeshRow2156(void* self, void* edxUnused, float* out)'),
 ('nxMassFrameRow835', 0x1c5a0, 0x1c5be, 'phys_fn_000835', '__declspec(naked) static void __fastcall nxMassFrameRow835(void* destination, void* edxUnused, const void* extra)'),
 ('nxMeshMassRow2241', 0x54bb0, 0x54e64, 'phys_fn_002241', '__declspec(naked) static void* __fastcall nxMeshMassRow2241(void* mesh)'),
 ('nxMeshAccumulateMassCached', 0x28e10, 0x28e78, 'phys_fn_001397', '__declspec(naked) bool MeshShape::nxMeshAccumulateMassCached(MassFrame* destination, float density, unsigned reserved) const'),
]
SYM = {0x100538e0: 'nxMeshRow2156', 0x1001c5a0: 'nxMassFrameRow835', 0x10054bb0: 'nxMeshMassRow2241'}
EXTERN = {
    0x1001bcc0: 'nxMSCallPoseCopy',
    0x1001bdc0: 'nxMSCallFold',
    0x1001c040: 'nxMSCallTranslate',
    0x1001c630: 'nxMSCallMerge',
    0x1002ea70: 'nxMSCallNoop',
    0x100f4140: 'nxMSCallFpclass',
}
SLOTS = {0x10104164: 'nxMSVolumeSlot', 0x101041b4: 'nxMSErrorSlot'}
STRINGS = {0x10108528: 'nxMSMessage', 0x10108364: 'nxMSFile'}
NULLSUB = 0

KSEEN = {}
SIZES = {'byte': 1, 'word': 2, 'dword': 4, 'qword': 8}
def konst(addr, size):
    n = SIZES[size]
    name = 'nxMSK%x_%d' % (addr, n)
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
