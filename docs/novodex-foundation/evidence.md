# Novodex Foundation reconstruction evidence

## Task 10 close-out verification

Foundation reconstruction gates are closed at Novodex implementation HEAD `62b26f4f672ea70b73742302db52c67e338565e3`. The checked-in [`tools/run_final_verification.ps1`](tools/run_final_verification.ps1) performed the reproducible consumer-read-only audit from clean evidence HEAD `391a1cf176f3c0a0e409c9737f3c1b5fc6d05a6f`. It may fresh-build Novodex and replace only its three exact Task 10 evidence outputs; it never deploys, removes, or launches consumer binaries. Its authoritative raw proof is [`dumps/task10_final_verification.raw.txt`](dumps/task10_final_verification.raw.txt), its exit/result record is [`dumps/task10_final_verification.result.txt`](dumps/task10_final_verification.result.txt), its exact live export output is [`dumps/task10_live_exports.txt`](dumps/task10_live_exports.txt), and [`dumps/task10_final_verification.txt`](dumps/task10_final_verification.txt) is only the concise human-readable summary.

The runner owns the raw transcript and records its exact direct invocation and final script exit in addition to every resolved child command, stdout, stderr, and exit. It fresh-configured, explicitly cleaned, and Release-built all five targets at `D:/github/Novodex`; logged four Export/SDK and 24 cluster processes; recorded each of the 12 modes exactly twice with resolved oracle/candidate paths; and ran CustomArray. All processes exited 0 and all 12 normalized stdout/stderr pairs were exact. The locked inventory code validated four and only four `pass` gates, 14 final decisions, the exact unique 134-name partition, and 134/134 raw joins. The 48 manifest rows record all 192 live raw/blob hashes; all 96 supporting binary Git-blob commands record their resolved command, byte count, SHA-256, stderr, and exit without repeating entire public-header bodies. Six Python tests passed with no bytecode cache. The transcript is bounded at 136,982 bytes / 1,913 lines, SHA-256 `641066f7a1a3950c8977aa4f2e9a19d0c937a6eee4e12a2bc3f34efb36ff5e47`, and has 146 command/exit records, 24 cluster commands (12 oracle/12 rebuilt), zero placeholders, zero failing exits/assertions, zero personal-identity/header matches, and zero credential-pattern matches. The clean-start status had zero changes in both repositories; after execution only raw/result were dirty, while deterministic `task10_live_exports.txt` remained clean, and direct content/semantic checks passed for all three generated artifacts.

The verification-time candidate is `D:/github/Novodex/build/Release/NxFoundation.dll`, SHA-256 `161438b06797246c39c9febe1dc5aba2702a36504bd5e91069ff512018ece332`, 57,344 bytes, COFF `0x014c`, PE32 magic `0x10b`, file/product version `2.1.2.6000`. This differs by hash only from earlier clean builds because modern-MSVC output is nondeterministic; its live 140-name export dump exactly equals `dumps/exports_rebuilt.txt`, with 134 required, missing 0, six documented extras, and recorder 0.

Scope review found no public/version header changes after transplant `ac804727f9565f4266aab3b794834ab5551d127f`, no Physics or Engine glue changes, and no DLL committed in the implementation range through `62b26f4f672ea70b73742302db52c67e338565e3`. Baseline tag `pre-ue3-headers` is asserted to resolve to `528a7bfef8d3816c36135ff3768b5b0c2ed68641`. The 41-commit evidence range from base `737b273367061e61a46d53f1c5fd3828c327f124` through audit input HEAD `391a1cf176f3c0a0e409c9737f3c1b5fc6d05a6f` changes 73 paths, all inside this reconstruction subtree, and commits no DLL.

The Task 9 consumer result remains the governing smoke criterion: candidate import load and source-backed Foundation/Novodex SDK bind equivalent to the pinned oracle through `Initializing Engine Completed`. The same later `0x80000003`/`KERNELBASE.dll` breakpoint occurs with the oracle and remains a known post-bind consumer-baseline limitation; no clean benchmark exit, gameplay stability, or soak is claimed. Close-out performed no deployment or consumer launch. The installed DLL is the exact pinned oracle (`7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990`, 172,032 bytes), with no backup or consumer process remaining; the fresh candidate is retained at its build path.

Non-blocking limits remain explicit: three C4291 legacy allocation-cleanup warnings, six accounted modern-MSVC export extras, non-exhaustive selected-behavior coverage, unavailable Kawaiidra export enumeration with approved PE/Capstone fallback, and nondeterministic binary hashes across clean builds. Physics reconstruction is deferred to a separate follow-on design and is not part of this Foundation closure.

## Task 9 consumer smoke — PASS on bind criterion

The audited Win32 Release candidate of record is `D:/github/Novodex/build/Release/NxFoundation.dll`, SHA-256 `80913618b823789f016081260fcdc1902e7f8eac9a1fa6003c16357d3dd72704`, 57,344 bytes, PE32 x86 (`machine=0x014c`, optional-header magic `0x10b`), file/product version `2.1.2.6000`. A fresh configure and clean-first build produced `NxFoundation` plus the Export, SDK, Cluster, and CustomArray loader targets. Complete stdout/stderr and exit-code records are preserved in `dumps/task9_rerun_configure.*`, `task9_rerun_clean.*`, and `task9_rerun_build.*`. The fresh export gate (`task9_rerun_export_compare.txt`) reported `oracle=134 rebuilt=140 missing=0 extra=6`; the raw oracle/candidate Export/SDK runs in `task9_rerun_export_sdk_tests.txt` all exited 0.

The same exact retained/deployed candidate identity is now tied to the complete Task 8 gate set, not merely the Export/SDK subset. [`dumps/task9_candidate_full_gates.txt`](dumps/task9_candidate_full_gates.txt) hashes the candidate before and after, records all 24 separate absolute-path `NxFoundationClusterTests` oracle/candidate commands, preserves raw stdout/stderr and exit codes, and reports `modes=12`, `exit_failures=0`, `output_mismatches=0`. Every already-normalized pair is case-sensitively exact. The private `NxFoundationCustomArrayTests` run exits 0 with the expected output. Candidate `80913618...72704` and pinned oracle `7e0596e4...0990` identities remain unchanged after all tests.

