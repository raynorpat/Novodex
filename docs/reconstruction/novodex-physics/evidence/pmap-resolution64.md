# PMap resolution-64 compute differential

Date: 2026-10-02

Scope: one authored tetrahedron, isolated `NxCreatePMap` call at density 64.

## Reproduction

Build `NxPhysicsTriangleMeshApiTests`, then run each pair in a fresh process:

```powershell
build\Release\NxPhysicsTriangleMeshApiTests.exe <absolute-oracle-pair-directory> 64
build\Release\NxPhysicsTriangleMeshApiTests.exe <absolute-candidate-pair-directory> 64
```

The test pins the mesh's cooked vertex and triangle arrays to the same four-point, four-face input before calling `NxCreatePMap`. The optional `64` argument runs only the PMap check; this keeps preceding calls from advancing the DLL's local `rand()` stream.

## Observed output

| Pair | Result | Size | FNV-1a |
|---|---:|---:|---|
| Oracle, Physics `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` | pass | 74,563 | `2c38820e277e9465` |
| Candidate, Physics `0c743099e81efecc3671947022334e196fcb7b0357d779ceca5d3316ebd7bbbc` | fail | 74,562 | `25b0f27b921ae809` |

The existing default density-32 path passes both DLLs with size 10,444 and hash `9a70de00aaf0edd4`.

An offline decoder found 178 differing stored face labels across the density-64 maps. These values are face identifiers. An earlier coordinate-level inspection suggested tie cases, but the Morton-axis mapping used for that inspection was wrong, so it does not establish equal-distance ownership. The stream's sign plane is not used as proof of identical occupancy because the final byte is bit-packed and the quick decoder does not model the stream's final-byte length behavior.

## Current diagnosis

The oracle PMap builder calls its OPCODE point-distance query (`phys_fn_005337`, `FUN_100e8650`) and triangle-distance helper (`FUN_100e7c50`). The candidate currently selects labels in `nxPMapNearestNoLeafNode` using the local `nxPMapPointTriangleSquareDistance` scan. Reversing the candidate's fixed positive/negative child order raised the label mismatch count from 178 to 288 and was reverted. The exact oracle traversal, distance precision, and tie ownership remain to be reconstructed; this evidence does not establish which one is causal.

`NxPhysicsTriangleMeshApiTests.exe <pair-directory> 64` is a focused red repro on the candidate and green on the pinned oracle. No Physics source changed while collecting this evidence.

## Follow-up checks

An IDA decompilation of oracle `sub_100E8650` (RVA `0x000e8650`) confirms the no-leaf traversal shape: check the current node AABB, visit its `+0x18` child first (recurse if internal), then process or descend the `+0x1c` sibling. Candidate `nxPMapNearestNoLeafNode` currently visits `mPosData` before `mNegData` and applies the same AABB pruning rule. This makes a simple positive/negative child-order reversal an unlikely explanation, though it does not prove the two built trees are byte-for-byte identical.

Two additional candidate-only tie experiments were rejected and reverted. Updating on `distance <= best` (or preferring the smaller face ID only on exact candidate-distance equality) produced density-64 size/hash `74574 / abfedae03fe9973b`, still different from the oracle; the smaller-face rule also broke the density-32 fixture. Re-evaluating the existing closest-point parameters with a widened final quadratic produced `74573 / 331649a5849dcb5f` at density 64 and broke density 32 (`10446 / 358d922e23c58b0f`). Neither is a valid fix.

The working source was restored after these experiments. A fresh Release build from the committed source passes the default density-32 candidate probe (`10444 / 9a70de00aaf0edd4`) and retains the density-64 red result (`74576 / 6a2ccf683df333aa`); the isolated oracle probe remains green (`74563 / 2c38820e277e9465`).

## Raw-grid comparison

The initial raw-grid capture used the `PenetrationMap::finish` entry (oracle RVA `0x502d0`, candidate capture-build RVA `0x45010`) with `mGrid` at object offset `+0x70`. The debugger range ended at `+0x3ffff`, so each file was 262,144 bytes: only the first 65,536 of 262,144 words (z slices 0–15), not the full 64^3-word grid. The 105-word mismatch and hashes below are historical first-quarter results only; the corrected full-grid evidence follows.

| Pair | Physics SHA-256 | Raw-grid SHA-256 |
|---|---|---|
| Oracle | `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` | `7aaf85f45105ad83687501eb7e73bcaff809a74246618862278034228942d138` |
| Candidate | `f9a2a22f5499a693a28fe799ae28bb9b5be743e6ae6c34ad3e79e9f7804bf33f` | `236b8a7f7eb97ad0aeb2d9f9d0047942cbb3122101ff067cc709e3f6d01475d8` |

