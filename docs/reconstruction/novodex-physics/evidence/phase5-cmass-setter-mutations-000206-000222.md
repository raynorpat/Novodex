# Phase 5 center-of-mass setter mutation closures

The registered `NxPhysicsActorCMassTests` differential begins from an exact
oracle/candidate baseline (`stdout_delta=0`, exact stderr). Eight reconstructed
Phase 5 setters were independently falsified by changing the field written by
that setter in a temporary candidate build. In every mutant both processes
exited 0, stderr remained exact, and the differential reported a nonzero stdout
delta. Restoring the source bytes and rebuilding returned each case to an exact
differential:

| Row | Isolated mutation | Mutant stdout delta |
|---|---|---:|
| `phys_fn_000206` | Zeroed the world center-of-mass position store at `+0x158`. | 148 |
| `phys_fn_000208` | Zeroed the world center-of-mass orientation store at `+0x134`. | 150 |
| `phys_fn_000210` | Zeroed the local center-of-mass position store at `+0x100` in the pose setter. | 28 |
| `phys_fn_000212` | Zeroed the local center-of-mass position store at `+0x100`. | 52 |
| `phys_fn_000214` | Zeroed the local center-of-mass orientation store at `+0xdc`. | 56 |
| `phys_fn_000218` | Replaced global-pose position conversion with a zero local-position store. | 120 |
| `phys_fn_000220` | Replaced global-position conversion with a zero local-position store. | 58 |
| `phys_fn_000222` | Replaced global-orientation conversion with a zero local-orientation store. | 60 |

An initial order-only mutation of the position conversion in row `000218` was
not observable in this fixture and was discarded; it is not used as proof. The
final mutation directly changes the row's stored result and is detected.

The exact runner was
`run_differential.ps1 -Targets NxPhysicsActorCMassTests`, using the pinned
oracle pair and the rebuilt Release candidate pair. Per-row mutant and restored
logs are in the ignored worktree build directory as `cmass-<row>-mutant-
differential.log` and `cmass-<row>-restored-differential.log`.

The proof covers the eight setter entries above. `phys_fn_000216` is recorded
separately as a continuation of row `000214`'s function body and is not claimed
closed by the input-store mutations in this packet.
