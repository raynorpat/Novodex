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

## Neighbor-propagation follow-up

Debugger tracing shows the pinned oracle does not call its random-ray classifier for `(36,26,5)`; the word is inherited through neighbor propagation. A temporary candidate diagnostic showed that the candidate reaches a direct point classification at this location instead. The production propagation loop was aligned with the recovered oracle behavior by routing neighbor coordinates through the same float-store coordinate helper and summing squared deltas in z→y→x order. The distance query returns a float square root, which the comparison squares again as the oracle does. This code change did not alter the density-64 result, so it is recorded as an implementation alignment rather than a resolution of the residual.

Fresh Release verification on 2026-10-02: `run_phase_gate.ps1 -Phase 5` passed with 2,037/2,037 coverage assertions; the gate's object-layout and shape-vtable differential tests passed. `NxPhysicsTriangleMeshApiTests` matches the oracle at density 32 (`10444 / 9a70de00aaf0edd4`). The isolated density-64 candidate remains red (`74562 / 25b0f27b921ae809`) against the oracle (`74563 / 2c38820e277e9465`). The fresh staged candidate DLL SHA-256 is `c6f53de91371297dc49e552f482f0da5e8c923ff928f4367dd037f46d3cf1708`.
