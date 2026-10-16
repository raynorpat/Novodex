# FluidManager dirty-refresh null-collision result (`phys_fn_003649`)

IDA decompilation of the pinned oracle's `phys_fn_003649` at RVA `0x8a000`
shows that a successful first-fluid creation publishes the internal fluid and
public solver entries before dirty-collision refresh. The refresh forwards the
result of `FluidManagerGetStaticCollision` directly to `phys_fn_003505` at RVA
`0x861b0`; that function immediately calls `StaticCollisionCreate` with the
pointer and does not test it for null.

The callback fixture now returns null for that one lookup. Before the candidate
change, the differential was red: the oracle faulted with
`EXCEPTION_ACCESS_VIOLATION` after array counts reached 1/1, while the candidate
returned the fluid and cleared the dirty flag (`stdout_delta=7`). Removing the
candidate collision builder's null-pointer early return makes it dispatch the
same callback and reproduce the access violation. The restored differential
matches exactly (`stdout_delta=0`, `stderr_exact=True`) in both manager-growth
failure targets. Reinstating the guard is mutation-rejected by the new marker.

Phase 7 now requires the null-collision marker in both allocation-ordinal
targets, raising its floor from 2,932 to 2,934 assertions. This closes only
this dirty-refresh failure branch; `phys_fn_003649` and the full DLL
reconstruction remain open. No public headers changed.

Raw red, guard-mutation, restored differential, and fresh Phase 7 transcripts
are in `evidence/raw/phase7-fluid-dirty-refresh-null-collision-003649-20261016/`.
