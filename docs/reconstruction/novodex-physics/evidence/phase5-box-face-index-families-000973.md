# Box hull per-face index families

`phys_fn_000973` (`BoxShape::nxBoxRebuildHull`) writes six 0x24-byte face records beginning at Shape+0x150. Each record receives a four-corner index list and a four-edge index list, with corner count 4. The oracle points the six corner rows at RVA `0x00106998` and the six edge rows at RVA `0x001069f8`; each family spans exactly 0x60 bytes. All 24 corner values are in `[0,7]`, and all 24 edge values are in `[0,11]`.

`NxPhysicsObjectLayoutTests` now calls the oracle rebuild after constructor-state capture, validates each record pointer against both contiguous RVA families, folds every pointed-to word, and independently rebuilds the candidate hull to compare both row families. The oracle family digest is `28d445a5`. The candidate reports the same digest and all bounds pass. Mutating the candidate's first edge index from 0 to 12 is caught (`face_indices=0`, `layout candidate mismatches=1`, exit 1); restoring the table returns exact output (`face_indices=1`, `layout candidate mismatches=0`, exit 0).

The existing `face()` accessor exposes the records. The direct runtime consumer of the second list inside the shipped DLL is not identified yet; this evidence closes its layout, contents, bounds, and candidate reconstruction differential.
