# ThirdParty oracle-only proof (2026-10-14)

`NxPhysicsThirdPartyTests` now has a retained oracle-only proof bound to the pinned UE3 oracle (`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`) and normalized fixture source (`e38b3c88010c9d93f3bcb94871c1ec1300f009c3def53897d748e3be8f85a389`). The proof requires the `xorshift32` differential generator, the QHull/OPCODE version marker, 27 coverage lines, final coverage `driven=116 divergent=26 words=2615055 layout_checks=47`, and oracle digest `e2a266c1`. Its selected-line hash excludes candidate-side output.

The Phase 4 gate ran against the fresh registered build and passed all 519/519 coverage assertions. It invoked `oracle_proof:NxPhysicsThirdPartyTests` successfully. The reconstruction tooling suite passed 815 tests; the focused oracle-proof suite passed 23 tests after this addition.

The machine-readable baseline and self-hashed proof are in `oracle-only-baselines.json` and `oracle-only-proofs/NxPhysicsThirdPartyTests.json`. Raw differential and Phase 4 logs are gzip archived in `raw/phase4-thirdparty-oracle-proof-20261014/`.

This closes the ThirdParty target's M1 proof gap. The broad Collision target still needs an oracle-only proof, and this harness work does not close any of the DLL's full-image reconstruction rows.
