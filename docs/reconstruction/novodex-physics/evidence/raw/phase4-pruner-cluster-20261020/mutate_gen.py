"""Row-aimed mutation sweep over a naked transcription file.

usage: python mutate_gen.py <source file> <test target> <exe flag> <family name> <count> <seed> <log> [<addr-lo> <addr-hi>]
Picks <count> eligible instruction lines (evenly spaced), mutates each one (a jump inverted, an offset or immediate moved,
an x87 operation swapped), rebuilds <test target>, runs it with <exe flag>, restores the file, and logs the result
('mismatches N', or crash/timeout/build-failure).
"""
import subprocess, re, sys, random

REPO = r'D:\github\Novodex'
src, target, flag, family, count, seed, log = sys.argv[1:8]
count = int(count)
lo = int(sys.argv[8], 16) if len(sys.argv) > 8 else 0
hi = int(sys.argv[9], 16) if len(sys.argv) > 9 else 1 << 32
INV = {'je': 'jne', 'jne': 'je', 'jb': 'jae', 'jae': 'jb', 'jbe': 'ja', 'ja': 'jbe', 'jl': 'jge', 'jge': 'jl', 'jle': 'jg',
       'jg': 'jle', 'js': 'jns', 'jns': 'js', 'jp': 'jnp', 'jnp': 'jp'}
SWAP = {'fadd': 'fsub', 'fsub': 'fadd', 'fmul': 'fadd', 'fdiv': 'fmul', 'faddp': 'fsubp', 'fsubp': 'faddp', 'fmulp': 'faddp'}


def mutate(text):
    m = re.match(r'(\w+)\s*(.*)$', text)
    if not m:
        return None
    op, rest = m.group(1), m.group(2)
    if op in INV:
        return '%s %s' % (INV[op], rest)
    mo = re.search(r'\[(\w+) \+ 0x([0-9a-f]+)\]', rest)
    if mo:
        return text.replace(mo.group(0), '[%s + 0x%x]' % (mo.group(1), int(mo.group(2), 16) + 4), 1)
    mo = re.search(r'\[(\w+)\]', rest)
    if mo and mo.group(1) in ('eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'ebp'):
        return text.replace(mo.group(0), '[%s + 4]' % mo.group(1), 1)
    if op in ('shl', 'shr', 'sar') and re.search(r', \d+$', rest):
        return re.sub(r', (\d+)$', lambda x: ', %d' % (int(x.group(1)) + 1), text)
    if op in ('cmp', 'add', 'sub', 'mov', 'push', 'and', 'test', 'or') and re.search(r'(?:, |^)(0x[0-9a-f]+|\d+)$', rest):
        return re.sub(r'(0x[0-9a-f]+|\d+)$', lambda x: ('0x%x' % (int(x.group(1), 16) + 1)) if x.group(1).startswith('0x') else str(int(x.group(1)) + 1), text)
    if op in SWAP:
        return text.replace(op, SWAP[op], 1)
    return None


orig = open(src, encoding='utf-8', newline='').read()
nl = '\r\n' if '\r\n' in orig else '\n'
cands = []
for m in re.finditer(r'^\t\t([^\r\n/]*?)\t\t// 0x([0-9a-f]{8})\r?$', orig, re.M):
    text, addr = m.group(1).strip(), int(m.group(2), 16)
    if not (lo <= addr < hi):
        continue
    new = mutate(text)
    if new and new != text:
        cands.append((addr, text, new))
random.seed(int(seed))
if len(cands) > count:
    step = len(cands) / float(count)
    cands = [cands[int(i * step)] for i in range(count)]
out = open(log, 'w')
for addr, text, new in cands:
    rx = re.compile(r'^(\t\t)[^\r\n]*?(\t\t// 0x%08x)(\r?)$' % addr, re.M)
    if len(rx.findall(orig)) != 1:
        continue
    mut = rx.sub(lambda m: m.group(1) + new + m.group(2) + m.group(3), orig)
    open(src, 'w', encoding='utf-8', newline='').write(mut)
    try:
        r = subprocess.run('cmake --build build --config Release --target ' + target, shell=True, capture_output=True, text=True, cwd=REPO)
        if ': error' in r.stdout or 'LNK2' in r.stdout:
            res = 'BUILD FAIL'
        else:
            try:
                r = subprocess.run([REPO + '/build/Release/%s.exe' % target, 'D:/FlamingEnt__/Unreal_3/Binaries',
                                    '4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c', flag],
                                   capture_output=True, text=True, cwd=REPO, timeout=120)
                mm = re.search(r'name=%s .*? mismatches=(\d+)' % family, r.stdout)
                res = 'exit %d mismatches %s' % (r.returncode, mm.group(1) if mm else '?')
            except subprocess.TimeoutExpired:
                res = 'TIMEOUT'
    finally:
        open(src, 'w', encoding='utf-8', newline='').write(orig)
    out.write('0x%08x  %s  ->  %s  :  %s\n' % (addr, text, new, res))
    out.flush()
out.write('DONE\n')
out.close()
