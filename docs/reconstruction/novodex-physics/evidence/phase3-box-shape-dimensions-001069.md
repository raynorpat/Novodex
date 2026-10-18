# `phys_fn_001069` — box-shape dimensions setter

IDA decompilation at RVA `0x000234c0` identifies the `NpBoxShape::setDimensions` wrapper. It takes the Scene write lock, forwards the three dimensions to the internal box setter, releases the lock, and reports the `NpBoxShape.cpp:0x1f` lock diagnostic if the write cannot proceed. The internal setter stores the three words, rebuilds the box hull, updates the owner, and marks the shape dirty. The candidate public handler is `nxBoxHandleSetDimensions` in `Physics/src/NpActor.cpp`.

`NxPhysicsActorDynamicSetterTests` sets dimensions to `(2, 3, 4)` and checks the public dimensions, internal shape words, dirty update, world bounds, and saved descriptor. A temporary no-op mutation of `nxBoxHandleSetDimensions` was rejected by the oracle/candidate differential (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=10`). Restoring the handler produced `stdout_delta=0`, exact stderr, and `differential=pass`. Full staged transcripts and hashes are in `evidence/raw/phase3-box-shape-dimensions-001069-20261018/`.

The public SDK headers are unchanged. This proof covers the dimension-setting path; it does not claim full reconstruction of the shape subsystem.
