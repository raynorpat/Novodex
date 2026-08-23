# Phase 4 evidence: the two third-party libraries, their versions, and the image-wide map

Read-only analysis. Every claim carries the RVA or raw file offset that
establishes it. Oracle: `Binaries/NxPhysics.dll`, SHA-256
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`, 1,253,376
bytes, re-verified before this document was written. Image base `0x10000000`;
`.text` RVA `0x00001000`, virtual size 1,056,977. The PE has `raw_offset == rva`
for every section, so an RVA below `0x00124000` is also its raw file offset.

Method, in the order the conclusions depend on it:

1. **Instruction ownership.** All 319,319 instructions in
   `oracle/capstone/manifest.json` fall inside the extent of exactly one of the
   6,338 census function rows; nothing is stranded. A per-row scan of operand
   immediates is therefore a complete scan of the image's absolute-address uses.
2. **Literal-pool exclusivity.** For each candidate library, the set of `.rdata`
   and `.data` addresses referenced from inside its code span and from nowhere
   else. A library's own message set is unique to it; a *pooled* literal is not
   (see §5).
3. **The direct-call boundary.** Every `call rel32` crossing the span, both ways.
4. **Class structure.** Vtables, constructor field-initialisation order, and
   `sizeof` read out of allocation sites — the evidence that survives having no
   RTTI (the image contains zero `.?AV` strings, i.e. `/GR-`) and no symbols.

---

## 1. qhull is **2003.1, released 2003/12/30**

Confidence: **certain**. The version string is in the image.

### 1.1 The version string, and the correction it forces

The Task 1b brief records: "There is no embedded date string: qhull's `Qhull %s:`
format takes its version from a global, and I found no `YYYY/MM/DD` constant in
the image." **That is wrong, and the string is the whole answer.**

| what | where |
| --- | --- |
| the literal `2003.1 2003/12/30` (17 bytes + NUL) | `.rdata` RVA `0x00109f90`, raw file offset `0x00109f90` |
| a relocated pointer holding `0x10109f90` | `.data` RVA `0x00122344`. A byte search of the whole file finds the constant `0x10109f90` exactly once, at raw offset `0x00122344` |
| the three rows that read that slot | `phys_fn_002862` `0x0006d200`, `phys_fn_003200` `0x0007a330`, `phys_fn_003408` `0x00084510` |
| the format `phys_fn_003200` pairs it with | `Options selected for Qhull %s:\n%s\n` at `.rdata:0x0010d350` — this is the `Qhull %s:` the brief saw |
| the format `phys_fn_003408` pairs it with | `\n%s\n qhull invoked by: %s \| %s\n%s with options:\n%s\n` at `.rdata:0x001159f4` |

No function references `0x00109f90` directly because `qh_version` is a
`const char *` variable, not a `const char[]` array: the code loads the *pointer*
at `.data:0x00122344`. A scan of operand immediates therefore never sees the
string address, only the slot address — which is why the earlier search missed
it, and which is itself a version signal (later `libqhull` releases declare
`qh_version` as an array).

`2003.1 2003/12/30` is the release identifier and release date of Qhull 2003.1.
No later release can carry it, so every release after 2003.1 is ruled out
outright.

### 1.2 Two independent structural corroborations

**`qh_rand`/`qh_srand` — Park & Miller's minimal standard generator, present.**

| row | rva | what the bytes say |
| --- | --- | --- |
| `phys_fn_002513` | `0x0005fe20` | `mov eax,[0x10122340]`; `mov ecx,0x1f31d` (127773 = *m div a*) at `0x0005fe26`; `idiv ecx`; `imul eax,eax,0xb14` (2836 = *m mod a*) at `0x0005fe2d`; `imul edx,edx,0x41a7` (16807 = *a*) at `0x0005fe33`; `sub edx,eax`; `add eax,0x7fffffff` when the result is not positive, at `0x0005fe41`; store back to `0x10122340` |
| `phys_fn_002515` | `0x0005fe50` | clamps a seed below 1 to `1` (`0x0005fe59`) and a seed at or above `0x7fffffff` to **`0x7ffffffe`** (`0x0005fe69`) — that is `qh_rand_m - 1`, and it is the only occurrence of the immediate 2147483646 anywhere in the image |
| `qh_last_random` | `.data:0x00122340` | initialised to `1` in the file image (bytes `01 00 00 00` at raw offset `0x00122340`), immediately below `qh_version` |

These 16807 / 127773 / 2836 immediates occur five times each, all inside the
qhull span. A build that used the C library `rand()` instead would show
`RAND_MAX`-shaped code here; it does not — all 18 occurrences of the immediate
`0x7fff` in the image are outside the qhull span.

**The 2003.1 feature set is in the message table.** The qhull literal pool holds
1,100 distinct NUL-terminated strings, and 144 distinct `qh_*` function names
appear inside them. Two families present are ones a 2002.1 build does not have:

- the `Qt` triangulated-output machinery — `qh_triangulate`,
  `qh_triangulate_facet`, `qh_triangulate_link`, `qh_triangulate_mirror`,
  `qh_triangulate_null`, plus the option names `Qtriangulate`
  (`.rdata:0x0010aad8`) and `Q11-trinormals Qtriangulate` (`0x0010a9c0`);
- the Voronoi-normal output — `qh_detvnorm` (`qh_detvnorm: Voronoi vertex or
  midpoint` at `0x0010bf40`, plus `qh_detvnorm: too few points (%d)` and
  `qh_detvnorm: points %d %d midpoint dist`), `qh_detvridge3`, `qh_eachvoronoi`,
  `qh_printvdiagram`, `qh_markvoronoi`, plus the option `QTestPoints`
  (`0x0010aacc`).

The option-letter table runs `.rdata:0x00109a97`–`0x0010bd00` (`<_wide-facet`,
`@Width-outside`, `?QJoggle`, `_joggle-seed`, `QupperDelaunay`,
`Qbbound-last-qj`, `FCentrums`, `Gintersections`, `Q10-no-narrow`,
`Q11-trinormals Qtriangulate`, …) and can be diffed name-for-name against a
fetched `global.c`; §6 gives the procedure.

### 1.3 Build-time configuration baked into the code

- **`qh_QHpointer` is OFF.** `phys_fn_002425` at `0x0005c5c0` is `qh_distplane`
  (it owns the message `qh_distplane: ` at `.rdata:0x001088f8`), and its first
  instruction is `mov ecx, dword ptr [0x101248c4]` — a *direct absolute* load of
  `qh hull_dim` — followed by `add ecx,-2`, `cmp ecx,6` and a 7-way jump table
  at `.text:0x0005c7ac` for dimensions 2 through 8. A `qh_QHpointer` build would
  load a pointer first and index off it. So `qhT qh` is a static global, the
  library is non-reentrant, and a vendored `user.h` must keep `qh_QHpointer`
  at 0.
- **The `qh` / `qhmem` / `qhstat` globals occupy `.data` RVA `0x00124678`–
  `0x001263a0`** — 1,126 distinct slots, referenced from qhull code and from
  nowhere else. That range is past `.data`'s 8,192 raw bytes, so it is
  zero-filled at load, as C statics should be.
- **stdio is compiled in.** Twelve CRT rows (`0x000f5068`, `0x000f5130`,
  `0x000f515d`, `0x000f51e0`, `0x000f536d`, `0x000f55ba`, `0x000f5760`,
  `0x000f59f0`, `0x000f5cb0`, `0x000f6124`, `0x000f61a0`, `0x000f621b`) have
  qhull rows as their **only** callers in the whole image. qhull is the only
  translation unit in `NxPhysics.dll` that uses `fprintf`, so its `qh ferr`
  tracing survived into the shipped Release build.

### 1.4 What NovodeX changed in qhull

**The allocator is not qhull's.** This is the modification most likely to make a
naive vendoring diverge silently.

| address | measurement |
| --- | --- |
| `0x0007ea1a` | `phys_fn_003279` opens with `mov eax,0x40d8` and a call to `_chkstk` — a 16,600-byte **stack** frame |
| `0x0007ea51` | `lea ecx,[ebp-0x4068]`, then `mov dword ptr [0x10125080], ecx` — the address of a 16,488-byte *stack* object is published to a global |
| `0x0007ea78` | `mov eax,[ebp-0x4068]` … `call dword ptr [eax+0x14]` — allocation is a **virtual call**, so the object has a vtable |
| `0x00084800` | `phys_fn_003413`, 17 bytes, 76 callers: `mov ecx,[0x10125080]`; `mov eax,[ecx]`; `push arg`; `call dword ptr [eax+0x20]` — free, also a virtual call |
| — | **175 of the 492 qhull rows** reference `0x00125080` |

Stock qhull allocates through `qh_memalloc`/`qh_setmalloc` and the `qh_malloc`/
`qh_free` macros in `user.h`, which expand to the C library. Here every
allocation site is redirected to a C++ interface reached through a global, and
the whole hull build runs inside a stack arena. Whether that was done by
redefining the `user.h` macros (the cheap explanation, and the one favoured by
175 *inlined* expansions rather than calls into one wrapper) or by editing
`mem.c` cannot be settled from the image alone. Either way a vendored
`mem.c`/`user.h` will not produce this code without the same redefinition.

**There is a NovodeX-written driver inside the qhull address span.**
`phys_fn_003279` at `0x0007ea10` (819 bytes) builds a two-element `argv`
(`[ebp-0x14] = "qhull"` at `0x0007ea35`; `[ebp-0x10] = 0x1011363c` at
`0x0007ea3c`), installs the arena, and drives the hull. It takes no command
string from its caller: `phys_fn_002233` at `0x00054920` passes a struct it
fills with `0xb7`, `0x3727c5ac` (1e-5f), `0x100` and `0x3f4ccccd` (0.8f) at
`0x0005494d`–`0x0005496f`. That is not `qh_new_qhull`'s signature. So at least
one source file in the qhull object set is NovodeX's own, and vendoring upstream
qhull will not supply it.

**Nothing else crosses the boundary.** The only two entries into the qhull span
from the rest of the image are `phys_fn_003279` (`0x0007ea10`, called from
`0x00054975`) and `phys_fn_003255` (`0x0007e300`, called from `0x00054a26`),
both from `phys_fn_002233`. This reproduces Task 1's finding exactly.

---

## 2. OPCODE is **1.3.x** — 1.3 or 1.3.1/1.3.2, and not 1.2 or earlier

Confidence: **1.3.x certain; the third digit undetermined.** There is no version
string for OPCODE anywhere in the image — its entire literal pool is twelve
strings — so the identification is structural. Every 1.3-specific structure the
image can show is present and matches. Nothing I could measure separates 1.3 from
1.3.1/1.3.2, so the range is reported rather than a point.

### 2.1 What rules out OPCODE 1.2 and earlier

**`MeshInterface` exists.** OPCODE 1.2 and earlier had no mesh-interface class;
`OPCODECREATE` carried triangle/vertex counts and pointers directly.

- `phys_fn_005358` at `0x000e9020` is `MeshInterface::SetPointers`: null-test both
  arguments (`0x000e9024`, `0x000e902c`), store the second to `[ecx+0xc]`
  (`0x000e9030`) and the first to `[ecx+8]` (`0x000e9033`), return `true`;
  otherwise push `MeshInterface::SetPointers: pointer is null`
  (`.rdata:0x0011ba48`), the file string
  `\Epic\Novodex\SDKs\Physics\src\opcode\OPC_MeshInterface.cpp`
  (`.rdata:0x0011ba74`) and line `0xe6` = **230**.
- `phys_fn_002083` at `0x00052280` writes `mNbVerts` to `MeshInterface+4`
  (`0x000522a2`) and `mNbTris` to `MeshInterface+0` (`0x000522af`) before
  calling `SetPointers`. So the layout is `mNbTris`(+0), `mNbVerts`(+4),
  `mTris`(+8), `mVerts`(+0xc) with **no vtable pointer and no stride fields** —
  `OPC_USE_CALLBACKS` off, `OPC_USE_STRIDE` off.

**`BuildSettings` exists, with 1.3's defaults.** `phys_fn_005372` at `0x000e92b0`
is `OPCODECREATE::OPCODECREATE()`. Its stores, in issue order:

| address | store | reading |
| --- | --- | --- |
| `0x000e92b2` | `[+0x0c] = 0x7fffffff` | inline `BuildSettings()` member ctor: `mRules = SPLIT_FORCE_DWORD` |
| `0x000e92bb` | `[+0x10] = 0` | *extra field* (§2.3) |
| `0x000e92be` | `[+0x14] = 0xffffffff` | *extra field* |
| `0x000e92c5` | `[+0x18] = 0` | *extra field* |
| `0x000e92cd` | `[+0x08] = 1` | inline `BuildSettings()` member ctor: `mLimit = 1` |
| `0x000e92d0` | `[+0x00] = 0` | `mIMesh = null` |
| `0x000e92d2` | `[+0x04] = 0` | *extra field* |
| `0x000e92d5` | `[+0x0c] = 0x22` | body: `mSettings.mRules = SPLIT_SPLATTER_POINTS \| SPLIT_GEOM_CENTER` |
| `0x000e92dc` | `[+0x08] = 1` | body: `mSettings.mLimit = 1` |
| `0x000e92df` | `[+0x1c] = 1` | `mNoLeaf = true` |
| `0x000e92e2` | `[+0x1d] = 1` | `mQuantized = true` |
| `0x000e92e5` | `[+0x1e] = 0` | `mKeepOriginal = false` |
| `0x000e92e8` | `[+0x1f] = 0` | `mCanRemap = false` |

`0x22` is `(1<<1) | (1<<5)`. The double write of `[+0x0c]` — `SPLIT_FORCE_DWORD`
first from the member initialiser, then `0x22` from the constructor body — is the
signature of a struct whose `BuildSettings` member has its own inline default
constructor and is then overwritten. `SPLIT_GEOM_CENTER` does not exist before
1.3.

**`AABBOptimizedTree` is a polymorphic base with four derived trees.** Five
contiguous vtables, eight slots each:

| vtable | class | node size, from that class's own `GetUsedRam` |
| --- | --- | --- |
| `.rdata:0x0011bc4c` | `AABBOptimizedTree` — seven of eight slots are `_purecall` (`0x000f41dc`) | — |
| `.rdata:0x0011bc6c` | `AABBCollisionTree` | `imul eax,eax,0x1c` at `0x000f2473` and `0x000f2573` → **28** |
| `.rdata:0x0011bc8c` | `AABBNoLeafTree` | `shl eax,5` at `0x000f26b3` and `0x000f2eb3` → **32** |
| `.rdata:0x0011bcac` | `AABBQuantizedTree` | `shl eax,4` at `0x000f3003`; `(n+2)*16` at `0x000f3623` → **16** plus 32 bytes of coefficients |
| `.rdata:0x0011bcd0` | `AABBQuantizedNoLeafTree` | `lea eax,[eax+eax*4]; shl eax,2` at `0x000f36d3` → **20**; `20n+0x20` at `0x000f3d36` |

28 / 32 / 16 / 20 are `AABBCollisionNode`, `AABBNoLeafNode`, `AABBQuantizedNode`
and `AABBQuantizedNoLeafNode` byte for byte. The four-way *polymorphic* hierarchy
under one base is a 1.3 restructuring.

**`AABBTree::Build`'s linear node pool is present.** `phys_fn_005523` at
`0x000f10c0`:

- allocates `mIndices` as `nbPrimitives*4` (`shl ecx,2` at `0x000f10fa`) and
  fills the identity permutation in the loop at `0x000f1120`;
- `mNodePrimitives = mIndices` (`0x000f112e`), `mNbPrimitives` (`0x000f1137`);
- tests `builder->mSettings.mLimit == 1` at `0x000f113a`, and only then allocates
  `2n-1` nodes (`lea ebp,[ebp+ebp-1]` at `0x000f1143`) of **36 bytes** each
  (`[ecx*4+4]` where `ecx = 9*ebp`, at `0x000f114e`–`0x000f1152`; the array
  constructor at `0x000f116d` passes element size `0x24`);
- writes the pool base back into the builder at `0x000f1180`.

36 bytes is `AABBTreeNode` = `AABB`(24) + `mPos`(4) + `mNodePrimitives`(4) +
`mNbPrimitives`(4). "Use a linear array for complete trees, and hand the pool
base back to the builder" is the change 1.3 introduced.

**`Model::Build` and `RayCollider::ValidateSettings` are 1.3's, statement for
statement.**

- `phys_fn_005368` at `0x000e9100`: null `mIMesh` (`0x000e910b`) → false;
  `mIMesh->IsValid()` (`0x000e9115`) → false; then `mSettings.mLimit != 1`
  (`0x000e9129`) → `OPCODE WARNING: supports complete trees only! Use mLimit = 1.\n`
  (`.rdata:0x0011bae4`) against `OPC_Model.cpp` (`.rdata:0x0011bb24`) line
  `0x93` = **147**.
- `phys_fn_004903` at `0x000b5770` returns, in this order:
  `Higher distance bound must be positive!` (`0x0011b724`, guard at
  `0x000b5776`); `Temporal coherence only works with First contact mode!`
  (`0x0011b6ec`); `Closest hit doesn't work with First contact mode!`
  (`0x0011b6b8`); `Temporal coherence can't guarantee to report closest hit!`
  (`0x0011b67c`); `SkipPrimitiveTests not possible for RayCollider ! (not
  implemented)` (`0x0011b638`). Five tests, that order, those exact spellings —
  including the two where the source's doubled-quote concatenation collapses to
  a single run in the binary.

