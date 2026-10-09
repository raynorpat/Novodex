# Phase 6 closure: shared joint base descriptor loader (`phys_fn_004121`)

`Joint::loadFromDescBase` copies both local axes and normals, derives each frame quaternion, and restores the base joint configuration used by all joint families. The registered `NxPhysicsJointStagedPairTests` matrix creates public joints from non-default descriptors and records family-specific internal state and descriptor round-trips.

With the staged matrix exact against the pinned oracle, inserted an immediate return at the start of `loadFromDescBase` and rebuilt NxPhysics.dll. The registered differential caught the missing base descriptor state with `stdout_delta=2806`, both processes exiting zero, and exact stderr. The mutation candidate Physics DLL was SHA-256 `d860c3dd6bf411304cf565dfe9d7cd3438e15d1e9d843969d9cd9e4258226392`.

After restoring the source and rebuilding the DLL, the same differential returned to exact output (`stdout_delta=0`, both exits zero, stderr exact). The restored candidate Physics DLL was SHA-256 `31fbce3ecd1296e8c55ab4c2cb877ac43b4c172af9bceddd09c15d7cd271d6ca`; the pinned oracle Physics DLL is `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

Build root: `D:\github\Novodex\build\m0-current-main-clean`.

Logs: `build/joint-desc-load-row004121-baseline.log`, `build/joint-desc-load-row004121-mutant.log`, and `build/joint-desc-load-row004121-restored.log`.
