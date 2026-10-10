# Phase 6 mutation evidence: `phys_fn_004141` — common joint constructor type map

`phys_fn_004141` (`Joint::Joint`, RVA `0x00099e60`) loads common descriptor state and maps the family type bit to the public `NxJointType`. The registered `NxPhysicsJointStagedPairTests` creates every supported family and checks `getType` and family dispatch.

The clean staged-pair differential passed with both exits 0, `stdout_delta=0`, and exact stderr. I temporarily changed only the `0x200` mapping from `NX_JOINT_FIXED` to `NX_JOINT_DISTANCE`. The registered differential caught the wrong type and downstream family checks with both exits 0, `stdout_delta=6006`, and exact stderr. The mutant candidate DLL SHA-256 was recorded in the retained transcript.

Restoring the source and rebuilding with `--clean-first` returned the differential to exact output (`stdout_delta=0`, exact stderr). Restored candidate DLL SHA-256: `40eff15c1448bd0d594020fcaa5fd0744cc89eae42c1bec90026b39659542efd`; pinned oracle: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. The mutation patch and full mutant/restored transcripts are retained beside this evidence. No public headers changed.

The complete Phase 6 gate passes at 1,554 assertions run (minimum required: 1,267); Phase 6 remains pending with 101 closed and 332 deferred function rows.
