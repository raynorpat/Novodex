# Direct-tail calling-convention cleanup proof — 2026-10-15

The cleanup audit previously left the cast site for `NxSceneAuxDestroyOracle`
at RVA `0x13000` unresolved. IDA's bounded row shows the function ends in the
five bytes `e9 45 e6 ff ff`, a direct tail jump to RVA `0x116b0`
(`phys_fn_000624`). The target row is 127 bytes and ends in a bare `ret`, so it
cleans zero stack bytes, matching the `void (__thiscall *)(void*)` cast.

`audit_call_conventions.py` now follows this transfer only when both the exact
tail bytes and destination RVA match its allowlist. It decodes cleanup from the
target's exact inventory-bounded row; changing the displacement is rejected.
The audit fails if any cast site is unresolved or mismatched. This resolves
278/278 sites with zero mismatches and zero undecided rows.

Regression tests pass (16 tests), including the bounded-tail mutation and
fail-closed unresolved-row cases. The complete reconstruction-tool suite passes
(835 tests, 743 subtests). The Phase 5 runner invokes the audit against the
pinned oracle binary on every phase; a fresh Phase 5 gate passed at
2,790/2,790. The runner log is retained locally as
`build/phase5-call-convention-gate-20261015.log`.

This closes the static stack-cleanup audit. It does not close M1's runtime
nonvolatile-register, stack-balance, structure-return, or broader adapter
coverage requirements.
