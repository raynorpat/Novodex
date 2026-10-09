# Phase 6 closure: joint limit-plane distance helper (`phys_fn_004131`)

`Joint::row004131` transforms a limit plane through the second solver body's frame and evaluates a point's signed distance. It is used while adding planes and iterating the joint's limit-plane list. The registered `NxPhysicsCoreDumpTests` fixture serializes the resulting plane state.

A temporary zero-return mutation was applied to `row004131`, then `NxPhysics.dll` was rebuilt. The staged-pair differential caught the changed limit-plane transcript with both processes exiting zero, exact stderr, and `stdout_delta=4448`. Mutant DLL SHA-256: `b8e8e7886250f157fa3d51c545c3dc1b3c4bd448a072eea9899f99f85a961e5e`.

After restoring `Joint.cpp` and rebuilding, the same staged differential returned to exact agreement (`stdout_delta=0`, both exits zero, exact stderr). Restored candidate DLL SHA-256: `ff0cfad2cdc6e4aecc0b183f08690fa7452324ca3798ed28b29dda0efad679ca`; pinned oracle DLL SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Logs: `build/joint-limit-plane-row004131-mutant.log` and `build/joint-limit-plane-row004131-control.log`.
