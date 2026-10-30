# Reconstruct the 270 remaining `discovered` rows

Date: 2026-10-30. Status: approved in conversation, awaiting written-spec review.

## Goal

Every code row of the pinned `NxPhysics.dll` (SHA-256 `4b7db3e1…602c`) has a candidate implementation with a static
proof before any further testing work. Today 270 of the 2,786 code rows (72,572 bytes, 7.7% of code bytes) are
`discovered`: no candidate implementation is recorded.

**Done for a row** means static proof only:

* it is a naked listing transcription in the candidate, or it is compiled from vendored library source that the
  vendored matcher accepts;
* its listing-compare script (first-party) or `tools/vendored_match.py` (vendored) passes;
* `inventory.json` records it as `reconstructed` with source, symbol, map reference and static proof, and an empty
  dynamic proof;
* its closure entry reads `reconstructed_not_falsified` (first-party) or stays `vendored_not_falsified` (vendored).

Not in scope: oracle differentials, mutation sweeps, new gate coverage lines. They belong to the later testing pass,
together with the 1,102 rows already waiting for row-specific falsification.

**Success:** zero `discovered` rows (or a short, reasoned hard-rows list), gates 3 to 7 passing, the tooling tests
passing (857 or more), `validate_inventory.py` reporting `unexplained=0`.

## The 270 rows

| Kind | Rows | Bytes | Where |
|---|---|---|---|
| Vendored qhull | 71 | 22,664 | `0x5c5c0`–`0x849a6`, all Phase 4 |
| Vendored OPCODE | 15 | 3,783 | `0xb4f00`–`0xe9277`, all Phase 4 |
| First-party | 184 | 46,125 | 46 address clusters, Phases 3, 4, 6 and 7 |

189 rows call no other `discovered` row; 13 call more than one. 84 are called by rows that are already done (through
C++ stand-ins or aliases). 115 have indirect call sites.

## Approach (hybrid)

A vendored pass handles the cheap promotions, and first-party clusters follow in phase order: 3, 4, 6, 7. The Phase 7
block goes last because it is the largest and riskiest.

### 1. Vendored pass (86 rows)

1. Triage with `tools/vendored_match.py` against the current build. Each row lands in one bucket:
   (a) already matches: a stale state, promote it;
   (b) diverges because the Novodex build of the library differs from upstream (a constant, an inlined helper, a
   compiler flag): fix it in the overlay `External/<lib>/novodex/<file>` and re-run the matcher until it matches;
   (c) absent from the candidate (not compiled or dropped by the linker): transcribe it as a naked row in
   `Physics/src/vendored/<Lib>Rows.cpp`, with the first-party recipe below.
2. The proof is matcher class SHAPE or better, recorded exactly as Task 5b's vendored-correspondence promotion recorded
   it (`evidence/vendored-correspondence.md`).
3. One commit for qhull and one for OPCODE (the qhull commit may split by source file).
4. A row that resists both a source fix and a transcription after about an hour goes on the hard-rows list with its
   reason; it does not block the pass.

### 2. First-party transcription unit (184 rows)

One unit is one address cluster (or a batch of small neighbours in one subsystem):

1. **Listing.** Capstone disassembles each row's inventory range into `cfg_<cluster>.py`. Rows the inventory splits at
   a jump target (as with 005244/005246) map to one candidate function covering both ids.
2. **Generate.** `gen11.py` emits naked `__asm` rows, callees first. A row goes into the existing file of its
   subsystem when there is one (for example a mesh row into `InternalTriangleMesh.cpp`). Otherwise it goes into a new
   file named for the subsystem, added to the four CMake source lists.
3. **Wiring**, as in the pruner cluster:
   * calls to done rows go through `/alternatename` aliases;
   * oracle `.data` becomes named statics or a `static_ranges` block;
   * imports use the candidate's IAT slots;
   * jump tables become compare chains.
4. **Re-pointing.** Where a done caller reaches the new row through a C++ stand-in, the stand-in becomes a thin
   forward to the naked row (or the alias is switched) and its old body is deleted.
