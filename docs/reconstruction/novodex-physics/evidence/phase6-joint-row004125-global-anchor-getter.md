# Phase 6 closure: shared joint global-anchor getter (`phys_fn_004125`)

`Joint::getGlobalAnchor` transforms each world anchor through the corresponding body pose and local mass frame, averages the two resulting points, and uses an untransformed world anchor when a body is absent. The registered `NxPhysicsJointStagedPairTests` ten-family public matrix exercises this path through D6 global-anchor readback.

A temporary `out.x += 1.0f` mutation after the computed X result was applied specifically inside `Joint::getGlobalAnchor`, then `NxPhysics.dll` was rebuilt. The staged-pair differential detected the changed readback across the matrix: both processes exited zero, stderr matched exactly, and `stdout_delta=246`. The mutant candidate DLL SHA-256 was `2b3e10f86e16af58ff0c7060c199d9604cf3fab62481703baef3111bf5f874fe`.

After removing the mutation, `NxPhysics.dll` was rebuilt and the same differential returned to exact agreement (`stdout_delta=0`, both exits zero, exact stderr). The restored candidate SHA-256 was `b58e432b5d9b19af16f413fe5eecda0703b6004df993db1e8cbc73d4452bc86c`; the pinned oracle SHA-256 was `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Logs: `build/joint-global-anchor-get-row004125-mutant.log` and `build/joint-global-anchor-get-row004125-control.log`. The row is registered for Phase 6 and Phase 7 through the staged-pair target.
