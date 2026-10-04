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
add_test(NAME ViewerSceneTests COMMAND ViewerSceneTests
    "${PROJECT_SOURCE_DIR}/ViewerScenes/PhysicsStepSmoke/demo.pds.ods"
    "${PROJECT_SOURCE_DIR}/ViewerScenes/BoxBoxTest00c/demo.pds.ods")
set_tests_properties(ViewerSceneTests PROPERTIES TIMEOUT 60)
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

foreach(scene BoxBoxTest00c Pyramid TruckTerrain TypesBreakable)
    add_test(NAME ViewerSmoke${scene}
        COMMAND Viewer --smoke-test "${PROJECT_SOURCE_DIR}/ViewerScenes/${scene}/demo.pds.ods")
    set_tests_properties(ViewerSmoke${scene} PROPERTIES TIMEOUT 60)
endforeach()
add_test(NAME ViewerSmokeSound COMMAND Viewer --smoke-test "${CMAKE_CURRENT_BINARY_DIR}/sound-smoke/demo.pds.ods")
set_tests_properties(ViewerSmokeSound PROPERTIES FIXTURES_REQUIRED SoundScene TIMEOUT 60)
add_test(NAME ViewerPhysicsStep
    COMMAND Viewer --physics-test
        "${PROJECT_SOURCE_DIR}/ViewerScenes/PhysicsStepSmoke/demo.pds.ods"
        FallingBox 8.0 8.5 60)
set_tests_properties(ViewerPhysicsStep PROPERTIES TIMEOUT 60)
add_test(NAME ViewerPhysicsContact
    COMMAND Viewer --physics-test
        "${PROJECT_SOURCE_DIR}/ViewerScenes/PhysicsContactSmoke/demo.pds.ods"
        FallingSphere 0.25 0.35 60)
set_tests_properties(ViewerPhysicsContact PROPERTIES TIMEOUT 60)
