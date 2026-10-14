# Phase 7 continuation: Scene-before-mesh SDK teardown (`phys_fn_000492`)

The pinned destructor destroys still-registered Scenes before the SDK-owned triangle meshes. The candidate previously destroyed triangle meshes first. `NxPhysicsPopulatedSceneTeardownTests` now leaves a triangle-mesh actor in an SDK-owned Scene and records allocator free order for the Scene and mesh objects.

The registered differential was red before the production reorder: oracle reported `sdk_scene_before_mesh=1`, candidate reported `0`, and the candidate failed (`stdout_delta=2`). Moving the triangle-mesh loop after the registered-Scene loop makes both pairs report the oracle order; the focused differential is exact (`stdout_delta=0`, exact stderr), and SDK teardown still reports zero outstanding allocator blocks. Fresh Phase 7 passes at 1,980/1,980 assertions.

This closes one remaining lifecycle behavior in `phys_fn_000492`; the function and scalar deleting thunk `phys_fn_000494` remain open pending full row-level accounting and closure proof. No public Physics headers changed. Compressed raw transcripts are in `evidence/raw/phase7-sdk-scene-before-mesh-000492-20261014/`.
