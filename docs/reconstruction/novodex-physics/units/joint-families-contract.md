## Shared NpJoint slots

Recovered by joint-families Task 1 (plan: `docs/superpowers/plans/2026-09-25-joint-families.md`, timing and results in
`evidence/joint-families.md`). This contract continues `units/revolute-contract.md` for the
nine remaining joint families and uses its conventions: addresses are RVAs (image base
0x10000000), rows are named by stable ID, and the Capstone listing
(`oracle/capstone/manifest.json`) is authoritative. A change to a layout, a name or a row
list is made here in the same commit.

Table contents are read from the relocated pointers in the PE image (`oracle/pe.json`
relocations; the inventory `notes` list only the first eight targets). Each family's data
object is the primary table of its 0x1c-byte public object followed by the one-slot
secondary table of the hook base at +0xc (the family's `sub ecx,0xc; jmp <slot 0>`
adjustor thunk).

### Tables

| Family | Table (RVA) | Data object | Primary slots | Secondary (hook base) slot 0 |
|---|---|---|---:|---|
| Prismatic | 0x0011b4b8 | `phys_data_002730` | 33 (0-32) | 0x0011b53c = phys_fn_004755 (8 B) |
| Revolute | 0x0011b328 | `phys_data_002727` | 45 (0-44) | 0x0011b3dc = phys_fn_004727 (8 B) |
| Cylindrical | 0x0011b198 | `phys_data_002724` | 33 (0-32) | 0x0011b21c = phys_fn_004677 (8 B) |
| Spherical | 0x0011b028 | `phys_data_002721` | 37 (0-36) | 0x0011b0bc = phys_fn_004651 (8 B) |
| PointOnLine | 0x0011aeb8 | `phys_data_002718` | 33 (0-32) | 0x0011af3c = phys_fn_004619 (8 B) |
| PointInPlane | 0x0011ad58 | `phys_data_002715` | 33 (0-32) | 0x0011addc = phys_fn_004593 (8 B) |
| Fixed | 0x0011abf8 | `phys_data_002712` | 33 (0-32) | 0x0011ac7c = phys_fn_004563 (8 B) |
| Distance | 0x0011aa98 | `phys_data_002709` | 33 (0-32) | 0x0011ab1c = phys_fn_004533 (8 B) |
| Pulley | 0x0011a938 | `phys_data_002706` | 33 (0-32) | 0x0011a9bc = phys_fn_004507 (8 B) |
| D6 | 0x0011a7c8 | `phys_data_002703` | 37 (0-36) | 0x0011a85c = phys_fn_004471 (8 B) |

### Slot split

A slot is **folded (shared)** when every family's table names the same target row: the
oracle keeps one identical-code-folded copy. It is **per family** when each family has its
own row. The per-family NxJoint rows have the same code shape in every family and differ
only in the `__FILE__` string and line their failed-tryLock report pushes (for example
revolute 004681 `push 0xe` at 0xb2d31 / `push 0x1011b2ec` at 0xb2d33, against pulley 004475
`push 0x10` at 0xb0c81 / `push 0x1011a8fc` at 0xb0c83). `=` repeats the first family's row.

