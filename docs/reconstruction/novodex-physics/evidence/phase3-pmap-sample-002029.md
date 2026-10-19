# Penetration-map cell sampler: phys_fn_002029

`phys_fn_002029` (`0x0004f070`, 3,751 bytes) is the private eight-corner query used by the penetration-map mesh-contact path. `PenetrationMap::sampleCell` in `Physics/src/PMap.cpp` reconstructs the Morton-spread indices, last-coordinate suppression, cell-corner positions, nearest-triangle barycentric projection, trilinear displacement weights, split cell-class accumulators, and optional direction/matrix acceptance test. The method uses the existing point-to-triangle kernel `phys_fn_001672`. No public headers changed.

`NxPhysicsAssetTests` now calls the oracle row directly with a raw `0x78`-byte private-layout map and a two-face triangle mesh. Seven fixtures compare the oracle and candidate return values plus all three output words:

| Fixture | Oracle result |
| --- | --- |
| outside cells | `true`, `bd000000 bd000000 be800000` |
| interior cells | `true`, `bd000000 bd000000 be800000` |
| mixed cells | `true`, `bd000000 bd000000 be800000` |
| split class reject | `false`, `bd000000 bd000000 3f000000` |
| direction and matrix | `true`, `bd000000 bd000000 be800000` |
| opposing direction | `false`, `bd000000 bd000000 be800000` |
| upper corner `(1,1,1)` | `true`, `be580000 be580000 bed80000` |

The oracle digest is `3416250d`; all candidate words match. The pinned DLL SHA-256 is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

The mutation swaps which accumulator receives high-bit and low-bit cells. It changes only the `split_class_reject` candidate return, from `false` to `true`; the target exits 1 with `candidate mismatches=1`. Restoring the code returns `candidate mismatches=0` and exit 0. Transcripts: `evidence/raw/phase3-pmap-sample-002029/mutant.log` and `evidence/raw/phase3-pmap-sample-002029/restored-control.log`.

This closes the direct cell-query row through the Phase 4 asset oracle gate and discharges the Phase 3 deferral. It does not close caller `phys_fn_001878` or claim that the full mesh-contact path is reconstructed. The alternate filename/stream load arms and compute-arm allocation/tree branches remain open as documented in `Physics/src/include/PMap.h`.

**Superseded implementation (2026-10-19).** The harness for the mesh/PMap contact rows drew 4,000 cells, points, directions and matrices over maps built from real meshes and found this row's C++ reconstruction disagreeing with the oracle on 1,583 of them; the seven fixtures above are real but were not enough. The row is now a listing transcription and agrees on all 4,000. See `evidence/phase3-mesh-pmap-contact.md`.
