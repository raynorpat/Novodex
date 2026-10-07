# Scene shape registration: `phys_fn_002423`

IDA identifies `phys_fn_002423` at RVA `0x0005c390` as inserting a shape pointer into the scene auxiliary manager's slot-indexed array. The shape's scene slot is at `shape+0xd4`; the pointer table begins at manager `+0x90` and grows in 256-slot chunks. `nxSceneAuxRegisterShape` in `Physics/src/Scene.cpp` models the insertion together with the shared occupancy, reverse-index, and active-list bookkeeping.

`NxPhysicsBodyCreationTests` checks the registered shape pointer and array capacity after out-of-order slot reuse and after 257 simultaneous bodies cross ID `0x100`. The staged oracle and candidate match exactly (`stdout_delta=0`, exact stderr; `build/phase3-mainline-002423-final.log`).

For falsification, a throwaway `git archive` of commit `5cbdfe20` was extracted under the system temp directory. Its `Scene.cpp` copy omitted only the pointer-store line; the rebuilt candidate and pinned oracle both exited 0, while the body-creation differential reported `stdout_delta=6` (`build/shape-register-002423-archive-mutation.log`). The generated project and mainline source were restored, and a clean Release `NxPhysics` rebuild followed by the same differential returned `stdout_delta=0` (`build/shape-register-002423-green.log`).

The Phase 3 gate passes at 527/527 coverage assertions. Phase 5 passes at 2,244/2,244 and Phase 7 at 1,365/1,365. The full Viewer CTest selection passes 56/56, covering all 39 available scenes; five known pinned-oracle asset cases skip on their established signatures. No public Physics header changed.