| Slot | NxJoint virtual | Kind | Prismatic | Revolute | Cylindrical | Spherical | PointOnLine | PointInPlane | Fixed | Distance | Pulley | D6 |
|---:|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | ~NxJoint (scalar deleting) | per family (dtor) | 004757 | 004729 | 004679 | 004653 | 004621 | 004595 | 004565 | 004535 | 004509 | 004473 |
| 1 | getActors | folded (shared) | 004539 | = | = | = | = | = | = | = | = | = |
| 2 | setGlobalAnchor | per family | 004731 | 004681 | 004655 | 004623 | 004597 | 004567 | 004541 | 004511 | 004475 | 004435 |
| 3 | getGlobalAnchor | folded (shared) | 004437 | = | = | = | = | = | = | = | = | = |
| 4 | setGlobalAxis | per family | 004733 | 004683 | 004657 | 004625 | 004599 | 004569 | 004543 | 004513 | 004477 | 004439 |
| 5 | getGlobalAxis | folded (shared) | 004441 | = | = | = | = | = | = | = | = | = |
| 6 | getGlobalAnchorVal | folded (shared) | 004497 | = | = | = | = | = | = | = | = | = |
| 7 | getGlobalAxisVal | folded (shared) | 004499 | = | = | = | = | = | = | = | = | = |
| 8 | getState | folded (shared) | 004483 | = | = | = | = | = | = | = | = | = |
| 9 | setBreakable | per family | 004735 | 004685 | 004659 | 004627 | 004601 | 004571 | 004545 | 004515 | 004481 | 004445 |
| 10 | getBreakable | folded (shared) | 004573 | = | = | = | = | = | = | = | = | = |
| 11 | setLimitPoint | per family | 004737 | 004687 | 004661 | 004629 | 004603 | 004575 | 004547 | 004517 | 004485 | 004447 |
| 12 | getLimitPoint | folded (shared) | 004577 | = | = | = | = | = | = | = | = | = |
| 13 | addLimitPlane | per family | 004739 | 004689 | 004663 | 004631 | 004605 | 004579 | 004549 | 004519 | 004487 | 004449 |
| 14 | purgeLimitPlanes | per family | 004747 | 004695 | 004669 | 004639 | 004611 | 004585 | 004555 | 004525 | 004495 | 004455 |
| 15 | resetLimitPlaneIterator | per family | 004741 | 004691 | 004665 | 004633 | 004607 | 004581 | 004551 | 004521 | 004489 | 004451 |
| 16 | hasMoreLimitPlanes | folded (shared) | 004491 | = | = | = | = | = | = | = | = | = |
| 17 | getNextLimitPlane | folded (shared) | 004635 | = | = | = | = | = | = | = | = | = |
| 18 | getType | folded (shared) | 004443 | = | = | = | = | = | = | = | = | = |
| 19 | is | folded (shared) | 004479 | = | = | = | = | = | = | = | = | = |
| 20 | isRevoluteJoint | NxJoint.h inline (shared) | 004417 | = | = | = | = | = | = | = | = | = |
| 21 | isPointInPlaneJoint | NxJoint.h inline (shared) | 004419 | = | = | = | = | = | = | = | = | = |
| 22 | isPointOnLineJoint | NxJoint.h inline (shared) | 004421 | = | = | = | = | = | = | = | = | = |
| 23 | isPrismaticJoint | NxJoint.h inline (shared) | 004423 | = | = | = | = | = | = | = | = | = |
| 24 | isCylindricalJoint | NxJoint.h inline (shared) | 004425 | = | = | = | = | = | = | = | = | = |
| 25 | isSphericalJoint | NxJoint.h inline (shared) | 004427 | = | = | = | = | = | = | = | = | = |
| 26 | isFixedJoint | NxJoint.h inline (shared) | 004429 | = | = | = | = | = | = | = | = | = |
| 27 | isDistanceJoint | NxJoint.h inline (shared) | 004431 | = | = | = | = | = | = | = | = | = |
| 28 | isPulleyJoint | NxJoint.h inline (shared) | 004433 | = | = | = | = | = | = | = | = | = |
| 29 | setName | per family | 004745 | 004693 | 004667 | 004637 | 004609 | 004583 | 004553 | 004523 | 004493 | 004453 |
| 30 | getName | folded (shared) | 004743 | = | = | = | = | = | = | = | = | = |
| 31 | loadFromDesc | per family | 004749 | 004697 | 004671 | 004641 | 004613 | 004587 | 004557 | 004527 | 004501 | 004457 |
| 32 | saveToDesc | per family | 004751 | 004699 | 004673 | 004643 | 004615 | 004589 | 004559 | 004529 | 004503 | 004459 |

Family-specific slots after slot 32 (the secondary slot follows them):

- Revolute: 33-44 = 004709, 004711, 004713, 004715, 004717, 004719, 004721, 004723,
  004701, 004703, 004705, 004707 (revolute contract `## Dispatch tables`).
- Spherical: 33 = 004645, 34 = **004703**, 35 = 004647, 36 = **004707**. Slots 34 and 36 name
  the revolute rows 004703 (getFlags) and 004707 (getProjectionMode): those two bodies are
  folded between revolute and spherical and are already claimed by `core/NpRevoluteJoint.cpp`.
  Task 3c implements them for NpSphericalJoint with the `// Shared NpJoint body; ...` comment
  form (or moves both bodies into `NpJointShared` as helpers), never a second stable-ID line.
