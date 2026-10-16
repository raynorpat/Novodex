# Sub-word stride edge for `phys_fn_003423`

The registered `NxPhysicsFluidStrideAlignmentTests` now tests both the existing 13-byte alignment case and 3-byte position, velocity, and lifetime strides. The public `NxParticleData` validation accepts these nonzero strides; the oracle masks each with `~3`, so all become zero. It reads the same first position for both particles and does not copy optional velocity or lifetime data. The restored candidate matches this output exactly.

A candidate mutation that uses the raw position stride is rejected by the same actual-DLL differential: the aligned stride case and sub-word position values mismatch (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=4`). Restoring stride rounding returns exact output (`stdout_delta=0`, `stderr_exact=True`). This proves another input edge for row `phys_fn_003423`; the row remains open for Phase 8's full instruction-level audit.

The fresh Phase 7 gate passes at 2,942/2,942 registered assertions. Gate-target tests pass 39/39; inventory validation reports zero unexplained rows. Oracle DLL SHA-256: `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. Restored candidate DLL SHA-256 is recorded in the adjacent manifest.
