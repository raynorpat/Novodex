# Vendored correspondence

Measurement note for the vendored qhull/OPCODE correspondence plan: prove, row by row, that the
candidate's vendored qhull 2003.1 and OPCODE 1.3 code (upstream plus the NovodeX overlays under
`External/*/novodex/`) corresponds to the oracle's. Task 1 built the structural matcher; later
tasks triage its DIFF/MISSING rows, fix real differences and move proven rows. Each task appends
one row to the timing table below.

## The matcher

`tools/vendored_match.py` (tests: `tools/tests/test_vendored_match.py`). For every row of
`evidence/phase4-third-party-map/{qhull,opcode,opcode_outside_span}_map.csv` graded `mapped` or
`probable` it finds the row's `source_function` in the candidate's linker map, disassembles both
bodies by recursive descent, and compares calls (resolved to identities), strings by content,
float constants by value, and global data references (through a learned oracle-to-candidate
correspondence, `phase4-third-party-map/vendored_data_map.csv`). Integer immediates, branch and
instruction counts, x87/SSE counts and call order are shape only. The rules are in the module
docstring. Outputs:

- `phase4-third-party-map/qhull_match.csv`
- `phase4-third-party-map/opcode_match.csv` (includes the nine `opcode_outside_span_map.csv` rows)
- `phase4-third-party-map/vendored_data_map.csv` (1,142 correspondences: 220 by co-occurrence,
  914 by a rigid symbol delta, 8 by rank)

Run: `python docs/reconstruction/novodex-physics/tools/vendored_match.py` (defaults: the oracle at
`D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`, the candidate `build/Release/NxPhysics.dll`
and its `.map`). First run against candidate sha256
`ac45c56b41240f928eaf74a4284e7b8fee0440ee495aed09c5149382c5784f34` (build of `b210042`; no
vendored source changed after it).

The classes route attention; none of them is a proof. MATCH and SHAPE say the two bodies call the
same things with the same strings, constants and globals, allowing for inlining; they say nothing
about arithmetic order or precision.

## Summary

Rows and bytes are map rows (a function the census split into several rows counts each row);
groups are candidate functions (the rows that resolve to one candidate symbol).

| Library | Class | Rows | Bytes | Groups |
|---|---|---:|---:|---:|
| qhull | MATCH | 2 | 70 | 2 |
| qhull | SHAPE | 177 | 42,961 | 148 |
| qhull | DIFF | 276 | 105,171 | 181 |
| qhull | MISSING | 0 | 0 | 0 |
| qhull | AMBIGUOUS | 0 | 0 | 0 |
| qhull | **total** | **455** | **148,202** | **331** |
| OPCODE | MATCH | 19 | 490 | 19 |
| OPCODE | SHAPE | 174 | 171,000 | 143 |
| OPCODE | DIFF | 72 | 57,904 | 62 |
| OPCODE | MISSING | 1 | 124 | 1 |
| OPCODE | AMBIGUOUS | 1 | 122 | 1 |
| OPCODE | **total** | **267** | **229,640** | **226** |

62 qhull and 39 OPCODE groups reached MATCH/SHAPE/DIFF only after the inlining check absorbed a
callee one side inlines (listed in the `shape` column as `inlining: ...`).

## What the DIFF rows say

The `diff_causes` column tags each DIFF. By group:

**qhull (181 DIFF groups).** 157 carry `host-seam`; 133 carry nothing else.

1. **The oracle keeps the trace macros on the CRT.** 127 groups: the oracle calls CRT `fprintf`
   (`0x000f4d5a`) where the candidate calls the host (`qhNovodeXFprintf`, slot `+0x10`). Across the
   qhull span the oracle has 217 direct CRT `fprintf` calls and 616 `call [reg+0x10]`. Classified
   by the upstream line that holds each call's format string, 184 of the 186 attributable CRT
   sites are inside a `traceN((...))` macro and 444 of the 451 attributable host sites are not.
   So NovodeX redirected qhull's plain `fprintf` and left the `trace0`..`trace5` macros on the
   CRT; the candidate's `#define fprintf qhNovodeXFprintf` in `user.h` catches both.
   (merge.c 41, poly2.c 23, poly.c 18, geom2.c 17, geom.c 10, qhull.c 9, io.c 6, global.c 3.)
