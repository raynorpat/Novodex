"""Route explicit casts on converted buffers through the wrapper's conversion.

A class with a user-defined conversion operator cannot be the operand of a
C-style cast to an unrelated pointer type or of `reinterpret_cast` -- MSVC raises
C2440. The wrapper's `operator unsigned char*()` is the intended bridge, so an
explicit cast on a converted buffer becomes `static_cast<unsigned char*>(x)`
first.

Run this after frame_locals.py, exactly once, on the same file.

Usage:
    python frame_casts.py [--path <file>] [--check]
"""

import argparse
import re
import sys

PATH = r'D:\github\Novodex\tests\PhysicsObjectLayoutTests.cpp'


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--path', default=PATH)
    ap.add_argument('--check', action='store_true')
    args = ap.parse_args()

    rawbytes = open(args.path, 'rb').read()
    text = (rawbytes.decode('utf-16') if rawbytes[:2] in (b'\xff\xfe', b'\xfe\xff')
            else rawbytes.decode('utf-8-sig'))
    eol = '\r\n' if '\r\n' in text else '\n'
    lines = text.split(eol)

    names = set()
    for line in lines:
        names.update(re.findall(r'\b(nxfb_\d+_\d+_\w+)\b', line))
    if not names:
        print('error: no converted buffers found; run frame_locals.py first',
              file=sys.stderr)
        return 2
    names = sorted(names, key=len, reverse=True)
    alt = '|'.join(re.escape(n) for n in names)

    patterns = [
        # (uintptr_t)(buf)
        (re.compile(r'(\(\s*(?:const\s+)?(?:uintptr_t|size_t|unsigned|int|long)\s*\))\s*\(\s*(' + alt + r')\s*\)'),
         r'\1(static_cast<unsigned char*>(\2))'),
        # (uintptr_t) buf
        (re.compile(r'(\(\s*(?:const\s+)?(?:uintptr_t|size_t)\s*\))\s*(' + alt + r')\b'),
         r'\1static_cast<unsigned char*>(\2)'),
        # reinterpret_cast<T*>(buf)
        (re.compile(r'(reinterpret_cast\s*<[^<>()]*>)\s*\(\s*(' + alt + r')\s*\)'),
         r'\1(static_cast<unsigned char*>(\2))'),
        # (T*)(buf) and (T*)(buf + k)
        (re.compile(r'(\(\s*(?:const\s+)?[A-Za-z_][\w\s]*\*+\s*\))\s*\(\s*(' + alt + r')(\s*[+\-][^()]*)?\s*\)'),
         r'\1(static_cast<unsigned char*>(\2)\3)'),
        # ((T*)buf)->...
        (re.compile(r'(\(\s*\(\s*(?:const\s+)?[A-Za-z_][\w\s]*\*\s*\))\s*(' + alt + r')\s*\)'),
         r'\1static_cast<unsigned char*>(\2))'),
    ]

    total = 0
    for pat, repl in patterns:
        for i, line in enumerate(lines):
            if 'nxfb_' not in line:
                continue
            new, n = pat.subn(repl, line)
            if n:
                lines[i] = new
                total += n

    print('cast sites routed through the conversion:', total)
    if args.check:
        return 0
    with open(args.path, 'w', encoding='utf-8', newline='') as fh:
        fh.write(eol.join(lines))
    return 0


if __name__ == '__main__':
    sys.exit(main())