# M1 oracle proof and staged-pair path regression — 2026-10-09

Source base: `94b251223fc774256224e4cba70078116e208c79` (`main`), executed in
an isolated worktree. The pinned oracle is
`D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`, SHA-256
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

## BOX sweep input proof

`NxPhysicsShapeVtableTests` now hashes the actual 84-call BOX sweep input
sequence: the four loop indices, rotation matrix, translation, dimensions,
direction, and initial output word for each call. It reports input digest
`46df40a9`; the oracle result digest remains `2c5d5c09`. The checked-in
baseline also pins the normalized fixture-source SHA-256. The gate requires the
input line and invokes `oracle_proof.py`, which checks the oracle DLL hash, case
count, input digest, output digest, and fixture hash while leaving candidate
values to the existing candidate/oracle differential.

The retained proof is
`oracle-only-proofs/phase5-shape-vtable-boxsweep.json`. The three existing
joint proofs were regenerated from fresh oracle runs against the updated
baseline manifest and retain the same case and digest values.

## Candidate pair path regression

A fresh Phase 5 gate using build root
`D:\Novodex-build-m1-oracle-proof` exposed a candidate access violation in
`NxPhysicsBodyCreationTests` (`candidate_exit=-1073741819`). The test chose its
oracle RVA whenever `wcsstr(pairDirectory, L"oracle")` matched, so the
candidate directory `...\m1-oracle-proof\pairs\candidate` incorrectly took
the oracle-RVA path before printing its first fixture result. This was a
harness path-selection defect, not a DLL behavior discrepancy.

The test now recognizes the pair by its final directory component and includes
a negative-control fixture for an `oracle`-named parent. Restoring the old
substring classifier makes that fixture fail (`mutant_exit=1`); with the fix,
the staged body-creation differential exits 0 on both sides with
`stdout_delta=0` and exact stderr. The path-selection line is required in both
Phases 3 and 5.

## Verification

- Fresh Phase 3 gate: `coverage_assertions_evaluated=544 floor=544`,
  `phase_gate=3 status=pass`.
- Fresh Phase 5 gate: `coverage_assertions_evaluated=2621 floor=2621`,
  `phase_gate=5 status=pass`.
- Fresh Phase 6 gate: `coverage_assertions_evaluated=1265 floor=1265`,
  `phase_gate=6 status=pass`.
- Fresh Phase 7 gate: `coverage_assertions_evaluated=1447 floor=1447`,
  `phase_gate=7 status=pass`.
- `python -B -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests`:
  810 tests passed.
- `git diff --check`: passed.

Gate logs are retained in the local build directory under
`D:\Novodex-build-m1-oracle-proof-phase{3,5-final,6,7}.log` and are not
required inputs. M1 remains open for additional oracle-only targets, broader
lifecycle/harness coverage, and remaining ABI audits; this packet does not
close any code row or complete the DLL reconstruction.
