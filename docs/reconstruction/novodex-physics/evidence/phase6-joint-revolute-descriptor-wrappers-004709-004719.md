# Phase 6 closure: revolute limit, motor, and spring wrappers

The existing staged-pair fixture calls the public setters and getters for the revolute limit, motor, and spring descriptors, then prints all enable bits and returned words. Each of the six `NpRevoluteJoint` wrapper rows was mutated independently by omitting its internal dispatch or replacing its returned enabled bit with false.

| Row | Mutation | `stdout_delta` | Mutant candidate SHA-256 |
|---|---|---:|---|
| `phys_fn_004709` | Omit `setLimits` dispatch | 6 | `3736a43a482f4daa854d8c7817e00a084616d768714bfe046201bc1765c89f99` |
| `phys_fn_004711` | Return false from `getLimits` | 2 | `d363c61e897c8a941162520574e4c6df3ec9882881a5e389729f5284110107a3` |
| `phys_fn_004713` | Omit `setMotor` dispatch | 6 | `cfbfddee63b3097c56882f0dd3c8601fe892b16c1ec051deeffd875244187499` |
| `phys_fn_004715` | Return false from `getMotor` | 2 | `505c9c6d8d83fdd770932c8479b3bbe278669284f6c0e3c89fdfdb8e2ee01c81` |
| `phys_fn_004717` | Omit `setSpring` dispatch | 6 | `fa92b7a055518b76c4376761c25da529d199c9a0a48e23fd978a8162fd348c7e` |
| `phys_fn_004719` | Return false from `getSpring` | 2 | `6220d7c5e96fa2561892d3e9f957c764b745f3157888670ccb79dfb580b2f44b` |

Every mutant was caught by `NxPhysicsJointStagedPairTests`; both processes exited 0 and stderr matched exactly. Restoring all six rows returned `stdout_delta=0`; restored candidate SHA-256: `a2bc9c04845cdb3c0dad4106e0f2299adc14d4f68444bff5ce6c5b73c074cd50`. Build, mutation, and restored-control logs are retained in the ignored `build/phase6-0047*-*` files.
