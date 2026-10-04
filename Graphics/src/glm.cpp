// Recovered from glm.obj; see reconstruction/geometry-notes.md.
#include "GeometryPrivate.h"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
template <class T> T *allocate(size_t n) {
  if (n > std::numeric_limits<size_t>::max() / sizeof(T))
    throw std::bad_alloc();
  T *p = static_cast<T *>(std::calloc(n, sizeof(T)));
  if (!p && n)
    throw std::bad_alloc();
  return p;
}
char *duplicate(const char *s) {
  if (!s)
    return nullptr;
  size_t n = std::strlen(s) + 1;
  char *p = allocate<char>(n);
  std::memcpy(p, s, n);
  return p;
}
template <class T> T *copy(const std::vector<T> &v) {
  T *p = allocate<T>(v.size());
  if (!v.empty())
    std::memcpy(p, v.data(), v.size() * sizeof(T));
  return p;
}
using Model = std::unique_ptr<GLMmodel, decltype(&glmDelete)>;
Model model(const char *path) {
  Model m(allocate<GLMmodel>(1), glmDelete);
  m->pathname = duplicate(path);
  return m;
}
std::string trim(const std::string &s) {
  auto b = s.find_first_not_of(" \t\r\n");
  return b == s.npos ? std::string()
                     : s.substr(b, s.find_last_not_of(" \t\r\n") - b + 1);
}
std::string directory(const char *path) {
  if (!path)
    return {};
  std::string p(path);
  auto i = p.find_last_of("/\\");
  return i == p.npos ? std::string() : p.substr(0, i + 1);
}
void normalize(float *n) {
  double d = std::sqrt(double(n[0]) * n[0] + double(n[1]) * n[1] +
                       double(n[2]) * n[2]);
  if (d > 0)
    for (int i = 0; i < 3; ++i)
      n[i] = float(n[i] / d);
}
void bounds(GLMmodel *m, float *lo, float *hi) {
  for (int j = 0; j < 3; ++j)
    lo[j] = hi[j] =
        (m && m->vertices && m->numvertices) ? m->vertices[3 + j] : 0;
  if (m && m->vertices)
    for (unsigned i = 1; i <= m->numvertices; ++i)
      for (int j = 0; j < 3; ++j) {
        lo[j] = std::min(lo[j], m->vertices[i * 3 + j]);
        hi[j] = std::max(hi[j], m->vertices[i * 3 + j]);
      }
}
GLMmaterial defaultMaterial(const char *name) {
  GLMmaterial m = {};
  m.name = duplicate(name);
  for (int i = 0; i < 3; ++i) {
    m.diffuse[i] = .8f;
    m.ambient[i] = .2f;
  }
  m.diffuse[3] = m.ambient[3] = m.specular[3] = m.emmissive[3] = 1;
  return m;
}
void readMTL(GLMmodel *m, const std::string &name) {
  std::ifstream in(directory(m->pathname) + name);
  if (!in)
    return;
  std::vector<GLMmaterial> mats;
  if (m->materials)
    mats.assign(m->materials, m->materials + m->nummaterials);
  else
    mats.push_back(defaultMaterial("default"));
  size_t current = 0;
  std::string line;
  while (std::getline(in, line)) {
    line = line.substr(0, line.find('#'));
    std::istringstream row(line);
    std::string cmd;
    row >> cmd;
    if (cmd == "newmtl") {
      std::string n;
      std::getline(row, n);
      n = trim(n);
      if (n.empty())
        continue;
      mats.push_back(defaultMaterial(n.c_str()));
      current = mats.size() - 1;
    } else {
      auto &a = mats[current];
      float *values = cmd == "Ka"   ? a.ambient
                      : cmd == "Kd" ? a.diffuse
                      : cmd == "Ks" ? a.specular
                      : cmd == "Ke" ? a.emmissive
                                    : nullptr;
      if (values)
        row >> values[0] >> values[1] >> values[2];
      else if (cmd == "Ns") {
        float ns = 0;
        if (row >> ns)
          a.shininess = std::max(0.f, std::min(128.f, ns * .128f));
      } else if (cmd == "d" || cmd == "Tr") {
        float alpha = 1;
        if (row >> alpha)
          a.diffuse[3] = a.ambient[3] = a.specular[3] = a.emmissive[3] =
              cmd == "Tr" ? 1 - alpha : alpha;
      }
    }
  }
  GLMmaterial *replacement = copy(mats);
  std::free(m->materials);
  m->materials = replacement;
  m->nummaterials = unsigned(mats.size());
}
unsigned material(GLMmodel *m, const std::string &n) {
  for (unsigned i = 0; i < m->nummaterials; ++i)
    if (m->materials[i].name && n == m->materials[i].name)
      return i;
  return 0;
}
unsigned objIndex(const std::string &s, size_t count,
                  bool deferPositive = false) {
  if (s.empty())
    return 0;
  size_t end = 0;
  long long n = std::stoll(s, &end);
  if (end != s.size() || n == 0)
    throw std::runtime_error("invalid OBJ index");
  long long idx = n > 0 ? n : static_cast<long long>(count) + n + 1;
  if (idx <= 0 || idx > std::numeric_limits<unsigned>::max() ||
      ((!deferPositive || n < 0) && idx > static_cast<long long>(count)))
    throw std::runtime_error("OBJ index out of bounds");
  return unsigned(idx);
}
std::array<unsigned, 3> corner(const std::string &s, size_t v, size_t t,
                               size_t n) {
  std::array<unsigned, 3> c = {};
  auto a = s.find('/');
  if (a == s.npos) {
    c[0] = objIndex(s, v);
    return c;
  }
  auto b = s.find('/', a + 1);
  c[0] = objIndex(s.substr(0, a), v);
  c[1] = objIndex(s.substr(a + 1, b == s.npos ? s.npos : b - a - 1), t, true);
  if (b != s.npos)
    c[2] = objIndex(s.substr(b + 1), n, true);
  if (!c[0])
    throw std::runtime_error("missing vertex");
  return c;
}
void appendTriangle(GLMgroup *g, unsigned index) {
  void *p = std::realloc(g->triangles,
                         (size_t(g->numtriangles) + 1) * sizeof(unsigned));
  if (!p)
    throw std::bad_alloc();
  g->triangles = static_cast<unsigned *>(p);
  g->triangles[g->numtriangles++] = index;
}
struct BinaryReader {
  std::ifstream in;
  size_t remaining = 0;
  explicit BinaryReader(const char *p) : in(p, std::ios::binary) {
    if (!in)
      throw std::runtime_error("open");
    in.seekg(0, std::ios::end);
    auto n = in.tellg();
    if (n < 0)
      throw std::runtime_error("size");
    remaining = size_t(n);
    in.seekg(0);
  }
  void bytes(void *p, size_t n) {
    if (n > remaining || !in.read(static_cast<char *>(p), std::streamsize(n)))
      throw std::runtime_error("truncated mesh");
    remaining -= n;
  }
  unsigned u32() {
    unsigned char b[4];
    bytes(b, 4);
    return unsigned(b[0]) | (unsigned(b[1]) << 8) | (unsigned(b[2]) << 16) |
           (unsigned(b[3]) << 24);
  }
  unsigned index(unsigned max) {
    unsigned char b[3];
    bytes(b, 3);
    unsigned x =
        unsigned(b[0]) | (unsigned(b[1]) << 8) | (unsigned(b[2]) << 16);
    if (x >= max)
      throw std::runtime_error("index");
    return x + 1;
  }
  float f32() {
    unsigned n = u32();
    float f;
    std::memcpy(&f, &n, 4);
    if (!std::isfinite(f))
      throw std::runtime_error("float");
    return f;
  }
  unsigned count(size_t elementBytes) {
    unsigned n = u32();
    if (n > remaining / elementBytes || n > 0xffffff)
      throw std::runtime_error("count");
    return n;
  }
};
struct BinaryWriter {
  std::ofstream out;
  explicit BinaryWriter(const char *p) : out(p, std::ios::binary) {}
  void u32(unsigned n) {
    char b[4] = {char(n), char(n >> 8), char(n >> 16), char(n >> 24)};
    out.write(b, 4);
  }
  void f32(float f) {
    unsigned n;
    std::memcpy(&n, &f, 4);
    u32(n);
  }
  void index(unsigned n) {
    unsigned v = n - 1;
    char b[3] = {char(v), char(v >> 8), char(v >> 16)};
    out.write(b, 3);
  }
};
unsigned currentShader = 0;
} // namespace

