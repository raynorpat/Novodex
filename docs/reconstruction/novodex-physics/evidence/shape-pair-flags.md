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

Not covered here: same-shape error reporting, actor-level multi-shape expansion,
actor-group pair flags, and the broader contact-report state machine.
