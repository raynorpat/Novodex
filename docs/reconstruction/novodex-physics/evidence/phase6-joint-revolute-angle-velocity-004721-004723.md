# Phase 6 closure: Revolute angle and velocity getters

The four-case staged Revolute fixture now reads `getAngle` and `getVelocity` and records both float results. Independently adding `+1.0f` to `phys_fn_004721` (`getAngle`) changed the transcript with `stdout_delta=28`; the `phys_fn_004723` (`getVelocity`) mutation produced the same delta. Both mutants had zero exit codes and exact stderr, and each restored control returned `stdout_delta=0` with exact stderr.

Mutant candidate SHA-256 values: `004721` `ecb1654de6b5d73c8f548f53b6e6c8e9ae80e73ebed0c0346fcd02e9d423dd29`; `004723` `0d688ee07862367c6cb3712c272c91830e05efb0095cf8bbc1b0549035f2c92f`. Restored clean candidate SHA-256: `e55d3d9dfe616e268f04ca6132a66c559e24ba2d805313074effc08067cda7bb`. The updated oracle-only matrix pins fixture source SHA-256 `f7b6b6c4993f13744e4e158007b95abf952a8f0819fd6e8c5f39bf9c8e5c4e9a`, 3,226 output lines, and digest `5ecb9a8cc222e863581c20674aa55b37f9b93b46e74d515c74c590df7c0926fa`.
