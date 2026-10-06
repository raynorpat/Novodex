if(NOT WIN32 OR NOT MSVC OR NOT CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "The viewer requires MSVC targeting Win32 (-A Win32).")
endif()

include(Dependencies.cmake)
add_library(ViewerPlatform STATIC src/ViewerPlatform.cpp)
target_include_directories(ViewerPlatform PUBLIC src)
target_link_libraries(ViewerPlatform PUBLIC ViewerImGui glu32)
add_executable(ViewerPlatformTests ../tests/ViewerPlatformTests.cpp)
target_link_libraries(ViewerPlatformTests PRIVATE ViewerPlatform)
add_test(NAME ViewerPlatformTests COMMAND ViewerPlatformTests)
add_executable(GraphicsRenderingTests ../tests/GraphicsRenderingTests.cpp)
target_link_libraries(GraphicsRenderingTests PRIVATE ViewerPlatform GraphicsLib)
add_test(NAME GraphicsRenderingTests COMMAND GraphicsRenderingTests)
add_executable(GraphicsOffscreenTests ../tests/GraphicsOffscreenTests.cpp)
target_link_libraries(GraphicsOffscreenTests PRIVATE ViewerPlatform GraphicsLib)
add_test(NAME GraphicsOffscreenTests COMMAND GraphicsOffscreenTests)
add_executable(ViewerSoundTests ../tests/ViewerSoundTests.cpp src/Sound.cpp)
target_include_directories(ViewerSoundTests PRIVATE src ../Foundation/include)
target_compile_definitions(ViewerSoundTests PRIVATE INCLUDE_SOUND=1)
target_link_libraries(ViewerSoundTests PRIVATE xaudio2 ole32)
add_test(NAME ViewerSoundTests COMMAND ViewerSoundTests)
add_test(NAME ViewerSoundBackendTests COMMAND ViewerSoundTests --backend)
set_tests_properties(ViewerSoundBackendTests PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/sound-smoke")
add_test(NAME ViewerSoundSceneSetup COMMAND ViewerSoundTests --scene "${CMAKE_CURRENT_BINARY_DIR}/sound-smoke")
set_tests_properties(ViewerSoundSceneSetup PROPERTIES FIXTURES_SETUP SoundScene)

# The obsolete src/glm SDK adapter is superseded by CollisionMeshAdapter.cpp.
set(VIEWER_APPLICATION_SOURCES
    src/Act.cpp src/ActActor.cpp src/ActController.cpp src/ActEffector.cpp
    src/ActJoint.cpp src/ActPicking.cpp src/ActRecorder.cpp src/CarController.cpp
    src/CloneController.cpp src/CollisionMeshAdapter.cpp src/CorpusDemoGraphic.cpp
    src/DebugRenderer.cpp src/FlyController.cpp src/FPSCounter.cpp
    src/PerfDisplay.cpp src/PhysicsDemo.cpp src/ShapeVis.cpp src/TextSpooler.cpp
    src/ViewerGraphicsContext.cpp src/Sound.cpp
)
add_executable(Viewer ${VIEWER_APPLICATION_SOURCES} src/ViewerMain.cpp compiler/vc6/Novodex.rc)
add_executable(ViewerSceneTests ${VIEWER_APPLICATION_SOURCES} ../tests/ViewerSceneTests.cpp)
target_include_directories(ViewerSceneTests PRIVATE
    src compiler/vc6 ../Foundation/include ../Physics/include
)
target_compile_definitions(ViewerSceneTests PRIVATE WIN32 _CONSOLE _CRT_SECURE_NO_WARNINGS INCLUDE_SOUND=1)
target_link_libraries(ViewerSceneTests PRIVATE GraphicsLib NxPhysics NxFoundation
    ViewerPlatform opengl32 glu32 winmm comdlg32 user32 gdi32 xaudio2 ole32
)
add_custom_command(TARGET ViewerSceneTests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:NxPhysics>" "$<TARGET_FILE:NxFoundation>"
        "$<TARGET_FILE_DIR:ViewerSceneTests>"
    VERBATIM
)
# Keep every Viewer run on a disposable copy: the loader can write diagnostic
# dumps next to the scene script while it starts up.
file(GLOB_RECURSE VIEWER_SCENE_DEMOS CONFIGURE_DEPENDS
    "${PROJECT_SOURCE_DIR}/ViewerScenes/*.pds.ods")
list(SORT VIEWER_SCENE_DEMOS)
set(VIEWER_SCENE_SMOKE_ROOT "${CMAKE_CURRENT_BINARY_DIR}/scene-smoke/assets")
add_test(NAME ViewerSceneSmokePrepareAssets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/ViewerScenes" "${VIEWER_SCENE_SMOKE_ROOT}")
set_tests_properties(ViewerSceneSmokePrepareAssets PROPERTIES
    FIXTURES_SETUP ViewerSceneAssets)
add_test(NAME ViewerSceneTests COMMAND ViewerSceneTests
    "${VIEWER_SCENE_SMOKE_ROOT}/PhysicsStepSmoke/demo.pds.ods"
    "${VIEWER_SCENE_SMOKE_ROOT}/BoxBoxTest00c/demo.pds.ods")
set_tests_properties(ViewerSceneTests PROPERTIES
    FIXTURES_REQUIRED ViewerSceneAssets
    RESOURCE_LOCK ViewerSceneSmokeAssets
    TIMEOUT 60)
target_include_directories(Viewer PRIVATE
    src compiler/vc6 ../Foundation/include ../Physics/include
)
target_compile_definitions(Viewer PRIVATE WIN32 _CONSOLE _CRT_SECURE_NO_WARNINGS INCLUDE_SOUND=1)
target_link_libraries(Viewer PRIVATE GraphicsLib NxPhysics NxFoundation
    ViewerPlatform opengl32 glu32 winmm comdlg32 user32 gdi32 xaudio2 ole32
)
target_link_options(Viewer PRIVATE /MAP)
set_target_properties(Viewer PROPERTIES
    OUTPUT_NAME NovodeXPersonal
    VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../ViewerScenes"
)
add_custom_command(TARGET Viewer POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:NxPhysics>" "$<TARGET_FILE:NxFoundation>"
        "$<TARGET_FILE_DIR:Viewer>"
    VERBATIM
)

# Run every checked-in demo through the viewer in an isolated copy. Scene
# startup can emit diagnostic dumps, so smoke runs must not write into the
# source asset tree.
foreach(sceneFile IN LISTS VIEWER_SCENE_DEMOS)
    file(RELATIVE_PATH sceneRelative "${PROJECT_SOURCE_DIR}/ViewerScenes" "${sceneFile}")
    string(REGEX REPLACE "\\.pds\\.ods$" "" sceneTestName "${sceneRelative}")
    string(REGEX REPLACE "[^A-Za-z0-9_]" "_" sceneTestName "${sceneTestName}")
    add_test(NAME ViewerSmokeScene_${sceneTestName}
        COMMAND Viewer --smoke-test "${VIEWER_SCENE_SMOKE_ROOT}/${sceneRelative}")
    set_tests_properties(ViewerSmokeScene_${sceneTestName} PROPERTIES
        FIXTURES_REQUIRED ViewerSceneAssets
        RESOURCE_LOCK ViewerSceneSmokeAssets
        TIMEOUT 30
        ENVIRONMENT "NOVODEX_DISABLE_GLFW_KEY_NAME_SCAN=1")
    # These checked-in scenes already fail with the pinned original DLL: four
    # need a PMap creation path the viewer cannot supply, and TruckDemo has a
    # missing material asset. Keep them in the all-scene inventory and mark
    # only those specific baseline diagnostics as skipped. Any other failure
    # (including a candidate-only crash) remains a failing test.
    if(sceneTestName MATCHES "^(CowPile_demo|PMapTest(8|10|12)_demo)$")
        set_tests_properties(ViewerSmokeScene_${sceneTestName} PROPERTIES
            SKIP_REGULAR_EXPRESSION "TriangleMesh::loadPMap: pmap creation failed!")
    elseif(sceneTestName STREQUAL "TruckDemo_demo")
        set_tests_properties(ViewerSmokeScene_${sceneTestName} PROPERTIES
            SKIP_REGULAR_EXPRESSION "Material #960.mat.ods")
    endif()
endforeach()
add_test(NAME ViewerSmokeSound COMMAND Viewer --smoke-test "${CMAKE_CURRENT_BINARY_DIR}/sound-smoke/demo.pds.ods")
set_tests_properties(ViewerSmokeSound PROPERTIES
    FIXTURES_REQUIRED SoundScene
    TIMEOUT 30
    ENVIRONMENT "NOVODEX_DISABLE_GLFW_KEY_NAME_SCAN=1")
add_test(NAME ViewerPhysicsStep
    COMMAND Viewer --physics-test
        "${VIEWER_SCENE_SMOKE_ROOT}/PhysicsStepSmoke/demo.pds.ods"
        FallingBox 8.0 8.5 60)
set_tests_properties(ViewerPhysicsStep PROPERTIES
    FIXTURES_REQUIRED ViewerSceneAssets
    RESOURCE_LOCK ViewerSceneSmokeAssets
    TIMEOUT 30
    ENVIRONMENT "NOVODEX_DISABLE_GLFW_KEY_NAME_SCAN=1")
add_test(NAME ViewerPhysicsContact
    COMMAND Viewer --physics-test
        "${VIEWER_SCENE_SMOKE_ROOT}/PhysicsContactSmoke/demo.pds.ods"
        FallingSphere 0.25 0.35 60)
set_tests_properties(ViewerPhysicsContact PROPERTIES
    FIXTURES_REQUIRED ViewerSceneAssets
    RESOURCE_LOCK ViewerSceneSmokeAssets
    TIMEOUT 30
    ENVIRONMENT "NOVODEX_DISABLE_GLFW_KEY_NAME_SCAN=1")
