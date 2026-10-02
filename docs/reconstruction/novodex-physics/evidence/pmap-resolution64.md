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

An offline decoder found 178 differing stored face labels across the density-64 maps. These values are face identifiers; mismatches inspected so far occur where adjacent faces have equal point distance. The stream's sign plane is not used as proof of identical occupancy because the final byte is bit-packed and the quick decoder does not model the stream's final-byte length behavior.

## Current diagnosis

The oracle PMap builder calls its OPCODE point-distance query (`phys_fn_005337`, `FUN_100e8650`) and triangle-distance helper (`FUN_100e7c50`). The candidate currently selects labels in `nxPMapNearestNoLeafNode` using the local `nxPMapPointTriangleSquareDistance` scan. Reversing the candidate's fixed positive/negative child order raised the label mismatch count from 178 to 288 and was reverted. The exact oracle traversal, distance precision, and tie ownership remain to be reconstructed; this evidence does not establish which one is causal.

`NxPhysicsTriangleMeshApiTests.exe <pair-directory> 64` is a focused red repro on the candidate and green on the pinned oracle. No Physics source changed while collecting this evidence.