GLMgroup *glmFindGroup(GLMmodel *m, const char *name) {
  if (m && name)
    for (auto *g = m->groups; g; g = g->next)
      if (g->name && !std::strcmp(g->name, name))
        return g;
  return nullptr;
}
GLMgroup *glmAddGroup(GLMmodel *m, const char *name) {
  if (!m || !name)
    return nullptr;
  if (auto *g = glmFindGroup(m, name))
    return g;
  auto *g = allocate<GLMgroup>(1);
  try {
    g->name = duplicate(name);
  } catch (...) {
    std::free(g);
    throw;
  }
  g->texture = -1;
  g->next = m->groups;
  m->groups = g;
  ++m->numgroups;
  return g;
}
float glmGetRadius(GLMmodel *m) {
  float extent[3] = {};
  if (m && m->vertices)
    for (unsigned i = 1; i <= m->numvertices; ++i)
      for (int j = 0; j < 3; ++j)
        extent[j] = std::max(extent[j], std::fabs(m->vertices[3 * i + j]));
  return std::sqrt(extent[0] * extent[0] + extent[1] * extent[1] +
                   extent[2] * extent[2]);
}
float glmUnitize(GLMmodel *m) {
  if (!m || !m->vertices || !m->numvertices)
    return 1;
  float lo[3], hi[3];
  bounds(m, lo, hi);
  float d = std::max({hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2]});
  float s = d > 0 ? 2 / d : 1;
  for (unsigned i = 1; i <= m->numvertices; ++i)
    for (int j = 0; j < 3; ++j)
      m->vertices[3 * i + j] =
          (m->vertices[3 * i + j] - (lo[j] + hi[j]) * .5f) * s;
  return s;
}
void glmDimensions(GLMmodel *m, float *d) {
  if (!d)
    return;
  float lo[3], hi[3];
  bounds(m, lo, hi);
  for (int j = 0; j < 3; ++j)
    d[j] = hi[j] - lo[j];
}
void glmScale(GLMmodel *m, float scale) {
  if (m && m->vertices)
    for (unsigned i = 3; i < 3 * (m->numvertices + 1); ++i)
      m->vertices[i] *= scale;
}
void glmReverseWinding(GLMmodel *m) {
  if (!m)
    return;
  for (unsigned i = 0; i < m->numtriangles; ++i) {
    auto &t = m->triangles[i];
    std::swap(t.vindices[0], t.vindices[2]);
    std::swap(t.nindices[0], t.nindices[2]);
    std::swap(t.tindices[0], t.tindices[2]);
  }
  if (m->normals)
    for (unsigned i = 3; i < 3 * (m->numnormals + 1); ++i)
      m->normals[i] = -m->normals[i];
  if (m->facetnorms)
    for (unsigned i = 3; i < 3 * (m->numfacetnorms + 1); ++i)
      m->facetnorms[i] = -m->facetnorms[i];
}
void glmFacetNormals(GLMmodel *m) {
  if (!m)
    return;
  float *normals = allocate<float>(3 * (size_t(m->numtriangles) + 1));
  for (unsigned i = 0; i < m->numtriangles; ++i) {
    auto &t = m->triangles[i];
    t.findex = i + 1;
    bool valid = m->vertices;
    for (unsigned j = 0; j < 3; ++j)
      valid = valid && t.vindices[j] > 0 && t.vindices[j] <= m->numvertices;
    if (!valid)
      continue;
    float *a = m->vertices + 3 * t.vindices[0];
    float *b = m->vertices + 3 * t.vindices[1];
    float *c = m->vertices + 3 * t.vindices[2];
    float u[3], v[3];
    for (int j = 0; j < 3; ++j) {
      u[j] = b[j] - a[j];
      v[j] = c[j] - a[j];
    }
    float *n = normals + 3 * (i + 1);
    n[0] = u[1] * v[2] - u[2] * v[1];
    n[1] = u[2] * v[0] - u[0] * v[2];
    n[2] = u[0] * v[1] - u[1] * v[0];
    normalize(n);
  }
  std::free(m->facetnorms);
  m->facetnorms = normals;
  m->numfacetnorms = m->numtriangles;
}
void glmVertexNormals(GLMmodel *m, float angle) {
  if (!m)
    return;
  if (!m->facetnorms || m->numfacetnorms != m->numtriangles)
    glmFacetNormals(m);
  std::vector<std::vector<std::pair<unsigned, unsigned>>> refs(
      size_t(m->numvertices) + 1);
  for (unsigned i = 0; i < m->numtriangles; ++i)
    for (unsigned j = 0; j < 3; ++j)
      if (m->triangles[i].vindices[j] <= m->numvertices)
        refs[m->triangles[i].vindices[j]].push_back({i, j});
  std::vector<float> normals(3, 0);
  float threshold = std::cos(angle * float(M_PI / 180));
  for (unsigned v = 1; v <= m->numvertices; ++v) {
    auto &r = refs[v];
    if (r.empty())
      continue;
    const float *reference =
        m->facetnorms + 3 * m->triangles[r.back().first].findex;
    float average[3] = {};
    std::vector<bool> smooth(r.size());
    for (size_t i = 0; i < r.size(); ++i) {
      const float *n = m->facetnorms + 3 * m->triangles[r[i].first].findex;
      float dot =
          n[0] * reference[0] + n[1] * reference[1] + n[2] * reference[2];
      smooth[i] = dot > threshold;
      if (smooth[i])
        for (int j = 0; j < 3; ++j)
          average[j] += n[j];
    }
    normalize(average);
    unsigned avg = unsigned(normals.size() / 3);
    bool any = std::find(smooth.begin(), smooth.end(), true) != smooth.end();
    if (any)
      normals.insert(normals.end(), average, average + 3);
    for (size_t i = 0; i < r.size(); ++i) {
      auto &t = m->triangles[r[i].first];
      unsigned n = avg;
      if (!smooth[i]) {
        n = unsigned(normals.size() / 3);
        const float *f = m->facetnorms + 3 * t.findex;
        normals.insert(normals.end(), f, f + 3);
      }
      t.nindices[r[i].second] = n;
    }
  }
  float *p = copy(normals);
  std::free(m->normals);
  m->normals = p;
  m->numnormals = unsigned(normals.size() / 3 - 1);
}
void glmLinearTexture(GLMmodel *m, float fu, float fv, unsigned axis) {
  if (!m)
    return;
  float dims[3];
  glmDimensions(m, dims);
  float d = std::max({dims[0], dims[1], dims[2]});
  float s = d > 0 ? 2 / d : 0;
  float *uv = allocate<float>((size_t(m->numvertices) + 1) * 2);
  for (unsigned i = 1; i <= m->numvertices; ++i) {
    uv[2 * i] = (s * m->vertices[3 * i + (axis + 2) % 3] * fu + 1) * .5f;
    uv[2 * i + 1] = (s * m->vertices[3 * i + (axis + 1) % 3] * fv + 1) * .5f;
  }
  std::free(m->texcoords);
  m->texcoords = uv;
  m->numtexcoords = m->numvertices;
  for (unsigned i = 0; i < m->numtriangles; ++i)
    for (int j = 0; j < 3; ++j)
      m->triangles[i].tindices[j] = m->triangles[i].vindices[j];
}
void glmSpheremapTexture(GLMmodel *m) {
  if (!m)
    return;
  if (!m->normals)
    glmVertexNormals(m, 90);
  float *uv = allocate<float>((size_t(m->numnormals) + 1) * 2);
  for (unsigned i = 1; i <= m->numnormals; ++i) {
    const float *n = m->normals + 3 * i;
    double r = std::sqrt(double(n[1]) * n[1] + double(n[2]) * n[2]);
    if (r > 0) {
      double mag = std::sqrt(r * r + double(n[0]) * n[0]);
      uv[2 * i] = float(
          (std::asin(std::max(-1., std::min(1., n[1] / r))) + M_PI / 2) / M_PI);
      uv[2 * i + 1] =
          float(std::acos(std::max(-1., std::min(1., n[0] / mag))) / M_PI);
    }
  }
  std::free(m->texcoords);
  m->texcoords = uv;
  m->numtexcoords = m->numnormals;
  for (unsigned i = 0; i < m->numtriangles; ++i)
    for (int j = 0; j < 3; ++j)
      m->triangles[i].tindices[j] = m->triangles[i].nindices[j];
}
void glmDelete(GLMmodel *m) {
  if (!m)
    return;
  std::free(m->pathname);
  std::free(m->mtllibname);
  std::free(m->vertices);
  std::free(m->normals);
  std::free(m->texcoords);
  std::free(m->facetnorms);
  std::free(m->triangles);
  if (m->materials)
    for (unsigned i = 0; i < m->nummaterials; ++i)
      std::free(m->materials[i].name);
  std::free(m->materials);
  while (m->groups) {
    auto *g = m->groups;
    m->groups = g->next;
    std::free(g->name);
    std::free(g->triangles);
    std::free(g);
  }
  std::free(m);
}
GLMmodel *glmReadOBJ(const char *filename) {
  if (!filename)
    return nullptr;
  try {
    std::ifstream in(filename);
    if (!in)
      return nullptr;
    auto m = model(filename);
    auto *g = glmAddGroup(m.get(), "default");
    std::vector<float> vs(3, 0), ns(3, 0), ts(2, 0);
    std::vector<GLMtriangle> tris;
    std::string line;
    unsigned mat = 0;
    while (std::getline(in, line)) {
      line = line.substr(0, line.find('#'));
      std::istringstream row(line);
      std::string cmd;
      row >> cmd;
      if (cmd.empty())
        continue;
      if (cmd == "v" || cmd == "vn" || cmd == "vt") {
        float a = 0, b = 0, c = 0;
        if (!(row >> a >> b))
          throw std::runtime_error("vertex");
        if (cmd != "vt" && !(row >> c))
          throw std::runtime_error("vertex");
        if (!std::isfinite(a) || !std::isfinite(b) || !std::isfinite(c))
          throw std::runtime_error("float");
        auto &target = cmd == "v" ? vs : cmd == "vn" ? ns : ts;
        target.push_back(a);
        target.push_back(b);
        if (cmd != "vt")
          target.push_back(c);
      } else if (cmd == "g" || cmd == "o") {
        std::string n;
        std::getline(row, n);
        n = trim(n);
        g = glmAddGroup(m.get(), n.empty() ? "default" : n.c_str());
        g->material = mat;
      } else if (cmd == "mtllib") {
        std::string n;
        std::getline(row, n);
        n = trim(n);
        if (!n.empty()) {
          if (!m->mtllibname)
            m->mtllibname = duplicate(n.c_str());
          readMTL(m.get(), n);
        }
      } else if (cmd == "usemtl") {
        std::string n;
        std::getline(row, n);
        mat = material(m.get(), trim(n));
        if (g->numtriangles && g->material != mat) {
          std::string newName = std::string(g->name) + ":" + trim(n);
          g = glmAddGroup(m.get(), newName.c_str());
        }
        g->material = mat;
      } else if (cmd == "f") {
        std::vector<std::array<unsigned, 3>> corners;
        std::string token;
        while (row >> token)
          corners.push_back(corner(token, vs.size() / 3 - 1, ts.size() / 2 - 1,
                                   ns.size() / 3 - 1));
        if (corners.size() < 3)
          throw std::runtime_error("face");
        for (size_t i = 1; i + 1 < corners.size(); ++i) {
          GLMtriangle t = {};
          size_t ix[3] = {0, i, i + 1};
          for (int j = 0; j < 3; ++j) {
            auto &c = corners[ix[j]];
            t.vindices[j] = c[0];
            t.tindices[j] = c[1];
            t.nindices[j] = c[2];
          }
          appendTriangle(g, unsigned(tris.size()));
          tris.push_back(t);
        }
      }
    }
    // Legacy shipped meshes may retain references after removing an optional
    // stream. Positive attribute references validate after the file; negative
    // references already resolved against the stream size at the face
    // declaration.
    const unsigned texcoordCount = unsigned(ts.size() / 2 - 1),
                   normalCount = unsigned(ns.size() / 3 - 1);
    for (auto &triangle : tris)
      for (unsigned j = 0; j < 3; ++j) {
        if (!texcoordCount)
          triangle.tindices[j] = 0;
        else if (triangle.tindices[j] > texcoordCount)
          throw std::runtime_error("OBJ texture index out of bounds");
        if (!normalCount)
          triangle.nindices[j] = 0;
        else if (triangle.nindices[j] > normalCount)
          throw std::runtime_error("OBJ normal index out of bounds");
      }
    m->vertices = copy(vs);
    m->numvertices = unsigned(vs.size() / 3 - 1);
    if (ns.size() > 3) {
      m->normals = copy(ns);
      m->numnormals = unsigned(ns.size() / 3 - 1);
    }
    if (ts.size() > 2) {
      m->texcoords = copy(ts);
      m->numtexcoords = unsigned(ts.size() / 2 - 1);
    }
    m->triangles = copy(tris);
    m->numtriangles = unsigned(tris.size());
    return m.release();
  } catch (...) {
    return nullptr;
  }
}
void glmWriteOBJ(GLMmodel *m, const char *filename, unsigned mode) {
  if (!m || !filename)
    return;
  std::ofstream out(filename);
  if (!out)
    return;
  mode = graphicsSanitizeMode(m, mode);
  out.precision(9);
  if ((mode & GLM_MATERIAL) && m->mtllibname) {
    out << "mtllib " << m->mtllibname << '\n';
    std::ofstream mtl(directory(filename) + m->mtllibname);
    mtl.precision(9);
    for (unsigned i = 0; i < m->nummaterials; ++i) {
      const auto &a = m->materials[i];
      mtl << "newmtl " << (a.name ? a.name : "default") << '\n';
      const float *vals[] = {a.ambient, a.diffuse, a.specular, a.emmissive};
      const char *cmds[] = {"Ka", "Kd", "Ks", "Ke"};
      for (int j = 0; j < 4; ++j)
        mtl << cmds[j] << ' ' << vals[j][0] << ' ' << vals[j][1] << ' '
            << vals[j][2] << '\n';
      mtl << "Ns " << a.shininess / 0.128f << "\nd " << a.diffuse[3] << "\n\n";
    }
  }
  for (unsigned i = 1; i <= m->numvertices; ++i)
    out << "v " << m->vertices[3 * i] << ' ' << m->vertices[3 * i + 1] << ' '
        << m->vertices[3 * i + 2] << '\n';
  if (mode & GLM_TEXTURE)
    for (unsigned i = 1; i <= m->numtexcoords; ++i)
      out << "vt " << m->texcoords[2 * i] << ' ' << m->texcoords[2 * i + 1]
          << '\n';
  const float *normals = (mode & GLM_SMOOTH) ? m->normals : m->facetnorms;
  unsigned count = (mode & GLM_SMOOTH) ? m->numnormals : m->numfacetnorms;
  if (mode & (GLM_SMOOTH | GLM_FLAT))
    for (unsigned i = 1; i <= count; ++i)
      out << "vn " << normals[3 * i] << ' ' << normals[3 * i + 1] << ' '
          << normals[3 * i + 2] << '\n';
  auto write = [&](unsigned i) {
    if (i >= m->numtriangles)
      return;
    auto &t = m->triangles[i];
    out << 'f';
    for (int j = 0; j < 3; ++j) {
      out << ' ' << t.vindices[j];
      unsigned uv = (mode & GLM_TEXTURE) ? t.tindices[j] : 0;
      unsigned n = (mode & GLM_SMOOTH) ? t.nindices[j]
                   : (mode & GLM_FLAT) ? t.findex
                                       : 0;
      if (uv || n) {
        out << '/';
        if (uv)
          out << uv;
        if (n)
          out << '/' << n;
      }
    }
    out << '\n';
  };
  if (m->groups)
    for (auto *g = m->groups; g; g = g->next) {
      out << "g " << (g->name ? g->name : "default") << '\n';
      if ((mode & GLM_MATERIAL) && g->material < m->nummaterials)
        out << "usemtl " << m->materials[g->material].name << '\n';
      for (unsigned j = 0; j < g->numtriangles; ++j)
        write(g->triangles[j]);
    }
  else
    for (unsigned i = 0; i < m->numtriangles; ++i)
      write(i);
}
void glmWeld(GLMmodel *m, float epsilon) {
  if (!m || !m->vertices || epsilon <= 0)
    return;
  std::vector<float> vs(3, 0);
  std::vector<unsigned> indices(size_t(m->numvertices) + 1);
  for (unsigned i = 1; i <= m->numvertices; ++i) {
    unsigned found = 0;
    for (unsigned j = 1; j < vs.size() / 3; ++j)
      if (std::fabs(vs[j * 3] - m->vertices[i * 3]) < epsilon &&
          std::fabs(vs[j * 3 + 1] - m->vertices[i * 3 + 1]) < epsilon &&
          std::fabs(vs[j * 3 + 2] - m->vertices[i * 3 + 2]) < epsilon) {
        found = j;
        break;
      }
    if (!found) {
      found = unsigned(vs.size() / 3);
      vs.insert(vs.end(), m->vertices + i * 3, m->vertices + i * 3 + 3);
    }
    indices[i] = found;
  }
  for (unsigned i = 0; i < m->numtriangles; ++i)
    for (int j = 0; j < 3; ++j)
      if (m->triangles[i].vindices[j] <= m->numvertices)
        m->triangles[i].vindices[j] = indices[m->triangles[i].vindices[j]];
  float *p = copy(vs);
  std::free(m->vertices);
  m->vertices = p;
  m->numvertices = unsigned(vs.size() / 3 - 1);
}
void glmWriteBinMesh(GLMmodel *m, const char *filename, unsigned mode) {
  if (!m || !filename)
    return;
  mode = graphicsSanitizeMode(m, mode);
  mode &= ~GLM_FLAT;
  BinaryWriter w(filename);
  if (!w.out)
    return;
  w.out.write("MESH", 4);
  w.u32(1);
  w.u32(m->numvertices);
  for (unsigned i = 3; i < (m->numvertices + 1) * 3; ++i)
    w.f32(m->vertices[i]);
  w.u32((mode & GLM_SMOOTH) ? m->numnormals : 0);
  if (mode & GLM_SMOOTH)
    for (unsigned i = 3; i < (m->numnormals + 1) * 3; ++i)
      w.f32(m->normals[i]);
  w.u32((mode & GLM_TEXTURE) ? 1 : 0);
  if (mode & GLM_TEXTURE) {
    w.u32(m->numtexcoords);
    w.u32(2);
    for (unsigned i = 2; i < (m->numtexcoords + 1) * 2; ++i)
      w.f32(m->texcoords[i]);
  }
  w.u32(m->numtriangles);
  w.u32(m->numgroups);
  for (auto *g = m->groups; g; g = g->next) {
    const char *n = g->name ? g->name : "";
    unsigned len = unsigned(std::strlen(n));
    w.u32(len);
    w.out.write(n, len + 1);
    w.u32(g->numtriangles);
    w.u32((mode & GLM_TEXTURE) ? 1 : 0);
    w.u32(1);
    for (unsigned i = 0; i < g->numtriangles; ++i) {
      auto &t = m->triangles[g->triangles[i]];
      for (int j = 0; j < 3; ++j) {
        w.index(t.vindices[j]);
        if (mode & GLM_SMOOTH)
          w.index(t.nindices[j]);
        if (mode & GLM_TEXTURE)
          w.index(t.tindices[j]);
      }
    }
  }
}
GLMmodel *glmReadBinMesh(const char *filename) {
  if (!filename)
    return nullptr;
  try {
    BinaryReader r(filename);
    char magic[4];
    r.bytes(magic, 4);
    if (std::memcmp(magic, "MESH", 4) || r.u32() != 1)
      return nullptr;
    auto m = model(filename);
    m->numvertices = r.count(12);
    m->vertices = allocate<float>((size_t(m->numvertices) + 1) * 3);
    for (unsigned i = 3; i < (m->numvertices + 1) * 3; ++i)
      m->vertices[i] = r.f32();
    m->numnormals = r.count(12);
    if (m->numnormals) {
      m->normals = allocate<float>((size_t(m->numnormals) + 1) * 3);
      for (unsigned i = 3; i < (m->numnormals + 1) * 3; ++i)
        m->normals[i] = r.f32();
    }
    unsigned channels = r.u32();
    if (channels > 1)
      return nullptr;
    if (channels) {
      m->numtexcoords = r.count(8);
      if (r.u32() != 2)
        return nullptr;
      m->texcoords = allocate<float>((size_t(m->numtexcoords) + 1) * 2);
      for (unsigned i = 2; i < (m->numtexcoords + 1) * 2; ++i)
        m->texcoords[i] = r.f32();
    }
    unsigned expected = r.u32();
    if (expected > 0xffffff || expected > r.remaining / 6)
      return nullptr;
    unsigned groups = r.count(17);
    std::vector<GLMtriangle> tris;
    for (unsigned gidx = 0; gidx < groups; ++gidx) {
      unsigned len = r.count(1);
      if (len > 1048576)
        return nullptr;
      std::string name(size_t(len) + 1, '\0');
      r.bytes(&name[0], len + 1);
      if (name[len] != '\0' || std::strlen(name.c_str()) != len)
        return nullptr;
      auto *g = glmAddGroup(m.get(), name.c_str());
      unsigned n = r.u32(), textures = r.u32(), type = r.u32();
      if (textures > 1 || (textures && !channels) || (type != 1 && type != 2) ||
          n > expected - tris.size() || (type == 2 && n % 2))
        return nullptr;
      for (unsigned i = 0; i < n; i += (type == 2 ? 2 : 1)) {
        std::array<unsigned, 3> c[4] = {};
        unsigned corners = type == 2 ? 4 : 3;
        for (unsigned j = 0; j < corners; ++j) {
          c[j][0] = r.index(m->numvertices);
          if (m->numnormals)
            c[j][1] = r.index(m->numnormals);
          if (textures)
            c[j][2] = r.index(m->numtexcoords);
        }
        for (unsigned t = 0; t < (type == 2 ? 2u : 1u); ++t) {
          GLMtriangle tri = {};
          unsigned ix[3] = {0, t ? 2u : 1u, t ? 3u : 2u};
          for (int j = 0; j < 3; ++j) {
            tri.vindices[j] = c[ix[j]][0];
            tri.nindices[j] = c[ix[j]][1];
            tri.tindices[j] = c[ix[j]][2];
          }
          appendTriangle(g, unsigned(tris.size()));
          tris.push_back(tri);
        }
      }
    }
    if (tris.size() != expected)
      return nullptr;
    m->triangles = copy(tris);
    m->numtriangles = expected;
    return m.release();
  } catch (...) {
    return nullptr;
  }
}
unsigned getShaderID(const char *name) {
  return name && !std::strcmp(name, "vertexArray") ? 1 : 0;
}
void glmSetShader(unsigned shader) { currentShader = shader == 1 ? 1 : 0; }
void glmGetShaderInfo(bool &needsDynaMesh) { needsDynaMesh = false; }
void glmDrawList(GLMmodel *m, unsigned list, unsigned mode, GLMgroup *g) {
  if (currentShader)
    GLVA_glmDrawList(m, list, mode, g);
  else
    GLDL_glmDrawList(m, list, mode, g);
}
unsigned glmList(GLMmodel *m, unsigned mode, GLMgroup *g) {
  return currentShader ? GLVA_glmList(m, mode, g) : GLDL_glmList(m, mode, g);
}
unsigned glmxNumTexUnits() {
  return currentShader ? GLVA_glmxNumTexUnits() : GLDL_glmxNumTexUnits();
}
void glmxActivateTexUnit(unsigned unit) {
  if (currentShader)
    GLVA_glmxActivateTexUnit(unit);
  else
    GLDL_glmxActivateTexUnit(unit);
}
