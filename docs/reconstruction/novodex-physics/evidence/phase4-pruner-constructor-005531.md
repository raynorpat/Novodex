# Phase 4 Pruner constructor: `phys_fn_005531`

`phys_fn_005531` is `Pruner::Pruner` at RVA `0x000f1550`. IDA shows the constructor initializing the pool member, writing its vtable, registering its per-instance handle in the process-wide OPCODE registry, and calling the empty-bounds initializer at `0x00053810` for the six words at `this+0x1c`.

The original bounds-only proof is preserved in the historical raw transcript at [raw/phase4-pruner-assertion-005531-20261014](raw/phase4-pruner-assertion-005531-20261014/). On 2026-10-15 the x87 candidate path was completed: `Pruner` now contains `NxPrunerRegistration` at +0x34, its base constructor and destructor register and unregister, `RemoveObject` and `UpdateObject` increment +0x38, and CMake includes the registry implementation in the DLL and source-linked harnesses.

The public `NxPhysicsSceneRaycastTests` path now follows the public Scene wrapper to both the static and dynamic pruners. It checks the six exact empty-bound words and requires the oracle registration words: static handle/timestamp `00000000.00000006`, dynamic `00000001.00000004`. The restored focused oracle/candidate pair is exact (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`).

For row-specific falsification, a throwaway mutation omitted only `nxPrunerRegister(mRegistration)` from `Pruner::Pruner`, then rebuilt the DLL and ran the registered staged pair. The oracle retained the expected registration words; the candidate reported poisoned words `cdcdcdcd.cdcdcdd3` and `cdcdcdcd.cdcdcdd1` and failed (`oracle_exit=0`, `candidate_exit=1`, `stdout_delta=2793`). The restored source was explicitly touched and rebuilt to prevent MSBuild from retaining the mutant object. Phases 4, 5 and 7 pass at 532/532, 2739/2739 and 2870/2870. Full transcripts and SHA-256 hashes are in [raw evidence](raw/phase4-pruner-registration-005531-20261015/).

Registration-buffer allocation failure, registry capacity exhaustion, and destructor handle-reuse behavior still need separate targeted proofs. This row’s successful constructor path is dynamically gated; this does not close those neighboring behaviors or complete the DLL.
