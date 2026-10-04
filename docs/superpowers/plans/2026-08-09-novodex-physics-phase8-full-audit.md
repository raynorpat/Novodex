# Novodex Physics Phase 8 Full Semantic Audit Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Prove the entire pinned Physics oracle census is reconstructed, build one retained Win32 Release candidate, run all ABI/static/differential/trajectory gates on that exact binary, and complete a controlled Unreal_3 consumer deployment with exact restoration.

**Architecture:** A checked-in final runner starts from clean recorded commits, regenerates evidence, performs one clean build, pins the candidate hash, and passes that same artifact through every gate. Deployment is last and uses transactional backup/restore with module/hash evidence.

**Tech Stack:** PowerShell orchestration; Python inventory/PE/static-proof audits; MSVC Win32 Release; all C++ differential harnesses; Ghidra/Capstone regeneration; Unreal_3 DemoGame and a minimal SDK consumer.

---

### Task 1: Audit complete census and source provenance

**Files:**
- Create: `docs/reconstruction/novodex-physics/tools/verify_static_proofs.py`
- Create: `docs/reconstruction/novodex-physics/tools/tests/test_static_proofs.py`
- Create: `docs/reconstruction/novodex-physics/gates/static-proof.json`
- Create: `docs/reconstruction/novodex-physics/gates/source-map.json`

