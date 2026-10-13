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

Fresh gate rerun on 2026-10-12 exposed a runner contract bug: this probe
requires the staged Physics DLL hash and performs an in-process source-vs-oracle
comparison, so it must run once against the oracle and must not be relaunched
with candidate RVAs interpreted against the candidate image. The differential
runner now passes the staged hash, validates the probe's explicit loaded-DLL
identity, and treats its linked candidate implementation as the candidate
side. The corrected full Phase 5 gate passes at 2,670/2,670 assertions. A fresh
focused transcript is [shape-vtable-differential.log](raw/phase5-five-family-destructor-model-20261012/shape-vtable-differential.log).

Current-main revalidation on 2026-10-13 passes the fresh Phase 5 gate at 2,678/2,678 assertions (phase_gate=5 status=pass). Candidate DLL SHA-256: 100584a3191d3cda2087ed9bc745446325e1c2cd26c520ecbb975fc7470196bc.

The 2026-10-13 rerun from the fluid-emitter integration worktree found the
Phase 5 gate still expected the earlier enabled-fluid summary line, even
though `NxPhysicsSimulationTests` now reports array agreement and the emitter
callback counts. Updated the required coverage line to match the measured
record; the full fresh gate passes at 2,678/2,678. This run again reports zero
mismatches for the five secondary and primary destructor families, and the
member-only model mutation reports five mismatches. Candidate DLL SHA-256:
83268060327066531df916359eebff96fec77e568a50bd7d96a7e2917e1321b4. Focused
transcript: [phase5-focused-gate.txt](raw/phase5-five-family-destructor-model-20261013/phase5-focused-gate.txt).
