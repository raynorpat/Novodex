# Phase 7: Scene trigger/contact callback accessors (`phys_fn_000549`–`000555`)

The private `NxSceneInternal` now exposes four out-of-line field accessors used by the public `NpScene` wrappers. The trigger report pointer is stored/read at `Scene+0x6b0`; the contact report pointer is stored/read at `Scene+0x6b4`. Public setters/getters take their existing scene write/read locks, and the public Physics headers remain unchanged.

The registered `NxPhysicsSimulationTests` staged-pair target was run after rebuilding `NxPhysics` and the test executable. Four independent helper mutations were rejected:

| Row | Mutation | Differential result |
|---|---|---|
| `000549` | Replace trigger callback store with a no-op | `oracle_exit=0`, `candidate_exit=1`, `stdout_delta=3990` |
| `000551` | Return null instead of the trigger callback field | both exit 0, `stdout_delta=2`, stderr exact |
| `000553` | Replace contact callback store with a no-op | `oracle_exit=0`, `candidate_exit=1`, `stdout_delta=3990` |
| `000555` | Return null instead of the contact callback field | `oracle_exit=0`, `candidate_exit=1`, `stdout_delta=3990` |

After restoring all four helpers, the same target passed with `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, and exact stderr. The restored candidate `NxPhysics.dll` SHA-256 was `5e44ae10c835aff5d7368fa5c9de8b80891127dfab5b679b3323201737c4e1ae`; the staged `NxFoundation.dll` SHA-256 was `4901bf502fe41bc01f6b5ab63d7bd07a9018a1704a4899a5b023e8ee28a0206d`.

The full staged transcripts preserve each mutation's loaded-module identity and detector output under [raw/phase7-scene-callback-accessors-000549-000555-20261014/](raw/phase7-scene-callback-accessors-000549-000555-20261014/). The fresh full Phase 7 gate passed all 1,770/1,770 registered coverage assertions; its compressed output is archived beside the mutation transcripts. The Phase 7 ledger now counts 274 closed rows and 286 deferred rows.
