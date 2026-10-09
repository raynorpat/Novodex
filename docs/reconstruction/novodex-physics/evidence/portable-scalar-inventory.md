# Portable scalar dependency inventory

Audited revision: `a4838ddf282ee859471e1526145b616de5ddedb6`, 2026-10-09. This is the bootstrap ledger, not a claim that all algorithms or native-platform dependencies are converted. Win32 32-bit x87 remains permanent. No public header, production source, historical digest or geometric epsilon changes in Task 1.

The companion `portable-scalar-dependencies.json` records the whole Physics/Foundation/External source census (comments and strings excluded), including declarations, conditional branches and shadowed vendor sources. Run `python docs/reconstruction/novodex-physics/tools/audit_portable_dependencies.py` from the repository root to reproduce the JSON and ignored `build/task1-evidence/census.md`. The scanner uses regular-expression signatures and balanced braces, not a C++ parser or preprocessor; enclosing names can be unavailable around conditional declarations. Each site includes its original source line to resolve that ambiguity. Candidate headers/branches are included even when not emitted in the current binary; the table count is not a count of live compiled assembly functions. Assembly entries retain instruction families, explicit calls/tail jumps, continuation labels and source references. References include declarations and table initializers; they are not a complete runtime call graph. Precise recovered contracts below govern indirect closure. ABI annotations without a definition are recorded at their exact declaration location. The initial 33 Physics assembly-file matches were not used as a completeness count.

Task 3 update: the companion census above is the immutable Task 1 snapshot. The audit command now writes a fresh `portable-scalar-dependencies.json` and `census.md` under `build/portable-audit`, with actual Git HEAD and dirty state (null identity for unversioned synthetic trees). Use `--output-dir build/task3-audit` or another explicit destination for later evidence; it no longer overwrites the historical ledger by default. `capture_portable_fixtures.py` is restricted to clean Task 1 revision `a4838ddf...` and fails on later revisions before writing. New reconstructed helper captures use `capture_scalar_math.py --reference-revision <actual-HEAD> --capture-id <unique-name> --kind math|rotations|conversions --exporter <Release-Win32-exporter> --output-dir <new-destination>`; existing destinations are refused, source assembly text is normalized and compared against the explicit revision, source hashes/dirty state and compiler/binary identity are recorded. This checks source instructions, not compiled instruction-byte equality. The original shared-math and shipped geometry fixtures remain unchanged.

Task 3 numeric evidence is [scalar-math-acceptance.json](../../../../tests/portable/fixtures/scalar-math-acceptance.json), with fixed controller-approved per-operation units/budgets, MSVC/Clang measurements, input domains, explicit extreme-exponent and hardware-trig dispositions. Additional immutable reconstructed captures are `shared-math-physics-domain-x87.nxpf` (4680 records), `shared-math-rotation-domain-x87.nxpf` (1152 records) and `conversions-x87.nxpf` (100 records); each has a separate identity JSON. Nearest records alone drive scalar comparison; chop records remain diagnostic. Shared helper selection now follows `NX_PHYSICS_USE_X87`, preserving x87 source bodies. `wuFistp255` and `sceneDumpRound` portable adapters deliberately round nearest-even to signed qword then observe signed low32, with invalid qword sentinel low32 zero. The preexisting Wu clamp remains. Foundation portable `NxIntChop/Floor/Ceil` uses checked private int32 conversions; failure returns INT32_MIN (also a valid endpoint), explicitly a new portable convention where the old shifts/overflow are undefined. Internal checked forms preserve the output on failure.

Task 3 remaining obligations: `IceSupportMaps.cpp::nxSupportMapComputeA` converts `(coordinate+1)*halfSubdivision` with live-CW qword stores and observes low32 for lookup indices; the typed routine, cube-axis decisions and integration fixture remain Task 5. `NxMath::trunc` ordinary public casts have no internal Physics/Foundation callers in this source census and remain an invalid-input API/platform audit obligation; no public-header rewrite is included. FPU environment APIs and public `NxSinCos` still block the full Foundation portable closure and belong Tasks 7/9. All full-engine migration guards remain installed; standalone C11/C++ tests do not establish full engine or operating-system support.

