# Phase 2 Task 3 — DLL and SDK lifecycle

Oracle: the UE3-shipped Win32 Release `NxPhysics.dll`
(`sha256 4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`,
image base `0x10000000`). Every address below is an RVA; add the image base to
compare against a decompiler listing.

Production commit: `D:\github\Novodex`, `physics: reconstruct SDK lifecycle`.

## 1. Deriving the working set

The plan's Task 3 step 1 says to "select the Phase 2 strongly connected
component containing the exported `NxCreatePhysicsSDK` row". That component has
one member, itself. Phase 1's dependency graph is 3,043 SCCs over 3,071 nodes
with 8 non-trivial components totalling 36 nodes, because it records only direct
call edges and omits all 3,203 indirect ones. It gave no grouping, so the
working set was derived here instead.

**Seeds.** `phys_fn_000490` (`NxCreatePhysicsSDK`, `0x0000fae0`) plus every slot
of the four vtables the reconstructed constructors install. The vtables were not
taken from Ghidra — its `vtables` and `rtti` arrays are both empty because the
DLL is built `/GR-` — but read out of `.rdata` at the addresses the constructors
store into `+0`:

| vtable | .rdata | slots | installed by |
| --- | --- | --- | --- |
| `NpPhysicsSDK` | `0x00105860` | 24 | `phys_fn_000226` at `0x0000b5d0` |
| `PhysicsSDK` | `0x00106384` | 1 | `phys_fn_000472` at `0x0000e1b4` |
| `ShapePairFunctionTable` | `0x0010884c` | 1 | `phys_fn_002338` at `0x0005a8e5` |
| `SdkAllocatorBridge` | `0x00106304` | 4 | static initialiser in the `.data` image |

Slot counts are measured, not assumed: the word after the last slot of each of
`PhysicsSDK`, `ShapePairFunctionTable` and `SdkAllocatorBridge` is ASCII string
data (`0x00106388` = `"PhysicsSDK::createScene: desc.isValid() is false!"`), and
the word after `NpPhysicsSDK` slot 23 is the start of the
`"\Epic\Novodex\SDKs\..."` literal pool. 24 is exactly the number of virtuals
`NxPhysicsSDK` declares with `NX_USE_FLUID_API` on, which is an independent
confirmation that the shipped DLL was built with the fluid API enabled.

**Edges.** Union of (a) Ghidra's per-function `called` list and (b) every
`call`/`jmp` in the Capstone corpus with a resolved `target_rva`, mapped from the
owning inventory row of the source to the owning row of the target.

**Result.** Transitive closure over the seeds: Ghidra 327 rows, Capstone 488
rows, union **490 rows** — phase 2: 249, phase 3: 10, phase 4: 9, phase 5: 52,
phase 6: 1, phase 7: 12, phase 8: 157. That closure is *reachability*, not this
task's deliverable: it runs straight through `createScene` into the whole
simulation. What this task owns is the lifecycle: the factory, the singleton,
the two SDK objects, the objects their constructors allocate, and the destructor
that releases them.

### 1.1 Rows reconstructed by this task

| stable id | rva | bytes | ghidra_ref | capstone_ref |
| --- | --- | --- | --- | --- |
| `phys_fn_000490` | `0x0000fae0` | 138 | `oracle/ghidra/manifest.json#/functions/179` | `oracle/capstone/manifest.json#/entries/1362` |
| `phys_fn_000472` | `0x0000e1b0` | 2172 | `oracle/ghidra/manifest.json#/functions/171` | `oracle/capstone/manifest.json#/entries/1245` |
| `phys_fn_000492` | `0x0000fb70` | 364 | `oracle/ghidra/manifest.json#/functions/180` | `oracle/capstone/manifest.json#/entries/1368` |
| `phys_fn_000494` | `0x0000fce0` | 34 | `oracle/ghidra/manifest.json#/functions/181` | `oracle/capstone/manifest.json#/entries/1382` |
| `phys_fn_000425` | `0x0000db30` | 27 | `oracle/ghidra/manifest.json#/functions/151` | `oracle/capstone/manifest.json#/entries/1177` |
| `phys_fn_000427` | `0x0000db50` | 175 | `oracle/ghidra/manifest.json#/functions/152` | `oracle/capstone/manifest.json#/entries/1179` |
| `phys_fn_000429` | `0x0000dc00` | 67 | `oracle/ghidra/manifest.json#/functions/153` | `oracle/capstone/manifest.json#/entries/1187` |
| `phys_fn_000448` | `0x0000def0` | 10 | `oracle/ghidra/manifest.json#/functions/163` | `oracle/capstone/manifest.json#/entries/1211` |
| `phys_fn_000450` | `0x0000df00` | 33 | `oracle/ghidra/manifest.json#/functions/164` | `oracle/capstone/manifest.json#/entries/1212` |
| `phys_fn_000458` | `0x0000e030` | 26 | `oracle/ghidra/manifest.json#/functions/168` | `oracle/capstone/manifest.json#/entries/1226` |
| `phys_fn_000226` | `0x0000b5d0` | 30 | `oracle/ghidra/manifest.json#/functions/78` | `oracle/capstone/manifest.json#/entries/862` |
| `phys_fn_000228` | `0x0000b5f0` | 8 | `oracle/ghidra/manifest.json#/instruction_ranges/112` | `oracle/capstone/manifest.json#/entries/863` |
| `phys_fn_000230` | `0x0000b600` | 216 | `oracle/ghidra/manifest.json#/functions/79` | `oracle/capstone/manifest.json#/entries/864` |
| `phys_fn_000232` | `0x0000b6e0` | 130 | `oracle/ghidra/manifest.json#/functions/80` | `oracle/capstone/manifest.json#/entries/873` |
| `phys_fn_000238` | `0x0000b7b0` | 8 | `oracle/ghidra/manifest.json#/instruction_ranges/117` | `oracle/capstone/manifest.json#/entries/881` |
| `phys_fn_000261` | `0x0000bdd0` | 122 | `oracle/ghidra/manifest.json#/functions/90` | `oracle/capstone/manifest.json#/entries/944` |
| `phys_fn_000281` | `0x0000c2b0` | 49 | `oracle/ghidra/manifest.json#/functions/96` | `oracle/capstone/manifest.json#/entries/994` |
| `phys_fn_000441` | `0x0000de30` | 115 | `oracle/ghidra/manifest.json#/functions/159` | `oracle/capstone/manifest.json#/entries/1205` |
| `phys_fn_002338` | `0x0005a8e0` | 371 | `oracle/ghidra/manifest.json#/functions/870` | `oracle/capstone/manifest.json#/entries/7314` |
| `phys_fn_002340` | `0x0005aa60` | 35 | `oracle/ghidra/manifest.json#/instruction_ranges/1141` | `oracle/capstone/manifest.json#/entries/7315` |
| `phys_fn_002358` | `0x0005b6a0` | 41 | `oracle/ghidra/manifest.json#/functions/879` | `oracle/capstone/manifest.json#/entries/7407` |
| `phys_fn_002360` | `0x0005b6d0` | 40 | `oracle/ghidra/manifest.json#/functions/880` | `oracle/capstone/manifest.json#/entries/7408` |
| `phys_fn_004805` | `0x000b4020` | 12 | `oracle/ghidra/manifest.json#/functions/1827` | `oracle/capstone/manifest.json#/entries/15615` |

