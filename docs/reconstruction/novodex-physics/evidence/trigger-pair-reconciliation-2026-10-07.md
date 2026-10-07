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

The public simulation corpus covers one pair through enter/stay/leave, eight
simultaneous pairs through the same transitions, and two trigger shapes on one
actor overlapping the same dynamic sphere. The compound fixture verifies each
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
falsification covers lookup matching in the lifecycle fixture; duplicate
identical pairs, compound expansion, unsupported overlap slots, and broader
allocation/list edge cases remain open, so the inventory row stays
`discovered`.

The current staged gates pass: Phase 5 at 2,264/2,264 assertions, Phase 6 at
1,062/1,062, and Phase 7 at 1,378/1,378 (`build/phase5-compound-trigger.log`,
`build/phase6-compound-trigger.log`, and `build/phase7-compound-trigger.log`).
These results establish the tested lifecycle, hash collisions exercised by the
batch, per-shape compound lifecycle, sleeping-overlap retention, and observed
event-mask behavior. The row remains discovered because duplicate identical
pairs, unsupported overlap slots, and broader allocation/list edge cases still
need oracle-backed fixtures and formal mutation falsification. Public Physics
headers were not changed.