**The `AABBTreeCollider` recursion set is the 1.3 set.** `phys_fn_005434` at
`0x000ef0d0` dispatches to six recursive collide bodies —
`0x000e9bb0` (4,848 B), `0x000eaea0` (2,435 B), `0x000eb990` (2,569 B),
`0x000ec580` (4,686 B), `0x000ed960` (4,880 B) and `0x000eec70` (526 B) — one
per node type plus the two triangle/box arms.

### 2.2 What I could *not* use to separate 1.3 from 1.3.1/1.3.2

No version string, no `__DATE__`, no RTTI. `HybridModel` is not compiled in —
only two model vtables exist, `BaseModel` at `.rdata:0x0011bb5c` and `Model` at
`.rdata:0x0011bac8` — which removes the class whose interface moved most across
the 1.3.x point releases. Fetch 1.3 and 1.3.2 and discriminate against source;
§6 gives the procedure.

### 2.3 What NovodeX changed in OPCODE — four modifications, all load-bearing

1. **`OPCODECREATE` and `BuildSettings` were extended by four fields.** From
   §2.1's table the struct is 32 bytes, with fields at `+0x04`, `+0x10`, `+0x14`
   and `+0x18` that stock 1.3 does not have (stock is 16 bytes: `mIMesh`, a
   two-`udword` `mSettings`, four `bool`s). The extras carry a height field:
   `phys_fn_002083` compares its first argument to `0xff` at `0x000522fa` —
   `NX_NOT_HEIGHTFIELD` — and only when it differs stores it to `+0x14`
   (`0x00052311`) and the second argument to `+0x10` (`0x00052319`). The ctor
   defaults `+0x14` to `0xffffffff`, `+0x10` and `+0x18` to 0.
   `Model::Build` copies **five** consecutive dwords from `OPCODECREATE+8` into
   the builder (`0x000e91cc`–`0x000e91f9`), not the two a stock `mSettings`
   assignment would; and `AABBTreeOfTrianglesBuilder`'s constructor
   `phys_fn_005360` at `0x000e9060` initialises the matching five (`+4=1`,
   `+8=0x7fffffff`, `+0xc=0`, `+0x10=0xffffffff`, `+0x14=0`). The extension runs
   all the way through the builder.
