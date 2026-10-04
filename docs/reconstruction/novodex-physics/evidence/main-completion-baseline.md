# NxPhysics main execution baseline

Date: 2026-09-30
Source baseline: `main` at `b942f0120450d86c29f41ed43fd5025d85b8251e`.
Workspace: managed worktree `C:\Users\raynorpat\.codex\worktrees\nxphysics-main-plan\Novodex`.
Public Physics headers: unchanged; 80-file hash check passes.

## Reproducible starting point

A fresh Visual Studio 18 2026 / Win32 Release build of `NxPhysics` succeeds. Candidate `NxPhysics.dll` SHA-256 at the baseline is `656b302ef3a0485098497f3bda54d36b313224d769812280ec083a2701c757ed`; pinned oracle SHA-256 is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Inventory validation passes for 6,338 function rows and 5,138 data rows, with zero unexplained rows. The full reconstruction tooling suite passes 770 tests.

The first M0 regression test failed against the committed work-unit map because it contained 143 unit records but only 112 unique names. Regeneration from the pinned inventory, Ghidra source attribution, and dependency graph produces 109 unique units: 60 named units and 49 gaps. All 1,876 ambiguous rows are assigned once, and every executable row has exactly one owner. The committed map is now regenerated and the regression test is green.

## Baseline gates on this source revision

- Phase 2: pass.
- Phase 3: pass (359/359 coverage assertions).
- Phase 4: fail. `NxPhysicsThirdPartyTests` exits with Windows status `0xC0000374` (heap corruption) before printing its registered `prunable_pruner` coverage line. The runner reports that required oracle-side line as missing.
- Phase 5: red. All 13 staged-pair targets return identical output; 2,037/2,037 coverage assertions run. The oracle-differential `NxPhysicsObjectLayoutTests` reports `layout candidate mismatches=1` and deliberately returns failure with “the Phase 5 reconstruction is incomplete; this gate is RED on purpose.” The shape-vtable oracle differential itself passes its 626 cases.
- Phase 6: pass (856/856 coverage assertions).
- Phase 7: pass on the original registry (1,129/1,129 coverage assertions). A new simulation target has since been registered, so this baseline result predates that addition.

Build, gate, verifier, and test outputs are retained under the ignored local `build/` directory. Inventory validation needs the local staged upstream source archive; this worktree accesses the already staged copy through a temporary read-only junction at `.analysis`.

## First simulation dependency packet

The public stepping worker path is `NxScene::simulate` vtable dispatch through `phys_fn_002400` at `0x5b9e0`, then `phys_fn_000659` at `0x13c40`, then the pre-tick/simulation pipeline `phys_fn_000655` at `0x137e0`. The stepper saves and restores the x87 control word and runs the physics step at 64-bit x87 precision with round-toward-zero. `000655` has three virtual call sites whose receivers still need concrete type resolution. The full direct-call closure seeded at `002400`, `000659`, and `000655` contains 176 rows; 69 rows remain marked discovered, totaling 20,243 bytes, before resolving those virtual calls. The row-by-row backlog, including phase/state, work-unit owner, direct callees, and recorded indirect calls, is [main-simulation-direct-closure.csv](main-simulation-direct-closure.csv).

The first public fixture creates a dynamic sphere at `(0,10,0)` in gravity `(0,-9.81,0)`, then runs eight blocking `simulate(0.125)`, `checkResults`, and `fetchResults` cycles. The oracle becomes active on the second cycle and reaches position bits `00000000.40c7d67b.00000000` and velocity bits `00000000.c108082a.00000000` after cycle 7. The current candidate leaves the body unmoved and returns false from both result calls every cycle. This gives M1 an oracle-grounded red test without substituting a guessed integrator.

The executable backlog is therefore not “write 69 functions and stop.” Each row in the closure must be checked for current source, indirect target, data/global ownership, test route, and its place in the broadphase/contact/island/solver/integration/report sequence. The new gravity fixture opens the path; static contact and two-body collision remain the next expansion gates.

## M1 red-test checkpoint

`NxPhysicsSimulationTests` is now registered in Phase 7 with 17 oracle-captured assertions (initial state plus eight result/state pairs). The pre-implementation gate run reports `NxPhysicsSimulationTests oracle_exit=0 candidate_exit=0 stdout_delta=30`; the first required candidate-side line is missing because candidate `checkResults` and `fetchResults` both return false. All other Phase 7 registered staged-pair targets still report zero stdout delta. The test compiles and both isolated pairs load with no rejected modules. This is the expected TDD red state for M1.
