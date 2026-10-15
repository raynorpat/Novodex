# `Row000713Fixture::row000713` proof (`phys_fn_000713`)

The Phase 3 `NxPhysicsJointSupportTests` fixture calls oracle RVA `0x00015d50` and the linked candidate method on self-root, one-link and two-link record chains. It compares both the returned root and the path-compressed `+0x1e8` cache words. The restored oracle and candidate digests match at `729c7b9811e2dcf5`.

Changing the candidate method to return `this` without resolving the parent chain is rejected in all three cases (`mismatches=3`). The full Phase 3, 6 and 7 gates pass at 984/984, 1666/1666 and 2878/2878 assertions. Phase 3 discharges this Phase 2-owned row. Its raw build, differential, oracle-proof and gate transcripts are in `evidence/raw/phase2-row-000713-20261015/`.