2. **A "build from a serialized blob" path was added.** `OPCODECREATE+0x04` is a
   pointer; when it is non-null `Model::Build` skips the `mLimit` check
   (`0x000e9125`), skips the tree build entirely, and dispatches `vtable[0x18]`
   with it (`0x000e916d`), returning immediately. This is what consumes the
   acceleration blob in the triangle-mesh stream Task 1 recovered. Stock OPCODE
   1.3 has no serialization at all.
3. **Tree serialization was added.** `0x000f2580`, slot 5 of `AABBCollisionTree`'s
   vtable, walks the node array and calls the NovodeX stream helpers
   `phys_fn_004797` (`0x000b3f00`, twice) and `phys_fn_004799` (`0x000b3f50`).
   Each of the four tree classes has the same slot.
4. **A single-triangle short circuit.** `Model::Build` reads `mIMesh->mNbTris` at
   `0x000e917b`; if it is 1 it sets bit 2 of `Model+8` (`0x000e9182`) and returns
   `true` without building a tree.

And two source files sit in the OPCODE directory that no OPCODE distribution has:

- **`IcePrunable.cpp`** — `\Epic\Novodex\SDKs\Physics\src\opcode\IcePrunable.cpp`
  at `.rdata:0x0011b5d4`, with `Invalid pruning type` (`0x0011b5bc`, line
  `0x98` = 152; the setter rejects values outside `[0,4)` and stores a byte at
  `this+0x2a` — `phys_fn_004888` at `0x000b55e0`) and `Invalid pruning section`
  (`0x0011b60c`, line `0xae` = 174; range `[0,3)`, byte at `this+0x2b` —
  `phys_fn_004890` at `0x000b5610`). Its class vtable is `.rdata:0x0011b5a4`,
  installed by `phys_fn_004874` at `0x000b54ac` and by `phys_fn_004892`
  (`0x000b5649`) and `phys_fn_004896` (`0x000b56d9`).
