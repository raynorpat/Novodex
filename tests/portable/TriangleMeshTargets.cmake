# Real vendor sources and genuine Foundation ownership; native packed-node
# pointers and the complete engine migration remain separately guarded.
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4 OR
        NOT (MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    message(FATAL_ERROR "The genuine mesh gate requires Win32: packed pointer migration belongs to Task9")
endif()
set(TRIANGLE_MESH_SOURCES TriangleMeshTests.cpp FixtureSupport.cpp SdkAllocatorKernel.cpp
    PhysicsSDKParametersKernel.cpp GenuineFoundationImportSlot.cpp
    ../../Physics/src/TriangleMesh.cpp ../../Physics/src/TriangleMeshPolygons.cpp
    ../../Physics/src/InternalTriangleMesh.cpp ../../Physics/src/PMap.cpp ../../Physics/src/MemoryStream.cpp
    ../../Physics/src/TriangleMeshTopology.cpp ../../Physics/src/ConvexHull.cpp
    ../../Physics/src/IceSupportMaps.cpp ../../Physics/src/IceMeshTools.cpp ../../Physics/src/IceMeshBuilder2.cpp
    ../../Physics/src/EdgeList.cpp ../../Physics/src/IceAdjacencies.cpp ../../Physics/src/SmoothNormals.cpp
    ../../Physics/src/QhullHost.cpp ../../Physics/src/Quantizer.cpp ../../Foundation/src/VolumeIntegration.cpp
    ../../Physics/src/ThirdPartyHost.cpp
    ../../Foundation/src/FoundationSDK.cpp ../../Foundation/src/Observable.cpp
    ../../Foundation/src/DebugRenderable.cpp ../../Foundation/src/Profiler.cpp
    ../../Foundation/src/Time.cpp ../../Foundation/src/Utilities.cpp ../../Foundation/src/Box.cpp)
set_property(SOURCE TriangleMeshTests.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../docs/reconstruction/novodex-physics/tools/capture_opcode_model.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../docs/reconstruction/novodex-physics/tools/capture_scalar_math.py")
foreach(unit OPC_AABBTree OPC_BaseModel OPC_Collider OPC_Common OPC_MeshInterface OPC_Model
        OPC_OptimizedTree OPC_RayCollider OPC_TreeBuilders)
    list(APPEND TRIANGLE_MESH_SOURCES "${ICE_TREE}/${unit}.cpp")
endforeach()
foreach(unit IceAABB IceHPoint IceContainer IceIndexedTriangle IceMatrix3x3 IceMatrix4x4 IcePlane IcePoint
        IceRandom IceRay IceRevisitedRadix IceTriangle IceUtils)
    list(APPEND TRIANGLE_MESH_SOURCES "${ICE_TREE}/Ice/${unit}.cpp")
endforeach()
function(nx_triangle_mesh_target target backend)
    add_executable(${target} ${TRIANGLE_MESH_SOURCES})
    target_include_directories(${target} PRIVATE . ../../Physics/src/include ../../Physics/include
        ../../Foundation/include ../../Foundation/src/include ../../Foundation/src "${ICE_TREE}" "${QHULL_MESH_TREE}")
    target_compile_definitions(${target} PRIVATE NX_PHYSICS_USE_X87=${backend} NX32=1 WIN32=1
        ICE_NO_DLL OPCODE_EXPORTS USE_MINMAX NXP_DLL_EXPORT= NXF_DLL_EXPORT=)
    target_compile_features(${target} PRIVATE cxx_std_11)
    if(backend)
        target_compile_options(${target} PRIVATE /arch:IA32 /fp:precise /Qfast_transcendentals)
    else()
        target_compile_options(${target} PRIVATE ${NOVODEX_PORTABLE_FP_OPTIONS})
    endif()
endfunction()
set(QHULL_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../External/qhull")
set(QHULL_MESH_TREE "${CMAKE_CURRENT_BINARY_DIR}/qhull-mesh-tree")
file(GLOB qhull_mesh_files RELATIVE "${QHULL_SOURCE_ROOT}/upstream/src" "${QHULL_SOURCE_ROOT}/upstream/src/*")
foreach(rel IN LISTS qhull_mesh_files)
    if(NOT IS_DIRECTORY "${QHULL_SOURCE_ROOT}/upstream/src/${rel}")
        if(EXISTS "${QHULL_SOURCE_ROOT}/novodex/${rel}")
            configure_file("${QHULL_SOURCE_ROOT}/novodex/${rel}" "${QHULL_MESH_TREE}/${rel}" COPYONLY)
        else()
            configure_file("${QHULL_SOURCE_ROOT}/upstream/src/${rel}" "${QHULL_MESH_TREE}/${rel}" COPYONLY)
        endif()
    endif()
endforeach()
file(GLOB qhull_overlay RELATIVE "${QHULL_SOURCE_ROOT}/novodex" "${QHULL_SOURCE_ROOT}/novodex/*")
foreach(rel IN LISTS qhull_overlay)
    if(NOT IS_DIRECTORY "${QHULL_SOURCE_ROOT}/novodex/${rel}")
        configure_file("${QHULL_SOURCE_ROOT}/novodex/${rel}" "${QHULL_MESH_TREE}/${rel}" COPYONLY)
    endif()
endforeach()
foreach(unit geom geom2 global io mem merge poly poly2 qhull qset stat user)
    list(APPEND TRIANGLE_MESH_SOURCES "${QHULL_MESH_TREE}/${unit}.c")
endforeach()
nx_triangle_mesh_target(NxPortableTriangleMeshTests 0)
add_test(NAME Portable.ConvexContact.TriangleMesh COMMAND NxPortableTriangleMeshTests
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/triangle-mesh-domain-x87.nxpf")
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    nx_triangle_mesh_target(NxPortableExportTriangleMesh 1)
endif()
