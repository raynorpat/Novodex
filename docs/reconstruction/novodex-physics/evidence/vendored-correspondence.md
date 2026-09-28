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
`build/Release/NxPhysics.dll`, and the candidate's `.map`. The Task 1 runs used candidate
sha256 `ac45c56b41240f928eaf74a4284e7b8fee0440ee495aed09c5149382c5784f34`, the build of
`b210042`. The Task 2 figures (the Summary's qhull rows and the Task 2 section) are from candidate
sha256 `0b3027e5c70d792c8df06677ae177ab1023b8c7e825bb0c9a6a76f9da4cdea35`, the build of the
Task 2 head; the review-fix rebuild (`3c5b834e...`, comment and matcher changes only) gives
identical match classes.

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

The qhull rows are after Task 2 (Task 1 ended at SHAPE 83 / REVIEW 85 / DIFF 287 rows; see the
Task 2 section); the OPCODE rows are after Task 3 (Task 1 ended at MATCH 17 / SHAPE 49 / REVIEW
127 / DIFF 72 / MISSING 1 / AMBIGUOUS 1 over 267 rows; see the Task 3 section). Rows and bytes count map rows, so a
function the census split into several rows counts each of its rows. Groups count candidate functions, meaning the rows that resolve to one candidate
symbol.

| Library | Class | Rows | Bytes | Groups |
|---|---|---:|---:|---:|
| qhull | MATCH | 0 | 0 | 0 |
| qhull | SHAPE | 162 | 41,396 | 142 |
| qhull | REVIEW | 197 | 60,831 | 142 |
| qhull | DIFF | 96 | 45,975 | 47 |
| qhull | MAPCHECK / MISSING / AMBIGUOUS | 0 | 0 | 0 |
| qhull | **total** | **455** | **148,202** | **331** |
| OPCODE | MATCH | 18 | 233 | 18 |
| OPCODE | SHAPE | 65 | 10,983 | 63 |
| OPCODE | REVIEW | 176 | 193,271 | 136 |
| OPCODE | DIFF | 14 | 28,004 | 14 |
| OPCODE | MAPCHECK | 0 | 0 | 0 |
| OPCODE | MISSING | 0 | 0 | 0 |
| OPCODE | AMBIGUOUS | 0 | 0 | 0 |
| OPCODE | **total** | **273** | **232,491** | **231** |

For REVIEW groups, the split by cause at the end of Task 1 is:

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
- the order of the arguments at a call the compiler inlined. Task 4 found one such difference by
  execution: `AABBCollider::_Collide(const AABBTreeNode*)` swaps them in stock 1.3;
- which object a field belongs to, beyond the `this`/pointer base class;
- arithmetic order and precision. In particular the grouping of float sums, which the 2003
  compiler reassociated per site. `tools/x87_sum_grouping.py` reports it for three-product sums
  (`phase4-third-party-map/sum_grouping.csv`; see "Summation order" below) but nothing
  classifies on it.

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
  for any group with x87 compares or x87 arithmetic, whatever else it has. Task 4 records it per
  group in `phase4-third-party-map/vendored_coverage.csv` (`hits`, `outcome`, `differential`),
  backed by `evidence/vendored-trace-{qhull,opcode}.txt`. Task 5a replaced the column's values
  with `exact` / `lastbit` / `discrete`, classed per execution (see "Task 5a"). Deciding which
  class counts as "a matching outcome" is Task 5b's call. *Task 5b's rule is under "Task 5b:
  promotion", below.*
- **Not promotable until fixed:** DIFF, MISSING and AMBIGUOUS groups. MAPCHECK groups wait for
  the map to be corrected.
- **Summation order and register lifetimes (added after the Task 3 review).** A static proof
  for a vendored row with x87 arithmetic says that the float-sum grouping and some register
  lifetimes of the oracle are not reproduced (see "Summation order"), and names the row's
  `sum_grouping.csv` sites. That is a recorded limitation, not a promotion blocker. Execution
  evidence is still required as above.

## What the DIFF rows say (at the end of Task 1)

The `diff_causes` column tags each DIFF. Counts below are by group. Task 2 resolved the qhull
causes; see the Task 2 section.

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

## Task 2: qhull triage

Every qhull DIFF group was traced to its cause, the real differences were fixed, and every qhull
REVIEW, SHAPE and DIFF group got a hand-review line in
`phase4-third-party-map/qhull_review.csv` (331 groups: group, class, reviewed_by_task, verdict,
addresses, note).

**Fixes, with the qhull match classes (rows) after each.**

| Commit | Change | SHAPE | REVIEW | DIFF |
|---|---|---:|---:|---:|
| (Task 1) | | 83 | 85 | 287 |
| `e6cc359` | Trace macros back on the CRT: `(fprintf) args` in a new `qhull_a.h` overlay; the plain-`fprintf` redirect is a function-like macro in `QhullNovodeXHost.h`, reached from `user.h` and a new `mem.h` overlay so `mem.c`/`qset.c` print through `+0x10` too | 133 | 206 | 116 |
| `93c0e1d` | `/Qfast_transcendentals` on `NxQhull`: `sqrt` is the inline `fsqrt` the oracle has (`0x0005fbac`), not `__CIsqrt` | 138 | 211 | 106 |
| `18774f3` | NovodeX's typed host dispatches: `io.c` overlay (`qh_printpointid` `+0x04`, `qh_printfacet3vertex` `+0x08`, `qh_printbegin` `+0x00`, `qh_printfacets` `+0x0c`) and `poly2.c` overlay (`qh_initialhull` `+0x1c`); five host hooks; matcher seams and two CRT identities | 140 | 216 | 99 |
| `dcb2836` | Map correction: `0x0007f310` is `qh_settempfree_all`, not `qh_setfree2` | 140 | 219 | 96 |
| `2d823f3` | Build parity: `geom.c` overlay evaluates `qh_gausselim`, `qh_getcenter` and `qh_normalize2` with the reciprocal the 2003 compiler formed (`0x0005d363`, `0x0005d501`, `0x0005d7d0`) | 142 | 217 | 96 |
| `a1e5428` | High-byte register bit tests (`test ch,8` is bit 11, not bit 3); `test ah,imm` after a field load is a field test, not the x87 status word | 162 | 197 | 96 |

**The one real behavioural divergence** was numeric: the oracle's compiler replaced
loop-invariant divisions with a reciprocal and multiplications. `x*(1/n)` and `x/n` differ in the
last bit, and `qh_normalize2` runs on every facet normal, `qh_getcenter` on the interior point and
every centrum. The `geom.c` overlay is recorded in `External/qhull/MODIFICATIONS.md` as build
parity, not as a NovodeX change. The same transformation on paths the NovodeX driver never enables
(its option string is `"o"`, `.rdata:0x0011363c`) is documented there and left stock.

**The remaining 47 DIFF groups are all explained and none is a defect.** 46 are equivalent:

- host `fprintf` calls whose base the matcher's tracer cannot attribute (`icall[?]`/`[getter]`/`[obj]`);
- inlining that the call-set check cannot absorb;
- ICF folds: `qh_comparevisit` into `qh_comparemerge`, and `qh_user_memsizes` into the one-byte `ret`;
- CRT `_iob` against `__acrt_iob_func`;
- string literals referenced as data;
- exact constant folds: `-2*x` as `fchs`/`fadd`, `2*x*0.002` as `x*0.004`, and `k*REALepsilon`
  with `REALepsilon = 2^-52`;
- a dead alignment check that the 2003 compiler kept;
- loop bounds written as `&PRINTout[qh_PRINTEND]`.

One group, `qh_initqhull_globals`, differs numerically (`RANDOMa`), but only under options the
driver never passes.

**REVIEW and SHAPE groups.** The review attributes every field, logic-immediate and x87-class
token to a listing form:

- host vptr loads;
- set walks;
- register-held masks and constants;
- high-byte tests;
- inlined callees;
- `memcpy` splits;
- magic division;
- the manual readings recorded in the notes.

Every one-sided compare/branch predicate is matched to the other side's form. `fabs_` compiled to
`fabs` differs only in the sign of a zero, and every use is a comparison or a magnitude. Eight
REVIEW groups carry a numeric difference that exists only under driver-disabled options. Ten SHAPE
groups were read in full against the listing: `qh_pointid`, `qh_setlast`, `qh_willdelete`,
`qh_delvertex`, `qh_setsize`, `qh_maxabsval`, `qh_point`, `qh_mergevertex_del`, `qh_memfree` and
`qh_crossproduct`. All of them agree statement for statement.

**The host object's slots.** The vtable at `.rdata:0x00113614` is:

| Slot | Row | Role |
|---|---|---|
| `+0x00` | `003259` | off-header |
| `+0x04` | `003261` | push three floats |
| `+0x08` | `003268` | facet append |
| `+0x0c` | `003265` | area/volume |
| `+0x10` | `003263` | fprintf |
| `+0x14` | `003272` | arena malloc |
| `+0x18` | `003275` | free |
| `+0x1c` | `001583` | a one-byte `ret`, so the narrow-hull hook does nothing |
| `+0x20` | `003267` | release, then `longjmp` to the driver's `jmp_buf` at `.data:0x00125040` |

So qhull's `"o"` output is how NovodeX collects the hull.

**Unmapped rows: not written.** The 37 unmapped qhull rows are NovodeX's own code, and the
candidate has none of them wired:

- Six have generic shape reconstructions in `Physics/src/ObjectModel.cpp` (`003238`, `003257`,
  `003261`, `003265`, `003268`, `003274`).
- 31 are `discovered`: 3,903 instructions, 750 of them x87.

They are a hull-cooking library around qhull:

- the driver `003279` and its `setjmp`, and `003236`, which calls `qh_init_A`, `qh_initflags`,
  `qh_init_B`, `qh_qhull`, `qh_check_output` and `qh_produce_output`;
- vertex clean-up with a bounding-box normalisation (`003243`/`003245`);
- the output arena class above;
- the `QHULL_OK_%04d.obj` and `QHULL_FAIL_%04d.obj` writers;
- band B (`0x0007fda0`-`0x000814f0`), a hashed vertex/edge structure.

Their only caller is `phys_fn_002233` in `TriangleMesh.cpp`, which is itself not reconstructed,
so nothing in the candidate can reach them, and no differential exercises them. This needs its own
work unit (bundle `gap:Controller.cpp..fluids\Fluid.cpp`), with a differential built through
`phys_fn_002233`.

## Task 3: OPCODE triage

Every OPCODE DIFF, MISSING and AMBIGUOUS group was traced to its cause. The real differences
were fixed in `External/opcode/novodex/` overlays, and the map errors were corrected. Every OPCODE
group now has a hand-review line in `phase4-third-party-map/opcode_review.csv`: 231 groups, with
the same columns as `qhull_review.csv`.

**Fixes, with the OPCODE match classes (rows) after each.** Map corrections add or remove rows,
because `unmapped` rows are outside the matcher.

| Commit | Change | MATCH | SHAPE | REVIEW | DIFF | MISSING | AMBIG. | Rows |
|---|---|---:|---:|---:|---:|---:|---:|---:|
| (Task 1) | | 17 | 49 | 127 | 72 | 1 | 1 | 267 |
| `6e14ab6` | `OPCODECREATE` clears `mDeserializeFrom` (`0x000e92d2`) | 17 | 50 | 126 | 72 | 1 | 1 | 267 |
| `dbd3172` | RayCollider: the culling arm of `RayTriOverlap` widens U, V and U+V by the float at `+0x88`, and the constructor clears it (see below) | 17 | 50 | 126 | 72 | 1 | 1 | 267 |
| `9266906` | Host allocator by class (`OPC_NOVODEX_ALLOCATEABLE`), node constructors empty, `mIndices` at its sites | 17 | 50 | 134 | 64 | 1 | 1 | 267 |
| `85004e8` | `CONTAINER_STATS` off (`Ice/IceContainer.h` overlay) | 17 | 50 | 138 | 60 | 1 | 1 | 267 |
| `d6ffcb3` | `/Qfast_transcendentals` on `NxOpcode`: inline `fsqrt` | 17 | 52 | 140 | 56 | 1 | 1 | 267 |
| `021fbe3` | Matcher: register vtable calls, trivial-constructor iterators, floats stored as immediates, CRT operator thunks | 18 | 55 | 148 | 44 | 1 | 1 | 267 |
| `88fe6ba` | Map: `CoplanarTriTri`, `OBB::IsInside`, the Matrix3x3 cast; parser keeps conversion operators | 18 | 57 | 151 | 41 | 1 | 1 | 269 |
| `75ff740` | Map: `OBB::ComputePlanes`, `OBB::ComputePoints` | 18 | 57 | 153 | 41 | 1 | 1 | 271 |
| `05d81a9` | SweepAndPrune: NovodeX `Init`, constructor, destructor, allocator; map rows corrected | 18 | 58 | 159 | 38 | 1 | 1 | 275 |
| `b7b1131` | Map: `ComputeWorldEdgeNormal` (was `Point::Mult`), `~BaseModel`; seeded data objects | 18 | 60 | 160 | 36 | 0 | 1 | 275 |
| `70ab8b1` | Map: `GetSplittingValue`'s loop block | 18 | 60 | 162 | 35 | 0 | 0 | 275 |
| `f075dd1` | Map: `0x000f11b0` is `Refit2`; `0x000f1350` is an ICE culling walk (unmapped) | 18 | 60 | 163 | 33 | 0 | 0 | 274 |
| `ad690da` | Map: `0x000e3f50` is `Triangle::Normal` | 18 | 61 | 163 | 32 | 0 | 0 | 274 |
| `e153286` | Matcher: `-1.0` against a negated `1.0` (**withdrawn in review**, see "Task 3 review fixes") | 18 | 62 | 163 | 31 | 0 | 0 | 274 |
| `660dcec` | Matcher: constant-store window of 12 | 18 | 62 | 168 | 26 | 0 | 0 | 274 |
| `19a9844` | LSSCollider: NovodeX members and inflated-box test; LSS distance rows renamed; census `third_party` for the newly mapped rows | 18 | 66 | 176 | 13 | 0 | 0 | 273 |
| `ba36f82`..`8c48bd5` | Review fixes: SphereTriOverlap reciprocal form (the `-1.0` rule removed), RayTriOverlap V lifetime, minors | 18 | 65 | 176 | 14 | 0 | 0 | 273 |

The final figures, after the review fixes, are MATCH 18 rows (233 bytes, 18 groups), SHAPE 65
(10,983, 63), REVIEW 176 (193,271, 136) and DIFF 14 (28,004, 14); from candidate sha256
`795324dcc62363a37bc28bd732268eb548bda87b993d6e3f2cf20dd5b2d3f45f`, the build of `8c48bd5`. At `19a9844` they were SHAPE 66 (12,418, 64) and DIFF 13 (26,569, 13).
The qhull classes did not change. Every qhull match line is unchanged except a candidate string
address inside one `report_jcc` value, which moved because the candidate DLL's layout moved.

**Task 3 review fixes** (`ba36f82`, `e4f9f73`, `8c48bd5`).

- **The `-1.0` matcher rule (`e153286`) was wrong, and its commit message is too.** It read the
  oracle's float `-1.0` in `SphereCollider::SphereTriOverlap` as the candidate's `1.0` negated at
  run time. In fact the oracle evaluates the edge-region distances `u = -fB/fA; SqrDist = fB*u+fC`
  as a reciprocal product `(-1.0/fA)*fB*fB + fC`, with nothing rounded to float
  (`0x000de623`..`0x000de635`, `0x000de710`..`0x000de722`), and keeps `SqrDist` on the x87 stack to
  `fabs; fcomp [mRadius2]` (`0x000de7c1`). The candidate divided (`fchs; fdivrp`) and rounded `u`
  to float, a low-bit difference the rule hid. The fix is a build-parity overlay,
  `OPC_SphereTriOverlap.h`, recorded like qhull's `geom.c`: it writes the reciprocal form and makes
  `SqrDist` a double, the project's convention for a register lifetime. The rule and its test are
  removed. The group is now DIFF on the `-1.0` constant alone. The 2026 compiler rewrites the new
  form as `fC - (1/fA)*fB*fB` (`fld1; fdivr; fmul; fmul; fsubr`), and every step of that rewrite is
  an exact sign identity, so the result is bit-identical. Its review line says so.
- **RayTriOverlap's V.** At all 14 inline sites the oracle compares V with `-t` and forms `U+V`
  from the unrounded register (`fcom [lo]; fst [esi+0x58]; ... fadd st(1)`, `0x000b87d9`..`0x000b87ef`).
  The candidate rounded V to float first. V is now a double lifetime, written out as its dot
  product because `Point::operator|` rounds on return. Only `mStabbedFace.mV` is float.
  - Not fixed, and recorded for Task 5: the oracle sums every dot product in these arms z-first,
    `(qz*Dz + qy*Dy) + qx*Dx` (`0x000b87c0`..`0x000b87d7`, and U at `0x000b871f`). That is a
    2003 reassociation; the candidate sums x-first.
- **Minors.**
  - The `/Qfast_transcendentals` comment now says future transcendentals are inlined too.
  - `External/README.md` states the line-ending rule.
  - The closure prose counts are corrected (306 rows; "FIVE of the 608").
  - The `BaseModel::Save`/`Load` stubs report through the SetIceError seam. The host side of that
    seam prints nothing, so no transcript changes.
  - `opcode_review.csv` no longer hard-codes candidate addresses in its hand notes: they moved with
    every rebuild (`0x000cfe36` was stale). Each line now ends with the current candidate sites,
    stamped with the build they were read from.

**Real behavioural divergences found and fixed.**

1. **`OPCODECREATE::mDeserializeFrom` was uninitialised.** `Model::Build` branches on it. A stack
   `OPCODECREATE` would take the load path with a garbage pointer.
2. **RayCollider's added member is a float tolerance on the barycentric bounds of the culling
   arm.** It is not a hit-distance tolerance:
   - `U < -t`, `U > det + t`, `V < -t` and `U + V > det + t` reject (`0x000b873b`..`0x000b87f1`),
     where stock tests the sign bit and `det` exactly;
   - the distance's sign test stays stock;
   - the non-culling arm is stock;
   - the same code is inlined at 14 sites: `RayCollider::InitQuery` `0x000b5aef`, `0x000b5f75`, and
     the twelve `_RayStab`/`_SegmentStab` copies `0x000b65be`, `0x000b6bc3`, `0x000b7113`,
     `0x000b755d`, `0x000b7b30`, `0x000b7f7a`, `0x000b873b`, `0x000b8d50`, `0x000b92d4`,
     `0x000b9715`, `0x000b9d1d`, `0x000ba15e` (an earlier version of this note said "all eight
     variants");
   - V is compared with the lower bound and added to U as the unrounded register value; only the
     store to `mStabbedFace.mV` is float (fixed in review; see below).

   The constructor zeroes it (`0x000b5736`), and the candidate's constructor had not. Its one
   writer is the scene raycast at `0x0002929b`, from `[[scene+0xe0]+0x70]`. The Scene
   reconstruction's stand-in for the constructor (`Scene.cpp`) already writes the 0.
3. **The allocator line is drawn by class.** The model, builder, tree, node and sweep-and-prune
   classes reach the singleton for every `new`/`delete` of them, including the compiler-generated
   ones. The colliders and `Plane` use the CRT.

   The earlier explicit helpers called a tree's destructor and then freed it. The image calls the
   tree's virtual deleting destructor instead (`0x000e931e`).
4. **The node constructors are empty in the image.** The vector constructor iterator is passed
   `0x00027f00`, `mov eax,ecx; ret`. Stock zeroes the child words.
5. **`CONTAINER_STATS` was compiled in.** The image keeps no container statics.
6. **`__CIsqrt` for `fsqrt`.** The UCRT's SSE2 path ignores the x87 control word, and OPCODE runs
   under `0x0f7f` as well as `0x027f`.
7. **SweepAndPrune is NovodeX-modified.** `Init` takes a third argument, a per-object byte array
   that drops static-static pairs. It also rejects empty input, frees the previous arrays, and
   keeps its scratch on the stack. The constructor zeroes the members, and the destructor frees
   them. The census had split `Init` across four rows under two wrong names.
8. **LSSCollider is NovodeX-modified.**
   - It adds the radius, half the segment's direction, the direction's absolute value, and the
     segment's centre. `mRadius2` moves from `+0x4c` to `+0x74`.
   - Box tests use RayCollider's segment-box separating-axis test against the box inflated by the
     radius, in place of the exact segment-box squared distance.

   The inflated test is conservative: it accepts every box stock accepts. So the triangle queries
   report the same triangles, but the `_CollideNoPrimitiveTest` paths, which dump whole subtrees,
   can report more.

Nothing in the candidate's product code calls the LSS, OBB, sphere, AABB or planes colliders or
SweepAndPrune yet, so no registered line moved. Gates 2, 3, 4, 6 and 7 pass. NxPhysicsThirdPartyTests
passes 101 of 101 with oracle digest `74ebc669`, and NxPhysicsAssetTests has digest `eaefc573`.

**Map corrections** are recorded in each row's `notes`:

- `0x000538b0`: respelled so it pairs with the deleting destructor.
- `0x000ba8e0`: `CoplanarTriTri`, not `TriTriOverlap`.
- `0x000e4d30`, `0x000e4580`, `0x000e48e0`: `OBB::IsInside`, `OBB::ComputePlanes` and
  `OBB::ComputePoints`, where they had been unmapped.
- `0x000e9b20`: stock `Matrix3x3::operator Matrix4x4`, left unmapped because the validator takes
  no operator names. The matcher names it through `ORACLE_KNOWN`.
- `0x000e4cb0`: `OBB::ComputeWorldEdgeNormal`, not `Point::Mult`.
- `0x000e9550`: `~BaseModel`, not `ReleaseBase`.
- `0x000e9aa0`: `GetSplittingValue`'s loop block.
- SweepAndPrune:
  - `0x000e6c10` is `SAP_PairData::Init`;
  - `0x000e6ca0`, `0x000e6db0` and `0x000e70c0` are one `Init`;
  - `0x000e71b0` and `0x000e71e0` are the destructor.
- `0x000f11b0`: `Refit2`, not `Walk`. The image has no `Walk`.
- `0x000f1350`: an ICE culling walk. It had been mapped as `Refit2`.
- `0x000e3f50`: `Triangle::Normal`, not `Compacity`.
- `0x000d1ed0`, `0x000d1930`, `0x000d14b0`: `OPC_SegmentTriangleSqrDist`,
  `OPC_SegmentSegmentSqrDist` and `OPC_PointTriangleSqrDist`, not `LSSTriOverlap` blocks.

The seven rows newly mapped to stock functions now declare `third_party=opcode` in the census and
defer `vendored_not_falsified`, as the validator requires. `phase4-closure.json` moves from 313/601
to 306/608. `0x000f1350` no longer declares a third party. It is still typed `compiler_artifact`,
although it is real code: a retype for the ledger's owner.

**Matcher changes** (`vendored_match.py`, 81 tests after the review):

- A register call through a just-loaded vtable slot is the memory call it stands for.
- A one-sided `vector constructor iterator` is unwrapped: a trivial constructor drops out, and a
  real one becomes a call that the inlining check can absorb.
- A float constant that only feeds stores matches an integer store of the same bits to the same
  field.
- ~~`-1.0` against a run-time-negated `1.0` is shape.~~ Withdrawn in review: it rested on a
  misreading and hid a real difference in `SphereCollider::SphereTriOverlap`.
- The CRT operator thunks are named, and so is `CompleteBoxPruning`.
- `SEEDED_DATA` holds three hand-identified data objects.
- The source-name parser keeps a conversion operator's two words.
- `ORACLE_KNOWN` applies to unmapped rows.

**Review.** `opcode_review.csv` has one line per group (231):

| Class | Groups | Verdicts |
|---|---:|---|
| REVIEW | 136 | all equivalent |
| SHAPE | 63 | all equivalent |
| MATCH | 18 | all equivalent |
| DIFF | 14 | 10 equivalent, 4 novodex-variant-unwritten |

The method:

1. Every field and bit token was re-derived with `lea`/`add`/`mov` copies folded into effective
   offsets, ignoring the base class.
2. Each residue was read against the listing and recorded with addresses. The residues are:
   - `rep movs` member copies;
   - allocator vtable loads;
   - loop-walked pointers;
   - reloaded `this`;
   - inlined callees.
3. Every one-sided logic or compare immediate was attributed to a listing form, again with
   addresses:
   - index scaling;
   - `fnstsw` tests;
   - flag set/clear in register versus memory form;
   - `(mFlags&3)==3`-style mask compares;
   - loop bounds;
   - the 2026 deleting-destructor flag 4;
   - `GetNbFaces()!=0` as `>=4`.

The nine equivalent DIFF groups are inlining, ICF, table-offset or cookie-loop forms, and each is
named in its line. Groups with x87 work still need execution evidence (Task 5). The review is
tool-assisted in the same sense as Task 2's: the rules attribute tokens to forms; they do not prove
a whole body equivalent.

**Still open.** These are separate work units, not vendored-correspondence rows:

1. **The NovodeX tree-collider callback variant.**
   - Rows `0x000d12b0`, `0x000cd700`, `0x000ca5a0` and `0x000cbe50`: 25,613 bytes.
   - A second instantiation of the no-leaf tree-versus-tree code that takes a depth counter and a
     callback, where stock adds to `mPairs`.
   - Its only caller is NovodeX's unreconstructed mesh-mesh contact, `phys_fn_001876`.
2. **Serialization.**
   - BaseModel's slots 4 to 6 (`0x000e9420`, `0x000e9440`, `0x000e94c0`), and the four trees'
     getSerialSize, save, load and pointer-relocation rows (`0x000f2570`..`0x000f3e70`): 17 rows,
     1,277 bytes.
   - The overlays still carry stubs that return 0 or false.
   - `BaseModel::Save` is reached from the reconstructed `TriangleMesh.cpp` save path, and
     `Model::Build` dispatches `Load` when `mDeserializeFrom` is set. So these stubs are live on
     reconstructed paths.
   - The stream helpers they call (`phys_fn_004797`, `004799`, `004774`, `004778`) are MemoryStream
     rows, so writing them needs new host seams and a direct-oracle differential.
3. **The unmapped NovodeX clusters inside the span.** 143 rows, 22,243 bytes. The classification
   is below.

**Unmapped rows (143).**

| Rows | Bytes | States | What |
|---:|---:|---|---|
| 12 | 556 | 11 dynamically_gated, 1 reconstructed | `IcePrunable.cpp`, the Prunable class (`.rdata:0x0011b5a4`); P4 Task 2b |
| 5 | 60 | 4 reconstructed, 1 dynamically_gated | constant getters `0x000e3190`/`a0`/`b0`/`0x000e4ca0`; `RadixSort::SetRankBuffers` |
| 24 | 2,384 | 22 discovered, 2 reconstructed | NovodeX pruner A (`0x000e50c0`..; vtables `0x0011b9c8`/`0x0011b9f0`): builds and refits an AABBTree and runs the Ray, Sphere and AABB colliders |
| 6 | 2,484 | discovered | NovodeX pruner B (`0x000e5ab0`..`0x000e6430`): container queries with static result buffers |
| 1 | 186 | discovered | SweepAndPrune box dump through a callback (`0x000e6b50`; NovodeX addition; caller `phys_fn_001978`) |
| 14 | 2,403 | 9 discovered, 2 dynamically_gated, 3 reconstructed | the NovodeX pruning owner and prunable pool (`0x000e7250`..; vtable `0x0011ba1c`) |
| 1 | 5 | classified | a `jmp rand` thunk (`0x000e7c40`) |
| 16 | 4,828 | 14 discovered, 2 reconstructed | penetration-map builder helpers (`0x000e7c50`..`0x000e8f40`; vtable `0x0011ba40`; `PenetrationMap::Create` callers) |
| 3 | 205 | 2 discovered, 1 reconstructed | BaseModel serialization slots (open item 2) |
| 1 | 94 | discovered | stock `Matrix3x3::operator Matrix4x4` (unmapped for the validator only) |
| 25 | 4,562 | 22 discovered, 3 reconstructed | NovodeX pruner C (`0x000ef270`..; vtables `0x0011bb98`/`0x0011bbc0`) |
| 1 | 418 | classified | the ICE plane-culling walk (`0x000f1350`) |
| 20 | 2,986 | 18 discovered, 2 reconstructed | NovodeX pruner D (`0x000f1550`..; vtable `0x0011bc18`) |
| 14 | 1,072 | 10 discovered, 4 reconstructed | tree serialization (open item 2) |

None of the pruner, penetration-map or dump clusters is called by a mapped OPCODE row. They call
OPCODE; OPCODE does not call them. So they are not needed for the vendored code to correspond. They
belong with the scene-query and mesh work units that own their callers.

The "reconstructed" rows in them are P4 Task 2b's small generic helpers (getters, zeroers, frees),
and none is wired to its cluster.

## Summation order (Task 3 follow-up)

**Finding.** The oracle does not add float products in source order. `a.x*b.x + a.y*b.y + a.z*b.z`
is `(x+y)+z` in the source. The 2003 compiler that built the oracle reassociated these sums site by
site; a 2026 `/fp:precise` build keeps the source order. The two groupings round differently in the
last bit of the register value, and that bit shows up in compares made on the register and in any
float that is later stored.

**Scope, measured by `tools/x87_sum_grouping.py`.** The tool uses a symbolic x87 evaluator over
every matched group; the method is in its docstring and it has 6 tests. Its output is
`phase4-third-party-map/sum_grouping.csv`. The figures below are after the review fix to its
`fxch` handling: capstone lists `fxch st(i)` as `(st(0), st(i))`, and the first version read the
zero index, which made every `fxch` a no-op. For example, the candidate site `0x000a4756` in
`RayCollider::InitQuery` now reads `(x+y)+z`.

- **OPCODE, oracle: 1,068 three-product sums in 70 groups.** Their groupings:

  | Grouping | Sites |
  |---|---:|
  | `(y+z)+x` | 408 |
  | `(x+z)+y` | 257 |
  | `(x+y)+z` (source order) | 217 |
  | unlabelled, no common Point base | 186 |

  So only about a quarter of the labelled sites are in source order.
- **OPCODE, candidate: 703 sites.**

  | Grouping | Sites |
  |---|---:|
  | `(x+y)+z` | 216 |
  | `(y+z)+x` | 10 |
  | `(x+z)+y` | 4 |
  | unlabelled | 473 |

  The candidate keeps more values in registers, so fewer products can be named. Where they can,
  they are overwhelmingly in source order.
- **Pairing.** Of the oracle sites whose three products the candidate also sums, 37 group the same
  way and 158 do not.
- **It is not one rule, and not one rule per source expression.** The same inlined source
  expression is grouped differently in different instantiations. RayTriOverlap's
  `det = edge1|pvec`, for example:
  - is `(z+y)+x` in `_RayStab(const AABBCollisionNode*)` (`0x000b86cb`..`0x000b86e1`);
  - is `(x+z)+y` in `_RayStab(const AABBQuantizedNode*)` (`0x000b8cd8`..`0x000b8cf2`);
  - and `_RayStab(const AABBCollisionNode*)` has all 7 of its sites as `(y+z)+x`, where
    `_RayStab(const AABBQuantizedNode*)` has 5 `(x+z)+y` and 2 `(y+z)+x`.

  The grouping follows which operand the 2003 scheduler had live on the x87 stack, not the
  expression. `IcePoint`'s `operator|` and the other inline operators are therefore not the unit
  of the difference. An overlay of `IcePoint.h` cannot reproduce it: no single spelling of
  `operator|` matches the instantiations.
- **qhull is affected too.** Task 2's rows do not already match. `qh_distplane`'s 3-d case, on the
  hull's main path, is `((p2*n2 + p1*n1) + p0*n0) + offset` in the oracle
  (`0x0005c601`..`0x0005c61d`), the source order reversed. The source, `geom.c`, reads
  `offset + p0*n0 + p1*n1 + p2*n2`. The tool's three-product pattern misses sums that begin with a
  non-product (the offset), so its qhull count (11 oracle and 7 candidate sites, all unlabelled
  `double` walks; 1 paired, grouped differently) undercounts.

**Register lifetimes vary per instantiation too.** RayTriOverlap's V uses the unrounded `qvec.z`
still on the stack (`0x000b87c0`). The CollisionNode copy's `det` multiplies `pvec.z` from the
register (`fst [esp+0x44]; fmul`, `0x000b86c7`), where the QuantizedNode copy reloads it rounded
from its float slot (`0x000b8cd4`). No shared source spelling reproduces both.

**Decision (user, via the coordinator, after the Task 3 review).** This block is finished and
promoted under the policy, with each static proof for a vendored row stating that summation order
and some register lifetimes are not reproduced. The per-site grouping is its own tool-driven work
unit, later.

**Not fixed; this needs its own work unit.** Reproducing the oracle needs an explicit grouping at
each of the several hundred OPCODE sites, and at the qhull sites once they are counted. Where an
inline header is instantiated several times with different groupings, the header needs a
per-instantiation grouping parameter. That is a rewrite of the collider and overlap headers
(`OPC_RayTriOverlap.h`, `OPC_TriBoxOverlap.h`, `OPC_BoxBoxOverlap.h`, the `_Collide` bodies, the
ICE math headers) and of qhull's `geom.c`/`geom2.c` sums, driven site by site from
`sum_grouping.csv` extended to n-term sums. A partial fix would leave a mix. So none is applied
here, including the RayTriOverlap `V` from the review fix: V's value is unrounded, but its sum
keeps source order.

Consequences for promotion:

- Every group with x87 sums already needs execution evidence under the Task 5 policy. That
  evidence will see these low-bit differences, and they are expected, not a new defect.
- `opcode_review.csv` notes each affected group's oracle and candidate groupings.
- No registered differential line moved. The families the differentials drive are either
  bit-exact already (`segment_sqrdist`: P4 Task 2b's `Ice/IceSegment.cpp` overlay spells the oracle's
  groupings out explicitly, e.g. `((dz*dz) + (dx*dx)) + (dy*dy)`, which is the precedent for the
  site-by-site unit) or contain no
  such sum.

## Task 4: execution coverage

The promotion policy needs execution evidence (a cdb trace, or a differential with a matching
outcome) for every group with x87 code. Task 4 measured how many groups the Phase 4 differentials
execute, found almost none, and extended `NxPhysicsThirdPartyTests` until most of them run with
their outcome compared against the oracle.

### Which binary is traced, and why it is the shipped code

The Phase 4 differentials are oracle differentials. `NxPhysicsThirdPartyTests` and
`NxPhysicsAssetTests` load the pinned oracle and call its rows at their RVAs. The candidate side
of every comparison is the vendored code linked into the test exe itself, not a staged pair of
DLLs. So the exe is what executes candidate code, and the exe is what is traced.

- The exe links the same `NxQhull.lib`/`NxOpcode.lib` objects that `NxPhysics.dll` takes with
  `/WHOLEARCHIVE`. `External/CMakeLists.txt` compiles each library once, with one set of flags.
- `tools/vendored_trace.py identity` checks the linked result as well. It compares each traced
  group's body in the exe with its body in `build/Release/NxPhysics.dll`, instruction by
  instruction. A relocated operand, or a branch or call target, is compared as the symbol (plus
  offset) it resolves to in that image's own linker map; one address can carry several names
  under the exe's `/OPT:ICF`.
- The result: every traced body is the same code except one, `??_GAABBTreeBuilder` (no x87
  code). The exe's linker took that inline COMDAT from the harness's own object.
- Both test targets now link with `/MAP`, which writes a map beside the exe and changes nothing
  in it. The exes have no PDB, so the trace resolves breakpoints from those maps.

### Method

`tools/vendored_trace.py` (14 tests in `tools/tests/test_vendored_trace.py`):

- `script` writes a cdb command file. It sets one counting breakpoint per distinct exe address of
  a MATCH/SHAPE/REVIEW/DIFF group (501 in the ThirdParty exe, 13 in the asset exe). The counters
  sit in a page allocated at `0x60000000`, and each is capped at 5 per family.
- A boundary breakpoint on `nxReport` prints `SEG <family>`, dumps the counters and re-arms them.
  A second boundary at `nxDriveQhullPure` closes the candidate-only layout assertions (`layout`).
- `parse` turns the log into per-family counts, and reads each family's verdict from the
  harness's own lines.
- `coverage` writes `phase4-third-party-map/vendored_coverage.csv`, one line per group, with
  these columns:
  - `x87`: the oracle rows' x87 instruction count;
  - `identity`;
  - `hits`: a trailing `+` means a family reached the cap;
  - `outcome` and `differential`: the families, with their verdicts.
- `report` writes the committed excerpts `evidence/vendored-trace-qhull.txt` and
  `vendored-trace-opcode.txt`. They pin the sha256 of the DLL and of both exes, list every
  group's counts per family, and keep the head of the raw log.

Pinned binaries:

- candidate DLL `f3a603a23b3cbbcd582c6fdec9f8ad2679a2a10e959078ffd521481f03e00224`;
- `NxPhysicsThirdPartyTests.exe` `77cb0e722b2465a152ef0a95d8dbf551964d96e61b7b376b6cedb91b0e0140b7`;
- `NxPhysicsAssetTests.exe` `3a18abf599ed3186ddf98c8007f50ffc2f854a650ec1cd5d0829ca903a30db72`;
- oracle `4b7db3e1…602c`.

Reading `outcome`:

| Value | Meaning |
|---|---|
| `exact` | Every family the group ran in compares with no mismatch. |
| `exact+divergent` | It ran in a drive whose discrete outcome is exact and whose float outputs are a measured, attributed divergence (below). |
| `divergent` | Only divergent families ran it. |
| `layout` | Only the candidate-only layout assertions ran it: execution, not a compared outcome. |
| `none` | Nothing ran it. |

These were Task 4's values. Task 5a replaced `exact+divergent` and `divergent` with `lastbit` and
`discrete`, classed per execution, and added `family_best` and `uncompared`; see "Task 5a". The
table below keeps Task 4's figures.

### Coverage before and after

Groups executed. The columns after "executed" split it by `outcome`. "x87 compared" counts the
groups with x87 code that ran in a compared family, whatever the verdict.

| Library | Class | Groups | Before: executed | Before: x87 compared | After: executed | exact | exact+divergent | divergent | layout | After: x87 compared / x87 groups |
|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| qhull | SHAPE | 142 | 6 | 3 | 76 | 3 | 73 | 0 | 0 | 19 / 48 |
| qhull | REVIEW | 142 | 2 | 0 | 81 | 0 | 81 | 0 | 0 | 38 / 63 |
| qhull | DIFF | 47 | 1 | 0 | 23 | 1 | 22 | 0 | 0 | 16 / 32 |
| OPCODE | MATCH | 18 | 1 (layout) | 0 | 11 | 6 | 5 | 0 | 0 | 0 / 0 |
| OPCODE | SHAPE | 63 | 14 (6 layout) | 1 | 50 | 23 | 22 | 5 | 0 | 13 / 15 |
| OPCODE | REVIEW | 136 | 12 (4 layout) | 5 | 124 | 79 | 40 | 5 | 0 | 96 / 96 |
| OPCODE | DIFF | 14 | 4 (3 layout) | 0 | 8 | 3 | 3 | 1 | 1 | 2 / 5 |

"Before" is the trace of the unextended harnesses, on candidate `e9e1ba15…11a9`. Together they
executed 40 groups.

### What was added

`tests/PhysicsThirdPartyTests.cpp` gains 29 registered families; `tests/PhysicsThirdPartyQhull.c`
holds the qhull half. Inputs are built once and handed to both sides. The oracle runs at its
RVAs on objects its own constructors initialise.

- The colliders query the **oracle-built** models on both sides, so they compare the colliders
  and not the builds.
- Containers the oracle filled are released through the oracle's own destructor.
- Registrations: exact families are registered whole, and divergent ones only up to their oracle
  digest, so no registered line pins candidate output. The Phase 4 floor goes from 101 to 130.

**OPCODE.** Six meshes are used:

- a height field;
- a triangle soup;
- a flat grid, where every triangle is coplanar;
- a degenerate set: collinear, repeated-vertex and duplicate triangles;
- a single triangle, the single-node model;
- a closed box.

What runs over them:

- **Model builds.** Each mesh in all four tree kinds, all five splitting-rule families, the kept
  source tree, and both NovodeX build settings (families `opcode_model_build`, `opcode_refit`).
- **Colliders.** `RayCollider` in ray and segment form, with culling, closest hit, first contact,
  temporal coherence, the `+0x88` tolerance, world matrices and `ValidateSettings`. `Sphere`,
  `OBB` (full box test on and off), `AABB`, `LSS` and `Planes`, with primitive tests on and off,
  temporal coherence, world matrices, huge and zero-size volumes, a degenerate segment, and
  planes that contain everything.
- **Tree-versus-tree.** `AABBTreeCollider` through `BVTCache`: seven model pairs in three
  placements, both full tests, first contact and temporal coherence. Box against box and flat
  against flat reach `CoplanarTriTri`.
- **Vanilla `AABBTree`.** Build, `Refit2`, and the ray, sphere and AABB queries on it.
- **`SweepAndPrune`.** The NovodeX three-argument `Init`, updates, and the pairs.
- **ICE maths.** `AABB`, `Plane`, `Triangle`, `IndexedTriangle`, `Matrix4x4`, `InvertPRMatrix`
  and `OBB`, over singular matrices, collinear triangles and zero extents.

**qhull.** It runs the NovodeX driver's own sequence (`003236`): `qh_init_A`, `qh_initflags`,
`qh_init_B`, `qh_qhull`, `qh_check_output`, `qh_produce_output`.

- The option sets are `"o"` over 12 point sets, and `"o Qt"`, `"o FA"`, `"o C-0"`, `"o Qx"`,
  `"o Qbb"`, `"o QbB"`, `"o Qs"`, `"o Tv"` and `"o QR1"` over the five sets that merge.
- The point sets include a lattice with coplanar points on every face, a jittered slab,
  duplicated points, a cylinder of coplanar rings, and points far from the origin. Three sets
  exit with an error: a flat set, too few points, and a collinear set.
- Every facet, ridge and vertex is compared: ids, flags, sets and point indices on one tape; the
  doubles on another.
- The oracle's host object (`.data:0x00125080`, the unrecovered NovodeX class) is replaced for
  the family by a stand-in with the same nine slots. Each slot's argument count was read at its
  call sites (`0x0006c727`, `0x00067f84`, `0x00067c7a`, `0x0006d458`, `0x0005c753`,
  `0x0006dade`, `0x0006dc74`, `0x0007965a`, `0x0008480d`).
- What `qh_produce_output` hands the host is not compared, because the candidate's hooks are
  shims. The state it leaves (total area and volume, facet areas) is compared.
- An error exit is compared as taken or not. The candidate's `qhNovodeXErrexit` drops the exit
  code and aborts; the harness catches the abort with a `SIGABRT` handler.

### Outcome differences found

**1. Fixed: `AABBCollider::_Collide(const AABBTreeNode*)` argument order.**

- Stock OPCODE 1.3 passes `(Center, Extents)` to `AABBAABBOverlap(const Point& b, const Point& Pb)`,
  whose parameters are extents then centre. Every other walk passes them the right way round.
- The image subtracts the node centre from the query centre (`0x000eef35`) and adds the node
  extents to the query extents (`0x000eef44`). The candidate did the reverse.
- As a result 33 of the 60 vanilla AABB queries came out differently; in 32 of them the
  candidate rejected the root.
- The fix is the new overlay `External/opcode/novodex/OPC_AABBCollider.cpp`, recorded in
  `MODIFICATIONS.md`. It is the only product change in Task 4, and it moves the candidate from
  `e9e1ba15…` to `f3a603a2…`. The match class of the group (REVIEW) is unchanged: the matcher
  cannot see argument order.

**2. Not fixed: the oracle's `VolumeCache` is not OPCODE 1.3's.**

- Stock 1.3 embeds the result `Container` at the head of the cache, and `InitQuery` takes its
  address. The oracle **loads a `Container*`** from `+0` instead: `mov ecx,[edx]; mov
  [esi+0x10],ecx` in `SphereCollider::InitQuery` (`0x000de925`), and the same in
  `OBBCollider::InitQuery` (`0x000d57ab`).
- It reads `cache.Model` at `+4` (`0x000dea64`), and `SphereCache`'s `Center`, `FatRadius2` and
  `FatCoeff` at `+8`, `+0x14` and `+0x18` (`0x000dea87`, `0x000dea58`, `0x000dea7f`).
- NovodeX made the base 8 bytes (`Container*`, `Model`) where stock is 20. The candidate keeps
  the stock layout: `0x000a112a` stores `&cache`.
- The harness therefore gives the oracle a NovodeX-layout cache image and the candidate its
  vendored one, from the same state. With that, all five volume colliders compare exactly, the
  cache's derived fields included, which confirms the NovodeX layout for all five.
- The fix changes `OPC_VolumeCollider.h` and the five `InitQuery` bodies, and with them the
  vendored API that NovodeX callers use. It is left as an open item (below) rather than done here.

**3. Divergent (measured, attributed, not fixed).** These all belong to the summation-order and
register-lifetime work unit.

| Family | Mismatches, worst ulp | What differs, and where |
|---|---|---|
| `opcode_model_build_x87` | 2,316 words | Two causes. **(a) `SPLIT_SPLATTER_POINTS` over a mesh whose x and y variances tie in exact arithmetic** (the height field, the flat grid, the box). The tie is broken by rounding, and the two sides round differently:<br>• `AABBTreeOfTrianglesBuilder::GetSplittingValue` sums `(v2+v1)+v0` and returns the x87 register unrounded in the oracle (`0x000e9925`..`0x000e9933`); the candidate sums `(v0+v1)+v2` and rounds to float (`0x0009f5c1`..`0x0009f5ea`);<br>• `AABBTreeNode::Subdivide` keeps `1/n` unrounded in the oracle (`0x000f0b37`); the candidate stores it as a float (`0x000c4081`).<br>The first split that differs changes the tree. **(b) Every quantized tree.** The oracle keeps `32767/CMax` on the x87 stack and takes `mCenterCoeff = 1/that` (`0x000f31f3`..`0x000f32fd`); the candidate rounds in between, so a coefficient can differ in its last bit and a quantized box by one step. These are three-term sums and precision, not the three-product sums `sum_grouping.csv` records, so no site is listed there. Builds without a tie are `opcode_model_build`, which is exact. |
| `opcode_ray_x87` | 194 of 525 float words, 377 ulp | Hit distances and barycentrics: `OPC_RayTriOverlap.h`'s sums. There are 84 oracle three-product sites in the eight stab groups and 20 in `RayCollider::InitQuery`. The large ulp values are relative to barycentrics near 0 (absolute differences of about `6e-8`). The discrete outcome of the same queries (`opcode_ray`: hits, face ids, BV and primitive test counts, the cache) is exact. |
| `opcode_ray_boundary` | 1 (4 words shorter on the candidate side) | Rays aimed exactly at a vertex, along the plane of the root box's face, or through an edge midpoint. `RayAABBOverlap`'s `f = mDir.y*Dz - mDir.z*Dy` stays unrounded in the oracle (`0x000b912d`..`0x000b913f`); the candidate rounds it to float before `fabs` (`0x000a63ff`..`0x000a6407`). The compare predicates are the same: reject iff `abs(f) > r` on both sides. So one boundary ray passes the oracle's root test and fails the candidate's. |
| `opcode_treecollider_boundary` | 1 (14 words) | The flat grid against the height field, unrotated. Their vertices lie on the same x/y lines, so triangle edges meet exactly, and `TriTriOverlap`'s last bit decides 1 to 5 pair verdicts in five of the queries (381 oracle sites in the ten `_Collide` groups). The other pairs and placements (`opcode_treecollider`) are exact. |
| `ice_plane_triangle` | 947 of 4,000 | Cross products and `Normalize`. On a collinear triangle the oracle's unrounded residue is tiny but non-zero, so it normalises it; the candidate's is exactly zero, so it returns a zero normal. 5 sum sites. |
| `ice_matrix4x4` | 1,815 of 6,800 | Cofactor sums: up to 106 ulp in `Invert`, and a singular determinant that is `0` in the oracle against `-1e-16` in the candidate (129 sites). |
| `ice_obb` | 1,204 of 10,800, 512 ulp | Rotations in `ComputePoints`/`ComputePlanes` (21 sites). |
| `qhull_hull_x87` | 1,284 of 32,036 double words | Normals, offsets, centrums, `max_outside`/`min_vertex` and areas. `qh_distplane` is one of them (see "Summation order"). The combinatorial hull of every run in `qhull_hull` (52 runs over 9 option sets) is exact. |
| `qhull_hull_rotated` | 1,201 of 9,431 | `"o QR1"` rotates the input by qhull's random matrix (`qh_randommatrix`, `qh_gram_schmidt`, `qh_rotatepoints`). The rotation rounds differently, the lattice and the slab stop being exactly coplanar in different places, and the merges that follow differ. |

**4. Stock behaviour on both sides, avoided.** OPCODE 1.3 faults in three places:

- a single-triangle model has no tree, and `Collide` walks it when primitive tests are off;
- `BaseModel::Refit` dereferences the missing tree (`0x000e9418`);
- `AABBTreeCollider` does the same.

The oracle faults in each, and so would the candidate. The harness does not drive them.

### What still does not run

- **qhull: 151 groups, 70 of them with x87 code.**
  - About 30 are `io.c` printers for formats the NovodeX driver never asks for (geomview,
    Mathematica, Voronoi, summaries, statistics).
  - The rest are geometry and merge paths that only other options reach: Delaunay, half-space,
    joggle, projection, `qh_getdistance`.
  - `qh_initqhull_start` and `qh_initqhull_buffers` run inlined into `qh_init_A` in the exe, so
    their own bodies are not hit.
- **OPCODE: 38 groups, 5 of them with x87 code.**
  - `AABBTreeBuilder::GetSplittingValue` (the base version, which only the vertex builder
    reaches) and the two `AABBTreeOfAABBsBuilder` rows.
  - The inlined `AABBTreeCollider::Collide(tree, tree)` overloads and `SAP_PairData::Init`/
    `DumpPairs`. Their code runs inside the callers that inline them in both images, but their
    own bodies are not hit.
  - The four callback-variant rows. Their oracle rows have no candidate body of their own.
  - The `Walk` functions, and destructors the exe inlines.

## Task 5a: enforceable execution evidence

Task 4's review approved its differential and traces, but found five gaps to close before Task 5b
promotes any row on them:

- a divergent family could not fail;
- `exact+divergent` put last-bit float noise and different trees in one class;
- the `VolumeCache` layout was still stock;
- no family queried a candidate-built tree with the candidate's colliders;
- serialization had not been looked at.

Task 5a closes them.

### Divergent families have ceilings

Every tape word now records its kind: discrete, float, or one half of a double. The kind is not
part of any digest.

A divergent family's line (`verdict=divergent`) now reports how far apart the two tapes are:

| Field | Meaning |
|---|---|
| `mismatches` | differing words, plus the length difference |
| `discrete` | differing discrete words, plus 1 if the lengths differ |
| `float_ulp`, `double_ulp` | the largest distance, in representable values, over the differing floats and doubles (`inf` for a sign change, an infinity or a NaN) |
| `beyond` | how many of those floats and doubles are more than 4 ulp apart |
| `first_diff` | the first differing index, or where the shorter tape ends |
| `length_delta` | candidate words minus oracle words |
| `ceiling` | the recorded words/discrete ceiling |

stderr names the first differing word, with both values and kinds.

`kDivergentCeilings` in `tests/PhysicsThirdPartyTests.cpp` records each family's measured
`mismatches` and `discrete` counts. A run over either ceiling prints `verdict=FAILED` and fails the
harness; a run under both prints an `IMPROVED ... lower the ceiling` note. Each ceiling is today's
measurement, so today passes and any regression fails. This was checked by lowering
`opcode_ray_x87` to 193, which failed, and raising `ice_obb` to 1205/1, which gave the note. The
families are deterministic: repeated runs print byte-identical output.

| Family | Words | Mismatches (ceiling) | Discrete (ceiling) | float ulp | double ulp | Beyond 4 ulp | First difference |
|---|---:|---:|---:|---:|---:|---:|---|
| `opcode_model_build_x87` | 19,406 | 2,316 | 779 | inf | – | 1,517 | word 38: a node box, 2.0 against 3.0 |
| `opcode_ray_x87` | 525 | 194 | 0 | 377 | – | 21 | word 0: a hit distance, 1 ulp |
| `opcode_ray_boundary` | 3,284 | 927 | 756 | (out of step) | – | 68 | word 48, 1 ulp; the candidate tape is 4 words shorter |
| `opcode_treecollider_boundary` | 872 | 684 | 671 | 0 | – | 0 | word 37, a discrete word: 0x4f against 0x4e; 14 words shorter |
| `ice_plane_triangle` | 4,000 | 947 | 0 | inf | – | 116 | word 0, 2 ulp |
| `ice_matrix4x4` | 6,800 | 1,815 | 0 | inf | – | 128 | word 1, 6 ulp |
| `ice_obb` | 10,800 | 1,204 | 0 | 512 | – | 72 | word 60, 2 ulp |
| `qhull_hull_x87` | 32,036 | 1,284 | 0 | – | 3.4e15 | 7 | word 404, 1 ulp |
| `qhull_hull_rotated` | 9,431 | 1,201 | 582 | – | inf | 146 | word 5: the next facet id, 25 against 21 |
| `opcode_candidate_trees_ray` (new) | 292 | 18 | 1 | 14 | – | 2 | word 51: a hit distance, 3 ulp |
| `opcode_candidate_trees_x87` (new) | 18,694 | 2,489 | 2,436 | (out of step) | – | 9 | word 2: a ray's BV test count |

Notes on the table:

- `opcode_ray_boundary` and `opcode_candidate_trees_x87` are out of step after their first
  difference, so from there on their float "distances" compare unrelated words.
- (Task 5b wording fix.) The `discrete` figures of the two `*_boundary` families (756 and 671)
  and of `opcode_candidate_trees_x87` are **not** counts of discrete outcome mismatches. Each is
  a deterministic count, inflated by tape misalignment after the first difference: one root
  rejection for the rays (after which the candidate tape is 4 words shorter), one pair verdict for
  the tree colliders (14 words shorter), and a different tree for the candidate-built quantized
  and tied models. From that point the harness compares words position by position that no longer
  describe the same query. The ceiling holds the count because it is deterministic, not because
  it counts outcomes.
- Of `qhull_hull_x87`'s 1,284 differing words, only 7 doubles are more than 4 ulp apart. They are
  values next to zero.

### Outcome classes

`tools/vendored_trace.py` (22 tests) no longer produces `exact+divergent`. It classifies each
family from its own line:

| Class | Meaning |
|---|---|
| `exact` | No word differs. |
| `lastbit` | Divergent, but no discrete word differs, the tapes are the same length, and every differing float or double is at most 4 ulp from the oracle's. |
| `discrete` | Anything else: a differing count, index, verdict, tree link or quantized box; a length difference; a sign change; or a float more than 4 ulp away. That covers a different tree topology, different hit or pair verdicts, a zero normal, a zero determinant and different merges, whatever their ulp. |

**Why 4 ulp.** A three-term sum that the 2003 build adds in a different order, or keeps unrounded
where the 2026 `/fp:precise` build rounds, differs from the candidate's by one rounding of the
result. A few such operations in a row differ by a few units. 4 ulp allows two roundings on each
side. A value further apart than that has been through a cancellation or a different branch,
which is not a last-bit difference. The harness (`kLastBitUlp`) and the tool (`LASTBIT_ULP`) use
the same bound.

**Executions, not families.** Some drives fill several families from the same queries and report
them back to back. For example, `opcode_ray` gets the discrete outcome and `opcode_ray_x87` the
distances. So the trace segments of the later families hold no hits.

- An execution is a segment with hits, plus the hit-less sibling segments reported right after it.
- An execution's class is the worst class among its families: the floats it produced are part of
  its outcome.
- A group's `outcome` is its best execution. So `exact` means that at least one execution of the
  group was compared and matched completely.
- The column `family_best` keeps the looser reading: the best class of any family the group ran
  in, whatever else that execution fed.

**Harness changes for attribution.** To keep each family's executions in their own segment, the
harness now runs its interleaved drives in passes:

- the model builds: all of `opcode_model_build`'s, then all of the x87 family's;
- the tree-collider queries: the ordinary ones, then the boundary pair;
- each candidate-tree pass builds its own models.

Each tape keeps its order, and every registered line prints unchanged.

A new `release` mark, at `nxDriveSap`, closes the harness's release of the Task 4 models. That
release ran inside `opcode_sap`'s segment before and was credited to that family. Its segment,
like `END`, is now `uncompared`.

**Segments still shared.** Two drives still share a segment:

- the boundary rays share one with `opcode_ray`;
- the `QR1` runs share one with `qhull_hull`.

Neither changes a class: those executions already include a `discrete` family (`opcode_ray_x87`
and `qhull_hull_x87` respectively).

Groups by class, for the groups with x87 code. "Not executed" is `none`.

| Library | x87 groups | exact | lastbit | discrete | not executed | `family_best` = exact |
|---|---:|---:|---:|---:|---:|---:|
| qhull | 143 | 3 | 0 | 70 | 70 | 73 |
| OPCODE | 116 | 89 | 0 | 22 | 5 | 98 |

For all traced groups:

- qhull: 331 groups, 9 exact, 171 discrete, 151 none;
- OPCODE: 231 groups, 160 exact, 27 discrete, 1 `layout`, 5 `uncompared`, 38 none. The
  `uncompared` ones are `Model`'s and the four trees' deleting destructors.

x87 groups by match class:

| Library | Match class | x87 groups | exact | discrete | not executed |
|---|---|---:|---:|---:|---:|
| qhull | SHAPE | 48 | 3 | 16 | 29 |
| qhull | REVIEW | 63 | 0 | 38 | 25 |
| qhull | DIFF | 32 | 0 | 16 | 16 |
| OPCODE | SHAPE | 15 | 8 | 5 | 2 |
| OPCODE | REVIEW | 96 | 80 | 16 | 0 |
| OPCODE | DIFF | 5 | 1 | 1 | 3 |

Reading these counts:

- **No family is `lastbit`.** Every divergent family has at least one float more than 4 ulp away,
  or a discrete difference. So with the stated bound, "divergent" and "discrete" name the same
  families today.
- **Every qhull group that runs in the hull is `discrete`.** Its only execution also fills
  `qhull_hull_x87`, and that family has 7 doubles past the bound. The combinatorial hull
  (`qhull_hull`) is exact, which is why `family_best` gives 73.
- **OPCODE's 22 discrete x87 groups are:**
  - the eight ray-stab walks and `ValidateSettings`. Every execution of them also feeds a float
    family: `opcode_ray_x87`, or the candidate-tree rays;
  - `AABBQuantized*Tree::Build`, whose coefficient rounds differently;
  - the ICE plane, triangle, matrix and OBB rows.
- **Before this split**, 36 OPCODE and 70 qhull x87 groups had been `exact+divergent`. Now:
  - the builders are `exact`, because the model builds' split into passes gives them an exact
    execution;
  - the ray walks are `discrete`;
  - the tree colliders, `CoplanarTriTri` and `OBB::IsInside` are `exact` through their own exact
    families.

### VolumeCache: fixed

The oracle's cache constructor is inlined into its owners. At `0x000e56d0` a NovodeX object:

- constructs a `SphereCache` at `+0x50` and an `AABBCache` at `+0x6c`;
- zeroes both of their first words (`0x000e56ef`, `0x000e56f2`, `0x000e5709`, `0x000e570c`);
- then points both at its own `Container` at `+0x40` (`0x000e5731`, `0x000e5734`).

`0x00059832`..`0x00059846` builds an `AABBCache` on the stack the same way.

None of the five `InitQuery` bodies tests the pointer. Each loads it and stores it in
`mTouchedPrimitives`:

| Collider | Address |
|---|---|
| LSS | `0x000d36f2` |
| OBB | `0x000d57ab` |
| Sphere | `0x000de925` |
| Planes | `0x000e173d` |
| AABB | `0x000e9c00` |

The fix:

- **The cache itself.** The new overlay `OPC_VolumeCollider.h` makes `VolumeCache`
  `{Container* TouchedPrimitives; const BaseModel* Model;}`, with a constructor that nulls both.
- **The three stock colliders.** The new overlays `OPC_SphereCollider.cpp`, `OPC_OBBCollider.cpp`
  and `OPC_PlanesCollider.cpp` are upstream copies, CRLF like upstream. Each changes only its
  `InitQuery` assignment and its hybrid `Reset`/assign pair.
- **The two existing overlays.** `OPC_AABBCollider.cpp` and `OPC_LSSCollider.cpp` take the same
  change.
- **The record.** `MODIFICATIONS.md` has the entries. `verify_vendored_sources` passes with 36
  locally modified files.

The harness now gives both sides the one vendored cache type, each pointing at its own
`Container`, and the oracle-only image is gone. All volume families still compare exactly, with
the same digests. The candidate is now `b0e275ae…b05d`.

The matcher was re-run. The class of every row is unchanged. The field reports of the five
`InitQuery` rows changed, and their review notes record the fix. `PlanesCollider::InitQuery`'s
`diff_fields` changed from empty to base-tag differences (`this` against `derived`), which Task 5b
has to re-review.

This rerun is also the first since Task 4's `AABBCollider` overlay, so `opcode_match.csv` carries
that change too. `sum_grouping.csv` was regenerated; the grouping counts are unchanged and only
the candidate addresses moved.

### Candidate-built trees, end to end

The new families build every mesh in every tree kind on both sides. Each query then runs:

- the oracle's collider on the oracle's model;
- the candidate's collider on the candidate's model.

Both sides get the same inputs. The queries cover rays of `opcode_ray`'s three shapes (not the
ones aimed at a boundary), the five volume colliders (primitive tests off and temporal coherence
included), and tree-versus-tree pairs in three placements. Each query's discrete outcome, the
caches' derived fields, and the ray hits' distances and barycentrics are compared.

| Family | Models | Result |
|---|---|---|
| `opcode_candidate_trees` | Volume colliders and tree pairs on the models `opcode_model_build` builds exactly: untied and not quantized, which means the soup, the degenerate set and the single triangle | **exact**, 3,674 words |
| `opcode_candidate_trees_ray` | Rays on the same models, in their own segment | divergent: 18 of 292 words, 1 discrete; ceiling 18/1 |
| `opcode_candidate_trees_x87` | Everything on the quantized trees and the tied meshes (height field, flat grid, box), and every pair that includes one | divergent: 2,489 of 18,694 words, 2,436 discrete (test counts, hits, pairs); ceiling 2,489/2,436 |

**The ray divergence comes from the collider, not the tree.**

- 17 of the ray family's differences are hit floats: `opcode_ray_x87`'s summation order.
- The one discrete difference is a ray's BV test count: 6 in the oracle, 14 in the candidate.
- When the candidate's collider was made to query the *oracle's* model instead, the same count
  came out. So it is the collider's own last bit (`opcode_ray_boundary`'s `RayAABBOverlap` `f`
  rounding) on a random ray that happens to graze a box.
