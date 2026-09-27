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
| OPCODE | SHAPE | 66 | 12,418 | 64 |
| OPCODE | REVIEW | 176 | 193,271 | 136 |
| OPCODE | DIFF | 13 | 26,569 | 13 |
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
| `e153286` | Matcher: `-1.0` against a negated `1.0` | 18 | 62 | 163 | 31 | 0 | 0 | 274 |
| `660dcec` | Matcher: constant-store window of 12 | 18 | 62 | 168 | 26 | 0 | 0 | 274 |
| `19a9844` | LSSCollider: NovodeX members and inflated-box test; LSS distance rows renamed; census `third_party` for the newly mapped rows | 18 | 66 | 176 | 13 | 0 | 0 | 273 |

The final figures are MATCH 18 rows (233 bytes, 18 groups), SHAPE 66 (12,418, 64), REVIEW 176
(193,271, 136) and DIFF 13 (26,569, 13). They come from candidate sha256
`ec405bc23037bc51c95c313752cb05732ee707d59364aa9956eacf00ef30f8ef`, the build of `19a9844`.
The qhull classes did not change. Every qhull match line is unchanged except a candidate string
address inside one `report_jcc` value, which moved because the candidate DLL's layout moved.

**Real behavioural divergences found and fixed.**

1. **`OPCODECREATE::mDeserializeFrom` was uninitialised.** `Model::Build` branches on it. A stack
   `OPCODECREATE` would take the load path with a garbage pointer.
2. **RayCollider's added member is a float tolerance on the barycentric bounds of the culling
   arm.** It is not a hit-distance tolerance:
   - `U < -t`, `U > det + t`, `V < -t` and `U + V > det + t` reject (`0x000b873b`..`0x000b87f1`),
     where stock tests the sign bit and `det` exactly;
   - the distance's sign test stays stock;
   - the non-culling arm is stock;
   - the same code is inlined in all eight `_RayStab`/`_SegmentStab` variants.

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

**Matcher changes** (`vendored_match.py`, 82 tests):

- A register call through a just-loaded vtable slot is the memory call it stands for.
- A one-sided `vector constructor iterator` is unwrapped: a trivial constructor drops out, and a
  real one becomes a call that the inlining check can absorb.
- A float constant that only feeds stores matches an integer store of the same bits to the same
  field.
- `-1.0` against a run-time-negated `1.0` is shape.
- The CRT operator thunks are named, and so is `CompleteBoxPruning`.
- `SEEDED_DATA` holds three hand-identified data objects.
- The source-name parser keeps a conversion operator's two words.
- `ORACLE_KNOWN` applies to unmapped rows.

**Review.** `opcode_review.csv` has one line per group (231):

| Class | Groups | Verdicts |
|---|---:|---|
| REVIEW | 136 | all equivalent |
| SHAPE | 64 | all equivalent |
| MATCH | 18 | all equivalent |
| DIFF | 13 | 9 equivalent, 4 novodex-variant-unwritten |

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
- **OPCODE, after Task 3 (separate work units; see the Task 3 section).**
  - The NovodeX callback instantiation of the no-leaf tree-versus-tree collider:
    - rows `0x000d12b0`, `0x000cd700`, `0x000ca5a0`, `0x000cbe50` (25,613 bytes);
    - to be written with its caller `phys_fn_001876`.
  - Serialization, 17 rows and 1,277 bytes:
    - BaseModel slots 4 to 6, and the four trees' getSerialSize, save, load and relocation rows;
    - they need MemoryStream host seams and a direct-oracle differential;
    - their stubs are reachable from the reconstructed `TriangleMesh.cpp` save path and from
      `Model::Build`'s load dispatch.
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
