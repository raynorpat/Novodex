# PMap resolution-80 compute differential

Date: 2026-10-06

Scope: isolated `NxCreatePMap` on the authored tetrahedron at density 80.
The separate executable target compiles the shared API harness with
`NX_PMAP_COMPUTE_DENSITY=80`, so each fresh oracle/candidate process begins
with the same local `rand()` stream.

## Result

| Pair | Physics SHA-256 | Result | Size | FNV-1a |
|---|---|---:|---:|---|
| Oracle | `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c` | pass | 144,272 | `1c6814b928a39f10` |
| Candidate | `fbb2ca03862a161c64296870e91185e53f5ed63174df9e43f83b935cafe5a100` | pass | 144,272 | `1c6814b928a39f10` |

The paired Phase 4 differential passes exactly (`stdout_delta=0`,
`stderr_exact=True`; `run_differential.ps1 -Targets NxPhysicsPMapResolution80Tests`).
The test prints and asserts the oracle-derived size/hash, and Phase 4 registers
that exact output as required coverage. Both binaries also agree on the
downstream tetrahedron cooking, PMap load/export, actor creation, and settle
transcript exercised by the shared harness.

This closes the authored-tetrahedron density-80 compute fixture only. Other
mesh topologies, resolutions, and alternate PMap creation arms remain open.
The all-scene Viewer suite remains an additional consumer smoke gate across all
39 available scenes, with the five existing signature-verified oracle asset
skips documented in the completion roadmap.
