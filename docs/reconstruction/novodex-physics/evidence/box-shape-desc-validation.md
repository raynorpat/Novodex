# Box shape descriptor validation

The row `phys_fn_002324` (RVA `0x0005a240`) is `NxBoxShapeDesc::isValid`, not
a controller descriptor method. IDA's vtable for the temporary box shape
descriptor used by the controller constructor places this function in slot 2.
The descriptor's type field is `2`, its shape flags are at `+0x38`, its group
and material are at `+0x3c` and `+0x3e`, and its dimensions are at `+0x4c`,
`+0x50`, and `+0x54`.

The oracle validates all twelve `NxMat34::localPose` floats at `+0x08..+0x34`
as finite, requires the group to be below 32, rejects shape-flag bits above the
low 16 bits, accepts types below 6, and rejects material `0xffff`. It then
requires all three box dimensions to be finite and nonnegative. This matches
the existing inline implementation in `Physics/include/NxShapeDesc.h` and
`Physics/include/NxBoxShapeDesc.h`; public headers remain unchanged.

`NxPhysicsControllerSweepFaceTests` now records and asserts the Scene actor
count after creating its box controller. With the pinned oracle and the
restored candidate, the fixture reports two actors (the triangle-mesh obstacle
and controller actor). A row-targeted mutation changed the temporary
`Physics/src/NxBoxShapeDesc.h` copy from rejecting negative X dimensions to
rejecting nonnegative X dimensions; it left the public header untouched. The
candidate then reported one actor and exited 1, while the oracle reported two
and exited 0. The staged pair rejected the mutant with `stdout_delta=5`.

After removing the temporary header, the focused controller differential
passed with exact output, and `NxPhysicsSimulationTests` also passed with exact
output. Captured logs: `build/phase5-boxdesc-002324-focused-mutation.log`,
`build/phase5-boxdesc-002324-restored-controller.log`, and
`build/phase5-boxdesc-002324-restored-simulation.log`.
