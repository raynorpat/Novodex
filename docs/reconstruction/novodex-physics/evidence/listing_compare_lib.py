"""Shared machinery of the listing compare scripts for naked transcriptions.

A compare script calls run(...) with:
  rows       [(candidate symbol or substring of it, oracle lo rva, oracle hi rva)]
  expected   {oracle rva of a call/jump/pushed target: substring of the candidate symbol it must reach}
  tables     {oracle absolute address of a jump table: entries}   (the candidate has a compare chain)
  data       [(oracle absolute address, candidate symbol substring, canonical name)]  named variables
  stamp      optional list of (oracle absolute address, canonical name) normalised without a candidate symbol

Both images are disassembled; every instruction is normalised (direct targets to instruction indices, calls to
<call>, pushed strings to their text, absolute operands to the bytes they read or to the import they name, named
variables to their canonical name, jump tables to the list of target indices) and compared pairwise; then each
direct call, jump out of the row and pushed address must reach the candidate symbol `expected` names.

Usage of a script: python <script> build/Release/NxPhysics.dll build/Release/NxPhysics.map
"""
import sys, re, pefile, capstone, struct

ORACLE_PATH = r'D:/FlamingEnt__/Unreal_3/Binaries/NxPhysics.dll'
SIZES = {'byte': 1, 'word': 2, 'dword': 4, 'qword': 8, 'tbyte': 10}


class Ins:
    def __init__(self, address, mnemonic, op_str, size):
        self.address, self.mnemonic, self.op_str, self.size = address, mnemonic, op_str, size


