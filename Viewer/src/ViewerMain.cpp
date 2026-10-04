#include "ViewerPlatform.h"
#include <cstring>
#include <cstdio>
#include <cerrno>
#include <cstdlib>
#include <cmath>

extern char * appGetTitle();
extern void appStartUp(int, char **);
extern void appShutDown();
extern void appRenderTargSized(unsigned, unsigned);
extern void appTick();
extern void viewerConfigurePhysicsTest(const char * actorName, float minY, float maxY, unsigned steps);
extern int viewerPhysicsTestResult();
extern void appRender();
extern void appKey(char, bool);
extern void appClick(int, int, int, bool);
extern void appDrag(int, int, int, int, int);

static const char * title() { return appGetTitle(); }

int main(int argc, char ** argv)
{
    const ViewerCallbacks callbacks = { title, appStartUp, appShutDown,
        appRenderTargSized, appTick, appRender, appKey, appClick, appDrag };
    if (argc > 1 && std::strcmp(argv[1], "--smoke-test") == 0) {
        if (argc != 3) {
            std::fprintf(stderr, "Usage: %s --smoke-test SCENE.pds.ods\n", argv[0]);
            return 1;
        }
        char * sceneArgs[] = { argv[0], argv[2] };
        return viewerRun(2, sceneArgs, callbacks, true, 3);
    }
    if (argc > 1 && std::strcmp(argv[1], "--physics-test") == 0) {
        if (argc != 7) {
            std::fprintf(stderr, "Usage: %s --physics-test SCENE.pds.ods ACTOR MIN_Y MAX_Y STEPS\n", argv[0]);
            return 1;
        }
        char * end = 0;
        const float minY = std::strtof(argv[4], &end);
        if (!end || *end || !std::isfinite(minY)) return 1;
        const float maxY = std::strtof(argv[5], &end);
        if (!end || *end || !std::isfinite(maxY) || minY >= maxY) return 1;
        errno = 0;
        const unsigned long steps = std::strtoul(argv[6], &end, 10);
        if (errno || !end || *end || steps == 0 || steps > 100000) return 1;

        viewerConfigurePhysicsTest(argv[3], minY, maxY, static_cast<unsigned>(steps));
        char * sceneArgs[] = { argv[0], argv[2] };
        const int runResult = viewerRun(2, sceneArgs, callbacks, true, static_cast<unsigned>(steps) + 1);
        return runResult ? runResult : viewerPhysicsTestResult();
    }
    return viewerRun(argc, argv, callbacks);
}
