# Spring-force branch differential: `phys_fn_003970`

The public `NxPhysicsSimulationTests` fixture now measures `SpringAndDamperEffector::springForce` through six independent dynamic bodies and world anchors. It fetches two `1/60`-second steps before recording each body's full linear-velocity bit pattern. Both ends use the same off-centre height, matching the proven public effector path while making their horizontal separation the exact spring length.

The cases cover, in order: stretch ramp at length 2 (relaxed length 1, stretch saturation 4); stretch saturation at 4; the relaxed boundary at 1; compression ramp at 0.75 (compression saturation 0.5); compression saturation at 0.5; and disabled spring force with both configured maxima zero. Oracle and restored candidate report the same six velocity vectors:

```text
408e38e3.00000000.00000000/
41555556.00000000.00000000/
00000000.00000000.00000000/
c0d55554.00000000.00000000/
c1555556.00000000.00000000/
00000000.00000000.00000000
```

Two source mutations demonstrate branch sensitivity. Replacing the stretch-ramp displacement `(distance - relaxed)` with `distance` changes the first case and is rejected (`stdout_delta=6`, both processes exit 0, stderr exact). Replacing the compression-ramp predicate with a predicate that skips the ramp changes the fourth case and is rejected (`stdout_delta=2`, both processes exit 0, stderr exact). Restored control is exact (`stdout_delta=0`, both processes exit 0, stderr exact).

The case marker is registered on the shared Phase 5/6/7 simulation target. Fresh gates pass at 2,807/2,807, 1,698/1,698, and 2,935/2,935 coverage assertions. Raw differentials, mutations, and gate logs are retained in `evidence/raw/phase6-spring-force-branches-003970-20261016/`. This closes the scalar spring branch proof only; the full DLL reconstruction and the remaining Phase 6/7 inventory are still open.
