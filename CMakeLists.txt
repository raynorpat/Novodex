cmake_minimum_required(VERSION 3.16...3.31)

project(Novodex VERSION 2.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
enable_testing()

# Graphics is reconstructed separately from the shipped VC6 archive.
if(WIN32)
	add_subdirectory(Graphics)
endif()

set(NOVODEX_VIEWER_DEFAULT OFF)
if(WIN32 AND MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(NOVODEX_VIEWER_DEFAULT ON)
endif()
option(NOVODEX_BUILD_VIEWER "Build the legacy Windows viewer" ${NOVODEX_VIEWER_DEFAULT})

#### FOUNDATION ####

file(GLOB_RECURSE FOUNDATION_HEADERS Foundation/include/*.h Foundation/src/include/*.h)
source_group("Header Files" FILES ${FOUNDATION_HEADERS})

file(GLOB FOUNDATION_SOURCES Foundation/src/*.cpp)
source_group("Source Files" FILES ${FOUNDATION_SOURCES})

if(WIN32)
	file(GLOB FOUNDATION_RESOURCES Foundation/src/win32/*.rc Foundation/src/win32/*.h)
	source_group("Resource Files" FILES ${FOUNDATION_RESOURCES})
endif()

add_library(NxFoundation SHARED ${FOUNDATION_HEADERS} ${FOUNDATION_SOURCES} ${FOUNDATION_RESOURCES})
set_target_properties(NxFoundation PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(NxFoundation PRIVATE 
	Foundation/include
	Foundation/src/include
)

target_compile_definitions(NxFoundation PRIVATE NX_FOUNDATION_DLL=1 FOUNDATIONDLL_EXPORTS=1)

# The Foundation's copy of the x87 rule the NxPhysics block below records.
# NxNormalToTangents is x87 in the oracle, and typing its register lifetimes
# `double` reproduces every finite word NxFoundationTangentTests compares; what
# it cannot reproduce is which NaN survives an operation with two NaN operands.
# Measured: with the default architecture ten of that differential's cases --
# the zero axis, the infinite and NaN axes, the two-payload inputs -- came out
# with the other NaN sign or payload, and with this flag none did.
if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
	set_source_files_properties(
		Foundation/src/Utilities.cpp
		PROPERTIES COMPILE_OPTIONS "/arch:IA32")
endif()

if(WIN32)
	add_executable(NxFoundationExportTests tests/FoundationExportTests.cpp)
	target_include_directories(NxFoundationExportTests PRIVATE Foundation/include)
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		add_executable(NxFoundationSDKTests tests/FoundationSDKTests.cpp)
		target_include_directories(NxFoundationSDKTests PRIVATE Foundation/include)
		add_executable(NxFoundationClusterTests tests/FoundationClusterTests.cpp)
		target_include_directories(NxFoundationClusterTests PRIVATE Foundation/include)
		add_executable(NxFoundationCustomArrayTests tests/FoundationCustomArrayTests.cpp Foundation/src/CustomArray.cpp)
		target_include_directories(NxFoundationCustomArrayTests PRIVATE Foundation/include Foundation/src)
	endif()
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	if(WIN32)
		target_compile_definitions(NxFoundation PRIVATE WIN64=1)
	endif()
	target_compile_definitions(NxFoundation PRIVATE NX64=1)
endif()

#### THIRD PARTY ####

# qhull 2003.1 and OPCODE 1.3 ship inside NxPhysics.dll: 722 census rows and
# 377,842 bytes, 91% of Phase 4's mapped bytes. They are vendored, not
# reconstructed. External/README.md carries the layout rule and the licences.
add_subdirectory(External)

#### PHYSICS ####

file(GLOB_RECURSE PHYSICS_HEADERS Physics/include/*.h Physics/src/include/*.h Physics/src/opcode/*.h)
source_group("Header Files" FILES ${PHYSICS_HEADERS})

# Physics/src/opcode/ is NovodeX's own directory name, not a second copy of
# OPCODE. The oracle's assert literals say
# `\Epic\Novodex\SDKs\Physics\src\opcode\IcePrunable.cpp`, and nothing under
# this directory has a counterpart in either pinned OPCODE 1.3 tree. Vendored
# OPCODE is External/opcode/ and nowhere else.
file(GLOB PHYSICS_SOURCES Physics/src/*.cpp Physics/src/core/*.cpp Physics/src/opcode/*.cpp)
# The joint families on their own, for the test targets that link Scene.cpp: its
# createJoint constructs every family, so they need the whole directory at link time.
file(GLOB PHYSICS_CORE_SOURCES Physics/src/core/*.cpp)
source_group("Source Files" FILES ${PHYSICS_SOURCES})

if(WIN32)
	file(GLOB PHYSICS_RESOURCES Physics/src/win32/*.rc Physics/src/win32/*.h)
	source_group("Resource Files" FILES ${PHYSICS_RESOURCES})
endif()

add_library(NxPhysics SHARED ${PHYSICS_HEADERS} ${PHYSICS_SOURCES} ${PHYSICS_RESOURCES})
# Emit a linker map for the rebuilt module, the same way the layout harness publishes its own. The
# harnesses that drive this library bind ORACLE RVAs by addition -- `base + kSomeRva` -- which is
# correct only when the loaded module IS the shipped one. Reading the candidate's own symbol names is
# what lets a target resolve by name instead, and the map is where they are written.
if(MSVC)
	# Keep unreferenced COMDATs. The default /OPT:REF drops any function nothing references, and the
# reconstruction contains rows whose code exists and is discarded for exactly that reason --
# measured in round 67 as 150 rows whose implementation object contributes no function.
target_link_options(NxPhysics PRIVATE /MAP /OPT:NOREF /OPT:NOICF)
	# Retain the embedded vendor archives while their callers are being
	# reconstructed. SweepAndPrune::Init's one missing stock-source dependency
	# is provided by the recovered NovodeX pruning helper at 0x000b4530.
	target_link_options(NxPhysics PRIVATE
		"/WHOLEARCHIVE:$<TARGET_FILE:NxOpcode>"
		"/WHOLEARCHIVE:$<TARGET_FILE:NxQhull>")
endif()
target_link_libraries(NxPhysics NxFoundation NxQhull NxOpcode)
# UCRT's printf rounds %f at the live x87 rounding mode (VS2019 16.2+); the
# oracle's 2003 static CRT always rounds its decimal digits to nearest in
# software. D6's dump rows (004188-004192) print from inside the simulation
# step's word 0x0f7f (round toward zero), where the two gave different last
# digits (joint-open-items Task 6). The linker option object restores the
# legacy rounding for NxPhysics.dll's own printf calls only.
if(MSVC)
	target_link_options(NxPhysics PRIVATE legacy_stdio_float_rounding.obj)
endif()
set_target_properties(NxPhysics PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(NxPhysics PRIVATE
	Foundation/include
	Foundation/src/include
	Physics/include
	Physics/src/include
	Physics/src/opcode
)

target_compile_definitions(NxPhysics PRIVATE NX_PHYSICS_DLL=1)

# The mass and geometry kernels are x87 in the oracle and their results depend
# on it. Two differences are measured, not assumed: the oracle keeps
# intermediates at the process's _PC_53 precision instead of rounding each one
# to 32 bits, and when an operation has two NaN operands x87 propagates the one
# with the larger significand where SSE propagates the first source. The C++
# expresses the first by typing register lifetimes `double`; only the second
# needs the code generator, and only on these translation units. Everything
# else in NxPhysics keeps the default architecture. The joint translation units
# (core/Joint.cpp, core/RevoluteJoint.cpp, core/PrismaticJoint.cpp,
# core/CylindricalJoint.cpp, core/SphericalJoint.cpp, core/PointOnLineJoint.cpp,
# core/PointInPlaneJoint.cpp, core/DistanceJoint.cpp, core/PulleyJoint.cpp,
# core/FixedJoint.cpp, core/D6Joint.cpp, core/JointSupport.cpp)
# are x87 in the oracle too and are reached from the solver step, so they are
# on the list for the same reason. core/JointSupport.cpp's rows 004389 and 004393 are x87 in the
# oracle and are called from the solver slots 004360 and 004362 inside the step.
# BodyStep.cpp and Island.cpp (scene-raycast Task 4) hold the body record's
# step rows -- the velocity integration, damping and clamp, the wake-counter
# update, the quaternion integrator (fsin/fcos/fsqrt), the CCD pose and swept
# box, and the island/sleep-group unions -- which are x87 in the oracle and
# run only inside the simulation step (000655's phases) or its CCD sweep
# (002264), under the step's control word 0x0f7f; they are on the list so
# the code generator keeps their register values and NaN propagation x87.
# ContactPairManager.cpp's rows (000750..000921, the contact and friction
# constraint rows, the friction patches and the contact reports) are x87 in the
# oracle and run from the simulation step (002400 -> 000659 -> 000655) under its
# control word 0x0f7f.
# Physics/src/StepOnlyRows.cpp holds 000867 (the kind accumulator, called only
# from the pair constructor 000893) and 000951 (BOX slot 7, called only from the
# CCD sweep 002264), with 000951's slab helper 001730/001732; they are x87 in
# the oracle and run only inside the simulation step under its control word
# 0x0f7f (64-bit, round toward zero), so the file is built x87 like the others.
# NpActor.cpp, the public actor API, is here for the same reason: its float
# rows (the x87 row sums of 000150/000152, the setters and getters) are x87 in
# the oracle, and its `double` register lifetimes must compile to x87 at the
# process's _PC_53 rather than to SSE2 (NpActor.cpp completion Task 2).
# ObjectModel.cpp is reached from the simulation step too: body pose row
# 000022 dispatches slot 6 on the owned shape after updating the body record.
# 001315 composes compound child poses in the step's x87 precision-64,
# round-toward-zero window; SSE2 double arithmetic leaves the cached poses
# one or more ulps away from the oracle.
# QhullHost.cpp is NovodeX's hull library around qhull (qhull-gap Task 4): x87 in the oracle
# (the box fallback, the OBJ writers' widening, the rescale) and run under the caller's word.
# Quantizer.cpp is the same library's band B (Wu's quantizer and reduceVertices, piece 4d), x87 too.
# PMap.cpp's point-to-triangle query is the x87 OPCODE row 005324 (0x000e7c50):
# the oracle compares its extended result against a float best distance. SSE2
# rounded the near-tied face distances before that comparison, changing 61 face
# labels in the exact-byte PMap fixture.
# core/SpringAndDamperEffector.cpp (effector-and-coredump Task 2) does x87
# work in 003962/003964/003970/003972 and its solver slot 003979 runs inside
# the step; core/NpSpringAndDamperEffector.cpp does none and stays default.
# core/SceneDump.cpp (effector-and-coredump Task 3) is the core dump: its
# float tokens compare and widen x87 values and round with fistp at the live
# control word, and its asset writer converts poses to quaternions with fsqrt.
# core/ActorMass.cpp (actor-mass Task 1) is the actor's mass from shapes,
# phys_fn_000008: x87 compares, scaling and the NxDiagonalizeInertiaTensor call.
if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
	set_source_files_properties(
		Physics/src/ContactPairManager.cpp
		Physics/src/StepOnlyRows.cpp
		Physics/src/Geometry.cpp
		Physics/src/MassProperties.cpp
		Physics/src/SmoothNormals.cpp
		Physics/src/NarrowPhase.cpp
		Physics/src/ContactGeneration.cpp
		Physics/src/Distance.cpp
		Physics/src/PMap.cpp
		Physics/src/ContactBoxMeshICE.cpp
		Physics/src/ContactCapsuleMesh.cpp
		Physics/src/ContactSphereMesh.cpp
		Physics/src/ContactMeshHeightfield.cpp
		Physics/src/ContactMeshMesh.cpp
		Physics/src/ContactPlaneMesh.cpp
		Physics/src/EdgeList.cpp
		Physics/src/InternalTriangleMesh.cpp
		Physics/src/IceMeshTools.cpp
		Physics/src/IceMeshBuilder2.cpp
		Physics/src/ConvexHull.cpp
		Physics/src/IceSupportMaps.cpp
		Physics/src/TriangleMeshPolygons.cpp
		Physics/src/ContactConvexConvex.cpp
		Physics/src/ContactConvexHeightfield.cpp
		Physics/src/ShapeRaycast.cpp
		Physics/src/NpActor.cpp
		Physics/src/ObjectModel.cpp
		Physics/src/core/Joint.cpp
		Physics/src/core/RevoluteJoint.cpp
		Physics/src/core/PrismaticJoint.cpp
		Physics/src/core/CylindricalJoint.cpp
		Physics/src/core/SphericalJoint.cpp
		Physics/src/core/PointOnLineJoint.cpp
		Physics/src/core/PointInPlaneJoint.cpp
		Physics/src/core/DistanceJoint.cpp
		Physics/src/core/PulleyJoint.cpp
		Physics/src/core/FixedJoint.cpp
		Physics/src/core/D6Joint.cpp
		Physics/src/core/JointSupport.cpp
		Physics/src/BodyStep.cpp
		Physics/src/Island.cpp
		Physics/src/QhullHost.cpp
		Physics/src/Quantizer.cpp
		Physics/src/core/SpringAndDamperEffector.cpp
		Physics/src/core/SceneDump.cpp
		Physics/src/core/ActorMass.cpp
		PROPERTIES COMPILE_OPTIONS "/arch:IA32")
	# opcode/IcePruner.cpp builds OPCODE's AABBTreeOfAABBsBuilder on the stack
	# (the static pruner's tree build, 005216), so it emits COMDAT copies of
	# the builder's inline virtuals and its table, and the linker keeps the
	# first copy it sees -- this file's, ahead of NxOpcode's. Built SSE2, its
	# AABBTreeBuilder::GetSplittingValue (002150) rounded the centre to float
	# where the x87 copy returns it unrounded. It is built as NxOpcode is
	# (/arch:IA32 /GR-) so the kept copies are OPCODE's code (scene-raycast
	# Task 3 review). Its own rows run at API time under 0x027f, where the x87
	# at 53 bits is what the listing does.
	set_property(SOURCE Physics/src/opcode/IcePruner.cpp
		APPEND PROPERTY COMPILE_OPTIONS "/arch:IA32" "/GR-")
endif()

# The joint classes are built without C++ exception handling, as the oracle
# is. Measured: the oracle's .text has no __CxxFrameHandler and its only fs:[0]
# uses are in CRT rows (0xf4000+); every joint constructor and deleting
# destructor in it is frameless (e.g. 004382, 004729). Under CMake's default
# /EHsc each of these translation units gave its constructors an unwind
# frame for the constructed bases, and its deleting destructors -- noexcept
# since C++11 -- a terminate state around the allocator calls the inlined class
# operator delete makes; /GS then put a cookie on every such frame. /EHs-c-
# removes the frames, and with them the cookies. It does not touch /GS: rows
# with a local buffer (D6Joint's row_slot6/row_slot8, Joint::setGlobalAnchor/
# setGlobalAxis) keep theirs. No joint file uses try, catch or throw.
if(MSVC)
	set_property(SOURCE
		Physics/src/core/Joint.cpp
		Physics/src/core/RevoluteJoint.cpp
		Physics/src/core/PrismaticJoint.cpp
		Physics/src/core/CylindricalJoint.cpp
		Physics/src/core/SphericalJoint.cpp
		Physics/src/core/PointOnLineJoint.cpp
		Physics/src/core/PointInPlaneJoint.cpp
		Physics/src/core/DistanceJoint.cpp
		Physics/src/core/PulleyJoint.cpp
		Physics/src/core/FixedJoint.cpp
		Physics/src/core/D6Joint.cpp
		Physics/src/core/NpJointShared.cpp
		Physics/src/core/NpRevoluteJoint.cpp
		Physics/src/core/NpPrismaticJoint.cpp
		Physics/src/core/NpCylindricalJoint.cpp
		Physics/src/core/NpSphericalJoint.cpp
		Physics/src/core/NpPointOnLineJoint.cpp
		Physics/src/core/NpPointInPlaneJoint.cpp
		Physics/src/core/NpDistanceJoint.cpp
		Physics/src/core/NpPulleyJoint.cpp
		Physics/src/core/NpFixedJoint.cpp
		Physics/src/core/NpD6Joint.cpp
		Physics/src/QhullHost.cpp
		Physics/src/Quantizer.cpp
		Physics/src/core/SpringAndDamperEffector.cpp
		Physics/src/core/NpSpringAndDamperEffector.cpp
		Physics/src/core/SceneDump.cpp
		APPEND PROPERTY COMPILE_OPTIONS "/EHs-c-")
	# The scene raycasts and the pruners (scene-raycast block Task 3), for the
	# same reason: the static pruner's tree build (0x000e5110) is frameless in
	# the oracle, where /EHsc gave its `new AABBTree` an unwind frame. None of
	# these files uses try, catch or throw. BodyCreation.cpp (Task 4): the
	# record destructor 000776 (0x18570) is frameless, where /EHsc framed the
	# Observable's destructor call.
	set_property(SOURCE
		Physics/src/SceneRaycast.cpp
		Physics/src/BodyCreation.cpp
		Physics/src/opcode/IcePruner.cpp
		Physics/src/opcode/IcePruningEngine.cpp
		APPEND PROPERTY COMPILE_OPTIONS "/EHs-c-")
endif()

# The same for the ICE-shaped mesh rows (convex-mesh gap Task 2c). Their
# listings are frameless (0x00051080, 0x0002dcf0, 0x0002def0, 0x00032610 set up
# no fs:[0] record); under /EHsc the RadixSort and EdgeList locals and the
# noexcept destructors gave CreateFacesToEdges, CreateDatabase, Adjacencies::
# Init, Valencies::Compute and the three destructors unwind frames the oracle
# lacks. None of the three files uses try, catch or throw. Task 2d adds
# IceMeshBuilder2.cpp for the same reason (0x0002f290, 0x0002fc50, 0x00030620,
# 0x00030d90 are frameless with their Container, reducer and RadixSort locals).
# Task 2e adds ConvexHull.cpp and TriangleMeshTopology.cpp (0x0002ae60,
# 0x000543d0 and 0x00054460 are frameless around their MeshNormals local and
# the objects they construct with placement new). Task 2f adds IceSupportMaps.cpp
# (the support-map rows are frameless; its C++ rows hold no object with a
# destructor, so the flag only keeps the file with its neighbours). Task 2g adds
# TriangleMeshPolygons.cpp and ContactConvexConvex.cpp, whose rows are all naked
# (the flag keeps them with their neighbours). Task 2h adds
# ContactConvexHeightfield.cpp, whose rows are all naked too.
if(MSVC)
	set_property(SOURCE
		Physics/src/EdgeList.cpp
		Physics/src/IceAdjacencies.cpp
		Physics/src/IceMeshTools.cpp
		Physics/src/IceMeshBuilder2.cpp
		Physics/src/ConvexHull.cpp
		Physics/src/TriangleMeshTopology.cpp
		Physics/src/IceSupportMaps.cpp
		Physics/src/TriangleMeshPolygons.cpp
		Physics/src/ContactConvexConvex.cpp
		Physics/src/ContactConvexHeightfield.cpp
		APPEND PROPERTY COMPILE_OPTIONS "/EHs-c-")
endif()

# These geometry and contact files are on that list because the x87 control word is not the same
# everywhere this code runs, and the flag is what lets a translation unit follow
# it at all.
#
# phys_fn_000659 at 0x00013c40 -- the simulation step -- calls
# NxSetFPURoundingChop and then NxSetFPUPrecision64 on its way in and restores
# the caller's word with `fldcw` on its way out. Everything it reaches runs at
# 64-bit precision with round-toward-zero; everything called straight out of a
# consumer runs at the CRT default 0x027f, 53-bit and round to nearest.
#
# Which files are inside that window is a question the recovered shape-pair
# dispatch matrix answers and nothing before it could. The step reaches the
# matrix through a function pointer, so no direct call edge crosses it, and
# these three exports looked like consumer-facing leaves until the matrix was
# read out of the oracle's own constructor:
#
#   NxBoxBoxIntersect     <- 0x00038a82 in phys_fn_001738, matrix B [BOX][BOX]
#   NxBuildSmoothNormals  <- 0x00052271 <- 0x0003c344 <- 0x0003d87c in
#                            phys_fn_001772, matrix A [BOX][MESH]
#   NxRayTriIntersect     <- 0x00036e4b <- 0x000415a2 <- 0x00042ea2
#                         <- 0x0004366c <- 0x00047067 in phys_fn_001876,
#                            matrix A [MESH][MESH]
#
# Every edge there is a direct `call rel32`. So Geometry.cpp and
# SmoothNormals.cpp are inside the window as well as outside it, and an earlier
# version of this comment calling all three of them consumer-facing was wrong.
# MassProperties.cpp is reached from Actor.cpp and is outside it.
#
# No C++ type names a 64-bit x87 significand: MSVC's long double is double. The
# precision therefore cannot be written into the source at all -- it is process
# state, and x87 code picks up whatever the control word says, which is exactly
# how the oracle gets it. An SSE2 build ignores the control word outright and
# can never follow it. So the flag is not an optimisation of the last bit; it is
# the only way any of these files can be right inside the step.
#
# NxPhysicsCollisionTests drives every kernel under both control words, and it
# is the reason this comment could be corrected. It also found the limit of the
# approach: NxBuildSmoothNormals agrees under 0x027f and differs on 24 of
# 459,676 checks under 0x0f7f, because under 64-bit precision every `double`
# the compiler spills truncates and nothing in C++ says where MSVC spills.
#
# NOTE FOR WHOEVER ADDS THE NEXT KERNEL TRANSLATION UNIT. This rule names four
# files, not a directory, so anything else added under Physics/src builds SSE2
# by default and will silently disagree with the oracle on NaN payloads -- and,
# if it is written with NxReal temporaries rather than double register
# lifetimes, on the last bit of ordinary results too. The remaining collision
# units (BroadPhase.cpp, ContactGeneration.cpp, Filtering.cpp) are on the same
# simulation-step path as NarrowPhase.cpp and will need the flag for the same
# reason. The check is cheap -- pick an input where the two models differ in the
# last bit and compare, the way the witnesses in
# tests/PhysicsKernelFuzzTests.cpp do.
#
# AND CHECK IT OVER THE RIGHT DOMAIN. SmoothNormals.cpp was added to this list
# late, because the first measurement said it did not need the flag. That
# measurement was wrong in a way worth repeating here: the normals fuzz block
# generated only finite vertices, and all 17 matrix cases are finite too, so
# nothing had ever put a NaN into the export. NaN payload propagation is exactly
# what the flag exists for, so the domain that was measured was the one domain
# where the flag cannot matter. Fed the same raw-bit mixture every other kernel
# gets, the SSE2 build differs from the oracle on the first NaN that reaches a
# two-NaN operation: oracle ffcad8b2, SSE2 ffc00000. A test that cannot fail is
# worth nothing whichever side of the build it sits on.

if(WIN32 AND CMAKE_SIZEOF_VOID_P EQUAL 4)
	add_executable(NxPhysicsExportTests tests/PhysicsExportTests.cpp)
	target_link_libraries(NxPhysicsExportTests bcrypt)
	add_executable(NxPhysicsSDKTests tests/PhysicsSDKTests.cpp)
	target_include_directories(NxPhysicsSDKTests PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxPhysicsSDKTests bcrypt)
	# The Phase 6 joint differential. A separate translation unit on purpose:
	# PhysicsObjectLayoutTests.cpp's frame is ~250 KB and a 0xA5 poisoned jump
	# follows any change to it (evidence/phase5-object-model.md 3z262-3z289), so
	# the joint cases go in a file with no such history rather than being appended
	# to that one. It loads the pair through PhysicsPairLoader.h and drives the
	# exported SDK over the C++ interface, like NxPhysicsSDKTests does.
	add_executable(NxPhysicsJointTests tests/PhysicsJointTests.cpp)
	# The same harness as its own STAGED-PAIR target: the closure schema needs a target
	# that loads the rebuilt module, and an oracle differential runs only against the
	# pinned DLL (evidence 11l). Same source, distinct target name and class.
	add_executable(NxPhysicsJointStagedPairTests tests/PhysicsJointTests.cpp)
	add_executable(NxPhysicsActorLifecycleTests tests/PhysicsActorLifecycleTests.cpp)
	target_include_directories(NxPhysicsActorLifecycleTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsActorLifecycleTests bcrypt)
	# A second process starts with a dynamic box, so OPCODE's shared first-use
	# allocations are measured independently of the static-first lifecycle drive.
	add_executable(NxPhysicsDynamicFirstTests tests/PhysicsActorLifecycleTests.cpp)
	target_compile_definitions(NxPhysicsDynamicFirstTests PRIVATE
		NX_PHYSICS_DYNAMIC_FIRST_ONLY)
	target_include_directories(NxPhysicsDynamicFirstTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsDynamicFirstTests bcrypt)
	# Fresh-process empty Scene creation and release pins the wrapper's ownership
	# order independently of actor teardown, which remains a separate phase-5 path.
	add_executable(NxPhysicsEmptySceneTests tests/PhysicsActorLifecycleTests.cpp)
	target_compile_definitions(NxPhysicsEmptySceneTests PRIVATE
		NX_PHYSICS_EMPTY_SCENE_ONLY)
	target_include_directories(NxPhysicsEmptySceneTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsEmptySceneTests bcrypt)
	add_executable(NxPhysicsSceneConstructorTests tests/PhysicsSceneConstructorTests.cpp)
	target_include_directories(NxPhysicsSceneConstructorTests PRIVATE
		Foundation/include Physics/include)
	target_link_libraries(NxPhysicsSceneConstructorTests bcrypt)
	add_executable(NxPhysicsControllerSweepFaceTests tests/PhysicsControllerSweepFaceTests.cpp)
	target_include_directories(NxPhysicsControllerSweepFaceTests PRIVATE
		Foundation/include Physics/include)
	target_link_libraries(NxPhysicsControllerSweepFaceTests bcrypt)
	# A fresh process isolates actor/shape name-map behavior from the populated
	# lifecycle fixture's intentionally stateful teardown sequence.
	# Foundation created with one counting allocator, NxCreatePhysicsSDK given a
	# second, so each Scene/actor/shape block reports which allocator the DLL
	# used: nxFoundationSDKAllocator or the phys_fn_004803 bridge.
	add_executable(NxPhysicsSceneAllocatorTests tests/PhysicsSceneAllocatorTests.cpp)
	target_include_directories(NxPhysicsSceneAllocatorTests PRIVATE
		Foundation/include Physics/include)
	target_link_libraries(NxPhysicsSceneAllocatorTests bcrypt)
	add_executable(NxPhysicsPopulatedSceneTeardownTests tests/PhysicsPopulatedSceneTeardownTests.cpp)
	target_include_directories(NxPhysicsPopulatedSceneTeardownTests PRIVATE
		Foundation/include Physics/include)
	target_link_libraries(NxPhysicsPopulatedSceneTeardownTests bcrypt)
	add_executable(NxPhysicsActorNameTests tests/PhysicsActorNameTests.cpp)
	target_include_directories(NxPhysicsActorNameTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsActorNameTests bcrypt)
	add_executable(NxPhysicsActorMetadataTests tests/PhysicsActorMetadataTests.cpp)
	target_include_directories(NxPhysicsActorMetadataTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsActorMetadataTests bcrypt)
	add_executable(NxPhysicsActorBodyFlagTests tests/PhysicsActorBodyFlagTests.cpp)
	target_include_directories(NxPhysicsActorBodyFlagTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsActorBodyFlagTests bcrypt)
	add_executable(NxPhysicsActorDynamicsTests tests/PhysicsActorDynamicsTests.cpp)
	target_include_directories(NxPhysicsActorDynamicsTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsActorDynamicsTests bcrypt)
	add_executable(NxPhysicsActorDynamicSetterTests tests/PhysicsActorDynamicSetterTests.cpp)
	target_include_directories(NxPhysicsActorDynamicSetterTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsActorDynamicSetterTests bcrypt)
	add_executable(NxPhysicsActorShapeMutationTests tests/PhysicsActorShapeMutationTests.cpp)
	target_include_directories(NxPhysicsActorShapeMutationTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsActorShapeMutationTests bcrypt)
	add_executable(NxPhysicsActorReleaseShapeProbeTests tests/PhysicsActorReleaseShapeProbeTests.cpp)
	target_include_directories(NxPhysicsActorReleaseShapeProbeTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsActorReleaseShapeProbeTests bcrypt)
	# Public convex-mesh cooking regression. This is the front door for the
	# TriangleMesh reconstruction packet and must pass before the Unreal KActor
	# convex-element smoke can pass.
	add_executable(NxPhysicsTriangleMeshApiTests tests/PhysicsTriangleMeshApiTests.cpp)
	target_include_directories(NxPhysicsTriangleMeshApiTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsTriangleMeshApiTests bcrypt)
	add_executable(NxPhysicsPMapResolution80Tests tests/PhysicsTriangleMeshApiTests.cpp)
	target_include_directories(NxPhysicsPMapResolution80Tests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_compile_definitions(NxPhysicsPMapResolution80Tests PRIVATE NX_PMAP_COMPUTE_DENSITY=80)
	target_link_libraries(NxPhysicsPMapResolution80Tests bcrypt)
	add_executable(NxPhysicsActorMomentumTests tests/PhysicsActorMomentumTests.cpp)
	target_include_directories(NxPhysicsActorMomentumTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsActorMomentumTests bcrypt)
	add_executable(NxPhysicsActorForceTests tests/PhysicsActorForceTests.cpp)
	target_include_directories(NxPhysicsActorForceTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsActorForceTests bcrypt)
	add_executable(NxPhysicsActorCMassTests tests/PhysicsActorCMassTests.cpp)
	target_include_directories(NxPhysicsActorCMassTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsActorCMassTests bcrypt)
	# Density-based actor creation exercises phys_fn_000008 across primitive,
	# posed and compound shapes, including the trigger-only refusal.
	add_executable(NxPhysicsActorMassTests tests/PhysicsActorMassTests.cpp)
	target_include_directories(NxPhysicsActorMassTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsActorMassTests bcrypt)
	# The dynamic body record's constructor and destructor rows (scene-raycast
	# block Task 4, body-creation): the record words the constructor chain
	# writes for default, live-SDK-parameter, explicit, zero-element-tensor and
	# kinematic body descriptors, the record id pool, and the release traffic.
	add_executable(NxPhysicsBodyCreationTests tests/PhysicsBodyCreationTests.cpp)
	target_include_directories(NxPhysicsBodyCreationTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsBodyCreationTests bcrypt)
	# The same include set as the oracle target above; the harness includes the
	# Foundation's allocator interface for the page-guarded allocator, and Physics'
	# public headers for the descriptors.
	target_include_directories(NxPhysicsJointStagedPairTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsJointStagedPairTests bcrypt)
	# A fresh process that creates the Foundation with one allocator before the
	# Physics SDK with another, so the joint rows' allocator (the Foundation's,
	# `[[0x101041bc]]`) is observable (joint-open-items Task 5 follow-up).
	add_executable(NxPhysicsJointAllocatorTests tests/PhysicsJointTests.cpp)
	target_compile_definitions(NxPhysicsJointAllocatorTests PRIVATE
		NX_PHYSICS_JOINT_ALLOCATOR_ONLY)
	target_include_directories(NxPhysicsJointAllocatorTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsJointAllocatorTests bcrypt)
	# The internal-slot joint differential (joint-open-items Task 6): each
	# family's internal visualization, step and projection slots called through
	# the internal object's own table by slot index, under 0x027f and 0x0f7f.
	add_executable(NxPhysicsJointSlotTests tests/PhysicsJointSlotTests.cpp)
	target_include_directories(NxPhysicsJointSlotTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsJointSlotTests bcrypt)
	# The scene raycast differential (scene-raycast block, Task 3): every public
	# NxScene raycast over static and dynamic box, sphere, capsule and plane
	# shapes, with the NxUserRaycastReport callbacks printed as words.
	add_executable(NxPhysicsSceneRaycastTests tests/PhysicsSceneRaycastTests.cpp)
	target_include_directories(NxPhysicsSceneRaycastTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsSceneRaycastTests bcrypt)
	# The scene visualisation differential (scene-raycast block, Task 4,
	# visualisation sub-area; Phase 7): NxScene::visualize over static and
	# dynamic actors, with what the NxUserDebugRenderer receives from
	# NxPhysicsSDK::visualize printed as words.
	add_executable(NxPhysicsSceneVisualizeTests tests/PhysicsSceneVisualizeTests.cpp)
	target_include_directories(NxPhysicsSceneVisualizeTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsSceneVisualizeTests bcrypt)
	# The spring-and-damper effector differential (effector-and-coredump Task
	# 2): NxScene's effector API, every NxSpringAndDamperEffector method, the
	# internal effector's slots 2 and 3 by index, the record teardown notify
	# and the scene release of a live effector, with each step's SDK
	# allocations and frees.
	add_executable(NxPhysicsEffectorTests tests/PhysicsEffectorTests.cpp)
	target_include_directories(NxPhysicsEffectorTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsEffectorTests bcrypt)
	# The scene core-dump differential (effector-and-coredump Task 4): two
	# populated scenes dumped through NxPhysicsSDK::coreDump in text and binary
	# mode, with and without an addendum, each .psc file printed back; then a
	# third scene (Task 5 review coverage) dumped on its own in text and binary.
	add_executable(NxPhysicsCoreDumpTests tests/PhysicsCoreDumpTests.cpp)
	target_include_directories(NxPhysicsCoreDumpTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsCoreDumpTests bcrypt)
	# Every block the page-guarded allocator returns to these targets is filled
	# with 0xcd instead of being left as fresh zeroed pages, so a scene, actor,
	# shape or joint constructor that leaves a field it later reads to the
	# allocation shows up as a fault or a transcript difference (joint-open-items,
	# Scene initialisation). Only targets whose registered lines the oracle
	# still prints under the fill are listed. NxPhysicsActorLifecycleTests prints
	# samples of the auxiliary manager's 256-slot arrays, whose unused slots the
	# oracle leaves unwritten and the candidate zeroes; NxPhysicsActorDynamicsTests
	# prints a changed-word mask (move_orientation) that counts words the fill
	# makes non-zero. Both run clean on both DLLs under the fill.
	foreach(nx_fill_target
			NxPhysicsActorNameTests NxPhysicsActorMetadataTests
			NxPhysicsActorBodyFlagTests
			NxPhysicsActorDynamicSetterTests NxPhysicsActorMomentumTests
			NxPhysicsActorForceTests NxPhysicsActorCMassTests
			NxPhysicsActorMassTests NxPhysicsActorShapeMutationTests NxPhysicsJointStagedPairTests
			NxPhysicsJointSlotTests NxPhysicsSceneRaycastTests
			NxPhysicsBodyCreationTests NxPhysicsSceneVisualizeTests
			NxPhysicsEffectorTests NxPhysicsCoreDumpTests)
		target_compile_definitions(${nx_fill_target} PRIVATE NX_PAGE_GUARDED_FILL)
	endforeach()
	target_include_directories(NxPhysicsJointTests PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxPhysicsJointTests bcrypt)
	# The two exported joint-descriptor rows on their own. Separate from
	# NxPhysicsJointTests because that one drives the whole SDK lifecycle, which
	# makes it sensitive to rows these two do not depend on.
	add_executable(NxPhysicsJointDescTests tests/PhysicsJointDescTests.cpp)
	target_include_directories(NxPhysicsJointDescTests PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxPhysicsJointDescTests bcrypt)
	# The synthetic-actor fixture for the two joint-descriptor rows. A measurement
	# probe: the rows reach their pose through raw pointer arithmetic, so a byte
	# buffer with the chain built into it exercises the transform arm without the
	# Scene the candidate does not have. See evidence/phase6-joints.md 7t.
	add_executable(NxJointDescSyntheticTests tests/NxJointDescSyntheticTests.cpp)
	target_include_directories(NxJointDescSyntheticTests PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxJointDescSyntheticTests bcrypt)
	# Dumps the REAL actor object graph through the oracle's own SDK, so the
	# synthetic fixture can be built to measured offsets rather than derived ones.
	# Oracle-only: the candidate's createScene returns 0. See evidence 8e-8f.
	add_executable(NxSceneGraphProbe tests/NxSceneGraphProbe.cpp)
	target_include_directories(NxSceneGraphProbe PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxSceneGraphProbe bcrypt)
	# A step probe: exercises the scene path one step at a time with no instrumentation
	# between steps, so a fault names the step. See evidence 9y.
	add_executable(NxSceneStepProbe tests/NxSceneStepProbe.cpp)
	target_include_directories(NxSceneStepProbe PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxSceneStepProbe bcrypt)
	add_executable(NxPhysicsSimulationTests tests/PhysicsSimulationTests.cpp)
	target_include_directories(NxPhysicsSimulationTests PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxPhysicsSimulationTests bcrypt)
	add_executable(NxPhysicsCcdSimulationTests tests/PhysicsCcdSimulationTests.cpp)
	target_include_directories(NxPhysicsCcdSimulationTests PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxPhysicsCcdSimulationTests bcrypt)
	add_executable(NxPhysicsSceneBoundsPlanesTests tests/PhysicsSceneBoundsPlanesTests.cpp)
	target_include_directories(NxPhysicsSceneBoundsPlanesTests PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxPhysicsSceneBoundsPlanesTests bcrypt)
	add_executable(NxPhysicsRangeIterationTests tests/PhysicsRangeIterationTests.cpp
		Physics/src/RecoveredRows.cpp)
	add_executable(NxPhysicsPairFlagTests tests/PhysicsPairFlagTests.cpp)
	target_include_directories(NxPhysicsPairFlagTests PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxPhysicsPairFlagTests bcrypt)
	add_executable(NxPhysicsMeshSimulationTests tests/PhysicsMeshSimulationTests.cpp)
	target_include_directories(NxPhysicsMeshSimulationTests PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxPhysicsMeshSimulationTests bcrypt)
	add_executable(NxPhysicsConvexMeshTests tests/PhysicsConvexMeshApiTests.cpp)
	target_include_directories(NxPhysicsConvexMeshTests PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxPhysicsConvexMeshTests bcrypt)
	add_executable(NxPhysicsTriggerSimulationTests tests/PhysicsTriggerSimulationTests.cpp)
	target_include_directories(NxPhysicsTriggerSimulationTests PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxPhysicsTriggerSimulationTests bcrypt)
	# A measurement probe, not a gate: it prints what the pinned
	# NxNormalToTangents returns so a joint row's localNormal can be attributed
	# to a specific output rather than guessed at.
	add_executable(NxNormalToTangentsProbe tests/NxNormalToTangentsProbe.cpp)
	target_include_directories(NxNormalToTangentsProbe PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxNormalToTangentsProbe bcrypt)
	# The gate that probe became: NxNormalToTangents's exact output words over
	# both arms, the threshold, degenerate inputs and a digest sweep, compared
	# between the two staged pairs. Registered on Phase 6 because
	# NxJointDesc::setGlobalAxis reaches it (evidence/phase6-joints.md 6o).
	add_executable(NxFoundationTangentTests tests/FoundationTangentTests.cpp)
	target_include_directories(NxFoundationTangentTests PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxFoundationTangentTests bcrypt)
	# Prints what NxVec3::magnitude and NxVec3::normalize do to the degenerate
	# vectors the tangent path builds, so a NaN sign can be attributed to one call.
	add_executable(NxVec3NormalizeProbe tests/NxVec3NormalizeProbe.cpp)
	target_include_directories(NxVec3NormalizeProbe PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxVec3NormalizeProbe bcrypt)
	# Shows whether NxMath::sqrt reaches fsqrt or the CRT, so a change to it can
	# be shown to have taken effect rather than merely to have compiled.
	add_executable(NxMathSqrtProbe tests/NxMathSqrtProbe.cpp)
	target_include_directories(NxMathSqrtProbe PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxMathSqrtProbe bcrypt)
	add_executable(NxPhysicsCoreClusterTests tests/PhysicsCoreClusterTests.cpp)
	target_include_directories(NxPhysicsCoreClusterTests PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxPhysicsCoreClusterTests bcrypt)
	add_executable(NxPhysicsGeometryTests tests/PhysicsGeometryTests.cpp)
	target_include_directories(NxPhysicsGeometryTests PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxPhysicsGeometryTests bcrypt)

	# The randomized kernel differential. Same staged-pair machinery as the case
	# matrix above; it is a second target rather than more cases because the
	# matrix cannot see the x87 precision model or NaN payload propagation, and
	# a digest over millions of inputs is not a transcript anyone would read
	# case by case.
	add_executable(NxPhysicsKernelFuzzTests tests/PhysicsKernelFuzzTests.cpp)
	target_include_directories(NxPhysicsKernelFuzzTests PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxPhysicsKernelFuzzTests bcrypt)

	# The Phase 4 asset-format gate. An oracle differential like
	# NxPhysicsCollisionTests below: it drives the recovered penetration-map and
	# triangle-mesh byte formats against the pinned DLL and then asks the
	# reconstruction the same questions.
	#
	# It links NxOpcode because PenetrationMap does: the payload loader builds an
	# Ice Container per value record (0x00050216) and the reorder pass sorts the
	# Morton keys with a RadixSort (0x0005052f), and both are vendored OPCODE
	# rows rather than reconstructed ones.
	#
	# ThirdPartyHost.cpp and PhysicsInternal.cpp come with NxOpcode for the same
	# reason they do in NxPhysicsThirdPartyTests: the vendored tree's allocation
	# seam is opcNovodeXAlloc/opcNovodeXFree and that reaches nxGetSdkAllocator.
	# TriangleMesh.cpp holds phys_fn_002233, the hull computation, which calls the hull
	# library (qhull-gap Task 4e), so QhullHost.cpp, Quantizer.cpp and NxQhull come with it.
	add_executable(NxPhysicsAssetTests tests/PhysicsAssetTests.cpp tests/PhysicsAssetTestSupport.cpp
		tests/PhysicsNoSdkStubs.cpp
		Physics/src/PMap.cpp Physics/src/MemoryStream.cpp Physics/src/TriangleMesh.cpp
		Physics/src/InternalTriangleMesh.cpp
		Physics/src/EdgeList.cpp Physics/src/IceAdjacencies.cpp Physics/src/IceMeshTools.cpp
		Physics/src/IceMeshBuilder2.cpp Physics/src/ConvexHull.cpp Physics/src/TriangleMeshTopology.cpp
		Physics/src/IceSupportMaps.cpp Physics/src/SmoothNormals.cpp Physics/src/TriangleMeshPolygons.cpp
		Physics/src/ThirdPartyHost.cpp Physics/src/PhysicsInternal.cpp
		Physics/src/QhullHost.cpp Physics/src/Quantizer.cpp
		tests/PhysicsInternalContactStubs.cpp tests/PhysicsInternalMeshContactStubs.cpp)
	target_include_directories(NxPhysicsAssetTests PRIVATE
		Foundation/include Foundation/src/include Physics/include Physics/src/include
		# The generated oracle-RVA to candidate-address table. It is built from the census and the
		# rebuild's linker map, and this target is the one that drives rows through it.
		docs/reconstruction/novodex-physics)
	# NxReleasePMap is declared NXP_DLL_EXPORT in the pinned public NxPMap.h and
	# this target links PMap.cpp directly, so the declaration must not come out
	# dllimport. NX_PHYSICS_DLL is the arm of Nxp.h's chain that gives NXP export
	# and leaves NXF import, which is what PhysicsInternal.cpp needs to reach
	# nxFoundationSDKAllocator in NxFoundation.dll. NX_PHYSICS_STATICLIB would
	# clear both and break that import.
	target_compile_definitions(NxPhysicsAssetTests PRIVATE NX_PHYSICS_DLL=1 NX_TEST_REAL_TRIANGLE_MESH=1 NX_TEST_STUB_TRIANGLE_MESH_POLYGON_TABLE=1)
	# NxFoundation stays linked FOR NOW. Removing it was tried in round 76 and fails at link time on one
	# imported variable, `nxFoundationSDKAllocator`, reached through the local nxGetSdkAllocator helper
	# by MemoryStream.cpp, ThirdPartyHost.cpp and PhysicsInternal.cpp. The dependency is one symbol and
	# the fix is to define it locally and bridge it, which is what the other harnesses here do through
	# nxSetSdkAllocatorBridge -- but the definition has to be in place before the link comes out, or the
	# tree stops building. Round 75 measured why it matters: linking the Foundation makes the loader
	# resolve it as an import before any harness code runs, so the PAIR's copy is never the one loaded.
	target_link_libraries(NxPhysicsAssetTests NxFoundation NxQhull NxOpcode bcrypt)
	# A linker map, so the vendored-correspondence execution trace can resolve the vendored
	# functions linked into this exe by name (evidence/vendored-trace-*.txt). /MAP writes a
	# file beside the exe and changes nothing in it.
	if(MSVC)
		target_link_options(NxPhysicsAssetTests PRIVATE /MAP)
	endif()

	# The vendored third-party differential. Same shape as the two above: it
	# holds the pinned oracle in its own process and calls qhull and OPCODE rows
	# at their recorded internal addresses, comparing against the vendored
	# sources linked in beside them.
	#
	# It exists because vendoring proves nothing on its own. 722 census rows
	# acquired a source file; a source file nothing runs is present, not proven.
	#
	# It links ThirdPartyHost.cpp and its neighbours because the vendored tree's
	# allocation seam reaches nxGetSdkAllocator -- phys_fn_004803, a Phase 2 row
	# -- and that pulls PhysicsInternal.cpp in with it, the same way
	# NxPhysicsInternalTests does.
	#
	# tests/PhysicsThirdPartyHost.cpp compiles Physics/src/ThirdPartyHost.cpp
	# and Physics/src/QhullHost.cpp unchanged but for qhull's nine hooks, which
	# it renames and routes per call: to the product's host object while one is
	# published (a candidate CreateConvexHull run), to the harness's capture
	# sink while a run installs one (qhull-gap Task 1: the oracle's printing is
	# captured through its host object, the candidate's through the same sink),
	# and otherwise to the stand-in every qhull family was measured against
	# (qhull-gap Task 4a). TriangleMesh.cpp (and MemoryStream.cpp, which its
	# writer uses) is linked for phys_fn_002233/002235/002237, which the
	# convex-cooking differentials drive (qhull-gap Task 4e). PMap.cpp is also
	# needed for TriangleMesh's public PMap attachment and load path.
	add_executable(NxPhysicsThirdPartyTests tests/PhysicsThirdPartyTests.cpp
		tests/PhysicsThirdPartyQhull.c
		tests/PhysicsThirdPartyHost.cpp Physics/src/Quantizer.cpp Physics/src/PhysicsInternal.cpp
		tests/PhysicsNoSdkStubs.cpp
		Physics/src/Containers.cpp tests/PhysicsInternalContactStubs.cpp
		tests/PhysicsInternalMeshContactStubs.cpp
		Physics/src/opcode/IcePrunable.cpp Physics/src/opcode/IcePruner.cpp
		Physics/src/opcode/NovodexBoxPruning.cpp
		Physics/src/PMap.cpp Physics/src/TriangleMesh.cpp Physics/src/InternalTriangleMesh.cpp
		Physics/src/MemoryStream.cpp
		Physics/src/EdgeList.cpp Physics/src/IceAdjacencies.cpp Physics/src/IceMeshTools.cpp
		Physics/src/IceMeshBuilder2.cpp Physics/src/ConvexHull.cpp Physics/src/TriangleMeshTopology.cpp
		Physics/src/IceSupportMaps.cpp Physics/src/SmoothNormals.cpp
		Physics/src/TriangleMeshPolygons.cpp)
	target_compile_definitions(NxPhysicsThirdPartyTests PRIVATE NX_PHYSICS_DLL=1)
	# TriangleMeshPolygons.cpp since convex-mesh gap Task 2g: the polygon_interface
	# family drives the TriangleMesh polygon interface (P-Mesh) over hull images.
	# SmoothNormals.cpp comes with IceMeshTools.cpp since convex-mesh gap Task 2e:
	# 001651 calls the angle helper 002144, which is written there, beside the
	# export that also calls it. The export is declared NXP_DLL_EXPORT, so the file
	# is compiled with the DLL's linkage in every target that links it (as
	# NxPhysicsCollisionTests does for the whole target); here only the file gets
	# it, so the harness's own translation units keep theirs.
	set_source_files_properties(Physics/src/SmoothNormals.cpp PROPERTIES COMPILE_DEFINITIONS NX_PHYSICS_DLL=1)
	set_source_files_properties(Physics/src/PMap.cpp PROPERTIES COMPILE_DEFINITIONS NX_PHYSICS_DLL=1)
	# The hull family's C half reads qhull's own structs, so it and only it sees the
	# merged tree's headers (their fprintf redirection stays out of the C++ harness).
	set_source_files_properties(tests/PhysicsThirdPartyQhull.c PROPERTIES
		INCLUDE_DIRECTORIES "${CMAKE_BINARY_DIR}/External/qhull-tree")
	target_include_directories(NxPhysicsThirdPartyTests PRIVATE
		Foundation/include Foundation/src/include Physics/include Physics/src/include
		Physics/src/opcode)
	target_compile_definitions(NxPhysicsThirdPartyTests PRIVATE NX_TEST_REAL_TRIANGLE_MESH=1)
	target_link_libraries(NxPhysicsThirdPartyTests NxFoundation NxQhull NxOpcode bcrypt)
	# The harness itself compares x87 results the vendored libraries produce, so
	# it follows the same architecture rule they do. A target option rather than
	# a source property: the source-property rule above names Physics/src files
	# and a second one naming the same files would silently override it.
	if(MSVC)
		target_compile_options(NxPhysicsThirdPartyTests PRIVATE /arch:IA32 /GR- /wd4996)
		# The map the vendored-correspondence trace resolves breakpoints from; see
		# NxPhysicsAssetTests above.
		target_link_options(NxPhysicsThirdPartyTests PRIVATE /MAP)
		# The candidate qhull linked into this exe prints its traces through the
		# UCRT, as NxPhysics.dll's does, so it gets NxPhysics.dll's printf rounding
		# too (see legacy_stdio_float_rounding.obj above): without it a "%2.2g" of
		# an exact tie such as -3.25 prints -3.2 where the oracle's 2003 CRT, and
		# the shipped candidate DLL, print -3.3 (qhull-gap Task 1).
		target_link_options(NxPhysicsThirdPartyTests PRIVATE legacy_stdio_float_rounding.obj)
	endif()

	# ObjectModel.cpp's MESH slot 7 (the 001407 model) calls the support-map
	# lookup 001556 and the hull's polygons 001472 (convex-mesh gap Task 2f), and
	# they bring the ICE rows they call, so every target that compiles
	# ObjectModel.cpp links these (SmoothNormals.cpp too, where the target does
	# not already have it; 001651 calls its angle helper 002144).
	set(NX_ICE_MESH_ROW_SOURCES
		Physics/src/IceSupportMaps.cpp Physics/src/ConvexHull.cpp Physics/src/IceAdjacencies.cpp
		Physics/src/EdgeList.cpp Physics/src/IceMeshTools.cpp Physics/src/IceMeshBuilder2.cpp)

	# A static-proof gate, not a differential: it links the private translation
	# units directly because the rows it covers cannot be reached through the
	# public API. Same shape as NxFoundationCustomArrayTests above.
	add_executable(NxPhysicsInternalTests tests/PhysicsInternalTests.cpp
		tests/PhysicsInternalMeshContactStubs.cpp
		Physics/src/PhysicsInternal.cpp
		Physics/src/Containers.cpp
		Physics/src/Scene.cpp Physics/src/NpScene.cpp Physics/src/NpActor.cpp
		Physics/src/SceneRaycast.cpp Physics/src/BodyCreation.cpp Physics/src/SceneVisualize.cpp
		Physics/src/BodyStep.cpp Physics/src/Island.cpp Physics/src/ContactPairManager.cpp
		Physics/src/ObjectModel.cpp Physics/src/StepOnlyRows.cpp Physics/src/MemoryStream.cpp
		Physics/src/Geometry.cpp Physics/src/ShapeRaycast.cpp
		Physics/src/InternalTriangleMesh.cpp
		Physics/src/TriangleMesh.cpp
		Physics/src/TriangleMeshPolygons.cpp Physics/src/TriangleMeshTopology.cpp Physics/src/PMap.cpp
		Physics/src/QhullHost.cpp Physics/src/Quantizer.cpp
		Physics/src/opcode/IcePrunable.cpp Physics/src/ThirdPartyHost.cpp
		Physics/src/opcode/IcePruner.cpp Physics/src/opcode/IcePruningEngine.cpp
		Physics/src/opcode/NovodexBoxPruning.cpp
		Physics/src/PhysicsSDK.cpp Physics/src/NpPhysicsSDK.cpp
		Physics/src/NarrowPhase.cpp Physics/src/ContactGeneration.cpp Physics/src/Distance.cpp
		${NX_ICE_MESH_ROW_SOURCES} Physics/src/SmoothNormals.cpp
		${PHYSICS_CORE_SOURCES})
	target_include_directories(NxPhysicsInternalTests PRIVATE
		Foundation/include Foundation/src/include Physics/include Physics/src/include
		Physics/src/opcode)
	# Geometry.cpp defines exports that ObjectModel.cpp calls, so it needs the
	# DLL's linkage here, as in NxPhysicsCollisionTests below. Scene.cpp's
	# createJoint constructs all ten joint families, so the target links
	# Physics/src/core/*.cpp, and the families' SDK-parameter reads
	# (PhysicsSDK::getParameter, PhysicsSDK::instance) pull in PhysicsSDK.cpp and
	# NpPhysicsSDK.cpp. The core dump (core/SceneDump.cpp) reads a shape's owner
	# and type through phys_fn_001281/001283 in ContactGeneration.cpp, which
	# needs NarrowPhase.cpp. The /arch:IA32 rule above is per directory, so it
	# gives the internal joint files NoExtensions here too.
	target_compile_definitions(NxPhysicsInternalTests PRIVATE NX_PHYSICS_DLL=1 NX_TEST_REAL_NARROW_PHASE=1)
	target_link_libraries(NxPhysicsInternalTests NxFoundation NxOpcode NxQhull)

	# The Phase 5 object-layout gate. RED on purpose until Tasks 2 and 3
	# transcribe the object-model classes: it pins vtable identity, the
	# collision-object constructor bytes and the owner accessor against the
	# pinned DLL so the layouts Tasks 2 and 3 write already have something
	# failing for them. See evidence/phase5-object-model.md.
	add_executable(NxPhysicsObjectLayoutTests tests/PhysicsObjectLayoutTests.cpp
		Physics/src/ObjectModel.cpp Physics/src/StepOnlyRows.cpp Physics/src/Containers.cpp
		tests/PhysicsObjectModelSceneStubs.cpp
		Physics/src/MemoryStream.cpp
		Physics/src/Geometry.cpp Physics/src/ShapeRaycast.cpp
		Physics/src/opcode/IcePrunable.cpp
		Physics/src/ThirdPartyHost.cpp Physics/src/PhysicsInternal.cpp
		tests/PhysicsInternalContactStubs.cpp tests/PhysicsInternalMeshContactStubs.cpp
		${NX_ICE_MESH_ROW_SOURCES} Physics/src/SmoothNormals.cpp)
	target_include_directories(NxPhysicsObjectLayoutTests PRIVATE
		Foundation/include Foundation/src/include Physics/include Physics/src/include
		Physics/src/opcode)
	target_link_libraries(NxPhysicsObjectLayoutTests NxFoundation NxOpcode bcrypt)
	if(MSVC)
		target_compile_options(NxPhysicsObjectLayoutTests PRIVATE /arch:IA32 /GR- /wd4996)
		# Emit a linker map so that a fault offset reported by the OS -- the
		# 0xC0000409 aborts this harness produces -- can be resolved to a
		# function instead of being guessed at from the source. The PDB is what
		# lets a crash dump unwind past the CRT fast-fail and name the frame
		# that tripped the /GS check.
		target_link_options(NxPhysicsObjectLayoutTests PRIVATE /MAP /DEBUG:FULL)
		target_compile_options(NxPhysicsObjectLayoutTests PRIVATE /Zi)
		# The layout-test translation unit carries the /GS stack-cookie check
		# failure documented in the phase-5 evidence (3z238-3z252). It is a
		# FAST_FAIL_STACK_COOKIE_CHECK_FAILURE raised inside an inlined wmain,
		# armed by ANY change to that function, and it discards buffered stdout
		# -- so it makes the harness unextendable. The corruption is in the
		# harness's own frame, not in the reconstructed product, and the
		# reconstructed translation units keep /GS. Disabling it here (and only
		# here) restores the ability to add differential blocks.
		set_source_files_properties(tests/PhysicsObjectLayoutTests.cpp
			PROPERTIES COMPILE_OPTIONS "/GS-")
	endif()

	# Isolated candidate/oracle dispatch tests for constructed shape tables.
	# The layout gate's single huge wmain has known stack-frame corruption if
	# extended, so vtable probes live in this small executable.
	add_executable(NxPhysicsShapeVtableTests tests/PhysicsShapeVtableTests.cpp
		Physics/src/ObjectModel.cpp Physics/src/StepOnlyRows.cpp Physics/src/Containers.cpp
		tests/PhysicsObjectModelSceneStubs.cpp
		Physics/src/MemoryStream.cpp Physics/src/Geometry.cpp
		Physics/src/ShapeRaycast.cpp
		Physics/src/opcode/IcePrunable.cpp
		Physics/src/ThirdPartyHost.cpp Physics/src/PhysicsInternal.cpp
		tests/PhysicsInternalContactStubs.cpp tests/PhysicsInternalMeshContactStubs.cpp
		${NX_ICE_MESH_ROW_SOURCES} Physics/src/SmoothNormals.cpp)
	target_include_directories(NxPhysicsShapeVtableTests PRIVATE
		Foundation/include Foundation/src/include Physics/include Physics/src/include
		Physics/src/opcode)
	target_link_libraries(NxPhysicsShapeVtableTests NxFoundation NxOpcode bcrypt)
	if(MSVC)
		target_compile_options(NxPhysicsShapeVtableTests PRIVATE /arch:IA32 /GR- /wd4996)
		# A linker map, so a cdb trace can break on the candidate functions
		# linked into this exe by name (evidence/scene-raycast-trace-task4-
		# shape.txt). /MAP writes a file beside the exe and changes nothing in it.
		target_link_options(NxPhysicsShapeVtableTests PRIVATE /MAP)
	endif()
	# The narrow-phase differential. Neither a staged-pair differential nor a
	# static proof: it holds the pinned oracle in its own process and calls the
	# recorded internal addresses, because the shape-pair dispatch matrix and
	# the overlap tests it selects are not exported and the public path that
	# would reach them belongs to phases 4, 5 and 7. It links the
	# reconstruction so both sides run in one process and the comparison is the
	# harness's own, which is why -- unlike a symmetric differential -- a check
	# inside it can fail the gate.
	#
	# The last four are not narrow-phase code. phys_fn_002266, which both sphere
	# entries of matrix A call, reads NX_CONTINUOUS_CD out of the SDK's live
	# parameter array through phys_fn_000429 -- a Phase 2 row -- so the
	# reconstruction of that guard has to reach the same array the oracle does
	# rather than a copy of it. PhysicsSDK.cpp owns it and drags its own
	# neighbours in with it.
	# tests/PhysicsCollisionOpcode.cpp (convex-mesh gap Task 2i) holds the candidate
	# side's OPCODE models, OBBCollider and the Foundation SDK the Task 2i
	# families need, apart from Opcode.h's pragmas (see PhysicsCollisionInflate.cpp).
	add_executable(NxPhysicsCollisionTests tests/PhysicsCollisionTests.cpp tests/PhysicsCollisionInflate.cpp tests/PhysicsCollisionOpcode.cpp
		Physics/src/NarrowPhase.cpp Physics/src/ContactGeneration.cpp
		Physics/src/Distance.cpp Physics/src/ShapeRaycast.cpp
		Physics/src/Geometry.cpp Physics/src/SmoothNormals.cpp
		Physics/src/ContactBoxMeshICE.cpp Physics/src/ContactCapsuleMesh.cpp Physics/src/ContactMeshHeightfield.cpp Physics/src/ContactMeshMesh.cpp Physics/src/ContactPlaneMesh.cpp
		Physics/src/ContactConvexConvex.cpp Physics/src/TriangleMeshPolygons.cpp
		Physics/src/ContactConvexHeightfield.cpp Physics/src/ContactSphereMesh.cpp
		Physics/src/TriangleMeshTopology.cpp Physics/src/TriangleMesh.cpp Physics/src/InternalTriangleMesh.cpp Physics/src/PMap.cpp
		Physics/src/QhullHost.cpp Physics/src/Quantizer.cpp
		Physics/src/PhysicsSDK.cpp Physics/src/NpPhysicsSDK.cpp
		Physics/src/PhysicsInternal.cpp
		Physics/src/Containers.cpp
		Physics/src/Scene.cpp Physics/src/NpScene.cpp Physics/src/NpActor.cpp
		Physics/src/SceneRaycast.cpp Physics/src/BodyCreation.cpp Physics/src/SceneVisualize.cpp
		Physics/src/ObjectModel.cpp Physics/src/BodyStep.cpp Physics/src/Island.cpp
		Physics/src/ContactPairManager.cpp Physics/src/StepOnlyRows.cpp Physics/src/MemoryStream.cpp
		Physics/src/opcode/IcePrunable.cpp Physics/src/ThirdPartyHost.cpp
		Physics/src/opcode/IcePruner.cpp Physics/src/opcode/IcePruningEngine.cpp
		Physics/src/opcode/NovodexBoxPruning.cpp
		${NX_ICE_MESH_ROW_SOURCES} Physics/src/SmoothNormals.cpp
		${PHYSICS_CORE_SOURCES})
	target_include_directories(NxPhysicsCollisionTests PRIVATE
		Foundation/include Foundation/src/include Physics/include Physics/src/include
		Physics/src/opcode)
	# Geometry.cpp defines exports, so it has to be compiled with the same
	# linkage here as in the DLL or every one of them warns.
	target_compile_definitions(NxPhysicsCollisionTests PRIVATE NX_PHYSICS_DLL=1)
	target_link_libraries(NxPhysicsCollisionTests NxFoundation NxOpcode NxQhull bcrypt)
	# The joint-support solver-row differential follows the same in-process oracle
	# pattern as the narrow-phase harness: it calls the pinned internal wrapper by
	# RVA and compares its record/body/callback effects with JointSupport.cpp linked
	# into the test. Kind 5 has no public joint-record producer in the current
	# reconstruction, so the fixture builds the measured Scene support arrays.
	add_executable(NxPhysicsJointSupportTests tests/PhysicsJointSupportTests.cpp
		Physics/src/core/JointSupport.cpp)
	target_include_directories(NxPhysicsJointSupportTests PRIVATE
		Foundation/include Foundation/src/include Physics/include Physics/src/include)
	target_compile_definitions(NxPhysicsJointSupportTests PRIVATE NX_PHYSICS_STATICLIB=1)
	target_link_libraries(NxPhysicsJointSupportTests bcrypt)
	# A linker map supports scene-raycast and convex-mesh-gap candidate traces;
	# it changes nothing in the executable.
	if(MSVC)
		target_link_options(NxPhysicsCollisionTests PRIVATE /MAP)
	endif()
	# No architecture rule here: source file properties are per directory, so
	# the /arch:IA32 rule above already covers these two files in this target
	# too. A second rule naming them would set the same value and silently
	# override the first, which would leave the first untestable -- the mutation
	# that removes NarrowPhase.cpp from it moved nothing until this was deleted.
endif()

#### VIEWER ####
set(NOVODEX_INSTALL_TARGETS NxFoundation NxPhysics)
if(NOVODEX_BUILD_VIEWER)
    add_subdirectory(Viewer)
    list(APPEND NOVODEX_INSTALL_TARGETS Viewer)
endif()

# Keep installed SDK libraries and the viewer together in the project root,
# including when an existing build cache has another install prefix.
install(TARGETS ${NOVODEX_INSTALL_TARGETS}
    RUNTIME DESTINATION "${PROJECT_SOURCE_DIR}/Bin"
    LIBRARY DESTINATION "${PROJECT_SOURCE_DIR}/Bin"
    ARCHIVE DESTINATION "${PROJECT_SOURCE_DIR}/Bin"
)
