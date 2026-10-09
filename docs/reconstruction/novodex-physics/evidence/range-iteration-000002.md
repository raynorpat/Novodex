# Reverse strided range helper (`phys_fn_000002`)

IDA decompiles the pinned function at RVA `0x1030` as a 16-byte `__stdcall`
helper taking a first address, stride, count, and a `__thiscall` callback. It
computes `first + count * stride`, then decrements by `stride` before each
callback. For a positive count it returns the last callback result; otherwise
it returns `count - 1` without calling back.

The implementation is in `Physics/src/RecoveredRows.cpp`. The focused
`NxPhysicsRangeIterationTests` static proof covers a three-element reverse
walk, a negative stride, the callback return value, and the zero-count path.
This row has no public API edge in the current test harness, so the proof checks
the reconstructed helper directly rather than claiming an oracle differential.

Verification:

- The focused test passes in Release. The candidate linker map retains
  `_phys_fn_000002@16` from `RecoveredRows.obj` at preferred address
  `0x1004e050` (`NxPhysics.dll` RVA `0x0004e050`). The row now records the
  source path, symbol, and map reference in `inventory.json`; this is static
  implementation mapping, not an oracle runtime differential.
- The full Phase 5 gate passes with its new static proof and all
  `2,226/2,226` registered coverage assertions
  (`build/phase5-range-iteration-final.log`).
- Inventory validation passes with 6,338 function rows, 5,138 data rows, and
  zero unexplained rows. The inventory and work-unit tests pass (250 and 12
  tests respectively).
- The public Physics header manifest passes for all 80 files.

This closes only the helper's reconstructed row and tested argument cases; it
does not change the full-DLL acceptance criteria.

## Mutation follow-up (2026-10-07)

The focused test was rerun after changing the loop update from `address -=
stride` to `address += stride`. The mutant exited 1 and printed three failed
checks: descending visitation, final callback result, and negative stride. The
restored candidate passes `NxPhysicsRangeIterationTests`; this falsifies
`phys_fn_000002` under the disassembly-based static proof and does not claim an
oracle differential.

## Candidate map and current gate follow-up (2026-10-08)

The inventory now maps this row to `Physics/src/RecoveredRows.cpp` and records
the candidate symbol `_phys_fn_000002@16` at preferred address `0x1004e050` in
`build/Release/NxPhysics.map`. The completion report carries this static map
reference separately from dynamic proof. Regenerating `work_units.json` also
restored agreement between the checked-in ownership census and its generator.

The full reconstruction tooling suite passes (778 tests). Phase 5, 6, and 7
gates pass at their current registered coverage counts: 2,595, 1,094, and
1,416 respectively. Their phase completion statuses remain pending because
these counts validate the registered coverage, not full phase closure.
