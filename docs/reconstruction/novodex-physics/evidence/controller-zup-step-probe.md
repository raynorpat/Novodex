# Controller grounded Z-up low-obstacle probe

Date: 2026-10-08

## Fixture and observed contract

`NxPhysicsSimulationTests` now exercises the public controller path with `upDirection = NX_Z`, a Z-up floor, a grounded controller, and a low box in its horizontal travel path. It records the final position as exact float words and the collision flags:

```text
simulation controller-grounded-zup-step-probe position=40400000.00000000.3f000000 flags=00000004
```

The pinned oracle and rebuilt candidate both produce `(3.0, 0.0, 0.5)` with flags `4`. This documents the behavior of this one grounded Z-up probe; it does not establish a general step-up rule or close the controller resolver's other sweep, correction, transformed-mesh, callback, or multi-contact paths.

## Sensitivity check

To check that the case observes the configured axis, a temporary mutation in `Scene.cpp` forced the resolver's up-axis to `1` (+Y). The oracle still exited 0, while the mutated candidate exited 1 with a 36-byte stdout delta (the mutated candidate ended at Z `0.7` with flags `1`). The mutation was reverted, the candidate rebuilt, and the direct simulation differential returned equal zero exits, `stdout_delta=0`, and exact stderr.

The restored candidate `NxPhysics.dll` SHA-256 was `64aa29740065dbec8b76444db13d656ea7b4cfc187484f4217628f9b6340678b`.

## Gate verification

- Phase 4 passed on the restored build.
- Phase 5 passed on the restored build; its simulation differential had equal zero exits and exact output.
- Phase 6 passed on the restored build; its simulation differential had equal zero exits and exact output.
- The first Phase 7 run reported an oracle access violation (`0xC0000005`) in `NxPhysicsSceneRaycastTests`. The isolated raycast rerun and a complete Phase 7 retry both passed with equal zero exits and exact output. The retry also passed the full phase gate and all 1,389 registered coverage assertions.
- `test_gate_targets.py` passed all 37 tests after registering the probe and updating the corresponding phase floors.

The gate registry now requires the exact probe line in both staged pairs. No public Physics headers were changed, and no production source change remains from the sensitivity check.