- A first version of this family taped the rays' discrete outcome only, and with other random
  draws it came out exact. That would have credited the ray stab groups with an `exact` that
  said nothing about their floats, so the rays now have a family of their own that compares
  everything.

**What this shows.** Where the build is exact, a candidate tree queried by the candidate's volume
and tree colliders gives the oracle's answers. Where the build ties or quantizes, it does not,
which `opcode_model_build_x87` already predicted. That divergence is now held by a ceiling.

**Registration.**

- The families are registered by appending: the exact line whole, the two divergent ones up to
  their digests, and a third pair of totals lines (`driven=47 divergent=11 words=486946`,
  digest `5f87aa37`).
- The Phase 4 floor goes from 130 to 135.
- Earlier in this task these lines were registered once, in `24cfafd`, before the rays had a
  family of their own. Commit `7835de6` replaces those lines; against the Task 4 head the
  registry is still append-only.
- The totals now print from one helper, and the divergent report is one printf literal. So each
  registration is a prefix of exactly one harness format, which `test_gate_targets` checks.
  Task 4's second copy of the totals printf had broken that check when it was run against the
  worktree's harness.

### Serialization: not executed

The candidate's `BaseModel::Save` and `BaseModel::Load` are the reporting stubs in
`novodex/OPC_BaseModel.cpp`, and `Model::Build`'s deserialize arm calls `Load`. The four trees'
serialization rows are not written. So no candidate deserialization exists to compare, and no
harness drives it. Serialization (17 rows, 1,277 bytes) stays unexecuted, as the open item says.

