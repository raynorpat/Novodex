# Phase 7: unlicensed fluid surface-mesh constructor warning (`phys_fn_003503`)

IDA at `0x00086080` shows the `NpFluid` construction path calling the internal
surface-mesh constructor when `NxFluidDesc::surfaceMeshDesc` is non-null. For a
valid descriptor and a product key without the mesh feature bit, the oracle
reports `NXE_DB_WARNING` from `Fluid.cpp:280` (`NxFluid::createSurfaceMesh:
Feature not available!`) and continues creating the fluid.

The candidate now emits that warning while preserving successful fluid
creation. `NxPhysicsSimulationTests` includes the case in the registered public
DLL differential and its required output line. Before the change, the staged
oracle passed and candidate failed; temporarily suppressing the warning also
fails. Restored oracle and candidate both exit zero with exact output
(`stdout_delta=0`, exact stderr).

The complete Phase 7 gate passes at 1,779/1,779 registered assertions, and the
reconstruction tooling suite passes all 814 tests. Raw differential transcripts
are in `evidence/raw/phase7-fluid-surface-unlicensed-create-003503-20261014/`.

This proves only the unlicensed-warning branch. The licensed surface-mesh
constructor and backend callback path remain unimplemented; `phys_fn_003503`
stays open. Public Physics headers are unchanged.
