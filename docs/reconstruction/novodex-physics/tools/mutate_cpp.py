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
    for m in re.finditer(r'(?<=[\w\)\]])\s(\*)\s(?=[\w\(])', seg):
        out.append((lo + m.start(1), '*', '+', 'arith'))
    for m in re.finditer(r'(?<![\w.])(\d+)(?![\w.])', seg):
        v = m.group(1)
        if len(v) > 1 and v.startswith('0'):
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
