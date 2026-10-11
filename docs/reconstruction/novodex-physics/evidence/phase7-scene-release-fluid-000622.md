# Phase 7 Scene `releaseFluid` row (phys_fn_000622)

`phys_fn_000622` is `NxSceneInternal::releaseFluid` at oracle RVA `0x00011620` (140 bytes). IDA 12.1.2 shows the callback-reentry guard, a call to the manager's fluid-release operation when Scene `+0x61c` is non-null, and deleting destruction plus clearing of the manager when its primary fluid array becomes empty.

The registered `NxPhysicsSimulationTests` fixture reaches this row through the public `NxScene::releaseFluid` API. Its disabled-backend case checks the release warning and that Scene `+0x61c` is cleared. A second seeded internal array fixture checks swap removal, secondary-array updates, and the fluid deleting-destructor dispatch.

The candidate-only mutation inserted an immediate return at the start of `NxSceneInternal::releaseFluid`, then rebuilt `NxPhysics.dll` and ran the registered staged-pair differential. Both processes exited zero with exact stderr, but the oracle/candidate stdout differed by 4 bytes: the public release retained its manager and warning was omitted, and the seeded arrays/destructor were left untouched. The mutation was rejected. Removing the mutation and rebuilding returned the entire simulation target to both exits zero, `stdout_delta=0`, and exact stderr.

Candidate Physics hashes: mutation `f515d3058bc10ce3f775e826de3f2b874f19bd58124001a4f4ead04ef4742d10`; restored `8aa092a476a6f7a09ebe3fd9bce1991e986bdcd31ec6ea86aa255a6da835384b`. Oracle Physics remains pinned to `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`; oracle Foundation is `7e0596e45af2f1ab937a948100528e51c80dfd02b295ba678898081883ae0990`; candidate Foundation is `d30e43dff46391931be74243c9d7859b049aa7cdeadc2a3435d17b26b48b5970`.

Exact staged-pair transcripts are retained losslessly in `raw/phase7-scene-release-fluid-000622/mutant-staged-pair.log.gz` and `raw/phase7-scene-release-fluid-000622/restored-control-staged-pair.log.gz`. The row proof covers the registered disabled-backend empty-manager and seeded-array lifecycle. Callback reentry and extension-backed live-fluid notifications remain open; Phase 8 full-image closure remains open. No public Physics header changed.
