# Phase 4 Task 3: the penetration-map load path, the mesh header tags, and what the fixtures cannot see

This is the evidence behind `NxPhysicsAssetTests` going from `candidate mismatches=21` to
`candidate mismatches=0` **without the oracle digest moving**: `b114fa73` before and `b114fa73`
after, `driven=21 accepted=4 rejected=16 errors=10` before and after, `expect_mismatches=0` in every
run recorded here.

| repository | commit | contents |
| --- | --- | --- |
| `D:\github\Novodex` (`main`) | `bcf544f` | `Physics/src/PMap.cpp`, `MemoryStream.cpp`, `TriangleMesh.cpp` and their private headers; the three `nxCandidate*` bodies; the CMake target |
| this repository | `a2ca0b6b`, `f3b0a5d6` | this file, the census changes, `evidence/phase4-artifact-retype.csv`, four new tests |

Oracle re-verified before and after: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`,
1,253,376 bytes. All probing in a `git archive` copy of `bcf544f` under the scratchpad; the production
tree was never mutated; no `git stash`.

---

## 1. The population, recounted

`inventory.json`, `phase == 4`, no `third_party` key, RVA outside all three library spans
(`evidence/phase4-third-party.md` §3.1): **229 rows, 60,146 bytes.**

| component | rows | bytes | Task 1 said | delta |
| --- | ---: | ---: | --- | --- |
| `mesh` | 153 | 41,010 | 156 / 41,083 | **−3 rows, −73 bytes** |
| `pmap` | 34 | 13,698 | 34 / 13,698 | — |
| `serialization` | 25 | 2,302 | 25 / 2,288 | **+14 bytes** |
| `convex_wrappers` | 12 | 2,466 | 12 / 2,466 | — |
| `ownership` | 5 | 670 | 5 / 670 | — |
| **total** | **229** | **60,146** | 232 / 60,205 | **−3 rows, −59 bytes** |

Two disagreements with Task 1's grouping, and both are Task 1's arithmetic rather than a boundary
dispute — the address ranges are unchanged and every row lands in exactly one of the five:

- **The three `mesh` rows are `phys_fn_002150` (`0x00053880`, 24), `phys_fn_002152` (`0x000538a0`,
  14) and `phys_fn_002154` (`0x000538b0`, 35).** They are inside `mesh`'s address range and Task 1
  counted them, but Task 1c later gave all three `third_party: opcode` — they are
  `OPC_MeshInterface.cpp` sitting below the `TriangleMesh.cpp` span. They are vendored now, so they
  leave this population.
- **`serialization` is 14 bytes heavier than Task 1 said with the same 25 rows.** Nothing moved; the
  earlier byte total was short.

---

## 2. What the four accepted fixtures actually measure — and it is less than it looks

**Every accepted fixture leaves a grid of nothing but `0xffffffff`.** All three recorded grid
digests are FNV-1a over N copies of `0xffffffff` and nothing else:

| fixture | cells | recorded digest | FNV-1a over N × `0xffffffff` |
| --- | ---: | --- | --- |
| `pmap.minimal_valid` | 1 | `e3160fb1` | `e3160fb1` |
| `pmap.multi_value` | 8 | `5b517625` | `5b517625` |
| `pmap.boundary_res4` | 64 | `06a34ac5` | `06a34ac5` |
| `pmap.boundary_res1_signclear` | 1 | `e3160fb1` | `e3160fb1` |

That is a property of the fixtures, not of the format, and it follows from three facts about them:
both value records in `pmap.multi_value` declare a cell-run count of **zero**, so no value is ever
written into a cell; the sign block only ever ORs `0x80000000` into a cell that is already all ones;
and the corner pass only ever ORs `0x40000000` into one. A permutation of a uniform array is that
array.

**So the grid digest measures the cell count and the fill value and nothing else.** Four rows'
worth of arithmetic — the spread table, the corner pass, the sign block and the Morton reorder — is
invisible to this differential, and §4 shows all four being broken with the gate staying green.
Task 1's fixtures are genuine and their oracle side is checked; what they do not do is exercise the
grid's contents.

---

## 3. What the reconstruction says, corrected against Task 1 where it differs

`Physics/src/PMap.cpp`, `MemoryStream.cpp` and `TriangleMesh.cpp` carry the address for every
statement. Three things `evidence/phase4-formats.md` states differently:

### 3.1 The corner pass tests for bit 31 SET, not clear

`phase4-formats.md` says phys_fn_002037 "sets `0x40000000` on a cell all of whose neighbours have
`0x80000000` **clear**". It is the other way round. Each of the eight tests is

```
0x0005039e  mov  ecx,[ebx+0x70]        ; the grid
0x000503a1  mov  edx,[ecx+eax*4]
0x000503a4  shr  edx,0x1f
0x000503a7  not  edx
0x000503a9  test dl,1
0x000503ac  jne  0x0005046e            ; -> next cell, no OR
```

`shr`/`not` maps bit 31 **clear** to `0xffffffff` (`dl & 1` set, jump taken, skip) and bit 31
**set** to `0xfffffffe` (fall through). The `or ecx,0x40000000` at `0x00050466` is reached only when
every one of the eight has bit 31 set.

### 3.2 They are the eight corners of a unit cube, not 26 neighbours

`phase4-formats.md` calls it "a 26-neighbour pass". The eight indices are assembled at
`0x00050310`-`0x0005034d` as `a`, `a+1`, `a+n`, `a+n+1`, `a+n²`, `a+n²+1`, `a+n²+n`, `a+n²+n+1`
— the cell itself and the seven cells at `+1` on each of the three axes. `0x0005035a` invalidates
the four `+1` indices when `k == n-1`, `0x00050379` the four `+n` ones when `j == n-1`, and
`0x0005038c` the four `+n²` ones when `i == n-1`.

### 3.3 The "re-sort" is a Morton reorder, and `PenetrationMap+0x04` is what drives it

`phase4-formats.md` says phys_fn_002037 "re-sorts the grid" without saying by what. The second half
of the function (`0x000504a5`-`0x000505d9`) builds one key per cell,

```
0x000504d0  mov  eax,[ebx+4]           ; the 256-entry spread table
0x000504d3  mov  edi,[eax+ebp*4]
0x000504d6  mov  edx,[eax+ecx*4]
0x000504d9  mov  eax,[eax+esi*4]
0x000504dc  lea  edx,[edi+edx*2]
0x000504df  lea  edi,[eax+edx*2]       ; table[k] + 2*table[j] + 4*table[i]
```

appends them to an Ice `Container`, radix-sorts them with `RadixSort::Sort(..., RADIX_SIGNED)` at
`0x00050544`, and permutes the grid by the ranks (`0x00050599`, copied back at `0x000505b6`).
`[ebx+4]` is a 256-dword table `phys_fn_001986` at `0x0004cb20` allocates and fills in the
constructor: entry `c` is the eight bits of `c` spread three apart, bit `i` landing at bit `3i`
(`0x0004cb37`-`0x0004cb80`). The three interleaved indices therefore make a Morton code, and the
grid ends up in Morton order. **`PenetrationMap+0x04` is not in `phase4-formats.md`'s layout table
at all**, and the destructor frees it (`0x0004cb07`), so the object is 0x78 bytes with two heap
pointers, not one.

### 3.4 A field the format table does not name

`PenetrationMap+0x38`..`+0x40` holds `max-min` unhalved (`0x00050060`-`0x00050082`), and `+0x44` and
`+0x50` hold `(n-1)/extent` and `extent/(n-1)` per axis (`0x00050085`-`0x000500c6`).
`phase4-formats.md`'s grid table stops at `+0x34`.

### 3.5 The load path is reached with `esi` carrying the stream

`phys_fn_002008` takes two stack arguments — the `Container` and the resolution — and reads the
stream out of **`esi`**, which its caller `phys_fn_002035` happens to be holding it in
(`0x0004dbd5` reads `[esi+0x18]` with nothing in the row having written `esi`). It is a register
convention the compiler produced, not a parameter; the reconstruction passes the stream explicitly.

---

## 4. Falsification: twenty mutations, sixteen red, four green by design

Every run rebuilds and re-runs `NxPhysicsAssetTests` in the archive copy after restoring all eight
of this task's files from the production tree, so no mutation leaks into the next. **The oracle
digest is `b114fa73` in every single run, and `driven=21 accepted=4 rejected=16 errors=10` and
`expect_mismatches=0` never move** — which is what says the mutations moved the candidate and not
the measurement.

| # | row (censused extent) | mutation | measured |
| --- | --- | --- | --- |
| 0 | — | control | **green**, `candidate mismatches=0` |
| A | `phys_fn_002047` `0x00050640`+2340 | the third tag byte is read but not compared | **red, 1** — `pmap.bad_magic_2 accepted=1/0 line=0x000/0x3d3` |
| B | `phys_fn_002047` | version test `!= 4` becomes `< 4` | **red, 2** — `bad_version_00000005`, `bad_version_ffffffff` |
| C | `phys_fn_002047` | the two reported line numbers swapped | **red, 10** — every malformed and truncated case, `line=0x3da/0x3d3` and `0x3d3/0x3da` |
| D | `phys_fn_002047` | the resolution argument kept instead of the file's | **red, 4** — all four accepted, `cells=0/1`, `0/8`, `0/64`, `0/1` |
| E | `phys_fn_002047` | the version failure is not reported | **red, 5** — `errors=0/1` on the four bad-version cases and `truncated_after_magic` |
| F | `phys_fn_002033` `0x0004ff80`+398 | cell count `n*n` instead of `n*n*n` | **red, 2** — `multi_value cells=4/8`, `boundary_res4 cells=16/64` |
| G | `phys_fn_002033` | its grid fill writes 0 instead of `0xffffffff` | **GREEN — see §5** |
| H | `phys_fn_002035` `0x00050110`+439 | the loader's own refill writes `0xfffffffe` | **red, 4** — all four accepted, `grid=8e5b0aa0/e3160fb1` and the rest |
| I | `phys_fn_002035` | the loader returns false | **red, 4** — all four accepted, `accepted=0/1` |
| J | `phys_fn_002045` `0x000505f0`+69 | the constructor leaves the resolution at 1 | **red, 10** — every rejected case, `resolution=1/0` |
| K | `phys_fn_002051` `0x00051040`+32 | `NxReleasePMap` returns false | **red, 1** — `returned=0/1` |
| L | `phys_fn_002051` | `NxReleasePMap` clears `dataSize` too | **red, 1** — `data_size=00000000/5a5a5a5a` |
| N | `phys_fn_002262` `0x00055cb0`+511 | both tags read before either is tested | **red, 3** — `dwords_read=2/1` on the three `bad_tag0` cases |
| O | `phys_fn_002262` | the second tag constant one nibble out (`0x4d455347`) | **red, 1** — `mesh.bad_tag1` reaches the unreconstructed arm and reports `CANDIDATE-MISSING` |
| P | `phys_fn_004772` `0x000b3aa0`+23 | `readByte` advances by two | **red, 9** — the four accepted, the four bad-version cases and `truncated_after_magic` |
| Q | `phys_fn_004774` `0x000b3ac0`+24 | `readDword` is big-endian | **red, 4** — all four accepted, `line=0x3da/0x000` |
| R | `phys_fn_002041` `0x00050310`+429 | the corner pass never ORs `0x40000000` | **GREEN — see §5** |
| S | `phys_fn_001986` `0x0004cb20`+115 | the spread table spreads two bits apart, not three | **GREEN — see §5** |
| T | `phys_fn_002035` | the sign block never ORs `0x80000000` | **GREEN — see §5** |
| U | `phys_fn_002043` `0x000504c0`+296 | the Morton reorder is skipped | **GREEN — see §5** |
| V | `phys_fn_002008` `0x0004dba0`+909 | the cell-run count is not read off the stream | **red by access violation, exit `0xc0000005`** — not a compared value; see §5 |
| Z | — | control again | **green**, `candidate mismatches=0` |

Sixteen rows' worth of behaviour is falsified by a mutation inside the row's own censused extent
with a measured non-zero delta. **Eight rows close on that basis:**

| row | rva | bytes | mutations |
| --- | --- | ---: | --- |
| `phys_fn_002045` `PenetrationMap::PenetrationMap` | `0x000505f0` | 69 | J |
| `phys_fn_002047` `PenetrationMap::Create` | `0x00050640` | 2,340 | A, B, C, D, E |
| `phys_fn_002033` `PenetrationMap::setup` | `0x0004ff80` | 398 | F |
| `phys_fn_002035` `PenetrationMap::loadPayload` | `0x00050110` | 439 | H, I |
| `phys_fn_002051` `NxReleasePMap` | `0x00051040` | 32 | K, L |
| `phys_fn_002262` `TriangleMesh::load` | `0x00055cb0` | 511 | N, O |
| `phys_fn_004772` `MemoryStream::readByte` | `0x000b3aa0` | 23 | P |
| `phys_fn_004774` `MemoryStream::readDword` | `0x000b3ac0` | 24 | Q |

**3,836 bytes of 60,146 — 3.5% of the rows and 6.4% of the bytes of this task's population.**

### And the registration, broken on purpose

Changing one hex digit of `gate_targets.ps1`'s `'asset oracle digest=b114fa73'` to `b114fa70` makes
`run_phase_gate.ps1 -Phase 4` fail with

```
GATE FAILED: requirement not met: NxPhysicsAssetTests reported its recorded oracle-side coverage
(0 occurrences): asset oracle digest=b114fa70
```

after the differential itself has already exited 0. Restored and re-verified. The registration is
load-bearing.

---

## 5. Reconstructed and NOT falsified, and why each one cannot be

| what | why no delta exists |
| --- | --- |
| **`phys_fn_001986`** `0x0004cb20`, the spread table (mutation S) | its only consumer is the Morton sort, and sorting a uniform array is the identity. Nothing in these fixtures can tell a spread of 3 from a spread of 2 |
| **`phys_fn_002037`/`002039`/`002041`/`002043`** `0x000502d0`-`0x000505e8`, 777 bytes (mutations R and U) | the corner pass ORs `0x40000000` into cells already `0xffffffff`, and the reorder permutes an array of identical words. Both are the identity on every recorded fixture |
| **the sign block inside `phys_fn_002035`** (mutation T) | same: it ORs `0x80000000` into cells already `0xffffffff`. `phys_fn_002035` closes on H and I; **this arm of it does not** |
| **`phys_fn_002033`'s `rep stosd 0xffffffff`** at `0x000500ea` (mutation G) | `phys_fn_002035` refills the whole grid with the same word at `0x00050160` before reading a bit, so setup's fill is dead on the load path. What H proves is that *the loader's* fill is `0xffffffff`; what G shows is that *setup's* is unobservable through the load path. The row closes on F |
| **`phys_fn_002008`** `0x0004dba0`, 909 bytes (mutation V) | the row's outputs — the count it returns and the cell indices it appends — are never compared, because every recorded fixture declares a count of zero. All the differential can see is the row's effect on the stream position, and removing the 32-bit count read desynchronises the bit stream so far that the reader walks off the fixture buffer and the process faults. A red by access violation is not a measured delta and this row is **not** claimed |
| **the bit-mask reset in `phys_fn_004772`/`phys_fn_004774`** (`mov byte ptr [ecx+0x18],0`) | it is the second instruction of each row and it is what stops a bit stream and a byte stream interleaving. No recorded fixture interleaves them: the header is read before the first bit and no raw read follows. Reproduced, unfalsified |
| **`phys_fn_001984`** `0x0004cae0`, the destructor | it frees the grid and the spread table. Nothing in the harness observes a free |
| **the two `__LINE__` values 979 and 986** | the image pushes `0x3d3` and `0x3da`, which are `__LINE__` in NovodeX's own `PenetrationMap.cpp`. `Physics/src/PMap.cpp` is not that file. They are named constants there, recorded rather than manufactured by padding the file to 986 lines. Mutation C falsifies *which branch reports which*, which is the part that is a fact about the oracle |
| **the four adapters `phys_fn_004870`/`004872`** | not this task's reconstruction; see §7 |

---

## 6. What was NOT reconstructed, named rather than filled in

- **The COMPUTE arm of `phys_fn_002047`, `0x00050768`-`0x00050f02`** — roughly 2,000 of that row's
  2,340 bytes. It rasterises the mesh through an OPCODE tree it builds on the spot and needs a real
  `InternalTriangleMesh`. The census row carries this in its `notes`.
- **`NxCreatePMap` (`phys_fn_002049`, `0x00050f70`, 198 bytes)** — its whole body is that arm. It is
  not exported by the reconstruction, because an export that cannot compute is worse than an absent
  one.
- **The filename arm of `phys_fn_002035` (`0x00050123`-`0x0005014f`) and the build-your-own-stream
  arm at `0x0005017b`**, both of which reach serialization rows this task does not cover.
- **The 5-bit cell walk in `phys_fn_002008`** — the 32-way jump table at `0x0004dc75` moving three
  cursor globals. Recorded unestablished by Task 1 and still unestablished.
- **Fields 3 to 19 of the mesh stream header.** The accept arm allocates through the Foundation SDK
  allocator at `0x101041bc`, null until an `NxPhysicsSDK` exists. `nxTriangleMeshReadHeader` returns
  a third value for "the tags passed and the rest is not reconstructed", and the harness reports
  `CANDIDATE-MISSING` for it rather than answering. No recorded fixture reaches it; mutation O shows
  what happens when one does.

---

## 7. The two blockers

### (a) The 63 `compiler_artifact` rows: all 63 are code, and every one is reached

`evidence/phase4-artifact-retype.csv` carries the row, the reference that reaches it and how.
Derivation, over the pinned image and the census:

- **27 rows** are the target of a direct `call`/`jmp rel32` from another censused row. Their callers
  are ordinary code rows in and out of the set — `phys_fn_004830` (`0x000b4cc0`), `phys_fn_004832`,
  `phys_fn_004834`, `phys_fn_004852`, `phys_fn_004855`, `phys_fn_002282`, `phys_fn_002284`,
  `phys_fn_005208`, `phys_fn_005254`, `phys_fn_005256` and others.
- **36 rows** have their virtual address written into the OPCODE vtable/literal pool at
  `.rdata:0x0011b5a4`-`0x0011bcf8`. Every hit is inside that pool; there are no coincidental matches
  elsewhere in the file.
- **0 rows** are unreached.

So the recorded proof — *"no product translation unit reaches these bytes and none is named above
the last one that does, at `0x000e9100`"* — is false for all 63, not for the three Task 2b named.
It is the same false negative the programme has been finding all phase: a reach heuristic built on
direct calls cannot see an indirect vtable dispatch, and 36 of these are reached only that way.

The census now types all 63 `code` and records the reaching reference in each row's `static_proof`.
**That is a `kind` change and nothing else** — no `phase`, no `third_party`, no `state` moves. It
unblocks 8,919 bytes: while a row carried `compiler_artifact`, `validate_inventory.py:464` refused
any reconstruction recorded against it, so those bytes could not be closed by anybody.

**What this does NOT settle.** All 63 sit inside the OPCODE core span and Task 2b called them "tree
serialization and the tree-class instantiations". Most are therefore probably *vendored OPCODE*
rather than NovodeX product code, which would make them `third_party: opcode` with a correspondence
map entry — three of them (`phys_fn_005380`, `005382`, `005386`) are certainly the NovodeX-added
`BaseModel`/`AABBOptimizedTree` virtuals and would be `External/opcode/novodex/`. Attributing each
one is Task 2a's correspondence-map machinery and is a second census change; it is not what was
blocking closure, and this task did not do it.

`RetypedArtifactsStayCode` in `tools/tests/test_gate_targets.py` binds the CSV to the census: the
file must carry 63 rows, each must be censused at the recorded RVA and size, and each must be
`kind: code`. Falsified four ways — reverting one row's kind fails it, emptying the CSV to its
header fails it, and the two asset-row assertions fail on a reverted `state` and on a dropped
`source`. All four red, control green.

### (b) The OPCODE span's low end really is 64 bytes too high

Confirmed on the disassembly. `phys_fn_004870` at `0x000b5460` (25 bytes) and `phys_fn_004872` at
`0x000b5480` (23) are each

```
0x000b5460  mov  eax,[0x10128470]      ; (0x10128474 for the second)
0x000b5465  test eax,eax
0x000b5467  je   <return>
0x000b5469  mov  ecx,[esp+4]           ; the first argument
0x000b546d  mov  edx,[ecx+4]           ; -> Prunable::mOwner
0x000b5470  mov  [esp+4],edx           ; replaces it
0x000b5474  jmp  eax
```

— an adapter that turns a `Prunable*` into its owner and tail-jumps through a global hook.
`Prunable::Prunable` installs their two addresses at `.data:0x001284fc` and `0x00128500`
(`0x000b54d0`, `0x000b54da`). The rows either side of them, `phys_fn_004866` (`0x000b53c0`) and
`phys_fn_004868` (`0x000b5410`), are Phase 7; the rows above them are the `IcePrunable.cpp`
asserters. `0x000b54a0 - 0x000b5460 = 0x40`.

**`evidence/phase4-third-party.md` §3.1 is corrected in place**, with the correction marked. The
effect on the two populations is stated and neither ledger moves: the two rows leave this task's
229 (which becomes 227 rows and 60,098 bytes if the span is redrawn) and join Task 2b's 186 (188 and
37,308). Neither task claims them — Task 2b wrote them in `Physics/src/opcode/IcePrunable.cpp` and
explicitly did not claim them, and nothing drives their bodies. The correction is recorded so that
whoever redraws the span does not have to rediscover it.

---

## 8. The mesh column does not unblock

Task 1 established that ten of the thirteen deferred Phase 3 rows reach `spatial_tree` by a direct
call and nothing else in Phase 4, and read that as "blocked on OPCODE traversal, not on
`TriangleMesh`". Both halves are true about *call edges*. Neither is what blocks a harness.

`gates/phase3-closure.json` records all ten with `blocked_on_type: TriangleMesh` and the note *"It
needs the triangle mesh, its acceleration structure and the vertex and index data behind them"*.
That is the operational statement, and vendoring OPCODE removes one of four prerequisites:

1. **the `InternalTriangleMesh` object** — vertex and triangle arrays, material indices, the face
   remap. Zero of the `mesh` component's 153 rows are reconstructed;
2. **`phys_fn_002083` at `0x00052280`**, which builds the OPCODE model out of field 19 of the stream.
   Task 1 recorded its `OPCODECREATE` fill as unestablished — *"the member offsets could not be
   tracked through the two stack adjustments in that frame"* — and this task did not change that;
3. **the accept arm of the mesh reader**, which allocates through the SDK allocator at
   `0x101041bc` (§6);
4. **the ten Phase 3 rows themselves.** Checked rather than assumed: each of the ten RVAs appears in
   `D:\github\Novodex` exactly once, in `tests/PhysicsCollisionTests.cpp`'s recovered dispatch-matrix
   table, which is an address map. **None of the ten has a candidate implementation anywhere.**

So the answer is no, and nothing here reopens Phase 3. No deferral is discharged, no
`discharged_by_phase` is written, and `closure phase=3 closed=61 deferred=475` is unchanged.

`phys_fn_001751`'s three named callees — `phys_fn_001670` (`0x00032840`), `phys_fn_001684`
(`0x00033a50`) and `phys_fn_001688` (`0x00033d00`), 1,018 bytes — are all in the `mesh` component
and are also untouched.

---

## 9. The control word

**All eight closed rows execute under `0x027f`, the CRT default**, and that is the word the
differential drives them under. Two things say so:

- none of the eight is in the direct-call closure from `phys_fn_000659` (`Scene::simulate`,
  `0x00013c40`), which is what installs `_PC_64 | _RC_CHOP`. The consumer entry points that reach
  them — `NxCreatePMap`, `NxReleasePMap`, `NxTriangleMesh::loadPMap`, the mesh factory — are all
  outside the step;
- that closure is a lower bound, because 57 of its 178 entries contain an indirect call. So this is
  what the walk found, not a proof of absence.

For six of the eight it does not matter either way: `phys_fn_002045`, `phys_fn_002051`,
`phys_fn_002262`, `phys_fn_004772`, `phys_fn_004774` and `phys_fn_002035` execute no
floating-point instruction that reaches a compared value. The two that do are `phys_fn_002033` —
the whole of `+0x20`..`+0x58` is x87, including a division by `(float)(n-1)` which is a division by
zero at `n == 1` — and, through it, `phys_fn_002047`. **None of those float fields is compared by
this differential**, so the control word cannot show there either; that is stated rather than used
as a reason to skip it.

Nothing in this build exercises `0x0f7f` for any Phase 4 row, and `NxPhysicsCollisionTests` is still
the only harness that can drive either word.

---

## 10. `USE_MINMAX`

Not touched, and it did not need to be. The open decision is whether `AABB` stores min/max or
centre/extents in the vendored OPCODE build. `PenetrationMap` stores **both**: min and max at
`+0x08`/`+0x14`, centre and half-extents at `+0x20`/`+0x2c`, all six derived in `phys_fn_002033`
from the six floats at `mesh+0x44`. It is a NovodeX layout in a NovodeX class and it does not use
`Ice::AABB` at all, so nothing here depends on which representation OPCODE compiles with, and
nothing here is evidence either way.
