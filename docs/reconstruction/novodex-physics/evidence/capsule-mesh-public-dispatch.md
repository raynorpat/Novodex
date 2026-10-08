# Public capsule/mesh dispatch and triangle-plane records

## Change

The public scene path could create a triangle mesh and a capsule, but the shape-pair table did not route capsule/mesh contact or trigger-overlap pairs. Once contact dispatch was enabled, the candidate reached `NxContactCapsuleMesh` with no per-triangle plane records: the oracle reads those records from the internal mesh at `TriangleMesh + 0x24`, and the candidate had built topology without calling `nxInternalMeshBuildTriangleData`.

`ShapePairFunctionTable` now binds the contact and overlap entries at `[3][4]`. `TriangleMesh::loadFromDesc` builds topology first and then fills the triangle planes before constructing the OPCODE model. The private layout/vtable harnesses use link-only capsule/mesh stubs, as they do for other mesh contact handlers; the production DLL and public simulation test use the real handlers.

The staged-pair simulation test creates an isolated scene with two separated 4x4-cell mesh patches, settles a capsule on the first patch, moves it onto the second, and requires actual contact callbacks on both. It rejects missing callbacks, missing contact points, or unrelated pairs. Public headers were not changed.

## Verification

Command:

```powershell
cmake --build build --config Release --target NxPhysics NxPhysicsMeshSimulationTests
powershell -NoProfile -File docs/reconstruction/novodex-physics/tools/run_differential.ps1 -Targets NxPhysicsMeshSimulationTests -PairsRoot D:\github\Novodex\build\segment-route-pairs
```

The pinned oracle and candidate both report:

```text
simulation mesh-capsule-multinode initial_calls=10 second_calls=1 unexpected=0 events=0000000a patches=1 points=6 y=3f7de83e
differential target=NxPhysicsMeshSimulationTests oracle_exit=0 candidate_exit=0 stdout_delta=0 stderr_exact=True
differential=pass
```

A dispatch-removal mutation (removing `mFunction[0][3][4]`) produced candidate `initial_calls=0 second_calls=0`, `oracle_exit=0`, `candidate_exit=1`, and `stdout_delta=2`. Restoring the entry and rebuilding returned to `stdout_delta=0`.

The full Phase 5 gate passed after adding the link-only harness definitions, including all 2,563 registered coverage assertions. `test_gate_targets.py -k coverage` and `-k floor` passed; inventory validation reports `unexplained=0`.

## Scope limits

This proves public capsule/mesh contact dispatch, the candidate mesh plane records needed by that handler, and contact discovery after moving between separated patches. A separate capsule-trigger fixture emitted no enter callback in the pinned oracle, so it was discarded rather than counted; public trigger-overlap coverage remains open. This test also did not detect a mutation of `Segment::SquareDistance`; DLL-level routing for `phys_fn_005493` remains open and must not be claimed from this result.
