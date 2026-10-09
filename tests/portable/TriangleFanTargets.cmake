# Compile the entire Geometry translation unit and its actual vendor closure.
# Public SDK headers select backend0 directly; no parser seam is injected.
if(NOT WIN32 OR NOT (MSVC OR CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC"))
    message(FATAL_ERROR "The full triangle fan gate currently requires Windows with an MSVC-compatible compiler")
endif()
set(TRIANGLE_FAN_SOURCES TriangleFanTests.cpp FixtureSupport.cpp
    ../../Physics/src/Geometry.cpp)
set_property(SOURCE TriangleFanTests.cpp APPEND PROPERTY OBJECT_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../docs/reconstruction/novodex-physics/tools/capture_triangle_fan.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../docs/reconstruction/novodex-physics/tools/capture_scalar_math.py")
foreach(unit IceTriangle IcePoint IcePlane IceMatrix3x3 IceMatrix4x4 IceHPoint IceIndexedTriangle IceRandom IceUtils)
    list(APPEND TRIANGLE_FAN_SOURCES "${ICE_TREE}/Ice/${unit}.cpp")
endforeach()
function(nx_triangle_fan_target target backend)
    add_executable(${target} ${TRIANGLE_FAN_SOURCES})
    target_include_directories(${target} PRIVATE . ../../Physics/src/include ../../Physics/include
        ../../Foundation/include "${ICE_TREE}")
    target_compile_definitions(${target} PRIVATE NX_PHYSICS_USE_X87=${backend} WIN32=1
        ICE_NO_DLL OPCODE_EXPORTS USE_MINMAX NXP_DLL_EXPORT= NXF_DLL_EXPORT=)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_compile_definitions(${target} PRIVATE NX64=1)
    else()
        target_compile_definitions(${target} PRIVATE NX32=1)
    endif()
    target_compile_features(${target} PRIVATE cxx_std_11)
    if(backend)
        target_compile_options(${target} PRIVATE /arch:IA32 /fp:precise /Qfast_transcendentals)
    else()
        target_compile_options(${target} PRIVATE ${NOVODEX_PORTABLE_FP_OPTIONS})
    endif()
endfunction()
nx_triangle_fan_target(NxPortableTriangleFanTests 0)
set(FAN_PREPROCESSED "${CMAKE_CURRENT_BINARY_DIR}/TriangleFanGeometryScalar.i")
separate_arguments(FAN_PREPROCESS_FLAGS NATIVE_COMMAND "${CMAKE_CXX_FLAGS}")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(FAN_POINTER_DEFINE NX64=1)
else()
    set(FAN_POINTER_DEFINE NX32=1)
endif()
file(GLOB_RECURSE FAN_PREPROCESS_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../Physics/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../Physics/src/include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../Foundation/include/*.h"
    "${ICE_TREE}/*.h")
add_custom_command(OUTPUT "${FAN_PREPROCESSED}"
    COMMAND "${CMAKE_CXX_COMPILER}" ${FAN_PREPROCESS_FLAGS} /nologo /P "/Fi${FAN_PREPROCESSED}"
        /DNX_PHYSICS_USE_X87=0 "/D${FAN_POINTER_DEFINE}" /DWIN32=1
        /DICE_NO_DLL /DOPCODE_EXPORTS /DUSE_MINMAX
        "/I${CMAKE_CURRENT_SOURCE_DIR}/../../Physics/src/include"
        "/I${CMAKE_CURRENT_SOURCE_DIR}/../../Physics/include"
        "/I${CMAKE_CURRENT_SOURCE_DIR}/../../Foundation/include" "/I${ICE_TREE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../Physics/src/Geometry.cpp"
    DEPENDS ../../Physics/src/Geometry.cpp ${FAN_PREPROCESS_HEADERS} VERBATIM)
add_custom_target(NxPortableTriangleFanLifetimeSource DEPENDS "${FAN_PREPROCESSED}")
add_dependencies(NxPortableTriangleFanTests NxPortableTriangleFanLifetimeSource)
add_test(NAME Portable.Geometry.TriangleFan COMMAND NxPortableTriangleFanTests
    "${CMAKE_CURRENT_SOURCE_DIR}/fixtures/triangle-fan-nearest-x87.nxpf")
add_test(NAME Portable.Geometry.TriangleFanLifetime COMMAND "${Python3_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_triangle_fan_lifetime.py" "${FAN_PREPROCESSED}")
if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    nx_triangle_fan_target(NxPortableExportTriangleFan 1)
endif()
