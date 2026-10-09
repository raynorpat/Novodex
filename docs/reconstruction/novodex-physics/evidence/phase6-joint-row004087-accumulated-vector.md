# Phase 6 closure: joint accumulated vector (`phys_fn_004087`)

`Joint::row004087` accumulates `(numerator / divisor) * v` into the three-component vector at `this + 0x154`. The staged-pair probe seeds that vector with `(0.5, 0.5, 0.5)`, then calls the function with numerator `1.5`, divisor `1.0`, and `v = (2, 3, 4)`. The expected result is `(3.5, 5.0, 6.5)`.

The oracle is called at its pinned RVA `0x00095cc0`. The candidate is resolved from the `NxPhysics.map` copied beside the staged candidate DLL, using decorated symbol `?row004087@Joint@@UAEXMABVNxVec3@@M@Z`; the tested map placed it at RVA `0x0006ddb0`. Before calling it, the harness verifies the target lies in executable memory owned by the loaded candidate image. `run_differential.ps1` stages this map only for the candidate pair.

For falsification, an immediate return was inserted at the top of `Joint::row004087` and `NxPhysics.dll` was rebuilt. The oracle printed `40600000.40a00000.40d00000`, while the mutant candidate printed the untouched seed `3f000000.3f000000.3f000000`; the candidate assertion failed and the staged-pair differential reported `stdout_delta=1479`. After restoring `Joint.cpp` byte-for-byte and forcing recompilation, both sides printed `40600000.40a00000.40d00000`; both processes exited zero, `stdout_delta=0`, and stderr matched exactly.

The committed target is `NxPhysicsJointSlotTests`, registered for Phase 6. Logs from the isolated worktree: `build/phase-step-rows/row004087-mutant.log` and `build/phase-step-rows/row004087-restored-control.log`.
