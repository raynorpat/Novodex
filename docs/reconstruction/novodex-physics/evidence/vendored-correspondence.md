# Vendored correspondence

Measurement note for the vendored qhull/OPCODE correspondence plan: prove, row by row, that the
candidate's vendored qhull 2003.1 and OPCODE 1.3 code (upstream plus the NovodeX overlays under
`External/*/novodex/`) corresponds to the oracle's. Task 1 built the structural matcher; later
tasks triage its DIFF/MISSING rows, fix real differences and move proven rows. Each task appends
one row to the timing table below.

## The matcher

`tools/vendored_match.py` (tests: `tools/tests/test_vendored_match.py`). It covers every row of
`evidence/phase4-third-party-map/{qhull,opcode,opcode_outside_span}_map.csv` graded `mapped` or
`probable`. For each row it:

1. finds the row's `source_function` in the candidate's linker map;
2. disassembles both bodies by recursive descent;
3. compares them feature by feature (the next section).

Global data references are compared through a learned oracle-to-candidate correspondence,
`phase4-third-party-map/vendored_data_map.csv`. The module docstring holds the full rules.

Outputs:

- `phase4-third-party-map/qhull_match.csv`
- `phase4-third-party-map/opcode_match.csv` (includes the nine `opcode_outside_span_map.csv` rows)
- `phase4-third-party-map/vendored_data_map.csv` (1,136 correspondences: 221 by co-occurrence,
  914 by a rigid symbol delta, 1 by rank vote; a rank pair needs two votes)

Run: `python docs/reconstruction/novodex-physics/tools/vendored_match.py`. The defaults are the
oracle at `D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`, the candidate
`build/Release/NxPhysics.dll`, and the candidate's `.map`. Every run recorded here used candidate
sha256 `ac45c56b41240f928eaf74a4284e7b8fee0440ee495aed09c5149382c5784f34`, the build of
`b210042`. No vendored source has changed since that build.

`tools/qhull_trace_attribution.py` backs the trace-macro finding below.

## What is compared, and what each class means

**Classifying features.** These decide DIFF and REVIEW. All are compared as sets, after an
inlining check has absorbed any callee that one side inlines.

- **Calls, strings, floats and data** decide DIFF.
  - **Calls** are resolved to identities. Overloaded members carry their parameter list, and a
    memory-indirect call carries where its base came from: the qhull host global, a global
    object, a getter's return, an object's vtable, a function-pointer member, a returned
    pointer, or untraced.
  - **Strings** are compared by content.
  - **Floats** are compared by value.
  - **Data** means global references.
- **Structure-field byte coverage** (`diff_fields`) decides REVIEW.
  - It covers memory operands based on a register that is not the stack, taken at non-negative
    displacements with no index.
  - Each byte is tagged with a base class, traced back through register copies and ebp spills:
    - `this`: ecx as it came in;
    - `derived`: a pointer loaded from memory, or an address computed from another register;
    - `other`: anything else.
  - For classification, `derived` and `other` count as one pointer class, because which one a
    register gets depends on which writer comes last in address order. `this` stays separate.
- **Logic and compare immediates** (`diff_imms`) decide REVIEW.
  - These are the immediates of cmp/test/and/or/xor/shifts, minus 0, the x87 status masks and
    stack alignment.
  - Bit operations on fields are recorded as base-classed absolute bit addresses
    (`this:bit@N`, `clear@`, `set@`, `flip@`), so byte-narrowing cancels. This covers memory
    forms, a test/and on a register just loaded from a field, and a register-form and/or/xor
    whose result is stored back to the same field (read-modify-write).
- **x87 operation classes** (`diff_x87ops`) decide REVIEW: the set of {add, sub, mul, div,
  sqrt, abs} each side executes. These are the SHAPE groups that turning it on sent to REVIEW,
  all explicable:
  - `abs` ×3 (`qh_determinant`, `qh_divzero`, `qh_inthresholds`): the candidate compiles qhull's
    `fabs_` ternary macro to `fabs`; the oracle branches and negates.
  - Reciprocal ×3 (`qh_distplane` +div, `qh_getcenter` −mul, `qh_tracemerging` −mul +div): one
    side divides by a value the other multiplies by the reciprocal of.

  Two more qhull groups carry an x87-class difference, but were REVIEW already for other
  features: `qh_getdistance` and `qh_randommatrix`.

**Report-only columns.** These support hand review and never change a class:

- `report_jcc`: pairs of a cmp-with-immediate and the conditional jump that follows it.
- `report_stores`: pairs of a field and the value stored to it, from `mov [r+d], imm` and from
  `fldz`/`fld1` followed by `fstp [r+d]`. Values are normalised: 0 whatever its type, and a float
  bit pattern shown as the float.

