cmake_minimum_required(VERSION 3.16)
project(NxPortableKernels LANGUAGES CXX)
enable_testing()
include(../../cmake/NxPhysicsBackend.cmake)
add_executable(NxPortableFixtureTests FixtureSupportTests.cpp FixtureSupport.cpp)
target_compile_features(NxPortableFixtureTests PRIVATE cxx_std_11)
target_compile_definitions(NxPortableFixtureTests PRIVATE NX_PHYSICS_USE_X87=0)
novodex_portable_fp_options(NOVODEX_PORTABLE_FP_OPTIONS)
target_compile_options(NxPortableFixtureTests PRIVATE ${NOVODEX_PORTABLE_FP_OPTIONS})
add_executable(NxPortableBackendHeaderTests BackendHeaderTests.cpp)
target_include_directories(NxPortableBackendHeaderTests PRIVATE ../../Physics/src/include)
target_compile_definitions(NxPortableBackendHeaderTests PRIVATE NX_PHYSICS_USE_X87=0)
target_compile_options(NxPortableBackendHeaderTests PRIVATE ${NOVODEX_PORTABLE_FP_OPTIONS})
target_compile_features(NxPortableBackendHeaderTests PRIVATE cxx_std_11)
add_test(NAME Portable.BackendHeader COMMAND NxPortableBackendHeaderTests)
add_test(NAME Portable.BackendSelection COMMAND "${CMAKE_COMMAND}"
    "-DTEST_WORK_DIR=${CMAKE_CURRENT_BINARY_DIR}/backend-selection-tests"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/BackendSelectionTests.cmake")
add_test(NAME Portable.FixtureSupport COMMAND NxPortableFixtureTests)
add_test(NAME Portable.FixtureReference COMMAND NxPortableFixtureTests
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/shared-math-x87.nxpf")
set_tests_properties(Portable.FixtureSupport Portable.FixtureReference PROPERTIES RUN_SERIAL TRUE)
# Reference capture requires the maintained MSVC x87 compiler. Other compilers
# can still build the standalone scalar tests without the legacy exporter.
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    novodex_check_x87_compiler()
    add_executable(NxPortableExportSharedMath ExportSharedMathFixtures.cpp)
    target_compile_definitions(NxPortableExportSharedMath PRIVATE NX_PHYSICS_USE_X87=1)
    target_include_directories(NxPortableExportSharedMath PRIVATE
        ../../Physics/src/include ../../Physics/include ../../Foundation/include)
    target_compile_options(NxPortableExportSharedMath PRIVATE /arch:IA32 /fp:precise)
endif()
