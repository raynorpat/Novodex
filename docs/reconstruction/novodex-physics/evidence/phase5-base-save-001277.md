# Phase 5: base shape save-to-descriptor (`phys_fn_001277`)

The base `NxShape` save row copies common shape state into the descriptor after the shape-specific save fields are written. The registered `NxPhysicsObjectLayoutTests` fixture constructs a sphere, invokes the real base save row through the save path, and compares the full poisoned 0x48-byte record against the pinned oracle.

- Baseline oracle: `saved=1`, digest `bc9dc964`; candidate: `ok=1`, digest `bc9dc964`; layout mismatches: `0`.
- Mutation: replace `unsigned int de = mHalfwordDE;` with `unsigned int de = 0;`. The oracle still saves successfully, while the candidate record digest becomes `6e8a0c5c`; the target reports `mismatches=6` and exits 1.
- Restored control: candidate digest `bc9dc964`, layout mismatches `0`, `layout result=differential-pass`.

The existing fixture reaches the saved word with its default value `8`; this proof establishes sensitivity to that serialized field, while additional non-default base flags remain useful coverage. Build and run logs: `build/phase5-base-save-mutant.log`, `build/phase5-base-save-restored.log`.
