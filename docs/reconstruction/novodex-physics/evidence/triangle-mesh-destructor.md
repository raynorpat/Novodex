# TriangleMesh destructor reconstruction evidence

Date: 2026-10-07
Oracle: pinned `D:\FlamingEnt__\Unreal_3\Binaries\NxPhysics.dll`
Oracle SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`
Candidate: `build/Release/NxPhysics.dll`
Candidate SHA-256: `043a028a913f77da20d97c3a8790e9fb42e74a8d18c28ae7abbb8e597c7ae2ac`

IDA decompiled RVA `0x00055570` (`0x10055570`) as the TriangleMesh destructor.
It restores the two interface words, deletes the public wrapper, and calls
`0x00054a80`. That helper releases the embedded model and arrays, the optional
arrays at `+0x94/+0x98`, the deleting-destructor objects at `+0xac/+0xa8/+0xa4`,
the convex mesh and PMap, the edge list at `+0x88`, the Foundation allocation
at `+0x3c`, and (when its value is at least 2) the adjacency cache at `+0x84`.
The final helper calls the internal cleanup again and calls `NxFluidAssert`, a
no-op in the pinned image.

`NxPhysicsInternalTests` now constructs a TriangleMesh with the per-triangle
data, both topology caches, both optional arrays, the `+0x3c` allocation, and
three valid polymorphic deleting-slot objects. Before the destructor fix, this
fixture reported `foundation=8.4 sdk=20.10 deleting=0` and failed. After the
fix it reports `foundation=8.8 sdk=20.20 deleting=3` and passes, proving every
fixture allocation is released and each opaque object's deleting destructor is
dispatched. The internal field layout and public header hash gate also pass.

The staged oracle/candidate differential passed `NxPhysicsTriangleMeshApiTests`
with both processes exiting zero, `stdout_delta=0`, and exact stderr. This
retains the public lifecycle coverage for release refusal while an actor owns a
mesh, successful release after actor teardown, computed convex meshes, and
loaded PMaps. Build command: `cmake --build build --config Release --target
NxPhysics NxPhysicsTriangleMeshApiTests NxPhysicsInternalTests`.

After this commit reached `main`, the approved Viewer suite was rebuilt against
the candidate DLL and run with `ctest --test-dir build -C Release -R "^Viewer"
--output-on-failure`. All 48 registered tests passed across all 39 available
scenes: 43 passed, and the five established oracle-asset cases were skipped.

The internal fixture directly verifies the cleanup branches that the public
path does not currently populate; the public differential verifies observable
API behavior. Individual allocation-failure branches remain open.
