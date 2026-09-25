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

- **write** (19 rows, 8,575 B): 004376, 004378, 004380, 004382, 004384, 004386 in
  `core/PrismaticJoint.cpp` (7,608 B); 004731, 004733, 004735, 004737, 004739, 004741, 004745,
  004747, 004749, 004751, 004753, 004755 (generated thunk; stable-ID line above the
  destructor it serves, as 004727), 004757 in `core/NpPrismaticJoint.cpp` (967 B).
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

### Result (Task 3a)

- Wired: `NxSceneInternal::createJoint` builds type 0 through `PrismaticJoint` (0x17c) and
  `nxPrismaticJointAttachScene`, sharing one block with the revolute case (allocation per
  family, then the common +0x48 test, link copy, `nxSceneAddJoint`, slot-5 delete and the
  0x14529-0x1453f exit). `nxJointSizeForType` is now reached only by types 2-9.
- The staged pair matched the oracle on the first run (`stdout_delta=0`); no transcript
  difference was found. Both prismatic cases' saved local normals matched as well: the
  Foundation `NxNormalToTangents` defect is on the |n.z| > 1/sqrt(2) arm, which neither
  index 0 nor index 3 reaches.
- A cdb trace of the candidate (`evidence/joint-families-trace-prismatic.txt`) shows 004380,
  004753, 004378, 004751 and 004376 executing in both cases; 004382, 004384, 004386 and the
  Np setters are compiled but not reached.

### Template notes for Tasks 3b-3i

- **Supplement**: pass the union of the existing `requested` RVAs plus the new ones; the
  existing entries come back byte-identical. The Ghidra project was not locked.
- **Internal tables differ per family** (prismatic 13 slots, revolute 17): declare each
  family's own slots (9 onwards) in its own header; `Joint` keeps slots 0-8 only.
- **Rows folded across families**: a family table may name another family's body (prismatic
  slot 4 = cylindrical 004318). Keep an asserting override whose comment names the folded row
  without the `// phys_fn_` form, and leave the row to its owning unit's task. The owner writes
  the body once where both can call it (Task 3b: the non-virtual `Joint::row004318`, declared
  in `core/Joint.h`, defined with its stable-ID line in the owner's file) and turns the
  borrower's stub into a call.
- **Scene wiring**: add the family to the dispatch block at the top of the reconstructed
  path in `Scene.cpp` (allocation case plus attach-helper case); each family needs its own
  `nx<Family>JointAttachScene` because `Scene.cpp` cannot include the Np headers.
- **NpJointShared**: add the explicit instantiation and the two includes in
  `core/NpJointShared.cpp`. The folded bodies then exist once per family in the candidate,
  so a trace breaks on the family's instantiation.
- **Shared data and helpers**: the unit axes are `gJointUnitAxis` (`core/JointSupport.h`).
  The record-bit and solve-tail helpers are file-static copies in `core/RevoluteJoint.cpp`
  and `core/PrismaticJoint.cpp`; the linear records here use a different bit sequence
  (kind 1, bit 10 computed) from revolute's (bit 10 forced), so read each site's listing
  rather than reusing a copy blindly. Task 3b hoisted the kind-1 linear record, the solve tail
  and the lever-pair error into `core/JointLinearRecords.h` (`jointLinearRecord`,
  `jointSolveRecord`, `jointLinearError`); prismatic and cylindrical use them. The three-term
  sums around them are NOT shared: cylindrical 004326 repeats prismatic's algorithm but groups
  about a dozen sums differently, so each family's sums come from its own listing.
- **Internal table length**: count the relocated words up to the unit's `__FILE__` string. The
  cylindrical table has three `return this` slots (11-13), one more than prismatic.
- **Gap rows next to a family unit** can be the tail of the neighbour's function (004314 is
  004312's loop and epilogue: same frame size, `ret 4`, entered by a `jmp` from 004312's last
  instruction). Check the frame and the entry before assigning such a row.