2. **`mem.c` and `qset.c` print through the CRT in the candidate.** 21 groups (qset.c 15, mem.c 6):
   the oracle uses slot `+0x10`, the candidate calls CRT `fprintf` because those files do not see
   the `user.h` redefinition.
3. **The `+0x04` geometry dump is not modelled.** 6 groups (`qh_printpointid`, `qh_printpoint`,
   `qh_printpoints_out`, `qh_printfacetheader`, `qh_printbegin`, `qh_printfacets`) call slot
   `+0x04` in the oracle; the candidate prints the coordinates with `%8.4g`/`%6.16g` strings.
4. **`_CIsqrt` for `fsqrt`.** 14 groups: the candidate calls `_CIsqrt` where the oracle has an
   inline `fsqrt` (the same compiler difference the joint work fixed with `JointX87.h`).
5. The remainder is small and mixed: float constants (`0.004` vs `0.002`, `DBL_EPSILON` vs a
   computed epsilon, 10 groups), a handful of `qh` field references (11 groups), and two NovodeX
   calls the oracle makes from qhull rows (`NxFluidAssert`, two census rows).

**OPCODE (62 DIFF groups).**

1. **Allocation still goes through `operator new[]`/`delete[]`.** 25 groups carry `allocator`: the
   oracle calls the allocator getter `phys_fn_004803` (`0x000b4000`) and dispatches `[+0x00]`/
   `[+0x0c]`, the candidate calls `operator new[]`/`operator delete[]`/`operator delete` (the
   upstream `DELETEARRAY`/`DELETESINGLE`/`new[]` macros), e.g. every tree `Build`, every tree
   scalar deleting destructor, `AABBTreeNode::~AABBTreeNode`, `AABBTree::Release`.
2. **Array construction.** 8 groups (`vector-iterator`): the oracle builds node arrays with the
   `vector constructor iterator` (`0x00001000`) and a folded no-op constructor; the candidate
   constructs inline.
3. **ICE container statistics are compiled in.** 6 `Container` rows reference
   `Container::mUsedRam`/`mNbContainers`; the oracle's `Container` has no statistics globals.
4. **`LSSCollider` distance helpers.** 8 groups call `SqrDistance`/`OPC_SegmentTriangleSqrDist`
   out of line where the oracle has them inlined into the recursion bodies without the same
   constants (`FLT_MAX`, `1e-5f` oracle-only).
5. **`_CIsqrt` for `fsqrt`** in 6 groups; a few virtual-slot differences in `Model::Build`,
   `BaseModel::Refit`, `AABBTreeNode::Split`, `AABBTree::Refit2`, `AABBTree::Walk` (NovodeX's
   extra `BuildSettings` fields and tree serialisation, `phase4-third-party.md` §2.3).

**MISSING.** `Point::Mult(const Matrix3x3&, const Point&)` (`0x000e4cb0`, 124 bytes):
`Ice/IcePoint.cpp` is not in the `NxOpcode` source list and no oracle map row calls it.

**AMBIGUOUS.** `AABBTreeOfTrianglesBuilder::GetSplittingValue(...) [outlined loop]`
(`0x000e9aa0`): the map row names no parameters and both overloads exist.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-27T15:24:12 | 2026-09-27T15:56:00 | 0 | 0 | Structural matcher `tools/vendored_match.py` + 29 unit tests; first run over 455 qhull and 267 OPCODE rows (qhull MATCH 2 / SHAPE 177 / DIFF 276; OPCODE MATCH 19 / SHAPE 174 / DIFF 72 / MISSING 1 / AMBIGUOUS 1). Top DIFF causes: qhull trace macros on the CRT in the oracle (127 groups), mem.c/qset.c on the CRT in the candidate (21), OPCODE allocator not redirected (25). No product code or ledger change; gates 2, 3, 4, 6, 7 pass, Phase 5 red only on `CANDIDATE-MISSING family=vtables`. |
