# FluidModel export resolver (phys_fn_003903)

`phys_fn_003903` at RVA `0x8d870` resolves 169 names from `FluidModel.DLL` and stores their addresses into the corresponding globals. `Physics/src/FluidModelExports.inc` records the manifest order, export names, and oracle global RVAs. `nxFluidManagerResolveModel` in `Physics/src/Scene.cpp` fills that table once after loading the optional module; typed callbacks read their entries from the same table.

The `NxPhysicsSimulationTests` child probe calls the pinned oracle loader/resolver and compares every oracle global with `GetProcAddress`. For the candidate it calls the candidate resolver and inspects each table slot. The registered marker is:

```text
simulation fluid-model-export-resolve row=003903 slots=169 present=83 matched=169 digest=cbb94fff
```

Mutation proof: the first candidate lookup was changed from `FluidManagerCreate` to `FluidManagerCreat`. The differential rejected it with `oracle_exit=0`, `candidate_exit=1`, `present=82`, `matched=168`, and `stdout_delta=4104`. Restoring the resolver and rebuilding Release produced `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and `stderr_exact=True` on both sides.

The fixture exports 83 of the 169 names; the remaining lookups correctly resolve to null. This closes the resolver's lookup-and-store behavior and exact table correspondence. It does not establish FluidModel ABI correctness for all 169 functions or reconstruct their implementation bodies.

The full Phase 5/6/7 gate logs, mutation run, and restored control are archived in `evidence/raw/phase7-fluid-model-export-resolution-003903-20261016/`. Gate results are 2,826/2,826, 1,715/1,715, and 2,959/2,959 coverage assertions.
