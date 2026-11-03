#!/usr/bin/env python3
"""Source-level mutation of one C++ function, to falsify the inventory row it implements.

The row's function is found in its source file (by the inventory symbol, or by the `// phys_fn_NNNNNN` marker the
reconstruction left above it), one operator inside it is changed, the candidate DLL is rebuilt, and the change counts
only when every changed byte of the DLL lies inside the row's compiled range and a gate family reports a difference.
See docs/superpowers/specs/2026-11-03-falsify-remaining-rows-design.md.
"""
import re

IDENT = re.compile(r'[A-Za-z_0-9]')


def code_mask(text):
    """A list of bools, True where the character is code: not in a comment, a string or character literal, or a
    preprocessor line."""
    n = len(text)
    mask = [True] * n
    i = 0
    line_start = True
    while i < n:
        c = text[i]
        if line_start and c == '#':
            j = i
            while j < n and text[j] != '\n':
                if text[j] == '\\' and j + 1 < n and text[j + 1] == '\n':
                    j += 1
                j += 1
            for k in range(i, min(j, n)):
                mask[k] = False
            i = j
            continue
        if c == '\n':
            line_start = True
            i += 1
            continue
        if c not in ' \t\r':
            line_start = False
        if c == '/' and i + 1 < n and text[i + 1] == '/':
            j = i
            while j < n and text[j] != '\n':
                j += 1
            for k in range(i, j):
                mask[k] = False
            i = j
            continue
        if c == '/' and i + 1 < n and text[i + 1] == '*':
            j = text.find('*/', i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                mask[k] = False
            i = j
            continue
        if c in '"\'':
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == '\\' else 1
            j = min(j + 1, n)
            for k in range(i, j):
                mask[k] = False
            i = j
            continue
        i += 1
    return mask


def _blank(text, mask):
    return ''.join(ch if m or ch == '\n' else ' ' for ch, m in zip(text, mask))


def _match(code, i, open_c, close_c):
    depth = 0
    for j in range(i, len(code)):
        if code[j] == open_c:
            depth += 1
        elif code[j] == close_c:
            depth -= 1
            if depth == 0:
                return j
    return -1


_TYPE_WORDS = {'int', 'char', 'short', 'long', 'float', 'double', 'unsigned', 'signed', 'void', 'bool', 'const', 'struct', 'class'}


def _norm_params(s):
    """The parameter types of a parameter list, without parameter names or default values."""
    parts, depth, cur = [], 0, ''
    for ch in s:
        if ch in '(<[':
            depth += 1
        elif ch in ')>]':
            depth -= 1
        if ch == ',' and depth == 0:
            parts.append(cur)
            cur = ''
        else:
            cur += ch
    parts.append(cur)
    out = []
    for p in parts:
        p = p.split('=')[0]
        toks = re.findall(r'[\w:]+|[&*]', p)
        if len(toks) > 1 and toks[-1] not in _TYPE_WORDS and re.match(r'[A-Za-z_]', toks[-1]) and toks[-2] not in ('struct', 'class'):
            toks = toks[:-1]
        out.append(' '.join(toks))
    joined = ','.join(out)
    return '' if joined in ('void',) else joined


def _split_symbol(symbol):
    """(qualified name, normalised parameter text or None, trailing const)."""
    symbol = symbol.strip()
    k = symbol.find('(')
    if k < 0:
        return symbol, None, False
    close = _match(symbol, k, '(', ')')
    params = symbol[k + 1:close] if close > 0 else symbol[k + 1:]
    tail = symbol[close + 1:] if close > 0 else ''
    return symbol[:k].strip(), _norm_params(params), bool(re.search(r'\bconst\b', tail))


def find_function_span(text, symbol):
    """Offsets (open brace, one past the closing brace) of the one definition of `symbol`, or None when it is absent
    or several definitions fit."""
    name, params, _const = _split_symbol(symbol)
    code = _blank(text, code_mask(text))
    found = []
    for m in re.finditer(r'(?<![A-Za-z_0-9~])%s\s*\(' % re.escape(name).replace(r'\:\:', '::'), code):
        po = code.index('(', m.start())
        pc = _match(code, po, '(', ')')
        if pc < 0:
            continue
        j = pc + 1
        # a definition: qualifiers, an optional member initialiser list, then the body; a call or a declaration meets ';' or ')' first
        depth = 0
        while j < len(code):
            c = code[j]
            if c in '([':
                depth += 1
            elif c in ')]':
                if depth == 0:
                    j = -1
                    break
                depth -= 1
            elif c == ';' and depth == 0:
                j = -1
                break
            elif c == '{' and depth == 0:
                break
            elif c == '=' and depth == 0 and not code[j:j + 2] == '==':
                j = -1
                break
            j += 1
        if j < 0 or j >= len(code):
            continue
        # what precedes the name must look like a declaration, not an expression
        before = code[:m.start()].rstrip()
        if before.endswith(('=', ',', '(', 'return', '!', '&&', '||', '?', ':')) and not before.endswith('::'):
            if not before.endswith(':') or before.endswith('::'):
                continue
        end = _match(code, j, '{', '}')
        if end < 0:
            continue
        if params is not None and _norm_params(code[po + 1:pc]) != params:
            continue
        found.append((j, end + 1))
    return found[0] if len(found) == 1 else None


def find_span_by_marker(text, row_id):
    """The body of the function that follows the `// <row_id>` comment line."""
    m = re.search(r'^[ \t]*//[ \t]*%s\b[^\n]*$' % re.escape(row_id), text, re.M)
    if not m:
        return None
    code = _blank(text, code_mask(text))
    j = m.end()
    depth = 0
    while j < len(code):
        c = code[j]
        if c in '([':
            depth += 1
        elif c in ')]':
            depth -= 1
        elif c == ';' and depth == 0:
            return None
        elif c == '{' and depth == 0:
            end = _match(code, j, '{', '}')
            return (j, end + 1) if end >= 0 else None
        j += 1
    return None


CMP = {'<': '<=', '>': '>=', '==': '!=', '!=': '==', '<=': '<', '>=': '>'}


def cpp_mutations(text, lo, hi):
    """Single-site mutations inside text[lo:hi): (offset, old, new, kind)."""
    code = _blank(text, code_mask(text))
    seg = code[lo:hi]
    out = []
    for m in re.finditer(r'(?<=\s)(<=|>=|<|>)(?=\s)|(==|!=)', seg):
        tok = m.group(0)
        out.append((lo + m.start(), tok, CMP[tok], 'cmp'))
    for m in re.finditer(r'(?<=[\w\)\]])\s(\+|-)\s(?=[\w\(])', seg):
        k = m.start(1)
        out.append((lo + k, seg[k], '-' if seg[k] == '+' else '+', 'arith'))
    for m in re.finditer(r'(?<![\w.])(\d+)(?![\w.])', seg):
        v = m.group(1)
        # 0 and 1 compile to xor/inc forms of another length, which shifts the code after them; octal-looking literals are skipped too
        if v in ('0', '1') or (len(v) > 1 and v.startswith('0')):
            continue
        out.append((lo + m.start(1), v, str(int(v) + 1), 'const'))
    for m in re.finditer(r'\bif\s*\(', seg):
        po = seg.index('(', m.start())
        pc = _match(seg, po, '(', ')')
        if pc > po + 1:
            cond = text[lo + po + 1:lo + pc]
            out.append((lo + po + 1, cond, '!(' + cond + ')', 'neg'))
    out.sort(key=lambda m: (m[0], m[3]))
    return out


def apply_mutation(text, m):
    off, old, new = m[0], m[1], m[2]
    if text[off:off + len(old)] != old:
        raise ValueError('the mutation no longer fits at offset %d' % off)
    return text[:off] + new + text[off + len(old):]


def spread(muts, limit):
    n = len(muts)
    if n <= limit:
        return list(muts)
    if limit <= 1:
        return list(muts[:limit])
    return [muts[round(i * (n - 1) / (limit - 1))] for i in range(limit)]


# ---- the compiled range -------------------------------------------------------------------------------------------

import struct


def function_name_at(text, span):
    """The qualified name in the definition whose body starts at span[0] (`A::f`, `helper`, `~A`)."""
    code = _blank(text, code_mask(text))
    head = code[:span[0]]
    cut = max(head.rfind(';'), head.rfind('}'), head.rfind('{'))
    seg = re.sub(r'__declspec\s*\([^)]*\)', ' ', head[cut + 1:])
    m = re.search(r'([~\w:]+)\s*\(', seg)
    return m.group(1) if m else None


def _pe_layout(image):
    pe = struct.unpack_from('<I', image, 0x3c)[0]
    nsec, = struct.unpack_from('<H', image, pe + 6)
    opt_size, = struct.unpack_from('<H', image, pe + 20)
    opt = pe + 24
    sections = []
    for i in range(nsec):
        o = opt + opt_size + 40 * i
        vsize, va, rawsize, rawptr = struct.unpack_from('<IIII', image, o + 8)
        sections.append((va, max(vsize, 0), rawptr, rawsize))
    dirs = {}
    for k in (0, 6):
        dirs[k] = struct.unpack_from('<II', image, opt + 96 + 8 * k)
    return sections, dirs


def _rva_to_off(sections, rva):
    for va, vsize, rawptr, rawsize in sections:
        if va <= rva < va + max(vsize, rawsize):
            return rawptr + rva - va
    return None


def _ignored(image, sections, dirs):
    """File-offset ranges that change on every link without the code changing: the debug directory, the data it
    points at, and the export directory's time stamp."""
    out = []
    rva, size = dirs[6]
    off = _rva_to_off(sections, rva) if rva else None
    if off is not None:
        out.append((off, off + size))
        for k in range(size // 28):
            f = struct.unpack_from('<IIHHIIII', image, off + 28 * k)
            if f[5] and f[7]:
                out.append((f[7], f[7] + f[5]))
    rva, size = dirs[0]
    off = _rva_to_off(sections, rva) if rva else None
    if off is not None:
        out.append((off + 4, off + 8))
    return out


def pe_changed_rvas(before, after):
    """Merged [lo, hi) RVA ranges where the sections of two PE images differ; header fields and the link-time stamps
    are not differences. A different section layout is reported as one range covering everything."""
    sb, db = _pe_layout(before)
    sa, da = _pe_layout(after)
    if sb != sa:
        return [(0, 0xffffffff)]
    skip = _ignored(before, sb, db) + _ignored(after, sa, da)
    changed = []
    for va, vsize, rawptr, rawsize in sb:
        n = min(rawsize, len(before) - rawptr, len(after) - rawptr)
        a, b = before[rawptr:rawptr + n], after[rawptr:rawptr + n]
        if a == b:
            continue
        for i in range(n):
            if a[i] != b[i] and not any(lo <= rawptr + i < hi for lo, hi in skip):
                rva = va + i
                if changed and changed[-1][1] == rva:
                    changed[-1] = (changed[-1][0], rva + 1)
                else:
                    changed.append((rva, rva + 1))
    return changed


def inside(changed, rva, size):
    return bool(changed) and all(rva <= lo and hi <= rva + size for lo, hi in changed)


def read_text_symbols(map_path):
    """[(rva, decorated name)] sorted, for the code symbols of an MSVC linker map, and the end of the code."""
    base = 0x10000000
    syms = []
    for line in open(map_path, encoding='latin-1'):
        m = re.search(r'Preferred load address is ([0-9a-f]+)', line)
        if m:
            base = int(m.group(1), 16)
        m = re.match(r'\s*0001:([0-9a-f]+)\s+(\S+)\s+([0-9a-f]{8})\s+(?:f\s+)?(?:i\s+)?\S+', line)
        if m:
            syms.append((int(m.group(3), 16) - base, m.group(2)))
    syms.sort()
    return syms


def symbol_at(syms, rva, end):
    """(name, lo, hi) of the symbol whose code contains rva, or None."""
    best = None
    for i, (a, name) in enumerate(syms):
        if a <= rva:
            hi = syms[i + 1][0] if i + 1 < len(syms) else end
            best = (name, a, hi)
        else:
            break
    return best if best and rva < best[2] else None


def symbol_matches(decorated, qualified):
    """True when a linker-map symbol can be the function `qualified` (`A::f`, `helper`, `qh_x`)."""
    parts = qualified.split('::')
    name = parts[-1]
    scopes = parts[:-1]
    if re.match(r'^[_@]?%s(@\d+)?$' % re.escape(name), decorated):
        return True
    tail = '@'.join(reversed(scopes))
    if name.startswith('~'):
        token = '??1' + (parts[-2] if scopes else name[1:]) + '@' + '@'.join(reversed(scopes[:-1]) if len(scopes) > 1 else []) + ('@' if len(scopes) > 1 else '') + '@'
        return token.replace('@@@', '@@') in decorated or ('??1' + name[1:] + '@') in decorated
    if scopes and name == scopes[-1]:
        return ('??0' + name + '@') in decorated
    token = '?' + name + '@' + (tail + '@' if tail else '') + '@'
    return token in decorated


def in_row_function(changed, syms, end, qualified):
    """The changed ranges all lie inside one linker-map function, and it is the one named `qualified`."""
    if not changed:
        return False
    first = symbol_at(syms, changed[0][0], end)
    if first is None or not symbol_matches(first[0], qualified):
        return False
    return all(first[1] <= lo and hi <= first[2] for lo, hi in changed)


# ---- one row ------------------------------------------------------------------------------------------------------

from pathlib import Path


class ControlNotExact(RuntimeError):
    """The unmutated source already reports a difference, so every mutant would look caught."""


LISTING = re.compile(r'//\s*0x([0-9a-f]{8})\s*$')


def line_address(text, offset):
    """The oracle address in the `// 0x........` listing comment at the end of the line holding `offset`, or None."""
    start = text.rfind('\n', 0, offset) + 1
    end = text.find('\n', offset)
    m = LISTING.search(text[start:end if end >= 0 else len(text)].rstrip())
    return int(m.group(1), 16) if m else None


def has_addresses(text, span):
    """True when the function at span is a listing transcription (its lines carry oracle addresses)."""
    return bool(re.search(r'//\s*0x[0-9a-f]{8}\s*$', text[span[0]:span[1]], re.M))


def shared_function(row_size, candidate_size):
    """True when the candidate function is far larger than the row: it implements other rows too, and an edit in it
    cannot be credited to this one."""
    return candidate_size > 2 * row_size + 64


def outside_oracle(text, span, m, oracle):
    """A reason when the mutated line of a listing transcription is not inside the row's oracle range, else None."""
    if not oracle or not has_addresses(text, span):
        return None
    addr = line_address(text, m[0])
    if addr is None or not oracle[0] <= addr < oracle[0] + oracle[1]:
        return 'line is outside the row\'s oracle range'
    return None


def describe(text, m):
    line = text.count('\n', 0, m[0]) + 1
    addr = line_address(text, m[0])
    return 'line %d: %s  ->  %s  (%s)%s' % (line, ' '.join(m[1].split())[:60], ' '.join(m[2].split())[:60], m[3],
                                            ' [0x%08x]' % addr if addr is not None else '')


def sweep_row(path, qualified, span, syms, end, baseline, run, build_dll, limit, survey=False, control=None,
              restore=None, write=None, mutations=None, oracle=None):
    """Try up to `limit` single-site mutations spread over the function at `span` in `path`.

    build_dll() rebuilds the candidate DLL from the (mutated) source on disk and returns its bytes; a mutation counts
    only when every changed byte lies in the one linker-map function `qualified` (else it is `rejected`). run(mutated
    text) returns the gate's detection count or None. Stops at the first catch unless `survey`. The file is restored
    after every attempt and whatever happens, and restore() rebuilds the original once at the end.
    Returns {'caught': [...], 'survived': [...], 'rejected': [...]} of (description, n or reason).
    """
    path = Path(path)
    original = path.read_bytes()
    text = original.decode('latin-1')
    if write is None:
        from mutate_rows import write_atomic as write
    out = {'caught': [], 'survived': [], 'rejected': []}
    try:
        if control is not None and control():
            raise ControlNotExact('the unmutated source already reports a difference')
        for m in (mutations if mutations is not None else spread(cpp_mutations(text, span[0], span[1]), limit)):
            mutated = apply_mutation(text, m)
            desc = describe(text, m)
            why = outside_oracle(text, span, m, oracle)
            if why:
                out['rejected'].append((desc, why))
                continue
            write(path, mutated.encode('latin-1'))
            try:
                changed = pe_changed_rvas(baseline, build_dll())
                if not changed:
                    out['rejected'].append((desc, 'no change in the DLL'))
                    continue
                if not in_row_function(changed, syms, end, qualified):
                    out['rejected'].append((desc, 'changed bytes outside %s' % qualified))
                    continue
                n = run(mutated)
            finally:
                write(path, original)
            if n:
                out['caught'].append((desc, n))
                if not survey:
                    break
            else:
                out['survived'].append((desc, None))
    finally:
        write(path, original)
        if restore:
            restore()
    if path.read_bytes() != original:
        raise RuntimeError('%s was not restored' % path)
    return out


# ---- a batch of rows ----------------------------------------------------------------------------------------------

def resolve_rva(syms, qualified):
    """The rva of the one linker-map function named `qualified`, or None when there is none or several."""
    hits = [a for a, name in syms if symbol_matches(name, qualified)]
    return hits[0] if len(hits) == 1 else None


def sweep_batch(entries, syms, end, baseline, run_family, build_all, limit, write=None):
    """One mutation per row in a single build and run, to screen many rows at once.

    entries: dicts with id, path, qualified, span, family, rva (the row's function in the linker map). Rows must have
    distinct families (ValueError). Round r applies every row's r-th mutation (mutations of the same file from the last
    offset to the first, so earlier offsets stay valid), build_all() rebuilds everything and returns the DLL bytes, and
    each row whose function changed (and nothing else of its own) has its family run: run_family(family) -> n or None.
    A detection is a *candidate*: another row's mutation in the same build may be what the family saw, so the caller
    confirms each candidate alone. A survivor is trustworthy, since extra mutations can only add detections.
    Returns {id: {'candidates': [(mutation, description, n)], 'survived': [...], 'rejected': [(mutation, description, reason)]}}.
    """
    fams = [e['family'] for e in entries]
    if len(fams) != len(set(fams)):
        raise ValueError('rows in one batch must have distinct families')
    if write is None:
        from mutate_rows import write_atomic as write
    originals = {}
    for e in entries:
        originals.setdefault(e['path'], Path(e['path']).read_bytes())
    texts = {p: b.decode('latin-1') for p, b in originals.items()}
    muts = {e['id']: spread(cpp_mutations(texts[e['path']], e['span'][0], e['span'][1]), limit) for e in entries}
    out = {e['id']: {'candidates': [], 'survived': [], 'rejected': []} for e in entries}
    try:
        for r in range(max([len(v) for v in muts.values()] + [0])):
            chosen = []
            for e in entries:
                if r >= len(muts[e['id']]):
                    continue
                m = muts[e['id']][r]
                why = outside_oracle(texts[e['path']], e['span'], m, e.get('oracle'))
                if why:
                    out[e['id']]['rejected'].append((m, describe(texts[e['path']], m), why))
                else:
                    chosen.append((e, m))
            if not chosen:
                continue
            by_file = {}
            for e, m in chosen:
                by_file.setdefault(e['path'], []).append(m)
            for path, ms in by_file.items():
                text = texts[path]
                for m in sorted(ms, key=lambda m: -m[0]):
                    text = apply_mutation(text, m)
                write(path, text.encode('latin-1'))
            try:
                dll = build_all()
                if dll is None:
                    for e, m in chosen:
                        out[e['id']]['rejected'].append((m, describe(texts[e['path']], m), 'build failed'))
                    continue
                changed = pe_changed_rvas(baseline, dll)
                for e, m in chosen:
                    desc = describe(texts[e['path']], m)
                    sym = symbol_at(syms, e['rva'], end)
                    mine = [c for c in changed if sym and sym[1] <= c[0] < sym[2]]
                    if not mine:
                        out[e['id']]['rejected'].append((m, desc, 'no change in the DLL'))
                    elif not symbol_matches(sym[0], e['qualified']) or any(c[1] > sym[2] for c in mine):
                        out[e['id']]['rejected'].append((m, desc, 'changed bytes outside %s' % e['qualified']))
                    else:
                        n = run_family(e['family'])
                        if n:
                            out[e['id']]['candidates'].append((m, desc, n))
                        else:
                            out[e['id']]['survived'].append((m, desc, None))
            finally:
                for path, b in originals.items():
                    write(path, b)
    finally:
        for path, b in originals.items():
            write(path, b)
    for path, b in originals.items():
        if Path(path).read_bytes() != b:
            raise RuntimeError('%s was not restored' % path)
    return out


# ---- the closing manifest -----------------------------------------------------------------------------------------

def write_manifest(lines, doc, sources, families=None):
    """The `close_static_rows.py --falsified` manifest for the first CAUGHT line of each row in a log.

    sources: row id -> source file (named in the mutation text); families: row id -> family name (default: the target,
    which is what a staged-pair gate calls its family)."""
    families = families or {}
    rows = {}
    for line in lines:
        m = re.match(r'CAUGHT (phys_fn_\d+) (\w+) (stdout_delta|mismatches)=(\d+) (line \d+: .*)$', line.strip())
        if not m or m.group(1) in rows:
            continue
        rid, gate, kind, n, desc = m.groups()
        rows[rid] = {'gate': gate, 'family': families.get(rid, gate), 'mutation': '%s %s' % (sources.get(rid, '?'), ' '.join(desc.split())),
                     'detected': '%s=%s' % (kind, n), 'source_mutation': True}
        if re.search(r'\[0x[0-9a-f]{8}\]$', desc.strip()):
            rows[rid]['listing'] = True
    return {'doc': doc, 'rows': rows}


def main_manifest(argv=None):
    import argparse
    import json
    here = Path(__file__).resolve().parent
    ap = argparse.ArgumentParser(description='write the closing manifest from a mutate_cpp log')
    ap.add_argument('--manifest', required=True, help='the log')
    ap.add_argument('--doc', required=True, help='the evidence file the manifest cites (relative to the novodex-physics directory)')
    ap.add_argument('--out', required=True)
    ap.add_argument('--inventory', default=str(here.parent / 'inventory.json'))
    ap.add_argument('--family', action='append', default=[], help='ROW=family (repeatable) for a harness family')
    a = ap.parse_args(argv)
    inv = {r['id']: r for r in json.load(open(a.inventory, encoding='utf-8'))['functions']}
    sources = {rid: (r.get('implementation') or r.get('source')) for rid, r in inv.items()}
    fams = dict(f.split('=', 1) for f in a.family)
    m = write_manifest(open(a.manifest, encoding='utf-8').read().splitlines(), a.doc, sources, fams)
    Path(a.out).write_text(json.dumps(m, indent=1), encoding='utf-8')
    print('manifest rows %d' % len(m['rows']))
    return 0


# ---- command line -------------------------------------------------------------------------------------------------

def locate(row, repo):
    """(source path, text, span, qualified name) of a row's C++ function, or (path, text, None, reason)."""
    rel = row.get('implementation') or row.get('source')
    if not rel:
        return None, None, None, 'no source file'
    import vendor_gate
    path = Path(repo) / (vendor_gate.merged_path(rel) or rel)
    if not path.is_file():
        return path, None, None, 'source file missing'
    text = path.read_bytes().decode('latin-1')   # byte offsets, CRLF kept: a span must index the bytes that are rewritten
    span = (find_function_span(text, row['implementation_symbol']) if row.get('implementation_symbol') else None) \
        or find_span_by_marker(text, row['id'])
    if span is None:
        return path, text, None, 'function not found'
    return path, text, span, function_name_at(text, span)


def main(argv=None):
    import argparse
    import json
    import mutate_rows as mr
    here = Path(__file__).resolve().parent
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--row', required=True, help='phys_fn_NNNNNN')
    ap.add_argument('--inventory', default=str(here.parent / 'inventory.json'))
    ap.add_argument('--target', required=True)
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument('--family')
    g.add_argument('--staged', action='store_true')
    ap.add_argument('--exe-arg', action='append', default=[])
    ap.add_argument('--count-crash', action='store_true')
    ap.add_argument('--limit', type=int, default=16)
    ap.add_argument('--survey', action='store_true')
    ap.add_argument('--map', default=str(mr.REPO / 'build' / 'Release' / 'NxPhysics.map'))
    ap.add_argument('--dll', default=str(mr.REPO / 'build' / 'Release' / 'NxPhysics.dll'))
    ap.add_argument('--log')
    a = ap.parse_args(argv)
    inv = json.load(open(a.inventory, encoding='utf-8'))
    row = next(r for r in inv['functions'] if r['id'] == a.row)
    path, text, span, qualified = locate(row, mr.REPO)
    if span is None:
        print('NOT LOCATED %s %s' % (a.row, qualified))
        return 2
    if not mr._build('NxPhysics', False):
        print('BUILD FAILED (baseline)')
        return 2
    baseline = Path(a.dll).read_bytes()
    syms = read_text_symbols(a.map)
    sections, _ = _pe_layout(baseline)
    end = sections[0][0] + sections[0][1]
    real = mr._real_run(a.target, a.family, a.staged, a.count_crash, a.exe_arg)

    def run(mutated):
        kind, n = real(mutated)
        run.kind = kind
        return n or None
    run.kind = 'mismatches'

    def restore():
        mr._build('NxPhysics', False)
        mr._build(a.target, a.staged)
    out = sweep_row(str(path), qualified, span, syms, end, baseline, run, lambda: (mr._build('NxPhysics', False) and Path(a.dll).read_bytes()) or b'',
                    a.limit, survey=a.survey, control=lambda: run(text), restore=restore)
    lines = []
    for desc, n in out['caught']:
        lines.append('CAUGHT %s %s %s=%s %s' % (a.row, a.target, 'stdout_delta' if a.staged else 'mismatches', n, desc))
    summary = 'SURVEY %s %s tried=%d caught=%d survived=%d rejected=%d' % (
        a.row, a.target, len(out['caught']) + len(out['survived']), len(out['caught']), len(out['survived']), len(out['rejected']))
    if a.survey:
        lines.append(summary)
        lines += ['  SURVIVED ' + d for d, _ in out['survived']] + ['  rejected ' + d + '  [' + r + ']' for d, r in out['rejected']]
    elif not out['caught']:
        lines.append('NOT CAUGHT %s (%d survived, %d rejected)' % (a.row, len(out['survived']), len(out['rejected'])))
    print('\n'.join(lines))
    if a.log:
        with open(a.log, 'a', encoding='utf-8') as f:
            f.write('\n'.join(lines) + '\n')
    return 0 if out['caught'] else 1


VOLATILE = ('pair_directory=', 'modules pair=', 'loaded module=')


def output_lines(stdout):
    """The lines of an oracle-only target's output that name the work, not the pair that was loaded."""
    return [l for l in stdout.splitlines() if not l.startswith(VOLATILE)]


def digest_difference(baseline, stdout, returncode, count_crash=False, baseline_rc=0):
    """The number of output lines that differ from the unmutated run, or None when nothing differs. A different exit
    code with unchanged output counts only under count_crash."""
    now = output_lines(stdout)
    if now == baseline:
        return 1 if (returncode != baseline_rc and count_crash) else None
    return max(1, len(set(now) ^ set(baseline)))


def run_gate(mr, spec):
    """Run an already built gate once; the detection count or None."""
    import subprocess
    try:
        if spec.get('lines'):
            import vendor_gate
            cmd = [mr.harness_command(spec['target'], [])[0], mr.ORACLE_DIR.replace('/', chr(92)), mr.ORACLE_SHA]
            now = vendor_gate.parse_lines(mr.run_tree(cmd, timeout=mr.TIMEOUT, cwd=mr.REPO).stdout)
            return vendor_gate.detection(spec['base_lines'], now, spec.get('families') or [])
        if spec.get('oracle_only'):
            cmd = [mr.harness_command(spec['target'], [])[0], mr.ORACLE_DIR.replace('/', chr(92)), mr.ORACLE_SHA]
            r = mr.run_tree(cmd, timeout=mr.TIMEOUT, cwd=mr.REPO)
            return digest_difference(spec['baseline'], r.stdout, r.returncode, spec.get('count_crash', False), spec.get('baseline_rc', 0))
        if spec.get('staged'):
            r = mr.run_tree(['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                             'docs/reconstruction/novodex-physics/tools/run_differential.ps1', '-Targets', spec['target']],
                            timeout=mr.TIMEOUT, cwd=mr.REPO)
            return mr.detect_staged(r.stdout, spec.get('count_crash', False))
        r = mr.run_tree(mr.harness_command(spec['target'], spec.get('exe_args')), timeout=mr.TIMEOUT, cwd=mr.REPO)
        return mr.detect_harness(r.stdout, r.returncode, spec.get('gate_family') or spec['family'], spec.get('count_crash', False))
    except subprocess.TimeoutExpired:
        return None


def main_batch(argv=None):
    """--plan FILE: screen many rows per build, then confirm each candidate alone. Plan: {"rows": [{"id", "target",
    "family", "staged", "exe_args", "count_crash"}]}; the rows must have distinct (target, family)."""
    import argparse
    import json
    import mutate_rows as mr
    here = Path(__file__).resolve().parent
    ap = argparse.ArgumentParser(description=main_batch.__doc__.splitlines()[0])
    ap.add_argument('--plan', required=True)
    ap.add_argument('--inventory', default=str(here.parent / 'inventory.json'))
    ap.add_argument('--limit', type=int, default=16)
    ap.add_argument('--map', default=str(mr.REPO / 'build' / 'Release' / 'NxPhysics.map'))
    ap.add_argument('--dll', default=str(mr.REPO / 'build' / 'Release' / 'NxPhysics.dll'))
    ap.add_argument('--log', required=True)
    a = ap.parse_args(argv)
    plan = json.load(open(a.plan, encoding='utf-8'))['rows']
    inv = {r['id']: r for r in json.load(open(a.inventory, encoding='utf-8'))['functions']}
    targets = sorted({p['target'] for p in plan if not p.get('staged')})
    if not mr._build('NxPhysics', False):
        print('BUILD FAILED (baseline)')
        return 2
    for t in targets:
        if not mr._build(t, False):
            print('BUILD FAILED (baseline %s)' % t)
            return 2
    baseline = Path(a.dll).read_bytes()
    syms = read_text_symbols(a.map)
    sections, _ = _pe_layout(baseline)
    end = sections[0][0] + sections[0][1]
    for p in plan:
        if p.get('lines'):
            import vendor_gate
            cmd = [mr.harness_command(p['target'], [])[0], mr.ORACLE_DIR.replace('/', chr(92)), mr.ORACLE_SHA]
            first = vendor_gate.parse_lines(mr.run_tree(cmd, timeout=mr.TIMEOUT, cwd=mr.REPO).stdout)
            again = vendor_gate.parse_lines(mr.run_tree(cmd, timeout=mr.TIMEOUT, cwd=mr.REPO).stdout)
            if first != again or not first:
                print('NOT DETERMINISTIC %s (%s); the plan is refused' % (p['id'], p['target']))
                return 2
            p['base_lines'] = first
        if p.get('oracle_only'):
            cmd = [mr.harness_command(p['target'], [])[0], mr.ORACLE_DIR.replace('/', chr(92)), mr.ORACLE_SHA]
            run1 = mr.run_tree(cmd, timeout=mr.TIMEOUT, cwd=mr.REPO)
            run2 = mr.run_tree(cmd, timeout=mr.TIMEOUT, cwd=mr.REPO)
            first, again = output_lines(run1.stdout), output_lines(run2.stdout)
            p['baseline_rc'] = run1.returncode
            if first != again or not first or run1.returncode != run2.returncode:
                print('NOT DETERMINISTIC %s (%s); the plan is refused' % (p['id'], p['target']))
                return 2
            p['baseline'] = first
    entries, skipped = [], []
    for p in plan:
        row = inv[p['id']]
        path, text, span, qualified = locate(row, mr.REPO)
        rva = resolve_rva(syms, qualified) if span else None
        if span is None or rva is None:
            skipped.append((p['id'], qualified if span is None else 'no unique map symbol for %s' % qualified))
            continue
        size = int(row['size'])
        cand = symbol_at(syms, rva, end)
        if not has_addresses(text, span) and shared_function(size, cand[2] - cand[1]):
            skipped.append((p['id'], 'the candidate function %s (%d bytes) is far larger than the row (%d bytes): other rows share it' % (qualified, cand[2] - cand[1], size)))
            continue
        # key: what keeps two rows of one family out of a batch; gate_family: what the gate prints
        entries.append(dict(p, path=str(path), span=span, qualified=qualified, rva=rva, oracle=(int(row['rva'], 16), size),
                            gate_family=p.get('family') or '', family=p['target'] + ':' + (p['id'] if p.get('lines') else (p.get('family') or ''))))
    kept = []
    for e in entries:
        if run_gate(mr, e):
            skipped.append((e['id'], 'the unmutated run already reports a difference (control not exact)'))
        else:
            kept.append(e)
    entries = kept
    specs = {e['family']: e for e in entries}

    round_out = {}

    def run_family(fam):
        """One run of a lines gate serves every row of the build; any other gate is run per row."""
        e = specs[fam]
        if not e.get('lines'):
            return run_gate(mr, e)
        import vendor_gate
        if 'now' not in round_out:
            cmd = [mr.harness_command(e['target'], [])[0], mr.ORACLE_DIR.replace('/', chr(92)), mr.ORACLE_SHA]
            try:
                round_out['now'] = vendor_gate.parse_lines(mr.run_tree(cmd, timeout=mr.TIMEOUT, cwd=mr.REPO).stdout)
            except Exception:
                round_out['now'] = e['base_lines']
        return vendor_gate.detection(e['base_lines'], round_out['now'], e.get('families') or [])

    def build_all():
        round_out.clear()
        mr.remove_stale_dll(mr.REPO)
        ok = mr._build('NxPhysics', False)
        for t in targets:
            ok = ok and mr._build(t, False)
        return Path(a.dll).read_bytes() if ok else None

    out = sweep_batch(entries, syms, end, baseline, run_family, build_all, a.limit)
    lines = []
    for e in entries:
        res = out[e['id']]
        confirmed, failed, screened = [], 0, 0
        for m, desc, n in res['candidates']:
            # a candidate is run alone because in a batch another row's mutation may be what the family saw; once two
            # candidates have held up alone and none has failed, the rest of the row's candidates are taken as caught
            if len(confirmed) >= 2 and not failed:
                screened += 1
                continue
            def rebuild(e=e):
                mr.remove_stale_dll(mr.REPO)
                return (mr._build('NxPhysics', False) and mr._build(e['target'], False) and Path(a.dll).read_bytes()) or b''
            single = sweep_row(e['path'], e['qualified'], e['span'], syms, end, baseline, lambda _t, e=e: run_gate(mr, e),
                               rebuild, 1, survey=True, mutations=[m], oracle=e.get('oracle'))
            confirmed += single['caught']
            if not single['caught']:
                failed += 1
                res['survived'].append((m, desc, None))
        lines.append('SURVEY %s %s tried=%d caught=%d survived=%d rejected=%d' % (
            e['id'], e['target'], len(confirmed) + screened + len(res['survived']), len(confirmed) + screened, len(res['survived']), len(res['rejected'])) +
                     ' nobuild=%d' % sum(1 for x in res['rejected'] if x[2] == 'build failed'))
        for desc, n in confirmed:
            lines.append('CAUGHT %s %s %s=%s %s' % (e['id'], e['target'], 'stdout_delta' if e.get('staged') else 'mismatches', n, desc))
        lines += ['  SURVIVED ' + d for _m, d, _n in res['survived']] + ['  rejected ' + d + '  [' + why + ']' for _m, d, why in res['rejected']]
    for rid, why in skipped:
        lines.append('SKIPPED %s %s' % (rid, why))
    for t in targets + ['NxPhysics']:
        mr._build(t, False)
    print(chr(10).join(lines))
    with open(a.log, 'a', encoding='utf-8') as f:
        f.write(chr(10).join(lines) + chr(10))
    return 0


if __name__ == '__main__':
    import sys
    sys.exit(main_batch() if '--plan' in sys.argv else main_manifest() if '--manifest' in sys.argv else main())
