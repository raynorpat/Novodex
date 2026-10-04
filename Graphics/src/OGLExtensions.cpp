// Context-local lookup supersedes the oracle's global extension-state cache.
#include "GeometryPrivate.h"

bool graphicsGLContext() {
#ifdef _WIN32
  return wglGetCurrentContext() != nullptr;
#else
  return glGetString(GL_VERSION) != nullptr;
#endif
}
namespace {
#ifdef _WIN32
using ActiveTextureProc = void(APIENTRY *)(GLenum);
ActiveTextureProc procedure(const char *core, const char *extension) {
  auto p = wglGetProcAddress(core);
  if (!p || p == reinterpret_cast<PROC>(1) || p == reinterpret_cast<PROC>(2) ||
      p == reinterpret_cast<PROC>(3) || p == reinterpret_cast<PROC>(-1))
    p = wglGetProcAddress(extension);
  if (!p || p == reinterpret_cast<PROC>(1) || p == reinterpret_cast<PROC>(2) ||
      p == reinterpret_cast<PROC>(3) || p == reinterpret_cast<PROC>(-1))
    return nullptr;
  return reinterpret_cast<ActiveTextureProc>(p);
}
#endif
} // namespace
unsigned graphicsTextureUnits() {
  if (!graphicsGLContext())
    return 1;
#ifdef _WIN32
  if (!procedure("glActiveTexture", "glActiveTextureARB"))
    return 1;
#endif
  GLint units = 1;
  glGetIntegerv(0x84e2, &units);
  return units > 0 ? unsigned(units) : 1;
}
void graphicsActivateTexture(unsigned unit) {
  if (!graphicsGLContext() || unit >= graphicsTextureUnits())
    return;
#ifdef _WIN32
  auto active = procedure("glActiveTexture", "glActiveTextureARB");
  auto client = procedure("glClientActiveTexture", "glClientActiveTextureARB");
  if (active)
    active(0x84c0 + unit);
  if (client)
    client(0x84c0 + unit);
#endif
}