`phys_fn_004805` is a **Phase 6** row by the corrected phase column, not a Phase 2
one. It is written in `PhysicsInternal.cpp` because `NxCreatePhysicsSDK` calls it
and `phys_fn_004803` reads what it stores; Phase 6 owns the row, and
`gates/phase2-closure.json` does not claim it. Its parameter widened from
`SdkAllocatorBridge*` to `SdkAllocator*` when the built-in default gained the same
four-slot interface. The four rows below it are Phase 2.

| `phys_fn_000460` | `0x0000e050` | 29 | `oracle/ghidra/manifest.json#/instruction_ranges/226` | `oracle/capstone/manifest.json#/entries/1227` |
| `phys_fn_000462` | `0x0000e070` | 8 | `oracle/ghidra/manifest.json#/instruction_ranges/227` | `oracle/capstone/manifest.json#/entries/1230` |
| `phys_fn_000464` | `0x0000e080` | 8 | `oracle/ghidra/manifest.json#/instruction_ranges/228` | `oracle/capstone/manifest.json#/entries/1231` |
| `phys_fn_000466` | `0x0000e090` | 28 | `oracle/ghidra/manifest.json#/instruction_ranges/229` | `oracle/capstone/manifest.json#/entries/1232` |
| `phys_fn_006070` | `0x000fe26f` | 6 | `oracle/ghidra/manifest.json#/functions/2417` | `oracle/capstone/manifest.json#/entries/21246` |

`phys_fn_000472` is reconstructed except for the two element-release loops and
the three cache teardowns listed in section 7.

### 1.2 Rows excluded, and why

**Read but not reconstructed.** `phys_fn_000476` (`createScene`),
`phys_fn_000478` (`createTriangleMesh`), `phys_fn_000482` (`addMaterial`),
`phys_fn_000431`/`000452`/`000437` (collision groups), `phys_fn_000433`/`000435`
(actor group pairs), `phys_fn_000456` (`getMaterial`), `phys_fn_000446`
(`setPerformanceInspector`), `phys_fn_004062` (`coreDump`). They were read to
recover the layouts the lifecycle depends on — `createScene` and `addMaterial`
are what fix the material array's `0x48` stride and the scene array's offsets —
but reconstructing them is Task 4's explicit charge ("enumerate and disposition
every `NxPhysicsSDK` public vtable slot"), and `createScene`/`createTriangleMesh`
cannot close before Phase 3 and Phase 4 own `Scene` and `TriangleMesh`.

