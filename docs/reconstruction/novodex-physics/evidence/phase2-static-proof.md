# Phase 2 — the eight rows closed on a static proof

**These rows are NOT closed on a differential, and Phase 8's audit must not treat
them as if they were.**

`ReadWriteLock`'s three entry points, the four `SdkContainer` rows and the SDK
allocator accessor cannot be reached by any input sequence through the public
API, so no oracle-versus-candidate transcript can exist for them.
`NxPhysicsInternalTests` links the candidate's own `PhysicsInternal.cpp` and
`Containers.cpp` and checks them against expectations read out of the
disassembly. It proves the reconstruction does what the disassembly was read to
say. **It does not prove the oracle does the same thing** — there is no
oracle-side object to link against it. The precedent is
`NxFoundationCustomArrayTests`, which the Foundation programme used for the same
reason.

The gate keeps the distinction structural: `gate_targets.ps1` holds
`$NxPhaseStaticProofTargets` separately from `$NxPhaseTestTargets`,
`run_phase_gate.ps1` asserts no target is in both, and the run reports
`gate=static_proof:NxPhysicsInternalTests` rather than a `differential` line.
`run_differential.ps1` can never be handed one.

Every RVA is into the UE3-shipped Win32 Release `NxPhysics.dll`
(`sha256 4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`).

---

## 1. Why these eight and no others

Reachability was computed over the union call graph from every entry point Phase
2 can drive — `NxCreatePhysicsSDK`, the nine `NxFluid*` exports and all 24
`NpPhysicsSDK` vtable rows — restricted to paths that stay inside Phase 2 and the
statically linked CRT. That reaches 125 rows, 72 of them Phase 2.

| row | static reachability | why no differential can reach it |
| --- | --- | --- |
| `phys_fn_002362` `lock` | inside Phase 2 | every caller is an `NpPhysicsSDK` wrapper reaching it through `PhysicsSDK::getScene(i)` in a loop bounded by `getNbScenes()`, which is 0 while `createScene` is blocked |
| `phys_fn_002364` `tryLock` | inside Phase 2 | the same loop |
| `phys_fn_002366` `unlock` | inside Phase 2 | the same loop |
| `phys_fn_004836` `SdkContainer::SdkContainer` | other phases only | no Phase 2 row constructs one |
| `phys_fn_004840` `resize` | other phases only | 148 callers, none drivable |
| `phys_fn_004846` `empty` | other phases only | 47 callers, none drivable |
| `phys_fn_004847` `setExternalBuffer` | other phases only | 3 callers, none drivable |
| `phys_fn_004803` accessor | other phases only | **127 callers and not one on a drivable path** |

The lock rows are the interesting case: static reachability says yes and dynamic
observability says no, because a call-graph edge cannot express "this loop runs
zero times". **A reader in Phase 7 needs to know the lock's dynamic behaviour was
never observed against the oracle, only derived from its disassembly.**

`phys_fn_004803`'s asymmetry is worth stating on its own: a row with 127 call
sites across five phases, unreachable from every public entry point Phase 2 owns.
That is either a statement about how narrow Phase 2's drivable surface is, or a
statement about how much of the SDK is only reachable once a scene exists. Both
readings point at the same place, and Task 5 should pick one deliberately.

---

## 2. `ReadWriteLock`

The block is a `CRITICAL_SECTION` with an interlocked owner flag at `+0x18` and
the owning thread id at `+0x1c`, as Task 3 measured. Imports resolved from the
IAT: `0x10104010` `EnterCriticalSection`, `0x10104014` `LeaveCriticalSection`,
`0x1010402c` `InterlockedCompareExchange`, `0x10104044` `GetCurrentThreadId`.

### `phys_fn_002362` — `lock` (`0x0005b700`, 43 bytes)

```
0005b706  call [EnterCriticalSection]        ; entered, and NOT left
0005b70e  push 0 / push 1 / lea +0x18
0005b716  call [InterlockedCompareExchange]  ; claim the flag if it was clear
0005b71c  call [GetCurrentThreadId]
0005b724  mov [edx + 0x1c], eax              ; record the owner
0005b727  mov al, 1                          ; a literal true
```

