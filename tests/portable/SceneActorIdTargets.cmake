if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4 OR NOT
        (MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    message(FATAL_ERROR "Scene actor-ID member requires fixed32 Win32; native layout remains Task9")
endif()
add_executable(NxPortableSceneActorIdTests SceneActorIdTests.cpp FixtureSupport.cpp
    ../../Physics/src/SceneActorIdPool.cpp
    ../../Foundation/src/FoundationSDK.cpp ../../Foundation/src/Observable.cpp
    ../../Foundation/src/DebugRenderable.cpp ../../Foundation/src/Profiler.cpp
    ../../Foundation/src/Time.cpp ../../Foundation/src/Utilities.cpp ../../Foundation/src/Box.cpp)
target_include_directories(NxPortableSceneActorIdTests PRIVATE . ../../Physics/src/include
    ../../Physics/include ../../Foundation/include ../../Foundation/src/include ../../Foundation/src)
target_compile_definitions(NxPortableSceneActorIdTests PRIVATE NX_PHYSICS_USE_X87=0 NX32=1 WIN32=1
    NXP_DLL_EXPORT= NXF_DLL_EXPORT=)
target_compile_features(NxPortableSceneActorIdTests PRIVATE cxx_std_14)
target_compile_options(NxPortableSceneActorIdTests PRIVATE ${NOVODEX_PORTABLE_FP_OPTIONS})
add_test(NAME Portable.ConvexContact.SceneActorIds COMMAND NxPortableSceneActorIdTests
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/scene-actor-ids-x87.nxpf")
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_executable(NxPortableExportSceneActorIds ExportShippedSceneActorIds.cpp)
    target_include_directories(NxPortableExportSceneActorIds PRIVATE ../../Physics/include ../../Foundation/include)
    target_compile_definitions(NxPortableExportSceneActorIds PRIVATE NX_PHYSICS_USE_X87=1 NX32=1 WIN32=1 NXP_DLL_EXPORT= NXF_DLL_EXPORT=)
    target_compile_features(NxPortableExportSceneActorIds PRIVATE cxx_std_14)
    target_compile_options(NxPortableExportSceneActorIds PRIVATE /arch:IA32 /fp:precise /Qfast_transcendentals)
    set_property(TARGET NxPortableExportSceneActorIds PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(NxPortableExportSceneActorIds PRIVATE bcrypt)
endif()
add_test(NAME Portable.ConvexContact.SceneActorIdPlacement COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/SceneActorIdPlacementTests.py")