Task 3 target-specific extreme-angle disposition: independent 100-digit mathematical sin/cos references at exact `2^62`/`2^63` remain enforced within absolute `2e-16` on x64/future native runners. MSVC Win32 UCRT ordinary out-of-line calls demonstrably have inaccurate range reduction there (volatile function-pointer calls and `/Oi-` give the same errors). Only those four Win32 test-environment probes require finite/bounded sine/cosine, allowing an improved runtime to pass; the current wrong outputs, errors and runtime identity are diagnostic fields in acceptance evidence, never exact correctness expectations. All practical/rotation budgets remain fixed, and production Win32 remains x87.

## Ownership, order and validation route

| Owner / order | Dependencies and strategy | Existing evidence / required fixture obligations |
|---|---|---|
| Task 3 shared math | `include/X87Sqrt.h`: 16 cdecl naked helpers return st(0), qword stack operands; `include/core/JointAcos.h`: jointCIacos, jointAcos float clamp. Retain original instructions; select scalar arithmetic and standard transcendental functions explicitly. | All 18 helpers captured in shared-math-x87.nxpf. Reconstructed helper provenance, never called shipped oracle rows. Task 3 must measure nearest scalar differences in operation output units, document grouping/stores/domain behavior and approve per-operation budgets before acceptance. |
| Task 3 conversion contracts | `Quantizer.cpp`, `core/SceneDump.cpp`, `IceSupportMaps.cpp`, Foundation FPU bit conversions and NxMath conversion paths. `fistp` consumes live rounding; there is no universal truncation policy. Quantizer integer histogram/index versus SceneDump textual integer encoding must remain distinct. | Quantizer/cooking, CoreDump, support-map fixtures required: literal half-integers, int32 extrema, adjacent representable values, nonfinite and range rejection; record current control word and error semantics. Budgets for numeric outputs and exact integer decisions pending owner capture. |
| Task 4 geometry | Geometry, Distance, SmoothNormals, ShapeRaycast, PMap, MassProperties; root/normalization spans may retain extended intermediates across reuse. Naked NxRayTriIntersect loads affect special-value branches. Keep grouping and intentional float stores in ordinary typed source. | Geometry matrix, KernelFuzz, Collision under both CWs. Capture distances (length), squared distances (length²), parameters (unitless), normals (angular), boolean intersections/contact presence. Threshold, parallel and degeneracy reproducers required; no epsilon relaxation. |
| Task 5 support/topology | EdgeList, IceMeshTools/Builder2, ConvexHull, TriangleMeshTopology, TriangleMeshPolygons, IceSupportMaps; explicit tables and this-adjusting receivers. Convert upstream accessors and callbacks before convex callers. | Collision Task2c–2i blocks, triangle mesh and convex cooking contracts. Exact topology/index outcomes, finite vertex scale, projection distance and support axis budgets required. |
| Task 5 complete convex routines | ContactConvexConvex, ContactConvexHeightfield: parameterless naked declarations hide register/stack contracts; whole routines, bridge calls and jump continuations must become typed internal functions. | `units/convex-mesh-gap-contract.md`, `evidence/convex-mesh-gap.md`, Collision registered input/oracle digests. Recover every receiver/register/output/scratch ownership before translation. Full per-routine contracts remain required; source census must not be mistaken for signature recovery. |
| Task 6 contact dispatch | ContactGeneration, NarrowPhase, ContactBoxMeshICE, ContactCapsuleMesh, ContactSphereMesh, ContactMeshHeightfield, ContactMeshMesh, ContactPlaneMesh, ContactPairManager. Matrix A/B indirect selection and tail jumps reach geometry and vendors inside Scene's FPU window. | Collision, MeshSimulation, TriggerSimulation, CCD. Contact set geometry (distance/angle), penetration, exact nonambiguous hit decisions, owner-pair/material/report effects; pin fixtures/budgets before each family. Current Collision pass does not close its explicitly reported unreconstructed entries. |
| Task 7 dynamics/environment | NpActor, ObjectModel, BodyCreation/Step, Island, StepOnlyRows, joint families/JointSupport, effector, SceneDump, Scene::simulate; generated x87 even where there is no assembly token. Portable environment needs nearest and restoration on every exit. | Actor families, BodyCreation, JointStagedPair/Slot/Allocator/Support, Simulation, CCD, Effector/CoreDump. Pose, velocity, angular residual, energy with damping/external work, sleep and error decisions require measured multistep budgets. |
| Tasks 7/9 Foundation | FPU.cpp control-word API and unsafe-range bit conversions, Utilities normalization divide, DebugRenderable trig, Profiler rdtsc, NxMath sqrt and platform/assert/calling-convention headers. Public declarations remain unchanged. | FoundationTangent/SDK/Export and simulation integration. Separate portable environment contract and finite math measurements; timing source is platform work, not a physics tolerance. |
| Task 9 vendor/platform | Effective merged OPCODE and qhull tree: compiler-generated x87 throughout, IceFPU FastSqrt, IceTypes calling conventions, OPC_RayAABBOverlap assembly overlay. Preserve x87 target flags; portable flags must be selected truthfully. | ThirdParty, convex/mesh cooking, Collision and mesh-step integration. Review effective overlay before upstream; host allocation, callbacks and native layout are independent blockers. |

