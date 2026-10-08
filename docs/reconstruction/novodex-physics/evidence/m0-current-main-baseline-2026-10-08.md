# M0 current-main baseline and accounting reconciliation

Base snapshot: `cdf03673513c7a20cbc6af96a48492f611617487` (`main`). Validation ran in an isolated Win32 worktree. The existing main worktree's local files were left untouched.

## Reproduced baseline

- A fresh Visual Studio 18 2026 / Win32 Release CMake configure and build produced `NxFoundation.dll` and `NxPhysics.dll`.
- `run_phase_gate.ps1 -Phase 5` passed all 18 staged targets and all 2,309 registered coverage assertions. It verified the oracle and candidate DLL identities, public headers, vendored sources, inventory, and candidate-side evidence before the differential runs.
- `validate_inventory.py` passed: 6,338 function rows, 5,138 data objects, zero unexplained executable bytes. Closure-ledger counts were Phase 2: 60/83, Phase 3: 63/329, Phase 4: 29/1,024, Phase 5: 124/81, Phase 6: 5/428, and Phase 7: 21/540 (closed/deferred).
- `verify_vendored_sources.py` passed over 148 pinned upstream files and 38 local overlay files. The copied `.analysis/novodex-physics/thirdparty` inputs matched the pinned source correspondence.
- The Physics public-header manifest passed for all 80 files. No public header changed.
- The all-scenes Viewer selection on this same main snapshot passed 48/48 registered tests, including every one of the 39 available scene scripts: 43 passed and five known pinned-oracle asset cases skipped on their established signatures.

## Accounting reconciliation

The three rows called out by the older completion-plan draft are already classified as Phase 4 product code in this snapshot, so no artifact-to-code reclassification was needed:

- `phys_fn_005493` is `Segment::SquareDistance`, with a local OPCODE overlay and exact direct-linked grid/wide comparisons. It remains open because there is no row-specific falsifying mutation.
- `phys_fn_005517` is `AABBTreeNode::_BuildHierarchy` and `phys_fn_005523` is `AABBTree::Build`. Both map to `External/opcode/novodex/OPC_AABBTree.cpp`; existing evidence establishes the shared overlay's layout effect, but not a row-specific mutation. The inventory's empty per-row `source` and `static_proof` fields remain explicit proof gaps.

The generated work-unit map was stale after eight Phase 5 row-state promotions made after its last refresh. Ownership was already unique and unchanged; only the aggregate counts in `gap:SceneRaycast.cpp..CapsuleShape.cpp` were stale. Regeneration updated its counts from 28 to 36 dynamically gated rows and from 112 to 104 reconstructed rows (4,136 to 4,884 and 43,338 to 42,590 bytes respectively). The eight rows are `000927`, `000935`, `000937`, `000939`, `000941`, `000945`, `000947`, and `000975`.

The completion-backlog generator also produced platform-dependent output bytes on Windows: parsed JSON matched, but default text-mode writing converted LF to CRLF. The writer now pins LF, the checked-in backlog has been regenerated to include the current generator fingerprint, and a regression test checks exact output bytes. The full metadata/tool suite passes 776 tests and 699 subtests.

## Remaining program state

This is an intermediate baseline, not DLL completion. The inventory accounts for all 5,138 data objects with no unexplained entries, but this packet did not attempt semantic reconstruction of every table or constant. The next plan packet is M1: reproduce the full object-model gate state and take the smallest remaining Phase 5 behavior gap through a public-DLL test and mutation.
