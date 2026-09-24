# Phase 5 first dynamic actor auxiliary manager — 2026-09-24

The staged `NxPhysicsActorLifecycleTests` drives the pinned oracle and rebuilt
Win32 DLL through static creation, the first dynamic box actor, two later
dynamic actors, a release, and a two-box actor. The complete ordinary stdout
is equal (`stdout_delta=0`, both exits 0). Fourteen new registered lines raise
the Phase 5 coverage floor from 209 to 223. No public header changed.

The first dynamic actor allocates, in hex size order:
`50.18.228.1c.260.800.400.800.400.800.400.400.400.8.3c.60.10`.
Three `800` scratch buffers are freed during the same creation. The first
five retained `400` buffers belong to the Scene's 0xa8-byte auxiliary
manager at internal Scene+0x48, in owner-offset order `+80`, `+40`, `+60`,
`+50`, `+70`. Guarded allocator owner and stack tracing established these
relationships on the oracle and was removed from the normal harness.
Oracle allocation sites are near RVAs `0x5c21c/0x5c354`,
`0x5bd2b/0x5be15`, `0x5be96/0x5bf89`, `0x5bfc7`, and `0x5c043`.

The manager's five `{first,last,end}` headers are zero after Scene creation
and after the static actor. They then hold 256-slot arrays; the `+50`
header's live count is 1, 2, 3, 2, and 3 after first dynamic, rotated,
quarter-turn, quarter release, and two-box creation. The other four headers
stay at `256/256`, `256/256`, `0/256`, and `256/256`. Registered index
samples check the active `ffffffff` markers at `+40`, the dense indices at
`+50` and `+60`, the `d00beed0` unused marker at `+60`, and zero `+70`
slots. The `+80` active slot points to each dynamic record's `+0x18` field.

The current implementation in `Physics/src/Scene.cpp` reproduces this
observed allocation/array sequence and updates measured slots on register
and release. Its staged-array helper does not establish the oracle's full
growth or allocation-failure contract. The tested release removes the last
dynamic record; a non-last release and Scene teardown still need dedicated
oracle cases. Phase 5 still fails on its explicit `family=vtables` missing
marker (`layout candidate mismatches=1`), despite 223/223 coverage checks
being evaluated.

Sensitivity check: changing the `+60` staging-array fill from `d00beed0` to
zero and rebuilding `NxPhysics` made the staged actor differential fail with
`stdout_delta=6`; `aux_indices_dynamic` visibly changed from the oracle's
`0,d00beed0,d00beed0,d00beed0` to `0,0,0,0`. Restoring the value and
rebuilding returned `stdout_delta=0`. The tool suite passed 628 tests; the
Phase 6, Phase 7, and historical `completed` gates passed after this packet.
