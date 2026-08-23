# Phase 2 — FluidSupport and Visualization

Recovered evidence for the thirteen Phase 2 rows reconstructed by Task 4 stage 2.
Every RVA is into the UE3-shipped Win32 Release `NxPhysics.dll`
(`sha256 4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`, image
base `0x10000000`). Ghidra references are `oracle/ghidra/manifest.json`; byte
sequences are `oracle/capstone/manifest.json`.

These two components close the Phase 2 export gate: with them the candidate
reports `exports expected=10 found=10` and `run_phase_gate.ps1 -Phase 2` reports
`phase_gate=2 status=pass`.

---

## 1. Rows

| row | rva | size | component | reconstructed as |
| --- | --- | --- | --- | --- |
| `phys_fn_000252` | `0x0000bb10` | 122 | Visualization | `NpPhysicsSDK::visualize` |
| `phys_fn_000439` | `0x0000de10` | 18 | Visualization | `PhysicsSDK::visualize` |
| `phys_fn_000443` | `0x0000deb0` | 26 | Visualization | `PhysicsSDK::getDebugRenderable` |
| `phys_fn_000445` | `0x0000ded0` | 16 | Visualization | `PhysicsSDK::clearDebugRenderable` |
| `phys_fn_001583` | `0x0002ea70` | 1 | FluidSupport | `NxFluidAssert` |
| `phys_fn_003907` | `0x0008e840` | 172 | FluidSupport | `NxFluidDebugTriangle` |
| `phys_fn_003909` | `0x0008e8f0` | 100 | FluidSupport | `NxFluidDebugLine` |
| `phys_fn_003911` | `0x0008e960` | 412 | FluidSupport | `NxFluidDebugSphere` |
| `phys_fn_003913` | `0x0008eb00` | 254 | FluidSupport | `NxFluidDebugPoint` |
| `phys_fn_003915` | `0x0008ec00` | 113 | FluidSupport | `NxFluidDebugArrow` |
| `phys_fn_003917` | `0x0008ec80` | 20 | FluidSupport | `NxFluidPAlloc` |
| `phys_fn_003919` | `0x0008eca0` | 18 | FluidSupport | `NxFluidFree` |
| `phys_fn_003921` | `0x0008ecc0` | 96 | FluidSupport | `NxFluidDebugAABB` |

The closure of the nine exports is exactly ten rows — themselves plus
`phys_fn_000443` — and every one is Phase 2. They add no import: the allocator
arrives through `__imp_?nxFoundationSDKAllocator@NxFoundation@@3PAVNxAllocator@@A`
at IAT `0x001041bc`, which the SDK lifecycle already imports, and the renderable
is reached through the existing `NxFoundationSDK*` at `.data 0x00123c08`.

---

## 2. The three visualization rows are `PhysicsSDK` members, not free functions

Ghidra types all three `__stdcall`, because none of them reads `ecx`. They are
members regardless, and the call sites prove it:

* `0x0000bb4b` `8b 4e 04` loads `this->mSdk` into `ecx` immediately before
  `0x0000bb4e` calls `phys_fn_000439`, exactly as `0x0000bb13` and `0x0000bb39`
  do for `getNbScenes` and `getScene`, which are members by Task 3's evidence.
* Every `NxFluidDebug*` export loads the singleton before calling
  `phys_fn_000443` — `0x0008ece4` `8b 0d 04 3c 12 10`
  (`mov ecx, [0x10123c04]`) in `NxFluidDebugAABB`, and the same six bytes at
  `0x0008e897`, `0x0008e927`, `0x0008e96f`, `0x0008eb03` and `0x0008ec03`.

`phys_fn_000439`'s stack purge of 4 is inherited from the tail jump into a
thiscall, not declared: `0x0000de1c` is `ff 60 24`, `jmp [eax+0x24]`, and the
only path that returns without jumping is `0x0000de1f` `c2 04 00`.

### `phys_fn_000439` — `PhysicsSDK::visualize(const NxUserDebugRenderer&)`

