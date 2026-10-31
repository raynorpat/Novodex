#!/usr/bin/env python3
"""Row-aimed mutation sweep over a naked transcription file.

usage: python mutate_rows.py --source <file> --row <phys_fn_id> --target <test target> (--family <name> | --staged)
                             [--limit 6] [--log <file>] [--inventory inventory.json]

Promoted from the pruner bundles' mutate_gen.py. For one row it mutates one instruction line at a time inside the row's
own range, rebuilds the target, runs it against the oracle, and restores the file. A mutation counts only when the gate
detects it: `mismatches=N` (N > 0) from the named family, or `stdout_delta=N` (N > 0) from a staged-pair target whose
candidate did not crash. A crash, timeout or build failure is logged and not counted. It stops at the first caught
mutation (default limit 6 attempts). Exit code 0 when caught, 1 when not.
"""
import argparse
import json
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[4]
ORACLE_DIR = 'D:/FlamingEnt__/Unreal_3/Binaries'
ORACLE_SHA = '4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c'
TIMEOUT = 300
CRASH = 0xC0000000

INV = {'je': 'jne', 'jne': 'je', 'jb': 'jae', 'jae': 'jb', 'jbe': 'ja', 'ja': 'jbe', 'jl': 'jge', 'jge': 'jl',
       'jle': 'jg', 'jg': 'jle', 'js': 'jns', 'jns': 'js', 'jp': 'jnp', 'jnp': 'jp'}
SWAP = {'fadd': 'fsub', 'fsub': 'fadd', 'fmul': 'fadd', 'fdiv': 'fmul', 'faddp': 'fsubp', 'fsubp': 'faddp',
        'fmulp': 'faddp'}
REGS = ('eax', 'ebx', 'ecx', 'edx', 'esi', 'edi', 'ebp')


def mutate(text):
    """One mutation of an instruction's text, or None when the instruction has no mutable part."""
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
    if mo and mo.group(1) in REGS:
        return text.replace(mo.group(0), '[%s + 4]' % mo.group(1), 1)
    if op in ('shl', 'shr', 'sar') and re.search(r', \d+$', rest):
        return re.sub(r', (\d+)$', lambda x: ', %d' % (int(x.group(1)) + 1), text)
    if op in ('cmp', 'add', 'sub', 'mov', 'push', 'and', 'test', 'or') and re.search(r'(?:, |^)(0x[0-9a-f]+|\d+)$', rest):
        return re.sub(r'(0x[0-9a-f]+|\d+)$',
                      lambda x: ('0x%x' % (int(x.group(1), 16) + 1)) if x.group(1).startswith('0x')
                      else str(int(x.group(1)) + 1), text)
    if op in SWAP:
        return text.replace(op, SWAP[op], 1)
    return None


def candidates(text, lo, hi):
    """(address, before, after) of each mutable instruction line whose `// 0x%08x` comment is in [lo, hi)."""
    found = []
    for m in re.finditer(r'^\t\t([^\r\n/]*?)\t\t// 0x([0-9a-f]{8})\r?$', text, re.M):
        before, addr = m.group(1).strip(), int(m.group(2), 16)
        if lo <= addr < hi:
            after = mutate(before)
            if after and after != before:
                found.append((addr, before, after))
    return found


def detect_family(stdout, family):
    """N from `name=<family> ... mismatches=N` (N > 0), else None."""
    for line in stdout.splitlines():
        if re.search(r'\bname=%s\b' % re.escape(family), line):
            m = re.search(r'\bmismatches=(\d+)', line)
            if m and int(m.group(1)) > 0:
                return int(m.group(1))
    return None


def detect_staged(output):
    """N from `stdout_delta=N` (N > 0) when the candidate did not crash, else None."""
    m = re.search(r'candidate_exit=(-?\d+).*?stdout_delta=(\d+)', output)
    if not m:
        return None
    code = int(m.group(1))
    if code < 0 or code >= CRASH:
        return None
    return int(m.group(2)) or None