**Excluded outright.** The 157 phase-8 rows in the closure (CRT and compiler
artifacts, reached through `DllMain` and through the CRT's own initialisers), and
everything reachable only through the non-reconstructed public slots.

**Not in this task despite being in `Physics/src`.** The nine `NxFluid*` exports
(`phys_fn_001583`, `003907`, `003909`, `003911`, `003913`, `003915`, `003917`,
`003919`, `003921`). The Phase 2 plan assigns them to Task 4's
`FluidSupport.cpp`. See section 10 for the consequence.

## 2. The exported factory

`phys_fn_000490`, `NxPhysicsSDK * __cdecl NxCreatePhysicsSDK(NxU32 sdkVersion,
NxUserAllocator * allocator, NxUserOutputStream * outputStream)`, export ordinal
16, stack purge 0. The prototype is Ghidra's and it matches the public header
byte for byte; the three stack reads at `0x0000fb22` (`[esp+8]`), `0x0000faea`
(`[esp+0xc]`) and `0x0000faee` (`[esp+0x10]`) confirm the order after the
`push esi` at `0x0000fae7`.

Recovered body, in order:

1. `0x0000fae0` If the Foundation SDK global at `.data 0x00123c08` is null:
   1. call the imported `NxCreateFoundationSDK` (IAT `0x001041a8`) with
      `(0x02010200, outputStream, allocator)` — `0x0000faf4` pushes the version
      last, `0x0000faf3` pushes `[esp+0x10]` (`outputStream`), `0x0000faf2`
      pushes `esi` (`allocator`); `0x0000faff` adjusts `esp` by 12, so `__cdecl`;
   2. store the result in `0x00123c08` and **return 0 if it is null**;
   3. if `allocator` is non-null, store it at `.data 0x001220ec` and pass
      `0x001220e8` to `phys_fn_004805`.
2. `0x0000fb22` If `sdkVersion != 0x02010200`, return 0. **The version check is
   after Foundation creation**, so a mismatched call still leaves a live
   Foundation SDK bound to whatever allocator and output stream it was given.
   The `NxPhysicsSDKTests` transcript depends on this: its first call is a
   deliberate version mismatch with null allocator and null stream, which is why
   the following `create_default_default` reports no allocator traffic.
3. `0x0000fb30` If the singleton at `.data 0x00123c04` is null, allocate
   `0x38` bytes and construct; store the result (null on allocation failure).
4. `0x0000fb65` Return `[singleton + 4]`, unguarded. On allocation failure the
   oracle dereferences null here. Reproduced.

### 2.1 Version constant

`0x02010200` = `(2 << 24) + (1 << 16) + (2 << 8) + 0`, which is exactly
`NX_PHYSICS_SDK_VERSION` from the transplanted `Nxp.h` with
`NxVersionNumber.h`'s `2.1.2`, and exactly `NX_FOUNDATION_SDK_VERSION`. The
oracle uses the same immediate in both places, so the reconstruction passes
`NX_FOUNDATION_SDK_VERSION` to the Foundation and compares `sdkVersion` against
`NX_PHYSICS_SDK_VERSION`.

## 3. The allocator interface, and an MSVC vtable-ordering finding

Every SDK allocation goes through the imported `nxFoundationSDKAllocator`
(IAT `0x001041bc`), dereferenced twice — the IAT slot holds the address of the
exported variable, the variable holds the `NxUserAllocator*`.

The observed slots are **+8 for allocation and +0x14 for release**:

```
0000fb39  mov   ecx, [0x101041bc]     ; &nxFoundationSDKAllocator
0000fb3f  mov   ecx, [ecx]            ; the NxUserAllocator*
0000fb41  mov   edx, [ecx]            ; vptr
0000fb43  push  eax                   ; eax == 0  (NX_MEMORY_PERSISTENT)
0000fb44  push  0x38
0000fb46  call  [edx + 8]
0000fb49  test  eax, eax              ; no stack adjustment: the callee purged 8
```

Two arguments are pushed and the caller does not clean up, so slot +8 takes two
arguments. Read against `NxUserAllocator`'s declaration order that is
`malloc(size_t)`, which takes one — a four byte leak per call, and `pop esi` at
`0x0000fb5c` would then return to garbage. The resolution is MSVC's rule that
**virtual functions sharing a name are emitted in reverse declaration order**:

| slot | declaration | function |
| --- | --- | --- |
| +0x00 | 2nd `mallocDEBUG` | `mallocDEBUG(size, file, line, className, type)` |
| +0x04 | 1st `mallocDEBUG` | `mallocDEBUG(size, file, line)` |
| +0x08 | 2nd `malloc` | `malloc(size, NxMemoryType)` |
| +0x0c | 1st `malloc` | `malloc(size)` |
| +0x10 | `realloc` | `realloc(memory, size)` |
| +0x14 | `free` | `free(memory)` |

That layout makes both observations consistent. The decisive independent witness
is `phys_fn_000466` at `0x0000e090`: it reads its **fifth** argument at
`[esp+0x14]` and tail-jumps allocator slot **+0**, which is only possible if slot
+0 is the five-argument `mallocDEBUG` — i.e. only under reverse-overload order.
`NxFluidPAlloc` (`0x0008ec80`), a two-argument call to +8, is a second witness.
`NxFluidFree` (`0x0008eca0`) is **not** a witness and must not be cited as one:
`free` sits at +0x14 under both readings, so it discriminates nothing.

The rule also explains why every allocation in the transcript is a *typed* one:
the SDK allocates through `NX_ALLOC`/`NX_NEW`, both of which resolve to
`malloc(size, NX_MEMORY_PERSISTENT)`.

Consequence for the reconstruction: nothing special. The candidate is compiled
against the same header, so `malloc(size, type)` lands in the same slot. The
finding matters because reading the disassembly against declaration order would
have produced a wrong allocator model.

## 4. Recovered layouts

Every size and offset below is asserted in the production sources with
`static_assert`, and every assert compiled on the first build.

### 4.1 `PhysicsSDK` — 0x38 bytes (`Physics/src/include/PhysicsSDK.h`)

| offset | member | measurement |
| --- | --- | --- |
| `0x00` | vtable pointer | `0x0000e1b4` writes `0x10106384` |
| `0x04` | `NpPhysicsSDK* mNp` | `0x0000ea18` stores the wrapper; `0x0000fb59`/`0x0000fb65` read it back as the returned `NxPhysicsSDK*` |
| `0x08` | `NxArraySDK<Scene*> mScenes` | `phys_fn_000448` computes `([+0xc] - [+8]) >> 2`; `phys_fn_000476` grows `+8/+0xc/+0x10` |
| `0x18` | `NxArraySDK<TriangleMesh*> mTriangleMeshes` | `phys_fn_000478` grows `+0x18/+0x1c/+0x20` with a `>> 2` stride |
| `0x28` | `NxArraySDK<NxMaterial> mMaterials` | `phys_fn_000458` computes `([+0x2c] - [+0x28]) / 0x48`; `phys_fn_000482` grows `+0x28/+0x2c/+0x30` |

Total size is measured directly: `0x0000fb44` pushes `0x38`.

The constructor clears exactly `+8/+0xc/+0x10`, `+0x18/+0x1c/+0x20` and
`+0x28/+0x2c/+0x30` and never touches `+0x14`, `+0x24` or `+0x34`. Those three
gaps are not padding and not uninitialised state: `NxArray` declares
`Iterator first, last, memEnd; AllocType allocator;` and `NxUserAllocatorAccess`
is an empty class, so each array is 16 bytes with a one-byte-padded-to-four empty
member at its end that no constructor writes. `0x04 + 0x04 + 3 * 0x10 = 0x38`.

Cross-check: `sizeof(FoundationSDK) == 56` is already gated by
`NxFoundationSDKTests`, and that only balances if `sizeof(NxArraySDK<T>) == 16`
(`4 + S + 4 + 4 + S + 4 + 4 + 4 = 56` gives `S = 16`).

**This closes a Phase 1 open item.** `ghidra/physics_x86_msvc.h` carries, on every
`NxArray_*` typedef, the note `ASSUMPTION NOT YET VERIFIED AGAINST THE BINARY:
this assumes MSVC empty-base optimization for the NxAllocateable base (0 bytes)
and a 1-byte empty-class member padded to the struct's 4-byte alignment, giving
16 bytes total. Task 5 must confirm the size against the oracle before any
reconstruction relies on it.` It is now confirmed against the oracle, not merely
consistent with it: three arrays at `+8`, `+0x18` and `+0x28` inside an object
whose total size is measured at `0x38` fix the stride at 16 directly, and the
untouched `+0x14`/`+0x24`/`+0x34` identify the mechanism as the padded empty
`AllocType` member — the second half of the assumption — rather than leaving 16
as one of several arithmetics that would fit. The empty-base half is confirmed by
the same arithmetic: a non-elided `NxAllocateable` base would put the arrays at
`+0xc`/`+0x1c`/`+0x2c` and the object at `0x3c`. A later phase should treat this
as settled and cite this section rather than re-deriving it.

The class derives from `NxAllocateable`: `NX_NEW` is what produces the
`malloc(size, NX_MEMORY_PERSISTENT)` at `0x0000fb46`, `NxAllocateable::operator
delete` is what produces the `free` at `0x0000fcf9`, and MSVC's null check
around the constructor call is what produces the `je 0x0000fb5e` /
`instance = 0` path. `NxAllocateable` is empty, so it costs nothing.

### 4.2 `NpPhysicsSDK` — 0xc bytes (`Physics/src/include/NpPhysicsSDK.h`)

| offset | member | measurement |
| --- | --- | --- |
| `0x00` | vtable pointer | `0x0000b5d6` writes `0x10105860`; `0x0000c2c6` restores `0x10105800` (the `NxPhysicsSDK` base vtable) in the destructor |
| `0x04` | `PhysicsSDK* mSdk` | `0x0000b5e5`; every wrapper method starts `mov ecx, [this+4]` |
| `0x08` | `ReadWriteLock mLock` | `0x0000b5d3` computes `this+8` for the `phys_fn_002358` call at `0x0000b5dc`; `0x0000c2b3` does the same for `phys_fn_002360` |

Size `0xc` measured at `0x0000ea05`. The constructor builds the lock before it
stores `mSdk`, which is what member construction order plus a constructor-body
assignment produces; the reconstruction is written the same way.

### 4.3 `ReadWriteLockData` — 0x20 bytes (`Physics/src/PhysicsInternal.cpp`)

`ReadWriteLock` itself is one word (the block pointer). The block:

| offset | member | measurement |
| --- | --- | --- |
| `0x00` | `CRITICAL_SECTION` | `0x0005b6bf` passes the block base to `InitializeCriticalSection`; `0x0005b6d6` to `DeleteCriticalSection` |
| `0x18` | `long mOwned` | `0x0005b6b5` clears it; `0x0005b712`, `0x0005b741`, `0x0005b799` pass `block+0x18` to `InterlockedCompareExchange` |
| `0x1c` | `unsigned long mOwnerThreadId` | `0x0005b724` and `0x0005b77b` store `GetCurrentThreadId()` there |

Size `0x20` measured at `0x0005b6ae`. `0x18` (CRITICAL_SECTION on x86) + 4 + 4
accounts for all of it, with no slack.

Only the constructor and destructor are reconstructed. The three lock entry
points (`phys_fn_002362` `lock`, `phys_fn_002364` `tryLock`, `phys_fn_002366`
`unlock`) are reached only through `NpScene`; their recovered semantics are
recorded in section 8 so Phase 3 does not have to re-derive them.

### 4.4 `ShapePairFunctionTable` — 0x124 bytes

| offset | member | measurement |
| --- | --- | --- |
| `0x00` | vtable pointer | `0x0005a8e5` writes `0x1010884c` |
| `0x04` | `void* mFunction[2][6][6]` | `phys_fn_002338` clears words 1..0x24 and 0x25..0x48 and writes handlers into them |

Size `0x124` measured at `0x0000e733` (`push 0x124`, immediately before the
`call [edx+8]` at `0x0000e738`); `4 + 72 * 4 = 292`. Its one-slot vtable is
a scalar deleting destructor whose only work is restoring the vptr and calling
`free`, so the class has a virtual destructor and no other virtual.

The `2 x 6 x 6` shape is measured, the reading is an **inference**: within each
36-word block the written entries are `(0,1..5) (1,1..5) (2,2..5) (3,3..5)
(4,4..5) (5,5)` of a 6x6 index — the upper triangle of a symmetric pair matrix
with the `(0,0)` cell left null — and every value written points into Phase 3
collision code. Calling that "six shape types" is not measured. Slots are left
null here; Phase 3 owns their values.

### 4.5 `SdkAllocatorBridge` — 8 bytes

`.data 0x001220e8`, a statically initialised object whose vptr is already in the
image. Vtable `0x00106304`, four slots, **no destructor slot**:

| slot | function | body |
| --- | --- | --- |
| 0 | `phys_fn_000460` `0x0000e050` | normalises arg2 to 0/1, tail-jumps `mAllocator` slot +8 |
| 1 | `phys_fn_000466` `0x0000e090` | normalises arg5 to 0/1, tail-jumps `mAllocator` slot +0 |
| 2 | `phys_fn_000462` `0x0000e070` | tail-jumps `mAllocator` slot +0x10 |
| 3 | `phys_fn_000464` `0x0000e080` | tail-jumps `mAllocator` slot +0x14 |

`mAllocator` at `+4` is measured from `0x0000fb14`, which stores the caller's
allocator at `0x001220ec` while `0x0000fb0f` passes `0x001220e8`. The size, 8, is
**bounded rather than measured**: nothing allocates this object, so there is no
size immediate anywhere. Two fields are used and the next word (`0x001220f0`) is
zero and unreferenced, which is absence of evidence for a third field, not
evidence of its absence. The
normalisation is real code (`test/jne/xor/mov 1`), not a no-op the compiler
would elide, so the reconstruction writes the ternary that produces it.

`phys_fn_004805` stores the pointer in a file-scope word (`.data 0x0012845c`) and
returns `true`. Nothing in the reconstructed set reads that word back; the
consumer is elsewhere in Phase 2.

### 4.6 `NxMaterial` — 0x48 bytes, and the default material

The material array strides by `0x48`, and the public `NxMaterial` from the
transplanted header is exactly `0x48` bytes with `flags` at `+0x38`. The global
template at `.data 0x001220a0` is **statically initialised in the image**: the
72 bytes there are already `0x3f800000` at `+0x1c` and `+0x28` and zero
elsewhere, and only four instructions in the whole binary reference the object,
none of them a CRT initialiser. `phys_fn_000472` then writes the same 18
words into it, all zero except `+0x1c = 1.0f` and `+0x28 = 1.0f`. Those are
`dirOfAnisotropy.x` and `dirOfMotion.x`, and the whole pattern is exactly what
`NxMaterial::setToDefault()` produces. `0x0000e9ee` then sets bit 31 of `flags`
**on the template, after** the copy into the array (`0x0000e9d4` pushes
`0x001220a0` as the source, `0x0000e9de` advances `last` by `0x48`, and only then
does `or edi, 0x80000000` / `mov [0x101220d8], edi` run). `NxMaterial.h` reserves
bits 16-31 for internal use. Nothing in the reconstructed set reads the bit back.

## 5. Global state and the singleton mechanism

| `.data` | contents | reconstructed as |
| --- | --- | --- |
| `0x001220a0` | default `NxMaterial` template | `gDefaultMaterial` |
| `0x001220e8` | allocator adapter | `gAllocatorBridge` |
| `0x001237c8` | `NxReal max[59]` | `gParameterMax` |
| `0x001238b8` | `NxReal default[59]` | `gParameterDefault` |
| `0x001239a8` | `NxReal min[59]` | `gParameterMin` |
| `0x00123a98` | `NxU32 groupCollisionMask[32]` | `gGroupCollisionMask` |
| `0x00123b18` | `NxReal current[59]` | `gParameter` |
| `0x00123c04` | `PhysicsSDK*` singleton | `PhysicsSDK::instance` |
| `0x00123c08` | `NxFoundationSDK*` | `gFoundation` |
| `0x00123c0c` | array released by the destructor | not reconstructed (section 7) |
| `0x00123c14` | `NxDebugRenderable*` | `gDebugRenderable` |
| `0x00123c18` | `ShapePairFunctionTable*` | `gShapePairFunctionTable` |
| `0x00123c28` | actor-group pair table | not reconstructed (section 7) |

The four parameter arrays are adjacent and contiguous: `0x001237c8 + 59*4 =
0x001238b4`, `0x001238b8 + 59*4 = 0x001239a4`, `0x001239a8 + 59*4 = 0x00123a94`,
`0x00123a98 + 32*4 = 0x00123b18`, `0x00123b18 + 59*4 = 0x00123c04`. The element
count 59 is independently `0x3b`, the immediate both `setParameter` and
`getParameter` compare against, and independently `NX_PARAMS_NUM_VALUES` from the
public header **with `NX_USE_FLUID_API` on** — the third confirmation of the
fluid build.

**The singleton is a plain non-owning global.** There is no reference count, no
guard and no lock. `NxCreatePhysicsSDK` constructs it if the word is null and
otherwise returns the existing wrapper, so a second create returns the same
pointer and performs no allocation. `PhysicsSDK::release` (`phys_fn_000425`)
ignores its `this` entirely — `0x0000db30` loads the global into `ecx` — and is
`NX_DELETE_SINGLE(instance)`: delete through the virtual destructor with the
scalar-deleting flag, then null the global. After release, a create rebuilds
everything from scratch, including the Foundation SDK.

## 6. Constructor order and allocation sizes

`phys_fn_000472`, in order:

1. vtable pointer; the three arrays cleared in declaration order.
2. 59 `(default, min, max)` triples written into the three tables. Every value is
   listed in section 6.1.
3. the 59 defaults copied into the live values.
4. `NX_NEW(ShapePairFunctionTable)` — **`malloc(0x124)`**.
5. all 32 collision-group masks set to `0xffffffff`.
6. `gDefaultMaterial.setToDefault()`.
7. `mMaterials.pushBack(gDefaultMaterial)`. The array is empty, so
   `pushBack` reserves `(1 + 0) * 2 = 2` elements — **`malloc(0x90)`** — and
   copies the template in with an 18-word assignment (`phys_fn_000441`).
8. `gDefaultMaterial.flags |= 0x80000000`.
9. `0x00123c0c = 0`.
10. `NX_NEW(NpPhysicsSDK)(this)` — **`malloc(0xc)`**, and inside it
    `ReadWriteLock` — **`malloc(0x20)`**; `mNp` assigned last.

The growth arithmetic is `NxArray::pushBack` plus `NxArray::reserve` verbatim:
`if (memEnd <= last) reserve((1 + size()) * 2)`, and `reserve` reallocates only
`if (capacity() < n)` with `capacity()` defined as `first == 0 ? 0 : memEnd -
first`. Both the `* 2 + 2` and the `first == 0` special case appear in
`phys_fn_000472`, `phys_fn_000476`, `phys_fn_000478` and `phys_fn_000482`, so
the transplanted `NxArraySDK` is used unchanged rather than reimplemented.

Total for one create with the Foundation not yet up: **six allocations** —
56 (Foundation SDK), 0x38, 0x124, 0x90, 0xc, 0x20 — all typed, and the first is
56 because the Foundation is constructed before anything in this DLL.

### 6.1 Parameter table

`min` and `max` are `0` / `NX_MAX_REAL` unless stated. A parameter whose `min`
and `max` are **both** zero is unbounded (see section 8).

| # | parameter | default | min | max |
| --- | --- | --- | --- | --- |
| 0 | `NX_PENALTY_FORCE` | `0.8f` | `0` | `NX_MAX_REAL` |
| 1 | `NX_MIN_SEPARATION_FOR_PENALTY` | `-0.05f` | `-NX_MAX_REAL` | `0` |
| 2 | `NX_DEFAULT_SLEEP_LIN_VEL_SQUARED` | `0.15f*0.15f` | `0` | `NX_MAX_REAL` |
| 3 | `NX_DEFAULT_SLEEP_ANG_VEL_SQUARED` | `0.14f*0.14f` | `0` | `NX_MAX_REAL` |
| 4 | `NX_BOUNCE_TRESHOLD` | `-2.0f` | `-NX_MAX_REAL` | `0` |
| 5 | `NX_DYN_FRICT_SCALING` | `1.0f` | | |
| 6 | `NX_STA_FRICT_SCALING` | `1.0f` | | |
| 7 | `NX_MAX_ANGULAR_VELOCITY` | `7.0f` | | |
| 8 | `NX_MESH_MESH_LEVEL` | `4.0f` | | |
| 9 | `NX_ENABLE_MESH_DEBUG` | `0.0f` | | |
| 10 | `NX_COLL_INFINITY` | `NX_MAX_REAL` | `0` | `0` |
| 11 | `NX_CONTINUOUS_CD` | `0.0f` | | |
| 12 | `NX_MESH_HINT_SPEED` | `1.0f` | | |
| 13..57 | every `NX_VISUALIZE*` and `NX_VISUALIZATION_SCALE` | `0.0f` | | |
| 58 | `NX_ADAPTIVE_FORCE` | `1.0f` | | |

Two notes. `NX_PENALTY_FORCE` really defaults to `0.8` (`0x3f4ccccd`), not the
`0.6` the public header's comment claims, and `NX_COLL_INFINITY` really defaults
to `FLT_MAX` (`0x7f7fffff`), not the `1e20` the comment claims. Both are read
from the immediates the constructor stores. `(0.15f*0.15f)` and `(0.14f*0.14f)`
were chosen over `0.0225f`/`0.0196f` because both spellings round to the measured
`0x3cb851ec`/`0x3ca0902e` and the products match the header's documented
meaning.

## 7. Destructor and release order

`PhysicsSDK::release` (`phys_fn_000425`) is `NX_DELETE_SINGLE(instance)`.
`delete` runs `phys_fn_000494`, the scalar deleting destructor: `phys_fn_000492`
then, because the flag is 1, `nxFoundationSDKAllocator->free(this)`.

`phys_fn_000492`, in order:

1. restore the `PhysicsSDK` vtable pointer;
2. `NX_DELETE_SINGLE(mNp)` — the wrapper's scalar deleting destructor
   (`phys_fn_000281`) destroys the lock (`free` of the 0x20 block) and then
   frees the 0xc object: **two frees**;
3. *(not reconstructed)* release the array at `0x00123c0c`;
4. *(not reconstructed)* the scene release loop over `mScenes`;
5. *(not reconstructed)* the mesh release loop over `mTriangleMeshes`;
6. `gFoundation->releaseDebugRenderable(gDebugRenderable)` — slot +0x20 of
   `NxFoundationSDK`, which is `releaseDebugRenderable` in declaration order;
7. `gFoundation->release(); gFoundation = 0;` — slot +0 — **one free**, in the
   Foundation, of its own 56-byte instance;
8. `NX_DELETE_SINGLE(gShapePairFunctionTable)` — **one free**;
9. *(not reconstructed)* `phys_fn_004834`, `phys_fn_004828`, `phys_fn_004149`;
10. member destructors in reverse declaration order — `mMaterials` (**one
    free**, the 0x90 block), then `mTriangleMeshes`, then `mScenes`, both of
    which have a null `first` and so free nothing.

Then `free(this)` — **one free**. Six allocations, six frees, balanced.

The five omissions at steps 3, 4, 5 and 9 are all guarded by state this
component cannot reach. `0x00123c0c` is written only by `phys_fn_000454` and
`phys_fn_000480`; `mScenes` and `mTriangleMeshes` are filled only by
`createScene` and `createTriangleMesh`; `0x0012846c`, `0x00128468`, `0x00128464`,
`0x00128460` and the fields of `0x00123c28` are all still null. Every one of them
is therefore a zero-iteration loop or an untaken branch for the whole of the
`NxPhysicsSDKTests` transcript, and the free count proves it: the candidate's
six frees match the oracle's six exactly.

## 8. Parameter get/set, error callbacks, and the reported file name

`phys_fn_000427` and `phys_fn_000429` are member functions that never read
`this` — Ghidra types them `__stdcall`, but their call sites at `0x0000b642` and
`0x0000b71b` load `mov ecx, [edi+4]` first, so they are `__thiscall`. They
operate entirely on the file-scope tables. Reconstructed as ordinary members.

`setParameter`:

* `paramEnum >= 0x3b` — error, return false;
* otherwise, if `min[i] == 0.0f && max[i] == 0.0f`, accept unconditionally
  (`0x0000db72`..`0x0000db9c`, two `fucompp` pairs with the classic
  `test ah,0x44` / `jp` "not equal" idiom);
* otherwise accept iff `value >= min[i] && value <= max[i]` (`0x0000db9e`..
  `0x0000dbc0`, `test ah,0x41` / `jp`, which also rejects unordered);
* on rejection, error and return false.

`getParameter` returns `gParameter[paramEnum]` for `paramEnum < 0x3b`, and
otherwise errors and returns `0.0f` (`fld` of the `.rdata` zero at `0x001041f0`).

All three error sites call the imported
`?error@FoundationSDK@NxFoundation@@SA_NW4NxErrorCode@@PBDHPA_N1ZZ`
(IAT `0x001041b4`), `__cdecl` varargs, five pushes then `add esp, 0x14`:

| site | code | file | line | message |
| --- | --- | --- | --- | --- |
| `0x0000db64` | 1 = `NXE_INVALID_PARAMETER` | `.rdata 0x0010613c` | `0x107` = 263 | `setParameter: parameter value out of range.` |
| `0x0000dbde` | 1 | same | `0x124` = 292 | same |
| `0x0000dc1e` | 1 | same | `0x132` = 306 | `getParameter: param is not an enum.` |

`.rdata 0x0010613c` is `\Epic\Novodex\SDKs\Physics\src\PhysicsSDK.cpp`. The
harness prints the reported file and line verbatim, so the reconstruction has to
emit that exact string and those exact numbers; `__FILE__` and `__LINE__` in this
tree would produce different values. They are written as named constants with the
measuring address in a comment. **This is a measurement, not a choice**: the
transcript's `line=292` and
`file=\Epic\Novodex\SDKs\Physics\src\PhysicsSDK.cpp` matched the oracle on the
first run.

The oracle also inlines a `FoundationSDK::getInstance()` null check
(`mov eax, [0x101041b0]` / `cmp [eax],0` / `jne +1` / `int3`) immediately before
each of these calls, while still calling `error` out of line — `error` is
varargs, which MSVC cannot inline. **The source construct is recovered**:
`FoundationSDK::error(...)` compiles to five pushes, the call and `add esp,20`
and nothing else, whereas `FoundationSDK::getInstance().error(...)` compiles to
the load of `__imp_?instance@...`, `cmp DWORD PTR [eax], 0`, `jne`, `int 3` and
then those same seven instructions — matching `0x0000db59`..`0x0000dbf7`
instruction for instruction. The reconstruction uses the second spelling. It is
also what makes the candidate import `?instance@FoundationSDK@NxFoundation@@`,
which the first spelling does not, and which commit `059c8d02` charters Phase 8
to compare.

### 8.1 Lock semantics recorded for Phase 3

`phys_fn_002362` (`lock`): `EnterCriticalSection(cs)`;
`InterlockedCompareExchange(&owned, 1, 0)`; `ownerThreadId = GetCurrentThreadId()`;
return true.
`phys_fn_002364` (`tryLock`): `InterlockedCompareExchange(&owned, 1, 0)`; if the
previous value was non-zero and `ownerThreadId != GetCurrentThreadId()`, return
false; otherwise `EnterCriticalSection`, re-set `owned`, refresh
`ownerThreadId`, return true.
`phys_fn_002366` (`unlock`): `InterlockedCompareExchange(&owned, 0, 1)`;
`LeaveCriticalSection(cs)`; return true.

Every wrapper method in `NpPhysicsSDK` brackets its forwarded call with a loop
that takes each live scene's writer lock (reader lock, at `NpScene+0x10`, for the
const queries) with `tryLock`, unwinding and reporting `NXE_INVALID_OPERATION`
with `\Epic\Novodex\SDKs\Physics\src\NpPhysicsSDK.cpp` and
`"PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a
deadlock!"` if one cannot be taken. Reaching those locks means going through
`NpScene` (`Scene + 0x6cc`), which Phase 3 owns, so the loops are not
reconstructed. They run zero iterations for every state this component can reach.

