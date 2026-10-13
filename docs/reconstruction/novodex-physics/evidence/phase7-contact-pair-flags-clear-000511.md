# Actor-pair flag clear helper (`phys_fn_000511`)

`cpmClearActorPairFlags` removes an existing actor-pair hash entry and frees
its allocated report record. The earlier pair-flag fixture only set a pair
once, so a deliberate no-op mutation remained invisible.

`NxPhysicsPairFlagTests` now creates a separate single-shape actor pair, stores
`NX_NOTIFY_ON_TOUCH` (the allocated-record path), clears the flags with a zero
value, and asserts that `getActorPairFlags` returns zero. The test prints the
required Phase 7 marker `pairflag actor_pair_cleared flags=00000000`.

A throwaway no-op mutation of `cpmClearActorPairFlags` left the old value
visible as `00000008`; the candidate exited 1 and the oracle/candidate
differential reported `stdout_delta=5`. After restoring the source byte for
byte and rebuilding, the marker was zero on both sides and the differential
was exact (`stdout_delta=0`, `stderr_exact=True`). The focused transcripts and
fresh full Phase 7 gate are retained in
[`raw/phase7-contact-pair-flags-clear-000511`](raw/phase7-contact-pair-flags-clear-000511/).

This closes only the clear/remove-and-free behavior of row 000511. The broader
contact-reporting gaps described in [contact-reporting.md](contact-reporting.md)
remain open. No public header changed.