- D6: 33-36 = 004461, 004463, 004465, 004467.
- Prismatic, Cylindrical, PointOnLine, PointInPlane, Fixed, Distance, Pulley: none (their
  public interface adds only loadFromDesc/saveToDesc).

### Write-lock report lines per family

The `push <line>` of the failed-tryLock report (`error(2, __FILE__, line, 0, "PhysicsSDK:
WriteLock is still aquired...")`), read from the listing for every per-family slot. All eight
NxJoint setter slots of a family use one line (the one source line that expands the shared
NxJoint methods); slots 31/32 have their own. The `__FILE__` string is always
`\Epic\Novodex\SDKs\Physics\src\core\Np<Family>Joint.cpp`.

| Family | Slots 2/4/9/11/13/14/15/29 | loadFromDesc (31) | saveToDesc (32) |
|---|---|---|---|
| Prismatic | 0xf | 0x13 | 0x1e |
| Revolute | 0xe | 0x12 | 0x1d |
| Cylindrical | 0x10 | 0x15 | 0x20 |
| Spherical | 0xf | 0x13 | 0x1e |
| PointOnLine | 0x10 | 0x14 | 0x1f |
| PointInPlane | 0xf | 0x13 | 0x1e |
| Fixed | 0x10 | 0x14 | 0x1f |
| Distance | 0x10 | 0x14 | 0x1f |
| Pulley | 0x10 | 0x14 | 0x1f |
| D6 | 0x11 | 0x15 | 0x20 |

### Class design (`Physics/src/include/core/NpJointShared.h`)

`template<class Iface, class Internal> class __declspec(novtable) NpJointShared : public
Iface, public EmbeddedHookBase` holds everything the NxJoint level shares:

- **Layout** (0x1c): `Iface` (vptr, userData +0x4, appData +0x8) primary, `EmbeddedHookBase`
  (vptr +0xc, write-lock link `mWord04` +0x10, read-lock link `mWord08` +0x14) secondary,
  `Internal* mInternal` +0x18. `Internal` is the family's internal `<Family>Joint`, so family
  methods call it without casts. The plan's one-parameter form `NpJointShared<Nx<Family>Joint>`
  is written `NpJointShared<Nx<Family>Joint, <Family>Joint>`.
- **Constructor** (protected): zeroes the hook base's two words, stores `internal` at +0x18 and
  again at +0x08: the part of every family's constructor (004725 for revolute) after the base
  constructions.
- **`operator delete`** on the SDK allocator, inherited by every family's deleting destructor.
- **`writeLink()` / `readLink()`**: the link VALUES at +0x10/+0x14 passed to the
  `nxNpSceneGuard*` helpers.
- **The 13 folded bodies** (slots 1, 3, 5, 6, 7, 8, 10, 12, 16, 17, 18, 19, 30) as virtual
  overrides, defined once in `Physics/src/core/NpJointShared.cpp` and explicitly instantiated
  there per family (`template class NpJointShared<NxRevoluteJoint, RevoluteJoint>;`). A family
  task adds its instantiation line and its internal header include to that file.
- **Shared bodies of the per-family setter rows** as protected helpers that take the family's
  file string and line: `forwardSetGlobalAnchor`, `forwardSetGlobalAxis`,
  `forwardSetBreakable`, `forwardSetLimitPoint`, `forwardAddLimitPlane`,
  `forwardPurgeLimitPlanes`, `forwardResetLimitPlaneIterator`, `forwardSetName`, the
  descriptor-typed templates `forwardLoadFromDesc` / `forwardSaveToDesc`, and
  `reportWriteLocked(file, line)` for family-specific write-locked rows. Each family class
  defines the virtual itself (slots 2, 4, 9, 11, 13, 14, 15, 29, 31, 32) as a one-line call
  with its own file and line, so every per-family row keeps its stable-ID line in
  `core/Np<Family>Joint.cpp`.