def _swap_line(text, addr, new):
    rx = re.compile(r'^(\t\t)[^\r\n]*?(\t\t// 0x%08x)(\r?)$' % addr, re.M)
    if len(rx.findall(text)) != 1:
        return None
    return rx.sub(lambda m: m.group(1) + new + m.group(2) + m.group(3), text)


def sweep(path, lo, hi, run, limit):
    """Mutate the row's lines one at a time until `run` reports a detection.

    run(mutated_text) -> (gate kind, detection N or None). Returns (mutation text, gate kind, N) or None. The file is
    restored after every attempt, and a RuntimeError is raised if its bytes then differ from the original.
    """
    path = Path(path)
    original = path.read_bytes()
    text = original.decode('utf-8')
    attempts = 0
    try:
        for addr, before, after in candidates(text, lo, hi):
            if attempts >= limit:
                break
            mutated = _swap_line(text, addr, after)
            if mutated is None:
                continue
            attempts += 1
            path.write_bytes(mutated.encode('utf-8'))
            try:
                kind, n = run(mutated)
            finally:
                path.write_bytes(original)
            if n:
                return ('0x%08x  %s  ->  %s' % (addr, before, after), kind, n)
    finally:
        path.write_bytes(original)
    if path.read_bytes() != original:
        raise RuntimeError('%s was not restored' % path)
    return None


def build_targets(target, staged):
    """What a run must rebuild: a staged pair stages build/Release/NxPhysics.dll, a harness links the sources itself."""
    return ['NxPhysics', target] if staged else [target]


def _real_run(target, family, staged):
    def run(_mutated_text):
        for t in build_targets(target, staged):
            build = subprocess.run('cmake --build build --config Release --target %s' % t, shell=True,
                                   capture_output=True, text=True, cwd=REPO)
            if ': error' in build.stdout or 'LNK2' in build.stdout:
                return ('build-failed', None)
        try:
            if staged:
                r = subprocess.run(['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                                    'docs/reconstruction/novodex-physics/tools/run_differential.ps1', '-Targets', target],
                                   capture_output=True, text=True, cwd=REPO, timeout=TIMEOUT)
                return ('stdout_delta', detect_staged(r.stdout))
            r = subprocess.run([str(REPO / 'build' / 'Release' / ('%s.exe' % target)), ORACLE_DIR, ORACLE_SHA],
                               capture_output=True, text=True, cwd=REPO, timeout=TIMEOUT)
            return ('mismatches', detect_family(r.stdout, family))
        except subprocess.TimeoutExpired:
            return ('timeout', None)
    return run


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--source', required=True)
    ap.add_argument('--row', required=True)
    ap.add_argument('--target', required=True)
    g = ap.add_mutually_exclusive_group(required=True)
    g.add_argument('--family')
    g.add_argument('--staged', action='store_true')
    ap.add_argument('--limit', type=int, default=6)
    ap.add_argument('--log')
    ap.add_argument('--inventory', default=str(Path(__file__).resolve().parents[1] / 'inventory.json'))
    a = ap.parse_args(argv)
    inv = json.loads(Path(a.inventory).read_text(encoding='utf-8'))
    row = next(r for r in inv['functions'] if r['id'] == a.row or r['id'].endswith(a.row))
    lo = int(row['rva'], 16)
    hi = lo + int(row['size'])
    got = sweep(a.source, lo, hi, _real_run(a.target, a.family, a.staged), a.limit)
    if got:
        text, kind, n = got
        line = 'CAUGHT %s %s %s=%d %s' % (row['id'], a.target, kind, n, text)
    else:
        line = 'NOT CAUGHT %s' % row['id']
    print(line)
    if a.log:
        with open(a.log, 'a') as f:
            f.write(line + '\n')
    return 0 if got else 1


if __name__ == '__main__':
    sys.exit(main())
