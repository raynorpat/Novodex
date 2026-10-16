# Phase 7 `phys_fn_003616`: fluid emitter local-pose setter

IDA decompilation at RVA `0x00089850` shows a 12-word copy from the supplied
local pose into the emitter record at `+0x18`, followed by the world-pose
refresh at `phys_fn_003605`. The candidate now expresses that boundary as
`nxNpFluidEmitterSetLocalPoseInternal` in `Physics/src/NpFluidEmitter.cpp`, and
`NpFluidEmitter::setLocalPose` delegates its copy and refresh to that helper.

`NxPhysicsFluidEmitterAbiTests` calls the oracle row and the map-resolved
candidate helper with an identity transform and a nontrivial transform. It
checks callback records, all 24 local and cached world-pose words, preserved
tail bytes, return values, and x86 stack balance. A mutation changing the copy
destination from `internal+0x18` to `internal+0x1c` is rejected in both cases
(`callback_checks=0`, `mismatches=2`, candidate exit 1). Restoring the offset
returns the test to `callback_checks=2`, `stack_checks=2`, `mismatches=0` and
candidate exit 0. The raw mutant and restored transcripts, DLL identities, and
fixture-source hash are recorded under
`evidence/raw/phase7-fluid-local-pose-003616-20261016/`.

This closes the 54-byte local-pose setter row only. Neighboring local-position
and local-orientation setters remain separate open rows. Public Physics
headers are unchanged.
