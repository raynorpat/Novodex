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

The negative-dimension path is present through `NxActor::createShape` in the
registered `NxPhysicsActorDynamicSetterTests` and
`NxPhysicsActorShapeMutationTests` targets. The inventory now records the
function as reconstructed, while Phase 5 retains it as unfalsified until a
row-targeted mutation is caught by the staged gate.
