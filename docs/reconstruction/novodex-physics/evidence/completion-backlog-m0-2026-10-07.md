# M0 completion backlog and gate baseline

The generated `completion-backlog.json` contains one entry per code row and keeps code separate from data objects and classified artifacts. It records candidate implementation mapping separately from oracle-source correspondence, names export reachability only where supported, and treats direct and indirect call uncertainty separately. Candidate map references found in dynamic proof text are labeled as recorded references, not as standalone proof of linkage. Textual control-word literals and simulation mentions are leads, not proof of runtime reachability. Each row points to its closure-ledger disposition and an actionable next packet.

Run the report from the repository root with:

```powershell
python docs/reconstruction/novodex-physics/tools/report_completion.py --repo-root .
```

The report records these input hashes: inventory `006cae55f5c9429a5b8c5a1278d418d9066e2842007b2cbc3ac7cdb9711873e9`, pinned Capstone manifest `869d38285f364bb1df5ea4b6558e43e334de5c6a44b97ee842198690557c44b0`, generator `fea7b52851980c1964d8a2abae9c0d0443685092c13bc6c542e887e957ce8336`, and oracle dependency graph `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. It also hashes each phase closure ledger independently so a closure-only edit changes the recorded provenance.

The baseline is 6,338 function rows: 2,787 code rows totaling 938,498 bytes and 3,551 classified artifact rows totaling 116,182 bytes. The code rows have no terminally closed rows yet; 1,669 have candidate implementation mappings and another 505 have source correspondence only. The 5,138 data objects totaling 176,884 bytes are classified. The inventory validator reports zero unexplained rows. Phase 5's gate is green at 2,303/2,303 registered assertions while its closure ledger remains partial at 65 closed and 140 deferred; Phase 3's current gate baseline is 532/532, with 63 closed and 329 deferred. These gate results do not mean either census is complete.

The Viewer test design already registered on main runs 48 CTest entries across all 39 available scenes. The 2026-10-07 Release sweep reports 43 passes and five established, signature-verified pinned-oracle asset skips, including the focused Viewer physics checks (`gates/phase5.json`). The skips remain visible outcomes and are not counted as passes.

The three OPCODE code rows `phys_fn_005493`, `phys_fn_005517`, and `phys_fn_005523` remain code rows, not artifacts. Their candidate source files and symbols are now explicit in `inventory.json`; the overlay evidence does not close them because no row-specific falsifying mutation has been recorded.

`NxPhysicsSceneAllocatorTests` remains a CMake executable but no current phase runs it. Removing its stale gate-registry entry restores the registry-to-phase consistency check; this baseline does not claim that allocator fixture is phase-gated.

## Reproducibility dependency

The validator and vendored-source verification still require the pinned third-party source snapshot under `.analysis/novodex-physics/thirdparty`. In this worktree, `.analysis` is an ignored junction to `D:\github\Novodex\.analysis`, so a clean checkout cannot reproduce those checks without arranging that snapshot. This M0 baseline records the dependency; portable acquisition or a documented setup remains open.
