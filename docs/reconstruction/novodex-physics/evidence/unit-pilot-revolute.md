# Translation-unit pilot: revolute joint

Design: `docs/superpowers/specs/2026-09-24-translation-unit-pilot-design.md`.
Plan: `docs/superpowers/plans/2026-09-25-translation-unit-pilot.md`.

## Timing

| Task | Start | End | Rows written | Bytes written | Notes |
|---|---|---|---:|---:|---|
| 1 | 2026-09-25T00:20:00 | 2026-09-25T00:25:39 | 57 | 226166 | core\RevoluteJoint.cpp ['0x000a8d40', '0x000ac630'] 22 phys_fn_004328 phys_fn_004370 {'discovered': 15, 'reconstructed': 7} {'discovered': 14296, 'reconstructed': 380}; core\NpRevoluteJoint.cpp ['0x000b2d10', '0x000b32b0'] 19 phys_fn_004681 phys_fn_004717 {'discovered': 9, 'reconstructed': 10} {'discovered': 707, 'reconstructed': 646}; Joint.cpp ['0x00095ab0', '0x0009a430'] 37 phys_fn_004074 phys_fn_004145 {'discovered': 27, 'dynamically_gated': 2, 'reconstructed': 8} {'discovered': 16608, 'dynamically_gated': 1689, 'reconstructed': 271} |
| 2 | 2026-09-25T00:26:00 | 2026-09-25T00:32:17 | 11 | 57498 | Requested RVAs (no ok decompile in manifest): 0x97fd0 0xa8d20 0xa8f10 0xa8fb0 0xa8fc0 0xa9650 0xaa060 0xab840 0xb3270 0xb3340 0xb3370; ghidra_version=12.1.2, analysis_options_sha256 matches pin; status counts {'ok': 11} (no create_failed/decompile_failed rows); large rows 0xa9650/0xaa060/0xab840 decompiled with 11204/18928/18497 C chars respectively; second run byte-identical to first (deterministic); project opened -readOnly -noanalysis and headless log reports "Discarding changes to the following read-only file: /NxPhysics.dll" |
| 3 | 2026-09-25T00:33:00 | 2026-09-25T00:36:15 | 78 | 237855 | Created unit_bundle.py and unit bundle tests; generated reference bundles for 3 units (core\RevoluteJoint.cpp, core\NpRevoluteJoint.cpp, Joint.cpp) with 78 total rows; phys_fn_004360 decompile labeled ghidra supplement as expected; phys_data_002684 dispatch entries verified |
| 4 | 2026-09-25T00:42:00 | 2026-09-25T01:11:30 | 88 | 62762 | Contract only, no product code: units/revolute-contract.md assigns 88 rows (24 RevoluteJoint.cpp incl. 004372/004374; 25 NpRevoluteJoint.cpp incl. 004719-004729; 32 Joint.cpp; 3 JointSupport.cpp rows 004389/004391/004393; 4 deferred other-unit stubs); 004675/004677/004679 belong to NpCylindricalJoint.cpp. Revolute is Scene::createJoint case 1 (alloc 0x204, ctor 004366); 0xad6e0/0x17c in Scene.cpp is case 0 (prismatic). Public object is the 0x1c-byte NpRevoluteJoint at [internal+0x48], table 0x1011b328 (002727). Split: Task 6 24 rows/7785 B, Task 7 18/2943 B, Task 8 6/15268 B (over 12 KB; proposed 8a 7172 B + 8b 8096 B), Task 9 25/1602 B + 13 unclaimed folded bodies. Task 10 must copy Scene+0x6cc holder[3]/[4] into np+0x10/0x14 and read the public object at byte +0x48 (Scene.cpp:1481 reads +0x10); Task 5 must drop two validator allowlist entries |
| 5 | 2026-09-25T01:15:00 | 2026-09-25T01:55:00 | 88 | 53475 | Created Physics/src/include/core/{Joint,RevoluteJoint,NpRevoluteJoint}.h and Physics/src/core/{Joint,RevoluteJoint,NpRevoluteJoint,JointSupport}.cpp (JointSupport.cpp added: contract assigns 004389/004391/004393 plus 4 deferred other-unit stubs to it); 88 stub rows (32 Joint.cpp, 24 RevoluteJoint.cpp, 25 NpRevoluteJoint.cpp claimed + 13 unclaimed folded overrides stubbed for vtable linkage, 7 JointSupport.cpp). NpRevoluteJoint : public NxRevoluteJoint, public EmbeddedHookBase (ObjectModel.h) gives the compiler-generated 0x1011b328/0x1011b3dc vtables and the phys_fn_004727 adjustor thunk for free, matching the contract without inventing a hierarchy. Joint/RevoluteJoint modelled as real (non-byte-array) structs with static_assert(offsetof/sizeof) per established field; unknown fields declared mUnknownNNN. CMakeLists.txt:67 globs Physics/src/core/*.cpp; PHYSICS_HEADERS GLOB_RECURSE already covered the new core/ header directory, no change needed. Removed the two now-resolved UNRESOLVED_SOURCE_PATHS entries (NpRevoluteJoint.cpp, RevoluteJoint.cpp) from validate_inventory.py, following the file's existing 'was here ... REMOVED' comment precedent. Build: `cmake --build build --config Release --target NxPhysics` succeeds, no warnings from Physics/src/core/ (pre-existing warnings elsewhere: ARRAYSIZE redefinition in NpActor.cpp/ObjectModel.cpp, NxAllocateable operator-new C4291 in PhysicsInternal.cpp/PhysicsSDK.cpp/Scene.cpp). Phase 6 gate: status=pass, transcript unaffected (nothing constructs the new classes yet). validate_inventory.py: inventory=pass, unexplained=0, exit 0. Tool unit tests: 643 tests OK |
| 6 | 2026-09-25T01:50:58 | 2026-09-25T02:12:00 | 24 | 7785 | Wrote core/Joint.cpp 004066 004070 004074 004076 004078 004080 004081 004083 004087 004089 004095 004097 004107 004121 004125 004127 004129 004131 004137 004139 004141 004145 and core/JointSupport.cpp 004389 004393 from the Capstone listing (x87: stack lifetimes as double, fstp dword spills as NxReal, listing grouping kept). Left defer (stubs, `// (deferred: ...)`): 004064 004093 004099 004101 004109 004111 004123 004133 004135 004143; 004391 000022 000571 000633 000758. Decompile/listing disagreements (listing followed): 004097 groups the third sum of each rotated vector as ((b8*z + b5*y) + b2*x) where the decompile shows ((b2*x + b5*y) + b8*z), uses the unrounded z anchor difference for x but the stored float for y/z, and in negative-trace cases 0/1 multiplies by a float-stored reciprocal and in case 2 divides by the float-stored root; 004121 case 0 subtracts the float-stored (m4+m8); 004125's two body blocks use different sum groupings and spills (both kept); 004131/004389 return the unrounded st(0) (declared NxF64). Contract corrections: solver body order (+0x24) is (body0, body1) when +0x2c bit 1 is SET, swapped when clear; typeBit byte table 0x9a048 read from the image (open issue 4 resolved); +0x14 is the limit point. Declarations changed (Joint.h, new JointSupport.h, contract section 'Declaration changes made by Task 6'). ObjectModel.cpp nxListFreeViaSingleton4089 gained the product pointer comment. Build clean (no Physics/src/core warnings); Phase 6 gate status=pass; validate_inventory exit 0; scratch harness sanity check (identity/90-degree bodies) gave the expected anchor/axis. Flag note: rows reached inside the simulation step (004097 via solver slots) would need /arch:IA32 on Joint.cpp to follow the step's control word; not changed |
| 7 | 2026-09-25T02:22:37 | 2026-09-25T02:34:00 | 18 | 2943 | Wrote core/RevoluteJoint.cpp 004328 004330 004332 004334 004336 004338 004340 004342 004344 004346 004348 004350 004352 004354 004358 004366 004368 004370 from the Capstone listing (x87 discipline of core/Joint.cpp). None left defer; Task 8's six large rows stay stubs. Decompile/listing disagreements (listing followed): 004352 decompile rounds body 0's rotated normal and the sign sum to float, listing keeps them on the FPU stack, stores only the cosine, and returns the acos result unrounded; 004358 decompile shows every intermediate as float, listing keeps the first cross component(s) and body 1's last two sums on the stack; 004334 listing stores the argument at +0x1a8 (3z158's zero was a drive artifact, superseded by 3z172). Declaration changes (contract "Declaration changes made by Task 7"): mLimit/mMotor/mSpring typed as the public descriptor classes; row004352 -> NxF64; getVelocity -> NxF64; row004358(NxVec3&) const; Joint::operator delete -> SDK free; JointBodyRecord +0x78 mAngularVelocity, +0x204 mUnknown204 (new JointBodyRecord204). 004332 uses inline-asm fcos/fsin. Product-row pointers added to nxGuardedStoreEx and nxLockedCopyAndFlag. Build clean; Phase 6 gate pass; tool tests 643 OK. |
| 8a | 2026-09-25T02:41:58 | 2026-09-25T03:02:30 | 3 | 7172 | Wrote core/RevoluteJoint.cpp 004360 (slot 6), 004362 (slot 7), 004374 (slot 0) from the Capstone listing (x87 discipline of core/Joint.cpp). None left open; 004356/004364/004372 stay stubs for Task 8b. Decompile/listing disagreements (listing followed): 004374 decompile shows no stack argument (listing `ret 4`, argument never read) and all intermediates as float (listing keeps a.z's sum, the ratio, t.z and partial sums on the stack; body 1's first cross term uses the register -t.z, its impulse the stored float); 004360 supplement drops the record kind tests as unreachable (0x100ab07d, 0x100ab142) and shows floats where the listing keeps K's k00-k02/k12/k22, the x anchor difference, a0 x a1's z and the cross y terms on the stack; 004362 decompile rounds the angle differences and spring terms to float. acos: _CIacos (0xf47f0) reproduced as file-static inline asm revoluteCIacos (core fld1/fadd/fld1/fsub/fmulp/fsqrt/fxch/fpatan 0xf4828-0xf4836 plus its control-word handling); 004330 and 004352 now use it via revoluteAcos. Declaration changes (contract "Declaration changes made by Task 8a"): JointBodyRecord204 folded into JointSupportBody (+0x0c float, +0x20 3x3); JointBodyRecord +0xc0, +0x164 3x3; JointSupportRecord 0x50 bytes with +0x30..+0x4c; row004093 returns JointSupportRecord*; row_slot6/7 take NxReal; JointBreakEvent; row000571 takes the event. SDK parameters 0/4 read through PhysicsSDK::getParameter. Build clean; Phase 6 gate pass; tool tests 643 OK. |
| 8b | 2026-09-25T03:16:04 | 2026-09-25T03:35:00 | 3 | 8096 | Wrote core/RevoluteJoint.cpp 004356 (slot 8, projection), 004364 (slot 4, debug visualization), 004372 (getAngle) from the Capstone listing (x87 discipline of 7/8a; new file-static helpers revoluteSum3, revoluteFcosFsin (inline-asm fcos/fsin of one unrounded angle), revoluteQuatToRows (= Joint.cpp's jointQuatToRows) and revoluteQuatToRowsSpilled; getAngle uses revoluteAcos). None left open; rows still reaching deferred stubs: 004356 -> 000022/000758, 004364 -> 004123. Decompile/listing disagreements (listing followed): 004356 supplement shows rotated axes, target and matrix product as float with reordered terms and compares the rounded dot, listing compares the unrounded dot (0xa98f9 fst/fcomp) and uses the float after; its quaternion switch's default arm (0xa9eb7, loads an unset local) is unreachable and not reproduced; 004364 supplement loses every renderable call's arguments and colours and misreads the end-spoke colour (listing flag[i/12]*0xd000+0xff0000), and shows rotated vectors as float with reordered terms; 004372 decompile reorders every product sum and shows floats, listing keeps body 0's first normal component on the stack, converts the quaternion three times with two spill patterns, and returns st(0) unrounded (004721 rounds it). Declaration changes (contract "Declaration changes made by Task 8b"): row_slot4(NxDebugRenderable&), row_slot8(void* body), getAngle -> NxF64, row004064(const NxVec3&, const NxVec3&, NxVec3&) const, row004123(NxVec3&), JointBodyRecord +0x124 mCMassOrientation[4], Row000022Fixture/Row000758Fixture; contract row 39 corrected (004372 returns unrounded, 004721 fstp-rounds). Build clean (no Physics/src/core warnings); Phase 6 gate pass; validate_inventory exit 0; public headers pass; tool tests 643 OK. |
| 9 | 2026-09-25T03:40:00 | 2026-09-25T04:05:00 | 25 | 1602 | Wrote core/NpRevoluteJoint.cpp: the 25 claimed rows (004681 004683 004685 004687 004689 004691 004693 004695 004697 004699 004701 004703 004705 004707 004709 004711 004713 004715 004717 004719 004721 004723 004725 004727-comment-only 004729) plus the 13 unclaimed folded NpJoint bodies (getActors 004539, getGlobalAnchor 004437, getGlobalAxis 004441, getGlobalAnchorVal 004497, getGlobalAxisVal 004499, getState 004483, getBreakable 004573, getLimitPoint 004577, hasMoreLimitPlanes 004491, getNextLimitPlane 004635, getType 004443, is 004479, getName 004743), all from the bundle's ghidra-manifest decompiles plus a small Python extraction (oracle/ghidra/manifest.json, oracle/capstone/manifest.json) for the six rows outside the bundle (004719-004729) and the 13 folded bodies. None left open/deferred: every row forwards through the named RevoluteJoint/Joint member or virtual on mInternal under nxNpSceneGuardEnter/nxNpSceneGuardWriteTry(&mWord08/&mWord04), matching the bundle's lock-bracketed-forwarder shape exactly, including the four write rows whose report line differs from the shared 0xe (004697 0x12, 004699 0x1d, 004701 0x25, 004705 0x32) and the two more with rows further down the file (004709 0x3f, 004717 0x57, read directly from their bundle decompiles rather than assumed from the dispatch-table summary). setMotor (004713) has no lock (tail-jump per the listing). Constructor (004725)/destructor (004729) need no manual vtable code: NxJoint()'s inline ctor already zeroes userData/appData and C++ installs every base/derived vtable automatically for this multiple-inheritance shape; only the hook base's mWord04/mWord08 needed an explicit zero (EmbeddedHookBase has no ctor). Declaration change (contract "Declaration changes made by Task 9"): NpRevoluteJoint gains `static void operator delete(void* p)` routing the compiler-generated deleting destructor's free to the SDK allocator, matching phys_fn_004729's tail. ObjectModel.cpp's nxLockedVtCallNoArg and nxLockedCopyAndFlag gained a second pointer comment noting they also cover 004703/004707 and 004711/004715/004719's shape (written as direct named calls instead, since NpRevoluteJoint is real C++, not the generic byte-offset NpJointObject). Build clean (no Physics/src/core warnings, pre-existing ARRAYSIZE redefinition warning from windows.h vs IceUtils.h unchanged); Phase 6 gate status=pass (transcript already matches -- Task 10 has not wired construction yet, so this proves the build is unaffected); validate_inventory exit 0, unexplained=0; tool tests 643 OK. |
| 10 | 2026-09-25T04:18:30 | 2026-09-25T04:27:00 | 0 | 3394 | Wired Scene::createJoint (Physics/src/Scene.cpp) for NX_JOINT_REVOLUTE (descriptor type 1 = oracle case 1; the brief's "case 0" is PRISMATIC, 0x17c/004380): SDK-allocator malloc(sizeof(RevoluteJoint)=0x204) + placement new RevoluteJoint(desc) (004366, builds NpRevoluteJoint via 004725); reads the public object at byte +0x48 (the candidate's generic path read [0x12/4] = byte +0x10); null +0x48 -> delete internal (004368) and return 0 per 0x14581-0x1458c (the candidate's comment claimed a fall-through return); copies NpScene holder[3]/[4] (Scene+0x6cc) into np+0x10/+0x14 per 0x14509-0x14521; nxSceneAddJoint(this, internal) (000661 hole, still no-op); returns the NpRevoluteJoint as NxJoint* (000297's [internal+0x48] load done inside Scene::createJoint so NpScene::createJoint is unchanged). np stores use byte offsets because core/NpRevoluteJoint.h -> ObjectModel.h's nxActorConstruct(void*,void*) returning void collides with Scene.cpp's void*-returning one (C2556). Other types unchanged (generic nxJointConstruct); "case 0 // revolute" relabelled prismatic, nxJointSizeForType comment corrected, sizes untouched. Release NOT wired (NpScene::releaseJoint empty). Defects found: none in the reconstructed rows -- Phase 6 staged-pair transcript byte-identical on the first run (stdout_delta=0, 11/11 coverage), including cases 1-3 anchor/axis (open issue 3: test bodies are identity-orientation, so only the identity case of the +0x5c/+0xdc conventions is confirmed). Evidence the rows run: cdb breakpoints (build/Release/NxPhysics.map addresses) on the candidate NxPhysicsJointTests run hit per case 000665(candidate), 004366, 004141, 004121, 004097 x2, 004725, 004332, 004437, 004125, 004441, 004129, 004483, 004078, 004539; 004107's out-of-line copy not hit (inlined or unobserved); no destructor hit. Gates: 4, 6, 7 pass; 5 fails only in NxPhysicsObjectLayoutTests (CANDIDATE-MISSING family=vtables; target links neither Scene.cpp nor the candidate DLL); 2 and 3 fail at build_physics compiling NpActor.cpp in the standalone NxPhysicsInternalTests/NxPhysicsCollisionTests targets (ObjectModel.h -> IcePrunable.h -> Opcode.h not on their include paths) -- PRE-EXISTING: reproduced with HEAD's Scene.cpp. Once those targets build they will also need Physics/src/core/*.cpp (Scene.cpp now references RevoluteJoint). validate_inventory exit 0; public headers pass (gate). |
| 11 | 2026-09-25T04:33:00 (approx.; after dc11888 at 04:32:53) | 2026-09-25T04:45:32 | 73 | 0 | Inventory, ledger, verification and this measurement; no product code. 73 pilot rows given `implementation`/`source` = their `Physics/src/core/*.cpp` file and an appended static proof; 48 of them (26,321 B) moved `discovered` -> `reconstructed`, the other 25 were already `reconstructed` (proofs appended, earlier source text and ObjectModel model pointers kept in `notes`). 9 rows (5,851 B) given a dynamic proof from the Task 10 cdb trace. Phase 6 ledger: 48 entries `not_reconstructed_in_phase` -> `reconstructed_not_falsified` (counts 302/129 -> 254/177); the validator asked for nothing further. work_units.json and the three bundles regenerated. Fresh configure + clean build, headers, 643 tool tests, gates 2-7 (see ## Verification). |

## Result

The wiring made by Task 10 was kept: `Scene::createJoint` builds `NX_JOINT_REVOLUTE` through
`core/RevoluteJoint.cpp` (004366), and the Phase 6 staged-pair transcript is byte-identical to the
oracle's. Every row written in Tasks 6-9 is in a wired file, so all of them were updated.

Rows given `implementation` = `source` = their product file (the file carries the row's exact
stable-ID line), per file. "New" is the subset that moved `discovered` -> `reconstructed`; the rest
were already `reconstructed` (mostly parameterised ObjectModel models) and only gained the native
source and an appended proof.

| File | Rows | Bytes | New rows | New bytes |
|---|---:|---:|---:|---:|
| `Physics/src/core/Joint.cpp` | 22 | 7,436 | 15 | 7,230 |
| `Physics/src/core/JointSupport.cpp` | 2 | 349 | 2 | 349 |
| `Physics/src/core/RevoluteJoint.cpp` | 24 | 18,211 | 17 | 17,831 |
| `Physics/src/core/NpRevoluteJoint.cpp` | 25 | 1,602 | 14 | 911 |
| **Total** | **73** | **27,598** | **48** | **26,321** |

Proofs:

- **Static (all 73):** `transcribed from units/<bundle>.md (<decompile label>) and checked against
  the Capstone listing; layout per units/revolute-contract.md`. Rows outside the bundles (004066,
  004070, 004389, 004393, 004719, 004725, 004729) cite `oracle/ghidra/manifest.json`; 004721 and
  004723 cite `oracle/ghidra/supplement.json`. Existing proof text was kept and the new text appended.
- **004727** (8 B, the `sub ecx,0xc; jmp` adjustor thunk) has no body in source, only its stable-ID
  line. It was given `reconstructed` on a checked static proof: the class layout makes the compiler
  emit `??_ENpRevoluteJoint@@WM@AEPAXI@Z`, which is at candidate 0x1002e9a5 in
  `build/Release/NxPhysics.map` after the clean rebuild and disassembles to `sub ecx,0xc; jmp <the
  deleting destructor>`. The oracle's 0x100b33e0 has the same two instructions.
- **Dynamic (9 rows, 5,851 B):** 004366, 004141, 004121, 004097, 004725, 004332, 004125, 004129,
  004078. The trimmed trace is `evidence/unit-pilot-revolute-trace.txt`: the breakpoint list and the
  hits for each case. Task 10 ran a cdb breakpoint trace of `NxPhysicsJointTests` against
  `build/pairs/candidate` (same sha256 as `build/Release/NxPhysics.dll`), with addresses taken from
  the map. Each of these rows was hit in every one of the four cases; 004097 was hit twice per case.
  The trace also hit 004437, 004441, 004483 and 004539. Those are folded NpJoint bodies that this
  pilot does not claim, so they get no proof here. 004107 (inlined into 004141, or at least never
  observed out of line) and the destructors 004368, 004095 and 004729 were not hit, so they get no
  dynamic proof. The other 64 rows are source-only: they are reviewed against the listing and pass
  the no-regression gate, but nothing shows them executing.

Rows left unchanged:

- **15 deferred stubs (11,077 B)** keep their state. Each has body `NX_ASSERT(0)` and a
  `// (deferred: ...)` line; the contract's `## Dependency closure` defer table gives the reasons.
  - `core/Joint.cpp`:
    - 004064: slot 8 projection
    - 004093: solver slots 6/7; needs Scene 000598
    - 004099, 004101, 004109, 004143: public setGlobalAnchor, setGlobalAxis, setLimitPoint and
      addLimitPlane, which the test never calls
    - 004111: break test
    - 004123: debug visualisation
    - 004133, 004135: solver slots 6/7
  - `core/JointSupport.cpp`:
    - 004391: solver
    - 000022 and 000758: slot-8 helpers owned by other units
    - 000571: Scene-owned; already `reconstructed` on its own row, unchanged
    - 000633: Scene joint removal, reached only on release
- **13 folded NpJoint bodies (601 B)**, including 004437, 004441, 004483 and 004539: they are
  implemented as NpRevoluteJoint methods but not claimed, so their rows stay with their own units.
- **Out-of-pilot rows:** Joint.cpp's 004085, 004091, 004103, 004105, 004113, 004115, 004117 and
  004119, and NpCylindricalJoint's 004675, 004677 and 004679.

Ledger: `gates/phase6-closure.json` moves 48 entries from `not_reconstructed_in_phase` to
`reconstructed_not_falsified`, using the ledger's existing note text for that reason. Counts go from
302/129 to 254/177, and `differential_falsified` stays at 2. All 48 rows are Phase 6 rows. The
validator named no further correction: no `program.json` change, no other ledger. No row is above
`reconstructed`.

Open items carried forward:

1. **Release is unwired.** `NpScene::releaseJoint` is empty and `nxSceneAddJoint` (000661) is a
   no-op, so revolute joints live until scene teardown. The release path 004368 -> 004095 is
   written but has never run, and its `~Joint` step reaches the deferred 000633.
2. **The generic `createJoint` path lacks step 12** (0x14529-0x1453f). Only the revolute path has
   the shared exit.
3. **Rotated-body conventions are untested** (contract open issue 3). The test bodies are only
   translated, so only the identity case of the +0x5c quaternion and +0xdc 3x3 conventions is
   confirmed.
4. **The Phase 2/3 test targets** (`NxPhysicsInternalTests`, `NxPhysicsCollisionTests`) do not
   build because of an include-path problem that predates the pilot. Once that is fixed they will also
   need `Physics/src/core/*.cpp` at link time, because `Scene.cpp` now references `RevoluteJoint`.
5. **Body +0x204 is unbuilt** (contract open issue 7). The solver-slot rows 004374, 004360
   and 004362 need body +0x204, a `JointSupportBody*`, and the candidate's body record never
   writes it. 004360/004362 also need 004093's Scene record array (+0x5b8, row 000598). None of
   these is on the transcript path, and none has run. 004356 and 004364 end in deferred stubs
   (open issue 9).
6. **Known codegen divergence in 004729 (`~NpRevoluteJoint`, deleting destructor).** 004729 is
   the jump target of the 004727 adjustor thunk. The candidate's copy at 0x1002e9b0 opens with an
   SEH frame and a /GS cookie:

   ```
   push ebp; mov ebp,esp; push -1; push 0x100b1570; mov eax,fs:[0]; ...; mov eax,[0x100c7480]; xor eax,ebp
   ```

   The oracle's 0x100b33f0 is frameless:

   ```
   push esi; mov esi,ecx; ...; ret 4
   ```

   The thunk's own two instructions match, so 004727's proof is unaffected. 004729 stays
   `reconstructed` on the pilot's source bar, and its inventory `notes` record the difference.
   The cause is not yet investigated; the likely candidates are the EH and /GS settings on this
   translation unit, or the compiler-generated destructor chain. The destructor has never run
   under a transcript (release is unwired, item 1).

## Defects found by the transcript

None. With the wiring from Task 10, the Phase 6 staged-pair transcript was byte-identical on the
first run: `stdout_delta=0 stderr_exact=True` and 11/11 coverage assertions. That includes the
anchor and axis output of cases 1-3, and it still holds after the clean rebuild in
`## Verification`. The defects the pilot did find came from review against the listing, not from the
transcript. Examples are the contract's solver body order at +0x24 (Task 6) and many
decompile-vs-listing differences in grouping and rounding; the timing rows for Tasks 6-8b list
them. The wiring also corrected two defects in the candidate's generic path. Both were found
from the listing (contract Task 4 and Task 10), not by the transcript: the public object was read
at byte +0x10 instead of +0x48, and a null +0x48 was claimed to fall through instead of deleting
the joint and returning 0. Treat "no defects" with caution: the test drives 9 of the 73 rows, and only with
identity-oriented bodies.

## Comparison

**Method.** For both sides, rows moved are rows whose inventory `state` changed, and bytes are
those rows' `size`. Hours are elapsed wall-clock time between commit timestamps
(`git log --format='%ci'`) or timing-table entries. Row counts come from comparing the two
committed `inventory.json` versions with a script (`git show <rev>:inventory.json`).

- **Pilot, writing + wiring (Tasks 6-10).**
  - Window: 2026-09-25T01:50:58 (Task 6 start) to 04:32:53 (`dc11888`, the Task 10 review fix),
    which is 2.70 h. The window includes every review loop and fix commit.
  - Output: 48 rows / 26,321 B moved to `reconstructed` (this task's inventory diff). 73 rows /
    27,598 B were given native source.
  - Rate: **17.8 rows/h, 9,750 B/h** moved. Counting all 73 rows given source, it is 27.0 rows/h and
    10,220 B/h.
  - The implementer intervals alone (the timing rows' own start and end, 105 min, with review time
    excluded) give 27.3 rows/h and 14,980 B/h. That is an upper bound, not the rate to quote.
- **Pilot tooling (Tasks 1-5).**
  - Window: 00:20:00 to 01:49:47, 1.50 h, with no rows moved. The work was the work-unit map, the
    Ghidra supplement, the bundles, the contract and the scaffold.
  - Including it (00:20:00 to 04:32:53, 4.21 h): **11.4 rows/h, 6,250 B/h**. Task 11 adds about
    0.25 h more.
- **Method-level packets, 2026-09-24.**
  - Range: `4de9bd1`..`5ebb0be`, 45 commits. Timed from the preceding commit `30dfd8f`
    (13:28:03) to `5ebb0be` (20:33:36), which is 7.09 h.
  - Diffing `inventory.json` from `4de9bd1~1` to `5ebb0be`: 9 rows / 5,448 B moved
    `discovered` -> `reconstructed`, and no row moved in any closure ledger.
  - Rate: **1.27 rows/h, 768 B/h** moved. 33 rows / 13,323 B had any field changed (mostly
    `dynamic_proof`/`static_proof`/`source` on rows already `reconstructed`), which gives
    4.65 rows/h and 1,878 B/h.
  - 22 of the 45 commits touch the inventory. The range also contains shape and mesh vtable-probe
    commits, not just actor rows.

**Headline.** On state-moved rows the pilot ran about 14x the packets' row rate and about 13x their
byte rate (17.8 vs 1.27 rows/h; 9,750 vs 768 B/h). On the gentler "rows touched" measure the gap is
about 5.8x in rows and 5.4x in bytes. Including the one-time tooling cost, the gap is about 9x on
moved rows.

**Caveats. These make the ratios an upper bound on the method's advantage, not a like-for-like
speed-up.**

- **The bar differs.** A pilot row moved on source written from the listing plus review against the
  listing, with no transcript regression. Only 9 of the 73 rows are shown to execute, and none has a
  behavioural differential of its own. Each 2026-09-24 packet shipped a behavioural differential for
  its rows: that range adds 8,137 lines across 57 files, mostly test harnesses and probes, against
  3,304 lines under `Physics/`. That test work is output the rows-moved measure does not count.
  A like-for-like comparison would need the pilot's 64 unexercised rows driven too.
- **The row mix differs.** The pilot's bytes are dominated by six large x87 rows (15,268 B in Task 8),
  whose value checking is deferred. The packets' rows were individually driven actor and shape
  behaviour. Also, 25 of the pilot's 73 rows were already `reconstructed` as models; this is why the
  pilot is also compared on rows touched.
- **Wall-clock is not effort.** The pilot ran as a sequence of implementer subagents, each followed
  by spec and code-quality reviewer subagents and fix rounds. The hours above are elapsed time, not
  agent-hours or tokens, and the review loops are included in them.
- **The agent may differ.** The completion plan's status notes say its execution ran in the
  `codex/nxphysics-completion` worktree. Those packets may have been driven partly by a different
  agent or model. Their elapsed time may also include idle gaps that were not work.
- **One unit, one sample.** This is a single pilot on one well-bounded unit family with a
  ready-made transcript.

## Verification

Fresh configure (`cmake -S . -B build -A Win32 --fresh`) and `--clean-first` build of
`NxPhysics`, run 2026-09-25T04:38:52 to 04:42:21 on the Task 11 working tree (inventory, ledger and
bundles updated; product code as of `dc11888`):

```
build exit 0            (no warnings from Physics/src/core; pre-existing C4291 in FoundationSDK/PhysicsInternal/PhysicsSDK/Scene.cpp
                         and C4005 'ARRAYSIZE' macro redefinition, winnt.h vs opcode-tree Ice/IceUtils.h)
public_headers=pass files=80
Ran 643 tests ... OK
phase 2 exit 1   GATE FAILED: build_physics exited 1
                 ObjectModel.h(11,10): error C1083: Cannot open include file: 'IcePrunable.h' [NxPhysicsInternalTests.vcxproj]
phase 3 exit 1   GATE FAILED: build_physics exited 1
                 ObjectModel.h(11,10): error C1083: Cannot open include file: 'IcePrunable.h' [NxPhysicsCollisionTests.vcxproj]
phase 4 exit 0   coverage_assertions_evaluated=101 floor=101 / phase_gate=4 status=pass
phase 5 exit 1   candidate CANDIDATE-MISSING family=vtables reason=shape finals/actor classes are Tasks 3-4
                 gate_failure=oracle_differential:NxPhysicsObjectLayoutTests exited 1 (coverage_assertions_evaluated=829 floor=829)
phase 6 exit 0   inventory=pass unexplained=0
                 differential target=NxPhysicsJointStagedPairTests oracle_exit=0 candidate_exit=0 stdout_delta=0 stderr_exact=True
                 coverage_assertions_evaluated=11 floor=11 / phase_gate=6 status=pass
phase 7 exit 0   coverage_assertions_evaluated=4 floor=4 / phase_gate=7 status=pass
```

- **Phases 2 and 3 were already failing at the branch's starting commit `1a14ad8`.** The failure
  was confirmed independently there, and Task 10 reproduced it with HEAD's `Scene.cpp`. The
  standalone test targets `NxPhysicsInternalTests` and `NxPhysicsCollisionTests` compile
  `NpActor.cpp`, which includes `ObjectModel.h` (since `1378003`), which includes `IcePrunable.h`,
  which is not on those targets' include paths. It is not fixed here (see open item 4).
- **Phase 5 fails only on its existing `CANDIDATE-MISSING family=vtables` marker.** All 12 of its
  staged-pair differentials report `stdout_delta=0`.
- `validate_inventory.py` exits 0 standalone as well (`inventory=pass`, `closure phase=6 closed=2
  deferred=431`, `unexplained=0`).