`DemoGame.exe` is the supported consumer: both local DemoGame scripts and current build/install logs name it, while `DemoEngine.ini` provides a real `Entry.war` game configuration. The exact executed procedure is checked in as [`tools/run_consumer_smoke.ps1`](tools/run_consumer_smoke.ps1), and both exact outer invocations are recorded verbatim in `spike.md` and the generated transcripts. Candidate/oracle raw logs, redirects, module polls, WER queries, and transcripts use the `dumps/task9_audit_candidate.*` and `dumps/task9_audit_oracle.*` prefixes.

Source order is `UGameEngine::Init` -> `InitGameRBPhys` -> checked `NxCreatePhysicsSDK` -> SDK parameter calls -> entry-map load; candidate PID 2596 reached `Initializing Engine Completed`. Its transcript hashes the source candidate, installed pre-start DLL, and installed DLL again after exact-PID WER capture as the same `80913618...72704` bytes. WER listed the deployed `NxFoundation.dll` and existing `NxPhysics.dll` at their exact Binaries paths, then recorded `0x80000003` at `KERNELBASE.dll+0x001fa5e2`. Live polling recorded 21 empty polls without an error; this is preserved as a proof limit rather than replaced by WER evidence.

Pinned-oracle PID 31172 used the same checked-in procedure, executable, flags, working directory, hidden window, redirects, and 90-second outer bound. Its 12 normalized milestones are line-for-line equal to the candidate through `Initializing Engine Completed`; it then produced the same unavailable wrapper exit code, exception, module/offset, WER bucket `88a4a13ea901cd3c6ae88e1119e106a6`, and Binaries DLL paths. The oracle hash is exact before launch, at WER capture, and after the procedure. [`dumps/consumer_smoke_differential.txt`](dumps/consumer_smoke_differential.txt) is the concise comparison. This demonstrates that the post-bind crash is baseline consumer behavior, with no earlier candidate-only difference.

The plan defines success as import load plus Foundation/Novodex SDK bind, not a full gameplay soak. On that explicit criterion the candidate passes equivalently to the pinned oracle, so `inventory.json` sets `consumer_smoke: pass`. The proof limit remains material: no benchmark completion, numeric zero exit, live module-poll row, or gameplay stability is claimed.

Recovery completed in the checked-in procedure's `finally`: the verified oracle backup was restored, the exact pinned 172,032-byte hash was rechecked, and only then was `NxFoundation.dll.oracle.bak` removed. The transcript records every identity boundary. The candidate remains at its build-of-record path. The oracle control did not modify the installed oracle. No consumer process or newly generated benchmark log remains.

## Task 8 final gates

All 14 clusters are closed: `c_sdk`, `c_fpu`, `c_util`, and `c_debug` are `PATCH`; the other ten are `KEEP` (with zero-export `c_misc` explicitly `measured-private`). The exact partition is 134 assigned names, 134 unique names, no duplicate groups, and no missing or extra oracle names. `structural=pass` covers all five exported vftables (the two FoundationSDK tables, Exception, Observable, and DefineZone) plus measured boundary sizes/layouts for FoundationSDK 56, Exception 16, Observable 16, DefineZone 56, Time 8, and private DebugRenderable 52. `selected_behavior=pass` means every non-trivial cluster has its documented Capstone and/or separate-process differential gate; it is selected-path evidence, not exhaustive equivalence. Task 9 subsequently set `consumer_smoke=pass` on the narrow import-load plus SDK-bind criterion documented above.

Verification is recorded in [`dumps/task8_final_verification.txt`](dumps/task8_final_verification.txt). After geometry hardening/patch commit `62b26f4`, a fresh-configured, clean-first Win32 Release build succeeded. The export and SDK loaders pass separately against both DLLs; all 12 exported cluster modes exit 0 against oracle and rebuild with exact normalized output equality; the private CustomArray unit exits 0. The verification-time artifact is PE32 x86 version `2.1.2.6000`, SHA-256 `135763400e9581d54065638bf37e9d0cc14d1575a189bfcb3bbeb770a34dc9c7`, size 57,344 bytes. This hash pins only that build output: fresh clean rebuilds have produced different hashes despite unchanged behavior/export facts, so PE build nondeterminism is explicitly observed. Task 9 must record the exact newly built and deployed candidate hash rather than treating this hash as timeless. Export comparison remains oracle 134/rebuild 140/missing 0/extra 6, recorder exports 0, and the committed rebuilt dump equals live parsing. All 48 immutable files pass all four raw/blob hash fields; six Task 2 tool tests pass with zero `__pycache__` directories.

## Task 8: `c_misc` — KEEP (`measured-private`)

The exact PE export partition assigns zero names to this cluster, so there is no oracle entrypoint, data target, or vftable to label or invoke. `CustomArray.cpp` is private and unchanged from the imported baseline; its only FoundationSDK call sites are behind disabled `DEBUG_DETERMINISM`, and fresh oracle/rebuild export parsing finds zero recorder exports. A dedicated Win32 Release source-unit gate nevertheless exercises representative private behavior: 4-to-8-byte block growth, exact heterogeneous binary layout and typed readback, address backpatching, bit packing/readback, ASCII storage, exact `FILE*` stream output, collapse, and independent copy/assignment. See [`dumps/misc_custom_array.txt`](dumps/misc_custom_array.txt). Decision: `KEEP`; the proof is intentionally local to unchanged private source, while oracle parity is limited to the demonstrable absence of assigned/recorder exports.

## Task 8: `c_debug` — PATCH

