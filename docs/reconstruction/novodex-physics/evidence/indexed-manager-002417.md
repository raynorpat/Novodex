# Shared indexed-manager insertion: `phys_fn_002417`

IDA identifies `phys_fn_002417` at RVA `0x0005bc90` as the shared insertion helper called by the dynamic-body and shape registration paths (`phys_fn_002421` and `phys_fn_002423`). The helper expands parallel per-ID storage in 256-entry chunks, writes the occupied sentinel and reverse active index, and appends the ID to the active list. The candidate models these writes in `nxSceneAuxRegisterRecord` and `nxSceneAuxRegisterShape` in `Physics/src/Scene.cpp`.

`NxPhysicsBodyCreationTests` now reads both manager records after out-of-order ID reuse and after creating 257 simultaneous bodies. It checks the record and shape occupancy sentinels, reverse-index-to-active-list identity, record/shape pointers, active counts, and auxiliary storage capacity. The oracle and candidate match exactly, including the transition through ID `0x100` (`stdout_delta=0`, exact stderr; `build/phase5-next/aux-manager-002417-green.log`).

Two independent mutations were measured against this differential: clearing the body-manager occupancy sentinel and clearing the shape-manager occupancy sentinel each produced `stdout_delta=6` with both processes exiting 0 (`build/phase5-next/aux-manager-002417-mutation.log` and `build/phase5-next/aux-manager-002417-shape-mutation.log`).

The row remains owned by Phase 2 and is discharged by Phase 3 because the shared helper has no Phase 2 public entry point; the passing Phase 3 body-creation differential drives both callers. The same target remains registered in Phase 5. No public Physics header changed.
