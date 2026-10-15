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
two stays and one leave through the missing-dispatcher branch. An empty
previous/current pair-list case also verifies the oracle's header swap and
empty-list reset. Candidate state matches all four oracle cases exactly.

A temporary unconditional `return -1` in `nxSceneFindTriggerPair` is rejected:
the overlap-true case reports four candidate events and four pairs, while the
false and null-slot cases report four candidate events. The fixture exits 1.
After restoring the source and rebuilding, the full collision harness passes
with `mismatches=0`
(`build/trigger-pair-duplicates-restored-full-collision.log`). Unsupported
overlap slots beyond the measured sphere/box null entry, plus broader
allocation/list edge cases, remain open, so the inventory row stays
`discovered`.

The current staged gates pass: Phase 5 at 2,264/2,264 assertions, Phase 6 at
1,062/1,062, and Phase 7 at 1,378/1,378 (`build/phase5-compound-trigger.log`,
`build/phase6-compound-trigger.log`, and `build/phase7-compound-trigger.log`).
These results establish the tested lifecycle, hash collisions exercised by the
batch, per-shape compound lifecycle, sleeping-overlap retention, and observed
event-mask behavior. The row remains discovered because other unsupported
overlap slots and broader allocation/list edge cases still need oracle-backed
fixtures and formal mutation falsification. Public Physics headers were not
changed.