### Evidence

The trace was re-run on the Task 5a build:

| Binary | sha256 |
|---|---|
| Candidate DLL | `b0e275ae70e5f381d251a582c300ffd9778c2442d14ddca1deae6e29f419b05d` |
| `NxPhysicsThirdPartyTests.exe` | `0d3551211a2a409c93b932036f64e0a5b14330aad6bd919c5012f967741b7dbc` |
| `NxPhysicsAssetTests.exe` | `8e77583c50343cbd15c0727986c1585eb2fe23a1bf4f4ea3873a30ff162b1418` |

- The ThirdParty trace has 50 segments; the asset trace, 1.
- Identity again finds every traced body the same code, except the `AABBTreeBuilder` deleting
  destructor.
- The groups executed are the same 373 as in Task 4.
- The results are in `vendored-trace-{qhull,opcode}.txt` and `vendored_coverage.csv` (columns
  `outcome` and `family_best`).

## Task 5b: promotion

Task 5b closes the four prerequisites the Task 5a review left, then moves the rows the evidence
supports from `discovered` to `reconstructed`.

### Prerequisites

1. **The gate holds the figures a proof cites** (`426c0a7`). `kDivergentCeilings` in
   `tests/PhysicsThirdPartyTests.cpp` now caps, per divergent family:
   - `words` and `discrete`, as before;
   - the worst float ulp and the worst double ulp;
   - `beyond`, the words more than 4 ulp apart;
   - `inf_words`, the floats and doubles an infinite distance apart;
   - `degenerate`, described below;
   - `finite_ulp`, the worst finite distance;
   - `beyond_abs`, the largest `|oracle - candidate|` over the `beyond` words. This is the bound
     that matters for values next to zero, where a large ulp distance is a tiny difference.

   A run over any cap fails the harness. Lowering `ice_obb`'s `beyond` to 71 and
   `qhull_hull_x87`'s `beyond_abs` to 2.9e-12 made the run fail on exactly those two families.
   Each word beyond 4 ulp is listed on stderr (`ULP_BEYOND`).
