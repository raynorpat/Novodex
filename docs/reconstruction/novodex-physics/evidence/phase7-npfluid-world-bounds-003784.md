# Public `NxFluid::getWorldBounds`

`phys_fn_003784` (`0x8c210`) now occupies public `NxFluid` vtable slot 45. It takes the Scene read lock and copies six words from the internal fluid's world-bounds field at `+0x5c` to the caller's `NxBounds3`.

The registered `NxPhysicsSimulationTests` fixture seeds six distinct words in that internal field, calls `getWorldBounds` through the public interface, and checks all six returned words. The initial differential was red because the candidate vtable slot was null (oracle exit 0; candidate access violation). The implementation restores an exact differential (`stdout_delta=0`, `stderr_exact=True`).

A wrong-offset mutation reading from `+0x60` was rejected (`candidate_exit=1`, `stdout_delta=60`); the restored build matched exactly. Phase 5, 6, and 7 coverage floors now include this public call. Raw build, mutation, restored differential, and phase-gate logs are under `evidence/raw/phase7-npfluid-world-bounds-003784/`.
