# Buffered contact-report rows `phys_fn_000917`, `phys_fn_000919`, and `phys_fn_000921`

The three oracle code spans map to the shared candidate body `cpmBufferContactReports0917` in `Physics/src/ContactPairManager.cpp`. It evaluates contact events, fills the 0x2c-byte callback record, appends it to the Scene report vector, grows/copies the vector when required, and frees stale records. The registered `NxPhysicsSimulationTests` target already checks the normal plane/sphere callback transcript and deferred-queue delivery.

A row-scoped mutation set the shared pair-hash loop count to zero and explicitly rebuilt `NxPhysics.dll`. The independent deferred queue check still passed, while real simulation callbacks disappeared: oracle `calls=26`, mutant candidate `calls=0`. The oracle exited 0; the candidate failed the registered transcript check (`candidate_exit=1`, `stdout_delta=669`). Mutant candidate SHA-256: `83fcd356fe9ba09d880073ecb5b8c3f5731329368795e47e7cfb01eba520b34d`.

After restoring the body and rebuilding, oracle and candidate both reported `calls=26` with matching contact payloads; the staged differential was exact (`stdout_delta=0`, `stderr_exact=True`). Restored candidate DLL SHA-256: `3a28cc7c4f4358faf04c9dee97a0893e038a8fcc4244720696b22611a5a6ec1e`.

The mutation, build, and restored transcripts are compressed in [raw evidence](raw/phase7-contact-report-buffer-000917-000919-000921-20261013/). These closures cover the shared buffering implementation represented by these three spans; the row `phys_fn_000913` final scene-release flush remains a separate implementation and gate. Public Physics headers are unchanged.