## 9. DLL attach and detach

The PE entry point is `0x000f74bd` (`phys_fn_005807`, `entry`), the stock
`_DllMainCRTStartup`. The `DllMain` it calls is `phys_fn_006070` at `0x000fe26f`,
six bytes:

```
000fe26f  xor  eax, eax
000fe271  inc  eax
000fe272  ret  0xc
```

`BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }` — identical to
the CRT's own default, so whether the shipped sources supplied one is not
decidable from the binary. Attach and detach do nothing beyond CRT
initialisation and termination. In particular **the SDK singleton is not torn
down on `DLL_PROCESS_DETACH`**: a process that never calls
`NxPhysicsSDK::release` leaks it, and the Foundation SDK with it.
`Physics/src/DllMain.cpp` reproduces the recovered body and records this.

There are no dynamic initialisers for any of the globals in section 5. The
`.data` image already holds `SdkAllocatorBridge`'s vptr and `gDefaultMaterial`'s
`setToDefault()` result; every other global is zero in the image and written by
the constructor.

The reconstruction reproduces this rather than diverging from it. Compiling
`static NxMaterial gDefaultMaterial;` against the real headers with MSVC 14.51
and `/FAsc` emits the identical static image and **no** `??__E` dynamic
initialiser, and the built `NxPhysics.dll` carries that exact 72-byte pattern at
file offset `0x3000` with no `??__E` symbol anywhere in the image. MSVC folds the
inline constructor into the static initialiser, which is what the 2004 compiler
did too.

