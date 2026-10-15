# Phase 6: Effector base destructor (`phys_fn_003936`)

IDA decompilation at RVA `0x0008eeb0` shows the 12-byte base destructor restoring the `Effector` vtable and calling `NxFoundation::Observable::~Observable`. The candidate expresses that contract as an empty, out-of-line C++ destructor; MSVC emits the required base-destructor call.

The registered `NxPhysicsEffectorTests` staged-pair differential releases spring-and-damper effectors through the derived deleting-destructor chain, reaching this base destructor. In a candidate-only mutation, the destructor was changed to a naked immediate return, suppressing the compiler-generated `Observable` teardown. The pinned oracle exited 0 and the mutant candidate access-violated (`candidate_exit=-1073741819`, `stdout_delta=75`, exact stderr). This proves the registered lifecycle fixture detects the row's base-destruction behavior.

After restoring the original destructor and forcing a CMake Release rebuild, the oracle and candidate both exited 0 with `stdout_delta=0` and exact stderr. The fresh full Phase 6 gate passes at `1,666/1,666`; its compressed transcript, plus mutant and restored focused transcripts, are in `evidence/raw/phase6-effector-base-destructor-003936-20261015/`.

The user-approved scene-dump date/time assertion was also checked in a fresh Win32 Release build: `NxPhysicsCoreDumpTests` matched the pinned oracle exactly. The full Phase 6 gate passed at `1,666/1,666` coverage assertions. No public headers changed. This closes only `phys_fn_003936`; the remaining Phase 6 and full-DLL reconstruction work remains open.
