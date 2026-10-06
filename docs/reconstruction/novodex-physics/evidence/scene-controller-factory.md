# Partial scene controller factory and release slice

The `NxScene::createController` and `releaseController` forwarding entries and
the tested allocation/list lifecycle slice are implemented in `NpScene.cpp` and
`Scene.cpp`. No public Physics header was changed; the SDK tree exposes
`NxController` and `NxControllerDesc` as incomplete types.

IDA evidence for the pinned DLL:

- RVA `0x0000c720` loads the internal Scene from `NpScene+0x24` and jumps to
  `Scene::createController` at `0x0005a880`.
- RVA `0x0000c730` performs the same Scene load and jumps to
  `Scene::releaseController` at `0x0005a4b0`.
- The factory rejects a nonzero descriptor word at `+8`, allocates `0x4c`
  bytes, constructs the controller and links its embedded object at allocation
  `+8` into `Scene+0x5a8`; the list link is at embedded-object `+0x30`.
- The constructor creates one kinematic box actor. Its half-extents are the
  descriptor dimensions at `+0x30`, `+0x34`, and `+0x38`, each multiplied by
  `1.1`; its position is copied from `+0x0c`, `+0x10`, and `+0x14`.
- Release unlinks the embedded controller object and calls its scalar deleting
  destructor. The oracle leaves the generated actor in the scene, so the
  candidate preserves that observed behavior.

The paired simulation regression covers an unsupported descriptor, two
controllers in one scene, actor count changes, generated actor position and box
dimensions, middle-node removal, and final head removal. On the final build,
`NxPhysicsSimulationTests` exits zero on both DLL pairs with `stdout_delta=0`
and exact stderr. Phase 5 passes 2,042/2,042 assertions; Phase 7 passes
1,338/1,338 assertions. The all-scene Viewer selection passes 48/48 selected
cases, including 43 passes and five existing signature-verified skips.

The returned object's primary vtable is a lifecycle shell with only destructor
slots. The test deliberately does not call controller methods. The rest of the
controller virtual interface and complete public-object vtable remain open, so
this is not yet usable controller support or a complete reconstruction of the
factory callees.
