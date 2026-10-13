# Public NxFluid stiffness and viscosity getters

This slice wires the public `getStiffness()` and `getViscosity()` vtable slots for the current unlicensed runtime. The oracle resolves a separate FluidKey product flag when it creates a fluid manager. With the feature unavailable, each getter returns `0.0f` and reports one `NXE_DB_WARNING` from `NpFluid.cpp`. The implementation mirrors that key check and diagnostic path; with the feature bit enabled it forwards through the corresponding FluidModel getter while holding the wrapper's scene read lock.

## Differential and mutation evidence

The simulation fixture calls both getters on a valid enabled-manager fluid and checks the returned words, warning count, error code, source path, and source line. Before the vtable slots were added, the oracle completed while the candidate access-violated at the missing stiffness slot (`oracle_exit=0`, `candidate_exit=-1073741819`, `stdout_delta=54`, exact stderr). With the getters wired, both processes exit 0 and match exactly (`stdout_delta=0`, exact stderr):

```text
simulation fluid public-api scalars stiffness=00000000.1 viscosity=00000000.1 diagnostics=1.1
```

Changing the stiffness warning line from 109 to 110 causes the candidate diagnostic check to fail and the registered differential to reject it (`stdout_delta=2`, both processes exit 0, exact stderr). Restoring line 109 returns to the exact oracle transcript.

Phase 5 passes at 2,680/2,680 coverage assertions and Phase 7 at 1,713/1,713. Raw red, green, mutation, and gate logs are in [the evidence directory](raw/phase7-npfluid-scalar-getters-20261013/).

## Remaining work

This exercises the current host's unlicensed stiffness/viscosity feature path. A positive FluidKey and those live getter callbacks still need an enabled-license differential. The damping getter is covered separately in [its evidence](phase7-npfluid-damping-getter.md), as are the collision getters in [their evidence](phase7-npfluid-collision-getters.md). Scalar setters, particle data, and the remaining `NxFluid` virtuals are open. The full vtable and DLL audit remain incomplete. No public headers changed.
