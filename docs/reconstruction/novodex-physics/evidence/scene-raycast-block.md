# SceneRaycast..CapsuleShape block

Measurement note for the scene-raycast block plan (`docs/superpowers/plans/2026-09-28-scene-raycast-block.md`),
which closes the work units `SceneRaycast.cpp`, `gap:SceneRaycast.cpp..CapsuleShape.cpp` and `CapsuleShape.cpp`.
The contract is `units/scene-raycast-contract.md`. Each task appends one row to the timing table below.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-28T08:03:47 | 2026-09-28T08:31:22 | 0 | 0 | Audit, contract and trace. No product change and no inventory change. Generated the three bundles; the Ghidra supplement was rerun for the 29 rows with no `ok` decompile, requesting the union with the 34 existing RVAs (63, all `ok`; the existing 34 are byte-identical). `units/scene-raycast-contract.md` gives every one of the 156 code rows its role, sub-area, candidate, status, listing verdict, reaching targets and public path. cdb traces of the oracle (156 row breakpoints) and the candidate (90 candidate-symbol breakpoints) were run over the 16 Phase 5/6/7 staged-pair targets: 51 rows are reached on the oracle side, and 30 of the 90 candidate breakpoints fire (28 on row candidates, 2 on the stand-in helpers 000746x and 000756x) (`evidence/scene-raycast-trace-audit.txt`). Findings: the six public scene raycasts are stubs; 000845 leaves a capsule inertia word uninitialised; 000738, 000841 and 000925 are recorded `reconstructed` but have no product function. |

## Audit summary (Task 1)

Rows (bytes) by sub-area and status. These are the contract's statuses, not the inventory states. Inventory
today: 91 discovered, 14 dynamically_gated, 51 reconstructed.

| Sub-area | implemented | partial | missing | total |
|---|---:|---:|---:|---:|
| scene raycast | 0 (0) | 0 (0) | 10 (2082) | 10 (2082) |
| body-actor math | 8 (4858) | 9 (11446) | 8 (2845) | 25 (19149) |
| island | 6 (601) | 1 (117) | 12 (2022) | 19 (2740) |
| shape | 26 (2416) | 7 (932) | 3 (2553) | 36 (5901) |
| other | 26 (3816) | 1 (706) | 39 (15666) | 66 (20188) |
| total | 66 (11691) | 18 (13201) | 72 (25168) | 156 (50060) |

Verdicts on the 84 implemented or partial rows: 46 faithful, 38 with a defect cited by oracle address.

- **Scene raycast.** All 10 `SceneRaycast.cpp` rows are missing. The public `NxScene::raycast*` methods
  (NpScene.cpp 455-491) are `// (unimplemented)` stubs, so no existing target reaches any raycast row. Their
  loop helpers 000680, 000682 and 000684/000686 sit in `gap:Scene.cpp..SceneRaycast.cpp`, and the NpScene
  wrappers are 000366-000376.
- **Promotion candidates.** Three discovered rows are implemented and faithful: 000768 (NpActorDynamicMath.h
  `nxNpActorUpdateMassFrame`, executed on 11 targets), 000746 (inline in 000768) and 000923 (vendored IceAABB
  `SetCenterExtents`; its candidate breakpoint is not hit, because its oracle callers are missing).
- **Implemented or partial discovered rows with listing defects.** 000756, 000776, 000782, 000784,
  000785/000787, 000789, 000791, 000793/000795, 000797, 000799, 000801, 000933, 000945, 000949, 000951,
  000983 and 000993.
- **Defects in rows already `reconstructed` or `dynamically_gated`.** 000713, 000742, 000829, 000833,
  000845, 000847, 000849, 000867, 000873, 000935, 000937, 000939, 000967, 000969, 000971, 000977, 000981,
  000989 and 000995 (details and addresses in the contract).
  - The worst is 000845. For selector 1, the oracle writes `side` to +0x00 at 0x1001c836. The candidate
    leaves that word unwritten, so every capsule mass computation folds an uninitialised inertia word.
- **State mismatches.** 000738, 000841 and 000925 are `reconstructed` in inventory.json but have no product
  function. 000953, 000961 and 000981 are recorded `reconstructed` but are only partial.
- **Unreached rows.** 105 of the 156 rows are hit by no staged-pair target on the oracle side, and most of
  them are step-only. Rows that are missing but reached by the existing targets on the oracle side: 000722
  and 000748 (every actor creation), 000973 (every box creation) and the mass-frame negated translate 000841.
  The oracle also reaches the box mass chain 000947, 000849, 000829, 000831, 000833, 000839 and 000847 on
  every dynamic box creation. The candidate never calls it: `nxActorComputeMass` computes the mass inline.