### `phys_fn_002364` — `tryLock` (`0x0005b730`, 82 bytes)

```
0005b745  call ICE(&mOwned, 1, 0)            ; returns the previous value
0005b747  test eax, eax / je 0x1005b760      ; was clear -> acquire
0005b751  call GetCurrentThreadId
0005b755  cmp [ecx + 0x1c], eax / je 0x1005b760   ; same thread -> acquire anyway
0005b75c  xor al, al                         ; the ONLY false return
0005b760  EnterCriticalSection, ICE, record owner, return true
```

Reentrant on the owning thread; refused only while another thread holds the flag.

### `phys_fn_002366` — `unlock` (`0x0005b790`, 32 bytes)

```
0005b79d  call ICE(&mOwned, 0, 1)            ; release the flag
0005b7a6  call [LeaveCriticalSection]
0005b7ac  mov al, 1
```

It checks nothing: unlocking a lock this thread does not hold is not refused.

### One property deliberately not asserted

After the first of two `unlock`s the flag is clear but the critical section is
still held, so a second thread's `tryLock` passes the flag test at `0x0005b747`
and then **blocks** inside `EnterCriticalSection`. That is a real property of the
design, and the first version of this test asserted it and hung. The test now
bounds every contender wait at five seconds and treats a timeout as a distinct
third outcome, so a future mistake reports rather than hangs — but the
between-unlocks window itself is recorded here rather than probed.

---

## 3. `SdkContainer`

**This is not the array the SDK singleton uses.** `NxArraySDK<T>` is
`{first, last, memEnd, allocator}`, entirely inline in the pinned public
`NxArray.h`, growing by `(1 + size()) * 2`. This one is
`{capacity, count, entries, growthFactor}` with a **float** factor, growing by
`capacity * factor`, allocating through the SDK allocator rather than through
`nxFoundationSDKAllocator`. Measured confirmation: `phys_fn_000482`,
`phys_fn_000484` and `phys_fn_000486` each call exactly one row, the material
copy, and **none of them calls `phys_fn_004840`**.

Constants read out of the image: `.rdata 0x001041f0` is `0.0f` and
`.rdata 0x001066f8` is `4294967296.0f`, the unsigned-to-float fixup, which is how
the capacity is known to be unsigned.

| row | rva | behaviour |
| --- | --- | --- |
| `phys_fn_004836` | `0x000b4d70` | clears three words and stores `0x40000000` — `2.0f` — in the fourth |
| `phys_fn_004840` | `0x000b4de0` | `resize(needed)` |
| `phys_fn_004846` | `0x000b4f50` | `empty()` |
| `phys_fn_004847` | `0x000b4f90` | `setExternalBuffer(capacity, entries)` |

### `resize`

* `0x000b4de4`–`0x000b4df8` compares the factor against `0.0f` and returns false
  when it is **not greater**, before touching anything.
* New capacity is `capacity * factor` through `_ftol` at `0x000b4e14`, or a
  literal `2` at `0x000b4e1b` when the capacity is zero, then clamped up to
  `count + needed`.
* **`0x000b4e2b` stores the capacity BEFORE the allocation.** A failed allocation
  therefore leaves the capacity already grown and the entries pointer untouched.
  That asymmetry is reproduced, not tidied, and the test asserts it.
* The block comes from slot `+0` of the SDK allocator with a memory type of 0;
  the live entries are copied with `rep movsd`; the old block goes back through
  slot `+0xc`.

### `empty` and `setExternalBuffer`

`0x000b4f5e` and `0x000b4f9e` test only the **sign** of the factor, so a negative
one keeps the buffer. `0x000b4fd5` is what writes `0xbf800000` — `-1.0f` — so an
adopted buffer is marked not-owned, is never freed, and can never grow, because
`-1.0f` also fails `resize`'s `> 0.0f` test. `empty` clears the capacity and
count at `0x000b4f81`/`0x000b4f87`, outside the branch, so a non-owned buffer
keeps its pointer while reporting a capacity of zero.