- [ ] Write failing tests that reject any non-closed row, missing source mapping, missing Ghidra/Capstone reference, stale evidence hash, unknown vtable slot, unexplained field used by code, unowned significant data, or compiler artifact without classification proof.
- [ ] Implement the auditor so every product source function maps to one or more stable oracle IDs and every non-artifact oracle function maps to a concrete source function.
- [ ] Regenerate PE/Ghidra/Capstone evidence from clean inputs and require deterministic normalized hashes.
- [ ] Run inventory and static-proof validators; resolve every failure through evidence or production changes in its owning phase, never by suppressing a row.
- [ ] Take the residue of the Phase 2 exception list. Phase 2 closed 56 of the 163 function rows it owns and none of its 1051 data objects; `gates/phase2-closure.json` defers the other 1155. Of those, 86 name the phases that can discharge them in `driving_phases` and **1069 name none** — all 1051 data objects, 11 of the 78 homeless shared rows, the 6 rows unreachable through the Phase 2 public API, and the one row with no mutation of its own. No closure schema lets any phase close a data object (`DATA_PROOF_KINDS` is empty), so the whole data half arrives here by construction. Assign each of the 1069 an owner or close it; do not let an empty `driving_phases` read as nobody's work.
- [ ] Take the residue of the Phase 3 exception list. Phase 3 closed 61 of the 441 function rows it owns and none of its 102 data objects; `gates/phase3-closure.json` defers the other 482. Twenty name a later phase, one — `phys_fn_001901` — is reconstructed and driven with no mutation aimed at its own body, 102 are data objects that no closure schema lets any phase close, and **360 say only that Phase 3 ran out of dispatches** and make no per-row reachability claim at all, deliberately: this component dispatches through two function-pointer matrices and per-type vtable slots, and a continuation row is not a call target, so four separate counting methods in this programme have each answered a narrower question than the one asked. Walk that residue properly — fold every continuation row into its entry before walking, and follow indirect edges — and assign each row an owner or close it.
- [ ] Take the residue of the Phase 4 exception list, which is the largest in the programme. Phase 4 closed 26 of the 1,050 code rows it owns; its 2,813 data objects and 87 compiler artifacts are `classified`. `gates/phase4-closure.json` defers 1,024: 315 say only that Phase 4 did not reconstruct the row, 108 are reconstructed and unfalsified, and **601 are vendored third-party rows that were never reconstructed at all** -- 341,680 bytes of qhull 2003.1 and OPCODE 1.3 that compile into the libraries `NxPhysics.dll` links. 595 of the 601 have had nothing aimed at them. Decide, in writing, whether a vendored row can ever be `closed` in this census and on what evidence; the answer governs a third of the function rows in the image.
- [ ] Discharge the `vendored_rows_are_present_not_proven` escalation in `gates/phase4.json`. Of 722 vendored rows, 3 carry a mutation and a measured delta, 32 are `reconstructed` with no mutation aimed at them, 601 carry `vendored_not_falsified`, and 86 are `classified` artifacts -- three of which, `phys_fn_005493`, `phys_fn_005517` and `phys_fn_005523`, are real functions on a false artifact proof, one of them driven and divergent. Retype those three, then either drive and mutate the vendored rows or record the programme's end state as "third-party code is vendored from identified upstream and verified byte-identical to it, not differentially proven".
- [ ] Discharge the `opcode_modification_list_not_closed` escalation in `gates/phase4.json`. `External/opcode/MODIFICATIONS.md` lists every NovodeX modification this project has *established* and says the list is not proven closed; `External/qhull/MODIFICATIONS.md` does not say so and should. `OPC_AABBTree.cpp` was vendored stock while carrying NovodeX code and was found by a reviewer reading the disassembly. Build the check that would find the next one, or record that none exists.
- [ ] Discharge the `use_minmax_representation_undecided` escalation in `gates/phase4.json`. `USE_MINMAX` is undefined in the vendored tree, so `NxOpcode` compiles the centre/extents `AABB` while the image compiles min/max, and `sizeof` is 24 either way. Switch it with a differential that can tell whether the results moved the right way.
- [ ] Discharge the `layout_assertions_cannot_close_a_row` escalation in `gates/phase4.json`. Deleting `External/opcode/novodex/OPC_AABBTree.cpp` fails five registered layout assertions aimed at `phys_fn_005517` and `phys_fn_005523`, and no detection form can carry it -- and both rows are `classified` artifacts on this census, outside every ledger. Either give the schema a detection form for a layout assertion, with a test that it cannot be spent on a row the assertion does not name, or record that layout-only falsifications never close rows.
- [ ] Discharge the `third_party_container_rows_not_reclosed` escalation in `gates/phase4.json`. Only `phys_fn_004840` of the four `Ice/IceContainer.cpp` rows Phase 2 closed on static proofs was re-closed; `phys_fn_004836`, `phys_fn_004846` and `phys_fn_004847` have no mutation of their own. Aim one at each, or record that the Phase 2 static proofs are all the evidence they will ever have.
- [ ] Discharge the `container_reconstructed_twice` escalation in `gates/phase4.json`. `SdkContainer` in `Physics/src/Containers.cpp` and the vendored `IceContainer.cpp` are two reconstructions of the same four rows, both in `NxPhysics.dll`. Decide which survives and delete the other with its gate re-pointed.
- [ ] Discharge the `retyped_opcode_rows_lack_attribution` escalation in `gates/phase4.json`. P4 Task 3 retyped 63 rows from `compiler_artifact` to `code` -- **35** reached by a direct call and **28** through the OPCODE vtable pool, the split `evidence/phase4-artifact-retype.csv` records and a test asserts -- and changed nothing else. Attribute all 63 to a library or to NovodeX, and reconstruct `phys_fn_005380` and `phys_fn_005386`, which are still stubs.
- [ ] Discharge the `mesh_column_blocks_phase3_mesh_deferrals` escalation in `gates/phase4.json`. **Thirteen** Phase 3 deferrals carry `blocked_on_type: TriangleMesh` and `driving_phases: [4]`, a phase that closed without them. Phase 5 inherits them; this audit inherits whatever Phase 5 does not discharge. Assign each an owner or close it.
- [ ] Discharge the `deferrals_left_on_passed_phase_4` escalation in `gates/phase4.json`. Seven deferrals named only Phase 4, which passed without them. `phys_fn_001751`, matrix B [BOX][CAPSULE], is this audit's outright: it is blocked on `phys_fn_001670`, `001684` and `001688`, Phase 4 rows no earlier phase reconstructs -- reconstruct them or re-point the row. The other six are Phase 5's; take whatever Phase 5 does not discharge. The escalation also records eighteen Phase 2 deferrals whose driving phases have all passed and that it does not carry; assign each an owner.
- [ ] Discharge the `phase4_never_driven_under_simulate_word` escalation in `gates/phase4.json`, which Phase 7 also carries. No Phase 4 row was ever measured under `0x0f7f`; state, for each of the 26 closed Phase 4 rows, which word it executes under and whether it has ever been gated under that word.
- [ ] Emit counts by proof level so direct dynamic proof, covered-caller proof, and static-only compiler artifact classification remain distinct.
- [ ] Discharge the `codegen_spill_divergence_pinned` escalation in `gates/phase3.json`. Two Phase 3 rows are closed on algorithmic agreement under a control word they do not execute under, with the divergence pinned: `phys_fn_001775` at 43 words and `phys_fn_001690` at 662, both from where MSVC spills a `double` and truncates a 64-bit significand to 53. The class is not uniformly unfixable — `NxBuildSmoothNormals` went from 24 to 0 and the `box/box` leaf from 88 to 0 once the carrier was found — and `phys_fn_001010`'s 242 belongs to its callee `NxRayCapsuleIntersect`, a closed Task 2 export with 37 qword stores, so `Geometry.cpp` is an open front. Either take the two counts to zero or record in writing that the programme's end state is "semantically identical, differentially proven, and bit-exact except where 2003 and 2026 codegen disagree" rather than byte-identical behaviour.
- [ ] Discharge the `narrow_phase_oracle_defects_to_reproduce` escalation in `gates/phase3.json`. `NxSeparatingAxis` with `fullTest` clear reads uninitialised stack at `esp+0x8c..0xa0` and is not a function of its arguments — confirmed dynamically against the shipped DLL, and the reconstruction seeds the residue with `-FLT_MAX` because that is the only value that cannot move the argmax — and the swept capsule/capsule contact normal is uninitialised stack because `phys_fn_001010` never writes `hit.worldNormal` while `phys_fn_001775` hands the emitter `hit+0x10` anyway. Record both as shipped behaviour and gate them as such; do not fix either into a divergence.
- [ ] Discharge the `box_box_contact_frame_overflow` escalation in `gates/phase3.json`. `phys_fn_001741` has no contact cap and returns up to eighteen contacts into `phys_fn_001749`'s sixteen-slot frame, where `+0x138` is the return address with nothing between and no `/GS` cookie — measured at about 2 in 32,000 aimed calls with the SAT-selected face and ordinary geometry, through `box_shim`, which drives the shim with exactly the entry's arguments into eighty slots instead of sixteen. Third confirmed oracle defect, beside `NxArray::insert` and `NxSeparatingAxis`: reproduce it, do not correct it.
- [ ] Discharge the `contact_sink_warm_start_axis_survives_reset` escalation in `gates/phase3.json`. `sink+0xe8` is a warm-start separating-axis index the reset at `0x0005b620` never clears, so a box pair's result depends on what was tested before it in the same step, and a warm byte makes `phys_fn_001745` skip nine edge-axis tests entirely. The trajectory gates must reproduce the carry rather than normalise it away.
- [ ] Discharge the `simulation_control_word_scope` escalation in `gates/phase3.json`. `Scene::simulate` installs `_PC_64 | _RC_CHOP` (`0x0f7f`) and code reached through the exported API sees `0x027f`; five of Task 2's closed exports turned out to run inside that window only after Phase 3 recovered the dispatch matrix. Every row in the census must record which word it executes under and be gated under that word, and the audit must state whether any row is still closed only under a word it does not execute under.
- [ ] Discharge the `static_proof_closures_have_no_oracle_side` escalation in `gates/phase2.json`. Ten Phase 2 rows are closed against checks written from the disassembly with no oracle side at all, and the three lock rows `phys_fn_002362`, `phys_fn_002364` and `phys_fn_002366` have never had their dynamic behaviour observed against the shipped DLL. Reach each of the ten through the public API against a staged pair and replace the derived expectation with a measured one, or record in writing why it cannot be reached. The store at `0x000b400e` is separately recorded as measured but unfalsifiable from outside; that mutation was applied and not caught.
- [ ] Commit the completed auditor, tests, and regenerated static manifests as `docs: add Physics full-census audit`; verify the evidence worktree is clean before Task 2.