All three assigned SDK exports are Capstone-attached at RVAs `0x00003f10`, `0x00003bd0`, and `0x000034c0`. A recording allocator proves a 52-byte object and all 15 public-interface slots are executable. Every selected primitive/generated record is serialized as all float bits plus color in insertion order. Stable low-level records and every AABB/frame endpoint are also hard-asserted; clear/buffer reuse and SDK ownership remain checked. A wrong line endpoint demonstrates the old color-only predicate's false-positive. The strengthened RED then exposed five real `addCircle` coordinate mismatches. Oracle vtable slot 14 RVA `0x00001930` uses extended x87 trig and rounds the second angle before trig; the minimal private Win32 x87 implementation patch restores those exact selected records. All 59 generated lines now compare bit-for-bit, with captured output-text SHA-256 `FA003E5A47C3CE945D9FA1DEA770137F15C63B6BE4860123E36C16C15E928B3F` for each process. See [`dumps/debug_differential.txt`](dumps/debug_differential.txt) and [`dumps/debug_circle_capstone.txt`](dumps/debug_circle_capstone.txt). Decision: `PATCH`.

## Task 8: `c_volume` — KEEP

The sole assigned export is attached at Capstone RVA `0x00007220`, and `VolumeIntegration.cpp` is unchanged from the imported source baseline. The separate-process gate proves the null-allocator false/unchanged-output path, then checks analytic mass, center of mass, origin inertia, and center-of-mass inertia for a tetrahedron using 32-bit indices, a translated density-scaled tetrahedron, and reversed 16-bit indices with `NX_MF_FLIPNORMALS`. Both binaries exit 0 with identical normalized output; see [`dumps/volume_differential.txt`](dumps/volume_differential.txt). Clean-first Release/export verification remains `134/140`, missing `0`, six documented extras, recorder exports `0`, and saved/live equality. Decision: `KEEP`; no production change. The documented `1e-4` tensor tolerance accommodates measured modern-compiler normalization/cancellation drift without obscuring the analytic cases; degenerate and invalid meshes remain outside the proof.

## Task 8: `c_ray_seg` — KEEP

Both assigned exports are Capstone-attached and exercised. Ray coverage includes only valid normalized-direction cases: interior projection, origin clamping, an on-ray point, null `t`, and aliasing `t` into the input point. Segment coverage includes interior projection, both endpoint clamps, zero length, and the same supported output/input alias pattern. Oracle and rebuild each exit 0 with identical normalized output; exact commands and RVAs are in [`dumps/ray_seg_differential.txt`](dumps/ray_seg_differential.txt). Clean-first Release/export verification remains `134/140`, missing `0`, six documented extras, recorder exports `0`, and saved/live equality. Decision: `KEEP`; no production change. Immutable `NxRay.h` requires a normalized direction, so both non-unit and zero directions are consistently excluded. The preliminary differing non-unit probe remains a non-authoritative characterization, and neither invalid direction is claimed as parity coverage.

## Task 8: `c_sphere` — KEEP

All three exports are Capstone-attached and exercised. Fast construction covers null, single, diameter pair, and multi-point enclosure; robust construction covers empty `NX_BS_NONE` and the oracle's measured `NX_BS_GEMS` path. Merge coverage includes disjoint, containment in both argument orders, coincident, touching, and first-input/output aliasing. Oracle and rebuild each exit 0 with identical output; see [`dumps/sphere_differential.txt`](dumps/sphere_differential.txt). Clean-first Release/export verification remains `134/140`, missing `0`, six documented extras, recorder exports `0`, and saved/live equality. Decision: `KEEP`; no claim is made for the problematic four-point miniball branch.

## Task 8: `c_capsule` — KEEP

Both exports are Capstone-attached and exercised. Capsule-to-box covers X/Y/Z segment axes with exact center/extents and all nine orientation values emitted in row order. Hard invariants cover all row and column norms, every pairwise row/column dot, and determinant handedness; the exact oracle bit matrices are asserted. Flipping both secondary rows preserves the former row0+det predicate, demonstrating its false-positive, but fails the strengthened gate. Box-to-capsule retains largest-axis, tie, translated-center, and zero-box cases. Oracle and rebuild output is exact (captured text SHA-256 `246404B439A1EC874E0946CF8C45EA8D301F4BF3090FC1A59330437B33C3DF0E`); see [`dumps/capsule_differential.txt`](dumps/capsule_differential.txt). Decision: `KEEP`; zero-length capsule normalization is not claimed.

## Task 8: `c_box` — KEEP

All 13 assigned exports have individual Capstone attachments. The gate hard-checks and bit-serializes every component of all six planes, eight points, and eight vertex normals in order. A deliberately wrong point[1] demonstrates that the former point[0]/point[6] sampling predicate false-passed; the strengthened oracle gate rejects it. Oracle/rebuild complete geometry text is exact (captured text SHA-256 `0C49B09FEA8AA4559E05BF52CDAFEA7E68209261A913D2DD5262878736112060`). Topology tables and canonical vertex-to-quad rows remain exact, as do the documented containment paths. See [`dumps/box_differential.txt`](dumps/box_differential.txt). Decision: `KEEP`; no production change. Non-orthonormal rotations and invalid indexes are excluded.

## Task 8: `c_util` — PATCH

All five exports are attached and exercised. CRC coverage extends beyond Task 6 to empty, canonical text, all byte values, `0xff`, embedded NUL, and a 4096-byte input. Bounds cover the no-op empty/null contract and duplicated extrema; tangents cover both oracle branches and orthonormal invariants; inertia covers diagonal and nontrivial symmetric positive tensors. Rotation covers general, parallel, and antiparallel unit vectors using immutable public `NxMat33::operator*` semantics.

The focused RED was oracle exit 0 versus rebuild exit 1: `NxFindRotationMatrix` mapped unit X to negative Y in the rebuild. Oracle matrix output and Capstone RVA `0x00005f20` supported reversing only the cross-product arguments. GREEN produces identical normalized oracle/rebuild output across the entire utility gate. Because `Utilities.cpp` contains a legacy non-UTF8 byte, the user explicitly authorized `git apply` for this line only. The byte gate proves equal 7,651-byte lengths, exactly changed offsets `5248..5255`, and legacy `0xF6` unchanged at offset `4956`; hashes and the rejected normalization attempt are documented in [`dumps/util_differential.txt`](dumps/util_differential.txt). Clean-first Release/export verification remains `134/140`, missing `0`, six documented extras, recorder exports `0`, and unchanged saved/live exports.

## Task 8: `c_fpu` — PATCH

