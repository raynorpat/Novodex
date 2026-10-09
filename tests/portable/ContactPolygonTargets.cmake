if(NOT WIN32 OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4 OR
        NOT (MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    message(FATAL_ERROR "The genuine contact stream gate requires Win32 layouts; native packing remains Task9")
endif()
set(CONTACT_POLYGON_SOURCES ConvexContactTests.cpp FixtureSupport.cpp SdkAllocatorKernel.cpp
    ContactPairEmitterKernel.cpp ContactPolygonImportSlot.cpp
    ../../Physics/src/ContactPolygon.cpp ../../Physics/src/ContactStream.cpp
    ../../Physics/src/Containers.cpp ../../Physics/src/ThirdPartyHost.cpp
    ../../Foundation/src/FoundationSDK.cpp ../../Foundation/src/Observable.cpp
    ../../Foundation/src/DebugRenderable.cpp ../../Foundation/src/Profiler.cpp
    ../../Foundation/src/Time.cpp ../../Foundation/src/Utilities.cpp ../../Foundation/src/Box.cpp)
foreach(unit IceContainer IceTriangle IcePoint IcePlane IceMatrix3x3 IceMatrix4x4 IceHPoint
        IceIndexedTriangle IceRandom IceUtils)
    list(APPEND CONTACT_POLYGON_SOURCES "${ICE_TREE}/Ice/${unit}.cpp")
endforeach()
function(nx_contact_polygon_target target backend)
    add_executable(${target} ${CONTACT_POLYGON_SOURCES})
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
nx_contact_polygon_target(NxPortableConvexContactTests 0)
add_test(NAME Portable.ConvexContact.Polygon COMMAND NxPortableConvexContactTests
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/contact-polygon-complete-x87.nxpf")
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    nx_contact_polygon_target(NxPortableExportConvexContact 1)
endif()

set_property(SOURCE ConvexContactTests.cpp APPEND PROPERTY OBJECT_DEPENDS
 "${CMAKE_CURRENT_SOURCE_DIR}/../../docs/reconstruction/novodex-physics/tools/capture_contact_polygon.py"
 "${CMAKE_CURRENT_SOURCE_DIR}/../../docs/reconstruction/novodex-physics/tools/capture_scalar_math.py")
