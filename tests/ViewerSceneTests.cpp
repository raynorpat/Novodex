#include "ViewerPlatform.h"
#include "PhysicsDemo.h"
#include "ViewerGraphicsContext.h"
#include <cstdio>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <commdlg.h>
#include <dlgs.h>
#include <algorithm>

extern void appStartUp(int, char **);
extern void appShutDown();
extern void appRenderTargSized(unsigned, unsigned);
extern void appTick();
extern void appRender();
extern bool appLoadScene(char *);
extern void fileMenuProc(int);
extern PhysicsDemo * pdemo;

static std::string firstScene, secondScene;
static unsigned ticks, stops;
static bool passed = true;
static const char * title() { return "Viewer scene replacement test"; }
static BOOL CALLBACK findPicker(HWND window, LPARAM result) {
    if (!IsWindowVisible(window)) return TRUE;
    char caption[64] = {};
    GetWindowTextA(window, caption, sizeof(caption));
    if (std::string(caption) != "Load Scene") return TRUE;
    *reinterpret_cast<HWND *>(result) = window;
    return FALSE;
}
static BOOL CALLBACK fillFilename(HWND control, LPARAM filename) {
    char className[64] = {};
    GetClassNameA(control, className, sizeof(className));
    if (std::string(className) == "Edit" &&
        (GetDlgCtrlID(control) == edt1 || GetDlgCtrlID(control) == cmb13))
        SetWindowTextA(control, reinterpret_cast<const char *>(filename));
    return TRUE;
}
static bool usePicker(const char * filename) {
    std::string dialogFilename = filename ? filename : "";
    std::replace(dialogFilename.begin(), dialogFilename.end(), '/', '\\');
    const DWORD uiThread = GetCurrentThreadId();
    bool handled = false;
    // Drive only this test's actual common dialog, without test hooks in the app.
    std::thread dialogInput([&] {
        for (unsigned attempt = 0; attempt < 1000; ++attempt) {
            HWND dialog = 0;
            EnumThreadWindows(uiThread, findPicker, reinterpret_cast<LPARAM>(&dialog));
            if (dialog) {
                if (filename) {
                    SendMessageA(dialog, CDM_SETCONTROLTEXT, edt1, reinterpret_cast<LPARAM>(dialogFilename.c_str()));
                    EnumChildWindows(dialog, fillFilename, reinterpret_cast<LPARAM>(dialogFilename.c_str()));
                }
                PostMessageA(dialog, WM_COMMAND, filename ? IDOK : IDCANCEL, 0);
                handled = true;
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });
    fileMenuProc(1);
    appTick();
    dialogInput.join();
    if (!handled) std::fprintf(stderr, "Scene picker did not appear for %s\n", filename ? filename : "cancel");
    return handled;
}
static bool load(const std::string & path) {
    std::vector<char> filename(path.begin(), path.end());
    filename.push_back(0); // The legacy loader splits its mutable path in place.
    return appLoadScene(filename.data());
}
static void tick() {
    if (ticks == 0) {
        PhysicsDemo * original = pdemo;
        SceneGraph::Object * camera = ViewGC::camera;
        if (appLoadScene(0) || pdemo != original || ViewGC::camera != camera)
            passed = false;
        char originalDirectory[MAX_PATH] = {}, currentDirectory[MAX_PATH] = {};
        GetCurrentDirectoryA(MAX_PATH, originalDirectory);
        if (!usePicker(0) || pdemo != original || ViewGC::camera != camera)
            passed = false;
        GetCurrentDirectoryA(MAX_PATH, currentDirectory);
        if (std::string(originalDirectory) != currentDirectory) passed = false;
        ViewGC::world->bHideSubtree = true;
        ViewGC::cameraEulers.Set(10, 20, 30);
        ViewGC::fov = 75;
        if (!usePicker(secondScene.c_str()) || !pdemo || !pdemo->getAct() ||
            pdemo->getAct()->findBody("FallingBox") ||
            pdemo->getModel("TestBox") || !pdemo->getModel("Box10") ||
            ViewGC::world->bHideSubtree || ViewGC::fov != 45 ||
            ViewGC::cameraEulers.x != 0 || ViewGC::cameraEulers.y != 86 ||
            ViewGC::cameraEulers.z != -23)
            {
            std::fprintf(stderr, "second scene: old actor=%p old model=%p new model=%p hidden=%d fov=%g eulers=%g,%g,%g\n",
                pdemo->getAct()->findBody("FallingBox"), pdemo->getModel("TestBox"),
                pdemo->getModel("Box10"), ViewGC::world->bHideSubtree, ViewGC::fov,
                ViewGC::cameraEulers.x, ViewGC::cameraEulers.y, ViewGC::cameraEulers.z);
            passed = false;
            }
    } else if (ticks == 1) {
        // Scripted scene lights overwrite fixed-function GL state during rendering.
        SceneGraph::Object scriptedLight;
        scriptedLight.setPosition(-5, 4, 1);
        scriptedLight.renderAsLight(0);
        if (!load(firstScene) || !pdemo->getAct()->findBody("FallingBox") ||
            !pdemo->getModel("TestBox") || pdemo->getModel("Box10") ||
            ViewGC::cameraEulers.x != 0 || ViewGC::cameraEulers.y != 0 ||
            ViewGC::cameraEulers.z != 0)
            {
            std::fprintf(stderr, "first scene: actor=%p model=%p old model=%p\n",
                pdemo->getAct()->findBody("FallingBox"), pdemo->getModel("TestBox"), pdemo->getModel("Box10"));
            passed = false;
            }
        GLfloat position[4], diffuse[4], specular[4], exponent;
        glGetLightfv(GL_LIGHT0, GL_POSITION, position);
        glGetLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
        glGetLightfv(GL_LIGHT0, GL_SPECULAR, specular);
        glGetLightfv(GL_LIGHT0, GL_SPOT_EXPONENT, &exponent);
        if (position[0] != 0 || position[1] != 0 || position[2] != 1 ||
            position[3] != 0 || diffuse[0] != 0.5f || specular[0] != 0.4f || exponent != 0) {
            std::fprintf(stderr, "Replacement scene inherited scripted lighting\n");
            passed = false;
        }
    } else {
        fileMenuProc(2); // File > Exit must use normal GLFW shutdown.
    }
    ++ticks;
    appTick();
}
static void stop() {
    appShutDown();
    ++stops;
    if (pdemo || ViewGC::world || ViewGC::camera || ViewGC::pickLine)
        passed = false;
}
int main(int argc, char ** argv) {
    if (argc != 3) return 1;
    firstScene = argv[1];
    secondScene = argv[2];
    char * initialArgs[] = { argv[0], argv[1] };
    const ViewerCallbacks callbacks = { title, appStartUp, stop,
        appRenderTargSized, tick, appRender, 0, 0, 0 };
    if (viewerRun(2, initialArgs, callbacks, true, 10) || ticks != 3 ||
        stops != 1 || !passed) {
        std::fprintf(stderr, "ViewerSceneTests: scene replacement or clean exit failed (ticks=%u stops=%u passed=%d)\n", ticks, stops, passed);
        return 1;
    }
    std::puts("ViewerSceneTests: cancellation, repeated replacement and File Exit passed");
    return 0;
}