Verification:

```powershell
$env:PYTHONDONTWRITEBYTECODE='1'
python -m unittest discover -s docs/reconstruction/novodex-physics/tools/tests -v
python docs/reconstruction/novodex-physics/tools/validate_inventory.py docs/reconstruction/novodex-physics/inventory.json
python docs/reconstruction/novodex-physics/tools/verify_static_proofs.py --inventory docs/reconstruction/novodex-physics/inventory.json --source-root D:\github\Novodex\Physics\src --output docs/reconstruction/novodex-physics/gates/static-proof.json
```

Expected: tests pass; all rows closed; unexplained bytes/data/source functions zero.

### Task 2: Build and pin the final candidate

**Files:**
- Create: `D:\github\Novodex\tests\PhysicsConsumerSmokeTests.cpp`
- Modify: `D:\github\Novodex\CMakeLists.txt`
- Create: `docs/reconstruction/novodex-physics/tools/run_final_verification.ps1`
- Create: `docs/reconstruction/novodex-physics/gates/candidate.json`

- [ ] Add a minimal Win32 consumer linked only through the pinned UE3 Physics import contract. It creates the SDK, material, scene, static plane, dynamic box, simulates fixed steps, fetches results, records state/callbacks, and releases in oracle order.
- [ ] Give the final runner two exclusive modes. `-Mode BuildAndVerify` refuses dirty trees, configures `--fresh`, clean-builds `NxFoundation` and `NxPhysics` once, copies both into `D:\FlamingEnt__\Unreal_3\.analysis\novodex-physics\retained`, hashes them, then verifies them. `-Mode VerifyRetained` refuses any build command, reads `candidate.json`, verifies the retained pair hashes, and reruns gates against those exact files. `-NoWrite -OutputRoot D:\FlamingEnt__\Unreal_3\.analysis\novodex-physics\postcommit` keeps post-commit verification artifacts outside Git.
- [ ] Verify Physics candidate machine `0x014c`, PE32 magic `0x010b`, SDK version resource, dependency imports, and all 41 named oracle exports with ordinals exactly equal to the oracle export table. Validate the import library independently; it may not weaken ordinal requirements.
- [ ] Make `candidate.json` record absolute paths, SHA-256 values, sizes, PE identities, implementation/evidence commits, and build command for both retained candidate DLLs. Every later command rejects either hash changing.
- [ ] Commit the CMake/consumer production change as `physics: add final SDK consumer gate`; commit the runner and candidate-schema tests as `docs: add reproducible Physics final runner`. Verify both worktrees are clean. Do not perform final candidate build or deployment yet.

