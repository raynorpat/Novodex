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
| Candidate, Physics `84415e27ead308c9b0577bb6801b8f67a9650f9552bd8e040192ec9991b27e37` | fail | 74,576 | `6a2ccf683df333aa` |

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

Captured `mGrid` at entry to `PenetrationMap::finish`, before its corner-interior pass and Morton reorder. In the pinned oracle, the breakpoint is RVA `0x502d0`; in the current candidate build it is RVA `0x45010` (`build/Release/NxPhysics.map`). At both stops `mGrid` is at object offset `+0x70`, and the capture is exactly `64^3 * 4 = 262,144` bytes.

| Pair | Physics SHA-256 | Raw-grid SHA-256 |
|---|---|---|
| Oracle | `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` | `7aaf85f45105ad83687501eb7e73bcaff809a74246618862278034228942d138` |
| Candidate | `f9a2a22f5499a693a28fe799ae28bb9b5be743e6ae6c34ad3e79e9f7804bf33f` | `236b8a7f7eb97ad0aeb2d9f9d0047942cbb3122101ff067cc709e3f6d01475d8` |

The grids differ at 105 words. All differences are low-bit face labels; the filled bit is identical at every cell. For each of those 105 coordinates, an independent double-precision point-to-triangle calculation over the authored tetrahedron gives equal squared distances for the oracle and candidate selected faces (largest computed gap `5.43e-18`). Thus the underlying compute grid already differs before `finish` adds interior flags and Morton-reorders it, and the labels disagree only at equidistant face choices. This also explains why `<=` alone and a globally smaller-face rule were invalid: the oracle's owner is not a uniform face-ID preference. The next discriminating check is the actual built tree and its per-query traversal/rounding at a tied point.
