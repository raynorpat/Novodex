# Actor mass-from-shapes falsification — 2026-10-07

`phys_fn_000008` (`nxActorComputeMassFromShapes`, RVA `0x000010a0`) computes dynamic actor mass, mass-space inertia, and center-of-mass pose from its shapes. The existing `PhysicsActorMassTests.cpp` fixture had fallen out of CMake and Phase 5 registration; it is now restored as `NxPhysicsActorMassTests`. The pinned-oracle baseline and current candidate match exactly across density-based sphere, box, cube, capsule, posed and compound actors, explicit-mass paths, and trigger-only refusals (`stdout_delta=0`, `stderr_exact=True`; `build/actor-mass-current-main-differential.log`). The fresh Phase 5 gate passes all 18 staged targets at 2,302/2,302 coverage assertions (`build/phase5-actor-mass-restored.log`).

A throwaway archive mutation added `1.0` to the first density-scaled inertia component in `nxActorComputeMassFromShapes`. The registered staged-pair differential reported `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=36`, and exact stderr (`build/actor-mass-000008-mutation.log`). The mutation did not touch the main checkout.

This closes the row's mutation-sensitivity requirement. The remaining Phase 5 rows and full-DLL work remain open. Public Physics headers are unchanged.
