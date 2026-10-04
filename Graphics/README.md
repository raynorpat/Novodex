# Graphics reconstruction

`GraphicsLib` is a source-built static library. Its 20 historical translation
units correspond to the object files in `lib/win32/Release/GraphicsLib.lib`.
`oracle-inventory.json` records each object's archive offset, size, SHA-256 and
source slot. The original library is reference material and is never linked
into the new viewer: its VC6 STL object layouts do not match modern MSVC.

The library implements scene ownership, object trees, transform accumulation,
camera/light rendering, traversal, model loading and merging, materials,
textures, primitives, bounds, OBJ/MTL parsing, binary MESH version 1, display-list
and vertex-array shader dispatch, plaintext/encrypted ODBlock scripts, CHU8/CHU9
terrain, offscreen rendering, and the legacy cl* interfaces.
Recovered behavior and object-local addresses are recorded in `reconstruction/`.
Malformed assets report recoverable failures rather than terminating the process.
OBJ assets with missing optional UV streams retain the original zero-UV behavior;
present attribute streams and position indices are bounds checked.

The following modules have replacements rather than binary reconstructions:

- `glutApp`: GLFW lifecycle/input in `Viewer/src/ViewerPlatform.cpp`.
- Viewer text: Dear ImGui overlays through the viewer's `ViewerUi` adapter.
- `gltxDEVIL`: stb image loading and BMP writing in `src/gltxSTB.cpp`.
- Viewer timers: `std::chrono::steady_clock` in `Viewer/src/ViewerClock.h`.

The original source slots remain so every archive member can still be inspected.
`Font.cpp` provides the legacy `clText` interface with Windows outline fonts;
`TimeWin.cpp` implements the public QPC timer interface. The viewer uses ImGui
and its modern clock adapter. The private NV vertex-array allocator is replaced
by owned buffers and native OpenGL display lists.
`gltxReadBMP` retains its historical name but detects all stb-supported formats
from their bytes. It returns RGBA8 pixels with a bottom-left origin, preserves
original dimensions, and returns null on decode failures. Free returned images
with `gltxDelete`. BMP writing expects a bottom-left image and returns false on
invalid input or write failure. This adapter does not emulate DevIL resizing,
compression, or the old codec-specific behavior.

## Inspect an object

From a Visual Studio developer PowerShell, extract into a build directory:

```powershell
New-Item -ItemType Directory -Force build/graphics-oracle | Out-Null
lib /list Graphics/lib/win32/Release/GraphicsLib.lib
lib /extract:.\Release\Scene.obj /out:build/graphics-oracle/Scene.obj Graphics/lib/win32/Release/GraphicsLib.lib
dumpbin /symbols build/graphics-oracle/Scene.obj
dumpbin /disasm build/graphics-oracle/Scene.obj
```

Do not copy STL layouts from the VC6 object into modern standard-library types.
Keep oracle evidence and tests alongside each recovered component.

## Verification and fidelity limits

With the viewer enabled, CTest covers script fixtures and all 698 supplied ODS
scripts, OBJ/MTL and binary mesh fixtures, the shipped Box10 meshes, geometry
operations, ownership and transforms, stb decoding, native shader rendering,
framebuffer pixels and occlusion, texture upload/filtering, resource cleanup,
and GLFW/ImGui lifecycle. Actual viewer smoke tests exercise BoxBoxTest00c,
Pyramid, TruckTerrain and TypesBreakable.

`Scene::renderToTexture` renders into a framebuffer with depth/stencil attachments,
reads back and resizes the result, and restores caller GL state, including on
exceptions. Its modern `Texture` and legacy `clTexture` targets share this path.
The field of view is horizontal; target selection and camera overrides do not
change the scene's camera or projection settings.

Terrain directives support CHU versions 8 and 9 and OBJ/MESH aliases. CHU rendering
uses the finest leaf meshes as a static surface, with normals and global UVs;
the public Terrain API does not expose adaptive LOD. See
`reconstruction/Terrain.md` for format provenance and validation.

Transform baking preserves geometric primitives, custom primitives, display
lists, and virtual models without mesh data. Shared source resources retain
their geometry. The old `3DClasses2.h` and `3DClasses3.h` interfaces use modern
resources while retaining virtual material/model callbacks, camera spaces,
sky spheres, and stencil reflections. See `reconstruction/LegacyGraphics-evidence.md`
and `reconstruction/Offscreen-and-baking.md`.

Missing scene assets still require the original files: TruckDemo references an
absent `Material #960.mat.ods`, and SimpleDemos references an absent `city.chu`.

## stb provenance

The files in `third_party/stb` are unmodified copies from
https://github.com/nothings/stb at commit
`2c980bb59875b0d32144a71867fbdebb2f77cd20`, including the upstream license.

| File | SHA-256 |
| --- | --- |
| stb_image.h | 594c2fe35d49488b4382dbfaec8f98366defca819d916ac95becf3e75f4200b3 |
| stb_image_write.h | cbd5f0ad7a9cf4468affb36354a1d2338034f2c12473cf1a8e32053cb6914a05 |
| LICENSE | bebfe904b14301657e4e5d655c811d51fd31b97c455b9cc2d8600d6bac6cff63 |