2. **Degenerate ICE inputs are named, not averaged in.** The ICE drive builds some inputs
   degenerate on purpose: a triangle with a repeated vertex (`c % 7 == 2`) or three collinear
   points (`c % 13 == 5`), and a matrix whose row 2 is twice row 1 (`c % 8 == 3`). Their tape words
   now carry a mark. A differing marked word is counted as `degenerate=`, listed on stderr
   (`ULP_DEGENERATE`), and kept out of the distance figures. The marking is by construction,
   decided before either side runs, so it cannot hide a regular input. Every infinite distance in
   `ice_plane_triangle` (31) and `ice_matrix4x4` (8) was on such an input. The cases whose words
   differ (29 triangles, 17 matrices) are listed in `vendored-trace-opcode.txt`.
3. **`lea r,[r]` is a copy** (`9425d75`). The candidate aligns a loop in
   `PlanesCollider::InitQuery` with `lea ebx,[ebx]` (`0x000aba8a`). `_base_class` read that as a
   derived pointer, which made the field report Task 5a flagged. `lea r,[s]` and `lea r,[s+0]`
   (no index, not a frame register) now trace through to `s`. One test was added, for 82 in all.
   The re-run changes no row's class; 44 qhull and 8 OPCODE rows change only their base-class
   tokens, which the hand reviews already ignore. `PlanesCollider::InitQuery`'s `diff_fields`
   is empty again. Its review line records the re-review: every `[ebx+d]` after `0x000aba8a` is
   `this+d`, and the verdict `equivalent` stands.
