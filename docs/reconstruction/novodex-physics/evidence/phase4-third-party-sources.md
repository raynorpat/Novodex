# Phase 4 evidence: do the vendored third-party sources correspond to the shipped bytes?

Companion to `phase4-third-party.md`, which identified the two libraries. This
document tests that identification against the *pinned source trees* and records
what a vendored tree would and would not account for. It corrects six claims
made in Task 1b, in both directions.

Oracle: `Binaries/NxPhysics.dll`, SHA-256
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`, 1,253,376
bytes, re-verified before any conclusion below. Image base `0x10000000`; `.text`
RVA `0x00001000`, virtual size 1,056,977 (`0x001020d1`). Every section has
`raw_offset == rva`, so an RVA below `0x00124000` is also its raw file offset.

## 0. Pinned inputs, verified

All four archive digests were recomputed before anything was read out of them
and all four match the pins in the brief:

| archive | SHA-256 | verdict |
| --- | --- | --- |
| `qhull-2003.1.tar.gz` | `c97d1982e6f5423379bf0ad6dd7293886ac69cae1596b59a305614aec4ae54f8` | matches |
| `qhull-2002.1.tar.gz` | `e05af7b9239ba214fe098a0e2b88bdaacdd37983471a6bc6075c2cbb6d7efb88` | matches |
| `ode-0.13.1.tar.gz` | `675b897736a1f3006be4b6c972e31eefdbed13a79368ddacab0af431b6ffa7e6` | matches |
| `Opcode13.zip` | `ecf649c786b4916e15cd613eef1ec5b727ee7cca11865fc9fae886151644fb91` | matches |

Extracted trees, as staged outside both repositories at
`.analysis/novodex-physics/thirdparty/`:

- `qhull-2003.1/` — `src/global.c:45` is `char *qh_version = "2003.1 2003/12/30";`,
  the exact literal at `.rdata:0x00109f90`. The negative control `qhull-2002.1/`
  carries a different string.
- `opcode13/Opcode/` — 102 files, no licence text of any kind.
- `opcode-ode-1.3.2/` — 100 files, including `COPYING` and `README-ODE.txt`.

---

## 1. The OPCODE point release: the framing in the brief is wrong, and the answer is sharper than "1.3.x"

### 1.1 The two pinned trees are the *same upstream release*

`ReadMe.txt` in the standalone tree and `ReadMe.txt` in ODE's bundled copy are
**byte-identical once line endings are normalised** — both hash to
`75fe8724b16d903272c7c249ce5bbad51f55d56fbdf3e385297efd1b300bf567` — and both
open with:

```
 OPCODE distribution 1.3 (june 2003)
```

Neither tree contains the string `1.3.1` or `1.3.2` anywhere (grep over both
trees, zero hits). ODE's own `README-ODE.txt` says only "This is a copy of the
OPCODE collision detection library by Pierre Terdiman … read the ReadMe.txt in
this directory", signed "Russ Smith, April 12 2005", and describes the copy as
*ported* so it compiles under gcc.

**So there is no "1.3 versus 1.3.2" to settle from these artefacts.** What is on
disk is upstream OPCODE 1.3 (June 2003) and a downstream fork of upstream
OPCODE 1.3 carrying ODE's patches. The `1.3.2` label is ODE's lineage, not a
codercorner release marker. Any conclusion phrased as "the image is 1.3.2" would
be unfalsifiable against these inputs.

### 1.2 What *can* be settled, and is: the image tracks the standalone tree, not ODE's

Three discriminators survive the absence of `HybridModel`, and all three land on
the standalone tree exactly.

**Discriminator A — `OPC_MeshInterface.cpp` line number, exact match.**

| side | measurement |
| --- | --- |
| image | `phys_fn_005358` at `0x000e9020` is `MeshInterface::SetPointers`. Its failure path at `0x000e903b` pushes `0xe6` = **230**, then `.rdata:0x0011ba74` = `\Epic\Novodex\SDKs\Physics\src\opcode\OPC_MeshInterface.cpp`, then `.rdata:0x0011ba48` = `MeshInterface::SetPointers: pointer is null` |
| standalone 1.3 | `opcode13/Opcode/OPC_MeshInterface.cpp:230` — `if(!tris \|\| !verts) return SetIceError("MeshInterface::SetPointers: pointer is null", null);` |
| ODE's copy | the same statement is at `opcode-ode-1.3.2/OPC_MeshInterface.cpp:**252**` |

230 is a zero-offset match against the standalone tree and a 22-line miss
against ODE's. NovodeX's `OPC_MeshInterface.cpp` is line-for-line identical to
standalone 1.3 for at least its first 230 lines.

**Discriminator B — `CheckTopology` is called; ODE deleted the call.**

ODE's only semantic edit to `OPC_Model.cpp` is at lines 148–149, where it
comments out both the `CheckTopology()` call and the degenerate-face `Log`:

```
opcode13/Opcode/OPC_Model.cpp:148 : 	udword NbDegenerate = create.mIMesh->CheckTopology();
opcode-ode-1.3.2/OPC_Model.cpp:148: 	//udword NbDegenerate = create.mIMesh->CheckTopology();
```

The image calls it. `Model::Build` at `0x000e9150` is
`mov ecx,[edi]` / `call 0x100e8fd0`, and `phys_fn_005357` at `0x000e8fd0` is
`MeshInterface::CheckTopology` beyond doubt: it loops `mNbTris` times over
`mTris` (`[ecx+8]`, stride 12 at `0x000e9012`), forms
`mVerts + mVRef[i]*12` three times (`lea edx,[edx+edx*2]` / `lea ebx,[esi+edx*4]`
at `0x000e8fe9`–`0x000e9000`), and increments a counter when any two of the
three vertex *pointers* coincide (`0x000e8ff8`–`0x000e9011`). That is
`OPC_MeshInterface.cpp:178-200` verbatim, in its non-callback non-stride form
(`OPC_USE_CALLBACKS` and `OPC_USE_STRIDE` both undefined).

**Discriminator C — `OPC_SweepAndPrune.cpp` is compiled in, and ODE deleted the
file.** `diff -rq` over the two trees reports `OPC_SweepAndPrune.cpp`,
`OPC_SweepAndPrune.h`, `OPC_BoxPruning.cpp` and `OPC_BoxPruning.h` as
`Only in opcode13/Opcode`. The image contains the first of those:

| image | source |
| --- | --- |
| `phys_fn_005289` `0x000e7180` (37 B) zeroes exactly eleven consecutive dwords: `+0x00`…`+0x1c`, then `+0x28`, `+0x24`, `+0x20` | `OPC_SweepAndPrune.h:65-84` — `SweepAndPrune { SAP_PairData mPairs; udword mNbObjects; SAP_Box* mBoxes; SAP_EndPoint* mList[3]; }` where `SAP_PairData` is six dwords (`OPC_SweepAndPrune.h:52-58`). Eleven dwords, exactly |
| `phys_fn_005271` `0x000e66f0` (89 B) loops `i` over `[ecx+0x10]`, indexes `[ecx+0x14] + i*4`, walks a list through `[esi+4]`, and calls back with `([esi], i, user_data)`; `ret 8` | `SAP_PairData::DumpPairs(PairCallback, void*)`, with `mNbObjects` at `+0x10` and `mArray` at `+0x14` — the fifth and sixth members |
| `phys_fn_005273` `0x000e6750` is five bytes: `jmp 0x000e66f0` | `OPC_SweepAndPrune.cpp:407-410` — `SweepAndPrune::GetPairs(PairCallback, void*) { mPairs.DumpPairs(callback, user_data); }` with `mPairs` at offset 0 |

This is the strongest of the three discriminators, because it is not a line
number that could drift under an unrelated edit — it is a whole translation unit
that does not exist in ODE's tree.

**Conclusion.** The correct statement of the OPCODE version is:

> **OPCODE 1.3 (June 2003), Pierre Terdiman's standalone distribution.** The
> image is not built from ODE's patched copy. Whether some intermediate
> codercorner point release exists that would also match cannot be decided from
> the pinned artefacts, because no such artefact is in hand — but every
> line-number and structural probe run against 1.3 matches 1.3, so 1.3 is the
> tree to vendor.

### 1.3 The only line-number discrepancy, and its size

NovodeX's `OPC_Model.cpp` is offset **+2 lines** from stock before line 145:

| side | measurement |
| --- | --- |
| image | `Model::Build` error path at `0x000e912f` pushes `0x93` = **147** with `.rdata:0x0011bb24` = `\Epic\Novodex\SDKs\Physics\src\opcode\OPC_Model.cpp` and `.rdata:0x0011bae4` = `OPCODE WARNING: supports complete trees only! Use mLimit = 1.\n` |
| stock 1.3 | that `return SetIceError(...)` is `OPC_Model.cpp:145` |

Two lines were inserted above line 145 in NovodeX's copy. `OPC_MeshInterface.cpp`
has no such offset (230 = 230), so the insertion is local to `OPC_Model.cpp`.

---

## 2. The claimed local modifications, re-checked against source

Task 1b listed eight divergences. Re-measured: **one is stock, two are wrong on
their key detail, one is understated, and the rest hold** — plus two the earlier
pass did not have, and one candidate raised in this pass that measurement
refuted.

### 2.1 REFUTED — the single-triangle short circuit is stock OPCODE 1.3

Task 1b: *"4. A single-triangle short circuit in `Model::Build` at
`0x000e917b`."* listed as a NovodeX modification.

It is upstream, and it is documented as such in the release notes NovodeX
received. `opcode13/Opcode/ReadMe.txt` line 24, in the "New in Opcode 1.3" list:

> `- it now works with meshes made of only 1 triangle (except in mesh-mesh case!)`

`opcode13/Opcode/OPC_BaseModel.h:47` defines `OPC_SINGLE_NODE = (1<<2)` with the
comment `//!< Special case for 1-node models`, at **the same line number in both
pinned trees**. `opcode13/Opcode/OPC_Model.cpp:157-165` is the short circuit:

```
	// Special case for 1-triangle meshes [Opcode 1.3]
	udword NbTris = create.mIMesh->GetNbTriangles();
	if(NbTris==1)
	{
		mModelCode |= OPC_SINGLE_NODE;
		return true;
	}
```

Compiling that file unmodified (MSVC 19.51.36252 x86, `/O2 /GR- /GS- /arch:IA32`)
emits, at offset `+0x40` of `Model::Build`:

```
  8b 07        mov  eax, DWORD PTR [edi]
  8b 18        mov  ebx, DWORD PTR [eax]
  83 fb 01     cmp  ebx, 1
  75 0f        jne  SHORT $LN6@Build
  83 4e 08 04  or   DWORD PTR [esi+8], 4
```

The image, at `0x000e9178`:

```
000e9178  8b07           mov eax, dword ptr [edi]
000e917a  53             push ebx
000e917b  8b18           mov ebx, dword ptr [eax]
000e917d  83fb01         cmp ebx, 1
000e9180  750f           jne 0x100e9191
000e9182  834e0804       or dword ptr [esi + 8], 4
```

Same instructions, same registers, same `75 0f` displacement, same `83 4e 08 04`.
**This row is not a modification and must not be reconstructed as one.**

### 2.2 CORRECTED — `OPCODECREATE` is 32 bytes because `BuildSettings` grew from 8 to 20, plus one added pointer

Task 1b said "extra fields at `+0x04`, `+0x10`, `+0x14`, `+0x18`", which reads
as four unrelated additions to `OPCODECREATE`. Three of the four are inside the
`BuildSettings` sub-object, and that matters because `BuildSettings` is embedded
in `AABBTreeBuilder` too — so getting it wrong breaks two classes, not one.

**Stock size, computed from source rather than assumed.**
`opcode13/Opcode/OPC_TreeBuilders.h:39-45` and `OPC_BaseModel.h:24-42` (members at lines 29-37):

| member | offset | size |
| --- | ---: | ---: |
| `const MeshInterface* mIMesh` | 0 | 4 |
| `BuildSettings mSettings` — `udword mLimit`, `udword mRules` | 4 | 8 |
| `bool mNoLeaf`, `mQuantized`, `mKeepOriginal`, `mCanRemap` | 12 | 4 |

**stock `sizeof(OPCODECREATE)` = 16**, `sizeof(BuildSettings)` = 8. Confirmed by
compiling: the stock `Model::Build` tests `cmp DWORD PTR [edi+4], 1` for
`mSettings.mLimit`.

**Image layout**, read out of three independent sites:

| offset | evidence | meaning |
| ---: | --- | --- |
| `+0x00` | `0x000e910b` `mov ecx,[edi]`; `0x000507ad`+`0x000507bd` writes `&IMesh` | `mIMesh` |
| `+0x04` | `0x000e9122` `mov eax,[edi+4]` guards the whole build; `0x000522cf` writes the caller's third stack argument here; ctor defaults it to 0 at `0x000e92d2` | **added** — deserialize source |
| `+0x08` | `0x000e9129` `cmp dword ptr [edi+8],1`; ctor writes 1 at `0x000e92cd` and again at `0x000e92dc`; call sites write 1 at `0x000507d4` and `0x000522ff` | `mSettings.mLimit` |
| `+0x0c` | ctor writes `0x7fffffff` at `0x000e92b2` then `0x22` at `0x000e92d5`; call sites write `0x22` at `0x000507df` and `0x00052307` | `mSettings.mRules` |
| `+0x10` | ctor defaults 0 at `0x000e92bb`; written at `0x00052319` only when `+0x14`'s source ≠ `0xff` | **added**, inside `BuildSettings` |
| `+0x14` | ctor defaults `0xffffffff` at `0x000e92be`; written at `0x00052311` from the argument tested `cmp eax,0xff` at `0x000522fa` | **added**, inside `BuildSettings` |
| `+0x18` | ctor defaults 0 at `0x000e92c5`; never written by either call site | **added**, inside `BuildSettings` |
| `+0x1c`…`+0x1f` | ctor writes bytes 1,1,0,0 at `0x000e92df`–`0x000e92e8`; call sites at `0x000507c4`/`0x000507cc`/`0x000507ea`/`0x000507f2` and `0x000522d3`/`0x000522e6`/`0x00052323`/`0x0005233b` | `mNoLeaf`, `mQuantized`, `mKeepOriginal`, `mCanRemap` |

**`sizeof(OPCODECREATE)` = 32.**

The three added dwords sit inside `BuildSettings`, not `OPCODECREATE`, and three
separate measurements say so:

1. **Constructor emission order.** `OPCODECREATE::OPCODECREATE` at `0x000e92b0`
   writes `+0x0c`, `+0x10`, `+0x14`, `+0x18`, `+0x08` *first* (the sub-object's
   default constructor, values `0x7fffffff`, `0`, `-1`, `0`, `1`), and only then
   the body's `+0x00 = 0`, `+0x04 = 0`, `+0x0c = 0x22`, `+0x08 = 1`, four bools.
   `OPCODECREATE`'s own members are assigned in the body; the five-dword block
   is initialised before them, which is where a member sub-object lands.
2. **The same five values appear in the builder.**
   `AABBTreeOfTrianglesBuilder`'s constructor `phys_fn_005360` at `0x000e9060`
   writes `+4 = 1`, `+8 = 0x7fffffff`, `+0xc = 0`, `+0x10 = 0xffffffff`,
   `+0x14 = 0` — the same five constants in the same order at the same relative
   offsets — and the builder contains no `OPCODECREATE`.
