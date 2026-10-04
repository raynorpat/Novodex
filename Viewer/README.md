# Viewer

The CMake viewer uses the source-built `NxPhysics`, `NxFoundation`, and
`GraphicsLib` targets. It requires MSVC targeting Win32 because the viewer's
profiler code and the reconstructed SDK use 32-bit x86 code.

```powershell
cmake -S . -B build -A Win32 -DNOVODEX_BUILD_VIEWER=ON
cmake --build build --config Release --target Viewer ViewerPlatformTests ViewerSceneTests GraphicsImageTests GraphicsScriptTests GraphicsGeometryTests GraphicsSceneGraphTests GraphicsTerrainTests GraphicsLegacyTests GraphicsRenderingTests GraphicsOffscreenTests ViewerSoundTests
ctest --test-dir build -C Release --output-on-failure
```

For an existing configured build, omit `-A` unless it matches the current
generator. `NOVODEX_BUILD_VIEWER` defaults on for Windows/MSVC/Win32 and off
elsewhere. Set it to `OFF` for an SDK-only build. CMake 3.16 or newer is required.

The executable is `build/Viewer/Release/NovodeXPersonal.exe`. CMake copies the
rebuilt Physics and Foundation DLLs beside it. Pass a scene's `.pds.ods` file
from `ViewerScenes` as the argument; the viewer changes directory to that file
so scene-relative assets resolve. The Visual Studio working directory defaults
to `ViewerScenes`.

After building, run the CMake install pass:

```powershell
cmake --install build --config Release
```

This installs `NxFoundation.dll`, `NxPhysics.dll`, `NovodeXPersonal.exe`, and the
two SDK import libraries into `Bin` at the project root. The destination is fixed
to that directory, including for out-of-source builds and existing install-prefix
settings. Use `--config Debug` to install the Debug artifacts instead; configurations
share the same filenames. Foundation and Physics still install with the viewer
disabled.

GLFW 3.4 and Dear ImGui 1.92.5 are fetched from pinned commits with SHA-256
checks in `Dependencies.cmake`. Initial configuration requires network access;
subsequent builds use CMake's cache. For offline configuration, prepopulate the
sources and set `FETCHCONTENT_SOURCE_DIR_GLFW` and
`FETCHCONTENT_SOURCE_DIR_IMGUI` to their local paths. No GLUT or DevIL library
or DLL is used. Dear ImGui uses its GLFW and OpenGL 2 backends alongside the
existing fixed-function renderer.

The menu bar and middle-button popup expose File, visualization, and profiler
actions. File > Load Scene opens a Windows file picker for `.pds.ods` scenes;
cancelling keeps the current scene running. Selecting a scene replaces the
physics, graphics, and audio resources and resolves assets relative to its
folder. File > Exit shuts down through the GLFW loop. HUD, help, scripted text,
and profiler overlays use ImGui.
Arrow/page keys keep the controller's existing input codes; character callbacks
handle Shift and keyboard layouts. Right-button picking and dragging use
framebuffer pixel coordinates, and UI capture blocks scene input. Insert/Delete
retain their nudge/delete actions. Escape leaves an active controller first;
otherwise Escape and window close shut down through the GLFW loop.

The Physics API migration covers value-based gravity, shape and mesh flags,
center-of-mass offsets, `NxRaycastHit`, `simulate`/`fetchResults`, and deferred
joint release after a break callback. Solver selection is now internal; the old
`Lagrange` directive has no explicit request API. The old car-reset force/torque
setters are removed because the SDK clears accumulators at each completed step.
Sound uses Windows XAudio2/X3DAudio, with no OpenAL or ALUT dependency. WAV loading
supports PCM 8/16/24/32-bit and float32, including extensible WAV headers. Scripted
sources support looping, pitch/gain and spatial positioning; actor poses and
camera motion update sources and the listener. Audio shutdown releases voices
before the engine and permits reopening. An unavailable audio device is reported
through `Sound::Manager::lastError()` and leaves visual simulation usable. The obsolete `src/glm` SDK
adapters are not built.

The source-built Graphics library loads and renders scenes. GLFW initialization
restores the original depth, lighting and normal state before scene startup;
field-of-view directives can run during loading, and scripted startup text has
an initialized ImGui frame. See `Graphics/README.md` for reconstruction evidence
and fidelity limits.

For a hidden three-frame load/render/shutdown check:

```powershell
build/Viewer/Release/NovodeXPersonal.exe --smoke-test ViewerScenes/BoxBoxTest00c/demo.pds.ods
```

For a deterministic viewer-to-physics integration check, the `ViewerPhysicsStep`
CTest loads a small falling-box scene, advances 60 fixed 0.01-second steps, and
checks the actor's final height while the viewer renders each frame. Run it with:

```powershell
ctest --test-dir build -C Release -R '^ViewerPhysicsStep$' --output-on-failure
```

`ViewerPhysicsContact` runs a sphere through the same viewer loop over the
scene's built-in ground plane. It checks that contact resolution keeps the actor
near the expected rest height after 60 fixed steps:

```powershell
ctest --test-dir build -C Release -R '^ViewerPhysicsContact$' --output-on-failure
```

Missing assets produce console diagnostics in smoke mode. TruckDemo requires
the absent `Material #960.mat.ods`; SimpleDemos requires the absent `city.chu`.

## Verification on 2026-10-04

The reconstructed library and viewer have 18 Graphics, platform, audio, scene,
and physics-integration checks. Actual-scene smoke tests cover BoxBoxTest00c, Pyramid, TruckTerrain,
TypesBreakable and a generated sound scene. Script checks parse all 698 supplied
ODS files. Terrain tests cover CHU8/CHU9, hierarchy, malformed inputs and a separate
real CHU9 crater corpus. Legacy tests cover loading, ownership, linked submeshes
and virtual callbacks. Rendering
checks inspect framebuffer pixels, depth occlusion, texture upload and filtering,
both shader implementations, and resource shutdown. Platform checks exercise
startup text, repeated contexts, menus and the installed callback chain for
Insert, Delete and Escape. The original archive remains unchanged.
Offscreen tests also inspect GL state restoration and exception paths, primitive
baking and legacy reflections/camera spaces. Audio tests check WAV decoding,
playback progress, looping and lifecycle; the backend test reports a CTest skip
if no output device is available. Verification uses a build directory on C:
because D: lacks enough space for SDK linking.

The final Viewer and test builds succeeded in both Release and Debug. All 18
CTest checks passed in each configuration, with actual audio output available
and no skipped tests. Independent review covered legacy virtual dispatch,
exception cleanup and repeated custom-model baking. The original GraphicsLib
archive's SHA-256 remains
`41f62c343a86f1e7d8bc41bed4b36b991038707179dd5078d3ca614934cdd7a3`.

Earlier SDK checks (before Graphics reconstruction) found two oracle/candidate differences in
`NxPhysicsSimulationTests`' triangle-mesh rows (submesh/vertex counts and indices).
`NxPhysicsSceneRaycastTests`, `NxPhysicsTriggerSimulationTests` and
`NxPhysicsJointStagedPairTests` matched the oracle. The Physics reconstruction
Python suite ran 770 tests with four failures: the phase-4 unregistered-phase
expectation and the plan-command checks for phases 4, 6 and 7. These SDK and gate
failures remain outstanding.