4. **Candidate-only builds fail.** `nxDriveCandidateTrees` used to skip a model that the oracle
   declined to build and the candidate built. It now fails on it.
5. **The QR1 runs have a segment of their own.** The qhull drive reports `qhull_hull` and
   `qhull_hull_x87` before its `"o QR1"` runs. The tapes, the draws and the report order are
   unchanged. The re-trace (`e172256`, exe `6658d086...`, candidate DLL `b0e275ae...` unchanged)
   shows 16 qhull groups that only the QR1 runs reach: `qh_randommatrix`, `qh_gram_schmidt`,
   `qh_rotateinput`, and merge paths such as `qh_renamevertex` and `qh_find_newvertex`. Those now
   read `family_best=discrete`. No other group changes its outcome.
6. **Wording.** The `discrete` counts of the `*_boundary` families and of
   `opcode_candidate_trees_x87` are described as what they are, in the Task 5a notes above:
   deterministic counts inflated by tape misalignment after the first root or verdict difference.

Every registered line prints unchanged, and the run is deterministic.

### The rule applied

A group's `discovered` rows are promoted when the group meets all of these:

- it is MATCH, SHAPE or REVIEW;
- its `{qhull,opcode}_review.csv` verdict is `equivalent`;
- it has no open triage item;
- it meets one of the arms below.

