# Phase 6 closure: shared joint type and state wrappers

The registered `NxPhysicsJointStagedPairTests` differential observes type-query results for the joint families and state for each case. Each shared wrapper was mutated independently in production source, rebuilt, and detected.

For `phys_fn_004479` at 0x000b0d20, changed only `NpJointShared::is` to return null for every requested type. The oracle and candidate both exited 0 with exact stderr; the runner reported `stdout_delta=5961`. Mutant candidate SHA-256: `c189f4fc65c5e7801360a5759f95f15102a296fbfec5c089030d4ba89688e93a`. Restored clean candidate SHA-256: `7311f3dc9dc1a2ba632830331363fcad45d18813318bd56c24f0f9bef8dc1460`; the control returned `stdout_delta=0` with exact stderr.

For `phys_fn_004483` at 0x000b0dc0, changed only `NpJointShared::getState` to report `NX_JS_BROKEN`. The oracle and candidate both exited 0 with exact stderr; the runner reported `stdout_delta=244`. Mutant candidate SHA-256: `e4d006ff4d9a1e2e995a00e9b7740fa15c24ddc76592bb3eeacf65363a8199e0`. Restored clean candidate SHA-256: `57f1c67e54d79a695454fcc7196782831d689b24015e9d62104fa907bba400c2`; the control returned `stdout_delta=0` with exact stderr.

Build, mutation, and restored-control logs are retained in ignored `build/phase6-004479-*` and `build/phase6-004483-*` files.
