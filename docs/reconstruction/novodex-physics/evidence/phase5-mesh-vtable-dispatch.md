# Mesh primary vtable: transform-center row

The pinned mesh primary table begins at `.rdata` RVA `0x107630`. Its 18 slot RVAs are `28e80, 27740, 256f0, 27f10, 28e10, 29230, 266a0, 29610, 27ec0, 28ed0, 29190, 27e90, 27e30, 27e60, 27f00, 27f00, 27f00, 27e20` (hex). Thus `phys_fn_001403` at `0x29190` belongs to slot 10. Earlier object-model notes and the inventory had mislabeled it slot 4; the existing transcription was also attached to `BoxShape`. It now belongs to `MeshShape` without changing the public headers or the function body.

Slot 10 copies four words from the mesh record at `+0x5c`, transforms the first three through the shape pose, and preserves the fourth. The shape vtable harness compares oracle and candidate output bytes for four nonidentity pose and center inputs. Its pinned transcript is `shape vtable oracle_digest=e7f8a2c7 cases=404 failures=0`. The older layout-harness drive also reports `transpt1403 candidate failures=0` after the class correction.

The candidate mesh constructor does not yet install an 18-entry table. Its raycast, sweep, mass, bounds, and debug-render rows still require reconstruction and callable-table tests. The Phase 5 gate therefore remains red at `CANDIDATE-MISSING family=vtables`, with the layout harness reporting one overall candidate mismatch. The new isolated slot-10 check does not close that marker.