All eleven exports are resolved and measured in separate loader processes. Capstone RVAs `0x000040d0..0x00004180` show the oracle calling its legacy CRT control helper with precision, rounding, and exception masks; integer conversion bodies begin at `0x000041c0`, `0x00004200`, and `0x00004270`. Direct `fnstcw` and `_mm_getcsr` measurement established the missing semantic detail: every oracle setter changes x87 only and leaves MXCSR byte-for-byte unchanged. Passing `false` to `NxSetFPUExceptions` leaves only the x87 denormal exception masked (`CW & 0x3f == 0x02`).

The focused RED was oracle exit 0 versus rebuild exit 1 at rounding-chop because modern `_controlfp` also changed MXCSR. The minimal patch replaces only the Win32 control wrappers with masked `fnstcw`/`fldcw` updates; integer conversion algorithms are untouched. GREEN gives identical normalized oracle/rebuild output for all three precision modes, all four rounding modes, both exception-mask states, exact incoming-state restoration, and seven finite positive/negative/fractional/integral/zero conversion vectors. Full commands and RED/GREEN values are in [`dumps/fpu_differential.txt`](dumps/fpu_differential.txt). The clean-first Release/export gate remains `134/140`, missing `0`, six documented extras, recorder exports `0`, and unchanged saved/live exports. Out-of-domain integer conversions and pending-exception delivery are not claimed.

## Task 8: `c_time` — KEEP

All six exports are resolved and exercised. Capstone identifies `QueryPerformanceCounter` conversion at `0x00005190`, cached `QueryPerformanceFrequency` conversion at `0x000051b0`, elapsed subtraction/epoch replacement and frequency division at `0x000051f0`, non-mutating peek at `0x00005250`, constructor seeding at `0x000052a0`, and exact 8-byte epoch assignment at `0x00004670`.

Oracle and rebuild run in separate processes and each exit 0 with the same normalized invariants: positive stable frequency, positive monotonic ticks, monotonic non-resetting peeks, elapsed reset behavior, and copied epochs. The timing policy uses intentionally broad scheduler-tolerant windows and does not compare nondeterministic absolute values; commands and limits are in [`dumps/time_differential.txt`](dumps/time_differential.txt). The clean-first Release and export gate remained `134/140`, missing `0`, the six documented extras, recorder exports `0`, and unchanged saved/live exports. Decision: `KEEP`; no production change. Counter failure, suspend/resume, CPU migration, and long-duration double precision remain untested.

## Task 8: `c_profiler` — KEEP

All 49 assigned exports have individual PE/code-or-data attachments in `dumps/oracle_pe_capstone.json`, and the loader requires every name to resolve. Capstone shows the SDK factory at `0x00002f70` allocating `0x38` bytes for `DefineZone`, named/default construction at `0x00004a60`/`0x00005170`, destruction and zone-list compaction at `0x000046a0`, and zone entry/exit state updates at `0x000048f0`/`0x00004970`. Full copy/assignment spans at `0x00003390`/`0x000033f0` copy `+4/+8` and `+0x10..+0x30`, intentionally not padding. Although a raw pointer walk from exported RVA `0x0001c288` continues through adjacent executable-pointer data, the immutable public `NxProfilingZone` interface plus runtime identity establishes only three semantic slots: `release`, `enter`, `leave`.

The separate-process profiler gate checks initial global dimensions/state, named and default construction, semantic-field copy/assignment, name/index/list placement, vtable order, direct and scoped enter/leave, tick monotonicity, dynamic SDK creation and release, standard deviation, sort ordering, mode selection, and normalized update invariants. Oracle and rebuild each exit 0 with identical normalized output; see [`dumps/profiler_differential.txt`](dumps/profiler_differential.txt). The initial both-binaries RED was a test defect—comparison of padding the oracle does not copy—and the dump records its Capstone-backed correction. The subsequent clean-first Release build/export gate remained `134/140`, missing `0`, six documented extras, recorder exports `0`, and unchanged saved/live exports. Decision: `KEEP`; no production change. Exact timestamp magnitudes and long-run profiler statistics are deliberately not claimed.

## Task 8: `c_observable` — KEEP

All ten assigned exports are gated. Capstone shows a 16-byte object: the constructor at `0x00003cf0` installs exported vftable `0x0001c2b0` and zeroes `+4/+8/+0xc`; those offsets are the pointer-array begin/end/capacity, corroborated by the count body at `0x000043d0`. The vftable contains only default event RVA `0x000042e0` before the next exported vftable. Copy and assignment RVAs `0x00003d40`/`0x00003d60` route the array at `+4` through deep-copy helpers, while destructor RVA `0x00003d10` frees non-null storage through the Foundation allocator.

Separate `NxFoundationClusterTests observable <dll>` invocations against the pinned oracle and rebuild each exit 0 with identical output. The gate resolves every assigned export, checks construction and one-slot vtable identity/order, exercises add/remove/count, verifies deterministic callbacks and removed-observer suppression, observes event `2` on last removal, proves independent storage after copy and assignment, and destroys all three objects. Raw evidence is in [`dumps/observable_differential.txt`](dumps/observable_differential.txt). The clean-first Release build and export comparison remained `134/140`, missing `0`, with only the six documented extras, no recorder exports, and an unchanged live export snapshot. Decision: `KEEP`; no production change. The selected behavior does not cover duplicate/missing observers, mutation during notification, self-deletion, allocation failure, or concurrency.

## Task 8: `c_exception` — KEEP

All seven assigned exports are gated. Capstone 5.0.6 x86-32 identifies a 16-byte `Exception`: the exported vftable at RVA `0x0001c210` points in order to getters at `0x00002d80`, `0x00002d90`, and `0x00002da0`; those bodies read `this+4`, `this+8`, and `this+0xc`. The value constructor at `0x00002e00` writes those same fields and the exported vftable. The copy constructor at `0x00002db0` and assignment at `0x00002de0` copy those fields, with assignment returning `this`.

