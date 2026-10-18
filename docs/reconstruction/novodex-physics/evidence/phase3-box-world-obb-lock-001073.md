# Box world OBB read-lock wrapper (`phys_fn_001073`)

IDA at RVA `0x00023550` shows a read-lock enter on the box handle's lock link, a call through the internal box shape world-OBB path, then the matching read-lock leave. `nxBoxHandleGetWorldOBB` now brackets the internal getter with that lock.

`NxPhysicsActorDynamicSetterTests` seeds the handle's read-lock state as held by no thread, calls `getWorldOBB`, and prints the resulting state. Oracle and restored candidate both report `setter box_obb_read_lock=0.1`. Omitting the enter/leave pair is detected (`stdout_delta=2`; oracle marker `0.1`, candidate marker `1.0`); the restored staged differential is exact (`stdout_delta=0`, stderr exact). The test marker is registered in the Phase 5/6/7 target contract.

Raw build and differential logs are archived under [evidence/raw/phase3-box-world-obb-lock-001073-20261018](raw/phase3-box-world-obb-lock-001073-20261018/).
