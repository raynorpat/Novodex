# Phase 2 row 000017: JointActorBody::getShapes

`phys_fn_000017` is the 28-byte helper at RVA `0x00001520` in `Physics/src/core/JointSupport.cpp`. Its contract is: return null when `mShape` is null; for a compound (type 5), return the pointer array at shape offset `+0xe0`; otherwise return the address of the embedded `mShape` member at `this+0x10`.

`NxPhysicsCoreDumpTests` now calls oracle RVA `0x1520` and resolves the candidate symbol `?getShapes@JointActorBody@@QAEPAPAXXZ` from the candidate map. It checks null, non-mesh member, populated compound array, and empty compound array. The registered coverage line is `coredump row_shape_list null=1 nonmesh_member=1 compound_array=1 empty_array=1`.

Mutation proof: changing the candidate compound branch to return `&mShape` makes both compound assertions fail; candidate exits 1 and normalized output differs by 2,241 lines. The source was restored byte-for-byte and rebuilt. Restored oracle and candidate both exit 0 with `stdout_delta=0` and exact stderr. Fresh Phase 3, 6, and 7 gates pass at 976/976, 1,664/1,664, and 2,876/2,876. The row is closed in Phase 2 through the passing Phase 3 staged-pair gate.

Compressed raw logs and hashes: [raw evidence manifest](raw/phase2-shape-list-000017-20261015/manifest.json).