The Win32 `NxFoundationClusterTests exception <dll>` loader uses only `LoadLibrary`/`GetProcAddress` and immutable public types. Separate oracle and rebuilt invocations each exit 0 with identical output for construction, getter values, byte-exact copy and assignment, return values, and three-slot vtable identity/order. The raw command, output, provenance, and structural facts are in [`dumps/exception_differential.txt`](dumps/exception_differential.txt). A clean-first Release Win32 rebuild then retained `oracle=134`, `rebuilt=140`, `missing=0`, the same six documented move-member extras, no recorder exports, and a live export dump identical to the committed snapshot. Decision: `KEEP`; no Foundation production source changed. This selected gate does not exercise invalid object pointers or ownership/lifetime beyond a stable caller-owned file string.

Historical Task 6 boundary: machine-readable cluster assignments live in `inventory.json`; per-export raw evidence lives in `dumps/oracle_pe_capstone.json`. At that earlier boundary the export ABI gate passed and only `c_sdk` had a final decision; the other cluster decisions and reconstruction gates were then still pending. The Task 8 sections above supersede that historical state.

## Task 7 `c_sdk` structural and selected-behavior gate

### Decision: PATCH

`c_sdk` is `PATCH`, not `KEEP`. Task 5 already changed this cluster's three live allocations from the obsolete object-like `NX_NEW type` spelling to the immutable 2.1.2 function-like `NX_NEW(type)` contract. Task 7 then removed the unreferenced private `currentID`/`freeIDs` storage and `getNewID`/`freeID` methods, which are not oracle exports, and reordered the remaining private members to the offsets directly observed below. No public header or root version header changed. `DefaultAllocator.*` required no Task 7 change; the exported default object is the immutable-header `NxUserAllocatorDefault` instance.

The RED was structural and reproducible. The first loader characterization showed one typed allocation of 56 bytes in the oracle versus 76 in the pre-patch rebuild. After the oracle value became a regression assertion, the pinned oracle exited 0 and the rebuild exited 1 with `FAIL FoundationSDK allocation size differs`. Oracle `NxCreateFoundationSDK` RVA `0x00004020` independently pushes `0x38` before the allocator virtual call at RVA `0x0000404f`. A local MSVC layout report identified the rebuild's extra private ID array/storage and debug array at `+0x3c`; repository search found no call sites for the ID methods. The minimal private-layout patch makes the same test GREEN at 56 bytes.

### Expanded oracle control flow

[`dumps/oracle_sdk_capstone.txt`](dumps/oracle_sdk_capstone.txt) extends the earlier eight-instruction windows while retaining PE-provided export RVAs and Capstone x86-32 decoding. It supports these bounded claims:

- `NxCreateFoundationSDK` compares the incoming version with `0x02010200` at RVA `0x00004020`; the unequal path returns zero at `0x0000402a..0x0000402c`. The create path selects the supplied allocator or data export RVA `0x00025040`, binds allocator-global RVA `0x00026980`, requests `0x38` bytes through a virtual allocator call, stores singleton RVA `0x00026984`, calls the error-stream virtual slot, sets the full-object byte at `+0x34`, and returns the interface subobject at full object `+0x14`.
- `release` RVA `0x00004090` clears the same lifecycle byte relative to the returned interface, adjusts `this` by `-0x14`, checks the observer count, invokes destructor RVA `0x00003e60` when it is zero, frees through the bound allocator, and clears singleton RVA `0x00026984`. `event` RVA `0x00003fe0` follows the analogous deferred-delete path for event value 2 when the lifecycle byte is false.
- `getErrorStream` RVA `0x00002f50` loads interface-relative `+4`; `getAllocator` RVA `0x00002f60` loads the allocator global; `getLastError`/`getFirstError` RVAs `0x00002fc0`/`0x00002fd0` return and clear interface-relative `+0x1c`/`+0x18`. `errorImpl` RVA `0x00002fe0` writes last/first state, branches on the output-stream pointer, formats the non-assert message, and dispatches to the output-stream virtual surface. These are instruction-level control-flow observations, not recovered source or proofs of every branch.

### Vftables, object offsets, and ABI surface

[`dumps/sdk_vtable_compare.txt`](dumps/sdk_vtable_compare.txt) records the safely bounded pointer walks and target-name joins for both binaries. The oracle and rebuild each have:

- a one-entry standalone `Observable` vftable (`event`);
- a one-entry `FoundationSDK` Observable-base vftable (`FoundationSDK::event`); and
- a ten-entry `FoundationSDK` primary/interface vftable in immutable-header order: `release`, `setErrorStream`, `getErrorStream`, `getLastError`, `getFirstError`, `getAllocator`, `createProfilingZone`, `createDebugRenderable`, `releaseDebugRenderable`, `renderDebugData`.

The oracle primary table is RVA `0x0001c2b4`, bounded by the secondary table at `0x0001c2dc`; the secondary run stops after one pointer at a non-executable dword. The rebuilt primary table is RVA `0x0000b204`; its preceding secondary table is RVA `0x0000b1fc`, and each run stops at the next non-executable dword. Absolute RVAs and adjacency order differ and are not asserted as ABI requirements.

Directly recoverable object facts are limited to: PE32 four-byte pointers; full allocation size 56 from both runtime allocation recording and oracle `push 0x38`; Observable subobject at full object `+0`; returned `NxFoundationSDK` interface subobject at `+0x14`; error stream at full object `+0x18`; debug-array pointer triple at `+0x1c..+0x24`; first/last errors at full object `+0x2c/+0x30`; lifecycle byte at full object `+0x34`. Other padding, RTTI metadata, and copy/assignment layout semantics remain unknown.

The immutable `NxFoundationSDK.h` declares ten virtual methods in the same order and declares `NxCreateFoundationSDK(NxU32, NxUserOutputStream*, NxUserAllocator*)` as `NX_C_EXPORT NXF_DLL_EXPORT` with `NX_CALL_CONV`; immutable `Nx.h` resolves `NX_CALL_CONV` to `__cdecl` on Win32. Decorated exports encode the expected x86 member/static roles (`UAE` virtual member entries, `SA` static helpers), and the undecorated C create export is present in both images. This checks the key published signatures/calling conventions; it does not prove all private exported constructors, copying, varargs, or RTTI behavior.

### Differential GREEN and proof limits

