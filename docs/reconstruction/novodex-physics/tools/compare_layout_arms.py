#!/usr/bin/env python
"""Compare HEAD against the frame conversion, using the recipe that reproduces.

Evidence: docs/reconstruction/novodex-physics/evidence/phase5-object-model.md
3z280-3z282. Three rounds of this programme were retracted for want of this
recipe, so every step in it is load-bearing:

  1. the base is the committed working-tree bytes, captured once and sha-pinned.
     `git cat-file` returns the same text with LF line endings and builds to a
     DIFFERENT program;
  2. each arm rebuilds the layout target with `--clean-first`. This tree's
     timestamps are not ordered, so MSBuild will otherwise run a previous arm's
     object and the arm reports a run it was supposed to replace;
  3. each arm runs three times. A single run is not evidence for a harness that
     has moved under this session more than once;
  4. a pass is exit 1 plus the gate's fold line -- NOT a marker string. This
     harness does not print `RED on purpose`, and a verdict test that looked for
     it scored every arm the same, crashing or not;
  5. the measured executable's hash is printed, so a reading names its artefact.

Arms: HEAD, the conversion with storage left on the stack (`--stack`, which
exercises the renaming and the sizeof rewriting alone), and the conversion with
storage moved to the heap (`--limit`). The working-tree source is restored in a
finally block, so a failed arm cannot leave the tree invalid.
"""

import hashlib
import io
import os
import re
import subprocess
import sys

ROOT = r'D:\github\Novodex'
SRC = ROOT + r'\tests\PhysicsObjectLayoutTests.cpp'
FL = ROOT + r'\docs\reconstruction\novodex-physics\tools\frame_locals.py'
FC = ROOT + r'\docs\reconstruction\novodex-physics\tools\frame_casts.py'
EXE = ROOT + r'\build\Release\NxPhysicsObjectLayoutTests.exe'
ORACLE = [r'D:\FlamingEnt__\Unreal_3\Binaries',
          '4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c']


def build():
    r = subprocess.run(['cmake', '--build', 'build', '--config', 'Release',
                        '--target', 'NxPhysicsObjectLayoutTests', '--clean-first'],
                       cwd=ROOT, capture_output=True, text=True)
    errs = [l for l in (r.stdout + r.stderr).splitlines()
            if 'error C' in l or 'error LNK' in l]
    return errs[0][:100] if errs else None


def run_once():
    p = subprocess.run([EXE] + ORACLE, capture_output=True)
    raw = p.stdout
    txt = raw.decode('utf-16' if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8', 'replace')
    lines = [l.rstrip() for l in txt.split('\n') if l.strip()]
    # The harness does not print a 'RED on purpose' marker in this build; its
    # end state is exit 1 plus the gate's own fold line. An earlier version of
    # this check looked for the marker, so every arm -- crashing or not --
    # scored the same, which is the defect this replaces.
    end = any('candidate_fold' in l or 'layout candidate mismatches' in l for l in lines)
    nz = [l for l in lines if 'failures=' in l and 'failures=0' not in l]
    return p.returncode, len(lines), end, (nz[0][:52] if nz else '-')


def arm(tag, args):
    r = subprocess.run([sys.executable, FL] + args, cwd=ROOT, capture_output=True, text=True)
    if r.returncode != 0:
        print('%-16s TOOL ERROR %s' % (tag, (r.stderr or '').strip().splitlines()[-1][:70]))
        return
    c = subprocess.run([sys.executable, FC], cwd=ROOT, capture_output=True, text=True)
    if c.returncode != 0:
        print('%-16s CAST ERROR %s' % (tag, (c.stderr or '').strip().splitlines()[-1][:70]))
        return
    if os.path.getsize(SRC) == 0:
        print('%-16s source empty after tooling' % tag)
        return
    err = build()
    if err:
        print('%-16s BUILD %s' % (tag, err))
        return
    exe_sha = hashlib.sha256(open(EXE, 'rb').read()).hexdigest()[:12]
    runs = [run_once() for _ in range(3)]
    ok = all(r[0] == 1 and r[2] for r in runs)
    print('%-16s exe=%s %s' % (tag, exe_sha, 'PASS' if ok else 'FAIL'))
    for i, r in enumerate(runs, 1):
        print('     run%d exit=%-12d lines=%-4d end=%-5s %s' % (i, r[0], r[1], r[2], r[3]))


def main():
    base = open(SRC, 'rb').read()
    print('canonical base: %d bytes sha %s' % (len(base), hashlib.sha256(base).hexdigest()[:12]))
    try:
        for tag, args in [
            ('HEAD', None),
            ('stack all', ['--stack', '--limit', '81']),
            ('heap all', ['--limit', '81']),
        ]:
            open(SRC, 'wb').write(base)
            if args is None:
                err = build()
                if err:
                    print('%-16s BUILD %s' % (tag, err))
                    continue
                exe_sha = hashlib.sha256(open(EXE, 'rb').read()).hexdigest()[:12]
                runs = [run_once() for _ in range(3)]
                ok = all(r[0] == 1 and r[2] for r in runs)
                print('%-16s exe=%s %s' % (tag, exe_sha, 'PASS' if ok else 'FAIL'))
                for i, r in enumerate(runs, 1):
                    print('     run%d exit=%-12d lines=%-4d end=%-5s %s' % (i, r[0], r[1], r[2], r[3]))
            else:
                arm(tag, args)
    finally:
        open(SRC, 'wb').write(base)
        print('restored the canonical base (%d bytes)' % len(base))


if __name__ == '__main__':
    main()