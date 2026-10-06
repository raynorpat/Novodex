# Phase 2 — Factory-A, part 1: collision groups and the material read/add pair

Recovered evidence for the thirteen Phase 2 rows Task 4 stage 3 reconstructed.
Every RVA is into the UE3-shipped Win32 Release `NxPhysics.dll`
(`sha256 4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`, image
base `0x10000000`). Ghidra references are `oracle/ghidra/manifest.json`; byte
sequences are `oracle/capstone/manifest.json`.

This closes seven `NxPhysicsSDK` vtable slots, including **both placeholders that
returned a value the oracle would not**.

---

## 1. Rows

| row | rva | size | reconstructed as | slot |
| --- | --- | --- | --- | --- |
| `phys_fn_000248` | `0x0000b9a0` | 217 | `NpPhysicsSDK::setGroupCollisionFlag` | 10 |
| `phys_fn_000250` | `0x0000ba80` | 140 | `NpPhysicsSDK::getGroupCollisionFlag` | 11 |
| `phys_fn_000254` | `0x0000bb90` | 90 | `NpPhysicsSDK::addMaterial` | 17 |
| `phys_fn_000256` | `0x0000bbf0` | 125 | its outlined unwind | — |
| `phys_fn_000260` | `0x0000bd50` | 128 | `NpPhysicsSDK::getMaterial` | 19 |
| `phys_fn_000273` | `0x0000c170` | 45 | `NpPhysicsSDK::setFluidGroupPairFlags` | 14 |
| `phys_fn_000275` | `0x0000c1a0` | 47 | `NpPhysicsSDK::getFluidGroupPairFlags` | 15 |
| `phys_fn_000431` | `0x0000dc50` | 133 | `PhysicsSDK::getGroupCollisionFlag` | — |
| `phys_fn_000437` | `0x0000dda0` | 109 | `setGroupCollisionMask`, a file-scope helper | — |
| `phys_fn_000452` | `0x0000df30` | 89 | `PhysicsSDK::setGroupCollisionFlag` | — |
| `phys_fn_000456` | `0x0000dfd0` | 95 | `PhysicsSDK::getMaterial` | — |
| `phys_fn_000482` | `0x0000ef50` | 373 | `PhysicsSDK::addMaterial` | — |
| `phys_fn_000240` | `0x0000b7c0` | 22 | **not** reconstructed — see §6 | 7 |

---

## 2. The collision-group mask

`.data 0x00123a98` is 32 words, one per group, every one filled with `0xffffffff`
by the constructor Task 3 reconstructed. Every pair therefore starts **enabled**.

### `phys_fn_000437` — the symmetric setter

`__cdecl`, three stack arguments, no `this`. Both groups record the other, in
this order:

```
0000ddab  be 01000000    mov esi, 1
0000ddb0  d3 e6          shl esi, cl              ; cl = group2
0000ddb2  84 c0          test al, al              ; the flag
0000ddb8  8b 0c85 983a1210  mov ecx, [eax*4 + 0x10123a98]   ; mask[group1]
0000ddbf  74 24          je 0x1000dde5            ; -> the clearing half
0000ddc1  0b ce          or ecx, esi
0000ddc3  89 0c85 983a1210  mov [eax*4 + 0x10123a98], ecx   ; mask[group1] |= 1<<group2
...
0000dddc  89 0495 983a1210  mov [edx*4 + 0x10123a98], eax   ; mask[group2] |= 1<<group1
```

The shift counts are never masked in the source — `shl` already takes `cl` modulo
32, and the two callers have both already rejected anything `>= 32`.

### `phys_fn_000452` — `PhysicsSDK::setGroupCollisionFlag`

```
0000df35  cmp cx, 0x20 / jae 0x1000df5e
0000df40  cmp ax, 0x20 / jae 0x1000df5e
0000df53  call 0x1000dda0
...
0000df71  push 0x240                      ; line 576
0000df7b  push 1                          ; NXE_INVALID_PARAMETER
0000df7d  call [0x101041b4]               ; FoundationSDK::error
```

