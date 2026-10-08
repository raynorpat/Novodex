# Phase 5: base shape descriptor load (`phys_fn_001347`)

`NxPhysicsObjectLayoutTests` now has a row-targeted pinned-oracle fixture for the inherited base slot 1. It invokes the oracle entry at RVA `0x27740` and the candidate `ShapeBase::nxApplyDescriptor` on equivalent detached base objects. The crafted 0x58-byte descriptor has a non-default 12-word pose, flags `0x1234`, group `5`, material `0x5678`, userData sentinel, and null name. The check compares the full 0xe0-byte object with the five module-pointer words masked and checks the decoded fields and return.

- Oracle and candidate fixture digest: `11e5e856`; both return true and the target reports zero mismatches.
- Mutation: replace the `+0x38` flags load/store with zero. Candidate digest becomes `b866a954`, flags become `0000`, and the target reports `mismatches=1`.
- Restored control: candidate digest returns to `11e5e856`, flags remain `1234`, and the full differential passes with zero mismatches.
- The Phase 5 gate requires the exact oracle row, exact candidate row, and `applydesc=1` coverage marker; its floor is 2,561 and the verified run evaluated 2,562 assertions.

Logs: `build/phase5-applydesc-fixture-baseline.log`, `build/phase5-applydesc-mutant.log`, `build/phase5-applydesc-restored.log`, and `build/phase5-applydesc-full-gate-pinned.log`.
