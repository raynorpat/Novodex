# Phase 7 public NxFluid flags (`phys_fn_003744`, `phys_fn_003746`)

The public solver vtable now dispatches `NxFluid::setFlag` and `getFlag` through the reconstructed wrappers in `Physics/src/Scene.cpp`. The setter takes the Scene write lock, applies the requested bit mask to internal-fluid+0x58, and honors the shared fluid API reentry guard. The getter takes the Scene read lock and returns the requested bits from the same word.

The registered `NxPhysicsSimulationTests` fixture seeds `NX_FF_MESH_INCLUDE_SEPARATED`, verifies the initial public getter result, clears the bit through the setter and checks zero, then sets it again and checks the flag. The fixture restores the original private word before continuing. Its required marker is `simulation fluid public-api flags initial=1 cleared=1 set=1`.

Independent mutations reject both rows. Replacing the setter bit update with a no-op is caught (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=61`); forcing the getter to return zero is also caught with the same differential (`stdout_delta=61`). The restored control is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). Mutation and restored logs are preserved in `evidence/raw/phase7-npfluid-flags-003744-003746/`.

Fresh Phase 5, 6, and 7 gates pass at 2,696/2,696, 1,630/1,630, and 1,738/1,738 coverage assertions, respectively. The focused restored differential is exact. Raw gate, red baseline, mutation, and restored logs are compressed under `evidence/raw/phase7-npfluid-flags-003744-003746/`.