- **`IceAdjacencies.cpp`** — `\Epic\Novodex\SDKs\Physics\src\IceAdjacencies.cpp`
  at `.rdata:0x00108528`, referenced from `phys_fn_002241` at `0x00054bb0`, laid
  out at `0x0002dbc0`–`0x0002dcf0` — **outside** both library spans, inside
  NovodeX's own `mesh` component.

Both names are `Ice`-prefixed, so the OPCODE-side source NovodeX had was a larger
ICE snapshot than the public OPCODE zip ships, or NovodeX renamed and added
files. Either way, fetching upstream OPCODE 1.3 will not reproduce this
directory.

---

## 3. The image-wide map

### 3.1 The four spans, and how each end was fixed

| region | span | how the low end was fixed | how the high end was fixed |
| --- | --- | --- | --- |
| **qhull** | `0x0005c5c0` – `0x00084a50` | `phys_fn_002425` (`qh_distplane`) is the first row that references the qhull literal pool; the pool's first entry `from p%d to f%d\n` at `.rdata:0x001088dc` is referenced only from `0x0005c5c0` | `phys_fn_003417` at `0x000849b0` is the last; the next row, `phys_fn_003419` at `0x00084a50`, begins the fluid library, whose literals start at `.rdata:0x00115aac` |
| **OPCODE/Ice core** | `0x000b55e0` – `0x000f40e9` | `phys_fn_004888`, the first `IcePrunable.cpp` asserter | `phys_fn_005662` at `0x000f40a0` (`AABBQuantizedNoLeafTree` ctor); `0x000f40ea` is CRT `_fpmath` initialisation, and `0x000f41dc` is `_purecall` |
| **OPCODE `IcePrunable.cpp` head** | `0x000b5460` – `0x000b55e0` | see the correction below | the core boundary |
| **CRT** | `0x000f40e9` – `0x001030d1` | `_fpmath` init at `0x000f40ea`; entry point is `0x000f74bd` | end of `.text` |

> **Corrected by P4 Task 3; the low end was 64 bytes too high.** This row used to read
> `0x000b54a0` – `0x000b55e0`, fixed by "`phys_fn_004874` at `0x000b54a0` installs the vtable
> `.rdata:0x0011b5a4` whose slots 1–4 are `0x000b54f0`, `0x000b5520`, `0x000b5550`, `0x000b5570`
> and whose slot 0 is `0x000b56d0` — inside the core". That is true of the *asserters* and not of
> the file. `phys_fn_004870` at `0x000b5460` (25 bytes) and `phys_fn_004872` at `0x000b5480` (23)
> belong to it too: each reads a hook global at `.data:0x10128470`/`0x10128474`, replaces its first
> stack argument with `argument->[4]` — `Prunable::mOwner` — and tail-jumps, and
> `Prunable::Prunable` installs both addresses at `.data:0x001284fc`/`0x00128500`
> (`0x000b54d0`, `0x000b54da`). The rows below them, `phys_fn_004866` at `0x000b53c0` and
> `phys_fn_004868` at `0x000b5410`, are Phase 7. P4 Task 2b found this and did not fix it; the
> derivation is in `evidence/phase4-pmap-reconstruction.md` §7 (b).
>
> **No ledger moves.** The two rows are Phase 4, not `third_party`, `discovered`, and claimed by
> nobody: Task 2b wrote them in `Physics/src/opcode/IcePrunable.cpp` and explicitly did not claim
> them. Redrawing the low end moves them out of Task 3's out-of-span population (229 rows / 60,146
> bytes becomes 227 / 60,098) and into Task 2b's in-span one (186 / 37,260 becomes 188 / 37,308).
> Every other row-count and byte-count in this document is computed from the qhull and OPCODE-core
> spans and is unaffected.

Three independent signals agree on the qhull span:

- **Literal-pool exclusivity.** `.rdata:0x001088dc`–`0x00115a8c` (53,168 bytes,
  1,100 strings) is referenced from code in `0x0005c5c0`–`0x00084a50` and from
  nowhere else, once pooled generics are removed (§5). `.data:0x00124678`–
  `0x001263a0` and `.data:0x00122340`/`0x00122344` likewise.
- **The direct-call boundary.** Exactly two entries in (§1.4). Calls out go only
  to `phys_fn_001583` (`0x0002ea70`, a one-byte assert stub) and 28 CRT rows.
- **Contiguity.** The span's 994 rows tile it with only 1,098 bytes uncovered,
  all of which are the 12 switch tables and their byte index arrays that the
  census already records as `phys_data_000012`–`phys_data_000405`.

The OPCODE literal/vtable pool is `.rdata:0x0011b5a4`–`0x0011bcf8`; every
referencing row lies in `0x000b54a0`–`0x000f40a0`, and `0x0011bcf8` onward is
CRT (`type_info`, `bad_alloc`, `CorExitProcess`, the month names).

### 3.2 Code rows and bytes, by category and by the phase the census gives them

Alignment padding is excluded here and counted separately in §3.3.

| category | ph2 | ph3 | ph4 | ph5 | ph6 | ph7 | ph8 | total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| **qhull** | 14 / 3,079 | – | 464 / 154,600 | 14 / 2,689 | – | – | – | **492 / 160,368** |
| **OPCODE core** | – | – | 250 / 210,384 | – | – | – | 150 / 43,495 | **400 / 253,879** |
| **OPCODE `IcePrunable.cpp` head** | 1 / 34 | 5 / 213 | – | – | – | 1 / 29 | – | **7 / 276** |
| **undetermined glue** | 5 / 551 | 3 / 237 | 9 / 578 | – | – | 7 / 597 | – | **24 / 1,963** |
| **NovodeX** | 143 / 32,994 | 433 / 145,233 | 223 / 59,627 | 162 / 57,395 | 433 / 147,006 | 554 / 113,410 | – | **1,948 / 555,665** |
| **CRT / compiler** | – | – | – | – | – | – | 573 / 60,553 | **573 / 60,553** |

Third-party total: **899 rows and 414,523 bytes**, 39.2% of `.text`'s 1,056,977
virtual bytes. Task 1's figure was 714 rows and 364,984 bytes (34.5%). The
delta is **+185 rows and +49,539 bytes**.

### 3.3 Alignment padding inside each span