The focused loader is `tests/FoundationSDKTests.cpp`, built as `NxFoundationSDKTests`. Oracle and rebuild run in separate processes. It covers exact-version create, wrong-version rejection without allocator-global mutation, singleton reuse, stream replacement, sticky allocator behavior, typed allocation/free activity, release/recreate lifecycle, initial and clearing error accessors, and a short `NXE_DB_WARNING` callback through the exported static error helper. After the patch, each process exits 0 and their seven output lines compare exactly (`output_diff_count=0`); see [`dumps/sdk_differential_red.txt`](dumps/sdk_differential_red.txt) and [`dumps/sdk_differential_green.txt`](dumps/sdk_differential_green.txt).

The results establish only the selected paths above. They do not cover assertion responses, the long-message realloc loop, allocator failure, live observers/deferred deletion, copy/assignment helpers, concurrent access, profiling/debug-render behavior, or exhaustive error codes. Kawaiidra was not queried again because its already-recorded enumeration/decompile interfaces are nonresponsive; no Kawaiidra-derived decompilation claim is made. Per the phase boundary, `structural` and `selected_behavior` remain globally `pending` until Task 8 completes all clusters.

### Task 7 final verification

- Clean-first build: `cmake --build build --config Release --target clean`, followed by the `NxFoundation`, `NxFoundationExportTests`, and `NxFoundationSDKTests` Release targets; all exited 0. The three pre-existing C4291 placement-allocation cleanup warnings remain.
- Separate-process differential: oracle exit 0, rebuild exit 0, `Compare-Object` output difference count 0. The older focused export loader also exits 0 for each DLL.
- Final export gate: oracle 134, rebuilt 140, missing 0, extra 6, compare exit 0. The committed `dumps/exports_rebuilt.txt` compares exactly to a fresh parser run; recorder exports 0. The six extras are the previously documented modern-MSVC move construction/assignment members. The two prior private ID-method extras disappeared with the oracle-backed layout patch.
- Final post-review clean-build DLL: SHA-256 `77d45baa120c12c55832e2709adb2eca671068ef659317a6204c0759082dfd39`, size 57,344 bytes, COFF machine `0x014c`, PE32 magic `0x10b`, file/product version `2.1.2.6000`. The hash is a pin for this final build output, not a reproducible-build claim.
- Immutable manifest: 48 paths, raw mismatches 0, current-Git-blob mismatches 0. Tool tests: 6 passed; `__pycache__` directories under the evidence tools: 0.
- Source/test commit: `b94320b` (`foundation: match SDK lifecycle layout`). Only `c_sdk` changed from `pending`; all other decisions and the three remaining global gates are still pending.
- Review hardening commit `12e7a59` gates `NxFoundationSDKTests` to Win32 four-byte-pointer configurations because its decorated export names, 56-byte layout assertion, and oracle are x86-specific. Fresh never-before-used Visual Studio build trees generated this target for Win32 and omitted it for x64; the general `NxFoundationExportTests` target remained in both.

## Task 6 export ABI gate

### RED and root cause

After implementation, the regression RED was reconstructed independently and auditably from the exact pre-fix parent. A temporary detached worktree at commit `979ee1bbd5feeb14f8f5568fbf620c4320d8d8cc` received only the test-target CMake lines and `tests/FoundationExportTests.cpp` from Task 6; `Utilities.cpp` and `Sphere.cpp` matched the parent. It was freshly configured with Visual Studio 18 2026, Win32, and built as Release. The temporary DLL had SHA-256 `58b45affb8d871f6e3dbb4b5d651192dea42863b2e6cd02c2da474922032b356` and size 57,344 bytes. The worktree was removed after capture.

The complete parent export list is [`dumps/exports_rebuilt_initial.txt`](dumps/exports_rebuilt_initial.txt). Deterministic command/result records are [`dumps/export_compare_initial.txt`](dumps/export_compare_initial.txt) and [`dumps/test_initial_red.txt`](dumps/test_initial_red.txt). These are explicitly reconstructed regression artifacts; they are not claimed to have been committed before the implementation.

The captured commands included:

```powershell
cmake -S . -B build --fresh -G "Visual Studio 18 2026" -A Win32
cmake --build build --config Release --target NxFoundation
cmake --build build --config Release --target NxFoundationExportTests
build\Release\NxFoundationExportTests.exe build\Release\NxFoundation.dll
python <evidence-root>\tools\pe_exports.py build\Release\NxFoundation.dll | Set-Content -Encoding utf8 <evidence-root>\dumps\exports_rebuilt_initial.txt
python <evidence-root>\tools\compare_exports.py --oracle <evidence-root>\dumps\exports_oracle.txt --rebuilt <evidence-root>\dumps\exports_rebuilt_initial.txt
```

The comparison exited 1 with `oracle: 134`, `rebuilt: 140`, `missing: 2`, and `extra: 8`. The only missing names were `NxCrc32` and `NxMergeSpheres`. Their immutable declarations were already correctly marked `NX_C_EXPORT NXF_DLL_EXPORT` with `NX_CALL_CONV`; the owning `Utilities.cpp` and `Sphere.cpp` simply had no definitions.

The focused Win32 loader gate exited 0 against the pinned oracle and exited 1 against the reconstructed parent DLL with both `GetProcAddress` results null. The self-contained oracle control is [`dumps/test_oracle_green.txt`](dumps/test_oracle_green.txt), paired with [`dumps/test_initial_red.txt`](dumps/test_initial_red.txt). This distinguishes missing behavior/exports from a test-build defect.

### Shipped import-library binding mode

The installed MSVC `dumpbin.exe` 14.51.36252.0 inspected the shipped link oracle `D:/FlamingEnt__/Unreal_3/Development/External/Novodex/Foundation/lib/win32/Release/NxFoundation.lib` (SHA-256 `4d86d8371c80192b8766f9b0e7f43cf349ba03d00d3577203cf3b03d29d5a161`) with `/HEADERS`. All 134 short-import records target `NxFoundation.dll`; their `Name type` values are 96 `name` and 38 `no prefix`, with zero `ordinal` records. Thus this shipped import library binds its Foundation imports through encoded export names, not runtime ordinals. The exact tool path, version, command, aggregate counts, and representative `NxCrc32`, `NxMergeSpheres`, and `NxCreateFoundationSDK` records are preserved in [`dumps/link_oracle_import_names.txt`](dumps/link_oracle_import_names.txt). Dumpbin `Hint` values are not described as runtime import ordinals, and no ordinal-drift claim is made here.

