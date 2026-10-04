#include "GeometryPrivate.h"

namespace {
// glDrawElements copies client data when compiled into a native display list.
// Using native list names gives callers the usual glDeleteLists ownership path.
void drawArrays(GLMmodel *m, unsigned requested, GLMgroup *group) {
  if (!graphicsGLContext() || !m)
    return;
  unsigned mode = graphicsSanitizeMode(m, requested);
  glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT |
               GL_CURRENT_BIT);
  if ((requested & GLM_TEXTURE) && !m->texcoords) {
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
  }
  glPushMatrix();
  glTranslatef(m->position[0], m->position[1], m->position[2]);
  auto draw = [&](GLMgroup *g) {
    auto data = graphicsIndexGroup(m, g, mode);
    graphicsApplyGroup(m, g, mode);
    graphicsDrawIndexed(data.vertices.data(),
                        unsigned(data.vertices.size() / 8), data.indices.data(),
                        unsigned(data.indices.size()), mode);
  };
  if (group)
    draw(group);
  else if (m->groups)
    for (auto *g = m->groups; g; g = g->next)
      draw(g);
  else
    draw(nullptr);
  glPopMatrix();
  glPopAttrib();
}
} // namespace
unsigned GLVA_glmList(GLMmodel *m, unsigned mode, GLMgroup *g) {
  if (!m || !graphicsGLContext())
    return 0;
  unsigned id = glGenLists(1);
  if (id) {
    glNewList(id, GL_COMPILE);
    drawArrays(m, mode, g);
    glEndList();
  }
  return id;
}
void GLVA_glmDrawList(GLMmodel *m, unsigned list, unsigned mode, GLMgroup *g) {
  if (!graphicsGLContext())
    return;
  if (list)
    glCallList(list);
  else
    drawArrays(m, mode, g);
}
void GLVA_glmxActivateTexUnit(unsigned unit) {
  if (unit == 0)
    graphicsActivateTexture(0);
}
unsigned GLVA_glmxNumTexUnits() { return 1; }
