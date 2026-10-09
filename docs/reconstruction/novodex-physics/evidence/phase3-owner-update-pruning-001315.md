# Shape owner-update pruning path (`phys_fn_001315`)

`phys_fn_001315` (`0x266a0`) had its pose composition reconstructed, but the
listing's owned-shape arm was missing: when `+0xdc` bit 1 is clear and `+0xa0`
names a pruning collection, it appends the shape to the collection's
`SdkContainer` at `+0x78`, sets the bit, clears `Prunable::mFlags` bit 1, and
calls the selected pruner's vtable slot 3.

`NxPhysicsObjectLayoutTests` now calls the pinned oracle entry and the
candidate method with separate fake static bodies, scenes, pruning collections,
and pruners. The slot-3 recorder checks the shape ID and flags at dispatch; the
test also reads back the collection count and entry identity. The existing
public actor-pose fixtures continue to cover transformed poses and group-child
updates.

The focused fixture was first run against the old candidate. Its red result was
expected: the oracle appended the shape while the candidate left the list
empty, though both dispatched slot 3 with the same cleared flags:

```text
phys_fn_001315 ownerupd pruning oracle_ok=1 candidate_ok=0 oracle_list=1/00315a51 candidate_list=0/00000000 oracle_update=1/00315a51/00000000 candidate_update=1/00315a51/00000000 mismatches=1
FAIL phys_fn_001315 pruning-list/update arm differs
```

After implementing the listing's append and flag order, the exact same probe
reports:

```text
ownerupd pruning oracle_ok=1 candidate_ok=1 oracle_list=1/00315a51 candidate_list=1/00315a51 oracle_update=1/00315a51/00000000 candidate_update=1/00315a51/00000000 mismatches=0
layout result=differential-pass
```

The row-specific negative control omitted the append and count increment. It
reproduced the red result and exited 1. The full Phase 5 gate then passed with
2,612 required coverage assertions, including the new owner-update line.
Oracle identity was pinned to SHA-256
`4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.
