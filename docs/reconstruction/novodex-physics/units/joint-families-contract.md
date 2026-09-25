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

## Shared rows

Recovered by joint-families Task 2 from the four shared-code bundles
(`units/Joint.cpp.md`, `units/gap__NpSpringAndDamperEffector.cpp__to__Joint.cpp.md`,
`units/gap__Joint.cpp__to__D6Joint.cpp.md`,
`units/gap__core__PrismaticJoint.cpp__to__core__NpD6Joint.cpp.md`), the Capstone listing,
`oracle/dependencies.dot` and the relocated table words in `oracle/pe.json`. Every row written
had an `ok` manifest decompile or a pinned supplement one (004111), so
`oracle/ghidra/supplement.json` is unchanged and Ghidra was not run.

Callers "by family" come from `dependencies.dot`, split with the listing into real calls and
table installs (a family constructor's edge to a base-table row it inherits). The family
internal tables were read from the constructors' edges: every family table inherits Joint
slot 2 (004111) and slot 3 (004087); the prismatic, cylindrical, point-on-line,
point-in-plane, distance, pulley, fixed and D6 tables also inherit slot 7 (004135); no
family inherits slot 6 (004133) or slot 5 (004119). The Joint base table (0x101192d0 in
`phys_data_002614`) reads 0:004248 1:001583 2:004111 3:004087 4:_purecall 5:004119 6:004133
7:004135 8:004248.

### Decision rule

`write` = a Joint base or constraint-record row that a family row calls or inherits through
its table (creation, getters, the public NxJoint setters every family's Np slot forwards to,
and the solver, visualization and projection slots), whose own callees are written, reused
or deferred stubs. This makes `core/Joint.cpp` and `core/JointSupport.cpp` complete for every
family, so Tasks 3a-3i write only family rows. `defer` = not called by any joint family
(effector, scene-dump, articulation and scene rows), or owned by a Scene/SDK unit (000xxx),
which keep asserting stubs even when a written row calls them. `reuse` = already written by
the pilot or Task 1, an existing candidate symbol, or compiler-generated.

No family's creation or getter path (createJoint -> 004141 -> 004107/004121/004097, the
family's saveToDesc -> 004066, and the folded Np getters) needs a new shared row: the pilot
already wrote all of them. The rows written here are on the families' setter, solver,
debug-visualization and projection paths.

### write (Task 2): 12 rows, 10,506 B

| Row | RVA | B | File | Called by (family rows) | Name |
|---|---|---:|---|---|---|
| 004064 | 0x957a0 | 385 | core/Joint.cpp | revolute 004356, spherical 004298 (projection slots) | `Joint::row004064` |
| 004091 | 0x95d60 | 62 | core/Joint.cpp | 004111 (every family, slot 2) | `Joint::row004091` |
| 004093 | 0x95da0 | 116 | core/Joint.cpp | the solver rows of every family: prismatic 004386, cylindrical 004326, spherical 004296 and 004310 (gap), point-on-line 004272, point-in-plane 004258, distance 004240, pulley 004228, fixed 004246, D6 004194/004196, revolute 004360/004362; and 004135 | `Joint::row004093` |
| 004099 | 0x962f0 | 1,112 | core/Joint.cpp | every family's Np slot 2: 004731 004681 004655 004623 004597 004567 004541 004511 004475 004435 | `Joint::setGlobalAnchor` |
| 004101 | 0x96750 | 5,302 | core/Joint.cpp | every family's Np slot 4: 004733 004683 004657 004625 004599 004569 004543 004513 004477 004439 | `Joint::setGlobalAxis` |
| 004109 | 0x97e60 | 366 | core/Joint.cpp | every family's Np slot 11: 004737 004687 004661 004629 004603 004575 004547 004517 004485 004447 | `Joint::setLimitPoint` |
| 004111 | 0x97fd0 | 113 | core/Joint.cpp | internal slot 2 of every family table (inherited) | `Joint::row004111` |
| 004123 | 0x98be0 | 518 | core/Joint.cpp | debug-visualization slots: revolute 004364, cylindrical 004318, spherical 004312 (gap), point-on-line 004274, point-in-plane 004260, D6 004200 | `Joint::row004123` |
| 004133 | 0x99ab0 | 134 | core/Joint.cpp | Joint base slot 6 (every family overrides it); called directly by the scene row 000728 (0x167ea) | `Joint::row_slot6` |
| 004135 | 0x99b40 | 701 | core/Joint.cpp | internal slot 7 of the prismatic, cylindrical, point-on-line, point-in-plane, distance, pulley, fixed and D6 tables (inherited); called directly by revolute 004362 and spherical 004310 | `Joint::row_slot7` |
| 004143 | 0x9a0d0 | 860 | core/Joint.cpp | every family's Np slot 13: 004739 004689 004663 004631 004605 004579 004549 004519 004487 004449 | `Joint::addLimitPlane` |
| 004391 | 0xaf3c0 | 837 | core/JointSupport.cpp | the solver rows of every family (as 004093), 004393, 004135; raycast rows 000879/000897 | `JointSupportRecord::row004391` |

004091 was already `reconstructed` through its recorded differential; it is now written
natively (proofs kept, new text appended). 004099, 004101, 004109 and 004143 carried the
oracle's own `__FILE__` (`Physics/src/Joint.cpp`) as `source`; their notes keep it, and the
validator's `UNRESOLVED_SOURCE_PATHS` entry for that path is removed.

### reuse: rows already written or generated

- Pilot (`core/Joint.cpp`, `core/JointSupport.cpp`; revolute contract `### write (Task 6)`):
  004066 004070 004074 004076 004078 004080 004081 004083 004087 004089 004095 004097
  004107 004121 004125 004127 004129 004131 004137 004139 004141 004145, 004389 004393.
- 004119 (0x98750, 66 B): the Joint base's scalar deleting destructor, generated from
  `~Joint()` (004095) and `Joint::operator delete`; not claimed. No family reaches it (each
  overrides slot 5 with its own deleting destructor, which calls 004095 directly).
- 004115, 004117 (`dynamically_gated`): already implemented in `Physics/src/JointDesc.cpp`.
- 004417-004433 (9 x 8 B): the `NxJoint.h` inline is*Joint bodies (Np slots 20-28),
  compiler-generated.

### defer: Joint.cpp and JointSupport.cpp rows

| Row | RVA | B | Reached from | Why deferred |
|---|---|---:|---|---|
| 004085 | 0x95cb0 | 10 | NpSphereShape 001183, fluid 003860, scene dump 004004/004007 | folded `getName` of objects keyed by `this` (000454); no joint family calls it (Np getName 004743 calls 000454 itself). Already `reconstructed` by its model |
| 004103 | 0x97c10 | 142 | Scene row 000632 | Scene-side joint teardown; needs Scene rows 000557 and 000633 |
| 004105 | 0x97ca0 | 133 | 004113 | break-event release path; needs 000557 and 000633 |
| 004113 | 0x98050 | 81 | break event slot 0 (the event 004111, revolute 004374 and spherical 004308 post) | fires the user joint-break notify and releases through Scene::releaseJoint 000653; dispatched only by the Scene's event flush, which the candidate lacks. Stays the inline asserting `JointBreakEvent::row004113` |
| 000022 | 0x1840 | 27 | revolute 004356, spherical 004298, D6 004207 | owner gap `<start>..Actor.cpp`; asserting stub `Row000022Fixture` |
| 000571 | 0x108e0 | 22 | 004111, revolute 004374, spherical 004308 | owner Scene.cpp; small, but its list at Scene+0x620 is consumed only by the Scene's event flush, which the candidate lacks, and the candidate's Scene+0x620 word is not established. Asserting stub `Row000571Fixture` |
| 000598 | 0x10f50 | 138 | 004093 | owner Scene.cpp (grows the record array at Scene+0x5b8); new asserting stub `Row000598Fixture` in `core/JointSupport.cpp` |
| 000633 | 0x12660 | 370 | 004095, 004107 (re-bind), 004103, 004105, 004119 | owner Scene.cpp (joint removal); asserting stub `Row000633Fixture` |
| 000758 | 0x17630 | 214 | revolute 004356 | owner gap SceneRaycast..CapsuleShape; asserting stub `Row000758Fixture` |

The solver, visualization and projection rows the families write in Tasks 3a-3i call
written rows (004093, 004391, 004389, 004393, 004123, 004127, 004064, 004135) and the stubs
above; as in the pilot, those stubs assert, so a family row that reaches one cannot complete
on that arm until its Scene row is written.

### defer: the gap rows that are not joint code

| Rows | Count | B | Owner (by evidence) | Why |
|---|---:|---:|---|---|
| 003946-003981 | 19 | 2,866 | SpringAndDamperEffector internals | callers NpSpringAndDamperEffector.cpp (003940) and Scene.cpp (000587, the effector creation path); 003966/003968 store 5 and 4 floats, the shapes of `NxSpringAndDamperEffector::setLinearSpring`/`setLinearDamper`; tables 002383/002386. No joint family calls them |
| 003983-004062 | 41 | 22,965 | scene-dump writer (`.psc`) | strings `%s.psc`, `PsJointBegin %s`, `PsShapeBegin Shape%d`, `PsActorPair`; entry 004062 called by NpPhysicsSDK.cpp 000267. It reads joints through 004068/004070/004072/004081/004083/004145; no joint family calls it |
| 004068 | 1 | 56 | Joint member, dump only | internal getActors (both bodies' +0x19c owners, `ret 8`); called only by the dump writer (004015, 004037). Already `reconstructed` by its model |
| 004072 | 1 | 25 | Joint member, dump only | internal `is(type)` over +0x168; called only by 004037 |
| 004147-004176 | 16 | 3,356 | scene / articulation helpers | callers Scene.cpp, PhysicsSDK.cpp and the SceneRaycast and ContactPlaneMesh gaps; 004172 reports "No articulateable joints in group." |
| 004387 | 1 | 6 | fluid trampoline | caller fluids gap 003934 |
| 004395-004405 | 6 | 2,906 | articulation solver | reached only from 004159/004174 (above) |
| 004407-004415 | 5 | 581 | list helpers of the NpPrismaticJoint..IcePrunable gap | callers 004759/004764 in that gap; not joint rows |

So the 65-row `gap:NpSpringAndDamperEffector.cpp..Joint.cpp` is effector code (19 rows),
the scene-dump writer (41) and five Joint members (004064 written, 004066/004070 pilot,
004068/004072 dump-only).

### Declaration changes made by Task 2

Headers `Physics/src/include/core/Joint.h` and `Physics/src/include/core/JointSupport.h`.

- `Joint::row004111(NxU32, NxU32)` -> `row004111(const JointSupportRecord* record, NxReal
  value)`: the listing tests `(record->mFlags >> 11) & 0xff == 0x4d` (0x97fd7-0x97fdd) and
  stores the second argument in the break event's +0xc (0x98015-0x98022). The supplement
  decompile names it `unaff_retaddr`; the listing reads `[esp+0xc]` after one push, the
  second stack argument, with `ret 8`.
- New `void Joint::row004091()` (the record-flag loop 004111 calls).
- `Joint::mUnknown160[2]` keeps its name; its comment now records what the rows establish:
  first index and count of the records taken from 004093 (the scene row 000728 resets them to
  -1 / 0 before calling 004133).
- New read view `JointActorBody` (0x50 bytes) for the record `JointBodyRecord::mOwner`
  (+0x19c) points to: +0x08 the dynamic body record (null for a static actor), +0x20 a
  row-major 3x3 and +0x44 a vec3, the static global pose. 004099 and 004101 read the actor's
  global pose through it (0x9636d-0x96465). The candidate's 0x50-byte actor body
  (`Physics/src/Scene.cpp`) has the same three fields.
- New deferred stub `Row000598Fixture::row000598()` in `JointSupport.h`/`.cpp`.
- `core/Joint.cpp` includes `PhysicsSDK.h` (004135 reads SDK parameters 0 and 1 through
  `PhysicsSDK::getParameter`, as the revolute solver rows do), `NxUtilities.h`
  (`NxNormalToTangents`, the Foundation import at `[0x1010418c]` 004101 calls) and `<new>`.
- `core/RevoluteJoint.cpp`: the comment at 004362's call of `Joint::row_slot7` no longer says
  the row is a stub. No code change.

### Decompile/listing disagreements (recorded in the row comments)

- 004099: the y difference stays unrounded (0x9648e-0x96495); the decompile rounds it.
- 004101: body 0 and body 1 group their frame-transform sums differently, and the four
  inlined quaternion-from-matrix sites differ in which diagonal pair sum they store as a
  float and reuse; the non-negative arm divides 0.5 by the float-stored root (unlike 004121's
  copy). The decompile shows every intermediate as a float.
- 004109: the unrounded z difference feeds y (0x97eed `fst`).
- 004111: the second argument (see above).
- 004135: the decompile drops the kind tests of the flag update as unreachable; the listing
  keeps them.
- 004143: the local point's x uses the unrounded z difference; d is formed with a multiply by
  -1.0f (0x9a2c5); the world limit point is inlined with its own grouping, not a call of
  004080.
- 004391: the listing's six-term sum order and, in the bit-10 arm, x summed unrounded across
  the bodies while y and z are stored first.

### Checks beyond the gate

The joint transcript does not reach any written row. An uncommitted experiment added calls of
`setGlobalAnchor`, `setGlobalAxis`, `setLimitPoint`, `addLimitPlane` and the limit-plane
iterator to the revolute cases (two dynamic bodies; then a joint with body 1 missing, and one
with body 0 missing) and compared the staged-pair transcripts. The world anchor and axis, the
local axis and anchor (through `saveToDesc`), `getLimitPoint` and every `getNextLimitPlane`
result matched the oracle word for word. The saved local normal differed only where it is a
tangent from `NxNormalToTangents` (with the test's identity body rotations it equals that
tangent exactly), which is a Foundation defect, not a Joint row: for |axis.z| > 1/sqrt(2)
the rebuilt `Foundation/src/Utilities.cpp` forms `t2.y`/`t2.z` from `n.x * k` where the
oracle's result is `n x t1` (from `n.x`), and on the other arm `t2.x` came out one ulp off in
two of eight cases. The frame quaternions (`mFrameQuat`, `mWorldQuat`) and the solver rows
(004091, 004093, 004111, 004133, 004135, 004391, 004064, 004123) are not observable through
the public API without a simulation step; they are checked by review and the build only.

## Prismatic

Recovered by joint-families Task 3a (the first family after the revolute pilot) from the unit
bundles `units/core__PrismaticJoint.cpp.md` and `units/core__NpPrismaticJoint.cpp.md`, the
Capstone listing, the relocated table words in `oracle/pe.json` and the pinned Ghidra
supplement. 004376 (saveToDesc) and 004386 (the solver slot) had no decompile; Task 3a added
0x000ad4e0 and 0x000ad850 to `oracle/ghidra/supplement.json` (the union with the 11 existing
`requested` RVAs; both `ok`; the 11 existing entries came back unchanged). The listing is
authoritative over both decompiles.

### Row assignment

`work_units.json` puts 6 rows in `core\PrismaticJoint.cpp` (evidenced span 0xad4e0-0xad780,
no ambiguous rows) and 14 in `core\NpPrismaticJoint.cpp` (evidenced span 0xb3430-0xb37b0, no
ambiguous rows). Checked by hand:

- **Before `core\PrismaticJoint.cpp`**: 004374 (revolute slot 0) ends at 0xad4dc; 004376 starts
  at 0xad4e0 and pushes the `PrismaticJoint.cpp` `__FILE__` (0x1011a504).
- **After it**: 004386 (0xad850, 6,772 B) ends at 0xaf2c4, where the `Observable::event` import
  thunk sits; 004387 (fluid trampoline) and 004389-004433 are the
  `gap:core\PrismaticJoint.cpp..core\NpD6Joint.cpp` rows `## Shared rows` already classified.
  004386 is slot 6 of the prismatic internal table (0x11a4e8) and is reached from nowhere
  else, so it is prismatic's.
- **Before `core\NpPrismaticJoint.cpp`**: 004729 (NpRevoluteJoint slot 0) ends at 0xb3427.
- **After it**: the constructor / thunk / deleting-destructor triple 004753 (installs
  0x1011b4b8, `phys_data_002730`), 004755 (`sub ecx,0xc; jmp 004757`, table 0x1011b53c) and
  004757 (slot 0) closes the unit, as in every Np unit. 004759 (0xb38a0) is called by the
  articulation row 004405, not a prismatic row.
- 004743 (slot 30, getName) is in this unit's extent but was claimed by Task 1 in
  `core/NpJointShared.cpp`; it is not written again.

| Stable ID | RVA | Size | File | Evidence |
|---|---|---:|---|---|
| phys_fn_004376 | 0x000ad4e0 | 54 | `core/PrismaticJoint.cpp` | internal slot 10 (0x11a4f8); "PrismaticJoint::saveToDesc" line 0x4d |
| phys_fn_004378 | 0x000ad520 | 443 | `core/PrismaticJoint.cpp` | called by 004380 (0xad725) and 004384 (0xad840); writes +0x16c..+0x178 |
| phys_fn_004380 | 0x000ad6e0 | 81 | `core/PrismaticJoint.cpp` | createJoint case 0 (0x143b8); installs 0x1011a4d0 (0xad6f3) |
| phys_fn_004382 | 0x000ad740 | 56 | `core/PrismaticJoint.cpp` | internal slot 5; reinstalls 0x1011a4d0 (0xad748) |
| phys_fn_004384 | 0x000ad780 | 202 | `core/PrismaticJoint.cpp` | internal slot 9; "PrismaticJoint::loadFromDesc" lines 0x39/0x3a |
| phys_fn_004386 | 0x000ad850 | 6772 | `core/PrismaticJoint.cpp` | internal slot 6 (0x11a4e8) |
| phys_fn_004731 | 0x000b3430 | 84 | `core/NpPrismaticJoint.cpp` | Np slot 2; line 0xf |
| phys_fn_004733 | 0x000b3490 | 84 | `core/NpPrismaticJoint.cpp` | Np slot 4; line 0xf |
| phys_fn_004735 | 0x000b34f0 | 89 | `core/NpPrismaticJoint.cpp` | Np slot 9; line 0xf |
| phys_fn_004737 | 0x000b3550 | 89 | `core/NpPrismaticJoint.cpp` | Np slot 11; line 0xf |
| phys_fn_004739 | 0x000b35b0 | 97 | `core/NpPrismaticJoint.cpp` | Np slot 13; line 0xf |
| phys_fn_004741 | 0x000b3620 | 74 | `core/NpPrismaticJoint.cpp` | Np slot 15; line 0xf |
| phys_fn_004745 | 0x000b36a0 | 88 | `core/NpPrismaticJoint.cpp` | Np slot 29; line 0xf |
| phys_fn_004747 | 0x000b3700 | 74 | `core/NpPrismaticJoint.cpp` | Np slot 14; line 0xf |
| phys_fn_004749 | 0x000b3750 | 84 | `core/NpPrismaticJoint.cpp` | Np slot 31 (loadFromDesc); line 0x13; internal `[vt+0x24]` |
| phys_fn_004751 | 0x000b37b0 | 84 | `core/NpPrismaticJoint.cpp` | Np slot 32 (saveToDesc); line 0x1e; internal `[vt+0x28]` |
| phys_fn_004753 | 0x000b3810 | 57 | `core/NpPrismaticJoint.cpp` | constructor; called by 004380 (0xad710) |
| phys_fn_004755 | 0x000b3850 | 8 | `core/NpPrismaticJoint.cpp` | secondary table 0x1011b53c slot 0; compiler-generated thunk |
| phys_fn_004757 | 0x000b3860 | 55 | `core/NpPrismaticJoint.cpp` | Np slot 0 (scalar deleting destructor) |

### Construction chain (NxJointType 0)

1. 000665's switch (table 0x14590, `cmp eax,9` at 0x1438a) sends type 0 to **0x1439a**: SDK
   allocator slot +8 with `(0x17c, 0)` (`push 0x17c` at 0x143a5), null -> 0x1458a (result 0);
   otherwise `call 0x100ad6e0` = **004380** on the block with the descriptor (0x143b8), then
   the shared exit at 0x144fc (the same code the revolute case jumps to).
2. **004380** PrismaticJoint::PrismaticJoint(const NxPrismaticJointDesc&) (`ret 4`):
   `Joint(desc, 0x80)` (004141; 0x80 -> NxJointType 0 through the byte table, as the pilot
   confirmed), vptr 0x1011a4d0 (0xad6f3), SDK allocator `(0x1c, 0)` -> **004753** on success
   or null, public object -> `this+0x48` (0xad719), `desc.userData` (desc+0x60) -> `np+4`
   **without a null check** (0xad71c-0xad71f), then **004378(desc)** (0xad725; the
   descriptor is pushed and popped by `ret 4` but never read). `NxPrismaticJointDesc` adds no
   field to `NxJointDesc`, so nothing else is loaded.
3. **004753** NpPrismaticJoint::NpPrismaticJoint(PrismaticJoint*): zeroes +4/+8, transient
   table 0x1011b3f8 (`NxPrismaticJoint`), 002404 on +0xc, secondary table 0x1011b53c, internal
   at +0x18 and +0x08, final table 0x1011b4b8. Exactly the revolute shape (004725).
4. Back in 000665 (0x144fc), identical to revolute: `[joint+0x48]` null -> internal slot 5
   with 1 (004382) and result 0; otherwise `[[Scene+0x6cc]+0xc]` -> np+0x10,
   `[[Scene+0x6cc]+0x10]` -> np+0x14, 000661; then `++[Scene+0x6c8]`, `[Scene+0x6bc] =
   [Scene+0x59c]`, re-entry flag cleared (0x14529-0x1453f). 000297 returns `[internal+0x48]`.

**Public-object offset: +0x48**, the same as revolute: it is the Joint base field
`mPublicObject`, which 004380 writes at 0xad719 and 000665 reads at 0x14502 for every case.

### Object layouts

**PrismaticJoint (internal), 0x17c bytes** (`push 0x17c` at 0x143a5):

| Off | Size | Field | Evidence |
|---|---:|---|---|
| +0x000 | 0x16c | `Joint` base; vptr 0x1011a4d0 | 004380 0xad6ee/0xad6f3; 004382 0xad748 |
| +0x16c | 0x10 | `mUnknown16c[4]`: quaternion x, y, z, w = conj(conj(q0) * q1) for the bodies' +0x124 quaternions q0, q1 (004378 conjugates q0 in place, multiplies by q1, then negates the vector part again); the identity stands for a missing body 0, and a missing body 1 leaves q0. 004386 multiplies the current conj(q0) * q1 by it, so it is the inverse of the relative rotation at creation. Name unknown (no string, descriptor field or public virtual) | written only by 004378 (0xad533-0xad6cf); read only by 004386 (0xaeea4-0xaef46) |

The candidate writes the body record's +0x124 quaternion (`nxNpActorUpdateCMassQuaternion`),
so 004378's inputs exist.

**NpPrismaticJoint (public), 0x1c bytes**: exactly `NpJointShared<NxPrismaticJoint,
PrismaticJoint>` (vptr 0x1011b4b8, userData +4, appData +8 = internal, hook base +0xc with
table 0x1011b53c, write link +0x10, read link +0x14, internal +0x18). No own field.

### Dispatch tables

**0x1011a4d0: PrismaticJoint internal (`phys_data_002694`, 13 slots)**

| Slot | Row | Declared as | Notes |
|---:|---|---|---|
| 0 | 004248 (folded, `ret 4`) | `Joint::row_slot0` inline | inherited no-op |
| 1 | 001583 (folded, `ret`) | `Joint::row_slot1` inline | inherited no-op |
| 2 | 004111 | `Joint::row004111` | inherited |
| 3 | 004087 | `Joint::row004087` | inherited |
| 4 | **004318** | `PrismaticJoint::row_slot4(NxDebugRenderable&)` | the folded debug-visualization body owned by `core\CylindricalJoint.cpp` (1,115 B). It reads only Joint base fields (+0x2c bit 9, the body stamps, 004097/004123/004127, SDK parameters through 0x10001000), which is why prismatic and cylindrical share one copy. **Deferred to Task 3b** (see below) |
| 5 | **004382** | `~PrismaticJoint()` (scalar deleting) | deletes `[this+0x48]` through its slot 0 with 1, 004095, frees if flag&1 |
| 6 | **004386** | `PrismaticJoint::row_slot6(NxReal)` | `ret 4`; the float argument is a divisor (`fld 1.0; fdiv [esp+0xc8]`, 0xae3aa) |
| 7 | 004135 | `Joint::row_slot7` | inherited |
| 8 | 004248 (folded) | `Joint::row_slot8` inline | inherited no-op |
| 9 | **004384** | `PrismaticJoint::loadFromDesc(const NxPrismaticJointDesc&)` | `ret 4` |
| 10 | **004376** | `PrismaticJoint::saveToDesc(NxPrismaticJointDesc&)` | `ret 4`; tail-jumps 004066 |
| 11 | 001391 (folded, `mov eax,ecx; ret`) | `PrismaticJoint::row_slot11()` inline | returns `this` |
| 12 | 001391 (folded) | `PrismaticJoint::row_slot12()` inline | returns `this` |

The revolute table has 17 slots (setFlags/getFlags/setProjectionMode/getProjectionMode at
11-14 before its two `return this` slots); the prismatic class has none of those, so its two
`return this` slots are 11 and 12. Each family declares its own slots 9 onwards in its own
header, not in `Joint`.

**0x1011b4b8: NpPrismaticJoint primary (`phys_data_002730`, 33 slots)**: slots 0-32 as the
`### Slot split` table above (per-family rows 004757, 004731, 004733, 004735, 004737, 004739,
004747, 004741, 004745, 004749, 004751; the folded rows through `NpJointShared`). Every
write-locked NxJoint row reports line 0xf; loadFromDesc 0x13, saveToDesc 0x1e. Slots 31/32
call internal slots 9/10 (`[vt+0x24]`/`[vt+0x28]`). **0x1011b53c** (secondary): 004755.

### Dependency closure

- **write** (19 rows, 8,610 B): 004376, 004378, 004380, 004382, 004384, 004386 in
  `core/PrismaticJoint.cpp` (7,608 B); 004731, 004733, 004735, 004737, 004739, 004741, 004745,
  004747, 004749, 004751, 004753, 004755 (generated thunk; stable-ID line above the
  destructor it serves, as 004727), 004757 in `core/NpPrismaticJoint.cpp` (1,002 B).
- **reuse**: Joint rows 004141, 004107, 004121, 004097, 004066, 004095, 004093, 004111,
  004087, 004135 (`core/Joint.cpp`) and 004391 (`core/JointSupport.cpp`); the 13 folded Np
  bodies (`NpJointShared`); 002362/002364/002366 (`nxNpSceneGuard*`), 002404/002406
  (`EmbeddedHookBase`), 000454/000480 (`nxGet/SetSdkPointerBinding`); 004248, 001583, 001391
  (inline bodies); 004417-004433, 005667 (generated from `NxJoint.h`); `NxNormalToTangents`
  (Foundation import `[0x1010418c]`, cdecl, called three times by 004386); the SDK
  allocator; SDK parameter 0 (`.data 0x10123b18`, read by 004386 through
  `PhysicsSDK::getParameter` as the revolute rows do).
- **New shared data**: the three unit vectors at `.data 0x10122054`, 0x10122060, 0x1012206c
  (`phys_data_003036`, 003039, 003042; (1,0,0), (0,1,0), (0,0,1) in the image). 004386 copies
  them into its three angular records (0xaf065-0xaf217); the fixed, spherical and D6 rows
  (0xa05ab-0xa46bd) multiply by them. They are read from memory, so they are non-const
  data rather than immediates: declared once as `gJointUnitAxis[3]` in `core/JointSupport.h`
  and defined in `core/JointSupport.cpp` for every family to use.
- **defer**: **004318** (slot 4, debug visualization), a `core\CylindricalJoint.cpp` row
  the prismatic table borrows. Task 3a keeps an asserting body for
  `PrismaticJoint::row_slot4` whose comment names the folded row without the stable-ID form,
  so the row is not claimed twice; **Task 3b** writes 004318 once, as a Joint-level body both
  families call (it reads only Joint base fields), and replaces the prismatic stub with a
  call to it.

### What the new test case reaches

`nxPrismaticCase` (two cases: indices 0 and 3 of the revolute table's anchor/axis values).
Creation: 000297, 000665 case 0, 004380, 004141 (-> 004107, 004121 -> 004097 x2, 000480),
004753 (002404), 004378 (both bodies present: the full quaternion product), 000661 (hole).
Getters: 004437/004125, 004441/004129, 004483/004078, 004539, 004443/004070,
`isPrismaticJoint` (inline 004423 -> 004479/004070), and saveToDesc 004751 -> internal slot
10 = **004376** -> 004066. Rows compiled but not reached: 004382 (release unwired), 004384,
004386, 004731-004747, 004757, 004318.
