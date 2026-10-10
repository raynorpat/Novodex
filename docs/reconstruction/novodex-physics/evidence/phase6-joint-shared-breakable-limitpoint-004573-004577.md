# Phase 6 closure: shared breakability and limit-point getters

The D6 staged case sets and reads breakability and limit-point state through shared NpJoint wrapper slots. Each wrapper was mutated independently in production source and detected by the registered `NxPhysicsJointStagedPairTests` differential.

For `phys_fn_004573` at 0x000b1bd0, omitted only the internal `getBreakable` readback. Oracle and candidate both exited 0 with exact stderr; the runner reported `stdout_delta=2`. Mutant candidate SHA-256: `2926b74877a4821f4b39f4b8539958392a4a4853be8d294e938df0e9ecff3b2d`. Restoring the row returned the control to `stdout_delta=0` with exact stderr; restored candidate SHA-256: `21a1cf67f26bca8ea3e4077572f6e3647032e20d6c844c45e621523d073e3a5d`.

For `phys_fn_004577` at 0x000b1c60, inverted the returned `getLimitPoint` presence flag. Oracle and candidate both exited 0 with exact stderr; the runner reported `stdout_delta=2`. Mutant candidate SHA-256: `8fbb6b20e07defbc291033bd902d12c6b97be10f87325a6b6b933190aab50d7d`. Restoring the row returned the control to `stdout_delta=0` with exact stderr; restored candidate SHA-256: `82e69e457661b60568814b96d96cb18c44de8e00afd4787526301bf3b3c01d75`.

Build, mutation, and restored-control logs are retained in ignored `build/phase6-004573-*` and `build/phase6-004577-*` files.
