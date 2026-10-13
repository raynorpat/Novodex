# Phase 7 contact-patch refresh (`phys_fn_000891`)

`NxActorPair::row000891` refreshes stored contact anchors and normals, re-anchors or drops stale patches, and accumulates dropped normal force. The existing registered sliding-sphere scene runs 60 fetched steps and exposes the resulting aggregate through its contact callback.

A no-op mutation replaced the row body and was rebuilt into the candidate DLL. Callback counts and final actor motion still appeared, but summed friction changed from `3d5ae33a.00000000.00000000` to `4096ae9b.00000000.00000000`; the staged pair differential reported `stdout_delta=118` with both children exiting zero. Restoring the implementation returned the complete output to an exact match (`stdout_delta=0`, stderr exact). The mutant and restored transcripts are in [raw evidence](raw/phase7-contact-patch-refresh-000891/).

Fresh Phase 5, Phase 6, and Phase 7 gates pass at 2,702/2,702, 1,635/1,635, and 1,744/1,744 assertions. See [Phase 5](raw/phase7-contact-patch-refresh-000891/phase5-gate.log.gz), [Phase 6](raw/phase7-contact-patch-refresh-000891/phase6-gate.log.gz), and [Phase 7](raw/phase7-contact-patch-refresh-000891/phase7-gate.log.gz) transcripts. The current Phase 7 ledger closes 250 functions and defers 310; full-DLL reconstruction remains in progress.
