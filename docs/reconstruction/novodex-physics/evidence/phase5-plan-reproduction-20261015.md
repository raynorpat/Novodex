# Phase 5 plan reproduction

I reproduced Phase 5 in a detached checkout of committed `HEAD` (`d7b1f6a7`)
with a new Win32 Release build directory. The full gate passed:
`phase_gate=5 status=pass`, `coverage_assertions_evaluated=2740 floor=2740`,
and the `NxPhysicsObjectLayoutTests` oracle-only proof matched the pinned
fixture hash and digest `774d629d`.

I also ran the gate from the primary working copy without modifying its files.
Its `NxPhysicsObjectLayoutTests` candidate differential completed with zero
mismatches, but the gate stopped at the oracle-proof identity check because
the current working-copy fixture hash is
`603724e849442b2cde5b8273779bc39fe89762cae9fe0fb4dfcd56883751726b`; the
committed oracle-only manifest pins
`a2f83215f4a03fdd8a4a2f0bda280cfa10af9e74fde367a23a709c1f07a92bc4`. The
working copy is already marked modified and appears binary to Git. I left that
file unchanged. The proof verifier correctly refused to certify oracle data
produced by a different fixture source.

This run does not reproduce the historical Phase 5 behavior failure. The
current committed gate is green; the primary working copy's red result is a
fixture-source pin mismatch. The complete gate transcripts are in
[`raw/phase5-plan-reproduction-20261015/`](raw/phase5-plan-reproduction-20261015/).
