#ifndef NOVODEX_VIEWER_PLATFORM_H
#define NOVODEX_VIEWER_PLATFORM_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <GL/gl.h>
#include <GL/glu.h>

extern bool keyDown[256];
extern bool bFramerateOn;

struct ViewerCallbacks
{
    const char * (*title)();
    void (*start)(int, char **);
    void (*stop)();
    void (*resize)(unsigned, unsigned);
    void (*tick)();
    void (*render)();
    void (*key)(char, bool);
    void (*click)(int, int, int, bool);
    void (*drag)(int, int, int, int, int);
};

int viewerRun(int argc, char ** argv, const ViewerCallbacks & callbacks,
              bool hidden = false, unsigned frameLimit = 0);
int viewerKeyCode(int glfwKey);
void viewerRequestClose();
bool viewerHiddenWindow();

namespace ViewerUi
{
int createMenu(void (*callback)(int));
void setMenu(int menu);
void addMenuEntry(const char * label, int value);
void addSubMenu(const char * label, int menu);
void setMainMenu(int menu);
// Preserve the old normalized HUD coordinates and inverse font-size scale.
void draw(unsigned font, float size, float x, float y, const char * text,
          const float * color = 0);
inline void start() {}
inline void end() {}
inline void shutDown() {}
}
#endif
