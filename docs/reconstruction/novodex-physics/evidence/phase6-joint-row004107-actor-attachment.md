# Phase 6 closure: joint actor attachment (`phys_fn_004107`)

`Joint::row004107` binds a joint to its two actor body records, records their stamps, orders solver bodies according to the joint flag, and raises their wake counters. The registered ten-family `NxPhysicsJointStagedPairTests` matrix creates public joints with actors and observes actor identity, internal state, and release.

With the matrix exact against the pinned oracle, inserted an immediate return at the start of `row004107` and rebuilt NxPhysics.dll. The oracle exited zero; the candidate access-violated during joint construction (`candidate_exit=-1073741819`) and the staged differential reported `stdout_delta=3160` with exact stderr. The mutation candidate Physics DLL was SHA-256 `40693a0a15001121bf0e9cbfb64f3e747787efcf2f112664cc870374d2e79a7b`.

After restoring the source and rebuilding, the same registered differential returned to exact output (`stdout_delta=0`, both exits zero, stderr exact). The restored candidate Physics DLL was SHA-256 `39afd985fadd888f7769891fa05c5be725a1546cdbe3182e6ac4f5f2fbc33917`; the pinned oracle Physics DLL is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

Build root: `D:\github\Novodex\build\m0-current-main-clean`.

Logs: `build/joint-attach-row004107-baseline.log`, `build/joint-attach-row004107-mutant.log`, and `build/joint-attach-row004107-restored.log`.
