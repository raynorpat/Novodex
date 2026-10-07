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

The public simulation corpus covers one pair through enter/stay/leave and eight
simultaneous pairs through the same transitions. The batch actors sleep while
overlapping, then move outside. It also configures a trigger with only
`NX_TRIGGER_ON_STAY`: the pinned oracle still reports enter, stay, and leave,
so the candidate now mirrors the observed rule that any enabled trigger event
bit admits all transitions. Both focused differentials are exact; the expanded
batch transcript is recorded in
`build/trigger-pair-eight-mask-restored.log` (`oracle_exit=0`,
`candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). A same-checkout
mutation that made the pair lookup always miss is caught by the trigger
lifecycle fixture (`build/trigger-pair-hash-mutation.log`). This mutation was
not run from a throwaway archive and is not formal closure evidence.

The current staged gates pass: Phase 5 at 2,263/2,263 assertions, Phase 6 at
1,061/1,061, and Phase 7 at 1,377/1,377. These results establish the tested
lifecycle, hash collisions exercised by the batch, sleeping-overlap retention,
and observed event-mask behavior. The row remains discovered because duplicate
identical pairs, compound expansion, unsupported overlap slots, and broader
allocation/list edge cases still need oracle-backed fixtures and formal
mutation falsification. Public Physics headers were not changed.
