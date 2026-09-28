# Phase 4 evidence: every mutation this phase aimed at a census row, and what it measures on this branch

This file exists because a closure ledger may only close a row on a mutation that lands inside that
row's own censused extent and a measured non-zero delta, and because both halves have to be readable
**from the evidence this phase publishes**. `gates/phase4-closure.json` names this file as its
`evidence_file`, and `validate_inventory.py` requires every closed row's stable ID to appear here on a
line that also carries the count the ledger spends.

**Every count below that a closure spends was re-measured on this branch, not transcribed.** The
Phase 4 close was first measured against the implementation tree at `bcf544f`. This branch
(`p4-close-port`, from `de56592`) is about 990 commits past it: `PMap.cpp`, `MemoryStream.cpp`,
`TriangleMesh.cpp`, `CMakeLists.txt` and the asset harness have all moved. So each recorded mutation
was re-applied to the branch's code in a `git archive` copy of `de56592` under the session
scratchpad, the one target it runs through was rebuilt, and the pinned DLL was driven again. The
branch's working tree was never mutated. Where a mutation's anchor had moved, the edit is re-anchored
and says so. Where a count differs from the one first published, both are given.

The two harnesses are oracle differentials. They load the pinned `NxPhysics.dll` (SHA-256
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`, 1,253,376 bytes), re-hash it
themselves, call the recorded internal addresses, and compare against the candidate linked into the
same process. They run once rather than once per pair, so what a mutation reports is a **mismatch
count against the shipped bytes** and never a transcript delta.

---

## 1. Controls

Eight control pairs bracket the re-measurement, each an un-mutated rebuild of the same archive copy:
`CTRL0` before the first mutation, `CTRL1` after the last of the closure set, and `CTRL2`-`CTRL7`
around the three later batches. Every one reads the same thing:

| target | control reading, every run | oracle-side digest, every run |
| --- | --- | --- |
| `NxPhysicsAssetTests` | `asset candidate mismatches=0 mode=differential`, exit 0 | `asset oracle digest=eaefc573 expect_mismatches=0` |
| `NxPhysicsThirdPartyTests` | `thirdparty candidate mismatches=0 layout_failures=0`, exit 0 | `thirdparty oracle digest=b87c3219` |

**The oracle-side digest is unmoved in every mutation run as well.** That is what says a mutation
moved the candidate and not the measurement. The asset digest is `eaefc573`, not the `b114fa73` the
close was first measured under: the canonical line's P4 Task 4 added nine writer probes to the fold
(21 probes became 30). A third-party family's count is read against the control's own count for that
family, which is zero for every family except the two registered `segment_sqrdist` families (§6).

Where the mutations came from:

| pass | first measured against | recorded in |
| --- | --- | --- |
| P4 Task 2a -- the vendored libraries | `bcf544f` | `.superpowers/sdd/p4-task-2a-report.md` §8 on the evidence branch |
| P4 Task 2a fix -- `OPC_AABBTree.cpp` | `bcf544f` | `.superpowers/sdd/p4-task-2a-fix-report.md` |
| P4 Task 2b -- the NovodeX rows inside the OPCODE span | `bcf544f` | `.superpowers/sdd/p4-task-2b-report.md` §4 |
| P4 Task 3 -- the penetration map and the mesh header | `bcf544f` | `evidence/phase4-pmap-reconstruction.md` §4 |
| the canonical line's P4 Task 4 -- the mesh writer | `1337106` | `evidence/phase4-mesh-writer.md` §3 |

---

## 2. P4 Task 2a -- the mutations aimed at the vendored libraries

Eleven mutations were run at `bcf544f`. **Three land inside a census row's extent and report a
mismatch count, and those three were re-measured here**; the other eight are compile failures,
layout-assertion failures or vendoring-verifier failures, which falsify a *modification* and not a
*row*, and are carried as P4 Task 2a recorded them. (P4 Task 2a's heading says "twelve" because it
counted a control row.)

| # | census row the edit sits in | mutation | count spent | re-measured on this branch |
| --- | --- | --- | ---: | --- |
| A | `phys_fn_004838` `0x000b4d90`+66, guard at `0x000b4d93` | `Container::Empty()`'s guard `mGrowthFactor >= 0.0f` weakened to `> 0.0f` | **10** | the `container` family's count as first published, split in the table below: 10 read after `Empty()`, this row's, and 10 read after `~Container()` |
| B | `phys_fn_004840` `0x000b4de0`+174, guard at `0x000b4de4`-`0x000b4df2` | `Container::Resize()`'s guard `<= 0.0f` narrowed to `< 0.0f` | **36** | `container` mismatches=36, as first published |
| G | `phys_fn_005157` `0x000e32c0`+27, store at `0x000e32d0` | `RadixSort::RadixSort` initialises `mDeleteRanks` false | **42** | `radixsort` mismatches=42 as first published, and `radix_setrankbuffers` 6 more -- a family P4 Task 2b added after G was first run -- for a target total of 48 |
| C | header; no row | `mDeleteRanks` deleted from `IceRevisitedRadix.h` | -- | carried: red at compile |
| D | header; no row | the 28 added `AABBTreeBuilder` bytes deleted | -- | carried: `sizeof_AABBTreeOfTrianglesBuilder is 44, the oracle says 72` |
| E | header; no row | `#define OPC_RAYHIT_CALLBACK` restored | -- | carried: red at compile |
| F | header; no row | `GetUsedBytes` declared back ahead of the three added tree virtuals | -- | carried: `GetUsedBytes_slot7 is 9202688, the oracle says 196` |
| V1-V4 | vendoring; no row | one upstream byte edited; an overlay reverted to stock; a marker stripped; the OPCODE licence statement softened | -- | carried: each fails `verify_vendored_sources.py` |

**A's edit is not confined to `phys_fn_004838`.** `Container::Empty()` is one function in the
vendored source and three copies of one statement in the image:
`evidence/phase4-third-party-map/opcode_outside_span_map.csv` records the same guard inlined into
`Container::SetSize` (`phys_fn_004842`, guard at `0x000b4e93`) and `Container::~Container`
(`phys_fn_004846`, guard at `0x000b4f53`). The `container` family reports one count for all four
members it drives. Re-measured with `nxReport`'s eight-line `MISMATCH` print cap lifted in the scratch
copy only, control / mutant / control, the 20 split exactly in two. Each case of the family writes a
64-word tape, and every mismatch is the entries-held flag, oracle 0 and candidate 1, on the ten cases
with growth factor `0.0f` or `-0.0f` and `nb` 1 to 5:

| tape offset | read after | mismatches | whose |
| ---: | --- | ---: | --- |
| 24 | `Empty()` | 10 | `phys_fn_004838`, the edited function -- the count its closure spends |
| 29 | `~Container()` | 10 | `phys_fn_004846`'s inlined copy of the guard |

None lands on the words `SetSize`'s call writes. The second ten are the objection that retired
mutation O: a count produced by another row's code. The ledger closes `phys_fn_004838` on its own 10,
where it spent the family's 20 until the port's review, and closes neither of the other two.

**G's count surfaces through the readers.** The byte the constructor writes is read by
`RadixSort::~RadixSort` (`phys_fn_005159`) and `RadixSort::Resize` (`phys_fn_005161`); the edited code
is the constructor's, so the constructor closes and the readers do not.

---

## 3. The `OPC_AABBTree.cpp` overlay -- a falsification the schema cannot spend

Deleting `External/opcode/novodex/OPC_AABBTree.cpp` from the archive copy, reconfiguring (the overlay
merge happens at configure time; the merged file was confirmed byte-identical to upstream) and
rebuilding `NxPhysicsThirdPartyTests`, re-measured on this branch between two green controls:

    layout AABBTree.extend_pulls_min=0 expected=3225419776 ... FAILED
    layout AABBTree.capture_latch_cleared=1 expected=0 ... FAILED
    layout AABBTree.captured_root_max_y=14093820 expected=1082130432 ... FAILED
    layout AABBTree.inflate_min=0 expected=3204448256 ... FAILED
    layout AABBTree.inflate_max=1082130432 expected=1083179008 ... FAILED
    thirdparty candidate mismatches=0 layout_failures=5          exit 1

`captured_root_max_y` read 12260066 at `bcf544f` and 14093820 here: it is uninitialised stack, which
is what "nothing writes those 28 bytes" means. The five failures are aimed at `phys_fn_005517`
(`AABBTreeNode::_BuildHierarchy`) and `phys_fn_005523` (`AABBTree::Build`). **No detection form in
the closure schema can carry a layout failure**, and on this branch the question does not even
arise inside the ledger: the census types both rows `compiler_artifact` -- on the classification
proof "no product translation unit reaches these bytes", which this measurement contradicts -- so
both stand at `classified` and outside it. `gates/phase4.json` escalates both halves.

---

## 4. P4 Task 2b -- the NovodeX rows inside the OPCODE span

All nineteen re-measured on this branch between green controls, oracle digest `b87c3219` in every
run. Each count below is the family count the mutation moved; where it moved two families both are
given and the ledger spends the larger.

| # | census row and extent | mutation | re-measured |
| --- | --- | --- | --- |
| A | `phys_fn_004874` `0x000b54a0`+76 `Prunable::Prunable` | the two adapter installs dropped | `prunable_ctor` 2 |
| B | `phys_fn_004874` | `mPrunable24` initialised `0` | `prunable_ctor` 1, `prunable_ranges` 16 |
| C | `phys_fn_004874` -- **count spent 16** | `mMember0C.mPrunable = this` dropped | `prunable_ctor` 1, `prunable_ranges` 16 |
| D | `phys_fn_005297` `0x000e7330`+23 `Prunable0C::Prunable0C` -- **count spent 16** | stops zeroing `mMember18` | `prunable_ctor` 3, `prunable_ranges` 16 |
| E | `phys_fn_005299` `0x000e7350`+7 `Prunable0C::~Prunable0C` -- **count spent 1** | writes a member | `prunable_ctor` 1 |
| F | `phys_fn_004876` `0x000b54f0`+40 `Prunable::SetFlags` -- **count spent 236** | early-out becomes a subset test | `prunable_flags` 236 |
| G | `phys_fn_004878` `0x000b5520`+45 `Prunable::ClearFlags` -- **count spent 684** | early-out `!(flags & mFlags)` becomes the subset test `(flags & mFlags) == flags`, as in F | `prunable_flags` 684 |
| H | `phys_fn_004880` `0x000b5550`+25 `Prunable::ToggleFlags` -- **count spent 236** | an already-set early-out added | `prunable_flags` 236 |
| I | `phys_fn_004882` `0x000b5570`+27 `Prunable::SetOrClearFlags` -- **count spent 1056** | the two arms swapped | `prunable_flags` 1056 |
| J | `phys_fn_004884` `0x000b5590`+29 `Prunable::GetWorldAABB` -- **count spent 30** | indexes `[0]` instead of `[mHandle]` | `prunable_pruner` 30 |
| K | `phys_fn_004886` `0x000b55b0`+34 `Prunable::UpdateWorldAABB` -- **count spent 12** | the flag set moved inside the callback's guard | `prunable_pruner` 12 |
| L | `phys_fn_004888` `0x000b55e0`+47 `Prunable::SetPruningType` -- **count spent 2** | the bound 4 becomes 5 | `prunable_ranges` 2 |
| M | `phys_fn_004890` `0x000b5610`+47 `Prunable::SetPruningSection` -- **count spent 2** | the bound 3 becomes 4 | `prunable_ranges` 2 |
| N | `phys_fn_004892` `0x000b5640`+39 `Prunable::~Prunable` -- **count spent 40** | the invalid-handle guard dropped | `prunable_pruner` 40 |
| O | `phys_fn_004894` `0x000b5670`+83 `Prunable::GetUpdatedWorldAABB` -- **count spent 108** | the flag guard dropped | `prunable_pruner` 108 |
| P | `phys_fn_004896` `0x000b56d0`+64 `Prunable::'scalar deleting destructor'` -- **not closed** | an extra virtual declared ahead of `~Prunable` | red **by crash**, exit `0xc0000005` inside `prunable_flags`; no count |
| Q | `phys_fn_005177` `0x000e3ea0`+36 `RadixSort::SetRankBuffers` -- **count spent 2** | `mRanks` stored before `ranks2` is tested | `radix_setrankbuffers` 2 |
| R | `phys_fn_005177` | stops clearing `mDeleteRanks` | `radix_setrankbuffers` 2 |
| S | `phys_fn_005311` `0x000e7670`+31 `Prunable0C::'scalar deleting destructor'` -- **not closed** | an extra virtual declared ahead of `~Prunable0C` | `prunable_ctor` 1 |

`phys_fn_004874` spends 16 from B or C, `phys_fn_005297` spends 16 from D. Every number matches what
P4 Task 2b published at `bcf544f`; neither `Physics/src/opcode/IcePrunable.cpp` nor
`External/opcode` has changed since.

**K came out green the first time P4 Task 2b ran it**, because the family then drove the row with the
world-AABB callback always installed; the harness now drives both readers with the callback null as
well (`prunable_pruner words=20900`), and the row closes on that.

**P and S edit a class declaration, not a row.** Both add a virtual ahead of a destructor so that every
vtable slot moves. That changes which function the harness's dispatch reaches; it does not change a
byte inside either row's censused extent, and both rows are compiler-generated deleting-destructor
thunks with no source statement of their own to edit. P additionally produces no count at all.
Neither row closes.

---

## 5. P4 Task 3 -- the penetration map, the mesh header and the two stream readers

The full table with each mutation's case names is `evidence/phase4-pmap-reconstruction.md` §4:
twenty-one lettered mutations, A to V with M skipped. All twenty-one were re-measured on this branch.

| census row | mutations, re-measured count each | count spent |
| --- | --- | ---: |
| `phys_fn_002045` `0x000505f0`+69 | J 10 | 10 |
| `phys_fn_002047` `0x00050640`+2340 | A 1, B 2, C 10, D 4, E 5 | 10 |
| `phys_fn_002033` `0x0004ff80`+398 | F 2; G green | 2 |
| `phys_fn_002035` `0x00050110`+439 | H 4, I 4; T green | 4 |
| `phys_fn_002051` `0x00051040`+32 | K 1, L 1 | 1 |
| `phys_fn_002262` `0x00055cb0`+511 | N 3; O 10, of which 9 are another row's | 3 |
| `phys_fn_004772` `0x000b3aa0`+23 | P 9 | 9 |
| `phys_fn_004774` `0x000b3ac0`+24 | Q 4 | 4 |

Every count equals the one P4 Task 3 published against `b114fa73` except **O**, which read 1 at
`bcf544f` and reads **10** here. The constant it edits, `kTriangleMeshTag1`, is file-scope in
`Physics/src/TriangleMesh.cpp`, and on this branch `TriangleMesh::save` (`phys_fn_002162`, added by
the canonical line's P4 Task 4) stores it as well: O moves `mesh.bad_tag1` (1) and all nine writer
cases (9). Nine of its ten are therefore `phys_fn_002162`'s, and `phys_fn_002262` closes on N, whose
three `bad_tag0` cases are its own.

**H was re-anchored.** The branch guards `loadPayload`'s refill on the grid pointer (commit
`8e010f9`), so the loop sits one level deeper than at `bcf544f`; the edit is the same word change,
`0xffffffff` to `0xfffffffe`, and it reads 4 as it did.

The **five** mutations that came out green at `bcf544f` are green again here, and **V**, which was
red only by access violation there, is green here:

| # | census row | mutation | re-measured | why no delta exists |
| --- | --- | --- | --- | --- |
| G | `phys_fn_002033` | `setup`'s fill writes 0 instead of `0xffffffff` | green | `phys_fn_002035` refills the whole grid before any bit is read |
| R | `phys_fn_002041` | the corner pass never ORs `0x40000000` | green | it ORs into cells already `0xffffffff` |
| S | `phys_fn_001986` | the spread table spreads two bits apart, not three | green | its only consumer sorts a uniform array |
| T | `phys_fn_002035` | the sign block never ORs `0x80000000` | green | the cells are already `0xffffffff` |
| U | `phys_fn_002043` | the Morton reorder is skipped | green | it permutes identical words |
| V | `phys_fn_002008` | the cell-run decoder consumes no count | green; red by crash at `bcf544f` | every recorded fixture declares a count of zero |

Every accepted fixture leaves a grid of nothing but `0xffffffff`, so each green mutation is the
identity on everything the harness can see. `phys_fn_002033` and `phys_fn_002035` close anyway on a
*different* mutation aimed at the same extent, with the green arm named in the ledger entry as the
part that does not close. `phys_fn_001986`, `phys_fn_002008`, `phys_fn_002041` and `phys_fn_002043`
close on nothing.

---

## 6. The canonical line's P4 Task 4 -- the mesh writer

`evidence/phase4-mesh-writer.md` §3 publishes six mutations aimed at `phys_fn_002162`
(`TriangleMesh::save`, `0x000539d0`+413) and none aimed at the eleven block-stream store rows it
reconstructed alongside it. The six were re-applied from their descriptions and re-measured on this
branch; all six reproduce the published count and the published localisation.

| # | mutation | re-measured | localisation |
| --- | --- | --- | --- |
| A | the two tags stored in swapped order -- `phys_fn_002162`, **count spent 9** | 9 | every writer case, first differing event 0 |
| B | `if(mConvexMesh) flags \|= 4;` removed whole | 2 | `writer.full`, `writer.hull_present`, first differing event 3, the flags store |
| C | vertices sized `* 8` instead of `* 12` | 8 | every case but `writer.empty_mesh` |
| D | the threshold stored as a literal `0.0f` | 9 | every case, first differing event 4 |
| E | the blob length store deleted | 9 | every case, one event short |
| F | array A sized `* 2` instead of `* 4` | 2 | `writer.full`, `writer.array_a_only` |

No mutation was ever aimed at any of the eleven store rows, and none is manufactured here. Nine are
Phase 4 rows -- `phys_fn_004768`, `004770`, `004780`, `004782`, `004784`, `004788`, `004791`,
`004795` and `004797` -- and defer `reconstructed_not_falsified` on this phase's ledger. The other two,
`phys_fn_004766` and `phys_fn_004786`, are Phase 2 rows: they stay on the Phase 2 ledger, deferred
`homeless_shared_code` with `driving_phases: [4, 6]`.

---

## 7. What no mutation in this phase reaches

- **601 vendored rows at `discovered`**, deferred `vendored_not_falsified`. FIVE own a family
  `NxPhysicsThirdPartyTests` drives and that agrees exactly with the shipped bytes -- `phys_fn_002493`
  (`qh_maxabsval`), `phys_fn_002505` (`qh_pointdist`), `phys_fn_003308` (`qh_set`), `phys_fn_004844`
  (`container_copy`) and `phys_fn_005357` (`mesh_topology`) -- and agreement is not falsification.
  Three more family owners the close counted among its eight, `phys_fn_002465`, `phys_fn_002513` and
  `phys_fn_004836`, stand at `reconstructed` on this branch, raised by later drives, and defer
  `reconstructed_not_falsified` with the same family note. `phys_fn_004842` carries an inlined copy of
  A's statement. The other 595 have had nothing aimed at them.

  *Update, vendored-correspondence Task 5b:* 351 of the then 608 vendored `discovered` rows
  (231,879 bytes, after the plan's final review held back eight ICE rows) are promoted to `reconstructed` on the structural matcher, a hand review and,
  where they have x87 code, an exact or outcome-exact execution against the oracle; they defer
  `reconstructed_not_falsified`, and no mutation is aimed at any of them either. Four of the five
  family owners above are among them (`phys_fn_002493`, `002505`, `003308`, `005357`), and so is
  `phys_fn_004842`. 257 vendored rows stay `discovered`, deferred `vendored_not_falsified`; the
  evidence and the per-reason split are in evidence/vendored-correspondence.md, "Results".
- **`phys_fn_005493`** `0x000f0560`+252, `Segment::SquareDistance`, vendored stock, driven, and it
  does **not** reproduce the image: re-measured here, `segment_sqrdist.grid` reads
  `mismatches=9356 worst_ulp=67` over 60,000 words and `segment_sqrdist.wide`
  `mismatches=15538 worst_ulp=8420` over 40,000, unchanged. On this branch the census types it
  `compiler_artifact` and it is `classified`, outside the ledger.
- **The two `OPC_AABBTree.cpp` rows** (§3), likewise `classified`.
- **Every row under `0x0f7f`.** No mutation in this phase was measured under the `_PC_64 | _RC_CHOP`
  control word `Scene::simulate` installs; both targets run under the CRT default `0x027f`.
  `evidence/phase4-third-party-sources.md` §4.6 records at least 21 OPCODE rows inside the step's
  closure by counting `jmp` edges beside `call` edges.

---

## 8. A Phase 3 row this port re-deferred, and its measurement

`phys_fn_002344` (`0x0005aae0`+104, the pair-list swap-remove) was closed on the Phase 3 ledger by
canonical commit `c962ffa` on Phase 5's `NxPhysicsObjectLayoutTests`. The gate-to-phase binding this
port carries rejects that closure, because that target is registered to phase 5 and runs neither when
Phase 3 is gated nor under `completed` while Phase 5 is pending. It is deferred
`blocked_on_later_phase` with `driving_phases: [5]` so a Phase 5 close can discharge it. Its recorded
mutation -- the match test's first-half arm `begin[i*2]==self` dropped -- re-measured on this branch,
control / mutant / control:

| run | `layout candidate mismatches` | `pairrm` candidate | `layout oracle digest` |
| --- | ---: | --- | --- |
| control | 1 | `ok=1 digest=f0bdae43` | `16dceb3c` |
| mutant | 2 | `ok=0 digest=2df229f4` | `16dceb3c` |
| control | 1 | `ok=1 digest=f0bdae43` | `16dceb3c` |

The control is **not** clean: its one mismatch is `candidate CANDIDATE-MISSING family=vtables`, the
target's designed RED. The row's own family moves from agreeing to disagreeing; the target-level
`mismatches=2` that `c962ffa` recorded is a count over a baseline of 1.

---

## 9. Three Phase 2 rows the review re-deferred, and their measurements

`phys_fn_002410` (`0x0005bac0`+240, the pool release arm), `phys_fn_002404` (`0x0005ba70`+17) and
`phys_fn_002406` (`0x0005ba90`+7), the actor's `+0x08` member subobject's constructor and destructor,
were closed on the Phase 2 ledger by canonical commits `c962ffa` and `e7094e9` on Phase 5's
`NxPhysicsObjectLayoutTests`, with `discharged_by_phase: 5`. The port's review added a rule the
validator did not have -- a discharge counts only once the discharging phase is `pass` in
`program.json`, the objection that re-deferred `phys_fn_002344` (§8) -- and Phase 5 is pending, so all
three are deferred `blocked_on_later_phase` for a Phase 5 close to discharge. Phase 2 closes 56. Each recorded mutation
was re-applied to `Physics/src/ObjectModel.cpp` in a `git archive` copy of `d9a459b` and
`NxPhysicsObjectLayoutTests` rebuilt, control / mutants / control:

| run | `layout candidate mismatches` | families that move | `layout oracle digest` |
| --- | ---: | --- | --- |
| control | 1 | none | `16dceb3c` |
| `phys_fn_002410`, `mir[idx] = u` for the poison | 4 | `relgrow`, `owndtor`, `miscsm2` | `16dceb3c` |
| `phys_fn_002404`, the two field zeroes dropped | 2 | `actorctor`, `ct=1/1/1/0/1` | `16dceb3c` |
| `phys_fn_002406`, a null restored for the third table | 2 | `actorctor`, `dd=1/0/1 adj=1/0/1` | `16dceb3c` |
| control | 1 | none | `16dceb3c` |

`c962ffa` recorded 3 for `phys_fn_002410`; the branch reads 4 because `miscsm2`, the shapes-clear drive
of `phys_fn_002411` added after that measurement, reaches the same release arm through the image's own
call. The two `e7094e9` counts reproduce. Every control is **dirty** in the same way as §8's: its one
mismatch is `candidate CANDIDATE-MISSING family=vtables`, the target's designed RED, so each count is
over a baseline of 1 and what a row owns is its family reading.

---

## 10. 2026-09-24 follow-up: Segment correction and DLL linkage

The numbers in §7 describe the unmodified OPCODE 1.3 source and remain the historical baseline.
The local `External/opcode/novodex/Ice/IceSegment.cpp` overlay now follows the oracle's x87
register and float-store sequence at `0x000f0560`. With the registered `NxPhysicsThirdPartyTests`
inputs against the pinned DLL (SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`),
`segment_sqrdist.grid` reads `mismatches=0 worst_ulp=0` over 60,000 words and
`segment_sqrdist.wide` reads `mismatches=0 worst_ulp=0` over 40,000. Both test families were first
made strict and observed RED on the stock source; they are now exact. The full direct-linked
third-party harness reports `driven=16 divergent=0 words=193028 layout_checks=47`.

The normal static-library link had omitted both `Segment::SquareDistance` and `qh_pointdist` from
the candidate DLL despite the direct-linked tests passing. Explicit MSVC `/INCLUDE` options now
retain both symbols in `NxPhysics.map`; the DLL builds. This establishes physical linkage only.
There is not yet a public-DLL drive of the Segment function, nor a falsifying mutation isolated
to `phys_fn_005493`, so its Phase 4 disposition is `reconstructed_not_falsified`.

Whole-archive linking remains open. It reaches stock `SweepAndPrune::Init`, which refers to
`CompleteBoxPruning` in `OPC_BoxPruning.cpp`, a file absent from the oracle image. The oracle's
`SweepAndPrune::Init` at `0x000e6db0` instead calls its own pruning helper at `0x000b4530`
(`phys_fn_004816`, with continuation `phys_fn_004818`). That helper and the wider archive
retention policy need reconstruction and a DLL-level drive before declaring the embedded
libraries complete.

---

## 11. 2026-09-24 follow-up: NovodeX pruning helper and AABB storage

`SweepAndPrune::Init` at `0x000e6db0` calls `phys_fn_004816` (`0x000b4530`),
whose continuation `phys_fn_004818` starts at `0x000b46b0`. The helper sorts
primary-axis box minima plus a `MAX_FLOAT` sentinel through a persistent
`RadixSort`, then emits ordered overlapping pairs into an Ice `Container`.
`Physics/src/opcode/NovodexBoxPruning.cpp` reconstructs that behavior and
supplies the stock sweep-and-prune object's unresolved dependency without
compiling `OPC_BoxPruning.cpp`, whose other routines are absent from the image.

The first oracle drive exposed a representation mismatch, not a pruning-loop
discrepancy. The oracle reads the first three AABB floats as minima and the
next three as maxima, while the unmodified OPCODE build used centre/extents.
`USE_MINMAX` is now PUBLIC on `NxOpcode`, so its library and consumers compile
the same private AABB layout. After a fresh Win32 Release build, the registered
`complete_pruning` family compares 257 deterministic box sets, counts 0–8,
three axis orders, return values, pair counts and ordered pair IDs: 730 words,
`oracle=39d67cbd`, `mismatches=0`. The complete third-party run reports
`driven=17 divergent=0 words=193758 layout_checks=47` and
`oracle digest=74ebc669`. Phase 2, 3 and 4 gates pass in the isolated worktree;
Phase 4 evaluates 101 of 101 registered coverage assertions.

Two direct-link mutations establish that the new family can fail for errors in
both portions of the oracle helper. Changing the primary sorting axis from
`axes.mAxis0` to `axes.mAxis1` made the 730-word oracle tape disagree with a
1,138-word candidate tape and exited 1. Changing the pair append from
`(first, second)` to `(first, first)` preserved the word count but yielded 108
mismatched words and exited 1. The oracle digest stayed `74ebc669` in both
mutants. Restoring each edit returned the family to `mismatches=0`. These
mutations support the intermediate Phase 4 closures for both stable IDs; they
do not prove a public SDK path reaches the rebuilt DLL implementation.

The registered harness reports `phys_fn_004816 mismatches=1` for the sorting
mutation because its tapes have different lengths, and
`phys_fn_004818 mismatches=108` for the pair-output mutation with equal lengths.

With this helper present, `/WHOLEARCHIVE` linkage of both `NxOpcode` and
`NxQhull` succeeds. The rebuilt `NxPhysics.map` contains the helper,
`Segment::SquareDistance`, `qh_pointdist`, and `SweepAndPrune` symbols. This
proves archive membership, while object lifecycle and public routing remain
work for the full reconstruction.
