"""Draft a transcribe_rows config + header for a batch of rows.

usage: python autocfg.py <name> <prefix> <id,id,...|lo-hi> [--extra extra.py]
writes <name>_cfg.py, <name>_head.txt and prints unresolved operands.
extra.py may define SYMBOLS {oracle rva: candidate mangled symbol}, NAMES {row id: function name},
DECLS {row id: declaration}, DATA {abs: name}, DATADECL [lines], PUSHES {abs: name}, HEAD_COMMENT str, INCLUDES [..]
"""
import json, re, sys, struct, importlib.util, capstone, pefile
sys.path.insert(0, 'D:/github/Novodex/docs/reconstruction/novodex-physics/tools')
import transcribe_rows as tr

ROOT = 'D:/github/Novodex/docs/reconstruction/novodex-physics/'
B = 0x10000000
name, prefix, sel = sys.argv[1], sys.argv[2], sys.argv[3]
extra = {}
if '--extra' in sys.argv:
    p = sys.argv[sys.argv.index('--extra') + 1]
    spec = importlib.util.spec_from_file_location('extra', p); m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
    extra = {k: getattr(m, k) for k in dir(m) if k.isupper()}
inv = json.load(open(ROOT + 'inventory.json', encoding='utf-8'))
F = inv['functions']
byid = {r['id']: r for r in F}
starts = sorted(int(r['rva'], 16) for r in F if r.get('kind') == 'code')
byrva = {int(r['rva'], 16): r for r in F if r.get('kind') == 'code'}
if '-' in sel and ',' not in sel:
    lo, hi = (int(x, 16) for x in sel.split('-'))
    ids = [r['id'] for r in F if r.get('kind') == 'code' and lo <= int(r['rva'], 16) < hi and r['state'] == 'discovered' and not r.get('third_party')]
else:
    ids = sel.split(',')
mapsyms = {}
for line in open('D:/github/Novodex/build/Release/NxPhysics.map', encoding='latin-1'):
    mm = re.match(r'\s*000[1-9]:[0-9a-f]+\s+(\S+)\s+([0-9a-f]{8})\s', line)
    if mm: mapsyms.setdefault(mm.group(1), int(mm.group(2), 16))
img = tr.Image.from_pe(tr.ORACLE)
ope = pefile.PE(tr.ORACLE, fast_load=True); ope.parse_data_directories()
imports = {}
for e in ope.DIRECTORY_ENTRY_IMPORT:
    for i in e.imports:
        if i.name: imports[i.address] = (e.dll.decode(), i.name.decode())
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

def owner(a):
    import bisect
    k = bisect.bisect_right(starts, a) - 1
    r = byrva[starts[k]]
    return r if a < starts[k] + int(r['size']) else None

# group rows into functions: contiguous rows where one jumps into the next
rows = sorted((byid[i] for i in ids), key=lambda r: int(r['rva'], 16))
def rng(r): a = int(r['rva'], 16); return a, a + int(r['size'])
def body_end(a, n):
    # rows may be padded; the transcription range ends at the last instruction before padding (int3/nop run)
    ins = list(md.disasm(img.read(a, n), B + a))
    end = a
    for i in ins:
        end = i.address - B + i.size
    return end, ins
groups = []
for r in rows:
    a, e = rng(r)
    if groups:
        g = groups[-1]
        ga, ge = rng(g[0])[0], rng(g[-1])[1]
        # does the group jump into r?
        jumps_in = False
        for x in g:
            xa, xe = rng(x)
            for i in md.disasm(img.read(xa, xe - xa), B + xa):
                if i.mnemonic.startswith('j') and i.op_str.startswith('0x') and a <= int(i.op_str, 16) - B < e:
                    jumps_in = True
        if jumps_in and a - ge < 16:
            g.append(r); continue
    groups.append([r])

ROWS, CALLS, SLOTS, DATA, STRINGS, PUSHES, unresolved, aliases, datadecl = [], {}, {}, dict(extra.get('DATA', {})), {}, dict(extra.get('PUSHES', {})), [], {}, list(extra.get('DATADECL', []))
SYMBOLS = extra.get('SYMBOLS', {})
NAMES = extra.get('NAMES', {})
names = {}
for g in groups:
    rid = g[0]['id']
    names[int(g[0]['rva'], 16)] = NAMES.get(rid, '%s%s' % (prefix, rid[-4:]))
    for x in g[1:]:
        names[int(x['rva'], 16)] = names[int(g[0]['rva'], 16)]


