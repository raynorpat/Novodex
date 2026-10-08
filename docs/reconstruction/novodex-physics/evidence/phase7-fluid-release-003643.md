# FluidManager releaseFluid mutation closure

`phys_fn_003643` (`FluidManager::releaseFluid`, RVA `0x00089e90`) already
matched the pinned manager warning, primary-array lookup, swap removal, parallel
array removal, and scalar deleting dispatch in the registered
`NxPhysicsSimulationTests` fixture. The fixture seeds two internal fluid
objects and calls the public `NxScene::releaseFluid` route; the clean archived
source matches the oracle exactly (`stdout_delta=0`, `stderr_exact=True`).

For the Phase 7 falsification, a throwaway `git archive` of HEAD
`e6616f8b2c9cfa5f4d44f9e728d47a3d9378df79` was freshly configured and built for
Win32 Release. The candidate mutation changed only the scalar deleting flag for
the removed fluid from `1` to `0`. The registered staged-pair differential
rejected it with both processes exiting 0, exact stderr, and
`stdout_delta=2`; the captured fixture line changed from `flags=1` to
`flags=0`. Restoring the exact `Scene.cpp` source and rebuilding returned the
pair to `stdout_delta=0`, with both processes exiting 0 and exact stderr.

The worktree source and public Physics headers are unchanged. This closes the
tested `releaseFluid` manager behavior; a real `NpFluid` destructor and the
extension-backed lifecycle remain open full-DLL work. Logs from the isolated
archive run are retained under
`D:\FlamingEnt__\novodex-analysis\archives\phase7-fluid-release-003643-20261008`.
