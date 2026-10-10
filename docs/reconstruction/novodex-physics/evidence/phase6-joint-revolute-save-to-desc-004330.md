# Phase 6 closure: revolute `saveToDesc` (`phys_fn_004330`)

The registered revolute staged-pair fixture now serializes the full limit pair, motor, spring, projection, and flag fields returned by the public `NxRevoluteJoint::saveToDesc` call. The index-0 case first assigns distinct values through the public setters so descriptor readback can be distinguished from defaults.

A temporary mutation omitted only `desc.limit = mLimit` in `RevoluteJoint::saveToDesc`. The registered differential returned defaults for the saved limits and caught it with oracle/candidate exit 0, `stdout_delta=2`, and exact stderr. Mutant candidate SHA-256: `008e29668d5a8e61270fa37a2ff315ed5bad286eaad0e85d04352e01778fba13`.

Restoring the assignment returned the requested limit words; the restored differential passed with both exits 0, `stdout_delta=0`, and exact stderr. Restored candidate SHA-256: `267bf324ce4728b406a922a1456a0f621dae1c73a4283342f90841f2f81aa963`. Full logs are retained in the ignored `build/phase6-004330-*` files.