All of it is phase 8 and `kind: compiler_artifact`, and all of it verifies as
`int3`, `nop`, a multi-byte `lea`-form `nop`, or a two-byte `jmp` over padding —
no real code.

| region | rows | bytes |
| --- | ---: | ---: |
| qhull | 502 | 3,542 |
| OPCODE core | 375 | 2,866 |
| OPCODE `IcePrunable.cpp` head | 7 | 44 |
| undetermined glue | 22 | 165 |
| NovodeX | 1,885 | 14,824 |
| CRT | 103 | 535 |

6,338 rows in, 6,338 rows out.

### 3.4 Data objects

| category | ph2 | ph3 | ph4 | ph5 | ph6 | ph7 | ph8 | total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| **qhull** | 58 / 1,312 | – | 2,654 / 56,799 | 16 / 341 | – | – | – | **2,728 / 58,452** |
| **OPCODE** | 26 / 420 | 1 / 4 | 62 / 990 | – | – | – | – | **89 / 1,414** |
| **CRT** | – | – | – | – | – | – | 17 / 328 | **17 / 328** |
| **NovodeX / other** | 967 / 70,576 | 101 / 4,223 | 97 / 3,113 | 106 / 5,886 | 531 / 21,718 | 502 / 11,174 | – | **2,304 / 116,690** |

5,138 objects in, 5,138 out. qhull's objects are the `.text` switch tables inside
its span, its `.rdata` literal pool, and its `.data` globals; OPCODE's are its
`.text` switch tables, the vtable/literal pool `0x0011b5a4`–`0x0011bcf8`, and 46
small `.data` slots in `0x00128450`–`0x00128530`.

### 3.5 Every row whose category differs from what the census implies

> **Superseded in one respect, and left standing as written.** Every phase this section quotes is the
> census *as it stood when this document was written*. Task 1c acted on the whole of it: all 150 rows
> in (a), all 28 in (b), all 7 in (c) and the (d) band are Phase 4 now, and the category findings below
> are why. The phase columns here are the defect, not the current state; the current state is the Task
> 1c report and `inventory.json`.

**(a) 150 OPCODE rows the census called Phase 8 compiler/runtime artifacts —
43,495 bytes.** This is the finding Task 1 did not have. The census's phase-4
`spatial_tree` component stops at `0x000e9280`; OPCODE runs on to `0x000f40e9`.
149 of the 150 carry `phase_provenance: runtime_tail`, so the mechanism is
visible: the census drew the "everything after this is runtime tail" line at
`0x000e9280`, and it is 43 KB early. The rows include `Model::~Model`
(`0x000e9280`), `OPCODECREATE::OPCODECREATE` (`0x000e92b0`, called from Phase 4
rows `0x00050640` and `0x00052280`), `BaseModel`'s constructor (`0x000e92f0`),
`AABBTree::Build` (`0x000f10c0`), all four optimized-tree classes and all six
`AABBTreeCollider` recursion bodies. Full list in §7.

**(b) 28 qhull rows the census gave to Phase 2 (14 rows, 3,079 bytes) and
Phase 5 (14 rows, 2,689 bytes).** Task 1 already recorded these; this task
re-derived the same 28 independently from the literal pool and the span, and
confirms them. Full list in §8.

**(c) 7 rows of `IcePrunable.cpp` the census gave to Phases 2, 3 and 7 —
276 bytes.** `phys_fn_004874` (`0x000b54a0`, ph3, 76 B), `phys_fn_004876`
(`0x000b54f0`, ph3, 40 B), `phys_fn_004878` (`0x000b5520`, ph3, 45 B),
`phys_fn_004880` (`0x000b5550`, ph3, 25 B), `phys_fn_004882` (`0x000b5570`,
ph3, 27 B), `phys_fn_004884` (`0x000b5590`, ph7, 29 B), `phys_fn_004886`
(`0x000b55b0`, ph2, 34 B). Evidence: `0x000b54a0` installs the vtable
`.rdata:0x0011b5a4`, slots 1–4 of which are the four ph3 rows and slot 0 of
which is `phys_fn_004896` at `0x000b56d0` inside the OPCODE core;
`phys_fn_004884` reads the same `+0x28` word sentinel `0xffff` and `+0x20`
pointer as `phys_fn_004894` (`0x000b5670`, core); `phys_fn_004886` reads the same
global `.data:0x00128478` as `phys_fn_004894`.

**(d) 24 rows I leave undetermined — `0x000b4c40`–`0x000b54a0`, 1,963 bytes.**
This band is NovodeX glue that constructs OPCODE objects, and I could not settle
which side of a vendoring boundary it falls on. Five of them are called *from*
inside OPCODE, which is why they are not simply NovodeX: `phys_fn_004830`
(`0x000b4cc0`, ph4) is a singleton getter over the global `.data:0x0012846c`
that constructs through `0x000ef270` and is called from `0x000f1550`;
`phys_fn_004832` (`0x000b4d20`, ph4) calls `0x000ef5c0` and is called from
`0x000f15a0` and `0x000f15c0`; `phys_fn_004852` (`0x000b5090`, ph4) is a
four-way factory that allocates `0x3c`/`0x40`/`0x90`/`0xa0` bytes through the
NovodeX Foundation allocator and constructs four OPCODE collider classes
(`0x000ef850`, `0x000e5870`, `0x000e56d0`, `0x000ef670`); `phys_fn_004834`
(`0x000b4d40`, ph4) and `phys_fn_004855` (`0x000b5140`, ph7) likewise. They use
the NovodeX allocator and are called from NovodeX core, so they read as NovodeX's
adapter layer over OPCODE — but a vendored OPCODE will not contain them and a
reconstruction has to write them, so Task 1c must decide deliberately rather than
inherit a default. The remaining 19 rows in the band are NovodeX array and
allocator helpers with dozens of NovodeX callers each and no OPCODE link.

**(e) One byte.** `phys_fn_005663` at `0x000f40e9` is a single `c3`. It is on the
OPCODE/CRT seam and is not worth a decision either way.

---

## 4. Which x87 control word each library runs under

| library | `0x027f` (CRT default, consumer API) | `0x0f7f` (installed by `Scene::simulate`) |
| --- | --- | --- |
| qhull | **yes** — reached only from `phys_fn_002233` (`0x00054920`), whose only caller is `phys_fn_002260` (`0x00055890`), slot 2 of the `TriangleMesh` vtable | **no direct-call evidence** — 0 qhull rows appear in the direct-call closure from `Scene::simulate` at `0x00013c40` |
| OPCODE | **yes** — `Model::Build` (`0x000e9100`) and `OPCODECREATE::OPCODECREATE` (`0x000e92b0`) are called from `0x00050640` and `0x00052280`, the triangle-mesh load path | **yes** — 8 OPCODE rows are in the direct-call closure from `Scene::simulate`, among them `0x000e6750`, `0x000e6ca0` and `0x000e7180`, all reached from `0x0004c290` |

