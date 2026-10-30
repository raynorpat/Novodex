# The internal triangle mesh's remaining rows

Sixteen Phase 4 rows (13 functions) in `Physics/src/TriangleMeshRows.cpp`, with 18 helper functions. They are naked
listing transcriptions, 1,730 instructions in all.

The rows: 001524 and 001528 (the hull subclass the mesh builds for its convex form: its constructor, its scalar
deleting destructor and its table), 002077, 002178, 002184, 002190 to 002194 (one function), 002200, 002205,
002239, 002243 with 002245 (one function), 002247, 002255 and 002256. `raw/phase4-triangle-mesh-rows-20261030/manifest.json`
says what each does, as read from the listing.

## Helpers

Some small rows these call exist in the candidate only as harness models (for example
`BoxShape::nxWrap1409`, which stores the oracle's raw table address) or with another calling convention (cdecl C++
functions for thiscall rows). Those are transcribed here as helpers and their own records are unchanged:

001409, 001413, 001421, 001455, 001457, 001526, 001544, 001668, 000001, 002060, 002069, 002071, 002073, 002079,
002081, 005155, 005157, 005159.

`autohelp.py` adds a helper for every call target that is a row start and resolves to no ABI-compatible symbol.

## What else the transcription names

* **Called directly:** the candidate rows that are ABI-compatible: the support-map constructors and initialiser
  (`IceSupportMaps.cpp`), the hull rows (`HullShape.cpp`, `HullBuild.cpp`), `TriangleMesh::createEdgeList`,
  `RadixSort::Sort`, `NxBuildSmoothNormals`, the internal mesh's release and topology rows, and the SDK allocator.
* **The oracle's tables are the candidate's by name:** the hull shape's, the hull's and the four support maps'.
  The hull subclass's table (`0x10107724`) is a new `nxTMHullTable`, which holds 001528, `HullShape.cpp`'s 001417 and
  001418, and 001526.
* **The CRT's `_CIpow`** (the oracle's `0x000f4b10`) is the candidate CRT's, through an alias.
* **The two five-way jump tables** are compare chains.

`evidence/phase4-triangle-mesh-rows-listing-compare.py` finds every instruction equal, with 57 call mappings as
expected (`raw/phase4-triangle-mesh-rows-20261030/listing-compare.txt`). Pushed and stored table addresses are
compared as pointers into a section; the script does not check which table they are.

**Not wired.** `TriangleMesh.cpp` builds and releases its meshes in C++ of another shape, so nothing calls these rows.
They are listed for the testing pass.

Rows with indirect calls: 001528, 002077, 002178, 002190, 002205, 002239, 002243, 002247 and 002255.

The rows await the testing pass.
