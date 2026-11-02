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
import os
import re
import subprocess
import sys
import time
from pathlib import Path

REPO = Path(__file__).resolve().parents[4]
ORACLE_DIR = os.environ.get('NX_ORACLE_DIR', 'D:/FlamingEnt__/Unreal_3/Binaries')
ORACLE_SHA = '4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c'
TIMEOUT = 120
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
    # sub/add esp only unbalance the stack: a crash, which says nothing about the row's data flow
    if op in ('sub', 'add') and rest.startswith('esp,'):
        return None
    mo = re.search(r'\[(\w+) \+ 0x([0-9a-f]+)\]', rest)
    if mo:
        return text.replace(mo.group(0), '[%s + 0x%x]' % (mo.group(1), int(mo.group(2), 16) + 4), 1)
    mo = re.search(r'\[(\w+) \+ (\d+)\]', rest)
    if mo:
        return text.replace(mo.group(0), '[%s + %d]' % (mo.group(1), int(mo.group(2)) + 4), 1)
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


def pick(found, limit):
    """At most `limit` of the candidates, evenly spaced, so the attempts cover the row rather than its first lines."""
    if len(found) <= limit:
        return list(found)
    step = len(found) / float(limit)
    return [found[int(i * step)] for i in range(limit)]


def detect_family(stdout, family):
    """N from `name=<family> ... mismatches=N` (N > 0), else None."""
    for line in stdout.splitlines():
        if re.search(r'\bname=%s\b' % re.escape(family), line):
            m = re.search(r'\bmismatches=(\d+)', line)
            if m and int(m.group(1)) > 0:
                return int(m.group(1))
    return None


def detect_harness(stdout, returncode, family, count_crash=False):
    """N for a harness run: the family's `mismatches=N`, or 1 for a crash that stopped the run before it printed the line.

    A mutant that faults inside the row kills the process before the family's summary, so without count_crash it would be
    scored as a survivor. A launch failure (exit 127, no output) is not a crash, so only a fault-range exit code counts.
    """
    n = detect_family(stdout, family)
    if n:
        return n
    if count_crash and not re.search(r'\bname=%s\b' % re.escape(family), stdout) and (returncode < 0 or returncode >= CRASH):
        return 1
    return None


def detect_staged(output, count_crash=False):
    """N from `stdout_delta=N` (N > 0) when the candidate did not crash, else None.

    count_crash also accepts a crashed candidate whose output differs from the oracle's: for a row that only walks
    structures, a data-offset mutant that faults is the gate failing, not noise.
    """
    m = re.search(r'candidate_exit=(-?\d+).*?stdout_delta=(\d+)', output)
    if not m:
        return None
    code = int(m.group(1))
    if (code < 0 or code >= CRASH) and not count_crash:
        return None
    return int(m.group(2)) or None


def staged_note(output):
    """Why a staged run detected nothing: a crash of the candidate, or an output identical to the oracle's."""
    m = re.search(r'candidate_exit=(-?\d+).*?stdout_delta=(\d+)', output)
    if not m:
        return 'no-summary'
    code, delta = int(m.group(1)), int(m.group(2))
    return '%s(exit=%d,delta=%d)' % ('crash' if code < 0 or code >= CRASH else 'same', code, delta)


def _swap_line(text, addr, new):
    rx = re.compile(r'^(\t\t)[^\r\n]*?(\t\t// 0x%08x)(\r?)$' % addr, re.M)
    if len(rx.findall(text)) != 1:
        return None
    return rx.sub(lambda m: m.group(1) + new + m.group(2) + m.group(3), text)


def _write_tmp(tmp, data):
    tmp.write_bytes(data)


def write_atomic(path, data, writer=_write_tmp, pause=0.3):
    """Replace `path` whole or not at all: write a sibling temp file, then os.replace it.

    A full disk once truncated a source file mid-restore; with this the file is either the old bytes or the new ones.
    """
    path = Path(path)
    tmp = path.with_name(path.name + '.mutate-tmp')
    try:
        writer(tmp, data)
        for attempt in range(10):
            try:
                os.replace(str(tmp), str(path))
                break
            except PermissionError:
                # Windows briefly denies the replace while a scanner or indexer holds the file
                if attempt == 9:
                    raise
                time.sleep(pause)
    except BaseException:
        try:
            tmp.unlink()
        except OSError:
            pass
        raise


