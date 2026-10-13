# Phase 7 closure: contact-pair visualization thunk (`phys_fn_000907`)

`phys_fn_000907` at RVA `0x0001fda0` is the 8-byte `NxPairNode` thunk:
`add ecx, 0x14; jmp 0x1001d2d0`. It forwards to the actor-pair contact
visualizer at `NxPairNode::pair()->row000869`.

The registered `NxPhysicsSceneVisualizeTests` fixture now creates an isolated
scene with an overlapping static plane and dynamic sphere, enables actor-pair
touch notifications, simulates and fetches 60 steps, then clears other
visualization parameters and enables contact-point rendering. The oracle and
restored candidate each render three contact-point lines; their staged
transcripts match exactly (`stdout_delta=0`, both exits zero, exact stderr).
The fixture emits `contact_lines=observed` only when the renderer reports a
nonzero line count, and Phase 7 requires that marker.

A no-op mutation of `NxPairNode::row000907` leaves the oracle's three contact
lines intact but removes all three candidate lines. The target fails with
`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=9`, and exact stderr. The
restored source was rebuilt and returned to an exact differential.

The fresh full Phase 7 gate passes with 1,711/1,711 registered coverage
assertions. The ledger now records 200 closed / 360 deferred Phase 7 functions.
The candidate DLL SHA-256 is
`9c8a0f99cdf1f836bef0dbbbb899942c95c9c43724cb9b4707f6aea5b38fc97c`; the
pinned oracle DLL SHA-256 is
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

Raw paired transcripts and the full gate log are in
`evidence/raw/phase7-scene-contact-pair-000907-20261012/`.