**Arm (i), exact.** At least one execution of the group was compared with the oracle and matched
in every word.

**Arm (ii), outcome-exact.** Some execution of the group meets all of these:

- **(a)** its discrete outcome is exact: a sibling family is exact, or, for `ice_obb`, the
  family's own 600 discrete words (three return values per input) are all exact. A family with no
  discrete words at all makes (a) vacuous, and does not qualify (final review, below);
- **(b)** its float family has `discrete=0` and `length_delta=0` (and `inf_words=0`);
- **(c)** the proof states that family's worst ulp, `beyond` and `beyond_abs`;
- **(d)** the gate holds those figures (prerequisite 1);
- **(e)** the proof lists, with attribution, every discrete-mismatch family the group also ran
  in.

The executions that qualify are:

| Execution | Exact part | Float family |
|---|---|---|
| The qhull hull | `qhull_hull` | `qhull_hull_x87` |
| The rays | `opcode_ray` | `opcode_ray_x87` (`opcode_ray_boundary` shares the segment and is attributed) |
| The OBB maths | `ice_obb`'s own discrete words | `ice_obb` |

**Arm static.** The static-only policy class: no x87 code, no `inlining:` note and no
other-immediates note, with a hand review. Such a group is promoted even if nothing executes it.

**Held back, even when an arm would admit the group:**

- DIFF groups.
- Reviews marked `equivalent-option-gated`: a numeric difference remains under options the
  NovodeX driver never passes.
- Groups whose only compared executions have discrete-outcome mismatches: the QR1-only qhull
  paths, and the quantized trees' constructors, `GetUsedBytes` and `Build`.
- The two `AABBTreeOfTrianglesBuilder::GetSplittingValue` overloads and
  `AABBTreeNode::Subdivide`. They have exact executions, but Task 4 attributes the tie divergence
  in `opcode_model_build_x87` to their own arithmetic: the `(v2+v1)+v0` sum returned unrounded,
  and `1/n`.
- Unexecuted groups with x87 code.
- Unexecuted groups with an inlining or other-immediate note.
- **The ICE plane, triangle and matrix groups (final review of the plan).** `ice_plane_triangle`
  and `ice_matrix4x4` tape no discrete words, so (a) is vacuous for them and the promotion
  would rest on the float bounds alone. Those bounds are not last-bit. Outside the excluded
  degenerate cases:

  | Family | Worst ulp | Words beyond 4 ulp | Largest absolute difference |
  |---|---:|---:|---|
  | `ice_plane_triangle` | 2,820 | 39 | 8.34e-07 |
  | `ice_matrix4x4` | 106 | 109 | 0.00128, on inverse entries up to 186 in magnitude (the cofactor sums are reassociated) |

  Task 5b had promoted these eight rows under (ii). The final review held them back and returned
  them to `discovered` / `vendored_not_falsified`; each row's `notes` keeps the proof Task 5b had
  written and says why it is held back:
  - `Plane::Set` (`phys_fn_005155`);
  - `Triangle::Area`, `Normal`, `Center` and `Inflate` (`005179`, `005181`, `005183`, `005185`);
  - `Matrix4x4::CoFactor`, `Determinant` and `Invert` (`005193`, `005195`, `005197`).

  That is 1,590 bytes.

**What each promoted row carries:**

- `state`: `reconstructed`.
- `implementation` and `source`: the file that defines the function, as the build merges it. That
  is the overlay under `External/*/novodex/` where one exists, otherwise the upstream file. A
  compiler-generated deleting destructor names the file of the destructor it wraps.
- `implementation_symbol`: the function's source name. The upstream files do not write stable
  IDs, so `_check_implementation_contains_row` accepts the symbol, as it does for the other rows
  that record one.
- `static_proof`, which states:
  - the matcher class and the features it compares;
  - the "Not compared" list;
  - that summation order and some register lifetimes are not reproduced, with the group's
    `sum_grouping.csv` sites;
  - the review verdict, with its addresses;
  - the execution class and its figures;
  - every divergent float family the group also ran in, with its figures;
  - the discrete-mismatch families with their attribution;
  - the evidence files.

  For a collider on quantized nodes that also ran over candidate-built trees, the proof says
  that it matches on oracle-built quantized trees and differs end to end, because the build
  rows are not promoted.
- `dynamic_proof`: only on rows whose group has trace hits. It cites
  `evidence/vendored-trace-{qhull,opcode}.txt`, the hit counts and families, the sha256 of both
  traced exes and of the candidate DLL, and the identity result. It also says that those pinned
  binaries predate the plan's final clean link, which changes the link timestamp only.

**Attribution quoted in the proofs:**

| Family | Attribution |
|---|---|
| `opcode_ray_boundary` | The oracle keeps `RayAABBOverlap`'s `f` unrounded (`0x000b912d`..`0x000b913f`) and the candidate rounds it. That flips one root rejection, and the count after it is misalignment. |
| `opcode_candidate_trees_ray` | One grazing ray's BV test count, 6 against 14. The same count appears when the candidate's collider queries the oracle's tree. |
| `opcode_candidate_trees_x87` | The unpromoted build rows make different quantized and tied trees. |
| `opcode_model_build_x87` | Ties and quantization in the builds. |
| `opcode_treecollider_boundary` | Edges aligned exactly: `TriTriOverlap`'s sums flip a pair verdict. |
| `qhull_hull_rotated` | QR1's rotation rounds differently, and the merges that follow differ. |

**The float figures the proofs cite.** The gate caps each of these at today's value.

| Family | Worst ulp | `beyond` | `beyond_abs` | Degenerate words |
|---|---:|---:|---:|---|
| `qhull_hull_x87` | 3,421,917,482,582,016 (double) | 7 | 2.96e-12 | none |
| `opcode_ray_x87` | 377 | 21 | 5.14e-07 | none |
| `ice_plane_triangle` | 2,820 | 39 | 8.34e-07 | 122, from 29 triangles |
| `ice_matrix4x4` | 106 | 109 | 0.00128 | 19, from 17 matrices |
| `ice_obb` | 512 | 72 | 2.38e-07 | none |

The seven `qhull_hull_x87` words beyond 4 ulp are all values of order 1e-16 to 1e-11:
distances and offsets that are zero in exact arithmetic. `ice_matrix4x4`'s absolute bound is
on inverse entries up to 186 in magnitude.

## Results

### Rows promoted (`9658600`, amended by the final review)

**By library:**

| Library | Arm | Groups | Rows | Bytes |
|---|---|---:|---:|---:|
| qhull | (i) exact | 3 | 3 | 248 |
| qhull | (ii) outcome-exact | 130 | 169 | 59,884 |
| qhull | static-only | 28 | 30 | 3,808 |
| qhull | **total** | **161** | **202** | **63,940** |
| OPCODE | (i) exact | 94 | 126 | 149,043 |
| OPCODE | (ii) outcome-exact | 11 | 13 | 18,239 |
| OPCODE | static-only | 10 | 10 | 657 |
| OPCODE | **total** | **115** | **149** | **167,939** |
| **both** | | **276** | **351** | **231,879** |

**By match class:**

