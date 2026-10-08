# Phase 5 vector push_back growth — `phys_fn_000028`

`phys_fn_000028` (`NxPhysics.dll+0x1b90`, 165 bytes) is implemented by `nxU32VectorPushBack` in `Physics/src/ObjectModel.cpp`. The candidate uses the VC9 vector header at offsets +0x04/+0x08/+0x0c, stores directly when capacity remains, and otherwise grows to `2*size+2` dwords through the Foundation SDK allocator, copies the live dwords, frees the prior block, and reseats the three cursors. The proxy at +0x00 is preserved.

The registered `NxPhysicsObjectLayoutTests` fixture calls the oracle and candidate across eight pushes from an empty vector and separately performs a push into reserved capacity. The clean baseline reports oracle capacity 14, three allocations totaling 100 bytes, two frees totaling 40 bytes, `vecgrow candidate ok=1`, matching digests `6f1d9bd7/8eac5155`, `candidate mismatches=0`, and exit 0. The reserved push performs no allocation and stores the value.

For falsification, changed only the candidate growth expression from `size + size + 2` to `size + size + 1`, rebuilt `NxPhysicsObjectLayoutTests`, and ran it against the pinned oracle. The candidate ended with capacity 15 instead of 14, reported `vecgrow candidate ok=0`, and the registered oracle differential rejected it with `candidate mismatches=2` and exit 1. Restoring the original expression, rebuilding, and rerunning returned capacity 14, both expected digests, `candidate mismatches=0`, and exit 0. The exact captured logs are `build/m2-vector-row-oracle-baseline.log`, `build/m2-vector-row-mutant.log`, and `build/m2-vector-row-restored.log`.

Closure: `oracle_differential_falsified` on `NxPhysicsObjectLayoutTests`, `mismatches=2`. No public Physics headers changed.
