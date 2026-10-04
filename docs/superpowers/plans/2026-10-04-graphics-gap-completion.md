# Graphics gap completion implementation plan

**Goal:** Complete the remaining Graphics interfaces and enable working viewer audio.

**Architecture:** Share the modern SceneGraph implementation with legacy adapters. Decode the supplied terrain format into owned meshes. Render offscreen with a color texture and depth attachment, preserving caller GL state. Preserve arbitrary primitive appearance during transform baking through owned transformed primitives. Replace obsolete audio dependencies with a supported Windows backend.

**Constraints:** MSVC Win32 and the existing OpenGL compatibility context. Keep the oracle archive unchanged, retain unstaged changes, avoid unrelated SDK modifications, and build on C: due limited D: space.

- [x] Terrain: grounded chunk-format loader, materials/groups, malformed-input fixtures and mesh checks.
- [x] Legacy API: working old headers and modern adapters; test load, scene ownership, tree operations and special cameras.
- [x] Offscreen rendering: modern Texture target and legacy adapter; test pixels, dimensions, camera/target selection, depth and GL state restoration.
- [x] Primitive baking: preserve custom/display-list and geometric primitive transforms and ownership; test repeated baking and appearance.
- [x] Audio: WAV decoding, working backend, scene/source/listener integration and lifecycle tests.
- [x] Integrate CMake and documentation, independently review, build and test Release/Debug, check original archive hash and remaining gap references.

## Final verification

Viewer and all ten test executables built in Release and Debug. All 17 CTest
checks passed in each configuration (45.14 seconds Release, 35.41 seconds Debug),
including audio backend playback and a generated scene using actor audio. Script
tests cover the 698 supplied ODS files. A separate real CHU9 crater corpus also
loaded successfully in Release. Independent review led to regression fixes for
pixel-transfer/client texture state, exception cleanup, legacy material and mesh
virtual callbacks, and primitive-container baking; no further findings remained.

The original GraphicsLib archive is unchanged: SHA-256
`41f62c343a86f1e7d8bc41bed4b36b991038707179dd5078d3ca614934cdd7a3`.
Changes remain unstaged. Missing original data still prevents the corresponding
scenes from loading: TruckDemo's `Material #960.mat.ods` and SimpleDemos' `city.chu`.
