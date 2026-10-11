# Task M7.6 brief — FluidManager substep processing

Plan: `docs/superpowers/plans/2026-09-24-nxphysics-completion.md`
Base: `b238993f3b372fa485bd5613e829e37fc2c6109a`

## Contract

Recover `phys_fn_003630` at RVA `0x89bd0`, called from the Scene substep loop. The current private `nxFluidManagerStepDisabled` only emits the `stepFluids` warning when manager byte `+0x2b` is zero. IDA's pinned-oracle listing also shows the `+0x28` dirty-static-collision branch: destroy the prior collision object, rebuild it from manager fields, clear the dirty flag, then walk the fluid pointer range `[+0x04,+0x08)` and call each fluid's step routine at RVA `0x85a40` (`phys_fn_003495`) with the substep timestep. The rebuild helper at RVA `0x861b0` is `phys_fn_003505` and has extensive static-collision dependencies.

Before implementing, recover the exact x86 calling convention, timestep precision, callbacks, and row dependencies for `phys_fn_003495` and `phys_fn_003505`. Establish scratch-object preconditions or use public scene-created objects; do not invoke guessed layouts. Keep unavailable-backend warning text/order and existing behavior intact. Public headers are immutable.

## Required proof

- Add a pinned-oracle/candidate actual-DLL differential with at least two live fluids and distinct callback payloads, checking iteration order, timestep bits, callback tuple/return/stack balance, warning order, and state changes.
- Cover dirty and clean static-collision paths, including replacement/destruction and allocator behavior, once their callee contracts are established.
- Mutation-test the manager's fluid loop and dirty-collision dispatch independently; the registered assertion must identify the corresponding row behavior.
- Rebuild through CMake; run the focused test, Phase 5/7 gates, inventory validation, and both public-header manifests.
- Record candidate/oracle hashes, seed, mutation patches, raw output, and dependency proof; keep `phys_fn_003630` open if a required callee remains guessed or unproven.