import glob, os
_SRC = None
GUESSED = {}
def guess_symbol(rva):
    """The function a source comment names for this row: the first definition within 12 lines after `phys_fn_NNNNNN`."""
    global _SRC
    r = byrva.get(rva)
    if not r: return None
    if _SRC is None:
        _SRC = {}
        for f in glob.glob('D:/github/Novodex/Physics/src/**/*.cpp', recursive=True) + glob.glob('D:/github/Novodex/Physics/src/**/*.inl', recursive=True):
            _SRC[f] = open(f, encoding='utf-8', errors='replace').read().splitlines()
    rid = r['id']
    cands = set()
    for f, L in _SRC.items():
        for k, l in enumerate(L):
            if rid in l and l.lstrip().startswith('//'):
                for m in L[k + 1:k + 13]:
                    if m.lstrip().startswith('//') or not m.strip(): continue
                    mm = re.match(r'^(?:extern "C" )?(?:static )?(?:__declspec\(\w+\) )*[\w:<>\*& ]*?([A-Za-z_]\w*)::(\w+)\s*\(', m) or re.match(r'^(?:extern "C" )?(?:__declspec\(\w+\) )*[\w:<>\*& ]*?([A-Za-z_]\w*)\s*\(', m)
                    if mm and not m.startswith((' ', '\t')):
                        if mm.lastindex == 2: cands.add(('?' + mm.group(2) + '@' + mm.group(1) + '@'))
                        else: cands.add(mm.group(1))
                    break
    hits = set()
    for c in cands:
        if c.startswith('?'):
            hits |= {k for k in mapsyms if k.startswith(c)}
        else:
            hits |= {k for k in mapsyms if k.startswith('?' + c + '@@') or k in ('_' + c, c) or re.fullmatch('_' + re.escape(c) + r'@\d+', k)}
    if len(hits) == 1:
        s = hits.pop(); GUESSED[rva] = s; return s
    return None

def cand_symbol(rva):
    if rva in SYMBOLS: return SYMBOLS[rva]
    r = byrva.get(rva)
    s = r and r.get('implementation_symbol')
    if s and s in mapsyms: return s
    if s:
        hits = [k for k in mapsyms if k.startswith('?' + s + '@@') or k == '_' + s or k == s]
        if len(hits) == 1: return hits[0]
    return guess_symbol(rva)

def alias_for(sym, kind='Call'):
    if sym not in aliases:
        base = re.sub(r'[^A-Za-z0-9]', '', sym.split('@')[0] if sym.startswith('?') else sym)[:40] or 'X'
        nm = '%s%s%s' % (prefix, kind, base[0].upper() + base[1:])
        while nm in aliases.values(): nm += 'X'
        aliases[sym] = nm
    return aliases[sym]

