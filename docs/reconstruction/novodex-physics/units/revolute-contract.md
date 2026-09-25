# Revolute pilot contract

Recovered by Task 4 from the three unit bundles (`units/core__RevoluteJoint.cpp.md`,
`units/core__NpRevoluteJoint.cpp.md`, `units/Joint.cpp.md`), the Capstone listing
(`oracle/capstone/manifest.json`), the Ghidra manifest and supplement, the relocated
pointers in `oracle/pe.json` (for table contents) and `oracle/dependencies.dot`.
Tasks 5–10 build on this document; a change to a layout, a name or a row list must be
made here in the same commit.

Conventions used below:

- Addresses are RVAs (image base 0x10000000). "0xac551" means the instruction at that RVA.
- `this+0xNN` offsets are byte offsets from the start of the object.
- **unknown** means the listing establishes the field's existence and the rows that touch
  it, but not its meaning. Do not invent a name for an unknown field; use
  `mUnknownNNN` (NNN = hex offset) in headers.
- Names given here come from one of three evidence kinds only: (a) an assert string in the
  row, (b) the public `NxJoint`/`NxRevoluteJoint` virtual that the row fills or forwards
  to (slot mapping in `## Dispatch tables`), (c) a public descriptor field the row copies
  to or from (`NxJointDesc`/`NxRevoluteJointDesc` offsets). Anything else is `rowNNNNNN`.
- Calling conventions come from the Capstone listing (`ret N`, `ecx` live on entry), not
  from the supplement, whose rows were created under `-noanalysis` and say `unknown` /
  `2147483647`. The supplement prototypes for 004328, 004334, 004336, 004338, 004356,
  004360, 004364, 004721, 004723 are wrong (they show `void`/no arguments); the listing
  wins (for example 004334 is `__thiscall`, one stack argument, `ret 4`).
- **Dependency edges in the bundles include vtable installs.** `dependencies.dot` records
  an edge from a constructor to every slot of the table it installs. So "004366 calls
  004374" and "004725 calls 004437" are table-install edges, not calls. Only the listing
  says which edges are real calls.

## Row assignment

### Decisions on the ambiguous and neighbouring rows

- **`phys_fn_004372` (0xac700, 2467 B) → `core/RevoluteJoint.cpp`.** Its only caller is
  `phys_fn_004721` (0xb3352 `call 0x100ac700`), which is slot 39 of the NpRevoluteJoint
  table 002727 = `NxRevoluteJoint::getAngle`. It follows 004370 directly (004370 ends at
  0xac6fa, int3 padding to 0xac700) and reads the Joint frame fields (+0x08, +0xcc, +0xe4,
  body+0x198 against +0x14c) the way the other revolute rows do. It has no assert string,
  which is why the evidenced span stopped at 0xac630.
- **`phys_fn_004374` (0xad0b0, 1068 B) → `core/RevoluteJoint.cpp`.** It is slot 0 of the
  revolute internal table: the relocated word at 0x11a1c0 (the first word of
  `phys_data_002684`) is 0x100ad0b0. It reads and writes revolute-only fields
  (+0x1ac..+0x200 of the 0x204-byte object) and calls revolute row 004358 (0xad0ca). It
  ends at 0xad4dc; `core\PrismaticJoint.cpp`'s evidenced span begins at 0xad4e0. The
  work-unit tool could not claim it because its only incoming edge is the table-install
  edge from 004366 and its outgoing edges go to 004358 (revolute) and 000571 (Scene).
- **`phys_fn_004675/004677/004679` (0xb2c80–0xb2d04) → NOT pilot rows; they belong to
  `core\NpCylindricalJoint.cpp`** (work_units already puts them in that unit's inferred
  extent). 004675 is a constructor with the same shape as 004725 but installs
  0x1011b198 (`phys_data_002724`, the NpCylindricalJoint table, whose slot 0 is 0xb2cd0 =
  004679) and 0x1011b21c at +0xc (0xb2ca0); 004677 is its `sub ecx,0xc` adjustor thunk.
  Every `Np*Joint.cpp` unit ends with the same constructor / thunk / deleting-destructor
  triple after its accessor rows, which is what makes 004725/004727/004729 NpRevoluteJoint's.
- **`phys_fn_004719..004729` (0xb3310–0xb3424) → `core/NpRevoluteJoint.cpp`.** 004719,
  004721 and 004723 are slots 38, 39, 40 of table 002727 (`getSpring`, `getAngle`,
  `getVelocity`) and call revolute rows 004350, 004372, 004354. 004725 installs 0x1011b328
  (002727) at 0xb33cd and is called by 004366 (0xac5c0). 004727 is slot 0 of the secondary
  table at 0x1011b3dc. 004729 is slot 0 of 002727. `core\NpPrismaticJoint.cpp` begins
  at 0xb3430.
