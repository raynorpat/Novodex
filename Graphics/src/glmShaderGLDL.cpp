#include "GeometryPrivate.h"

void graphicsDrawModel(GLMmodel *m, unsigned mode, GLMgroup *selected) {
  if (!graphicsGLContext() || !m || !m->vertices || !m->triangles)
    return;
  bool sphere = (mode & GLM_TEXTURE) && !m->texcoords;
  mode = graphicsSanitizeMode(m, mode);
  glPushAttrib(GL_ENABLE_BIT | GL_TEXTURE_BIT | GL_LIGHTING_BIT |
               GL_CURRENT_BIT);
  if (sphere) {
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
  }
  glPushMatrix();
  glTranslatef(m->position[0], m->position[1], m->position[2]);
  auto draw = [&](GLMgroup *g) {
    graphicsApplyGroup(m, g, mode);
    glBegin(GL_TRIANGLES);
    unsigned count = g ? g->numtriangles : m->numtriangles;
    for (unsigned i = 0; i < count; ++i) {
      unsigned index = g ? g->triangles[i] : i;
      if (index >= m->numtriangles)
        continue;
      const auto &t = m->triangles[index];
      bool valid = true;
      for (int j = 0; j < 3; ++j)
        valid = valid && t.vindices[j] > 0 && t.vindices[j] <= m->numvertices;
      if (!valid)
        continue;
      if ((mode & GLM_FLAT) && t.findex > 0 && t.findex <= m->numfacetnorms)
        glNormal3fv(m->facetnorms + t.findex * 3);
      for (int j = 0; j < 3; ++j) {
        if ((mode & GLM_SMOOTH) && t.nindices[j] > 0 &&
            t.nindices[j] <= m->numnormals)
          glNormal3fv(m->normals + t.nindices[j] * 3);
        if ((mode & GLM_TEXTURE) && t.tindices[j] > 0 &&
            t.tindices[j] <= m->numtexcoords)
          glTexCoord2fv(m->texcoords + t.tindices[j] * 2);
        glVertex3fv(m->vertices + t.vindices[j] * 3);
      }
    }
    glEnd();
  };
  if (selected)
    draw(selected);
  else if (m->groups)
    for (auto *g = m->groups; g; g = g->next)
      draw(g);
  else
    draw(nullptr);
  glPopMatrix();
  glPopAttrib();
}
void GLDL_glmDrawList(GLMmodel *m, unsigned list, unsigned mode, GLMgroup *g) {
  if (!graphicsGLContext())
    return;
  if (list)
    glCallList(list);
  else
    graphicsDrawModel(m, mode, g);
}
unsigned GLDL_glmList(GLMmodel *m, unsigned mode, GLMgroup *g) {
  if (!graphicsGLContext() || !m)
    return 0;
  unsigned list = glGenLists(1);
  if (list) {
    glNewList(list, GL_COMPILE);
    graphicsDrawModel(m, mode, g);
    glEndList();
  }
  return list;
}
void GLDL_glmxActivateTexUnit(unsigned unit) { graphicsActivateTexture(unit); }
unsigned GLDL_glmxNumTexUnits() { return graphicsTextureUnits(); }