## 10. Differential result

`run_differential.ps1 -Targets NxPhysicsSDKTests` — **`differential=pass`**,
`stdout_delta=0`, `stderr_exact=True`, `oracle_exit=0 candidate_exit=0`, on the
first run of the reconstruction. Normalized transcript, identical for both pairs:

```
export=NxCreatePhysicsSDK present=yes
version=0x02010200
step=wrong_version sdk=null
step=create_default_default sdk=nonnull
step=release_default_default
step=create_allocator_default sdk=nonnull
step=create_allocator_default.allocator alloc_calls=6 typed_calls=6 realloc_calls=0 free_calls=0 first_size=56
step=release_allocator_default
step=release_allocator_default.allocator alloc_calls=6 typed_calls=6 realloc_calls=0 free_calls=6 first_size=56
step=create_default_stream sdk=nonnull
step=create_default_stream.stream errors=0 asserts=0 prints=0 code=0 line=0 message= file=
step=release_default_stream
step=create_allocator_stream sdk=nonnull
step=create_allocator_stream.allocator alloc_calls=6 typed_calls=6 realloc_calls=0 free_calls=0 first_size=56
step=create_allocator_stream.stream errors=0 asserts=0 prints=0 code=0 line=0 message= file=
step=release_allocator_stream
step=release_allocator_stream.allocator alloc_calls=6 typed_calls=6 realloc_calls=0 free_calls=6 first_size=56
step=create sdk=nonnull
step=create.allocator alloc_calls=6 typed_calls=6 realloc_calls=0 free_calls=0 first_size=56
step=create.stream errors=0 asserts=0 prints=0 code=0 line=0 message= file=
step=second_create sdk=nonnull identity=same
step=second_create.allocator alloc_calls=0 typed_calls=0 realloc_calls=0 free_calls=0 first_size=0
step=parameters out_of_range=0 in_range=1 value=0.500000 scenes=0 materials=1
step=parameters.stream errors=1 asserts=0 prints=0 code=1 line=292 message=setParameter: parameter value out of range. file=\Epic\Novodex\SDKs\Physics\src\PhysicsSDK.cpp
step=release
step=release.allocator alloc_calls=6 typed_calls=6 realloc_calls=0 free_calls=6 first_size=56
step=release.stream errors=1 asserts=0 prints=0 code=1 line=292 message=setParameter: parameter value out of range. file=\Epic\Novodex\SDKs\Physics\src\PhysicsSDK.cpp
step=recreate sdk=nonnull
step=recreate.allocator alloc_calls=6 typed_calls=6 realloc_calls=0 free_calls=0 first_size=56
step=recreate.stream errors=0 asserts=0 prints=0 code=0 line=0 message= file=
step=recreate_second sdk=nonnull identity=same
step=recreate_release
step=recreate_release.allocator alloc_calls=6 typed_calls=6 realloc_calls=0 free_calls=6 first_size=56
```

