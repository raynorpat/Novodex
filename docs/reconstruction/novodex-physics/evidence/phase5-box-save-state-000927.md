# Phase 5 box slot-13 save-state row

`phys_fn_000927` (`BoxShape::nxBoxSaveState`) copies the three box dimensions to descriptor record offsets `+0x4c`, `+0x50`, and `+0x54`, then reuses the base save-state routine. The registered `NxPhysicsObjectLayoutTests` box-row fixture constructs a unit box, poisons a `0x58`-byte output record, and compares the candidate record against the pinned oracle.

For the mutation audit, the first copied dimension was changed to `dimension + 1.0f`. The oracle produced digest `853c971d`; the mutant produced `ea972950`, reported `boxrow2 candidate ok=0`, and failed with `layout candidate mismatches=1` (exit 1). After restoring the source and rebuilding, the candidate reported `boxrow2 candidate ok=1`, digest `853c971d`, and `layout candidate mismatches=0` (exit 0).

The pinned oracle SHA-256 was `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`. The assertion floor remains 2,308 because this proof uses existing registered observations. No public Physics header or production behavior changed.