So **OPCODE must be bit-exact under both control words** and qhull, on present
evidence, only under `0x027f`. The `Scene::simulate` closure is 178 entries and
57 of them carry indirect calls, so the qhull row is a measured absence from a
lower bound, not a proof of absence.

---

## 5. What string pooling and COMDAT folding cost this analysis

MSVC pools identical string literals and folds identical COMDATs, so a literal
inside a library's pool can be referenced by code that is not that library, and a
tiny function can have one address serving several translation units. Six
addresses inside the qhull pool are referenced from outside the qhull span and
are **not** qhull evidence:

| address | contents | referenced from |
| --- | --- | --- |
| `0x00109ae8` | the double `3.0` | `0x000886b0` |
| `0x0010a3d0` | `"w"` | `0x0009cba0` |
| `0x0010c1f4` | `"%d "` | `0x00092b50` |
| `0x0010c2e4` | `"%d %d %d "` | `0x00092b50` |
| `0x001135bc` | `"\r\n"` | `0x00090fb0`, `0x00092b50`, `0x00094ac0`, `0x000950f0` |
| `0x00113638` | the float `0.25` | `0x000886b0`, `0x000e76c0` |
| `0x0010d744` | `"\n\n"` | `0x000fe275`, `0x000fe5e6` (CRT) |

Two folded functions cross the other way: `0x000538b0` and `0x000538a0`, which
sit in `TriangleMesh.cpp`'s address range, occupy slots 0 and 4 of OPCODE's
`AABBTreeOfTrianglesBuilder` vtable at `.rdata:0x0011bab4`. **A folded address is
evidence about neither owner.** Only a library-unique literal attributes a row.

---

## 6. What to fetch, and what to check the moment it lands

**Do not treat the locations below as verified — nothing was downloaded for this
task.** They are the canonical homes to try; confirm the archive's own
`COPYING`/`README` before anything is vendored.

| library | release to fetch | canonical home |
| --- | --- | --- |
| **qhull** | **2003.1** exactly | `qhull.org` download page, "old versions" — `qhull-2003.1.zip` (Windows source) or `qhull-2003.1.tgz`. The modern `github.com/qhull/qhull` repository is the successor project and may not carry a 2003.1 tag |
| **OPCODE** | **1.3** and **1.3.2**, both, so the third digit can be settled | 1.3: Pierre Terdiman's `codercorner.com` OPCODE page. 1.3.2: bundled inside Open Dynamics Engine as `ode/OPCODE/` (the ODE repository or an ODE source release) |

**Licence, as it bears on vendoring into this repository — verify, do not assume.**
Both libraries are commonly described as permissive, and NovodeX shipped both
inside a commercial product, which is circumstantial evidence that redistribution
in source form was allowed. Neither is GPL. But the Qhull licence is a custom
Geometry Center text with a citation-request clause, not a stock BSD/MIT, and
OPCODE's terms live in its `README`, not in a named licence file. Read both
verbatim from the fetched archives and record the exact text in this repository
alongside the vendored source; do not paraphrase either into a `LICENSE` header.

**The first six checks to run against the fetched qhull 2003.1:**

1. `global.c` — is `qh_version` declared `const char *` (matches) or
   `const char[]` (does not)? Is its value exactly `2003.1 2003/12/30`?
2. `geom2.c` — are `qh_rand`/`qh_srand` present with `qh_rand_a` 16807,
   `qh_rand_q` 127773, `qh_rand_r` 2836, `qh_rand_m` 2147483647, and
   `static int qh_last_random = 1`?
3. `user.h` — `qh_QHpointer` must be 0. Record the shipped values of every
   `qh_` tunable; the image will disagree with at least `qh_malloc`/`qh_free`.
4. `global.c`'s `qh_initflags` option table — diff every option name against the
   list at `.rdata:0x00109a97`–`0x0010bd00`. A name in one and not the other is a
   version mismatch or a NovodeX edit and must be resolved before vendoring.
5. `io.c`/`poly2.c` — do `qh_eachvoronoi`, `qh_printvdiagram`, `qh_detvnorm`,
   `qh_detvridge3` and the five `qh_triangulate*` functions exist?
6. Count the distinct format strings in the release and compare with 1,100.

**The first five checks against the fetched OPCODE:**

1. `OPC_BaseModel.h` — is `OPCODECREATE` 16 bytes with `mIMesh`, `mSettings`,
   four `bool`s? It must be, and the image's 32 bytes is then the measured
   NovodeX delta.
2. `OPC_TreeBuilders.h` — `BuildSettings()` must initialise `mLimit(1)` and
   `mRules(SPLIT_FORCE_DWORD)`; `SPLIT_SPLATTER_POINTS` must be `1<<1` and
   `SPLIT_GEOM_CENTER` must be `1<<5`, so that the ctor default is `0x22`.
3. `OPC_OptimizedTree.h` — `sizeof` of the four node types must be 28, 32, 16
   and 20, and `AABBTreeNode` must be 36.
4. `OPC_RayCollider.cpp` — `ValidateSettings` must return the five strings in the
   order at `0x000b5770`.
5. `OPC_Model.cpp` line 147 and `OPC_MeshInterface.cpp` line 230 must be the two
   `SetIceError` calls the image names. If the line numbers differ, the point
   release differs — this is the sharpest 1.3 vs 1.3.2 discriminator available,
   because NovodeX's own edits would have to have been line-neutral for it to
   mislead.

---

## 7. The 150 OPCODE rows the census places in Phase 8

Each is real code, not padding: verified byte-wise against every MSVC alignment
encoding. All lie in `0x000e7c40`–`0x000f40a0`.

