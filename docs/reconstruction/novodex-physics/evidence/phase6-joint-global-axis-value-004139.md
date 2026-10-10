# Phase 6 mutation evidence: `phys_fn_004139` — global axis value return

The joint staged-pair harness now records actual value-return anchor and axis bit patterns, rather than relying only on a comparison that called the same implementation twice. The Phase 6 gate requires the family rows.

A temporary `+1.0f` X mutation in `Joint::getGlobalAxisVal` changed the registered `NxPhysicsJointStagedPairTests` transcript (`stdout_delta=244`, both exits 0, exact stderr). Mutant DLL SHA-256: `74b466dde168e268a53f6ef99195c60ac4db329e184466da7852182275954251`. After restoring the source and clean rebuilding, the staged differential matched exactly (`stdout_delta=0`, exact stderr); restored DLL SHA-256: `9db5d159746e828b20a333fe0261cb9e7599877184aaca292cc324eb9a1e20ce`. Full mutant and restored transcripts are retained beside this file.

The full Phase 6 gate passes with all ten staged-pair targets exact and 1,564 coverage assertions run (minimum required: 1,267). Phase 6 remains pending with 104 closed and 329 deferred function rows.
