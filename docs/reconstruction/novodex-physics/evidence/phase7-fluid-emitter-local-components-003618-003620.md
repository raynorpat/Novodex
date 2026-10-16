# Phase 7 `phys_fn_003618` / `003620`: local pose component setters

IDA decompilation at RVA `0x00089890` shows `phys_fn_003618` copying the requested three-float position to the local pose translation at `internal+0x3c`, then refreshing the world pose through `phys_fn_003611`. At RVA `0x000898b0`, `phys_fn_003620` copies the requested 3x3 orientation (36 bytes) to `internal+0x18` and makes the same refresh call.

The existing `NpFluidEmitter::setLocalPosition` and `setLocalOrientation` implementations match these source-level operations. The registered `NxPhysicsFluidEmitterAbiTests` fixture now invokes both public setters on the oracle, the linked candidate, and a wrapper constructed by the candidate DLL. It uses nontrivial local transforms and checks the resulting local pose, `EmitterSetGlobalPose` callback, and x86 stack balance. The fixture also retains the two world-component cases, for four total.

Moving only the local-position destination from `internal+0x3c` to `+0x40` is rejected in case 2 (`callback_checks=3`, `stack_checks=4`, `mismatches=1`). Moving only the local-orientation destination from `internal+0x18` to `+0x1c` is rejected in case 3 with the same mismatch counts. The restored control reports `cases=4 callback_checks=4 stack_checks=4 mismatches=0`. Raw logs and file hashes are under `evidence/raw/phase7-fluid-local-components-003618-003620-20261016/`.

These rows cover unattached emitters. Public Physics headers are unchanged.
