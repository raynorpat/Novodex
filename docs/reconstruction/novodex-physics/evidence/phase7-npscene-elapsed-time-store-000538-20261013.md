# NpScene submitted elapsed-time store `phys_fn_000538`

`phys_fn_000538` stores the elapsed time supplied to `NpScene::simulate` at the internal Scene's `+0x544` field. `NpScene::simulate` in `Physics/src/NpScene.cpp` performs this store before marking simulation pending and signaling the worker.

A throwaway mutation suppressed the store. The registered `NxPhysicsSimulationTests` staged differential catches it (`oracle_exit=0`, candidate exit 1, `stdout_delta=6891`; failed-candidate stderr is not exact). Restoring the store returns to an exact staged differential (`stdout_delta=0`, both exits zero, exact stderr). The fresh Phase 7 gate passed at `1,770/1,770` coverage assertions, including the immutable public-header check. Raw logs are in [`evidence/raw/phase7-npscene-elapsed-time-store-000538-20261013/`](raw/phase7-npscene-elapsed-time-store-000538-20261013/).