Message: `PhysicsSDK::setGroupCollisionFlag: invalid params!  Group must be <= 31!`
(two spaces after the exclamation mark), file
`\Epic\Novodex\SDKs\Physics\src\PhysicsSDK.cpp` at `.rdata 0x0010613c`.

### `phys_fn_000431` — `PhysicsSDK::getGroupCollisionFlag`, and a dead branch

```
0000dc55  cmp dx, 0x20 / jae 0x1000dca8          ; the error path
0000dc60  cmp ax, 0x20 / jae 0x1000dca8
0000dc6c  cmp eax, 0xffff / je 0x1000dc97        ; unreachable
0000dc73  cmp ecx, 0xffff / je 0x1000dc97        ; unreachable
0000dc7b  mov eax, [eax*4 + 0x10123a98]
0000dc87  shl edx, cl
0000dc8b  and eax, edx / setne cl                ; the answer
0000dc97  mov eax, 1 ... setne cl                ; returns true, never entered
0000dcbb  push 0x248                             ; line 584
```

The `0xffff` sentinel tests at `0x0000dc6c` and `0x0000dc73` sit **behind** a
range check that has already rejected `0xffff`, so the block at `0x0000dc97`
that returns `true` cannot be entered. It is written out because the source has
it. The differential pins this: `getGroupCollisionFlag(0xffff, 0)` returns
`false` and reports the range error, it does not return `true`.

**This is the placeholder that lied.** It returned `false`; the oracle returns
`true` for every pair of a freshly created SDK.

---

## 3. The material array

`PhysicsSDK::mMaterials` is `NxArraySDK<NxMaterial>` at `+0x28`, stride `0x48`.

### `phys_fn_000482` — `PhysicsSDK::addMaterial`

`0x0000ef50` is `NxArray::pushBack` inlined, and the growth policy matches the
pinned public header exactly: `reserve((1 + size()) * 2)` when `memEnd <= last`,
copy every element eighteen words at a time, free the old block, then assign and
step `last`. The return is `(last - first) / 0x48 - 1`, the index it appended at.

**This is the other placeholder that lied.** It returned `0`, which is not an
error value — it is the default material's index.

### `phys_fn_000456` — `PhysicsSDK::getMaterial`

```
0000dfd0  8b 0d 043c1210   mov ecx, [0x10123c04]    ; the singleton, not `this`
0000dfd6  8b 41 28         mov eax, [ecx + 0x28]    ; mMaterials.first
0000dff8  cmp edx, esi / jae 0x1000e00a             ; index >= size -> index 0
0000e005  lea eax, [esi + eax*8]                    ; first + index * 0x48
0000e00c  8b 50 38         mov edx, [eax + 0x38]    ; NxMaterial::flags
0000e00f  85 d2 / 79 18    test edx, edx / jns      ; bit 31 clear -> return it
0000e029  8b c6            mov eax, esi             ; otherwise return index 0
```

Two silent fallbacks, neither of them reported:

1. an index at or past the end becomes index 0;
2. a slot whose `flags` carries **bit 31** becomes index 0.

`flags` is at `+0x38` of `NxMaterial` in the pinned public header, which is what
`0x0000e00c` reads. Bit 31 is the same bit Task 3 measured being set on the
template at `0x0000e9ee`; reading it here as "this slot is not live" is what the
fallback means, and the differential gates it directly by adding a material with
bit 31 set and requiring `getMaterial` to hand back index 0 instead.

Like the visualization rows, this member reads `PhysicsSDK::instance` rather than
its own `this`, which is why Ghidra types it `__stdcall`.

---

## 4. The two fluid group-pair slots