`pe32 machine=0x014c magic=0x010b`.

Staged pair hashes. The oracle pair is pinned by `program.json` and the runner
asserts it: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`
(`NxPhysics.dll`) and
`7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990`
(`NxFoundation.dll`). **The candidate hashes are not a pin and must not be read
as one** — they are a record of one build. The first green run staged
`15d6edc4...` / `de34b696...`; after the `getInstance()` change and a `--fresh` +
`--clean-first` rebuild the same green transcript came from `40f720e3...` /
`52847b69...`. The runner asserts only that the candidate `NxPhysics.dll` differs
from the pinned oracle, which is the property that matters.

### Import table

The component's four `NxFoundation.dll` imports in the built candidate, read out
of its import directory:

```
NxCreateFoundationSDK
?error@FoundationSDK@NxFoundation@@SA_NW4NxErrorCode@@PBDHPA_N1ZZ
?instance@FoundationSDK@NxFoundation@@0PAV12@A
?nxFoundationSDKAllocator@@3PAVNxUserAllocator@@A
```

The same four the oracle imports for this component. `?instance@...` is present
only because the error calls go through `getInstance()` (§8); spelled
`FoundationSDK::error(...)` the candidate imported three, not four.

The inlined null check is confirmed **per site, not by a byte count**. Counting
the encoding `83 38 00 75 01 CC` (`cmp dword ptr [eax],0` / `jne +1` / `int3`)
is not a witness: it returns 387 over the whole oracle image, because that is
the ordinary `getInstance()` idiom everywhere in the DLL, and it returns only
**2** over this component's three error sites — `0x0000dbd2` loads the import
into `ecx` (`8b 0d`) and so encodes `83 39 00 75 01 CC`. The check that holds is
the instruction comparison at each of the three sites, and it passes:

| site | oracle | candidate |
| --- | --- | --- |
| `setParameter` line 263 | `a1` imm / `83 38 00` / `75 01` / `cc` | same |
| `setParameter` line 292 | `8b 0d` imm / `83 39 00` / `75 01` / `cc` | `a1` imm / `83 38 00` / `75 01` / `cc` |
| `getParameter` line 306 | `a1` imm / `83 38 00` / `75 01` / `cc` | same |

Both spellings are `mov <reg>, [__imp_?instance@…]` followed by the same three
instructions; only MSVC's register choice differs, which is not part of the
recovered source construct. The candidate carries three `83 38 00 75 01 CC` and
no `83 39 00 75 01 CC`.

### The Phase 2 gate is still red, and correctly so

`run_phase_gate.ps1 -Phase 2` exits 1. It runs `NxPhysicsExportTests` first, and
the candidate reports:

```
export_missing name=NxFluidAssert
export_missing name=NxFluidDebugAABB
export_missing name=NxFluidDebugArrow
export_missing name=NxFluidDebugLine
export_missing name=NxFluidDebugPoint
export_missing name=NxFluidDebugSphere
export_missing name=NxFluidDebugTriangle
export_missing name=NxFluidFree
export_missing name=NxFluidPAlloc
exports expected=10 found=1
FAIL Phase 2 export missing from NxPhysics.dll
```

against the oracle's `exports expected=10 found=10`. Those nine exports are
assigned to Task 4's `FluidSupport.cpp` by the Phase 2 plan, with their own
differential obligations. Three of them (`NxFluidAssert`, a bare `ret`;
`NxFluidPAlloc`; `NxFluidFree`) are fully recovered in section 3 and could be
written in ten lines, but writing them here would ship them without the gate
Task 4 is required to build for them. `NxPhysicsExportTests` compares the whole
transcript, so a partial implementation would leave the gate red anyway. The
gate goes green when Task 4 lands.

## 10a. Version resource

`Physics/src/win32/resource.rc` shipped `FILEVERSION 2,1,0,0` and
`VALUE "FileVersion", "2, 1, 0, 0"` while the oracle's version resource reports
`2.1.2.6000`, in the same tree where `0x02010200` is a load-bearing constant. It
now mirrors what commit `979ee1b` did for the Foundation: include
`../../../NxBuildNumber.h` and use `NX_SDK_VERSION_MAJOR,MINOR,BUGFIX,BUILDNUM`
and `NX_SDK_FULL_VERSION_STRING`. Verified against the oracle:

```
oracle    FileVersion=2.1.2.6000 ProductVersion=2.1.2.6000
candidate FileVersion=2.1.2.6000 ProductVersion=2.1.2.6000
```

The other version strings already matched (`NovodeX Physics SDK`, `NxPhysics`,
`  NxPhysics`, `NovodeX AG`, `Part of NovodeX SDK.`).

One defect in that file is **noticed and deliberately not fixed here**: its
`LegalCopyright` holds the byte sequence `EF BF BD` (U+FFFD, the replacement
character) where the Foundation's equivalent line holds `A9` (`(c)` in the
`code_page(1252)` the file declares). The candidate's copyright string therefore
differs from the oracle's. It predates this task, it is not part of the version
fix, and no gate compares resources today. Flagged rather than changed.

## 10b. Two placeholders return what the oracle would not

Of the 18 unreconstructed public slots, 16 return a value that is at worst
uninformative. Two are worse, and are called out in the `NpPhysicsSDK.h` comment
as well as here:

* `getGroupCollisionFlag` returns `false`. This task recovered that the
  constructor sets all 32 masks to `0xffffffff`, so the oracle returns `true` for
  every pair. The placeholder is the exact inverse of state this task itself
  established.
* `addMaterial` returns `0`, which is a valid `NxMaterialIndex` — the default
  material's — and not an error value.

Neither is reachable from `NxPhysicsSDKTests`, so no gate can catch a caller who
believes them. Task 4 closes both.

## 11. Where the two oracles disagreed

1. **Function boundaries.** Ghidra created no function at `0x0000b5f0`,
   `0x0000b7b0`, `0x0000b770`, `0x0000b7c0`, `0x0005aa60`, `0x0000e050`,
   `0x0000e070`, `0x0000e080` or `0x0000e090` — their inventory `ghidra_ref`
   points at an `instruction_ranges` entry, not a `functions` entry — because
   each ends in a tail `jmp` through a vtable or into another function. Capstone
   has all of them as entries. Resolved in Capstone's favour; they are the
   `release`, `getNbScenes`, `createScene`, `getScene`, table-destructor and
   allocator-adapter bodies, and the whole `SdkAllocatorBridge` vtable would have
   been unreadable without them.
2. **Closure size.** Ghidra's `called` closure from the seeds is 327 rows,
   Capstone's is 488, union 490. 163 rows are Capstone-only, almost all reached
   by a tail `jmp` Ghidra's `called` list does not record; two rows
   (`phys_fn_006139`, `phys_fn_006148`) are Ghidra-only, reached through CRT
   dispatch Capstone does not resolve to a direct target. Both were needed.
3. **Calling convention.** Ghidra types `phys_fn_000425`, `phys_fn_000427`,
   `phys_fn_000429`, `phys_fn_000431`, `phys_fn_000433`, `phys_fn_000435`,
   `phys_fn_000452`, `phys_fn_000456` as `__stdcall` because their bodies never
   read `ecx`. The Capstone call sites show `ecx` being loaded with `this`
   immediately before each call. Resolved in Capstone's favour: they are
   ordinary members that happen to work on file-scope state, and the
   reconstruction declares them as members. Had Ghidra been believed they would
   have become free functions and `NpPhysicsSDK` would have called them without
   a receiver — same behaviour, wrong shape.
4. **Ghidra's `NxUserAllocator` slot naming** would have made `+8`
   `malloc(size_t)`. The Capstone byte-level evidence (two pushes, no caller
   cleanup, `pop esi`/`ret` immediately after) forced the MSVC reverse-overload
   reading in section 3. This was the only disagreement that would have produced
   a wrong behaviour rather than a wrong shape.

Where the two agreed, they agreed exactly: every immediate in the parameter
tables, every allocation size, and every field offset in section 4 reads the same
from `decompiler_c` and from the instruction stream.

## 12. Inferences, called out

Everything above is measured except these:

* **`ShapePairFunctionTable` is a 6x6 shape-pair matrix, twice.** The offsets,
  the two 36-word blocks and the written-entry pattern are measured; reading the
  pattern as a symmetric pair matrix over six shape types is an inference, and
  the type name reflects it.
* **`SdkAllocatorBridge`'s second parameter is a memory type.** Measured: the
  slot forwards to `NxUserAllocator`'s typed `malloc` after normalising the
  argument to 0 or 1. Inferred: that the caller's domain is `NxMemoryType` rather
  than a `bool`.
* **The class names** `PhysicsSDK`, `NpPhysicsSDK`, `ReadWriteLock`,
  `ShapePairFunctionTable`, `SdkAllocatorBridge`. `PhysicsSDK` and
  `NpPhysicsSDK` come from the inventory's `source` attribution
  (`Physics/src/PhysicsSDK.cpp` on 10 rows, `Physics/src/NpPhysicsSDK.cpp` on 16)
  and from the error strings the oracle itself emits (`PhysicsSDK::createScene:
  ...`), so they are evidence. `ReadWriteLock`, `ShapePairFunctionTable` and
  `SdkAllocatorBridge` are descriptions of measured behaviour, not recovered
  names.
* **`NX_MF_INTERNAL_BIT31`.** The bit and the write are measured; that it is a
  meaningful internal material flag rather than a leftover is inferred from
  `NxMaterial.h`'s "Bits 16-31 are reserved for internal use".
* **`DllMain`'s provenance.** Its body is measured; whether the shipped sources
  defined it or the CRT supplied the default is not decidable.

Member *access specifiers* are not recoverable at all and were not inferred: the
recovered fields are left accessible so the measured offsets can be asserted.