- **cdb**: pass absolute paths (`cygpath -aw`) for the script, the executable and the pair
  directory; the harness rejects a relative pair directory ("FAIL pair directory is not an
  existing absolute canonical path"). A HIT line can be printed on the same line as program
  output; match `HIT .*$` anywhere in the line.
- **Ledger**: move each reconstructed row to `reconstructed_not_falsified` with the file's
  standard note and recount from the entries (the counts are the per-reason entry counts).
- **Floors**: the Phase 7 floor counts the staged-pair lines too; raise `'7'` with `'6'`.
- **Environment**: bash heredocs in this environment collapse `\\` sequences (a `"\\n"` in a
  heredoc'd Python script became a real newline); write helper scripts with the Write tool.
  cdb needs the pair directory as a backslash Windows path (`cygpath -w`).
- **A gap row that is a function tail** (spherical 004314, 004312's loop and epilogue): write
  it inside the owning function and stack its stable-ID line above the owner's with a line
  saying it is not a function (the generated-thunk form); record it `reconstructed` with the
  owner's file as `implementation` (the validator only needs the file to name the row).
- **Np slots that name another family's row** (spherical 34/36 = revolute 004703/004707): the
  family class declares the virtual itself with the same body and a comment naming the folded
  row without the `// phys_fn_` form. Such bodies work only when the internal tables put the
  called slot at the same index (spherical and revolute both have the flags/projection-mode
  quartet at 11-14).
- **Same instructions, other offsets**: spherical 004294 is revolute 004358 and 004308 is
  revolute 004374 (plus one scaling block) at other field offsets; 004296 repeats revolute
  004360's effective-mass build but groups the lever and inertia sums differently. A normalised
  listing diff (addresses stripped) against the revolute row finds these quickly; the sums still
  come from the family's own listing.
- **acos**: `revoluteCIacos`/`revoluteAcos` now live in `core/JointAcos.h` as
  `jointCIacos`/`jointAcos`; the spherical, and any later family's, acos sites use them.
- **MSVC evaluates printf arguments right to left**: a transcript line that calls two getters
  in its argument list calls the later one first (the spherical trace shows getProjectionMode
  before getFlags). Harmless for the transcript, but read the trace order with it in mind.
- **Check the `/arch:IA32` list itself, not only its comment**: Task 3c named
  `core/SphericalJoint.cpp` in the CMake comment but not in the `set_source_files_properties`
  list; Task 3d added it (and `core/PointOnLineJoint.cpp`). The joint transcript did not change.
- **A slot that calls itself** (point-on-line slot 11, 004270: `mov eax,[ecx]; jmp [eax+0x2c]`)
  is written as the virtual self-call `return row_slot11();`, which MSVC compiles to the same five
  bytes; check the candidate bytes through the map (`?row_slot11@...`).
- **A family class with no field of its own** (point-on-line, 0x16c = `sizeof(Joint)`): the header
  asserts only the size; the contract lists the Joint base fields the rows use instead.
- **Point-on-line and point-in-plane** share no row (see `## PointOnLine` "### Point-in-plane"
  for the listing differences Task 3e will meet).

## Cylindrical

Recovered by joint-families Task 3b from the unit bundles `units/core__CylindricalJoint.cpp.md`,
`units/core__NpCylindricalJoint.cpp.md` and
`units/gap__core__SphericalJoint.cpp__to__core__CylindricalJoint.cpp.md`, the Capstone listing,
the relocated table words in `oracle/pe.json` and the pinned Ghidra supplement. 004316
(saveToDesc) and 004326 (the solver slot) had no decompile; Task 3b added 0x000a7200 and
0x000a7810 to `oracle/ghidra/supplement.json` (the union with the 13 existing `requested`
RVAs; both `ok`; the 13 existing entries came back unchanged). The listing is authoritative
over both decompiles.

### Row assignment

`work_units.json` puts 6 rows in `core\CylindricalJoint.cpp` (evidenced span 0xa7200-0xa7740,
no ambiguous rows) and 13 in `core\NpCylindricalJoint.cpp` (evidenced span 0xb28d0-0xb2c20, no
ambiguous rows). Checked by hand:

- **Before `core\CylindricalJoint.cpp`**: the five rows of
  `gap:core\SphericalJoint.cpp..core\CylindricalJoint.cpp` (004306-004314, 0xa4ac0-0xa71fb,
  9,445 B) are all **spherical**; none is cylindrical, so this task writes none of them and
  leaves all five to Task 3c. Evidence, row by row:
  - 004308 (0xa4f00) is slot 0, 004312 (0xa5ee0) slot 4 and 004310 (0xa5360) slot 7 of the
    spherical internal table 0x10119e20 (`phys_data_002668`, installed by the spherical
    constructor 004300; the only caller of all three is that constructor's table install).
  - 004306 (0xa4ac0) is called only by 004310 and 004312 (the listing's two direct calls).
  - 004314 (0xa7050, 430 B, no callers) is not a function: it is the loop body and epilogue
    of 004312. 004312 opens with `sub esp,0xe8` (0xa5ee0) and its last instruction is
    `jmp 0x100a7050` (0xa704b); 004314 loops back to itself (`jbe 0x100a7050`, 0xa71eb) and
    ends with `add esp,0xe8; ret 4` (0xa71f5), 004312's frame and stack purge.
  - No cylindrical row calls any of them (the cylindrical bundles' dependency lists name none).
- **`core\CylindricalJoint.cpp`**: 004316 starts at 0xa7200 right after 004314's `ret 4` and
  pushes the `CylindricalJoint.cpp` `__FILE__` (0x1011a080); 004326 (0xa7810, 5,377 B) ends at
  0xa8d0e, and the next row 004328 (0xa8d20) is revolute's (revolute contract).
- **`core\NpCylindricalJoint.cpp`**: 004653 (NpSphericalJoint slot 0) ends at 0xb28c4; 004655
  starts at 0xb28d0 and pushes the `NpCylindricalJoint.cpp` `__FILE__` (0x1011b15c). The
  constructor / thunk / deleting-destructor triple 004675 (installs 0x1011b198,
  `phys_data_002724`), 004677 (`sub ecx,0xc; jmp 004679`, table 0x1011b21c) and 004679 (slot
  0) closes the unit; 004681 (0xb2d10) is NpRevoluteJoint's.

| Stable ID | RVA | Size | File | Evidence |
|---|---|---:|---|---|
| phys_fn_004316 | 0x000a7200 | 54 | `core/CylindricalJoint.cpp` | internal slot 10 (0x11a070); line 0x3b; the string reads "CylindricalJoint::loadFromDesc: ... can't be saved!" (the oracle's own text) |
| phys_fn_004318 | 0x000a7240 | 1115 | `core/CylindricalJoint.cpp` | internal slot 4 of the cylindrical (0x11a058) **and** prismatic (0x11a4e0) tables |
| phys_fn_004320 | 0x000a76a0 | 87 | `core/CylindricalJoint.cpp` | createJoint case 2 (0x1440a); installs 0x1011a048 (0xa76b3) |
| phys_fn_004322 | 0x000a7700 | 56 | `core/CylindricalJoint.cpp` | internal slot 5; reinstalls 0x1011a048 (0xa7708) |
| phys_fn_004324 | 0x000a7740 | 194 | `core/CylindricalJoint.cpp` | internal slot 9; "CylindricalJoint::loadFromDesc" lines 0x26/0x27 |
| phys_fn_004326 | 0x000a7810 | 5377 | `core/CylindricalJoint.cpp` | internal slot 6 (0x11a060) |
| phys_fn_004655 | 0x000b28d0 | 84 | `core/NpCylindricalJoint.cpp` | Np slot 2; line 0x10 |
| phys_fn_004657 | 0x000b2930 | 84 | `core/NpCylindricalJoint.cpp` | Np slot 4; line 0x10 |
| phys_fn_004659 | 0x000b2990 | 89 | `core/NpCylindricalJoint.cpp` | Np slot 9; line 0x10 |
| phys_fn_004661 | 0x000b29f0 | 89 | `core/NpCylindricalJoint.cpp` | Np slot 11; line 0x10 |
| phys_fn_004663 | 0x000b2a50 | 97 | `core/NpCylindricalJoint.cpp` | Np slot 13; line 0x10 |
| phys_fn_004665 | 0x000b2ac0 | 74 | `core/NpCylindricalJoint.cpp` | Np slot 15; line 0x10 |
| phys_fn_004667 | 0x000b2b10 | 88 | `core/NpCylindricalJoint.cpp` | Np slot 29; line 0x10 |
| phys_fn_004669 | 0x000b2b70 | 74 | `core/NpCylindricalJoint.cpp` | Np slot 14; line 0x10 |
| phys_fn_004671 | 0x000b2bc0 | 84 | `core/NpCylindricalJoint.cpp` | Np slot 31 (loadFromDesc); line 0x15; internal `[vt+0x24]` |
| phys_fn_004673 | 0x000b2c20 | 84 | `core/NpCylindricalJoint.cpp` | Np slot 32 (saveToDesc); line 0x20; internal `[vt+0x28]` |
| phys_fn_004675 | 0x000b2c80 | 57 | `core/NpCylindricalJoint.cpp` | constructor; called by 004320 (0xa76d0) |
| phys_fn_004677 | 0x000b2cc0 | 8 | `core/NpCylindricalJoint.cpp` | secondary table 0x1011b21c slot 0; compiler-generated thunk |
| phys_fn_004679 | 0x000b2cd0 | 55 | `core/NpCylindricalJoint.cpp` | Np slot 0 (scalar deleting destructor) |

004665, 004669, 004671 and 004673 are already `reconstructed` through `ObjectModel.cpp`
differentials (the tailjmp, mutexlistfree and mutexfamily models); their proofs are kept and
the models gain a `// Product row:` pointer.

### Construction chain (NxJointType 2)

1. 000665's switch (table 0x14590) sends type 2 to **0x143eb**: SDK allocator slot +8 with
   `(0x16c, 0)` (`push 0x16c` at 0x143f7), null -> 0x1458a (result 0); otherwise
   `call 0x100a76a0` = **004320** on the block with the descriptor (0x1440a), then the shared
   exit at 0x144fc.
2. **004320** CylindricalJoint::CylindricalJoint(const NxCylindricalJointDesc&) (`ret 4`):
   `Joint(desc, 0x100)` (004141, `push 0x100` at 0xa76a6), vptr 0x1011a048 (0xa76b3), SDK
   allocator `(0x1c, 0)` -> **004675** on success, public object -> `this+0x48` (0xa76d5),
   `desc.userData` (desc+0x60) -> `np+4` **without a null check** (the null arm stores 0 at
   +0x48 and then writes `[0+4]`, 0xa76e5-0xa76ed). Nothing else: `NxCylindricalJointDesc`
   adds no field and the class has none of its own, so there is no 004378-style call.
3. **004675** NpCylindricalJoint::NpCylindricalJoint(CylindricalJoint*): zeroes +4/+8,
   transient table 0x1011b0d8 (`NxCylindricalJoint`), 002404 on +0xc, secondary table
   0x1011b21c, internal at +0x18 and +0x08, final table 0x1011b198. The prismatic shape.
4. Back in 000665 (0x144fc): identical to revolute and prismatic (`[joint+0x48]` null ->
   internal slot 5 with 1 (004322) and result 0; otherwise the link copy, 000661, and the
   0x14529-0x1453f exit).

**Public-object offset: +0x48** (the Joint base field `mPublicObject`; 004320 writes it at
0xa76d5, 000665 reads it at 0x14502).

### Object layouts

**CylindricalJoint (internal), 0x16c bytes** (`push 0x16c` at 0x143f7) = `sizeof(Joint)`: the
`Joint` base with vptr 0x1011a048 and no field of its own.

**NpCylindricalJoint (public), 0x1c bytes**: exactly `NpJointShared<NxCylindricalJoint,
CylindricalJoint>` (vptr 0x1011b198, userData +4, appData +8 = internal, hook base +0xc with
table 0x1011b21c, write link +0x10, read link +0x14, internal +0x18). No own field.

### Dispatch tables

**0x1011a048: CylindricalJoint internal (`phys_data_002678`, 14 slots)**

| Slot | Row | Declared as | Notes |
|---:|---|---|---|
| 0 | 004248 (folded, `ret 4`) | `Joint::row_slot0` inline | inherited no-op |
| 1 | 001583 (folded, `ret`) | `Joint::row_slot1` inline | inherited no-op |
| 2 | 004111 | `Joint::row004111` | inherited |
| 3 | 004087 | `Joint::row004087` | inherited |
| 4 | **004318** | `CylindricalJoint::row_slot4(NxDebugRenderable&)` | calls `Joint::row004318` (below) |
| 5 | **004322** | `~CylindricalJoint()` (scalar deleting) | deletes `[this+0x48]` through its slot 0 with 1, 004095, frees if flag&1 |
| 6 | **004326** | `CylindricalJoint::row_slot6(NxReal)` | `ret 4`; the float is a divisor (`fld 1.0; fdiv [esp+0xb8]`, 0xa832b) |
| 7 | 004135 | `Joint::row_slot7` | inherited |
| 8 | 004248 (folded) | `Joint::row_slot8` inline | inherited no-op |
| 9 | **004324** | `CylindricalJoint::loadFromDesc(const NxCylindricalJointDesc&)` | `ret 4` |
| 10 | **004316** | `CylindricalJoint::saveToDesc(NxCylindricalJointDesc&)` | `ret 4`; tail-jumps 004066 |
| 11 | 001391 (folded, `mov eax,ecx; ret`) | `CylindricalJoint::row_slot11()` inline | returns `this` |
| 12 | 001391 (folded) | `CylindricalJoint::row_slot12()` inline | returns `this` |
| 13 | 001391 (folded) | `CylindricalJoint::row_slot13()` inline | returns `this` |

The table runs to 0x1011a080, where the unit's `__FILE__` string starts; the relocated words
at 0x11a074, 0x11a078 and 0x11a07c all name 001391, and nothing in the listing addresses
0x1011a07c on its own. So the cylindrical class has one more `return this` virtual than the
prismatic one (13 slots). The meaning of the third is unknown; it is declared so the
candidate's table has the oracle's slot count.

**0x1011b198: NpCylindricalJoint primary (`phys_data_002724`, 33 slots)**: slots 0-32 as the
`### Slot split` table (per-family rows 004679, 004655, 004657, 004659, 004661, 004663, 004669,
004665, 004667, 004671, 004673; the folded rows through `NpJointShared`). Every write-locked
NxJoint row reports line 0x10; loadFromDesc 0x15, saveToDesc 0x20. Slots 31/32 call internal
slots 9/10 (`[vt+0x24]`/`[vt+0x28]`). **0x1011b21c** (secondary): 004677.

### 004318: one body for two families

004318 reads only Joint base fields (+0x2c bit 9, the body stamps, the world anchors
+0x114/+0x120 and axes +0xfc/+0x108, the bodies' +0x134/+0x158 poses), calls 004097, 004123
and 004127, and reads SDK parameters 31, 32 and 13 (`.data 0x10123b94`, 0x10123b98,
0x10123b4c). It is written once as the non-virtual `Joint::row004318(NxDebugRenderable&)`
(declared in `core/Joint.h`, defined with its stable-ID line in `core/CylindricalJoint.cpp`),
and both `CylindricalJoint::row_slot4` and `PrismaticJoint::row_slot4` call it; the prismatic
asserting stub from Task 3a is replaced. What it draws, when bit 9 (NX_JF_VISUALIZATION) is
set, after the stale-body refresh:

- world axes (parameter 32 non-zero): `addArrow(row004123 point, row004127 axis, 1,
  scale13 * p32, 0xffffff)`, as revolute 004364;
- local axes (parameter 31 non-zero): each body's world anchor and axis carried through its
  +0x134/+0x158 pose (as stored without a body), then `addArrow(anchor0, axis0, 1, p31 *
  scale13, 0x202090)` and `addArrow(anchor1, axis1, 1, same, 0x5050e0)`. It builds four
  two-element NxVec3 arrays through the `eh vector constructor iterator` (000001, with the
  folded NxVec3 constructor 001391) and uses two of them (anchors, axes).

Listing over decompile: the decompile passes the second arrow `(&fStack_50, auStack_68)`;
the listing pushes body 1's anchor ([esp+0x4c]) and axis ([esp+0x34]) (0xa7683-0xa768c).

### 004326: the solver slot

The first four records of prismatic 004386 and nothing else: the same stale-body refresh,
slide direction n, tangents t1/t2 (NxNormalToTangents), the two lever pairs (anchors, then
points one axis length along; with both bodies the levers towards C0 and C1) and the four
kind-1 linear records (t1, t2, t1, t2) with bias `(t . error) / arg` and +0x48 = maxForce.
There is no angular record and no quaternion, which is what lets the joint turn about its
axis. The record header and bit sequence, the error and the solve tail are the same
instructions as prismatic's; the three shared helpers move into
`core/JointLinearRecords.h` (below). The three-term groupings differ from prismatic's at
many sites (the listing is followed; each site is named in the row comment). Listing over
decompile (supplement): the decompile drops the kind tests as unreachable (0x100a8458,
0x100a85cf, 0x100a8ae2, 0x100a8c59), shows every intermediate as a float and regroups most
sums.

### Dependency closure

- **write** (19 rows, 7,850 B): 004316, 004318, 004320, 004322, 004324, 004326 in
  `core/CylindricalJoint.cpp` (6,883 B); 004655, 004657, 004659, 004661, 004663, 004665,
  004667, 004669, 004671, 004673, 004675, 004677 (generated thunk; stable-ID line above the
  destructor it serves), 004679 in `core/NpCylindricalJoint.cpp` (967 B).
- **reuse**: Joint rows 004141, 004107, 004121, 004097, 004066, 004095, 004093, 004111,
  004087, 004135, 004123, 004127 (`core/Joint.cpp`) and 004391 (`core/JointSupport.cpp`); the
  13 folded Np bodies (`NpJointShared`); 002362/002364/002366, 002404/002406, 000454/000480;
  004248, 001583, 001391 (inline bodies); 000001 (the compiler's `eh vector constructor
  iterator`); 004417-004433, 005667; `NxNormalToTangents` (Foundation import `[0x1010418c]`,
  called three times by 004326); the SDK allocator; SDK parameters 0, 13, 31, 32.
- **Shared helpers moved**: `prismaticLinearRecord`, `prismaticSolveRecord` and
  `prismaticError` (file-static in `core/PrismaticJoint.cpp` since Task 3a) become
  `jointLinearRecord`, `jointSolveRecord` and `jointLinearError` in the new internal header
  `Physics/src/include/core/JointLinearRecords.h`, used by both families. They are the same
  instructions in both listings (record and tail: prismatic 0xae418-0xae577, cylindrical
  0xa83a3-0xa84f7, and their copies; error: 0xae31a-0xae3a4 / 0xa829b-0xa832b).
- **defer**: none. The five gap rows 004306-004314 are spherical's (Task 3c).

### What the new test case reaches

`nxCylindricalCase` (indices 0 and 3 of the revolute table's anchor/axis values). Creation:
000297, 000665 case 2, 004320, 004141 (-> 004107, 004121 -> 004097 x2, 000480), 004675
(002404), 000661. Getters: 004437/004125, 004441/004129, 004483/004078, 004539, 004443/004070,
`isCylindricalJoint` (inline 004425 -> 004479/004070), and saveToDesc 004673 -> internal slot
10 = **004316** -> 004066. Compiled but not reached: 004318 (no debug render), 004322
(release unwired), 004324, 004326 (no simulation step), 004655-004671, 004679.

### Result (Task 3b)

- Wired: `NxSceneInternal::createJoint` builds type 2 through `CylindricalJoint` (0x16c) and
  `nxCylindricalJointAttachScene` in the same block as prismatic and revolute.
  `nxJointSizeForType` is now reached only by types 3-9.
- The staged pair matched the oracle on the first run (`stdout_delta=0`, 27/27 Phase 6
  coverage, 12/12 Phase 7); no transcript difference was found. Registered lines (four per
  joint list) were copied from the oracle side of that run: the oracle-differential section
  of the Phase 6 log for `NxPhysicsJointTests` and the `pair=oracle` child output for
  `NxPhysicsJointStagedPairTests` (the two are identical for the cylindrical lines).
- A cdb trace of the candidate (`evidence/joint-families-trace-cylindrical.txt`) shows 004320,
  004675, 004673 and 004316 executing in both cases; 004318, 004322, 004324, 004326 and the Np
  setters are compiled but not reached. 004318 and 004326 are checked against the listing by
  review and the build only.
- 004318 replaced prismatic's asserting slot-4 stub, and the prismatic solver now uses the
  shared `core/JointLinearRecords.h` helpers; the prismatic transcript lines are unchanged.
- Ledger: the 19 rows are `reconstructed_not_falsified` in `gates/phase6-closure.json`
  (15 moved from `not_reconstructed_in_phase`; 004665, 004669, 004671 and 004673 already were);
  counts 239 / 192. Task 3a did not move the prismatic rows in the ledger (004376-004386 and
  004731-004757 are still `not_reconstructed_in_phase` there); left for the controller.

## Spherical

Recovered by joint-families Task 3c from the unit bundles `units/core__SphericalJoint.cpp.md`,
`units/core__NpSphericalJoint.cpp.md` and
`units/gap__core__SphericalJoint.cpp__to__core__CylindricalJoint.cpp.md`, the Capstone listing,
the relocated table words in `oracle/pe.json` and the pinned Ghidra supplement. 004296 (the
solver slot), 004304 (loadFromDesc) and 004312 (debug visualization) had no decompile; Task 3c
added 0x000a3090, 0x000a4a00 and 0x000a5ee0 to `oracle/ghidra/supplement.json` (the union with
the 15 existing `requested` RVAs; all three `ok`; the 15 existing entries came back unchanged).
Ghidra's body for 0x000a5ee0 is two ranges, 0xa5ee0-0xa704d and 0xa7050-0xa71fe: it takes
004314 as part of 004312 (below). The listing is authoritative over every decompile.

### Row assignment

`work_units.json` puts 12 rows in `core\SphericalJoint.cpp` (evidenced span 0xa2dc0-0xa4a00, no
ambiguous rows), 16 in `core\NpSphericalJoint.cpp` (evidenced span 0xb2390-0xb27e0, no ambiguous
rows) and 5 ambiguous rows in `gap:core\SphericalJoint.cpp..core\CylindricalJoint.cpp`. Checked
by hand:

- **Before `core\SphericalJoint.cpp`**: the unit starts at 004282 (0xa2bf0), before the evidenced
  span. The row before it ends with `ret 4` at 0xa2bdf.
  004282 (slot 1 of the spherical internal table, zeroes +0x1f0..+0x1f8) and 004284 (called only
  by the spherical constructor 004300 and loadFromDesc 004304) are spherical.
- **The gap `core\SphericalJoint.cpp..core\CylindricalJoint.cpp`** (004306-004314, 0xa4ac0-0xa71fb)
  is spherical, as Task 3b found: 004308, 004312 and 004310 are slots 0, 4 and 7 of the spherical
  internal table 0x10119e20; 004306 is called only by 004310 (0xa53af) and 004312 (0xa67d0); and
  004314 is not a function but 004312's tail. 004312 opens `sub esp,0xe8` (0xa5ee0) and its last
  instruction is `jmp 0x100a7050` (0xa704b); 004314 (0xa7050, 430 B, no callers) loops back to
  itself (`jbe 0x100a7050`, 0xa71eb) and ends `pop ebp; pop edi; pop ebx; pop esi; add esp,0xe8;
  ret 4` (0xa71f1-0xa71fb), which pops the registers 004312 pushed and purges its frame and its
  one argument. It is written as part of 004312's body; its stable-ID line is stacked above
  004312's with a comment saying it is not a function (the form the generated adjustor thunks
  004755/004677 use), so the file names the row and the validator's implementation check holds.
  It is recorded `reconstructed` with `implementation` = `core/SphericalJoint.cpp` and a note
  that it is 004312's tail.
- **After the gap**: 004316 (0xa7200) is cylindrical.
- **`core\NpSphericalJoint.cpp`**: 004621 (NpPointOnLineJoint slot 0) ends `ret 4` at 0xb2384;
  004623 starts at 0xb2390 and pushes the `NpSphericalJoint.cpp` `__FILE__` (0x1011afec). The
  constructor / thunk / deleting-destructor triple 004649 (installs 0x1011b028,
  `phys_data_002721`), 004651 (`sub ecx,0xc; jmp 004653`, table 0x1011b0bc) and 004653 (slot 0)
  closes the unit; 004655 (0xb28d0) is NpCylindricalJoint's.
- 004635 (slot 17, getNextLimitPlane) is in this unit's extent but was claimed by Task 1 in
  `core/NpJointShared.cpp`; it is not written again. Slots 34 and 36 name revolute's 004703 and
  004707 (below); they get no stable-ID line here.

| Stable ID | RVA | Size | File | Evidence |
|---|---|---:|---|---|
| phys_fn_004282 | 0x000a2bf0 | 21 | `core/SphericalJoint.cpp` | internal slot 1 (0x119e24); zeroes +0x1f0..+0x1f8 |
| phys_fn_004284 | 0x000a2c10 | 427 | `core/SphericalJoint.cpp` | called by 004300 (0xa49a5) and 004304 (0xa4ab0); desc+0x6c..+0xc8 -> +0x16c..+0x1d4, +0x44 |
| phys_fn_004286 | 0x000a2dc0 | 283 | `core/SphericalJoint.cpp` | internal slot 10 (0x119e48); "SphericalJoint::saveToDesc" line 0x70 |
| phys_fn_004288 | 0x000a2ee0 | 65 | `core/SphericalJoint.cpp` | internal slot 11; "SphericalJoint::setFlags" line 0x83 |
| phys_fn_004290 | 0x000a2f30 | 7 | `core/SphericalJoint.cpp` | internal slot 12; returns +0x1d0 |
| phys_fn_004292 | 0x000a2f40 | 62 | `core/SphericalJoint.cpp` | internal slot 13; "SphericalJoint::setProjectionMode" line 0x8e |
| phys_fn_004294 | 0x000a2f80 | 269 | `core/SphericalJoint.cpp` | called by 004308 (0xa4f1a); revolute 004358's instructions over +0x1d8/+0x1e4 |
| phys_fn_004296 | 0x000a3090 | 5907 | `core/SphericalJoint.cpp` | internal slot 6 (0x119e38) |
| phys_fn_004298 | 0x000a47b0 | 317 | `core/SphericalJoint.cpp` | internal slot 8 (0x119e40); the projection (anchor distance only) |
| phys_fn_004300 | 0x000a48f0 | 194 | `core/SphericalJoint.cpp` | createJoint case 3 (0x14433); installs 0x10119e20 (0xa4901) |
| phys_fn_004302 | 0x000a49c0 | 56 | `core/SphericalJoint.cpp` | internal slot 5; reinstalls 0x10119e20 (0xa49c8) |
| phys_fn_004304 | 0x000a4a00 | 186 | `core/SphericalJoint.cpp` | internal slot 9; "SphericalJoint::loadFromDesc" lines 0x39/0x3a |
| phys_fn_004306 | 0x000a4ac0 | 1075 | `core/SphericalJoint.cpp` | called by 004310 and 004312; `ret 8`, returns st(0) (the twist angle) |
| phys_fn_004308 | 0x000a4f00 | 1107 | `core/SphericalJoint.cpp` | internal slot 0 (0x119e20) |
| phys_fn_004310 | 0x000a5360 | 2942 | `core/SphericalJoint.cpp` | internal slot 7 (0x119e3c) |
| phys_fn_004312 | 0x000a5ee0 | 4461 | `core/SphericalJoint.cpp` | internal slot 4 (0x119e30) |
| phys_fn_004314 | 0x000a7050 | 430 | `core/SphericalJoint.cpp` | 004312's swing-limit loop and epilogue (not a function) |
| phys_fn_004623 | 0x000b2390 | 84 | `core/NpSphericalJoint.cpp` | Np slot 2; line 0xf |
| phys_fn_004625 | 0x000b23f0 | 84 | `core/NpSphericalJoint.cpp` | Np slot 4; line 0xf |
| phys_fn_004627 | 0x000b2450 | 89 | `core/NpSphericalJoint.cpp` | Np slot 9; line 0xf |
| phys_fn_004629 | 0x000b24b0 | 89 | `core/NpSphericalJoint.cpp` | Np slot 11; line 0xf |
| phys_fn_004631 | 0x000b2510 | 97 | `core/NpSphericalJoint.cpp` | Np slot 13; line 0xf |
| phys_fn_004633 | 0x000b2580 | 74 | `core/NpSphericalJoint.cpp` | Np slot 15; line 0xf |
| phys_fn_004637 | 0x000b2610 | 88 | `core/NpSphericalJoint.cpp` | Np slot 29; line 0xf |
| phys_fn_004639 | 0x000b2670 | 74 | `core/NpSphericalJoint.cpp` | Np slot 14; line 0xf |
| phys_fn_004641 | 0x000b26c0 | 84 | `core/NpSphericalJoint.cpp` | Np slot 31 (loadFromDesc); line 0x13; internal `[vt+0x24]` |
| phys_fn_004643 | 0x000b2720 | 84 | `core/NpSphericalJoint.cpp` | Np slot 32 (saveToDesc); line 0x1e; internal `[vt+0x28]` |
| phys_fn_004645 | 0x000b2780 | 84 | `core/NpSphericalJoint.cpp` | Np slot 33 (setFlags); line 0x27; internal `[vt+0x2c]` |
| phys_fn_004647 | 0x000b27e0 | 84 | `core/NpSphericalJoint.cpp` | Np slot 35 (setProjectionMode); line 0x34; internal `[vt+0x34]` |
| phys_fn_004649 | 0x000b2840 | 57 | `core/NpSphericalJoint.cpp` | constructor; called by 004300 (0xa4990) |
| phys_fn_004651 | 0x000b2880 | 8 | `core/NpSphericalJoint.cpp` | secondary table 0x1011b0bc slot 0; compiler-generated thunk |
| phys_fn_004653 | 0x000b2890 | 55 | `core/NpSphericalJoint.cpp` | Np slot 0 (scalar deleting destructor) |

Already `reconstructed` through `ObjectModel.cpp` differentials or drives (proofs kept, new text
appended; the covering models gain a `// Product row:` pointer): 004282 (zmix drive), 004288 and
004292 (guardedstore), 004290 (batchgetters), 004633 (tailjmp), 004639 (mutexlistfree), 004641,
004643, 004645, 004647 (mutexfamily).

### Construction chain (NxJointType 3)

1. 000665's switch (table 0x14590) sends type 3 to **0x14414**: SDK allocator slot +8 with
   `(0x23c, 0)` (`push 0x23c` at 0x14420), null -> 0x1458a (result 0); otherwise
   `call 0x100a48f0` = **004300** on the block with the descriptor (0x14433), then the shared
   exit at 0x144fc.
2. **004300** SphericalJoint::SphericalJoint(const NxSphericalJointDesc&) (`ret 4`):
   `Joint(desc, 8)` (004141, `push 8` at 0xa48f7), vptr 0x10119e20 (0xa4901), then the member
   default constructors: twistLimit (0, 0, 1, 0, 0, 1) at +0x16c, swingLimit (0, 0, 1) at +0x184,
   the three springs (0, 0, 0) at +0x190/+0x19c/+0x1a8 (0xa4909-0xa4974) -- the inline
   `NxJointLimitPairDesc`/`NxJointLimitDesc`/`NxSpringDesc` constructors. SDK allocator `(0x1c,
   0)` -> **004649** on success, public object -> `this+0x48` (0xa4999), `desc.userData`
   (desc+0x60) -> `np+4` **without a null check** (0xa499c-0xa499f), then **004284(desc)**
   (0xa49a5).
3. **004649** NpSphericalJoint::NpSphericalJoint(SphericalJoint*): zeroes +4/+8, transient table
   0x1011af58 (`NxSphericalJoint`), 002404 on +0xc, secondary table 0x1011b0bc, internal at +0x18
   and +0x08, final table 0x1011b028. The prismatic shape.
4. Back in 000665 (0x144fc): identical to the other wired families (`[joint+0x48]` null ->
   internal slot 5 with 1 (004302) and result 0; otherwise the link copy, 000661, and the
   0x14529-0x1453f exit).

**Public-object offset: +0x48** (`mPublicObject`; 004300 writes it at 0xa4999, 000665 reads it at
0x14502).

### Object layouts

**SphericalJoint (internal), 0x23c bytes** (`push 0x23c` at 0x14420):

| Off | Size | Field | Evidence |
|---|---:|---|---|
| +0x000 | 0x16c | `Joint` base; vptr 0x10119e20 | 004300 0xa48fc/0xa4901; 004302 0xa49c8 |
| +0x16c | 0x18 | `mTwistLimit` (`NxJointLimitPairDesc`: low value/restitution/hardness, high ...) | 004300, 004284 (desc+0x7c), 004286; read by 004310, 004312 |
| +0x184 | 0xc | `mSwingLimit` (`NxJointLimitDesc`) | 004284 (desc+0x94), 004286; 004310 (+0x184, +0x188) |
| +0x190 | 0xc | `mTwistSpring` (`NxSpringDesc`) | 004284 (desc+0xa0), 004286; 004310 |
| +0x19c | 0xc | `mSwingSpring` | 004284 (desc+0xac), 004286; 004310 |
| +0x1a8 | 0xc | `mJointSpring` | 004284 (desc+0xb8), 004286; 004296 (+0x1a8, +0x1ac) |
| +0x1b4 | 0xc | `mSwingAxis` (desc.swingAxis, joint space of body 0) | 004284, 004286 |
| +0x1c0 | 0xc | `mSwingAxisWorld`: `(mWorldNormal[0] * s.x + mWorldAxis[0] * s.z) + mWorldCross[0] * s.y`, s = swingAxis (0xa2cdc-0xa2d91). Name unknown | written only by 004284; read by 004310, 004312 |
| +0x1cc | 4 | `mSwingLimitCos` = fcos(swingLimit.value) | 004284 0xa2c63-0xa2c73; 004310 0xa5ce2, 004312 |
| +0x1d0 | 4 | `mSphericalFlags` (NX_SJF_*) | 004284, 004286, 004288, 004290; 004296, 004308, 004310, 004312 |
| +0x1d4 | 4 | `mProjectionDistance` | 004284, 004286; 004298 |
| +0x1d8 | 0xc | `mLever[0]`: body 0's anchor lever (rotated local anchor) | written by 004296 (0xa32ea); read by 004294, 004308 |
| +0x1e4 | 0xc | `mLever[1]` | 004296 (0xa3308); 004294, 004308 |
| +0x1f0 | 0xc | `mBias`: the position error times SDK parameter 0 / arg (and the spring gain) | 004282 (zero), 004296; 004308 |
| +0x1fc | 0xc | `mSpringGain`: 1 / (g K^-1[i][i] + 1) per axis when the joint spring is on | 004296 (0xa3efc-0xa3f3c); 004308 (flag 0x10) |
| +0x208 | 0x24 | `mInverseMass`: row-major 3x3, the inverse of the point constraint's effective mass (identity when singular) | 004296; 004308 |
| +0x22c | 0xc | `mAccumulatedImpulse` | 004296 (zeroed), 004308 |
| +0x238 | 4 | `mMaxImpulseSquared`: maxForce^2 (1.1920929e-7f when that is 0) | 004296 (0xa3f91-0xa3fa8); 004308 |

The +0x1d8..+0x238 fields have the revolute pilot's roles (revolute +0x1dc/+0x1e8 levers,
+0x1ac bias, +0x1b8 3x3, +0x1f4 accumulator, +0x200 limit) at spherical offsets; names are
descriptive, from the rows' arithmetic.

**NpSphericalJoint (public), 0x1c bytes**: exactly `NpJointShared<NxSphericalJoint,
SphericalJoint>` (vptr 0x1011b028, userData +4, appData +8 = internal, hook base +0xc with table
0x1011b0bc, write link +0x10, read link +0x14, internal +0x18). No own field.

### Dispatch tables

**0x10119e20: SphericalJoint internal (`phys_data_002668`, 17 slots)**

| Slot | Row | Declared as | Notes |
|---:|---|---|---|
| 0 | **004308** | `SphericalJoint::row_slot0(NxU32)` | `ret 4`, argument unread (revolute 004374's shape) |
| 1 | **004282** | `SphericalJoint::row_slot1()` | zeroes +0x1f0..+0x1f8 |
| 2 | 004111 | `Joint::row004111` | inherited |
| 3 | 004087 | `Joint::row004087` | inherited |
| 4 | **004312** (+ 004314) | `SphericalJoint::row_slot4(NxDebugRenderable&)` | `ret 4` |
| 5 | **004302** | `~SphericalJoint()` (scalar deleting) | deletes `[this+0x48]` through its slot 0 with 1, 004095, frees if flag&1 |
| 6 | **004296** | `SphericalJoint::row_slot6(NxReal)` | `ret 4`; the float is a divisor (`fld 1.0; fdiv [esp+0xb8]`, 0xa32d1) |
| 7 | **004310** | `SphericalJoint::row_slot7(NxReal)` | `ret 4`; ends by calling 004135 (Joint base slot 7) directly (0xa5ecf) |
| 8 | **004298** | `SphericalJoint::row_slot8(void*)` | `ret 4` |
| 9 | **004304** | `SphericalJoint::loadFromDesc(const NxSphericalJointDesc&)` | `ret 4` |
| 10 | **004286** | `SphericalJoint::saveToDesc(NxSphericalJointDesc&)` | `ret 4`; calls 004066 |
| 11 | **004288** | `SphericalJoint::setFlags(NxU32)` | `ret 4`; no wake raise (unlike revolute 004334) |
| 12 | **004290** | `SphericalJoint::getFlags() const` | reached from Np 004703 (`[vt+0x30]`) |
| 13 | **004292** | `SphericalJoint::setProjectionMode(NxJointProjectionMode)` | `ret 4` |
| 14 | 004186 (folded, `mov eax,[ecx+0x44]; ret`) | `SphericalJoint::getProjectionMode()` inline | as revolute slot 14; reached from Np 004707 (`[vt+0x38]`) |
| 15 | 001391 (folded) | `SphericalJoint::row_slot15()` inline | returns `this` |
| 16 | 001391 (folded) | `SphericalJoint::row_slot16()` inline | returns `this` |

The table runs to 0x10119e64, where the unit's `__FILE__` string starts. It has the revolute
table's shape (17 slots, the flags/projection-mode quartet at 11-14), so the two Np bodies that
read slots 12 and 14 (004703, 004707) serve both families unchanged.

**0x1011b028: NpSphericalJoint primary (`phys_data_002721`, 37 slots)**: slots 0-32 as the
`### Slot split` table (per-family rows 004653, 004623, 004625, 004627, 004629, 004631, 004639,
004633, 004637, 004641, 004643; the folded rows through `NpJointShared`), then 33 = **004645**
setFlags (line 0x27, `[vt+0x2c]`), 34 = 004703 getFlags, 35 = **004647** setProjectionMode (line
0x34, `[vt+0x34]`), 36 = 004707 getProjectionMode. Every write-locked NxJoint row reports line
0xf; loadFromDesc 0x13, saveToDesc 0x1e. **0x1011b0bc** (secondary): 004651.

**Slots 34 and 36.** The oracle's table points at revolute's bodies (identical-code folding: read
lock, internal slot 12 or 14, unlock). `NpSphericalJoint::getFlags`/`getProjectionMode` are
written in `core/NpSphericalJoint.cpp` with the same body and a comment naming the folded row
without the stable-ID form; `core/NpRevoluteJoint.cpp` keeps the stable-ID lines.

### The rows' shape

- **004296** (solver slot, arg = the step divisor): after the stale-body refresh, the levers r_i =
  body i's +0x134 3x3 times the world anchor (the anchor itself without the body), the position
  error e = (r0 - r1) + body 0's +0x158 - body 1's +0x158 (each stored), then +0x1d8/+0x1e4 = r0/r1,
  +0x1f0 = e * (1/arg * SDK parameter 0). The effective mass K = sum over bodies of (invMass * I
  - [r]x Iinv [r]x), built column by column against the unit axes (the `* 0.0f` products kept:
  the compiler cannot fold them); its adjugate over the determinant goes to +0x208. Determinant
  non-zero: one kind-6 record (+0x30 = this, everything else zero) that marks the joint for the
  point solve of slot 0, then, with the joint spring on (flag 0x10), the per-axis spring gains at
  +0x1fc and the bias scaled by them, and with finite maxForce +0x238 = maxForce^2 and +0x22c = 0.
  Determinant zero: +0x208 = identity and three kind-1 linear records along the unit axes
  (`gJointUnitAxis`): through 004393 with the spring's (1/((spring*arg + damper)*arg),
  spring/(spring*arg+damper)*arg) when the joint spring is on, otherwise through 004391 with bias
  e_i / arg and +0x48 = maxForce.
- **004308** (slot 0): revolute 004374's instructions over the spherical fields, plus the
  flag-0x10 spring-gain scaling of the velocity error (0xa4f1f-0xa4f4e) and `fchs` where revolute
  multiplies by -1.0f (same values).
- **004310** (slot 7): the twist spring (flag 4, one kind-3 record through 004393 along body 1's
  world axis), the swing spring (flag 8, two kind-3 records about the swing plane), the twist limit
  (flag 1: low == high -> one kind-3 lock record; otherwise the kind-2 limit records with
  restitution through 004389 and SDK parameter 4), the swing limit (flag 2, cos below
  +0x1cc -> one kind-2 record), then Joint base slot 7 (004135) called directly.
- **004306** (helper of 004310/004312): the twist angle about the half-way axis h = normalize(a0 +
  a1) (a_i body i's world axis): `-fpatan(c . n1, b . n1)` with c = normalize(h x n0), b = c x h;
  writes h and a cone factor (dot < 0 -> dot + 1, else 1).
- **004312 + 004314** (slot 4): with bit 9 (NX_JF_VISUALIZATION) after the stale-body refresh:
  parameter 32 -> three axis lines through row004123's point (red, green, blue, +/- scale);
  parameter 31 -> six arrows (both bodies' normals, crosses and axes, colours 0x902020, 0x209020,
  0x202090, 0xe05050, 0x50e050, 0x5050e0) and a yellow line between the two anchors, with four
  two-element NxVec3 arrays built through the `eh vector constructor iterator` (000001);
  parameter 33 -> the twist-limit arc (flag 1, 13 points between low and high with the limit
  colours of revolute's arc, the 004306 twist angle arrow in 0xff00d0) and the swing-limit cone
  (flag 2; 004314: 24 spokes of radius tan(acos(+0x1cc)), clamped to 1000 for cos in (-0.01,
  0.01), each spoke from the centre coloured by whether s . a1 is below the limit cosine).

### Dependency closure

- **write** (33 rows): 004282-004314 in `core/SphericalJoint.cpp` (17 rows, 004314 inside
  004312); 004623-004653 minus 004635 in `core/NpSphericalJoint.cpp` (15 rows, 004651 generated).
- **reuse**: Joint rows 004141, 004107, 004121, 004097, 004066, 004095, 004093, 004111, 004087,
  004135, 004123, 004064 (`core/Joint.cpp`), 004389, 004391, 004393 (`core/JointSupport.cpp`); the
  13 folded Np bodies (`NpJointShared`); 004703/004707 (bodies shared with revolute, see above);
  002362/002364/002366, 002404/002406, 000454/000480; 001391 and 004186 (inline bodies); 000001 (`eh vector constructor iterator`); 004417-004433, 005667;
  `NxNormalToTangents` (Foundation import `[0x1010418c]`, called once by 004312); `_CIacos`
  (0x000f47f0, through the shared x87 reproduction); the SDK allocator; SDK parameters 0, 4, 13,
  31, 32, 33; `gJointUnitAxis`.
- **Shared helpers moved**: revolute's file-static `revoluteCIacos`/`revoluteAcos` move to the new
  internal header `Physics/src/include/core/JointAcos.h` as `jointCIacos`/`jointAcos` (the
  Global Constraints' rule for reusing the `_CIacos` reproduction); `core/RevoluteJoint.cpp` calls
  them. The spherical acos sites (0xa5723-0xa5740, 0xa5d9f-0xa5dbc, 0xa6ff0-0xa700d) are the same
  clamp with the unit's own pi float (0x10119e10, the same value).
- **Reused from `core/JointLinearRecords.h`**: `jointLinearRecord` for 004296's three singular-arm
  records (the listing's record header and bit sequence at 0xa3c3a-0xa3d1a / 0xa3fd1-0xa4017 and
  copies; the cross products are the same products subtracted in the same order, stored) and
  `jointSolveRecord` for the rigid arm's tail (0xa4414-0xa446e and copies). Not reused:
  `jointLinearError` (004296 stores the error after each add; the helper keeps it on the stack).
- **defer** (existing stubs, as the pilot): 000571 (the break event 004308 posts), 000022 (the
  owner notify 004298 calls).

### What the new test case reaches

`nxSphericalCase` (indices 0 and 3 of the revolute table's anchor/axis values). Creation: 000297,
000665 case 3, 004300, 004141 (-> 004107, 004121 -> 004097 x2, 000480), 004649 (002404), 004284,
000661. Getters: 004437/004125, 004441/004129, 004483/004078, 004539, 004443/004070,
`isSphericalJoint` (inline 004427 -> 004479/004070), saveToDesc 004643 -> internal slot 10 =
**004286** -> 004066, getFlags (004703 body) -> internal slot 12 = **004290**, getProjectionMode
(004707 body) -> slot 14 (004186 inline). Compiled but not reached: 004282, 004288, 004292-004298,
004302 (release unwired), 004304, 004306-004314, 004623-004641, 004645, 004647, 004653.

### Result (Task 3c)

- Wired: `NxSceneInternal::createJoint` builds type 3 through `SphericalJoint` (0x23c) and
  `nxSphericalJointAttachScene` in the same block as the other wired families.
  `nxJointSizeForType` is now reached only by types 4-9.
- The staged pair matched the oracle on the first run (`stdout_delta=0`, 35/35 Phase 6
  coverage, 16/16 Phase 7); no transcript difference was found. Registered lines (four per
  joint list) were copied from the oracle side of that run: the oracle-differential section
  of the Phase 6 log for `NxPhysicsJointTests` and the `pair=oracle` child output for
  `NxPhysicsJointStagedPairTests` (identical lines).
- A cdb trace of the candidate (`evidence/joint-families-trace-spherical.txt`) shows 004300,
  004649, 004284, 004643, 004286 and 004290 executing in both cases, and the Np bodies for
  slots 34/36 reaching internal slots 12/14; 004282, 004288, 004292-004298, 004302-004314 and
  the Np setters are compiled but not reached. The solver, projection and visualization rows
  are checked against the listing by review and the build only.
- Ledger: the 32 rows are `reconstructed_not_falsified` in `gates/phase6-closure.json` (22
  moved from `not_reconstructed_in_phase`; 10 already were); counts 217 / 214.

## PointOnLine

Recovered by joint-families Task 3d from the unit bundles `units/core__PointOnLineJoint.cpp.md`,
`units/core__NpPointOnLineJoint.cpp.md` and
`units/gap__core__NpPointOnLineJoint.cpp__to__core__NpSphericalJoint.cpp.md`, the Capstone
listing, the relocated table words in `oracle/pe.json` and the pinned Ghidra supplement. 004268
(saveToDesc) had no decompile; Task 3d added 0x000a1cc0 to `oracle/ghidra/supplement.json` (the
union with the 18 existing `requested` RVAs; `ok`; the 18 existing entries came back unchanged).
004270 is two instructions and is read from the listing only. The listing is authoritative over
every decompile.

### Row assignment

`work_units.json` puts 7 rows in `core\PointOnLineJoint.cpp` (evidenced span 0xa1cc0-0xa2b20, no
ambiguous rows), 10 in `core\NpPointOnLineJoint.cpp` (evidenced span 0xb1f50-0xb22a0, no
ambiguous rows) and 3 ambiguous rows in
`gap:core\NpPointOnLineJoint.cpp..core\NpSphericalJoint.cpp`. Checked by hand:

- **Before `core\PointOnLineJoint.cpp`**: 004266 (PointInPlaneJoint::loadFromDesc, slot 9 of the
  point-in-plane table 0x10119b48) ends `ret 4` at 0xa1caf; 004268 starts at 0xa1cc0 and pushes
  the `PointOnLineJoint.cpp` `__FILE__` (0x10119ce4).
- **Inside it**: 004270 (0xa1d00, 5 B: `mov eax,[ecx]; jmp [eax+0x2c]`) is slot 11 of the
  point-on-line internal table 0x10119cb0 (0x119cdc) and is referenced from nowhere else; it sits
  between 004268 and 004272 in this unit's code, so it is this unit's.
- **After it**: 004280 (loadFromDesc) ends `ret 4` at 0xa2bdf; the next row 004282 (0xa2bf0) is
  spherical (`## Spherical`).
- **`core\NpPointOnLineJoint.cpp`**: 004595 (NpPointInPlaneJoint slot 0) ends `ret 4` at
  0xb1f44; 004597 starts at 0xb1f50 and pushes the `NpPointOnLineJoint.cpp` `__FILE__`
  (0x1011ae7c).
- **The gap `core\NpPointOnLineJoint.cpp..core\NpSphericalJoint.cpp`** (004617-004621,
  0xb2300-0xb2384) is the constructor / thunk / deleting-destructor triple that closes every Np
  unit, and it is point-on-line's: 004617 installs the transient table 0x1011adf8
  (`NxPointOnLineJoint`), the secondary table 0x1011af3c and the final table 0x1011aeb8
  (`phys_data_002718`) and is called only by the point-on-line constructor 004276 (0xa2aad);
  004619 is `sub ecx,0xc; jmp 004621`, the only slot of the secondary table 0x1011af3c; 004621 is
  slot 0 of 0x1011aeb8 and reinstalls 0x1011aeb8/0x1011af3c. 004623 (0xb2390) is
  NpSphericalJoint's.
- No neighbouring ambiguous row belongs to another family, and no point-on-line row is named by
  another family's table (see "### Point-in-plane" below).

| Stable ID | RVA | Size | File | Evidence |
|---|---|---:|---|---|
| phys_fn_004268 | 0x000a1cc0 | 54 | `core/PointOnLineJoint.cpp` | internal slot 10 (0x119cd8); "PointOnLineJoint::saveToDesc" line 0x41 |
| phys_fn_004270 | 0x000a1d00 | 5 | `core/PointOnLineJoint.cpp` | internal slot 11 (0x119cdc); calls slot 11 of `this` |
| phys_fn_004272 | 0x000a1d10 | 2073 | `core/PointOnLineJoint.cpp` | internal slot 6 (0x119cc8); the solver slot |
| phys_fn_004274 | 0x000a2530 | 1345 | `core/PointOnLineJoint.cpp` | internal slot 4 (0x119cc0); debug visualization |
| phys_fn_004276 | 0x000a2a80 | 84 | `core/PointOnLineJoint.cpp` | createJoint case 4 (0x1445c); installs 0x10119cb0 (0xa2a90) |
| phys_fn_004278 | 0x000a2ae0 | 56 | `core/PointOnLineJoint.cpp` | internal slot 5; reinstalls 0x10119cb0 (0xa2ae8) |
| phys_fn_004280 | 0x000a2b20 | 194 | `core/PointOnLineJoint.cpp` | internal slot 9; "PointOnLineJoint::loadFromDesc" lines 0x2d/0x2e |
| phys_fn_004597 | 0x000b1f50 | 84 | `core/NpPointOnLineJoint.cpp` | Np slot 2; line 0x10 |
| phys_fn_004599 | 0x000b1fb0 | 84 | `core/NpPointOnLineJoint.cpp` | Np slot 4; line 0x10 |
| phys_fn_004601 | 0x000b2010 | 89 | `core/NpPointOnLineJoint.cpp` | Np slot 9; line 0x10 |
| phys_fn_004603 | 0x000b2070 | 89 | `core/NpPointOnLineJoint.cpp` | Np slot 11; line 0x10 |
| phys_fn_004605 | 0x000b20d0 | 97 | `core/NpPointOnLineJoint.cpp` | Np slot 13; line 0x10 |
| phys_fn_004607 | 0x000b2140 | 74 | `core/NpPointOnLineJoint.cpp` | Np slot 15; line 0x10 |
| phys_fn_004609 | 0x000b2190 | 88 | `core/NpPointOnLineJoint.cpp` | Np slot 29; line 0x10 |
| phys_fn_004611 | 0x000b21f0 | 74 | `core/NpPointOnLineJoint.cpp` | Np slot 14; line 0x10 |
| phys_fn_004613 | 0x000b2240 | 84 | `core/NpPointOnLineJoint.cpp` | Np slot 31 (loadFromDesc); line 0x14; internal `[vt+0x24]` |
| phys_fn_004615 | 0x000b22a0 | 84 | `core/NpPointOnLineJoint.cpp` | Np slot 32 (saveToDesc); line 0x1f; internal `[vt+0x28]` |
| phys_fn_004617 | 0x000b2300 | 57 | `core/NpPointOnLineJoint.cpp` | constructor; called by 004276 (0xa2aad) |
| phys_fn_004619 | 0x000b2340 | 8 | `core/NpPointOnLineJoint.cpp` | secondary table 0x1011af3c slot 0; compiler-generated thunk |
| phys_fn_004621 | 0x000b2350 | 55 | `core/NpPointOnLineJoint.cpp` | Np slot 0 (scalar deleting destructor) |

Already `reconstructed` through `ObjectModel.cpp` differentials (proofs kept, new text appended):
004607 (tailjmp), 004611 (mutexlistfree; its model `nxMutexListFree` gains the `// Product row:`
pointer), 004613 and 004615 (mutexfamily).

### Construction chain (NxJointType 4)

1. 000665's switch (table 0x14590) sends type 4 to **0x1443d**: SDK allocator slot +8 with
   `(0x16c, 0)` (`push 0x16c` at 0x14449), null -> 0x1458a (result 0); otherwise
   `call 0x100a2a80` = **004276** on the block with the descriptor (0x1445c), then the shared exit
   at 0x144fc.
2. **004276** PointOnLineJoint::PointOnLineJoint(const NxPointOnLineJointDesc&) (`ret 4`):
   `Joint(desc, 4)` (004141, `push 4` at 0xa2a86), vptr 0x10119cb0 (0xa2a90), SDK allocator
   `(0x1c, 0)` -> **004617** on success, public object -> `this+0x48` (0xa2ab2; 0xa2ac4 stores the
   null), `desc.userData` (desc+0x60) -> `np+4` **without a null check** (0xa2ab5-0xa2ab8, and
   0xa2ac7-0xa2aca on the null path, which writes to address 4). Nothing follows: the class has no
   field of its own and no helper after the base constructor.
3. **004617** NpPointOnLineJoint::NpPointOnLineJoint(PointOnLineJoint*): zeroes +4/+8, transient
   table 0x1011adf8 (`NxPointOnLineJoint`), 002404 on +0xc, secondary table 0x1011af3c, internal at
   +0x18 and +0x08, final table 0x1011aeb8. The prismatic shape.
4. Back in 000665 (0x144fc): identical to the other wired families (`[joint+0x48]` null ->
   internal slot 5 with 1 (004278) and result 0; otherwise the link copy, 000661, and the
   0x14529-0x1453f exit).

**Public-object offset: +0x48** (`mPublicObject`; 004276 writes it at 0xa2ab2, 000665 reads it at
0x14502).

### Object layouts

**PointOnLineJoint (internal), 0x16c bytes** (`push 0x16c` at 0x14449): exactly the `Joint` base
with vptr 0x10119cb0 (004276 0xa2a90, 004278 0xa2ae8). No row of the unit reads or writes an offset
at or above +0x16c. The rows use only `Joint` base fields: `mBody` (+0x08/+0x0c), `mFlags`
(+0x2c), `mMaxForce` (+0x3c), `mPublicObject` (+0x48), `mWorldNormal[0]` (+0xcc),
`mWorldCross[0]` (+0xe4), `mWorldAnchor` (+0x114/+0x120), `mBodyStamp` (+0x14c/+0x150), and the
body records' +0x134 3x3, +0x158 position and +0x204 support-body pointer.

**NpPointOnLineJoint (public), 0x1c bytes**: exactly `NpJointShared<NxPointOnLineJoint,
PointOnLineJoint>` (vptr 0x1011aeb8, userData +4, appData +8 = internal, hook base +0xc with table
0x1011af3c, write link +0x10, read link +0x14, internal +0x18). No own field.

### Dispatch tables

**0x10119cb0: PointOnLineJoint internal (`phys_data_002662`, 13 slots)**

| Slot | Row | Declared as | Notes |
|---:|---|---|---|
| 0 | 004248 (folded, `ret 4`) | `Joint::row_slot0` inline | inherited no-op |
| 1 | 001583 (folded, `ret`) | `Joint::row_slot1` inline | inherited no-op |
| 2 | 004111 | `Joint::row004111` | inherited |
| 3 | 004087 | `Joint::row004087` | inherited |
| 4 | **004274** | `PointOnLineJoint::row_slot4(NxDebugRenderable&)` | `ret 4` |
| 5 | **004278** | `~PointOnLineJoint()` (scalar deleting) | deletes `[this+0x48]` through its slot 0 with 1, 004095, frees if flag&1 |
| 6 | **004272** | `PointOnLineJoint::row_slot6(NxReal)` | `ret 4`; the float is a divisor (`fld 1.0; fdiv [esp+0x78]`, 0xa2200-0xa2209) |
| 7 | 004135 | `Joint::row_slot7` | inherited |
| 8 | 004248 (folded) | `Joint::row_slot8` inline | inherited no-op |
| 9 | **004280** | `PointOnLineJoint::loadFromDesc(const NxPointOnLineJointDesc&)` | `ret 4` |
| 10 | **004268** | `PointOnLineJoint::saveToDesc(NxPointOnLineJointDesc&)` | `ret 4`; tail-jumps 004066 |
| 11 | **004270** | `PointOnLineJoint::row_slot11()` | `mov eax,[ecx]; jmp [eax+0x2c]`: a virtual call of slot 11 on `this`, that is of itself, so it never returns if called. Nothing in the image calls internal slot 11 of a point-on-line joint (the Np rows use slots 9 and 10 only). Written as the body that compiles to it (`return row_slot11();`) |
| 12 | 001391 (folded, `mov eax,ecx; ret`) | `PointOnLineJoint::row_slot12()` inline | returns `this` |

The table runs to 0x10119ce4, where the unit's `__FILE__` string starts. Where prismatic has two
`return this` slots (11, 12), point-on-line has 004270 at 11 and one `return this` at 12.

**0x1011aeb8: NpPointOnLineJoint primary (`phys_data_002718`, 33 slots)**: slots 0-32 as the
`### Slot split` table (per-family rows 004621, 004597, 004599, 004601, 004603, 004605, 004611,
004607, 004609, 004613, 004615; the folded rows through `NpJointShared`). Every write-locked
NxJoint row reports line 0x10; loadFromDesc 0x14, saveToDesc 0x1f (the `push` before each
`push 0x1011ae7c`: 0xb1f71 ... 0xb2211, 0xb2261, 0xb22c1). Slots 31/32 call internal slots 9/10
(`[vt+0x24]` 0xb2285 / `[vt+0x28]` 0xb22e5). **0x1011af3c** (secondary): 004619.

### The rows' shape

- **004272** (solver slot, arg = the step divisor): the support-body pointers (body +0x204) are
  read before the stale-body refresh. Then, with R0/R1 the bodies' +0x134 3x3s and t0/t1 their
  +0x158 positions: n = R0 * worldNormal[0] and c = R0 * worldCross[0] (copies without body 0);
  p0 = R0 * worldAnchor[0] + t0; r1 = R1 * worldAnchor[1] (stored) and p1 = r1 + t1 (copies
  without the bodies). With d = p1 - p0, s = n . d (stored) and t = c . d (kept), the point of the
  line through p0 along the joint axis nearest p1 is x = (p1 - n s) - c t, and body 0's lever is
  r0 = x - t0. The error of the lever pair is `jointLinearError(r0, r1)` (the same instructions,
  0xa218a-0xa21fa). Two kind-1 linear records follow, along n and along c, each through
  `jointLinearRecord` (the cross products are the same products subtracted in the same order; the
  listing stores +0x18/+0x24 in x, y, z order where the helper stores y, z, x) and
  `jointSolveRecord` with bias (e . n) / arg and (e . c) / arg and +0x48 = maxForce. Listing over
  decompile: the decompile regroups every dot product (the listing's are
  ((n.z d.z + d.x n.x) + d.y n.y), ((d.x c.x + d.z c.z) + d.y c.y), ((e.z n.z + e.x n.x) + e.y n.y)
  and ((e.x c.x + e.z c.z) + e.y c.y)) and drops the kind tests of the record flags as unreachable;
  p1.x, d, t, x.x and e.x stay unrounded on the FPU stack where the decompile shows floats. The
  first record passes the local that held its bias as 004391's (write-only) first output, the
  second the local that held maxForce; neither is read afterwards.
- **004274** (slot 4, `NxDebugRenderable&`): with +0x2c bit 9 (NX_JF_VISUALIZATION), after the
  stale-body refresh. Parameter 32 (NX_VISUALIZE_JOINT_WORLD_AXES, 0x10123b98) non-zero: s =
  param 13 * param 32; P = row004123 (the anchors' midpoint), A = row004127 (world axis 0); three
  lines through P, P -/+ s along x (0xff0000), y (0xff00) and z (0xff), then A is scaled by s in
  place and the line P - A to P + A is drawn (0xffffff). Parameter 31
  (NX_VISUALIZE_JOINT_LOCAL_AXES, 0x10123b94) non-zero: s = param 31 * param 13; the same four
  lines, then the same three-line cross at body 1's world anchor Q = R1 * worldAnchor[1] + t1 (the
  copy without body 1) in 0xcf0000, 0xcf00 and 0xcf. All lines go through `addLine` (renderable
  slot +0x20), minus end first. Listing over decompile: the decompile loses the argument order
  and most arguments of every call.

### Point-in-plane

Task 3e's rows are near-identical but not folded: every point-in-plane row has its own address
and its own table entry (0x10119b48 names 004260/004264/004258 at slots 4/5/6), so nothing here is
shared with Task 3e and neither family overrides a row of the other. For Task 3e: a normalised
listing diff of 004274 against 004260 shows the same instructions apart from the white line
(004260 starts it at P itself, forming no P - A, and sums P + A in the other operand order) and
the grouping of Q's three-term sums. 004272 (2,073 B) against 004258 (1,391 B): 004258 reads the
support-body pointers after the refresh and builds one record, so the solver rows differ
throughout.

### Dependency closure

- **write** (20 rows, 4,778 B): 004268, 004270, 004272, 004274, 004276, 004278, 004280 in
  `core/PointOnLineJoint.cpp` (3,811 B); 004597-004621 in `core/NpPointOnLineJoint.cpp` (13 rows,
  967 B; 004619 generated, its stable-ID line above the destructor it serves).
- **reuse**: Joint rows 004141, 004107, 004121, 004097, 004066, 004095, 004093, 004111, 004087,
  004135, 004123, 004127 (`core/Joint.cpp`) and 004391 (`core/JointSupport.cpp`); the 13 folded Np
  bodies (`NpJointShared`); 002362/002364/002366, 002404/002406, 000454/000480; 004248, 001583,
  001391 (inline bodies); 004417-004433, 005667; `jointLinearError`, `jointLinearRecord`,
  `jointSolveRecord` and `jointLinearSdkParameter` (`core/JointLinearRecords.h`); the SDK
  allocator; SDK parameters 0, 13, 31, 32. `core/JointAcos.h` is not needed (the unit has no acos).
- **defer**: none.

### What the new test case reaches

`nxPointOnLineCase` (indices 0 and 3 of the revolute table's anchor/axis values). Creation:
000297, 000665 case 4, 004276, 004141 (-> 004107, 004121 -> 004097 x2, 000480), 004617 (002404),
000661. Getters: 004437/004125, 004441/004129, 004483/004078, 004539, 004443/004070,
`isPointOnLineJoint` (inline 004421 -> 004479/004070), saveToDesc 004615 -> internal slot 10 =
**004268** -> 004066. Compiled but not reached: 004270, 004272, 004274, 004278 (release unwired),
004280, 004597-004613, 004621.

### Result (Task 3d)

- Wired: `NxSceneInternal::createJoint` builds type 4 through `PointOnLineJoint` (0x16c) and
  `nxPointOnLineJointAttachScene` in the same block as the other wired families.
  `nxJointSizeForType` is now reached only by types 5-9.
- The staged pair matched the oracle on the first run (`stdout_delta=0`, 43/43 Phase 6 coverage,
  20/20 Phase 7); no transcript difference was found. Registered lines (four per joint list) were
  copied from the oracle side of that run: the oracle-differential section of the Phase 6 log for
  `NxPhysicsJointTests` and the `pair=oracle` child output for `NxPhysicsJointStagedPairTests`
  (identical lines).
- A cdb trace of the candidate (`evidence/joint-families-trace-point-on-line.txt`) shows 004276,
  004617, 004615 and 004268 executing in both cases; 004270-004274, 004278, 004280 and the Np
  setters are compiled but not reached. The solver and visualization rows are checked against the
  listing by review and the build only.
- Ledger: the 20 rows are `reconstructed_not_falsified` in `gates/phase6-closure.json` (16 moved
  from `not_reconstructed_in_phase`; 4 already were); counts 201 / 230.

## PointInPlane

Recovered by joint-families Task 3e from the unit bundles `units/core__PointInPlaneJoint.cpp.md`,
`units/core__NpPointInPlaneJoint.cpp.md` and
`units/gap__core__NpFixedJoint.cpp__to__core__NpPointInPlaneJoint.cpp.md`, the Capstone listing,
the relocated table words in `oracle/pe.json` and the pinned Ghidra supplement. 004256
(saveToDesc) and 004260 (debug visualization) had no decompile; Task 3e added 0x000a10a0 and
0x000a1650 to `oracle/ghidra/supplement.json` (the union with the 19 existing `requested` RVAs;
both `ok`; the 19 existing entries came back unchanged). The listing is authoritative over every
decompile.

### Row assignment

`work_units.json` puts 6 rows in `core\PointInPlaneJoint.cpp` (evidenced span 0xa10a0-0xa1bf0, no
ambiguous rows), 15 in `core\NpPointInPlaneJoint.cpp` (evidenced span 0xb1ab0-0xb1e60, no
ambiguous rows; 004573 and 004577 are folded NpJoint bodies that `NpJointShared` already claims)
and 3 ambiguous rows in `gap:core\NpFixedJoint.cpp..core\NpPointInPlaneJoint.cpp`. Checked by
hand:

- **Before `core\PointInPlaneJoint.cpp`**: 004254 (`source` FixedJoint.cpp) ends before 0xa10a0;
  004256 starts at 0xa10a0 and pushes the `PointInPlaneJoint.cpp` `__FILE__` (0x10119b7c).
- **After it**: 004266 (loadFromDesc) ends `ret 4` at 0xa1caf; 004268 (0xa1cc0) is
  point-on-line's (`## PointOnLine`).
- **`core\NpPointInPlaneJoint.cpp`** ends with its own constructor / thunk / deleting-destructor
  triple (004591-004595, 0xb1ec0-0xb1f44): 004591 installs the transient table 0x1011ac98
  (`phys_data_002713`, `NxPointInPlaneJoint`), the secondary table 0x1011addc and the final table
  0x1011ad58 (`phys_data_002715`), and its only caller is the point-in-plane constructor 004262
  (0xa1b7d); 004593 is `sub ecx,0xc; jmp 004595`, the only slot of 0x1011addc; 004595 is slot 0
  of 0x1011ad58. 004597 (0xb1f50) is NpPointOnLineJoint's.
- **The gap `core\NpFixedJoint.cpp..core\NpPointInPlaneJoint.cpp` (004561-004565,
  0xb1a20-0xb1aa6) is NOT this family's.** 004561 installs 0x1011ab38 (`phys_data_002710`),
  0x1011ac7c and 0x1011abf8 (`phys_data_002712`, the NpFixedJoint table whose slot 0 is 004565).
  Its only caller is 004250 (0xa0f70), the constructor that createJoint case 8 calls (0x144ab).
  The gap is the fixed family's Np tail and is left to Task 3h.
- No neighbouring ambiguous row belongs to this family, and no other family's table names a
  point-in-plane row.

| Stable ID | RVA | Size | File | Evidence |
|---|---|---:|---|---|
| phys_fn_004256 | 0x000a10a0 | 54 | `core/PointInPlaneJoint.cpp` | internal slot 10 (0x119b70); "PointInPlaneJoint::saveToDesc" line 0x3e |
| phys_fn_004258 | 0x000a10e0 | 1391 | `core/PointInPlaneJoint.cpp` | internal slot 6 (0x119b60); the solver slot |
| phys_fn_004260 | 0x000a1650 | 1273 | `core/PointInPlaneJoint.cpp` | internal slot 4 (0x119b58); debug visualization |
| phys_fn_004262 | 0x000a1b50 | 84 | `core/PointInPlaneJoint.cpp` | createJoint case 5 (0x14485); installs 0x10119b48 (0xa1b60) |
| phys_fn_004264 | 0x000a1bb0 | 56 | `core/PointInPlaneJoint.cpp` | internal slot 5; reinstalls 0x10119b48 |
| phys_fn_004266 | 0x000a1bf0 | 194 | `core/PointInPlaneJoint.cpp` | internal slot 9; "PointInPlaneJoint::loadFromDesc" lines 0x2a/0x2b |
| phys_fn_004567 | 0x000b1ab0 | 84 | `core/NpPointInPlaneJoint.cpp` | Np slot 2; line 0xf |
| phys_fn_004569 | 0x000b1b10 | 84 | `core/NpPointInPlaneJoint.cpp` | Np slot 4; line 0xf |
| phys_fn_004571 | 0x000b1b70 | 89 | `core/NpPointInPlaneJoint.cpp` | Np slot 9; line 0xf |
| phys_fn_004575 | 0x000b1c00 | 89 | `core/NpPointInPlaneJoint.cpp` | Np slot 11; line 0xf |
| phys_fn_004579 | 0x000b1c90 | 97 | `core/NpPointInPlaneJoint.cpp` | Np slot 13; line 0xf |
| phys_fn_004581 | 0x000b1d00 | 74 | `core/NpPointInPlaneJoint.cpp` | Np slot 15; line 0xf |
| phys_fn_004583 | 0x000b1d50 | 88 | `core/NpPointInPlaneJoint.cpp` | Np slot 29; line 0xf |
| phys_fn_004585 | 0x000b1db0 | 74 | `core/NpPointInPlaneJoint.cpp` | Np slot 14; line 0xf |
| phys_fn_004587 | 0x000b1e00 | 84 | `core/NpPointInPlaneJoint.cpp` | Np slot 31 (loadFromDesc); line 0x13; internal `[vt+0x24]` |
| phys_fn_004589 | 0x000b1e60 | 84 | `core/NpPointInPlaneJoint.cpp` | Np slot 32 (saveToDesc); line 0x1e; internal `[vt+0x28]` |
| phys_fn_004591 | 0x000b1ec0 | 57 | `core/NpPointInPlaneJoint.cpp` | constructor; called by 004262 (0xa1b7d) |
| phys_fn_004593 | 0x000b1f00 | 8 | `core/NpPointInPlaneJoint.cpp` | secondary table 0x1011addc slot 0; compiler-generated thunk |
| phys_fn_004595 | 0x000b1f10 | 55 | `core/NpPointInPlaneJoint.cpp` | Np slot 0 (scalar deleting destructor) |

Already `reconstructed` through `ObjectModel.cpp` differentials (proofs kept, new text appended):
004581 (tailjmp), 004585 (mutexlistfree; its model `nxMutexListFree` gains the `// Product row:`
pointer), 004587 and 004589 (mutexfamily).

A normalised listing diff of the Np unit (0xb1ab0-0xb1f50) against NpPointOnLineJoint
(0xb1f50-0xb2390) differs only in:

- the report lines (0xf/0x13/0x1e against 0x10/0x14/0x1f);
- the three table addresses;
- the two folded bodies 004573/004577, which sit inside this unit's range.

The five small internal rows diff the same way against point-on-line's:

- 004256/004268 and 004266/004280 differ only in the `__FILE__`, message and line pushes;
- 004262/004276 differ in the type bit (`push 2` at 0xa1b56 against `push 4`), the vptr and the
  Np constructor;
- 004264/004278 differ in the vptr only.

### Construction chain (NxJointType 5)

1. 000665's switch (table 0x14590) sends type 5 to **0x14466**: SDK allocator slot +8 with
   `(0x16c, 0)` (`push 0x16c` at 0x14472), null -> 0x1458a (result 0); otherwise
   `call 0x100a1b50` = **004262** on the block with the descriptor (0x14485), then the shared exit
   at 0x144fc.
2. **004262** PointInPlaneJoint::PointInPlaneJoint(const NxPointInPlaneJointDesc&) (`ret 4`):
   `Joint(desc, 2)` (004141, `push 2` at 0xa1b56: the type bit, not the NxJointType), vptr
   0x10119b48 (0xa1b60), SDK allocator `(0x1c, 0)` -> **004591** on success, public object ->
   `this+0x48` (0xa1b82; 0xa1b94 stores the null), `desc.userData` (desc+0x60) -> `np+4`
   **without a null check** (0xa1b85-0xa1b88, and 0xa1b97-0xa1b9a on the null path). Nothing
   follows: the class has no field of its own.
3. **004591** NpPointInPlaneJoint::NpPointInPlaneJoint(PointInPlaneJoint*): zeroes +4/+8,
   transient table 0x1011ac98 (`NxPointInPlaneJoint`), 002404 on +0xc, secondary table 0x1011addc,
   internal at +0x18 and +0x08, final table 0x1011ad58. The prismatic shape.
4. Back in 000665 (0x144fc): identical to the other wired families (`[joint+0x48]` null ->
   internal slot 5 with 1 (004264) and result 0; otherwise the link copy, 000661, and the
   0x14529-0x1453f exit).

**Public-object offset: +0x48** (`mPublicObject`; 004262 writes it at 0xa1b82, 000665 reads it at
0x14502).

### Object layouts

**PointInPlaneJoint (internal), 0x16c bytes** (`push 0x16c` at 0x14472): exactly the `Joint` base
with vptr 0x10119b48 (004262 0xa1b60, 004264). No row of the unit reads or writes an offset at or
above +0x16c. The rows use only `Joint` base fields: `mBody` (+0x08/+0x0c), `mFlags` (+0x2c),
`mMaxForce` (+0x3c), `mPublicObject` (+0x48), `mWorldAxis[0]` (+0xfc), `mWorldAnchor`
(+0x114/+0x120), `mBodyStamp` (+0x14c/+0x150), and the body records' +0x134 3x3, +0x158 position
and +0x204 support-body pointer.

**NpPointInPlaneJoint (public), 0x1c bytes**: exactly `NpJointShared<NxPointInPlaneJoint,
PointInPlaneJoint>` (vptr 0x1011ad58, userData +4, appData +8 = internal, hook base +0xc with
table 0x1011addc, write link +0x10, read link +0x14, internal +0x18). No own field.

### Dispatch tables

**0x10119b48: PointInPlaneJoint internal (`phys_data_002657`, 13 slots)**

| Slot | Row | Declared as | Notes |
|---:|---|---|---|
| 0 | 004248 (folded, `ret 4`) | `Joint::row_slot0` inline | inherited no-op |
| 1 | 001583 (folded, `ret`) | `Joint::row_slot1` inline | inherited no-op |
| 2 | 004111 | `Joint::row004111` | inherited |
| 3 | 004087 | `Joint::row004087` | inherited |
| 4 | **004260** | `PointInPlaneJoint::row_slot4(NxDebugRenderable&)` | `ret 4` |
| 5 | **004264** | `~PointInPlaneJoint()` (scalar deleting) | deletes `[this+0x48]` through its slot 0 with 1, 004095, frees if flag&1 |
| 6 | **004258** | `PointInPlaneJoint::row_slot6(NxReal)` | `ret 4`; the float is a divisor (`fdiv [esp+0x54]`, 0xa14d9) |
| 7 | 004135 | `Joint::row_slot7` | inherited |
| 8 | 004248 (folded) | `Joint::row_slot8` inline | inherited no-op |
| 9 | **004266** | `PointInPlaneJoint::loadFromDesc(const NxPointInPlaneJointDesc&)` | `ret 4` |
| 10 | **004256** | `PointInPlaneJoint::saveToDesc(NxPointInPlaneJointDesc&)` | `ret 4`; tail-jumps 004066 |
| 11 | 001391 (folded, `mov eax,ecx; ret`) | `PointInPlaneJoint::row_slot11()` inline | returns `this` |
| 12 | 001391 (folded) | `PointInPlaneJoint::row_slot12()` inline | returns `this` |

The table runs to 0x10119b7c, where the unit's `__FILE__` string starts. Point-on-line has its
self-calling row 004270 at slot 11. Point-in-plane has prismatic's two `return this` slots
instead.

**0x1011ad58: NpPointInPlaneJoint primary (`phys_data_002715`, 33 slots)**: slots 0-32 as the
`### Slot split` table (per-family rows 004595, 004567, 004569, 004571, 004575, 004579, 004585,
004581, 004583, 004587, 004589; the folded rows through `NpJointShared`). Every write-locked
NxJoint row reports line 0xf; loadFromDesc 0x13, saveToDesc 0x1e (the `push` before each
`push 0x1011ad1c`: 0xb1ad1 ... 0xb1dd1, 0xb1e21, 0xb1e81). Slots 31/32 call internal slots 9/10
(`[vt+0x24]` 0xb1e45 / `[vt+0x28]` 0xb1ea5). **0x1011addc** (secondary): 004593.

### The rows' shape

- **004258** (solver slot, arg = the step divisor):
  - Order: the stale-body refresh comes first. The support-body pointers (body +0x204) are read
    after it, inside the anchor blocks (0xa11d3, 0xa12b6).
  - Frames: R0/R1 are the bodies' +0x134 3x3s and t0/t1 their +0x158 positions. n = R0 *
    worldAxis[0] is the plane normal (the copy without body 0). p0 = R0 * worldAnchor[0] + t0.
    r1 = R1 * worldAnchor[1] (stored), and p1 = r1 + t1.
  - Projection: with d = p1 - p0 and s = (d.y n.y + d.z n.z) + d.x n.x (kept), the projection of
    p1 on the plane through p0 is x = p1 - n s (n.x s unrounded, n.y s and n.z s stored). Body 0's
    lever is r0 = x - t0 (x itself without body 0).
  - Error: `jointLinearError(r0, r1)` gives the error of the lever pair (the same instructions,
    0xa1444-0xa14b4).
  - Record: ONE kind-1 linear record follows, along n, through `jointLinearRecord`. It forms the
    same products, subtracted in the same order; the listing stores +0x18/+0x24 in x, y, z order
    where the helper stores y, z, x.
  - Solve: `jointSolveRecord` with bias ((e.y n.y + e.z n.z) + e.x n.x) / arg and +0x48 =
    maxForce. The bias is an `fdiv` stored into the argument's slot (0xa14d9-0xa14dd); maxForce is
    read at 0xa14bc, before 004093. 004391's first output is the argument slot holding the bias
    (0xa15fd), and 004391 only writes it.
  - Listing over decompile: the decompile regroups the sums of n, p0 and r1 and every dot product,
    and drops the kind tests of the record flags as unreachable. p1.x, d, s, n.x s and e stay
    unrounded on the FPU stack.
- **004260** (slot 4, `NxDebugRenderable&`): the instructions of point-on-line 004274, with two
  differences:
  1. The white axis line starts at P itself: `addLine(P, A + P, 0xffffff)`, with A scaled in place
     and no P - A formed. The world-axes block sums the end as A + P in every component
     (0xa17f1-0xa1811). The local-axes block sums P.x + A.x, A.y + P.y and A.z + P.z
     (0xa196e-0xa198e).
  2. Body 1's world anchor Q sums x as ((R1 a.y + R2 a.z) + R0 a.x), y as
     ((R4 a.y + R3 a.x) + R5 a.z) and z as ((R7 a.y + R6 a.x) + R8 a.z) (0xa19bb-0xa1a35).

  The crosses (0xff0000/0xff00/0xff at P, 0xcf0000/0xcf00/0xcf at Q) and the parameter gates (32
  world axes, 31 local axes, each scaled by 13) are point-on-line's. Listing over decompile: the
  supplement decompile loses the argument order and most arguments of every call.

### Dependency closure

- **write** (19 rows, 4,019 B): 004256, 004258, 004260, 004262, 004264, 004266 in
  `core/PointInPlaneJoint.cpp` (3,052 B); 004567-004595 minus 004573/004577 in
  `core/NpPointInPlaneJoint.cpp` (13 rows, 967 B; 004593 generated, its stable-ID line above the
  destructor it serves).
- **reuse**:
  - Joint rows 004141, 004107, 004121, 004097, 004066, 004095, 004093, 004111, 004087, 004135,
    004123 and 004127 (`core/Joint.cpp`), and 004391 (`core/JointSupport.cpp`).
  - The 13 folded Np bodies (`NpJointShared`), including 004573/004577 in this unit's range.
  - 002362/002364/002366, 002404/002406 and 000454/000480.
  - The inline bodies 004248, 001583 and 001391.
  - 004417-004433 and 005667.
  - `jointLinearError`, `jointLinearRecord`, `jointSolveRecord` and `jointLinearSdkParameter`
    (`core/JointLinearRecords.h`).
  - The SDK allocator and SDK parameters 0, 13, 31 and 32.
  - No acos is needed.
- **defer**: none. The gap rows 004561-004565 are the fixed family's (Task 3h).

### What the new test case reaches

`nxPointInPlaneCase` (indices 0 and 3 of the revolute table's anchor/axis values).

- Creation: 000297, 000665 case 5, 004262, 004141 (-> 004107, 004121 -> 004097 x2, 000480),
  004591 (002404), 000661.
- Getters: 004437/004125, 004441/004129, 004483/004078, 004539, 004443/004070,
  `isPointInPlaneJoint` (inline 004419 -> 004479/004070), and saveToDesc 004589 -> internal slot
  10 = **004256** -> 004066.
- Compiled but not reached: 004258, 004260, 004264 (release unwired), 004266, 004567-004587,
  004595.
