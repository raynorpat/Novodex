#include "ViewerPlatform.h"
#include <GLFW/glfw3.h>
#include "imgui.h"
#include <cstdio>

static unsigned starts, stops, frames, resizes;
static bool renderedUi = true;
static unsigned keyEvents;
static bool inputPassed = true;
static bool requestCloseOnEscape;
static unsigned closeEvents;
static void key(char code, bool down) {
    if (requestCloseOnEscape) {
        if (code != 27) inputPassed = false;
        ++closeEvents;
        if (down) viewerRequestClose();
        return;
    }
    const char expectedCodes[] = { 28, 28, 127, 127, 27, 27 };
    if (keyEvents >= 6 || code != expectedCodes[keyEvents] || down != (keyEvents % 2 == 0))
        inputPassed = false;
    ++keyEvents;
}
static const char * title() { return "Viewer platform smoke test"; }
static void start(int, char **) {
    ++starts;
    ViewerUi::draw(1,24,0,0,"Scene loading text");
    const int root = ViewerUi::createMenu(0);
    ViewerUi::addMenuEntry("Test action", 7);
    ViewerUi::setMainMenu(root);
}
static void stop() {
    ++stops;
    const ImDrawData * drawData = ImGui::GetDrawData();
    renderedUi = renderedUi && drawData && drawData->TotalVtxCount > 0 && glGetError() == GL_NO_ERROR;
}
static void resize(unsigned w, unsigned h) { if (w && h) ++resizes; }
static void render() {
    ++frames;
    if (frames == 1 || requestCloseOnEscape) {
        GLFWwindow * window = glfwGetCurrentContext();
        const GLFWkeyfun dispatch = glfwSetKeyCallback(window, 0);
        glfwSetKeyCallback(window, dispatch);
        // Exercise the installed ImGui/GLFW callback chain without OS input.
        if (requestCloseOnEscape) {
            dispatch(window, GLFW_KEY_ESCAPE, 0, GLFW_PRESS, 0);
        } else {
            const int keys[] = { GLFW_KEY_INSERT, GLFW_KEY_DELETE, GLFW_KEY_ESCAPE };
            for (int k : keys) {
                dispatch(window, k, 0, GLFW_PRESS, 0);
                dispatch(window, k, 0, GLFW_RELEASE, 0);
            }
        }
    }
    glClear(GL_COLOR_BUFFER_BIT);
    ViewerUi::draw(1, 24, -0.9f, 0.8f, "GLFW + Dear ImGui");
}

int main(int argc, char ** argv)
{
    if (viewerKeyCode(GLFW_KEY_LEFT) != 20 || viewerKeyCode(GLFW_KEY_UP) != 21 ||
        viewerKeyCode(GLFW_KEY_PAGE_DOWN) != 25 || viewerKeyCode(GLFW_KEY_A) != 'a' ||
        viewerKeyCode(GLFW_KEY_ESCAPE) != 27 || viewerKeyCode(GLFW_KEY_UNKNOWN) != 0)
        return 1;
    const ViewerCallbacks callbacks = { title, start, stop, resize, 0, render, key, 0, 0 };
    if (viewerRun(argc, argv, callbacks, true, 3) != 0) return 1;
    if (starts != 1 || stops != 1 || frames != 3 || resizes == 0 ||
        keyEvents != 6 || !inputPassed) return 1;
    // A second lifecycle catches stale menus, callbacks and ImGui contexts.
    if (viewerRun(argc, argv, callbacks, true, 2) != 0) return 1;
    if (starts != 2 || stops != 2 || frames != 5 || !renderedUi) return 1;
    // An application-consumed Escape above must keep running. An explicit
    // quit request must leave the loop and run shutdown with the context alive.
    requestCloseOnEscape = true;
    if (viewerRun(argc, argv, callbacks, true, 10) != 0) return 1;
    if (starts != 3 || stops != 3 || frames != 6 || closeEvents != 2 ||
        !inputPassed || !renderedUi) return 1;
    std::puts("ViewerPlatformTests: hidden GLFW contexts, ImGui frames and shutdown passed");
    return 0;
}
