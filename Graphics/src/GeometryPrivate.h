#pragma once
#include "glm.h"
#include <array>
#include <vector>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
#include <GL/gl.h>

bool graphicsGLContext();
unsigned graphicsTextureUnits();
void graphicsActivateTexture(unsigned unit);
unsigned graphicsSanitizeMode(const GLMmodel *, unsigned);
void graphicsApplyGroup(const GLMmodel *, const GLMgroup *, unsigned);
void graphicsDrawModel(GLMmodel *, unsigned, GLMgroup *);
void graphicsDrawIndexed(const float *, unsigned, const unsigned *, unsigned,
                         unsigned mode = GLM_SMOOTH | GLM_TEXTURE);
struct GraphicsIndexedGroup {
  std::vector<float> vertices;
  std::vector<unsigned> indices;
};
GraphicsIndexedGroup graphicsIndexGroup(const GLMmodel *, const GLMgroup *,
                                        unsigned mode = GLM_SMOOTH |
                                                        GLM_TEXTURE);
unsigned GLDL_glmList(GLMmodel *, unsigned, GLMgroup *);
void GLDL_glmDrawList(GLMmodel *, unsigned, unsigned, GLMgroup *);
unsigned GLDL_glmxNumTexUnits();
void GLDL_glmxActivateTexUnit(unsigned);
unsigned GLVA_glmList(GLMmodel *, unsigned, GLMgroup *);
void GLVA_glmDrawList(GLMmodel *, unsigned, unsigned, GLMgroup *);
unsigned GLVA_glmxNumTexUnits();
void GLVA_glmxActivateTexUnit(unsigned);
