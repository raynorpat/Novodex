#include "Mesh.h"
#include "GeometryPrivate.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>

namespace SceneGraph {
SubMesh::~SubMesh() {
  changed();
  delete[] vertexList;
  delete[] indexList;
}
void SubMesh::changed() {
  if (displayList && displayList != ~0u && graphicsGLContext())
    glDeleteLists(displayList, 1);
  displayList = ~0u;
}
void SubMesh::render() {
  if (!graphicsGLContext() || !vertexList || !indexList || !nTriangles)
    return;
  if (!displayList || displayList == ~0u) {
    displayList = glGenLists(1);
    if (displayList) {
      glNewList(displayList, GL_COMPILE);
      graphicsDrawIndexed(vertexList, nVertices, indexList, nTriangles * 3);
      glEndList();
    }
  }
  if (displayList && displayList != ~0u)
    glCallList(displayList);
  else
    graphicsDrawIndexed(vertexList, nVertices, indexList, nTriangles * 3);
}
void SubMesh::renderAsPoints(float size) {
  if (!graphicsGLContext() || !vertexList || !nVertices)
    return;
  glPushAttrib(GL_ENABLE_BIT | GL_POINT_BIT);
  glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
  if (size > 0)
    glPointSize(size);
  glDisable(GL_LIGHTING);
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, 32, vertexList);
  glDrawArrays(GL_POINTS, 0, nVertices);
  glPopClientAttrib();
  glPopAttrib();
}
void SubMesh::DEBUGassertIntegrity() {
  assert(!nVertices || vertexList);
  assert(!nTriangles || indexList);
  if (indexList)
    for (unsigned i = 0; i < nTriangles * 3; ++i)
      assert(indexList[i] < nVertices);
}
Mesh::Mesh() { bbox.x = bbox.y = bbox.z = 0; }
Mesh::~Mesh() {
  for (auto *s : subMeshList)
    delete s;
}
void Mesh::glmGroupToIndexedList(GLMmodel *m, GLMgroup *g, unsigned &nv,
                                 unsigned &nt, float *&vs, unsigned *&is) {
  auto data = graphicsIndexGroup(m, g);
  nv = unsigned(data.vertices.size() / 8);
  nt = unsigned(data.indices.size() / 3);
  std::unique_ptr<float[]> vertices(nv ? new float[size_t(nv) * 8] : nullptr);
  std::unique_ptr<unsigned[]> indices(nt ? new unsigned[size_t(nt) * 3]
                                         : nullptr);
  if (nv)
    std::copy(data.vertices.begin(), data.vertices.end(), vertices.get());
  if (nt)
    std::copy(data.indices.begin(), data.indices.end(), indices.get());
  vs = vertices.release();
  is = indices.release();
}
void Mesh::load(const char *path, bool textureCoords, float scale,
                bool forceNormals, unsigned forceTexCoordType) {
  if (!path)
    return;
  std::ifstream in(path, std::ios::binary);
  char magic[4] = {};
  in.read(magic, 4);
  bool binary = !std::memcmp(magic, "MESH", 4);
  std::unique_ptr<GLMmodel, decltype(&glmDelete)> m(
      binary ? glmReadBinMesh(path) : glmReadOBJ(path), glmDelete);
  if (!m || !m->numvertices || !m->numtriangles)
    throw std::runtime_error(std::string("Cannot load mesh: ") + path);
  if (scale != 0) {
    glmUnitize(m.get());
    glmScale(m.get(), scale);
  }
  float dims[3];
  glmDimensions(m.get(), dims);
  if (forceNormals) {
    if (!m->facetnorms)
      glmFacetNormals(m.get());
    if (!m->normals)
      glmVertexNormals(m.get(), 90);
  }
  if (forceTexCoordType) {
    if (!m->normals)
      glmVertexNormals(m.get(), 90);
    if (forceTexCoordType == 1)
      glmSpheremapTexture(m.get());
    else
      glmLinearTexture(m.get(), 5, 5, 2);
  }
  if (!textureCoords) {
    std::free(m->texcoords);
    m->texcoords = nullptr;
    m->numtexcoords = 0;
  }
  SubMeshList next;
  try {
    for (auto *g = m->groups; g; g = g->next)
      if (g->numtriangles) {
        std::unique_ptr<SubMesh> s(new SubMesh);
        s->displayList = ~0u;
        s->name = g->name ? g->name : "";
        glmGroupToIndexedList(m.get(), g, s->nVertices, s->nTriangles,
                              s->vertexList, s->indexList);
        next.push_back(s.get());
        s.release();
      }
  } catch (...) {
    for (auto *s : next)
      delete s;
    throw;
  }
  for (auto *s : subMeshList)
    delete s;
  subMeshList.swap(next);
  name = path;
  bbox.x = dims[0];
  bbox.y = dims[1];
  bbox.z = dims[2];
}
void Mesh::render(int index) {
  if (index == -1) {
    for (auto *s : subMeshList)
      if (s)
        s->render();
  } else if (index >= 0 && size_t(index) < subMeshList.size() &&
             subMeshList[index])
    subMeshList[index]->render();
}
int Mesh::findGroup(const char *n) {
  if (n)
    for (size_t i = 0; i < subMeshList.size(); ++i)
      if (subMeshList[i] && subMeshList[i]->name == n)
        return int(i);
  return -1;
}
void Mesh::getBoundingBox(Vec3 &dims) { dims = bbox; }
void Mesh::changed(int index) {
  if (index == -1) {
    for (auto *s : subMeshList)
      if (s)
        s->changed();
  } else if (index >= 0 && size_t(index) < subMeshList.size() &&
             subMeshList[index])
    subMeshList[index]->changed();
  bool any = false;
  float lo[3] = {}, hi[3] = {};
  for (auto *s : subMeshList)
    if (s && s->vertexList)
      for (unsigned i = 0; i < s->nVertices; ++i)
        for (int j = 0; j < 3; ++j) {
          float v = s->vertexList[8 * i + j];
          if (!any && i == 0) {
            lo[j] = hi[j] = v;
          } else {
            lo[j] = std::min(lo[j], v);
            hi[j] = std::max(hi[j], v);
          }
          if (j == 2)
            any = true;
        }
  bbox.x = any ? hi[0] - lo[0] : 0;
  bbox.y = any ? hi[1] - lo[1] : 0;
  bbox.z = any ? hi[2] - lo[2] : 0;
}
void Mesh::collapseToSingleSubmesh() {
  if (subMeshList.size() < 2)
    return;
  std::unique_ptr<SubMesh> merged(new SubMesh);
  merged->displayList = ~0u;
  for (auto *s : subMeshList) {
    if (!s || (s->nVertices && !s->vertexList) ||
        (s->nTriangles && !s->indexList))
      throw std::invalid_argument("Invalid submesh storage");
    if (s->nVertices >
            std::numeric_limits<unsigned>::max() - merged->nVertices ||
        s->nTriangles >
            std::numeric_limits<unsigned>::max() / 3 - merged->nTriangles)
      throw std::length_error("Mesh is too large");
    for (unsigned i = 0; i < s->nTriangles * 3; ++i)
      if (s->indexList[i] >= s->nVertices)
        throw std::invalid_argument("Invalid submesh index");
    merged->nVertices += s->nVertices;
    merged->nTriangles += s->nTriangles;
  }
  merged->name = subMeshList[0]->name;
  merged->userData = subMeshList[0]->userData;
  merged->vertexList = new float[size_t(merged->nVertices) * 8];
  merged->indexList = new unsigned[size_t(merged->nTriangles) * 3];
  unsigned v = 0, t = 0;
  for (auto *s : subMeshList) {
    if (s->nVertices)
      std::copy(s->vertexList, s->vertexList + size_t(s->nVertices) * 8,
                merged->vertexList + size_t(v) * 8);
    for (unsigned i = 0; i < s->nTriangles * 3; ++i)
      merged->indexList[t++] = s->indexList[i] + v;
    v += s->nVertices;
  }
  for (auto *s : subMeshList)
    delete s;
  subMeshList.clear();
  subMeshList.push_back(merged.get());
  merged.release();
}
} // namespace SceneGraph
