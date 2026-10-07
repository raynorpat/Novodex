# Phase 5 actor accessor mutation evidence

Seven actor getter rows are independently falsified through their existing
oracle-backed staged-pair targets:

| Row | Mutation | Target | Mutant transcript delta |
|---|---|---|---:|
| `phys_fn_000096` | local mass-pose rotation source +0xdc → +0xe0 | `NxPhysicsActorCMassTests` | 56 |
| `phys_fn_000098` | local mass-position source +0x100 → +0x104 | `NxPhysicsActorCMassTests` | 56 |
| `phys_fn_000100` | local mass-orientation source +0xdc → +0xe0 | `NxPhysicsActorCMassTests` | 58 |
| `phys_fn_000102` | mass-space inertia source +0x18c → +0x190 | `NxPhysicsConvexMeshTests` | 2 |
| `phys_fn_000104` | linear-velocity source +0x6c → +0x70 | `NxPhysicsActorDynamicsTests` | 2 |
| `phys_fn_000106` | angular-velocity source +0x78 → +0x7c | `NxPhysicsActorDynamicsTests` | 2 |
| `phys_fn_000108` | linear-momentum mass source +0x188 → +0x18c | `NxPhysicsActorMomentumTests` | 4 |

For every mutation, oracle and mutant processes exited zero and stderr matched
exactly. The row's differential reported the nonzero `stdout_delta` above.
After each mutation, the source was restored byte-for-byte, the DLL and target
were rebuilt, and the clean differential returned to zero delta with exact
stderr.

The ignored worktree `build/` directory contains the matching mutant, restored,
and build logs under the prefix `phase5-phys_fn_0000` followed by each row ID.