| row | rva | bytes | census provenance |
| --- | --- | ---: | --- |
</content>
</invoke>
| `phys_fn_005322` | `0x000e7c40` | 5 | runtime_artifact |
| `phys_fn_005370` | `0x000e9280` | 47 | runtime_tail |
| `phys_fn_005372` | `0x000e92b0` | 60 | runtime_tail |
| `phys_fn_005374` | `0x000e92f0` | 23 | runtime_tail |
| `phys_fn_005376` | `0x000e9310` | 249 | runtime_tail |
| `phys_fn_005378` | `0x000e9410` | 15 | runtime_tail |
| `phys_fn_005380` | `0x000e9420` | 19 | runtime_tail |
| `phys_fn_005382` | `0x000e9440` | 53 | runtime_tail |
| `phys_fn_005384` | `0x000e9480` | 61 | runtime_tail |
| `phys_fn_005386` | `0x000e94c0` | 133 | runtime_tail |
| `phys_fn_005388` | `0x000e9550` | 67 | runtime_tail |
| `phys_fn_005390` | `0x000e95a0` | 91 | runtime_tail |
| `phys_fn_005392` | `0x000e9600` | 126 | runtime_tail |
| `phys_fn_005394` | `0x000e9680` | 33 | runtime_tail |
| `phys_fn_005396` | `0x000e96b0` | 569 | runtime_tail |
| `phys_fn_005398` | `0x000e98f0` | 79 | runtime_tail |
| `phys_fn_005400` | `0x000e9940` | 345 | runtime_tail |
| `phys_fn_005402` | `0x000e9aa0` | 122 | runtime_tail |
| `phys_fn_005404` | `0x000e9b20` | 94 | runtime_tail |
| `phys_fn_005406` | `0x000e9b80` | 18 | runtime_tail |
| `phys_fn_005408` | `0x000e9ba0` | 11 | runtime_tail |
| `phys_fn_005410` | `0x000e9bb0` | 4848 | runtime_tail |
| `phys_fn_005411` | `0x000eaea0` | 2435 | runtime_tail |
| `phys_fn_005413` | `0x000eb830` | 337 | runtime_tail |
| `phys_fn_005415` | `0x000eb990` | 2569 | runtime_tail |
| `phys_fn_005417` | `0x000ec3a0` | 13 | runtime_tail |
| `phys_fn_005419` | `0x000ec3b0` | 459 | runtime_tail |
| `phys_fn_005421` | `0x000ec580` | 4686 | runtime_tail |
| `phys_fn_005423` | `0x000ed7d0` | 13 | runtime_tail |
| `phys_fn_005425` | `0x000ed7e0` | 372 | runtime_tail |
| `phys_fn_005427` | `0x000ed960` | 4880 | runtime_tail |
| `phys_fn_005428` | `0x000eec70` | 526 | runtime_tail |
| `phys_fn_005430` | `0x000eee80` | 539 | runtime_tail |
| `phys_fn_005432` | `0x000ef0a0` | 36 | runtime_tail |
| `phys_fn_005434` | `0x000ef0d0` | 351 | runtime_tail |
| `phys_fn_005436` | `0x000ef230` | 55 | runtime_tail |
| `phys_fn_005438` | `0x000ef270` | 182 | runtime_tail |
| `phys_fn_005440` | `0x000ef330` | 152 | runtime_tail |
| `phys_fn_005442` | `0x000ef3d0` | 485 | runtime_tail |
| `phys_fn_005444` | `0x000ef5c0` | 146 | runtime_tail |
| `phys_fn_005446` | `0x000ef660` | 14 | runtime_tail |
| `phys_fn_005448` | `0x000ef670` | 29 | runtime_tail |
| `phys_fn_005450` | `0x000ef690` | 50 | runtime_tail |
| `phys_fn_005452` | `0x000ef6d0` | 117 | runtime_tail |
| `phys_fn_005454` | `0x000ef750` | 53 | runtime_tail |
| `phys_fn_005456` | `0x000ef790` | 44 | runtime_tail |
| `phys_fn_005458` | `0x000ef7c0` | 44 | runtime_tail |
| `phys_fn_005460` | `0x000ef7f0` | 39 | runtime_tail |
| `phys_fn_005462` | `0x000ef820` | 39 | runtime_tail |
| `phys_fn_005464` | `0x000ef850` | 18 | runtime_tail |
| `phys_fn_005466` | `0x000ef870` | 536 | runtime_tail |
| `phys_fn_005468` | `0x000efa90` | 503 | runtime_tail |
| `phys_fn_005470` | `0x000efc90` | 208 | runtime_tail |
| `phys_fn_005471` | `0x000efd60` | 273 | runtime_tail |
| `phys_fn_005473` | `0x000efe80` | 40 | runtime_tail |
| `phys_fn_005475` | `0x000efeb0` | 27 | runtime_tail |
| `phys_fn_005477` | `0x000efed0` | 65 | runtime_tail |
| `phys_fn_005479` | `0x000eff20` | 148 | runtime_tail |
| `phys_fn_005481` | `0x000effc0` | 252 | runtime_tail |
| `phys_fn_005483` | `0x000f00c0` | 476 | runtime_tail |
| `phys_fn_005485` | `0x000f02a0` | 622 | runtime_tail |
| `phys_fn_005487` | `0x000f0510` | 20 | runtime_tail |
| `phys_fn_005489` | `0x000f0530` | 7 | runtime_tail |
| `phys_fn_005491` | `0x000f0540` | 31 | runtime_tail |
| `phys_fn_005493` | `0x000f0560` | 252 | runtime_tail |
| `phys_fn_005495` | `0x000f0660` | 29 | runtime_tail |
| `phys_fn_005497` | `0x000f0680` | 18 | runtime_tail |
| `phys_fn_005499` | `0x000f06a0` | 136 | runtime_tail |
| `phys_fn_005501` | `0x000f0730` | 136 | runtime_tail |
| `phys_fn_005503` | `0x000f07c0` | 90 | runtime_tail |
| `phys_fn_005505` | `0x000f0820` | 90 | runtime_tail |
| `phys_fn_005507` | `0x000f0880` | 14 | runtime_tail |
| `phys_fn_005509` | `0x000f0890` | 158 | runtime_tail |
| `phys_fn_005511` | `0x000f0930` | 115 | runtime_tail |
| `phys_fn_005513` | `0x000f09b0` | 1085 | runtime_tail |
| `phys_fn_005515` | `0x000f0df0` | 167 | runtime_tail |
| `phys_fn_005517` | `0x000f0ea0` | 442 | runtime_tail |
| `phys_fn_005519` | `0x000f1060` | 23 | runtime_tail |
| `phys_fn_005521` | `0x000f1080` | 56 | runtime_tail |
| `phys_fn_005523` | `0x000f10c0` | 229 | runtime_tail |
| `phys_fn_005525` | `0x000f11b0` | 404 | runtime_tail |
| `phys_fn_005527` | `0x000f1350` | 418 | runtime_tail |
| `phys_fn_005529` | `0x000f1500` | 79 | runtime_tail |
| `phys_fn_005531` | `0x000f1550` | 37 | runtime_tail |
| `phys_fn_005533` | `0x000f1580` | 5 | runtime_tail |
| `phys_fn_005535` | `0x000f1590` | 5 | runtime_tail |
| `phys_fn_005537` | `0x000f15a0` | 26 | runtime_tail |
| `phys_fn_005539` | `0x000f15c0` | 51 | runtime_tail |
| `phys_fn_005541` | `0x000f1600` | 444 | runtime_tail |
| `phys_fn_005543` | `0x000f17c0` | 420 | runtime_tail |
| `phys_fn_005545` | `0x000f1970` | 178 | runtime_tail |
| `phys_fn_005547` | `0x000f1a30` | 56 | runtime_tail |
| `phys_fn_005549` | `0x000f1a70` | 80 | runtime_tail |
| `phys_fn_005550` | `0x000f1ac0` | 412 | runtime_tail |
| `phys_fn_005552` | `0x000f1c60` | 70 | runtime_tail |
| `phys_fn_005554` | `0x000f1cb0` | 93 | runtime_tail |
| `phys_fn_005556` | `0x000f1d10` | 49 | runtime_tail |
| `phys_fn_005558` | `0x000f1d50` | 93 | runtime_tail |
| `phys_fn_005560` | `0x000f1db0` | 49 | runtime_tail |
| `phys_fn_005562` | `0x000f1df0` | 605 | runtime_tail |
| `phys_fn_005564` | `0x000f2050` | 57 | runtime_tail |
| `phys_fn_005566` | `0x000f2090` | 199 | runtime_tail |
| `phys_fn_005568` | `0x000f2160` | 57 | runtime_tail |
| `phys_fn_005570` | `0x000f21a0` | 35 | runtime_tail |
| `phys_fn_005572` | `0x000f21d0` | 285 | runtime_tail |
| `phys_fn_005574` | `0x000f22f0` | 341 | runtime_tail |
| `phys_fn_005576` | `0x000f2450` | 17 | runtime_tail |
| `phys_fn_005578` | `0x000f2470` | 7 | runtime_tail |
| `phys_fn_005580` | `0x000f2480` | 171 | runtime_tail |
| `phys_fn_005582` | `0x000f2530` | 60 | runtime_tail |
| `phys_fn_005584` | `0x000f2570` | 10 | runtime_tail |
| `phys_fn_005586` | `0x000f2580` | 43 | runtime_tail |
| `phys_fn_005588` | `0x000f25b0` | 168 | runtime_tail |
| `phys_fn_005590` | `0x000f2660` | 36 | runtime_tail |
| `phys_fn_005592` | `0x000f2690` | 17 | runtime_tail |
| `phys_fn_005594` | `0x000f26b0` | 7 | runtime_tail |
| `phys_fn_005596` | `0x000f26c0` | 174 | runtime_tail |
| `phys_fn_005598` | `0x000f2770` | 73 | runtime_tail |
| `phys_fn_005600` | `0x000f27c0` | 1695 | runtime_tail |
| `phys_fn_005602` | `0x000f2e60` | 65 | runtime_tail |
| `phys_fn_005604` | `0x000f2eb0` | 10 | runtime_tail |
| `phys_fn_005606` | `0x000f2ec0` | 43 | runtime_tail |
| `phys_fn_005608` | `0x000f2ef0` | 168 | runtime_tail |
| `phys_fn_005610` | `0x000f2fa0` | 52 | runtime_tail |
| `phys_fn_005612` | `0x000f2fe0` | 17 | runtime_tail |
| `phys_fn_005614` | `0x000f3000` | 7 | runtime_tail |
| `phys_fn_005616` | `0x000f3010` | 1197 | runtime_tail |
| `phys_fn_005618` | `0x000f34c0` | 274 | runtime_tail |
| `phys_fn_005620` | `0x000f35e0` | 60 | runtime_tail |
| `phys_fn_005622` | `0x000f3620` | 10 | runtime_tail |
| `phys_fn_005624` | `0x000f3630` | 117 | runtime_tail |
| `phys_fn_005626` | `0x000f36b0` | 17 | runtime_tail |
| `phys_fn_005628` | `0x000f36d0` | 10 | runtime_tail |
| `phys_fn_005630` | `0x000f36e0` | 1177 | runtime_tail |
| `phys_fn_005632` | `0x000f3b80` | 26 | runtime_tail |
| `phys_fn_005634` | `0x000f3ba0` | 299 | runtime_tail |
| `phys_fn_005636` | `0x000f3cd0` | 5 | runtime_tail |
| `phys_fn_005638` | `0x000f3ce0` | 65 | runtime_tail |
| `phys_fn_005640` | `0x000f3d30` | 14 | runtime_tail |
| `phys_fn_005642` | `0x000f3d40` | 118 | runtime_tail |
| `phys_fn_005644` | `0x000f3dc0` | 167 | runtime_tail |
| `phys_fn_005646` | `0x000f3e70` | 116 | runtime_tail |
| `phys_fn_005648` | `0x000f3ef0` | 36 | runtime_tail |
| `phys_fn_005650` | `0x000f3f20` | 36 | runtime_tail |
| `phys_fn_005652` | `0x000f3f50` | 36 | runtime_tail |
| `phys_fn_005654` | `0x000f3f80` | 36 | runtime_tail |
| `phys_fn_005656` | `0x000f3fb0` | 73 | runtime_tail |
| `phys_fn_005658` | `0x000f4000` | 73 | runtime_tail |
| `phys_fn_005660` | `0x000f4050` | 73 | runtime_tail |
| `phys_fn_005662` | `0x000f40a0` | 73 | runtime_tail |