## Recovered ABI, layout and indirect edges

* `TriangleMeshPolygons.h` defines all 12 slots of `gTriangleMeshPolygonTable` (shipped table 0x101085d4). Receiver ECX is the mesh plus four, EDX unused; index/direction/pose/scratch/output pointers are explicit stack arguments. Slots 0–11 are centre, vertex count, vertices, polygon count, polygon, edge axes, edges, edge-to-polygons, edge polygons, support polygon, support face, project. `nxMeshHullProject(iface, edx, scratch, least, greatest, dir, pose, map)` writes two float projections; do not reinterpret scratch bytes as portable serialized state. `nxScratchStamp` mutates the caller's stamp/scratch and is not pure math.
* `IceSupportMaps.h` recovers `IceSupportMap`: 0x14-byte A/B, 0x18-byte C; table at +0, subdiv +4, sample count +8, samples +0xc, hull/second samples +0x10, C source +0x14. Four tables at 0x10107848/5c/6c/90 select deleting destructor, allocate, compute(sample,dir), noop. `nxSupportMapInit` indirectly calls all three non-destructor slots. Cube-face selection uses dominant magnitude bits and ties to lower axis; support lookup/compute boundary classifications must preserve these discrete branches.
* `ConvexHull.h` and `TriangleMeshPolygons.cpp` encode HullPolygon/HullEdge/EdgeDesc fields and raw offsets, not a native portable ABI. `gTriangleMeshPolygonTable` callbacks are called through interface slots by both convex units. `gIceSupportMapFaceCases` jumps into cube-face continuation labels. The census retains these labels and call/jump operands; new typed signatures must identify each continuation's live register/stack inputs before use.
* `ContactConvexConvex.cpp` / `ContactConvexHeightfield.cpp` naked parameterless declarations are insufficient interfaces. Bridges `nxConvexMeshCallInvert`, `CreateEdgeList`, `ObbCollide`, RadixSort constructors/destructors/sort/rank buffers, TriangleArea/Center, UpdateWorldAABB, VertexNormals and `_chkstk` are ABI dependencies. Foundation instance/error slots and interface function pointers require ordinary typed ownership. Exact routine register/alias/temp contracts are **unrecovered obligations**, not inferred void-argument signatures. Contract source: `units/convex-mesh-gap-contract.md`, especially dispatch/continuation supplements and P-Mesh.
* Scene simulation row 000659 (0x13c40) saves CW, installs precision64/chop (0x0f7f), dispatches through shape-pair matrices, then restores; direct API ordinarily sees 0x027f. NxBoxBoxIntersect <- matrix B box/box, NxBuildSmoothNormals <- matrix A box/mesh, NxRayTriIntersect <- matrix A mesh/mesh. Both direct and indirect execution environments matter. OPCODE tail-jump closure is inside this window; qhull cooking is outside it.
* Actor vtable 0x10104530 has 87 public slots plus an actor+8 adjustment thunk, as recovered by `units/npactor-contract.md`; Joint slots, Scene support arrays and body callbacks are recovered by `joint-open-items-contract.md`, `joint-families-contract.md`, `scene-raycast-contract.md`, `actor-mass-contract.md`, `effector-coredump-contract.md`. Historical raw-object tests exercise this Win32 ABI, not a portable fixture representation.

