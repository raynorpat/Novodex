# Geometry reconstruction

Source oracle: the shipped VC6 GraphicsLib archive, objects extracted unchanged
into the task's `graphics-oracle` directory. All six geometry objects were opened
with IDA and all discovered functions exported to the adjacent `*-oracle.txt`
files. Addresses below are object-local effective addresses, not linked RVAs.

## Recovered layout and behavior

`glm.obj`: `glmAddGroup` at 0x260 prepends groups, defaults material to zero and
texture to -1. GLM coordinates/normals/facet normals/UVs use one-based indices;
triangles and group triangle references use zero-based indices. On VC6 the model
is 0x4c bytes, group 0x18 bytes, triangle 0x28 bytes, material 0x48 bytes. The
public structs remain intact; pointer widths naturally change on modern builds.

`glmGetRadius` at 0x360 returns the length of the componentwise maximum absolute
vertex coordinates. `glmUnitize` at 0x430 centers the bounds and scales their
longest dimension to 2. Linear texture generation at 0xf00 scales using the
longest dimension; U uses `(axis+2)%3`, V uses `(axis+1)%3`. The supplied factors
are applied before adding 1 and dividing by 2. Spheremap at 0x1064 maps normals
with poles on X. Vertex-normal smoothing at 0xa88 uses the most recently added
triangle at each vertex as its reference and compares dot products strictly to
cos(angle). MTL defaults diffuse .8, ambient .2, specular zero, alpha 1 and
shininess zero; Ns is mapped from 0..1000 to 0..128.

Shader table at 0xc contains `displayList` ID 0 and `vertexArray` ID 1; default
is displayList. Both table entries have needsDynaMesh false. Unknown shader name
returns 0. GLDL texture-unit count queries GL_MAX_TEXTURE_UNITS_ARB if extension
loading succeeds, otherwise 1. GLVA exposes one unit. IDs outside the valid
range safely select displayList rather than indexing outside the table.

`Mesh.obj`: load at 0x3c detects binary by its first byte `M`; a nonzero Scale
first unitizes, then scales. Missing requested normals are generated with 90
degree smoothing. forceTexCoordType 1 selects spheremap; other nonzero values
select linear factors 5,5 with axis 2. Empty groups are skipped. Conversion at
0x9cc keys vertices on all three independent indices (position/normal/UV), with
eight float components per output vertex, and 32-bit zero-based triangle
indices. changed at 0x72c deletes a list and sets the sentinel to 0xffffffff.
The supplied constructor defaults displayList to zero; the modern renderer
accepts both zero and the sentinel as not compiled. Array ownership uses new[]
and delete[].

## Binary MESH version 1

Recovered directly from `glmWriteBinMesh` at 0x345c and `glmReadBinMesh` at
0x3918. No struct images or machine pointers are serialized. Layout:

1. Four bytes `MESH`; uint32 version 1.
2. uint32 vertex count; count XYZ float32 triplets.
3. uint32 normal count; count XYZ float32 triplets (optional smooth normals).
4. uint32 UV channel count (0 or 1). If 1: uint32 UV count, uint32 component
   count 2, then count float32 pairs.
5. uint32 total triangle count; uint32 group count.
6. For each group: uint32 name length excluding null, name bytes including null,
   uint32 triangle count, uint32 UV reference-channel count (0 or 1), uint32
   topology type (1 triangles; 2 quads).
7. For every corner, position index, optional normal index if the file has
   normals, optional UV index if that group has UV references. Every index is
   exactly **three little-endian bytes**, zero-based. Quads emit four corners
   for each pair of output triangles and split (0,1,2), (0,2,3).

All uint32 and IEEE float32 fields are little-endian. Flat-normal write mode
does not serialize facet normals in the oracle. Materials, group texture IDs,
model position, pathname and material-library names are absent. The new reader
enforces version/channel/components/index/count validity and returns nullptr
for malformed input, instead of the original unchecked reads and thrown heap
int pointers. The original unrelated HACK_maxVertexCount truncation is omitted.
Tests include a literal independent quad wire fixture in addition to roundtrip.

## Modern private rendering

`glmShaderGLDL.obj` compiles fixed-function display lists and emits immediate
triangles with independent normals/UVs and group materials. The reconstruction
retains this behavior and applies materials before glBegin (the oracle applies
some material state inside glBegin, which is illegal for glMaterialf). Requested
texture coordinates absent from a model use sphere-map texture generation.

`glmShaderGLVA.obj` used private Vlist/Dlist pools and required all attribute
counts to match positions, incorrectly assuming shared attribute indices. The
modern backend converts CPU arrays keyed by corner indices and issues
glDrawElements. When caching, those draw commands compile into native OpenGL
display lists, remaining drawable after glmDelete. Their native IDs support
the ordinary glDeleteLists ownership path. GLVertexArray.cpp
implements this shared conversion and array drawing instead of restoring the
old private Vlist/Dlist class ABI; those types were never supplied publicly.
OGLExtensions.cpp loads the required active/client-active texture functions per
current WGL context instead of caching pointers across unrelated contexts. Other
original extension entry points had no supplied public declarations or callers
and are retained as reverse-engineering evidence, not exposed as a speculative
new API.

OpenGL calls and list creation require a current context. CPU loading,
conversion and manipulation work without one. No public API header changes were made.
Additional safety behavior: null deletion is accepted, degenerate normals are
zero rather than NaN, empty bounds are zero, point rendering restores GL state,
out-of-range submesh requests are ignored, reload replaces old owned geometry,
changed recalculates bounds, and collapse packs arrays with shifted indices.
Mesh load throws std::runtime_error for unreadable/malformed/empty geometry so
callers can recover instead of registering an unusable empty mesh. Collapse
checks pointers, indices and count overflows before modifying the mesh.
The oracle's dimensions implementation adds abs(min)+abs(max), overstating
dimensions when a bound does not straddle zero. The reconstruction honors the
public documented edge-length semantics with max-min, including unitization.

Shipped-asset integration found 142 of 352 OBJ files with positive UV indices
but no UV records (including BoxBoxTest00c/Box10.obj). The original reader
accepted these dangling optional references. The reconstruction accepts absent
normal/UV streams and clears their indices to zero, preventing out-of-bounds
access. Positive optional-attribute indices validate after the full file is
read, permitting forward references; indices beyond a present stream reject
the file. Position indices remain strict, and negative references resolve
against the stream size at the face declaration. Tests cover four shipped
Box10 variants, absent normals/UVs, forward attributes and invalid indices.
