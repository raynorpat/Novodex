# Capsule primary vtable frontier

The pinned capsule primary table begins at `.rdata` RVA `0x106b20` and contains 19 entries before adjacent string data. Its function RVAs, slots 0–18, are `225e0, 27740, 256f0, 21cd0, 22440, 22480, 266a0, 225d0, 21c80, 22620, 21c30, 21c60, 21ad0, 21b40, 21be0, 27920, 27f00, 27f00, 27f00` (hex). The final candidate table is **not installed yet**: slot 3 is a 0x758-byte capsule renderer, and the other inherited/derived rows still need callable-table verification.

Slot 7, `phys_fn_001012` at `0x225d0`, writes a zero dword through its first argument and returns false with `ret 8`; the second argument is unread. `CapsuleShape::nxCapsuleSweepZero` now matches four oracle cases with different initial output patterns and a poisoned unread pointer.

Slot 0, `phys_fn_001014` at `0x225e0`, destroys the collision object through its deleting entry, runs the base/prunable chain, and frees the capsule when flags bit 0 is set. The candidate now follows that path. Stack and heap drives agree with the oracle's allocator free deltas of one and two.

Slot 5, `phys_fn_001010` at `0x22480`, is the existing capsule raycast implementation. Its body moved unchanged from `ContactGeneration.cpp` to `ShapeRaycast.cpp` so the isolated shape differential can link it. Sixteen oracle-slot versus candidate-direct ray/limit/normal-hint cases match bitwise, including the deliberately untouched normal field. `NxPhysicsCollisionTests` still reports `collision=pass` after the move.

The pinned `NxPhysicsShapeVtableTests` transcript now reads `shape vtable oracle_digest=9e9340bb cases=227 failures=0`. The capsule comparisons contribute 22 cases; the capsule table itself and the remaining shape/actor tables are still required before the Phase 5 vtable gate can turn green.