```
0000de10  8b 0d 08 3c 12 10   mov ecx, [0x10123c08]     ; gFoundation
0000de16  85 c9               test ecx, ecx
0000de18  74 05               je 0x1000de1f
0000de1a  8b 01               mov eax, [ecx]
0000de1c  ff 60 24            jmp [eax + 0x24]          ; renderDebugData
0000de1f  c2 04 00            ret 4
```

Slot `+0x24` is the tenth entry of `NxFoundationSDK`, `renderDebugData`, read
against the pinned public header's declaration order (`release`,
`setErrorStream`, `getErrorStream`, `getLastError`, `getFirstError`,
`getAllocator`, `createProfilingZone`, `createDebugRenderable`,
`releaseDebugRenderable`, `renderDebugData`).

### `phys_fn_000443` — `PhysicsSDK::getDebugRenderable()`

```
0000deb0  a1 14 3c 12 10      mov eax, [0x10123c14]     ; gDebugRenderable
0000deb5  85 c0               test eax, eax
0000deb7  75 10               jne 0x1000dec9
0000deb9  8b 0d 08 3c 12 10   mov ecx, [0x10123c08]     ; gFoundation, unchecked
0000debf  8b 01               mov eax, [ecx]
0000dec1  ff 50 1c            call [eax + 0x1c]         ; createDebugRenderable
0000dec4  a3 14 3c 12 10      mov [0x10123c14], eax
0000dec9  c3                  ret
```

It returns the renderable in `eax`; Ghidra types it `void`, and the six draw
exports refute that by reading `eax` back after the call: `0x0008e8ae` and
`0x0008eabd` `mov esi, eax`, `0x0008eb10` `mov esi, eax`, `0x0008e93e` and
`0x0008ed0d` `mov edx, [eax]`, and `NxFluidDebugArrow` simply leaves it in `eax`
across `0x0008ec0e`-`0x0008ec25`, which touch only `ecx` and `edx`. There is no null check on
`gFoundation` anywhere on this path, so a draw made before `NxCreatePhysicsSDK`
faults; that is reproduced, not guarded, which is why the differential never
calls one before creating the SDK.

### `phys_fn_000445` — `PhysicsSDK::clearDebugRenderable()`

```
0000ded0  8b 0d 14 3c 12 10   mov ecx, [0x10123c14]
0000ded6  85 c9               test ecx, ecx
0000ded8  74 05               je 0x1000dedf
0000deda  8b 01               mov eax, [ecx]
0000dedc  ff 60 18            jmp [eax + 0x18]          ; NxDebugRenderable::clear
0000dedf  c3                  ret
```

Its only caller is `phys_fn_000608` at `0x00011190`, Phase 7. No Phase 2
differential can reach it, so it is reconstructed and statically reviewed but not
dynamically gated.

### `phys_fn_000252` — `NpPhysicsSDK::visualize`

It is the one wrapper in the 24-slot vtable that takes each scene's lock outright
instead of trying for it: `0x0000bb34` calls `phys_fn_002362` (`lock`), not
`phys_fn_002364` (`tryLock`), and correspondingly the row references neither the
deadlock string at `.rdata` nor any error site — the only vtable row besides the
two fluid group-pair stubs with no string reference at all. The two scene walks
around the forwarded call run zero iterations for every state this component can
reach, because `createScene` is the only entry point that adds a scene.

---

## 3. The `NxDebugRenderable` vtable slots the shim uses

Read against `Foundation/include/NxDebugRenderable.h`'s declaration order:

| slot | method | used by |
| --- | --- | --- |
| `+0x18` | `clear` | `phys_fn_000445` |
| `+0x20` | `addLine(const NxVec3&, const NxVec3&, NxU32)` | Line, Triangle, Point |
| `+0x2c` | `addAABB(const NxBounds3&, NxU32, bool)` | AABB |
| `+0x30` | `addArrow(const NxVec3&, const NxVec3&, NxReal, NxReal, NxU32)` | Arrow |
| `+0x38` | `addCircle(NxU32, const NxMat34&, NxU32, NxF32, bool)` | Sphere |

`+0x24` is `addTriangle` and `+0x1c` is `addPoint`. **Neither is ever reached.**
`NxFluidDebugTriangle` draws three lines and `NxFluidDebugPoint` draws a
three-axis cross; the shipped DLL never calls the two obvious slots.

