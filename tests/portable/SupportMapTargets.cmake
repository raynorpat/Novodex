# Actual production sources; the kernel boundary excludes only deferred mesh
# normal/pose/build families and leaves full-engine migration guards intact.
if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4 OR
        NOT (MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    message(FATAL_ERROR "The hull gate currently requires a Win32 MSVC-compatible target: raw32 array-cookie/layout migration belongs to Task9")
endif()
set(SUPPORT_MAP_SOURCES SupportMapTests.cpp SdkAllocatorKernel.cpp FixtureSupport.cpp
    ../../Physics/src/IceSupportMaps.cpp ../../Physics/src/ConvexHull.cpp ../../Physics/src/IceMeshTools.cpp
    ../../Physics/src/IceMeshBuilder2.cpp ../../Physics/src/EdgeList.cpp ../../Physics/src/IceAdjacencies.cpp
    "${ICE_TREE}/Ice/IceContainer.cpp" "${ICE_TREE}/Ice/IceRevisitedRadix.cpp"
    "${ICE_TREE}/Ice/IcePlane.cpp" "${ICE_TREE}/Ice/IceTriangle.cpp"
    "${ICE_TREE}/Ice/IceIndexedTriangle.cpp" "${ICE_TREE}/Ice/IceRandom.cpp")
function(nx_support_map_target target backend)
    add_executable(${target} ${SUPPORT_MAP_SOURCES})
    target_include_directories(${target} PRIVATE . ../../Physics/src/include ../../Physics/include
        ../../Foundation/include ../../Foundation/src/include ../../Foundation/src ${ICE_TREE})
    target_compile_definitions(${target} PRIVATE NX_PHYSICS_USE_X87=${backend}
        NX_PHYSICS_HULL_KERNEL_ONLY=1 ICE_NO_DLL OPCODE_EXPORTS USE_MINMAX WIN32=1 NXP_DLL_EXPORT= NXF_DLL_EXPORT=)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_compile_definitions(${target} PRIVATE NX64=1)
    else()
        target_compile_definitions(${target} PRIVATE NX32=1)
    endif()
    target_compile_features(${target} PRIVATE cxx_std_11)
    if(backend)
        target_compile_options(${target} PRIVATE /arch:IA32 /fp:precise /Qfast_transcendentals /FIGeometrySdkHeaderSeam.h)
    else()
        target_compile_options(${target} PRIVATE ${NOVODEX_PORTABLE_FP_OPTIONS} /FIGeometrySdkHeaderSeam.h)
    endif()
endfunction()
nx_support_map_target(NxPortableSupportMapTests 0)
add_test(NAME Portable.ConvexContact.SupportMaps COMMAND NxPortableSupportMapTests "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/support-maps-domain-x87.nxpf")
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    nx_support_map_target(NxPortableExportSupportMaps 1)
endif()
