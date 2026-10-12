# Fluid static-collision rebuild: `phys_fn_003505`

`phys_fn_003505` is the 473-byte cdecl routine at RVA `0x861b0`. IDA's caller xrefs identify the dirty-fluid manager path `phys_fn_003630` and one other caller at RVA `0x8a000`. The dirty manager destroys the old collision object, obtains the replacement object, invokes this row with the scene, rest-particle distance, and kernel-radius multiplier, then clears the dirty byte.

The rebuild initializes static-collision state and derives its grid parameters as `cellSize = kernelRadiusMultiplier * 8` and `margin = restParticleDistance * 13 + 0.001`. It gathers static and dynamic scene shapes into mesh and primitive arrays, then runs the mesh grid pipeline: count triangle cells into 2,048 hash buckets; compute bucket prefixes; enumerate and deduplicate cells; compute cell association prefixes; append mesh/triangle indices; copy the finished descriptor into the collision object; destroy temporary hash arrays and the signed-16-bit cell buffer.

The scene shape collection is reached at `scene + 0x624`; the internal container pointer is at collection `+0x1c`, and the count is the sum of its words at `+8` and `+0xc`. The empty-container path skips scene element access, which makes it a useful first differential case, but does not cover the builder's semantic purpose. The nonempty path depends on the still-open collection and shape-packing rows `003511` and `003540` through `003549`.

The pinned UE3 directory has no `FluidModel.dll`. `003505` therefore cannot be tested by leaving its named extension callbacks unbound. The oracle fixture must explicitly bind the static-collision constructor, primitive and mesh array growth, bucket/cell/index array creation and copying, and corresponding temporary-array destruction operations. These fixtures provide the callback contracts while keeping the test focused on the reconstructed `NxPhysics.dll` row. SAT, cell growth, and cell lookup fixture operations already exist for the lower-level triangle and association probes.

The lower-level packer row `phys_fn_003540` now has a candidate in
`Physics/src/FluidCollision.cpp` and a two-case oracle differential in
`NxPhysicsFluidEmitterAbiTests`. The first run caught a pose-layout mismatch:
the oracle stores the 3x3 rotation words in column-major order before the
translation, while the candidate initially copied the source's 3x4 words
unchanged. Commit `86dad44c` corrected that permutation; both packer cases now
match, and the fresh Phase 5 gate passes at 2,651/2,651 assertions. See the
local transcript at
`build/phase5-approved-destructor-model/phase5-after-pose-order-fix.log`.

`phys_fn_003505` is now reconstructed in `Physics/src/FluidCollision.cpp` as
`nxFluidCollisionBuildStaticCollision`. The implementation preserves the
oracle's callback order, uses FluidModel-owned array and hash callbacks, and
passes the count-then-grow-then-collect lifecycle into the mesh-grid pipeline.
The explicit fixture models array growth by clearing the sizing-pass count
before the collection pass.

The registered differential calls the pinned oracle at RVA `0x861b0` and the
map-resolved candidate DLL with two scenes. The empty case matches initialized
parameters, all typed-array headers, the 2,048-bucket empty grid, return value,
and x86 stack. The nonempty case routes one triangle mesh through the complete
grid build and matches all typed arrays, defined mesh-record bytes, bucket
counts/offsets, cell records, triangle associations, parameters, return value,
and stack. Undefined mesh-record tail dwords and bucket words are ignored. The
restored control reports zero mismatches in both cases.

A targeted mutation changes the margin bias from `0.001` to `0.002`; both
cases reject it through the registered parameters comparison (`mismatches=2`)
and the harness exits nonzero. See the raw direct differential, mutation, and
Phase 5/7 gate transcripts under
`evidence/raw/phase7-fluid-static-collision-rebuild-003505/`.

The pinned UE3 binaries do not contain production `FluidModel.dll`; this closes
the `NxPhysics.dll` builder against explicit callback fixtures. Loading the
production companion module in Unreal remains a separate integration check.

Fresh Win32 Release gates on 2026-10-12 pass Phase 5 at 2,664/2,664 evaluated assertions and Phase 7 at 1,691/1,691. Full logs: vidence/raw/phase7-fluid-static-collision-rebuild-003505/phase5-gate.log and phase7-gate.log.
