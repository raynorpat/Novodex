"""Guard upstream function bodies in a novodex/ copy with NX_VENDORED_LISTING_ROWS.

usage: python guard.py <novodex file> <upstream rel path> <rows file> <row id> <rva> "<definition line prefix>" [...more id rva prefix]

The definition starts at the line beginning with the prefix and ends at the next line that is exactly `}`.
A file without a NOVODEX LOCAL MODIFICATION block gets one; each guarded function adds an entry to it.
Line endings are kept.
"""
import sys

path, upstream, rows_file = sys.argv[1:4]
specs = [sys.argv[k:k + 3] for k in range(4, len(sys.argv), 3)]
raw = open(path, encoding='latin-1', newline='').read()
nl = '\r\n' if '\r\n' in raw else '\n'
lines = raw.split(nl)
entries = []
for rid, rva, prefix in specs:
    starts = [k for k, l in enumerate(lines) if l.startswith(prefix)]
    if len(starts) != 1:
        raise SystemExit('%s: %d definitions start with %r' % (path, len(starts), prefix))
    a = starts[0]
    b = next(k for k in range(a, len(lines)) if lines[k] == '}')
    lines[a:b + 1] = (['#ifndef NX_VENDORED_LISTING_ROWS'] + lines[a:b + 1] +
                      ['#endif', '/* NX_VENDORED_LISTING_ROWS: transcribed in %s (%s) */' % (rows_file, rid)])
    entries.append(' * [vendored rows] %s (%s): the body is guarded off; the oracle\'s own instructions are transcribed in'
                   % (prefix.strip(), rid))
    entries.append(' *     %s. established at %s' % (rows_file, rva))
text = nl.join(lines)
if 'NOVODEX LOCAL MODIFICATION' not in text:
    head = ['/*', ' * NOVODEX LOCAL MODIFICATION', ' * upstream: %s' % upstream, ' *'] + entries + [' */']
    text = nl.join(head) + nl + text
else:
    end = text.index(' */', text.index('NOVODEX LOCAL MODIFICATION'))
    text = text[:end] + nl.join(entries) + nl + text[end:]
open(path, 'w', encoding='latin-1', newline='').write(text)
print('guarded %d in %s' % (len(specs), path))
