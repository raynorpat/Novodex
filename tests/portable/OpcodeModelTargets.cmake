# Real vendor sources and genuine Foundation ownership; native packed-node
# pointers and the complete engine migration remain separately guarded.
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4 OR
        NOT (MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    message(FATAL_ERROR "The OPCODE model gate requires Win32: packed pointer migration belongs to Task9")
endif()
set(OPCODE_MODEL_SOURCES OpcodeModelTests.cpp FixtureSupport.cpp SdkAllocatorKernel.cpp
    ../../Physics/src/ThirdPartyHost.cpp
    ../../Foundation/src/FoundationSDK.cpp ../../Foundation/src/Observable.cpp
    ../../Foundation/src/DebugRenderable.cpp ../../Foundation/src/Profiler.cpp
    ../../Foundation/src/Time.cpp ../../Foundation/src/Utilities.cpp ../../Foundation/src/Box.cpp)
set_property(SOURCE OpcodeModelTests.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../docs/reconstruction/novodex-physics/tools/capture_opcode_model.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../docs/reconstruction/novodex-physics/tools/capture_scalar_math.py")
foreach(unit OPC_AABBTree OPC_BaseModel OPC_Collider OPC_Common OPC_MeshInterface OPC_Model
        OPC_OptimizedTree OPC_RayCollider OPC_TreeBuilders)
    list(APPEND OPCODE_MODEL_SOURCES "${ICE_TREE}/${unit}.cpp")
endforeach()
foreach(unit IceAABB IceHPoint IceContainer IceIndexedTriangle IceMatrix3x3 IceMatrix4x4 IcePlane IcePoint
        IceRandom IceRay IceRevisitedRadix IceTriangle IceUtils)
    list(APPEND OPCODE_MODEL_SOURCES "${ICE_TREE}/Ice/${unit}.cpp")
endforeach()
function(nx_opcode_model_target target backend)
    add_executable(${target} ${OPCODE_MODEL_SOURCES})
    target_include_directories(${target} PRIVATE . ../../Physics/src/include ../../Physics/include
        ../../Foundation/include ../../Foundation/src/include ../../Foundation/src "${ICE_TREE}")
    target_compile_definitions(${target} PRIVATE NX_PHYSICS_USE_X87=${backend} NX32=1 WIN32=1
        ICE_NO_DLL OPCODE_EXPORTS USE_MINMAX NXP_DLL_EXPORT= NXF_DLL_EXPORT=)
    target_compile_features(${target} PRIVATE cxx_std_11)
    if(backend)
        target_compile_options(${target} PRIVATE /arch:IA32 /fp:precise /Qfast_transcendentals)
    else()
        target_compile_options(${target} PRIVATE ${NOVODEX_PORTABLE_FP_OPTIONS})
    endif()
endfunction()
nx_opcode_model_target(NxPortableOpcodeModelTests 0)
add_test(NAME Portable.ConvexContact.OpcodeModel COMMAND NxPortableOpcodeModelTests
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/opcode-model-domain-x87.nxpf")
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    nx_opcode_model_target(NxPortableExportOpcodeModel 1)
endif()
