# Task M7.6 brief — FluidManager substep processing

Plan: `docs/superpowers/plans/2026-09-24-nxphysics-completion.md`
Base: `b238993f3b372fa485bd5613e829e37fc2c6109a`

## Contract

Recover `phys_fn_003630` at RVA `0x89bd0`, called from the Scene substep loop. IDA's pinned-oracle listing shows the `+0x28` dirty-static-collision branch: destroy the prior collision object, rebuild it from manager fields, clear the dirty flag, then walk the fluid pointer range `[+0x04,+0x08)` and call each fluid's step routine at RVA `0x85a40` (`phys_fn_003495`) with the substep timestep. The rebuild helper at RVA `0x861b0` is `phys_fn_003505` and has extensive static-collision dependencies.

Before implementing, recover the exact x86 calling convention, timestep precision, callbacks, and row dependencies for `phys_fn_003495` and `phys_fn_003505`. Establish scratch-object preconditions or use public scene-created objects; do not invoke guessed layouts. Keep unavailable-backend warning text/order and existing behavior intact. Public headers are immutable.

## Required proof

- Add a pinned-oracle/candidate actual-DLL differential with at least two live fluids and distinct callback payloads, checking iteration order, timestep bits, callback tuple/return/stack balance, warning order, and state changes.
- Cover dirty and clean static-collision paths, including replacement/destruction and allocator behavior, once their callee contracts are established.
- Mutation-test the manager's fluid loop and dirty-collision dispatch independently; the registered assertion must identify the corresponding row behavior.
- Rebuild through CMake; run the focused test, Phase 5/7 gates, inventory validation, and both public-header manifests.
- Record candidate/oracle hashes, seed, mutation patches, raw output, and dependency proof; keep `phys_fn_003630` open if a required callee remains guessed or unproven.

## Current progress

Implemented and directly compared the empty-emitter per-fluid substep and clean-static-collision manager branch. The actual-DLL fixture covers one direct substep and two live fluids with distinct callback payloads, comparing callback order/arguments, timestep bits, state clears, result, and stack balance. The manager-loop increment-only mutation is rejected with three mismatches; restoring the dispatch returns to zero. Phase 5 and Phase 7 gates pass at 2,635/2,635 and 1,662/1,662. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-substep-003495-manager-clean-003630.md`.

Fresh CMake Release rebuild passed; all 810 reconstruction-tool tests pass. Viewer CTest exercised all 39 registered scene entrypoints (34 passed, five known oracle-baseline PMap/material skips), and the viewer scene replacement/cancellation/exit test passed. Candidate hash for these gate runs: `970bc0866483b8fe0943ff02fe2f8f8db88d50fa134de168b1113c85c054811c`.

The `+0x28` dirty static-collision branch remains unimplemented. The nonempty-emitter path through `003624`/`003601` also remains unproven; keep `003495`, `003601`, `003505`, and `003630` open. Do not count this packet as complete until those dependencies, dirty/clean collision behavior, and a dirty-branch mutation are verified.

Partial follow-up: the enabled, bodyless-actor branch of `phys_fn_003601` now has a direct oracle/candidate callback differential. A mutant enabled-check polarity is rejected; restored control matches all three callbacks, their order/arguments, vector words, mass, return, and stack. The row remains open because the live-body force-application branch and the nonempty-emitter caller path are not exercised. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-body-repulsion-003601.md`.

Live-body follow-up: `phys_fn_003601` now has a second actual-DLL differential using SDK-created actors in isolated staged pairs. It verifies the callback payloads, impulse application, body state, and x86 stack. The unscaled-vector mutation is rejected; restored transcript is exact. The prior indexed x87 scaling loop faulted on the body-backed path, so it is now scalar float multiplication matching the pinned state words. This still does not prove the full `003624` nonempty-emitter caller or dirty `003505` branch. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-emitter-live-body-repulsion-003601.md`.

Nonempty-substep follow-up: `NxPhysicsFluidEmitterAbiTests` calls the actual oracle and map-resolved `003495` entries with one bodyless emitter, checking the pose callback, FluidRun tuple/vector/timestep, state transitions, skipped repulsion, return, and stack. A mutation removing the per-emitter `003624` update call is rejected (`pose_callback=0`, `mismatches=1`); restored output is exact. Fresh Phase 5/7 pass at 2,638/2,638 and 1,665/1,665, and all 810 tooling tests pass. Candidate SHA-256 `8180eeee170d23d07139445d6e91edb888a454524cdf61d3d5cc6a9d643445b9`; oracle SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Evidence: `docs/reconstruction/novodex-physics/evidence/phase7-fluid-nonempty-emitter-substep-003495.md`. The attached-emitter full call chain, `003601` early guards, and dirty `003505` branch remain open.
