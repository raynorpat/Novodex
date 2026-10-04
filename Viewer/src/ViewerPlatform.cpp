#include "ViewerPlatform.h"
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

bool keyDown[256] = {};
bool bFramerateOn = false;

namespace {
struct MenuEntry { std::string label; int value; int submenu; };
struct Menu { void (*callback)(int); std::vector<MenuEntry> entries; };
std::vector<Menu> menus;
int currentMenu = -1, mainMenu = -1;
bool popupRequested = false;
bool hiddenWindow = false;
const ViewerCallbacks * app = 0;
int pressedCharacters[GLFW_KEY_LAST + 1] = {};
int lastKey = GLFW_KEY_UNKNOWN;
bool mouseDown[3] = {};
int mouseX = 0, mouseY = 0;

void renderMenu(int id)
{
    if (id < 0 || size_t(id) >= menus.size()) return;
    for (const MenuEntry & entry : menus[id].entries) {
        if (entry.submenu >= 0) {
            if (ImGui::BeginMenu(entry.label.c_str())) {
                renderMenu(entry.submenu);
                ImGui::EndMenu();
            }
        } else if (ImGui::MenuItem(entry.label.c_str()) && menus[id].callback) {
            menus[id].callback(entry.value);
        }
    }
}

void renderMenus()
{
    if (mainMenu < 0) return;
    if (ImGui::BeginMainMenuBar()) {
        renderMenu(mainMenu);
        ImGui::EndMainMenuBar();
    }
    if (popupRequested) {
        ImGui::OpenPopup("Viewer controls");
        popupRequested = false;
    }
    if (ImGui::BeginPopup("Viewer controls")) {
        renderMenu(mainMenu);
        ImGui::EndPopup();
    }
}

void releaseKeys()
{
    for (int i = 1; i < 256; ++i) {
        if (keyDown[i] && app->key) app->key(static_cast<char>(i), false);
        keyDown[i] = false;
    }
    std::fill(pressedCharacters, pressedCharacters + GLFW_KEY_LAST + 1, 0);
    lastKey = GLFW_KEY_UNKNOWN;
}

void onKey(GLFWwindow * window, int key, int, int action, int)
{
    if (key < 0 || key > GLFW_KEY_LAST) return;
    const int code = viewerKeyCode(key);
    if (action == GLFW_RELEASE) {
        if (code) keyDown[code] = false;
        const int character = pressedCharacters[key];
        if (character && app->key) app->key(static_cast<char>(character), false);
        if (character) keyDown[character] = false;
        pressedCharacters[key] = 0;
        return;
    }
    if (ImGui::GetIO().WantCaptureKeyboard) return;
    lastKey = key;
    if (code) keyDown[code] = true;
    // Printable key presses come from the character callback (keyboard layout
    // and Shift included). Controller arrows retain their historical codes.
    if (code && (code < 32 || code == 127)) {
        pressedCharacters[key] = code;
        if (app->key) app->key(static_cast<char>(code), true);
    }
}

void onCharacter(GLFWwindow *, unsigned codepoint)
{
    if (!codepoint || codepoint > 255 || ImGui::GetIO().WantCaptureKeyboard) return;
    if (lastKey >= 0 && lastKey <= GLFW_KEY_LAST)
        pressedCharacters[lastKey] = codepoint;
    keyDown[codepoint] = true;
    if (app->key) app->key(static_cast<char>(codepoint), true);
}

void cursorPixels(GLFWwindow * window, double x, double y, int & px, int & py)
{
    int w, h, fw, fh;
    glfwGetWindowSize(window, &w, &h);
    glfwGetFramebufferSize(window, &fw, &fh);
    px = w ? int(x * fw / w) : 0;
    py = h ? int(y * fh / h) : 0;
}

void onMouseButton(GLFWwindow * window, int button, int action, int)
{
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
        popupRequested = true;
        return;
    }
    const int index = button == GLFW_MOUSE_BUTTON_LEFT ? 0 :
                      button == GLFW_MOUSE_BUTTON_RIGHT ? 1 : -1;
    if (index < 0) return;
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    cursorPixels(window, x, y, mouseX, mouseY);
    if (action == GLFW_RELEASE) {
        if (mouseDown[index] && app->click) app->click(mouseX, mouseY, index + 1, false);
        mouseDown[index] = false;
    } else if (!ImGui::GetIO().WantCaptureMouse &&
               y >= ImGui::GetFrameHeight()) {
        mouseDown[index] = true;
        if (app->click) app->click(mouseX, mouseY, index + 1, true);
    }
}

void onCursor(GLFWwindow * window, double x, double y)
{
    int px, py;
    cursorPixels(window, x, y, px, py);
    if (!ImGui::GetIO().WantCaptureMouse && app->drag) {
        for (int i = 0; i < 2; ++i)
            if (mouseDown[i]) app->drag(px, py, px - mouseX, py - mouseY, i + 1);
    }
    mouseX = px;
    mouseY = py;
}

void onFocus(GLFWwindow *, int focused)
{
    if (focused) return;
    releaseKeys();
    for (int i = 0; i < 2; ++i) {
        if (mouseDown[i] && app->click) app->click(mouseX, mouseY, i + 1, false);
        mouseDown[i] = false;
    }
}

void onResize(GLFWwindow *, int width, int height)
{
    if (width <= 0 || height <= 0) return;
    glViewport(0, 0, width, height);
    if (app->resize) app->resize(width, height);
}
}

