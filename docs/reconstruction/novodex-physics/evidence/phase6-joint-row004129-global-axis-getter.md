# Phase 6 closure: shared joint global-axis getter (`phys_fn_004129`)

`Joint::getGlobalAxis` transforms the first world axis through the first solver body's pose and local frame, or copies the world axis when no body is present. The registered `NxPhysicsJointStagedPairTests` ten-family public matrix exercises the getter across joint families.

A temporary `out.x += 1.0f` mutation after the computed X result was applied inside `Joint::getGlobalAxis`, then `NxPhysics.dll` was rebuilt. The staged-pair differential caught changed axis readbacks with both processes exiting zero, exact stderr, and `stdout_delta=246`. Mutant DLL SHA-256: `d67f0ffaf08dbd18cff5165de29670e1104d2a15760a04d5221499ffe1a331c`.

After restoring `Joint.cpp` and rebuilding, the same differential returned to exact agreement (`stdout_delta=0`, both exits zero, exact stderr). Restored candidate DLL SHA-256: `dc23068c6ebd145930b9c71ac6ac4ea5b0238a156a41b10d139b410ad49e5453`; pinned oracle DLL SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Logs: `build/joint-global-axis-getter-row004129-mutant.log` and `build/joint-global-axis-getter-row004129-control.log`.