decls = {}
for g in groups:
    a, e = rng(g[0])[0], rng(g[-1])[1]
    e, ins = body_end(a, e - a)
    # convention
    uses_ecx = False
    for i in ins:
        regs_read = i.op_str
        if re.search(r'\becx\b', i.op_str) or re.search(r'\bcx\b|\bcl\b', i.op_str):
            if i.mnemonic in ('mov', 'lea', 'xor', 'pop') and i.op_str.startswith('ecx,') and not re.search(r'\becx\b', i.op_str.split(',', 1)[1]):
                break
            uses_ecx = True; break
        if i.mnemonic == 'call':
            uses_ecx = True  # ecx reaches the callee untouched: thiscall pass-through
            break
    rets = {i.op_str for i in ins if i.mnemonic == 'ret'}
    nargs = max([int(x, 16) // 4 if x.startswith('0x') else int(x or 0) // 4 for x in rets] or [0])
    nm = names[a]
    params = ', '.join('unsigned a%d' % k for k in range(nargs))
    if uses_ecx:
        decl = '__declspec(naked) unsigned __fastcall %s(void* self, void* edxUnused%s)' % (nm, (', ' + params) if params else '')
    elif nargs:
        decl = '__declspec(naked) unsigned __stdcall %s(%s)' % (nm, params)
    else:
        decl = '__declspec(naked) unsigned __cdecl %s()' % nm
    decl = extra.get('DECLS', {}).get(g[0]['id'], decl)
    tag = ' and '.join(x['id'] for x in g) + (' (one function)' if len(g) > 1 else '')
    ROWS.append((nm, a, e, tag, decl))
    for i in ins:
        op = i.op_str
        if (i.mnemonic == 'call' or i.mnemonic.startswith('j')) and op.startswith('0x'):
            t = int(op, 16)
            if a <= t - B < e: continue
            if t - B in names: CALLS[t] = names[t - B]; continue
            s = cand_symbol(t - B)
            if s: CALLS[t] = alias_for(s); continue
            o = owner(t - B)
            unresolved.append('call %x from %x -> %s %s' % (t - B, i.address - B, o and o['id'], o and o['state']))
            continue
        for mm in re.finditer(r'\[(0x1[0-9a-f]{7})\]', op):
            v = int(mm.group(1), 16)
            if v in imports:
                imp = '__imp_' + imports[v][1] if imports[v][1].startswith('?') else '__imp__' + imports[v][1]
                nmv = alias_for(imp, 'Imp')
                if i.mnemonic in ('call', 'jmp') and op.startswith('dword ptr ['): SLOTS[v] = nmv
                else: DATA[v] = nmv
            elif v not in DATA:
                sec = [s.Name.rstrip(b'\0').decode() for s in ope.sections if s.VirtualAddress <= v - B < s.VirtualAddress + max(s.Misc_VirtualSize, s.SizeOfRawData)]
                if sec and sec[0] == '.rdata' and not op.startswith('dword ptr [0x') or (sec and sec[0] == '.rdata' and i.mnemonic.startswith('f')):
                    continue  # read-only constant: transcribe_rows names it as a konst
                if sec and sec[0] == '.rdata':
                    continue
                unresolved.append('data %s %s at %x (%s)' % (i.mnemonic, op, i.address - B, sec))
        if i.mnemonic == 'push' and re.fullmatch(r'0x1[0-9a-f]{7}', op):
            v = int(op, 16)
            if v in PUSHES: continue
            d = img.read(v - B, 300)
            if b'\0' in d and all(32 <= c < 127 or c in (9, 10, 13) for c in d[:d.index(b'\0')]) and d.index(b'\0') > 2:
                STRINGS[v] = '%sStr%x' % (prefix, v - B)
            elif v - B in names:
                PUSHES[v] = names[v - B]
            else:
                unresolved.append('push %x at %x' % (v - B, i.address - B))
        if i.mnemonic == 'mov' and re.search(r', 0x1[0-9a-f]{7}$', op):
            v = int(op.rsplit(', ', 1)[1], 16)
            if v not in PUSHES:
                unresolved.append('mov imm %x at %x' % (v - B, i.address - B))
        if i.mnemonic == 'jmp' and '*4 +' in op:
            unresolved.append('jump table %s at %x' % (op, i.address - B))

open(name + '_cfg.py', 'w').write('PREFIX = %r\nROWS = %s\nCALLS = %r\nSLOTS = %r\nDATA = %r\nSTRINGS = %r\nPUSHES = %r\nTABLES = %r\n' % (
    prefix + 'K', json.dumps(ROWS, indent=1).replace('[\n', '(').replace('\n ]', ')') if False else repr(ROWS), CALLS, SLOTS, DATA, STRINGS, PUSHES, extra.get('TABLES', {})))
head = ['/*----------------------------------------------------------------------------*\\', '|', '|\t\t\t\t\t\t\t\tNovodeX Technology', '|',
        '|\t\t\t\t\t\t\t     www.novodex.com', '|', '\\*----------------------------------------------------------------------------*/', '']
head += ['// ' + l for l in extra.get('HEAD_COMMENT', 'TODO').split('\n')]
head += ['#include "%s"' % x for x in extra.get('INCLUDES', [])]
head.append('')
for sym, nm in sorted(aliases.items(), key=lambda x: x[1]):
    if sym.startswith('__imp_'):
        head.append('extern "C" void* %s;' % nm)
    else:
        head.append('extern "C" void %s();' % nm)
for sym, nm in sorted(aliases.items(), key=lambda x: x[1]):
    head.append('#pragma comment(linker, "/alternatename:_%s=%s")' % (nm, sym))
head += datadecl
head += ['', '@@PROTOS@@', '', '@@BODY@@', '']
open(name + '_head.txt', 'w').write('\n'.join(head))
print('rows', len(ROWS), 'groups', [x[3] for x in ROWS if 'one function' in x[3]])
for x in ROWS: print('  ', x[0], hex(x[1]), hex(x[2]), x[4])
print('guessed (check):')
for k,v in sorted(GUESSED.items()): print('   %x %s %s' % (k, byrva[k]['id'], v))
print('unresolved:')
for u in unresolved: print('  ', u)
