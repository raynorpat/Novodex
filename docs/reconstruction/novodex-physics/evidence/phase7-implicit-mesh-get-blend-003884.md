# Phase 7: implicit-mesh `getBlend` (`phys_fn_003884`)

IDA identifies the 46-byte public `NpImplicitMesh::getBlend` body at RVA `0x0008d4a0`. It reports the feature-unavailable `NXE_DB_WARNING` at source line 58 and returns `0.0f`. The candidate Release map resolves `?getBlend@NpImplicitMesh@@UBEMXZ` at preferred address `0x1002e190` in `ImplicitMesh.obj`.

The registered `NxPhysicsSimulationTests` licensed-surface-mesh fixture now gives this accessor its own gate marker. It checks the exact return bits, one warning, error code 206, source line 58, and exact warning message. Oracle and candidate both report `matched=1`; the staged differential exits 0 on both sides with `stdout_delta=0` and exact stderr.

A candidate-only mutation changed the return from `0.0f` to `1.0f`. The new marker reported `value=3f800000 matched=0`; the oracle exited 0, candidate exited 1, and the staged transcript differed by 71 lines. Restoring the implementation returned to the exact control output.

Fresh Phase 5, Phase 6, and Phase 7 gates pass at 2,832/2,832, 1,719/1,719, and 2,965/2,965 coverage assertions. Detailed logs, the source mutation, and staged candidate identities are retained in `evidence/raw/phase7-implicit-mesh-get-blend-003884-20261017/`. Public SDK headers are unchanged. Other implicit-mesh rows and the full NxPhysics.dll reconstruction remain open.
