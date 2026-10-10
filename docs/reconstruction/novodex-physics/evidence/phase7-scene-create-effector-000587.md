# `NxSceneInternal::createSpringAndDamperEffector` mutation closure (`phys_fn_000587`)

`phys_fn_000587` is the 187-byte Scene row at RVA `0x00010c90`. It allocates and constructs a `SpringAndDamperEffector`, links it into the Scene effector list, updates count and iterator state, and initializes its bodies, spring, and damper values. The registered Phase 7 `NxPhysicsEffectorTests` fixture exercises creation, configuration, list ordering, release, actor teardown, and Scene teardown.

The clean baseline matched the pinned oracle (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). For the row-specific mutation, an immediate null return was inserted at the start of `createSpringAndDamperEffector`. The fixture changed from `effector create created=yes` to `created=no`; the registered differential rejected the mutant (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=85`, exact stderr). Mutant `NxPhysics.dll` SHA-256: `8385eafd26be1b7b96a1c1c810834c47a3a580886cdd11eee854bdb1c6579879`.

`Scene.cpp` was restored byte-for-byte (SHA-256 `5806d9cb903e2c11d863ee69ae6202d3c6c7ce481b0937e73c99080383bd7cf9` before and after). After rebuilding, the restored differential returned both exits zero, `stdout_delta=0`, and exact stderr. Restored `NxPhysics.dll` SHA-256: `73bc361af8d7976c1170b8a37f6de284da9365b920a8f3a5c586b4d7bd85b54e`.

Evidence: `build/mutation-000587/mutant-differential.log` and `build/mutation-000587/restored-differential.log`. The row is now closed in the Phase 7 ledger. Phase 8 terminal closure remains open.