```
0000c17b  push 0x101058f0      ; "NxFluid::setFluidGroupPairFlags(): Feature not available!"
0000c180  push 0
0000c182  push 0x101           ; line 257
0000c187  push 0x101058c0      ; NpPhysicsSDK.cpp
0000c18c  push 0xce            ; NXE_DB_WARNING, not an error code
0000c191  call [0x101041b4]
0000c19a  ret 0xc
```

`0xce` is 206, `NXE_DB_WARNING` in the pinned `Nx.h`. The getter is the same
shape at line `0x109` (265) and returns 0 through `xor eax, eax`. Neither slot
touches the SDK, takes a lock, or reports an error code. The shipped DLL exports
the fluid API and refuses this part of it with a debug warning.

---

## 5. The wrappers, and the lock they take

The 24-slot vtable divides cleanly by constness, and the two group slots show
both halves in one pair:

| wrapper | lock word | entry point | failure path |
| --- | --- | --- | --- |
| `phys_fn_000248` `setGroupCollisionFlag` | `+0xc`, writer | `phys_fn_002364` `tryLock` at `0x0000b9c8` | unwinds, reports `NpPhysicsSDK.cpp:136` |
| `phys_fn_000250` `getGroupCollisionFlag` | `+0x10`, reader | `phys_fn_002362` `lock` at `0x0000bab5` | none at all |
| `phys_fn_000254` `addMaterial` | `+0xc`, writer | `tryLock` at `0x0000bbb4` | unwinds, reports `NpPhysicsSDK.cpp:173` |
| `phys_fn_000260` `getMaterial` | `+0x10`, reader | `lock` | none |

The mutating wrappers reference
`PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!`
and report it with error code 2, `NXE_INVALID_OPERATION`; the const wrappers
reference no string. Both walks reach the lock through `Scene` at `+0x6cc`, which
Phase 3 owns. The original Phase 2 fixture did not create scenes, so those
walks ran zero iterations there. Scene creation is now implemented; lock
behavior with live scenes remains a separate differential concern.

---

## 6. Slot 7: blocked at the original Phase 2 close, implemented after Scene layout recovery

Stage 2 listed `getScene` as closing inside Phase 2. It does not. `phys_fn_000240`
is 22 bytes with no lock walk:

```
0000b7c4  8b 49 04            mov ecx, [ecx + 4]      ; this->mSdk
0000b7c8  e8 33270000         call 0x1000df00         ; PhysicsSDK::getScene
0000b7cd  8b 80 cc060000      mov eax, [eax + 0x6cc]  ; Scene::mNp, unguarded
0000b7d3  c2 0400             ret 4
```

`PhysicsSDK::getScene` returns null for an out-of-range index, and `0x0000b7cd`
dereferences it without a check, so `NpPhysicsSDK::getScene` faults on a bad
index. At the original Phase 2 close, the internal `Scene` layout had not yet
been recovered and the wrapper remained a placeholder. The later Scene layout
work established the `+0x6cc` public-wrapper field; the slot now forwards the
index and returns that wrapper in `Physics/src/NpPhysicsSDK.cpp`. The paired
`NxPhysicsSDKTests` probe verifies two valid indices and the survivor after
release, with exact oracle output. Its evidence is in
`evidence/sdk-get-scene.md`. Out-of-range behavior remains intentionally
untested because the oracle faults.

---

## 7. Differential

`run_differential.ps1 -Targets NxPhysicsCoreClusterTests` reports
`oracle_exit=0 candidate_exit=0 stdout_delta=0 stderr_exact=True`,
`differential=pass`. The Factory-A part of the oracle transcript, verbatim:

```
step=groups_default self=1 pair=1 high=1 cross=1
step=groups_disabled forward=0 reverse=0 neighbour=1 unrelated=1
step=groups_reenabled forward=1 reverse=1
step=groups_get_out_of_range errors=1 code=1 line=584 file=\Epic\Novodex\SDKs\Physics\src\PhysicsSDK.cpp message=PhysicsSDK::getGroupCollisionFlag: invalid params!  Group must be <= 31!
step=groups_get_out_of_range value=0
step=groups_get_sentinel value=0
step=groups_get_sentinel errors=3 code=1 line=584 file=\Epic\Novodex\SDKs\Physics\src\PhysicsSDK.cpp message=PhysicsSDK::getGroupCollisionFlag: invalid params!  Group must be <= 31!
step=groups_set_out_of_range errors=4 code=1 line=576 file=\Epic\Novodex\SDKs\Physics\src\PhysicsSDK.cpp message=PhysicsSDK::setGroupCollisionFlag: invalid params!  Group must be <= 31!
step=groups_unchanged_after_error pair=1
step=materials_initial count=1
step=material_default nonnull=1 dynamic=0.000000 static=0.000000 restitution=0.000000 flags=0x00000000
step=material_out_of_range is_default=1
step=materials_added count=10 indices=1,2,3,4,5,6,7,8,9
  material=1 dynamic=0.125000 static=0.250000 restitution=0.062500 is_default=0
  material=2 dynamic=0.250000 static=0.500000 restitution=0.125000 is_default=0
  material=3 dynamic=0.375000 static=0.750000 restitution=0.187500 is_default=0
  material=4 dynamic=0.500000 static=1.000000 restitution=0.250000 is_default=0
  material=5 dynamic=0.625000 static=1.250000 restitution=0.312500 is_default=0
  material=6 dynamic=0.750000 static=1.500000 restitution=0.375000 is_default=0
  material=7 dynamic=0.875000 static=1.750000 restitution=0.437500 is_default=0
  material=8 dynamic=1.000000 static=2.000000 restitution=0.500000 is_default=0
  material=9 dynamic=1.125000 static=2.250000 restitution=0.562500 is_default=0
step=material_default_still_reads dynamic=0.000000
step=materials.allocator alloc_calls=28 refused_calls=1 free_calls=19
step=material_bit31 index=10 count=11 is_default=1 dynamic=0.000000
step=material_bit31 errors=4 code=1 line=576 file=\Epic\Novodex\SDKs\Physics\src\PhysicsSDK.cpp message=PhysicsSDK::setGroupCollisionFlag: invalid params!  Group must be <= 31!
step=fluid_pair_set errors=5 code=206 line=257 file=\Epic\Novodex\SDKs\Physics\src\NpPhysicsSDK.cpp message=NxFluid::setFluidGroupPairFlags(): Feature not available!
step=fluid_pair_get value=0
step=fluid_pair_get errors=6 code=206 line=265 file=\Epic\Novodex\SDKs\Physics\src\NpPhysicsSDK.cpp message=NxFluid::getFluidGroupPairFlags(): Feature not available!
```

`step=material_bit31 errors=4` repeating the previous report is the point of
printing the stream there: adding and reading a retired slot raises nothing.

### The harness was falsified five ways

Each mutation was applied to the committed reconstruction, rebuilt, and run:

| mutation | result |
| --- | --- |
| `getGroupCollisionFlag` returns the old placeholder `false` | `stdout_delta=22` |
| `addMaterial` returns the old placeholder `0` | `stdout_delta=24` |
| the group mask is written one way instead of symmetrically | `stdout_delta=2` |
| `getMaterial` drops the bit-31 fallback | `stdout_delta=6` |
| `getGroupCollisionFlag` reports line 585 instead of 584 | `stdout_delta=4` |

All five fail; the unmutated reconstruction is `stdout_delta=0`.

### What the transcript gates

* **valid lifecycle** — a fresh SDK's 32 group words and one-element material
  array, read back through the public API.
* **capacity growth** — nine pushes past a capacity of one drive
  `(1 + size()) * 2` twice over, and every element is read back afterwards.
* **removal and reuse order** — the bit-31 slot: appended, counted, and then
  refused by `getMaterial` in favour of index 0.
