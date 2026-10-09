# NxPhysicsJointDesc oracle-only proof

`NxPhysicsJointDescTests` is an oracle-only Phase 6 target: it reports two fixed
input descriptors (cases 0 and 3), then the complete descriptor state before
and after `setGlobalAnchor` and `setGlobalAxis`. The proof verifier hashes the
input records and all six oracle output records independently of candidate
output, and also pins the oracle DLL and fixture source.

A real Win32 Release oracle run produced input SHA-256
`77dc2aee3a82fae3b6f6a800ac25a617dfa78727c0858a74a8c1a46bb4766d45` and output
SHA-256 `32b7860b476e812eea9325ea475ef9e89b15b9fcef21eaa95f86c8e2d9569904`.
The oracle image is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
Fixture source hashes canonicalize CRLF to LF so the same source proof works in
the main checkout and managed worktrees. The source hash and retained
self-hashed proof are in
`oracle-only-baselines.json` and `oracle-only-proofs/phase6-joint-desc.json`.

Validation: fresh Phase 6 gate passed, including the oracle differential and
machine-readable proof verifier. Phase 7 remains independently checked with its
retained joint-support proof. Public headers were not changed.
