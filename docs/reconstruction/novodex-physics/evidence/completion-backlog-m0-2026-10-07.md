# M0 completion backlog and gate baseline

The generated `completion-backlog.json` contains one entry per code row and keeps code separate from data objects and classified artifacts. It records candidate implementation mapping separately from oracle-source correspondence, names export reachability only where supported, and treats direct and indirect call uncertainty separately. Candidate map references found in dynamic proof text are labeled as recorded references, not as standalone proof of linkage. Textual control-word literals and simulation mentions are leads, not proof of runtime reachability. Each row points to its closure-ledger disposition and an actionable next packet.

Run the report from the repository root with:

```powershell
python docs/reconstruction/novodex-physics/tools/report_completion.py --repo-root .
```

The 2026-10-08 refresh records these input hashes: inventory `6f12cae7ad77b1e6b13a5e45d991ea55858d37a021a399178381789a6beb9346`, pinned Capstone manifest `869d38285f364bb1df5ea4b6558e43e334de5c6a44b97ee842198690557c44b0`, generator `3695dd2e6b069def544c821b383613af92b49f565cb2603e57304436832ea622`, and oracle dependency graph `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. It also hashes each phase closure ledger independently so a closure-only edit changes the recorded provenance; the refreshed Phase 7 closure-ledger hash is `7faca2c0c86fba327359a55901cd202c2947dd8971de505199346520d2575264`.

The refreshed baseline is 6,338 function rows: 2,787 code rows totaling 938,498 bytes and 3,551 classified artifact rows totaling 116,182 bytes. The code rows have no terminally closed rows yet; 1,672 have candidate implementation mappings and another 504 have source correspondence only. The 5,138 data objects totaling 176,884 bytes are classified. The inventory validator reports zero unexplained rows. The latest recorded Phase 5 gate is green at 2,307/2,307 assertions with 110 closed / 95 deferred rows; Phase 6 is green at 1,065/1,065; Phase 3 is green at 532/532 with 63 closed / 329 deferred rows; Phase 7 is green at 1,386/1,386 with 21 closed / 540 deferred rows. These gates and classifications remain partial and do not close the census.

The Viewer test design already registered on main runs 48 CTest entries across all 39 available scenes. The 2026-10-07 Release sweep reports 43 passes and five established, signature-verified pinned-oracle asset skips, including the focused Viewer physics checks (`gates/phase5.json`). The skips remain visible outcomes and are not counted as passes.

The three OPCODE code rows `phys_fn_005493`, `phys_fn_005517`, and `phys_fn_005523` remain code rows, not artifacts. Their candidate source files and symbols are now explicit in `inventory.json`; the overlay evidence does not close them because no row-specific falsifying mutation has been recorded.

`NxPhysicsSceneAllocatorTests` remains a CMake executable but no current phase runs it. Removing its stale gate-registry entry restores the registry-to-phase consistency check; this baseline does not claim that allocator fixture is phase-gated.

## Reproducibility dependency

The validator and vendored-source verification still require the pinned third-party source snapshot under `.analysis/novodex-physics/thirdparty`. In this worktree, `.analysis` is an ignored junction to `D:\github\Novodex\.analysis`, so a clean checkout cannot reproduce those checks without arranging that snapshot. This M0 baseline records the dependency; portable acquisition or a documented setup remains open.

## Refresh and data-structure audit — 2026-10-09

Regenerated `completion-backlog.json` after the Phase 6 closure ledger changed. The refresh updated its recorded Phase 6 ledger digest; `test_completion_report.py` now passes all 11 tests, including the CLI snapshot check. `validate_inventory.py` passes with 6,338 function rows, 5,138 data objects, and zero unexplained rows. All 5,138 data objects are in the `classified` terminal state, each has a structural proof accepted by the validator's type-to-proof vocabulary, and the census contains 11 recognized data types. The recorded distribution is: code-addressed globals 2,367; Ghidra-typed data 1,308; strings 1,128; dispatch tables 131; import address table entries 110; pointer slots 43; switch tables 42; derived switch tables 6; and one each of ASCII blob, export directory, and relocation metadata. No classification inconsistency was found; reopen a row only with concrete contradictory evidence.

The generated backlog was refreshed independently of gate state; all 2,787 code rows remain open. The ignored `.analysis` source-snapshot dependency above is still unresolved.

## Fresh gate baseline — 2026-10-09

After the aggregate coverage-floor fix was merged, fresh Win32 runs from the main checkout passed the `completed` selection (Phases 1–5, 3,224/3,224 assertions), standalone Phase 5 (2,597/2,597), Phase 6 (1,237/1,237), and Phase 7 (1,428/1,428). The Phase 7 run exercised the approved public three-step spring/damper fixture on both oracle and candidate pairs; its second- and third-step velocity words were `3e9277f6` and `3f092d93`. Logs are retained under `D:\github\Novodex\build\m0-current-main-clean\` as `completed-main-postmerge.log`, `phase5-standalone-main-20261009.log`, `phase6-main-postmerge.log`, and `phase7-main-postmerge.log`. These are partial phase gates, not full-census closure. The all-scenes Viewer sweep is recorded separately in `evidence/viewer-all-scenes-effector-order-2026-10-09.md` (48 tests, 43 passed, five known asset skips across all 39 available scenes).

## Clean-checkout source pinning resolution — 2026-10-09

The earlier reproducibility note above records the observed limitation at the time: `validate_inventory.py` and `verify_vendored_sources.py` depended on the ignored `.analysis/novodex-physics/thirdparty` snapshot. That dependency is now removed. Both tools locate the committed `External/qhull/upstream/src` and `External/opcode/upstream/Opcode` trees; the inventory validator checks correspondence against those roots, while the vendor checker verifies each upstream file and rejects missing or extra files using the checked-in `UPSTREAM-MANIFEST.sha256` lists. The manifest bytes themselves are pinned to SHA-256 `e8fa2ad3cdeee2bc48d043683ecdc50271b5f3da170dc67a4697fbb46b4cb0dc` (OPCODE) and `c6cb967e5b2d5f8e06c7ef074dd9b7b2133926441ffb8625347c212835ce9ee1` (qhull). The real-tree unit test no longer skips if these tracked sources are absent, and a mutation test proves that a changed upstream file is rejected.

The resolution was checked from a `git archive` extraction of the source tree with no `.analysis` directory: inventory validation reported 6,338 function rows, 5,138 data objects, and zero unexplained rows; vendored-source verification checked 152 upstream files and found 38 local modifications; `test_vendored_sources.py` passed 16 tests and `test_inventory.py` passed 251 tests. This makes the check reproducible from committed files alone. The older dependency note above is retained as historical context; it is superseded by this resolution.
