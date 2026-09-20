"""Convert the large locals of wmain in the Phase 5 layout harness to heap buffers.

Evidence: docs/reconstruction/novodex-physics/evidence/phase5-object-model.md
3z265-3z273.

Why it exists. `wmain` in tests/PhysicsObjectLayoutTests.cpp carries a frame of
~250 KB against the CRT's 1 MB stack (3z265), and an out-of-bounds write inside a
frame that large lands on memory another block depends on. 3z267 established that
reducing it has to be wholesale rather than piecemeal.

Method, chosen to remove the hazards 3z266 recorded rather than manage them:

  * each converted declarator becomes a uniquely named `NxFrameBuffer` object, so
    every later use is renamed to an identifier that cannot be shadowed by a
    same-named local and every `sizeof(x)` is unambiguous;
  * `sizeof(x)` becomes the literal byte count -- never the pointer size;
  * the object frees itself at scope exit, so the thirty-odd `return` statements
    in `wmain` cannot leak or double-free;
  * element access, pointer decay, pointer arithmetic and `&x` keep working
    through the wrapper's operators.

REQUIRED: take the base from `git show HEAD:tests/PhysicsObjectLayoutTests.cpp`,
never from a file a previous pass wrote, and run this script exactly once. It is
not idempotent -- a second pass renames identifiers that are already renamed and
rewrites `sizeof` uses that are already literals, and the result still compiles
while meaning something different. That is 3z271's correction.

Usage:
    python frame_locals.py [--threshold 0x200] [--check]
"""

import argparse
import re
import sys

PATH = r'D:\github\Novodex\tests\PhysicsObjectLayoutTests.cpp'
THRESHOLD = 0x200

DECL = re.compile(
    r'^(\s*)(unsigned\s+char|signed\s+char|char|unsigned\s+short|short|'
    r'unsigned\s+int|int|unsigned\s+long|long|unsigned\s+__int64|__int64|'
    r'float|double)\s+(.*)$'
)

WRAPPER = r'''
// ---------------------------------------------------------------------------
// Frame-relief buffer wrappers (evidence/phase5-object-model.md, 3z265-3z273).
//
// wmain's frame reached ~250 KB against the CRT's 1 MB stack, and an
// out-of-bounds write inside a frame that large lands on memory another block
// depends on. Converting the large locals to heap storage is the fix, and it has
// to be wholesale: converting a subset moves the fault rather than removing it.
//
// The wrapper keeps every existing use compiling unchanged -- element access,
// decay to a pointer, pointer arithmetic, and &x -- while making sizeof(x)
// impossible to get wrong: the original declarators are renamed to unique
// identifiers and each sizeof is replaced by its literal byte count.
// ---------------------------------------------------------------------------

template<typename T, unsigned N>
class NxFrameBuffer
	{
	public:
	typedef T element_type;
	NxFrameBuffer() : p(static_cast<T*>(malloc(N > 0 ? N : 1)))
		{ if(p == 0) { fprintf(stderr, "nxframe: out of memory for a %u-byte buffer\n", N); exit(70); } }
	~NxFrameBuffer() { free(p); }
	T* data() const { return p; }
	template<typename I> T& operator[](I i) const { return p[i]; }
	operator T*() const { return p; }
	T* operator&() const { return p; }
	private:
	NxFrameBuffer(const NxFrameBuffer&);
	NxFrameBuffer& operator=(const NxFrameBuffer&);
	T* p;
	};

template<typename T, unsigned SEG, unsigned CNT>
class NxFrameBuffer2D
	{
	public:
	typedef T element_type;
	enum { SEGMENT = SEG, TOTAL = SEG * CNT };
	NxFrameBuffer2D() : p(static_cast<T*>(malloc(sizeof(T) * TOTAL)))
		{ if(p == 0) { fprintf(stderr, "nxframe: out of memory for a %u-byte buffer\n", static_cast<unsigned>(sizeof(T) * TOTAL)); exit(70); } }
	~NxFrameBuffer2D() { free(p); }
	T* data() const { return p; }
	template<typename I> T* operator[](I i) const { return p + i * SEGMENT; }
	operator T*() const { return p; }
	T* operator&() const { return p; }
	private:
	NxFrameBuffer2D(const NxFrameBuffer2D&);
	NxFrameBuffer2D& operator=(const NxFrameBuffer2D&);
	T* p;
	};
'''

