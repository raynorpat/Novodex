# Shape-pair flag access

This slice reconstructs the public `NxScene::setShapePairFlags()` and
`getShapePairFlags()` path without changing public Physics headers.

The oracle's internal `Scene` routines (`phys_fn_000511`, `000513`, `000589`,
and `000590`) use the shape IDs at `+0xd4` as keys in the scene pair hash. A
stored immediate value returns its low 29 flag bits; a record-backed value
returns the same mask from its first word. Setting zero frees a record-backed
entry and erases the key. The actor-pair setter uses the same storage contract.

Public `NxShape` values are handles. `Actor::releaseShape()` reads the internal
shape pointer from handle `+8`, and `getShapes()` exposes the public handle
array. `NpScene` therefore unwraps each public handle before passing the
underlying shape records to the pair-hash helpers. Passing the public handle
itself produces a second pair key: the getter alone may appear correct, while
the pair count and pair-array classification diverge.

`tests/PhysicsSimulationTests.cpp` exercises three states on an actual
contacting shape pair:

- Inline `NX_IGNORE_PAIR`: getter returns `00000001`; the pair array contains
  one shape pair (`isActorPair() == 0`) with flags `00000001`.
- Record-backed `NX_NOTIFY_ON_TOUCH`: getter returns `00000008`.
- Clear with zero: getter returns `00000000`.

The pre-implementation run failed four output comparisons against the pinned
oracle. After unwrapping the handles at the `NpScene` boundary, the focused
`NxPhysicsSimulationTests` differential passed with `stdout_delta=0` and exact
stderr (`D:\FlamingEnt__\novodex-analysis\pairs\shape-pair-verified-20261004.log`).
The candidate and oracle both exited 0. The CMake Release build of `NxPhysics`
and `NxPhysicsSimulationTests` succeeded. The three oracle outputs are
registered in Phase 7, whose coverage floor is now 1,315.

The follow-up `NxPhysicsPairFlagTests` covers a two-child actor pair and the
same-shape rejection path. For a flagged pair containing a compound actor, the
oracle returns the public actor handles in order (`objects=0.1`), marks the
entry as an actor pair, and preserves `NX_IGNORE_PAIR`. The candidate initially
returned internal body pointers; `NxSceneInternal::getPairFlagArray()` now maps
each internal body back to its public actor by scanning the scene actor range.

The same target passes one public shape reference twice to
`setShapePairFlags()`. The oracle reports `NXE_INVALID_PARAMETER` at
`Scene.cpp:0x388` with the exact message, then leaves the flag value at zero,
keeps the original pair count at one, and emits no self-pair entry. The first
candidate run crashed because the pair-hash helper received duplicate shape
IDs. Oracle listing `phys_fn_000590` shows the missing guard: identical shape
references must be rejected before the hash call. `NxSceneInternal::setShapePairFlags()`
now performs that check and reports the same error.

The focused pair-flag differential passes with `oracle_exit=0`,
`candidate_exit=0`, `stdout_delta=0`, and exact stderr. The Phase 7 registry
requires both the error line and the unchanged-state result; the current Phase 7
coverage floor is 1,318. The fresh Phase 5 gate also passes after its coverage
floor was corrected to match the 2,042 currently registered lines.

Not covered here: actor-group pair flags and the broader contact-report state
machine. The complete Phase 7 gate was not rerun as part of this follow-up.
