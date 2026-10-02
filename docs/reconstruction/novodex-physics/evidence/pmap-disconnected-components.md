# PMap disconnected-component compute mismatch

Date: 2026-10-02. Oracle: pinned `NxPhysics.dll` SHA-256
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` from
`D:\FlamingEnt__\Unreal_3\Binaries`.

## Reproduction

Build `NxPhysicsTriangleMeshApiTests` and run the isolated fixture:

```powershell
build\Release\NxPhysicsTriangleMeshApiTests.exe <absolute-pair-directory> disconnected32
```

The fixture creates two disjoint, identically oriented tetrahedra with eight
vertices and eight triangles. It overwrites the cooked arrays with the same
authored arrays before calling `NxCreatePMap` at density 32, and resets `rand`
immediately before the call. Oracle and candidate bounds are identical:
`bf800000.bf800000.bf800000.40a00000.3f800000.3f800000`.

## Result

| Pair | Physics SHA-256 | Result | Size | FNV-1a |
|---|---|---:|---:|---|
| Oracle | `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` | pass | 4,490 | `c2276558dc4be981` |
| Candidate | `c6332d1db3fe07d0b3e9d2f32ddc0b29f14801c78b6c79bcc6bde4966fd5af` | fail | 9,740 | `e29f19f7b131fdf8` |

This demonstrates that the exact authored-tetra fixtures do not establish
multi-component PMap fidelity. The density-64 authored tetrahedron remains
exact (`74,563`, `2c38820e277e9465`) on the same candidate.

## Triage

The oracle and candidate compute the same six mesh bounds. A temporary change
from the candidate's unquantized Opcode build to a quantized build left the
candidate output unchanged. Enabling ray backface culling changed the result to
`30,792 / f93a38a59cc17dcf`, farther from the oracle. Both experiments were
reverted; the candidate source retains `mNoLeaf=true`, `mQuantized=false`, and
backface culling disabled. The root cause remains unresolved. Trace the first
divergent unclassified grid sample through random-ray generation, Opcode tree
traversal, and hit parity before changing the implementation.

This is a pinned red fixture for the PMap work queue, not a closed compute
path. Public Physics headers remain unchanged.