3. **`Model::Build` copies exactly those five dwords as a unit.**
   `0x000e91cc`–`0x000e91fd`: `lea ecx,[edi+8]` then five loads from `[ecx+0]`
   `[ecx+4]` `[ecx+8]` `[ecx+0xc]` `[ecx+0x10]` stored to `TB+4` `TB+8` `TB+0xc`
   `TB+0x10` `TB+0x14`, followed by `NbTris` to `TB+0x18`. A 20-byte struct
   assignment, then `mNbPrimitives`.

So `sizeof(BuildSettings)` = **20**, `sizeof(AABBTreeOfTrianglesBuilder)` = **72**
(`0x48`, the exact size of `Model::Build`'s local frame at `0x000e9100`;
`mIMesh` at `+0x44`, written at `0x000e91d1`), against stock's 8 and 32.

The `0xff` at `0x000522fa` is a *caller-side* sentinel on the value that lands at
`+0x14`; the struct's own default for that slot is `0xffffffff`. Both fields are
written together or not at all, from `InternalTriangleMesh.cpp` (`0x00052280`,
file literal at `.rdata:0x00108244`). Task 1b's reading of them as a height-field
axis and extent is consistent with the measurement but is not established by it;
what is established is that they are two co-written build parameters guarded by a
`0xff` sentinel.

### 2.3 HOLDS, and is larger than reported — the added virtual interface on `BaseModel`/`Model`

Stock `BaseModel` (`OPC_BaseModel.h:55,64,72,82`) declares four virtuals:
`~BaseModel`, `Build`, `GetUsedBytes`, `Refit`. Stock vtables therefore have
**four** slots.

The image's vtables have **seven**:

| vtable | slots | targets |
| --- | ---: | --- |
| `Model` `.rdata:0x0011bac8` | 7 (`0x0011bac8`+28 = `0x0011bae4`, where the `OPCODE WARNING` literal begins) | `0x000e9280`, `0x000e9100`, `0x000e90c0`, `0x000e9410`, `0x000e9420`, `0x000e9440`, `0x000e94c0` |
| `BaseModel` `.rdata:0x0011bb5c` | 7 | `0x000e95a0`, `_purecall` `0x000f41dc`, `_purecall`, `0x000e9410`, `0x000e9420`, `0x000e9440`, `0x000e94c0` |

**Three added virtual slots, 4/5/6.** Slots 5 and 6 are the serialization pair:

- **slot 5, `0x000e9440` (53 bytes) — the writer.** `push mModelCode` (`[esi+8]`),
  `call 0x000b3f00` on the argument (`0x000e944e`), then
  `if(mTree) mTree->vtable[5](stream)` (`call dword ptr [edx+0x14]` at
  `0x000e945f`), else `return (mModelCode>>2)&1` (`0x000e946a`, the
  `OPC_SINGLE_NODE` case).
- **slot 6, `0x000e94c0` (133 bytes) — the reader.** Releases `mSource`/`mTree`,
  reads a dword with `call 0x000b3ac0` (`0x000e9500`), stores it to `mModelCode`,
  returns true immediately if `OPC_SINGLE_NODE` (`test al,4` at `0x000e9505`),
  otherwise calls `CreateTree(no_leaf=(code>>1)&1, quantized=code&1)`
  (`0x000e9529` → `0x000e9310`) and dispatches `mTree->vtable[6](stream)`
  (`call dword ptr [edx+0x18]` at `0x000e953d`).

`Model::Build` reaches the reader directly: `0x000e9161` tests `create+0x4`, and
`0x000e916d` is `call dword ptr [edx+0x18]` — slot 6 — with that pointer as the
argument, returning immediately afterwards. So the blob-load path Task 1b
described is one arm of the same added interface, not a separate change.

Slot 4 is a third addition, `0x000e9420` (19 bytes), which tail-calls the tree's
own slot 4 (`jmp dword ptr [edx+0x10]`). Its guard reads
`mov eax,[ecx+0x10]; add eax,0x20; je` — bytes `8b 41 10 83 c0 20 74 08`,
re-read from the raw file at offset `0x000e9420` — which returns 0 only when
`mTree == (void*)-0x20`, not when `mTree` is null. I could not name this
function and am not guessing at it; it is recorded as added, unidentified.

**`AABBOptimizedTree` gained exactly the same three slots.** Stock declares five
virtuals (`OPC_OptimizedTree.h:140,149,158,168,171` — `~AABBOptimizedTree`,
`Build`, `Refit`, `Walk`, `GetUsedBytes`), so a stock vtable has five slots. The
image's abstract-base vtable at `.rdata:0x0011bc4c` has **eight**: slot 0 is a
real destructor at `0x000f21a0`, slots 1 through 7 are all `_purecall`
(`0x000f41dc`, which loads the handler at `.data:0x0012851c` and otherwise raises
`0x19`). The dispatch sites fix the mapping:

| tree slot | reached from | stock name |
| ---: | --- | --- |
| 1 | — | `Build` |
| 2 | `Model::Refit` `0x000e9410`, `call dword ptr [edx+8]` with `mIMesh` | `Refit` |
| 3 | — | `Walk` |
| 4 | `0x000e9420`, `jmp dword ptr [edx+0x10]` | **added** |
| 5 | `0x000e945f`, `call dword ptr [edx+0x14]` | **added** — writer |
| 6 | `0x000e953d` and `0x000e916d`, `call dword ptr [edx+0x18]` | **added** — reader |
| 7 | `Model::GetUsedBytes` `0x000e90c0`, `jmp dword ptr [eax+0x1c]` | `GetUsedBytes`, displaced from slot 4 to slot 7 |

So the modification is a matched **three-slot virtual interface added to both
class hierarchies at slots 4/5/6**, displacing `AABBOptimizedTree::GetUsedBytes`
from slot 4 to slot 7. A vendored stock header produces a five-slot tree vtable
and a four-slot model vtable; every indirect call in the shipped image would go
to the wrong function.

**This is a correction to the programme's record in the other direction**: the
writer is in the image, at `0x000e9440`, one vtable slot below the reader.

### 2.4 NEW — `OPC_RAYHIT_CALLBACK` was switched off, and stock defaults it on

`opcode13/Opcode/OPC_Settings.h:45` ships `#define OPC_RAYHIT_CALLBACK`
uncommented. NovodeX's build has it **undefined**. Two independent proofs:

1. `RayCollider::ValidateSettings` `phys_fn_004903` at `0x000b5770` (91 bytes) returns all
   five messages, including the two that `OPC_RayCollider.cpp:240-243` wraps in
   `#ifndef OPC_RAYHIT_CALLBACK`: `Closest hit doesn't work with First contact
   mode!` (`0x000b57ab`, `.rdata:0x0011b6b8`) and `Temporal coherence can't
   guarantee to report closest hit!` (`0x000b57b9`, `.rdata:0x0011b67c`).
2. Member offsets. `OPC_RayCollider.h:186-191` swaps `HitCallback mHitCallback;
   void* mUserData;` (8 bytes) for `CollisionFaces* mStabbedFaces;` (4 bytes)
   depending on the define, shifting everything after it. The image reads
   `mMaxDist` at `this+0x84` (`fld dword ptr [ecx+0x84]`, `0x000b5770`).
   Recompiling `OPC_RayCollider.cpp` with the define commented out puts
   `mMaxDist` at `+0x84` (`fcomp DWORD PTR [edx+132]`); with it defined it would
   be at `+0x88`.

A vendored stock `OPC_Settings.h` gets this wrong silently: the code compiles,
links, runs, and `RayCollider`'s layout is four bytes off from `mMaxDist`
onward.

The compiled-with-the-define-off listing also reproduces the image's idiom for
the last check almost exactly — image `0x000b57bf` `and al,0x10` / `neg al` /
`sbb eax,eax` / `and eax,OFFSET str`; compiled `and cl,16` / `movzx eax,cl` /
`neg eax` / `sbb eax,eax` / `and eax,OFFSET str`.

