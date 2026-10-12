# Fluid scene-shape collector (`phys_fn_003555`)

`phys_fn_003555` (`0x00087b80`, 189 bytes) walks the Scene shape container in
container order and passes each ordinary shape to the fluid shape-array router
at `0x00087990`. A compound shape (type 5) contributes every member in its
`[shape+0xe0, shape+0xe4)` range instead of the compound object. The row returns
void and uses the cdecl `(builder, scene)` convention.

The reconstruction is `nxFluidCollisionCollectSceneShapes` in
`Physics/src/FluidCollision.cpp`. Its direct differential is part of
`NxPhysicsFluidEmitterAbiTests`, registered in Phase 7. The fixture first checks
an empty container, then a populated eight-entry container with one six-member
compound and one empty compound. Twelve resulting leaves exercise the existing
typed array router, including eligible and filtered shapes. It compares array
counts, defined record prefixes, geometry callback payloads, untouched output
tails, and stack balance. The restored pinned-oracle run reports
`cases=2 empty=1 populated=1 compound_children=6 routed_shapes=12`,
`oracle_contract=1`, `stack=1/1`, and `mismatches=0`.

Mutation evidence: restricting the compound loop to its first child produced
five observed differential mismatches, including missing mesh, plane, and
capsule records and reduced box/sphere counts. The restored candidate again
matches the pinned DLL with zero mismatches. Raw transcripts are in
`evidence/raw/phase7-fluid-scene-shape-collector-003555/`.

Fresh full Phase 5 and Phase 7 gates pass at 2,660/2,660 and 1,687/1,687
registered assertions, respectively. Both gates include the new collector
assertion, immutable-header checks (80 files), and inventory validation
(6,338 functions, 5,138 data objects, zero unexplained entries). Full logs are
retained in the raw evidence directory.

The collector row is closed. The enclosing dirty static-collision builder
`phys_fn_003505` and production `FluidModel.dll` callback integration remain
open dependencies for end-to-end static collision reconstruction.
