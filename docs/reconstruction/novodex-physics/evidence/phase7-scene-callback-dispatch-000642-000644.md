# Scene callback dispatcher continuations (`phys_fn_000642`, `phys_fn_000644`)

Date: 2026-10-15
Gate: Phase 7 `NxPhysicsSimulationTests` staged-pair differential
Source: `Physics/src/Scene.cpp`, `NxSceneInternal::processSimulationCallbacks`

IDA identifies both census spans as continuations of the method beginning at
oracle RVA `0x00012a90`. `phys_fn_000642` traverses 12-byte trigger records,
calls `NxUserTriggerReport::onTrigger` with the two recorded shapes and flags,
conditionally resets the trigger queue end pointer, and drains queued
joint-break events. `phys_fn_000644` traverses 44-byte contact records, forwards
the pair and event word to `NxUserContactReport::onContactNotify`, and
conditionally resets the contact queue end pointer.

The registered simulation fixture now asserts listener identity, event values,
callback counts, and that each queue is reset after dispatch. It retains the
real public trigger enter/stay/leave and joint-break scenarios. The restored
oracle/candidate simulation target exits 0 on both sides with `stdout_delta=0`
and exact stderr; the new fixture line is identical on both sides:

```text
simulation fetch-callback summary ready=1 fetched=1 trigger_get=1 trigger_calls=1 trigger_cleared=1 contact_get=1 contact_calls=1 contact_cleared=1 exact=1
```

Candidate-only source mutations were rejected as follows:

- `phys_fn_000642`: suppressing trigger callback delivery gives
  `stdout_delta=4072`; suppressing joint-break queue draining gives
  `stdout_delta=842`; retaining the trigger end pointer gives
  `stdout_delta=4074`.
- `phys_fn_000644`: suppressing contact callback delivery gives
  `stdout_delta=610`; retaining the contact end pointer gives
  `stdout_delta=587`.

Each mutant exits nonzero on the candidate while the oracle exits 0. The source
was restored from a byte-for-byte backup; its tracked diff is empty. The
restored candidate DLL SHA-256 is
`7c43b72a1d36f07420acf97e3a9af29eaa47475093cab0213349ba1cbcdbef24`, and the
restored control matches the oracle. Fresh Phase 5/6/7 gates pass at
`2,786/2,786`, `1,687/1,687`, and `2,903/2,903`. The reconstruction tooling
suite passes all 816 tests, the inventory validator reports zero unexplained
records, and all 80 public Physics headers match their pinned hashes. The
candidate map resolves the shared implementation to
`?processSimulationCallbacks@NxSceneInternal@@QAEXXZ` at `0x1006cd70` in
`Scene.obj`. Raw mutation, restored-pair, and gate transcripts with hashes are
under `evidence/raw/phase7-scene-callback-dispatch-000642-000644-20261015/`.

This closes only the trigger/contact dispatch continuation rows. It does not
close the remaining simulation pipeline or the full DLL census. Public SDK
headers and production callback behavior are unchanged.