### 2.5 NEW — one added 4-byte member in `RayCollider`

With `OPC_RAYHIT_CALLBACK` off, the compiled layout puts `mClosestHit` at
`+0x88`, immediately after `mMaxDist` at `+0x84`. The image reads it at
`+0x8c` (`mov cl, byte ptr [ecx+0x8c]`, `0x000b579d`). Everything from the vptr
through `mMaxDist` matches stock exactly; one 4-byte member was inserted between
`mMaxDist` and `mClosestHit`.

### 2.6 HOLDS — `SetIceError` and `Log` were replaced, and `new` was rerouted

Stock `OPC_IceHook.h:28-29` defines `Log` as `{}` and `SetIceError` as `false`.
Under MSVC these are accepted (warning C4353, "constant 0 as function
expression") and evaluate to nothing, so **the stock tree does compile** — see
§5. NovodeX replaced `SetIceError` with a real reporter carrying `__FILE__` and
`__LINE__`:

`phys_fn_002160` at `0x000539b0` (31 bytes) takes `(message, file, line)`,
forwards to the global error-stream pointer at `.data:0x001041b4` as
`(2, file, line, 0, message)`, and returns false (`xor al,al`, `0x000539cc`).
The same indirect target is called directly from NovodeX code with severity 4
(e.g. `0x00052381`, `0x00050831`).

Allocation was rerouted the same way qhull's was. Where stock emits
`push 48; call operator new`, the image at `0x000e9191` calls
`phys_fn_004803` `0x000b4000` (an allocator singleton getter) and then
`call dword ptr [edx]` with `(0x30, 0)`; frees go through
`call dword ptr [edx+0xc]` (`0x000e9258`, `0x000e949c`, `0x000e94dc`).
**`sizeof(AABBTree)` is `0x30` = 48 in the image and 48 in the compiled stock
tree** — an independent confirmation that the class is unmodified.

### 2.7 HOLDS — `IcePrunable.cpp` and `IceAdjacencies.cpp` are in neither tree

`grep -rn "Prunable\|Adjacencies"` over `opcode13/` and `opcode-ode-1.3.2/`
returns zero hits. Neither file ships in either pinned OPCODE tree, so vendoring
cannot supply them and those rows must be reconstructed.

**`IcePrunable.cpp`** — `\Epic\Novodex\SDKs\Physics\src\opcode\IcePrunable.cpp`
at `.rdata:0x0011b5d4`, with `Invalid pruning type` (`.rdata:0x0011b5bc`, line
`0x98` = 152) and `Invalid pruning section` (`.rdata:0x0011b60c`, line `0xae` =
174). Its vtable is `.rdata:0x0011b5a4`, six slots:
`0x000b56d0` (deleting destructor), `0x000b54f0`, `0x000b5520`, `0x000b5550`,
`0x000b5570` (flag set/clear/toggle/set-or-clear), and a folded stub at
`0x0000dee0`. The class carries a vptr, an owner at `+4`, flags at `+8`, a
pruner back-pointer at `+0x20`, a `udword` handle at `+0x24`, a `uword` handle
at `+0x28` with `0xffff` as the invalid marker, and two range-checked bytes at
`+0x2a`/`+0x2b`. `0x000b5590` returns `pruner->array[+0x14] + handle*24` — 24
bytes being one `AABB`. Four nine-slot pruner vtables (`0x0011b9c8`,
`0x0011b9f0`, `0x0011bb98`, `0x0011bbc0`) plus a base at `0x0011bc18` call into
it. **No OPCODE 1.3 class declares nine virtuals.** This is NovodeX's own
scene-query base, filed under `src\opcode\` by convention only.

**`IceAdjacencies.cpp` — Task 1b's address is wrong.** `.rdata:0x00108528`
holds `TriangleMesh: Mesh has a negative volume! Is it open or do (some) faces
have reversed winding? (Taking absolute value.)`. The `IceAdjacencies` literal is
at **`.rdata:0x001077cc`**, and its path is
`\Epic\Novodex\SDKs\Physics\src\IceAdjacencies.cpp` — NovodeX's **main `src\`**,
not `src\opcode\`. Its two asserts are
`Adjacencies::UpdateLink: invalid edge reference in first triangle`
(`.rdata:0x00107788`) and `… in second triangle` (`.rdata:0x00107740`). The class
name and message text are Pierre Terdiman's ICE adjacency builder, so this reads
as ICE code licensed separately from OPCODE rather than as NovodeX's own; the
code at `0x0002dbc0`–`0x0002dcf0` was not disassembled in this pass and that
characterisation rests on the strings alone.

### 2.8 HOLDS, and the qhull allocator is a named class with a nine-slot vtable

Task 1b established that qhull's allocations are routed through a global at
`.data:0x00125080` pointing at a 16,488-byte stack object. The object's class is
now pinned:

| measurement | address |
| --- | --- |
| the constructor writes `vptr = 0x10113614` | `0x0007e3b4` |
| it stores its one argument at `this+0x4048` and zeroes `this+0x08`, `+0x0c`, `+0x24`…`+0x30`, `+0x4034`…`+0x4044`, `+0x404c`, `+0x4050` | `0x0007e37a`–`0x0007e3c3` |
| it `rep stosd`-zeroes `0x1000` dwords — a **16,384-byte inline arena** — starting at `this+0x34` | `0x0007e3ac`–`0x0007e3ba` |
| the vtable at `.rdata:0x00113614` has **nine slots**, ending immediately before the pooled float `0.25f` at `.rdata:0x00113638` | slots: `0x0007e3d0`, `0x0007e4b0`, `0x0007e560`, `0x0007e520`, `0x0007e4e0`, `0x0007e810`, `0x0007e920`, `0x0002ea70`, `0x0007e540` |
| slot 5 (`+0x14`) is the allocator qhull calls | `0x0007e810` (222 bytes), reached from `0x0007ea78` |
| slot 8 (`+0x20`) is the free path | `0x0007e540` (32 bytes), reached from `phys_fn_003413` `0x00084800` |
| slot 7 (`+0x1c`) is a one-byte `ret` at `0x0002ea70` — a folded no-op far outside the qhull span | — |

Eight of the nine slots, the constructor at `0x0007e370`, the driver at
`0x0007ea10`, and the non-virtual method at `0x0007d5b0` (called with the arena
as `this` and eleven pushed arguments from `0x0007ead0`) are NovodeX code sitting
inside the qhull address span. None of it exists upstream.

The driver's argument vector is `argv[0] = "qhull"` (`.rdata:0x0010a040`, written
at `0x0007ea35`) and `argv[1] = "o"` (`.rdata:0x0011363c`, a single byte `0x6f`,
written at `0x0007ea3c`) — a **two-element `argv`, which is `qh_init_A`'s
interface, not `qh_new_qhull`'s**. `qhull-2003.1/src/user.c:117-118` declares
`int qh_new_qhull (int dim, int numpoints, coordT *points, boolT ismalloc, char *qhull_cmd, FILE *outfile, FILE *errfile)`
— a single flat command string, not an argv array. `qhull-2003.1/src/global.c:397`
declares `void qh_init_A (FILE *infile, FILE *outfile, FILE *errfile, int argc, char *argv[])`,
which is what `qconvex.c`'s `main()` uses.

`0x0007ea10` only *builds* the vector. The driver proper is **`phys_fn_003236`
at `0x0007d420`** (214 bytes), which opens:

```
0007d42e  push 0x10122640            ; errfile
0007d433  push 0x10122620            ; outfile
0007d438  push 0x10122600            ; infile
0007d43d  call 0x100626c0            ; qh_init_A(infile,outfile,errfile,argc,argv)
0007d442  push 0x101248e0            ; qh qhull_command
0007d447  call 0x100626f0            ; qh_initflags(qh qhull_command)
...
0007d458  lea edi,[edi+edi*2]        ; numpoints*3
0007d45e  lea eax,[edi*8]            ; *8  -> realT is double, dim is hard-coded 3
0007d466  call dword ptr [edx+0x14]  ; arena malloc
```

That is `unix.c`/`qconvex.c`'s `main()` shape, not `qh_new_qhull`'s.
**`qh_new_qhull` is absent from the image entirely** — all three of its own
literals, including the `"qhull "` prefix its `strncmp` guard needs
(`user.c:128`), are missing. Vendoring `user.c` supplies nothing for either
function.

### 2.9 CORRECTED — `0x00084800` is `qh_errexit`, not the free path

Task 1b: *"`phys_fn_003413` at `0x00084800`, 17 bytes, 76 callers:
… free, also a virtual call"*.

The seventeen bytes are:

```
00084800  8b0d80501210   mov ecx, dword ptr [0x10125080]
00084806  8b542404       mov edx, dword ptr [esp+4]
0008480a  8b01           mov eax, dword ptr [ecx]
0008480c  52             push edx
0008480d  ff5020         call dword ptr [eax+0x20]
00084810  c3             ret
```

`ret` with no stack adjustment — cdecl, caller cleans — and it forwards only its
**first** argument. Its callers pass three. `qh_memalloc` at `0x0006da50` reaches
it as `push edi; push edi; push 4`, i.e. `f(4, NULL, NULL)` where 4 is
`qhmem_ERRmem`. That is **`qh_errexit(int exitcode, facetT *facet, ridgeT *ridge)`**,
`qhull-2003.1/src/user.c:189`, rewritten to hand the exit code to the host. It is
called from 69 distinct qhull functions.

The real free path is **`qh_memfree` at `0x0006dc10`**, through slot `+0x18`.

### 2.10 CORRECTED — the `0x00125080` object is an I/O shim first and an allocator second

Task 1b's count is exact — **175 of the 492 qhull rows reference
`.data:0x00125080`**, all of them inside the span, and `0x0007ea51` is the only
write to that global in the whole image. But a census of the virtual calls made
through it changes what the object *is*:

| slot | call sites | rows | what it does |
| --- | ---: | ---: | --- |
| `+0x10` | **561** | 161 | `fprintf` — qhull's entire `qh ferr`/`qh fout` diagnostic stream |
| `+0x14` | 4 | 3 | `malloc` |
| `+0x18` | 3 | 3 | `free` |
| `+0x20` | 1 | 1 | error exit (`qh_errexit`, §2.9) |
| `+0x04` | 6 | 5 | emits three floats — a geometry-dump hook |
| `+0x08`, `+0x0c`, `+0x1c` | 1 each | | — |

So the 175 rows are overwhelmingly rows that *print*, not rows that allocate.
That matters for reconstruction: the qhull diagnostic machinery survived into the
shipped Release build and was rewired wholesale, and a vendored `user.h` that
only redefines `qh_malloc`/`qh_free` reproduces none of it.

### 2.11 CHECKED AND REFUTED — `qh_MAXnarrow` was *not* changed

A candidate divergence surfaced during mapping: `qh_initialhull` compares against
the double `-0.999999999999999` at `.rdata:0x00111a68`, where
`qhull-2003.1/src/user.h:726` defines `qh_MAXnarrow` as `-0.99999999`. Measured,
this is not a modification. **Both** stock constants are present at their stock
values and both are referenced from the same row:

| constant | source | `.rdata` | referenced at |
| --- | --- | --- | --- |
| `qh_MAXnarrow` = `-0.99999999` | `user.h:726` | `0x00111a80` | `fcomp qword ptr [0x10111a80]` at `0x000795ed` |
| `qh_WARNnarrow` = `-0.999999999999999` | `user.h:738` | `0x00111a68` | `fcomp qword ptr [0x10111a68]` at `0x00079630` |

A whole-file byte search finds exactly one occurrence of each. Both references
are inside row `0x000793f0`, in that order, which is exactly
`poly2.c:1790` (`minangle < qh_MAXnarrow`) followed by
`poly2.c:1795` (`minangle < qh_WARNnarrow`). `user.h`'s narrow-angle constants
are unmodified; the candidate was a conflation of the two.

---

## 3. The attribution hazards: one confirmed, one re-explained

### 3.1 COMDAT placement — the mechanism is not what Task 1b said, and the consequence is worse

Task 1b: *"Two functions at `0x000538b0` and `0x000538a0`, in `TriangleMesh.cpp`'s
address range, occupy slots 0 and 4 of OPCODE's `AABBTreeOfTrianglesBuilder`
vtable at `.rdata:0x0011bab4`. A folded address is evidence about neither owner."*

The vtable and the slots are exactly as reported. The reading of them is not.
Both functions **are OPCODE code**, emitted from OPCODE headers as COMDATs that
the linker happened to place inside the address band the census gives to
`TriangleMesh.cpp`:

| slot | target | what it is |
| ---: | --- | --- |
| 0 | `0x000538b0` (35 B) | scalar deleting destructor. Sets the vptr to `0x0010833c`, then `if(flags&1)` frees through the NovodeX allocator. `0x0010833c` is the **abstract base** `AABBTreeBuilder`'s vtable — `[0]=0x000538b0`, `[1]=_purecall`, `[2]=0x00053880`, `[3]=_purecall`, `[4]=0x000538a0`. `~AABBTreeOfTrianglesBuilder` and `~AABBTreeBuilder` are byte-identical once the derived dtor's dead vptr store is elided, so `/OPT:ICF` folded them |
| 4 | `0x000538a0` (14 B) | `AABBTreeBuilder::ValidateSubdivision`, `OPC_TreeBuilders.h:105-111`. `mov eax,[ecx+4]; cmp eax,[esp+8]; sbb eax,eax; neg eax; ret 0xc` — `return nb_prims > mSettings.mLimit`, with `mLimit` at `this+4` (agreeing with §2.2) and three stack arguments matching `(const udword*, udword, const AABB&)` |

A third member of the same set: `0x00053880` (24 B) is
`AABBTreeBuilder::GetSplittingValue(const udword*, udword, const AABB&, udword)`,
`OPC_TreeBuilders.h:90-94` — `ret 0x10`, body
`fld [box+axis*4+0xc]; fadd [box+axis*4]; fmul 0.5` = `global_box.GetCenter(axis)`
on a min/max `AABB`.

The `0x0010833c` vtable is referenced from exactly one place in the whole file
(raw offset `0x000538ba`, the store inside the folded destructor), which is why
no constructor appears to install it: the class is abstract.

**Why this is worse than the original claim.** It is not that a folded address
is uninformative. It is that **address-range attribution is unsound for any
class whose members are defined in headers**: OPCODE is an `inline_`-heavy
library, MSVC emits header-defined and implicit members as COMDATs, and COMDAT
ordering ignores source grouping. At least three OPCODE functions live 600 KB
away from the OPCODE span. Any census that assigns ownership by address will
misfile them, in both directions.

### 3.2 String pooling — all seven verified, and the hazard runs the other way

Each of the seven addresses holds the claimed value, and each is referenced from
outside the qhull span. Referencing rows, deduplicated:

| address | value | referencing rows inside `0x0005c5c0`–`0x00084a50` | referencing rows outside |
| --- | --- | --- | --- |
| `0x00109ae8` | double 3.0 | `0x00060be0` (`qh_detroundoff`) | `0x000886b0` |
| `0x0010a3d0` | `"w"` | `0x00064560` | `0x0009cba0` |
| `0x0010c1f4` | `"%d "` | `0x00067930`, `0x00067ca0`, `0x00067e00`, `0x00067ed0`, `0x0007df20` | `0x00092b50` |
| `0x0010c2e4` | `"%d %d %d "` | `0x00068290` | `0x00092b50` |
| `0x001135bc` | `"\r\n"` | `0x0007df20` | `0x00090fb0`, `0x00092b50`, `0x00094ac0`, `0x000950f0` |
| `0x00113638` | float 0.25 | **none** | `0x000886b0`, `0x000e76c0` |
| `0x0010d744` | `"\n\n"` | `0x0006df50`, `0x0007aef0`, `0x00084750` | `0x000fe275`, `0x000fe5e6` (CRT) |

**No row is wrongly attributed to qhull by any of them**, because the span is
address-delimited and no outside referencing row falls inside it. `0x00113638`
is referenced only from outside — it is a pool-range artefact with no qhull use
at all.

The real hazard points the other way. `0x0007df20` sits **inside** the span and
references two of these pooled literals, but it is a NovodeX OBJ-file writer: its
caller `0x0007dea0` pushes `QHULL_OK_%04d.obj` (`.rdata:0x001135ec`) and
`0x0007e050` pushes `QHULL_FAIL_%04d.obj` (`.rdata:0x00113600`). Neither string
exists in qhull. Pooling did not drag a foreign row into the span; the span
already contains foreign rows (§4).

---

## 4. The correspondence map

### 4.1 The criterion, stated and defended

Byte identity is not the test and cannot be: the shipped code was produced by a
2003-era MSVC with unknown flags, and no modern compiler reproduces it. The test
used here is **structural correspondence**, graded:

- **`mapped`** — two or more *independent* signals agree and none contradicts.
  Admissible signals: (1) a string literal the row references occurs in exactly
  one source function; (2) call-graph position — the row's callees and callers
  map to that function's callees and callers; (3) a distinctive numeric constant;
  (4) control-flow shape — loop nesting, return count, switch tables;
  (5) member-offset pattern against the class layout the headers imply;
  (6) vtable-slot identity.
- **`probable`** — one strong signal, nothing contradicting.
- **`unmapped`** — no upstream counterpart.

### 4.2 Result

| library | rows | `mapped` | `probable` | `unmapped` |
| --- | ---: | ---: | ---: | ---: |
| qhull | 492 | 454 (148,128 B) | 1 (74 B) | 37 (12,166 B) |
| OPCODE incl. `IcePrunable` head | 407 | 227 (195,466 B) | 32 (33,631 B) | 148 (25,058 B) |
| **total** | **899** | **681** | **33** | **185** |

**714 of 899 rows (79.4%) and 377,299 of 414,523 bytes (91.0%) have an upstream
source function.** The 185 `unmapped` rows are not mapping failures: they are
NovodeX code inside the two library address spans.

### 4.3 qhull — translation-unit tiling

Link order is strictly alphabetical; the twelve library translation units plus
two NovodeX blocks tile the span exactly to 160,368 bytes.

| TU | span | rows | functions found / in source |
| --- | --- | ---: | --- |
| `geom.c` | `0x0005c5c0`–`0x0005ea51` | 19 | 16/16 |
| `geom2.c` | `0x0005ea60`–`0x00061d8d` | 50 | 40/44 |
| `global.c` | `0x00061d90`–`0x0006626c` | 46 | 13/20 |
| `io.c` | `0x00066270`–`0x0006da35` | 91 | 62/71 |
| `mem.c` | `0x0006da40`–`0x0006e0a8` | 8 | 8/9 |
| `merge.c` | `0x0006e0b0`–`0x00073d1d` | 77 | 55/57 |
| `poly.c` | `0x00073d20`–`0x00075858` | 27 | 21/21 |
| `poly2.c` | `0x00075860`–`0x0007a829` | 60 | 46/54 |
| `qhull.c` | `0x0007a830`–`0x0007d410` | 18 | 14/14 |
| **NovodeX block A** | `0x0007d420`–`0x0007ed42` | **24** | — |
| `qset.c` | `0x0007ed50`–`0x0007fd9f` | 34 | 34/37 |
| **NovodeX block B** | `0x0007fda0`–`0x000814ef` | **13** | — |
| `stat.c` | `0x000814f0`–`0x000847ff` | 22 | 19/21 |
| `user.c` | `0x00084800`–`0x00084a43` | 3 | 3/5 |

Block A contains the driver (`0x0007d420`), the arena class (§2.8) and the OBJ
writer (§3.2). Block B is ten functions with zero qhull calls, zero qhull global
references, reachable only from `0x0007d5b0`, and heavily x87 (203 float ops in
`0x00080e90`); it was not analysed beyond "not qhull".

**Reverse list — 85 source functions with no image counterpart.** 30 belong to
the absent main programs (`qconvex`, `qdelaun`, `qhalf`, `qvoronoi`, `unix`,
`rbox`, `user_eg`, `user_eg2` — none of their
`"qhull internal warning (main): …"` literals is in the image); 17 are `#if`
alternates not selected; **38 are genuine library absences**, each with a cause:
`/OPT:REF` elimination, inlining (`qh_appendprint` into `qh_initflags`,
`qh_settempfree_all` into `qh_freebuild`), `/OPT:ICF` folding
(`qh_comparevisit` into `qh_comparemerge` `0x0006e650`, address-taken from
exactly the three source `qsort` sites), `#if 0` in source (`qh_stddev`,
`qh_eachvoronoi_all`), or removal by the `qh_errexit` rewrite
(`qh_printhelp_degenerate`, `qh_printhelp_singular`). One is present but
*outside* the span: `qh_user_memsizes` (`user.c:320`) is the bare `ret` at
`0x0002ea70`, tail-called from `qh_initqhull_mem` `0x00062390`.

**Build-time `#define`s, read out of the code** — `qh_QHpointer` = 0,
`qh_KEEPstatistics` **set**, `qh_NOmerge` / `qh_NOmem` / `qh_NOtrace` **not**
set, `realT` = `double`; from `qh_initqhull_mem` `0x00062320`, `qh_MEMalign` = 8,
`qh_MEMbufsize` = `0x10000`, `qh_MEMinitbuf` = `0x20000`, 18 size classes;
`qh_RANDOMmax` = 2147483646. All stock. Linker: `/Gy` + `/OPT:REF`, `/OPT:ICF`,
`/GF`.

### 4.4 OPCODE — translation units present and absent

**Present**, each anchored on a vtable RVA written by a located constructor or
destructor: `OPC_RayCollider` (`0x0011b628`), `OPC_TreeCollider` (`0x0011b750`),
`OPC_LSSCollider` (`0x0011b764`), `OPC_OBBCollider` (`0x0011b774`),
`OPC_SphereCollider` (`0x0011b784`), `OPC_PlanesCollider` (`0x0011b794`),
`OPC_AABBCollider` (`0x0011bb84`), `OPC_Collider` (`0x0011bbec`),
`OPC_VolumeCollider` (`0x0011bc00`), `OPC_OptimizedTree` (base `0x0011bc4c` plus
four subclass tables), `OPC_AABBTree`, `OPC_TreeBuilders` (`0x0011bab4` and
`0x00108350`), `OPC_MeshInterface`, `OPC_Model`, `OPC_BaseModel`,
**`OPC_SweepAndPrune`** (§1.2), and `Ice/IceRevisitedRadix`, `IceMatrix4x4`,
`IceAABB`, `IceTriangle`, `IceIndexedTriangle`, `IcePlane`, `IcePoint`,
`IceContainer`.

**Absent:** `OPC_HybridModel.cpp` (no `HybridModel` vtable anywhere in the image;
every `Hybrid*Collider` likewise missing), `OPC_BoxPruning.cpp` (no
`BipartiteBoxPruning`/`CompleteBoxPruning`, no `PRUNING_SORTER` static-init
guards), `OPC_Picking.cpp` (none of the five entry points),
`Opcode.cpp` (no `InitOpcode`/`CloseOpcode`/`ModuleAttach`), and
`Ice/IceOBB`, `IceHPoint`, `IceMatrix3x3`, `IceRandom`, `IceRay`, `IceUtils`,
`IceSegment` (no surviving out-of-line members). `OPC_Common.cpp` and
`StdAfx.cpp` define no functions.

### 4.5 Where the map is weaker than the criterion

- **147 of the 492 qhull rows are continuation blocks**, not function entries;
  they inherit their parent's grade rather than carrying their own evidence.
  Rebuilding boundaries from call targets and intra-function jumps gives **363
  functions** behind the 492 rows — `qh_initflags` alone spans 26 rows,
  `0x000626f0`–`0x00064df6`. Only the 345 entry rows are independently evidenced.
- **One qhull signal is a prior, not a check.** "This row falls in translation
  unit band X" is derived from the alphabetical link order. Rows graded `mapped`
  on "string + TU band" carry closer to 1.5 independent signals than 2.
- **32 OPCODE rows are `probable` on one signal** — mostly outlined continuation
  blocks with a single caller, plus Ice leaf functions matched on control-flow
  shape alone.
- **Four OPCODE rows are a suspicion, not a result.** `0x000ca5a0`, `0x000cbe50`,
  `0x000cd700`, `0x000d12b0` are size- and shape-twins of the `AABBNoLeafNode`
  tree-tree collide family at `0x000bc770`/`0x000be230`/`0x000bfd10`/`0x000d0960`
  but never call the mesh accessor `0x000b4de0`. Recorded as a suspected
  direct-pointer specialisation; not proved.
- **`0x0002dbc0`–`0x0002dcf0` (`IceAdjacencies`) was not disassembled.**
- **No byte-identity test was attempted anywhere.** Every match is structural.

### 4.6 The x87 control word, recomputed

Task 1b reported 8 OPCODE rows in the direct-call closure from `Scene::simulate`
`0x00013c40`. Recomputed from the whole-image call graph, that reproduces
exactly — **178 rows in the closure, 59 of which make indirect calls, 8 of them
in the OPCODE span** — and the three rows Task 1b named are among them:

| RVA | source function |
| --- | --- |
| `0x000b55b0` | `Prunable::UpdateWorldAABB` — `IcePrunable.cpp` (NovodeX) |
| `0x000b5670` | `Prunable::GetUpdatedWorldAABB` — same |
| `0x000e32c0` | `RadixSort::RadixSort` — `Ice/IceRevisitedRadix.cpp:170` |
| `0x000e3330` | `RadixSort::Resize` — `Ice/IceRevisitedRadix.cpp:204` |
| `0x000e3920` | `RadixSort::Sort(const float*, udword)` — `Ice/IceRevisitedRadix.cpp:350` |
| `0x000e6750` | `SweepAndPrune::GetPairs(PairCallback, void*)` — `OPC_SweepAndPrune.cpp:407` |
| `0x000e6ca0` | NovodeX broad-phase wrapper over `SweepAndPrune::Init` `0x000e6db0` |
| `0x000e7180` | `SweepAndPrune::SweepAndPrune` — `OPC_SweepAndPrune.cpp:389` |

**But 8 is an undercount of the method's own intent.** `0x000e6750` is a
five-byte `jmp`, i.e. the closure already depends on tail calls being followed at
least once. Counting `jmp` edges as well as `call` edges — they are the same
control transfer — the closure is **244 rows and 21 OPCODE rows**. Both figures
are lower bounds, because 59 rows in the closure make indirect calls.

`Model::Build` `0x000e9100`, `OPCODECREATE::OPCODECREATE` `0x000e92b0` and the
mesh-load roots `0x00050640`/`0x00052280` are all confirmed **outside** the
closure, so the tree-build path runs under `0x027f` and the sweep-and-prune
broad phase plus the radix sort it drives run under `0x0f7f`.

**qhull is absent from the closure under both methods** — zero rows either way.
That is a measured absence from a lower bound, not a proof.

### 4.7 Two count corrections to Task 1b

- **The "150 OPCODE rows past `0x000e9280`" is 148.** Non-padding rows with
  `rva > 0x000e9280` number 148; including `0x000e9280` itself, 149 — and **all
  149** carry `phase_provenance: runtime_tail`, so there is no "149 of 150"
  split at that boundary. The span does hold exactly 150 phase-8 rows, but the
  150th is `0x000e7c40` (5 bytes, `runtime_artifact`), *below* the boundary.
- **Not all of those rows are OPCODE.** Roughly 60 of the 149 are NovodeX
  pruner/container/stream code (`0x000ef270`–`0x000f0510` and
  `0x000f1550`–`0x000f21a0`) and 23 are the added serialization virtuals.

## 5. The compile experiment

**Toolchain.** `cl.exe` 19.51.36252 for x86 (MSVC 14.51, Visual Studio 18
Professional), invoked through `vcvarsall.bat x64_x86`.
Flags: `/c /O2 /GR- /GS- /arch:IA32 /I. /DOPCODE_EXPORTS
/D_CRT_SECURE_NO_WARNINGS /FAsc`, and separately with `/EHsc` added.
The results below are evidence about **structure**, not about bytes; a 2003-era
MSVC with unknown flags produced the image and no modern compiler reproduces it.

**Result 1 — stock OPCODE 1.3 compiles unmodified.** All 20 `OPC_*.cpp`, plus
`Opcode.cpp`, plus all 15 `Ice/Ice*.cpp` (the last needing only `/I.` because
they `#include "Stdafx.h"` from the parent directory) produced 36 objects. The
only diagnostic is C4353 on the `Log`/`SetIceError` no-op macros. **No patching
is required to build the pinned tree**, which removes the largest practical
risk from vendoring.

**Result 2 — stock qhull 2003.1 compiles unmodified.** `geom.c geom2.c global.c
io.c mem.c merge.c poly.c poly2.c qhull.c qset.c stat.c user.c` all compile
clean under `/O2 /GS- /arch:IA32`.

**Result 3 — `Model::Build` is instruction-for-instruction stock in its stock
parts.** With `/EHsc` omitted (the image has no SEH frame in this function, so
NovodeX built without C++ exception handling), the compiled prologue and first
20 instructions are:

```
compiled                          image (0x000e9100)
83 ec 20  sub esp,32              83 ec 48  sub esp,0x48      (bigger builder)
53 56 57  push ebx,esi,edi        56 57     push esi,edi
8b 7c 24 30  mov edi,create       8b 7c 24 54  mov edi,[esp+0x54]
8b f1     mov esi,ecx             8b f1     mov esi,ecx
8b 0f     mov ecx,[edi]           8b 0f     mov ecx,[edi]
85 c9     test ecx,ecx            85 c9     test ecx,ecx
0f 84 ..  je                      0f 84 ..  je
e8 ..     call IsValid            e8 ..     call 0x000e8fb0
84 c0     test al,al              84 c0     test al,al
0f 84 ..  je                      0f 84 ..  je
83 7f 04 01  cmp [edi+4],1        —— NovodeX: test [edi+4]; cmp [edi+8],1 ——
8b 0f     mov ecx,[edi]           8b 0f     mov ecx,[edi]      (0x000e914e)
e8 ..     call CheckTopology      e8 ..     call 0x000e8fd0
8b ce     mov ecx,esi             8b ce     mov ecx,esi
e8 ..     call ReleaseBase        e8 ..     call 0x000e9480
8b 07     mov eax,[edi]           8b 07     mov eax,[edi]
89 46 04  mov [esi+4],eax         89 46 04  mov [esi+4],eax
8b 07     mov eax,[edi]           —— NovodeX: deserialize branch ——
8b 18     mov ebx,[eax]           8b 18     mov ebx,[eax]      (0x000e917b)
83 fb 01  cmp ebx,1               83 fb 01  cmp ebx,1
75 0f     jne                     75 0f     jne
83 4e 08 04  or [esi+8],4         83 4e 08 04  or [esi+8],4
...
6a 30     push 48 (new AABBTree)  6a 30     push 0x30 (allocator)
e8 ..     call AABBTree::AABBTree e8 ..     call 0x000f1060
89 46 0c  mov [esi+12],eax        89 46 0c  mov [esi+0xc],eax
```

Identical instruction selection *and identical register allocation*
(`edi`=create, `esi`=this, `ebx`=NbTris) across a 22-year compiler gap, with
`ReleaseBase` inlined out of `Model::Release()` in both. This is what
"structural correspondence" should mean, and it sets the bar for the rest of the
map.

Note the two things the compile also settles about the shipped build: **no RTTI**
(already known from zero `.?AV` strings), and **no C++ exception handling** — the
`/EHsc` build wraps `Model::Build` in an SEH frame (`push -1`, `__ehhandler$`,
`mov fs:0,esp`) that the image does not have.

**Confirmed `#define` set for OPCODE**, from the image:

| define | stock 1.3 default | NovodeX | evidence |
| --- | --- | --- | --- |
| `OPC_USE_CALLBACKS` | off | off | `SetPointers` exists at `0x000e9020` |
| `OPC_USE_STRIDE` | off | off | `CheckTopology` `0x000e8fe9` uses fixed stride 12 |
| `OPC_USE_FCOMI` | on | on | 294 `fcomi` + 294 `fcmovb`/`fcmovnb` in the span, first at `0x000bc7d2` |
| `OPC_RAYHIT_CALLBACK` | **on** | **off** | §2.4 |
| `__MESHMERIZER_H__` | off | off | `Model::Build` has no hull branch; `Model` has no `mHull` |
| `qh_QHpointer` (qhull) | 0 | 0 | Task 1b §1.3, `0x0005c5c0` |

---

## 6. Licence obligations

### 6.1 qhull — Geometry Center licence, five clauses, verbatim

`qhull-2003.1/COPYING.txt` travels with the vendored tree unmodified; see §6.3. Clauses 1–5 are
binding on the vendored tree. Two require text this project must write:

- **Clause 3.** *"If you modify Qhull, you must include a notice giving the name
  of the person performing the modification, the date of modification, and the
  reason for such modification."*
- **Clause 4.** *"When distributing modified versions of Qhull, or other software
  products that include Qhull, you must provide notice that the original source
  code may be obtained as noted above"* — "as noted above" being
  *"Qhull is free software and may be obtained via http from www.qhull.org."*

Clause 1 forbids stripping the per-file copyright headers; clause 2 requires
`COPYING.txt` to travel with any copy, including copies inside a larger product.

### 6.2 OPCODE

- **`opcode13/Opcode/` (codercorner standalone) carries no licence text at all.**
  There is no `LICENSE` and no `COPYING` in the 102 files, and over the whole
  tree `licen`, `public domain`, `royalt` and `permission` each return zero
  files. *(Corrected by the P4 Task 2a fix pass: this bullet also said there was
  no per-file notice anywhere. There is. `copyright` returns **46** files — 45
  carrying the header `Copyright (C) 2001 Pierre Terdiman`, plus `ReadMe.txt`,
  whose one hit is line 100 and is about the demo's artwork. `ReadMe.txt` is 171
  lines and says nothing about licensing at all. This strengthens the conclusion
  below rather than weakening it: 45 assertions of copyright with no grant
  anywhere is a harder problem than silence would be.)*
- **`opcode-ode-1.3.2/COPYING` supplies the grant.** It states: *"The OPCODE
  library distributed as part of ODE is licensed under the same terms as ODE
  (LGPLv2.1+ and BSD)"*, and quotes a dated public message from the author
  ("Opcode is good under ODE's license", Pierre Terdiman, 2003-07-01), citing
  `http://permalink.gmane.org/gmane.comp.lib.ode/3237`.

The tension to resolve before vendoring: **the code we need is the standalone
tree (§1.2), and the only licence text is attached to ODE's copy.** The author's
quoted statement is general rather than limited to ODE's fork, and it predates
the shipped DLL, but this is a question for a human, not for this analysis. What
is measured: the standalone archive contains no terms; ODE's contains the terms
above.

### 6.3 Exact notice text, and where each file goes

Assuming the vendored tree lands at `<repo>/External/qhull/` and
`<repo>/External/opcode/`:

**`External/qhull/COPYING.txt`** — a byte-for-byte copy of
`qhull-2003.1/COPYING.txt`, unmodified. Satisfies clause 2.

**`External/qhull/NOTICE.txt`** — new file, satisfying clauses 3 and 4:

```
Qhull 2003.1 (released 2003/12/30) - modification notice
========================================================

This directory contains Qhull 2003.1, obtained from
http://www.qhull.org, in modified form.

ORIGINAL SOURCE
---------------
The original, unmodified source code for Qhull may be obtained via http
from www.qhull.org.  The version vendored here is Qhull 2003.1, whose
source archive qhull-2003.1.tar.gz has SHA-256
c97d1982e6f5423379bf0ad6dd7293886ac69cae1596b59a305614aec4ae54f8.

MODIFICATIONS
-------------
1. Modified by: NovodeX AG / AGEIA Technologies (original modification,
   reconstructed here, not performed by this project).
   Date: on or before 2005, the build date of the reference binary
   NxPhysics.dll (SHA-256
   4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c).
   Reason: to route every Qhull allocation through the host engine's own
   allocator interface instead of the C library, and to drive Qhull from
   an in-process convex-hull cooker rather than from a command-line
   program.  The modifications are reconstructed in this tree because the
   reference binary cannot be reproduced without them.

2. Modified by: <name of the person making each further change>
   Date: <YYYY-MM-DD>
   Reason: <why>
   (Append one entry per change.  Clause 3 of COPYING.txt requires a
   name, a date, and a reason for every modification.)

All copyright notices in the original files have been left intact, as
clause 1 of COPYING.txt requires.
```

Every modified `.c`/`.h` additionally gets a short in-file marker immediately
below the existing Geometry Center header — clause 1 forbids removing that
header, so the marker goes after it, not in place of it:

```
/* MODIFIED for <project>: see ../NOTICE.txt entry <n>.
   Original source: http://www.qhull.org */
```

**`External/opcode/COPYING`** — a byte-for-byte copy of
`opcode-ode-1.3.2/COPYING`, which is the only licence statement that exists for
this library.

**`External/opcode/NOTICE.txt`** — new file. OPCODE's terms impose no notice
requirement, so this exists for the reader, not for the licence:

```
OPCODE 1.3 (June 2003) - provenance and modification notice
===========================================================

Source: Pierre Terdiman's OPCODE 1.3 standalone distribution,
http://www.codercorner.com/Opcode.htm, archive Opcode13.zip SHA-256
ecf649c786b4916e15cd613eef1ec5b727ee7cca11865fc9fae886151644fb91.
That archive carries no licence text.  The terms in COPYING beside this
file are taken from the copy of OPCODE bundled with the Open Dynamics
Engine (ode-0.13.1.tar.gz, SHA-256
675b897736a1f3006be4b6c972e31eefdbed13a79368ddacab0af431b6ffa7e6),
which quotes the author granting ODE's terms (LGPLv2.1+ and BSD).

The code here is the standalone 1.3 tree, NOT ODE's patched copy: the
reference binary calls MeshInterface::CheckTopology from Model::Build,
which ODE's copy comments out, and its OPC_MeshInterface.cpp assertion
line number (230) matches the standalone tree exactly.

Modifications relative to stock 1.3 are listed in
docs/reconstruction/novodex-physics/evidence/phase4-third-party-sources.md
section 2.
```