### Task 3: Wire the complete differential and trajectory suite into the final runner

**Files:**
- Create: `docs/reconstruction/novodex-physics/gates/differential.json`
- Create: `docs/reconstruction/novodex-physics/gates/trajectory.json`
- Create: `docs/reconstruction/novodex-physics/dumps/final-verification.raw.txt`

- [ ] Enumerate every registered test mode and make the runner assert each closed dynamic-proof inventory row references at least one mode.
- [ ] Make the runner stage hash-verified oracle and retained candidate DLL-pair directories, launch every mode exactly once against each pair in fresh processes, and capture stdout/stderr/exit code plus both loaded module paths/hashes. Reject any Novodex or non-system dependency outside its selected pair; permit only a recorded allowlist of Windows modules whose resolved paths are under the Windows system directory.
- [ ] Require exact normalized outputs except for explicitly approved tolerance fields; independently validate each tolerance artifact against repeated oracle measurements.
- [ ] Discharge the `foundation_mat34_multiply_ulp` escalation in `gates/phase2.json`. Gate `addCircle` and the hash-pinned inline `NxMat34::multiply` under a non-identity rotation and decide whether one ULP of x87 scheduling is in scope for the Foundation. Reproduction: `NxPhysicsCoreClusterTests` `step=sphere_arguments` draws 120 lines, of which line 79 `p0.z`, line 102 `p0.z` and line 138 `p0.z` differ between the shipped and the rebuilt `NxFoundation.dll`; the identity transform reproduces nothing.
- [ ] Discharge the `nxarray_insert_at_end_overflow` escalation in `gates/phase2.json`. `NxArray::insert(end(), n, x)` reserves `size()+n` and writes `size()+2n`, so `NxPhysicsSDK::setMaterialAtIndex(index, material)` past the end corrupts the heap whenever `size()+n <= capacity() < size()+2n` — measured from 11 materials at capacity 14, index 13 and index 14 fault while 12 and 16 complete. `NxArray.h` is hash-pinned, so the candidate reproduces it exactly: record it as shipped behaviour and gate it as such. Do not fix it into a divergence.
- [ ] Restore the loaded-module and import-closure lines to the compared region. Phase 2 excluded them from `run_differential.ps1` because a candidate built with a modern MSVC cannot reproduce the 2004 oracle's 3-DLL, 110-import closure; import-table fidelity is a Phase 8 claim, so both must be compared here as exact equalities rather than reported outside the comparison.
- [ ] Wire all fixed multi-frame scenarios, compare every recorded step, and reject skipped frames, missing callbacks, output truncation, leaks, timeouts, or nondeterministic ordering.
- [ ] Append exact commands, child exit codes, hashes, and summaries to the raw transcript; the runner exits nonzero on any mismatch.
- [ ] Commit the completed runner wiring and its command-registry tests as `docs: wire complete Physics differential audit`; verify the evidence worktree is clean. Generated gate files are created only by Task 5.