**Shape only.** None of these changes the class:

- the other integer immediates (store values, sizes and counts, which compilers choose freely);
- instruction, branch, x87, SSE and fchs counts;
- call order and occurrence counts;
- the trivial float constants 0.0 and 1.0;
- a pair of floats that is one constant written as its reciprocal (product 1) or halved
  (product 2);
- a pair of floats that are negations of each other, but only when the other side has a
  compensating `fchs`;
- one-sided field bytes that lie inside a wider access on their own side, provided that access
  also covers bytes the other side touches (narrower or wider access to the same field). Two
  adjacent one-byte fields do not qualify: RayCollider's `mClosestHit` at `+0x8c` against
  `mCulling` at `+0x8d` stays REVIEW.
- a field or bit token that differs only between the `derived` and `other` pointer classes;
- the ebp frame.

A row takes the first class that applies:

| Class | Meaning |
|---|---|
| MISSING | No candidate symbol. The notes name the oracle callers' candidate symbols as likely inliners. |
| AMBIGUOUS | More than one candidate symbol survives the overload filters. |
| MAPCHECK | Several untagged map rows, none a "continuation" block, resolve to one symbol, so the map may name two functions alike. `compare_class` keeps the comparison's class. |
| DIFF | Calls, strings, floats or global data differ. `diff_causes` tags them. |
| REVIEW | Field coverage, logic/compare immediates or x87 operation classes differ. **This is a mandatory hand-review gate.** The bodies call and reference the same things but read, write or test different parts of the objects. |
| SHAPE | Everything above agrees; only shape differs. |
| MATCH | Every feature is equal. |

None of the classes is a proof. REVIEW exists because SHAPE used to admit two real behavioural
differences, and it now catches both:

- **`OPCODECREATE::OPCODECREATE`** (`0x000e92b0`) is REVIEW, `diff_fields -this[0x4..0x7]`.
  The oracle zeroes `mDeserializeFrom` at `this+4`. The candidate's
  `External/opcode/novodex/OPC_BaseModel.cpp:60` never initialises it, yet `OPC_Model.cpp`
  branches on it.