## Generated arithmetic, fallbacks and build dependencies

The root CMake `/arch:IA32` list is a dependency even for assembly-free source: ContactPairManager, StepOnlyRows, Geometry, MassProperties, SmoothNormals, NarrowPhase, ContactGeneration, Distance, PMap, all six listed mesh contact units, EdgeList, InternalTriangleMesh, IceMeshTools/Builder2, ConvexHull, IceSupportMaps, TriangleMeshPolygons, both convex units, ShapeRaycast, NpActor, ObjectModel, Joint, Revolute, Prismatic, Cylindrical, Spherical, PointOnLine, PointInPlane, DistanceJoint, Pulley, Fixed, D6, JointSupport, BodyStep, Island, QhullHost, Quantizer, SpringAndDamperEffector, SceneDump, ActorMass; plus opcode/IcePruner and Foundation Utilities. Existing target-level architecture flags also cover legacy direct-link harnesses. Keep all retained backend flags.

External/CMakeLists applies `/Qfast_transcendentals` unconditionally to **both** NxOpcode and NxQhull, and `/arch:IA32 /GR- /EHs-c-` under MSVC to both. These inline generated x87 transcendental instructions. Portable selection must guard them without weakening legacy behavior; GCC/Clang must not receive these MSVC flags. Vendor overlays are merged through configure_file, so audit the effective build tree as well as upstream sources.

Reviewed scalar fallbacks: X87Sqrt.h uses standard sqrt/sin/cos/acos and grouped double expressions; JointAcos falls back to acos with its existing clamp. They are reference source, not acceptance evidence. NxMath sqrt has platform branches. Foundation FPU's LINUX branch still uses x86 fpu_control operations, so it is not arm64-ready. Naked convex/support/polygon units are unguarded MSVC x86 assembly without complete scalar arms; portable selection must supply real definitions explicitly. `RecoveredRows.cpp::phys_fn_000002` uses int-width addresses and a thiscall callback; it is an ABI/platform dependency, not a math fallback. Test-only contact stubs cannot substitute for a portable production path. Profiler's non-Windows timer branch is separate platform work. OPCODE IceFPU::FastSqrt has no callers in this source census but its unguarded inline assembly still blocks parsing by a non-MSVC compiler; the old SetFPU assembly in that header is commented out and excluded from the census.

## Baseline evidence and next gates

See Task 1 report for commands, hashes and results. Exact staged-pair Geometry, KernelFuzz, ActorMass, Simulation and BodyCreation passed; Collision and JointSupport direct oracle probes passed. These are seven selected legacy targets, not every registered reconstruction gate. Existing conditional mismatches accepted and printed by historical harnesses remain historical classifications, not permission to widen portable budgets. An unavailable or failing family baseline stops that family's conversion until classified.

Shared-math captures are immutable binary64 observations under two recorded CWs. Nearest and simulation-CW records are separate references; portable execution targets nearest and must not treat every chop/extended difference as a regression. Repeat-capture exact budget is zero. Per-operation portable acceptance budgets remain pending Task 3's measurement/review; all other families are explicitly unmeasured in manifest.json. A null budget never authorizes acceptance.

