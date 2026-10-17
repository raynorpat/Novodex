# NpScene run and timing source/map linkage (`phys_fn_000333`–`000342`)

The candidate source and linker-map associations are now recorded for the six `NpScene` run/timing census rows. IDA confirms the oracle routines at RVAs `0xCA30` (`flushStream`, 8 bytes), `0xCA40` (`startRun`, 80 bytes), `0xCA90` (`finishRun`, 77 bytes), `0xCAE0` (`setTiming`, 97 bytes), `0xCB50` (`getTiming`, 49 bytes), and `0xCB90` (`runFor`, 123 bytes). Their candidate implementations are in `Physics/src/NpScene.cpp`, object `NpScene.obj`:

| Row | Candidate symbol | Map VA | Oracle behavior |
| --- | --- | --- | --- |
| `000333` | `?flushStream@NpScene@@UAEXXZ` | `0x10042530` | Checks the stored fluid pointer through the oracle's no-op `NxFluidAssert`. |
| `000335` | `?startRun@NpScene@@UAEXM@Z` | `0x100442d0` | Emits the once-only warning and forwards elapsed time to `simulate`. |
| `000336` | `?finishRun@NpScene@@UAEXXZ` | `0x100424e0` | Emits the once-only warning and calls blocking `fetchResults`. |
| `000338` | `?setTiming@NpScene@@UAEXMIW4NxTimeStepMethod@@@Z` | `0x10044150` | Takes the write lock, forwards the timing tuple, and reports lock failure. |
| `000340` | `?getTiming@NpScene@@UBEXAAMAAIAAW4NxTimeStepMethod@@@Z` | `0x10042ca0` | Takes the read lock, copies three timing outputs, and unlocks. |
| `000342` | `?runFor@NpScene@@UAEXMMIW4NxTimeStepMethod@@@Z` | `0x10043e40` | Emits the once-only warning, then calls set-timing, simulate, flush, and blocking fetch in order. |

The existing row-specific dynamic proofs remain authoritative: the public no-op/mutation differential is recorded for `000333`; dispatch ordering and once-only reporting are covered for `000335`, `000336`, and `000342`; and lock/output behavior is covered for `000338` and `000340`. The associated detailed behavioral evidence is in `phase7-npscene-legacy-run-wrappers-000335-000336-000342.md` and `phase7-npscene-timing-wrappers-000338-000340.md`. This update closes the missing source/map ledger links; it does not claim additional semantic coverage.
