# Phase 6 closure: revolute descriptor load path

The index-0 revolute case now loads a second descriptor with distinct limits, motor, spring, projection, and flags, then saves and prints the readback through the public interface.

Two row-specific mutations were caught independently. `phys_fn_004332` was mutated by omitting `mLimit = desc.limit` from the family-field loader; the readback kept the earlier values and the registered differential reported `stdout_delta=2` (mutant candidate SHA-256 `1f2de15ddb67a60814b08b85aff95d27ee1f07e17bc9be3bfb3c7308ecf0c215`). `phys_fn_004370` was mutated by omitting the family-field loader call from `RevoluteJoint::loadFromDesc`; the readback likewise retained the earlier descriptor, producing `stdout_delta=4` (mutant candidate SHA-256 `5e03249ebb70c414a9f0ddaef711c9b5657e9d29c76da67859bb12970f7641c6`). Both oracle/candidate pairs exited 0 with exact stderr.

Restoring both rows returned the requested loaded values; the staged-pair differential passed with `stdout_delta=0` and exact stderr. Restored candidate SHA-256: `7d4b9d621957a4691ade6ec232cbf082d68d9301f1acef8aa5b0bc6464d72e5e`. Full logs are retained in the ignored `build/phase6-004332-*`, `build/phase6-004370-*`, and `build/phase6-revolute-load-*` files.
