# Terrain loading

The supplied Model oracle contains a disabled Terrain branch, and GraphicsLib
has no terrain object or public Terrain declaration. The reconstruction adds
`loadTerrain(Mesh&,...)` in Terrain.h and Terrain.cpp and routes Model's Terrain
scripts through it. It preserves the existing Mesh API and ownership scheme.

`ViewerScenes/SimpleDemos/Terrain.mod.ods` references `city.chu`; comments also
mention Terrain.chu and Desert.chu. No CHU files were supplied anywhere in the
repository. The material script references cityTex.bmp and desertsand.BMP.
Missing terrain files now produce explicit runtime_error messages containing
the filename. No replacement city geometry or texture was fabricated.

## Primary source and independent corpus

Format research uses Thatcher Ulrich's public-domain reference implementation,
linked from [his Chunked LOD project](https://tulrich.com/geekstuff/chunklod.html):
[SIGGRAPH 2002 source archive](https://downloads.sourceforge.net/project/tu-testbed/demos/chunkdemo-2002-08-06/chunkdemo-2002-08-06.zip).
The author's page explicitly puts the implementation in the public domain.
The archived heightfield_chunker.cpp writes the format; chunklod.cpp reads and
decompresses it. These are primary implementation sources, not third-party
format guesses. The original archive also contains a real `crater/crater.chu`
file (4,066,479 bytes), retained on C under this task's chunklod-reference
directory for independent integration testing. It is not added to ViewerScenes
or used as a replacement for city.chu.

Source anchors in that archive:

- heightfield_chunker.cpp 731: CHU9 file header.
- heightfield_chunker.cpp 1011 and 1059: 33-byte node header and addresses.
- heightfield_chunker.cpp 1496 and 1531: quantized vertex/strip encoding.
- chunklod.cpp 148, 202: vertex and strip decoding.
- chunklod.cpp 344: chunk center/extent computation.
- chunklod.cpp 1001: quantization decompression and vertical geomorph.
- chunklod.cpp 1622: recursive CHU8/9 node loading.
- chunklod.cpp 1719: global header loading.

## Wire layout

All fields are little-endian. Header (24 bytes): `CHU\0`, uint16 version (8 or
9), uint16 tree depth, float32 maximum base error, float32 vertical scale,
float32 finest chunk dimension, uint32 chunk count. The tree is complete with
sum(4^level) nodes, serialized preorder NW/NE/SW/SE.

Node header (33 bytes): uint32 label, four neighbor uint32 labels (-1 means no
neighbor), uint8 level, uint16 X, uint16 Z, int16 minimum Y, int16 maximum Y,
uint32 location. In CHU9 location is an absolute mesh offset; all node headers
precede geometry. In CHU8 location is inline mesh byte length; geometry follows
its node header before the child headers.

Mesh data: uint16 vertex count; per vertex int16 X/Y/Z and int16 vertical morph
delta; uint32 strip index count; uint16 indices; uint32 triangle statistics.
No materials, texture names, normals or UV arrays are encoded in CHU.

For a node at level L, dimension is baseDimension * 2^(depth-1-L). X/Z center
is (coordinate+.5)*dimension. X/Z extent is dimension*.5+.001, matching the
reference reader's millimeter overlap. World X/Z is center+encodedCoordinate *
extent/16384. World Y at full detail is encodedY*verticalScale. Morph deltas
encode transitions toward coarser ancestors; finest geometry uses unmodified Y.

## Conversion and rendering behavior

The loader validates both supported versions, complete tree topology, labels,
neighbors, addresses, finite/positive scales, counts, bounds, chunk offsets,
strip indices, inline lengths and overlapping CHU9 geometry. It validates all
levels while emitting only finest leaf chunks, avoiding overlapping ancestor
and descendant surfaces. Triangle-strip parity is preserved and duplicate-index
degenerates are skipped. It converts the real quantized heights and encoded
edge skirts into the existing indexed eight-float Mesh representation.

CHU surfaces form one `default` submesh, allowing the supplied Groups.default
material mapping to work directly. Generated UVs cover the entire root extent;
normals and explicit Scale/ForceTexCoords options use existing GLM operations.
OBJ and MESH terrain aliases preserve their actual groups. All decoding and
allocation completes before replacing the destination, so failures preserve
previous owned geometry.

This is a static finest-resolution surface for the existing Mesh renderer and
collision consumers. It decodes the hierarchy but does not add a new adaptive
LOD, texture-quadtree paging or geomorphing API; the original Novodex Terrain
branch supplied no such API. Its geometry remains the encoded terrain rather
than a flat approximation.

Tests cover independent literal CHU8/9 fixtures with nonzero slopes and morph
deltas, winding, attributes, bounds, material mapping, optional attribute and
scale flags, aliases, missing input, unsupported versions, every-byte
truncation and an optional genuine crater corpus argument. Root recorded the
RED failures of the disabled Terrain branch before implementation.