5. **Proof.** `evidence/phase<N>-<topic>-listing-compare.py` (on `listing_compare_lib.py`) must print `ALL EQUAL`
   and `unexpected=0`. Its output and the generator inputs go to `evidence/raw/phase<N>-<topic>-<date>/`. A short
   `evidence/phase<N>-<topic>.md` lists the rows, what each does, any place that differs from a plain copy, and the
   rows with indirect calls.
6. **Registry.** In `inventory.json` the rows become `reconstructed` with source, symbol, map reference and static
   proof, and their notes say they await the testing pass. In the phase closure ledger the entry goes from
   `not_reconstructed_in_phase` to `reconstructed_not_falsified` and its counts are updated.

About 35 to 45 commits for the 46 clusters, with small single-row neighbours batched per subsystem.

### 3. Failure handling

* **A row that cannot be transcribed as-is** (SEH frames, label tables, computed jumps into the middle of a function).
  Try the existing workarounds first: compare chains, and aliases for jumps out of the row. Otherwise emit that span
  with `__emit` and list it in the compare script's allowed spans with the reason. If neither works, the row goes on
  the hard-rows list.
* **Indirect and virtual calls.** The static proof covers them because the sequence is equal. The evidence doc flags
  each such row for the testing pass (it needs a live vtable).
* **Re-pointing breaks a passing test.** That is a real finding. Stop, compare the stand-in with the oracle listing,
  fix whichever is wrong, and say so in the commit message.
* **Build breaks** (duplicate symbols, alias collisions). Every unit builds the full Release solution before commit.
* **Registry drift.** Every commit runs `validate_inventory.py` (`unexplained=0`), `report_completion.py`,
  `work_units.py` and the tooling tests.
* **Gates 3 to 7** (about 10 minutes) run at each stage exit below, not per cluster.

### 4. Stages

| Stage | Content | Clusters | Rows | Bytes | Exit |
|---|---|---|---|---|---|
| 1 | Phase 3 first-party | 9 | 49 | 10,112 | gates 3–7 |
| 2 | Vendored pass (qhull, OPCODE) | — | 86 | 26,447 | gates 3–7 |
| 3 | Phase 4 first-party, `0x2d940`–`0x5580a` | 11 | 53 | 9,681 | — |
| 4 | Phase 4 first-party, `0xb3d20`–`0xf3ee4` (with the mixed 4/6 and 4/7 clusters) | 17 | 46 | 6,015 | gates 3–7 |
| 5 | Phase 6 | 5 | 10 | 2,577 | gates 3–7 |
| 6 | Phase 7, `0x56690`–`0x5ba3a` (including the 14.7 KB block) | 4 | 26 | 17,740 | gates 3–7 |

Progress is the `discovered` count from `report_completion.py`, going from 270 to 0. The implementation plan carries
one checkbox per cluster.

## End state and handoff

* No `discovered` rows remain; the only exceptions are on the hard-rows list, each with its reason.
* Every new row is `reconstructed` with a static proof, and its closure entry reads `reconstructed_not_falsified` or
  `vendored_not_falsified`.
* Gates 3 to 7, the tooling tests and the inventory validator pass.
* A handoff note groups the newly reconstructed rows by the test family that would naturally drive them, and marks
  the rows with indirect calls, as the input to the testing pass.

## Appendix: first-party clusters

Address range, rows, bytes, phase, first and last row id (`phys_fn_`).

Stage 1 (Phase 3):

| Range | Rows | Bytes | Phase | Ids |
|---|---|---|---|---|
| 0x23940-0x23a54 | 4 | 246 | 3 | 001099-001105 |
| 0x23da0-0x24399 | 13 | 1025 | 3 | 001131-001167 |
| 0x245e0-0x24634 | 1 | 84 | 3 | 001181 |
| 0x24990-0x24da4 | 10 | 800 | 3 | 001211-001235 |
| 0x258f0-0x26699 | 6 | 3467 | 3 | 001303-001313 |
| 0x26ad0-0x27732 | 12 | 2758 | 3 | 001317-001345 |
| 0x46ab0-0x470ed | 1 | 1597 | 3 | 001876 |
| 0x4b9c0-0x4b9ee | 1 | 46 | 3 | 001935 |
| 0x4bc20-0x4bc79 | 1 | 89 | 3 | 001947 |

