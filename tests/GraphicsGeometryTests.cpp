#include "Mesh.h"
#include "glm.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <stdexcept>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <GL/gl.h>
#endif

static int failures;
#define CHECK(x)                                                               \
  do {                                                                         \
    if (!(x)) {                                                                \
      std::fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x);                     \
      ++failures;                                                              \
    }                                                                          \
  } while (0)
static bool nearlyEqual(float a, float b) { return std::fabs(a - b) < 0.0001f; }
static void fixture(const char *path, const char *data) {
  std::ofstream(path) << data;
}
#ifdef _WIN32
static void renderTests() {
  HWND window =
      CreateWindowExA(0, "STATIC", "Geometry tests", WS_POPUP, 0, 0, 64, 64,
                      nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
  CHECK(window != nullptr);
  if (!window)
    return;
  HDC dc = GetDC(window);
  PIXELFORMATDESCRIPTOR format = {};
  format.nSize = sizeof(format);
  format.nVersion = 1;
  format.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL;
  format.iPixelType = PFD_TYPE_RGBA;
  format.cColorBits = 24;
  int pixelFormat = ChoosePixelFormat(dc, &format);
  CHECK(pixelFormat != 0);
  if (!pixelFormat || !SetPixelFormat(dc, pixelFormat, &format)) {
    ReleaseDC(window, dc);
    DestroyWindow(window);
    CHECK(false);
    return;
  }
  HGLRC context = wglCreateContext(dc);
  CHECK(context != nullptr);
  if (!context) {
    ReleaseDC(window, dc);
    DestroyWindow(window);
    return;
  }
  CHECK(wglMakeCurrent(dc, context));
  glViewport(0, 0, 64, 64);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glDisable(GL_LIGHTING);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);
  fixture("geometry-render.obj",
          "v -.75 -.75 0\nv .75 -.75 0\nv 0 .75 0\nf 1 2 3\n");
  for (unsigned shader = 0; shader < 2; ++shader) {
    glmSetShader(shader);
    GLMmodel *m = glmReadOBJ("geometry-render.obj");
    CHECK(m);
    unsigned list = glmList(m, GLM_NONE);
    CHECK(list && glIsList(list));
    glmDelete(m);
    glClearColor(1, 1, 1, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1, 0, 0);
    glmDrawList(nullptr, list, GLM_NONE);
    glFinish();
    unsigned char rgb[3] = {};
    glReadPixels(32, 32, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, rgb);
    CHECK(rgb[0] > 240 && rgb[1] < 10 && rgb[2] < 10);
    CHECK(glGetError() == GL_NO_ERROR);
    glDeleteLists(list, 1);
    CHECK(!glIsList(list));
  }
  {
    SceneGraph::Mesh mesh;
    mesh.load("geometry-render.obj", false, 0, false);
    CHECK(mesh.subMeshList.size() == 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0, 1, 0);
    mesh.render();
    glFinish();
    unsigned char rgb[3] = {};
    glReadPixels(32, 32, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, rgb);
    CHECK(rgb[0] < 10 && rgb[1] > 240 && rgb[2] < 10);
    CHECK(glGetError() == GL_NO_ERROR);
    unsigned old = mesh.subMeshList[0]->displayList;
    CHECK(glIsList(old));
    mesh.changed();
    CHECK(!glIsList(old) && mesh.subMeshList[0]->displayList == ~0u);
  }
  glmSetShader(0);
  wglMakeCurrent(nullptr, nullptr);
  wglDeleteContext(context);
  ReleaseDC(window, dc);
  DestroyWindow(window);
  std::remove("geometry-render.obj");
}
#endif
int main() {
  fixture("geometry-fixture.mtl",
          "newmtl red\nKd 1 0 0\nKa .1 .2 .3\nNs 500\nd .5\n");
  fixture("geometry-fixture.obj",
          "mtllib geometry-fixture.mtl\nv 0 0 0\nv 2 0 0\nv 2 2 0\nv 0 2 0\nvt "
          ".25 .5\nvt .75 .5\nvn 0 0 1\nvn 0 0 -1\ng surface\nusemtl red\nf "
          "-4/2/1 -3/1/1 -2/2/1 -1/1/1\ng seam\nf 1/1/2 3/1/2 2/2/2\n");
  GLMmodel *m = glmReadOBJ("geometry-fixture.obj");
  CHECK(m);
  if (m) {
    CHECK(m->numvertices == 4 && m->numtriangles == 3 && m->numnormals == 2);
    GLMgroup *g = glmFindGroup(m, "surface");
    CHECK(g && g->numtriangles == 2 && g->texture == -1);
    CHECK(m->triangles[0].vindices[0] == 1 && m->triangles[0].tindices[0] == 2);
    CHECK(m->materials && m->nummaterials == 2 &&
          nearlyEqual(m->materials[1].diffuse[3], .5f));
    float dims[3];
    glmDimensions(m, dims);
    CHECK(nearlyEqual(dims[0], 2) && nearlyEqual(dims[1], 2) && nearlyEqual(dims[2], 0));
    glmWriteOBJ(m, "geometry-roundtrip.obj",
                GLM_SMOOTH | GLM_TEXTURE | GLM_MATERIAL);
    GLMmodel *r = glmReadOBJ("geometry-roundtrip.obj");
    CHECK(r && r->numtriangles == 3);
    if (r) {
      auto *surface = glmFindGroup(r, "surface");
      CHECK(surface && surface->numtriangles == 2);
      if (surface)
        CHECK(r->triangles[surface->triangles[0]].tindices[0] == 2);
    }
    glmDelete(r);
    glmWriteBinMesh(m, "geometry-roundtrip.mesh", GLM_SMOOTH | GLM_TEXTURE);
    r = glmReadBinMesh("geometry-roundtrip.mesh");
    CHECK(r && r->numvertices == 4 && r->numtriangles == 3 &&
          r->numnormals == 2 && r->numtexcoords == 2);
    glmDelete(r);
    glmFacetNormals(m);
    CHECK(m->numfacetnorms == 3 && nearlyEqual(m->facetnorms[5], 1));
    glmVertexNormals(m, 90);
    CHECK(m->numnormals >= 4);
    glmLinearTexture(m, 2, 3, 1);
    CHECK(m->numtexcoords == 4);
    CHECK(nearlyEqual(glmUnitize(m), 1));
    glmDimensions(m, dims);
    CHECK(nearlyEqual(dims[0], 2));
    glmReverseWinding(m);
    CHECK(m->triangles[0].vindices[0] == 3);
    glmDelete(m);
  }
  SceneGraph::Mesh mesh;
  mesh.load("geometry-fixture.obj", true, 1, false);
  CHECK(mesh.findGroup("surface") >= 0 && mesh.findGroup("absent") < 0);
  if (mesh.findGroup("surface") >= 0) {
    auto *s = mesh.subMeshList[mesh.findGroup("surface")];
    CHECK(s->nVertices == 4);
    CHECK(nearlyEqual(s->vertexList[6], .75f) && nearlyEqual(s->vertexList[7], .5f) &&
          nearlyEqual(s->vertexList[5], 1));
  }
  if (mesh.findGroup("seam") >= 0) {
    auto *s = mesh.subMeshList[mesh.findGroup("seam")];
    CHECK(nearlyEqual(s->vertexList[5], -1));
  }
  unsigned tr = 0;
  for (auto *s : mesh.subMeshList) {
    tr += s->nTriangles;
    for (unsigned i = 0; i < s->nTriangles * 3; ++i)
      CHECK(s->indexList[i] < s->nVertices);
  }
  CHECK(tr == 3);
  mesh.collapseToSingleSubmesh();
  CHECK(mesh.subMeshList.size() == 1 && mesh.subMeshList[0]->nTriangles == 3);
  Vec3 dims;
  mesh.getBoundingBox(dims);
  CHECK(nearlyEqual(dims.x, 2) && nearlyEqual(dims.y, 2));
  fixture("geometry-bad.obj", "v 0 0 0\nf 1 2 3\n");
  CHECK(!glmReadOBJ("geometry-bad.obj"));
  fixture("geometry-optional.obj","v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1/7/4 2/8/5 3/9/6\n");
  m=glmReadOBJ("geometry-optional.obj");CHECK(m && m->numtexcoords==0 && m->numnormals==0);
  if(m){for(unsigned j=0;j<3;++j)CHECK(m->triangles[0].tindices[j]==0 && m->triangles[0].nindices[j]==0);glmDelete(m);}
  fixture("geometry-optional.obj","v 0 0 0\nv 1 0 0\nv 0 1 0\nf 1/1/1 2/1/1 3/1/1\nvt .2 .3\nvn 0 0 1\n");
  m=glmReadOBJ("geometry-optional.obj");CHECK(m && m->numtexcoords==1 && m->numnormals==1);glmDelete(m);
  fixture("geometry-optional.obj","v 0 0 0\nv 1 0 0\nv 0 1 0\nvt .2 .3\nf 1/2 2/1 3/1\n");CHECK(!glmReadOBJ("geometry-optional.obj"));
  fixture("geometry-optional.obj","v 0 0 0\nv 1 0 0\nv 0 1 0\nvn 0 0 1\nf 1//2 2//1 3//1\n");CHECK(!glmReadOBJ("geometry-optional.obj"));
  bool threw = false;
  try {
    mesh.load("geometry-bad.obj");
  } catch (const std::runtime_error &) {
    threw = true;
  }
  CHECK(threw && mesh.subMeshList.size() == 1 &&
        mesh.subMeshList[0]->nTriangles == 3);
  CHECK(!glmReadOBJ(nullptr));
  fixture("geometry-bad.mesh", "MESH");
  CHECK(!glmReadBinMesh("geometry-bad.mesh"));
  // Independent wire-format fixture: MESH v1, four vertices, no normals/UV,
  // two output triangles encoded as one quad. Counts are LE32; indices LE24.
  const unsigned char quad[] = {
      'M', 'E', 'S', 'H', 1,   0,   0,   0,   4,  0, 0, 0, 0, 0, 0, 0, 0, 0,  0,
      0,   0,   0,   0,   0,   0,   0,   0,   64, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,
      0,   64,  0,   0,   0,   64,  0,   0,   0,  0, 0, 0, 0, 0, 0, 0, 0, 64, 0,
      0,   0,   0,   0,   0,   0,   0,   0,   0,  0, 0, 2, 0, 0, 0, 1, 0, 0,  0,
      4,   0,   0,   0,   'q', 'u', 'a', 'd', 0,  2, 0, 0, 0, 0, 0, 0, 0, 2,  0,
      0,   0,   0,   0,   0,   1,   0,   0,   2,  0, 0, 3, 0, 0};
  {
    std::ofstream out("geometry-quad.mesh", std::ios::binary);
    out.write(reinterpret_cast<const char *>(quad), sizeof(quad));
  }
  m = glmReadBinMesh("geometry-quad.mesh");
  CHECK(m && m->numtriangles == 2 && m->numvertices == 4);
  if (m) {
    CHECK(m->triangles[1].vindices[0] == 1 &&
          m->triangles[1].vindices[1] == 3 && m->triangles[1].vindices[2] == 4);
    glmDelete(m);
  }
  for (size_t length = 0; length < sizeof(quad); ++length) {
    {
      std::ofstream out("geometry-bad.mesh", std::ios::binary);
      out.write(reinterpret_cast<const char *>(quad), length);
    }
    m = glmReadBinMesh("geometry-bad.mesh");
    CHECK(!m);
    glmDelete(m);
  }
  fixture("geometry-weld.obj",
          "v 0 0 0\nv .000001 0 0\nv 1 0 0\nv 0 1 0\nf 1 3 4\nf 2 3 4\n");
  m = glmReadOBJ("geometry-weld.obj");
  CHECK(m);
  if (m) {
    glmWeld(m, .00001f);
    CHECK(m->numvertices == 3 &&
          m->triangles[0].vindices[0] == m->triangles[1].vindices[0]);
    glmDelete(m);
  }
#ifdef NOVODEX_SOURCE_DIR
  const char* missingUVAssets[] = {
    NOVODEX_SOURCE_DIR "/ViewerScenes/BoxBoxTest00c/Box10.obj",
    NOVODEX_SOURCE_DIR "/ViewerScenes/BoxBoxTest00d/Box10.obj",
    NOVODEX_SOURCE_DIR "/ViewerScenes/Character/Box10.obj",
    NOVODEX_SOURCE_DIR "/ViewerScenes/Funnel/Box10.obj"
  };
  for (auto path : missingUVAssets) {
    m=glmReadOBJ(path);CHECK(m && m->numtriangles>0 && m->numtexcoords==0);
    if(m) {for(unsigned i=0;i<m->numtriangles;++i) for(unsigned j=0;j<3;++j) CHECK(m->triangles[i].tindices[j]==0);glmDelete(m);}
  }
  m = glmReadOBJ(NOVODEX_SOURCE_DIR "/ViewerScenes/CowPile/Quad.obj");
  CHECK(m && m->numvertices == 4 && m->numtriangles == 2);
  glmDelete(m);
  m = glmReadOBJ(NOVODEX_SOURCE_DIR "/ViewerScenes/CowPile/cow.obj");
  CHECK(m && m->numtriangles > 100);
  glmDelete(m);
#endif
  CHECK(getShaderID("displayList") == 0 && getShaderID("vertexArray") == 1 &&
        getShaderID("missing") == 0);
  bool dynamic = true;
  glmSetShader(1);
  glmGetShaderInfo(dynamic);
  CHECK(!dynamic && glmxNumTexUnits() == 1);
  glmSetShader(0);
  const char *files[] = {"geometry-fixture.mtl",   "geometry-fixture.obj",
                         "geometry-roundtrip.obj", "geometry-roundtrip.mesh",
                         "geometry-bad.obj",       "geometry-bad.mesh",
                         "geometry-weld.obj",      "geometry-quad.mesh",
                         "geometry-optional.obj"};
  for (auto f : files)
    std::remove(f);
#ifdef _WIN32
  renderTests();
#endif
  return failures ? 1 : 0;
}
