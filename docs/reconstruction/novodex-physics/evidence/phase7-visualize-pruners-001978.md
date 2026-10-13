# Phase 7: pruning visualization row 001978

`NxSceneInternal::visualize` calls `phys_fn_001978` at RVA `0x0004c980` after it creates the Scene renderable. IDA confirms four independent parameter paths: static and bounded-dynamic pruning trees, unbounded/free-pruner world boxes, and the SAP broad-phase entry callback. Tree nodes are red, free-pruner boxes are blue, and SAP boxes are cyan. Tree and pool output goes to the PhysicsSDK-wide cached renderable; SAP output goes to the Scene renderable.

`Physics/src/SceneVisualize.cpp` now builds the static tree through `StaticPruner::BuildTree`, draws its node AABBs in preorder, builds a current-pool OPCODE tree view for the bounded-dynamic branch, draws free-pruner boxes, and decodes the six SAP endpoint pointers when the broad-phase object exists. `PhysicsSDK::createScene` clears the SDK-wide diagnostic renderable before a later Scene begins, matching the public fixture's fresh-scene output.

The registered `NxPhysicsSceneVisualizeTests` differential exercises SAP, static, dynamic, and free parameters independently. In this fixture SAP has no broad-phase object, so that case verifies the empty callback path. Static, dynamic, and free render output matches the oracle, including line order, float words, colors, and renderable counts. Restored run: `oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, `stderr_exact=True`.

A mutation replaced `nxSceneVisualizeCollisionPruners` with a no-op and rebuilt the candidate DLL. The registered differential rejected it with `stdout_delta=1963` (`oracle_exit=0`, `candidate_exit=0`). The no-op mutation transcript is archived at `evidence/raw/phase7-visualize-pruners-001978-20261013/no-op-mutation.log`.