def run(rows, expected, tables=None, data=None, argv=None):
    tables = tables or {}
    data = data or []
    args = [a for a in (argv or sys.argv)[1:] if not a.startswith('--')]
    img, mp = args[0], args[1]
    pe = pefile.PE(img)
    orc = pefile.PE(ORACLE_PATH)
    syms = {}
    for line in open(mp, encoding='latin-1'):
        m = re.match(r'\s*000[1-9]:[0-9a-f]+\s+(\S+)\s+([0-9a-f]{8})\s', line)
        if m:
            syms[m.group(1)] = int(m.group(2), 16)
    rev = {}
    for k, v in syms.items():
        rev.setdefault(v, k)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    imports_cache = {}

    def imports(pe_):
        if id(pe_) not in imports_cache:
            out = {}
            for e in getattr(pe_, 'DIRECTORY_ENTRY_IMPORT', []):
                for i in e.imports:
                    if i.name:
                        out[i.address] = i.name.decode()
            imports_cache[id(pe_)] = out
        return imports_cache[id(pe_)]

    named = {}      # absolute address in each image -> canonical name
    for oracle_va, cand_sub, canon in data:
        named[(id(orc), oracle_va)] = canon
        found = [k for k in syms if cand_sub in k]
        assert len(found) == 1, (cand_sub, found)
        named[(id(pe), syms[found[0]])] = canon

    def absolute(pe_, op):
        for (pid, va), canon in named.items():
            if pid == id(pe_):
                op = op.replace('[0x%x]' % va, '[<%s>]' % canon)
        imp = imports(pe_)

        def value(m):
            va = int(m.group(2), 16)
            n = SIZES.get(m.group(1), 4)
            if va in imp:
                return 'import[%s]' % imp[va]
            try:
                return '%s[=%s]' % (m.group(1) or '', pe_.get_data(va - pe_.OPTIONAL_HEADER.ImageBase, n).hex())
            except Exception:
                return '%s[?%x]' % (m.group(1) or '', va)
        return re.sub(r'(?:(byte|word|dword|qword|tbyte) ptr )?\[0x([0-9a-f]+)\]', value, op)

    def dis(pe_, va, size):
        return list(md.disasm(pe_.get_data(va - pe_.OPTIONAL_HEADER.ImageBase, size), va))

    def lookup(name):
        cand = [name] if name in syms else [k for k in syms if name in k and not k.startswith('__')]
        assert len(cand) == 1, (name, cand)
        return cand[0], syms[cand[0]]

    def section_of(pe_, va):
        rva = va - pe_.OPTIONAL_HEADER.ImageBase
        for sec in pe_.sections:
            if sec.VirtualAddress <= rva < sec.VirtualAddress + max(sec.Misc_VirtualSize, sec.SizeOfRawData):
                return sec
        return None

    def collapse(ins):
        """The candidate's compare chains for jump tables: cmp r,0; je X; cmp r,1; je Y; ... jmp Z."""
        out = []
        k = 0
        while k < len(ins):
            i = ins[k]
            if (i.mnemonic == 'cmp' and re.fullmatch(r'[a-z]{3}, 0', i.op_str) and k + 1 < len(ins)
                    and ins[k + 1].mnemonic == 'je'):
                reg = i.op_str.split(',')[0]
                j, step, targets = k, 0, []
                while (j + 1 < len(ins) and ins[j].mnemonic == 'cmp' and ins[j].op_str == '%s, %d' % (reg, step)
                       and ins[j + 1].mnemonic == 'je'):
                    targets.append(int(ins[j + 1].op_str, 16))
                    j += 2
                    step += 1
                if step >= 3 and j < len(ins) and ins[j].mnemonic == 'jmp' and ins[j].op_str.startswith('0x'):
                    targets.append(int(ins[j].op_str, 16))
                    size = sum(x.size for x in ins[k:j + 1])
                    out.append(Ins(i.address, 'jmp', 'chain:' + ','.join('%x' % t for t in targets), size))
                    k = j + 1
                    continue
            out.append(i)
            k += 1
        return out

    def norm(ins, pe_, is_oracle):
        out = []
        addrs = [i.address for i in ins]
        idx = {a: i for i, a in enumerate(addrs)}
        for i in ins:
            op = i.op_str
            if i.mnemonic == 'jmp' and op.startswith('chain:'):
                op = 'table%s' % (tuple(idx.get(int(t, 16), -1) for t in op[6:].split(',')),)
            elif (i.mnemonic.startswith('j') or i.mnemonic == 'loop') and op.startswith('0x'):
                op = '@%d' % idx.get(int(op, 16), -1)
            elif i.mnemonic == 'call' and op.startswith('0x'):
                op = '<call>'
            elif i.mnemonic == 'push' and op.startswith('0x') and section_of(pe_, int(op, 16)) is not None:
                sec = section_of(pe_, int(op, 16))
                if sec.Characteristics & 0x20000000:
                    op = '<code>'
                else:
                    d = pe_.get_data(int(op, 16) - pe_.OPTIONAL_HEADER.ImageBase, 256)
                    op = '<string %r>' % d[:d.index(bytes(1))]
            elif i.mnemonic == 'mov' and re.search(r', 0x1[0-9a-f]{7}$', op) and section_of(pe_, int(op.rsplit(', ', 1)[1], 16)) is not None:
                op = op.rsplit(', ', 1)[0] + ', <ptr>'
            elif i.mnemonic == 'jmp' and re.search(r'\*4 \+ 0x', op):
                m = re.search(r'\*4 \+ 0x([0-9a-f]+)\]', op)
                base = int(m.group(1), 16)
                n = tables[base]
                tg = [struct.unpack('<I', pe_.get_data(base + 4 * k - pe_.OPTIONAL_HEADER.ImageBase, 4))[0] for k in range(n)]
                op = 'table%s' % (tuple(idx.get(t, -1) for t in tg),)
            op = absolute(pe_, op)
            if i.mnemonic == 'test' and re.fullmatch(r'[a-z]{2,3}, [a-z]{2,3}', op):
                op = ', '.join(sorted(op.split(', ')))
            out.append((i.mnemonic, op))
        return out

    ok = True
    for name, lo, hi in rows:
        o = dis(orc, 0x10000000 + lo, hi - lo)
        assert sum(i.size for i in o) == hi - lo, (name, hex(lo))
        sym, va = lookup(name)
        c = dis(pe, va, hi - lo + 1024)
        # The candidate is longer where a table became a chain; read until the oracle's last mnemonic is matched.
        c = collapse(c)[:len(o)]
        on, cn = norm(o, orc, True), norm(c, pe, False)
        diff = [(k, a, b) for k, (a, b) in enumerate(zip(on, cn)) if a != b]
        if len(c) < len(o):
            diff.append((len(c), '(end)', '(short)'))
        print('%s %s instructions=%d differing=%d' % (name, sym, len(o), len(diff)))
        for k, a, b in diff[:40]:
            print('   #%d %08x oracle=%s candidate=%s' % (k, o[k].address if k < len(o) else 0, a, b))
        ok &= not diff
    print('ALL EQUAL' if ok else 'DIFFERENCES')

    checked = bad = 0
    for name, lo, hi in rows:
        o = dis(orc, 0x10000000 + lo, hi - lo)
        sym, va = lookup(name)
        c = collapse(dis(pe, va, hi - lo + 1024))[:len(o)]
        for a, b in zip(o, c):
            external = a.mnemonic.startswith('j') and a.op_str.startswith('0x') and not (0x10000000 + lo <= int(a.op_str, 16) < 0x10000000 + hi)
            if (a.mnemonic == 'call' or a.mnemonic == 'push' or a.mnemonic == 'mov' or external) and a.op_str.split(', ')[-1].startswith('0x') and b.op_str.split(', ')[-1].startswith('0x'):
                if a.mnemonic in ('push', 'mov'):
                    t = a.op_str.split(', ')[-1]
                    if not re.fullmatch(r'0x1[0-9a-f]{7}', t) or not (0x10001000 <= int(t, 16) < 0x10150000):
                        continue
                    if section_of(orc, int(t, 16)) is None or not (section_of(orc, int(t, 16)).Characteristics & 0x20000000):
                        continue
                tgt_o = int(a.op_str.split(', ')[-1], 16)
                target = int(b.op_str.split(', ')[-1], 16)
                want = expected.get(tgt_o - 0x10000000)
                good = want is not None and any(want in k and v == target for k, v in syms.items())
                checked += 1
                bad += not good
                print('  %s %s -> oracle %s ; candidate %s%s' % (a.mnemonic, name, a.op_str, rev.get(target, b.op_str), '' if good else '  <-- UNEXPECTED'))
    print('MAPPINGS checked=%d unexpected=%d' % (checked, bad))
    sys.exit(0 if ok and not bad else 1)
