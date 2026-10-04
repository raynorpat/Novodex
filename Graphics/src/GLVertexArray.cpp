// The oracle's Vlist/Dlist internals are replaced by owned indexed arrays.
#include "GeometryPrivate.h"
#include <algorithm>
#include <map>
#include <tuple>

unsigned graphicsSanitizeMode(const GLMmodel *m, unsigned mode) {
  if (!m)
    return GLM_NONE;
  if (!m->facetnorms)
    mode &= ~GLM_FLAT;
  if (!m->normals)
    mode &= ~GLM_SMOOTH;
  if (!m->texcoords)
    mode &= ~GLM_TEXTURE;
  if (!m->materials)
    mode &= ~(GLM_COLOR | GLM_MATERIAL);
  if (mode & GLM_SMOOTH)
    mode &= ~GLM_FLAT;
  if (mode & GLM_MATERIAL)
    mode &= ~GLM_COLOR;
  return mode;
}
void graphicsApplyGroup(const GLMmodel *m, const GLMgroup *g, unsigned mode) {
  if (g && g->texture >= 0)
    glBindTexture(GL_TEXTURE_2D, unsigned(g->texture));
  if (g && m->materials && g->material < m->nummaterials) {
    const auto &a = m->materials[g->material];
    if (mode & GLM_MATERIAL) {
      glDisable(GL_COLOR_MATERIAL);
      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, a.ambient);
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, a.diffuse);
      glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, a.specular);
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, a.emmissive);
      glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, a.shininess);
    } else if (mode & GLM_COLOR) {
      glEnable(GL_COLOR_MATERIAL);
      glColor4fv(a.diffuse);
    }
  }
}
GraphicsIndexedGroup graphicsIndexGroup(const GLMmodel *m, const GLMgroup *g,
                                        unsigned mode) {
  GraphicsIndexedGroup out;
  if (!m || !m->vertices || !m->triangles)
    return out;
  mode = graphicsSanitizeMode(m, mode);
  std::map<std::tuple<unsigned, unsigned, unsigned>, unsigned> seen;
  unsigned count = g ? g->numtriangles : m->numtriangles;
  out.indices.reserve(size_t(count) * 3);
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
    for (int j = 0; j < 3; ++j) {
      unsigned v = t.vindices[j],
               n = (mode & GLM_FLAT)     ? t.findex
                   : (mode & GLM_SMOOTH) ? t.nindices[j]
                                         : 0,
               uv = (mode & GLM_TEXTURE) ? t.tindices[j] : 0;
      if (n > ((mode & GLM_FLAT) ? m->numfacetnorms : m->numnormals))
        n = 0;
      if (uv > m->numtexcoords)
        uv = 0;
      auto key = std::make_tuple(v, n, uv);
      auto found = seen.find(key);
      if (found != seen.end()) {
        out.indices.push_back(found->second);
        continue;
      }
      unsigned next = unsigned(out.vertices.size() / 8);
      seen[key] = next;
      out.indices.push_back(next);
      out.vertices.insert(out.vertices.end(), m->vertices + 3 * v,
                          m->vertices + 3 * v + 3);
      const float *normals = (mode & GLM_FLAT) ? m->facetnorms : m->normals;
      if (n && normals)
        out.vertices.insert(out.vertices.end(), normals + 3 * n,
                            normals + 3 * n + 3);
      else
        out.vertices.insert(out.vertices.end(), {0, 0, 0});
      if (uv && m->texcoords)
        out.vertices.insert(out.vertices.end(), m->texcoords + 2 * uv,
                            m->texcoords + 2 * uv + 2);
      else
        out.vertices.insert(out.vertices.end(), {0, 0});
    }
  }
  return out;
}
void graphicsDrawIndexed(const float *vertices, unsigned nVertices,
                         const unsigned *indices, unsigned count,
                         unsigned mode) {
  if (!graphicsGLContext() || !vertices || !indices || !nVertices || !count)
    return;
  glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
  glEnableClientState(GL_VERTEX_ARRAY);
  if (mode & (GLM_SMOOTH | GLM_FLAT))
    glEnableClientState(GL_NORMAL_ARRAY);
  else
    glDisableClientState(GL_NORMAL_ARRAY);
  if (mode & GLM_TEXTURE)
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
  else
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
  glVertexPointer(3, GL_FLOAT, 32, vertices);
  glNormalPointer(GL_FLOAT, 32, vertices + 3);
  glTexCoordPointer(2, GL_FLOAT, 32, vertices + 6);
  glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, indices);
  glPopClientAttrib();
}