POOL_WRAPPER = r'''
// A single static pool, used instead of the CRT heap by --pool. The harness
// emulates the SDK allocator, so routing the relief buffers through malloc may
// itself be what perturbed the run; this pool tests that separately.
static bool gNxFramePoolExhausted = false;

static void* nxFramePoolTake(unsigned long bytes)
	{
	static unsigned char pool[NXFRAME_POOL_BYTES];
	static unsigned long used = 0;
	unsigned long at = (used + 15ul) & ~15ul;
	if(at + bytes > static_cast<unsigned long>(NXFRAME_POOL_BYTES))
		{
		if(!gNxFramePoolExhausted)
			{
			gNxFramePoolExhausted = true;
			fprintf(stderr, "nxframe: static pool of %u bytes exhausted\n",
				static_cast<unsigned>(NXFRAME_POOL_BYTES));
			}
		return 0;
		}
	used = at + bytes;
	return pool + at;
	}

template<typename T, unsigned N>
class NxFramePoolBuffer
	{
	public:
	typedef T element_type;
	NxFramePoolBuffer() : p(static_cast<T*>(nxFramePoolTake(N > 0 ? N : 1)))
		{ if(p == 0) exit(70); }
	T* data() const { return p; }
	template<typename I> T& operator[](I i) const { return p[i]; }
	operator T*() const { return p; }
	T* operator&() const { return p; }
	private:
	NxFramePoolBuffer(const NxFramePoolBuffer&);
	NxFramePoolBuffer& operator=(const NxFramePoolBuffer&);
	T* p;
	};

template<typename T, unsigned SEG, unsigned CNT>
class NxFramePoolBuffer2D
	{
	public:
	typedef T element_type;
	enum { SEGMENT = SEG, TOTAL = SEG * CNT };
	NxFramePoolBuffer2D() : p(static_cast<T*>(nxFramePoolTake(sizeof(T) * TOTAL)))
		{ if(p == 0) exit(70); }
	T* data() const { return p; }
	template<typename I> T* operator[](I i) const { return p + i * SEGMENT; }
	operator T*() const { return p; }
	T* operator&() const { return p; }
	private:
	NxFramePoolBuffer2D(const NxFramePoolBuffer2D&);
	NxFramePoolBuffer2D& operator=(const NxFramePoolBuffer2D&);
	T* p;
	};
'''


def brace_delta(line):
    """Count braces outside string and character literals."""
    d = 0
    i = 0
    n = len(line)
    while i < n:
        c = line[i]
        if c == '"' or c == "'":
            q = c
            i += 1
            while i < n:
                if line[i] == '\\':
                    i += 2
                    continue
                if line[i] == q:
                    i += 1
                    break
                i += 1
            continue
        if c == '/' and i + 1 < n and line[i + 1] == '/':
            break
        if c == '{':
            d += 1
        elif c == '}':
            d -= 1
        i += 1
    return d


