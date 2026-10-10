# Genuine Foundation singleton, observers and required real virtual dependencies.
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4 OR
        NOT (MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    message(FATAL_ERROR "The Foundation lifecycle gate currently requires Win32: native platform timing remains Task9")
endif()
set(FOUNDATION_LIFECYCLE_SOURCES FoundationLifecycleTests.cpp FixtureSupport.cpp
    ../../Foundation/src/FoundationSDK.cpp ../../Foundation/src/Observable.cpp
    ../../Foundation/src/DebugRenderable.cpp ../../Foundation/src/Profiler.cpp
    ../../Foundation/src/Time.cpp ../../Foundation/src/Utilities.cpp ../../Foundation/src/Box.cpp)
function(nx_foundation_lifecycle_target target backend)
    add_executable(${target} ${FOUNDATION_LIFECYCLE_SOURCES})
    target_include_directories(${target} PRIVATE . ../../Physics/src/include
        ../../Foundation/include ../../Foundation/src/include ../../Foundation/src)
    target_compile_definitions(${target} PRIVATE NX_PHYSICS_USE_X87=${backend}
        NX32=1 WIN32=1 NXF_DLL_EXPORT=)
    target_compile_features(${target} PRIVATE cxx_std_11)
    if(backend)
        target_compile_options(${target} PRIVATE /arch:IA32 /fp:precise /Qfast_transcendentals)
    else()
        target_compile_options(${target} PRIVATE ${NOVODEX_PORTABLE_FP_OPTIONS})
    endif()
endfunction()
nx_foundation_lifecycle_target(NxPortableFoundationLifecycleTests 0)
add_test(NAME Portable.Foundation.Lifecycle COMMAND NxPortableFoundationLifecycleTests
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/foundation-lifecycle-x87.nxpf")
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    nx_foundation_lifecycle_target(NxPortableExportFoundationLifecycle 1)
endif()
foreach(backend 0 1)
    if(backend AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        continue()
    endif()
    if(backend)
        set(target NxPortableExportFoundationPublicMath)
    else()
        set(target NxPortableFoundationPublicMathTests)
    endif()
    add_executable(${target} FoundationPublicMathTests.cpp FixtureSupport.cpp)
    target_include_directories(${target} PRIVATE . ../../Foundation/include)
    target_compile_definitions(${target} PRIVATE NX_PHYSICS_USE_X87=${backend} NX32=1 WIN32=1 NXF_DLL_EXPORT=)
    target_compile_features(${target} PRIVATE cxx_std_11)
    if(backend)
        target_compile_options(${target} PRIVATE /arch:IA32 /fp:precise /Qfast_transcendentals)
    else()
        target_compile_options(${target} PRIVATE ${NOVODEX_PORTABLE_FP_OPTIONS})
    endif()
endforeach()
add_test(NAME Portable.Foundation.PublicMath COMMAND NxPortableFoundationPublicMathTests
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/foundation-public-math-x87.nxpf")
