if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4 OR NOT
        (MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    message(FATAL_ERROR "Pruner registration requires measured fixed32 Win32; native layout remains Task9")
endif()
set(PRUNER_REGISTRATION_SOURCES PrunerRegistrationTests.cpp FixtureSupport.cpp SdkAllocatorKernel.cpp
    ../../Physics/src/PrunerRegistration.cpp
    ../../Physics/src/Containers.cpp ../../Physics/src/ThirdPartyHost.cpp
    ../../Physics/src/opcode/IcePrunable.cpp ../../Physics/src/opcode/IcePruner.cpp
    ../../Foundation/src/FoundationSDK.cpp ../../Foundation/src/Observable.cpp
    ../../Foundation/src/DebugRenderable.cpp ../../Foundation/src/Profiler.cpp
    ../../Foundation/src/Time.cpp ../../Foundation/src/Utilities.cpp ../../Foundation/src/Box.cpp)
foreach(unit OPC_AABBTree OPC_BaseModel OPC_Collider OPC_Common OPC_MeshInterface OPC_Model
        OPC_OptimizedTree OPC_RayCollider OPC_TreeBuilders OPC_AABBCollider OPC_SphereCollider OPC_VolumeCollider)
    list(APPEND PRUNER_REGISTRATION_SOURCES "${ICE_TREE}/${unit}.cpp")
endforeach()
foreach(unit IceAABB IceHPoint IceContainer IceIndexedTriangle IceMatrix3x3 IceMatrix4x4 IcePlane
        IcePoint IceRandom IceRay IceRevisitedRadix IceTriangle IceUtils)
    list(APPEND PRUNER_REGISTRATION_SOURCES "${ICE_TREE}/Ice/${unit}.cpp")
endforeach()
add_executable(NxPortablePrunerRegistrationTests ${PRUNER_REGISTRATION_SOURCES})
target_include_directories(NxPortablePrunerRegistrationTests PRIVATE . ../../Physics/src/include ../../Physics/src/opcode
    ../../Physics/include ../../Foundation/include ../../Foundation/src/include ../../Foundation/src "${ICE_TREE}")
target_compile_definitions(NxPortablePrunerRegistrationTests PRIVATE NX_PHYSICS_USE_X87=0 NX32=1 WIN32=1
    ICE_NO_DLL OPCODE_EXPORTS USE_MINMAX NXP_DLL_EXPORT= NXF_DLL_EXPORT=)
target_compile_features(NxPortablePrunerRegistrationTests PRIVATE cxx_std_11)
target_compile_options(NxPortablePrunerRegistrationTests PRIVATE ${NOVODEX_PORTABLE_FP_OPTIONS})
add_test(NAME Portable.ConvexContact.PrunerRegistration COMMAND NxPortablePrunerRegistrationTests
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/pruner-registration-x87.nxpf")
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_executable(NxPortableExportPrunerRegistration ExportShippedPrunerRegistration.cpp)
    target_include_directories(NxPortableExportPrunerRegistration PRIVATE ../../Physics/include ../../Foundation/include)
    target_compile_definitions(NxPortableExportPrunerRegistration PRIVATE NX_PHYSICS_USE_X87=1 NX32=1 WIN32=1 NXP_DLL_EXPORT= NXF_DLL_EXPORT=)
    target_compile_features(NxPortableExportPrunerRegistration PRIVATE cxx_std_11)
    target_compile_options(NxPortableExportPrunerRegistration PRIVATE /arch:IA32 /fp:precise /Qfast_transcendentals)
    set_property(TARGET NxPortableExportPrunerRegistration PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_link_libraries(NxPortableExportPrunerRegistration PRIVATE bcrypt)
endif()
add_test(NAME Portable.ConvexContact.PrunerRegistrationPlacement COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/PrunerRegistrationPlacementTests.py")