### Oracle-backed implementation boundary

- `NxCrc32` at oracle RVA `0x00006230` initializes the accumulator to zero, indexes a reflected CRC table using `(crc ^ byte) & 0xff`, shifts right eight bits, and performs no final XOR. The first table entries at RVA `0x00025090` (`0x00000000`, `0x77073096`, `0xee0e612c`, `0x990951ba`, ...) identify the standard reflected polynomial `0xedb88320`. The minimal bitwise implementation is equivalent; the loader gate proves empty input returns zero and `"123456789"` returns `0x2dfd2d88` in both oracle and rebuild.
- `NxMergeSpheres` at oracle RVA `0x0000a540` forms the center delta, compares squared center distance with squared radius difference, returns the containing sphere when applicable, and otherwise shifts from `sphere0.center` by `(distance + radius1 - radius0) / (2 * distance)` before setting `(distance + radius0 + radius1) / 2`. The disassembly's division guard reads `1.1920928955078125e-07`, matching `NX_EPS_REAL`. The loader gate proves a disjoint unequal-radius case and a containment case against both DLLs.

This is focused export and selected-path proof, not full behavioral equivalence. Exhaustive CRC inputs, NaN/invalid spheres, aliasing, floating-point edge cases, and broader `c_util`/`c_sphere` decisions remain Task 8 work; both cluster decisions therefore remain `pending`.

### Task 6 historical GREEN and then-retained extras

At the end of Task 6, after implementation commit `8b45212` (`foundation: restore CRC and sphere merge exports`), a clean-first Release Win32 build produced `oracle: 134`, `rebuilt: 142`, `missing: 0`, `extra: 8`, exit 0. Those counts describe the Task 6 snapshot and are superseded by the Task 7 `c_sdk` PATCH; `dumps/exports_rebuilt.txt` now records the current 140-name rebuild.

The eight extras present in the Task 6 snapshot were:

- Six `$$QAV` symbols are move construction/assignment members emitted by the modern MSVC toolchain for DLL-exported classes: `Exception` (two), `Observable` (two), `Time` (one), and the immutable public `NxProfiler` (one). Eliminating all six would require changing exported class declarations, including frozen `Foundation/include/NxProfiler.h`, or relying on unsupported compiler/linker ABI suppression.
- Two were `FoundationSDK::getNewID` and `FoundationSDK::freeID`, real private methods emitted by the class-level `NXF_DLL_EXPORT`. Task 6 retained them pending ownership/layout evidence rather than modifying private class exports speculatively.

Task 7 superseded the second bullet after direct layout evidence established that the private ID storage/methods were absent from the oracle unit and had no repository call sites. The `c_sdk` PATCH removed them rather than changing export annotations. The current rebuild therefore has 140 names and six extras, all modern-MSVC move construction/assignment members; required names remain 134/134 with missing 0.

Extras alone are accepted by `compare_exports.py`; no public/private behavior was distorted merely to remove them. No recorder API is exported.

### Task 6 verification facts (historical snapshot)

- Clean/build commands: `cmake --build build --config Release --target clean`, then `cmake --build build --config Release --target NxFoundation`, then `cmake --build build --config Release --target NxFoundationExportTests` from `D:/github/Novodex`; all exited 0. The build retained three pre-existing C4291 warnings in `FoundationSDK.cpp` about placement allocation exception cleanup.
- Behavior command: `build\Release\NxFoundationExportTests.exe build\Release\NxFoundation.dll`; exit 0.
- Output: `D:/github/Novodex/build/Release/NxFoundation.dll`, SHA-256 `229b987b4f3e622282cb2347f6bc94672ed56f06ed7b146786ed02f8ae334a0e`, size 58,368 bytes, COFF machine `0x014c`, PE32 magic `0x10b`, file/product version `2.1.2.6000`.
- Surgical source correction: commit `f6d8db2` restores the unrelated `Utilities.cpp` author-name byte from UTF-8 `c3 b6` to its parent legacy byte `f6`; the parent-to-current implementation diff remains only `NxCrc32` in that file.
- Task 2 tests: `PYTHONDONTWRITEBYTECODE=1 python -m unittest discover -s docs/reconstruction/novodex-foundation/tools/tests -v`; 6 tests passed and no `__pycache__` directory was created.
- Immutable manifest: all 48 raw worktree/oracle hashes still match, and each current `HEAD` public Git blob matches the manifest's `current_blob_sha256`; mismatches 0.

## Provenance and approved fallback

- Oracle: `D:/FlamingEnt__/Unreal_3/Binaries/NxFoundation.dll`
- SHA-256: `7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990`
- ImageBase: `0x10000000`; recorded addresses are preferred RVAs (`VA = 0x10000000 + RVA`).
- Kawaiidra/Ghidra imported and analyzed binary `NxFoundation.dll` in project `novodex_foundation_dropin` (`Import: Success`, `Analysis complete!`). Its `list_exports`, read-only export scripts, and narrower `list_labels` queries reproducibly hung, so Kawaiidra is not claimed as the source of the export mapping.
- The pinned oracle's PE export directory is authoritative for export names, ordinals, and RVAs. PE section characteristics classify executable and data targets.
- Under the user's approved fallback, Capstone 5.0.6 in x86 32-bit mode characterizes executable targets at the PE-provided RVAs. All 101 executable export targets decode; the bounded instruction windows captured 42 direct immediate call/jump targets.
- The remaining 33 exports are data. Their attachment rows include PE section identity and up to 16 file-backed raw bytes/four aligned dwords; virtual-only `.data` storage is explicitly marked unbacked. Exported vftables additionally receive bounded executable-pointer walks.

