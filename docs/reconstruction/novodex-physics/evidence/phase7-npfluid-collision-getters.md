# Public `NxFluid` collision scalar getters

The public fluid table now implements four collision-property getters. `getStaticCollisionRestitution()` and `getStaticCollisionAdhesion()` acquire the wrapper's scene read lock and forward through `FluidGetStaticCollisionRestitution(manager, fluid)` and `FluidGetStaticCollisionAdhesion(manager, fluid)`. The dynamic-collision getters reproduce the oracle's unavailable-feature warnings and return zero.

## Differential and mutation evidence

The enabled FluidModel fixture accepts only the live backend manager and fluid handles. It returns `0.125f` for static restitution and `0.625f` for static adhesion, with exactly one call to each callback. The test checks the returned IEEE-754 words and callback counts. It also checks one `NXE_DB_WARNING` for each dynamic getter, including the exact `NpFluid.cpp` source path, line 188 or 203, and message.

Before implementing the slots, the oracle emitted both callback values and exact dynamic warnings; the candidate access-violated at the missing static-restitution slot. With the slots restored, both processes exit 0 and match stdout and stderr exactly. Replacing static-restitution forwarding with a zero return is rejected (`candidate_exit=1`, `stdout_delta=52`); the mutant marker reports `value=00000000 calls=0`.

The registered markers are:

```text
simulation fluid public-api static-collision restitution=3e000000 calls=1 adhesion=3f200000 calls=1
simulation fluid public-api dynamic-collision restitution=00000000.1.1 adhesion=00000000.1.1
```

Fresh Phase 5 and Phase 7 gates pass at 2,683/2,683 and 1,716/1,716 assertions. Compressed red, green, mutation, and gate transcripts are in [the raw evidence directory](raw/phase7-npfluid-collision-getters-20261013/).

No public header changed. Collision-property setters, particle data, the remaining public `NxFluid` methods, and full-DLL closure remain open.