- **Joint rows named in the brief.** Per `work_units.json`, `004093 004097 004123 004127
  004129 004135` are in the `Joint.cpp` unit's inferred extent. `004064 004066` are in
  `gap:NpSpringAndDamperEffector.cpp..Joint.cpp`; `004389 004391 004393` are in
  `gap:core\PrismaticJoint.cpp..core\NpD6Joint.cpp`. Decided:
  - `004064`, `004066` (and `004070`, pulled in by the folded Np accessors) →
    `core/Joint.cpp`. They are `Joint` members: `this+8`/`this+0xc` are the two body
    pointers and `this+0x168` the joint type that the Joint constructor 004141 writes;
    004066 is the base half of `saveToDesc` called by all ten joint `saveToDesc` rows
    (004330 calls it at 0xa8d80). They sit at 0x957a0–0x95a86, directly before the Joint.cpp
    span (0x95ab0). The oracle `__FILE__` for this unit is
    `\Epic\Novodex\SDKs\Physics\src\Joint.cpp` (not `core\`); the pilot still writes it as
    `Physics/src/core/Joint.cpp` per the design spec 4.4.
  - `004389`, `004391`, `004393` → `core/JointSupport.cpp`. They are not Joint members:
    their receiver is a different record (flags word at +0xc tested for bit 10, two body
    pointers at +0x10/+0x14, vectors at +0x00..+0x2c, outputs at +0x3c/+0x40 — 004389
    decompile, 004393 0xaf710), and their callers span every joint family plus the
    raycast gap rows 000879, 000885 and 000899 (call sites 0x1e2c9/0x1e46d, 0x1e8f2,
    0x1f8af/0x1f8fd). `dependencies.dot` attributes these call sites to 000883/000897; the
    listing puts them in 000879/000885/000899.

### Table

Every row the pilot writes, defers or must decide. Rows not in this table are not pilot
rows. `core/Joint.cpp`/`core/JointSupport.cpp` rows are written only if
`## Dependency closure` marks them `write`.

| Stable ID | RVA | Size | State | Assigned file | Evidence |
|---|---|---:|---|---|---|
| phys_fn_004328 | 0x000a8d20 | 21 | reconstructed | `core/RevoluteJoint.cpp` | internal table 002684 slot 1 (0x11a1c4); writes this+0x1ac..0x1b4, revolute-only fields |
| phys_fn_004330 | 0x000a8d40 | 281 | discovered | `core/RevoluteJoint.cpp` | 002684 slot 10; string "RevoluteJoint::saveToDesc" + __FILE__ RevoluteJoint.cpp |
| phys_fn_004332 | 0x000a8e60 | 171 | discovered | `core/RevoluteJoint.cpp` | called by 004366 (0xac5d5) and 004370 (0xac6f0); writes +0x16c..+0x1a8 |
| phys_fn_004334 | 0x000a8f10 | 147 | reconstructed | `core/RevoluteJoint.cpp` | 002684 slot 11; string "RevoluteJoint::setFlags" |
| phys_fn_004336 | 0x000a8fb0 | 7 | reconstructed | `core/RevoluteJoint.cpp` | 002684 slot 12; reached from Np getFlags 004703 via [vt+0x30] |
| phys_fn_004338 | 0x000a8fc0 | 62 | reconstructed | `core/RevoluteJoint.cpp` | 002684 slot 13; string "RevoluteJoint::setProjectionMode" |
| phys_fn_004340 | 0x000a9000 | 189 | discovered | `core/RevoluteJoint.cpp` | string "RevoluteJoint::setLimits"; called by 004709 |
| phys_fn_004342 | 0x000a90c0 | 58 | reconstructed | `core/RevoluteJoint.cpp` | called by 004711; reads +0x16c..+0x180, +0x1a8 |
| phys_fn_004344 | 0x000a9100 | 171 | discovered | `core/RevoluteJoint.cpp` | string "RevoluteJoint::setMotor"; tail-jumped from 004713 |
| phys_fn_004346 | 0x000a91b0 | 42 | reconstructed | `core/RevoluteJoint.cpp` | called by 004715; reads +0x184..+0x18c |
| phys_fn_004348 | 0x000a91e0 | 171 | discovered | `core/RevoluteJoint.cpp` | string "RevoluteJoint::setSpring"; called by 004717 |
| phys_fn_004350 | 0x000a9290 | 43 | reconstructed | `core/RevoluteJoint.cpp` | called by 004719; reads +0x190..+0x198 |
| phys_fn_004352 | 0x000a92c0 | 694 | discovered | `core/RevoluteJoint.cpp` | in evidenced span; called by 004362/004364 |
| phys_fn_004354 | 0x000a9580 | 197 | discovered | `core/RevoluteJoint.cpp` | in evidenced span; called by 004723 (Np getVelocity) |
| phys_fn_004356 | 0x000a9650 | 2303 | discovered | `core/RevoluteJoint.cpp` | 002684 slot 8 |
| phys_fn_004358 | 0x000a9f50 | 269 | discovered | `core/RevoluteJoint.cpp` | in evidenced span; called by 004374; reads +0x1dc..+0x1f0 |
| phys_fn_004360 | 0x000aa060 | 4460 | discovered | `core/RevoluteJoint.cpp` | 002684 slot 6 |
| phys_fn_004362 | 0x000ab1d0 | 1644 | discovered | `core/RevoluteJoint.cpp` | 002684 slot 7 |
| phys_fn_004364 | 0x000ab840 | 3326 | discovered | `core/RevoluteJoint.cpp` | 002684 slot 4 |
| phys_fn_004366 | 0x000ac540 | 162 | discovered | `core/RevoluteJoint.cpp` | installs 0x1011a1c0 (002684) at 0xac551; case 1 of 000665 switch (0x143e1) |
| phys_fn_004368 | 0x000ac5f0 | 56 | discovered | `core/RevoluteJoint.cpp` | 002684 slot 5; reinstalls 0x1011a1c0 at 0xac5f8 |
| phys_fn_004370 | 0x000ac630 | 202 | discovered | `core/RevoluteJoint.cpp` | 002684 slot 9; strings "RevoluteJoint::loadFromDesc" x2 |
| phys_fn_004372 | 0x000ac700 | 2467 | discovered | `core/RevoluteJoint.cpp` | DECIDED: called only by 004721 = NpRevoluteJoint slot 39 getAngle (0xb3352); reads Joint +0x8/+0xcc/+0xe4 frames; contiguous after 004370 (0xac630+202=0xac6fa, pad to 0xac700) |
| phys_fn_004374 | 0x000ad0b0 | 1068 | discovered | `core/RevoluteJoint.cpp` | DECIDED: 002684 slot 0 (0x11a1c0 -> 0xad0b0); reads/writes revolute-only +0x1ac..+0x200; ends 0xad4dc, PrismaticJoint.cpp span starts 0xad4e0 |
| phys_fn_004681 | 0x000b2d10 | 84 | discovered | `core/NpRevoluteJoint.cpp` | 002727 slot 2; __FILE__ NpRevoluteJoint.cpp |
| phys_fn_004683 | 0x000b2d70 | 84 | discovered | `core/NpRevoluteJoint.cpp` | 002727 slot 4; __FILE__ NpRevoluteJoint.cpp |
| phys_fn_004685 | 0x000b2dd0 | 89 | discovered | `core/NpRevoluteJoint.cpp` | 002727 slot 9; __FILE__ NpRevoluteJoint.cpp |
| phys_fn_004687 | 0x000b2e30 | 89 | discovered | `core/NpRevoluteJoint.cpp` | 002727 slot 11; __FILE__ NpRevoluteJoint.cpp |
| phys_fn_004689 | 0x000b2e90 | 97 | discovered | `core/NpRevoluteJoint.cpp` | 002727 slot 13; __FILE__ NpRevoluteJoint.cpp |
| phys_fn_004691 | 0x000b2f00 | 74 | reconstructed | `core/NpRevoluteJoint.cpp` | 002727 slot 15; __FILE__ NpRevoluteJoint.cpp |
| phys_fn_004693 | 0x000b2f50 | 88 | discovered | `core/NpRevoluteJoint.cpp` | 002727 slot 29; __FILE__ NpRevoluteJoint.cpp |
| phys_fn_004695 | 0x000b2fb0 | 74 | reconstructed | `core/NpRevoluteJoint.cpp` | 002727 slot 14; __FILE__ NpRevoluteJoint.cpp |
| phys_fn_004697 | 0x000b3000 | 84 | reconstructed | `core/NpRevoluteJoint.cpp` | 002727 slot 31; __FILE__ line 0x12 |
| phys_fn_004699 | 0x000b3060 | 84 | reconstructed | `core/NpRevoluteJoint.cpp` | 002727 slot 32; __FILE__ line 0x1d |
| phys_fn_004701 | 0x000b30c0 | 84 | reconstructed | `core/NpRevoluteJoint.cpp` | 002727 slot 41; __FILE__ line 0x25 |
| phys_fn_004703 | 0x000b3120 | 36 | reconstructed | `core/NpRevoluteJoint.cpp` | 002727 slot 42; also installed by the NpSphericalJoint ctor 004649 (folded), body lies in this unit's range |
| phys_fn_004705 | 0x000b3150 | 84 | reconstructed | `core/NpRevoluteJoint.cpp` | 002727 slot 43; __FILE__ line 0x32 |
| phys_fn_004707 | 0x000b31b0 | 36 | reconstructed | `core/NpRevoluteJoint.cpp` | 002727 slot 44 (folded like 004703) |
| phys_fn_004709 | 0x000b31e0 | 84 | discovered | `core/NpRevoluteJoint.cpp` | 002727 slot 33; __FILE__ NpRevoluteJoint.cpp |
| phys_fn_004711 | 0x000b3240 | 45 | reconstructed | `core/NpRevoluteJoint.cpp` | 002727 slot 34 |
| phys_fn_004713 | 0x000b3270 | 8 | discovered | `core/NpRevoluteJoint.cpp` | 002727 slot 35; jmp 004344 |
| phys_fn_004715 | 0x000b3280 | 45 | reconstructed | `core/NpRevoluteJoint.cpp` | 002727 slot 36 |
| phys_fn_004717 | 0x000b32b0 | 84 | discovered | `core/NpRevoluteJoint.cpp` | 002727 slot 37; __FILE__ NpRevoluteJoint.cpp |
| phys_fn_004719 | 0x000b3310 | 45 | reconstructed | `core/NpRevoluteJoint.cpp` | DECIDED: 002727 slot 38 (getSpring); calls revolute 004350 (0xb3327) |
| phys_fn_004721 | 0x000b3340 | 42 | discovered | `core/NpRevoluteJoint.cpp` | DECIDED: 002727 slot 39 (getAngle); calls revolute 004372 (0xb3352) |
| phys_fn_004723 | 0x000b3370 | 42 | discovered | `core/NpRevoluteJoint.cpp` | DECIDED: 002727 slot 40 (getVelocity); calls revolute 004354 (0xb3382) |
| phys_fn_004725 | 0x000b33a0 | 57 | discovered | `core/NpRevoluteJoint.cpp` | DECIDED: installs 0x1011b328 (002727) at 0xb33cd and 0x1011b3dc at 0xb33c0; called by 004366 (0xac5c0) |
| phys_fn_004727 | 0x000b33e0 | 8 | discovered | `core/NpRevoluteJoint.cpp` | DECIDED: secondary table 0x1011b3dc slot 0 (inside 002727 at +0xb4); sub ecx,0xc; jmp 004729 |
| phys_fn_004729 | 0x000b33f0 | 55 | discovered | `core/NpRevoluteJoint.cpp` | DECIDED: 002727 slot 0; reinstalls 0x1011b328/0x1011b3dc then NxJoint table 0x1011a680 |
| phys_fn_004064 | 0x000957a0 | 385 | discovered | `core/Joint.cpp` | DECIDED (gap row): transforms two points through body[0]/body[1] (this+8/+0xc, body+0x134..+0x160); callers 004356, 004298 (joint rows only) |
| phys_fn_004066 | 0x00095930 | 266 | discovered | `core/Joint.cpp` | DECIDED (work_units: gap NpSpringAndDamperEffector..Joint): base saveToDesc, called by all ten joint saveToDesc rows incl. 004330 (0xa8d80); writes NxJointDesc fields from Joint +0x3c..+0xa8/+0x2c/+0x48 |
| phys_fn_004070 | 0x00095a80 | 7 | reconstructed | `core/Joint.cpp` | DECIDED (gap row): returns +0x168, the NxJointType 004141 stores; called by folded Np getType/is (004443/004479) and by 004037 (gap NpSpringAndDamperEffector..Joint, not Np) |
| phys_fn_004074 | 0x00095ab0 | 216 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; setBreakable (Np slot 9 004685) |
| phys_fn_004076 | 0x00095b90 | 21 | reconstructed | `core/Joint.cpp` | Joint.cpp per work_units; getBreakable (folded Np slot 10) |
| phys_fn_004078 | 0x00095bb0 | 10 | reconstructed | `core/Joint.cpp` | Joint.cpp per work_units; getState (folded Np slot 8) |
| phys_fn_004080 | 0x00095bc0 | 208 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; getLimitPoint (folded Np slot 12) |
| phys_fn_004081 | 0x00095c90 | 9 | reconstructed | `core/Joint.cpp` | Joint.cpp per work_units; resetLimitPlaneIterator (Np slot 15 004691) |
| phys_fn_004083 | 0x00095ca0 | 14 | reconstructed | `core/Joint.cpp` | Joint.cpp per work_units; hasMoreLimitPlanes (folded Np slot 16) |
| phys_fn_004087 | 0x00095cc0 | 87 | reconstructed | `core/Joint.cpp` | Joint.cpp per work_units; internal table slot 3 (base and revolute) |
| phys_fn_004089 | 0x00095d20 | 58 | reconstructed | `core/Joint.cpp` | Joint.cpp per work_units; purgeLimitPlanes (Np slot 14 004695; tail of 004095) |
| phys_fn_004093 | 0x00095da0 | 116 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; callers are 14 joint rows incl. 004360/004362 |
| phys_fn_004095 | 0x00095e20 | 41 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; base destructor body: reinstalls 0x101192d0 (0x95e26); called by 004368 |
| phys_fn_004097 | 0x00095e50 | 1176 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; per-body frame refresh (arg i), 36 joint callers |
| phys_fn_004099 | 0x000962f0 | 1112 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; setGlobalAnchor body (Np slot 2 004681) |
| phys_fn_004101 | 0x00096750 | 5302 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; setGlobalAxis body (Np slot 4 004683) |
| phys_fn_004107 | 0x00097d30 | 297 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; called by 004141 (0x99ed3) and 004370 (0xac6e0) |
| phys_fn_004109 | 0x00097e60 | 366 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; setLimitPoint body (Np slot 11 004687) |
| phys_fn_004111 | 0x00097fd0 | 113 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; internal table slot 2 (base and revolute) |
| phys_fn_004121 | 0x000987a0 | 1084 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; called by 004141 (every case) and 004370 (0xac6e8) |
| phys_fn_004123 | 0x00098be0 | 518 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; `ret 4`, one output vec3 (Task 8b); called by 004364 |
| phys_fn_004125 | 0x00098df0 | 1940 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; getGlobalAnchor body (folded Np slot 3 004437) |
| phys_fn_004127 | 0x00099590 | 235 | discovered | `core/Joint.cpp` | Joint.cpp per work_units |
| phys_fn_004129 | 0x00099680 | 787 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; getGlobalAxis body (via Np slot 5 004441) |
| phys_fn_004131 | 0x000999a0 | 260 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; called by 004145 |
| phys_fn_004133 | 0x00099ab0 | 134 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; Joint base table slot 6 |
| phys_fn_004135 | 0x00099b40 | 701 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; Joint base table 0x1192d0 slot 7 |
| phys_fn_004137 | 0x00099e00 | 41 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; getGlobalAnchorVal body (folded Np slot 6) |
| phys_fn_004139 | 0x00099e30 | 41 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; getGlobalAxisVal body (folded Np slot 7) |
| phys_fn_004141 | 0x00099e60 | 464 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; Joint ctor: installs 0x101192d0 (0x99e70); called by 004366 (0xac54c) |
| phys_fn_004143 | 0x0009a0d0 | 860 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; addLimitPlane body (Np slot 13 004689) |
| phys_fn_004145 | 0x0009a430 | 174 | discovered | `core/Joint.cpp` | Joint.cpp per work_units; getNextLimitPlane body (folded Np slot 17) |
| phys_fn_004389 | 0x000af2d0 | 227 | discovered | `core/JointSupport.cpp` | DECIDED (gap PrismaticJoint..NpD6Joint): operates on a separate record (flags +0xc, body ptrs +0x10/+0x14), callers 004310, 004362, 004397, 004399 (joint rows) + raycast row 000899 (0x1f8fd) |
| phys_fn_004391 | 0x000af3c0 | 837 | discovered | `core/JointSupport.cpp` | DECIDED (gap row): same record type as 004389; callers across joint families, Joint.cpp 004135, and raycast rows 000879 (0x1e2c9, 0x1e46d), 000899 (0x1f8af) |
| phys_fn_004393 | 0x000af710 | 122 | discovered | `core/JointSupport.cpp` | DECIDED (gap row): calls 004391, writes record +0x3c/+0x40; raycast caller 000885 (0x1e8f2) |
| phys_fn_000022 | 0x00001840 | 27 | discovered | `core/JointSupport.cpp` | deferred stub only (owner gap <start>..Actor.cpp); called by 004356 |
| phys_fn_000571 | 0x000108e0 | 22 | reconstructed | `core/JointSupport.cpp` | deferred stub only (owner Scene.cpp; link-insert at Scene+0x620); called by 004374, 004111 |
| phys_fn_000633 | 0x00012660 | 370 | discovered | `core/JointSupport.cpp` | deferred stub only (owner Scene.cpp; joint removal); called by 004107 (third arg false), 004095 |
| phys_fn_000758 | 0x00017630 | 214 | discovered | `core/JointSupport.cpp` | deferred stub only (owner gap SceneRaycast..CapsuleShape); called by 004356 |

The State column above is a snapshot taken when the contract was written (Task 4). `inventory.json` is authoritative for current row states; Task 11 moved the written rows to `reconstructed`.

`phys_fn_004675/004677/004679` are deliberately absent (NpCylindricalJoint.cpp, see above).
The 13 folded NpJoint accessor bodies that table 002727 borrows from other units are
listed in `## Dispatch tables` and `## Task split`; they are implemented as
NpRevoluteJoint methods but **not claimed** (their rows stay with their own units).

Joint.cpp inferred-extent rows 004085, 004091, 004103, 004105, 004113, 004115, 004117 and
004119 are out of scope: no pilot row calls them directly (004091, 004113 and 004119 are
reached only through deferred 004111; 004105 only through 004113; 004085/004103 from
outside the joint units; 004115/004117 have no callers). 004119 is the Joint base scalar
deleting destructor in base slot 5; no pilot path reaches it through the vtable, because
RevoluteJoint overrides slot 5 with 004368, which calls the destructor body 004095
directly, so release (if Task 10 wires it) goes 004368 → 004095, never 004119.

### Declaration changes made by Task 6

Recorded here as the procedure requires; the headers are `Physics/src/include/core/Joint.h`
and the new `Physics/src/include/core/JointSupport.h`.

- `Joint::mUnknown014[3]` → `NxVec3 mLimitPoint` (+0x14; evidence: 004080, table above).
  `mLimitPlaneHead` is typed `JointLimitPlane*` (new 0x14-byte node struct in `Joint.h`).
- New read view `JointBodyRecord` in `Joint.h` for the body record `mBody[i]` points to
  (offset-asserted: +0x4c, +0x50, +0x5c, +0xdc, +0x100, +0x10c, +0x114, +0x134, +0x158,
  +0x198, +0x19c). Nothing constructs it.
- `row004087(NxU32, NxU32, NxU32)` → `row004087(NxReal numerator, const NxVec3& v, NxReal divisor)`
  (listing: `fld arg1; fdiv arg3`, arg2 dereferenced; `ret 0xc` unchanged).
- `row004127(NxU32)` → `void row004127(NxVec3& out) const` (`ret 4`, one output pointer).
- `row004131(NxU32)` → `NxF64 row004131(const JointLimitPlane*, const NxVec3& point, NxVec3& planeNormal, NxReal& planeD)`
  (`ret 0x10`; the st(0) result is compared unrounded by 004145, hence `NxF64`).
- `row004107` parameters renamed `actorImpl0/actorImpl1` (they are `desc.actor[i]+0x14`; the
  row stores their `+8`). Types unchanged.
- `JointSupport.cpp` rows are members of `JointSupportRecord` (offset view of the record:
  +0x00 vec3, +0x0c flags, +0x10/+0x14 `JointSupportBody*`, +0x18/+0x24 vec3, +0x3c/+0x40
  outputs): `NxF64 row004389() const`, `void row004391(NxReal&, NxReal&)` (deferred; `ret 8`),
  `void row004393(NxReal, NxReal)` (`ret 8`). 000633 became `Row000633Fixture::row000633(void* joint)`
  (thiscall on the Scene, `ret 4`, as 000571). All declared in `JointSupport.h`.
- `Joint::getGlobalAnchor/getGlobalAxis/row004127` stay `const`; the stale-body refresh
  they begin with (004097) goes through a `const_cast`, as the oracle row mutates the cache.

### Declaration changes made by Task 7

Headers `Physics/src/include/core/RevoluteJoint.h` and `Physics/src/include/core/Joint.h`.

- `RevoluteJoint::mLimit/mMotor/mSpring` are now `NxJointLimitPairDesc`/`NxMotorDesc`/`NxSpringDesc`
  (the local `LimitLeg`/`Motor`/`Spring` structs are gone; offsets unchanged). Evidence: 004366
  0xac559–0xac5a4 stores exactly the values the three inline default constructors store, and
  every row copies the blocks dword-for-dword; `Motor::freeSpin` had been typed `NxReal`,
  but it is `NX_BOOL`.
- `void row004352()` → `NxF64 row004352()` (`this` in ecx, no stack args, plain `ret`, result
  unrounded in st(0)).
- `NxReal getVelocity() const` → `NxF64 getVelocity() const` (004354 returns st(0) unrounded;
  the Np caller 004723 rounds it with its own `fstp`).
- `void row004358()` → `void row004358(NxVec3& out) const` (`ret 4`, one output vec3).
- `Joint` gains `static void operator delete(void*)` → SDK allocator free (slot +0x14), so the
  compiler's deleting destructors free as 004119 (0x98789) and 004368 (0xac61f) do.
- `JointBodyRecord` gains `NxVec3 mAngularVelocity` at +0x78 (004354; the candidate stores the
  body descriptor's angularVelocity there) and `JointBodyRecord204* mUnknown204` at +0x204
  (004358, and 004374 per the field list below). New read view `JointBodyRecord204`: vec3 at
  +0x00 and +0x10 (`mUnknown000`, `mUnknown010`; 004358 forms `mUnknown000 + mUnknown010 × r`).
- 004352 does not use `NxMath::acos(NxF32)` as the `reuse` table says: the listing leaves the
  acos result unrounded, so the same clamp is inlined at double precision. (Task 8a: 004330
  no longer uses it either; both sites call the file-static `revoluteAcos` clamp over
  `revoluteCIacos`, the x87 `_CIacos` sequence. See the `reuse` table.)

### Declaration changes made by Task 8a

Headers `Physics/src/include/core/Joint.h`, `JointSupport.h`, `RevoluteJoint.h`.

- `JointBodyRecord204` is gone: it is the same record as `JointSupportBody`. 004360 (0xaa0ac,
  0xaa169) and 004362 (0xab1e2, 0xab1f9) copy body +0x204 into `JointSupportRecord::mBody`,
  whose pointee 004389 reads. `JointBodyRecord::mUnknown204` is now `JointSupportBody*`;
  `JointSupportBody` gains `NxReal mUnknown00c` (was an unread `NxU32`, now the factor 004374
  multiplies its impulse by and tests against 0), `mUnknown01c` (unread) and
  `NxReal mUnknown020[9]` (row-major 3x3, 004374 0xad345-0xad3a9).
- `JointBodyRecord` gains `NxReal mInverseMass` (+0xc0) and `NxReal mWorldInverseInertia[9]`
  (+0x164..+0x184), the scalar and 3x3 004360 builds the +0x1b8 matrix from. Names from the
  candidate's writers: `Scene.cpp:1905` stores 1.0f / mass at +0xc0, and
  `nxNpActorUpdateInertiaMatrices` (`Physics/src/include/NpActorDynamicMath.h:59–72`, called
  at `Scene.cpp:1917` and `NpActor.cpp:1247`) stores the world inverse inertia at +0x164 from
  the +0xc4 diagonal and the +0x134 rotation.
- `JointSupportRecord` is 0x50 bytes (004093 returns `[Scene+0x5b8] + index * 0x50`,
  0x95dde-0x95df5); +0x30 `void* mUnknown030` (the joint), +0x34/+0x38 `NxReal`, +0x44 `NxU32`,
  +0x48 `NxReal`, +0x4c `NxU32` replace the old +0x30 padding. Bits 0-4 of +0x0c are a kind the
  rows test (0/2 and 1/3 select the +0x40 scale, see 004360/004362).
- `void row004093(NxU32)` → `JointSupportRecord* row004093()` (no stack arguments, plain `ret`;
  still deferred).
- `row_slot6`/`row_slot7` take `NxReal` (Joint and RevoluteJoint): 004360 and 004362 divide by the
  argument (0xaa2a7, 0xab261); 004362 passes it on unchanged to the base slot-7 body 004135.
  `row_slot0` keeps `NxU32`: 004374 never reads its argument.
- New `JointBreakEvent` in `Joint.h` (the 0x101192cc object: vptr, +4 list link, +8 joint,
  +0xc float; slot 0 = row 004113, declared inline as an asserting body, not claimed).
  `Row000571Fixture::row000571`'s parameter is the event, not the joint (000571 writes the
  argument's +4, 0x108ea).

### Declaration changes made by Task 8b

Headers `Physics/src/include/core/Joint.h`, `JointSupport.h`, `RevoluteJoint.h`.

- `row_slot4(NxU32)` → `row_slot4(NxDebugRenderable& renderable)` (Joint, pure; RevoluteJoint):
  004364 calls the argument's slots +0x20 and +0x30 with exactly the argument shapes of the
  public `NxDebugRenderable::addLine(p0, p1, color)` and `addArrow(position, direction,
  length, scale, color)` (`Foundation/include/NxDebugRenderable.h`, slots 8 and 12), for
  example 0xab8c9-0xab8e6. Slot 4 is the joint's debug visualization (see the table below).
- `row_slot8(NxU32)` → `row_slot8(void* body)` (Joint default, RevoluteJoint): 004356 compares
  the argument with `mBody[0]`/`mBody[1]` (0xa96e4, 0xa9916, 0xa991b) and writes the record
  through it (+0x124..+0x160). Typed `void*` like `mBody`.
- `NxReal getAngle() const` → `NxF64 getAngle() const`: 004372 returns st(0) unrounded
  (0xad08b/0xad09a `fmulp`, `ret`); 004721 rounds it (`fstp dword [esp+8]`, 0xb3357) and
  reloads it after the unlock (0xb3362).
- `row004064(NxVec3&, NxVec3&, const NxVec3&, const NxVec3&)` →
  `row004064(const NxVec3& anchor0, const NxVec3& anchor1, NxVec3& out) const`: `ret 0xc`
  (0x958fc), three pointers; 004356 passes `&mWorldAnchor[0]`, `&mWorldAnchor[1]`, a local
  (0xa968c-0xa96a1). Still deferred.
- `row004123(NxU32)` → `row004123(NxVec3& out)`: `ret 4` (0x98de3), one output vec3 (manifest
  prototype `FUN_10098be0(float*)`). Still deferred.
- `JointBodyRecord` gains `NxReal mCMassOrientation[4]` at +0x124 (x, y, z, w): 004356 writes it
  (0xa9f10-0xa9f24) and has 000758 rebuild the +0x134 3x3 from it (000758 reads +0x124..+0x130
  as x, y, z, w); the name is the candidate's writer, `nxNpActorUpdateCMassQuaternion`
  (`Physics/src/include/NpActorDynamicMath.h`), which fills it from +0x134.
- `row000022()` → `Row000022Fixture::row000022(NxU32)`: thiscall on the body's +0x19c owner,
  one stack argument (`push 1` at 0xa9f3b), `ret 4` (0x1858). `row000758()` →
  `Row000758Fixture::row000758()`: `this` (the body record) in ecx (0xa9f0e), plain `ret`
  (0x17705). Both keep the 000571 fixture convention; both still deferred.

### Declaration changes made by Task 9

Header `Physics/src/include/core/NpRevoluteJoint.h`.

- `NpRevoluteJoint` gains `static void operator delete(void* p) { nxGetSdkAllocator()->free(p); }`,
  the same declaration `Joint.h` adds for `Joint`'s deleting destructors. The compiler-generated
  scalar deleting destructor that wraps `~NpRevoluteJoint()` (phys_fn_004729's tail, 0xb3416-
  0xb3423) frees through `operator delete` when its flag bit is set; without this declaration
  that call resolves to the global operator delete (a plain CRT free) instead of the SDK
  allocator the oracle uses (`[[0x101041bc]]` slot +0x14).
- `NpRevoluteJoint` gains two private accessors, `void* writeLink() const { return
  reinterpret_cast<void*>(mWord04); }` and `void* readLink() const { return
  reinterpret_cast<void*>(mWord08); }`. `mWord04`/`mWord08` (+0x10/+0x14) do not hold the
  lock block directly; per "## Object layouts" each holds a pointer to a one-word link
  whose word points to the lock block, and that is exactly the `link` value the
  `nxNpSceneGuardEnter`/`nxNpSceneGuardWriteTry`/`nxNpSceneGuardLeave` helpers
  (`NpSceneGuard.h`) take. Every locked body in `NpRevoluteJoint.cpp` calls `writeLink()`/
  `readLink()` once, before the guarded work, and passes that captured value to the guard
  calls -- reproducing the listing's `mov ecx,[esi+0x10]` / `[esi+0x14]` reads (e.g.
  phys_fn_004681 0xb2d16) exactly, rather than passing the field's own address.
- Beyond the `operator delete` and the `writeLink()`/`readLink()` accessors above, no
  other declaration changes were needed: `NpRevoluteJoint`'s constructor needs no explicit
  vtable or `userData`/`appData` code -- `NxJoint()`'s inline default constructor (already
  in the immutable public header) zeroes `userData`/`appData` as part of ordinary base
  construction, and every vtable phys_fn_004725/phys_fn_004729 install is the vtable C++
  installs automatically for this base/derived shape (see the constructor and destructor
  bodies' comments in `Physics/src/core/NpRevoluteJoint.cpp`). Only the hook base's two
  words (`mWord04`/`mWord08`) need an explicit zero in the constructor, since
  `EmbeddedHookBase` has no constructor of its own; this reproduces phys_fn_002404's
  zeroing without adding a constructor to the shared `EmbeddedHookBase` struct (out of
  this task's file scope, and other embedders such as `CollisionObject` deliberately skip
  it -- see `ObjectModel.cpp`'s `CollisionObject::CollisionObject` comment).
- `nxLockedVtCallNoArg` and `nxLockedCopyAndFlag` (`ObjectModel.cpp`) gain a second pointer
  comment: their shape also covers phys_fn_004703/004707 and phys_fn_004711/004715/004719
  respectively, but those rows are written as direct calls through the named `RevoluteJoint`
  accessor (real C++ member/virtual calls through `mInternal`) rather than through the
  byte-offset model, since `NpRevoluteJoint` is real multiple-inheritance C++, not the generic
  `NpJointObject` byte array the model targets elsewhere.

### Declaration changes made by joint-families Task 1

Headers `Physics/src/include/core/NpRevoluteJoint.h` and the new
`Physics/src/include/core/NpJointShared.h`; details in `units/joint-families-contract.md`
`## Shared NpJoint slots`.

- `NpRevoluteJoint` now derives from `NpJointShared<NxRevoluteJoint, RevoluteJoint>`, a
  `__declspec(novtable)` template that itself derives from `NxRevoluteJoint` and
  `EmbeddedHookBase` in that order. The layout, the static_asserts and the table slot order
  are unchanged; `mInternal` (+0x18), `operator delete`, `writeLink()`/`readLink()` and the
  hook-word zeroing of the constructor moved into the base.
- The 13 folded bodies are no longer `NpRevoluteJoint` members: they are
  `NpJointShared` members defined in `Physics/src/core/NpJointShared.cpp`, which now claims
  them (stable-ID lines; `reconstructed`, `implementation` = that file). The statement in
  `### Table` that they are "implemented as NpRevoluteJoint methods but not claimed" is
  superseded.
- The revolute rows of slots 2, 4, 9, 11, 13, 14, 15, 29, 31 and 32 keep their stable-ID lines
  in `core/NpRevoluteJoint.cpp`; their bodies are one-line calls to `NpJointShared`'s shared
  `forward*` helpers with this unit's `__FILE__` and line. The other write-locked rows report
  through `reportWriteLocked`.

### Declaration changes made by joint-families Task 2

Details in `units/joint-families-contract.md` `## Shared rows`. Of the rows `## Dependency
closure` defers, 004064, 004093, 004099, 004101, 004109, 004111, 004123, 004133, 004135,
004143 (`core/Joint.cpp`) and 004391 (`core/JointSupport.cpp`) are now written, with 004091
(which 004111 calls); 000022, 000571, 000633 and 000758 stay deferred stubs, joined by
000598 (called by 004093). `Joint::row004111` now takes `(const JointSupportRecord*,
NxReal)`. The revolute rows that called those stubs (004356, 004360, 004362, 004364) now
reach real bodies there, except where they reach 000022, 000571, 000598 or 000758.

## Construction chain

The public call is `NxScene::createJoint(desc)` with `desc.type == NX_JOINT_REVOLUTE (1)`.
Ordered list, caller → callee (purpose), with the instruction that makes the call:

1. `phys_fn_000297` NpScene::createJoint (0xc560, `ret 4`) → `phys_fn_002364` on
   `[NpScene+0xc]` (0xc566, tryLock of the scene write lock; failure reports line 0x78 and
   returns 0).
2. `phys_fn_000297` → `phys_fn_000665` Scene::createJoint on `[NpScene+0x24]` (0xc5a5).
3. `phys_fn_000665`: re-entry guard `.data 0x10123c10`; `desc.isValid()` through
   `[desc vtable]+8` (0x1430e); dynamic-actor test on `[[desc+8]+0x14]+8` and
   `[[desc+0xc]+0x14]+8` (0x14323–0x1435b); `switch(desc+4)` through the table at 0x14590
   (0x14393). The ten cases and their allocation literals:

   | type | case target | alloc | constructor |
   |---|---|---:|---|
   | 0 PRISMATIC | 0x1439a | 0x17c | 004380 (0xad6e0) |
   | **1 REVOLUTE** | **0x143c2** | **0x204** | **004366 (0xac540)** |
   | 2 CYLINDRICAL | 0x143eb | 0x16c | 004320 |
   | 3 SPHERICAL | 0x14414 | 0x23c | 004300 |
   | 4 POINT_ON_LINE | 0x1443d | 0x16c | 004276 |
   | 5 POINT_IN_PLANE | 0x14466 | 0x16c | 004262 |
   | 6 DISTANCE | 0x144b2 | 0x184 | 004234 |
   | 7 PULLEY | 0x144d8 | 0x1e0 | 004222 |
   | 8 FIXED | 0x1448c | 0x188 | 004250 |
   | 9 D6 | 0x14554 | 0x270 | 004210 |

4. `phys_fn_000665` → SDK allocator `[[0x101041bc]]` slot +8 with `(0x204, 0)` (0x143cc–0x143d3;
   candidate: `nxGetSdkAllocator()->malloc(0x204, NX_MEMORY_PERSISTENT)`).
5. `phys_fn_000665` → `phys_fn_004366` RevoluteJoint::RevoluteJoint(const NxRevoluteJointDesc&)
   on the new block (0x143e1; `__thiscall`, `ret 4`).
6. `phys_fn_004366` → `phys_fn_004141` Joint::Joint(const NxJointDesc& desc, NxU32 typeBit)
   with typeBit 0x40 (0xac547 `push 0x40`, call at 0xac54c; `__thiscall`, `ret 8`). 004141
   stores typeBit at +0x04 and the base vptr 0x101192d0 at +0x00, zeroes +0x48 +0x2c +0x10
   +0x34 +0x38 +0x154..+0x15c +0x14..+0x1c +0x20 +0x44, then:
   1. `phys_fn_004141` → `phys_fn_004107` (0x99ed3) with
      `([desc.actor[0]+0x14], [desc.actor[1]+0x14], true)` (null actor → 0; `__thiscall`,
      `ret 0xc`): stores each argument's `+8` (the body) at +0x08/+0x0c, sets +0x14c/+0x150
      to -1, orders +0x24/+0x28 by +0x2c bit 1, and raises each body's `+0x4c` to 0.4f
      (0x3ecccccc) when it is below that and body `+0x114` bit 8 is clear. With the third
      argument true it neither detaches (000633) nor registers (000661).
   2. `phys_fn_004141` maps typeBit to NxJointType at +0x168 (0x99ed8–0x9a01f; 0x40 → 1,
      mapping and evidence in `## Object layouts`) and, on every case, →
      `phys_fn_004121(desc)` (`__thiscall`, `ret 4`): the base part of loadFromDesc —
      copies localNormal/localAxis/localAnchor, computes per body the cross product and
      the frame quaternion, and for each body either copies the local block to the world
      block (null body) or → `phys_fn_004097(i)` (per-body frame refresh, `ret 4`);
      copies maxForce/maxTorque to +0x3c/+0x40; writes desc.userData to `[this+0x48]+4`
      **only if +0x48 is non-null — it is still null at this point**; →
      `phys_fn_000480(this, desc.name)` (name binding); maps desc.jointFlags bits 0/1 to
      +0x2c bits 8/9; raises the bodies' +0x4c as above.
7. `phys_fn_004366` stores vptr 0x1011a1c0 (`phys_data_002684`, 0xac551) and initialises
   +0x16c..+0x198 (limit 0, 0, 1.0f, 0, 0, 1.0f; motor 0x7f7fffff, 0, 0; spring 0, 0, 0 —
   0xac559–0xac5a4).
8. `phys_fn_004366` → SDK allocator slot +8 with `(0x1c, 0)` (0xac5b4) → `phys_fn_004725`
   NpRevoluteJoint::NpRevoluteJoint(RevoluteJoint*) (0xac5c0; `ret 4`), or null on
   allocation failure. 004725: zeroes +0x04/+0x08 (inlined `NxJoint()`), vptr 0x1011b238
   (inlined `NxRevoluteJoint()`, 0xb33b1), → `phys_fn_002404` on `this+0xc` (0xb33b7, hook
   base ctor: vptr 0x101088b8, zeroes +0x10/+0x14), secondary vptr 0x1011b3dc at +0xc
   (0xb33c0), internal pointer at +0x18 and +0x08 (0xb33c6, 0xb33c9), final vptr 0x1011b328
   (0xb33cd). Returns `this`.
9. `phys_fn_004366` stores the public object at `this+0x48` (0xac5c9) and `desc.userData`
   (desc+0x60) at `np+4` (0xac5cc–0xac5cf; no null check — a failed 0x1c allocation
   faults here in the oracle).
10. `phys_fn_004366` → `phys_fn_004332(desc)` (0xac5d5, `ret 4`): revolute part of
    loadFromDesc — desc+0x6c..+0x9c → +0x16c..+0x19c, `cos`/`sin` of desc.projectionAngle
    (x87 `fcos`/`fsin`) → +0x1a0/+0x1a4, desc.flags → +0x1a8, desc.projectionMode → +0x44.
    004366 returns `this` (0xac5db).
11. Back in `phys_fn_000665` (0x144fc): if `[joint+0x48]` is null → joint table slot 5
    (scalar deleting destructor, 004368) with 1, and **returns 0** (0x14581–0x1458c,
    `xor esi,esi`). Otherwise copies `[[Scene+0x6cc]+0xc]` → `np+0x10` and
    `[[Scene+0x6cc]+0x10]` → `np+0x14` (0x14509–0x14521; the NpScene write-lock and
    read-lock links, since Scene+0x6cc is the NpScene) and → `phys_fn_000661`
    Scene::addJoint (0x14524): refuses if joint+0x2c bit 0 is already set (line 0x752),
    else sets it, links joint+0x10 into the list headed at Scene+0x59c, appends to the
    array at Scene+0x58c, stores the Scene at joint+0x30.
12. `phys_fn_000665` increments Scene+0x6c8, copies Scene+0x59c to Scene+0x6bc, clears the
    re-entry flag and **returns the internal RevoluteJoint*** (0x1453c).
13. `phys_fn_000297` returns `[internal+0x48]` — **the 0x1c-byte NpRevoluteJoint** — after
    unlocking (0xc5ae–0xc5b9). That pointer is the user's `NxJoint*`; its vptr is 0x1011b328.

### Reconciliation with the candidate

- `Physics/src/Scene.cpp:1392–1411` says "the revolute case allocates 0x17c bytes and
  constructs through phys_fn_000ad6e0". That is **case 0, NX_JOINT_PRISMATIC** (0x1439a:
  `push 0x17c` … `call 0x100ad6e0` = `phys_fn_004380`, in `core\PrismaticJoint.cpp`). The
  revolute case is case 1: `push 0x204` at 0x143ce and `call 0x100ac540` at 0x143e1, which
  is the dependency edge `phys_fn_000665 → phys_fn_004366`. `Scene.cpp:1462`
  (`case 0: size = 0x17c; // revolute`) and every size in `nxJointSizeForType`
  (`Scene.cpp:2350`) disagree with the table above. `NpJointObject`'s 0x17c
  (`NpJoint.h:27`) is the prismatic internal size, not a revolute size of any object.
- The oracle's marker is dword index 0x12 = byte offset **+0x48**, the public-object
  pointer (000665 0x14502 `mov eax,[esi+0x48]`). The candidate's read at `Scene.cpp:1481`,
  `reinterpret_cast<unsigned*>(joint)[0x12 / 4]`, is dword index 4 = **byte +0x10**, which
  is wrong (it reads the Joint's scene-list link). Task 10 must read byte +0x48.
- The candidate comment at `Scene.cpp:1492–1504` says the oracle "falls through and returns
  the joint" when that word is null. The listing does not: 0x1458a zeroes `esi` and 0x1453c
  returns it, so the oracle returns 0.
- The candidate's Scene::createJoint returns the object it built and NpScene::createJoint
  (`NpScene.cpp:579`) returns it unchanged; the oracle's NpScene::createJoint returns
  `[internal+0x48]`.
- Scene+0x6cc already holds the NpScene in the candidate: the NxSceneInternal constructor
  zeroes it (`Scene.cpp:467`) and stores the wrapper (`Scene.cpp:494`,
  `p[0x1b3] = reinterpret_cast<unsigned>(wrapper)`, 0x1b3·4 = 0x6cc), on the live path from
  `PhysicsSDK.cpp:266`; `Scene.h:97` `setPublicScene` writes the same word, and
  `createActor` (`Scene.cpp:1286`) already copies holder[3]/holder[4] (NpScene
  `mWriteLock`/`mReadLock`) into each actor. Task 10 only needs to copy holder[3]/holder[4]
  into `np+0x10`/`np+0x14` inside Scene::createJoint, as the oracle does at 0x14509–0x14521.

### Wiring made by Task 10

`NxSceneInternal::createJoint` (`Physics/src/Scene.cpp`) now builds `NX_JOINT_REVOLUTE`
(descriptor word 1 == 1, the oracle's case 1) through the construction chain above:
`nxGetSdkAllocator()->malloc(sizeof(RevoluteJoint) /* 0x204, asserted */, NX_MEMORY_PERSISTENT)`,
placement `new RevoluteJoint(desc)` (004366, which builds the NpRevoluteJoint via 004725),
then byte +0x48 (`mPublicObject`): null → `delete internal` (the class deleting destructor
004368, freeing through `Joint::operator delete`) and a result of 0, as 0x14581-0x1458c;
otherwise holder[3]/holder[4] of Scene+0x6cc (NpScene `mWriteLock`/`mReadLock`) → np+0x10 /
np+0x14 (0x14509-0x14521), `nxSceneAddJoint(this, internal)` (000661's hole, still a no-op),
and the NpRevoluteJoint as the result. Every exit after the switch (success, allocation
failure, null +0x48) then runs step 12 (0x14529-0x1453f): `++[Scene+0x6c8]`,
`[Scene+0x6bc] = [Scene+0x59c]`, clear the re-entry flag, return the result. The oracle's Scene::createJoint returns
the internal joint and 000297 loads `[internal+0x48]`; the candidate does that load inside
Scene::createJoint so `NpScene::createJoint` stays unchanged for every type.

- The np+0x10/+0x14 stores and the `NxJoint*` conversion are done by
  `nxRevoluteJointAttachScene(RevoluteJoint*, void* writeLink, void* readLink)` (not an
  oracle row; declared in `core/RevoluteJoint.h`, defined in `core/NpRevoluteJoint.cpp`
  through `mWord04`/`mWord08`), because Scene.cpp cannot include `core/NpRevoluteJoint.h`:
  its `ObjectModel.h` declares `void nxActorConstruct(void*, void*)`, which collides with
  Scene.cpp's own `void* nxActorConstruct(void*, void*)`.
- The generic (non-revolute) path still lacks step 12 (the +0x6c8 increment and the
  +0x6bc copy); out of pilot scope, left unchanged.
- Every other type keeps the generic path (`nxJointConstruct` over `NpJointObject`).
  The candidate's `case 0: size = 0x17c; // revolute` comment now says prismatic;
  `nxJointSizeForType`'s sizes are unchanged (still not the oracle's literals), and its
  `case 1` is labelled revolute and unreachable.
- Release is not wired (`NpScene::releaseJoint` stays empty; open issue 2 stands).
- Phase 6: the staged-pair transcript is byte-identical to the oracle's on the first run
  (no transcript difference found). A cdb breakpoint trace over the candidate
  `NxPhysicsJointTests` run (addresses from `build/Release/NxPhysics.map`) hit, once per
  case, in order: Scene::createJoint, 004366, 004141, 004121, 004097 ×2, 004725, 004332,
  004437, 004125, 004441, 004129, 004483, 004078, 004539. 004107's out-of-line copy was
  not hit (inlined into 004141 or not separately observed); no destructor row was hit.

### What the staged-pair joint test reaches

`tests/PhysicsJointTests.cpp:116–156` (target `NxPhysicsJointTests`, also built as
`NxPhysicsJointStagedPairTests`) runs four revolute cases. Both actors are dynamic
(density 1, one unit box each; actor a at the origin, actor b at (4,0,0)). With revolute
creation wired to the new code, each case reaches exactly:

- `createJoint`: 000297, 000665, 004366, 004141, 004107 (third argument true), 004121,
  004097 ×2 (both bodies non-null), 000480, 004725, 002404, 004332, 000661, plus the SDK
  allocator twice.
- `getGlobalAnchor` → table 002727 slot 3 = `phys_fn_004437` (folded; 0xb0670) → 002362 on
  `np+0x14`, `phys_fn_004125(out)` on `np+0x18` (may call 004097), 002366.
- `getGlobalAxis` → slot 5 = `phys_fn_004441` (folded; 0xb0700) → 002362, `phys_fn_004129`
  (may call 004097), 002366.
- `getState` → slot 8 = `phys_fn_004483` (folded; 0xb0dc0) → 002362, `phys_fn_004078`, 002366.
- `getActors` → slot 1 = `phys_fn_004539` (folded; 0xb1600): `*a1 = *[[joint+8]+0x19c]`
  (0 if body null), `*a2` likewise from +0xc, under 002362/002366.
- `releaseJoint`: the candidate's `NpScene::releaseJoint` (`Physics/src/NpScene.cpp:204`) is
  an empty `(unimplemented)` body, so **no release row is reached** unless Task 10 wires
  it. The oracle path is 000299 → 000653 (Scene::releaseJoint: 000633 remove, then joint
  table slot 5 with 1) → 004368 → np table slot 0 with 1 = 004729 (→ 002406, free) →
  004095 (→ 000480(this, 0), 000633 if joint+0x30, then tail-jump 004089).

Pilot rows (by ID) on the transcript path: **004366, 004332, 004725** and the Joint rows
**004141, 004107, 004121, 004097, 004125, 004129, 004078**, plus the four folded Np bodies
(004437, 004441, 004483, 004539) that NpRevoluteJoint implements. Everything else in the
pilot is compiled but not executed by the test. The registered lines
(`tools/gate_targets.ps1:834–843`) pin `created=yes`, `out_anchor`/`out_axis`/`state=0` for
cases 0 and 3, and `actors a=match b=match`; Task 10 compares the whole transcript.

## Object layouts

Three objects. Sizes are established by allocation literals; every field lists the row
and instruction (or decompile line) that establishes it.

### NpRevoluteJoint — the public object, 0x1c bytes

Size: `push 0x1c` at 0xac5b4 (004366). Class shape: `NxRevoluteJoint` primary base
(vptr, `userData`, `appData` — the public header's members, `NxJoint.h:266–267`), then a
12-byte base with one virtual at +0xc (the `sub ecx,0xc` thunk 004727 is the
multiple-inheritance adjustor for its destructor), then one pointer.

| Off | Size | Field | Evidence |
|---|---:|---|---|
| +0x00 | 4 | vptr → 0x1011b328 (`phys_data_002727` primary, 45 slots) | 004725 0xb33cd; 004729 0xb33f6 |
| +0x04 | 4 | `NxJoint::userData` = desc.userData | 004725 zero 0xb33ab; 004366 0xac5cc–0xac5cf; 004121 writes `[joint+0x48]+4` |
| +0x08 | 4 | `NxJoint::appData` = the internal RevoluteJoint* | 004725 zero 0xb33ae, store 0xb33c9; 000299 passes `[np+8]` to Scene::releaseJoint (0xc60b) |
| +0x0c | 4 | hook base vptr → 0x1011b3dc (1 slot: 004727) | 004725 0xb33c0; 004729 0xb33fc; 002404/002406 install 0x101088b8 |
| +0x10 | 4 | scene write-lock link (tryLock 002364 / unlock 002366 in every setter) | 002404 zeroes (0x5ba7a); 000665 0x14512 copies `[[Scene+0x6cc]+0xc]`; 004681 0xb2d13 `mov ecx,[esi+0x10]` |
| +0x14 | 4 | scene read-lock link (lock 002362 / unlock 002366 in every getter) | 002404 zeroes (0x5ba7d); 000665 0x14521 copies `[[Scene+0x6cc]+0x10]`; 004719 0xb3315 |
| +0x18 | 4 | internal `RevoluteJoint*` every accessor forwards to | 004725 0xb33c6; 004713 0xb3270 `mov ecx,[ecx+0x18]` |

The +0x10/+0x14 links have the same meaning as `NpScene::mWriteLock` (+0x0c) and
`NpScene::mReadLock` (+0x10) (`Physics/src/include/NpScene.h:133–134`): each is a pointer
to a one-word link whose word points to the lock block, and the candidate's
`nxNpSceneGuardWriteTry` / `nxNpSceneGuardEnter` / `nxNpSceneGuardLeave`
(`Physics/src/include/NpSceneGuard.h`) are the 002364 / 002362 / 002366 operations on
exactly such links. The hook base (`vptr` + two words, ctor 002404, dtor 002406, table
0x101088b8 whose one slot is 002408) is the same 12-byte shape the candidate already
declares as `EmbeddedHookBase` (`Physics/src/include/ObjectModel.h:33`); its class name is
**unknown**.

### RevoluteJoint — the internal object, 0x204 bytes

Size: `push 0x204` at 0x143ce (000665 case 1). Base part (0x00–0x16b) is `Joint`, below.

| Off | Size | Field | Evidence |
|---|---:|---|---|
| +0x000 | 0x16c | `Joint` base | 004141 called first (0xac54c) |
| +0x16c | 0x18 | `limit` (NxJointLimitPairDesc copy: low.value, low.restitution, low.hardness, high.value, high.restitution, high.hardness) | 004332 from desc+0x6c..+0x80; 004340 setLimits; 004342 getLimits; 004330 save; ctor 0xac559–0xac580 = 0, 0, 1.0f, 0, 0, 1.0f |
| +0x184 | 0xc | `motor` (NxMotorDesc: velTarget, maxForce, freeSpin) | 004332 from desc+0x84..+0x8c; 004344/004346; ctor 0xac582 velTarget = 0x7f7fffff, then 0, 0 |
| +0x190 | 0xc | `spring` (NxSpringDesc: spring, damper, targetValue) | 004332 from desc+0x90..+0x98; 004348/004350; ctor zero 0xac598–0xac5a4 |
| +0x19c | 4 | `projectionDistance` | 004332 from desc+0x9c; 004330 to desc+0x9c; read by 004356 |
| +0x1a0 | 4 | cos(projectionAngle) | 004332 `fcos` of desc+0xa0; 004330 saves `acos` (via 005697) when in (-1,1), 0 when ≥ 1, π when ≤ -1; read by 004356 |
| +0x1a4 | 4 | sin(projectionAngle) | 004332 `fsin` of desc+0xa0; read by 004356 |
| +0x1a8 | 4 | `flags` (NX_RJF_*: bit0 limit, bit1 motor, bit2 spring) | 004332 from desc+0xa4; 004334 setFlags; 004336 getFlags; 004340/004344/004348 OR in 1/2/4; 004342/004346/004350 return bit 0/1/2; read by 004362 |
| +0x1ac | 0xc | **unknown** vec3 | zeroed by 004328 (slot 1); read by 004360, 004374 (0xad20a–0xad21e) |
| +0x1b8 | 0x24 | **unknown** 9 floats (3×3, multiplied as a matrix by 004374 0xad246–0xad29c) | read/written by 004360, 004374 |
| +0x1dc | 0xc | **unknown** vec3 | 004358, 004360, 004374 |
| +0x1e8 | 0xc | **unknown** vec3 | 004358, 004360, 004374 |
| +0x1f4 | 0xc | **unknown** vec3 (written by 004374 0xad1d3–0xad1fb) | 004360, 004374 |
| +0x200 | 4 | **unknown** float (compared with a squared length and `fsqrt`ed by 004374 0xad125–0xad17f) | 004360, 004374 |

Fields +0x1ac..+0x200 are not written by the constructor (only 004328 zeroes +0x1ac..+0x1b4)
and are not read on the transcript path. Touch lists above are a displacement scan of
the revolute rows' listings; the receiver register is `this` in each listed row.

### Joint — the internal base, 0x16c bytes

Size: the first revolute field is +0x16c (004366 0xac559) and 004141 writes up to +0x168.

| Off | Size | Field | Evidence |
|---|---:|---|---|
| +0x000 | 4 | vptr: base 0x101192d0; revolute 0x1011a1c0 | 004141 0x99e70; 004095 0x95e26; 004119 0x98756; 004366 0xac551; 004368 0xac5f8 |
| +0x004 | 4 | joint type bit (0x40 revolute; one bit per type, see mapping below) | 004141 0x99e6d (the constructor's second argument). Readers outside the ctor: **unknown** |
| +0x008 | 4×2 | body[0], body[1] — `[actorImpl+8]` for desc.actor[i]'s `+0x14`; 0 = world | 004107 0x97dc7, 0x97dea; 004066, 004064, 004539 read them |
| +0x010 | 4 | next joint in the Scene's list (head at Scene+0x59c) | 000661 0x13e4a; 000633 0x12678; 004141 zero 0x99e87 |
| +0x014 | 0xc | limit point in mSolverBody[0]'s frame (`mLimitPoint`; Task 6: 004080 getLimitPoint transforms it by that body's +0x134 pose into worldLimitPoint, or copies it) | 004141 zero 0x99ea2–0x99ea8; 004080 |
| +0x020 | 4 | limit-plane list head (`JointLimitPlane*`: 0x14-byte nodes, normal +0, d +0xc, next +0x10; 004143 `push 0x14`) | 004141 zero 0x99eab; 004081 copies it to `.data 0x10127180`; 004089 walks and frees it |
| +0x024 | 4×2 | body pair in solver order: +0x2c bit 1 set → (body[0], body[1]); clear → (body[1], body[0]) (Task 6 correction: the earlier text said "swapped when set"; 0x97de5 `je` takes the swapped arm when the bit is clear) | 004107 0x97de5–0x97dfa |
| +0x02c | 4 | flags: bit0 in scene (000661/000633); bit1 solver order (004107 0x97de5: clear → +0x24 = body[1], +0x28 = body[0]; set → body[0], body[1]); bit2 **unknown** (tested by 004133 0x99b22); bits3–4 state, `(>>3)&3` = NxJointState, 0x10 = broken (004078; every setter's `(+0x2c & 0x18) == 0x10` test; 004107 0x97da9 and 004374 0xad13b set broken); bit8/bit9 = NX_JF_COLLISION_ENABLED/NX_JF_VISUALIZATION (004121 from desc.jointFlags; 004066 back) | as listed |
| +0x030 | 4 | owning Scene* | 000661 0x13f1a; 004107 0x97d8b/0x97e3e; 004095 0x95e31; 004374 0xad170 |
| +0x034 | 4×2 | **unknown** (+0x34, +0x38) | 004141 zero 0x99e8a, 0x99e8d |
| +0x03c | 4 | maxForce | 004121 from desc+0x58; 004066; 004076 getBreakable; 004374 0xad0cf compares it |
| +0x040 | 4 | maxTorque | 004121 from desc+0x5c; 004066; 004076 |
| +0x044 | 4 | projectionMode (NxJointProjectionMode) | 004332 from desc+0xa8; 004338 setter; 004186 getter (table slot 14); 004141 zero 0x99eae |
| +0x048 | 4 | public object (`NpRevoluteJoint*` for this class) | 004366 0xac5c9; 004368 0xac5f3; 000665 0x14502; 004121 |
| +0x04c | 0xc×2 | localNormal[2] | 004121 from desc+0x10/+0x1c; 004066 to desc |
| +0x064 | 0xc×2 | localNormal[i] × localAxis[i] (name **unknown**) | 004121 computes |
| +0x07c | 0xc×2 | localAxis[2] | 004121 from desc+0x28/+0x34; 004066 |
| +0x094 | 0xc×2 | localAnchor[2] | 004121 from desc+0x40/+0x4c; 004066 |
| +0x0ac | 0x10×2 | frame quaternion[2] (x, y, z stored negated, w last) | 004121 (`pfVar7` block) |
| +0x0cc | 0x80 | world copy of the +0x4c..+0xcb block, same order (normal[2] +0xcc, cross[2] +0xe4, axis[2] +0xfc, anchor[2] +0x114, quat[2] +0x12c) | 004121 copies it when body[i] is null; 004097(i) otherwise; 004372 reads +0xcc/+0xe4 |
| +0x14c | 4×2 | body[i] stamp cache, compared with body+0x198; -1 forces a refresh | 004107 0x97dd6/0x97ddc; 004133, 004372 compare |
| +0x154 | 0xc | accumulated vec3 (`+= (a/b)·v` by table slot 3) | 004087; 004141 zero 0x99e90–0x99e9c |
| +0x160 | 4×2 | **unknown** (+0x160, +0x164) | no row read in this task touches them |
| +0x168 | 4 | NxJointType | 004141 per-case stores; 004070 getter |

**typeBit → NxJointType** (004141): 0x80→0, 0x40→1, 0x100→2, 8→3, 4→4, 2→5, 0x2000→6,
0x1000→7, 0x200→8, 0x4000→9. The ≥ 0x100 cases are explicit compares (0x99ed8–0x9a015).
The < 0x100 cases go through a byte table at 0x9a048 that is not in the pinned data;
the mapping is inferred from the jump table 0x9a030 (targets store 5, 4, 3, 1, 0 in
ascending address order) against the ten constructors' pushes (004262 `push 2`, 004276
`push 4`, 004300 `push 8`, 004366 `push 0x40`, 004380 `push 0x80`), which agree with
the Scene switch types. Task 6 read both tables from the shipped image: jump table 0x9a030 = 0x99f04, 0x99f1f, 0x99f3a, 0x99f55, 0x99f70, 0x9a01f (stores 5, 4, 3, 1, 0, default) and byte table 0x9a048 indexed by typeBit-2 = {2→0, 4→1, 8→2, 0x40→3, 0x80→4, all others 5}. The mapping above is confirmed.

**Body fields the Joint rows read** (the "body" is the 0x260-byte record the candidate
builds in `nxActorBuildBody`, `Scene.cpp:1777`, reached through `actor+0x14` → `+8`):
+0x4c (raised to 0.4f by 004107/004121/setters), +0x50..+0x58 position and +0x5c..+0x68
quaternion (004125, 004129; the candidate writes both, `Scene.cpp:1811–1823`),
+0xdc..+0xfc 3×3 and +0x100..+0x108 vec3 (004097, 004125, 004129; the candidate writes
massLocalPose there, `Scene.cpp:1830–1831`), +0x10c (bit 0x80 tested by 004133), +0x114
(bit 0x100), +0x134..+0x160 (3×3 + vec3; read by 004064, and by 004080, 004127, 004131 — Task 6), +0x198 (stamp; the
candidate sets 2, `Scene.cpp:1919`), +0x19c (pointer whose first word is the `NxActor*`;
the candidate's 0x50-byte body has `actor` at +0, `Scene.cpp:1784`), +0x78 (angular
velocity; 004354 — Task 7), +0x204 (pointer to a `JointSupportBody`: vec3s at +0x00/+0x10,
a float at +0x0c, a 3x3 at +0x20; read by 004358, written through by 004374, copied into
004093 records by 004360/004362; the candidate does not write it), +0xc0 (inverse mass;
the candidate writes it, `Scene.cpp:1905`) and +0x164..+0x184 (world inverse inertia 3x3;
the candidate writes it in `nxNpActorUpdateInertiaMatrices`, `NpActorDynamicMath.h:59–72`)
(both read by 004360 — Task 8a), +0x124..+0x130 (quaternion x, y, z, w of the +0x134 3x3; 004356 writes it and 000758 rebuilds +0x134 from it; the candidate writes it in `nxNpActorUpdateCMassQuaternion` — Task 8b).

## Dispatch tables

Contents are read from the relocated pointers in `oracle/pe.json` (one pointer per slot),
not from the inventory's `notes`, which list only the first eight targets. Two inventory
data objects span more than one table: `phys_data_002614` (0x1192cc, "10 slots") is a
one-slot table at 0x1192cc followed by the Joint base table at 0x1192d0, and
`phys_data_002727` (0x11b328, "46 slots") is the 45-slot NpRevoluteJoint primary table
followed by the one-slot secondary table at 0x11b3dc. There are no RTTI locator words
before any of these tables.

**The table installed on the object `createJoint` returns is 0x1011b328
(`phys_data_002727`, slots 0–44), the NpRevoluteJoint implementation of
`NxRevoluteJoint`.** 004725 stores it at 0xb33cd.

### 0x1011b328 — NpRevoluteJoint primary (`phys_data_002727` slots 0–44)

Slot order is the declaration order of `NxJoint.h` (dtor, then lines 67–263) followed by
`NxRevoluteJoint.h` (lines 32–148); no virtual is overloaded, so MSVC keeps declaration
order. "Folded" rows are identical-code-folded NpJoint bodies that live in another
unit's range; NpRevoluteJoint must still implement them (see `## Task split`).
`R` = read lock `np+0x14` (002362), `W` = write tryLock `np+0x10` (002364, failure reports
"PhysicsSDK: WriteLock is still aquired…" code 2 at `__FILE__` NpRevoluteJoint.cpp).

| Slot | Off | Virtual | Target | Unit of target | Body |
|---:|---|---|---|---|---|
| 0 | +0x00 | `~NxJoint()` (scalar deleting) | 004729 | NpRevoluteJoint | vptrs back, 002406 on +0xc, NxJoint table 0x1011a680, free if flag&1 |
| 1 | +0x04 | getActors | 004539 | NpDistanceJoint (folded) | R; `*[[j+8]+0x19c]`, `*[[j+0xc]+0x19c]` |
| 2 | +0x08 | setGlobalAnchor | 004681 | NpRevoluteJoint | W line 0xe; → 004099 |
| 3 | +0x0c | getGlobalAnchor | 004437 | NpD6Joint (folded) | R; → 004125 |
| 4 | +0x10 | setGlobalAxis | 004683 | NpRevoluteJoint | W line 0xe; → 004101 |
| 5 | +0x14 | getGlobalAxis | 004441 | NpD6Joint (folded) | R; → 004129 |
| 6 | +0x18 | getGlobalAnchorVal | 004497 | NpPulleyJoint (folded) | R; → 004137 (→ 004125) |
| 7 | +0x1c | getGlobalAxisVal | 004499 | NpPulleyJoint (folded) | R; → 004139 (→ 004129) |
| 8 | +0x20 | getState | 004483 | NpPulleyJoint (folded) | R; → 004078 |
| 9 | +0x24 | setBreakable | 004685 | NpRevoluteJoint | W line 0xe; → 004074 |
| 10 | +0x28 | getBreakable | 004573 | NpPointInPlaneJoint (folded) | R; → 004076 |
| 11 | +0x2c | setLimitPoint | 004687 | NpRevoluteJoint | W line 0xe; → 004109 |
| 12 | +0x30 | getLimitPoint | 004577 | NpPointInPlaneJoint (folded) | R; → 004080 |
| 13 | +0x34 | addLimitPlane | 004689 | NpRevoluteJoint | W line 0xe; → 004143 |
| 14 | +0x38 | purgeLimitPlanes | 004695 | NpRevoluteJoint | W line 0xe; → 004089, tail-jump unlock |
| 15 | +0x3c | resetLimitPlaneIterator | 004691 | NpRevoluteJoint | W line 0xe; → 004081, tail-jump unlock |
| 16 | +0x40 | hasMoreLimitPlanes | 004491 | NpPulleyJoint (folded) | R; → 004083 |
| 17 | +0x44 | getNextLimitPlane | 004635 | NpSphericalJoint (folded) | R; → 004145 |
| 18 | +0x48 | getType | 004443 | NpD6Joint (folded) | R; → 004070 |
| 19 | +0x4c | is(NxJointType) | 004479 | NpPulleyJoint (folded) | R; returns `this` if arg == 004070(), else 0 |
| 20–28 | +0x50–+0x70 | isRevoluteJoint … isPulleyJoint | 004417 … 004433 | gap PrismaticJoint..NpD6Joint (folded) | the inline bodies in `NxJoint.h:212–252`; compiler-generated, nothing to write |
| 29 | +0x74 | setName | 004693 | NpRevoluteJoint | W line 0xe; → 000480(`[np+0x18]`, name) |
| 30 | +0x78 | getName | 004743 | NpPrismaticJoint (folded) | R; → 000454(`[np+0x18]`) |
| 31 | +0x7c | loadFromDesc | 004697 | NpRevoluteJoint | W line 0x12; internal slot 9 (+0x24) |
| 32 | +0x80 | saveToDesc | 004699 | NpRevoluteJoint | W line 0x1d; internal slot 10 (+0x28) |
| 33 | +0x84 | setLimits | 004709 | NpRevoluteJoint | W line 0xe; → 004340 |
| 34 | +0x88 | getLimits | 004711 | NpRevoluteJoint | R; → 004342, returns its `al` |
| 35 | +0x8c | setMotor | 004713 | NpRevoluteJoint | **no lock**: `mov ecx,[ecx+0x18]; jmp 004344` |
| 36 | +0x90 | getMotor | 004715 | NpRevoluteJoint | R; → 004346 |
| 37 | +0x94 | setSpring | 004717 | NpRevoluteJoint | W line 0xe; → 004348 |
| 38 | +0x98 | getSpring | 004719 | NpRevoluteJoint | R; → 004350 |
| 39 | +0x9c | getAngle | 004721 | NpRevoluteJoint | R; → 004372, which returns `NxF64` (unrounded st(0)); 004721 rounds it with its own `fstp dword` (0xb3357) and reloads it after the unlock (0xb3362) |
| 40 | +0xa0 | getVelocity | 004723 | NpRevoluteJoint | R; → 004354, which returns `NxF64` (unrounded st(0)); 004723 rounds it with its own `fstp` |
| 41 | +0xa4 | setFlags | 004701 | NpRevoluteJoint | W line 0x25; internal slot 11 (+0x2c) |
| 42 | +0xa8 | getFlags | 004703 | NpRevoluteJoint | R; internal slot 12 (+0x30) |
| 43 | +0xac | setProjectionMode | 004705 | NpRevoluteJoint | W line 0x32; internal slot 13 (+0x34) |
| 44 | +0xb0 | getProjectionMode | 004707 | NpRevoluteJoint | R; internal slot 14 (+0x38) |

Every W row: on tryLock failure, `if(!FoundationSDK instance) int3`, then
`error(2, __FILE__, line, 0, "PhysicsSDK: WriteLock is still aquired. Procedure call skipped to avoid a deadlock!")`
and return (0 / nothing). Line 0xe is shared by the macro-generated rows; the four rows
with their own lines are listed. Candidate: `NxFoundation::FoundationSDK::getInstance().error(...)`
as in `Physics/src/PhysicsSDK.cpp:206`.

### 0x1011b3dc — NpRevoluteJoint secondary (hook base at +0xc; `phys_data_002727` slot 45)

| Slot | Target | Body |
|---:|---|---|
| 0 | 004727 | `sub ecx,0xc; jmp 004729` — adjustor thunk to the deleting destructor |

### 0x1011b238 — `NxRevoluteJoint` abstract (`phys_data_002725`, 45 slots)

Installed transiently by 004725 (0xb33b1) — the inlined `NxRevoluteJoint()` constructor.
Slot 0 = 004537 (NxJoint deleting dtor body: vptr 0x1011a680, free if flag&1), slots 1–19
and 29–44 = 005667 (`_purecall`), slots 20–28 = 004417–004433. Compiler-generated from the
public header; nothing to write.

### 0x1011a680 — `NxJoint` abstract (`phys_data_002700`, 31 slots)

Installed by 004729 (0xb340c) and 004537 — the inlined `~NxJoint()`. Same pattern as above
for slots 0–30. Compiler-generated.

### 0x1011a1c0 — RevoluteJoint internal (`phys_data_002684`, 17 slots)

Installed by 004366 (0xac551) and 004368 (0xac5f8). Slots 0–8 override the Joint base
table; slots 9–16 are RevoluteJoint's own. Names only where evidenced; otherwise the
virtual is **unknown** and is declared `rowNNNNNN`. Receivers of the internal slots, as
far as the listing shows: 000665/000653 call slot 5; the NpRevoluteJoint rows call 9–14;
004133 calls 6 and 7; the others are called from outside the pilot (scene/solver code).

| Slot | Off | Joint base (0x1192d0) | RevoluteJoint | Signature (listing) | Meaning |
|---:|---|---|---|---|---|
| 0 | +0x00 | 004248 (`ret 4`, FixedJoint.cpp, folded) | **004374** | thiscall, 1 arg (never read), `ret 4` | unknown (reads maxForce, may break the joint and post a 0x10-byte event through 000571; applies an impulse to both bodies' +0x204 records) |
| 1 | +0x04 | 001583 (`ret`, folded) | **004328** | thiscall, 0 args | unknown (zeroes +0x1ac..+0x1b4) |
| 2 | +0x08 | 004111 | 004111 (inherited) | thiscall, 2 args, `ret 8` | unknown (break test: sets broken, allocates the 0x101192cc event) |
| 3 | +0x0c | 004087 | 004087 (inherited) | thiscall, 3 args, `ret 0xc` | unknown (accumulates into +0x154) |
| 4 | +0x10 | `_purecall` 005667 | **004364** | thiscall, `NxDebugRenderable&`, `ret 4` | debug visualization (Task 8b): with +0x2c bit 9 set, draws world axes, local axes and the limit arc through the renderable's addArrow (+0x30) / addLine (+0x20), gated by SDK parameters 32, 31, 33 times 13 |
| 5 | +0x14 | 004119 | **004368** | thiscall, 1 arg (flags), `ret 4` | scalar deleting destructor (deletes `[this+0x48]` through its slot 0 with 1, then 004095, then frees if flag&1) |
| 6 | +0x18 | 004133 | **004360** | thiscall, 1 float arg (a divisor), `ret 4` | unknown (fills +0x1ac..+0x200 and two or three 004093 records: 0xaac53 only when the determinant is non-zero, then 0xab03c and 0xab0fc) |
| 7 | +0x1c | 004135 | **004362** | thiscall, 1 float arg (a divisor), `ret 4` | unknown (limit/motor/spring 004093 records) |
| 8 | +0x20 | 004248 (`ret 4`, folded) | **004356** | thiscall, body record (`void*`), `ret 4` | unknown name; projects the given body (Task 8b): moves its +0x158 by the anchor gap beyond projectionDistance, turns its axis to within projectionAngle (NxFindRotationMatrix, +0x124 quaternion, 000758), then 000022 on its +0x19c owner |
| 9 | +0x24 | — | **004370** | thiscall, `const NxRevoluteJointDesc&`, `ret 4` | loadFromDesc (strings) |
| 10 | +0x28 | — | **004330** | thiscall, `NxRevoluteJointDesc&`, `ret 4` | saveToDesc (string) |
| 11 | +0x2c | — | **004334** | thiscall, `NxU32`, `ret 4` | setFlags (string) |
| 12 | +0x30 | — | **004336** | thiscall, 0 args, returns `+0x1a8` | getFlags (called by Np getFlags 004703) |
| 13 | +0x34 | — | **004338** | thiscall, `NxJointProjectionMode`, `ret 4` | setProjectionMode (string) |
| 14 | +0x38 | — | 004186 (D6Joint.cpp, folded) | thiscall, 0 args, returns `+0x44` | getProjectionMode (called by Np 004707) |
| 15 | +0x3c | — | 001391 (`mov eax,ecx; ret`, folded) | thiscall, 0 args, returns `this` | unknown |
| 16 | +0x40 | — | 001391 (folded) | thiscall, 0 args, returns `this` | unknown |

Declare Joint's virtuals 0–8 in that order in `Joint.h` (slot 4 pure), and RevoluteJoint's
9–16 in that order in `RevoluteJoint.h`. The folded targets (004248, 001583, 004186,
001391) are written as inline bodies in the headers and are not claimed.

### 0x101192d0 — Joint base (inside `phys_data_002614`, 9 slots)

Installed by 004141 (0x99e70), 004095 (0x95e26) and 004119 (0x98756). Slots listed in the
table above (base column). Never the final table of a live object (slot 4 is pure).

### 0x101192cc — joint break event (inside `phys_data_002614`, 1 slot)

Slot 0 = 004113 (thiscall, no args): if the scene's user notify `[[event+8]+0x30]+0x6ac`
is set, calls its slot 0 with `([event+0xc], [joint+0x48])` under the re-entry flag and
releases the joint through 000653 when it returns true; otherwise tail-jumps to 004105.
Event object: 0x10 bytes — vptr, **unknown** word at +4 (the list link 000571 writes),
joint at +8, float at +0xc (004111 0x98019–0x98022, 004374 0xad160–0xad169). Allocated
only by 004111 and 004374, neither on the transcript path.

### 0x101088b8 — hook base (outside the pilot)

Installed by 002404/002406; slot 0 = 002408. Shape only; see `## Object layouts`.

## Dependency closure

Every callee of a pilot row that is not itself a pilot row — the bundles' "External
dependencies" sections, the same for the six NpRevoluteJoint rows outside the bundle
(004719–004729), and the bodies table 002727 borrows. Decision rule: `write` = on the
transcript path (see `## Construction chain`), or ≤ 300 B and needed by a written row or
by an NpRevoluteJoint virtual; `defer` = reached only on a path the joint transcript
cannot reach (named); `reuse` = an existing candidate symbol or code the compiler
generates. A `defer` row is still declared, with body `NX_ASSERT(0);` and the zero
return, preceded by its stable-ID comment line and `// (deferred: <path>)`, so pilot rows
can call it; it keeps its inventory state.

### write (Task 6)

| Row | RVA | B | File | Needed by | Name |
|---|---|---:|---|---|---|
| 004066 | 0x95930 | 266 | core/Joint.cpp | 004330 | Joint::saveToDesc (base part) |
| 004070 | 0x95a80 | 7 | core/Joint.cpp | Np getType/is | Joint::getType |
| 004074 | 0x95ab0 | 216 | core/Joint.cpp | 004685 | Joint::setBreakable |
| 004076 | 0x95b90 | 21 | core/Joint.cpp | Np getBreakable | Joint::getBreakable |
| 004078 | 0x95bb0 | 10 | core/Joint.cpp | Np getState (transcript) | Joint::getState |
| 004080 | 0x95bc0 | 208 | core/Joint.cpp | Np getLimitPoint, 004145 | Joint::getLimitPoint |
| 004081 | 0x95c90 | 9 | core/Joint.cpp | 004691 | Joint::resetLimitPlaneIterator |
| 004083 | 0x95ca0 | 14 | core/Joint.cpp | Np hasMoreLimitPlanes | Joint::hasMoreLimitPlanes |
| 004087 | 0x95cc0 | 87 | core/Joint.cpp | internal slot 3 | row004087 |
| 004089 | 0x95d20 | 58 | core/Joint.cpp | 004695, 004095 | Joint::purgeLimitPlanes |
| 004095 | 0x95e20 | 41 | core/Joint.cpp | 004368 | Joint::~Joint (body) |
| 004097 | 0x95e50 | 1176 | core/Joint.cpp | 004121 (transcript), 004125, 004129 | row004097 (per-body frame refresh, arg = body index) |
| 004107 | 0x97d30 | 297 | core/Joint.cpp | 004141 (transcript), 004370 | row004107 |
| 004121 | 0x987a0 | 1084 | core/Joint.cpp | 004141 (transcript), 004370 | row004121 (base part of loadFromDesc) |
| 004125 | 0x98df0 | 1940 | core/Joint.cpp | Np getGlobalAnchor (transcript) | Joint::getGlobalAnchor |
| 004127 | 0x99590 | 235 | core/Joint.cpp | 004362, 004364 | row004127 |
| 004129 | 0x99680 | 787 | core/Joint.cpp | Np getGlobalAxis (transcript), 004354 | Joint::getGlobalAxis |
| 004131 | 0x999a0 | 260 | core/Joint.cpp | 004145 | row004131 |
| 004137 | 0x99e00 | 41 | core/Joint.cpp | Np getGlobalAnchorVal | Joint::getGlobalAnchorVal |
| 004139 | 0x99e30 | 41 | core/Joint.cpp | Np getGlobalAxisVal | Joint::getGlobalAxisVal |
| 004141 | 0x99e60 | 464 | core/Joint.cpp | 004366 (transcript) | Joint::Joint |
| 004145 | 0x9a430 | 174 | core/Joint.cpp | Np getNextLimitPlane | Joint::getNextLimitPlane |
| 004389 | 0xaf2d0 | 227 | core/JointSupport.cpp | 004362 | `JointSupportRecord::row004389() const` (`this` in ecx, no stack args, `ret`: a no-argument thiscall member; returns the unrounded st(0) value, declared `NxF64`) |
| 004393 | 0xaf710 | 122 | core/JointSupport.cpp | 004362 | row004393 |

Several of these are `reconstructed` as parameterised models (004070, 004076, 004078,
004081, 004083, 004087, 004089); Task 6 writes them natively and adds the model pointer
comment. 004066, 004070, 004389, 004393 are not in the Joint.cpp bundle: read them from
`oracle/ghidra/manifest.json` (all have an `ok` decompile) and the Capstone listing.

### defer

| Row | RVA | B | Called by (pilot) | Unreachable path |
|---|---|---:|---|---|
| 004064 | 0x957a0 | 385 | 004356 | internal slot 8 (004356), called only from scene code outside the pilot |
| 004093 | 0x95da0 | 116 | 004360, 004362 | internal slots 6/7 (solver); also needs Scene row 000598, absent from the candidate. Returns the next 0x50-byte `JointSupportRecord` of the array at Scene+0x5b8 and counts it in Joint +0x160/+0x164 |
| 004099 | 0x962f0 | 1112 | 004681 | `NxJoint::setGlobalAnchor` — the test never calls it |
| 004101 | 0x96750 | 5302 | 004683 | `NxJoint::setGlobalAxis` — not called |
| 004109 | 0x97e60 | 366 | 004687 | `NxJoint::setLimitPoint` — not called |
| 004111 | 0x97fd0 | 113 | internal slot 2 | break test from the solver; needs 000571, 004091 and the break event (004113 → 000653/004105) |
| 004123 | 0x98be0 | 518 | 004364 | internal slot 4 (004364, debug visualization; Task 8b correction: not the solver) |
| 004133 | 0x99ab0 | 134 | Joint base slot 6 | overridden by 004360 in RevoluteJoint; declare as Joint's slot-6 default |
| 004135 | 0x99b40 | 701 | 004362; Joint base slot 7 | internal slot 7 (solver) |
| 004143 | 0x9a0d0 | 860 | 004689 | `NxJoint::addLimitPlane` — not called |
| 004391 | 0xaf3c0 | 837 | 004360, 004362, 004393 | solver slots 6/7 |
| 000022 | 0x1840 | 27 | 004356 | slot 8 (as 004064); owner gap `<start>..Actor.cpp`; thiscall on the body's +0x19c owner with 1, `ret 4` (`Row000022Fixture`) |
| 000758 | 0x17630 | 214 | 004356 | slot 8; owner gap SceneRaycast..CapsuleShape; `this` = the body record, rebuilds its +0x134 3x3 from the +0x124 quaternion (`Row000758Fixture`) |
| 000571 | 0x108e0 | 22 | 004374, 004111 | break-event post (slot 0 / slot 2 break): links the event through its +4 into the list at Scene+0x620; Scene-owned, no candidate symbol |
| 000633 | 0x12660 | 370 | 004107 (third arg false), 004095 | 004370 re-binding actors / joint release; Scene-owned, no candidate symbol |

Where the stubs go: deferred Joint rows in `core/Joint.cpp`; 004391 and the deferred rows
owned by other units (000022, 000571, 000633, 000758) in `core/JointSupport.cpp`.

### reuse

| Row | Called by | Existing symbol |
|---|---|---|
| 000480 | 004693, 004121, 004095 | `nxSetSdkPointerBinding(void* key, void* value)` — `Physics/src/include/PhysicsInternal.h:186`, `Physics/src/PhysicsInternal.cpp:205` |
| 000454 | Np getName (004743 body) | `nxGetSdkPointerBinding(void* key)` — `PhysicsInternal.h:183`, `PhysicsInternal.cpp:184` |
| 002362 | every R accessor | `nxNpSceneGuardEnter(void* link)` — `Physics/src/include/NpSceneGuard.h:17` (same operation as `ReadWriteLock::lock`, `PhysicsInternal.cpp:54`) |
| 002364 | every W accessor | `nxNpSceneGuardWriteTry(void* link)` — `NpSceneGuard.h:26` |
| 002366 | every accessor | `nxNpSceneGuardLeave(void* link)` — `NpSceneGuard.h:40` |
| 002404 / 002406 | 004725 / 004729 | compiler-generated ctor/dtor of the 12-byte hook base (shape `EmbeddedHookBase`, `Physics/src/include/ObjectModel.h:33`; models `nxActorMemberInit`/`nxActorMemberReset`, `ObjectModel.h:784–785`). Rows owned by gap Controller..Fluid; not claimed |
| 000661 | 000665 (Task 10), 004107 (third arg false) | `nxSceneAddJoint(void*, void*)` — `Physics/src/Scene.cpp:2370`, currently a no-op hole standing for the row |
| 004186 | internal slot 14 | inline `getProjectionMode() { return projectionMode; }` in `Joint.h`; row owned by D6Joint.cpp, not claimed |
| 001391 | internal slots 15/16 | inline `return this` bodies in `RevoluteJoint.h`; not claimed |
| 004248, 001583 | Joint base slots 0/1/8 | inline empty bodies in `Joint.h`; not claimed |
| 004417–004433, 004537, 005667 | tables 002727/002725/002700 | generated from `NxJoint.h`; nothing to write |
| 005697 (`_CIacos`, 0xf47f0) | 004330, 004352, 004372 | Task 8a: not `NxMath::acos(NxF32)` (the CRT acos need not match the oracle's x87 sequence). `core/RevoluteJoint.cpp` has one file-static inline-asm helper, `revoluteCIacos`, reproducing `_CIacos`'s core `fld1; fadd st,st(1); fld1; fsub st,st(2); fmulp st(1),st; fsqrt; fxch st(1); fpatan` (0xf4828–0xf4836) and its control-word handling (non-default word → `(cw & 0x300) \| 0x7f` for the core, 0xfa9b5; restored on the 0xfaa4b exit at 0xfaa70 or via 0xfa957 at 0xfa98e — 0xfaa3e is dead, the flag at 0x10128514 is never set; the 0xfa957 qword round-trip and the NaN arm 0xf4881 → 0xfa9cc are not reproduced and change no value, see the helper's comment), wrapped by `revoluteAcos`, the ≥ 1 → 0 / ≤ -1 → π clamp all three call sites carry inline (004330 0xa8dfe–0xa8e34, 004352 0xa94fd–0xa9530, 004372 0xad015–0xad04b, both starting at their `fld`/`fcomp` ≥ 1 test; π is the float at 0x1011a1b0, `phys_data_002683`). 004372 (Task 8b) calls `revoluteAcos` too |
| `NxFindRotationMatrix` (Foundation export, import slot `[0x10104174]`) | 004356 (0xa9a90) | `NxFindRotationMatrix(const NxVec3&, const NxVec3&, NxMat33&)` — `Foundation/include/NxUtilities.h:102`, `Foundation/src/Utilities.cpp:243`; cdecl (`add esp, 0xc` at 0xa9a9d) |
| `NxDebugRenderable::addLine` / `addArrow` (+0x20 / +0x30) | 004364 | the renderable's virtuals, `Foundation/include/NxDebugRenderable.h` |
| SDK allocator `[[0x101041bc]]` +8 / +0x14 | 004366, 004368, 004729, 000665 | `nxGetSdkAllocator()->malloc(size, NX_MEMORY_PERSISTENT)` / `->free(p)` — `PhysicsInternal.h:158` |
| `FoundationSDK::error` import `[0x101041b4]` | all asserting rows | `NxFoundation::FoundationSDK::getInstance().error(code, file, line, 0, msg)` — `Physics/src/PhysicsSDK.cpp:206` |
| folded Np bodies 004437 004441 004443 004479 004483 004491 004497 004499 004539 004573 004577 004635 004743 | table 002727 | implemented as NpRevoluteJoint methods in `core/NpRevoluteJoint.cpp` by Task 9; **not claimed** (see `## Task split`) |

## Existing candidate code

What the new code replaces or must stay compatible with. Line numbers are at commit 3cab671.

| Symbol | File:line | Relationship |
|---|---|---|
| `NxSceneInternal::createJoint` | `Physics/src/Scene.cpp:1413` (comment 1392–1411) | Task 10 routes `d[1] == 1` (revolute) to `new (malloc(0x204)) RevoluteJoint(desc)`, reads `+0x48`, copies the lock links, calls `nxSceneAddJoint`, returns per the chain. Its `case 0: size = 0x17c; // revolute` (line 1462) is the prismatic literal; its null-marker branch (1492–1504) disagrees with the listing. Other types keep the generic path |
| `nxJointConstruct` | `Scene.cpp:2317` (decl 291) | generic hole; stays for non-revolute types |
| `nxJointSizeForType` | `Scene.cpp:2350` (decl 294) | generic hole; its sizes are wrong (true sizes in `## Construction chain` step 3); leave for non-revolute types or correct them — Task 10's call |
| `nxSceneAddJoint` | `Scene.cpp:2370` (decl 296) | no-op hole for 000661; reused |
| `nxJointDestroy` | `Scene.cpp:2377` (decl 299) | no-op hole; the revolute failure path calls the real deleting destructor (internal slot 5) instead |
| `nxActorBuildBody` | `Scene.cpp:1777` | builds the body/record the Joint rows read; field agreement in `## Object layouts` |
| `NpScene::createJoint` | `Physics/src/NpScene.cpp:579` | must return `[internal+0x48]` for revolute (the oracle's 000297); for other types, what it returns today |
| `NpScene::releaseJoint` | `NpScene.cpp:204` | empty; decides whether release rows run (Task 10) |
| `NpJointObject` | `Physics/src/include/NpJoint.h:25` (`REVOLUTE_SIZE = 0x17c` line 27) | generic stand-in; not the revolute object (true: 0x1c public / 0x204 internal). Keep for other types |
| `NpJointVtable` | `NpJoint.h:54`; bodies `Physics/src/NpJoint.cpp:33–192` (`isRevoluteJoint` returns 0 at 149) | generic stand-in; NpRevoluteJoint replaces it for revolute joints only |
| `NpJointObject::installVtable` | `NpJoint.cpp:15` | not used by the revolute path |
| model `nxLockedCopyAndFlag` | `Physics/src/include/ObjectModel.h:1611`, `ObjectModel.cpp:1575` | covers 004342/004346/004350 (and 004711/004715/004719 shape); gains a product pointer comment |
| model `nxGuardedStoreEx` | `ObjectModel.h:1484`, `ObjectModel.cpp:2067` | covers 004338 (and the 004334 guard); pointer comment |
| model `nxLockedVtCallNoArg` | `ObjectModel.h:1028`, `ObjectModel.cpp:1998` | covers 004703; pointer comment |
| model `nxListFreeViaSingleton4089` | `ObjectModel.h:1249`, `ObjectModel.cpp:2713` | covers 004089; pointer comment |
| `ReadWriteLock` | `Physics/src/include/PhysicsInternal.h:49` | same rows as the guard functions, on a different handle shape; do not mix |
| test | `tests/PhysicsJointTests.cpp:116–156` | the four revolute cases; see `## Construction chain` |
| validator allowlist | `docs/reconstruction/novodex-physics/tools/validate_inventory.py:1964, 1970` | `'Physics/src/core/NpRevoluteJoint.cpp'` and `'Physics/src/core/RevoluteJoint.cpp'` are on `UNRESOLVED_SOURCE_PATHS`. Once Task 5 creates the files the validator fails with "is on the allowlist but no longer unresolved; remove the entry" (line 2033). **Task 5 must remove those two entries.** `'Physics/src/Joint.cpp'` (line 1931, the oracle's real path for Joint.cpp) is unaffected by `core/Joint.cpp` |

### Open issues for later tasks

1. ~~**Lock links.**~~ Resolved by Task 10 (see "Wiring made by Task 10"). Every NpRevoluteJoint accessor locks `np+0x10` or `np+0x14`. Scene+0x6cc
   already holds the NpScene (`Scene.cpp:494`), so Task 10 copies holder[3]/holder[4] into
   `np+0x10`/`np+0x14` in Scene::createJoint (oracle 0x14509–0x14521), reading the public
   object from byte +0x48 (not the current `[0x12 / 4]` read at `Scene.cpp:1481`).
2. **Release is unwired.** `NpScene::releaseJoint` is empty, so the four joints are never
   destroyed (they leak into scene teardown). The transcript prints `released=yes`
   regardless. If Task 10 wires release it also needs 000653/000633 (deferred Scene rows).
3. **Body agreement.** 004097/004125/004129 read body +0x50, +0x5c, +0xdc..+0x108, +0x198;
   the candidate writes those (`Scene.cpp:1811–1919`), but +0x5c's quaternion convention
   and +0xdc's row-major order are candidate choices not checked against these rows.
   A wrong convention changes `out_anchor`/`out_axis` for cases 1–3.
   Task 10: the four cases' `out_anchor`/`out_axis` match the oracle byte-for-byte. The
   test's two actors have identity orientation (only translated), so this confirms the
   +0x50 position and the identity case of the +0x5c/+0xdc conventions, not a rotated body.
4. ~~The typeBit→type byte table (0x9a048) is inferred, not read.~~ Resolved by Task 6: read from the image, mapping confirmed (see `## Object layouts`).
5. Fields marked **unknown** (Joint +0x04 readers, +0x34/+0x38,
   +0x160/+0x164; revolute +0x1ac..+0x200) must be declared by offset only.
6. `phys_fn_004727` has no decompile anywhere (Capstone listing only; 8 bytes).
7. **Solver-slot inputs the candidate does not build (Task 8a).** Driving 004374 (slot 0)
   requires body +0x204 (a `JointSupportBody*`): it calls 004358, which dereferences it, and
   writes its impulse through it; the candidate's body record never writes +0x204. 004360
   and 004362 (slots 6/7) copy the same pointer into their records and take records from
   004093, which needs the Scene's record array at +0x5b8 and row 000598. (The other body
   fields 004360 reads, +0xc0 and +0x164, the candidate does write.) None of the three is on
   the transcript path.
8. **SDK parameters read by the solver-slot rows.** 004360 and 004362 read the live parameter
   array (`gParameter`, `.data 0x10123b18`) directly: element 0 (`NX_PENALTY_FORCE`) scales
   +0x1ac and the kind-0/2 record outputs, element 4 (`NX_BOUNCE_TRESHOLD`) gates the limit
   restitution. The product reads them through `PhysicsSDK::getParameter` (0 with no SDK, the
   array's static value), as `ContactGeneration.cpp` does.
9. **Rows 8b leaves unrunnable (Task 8b).** 004356 ends in deferred 000022 (always, when it
   changes the body) and, on the turn arm, 000758; 004364 calls deferred 004123 in its world-axes
   and limit arms. Their stubs assert, so neither row can complete on the arms that reach the
   deferred rows until those rows are written. 004356 also needs body +0x124/+0x19c and 004364 a renderable; neither is on the
   transcript path. 004364 reads SDK parameters 13, 31, 32 and 33 through
   `PhysicsSDK::getParameter`, as 8a's rows do (see 8).

## Task split

Sizes from `inventory.json`. Every row below is written with the exact stable-ID comment
line from the Global Constraints. The folded Np bodies are the exception: they are
written but not claimed, and their comment must **not** use the stable-ID form; use
`// Shared NpJoint body; the oracle keeps one folded copy at 0x000b0670 (core\NpD6Joint.cpp).`

### Task 6 — `core/Joint.cpp` and `core/JointSupport.cpp` (`write` rows): 24 rows, 7,785 B

- core/Joint.cpp (22 rows, 7,436 B): 004066, 004070, 004074, 004076, 004078, 004080,
  004081, 004083, 004087, 004089, 004095, 004097, 004107, 004121, 004125, 004127, 004129,
  004131, 004137, 004139, 004141, 004145.
- core/JointSupport.cpp (2 rows, 349 B): 004389, 004393.
- Plus declared `defer` stubs (no bodies): 004064, 004093, 004099, 004101, 004109, 004111,
  004123, 004133, 004135, 004143 in core/Joint.cpp; 004391, 000022, 000758, 000571, 000633
  in core/JointSupport.cpp.
- Large rows: 004125 (1,940), 004097 (1,176), 004121 (1,084), 004129 (787). Write these
  first: they are the transcript path.

### Task 7 — `core/RevoluteJoint.cpp` rows under 700 B: 18 rows, 2,943 B

004328, 004330, 004332, 004334, 004336, 004338, 004340, 004342, 004344, 004346, 004348,
004350, 004352 (694 B), 004354, 004358, 004366, 004368, 004370.

### Task 8 — `core/RevoluteJoint.cpp` rows of 700 B or more: 6 rows, 15,268 B

004356 (2,303), 004360 (4,460), 004362 (1,644), 004364 (3,326), 004372 (2,467),
004374 (1,068).

**15,268 B is over the ~12 KB one implementer can handle. Proposed split:**

- **Task 8a — 3 rows, 7,172 B:** 004360 (slot 6), 004362 (slot 7), 004374 (slot 0). They
  share the +0x1ac..+0x200 fields and 004358/004391/004389/004393.
- **Task 8b — 3 rows, 8,096 B:** 004356 (slot 8), 004364 (slot 4), 004372 (getAngle).
  004364 and 004372 share 004097/004352; 004356 is the only projection-field reader.

004356, 004360 and 004364 have only the supplement decompile (`ghidra supplement`);
004362, 004372, 004374 have manifest decompiles. None of the six is on the transcript
path, so their correctness is checked only by review and the build.

### Task 9 — `core/NpRevoluteJoint.cpp`: 25 claimed rows, 1,602 B, plus 13 unclaimed bodies (601 B)

Claimed: 004681, 004683, 004685, 004687, 004689, 004691, 004693, 004695, 004697, 004699,
004701, 004703, 004705, 004707, 004709, 004711, 004713, 004715, 004717, 004719, 004721,
004723, 004725, 004727 (the thunk: emitted by the compiler for the second base's
destructor; write the comment line above the destructor it serves and note that the
thunk is generated), 004729.

Unclaimed NpRevoluteJoint methods (bodies from the folded rows, in table order):
getActors (004539), getGlobalAnchor (004437), getGlobalAxis (004441), getGlobalAnchorVal
(004497), getGlobalAxisVal (004499), getState (004483), getBreakable (004573),
getLimitPoint (004577), hasMoreLimitPlanes (004491), getNextLimitPlane (004635), getType
(004443), is (004479), getName (004743). getActors, getGlobalAnchor, getGlobalAxis and
getState are on the transcript path.

Order of work across tasks: 5 → 6 → (7, 8a, 8b, 9 in any order) → 10. Task 9 depends on
Task 6's Joint accessors and Task 7's 004366/004342/004346/004350/004354; Task 8b's
004372 is called by 004721.
