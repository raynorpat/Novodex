# Scene pair-flag getters (`phys_fn_000513`, `phys_fn_000515`)

IDA decompilation distinguishes the Scene getter paths from the public
`NpScene` wrappers. `phys_fn_000513` rejects an identical shape pair with the
invalid-parameter diagnostic at `Scene.cpp` line 916, then reads the stored
pair value. `phys_fn_000515` rejects identical actors at line 948, resolves
each internal actor's first shape, and forwards through the shape getter.
`NpScene::getActorPairFlags` holds its scene read lock while forwarding those
internal actor pointers.

The prior candidate called the pair-hash helpers directly from the public
wrappers, so both same-object diagnostics were missing. `NxPhysicsPairFlagTests`
now checks the same-shape and same-actor getter paths, along with their valid
stored-value cases. Before the implementation change the new assertions were
red (`candidate_exit=1`, `stdout_delta=21`). After routing through the private
Scene methods, the focused differential is exact.

Independent no-op mutations reject each getter: `phys_fn_000513` reports
`stdout_delta=23`; `phys_fn_000515` reports `stdout_delta=11`. Restoring each
source version returns an exact differential (`stdout_delta=0`,
`stderr_exact=True`). The full Phase 7 gate and all raw transcripts are stored
in [`raw/phase7-contact-pair-flag-getters-000513-000515`](raw/phase7-contact-pair-flag-getters-000513-000515/).

The two rows are independently mutation-falsified. Broader contact-reporting
gaps listed in [contact-reporting.md](contact-reporting.md) remain open. No
public header changed.
