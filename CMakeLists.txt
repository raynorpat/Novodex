cmake_minimum_required(VERSION 3.2...3.16)

project(Novodex VERSION 2.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)

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
file(GLOB PHYSICS_SOURCES Physics/src/*.cpp Physics/src/opcode/*.cpp)
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
# else in NxPhysics keeps the default architecture.
if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
	set_source_files_properties(
		Physics/src/Geometry.cpp
		Physics/src/MassProperties.cpp
		Physics/src/SmoothNormals.cpp
		Physics/src/NarrowPhase.cpp
		Physics/src/ContactGeneration.cpp
		PROPERTIES COMPILE_OPTIONS "/arch:IA32")
endif()

# All four files are on that list because the x87 control word is not the same
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
	# A fresh process isolates actor/shape name-map behavior from the populated
	# lifecycle fixture's intentionally stateful teardown sequence.
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
	# The same include set as the oracle target above; the harness includes the
	# Foundation's allocator interface for the page-guarded allocator, and Physics'
	# public headers for the descriptors.
	target_include_directories(NxPhysicsJointStagedPairTests PRIVATE
		Foundation/include Foundation/src/include Physics/include)
	target_link_libraries(NxPhysicsJointStagedPairTests bcrypt)
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
	# A measurement probe, not a gate: it prints what the pinned
	# NxNormalToTangents returns so a joint row's localNormal can be attributed
	# to a specific output rather than guessed at.
	add_executable(NxNormalToTangentsProbe tests/NxNormalToTangentsProbe.cpp)
	target_include_directories(NxNormalToTangentsProbe PRIVATE Foundation/include Physics/include)
	target_link_libraries(NxNormalToTangentsProbe bcrypt)
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
	add_executable(NxPhysicsAssetTests tests/PhysicsAssetTests.cpp
		Physics/src/PMap.cpp Physics/src/MemoryStream.cpp Physics/src/TriangleMesh.cpp
		Physics/src/ThirdPartyHost.cpp Physics/src/PhysicsInternal.cpp)
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
	target_compile_definitions(NxPhysicsAssetTests PRIVATE NX_PHYSICS_DLL=1)
	# NxFoundation stays linked FOR NOW. Removing it was tried in round 76 and fails at link time on one
	# imported variable, `nxFoundationSDKAllocator`, reached through the local nxGetSdkAllocator helper
	# by MemoryStream.cpp, ThirdPartyHost.cpp and PhysicsInternal.cpp. The dependency is one symbol and
	# the fix is to define it locally and bridge it, which is what the other harnesses here do through
	# nxSetSdkAllocatorBridge -- but the definition has to be in place before the link comes out, or the
	# tree stops building. Round 75 measured why it matters: linking the Foundation makes the loader
	# resolve it as an import before any harness code runs, so the PAIR's copy is never the one loaded.
	target_link_libraries(NxPhysicsAssetTests NxOpcode bcrypt)

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
	add_executable(NxPhysicsThirdPartyTests tests/PhysicsThirdPartyTests.cpp
		Physics/src/ThirdPartyHost.cpp Physics/src/PhysicsInternal.cpp
		Physics/src/opcode/IcePrunable.cpp Physics/src/opcode/NovodexBoxPruning.cpp)
	target_include_directories(NxPhysicsThirdPartyTests PRIVATE
		Foundation/include Foundation/src/include Physics/include Physics/src/include
		Physics/src/opcode)
	target_link_libraries(NxPhysicsThirdPartyTests NxFoundation NxQhull NxOpcode bcrypt)
	# The harness itself compares x87 results the vendored libraries produce, so
	# it follows the same architecture rule they do. A target option rather than
	# a source property: the source-property rule above names Physics/src files
	# and a second one naming the same files would silently override it.
	if(MSVC)
		target_compile_options(NxPhysicsThirdPartyTests PRIVATE /arch:IA32 /GR- /wd4996)
	endif()

	# A static-proof gate, not a differential: it links the private translation
	# units directly because the rows it covers cannot be reached through the
	# public API. Same shape as NxFoundationCustomArrayTests above.
	add_executable(NxPhysicsInternalTests tests/PhysicsInternalTests.cpp
		Physics/src/PhysicsInternal.cpp Physics/src/Containers.cpp
		Physics/src/Scene.cpp Physics/src/NpScene.cpp Physics/src/NpActor.cpp Physics/src/NpJoint.cpp)
	target_include_directories(NxPhysicsInternalTests PRIVATE
		Foundation/include Foundation/src/include Physics/include Physics/src/include)
	target_link_libraries(NxPhysicsInternalTests NxFoundation)

	# The Phase 5 object-layout gate. RED on purpose until Tasks 2 and 3
	# transcribe the object-model classes: it pins vtable identity, the
	# collision-object constructor bytes and the owner accessor against the
	# pinned DLL so the layouts Tasks 2 and 3 write already have something
	# failing for them. See evidence/phase5-object-model.md.
	add_executable(NxPhysicsObjectLayoutTests tests/PhysicsObjectLayoutTests.cpp
		Physics/src/ObjectModel.cpp Physics/src/Containers.cpp
		Physics/src/MemoryStream.cpp
		Physics/src/Geometry.cpp
		Physics/src/opcode/IcePrunable.cpp
		Physics/src/ThirdPartyHost.cpp Physics/src/PhysicsInternal.cpp)
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
	add_executable(NxPhysicsCollisionTests tests/PhysicsCollisionTests.cpp
		Physics/src/NarrowPhase.cpp Physics/src/ContactGeneration.cpp
		Physics/src/Geometry.cpp Physics/src/SmoothNormals.cpp
		Physics/src/PhysicsSDK.cpp Physics/src/NpPhysicsSDK.cpp
		Physics/src/PhysicsInternal.cpp Physics/src/Containers.cpp
		Physics/src/Scene.cpp Physics/src/NpScene.cpp Physics/src/NpActor.cpp Physics/src/NpJoint.cpp)
	target_include_directories(NxPhysicsCollisionTests PRIVATE
		Foundation/include Foundation/src/include Physics/include Physics/src/include)
	# Geometry.cpp defines exports, so it has to be compiled with the same
	# linkage here as in the DLL or every one of them warns.
	target_compile_definitions(NxPhysicsCollisionTests PRIVATE NX_PHYSICS_DLL=1)
	target_link_libraries(NxPhysicsCollisionTests NxFoundation bcrypt)
	# No architecture rule here: source file properties are per directory, so
	# the /arch:IA32 rule above already covers these two files in this target
	# too. A second rule naming them would set the same value and silently
	# override the first, which would leave the first untestable -- the mutation
	# that removes NarrowPhase.cpp from it moved nothing until this was deleted.
endif()