int viewerKeyCode(int key)
{
    if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) return 'a' + key - GLFW_KEY_A;
    if (key >= GLFW_KEY_SPACE && key <= GLFW_KEY_GRAVE_ACCENT) return key;
    switch (key) {
    case GLFW_KEY_ENTER: return 13;
    case GLFW_KEY_LEFT: return 20;
    case GLFW_KEY_UP: return 21;
    case GLFW_KEY_RIGHT: return 22;
    case GLFW_KEY_DOWN: return 23;
    case GLFW_KEY_PAGE_UP: return 24;
    case GLFW_KEY_PAGE_DOWN: return 25;
    case GLFW_KEY_ESCAPE: return 27;
    case GLFW_KEY_INSERT: return 28;
    case GLFW_KEY_DELETE: return 127;
    default: return 0;
    }
}

namespace ViewerUi {
int createMenu(void (*callback)(int))
{
    menus.push_back(Menu{ callback, {} });
    return currentMenu = int(menus.size()) - 1;
}
void setMenu(int menu) { currentMenu = menu; }
void setMainMenu(int menu) { mainMenu = menu; }
void addMenuEntry(const char * label, int value)
{
    menus.at(currentMenu).entries.push_back(MenuEntry{ label, value, -1 });
}
void addSubMenu(const char * label, int menu)
{
    menus.at(currentMenu).entries.push_back(MenuEntry{ label, 0, menu });
}
void draw(unsigned, float size, float x, float y, const char * text, const float * color)
{
    if (!text || size <= 0) return;
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    const float pixels = std::max(1.0f, display.y / (2.0f * size));
    const ImVec2 extent = ImGui::GetFont()->CalcTextSizeA(pixels, FLT_MAX, 0, text);
    ImVec2 pos((x + 1) * display.x / 2, (1 - y) * display.y / 2 - extent.y);
    if (x == 1) pos.x = (display.x - extent.x) / 2;
    if (y == 1) pos.y = (display.y - extent.y) / 2;
    const ImU32 tint = color ? ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], 1)) : IM_COL32_WHITE;
    ImGui::GetBackgroundDrawList()->AddText(ImGui::GetFont(), pixels, pos, tint, text);
}
}

void viewerRequestClose()
{
    if (GLFWwindow * window = glfwGetCurrentContext())
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

bool viewerHiddenWindow() { return hiddenWindow; }

int viewerRun(int argc, char ** argv, const ViewerCallbacks & callbacks, bool hidden, unsigned frameLimit)
{
    hiddenWindow = hidden;
    glfwSetErrorCallback([](int code, const char * message) {
        std::fprintf(stderr, "GLFW error %d: %s\n", code, message);
    });
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_VISIBLE, hidden ? GLFW_FALSE : GLFW_TRUE);
    GLFWwindow * window = glfwCreateWindow(1024, 768, callbacks.title ? callbacks.title() : "NovodeX", 0, 0);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);
    // Restore the fixed-function defaults recovered from glutApp::initGLState.
    glClearDepth(1.0);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    const GLfloat ambient[] = {0.2f, 0.2f, 0.2f, 1.0f};
    const GLfloat diffuse[] = {0.5f, 0.5f, 0.5f, 1.0f};
    const GLfloat specular[] = {0.4f, 0.4f, 0.4f, 1.0f};
    const GLfloat black[] = {0.0f, 0.0f, 0.0f, 1.0f};
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
    glLightfv(GL_LIGHT0, GL_AMBIENT, black);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
    glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_FLAT);
    app = &callbacks;
    menus.clear();
    currentMenu = mainMenu = -1;
    popupRequested = false;
    std::fill(keyDown, keyDown + 256, false);
    std::fill(mouseDown, mouseDown + 3, false);
    std::fill(pressedCharacters, pressedCharacters + GLFW_KEY_LAST + 1, 0);
    lastKey = GLFW_KEY_UNKNOWN;
    glfwSetKeyCallback(window, onKey);
    glfwSetCharCallback(window, onCharacter);
    glfwSetMouseButtonCallback(window, onMouseButton);
    glfwSetCursorPosCallback(window, onCursor);
    glfwSetWindowFocusCallback(window, onFocus);
    glfwSetFramebufferSizeCallback(window, onResize);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::GetIO().IniFilename = 0;
    ImGui::StyleColorsDark();
    // Install and chain the application callbacks so both backends see input.
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL2_Init();
    // Scene loading can draw scripted text before the normal render loop starts.
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    if (callbacks.start) callbacks.start(argc, argv);
    ImGui::EndFrame();
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    onResize(window, width, height);
    unsigned frames = 0;
    while (!glfwWindowShouldClose(window) && (!frameLimit || frames < frameLimit)) {
        glfwPollEvents();
        glfwGetFramebufferSize(window, &width, &height);
        if (!width || !height) { glfwWaitEventsTimeout(0.05); continue; }
        ImGui_ImplOpenGL2_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        renderMenus();
        if (ImGui::GetIO().WantCaptureKeyboard) releaseKeys();
        if (callbacks.tick) callbacks.tick();
        if (callbacks.render) callbacks.render();
        ImGui::Render();
        ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        ++frames;
    }
    releaseKeys();
    if (callbacks.stop) callbacks.stop();
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    menus.clear();
    app = 0;
    return 0;
}
