#include "GeometryPrivate.h"
#include "Mesh.h"
#include "Terrain.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace SceneGraph {
namespace {
// Wire layout follows Thatcher Ulrich's public-domain SIGGRAPH 2002 source.
// See reconstruction/Terrain.md for source locations and format details.
class Reader {
public:
  explicit Reader(const char *path) : filename(path ? path : ""), position(0) {
    std::ifstream input(filename, std::ios::binary);
    if (!input)
      fail("cannot open file");
    input.seekg(0, std::ios::end);
    const auto size = input.tellg();
    if (size < 24 || static_cast<unsigned long long>(size) >
                         std::numeric_limits<uint32_t>::max())
      fail("invalid file size");
    input.seekg(0);
    bytes.resize(static_cast<size_t>(size));
    if (!input.read(reinterpret_cast<char *>(bytes.data()), size))
      fail("cannot read file");
  }
  void fail(const char *reason) const {
    throw std::runtime_error("Terrain '" + filename + "': " + reason);
  }
  void seek(size_t offset) {
    if (offset > bytes.size())
      fail("invalid file offset");
    position = offset;
  }
  void require(size_t count, size_t limit) const {
    if (position > limit || limit > bytes.size() || count > limit - position)
      fail("truncated chunk data");
  }
  unsigned u8() {
    require(1, bytes.size());
    return bytes[position++];
  }
  unsigned u16() {
    unsigned a = u8(), b = u8();
    return a | (b << 8);
  }
  int s16() {
    unsigned n = u16();
    return n < 32768 ? int(n) : int(n) - 65536;
  }
  uint32_t u32() {
    uint32_t a = u16(), b = u16();
    return a | (b << 16);
  }
  float f32() {
    uint32_t n = u32();
    float f;
    std::memcpy(&f, &n, 4);
    if (!std::isfinite(f))
      fail("non-finite header value");
    return f;
  }
  std::string filename;
  std::vector<unsigned char> bytes;
  size_t position;
};
struct Node {
  unsigned label, level, x, z;
  int minY, maxY;
  size_t offset, limit;
};
struct Tree {
  Reader reader;
  unsigned version, depth, count;
  float verticalScale, baseDimension, rootDimension;
  std::vector<Node> nodes;
  std::vector<bool> labels;
  size_t tocEnd;
  explicit Tree(const char *path) : reader(path), tocEnd(0) {
    if (reader.u32() != 0x00554843u)
      reader.fail("expected CHU signature");
    version = reader.u16();
    if (version != 8 && version != 9)
      reader.fail("unsupported CHU version (expected 8 or 9)");
    depth = reader.u16();
    float maximumError = reader.f32();
    verticalScale = reader.f32();
    baseDimension = reader.f32();
    count = reader.u32();
    if (depth == 0 || depth > 16 || maximumError < 0 || verticalScale <= 0 ||
        baseDimension <= 0)
      reader.fail("invalid terrain parameters");
    uint64_t expected = 0, levelCount = 1;
    for (unsigned level = 0; level < depth; ++level) {
      expected += levelCount;
      levelCount *= 4;
    }
    if (count != expected || count > (reader.bytes.size() - 24) / 33)
      reader.fail("invalid quadtree chunk count");
    rootDimension = std::ldexp(baseDimension, depth - 1);
    if (!std::isfinite(rootDimension))
      reader.fail("terrain dimensions overflow");
    nodes.reserve(count);
    labels.resize(count);
    readNode(0, 0, 0);
    tocEnd = reader.position;
    if (nodes.size() != count)
      reader.fail("incomplete chunk tree");
    if (version == 9)
      for (const auto &n : nodes)
        if (n.offset < tocEnd)
          reader.fail("chunk offset overlaps table of contents");
  }
  void readNode(unsigned expectedLevel, unsigned expectedX,
                unsigned expectedZ) {
    reader.require(33, reader.bytes.size());
    Node n = {};
    n.label = reader.u32();
    if (n.label >= count || labels[n.label])
      reader.fail("invalid or duplicate chunk label");
    labels[n.label] = true;
    for (unsigned i = 0; i < 4; ++i) {
      unsigned neighbor = reader.u32();
      if (neighbor != ~0u && neighbor >= count)
        reader.fail("invalid neighbor label");
    }
    n.level = reader.u8();
    n.x = reader.u16();
    n.z = reader.u16();
    n.minY = reader.s16();
    n.maxY = reader.s16();
    if (n.level != expectedLevel || n.x != expectedX || n.z != expectedZ ||
        n.minY > n.maxY)
      reader.fail("invalid chunk address or bounds");
    unsigned location = reader.u32();
    if (version == 9) {
      n.offset = location;
      n.limit = reader.bytes.size();
      if (n.offset > n.limit)
        reader.fail("chunk offset beyond file");
    } else {
      n.offset = reader.position;
      reader.require(location, reader.bytes.size());
      n.limit = n.offset + location;
      reader.seek(n.limit);
    }
    nodes.push_back(n);
    if (expectedLevel + 1 < depth)
      for (unsigned child = 0; child < 4; ++child)
        readNode(expectedLevel + 1, expectedX * 2 + (child & 1),
                 expectedZ * 2 + (child >> 1));
  }
  void geometry(std::vector<float> &vertices,
                std::vector<GLMtriangle> &triangles,
                std::vector<float> &texcoords) {
    std::vector<std::pair<size_t, size_t>> ranges;
    for (const auto &node : nodes) {
      reader.seek(node.offset);
      reader.require(2, node.limit);
      unsigned vertexCount = reader.u16();
      reader.require(size_t(vertexCount) * 8 + 8, node.limit);
      const bool leaf = node.level + 1 == depth;
      unsigned base = unsigned(vertices.size() / 3 - 1);
      if (leaf && vertexCount > std::numeric_limits<unsigned>::max() - base)
        reader.fail("too many terrain vertices");
      const float dimension = std::ldexp(baseDimension, depth - 1 - node.level);
      const float centerX = (node.x + .5f) * dimension,
                  centerZ = (node.z + .5f) * dimension;
      const float coordinateScale = (dimension * .5f + .001f) / 16384.f;
      for (unsigned i = 0; i < vertexCount; ++i) {
        int x = reader.s16(), y = reader.s16(), z = reader.s16();
        reader.s16(); // morph delta; full-detail geometry uses stored y
        if (leaf) {
          float px = centerX + x * coordinateScale, py = y * verticalScale,
                pz = centerZ + z * coordinateScale;
          if (!std::isfinite(px) || !std::isfinite(py) || !std::isfinite(pz))
            reader.fail("vertex coordinates overflow");
          vertices.insert(vertices.end(), {px, py, pz});
          texcoords.insert(texcoords.end(),
                           {px / rootDimension, pz / rootDimension});
        }
      }
      unsigned indexCount = reader.u32();
      reader.require(4, node.limit);
      // Compare before multiplying: CHU files and Win32 size_t are both 32 bit.
      if(indexCount>(node.limit-reader.position-4)/2)
        reader.fail("truncated or excessive strip index count");
      unsigned previous[2] = {};
      for (unsigned i = 0; i < indexCount; ++i) {
        unsigned index = reader.u16();
        if (index >= vertexCount)
          reader.fail("vertex index out of bounds");
        if (leaf && i >= 2 && previous[0] != previous[1] &&
            previous[0] != index && previous[1] != index) {
          if (triangles.size() >= std::numeric_limits<unsigned>::max() / 3)
            reader.fail("too many terrain triangles");
          GLMtriangle t = {};
          t.vindices[0] = base + previous[0] + 1;
          t.vindices[1] = base + previous[1] + 1;
          t.vindices[2] = base + index + 1;
          if (i & 1)
            std::swap(t.vindices[0], t.vindices[1]);
          for (unsigned j = 0; j < 3; ++j)
            t.tindices[j] = t.vindices[j];
          triangles.push_back(t);
        }
        previous[0] = previous[1];
        previous[1] = index;
      }
      unsigned declaredTriangles = reader.u32();
      if (declaredTriangles > (indexCount >= 2 ? indexCount - 2 : 0))
        reader.fail("invalid chunk triangle statistics");
      if (version == 8 && reader.position != node.limit)
        reader.fail("invalid inline chunk length");
      ranges.push_back({node.offset, reader.position});
    }
    if (version == 9) {
      std::sort(ranges.begin(), ranges.end());
      for (size_t i = 1; i < ranges.size(); ++i)
        if (ranges[i].first < ranges[i - 1].second)
          reader.fail("overlapping chunk geometry");
    }
    if (triangles.empty())
      reader.fail("terrain has no surface triangles");
  }
};
struct Attributes {
  GLMmodel model = {};
  ~Attributes() {
    std::free(model.normals);
    std::free(model.facetnorms);
    std::free(model.texcoords);
  }
};
} // namespace
void loadTerrain(Mesh &destination, const char *filename, bool textureCoords,
                 float scale, bool forceNormals, unsigned forceTexCoordType) {
  if (!filename || !*filename)
    throw std::runtime_error("Terrain: missing filename");
  std::ifstream input(filename, std::ios::binary);
  char signature[4] = {};
  if (!input)
    throw std::runtime_error(std::string("Terrain '") + filename +
                             "': cannot open file");
  input.read(signature, 4);
  if (std::memcmp(signature, "CHU\0", 4)) {
    std::string path(filename);
    size_t dot = path.find_last_of('.');
    std::string extension = dot == path.npos ? "" : path.substr(dot);
    for (char &c : extension)
      if (c >= 'A' && c <= 'Z')
        c = char(c - 'A' + 'a');
    if (extension == ".chu")
      throw std::runtime_error(std::string("Terrain '") + filename +
                               "': expected CHU signature");
    destination.load(filename, textureCoords, scale, forceNormals,
                     forceTexCoordType);
    return;
  }
  Tree tree(filename);
  std::vector<float> vertices(3, 0), uv(2, 0);
  std::vector<GLMtriangle> triangles;
  tree.geometry(vertices, triangles, uv);
  Attributes attributes;
  auto &model = attributes.model;
  model.vertices = vertices.data();
  model.numvertices = unsigned(vertices.size() / 3 - 1);
  model.triangles = triangles.data();
  model.numtriangles = unsigned(triangles.size());
  if (textureCoords) {
    model.texcoords =
        static_cast<float *>(std::malloc(uv.size() * sizeof(float)));
    if (!model.texcoords)
      throw std::bad_alloc();
    std::copy(uv.begin(), uv.end(), model.texcoords);
    model.numtexcoords = model.numvertices;
  }
  if (scale != 0) {
    glmUnitize(&model);
    glmScale(&model, scale);
  }
  if (forceNormals || forceTexCoordType)
    glmVertexNormals(&model, 90);
  if (textureCoords && forceTexCoordType) {
    if (forceTexCoordType == 1)
      glmSpheremapTexture(&model);
    else
      glmLinearTexture(&model, 5, 5, 2);
  }
  auto indexed = graphicsIndexGroup(&model, nullptr);
  std::unique_ptr<SubMesh> surface(new SubMesh);
  surface->name = "default";
  surface->displayList = ~0u;
  surface->nVertices = unsigned(indexed.vertices.size() / 8);
  surface->nTriangles = unsigned(indexed.indices.size() / 3);
  surface->vertexList = new float[indexed.vertices.size()];
  surface->indexList = new unsigned[indexed.indices.size()];
  std::copy(indexed.vertices.begin(), indexed.vertices.end(),
            surface->vertexList);
  std::copy(indexed.indices.begin(), indexed.indices.end(), surface->indexList);
  Mesh::SubMeshList replacement;
  replacement.push_back(surface.get());
  std::string name(filename);
  for (auto *old : destination.subMeshList)
    delete old;
  destination.subMeshList.swap(replacement);
  surface.release();
  destination.name.swap(name);
  destination.changed();
}
} // namespace SceneGraph
