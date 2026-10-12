# Fluid-emitter backend attributes (`phys_fn_003569`–`phys_fn_003591`)

The rebuilt wrapper now implements the twelve scalar and vector callback helpers used by `NxFluidEmitter`: dimension X/Y, random position, random angle, fluid velocity magnitude, rate, and particle lifetime. It also reconstructs six public vtable wrappers that were still discovered: the random-position getter/setter and the random-angle, velocity-magnitude, rate, and lifetime setters. The public headers remain unchanged. Public setters retain their write-lock and callback re-entry checks; getters take the wrapper's read lock. The helpers extract the callback context, fluid handle, and emitter handle from the observed internal layout. The random-position getter copies the callback's three returned float words into the caller's `NxVec3` result.

The registered `NxPhysicsFluidEmitterAbiTests` fixture calls every operation through both the in-process probe and the rebuilt candidate DLL, then compares callback selection, all three callback arguments, setter payload words, and getter results against the pinned oracle (`NxPhysics.dll`, SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`). The restored run reports:

```text
fluid emitter attribute rows=12 cases=12 candidate_dll_cases=12 callbacks=24 mismatches=0
```

Each helper row was independently falsified by changing only its emitter callback argument (`emitter ^ 1`). All twelve mutants exited nonzero and reported one mismatch at the attribute gate. Each of the six public wrapper rows was also independently replaced with a no-op; the candidate DLL differential rejected every mutation (three mismatches for each setter and seven for the vector getter). Restoring the source byte-for-byte, rebuilding, and rerunning returned exit 0 with zero mismatches. The per-row mutation, build, stderr, restored-control, and full Phase 5/7 gate logs are under [evidence/raw/phase7-fluid-emitter-attributes-003569-003591](raw/phase7-fluid-emitter-attributes-003569-003591/). The final gates passed 2,667/2,667 Phase 5 assertions and 1,694/1,694 Phase 7 assertions.

The public setter mutants verify that the candidate DLL enters the wrapper and dispatches its backend callback. Lock rejection and callback re-entry checks are reconstructed from the oracle wrapper bodies but need their own negative-path fixture. Other still-missing emitter interface methods remain open. Final registered gates passed at Phase 5 `2667/2667` and Phase 7 `1694/1694`; the 810 reconstruction-tooling tests and inventory validation passed.