- **`RayCollider::_RayStab` ×4 and `_SegmentStab` ×4** (`0x000b84c0`, `0x000b8a50`,
  `0x000b9070`, `0x000b9a30`, `0x000b6380`, `0x000b6900`, `0x000b6ef0`, `0x000b7880`; the
  `_SegmentStab` `[body]` rows `0x000b6390` and `0x000b6910` share their group's class) are all
  REVIEW. Six of them carry `diff_fields -this[0x88..0x8b]`; the two NoLeaf `_SegmentStab` groups
  (`0x000b6ef0`, `0x000b7880`) carry `-this[0x84..0x8c]` against `+derived[0x84..0x87]`. The oracle reads `mNovodeXSetting88` as a ± hit-distance
  tolerance. The candidate keeps the stock sign test.

Both are left for Task 3 to fix.

## Summary

Rows and bytes count map rows, so a function the census split into several rows counts each of
its rows. Groups count candidate functions, meaning the rows that resolve to one candidate
symbol.

| Library | Class | Rows | Bytes | Groups |
|---|---|---:|---:|---:|
| qhull | MATCH | 0 | 0 | 0 |
| qhull | SHAPE | 83 | 20,885 | 78 |
| qhull | REVIEW | 85 | 17,889 | 64 |
| qhull | DIFF | 287 | 109,428 | 189 |
| qhull | MAPCHECK / MISSING / AMBIGUOUS | 0 | 0 | 0 |
| qhull | **total** | **455** | **148,202** | **331** |
| OPCODE | MATCH | 17 | 218 | 17 |
| OPCODE | SHAPE | 49 | 2,651 | 49 |
| OPCODE | REVIEW | 127 | 168,752 | 96 |
| OPCODE | DIFF | 72 | 57,773 | 63 |
| OPCODE | MAPCHECK | 0 | 0 | 0 |
| OPCODE | MISSING | 1 | 124 | 1 |
| OPCODE | AMBIGUOUS | 1 | 122 | 1 |
| OPCODE | **total** | **267** | **229,640** | **227** |

For REVIEW groups, the split by cause is:

| Library | Fields | Immediates | Fields + immediates | x87 classes | Immediates + x87 classes |
|---|---:|---:|---:|---:|---:|
| qhull | 21 | 29 | 6 | 6 | 2 |
| OPCODE | 16 | 12 | 68 | 0 | 0 |

Field, immediate or x87-class differences also appear in 163 qhull DIFF groups and 59 OPCODE DIFF
groups, so fixing their calls will not by itself clear them.

The inlining check changed the outcome for 61 qhull groups and 39 OPCODE groups; the `shape`
column lists each as `inlining: ...`.

**Map corrections.** On the map as first committed, MAPCHECK flagged the two untagged
`Model::Release` rows. Both corrections are recorded in `opcode_map.csv` with their evidence:

- `0x000e90e0` is `Model::~Model`. It writes the Model vptr `0x0011bac8`, calls
  `Model::Release` at `0x000e9480`, and tail-jumps to the BaseModel release path at `0x000e9550`.
- `0x000f0890` is `AABBTreeNode`'s vector deleting destructor, with `~AABBTreeNode` inlined. It
  has a `flags&2` array arm over a count at `this-4` with a stride of `0x24`, and it recurses
  with `push 3`.

A row whose alternatives include a deleting destructor now resolves to that destructor, so
`0x000e9280` meets `??_GModel`. MAPCHECK is 0 after the correction.

**Build differences recorded, not compared.** The candidate builds qhull and OPCODE with /GS;
the matcher drops `__security_check_cookie` and `___security_cookie`. It also sets up an ebp
frame in functions where the oracle uses ebp as a general register: 228 qhull and 170 OPCODE
groups carry `ebp frame no->yes` in `shape`.

## Not compared

The matcher does not look at any of the following. A static proof that relies on it must say so:

- register-only ALU operations, and the operand order of x87 operations (`fsub` against `fsubr`,
  `fdiv` against `fdivr`: only the operation class is compared);
- conditional-jump kinds and signedness, except as the report-only `report_jcc` column;
- x87 compare predicates (operand order and branch sense need data flow);
- store and argument immediates (report-only `report_stores`, and the shape `imms` note);
- indexed operands and array offsets, and negative displacements (pointer walks);
- call counts once the inlining check has absorbed a callee (compared as sets only);
- which object a field belongs to, beyond the `this`/pointer base class;
- arithmetic order and precision.

## Promotion policy for Task 5

Which evidence a group needs before its rows can move depends on its class and notes:

- **Static proof alone** covers SHAPE and MATCH groups only when all of these hold:
  - no x87 compare;
  - no x87 arithmetic;
  - no `inlining:` note;
  - no `imms` note in `shape`.

  The proof text must list the uncompared features above.
- **A recorded hand review is required for:**
  - REVIEW groups;
  - SHAPE and MATCH groups with an inlining note or an other-immediates note;
  - SHAPE and MATCH groups with integer conditional branches.

  The review must record which addresses it read and what each difference is.
- **Execution evidence** (a cdb trace, or a differential with a matching outcome) is required
  for any group with x87 compares or x87 arithmetic, whatever else it has.
- **Not promotable until fixed:** DIFF, MISSING and AMBIGUOUS groups. MAPCHECK groups wait for
  the map to be corrected.

## What the DIFF rows say

The `diff_causes` column tags each DIFF. Counts below are by group.

**qhull (189 DIFF groups).** 158 carry `host-seam`, and 125 carry nothing else.

1. **The oracle keeps the trace macros on the CRT.** In 127 groups the oracle calls CRT `fprintf`
   (`0x000f4d5a`) where the candidate calls the host (`qhNovodeXFprintf`, slot `+0x10`).
   - Across the qhull span the oracle has 217 direct CRT `fprintf` calls and 616
     `call [reg+0x10]`.
   - `tools/qhull_trace_attribution.py` finds each call's format string in the upstream sources:
     - CRT sites: 208 sit inside a `traceN((...))` macro, 2 are plain `fprintf` calls, and 7
       are unattributed.
     - Host sites: 8 sit inside a trace macro, 555 are plain calls, and 53 are unattributed.
   - So NovodeX redirected qhull's plain `fprintf` and left `trace0`..`trace5` on the CRT. The
     candidate's `#define fprintf qhNovodeXFprintf` in `user.h` redirects both.
   - By file: merge.c 41, poly2.c 23, poly.c 18, geom2.c 17, geom.c 10, qhull.c 9, io.c 6,
     global.c 3.
2. **`mem.c` and `qset.c` print through the CRT in the candidate.** 21 groups (qset.c 15, mem.c 6).
   The oracle uses slot `+0x10`. The candidate calls CRT `fprintf` because neither file includes
   `user.h`.
3. **The `+0x04` geometry dump is not modelled.** 6 groups: `qh_printpointid`, `qh_printpoint`,
   `qh_printpoints_out`, `qh_printfacetheader`, `qh_printbegin` and `qh_printfacets`. The oracle
   calls slot `+0x04`; the candidate prints the coordinates with `%8.4g`/`%6.16g` strings.
4. **`_CIsqrt` for `fsqrt`.** 14 groups: the candidate calls `_CIsqrt` where the oracle has an
   inline `fsqrt`.
5. **The rest is small and mixed:**
   - float constants in 10 groups;
   - `qh` field references in 17 groups;
   - 11 oracle host calls whose base the tracer could not attribute (`icall[?]`);
   - NovodeX calls made from qhull rows (`NxFluidAssert`, two census rows).

**OPCODE (63 DIFF groups).**

1. **Allocation still goes through `operator new[]`/`delete[]`.** 24 groups carry `allocator`.
   The oracle calls the allocator getter `phys_fn_004803` (`0x000b4000`) and dispatches
   `icall[getter]+0x0`/`+0xc`. The candidate calls `operator new[]`, `operator delete[]` and
   `operator delete`, from the upstream `DELETEARRAY`/`DELETESINGLE`/`new[]` macros.
2. **Array construction.** 8 groups carry `vector-iterator`. The oracle uses the `vector
   constructor iterator` at `0x00001000`; the candidate constructs inline.
3. **ICE `CONTAINER_STATS` is compiled in.** It is defined at upstream `IceContainer.h:15`, and 6
   `Container` rows reference `mUsedRam`/`mNbContainers`, which the oracle does not have.
4. **`LSSCollider` distance helpers.** In 8 groups the candidate calls
   `SqrDistance`/`OPC_SegmentTriangleSqrDist` out of line. The oracle inlines them, with the
   oracle-only constants `FLT_MAX` and `1e-5f`.
5. **The rest:**
   - `_CIsqrt` in 6 groups;
   - vtable-slot differences where NovodeX extended OPCODE (`phase4-third-party.md` §2.3):
     `Model::Build`, `BaseModel::Refit`, `AABBTreeNode::Split`, `AABBTree::Refit2`,
     `AABBTree::Walk`.

**MISSING.** `Point::Mult(const Matrix3x3&, const Point&)` (`0x000e4cb0`). `Ice/IcePoint.cpp` is
not compiled into `NxOpcode`.

**AMBIGUOUS.** `AABBTreeOfTrianglesBuilder::GetSplittingValue(...) [outlined loop]`
(`0x000e9aa0`). The map row names no parameters, and both overloads exist.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-27T15:24:12 | 2026-09-27T15:56:00 | 0 | 0 | Structural matcher `tools/vendored_match.py` + 29 unit tests; first run over 455 qhull and 267 OPCODE rows (qhull MATCH 2 / SHAPE 177 / DIFF 276; OPCODE MATCH 19 / SHAPE 174 / DIFF 72 / MISSING 1 / AMBIGUOUS 1). Top DIFF causes: qhull trace macros on the CRT in the oracle (127 groups), mem.c/qset.c on the CRT in the candidate (21), OPCODE allocator not redirected (25). No product code or ledger change; gates 2, 3, 4, 6, 7 pass, Phase 5 red only on `CANDIDATE-MISSING family=vtables`. |
| 1 review | 2026-09-27T16:06:39 | 2026-09-27T16:30:00 | 0 | 0 | Matcher strengthened after review: REVIEW class (field byte coverage, logic/compare immediates with bit-address normalisation), MAPCHECK class, overload-aware call keys and real operator names, icall base tags, float rule narrowed to reciprocal/halving and fchs-backed negation, repe cmps, ebp-frame note, rank pairs need two votes; opcode_map.csv 0x000e90e0 and 0x000f0890 corrected; tools/qhull_trace_attribution.py committed; 58 matcher tests. qhull MATCH 0 / SHAPE 88 / REVIEW 80 / DIFF 287; OPCODE MATCH 17 / SHAPE 60 / REVIEW 116 / DIFF 72 / MISSING 1 / AMBIGUOUS 1. OPCODECREATE ctor and the 8 RayCollider stab rows are REVIEW. No product code or ledger change. |
| 1 final | 2026-09-27T16:33:00 | 2026-09-27T16:44:39 | 0 | 0 | Router approved; final changes: x87 operation classes as a REVIEW feature (6 SHAPE groups flipped, all explained: fabs_ macro x3, reciprocal x3); coverage-based narrowing replaces the same-dword downgrade (adjacent one-byte fields stay REVIEW); field and bit tokens carry a base class (this / derived / other, derived and other merged for classification) traced through copies and ebp spills; register read-modify-write and/or/xor normalised to the memory form; report-only report_jcc and report_stores columns; "Not compared" and "Promotion policy for Task 5" sections; 69 matcher tests. qhull SHAPE 83 / REVIEW 85 / DIFF 287; OPCODE MATCH 17 / SHAPE 49 / REVIEW 127 / DIFF 72 / MISSING 1 / AMBIGUOUS 1. No product code or ledger change. |