The shape — a `{max, current, entries, growth}` header with a `2.0f` default and
a `-1.0f` external-buffer marker — matches Ice's `Container`, and the rows sit in
the gap below `opcode/IcePrunable.cpp`. **That identification is an inference**;
only the behaviour above is measured, and the reconstruction is written from the
behaviour.

---

## 4. The allocator accessor, and one expectation that could not be derived

`phys_fn_004803` (`0x000b4000`, 20 bytes):

```
000b4000  mov eax, [0x1012845c]          ; the registered allocator
000b4005  test eax, eax / jne 0x100b4013
000b4009  mov eax, 0x10122368            ; the built-in default
000b400e  mov [0x1012845c], eax          ; install it
000b4013  ret
```

`.data 0x00122368` holds `0x1011b580`, the four-slot vtable stage 1 recovered.
The built-in default's four bodies are **not Phase 2 rows** — `phys_fn_004807`
(`0x000b4030`, Phase 6), `phys_fn_004812` (`0x000b4070`, Phase 3),
`phys_fn_004808` (`0x000b4040`, Phase 6) and `phys_fn_004810` (`0x000b4060`,
Phase 6), each forwarding to the statically linked CRT. They are written in
`PhysicsInternal.cpp` only so the accessor has an object to install; their phases
own the bodies, and the header says so.

**The store at `0x000b400e` is not observable.** An implementation that returned
`&gSdkDefaultAllocator` without recording it is behaviourally identical, because
every later call returns the same object either way. That mutation was applied
and the test still passed, so the check was reworded to claim only what it
proves — that repeated calls agree — and the store is recorded here as measured
but unfalsifiable from outside. It is the one expectation in this file that could
not be derived into a check.

---

## 5. Falsification

Six mutations, each applied to the committed reconstruction and rebuilt:

| mutation | result |
| --- | --- |
| `resize` stores the capacity after the allocation instead of before | `check_failed a failed allocation still leaves the capacity grown`, exit 1 |
| `resize` accepts a zero growth factor | `check_failed a zero growth factor is refused`, exit 1 |
| `empty` releases a buffer it does not own | `check_failed empty does not release a buffer it does not own`, exit 1 |
| `setExternalBuffer` leaves the growth factor positive | `check_failed phys_fn_004847 installs the buffer and the marker`, exit 1 |
| `tryLock` is not reentrant on the owning thread | `check_failed tryLock is reentrant on the owning thread`, exit 1 |
| the accessor does not cache the default it installs | **not caught** — see §4 |

The third mutation originally killed the process with `0xC0000374`
(`STATUS_HEAP_CORRUPTION`) instead of reporting, because the adopted buffer was a
stack array. The test now adopts a heap buffer, so a wrong release is a counted
free and a clean check failure.

Three later mutations were applied to the pointer binding table, and one of them
did not die either: **"removing the last binding leaves an empty table behind"
passed**, because the table's destruction had no observable consequence. The
harness now hands a counting `NxUserAllocator` to `NxCreateFoundationSDK`, so the
two frees `phys_fn_000474` performs are asserted and the rebuild afterwards
proves the pointer was cleared rather than left dangling. The mutation now fails
with `check_failed emptying the table releases its entries and the table itself`.

Unmutated: `static_proof checks=35 status=pass`.

---

## 6. Gate

```
selected_static_proof_targets=NxPhysicsInternalTests
gate=static_proof:NxPhysicsInternalTests command="D:\github\Novodex\build\Release\NxPhysicsInternalTests.exe"
static_proof checks=35 status=pass
gate=static_proof:NxPhysicsInternalTests exit=0
```

Eight rows closed on a **static proof**. Phases 3 and 7 inherit the note that the
lock's dynamic behaviour is still unobserved.