Within that first quarter, the grids differed at 105 words. Those partial results do not prove anything about the remaining 48 z slices. The face-preorder and tie analysis remains useful as local evidence for those captured cells, but should not be generalized to the full resolution-64 grid.

The first oracle query trace (`build/trace-oracle-first35.log`) corrects the world coordinate for the raw-grid mismatch at cell `(7,0,0)`: the helper sees float bits `bf3e93ea/bf800000/bf800000`. Earlier offline arithmetic had predicted `bf3e93e8`, two ULPs away, so it was not a valid debugger condition. The oracle helper's disassembly confirms `0x000e7c50` returns its result directly in x87 ST0 and the caller compares that extended value against a stored float best; the candidate currently uses its local `Distance.cpp`-derived kernel through separate double-returning helpers. Changing coefficient/interior expression order and forcing those helpers inline left density-32 and density-64 hashes unchanged; both experiments were reverted. The exact one-ULP-sensitive behavior still needs a direct matched implementation.

## Follow-up after coordinate-rounding commit `d90d2c2`

The candidate grid sampler now forces the oracle's float stores between multiply, subtract, and add, and `setup()` recomputes `1.0f / lastIndex` for each cell scale instead of reusing the stored reciprocal. The fresh density-32 API test still passes (`10,444` bytes, `9a70de00aaf0edd4`). Density 64 remains red but moved from `74,576 / 6a2ccf683df333aa` to `74,562 / 25b0f27b921ae809`; the oracle remains `74,563 / 2c38820e277e9465`.

A complete 64^3-word capture is 1,048,576 bytes. The corrected comparison uses full captures before and after the surface-corner pass:

| Stage / pair | Capture | SHA-256 |
|---|---|---|
| Before corner pass, oracle | `build/oracle-current64-grid-before-corner-pass-full.bin` | `5c632fe2895a7d431e3b634c4b3035a0c04882163ebae39ad822edc3a8cb0bd4` |
| Before corner pass, candidate | `build/candidate-grid-before-corners.bin` | `f60f68e20888e8097faf4a38b704834fad50916f106a19f91ecf6038a42e6060` |
| After corner pass, oracle | `build/oracle-current64-grid-after-corners-full.bin` | `a3e12c9464a36fa5117f32cbeb4212ba4a96e97b32fddd7080ac248cf6c83ac4` |
| After corner pass, candidate | `build/candidate-current64-grid-after-corners-full.bin` | `b413394f88ef30a2fc50f7dc68603edf348835318d61eac3134c59b24bbb009b` |

Before corner marking, the full grids differ at one coordinate. After corner marking, they differ at two:

| Grid coordinate `(x,y,z)` | Oracle word | Candidate word |
|---|---:|---:|
| Before pass: `(36,26,5)` | `0x00000000` | `0xffffffff` |
| After pass: `(37,27,4)` | `0x80000000` | `0xffffffff` |
| After pass: `(36,26,5)` | `0x00000000` | `0x80000000` |

Thus the current residual starts with one main-grid occupancy/classification discrepancy at `(36,26,5)`; the corner pass leaves that word divergent and also creates the `(37,27,4)` difference. This substantially narrows the immediate issue to the one pre-pass cell and how its surrounding corners are handled. The diagnostic candidate build included a temporary memory dump and breakpoint immediately before corner marking; this instrumentation was removed, then the normal Release DLL was rebuilt and retested. The production candidate SHA-256 is `28eed3f5b24c0ff5b07b54043d75df8a86dec0e79fefa14340f755483a04324f`; density 32 passes (`10444 / 9a70de00aaf0edd4`), while isolated density 64 remains red (`74562 / 25b0f27b921ae809` versus oracle `74563 / 2c38820e277e9465`). Continue by tracing the one pre-pass classification at `(36,26,5)` and the surface-corner handling of both coordinates, then rerun the density-32 and isolated density-64 fixtures.

## Corrected classification trace

The earlier note that `(36,26,5)` was inherited through neighbor propagation was incorrect. The conditional breakpoint at oracle RVA `0x5095d` fires for cell index `0x56a4` and the oracle enters its random-ray classifier (`FUN_1004e5d0`) with mode 3. A breakpoint immediately before the call shows the sample point bits `3ea0ea0c / bdbc8bc3 / bf4d34d3`.

