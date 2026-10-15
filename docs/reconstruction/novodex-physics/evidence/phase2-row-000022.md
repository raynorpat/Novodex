# Phase 2 row 000022: body pose update and slot-6 dispatch

`phys_fn_000022` is the 27-byte helper at RVA `0x00001840` in `Physics/src/core/JointSupport.cpp`. It calls `phys_fn_000754` on the body record at `this+0x08`, then tail-dispatches the same argument to slot 6 of the object at `this+0x10` when present.

`NxPhysicsJointSupportTests` now calls the pinned oracle RVA and the linked candidate implementation over a target-present and target-null case. A synthetic identity mass frame gives the expected position `(0.75, 2.5, 2.0)` and identity quaternion; the target-present case must invoke slot 6 once with `0x13579bdf`, while the null-target case still updates pose and makes no callback. The oracle and candidate digests match.

Mutation proof: removing the `slot6(arg)` call produces two mismatches in the target-present case while the null-target control remains exact. Restoring the source and rebuilding returns the digests to equality. Fresh Phase 6 and 7 gates pass at 1,665/1,665 and 2,877/2,877. The Phase 6 oracle-only fixture pin was updated for the changed source; its existing oracle input and output digest remain unchanged. Phase 2 closes this row through the passing Phase 3 oracle-differential gate; Phases 6 and 7 also pass with this probe registered.

Compressed raw logs and hashes: [raw evidence manifest](raw/phase2-row-000022-20261015/manifest.json).
