# Type-3 pruner tree visualization (`phys_fn_001939`)

IDA at RVA `0x0004ba30` shows a recursive AABB-tree walk. Each 36-byte node contributes its 24-byte bounds through `NxDebugRenderable::addAABB` (vtable slot 11), color `0xffff0000`, frame `false`; the low child-pointer tag bit is cleared before recursing, and the loop then advances to the sibling at child+36. `phys_fn_001978` calls it for pruner slot 3 when dynamic collision visualization is enabled.

`nxSceneVisualizeCollisionPruners` now lazily builds the slot-3 tree when absent and routes it through the same `AABBTree::Walk` callback as the static tree. The public Scene differential aliases its already-built type-0 tree into slot 3, enabling the oracle path without fabricating the type-3 object layout, and asserts 36 red lines (three AABBs). The baseline candidate emits 0; the oracle emits 36.

A candidate-only mutation that suppresses the type-3 draw call is rejected: `oracle_exit=0`, `candidate_exit=1`, `stdout_delta=46`, and the marker changes from 36 to 0. After restoration, oracle and candidate both exit 0, `stdout_delta=0`, and stderr matches exactly. The fixture runs last in a fresh scene, keeping earlier visualization coverage stable. Raw transcripts and hashes are in [the evidence directory](raw/phase7-type3-pruner-visualize-001939-20261017/).


The fresh full Phase 7 gate passed at the updated floor (`coverage_assertions_evaluated=2984 floor=2984`, `phase_gate=7 status=pass`). Its raw output is retained in the evidence directory.
