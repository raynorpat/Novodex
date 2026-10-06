# `NxScene::simulate` elapsed-time boundary

The public `NxScene::simulate` path is now pinned at its negative and zero elapsed-time boundaries. The negative-time fixture first failed against the candidate: the oracle returned one `NXE_INVALID_PARAMETER` report at `NpScene.cpp:540`, while the candidate returned none (`stdout_delta=2`). The oracle also accepted `simulate(0.0f)`, completed the step, and allowed nonblocking fetch; the candidate left the scene idle. The fixture uses a bounded nonblocking poll so the candidate failure cannot hang the test.

`NpScene::simulate` now reports the oracle's exact invalid-parameter tuple for negative elapsed time, then accepts zero and submits it through the existing scene worker path. The focused paired differential is exact (`stdout_delta=0`, `stderr_exact=True`; `build/FluidGate/negative-and-zero-dt-green.log`). The full Phase 7 gate passes 11 targets and 1,329/1,329 registered assertions (`build/FluidGate/phase7-negative-zero-dt.log`), and the Phase 5 staged differential passes all 13 targets (`build/FluidGate/phase5-negative-zero-dt.log`).

After rebuilding the Viewer and its CTest support targets against this DLL, all 48 Viewer selections pass, covering every available scene. The five pinned-oracle asset cases remain skipped by their existing exact signatures (`build/FluidGate/viewer-scenes-negative-zero-dt.log`).

This verifies the elapsed-time boundary and a zero-time completion. It does not close all of `phys_fn_000394`: the full row's lock/reentry protocol, internal scene dispatch, condition signaling, and worker-state effects still require static review and row-specific mutation falsification. Public Physics headers are unchanged.
