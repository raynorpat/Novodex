# Actor vtable wrappers `phys_fn_000038` and `phys_fn_000040`

The object-layout differential now requires exact slot forwarding for both actor wrappers. The fixture provides a safe vtable through slot `0x108`, gives each slot a distinct return record, and reports candidate mismatches as a hard failure.

The two rows are the inherited `NxActor::getPointVelocityVal(const NxVec3&)`
and `NxActor::getLocalPointVelocityVal(const NxVec3&)` virtuals. IDA decompiles
`phys_fn_000038` at `0x10002400` as a call through vtable byte offset `0x104`,
and `phys_fn_000040` at `0x10002430` as a call through `0x108`; both copy the
three-word returned `NxVec3` into the caller's hidden result buffer. The
prototypes and adjacent method order in the frozen `NxActor.h` match those
signatures. Private wrappers and the layout fixture now use these recovered
method names. This resolves the slot-identity question without changing public
headers or the dispatch behavior.

Mutation checks were independent:

- `phys_fn_000038`: route the slot `0x104` wrapper through `0x108`; detected as `mismatches=1`.
- `phys_fn_000040`: route the slot `0x108` wrapper through `0x104`; detected as `mismatches=1`.

After restoring both wrappers, `NxPhysicsObjectLayoutTests` reported `actorthunk candidate failures=0` and `layout result=differential-pass` against the pinned NxPhysics oracle.