---

## 4. The nine exports

All nine end in a bare `ret` — stack purge 0, `__cdecl`, caller cleans. None
validates an input. Vectors arrive as raw float arrays: each one is copied into a
stack temporary float by float through the x87 stack (`fld`/`fstp` pairs with the
third component moved as an integer) before the renderable is called, which is
why the parameters are reconstructed as `const NxF32*` and not `const NxVec3&`.

### `NxFluidAssert` (`phys_fn_001583`)

One byte, `c3`. A no-op hook. The encoding fixes the calling convention and the
absence of a return value; **the parameter list is not recoverable and is an
inference** — it is reconstructed as taking none.

### `NxFluidPAlloc` / `NxFluidFree` (`phys_fn_003917` / `phys_fn_003919`)

```
0008ec80  a1 bc 41 10 10   mov eax, [0x101041bc]   ; __imp_ nxFoundationSDKAllocator
0008ec85  8b 08            mov ecx, [eax]          ; the NxAllocator, unchecked
0008ec87  8b 44 24 04      mov eax, [esp + 4]      ; size
0008ec8b  8b 11            mov edx, [ecx]
0008ec8d  6a 00            push 0                  ; NX_MEMORY_PERSISTENT
0008ec8f  50               push eax
0008ec90  ff 52 08         call [edx + 8]          ; malloc(size, type)
0008ec93  c3               ret
```

Free is the same shape through slot `+0x14`. The import is dereferenced twice
with no null check on either side, and the pointer is neither checked nor cleared
on free — `NX_FREE` does both, so it is not what the oracle compiles to and is
not used here.

### `NxFluidDebugLine` (`phys_fn_003909`)

Six floats in, one `addLine`. The pushes at `0x0008e940`–`0x0008e94a` are
`color`, `&temp1`, `&temp0`, so the call is `addLine(points[0], points[1], color)`.

### `NxFluidDebugTriangle` (`phys_fn_003907`)

Nine floats in, three `addLine` calls at `0x0008e8bf`, `0x0008e8d1` and
`0x0008e8e3`. Resolving the `lea`s against the frame after `push esi; push edi`,
the three temporaries sit at `esp+0xc`, `esp+0x0` and `esp+0x18` of the
post-prologue frame and hold `points[0]`, `points[1]` and `points[2]`; the pairs
passed are `(p0,p1)`, `(p1,p2)`, `(p2,p0)`.

### `NxFluidDebugPoint` (`phys_fn_003913`)

A three-axis cross of half-extent `extent` about `point`, three `addLine` calls.
In every pair the `+` end is built first: `0x0008eb49` is `fadd [esp+0x40]`
before `0x0008eb5f`'s `fsub`, and the same order holds for y at
`0x0008eb70`/`0x0008eb8e` and for z at `0x0008ebb6`/`0x0008ebd2`.

### `NxFluidDebugAABB` (`phys_fn_003921`)

Six floats copied into one `NxBounds3` at `esp+0x18`, then
`addAABB(bounds, color, 0)` — the `renderFrame` argument is the literal
`6a 00` at `0x0008ed0f`.

### `NxFluidDebugArrow` (`phys_fn_003915`)

`addArrow(position, direction, length, scale, color)` through slot `+0x30`, with
the two vectors copied into locals and the last three arguments forwarded
untouched.

### `NxFluidDebugSphere` (`phys_fn_003911`)

Three `addCircle` calls at `0x0008eacb`, `0x0008eadd` and `0x0008eaef`, each with
`6a 28` — a literal 40 segments — and a literal `6a 00` for `semicircle`. The
`0x9c` byte frame holds three separate 48-byte `NxMat34`s at `esp+0`, `esp+0x30`
and `esp+0x60`, all filled before the first call. Resolving every store in
`0x0008e992`–`0x0008eaa0` against that frame gives, in row-major storage order:

| matrix | rotation rows | translation |
| --- | --- | --- |
| `esp+0x00` | `(1,0,0) (0,1,0) (0,0,1)` | centre |
| `esp+0x30` | `(0,0,1) (0,1,0) (1,0,0)` | centre |
| `esp+0x60` | `(1,0,0) (0,0,1) (0,1,0)` | centre |