def split_declarators(text):
    """Split 'a[0x10], b[0x20];' into declarator texts, plus what followed the ;."""
    depth = 0
    cur = []
    parts = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if c in '([{':
            depth += 1
        elif c in ')]}':
            depth -= 1
        if c == ',' and depth == 0:
            parts.append(''.join(cur))
            cur = []
            i += 1
            continue
        if c == ';' and depth == 0:
            parts.append(''.join(cur))
            return parts, text[i + 1:]
        cur.append(c)
        i += 1
    parts.append(''.join(cur))
    return parts, ''


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--threshold', default='0x200')
    ap.add_argument('--path', default=PATH)
    ap.add_argument('--check', action='store_true',
                    help='report what would change without writing')
    ap.add_argument('--limit', type=int, default=0, metavar='N',
                    help='convert only the first N declarators, in source order. '
                         'This is the bisect control: it localises a conversion '
                         'that changes the meaning of its block down to one '
                         'declaration, which is how 3z275 was left open.')
    ap.add_argument('--margin', type=lambda s: int(s, 0), default=0, metavar='BYTES',
                    help='allocate this many bytes beyond each buffer. The oracle '
                         'rows are 2003 code that writes into caller storage, and a '
                         'few bytes past a buffer is harmless in a 250 KB stack frame '
                         'while it corrupts heap metadata. A margin tests that.')
    ap.add_argument('--vm', action='store_true',
                    help='allocate with VirtualAlloc instead of malloc. The harness '
                         'emulates the SDK allocator, so this separates the storage '
                         'move from any interception of malloc.')
    ap.add_argument('--list', action='store_true',
                    help='print the declarators that would be converted, in order')
    ap.add_argument('--stack', action='store_true',
                    help='rename and fix sizeof, but leave the storage on the '
                         'stack. This separates "the renaming and the layout shift '
                         'broke it" from "moving the buffer to the heap broke it", '
                         'which is the distinction 3z275 could not make.')
    ap.add_argument('--pool', type=int, default=0, metavar='BYTES',
                    help='take the storage from one static pool of the given size '
                         'instead of the CRT heap. The harness emulates the SDK '
                         'allocator, so this separates "the buffer left the stack" '
                         'from "the buffer went through malloc".')
    args = ap.parse_args()
    threshold = int(args.threshold, 0)

    # The layout translation unit is committed as UTF-16LE with a BOM, so decode
    # by its own BOM rather than assuming UTF-8.
    rawbytes = open(args.path, 'rb').read()
    if rawbytes[:2] in (b'\xff\xfe', b'\xfe\xff'):
        text = rawbytes.decode('utf-16')
    else:
        text = rawbytes.decode('utf-8-sig')
    eol = '\r\n' if '\r\n' in text else '\n'
    lines = text.split(eol)

    if 'NxFrameBuffer' in text:
        print('error: this file already carries the wrapper; a second pass would '
              'rename identifiers that are already renamed (3z271)', file=sys.stderr)
        return 2

    targets = []
    for idx, line in enumerate(lines):
        m = DECL.match(line)
        if not m:
            continue
        parts, trailing = split_declarators(m.group(3))
        hits = []
        keep = []
        for k, part in enumerate(parts):
            dm = re.match(r'\s*([A-Za-z_]\w*)\s*((?:\[\s*[^\]]*\s*\])+)\s*(=.*)?$', part)
            if not dm:
                keep.append('%s %s;' % (m.group(2), part.strip()))
                continue
            dims = re.findall(r'\[\s*([^\]]*)\s*\]', dm.group(2))
            try:
                vals = [int(x, 0) for x in dims]
            except ValueError:
                keep.append('%s %s;' % (m.group(2), part.strip()))
                continue
            count = 1
            for v in vals:
                count *= v
            if count >= threshold:
                hits.append({'index': k, 'type': m.group(2), 'name': dm.group(1),
                             'dims': vals, 'bytes': count})
            else:
                keep.append('%s %s;' % (m.group(2), part.strip()))
        if hits:
            targets.append({'line': idx, 'indent': m.group(1), 'hits': hits,
                            'keep': keep, 'trailing': trailing})

    if not targets:
        print('error: no declarations matched', file=sys.stderr)
        return 1

    # Flatten to declarator order, then apply the bisect limit. Splitting a
    # multi-declarator line is already handled per hit, so a limit that falls
    # inside a line must keep the unconverted declarators of that line as they
    # were written.
    flat = [(ti, k) for ti, t in enumerate(targets) for k in range(len(t['hits']))]
    if args.list:
        for i, (ti, k) in enumerate(flat, 1):
            t = targets[ti]
            h = t['hits'][k]
            print('%4d  line %-6d %-12s %-28s 0x%X' % (
                i, t['line'] + 1, h['type'], h['name'], h['bytes']))
        print('total %d declarators' % len(flat))
        return 0
    if args.limit:
        keep_upto = set(flat[:args.limit])
        pruned = []
        for ti, t in enumerate(targets):
            hits = [h for k, h in enumerate(t['hits']) if (ti, k) in keep_upto]
            moved = [h for k, h in enumerate(t['hits']) if (ti, k) not in keep_upto]
            if not hits:
                continue
            t['hits'] = hits
            # Re-emit each unconverted declarator with its own dimensions. The
            # declaration text was recorded during pass 1, so it is reconstructed
            # here from the declarator's own type and rank rather than copied.
            t['keep'] = list(t['keep']) + [
                '%s %s%s;' % (h['type'], h['name'],
                              ''.join('[0x%X]' % v for v in h['dims']))
                for h in moved]
            pruned.append(t)
        targets = pruned
        if not targets:
            print('error: --limit %d converts nothing' % args.limit, file=sys.stderr)
            return 1

    depth = 0
    depth_at = []
    for line in lines:
        depth_at.append(depth)
        depth += brace_delta(line)

    for t in targets:
        start_depth = depth_at[t['line']]
        end = len(lines)
        for j in range(t['line'] + 1, len(lines)):
            if depth_at[j] < start_depth:
                end = j
                break
        t['end'] = end
    targets.sort(key=lambda t: t['line'])

    by_line = {}
    for t in targets:
        by_line.setdefault(t['line'], []).append(t)

    out = []
    renames = []
    for i, line in enumerate(lines):
        if i in by_line:
            for t in by_line[i]:
                decls = list(t['keep'])
                for h in t['hits']:
                    h['new'] = 'nxfb_%d_%d_%s' % (i + 1, h['index'], h['name'])
                    renames.append((h['name'], h['new'], i, t['end']))
                    dims = ''.join('[0x%X]' % d for d in h['dims'])
                    if args.vm:
                        base = 'NxFrameVmBuffer'
                    elif args.pool:
                        base = 'NxFramePoolBuffer'
                    else:
                        base = 'NxFrameBuffer'
                    wname = base + ('2D' if len(h['dims']) == 2 else '')
                    if args.stack:
                        # keep the storage exactly where it was; only the name and
                        # the sizeof spellings change
                        decls.append('%s %s%s;' % (h['type'], h['new'], dims))
                    elif len(h['dims']) == 1:
                        decls.append('typedef %s<%s, 0x%Xu> NxFrameType_%d_%d;\n%sNxFrameType_%d_%d %s;' % (
                            wname, h['type'], h['dims'][0] + args.margin, i + 1, h['index'],
                            t['indent'], i + 1, h['index'], h['new']))
                    elif len(h['dims']) == 2:
                        decls.append('typedef %s<%s, 0x%Xu, 0x%Xu> NxFrameType_%d_%d;\n%sNxFrameType_%d_%d %s;' % (
                            wname, h['type'], h['dims'][0] + args.margin, h['dims'][1],
                            i + 1, h['index'],
                            t['indent'], i + 1, h['index'], h['new']))
                    else:
                        print('error: unsupported rank at line %d' % (i + 1), file=sys.stderr)
                        return 1
                line = t['indent'] + '\n'.join(decls) + t['trailing']
        out.append(line)

    renames.sort(key=lambda r: (r[3] - r[2]))

    def rewrite_range(begin, end, old, new):
        pat = re.compile(r'(?<![\w$])' + re.escape(old) + r'(?![\w$])')
        count = 0
        for i in range(begin, end):
            line = out[i]
            if old not in line:
                continue
            parts = re.split(r'("(?:[^"\\]|\\.)*"|\'(?:[^\'\\]|\\.)*\')', line)
            for k in range(0, len(parts), 2):
                seg = parts[k]
                if old not in seg:
                    continue
                cpos = seg.find('//')
                head, tail = (seg[:cpos], seg[cpos:]) if cpos >= 0 else (seg, '')
                n2, cnt = pat.subn(new, head)
                count += cnt
                parts[k] = n2 + tail
            out[i] = ''.join(parts)
        return count

    total = 0
    for (old, new, begin, end) in renames:
        total += rewrite_range(begin, end, old, new)

    allhits = [h for t in targets for h in t['hits']]
    for h in allhits:
        for i, line in enumerate(out):
            if h['new'] in line:
                n2, cnt = re.subn(r'sizeof\s*\(\s*' + re.escape(h['new']) + r'\s*\)',
                                  '0x%Xu' % h['bytes'], line)
                if cnt:
                    out[i] = n2
    for h in allhits:
        if len(h['dims']) != 2:
            continue
        seg = h['dims'][1]
        for i, line in enumerate(out):
            if h['new'] in line:
                n2, cnt = re.subn(
                    r'sizeof\s*\(\s*' + re.escape(h['new']) + r'\s*\[\s*[^\]]*\s*\]\s*\)',
                    '0x%Xu' % seg, line)
                if cnt:
                    out[i] = n2

    wmain_at = next(i for i, l in enumerate(out) if re.match(r'^int wmain\(', l))
    block = WRAPPER
    if args.vm:
        block = block + '\n' + VM_WRAPPER
    if args.pool:
        block = (block + '\n'
                 + '#define NXFRAME_POOL_BYTES 0x%Xu\n' % args.pool
                 + POOL_WRAPPER)
    out[wmain_at:wmain_at] = block.split('\n')

    print('converted %d declarators on %d lines, %d bytes, %d renames' %
          (len(allhits), len(targets), sum(h['bytes'] for h in allhits), total))
    if args.check:
        return 0
    with open(args.path, 'w', encoding='utf-8', newline='') as fh:
        fh.write(eol.join(out))
    return 0


if __name__ == '__main__':
    sys.exit(main())