def sweep(path, lo, hi, run, limit, restore_build=None, trace=None, control=None):
    """Mutate the row's lines one at a time until `run` reports a detection.

    run(mutated_text) -> (gate kind, detection N or None). Returns (mutation text, gate kind, N) or None. The file is
    restored after every attempt, and a RuntimeError is raised if its bytes then differ from the original.
    restore_build() runs once at the end, however the sweep ends, so the built binaries match the restored source.
    control(original_text) runs the gate on the unmutated source first; a detection there means the baseline already
    differs, and every mutant would be "caught", so the sweep refuses to start.
    """
    path = Path(path)
    original = path.read_bytes()
    text = original.decode('utf-8')
    attempts = 0
    result = None
    try:
        if control is not None:
            kind, n = control(text)
            if n:
                raise RuntimeError('the unmutated source already reports %s=%s; the baseline differs' % (kind, n))
        for addr, before, after in pick(candidates(text, lo, hi), limit):
            mutated = _swap_line(text, addr, after)
            if mutated is None:
                continue
            attempts += 1
            write_atomic(path, mutated.encode('utf-8'))
            try:
                kind, n = run(mutated)
            finally:
                write_atomic(path, original)
            if trace:
                trace(addr, before, after, kind, n)
            if n:
                result = ('0x%08x  %s  ->  %s' % (addr, before, after), kind, n)
                break
    finally:
        write_atomic(path, original)
        if restore_build:
            restore_build()
    if path.read_bytes() != original:
        raise RuntimeError('%s was not restored' % path)
    return result


def survey(path, lo, hi, run, limit, skip=0, restore_build=None, trace=None):
    """Try `limit` mutations spread over the row (after dropping its first `skip` candidates) and keep going past a catch.

    Returns [(mutation text, detection N or None)] in order: a None is a mutant the gate did not notice. This measures
    how much of the row's body the gate sees; `sweep` only needs one catch and so tends to stop on the row's first line.
    """
    path = Path(path)
    original = path.read_bytes()
    text = original.decode('utf-8')
    out = []
    try:
        for addr, before, after in pick(candidates(text, lo, hi)[skip:], limit):
            mutated = _swap_line(text, addr, after)
            if mutated is None:
                continue
            write_atomic(path, mutated.encode('utf-8'))
            try:
                kind, n = run(mutated)
            finally:
                write_atomic(path, original)
            if trace:
                trace(addr, before, after, kind, n)
            out.append(('0x%08x  %s  ->  %s' % (addr, before, after), n or None))
    finally:
        write_atomic(path, original)
        if restore_build:
            restore_build()
    if path.read_bytes() != original:
        raise RuntimeError('%s was not restored' % path)
    return out


def build_targets(target, staged):
    """What a run must rebuild: a staged pair stages build/Release/NxPhysics.dll, a harness links the sources itself."""
    return ['NxPhysics', target] if staged else [target]


def run_tree(cmd, timeout, cwd):
    """subprocess.run that, on a timeout, kills the whole process tree (a hung mutant test otherwise keeps the DLLs locked)."""
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, cwd=cwd)
    try:
        out, err = proc.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        subprocess.run(['taskkill', '/F', '/T', '/PID', str(proc.pid)], capture_output=True)
        out, err = proc.communicate()
        raise subprocess.TimeoutExpired(cmd, timeout, output=out, stderr=err)
    return subprocess.CompletedProcess(cmd, proc.returncode, out, err)


def remove_stale_dll(root):
    """Delete the built candidate DLL: MSBuild's tracker can call the link up to date after an object changed."""
    try:
        (Path(root) / 'build' / 'Release' / 'NxPhysics.dll').unlink()
    except FileNotFoundError:
        pass


