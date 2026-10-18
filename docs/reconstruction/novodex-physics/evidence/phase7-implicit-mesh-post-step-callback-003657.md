# Implicit-mesh post-step callback (`phys_fn_003657`)

IDA/Ghidra identify `phys_fn_003657` at RVA `0x0008abe0` as the post-step callback for an implicit fluid mesh. It asks `FluidModel.dll` for `ImplicitFluidMeshGetMesh`, using the owning fluid backend's system and handle, then passes the returned mesh to the existing stream-copy routine. The candidate now has a dedicated `nxFluidPostStepImplicitMeshUpdate` helper, and `nxFluidPostStepUpdate` routes its mesh record through that helper.

The registered `NxPhysicsSimulationTests` fixture advances a licensed fluid with one vertex and one indexed triangle. Both staged pairs report `mapped=1`, `ready=1`, `fetched=1`, and matching position, normal, index, and count data. The full Phase 7 gate passes all 3,048 required coverage assertions.

A mutation that returns immediately from the helper keeps its map entry but leaves the mesh outputs zero. The row-specific result remains mapped, while the copy reports `matched=0`, and the test exits 1 with `FAIL licensed NxFluid implicit-mesh API did not match the fixture contract`. The registered oracle/candidate runner reports `oracle_exit=0`, `candidate_exit=1`, and `stdout_delta=4130`. Restoring the callback reports `matched=1` and exits 0. Raw mutation and restored transcripts are in `evidence/raw/phase7-implicit-mesh-post-step-callback-003657-20261018/`.

The fresh Phase 5, Phase 6, and Phase 7 gates pass at 2,909/2,909, 1,766/1,766, and 3,048/3,048 registered coverage assertions. Their complete runner logs are retained in `evidence/raw/phase7-implicit-mesh-post-step-callback-003657-20261018/`.
