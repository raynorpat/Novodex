# `NxFluid` solver constructor (`phys_fn_003692`)

IDA at RVA `0x0008b000` shows the 53-byte constructor initializing the `NxFluid` primary base, calling the read-lock base initializer at `this+8`, zeroing base words at `+4`, `+0x0c`, and `+0x10`, installing the final primary and secondary vtables, storing the internal-fluid pointer at `+0x14`, returning the original `this`, and using the x86 `retn 4` convention.

`Physics/src/Scene.cpp` now implements `nxFluidSolverConstructor` and calls it from `NxSceneInternal::createFluid` after solver allocation. The enclosing initializer retains its descriptor `userData` store at `+4` after construction. No public Physics headers changed.

`NxPhysicsFluidEmitterAbiTests` calls the pinned oracle row and candidate constructor, checking both final vtables, all three zeroed words, the internal-fluid pointer, return identity, and x86 stack balance. The restored probe reports `oracle_primary=1 candidate_primary=1 oracle_secondary=1 candidate_secondary=1 zero_fields=1 user_data=1 return=1 stack=1 mismatches=0`.

For the committed-archive mutation, the `+4` zero-store was changed to `1` in an archive of commit `170fff77`, every `.cpp` and `.h` timestamp was refreshed as required by the repository mutation procedure, and the ABI target was rebuilt. The registered differential rejected the mutant: `zero_fields=0 mismatches=1` and `oracle_exit=1`. Restoring the committed source and rebuilding produced the exact zero-mismatch control.

Fresh local-main Phase 5 and Phase 7 gates pass at `2869/2869` and `3008/3008` registered coverage assertions. Raw mutation, restored differential, and gate transcripts are in [the committed-archive evidence directory](raw/phase7-fluid-solver-constructor-003692-committed/).
