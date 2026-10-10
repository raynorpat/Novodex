# `NxSceneInternal::removeEffector` mutation closure (`phys_fn_000573`)

`phys_fn_000573` is the 109-byte Scene row at RVA `0x00010900`. It removes an effector from the singly linked Scene list at `Scene + 0x5a4`, handling both the head and successor cases and clearing the removed node's next link. The registered Phase 7 `NxPhysicsEffectorTests` fixture reaches it through explicit effector release and actor teardown.

The clean baseline matched the pinned oracle (`oracle_exit=0`, `candidate_exit=0`, `stdout_delta=0`, exact stderr). For the row-specific mutation, the complete `removeEffector` body was replaced with a no-op. The subsequent public release destroyed the effector but left its freed node linked from the Scene head; the candidate then crashed while the fixture inspected the released list. The registered differential rejected the mutant (`oracle_exit=0`, `candidate_exit=-1073741819`, `stdout_delta=48`, exact stderr). Mutant `NxPhysics.dll` SHA-256: `c20908d2061d51e51e3062ae8bd0e289b877567cedda6b887b5fc5e37f12b5ea`.

`Scene.cpp` was restored byte-for-byte (SHA-256 `5806d9cb903e2c11d863ee69ae6202d3c6c7ce481b0937e73c99080383bd7cf9` before and after). After rebuilding, the restored differential returned both exits zero, `stdout_delta=0`, and exact stderr. Restored `NxPhysics.dll` SHA-256: `1f9d19989adaf48df14e2e6919054d956a96403b716d1081c26d5716934d0bd6`.

The fresh full Phase 7 gate passes with 1,491 coverage assertions against the 1,457 floor. Inventory validation passes at 6,338 functions, 5,138 data objects, and zero unexplained entries. The focused completion-report tests pass 11/11; work-unit and inventory tests pass 12/12 and 251/251. Phase 8 terminal closure remains open.
