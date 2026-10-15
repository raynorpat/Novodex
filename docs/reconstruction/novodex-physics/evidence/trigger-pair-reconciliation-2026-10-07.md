# Trigger-pair reconciliation — 2026-10-07

`phys_fn_002350` at `NxPhysics.dll+0x0005ae50` reconciles the scene's previous
and current trigger-pair lists. IDA pseudocode shows it builds a temporary
hash-chain index over the previous pairs using the packed shape IDs at `+0xd4`.
It classifies current pairs as enter (`1`) or stay (`4`), marks prior matches,
then rechecks unmatched old pairs through the shape-type overlap dispatch at
`+0x94`. A prior pair that still overlaps is reinserted and reports stay;
otherwise it reports leave (`2`). Finally, the list headers are swapped and
the new current list is reset.

`Physics/src/Scene.cpp#nxSceneProcessTriggerPairs` now follows that structure:
it uses the native integer hash mixing and arithmetic-shift behavior, power-of-
two buckets, collision chains, and a per-pair match bitmap. The scratch arrays
are allocated in the processing caller's stack frame so their lifetime covers
all lookups. The overlap fallback retains sleeping bodies that are still
inside a trigger even when the broadphase omits them.

The public simulation corpus covers one pair through enter/stay/leave, 32
simultaneous pairs through the same transitions, and two trigger shapes on one
actor overlapping the same dynamic sphere. The 32-pair fixture exercises the
hash-chain lookup, overlap retention and event-buffer growth at a larger batch
size; oracle and candidate output match exactly (`oracle_exit=0`,
`candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`,
`build/trigger-pair-32-differential.log`). The compound fixture verifies each
shape receives exactly one enter, stay, and leave callback with its own shape
handle; the pinned oracle and candidate transcripts match exactly
(`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`,
`build/trigger-compound-differential.log`). The batch actors sleep while
overlapping, then move outside. It also configures a trigger with only
`NX_TRIGGER_ON_STAY`: the pinned oracle still reports enter, stay, and leave,
so the candidate now mirrors the observed rule that any enabled trigger event
bit admits all transitions. Both focused differentials are exact; the expanded
batch transcript is recorded in
`build/trigger-pair-eight-mask-restored.log` (`oracle_exit=0`,
`candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). The clean candidate
also passed the registered simulation differential from a throwaway archive
of `1e23e707` (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`,
`stderr_exact=True`). A second build from that archive changed the matching
branch in `nxSceneFindTriggerPair` to return `-1`. The pinned control still
passed; the mutant emitted a second enter and duplicate leave, reported
`trigger-lifecycle summary ... exact=0`, and exited 1. The archive runner's
identity check stops before it prints a whole-target delta for failing
children, so the focused mismatch is recorded directly rather than claiming a
`stdout_delta` from that runner. The staged oracle and mutant Physics hashes
were `4b7db3e1...79602c` and `d846ee82...68fe86` respectively. This formal
falsification covers lookup matching in the lifecycle fixture.

The direct `NxPhysicsCollisionTests` fixture seeds two identical previous
pairs and two identical current pairs, using a box trigger followed by a
sphere. This exercises the oracle's type-order swap before it selects the
sphere/box overlap slot. With overlap returning true, the oracle matches both
current entries to the same prior entry, then rechecks the unmatched duplicate
and appends it as a third stay (`oracle_events=3`, `oracle_pairs=3`). With the
slot populated but returning false, it emits two stays and one leave and
retains two current pairs. With the sphere/box slot null, it reports the same
two stays and one leave through the missing-dispatcher branch. A retained old
pair with an empty current list exercises both vector-growth paths: oracle and
candidate each allocate one current-pair array and one event array, then
produce one stay and one current pair. A partially filled current list then
forces growth while retaining an existing pair: both sides copy that pair,
append the retained old pair, allocate two arrays and free the replaced pair
buffer once. An empty previous/current pair-list case verifies the oracle's
header swap and empty-list reset. Candidate state and allocator call counts
match all six oracle cases exactly.

The retained-pair fixture checks all 36 ordered pairs of six shape types with
the trigger on either side and event-mask bytes 0 through 15 (1,152
oracle/candidate cases). This exposed two mismatches: after sorting by type,
the oracle puts the flagged trigger first in the retained pair; and, once a
pair is in the trigger-pair list, the oracle emits its transition even when
the low event-mask bits are zero. The candidate now matches both rules. The
matrix compares event identity, pair-list order, and allocations. Disabling
the trigger-orientation swap produces 36 mismatches; restoring the candidate's
old low-bit event filter produces 144. The restored matrix reports zero
mismatches (`build/trigger-pair-duplicates-restored-full-collision.log`).
The expanded full collision harness reports `collision=pass` with the matrix
at 1,152/1,152 (`build/trigger-pair-event-mask-full-collision.log`). The
registered Phase 3 gate passed at 971/971 coverage assertions for the null-slot
matrix (`build/phase3-trigger-event-mask-gate.log`); the event-growth assertion
is included in the current 973-line Phase 3 gate run.
The rebuilt production DLL also passes the public simulation differential with
exact enter/stay/leave callbacks (`stdout_delta=0`, `stderr_exact=True`,
`build/trigger-pair-event-mask-simulation-differential.log`).

The null-dispatch branch now has a second matrix over all 21 unordered pairs
of the six shape types. For each case, only that overlap slot is null while
the remaining slots are populated; oracle and candidate both emit one leave,
retain no pair, and make one event allocation. The restored fixture reports
21/21 matches (`build/trigger-pair-null-slot-full-collision.log`). A mutation
that treated a missing overlap callback as a positive overlap produced 21
mismatches, one per slot, then was reverted.

An event-buffer growth case starts with one existing event and a full buffer,
then processes one retained pair. Both implementations preserve the old
record, append the stay event, retain one pair, and perform one allocation and
one free. Omitting the candidate copy loop is rejected by the new assertion.

A temporary unconditional `return -1` in `nxSceneFindTriggerPair` is rejected:
the overlap-true case reports four candidate events and four pairs, while the
false and null-slot cases report four candidate events. The fixture exits 1.
After restoring the source and rebuilding, the full collision harness passes
with `mismatches=0`
(`build/trigger-pair-duplicates-restored-full-collision.log`). Broader
pair-list allocation and edge cases remain open, so the inventory row stays
`discovered`.

The current staged gates pass: Phase 5 at 2,264/2,264 assertions, Phase 6 at
1,062/1,062, and Phase 7 at 1,378/1,378 (`build/phase5-compound-trigger.log`,
`build/phase6-compound-trigger.log`, and `build/phase7-compound-trigger.log`).
These results establish the tested lifecycle, hash collisions exercised by the
batch, per-shape compound lifecycle, sleeping-overlap retention, and observed
event-mask behavior. The row remains discovered because broader allocation
and list edge cases still need oracle-backed fixtures and formal mutation
falsification. Public Physics headers were not changed.
