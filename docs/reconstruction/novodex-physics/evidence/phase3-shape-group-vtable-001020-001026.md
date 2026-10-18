# ShapeGroup vtable reconstruction (phys_fn_001020, 001022, 001026)

The candidate ShapeGroup vtable now implements all 15 entries from the pinned oracle table at .rdata RVA 0x106c2c. It includes compound raycast, cached world-sphere merge, child slot-3 result dispatch, inherited descriptor load/save, mass and owner operations, AABB aggregation, and shared sweep/no-op/identity entries. No public Physics headers changed.

NxPhysicsActorShapeMutationTests drives the actual group root table in paired processes. It checks the full slot mask (0x7fff), descriptor apply/save values and preserved bytes, two-child dispatch order and final result, masked nearest-hit raycasts, and the merged world sphere. The restored pair exits 0 on both sides with stdout_delta=0 and exact stderr.

Row-specific candidate table-entry mutations are rejected: removing slot 3 changes the dispatcher to result=0, calls=0, mismatches=1; removing slot 5 makes both ray queries miss (mismatches=1); removing slot 11 leaves the poisoned output sphere and reports mismatches=1. Removing inherited slots 1 and 2 is also rejected by their descriptor probes. Every mutation run leaves the pinned oracle passing and fails the candidate pair.

The registered marker lines are required by the Phase 5, 6, and 7 gate entries for NxPhysicsActorShapeMutationTests. The Phase 3 ledger discharges rows 001020, 001022, and 001026 through Phase 5. Raw restored and mutation transcripts are compressed in evidence/raw/phase3-shape-group-vtable-001020-001026-20261018/.

The fixture initially compared the group identifier as a 32-bit word, which included the adjacent material halfword. Reading the field as a 16-bit group value matches the oracle record and preserves the material comparison independently.

Fresh gates on the restored DLL pass: Phase 3 984/984, Phase 5 2,924/2,924, Phase 6 1,777/1,777, and Phase 7 3,060/3,060 assertions. Each of Phases 5, 6, and 7 requires and recognizes all six ShapeGroup marker lines. Inventory validation reports 6,338 function rows, 5,138 data objects, and zero unexplained entries. The 14 retained oracle-only proof records were re-linked to the current baseline manifest; the two ShapeVtable records were regenerated from Phase 5's fresh oracle-only transcript with the same 84 cases and matching input/output digests.

Viewer integration coverage was rebuilt and run with `ctest --test-dir build -C Release --output-on-failure -R '^Viewer'`. All 48 registered tests passed or were explicitly skipped by the suite: 43 passed and five demo scenes were skipped because their required assets are unavailable (`CowPile`, `PMapTest8`, `PMapTest10`, `PMapTest12`, and `TruckDemo`). This exercises every currently available Viewer scene entry and the Viewer platform, sound, and physics smoke tests.

The fresh reconstruction tooling suite passes all 823 tests. The public-header manifest verification passes against `Physics/include`; no public Physics header changed.