- Slot 0 (deleting destructor) and the secondary adjustor thunk stay per family: each family
  declares `virtual ~Np<Family>Joint();` with an empty body; the thunk is compiler-generated.

The class declares no new virtual and overrides only `NxJoint` virtuals, so each family's
tables keep the public interface's declaration order. `__declspec(novtable)` keeps the
intermediate class out of the constructor/destructor vtable installs (the oracle installs the
NxJoint / Nx<Family>Joint tables, then the family's final pair, nothing in between).

Verified for revolute: in `build/Release/NxPhysics.map` plus the candidate DLL, the 46 entries
of `??_7NpRevoluteJoint@@6BNxRevoluteJoint@@@` and the one of
`??_7NpRevoluteJoint@@6BEmbeddedHookBase@@@` name the same function in every slot before and
after the change, except that slots 1, 3, 5-8, 10, 12, 16-19 and 30 now name the
`NpJointShared<NxRevoluteJoint,RevoluteJoint>` member instead of the `NpRevoluteJoint` one.
No `NpJointShared` vtable is emitted. The staged-pair joint transcript is unchanged
(`stdout_delta=0`).

### Claimed rows

The 13 folded bodies belong to Np units other than revolute (`work_units.json` inferred
extents). Task 1 claims them in `Physics/src/core/NpJointShared.cpp` (stable-ID line on the
shared body, owning unit named in the body comment) and records them `reconstructed` with
`source`/`implementation` = that file. Rows that were already `reconstructed` through an
`ObjectModel.cpp` model keep their proofs (new text appended); the model gains a
`// Product row:` pointer.

| Stable ID | RVA | Size | Slot | Virtual | Owning unit | Forwards to |
|---|---|---:|---:|---|---|---|
| phys_fn_004539 | 0x000b1600 | 98 | 1 | getActors | core\NpDistanceJoint.cpp | `*[[j+8]+0x19c]`, `*[[j+0xc]+0x19c]` (0 for a null body) |
| phys_fn_004437 | 0x000b0670 | 39 | 3 | getGlobalAnchor | core\NpD6Joint.cpp | 004125 |
| phys_fn_004441 | 0x000b0700 | 39 | 5 | getGlobalAxis | core\NpD6Joint.cpp | 004129 |
| phys_fn_004497 | 0x000b0ff0 | 43 | 6 | getGlobalAnchorVal | core\NpPulleyJoint.cpp | 004137 |
| phys_fn_004499 | 0x000b1020 | 43 | 7 | getGlobalAxisVal | core\NpPulleyJoint.cpp | 004139 |
| phys_fn_004483 | 0x000b0dc0 | 36 | 8 | getState | core\NpPulleyJoint.cpp | 004078 |
| phys_fn_004573 | 0x000b1bd0 | 44 | 10 | getBreakable | core\NpPointInPlaneJoint.cpp | 004076 |
| phys_fn_004577 | 0x000b1c60 | 45 | 12 | getLimitPoint | core\NpPointInPlaneJoint.cpp | 004080 |
| phys_fn_004491 | 0x000b0f10 | 38 | 16 | hasMoreLimitPlanes | core\NpPulleyJoint.cpp | 004083 |
| phys_fn_004635 | 0x000b25d0 | 50 | 17 | getNextLimitPlane | core\NpSphericalJoint.cpp | 004145 |
| phys_fn_004443 | 0x000b0730 | 36 | 18 | getType | core\NpD6Joint.cpp | 004070 |
| phys_fn_004479 | 0x000b0d20 | 50 | 19 | is | core\NpPulleyJoint.cpp | 004070; `this & ((arg != type) - 1)` |
| phys_fn_004743 | 0x000b3670 | 40 | 30 | getName | core\NpPrismaticJoint.cpp | 000454(`[np+0x18]`) |

Total 591 B. Each body was re-read against the listing for this task (read lock `[esi+0x14]`
through 002362, internal `[esi+0x18]`, unlock 002366, the stack purge above). Not claimed:
004417-004433 (slots 20-28; `gap:core\PrismaticJoint.cpp..core\NpD6Joint.cpp`, not an Np
unit, and compiler-generated from `NxJoint.h`'s inline bodies), and the per-family rows of
every other slot (each family task claims its own).