## 8. The 28 qhull rows the census places in Phases 2 and 5

Re-derived independently of Task 1 (literal pool plus span), and identical to it.

| row | rva | bytes | census phase | census provenance |
| --- | --- | ---: | ---: | --- |
| `phys_fn_002446` | `0x0005d670` | 770 | 2 | shared_by_callers |
| `phys_fn_002452` | `0x0005de50` | 348 | 2 | shared_by_callers |
| `phys_fn_002458` | `0x0005e470` | 28 | 5 | layout_adjacency |
| `phys_fn_002475` | `0x0005ed60` | 39 | 5 | layout_adjacency |
| `phys_fn_002497` | `0x0005f600` | 473 | 2 | shared_by_callers |
| `phys_fn_002499` | `0x0005f7e0` | 516 | 2 | shared_by_callers |
| `phys_fn_002507` | `0x0005fbb0` | 146 | 2 | shared_by_callers |
| `phys_fn_002546` | `0x000612a0` | 100 | 2 | shared_by_callers |
| `phys_fn_002684` | `0x00066270` | 74 | 5 | layout_adjacency |
| `phys_fn_002686` | `0x000662c0` | 32 | 5 | layout_adjacency |
| `phys_fn_002687` | `0x000662e0` | 39 | 5 | layout_adjacency |
| `phys_fn_002689` | `0x00066310` | 317 | 5 | layout_adjacency |
| `phys_fn_002691` | `0x00066450` | 389 | 5 | layout_adjacency |
| `phys_fn_002693` | `0x000665e0` | 90 | 5 | layout_adjacency |
| `phys_fn_002695` | `0x00066640` | 1233 | 5 | layout_adjacency |
| `phys_fn_002753` | `0x00068290` | 223 | 5 | layout_adjacency |
| `phys_fn_002755` | `0x00068370` | 156 | 5 | layout_adjacency |
| `phys_fn_002868` | `0x0006da40` | 13 | 5 | layout_adjacency |
| `phys_fn_002894` | `0x0006e620` | 33 | 5 | layout_adjacency |
| `phys_fn_002896` | `0x0006e650` | 23 | 5 | layout_adjacency |
| `phys_fn_003057` | `0x00074710` | 104 | 2 | shared_by_callers |
| `phys_fn_003296` | `0x0007f020` | 43 | 2 | shared_by_callers |
| `phys_fn_003308` | `0x0007f2a0` | 98 | 2 | shared_by_callers |
| `phys_fn_003324` | `0x0007f670` | 121 | 2 | shared_by_callers |
| `phys_fn_003334` | `0x0007fa60` | 70 | 2 | shared_by_callers |
| `phys_fn_003340` | `0x0007fba0` | 145 | 2 | shared_by_callers |
| `phys_fn_003346` | `0x0007fd20` | 128 | 2 | shared_by_callers |
| `phys_fn_003413` | `0x00084800` | 17 | 2 | shared_by_callers |
