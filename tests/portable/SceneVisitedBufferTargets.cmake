if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4 OR NOT
        (MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    message(FATAL_ERROR "Scene visited buffers require the measured Win32 layout; native layout remains Task9")
endif()
set(SCENE_VISITED_SOURCES SceneVisitedBufferTests.cpp FixtureSupport.cpp SdkAllocatorKernel.cpp
    ../../Physics/src/Containers.cpp ../../Physics/src/ThirdPartyHost.cpp
    ../../Physics/src/opcode/IcePrunable.cpp ../../Physics/src/opcode/IcePruner.cpp
    ../../Foundation/src/FoundationSDK.cpp ../../Foundation/src/Observable.cpp
    ../../Foundation/src/DebugRenderable.cpp ../../Foundation/src/Profiler.cpp
    ../../Foundation/src/Time.cpp ../../Foundation/src/Utilities.cpp ../../Foundation/src/Box.cpp)
foreach(unit OPC_AABBTree OPC_BaseModel OPC_Collider OPC_Common OPC_MeshInterface OPC_Model
        OPC_OptimizedTree OPC_RayCollider OPC_TreeBuilders OPC_AABBCollider OPC_SphereCollider OPC_VolumeCollider)
    list(APPEND SCENE_VISITED_SOURCES "${ICE_TREE}/${unit}.cpp")
endforeach()
foreach(unit IceAABB IceHPoint IceContainer IceIndexedTriangle IceMatrix3x3 IceMatrix4x4 IcePlane
        IcePoint IceRandom IceRay IceRevisitedRadix IceTriangle IceUtils)
    list(APPEND SCENE_VISITED_SOURCES "${ICE_TREE}/Ice/${unit}.cpp")
endforeach()
function(nx_scene_visited_target target backend)
    add_executable(${target} ${SCENE_VISITED_SOURCES})
    target_include_directories(${target} PRIVATE . ../../Physics/src/include ../../Physics/src/opcode
        ../../Physics/include ../../Foundation/include ../../Foundation/src/include ../../Foundation/src "${ICE_TREE}")
    target_compile_definitions(${target} PRIVATE NX_PHYSICS_USE_X87=${backend} NX32=1 WIN32=1
        ICE_NO_DLL OPCODE_EXPORTS USE_MINMAX NXP_DLL_EXPORT= NXF_DLL_EXPORT=)
    target_compile_features(${target} PRIVATE cxx_std_11)
    if(backend)
        target_compile_options(${target} PRIVATE /arch:IA32 /fp:precise /Qfast_transcendentals)
    else()
        target_compile_options(${target} PRIVATE ${NOVODEX_PORTABLE_FP_OPTIONS})
    endif()
endfunction()
nx_scene_visited_target(NxPortableSceneVisitedBufferTests 0)
add_test(NAME Portable.ConvexContact.SceneVisitedBuffers COMMAND NxPortableSceneVisitedBufferTests
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/scene-visited-buffers-x87.nxpf")
add_test(NAME Portable.ConvexContact.SceneVisitedPlacement COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/SceneVisitedBufferPlacementTests.py")
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_executable(NxPortableExportSceneVisitedBuffers ExportShippedSceneVisitedBuffers.cpp)
    target_include_directories(NxPortableExportSceneVisitedBuffers PRIVATE ../../Physics/include ../../Foundation/include)
    target_compile_definitions(NxPortableExportSceneVisitedBuffers PRIVATE NX_PHYSICS_USE_X87=1 NX32=1 WIN32=1 NXP_DLL_EXPORT= NXF_DLL_EXPORT=)
    target_compile_features(NxPortableExportSceneVisitedBuffers PRIVATE cxx_std_11)
    target_compile_options(NxPortableExportSceneVisitedBuffers PRIVATE /arch:IA32 /fp:precise /Qfast_transcendentals)
    set_property(TARGET NxPortableExportSceneVisitedBuffers PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(NxPortableExportSceneVisitedBuffers PRIVATE bcrypt)
endif()