Three great circles, in the local XY plane of the identity and of the two
axis swaps. That the source declares three separate locals rather than reusing
one is an inference from the three live 48-byte frames; the contents and the
argument lists are measured.

---

## 5. A Foundation divergence this differential found

`addCircle`'s coordinates are not this component's to produce: the Foundation
computes them from `computeScaledCircleSinCos` and the pinned inline
`NxMat34::multiply`, which is `dst = M * src + t` in `NxMat34.h`. Compared over
all 120 sphere points, the shipped and the rebuilt `NxFoundation.dll` **disagree
by one unit in the last place on 3 of them**, and only once the rotation is not
the identity:

| point | oracle | candidate |
| --- | --- | --- |
| line 79 `p0.z` | `6.414214` | `6.414213` |
| line 102 `p0.z` | `4.381966` | `4.381967` |
| line 138 `p0.z` | `6.175571` | `6.175570` |

The source is identical on both sides — `NxMat34.h` is hash-pinned — so this is
x87 scheduling in the rebuilt Foundation, not a reconstruction defect, and Phase
2 cannot fix it. `FoundationClusterTests` reaches `addCircle` but asserts only
`getNbLines()==59`, so nothing gated the coordinates until now, and it only ever
passes the identity transform, which is the case that does not diverge.

Rather than gate a Phase 2 component on a Foundation difference, the differential
gates what `NxFluidDebugSphere` owns — the argument list. In the same process and
through the same pair, the harness asks the same renderable to draw the three
circles this reconstruction claims and requires the replayed 120 lines to be
byte-identical to the shipped ones (`step=sphere_arguments drawn=120 replayed=120
mismatches=0`). **This is an escalation, not a workaround:** Phase 8 should gate
`addCircle` and `NxMat34::multiply` under a non-identity rotation, and decide
whether one ULP of x87 scheduling is in scope for the Foundation.

---

## 6. Differential transcript

`run_differential.ps1 -Targets NxPhysicsCoreClusterTests` reports
`oracle_exit=0 candidate_exit=0 stdout_delta=0 stderr_exact=True`,
`differential=pass`. The oracle transcript, verbatim, with the two pair-identity
lines the runner matches against what it staged elided:

