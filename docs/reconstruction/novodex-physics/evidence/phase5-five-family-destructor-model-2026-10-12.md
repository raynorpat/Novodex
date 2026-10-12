# Five-family collision-object destructor differential

The approved private layout uses each collision object's primary interface and
`EmbeddedHookBase` as a secondary base. Five final family classes install their
own primary vtable so MSVC emits the secondary deleting-destructor adjustor for
the complete object.

`NxPhysicsShapeVtableTests` calls each oracle and candidate secondary deleting
destructor on scratch copies with flags 0 and on heap objects with flags 1. It
checks the complete-object return pointer, both vptr transitions, x86 stack
balance, and allocator free count. The five-family differential reports
`mismatches=0`; the primary deleting-destructor checks also report zero. The
member-only mutation replaces the secondary table with the unadjusted
`EmbeddedHookBase` table and is detected in all five families
(`mismatches=5`). The Phase 5 gate registered this target and passed all
`2,665/2,665` required coverage assertions.

The gate was initially red before differential execution because inventory
validation rejected three newly closed Phase 7 local-pose rows: their detection
strings did not match the required `mismatches=N` form, the closure counts were
stale, and the bound evidence file did not name the rows with their measured
counts. The ledger counts and detection fields now match their entries, and the
evidence file records the three measured results. Inventory validation and the
fresh Phase 5 gate both pass.

Full transcript: [phase5-gate.log](raw/phase5-inventory-red-repair-20261012/phase5-gate.log).
