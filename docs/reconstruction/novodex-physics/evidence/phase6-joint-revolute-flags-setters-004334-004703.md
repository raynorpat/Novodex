# Phase 6 closure: revolute flag wrapper and internal setter

The revolute index-0 staged-pair case now sets flags to `0x05` through `NxRevoluteJoint::setFlags` after enabling the default limit, motor, and spring bits, then prints the public getter result. The required value is `00000005`.

Two mutations were tested independently. Omitting `mInternal->setFlags(flags)` in `NpRevoluteJoint::setFlags` left the value at `00000007` (`phys_fn_004703`, mutant candidate SHA-256 `a5327e662cc1652c929291180c1e2edc756c50a18a7c9db52579eac645d09c86`). Omitting `mRevoluteFlags = flags` in `RevoluteJoint::setFlags` also left it at `00000007` (`phys_fn_004334`, mutant candidate SHA-256 `76bbe261824f6231e223d56ba738772fd38f0bb18691905eff5e4afb8ddad668`). The registered differential caught each mutation with both exits 0, `stdout_delta=2`, and exact stderr.

After restoring both statements and rebuilding, the public getter returned `00000005`; the restored differential had both exits 0, `stdout_delta=0`, and exact stderr. Restored candidate SHA-256: `a3049afbca1ba46437559f8202085f7eb9d53be69f7af54e9a29bcad705bead0`. Full logs are retained in the ignored `build/phase6-004334-*`, `build/phase6-004703-*`, and `build/phase6-revolute-flags-*` files.
