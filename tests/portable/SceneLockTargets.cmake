if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4 OR NOT
        (MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    message(FATAL_ERROR "Scene lock lifetime requires fixed32 Win32; native OS/layout remains Task9")
endif()
add_executable(NxPortableSceneLockTests SceneLockTests.cpp FixtureSupport.cpp
    ../../Physics/src/ReadWriteLockScalar.cpp
    ../../Foundation/src/FoundationSDK.cpp ../../Foundation/src/Observable.cpp
    ../../Foundation/src/DebugRenderable.cpp ../../Foundation/src/Profiler.cpp
    ../../Foundation/src/Time.cpp ../../Foundation/src/Utilities.cpp ../../Foundation/src/Box.cpp)
target_include_directories(NxPortableSceneLockTests PRIVATE . ../../Physics/src/include
    ../../Physics/include ../../Foundation/include ../../Foundation/src/include ../../Foundation/src)
target_compile_definitions(NxPortableSceneLockTests PRIVATE NX_PHYSICS_USE_X87=0 NX32=1 WIN32=1
    NXP_DLL_EXPORT= NXF_DLL_EXPORT=)
target_compile_features(NxPortableSceneLockTests PRIVATE cxx_std_14)
target_compile_options(NxPortableSceneLockTests PRIVATE ${NOVODEX_PORTABLE_FP_OPTIONS})
add_test(NAME Portable.ConvexContact.SceneLocks COMMAND NxPortableSceneLockTests
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/scene-locks-x87.nxpf")
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_executable(NxPortableExportSceneLocks ExportShippedSceneLocks.cpp)
    target_include_directories(NxPortableExportSceneLocks PRIVATE ../../Physics/include ../../Foundation/include)
    target_compile_definitions(NxPortableExportSceneLocks PRIVATE NX_PHYSICS_USE_X87=1 NX32=1 WIN32=1 NXP_DLL_EXPORT= NXF_DLL_EXPORT=)
    target_compile_features(NxPortableExportSceneLocks PRIVATE cxx_std_14)
    target_compile_options(NxPortableExportSceneLocks PRIVATE /arch:IA32 /fp:precise /Qfast_transcendentals)
    set_property(TARGET NxPortableExportSceneLocks PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(NxPortableExportSceneLocks PRIVATE bcrypt)
endif()
add_test(NAME Portable.ConvexContact.SceneLockPlacement COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/SceneLockPlacementTests.py")