def _build(target, staged):
    """Build what a run needs; False when the build failed."""
    if staged:
        remove_stale_dll(REPO)
    for t in build_targets(target, staged):
        build = subprocess.run('cmake --build build --config Release --target %s' % t, shell=True,
                               capture_output=True, text=True, cwd=REPO)
        if build.returncode != 0 or ': error' in build.stdout or 'LNK2' in build.stdout:
            return False
    return True


def harness_command(target, extra_args):
    """The command line of an oracle-differential harness, with any extra arguments after the oracle directory and hash."""
    return [str(REPO / 'build' / 'Release' / ('%s.exe' % target)), ORACLE_DIR, ORACLE_SHA] + list(extra_args or [])


def _real_run(target, family, staged, count_crash=False, extra_args=None):
    def run(_mutated_text):
        if not _build(target, staged):
            return ('build-failed', None)
        try:
            if staged:
                r = run_tree(['powershell', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
                              'docs/reconstruction/novodex-physics/tools/run_differential.ps1', '-Targets', target],
                             timeout=TIMEOUT, cwd=REPO)
                n = detect_staged(r.stdout, count_crash)
                return ('stdout_delta' if n else 'stdout_delta[%s]' % staged_note(r.stdout), n)
            r = run_tree(harness_command(target, extra_args), timeout=TIMEOUT, cwd=REPO)
            return ('mismatches', detect_harness(r.stdout, r.returncode, family, count_crash))
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
    ap.add_argument('--exe-arg', action='append', default=[], help='an extra argument for a harness target (repeatable)')
    ap.add_argument('--no-control', action='store_true', help='skip the unmutated control run')
    ap.add_argument('--count-crash', action='store_true', help='a faulting candidate with differing output counts as a detection')
    ap.add_argument('--survey', action='store_true', help='try --limit mutations across the row, past any catch, and list the survivors')
    ap.add_argument('--skip', type=int, default=0, help='with --survey: drop the first N candidates (the row head)')
    ap.add_argument('--log')
    ap.add_argument('--verbose', action='store_true', help='print every attempt and what the gate reported')
    ap.add_argument('--inventory', default=str(Path(__file__).resolve().parents[1] / 'inventory.json'))
    a = ap.parse_args(argv)
    inv = json.loads(Path(a.inventory).read_text(encoding='utf-8'))
    row = next(r for r in inv['functions'] if r['id'] == a.row or r['id'].endswith(a.row))
    lo = int(row['rva'], 16)
    hi = lo + int(row['size'])
    tried = []
    if a.survey:
        run = _real_run(a.target, a.family, a.staged, a.count_crash, a.exe_arg)
        results = survey(a.source, lo, hi, run, a.limit, skip=a.skip, restore_build=lambda: _build(a.target, a.staged))
        caught = sum(1 for _, n in results if n)
        lines = ['SURVEY %s %s tried=%d caught=%d survived=%d' % (row['id'], a.target, len(results), caught, len(results) - caught)]
        lines += ['  %s %s' % ('caught  ' if n else 'SURVIVED', m) for m, n in results]
        print('\n'.join(lines))
        if a.log:
            with open(a.log, 'a') as f:
                f.write('\n'.join(lines) + '\n')
        return 0
    run = _real_run(a.target, a.family, a.staged, a.count_crash, a.exe_arg)
    got = sweep(a.source, lo, hi, run, a.limit, control=None if a.no_control else run,
                restore_build=lambda: _build(a.target, a.staged),
                trace=lambda addr, before, after, kind, n: tried.append('TRY 0x%08x  %s  ->  %s  :  %s %s' % (addr, before, after, kind, n)))
    if got:
        text, kind, n = got
        line = 'CAUGHT %s %s %s=%d %s' % (row['id'], a.target, kind, n, text)
    else:
        line = 'NOT CAUGHT %s' % row['id']
    print(line)
    if a.verbose:
        for t in tried:
            print(t)
    if a.log:
        with open(a.log, 'a') as f:
            f.write(line + '\n')
    return 0 if got else 1


if __name__ == '__main__':
    sys.exit(main())
