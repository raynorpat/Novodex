include(FetchContent)
set(GLFW_LIBRARY_TYPE STATIC CACHE STRING "Link the viewer's GLFW dependency statically" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_Declare(glfw
    URL https://codeload.github.com/glfw/glfw/zip/a74efa0d5628b74adc0426af4c5710e287fa7c2c
    URL_HASH SHA256=f14bfe88b244a759e3c6ce0f981acde81a1cb21cbc5b25b0277f2d0120364854
)
FetchContent_Declare(imgui
    URL https://codeload.github.com/ocornut/imgui/zip/6d910d5487d11ca567b61c7824b0c78c569d62f0
    URL_HASH SHA256=138804e5d53207e8ce7ba9f8baf7ba31417b7b4b0549696d7ec913daf38d16c1
)
FetchContent_MakeAvailable(glfw imgui)

# glfwInit maps every Win32 key through ToUnicode to populate glfwGetKeyName.
# The hidden Viewer tests never consume key names, and this OS input-stack scan
# can take over a minute on the headless runner. Leave normal Viewer startup
# unchanged; CTest sets the environment variable only for hidden test runs.
set(NOVODEX_GLFW_WIN32_INIT_SOURCE "${glfw_SOURCE_DIR}/src/win32_init.c")
file(READ "${NOVODEX_GLFW_WIN32_INIT_SOURCE}" NOVODEX_GLFW_WIN32_INIT)
set(NOVODEX_GLFW_KEY_NAME_SCAN_CALL "    _glfwUpdateKeyNamesWin32();")
set(NOVODEX_GLFW_KEY_NAME_SCAN_GUARD
    "    if (!GetEnvironmentVariableA(\"NOVODEX_DISABLE_GLFW_KEY_NAME_SCAN\", disableKeyNameScan, sizeof(disableKeyNameScan)))\n        _glfwUpdateKeyNamesWin32();")
string(FIND "${NOVODEX_GLFW_WIN32_INIT}" "${NOVODEX_GLFW_KEY_NAME_SCAN_GUARD}" NOVODEX_GLFW_KEY_NAME_SCAN_GUARD_POSITION)
string(FIND "${NOVODEX_GLFW_WIN32_INIT}" "    char disableKeyNameScan[2];" NOVODEX_GLFW_KEY_NAME_SCAN_BUFFER_POSITION)
if(NOT NOVODEX_GLFW_KEY_NAME_SCAN_GUARD_POSITION EQUAL -1 AND NOT NOVODEX_GLFW_KEY_NAME_SCAN_BUFFER_POSITION EQUAL -1)
    # Already patched by a previous configure.
elseif(NOT NOVODEX_GLFW_WIN32_INIT MATCHES "NOVODEX_DISABLE_GLFW_KEY_NAME_SCAN")
    string(FIND "${NOVODEX_GLFW_WIN32_INIT}" "${NOVODEX_GLFW_KEY_NAME_SCAN_CALL}" NOVODEX_GLFW_KEY_NAME_SCAN_CALL_POSITION)
    if(NOT NOVODEX_GLFW_KEY_NAME_SCAN_CALL_POSITION EQUAL -1)
        string(REPLACE "int _glfwInitWin32(void)\n{"
            "int _glfwInitWin32(void)\n{\n    char disableKeyNameScan[2];"
            NOVODEX_GLFW_WIN32_INIT "${NOVODEX_GLFW_WIN32_INIT}")
        string(REPLACE "${NOVODEX_GLFW_KEY_NAME_SCAN_CALL}"
            "${NOVODEX_GLFW_KEY_NAME_SCAN_GUARD}"
            NOVODEX_GLFW_WIN32_INIT "${NOVODEX_GLFW_WIN32_INIT}")
        file(WRITE "${NOVODEX_GLFW_WIN32_INIT_SOURCE}" "${NOVODEX_GLFW_WIN32_INIT}")
    else()
        message(FATAL_ERROR "Pinned GLFW Win32 key-name initialization changed; update the smoke-test patch")
    endif()
else()
    message(FATAL_ERROR "Pinned GLFW Win32 key-name initialization changed; update the smoke-test patch")
endif()

add_library(ViewerImGui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl2.cpp
)
target_include_directories(ViewerImGui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_link_libraries(ViewerImGui PUBLIC glfw opengl32)