Task 5 expected result: all modes have two successful processes; mismatch count zero; both candidate hashes unchanged.

### Task 4: Perform controlled UE3 consumer deployment

**Files:**
- Create: `docs/reconstruction/novodex-physics/tools/run_consumer_smoke.ps1`
- Create: `docs/reconstruction/novodex-physics/gates/consumer.json`
- Create: `docs/reconstruction/novodex-physics/dumps/consumer/`

- [ ] Make the script reject a running DemoGame process, either existing DLL backup, either unexpected installed oracle hash, or either candidate hash different from `candidate.json`.
- [ ] Implement one transaction for both DLLs: copy installed `NxFoundation.dll` and `NxPhysics.dll` to distinct `.oracle.bak` files, verify both oracle hashes, deploy both retained candidates, and verify both installed candidate hashes before launch.
- [ ] Launch the candidate DemoGame process with an absolute log. Require module capture showing both candidate hashes, `NxCreatePhysicsSDK` success, scene creation, and actual simulated Physics activity through the selected map or deterministic startup consumer path. Stop and reap that exact launched process before changing either DLL.
- [ ] Restore both oracle DLLs from their verified backups, verify both installed oracle hashes, then launch the oracle baseline as a fresh process with the same command. Stop and reap the exact baseline process and compare normalized milestones, physics callbacks/state, and any later baseline failure behavior.
- [ ] In `finally`, terminate only a process launched by this script, restore both verified oracle backups whenever either installed hash is not the oracle, verify both oracle hashes, remove both backups, and prove no backup/process remains.
- [ ] Set `consumer.status` to pass only if binding and representative physics activity occurred; a load-only observation is insufficient.
- [ ] Add transaction-state unit tests for every failure point and commit the script/tests as `docs: add transactional Physics consumer audit`. Do not deploy in this task.

### Task 5: Close all gates and commit

**Files:**
- Create: `docs/reconstruction/novodex-physics/gates/final.json`
- Create: `docs/reconstruction/novodex-physics/closeout.md`
- Modify: `docs/reconstruction/novodex-physics/program.json`
- Modify: `docs/reconstruction/novodex-physics/inventory.json`

- [ ] From clean implementation and evidence commits, run `-Mode BuildAndVerify` exactly once without deployment. It builds, externally retains, and pins the Foundation/Physics pair and must pass census, public headers, build, exports, ABI, structures, static proofs, isolated-pair identities, all differential modes, all trajectories, Python tests, and scope checks.
- [ ] Run the checked-in consumer script against that exact retained pair. Require the candidate-then-oracle sequence and unconditional two-DLL restoration to pass.
- [ ] Validate both retained candidate hashes still match every gate artifact and the consumer-deployed hashes.
- [ ] Set all phase/global statuses to pass only after direct semantic validation of counts and references; do not rely on text search alone.
- [ ] Write closeout with exact commits, both candidate/oracle hash pairs, function/data/proof counts, test process counts, consumer evidence, restoration, and limitations.
- [ ] Confirm Physics product diff contains only the recursively pinned `Physics/include` transplant, `Physics/src`, Physics-specific tests, CMake target wiring, and required resources; public header bytes equal their pin; unrelated Engine code is unchanged.
- [ ] Verify Novodex is already clean at the exact production commit tested; do not create an empty close-out commit. Commit the final evidence as `docs: close Novodex Physics reconstruction gates` and record the tested production commit in every final artifact.
- [ ] After that evidence commit, run `-Mode VerifyRetained -NoWrite` with an external output root. Require the retained pair hashes, all gates, and restoration state to pass without invoking CMake or changing either repository.

Final verification:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File docs/reconstruction/novodex-physics/tools/run_final_verification.ps1 -Mode VerifyRetained -NoWrite -OutputRoot D:\FlamingEnt__\Unreal_3\.analysis\novodex-physics\postcommit
git -C D:\github\Novodex status --short
git status --short
```

Expected: runner exit `0`; both scoped worktrees clean; oracle restored; no backup or launched consumer process remains.