Stage 3 (Phase 4, first half):

| Range | Rows | Bytes | Phase | Ids |
|---|---|---|---|---|
| 0x2d940-0x2d9a8 | 2 | 83 | 4 | 001524-001528 |
| 0x4cba0-0x4cc55 | 1 | 181 | 4 | 001988 |
| 0x4cee0-0x4db9e | 8 | 3226 | 4 | 001992-002006 |
| 0x4df30-0x4e112 | 3 | 476 | 4 | 002010-002014 |
| 0x4ff20-0x4ff71 | 1 | 81 | 4 | 002031 |
| 0x52060-0x521ab | 1 | 331 | 4 | 002077 |
| 0x52a60-0x5328c | 24 | 2010 | 4 | 002095-002138 |
| 0x53f40-0x54793 | 7 | 1316 | 4 | 002178-002205 |
| 0x54a80-0x54ba2 | 1 | 290 | 4 | 002239 |
| 0x54e70-0x552b5 | 3 | 1085 | 4 | 002243-002247 |
| 0x555b0-0x5580a | 2 | 602 | 4 | 002255-002256 |

Stage 4 (Phase 4, second half):

| Range | Rows | Bytes | Phase | Ids |
|---|---|---|---|---|
| 0xb3d20-0xb3fff | 4 | 418 | 4/6 | 004790-004801 |
| 0xb4790-0xb4d6b | 7 | 1155 | 4/7 | 004820-004834 |
| 0xb5090-0xb5123 | 1 | 147 | 4 | 004852 |
| 0xb5460-0xb5497 | 2 | 48 | 4 | 004870-004872 |
| 0xe5590-0xe56d0 | 1 | 320 | 4 | 005231 |
| 0xe6230-0xe647e | 2 | 590 | 4 | 005262-005263 |
| 0xe6b50-0xe6c0a | 1 | 186 | 4 | 005279 |
| 0xe9420-0xe9545 | 2 | 152 | 4 | 005380-005386 |
| 0xe9b20-0xe9b7e | 1 | 94 | 4 | 005404 |
| 0xef670-0xefa88 | 8 | 901 | 4 | 005448-005466 |
| 0xefc90-0xeffb4 | 4 | 669 | 4 | 005470-005479 |
| 0xf15c0-0xf15f3 | 1 | 51 | 4 | 005539 |
| 0xf2090-0xf2199 | 2 | 256 | 4 | 005566-005568 |
| 0xf2580-0xf2684 | 3 | 247 | 4 | 005586-005590 |
| 0xf2ec0-0xf2fd4 | 3 | 263 | 4 | 005606-005610 |
| 0xf3630-0xf36a5 | 1 | 117 | 4 | 005624 |
| 0xf3d40-0xf3ee4 | 3 | 401 | 4 | 005642-005646 |

Stage 5 (Phase 6):

| Range | Rows | Bytes | Phase | Ids |
|---|---|---|---|---|
| 0x9ab00-0x9b0d0 | 4 | 1190 | 6 | 004159-004171 |
| 0xafcc0-0xaffda | 1 | 794 | 6 | 004401 |
| 0xb02e0-0xb0548 | 2 | 454 | 6 | 004405-004413 |
| 0xb15e0-0xb15ff | 1 | 31 | 6 | 004537 |
| 0xb38a0-0xb3917 | 2 | 108 | 6 | 004759-004761 |

Stage 6 (Phase 7):

| Range | Rows | Bytes | Phase | Ids |
|---|---|---|---|---|
| 0x56690-0x5a16a | 22 | 14739 | 7 | 002268-002316 |
| 0x5a520-0x5a8d3 | 2 | 929 | 7 | 002332-002336 |
| 0x5ae50-0x5b60e | 1 | 1982 | 7 | 002350 |
| 0x5b9e0-0x5ba3a | 1 | 90 | 7 | 002400 |
