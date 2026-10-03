# Phase 4 Task 1: the asset formats, the component map, and the RED gate

Phase 4 owns **946 function rows, 425,189 executable bytes — 40.3% of the image —
and 2,813 data objects**, more than half the data census. Everything the
programme has closed so far is 43,145 bytes. Nothing in this task closes a row.
What it does is three things: divide those 946 rows into components that Tasks 2
and 3 can be planned against, recover the two byte formats Phase 4 loads assets
from with an address for every field, and stand up a harness that drives those
formats against the shipped DLL and fails.

| repository | commit | contents |
| --- | --- | --- |
| `D:\github\Novodex` (`main`) | `d1d5d8d` | `tests/PhysicsAssetTests.cpp` and its CMake target |
| this repository | this commit | this file, `cases/assets/`, `tools/tests/test_asset_cases.py`, the `NxPhysicsAssetTests` registration, and two fixes to `run_phase_gate.ps1` |

## The component map

Every Phase 4 function row is assigned to exactly one of seven components, and
the assignment is by **address region**, not by call graph. That choice is
deliberate: the README's standing warning is that a closure over direct
`call rel32` edges undercounts a subsystem that dispatches through function
pointers and vtables, and Phase 4 is the worst case for it — 57 of the 178
entries in the direct-call closure from `Scene::simulate` contain an indirect
call. Address regions come from the linker's own layout, which the census
already established is evidence: the 57 `NX_ASSERT` `__FILE__` spans neither
overlap nor interleave.

The regions are delimited by the translation-unit spans the census recovered.
Nine of the 57 spans are Phase 4's:

| translation unit | span |
| --- | --- |
| `IceAdjacencies.cpp` | `0x0002dbc0` – `0x0002dcf0` |
| `PenetrationMap.cpp` | `0x00050640` |
| `EdgeList.cpp` | `0x000512b0` – `0x00051640` |
| `InternalTriangleMesh.cpp` | `0x00052280` |
| `NpTriangleMesh.cpp` | `0x00052a80` – `0x00052c60` |
| `TriangleMesh.cpp` | `0x00053910` – `0x00055890` |
| `opcode\IcePrunable.cpp` | `0x000b55e0` – `0x000b5610` |
| `opcode\OPC_MeshInterface.cpp` | `0x000e9020` |
| `opcode\OPC_Model.cpp` | `0x000e9100` |

Two of the regions between them carry no `__FILE__` string at all and are the
bulk of the phase. Both are identified from their own contents rather than from
their neighbours:

- `0x0005c5c0` – `0x00084a44`, 464 Phase 4 rows and 154,600 bytes, every one
  placed by the `callers` rule. It is **qhull**: `phys_fn_003210` at `0x0007aef0`
  alone references *"Convex hull of %d points in %d-d:"*, *"Delaunay
  triangulation by the convex hull"* and *"Halfspace intersection by the convex
  hull"*, and `phys_fn_003408` at `0x00084510` references the qhull bug-report
  banner.