## Source dependency census

57 files, 414 grouped candidate entries after annotation-aware signature matching (including conditional/inactive branches and vendor headers). All 106 groups with naked annotations resolve to named function bodies. Body records retain assembly instruction families, explicit call/jump operands, continuation labels and source references; instructions come from assembly spans, including instructions after same-line labels. Raw-emission bodies are distinguished below.

The JSON explicitly lists 99 unresolved candidates with exact source sites and reasons: 98 ABI declaration/callback/alias sites without a parsed body, and Foundation/src/include/CustomAssert.h:53, a GNU `asm("int $3")` macro in the LINUX debug-assert branch. The macro has no function signature and remains owned by Foundation/platform work; its raw source site preserves the instruction. No naked annotation, MSVC assembly marker or FPU-instruction site remains without a parsed enclosing body. This is source coverage, not compiler reachability or full typed ABI recovery.

Two naked functions use `_emit` directives rather than mnemonic assembly: `ContactBoxMeshICE.cpp::nxBoxMeshTransformTriangle` L184 emits 386 bytes, and `IceSupportMaps.cpp::nxSupportMapNoop` L927 emits one byte (0xc3, the documented ret). Their JSON records retain literal emitted bytes, source references and `raw_byte_decode_required=true`. Empty textual call/jump/label lists for these wrappers do **not** establish an absence of executable dependencies. Task 5 must reconcile the bytes with the authoritative listing and recover instruction/edge/register contracts before translation; the noop byte may be classified as ret only after that reconciliation.

