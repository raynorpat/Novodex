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

IDA confirms the primary vtable at `0x10108824` has three entries: deleting
destructor (`0x1005a470`), `move` (`0x10059710`), and `getPosition`
(`0x1005a870`). The getter returns `this + 0x28`; the constructor initializes
that vector from descriptor `+0x0c`. A private test-only ABI mirror now calls
the getter on two controllers and checks both bit-exact positions. Before the
implementation change the oracle returned the descriptor values while the
candidate's old destructor-only vtable returned invalid values; after the
change the simulation differential passes with `stdout_delta=0` and exact
stderr.

The vtable layout and getter are reconstructed. The `move` slot is present to
preserve dispatch layout. A new paired probe exercises collision-free motion in
an unobstructed path: a `+0.25` X displacement with `minDistance=0.001` updates the
controller getter to `0.25` and clears collision flags, while the generated
kinematic actor still reports its pre-step pose. The candidate's translation
subset matches those oracle observations exactly (`stdout_delta=0`, exact
stderr); the test output is registered in Phase 7.

General collision-aware sweep, step response, and hit callbacks remain
incomplete. A second paired fixture places an
axis-aligned static box at X=1.5: moving the controller from X=0 by +2 stops at
X=0.5 with collision flag 4. Repeating that move with active group mask zero
passes through to X=2.5 with no collision flags. A temporary mutation that
ignored the group mask failed the test (`candidate_exit=1`, `stdout_delta=2`);
the restored implementation matches the oracle exactly. This is a swept-AABB
box subset. A vertical fixture moving +Y toward an overhead box stops at Y=0.5
with collision flag 1, and catches a candidate that reports the horizontal
side flag for every axis. An enabled trigger shape remains a blocking sweep
candidate in this oracle build; moving toward it stops at X=7.5 with flag 4.
A clean-scene diagonal fixture also verifies tangential projection along the
X wall: displacement `(2,0,1)` ends at `(0.5,0,1)` with flag 4, while the
candidate before slide handling stopped at `(0.5,0,0.25)`. Rotated shapes,
convex/mesh query faces, initial-overlap, multiple-face sliding, step-up, and
callbacks remain open. The rest of the
constructor/destructor callees and complete controller interface remain open;
this is still partial controller support, not a claim that controller
behavior is complete.