- `0x000b5640` – `0x000e9020`, 242 rows and 209,751 bytes, every one placed by
  `enclosed_by_one_phase` between two `opcode\` translation units. It is
  **OPCODE**: `phys_fn_004903` at `0x000b5770` references
  *"SkipPrimitiveTests not possible for RayCollider ! (not implemented)"*,
  *"Temporal coherence only works with First
  contact mode!"* and *"Closest hit doesn't work with First contact mode!"*.

### The single entry into the qhull span

`phys_fn_002233` at `0x00054920`, and nothing else. Disassembling every Phase 4
row and taking its direct `call rel32` targets gives **exactly one** row outside
`0x0005c5c0`–`0x00084a50` that calls into it: `phys_fn_002233`, which calls
`phys_fn_003279` at `0x0007ea10` (from `0x00054975`) and `phys_fn_003255` at
`0x0007e300` (from `0x00054a26`). Its own only caller is `phys_fn_002260` at
`0x00055890`, which is a `mesh` row and slot 2 of the `TriangleMesh` vtable. No
data word anywhere in the image holds `0x10054920`, so no vtable reaches
`phys_fn_002233` either.

**So the row that reaches qhull is in `mesh`, not in the convex component.**
An earlier version of this file named `TriangleMesh::computeConvexHull`,
`phys_fn_002164` at `0x00053b70`, as the qhull caller. That is wrong twice over:
`phys_fn_002164` calls four rows — `0x0000dc00`, `0x00053910`, `0x000b4000` and
`0x0002d940` — none of them in the qhull span, and its whole transitive
direct-call reach is six rows, of which **zero** are qhull. What it does own is
the error report: *"TriangleMesh::computeConvexHull: convex hull init failed!"*
against `TriangleMesh.cpp` line `0x23f`, pushed at `0x00053c3a`. Reporting the
failure of a hull build is not calling the library that builds it.

| component | rows | bytes | % of phase |
| --- | ---: | ---: | ---: |
| `spatial_tree` (OPCODE) | 250 | 210,384 | 49.48% |
| `qhull` | 464 | 154,600 | 36.36% |
| `mesh` | 156 | 41,083 | 9.66% |
| `pmap` | 34 | 13,698 | 3.22% |
| `convex_wrappers` | 12 | 2,466 | 0.58% |
| `serialization` | 25 | 2,288 | 0.54% |
| `ownership` | 5 | 670 | 0.16% |
| **total** | **946** | **425,189** | **100.00%** |

The address ranges, so the table can be recomputed rather than believed:

| component | ranges |
| --- | --- |
| `qhull` | `0x0005c5c0`–`0x00084a50` |
| `convex_wrappers` | `0x0002c8f0`–`0x0002dbc0` |
| `mesh` | `0x0002dbc0`–`0x00036100`, `0x00051080`–`0x00052a80`, `0x00052d30`–`0x00055eb0` |
| `pmap` | `0x0004cae0`–`0x00051080` |
| `ownership` | `0x00052a80`–`0x00052d30` |
| `serialization` | `0x000b3a30`–`0x000b54a0` |
| `spatial_tree` | `0x000b55e0`–`0x000e9280` |

### Why `convex` is two components and not one

The first version of this file had one `convex` component of 476 rows and
157,066 bytes over both ranges, and called all of it third-party. The two ranges
are not the same thing:

- `0x0005c5c0`–`0x00084a50` is 464 rows and 154,600 bytes and is qhull, by its
  own strings.
- `0x0002c8f0`–`0x0002dbc0` is **12 rows and 2,466 bytes and is not qhull**:
  `phys_fn_001496`, `phys_fn_001498`, `phys_fn_001500`, `phys_fn_001514`,
  `phys_fn_001516`, `phys_fn_001518`, `phys_fn_001520`, `phys_fn_001522`,
  `phys_fn_001524`, `phys_fn_001526`, `phys_fn_001528` and `phys_fn_001537`.
  Not one of them references a string of any kind, let alone a qhull one, and
  the transitive direct-call reach of each contains **zero** qhull rows. Their
  callers from outside the block are `phys_fn_002158` (`0x00053910`),
  `phys_fn_002164` (`0x00053b70`), `phys_fn_002217` (`0x00054830`),
  `phys_fn_002219` (`0x00054840`) and `phys_fn_002225` (`0x000548a0`) — all
  inside the `TriangleMesh.cpp` span — plus `phys_fn_001569` (`0x0002e610`,
  a `mesh` row) and `phys_fn_001546` (`0x0002def0`, a Phase 2 row).

They are Novodex's own hull wrappers, and the census places eight of them by the
`callers` rule and `phys_fn_001537` by `layout_adjacency`.

### 28 rows of qhull are not Phase 4's, and two other phases will be asked to reconstruct them

The qhull span `0x0005c5c0`–`0x00084a50` holds 994 `.text` rows. 464 of them
carry `phase: 4` and are the component above. **502 are `phase: 8` padding, and
the remaining 28 carry `phase: 2` or `phase: 5` — 5,768 bytes of qhull on two
other phases' ledgers.**

| row | rva | bytes | phase | provenance | what says it is qhull |
| --- | --- | ---: | ---: | --- | --- |
| `phys_fn_002446` | `0x0005d670` | 770 | 2 | `shared_by_callers` | *"qh_normalize: norm=%2.2g too small during p%d"* at `0x0005d95d` |
| `phys_fn_002452` | `0x0005de50` | 348 | 2 | `shared_by_callers` | *"qh_sethyperplane_gauss: nearly singular…"* at `0x0005ded9` |
| `phys_fn_002458` | `0x0005e470` | 28 | 5 | `layout_adjacency` | its one caller is in the span |
| `phys_fn_002475` | `0x0005ed60` | 39 | 5 | `layout_adjacency` | address region only |
| `phys_fn_002497` | `0x0005f600` | 473 | 2 | `shared_by_callers` | *"qhull precision error (qh_maxsimplex for voronoi_center)"* at `0x0005f77e` |
| `phys_fn_002499` | `0x0005f7e0` | 516 | 2 | `shared_by_callers` | *"qh_maxsimplex: searching all points…"* at `0x0005f897` |
| `phys_fn_002507` | `0x0005fbb0` | 146 | 2 | `shared_by_callers` | all 4 callers in the span |
| `phys_fn_002546` | `0x000612a0` | 100 | 2 | `shared_by_callers` | all 3 callers in the span |
| `phys_fn_002684` | `0x00066270` | 74 | 5 | `layout_adjacency` | address region only |
| `phys_fn_002686` | `0x000662c0` | 32 | 5 | `layout_adjacency` | address region only |
| `phys_fn_002687` | `0x000662e0` | 39 | 5 | `layout_adjacency` | address region only |
| `phys_fn_002689` | `0x00066310` | 317 | 5 | `layout_adjacency` | both callees in the span |
| `phys_fn_002691` | `0x00066450` | 389 | 5 | `layout_adjacency` | *"qh_detvnorm: too few points (%d)…"* |
| `phys_fn_002693` | `0x000665e0` | 90 | 5 | `layout_adjacency` | *"qh_detvnorm: Voronoi vertex or midpoint"* |
| `phys_fn_002695` | `0x00066640` | 1,233 | 5 | `layout_adjacency` | *"qh_detvnorm: points %d %d midpoint dist…"* |
| `phys_fn_002753` | `0x00068290` | 223 | 5 | `layout_adjacency` | both callees in the span |
| `phys_fn_002755` | `0x00068370` | 156 | 5 | `layout_adjacency` | both callees in the span |
| `phys_fn_002868` | `0x0006da40` | 13 | 5 | `layout_adjacency` | address region only |
| `phys_fn_002894` | `0x0006e620` | 33 | 5 | `layout_adjacency` | address region only |
| `phys_fn_002896` | `0x0006e650` | 23 | 5 | `layout_adjacency` | address region only |
| `phys_fn_003057` | `0x00074710` | 104 | 2 | `shared_by_callers` | all 57 callers in the span |
| `phys_fn_003296` | `0x0007f020` | 43 | 2 | `shared_by_callers` | all 11 callers in the span |
| `phys_fn_003308` | `0x0007f2a0` | 98 | 2 | `shared_by_callers` | *"qhull internal error (qh_setsize)…"* |
| `phys_fn_003324` | `0x0007f670` | 121 | 2 | `shared_by_callers` | *"qhull internal error (qh_setaddnth)…"* |
| `phys_fn_003334` | `0x0007fa60` | 70 | 2 | `shared_by_callers` | all 57 callers in the span |
| `phys_fn_003340` | `0x0007fba0` | 145 | 2 | `shared_by_callers` | *"qh_settemp: temp set %p of %d elements…"* |
| `phys_fn_003346` | `0x0007fd20` | 128 | 2 | `shared_by_callers` | *"qhull internal error (qh_settempfree)…"* |
| `phys_fn_003413` | `0x00084800` | 17 | 2 | `shared_by_callers` | all 76 callers in the span |

Phase 2 carries 14 rows and 3,079 bytes of it; Phase 5 carries 14 rows and 2,689
bytes. The `shared_by_callers` ones are qhull's own leaf allocators and set
primitives, shared widely enough inside the library that the census's earliest
rule pulled them to the lowest phase that reaches them. Seven of the
`layout_adjacency` ones have neither caller nor callee in the census at all and
rest on the address region alone.

**Why this is recorded here and not fixed here.** As the census stands, Phase 2
is closed carrying 14 rows of a third-party convex-hull library it never named,
and Phase 5's plan will be handed 14 more. Whatever the programme decides about
vendoring qhull, it has to decide it for 492 rows and 160,368 bytes and not for
464 and 154,600. Moving the `phase` column is a census change and needs its own
gate, so this task does not touch it; it is escalated to Phase 2 and Phase 5 in
the same way `simulation_control_word_scope` was escalated, and named in
"What Task 2 inherits" below.

**Data objects.** The data column is left as the census recorded it for the two
ranges together — 2,660 objects and 56,923 bytes — because the attribution rule
it was produced under reads the Ghidra reference table and this task did not
re-run it. What can be said from the disassembly alone is the direction: a scan
of every immediate in the twelve wrapper rows finds them referencing **one**
Phase 4 data object of 16 bytes, against 2,223 for the qhull rows. Whatever the
exact split, the data is qhull's.

Data objects in `.text` fall in the ranges directly. The 2,411 in `.rdata` and
`.data` are attributed to the component whose code references them, read out of
the Ghidra reference table; three objects totalling 56 bytes are referenced from
two components and are counted once, in the first by address. No Phase 4 data
object has no Phase 4 reader.

| component | data objects | data bytes |
| --- | ---: | ---: |
| `spatial_tree` | 68 | 1,566 |
| `qhull` + `convex_wrappers` | 2,660 | 56,923 |
| `mesh` | 36 | 1,736 |
| `pmap` | 43 | 551 |
| `serialization` | 2 | 20 |
| `ownership` | 1 | 50 |
| **total** | **2,813** | **60,902** |

**What this map is worth, and what it is not.** It is a statement about where
the linker put things, and the census has already shown that is strong evidence
about which translation unit a row belongs to. It is **not** a claim about which
component *uses* a row, and it is not a cost model: 464 rows of qhull and 250
rows of OPCODE are third-party libraries this project links, not code anyone
wrote for Novodex. That is **714 rows and 364,984 bytes, 85.84% of the phase's
bytes**. Everything else — `mesh`, `pmap`, `convex_wrappers`, `serialization`
and `ownership` — is **232 rows and 60,205 bytes**, 14.16% of the phase, and
that alone is larger than everything closed in the programme so far.

The 2,466 bytes of `convex_wrappers` are the whole reason this split matters:
under the old table they sat inside the number the plan is being asked whether
to vendor, and vendoring 364,984 bytes of third-party library on the strength of
a figure that included them would have vendored twelve rows of Novodex.

## The mesh column, and which component reaches it

`gates/phase3-closure.json` defers 19 rows `blocked_on_later_phase`. Thirteen of
them carry `blocked_on_type: TriangleMesh` with `driving_phases: [4]` — the mesh
column of the recovered shape-pair dispatch matrix — and `phys_fn_001751` defers
on three Phase 4 rows named individually rather than on a type. All sixteen are
Phase 3 rows; what follows is which Phase 4 component each one reaches by a
direct call, which is what says who has to exist before it can be driven.

| deferred Phase 3 row | rva | matrix slot | reaches |
| --- | --- | --- | --- |
| `phys_fn_001757` | `0x0003bcd0` | B `[BOX][MESH]` | `spatial_tree` |
| `phys_fn_001772` | `0x0003d500` | A `[BOX][MESH]` | `spatial_tree` |
| `phys_fn_001777` | `0x0003e370` | B `[CAPSULE][MESH]` | `spatial_tree` |
| `phys_fn_001779` | `0x0003e530` | A `[CAPSULE][MESH]` | `spatial_tree` |
| `phys_fn_001781` | `0x0003ebb0` | continuation of A `[CAPSULE][MESH]` | none directly |
| `phys_fn_001783` | `0x0003ef20` | continuation of A `[CAPSULE][MESH]` | `mesh` |
| `phys_fn_001870` | `0x00046550` | B `[MESH][MESH]` | `spatial_tree` |
| `phys_fn_001876` | `0x00046ab0` | A `[MESH][MESH]` | `spatial_tree` |
| `phys_fn_001893` | `0x00048680` | B `[PLANE][MESH]` | `spatial_tree` |
| `phys_fn_001895` | `0x00048760` | A `[PLANE][MESH]` | `spatial_tree` |
| `phys_fn_001897` | `0x000488f0` | continuation of A `[PLANE][MESH]` | none directly |
| `phys_fn_001925` | `0x0004a820` | B `[SPHERE][MESH]` | `spatial_tree` |
| `phys_fn_001929` | `0x0004b1f0` | A `[SPHERE][MESH]` | `spatial_tree` |
| `phys_fn_001670` | `0x00032840` | *(callee of `phys_fn_001751`)* | in `mesh` |
| `phys_fn_001684` | `0x00033a50` | *(callee of `phys_fn_001751`)* | in `mesh` |
| `phys_fn_001688` | `0x00033d00` | *(callee of `phys_fn_001751`)* | in `mesh` |

**Ten** of the thirteen reach `spatial_tree` directly and nothing else in
Phase 4 — `phys_fn_001757`, `phys_fn_001772`, `phys_fn_001777`,
`phys_fn_001779`, `phys_fn_001870`, `phys_fn_001876`, `phys_fn_001893`,
`phys_fn_001895`, `phys_fn_001925` and `phys_fn_001929` — which is what says
**the mesh column is blocked on OPCODE, not on the triangle mesh object**. There
are **three** exceptions, not two: `phys_fn_001781` and `phys_fn_001897` are
continuation rows, which are not call targets and are reached by falling into
them, and `phys_fn_001783` reaches `mesh` through `phys_fn_001694` at
`0x00034860`, a 6,266-byte row. The table above says ten; the sentence under it
said eleven, and the sentence was wrong.

**Which count comes from which source, because two are defensible and they
differ.** Ten is the figure from disassembling each row over its own censused
extent and taking its direct `call rel32` targets, which is how the table above
was built and is why `phys_fn_001783` appears in it with an edge of its own.
Under `oracle/dependencies.dot` the figure is **nine**: that graph folds a
continuation's edges into the row it continues, so `phys_fn_001779` carries
`phys_fn_001694` and `phys_fn_000875` there and `phys_fn_001783` carries nothing
— which puts `001779` in both `spatial_tree` and `mesh` and takes it out of the
"spatial_tree and nothing else" set. (`phys_fn_001895` and `phys_fn_001897` are
folded the same way, but `001897` reaches no Phase 4 component either way, so
that pair does not move the count.) Nine and ten are the same fact counted under
two edge conventions. Neither is eleven, and the conclusion — `spatial_tree`
first — does not depend on which is used.

The three `phys_fn_001751` callees are all inside `mesh`, and `phys_fn_001688`
at `0x00033d00` is called by **three** matrix entries, not one:
`phys_fn_001751` (B `[BOX][CAPSULE]`), `phys_fn_001753` (A `[BOX][CAPSULE]`) and
`phys_fn_001785` (B `[CAPSULE][COMPOUND]`). Reconstructing it discharges part of
three deferrals rather than one.

So the dependency order Task 2 should read out of this is `spatial_tree` first
for ten rows, `mesh`'s three primitive helpers for `phys_fn_001751`, and `mesh`
proper for the `TriangleMesh` object those ten traverse.

## The penetration-map format

Two rows write and read it, and they agree field for field. The writer is
`phys_fn_002017` at `0x0004e1a0`; the reader is `phys_fn_002047` at
`0x00050640` for the header and `phys_fn_002035` at `0x00050110` for the
payload. Every row below names the instruction that establishes it.

### Header

| field | width | value | established by |
| --- | --- | --- | --- |
| tag byte 0 | 1 | `0x50` `'P'` | `push 0x50` `0x0004e23a`, stored `0x0004e23e`; read+compare `0x0005069a` (`cmp al, 0x50`) |
| tag byte 1 | 1 | `0x4d` `'M'` | `push 0x4d` `0x0004e238`, stored `0x0004e245`; compare `0x000506a8` |
| tag byte 2 | 1 | `0x41` `'A'` | `push 0x41` `0x0004e236`, stored `0x0004e24c`; compare `0x000506b3` |
| tag byte 3 | 1 | `0x50` `'P'` | `push 0x50` `0x0004e234`, stored `0x0004e253`; compare `0x000506be` |
| version | 4 | must be exactly `4` | `push 4` `0x0004e232`, stored `0x0004e25a`; compare `0x000506c9` (`cmp eax, 4`) |
| resolution | 4 | grid side length | stored `0x0004e261`; read `0x000506f7`, kept at `PenetrationMap+0x5c` by `0x0004ff80` |

The four tag bytes are read one at a time through `phys_fn_004772` at
`0x000b3aa0` — a byte read, not a dword — so the tag has no byte order to get
wrong. The version and the resolution are read through `phys_fn_004774` at
`0x000b3ac0`, whose whole body is `mov eax,[eax]` at `0x000b3ad5`: **a native
little-endian load with no `bswap` anywhere in the class.** The format is
little-endian by omission, not by choice, and it has no endianness marker.

Rejection is by `NxUserOutputStream::reportError` and a `false` return, at two
sites and no others. Both are named here with the address of their own
`push <line>`, because an earlier version of this file paired each site with the
other's line number:

| site | line | message pointer | message | reached from |
| --- | --- | --- | --- | --- |
| `push 0x3da` at `0x000506d7` | `986` | `0x10107fb8` | *"PenetrationMap::Create: the pmap file is invalid (bad version number)"* | `cmp eax, 4` at `0x000506c9` failing |
| `push 0x3d3` at `0x0005070c` | `979` | `0x10107f78` | *"PenetrationMap::Create: the pmap file is invalid (bad header)"* | any of the four `jne 0x00050703` at `0x0005069c`, `0x000506aa`, `0x000506b5`, `0x000506c0` |

Both push the file string `0x1010802c` (`\Epic\Novodex\SDKs\Physics\src\PenetrationMap.cpp`)
and both push error code `4` (`push 4` at `0x000506e6` and `0x0005071b`). The
version site's `call dword ptr [edx]` is at `0x000506e8` and the header site's
`call dword ptr [eax]` at `0x0005071d`. Which line the oracle reports is what
tells the two branches apart, so the fixtures pin it — and the fixtures and
`gate_targets.ps1` had the pairing right all along: `pmap.bad_magic_*` records
`line=0x3d3` and `pmap.bad_version_*` records `line=0x3da`, which is what the
oracle prints. Only the prose was inverted.

### Grid

`phys_fn_002033` at `0x0004ff80` derives everything else from the resolution
`n` and the mesh's AABB:

| field | offset | established by |
| --- | ---: | --- |
| spread table | `+0x04` | 256 dwords, `malloc(0x400)` at `0x0004cb28`, filled `0x0004cb35`–`0x0004cb8f`, freed `0x0004cb07` |
| resolution `n` | `+0x5c` | `0x0004ff99` |
| `n * n` | `+0x60` | `imul` `0x0004ff8d`, store `0x0004ff9c` |
| `(float)(n - 1)` | `+0x64` | `0x0004ffab` |
| `1 / (n - 1)` | `+0x68` | `0x0004ffba` |
| AABB min, max | `+0x08` … `+0x1c` | six copies from `mesh+0x44`, `0x0004ffbf` … `0x0004ffdf` |
| centre | `+0x20` … `+0x28` | `(min + max)` at `0x0004ffe8`, `0x0004fff1`, `0x0004fffa`, halved from `0x00050006` |
| half extents | `+0x2c` … `+0x34` | `(max - min)` at `0x00050027`, `0x00050030`, `0x00050039`, halved from `0x0005003f` |
| extents | `+0x38` … `+0x40` | `(max - min)` unhalved, `0x00050060` … `0x00050082` |
| cells per unit | `+0x44` … `+0x4c` | `(n-1) / extent`, `0x00050085` … `0x0005009d` |
| units per cell | `+0x50` … `+0x58` | `extent / (n-1)`, `0x000500a0` … `0x000500c6` |
| cell count `n³` | `+0x6c` | `imul edi, ecx` `0x000500b6`, store `0x000500ca` |
| grid | `+0x70` | `malloc(n³ * 4)` at `0x000500cd`, `rep stosd 0xffffffff` at `0x000500ea`, store `0x000500ef` |

`sizeof(PenetrationMap)` is **`0x78`**, from `push 0x78` at `0x00053d35` in
`TriangleMesh::loadPMap`; `+0x74` holds the mesh pointer, written at
`0x0005073e`. The constructor `phys_fn_002045` at `0x000505f0` installs the
vtable `0x10107f40` and seeds the AABB with `0x7f7fffff` and `0xff7fffff`
(`FLT_MAX` / `-FLT_MAX`) at `0x000505f9` and `0x00050607`.

**No dimension in the header is validated against anything.** There is no
minimum, no maximum and no length field: a resolution of `0` allocates a
zero-cell grid and a resolution of `0x400` allocates 4 GB, and both are accepted
by the same code path.

### Payload

A bit stream, MSB first inside each byte, read through the same byte reader. The
bit accumulator lives in the stream object at `+0x18` (mask) and `+0x19`
(current byte), and **every raw read resets it**: `mov byte ptr [ecx+0x18], 0`
is the second instruction of `phys_fn_004772` at `0x000b3aa2`, so a bit stream
and a byte stream cannot be interleaved.

The value loop, from `0x000501b0`:

| step | established by |
| --- | --- |
| refill when the mask is zero | `test [esi+0x18]` at `0x000501b2`, `readByte` at `0x000501bb`, `mov byte ptr [esi+0x18], 0x80` at `0x000501c3` |
| one flag bit, MSB first | `test [esi+0x19], al` at `0x000501ca`, `shr al, 1` at `0x000501d0` |
| flag set → value is the previous value plus one | `jne` at `0x000501d7` skipping the read; the increment is `inc edi` at `0x0005020c` |
| flag clear → a 32-bit value, MSB first | the loop at `0x000501e0` – `0x00050208` |
| terminator | `cmp ebp, -1; je` at `0x0005020d` |
| the cells carrying that value | `phys_fn_002008` at `0x0004dba0`, called at `0x00050224` |
| assignment | `mov [ebx + edx*4], ebp` at `0x0005023f` |

After the terminator, one bit per cell, and a **clear** bit sets `0x80000000` in
that cell: the loop runs from `0x00050260`, the bit is tested at `0x00050278`
and the `or ecx, 0x80000000` is at `0x00050290`. Then `phys_fn_002037` at
`0x000502d0` runs a corner pass and a Morton reorder.

> **Corrected by P4 Task 3.** This paragraph used to say phys_fn_002037 "runs a
> 26-neighbour pass that sets `0x40000000` on a cell all of whose neighbours have
> `0x80000000` clear, and re-sorts the grid". Three things in that sentence are
> wrong and `evidence/phase4-pmap-reconstruction.md` §3 carries the disassembly:
> it is the **eight corners** of the unit cube at `(i,j,k)` — `a`, `a+1`, `a+n`,
> `a+n+1`, `a+n²`, `a+n²+1`, `a+n²+n`, `a+n²+n+1`, assembled at `0x00050310`–
> `0x0005034d` — not 26 neighbours; the `or` at `0x00050466` fires when every
> surviving corner has bit 31 **set**, not clear (`shr edx,0x1f; not edx;
> test dl,1; jne` skips on clear); and the "re-sort" is a **Morton reorder**
> through the 256-entry bit-spreading table at `PenetrationMap+0x04`, radix-sorted
> at `0x00050544` and applied at `0x00050599`. `+0x04` is missing from the layout
> table below; it is a second heap pointer, allocated by `phys_fn_001986` at
> `0x0004cb28` and freed at `0x0004cb07`.

`phys_fn_002008` reads a 32-bit element count MSB first
(`0x0004dbd5` – `0x0004dc00`) and then walks a 3D cursor with 5-bit codes
(`0x0004dc30` – `0x0004dc5b`) dispatched through a 32-way jump table at
`0x0004dc75` that moves three globals at `0x101222e8`, `0x101222ec` and
`0x101222f0`. A zero count returns without entering that walk, which the
`pmap.multi_value` fixture measures rather than asserts: it declares two value
records with a zero count each and the oracle accepts it. The first three
instructions of the row are the one place in this format where the resolution is
interpreted: `cmp eax, 0x20 → 5`, `cmp eax, 0x40 → 6`, `cmp eax, 0x50 → 7` at
`0x0004dbab`, `0x0004dbb8` and `0x0004dbc4`, **defaulting to 0 for every other
resolution**. So the cell-run encoding is defined for resolutions 32, 64 and 80
and for nothing else, while the header accepts any resolution at all.

### `NxCreatePMap` and `NxReleasePMap`

`NxCreatePMap` is `phys_fn_002049` at `0x00050f70`. It reads the core mesh
pointer out of the public object at `+0x04` (`mov esi, [eax+4]` at
`0x00050f7b`), builds a growable memory stream with a `0x1000` initial block
(`0x00050f92`), calls `PenetrationMap::Create` with `load = 0` at `0x00050fb5`,
then takes the stream's byte count from `phys_fn_004768` at `0x000b3a30`
(called at `0x00050fc4`), stores it into `NxPMap::dataSize` at `0x00050fd1`,
**`malloc`s** that many bytes at `0x00050fd3` and stores the copy into
`NxPMap::data` at `0x00050fe3`. `NxReleasePMap`, `phys_fn_002051` at
`0x00051040`, tests `data` at `0x00051048`, `free`s it at `0x0005104d`, nulls it
at `0x00051055`, and returns `1` unconditionally from `mov al, 1` at
`0x0005105c`.

Three consequences a reimplementation gets wrong by default, all measured:

- The PMap buffer is CRT heap, not the Foundation allocator. `0x000f48c0` and
  `0x000f48bb` are `malloc` and `free`; the Foundation allocator at
  `0x101041bc` is not consulted.
- `NxReleasePMap` **returns true for a PMap it did not release**, and leaves
  `dataSize` untouched. The harness drives exactly this and records
  `returned=1 data_size=5a5a5a5a data=00000000`.
- `NxCreatePMap` never validates `density`. It goes straight into the
  resolution field.

## The triangle-mesh stream format

`phys_fn_002262` at `0x00055cb0` reads a `TriangleMesh` from an `NxStream`. The
stream is the public interface from the pinned `NxStream.h`, and the slot
offsets are established by use, not by counting: `+0x0c` for every integer,
`+0x10` for every float, `+0x18` for every block, which is `readDword`,
`readFloat` and `readBuffer` in declaration order behind the virtual destructor.

| # | field | width | destination | established by |
| ---: | --- | ---: | --- | --- |
| 1 | tag 1 | 4 | — | `readDword` at `0x00055cbf`, `cmp eax, 0x4e585354` at `0x00055cc2` |
| 2 | tag 2 | 4 | — | `readDword` at `0x00055ccd`, `cmp eax, 0x4d455348` at `0x00055cd0` |
| 3 | *(unnamed)* | 4 | discarded | `readDword` at `0x00055ce7`, result never stored |
| 4 | flags | 4 | `ebx`, bits 0–3 used | `readDword` at `0x00055cee` |
| 5 | convex-edge threshold | 4 f32 | `+0x6c` | `readFloat` at `0x00055cf7`, `fstp [edi+0x6c]` at `0x00055cfa` |
| 6 | height-field vertical axis | 4 | `+0x7c` | `readDword` at `0x00055d01`, store `0x00055d04` |
| 7 | height-field vertical extent | 4 f32 | `+0x80` | `readFloat` at `0x00055d0b`, `fstp` `0x00055d0e` |
| 8 | vertex count | 4 | `internal+0x00` | `readDword` at `0x00055d1b` → `phys_fn_002069` at `0x00051fa0` |
| 9 | triangle count | 4 | `internal+0x04` | `readDword` at `0x00055d2e` → `phys_fn_002071` at `0x00051fd0` |
| 10 | vertices | `vertexCount * 12` | `internal+0x08` | `readBuffer` at `0x00055d50`, size `[ebp]*3<<2` at `0x00055d43` |
| 11 | triangles | `triangleCount * 12` | `internal+0x0c` | `readBuffer` at `0x00055d66`, size `[edi+0xc]*3<<2` at `0x00055d5c` |
| 12 | material indices *(flags bit 0)* | `triangleCount * 2` | `internal+0x10` | `test bl, 1` at `0x00055d69`, allocation `phys_fn_002073` at `0x00052000`, `readBuffer` at `0x00055d80` |
| 13 | face remap *(flags bit 1)* | `triangleCount * 4` | `internal+0x14` | `test bl, 2` at `0x00055d83`, allocation `phys_fn_002075` at `0x00052030`, `readBuffer` at `0x00055d9b` |
| 14 | presence flag A | 4 | `+0x8c` | `readDword` at `0x00055da2`, store `0x00055da5` |
| 15 | presence flag B | 4 | `+0x90` | `readDword` at `0x00055daf`, store `0x00055db2` |
| 16 | array A *(if flag A ≠ 0)* | `triangleCount * 4` | `+0x94` | `test` `0x00055dbe`, allocation `0x00055dd5`, `readBuffer` `0x00055dea` |
| 17 | array B *(if flag B ≠ 0)* | `triangleCount * 4` | `+0x98` | `test` `0x00055df5`, allocation `0x00055e0a`, `readBuffer` `0x00055e1f` |
| 18 | acceleration blob length | 4 | — | `readDword` at `0x00055e26` |
| 19 | acceleration blob | that many bytes | a memory stream | `readBuffer` at `0x00055e57` into a buffer built at `0x00055e34` |

Then `phys_fn_002083` at `0x00052280` builds the OPCODE model from the blob and
the two height-field fields (`0x00055e6c`); if flags bit 2 is set,
`phys_fn_002164` at `0x00053b70` builds or reads the convex hull with flags bit
3 selecting which (`shr ebx, 3; and ebx, 0xffffff01` at `0x00055e7f`); and
`phys_fn_002255` at `0x000555b0` finishes the object.

### The writer, which IS in this image

An earlier version of this file said *"the writer is not in this image — there
is no store path for this format anywhere in `NxPhysics.dll`"*, and three of the
items it recorded as unestablished rested on that. It is false.

**`phys_fn_002162` at `0x000539d0`**, 413 bytes, translation unit
`TriangleMesh.cpp` (`0x00053910`–`0x00055890`), is the writer. It is **slot 18
of the same nineteen-slot `TriangleMesh` vtable this task enumerated for slot 7**
— `phys_data_000967` at `0x00108608`, and `0x00108650` holds `0x100539d0`. Slot
17 immediately before it is `0x10055cb0`, the reader. They are the `save`/`load`
pair and they sit next to each other.

It writes through the same `NxStream` the reader reads through, one slot further
down the same vtable: `storeDword` at `+0x24` against `readDword` at `+0x0c`,
`storeFloat` at `+0x28` against `readFloat` at `+0x10`, `storeBuffer` at `+0x30`
against `readBuffer` at `+0x18`. Field for field, in the reader's order:

| # | field | writer source | established by |
| ---: | --- | --- | --- |
| 1 | tag 1 | `0x4e585354` | `push` `0x000539de`, `storeDword` `0x000539e5` |
| 2 | tag 2 | `0x4d455348` | `push` `0x000539ea`, `storeDword` `0x000539f1` |
| 3 | *(unnamed)* | the global at `0x10124120` | `mov ecx, [0x10124120]` `0x000539f4`, `push` `0x000539fc`, `storeDword` `0x000539ff` |
| 4 | flags | assembled from four object fields | `0x00053a02` – `0x00053a2d`, `storeDword` `0x00053a35` |
| 5 | convex-edge threshold | `[edi+0x6c]` | `push` `0x00053a3d`, `storeFloat` `0x00053a40` |
| 6 | height-field axis | `[edi+0x7c]` | `push` `0x00053a48`, `storeDword` `0x00053a4b` |
| 7 | height-field extent | `[edi+0x80]` | `push` `0x00053a56`, `storeFloat` `0x00053a59` |
| 8 | vertex count | `[edi+0x08]` | `push` `0x00053a61`, `storeDword` `0x00053a64` |
| 9 | triangle count | `[edi+0x0c]` | `push` `0x00053a6c`, `storeDword` `0x00053a6f` |
| 10 | vertices | `[edi+0x10]`, `[edi+0x08]*3<<2` | size `0x00053a7a`, `storeBuffer` `0x00053a84` |
| 11 | triangles | `[edi+0x14]`, `[edi+0x0c]*3<<2` | size `0x00053a8f`, `storeBuffer` `0x00053a99` |
| 12 | material indices | `[edi+0x18]` if non-null, `[edi+0x0c]<<1` | `test` `0x00053a9f`, size `0x00053aa8`, `storeBuffer` `0x00053aae` |
| 13 | face remap | `[edi+0x1c]` if non-null, `[edi+0x0c]<<2` | `test` `0x00053ab4`, size `0x00053abd`, `storeBuffer` `0x00053ac4` |
| 14 | presence flag A | `[edi+0x8c]` | `push` `0x00053acf`, `storeDword` `0x00053ad2` |
| 15 | presence flag B | `[edi+0x90]` | `push` `0x00053ade`, `storeDword` `0x00053ae0` |
| 16 | array A | `[edi+0x94]` if non-null, `[edi+0x0c]<<2` | `test` `0x00053ae9`, `storeBuffer` `0x00053af9` |
| 17 | array B | `[edi+0x98]` if non-null, `[edi+0x0c]<<2` | `test` `0x00053b02`, `storeBuffer` `0x00053b12` |
| 18 | blob length | the growable stream's byte count | stream built `0x00053b20` with `push 0x1000` at `0x00053b17`, model save `0x00053b2f`, byte count `0x00053b36`, `storeDword` `0x00053b42` |
| 19 | blob | that many bytes | data pointer `0x00053b4e`, `storeBuffer` `0x00053b56` |

Fields 8 to 13 are read out of `TriangleMesh+0x08` … `+0x1c` and the reader
stores them into `InternalTriangleMesh+0x00` … `+0x14`, because the internal
mesh is **embedded at `TriangleMesh+0x08`**: `lea ebp, [edi + 8]` at
`0x00055d18`, and every `phys_fn_0020xx` setter the reader calls is called with
that `ebp`. The offsets are the same six fields under two names.

The writer returns `1` unconditionally (`mov al, 1` at `0x00053b64`, `ret 4` at
`0x00053b6a`) and there is no error path in it.

#### What the writer settles, and what it does not

**Field 3 is written from a global, and the global is never written.** The
source is `phys_data_003609` at `0x00124120`, a four-byte Phase 4 `.data`
object. Two measurements, both from the pinned image:

- the four-byte little-endian constant `0x10124120` occurs **exactly once in the
  whole file**, at raw offset `0x539f6` — the operand of the `mov ecx` at
  `0x000539f4`. Nothing else in `NxPhysics.dll` reads or writes it;
- `0x00124120` is past the end of `.data`'s raw bytes (`.data` is `0x00122000`
  with `SizeOfRawData` `0x2000`, so the file supplies `0x00122000`–`0x00124000`
  and the rest is zero-filled at load). It is `0` when the writer runs.

So the field this image writes there is `0`, the field this image reads there is
discarded (`readDword` at `0x00055ce7`, result never stored), and **what it
means is still unestablished**. What the writer removes is the possibility that
it is computed from the mesh: it is a program-lifetime global, not a property of
the object being written.

**The flags word has four bits and no more — measured, not assumed.** The writer
assembles it and sets nothing above bit 3:

| bit | writer condition | established by |
| ---: | --- | --- |
| 0 | `[edi+0x18]` non-null (material indices present) | `test ecx, ecx` `0x00053a07`, `mov eax, 1` `0x00053a0b` |
| 1 | `[edi+0x1c]` non-null (face remap present) | `test ecx, ecx` `0x00053a13`, `or eax, 2` `0x00053a17` |
| 2 | `[edi+0xa0]` non-null | `test ecx, ecx` `0x00053a22`, `or eax, 4` `0x00053a24` |
| 3 | `test byte ptr [edi+0x40], 1` | `0x00053a27`, `or eax, 8` `0x00053a2d` |

`eax` is zeroed at `0x00053a05` and the four instructions above are the only
ones that touch it before the store at `0x00053a35`. The reader tests bits 0, 1,
2 and 3 and no others. **The other 28 bits are zero on anything this image
writes and unread by anything this image reads** — which is a measurement, and
replaces the earlier "whether they are unused or are consumed by the cooker is
unestablished". A cooker could still set them; nothing in this image would
notice, and nothing in this image would produce them.

**Bits 2 and 3 now have meanings, from the writer's conditions read against
`phys_fn_002164`'s body.** `+0xa0` is the convex hull pointer: `phys_fn_002164`
releases whatever is there through its slot 0 (`0x00053b84`), nulls it
(`0x00053b86`), and on the arm that builds one stores the result back into
`+0xa0` at `0x00053bc6`. `[edi+0x40]` bit 0 is which of its two arms ran: it
clears the bit at `0x00053b97`/`0x00053b9c` and sets it with
`or dword ptr [esi+0x40], 1` at `0x00053bd2` on the arm reached when the
argument is non-zero. So:

- **bit 2 means "a hull is present"** — the writer sets it exactly when `+0xa0`
  is non-null, and the reader calls `phys_fn_002164` exactly when it is set
  (`test bl, 4` at `0x00055e7a`);
- **bit 3 is the argument `phys_fn_002164` is given**, which selects which of
  its two constructions to use — the reader extracts it with
  `shr ebx, 3; and ebx, 0xffffff01` at `0x00055e7f`/`0x00055e82`, and the writer
  reproduces it from the bit that construction left behind at `[edi+0x40]`.

The two round-trip: bit 3 out is the bit `phys_fn_002164` set going in.

### The two tags

`0x4e585354` and `0x4d455348` are read as **dwords**, so on the little-endian
target the bytes on disc are `54 53 58 4e` and `48 53 45 4d`. Written
most-significant byte first those constants spell `NXST` and `MESH`; written in
file order they spell `TSXN` and `HSEM`. (`0x4d455348` is `48 53 45 4d` least
significant byte first: `0x48`, `0x53`, `0x45`, `0x4d`. An earlier version of
this file transposed the middle two, giving `48 45 53 4d` and `HESM`. The
fixtures had it right — `mesh.bad_tag0` is `5353584e4853454d`, whose second
dword is `4853454d`.)

**Which of those two spellings the source used is not established**, and the
writer does not settle it either: `phys_fn_002162` at `0x000539d0` pushes the
same two 32-bit constants and hands them to the stream's `storeDword`
(`push 0x4e585354` at `0x000539de`, `push 0x4d455348` at `0x000539ea`), so it
says the reader and the writer agree and nothing about which byte order the
programmer had in mind when typing the constant. What *is* established is the
32-bit value each comparison requires and each store emits, and the fixture
`mesh.bad_tag0_byteswapped` drives the other order and records that the oracle
rejects it.

### Field identifications, and where they come from

`+0x6c`, `+0x7c` and `+0x80` are not guesses from a header. The constructor at
`0x000554d4` writes `0x3a83126f` — exactly `0.001f` — into `+0x6c`, which is
`NxTriangleMeshDesc::setToDefault`'s `convexEdgeThreshold = 0.001f`;
`0x000540e9` and `0x000540f0` write `0xff` and `0` into `+0x7c` and `+0x80`,
which is `heightFieldVerticalAxis = NX_NOT_HEIGHTFIELD` and
`heightFieldVerticalExtent = 0`. `NX_NOT_HEIGHTFIELD` is `0xff` in the pinned
`NxTriangleMesh.h`, and `0xff` is the value `phys_fn_002083` tests at
`0x000522fa` before passing the pair on.

The `InternalTriangleMesh` layout, from the allocation sites:

| offset | field | established by |
| ---: | --- | --- |
| `+0x00` | vertex count | store `0x00051fa7` |
| `+0x04` | triangle count | store `0x00051fd7` |
| `+0x08` | vertices, 12 bytes each | size `0x00051fb3`, allocate `0x00051fbc`, store `0x00051fbf` |
| `+0x0c` | triangles, 12 bytes each | size `0x00051fe4`, allocate `0x00051fed`, store `0x00051ff0` |
| `+0x10` | material indices, 2 bytes each | size `add eax, eax` `0x00052018`, allocate `0x0005201b`, store `0x0005201e` |
| `+0x14` | face remap, 4 bytes each | size `shl eax, 2` `0x00052048`, allocate `0x0005204c`, store `0x0005204f` |
| `+0x18` | vertex normals, 12 bytes each | size `0x0005224e`, allocate `0x00052257`, store `0x00052269`, filled by `NxBuildSmoothNormals` |
| `+0x1c` | owned per-triangle records, 16 bytes each; record contents opaque | allocation `shl eax, 4` `0x000521db`, store `0x000521e2`; each record built by `phys_fn_005155`; released by `phys_fn_002067` |
| `+0x20` | the OPCODE model | released and cleared `0x00052296`, installed `0x00052360` |
| `+0x24` | the embedded `MeshInterface` | `lea` `0x000522a9`, `SetPointers` `0x000522b1` into `0x000e9020` |

**Triangle indices are 32 bit and there is no 16-bit variant in this format.**
Both the file field and the in-memory array are `triangleCount * 12`, at
`0x00055d5c` and `0x00051fe4`, and no flag selects anything narrower.

## What this task could not establish

Named, rather than filled in from what a mesh format usually looks like. Three
of these moved when `phys_fn_002162` was found; each says below what the writer
settled and what it left.

- **What field 3 of the mesh header MEANS.** Read at `0x00055ce7` and discarded;
  written at `0x000539ff` from `phys_data_003609` at `0x00124120`, a global
  no instruction in this image writes and which is zero at load. Its position is
  where a version would sit. It is still unestablished, not "the version" — but
  it is now unestablished as *a program-lifetime global whose value this image
  always writes as zero*, not as a field of unknown provenance.
- **What the mesh flag bits above 3 are FOR.** Bits 0, 1, 2 and 3 are tested by
  the reader at `0x00055d69`, `0x00055d83`, `0x00055e7a` and `0x00055e7f`, and
  the writer sets those four and no others (`xor eax, eax` at `0x00053a05`, then
  only `mov eax, 1`, `or eax, 2`, `or eax, 4`, `or eax, 8`). So the earlier
  wording — "whether the remaining 28 bits are unused or are consumed by the
  cooker is unestablished" — is replaced by a measurement: **nothing in this
  image writes a bit above 3 and nothing in this image reads one**. A cooker
  outside this image could still set one; that is what remains unestablished,
  and it is a smaller claim than the one it replaces.
- **The MEANING of `+0x8c`/`+0x90` and their arrays at `+0x94`/`+0x98`.** The
  format is established from both directions now — two `NxU32` presence flags
  followed by `triangleCount * 4` bytes each, read at `0x00055da2`/`0x00055daf`
  and written at `0x00053ad2`/`0x00053ae0` — and what the numbers *mean* is
  still not. The writer's own predicate is only "the pointer is not null", which
  says the arrays are optional and nothing about what is in them.
- **The `OPCODECREATE` structure `phys_fn_002083` fills.** The immediates `1`
  and `0x22` are written at `0x000522ff` and `0x00052307` and two booleans come
  from float comparisons at `0x000522e3` and `0x0005232e`, but the member
  offsets could not be tracked through the two stack adjustments in that frame
  and are left unestablished rather than approximated.
- **The internal layout of the acceleration blob.** Field 19 is a length and a
  byte count; what is inside it belongs to `spatial_tree` and to Task 2.
- **The 5-bit cell-walk codes in `phys_fn_002008`.** The 32-way jump table at
  `0x0004dc75` moves three global cursors. The fixtures deliberately declare a
  zero element count so the switch is never entered, and the encoding is
  recorded as unestablished.
- **Whether the mesh stream format has any length or bounds validation.** It has
  none that this task found: the stream reader `phys_fn_004772` bounds-checks
  nothing, so a truncated payload does not fail, it reads past the buffer. The
  pmap fixtures are padded with zeroes for exactly this reason, and the two
  truncation cases that *are* driven are truncations inside the header, which is
  the only region where truncation is detected — and then only because the bytes
  that follow happen to fail the tag or version test.
- **The accept arm of the mesh reader.** It allocates through the Foundation SDK
  allocator at `0x101041bc`, which is null until an `NxPhysicsSDK` exists.
  Driving it needs the mesh factory Task 2 reconstructs.
- **`NxCreatePMap`'s compute path.** It needs a real triangle mesh. The plan
  gives that to Task 3, and this task drives the load path instead, which reads
  only the six floats at `mesh+0x44`.

## The control word

Every probe in this task ran under the CRT default **`0x027f`**, which is the
word each probed row executes under in the shipped SDK: `NxCreatePMap`,
`NxReleasePMap` and `NxTriangleMesh::loadPMap` are consumer entry points, and
none of `phys_fn_002047`, `phys_fn_002035`, `phys_fn_002037`, `phys_fn_002033`,
`phys_fn_002008`, `phys_fn_002262` or the stream primitives is in the direct
call closure from `Scene::simulate` at `0x00013c40`. That closure is a lower
bound — 57 of its 178 entries contain an indirect call — so this is stated as
what the walk found and not as a proof of absence.

### A Phase 3 closed row reached under the other word, and Phase 4 owns the row that reaches it

`NxBuildSmoothNormals` is `phys_fn_002146` at `0x000533c0`. Phase 3 Task 2
closed it under `0x027f`; Phase 3 Task 3 found it inside the simulation step and
pinned a divergence of 24 words in 459,676 under `0x0f7f`.

It has **exactly one caller in the whole image**: `phys_fn_002081` at
`0x00052240`, a 59-byte **Phase 4** row that allocates `vertexCount * 12` bytes
at `0x00052257`, stores the result into `InternalTriangleMesh+0x18` at
`0x00052269` and then calls the export at `0x00052271`.
`phys_fn_002081` has eight callers:

- seven Phase 3 rows — `0x00028370`, `0x00029230`, `0x0003c240`, `0x000427d0`,
  `0x00045d70`, `0x000470f0`, `0x0004a940` — every one of them narrow-phase or
  mesh-contact code the recovered dispatch matrix puts inside the step, so
  `0x0f7f`;
- one Phase 4 row, `phys_fn_002205` at `0x00054720`, which no direct call edge
  in the census reaches, because it is **slot 7 of the `TriangleMesh` vtable**.

That vtable is `phys_data_000967` at `0x00108608`, a 19-slot dispatch table, and
it is the value `TriangleMesh::release` writes back at `0x00055822`. Slot 7 is
`getBase(NxSubmeshIndex, NxInternalArray)`: `ret 8`, a five-way jump table at
`0x00054734` over `NX_ARRAY_TRIANGLES … NX_ARRAY_HULL_POLYGONS`, and the
`NX_ARRAY_NORMALS` arm at `0x00054742` calls `phys_fn_002081` when the normals
pointer is still null.

So a public `NxTriangleMesh::getBase(_, NX_ARRAY_NORMALS)` call — plain consumer
code, `0x027f` — builds the smooth normals through the same 59-byte Phase 4
wrapper that seven step-internal rows use under `0x0f7f`. This is the
`simulation_control_word_scope` escalation arriving in the direction the brief
did not predict: recovering a Phase 4 vtable did not move a Phase 3 row *into*
the step, it showed that Phase 4's own public API reaches a step-internal row
from *outside* it. **`phys_fn_002081` is a Phase 4 row that runs under both
words**, and Task 2 has to close it under both.

Two things follow for Task 2. `Physics/src/SmoothNormals.cpp` already carries
the `/arch:IA32` rule for this reason and the reason is now two paths rather
than one. And whatever translation unit `phys_fn_002081` lands in needs the same
rule, because it is on the step side of that pair; the CMake comment that names
four files and warns that anything else added under `Physics/src` builds SSE2 by
default applies to it directly.

## The gate

`NxPhysicsAssetTests` is an oracle differential of the same kind as
`NxPhysicsCollisionTests`: it loads the pinned `NxPhysics.dll` into its own
process, re-hashes it, refuses to run unless the hash is the pin, and calls the
recorded internal addresses. It drives 21 probes — 14 pmap fixtures, 6 mesh
fixtures and one `NxReleasePMap` case — and its fixtures are hex byte strings
and nothing else, so no oracle process pointer can reach one. The cases and the
transcript are in `cases/assets/`, and `tools/tests/test_asset_cases.py` holds
the three artifacts to each other.

The harness has two sides and they fail independently:

- the **oracle** side is checked against the answers recorded in
  `cases/assets/formats.json`, which is a check that can fail with no
  reconstruction in the picture at all;
- the **candidate** side is checked against the oracle side, and today the three
  `nxCandidate*` functions report that the row is not reconstructed.

`--self` runs the first alone, which is what "GREEN against the oracle" means
here — not a comparison of the oracle with itself.

### RED

```
gate=oracle_differential:NxPhysicsAssetTests command="D:\github\Novodex\build\Release\NxPhysicsAssetTests.exe" ...
oracle pin=matched
asset fixtures pmap=14 mesh=6 release=1
pmap case=pmap.minimal_valid dimension=minimal_valid bytes=17 accepted=1 errors=0 line=0x000 resolution=1 cells=1 grid=e3160fb1
pmap CANDIDATE-MISSING case=pmap.minimal_valid: no reconstruction of phys_fn_002047
...
asset coverage driven=21 accepted=4 rejected=16 errors=10
asset oracle digest=b114fa73 expect_mismatches=0
asset candidate mismatches=21 mode=differential
FAIL the reconstruction does not agree with the pinned oracle
gate=oracle_differential:NxPhysicsAssetTests exit=1
GATE FAILED: oracle_differential:NxPhysicsAssetTests exited 1
```

`expect_mismatches=0` beside `candidate mismatches=21` is the distinction that
matters: the oracle was called 21 times and answered exactly what the recorded
cases say, and the reconstruction answered nothing. An **absent** target does
not look like this. Phase 5, which registers nothing, prints

```
phase_gate=5 status=skipped reason=no_registered_test_targets
```

and exits 3, and a target that failed to build would stop at
`built test target exists:` before any oracle line was printed.

### GREEN

```
asset coverage driven=21 accepted=4 rejected=16 errors=10
asset oracle digest=b114fa73 expect_mismatches=0
asset candidate mismatches=0 mode=self
asset result=pass
```

Same digest, same 21 probes, exit 0.

### Proof that it can fail, by the strong form

Both mutations were built and run in a `git archive` copy of `d1d5d8d`, with
every source touched after extraction, and the un-mutated control run in that
same directory before and after the set. Both controls printed
`asset oracle digest=b114fa73 expect_mismatches=0 … result=pass` and were
byte-identical to each other.

| variant | mutation | mode | digest | result |
| --- | --- | --- | --- | --- |
| control (before) | none | `--self` | `b114fa73` | exit 0, `expect_mismatches=0` |
| A | one byte of `pmap.bad_version_00000003`: version `3` → `4` | `--self` | **`245a212e`** | **exit 1**, `expect_mismatches=1` |
| B | `nxCandidatePMapLoad` answers, and answers wrongly | differential | `b114fa73` | **exit 1**, 14 `CANDIDATE-MISMATCH` lines |
| control (after) | none | `--self` | `b114fa73` | exit 0, `expect_mismatches=0` |

A is the one that matters. It changes **only the fixture** — not the recorded
expectation, not the harness logic, not the reconstruction — and the oracle's own
answer moves from `accepted=0 errors=1 line=0x3da` to `accepted=1`, the digest
moves, and the run fails. That is what says the fixture bytes are load-bearing
and the version check is really being driven.

B is the complement: with a candidate that *answers*, the comparison produces
`CANDIDATE-MISMATCH` with both sides printed rather than `CANDIDATE-MISSING`,
the oracle digest does not move, and `expect_mismatches` stays 0 — the two sides
are independent, which is the property Task 2 will rely on.

### The gate defects this task found and fixed

The first two are in `run_phase_gate.ps1`; the third is the other half of the
first and was introduced by fixing it; the fourth is in `gate_targets.ps1` and
predates this task. All four are the shape this programme keeps finding: a check
that could not fire, or a phase that could not report the result it was for.

1. **A phase with only an oracle differential reported itself ungated.** The
   "nothing registered cannot be gated" guard read `$targets`, the staged-pair
   list, alone — the same blind spot that once let an emptied oracle-differential
   list evaluate 0 of 37 coverage assertions under `status=pass`. Phase 4
   registers an oracle differential and no staged-pair differential, so
   `-Phase 4` would have printed `status=skipped` and exited 3 while a
   registered, failing target sat unbuilt. The guard now counts all three lists.
   Phases 5–8 still register nothing and are still skipped.
2. **A failing oracle differential threw before the runner could read its exit
   code.** `2>&1` turns each stderr line into an `ErrorRecord`, and
   `$ErrorActionPreference` is `Stop`, so the pipeline threw *while the process
   was still running*: the transcript ended in a `NativeCommandError`, the
   coverage assertions never ran, and `$LASTEXITCODE` still held the `0` assigned
   before the call rather than the exit code of a process that had not exited.
   The gate happened to fail anyway, through the throw — but the mechanism it
   was relying on had stopped working. `Stop` is now suspended for the
   invocation, and the run above prints
   `GATE FAILED: oracle_differential:NxPhysicsAssetTests exited 1`.

3. **And then the phase the first fix admitted still could not pass.**
   `run_differential.ps1 -Phase 4` resolves `$NxPhaseTestTargets['4']` to an
   empty list, prints `differential=skipped`, and exits `$NxSkippedExitCode`;
   `run_phase_gate.ps1` threw on any non-zero differential exit. So the guard
   admitted Phase 4 to the gate and the throw at the end failed it for having
   exactly the shape the guard had been widened to admit — a phase whose only
   registration is an oracle differential could **never** report `pass`. It is
   invisible today only because the oracle differential is RED and throws
   earlier; it would have surfaced the moment Task 2 turned that green, which is
   the worst possible time. The skip code is now not a failure when this runner
   resolved no staged-pair target either, and only then. Fixing half of a guard
   and exercising only its fail path is what produced this.
4. **The target registry was derived from the phase maps it checks.**
   `gate_targets.ps1` built `$NxRegisteredTestTargets`,
   `$NxRegisteredStaticProofTargets` and `$NxRegisteredOracleDifferentialTargets`
   by flattening `$NxPhaseTestTargets`, `$NxPhaseStaticProofTargets` and
   `$NxPhaseOracleDifferentialTargets`. Four assertions read those lists — three
   in `run_phase_gate.ps1` and one in `run_differential.ps1` — and every one of
   them compared a value against a set built from that value. Registering
   `'4' = @('NxNotInTheRegistry')` printed
   `pass: selected oracle-differential target is registered` and
   `pass: test target is registered in gate_targets.ps1: NxNotInTheRegistry`.
   The three registries are now written out, so registering a target is two
   edits and a typo in either fails; `test_gate_targets.py` asserts they are not
   derived, that the two agree in both directions, and drives
   `run_differential.ps1` over a copied registry with an unregistered name to
   watch the check fire.

The second one was only visible because an oracle differential finally failed,
which is the state Phase 4 is in on purpose for the next two tasks.

## What Task 2 inherits

- The dependency order out of the component map: `spatial_tree` unblocks ten of
  the thirteen deferred mesh-column rows (nine under `dependencies.dot`'s edge
  convention; the section above says which is which); `mesh`'s three primitive
  helpers unblock `phys_fn_001751`, `phys_fn_001753` and `phys_fn_001785`
  between them.
- `phys_fn_002081` at `0x00052240` runs under both control words and needs the
  `/arch:IA32` rule in whatever translation unit it lands in.
- The mesh reader's accept arm and `NxCreatePMap`'s compute path are the two
  format paths this task could not drive, and both are named above with the
  reason.
- Seven fields and one sub-encoding are recorded as **unestablished**. None of
  them may be filled in from what a format usually looks like.
- **`phys_fn_002162` at `0x000539d0` is the mesh-stream writer** and is slot 18
  of the `TriangleMesh` vtable, next to the reader at slot 17. Task 2 gets both
  sides of the format and can round-trip a reconstruction against the oracle
  instead of only parsing.
- **The candidate side of `NxPhysicsAssetTests` is handed fixture bytes and a
  length, and nothing else.** The three `nxCandidate*` functions must keep that
  signature when Task 2 fills them in: a candidate that can see
  `NxPMapFixture`/`NxMeshFixture` can see the recorded oracle answers and can be
  made green without parsing anything. `test_asset_cases.py` asserts it
  structurally.

Escalations out of this task that are not Task 2's:

- **28 rows and 5,768 bytes of qhull sit on Phase 2's and Phase 5's ledgers**,
  listed row by row above. Phase 2 is closed carrying 14 of them. Any decision
  about vendoring qhull covers 492 rows and 160,368 bytes, not 464 and 154,600.
  Re-phasing them is a census change and needs its own gate.
