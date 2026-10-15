# Phase 6 — `phys_fn_004119` Joint scalar deleting destructor

The base Joint scalar deleting destructor is emitted by MSVC from the existing private `Joint::~Joint` and `Joint::operator delete`; no production implementation or public header changed. The oracle body at RVA `0x98750` resets the base vptr, removes an attached joint from its Scene, purges limit planes, and calls the Foundation SDK allocator only when delete flag bit 0 is set.

`NxPhysicsJointSlotTests` now calls the oracle RVA and candidate map-resolved `??_GJoint@@UAEPAXI@Z` on zeroed scratch records with a concrete joint vptr. Flag 0 must return `this`, reset the vptr, and preserve storage; flag 1 must return `this` and free exactly the 0x16c-byte record. Oracle and restored candidate both report `returned=1 vptr_reset=1 frees=0,1 mismatches=0`; the staged transcript is exact (`stdout_delta=0`, stderr exact).

The no-op allocator-delete mutation was built in an isolated archive of `776487b3`. It reports `frees=0,0 mismatches=1` and exits 1 (`stdout_delta=87`), so the fixture falsifies the delete branch. The Phase 6 and Phase 7 coverage floors increase by one. Raw compressed transcripts and artifact hashes are in `raw/phase6-joint-base-delete-004119-20261015/manifest.json`.
