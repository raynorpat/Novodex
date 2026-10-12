# PMap compute arm (`phys_fn_002047`)

`PenetrationMap::create` now executes its compute arm on a real
`InternalTriangleMesh` and builds the packed map through a temporary OPCODE
model. The authored tetrahedron is pinned at density 32 to 10,444 bytes with
hash `9a70de00aaf0edd4`; the full Phase 4 gate also compares density 80 at
144,272 bytes with hash `1c6814b928a39f10`.

For the row-specific mutation, the one-ray classification loop was changed
from one sample to zero. The candidate then emitted 4,120 bytes with hash
`387b40b2b1eb1513`; the pinned oracle remained at 10,444 bytes and hash
`9a70de00aaf0edd4`. The registered `NxPhysicsTriangleMeshApiTests`
differential rejected the candidate (`stdout_delta=26`, oracle exit 0,
candidate exit 1). Restoring the loop returned the complete target transcript
to an exact match (`stdout_delta=0`, both exits 0, exact stderr).

The previously reconstructed load arm remains covered by the direct
`NxPhysicsAssetTests` probes in `phase4-falsification.md` section 5. The new
compute mutation covers the real-mesh path. Allocation-failure exits and
alternate OPCODE-tree shapes remain open branch coverage.

Raw transcripts:

- [mutation differential](raw/phase4-pmap-export-002049/pmap-compute-mutation.log)
- [restored differential](raw/phase4-pmap-export-002049/pmap-compute-restored.log)
- [full Phase 4 gate](raw/phase4-pmap-export-002049/phase4-gate.log)