* **allocation failure** — carried from the fluid section's
  `palloc_refused block=null`; the material path has no failure return of its own.
* **boundary and failure** — group 31 legal, 32 and `0xffff` both rejected with
  the exact code, file, line and message, and the mask unchanged afterwards.
* **observable callbacks** — every error site is observed through the user's own
  `NxUserOutputStream`.

---

## 8. Slot disposition after this work

Closed here: **10, 11, 14, 15, 17, 19** — six slots; slot 7 is blocked, not
closed, per §6. With slot 16 from the fluid and visualization work that is
**7 of the 18 closed**.

Slots 12 and 13 moved from open to blocked when the census corrected the phase
column: `phys_fn_000433` calls `phys_fn_004155` and `phys_fn_000435` calls
`phys_fn_004153` (`0x0009a610`, `0x0009a570`), the pair-keyed hash over
`.data 0x00123c28`, and both rows are now Phase 6 by translation-unit enclosure.

Still open inside Phase 2: **18** (`setMaterialAtIndex`, `phys_fn_000484`, 1,457
bytes), **21** (`purgeMaterials`, `phys_fn_000486` + `phys_fn_000488`, 1,081
bytes) and **23** (`setPerformanceInspector`, `phys_fn_000446`, which returns a
literal 1).

---

## 9. Part 2: `setMaterialAtIndex` and `purgeMaterials`

Four more rows: `phys_fn_000258` (`0x0000bc70`, slot 18 wrapper),
`phys_fn_000263` + `phys_fn_000265` (`0x0000be50`, `0x0000bed0`, slot 21 wrapper
and its outlined unwind), `phys_fn_000484` (`0x0000f0d0`, 1,457 bytes) and
`phys_fn_000486` + `phys_fn_000488` (`0x0000f690`, `0x0000f9a0`, 1,081 bytes,
which the census splits but Ghidra decodes as one function).

Both implementations are `NxArray` operations from the hash-pinned public header,
inlined. Nothing in either is a new algorithm.

### `phys_fn_000484` — `PhysicsSDK::setMaterialAtIndex`

Four reachable cases and one that does nothing:

| index | material | effect |
| --- | --- | --- |
| in range | non-null | assign in place |
| in range, not 0 | null | set bit 31 on the slot -- retire it |
| in range, 0 | null | **nothing**; `0x0000f11a` skips the flag write |
| past the end | non-null | `resize(index, gDefaultMaterial)` then `pushBack(*material)` |
| past the end | null | **nothing** |

The gap the grow leaves is filled from `gDefaultMaterial` at `.data 0x001220a0`
and not from a fresh `NxMaterial`: `0x0000f13f` copies that global into the
insert temporary. The template carries bit 31 -- the constructor sets it there
after the first copy -- so every gap slot reads back through `getMaterial` as the
default material rather than as itself. The differential pins that:
`gap12_is_default=1 gap15_is_default=1 last_live=1`.

### `phys_fn_000486` — `PhysicsSDK::purgeMaterials`

The whole body is `NxArray::resize(1)` with the defaulted second argument. The
temporary built at `0x0000f6b4` is a default-constructed `NxMaterial` -- 18 words
with `1.0f` at `+0x1c` and `+0x28`, which is `setToDefault()` -- built
unconditionally and used only when the array is empty. The tail from `0x0000fa49`
is `resize`'s own "free when empty, otherwise `realloc` down to `size()`".

`resize` does not touch the elements it keeps, so **a modified index 0 survives a
purge unchanged**: `step=purge count=1 dynamic=7.500000 restitution=0.375000`.

### A latent heap overflow in the shipped SDK, measured

`setMaterialAtIndex` past the end reaches `NxArray::insert(end(), n, x)`, which
reserves `size() + n` but then runs
`copy(where, where + n, where + n)` -- the shift that makes room for a middle
insert, and an out-of-bounds write of `n` elements when `where == end()`. It therefore needs
`size() + 2n` and only guarantees `size() + n`. The reserve path is always safe,
because `2(size() + n) >= size() + 2n`.