For that same cell, the oracle's random helper returns `0x6867`, `0x3d60`, and `0x6cf8`. The candidate diagnostic receives those exact values and constructs the same normalized direction (`0.667703, -0.0433473, 0.743165`), so the RNG source and call count are not the residual. The oracle helper reports one ray-triangle intersection and classifies the cell inside. The candidate `RayCollider` reports two: face 3 at distance `0.263247` and face 0 at distance `6.95948e-8`; even parity classifies the cell outside. The extra candidate hit is therefore a near-origin face-0 intersection. The remaining cause is in ray-triangle acceptance or its floating-point evaluation, not neighbor propagation.

The candidate's cell state was zero before its direct ray test, confirming it was not preclassified by propagation. The earlier propagation-coordinate alignment did not change the density-64 result and does not explain this cell. A later oracle traversal trace shows the no-leaf tree visits leaf IDs 3, 1, 2, 0 in that order; only face 3 reaches the oracle hit-store path. Face 0 sits under a different child branch, so the missing oracle hit could result from tree bounds/traversal pruning or the triangle test itself. The current trace does not distinguish those causes. Temporary diagnostics and `CollisionFaces` capture were removed; no diagnostic source change is retained.

Fresh Release verification after restoring the uninstrumented source on 2026-10-02: build `NxPhysics` and `NxPhysicsTriangleMeshApiTests` succeeded. The density-32 candidate probe passed (`10444 / 9a70de00aaf0edd4`). Density 64 remains red (`74562 / 25b0f27b921ae809`) against the oracle (`74563 / 2c38820e277e9465`). Candidate DLL SHA-256: `0c743099e81efecc3671947022334e196fcb7b0357d779ceca5d3316ebd7bbbc`. The Phase 5 gate passed earlier in this same turn's prior verified build with 2,037/2,037 assertions.

## Ray arithmetic follow-up

An instrumented candidate run exposed two arithmetic differences before the ray entered `RayCollider`: the sample coordinate and normalized direction. For the target cell `(36,26,5)`, the revised `nxPMapCellCoordinate` keeps multiply/subtract/add in the x87 register stack and rounds only when the caller stores the completed coordinate; the resulting point bits are `3ea0ea0c / bdbc8bc3 / bf4d34d3`, matching the oracle. The random-vector path now materializes each scaled RNG product as float before subtracting `0.5`, matching the oracle raw x component (`3ea19f44`), then reproduces its x87 `z² + y² + x²`, square-root, reciprocal, and component-multiply sequence. The normalized direction bits now match exactly at `3f2aee90 / bd318ce4 / 3f3e400f`.

This correction does not resolve the hit-count difference. The density-64 fixture now returns `74,564` bytes/hash `18da96c824fb1692` versus oracle `74,563`/`2c38820e277e9465`; density 32 remains exact at `10,444`/`9a70de00aaf0edd4`. Thus the ray inputs at the first discrepant cell match, but the current source still counts an extra near-origin face-0 hit. Next trace candidate and oracle node bounds/branch decisions for that face, then compare triangle acceptance if both paths reach the test. The Release build of `NxPhysics` and `NxPhysicsTriangleMeshApiTests` succeeds. These are experimental production arithmetic changes retained for the next debugging step, not a density-64 fix.

## Resolution of the density-64 residual

After matching the ray input at `(36,26,5)`, a fresh full-grid comparison shows that cell now agrees with the oracle. The remaining pre-corner difference moved to `(29,8,2)` (linear index `0x221d`), with corner marking adding `(30,9,1)` as the second final-grid difference.

At `(29,8,2)`, the candidate point is `0.0587300956 / -0.720634937 / -0.920634925`. Its nearest face is face 0 and the candidate squared distance is `6.9477756881042296e-13`. The candidate ray parity is outside (two intersections), and the oracle classifier returns outside (`EAX=0`) for the same cell. The mismatch came from the extra `bestDistanceSquared <= 1.0e-12f` override, which reclassified this nonzero near-surface sample as inside. Narrowing the condition to exact zero (`bestDistanceSquared == 0.0f`) keeps exact-surface handling while matching the oracle for this cell.

Fresh Release build and API verification: density 32 exits 0 at `10444 / 9a70de00aaf0edd4`; an isolated density-64 run exits 0 at `74563 / 2c38820e277e9465`, matching the oracle exactly. Fresh Phase 4 and Phase 5 gates pass at 249/249 and 2,037/2,037 assertions. Candidate DLL SHA-256: `ae962ed1c51140239033ef0a24064c4d6fc6c8d5da6bd721a146a6caec3f9749`. The earlier face-0 traversal hypothesis applied to the pre-normalization candidate and is superseded by this result. Both authored-tetra resolution fixtures are now closed; other topologies/resolutions and alternate PMap creation arms remain unverified.
