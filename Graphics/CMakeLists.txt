# Keep this source list aligned with `lib /list lib/win32/Release/GraphicsLib.lib`.
# The binary archive is an oracle; its VC6 STL ABI is incompatible with modern MSVC.
add_library(GraphicsLib STATIC
    src/Iterator.cpp src/Material.cpp src/Mesh.cpp src/Model.cpp
    src/Object.cpp src/Primitive.cpp src/Scene.cpp src/Texture.cpp
    src/Bounds3d.cpp src/Font.cpp src/glm.cpp src/glmShaderGLDL.cpp
    src/glmShaderGLVA.cpp src/gltxDEVIL.cpp src/gltxSTB.cpp src/glutApp.cpp
    src/GLVertexArray.cpp src/lmemorymanager.cpp src/ODBlock.cpp
    src/OGLExtensions.cpp src/TimeWin.cpp src/Terrain.cpp src/RenderToTexture.cpp src/LegacyGraphics.cpp
)
target_include_directories(GraphicsLib PUBLIC include)
target_include_directories(GraphicsLib PRIVATE third_party/stb)
target_link_libraries(GraphicsLib PUBLIC opengl32 glu32 gdi32 user32)
if(WIN32)
    target_compile_definitions(GraphicsLib PRIVATE WIN32)
endif()

add_executable(GraphicsImageTests ../tests/GraphicsImageTests.cpp)
target_link_libraries(GraphicsImageTests PRIVATE GraphicsLib)
add_test(NAME GraphicsImageTests COMMAND GraphicsImageTests)
add_executable(GraphicsScriptTests ../tests/GraphicsScriptTests.cpp)
target_link_libraries(GraphicsScriptTests PRIVATE GraphicsLib)
file(GLOB_RECURSE GRAPHICS_SCENE_SCRIPTS CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/ViewerScenes/*.ods")
string(REPLACE ";" "\n" GRAPHICS_SCENE_MANIFEST "${GRAPHICS_SCENE_SCRIPTS}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/scene-scripts.txt" "${GRAPHICS_SCENE_MANIFEST}\n")
add_test(NAME GraphicsScriptTests COMMAND GraphicsScriptTests "${CMAKE_CURRENT_BINARY_DIR}/scene-scripts.txt")
add_executable(GraphicsGeometryTests ../tests/GraphicsGeometryTests.cpp)
target_link_libraries(GraphicsGeometryTests PRIVATE GraphicsLib)
target_compile_definitions(GraphicsGeometryTests PRIVATE NOVODEX_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
add_test(NAME GraphicsGeometryTests COMMAND GraphicsGeometryTests)
add_executable(GraphicsSceneGraphTests ../tests/GraphicsSceneGraphTests.cpp)
target_link_libraries(GraphicsSceneGraphTests PRIVATE GraphicsLib)
add_test(NAME GraphicsSceneGraphTests COMMAND GraphicsSceneGraphTests)
add_executable(GraphicsTerrainTests ../tests/GraphicsTerrainTests.cpp)
target_link_libraries(GraphicsTerrainTests PRIVATE GraphicsLib)
add_test(NAME GraphicsTerrainTests COMMAND GraphicsTerrainTests)
add_executable(GraphicsLegacyTests ../tests/GraphicsLegacyTests.cpp)
target_link_libraries(GraphicsLegacyTests PRIVATE GraphicsLib)
add_test(NAME GraphicsLegacyTests COMMAND GraphicsLegacyTests)
if(MSVC)
    target_compile_definitions(GraphicsLib PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

