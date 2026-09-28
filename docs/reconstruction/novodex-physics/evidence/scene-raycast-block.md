# SceneRaycast..CapsuleShape block

Measurement note for the scene-raycast block plan (`docs/superpowers/plans/2026-09-28-scene-raycast-block.md`),
which closes the work units `SceneRaycast.cpp`, `gap:SceneRaycast.cpp..CapsuleShape.cpp` and `CapsuleShape.cpp`.
The contract is `units/scene-raycast-contract.md`. Each task appends one row to the timing table below.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-28T08:03:47 | 2026-09-28T08:31:22 | 0 | 0 | Audit, contract and trace. No product change and no inventory change. Generated the three bundles; the Ghidra supplement was rerun for the 29 rows with no `ok` decompile, requesting the union with the 34 existing RVAs (63, all `ok`; the existing 34 are byte-identical). `units/scene-raycast-contract.md` gives every one of the 156 code rows its role, sub-area, candidate, status, listing verdict, reaching targets and public path. cdb traces of the oracle (156 row breakpoints) and the candidate (90 candidate-symbol breakpoints) were run over the 16 Phase 5/6/7 staged-pair targets: 51 rows are reached on the oracle side, and 30 of the 90 candidate breakpoints fire (28 on row candidates, 2 on the stand-in helpers 000746x and 000756x) (`evidence/scene-raycast-trace-audit.txt`). Findings: the six public scene raycasts are stubs; 000845 leaves a capsule inertia word uninitialised; 000738, 000841 and 000925 are recorded `reconstructed` but have no product function. |
| 1 (revision 2) | 2026-09-28T08:34:00 | 2026-09-28T08:48:04 | 0 | 0 | Review fixes, docs only. Every implemented row was re-swept for CRT math where the oracle uses x87, for the allocator ([0x101041bc] versus nxGetSdkAllocator) and for the calling convention. Eight faithful verdicts became defects (000744, 000768, 000851, 000853, 000943, 000975, 000979, 000987), and 000953/000961 are now counted as defects. The final tally is 36 faithful and 48 defects. The sub-areas are split (CCD, mass properties, contact-pair manager, visualisation), with a step-only column (46 rows, 17,939 B). Added the handover to the NpActor session (000742, 000784, 000785), the handle-row ownership correction (000929, 000933, 000983, 000993, 000995 are ours), the in-scope raycast entries (000366-000376, 000680, 000682, 000684/000686), the state mismatches with actions, and the ObjectModel.cpp:1057 marker violation. |

## Audit summary (Task 1, revision 2)

Rows (bytes) by sub-area and status. These are the contract's statuses, not the inventory states. Inventory
today: 91 discovered, 14 dynamically_gated, 51 reconstructed.

| Sub-area | implemented | partial | missing | total |
|---|---:|---:|---:|---:|
| scene raycast | 0 (0) | 0 (0) | 10 (2082) | 10 (2082) |
| body-actor math | 8 (4858) | 9 (11446) | 6 (2604) | 23 (18908) |
| island | 6 (601) | 1 (117) | 12 (2022) | 19 (2740) |
| CCD | 0 (0) | 0 (0) | 4 (685) | 4 (685) |
| shape | 26 (2416) | 7 (932) | 3 (2553) | 36 (5901) |
| mass properties | 25 (3713) | 0 (0) | 1 (43) | 26 (3756) |
| contact-pair manager | 1 (103) | 1 (706) | 32 (13082) | 34 (13891) |
| visualisation | 0 (0) | 0 (0) | 3 (2084) | 3 (2084) |
| other | 0 (0) | 0 (0) | 1 (13) | 1 (13) |
| total | 66 (11691) | 18 (13201) | 72 (25168) | 156 (50060) |

**Verdicts.** Of the 84 implemented or partial rows, 36 are faithful and 48 have a defect cited by oracle
address.

Revision 2 re-swept every implemented row against three rules:
- CRT math where the oracle uses x87;
- the allocator: every allocating row here uses the Foundation allocator [0x101041bc], never
  nxGetSdkAllocator;
- the calling convention.

Eight verdicts moved from faithful to a defect: 000744, 000768, 000851, 000853, 000943, 000975, 000979
and 000987. 000953 and 000961 (constants only) are also counted as defects now.

**Scene raycast.** All 10 `SceneRaycast.cpp` rows are missing. The public `NxScene::raycast*` methods
(NpScene.cpp 455-491) are `// (unimplemented)` stubs, so no existing target reaches any raycast row.
Task 3 also needs, in scope and unclaimed by other sessions:
- the NpScene wrappers 000366-000376 (slots 42-47 of 0x10105a98);
- the loop helpers 000680, 000682 and 000684/000686 (gap:Scene..SceneRaycast).

**Promotion candidates.** Only two discovered rows are implemented and faithful:
- 000746, inline in 000768, with no own hit. Its out-of-line copies at candidate 0x130a0 and 0x2a960 are
  never hit, so its evidence is 000768's breakpoint.
- 000923, vendored IceAABB `SetCenterExtents`. Its candidate breakpoint is not hit, because its oracle
  callers are missing.

000768 needs the X87Sqrt.h fix (fsqrt at 0x10018216/0x100182a8/0x100182f3/0x10018339) and a thiscall
entry before it can be promoted.

**Defects in rows already `reconstructed` or `dynamically_gated`:** 000713, 000742, 000744, 000829,
000833, 000845, 000847, 000849, 000851, 000853, 000867, 000873, 000935, 000937, 000939, 000943, 000967,
000969, 000971, 000975, 000977, 000979, 000981, 000987, 000989 and 000995 (details and addresses in the
contract).

The worst is 000845. For selector 1, the oracle writes `side` to +0x00 at 0x1001c836. The candidate leaves
that word unwritten, so every capsule mass computation folds an uninitialised inertia word.

**State mismatches.** 000738, 000841 and 000925 are recorded `reconstructed`, but no product function
exists. Their proofs are oracle-only harness blocks in `tests/PhysicsObjectLayoutTests.cpp`: smallflag2,
negtrans and shapegetters2. 000953, 000961 and 000981 are recorded `reconstructed` but are only partial.
The contract gives the recommended action for each.

**Handover.** 000742 (via 000060), 000784 (inlined in 000090/000124/000126) and 000785 (via
000188/000190) are fixed at NpActor-unit call sites, so they go to the NpActor session. The handle rows
000929, 000933, 000983, 000993 and 000995 are this block's.

**Unreached rows.** 105 of the 156 rows are hit by no staged-pair target on the oracle side.
- 46 rows (17,939 B) are step-only. 45 of them (16,991 B) are missing or partial.
- Visualisation (000766, 000869, 000907) is reachable only through the stubbed `NxScene::visualize`.
- Rows that are missing but reached by the existing targets on the oracle side: 000722 and 000748 (every
  actor creation), 000973 (every box creation) and the mass-frame negated translate 000841.
- The oracle also reaches the box mass chain 000947, 000849, 000829, 000831, 000833, 000839 and 000847 on
  every dynamic box creation. The candidate never calls it: `nxActorComputeMass` computes the mass inline.
