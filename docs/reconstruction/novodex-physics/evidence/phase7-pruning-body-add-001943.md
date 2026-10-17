# Scene pruning body registration (`phys_fn_001943`)

`phys_fn_001943` is the 270-byte body-registration helper called by `nxSceneAddShape` (row `000531`). It links a body root to its pruning collection, assigns its pruning type and single/group kind, inserts the root and current compound children into the scene pruners, and appends the root to the collection's `SdkContainer` at `+0x78`. The candidate transcription is `nxPruningAddBody` in `Physics/src/Scene.cpp`; the static helper is inlined into `nxSceneAddShape` in the Release DLL.

`NxPhysicsActorShapeMutationTests` asserts two explicit row markers. A dynamic single-shape body must appear once in the pruning list and carry root type 2, kind 1, and a live pruner/handle. A dynamic compound body must appear once with root type 2/kind 2 and all three children must have kind 0 and valid pruner/handle registration.

Three candidate mutations were detected by the registered actor-shape differential:

- Replacing `nxPruningAddBody` with a no-op changes root state and list membership (`stdout_delta=122`).
- Setting each compound child's kind to 1 instead of 0 crashes the candidate (`candidate_exit=-1073741819`, oracle exit 0).
- Omitting the root-list count increment leaves the single/group roots absent from the collection and is rejected (`stdout_delta=4`).

After restoring the source and forcing a Release rebuild, both row markers matched the oracle and the differential was exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`). The candidate DLL used for the restored focused run had SHA-256 `14c39ae15cfa67047164c8f9452ed846d56375c49dc6616b6dc8cd14f6894d08`; the oracle DLL remained pinned at `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

The full logs, compressed deterministically with hashes, and the whole-body mutation patch are under [raw/phase7-pruning-body-add-001943-20261017](raw/phase7-pruning-body-add-001943-20261017/). Fresh full Phase 5 and Phase 7 gates pass with 2,852/2,852 and 2,986/2,986 registered coverage assertions, respectively; the complete transcripts and source/compressed hashes are recorded in the raw manifest.
