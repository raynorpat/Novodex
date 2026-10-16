# Phase 7 `phys_fn_003503`: fluid creation callback order

The enabled-fluid simulation fixture now records the callback sequence spanning backend fluid creation, descriptor-emitter creation, and initial-particle array allocation. Its registered assertion requires `C → E → P` on both the shipped oracle and the candidate DLL. This extends the prior `EP` check, which did not observe whether `FluidCreate` preceded the two descriptor initialization callbacks.

The staged `NxPhysicsSimulationTests` differential passes with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and `stderr_exact=True`. The full Phase 7 gate passes at 2,913/2,913 coverage assertions. The reconstruction-tool suite passes 835 tests and 743 subtests; inventory validation reports 6,338 function rows, 5,138 data objects, and zero unexplained records. Public Physics headers are unchanged.

This assertion documents only the callback order exercised by its one-emitter, one-initial-particle case. It does not close `phys_fn_003503`; the rest of the callback table and full constructor body remain open.