| File | Owner / order | Candidate groups |
|---|---|---|
| `External/opcode/novodex/OPC_RayAABBOverlap.h` | vendor-platform (Task 9; consumers Tasks 5-8) | 1 |
| `External/opcode/upstream/Opcode/Ice/IceFPU.h` | vendor-platform (Task 9; consumers Tasks 5-8) | 1 |
| `External/opcode/upstream/Opcode/Ice/IceTypes.h` | vendor-platform (Task 9; consumers Tasks 5-8) | 1 |
| `Foundation/include/NxMath.h` | foundation-platform (Tasks 7/9) | 2 |
| `Foundation/src/DebugRenderable.cpp` | foundation-platform (Tasks 7/9) | 2 |
| `Foundation/src/FPU.cpp` | foundation-platform (Tasks 7/9) | 9 |
| `Foundation/src/include/CustomAssert.h` | foundation-platform (Tasks 7/9) | 1 |
| `Foundation/src/include/FoundationSDK.h` | foundation-platform (Tasks 7/9) | 1 |
| `Foundation/src/Profiler.cpp` | foundation-platform (Tasks 7/9) | 1 |
| `Foundation/src/Utilities.cpp` | foundation-platform (Tasks 7/9) | 1 |
| `Physics/src/ContactBoxMeshICE.cpp` | contact-dispatch (Task 6) | 8 |
| `Physics/src/ContactCapsuleMesh.cpp` | contact-dispatch (Task 6) | 3 |
| `Physics/src/ContactConvexConvex.cpp` | mesh-support-convex (Task 5) | 9 |
| `Physics/src/ContactConvexHeightfield.cpp` | mesh-support-convex (Task 5) | 16 |
| `Physics/src/ContactGeneration.cpp` | contact-dispatch (Task 6) | 9 |
| `Physics/src/ContactMeshHeightfield.cpp` | contact-dispatch (Task 6) | 7 |
| `Physics/src/ContactMeshMesh.cpp` | contact-dispatch (Task 6) | 3 |
| `Physics/src/ContactPairManager.cpp` | contact-dispatch (Task 6) | 12 |
| `Physics/src/ContactSphereMesh.cpp` | contact-dispatch (Task 6) | 3 |
| `Physics/src/ConvexHull.cpp` | mesh-support-convex (Task 5) | 14 |
| `Physics/src/core/D6Joint.cpp` | dynamics-environment (Task 7) | 2 |
| `Physics/src/core/JointSupport.cpp` | dynamics-environment (Task 7) | 4 |
| `Physics/src/core/RevoluteJoint.cpp` | dynamics-environment (Task 7) | 3 |
| `Physics/src/core/SceneDump.cpp` | dynamics-environment (Task 7) | 1 |
| `Physics/src/core/SphericalJoint.cpp` | dynamics-environment (Task 7) | 5 |
| `Physics/src/Distance.cpp` | geometry-ABI (Tasks 4/9) | 1 |
| `Physics/src/EdgeList.cpp` | mesh-support-convex (Task 5) | 1 |
| `Physics/src/Geometry.cpp` | geometry-ABI (Tasks 4/9) | 3 |
| `Physics/src/IceMeshBuilder2.cpp` | mesh-support-convex (Task 5) | 5 |
| `Physics/src/IceMeshTools.cpp` | mesh-support-convex (Task 5) | 3 |
| `Physics/src/IceSupportMaps.cpp` | mesh-support-convex (Task 5) | 19 |
| `Physics/src/include/ContactGeneration.h` | contact-dispatch (Task 6) | 10 |
| `Physics/src/include/ContactPairManager.h` | contact-dispatch (Task 6) | 8 |
| `Physics/src/include/ConvexHull.h` | mesh-support-convex (Task 5) | 12 |
| `Physics/src/include/core/JointAcos.h` | shared-math (Task 3) | 1 |
| `Physics/src/include/IceMeshBuilder2.h` | mesh-support-convex (Task 5) | 1 |
| `Physics/src/include/IceMeshTools.h` | mesh-support-convex (Task 5) | 2 |
| `Physics/src/include/IceSupportMaps.h` | mesh-support-convex (Task 5) | 18 |
| `Physics/src/include/NxMeshContactHelpers.h` | contact-dispatch (Task 6) | 1 |
| `Physics/src/include/ObjectModel.h` | geometry-ABI (Tasks 4/9) | 10 |
| `Physics/src/include/TriangleMeshPolygons.h` | mesh-support-convex (Task 5) | 13 |
| `Physics/src/include/X87Sqrt.h` | shared-math (Task 3) | 16 |
| `Physics/src/NpActor.cpp` | dynamics-environment (Task 7) | 61 |
| `Physics/src/ObjectModel.cpp` | geometry-ABI (Tasks 4/9) | 54 |
| `Physics/src/opcode/IcePrunable.cpp` | geometry-ABI (Tasks 4/9) | 2 |
| `Physics/src/opcode/IcePruner.cpp` | geometry-ABI (Tasks 4/9) | 1 |
| `Physics/src/PMap.cpp` | geometry-ABI (Tasks 4/9) | 1 |
| `Physics/src/Quantizer.cpp` | conversion (Task 3/5) | 1 |
| `Physics/src/RecoveredRows.cpp` | geometry-ABI (Tasks 4/9) | 1 |
| `Physics/src/Scene.cpp` | dynamics-environment (Task 7) | 25 |
| `Physics/src/SceneRaycast.cpp` | geometry-ABI (Tasks 4/9) | 1 |
| `Physics/src/SceneVisualize.cpp` | geometry-ABI (Tasks 4/9) | 1 |
| `Physics/src/ShapeRaycast.cpp` | geometry-ABI (Tasks 4/9) | 4 |
| `Physics/src/SmoothNormals.cpp` | geometry-ABI (Tasks 4/9) | 3 |
| `Physics/src/TriangleMesh.cpp` | geometry-ABI (Tasks 4/9) | 2 |
| `Physics/src/TriangleMeshPolygons.cpp` | mesh-support-convex (Task 5) | 13 |
| `Physics/src/TriangleMeshTopology.cpp` | geometry-ABI (Tasks 4/9) | 1 |
