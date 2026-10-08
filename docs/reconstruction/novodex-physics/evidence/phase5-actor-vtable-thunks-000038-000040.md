# Actor vtable wrappers `phys_fn_000038` and `phys_fn_000040`

The object-layout differential now requires exact slot forwarding for both actor wrappers. The fixture provides a safe vtable through slot `0x108`, gives each slot a distinct return record, and reports candidate mismatches as a hard failure.

Mutation checks were independent:

- `phys_fn_000038`: route the slot `0x104` wrapper through `0x108`; detected as `mismatches=1`.
- `phys_fn_000040`: route the slot `0x108` wrapper through `0x104`; detected as `mismatches=1`.

After restoring both wrappers, `NxPhysicsObjectLayoutTests` reported `actorthunk candidate failures=0` and `layout result=differential-pass` against the pinned NxPhysics oracle.
