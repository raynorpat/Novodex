# Phase 5: populated Scene teardown frontier

The Scene scalar deleting destructor at `0x145c0` calls the 906-byte body at `0x13f30`. Its internal actor walk (`0x10f00`) destroys each live actor body before releasing Scene arrays and pruning structures. The actor lifecycle drive has two live public actors immediately before `releaseScene`. Both oracle and candidate now free both public actor blocks during that call, registered as `actor scene_live_actor_frees=2.2` and raising the Phase 5 floor from 363 to 364.

The broader destructor is still divergent. The oracle makes two teardown allocations (`30.18`) and frees 45 blocks; the candidate currently makes none and frees 19, including the seven public-wrapper blocks, five blocks for each live actor, the 0xa8 auxiliary manager, and the 0x710 Scene. The oracle free-size sequence is:

```text
14.18.20.4.20.4.28.18.260.1c.10.228.50.18.8.260.1c.228.50.400.400.400.400.400.400.400.400.400.400.400.400.a8.18.18.18.10.60.10.90.c0.20.3c.18.18.710
```

The first seven frees are the empty-scene wrapper path. The next two actor runs and the later auxiliary/pruning releases need their own reconstruction; using the public `releaseActor` path for live actors currently proves ownership release, not the oracle's exact internal destructor sequence. After populated Scene release, SDK teardown is also incomplete (`14` oracle frees versus `7` candidate frees), including the global name-table header. Keep the opt-in `NX_PHYSICS_PROBE_SCENE_TEARDOWN` and `NX_PHYSICS_PROBE_SDK_TEARDOWN` drives red until these sequences match. The final-vtable marker remains a separate gap.

An aimed mutation skipped the live-actor loop. The staged target changed from `actor scene_live_actor_frees=2.2` to `2.0` on the candidate and failed with `stdout_delta=2`; the loop was restored.