| Library | Arm | MATCH | SHAPE | REVIEW |
|---|---|---|---|---|
| qhull | (i) | – | 3 rows / 248 B | – |
| qhull | (ii) | – | 68 / 23,776 | 101 / 36,108 |
| qhull | static | – | 19 / 2,022 | 11 / 1,786 |
| OPCODE | (i) | 3 / 61 | 22 / 8,317 | 101 / 140,665 |
| OPCODE | (ii) | – | – | 13 / 18,239 |
| OPCODE | static | 5 / 69 | – | 5 / 588 |

**What the arms contain:**

- OPCODE's (ii) groups are:
  - the eight `_RayStab`/`_SegmentStab` walks and
    `RayCollider::Collide(const Ray&, const Model&, ...)`;
  - `OBB::ComputePlanes` and `ComputePoints`.

  The plane, triangle and matrix groups are held back (see "Held back").

  `RayCollider::ValidateSettings` meets the rule too, but its row already stood at
  `reconstructed` and is left there.
- qhull's (ii) groups are every hull group that ran in the unrotated runs.
- The static arm holds:
  - the destructors;
  - qhull printers, set and statistics helpers with no x87 code and no notes, which nothing
    compares.

**The ledger** (`gates/phase4-closure.json`) moves 351 deferrals from `vendored_not_falsified`
to `reconstructed_not_falsified`:

- `vendored_not_falsified`: 608 to 257;
- `reconstructed_not_falsified`: 111 to 462.

Task 5b had moved 359. The final review returned the eight ICE rows.

Existing notes are kept. The prose counts are updated. `validate_inventory` requires that the one
promoted row whose source had been `Physics/src/opcode/OPC_MeshInterface.cpp` (`phys_fn_005358`,
`MeshInterface::SetPointers`) leave the unresolved-source allowlist, so its entry is removed.

### Rows left at `discovered`, by reason

| Library | Reason | Groups | Rows | Bytes |
|---|---|---:|---:|---:|
| qhull | DIFF (46 equivalent and 1 option-gated in review; not promotable under the rule) | 47 | 96 | 45,975 |
| qhull | not executed, x87 code | 48 | 65 | 19,238 |
| qhull | not executed, inlining or other-immediate note | 42 | 59 | 12,704 |
| qhull | only compared execution is QR1 (discrete) | 10 | 10 | 2,074 |
| qhull | review `equivalent-option-gated` | 8 | 8 | 3,683 |
| qhull | **total** | | **238** | **83,674** |
| OPCODE | DIFF, including the four callback-variant groups (`novodex-variant-unwritten`) | 10 | 10 | 27,664 |
| OPCODE | not executed, inlining or other-immediate note | 1 | 1 | 142 |
| OPCODE | held back in the final review: the ICE plane/triangle and matrix groups (no discrete words; float divergence not last-bit) | 8 | 8 | 1,590 |
| OPCODE | **total** | | **19** | **29,396** |

**Rows already above `discovered`.** In the matched groups, 82 OPCODE rows stand at
`classified`, 35 vendored rows (20 OPCODE, 15 qhull) at `reconstructed` and 3 OPCODE rows at
`dynamically_gated`. They are left as
they are. This includes:

- the quantized trees;
- `GetSplittingValue`;
- `Subdivide`;
- the remaining ICE rows.

### Deferred units (not vendored-correspondence rows)

- **Serialization: 17 rows, 1,277 bytes.**
  - The rows: BaseModel's slots 4 to 6, and the four trees' `GetSerialSize`, save, load and
    relocation rows.
  - They need MemoryStream host seams and a direct-oracle differential. The candidate has only
    the reporting stubs.
  - The reconstructed `TriangleMesh.cpp` save path and `Model::Build`'s load dispatch reach them.
  - 12 of them are unmapped `discovered` rows in the OPCODE span.
- **The tree-collider callback variant: 4 groups, 25,613 bytes.**
  - The rows are `0x000d12b0`, `0x000cd700`, `0x000ca5a0` and `0x000cbe50`.
  - They are to be written with their caller `phys_fn_001876`, NovodeX's mesh-mesh contact.
- **The NovodeX hull library: 31 qhull-span rows, `discovered`, 11,641 bytes.**
  - These are the drivers `003279`/`003236`, the clean-up, the output arena, the OBJ writers and
    band B.
  - They form bundle `gap:Controller.cpp..fluids\Fluid.cpp`, with their only caller
    `phys_fn_002233` in `TriangleMesh.cpp`.
- **The unmapped NovodeX clusters in the OPCODE span: 92 `discovered` rows.**
  - Measured here as 105 unmapped `discovered` rows, less the 12 serialization rows and the
    stock `Matrix3x3` cast. The coordinator's figure was 91.
  - The clusters, with the units of their direct callers (from `oracle/dependencies.dot` and
    `work_units.json`):

    | Cluster | Rows | Bytes | Direct callers' units |
    |---|---:|---:|---|
    | pruner A | 22 | 2,373 | `gap:core\NpPrismaticJoint.cpp..opcode\IcePrunable.cpp` (`phys_fn_004852`), `gap:ContactPlaneMesh.cpp..PenetrationMap.cpp` (`001967`, `001969`) |
    | pruner B | 6 | 2,484 | none by direct call (vtable dispatch) |
    | SweepAndPrune box dump | 1 | 186 | `gap:ContactPlaneMesh.cpp..PenetrationMap.cpp` (`001978`) |
    | pruning owner and pool | 9 | 2,272 | `gap:ContactPlaneMesh.cpp..PenetrationMap.cpp` (`001967`) |
    | penetration-map builder | 14 | 4,805 | `PenetrationMap.cpp` (`002047`), `gap:ContactPlaneMesh.cpp..PenetrationMap.cpp` (`002025`) |
    | pruner C | 22 | 4,420 | `gap:core\NpPrismaticJoint.cpp..opcode\IcePrunable.cpp` (`004830`, `004832`, `004834`, `004852`, `004855`) |
    | pruner D | 18 | 2,976 | none by direct call (vtable dispatch) |
- **Summation order and register lifetimes: their own tool-driven unit.** It covers several
  hundred OPCODE sites and at least `qh_distplane`, and is driven by `sum_grouping.csv` extended
  to offset-led and longer sums. Every promoted row's proof records this as not reproduced.

  The unit's scope also takes the degenerate-input differences the ICE drive marks. Both come
  from where the oracle keeps an unrounded residue:
  - **Zero-area triangles:** 29 inputs. The candidate's normal is exactly zero; the oracle
    normalises its unrounded non-zero residue.
  - **Singular matrices:** 17 inputs. The oracle's determinant is 0; the candidate's is about
    -1e-16.

  Their float divergence outside those cases (2,820 and 106 ulp) is why the eight plane,
  triangle and matrix rows are held back.

### Defects found and fixed during the plan

| Task | Commit | Defect |
|---|---|---|
| 2 | `e6cc359` | qhull's trace macros must print through the CRT, and `mem.c`/`qset.c` through the host |
| 2 | `93c0e1d` | `__CIsqrt` where the oracle has an inline `fsqrt` (qhull) |
| 2 | `18774f3` | NovodeX's typed host dispatches in `io.c`/`poly2.c` |
| 2 | `2d823f3` | `geom.c` reciprocal build parity (`qh_gausselim`, `qh_getcenter`, `qh_normalize2`) |
| 3 | `6e14ab6` | `OPCODECREATE::mDeserializeFrom` uninitialised |
| 3 | `dbd3172`, `e4f9f73` | RayCollider's `+0x88` barycentric tolerance, and V compared unrounded |
| 3 | `9266906` | OPCODE's allocator by class, and empty node constructors |
| 3 | `85004e8` | `CONTAINER_STATS` compiled in |
| 3 | `d6ffcb3` | `__CIsqrt` (OPCODE) |
| 3 | `05d81a9` | NovodeX `SweepAndPrune` |
| 3 | `19a9844` | NovodeX `LSSCollider` |
| 3 | `ba36f82` | `SphereTriOverlap`'s reciprocal form and register lifetime |
| 4 | `045ed3b` | stock `AABBCollider::_Collide(const AABBTreeNode*)` argument swap |
| 5a | `1aa9092` | NovodeX `VolumeCache` holds `Container*` at +0 |

The matcher and harness defects found along the way:

- the high-byte bit tests (`a1e5428`);
- the withdrawn `-1.0` rule (`ba36f82`);
- the `fxch` register (`e44b61c`);
- a divergent family that could not fail (`82d5320`);
- the `lea` copy (`9425d75`);
- the QR1 segment and the candidate-only build skip (`426c0a7`).

### Rate

Of the 728 vendored map rows (qhull 455, OPCODE 273), 386 now stand at `reconstructed`: 351 from
this task, and 35 raised earlier by other drives. That is 231,879 of the 380,693 matched bytes
(61%) promoted by this plan.

The plan's recorded wall time, Task 1 to the end of Task 5b, is 7.01 hours (the nine timing rows, 2026-09-27T15:24:12 to 2026-09-28T00:08:17). That gives
about 33,072 bytes per hour for the bytes promoted.

### Verification

| Check | Result |
|---|---|
| Fresh configure | `cmake -G "Visual Studio 18 2026" -A Win32 --fresh`: exit 0. |
| Clean build | `cmake --build build --config Release --clean-first`: exit 0, no errors. |
| Candidate DLL after the clean build | sha256 `f9075db4...03ef`. It differs from Task 5a's `b0e275ae...` only by the link: the PE timestamp changes with every link. The matcher re-run over it (`--out-dir` scratch) gives `qhull_match.csv`, `opcode_match.csv` and `vendored_data_map.csv` byte-identical to the committed ones, candidate addresses and sizes included. The gates' own `--fresh` rebuild gave the same `f9075db4...`. |
| Test exes after the clean build | `NxPhysicsThirdPartyTests.exe` `84b6c878...` and `NxPhysicsAssetTests.exe` `ae2d812b...`. The traces pin `6658d086...` and `8e77583c...`, linked before the clean build. As with the DLL, the difference is the link timestamp only, and the Phase 4 gate ran the relinked pair with the same digests. |
| Public headers | `public_headers=pass` against the oracle tree and the worktree's `Physics/include` (in every gate run). |
| Tool tests | `python -m unittest discover -s tests`: 753 tests OK (752 + the new `lea` test). `test_gate_targets` against the worktree harness: OK. |
| `verify_vendored_sources.py` | pass: 148 upstream files checked, 36 locally modified, 0 failures. |
| `validate_inventory.py` | exit 0, `unexplained=0`; phase 4 `closed=28 deferred=1025`. |
| Phase 2 | `phase_gate=2 status=pass` |
| Phase 3 | `phase_gate=3 status=pass`, 103/103 |
| Phase 4 | `phase_gate=4 status=pass`, 135/135; asset `eaefc573`; thirdparty digests `74ebc669`, `c16f0c0c`, `5f87aa37`; `thirdparty candidate mismatches=0`. |
| Phase 5 | Fails only on `candidate CANDIDATE-MISSING family=vtables`, and the layout harness exits 1 because of it, as before. 871/871. |
| Phase 6 | `phase_gate=6 status=pass`, 403/403 |
| Phase 7 | `phase_gate=7 status=pass`, 276/276 |
| `NxPhysicsThirdPartyTests` | Exit 0 and deterministic: two runs are byte-identical on stdout and stderr. Every exact line, and every registered prefix, is unchanged from Task 5a. |

## Open items

- **The NovodeX hull library (separate work unit, controller decision after Task 2).** The 31
  `discovered` unmapped rows in the qhull span are NovodeX's own code: 12,166 bytes, 3,903
  instructions, 750 of them x87. They are listed in the Task 2 section:
  - the drivers `003279` and `003236`, and the entry `003255`;
  - the clean-up `003243`/`003245`;
  - the output-arena class behind `.rdata:0x00113614`;
  - the OBJ writers;
  - band B at `0x0007fda0`-`0x000814f0`.

  They are not vendored-correspondence rows. They get their own work unit (bundle
  `gap:Controller.cpp..fluids\Fluid.cpp`), reconstructed together with their only caller
  `phys_fn_002233` and a differential through it. Until then the qhull host hooks in
  `Physics/src/ThirdPartyHost.cpp` stay shims.
- **Summation order, and per-instantiation register lifetimes: separate work unit** (user decision
  after the Task 3 review; see "Summation order"). It is driven by `sum_grouping.csv`, with the
  tool extended to offset-led and longer sums, and covers several hundred OPCODE sites and at least
  `qh_distplane` in qhull. Until it lands, Task 5's static proofs for vendored rows state that
  summation order and some register lifetimes are not reproduced, and rows with x87 arithmetic
  still need execution evidence.
- **The NovodeX `VolumeCache` layout: fixed in Task 5a.** The cache holds a `Container*` at `+0`
  and the owning model at `+4`. The owner supplies the `Container`, and the colliders never test
  the pointer. The fix is `novodex/OPC_VolumeCollider.h` plus the five `InitQuery` overlays; see
  "Task 5a". `PlanesCollider::InitQuery`'s matcher field report changed with the rebuild, and
  Task 5b has to re-review it. *Done in Task 5b:* a matcher artifact of the candidate's
  alignment no-op `lea ebx,[ebx]`, fixed in the matcher (`9425d75`); the verdict stands.