The raw attachment is deterministic and contains no analysis timestamp. Its tool versions are Python 3.13.9, pefile 2024.8.26, and Capstone 5.0.6. Each `cluster.oracle_refs` row joins to exactly one attachment row by export name and RVA.

## Exact export partition

| Cluster | Exports | Inventory basis |
|---|---:|---|
| `c_sdk` | 22 | FoundationSDK lifecycle, allocator/error surface, globals, and `NxCreateFoundationSDK` |
| `c_exception` | 7 | Exception constructors/assignment/accessors/vftable |
| `c_observable` | 10 | Observable lifecycle/observer operations/event/vftable |
| `c_profiler` | 49 | NxProfiler, DefineZone, SetCurrentZone, profiler state, and SDK profiling-zone creation |
| `c_time` | 6 | Time construction/assignment and clock/elapsed operations |
| `c_fpu` | 11 | FPU control and the three FPU-backed integer conversions |
| `c_box` | 13 | Box construction/query/table helpers |
| `c_capsule` | 2 | `NxComputeBoxAroundCapsule` and `NxComputeCapsuleAroundBox` |
| `c_sphere` | 3 | Sphere compute/fast-compute/merge |
| `c_ray_seg` | 2 | Distance/square-distance helpers |
| `c_volume` | 1 | Volume integrals |
| `c_util` | 5 | Bounds, CRC, inertia diagonalization, rotation matrix, tangents |
| `c_debug` | 3 | DebugRenderable create/release/render methods |
| `c_misc` | 0 | No leftovers |

Total: 134 assigned exports. Recorder APIs present only in public headers were not added.

## Capstone code evidence

For each executable export, `dumps/oracle_pe_capstone.json` records up to eight instructions decoded from a 64-byte window, stopping earlier at a return, trap, or direct unconditional jump. Each instruction includes RVA, bytes, mnemonic, and operands. Immediate call/jump operands include target VAs and in-image target RVAs. This proves that the 101 PE-classified code targets are decodable x86-32 entrypoints; it does not claim recovered function boundaries or behavioral equivalence.

## Bounded structural seeds

The attachment walks only consecutive dwords that point into an executable PE section. A walk stops at the next exported vftable, a non-image/non-executable value, or a 16-entry cap. The counts below describe safely bounded pointer runs, not final semantic slot names or ABI parity.

### FoundationSDK

- Primary exported vftable `??_7FoundationSDK@NxFoundation@@6BNxFoundationSDK@@@`: RVA `0x0001c2b4`, `.rdata`. Ten executable pointers occupy the bounded interval before the next exported vftable at `0x0001c2dc`: `0x00004090`, `0x00002f40`, `0x00002f50`, `0x00002fc0`, `0x00002fd0`, `0x00002f60`, `0x00002f70`, `0x00003f10`, `0x00003bd0`, `0x000034c0`.
- Observable-base exported vftable `??_7FoundationSDK@NxFoundation@@6BObservable@1@@`: RVA `0x0001c2dc`, `.rdata`. One executable pointer, `0x00003fe0`, precedes a non-image dword.
- Both seeds are referenced by `c_sdk`.

### Observable

- Exported vftable `??_7Observable@NxFoundation@@6B@`: RVA `0x0001c2b0`, `.rdata`.
- One executable pointer, `0x000042e0`, occupies the bounded interval before the next exported vftable at `0x0001c2b4`.
- Seed is referenced by `c_observable`.

### Exception

- Exported vftable `??_7Exception@NxFoundation@@6B@`: RVA `0x0001c210`, `.rdata`.
- Three executable pointers, `0x00002d80`, `0x00002d90`, and `0x00002da0`, are followed by a zero dword. These RVAs are also the PE-exported `getErrorCode`, `getFile`, and `getLine` code targets.
- Seed is referenced by `c_exception`.

### DefineZone

- Exported vftable `??_7DefineZone@NxProfiler@@6B@`: RVA `0x0001c288`, `.rdata`.
- Ten executable pointers occupy the bounded interval before the next exported vftable at `0x0001c2b0`: `0x00004ab0`, `0x00004b00`, `0x00004b10`, `0x00003470`, `0x00003480`, `0x00003450`, `0x00003460`, `0x00003490`, `0x000034b0`, `0x00002f20`.
- Seed is referenced by `c_profiler`.

## Status boundary

All clusters remain `pending`. PE+Capstone evidence establishes the export inventory, executable decoding, and bounded data-pointer seeds only. It does not establish complete vtable semantics, object sizes, behavior parity, build parity, or consumer compatibility; those remain later gates.

## Provisional Task 5 PATCH findings

These findings are compile-reconciliation evidence only. They do not change any `inventory.json` decision; Tasks 7-8 retain ownership of formal KEEP/PATCH/REPLACE decisions.

- **`c_sdk`, `c_profiler`, `c_debug` — 2.1.2 allocator macro form:** the transplanted `NxUserAllocator.h` defines function-like `NX_NEW(type)`, while the 2.1.0 private source used the former object-like `NX_NEW type` spelling. The three live allocations were adapted without changing allocation policy. Owning oracle references are `NxCreateFoundationSDK`, `?createProfilingZone@FoundationSDK@NxFoundation@@UAEPAVNxProfilingZone@@PBD@Z`, and `?createDebugRenderable@FoundationSDK@NxFoundation@@UAEPAVNxDebugRenderable@@XZ`.
- **`c_debug` — expanded AABB debug-render contract:** transplanted `NxDebugRenderable` requires `addAABB(const NxBounds3&, NxU32, bool)`. The private `DebugRenderable` override now accepts the color/frame arguments and forwards them to its existing `addOBB` implementation. This is provisionally attached to the `c_debug` create/render lifecycle, with oracle reference `?createDebugRenderable@FoundationSDK@NxFoundation@@UAEPAVNxDebugRenderable@@XZ`; `addAABB` itself is not a named export.
- **`c_util` — bounds overload ownership:** transplanted `NxUtilities.h` implements the `NxBounds3&` convenience overload inline and exports `NxComputeBounds(NxVec3& min, NxVec3& max, ...)`. The old out-of-line body was retargeted to the exported min/max signature while preserving its loop. Owning oracle reference: `NxComputeBounds`.
