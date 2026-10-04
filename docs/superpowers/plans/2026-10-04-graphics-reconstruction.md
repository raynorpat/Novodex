# Graphics reconstruction implementation plan

> Execute the independent reconstruction tasks in this checkout, then review and verify their integration. Leave changes unstaged.

**Goal:** Replace the GraphicsLib fail-fast stubs with working implementations so the migrated viewer can load and render the supplied scenes.

**Architecture:** Retain the supplied C++ public interfaces and the original archive as the behavioral oracle. Extract its 20 COFF members and inspect/decompile their functions with IDA. Implement modern C++ ownership and OpenGL compatibility rendering, retaining GLFW/ImGui/stb replacements for application, HUD, and image handling.

**Constraints:** MSVC Win32, C++11, fixed-function OpenGL compatibility context; preserve original archive bytes; avoid unrelated Foundation/Physics edits. Keep malformed input and missing files recoverable. Record oracle evidence and intentional platform replacements.

## Tasks

- [x] Parser: implement `Graphics/src/ODBlock.cpp`, preserving nested blocks, terminals, comments, quoting, file encryption, iteration and query behavior. Add `tests/GraphicsScriptTests.cpp` for real scene parsing, save/reload, encryption, malformed input and ownership.
- [x] Geometry: implement `glm.cpp`, shader modules and `Mesh.cpp` from the COFF objects. Test OBJ groups, independent position/normal/UV indices, negative indices, triangulation, normals, scale/weld, bounds, collapse, binary round trips and missing/malformed assets in `tests/GraphicsGeometryTests.cpp`.
- [x] Scene graph: implement Object, Iterator, Model and Bounds3d. Test ternary-tree overflow/reparent/removal, nested transform accumulation, material association, geometry merge and repeated destruction in `tests/GraphicsSceneGraphTests.cpp`.
- [x] Rendering/resources: implement Scene, Material, Texture and Primitive using recovered defaults and script directives; replace legacy Font/Time bodies with supported implementations. Add hidden-context rendering tests for textured meshes, primitives, camera/lights, state restoration and cleanup.
- [x] Integration: register all tests in Graphics/CMakeLists.txt, build Release/Debug, run focused tests, and load/render supplied ViewerScenes through the actual viewer. Fix failures by their root cause and inspect remaining stub references.
- [x] Review/documentation: independently review the integrated code; update Graphics/Viewer documentation to describe implemented behavior, oracle evidence, tests and any remaining fidelity limits.

## Review focus

Completed verification: Viewer and all six test executables built in Release
and Debug. Ten CTests passed in each configuration, including four actual-scene
smoke runs. All 698 ODS scripts parsed. Independent review findings for depth,
startup aspect, startup ImGui text, deferred texture filtering, unpack state,
and exception-safe material overrides were fixed and verified. The original
Graphics archive SHA-256 remains unchanged. TruckDemo's missing material and
original Terrain/renderToTexture no-ops were documented. The follow-up
`2026-10-04-graphics-gap-completion.md` supplies their working replacements,
primitive baking, legacy adapters and audio. Changes remain unstaged.

Parser errors must not leak partial trees. Geometry must reject invalid indices without reading out of bounds. GL resources must be released while a context exists. Objects and scene-owned assets must have one owner during reparenting and shutdown. Camera transforms must match nested scaled geometry.
