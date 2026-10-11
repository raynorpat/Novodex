`phys_fn_003495` (RVA `0x85a40`) now reconstructs the observed empty-emitter substep, and `phys_fn_003630` (RVA `0x89bd0`) dispatches the clean-static-collision path over the manager's live-fluid range. The manager ABI wrapper consumes one stack `NxReal` (`ret 4`) and forwards it in pointer order. Dirty static-collision refresh remains unimplemented, so neither row is closed.

The registered `NxPhysicsFluidEmitterAbiTests` fixture patches the oracle's `FluidRun` callback slot and calls the actual pinned DLL at `0x85a40` and `0x89bd0`; candidate entries are resolved from `NxPhysics.map` and run from the built DLL. The one-fluid substep case checks the timestep bits at `fluid+0x74`, the vector copied from `backendCore+0x520`, the full callback tuple, the callback's float return handling, and clearing the state word at `state+4`. The manager case uses two live fluids with different system handles, fluid handles, and vector payloads. It checks oracle/candidate callback order, callback arguments, timestep bits, state clears, return values, and x86 stack balance.

The focused transcript is `build/FluidGate/phase7-fluid-step-substep.log`:

```text
fluid substep empty-emitter cases=1 callback=1 timestep=1 state-clear=1 mismatches=0
fluid manager step live-fluids=2 oracle_callbacks=2 candidate_callbacks=2 order=1 timestep=1 state=1 stack=1/1 mismatches=0
fluid emitter aggregate cases=6 mismatches=0
```

Verification on the current Win32 Release build:

- Phase 5 gate passes at `2635/2635`; Phase 7 passes at `1662/1662`.
- Inventory passes with 6,338 functions, 5,138 data objects, and zero unexplained rows.
- Both public Physics header manifests pass with 80 files each.
- Oracle `NxPhysics.dll` SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
- Candidate `NxPhysics.dll` SHA-256: `970bc0866483b8fe0943ff02fe2f8f8db88d50fa134de168b1113c85c054811c`.

The Release viewer was rebuilt against this candidate and the complete registered
scene smoke inventory was run with CTest. All 39 scene entrypoints were exercised:
34 passed and five were skipped only under the pinned-baseline diagnostics already
registered by `Viewer/CMakeLists.txt` (CowPile and PMapTest8/10/12 need PMap
creation; TruckDemo lacks its material asset). The separate scene replacement,
cancellation, and clean-exit test also passed. The scene asset preparation is a
disposable copy under `build/Viewer/scene-smoke/assets`.

The direct differential intentionally uses empty emitter ranges. The following remain open before row closure: nonempty-emitter `003624`/`003601` callback and actor-force effects; `003505` static-collision construction; `003630` dirty-collision destruction/rebuild/clear path; and independent mutation evidence for the dirty flag and fluid loop. The inventory therefore keeps `003495`, `003601`, `003505`, and `003630` open.

The manager-loop assertion was mutation-tested by replacing the candidate's per-fluid call with an increment-only loop. The actual oracle/candidate differential then reported `oracle_callbacks=2 candidate_callbacks=0 order=0 timestep=0 state=0 mismatches=3` and exited 1 (`build/FluidGate/phase7-fluid-manager-loop-mutant.log`). Restoring the call rebuilt cleanly and returned to zero mismatches (`build/FluidGate/phase7-fluid-step-substep.log`).
