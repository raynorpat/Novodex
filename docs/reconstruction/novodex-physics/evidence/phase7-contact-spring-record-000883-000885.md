# Phase 7 swept-capsule spring record (`phys_fn_000883` / `phys_fn_000885`)

The candidate reconstructs the two adjacent oracle spans as one `NxActorPair::row000883` implementation: `000883` prepares the contact record and `000885` builds its spring response values. The registered simulation fixture creates a swept capsule with `NX_MF_SPRING_CONTACT`, a live `NxSpringDesc`, and a static box. The oracle and restored candidate both deliver four contact callbacks and finish with the same captured actor state.

A no-op mutation replaced the combined candidate body and was rebuilt into `NxPhysics.dll`. The spring fixture then produced 24 callbacks instead of 4 and a different final actor state; the staged differential reported `stdout_delta=2` with both child processes exiting zero. Restoring the implementation returned the complete simulation transcript to `stdout_delta=0` with exact stderr. The focused logs are in [raw evidence](raw/phase7-contact-spring-record-000883-000885/).

Fresh Phase 5 gate: pass at 2,702/2,702 assertions. See [phase5-gate.log.gz](raw/phase7-contact-spring-record-000883-000885/phase5-gate.log.gz).

Fresh Phase 6 gate: pass at 1,635/1,635 assertions. See [phase6-gate.log.gz](raw/phase7-contact-spring-record-000883-000885/phase6-gate.log.gz).

Fresh Phase 7 gate: pass at 1,744/1,744 assertions. See [phase7-gate.log.gz](raw/phase7-contact-spring-record-000883-000885/phase7-gate.log.gz). Phase 7 now closes 249 functions and defers 311; the full-DLL reconstruction remains in progress.