Measured against the shipped DLL from one exact state, 11 materials with a
capacity of 14:

| index | n | reserve? | writes to | result |
| --- | --- | --- | --- | --- |
| 12 | 1 | no | 12 | completes |
| 13 | 2 | no | 13, 14 | **faults** |
| 14 | 3 | no | 14, 15, 16 | **faults** |
| 16 | 5 | yes, to 32 | 16..20 | completes |

The faulting cases corrupt the CRT heap and die inside a later `free` or
`realloc`, so no transcript taken from them would mean anything. The differential
therefore uses index 16, which exercises the same grow, gap fill and append.

This is the oracle's defect, not the reconstruction's: `NxArray.h` is
hash-pinned, so the candidate reproduces it exactly. It is reachable from the
public API by any caller of
`NxPhysicsSDK::setMaterialAtIndex(index, material)` with
`index > getNbMaterials()` whenever
`size() + n <= capacity() < size() + 2n`. **Phase 8 should record it as shipped
behaviour rather than let a later phase "fix" it into a divergence.**

### The wrappers

`phys_fn_000258` is a writer slot reporting `NpPhysicsSDK.cpp:182` from the
immediate at `0x0000bd2f`; `phys_fn_000263`'s outlined unwind `phys_fn_000265`
reports `NpPhysicsSDK.cpp:211` from `0x0000beff`. Both push error code 2,
`NXE_INVALID_OPERATION`.

### Transcript

```
step=set_in_range count=11 dynamic=42.000000 is_default=0
step=set_null_retires count=11 is_default=1
step=set_null_index0 count=11 flags=0x00000000 dynamic=0.000000
step=set_past_end before=11 count=17 placed_dynamic=3.500000 placed_restitution=0.750000
step=set_past_end_gap gap12_is_default=1 gap15_is_default=1 last_live=1
step=set_past_end_null count=17
step=set_at_end before=17 count=18 static=6.250000
step=set_material.allocator alloc_calls=31 refused_calls=1 realloc_calls=2 free_calls=22
step=purge count=1 dynamic=7.500000 restitution=0.375000 flags=0x00000000
step=purge.allocator alloc_calls=31 refused_calls=1 realloc_calls=3 free_calls=22
step=purge_again count=1 dynamic=7.500000
step=purge_then_add index=1 count=2
step=purge_then_add.allocator alloc_calls=32 refused_calls=1 realloc_calls=3 free_calls=23
```

Falsified four ways, each rebuilt and run through the real differential:

| mutation | result |
| --- | --- |
| `purgeMaterials` resets index 0 instead of keeping it | `stdout_delta=10` |
| `setMaterialAtIndex` retires index 0 as well | `stdout_delta=2` |
| the gap is filled with a fresh `NxMaterial` rather than `gDefaultMaterial` | `stdout_delta=2` |
| a null material past the end appends instead of doing nothing | `stdout_delta=12` |

### One allocator slot this closed in passing

`realloc_calls` moving 2 -> 3 across `step=purge` is `resize`'s shrink-to-fit
reaching `SdkAllocatorBridge::realloc` (`phys_fn_000462`, slot `+8` of the vtable
at `.rdata 0x00106304`). It is the only thing in Phase 2 that reaches that slot,
and until `purgeMaterials` existed nothing exercised it. The bridge's `malloc`
and `free` slots were already gated by `NxPhysicsSDKTests`; `mallocDEBUG`
(`phys_fn_000466`) remains unexercised, because nothing on a Release path calls
it.

### Harness change

`wmain` now sets `stdout` unbuffered. A fault used to truncate the transcript at
whatever stdio had last flushed, which sent the first investigation of the
overflow above to the wrong call; unbuffered, it truncates at the call that
faulted.
