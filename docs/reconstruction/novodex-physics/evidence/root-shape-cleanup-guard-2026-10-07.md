# Root-shape cleanup guard mutation closure (2026-10-07)

The Phase 5 actor shape-mutation fixture reaches `phys_fn_000006` through root
shape cleanup. Its registered differential is
`NxPhysicsActorShapeMutationTests`.

The first mutant inverted the dynamic-body test. That was not an adequate probe:
with the fixture's no-FluidManager Scene, both cleanup branches reach the same
pruning removal behavior, and the transcript stayed identical. It did not close
the row.

A second mutation in an isolated `git archive` copy omitted the early return for
a null root, so cleanup continued into the static/dynamic removal path with a
null shape. The staged candidate loaded the mutated Physics DLL
(`sha256=510bf6e26469eb4a86fdc36c50466477c0fc42be30bd04cc191124304fd24ba9`)
and its paired Foundation DLL. The oracle exited 0; the candidate exited with
Windows access-violation status `-1073741819`. The runner captured 155 normalized
stdout differences and identical stderr, then failed the differential as
expected. The early identity lines prove the actual loaded pair before the
fixture begins.

This run exposed a harness gap: `nxReportPairIdentity` previously ran only at
the end of each target. A test process that crashed could not report its loaded
DLLs, and the runner rejected it before classifying its exit and transcript
mismatch. `nxOpenPair` now reports and flushes the loaded module paths and hashes
immediately after loading and auditing the pair. The final identity report and
audit remain in place to catch modules loaded later in the target lifecycle.

The restored candidate baseline was rebuilt and compared against the pinned
oracle before the mutation run: both exited 0, `stdout_delta=0`, and stderr was
identical. The mutation transcript is in the ignored worktree build directory at
`build/phase5-root-mutation-000006-null-guard-mutant-differential.log`; the clean
baseline is recorded in `build/phase5-root-cleanup-gate.log`.

This closes the row-specific null guard behavior exercised by the fixture. It
does not claim every static/dynamic removal branch or every body/shape lifetime
case has been reconstructed.
