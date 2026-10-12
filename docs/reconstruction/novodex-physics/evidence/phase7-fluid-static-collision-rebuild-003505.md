# Fluid static-collision rebuild: `phys_fn_003505`

`phys_fn_003505` is the 473-byte cdecl routine at RVA `0x861b0`. IDA's caller xrefs identify the dirty-fluid manager path `phys_fn_003630` and one other caller at RVA `0x8a000`. The dirty manager destroys the old collision object, obtains the replacement object, invokes this row with the scene, rest-particle distance, and kernel-radius multiplier, then clears the dirty byte.

The rebuild initializes static-collision state and derives its grid parameters as `cellSize = kernelRadiusMultiplier * 8` and `margin = restParticleDistance * 13 + 0.001`. It gathers static and dynamic scene shapes into mesh and primitive arrays, then runs the mesh grid pipeline: count triangle cells into 2,048 hash buckets; compute bucket prefixes; enumerate and deduplicate cells; compute cell association prefixes; append mesh/triangle indices; copy the finished descriptor into the collision object; destroy temporary hash arrays and the signed-16-bit cell buffer.

The scene shape collection is reached at `scene + 0x624`; the internal container pointer is at collection `+0x1c`, and the count is the sum of its words at `+8` and `+0xc`. The empty-container path skips scene element access, which makes it a useful first differential case, but does not cover the builder's semantic purpose. The nonempty path depends on the still-open collection and shape-packing rows `003511` and `003540` through `003549`.

The pinned UE3 directory has no `FluidModel.dll`. `003505` therefore cannot be tested by leaving its named extension callbacks unbound. The oracle fixture must explicitly bind the static-collision constructor, primitive and mesh array growth, bucket/cell/index array creation and copying, and corresponding temporary-array destruction operations. These fixtures provide the callback contracts while keeping the test focused on the reconstructed `NxPhysics.dll` row. SAT, cell growth, and cell lookup fixture operations already exist for the lower-level triangle and association probes.

This is static analysis only. No candidate implementation, direct `003505` differential, mutation, gate closure, or full row reconstruction is claimed yet.
