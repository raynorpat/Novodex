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

#### PHYSICS ####

file(GLOB_RECURSE PHYSICS_HEADERS Physics/include/*.h Physics/src/include/*.h)
source_group("Header Files" FILES ${PHYSICS_HEADERS})

file(GLOB PHYSICS_SOURCES Physics/src/*.cpp)
source_group("Source Files" FILES ${PHYSICS_SOURCES})

if(WIN32)
	file(GLOB PHYSICS_RESOURCES Physics/src/win32/*.rc Physics/src/win32/*.h)
	source_group("Resource Files" FILES ${PHYSICS_RESOURCES})
endif()

add_library(NxPhysics SHARED ${PHYSICS_HEADERS} ${PHYSICS_SOURCES} ${PHYSICS_RESOURCES})
target_link_libraries(NxPhysics NxFoundation)
set_target_properties(NxPhysics PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(NxPhysics PRIVATE
	Foundation/include
	Foundation/src/include
	Physics/include
	Physics/src/include
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
		PROPERTIES COMPILE_OPTIONS "/arch:IA32")
endif()

# NOTE FOR WHOEVER ADDS THE NEXT KERNEL TRANSLATION UNIT. This rule names three
# files, not a directory, so anything else added under Physics/src builds SSE2
# by default and will silently disagree with the oracle on NaN payloads -- and,
# if it is written with NxReal temporaries rather than double register
# lifetimes, on the last bit of ordinary results too. The collision kernels
# Phase 3 Task 3 adds (BroadPhase.cpp, NarrowPhase.cpp, ContactGeneration.cpp,
# Filtering.cpp) are almost certainly x87 in the 2003 oracle as well: check one
# against the shipped export before assuming otherwise, and extend this list if
# so. The check is cheap -- pick an input where the two models differ in the
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

	# A static-proof gate, not a differential: it links the private translation
	# units directly because the rows it covers cannot be reached through the
	# public API. Same shape as NxFoundationCustomArrayTests above.
	add_executable(NxPhysicsInternalTests tests/PhysicsInternalTests.cpp
		Physics/src/PhysicsInternal.cpp Physics/src/Containers.cpp)
	target_include_directories(NxPhysicsInternalTests PRIVATE
		Foundation/include Foundation/src/include Physics/include Physics/src/include)
	target_link_libraries(NxPhysicsInternalTests NxFoundation)
endif()
