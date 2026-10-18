# Contact stream subobject constructor (`phys_fn_002356`)

IDA's body at RVA `0x0005b680` constructs the `SdkContainer` at stream-object `+0x28`, calls the `phys_fn_002354` reset on the stream object, then returns the original object pointer. Its ABI is `thiscall`, with the object in ECX and no stack arguments.

The candidate now exposes an internal ABI-shaped `__fastcall` wrapper: ECX carries the stream object, EDX is ignored, and the wrapper returns that object. `NxActorPair::row000893` uses this wrapper. The registered simulation fixture calls the pinned oracle row or candidate wrapper on sentinel-filled scratch storage; it checks the ten reset words, container capacity and count, zero seed, growth factor, return identity, and untouched tail bytes, then releases the allocated container storage. Oracle and candidate report the same state, and the staged differential is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

For the row-specific mutation, a `git archive` of commit `8c96e165` was rebuilt after omitting the reset call in `cpmOpen002356`. The candidate failed the direct assertion (`zeroed=0 capacity=0 count=0 seed=0 matched=0`); the oracle passed, and the registered differential rejected the mutant (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=4108`). Restoring the archived source, rebuilding, and rerunning produced an exact differential.

Fresh Phase 5, 6, and 7 gates pass at 2,883/2,882, 1,743/1,742, and 3,022/3,021 coverage assertions. Public Physics headers are unchanged. Raw mutation, restored, and gate transcripts are compressed under `evidence/raw/phase7-contact-stream-constructor-002356-20261017/`.