```
resolved create=1 assert=1 palloc=1 free=1 line=1 triangle=1 point=1 sphere=1 arrow=1 aabb=1
step=assert_before_create returned=1
step=create sdk=nonnull
step=visualize_before_any_draw datas=0 points=0 lines=0 triangles=0 digest=0x811c9dc5
step=line datas=1 points=0 lines=1 triangles=0 digest=0xc8c7dc76
  line=0 p0=1.000000,2.000000,3.000000 p1=4.000000,5.000000,6.000000 color=0xff0000ff
step=triangle datas=1 points=0 lines=4 triangles=0 digest=0x9283419d
  line=1 p0=0.000000,0.000000,0.000000 p1=1.000000,0.000000,0.000000 color=0xff00ff00
  line=2 p0=1.000000,0.000000,0.000000 p1=0.000000,1.000000,0.000000 color=0xff00ff00
  line=3 p0=0.000000,1.000000,0.000000 p1=0.000000,0.000000,0.000000 color=0xff00ff00
step=point datas=1 points=0 lines=7 triangles=0 digest=0x47cacccf
  line=4 p0=10.500000,20.000000,30.000000 p1=9.500000,20.000000,30.000000 color=0xffff0000
  line=5 p0=10.000000,20.500000,30.000000 p1=10.000000,19.500000,30.000000 color=0xffff0000
  line=6 p0=10.000000,20.000000,30.500000 p1=10.000000,20.000000,29.500000 color=0xffff0000
step=aabb datas=1 points=0 lines=19 triangles=0 digest=0x93afa565
  line=7 p0=-1.000000,-2.000000,-3.000000 p1=4.000000,-2.000000,-3.000000 color=0xffffff00
  line=8 p0=4.000000,-2.000000,-3.000000 p1=4.000000,5.000000,-3.000000 color=0xffffff00
  line=9 p0=4.000000,5.000000,-3.000000 p1=-1.000000,5.000000,-3.000000 color=0xffffff00
  line=10 p0=-1.000000,5.000000,-3.000000 p1=-1.000000,-2.000000,-3.000000 color=0xffffff00
step=arrow datas=1 points=0 lines=24 triangles=0 digest=0x57f690e2
  line=19 p0=1.000000,1.000000,1.000000 p1=1.000000,1.500000,1.000000 color=0xff00ffff
  line=20 p0=1.000000,1.500000,1.000000 p1=0.925000,1.425000,1.000000 color=0xff00ffff
  line=21 p0=1.000000,1.500000,1.000000 p1=1.075000,1.425000,1.000000 color=0xff00ffff
  line=22 p0=1.000000,1.500000,1.000000 p1=1.000000,1.425000,1.075000 color=0xff00ffff
step=sphere datas=1 points=0 lines=144 triangles=0 digest=0x811c9dc5
step=sphere_replay datas=1 points=0 lines=264 triangles=0 digest=0x811c9dc5
step=sphere_arguments drawn=120 replayed=120 mismatches=0
step=draws.allocator alloc_calls=16 refused_calls=0 free_calls=7
step=palloc block=nonnull
step=palloc.allocator alloc_calls=17 refused_calls=0 free_calls=7
step=free.allocator alloc_calls=17 refused_calls=0 free_calls=8
step=palloc_refused block=null
step=palloc_refused.allocator alloc_calls=17 refused_calls=1 free_calls=8
step=release
step=release.allocator alloc_calls=17 refused_calls=1 free_calls=17
step=recreate sdk=nonnull
step=visualize_after_recreate datas=0 points=0 lines=0 triangles=0 digest=0x811c9dc5
step=line_after_recreate datas=1 points=0 lines=1 triangles=0 digest=0xc8c7dc76
  line=0 p0=1.000000,2.000000,3.000000 p1=4.000000,5.000000,6.000000 color=0xff0000ff
step=recreate_release
step=recreate_release.allocator alloc_calls=26 refused_calls=1 free_calls=26
stream errors=0 asserts=0 prints=0
```

What each part of that gates:

* **valid lifecycle** — `create`, the six draws, `release`, `recreate`,
  `recreate_release`.
* **capacity growth** — the line list runs 1, 4, 7, 19, 24, 144 through the
  Foundation's growable array, and `draws.allocator alloc_calls=16` counts every
  growth the whole sequence caused.
* **removal and reuse order** — `visualize_after_recreate datas=0` is the case
  that proves the destructor's `releaseDebugRenderable(gDebugRenderable)` takes
  the cached pointer by reference and nulls it: a stale cache would make the next
  draw write into a freed renderable, and `line_after_recreate lines=1` shows the
  next one is fresh rather than carrying the first SDK's 144 lines.
* **allocation failure** — `palloc_refused block=null` with
  `refused_calls=1`: the allocator's null is returned unchanged and unchecked.
* **destruction** — `release.allocator free_calls=17` against `alloc_calls=17`.
* **observable callbacks** — every `line=` row is the renderer's own
  `renderData` reading back what the shim wrote.
* **the visualize path itself** — `visualize_before_any_draw datas=0` separates
  "visualize forwards" from "visualize renders". A wrapper that never reached
  `renderDebugData` would report `datas=0` everywhere, and every other step
  reports `datas=1`.

`exports expected=10 found=10` and `phase_gate=2 status=pass`.

---

## 7. Slots this closes

Of the 18 placeholder `NxPhysicsSDK` slots, this work closes slot 16
(`visualize`, `phys_fn_000252`). Slots 14 and 15
(`setFluidGroupPairFlags` / `getFluidGroupPairFlags`, `phys_fn_000273` /
`phys_fn_000275`) remain open: they are oracle error stubs reporting
`NxFluid::setFluidGroupPairFlags(): Feature not available!` from
`\Epic\Novodex\SDKs\Physics\src\NpPhysicsSDK.cpp`, and reconstructing them means
reproducing that error report and its `__LINE__`, which is Factory-A's error-site
work and not this component's.
