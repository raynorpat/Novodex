# Phase 7 closure: actor-pair contact-stream reset (`phys_fn_000863`)

`phys_fn_000863` at RVA `0x0001d0b0` is the fastcall tail thunk that passes
the actor-pair contact-stream subobject at `pair + 0x10` to row 002354
(`NxContactSinkResetState`). The per-step `NxPairNode::row000905` refresh calls
it when the pair's body-record pointers change before narrow-phase
reregistration.

The registered `NxPhysicsSceneVisualizeTests` target now provides a live pair
through an overlapping static plane and dynamic sphere, simulates and fetches
60 steps, and renders the contact-point stream. Oracle and restored candidate
each render three lines and match exactly (`stdout_delta=0`, both exits zero,
exact stderr).

A no-op mutation of `cpmActorPairResetStream0863` omits the row 002354 reset.
The candidate retains stale stream records and renders 78 lines instead of the
oracle's 3. The paired target rejects the mutant (`stdout_delta=87`, both
exits zero, exact stderr). Restoring the reset forward returns the differential
to `stdout_delta=0` with exact stderr.

The fresh full Phase 7 gate passes at 1,711/1,711 assertions. The ledger now
records 201 closed / 359 deferred Phase 7 functions. The restored candidate
DLL SHA-256 is
`803cb5d0de6bac5faabab76493e19c708dab634629c119a7d498a6400abcec3a`; the
pinned oracle DLL SHA-256 is
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

Raw paired transcripts and the full gate log are in
`evidence/raw/phase7-scene-contact-stream-reset-000863-20261012/`.