- **Execution gaps after Task 4.** qhull has 70 x87 groups that no differential runs, most of them
  printers for output formats the NovodeX driver never requests. OPCODE has 5: the base
  `GetSplittingValue`, the two `AABBTreeOfAABBsBuilder` rows, and two bodies the exes inline. See
  "What still does not run". A static proof covers these, or nothing does.
- **OPCODE, after Task 3 (separate work units; see the Task 3 section).**
  - The NovodeX callback instantiation of the no-leaf tree-versus-tree collider:
    - rows `0x000d12b0`, `0x000cd700`, `0x000ca5a0`, `0x000cbe50` (25,613 bytes);
    - to be written with its caller `phys_fn_001876`.
  - Serialization, 17 rows and 1,277 bytes:
    - BaseModel slots 4 to 6, and the four trees' getSerialSize, save, load and relocation rows;
    - they need MemoryStream host seams and a direct-oracle differential;
    - their stubs are reachable from the reconstructed `TriangleMesh.cpp` save path and from
      `Model::Build`'s load dispatch;
    - Task 5a confirmed that nothing executes them: the candidate side has only the reporting
      stubs, so no differential can compare a deserialize.
  - The NovodeX pruner, penetration-map and SweepAndPrune-dump clusters in the OPCODE span, with
    the scene-query and mesh units that own their callers.
  - `0x000f1350`, the ICE culling walk: typed `compiler_artifact` but real code.
  - RayCollider's `+0x88` tolerance has no setter in the overlay; the scene raycast
    (`0x0002929b`) writes it from outside the class, so whoever reconstructs that raycast adds one.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-27T15:24:12 | 2026-09-27T15:56:00 | 0 | 0 | Structural matcher `tools/vendored_match.py` + 29 unit tests; first run over 455 qhull and 267 OPCODE rows (qhull MATCH 2 / SHAPE 177 / DIFF 276; OPCODE MATCH 19 / SHAPE 174 / DIFF 72 / MISSING 1 / AMBIGUOUS 1). Top DIFF causes: qhull trace macros on the CRT in the oracle (127 groups), mem.c/qset.c on the CRT in the candidate (21), OPCODE allocator not redirected (25). No product code or ledger change; gates 2, 3, 4, 6, 7 pass, Phase 5 red only on `CANDIDATE-MISSING family=vtables`. |
| 1 review | 2026-09-27T16:06:39 | 2026-09-27T16:30:00 | 0 | 0 | Matcher strengthened after review: REVIEW class (field byte coverage, logic/compare immediates with bit-address normalisation), MAPCHECK class, overload-aware call keys and real operator names, icall base tags, float rule narrowed to reciprocal/halving and fchs-backed negation, repe cmps, ebp-frame note, rank pairs need two votes; opcode_map.csv 0x000e90e0 and 0x000f0890 corrected; tools/qhull_trace_attribution.py committed; 58 matcher tests. qhull MATCH 0 / SHAPE 88 / REVIEW 80 / DIFF 287; OPCODE MATCH 17 / SHAPE 60 / REVIEW 116 / DIFF 72 / MISSING 1 / AMBIGUOUS 1. OPCODECREATE ctor and the 8 RayCollider stab rows are REVIEW. No product code or ledger change. |
| 1 final | 2026-09-27T16:33:00 | 2026-09-27T16:44:39 | 0 | 0 | Router approved; final changes: x87 operation classes as a REVIEW feature (6 SHAPE groups flipped, all explained: fabs_ macro x3, reciprocal x3); coverage-based narrowing replaces the same-dword downgrade (adjacent one-byte fields stay REVIEW); field and bit tokens carry a base class (this / derived / other, derived and other merged for classification) traced through copies and ebp spills; register read-modify-write and/or/xor normalised to the memory form; report-only report_jcc and report_stores columns; "Not compared" and "Promotion policy for Task 5" sections; 69 matcher tests. qhull SHAPE 83 / REVIEW 85 / DIFF 287; OPCODE MATCH 17 / SHAPE 49 / REVIEW 127 / DIFF 72 / MISSING 1 / AMBIGUOUS 1. No product code or ledger change. |
| 2 | 2026-09-27T16:45:42 | 2026-09-27T17:56:47 | 0 | 0 | qhull triage. Fixes: trace macros on the CRT and mem.c/qset.c prints on the host (qhull_a.h, mem.h, host header), inline fsqrt (/Qfast_transcendentals on NxQhull), io.c/poly2.c typed host dispatches (+0x00/+0x04/+0x08/+0x0c/+0x1c), geom.c reciprocal parity for qh_gausselim/qh_getcenter/qh_normalize2, map row 0x0007f310 corrected to qh_settempfree_all; matcher: high-byte bit tests, new seams, two CRT identities, 71 tests. qhull SHAPE 83->162 / REVIEW 85->197 / DIFF 287->96 rows; qhull_review.csv covers all 331 groups (no unreviewed line). Unmapped NovodeX rows (31 discovered, 3,903 insns) not written: need their own unit. No ledger change; gates 2, 3, 4, 6, 7 pass, Phase 5 red only on `CANDIDATE-MISSING family=vtables`. |
| 3 | 2026-09-27T17:58:46 | 2026-09-27T19:21:57 | 0 | 0 | OPCODE triage. Fixes: OPCODECREATE clears mDeserializeFrom; RayCollider's +0x88 float widens the culling arm's barycentric bounds (all eight stab variants) and the constructor clears it; host allocator by class (OPC_NOVODEX_ALLOCATEABLE) with empty node constructors and mIndices at its sites; CONTAINER_STATS off; /Qfast_transcendentals on NxOpcode; NovodeX SweepAndPrune (3-argument Init, ctor/dtor, allocator); NovodeX LSSCollider (radius, precomputed segment, inflated-box SAT). Map: 20 rows corrected (CoplanarTriTri, OBB::IsInside/ComputePlanes/ComputePoints/ComputeWorldEdgeNormal, Matrix3x3 cast, SAP Init/PairData::Init/dtor, ~BaseModel, Refit2, Triangle::Normal, the three LSS distance functions, GetSplittingValue loop, the ICE culling walk). Matcher: register vtable calls, trivial-ctor iterators, floats stored as immediates, CRT operators, seeded data, -1.0 negation, conversion-operator names; 82 tests. OPCODE MATCH 17->18 / SHAPE 49->66 / REVIEW 127->176 / DIFF 72->13 / MISSING 1->0 / AMBIGUOUS 1->0 rows; opcode_review.csv covers all 231 groups (227 equivalent, 4 novodex-variant-unwritten). Census: 7 rows third_party=opcode and vendored_not_falsified, 0x000f1350 no longer third party. Unmapped 143 rows classified; the callback tree-collider variant (25,613 B) and serialization (1,277 B) are separate units. Gates 2, 3, 4, 6, 7 pass; Phase 5 red only on `CANDIDATE-MISSING family=vtables`. |
| 3 review | 2026-09-27T19:25:00 | 2026-09-27T20:01:01 | 0 | 0 | Review fixes: SphereTriOverlap reciprocal form and SqrDist register lifetime (build-parity overlay OPC_SphereTriOverlap.h); the e153286 `-1.0` matcher rule withdrawn with its test (it hid that difference; the group is DIFF on the constant alone, bit-identical by exact sign identities); RayTriOverlap V compared and summed unrounded at all 14 sites; CMake comment, README EOL rule, closure prose, reporting Save/Load stubs; opcode_review.csv notes build-stamped. OPCODE MATCH 18 / SHAPE 65 / REVIEW 176 / DIFF 14 rows. Gates 2, 3, 4, 6, 7 pass; Phase 5 red only on `CANDIDATE-MISSING family=vtables`. |
| 4 | 2026-09-27T21:03:00 | 2026-09-27T22:21:48 | 0 | 0 | Execution coverage. `tools/vendored_trace.py` (14 tests): exe-versus-DLL body identity (every traced body the same code but one COMDAT without x87), counting cdb breakpoints per family, coverage CSV and trace excerpts; `/MAP` on both Phase 4 harnesses. `NxPhysicsThirdPartyTests` + 29 families (OPCODE builds, all colliders over oracle-built models, vanilla tree, SAP, ICE maths, qhull through the NovodeX call sequence over 10 option sets); 20 exact, 9 divergent and registered to the oracle digest only; Phase 4 floor 101 -> 130. Groups executed 40 -> 373 of 562; x87 groups in a compared family 9 -> 184 of 259. Found and fixed: stock `AABBCollider::_Collide(const AABBTreeNode*)` argument swap (overlay; candidate `e9e1ba15...` -> `f3a603a2...`). Found, not fixed: NovodeX `VolumeCache` holds `Container*` at +0 (open item). Divergences attributed: splatter-tie and quantized builds, RayTriOverlap/TriTri/ICE sums, boundary inputs, qhull doubles, QR1. No ledger change; gates 2, 3, 4, 6, 7 pass, Phase 5 red only on `CANDIDATE-MISSING family=vtables`. |
| 5a | 2026-09-27T22:37:38 | 2026-09-27T23:24:09 | 0 | 0 | Execution evidence made enforceable. Divergent families fail above a recorded words/discrete ceiling (`kDivergentCeilings`, set to today's measurement; stderr names the first differing word); tape words carry their kind. `vendored_trace.py` (22 tests) classes families exact / lastbit (<= 4 ulp, no discrete word) / discrete and groups by their best execution, with `family_best` alongside: x87 groups qhull 3 exact / 0 lastbit / 70 discrete / 70 not executed, OPCODE 89 / 0 / 22 / 5; no family is lastbit. Harness drives split into per-family passes for attribution; a `release` mark. Fixed: NovodeX `VolumeCache` holds `Container*` at +0 (new overlays OPC_VolumeCollider.h, OPC_Sphere/OBB/PlanesCollider.cpp; AABB/LSS overlays; candidate `f3a603a2...` -> `b0e275ae...`). New families: candidate-built trees queried by candidate colliders (`opcode_candidate_trees` exact; `_ray` divergent by the collider's last bit; `_x87` divergent on quantized/tied trees); Phase 4 floor 130 -> 135. Serialization confirmed unexecuted (candidate stubs). No ledger change; gates 2, 3, 4, 6, 7 pass, Phase 5 red only on `CANDIDATE-MISSING family=vtables`. |
| 5b | 2026-09-27T23:30:00 | 2026-09-28T00:08:17 | 359 | 233469 | Promotion. Prerequisites: kDivergentCeilings also caps worst float/double ulp, beyond, inf_words, degenerate, finite_ulp and beyond_abs (the absolute bound for values near zero); degenerate ICE inputs (29 zero-area triangles, 17 singular matrices) marked by construction and kept out of the distance figures; QR1 runs in their own trace segment (16 QR1-only qhull groups separated); a candidate-only build fails; matcher traces `lea r,[r]`/`lea r,[s+0]` as a copy (PlanesCollider::InitQuery artifact gone, no class change; 82 tests); boundary-count wording. Promoted discovered -> reconstructed: qhull 202 rows / 63,940 B (exact 3, outcome-exact 169, static-only 30), OPCODE 157 / 169,529 B (126 / 21 / 10); each with implementation/source = the defining upstream or overlay file, implementation_symbol, static_proof (matcher class, compared and not-compared features, summation order and register lifetimes not reproduced with sum_grouping sites, review verdict, execution class, attributed discrete families) and dynamic_proof where traced. Ledger: vendored_not_falsified 608 -> 249, reconstructed_not_falsified 111 -> 470; validate_inventory exits 0. Left: qhull DIFF 96, unexecuted x87 65, unexecuted with notes 59, QR1-only 10, option-gated 8; OPCODE DIFF 10, unexecuted with notes 1. work_units.json and the two vendored gap bundles regenerated. Fresh configure and clean build; gates 2, 3, 4, 6, 7 pass, Phase 5 red only on `CANDIDATE-MISSING family=vtables`. |
| 5b review | 2026-09-28T00:20:00 | 2026-09-28T00:26:11 | -8 | -1590 | Final whole-plan review fixes: the eight ICE plane/triangle and matrix rows held back and returned to discovered / vendored_not_falsified (no discrete words, so rule (ii)(a) is vacuous; float divergence 2,820 and 106 ulp outside the degenerate cases is not last-bit), with their former proofs kept in notes; degenerate-input differences added to the summation-order unit's scope; qh_basevertices review line corrected (no other-immediates note); every promoted proof lists the divergent float families the group also ran in; the dynamic proofs and Verification say the pinned binaries predate the final clean link (timestamp only); OPC_RayTriOverlap.h's V lifetime recorded as build parity in MODIFICATIONS.md. Final: 351 rows / 231,879 B promoted (qhull 202 / 63,940, OPCODE 149 / 167,939); ledger vendored_not_falsified 257, reconstructed_not_falsified 462. Validator, 753 tool tests and verify_vendored_sources pass. |
